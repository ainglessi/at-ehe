//
// AnaFADC class for active target peak analysis
// based on anafadc.f code made for IKAR experiments by A. Dobrovolsky
// 2014 A. Inglessi <inglessi_ag@pnpi.nrcki.ru>
//
// To use, first create an object specifying trace length:
// AnaFADC *myAna = new AnaFADC(2050);
// then fill signal arrays with data:
// for(int i = 0; i<2050; i++) anaAlpha->trace[anode_no][i] = myHisto->GetBinContent(i);
// and call main analysis routine:
// anaAlpha->Analyze(anode_no);
//

#include <iostream>
#include <math.h>

#define NOFANODES 66  // number of anode signals
#define NOFLOGICAL 5  // number of logical signals
#define BINWIDTH 0.04  // 0.04 us = 40 ns
#define MAXPEAKS 50
#define SMOOTH1_T 30  // smoothing period for Smooth1 routine
#define EXT_LEFT 20  // left extension for integration
#define EXT_RIGHT 75  // right extension (also see RIGHTINTERSECTION below)
#define TIME_INTERVAL 0.04
#define RIGHTINTERSECTION 0  // 1 to use right signal intersection with pedestal for integration
#define THR 15  // anode energy threshold

#define DEBUG

#ifdef DEBUG
#  define D(x) x
#else
#  define D(x)
#endif

class AnaFADC {
public:
   AnaFADC(int traceLength);
   ~AnaFADC();
   int trace[NOFANODES + 1][TRACELENGTH];  // signals for all anodes (1 .. NOFANODES)
   float aenergy[NOFANODES + 1];  // energies
   float abegin[NOFANODES + 1];  // begins
   float aend[NOFANODES + 1];  // ends
   float aduration[NOFANODES + 1];  // durations
   float renergy[4];  // ring energies
   int recoilAnodes[12];  // Anode numbers where we look for recoil signal.
   void Analyze(int anodeNumber);  // common analysis, anode number = 1 .. NOFANODES (as per mapping)
   float signal[TRACELENGTH];  // current signal to process and analyze
   float signal_d1[TRACELENGTH];  // 1st derivative
   float Pedestal[NOFANODES + 1][TRACELENGTH];
   int traceLength;  // current length of trace in bins
   int MaxAmp, T_maxamp;  // maximum amplitude and its position
   int Duration,  // peak duration, [channels]
       T_begin,  // left tangent intersection point with pedestal
       T_end,  // right tangent intersection point with pedestal
       T_end_i;  // right signal intersection point with pedestal (before ripple drop)
   float T_begin_f, T_end_f;  // tangent intersection points with pedestal, float
   float Square,  // peak square
         Energy,  // peak energy
         Amplitude,  // peak amplitude
         FWHM,  // peak's FWHM
         Slope_l,  // max value of 1st derivative
         Slope_r;  // min value of 1st derivative

private:
   int currentAnodeNumber;
   void Smooth1(int T);  // smoothing (averaging withing given period)
   int GetPeakparam(int Iteration);  // peak analysis
   void Derivatives();  // derivative calculation
	void Risetime(int);  // risetime calculation subroutine
   bool Search_Xl_Xr(int Start, int Ende);   // derivative extremes calculation
   bool Calc_Il_Ir();  // derivatives intersection with pedestal
   bool Calc_Psquare();
   int Search_Pulse(int Npeak);
   float Itot;  // whole signal integral
   int Xl, Xr;  // derivative extremes coordinates
   int Il, Ir, Iri;  // intersection coordinates
   float Ilf, Irf;  // intersection coordinates, float
   bool Left_flag, Right_flag;
   float Psquare, Range;
   bool Valid[MAXPEAKS];
   bool VLeft[MAXPEAKS];
   bool VRight[MAXPEAKS];
   int P_Duration[MAXPEAKS];
   int x_l[MAXPEAKS];  // x-value of left slope parameter point
   int x_r[MAXPEAKS];  // x-value of right slope parameter point
   int I_l[MAXPEAKS];  // left tangent intersection point
   int I_r[MAXPEAKS];  // right tangent intersection point
   float I_l_f[MAXPEAKS];  // left tangent intersection point, float
   float I_r_f[MAXPEAKS];  // right tangent intersection point, float
   int I_r_i[MAXPEAKS];  // right signal intersection point
   float A_l[MAXPEAKS];  // Left slope parameter
   float A_r[MAXPEAKS];  // Right Slope parameter
   float B_l[MAXPEAKS];  // y-value of left slope parameter point
   float B_r[MAXPEAKS];  // y-value of right slope parameter point
   float I_2[MAXPEAKS];  // left intrsctn point trapezium top
   float I_3[MAXPEAKS];   // right intrsctn point trapezium top
   float P_Energy[MAXPEAKS];  // pulse energy
   float P_Square[MAXPEAKS];  // pulse square
   float P_Amplitude[MAXPEAKS];
   float P_FWHM[MAXPEAKS];
   int Int_Range[MAXPEAKS];
};



