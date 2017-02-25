/*
 * kernel/private/drivers/ambarella/vout/arch_s2l/vout_arch.c
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

#include <linux/i2c.h>
#include <linux/clk.h>
#include <plat/clk.h>
#include <iav_ioctl.h>
#include "../vout_pri.h"
#include "vout_arch.h"

static u32 vout_cvbs_mode_list[] = {
	AMBA_VIDEO_MODE_480I,
	AMBA_VIDEO_MODE_576I,

	AMBA_VIDEO_MODE_MAX
};

static int ambtve_dve_ntsc_config(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int             errorCode = 0;
	dram_dve_t      dve;

	errorCode = amba_s2l_vout_get_dve(psrc, &dve);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	/* Fsc divisor, Fsc= 3.57561149 */
	dve.phi_24_31 = 0x00000021;

	/* PHI = 0x21f07c1f */
	dve.phi_16_23 = 0x000000f0;
	dve.phi_15_8 = 0x0000007c;
	dve.phi_7_0 = 0x0000001f;
	dve.sctoh_31_24 = 0;
	dve.sctoh_23_16 = 0;
	dve.sctoh_15_8 = 0;
	dve.sctoh_7_0 = 0;

	dve.dve_40.s.u_invert = 0;
	dve.dve_40.s.v_invert = 0;
	dve.dve_40.s.t_offset_phase = 0;
	dve.dve_40.s.t_reset_fsc = 0;

	/* compisite */
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	} else {
		/* black level:140 in DVE42 */
		dve.black_lvl = 0x0000007d;
		/* blank level:120 in DVE43 */
		dve.blank_lvl = 0x0000007a;
	}

	/* clamp level of 16 in DVE44 */
	dve.clamp_lvl = 0x00000004;

	/* sync level of 8 in DVE45 */
	dve.sync_lvl = 0x00000008;

	//do not use interpolation in DVE46
	dve.dve_46.s.y_interp = 0;
	/* color settings */
	if (sink_mode->mode == AMBA_VIDEO_MODE_TEST)
		dve.dve_46.s.y_colorbar_en = 1;
	else
		dve.dve_46.s.y_colorbar_en = 0;
	/* zero delay in Y*/
	dve.dve_46.s.t_ydel_adj = 4;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		dve.dve_46.s.t_sel_ylpf = 0;
		dve.dve_46.s.t_ygain_val = 0;
	} else {
		dve.dve_46.s.t_sel_ylpf = 1;
		dve.dve_46.s.t_ygain_val = 1;
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_47.s.sel_yuv = 1;
	else
		dve.dve_47.s.sel_yuv = 0;
	dve.dve_47.s.pwr_dwn_cv_dac = 0;
	dve.dve_47.s.pwr_dwn_y_dac = 0;
	dve.dve_47.s.pwr_dwn_c_dac = 0;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* NBA of 0 in DVE50 */
		dve.nba = 0;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	} else {
		/* NBA of -60 in DVE50 */
		dve.nba = 0x000000c4;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	}

	/* DVE52 */
	dve.dve_52.s.pal_c_lpf = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_52.s.sel_c_gain = 0;
	else
		dve.dve_52.s.sel_c_gain = 0;

	/*DVE56 */
	dve.dve_56.s.t_hsync_phs = 0;
	dve.dve_56.s.t_vsync_phs = 0;
	/* H/V sync in, 16 bit sep YCbCr */
	dve.dve_56.s.y_tsyn_mode = 1;
	dve.dve_56.s.y_tencd_mode =0;

	/* DVE57 */
	dve.dve_57.s.clk_phs = 0;
	dve.dve_57.s.t_psync_enb = 0;
	dve.dve_57.s.t_psync_phs = 0;
	dve.dve_57.s.unused = 0;
	dve.dve_57.s.vso = 0;

	/* DVE58 */
	dve.vso_7_0 = 0x00;

	/* DVE59 */
	dve.hso_10_8 = 0;
	/* DVE60 */
	dve.hso_7_0 = 0x00;
	/* DVE61 */
	dve.hcl_9_8 = 0x00000003;
	/* DVE62: 0x1716/2-1=0x359 */
	dve.hcl_7_0 = 0x00000059;
	/* DVE65 */
	dve.ccd_odd_15_8 = 0;
	/*DVE66 */
	dve.ccd_odd_7_0 = 0;
	/* DVE67 */
	dve.ccd_even_15_8 = 0;
	/* DVE68 */
	dve.ccd_even_7_0 = 0;
	/* DVE69 */
	dve.cc_enbl = 0;
	/* turn off macrovision in DVE72 */
	dve.mvfcr = 0;
	/*DVE73 */
	dve.mvcsl1_5_0 = 0;
	/*DVE74 */
	dve.mvcls1_5_0 = 0;
	/*DVE75 */
	dve.mvcsl2_5_0 = 0;
	/*DVE76 */
	dve.mvcls2_5_0 = 0;
	/* DVE77 */
	dve.dve_77.s.cs_sp = 0;
	dve.dve_77.s.cs_num = 0;
	dve.dve_77.s.cs_ln = 0;
	/* DVE78 */
	dve.mvpsd_5_0 = 0;
	/*DVE79 */
	dve.mvpsl_5_0 = 0;
	/*DVE80 */
	dve.mvpss_5_0 = 0;
	/*DVE81 */
	dve.mvpsls_14_8 = 0;
	/* DVE82 */
	dve.mvpsls_7_0 = 0;
	/* DVE83 */
	dve.mvpsfs_14_8 = 0;
	/* DVE84 */
	dve.mvpsfs = 0;
	/* DVE85 */
	dve.mvpsagca = 0;
	/* DVE86 */
	dve.mvpsagcb = 0;
	/* DVE87 */
	dve.mveofbpc = 0;

	/* DVE88 */
	dve.dve_88.s.bst_zone_sw1 = 8;
	dve.dve_88.s.bst_zone_sw2 = 0;

	/* DVE89 */
	dve.dve_89.s.bz3_end = 5;
	dve.dve_89.s.adv_bs_en = 1;
	dve.dve_89.s.bz_invert_en = 4;

	/* DVE90 */
	dve.mvpcslimd_7_0 = 0;
	/* DVE91 */
	dve.mvpcslimd_9_8 = 0;

	/* DVE96 */
	dve.dve_96.s.sel_sin = 0;
	dve.dve_96.s.fsc_tst_en = 0;

	/* DVE97 */
	dve.dve_97.s.byp_y_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn on luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 1;
	} else {
		/* turn on the luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 0;
	}

	dve.dve_97.s.sin_cos_en = 0;
	dve.dve_97.s.sel_dac_tst = 0;
	dve.dve_97.s.dig_out_en = 0;

	dve.dve_99.s.byp_c_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn off chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;
		dve.dve_99.s.cgain_off = 1;
	} else {
		/* turn on the chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;  /* Fix the false color */
		dve.dve_99.s.cgain_off = 0;
	}
	dve.mvtms_3_0 = 0;
	dve.hlr_9_8 = 0;
	dve.hlr_7_0 = 0;
	dve.vsmr_4_0 = 0x00010000;

	errorCode = amba_s2l_vout_set_dve(psrc, &dve);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	return errorCode;
}

