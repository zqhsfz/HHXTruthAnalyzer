#ifndef HHXTruthAnalyzer_NtupleMaker_H
#define HHXTruthAnalyzer_NtupleMaker_H

// Infrastructure includes
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"

// EL includes
#include <EventLoop/StatusCode.h>
#include <EventLoop/Algorithm.h>

// package inclues
#include "AutoHists/Analysis_AutoTrees.h"

// EDM includes
#include "xAODEventInfo/EventInfo.h"
#include "xAODTruth/TruthParticleContainer.h"

class NtupleMaker : public EL::Algorithm
{
  // put your configuration variables here as public variables.
  // that way they can be set directly from CINT and python.
public:
  // float cutValue;

  bool         m_debug;
  std::string  m_outputFileName;
  std::string  m_outputTreeName;



  // variables that don't get filled at submission time should be
  // protected from being send from the submission node to the worker
  // node (done by the //!)
public:
  // Tree *myTree; //!
  // TH1 *myHist; //!

  xAOD::TEvent* m_event; //!
  xAOD::TStore* m_store; //!

  Analysis_AutoTrees* m_ntupleMaker; //!

  const xAOD::EventInfo* m_eventInfo; //!

  // this is a standard constructor
  NtupleMaker ();

  // these are the functions inherited from Algorithm
  virtual EL::StatusCode setupJob (EL::Job& job);
  virtual EL::StatusCode fileExecute ();
  virtual EL::StatusCode histInitialize ();
  virtual EL::StatusCode changeInput (bool firstFile);
  virtual EL::StatusCode initialize ();
  virtual EL::StatusCode execute ();
  virtual EL::StatusCode postExecute ();
  virtual EL::StatusCode finalize ();
  virtual EL::StatusCode histFinalize ();

  // initialization functions
  bool NtupleSvcInit();

  // utils
  static bool SortPt(const xAOD::IParticle* p1, const xAOD::IParticle* p2) { return p1->pt() > p2->pt(); }
  const xAOD::TruthParticle* GetFinalState(const xAOD::TruthParticle* initParticle);

  // this is needed to distribute the algorithm to the workers
  ClassDef(NtupleMaker, 1);
};

#endif
