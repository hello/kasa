/*
 * test_tuning.h
 *
 * History:
 *	2012/06/23 - [Teng Huang] created file
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

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"
#include "AmbaDSP_ImgUtility.h"
#include "AmbaDSP_ImgFilter.h"


#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "AmbaDSP_Img3aStatistics.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif
#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#define INFO_SOCKET_PORT		10006
#define ALL_ITEM_SOCKET_PORT 	10007
#define PREVIEW_SOCKET_PORT 	10008
#define HISTO_SOCKET_PORT 		10009

#define FIR_COEFF_SIZE  (10)
#define CORING_TABLE_SIZE	(256)
#define MAX_YUV_BUFFER_SIZE		(720*480)		// 1080p
#define ThreeD_size (8192)

//#define MAX_DUMP_BUFFER_SIZE (256*1024)
#define NO_ARG	0
#define HAS_ARG	1

const int n_bin =64;
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define CHECK_RVAL if(rval<0)	printf("error: %s:%d\n",__func__,__LINE__);
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))



static amba_img_dsp_local_exposure_t local_exposure = { 1, 4, 16, 16, 16, 6,
	{1024,1054,1140,1320,1460,1538,1580,1610,1630,1640,1640,1635,1625,1610,1592,1563,
	1534,1505,1475,1447,1417,1393,1369,1345,1321,1297,1273,1256,1238,1226,1214,1203,
	1192,1180,1168,1157,1149,1142,1135,1127,1121,1113,1106,1098,1091,1084,1080,1077,
	1074,1071,1067,1065,1061,1058,1055,1051,1048,1045,1044,1043,1042,1041,1040,1039,
	1038,1037,1036,1035,1034,1033,1032,1031,1030,1029,1029,1029,1029,1028,1028,1028,
	1028,1027,1027,1027,1026,1026,1026,1026,1025,1025,1025,1025,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}};
static amba_img_dsp_tone_curve_t tone_curve = {
		{/* red */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* green */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* blue */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
	};


enum ITEM_ID
{
       TUNING_DONE ='0',
	BlackLevelCorrection='A',
	ColorCorrection='B',
	ToneCurve='C',
	RGBtoYUVMatrix='D',
	WhiteBalanceGains='E',
	DGainSaturaionLevel='F',
	LocalExposure='G',
	ChromaScale='H',

	FPNCorrection='I',
	BadPixelCorrection='J',
	CFALeakageFilter='K',
	AntiAliasingFilter='L',
	CFANoiseFilter='M',
	ChromaMedianFiler='N',
	SharpeningA_ASF='O',
	MCTFControl='P',
	SharpeningBControl='Q',
	ColorDependentNoiseReduction='R',
	ChromaNoiseFilter='S',
	GMVSETTING='T',

	ConfigAAAControl='U',
	TileConfiguration='V',
	AFStatisticSetupEx='Y',
	ExposureControl='Z',

	CHECK_IP ='a',
	GET_RAW ='b',
	DSP_DUMP ='c',
	BW_MODE ='d',
	BP_CALI ='e',
	LENS_CALI ='f',
	AWB_CALI ='g',
	DEMOSAIC='h',
	MisMatchGr_Gb='i',
	WideChromaFilter ='j',
	WideChromaFilterCombine ='k',
	ImgConfigInfo='l',
	HDR_BATCH_CMD='m',
	AEBSetting='n',
	ITUNER_FILE =32,


	MoCFALeakageFilter = '1',
	MoAntiAlisingFilter = '2',
	MoCFANoiseFilter = '3',
	MoChromaMedianFilter = '4',
	MoSharpeningA_ASF = '5',
	MoChromaNoiseFilter = '6',
	MoDemosaic = '7',
	MoMisMatchGr_Gb = '8',
	MoBadPixelCorrection = '9',



};

enum REQ_ID
{
	LOAD='0',
	APPLY,
	OTHER,

};

enum TAB_ID
{
	OnlineTuning ='0',
	HDRTuning,

};

typedef struct
{
	u8 req_id;
	u8 tab_id;
	u8 item_id;
	u8 reserved;
	u32 size;

}TUNING_ID;



typedef struct
{
	u32 gain_tbl_idx;
	u32 shutter_row;
	u32 iris_index;
	u16 dgain;

}EC_INFO;