static int ambtve_dve_pal_config(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int             errorCode = 0;
	dram_dve_t      dve;

	errorCode = amba_s2l_vout_get_dve(psrc, &dve);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	/* Fsc divisor, Fsc= 3.57561149 */
	dve.phi_24_31 = 0x0000002a;
	/* PHI = 0x21f07c1f */
	dve.phi_16_23 = 0x00000009;
	dve.phi_15_8 = 0x0000008a;
	dve.phi_7_0 = 0x000000cb;
	dve.sctoh_31_24 = 0;
	dve.sctoh_23_16 = 0;
	dve.sctoh_15_8 = 0;
	dve.sctoh_7_0 = 0;

	dve.dve_40.s.u_invert = 0;
	dve.dve_40.s.v_invert = 0;
	dve.dve_40.s.t_offset_phase = 0;
	dve.dve_40.s.t_reset_fsc = 0;

	/* compisite */
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	} else {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	}

	/* clamp level of 16 in DVE44 */
	dve.clamp_lvl = 0x00000013;

	/* sync level of 8 in DVE45 */
	dve.sync_lvl = 0x00000010;

	//do not use interpolation in DVE46
	dve.dve_46.s.y_interp = 0;
	/* color settings */
	if (sink_mode->mode == AMBA_VIDEO_MODE_TEST)
		dve.dve_46.s.y_colorbar_en = 1;
	else
		dve.dve_46.s.y_colorbar_en = 0;
	/* zero delay in Y*/
	dve.dve_46.s.t_ydel_adj = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		dve.dve_46.s.t_sel_ylpf = 0;
		dve.dve_46.s.t_ygain_val = 0;
	} else {
		dve.dve_46.s.t_sel_ylpf = 1;
		dve.dve_46.s.t_ygain_val = 1;
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_47.s.sel_yuv = 1;
	else
		dve.dve_47.s.sel_yuv = 0;
	dve.dve_47.s.pwr_dwn_cv_dac = 0;
	dve.dve_47.s.pwr_dwn_y_dac = 0;
	dve.dve_47.s.pwr_dwn_c_dac = 0;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* NBA of 0 in DVE50 */
		dve.nba = 0;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	} else {
		/* NBA of -60 in DVE50 */
		dve.nba = 0x000000d3;
		/* PBA of 0 in DVE51 */
		dve.pba = 0x0000002d;
	}

	/* DVE52 */
	dve.dve_52.s.pal_c_lpf = 1;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_52.s.sel_c_gain = 0;
	else
		dve.dve_52.s.sel_c_gain = 1;

	/*DVE56 */
	dve.dve_56.s.t_hsync_phs = 0;
	dve.dve_56.s.t_vsync_phs = 0;
	/* H/V sync in, 16 bit sep YCbCr */
	dve.dve_56.s.y_tsyn_mode = 1;
	dve.dve_56.s.y_tencd_mode =1;

	/* DVE57 */
	dve.dve_57.s.clk_phs = 0;
	dve.dve_57.s.t_psync_enb = 0;
	dve.dve_57.s.t_psync_phs = 0;
	dve.dve_57.s.unused = 0;
	dve.dve_57.s.vso = 0;

	/* DVE58 */
	dve.vso_7_0 = 0x00;

	/* DVE59 */
	dve.hso_10_8 = 0;
	/* DVE60 */
	dve.hso_7_0 = 0x00;
	/* DVE61 */
	dve.hcl_9_8 = 0x00000003;
	/* DVE62: 864 * 2 / 2 - 1 = 0x359 */
	dve.hcl_7_0 = 0x0000005f;
	/* DVE65 */
	dve.ccd_odd_15_8 = 0;
	/*DVE66 */
	dve.ccd_odd_7_0 = 0;
	/* DVE67 */
	dve.ccd_even_15_8 = 0;
	/* DVE68 */
	dve.ccd_even_7_0 = 0;
	/* DVE69 */
	dve.cc_enbl = 0;
	/* turn off macrovision in DVE72 */
	dve.mvfcr = 0;
	/*DVE73 */
	dve.mvcsl1_5_0 = 0;
	/*DVE74 */
	dve.mvcls1_5_0 = 0;
	/*DVE75 */
	dve.mvcsl2_5_0 = 0;
	/*DVE76 */
	dve.mvcls2_5_0 = 0;
	/* DVE77 */
	dve.dve_77.s.cs_sp = 0;
	dve.dve_77.s.cs_num = 0;
	dve.dve_77.s.cs_ln = 0;
	/* DVE78 */
	dve.mvpsd_5_0 = 0;
	/*DVE79 */
	dve.mvpsl_5_0 = 0;
	/*DVE80 */
	dve.mvpss_5_0 = 0;
	/*DVE81 */
	dve.mvpsls_14_8 = 0;
	/* DVE82 */
	dve.mvpsls_7_0 = 0;
	/* DVE83 */
	dve.mvpsfs_14_8 = 0;
	/* DVE84 */
	dve.mvpsfs = 0;
	/* DVE85 */
	dve.mvpsagca = 0;
	/* DVE86 */
	dve.mvpsagcb = 0;
	/* DVE87 */
	dve.mveofbpc = 0;

	/* DVE88 */
	dve.dve_88.s.bst_zone_sw1 = 8;
	dve.dve_88.s.bst_zone_sw2 = 0;

	/* DVE89 */
	dve.dve_89.s.bz3_end = 5;
	dve.dve_89.s.adv_bs_en = 1;
	dve.dve_89.s.bz_invert_en = 4;

	/* DVE90 */
	dve.mvpcslimd_7_0 = 0;
	/* DVE91 */
	dve.mvpcslimd_9_8 = 0;

	/* DVE96 */
	dve.dve_96.s.sel_sin = 0;
	dve.dve_96.s.fsc_tst_en = 0;

	/* DVE97 */
	dve.dve_97.s.byp_y_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn on luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 1;
	} else {
		/* turn on the luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 0;
	}

	dve.dve_97.s.sin_cos_en = 0;
	dve.dve_97.s.sel_dac_tst = 0;
	dve.dve_97.s.dig_out_en = 0;

	dve.dve_99.s.byp_c_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn off chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;
		dve.dve_99.s.cgain_off = 1;
	} else {
		/* turn on the chroma lpf */
		dve.dve_99.s.byp_c_lpf = 0;
		dve.dve_99.s.cgain_off = 0;
	}
	dve.mvtms_3_0 = 0;
	dve.hlr_9_8 = 0;
	dve.hlr_7_0 = 0;
	dve.vsmr_4_0 = 0x00010000;

	errorCode = amba_s2l_vout_set_dve(psrc, &dve);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	return errorCode;
}

