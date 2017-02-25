/*
 * kernel/private/include/arch_s2l/amba_arch_vout.h
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
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


#ifndef __AMBA_ARCH_VOUT_H
#define __AMBA_ARCH_VOUT_H

#include "dsp_cmd_msg.h"

/******************************** DVE *****************************************/
typedef union {
	struct {
	u32 t_reset_fsc    :  1; /* [0]  */
#define RST_PHASE_ACCUMULATOR_DIS       0
#define RST_PHASE_ACCUMULATOR_ENA       1

	u32 t_offset_phase :  1; /* [1] */
#define PHASE_OFFSET_DIS                0
#define PHASE_OFFSET_ENA                1

	u32 v_invert       :  1; /* [2] */
	u32 u_invert       :  1; /* [3] */
#define INVERT_COMP_DIS                 0
#define INVERT_COMP_ENA	                1

	u32 reserved       :  4; /* [7:6] */
	u32 unused         : 24; /*[31:8] */
	} s;
	u32 w;
} dve40_t;

typedef union {
	struct {
	u32 t_ygain_val   :  1; /* [0] */
	u32 t_sel_ylpf    :  1; /* [1] Luma LPF select */
	u32 t_ydel_adj    :  3; /* [4:2] Adjust Y delay */
	u32 y_colorbar_en :  1; /* [5] Enable internal color bar gen */
	u32 y_interp      :  2; /* [7:6] choose luma interpolatino mode */
	u32 unused        : 24; /* [31:8] */
	} s;
	u32 w;
} dve46_t;

typedef union {
	struct {
	u32 pwr_dwn_c_dac  :  1; /* [0] */
	u32 pwr_dwn_y_dac  :  1; /* [1] */
	u32 pwr_dwn_cv_dac :  1; /* [2] */
	u32 sel_yuv        :  1; /* [3] */
	u32 reserved       :  4; /* [7:4] */
	u32 unused         : 24; /* [31:8] */
	} s;
	u32 w;
} dve47_t;

typedef union {
	struct {
	u32 sel_c_gain     :  1; /* [0] */
	u32 pal_c_lpf      :  1; /* [1] */
	u32 reserved       :  4; /* [7:4] */
	u32 unused         : 24; /* [31:8] */
	} s;
	u32 w;
} dve52_t;

typedef union {
	struct {
	u32 y_tencd_mode  :  3; /* [2:0] select TV standard */
	u32 y_tsyn_mode   :  3; /* [5:3] master/slave/timecode ysnc
			         	 mode and input format select */
	u32 t_vsync_phs   :  1; /* [6] select phase of Vsync in/out */
	u32 t_hsync_phs   :  1; /* [7] select phase of hsync in/out */
	u32 unused        : 24; /* [31:8] */
	} s;
	u32 w;
} dve56_t;

typedef union {
	struct {
	u32 vso          :  2; /* [1:0] vertical sync offset MSBs */
	u32 unused       :  2; /* [3:2] */
	u32 t_psync_phs  :  1; /* [4] select phase of field sync in/out */
	u32 t_psync_enb  :  1; /* [5] enable ext_vsync_in as
			      	      field sync input */
	u32 clk_phs      :  2; /* [7:6] 6.75/13.5 MHz phase adjust */
	u32 unused2      : 24; /* [31:8] */
	} s;
	u32 w;
} dve57_t;

typedef union {
	struct {
	u32 cs_ln       :  2; /* [1:0] */
	u32 cs_num      :  3; /* [4:2] */
	u32 cs_sp       :  3; /* [7:5] */
	u32 unused      : 24; /* [31:8] */
	} s;
	u32 w;
} dve77_t;

typedef union {
	struct {
	u32 bst_zone_sw2 :  4; /* [3:0] */
	u32 bst_zone_sw1 :  4; /* [7:4] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve88_t;

typedef union {
	struct {
	u32 bz_invert_en :  3; /* [2:0] */
	u32 adv_bs_en    :  1; /* [3] */
	u32 bz3_end      :  4; /* [7:4] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve89_t;

typedef union {
	struct {
	u32 fsc_tst_en   :  1; /* [0] */
	u32 sel_sin      :  1; /* [1] */
	u32 reserved     :  6; /* [7:2] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve96_t;

typedef union {
	struct {
	u32 dig_out_en  :  2; /* [1:0] */
	u32 sel_dac_tst :  1; /* [2] */
	u32 sin_cos_en  :  1; /* [3] */
	u32 ygain_off   :  1; /* [4] */
	u32 sel_y_lpf   :  1; /* [5] */
	u32 byp_y_ups   :  1; /* [6] */
	u32 reserved    :  1; /* [7] */
	u32 unused      : 24; /* [31:8] */
	} s;
	u32 w;
} dve97_t;