typedef struct
{
	u16 cfa_tile_num_col;
	u16 cfa_tile_num_row;
	u16 cfa_tile_col_start;
	u16 cfa_tile_row_start;
	u16 cfa_tile_width;
	u16 cfa_tile_height;
	u16 cfa_tile_active_width;
	u16 cfa_tile_active_height;
	u16 cfa_tile_cfa_y_shift;
	u16 cfa_tile_rgb_y_shift;
	u16 cfa_tile_min_max_shift;

}CFA_TILE_INFO;
typedef struct
{
	amba_img_dsp_aaa_stat_info_t stat_config_info;

}TILE_PKG;

typedef struct
{
	amba_img_dsp_fir_t fir;
	amba_img_dsp_coring_t coring;
	amba_img_dsp_level_t	ScaleCoring;
	amba_img_dsp_level_t	MinCoringResult;
	amba_img_dsp_level_t	CoringIndexScale;
	amba_img_dsp_sharpen_both_t both;
	amba_img_dsp_sharpen_noise_t noise;

}SHARPEN_PKG;
typedef struct
{
     amba_img_dsp_liso_process_select_t select_mode;
     SHARPEN_PKG sa_info;
     amba_img_dsp_asf_info_t asf_info;
}SA_ASF_PKG;

typedef struct
{
	amba_img_dsp_def_blc_t              DefBlc;
	amba_img_dsp_black_correction_t blc_info;
}BLC_PKG;

typedef struct
{
    amba_img_dsp_video_mctf_info_t mctf_info;
    amba_img_dsp_video_mctf_temporal_adjust_t mctf_temporal_adjust;
}MCTF_PKG;

typedef struct
{
	u16 ae_target_ratio;
	u8   awb_r_ratio;
	u8   awb_b_ratio;
}AEB_SETTING_INFO;

typedef struct
{
	cfa_pre_hdr_histogram_t  cfa_pre_histogram[MAX_EXPOSURE_NUM];
	cfa_histogram_stat_t cfa_histogram;
	rgb_histogram_stat_t rgb_histogram;
}HISTO_PKG;

typedef struct
{
	u16 amp_linear_lut[4*AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE];
	amba_img_dsp_blklvl_amplinear_t ampli_blc[MAX_HDR_EXPOSURE_NUM - 1];
	u8 alpha_lut [(MAX_HDR_EXPOSURE_NUM - 1)*AMBA_DSP_IMG_ALPHA_LUT_SIZE];
	amba_img_dsp_black_correction_t prebld_blc;
	u32 wb_round0_r;
	u32 wb_round0_g;
	u32 wb_round0_b;
	u32 wb_round1_r;
	u32 wb_round1_g;
	u32 wb_round1_b;
	u32 wb_round2_r;
	u32 wb_round2_g;
	u32 wb_round2_b;
	u32 wb_round3_r;
	u32 wb_round3_g;
	u32 wb_round3_b;
	amba_img_dsp_hdr_alpha_calc_thresh_t 	prebld_thre[MAX_HDR_EXPOSURE_NUM - 1];
	amba_img_dsp_black_correction_t sensor_blc[MAX_HDR_EXPOSURE_NUM];
	u32 ae_target;
	u32 shutter[MAX_HDR_EXPOSURE_NUM];
	u32 agc[MAX_HDR_EXPOSURE_NUM];
	u32 dgain;
	u32 hdr_enable;
	amba_img_dsp_contrast_enhc_t ce;
	hdr_alpha_value_t alpha_value;
	hdr_threshold_t threshold_value;
}BATCH_CMD_PKG;


void feed_batch_cmd_pkg(BATCH_CMD_PKG* batch_cmd_pkg,hdr_proc_data_t* hdr_proc_data,int expo_num)
{
	//1.alpha_lut
	int i=0;
	int offset =0;
	for(i=0;i<expo_num-1; i++){
		memcpy(batch_cmd_pkg->alpha_lut+offset,(u8*)hdr_proc_data->prebld_alpha[i].alpha_table_addr,AMBA_DSP_IMG_ALPHA_LUT_SIZE);
		offset+= AMBA_DSP_IMG_ALPHA_LUT_SIZE;
	}

	//2.alpha blc
//	memcpy((u8*)&batch_cmd_pkg->prebld_blc,(u8*)&hdr_proc_data->prebld_blc,sizeof(amba_img_dsp_black_correction_t));

	//3.amp_linear_lut
	offset =0;
	for(i=0;i<expo_num-1; i++){
		if(i ==0){
			memcpy((u8*)batch_cmd_pkg->amp_linear_lut+offset,(u8*)hdr_proc_data->ampli_info[i].amp_linear[0].lut_addr,AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16));
			offset +=AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16);
		}
		memcpy((u8*)batch_cmd_pkg->amp_linear_lut+offset,(u8*)hdr_proc_data->ampli_info[i].amp_linear[1].lut_addr,AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16));
		offset +=AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16);
	}
	//4.ampli_blc
