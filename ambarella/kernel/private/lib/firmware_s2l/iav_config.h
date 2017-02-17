/*
 * iav_config.h
 *
 * History:
 *	2008/10/23 - [Louis Sun] created file
 *	2012/03/06 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_CONFIG_H__
#define __IAV_CONFIG_H__

#include <linux/mm.h>
#include "amba_arch_mem.h"


/************************************************
 *
 * Following definitions can be modified.
 * They are software limitations based on different
 * NAND size and memory configurations.
 *
 ***********************************************/

/*
 * Debug option
 */
#define CONFIG_AMBARELLA_IAV_DEBUG


/*
 * Due to CPU performance penalty, disable using HR timer for
 * polling read out protocol
 */
#define CONFIG_DISABLE_HRTIMER_POLL_READOUT


/*
 * Bit stream info descriptor total number
 * add padding to avoid cache alignment problem
 */
#define NUM_BS_DESC				(256)



/*
 * Force IDR limit, cannot insert faster than this LIMIT number of VSYNC
 */
#define IDR_INSERT_LIMIT			(2)


/*
 * Dump QP histogram, preview and ME1 buffer configurations
 */
#define CONFIG_DUMP_QP_HIST_INFO
#define CONFIG_DUMP_PREVIEWC_BUFFER
#define CONFIG_DUMP_ME1_BUFFER



/************************************************
 *
 * Following definitions CANNOT be modified !!
 * They are HW limitations !!
 *
 ***********************************************/

/*
 * Basic configuration
 */
#define PIXEL_IN_MB			(16)
#define MAX_ENC_DUMMY_LATENCY	(5)
#define IAV_MAX_ENCODE_BITRATE	(50 * MB)
#define IAV_PARTITION_TOGGLE_NUM	(4)
#define FPS_Q9_BASE			(512000000U)
#define MAX_SLICE_FOR_FRAME	(2)
#define TWO_JIFFIES			(msecs_to_jiffies(2000))
#define FIVE_JIFFIES		(msecs_to_jiffies(5000))

#define MIN_CRD_IN_MS		(4)
#define AUDIO_CLK_KHZ		(12288)
#define MIN_CRD_IN_CLK		(MIN_CRD_IN_MS * AUDIO_CLK_KHZ)

#define MIN_VIN_UPSAMPLE_FRAME_RATE	(FPS_Q9_BASE / 10)

#define KByte(x)				((x) << 10)
#define MByte(x)				((x) << 20)

#define MIN_WARP_INPUT_WIDTH	(320)
#define MAX_WIDTH_FOR_TWO_REF	(2560)

/*
 * GOP and preview configuration for 2Gb and 1Gb
 */
#define CONFIG_STREAM_A_MAX_GOP_M			3
#define CONFIG_MAX_PREVIEW_NUM			2

/*
 * Encode configuration
 */
#define IAV_MAX_ENCODE_STREAMS_NUM		(4)
#define STREAM_ID_MASK			((1 << IAV_MAX_ENCODE_STREAMS_NUM) - 1)
#define PREVIEW_ID_MASK			((1 << CONFIG_MAX_PREVIEW_NUM) - 1)
#define LONG_TERM_INTVL_MAX 	(63)
#define MAX_REF_FRAME_INTVL		(3)
#define MAX_ZMV_THRESHOLD		(255)
#define MIN_INTRABIAS 			(1)
#define MAX_INTRABIAS			(4000)
#define MIN_USER_BIAS 			(0)
#define MAX_USER_BIAS			(128)
#define MAX_FRAME_DROP_COUNT	(255)

#define QP_QUALITY_MIN	(0)
#define QP_QUALITY_MAX	(3)
#define QP_OFFSET_MIN	(-51)
#define QP_OFFSET_MAX	(51)



typedef enum {
	IAV_EXTRA_DRAM_BUF_DEFAULT = 0,
	IAV_EXTRA_DRAM_BUF_MIN = 0,
	IAV_EXTRA_DRAM_BUF_MAX = 7,
} IAV_EXTRA_DRAM_BUF;


/*
 * Encode performance load
 */
