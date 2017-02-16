/****************************************************************************
 *
 * (C) copyright Coding Technologies (2002)
 *  All Rights Reserved
 *
 *  This software module was developed by Coding Technologies (CT). This is
 *  company confidential information and the property of CT, and can not be
 *  reproduced or disclosed in any form without written authorization of CT.
 *
 *  Those intending to use this software module for other purposes are advised
 *  that this infringe existing or pending patents. CT has no liability for
 *  use of this software module or derivatives thereof in any implementation.
 *  Copyright is not released for any means. CT retains full right to use the
 *  code for its own purpose, assign or sell the code to a third party and to
 *  inhibit any user or third party from using the code. This copyright notice
 *  must be included in all copies or derivative works.
 *
 *  test_aacdec.c r1831 2011/08/05 06:28:46
 *
 ****************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "aac_audio_dec.h"


typedef struct {
	s32	StatusNr;
	s8	StatusDescription[128];
} STATUS_MESSAGE;

STATUS_MESSAGE StatusMessage[] = {
	{MAIN_OK, "OK"},
	{MAIN_UCI_OUT_OF_MEMORY, "MAIN_UCI_OUT_OF_MEMORY"},
	{MAIN_UCI_HELP_MODE_ACTIV, "MAIN_UCI_HELP_MODE_ACTIV"},
	{MAIN_OPEN_BITSTREAM_FILE_FAILED, "MAIN_OPEN_BITSTREAM_FILE_FAILED"},
	{MAIN_OPEN_16_BIT_PCM_FILE_FAILED, "MAIN_OPEN_16_BIT_PCM_FILE_FAILED"},
	{MAIN_FRAME_COUNTER_REACHED_STOP_FRAME, "MAIN_FRAME_COUNTER_REACHED_STOP_FRAME"},
	{MAIN_TERMINATED_BY_ESC, "MAIN_TERMINATED_BY_ESC"},

	{AAC_DEC_ADTS_SYNC_ERROR, "AAC_DEC_ADTS_SYNC_ERROR"},
	{AAC_DEC_LOAS_SYNC_ERROR, "AAC_DEC_LOAS_SYNC_ERROR"},
	{AAC_DEC_ADTS_SYNCWORD_ERROR, "AAC_DEC_ADTS_SYNCWORD_ERROR"},
	{AAC_DEC_LOAS_SYNCWORD_ERROR, "AAC_DEC_LOAS_SYNCWORD_ERROR"},
	{AAC_DEC_ADIF_SYNCWORD_ERROR, "AAC_DEC_ADIF_SYNCWORD_ERROR"},
	{AAC_DEC_UNSUPPORTED_FORMAT, "AAC_DEC_UNSUPPORTED_FORMAT"},
	{AAC_DEC_DECODE_FRAME_ERROR, "AAC_DEC_DECODE_FRAME_ERROR"},
	{AAC_DEC_CRC_CHECK_ERROR, "AAC_DEC_CRC_CHECK_ERROR"},
	{AAC_DEC_INVALID_CODE_BOOK, "AAC_DEC_INVALID_CODE_BOOK"},
	{AAC_DEC_UNSUPPORTED_WINOW_SHAPE, "AAC_DEC_UNSUPPORTED_WINOW_SHAPE"},
	{AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC, "AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC"},
	{AAC_DEC_UNIMPLEMENTED_CCE, "AAC_DEC_UNIMPLEMENTED_CCE"},
	{AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA, "AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA"},
	{AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE, "AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE"},
	{AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE, "AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE"},
	{AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE, "AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE"},
	{AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER, "AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER"},
	{AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS, "AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS"},
	{AAC_DEC_NEED_MORE_DATA, "AAC_DEC_NEED_MORE_DATA"}
};

#define INBUF_SIZE	(16384)
/* --- default settings of aacPlus decoder Test Env. --------- */
FILE *pInputFile;									/*!< file pointer to bitstream file */
FILE *pOutputFile = 0;								 /*!< file pointer to 16 bit output file */
s8 inputBsFilename[FILENAME_MAX] = "input.aac";		 /*!< input file */
s8 outputPcmFilename[FILENAME_MAX] = "output.pcm";	/*!< output file */

u16 frameCounter = 0;						/*!< frame counter */
s8 loadInputBufferWithAverageBitrate = 0;		/*!< 0 => fill up input buffer completely
											  1 => fill up input buffer with average bitrate */
u32 share_MEM[25000];

u8 input_Buffer[INBUF_SIZE];
u8 *inBuffer;

/* --- default settings of aacPlus decoder --------- */
au_aacdec_config_t au_aacdec_config;

/*!
  brief Prints status message
  The function prints the status messages of the current decoding process.
  return  none
*/
void PrintStatusMessage(s32 status)
{
	u32 i;

	for (i = 0; i < sizeof(StatusMessage) / sizeof(STATUS_MESSAGE); i++) {
		if (status == StatusMessage[i].StatusNr)
			printf("\nStatus: %s\n", StatusMessage[i].StatusDescription);
	}
}

/*!
  brief Open bitstream file
  The function opens the bitstream file.
  return  error status
*/
s32 AuChannelOpenBitstream(s8 *file)
{
	pInputFile = fopen((const char *)file, "rb");
	if (pInputFile == 0) {
		fprintf(stderr, "\n bitstream file not found\n\n");
		return(MAIN_OPEN_BITSTREAM_FILE_FAILED);
	}

	return (MAIN_OK);
}