//
// Constructor
//
AnaFADC::AnaFADC(int traceLengthInit)
{
   currentAnodeNumber = -1;
   //printf("AnaFADC initialized. \n");
   for (int i = 0; i < TRACELENGTH; i++) {
      signal[i] = 0;
      signal_d1[i] = 0;
      for (int j = 0; j < NOFANODES + 1; j++) {
         Pedestal[j][i] = 0;
         trace[j][i] = 0;
      }
   }
   for (int i = 0; i < 4; i++)
      renergy[i] = 0;
   for (int i = 0; i < NOFANODES + 1; i++) {
      aenergy[i] = -1;
      abegin[i] = -1;
      aend[i] = -1;
      aduration[i] = -1;
   }
   for (int i = 0; i < 12; i++)
      recoilAnodes[i] = -1;
   traceLength = traceLengthInit;
   Duration = 0;
   T_begin = 0;
   T_end  = 0;
   T_end_i = 0;
   T_begin_f = 0;
   T_end_f = 0;
   Square = 0;
   Energy = 0;
   Amplitude = 0;
   FWHM = 0;
   Slope_l = 0;
   Slope_r = 0;
   Psquare = 0;
   Range = 0;
   for (int i = 0; i < MAXPEAKS; i++) {
      P_Duration[i] = 0;
      x_l[i] = 0;
      x_r[i] = 0;
      I_l[i] = 0;
      I_r[i] = 0;
      I_l_f[i] = 0;
      I_r_f[i] = 0;
      P_Square[i] = 0;
      P_Energy[i] = 0;
      P_Amplitude[i] = 0;
      P_FWHM[i] = 0;
      A_l[i] = 0;
      A_r[i] = 0;
   }
}


AnaFADC::~AnaFADC() {
}


