// $Id$
//
//    File: JEventProcessor_CDC_Efficiency.cc
// Created: Tue Sep  9 15:41:38 EDT 2014
// Creator: hdcdcops (on Linux gluon05.jlab.org 2.6.32-358.18.1.el6.x86_64 x86_64)
//

#include "JEventProcessor_CDC_Efficiency.h"
#include "HDGEOMETRY/DMagneticFieldMapNoField.h"
#include "CDC/DCDCDigiHit.h"
#include "DAQ/Df125CDCPulse.h"
#include "DANA/DEvent.h"
#include "TRIGGER/DTrigger.h"
#include "HistogramTools.h"


// Routine used to create our JEventProcessor
extern "C"{
void InitPlugin(JApplication *app){
    InitJANAPlugin(app);
    app->Add(new JEventProcessor_CDC_Efficiency());
}
} // "C"


//------------------
// JEventProcessor_CDC_Efficiency (Constructor)
//------------------
JEventProcessor_CDC_Efficiency::JEventProcessor_CDC_Efficiency()
{
	SetTypeName("JEventProcessor_CDC_Efficiency");
}

//------------------
// ~JEventProcessor_CDC_Efficiency (Destructor)
//------------------
JEventProcessor_CDC_Efficiency::~JEventProcessor_CDC_Efficiency()
{
    ;
}

