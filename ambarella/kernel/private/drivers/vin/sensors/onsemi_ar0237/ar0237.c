/*
 * Filename : ar0237.c
 *
 * History:
 *    2015/09/07 - [Hao Zeng] Create
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
#include "ar0237.h"
#include "ar0237_table.c"

static int bus_addr = (0 << 16) | (0x20 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ar0237_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 frame_length_lines;
	u32 line_length;
};

static int ar0237_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ar0237_priv *ar0237;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ar0237 = (struct ar0237_priv *)vdev->priv;
	client = ar0237->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = (data & 0xff00) >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == AR0237_RESET_REGISTER))
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

static int ar0237_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ar0237_priv *ar0237;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	ar0237 = (struct ar0237_priv *)vdev->priv;
	client = ar0237->control_data;

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

static int ar0237_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ar0237_config;

	memset(&ar0237_config, 0, sizeof (ar0237_config));

	ar0237_config.interface_type = SENSOR_SERIAL_LVDS;
	ar0237_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ar0237_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	ar0237_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_HISPI;

	ar0237_config.cap_win.x = format->def_start_x;
	ar0237_config.cap_win.y = format->def_start_y;
	ar0237_config.cap_win.width = format->def_width;
	ar0237_config.cap_win.height = format->def_height;

	ar0237_config.hdr_cfg.act_win.x = format->act_start_x;
	ar0237_config.hdr_cfg.act_win.y = format->act_start_y;
	ar0237_config.hdr_cfg.act_win.width = format->act_width;
	ar0237_config.hdr_cfg.act_win.height = format->act_height;
	ar0237_config.hdr_cfg.act_win.max_width = format->max_act_width;
	ar0237_config.hdr_cfg.act_win.max_height = format->max_act_height;

	ar0237_config.sensor_id	= GENERIC_SENSOR;
	ar0237_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ar0237_config.bayer_pattern	= format->bayer_pattern;
	ar0237_config.video_format	= format->format;
	ar0237_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ar0237_config);
}

static void ar0237_sw_reset(struct vin_device *vdev)
{
	ar0237_write_reg(vdev, AR0237_RESET_REGISTER, 0x0001);/* Register RESET_REGISTER */
	msleep(1);
	ar0237_write_reg(vdev, AR0237_RESET_REGISTER, 0x10D8);/* Register RESET_REGISTER */
}

static int ar0237_init_device(struct vin_device *vdev)
{
	ar0237_sw_reset(vdev);
	return 0;
}

static int ar0237_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = ar0237_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ar0237_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		ar0237_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void ar0237_start_streaming(struct vin_device *vdev)
{
	u32 data;
	ar0237_read_reg(vdev, AR0237_RESET_REGISTER, &data);
	data = (data | 0x0004);
	ar0237_write_reg(vdev, AR0237_RESET_REGISTER, data);/* start streaming */
}

static int ar0237_update_hv_info(struct vin_device *vdev)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	ar0237_read_reg(vdev, AR0237_LINE_LENGTH_PCK, &pinfo->line_length);
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ar0237_read_reg(vdev, AR0237_FRAME_LENGTH_LINES, &pinfo->frame_length_lines);

	return 0;
}

