/*******************************************************************************
 * am_mp_mic.cpp
 *
 * History:
 *   Mar 19, 2015 - [longli] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"
#include "am_mp_server.h"
#include "am_mp_mic.h"

#define MIC_RECORD_TIME 5 //UNIT second
#define RECORDED_FILE "/tmp/test"

typedef enum {
  MP_MIC_HAND_SHAKE = 0,
  MP_MIC_RECORD_THD,
} mp_mic_stage_t;

//=================Start for Fourier Transform=================================

/*  Loads a 8192 sample long section of a sound data file
    and Fourier Transforms it to produce the power/frequency
    spectrum. Averages over a series of spectra.
    Reads WAV LPCM format data
 */

//#define SWAP(a,b) {tempr=(a) ; (a)=(b) ; (b)=tempr;}

FILE* infile;
FILE* rfile;
//FILE* outfile;

unsigned char ChunkID[5],Format[5],Subchunk1ID[5],Subchunk2ID[5];

long ChunkSize, Subchunk1Size, AudioFormat;
long NumChannels, SampleRate, ByteRate, BlockAlign;
long BitsPerSample, Subchunk2Size, BytesPerSample;

long pairs_per_sec, ifl;

long ilength, offset, block_number, samples_per_block;
long nn, n, npoints, nhalf, nspectra, spectrum_number, ip, ipk;
long harm, apodising, infileok, ok;

double tstart, tstop, block_duration, total_time, t, dt, f, f0, pi2, pi, sn;

double pmean_left, pmean_right, fpeak, ppeak, psum, tnow;
double ppeak_left, ppeak_right, thd_left, thd_right, sum_thd_left, sum_thd_right;
double fcount;

//unsigned char tc;
unsigned char apstring[16];

unsigned char datablock[49152];
double left[8192], right[8192],weight[8192];
double pout_left[4096], pout_right[4096], pone_left[4096], pone_right[4096];
double x[16385];

char infilename[512];
//char pathname[512];
char leafname[128];
//char outfilename[512];
char rfilename[512];
char appdirname[512];

//void read_path_file(void);

long get_start_time(void);
long get_stop_time(void);
void load_data_block(void);
void load_data_block24(void);
//void zero_spectra(void);
void transform(void);

void load_fft_array(double*);
void save_powerspectrum(double*, double*);
//double db(double);
//double db0(double);
void fft(void);
void bit_rev(void);
//void save_output(void);
void analyse_onespectrum(void);

void generate_apodise_weights(void);
void apodise(void);

double find_counter_frequency(void);

double read_header(void);
void get4(unsigned char*);
long get4int(void);
long get2int(void);

//void veusz_output(void);
//void veusz_report(void);
//void veusz_label(char*, char*, double, double);

#ifdef __WIN32__
#define DIR_DELIMITER '\\'
#else
#define DIR_DELIMITER '/'
#endif

int calculate_thd(const char *file_name,
                  double *avg_thd_left,
                  double *avg_thd_right)
{
#if 1
  samples_per_block = 8192;  /* Number of samples per channel per block */
  n = samples_per_block;
  npoints = n;
  nn = 2*n;
  nhalf = n/2;
  pi = 3.1415927;
  pi2 = 2.0*pi;
#endif

  if (file_name) {
    strcpy(infilename, file_name);
  }
#if 1
  apodising = 1; //hard code

  generate_apodise_weights();

  spectrum_number = 0;

  if (apodising != 0)
  {
    //printf("Apodising data\n");
    sprintf((char*)apstring, "Apodised(%ld)  ", apodising);
  }
  else
  {
    printf("No apodising\n");
    strcpy((char*)apstring, "");
  }


  ok = (long)strrchr(infilename, DIR_DELIMITER);

  if (!ok) {
    strcpy(leafname, infilename);
  }
  else {
    strcpy(leafname, (char*)(ok+1));
  }
  printf("Leafname = %s\n", leafname);
  sprintf(rfilename, "%s%cReport.csv", appdirname, DIR_DELIMITER);
  printf("Report file name = %s\n\n", rfilename);
  printf("Now reading WAV header\n");

  total_time = read_header();
  printf("total time = %6.2lf\n", total_time);
  dt = 1.0/(double)pairs_per_sec;
  block_duration = dt*(double)samples_per_block;
  f0 = 1.0/(double)block_duration;

  offset = get_start_time();
  printf("Offset %ld\n", offset);

  nspectra = get_stop_time();

  printf("Will run for %ld spectra %8.4lf sec \n", nspectra,block_duration*(double)nspectra);

  sn = (double)nspectra;
  sn = 1.0/sn;

  rfile = fopen(rfilename, "wt");

  // fprintf(rfile,"\"time\", \"fmax\", \"pleft\", \"pright\", \"pfmaxleft\", \"pfmaxright\", \"n\", \"thdleft\", \"thdright\", \"fcount\",\n");


  infile = fopen(infilename, "rb");
  if (!infile) {
    printf("cannot open file %s \n", infilename);
  }

  fseek(infile, offset+44, SEEK_SET);

  sum_thd_left = 0;
  sum_thd_right = 0;

  do
  {
    if (BytesPerSample == 2)
    {
      load_data_block();
    }

    if (BytesPerSample == 3)
    {
      load_data_block24();
    }

    fcount = find_counter_frequency();
    if (apodising != 0)   apodise();
    transform();
    fpeak = f0*(double)ipk;
    tnow = tstart+block_duration*(double)spectrum_number;
    analyse_onespectrum();

    sum_thd_left += thd_left;
    sum_thd_right += thd_right;
    spectrum_number++;
  } while (spectrum_number < nspectra);

  *avg_thd_left = sum_thd_left / spectrum_number;
  *avg_thd_right = sum_thd_right / spectrum_number;
  fclose(infile);
  fclose(rfile);
#endif

  return 0;
}

