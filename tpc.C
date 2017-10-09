#define tpc_cxx
// The class definition in tpc.h has been generated automatically
// by the ROOT utility TTree::MakeSelector(). This class is derived
// from the ROOT class TSelector. For more information on the TSelector
// framework see $ROOTSYS/README/README.SELECTOR or the ROOT User Manual.

// The following methods are defined in this file:
//    Begin():        called every time a loop on the tree starts,
//                    a convenient place to create your histograms.
//    SlaveBegin():   called after Begin(), when on PROOF called only on the
//                    slave servers.
//    Process():      called for each event, in this function you decide what
//                    to read and fill your histograms.
//    SlaveTerminate: called at the end of the loop on the tree, when on PROOF
//                    called only on the slave servers.
//    Terminate():    called at the end of the loop on the tree,
//                    a convenient place to draw/fit your histograms.
//
// To use this file, try the following session on your Tree T:
//
// Root > h101->Process("tpc.C")
// Root > h101->Process("tpc.C","options", no. of events, first event)
// Root > h101->Process("tpc.C+")
//

#define ENABLE_DEBUG 0  // Verbose logging with LOG() = printf().

#define TRACELENGTH 2692  // Trace length.

#include "anafadc.C"  // Signal analysis class.

#include "tpc.h"
#include <TH2.h>
#include <TStyle.h>
#include <TSystem.h>
#include "TCanvas.h"
#include "TLatex.h"
#include "TLine.h"
#include <TH1I.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TF1.h>

#define DISPLAY_THR 10.0  // Anode energy threshold, AU.
#define THR0 5.0  // Anode energy threshold on central anodes, AU.
#define THR1 5.0  // Anode energy threshold on ring 1, AU.
#define THR2 5.0  // Anode energy threshold on ring 2, AU.
#define THR3 5.0  // Anode energy threshold on ring 3, AU.
#define THR4 5.0  // Anode energy threshold on ring 4, AU.
#define ER4MAX 100.0  // Maximum allowed ring 4 energy, AU.

#define NEIGHBOR_DT 34  // Maximum left and right neighbor begin time deviation, ch.
#define R32R21 30  // Maximum ring 3 & 2 vs. ring 2 & 1 time difference, ch.
#define MAX_RING_DT 200  // Maximum time difference on adjacent rings, ch.
#define MIN_RING_DT -5  // Minimum time difference on adjacent rings (leeway for tracks parallel to anode), ch.
// #define SHAPING_T 75  // 3 us, 0 < Tend1 - Tbeg2 < SHAPING_T, ch.
#define SHAPING_T 35  // 1.4 us, abs(Tend1 - SHAPING_T - Tbeg2) < ENDBEG_DT, ch.
#define ENDBEG_DT 75

#define TMIN 200  // Left time window, ch.
#define TMAX 2600  // Right time window, ch.
#define DURMIN 30  // Minimal signal duration, ch.
#define DURMAX 210  // Maximal signal duration, ch.

#define MAXCLUSTERS 13  // Maximum number of valid anode clusters.
#define MAXGOOD 9  // Maximum number of valid signals per cluster.

#define MAXPDF 200  // Maximum number of debug pages to write to PDF.

#if ENABLE_DEBUG
#define LOG printf
#else
#define LOG(...)
#endif

int sCount(TH1I, int, int);

TH1I *ht[69];  // Traces.
TH1I *hts[67];  // Traces, smoothed.
TH1I *hp[67];  // Pedestals.
TH1I *hlanti = new TH1I("hlanti", "S1*!(S2+S3) counts", TRACELENGTH, 0, TRACELENGTH); // Anticoincidence logical signal trace.

float	e[67],  // Anode energies,
pbeg[67],  // signal begin times,
pend[67];  // signal end times.

// Create ROOT file.
TFile ft("tree.root", "RECREATE");

// Create tree.
TTree *t1 = new TTree("t1","Signal parameters");
// Add branches.
Float_t tbeg[67], tend[67], ea[67], etot, er0, er1, er2, er3, er4;
Float_t dbegc, dbeg166, dbeg21, dbeg32, dbeg43, dend0beg0, dend1beg2, dend2beg3, dend3beg4;
Int_t mr0, mr1, mr2, mr3, mr4;  // Multiplicity (main + neighbors) on rings 0-4.
Int_t tevent;

// Some counters.
int event = 0;
int passedVolumeCutCount = 0;
int aboveThresholdCount = 0;
int validClustersCount = 0;
int aboveThr65Count = 0;
int aboveThr66Count = 0;
int aboveThr65and66Count = 0;
int passedVolumeCutAndAboveThr65 = 0;
int valid65Count = 0;
int valid66Count = 0;
int valid65and66Count = 0;
int eventsWithOneAntiCoincidence = 0;
int page = 1;

int MaxNeighborI(int a, int b, int c) {
   float maxe = e[a];
   int maxi = a;
   if (e[b] > maxe) {
      maxe = e[b];
      maxi = b;
   }
   if (e[c] > maxe) {
      maxe = e[c];
      maxi = c;
   }
   return maxi;
}