static int ar0237_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ar0237_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num = 0, rval;
	struct ar0237_priv *pinfo;

	pinfo = (struct ar0237_priv *)vdev->priv;

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		regs = ar0237_linear_share_regs;
		regs_num = ARRAY_SIZE(ar0237_linear_share_regs);
		for (i = 0; i < regs_num; i++)
			ar0237_write_reg(vdev, regs[i].addr, regs[i].data);

		regs = ar0237_mode_regs[format->device_mode];
		regs_num = ARRAY_SIZE(ar0237_mode_regs[format->device_mode]);
		for (i = 0; i < regs_num; i++)
			ar0237_write_reg(vdev, regs[i].addr, regs[i].data);
		/* mapping from 0db */
		vdev->agc_db_max = 0x2C700000;	/* 44.4375dB */
		vdev->agc_db_min = 0x00000000;	/* 0dB */
		vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
	} else {
		regs = ar0237_2x_hdr_share_regs;
		regs_num = ARRAY_SIZE(ar0237_2x_hdr_share_regs);
		for (i = 0; i < regs_num; i++)
			ar0237_write_reg(vdev, regs[i].addr, regs[i].data);
		/* mapping from 0db */
		vdev->agc_db_max = 0x1A700000;	/* 26.4375dB */
		vdev->agc_db_min = 0x00000000;	/* 0dB */
		vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
	}

	rval = ar0237_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ar0237_get_line_time(vdev);

	/* Enable Streaming */
	ar0237_start_streaming(vdev);

	/* communiate with IAV */
	rval = ar0237_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ar0237_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 4) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	ar0237_write_reg(vdev, AR0237_COARSE_INTEGRATION_TIME, num_line);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ar0237_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;
	u32 t1t2_ratio, shutter_long, shutter_short;
	u32 use_reg;
	u64 vb_time;
	int errCode = 0;

	ar0237_read_reg(vdev, 0x3082, &t1t2_ratio);
	use_reg = t1t2_ratio&0x0800; /* bit 11 */
	t1t2_ratio = 1<<(((t1t2_ratio&0xC)>>2) + 2);

	/* T1 */
	shutter_long = p_shutter_gp->l;
	/* T2 */
	shutter_short = use_reg ? p_shutter_gp->s1 : (shutter_long/t1t2_ratio);

	/* shutter limitation check */
	if ((shutter_long + shutter_short + 2 > pinfo->frame_length_lines) ||
		(1080 + shutter_short + 2 > pinfo->frame_length_lines)) {
		vin_error("shutter exceeds limitation! long:%d, short:%d, V:%d, ratio:%d\n",
			shutter_long, shutter_short, pinfo->frame_length_lines, t1t2_ratio);
		return -EPERM;
	}

	/* T1 */
	ar0237_write_reg(vdev, AR0237_COARSE_INTEGRATION_TIME, shutter_long);
	if (use_reg) /* T2 */
		ar0237_write_reg(vdev, AR0237_COARSE_INTEGRATION_TIME2, shutter_short);

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	vb_time = pinfo->frame_length_lines - vdev->cur_format->height - pinfo->wdr_shutter_gp.s1;
	vb_time = pinfo->line_length * (u64)vb_time * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	vin_debug("shutter long:%d, short:%d, ratio:%d, vb_time:%d\n",
		shutter_long, shutter_short, t1t2_ratio, vdev->cur_format->vb_time);

	return errCode;
}

static int ar0237_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int ar0237_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (unlikely(!pinfo->line_length)) {
		rval = ar0237_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ar0237_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ar0237_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	u32 vsync_delay;
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ar0237_write_reg(vdev, AR0237_FRAME_LENGTH_LINES, v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		/* FIXME: make T1 and T2 effective at n+2 frame after T2 sync point */
		vsync_delay = (u32)DIV64_CLOSEST((u64)vdev->cur_format->line_time * 1000, 512);
		vsync_delay *= (pinfo->frame_length_lines - 1080 - 20 - 8);
		ambarella_vin_vsync_delay(vdev, vsync_delay);
	}

	return 0;
}

static int ar0237_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > AR0237_GAIN_48DB) {
		vin_error("agc index %d exceeds maximum %d\n", agc_idx, AR0237_GAIN_48DB);
		return -EINVAL;
	}

	ar0237_write_reg(vdev, AR0237_AGAIN, AR0237_GAIN_TABLE[agc_idx][AR0237_GAIN_COL_AGAIN]);
	ar0237_write_reg(vdev, AR0237_DGAIN, AR0237_GAIN_TABLE[agc_idx][AR0237_GAIN_COL_DGAIN]);
	ar0237_write_reg(vdev, AR0237_DCG_CTL, AR0237_GAIN_TABLE[agc_idx][AR0237_GAIN_COL_DCG]);

	return 0;
}

