/*
 * Filename : ar0230.c
 *
 * History:
 *    2014/11/18 - [Long Zhao] Create
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

#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "ar0230.h"
#include "ar0230_table.c"

static int bus_addr = (0 << 16) | (0x20 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ar0230_priv {
	void *control_data;
	u32	dgain_r_ratio;
	u32	dgain_b_ratio;
	u16	dgain_base;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 frame_length_lines;
	u32 line_length;
};

static int ar0230_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ar0230_priv *ar0230;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ar0230 = (struct ar0230_priv *)vdev->priv;
	client = ar0230->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = (data & 0xff00) >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == AR0230_RESET_REGISTER))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
		msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}

	return 0;
}

static int ar0230_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ar0230_priv *ar0230;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	ar0230 = (struct ar0230_priv *)vdev->priv;
	client = ar0230->control_data;

	pbuf0[0] = (subaddr & 0xff00) >> 8;
	pbuf0[1] = subaddr & 0xff;

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 2;

	rval = i2c_transfer(client->adapter, msgs, 2);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = (pbuf[0] << 8) | pbuf[1];

	return 0;
}

static int ar0230_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ar0230_config;

	memset(&ar0230_config, 0, sizeof(ar0230_config));

	ar0230_config.interface_type = SENSOR_SERIAL_LVDS;
	ar0230_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ar0230_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	ar0230_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_HISPI;

	ar0230_config.cap_win.x = format->def_start_x;
	ar0230_config.cap_win.y = format->def_start_y;
	ar0230_config.cap_win.width = format->def_width;
	ar0230_config.cap_win.height = format->def_height;

	ar0230_config.hdr_cfg.act_win.x = format->act_start_x;
	ar0230_config.hdr_cfg.act_win.y = format->act_start_y;
	ar0230_config.hdr_cfg.act_win.width = format->act_width;
	ar0230_config.hdr_cfg.act_win.height = format->act_height;
	ar0230_config.hdr_cfg.act_win.max_width = format->max_act_width;
	ar0230_config.hdr_cfg.act_win.max_height = format->max_act_height;

	ar0230_config.sensor_id	= GENERIC_SENSOR;
	ar0230_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ar0230_config.bayer_pattern	= format->bayer_pattern;
	ar0230_config.video_format	= format->format;
	ar0230_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ar0230_config);
}

static void ar0230_sw_reset(struct vin_device *vdev)
{
	ar0230_write_reg(vdev, AR0230_RESET_REGISTER, 0x0001);/* Register RESET_REGISTER */
	msleep(1);
	ar0230_write_reg(vdev, AR0230_RESET_REGISTER, 0x10D8);/* Register RESET_REGISTER */
}

static int ar0230_init_device(struct vin_device *vdev)
{
	ar0230_sw_reset(vdev);
	return 0;
}

static int ar0230_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = ar0230_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ar0230_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		ar0230_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void ar0230_start_streaming(struct vin_device *vdev)
{
	u32 data;
	ar0230_read_reg(vdev, AR0230_RESET_REGISTER, &data);
	data = (data | 0x0004);
	ar0230_write_reg(vdev, AR0230_RESET_REGISTER, data);/* start streaming */
}

static int ar0230_update_hv_info(struct vin_device *vdev)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	ar0230_read_reg(vdev, AR0230_LINE_LENGTH_PCK, &pinfo->line_length);
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ar0230_read_reg(vdev, AR0230_FRAME_LENGTH_LINES, &pinfo->frame_length_lines);

	return 0;
}