double find_counter_frequency(void)
{
  /* This proceedure employs a fringe counting method to
     determine the frequency of the periodic signal.
     It then sets f0 to be the found value for whichever
     channel has the highest power level.
   */
  long last_left,last_right, first_left = 0,first_right = 0;
  long ii,detect_left,detect_right,count_left,count_right;
  double ffound,f0_left,f0_right;
  last_left = -10; last_right = -10;
  count_left = 0; count_right = 0;
  ii = 1;
  do
  {
    detect_left = 0;
    if (left[ii-1] <= 0)
    {
      if (left[ii] > 0)
      {
        detect_left= 1;

        if(last_left < 0)
        {
          first_left = ii;
          last_left = detect_left;
        }
        else
        {
          if ((ii-last_left > 4))
          {
            last_left = ii;
            count_left++;
          }
        }
      }
    }

    detect_right = 0;
    if (right[ii-1] <= 0)
    {
      if (right[ii] > 0)
      {
        detect_right = 1;

        if(last_right < 0)
        {
          first_right = ii;
          last_right = detect_right;
        }
        else
        {
          if ((ii-last_right > 4))
          {
            last_right = ii;
            count_right++;
          }
        }
      }
    }
    ii++;
  } while (ii < 8192);

  /*
    printf("Left  %ld to %ld has %ld cycles\n",first_left,last_left,count_left);
    printf("Right %ld to %ld has %ld cycles\n",first_right,last_right,count_right);
   */

  ffound = (double)(last_left-first_left);

  if(count_left > 0)
  {
    ffound = ffound/(double)(count_left);
    f0_left = (double)pairs_per_sec/ffound;
  }
  else
  {
    f0_left = 0.0;
  }

  ffound = (double)(last_right-first_right);

  if(count_right > 0)
  {
    ffound = ffound/(double)(count_right);
    f0_right = (double)pairs_per_sec/ffound;
  }
  else
  {
    f0_right = 0.0;
  }

  /*
    printf("Frequencies %6.2lf Hz Left  %6.2lf Right\n",f0_left,f0_right);
   */
  if(pmean_left > pmean_right)
  {
    ffound = f0_left;
    //printf("Left chosen\n");
  }
  else
  {
    ffound = f0_right;
    //printf("Right chosen\n");
  }

  return ffound*0.001;
}

void apodise(void)
{
  long iap = 0;
  do
  {
    left[iap] = weight[iap]*left[iap];
    right[iap] = weight[iap]*right[iap];
    iap++;
  } while (iap < 8192);
}

