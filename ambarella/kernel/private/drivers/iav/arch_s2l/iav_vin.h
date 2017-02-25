/*
 * iav_vin.h
 *
 * History:
 *	2013/08/29- [Cao Rongrong] Created file
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


#ifndef __IAV_VIN_H__
#define __IAV_VIN_H__

#include <linux/irqreturn.h>
#include <linux/idr.h>
#include <plat/ambsyncproc.h>
#include <plat/rct.h>
#include "iav.h"

#define VIN_PRIMARY		(0)
#define VIN_PIP			(1)

#define DSP_VIN_DEBUG_OFFSET		0x110000
#define DSP_VIN_DEBUG_BASE		(DBGBUS_BASE + DSP_VIN_DEBUG_OFFSET)
#define DSP_VIN_DEBUG_REG(x)		(DSP_VIN_DEBUG_BASE + (x))

#define USE_CLK_SI_INPUT_MODE_OFFSET	0xBC
#define LVDS_LVCMOS_OFFSET		0xF8
#define LVDS_ASYNC_OFFSET		0xFC
#define USE_CLK_SI_INPUT_MODE_REG	RCT_REG(USE_CLK_SI_INPUT_MODE_OFFSET)
#define LVDS_LVCMOS_REG			RCT_REG(LVDS_LVCMOS_OFFSET)
#define LVDS_ASYNC_REG			RCT_REG(LVDS_ASYNC_OFFSET)

#define AMBA_VIN_IRQ_NA			(0x00)
#define AMBA_VIN_IRQ_WAIT		(0x01)
#define AMBA_VIN_IRQ_READY		(0x02)

#define CLAMP_GAIN			(36 << 24) /* 36db */
#define CLAMP_SHUTTER		(AMBA_VIDEO_FPS(240)) /* 1/240s */

