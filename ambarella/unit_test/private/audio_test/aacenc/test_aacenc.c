/****************************************************************************
 *
 *  (C) copyright Coding Technologies (2005)
 *  All Rights Reserved
 *
 *  This software module was developed by Coding Technologies (CT). This is
 *  company confidential information and the property of CT, and can not be
 *  reproduced or disclosed in any form without written authorization of CT.
 *
 *  Those intending to use this software module for other purposes are advised
 *  that this infringe existing or pending patents. CT has no liability for
 *  use of this software module or derivatives thereof in any implementation.
 *  Copyright is not released for any means.  CT retains full right to use the
 *  code for its own purpose, assign or sell the code to a third party and to
 *  inhibit any user or third party from using the code. This copyright notice
 *  must be included in all copies or derivative works.
 *
 *  test_aacenc.c, v 1.4 2007/01/29 08:15:14
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include "aac_audio_enc.h"

s32 inputBuffer[1024*2];
u8  outputBuffer[(6144/8)*2+100];

static char inputFilename [256] = "input.pcm";
static char outputFilename [256] = "output.aac";
u8 mpTmpEncBuf[106000];

au_aacenc_config_t au_aacenc_config;

static const char *short_options = "hm:r:R:b:c:C:q:";
static struct option long_options[] = {
	{"help", 0 , 0 , 'h'} ,
	{"mode", 0 , 0 , 'm'} ,
	{"sfreq", 0 , 0 , 'r'} ,
	{"coresfreq", 0 , 0 , 'R'} ,
	{"bitrate", 1 , 0 , 'b'} ,
	{"srcch", 1 , 0 , 'c'} ,
	{"outch", 1 , 0 , 'C'} ,
	{"quality", 1 , 0 , 'q'} ,
	{0, 0 , 0 , 0}
};

static void usage(void)
{
	printf("\nusage: test_aacenc [OPTION]...[FILE]...\n");
	printf("\t-h, --help      help\n"
		   "\t-m, --mode=#    encode mode\n"
		"\t-r, --sfreq=#      sample frequency\n"
		"\t-R, --coresfreq=#  core sample frequency\n"
		"\t-b, --bitrate=#    bit rate\n"
		"\t-c, --srcch=#      channel number of source\n"
		"\t-C, --outch=#      channel number of out\n"
		"\t-q, --quality=#    quantizer quality\n");
	printf("Default: AAC mode, 48000 sample frequency and core sample frequency,\n"
		"\t128000 bit rate, 2 source and out channels, and quantizer quality is 2\n");
	printf("Please use raw PCM file as input, because this tool takes RAW input \n"
		"\t If you have a wav file, you may use CoolEdit to save it as RAW PCM \n");
}

int main( int argc, char **argv)
{
	FILE *fIn = NULL;
	FILE *fOut = NULL;
	int tmp, option_index, ch;
	int enc_time = 0, min_time = 0xffff, max_time = 0;
	struct timeval tm_begin, tm_end;

	au_aacenc_config.enc_mode		= 0;	  // 0: AAC; 1: AAC_PLUS; 3: AACPLUS_PS;
	au_aacenc_config.sample_freq		= 48000;
	au_aacenc_config.coreSampleRate	= 48000;
	au_aacenc_config.Src_numCh		= 2;
	au_aacenc_config.Out_numCh		= 2;
	au_aacenc_config.tns				= 1;
	au_aacenc_config.ffType			= 't';
	au_aacenc_config.bitRate			= 128000; // AAC: 128000; AAC_PLUS: 64000; AACPLUS_PS: 40000;
	au_aacenc_config.quantizerQuality	= 2;
	au_aacenc_config.codec_lib_mem_adr = (u32*)mpTmpEncBuf;

	while ((ch = getopt_long(argc, argv, short_options,  long_options, &option_index)) != -1) {
		switch (ch) {
		case 'h':
			usage();
			return 0;
		case 'm':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.enc_mode = tmp;
			break;
		case 'r':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.sample_freq = tmp;
			break;
		case 'R':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.coreSampleRate = tmp;
			break;
		case 'b':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.bitRate = tmp;
			break;
		case 'c':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.Src_numCh = tmp;
			break;
		case 'C':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.Out_numCh = tmp;
			break;
		case 'q':
			tmp = strtol(optarg, NULL, 0);
			au_aacenc_config.quantizerQuality = tmp;
			break;
		default:
			printf("Try `--help' for more information.\n");
			return -1;
		}
	}

	if (optind <= argc -1)
		strcpy(inputFilename, argv[optind]);

	fIn = fopen (inputFilename, "rb" );
	if (!fIn) {
		fprintf(stderr, "could not open %s for reading\n", inputFilename);
		exit(10);
	}

	fOut = fopen(outputFilename, "wb");
	if (!fOut) {
		fprintf(stderr, "\ncould not open %s for writing\n", outputFilename);
		exit(11);
	}

	printf("\tmode(%d), sfreq(%d), coresfreq(%d), bitrate(%d), \n"
		   "\tsrcch(%d), outch(%d), quality(%d)\n",
		   au_aacenc_config.enc_mode,
		   au_aacenc_config.sample_freq,
		   au_aacenc_config.coreSampleRate,
		   au_aacenc_config.bitRate,
		   au_aacenc_config.Src_numCh,
		   au_aacenc_config.Out_numCh,
		   au_aacenc_config.quantizerQuality);

	aacenc_setup(&au_aacenc_config);
	if (au_aacenc_config.ErrorStatus !=ENCODE_OK) {
		printf("aacenc_setup:ErrorStatus :%d \n", au_aacenc_config.ErrorStatus);
		fclose(fIn);
		fclose(fOut);
		return -1;
	}
	aacenc_open(&au_aacenc_config);

	while (0 != fread(inputBuffer, sizeof(short), 1024 * au_aacenc_config.Src_numCh * au_aacenc_config.sample_freq / au_aacenc_config.coreSampleRate, fIn) )
	{
		au_aacenc_config.enc_rptr = inputBuffer;
		au_aacenc_config.enc_wptr = outputBuffer;

		gettimeofday(&tm_begin, NULL);
		aacenc_encode(&au_aacenc_config);
		gettimeofday(&tm_end, NULL);

		tmp = (tm_end.tv_sec - tm_begin.tv_sec) * 1000000 +
			(tm_end.tv_usec - tm_begin.tv_usec);
		if (tmp != 0) {
			min_time = min_time < tmp ? min_time : tmp;
			max_time = max_time > tmp ? max_time : tmp;
			enc_time = (enc_time + tmp) / 2;
		}

		/* and write to the output file */
		fwrite(au_aacenc_config.enc_wptr, sizeof(char), (au_aacenc_config.nBitsInRawDataBlock + 7) >> 3,  fOut);

		au_aacenc_config.frameCounter++;
		fprintf( stderr, "\r[%d][%d ms][%d ms][%d ms]",
			au_aacenc_config.frameCounter,
			enc_time/1000, min_time/1000, max_time/1000);
	}

	fprintf(stderr, ".\n");
	fflush(stderr);

	fclose(fIn);
	fclose(fOut);

	return 0;
}