typedef enum {
	/* Encode performance in MB unit:
		216000 = (1280/16) * (720/16) * 60
		305340 = (1920/16) * (1088/16) * 31 + (720/16) * (480/16) * 30 + (352/16) * (288/16) * 30
		336960 = (2592/16) * (1952/16) * 15 + (720/16) * (480/16) * 30
		409140 = (2048/16) * (1536/16) * 30 + (720/16) * (480/16) * 30
		422280 = (2592/16) * (1952/16) * 20 + (720/16) * (480/16) * 20
		442044 = (2592/16) * (1952/16) * 21 + (720/16) * (480/16) * 20
		491520 = (2048/16) * (2048/16) * 30
		570600 = (1920/16) * (1088/16) * 60 + (720/16) * (480/16) * 60
		592920 = (2592/16) * (1952/15) * 30
		633420 = (2592/16) * (1952/16) * 30 + (720/16) * (480/16) * 30
		653184 = (2592/16) * (1952/15) * 31 + (720/16) * (480/16) * 30
		897600 = (1920/16) * (1088/16) * 110
	*/
	LOAD_720P60 = (1280 * 720 / 256 * 60),
	LOAD_1080P31_480P30_CIFP30 = (1920 * 1088 / 256 * 31 + 720 * 480 / 256 * 30 + 352 * 288 / 256 * 30),
	LOAD_5MP15_480P30 = (2592 * 1952 / 256 * 15 + 720 * 480 / 256 * 30),
	LOAD_3MP30_480P30 = (2048 * 1536 / 256 * 30 + 720 * 480 / 256 * 30),
	LOAD_5MP20_480P20 = (2592 * 1952 / 256 * 20 + 720 * 480 / 256 * 20),
	LOAD_4MP30 = (2048 * 2048 / 256 * 30),
	LOAD_1080P60_480P60 = (1920 * 1088 / 256 * 60 + 720 * 480 / 256 * 60),
	LOAD_5MP30 = (2592 * 1952 / 256 * 30),
	LOAD_5MP30_480P30 = (2592 * 1952 / 256 * 30 + 720 * 480 / 256 * 30),
	LOAD_5MP31_480P30 = (2592 * 1952 / 256 * 31 + 720 * 480 / 256 * 30),
	LOAD_1080P110 = (1920 * 1088 / 256 * 110),

	SYS_ENC_LOAD_S2L_22M = LOAD_720P60,
	SYS_ENC_LOAD_S2L_33M = LOAD_1080P31_480P30_CIFP30,
	SYS_ENC_LOAD_S2L_55M = LOAD_3MP30_480P30,
	SYS_ENC_LOAD_S2L_99M = LOAD_5MP20_480P20,
	SYS_ENC_LOAD_S2L_63 = LOAD_3MP30_480P30,
	SYS_ENC_LOAD_S2L_66 = LOAD_5MP30_480P30,
	SYS_ENC_LOAD_S2L_88 = LOAD_5MP31_480P30,
	SYS_ENC_LOAD_S2L_99 = LOAD_1080P110,
	SYS_ENC_LOAD_S2L_TEST = LOAD_5MP20_480P20,
} IAV_SYS_ENC_LOAD;


/*
 * Encode resource limitation
 */
