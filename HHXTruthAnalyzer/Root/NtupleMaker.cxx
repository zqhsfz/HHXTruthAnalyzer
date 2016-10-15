// EL includes
#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>
#include "EventLoop/OutputStream.h"

// package includes
#include "xAODAnaHelpers/HelperClasses.h"
#include "xAODAnaHelpers/HelperFunctions.h"
#include <xAODAnaHelpers/tools/ReturnCheck.h>
#include <HHXTruthAnalyzer/NtupleMaker.h>

// C++ standard
#include <iostream>
#include <algorithm>
#include <vector>

// this is needed to distribute the algorithm to the workers
ClassImp(NtupleMaker)



NtupleMaker :: NtupleMaker ()
{
  // Here you put any code for the base initialization of variables,
  // e.g. initialize all pointers to 0.  Note that you should only put
  // the most basic initialization here, since this method will be
  // called on both the submission and the worker node.  Most of your
  // initialization code will go into histInitialize() and
  // initialize().

  Info("NtupleMaker()", "Calling constructore ...");

  // options 

  m_debug = false;
  m_outputFileName = "TEST";
  m_outputTreeName = "TEST";

  // members

  m_event = 0;
  m_store = 0;
  m_ntupleMaker = 0;
  m_eventInfo = 0;
}



