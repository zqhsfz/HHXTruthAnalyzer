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

   RETURN_CHECK("NtupleMaker::execute()", HelperFunctions::retrieve(m_eventInfo, "EventInfo", m_event, m_store, m_debug), "");

   // Fill Ntuples
   m_ntupleMaker->ResetBranches();

   m_ntupleMaker->SetEventValue("runNumber", m_eventInfo->runNumber());
   m_ntupleMaker->SetEventValue("eventNumber", m_eventInfo->eventNumber());
   m_ntupleMaker->SetEventValue("channelNumber", m_eventInfo->mcChannelNumber());

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

    // etc ...
  };

  m_ntupleMaker->SetEventVariableList(EventVariableList);

  if(!m_ntupleMaker->SetupBranches()){
    Error("NtupleSvcInit()", "Error in ntuple branch setting up!");
    return false;
  }

  return true;
}