typedef enum {
	MIN_WIDTH_IN_STREAM = 320,
	MIN_WIDTH_IN_STREAM_ROTATE = 240,
	MIN_HEIGHT_IN_STREAM = 240,

	MAX_WIDTH_IN_BUFFER = 4 * 1024,
	MAX_HEIGHT_IN_BUFFER = 4 * 1024,

	/* full frame rate mode for CFA */
	MAX_WIDTH_IN_FULL_FPS = 1920,
	MAX_HEIGHT_IN_FULL_FPS = 1088,
	MIN_WIDTH_IN_FULL_FPS = MIN_WIDTH_IN_STREAM_ROTATE,
	MIN_HEIGHT_IN_FULL_FPS = MIN_HEIGHT_IN_STREAM,

	/* mutli-region warping mode for CFA */
	MAX_WIDTH_IN_WARP_REGION = 1920,
	MAX_PRE_WIDTH_IN_WARP = MAX_WIDTH_IN_WARP_REGION,
	MAX_PRE_HEIGHT_IN_WARP = MAX_WIDTH_IN_WARP_REGION,
	MAX_WIDTH_IN_WARP = MAX_WIDTH_IN_WARP_REGION,
	MAX_HEIGHT_IN_WARP = MAX_WIDTH_IN_WARP_REGION,
	MIN_WIDTH_IN_WARP = MIN_WIDTH_IN_STREAM,
	MIN_HEIGHT_IN_WARP = MIN_HEIGHT_IN_STREAM,

	/* high MP mode */
	MAX_WIDTH_IN_HIGH_MP = 3072,
	MAX_HEIGHT_IN_HIGH_MP = 2048,
	MIN_WIDTH_IN_HIGH_MP = MIN_WIDTH_IN_STREAM_ROTATE,
	MIN_HEIGHT_IN_HIGH_MP = MIN_HEIGHT_IN_STREAM,

	/* RAW encode case */
	MAX_WIDTH_FOR_RAW = MAX_WIDTH_IN_HIGH_MP,
	MAX_HEIGHT_FOR_RAW = MAX_HEIGHT_IN_HIGH_MP,
	MIN_WIDTH_FOR_RAW = MIN_WIDTH_IN_FULL_FPS,
	MIN_HEIGHT_FOR_RAW = MIN_HEIGHT_IN_FULL_FPS,

	/* EFM encode case */
	MAX_WIDTH_FOR_EFM = MAX_WIDTH_IN_HIGH_MP,
	MAX_HEIGHT_FOR_EFM = MAX_HEIGHT_IN_HIGH_MP,
	MIN_WIDTH_FOR_EFM = MIN_WIDTH_IN_FULL_FPS,
	MIN_HEIGHT_FOR_EFM = MIN_HEIGHT_IN_FULL_FPS,
} RESOLUTION_RESOURCE_LIMIT;


/*
 * Capture source buffer configuration
 */
#define CONFIG_MAX_SOURCE_BUFFER_NUM	(4)

typedef enum {
	MNBUF_MAX_ZOOMIN_FACTOR = 8,
	MNBUF_MAX_ZOOMOUT_FACTOR = 4,

	PCBUF_MAX_ZOOMIN_FACTOR = 1,
	PCBUF_MAX_ZOOMOUT_FACTOR = 16,

	PBBUF_MAX_ZOOMIN_FACTOR = 8,
	PBBUF_MAX_ZOOMOUT_FACTOR = 8,

	PABUF_MAX_ZOOMIN_FACTOR = 8,
	PABUF_MAX_ZOOMOUT_FACTOR = 16,
} SOURCE_BUFFER_ZOOM_FACTOR_LIMIT;


typedef enum {
	/*
	 * Resolution limitation for the 4 source buffers
	 */
	MNBUF_LIMIT_MAX_WIDTH = MAX_WIDTH_IN_HIGH_MP,
	MNBUF_LIMIT_MAX_HEIGHT = MAX_HEIGHT_IN_HIGH_MP,
	PCBUF_LIMIT_MAX_WIDTH = 720,
	PCBUF_LIMIT_MAX_HEIGHT = 720,
	PBBUF_LIMIT_MAX_WIDTH = 1920,
	PBBUF_LIMIT_MAX_HEIGHT = 1920,
	PABUF_LIMIT_MAX_WIDTH = 1280,
	PABUF_LIMIT_MAX_HEIGHT = 1280,

	/*
	 * Default max resolution for the 4 source buffers
	 */
	MNBUF_DEFAULT_MAX_WIDTH = 1920,
	MNBUF_DEFAULT_MAX_HEIGHT = 1080,
	PCBUF_DEFAULT_MAX_WIDTH = 720,
	PCBUF_DEFAULT_MAX_HEIGHT = 576,
	PBBUF_DEFAULT_MAX_WIDTH = 1280,
	PBBUF_DEFAULT_MAX_HEIGHT = 720,
	PABUF_DEFAULT_MAX_WIDTH = 720,
	PABUF_DEFAULT_MAX_HEIGHT = 576,
} SOURCE_BUFFER_RESOLUTION_LIMIT;


/*
 * HDR configuration
 */
typedef enum {
	// Multi exposures supported in DSP
	MIN_HDR_EXPOSURES = 1,
	MAX_HDR_2X = 2,
	MAX_HDR_3X = 3,
	MAX_HDR_EXPOSURES = 4,
} IAV_HDR_PARAMS_LIMIT;

/*
 * EFM configuration
 */