static int ar0230_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ar0230_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num = 0, rval;
	struct ar0230_priv *pinfo;

	pinfo = (struct ar0230_priv *)vdev->priv;

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		regs = ar0230_linear_share_regs;
		regs_num = ARRAY_SIZE(ar0230_linear_share_regs);
		/* mapping from 0db */
		vdev->agc_db_max = 0x2C700000;	/* 44.4375dB */
		vdev->agc_db_min = 0x00000000;	/* 0dB */
		vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
	} else {
		if (format->hdr_mode == AMBA_VIDEO_INT_HDR_MODE) {
			regs = ar0230_hdr_share_regs;
			regs_num = ARRAY_SIZE(ar0230_hdr_share_regs);
		} else if (format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
			regs = ar0230_2x_hdr_share_regs;
			regs_num = ARRAY_SIZE(ar0230_2x_hdr_share_regs);
		}
		/* mapping from 0db */
		vdev->agc_db_max = 0x1A700000;	/* 26.4375dB */
		vdev->agc_db_min = 0x00000000;	/* 0dB */
		vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
		pinfo->dgain_r_ratio = 1024; /* 1x */
		pinfo->dgain_b_ratio = 1024; /* 1x */
	}

	for (i = 0; i < regs_num; i++)
		ar0230_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ar0230_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ar0230_get_line_time(vdev);

	/* Enable Streaming */
	ar0230_start_streaming(vdev);

	/* communiate with IAV */
	rval = ar0230_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ar0230_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 t1t2_ratio;
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int max_shutter_width;
	int errCode = 0;
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	num_line = row;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_INT_HDR_MODE) {
		ar0230_read_reg(vdev, 0x3082, &t1t2_ratio);
		t1t2_ratio = 1<<(((t1t2_ratio&0xC)>>2) + 2);
		max_shutter_width = MIN(70 * t1t2_ratio, pinfo->frame_length_lines - 71);

		min_line = t1t2_ratio/2;
		max_line = max_shutter_width;
		num_line = clamp(num_line, min_line, max_line);
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		/* FIXME: shutter width: 1 ~ (Frame format(V) - 4) */
		min_line = 1;
		max_line = pinfo->frame_length_lines - 4;
		num_line = clamp(num_line, min_line, max_line);
	}

	ar0230_write_reg(vdev, AR0230_COARSE_INTEGRATION_TIME, num_line);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ar0230_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;
	u32 t1t2_ratio, shutter_long, shutter_short;
	u64 vb_time;
	int errCode = 0;

	ar0230_read_reg(vdev, 0x3082, &t1t2_ratio);
	t1t2_ratio = 1<<(((t1t2_ratio&0xC)>>2) + 2);

	/* T1 */
	shutter_long = p_shutter_gp->l;
	/* T2 */
	shutter_short = shutter_long/t1t2_ratio;

	/* shutter limitation check */
	if ((shutter_long + shutter_short + 2 > pinfo->frame_length_lines) ||
		(1080 + shutter_short + 2 > pinfo->frame_length_lines)) {
		vin_error("shutter exceeds limitation! long:%d, short:%d, V:%d, ratio:%d\n",
			shutter_long, shutter_short, pinfo->frame_length_lines, t1t2_ratio);
		return -EPERM;
	}

	/* T1 */
	ar0230_write_reg(vdev, AR0230_COARSE_INTEGRATION_TIME, shutter_long);

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	vb_time = pinfo->frame_length_lines - vdev->cur_format->height - pinfo->wdr_shutter_gp.s1;
	vb_time = pinfo->line_length * (u64)vb_time * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	vin_debug("shutter long:%d, short:%d, ratio:%d, vb_time:%d\n",
		shutter_long, shutter_short, t1t2_ratio, vdev->cur_format->vb_time);

	return errCode;
}

