/*
 * iav_dec_api.h
 *
 * History:
 *	2015/01/21 - [Zhi He] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#ifndef __IAV_DEC_API_H__
#define __IAV_DEC_API_H__

//from dsp
typedef enum {
	HDEC_OPM_IDLE = 0,
	HDEC_OPM_VDEC_RUN,
	HDEC_OPM_VDEC_IDLE, // still have last picture left
	HDEC_OPM_VDEC_2_IDLE,
	HDEC_OPM_VDEC_2_VDEC_IDLE,
	HDEC_OPM_JDEC_SLIDE, // must be the first of JPEG stuff
	HDEC_OPM_JDEC_SLIDE_IDLE,
	HDEC_OPM_JDEC_SLIDE_2_IDLE,
	HDEC_OPM_JDEC_SLIDE_2_SLIDE_IDLE,
	HDEC_OPM_JDEC_MULTI_S,
	HDEC_OPM_JDEC_MULTI_S_2_IDLE,
	HDEC_OPM_RAW_DECODE,
	HDEC_OPM_RAW_DECODE_2_IDLE, // not used yet - jrc 12/01/2005
} HDEC_OPMODE;

typedef enum {
	ERR_NO_ERROR = 0,          //!< No error.
	ERR_NO_4BYTEHEADER,        //!< Failed to search for the 4 byte sequence(0x00000001).
	ERR_NALU_FORBIDDEN_ZERO,   //!< forbidden_zero_bit of NALU header is non-zero.
	ERR_SPS_ID_OUTOFRANGE,     //!< seq_id over supported range, (2 by default, standard allows 0..31.
	ERR_PPS_ID_OUTOFRANGE,     //!< pps_id over supported range, (2 by default, standard allows 0..255.
	ERR_UNSUPPORTED_PROFILE,   //!< Only Main profile is supported.
	ERR_UNSUPPORTED_POC_TYPE,  //!< Only type=0 is implemented by now.
	ERR_LOG2MAXFRNUMOUTRANGE,  //!< Over 12, according to the Standard.
	ERR_LOG2MAXPOCNTOUTRANGE,  //!< Over 12, according to the Standard.
	ERR_PIC_WIDTHOUTRANGE,     //!< Picture is too wide.
	ERR_PIC_HEIGHTOUTRANGE,    //!< Picture is too high.

	ERR_PPS_SPS_ID_OUTRANGE,   //!< PPS.seq_id is out of range.
	ERR_PPS_SPS_ID_NONEXISTS,  //!< PPS.seq_id does not exist.

	ERR_SH_PPS_ID_OUTOFRANGE,  //!< SH.pps_id is out of range.
	ERR_SH_PPS_ID_NONEXISTS,   //!< SH.pps_id does not exist.

	ERR_CAVLC_NOTSUPPORTED,    //!< Not a CABAC stream (not supported).
	ERR_MORETHANONESLICE,      //!< More than one slice per picture, not supported.
	ERR_FIRSTMBADDRNONEZERO,   //!< First MB address of the slice is not zero.
	ERR_INVALID_SLICETYPE,     //!< Slice type is not supported.
	ERR_NOENOUGHFRMBUFS,       //!< There is no enough frame buffers as reqired by DPB.
	ERR_FRMNO_IDRNONZERO,      //!< Current slice is IDR but frame num is not zero.
	ERR_DIRECT_NONSPATIAL,     //!< Direct more is not spatial (temporal).
	ERR_REFREORDER_INVALID,    //!< Invalid reordering_of_pic_idc (not 0 ~3)
	ERR_REFREORDER_OVERLIMIT,  //!< Reorder reference picture over limit, see 7.4.3.1.
	ERR_ERMARKER_OVERLIMIT,    //!< Ref picture markering number over limit, see 7.4.3.3
	ERR_NUM_REFLIST0_TOOBIG,   //!<
	ERR_NUM_REFLIST1_TOOBIG,   //!<
	ERR_CABAC_INIT_IDC_NOZERO, //!< Only INIT_CABAC_IDC=0 is implemented.
	ERR_DECMB,                 //!< Error occurs during decoding an individual MB
	ERR_CMDINWROUNDSTATE,      //!< Command issued in wrong state, thus cannot be executed.
	ERR_NOIDR,                 //!< Have not seen an IDR slice
	ERR_BWD,                   //!< General BWD errors
	ERR_POCTYPE_NONSUPPORTED,  //!< POC type not supported
	ERR_LIST_TOO_SMALL,        //!< The number of entries of the list smaller than active_minus1 + 1
	ERR_MMBUF,                 //!< Invalid Memory management or buffer operations
	ERR_MISMATCHFIELDS,        //!< Field pairs not matched
	ERR_PJPEG_TOO_BIG,         //!< PJPEG too big
	ERR_UNKNOWN,               //!< Unknown error
}HDEC_ERROR;

void iav_clean_decode_stuff(struct ambarella_iav *iav);

int iav_decode_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg);

#endif	// __IAV_DEC_API_H__