typedef union {
	struct {
	u32 cgain_off    :  1; /* [0] */
	u32 byp_c_lpf    :  1; /* [1] */
	u32 byp_c_ups    :  1; /* [2] */
	u32 reserved     :  5; /* [7:3] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve99_t;

//read only
typedef union {
	struct {
	u32 ed_stat_full :  1; /* [0] */
	u32 cc_stat_full :  1; /* [1] */
	u32 reserved     :  6; /* [7:2] */
	u32 unused       : 24; /* [31:8] */
	} s;
	u32 w;
} dve128_t;

typedef struct {
	u32		unsued1[32];
	u32		phi_7_0;
	u32		phi_15_8;
	u32		phi_16_23;
	u32		phi_24_31;
	u32		sctoh_7_0;
	u32		sctoh_15_8;
	u32		sctoh_23_16;
	u32		sctoh_31_24;
	dve40_t		dve_40;
	u32		unused2;
	u32		black_lvl;
	u32		blank_lvl;
	u32		clamp_lvl;
	u32		sync_lvl;
	dve46_t		dve_46;
	dve47_t		dve_47;
	u32		unused3[2];
	u32		nba;
	u32		pba;
	dve52_t		dve_52;
	u32		unused4[3];
	dve56_t		dve_56;
	dve57_t		dve_57;
	u32		vso_7_0;
	u32		hso_10_8;
	u32		hso_7_0;
	u32		hcl_9_8;
	u32		hcl_7_0;
	u32		unused5[2];
	u32		ccd_odd_15_8;
	u32		ccd_odd_7_0;
	u32		ccd_even_15_8;
	u32		ccd_even_7_0;
	u32		cc_enbl;
	u32		unused6[2];
	u32		mvfcr;
	u32		mvcsl1_5_0;
	u32		mvcls1_5_0;
	u32		mvcsl2_5_0;
	u32		mvcls2_5_0;
	dve77_t		dve_77;
	u32		mvpsd_5_0;
	u32		mvpsl_5_0;
	u32		mvpss_5_0;
	u32		mvpsls_14_8;
	u32		mvpsls_7_0;
	u32		mvpsfs_14_8;
	u32		mvpsfs;
	u32		mvpsagca;
	u32		mvpsagcb;
	u32		mveofbpc;
	dve88_t		dve_88;
	dve89_t		dve_89;
	u32		mvpcslimd_7_0;
	u32		mvpcslimd_9_8;
	u32		unused7[4];
	dve96_t		dve_96;
	dve97_t		dve_97;
	u32		unused8;
	dve99_t		dve_99;
	u32		mvtms_3_0;
	u32		unused9[8];
	u32		hlr_9_8;
	u32		hlr_7_0;
	u32		vsmr_4_0;
	u32		unused10[4];
	u32		dve116;
	u32		dve117;
	u32		dve118;
	u32		dve119;
	u32		dve120;
	u32		dve121;
	u32		dve122;
	u32		dve123;
	u32		dve124;
	u32		unused11[3];
	dve128_t	dve_r128;
} dram_dve_t;