EL::StatusCode NtupleMaker :: setupJob (EL::Job& job)
{
  // Here you put code that sets up the job on the submission object
  // so that it is ready to work with your algorithm, e.g. you can
  // request the D3PDReader service or add output files.  Any code you
  // put here could instead also go into the submission script.  The
  // sole advantage of putting it here is that it gets automatically
  // activated/deactivated when you add/remove the algorithm from your
  // job, which may or may not be of value to you.

  Info("setupJob()", "Calling seutpJob ...");

  job.useXAOD();
  xAOD::Init("NtupleMaker").ignore();  // call before opening first file

  // setup output stream
  EL::OutputStream outputStream(m_outputFileName);
  job.outputAdd(outputStream);

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: histInitialize ()
{
  // Here you do everything that needs to be done at the very
  // beginning on each worker node, e.g. create histograms and output
  // trees.  This method gets called before any input files are
  // connected.

  if(!NtupleSvcInit()) return EL::StatusCode::FAILURE;

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: fileExecute ()
{
  // Here you do everything that needs to be done exactly once for every
  // single file, e.g. collect a list of all lumi-blocks processed
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: changeInput (bool firstFile)
{
  // Here you do everything you need to do when we change input files,
  // e.g. resetting branch addresses on trees.  If you are using
  // D3PDReader or a similar service this method is not needed.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: initialize ()
{
  // Here you do everything that you need to do after the first input
  // file has been connected and before the first event is processed,
  // e.g. create additional histograms based on which variables are
  // available in the input files.  You can also create all of your
  // histograms and trees in here, but be aware that this method
  // doesn't get called if no events are processed.  So any objects
  // you create here won't be available in the output if you have no
  // input events.

  Info("initialize()", "Calling initialize ...");

  // setup the xAOD readout stuff

  m_event = wk()->xaodEvent();
  m_store = wk()->xaodStore();

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: execute ()
{
  // Here you do everything that needs to be done on every single
  // events, e.g. read input variables, apply cuts, and fill
  // histograms and trees.  This is where most of your actual analysis
  // code will go.

  if(m_debug) Info("execute()", "Calling execute ...");

  // obtain basic event information
  m_eventInfo = 0;  
  RETURN_CHECK("NtupleMaker::execute()", HelperFunctions::retrieve(m_eventInfo, "EventInfo", m_event, m_store, m_debug), "");

  // obtain container of all truth particles
  const xAOD::TruthParticleContainer* m_truthParticleContainer = 0;
  RETURN_CHECK("NtupleMaker::execute()", HelperFunctions::retrieve(m_truthParticleContainer, "TruthParticles", m_event, m_store, m_debug), "");
  if(m_truthParticleContainer == 0){
    Warning("execute()", "Unable to get \"TruthParticles\" container! Current event will be skipped");
    return EL::StatusCode::SUCCESS;
  }

  // obtain list of neutralinos (sorted by pT)
  std::vector<const xAOD::TruthParticle*> FinalNeutralinos;
  for(auto truthParticle : *m_truthParticleContainer){
    if( std::abs(truthParticle->pdgId()) == 1000022 ){
      auto finalNeutralino = GetFinalState(truthParticle);
      if(std::find(FinalNeutralinos.begin(), FinalNeutralinos.end(), finalNeutralino) == FinalNeutralinos.end()) 
        FinalNeutralinos.push_back(finalNeutralino);
    }
  }
  std::sort(FinalNeutralinos.begin(), FinalNeutralinos.end(), NtupleMaker::SortPt);

  if(FinalNeutralinos.size() != 2){
    Warning("execute()", "Expect 2 neutralinos but only get %d! Current event will be skipped", int(FinalNeutralinos.size()));
    return EL::StatusCode::SUCCESS;
  }

  // obtain list of Higgs/Gravitino from neutralino
  std::vector<const xAOD::TruthParticle*> FinalHiggs;
  std::vector<const xAOD::TruthParticle*> FinalGravitinos;
  for(auto neutralino : FinalNeutralinos){
    // get Higgs/Gravitino out of neutralino
    const xAOD::TruthParticle* initHiggs = 0;
    const xAOD::TruthParticle* initGravitino = 0;
    for(unsigned int iChild = 0; iChild < neutralino->nChildren(); iChild++){
      auto child = neutralino->child(iChild);

      if( (initHiggs == 0) && (std::abs(child->pdgId()) == 25) ){
        initHiggs = child;
      }

      if( (initGravitino == 0) && (std::abs(child->pdgId()) == 1000039) ){
        initGravitino = child;
      }
    }


    // sanity check
    if(initHiggs == 0){
      Warning("execute()", "Expect Higgs out of neutralino, but get none! This event will be skipped");
      return EL::StatusCode::SUCCESS;
    }

    if(initGravitino == 0){
      Warning("execute()", "Expect Gravitino out of neutralino, but get none! This event will be skipped");
      return EL::StatusCode::SUCCESS;
    }

    // track to the last Higgs/Gravitino
    auto finalHiggs = GetFinalState(initHiggs);
    FinalHiggs.push_back(finalHiggs);

    auto finalGravitino = GetFinalState(initGravitino);
    FinalGravitinos.push_back(finalGravitino);
  }

  // obtain list of b-quarks out of Higgs decay
  std::vector<std::vector<const xAOD::TruthParticle*> > InitialBquarks;
  for(auto higgs : FinalHiggs){
    std::vector<const xAOD::TruthParticle*> DecayedInitialBquarks;

    if( (higgs->nChildren() != 2) || (std::abs(higgs->child(0)->pdgId()) != 5) || (std::abs(higgs->child(1)->pdgId()) != 5) ){
      if(m_debug){ // Conclusion: This sample is inclusive Higgs decay (i.e. it can decay to any products)
        Warning("execute()", "Expect H->bb decay, but get something else! This event will be skipped");

        for(unsigned int iChild = 0; iChild < higgs->nChildren(); iChild++){
          auto child = higgs->child(iChild);
          std::cout << iChild << " : " << child->pdgId() << std::endl;
        }
      }
      
      return EL::StatusCode::SUCCESS;
    }

    DecayedInitialBquarks.push_back(higgs->child(0));
    DecayedInitialBquarks.push_back(higgs->child(1));
    std::sort(DecayedInitialBquarks.begin(), DecayedInitialBquarks.end(), NtupleMaker::SortPt);

    InitialBquarks.push_back(DecayedInitialBquarks);
  }

  // Fill Ntuples
  m_ntupleMaker->ResetBranches();

  m_ntupleMaker->SetEventValue("runNumber", m_eventInfo->runNumber());
  m_ntupleMaker->SetEventValue("eventNumber", m_eventInfo->eventNumber());
  m_ntupleMaker->SetEventValue("channelNumber", m_eventInfo->mcChannelNumber());

  m_ntupleMaker->SetEventValue("nNeutralinos", FinalNeutralinos.size());
  m_ntupleMaker->SetEventValue("N1_Pt", FinalNeutralinos[0]->pt()/1000.);
  m_ntupleMaker->SetEventValue("N1_Eta", FinalNeutralinos[0]->eta());
  m_ntupleMaker->SetEventValue("N1_Phi", FinalNeutralinos[0]->phi());
  m_ntupleMaker->SetEventValue("N1_M", FinalNeutralinos[0]->m()/1000.);
  m_ntupleMaker->SetEventValue("N1_E", FinalNeutralinos[0]->e()/1000.);
  m_ntupleMaker->SetEventValue("N2_Pt", FinalNeutralinos[1]->pt()/1000.);
  m_ntupleMaker->SetEventValue("N2_Eta", FinalNeutralinos[1]->eta());
  m_ntupleMaker->SetEventValue("N2_Phi", FinalNeutralinos[1]->phi());
  m_ntupleMaker->SetEventValue("N2_M", FinalNeutralinos[1]->m()/1000.);
  m_ntupleMaker->SetEventValue("N2_E", FinalNeutralinos[1]->e()/1000.);

  m_ntupleMaker->SetEventValue("H1_Pt", FinalHiggs[0]->pt()/1000.);
  m_ntupleMaker->SetEventValue("H1_Eta", FinalHiggs[0]->eta());
  m_ntupleMaker->SetEventValue("H1_Phi", FinalHiggs[0]->phi());
  m_ntupleMaker->SetEventValue("H1_M", FinalHiggs[0]->m()/1000.);
  m_ntupleMaker->SetEventValue("H1_E", FinalHiggs[0]->e()/1000.);
  m_ntupleMaker->SetEventValue("H2_Pt", FinalHiggs[1]->pt()/1000.);
  m_ntupleMaker->SetEventValue("H2_Eta", FinalHiggs[1]->eta());
  m_ntupleMaker->SetEventValue("H2_Phi", FinalHiggs[1]->phi());
  m_ntupleMaker->SetEventValue("H2_M", FinalHiggs[1]->m()/1000.);
  m_ntupleMaker->SetEventValue("H2_E", FinalHiggs[1]->e()/1000.);

  m_ntupleMaker->SetEventValue("G1_Pt", FinalGravitinos[0]->pt()/1000.);
  m_ntupleMaker->SetEventValue("G1_Eta", FinalGravitinos[0]->eta());
  m_ntupleMaker->SetEventValue("G1_Phi", FinalGravitinos[0]->phi());
  m_ntupleMaker->SetEventValue("G1_M", FinalGravitinos[0]->m()/1000.);
  m_ntupleMaker->SetEventValue("G1_E", FinalGravitinos[0]->e()/1000.);
  m_ntupleMaker->SetEventValue("G2_Pt", FinalGravitinos[1]->pt()/1000.);
  m_ntupleMaker->SetEventValue("G2_Eta", FinalGravitinos[1]->eta());
  m_ntupleMaker->SetEventValue("G2_Phi", FinalGravitinos[1]->phi());
  m_ntupleMaker->SetEventValue("G2_M", FinalGravitinos[1]->m()/1000.);
  m_ntupleMaker->SetEventValue("G2_E", FinalGravitinos[1]->e()/1000.);

  m_ntupleMaker->SetEventValue("b11_Pt", InitialBquarks[0][0]->pt()/1000.);
  m_ntupleMaker->SetEventValue("b11_Eta", InitialBquarks[0][0]->eta());
  m_ntupleMaker->SetEventValue("b11_Phi", InitialBquarks[0][0]->phi());
  m_ntupleMaker->SetEventValue("b11_M", InitialBquarks[0][0]->m()/1000.);
  m_ntupleMaker->SetEventValue("b11_E", InitialBquarks[0][0]->e()/1000.);
  m_ntupleMaker->SetEventValue("b12_Pt", InitialBquarks[0][1]->pt()/1000.);
  m_ntupleMaker->SetEventValue("b12_Eta", InitialBquarks[0][1]->eta());
  m_ntupleMaker->SetEventValue("b12_Phi", InitialBquarks[0][1]->phi());
  m_ntupleMaker->SetEventValue("b12_M", InitialBquarks[0][1]->m()/1000.);
  m_ntupleMaker->SetEventValue("b12_E", InitialBquarks[0][1]->e()/1000.);
  m_ntupleMaker->SetEventValue("b21_Pt", InitialBquarks[1][0]->pt()/1000.);
  m_ntupleMaker->SetEventValue("b21_Eta", InitialBquarks[1][0]->eta());
  m_ntupleMaker->SetEventValue("b21_Phi", InitialBquarks[1][0]->phi());
  m_ntupleMaker->SetEventValue("b21_M", InitialBquarks[1][0]->m()/1000.);
  m_ntupleMaker->SetEventValue("b21_E", InitialBquarks[1][0]->e()/1000.);
  m_ntupleMaker->SetEventValue("b22_Pt", InitialBquarks[1][1]->pt()/1000.);
  m_ntupleMaker->SetEventValue("b22_Eta", InitialBquarks[1][1]->eta());
  m_ntupleMaker->SetEventValue("b22_Phi", InitialBquarks[1][1]->phi());
  m_ntupleMaker->SetEventValue("b22_M", InitialBquarks[1][1]->m()/1000.);
  m_ntupleMaker->SetEventValue("b22_E", InitialBquarks[1][1]->e()/1000.);

  m_ntupleMaker->AutoFill();

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: postExecute ()
{
  // Here you do everything that needs to be done after the main event
  // processing.  This is typically very rare, particularly in user
  // code.  It is mainly used in implementing the NTupleSvc.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: finalize ()
{
  // This method is the mirror image of initialize(), meaning it gets
  // called after the last event has been processed on the worker node
  // and allows you to finish up any objects you created in
  // initialize() before they are written to disk.  This is actually
  // fairly rare, since this happens separately for each worker node.
  // Most of the time you want to do your post-processing on the
  // submission node after all your histogram outputs have been
  // merged.  This is different from histFinalize() in that it only
  // gets called on worker nodes that processed input events.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode NtupleMaker :: histFinalize ()
{
  // This method is the mirror image of histInitialize(), meaning it
  // gets called after the last event has been processed on the worker
  // node and allows you to finish up any objects you created in
  // histInitialize() before they are written to disk.  This is
  // actually fairly rare, since this happens separately for each
  // worker node.  Most of the time you want to do your
  // post-processing on the submission node after all your histogram
  // outputs have been merged.  This is different from finalize() in
  // that it gets called on all worker nodes regardless of whether
  // they processed input events.
  return EL::StatusCode::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////

bool NtupleMaker :: NtupleSvcInit(){
  // create object and link it to the output file
  m_ntupleMaker = new Analysis_AutoTrees(m_outputTreeName);
  m_ntupleMaker->GetTree()->SetDirectory( wk()->getOutputFile(m_outputFileName) );

  // define event-level branches
  std::vector<TString> EventVariableList = {
    // basic event information
    "runNumber",
    "eventNumber",
    "channelNumber",

    // neutralinos (final)
    "nNeutralinos",
    "N1_Pt",
    "N1_Eta",
    "N1_Phi",
    "N1_M",
    "N1_E",
    "N2_Pt",
    "N2_Eta",
    "N2_Phi",
    "N2_M",
    "N2_E",

    // Higgs (final)
    "H1_Pt",
    "H1_Eta",
    "H1_Phi",
    "H1_M",
    "H1_E",
    "H2_Pt",
    "H2_Eta",
    "H2_Phi",
    "H2_M",
    "H2_E",

    // Gravitino (final)
    "G1_Pt",
    "G1_Eta",
    "G1_Phi",
    "G1_M",
    "G1_E",
    "G2_Pt",
    "G2_Eta",
    "G2_Phi",
    "G2_M",
    "G2_E",

    // b-quarks out of Higgs decay (initial)
    "b11_Pt",
    "b11_Eta",
    "b11_Phi",
    "b11_M",
    "b11_E",
    "b12_Pt",
    "b12_Eta",
    "b12_Phi",
    "b12_M",
    "b12_E",
    "b21_Pt",
    "b21_Eta",
    "b21_Phi",
    "b21_M",
    "b21_E",
    "b22_Pt",
    "b22_Eta",
    "b22_Phi",
    "b22_M",
    "b22_E",
  };

  m_ntupleMaker->SetEventVariableList(EventVariableList);

  if(!m_ntupleMaker->SetupBranches()){
    Error("NtupleSvcInit()", "Error in ntuple branch setting up!");
    return false;
  }

  return true;
}

const xAOD::TruthParticle* NtupleMaker :: GetFinalState(const xAOD::TruthParticle* initParticle){
  int abspdgId = std::abs(initParticle->pdgId());

  int nChild = initParticle->nChildren();
  for(int iChild = 0; iChild < nChild; iChild++){
    auto child = initParticle->child(iChild);
    
    if(std::abs(child->pdgId()) != abspdgId) continue;

    return GetFinalState(child);
  }

  return initParticle;
}