//------------------
// Init
//------------------
void JEventProcessor_CDC_Efficiency::Init()
{

	auto app = GetApplication();
	lockService = app->GetService<JLockService>();

	dMinTrackingFOM = 5.73303E-7; // +/- 5 sigma
	dMinNumRingsToEvalSuperlayer = 3; //technically, reconstruction minimum is 2 (TRKFIND:MIN_SEED_HITS) //but trust more with 3

    // For the overall 2D plots, thus DOCA cut is used
    DOCACUT = 0.35;
    app->SetDefaultParameter("CDC_EFFICIENCY:DOCACUT", DOCACUT, "DOCA Cut on Efficiency Measurement");

    // Reject tracks with momentum less than PCUTL
    PCUTL = 0.5;
    if(app){
        app->SetDefaultParameter("CDC_EFFICIENCY:PCUTL", PCUTL, "Low momentum Cut on Efficiency Measurement");
    }  

    // Reject tracks with momentum greater than PCUTH
    PCUTH = 6.0;
    if(app){
        app->SetDefaultParameter("CDC_EFFICIENCY:PCUTH", PCUTH, "High momentum Cut on Efficiency Measurement");
    }  

    // Fill extra histos requiring dE/dx > 0 if set to 1
    FILL_DEDX_HISTOS = 0;
    if(app){
        app->SetDefaultParameter("CDC_EFFICIENCY:FILL_DEDX_HISTOS", FILL_DEDX_HISTOS, "Fill 'with dE/dx' histos if DOCA < CDC_GAIN_DOCA_PARS[0]");
    }  

    
    for (unsigned int i=0; i < 28; i++){
        for (unsigned int j=0; j < 209; j++){
            ChannelFromRingStraw[i][j] = -1;
            SlotFromRingStraw[i][j] = -1;
            ChannelFromRingStraw[i][j] = -1;
        }
    } 
    // Some information

    int Nstraws[28] = {42, 42, 54, 54, 66, 66, 80, 80, 93, 93, 106, 106, 123, 123, 135, 135, 146, 146, 158, 158, 170, 170, 182, 182, 197, 197, 209, 209};


    double radius[28] = {10.72134, 12.08024, 13.7795, 15.14602, 18.71726, 20.2438, 22.01672, 23.50008, 25.15616, 26.61158, 28.33624, 29.77388, 31.3817, 32.75838, 34.43478, 35.81146, 38.28542, 39.7002, 41.31564, 42.73042, 44.34078, 45.75302, 47.36084, 48.77054, 50.37582, 51.76012, 53.36286, 54.74716};
    double phi[28] = {0, 0.074707844, 0.038166294, 0.096247609, 0.05966371, 0.012001551, 0.040721951, 0.001334527, 0.014963808, 0.048683644, 0.002092645, 0.031681749, 0.040719354, 0.015197341, 0.006786058, 0.030005892, 0.019704045, -0.001782064, -0.001306618, 0.018592421, 0.003686784, 0.022132975, 0.019600866, 0.002343723, 0.021301449, 0.005348855, 0.005997358, 0.021018761};

    // Define a different 2D histogram for each ring. X-axis is phi, Y-axis is radius (to plot correctly with "pol" option)

    // create root folder for cdc and cd to it, store main dir
    TDirectory *main = gDirectory;
    gDirectory->mkdir("CDC_Efficiency")->cd();
    gDirectory->mkdir("CDC_View")->cd();

    cdc_measured_ring.resize(29);
    cdc_expected_ring.resize(29);

    cdc_measured_with_dedx_ring.resize(29);


    for(int locDOCABin = 0; locDOCABin < 8; ++locDOCABin)
    {
    	cdc_measured_ringmap[locDOCABin].resize(29);
    	cdc_expected_ringmap[locDOCABin].resize(29);

    	cdc_measured_with_dedx_ringmap[locDOCABin].resize(29);
    }

    for(int iring=0; iring<28; iring++){
        double r_start = radius[iring] - 0.8;
        double r_end = radius[iring] + 0.8;
        double phi_start = phi[iring]; // this is for center of straw. Need additional calculation for phi at end plate
        double phi_end = phi_start + TMath::TwoPi();

        char hname_measured[256];
        char hname_expected[256];
        sprintf(hname_measured, "cdc_measured_ring[%d]", iring+1);
        sprintf(hname_expected, "cdc_expected_ring[%d]", iring+1);

	cdc_measured_ring[iring+1] = new TH2D(hname_measured, "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);

	cdc_measured_ringmap[0][iring+1] = new TH2D((TString)hname_measured +"DOCA0", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[1][iring+1] = new TH2D((TString)hname_measured +"DOCA1", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[2][iring+1] = new TH2D((TString)hname_measured +"DOCA2", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[3][iring+1] = new TH2D((TString)hname_measured +"DOCA3", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[4][iring+1] = new TH2D((TString)hname_measured +"DOCA4", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[5][iring+1] = new TH2D((TString)hname_measured +"DOCA5", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[6][iring+1] = new TH2D((TString)hname_measured +"DOCA6", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_measured_ringmap[7][iring+1] = new TH2D((TString)hname_measured +"DOCA7", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);

	cdc_expected_ring[iring+1] = new TH2D(hname_expected, "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[0][iring+1] = new TH2D((TString)hname_expected + "DOCA0", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[1][iring+1] = new TH2D((TString)hname_expected + "DOCA1", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[2][iring+1] = new TH2D((TString)hname_expected + "DOCA2", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[3][iring+1] = new TH2D((TString)hname_expected + "DOCA3", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[4][iring+1] = new TH2D((TString)hname_expected + "DOCA4", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[5][iring+1] = new TH2D((TString)hname_expected + "DOCA5", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[6][iring+1] = new TH2D((TString)hname_expected + "DOCA6", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	cdc_expected_ringmap[7][iring+1] = new TH2D((TString)hname_expected + "DOCA7", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);

	if (FILL_DEDX_HISTOS) {

	  sprintf(hname_measured, "cdc_measured_with_dedx_ring[%d]", iring+1);
	  sprintf(hname_expected, "cdc_expected_with_dedx_ring[%d]", iring+1);

	  cdc_measured_with_dedx_ring[iring+1] = new TH2D(hname_measured, "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);

	  cdc_measured_with_dedx_ringmap[0][iring+1] = new TH2D((TString)hname_measured +"DOCA0", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[1][iring+1] = new TH2D((TString)hname_measured +"DOCA1", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[2][iring+1] = new TH2D((TString)hname_measured +"DOCA2", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[3][iring+1] = new TH2D((TString)hname_measured +"DOCA3", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[4][iring+1] = new TH2D((TString)hname_measured +"DOCA4", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[5][iring+1] = new TH2D((TString)hname_measured +"DOCA5", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[6][iring+1] = new TH2D((TString)hname_measured +"DOCA6", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	  cdc_measured_with_dedx_ringmap[7][iring+1] = new TH2D((TString)hname_measured +"DOCA7", "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
	}

    }	

    gDirectory->cd("/CDC_Efficiency");
    gDirectory->mkdir("Track_Quality")->cd();
    
    hChi2OverNDF = new TH1I("hChi2OverNDF","hChi2OverNDF", 500, 0.0, 1.0);
    ChargeVsTrackLength = new TH2I("ChargeVsTrackLength", "ChargeVsTrackLength", 1000, 0, 8.0, 2000, 0, 5000000);
    hResVsT = new TH2I("hResVsT","Tracking Residual (Biased) Vs Drift Time; Drift Time [ns]; Residual [cm]", 500, 0.0, 700.0, 1000, -0.5, 0.5);

    gDirectory->cd("/CDC_Efficiency");
    gDirectory->mkdir("Offline")->cd();
    
	hExpectedHitsVsPathLength = new TH1F("Expected Hits Vs Path Length", "Expected Hits", 100, 0 , 4.0);
	hExpectedHitsVsDOCA = new TH1F("Expected Hits Vs DOCA", "Expected Hits", 100, 0 , 0.78);
	hExpectedHitsVsTrackingFOM = new TH1F("Expected Hits Vs Tracking FOM",  "Expected Hits", 100, 0 , 1.0);
	hExpectedHitsVsTheta = new TH1F("Expected Hits Vs theta", "Expected Hits", 100, 0, 180);
	hExpectedHitsVsMom = new TH1F("Expected Hits Vs p", "Expected Hits", 100, 0 , 4.0);
	hExpectedHitsVsDelta = new TH1F("Expected Hits Vs delta", "Expected Hits", 100, -0.3 , 0.3);
	hExpectedHitsMomVsTheta = new TH2F("Expected hits p Vs Theta",  "Expected Hits", 100, 0, 180, 100, 0 , 4.0);
	hExpectedHitsVsN = new TH1F("Expected Hits Vs N", "Expected Hits", 3522, 0.5, 3522.5);
	
	hMeasuredHitsVsPathLength = new TH1F("Measured Hits Vs Path Length", "Measured Hits", 100, 0 , 4.0);
	hMeasuredHitsVsDOCA = new TH1F("Measured Hits Vs DOCA", "Measured Hits", 100, 0 , 0.78);
	hMeasuredHitsVsTrackingFOM = new TH1F("Measured Hits Vs Tracking FOM",  "Measured Hits", 100, 0 , 1.0);
	hMeasuredHitsVsTheta = new TH1F("Measured Hits Vs theta", "Measured Hits", 100, 0, 180);
	hMeasuredHitsVsMom = new TH1F("Measured Hits Vs p", "Measured Hits", 100, 0 , 4.0);
	hMeasuredHitsVsDelta = new TH1F("Measured Hits Vs delta", "Measured Hits", 100, -0.3 , 0.3);
	hMeasuredHitsMomVsTheta = new TH2F("Measured hits p Vs Theta",  "Measured Hits", 100, 0, 180, 100, 0 , 4.0);
	hMeasuredHitsVsN = new TH1F("Measured Hits Vs N", "Measured Hits", 3522, 0.5, 3522.5);

	if (FILL_DEDX_HISTOS) {
	  hMeasuredHitsWithDEDxVsPathLength = new TH1F("Measured Hits with dE/dx info Vs Path Length", "Measured Hits with dE/dx info", 100, 0 , 4.0);
	  hMeasuredHitsWithDEDxVsDOCA = new TH1F("Measured Hits with dE/dx info Vs DOCA", "Measured Hits with dE/dx info", 100, 0 , 0.78);
	  hMeasuredHitsWithDEDxVsTrackingFOM = new TH1F("Measured Hits with dE/dx info Vs Tracking FOM",  "Measured Hits with dE/dx info", 100, 0 , 1.0);
	  hMeasuredHitsWithDEDxVsTheta = new TH1F("Measured Hits with dE/dx info Vs theta", "Measured Hits with dE/dx info", 100, 0, 180);
	  hMeasuredHitsWithDEDxVsMom = new TH1F("Measured Hits with dE/dx info Vs p", "Measured Hits with dE/dx info", 100, 0 , 4.0);
	  hMeasuredHitsWithDEDxVsDelta = new TH1F("Measured Hits with dE/dx info Vs delta", "Measured Hits with dE/dx info", 100, -0.3 , 0.3);
	  hMeasuredHitsWithDEDxMomVsTheta = new TH2F("Measured Hits with dE/dx info p Vs Theta",  "Measured Hits with dE/dx info", 100, 0, 180, 100, 0 , 4.0);
	  hMeasuredHitsWithDEDxVsN = new TH1F("Measured Hits with dE/dx info Vs N", "Measured Hits with dE/dx info", 3522, 0.5, 3522.5);
	}
	
    gDirectory->cd("/CDC_Efficiency");
    gDirectory->mkdir("Online")->cd();
    
	hEfficiencyVsPathLength = new TProfile("Efficiency Vs Path Length", "Efficiency", 100, 0 , 4.0);
	hEfficiencyVsDOCA = new TProfile("Efficiency Vs DOCA", "Efficiency", 100, 0 , 0.78);
	hEfficiencyVsTrackingFOM = new TProfile("Efficiency Vs Tracking FOM",  "Efficiency", 100, 0 , 1.0);
	hEfficiencyVsTheta = new TProfile("Efficiency Vs theta", "Efficiency", 100, 0, 180);
	hEfficiencyVsMom = new TProfile("Efficiency Vs p", "Efficiency", 100, 0 , 4.0);
	hEfficiencyVsDelta = new TProfile("Efficiency Vs delta", "Efficiency", 100, -0.3 , 0.3);
	hEfficiencyVsN = new TProfile("Efficiency Vs N", "Efficiency", 3522, 0.5, 3522.5);
	
	if (FILL_DEDX_HISTOS) {
	  hEfficiencyWithDEDxVsPathLength = new TProfile("Efficiency (with dE/dx)  Vs Path Length", "Efficiency (with dE/dx) ", 100, 0 , 4.0);
	  hEfficiencyWithDEDxVsDOCA = new TProfile("Efficiency (with dE/dx)  Vs DOCA", "Efficiency (with dE/dx) ", 100, 0 , 0.78);
	  hEfficiencyWithDEDxVsTrackingFOM = new TProfile("Efficiency (with dE/dx)  Vs Tracking FOM",  "Efficiency (with dE/dx) ", 100, 0 , 1.0);
	  hEfficiencyWithDEDxVsTheta = new TProfile("Efficiency (with dE/dx)  Vs theta", "Efficiency (with dE/dx) ", 100, 0, 180);
	  hEfficiencyWithDEDxVsMom = new TProfile("Efficiency (with dE/dx)  Vs p", "Efficiency (with dE/dx) ", 100, 0 , 4.0);
	  hEfficiencyWithDEDxVsDelta = new TProfile("Efficiency (with dE/dx)  Vs delta", "Efficiency (with dE/dx) ", 100, -0.3 , 0.3);
	  hEfficiencyWithDEDxVsN = new TProfile("Efficiency (with dE/dx)  Vs N", "Efficiency (with dE/dx) ", 3522, 0.5, 3522.5);
	}
	
	int ROCnums[] = {25, 26, 27, 28};
	hEfficiencyVsChannel = new TProfile("Efficiency Vs Channel Number", "Efficiency; Channel Number; Efficiency", 73, -0.5 , 72.5);
	for(int i=0; i<4; i++) {
        char name [200];
        sprintf(name, "Slot Efficiency ROCID %.2i", ROCnums[i]);
		hEfficiencyVsSlotROC[ROCnums[i]] = new TProfile(name, "Efficiency; Slot Number; Efficiency", 21, -0.5 , 20.5);
        sprintf(name, "Channel Efficiency ROCID %.2i", ROCnums[i]);
		hEfficiencyVsChannelROC[ROCnums[i]] = new TProfile(name, "Efficiency; Channel; Efficiency", 1501, 299.5 , 1800.5);
	}
	
	if (FILL_DEDX_HISTOS) {
	  hEfficiencyWithDEDxVsChannel = new TProfile("Efficiency (with dE/dx) Vs Channel Number", "Efficiency; Channel Number; Efficiency", 73, -0.5 , 72.5);
	  for(int i=0; i<4; i++) {
	    char name [200];
	    sprintf(name, "Slot Efficiency (with dE/dx) ROCID %.2i", ROCnums[i]);
	    hEfficiencyWithDEDxVsSlotROC[ROCnums[i]] = new TProfile(name, "Efficiency; Slot Number; Efficiency", 21, -0.5 , 20.5);
	    sprintf(name, "Channel Efficiency (with dE/dx) ROCID %.2i", ROCnums[i]);
	    hEfficiencyWithDEDxVsChannelROC[ROCnums[i]] = new TProfile(name, "Efficiency; Channel; Efficiency", 1501, 299.5 , 1800.5);
	  }
	}
	
	hEfficiencyMomVsTheta = new TProfile2D("Efficiency p Vs Theta", "Efficiency; Track #Theta [deg]; Momentum [GeV]", 100, 0, 180, 100, 0 , 4.0);
	hEfficiencyDistanceVsDelta = new TProfile2D("Efficiency distance Vs delta", "Efficiency;#delta [cm]; DOCA [cm]", 100, -0.3, 0.3, 100, 0 , 1.2);
	hEfficiencyZVsDelta = new TProfile2D("Efficiency z Vs delta", "Efficiency;#delta [cm]; z [cm] (CDC local coordinates)", 100, -0.3, 0.3, 150, -75 , 75);

	if (FILL_DEDX_HISTOS) {
	  hEfficiencyWithDEDxMomVsTheta = new TProfile2D("Efficiency (with dE/dx) p Vs Theta", "Efficiency (with dE/dx); Track #Theta [deg]; Momentum [GeV]", 100, 0, 180, 100, 0 , 4.0);
	  hEfficiencyWithDEDxDistanceVsDelta = new TProfile2D("Efficiency (with dE/dx) distance Vs delta", "Efficiency (with dE/dx);#delta [cm]; DOCA [cm]", 100, -0.3, 0.3, 100, 0 , 1.2);
	  hEfficiencyWithDEDxZVsDelta = new TProfile2D("Efficiency (with dE/dx) z Vs delta", "Efficiency (with dE/dx);#delta [cm]; z [cm] (CDC local coordinates)", 100, -0.3, 0.3, 150, -75 , 75);
	}

	main->cd();

	dTargetCenterZ = 65.0;
	dTargetLength = 30.0;

    return;
}

//------------------
// BeginRun
//------------------
void JEventProcessor_CDC_Efficiency::BeginRun(const std::shared_ptr<const JEvent>& event)
{

    // This is called whenever the run number changes
    dIsNoFieldFlag = (dynamic_cast<const DMagneticFieldMapNoField*>(GetBfield(event)) != NULL);
    JCalibration *jcalib = GetJCalibration(event);
    dgeom  = GetDGeometry(event);
    //bfield = dapp->GetBfield();

	//Get Target Center Z, length
	dgeom->GetTargetZ(dTargetCenterZ);
	dgeom->GetTargetLength(dTargetLength);

    // Get the position of the CDC downstream endplate from DGeometry
    //double endplate_z,endplate_dz,endplate_rmin,endplate_rmax;
    //dgeom->GetCDCEndplate(endplate_z,endplate_dz,endplate_rmin,endplate_rmax);
    dgeom->GetCDCWires(cdcwires);
    unsigned int numstraws[28]={42,42,54,54,66,66,80,80,93,93,106,106,123,123,
        135,135,146,146,158,158,170,170,182,182,197,197,
        209,209};

    // Get the straw sag parameters from the database
    vector< map<string, double> > tvals;
    max_sag.clear();
    sag_phi_offset.clear();
    unsigned int straw_count=0,ring_count=0;
    if (jcalib->Get("CDC/sag_parameters", tvals)==false){
        vector<double>temp,temp2;
        for(unsigned int i=0; i<tvals.size(); i++){
            map<string, double> &row = tvals[i];

            temp.push_back(row["offset"]);
            temp2.push_back(row["phi"]);

            straw_count++;
            if (straw_count==numstraws[ring_count]){
                max_sag.push_back(temp);
                sag_phi_offset.push_back(temp2);
                temp.clear();
                temp2.clear();
                straw_count=0;
                ring_count++;
            }
        }
    }

    // CDC correction for gain drop from progressive gas deterioration in spring 2018

    if (jcalib->Get("CDC/gain_doca_correction", CDC_GAIN_DOCA_PARS)) cout << "Error loading CDC/gain_doca_correction !" << endl;


	MAX_DRIFT_TIME = 1000.0; //ns: from TRKFIND:MAX_DRIFT_TIME in DTrackCandidate_factory_CDC
	//Make sure it gets initialize first, in case we want to change it:
   if(!dIsNoFieldFlag){
      vector<const DTrackCandidate*> locTrackCandidates;
      event->Get(locTrackCandidates);
      GetApplication()->GetParameter("TRKFIND:MAX_DRIFT_TIME", MAX_DRIFT_TIME);
   }

}

//------------------
// Process
//------------------
void JEventProcessor_CDC_Efficiency::Process(const std::shared_ptr<const JEvent>& event){
	const DTrigger* locTrigger = NULL; 
	event->GetSingle(locTrigger); 
	if(locTrigger->Get_L1FrontPanelTriggerBits() != 0)
	  return;
	if (!locTrigger->Get_IsPhysicsEvent()){ // do not look at PS triggers
	  return;
	}

	
   	vector<const DTrackFitter *> fitters;
	event->Get(fitters);

	if(fitters.size()<1){
	  _DBG_<<"Unable to get a DTrackFinder object!"<<endl;
	  throw JException("Missing DTrackFinder");
	}

	const DTrackFitter *fitter = fitters[0];
	
	
	// Get the particle ID algorithms
	vector<const DParticleID *> pid_algorithms;
	event->Get(pid_algorithms);
	if(pid_algorithms.size()<1){
	  _DBG_<<"Unable to get a DParticleID object! NO PID will be done!"<<endl;
	  return;
	}

	const DParticleID* pid_algorithm = pid_algorithms[0];

   //use CDC track hits: have drift time, can cut
   vector< const DCDCTrackHit *> locCDCTrackHits;
   event->Get(locCDCTrackHits);

   //Pre-sort hits by ring to save time //only need to search within the given ring, straw
   map<int, map<int, set<const DCDCTrackHit*> > > locSortedCDCTrackHits; //first int: ring //second int: straw
   for(auto& locTrackHit : locCDCTrackHits)
   {
      if(locTrackHit->tdrift <= MAX_DRIFT_TIME)
         locSortedCDCTrackHits[locTrackHit->wire->ring][locTrackHit->wire->straw].insert(locTrackHit);
   }

   const DDetectorMatches *detMatches = nullptr;
   if(!dIsNoFieldFlag)
      event->GetSingle(detMatches);

   const DParticleID *locParticleID = nullptr;
   event->GetSingle(locParticleID);

   vector <const DChargedTrack *> chargedTrackVector;
   event->Get(chargedTrackVector);

   vector <const DTrackTimeBased *> bestTimeBasedTracks;
   if(!dIsNoFieldFlag){
      for (unsigned int iTrack = 0; iTrack < chargedTrackVector.size(); iTrack++){
         const DChargedTrackHypothesis* bestHypothesis = chargedTrackVector[iTrack]->Get_BestTrackingFOM();
         bestTimeBasedTracks.push_back(bestHypothesis->Get_TrackTimeBased());
      }
   }
   else{
      event->Get(bestTimeBasedTracks);
   }

   for (unsigned int iTrack = 0; iTrack < bestTimeBasedTracks.size(); iTrack++){
      auto thisTimeBasedTrack = bestTimeBasedTracks[iTrack];

      lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
      hChi2OverNDF->Fill(thisTimeBasedTrack->FOM);
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

      if (thisTimeBasedTrack->FOM < dMinTrackingFOM)
         continue;

      //The cuts used for track quality
      if(!dIsNoFieldFlag){ // Quality cuts for Field on runs.
         if(thisTimeBasedTrack->ddEdx_CDC_amp > 1E-3) {
            //cout << "Cut on dEdX" << endl;
            continue; // Trying to cut out "proton" candidates
         }
         if(thisTimeBasedTrack->pmag() < PCUTL || thisTimeBasedTrack->pmag() > PCUTH) {
            //cout << "Cut on momentum" << endl;
            continue; // Cut on the reconstructed momentum to make sure we have the right
         }
         if(!detMatches->Get_IsMatchedToDetector(thisTimeBasedTrack, SYS_TOF) && !detMatches->Get_IsMatchedToDetector(thisTimeBasedTrack, SYS_BCAL))
         {
            //cout << "Cut on detector matches" << endl;
            continue; // Require there to be at least one match to BCAL or TOF //not SC: lights up like xmas tree
         }
         if(fabs(thisTimeBasedTrack->position().Z() - dTargetCenterZ) > dTargetLength/2.0 || thisTimeBasedTrack->position().Perp() > 1.0) {
            //cout << " Cut on vertex " << endl;
            continue; // Cut on reconstructed vertex location
         }
      }
      // Require many hits on the track for cosmics
      else{
         if(thisTimeBasedTrack->Ndof < 11) continue;
      }

      // Require hits on at least 2 axial layers and at least 2 stereo layers:
      // necessary to trust reconstructed phi & theta: respectable projection
      set<int> locCDCRings;
      locParticleID->Get_CDCRings(thisTimeBasedTrack->dCDCRings, locCDCRings);

      map<int, int> locNumHitRingsPerSuperlayer; //key: superlayer (1 -> 7) //axial: 1, 4, 7
      locParticleID->Get_CDCNumHitRingsPerSuperlayer(locCDCRings, locNumHitRingsPerSuperlayer);

      int locNumSuperLayersWith2Hits_Axial = 0;
      int locNumSuperLayersWith2Hits_Stereo = 0;
      for(auto& locSuperlayerPair : locNumHitRingsPerSuperlayer)
      {
         if(locSuperlayerPair.second < 2)
            continue;
         if((locSuperlayerPair.first == 1) || (locSuperlayerPair.first == 4) || (locSuperlayerPair.first == 7))
            ++locNumSuperLayersWith2Hits_Axial;
         else
            ++locNumSuperLayersWith2Hits_Stereo;
      }

      if((locNumSuperLayersWith2Hits_Axial < 2) || (locNumSuperLayersWith2Hits_Stereo < 2))
         continue; //don't trust the track projections

      // Alright now we truly have the tracks we are interested in for calculating the efficiency
      //BUT, we need to make sure that we aren't biased by the fact that the track was reconstructed in the first place
      //AND by our requirement above that there be at least 2 hits in a few superlayers
      for(int locCDCSuperlayer = 1; locCDCSuperlayer <= 7; ++locCDCSuperlayer)
      {
         if(locNumHitRingsPerSuperlayer[locCDCSuperlayer] < dMinNumRingsToEvalSuperlayer)
            continue;

         int locFirstRing = 4*(locCDCSuperlayer - 1) + 1;
         if(locNumHitRingsPerSuperlayer[locCDCSuperlayer] == dMinNumRingsToEvalSuperlayer)
         {
            //All hits required: Can only evaluate the rings that do NOT have hits
            for (int locRing = locFirstRing; locRing < locFirstRing + 4; ++locRing)
            {
               if(locCDCRings.find(locRing) == locCDCRings.end())

		 //june12

                  Fill_Efficiency_Histos(locRing, thisTimeBasedTrack, locSortedCDCTrackHits, pid_algorithm, fitter); 

            }
            continue;
         }
         //so many hits that no individual ring was required: evaluate for all
         for (int locRing = locFirstRing; locRing < locFirstRing + 4; ++locRing)

	   //june12
            Fill_Efficiency_Histos(locRing, thisTimeBasedTrack, locSortedCDCTrackHits, pid_algorithm, fitter); 
      }
   }
   return;
}


void JEventProcessor_CDC_Efficiency::Fill_Efficiency_Histos(unsigned int ringNum, const DTrackTimeBased *thisTimeBasedTrack, map<int, map<int, set<const DCDCTrackHit*> > >& locSortedCDCTrackHits, const DParticleID * pid_algorithm, const DTrackFitter *fitter)
{
  vector<DTrackFitter::Extrapolation_t>extrapolations=thisTimeBasedTrack->extrapolations.at(SYS_CDC);
  if (extrapolations.size()==0) return;
  
   int Nstraws_previous[28] = {0,42,84,138,192,258,324,404,484,577,670,776,882,1005,1128,1263,1398,1544,1690,1848,2006,2176,2346,2528,2710,2907,3104,3313};
   vector< DCDCWire * > wireByNumber = cdcwires[ringNum - 1];
   DVector3 pos;
   DVector3 mom=thisTimeBasedTrack->momentum();
   for (unsigned int wireIndex = 0; wireIndex < wireByNumber.size(); wireIndex++)
   {
      int wireNum = wireIndex+1;
      DCDCWire * wire = wireByNumber[wireIndex];
      double distanceToWire;
      if(!dIsNoFieldFlag){
	distanceToWire = fitter->DistToWire(wire,extrapolations,&pos,&mom);
      }
      else {
         DVector3 POCAOnTrack, POCAOnWire;
         distanceToWire = GetDOCAFieldOff(wire->origin, wire->udir,
					  thisTimeBasedTrack->position(),mom,
					  POCAOnTrack, POCAOnWire);
	 pos=POCAOnTrack;
      }

      //SKIP IF NOT CLOSE - Field on
      if(!dIsNoFieldFlag){
         if(distanceToWire > 50.0)
         {
            wireIndex += 30;
            continue;
         }
         if(distanceToWire > 20.0)
         {
            wireIndex += 10;
            continue;
         }
         if(distanceToWire > 10.0)
         {
            wireIndex += 5;
            continue;
         }
      }

      double delta = 0.0, dz = 0.0;
      if(!Expect_Hit(thisTimeBasedTrack, wire, distanceToWire, pos, delta, dz))
         continue;

      //FILL EXPECTED HISTOGRAMS
      double dx = pid_algorithm->CalcdXHit(mom,pos,wire);
      double locTheta = thisTimeBasedTrack->momentum().Theta()*TMath::RadToDeg();
      
      lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
      hExpectedHitsVsPathLength->Fill(dx);
      hExpectedHitsVsDOCA->Fill(distanceToWire);
      hExpectedHitsVsTrackingFOM->Fill(thisTimeBasedTrack->FOM);
      hExpectedHitsVsTheta->Fill(locTheta);
      hExpectedHitsVsMom->Fill(thisTimeBasedTrack->pmag());
      hExpectedHitsVsDelta->Fill(delta);
      hExpectedHitsMomVsTheta->Fill(locTheta, thisTimeBasedTrack->pmag());
      hExpectedHitsVsN->Fill(Nstraws_previous[ringNum-1]+wireNum);            //expected hits by straw number
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
   
      
      // look for a CDC hit match
      // We need a backwards map from ring/straw to flash channel. Unfortunately there is no easy way
      // Will construct the map manually
      const DCDCTrackHit* locTrackHit = Find_Hit(ringNum, wireNum, locSortedCDCTrackHits[ringNum]);
      const DCDCHit* locHit = nullptr;
      if(locTrackHit != nullptr)
         locTrackHit->GetSingle(locHit);

      bool foundHit = (locTrackHit != nullptr);

      bool foundHitWithdEdx = foundHit;
      // hits with doca outside param 0 are ignored in dE/dx calc
      if (distanceToWire > CDC_GAIN_DOCA_PARS[0]) foundHitWithdEdx = 0;

      if(foundHit)
      {

         const DCDCDigiHit *thisDigiHit = NULL;
         const Df125CDCPulse *thisPulse = NULL;
         locHit->GetSingle(thisDigiHit);
         if (thisDigiHit != NULL)
            thisDigiHit->GetSingle(thisPulse);
         if (thisPulse != NULL)
         {
            ROCIDFromRingStraw[ringNum - 1][wireNum - 1] = thisPulse->rocid;
            SlotFromRingStraw[ringNum - 1][wireNum - 1] = thisPulse->slot;
            ChannelFromRingStraw[ringNum - 1][wireNum - 1] = thisPulse->channel;
         }

      	  lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK

		  hMeasuredHitsVsPathLength->Fill(dx);
		  hMeasuredHitsVsDOCA->Fill(distanceToWire);
		  hMeasuredHitsVsTrackingFOM->Fill(thisTimeBasedTrack->FOM);
		  hMeasuredHitsVsTheta->Fill(locTheta);
		  hMeasuredHitsVsMom->Fill(thisTimeBasedTrack->pmag());
		  hMeasuredHitsVsDelta->Fill(delta);
		  hMeasuredHitsMomVsTheta->Fill(locTheta, thisTimeBasedTrack->pmag());
		  hMeasuredHitsVsN->Fill(Nstraws_previous[ringNum-1]+wireNum);            //expected hits by straw number

	 if (FILL_DEDX_HISTOS) {
	   if (foundHitWithdEdx) { // fill histos for which hit contributes to dE/dx
		  hMeasuredHitsWithDEDxVsPathLength->Fill(dx);
		  hMeasuredHitsWithDEDxVsDOCA->Fill(distanceToWire);
		  hMeasuredHitsWithDEDxVsTrackingFOM->Fill(thisTimeBasedTrack->FOM);
		  hMeasuredHitsWithDEDxVsTheta->Fill(locTheta);
		  hMeasuredHitsWithDEDxVsMom->Fill(thisTimeBasedTrack->pmag());
		  hMeasuredHitsWithDEDxVsDelta->Fill(delta);
		  hMeasuredHitsWithDEDxMomVsTheta->Fill(locTheta, thisTimeBasedTrack->pmag());
		  hMeasuredHitsWithDEDxVsN->Fill(Nstraws_previous[ringNum-1]+wireNum);            //expected hits by straw number
	   }
	 }
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

      }

      lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK

      //FILL PROFILES: BASED ON FOUND OR NOT
	  hEfficiencyVsPathLength->Fill(dx,foundHit);
	  hEfficiencyVsDOCA->Fill(distanceToWire,foundHit);
	  hEfficiencyVsTrackingFOM->Fill(thisTimeBasedTrack->FOM,foundHit);
	  hEfficiencyVsTheta->Fill(locTheta,foundHit);
	  hEfficiencyVsMom->Fill(thisTimeBasedTrack->pmag(),foundHit);
	  hEfficiencyVsDelta->Fill(delta,foundHit);
	  hEfficiencyVsN->Fill(Nstraws_previous[ringNum-1]+wireNum,foundHit);            //expected hits by straw number


      // repeat for hits contributing to dE/dx

      if (FILL_DEDX_HISTOS) {
		  hEfficiencyWithDEDxVsPathLength->Fill(dx,foundHitWithdEdx);
		  hEfficiencyWithDEDxVsDOCA->Fill(distanceToWire,foundHitWithdEdx);
		  hEfficiencyWithDEDxVsTrackingFOM->Fill(thisTimeBasedTrack->FOM,foundHitWithdEdx);
		  hEfficiencyWithDEDxVsTheta->Fill(locTheta,foundHitWithdEdx);
		  hEfficiencyWithDEDxVsMom->Fill(thisTimeBasedTrack->pmag(),foundHitWithdEdx);
		  hEfficiencyWithDEDxVsDelta->Fill(delta,foundHitWithdEdx);
		  hEfficiencyWithDEDxVsN->Fill(Nstraws_previous[ringNum-1]+wireNum,foundHitWithdEdx);            //expected hits by straw number
      }


      if( ChannelFromRingStraw[ringNum - 1][wireNum - 1] != -1)
      {
         hEfficiencyVsChannel->Fill(ChannelFromRingStraw[ringNum - 1][wireNum - 1],foundHit);
         hEfficiencyVsSlotROC[ ROCIDFromRingStraw[ringNum - 1][wireNum - 1] ]->Fill(SlotFromRingStraw[ringNum - 1][wireNum - 1],foundHit);
         hEfficiencyVsChannelROC[ ROCIDFromRingStraw[ringNum - 1][wireNum - 1] ]->Fill(SlotFromRingStraw[ringNum - 1][wireNum - 1] * 100 + ChannelFromRingStraw[ringNum - 1][wireNum - 1],foundHit);

	 if (FILL_DEDX_HISTOS) {
	   // repeat w dedx info 
         hEfficiencyWithDEDxVsChannel->Fill(ChannelFromRingStraw[ringNum - 1][wireNum - 1],foundHitWithdEdx);
         hEfficiencyWithDEDxVsSlotROC[ ROCIDFromRingStraw[ringNum - 1][wireNum - 1] ]->Fill(SlotFromRingStraw[ringNum - 1][wireNum - 1],foundHitWithdEdx);
         hEfficiencyWithDEDxVsSlotROC[ ROCIDFromRingStraw[ringNum - 1][wireNum - 1] ]->Fill(SlotFromRingStraw[ringNum - 1][wireNum - 1] * 100 + ChannelFromRingStraw[ringNum - 1][wireNum - 1],foundHitWithdEdx);
	 }
      }

		hEfficiencyMomVsTheta->Fill(locTheta, thisTimeBasedTrack->pmag(),foundHit);
		hEfficiencyDistanceVsDelta->Fill(delta,distanceToWire,foundHit);
		hEfficiencyZVsDelta->Fill(delta,dz,foundHit);

      if (FILL_DEDX_HISTOS) {
         // repeat w dedx info 
		hEfficiencyWithDEDxMomVsTheta->Fill(locTheta, thisTimeBasedTrack->pmag(),foundHitWithdEdx);
		hEfficiencyWithDEDxDistanceVsDelta->Fill(delta,distanceToWire,foundHitWithdEdx);
		hEfficiencyWithDEDxZVsDelta->Fill(delta,dz,foundHitWithdEdx);
      }
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK


      //FILL AS FUNCTION OF DOCA
      if (distanceToWire < 0.78)
      {
         Fill_ExpectedHit(ringNum, wireNum, distanceToWire);
         if(foundHit)
	   //june12
	   Fill_MeasuredHit(foundHitWithdEdx, ringNum, wireNum, distanceToWire, pos, mom, wire, 
			    locHit, pid_algorithm);
      }
   }
}

bool JEventProcessor_CDC_Efficiency::Expect_Hit(const DTrackTimeBased* thisTimeBasedTrack, DCDCWire* wire, double distanceToWire, const DVector3 &pos,double& delta, double& dz)
{
   delta = 0.0;
   dz = 0.0;
   if (distanceToWire >= 1.2 )
      return false;
   
   // Form the vector between the wire and the DOCA point
   DVector3 DOCA;
  
   if(!dIsNoFieldFlag) {
      DOCA = (-1) * ((wire->origin - pos) - (wire->origin - pos).Dot(wire->udir) * wire->udir);
      dz = (pos - wire->origin).Z();
   }
   else {
      DVector3 POCAOnTrack, POCAOnWire;
      GetDOCAFieldOff(wire->origin, wire->udir, thisTimeBasedTrack->position(), thisTimeBasedTrack->momentum(), POCAOnTrack, POCAOnWire);
      DOCA = POCAOnTrack - POCAOnWire;
      dz = (POCAOnWire - wire->origin).Z();
   }

   double docaphi = DOCA.Phi();
   //cout << "distanceToWire = " << distanceToWire << " DOCA = " << DOCA.Mag() << endl;
   // Get delta at this location for this straw
   int ring_index = wire->ring - 1;
   int straw_index = wire->straw - 1;
   delta = max_sag[ring_index][straw_index] * ( 1. - (dz*dz/5625.)) * TMath::Cos(docaphi + sag_phi_offset[ring_index][straw_index]);

   return (distanceToWire < (0.78 + delta) && fabs(dz) < 65.0);
}

//june12

void JEventProcessor_CDC_Efficiency::Fill_MeasuredHit(bool withdEdx, int ringNum, int wireNum, double distanceToWire, const DVector3 &pos, const DVector3 &mom, DCDCWire* wire, const DCDCHit* locHit, const DParticleID * pid_algorithm)
{
   //Double_t w = cdc_occ_ring[ring]->GetBinContent(straw, 1) + 1.0;
   //cdc_occ_ring[ring]->SetBinContent(straw, 1, w);
   //Fill the expected number of hits histogram
   if (distanceToWire < DOCACUT)
   {
      //printf("Matching Hit!!!!!\n");
      lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
      {
         Double_t v = cdc_measured_ring[ringNum]->GetBinContent(wireNum, 1) + 1.0;
         cdc_measured_ring[ringNum]->SetBinContent(wireNum, 1, v);
         double dx = pid_algorithm->CalcdXHit(mom,pos,wire);
         ChargeVsTrackLength->Fill(dx, locHit->q);

	if (FILL_DEDX_HISTOS) {
	  if (withdEdx) {
	    Double_t vw = cdc_measured_with_dedx_ring[ringNum]->GetBinContent(wireNum, 1) + 1.0;
	    cdc_measured_with_dedx_ring[ringNum]->SetBinContent(wireNum, 1, vw);
	  }
	}

         // ?	Double_t w = cdc_expected_ring[ringNum]->GetBinContent(wireNum, 1) + 1.0;
         // ?    cdc_measured_ring[ringNum]->SetBinContent(wireNum, 1, w);
      }
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
   }

   int locDOCABin = (int) (distanceToWire * 10) % 8;
   TH2D* locHistToFill = cdc_measured_ringmap[locDOCABin][ringNum];
   lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
   {
      Double_t w = locHistToFill->GetBinContent(wireNum, 1) + 1.0;
      locHistToFill->SetBinContent(wireNum, 1, w);
   }
   lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

   if (FILL_DEDX_HISTOS) {
     if (withdEdx) {

       TH2D* locHistToFill = cdc_measured_with_dedx_ringmap[locDOCABin][ringNum];
       lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
       {
	 Double_t w = locHistToFill->GetBinContent(wireNum, 1) + 1.0;
	 locHistToFill->SetBinContent(wireNum, 1, w);
       }
       lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK

     }
   }
}

void JEventProcessor_CDC_Efficiency::Fill_ExpectedHit(int ringNum, int wireNum, double distanceToWire)
{
   //Double_t w = cdc_occ_ring[ring]->GetBinContent(straw, 1) + 1.0;
   //cdc_occ_ring[ring]->SetBinContent(straw, 1, w);
   //Fill the expected number of hits histogram
   if (distanceToWire < DOCACUT)
   {
      lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
      {
         Double_t w = cdc_expected_ring[ringNum]->GetBinContent(wireNum, 1) + 1.0;
         cdc_expected_ring[ringNum]->SetBinContent(wireNum, 1, w);
      }
      lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
   }

   int locDOCABin = (int) (distanceToWire * 10) % 8;
   TH2D* locHistToFill = cdc_expected_ringmap[locDOCABin][ringNum];
   lockService->RootFillLock(this); //ACQUIRE ROOT FILL LOCK
   {
      Double_t w = locHistToFill->GetBinContent(wireNum, 1) + 1.0;
      locHistToFill->SetBinContent(wireNum, 1, w);
   }
   lockService->RootFillUnLock(this); //RELEASE ROOT FILL LOCK
}

const DCDCTrackHit* JEventProcessor_CDC_Efficiency::Find_Hit(int locRing, int locProjectedStraw, map<int, set<const DCDCTrackHit*> >& locSortedCDCTrackHits)
{
   if(!locSortedCDCTrackHits[locProjectedStraw].empty())
      return *(locSortedCDCTrackHits[locProjectedStraw].begin());

   int locNumStraws = cdcwires[locRing - 1].size();

   //previous straw
   int locSearchStraw = locProjectedStraw - 1;
   if(locSearchStraw <= 0)
      locSearchStraw += locNumStraws;
   if(!locSortedCDCTrackHits[locSearchStraw].empty())
      return *(locSortedCDCTrackHits[locProjectedStraw].begin());

   //next straw
   locSearchStraw = locProjectedStraw + 1;
   if(locSearchStraw > locNumStraws)
      locSearchStraw -= locNumStraws;
   if(!locSortedCDCTrackHits[locSearchStraw].empty())
      return *(locSortedCDCTrackHits[locProjectedStraw].begin());

   return nullptr;
}

double JEventProcessor_CDC_Efficiency::GetDOCAFieldOff(DVector3 wirePosition, DVector3 wireDirection, DVector3 trackPosition, DVector3 trackMomentum, DVector3 &POCAOnTrack, DVector3 &POCAOnWire){
   // Get the vector pointing from the wire to the doca point
   Float_t a = trackMomentum.Dot(trackMomentum);
   Float_t b = trackMomentum.Dot(wireDirection);
   Float_t c = wireDirection.Dot(wireDirection);
   DVector3 w0 = trackPosition - wirePosition;
   Float_t d = trackMomentum.Dot(w0);
   Float_t e = wireDirection.Dot(w0);
   Float_t sc = ((b*e - c*d)/(a*c-b*b));
   Float_t tc = ((a*e - b*d)/(a*c-b*b));
   //if (sc < 0) continue; // Track must come from location away from origin
   POCAOnTrack = trackPosition + sc * trackMomentum;
   POCAOnWire  = wirePosition + tc * wireDirection;
   DVector3 LOCA = w0 + sc*trackMomentum - tc*wireDirection;
   return LOCA.Mag();
}

//------------------
// EndRun
//------------------
void JEventProcessor_CDC_Efficiency::EndRun()
{
   // This is called whenever the run number changes, before it is
   // changed to give you a chance to clean up before processing
   // events from the next run number.
   return;
}

//------------------
// Finish
//------------------
void JEventProcessor_CDC_Efficiency::Finish()
{
   return;
}

