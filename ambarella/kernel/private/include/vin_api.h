/*
 * kernel/private/include/vin_api.h
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
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


#ifndef __VIN_PRI_H
#define __VIN_PRI_H

#include <linux/list.h>
#include <iav_ioctl.h>
#include <vin_sensors.h>

typedef enum {
	AMBA_VIN_INPUT_FORMAT_RGB_RAW = 0,
	AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC = 1,
	AMBA_VIN_INPUT_FORMAT_YUV_422_PROG = 2,
	AMBA_VIN_INPUT_FORMAT_DRAM_INTLC = 3,
	AMBA_VIN_INPUT_FORMAT_DRAM_PROG = 4,
} AMBA_VIN_INPUT_FORMAT;

struct vin_device_config {
	u8 interface_type;
	u8 video_format;
	u8 bit_resolution;
	u8 sensor_id;

	u8 bayer_pattern;
	u8 input_format;
	u8 readout_mode;
	u8 yuv_pixel_order;

	u8 sync_mode;
	u8 input_mode;
	union {
		/* for serial lvds */
		struct {
			u32 lane_number : 8;
			u32 sync_code_style : 8;
			u32 lane_mux_0 : 16;
			u32 lane_mux_1 : 16;
			u32 lane_mux_2 : 16;
		} slvds_cfg;
		/* for parallel lvds */
		struct {
			u32 sync_code_style : 8;
			u32 reserved0 : 24;
			u32 reserved1;
		} plvds_cfg;
		/* for parallel lvcmos */
		struct {
			u32 paralle_sync_type : 8;
			u32 sync_code_style : 8;
			u32 vs_hs_polarity : 8;
			u32 data_edge : 8;
			u32 reserved;
		} plvcmos_cfg;
		/* for mipi */
		struct {
			u32 lane_number : 8;
			u32 bit_rate : 8;
			u32 reserved0 : 16;
			u32 reserved1;
		} mipi_cfg;
	};
	/* for hdr sensor */
	struct {
		struct {
			u32 x : 16;
			u32 y : 16;
			u32 width : 16;
			u32 height : 16;
			u32 max_width : 16;
			u32 max_height : 16;
		} act_win;

		u32 split_width;
	} hdr_cfg;
	/* for capture window */
	struct {
		u32 x : 16;
		u32 y : 16;
		u32 width : 16;
		u32 height : 16;
	} cap_win;
};

struct vin_wdr_gp_info {
	struct task_struct	*kthread;
	struct vindev_wdr_gp_s shutter_gp;
	struct vindev_wdr_gp_s again_gp;
	struct vindev_wdr_gp_s dgain_gp;
	u32 update_flag;
	u32 killing_kthread;
	u32 wake_flag;
};

struct vin_device {
	int vsrc_id;
	const char *name;
	struct device *dev;
	u32 intf_id;
	u32 dev_type;
	u32 sub_type;
	u32 sensor_id;
	u32 pixel_size;

	struct vin_ops *ops;
	struct list_head list;

	int default_mode;
	int default_hdr_mode;
	int frame_rate;
	int shutter_time;
	int agc_db;
	int agc_db_max;
	int agc_db_min;
	int agc_db_step;
	int wdr_again_idx_min;
	int wdr_again_idx_max;
	int wdr_dgain_idx_min;
	int wdr_dgain_idx_max;

	struct vin_video_format *formats;
	u32 num_formats;
	struct vin_video_format *cur_format;
	struct vin_video_pll *plls;
	u32 num_plls;
	struct vin_video_pll *cur_pll;
	struct vin_precise_fps *p_fps;
	u32 num_p_fps;
	u32 pre_video_mode;
	u32 pre_hdr_mode;
	u32 pre_bits;
	struct vin_wdr_gp_info wdr_gp;
	bool reset_for_mode_switch;
	unsigned long priv[0];
};

struct vin_reg_16_8 {
	u16 addr;
	u8 data;
};

struct vin_reg_16_16 {
	u16 addr;
	u16 data;
};

struct vin_video_pll {
	u32 mode;
	u32 clk_si;	/* output to sensor */
	u32 pixelclk;	/* input from sensor */
};

struct vin_precise_fps {
	int fps;
	int video_mode;
	int pll_idx;
};

/* for slave sensor */
struct vin_master_sync {
	u32 hsync_period;
	u32 hsync_width : 16;
	u32 vsync_period : 16;
	u32 vsync_width : 16;
	u32 reserved0 : 16;
};