bool ValidSignal(int anode) {
   float b = pbeg[anode];
   float e = pend[anode];
   float d = e - b;
   // 	if ((b > TMIN) && (b < (TMAX - DURMIN)) &&  // Not valid for self mode.
   if ((b > TMIN) && (e < TMAX) &&
      (d < DURMAX) && (d > DURMIN))
      return true;
   else
      return false;
}

bool IsValidWidth(int anode) {
   float d = pend[anode] - pbeg[anode];
   if ((d < DURMAX) && (d > DURMIN))
      return true;
   else
      return false;
}

bool IsValidPosition(int anode) {
   float b = pbeg[anode];
   float e = pend[anode];
   if ((b > TMIN) && (e < TMAX))  // Should always be true in self mode.
      return true;
   else
      return false;
}

int sCount(TH1I h, int startCh, int endCh) {
	const int losthr = -400;  // NIM signal is negative.
	bool below = false;
	int npeaks = 0;

	for (int i = startCh; i <= endCh; i++) {
		if (h.GetBinContent(i) < losthr) {
			below = true;
		}
		else {
			if (below == true) {
				npeaks++;
			}
			below = false;
		}
	}

	return npeaks;
}



void tpc::Begin(TTree * /*tree*/)
{
   // The Begin() function is called at the start of the query.
   // When running with PROOF Begin() is only called on the client.
   // The tree argument is deprecated (on PROOF 0 is passed).

   TString option = GetOption();

   gROOT->ProcessLine("gROOT->SetBatch()");

   for (int j = 0; j <= 66; j++) {
      char *histname = new char[255];
      sprintf(histname, "A%d trace", j);
      ht[j] = new TH1I(histname, histname, TRACELENGTH, 0, TRACELENGTH);
      hts[j] = new TH1I("Smoothed", "Smoothed", TRACELENGTH, 0, TRACELENGTH);
      hp[j] = new TH1I("Pedestal", "Pedestal", TRACELENGTH, 0, TRACELENGTH);
   }

   TCanvas * c0 = new TCanvas("cnvs0");
   c0->Print("events.pdf[");  // Start writing to file, don't actually write ([).
   delete c0;

   t1->Branch("event", &tevent, "tevent/I");  // Event number.
   t1->Branch("tbeg", tbeg, "tbeg[67]/F");  // Signal begin time, ch.
	t1->Branch("tend", tend, "tend[67]/F");  // Signal end time, ch.
   t1->Branch("ea", ea, "ea[67]/F");  // Individual anode energies, AU.
   t1->Branch("etot", &etot, "etot/F");  // Total cluster energy, AU.
   t1->Branch("er0", &er0, "er0/F");  // A65 + A66 energy.
   t1->Branch("er1", &er1, "er1/F");  // Ring 1 (anodes 1, 5, 9, ...) energy.
	t1->Branch("er2", &er2, "er2/F");  // Ring 2 (anodes 2, 6, 10, ...) energy.
	t1->Branch("er3", &er3, "er3/F");  // Ring 3 (anodes 3, 7, 11, ...) energy.
	t1->Branch("er4", &er4, "er4/F");  // Ring 4 (anodes 4, 8, 12, ...) energy.
	t1->Branch("dbegc", &dbegc, "dbegc/F");  // Tbeg[66] - Tbeg[65], ch.
	t1->Branch("dbeg166", &dbeg166, "dbeg166/F");  // Tbeg[ring1] - Tbeg[66], ch.
	t1->Branch("dbeg21", &dbeg21, "dbeg21/F");  // Tbeg[ring2] - Tbeg[ring1], ch.
	t1->Branch("dbeg32", &dbeg32, "dbeg32/F");  // Tbeg[ring3] - Tbeg[ring2], ch.
	t1->Branch("dbeg43", &dbeg43, "dbeg43/F");  // Tbeg[ring4] - Tbeg[ring3], ch.
	t1->Branch("dend1beg2", &dend1beg2, "dend1beg2/F");  // Tend[ring1] - Tbeg[ring2], ch.
	t1->Branch("dend2beg3", &dend2beg3, "dend2beg3/F");  // Tend[ring2] - Tbeg[ring3], ch.
	t1->Branch("dend3beg4", &dend3beg4, "dend3beg4/F");  // Tend[ring3] - Tbeg[ring4], ch.
   t1->Branch("mr0", &mr0, "mr0/I");  // Multiplicity (A65, A66 or both).
   t1->Branch("mr1", &mr1, "mr1/I");  // Multiplicity (number of ring 1 signals + neighbors).
   t1->Branch("mr2", &mr2, "mr2/I");  // Multiplicity (number of ring 2 signals + neighbors).
   t1->Branch("mr3", &mr3, "mr3/I");  // Multiplicity (number of ring 3 signals + neighbors).
   t1->Branch("mr4", &mr4, "mr4/I");  // Multiplicity (number of ring 4 signals + neighbors).

}

void tpc::SlaveBegin(TTree * /*tree*/)
{
   // The SlaveBegin() function is called after the Begin() function.
   // When running with PROOF SlaveBegin() is called on each slave server.
   // The tree argument is deprecated (on PROOF 0 is passed).

   TString option = GetOption();

}

