/*
 * kernel/private/include/dsp_format.h
 *
 * History:
 *    2008/08/03 - [Anthony Ginger] Create
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


#ifndef __AMBA_DSP_H
#define __AMBA_DSP_H

#include <iav_common.h>

#define MIN_HDR_EXPO_NUM	(1)
#define MAX_HDR_EXPO_NUM	(5)

enum {
	AMBA_DSP_VFR_29_97_INTERLACE		= 0,
	AMBA_DSP_VFR_29_97_PROGRESSIVE		= 1,
	AMBA_DSP_VFR_59_94_INTERLACE		= 2,
	AMBA_DSP_VFR_59_94_PROGRESSIVE		= 3,
	AMBA_DSP_VFR_23_976_PROGRESSIVE		= 4,
	AMBA_DSP_VFR_12_5_PROGRESSIVE		= 5,
	AMBA_DSP_VFR_6_25_PROGRESSIVE		= 6,
	AMBA_DSP_VFR_3_125_PROGRESSIVE		= 7,
	AMBA_DSP_VFR_7_5_PROGRESSIVE		= 8,
	AMBA_DSP_VFR_3_75_PROGRESSIVE		= 9,
	AMBA_DSP_VFR_10_PROGRESSIVE		= 10,
	AMBA_DSP_VFR_15_PROGRESSIVE		= 15,
	AMBA_DSP_VFR_24_PROGRESSIVE		= 24,
	AMBA_DSP_VFR_25_PROGRESSIVE		= 25,
	AMBA_DSP_VFR_30_PROGRESSIVE		= 30,
	AMBA_DSP_VFR_50_PROGRESSIVE		= 50,
	AMBA_DSP_VFR_60_PROGRESSIVE		= 60,
	AMBA_DSP_VFR_120_PROGRESSIVE		= 120,

	AMBA_DSP_VFR_25_INTERLACE		= (0x80 | 25),
	AMBA_DSP_VFR_50_INTERLACE		= (0x80 | 50),
	AMBA_DSP_VFR_23_976_INTERLACED		= (0x80 | 4),
	AMBA_DSP_VFR_12_5_INTERLACED		= (0x80 | 5),
	AMBA_DSP_VFR_6_25_INTERLACED		= (0x80 | 6),
	AMBA_DSP_VFR_3_125_INTERLACED		= (0x80 | 7),
	AMBA_DSP_VFR_7_5_INTERLACED		= (0x80 | 8),
	AMBA_DSP_VFR_3_75_INTERLACED		= (0x80 | 9),
};


static inline u32 amba_iav_fps_format_to_vfr(u32 fps, u32 format, u32 multi_frames)
{
	u32 vfr = AMBA_DSP_VFR_29_97_PROGRESSIVE;

	if ((fps == AMBA_VIDEO_FPS_29_97) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_29_97_INTERLACE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_29_97) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = (multi_frames == 2) ? AMBA_DSP_VFR_15_PROGRESSIVE :
			AMBA_DSP_VFR_29_97_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_59_94) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_59_94_INTERLACE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_59_94) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = (multi_frames == 2) ? AMBA_DSP_VFR_29_97_PROGRESSIVE :
			AMBA_DSP_VFR_59_94_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_23_976) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_23_976_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_23_976) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_23_976_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_12_5) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_12_5_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_12_5) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_12_5_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_6_25) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_6_25_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_6_25) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_6_25_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_3_125) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_3_125_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_3_125) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_3_125_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_7_5) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_7_5_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_7_5) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_7_5_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_3_75) && (format == AMBA_VIDEO_FORMAT_INTERLACE)) {
		vfr = AMBA_DSP_VFR_3_75_INTERLACED;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_3_75) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_3_75_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_25) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_25_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_30) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_30_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_24) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_24_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_15) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_15_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_50) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_50_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_60) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_60_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if ((fps == AMBA_VIDEO_FPS_120) && (format == AMBA_VIDEO_FORMAT_PROGRESSIVE)) {
		vfr = AMBA_DSP_VFR_120_PROGRESSIVE;
		goto amba_iav_fps_format_to_vfr_exit;
	}

	if (format == AMBA_VIDEO_FORMAT_INTERLACE) {
		vfr = DIV_CLOSEST(512000000, fps) | 0x80;
	} else if (format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		vfr = DIV_CLOSEST(512000000, fps);
		if (multi_frames > MIN_HDR_EXPO_NUM && multi_frames < MAX_HDR_EXPO_NUM)
			vfr /= multi_frames;
	}

amba_iav_fps_format_to_vfr_exit:
	return vfr;
}

enum {
	AMBA_DSP_VIDEO_FORMAT_PROGRESSIVE		= 0,
	AMBA_DSP_VIDEO_FORMAT_INTERLACE			= 1,
	AMBA_DSP_VIDEO_FORMAT_DEF_PROGRESSIVE		= 2,
	AMBA_DSP_VIDEO_FORMAT_DEF_INTERLACE		= 3,
	AMBA_DSP_VIDEO_FORMAT_TOP_PROGRESSIVE		= 4,
	AMBA_DSP_VIDEO_FORMAT_BOT_PROGRESSIVE		= 5,
	AMBA_DSP_VIDEO_FORMAT_NO_VIDEO			= 6,
};

static inline u8 amba_iav_format_to_format(u32 format)
{
	u8 dsp_format;

	switch(format) {
	case AMBA_VIDEO_FORMAT_INTERLACE:
		dsp_format = AMBA_DSP_VIDEO_FORMAT_INTERLACE;
		break;
	case AMBA_VIDEO_FORMAT_PROGRESSIVE:
		dsp_format = AMBA_DSP_VIDEO_FORMAT_PROGRESSIVE;
		break;
	default:
		dsp_format = AMBA_DSP_VIDEO_FORMAT_NO_VIDEO;
		break;
	}

	return dsp_format;
}

static inline u8 dsp_format_to_amba_iav_format(u32 format)
{
	u8 iav_format;

	switch(format) {
	case AMBA_DSP_VIDEO_FORMAT_INTERLACE:
		iav_format = AMBA_VIDEO_FORMAT_INTERLACE;
		break;
	case AMBA_DSP_VIDEO_FORMAT_PROGRESSIVE:
		iav_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		break;
	default:
		iav_format = AMBA_VIDEO_FORMAT_AUTO;
		break;
	}

	return iav_format;
}

enum {
	AMBA_DSP_VIDEO_FPS_29_97	= 0,
	AMBA_DSP_VIDEO_FPS_59_94	= 1,
	AMBA_DSP_VIDEO_FPS_23_976	= 2,
	AMBA_DSP_VIDEO_FPS_12_5		= 3,
	AMBA_DSP_VIDEO_FPS_6_25		= 4,
	AMBA_DSP_VIDEO_FPS_3_125	= 5,
	AMBA_DSP_VIDEO_FPS_7_5		= 6,
	AMBA_DSP_VIDEO_FPS_3_75		= 7,
	AMBA_DSP_VIDEO_FPS_15		= 15,
	AMBA_DSP_VIDEO_FPS_24		= 24,
	AMBA_DSP_VIDEO_FPS_25		= 25,
	AMBA_DSP_VIDEO_FPS_30		= 30,
	AMBA_DSP_VIDEO_FPS_50		= 50,
	AMBA_DSP_VIDEO_FPS_60		= 60,
	AMBA_DSP_VIDEO_FPS_120		= 120,
};

static inline u8 amba_iav_fps_to_fps(u32 fps)
{
	u8 dsp_fps;

	switch (fps) {
	case AMBA_VIDEO_FPS_29_97:
		dsp_fps = AMBA_DSP_VIDEO_FPS_29_97;
		break;
	case AMBA_VIDEO_FPS_59_94:
		dsp_fps = AMBA_DSP_VIDEO_FPS_59_94;
		break;
	case AMBA_VIDEO_FPS_25:
		dsp_fps = AMBA_DSP_VIDEO_FPS_25;
		break;
	case AMBA_VIDEO_FPS_30:
		dsp_fps = AMBA_DSP_VIDEO_FPS_30;
		break;
	case AMBA_VIDEO_FPS_50:
		dsp_fps = AMBA_DSP_VIDEO_FPS_50;
		break;
	case AMBA_VIDEO_FPS_60:
		dsp_fps = AMBA_DSP_VIDEO_FPS_60;
		break;
	case AMBA_VIDEO_FPS_15:
		dsp_fps = AMBA_DSP_VIDEO_FPS_15;
		break;
	case AMBA_VIDEO_FPS_7_5:
		dsp_fps = AMBA_DSP_VIDEO_FPS_7_5;
		break;
	case AMBA_VIDEO_FPS_3_75:
		dsp_fps = AMBA_DSP_VIDEO_FPS_3_75;
		break;
	case AMBA_VIDEO_FPS_12_5:
		dsp_fps = AMBA_DSP_VIDEO_FPS_12_5;
		break;
	case AMBA_VIDEO_FPS_6_25:
		dsp_fps = AMBA_DSP_VIDEO_FPS_6_25;
		break;
	case AMBA_VIDEO_FPS_3_125:
		dsp_fps = AMBA_DSP_VIDEO_FPS_3_125;
		break;
	case AMBA_VIDEO_FPS_23_976:
		dsp_fps = AMBA_DSP_VIDEO_FPS_23_976;
		break;
	case AMBA_VIDEO_FPS_24:
		dsp_fps = AMBA_DSP_VIDEO_FPS_24;
		break;
	case AMBA_VIDEO_FPS_120:
		dsp_fps = AMBA_DSP_VIDEO_FPS_120;
		break;
	default:
		dsp_fps = AMBA_DSP_VIDEO_FPS_29_97;
		break;
	}

	return dsp_fps;
}

/* ==========================================================================*/

#endif	//__AMBA_DSP_H