struct vin_dsp_config {
	struct {
		u16 swreset: 1;
		u16 enable : 1;
		u16 output_enable : 1;
		u16 word_size : 3;
		u16 lane_enable : 10;
	}r0;
	struct {
		u16 p_pad : 1;
		u16 p_rate : 1;
		u16 p_width : 1;
		u16 p_edge : 1;
		u16 p_sync : 1;
		u16 p_emb : 2;
		u16 rgb_yuv_mode : 1;
		u16 pix_reorder : 2;
		u16 vsync_polarity : 1;
		u16 hsync_polarity : 1;
		u16 field_polarity : 1;
		u16 field_mode : 1;
		u16 disable_after_vsync : 1;
		u16 double_buff_enable : 1;
	}r1;
	struct {
		u16 field_pin_sel : 5;
		u16 vsync_pin_sel : 5;
		u16 hsync_pin_sel : 5;
		u16 reserved : 1;
	}r2;
	struct {
		u16 lane_select0 : 4;
		u16 lane_select1 : 4;
		u16 lane_select2 : 4;
		u16 lane_select3 : 4;
	}r3;
	struct {
		u16 lane_select4 : 4;
		u16 lane_select5 : 4;
		u16 lane_select6 : 4;
		u16 lane_select7 : 4;
	}r4;
	struct {
		u16 lane_select8 : 4;
		u16 lane_select9 : 4;
		u16 reserved1 : 4;
		u16 reserved2 : 4;
	}r5;
	struct {
		u16 active_width;
	}r6;
	struct {
		u16 active_height;
	}r7;
	struct {
		u16 split_width;
	}r8;
	struct {
		u16 crop_start_col;
	}r9;
	struct {
		u16 crop_start_row;
	}r10;
	struct {
		u16 crop_end_col;
	}r11;
	struct {
		u16 crop_end_row;
	}r12;
	struct {
		u16 sync_interleaving : 2;
		u16 sync_tolerance : 2;
		u16 sync_all_lanes : 1;
		u16 reserved : 11;
	}r13;
	struct {
		u16 allow_partial_codes : 1;
		u16 lock_sync_phase : 1;
		u16 sync_correction : 1;
		u16 deskew_enable : 1;
		u16 sync_type : 1;
		u16 enable_656_ecc : 1;
		u16 pat_mask_align: 1;
		u16 unlock_on_timeout : 1;
		u16 unlock_on_deskew_err : 1;
		u16 line_reorder : 1;
		u16 reserved : 6;
	}r14;
	struct {
		u16 sol_en : 1;
		u16 eol_en : 1;
		u16 sof_en : 1;
		u16 eof_en : 1;
		u16 sov_en : 1;
		u16 eov_en : 1;
		u16 reserved : 10;
	}r15;
	struct {
		u16 sync_timeout_l;
	}r16;
	struct {
		u16 sync_timeout_h;
	}r17;
	struct {
		u16 sync_detect_mask;
	}r18;
	struct {
		u16 sync_detect_pat;
	}r19;
	struct {
		u16 sync_compare_mask;
	}r20;
	struct {
		u16 sol_pat;
	}r21;
	struct {
		u16 eol_pat;
	}r22;
	struct {
		u16 sof_pat;
	}r23;
	struct {
		u16 eof_pat;
	}r24;
	struct {
		u16 sov_pat;
	}r25;
	struct {
		u16 eov_pat;
	}r26;
	struct {
		u16 mipi_vc_mask : 2;
		u16 mipi_vc_pat : 2;
		u16 mipi_dt_mask : 6;
		u16 mipi_dt_pat : 6;
	}r27;
	struct {
		u16 mipi_decompr_enable : 1;
		u16 mipi_decompr_mode : 2;
		u16 mipi_byte_swap : 1;
		u16 mipi_ecc_enable : 1;
		u16 reserved : 11;
	}r28;
	struct {
		u16 vout_sync0_mode : 2;
		u16 vout_sync0_line : 14;
	}r29;
	struct {
		u16 vout_sync1_mode : 2;
		u16 vout_sync1_line : 14;
	}r30;
	struct {
		u16 strig0_enable : 1;
		u16 strig0_polarity : 1;
		u16 strig0_start_line : 14;
	}r31;
	struct {
		u16 strig0_end_line : 16;
	}r32;
	struct {
		u16 strig1_enable : 1;
		u16 strig1_polarity : 1;
		u16 strig1_start_line : 14;
	}r33;
	struct {
		u16 strig1_end_line : 16;
	}r34;
	struct {
		u16 delayed_vsync_l : 16;
	}r35;
	struct {
		u16 delayed_vsync_h : 16;
	}r36;
	struct {
		u16 delayed_vsync_irq_l : 16;
	}r37;
	struct {
		u16 delayed_vsync_irq_h : 16;
	}r38;
	struct {
		u16 got_act_sof : 1;
		u16 strig0_status : 1;
		u16 strig1_status : 1;
		u16 sfifo_overflow : 1;
		u16 short_line : 1;
		u16 short_frame : 1;
		u16 field : 3;
		u16 sync_timeout : 1;
		u16 dbg_reserved0 : 1;
		u16 got_vsync : 1;
		u16 sent_master_vsync : 1;
		u16 got_win_eof : 1;
		u16 uncorrectable_656_error : 1;
		u16 programming_error : 1;
	}r39;
	struct {
		u16 sync_locked : 1;
		u16 lost_lock_after_sof : 1;
		u16 partial_sync_detected : 1;
		u16 unknown_sync_code : 1;
		u16 got_win_sof : 1;
		u16 afifo_overflow : 1;
		u16 sync_state : 2;
		u16 reserved : 8;
	}r40;
	struct {
		u16 sfifo_count;
	}r41;
	struct {
		u16 detected_active_width;
	}r42;
	struct {
		u16 detected_active_height;
	}r43;
	struct {
		u16 sync_code;
	}r44;
	struct {
		u16 act_v_count;
	}r45;
	struct {
		u16 act_h_count;
	}r46;
	struct {
		u16 win_v_count;
	}r47;
	struct {
		u16 win_h_count;
	}r48;
	struct {
		u16 mipi_rxulpsclknot : 1;
		u16 mipi_rxclkactivehs : 1;
		u16 mipi_rxulpsactivenotclk : 1;
		u16 mipi_rxclkstopstate : 1;
		u16 mipi_rxactivehs : 4;
		u16 mipi_rxstopstate : 4;
		u16 mipi_rxulpsactivenot : 4;
	}r49;
	struct {
		u16 mipi_errsoths : 4;
		u16 mipi_errsotsynchs : 4;
		u16 mipi_errcontrol : 4;
		u16 mipi_ecc_err_2bit : 1;
		u16 mipi_ecc_err_1bit : 1;
		u16 mipi_crc_error : 1;
		u16 mipi_err_id : 1;
	}r50;
	struct {
		u16 mipi_err_fs : 1;
		u16 mipi_prot_state : 2;
		u16 mipi_sof_pkt_rcvd : 1;
		u16 mipi_eof_pkt_rcvd : 1;
		u16 reserved : 11;
	}r51;
	struct {
		u16 mipi_long_pkt_count;
	}r52;
	struct {
		u16 mipi_short_pkt_count;
	}r53;
	struct {
		u16 mipi_long_pkt_size;
	}r54;
	struct {
		u16 mipi_num_err_frames;
	}r55;
	struct {
		u16 mipi_frame_number;
	}r56;
};