static int ambtve_init_480i_s2l(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int					rval = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
	vd_config_t	sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	//set hv
	sink_hv.hsize = YUV525I_Y_PELS_PER_LINE * 2;
	sink_hv.vtsize = 262;
	sink_hv.vbsize = 263;
	rval = amba_s2l_vout_set_hv(psrc, &sink_hv);
	if(rval)
		return rval;

	//set hvsync
	sink_sync.hsync_start = 0;
	sink_sync.hsync_end = 1;
	sink_sync.vtsync_start = 0;
	sink_sync.vtsync_end = 1;
	sink_sync.vbsync_start = 0;
	sink_sync.vbsync_end = 1;
	sink_sync.vtsync_start_row = 0;
	sink_sync.vtsync_start_col = 0;
	sink_sync.vtsync_end_row = 3;
	sink_sync.vtsync_end_col = 0;
	sink_sync.vbsync_start_row = 0x3ffd;
	sink_sync.vbsync_start_col = 858;
	sink_sync.vbsync_end_row = 0x3fff;
	sink_sync.vbsync_end_col = 858;
	sink_sync.sink_type = sink_mode->sink_type;
	rval = amba_s2l_vout_set_hvsync(psrc, &sink_sync);
	if(rval)
		return rval;

	//set active window
	sink_window.start_x = 274;
	sink_window.end_x = 1713;
	sink_window.start_y = 22;
	sink_window.end_y = 261;
	sink_window.width = (sink_window.end_x - sink_window.start_x + 1) >> 1;
	sink_window.field_reverse = 0;
	rval = amba_s2l_vout_set_active_win(psrc, &sink_window);
	if(rval)
		return rval;

	// set video size
	sink_window.start_x = sink_mode->video_offset.offset_x;
	sink_window.end_x =
		sink_window.start_x + sink_mode->video_size.video_width - 1;
	sink_window.start_y = sink_mode->video_offset.offset_y;
	sink_window.end_y =
		sink_window.start_y + sink_mode->video_size.video_height - 1;
	if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {
		sink_window.start_y >>= 1;
		sink_window.end_y >>= 1;
	}
	rval = amba_s2l_vout_set_video_size(psrc, &sink_window);
	if(rval)
		return rval;

	// set config
	rval = amba_s2l_vout_get_config(psrc, &sink_cfg);
	if (rval)
		return rval;

	sink_cfg.d_control.s.hdmi_out = 0;
	sink_cfg.d_control.s.digital_out = 0;
	sink_cfg.d_control.s.analog_out = 1;
	sink_cfg.d_control.s.interlace = VD_INTERLACE;
	sink_cfg.d_control.s.vid_format = VD_480I60;
	sink_cfg.d_analog_output_mode.s.hspol = ANALOG_ACT_HIGH;
	sink_cfg.d_analog_output_mode.s.vspol = ANALOG_ACT_HIGH;
	rval = amba_s2l_vout_set_config(psrc, &sink_cfg);
	if(rval)
		return rval;

	//set csc
	sink_csc.path = AMBA_VIDEO_SOURCE_CSC_ANALOG;
	if (sink_mode->csc_en)
		sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_ANALOG_SD;
	else
		sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
	sink_csc.clamp = AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD_NTSC;
	rval = amba_s2l_vout_set_csc(psrc, &sink_csc);
	if (rval)
		return rval;

	rval = ambtve_dve_ntsc_config(psrc, sink_mode);
	if (rval)
		return rval;

	return rval;
}