static int ar0230_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int ar0230_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (unlikely(!pinfo->line_length)) {
		rval = ar0230_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ar0230_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ar0230_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ar0230_write_reg(vdev, AR0230_FRAME_LENGTH_LINES, v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ar0230_convert_dgain_ratio(u16 old_dgain, u32 ratio, u16 *new_dgain)
{
	u32 tmp_dgain;

	tmp_dgain = (old_dgain * ratio)/1024;

	if (tmp_dgain > 0x7FF) {/* dgain should be less than 15.992 */
		tmp_dgain = 0x7FF;
		vin_warn("dgain value is too high!\n");
	} else if ((ratio != 0) && (tmp_dgain == 0)) {/* 0<ratio<1/128, set to 1/128 */
		tmp_dgain = 1;
	}
	*new_dgain = tmp_dgain;

	vin_debug("old_dgain=%d, ratio=%d, new_dgain=%d\n", old_dgain, ratio, *new_dgain);

	return 0;
}

static int ar0230_set_dgain_ratio(struct vin_device *vdev, struct vindev_dgain_ratio *p_dgain_ratio)
{
	u16 new_dgain;
	int errCode = 0;
	struct ar0230_priv *pinfo;

	pinfo = (struct ar0230_priv *)vdev->priv;

	/* r */
	if (p_dgain_ratio->r_ratio == 0)
		vin_warn("Warning:R dgain ratio is set to 0!\n");
	ar0230_convert_dgain_ratio(pinfo->dgain_base, p_dgain_ratio->r_ratio, &new_dgain);
	ar0230_write_reg(vdev, AR0230_R_GAIN, new_dgain);
	pinfo->dgain_r_ratio = p_dgain_ratio->r_ratio;

	/* gr */
	if (p_dgain_ratio->gr_ratio == 0)
		vin_warn("Warning:Gr dgain ratio is set to 0!\n");
	ar0230_convert_dgain_ratio(pinfo->dgain_base, p_dgain_ratio->gr_ratio, &new_dgain);
	ar0230_write_reg(vdev, AR0230_GR_GAIN, new_dgain);

	/* gb */
	if (p_dgain_ratio->gb_ratio == 0)
		vin_warn("Warning:Gb dgain ratio is set to 0!\n");
	ar0230_convert_dgain_ratio(pinfo->dgain_base, p_dgain_ratio->gb_ratio, &new_dgain);
	ar0230_write_reg(vdev, AR0230_GB_GAIN, new_dgain);

	/* b */
	if (p_dgain_ratio->b_ratio == 0)
		vin_warn("Warning:B dgain ratio is set to 0!\n");
	ar0230_convert_dgain_ratio(pinfo->dgain_base, p_dgain_ratio->b_ratio, &new_dgain);
	ar0230_write_reg(vdev, AR0230_B_GAIN, new_dgain);
	pinfo->dgain_b_ratio = p_dgain_ratio->b_ratio;

	return errCode;
}

static int ar0230_get_dgain_ratio(struct vin_device *vdev, struct vindev_dgain_ratio *p_dgain_ratio)
{
	int errCode = 0;
	u32 current_dgain = 0, dgain_ratio;

	/* r */
	ar0230_read_reg(vdev, AR0230_R_GAIN, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	p_dgain_ratio->r_ratio = dgain_ratio;

	/* gr */
	ar0230_read_reg(vdev, AR0230_GR_GAIN, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	p_dgain_ratio->gr_ratio = dgain_ratio;

	/* gb */
	ar0230_read_reg(vdev, AR0230_GB_GAIN, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	p_dgain_ratio->gb_ratio = dgain_ratio;

	/* b */
	ar0230_read_reg(vdev, AR0230_B_GAIN, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	p_dgain_ratio->b_ratio = dgain_ratio;

	return errCode;
}

static int ar0230_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	int agc_max_index;
	u16 new_dgain, again;
	struct ar0230_priv *pinfo;

	pinfo = (struct ar0230_priv *)vdev->priv;

	if  (vdev->cur_format->hdr_mode == AMBA_VIDEO_INT_HDR_MODE)
		agc_max_index = AR0230_GAIN_30DB;
	else
		agc_max_index = AR0230_GAIN_48DB;

	if (agc_idx > agc_max_index) {
		vin_error("agc index %d exceeds maximum %d\n", agc_idx, agc_max_index);
		return -EINVAL;
	}

	if (agc_idx < AR0230_GAIN_DCG_ON) {
		/* Low Conversion Gain */
		ar0230_write_reg(vdev, 0x3206, 0x0B08);/* ADACD_NOISE_FLOOR1 */
		ar0230_write_reg(vdev, 0x3208, 0x1E13);/* ADACD_NOISE_FLOOR2 */
		ar0230_write_reg(vdev, 0x3202, 0x0080);/* ADACD_NOISE_MODEL1 */
		if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE) {
			ar0230_write_reg(vdev, 0x3096, 0x0480);/* ROW_NOISE_ADJUST_TOP */
			ar0230_write_reg(vdev, 0x3098, 0x0480);/* ROW_NOISE_ADJUST_BTM */
		}
	} else {
		/* High Conversion Gain */
		ar0230_write_reg(vdev, 0x3206, 0x1C0E);/* ADACD_NOISE_FLOOR1 */
		ar0230_write_reg(vdev, 0x3208, 0x4E39);/* ADACD_NOISE_FLOOR2 */
		ar0230_write_reg(vdev, 0x3202, 0x00B0);/* ADACD_NOISE_MODEL1 */
		if  (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE) {
			ar0230_write_reg(vdev, 0x3096, 0x0780);/* ROW_NOISE_ADJUST_TOP */
			ar0230_write_reg(vdev, 0x3098, 0x0780);/* ROW_NOISE_ADJUST_BTM */
		}
	}

	if  (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		ar0230_write_reg(vdev, AR0230_AGAIN,
			AR0230_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_AGAIN]);
		ar0230_write_reg(vdev, AR0230_DGAIN,
			AR0230_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DGAIN]);
		ar0230_write_reg(vdev, AR0230_DCG_CTL,
			AR0230_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DCG]);
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		ar0230_write_reg(vdev, AR0230_AGAIN,
			AR0230_HDR_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_AGAIN]);
		ar0230_write_reg(vdev, AR0230_DGAIN,
			AR0230_HDR_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DGAIN]);
		ar0230_write_reg(vdev, AR0230_DCG_CTL,
			AR0230_HDR_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DCG]);
	} else {
		again = AR0230_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_AGAIN];
		ar0230_write_reg(vdev, AR0230_AGAIN, again);
		ar0230_write_reg(vdev, AR0230_DCG_CTL,
			AR0230_HDR_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DCG]);

		pinfo->dgain_base = AR0230_GAIN_TABLE[agc_idx][AR0230_GAIN_COL_DGAIN];

		/* r dgain */
		ar0230_convert_dgain_ratio(pinfo->dgain_base, pinfo->dgain_r_ratio, &new_dgain);
		ar0230_write_reg(vdev, AR0230_R_GAIN, new_dgain);

		/* gr/gb dgain, ratio is fixed to 1 */
		ar0230_write_reg(vdev, AR0230_GR_GAIN, pinfo->dgain_base);
		ar0230_write_reg(vdev, AR0230_GB_GAIN, pinfo->dgain_base);

		/* b dgain */
		ar0230_convert_dgain_ratio(pinfo->dgain_base, pinfo->dgain_b_ratio, &new_dgain);
		ar0230_write_reg(vdev, AR0230_B_GAIN, new_dgain);
	}

	return 0;
}