struct vin_master_config {
	struct {
		u16 hsync_period_l;
	}r0;
	struct {
		u16 hsync_period_h;
	}r1;
	struct {
		u16 hsync_width;
	}r2;
	struct {
		u16 hsync_offset;
	}r3;
	struct {
		u16 vsync_period;
	}r4;
	struct {
		u16 vsync_width;
	}r5;
	struct {
		u16 vsync_offset;
	}r6;
	struct {
		u16 hsync_polarity : 1;
		u16 vsync_polarity : 1;
		u16 no_vb_hsync : 1;
		u16 intr_mode : 1;
		u16 vsync_width_unit : 1;
		u16 num_vsync : 8;
		u16 continuous : 1;
		u16 preempt : 1;
		u16 reserved : 1;
	}r7;
};

struct iav_vin_format {
	u32 interface_type;
	struct iav_rect vin_win;
	struct iav_rect act_win;
	struct iav_window max_act_win;
	u32 frame_rate;
	u32 video_format;
	u32 bit_resolution;
	u32 sensor_id;
	u32 bayer_pattern;
	u32 input_format;
	u32 readout_mode;
	u32 sync_mode;
};

struct ambarella_vin_irq {
	const char *name;
	int irq_no;
	irqreturn_t (*irq_handler)(int irqno, void *dev_id);
	char irq_name[32];
};

struct vin_controller {
	int id;
	char name[32];
	struct ambarella_iav *iav;

	int rst_gpio;
	u8 rst_gpio_active;
	/* no sensors should need power control gpios more than 3 */
	int pwr_gpio[3];
	u8 pwr_gpio_active[3];

	wait_queue_head_t wq;
	atomic_t wq_flag, wdr_wq_flag;
	u32 irq_counter;
	u32 tmo;

	u32 vsync_delay;			/* in unit of ns */
	struct hrtimer vsync_timer;
	struct list_head list;

	struct mutex dev_mutex;
	struct list_head dev_list;		/* used to list vin source */
	struct vin_device *dev_active;
	struct vin_dsp_config *dsp_config;	/* used by dsp */
	struct vin_master_config *master_config; /* used by dsp */
	/* VIN device formamt: necessary information used by software */
	struct iav_vin_format vin_format;

	struct ambsync_proc_hinfo proc_hinfo;

	int (*ioctl)(struct vin_controller *vinc,
			unsigned int cmd, unsigned long args);
};


void rct_set_so_clk_src(u32 mode);
void rct_set_so_freq_hz(u32 freq_hz);
u32 rct_get_so_freq_hz(void);

int iav_vin_init(struct ambarella_iav *iav);
void iav_vin_exit(struct ambarella_iav *iav);
int vin_pm_suspend(struct vin_device *vdev);
int vin_pm_resume(struct vin_device *vdev);

int iav_vin_idsp_cross_check(struct ambarella_iav *iav);
int iav_vin_get_idsp_frame_rate(struct ambarella_iav *iav, u32 *idsp_out_frame_rate);
int iav_vin_update_stream_framerate(struct ambarella_iav *iav);
#endif