static int ambtve_init_576i_s2l(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int					rval = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
	vd_config_t	sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	//set hv
	sink_hv.hsize = YUV625I_Y_PELS_PER_LINE * 2;
	sink_hv.vtsize = 312;
	sink_hv.vbsize = 313;
	rval = amba_s2l_vout_set_hv(psrc, &sink_hv);
	if(rval)
		return rval;

	//set hvsync
	sink_sync.hsync_start = 0;
	sink_sync.hsync_end = 1;
	sink_sync.vtsync_start = 0;
	sink_sync.vtsync_end = 1;
	sink_sync.vbsync_start = 0;
	sink_sync.vbsync_end = 1;
	sink_sync.vtsync_start_row = 0;
	sink_sync.vtsync_start_col = 0;
	sink_sync.vtsync_end_row = 3;
	sink_sync.vtsync_end_col = 0;
	sink_sync.vbsync_start_row = 0x3ffd;
	sink_sync.vbsync_start_col = 864;
	sink_sync.vbsync_end_row = 0x3fff;
	sink_sync.vbsync_end_col = 864;
	sink_sync.sink_type = sink_mode->sink_type;
	rval = amba_s2l_vout_set_hvsync(psrc, &sink_sync);
	if(rval)
		return rval;

	//set active window
	sink_window.start_x = 286;
	sink_window.end_x = 1725;
	sink_window.start_y = 24;
	sink_window.end_y = 311;
	sink_window.width = (sink_window.end_x - sink_window.start_x + 1) >> 1;
	sink_window.field_reverse = 0;
	rval = amba_s2l_vout_set_active_win(psrc, &sink_window);
	if(rval)
		return rval;

	//set video size
	sink_window.start_x = sink_mode->video_offset.offset_x;
	sink_window.end_x =
		sink_window.start_x + sink_mode->video_size.video_width - 1;
	sink_window.start_y = sink_mode->video_offset.offset_y;
	sink_window.end_y =
		sink_window.start_y + sink_mode->video_size.video_height - 1;
	if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {
		sink_window.start_y >>= 1;
		sink_window.end_y >>= 1;
	}
	rval = amba_s2l_vout_set_video_size(psrc, &sink_window);
	if(rval)
		return rval;

	// set config
	rval = amba_s2l_vout_get_config(psrc, &sink_cfg);
	if (rval)
		return rval;

	sink_cfg.d_control.s.hdmi_out = 0;
	sink_cfg.d_control.s.digital_out = 0;
	sink_cfg.d_control.s.analog_out = 1;
	sink_cfg.d_control.s.interlace = VD_INTERLACE;
	sink_cfg.d_control.s.vid_format = VD_576I50;
	sink_cfg.d_analog_output_mode.s.hspol = ANALOG_ACT_HIGH;
	sink_cfg.d_analog_output_mode.s.vspol = ANALOG_ACT_HIGH;
	rval = amba_s2l_vout_set_config(psrc, &sink_cfg);
	if(rval)
		return rval;

	//set csc
	sink_csc.path = AMBA_VIDEO_SOURCE_CSC_ANALOG;
	if (sink_mode->csc_en)
		sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_ANALOG_SD;
	else
		sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
	sink_csc.clamp = AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD;
	rval = amba_s2l_vout_set_csc(psrc, &sink_csc);
	if (rval)
		return rval;

	// pal config
	rval = ambtve_dve_pal_config(psrc, sink_mode);
	if (rval)
		return rval;

	return rval;
}