void generate_apodise_weights(void)
{
  long iwf, iwb;
  double weight_now, wscale, xplace;
  iwf=0; iwb=8191; wscale=0.0;

  if (apodising > 2)
  {
    apodising = 0;
  }

  if (apodising == 0)
  {
    printf("No apodisation\n");
  }

  if (apodising == 1)
  {
    printf("BH window\n");

    do
    {
      xplace = (double)iwf;
      xplace = pi2*xplace/8191.0;
      weight_now = 0.35875-0.48829*cos(xplace)+0.14128*cos(2.0*xplace)-0.01168*cos(3.0*xplace);
      weight[iwf] = weight_now;
      weight[iwb] = weight_now;
      wscale += weight_now;
      iwf++;
      iwb--;
    } while (iwf < iwb);

    wscale = wscale/((double)(iwf-1));

    iwf = 0;
    do
    {
      weight[iwf] = weight[iwf]/wscale;
      iwf++;
    } while (iwf < 8192);
  }

  if (apodising == 2)
  {
    printf("triangular window\n");
    do
    {
      weight_now = (double)iwf;
      weight_now = 1.735921*weight_now/4095.0;
      weight[iwf] = weight_now;
      weight[iwb] = weight_now;
      wscale += weight_now;
      iwf++;
      iwb--;
    } while (iwf < iwb);
  }
}

void analyse_onespectrum(void)
{
  long dosp, id, idc, hno;
  harm = 0;
  thd_left = 0.0; thd_right = 0.0;

  dosp = 0;

  if (fpeak > 15.0)
  {
    if (fpeak < 10800.0)
    {
      dosp = 1;
      harm = (long)(21005.0/fpeak);
      if (harm > 10) harm = 10;
    }
  }

  if (dosp == 0)
  {
    ppeak_left = 0.0; ppeak_right = 0.0;
    id = ipk-2;
    if (id < 0) id = 0;
    if (id > 4090) id = 4090;
    idc = 0;
    do
    {
      ppeak_left += pone_left[id];
      ppeak_right += pone_right[id];
      id++;
      idc++;
    } while (idc < 5);
  }

  if (dosp == 1)
  {
    ppeak_left = 0.0; ppeak_right = 0.0;

    id = ipk-2; idc = 0;
    do
    {
      ppeak_left += pone_left[id];
      ppeak_right += pone_right[id];
      id++;
      idc++;
    } while (idc < 5);

    if(ppeak_left > 0.00000001)
    {
      hno = 2;
      do
      {
        id = ipk*hno-2; idc = 0;
        do
        {
          thd_left += pone_left[id];
          id++;
          idc++;
        }while (idc < 5);
        hno++;
      } while (hno <= harm);

      if (thd_left > 0.0)
      {
        thd_left = thd_left/ppeak_left;
        thd_left = 100.0*sqrt(thd_left);
      }
      else
      {
        thd_left = 0.0;
      }
    }

    if(ppeak_right > 0.00000001)
    {
      hno = 2;
      do
      {
        id = ipk*hno-2; idc = 0;
        do
        {
          thd_right += pone_right[id];
          id++;
          idc++;
        }while (idc<5);
        hno++;
      } while (hno <= harm);

      if (thd_right > 0.0)
      {
        thd_right = thd_right/ppeak_right;
        thd_right = 100.0*sqrt(thd_right);
      }
      else
      {
        thd_right = 0.0;
      }
    }
  }
}

#if 0
void zero_spectra(void)
{
  long iz;
  iz = 0;
  do
  {
    pout_left[iz] = 0.0;
    pout_right[iz] = 0.0;
    iz++;
  } while (iz < 4096);
}
#endif

void transform(void)
{
  ppeak = 0.0;
  fpeak = 0.0;

  load_fft_array(left);
  fft();
  save_powerspectrum(pout_left, pone_left);
  load_fft_array(right);
  fft();
  save_powerspectrum(pout_right, pone_right);
}

void load_fft_array(double* inarray)
{
  long icount, icd;
  icount = 0; icd = 1;
  x[0] = 0.0;
  do
  {
    x[icd] = inarray[icount];
    icd++;
    x[icd] = 0.0;
    icd++;
    icount++;
  } while (icount < npoints);
}

void save_powerspectrum(double* outarray, double* onearray)
{
  long icount, idc;
  double p1, p2;

  icount = 0; idc = 1;
  do
  {
    p1 = x[idc];
    idc++;
    p2 = x[idc];
    idc++;
    p1 = p1*p1+p2*p2;

    if (p1 > ppeak)
    {
      ppeak = p1;
      ipk = icount;
    }

    outarray[icount] = outarray[icount]+p1;
    onearray[icount] = p1;
    icount++;

  } while (icount < nhalf);
}
#if 0
double db(double pin)
{
  double answer;
  if (pin < 0.000000001)
  {
    pin = 0.000000001;
  }
  pin = sn*pin;
  answer = 10.0*log10(pin)-72.25;
  return answer;
}

