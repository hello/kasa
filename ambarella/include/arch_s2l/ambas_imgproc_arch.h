/*
 * include/arch_s2l/ambas_imgproc_arch.h
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

#ifndef __AMBAS_IMGPROC_ARCH_H__
#define __AMBAS_IMGPROC_ARCH_H__

#define	INVALID_AAA_DATA_ENTRY		(0xFF)
#define	MAX_AAA_DATA_NUM			(10)
#define	MAX_HDR_DATA_NUM			(12)
#define	RGB_AAA_DATA_BLOCK			(2176)
#define	CFA_AAA_DATA_BLOCK			(12672)
#define	CFA_PRE_HDR_BLOCK				(1664)
#define	RGB_AAA_DATA_BLOCK_ARRAY		(RGB_AAA_DATA_BLOCK * MAX_AAA_DATA_NUM)
#define	CFA_AAA_DATA_BLOCK_ARRAY		(CFA_AAA_DATA_BLOCK * MAX_AAA_DATA_NUM)
#define	CFA_PRE_HDR_BLOCK_ARRAY			(CFA_PRE_HDR_BLOCK * MAX_HDR_DATA_NUM)
#define	INPUT_LOOK_UP_TABLE_SIZE		(192*3*4)
#define	MATRIX_TABLE_SIZE			(16*16*16*4)

#define	MAX_EXPOSURE_NUM		(4)

#define	SENSOR_HIST_ROW_NUM		(2)
#define	SENSOR_HIST_DATA_PITCH	(2080 * 2)
#define	SENSOR_HIST_DATA_BLOCK	(SENSOR_HIST_DATA_PITCH * SENSOR_HIST_ROW_NUM * 2)

/*
struct aaa_tile_info {
	u16 awb_tile_col_start;
	u16 awb_tile_row_start;
	u16 awb_tile_width;
	u16 awb_tile_height;
	u16 awb_tile_active_width;
	u16 awb_tile_active_height;
	u16 awb_rgb_shift;
	u16 awb_y_shift;
	u16 awb_min_max_shift;
	u16 ae_tile_col_start;
	u16 ae_tile_row_start;
	u16 ae_tile_width;
	u16 ae_tile_height;
	u16 ae_y_shift;
	u16 ae_linear_y_shift;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	u16 af_y_shift;
	u16 af_cfa_y_shift;
	u16  awb_tile_num_col;
	u16  awb_tile_num_row;
	u16  ae_tile_num_col;
	u16  ae_tile_num_row;
	u16  af_tile_num_col;
	u16  af_tile_num_row;
	u8 total_slices_x;
	u8 total_slices_y;
	u8 slice_index_x;
	u8 slice_index_y;
	u16 slice_width;
	u16 slice_height;
	u16 slice_start_x;
	u16 slice_start_y;
	u16  exposure_index;
	u16  total_exposure;
	u16 reserved[31];
};
*/
struct aaa_tile_info {
    // 1 u32
	u16 awb_tile_col_start;
	u16 awb_tile_row_start;
    // 2 u32
	u16 awb_tile_width;
	u16 awb_tile_height;
    // 3 u32
	u16 awb_tile_active_width;
	u16 awb_tile_active_height;
    //4 u32
	u16 awb_rgb_shift;
	u16 awb_y_shift;
    //5 u32
	u16 awb_min_max_shift;
	u16 ae_tile_col_start;
    //6 u32
	u16 ae_tile_row_start;
	u16 ae_tile_width;
    //7 u32
	u16 ae_tile_height;
	u16 ae_y_shift;
    //8 u32
	u16 ae_linear_y_shift;
	u16 ae_min_max_shift;

    //9 u32
	u16 af_tile_col_start;
	u16 af_tile_row_start;

    //10 u32
	u16 af_tile_width;
	u16 af_tile_height;

    //11 u32
	u16 af_tile_active_width;
	u16 af_tile_active_height;

    //12 u32
	u16 af_y_shift;
	u16 af_cfa_y_shift;

