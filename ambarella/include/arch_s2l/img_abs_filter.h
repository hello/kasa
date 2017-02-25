/*
 * include/arch_s2l/img_abs_filter.h
 *
 * History:
 *	2012/10/10 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] modified file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef IMG_ABS_FILTER_H
#define IMG_ABS_FILTER_H

#include "basetypes.h"


#define ADJ_MAX_ENTRY_NUM	(16)
#define ADJ_MAX_INNER_TBL_NUM	(4)

#define FILTER_ID_MASK ((1 << 24) - 1)

typedef enum {
	FILTER_OFFSET_NORAML = (0 << 24),
	FILTER_OFFSET_WDR = (1 << 24),
	FILTER_OFFSET_2X_HDR = (2 << 24),
	FILTER_OFFSET_3X_HDR = (3 << 24),
	FILTER_OFFSET_4X_HDR = (4 << 24),
} filter_offset_id;

typedef enum{
	AEB_TILE_CONFIG =0x0,
	AEB_EXPO_LINES =0X1,
	AEB_EXPO_LINES_2X_HDR =0x2,
	AEB_EXPO_LINES_3X_HDR =0x3,
	AEB_EXPO_LINES_4X_HDR =0x4,
	AEB_WB_PARAM =0x5,
	AEB_SENSOR_CONFIG =0x6,
	AEB_SHT_NL_TABLE =0x7,
	AEB_GAIN_TABLE =0x8,
	AEB_EXPO_LINES_PIRIS_LINEAR1 =0x9,
	AEB_EXPO_LINES_PIRIS_LINEAR2 =0xa,
	AEB_EXPO_LINES_PIRIS_HDR1 =0xb,
	AEB_EXPO_LINES_PIRIS_HDR2 =0xc,
	AEB_AUTO_KNEE =0xd,
	AEB_DIGIT_WDR = 0xe,
	AEB_DIGIT_WDR_2X_HDR = 0xf,
	AEB_DIGIT_WDR_3X_HDR = 0x10,
	AEB_ID_NUM,
}img_aeb_id;

typedef enum{
	ADJ_AE_TARGET = 0x0,
	ADJ_WB_RATIO = 0x1,
	ADJ_BLC = 0x2,
	ADJ_ANTIALIASING = 0x3,
	ADJ_GRGB_MISMATCH = 0x4,
	ADJ_DPC = 0x5,
	ADJ_CFANF_LOW = 0x6,
	ADJ_CFANF_HIGH = 0x7,
	ADJ_LE = 0x8,
	ADJ_DEMOSAIC = 0x9,
	ADJ_CC = 0xA,
	ADJ_TONE = 0xB,
	ADJ_RGB2YUV = 0xC,
	ADJ_CHROMA_SCALE = 0xD,
	ADJ_CHROMA_MEDIAN = 0xE,
	ADJ_CDNR = 0xF,
	ADJ_1STMODE_SEL = 0x10,
	ADJ_ASF = 0x11,
	ADJ_1ST_SHPBOTH = 0x12,
	ADJ_1ST_SHPNOISE = 0x13,
	ADJ_1ST_SHPFIR = 0x14,
	ADJ_1ST_SHPCORING = 0x15,
	ADJ_1ST_SHPCORING_INDEX_SCALE = 0x16,
	ADJ_1ST_SHPCORING_MIN_RESULT = 0x17,
	ADJ_1ST_SHPCORING_SCALE_CORING = 0x18,
	ADJ_VIDEO_MCTF = 0x19,
	ADJ_HDR_ALPHA = 0x1A,
	ADJ_HDR_THRESHOLD = 0x1B,
	ADJ_HDR_CE = 0x1C,
	ADJ_CHROMANF = 0x1D,
	ADJ_TEMPORAL_ADJUST =0x1E,
	ADJ_FINAL_SHPBOTH = 0x1F,
	ADJ_FINAL_SHPNOISE = 0x20,
	ADJ_FINAL_SHPFIR = 0x21,
	ADJ_FINAL_SHPCORING = 0x22,
	ADJ_FINAL_SHPCORING_INDEX_SCALE = 0x23,
	ADJ_FINAL_SHPCORING_MIN_RESULT = 0x24,
	ADJ_FINAL_SHPCORING_SCALE_CORING = 0x25,
	ADJ_WIDE_CHROMA_FILTER =0x26,
	ADJ_WIDE_CHROMA_FILTER_COMBINE =0x27,

	#if 1
	ADJ_MO_ANTIALIASING =0x28,		//
	ADJ_MO_GRGB_MISMATCH=0x29,		//
	ADJ_MO_DPC=0x2A,			//
	ADJ_MO_CFANF=0x2B,			//
	ADJ_MO_DEMOSAIC=0x2C,		//
	ADJ_MO_CHROMA_MEDIAN=0x2D,		//
	ADJ_MO_CNF=0x2E,
	ADJ_MO_1STMODE_SEL=0x2F,			//
	ADJ_MO_ASF=0x30,		//

	ADJ_MO_1ST_SHARPEN_BOTH=0x31,	//
	ADJ_MO_1ST_SHARPEN_NOISE=0x32,
	ADJ_MO_1ST_SHARPEN_FIR=0x33,
	ADJ_MO_1ST_SHARPEN_CORING=0x34,
	ADJ_MO_1ST_SHARPEN_CORING_INDEX_SCALE=0x35,
	ADJ_MO_1ST_SHARPEN_CORING_MIN_RESULT=0x36,
	ADJ_MO_1ST_SHARPEN_CORING_SCALE_CORING=0x37,
	#endif

	ADJ_FILTER_NUM,
}filter_id;

#define RESERVED_FILTER_NUM		(32)	//include the same filter id
#define TOTAL_FILTER_NUM (ADJ_FILTER_NUM + RESERVED_FILTER_NUM)

typedef struct argu2 {int argu[2];} argu2;
typedef struct argu4 {int argu[4];} argu4;
typedef struct argu8 {int argu[8];} argu8;
typedef struct argu16 {int argu[16];} argu16;
typedef struct argu32 {int argu[32];} argu32;
typedef struct argu64 {int argu[64];} argu64;

//for u16 PerDirFirIsoStrengths[9]
typedef struct table_u16x16 {u16 element[16];} table_u16x16;
//for s16  Coefs[9][25]
typedef struct table_s16x256 {s16 element[256];} table_s16x256;
//for u8   Coring[256];
typedef struct table_u8x256 {u8 element[256];} table_u8x256;
// for rgb2yuv[12]
typedef struct table_s16x16 {s16 element[16];} table_s16x16;
// for u16  CS[256];
typedef struct table_u16x128 {u16 element[128];} table_u16x128;
// for  u16  ToneCurve[256]; LE: u16  GainCurveTable[256];
typedef struct table_u16x256 {u16 element[256];} table_u16x256;
//for cc 3d
typedef struct table_u32x4096 {u32 element[4096];} table_u32x4096;


typedef struct tbl_des_s{
	int auto_adj_tbl;
	int active_element_num;
	int tbl_entry_start_idx;
	int active_tbl_entry_idx;
}tbl_des_t;

typedef struct filter_container_header_s{
	int filter_container_id;
	int auto_adj_argu;	//-1 not in use, 0 no interpo, 1 interpo, 2 only for init
	int active_argu_num;
	int argu_entry_start_idx;
	int active_argu_entry_idx;
	int inner_tbl_num;
	tbl_des_t tbl_info[ADJ_MAX_INNER_TBL_NUM];
}filter_container_header_t;

//amba_img_dsp_asf_info_t
typedef struct filter_container_p64_t4_s{
	filter_container_header_t header;
	argu64 param[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl2[ADJ_MAX_ENTRY_NUM];
	table_s16x256 tbl3[ADJ_MAX_ENTRY_NUM];
}filter_container_p64_t4_t;

//amba_img_dsp_sharpen_noise_t
typedef struct filter_container_p16_t4_s{
	filter_container_header_t header;
	argu16 param[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl2[ADJ_MAX_ENTRY_NUM];
	table_s16x256 tbl3[ADJ_MAX_ENTRY_NUM];
}filter_container_p16_t4_t;

//amba_img_dsp_local_exposure_t, amba_img_dsp_chroma_scale_t
typedef struct filter_container_p8_t1_s{
	filter_container_header_t header;
	argu8 param[ADJ_MAX_ENTRY_NUM];
	table_u16x256 tbl0[ADJ_MAX_ENTRY_NUM];
}filter_container_p8_t1_t;

//amba_img_dsp_coring_t
typedef struct filter_container_p0_t1_s{
	filter_container_header_t header;
	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u8x256 tbl0[ADJ_MAX_ENTRY_NUM];
}filter_container_p0_t1_t;

//Tone curve
typedef struct filter_container_p0_t3_s{
	filter_container_header_t header;
//	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u16x256 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x256 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x256 tbl2[ADJ_MAX_ENTRY_NUM];
}filter_container_p0_t3_t;

//rgb2yuv
typedef struct filter_container_p4_t3_s{
	filter_container_header_t header;
	argu4 param[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl2[ADJ_MAX_ENTRY_NUM];
}filter_container_p4_t3_t;

typedef struct filter_container_p2_s{
	filter_container_header_t header;
	argu2 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p2_t;

typedef struct filter_container_p4_s{
	filter_container_header_t header;
	argu4 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p4_t;

typedef struct filter_container_p8_s{
	filter_container_header_t header;
	argu8 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p8_t;

typedef struct filter_container_p16_s{
	filter_container_header_t header;
	argu16 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p16_t;

typedef struct filter_container_p32_s{
	filter_container_header_t header;
	argu32 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p32_t;

typedef struct filter_container_p64_s{
	filter_container_header_t header;
	argu64 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p64_t;


typedef struct filter_header_s{
	int filter_id;
	int filter_update_period;
	int filter_update_cnt;
	int filter_update_flag;
	int inner_tbl_num;
	int force_commit_flag;
}filter_header_t;

typedef struct filter_p64_t4_s{
	filter_header_t header;
	argu64 param;
	table_u16x16 tbl0;
	table_u16x16 tbl1;
	table_u16x16 tbl2;
	table_s16x256 tbl3;
}filter_p64_t4_t;

typedef struct filter_p16_t4_s{
	filter_header_t header;
	argu16 param;
	table_u16x16 tbl0;
	table_u16x16 tbl1;
	table_u16x16 tbl2;
	table_s16x256 tbl3;
}filter_p16_t4_t;

typedef struct filter_p8_t1_s{
	filter_header_t header;
	argu8 param;
	table_u16x256 tbl0;
}filter_p8_t1_t;

typedef struct filter_p0_t1_s{
	filter_header_t header;
	table_u8x256 tbl0;
}filter_p0_t1_t;

typedef struct filter_p0_t3_s{
	filter_header_t header;
//	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u16x256 tbl0;
	table_u16x256 tbl1;
	table_u16x256 tbl2;
}filter_p0_t3_t;

typedef struct filter_p4_t3_s{
	filter_header_t header;
	argu4 param;
	table_s16x16 tbl0;
	table_s16x16 tbl1;
	table_s16x16 tbl2;
}filter_p4_t3_t;

typedef struct filter_p2_s{
	filter_header_t header;
	argu2 param;
}filter_p2_t;

typedef struct filter_p4_s{
	filter_header_t header;
	argu4 param;
}filter_p4_t;

typedef struct filter_p8_s{
	filter_header_t header;
	argu8 param;
}filter_p8_t;

typedef struct filter_p16_s{
	filter_header_t header;
	argu16 param;
}filter_p16_t;

typedef struct filter_p32_s{
	filter_header_t header;
	argu32 param;
}filter_p32_t;

typedef struct filter_p64_s{
	filter_header_t header;
	argu64 param;
}filter_p64_t;

#endif