Bool_t tpc::Process(Long64_t entry)
{
   // The Process() function is called for each entry in the tree (or possibly
   // keyed object in the case of PROOF) to be processed. The entry argument
   // specifies which entry in the currently loaded tree is to be processed.
   // It can be passed to either tpc::GetEntry() or TBranch::GetEntry()
   // to read either all or the required parts of the data. When processing
   // keyed objects with PROOF, the object is already loaded and is available
   // via the fObject pointer.
   //
   // This function should contain the "body" of the analysis. It can contain
   // simple or elaborate selection criteria, run algorithms on the data
   // of the event and typically fill histograms.
   //
   // The processing can be stopped by calling Abort().
   //
   // Use fStatus to set the return value of TTree::Process().
   //
   // The return value is currently not used.

   fChain->GetTree()->GetEntry(entry);

   float dT166 = -1, dT21 = -1, dT32 = -1, dT43 = -1;  // Begin time difference between max signal on rings.
   float dTe66b1 = -1, dTe1b2 = -1, dTe2b3 = -1, dTe3b4 = -1;  // End minus begin of max signals on susequent rings.

   bool passedVolumeCut = false;
   bool aboveThr65 = false;
   bool aboveThr66 = false;

   struct cluster {
      int goodAnodes[5][MAXGOOD];  // center (0) + 4 rings, MAXGOOD anodes.
      float energy;
      bool valid;
   };
   cluster clusters[MAXCLUSTERS] = {};

   // Initialize TTree variables.
   for (int i = 0; i < 67; i++) {
      tbeg[i] = -1;
      tend[i] = -1;
      ea[i] = -1;
   }
   etot = -1;
   er0 = -1;
   er1 = -1;
   er2 = -1;
   er3 = -1;
   er4 = -1;
   dbeg21 = -1;
   dbeg32 = -1;
   dbeg43 = -1;
   mr0 = 0;
   mr1 = 0;
   mr2 = 0;
   mr3 = 0;
   mr4 = 0;

	AnaFADC *anaSignal = new AnaFADC(TRACELENGTH);

   for(int i = 0; i < TRACELENGTH; i++) {
      anaSignal->trace[1][i] = at_a1c1sv[i] - 8192;
      anaSignal->trace[2][i] = at_a1c2sv[i] - 8192;
      anaSignal->trace[3][i] = at_a1c3sv[i] - 8192;
      anaSignal->trace[4][i] = at_a1c4sv[i] - 8192;
      anaSignal->trace[5][i] = at_a1c5sv[i] - 8192;
      anaSignal->trace[6][i] = at_a1c6sv[i] - 8192;
      anaSignal->trace[7][i] = at_a1c7sv[i] - 8192;
      anaSignal->trace[8][i] = at_a1c8sv[i] - 8192;
      anaSignal->trace[9][i] = at_a1c9sv[i] - 8192;
      anaSignal->trace[10][i] = at_a1c10sv[i] - 8192;
      anaSignal->trace[11][i] = at_a1c11sv[i] - 8192;
      anaSignal->trace[12][i] = at_a1c12sv[i] - 8192;
      anaSignal->trace[13][i] = at_a1c13sv[i] - 8192;
      anaSignal->trace[14][i] = at_a1c14sv[i] - 8192;
      anaSignal->trace[15][i] = at_a1c15sv[i] - 8192;
      anaSignal->trace[16][i] = at_a1c16sv[i] - 8192;
      anaSignal->trace[17][i] = at_a2c1sv[i] - 8192;
      anaSignal->trace[18][i] = at_a2c2sv[i] - 8192;
      anaSignal->trace[19][i] = at_a2c3sv[i] - 8192;
      anaSignal->trace[20][i] = at_a2c4sv[i] - 8192;
      anaSignal->trace[21][i] = at_a2c5sv[i] - 8192;
      anaSignal->trace[22][i] = at_a2c6sv[i] - 8192;
      anaSignal->trace[23][i] = at_a2c7sv[i] - 8192;
      anaSignal->trace[24][i] = at_a2c8sv[i] - 8192;
      anaSignal->trace[25][i] = at_a2c9sv[i] - 8192;
      anaSignal->trace[26][i] = at_a2c10sv[i] - 8192;
      anaSignal->trace[27][i] = at_a2c11sv[i] - 8192;
      anaSignal->trace[28][i] = at_a2c12sv[i] - 8192;
      anaSignal->trace[29][i] = at_a2c13sv[i] - 8192;
      anaSignal->trace[30][i] = at_a2c14sv[i] - 8192;
      anaSignal->trace[31][i] = at_a2c15sv[i] - 8192;
      anaSignal->trace[32][i] = at_a2c16sv[i] - 8192;
      anaSignal->trace[33][i] = at_a3c1sv[i] - 8192;
      anaSignal->trace[34][i] = at_a3c2sv[i] - 8192;
      anaSignal->trace[35][i] = at_a3c3sv[i] - 8192;
      anaSignal->trace[36][i] = at_a3c4sv[i] - 8192;
      anaSignal->trace[37][i] = at_a3c5sv[i] - 8192;
      anaSignal->trace[38][i] = at_a3c6sv[i] - 8192;
      anaSignal->trace[39][i] = at_a3c7sv[i] - 8192;
      anaSignal->trace[40][i] = at_a3c8sv[i] - 8192;
      anaSignal->trace[41][i] = at_a3c9sv[i] - 8192;
      anaSignal->trace[42][i] = at_a3c10sv[i] - 8192;
      anaSignal->trace[43][i] = at_a3c11sv[i] - 8192;
      anaSignal->trace[44][i] = at_a3c12sv[i] - 8192;
      anaSignal->trace[45][i] = at_a3c13sv[i] - 8192;
      anaSignal->trace[46][i] = at_a3c14sv[i] - 8192;
      anaSignal->trace[47][i] = at_a3c15sv[i] - 8192;
      anaSignal->trace[48][i] = at_a3c16sv[i] - 8192;
      anaSignal->trace[49][i] = at_a4c1sv[i] - 8192;
      anaSignal->trace[50][i] = at_a4c2sv[i] - 8192;
      anaSignal->trace[51][i] = at_a4c3sv[i] - 8192;
      anaSignal->trace[52][i] = at_a4c4sv[i] - 8192;
      anaSignal->trace[53][i] = at_a4c5sv[i] - 8192;
      anaSignal->trace[54][i] = at_a4c6sv[i] - 8192;
      anaSignal->trace[55][i] = at_a4c7sv[i] - 8192;
      anaSignal->trace[56][i] = at_a4c8sv[i] - 8192;
      anaSignal->trace[57][i] = at_a4c9sv[i] - 8192;
      anaSignal->trace[58][i] = at_a4c10sv[i] - 8192;
      anaSignal->trace[59][i] = at_a4c11sv[i] - 8192;
      anaSignal->trace[60][i] = at_a4c12sv[i] - 8192;
      anaSignal->trace[61][i] = at_a4c13sv[i] - 8192;
      anaSignal->trace[62][i] = at_a4c14sv[i] - 8192;
      anaSignal->trace[63][i] = at_a4c15sv[i] - 8192;
      anaSignal->trace[64][i] = at_a4c16sv[i] - 8192;
      anaSignal->trace[65][i] = at_a5c1sv[i] - 8192;
      anaSignal->trace[66][i] = at_a5c2sv[i] - 8192;
      hlanti->SetBinContent(i + 1, (float) at_a5c7sv[i] - 8192);  // S1*!(S2+S3) histogram.
   }

   // Volume cut (for low beam rate only).
   // if ((sCount(*hlanti, 1, 50) == 0) && (sCount(*hlanti, 1800, 1900) == 0) && (sCount(*hlanti, 51, 1799) == 1)) {
   if (true) {

		passedVolumeCut = true;
		passedVolumeCutCount++;
		for (int i = 1; i < 67; i++) {

			anaSignal->Analyze(i);
			e[i] = anaSignal->Energy;
			pbeg[i] = anaSignal->T_begin_f;
			pend[i] = anaSignal->T_end_f;
			//printf("A%d: E = %f (%f - %f) \n", i, anaSignal->Energy, anaSignal->T_begin_f, anaSignal->T_end_f);
			char *httitle = new char[255];
			sprintf(httitle, "A%d, E = %.0f AU [%.0f-%.0f] (%.0f)", i, e[i], pbeg[i], pend[i], pend[i] - pbeg[i]);
			ht[i]->SetTitle(httitle);

			for (int j = 0; j < TRACELENGTH; j++) {
				ht[i]->SetBinContent(j + 1, anaSignal->trace[i][j]);
				hts[i]->SetBinContent(j + 1, anaSignal->signal[j]);
				hp[i]->SetBinContent(j + 1, anaSignal->Pedestal[i][j]);
			}
		}

		int neighbor_left;
		int neighbor_right;
		int currentCluster = 0;

		int currentAnode = 0;

		// Central beam anode (A65).
		LOG("A65: E = %.0f AU [%.0f-%.0f] (%.0f): ", e[65], pbeg[65], pend[65], pend[65] - pbeg[65]);
		if ((e[65] > THR0) && ValidSignal(65)) {  // TODO: New subroutines.
			LOG("valid.\n");
			aboveThr65Count++;
			aboveThr65 = true;
		} else {
			LOG("invalid.\n");
		}

		if (passedVolumeCut && aboveThr65)
			passedVolumeCutAndAboveThr65++;

		// Central ring anode (A66).
		LOG("A66: E = %.0f AU [%.0f-%.0f] (%.0f): ", e[66], pbeg[66], pend[66], pend[66] - pbeg[66]);
		int dT6665 = pbeg[66] - pbeg[65];
		if ((e[66] > THR0) && ValidSignal(66) &&
			((pend[65] - pbeg[66]) > 0) && (dT6665 < MAX_RING_DT) && (dT6665 > MIN_RING_DT)) {
         LOG("valid.\n");
         aboveThr66Count++;
         aboveThr66 = true;
      } else {
				LOG("invalid.\n");
		}

      for (int i = 1; i < 62; i = i + 4) {

         if ((i < 17) || (i > 32)) {  // Exclude FADC2 with different shaping time.

            if (aboveThr65) {
               clusters[currentCluster].goodAnodes[0][0] = 65;
               clusters[currentCluster].energy = e[65];
            }

            if (aboveThr66) {
               clusters[currentCluster].goodAnodes[0][1] = 66;
               clusters[currentCluster].energy = clusters[currentCluster].energy + e[66];
            }

            clusters[currentCluster].valid = false;

            // Ring 1.
            if ((e[i] > THR1) && IsValidWidth(i)) {
               dT166 = pbeg[i] - pbeg[66];
               dTe66b1 = pend[66] - pbeg[i];
               LOG("*** A%d cluster is a candidate: E = %.1f [%.0f - %.0f]\n", i, e[i], pbeg[i], pend[i]);
               if (IsValidPosition(i) && aboveThr65 && aboveThr66)
                  clusters[currentCluster].valid = true;
               if ((dT166 > MAX_RING_DT) || (dT166 < MIN_RING_DT) ||
                  //(pend[i] - pbeg[maxR2I] < 0) || (pend[i] - pbeg[maxR2I] > SHAPING_T))
                  (abs(dTe66b1 - SHAPING_T) > ENDBEG_DT))
                  clusters[currentCluster].valid = false;
               clusters[currentCluster].goodAnodes[1][currentAnode] = i;
               clusters[currentCluster].energy = clusters[currentCluster].energy + e[i];
               currentAnode++;
               if (i == 1)
                  neighbor_left = 61;
               else
                  neighbor_left = i - 4;
               if ((abs(pbeg[neighbor_left] - pbeg[i]) < NEIGHBOR_DT) && ValidSignal(neighbor_left)) {  // TODO: Update.
                  LOG("Adding A%d as a neigbor of A%d\n", neighbor_left, i);
                  clusters[currentCluster].goodAnodes[1][currentAnode] = neighbor_left;
                  clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_left];
                  currentAnode++;
               }
               if (i == 61)
                  neighbor_right = 1;
               else
                  neighbor_right = i + 4;
               if ((abs(pbeg[neighbor_right] - pbeg[i]) < NEIGHBOR_DT) && ValidSignal(neighbor_right)) {
                  LOG("Adding A%d as a neigbor of A%d\n", neighbor_right, i);
                  clusters[currentCluster].goodAnodes[1][currentAnode] = neighbor_right;
                  clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_right];
                  currentAnode++;
               }

               // Ring 2.
               currentAnode = 0;
               int r2i = i + 1;
               if (r2i == 2)
                  neighbor_left = 62;
               else
                  neighbor_left = r2i - 4;
               if (r2i == 62)
                  neighbor_right = 2;
               else
                  neighbor_right = r2i + 4;
               int maxR2I = MaxNeighborI(r2i, neighbor_left, neighbor_right);
               //LOG("A%d: %.1f > %.1f, width_valid = %d, dt = %.1f \n", r2i, e[r2i], THR2, IsValidWidth(maxR2I), dT21);
               if ((e[maxR2I] > THR2) && IsValidWidth(maxR2I)) {
                  dT21 = pbeg[maxR2I] - pbeg[i];
                  dTe1b2 = pend[i] - pbeg[maxR2I];
                  LOG("Ring 2 passed\n");
                  if (!IsValidPosition(maxR2I) || //(dT21 > MAX_RING_DT) || (dT21 < MIN_RING_DT) ||
                     //(pend[i] - pbeg[maxR2I] < 0) || (pend[i] - pbeg[maxR2I] > SHAPING_T))
                     (abs(dTe1b2 - SHAPING_T) > ENDBEG_DT))
                     clusters[currentCluster].valid = false;
                  LOG("Ring 2: A%d is the biggest\n", maxR2I);
                  clusters[currentCluster].goodAnodes[2][currentAnode] = maxR2I;
                  clusters[currentCluster].energy = clusters[currentCluster].energy + e[maxR2I];
                  currentAnode++;

                  if (maxR2I == r2i) {
                     if ((abs(pbeg[neighbor_left] - pbeg[r2i]) < NEIGHBOR_DT) && ValidSignal(neighbor_left)) {
                        LOG("Adding A%d as a neigbor of A%d\n", neighbor_left, maxR2I);
                        clusters[currentCluster].goodAnodes[2][currentAnode] = neighbor_left;
                        clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_left];
                        currentAnode++;
                     }

                     if ((abs(pbeg[neighbor_right] - pbeg[r2i]) < NEIGHBOR_DT) && ValidSignal(neighbor_right)) {
                        LOG("Adding A%d as a neigbor of A%d\n", neighbor_right, maxR2I);
                        clusters[currentCluster].goodAnodes[2][currentAnode] = neighbor_right;
                        clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_right];
                        currentAnode++;
                     }
                  } else {
                     if ((abs(pbeg[maxR2I] - pbeg[r2i]) < NEIGHBOR_DT) && ValidSignal(r2i)) {
                        LOG("Adding A%d as a neigbor of A%d\n", r2i, maxR2I);
                        clusters[currentCluster].goodAnodes[2][currentAnode] = r2i;
                        clusters[currentCluster].energy = clusters[currentCluster].energy + e[r2i];
                        currentAnode++;
                     }
                  }

                  // Ring 3.
                  currentAnode = 0;
                  int r3i = i + 2;
                  if (r3i == 3)
                     neighbor_left = 63;
                  else
                     neighbor_left = r3i - 4;
                  if (r3i == 63)
                     neighbor_right = 3;
                  else
                     neighbor_right = r3i + 4;
                  int maxR3I = MaxNeighborI(r3i, neighbor_left, neighbor_right);

                  //LOG("A%d: %.1f > %.1f, valid = %d, dbeg32 = %.1f, ddt = %.1f \n", r3i, e[r3i], THR3, ValidSignal(r3i), dT32, abs(dT21 - dT32));

                  if ((e[maxR3I] > THR3) && IsValidWidth(maxR3I)) {  // TODO: check if on straight line.
                     dT32 = pbeg[maxR3I] - pbeg[maxR2I];
                     dTe2b3 = pend[maxR2I] - pbeg[maxR3I];
                     if (!IsValidPosition(maxR3I) || (dT32 > MAX_RING_DT) || (dT32 < MIN_RING_DT) || (abs(dT21 - dT32) > R32R21) ||
                        //(pend[maxR2I] - pbeg[maxR3I] < 0) || (pend[maxR2I] - pbeg[maxR3I] > SHAPING_T))
                        (abs(dTe2b3 - SHAPING_T) > ENDBEG_DT))
                        clusters[currentCluster].valid = false;
                     LOG("Ring 3: A%d is the biggest\n", maxR3I);
                     clusters[currentCluster].goodAnodes[3][currentAnode] = maxR3I;
                     clusters[currentCluster].energy = clusters[currentCluster].energy + e[maxR3I];
                     currentAnode++;

                     if (maxR3I == r3i) {
                        if ((abs(pbeg[neighbor_left] - pbeg[r3i]) < NEIGHBOR_DT) && ValidSignal(neighbor_left)) {
                           LOG("Adding A%d as a neigbor of A%d\n", neighbor_left, maxR3I);
                           clusters[currentCluster].goodAnodes[3][currentAnode] = neighbor_left;
                           clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_left];
                           currentAnode++;
                        }

                        if ((abs(pbeg[neighbor_right] - pbeg[r3i]) < NEIGHBOR_DT) && ValidSignal(neighbor_right)) {
                           LOG("Adding A%d as a neigbor of A%d\n", neighbor_right, maxR3I);
                           clusters[currentCluster].goodAnodes[3][currentAnode] = neighbor_right;
                           clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_right];
                           currentAnode++;
                        }
                     } else {
                        if ((abs(pbeg[maxR3I] - pbeg[r3i]) < NEIGHBOR_DT) && ValidSignal(r3i)) {
                           LOG("Adding A%d as a neigbor of A%d\n", r3i, maxR3I);
                           clusters[currentCluster].goodAnodes[3][currentAnode] = r3i;
                           clusters[currentCluster].energy = clusters[currentCluster].energy + e[r3i];
                           currentAnode++;
                        }
                     }

                     // Ring 4.
                     currentAnode = 0;
                     int r4i = i + 3;
                     if (r4i == 4)
                        neighbor_left = 64;
                     else
                        neighbor_left = r4i - 4;
                     if (r4i == 64)
                        neighbor_right = 4;
                     else
                        neighbor_right = r4i + 4;
                     int maxR4I = MaxNeighborI(r4i, neighbor_left, neighbor_right);
                     float eR4 = 0;

                     //LOG("A%d: %.1f > %.1f, valid = %d, dbeg43 = %.1f, ddt = %.1f \n", maxR4I, e[maxR4I], THR4, ValidSignal(maxR4I), dT43, abs(dT21 - dT43));

                     if ((e[maxR4I] > THR4) && IsValidWidth(maxR4I)) {  // TODO: check if on straight line.
                        dT43 = pbeg[maxR4I] - pbeg[maxR3I];
                        dTe3b4 = pend[maxR3I] - pbeg[maxR4I];
                        if (!IsValidPosition(maxR4I) || (dT43 > MAX_RING_DT) || (dT43 < MIN_RING_DT) || (abs(dT21 - dT43) > R32R21) ||
                           //(pend[maxR3I] - pbeg[maxR4I] < 0) || (pend[maxR3I] - pbeg[maxR4I] > SHAPING_T))
                           (abs(dTe3b4 - SHAPING_T) > ENDBEG_DT))
                           clusters[currentCluster].valid = false;
                        clusters[currentCluster].goodAnodes[4][currentAnode] = maxR4I;
                        clusters[currentCluster].energy = clusters[currentCluster].energy + e[maxR4I];
                        currentAnode++;

                        eR4 = eR4 + e[maxR4I];

                        if (maxR4I == r4i) {
                           if ((abs(pbeg[neighbor_left] - pbeg[r4i]) < NEIGHBOR_DT) && ValidSignal(neighbor_left)) {
                              clusters[currentCluster].goodAnodes[4][currentAnode] = neighbor_left;
                              clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_left];
                              currentAnode++;
                              eR4 = eR4 + e[neighbor_left];
                           }

                           if ((abs(pbeg[neighbor_right] - pbeg[r4i]) < NEIGHBOR_DT) && ValidSignal(neighbor_right)) {
                              clusters[currentCluster].goodAnodes[4][currentAnode] = neighbor_right;
                              clusters[currentCluster].energy = clusters[currentCluster].energy + e[neighbor_right];
                              currentAnode++;
                              eR4 = eR4 + neighbor_right;
                           }
                        } else {
                           if ((abs(pbeg[maxR4I] - pbeg[r4i]) < NEIGHBOR_DT) && ValidSignal(r4i)) {
                              clusters[currentCluster].goodAnodes[4][currentAnode] = r4i;
                              clusters[currentCluster].energy = clusters[currentCluster].energy + e[r4i];
                              currentAnode++;
                              eR4 = eR4 + e[r4i];
                           }
                        }
                        if (eR4 > ER4MAX)
                           clusters[currentCluster].valid = false;
                        LOG("Cluster %d: ER4 = %f, valid = %d \n", currentCluster, eR4, clusters[currentCluster].valid);
                     }
                  }

               }
               if (clusters[currentCluster].valid) {
                  LOG("=== Cluster %d: valid = %d E = %.1f\n", currentCluster, clusters[currentCluster].valid, clusters[currentCluster].energy);
               }
               currentCluster++;
            }
         }
      }

      float maxe = -1000;
      int maxc = -1;
      for (int c = 0; c < MAXCLUSTERS; c++) {
         if (clusters[c].valid) {
            if (clusters[c].energy > maxe) {
               maxe = clusters[c].energy;
               maxc = c;
            }
         }
      }

      bool eat = false;
      for (int i = 1; i < 67; i++) {
         if (e[i] > DISPLAY_THR) {  // There are anodes with energy above THR.
            eat = true;
            break;
         }
      }

      if (eat)
         aboveThresholdCount++;

      if (maxc > -1) {
         validClustersCount++;
         if (clusters[maxc].goodAnodes[0][0] != 0)
            valid65Count++;
         if (clusters[maxc].goodAnodes[0][1] != 0)
            valid66Count++;
         if ((clusters[maxc].goodAnodes[0][0] != 0) && (clusters[maxc].goodAnodes[0][1] != 0))
            valid65and66Count++;
      }

      if (((maxc > -1) || eat) && (page < MAXPDF)) {  // There are either valid clusters or just anodes above THR.
         page++;
         TCanvas * cc = new TCanvas("cnvsC");
         gStyle->SetPaperSize(TStyle::kA4);
         cc->Divide(1, 2);
         cc->cd(1);

         // Draw central anodes.

         char *hstitle = new char[255];
         sprintf(hstitle, "Event %lld: E_{SUM} = %.1f [%.0f-%.0f] + %.1f [%.0f-%.0f] = %.1f AU", entry, e[65], pbeg[65], pend[65], e[66], pbeg[66], pend[66], e[65] + e[66]);
         TH1I * hs = new TH1I("A65+A66", hstitle, TRACELENGTH, 0, TRACELENGTH);
         hs->Add(ht[65]);
         hs->Add(ht[66]);
         hs->SetLineColor(kGray);
         hs->Draw();
         ht[65]->SetLineColor(kBlue);
         ht[65]->Draw("same");
         ht[66]->SetLineColor(kMagenta);
         ht[66]->Draw("same");

         if (maxc > -1) {
            char *estitle = new char[255];
            strcpy(estitle, "E_{R} = [");
            for (int ri = 0; ri < 5; ri++) {  // Rings 0 - 4.
               for (int ai = 0; ai < MAXGOOD; ai++) {  // Good anodes in ring.
                  int ca = clusters[maxc].goodAnodes[ri][ai];
                  if (ca > 0) {
                     char *ae = new char[255];
                     sprintf(ae, " %.1f_{A%d}", e[ca], ca);
                     strcat(estitle, ae);
                  }
               }
            }
            char *et = new char[255];
            sprintf(et, "] = %.1f AU", maxe);
            strcat(estitle, et);
            TLatex energySumLabel;
            energySumLabel.SetTextSize(0.04);
            int ypos = ht[65]->GetBinContent(1) + 150;
            energySumLabel.DrawLatex(300, ypos, estitle);
            LOG("%s\n", estitle);
         }

         // Draw anodes above threshold.
         TPad* pad2 = (TPad*)(cc->GetPrimitive("cnvsC_2"));
         pad2->Divide(4,2);
         int pad2i = 1;


         for (int k = 1; k < 65; k++) {
            bool foundInCluster = false;
            for (int ri = 0; ri < 5; ri++)
               for (int ai = 0; ai < MAXGOOD; ai++) {
                  if ((maxc > -1) && (clusters[maxc].valid) && (clusters[maxc].goodAnodes[ri][ai] == k)) {
                     LOG("Valid cluster: %d, [%d][%d] = %d \n", maxc, ri, ai, k);
                     foundInCluster = true;
                  }
               }
               if ((pad2i < 9) && (foundInCluster || (e[k] > DISPLAY_THR))) {
                  if (foundInCluster)
                     ht[k]->SetLineColor(kGreen);
                  else
                     ht[k]->SetLineColor(kBlue);
                  pad2->cd(pad2i);
                  pad2i++;
                  ht[k]->Draw();
                  //hts[i]->SetLineColor(kGray);
                  hts[k]->SetLineColorAlpha(kGray, 0.7);
                  hts[k]->Draw("same");
                  hp[k]->SetLineColorAlpha(kGray, 0.7);
                  hp[k]->Draw("same");
                  auto lbeg = new TLine(pbeg[k], hp[k]->GetBinContent(TMIN + 1), pbeg[k], 100);
                  lbeg->SetLineColor(kBlack);
                  lbeg->SetLineStyle(3);
                  lbeg->Draw();
                  auto lend = new TLine(pend[k], hp[k]->GetBinContent(TMAX - 1), pend[k], 100);
                  lend->SetLineColor(kBlack);
                  lend->SetLineStyle(3);
                  lend->Draw();
               }
         }

         char *eventtitle = new char[255];
         sprintf(eventtitle, "Title:Event %lld", entry);
         cc->Print("events.pdf", eventtitle);

         delete hs;

         delete cc;
      }


      // Fill TTree.
      tevent = event;
      for (int i = 0; i < 67; i++) {
         tbeg[i] = pbeg[i];
         tend[i] = pend[i];
         ea[i] = e[i];
      }

      er0 = 0;
      er1 = 0;
      er2 = 0;
      er3 = 0;
      er4 = 0;
      dbegc = 0;
      dend0beg0 = 0;
      dbeg21 = 0;
      dbeg32 = 0;
      dbeg43 = 0;
      dend1beg2 = 0;
      dend2beg3 = 0;
      dend3beg4 = 0;

      if (aboveThr65) {
         er0 = e[65];
         etot = e[65];
      }
      if (aboveThr66) {
         er0 = er0 + e[66];
         dbegc = pbeg[66] - pbeg[65];
         dend0beg0 = pend[65] - pbeg[66];
         etot = etot + e[66];
      }
      if (maxc > -1) {
         etot = maxe;
         for (int i = 0; i < MAXGOOD; i++) {
            if (clusters[maxc].goodAnodes[0][i] > 0) {
               er0 = er0 + e[clusters[maxc].goodAnodes[0][i]];
               mr0++;
            }
            if (clusters[maxc].goodAnodes[1][i] > 0) {
               er1 = er1 + e[clusters[maxc].goodAnodes[1][i]];
               dbeg166 = dT166;
               mr1++;
            }
            if (clusters[maxc].goodAnodes[2][i] > 0) {
               er2 = er2 + e[clusters[maxc].goodAnodes[2][i]];
               dbeg21 = dT21;
               dend1beg2 = dTe1b2;
               mr2++;
            }
            if (clusters[maxc].goodAnodes[3][i] > 0) {
               er3 = er3 + e[clusters[maxc].goodAnodes[3][i]];
               dbeg32 = dT32;
               dend2beg3 = dTe2b3;
               mr3++;
            }
            if (clusters[maxc].goodAnodes[4][i] > 0) {
               er4 = er4 + e[clusters[maxc].goodAnodes[4][i]];
               dbeg43 = dT43;
               dend3beg4 = dTe3b4;
               mr4++;
            }
         }
      }

      t1->Fill();

	}

	delete anaSignal;

	event++;
	if (event%100 == 0)
		printf("%d events\n", event);

   return kTRUE;
}