#define IAV_EFM_MAX_BUF_NUM		(10)
#define IAV_EFM_MIN_BUF_NUM		(5)
#define IAV_EFM_INIT_REQ_NUM	(4)

typedef enum {
	EFM_STATE_INIT_OK = 0,
	EFM_STATE_CREATE_OK = 1,
	EFM_STATE_WORKING_OK = 2,
} IAV_EFM_STATE;

/*
 * Overlay memory configuration
 */
typedef enum {
	// OSD width CANNOT be larger than 1920, it cannot do stitching
	OVERLAY_AREA_MAX_WIDTH = MAX_WIDTH_IN_FULL_FPS,
	MAX_NUM_OSD_AERA_NO_EXTENSION = 3,
	MAX_FASTOSD_STRING_LENGTH = 32,
} IAV_OVERLAY_PARAMS;


typedef enum {
	DSP_ENC_CMD_SIZE = (128),
	ENC_CMD_TOGGLED_NUM = (8),
	CMD_SYNC_SIZE = (DSP_ENC_CMD_SIZE * IAV_MAX_ENCODE_STREAMS_NUM),
	CMD_SYNC_TOTAL_SIZE = (CMD_SYNC_SIZE * ENC_CMD_TOGGLED_NUM),
} IAV_CMD_SYNC_PARAMS;

/*
 *
 */
typedef enum {
	SINGLE_QP_MATRIX_SIZE = IAV_MEM_QPM_SIZE,
#ifndef CONFIG_AMBARELLA_IAV_QP_OFFSET_IPB
	STREAM_QP_MATRIX_NUM = (1),
#else
	STREAM_QP_MATRIX_NUM = (3),
#endif
	STREAM_QP_MATRIX_SIZE = (SINGLE_QP_MATRIX_SIZE * STREAM_QP_MATRIX_NUM),
	QP_MATRIX_SIZE = (STREAM_QP_MATRIX_SIZE * IAV_MAX_ENCODE_STREAMS_NUM),
	/* 96KB * 4 * (1 + 8) = 3456 KB for 1 qp matrix per stream*/
	/* 96KB * 3 * 4 * (1 + 8) = 10368 KB for 3 qp matrixes per stream*/
	QP_MATRIX_TOTAL_SIZE = (QP_MATRIX_SIZE * (1 + ENC_CMD_TOGGLED_NUM)),
}IAV_QPMATRIX_PARAMS;


/*
 * Privacymask memory configuration
 */
typedef enum {
	/*
	 * BPC based privacy mask memory configuration
	 */
	PM_BPC_WIDTH_MAX = MAX_WIDTH_IN_HIGH_MP,
	PM_BPC_HEIGHT_MAX = MAX_HEIGHT_IN_HIGH_MP,
	//User Privacy Mask Partition
	UPMP_NUM = 1,
	PM_BPC_PARTITION_SIZE = ALIGN(PM_BPC_WIDTH_MAX / 8, 32) * PM_BPC_HEIGHT_MAX,
	/*
	 * MCTF based privacy mask memory configuration
	*/
	PM_MCTF_PARTITION_SIZE = (96 << 10),
	PM_MCTF_TOGGLED_NUM = (8),
	PM_MCTF_TOTAL_SIZE = (PM_MCTF_PARTITION_SIZE * PM_MCTF_TOGGLED_NUM),
} IAV_PM_PARAMS;

/*
 * WARP configuration
 */
typedef enum {
	VWARP_BLOCK_HEIGHT_MAX = 64,
	VWARP_BLOCK_HEIGHT_MAX_LDC = 32,
	VWARP_BLOCK_HEIGHT_MIN = 28,
	LDC_PADDING_WIDTH_MAX = 256,

	WARP_TABLE_AREA_MAX_NUM = 8,
	WARP_TABLE_AREA_MAX_WIDTH = 32,
	WARP_TABLE_AREA_MAX_HEIGHT = 48,
	WARP_TABLE_AREA_MAX_SIZE = (WARP_TABLE_AREA_MAX_WIDTH
	    * WARP_TABLE_AREA_MAX_HEIGHT),
	/* User Warp Partition */
	UWARP_NUM = 1,
	WARP_CMD_SIZE = 128,
	WARP_VECT_PART_SIZE = (WARP_TABLE_AREA_MAX_NUM * \
		WARP_TABLE_AREA_MAX_SIZE * sizeof(s16) * 3),  // 3 tables per one area(H-warp, V-warp, ME1-Vwarp)
	WARP_VECT_TOTAL_SIZE = WARP_VECT_PART_SIZE * (IAV_PARTITION_TOGGLE_NUM + UWARP_NUM),
	WARP_BATCH_PART_SIZE = WARP_TABLE_AREA_MAX_NUM * WARP_CMD_SIZE,
	WARP_BATCH_CMDS_OFFSET = WARP_VECT_TOTAL_SIZE,
	WARP_BATCH_CMDS_SIZE = WARP_BATCH_PART_SIZE * IAV_PARTITION_TOGGLE_NUM,

	/* 8 * 3KB * 3 * (4 + 1) + 4 = 364 KB */
	WARP_BUFFER_TOTAL_SIZE = WARP_VECT_TOTAL_SIZE + WARP_BATCH_CMDS_SIZE,
} IAV_WARP_PARAMS;