/*!
  format convert pcm 32bit to 16bit
  The function convert 32bit to 16bit interleave PCM format.
*/
void fc32ito16i(s32 *bufin, s16 *bufout, s32 ch, s32 proc_size)
{
	s32 i, j;
	s32 *bufin_ptr;
	s16 *bufout_ptr;

	bufin_ptr = bufin;
	bufout_ptr = bufout;
	for (i = 0; i < proc_size; i++) {
		for(j = 0; j < ch; j++) {
			*bufout_ptr = (*bufin_ptr) >> 16;
			bufin_ptr++;
			bufout_ptr++;
		}
	}
}

/*!
  brief aacPlus decoder
  After an initialization and synchronization phase, CAacDecoder_DecodeFrame() and applySBR()
  are called in an endless loop until the complete bitstream has been decoded.
  The main program provides the aac core and the sbr tool with the time data buffer of length
  4 * SAMPLES_PER_FRAME.
  The input buffer can be filled in two different ways. On the one hand, the input buffer can
  be handled that it is always filled completely by the bitstream routine. On the other hand,
  the input buffer can be filled with an average bitrate.
  In both cases the input buffer is full, when the synchronisation routine is called.

  return  error status
*/
s32 main(int argc, char *argv[])
{
	s32 endOfFile = 0;
	s32 ret = 0;
	u8 *bitbuf_rptr;
	s32 pTimeDataPcm[sizeof(s32)*2*SAMPLES_PER_FRAME]; // 32bit per sample * 2ch * 1024 samples per frame
	s16 buff[sizeof(s16)*SAMPLES_PER_FRAME]; // 16bit per sample * 2ch * 1024 samples per frame

	/* Open output one frame later in case of concealment. */
	if (!pOutputFile)
		pOutputFile = fopen((const char *)outputPcmFilename, "w+b");

	/* set unbuffered output */
	setbuf(stdout, 0);

	au_aacdec_config.externalSamplingRate = 48000;	/*!< external sampling rate for raw decoding */
	au_aacdec_config.bsFormat = ADTS_BSFORMAT;	/*!< ADTS_BSFORMAT : default Bitstream Type */
	au_aacdec_config.srcNumCh = 2;				/* set srcNumCh to 1 for mono aac clip */
	au_aacdec_config.outNumCh = 2;
	au_aacdec_config.externalBitrate = 0;			/*!< external bitrate for loading the input buffer */
	au_aacdec_config.frameCounter = 0;
	au_aacdec_config.errorCounter = 0;
	au_aacdec_config.interBufSize = 0;
	au_aacdec_config.codec_lib_mem_addr = share_MEM;
	au_aacdec_config.consumedByte = 0;

	aacdec_setup(&au_aacdec_config);
	aacdec_open(&au_aacdec_config);

	/* open bitstream file */
	if (!au_aacdec_config.ErrorStatus)
		au_aacdec_config.ErrorStatus = AuChannelOpenBitstream(inputBsFilename);

	au_aacdec_config.dec_rptr = input_Buffer;
	bitbuf_rptr = input_Buffer;
	au_aacdec_config.dec_wptr = pTimeDataPcm;

	do {
		if (endOfFile == 0) {
			if ((INBUF_SIZE - au_aacdec_config.interBufSize) >= (INBUF_SIZE >> 1)) {
				ret = fread(bitbuf_rptr + au_aacdec_config.interBufSize, 1, (INBUF_SIZE>>1), pInputFile);
				au_aacdec_config.interBufSize += ret;
			}
			if (ret < (INBUF_SIZE >> 1))
				endOfFile = 1;
		}
		aacdec_set_bitstream_rp(&au_aacdec_config);

		//if (endOfFile)	break;

		aacdec_decode(&au_aacdec_config);

		if (au_aacdec_config.ErrorStatus)
	 		break;

		if (au_aacdec_config.consumedByte > 0) {
			au_aacdec_config.dec_rptr = input_Buffer;
			if (au_aacdec_config.interBufSize >= au_aacdec_config.consumedByte) {
				au_aacdec_config.interBufSize -= au_aacdec_config.consumedByte;
			} else {
				au_aacdec_config.interBufSize = 0;
			}
			memmove(bitbuf_rptr,
				au_aacdec_config.dec_rptr + au_aacdec_config.consumedByte,
				au_aacdec_config.interBufSize);
			au_aacdec_config.consumedByte = 0;
		}
		/* Open output file if not done yet. */
		if (au_aacdec_config.frameCounter > 0) {
			// Format convert PCM from 32bit to 16bit interleave
			fc32ito16i(au_aacdec_config.dec_wptr, &buff[0],
				au_aacdec_config.outNumCh,
				au_aacdec_config.frameSize);

			/* write audio channels to pcm file */
			fwrite(&buff[0], sizeof(s16),
				au_aacdec_config.frameSize*au_aacdec_config.outNumCh,
				pOutputFile);
		}
		fprintf(stderr, "\r[%d]", au_aacdec_config.frameCounter);

		if (au_aacdec_config.interBufSize < 8)
			break;

	}while (!au_aacdec_config.ErrorStatus);

	if (pInputFile)
		fclose(pInputFile);
	if (pOutputFile)
		fclose(pOutputFile);

	return (au_aacdec_config.ErrorStatus);
}