//
// Main analysis routine
//
void AnaFADC::Analyze(int anodeNumber)
{
   int Iteration, Npeak, Pnumber;
   float Delta_ped, Iped;
   bool FirstSearch;
   bool PSC_Done = false;  // true when ped. correction for parallel shift is done

   currentAnodeNumber = anodeNumber;

   Duration = 0;
   T_begin = 0;
   T_end  = 0;
   T_end_i = 0;
   T_begin_f = 0;
   T_end_f   = 0;
   Square = 0;
   Energy = 0;
   Amplitude = 0;
   FWHM = 0;
   Slope_l = 0;
   Slope_r = 0;
   Psquare = 0;
   Range = 0;

   for (int i = 0; i < TRACELENGTH; i++) {
      signal[i] = 0;
      signal_d1[i] = 0;
      for (int j = 0; j < NOFANODES + 1; j++) {
         Pedestal[j][i] = 0;
      }
   }

   for (int i = 0; i < 12; i++)
      recoilAnodes[i] = -1;

   for (int i = 0; i < MAXPEAKS; i++) {
      P_Duration[i] = 0;
      x_l[i] = 0;
      x_r[i] = 0;
      I_l[i] = 0;
      I_r[i] = 0;
      I_l_f[i] = 0;
      I_r_f[i] = 0;
      P_Square[i] = 0;
      P_Energy[i] = 0;
      P_Amplitude[i] = 0;
      P_FWHM[i] = 0;
      A_l[i] = 0;
      A_r[i] = 0;
   }

   //printf("AnaFADC started. \n");
   for (int i = 0; i < traceLength; i++) {
      signal[i] = trace[anodeNumber][i];
   }
   //printf("signal[1000] = %f \n", signal[1000]);
   // smooth signal
   Smooth1(SMOOTH1_T); // TODO: determine optimal value
   // search for maximum, calculate integral
   Itot = 0;
   Iped = 0;
   MaxAmp = signal[0];
   T_maxamp = 0;

   for (int i = 0; i < traceLength; i++) {
      Itot = Itot + signal[i];
      if (signal[i] > MaxAmp) {
         MaxAmp = signal[i];
         T_maxamp = i;
      }
   }

   Iteration = 1;
   FirstSearch = false;
   Npeak = GetPeakparam(Iteration);
   //printf("GetPeakparam: %d peaks \n", Npeak);

search_start:

   if (Npeak == 0) {
      Delta_ped = Itot / traceLength;
      //printf("Npeak = 0 | Delta_ped: %f\n", Delta_ped);
      for (int i = 0; i < traceLength; i++) {
         Pedestal[currentAnodeNumber][i] = Pedestal[currentAnodeNumber][i] + Delta_ped;  // corrected pedestal
      }
      Iped = 0;
      for (int i = 0; i < traceLength; i++) {  // integral of corrected pedestal
         Iped = Iped + Pedestal[currentAnodeNumber][i];
      }
   }

   if (Npeak >= 1) {
      if (Npeak == 1) {
         Pnumber = 0;
      } else {
         Pnumber = Search_Pulse(Npeak);  // search for pulse with max energy
      }
      FirstSearch = true;
      int j = I_l[Pnumber] - EXT_LEFT;
      int k = I_r[Pnumber] + EXT_RIGHT;
      if (j < 0)
         j = 0;
      if (k > traceLength)
         k = traceLength;
      if (!PSC_Done) {
         float Isig = 0;
         float Ipp  = 0;
         for (int i = j; i < k; i++) {
            Isig = Isig + signal[i];
            Ipp = Ipp + Pedestal[currentAnodeNumber][i];
         }
         //printf("j=%d, k=%d, Itot=%.1f, Isig=%.1f, Iped=%.1f, Ipp=%.1f \n", j, k, Itot, Isig, Iped, Ipp);
         Delta_ped = (Itot - Isig) - (Iped - Ipp);
         // printf("traceLength = %d, k = %d, j = %d \n", traceLength, k, j);
         if ((traceLength - (k - j + 1) > 0)) {
            Delta_ped = Delta_ped / (traceLength - (k - j + 1));
         }
         Iped = 0;
         //printf("Peaks: %d | I_l[%d]: %d | Delta_ped: %f\n", Pnumber, Pnumber, I_l[Pnumber], Delta_ped);
         for (int i = 0; i < traceLength; i++) {
            Pedestal[currentAnodeNumber][i] = Pedestal[currentAnodeNumber][i] + Delta_ped;
            Iped = Iped + Pedestal[currentAnodeNumber][i];
         }
         PSC_Done = true;
      }
   }

   Iteration++;
   Npeak = GetPeakparam(Iteration);
	//printf("Iteration: %d - Pedestal[2] = %.1f, Delta_ped = %.1f, Iped = %.1f \n", Iteration, Pedestal[currentAnodeNumber][2], Delta_ped, Iped);

   if (Npeak == 0) return;   // no peaks found

   if ((!FirstSearch) || (!PSC_Done)) {
      Iteration--;
      goto search_start;
   }

   if (Npeak == 1) {
      Pnumber = 0;
   } else {
      Pnumber = Search_Pulse(Npeak);  // search for pulse with max energy
   }

   Risetime(Pnumber);

   //~ printf("GetPeakparam: Pnumber=%d \n", Pnumber);
   //~ printf("peaks: %d \n", Npeak);
	//printf("A%d energy[%d-%d]: %f\n", Pnumber, I_l[Pnumber], I_r[Pnumber], P_Energy[Pnumber]);
   //~ printf("Delta_ped: %f\n", Delta_ped);
   //~ printf("FWHM: %f\n", P_FWHM[Pnumber]);
   //printf("Duration: %d\n", P_Duration[Pnumber]);

   Duration = P_Duration[Pnumber];
   T_begin = I_l[Pnumber];
   T_end  = I_r[Pnumber];
   T_end_i = I_r_i[Pnumber];
   T_begin_f = I_l_f[Pnumber];
   T_end_f   = I_r_f[Pnumber];
   Square = P_Square[Pnumber];
   Energy = P_Energy[Pnumber];
   Amplitude = P_Amplitude[Pnumber];
   FWHM = P_FWHM[Pnumber];
   Slope_l = A_l[Pnumber];
   Slope_r = A_r[Pnumber];

   aenergy[anodeNumber] = Energy;
   aduration[anodeNumber] = T_end_f - T_begin_f;
   abegin[anodeNumber] = T_begin;
   aend[anodeNumber] = T_end;

	//printf("A%d energy[%d-%d]: %f\n", Pnumber, I_l[Pnumber], I_r[Pnumber], P_Energy[Pnumber]);
	//printf("A%d energy[%f-%f]: %f\n", Pnumber, I_l_f[Pnumber], I_r_f[Pnumber], P_Energy[Pnumber]);
}