//	memcpy((u8*)batch_cmd_pkg->ampli_blc,(u8*)hdr_proc_data->ampli_blc,(expo_num - 1)*sizeof(amba_img_dsp_black_correction_t));

	//5.wb
	for(i=0;i<expo_num-1; i++){
		if(i==0){
			batch_cmd_pkg->wb_round0_r =hdr_proc_data->ampli_info[0].amp_linear[0].mul_r;
			batch_cmd_pkg->wb_round0_g =hdr_proc_data->ampli_info[0].amp_linear[0].mul_g;
			batch_cmd_pkg->wb_round0_b=hdr_proc_data->ampli_info[0].amp_linear[0].mul_b;
			batch_cmd_pkg->wb_round1_r =hdr_proc_data->ampli_info[0].amp_linear[1].mul_r;
			batch_cmd_pkg->wb_round1_g =hdr_proc_data->ampli_info[0].amp_linear[1].mul_g;
			batch_cmd_pkg->wb_round1_b=hdr_proc_data->ampli_info[0].amp_linear[1].mul_b;
		}else{
			batch_cmd_pkg->wb_round2_r =hdr_proc_data->ampli_info[i].amp_linear[1].mul_r;
			batch_cmd_pkg->wb_round2_g =hdr_proc_data->ampli_info[i].amp_linear[1].mul_g;
			batch_cmd_pkg->wb_round2_b=hdr_proc_data->ampli_info[i].amp_linear[1].mul_b;
		}

	}
	//6.threshold
//	memcpy((u8*)batch_cmd_pkg->prebld_thre,(u8*)hdr_proc_data->prebld_thre,(expo_num - 1)*sizeof(amba_img_dsp_hdr_alpha_calc_thresh_t));
	//7.ae_target

      //8.sensor blc