double db0(double pin)
{
  double answer;
  if (pin < 0.000000001)
  {
    pin = 0.000000001;
  }
  answer = 10.0*log10(pin);
  return answer;
}
#endif
void fft(void)
{
  long mmax, istep, m, i, j;
  long ibig, ismall, jbig, jsmall;
  double theta, wpi, wpr;
  double wi, wr, tr, ti, wtemp;

  ibig = -99;  jbig = -99;
  ismall = 99; jsmall = 99;

  mmax = 2;
  theta = 2.0*pi/mmax;

  bit_rev();

  while (nn > mmax)
  {
    istep = 2*mmax;
    theta = 2.0*pi/mmax;
    wpr = -2.0*sin(0.5*theta)*sin(0.5*theta);
    wpi = sin(theta);
    wr = 1.0;
    wi = 0.0;
    m = 1;
    do
    {
      i = m;
      do
      {
        j = i+mmax;

        if (i > ibig)
        {
          ibig = i;
        }
        if (i < ismall)
        {
          ismall = i;
        }
        if (j > jbig)
        {
          jbig = j;
        }
        if (j < jsmall)
        {
          jsmall = j;
        }

        tr = wr*x[j]-wi*x[j+1];
        ti = wr*x[j+1]+wi*x[j];
        x[j] = x[i]-tr;
        x[j+1] = x[i+1]-ti;
        x[i] = x[i]+tr;
        x[i+1] = x[i+1]+ti;
        i += istep;
      } while (i < nn);
      wtemp = wr;
      wr = wr*wpr-wi*wpi+wr;
      wi = wi*wpr+wtemp*wpi+wi;
      m += 2;
    } while (m < mmax);
    mmax = istep;
  }
}

void bit_rev(void)
{
  long ibr, jbr, mbr;
  double tr, ti;
  jbr = 1;
  ibr = 1;
  do
  {
    if (jbr > ibr)
    {
      tr = x[jbr];
      ti = x[jbr+1];
      x[jbr] = x[ibr];
      x[jbr+1] = x[ibr+1];
      x[ibr] = tr;
      x[ibr+1] = ti;
    }
    mbr = nn/2;
    while (mbr >= 2 && jbr > mbr)
    {
      jbr -= mbr;
      mbr = mbr/2;
    }
    jbr = jbr+mbr;
    ibr += 2;
  } while (ibr < nn);
}

void load_data_block(void)
{
  /* This procedure finds and loads the selected block of sample
     data and then converts the raw data into a set of real sample
     values for analysis. */

  long icut, icut2;
  long io, ii, lt, rt;
  double samscale;
  double dc_left, dc_right, rms_left, rms_right;

  icut = 32768; icut2 = icut*2;
  samscale = 1.0/(double)icut;
  dc_left = 0.0; dc_right = 0.0;
  rms_left = 0.0; rms_right = 0.0;
  ii = 0; io = 0;

  fread(datablock,sizeof(unsigned char),32768,infile);


  do
  {
    lt = 256*(long)datablock[ii+1] + (long)datablock[ii];
    ii += 2;
    if (NumChannels == 2)
    {
      rt = 256*(long)datablock[ii+1] + (long)datablock[ii];
      ii += 2;
    }
    else
    {
      rt = lt;
    }
    if (lt >= icut) lt -= icut2;
    if (rt >= icut) rt -= icut2;
    left[io] = samscale*(double)lt;
    right[io] = samscale*(double)rt;
    rms_right+=right[io]*right[io];
    rms_left += left[io]*left[io];

    io++;

  } while (io < 8192);

  dc_right = dc_right/8192.0;
  dc_left = dc_left/8192.0;
  rms_left = rms_left/8192.0;
  rms_right = rms_right/8192.0;

  pmean_left = rms_left;
  pmean_right = rms_right;

  rms_left = sqrt(rms_left);
  rms_right = sqrt(rms_right);

  //printf("\n\nSpectrum %ld of %ld\n", spectrum_number+1, nspectra);
}

