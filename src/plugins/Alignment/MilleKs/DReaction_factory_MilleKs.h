// $Id$
//
//    File: DReaction_factory_MilleKs.h
// Created: Mon Feb 22 18:20:38 EST 2021
// Creator: keigo (on Linux ifarm1901.jlab.org 3.10.0-1062.4.1.el7.x86_64
// x86_64)
//

#ifndef _DReaction_factory_MilleKs_
#define _DReaction_factory_MilleKs_

#include <ANALYSIS/DCutActions.h>
#include <ANALYSIS/DHistogramActions.h>
#include <ANALYSIS/DReaction.h>
#include <JANA/JFactoryT.h>

#include <iomanip>
#include <iostream>

using namespace std;

class DReaction_factory_MilleKs : public JFactoryT<DReaction> {
 public:
  DReaction_factory_MilleKs() {
    // This is so that the created DReaction objects persist throughout the life
    // of the program instead of being cleared each event.
    SetFactoryFlag(PERSISTENT);
    SetTag("MilleKs");
  }

 private:
  void Process(const std::shared_ptr<const JEvent>& event) override;
  void Finish() override;

  deque<DReactionStep*> dReactionStepPool;  // to prevent memory leaks
};

#endif  // _DReaction_factory_MilleKs_