static int ambtve_init_480i(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int	errorCode = 0;
	struct amba_video_info	sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;
	vout_video_setup_t	video_setup;

	//set clock
	clk_setup.src = VO_CLK_ONCHIP_PLL_27MHZ;
	clk_setup.freq_hz = PLL_CLK_27MHZ;
	psrc->freq_hz = clk_setup.freq_hz;
	amba_s2l_vout_set_clock_setup(psrc, &clk_setup);

	errorCode = ambtve_init_480i_s2l(psrc, sink_mode);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set vbi
	errorCode = amba_s2l_vout_set_vbi(psrc, sink_mode);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set video info
	sink_video_info.width = sink_mode->video_size.video_width;
	sink_video_info.height = sink_mode->video_size.video_height;
	sink_video_info.system = AMBA_VIDEO_SYSTEM_NTSC;
	sink_video_info.fps = YUV525I_DEFAULT_FRAME_RATE;
	sink_video_info.format = sink_mode->format;
	sink_video_info.type = sink_mode->type;
	sink_video_info.bits = sink_mode->bits;
	sink_video_info.ratio = sink_mode->ratio;
	sink_video_info.flip = sink_mode->video_flip;
	sink_video_info.rotate = sink_mode->video_rotate;
	errorCode = amba_s2l_vout_set_video_info(psrc, &sink_video_info);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set vout video setup
	errorCode = amba_s2l_vout_get_setup(psrc, &video_setup);
	if(errorCode)
		return errorCode;
	video_setup.en = sink_mode->video_en;
	switch (sink_mode->video_flip) {
	case AMBA_VOUT_FLIP_NORMAL:
		video_setup.flip = 0;
		break;
	case AMBA_VOUT_FLIP_HV:
		video_setup.flip = 1;
		break;
	case AMBA_VOUT_FLIP_HORIZONTAL:
		video_setup.flip = 2;
		break;
	case AMBA_VOUT_FLIP_VERTICAL:
		video_setup.flip = 3;
		break;
	default:
		vout_err("%s can't support flip[%d]!\n", __func__, sink_mode->video_flip);
		return -EINVAL;
	}

	switch (sink_mode->video_rotate) {
	case AMBA_VOUT_ROTATE_NORMAL:
		video_setup.rotate = 0;
		break;
	case AMBA_VOUT_ROTATE_90:
		video_setup.rotate = 1;
		break;
	default:
		vout_info("%s can't support rotate[%d]!\n", __func__, sink_mode->video_rotate);
		return -EINVAL;
	}

	errorCode = amba_s2l_vout_set_setup(psrc, &video_setup);
	if(errorCode)
		return errorCode;

	return errorCode;
}