void load_data_block24(void)
{
  /* This procedure finds and loads the selected block of sample
     data and then converts the raw data into a set of real sample
     values for analysis. */

  //long block_size_read;
  long icut, icut2;
  long io, ii, lt, rt;
  double samscale;
  double dc_left, dc_right, rms_left, rms_right;

  icut = 32768*256; icut2 = icut*2;
  samscale = 1.0/(double)icut;
  dc_left = 0.0; dc_right = 0.0;
  rms_left = 0.0; rms_right = 0.0;
  //block_size_read=fread(datablock,sizeof(unsigned char),49152,infile);
  ii = 0; io = 0;

  do
  {
    lt = 65536*(long)datablock[ii+2] + 256*(long)datablock[ii+1] + (long)datablock[ii];
    ii += 3;
    if(NumChannels == 2)
    {
      rt = 65536*(long)datablock[ii+2] + 256*(long)datablock[ii+1] + (long)datablock[ii];
      ii += 3;
    }
    else
    {
      rt = lt;
    }
    if (lt >= icut) lt -= icut2;
    if (rt >= icut) rt -= icut2;
    left[io] = samscale*(double)lt;
    right[io] = samscale*(double)rt;
    rms_right += right[io]*right[io];
    rms_left += left[io]*left[io];

    io++;
  } while (io < 8192);

  dc_right = dc_right/8192.0;
  dc_left = dc_left/8192.0;
  rms_left = rms_left/8192.0;
  rms_right = rms_right/8192.0;

  pmean_left = rms_left;
  pmean_right = rms_right;

  rms_left = sqrt(rms_left);
  rms_right = sqrt(rms_right);

  //printf("\n\nSpectrum %ld of %ld\n", spectrum_number+1, nspectra);
}

long get_stop_time(void)
{
  /* This procedure asks the user what time (in seconds from the
     start of the file) is the end of the chunk to be loaded
     and analysed. */

  long ir, ir4;
  double irtemp;
  ir = 0;
  //printf("Enter stop time of section to transform [seconds]\n   => ");

  //scanf("%lf",&tstop);
  tstop = total_time - 0.25;

  printf("\nRequested stop time %7.2lf\n", tstop);

  if (tstop > (total_time-0.25))
  {
    printf("\nTime requested goes beyond end of file!\n");

    tstop = total_time-0.25;

    printf("Reduced to %6.2lf sec!\n", tstop);
  }

  if (tstop < 0.0)
  {
    ir = -2;
    printf("\nTime requested before file starts!\n");
  }

  if (ir == 0)
  {
    irtemp = (double)pairs_per_sec*tstop;

    ir4 = (long)(4.0*irtemp);

    ir = 4*(long)irtemp;

    printf("Nominal end sample %8.2lf  %ld  %ld\n", irtemp, ir4, ir);
  }
  ir = ir-offset;
  ir = ir/(4*8192);
  return ir;
}

long get_start_time(void)
{
  /* This procedure asks the user what time (in seconds from the
     start of the file) is the beginning of the chunk to be loaded
     and analysed. */

  long ir, ir4;
  double irtemp;
  ir = 0;
  tstart = 0.0;

  printf("\nRequested start time %7.2lf\n", tstart);

  if (tstart > (total_time-0.05))
  {
    ir = -1;
    printf("\nTime requested goes beyond end of file!\n");
  }

  if (tstart < 0.0)
  {
    ir = -2;
    printf("\nTime requested before file starts!\n");
  }

  if (ir == 0)
  {
    irtemp = (double)pairs_per_sec*tstart;
    ir4 = (long)(4.0*irtemp);
    ir = 4*(long)irtemp;

    printf("Start sample %8.2lf  %ld  %ld\n", irtemp, ir4, ir);
  }
  return ir;
}