/* Sync to amboot/include/dsp/s2l_cmd_msg.h */
struct vin_video_format {
	u32 video_mode;
	u32 device_mode;
	u32 pll_idx;	/* clock index */
	u16 width;	/* image horizontal size in unit of pixel */
	u16 height;	/* image vertical size in unit of line */

	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u8 format;
	u8 type;
	u8 bits;
	u8 ratio;
	u8 mirror_pattern;
	u8 bayer_pattern;
	u8 hdr_mode;
	u8 readout_mode;
	u32 line_time;
	u32 vb_time;

	u32 max_fps;
	int default_fps;
	int default_agc;
	int default_shutter_time;
	int default_bayer_pattern;

	/* hdr mode related */
	u16 act_start_x;
	u16 act_start_y;
	u16 act_width;
	u16 act_height;
	u16 max_act_width;
	u16 max_act_height;
	u16 hdr_long_offset;
	u16 hdr_short1_offset;
	u16 hdr_short2_offset;
	u16 hdr_short3_offset;
	u16 dual_gain_mode;
};

struct vin_ops {
	int (*init_device)(struct vin_device *vdev);
	int (*suspend)(struct vin_device *vdev);
	int (*resume)(struct vin_device *vdev);
	int (*set_format)(struct vin_device *vdev, struct vin_video_format *format);
	int (*set_frame_rate)(struct vin_device *vdev, int fps);
	int (*set_agc_index)(struct vin_device *vdev, int idx);
	int (*set_pll)(struct vin_device *vdev, int pll_idx);
	int (*set_mirror_mode)(struct vin_device *vdev, struct vindev_mirror *args);
	int (*set_operation_mode)(struct vin_device *vdev, u32 op_mode);
	int (*get_eis_info)(struct vin_device *vdev, struct vindev_eisinfo *args);
	int (*read_reg)(struct vin_device *vdev, u32 reg, u32 *data);
	int (*write_reg)(struct vin_device *vdev, u32 reg, u32 data);
	int (*shutter2row)(struct vin_device *vdev, u32 *shutter_time);
	int (*set_shutter_row)(struct vin_device *vdev, u32 shutter_row);
	int (*set_dgain_ratio)(struct vin_device *vdev, struct vindev_dgain_ratio *args);
	int (*get_dgain_ratio)(struct vin_device *vdev, struct vindev_dgain_ratio *args);
	int (*set_hold_mode)(struct vin_device *vdev, u32 hold_mode);
	int (*get_chip_status)(struct vin_device *vdev, struct vindev_chip_status *args);
	int (*get_aaa_info)(struct vin_device *vdev, struct vindev_aaa_info *args);

	/* WDR control */
	int (*set_wdr_again_idx_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);
	int (*get_wdr_again_idx_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);

	int (*set_wdr_dgain_idx_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);
	int (*get_wdr_dgain_idx_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);

	int (*set_wdr_shutter_row_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);
	int (*get_wdr_shutter_row_gp)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);

	int (*wdr_shutter2row)(struct vin_device *vdev, struct vindev_wdr_gp_s *args);

	/* For decoder, run-time report video mode */
	int (*get_format)(struct vin_device *vdev);
};

/* ========================================================================== */
void ambarella_vin_mipi_phy_reset(void);
void ambarella_vin_mipi_phy_enable(u8 lanes);
int ambarella_set_vin_config(struct vin_device *vdev, struct vin_device_config *cfg);
int ambarella_set_vin_master_sync(struct vin_device *vdev,
		struct vin_master_sync *master_cfg, bool by_dbg_bus);
int ambarella_vin_vsync_delay(struct vin_device *vdev, u32 vsync_delay);
void ambarella_vin_add_precise_fps(struct vin_device *vdev,
		struct vin_precise_fps *p_fps, u32 num_p_fps);
struct vin_device *ambarella_vin_create_device(const char *name,
		u32 sensor_id, u32 priv_size);
void ambarella_vin_free_device (struct vin_device *vdev);
int ambarella_vin_register_device(struct vin_device *vdev, struct vin_ops *ops,
		struct vin_video_format *formats, u32 num_formats,
		struct vin_video_pll *plls, u32 num_plls);
int ambarella_vin_unregister_device(struct vin_device *vdev);

#endif //__VIN_PRI_H