//
// Smooth1 - smooth the contents of FADC array so as to integrate the harmonic with the period of T (in channels)
//
void AnaFADC::Smooth1(int T)
{
   if (T == 0)
      return;
   int Dt = T / 2;
   float signal_temp[TRACELENGTH];

   for (int i = 0; i <= Dt; i++) {
      signal_temp[i] = signal[i];
   }

   for (int i = traceLength - Dt; i < traceLength; i++) {
      signal_temp[i] = signal[i];
   }

   for (int i = (Dt + 1); i < (traceLength - Dt); i++) {
      signal_temp[i] = (signal[i - Dt] + 2 * signal[i] + signal[i + Dt]) / 4;
   }

   for (int i = 0; i < traceLength; i++) {
      signal[i] = signal_temp[i];
   }
}



//
// GetPeakparam - analyzes information from flash ADC no. FADCn, finds valid peaks, calculates peak parameters and put them in block /PEAKDATA/. Returns  Npeak = number of found valid peaks.
//
int AnaFADC::GetPeakparam(int Iteration)
{
   const float MinDuration = 1.8;  // minimal peak duration
   const float Calib = 1;  // energy calibration
   int Start, Ende,  // start and end channels
       Range,  // peak integration range
       Pnumber,  // peak's number
       Npeak;  // number of found valid peaks

   float Pamp,  // peak amplitude
         PDuration,  // peak duration (Ende - Start), uS
         Bl, Br,  // ordinatas of slope points
         Al, Ar,  // slope parameters
         Penergy;  // pulse energy

   bool Pvalid;  // flag for valid peaks

   Pvalid = false;
   for (int i = 0; i < MAXPEAKS; i++)
      Valid[i] = false;
   Derivatives();
   Npeak = 0;
   Pnumber = 0;
   Range = -1;

   int i = 1;
   int j = 0;
   while ((i <= traceLength) && (Pnumber <= MAXPEAKS)) {
      if (signal[i] > Pedestal[currentAnodeNumber][i]) {
         Start = i;
         Pamp = signal[i];
         while ((i < traceLength) && (signal[i] > Pedestal[currentAnodeNumber][i])) {
            if (signal[i] > Pamp) {
               Pamp = signal[i];
               j = i;
            }
            i = i + 1;
         }
         Pamp = Pamp - Pedestal[currentAnodeNumber][j];
         Ende = i - 1;
         PDuration = float(Ende - Start + 1) * BINWIDTH;
         Pvalid = (PDuration > MinDuration);
         if (PDuration == 32) { // TODO: check
            Pvalid = false;
         }
         if (Pvalid) {
            if (!Search_Xl_Xr(Start, Ende)) {
               D(printf("Error in Search_Xl_Xr\n"));
            }
         }
         // find intersection points
         if (Pvalid) {
            Bl = signal[Xl];
            Br = signal[Xr];
            Al = signal_d1[Xl];
            Ar = signal_d1[Xr];
            if (!Calc_Il_Ir()) {
               //printf("Error in Calc_Il_Ir\n");
            }
         }
         // find pulse energy and compare with threshould. This comparison should be made only for anodes, and only after pedestal correction,  for final step (iteration) of peak analysis
         if (Pvalid) {
            //printf("TODO: Calc_Psquare \n");
            Calc_Psquare();
            //printf("GetPeakparam: Psquare: %f \n", Psquare);
            Penergy = (Psquare * Calib) / 1000; // energy in MeV, without 50keV subtraction
            //printf("GetPeakparam: Penergy: %f \n", Penergy);
            Pvalid = (Penergy > 0);
            if (Pvalid) {  // valid peak is found
               Npeak = Npeak + 1;
               //printf("GetPeakparam: Npeak: %f \n", Npeak);
               Pnumber = Pnumber + 1;
               Valid[Pnumber - 1] = Pvalid;
               VLeft[Pnumber - 1] = Left_flag;
               VRight[Pnumber - 1] = Right_flag;
               P_Duration[Pnumber - 1] = Ir - Il + 1;
               x_l[Pnumber - 1] = Xl;
               x_r[Pnumber - 1] = Xr;
               I_l[Pnumber - 1] = Il;
               I_r[Pnumber - 1] = Ir;
               I_r_i[Pnumber - 1] = Iri;
               I_l_f[Pnumber - 1] = Ilf;
               I_r_f[Pnumber - 1] = Irf;
               A_l[Pnumber - 1] = Al;
               A_r[Pnumber - 1] = Ar;
               B_l[Pnumber - 1] = Bl;
               B_r[Pnumber - 1] = Br;
               P_Energy[Pnumber - 1] = Penergy;
               P_Square[Pnumber - 1] = Psquare;
               P_Amplitude[Pnumber - 1] = Pamp;
               Int_Range[Pnumber - 1] = Range;
               //~ printf("GetPeakparam: %d [%d-%d]\n", Pnumber - 1, Il, Ir);
            }
         }
      } else {
         i = i + 1;
      }
   }
   return Npeak;
}



