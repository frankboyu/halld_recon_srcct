// $Id: ANALYSIS_init.cc 2433 2007-04-07 14:57:32Z pmatt $

#include <JANA/JEvent.h>

//OK
#include "DReaction_factory_Thrown.h"

#include "DAnalysisUtilities_factory.h"
#include "DMCThrownMatching_factory.h"

#include "DAnalysisResults_factory.h"
#include "DEventWriterROOT_factory.h"

#include "DTrackTimeBased_factory_Combo.h"
#include "DDetectorMatches_factory_Combo.h"
#include "DChargedTrack_factory_Combo.h"
#include "DNeutralParticle_factory_Combo.h"

#include "DReactionVertexInfo_factory.h"

#include "DHistogramActions.h"
#include "DCutActions.h"

void ANALYSIS_init(JFactorySet* factorySet)
{
	/// Create and register ANALYSIS data factories
	factorySet->Add(new DReaction_factory_Thrown);

	factorySet->Add(new DAnalysisUtilities_factory);
	factorySet->Add(new DMCThrownMatching_factory);

	factorySet->Add(new DAnalysisResults_factory);
	factorySet->Add(new DEventWriterROOT_factory);

	factorySet->Add(new DTrackTimeBased_factory_Combo);
	factorySet->Add(new DDetectorMatches_factory_Combo);
	factorySet->Add(new DChargedTrack_factory_Combo);
	factorySet->Add(new DNeutralParticle_factory_Combo);

	factorySet->Add(new DReactionVertexInfo_factory);

	//For some reason, have difficulty linking these classes without using them somewhere within the library
	DHistogramAction_ThrownParticleKinematics();
	DHistogramAction_DetectedParticleKinematics();
	DHistogramAction_ReconnedThrownKinematics();
	DHistogramAction_GenReconTrackComparison();
	DHistogramAction_TrackMultiplicity();
	DHistogramAction_NumReconstructedObjects();
	DHistogramAction_DetectorMatchParams();
	DHistogramAction_Neutrals();
	DHistogramAction_DetectorPID();
	DHistogramAction_DetectorMatching();
	DHistogramAction_Reconstruction();
	DHistogramAction_ObjectMemory();

	DHistogramAction_PID(NULL, false);
	DHistogramAction_TrackVertexComparison(NULL);
	DHistogramAction_ParticleComboKinematics(NULL, false);
	DHistogramAction_TruePID(NULL);
	DHistogramAction_InvariantMass(NULL, UnknownParticle, false, 0, 0.0, 0.0);
	DHistogramAction_MissingMass(NULL, false, 0, 0.0, 0.0);
	DHistogramAction_MissingMassSquared(NULL, false, 0, 0.0, 0.0);
	DHistogramAction_KinFitResults(NULL, 0.0);
	DHistogramAction_ParticleComboGenReconComparison(NULL, false);
	DHistogramAction_MissingTransverseMomentum(NULL, false, 0, 0.0, 0.0);
	DHistogramAction_2DInvariantMass(NULL, 0, deque<Particle_t>(), deque<Particle_t>(), false, 0, 0.0, 0.0, 0, 0.0, 0.0);
	DHistogramAction_Dalitz(NULL, 0, deque<Particle_t>(), deque<Particle_t>(), false, 0, 0.0, 0.0, 0, 0.0, 0.0);

	DCutAction_MinTrackHits(NULL, 0);
	DCutAction_ThrownTopology(NULL, true);
	DCutAction_PIDFOM(NULL, UnknownParticle, UnknownParticle, 0.0);
	DCutAction_AllTracksHaveDetectorMatch(NULL);
	DCutAction_CombinedPIDFOM(NULL, 0.0);
	DCutAction_EachPIDFOM(NULL, 0.0);
	DCutAction_CombinedTrackingFOM(NULL, 0.0);
	DCutAction_MissingMass(NULL, false, 0.0, 0.0);
	DCutAction_MissingMassSquared(NULL, false, 0.0, 0.0);
	DCutAction_InvariantMass(NULL, UnknownParticle, false, 0.0, 0.0);
	DCutAction_AllVertexZ(NULL, 0.0, 0.0);
	DCutAction_ProductionVertexZ(NULL, 0.0, 0.0);
	DCutAction_MaxTrackDOCA(NULL, UnknownParticle, 0.0);
	DCutAction_KinFitFOM(NULL, 0.0);
	DCutAction_TruePID(NULL, UnknownParticle, UnknownParticle, 0.0);
	DCutAction_AllTruePID(NULL, 0.0);
	DCutAction_GoodEventRFBunch(NULL, false);
	DCutAction_TransverseMomentum(NULL, 0.0);
	DCutAction_TrueBeamParticle(NULL);
	DCutAction_TrueCombo(NULL, 0.0, false);
	DCutAction_BDTSignalCombo(NULL, 0.0, false, false);

	DCutAction_TrackHitPattern(NULL);
	DCutAction_dEdx(NULL);
	DCutAction_BeamEnergy(NULL, false, 0.0, 0.0);
	DCutAction_TrackFCALShowerEOverP(NULL, false, 0.0);
	DCutAction_NoPIDHit(NULL);
	DCutAction_PIDDeltaT(NULL, false, 0.0);
	DCutAction_PIDTimingBeta(NULL, 0.0, 0.0);
	DCutAction_OneVertexKinFit(NULL);
}

