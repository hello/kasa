/*
 *
 * mw_dynamic_tonecurve_proc.c
 *
 * History:
 *	2016/5/27 - [Jian Tang] Created this file
 *
 * Copyright (C) 2016 Ambarella, Inc.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include <pthread.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_fastboot.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "ambas_imgproc_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "img_dev.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"
#include "mw_ir_led.h"

#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"
#include "img_customer_interface_arch.h"

extern _mw_global_config G_mw_config;
u8 G_dynamic_tone_curve_active = 0;

static amba_img_dsp_tone_curve_t cus_dyn_tone;
static u16 cus_histo_info[64];	//for pre-HDR is 128, else are 64

//2xHDR tone 0%
static amba_img_dsp_tone_curve_t cus_dyn_tone_2xHDR_0 = {
	.ToneCurveRed = {
		  0,   9,  14,  20,  26,  33,  39,  44,  51,  56,
		 64,  69,  78,  83,  89,  97, 103, 109, 116, 122,
		128, 134, 141, 147, 154, 161, 167, 174, 181, 188,
		195, 202, 206, 213, 220, 227, 231, 238, 245, 253,
		257, 264, 271, 275, 283, 290, 294, 302, 309, 313,
		321, 328, 332, 340, 347, 351, 359, 362, 370, 374,
		381, 389, 393, 400, 404, 412, 416, 423, 427, 434,
		442, 445, 453, 457, 464, 468, 475, 478, 486, 489,
		496, 500, 507, 511, 517, 521, 528, 531, 538, 541,
		548, 551, 558, 564, 568, 574, 577, 583, 586, 593,
		596, 602, 604, 610, 613, 619, 622, 627, 633, 635,
		640, 643, 648, 651, 656, 658, 663, 666, 671, 673,
		678, 681, 683, 688, 691, 696, 698, 701, 706, 708,
		711, 715, 718, 720, 725, 728, 730, 732, 737, 740,
		742, 744, 749, 751, 754, 756, 761, 763, 765, 768,
		770, 775, 777, 779, 781, 786, 788, 791, 793, 795,
		800, 802, 804, 806, 808, 811, 815, 817, 819, 821,
		824, 828, 830, 832, 834, 836, 839, 843, 845, 847,
		849, 851, 855, 857, 859, 861, 863, 867, 869, 871,
		873, 875, 879, 881, 883, 885, 887, 891, 893, 894,
		896, 900, 902, 904, 906, 909, 911, 913, 915, 918,
		920, 922, 925, 927, 929, 932, 934, 935, 939, 940,
		942, 945, 947, 950, 952, 953, 956, 958, 961, 962,
		965, 967, 970, 971, 974, 976, 978, 981, 982, 985,
		988, 990, 992, 994, 997, 999,1002,1004,1006,1009,
		1011,1014,1016,1019,1021,1023
	},
	.ToneCurveGreen = {
		  0,   9,  14,  20,  26,  33,  39,  44,  51,  56,
		 64,  69,  78,  83,  89,  97, 103, 109, 116, 122,
		128, 134, 141, 147, 154, 161, 167, 174, 181, 188,
		195, 202, 206, 213, 220, 227, 231, 238, 245, 253,
		257, 264, 271, 275, 283, 290, 294, 302, 309, 313,
		321, 328, 332, 340, 347, 351, 359, 362, 370, 374,
		381, 389, 393, 400, 404, 412, 416, 423, 427, 434,
		442, 445, 453, 457, 464, 468, 475, 478, 486, 489,
		496, 500, 507, 511, 517, 521, 528, 531, 538, 541,
		548, 551, 558, 564, 568, 574, 577, 583, 586, 593,
		596, 602, 604, 610, 613, 619, 622, 627, 633, 635,
		640, 643, 648, 651, 656, 658, 663, 666, 671, 673,
		678, 681, 683, 688, 691, 696, 698, 701, 706, 708,
		711, 715, 718, 720, 725, 728, 730, 732, 737, 740,
		742, 744, 749, 751, 754, 756, 761, 763, 765, 768,
		770, 775, 777, 779, 781, 786, 788, 791, 793, 795,
		800, 802, 804, 806, 808, 811, 815, 817, 819, 821,
		824, 828, 830, 832, 834, 836, 839, 843, 845, 847,
		849, 851, 855, 857, 859, 861, 863, 867, 869, 871,
		873, 875, 879, 881, 883, 885, 887, 891, 893, 894,
		896, 900, 902, 904, 906, 909, 911, 913, 915, 918,
		920, 922, 925, 927, 929, 932, 934, 935, 939, 940,
		942, 945, 947, 950, 952, 953, 956, 958, 961, 962,
		965, 967, 970, 971, 974, 976, 978, 981, 982, 985,
		988, 990, 992, 994, 997, 999,1002,1004,1006,1009,
		1011,1014,1016,1019,1021,1023
	},
	.ToneCurveBlue = {
		  0,   9,  14,  20,  26,  33,  39,  44,  51,  56,
		 64,  69,  78,  83,  89,  97, 103, 109, 116, 122,
		128, 134, 141, 147, 154, 161, 167, 174, 181, 188,
		195, 202, 206, 213, 220, 227, 231, 238, 245, 253,
		257, 264, 271, 275, 283, 290, 294, 302, 309, 313,
		321, 328, 332, 340, 347, 351, 359, 362, 370, 374,
		381, 389, 393, 400, 404, 412, 416, 423, 427, 434,
		442, 445, 453, 457, 464, 468, 475, 478, 486, 489,
		496, 500, 507, 511, 517, 521, 528, 531, 538, 541,
		548, 551, 558, 564, 568, 574, 577, 583, 586, 593,
		596, 602, 604, 610, 613, 619, 622, 627, 633, 635,
		640, 643, 648, 651, 656, 658, 663, 666, 671, 673,
		678, 681, 683, 688, 691, 696, 698, 701, 706, 708,
		711, 715, 718, 720, 725, 728, 730, 732, 737, 740,
		742, 744, 749, 751, 754, 756, 761, 763, 765, 768,
		770, 775, 777, 779, 781, 786, 788, 791, 793, 795,
		800, 802, 804, 806, 808, 811, 815, 817, 819, 821,
		824, 828, 830, 832, 834, 836, 839, 843, 845, 847,
		849, 851, 855, 857, 859, 861, 863, 867, 869, 871,
		873, 875, 879, 881, 883, 885, 887, 891, 893, 894,
		896, 900, 902, 904, 906, 909, 911, 913, 915, 918,
		920, 922, 925, 927, 929, 932, 934, 935, 939, 940,
		942, 945, 947, 950, 952, 953, 956, 958, 961, 962,
		965, 967, 970, 971, 974, 976, 978, 981, 982, 985,
		988, 990, 992, 994, 997, 999,1002,1004,1006,1009,
		1011,1014,1016,1019,1021,1023
	},
};

//2xHDR tone 100%
static amba_img_dsp_tone_curve_t cus_dyn_tone_2xHDR_100 = {
	.ToneCurveRed = {
		  0,   1,   1,   2,   2,   3,   3,   4,   4,   5,
		 11,  15,  19,  24,  27,  32,  36,  41,  45,  49,
		 53,  58,  62,  66,  71,  74,  79,  83,  87,  92,
		 97, 100, 105, 109, 112, 117, 121, 126, 130, 134,
		138, 143, 147, 151, 155, 159, 165, 169, 173, 177,
		181, 185, 189, 193, 198, 202, 206, 210, 215, 219,
		224, 228, 232, 237, 239, 244, 248, 253, 257, 262,
		264, 269, 273, 278, 283, 285, 290, 295, 299, 302,
		307, 311, 316, 319, 324, 328, 331, 336, 341, 343,
		348, 353, 356, 361, 366, 368, 373, 378, 381, 386,
		389, 394, 400, 402, 405, 411, 414, 419, 422, 428,
		431, 434, 440, 443, 447, 453, 456, 459, 463, 469,
		472, 476, 479, 486, 489, 492, 496, 499, 506, 510,
		513, 517, 520, 524, 531, 534, 538, 541, 545, 549,
		552, 560, 563, 567, 571, 574, 578, 582, 585, 589,
		593, 600, 604, 608, 612, 615, 619, 623, 627, 630,
		634, 638, 642, 649, 653, 657, 661, 665, 669, 672,
		676, 680, 684, 688, 691, 695, 703, 707, 710, 714,
		718, 722, 725, 729, 733, 737, 741, 744, 748, 756,
		759, 763, 767, 770, 774, 778, 781, 785, 792, 796,
		800, 803, 807, 810, 814, 818, 825, 828, 832, 835,
		839, 846, 849, 852, 856, 859, 866, 869, 873, 876,
		879, 886, 889, 892, 898, 901, 905, 911, 914, 917,
		923, 926, 929, 934, 937, 943, 946, 951, 954, 959,
		962, 967, 972, 974, 979, 984, 988, 990, 995, 999,
		1003,1007,1013,1016,1020,1023
	},
	.ToneCurveGreen = {
		  0,   1,   1,   2,   2,   3,   3,   4,   4,   5,
		 11,  15,  19,  24,  27,  32,  36,  41,  45,  49,
		 53,  58,  62,  66,  71,  74,  79,  83,  87,  92,
		 97, 100, 105, 109, 112, 117, 121, 126, 130, 134,
		138, 143, 147, 151, 155, 159, 165, 169, 173, 177,
		181, 185, 189, 193, 198, 202, 206, 210, 215, 219,
		224, 228, 232, 237, 239, 244, 248, 253, 257, 262,
		264, 269, 273, 278, 283, 285, 290, 295, 299, 302,
		307, 311, 316, 319, 324, 328, 331, 336, 341, 343,
		348, 353, 356, 361, 366, 368, 373, 378, 381, 386,
		389, 394, 400, 402, 405, 411, 414, 419, 422, 428,
		431, 434, 440, 443, 447, 453, 456, 459, 463, 469,
		472, 476, 479, 486, 489, 492, 496, 499, 506, 510,
		513, 517, 520, 524, 531, 534, 538, 541, 545, 549,
		552, 560, 563, 567, 571, 574, 578, 582, 585, 589,
		593, 600, 604, 608, 612, 615, 619, 623, 627, 630,
		634, 638, 642, 649, 653, 657, 661, 665, 669, 672,
		676, 680, 684, 688, 691, 695, 703, 707, 710, 714,
		718, 722, 725, 729, 733, 737, 741, 744, 748, 756,
		759, 763, 767, 770, 774, 778, 781, 785, 792, 796,
		800, 803, 807, 810, 814, 818, 825, 828, 832, 835,
		839, 846, 849, 852, 856, 859, 866, 869, 873, 876,
		879, 886, 889, 892, 898, 901, 905, 911, 914, 917,
		923, 926, 929, 934, 937, 943, 946, 951, 954, 959,
		962, 967, 972, 974, 979, 984, 988, 990, 995, 999,
		1003,1007,1013,1016,1020,1023
	},
	.ToneCurveBlue = {
		  0,   1,   1,   2,   2,   3,   3,   4,   4,   5,
		 11,  15,  19,  24,  27,  32,  36,  41,  45,  49,
		 53,  58,  62,  66,  71,  74,  79,  83,  87,  92,
		 97, 100, 105, 109, 112, 117, 121, 126, 130, 134,
		138, 143, 147, 151, 155, 159, 165, 169, 173, 177,
		181, 185, 189, 193, 198, 202, 206, 210, 215, 219,
		224, 228, 232, 237, 239, 244, 248, 253, 257, 262,
		264, 269, 273, 278, 283, 285, 290, 295, 299, 302,
		307, 311, 316, 319, 324, 328, 331, 336, 341, 343,
		348, 353, 356, 361, 366, 368, 373, 378, 381, 386,
		389, 394, 400, 402, 405, 411, 414, 419, 422, 428,
		431, 434, 440, 443, 447, 453, 456, 459, 463, 469,
		472, 476, 479, 486, 489, 492, 496, 499, 506, 510,
		513, 517, 520, 524, 531, 534, 538, 541, 545, 549,
		552, 560, 563, 567, 571, 574, 578, 582, 585, 589,
		593, 600, 604, 608, 612, 615, 619, 623, 627, 630,
		634, 638, 642, 649, 653, 657, 661, 665, 669, 672,
		676, 680, 684, 688, 691, 695, 703, 707, 710, 714,
		718, 722, 725, 729, 733, 737, 741, 744, 748, 756,
		759, 763, 767, 770, 774, 778, 781, 785, 792, 796,
		800, 803, 807, 810, 814, 818, 825, 828, 832, 835,
		839, 846, 849, 852, 856, 859, 866, 869, 873, 876,
		879, 886, 889, 892, 898, 901, 905, 911, 914, 917,
		923, 926, 929, 934, 937, 943, 946, 951, 954, 959,
		962, 967, 972, 974, 979, 984, 988, 990, 995, 999,
		1003,1007,1013,1016,1020,1023
	}
};

static struct rgb_aaa_stat rgb_stat[MAX_SLICE_NUM];
static struct cfa_aaa_stat cfa_stat[MAX_SLICE_NUM];
static struct cfa_pre_hdr_stat hdr_cfa_pre_stat[MAX_PRE_HDR_STAT_BLK_NUM];
static u8 rgb_stat_valid;
static u8 cfa_stat_valid;
static u8 hdr_cfa_pre_valid;

extern int img_adj_set_filter_adj_argu(s8 mode, filter_id id);
extern int coll_transmit_filter(void *filter, void *cmd, void *usr1, void *usr2);

static int prepare_to_get_statistics(amba_dsp_aaa_statistic_data_t *stat)
{
	memset(rgb_stat, 0, sizeof(rgb_stat));
	memset(cfa_stat, 0, sizeof(cfa_stat));
	memset(hdr_cfa_pre_stat, 0, (sizeof(struct cfa_pre_hdr_stat) * MAX_PRE_HDR_STAT_BLK_NUM));
	rgb_stat_valid = 0;
	cfa_stat_valid = 0;
	hdr_cfa_pre_valid = 0;
	memset(stat, 0, sizeof(amba_dsp_aaa_statistic_data_t));

	stat->CfaAaaDataAddr = (u32)cfa_stat;
	stat->RgbAaaDataAddr = (u32)rgb_stat;
	stat->CfaPreHdrDataAddr = (u32)hdr_cfa_pre_stat;
	stat->CfaDataValid = (u32)&cfa_stat_valid;
	stat->RgbDataValid = (u32)&rgb_stat_valid;
	stat->CfaPreHdrDataValid = (u32)&hdr_cfa_pre_valid;

	return 0;
}

static void get_ae_histo(u32 *p_aaa_Histo_g)
{
	int i, bin_count;
	long long sum, tmp;

	sum = 0;
	bin_count = 64;
	for(i = 0; i < bin_count; i++){
		sum += p_aaa_Histo_g[i];
	}
	memset(cus_histo_info, 0, sizeof(u16) * bin_count);
	if(sum > 0) {
		for (i = 0; i < bin_count; i++) {
			tmp = ((long long)p_aaa_Histo_g[i] * 4096) / sum;
			cus_histo_info[i] = (u16)tmp;
		}
	}
}

static u16 get_ae_histo_sum(u16 srt, u16 end)
{
	u16 sum_tmp, i;

	sum_tmp = 0;
	for (i = srt; i <= end; i++) {
	sum_tmp += cus_histo_info[i];
	}

	return (sum_tmp);
}

static u8 dynamic_calc_tone_curve(img_aaa_stat_t *aaa_stat, u8 expo_num)
{
	u8 ret = 0;
	u16 histo_sum;
	u16 Th0 = 10;
	u16	Th1 =200;

	u16 i, w_ratio0 = 100, w_ratio100 = 0, w_sum = 100;
	static u16 pre_ratio0 = 0;

	//check CFA-histo
	get_ae_histo(aaa_stat->rgb_hist.his_bin_y);
	histo_sum = get_ae_histo_sum(49, 63);	//normal v.s. HDR scene

	switch (expo_num) {
	case 1:
		MW_MSG("Linear mode not support dynamic tone curve!\n");
		break;
	case 2:
	    if (histo_sum <= Th0) {
			w_ratio0 = 100;
			w_ratio100 = 0;
			w_sum = 100;
		} else if (histo_sum >= Th1) {
			w_ratio0 = 0;
			w_ratio100 = 100;
			w_sum = 100;
		} else {
			w_ratio0 = (Th1 - histo_sum) * 100 / (Th1 - Th0);
			w_ratio100 = (histo_sum - Th0) * 100 / (Th1 - Th0);
			w_sum = 100;
		}

		if ((w_ratio0 - pre_ratio0) > 4) {
			w_ratio0 = pre_ratio0 + 3;
			w_ratio100 = 100 - w_ratio0;
			if(w_ratio100 < 0){
				w_ratio0 = 100;
				w_ratio100 = 0;
			}
		} else if ((w_ratio0 - pre_ratio0) < -4) {
			w_ratio0 = pre_ratio0 - 3;
			w_ratio100 = 100 - w_ratio0;
			if(w_ratio100 > 100){
				w_ratio0 = 0;
				w_ratio100 = 100;
			}
		}
		w_sum = w_ratio0+w_ratio100;


		for (i = 0; i < 256; i++) {
			cus_dyn_tone.ToneCurveGreen[i] = (cus_dyn_tone_2xHDR_0.ToneCurveGreen[i]*w_ratio0 +
				(cus_dyn_tone_2xHDR_100.ToneCurveGreen[i] * w_ratio100)) / w_sum;
		}
		memcpy(cus_dyn_tone.ToneCurveRed, cus_dyn_tone.ToneCurveGreen, sizeof(u16) * 256);
		memcpy(cus_dyn_tone.ToneCurveBlue, cus_dyn_tone.ToneCurveGreen, sizeof(u16) * 256);

		pre_ratio0 = (w_ratio0 * 100) / w_sum;
		ret = 1;
		break;
	case 3:
		MW_MSG("Linear mode not support dynamic tone curve!\n");
		break;
	}

	return ret;
}

void tone_curve_task(void *arg)
{
	u8 expo_num, tone_update;
	u8 frame_skip = 0;
	amba_dsp_aaa_statistic_data_t g_stat;
	amba_img_dsp_mode_cfg_t dsp_mode_cfg;
	img_aaa_stat_t aaa_stat_info[MAX_HDR_EXPOSURE_NUM];
	aaa_tile_report_t act_tile;

	memset(&dsp_mode_cfg, 0, sizeof(dsp_mode_cfg));
	if (get_dsp_mode_cfg(&dsp_mode_cfg) < 0) {
		return;
	}
	prepare_to_get_statistics(&g_stat);
	/* stop tone curve adj */
	img_adj_set_filter_adj_argu(-1, ADJ_TONE);

	while (G_dynamic_tone_curve_active == 1) {
		if (G_mw_config.ae_params.tone_curve_duration > 0) {
			frame_skip = G_mw_config.ae_params.tone_curve_duration - 1;
			if (frame_skip != 0) {
				wait_irq_count(frame_skip);
			}
			if (amba_img_dsp_3a_get_aaa_stat(G_mw_config.fd, &dsp_mode_cfg, &g_stat) < 0) {
				MW_MSG("amba_img_dsp_3a_get_aaa_stat fail\n");
				continue;
			}
			if (parse_aaa_data(&g_stat, G_mw_config.res.hdr_mode, aaa_stat_info, &act_tile) < 0) {
				MW_MSG("parse_aaa_data fail\n");
				continue;
			}

			expo_num = G_mw_config.res.hdr_expo_num;
			tone_update = dynamic_calc_tone_curve(&aaa_stat_info[0], expo_num);
			if(tone_update) {
				filter_header_t f_header;
				f_header.filter_id = ADJ_TONE;
				coll_transmit_filter((void*)&f_header, (void *)&cus_dyn_tone, (void *)G_mw_config.fd, (void *)&dsp_mode_cfg);
			}
		}
	}
}