//
// Numerical calculation of 1st derivative of the FADC signal
//
void AnaFADC::Derivatives()
{
   int Npt = 3;  // interval
   float Coeffa, Sigma_i, Step, fNpt;

   Coeffa = 3 / float(Npt * (Npt + 1) * (2 * Npt + 1));
   //  calculate first derivative
   for (int i = Npt; i < traceLength - Npt; i++) {
      Sigma_i = 0;
      for (int j = i - Npt; j <= i + Npt; j++) {
         Sigma_i = Sigma_i + (j - i) * signal[j];
      }
      signal_d1[i] = Coeffa * Sigma_i;
   }
   //  the numerical calculation of derivative starts from Npt+1 channel (and ends at 800-Npt channel). For these start (end) channels I make the interpolation from 0.
   Step = signal_d1[Npt] / Npt;
   for (int i = 0; i < Npt; i++) {
      signal_d1[i] = Step * i;
   }
   Step = signal_d1[traceLength - Npt] / Npt;
   for (int i = traceLength - Npt; i < traceLength; i++) {
      signal_d1[i] = Step * (traceLength - (i + 1));
   }

}



//
// Calculates left and right points Xl, Xr (in between Start and Ende points) where the first derivative has extremal values. Returns true on success.
//
bool AnaFADC::Search_Xl_Xr(int Start, int Ende)
{
   int g1x_max, g1x_min;  // auxilliary values
   float g1y_max, g1y_min;
   bool result = false;

   Xl = 0;
   Xr = 0;
   g1x_min = 0;
   g1x_max = 0;
   g1y_min = 0;
   g1y_max = 0;
   //printf("Search_Xl_Xr: Start: %d, Ende: %d\n", Start, Ende);
   for (int i = Start; i <= Ende; i++) {
      if (signal_d1[i] > g1y_max) {
         g1y_max = signal_d1[i];
         g1x_max = i;
      }
      if (signal_d1[i] < g1y_min) {
         g1y_min = signal_d1[i];
         g1x_min = i;
      }
   }
   result = true;
   Xl = g1x_max;
   Xr = g1x_min;
   return result;
}