//      memcpy((u8*)batch_cmd_pkg->sensor_blc,(u8*)hdr_proc_data->sensor_blc,(expo_num-1)*sizeof(amba_img_dsp_black_correction_t));
	for (i = 0; i < expo_num - 1; ++i) {
		batch_cmd_pkg->sensor_blc[i].BlackR = 0;
		batch_cmd_pkg->sensor_blc[i].BlackGr = 0;
		batch_cmd_pkg->sensor_blc[i].BlackGb = 0;
		batch_cmd_pkg->sensor_blc[i].BlackB = 0;
	}

	offset =0;
	for(i=0;i<expo_num-1; i++){
		memcpy(batch_cmd_pkg->alpha_lut+offset,(u8*)hdr_proc_data->prebld_alpha[i].alpha_table_addr,AMBA_DSP_IMG_ALPHA_LUT_SIZE);
		offset+= AMBA_DSP_IMG_ALPHA_LUT_SIZE;
	}
	//9.cnst_enhc
	batch_cmd_pkg->ce.low_pass_radius =hdr_proc_data->cnst_enhc.low_pass_radius;
	batch_cmd_pkg->ce.enhance_gain =hdr_proc_data->cnst_enhc.enhance_gain;

}
void feed_hdr_proc_data(hdr_proc_data_t* hdr_proc_data,BATCH_CMD_PKG* batch_cmd_pkg,int expo_num)
{

	//1.alpha_lut
	int i=0;
	int offset =0;
	static u8 alpha_lut[MAX_HDR_EXPOSURE_NUM-1][AMBA_DSP_IMG_ALPHA_LUT_SIZE];
	static u16 amplinear_lut[MAX_HDR_EXPOSURE_NUM][AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE];

	if(batch_cmd_pkg->hdr_enable ==0){

		for(i=0;i<expo_num -1;i++){
			hdr_proc_data->prebld_alpha[i].alpha_table_addr =(u32)alpha_lut[i];
		}
		for(i=0;i<expo_num-1;i++){
			if(i ==0){
				hdr_proc_data->ampli_info[i].amp_linear[0].lut_addr =(u32)amplinear_lut[0];
				hdr_proc_data->ampli_info[i].amp_linear[1].lut_addr=(u32)amplinear_lut[1];
			}
			else
				hdr_proc_data->ampli_info[i].amp_linear[1].lut_addr=(u32)amplinear_lut[i+1];

		}
		for(i=0;i<expo_num-1; i++){
			memcpy((u8*)hdr_proc_data->prebld_alpha[i].alpha_table_addr,batch_cmd_pkg->alpha_lut+offset,AMBA_DSP_IMG_ALPHA_LUT_SIZE);
			offset+= AMBA_DSP_IMG_ALPHA_LUT_SIZE;
		}
		//2.alpha blc
	//	memcpy((u8*)&hdr_proc_data->prebld_blc,(u8*)&batch_cmd_pkg->prebld_blc,sizeof(amba_img_dsp_black_correction_t));
		//3.amp_linear_lut
		offset =0;
		for(i=0;i<expo_num-1; i++){
			if(i ==0){
				memcpy((u8*)hdr_proc_data->ampli_info[i].amp_linear[0].lut_addr,(u8*)batch_cmd_pkg->amp_linear_lut+offset,AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16));
				offset +=AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16);
			}
			memcpy((u8*)hdr_proc_data->ampli_info[i].amp_linear[1].lut_addr,(u8*)batch_cmd_pkg->amp_linear_lut+offset,AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16));
			offset +=AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE*sizeof(u16);
		}
		//4.ampli_blc
	//	memcpy((u8*)hdr_proc_data->ampli_blc,(u8*)batch_cmd_pkg->ampli_blc,(expo_num-1)*sizeof(amba_img_dsp_black_correction_t));
		//5.wb
		for(i =0;i<MAX_HDR_EXPOSURE_NUM-1;i++){
			if(i ==0){
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_r=batch_cmd_pkg->wb_round0_r;
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_g=batch_cmd_pkg->wb_round0_g;
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_b=batch_cmd_pkg->wb_round0_b;
				hdr_proc_data->ampli_info[i].amp_linear[1].mul_r =batch_cmd_pkg->wb_round1_r;
				hdr_proc_data->ampli_info[i].amp_linear[1].mul_g=batch_cmd_pkg->wb_round1_g;
				hdr_proc_data->ampli_info[i].amp_linear[1].mul_b=batch_cmd_pkg->wb_round1_b;
			}
			else{
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_r=4096;
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_g=4096;
				hdr_proc_data->ampli_info[i].amp_linear[0].mul_b=4096;
				if(i==1){
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_r =batch_cmd_pkg->wb_round2_r;
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_g=batch_cmd_pkg->wb_round2_g;
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_b=batch_cmd_pkg->wb_round2_b;
				}else{
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_r =batch_cmd_pkg->wb_round3_r;
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_g=batch_cmd_pkg->wb_round3_g;
					hdr_proc_data->ampli_info[i].amp_linear[1].mul_b=batch_cmd_pkg->wb_round3_b;
				}
			}
		}
		//6.threshold
	//	memcpy((u8*)hdr_proc_data->prebld_thre,(u8*)batch_cmd_pkg->prebld_thre,(expo_num - 1)*sizeof(amba_img_dsp_hdr_alpha_calc_thresh_t));
		//7.ae_target

		//8.sensor blc
	//	memcpy((u8*)hdr_proc_data->sensor_blc,(u8*)batch_cmd_pkg->sensor_blc,(expo_num-1)*sizeof(amba_img_dsp_black_correction_t));

		//9.ce
		hdr_proc_data->cnst_enhc.low_pass_radius=batch_cmd_pkg->ce.low_pass_radius;
		hdr_proc_data->cnst_enhc.enhance_gain=batch_cmd_pkg->ce.enhance_gain;

		for(i =0;i<expo_num;i++){
	//		hdr_proc_data->prebld_cfg_update[i] =1;
	//		hdr_proc_data->prebld_thre_update[i] =1;
			hdr_proc_data->prebld_alp_update[i] =1;
			hdr_proc_data->ampli_info_update[i] =1;
		}
		hdr_proc_data->ce_update =1;
	}

}