    //13 u32
	u8  awb_tile_num_col;
	u8  awb_tile_num_row;
	u8  ae_tile_num_col;
	u8  ae_tile_num_row;

    //14 u32
	u8  af_tile_num_col;
	u8  af_tile_num_row;
	u8 total_exposures;
	u8 exposure_index;

    //15 u32
	u8 total_slices_x;
	u8 total_slices_y;
	u8 slice_index_x;
	u8 slice_index_y;

    //16 u32
	u16 slice_width;
	u16 slice_height;

    //17 u32
	u16 slice_start_x;
	u16 slice_start_y;

	u16 black_red;
	u16 black_green;
	u16 black_blue;
	u16 reserved[27];
};

struct cfa_hdr_cfg_info
{
  u8  vin_stats_type; // 0: main; 1: hdr
  u8  reserved_0;
  u8  total_exposures;
  u8  blend_index;    // exposure no.

  u8  total_slice_in_x;
  u8  slice_index_x;
  u8  total_slice_in_y;
  u8  slice_index_y;

  u16 vin_stats_slice_left;
  u16 vin_stats_slice_width;

  u16 vin_stats_slice_top;
  u16 vin_stats_slice_height;

  u32 reserved[28]; // pad to 128B

} ;

struct af_stat {
	u16 sum_fy;
	u16 sum_fv1;
	u16 sum_fv2;
};

struct rgb_histogram_stat {
	u32 his_bin_y[64];
	u32 his_bin_r[64];
	u32 his_bin_g[64];
	u32 his_bin_b[64];
};

struct rgb_aaa_stat {
	struct aaa_tile_info	aaa_tile_info;
	u16			frame_id;
	struct af_stat		af_stat[96];
	u16			ae_sum_y[96];
	struct rgb_histogram_stat	histogram_stat;
	u8			reserved[249];
};


struct cfa_awb_stat {
	u16	sum_r;
	u16	sum_g;
	u16	sum_b;
	u16	count_min;
	u16	count_max;
};

struct cfa_ae_stat {
	u16	lin_y;
	u16	count_min;
	u16	count_max;
};

struct cfa_af_stat {
	u16	sum_fy;
	u16	sum_fv1;
	u16	sum_fv2;
};

struct cfa_histogram_stat {
	u32	his_bin_r[64];
	u32	his_bin_g[64];
	u32	his_bin_b[64];
	u32	his_bin_y[64];
};

/* for A5 = 12288, A5S = 12416 */
struct cfa_aaa_stat {
	struct aaa_tile_info	aaa_tile_info;
	u16       frame_id;
	struct cfa_awb_stat	awb_stat[1024];
	struct cfa_ae_stat	ae_stat[96];
	struct af_stat		af_stat[96];
	struct cfa_histogram_stat	histogram_stat;
	u8			reserved[121];
};

struct cfa_hdr_hist_stat{
	u32 his_bin_r[128];
	u32 his_bin_g[128];
	u32 his_bin_b[128];
};

struct cfa_pre_hdr_stat{
	struct cfa_hdr_cfg_info cfg_info;
	struct cfa_hdr_hist_stat cfa_hist;
};

struct contrast_enhance_info{
	u16	low_pass_radius;	// 0: 16x, 1: 32x, 2: 60x
	u16	enhance_gain;	// unit: 1024; 0 : off
};

/*
typedef  struct cfa_aaa_stat_s {
 struct aaa_tile_info_t    aaa_tile_info;
 u16       frame_id;
 struct cfa_awb_stat_t    awb_stat[1024];
 struct cfa_ae_stat_t    ae_stat[96];
 struct af_stat_t     af_stat[96];
 struct cfa_histogram_stat_t histogram_stat;
 u8  reserved[121];

 //ARM SIZE
 a5s_cfa_histogram_stat_t *histogram_stat_ch1;
 u8 plview_delay_cnt;
}  cfa_aaa_stat_t;
*/