/******************************** DISPLAY *************************************/
typedef struct vd_ctrl_s {
	u32 vid_format		:  5;	/* [4:0] Fixed Format Select */

#define	VD_NON_FIXED		0x0
#define	VD_480I60		0x1
#define	VD_480P60		0x2
#define	VD_576I50		0x3
#define	VD_576P50		0x4
#define	VD_720P60		0x5
#define	VD_720P50		0x6
#define	VD_1080I60		0x7
#define	VD_1080I50		0x8
#define	VD_1080I48_1080PSF24	0x9
#define	VD_1080P60		0xa	/* only for HDMI mode */
#define	VD_1080P50		0xb	/* only for HDMI mode */
#define	VD_1080P24		0xc 	/* test mode */
#define	VD_1080P25		0xd 	/* test mode */
#define	VD_1080P30		0xe     /* test mode */

	u32 interlace		:  1;	/* [5:5] Interlace Enable */

#define VD_PROGRESSIVE          0
#define VD_INTERLACE            1

	u32 reverse_mode	:  1;	/* [6:6] Reverse Mode Enable, Video Data
						 is horizontally reversed */

#define VD_REVERSE_DIS		0
#define VD_REVERSE_ENA		1

	u32 reserved1		: 18;	/* [24:7] Reserved */

	u32 sync_to_vout	:  1;	/* [25:25] VOUT-VOUT Sync Enable */

#define SYNC_TO_VOUT_DIS	0
#define SYNC_TO_VOUT_ENA	1

	u32 sync_to_vin		:  1;	/* [26:26] VIN-VOUT Sync Enable */

#define SYNC_TO_VIN_DIS		0
#define SYNC_TO_VIN_ENA		1

	u32 digital_out		:  1;	/* [27:27] Digital Output Enable */

#define VD_DIGITAL_DIS		0
#define VD_DIGITAL_ENA		1

	u32 analog_out		:  1;	/* [28:28] Analog Output Enable */

#define VD_ANALOG_DIS		0
#define VD_ANALOG_ENA		1

	u32 hdmi_out		:  1;	/* [29:29] HDMI Output Enable */

#define VD_HDMI_DIS		0
#define VD_HDMI_ENA		1

	u32 dve_reset  		:  1;   /* [30:30] DVE Soft Reset */

	u32 display_reset	:  1;	/* [31:31] Software Reset */
} vd_ctrl_t;

typedef union {
	vd_ctrl_t s;
	u32 w;
} vd_control_t;

typedef union {
	struct {
	u32 hdmi_field		:  1;	/* [0] HDMI Field */
	u32 analog_field	:  1;	/* [1] Analog Fied */
	u32 digital_field	:  1;	/* [2] Digital Field */
	u32 reserved		: 24;	/* [26:3] */
	u32 hdmi_uf		:  1;	/* [27] HDMI Underflow */
	u32 analog_uf		:  1;	/* [28] Analog Underflow */
	u32 digital_uf		:  1;	/* [29] Digital Underflow */
	u32 dve_config_ready	:  1;	/* [30] SDTV Configuration Ready */
	u32 reset_complete	:  1;	/* [31] Reset Complete */
	} s;
	u32 w;
} vd_status_t;