//
// Returns intersection points Il and Ir calculated from slope points Xl and Xr. Returns true on success.
//
bool AnaFADC::Calc_Il_Ir()
{
   float b, Ilh, Irh, d;  // auxilliary values
   bool result = true;
   Il = 0;
   Ir = 0;
   Iri = 0;
   Ilh = 0;
   Irh = traceLength - 1;
   Left_flag = false;
   Right_flag = false;

   // calculate left intersection point
   if ((Xl < 0) || (Xl >= traceLength)) {
      D(printf("Calc_Il_Ir: Xl=%d\n", Xl));
      result = false;
   }
   b = signal[Xl] - Xl * signal_d1[Xl];
   if (signal_d1[Xl] != 0)
      Ilh = (Pedestal[currentAnodeNumber][Xl] - b) / signal_d1[Xl];
   if (Ilh >= traceLength) {
      D(printf("Calc_Il_Ir: Ilh=%f\n", Ilh));
      result = false;
      Ilh = traceLength - 1;
   }
   if (Ilh < 0) {  // peak at the very beginning
      Il = 0;
		Ilf = 0;
      Left_flag = true;
   } else {
      Il = int(Ilh);
      Ilf = Ilh;
      // check here did we realy find the correct point
      d = signal_d1[Xl] * (Il + 1) + b  - Pedestal[currentAnodeNumber][Il + 1];
      while ((d <= 0) && (Il < traceLength - 1)) {
         // pedestal is higher - so one has to move right
         Il = Il + 1;
         Ilf = Ilf + 1;
         d = signal_d1[Xl] * (Il + 1) + b  - Pedestal[currentAnodeNumber][Il + 1];
      }
   }

   // calculate right intersection point
   if ((Xr < 0) || (Xr >= traceLength)) {
      D(printf("Calc_Il_Ir: Xr=%d\n", Xr));
      result = false;
   }
   b = signal[Xr] - Xr * signal_d1[Xr];
   if (signal_d1[Xr] != 0)
      Irh = (Pedestal[currentAnodeNumber][Xr] - b) / signal_d1[Xr];
   if (Irh < 1) {
      D(printf("Calc_Il_Ir: Irh=%f\n", Irh));
      result = false;
      Irh = 1;
   }
   if (Irh >= traceLength) {  // peak at the very end
      Ir = traceLength - 1;
		Irf = traceLength - 1;
      Right_flag = true;
   } else {
      Ir = int(Irh);
      Irf = Irh;
      //printf("Irh = %f, Ir = %d\n", Irh, Ir);
      // check here did we realy find the correct point
      d = signal_d1[Xr] * (Ir - 1) + b  - Pedestal[currentAnodeNumber][Ir - 1];
      while ((d <= 0) && (Ir > 1)) {
         // pedestal is higher - so one has to move left
         Ir = Ir - 1;
         Irf = Irf - 1;
         d = signal_d1[Xr] * (Ir - 1) + b  - Pedestal[currentAnodeNumber][Ir - 1];
      }
   }
   if (Ir <= Il) {
      //printf("Calc_Il_Ir: Il=%d >= Ir=%d\n", Il, Ir);
      result = false;
   }

   // Determine right signal intersection point with pedestal before electronic ripple drop
   int iri = Ir;
   //printf("T_End_i = %d\n", iri);
   while ((signal[iri] > Pedestal[currentAnodeNumber][iri]) && (iri < traceLength))
      iri++;
   Iri = iri;
   //printf("T_End_i = %d\n", iri);

	//printf("Il = %d / %f, Ir = %d / %f, Iri = %d\n", Il, Ilf, Ir, Irf, Iri);

   return result;
}