static int ambtve_init_576i(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_video_info			sink_video_info;
	struct amba_video_source_clock_setup	clk_setup;
	vout_video_setup_t		        video_setup;

	//set clock
	clk_setup.src = VO_CLK_ONCHIP_PLL_27MHZ;
	clk_setup.freq_hz = PLL_CLK_27MHZ;
	psrc->freq_hz = clk_setup.freq_hz;
	amba_s2l_vout_set_clock_setup(psrc, &clk_setup);

	ambtve_init_576i_s2l(psrc, sink_mode);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set vbi
	errorCode = amba_s2l_vout_set_vbi(psrc, sink_mode);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set video info
	sink_video_info.width = sink_mode->video_size.video_width;
	sink_video_info.height = sink_mode->video_size.video_height;
	sink_video_info.system = AMBA_VIDEO_SYSTEM_PAL;
	sink_video_info.fps = YUV625I_DEFAULT_FRAME_RATE;
	sink_video_info.format = sink_mode->format;
	sink_video_info.type = sink_mode->type;
	sink_video_info.bits = sink_mode->bits;
	sink_video_info.ratio = sink_mode->ratio;
	sink_video_info.flip = sink_mode->video_flip;
	sink_video_info.rotate = sink_mode->video_rotate;
	errorCode = amba_s2l_vout_set_video_info(psrc, &sink_video_info);
	if (errorCode) {
		vout_errorcode();
		return errorCode;
	}

	//set vout video setup
	errorCode = amba_s2l_vout_get_setup(psrc, &video_setup);
	if(errorCode)
		return errorCode;
	video_setup.en = sink_mode->video_en;
	switch (sink_mode->video_flip) {
	case AMBA_VOUT_FLIP_NORMAL:
		video_setup.flip = 0;
		break;
	case AMBA_VOUT_FLIP_HV:
		video_setup.flip = 1;
		break;
	case AMBA_VOUT_FLIP_HORIZONTAL:
		video_setup.flip = 2;
		break;
	case AMBA_VOUT_FLIP_VERTICAL:
		video_setup.flip = 3;
		break;
	default:
		vout_err("%s can't support flip[%d]!\n", __func__, sink_mode->video_flip);
		return -EINVAL;
	}
	switch (sink_mode->video_rotate) {
	case AMBA_VOUT_ROTATE_NORMAL:
		video_setup.rotate = 0;
		break;
	case AMBA_VOUT_ROTATE_90:
		video_setup.rotate = 1;
		break;
	default:
		vout_info("%s can't support rotate[%d]!\n", __func__, sink_mode->video_rotate);
		return -EINVAL;
	}

	errorCode = amba_s2l_vout_set_setup(psrc, &video_setup);
	if(errorCode)
		return errorCode;

	return errorCode;
}