typedef union {
	struct {
	u32 v			: 14;	/* [13:0]  num of hync's per vsync */
	u32 reserved1		:  2;	/* [15:14] */
	u32 h			: 14;	/* [29:16] num of CLK/DCLK per hsync */
	u32 reserved2		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_hv_t;

typedef union {
	struct {
	u32 v			: 14;	/* [13:0]  num of hync's per vsync */
	u32 reserved		: 18;	/* [31:14] */
	} s;
	u32 w;
} vd_v1_t;

typedef union {
	struct {
	u32 startrow		: 14;	/* [13:0]  vertical starting position
					           of active display region */
	u32 reserved1		:  2;	/* [15:14] */
	u32 startcol		: 14;	/* [29:16] horizontal starting position
					          of active display region */
	u32 reserved2		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_actstart_t;

typedef union {
	struct {
	u32 endrow		: 14;	/* [13:0]  last line position
					           of active display region */
	u32 reserved1		:  2;	/* [15:14] */
	u32 endcol		: 14;	/* [29:16] last pixel of active column
					           of active display region */
	u32 reserved2		:  2;	/* [31:30] */
	} s;
	u32 w;
} vd_actend_t;

typedef union {
	struct {
	u32 hsyncend		: 14;	/* [13:0]  Hsync end column */
	u32 reserved1		:  2;	/* [15:14] Reserved */
	u32 hsyncstart		: 14;   /* [29:16] Hsync start column */
	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} vd_hsync_t;

typedef union {
	struct {
	u32 vsyncstart_row	: 14;	/* [13:0]  Vsync start row */
	u32 reserved1		:  2;	/* [15:14] Reserved */
	u32 vsyncstart_column	: 14;   /* [29:16] Vsync start column */
	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} vd_vsync_start_t;

typedef union {
	struct {
	u32 vsyncend_row	: 14;	/* [13:0]  Vsync end row */
	u32 reserved1		:  2;	/* [15:14] Reserved */
	u32 vsyncend_column	: 14;   /* [29:16] Vsync end column */
	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} vd_vsync_end_t;

typedef union {
	struct {
	u32 cr			:  8;	/* [7:0]  background Cr value */
	u32 cb			:  8;	/* [15:8]  background Cb value */
	u32 y			:  8;	/* [23:16] background Y value */
	u32 reserved		:  8;	/* [31:24] */
	} s;
	u32 w;
} vd_bg_t;

typedef union {
	struct {
	u32 start_row		:  14;	/* [13:0] Display A/B Start Row */
	u32 reserved1		:  2;	/* [15:14]  Reserved */
	u32 field		:  1;	/* [16:16] Field Select */
	u32 reserved2		:  15;	/* [31:17] Reserved */
	} s;
	u32 w;
} vd_vout_vout_sync_t;

typedef union {
	struct {
	u32 input_select	:  1;		/* [0] Enables input from SMEM */
	u32 reserved		:  31;	/* [31:1]  Reserved */
	} s;
	u32 w;
} vd_input_stream_enable_t;

/* Digital/LCD */
typedef union {
	struct {
		u32 hspol	:  1;	/* [0:0] Digital Hsync Polarity */

		u32 vspol	:  1; 	/* [1:1] Digital Vsync Polarity */

#define LCD_ACT_LOW		0
#define LCD_ACT_HIGH		1

		u32 clk_src	:  1;	/* [2:2] Digital Clock Output Divider */

#define LCD_CLK_UNDIVIDED	0
#define LCD_CLK_DIVIDED		1

		u32 divider_en	:  1;	/* [3:3] Digital Clock Divider Enable */

#define LCD_CLK_DIV_DIS		0
#define LCD_CLK_DIV_ENA		1

		u32 clk_edge	:  1;	/* [4:4] Digital Clock Edge Select */

#define LCD_CLK_VLD_RISING	0
#define LCD_CLK_VLD_FALLING	1

		u32 clk_dis	:  1;	/* [5:5] Digital Clock Disable */

#define LCD_CLK_NORMAL		0
#define LCD_CLK_DISABLE		1

		u32 div_ptn_len	:  7;	/* [12:6] Digital Clock Divider Pattern Width */

		u32 mipi_config	:  6;	/* [18:13] MIPI Configuration */

		u32 hvld_pol	:  1;	/* [19:19] cfg_digital_hvld_polarity  */

		u32 reserved	:  1;	/* [20:20] Reserved */

		u32 seqe	:  3;	/* [23:21] Color Sequence Even Lines */

		u32 seqo	:  3;	/* [26:24] Color Sequence Odd Lines */

#define VO_SEQ_R0_G1_B2		0
#define VO_SEQ_R0_B1_G2         1
#define VO_SEQ_G0_R1_B2         2
#define VO_SEQ_G0_B1_R2         3
#define VO_SEQ_B0_R1_G2         4
#define VO_SEQ_B0_G1_R2         5

#define VO_SEQ_R0_G1		0
#define VO_SEQ_G0_R1		2
#define VO_SEQ_G0_B1		3
#define VO_SEQ_B0_G1		5

		u32 mode	:  5;	/* [31:27] Digital Output Mode */

#define LCD_MODE_1COLOR		0
#define	LCD_MODE_3COLORS	1
#define	LCD_MODE_3COLORS_DUMMY  2
#define	LCD_MODE_RGB565		3

#define	LCD_MODE_656		4

#define	LCD_MODE_601_16BITS	5
#define	LCD_MODE_601_24BITS	6
#define	LCD_MODE_601_8BITS	7

#define	LCD_MODE_BAYER_PATTERN	8
#define	LCD_MODE_MIPI_YUV422	9
#define	LCD_MODE_MIPI_RGB565	10
#define	LCD_MODE_MIPI_RAW8	11
#define	LCD_MODE_MIPI_RGB888	12
	} s;
	u32 w;
} lcd_control_t;

typedef struct {
	int length;             /* Pattern length in bits */
	int enable;             /* Pattern enable/disable */
	int set;                /* Pattern set */
	u32 pattern3;           /* MSB */
	u32 pattern2;
	u32 pattern1;
	u32 pattern0;           /* LSB */
} lcd_clk_pattern_t;

typedef union {
	struct {
	u32 vbit_end_row	: 14;	/* [13:0]  656 V bit end row */
	u32 reserved1		:  2;	/* [15:14] Reserved */
	u32 vbit_start_row	: 14;   /* [29:16] 656 V bit start row */
	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} lcd_656_vbit_t;

typedef union {
	struct {
	u32 sav_start		: 14;	/* [13:0]  SAV Code Start Location */
	u32 reserved		: 18;	/* [31:14] Reserved */
	} s;
	u32 w;
} lcd_656_sav_start_t;

/* ANALOG */
typedef union {
	struct {
		u32 hspol	:  1;	/* [0:0] Digital Hsync Polarity */

		u32 vspol	:  1; 	/* [1:1] Digital Vsync Polarity */

#define ANALOG_ACT_LOW		0
#define ANALOG_ACT_HIGH		1

		u32 reserved	:  30;	/* [31:2] Reserved */
	} s;
	u32 w;
} analog_control_t;

typedef union {
	struct {
	u32 vbi_0_voltage	:  10;	/* [9:0] VBI Zero Level */

	u32 vbi_1_voltage	:  10;	/* [19:10] VBI One Level */

	u32 cpb			:  7;	/* [26:20] VBI Repeat Count */

	u32 sd_comp		:  1;   /* [27:27] SD Component */

#define HD_CVBS_SD              0
#define COMP_SD                 1

	u32 reserved		:  4;	/* [31:28] Reserved */
	} s;
	u32 w;
} vd_vbi_t;

typedef union {
	struct {
	u32 start_row_0		:  14;	/* [13:0] VBI start row field 0 */

	u32 reserved1		:  2;	/* [15:14] Reserved */

	u32 start_row_1		:  14;	/* [29:16] VBI start row field 1 */

	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} vd_vbi_row_t;

typedef union {
	struct {
	u32 end_col		:  14;	/* [13:0] VBI end column */

	u32 reserved1		:  2;	/* [15:14] Reserved */

	u32 start_col		:  14;	/* [29:16] VBI start column */

	u32 reserved2		:  2;	/* [31:30] Reserved */
	} s;
	u32 w;
} vd_vbi_col_t;

typedef union {
	struct {
	u32 y			:  11;	/* [10:0] SD Scale Y Coefficient */

	u32 reserved1		:  5;	/* [15:11] Reserved */

	u32 scale_en		:  1;	/* [16:16] SD Scale Enable */

	u32 reserved2		:  15;	/* [31:17] Reserved */
	} s;
	u32 w;
} vd_sd_scale_y_t;

typedef union {
	struct {
	u32 pr			:  11;	/* [10:0] SD Scale Pr Coefficient */

	u32 reserved1		:  5;	/* [15:11] Reserved */

	u32 pb			:  11;	/* [26:16] SD Scale Pb Coefficient */

	u32 reserved2		:  5;	/* [31:27] Reserved */
	} s;
	u32 w;
} vd_sd_scale_pbpr_t;

/* HDMI */
typedef union {
	struct {
		u32 hspol	:  1;	/* [0:0] Digital Hsync Polarity */

		u32 vspol	:  1; 	/* [1:1] Digital Vsync Polarity */

#define HDMI_ACT_LOW		0
#define HDMI_ACT_HIGH		1

		u32 reserved	:  27;	/* [28:2] Reserved */

		u32 mode	:  3;	/* [31:29] HDMI Output Mode */

#define HDMI_MODE_YCBCR_444	0
#define HDMI_MODE_RGB_444	1
#define HDMI_MODE_YC_422	2
	} s;
	u32 w;
} hdmi_control_t;

typedef struct {
	/* Global */
	vd_control_t		d_control;
	vd_status_t		d_status;
	vd_hv_t			d_frame_size;
	vd_v1_t			d_frame_height_field_1;
	vd_actstart_t		d_active_region_start_0;
	vd_actend_t		d_active_region_end_0;
	vd_actstart_t		d_active_region_start_1;
	vd_actend_t		d_active_region_end_1;
	vd_bg_t			d_background;

	/* Digital */
	lcd_control_t		d_digital_output_mode;
	vd_hsync_t		d_digital_hsync_control;
	vd_vsync_start_t	d_digital_vsync_start_0;
	vd_vsync_end_t		d_digital_vsync_end_0;
	vd_vsync_start_t	d_digital_vsync_start_1;
	vd_vsync_end_t		d_digital_vsync_end_1;
	lcd_656_vbit_t		d_digital_656_vbit;
	lcd_656_sav_start_t	d_digital_656_sav_start;
	u32			d_digital_clock_pattern_0;
	u32			d_digital_clock_pattern_1;
	u32			d_digital_clock_pattern_2;
	u32			d_digital_clock_pattern_3;
	u32			d_digital_csc_param_0;
	u32			d_digital_csc_param_1;
	u32			d_digital_csc_param_2;
	u32			d_digital_csc_param_3;
	u32			d_digital_csc_param_4;
	u32			d_digital_csc_param_5;
	u32			d_digital_csc_param_6;
	u32			d_digital_csc_param_7;
	u32			d_digital_csc_param_8;

	/* Analog */
	analog_control_t	d_analog_output_mode;
	vd_hsync_t		d_analog_hsync_control;
	vd_vsync_start_t	d_analog_vsync_start_0;
	vd_vsync_end_t		d_analog_vsync_end_0;
	vd_vsync_start_t	d_analog_vsync_start_1;
	vd_vsync_end_t		d_analog_vsync_end_1;
	vd_vbi_t		d_analog_vbi_control;
	vd_vbi_row_t		d_analog_vbi_start_v;
	vd_vbi_col_t		d_analog_vbi_h;
	u32			d_analog_vbi_data0;
	u32			d_analog_vbi_data1;
	u32			d_analog_vbi_data2;
	u32			d_analog_vbi_data3;
	u32			d_analog_vbi_data4;
	u32			d_analog_vbi_data5;
	u32			d_analog_vbi_data6;
	u32			d_analog_vbi_data7;
	u32			d_analog_vbi_data8;
	u32			d_analog_vbi_data9;
	u32			d_analog_vbi_data10;
	u32			d_analog_vbi_data11;
	u32			d_analog_csc_param_0;
	u32			d_analog_csc_param_1;
	u32			d_analog_csc_param_2;
	u32			d_analog_csc_param_3;
	u32			d_analog_csc_param_4;
	u32			d_analog_csc_param_5;
	u32			d_analog_csc_2_param_0;
	u32			d_analog_csc_2_param_1;
	u32			d_analog_csc_2_param_2;
	u32			d_analog_csc_2_param_3;
	u32			d_analog_csc_2_param_4;
	u32			d_analog_csc_2_param_5;
	vd_sd_scale_y_t		d_analog_sd_scale_y;
	vd_sd_scale_pbpr_t	d_analog_sd_scale_pbpr;

	/* HDMI */
	hdmi_control_t		d_hdmi_output_mode;
	vd_hsync_t		d_hdmi_hsync_control;
	vd_vsync_start_t	d_hdmi_vsync_start_0;
	vd_vsync_end_t		d_hdmi_vsync_end_0;
	vd_vsync_start_t	d_hdmi_vsync_start_1;
	vd_vsync_end_t		d_hdmi_vsync_end_1;
	u32			d_hdmi_csc_param_0;
	u32			d_hdmi_csc_param_1;
	u32			d_hdmi_csc_param_2;
	u32			d_hdmi_csc_param_3;
	u32			d_hdmi_csc_param_4;
	u32			d_hdmi_csc_param_5;
	u32			d_hdmi_csc_param_6;
	u32			d_hdmi_csc_param_7;
	u32			d_hdmi_csc_param_8;

	u32			unused[7];

	vd_vout_vout_sync_t		d_vout_vout_sync;
	vd_input_stream_enable_t 	d_input_stream_enable;
} dram_display_t;

typedef struct {
	vd_control_t		d_control;
	lcd_control_t		d_digital_output_mode;
	analog_control_t	d_analog_output_mode;
	hdmi_control_t		d_hdmi_output_mode;
} vd_config_t;

#endif //__AMBA_ARCH_VOUT_H