//
// Calculates the square of the peak with intersection points Il, Ir. Returns true on success.
//
bool AnaFADC::Calc_Psquare()
{
   int Left, Right;  // integration range boundaries
   bool result = true;

   Psquare = 0;
   Left = Il - EXT_LEFT;
   if (RIGHTINTERSECTION == 1) {
      Right = Iri;
   } else {
      Right = Ir + EXT_RIGHT;
   }
   if (Right < Left)
      result = false;
   if ((Left > traceLength) || (Right < 1))
      result = false;
   if (Left < 1)
      Left = 1;
   if (Right > traceLength)
      Right = traceLength;
   for (int i = Left; i < Right; i++) {
      Psquare = Psquare + signal[i] - Pedestal[currentAnodeNumber][i];
   }
   Range = Right - Left + 1;
   //printf("Calc_Psquare: square=%f \n", Psquare);

   return result;
}



//
// Search for the peak with max energy
//
// TODO: Check if max amplitude search is really more accurate.
//
// int AnaFADC::Search_Pulse(int Npeak)
// {
//    int Pnumber = 0;
//    float MaxEnergy = 0;
//
//    for (int i = 0; i < Npeak; i++) {
//       if (P_Energy[i] > MaxEnergy) {
//          Pnumber = i;
//          MaxEnergy = P_Energy[i];
//       }
//    }
//    return Pnumber;
// }
int AnaFADC::Search_Pulse(int Npeak)
{
	int Pnumber = 0;
	float MaxAmp = 0;

	for (int i = 0; i < Npeak; i++) {
		if (P_Amplitude[i] > MaxAmp) {
			Pnumber = i;
			MaxAmp = P_Amplitude[i];
		}
	}
	return Pnumber;
}