int amba_s2l_vout_analog_init(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *sink_mode)
{
	int     i, errorCode = 0;

	psrc->active_sink_type = AMBA_VOUT_SINK_TYPE_CVBS;

	for (i = 0; i < AMBA_VIDEO_MODE_MAX; i++)	{
		if (vout_cvbs_mode_list[i] == AMBA_VIDEO_MODE_MAX) {
			errorCode = -ENOIOCTLCMD;
			vout_err("vout analog can not support mode %d!\n",
				sink_mode->mode);
			return errorCode;
		}
		if (vout_cvbs_mode_list[i] == sink_mode->mode)
			break;
	}

	switch(sink_mode->mode) {
	case AMBA_VIDEO_MODE_480I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_480i(psrc, sink_mode);
		}
		break;

	case AMBA_VIDEO_MODE_576I:
		if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
			sink_mode->ratio = AMBA_VIDEO_RATIO_4_3;
		if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
			sink_mode->bits = AMBA_VIDEO_BITS_16;
		if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
			sink_mode->type = AMBA_VIDEO_TYPE_YUV_601;
		if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
			sink_mode->format = AMBA_VIDEO_FORMAT_INTERLACE;

		if ((sink_mode->type = AMBA_VIDEO_TYPE_YUV_601) &&
			(sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)) {
			errorCode = ambtve_init_576i(psrc, sink_mode);
		}
		break;

	default:
		vout_err("vout analog do not support mode %d!\n",
			sink_mode->mode);
		errorCode = -ENOIOCTLCMD;
		break;
	}

	return errorCode;
}