static int ar0230_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;
	int again_index;

	again_index = p_again_gp->l;

	ar0230_set_agc_index(vdev, again_index);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));
	vin_debug("hdr again index:%d\n", p_again_gp->l);
	return 0;
}

static int ar0230_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ar0230_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	return 0;
}

static int ar0230_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	return 0;
}

static int ar0230_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	return ar0230_shutter2row(vdev, &p_shutter2row->l);
}

static int ar0230_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = AR0230_H_MIRROR + AR0230_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = AR0230_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = AR0230_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ar0230_read_reg(vdev, AR0230_READ_MODE, &tmp_reg);
	tmp_reg &= (~AR0230_MIRROR_MASK);
	tmp_reg |= readmode;
	ar0230_write_reg(vdev, AR0230_READ_MODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int ar0230_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct ar0230_priv *pinfo = (struct ar0230_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 4;
	aaa_info->sht1_max = pinfo->frame_length_lines - vdev->cur_format->height - 2;
	aaa_info->sht2_max = 0;

	return 0;
}

#ifdef CONFIG_PM
static int ar0230_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ar0230_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u16)tmp;
	}

	return 0;
}

static int ar0230_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ar0230_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops ar0230_ops = {
	.init_device		= ar0230_init_device,
	.set_pll			= ar0230_set_pll,
	.set_format		= ar0230_set_format,
	.set_shutter_row	= ar0230_set_shutter_row,
	.shutter2row		= ar0230_shutter2row,
	.set_frame_rate	= ar0230_set_fps,
	.set_agc_index		= ar0230_set_agc_index,
	.set_mirror_mode	= ar0230_set_mirror_mode,
	.set_dgain_ratio	= ar0230_set_dgain_ratio,
	.get_dgain_ratio	= ar0230_get_dgain_ratio,
	.get_aaa_info		= ar0230_get_aaa_info,
	.read_reg			= ar0230_read_reg,
	.write_reg		= ar0230_write_reg,
#ifdef CONFIG_PM
	.suspend		= ar0230_suspend,
	.resume			= ar0230_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = ar0230_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = ar0230_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = ar0230_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = ar0230_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = ar0230_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = ar0230_get_wdr_shutter_row_group,
	.wdr_shutter2row = ar0230_wdr_shutter2row,
};

/* ========================================================================== */
static int ar0230_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ar0230_priv *ar0230;
	u32 version;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_AR0230, sizeof(struct ar0230_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2C700000;	/* 44.4375dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = AR0230_GAIN_48DB;

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	ar0230 = (struct ar0230_priv *)vdev->priv;
	ar0230->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ar0230_ops,
			ar0230_formats, ARRAY_SIZE(ar0230_formats),
			ar0230_plls, ARRAY_SIZE(ar0230_plls));
	if (rval < 0)
		goto ar0230_probe_err;

	/* query sensor id */
	ar0230_read_reg(vdev, AR0230_CHIP_VERSION_REG, &version);
	vin_info("AR0230 init(4-lane HISPI), sensor ID: 0x%x\n", version);

	return 0;

ar0230_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ar0230_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ar0230_idtable[] = {
	{ "ar0230", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0230_idtable);

static struct i2c_driver i2c_driver_ar0230 = {
	.driver = {
		.name	= "ar0230",
	},

	.id_table	= ar0230_idtable,
	.probe		= ar0230_probe,
	.remove		= ar0230_remove,

};

static int __init ar0230_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ar0230", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ar0230);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ar0230_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0230);
}

module_init(ar0230_init);
module_exit(ar0230_exit);

MODULE_DESCRIPTION("AR0230 1/2.7 -Inch, 1928x1088, 2.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