typedef enum {
	JPEG_QT_SIZE = 128,
	JPEG_QT_BUFFER_NUM = 4,
	JPEG_QT_TOTAL_SIZE = (IAV_MAX_ENCODE_STREAMS_NUM * JPEG_QT_SIZE * JPEG_QT_BUFFER_NUM),
} IAV_JPEG_QT_PARAMS;

/*
 * Dump YUV buffer / ME 1 buffer / histogram configurations
 */
typedef enum {
	NUM_QP_HISTOGRAM_BUF = 12,
} IAV_DUMP_BUFFER_PARAMS;

/*
 * Memory configuration
 */
typedef enum {
	IAV_DRAM_SIZE_1Gb = 0x1,
	IAV_DRAM_SIZE_2Gb = 0x2,
	IAV_DRAM_SIZE_4Gb = 0x4,
	IAV_DRAM_SIZE_8Gb = 0x8,
	IAV_DRAM_SIZE_16Gb = 0x10,

	/* All DRAM size must be aligned to PAGE size (4KB). */
	IAV_DRAM_BSB = DSP_BSB_SIZE,
	IAV_DRAM_USR = IAV_MEM_USR_SIZE,
	IAV_DRAM_MV = 0,
	IAV_DRAM_OVERLAY = IAV_MEM_OVERLAY_SIZE,
	IAV_DRAM_QPM = PAGE_ALIGN(QP_MATRIX_TOTAL_SIZE),
#ifndef CONFIG_AMBARELLA_IAV_DRAM_WARP_MEM
	IAV_DRAM_WARP = 0,
#else
	IAV_DRAM_WARP = PAGE_ALIGN(WARP_BUFFER_TOTAL_SIZE),
#endif
	IAV_DRAM_QUANT = PAGE_ALIGN(JPEG_QT_TOTAL_SIZE),
	IAV_DRAM_IMG = DSP_IMGRSVD_SIZE,

	/* 768KB * (4 + 1) = 3840 KB */
	IAV_DRAM_PM = PAGE_ALIGN(IAV_MEM_PM_SIZE_S2L),

	/* One Page is for BPC_SETUP, and is used internally. Do not touch this PAGE!
	 *  Total size = 768 KB + 4 KB = 772 KB */
	IAV_DRAM_BPC = PAGE_SIZE + PAGE_ALIGN(PM_BPC_PARTITION_SIZE),

	IAV_DRAM_CMD_SYNC = PAGE_ALIGN(CMD_SYNC_TOTAL_SIZE),
	/* The end of IAVRSVD */

	IAV_DRAM_FB_DATA = PAGE_ALIGN(DSP_FASTDATA_SIZE),
	IAV_DRAM_FB_AUDIO = PAGE_ALIGN(DSP_FASTAUDIO_SIZE),

	IAV_DRAM_MAX = IAV_DRAM_BSB + IAV_DRAM_USR + IAV_DRAM_MV + IAV_DRAM_OVERLAY +
		IAV_DRAM_QPM + IAV_DRAM_WARP + IAV_DRAM_QUANT + IAV_DRAM_IMG +
		IAV_DRAM_PM + IAV_DRAM_BPC + IAV_DRAM_CMD_SYNC,

	IAV_DRAM_IMG_OFFET = (MByte(4)),
} IAV_DRAM;


#endif	// __IAV_CONFIG_H__