static int ar0237_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;
	int gain_index;

	gain_index = p_again_gp->l;

	ar0237_write_reg(vdev, AR0237_AGAIN, AR0237_HDR_GAIN_TABLE[gain_index][AR0237_GAIN_COL_AGAIN]);
	ar0237_write_reg(vdev, AR0237_DGAIN, AR0237_HDR_GAIN_TABLE[gain_index][AR0237_GAIN_COL_DGAIN]);
	ar0237_write_reg(vdev, AR0237_DCG_CTL, AR0237_HDR_GAIN_TABLE[gain_index][AR0237_GAIN_COL_DCG]);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));
	vin_debug("hdr again index:%d\n", p_again_gp->l);
	return 0;
}

static int ar0237_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ar0237_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	return 0;
}

static int ar0237_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	return 0;
}

static int ar0237_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	return ar0237_shutter2row(vdev, &p_shutter2row->l);
}

static int ar0237_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = AR0237_H_MIRROR + AR0237_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = AR0237_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = AR0237_V_FLIP;
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

	ar0237_read_reg(vdev, AR0237_READ_MODE, &tmp_reg);
	tmp_reg &= (~AR0237_MIRROR_MASK);
	tmp_reg |= readmode;
	ar0237_write_reg(vdev, AR0237_READ_MODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int ar0237_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct ar0237_priv *pinfo = (struct ar0237_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 4;
	aaa_info->sht1_max = pinfo->frame_length_lines - vdev->cur_format->height - 2;
	aaa_info->sht2_max = 0;

	return 0;
}

#ifdef CONFIG_PM
static int ar0237_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ar0237_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u16)tmp;
	}

	return 0;
}

static int ar0237_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ar0237_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops ar0237_ops = {
	.init_device		= ar0237_init_device,
	.set_pll			= ar0237_set_pll,
	.set_format		= ar0237_set_format,
	.set_shutter_row	= ar0237_set_shutter_row,
	.shutter2row		= ar0237_shutter2row,
	.set_frame_rate	= ar0237_set_fps,
	.set_agc_index		= ar0237_set_agc_index,
	.set_mirror_mode	= ar0237_set_mirror_mode,
	.get_aaa_info		= ar0237_get_aaa_info,
	.read_reg			= ar0237_read_reg,
	.write_reg		= ar0237_write_reg,
#ifdef CONFIG_PM
	.suspend		= ar0237_suspend,
	.resume 		= ar0237_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = ar0237_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = ar0237_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = ar0237_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = ar0237_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = ar0237_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = ar0237_get_wdr_shutter_row_group,
	.wdr_shutter2row = ar0237_wdr_shutter2row,
};

/* ========================================================================== */
static int ar0237_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ar0237_priv *ar0237;
	u32 version;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_AR0237, sizeof(struct ar0237_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2C700000; /* 44.4375dB */
	vdev->agc_db_min = 0x00000000; /* 0dB */
	vdev->agc_db_step = 0x00180000;/* 0.09375dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = AR0237_GAIN_48DB;

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	ar0237 = (struct ar0237_priv *)vdev->priv;
	ar0237->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ar0237_ops,
			ar0237_formats, ARRAY_SIZE(ar0237_formats),
			ar0237_plls, ARRAY_SIZE(ar0237_plls));
	if (rval < 0)
		goto ar0237_probe_err;

	/* query sensor id */
	ar0237_read_reg(vdev, AR0237_CHIP_VERSION_REG, &version);
	vin_info("AR0237 init(4-lane HISPI), sensor ID: 0x%x\n", version);

	return 0;

ar0237_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ar0237_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ar0237_idtable[] = {
	{ "ar0237", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0237_idtable);

static struct i2c_driver i2c_driver_ar0237 = {
	.driver = {
		.name	= "ar0237",
	},

	.id_table	= ar0237_idtable,
	.probe		= ar0237_probe,
	.remove		= ar0237_remove,

};

static int __init ar0237_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ar0237", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ar0237);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ar0237_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0237);
}

module_init(ar0237_init);
module_exit(ar0237_exit);

MODULE_DESCRIPTION("AR0237 1/2.7 -Inch, 1928x1088, 2.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