double read_header(void)
{
  /* This procedure reads the header if it is a WAV file */

  infile = fopen(infilename, "rb");
  if (!infile) {
    printf("cannot open file %s \n", infilename);
    exit(1);
  }

  double result;
  get4(ChunkID);
  printf("ChunkID = %s\n", ChunkID);
  ChunkSize = get4int();
  printf("Chunk Size = %ld\n", ChunkSize);

  get4(Format);
  printf("Format = %s\n\n", Format);
  get4(Subchunk1ID);
  printf("Subchunk1ID = %s\n", Subchunk1ID);
  Subchunk1Size = get4int();
  printf("Subchunk1Size = %ld [should be 16 for LPCM]\n", Subchunk1Size);

  AudioFormat = get2int();
  printf("AudioFormat = %ld [should be 1 for LPCM]\n",AudioFormat);

  NumChannels = get2int();
  printf("Number of Channels = %ld\n", NumChannels);
  SampleRate = get4int();
  printf("Sample Rate = %ld Samples per sec\n", SampleRate);
  ByteRate = get4int();
  printf("Byte Rate = %ld Bytes per sec\n", ByteRate);

  BlockAlign = get2int();
  printf("Block Align = %ld\n\n", BlockAlign);
  BitsPerSample = get2int();
  printf("Bits per sample = %ld\n", BitsPerSample);
  BytesPerSample = BitsPerSample/8;
  printf("Bytes per sample = %ld\n\n", BytesPerSample);

  get4(Subchunk2ID);
  printf("Subchunk2ID = %s\n", Subchunk2ID);
  Subchunk2Size = get4int();
  printf("Subchunk2Size = %ld [Number of Data payload bytes]\n\n", Subchunk2Size);

  result = 0;

  if(Subchunk2ID[0] == 'd') result = 1;
  if(Subchunk2ID[1] == 'a') result += 1;
  if(Subchunk2ID[2] == 't') result += 1;
  if(Subchunk2ID[3] == 'a') result += 1;

  if(result == 4)
  {
    printf("WAV data start located OK\n");

    result = (double)Subchunk2Size/(double)ByteRate;
    pairs_per_sec=SampleRate;

    if(NumChannels == 1)
    {
      printf("MONO Samples per sec = %ld\n", pairs_per_sec);
    }
    else
    {
      printf("Sample pairs per sec = %ld\n", pairs_per_sec);
    }

    printf("Duration = %6.4lf sec\n\n", result);
  }
  else
  {
    printf("Not identified as WAV data!\n");
    result = 0.0;
  }
  fclose(infile);
  infile = NULL;
  return result;
}

long get2int(void)
{
  long result;
  unsigned char cip1, cip2;

  fscanf(infile, "%c", &cip1);
  fscanf(infile, "%c", &cip2);

  result = (long)cip2;
  result = (long)cip1+256*result;

  return result;
}

long get4int(void)
{
  long result;

  unsigned char cip1, cip2, cip3, cip4;

  fscanf(infile, "%c%c%c%c", &cip1, &cip2, &cip3, &cip4);
  printf("cip1=%d\ncip2=%d\ncip3=%d\ncip4=%d\n", cip1, cip2, cip3, cip4);

  result = (long)cip4;
  result = (long)cip3+256*result;
  result = (long)cip2+256*result;
  result = (long)cip1+256*result;
  //fscanf(infile, "%ld", &result);
  return result;
}

void get4(unsigned char* outstring)
{
  long isp;
  unsigned char csp;
  isp = 0;
  do
  {
    fscanf(infile, "%c", &csp);
    outstring[isp] = csp;
    isp++;
  } while (isp < 4);
  outstring[4] = (unsigned char)0;
}

//=================End for Fourier Transform===================================

am_mp_err_t mptool_mic_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  double avg_thd_left, avg_thd_right;
  int32_t status;
  char cmd[BUF_LEN] = {0};

  to_msg->result.ret = MP_OK;
  switch(from_msg->stage) {
    case MP_MIC_HAND_SHAKE:
      break;
    case MP_MIC_RECORD_THD: //start recording audio

      snprintf(cmd, sizeof(cmd) - 1,
               "/usr/bin/arecord -f dat %s &", RECORDED_FILE);

      status = system(cmd);
      if (!WEXITSTATUS(status)) {
        sleep(MIC_RECORD_TIME);

        sprintf(cmd,"killall arecord");
        system(cmd);
        sleep(2);
        calculate_thd(RECORDED_FILE, &avg_thd_left, &avg_thd_right);
        remove(RECORDED_FILE);
        printf("avg_thd_left = %lf, avg_thd_right = %lf\n", avg_thd_left, avg_thd_right);
        if (avg_thd_left > 0.3 || avg_thd_right > 0.3) {
          to_msg->result.ret = MP_ERROR;
          printf("average thd exceeds threshold.");
        } else if (!avg_thd_left && !avg_thd_right) {
          to_msg->result.ret = MP_ERROR;
        }
      } else {
        printf("system call arecord failed!");
        to_msg->result.ret = MP_ERROR;
      }

      if (mptool_save_data(from_msg->result.type, to_msg->result.ret, SYNC_FILE) != MP_OK) {
        printf("save failed.");
        to_msg->result.ret = MP_IO_ERROR;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