//
// Calculate risetime (falltime) and FWHM for peak no. Pnumber
//
void AnaFADC::Risetime(int Pnumber)
{

   float A,          // peak square
         B,           // peak duration
         C,           // trapezium height
         Radikant;

   float PedMax,           // max pedestal value
         PedAve,            // average pedestal value
         Al, Ar,            // derivatives
         Bl, Br,
         Dsquare,
         x, hh;

   int i,            // do loop index
       xl, xr,        // slope points
       Il, Ir,        // intersection points
       Iln, Irn;         // auxilliary intersection points

   if (VLeft[Pnumber] || VRight[Pnumber]) {
      P_FWHM[Pnumber] = P_Square[Pnumber] / P_Amplitude[Pnumber];
      P_FWHM[Pnumber] = P_FWHM[Pnumber] * TIME_INTERVAL;
      //printf("Risetime: Vleft or right, return\n");
      return;
   }
   xl = x_l[Pnumber];
   xr = x_r[Pnumber];
   //printf("Risetime: Xl: %d, Xr: %d\n", xl, xr);
   Al = signal_d1[xl];
   Ar = signal_d1[xr];
   Il = I_l[Pnumber];
   Ir = I_r[Pnumber];

   if (Il < 0) {
      D(printf("Risetime: Il=%d, setting to 0\n", Il));
      Il = 0;
   }

   if (Ir > traceLength - 1) {
      D(printf("Risetime: Ir=%d, setting to %d\n", Ir, traceLength - 1));
      Ir = traceLength - 1;
   }

   // First find the maximum and average pedestal value
   PedAve = 0;
   PedMax = 0;
   if (Il > Ir) {
      //D(printf("Il=%d > Ir=%d, setting to %d\n", Il, Ir, Ir));
      // TODO: check
      Il = Ir;
   }
   for (int i = Il; i <= Ir; i++) {
      PedAve = PedAve + Pedestal[currentAnodeNumber][i];
      if (Pedestal[currentAnodeNumber][i] >= PedMax) PedMax = Pedestal[currentAnodeNumber][i];
   }
   PedAve = PedAve / (Ir - Il + 1);

   // Now find Iln and Irn - intersection points with constant pedestal=PedMax
   Bl  = B_l[Pnumber] - Al * xl;
   hh  = (PedMax - Bl) / Al;
   if (hh < Il) hh = Il;
   Iln = int(hh);
   Br  = B_r[Pnumber] - Ar * xr;
   hh  = (PedMax - Br) / Ar;
   if (hh > Ir) hh = Ir;
   Irn = int(hh);

   // calculate the square between pedestal and PedMax
   Dsquare = 0;
   if (Il > Iln) {
      //D(printf("Risetime: Il=%d > Iln=%d\n", Il, Iln));
      // TODO: check
      return;
   }
   if (Iln > traceLength - 1) {
      Iln = traceLength - 1;
   }
   for (int i = Il; i <= Iln; i++) {
      x = float(i);
      Dsquare = Dsquare + (Al * x + Bl) - Pedestal[currentAnodeNumber][i];
   }
   if ((Iln + 1) > (Irn - 1)) {
      //D(printf("Risetime: Iln+1=%d > Irn-1=%d\n", Iln + 1, Irn - 1));
      // TODO: check
      return;
   }
   if (((Iln + 1) > traceLength) || ((Irn - 1) > traceLength)) {
      D(printf("Risetime: Iln+1=%d/Irn-1=%d > %d\n", Iln + 1, Irn - 1, traceLength));
      return;
   }
   for (int i = Iln + 1; i <= Irn - 1; i++) {
      Dsquare = Dsquare + PedMax - Pedestal[currentAnodeNumber][i];
   }
   if (Irn > Ir) {
      //D(printf("Risetime: Irn=%d > Ir=%d\n", Irn, Ir));
      // TODO: check
      return;
   }
   for (int i = Irn; i <= Ir; i++) {
      x = float(i);
      Dsquare = Dsquare + (Ar * x + Br) - Pedestal[currentAnodeNumber][i];
   }

   // Now determine whether the Pulse can be described by a trapezium or a triangle
   B = Irn - Iln + 1;               // duration
   A = P_Square[Pnumber] - Dsquare;  // square
   Radikant = B * B - 2.0 * A * (1.0 / Al - 1.0 / Ar);

   // If Radikant > 0 then the pulse can be described by a trapezium
   if (Radikant > 0)  {
      // Calculate the distance of a parallel line to the pedestal PedMax to get a trapezium with the square = A
      C = (B - sqrt(Radikant)) / (1.0 / Al - 1.0 / Ar);
      // Now the intersectionpoints of the line with the left and right line of the trapezium must be calculated
      I_2[Pnumber] = (C + PedMax - Bl) / Al;
      I_3[Pnumber] = (C + PedMax - Br) / Ar;
   } else {  // Radikant <= 0
      // If Radikant <= 0 then the pulse can be described by a triangle
      // Now the intersectionpoint of the left and right line of the triangle must be calculated
      I_2[Pnumber] = (Br - Bl) / (Al - Ar);
      I_3[Pnumber] = I_2[Pnumber];
      C = signal[int(I_2[Pnumber])] - PedMax;
   }

//  Now determine the FWHM of the signal via it's square
   C = C + PedMax - PedAve;            /// height of the pulse
   P_FWHM[Pnumber] = (P_Square[Pnumber] / C);
   P_FWHM[Pnumber] = P_FWHM[Pnumber] * TIME_INTERVAL;
}