void tpc::SlaveTerminate()
{
   // The SlaveTerminate() function is called after all entries or objects
   // have been processed. When running with PROOF SlaveTerminate() is called
   // on each slave server.

}

void tpc::Terminate()
{
   // The Terminate() function is the last function to be called during
   // a query. It always runs on the client, it can be used to present
   // the results graphically or save the results to file.

	ft.Write();
	ft.Close();

	gROOT->ProcessLine("gROOT->SetBatch(kFALSE)");

	TCanvas * c0 = new TCanvas("cnvs0");
	c0->Print("events.pdf]");  // Finish writing to file, don't actually write (]).
	delete c0;

	printf("--------------------------------------------------------\n");
	printf("Total events:                                 %d\n", event);
	printf("Events with only one anti-coincidence:        %d\n", eventsWithOneAntiCoincidence);
	printf("Passed volume cut:                            %d\n", passedVolumeCutCount);
	printf("Events with anodes above threshold:           %d [>%.1f AU]\n", aboveThresholdCount, DISPLAY_THR);
	printf("Events with valid clusters:                   %d\n", validClustersCount);
	printf("Events with valid A65 above threshold:        %d [>%.1f AU]\n", aboveThr65Count, THR0);
	printf("Events with valid A66 above threshold:        %d [>%.1f AU]\n", aboveThr66Count, THR0);
	printf("Passed volume cut AND A65 > THR0:             %d\n", passedVolumeCutAndAboveThr65);
	printf("Events with valid signal on A65:              %d\n", valid65Count);
	printf("Events with valid signal on A66:              %d\n", valid66Count);
	printf("Events with valid signal on A65 && A66:       %d\n", valid65and66Count);
}