struct img_statistics {
	u8	*rgb_data_valid;
	u8	*cfa_data_valid;
	u8	*hist_data_valid;
	u8 	*cfa_hdr_valid;
	void	*rgb_statis;
	void	*cfa_statis;
	void	*hist_statis;
	void	*cfa_hdr_statis;
	u32	*dsp_pts_data_addr;
	u64	*mono_pts_data_addr;
};


struct aaa_statistics_ex {
	u8 af_horizontal_filter1_mode;
	u8 af_horizontal_filter1_stage1_enb;
	u8 af_horizontal_filter1_stage2_enb;
	u8 af_horizontal_filter1_stage3_enb;
	s16 af_horizontal_filter1_gain[7];
	u16 af_horizontal_filter1_shift[4];
	u16 af_horizontal_filter1_bias_off;
	u16 af_horizontal_filter1_thresh;
	u16 af_vertical_filter1_thresh;
	u16 af_tile_fv1_horizontal_shift;
	u16 af_tile_fv1_vertical_shift;
	u16 af_tile_fv1_horizontal_weight;
	u16 af_tile_fv1_vertical_weight;

	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	s16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_horizontal_filter2_thresh;
	u16 af_vertical_filter2_thresh;
	u16 af_tile_fv2_horizontal_shift;
	u16 af_tile_fv2_vertical_shift;
	u16 af_tile_fv2_horizontal_weight;
	u16 af_tile_fv2_vertical_weight;
};

#define DUMP_IDSP_0		0
#define DUMP_IDSP_1		1
#define DUMP_IDSP_2		2
#define DUMP_IDSP_3		3
#define DUMP_IDSP_4		4
#define DUMP_IDSP_5		5
#define DUMP_IDSP_6		6
#define DUMP_IDSP_7		7
#define DUMP_IDSP_100		100
#define DUMP_FPN		200
#define DUMP_VIGNETTE		201
#define MAX_DUMP_BUFFER_SIZE	256*1024

typedef struct idsp_config_info_s {
	u8*		addr;
	u32		addr_long;
	u32		id_section;
}idsp_config_info_t;


struct raw2enc_raw_feed_info
{
	u32 daddr_offset;		/* offset from sensor raw start daddr to the start address of ik */
	u32 raw_size;	/* offset from sensor_raw_start_daddr to the start address of the next frame in DRAM */
	u32 dpitch : 16;		/* buffer pitch in bytes */
	u32 num_frames : 8;	/* num of frames stored in DRAM starting from sensor_raw_start_daddr */
	u32 reserved : 8;
};

typedef struct idsp_hiso_config_update_flag_s {
	u32 hiso_config_common_update : 1 ;
	u32 hiso_config_color_update : 1 ;
	u32 hiso_config_mctf_update : 1 ;
	u32 hiso_config_step1_update : 1 ;
	u32 hiso_config_step2_update : 1 ;
	u32 hiso_config_step3_update : 1 ;
	u32 hiso_config_step4_update : 1 ;
	u32 hiso_config_step5_update : 1 ;
	u32 hiso_config_step6_update : 1 ;
	u32 hiso_config_step7_update : 1 ;
	u32 hiso_config_step8_update : 1 ;
	u32 hiso_config_step9_update : 1 ;
	u32 hiso_config_step10_update : 1 ;
	u32 hiso_config_step11_update : 1 ;
	u32 hiso_config_step15_update : 1 ;
	u32 hiso_config_step16_update : 1 ;
	u32 hiso_config_vwarp_update : 1 ;
	u32 hiso_config_flow_update : 1 ;
	u32 hiso_config_aaa_update : 1 ; // AAA setup update
	u32 reserved : 13 ;
} idsp_hiso_config_update_flag_t;

typedef struct idsp_hiso_config_update_s {
	u32 hiso_param_daddr;

	/* config update flags */
	union {
		idsp_hiso_config_update_flag_t flag;
		u32 word;
	} loadcfg_type;
} idsp_hiso_config_update_t;

#endif

