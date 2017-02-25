/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx290/imx290.c
 *
 * History:
 *    2015/03/23 - [Long Zhao] Create
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
#include "imx290.h"

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct imx290_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 frame_length_lines;
	u32 line_length;
	u32 fsc, rhs1, rhs2;
};

#include "imx290_table.c"

static int imx290_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct imx290_priv *imx290;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	imx290 = (struct imx290_priv *)vdev->priv;
	client = imx290->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX290_SWRESET))
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

static int imx290_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct imx290_priv *imx290;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	imx290 = (struct imx290_priv *)vdev->priv;
	client = imx290->control_data;

	pbuf0[0] = (subaddr &0xff00) >> 8;
	pbuf0[1] = subaddr & 0xff;

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 1;

	rval = i2c_transfer(client->adapter, msgs, 2);
	if (rval < 0){
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int imx290_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	imx290_write_reg(vdev, IMX290_REGHOLD, hold_mode);
	return 0;
}

static int imx290_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx290_config;

	memset(&imx290_config, 0, sizeof (imx290_config));

	imx290_config.interface_type = SENSOR_SERIAL_LVDS;
	imx290_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	if (format->video_mode == AMBA_VIDEO_MODE_720P ||
		format->device_mode == 4 ||
		format->device_mode == 7) {/* 1080p 12bits and 720p */
		imx290_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	} else {
		imx290_config.slvds_cfg.lane_number = SENSOR_8_LANE;
	}

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		imx290_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;
	} else {
		imx290_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY_DOL;
		/* use split width to make VIN divide the long expo lines */

		if (format->video_mode == AMBA_VIDEO_MODE_1080P) {
			imx290_config.hdr_cfg.split_width = (format->bits == AMBA_VIDEO_BITS_10)? IMX290_1080P_H_PERIOD : IMX290_1080P_12B_H_PERIOD;
		} else {
			imx290_config.hdr_cfg.split_width = IMX290_720P_H_PERIOD;
		}
	}

	imx290_config.cap_win.x = format->def_start_x;
	imx290_config.cap_win.y = format->def_start_y;
	imx290_config.cap_win.width = format->def_width;
	imx290_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	imx290_config.hdr_cfg.act_win.x = format->act_start_x;
	imx290_config.hdr_cfg.act_win.y = format->act_start_y;
	imx290_config.hdr_cfg.act_win.width = format->act_width;
	imx290_config.hdr_cfg.act_win.height = format->act_height;
	imx290_config.hdr_cfg.act_win.max_width = format->max_act_width;
	imx290_config.hdr_cfg.act_win.max_height = format->max_act_height;

	imx290_config.sensor_id	= GENERIC_SENSOR;
	imx290_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx290_config.bayer_pattern = format->bayer_pattern;
	imx290_config.video_format = format->format;
	imx290_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx290_config);
}

static void imx290_sw_reset(struct vin_device *vdev)
{
	imx290_write_reg(vdev, IMX290_STANDBY, 0x1);/* STANDBY */
	imx290_write_reg(vdev, IMX290_SWRESET, 0x1);
	msleep(10);
}

static void imx290_start_streaming(struct vin_device *vdev)
{
	imx290_write_reg(vdev, IMX290_XMSTA, 0x0); /* master mode start */
	imx290_write_reg(vdev, IMX290_STANDBY, 0x0); /* cancel standby */
	msleep(50);
}

static int imx290_init_device(struct vin_device *vdev)
{
	imx290_sw_reset(vdev);
	return 0;
}

static int imx290_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx290_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx290_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx290_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx290_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	imx290_read_reg(vdev, IMX290_HMAX_MSB, &val_high);
	imx290_read_reg(vdev, IMX290_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx290_read_reg(vdev, IMX290_VMAX_HSB, &val_high);
	imx290_read_reg(vdev, IMX290_VMAX_MSB, &val_mid);
	imx290_read_reg(vdev, IMX290_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx290_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx290_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx290_share_regs;
	regs_num = ARRAY_SIZE(imx290_share_regs);
	for (i = 0; i < regs_num; i++)
		imx290_write_reg(vdev, regs[i].addr, regs[i].data);

	regs = imx290_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx290_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		imx290_write_reg(vdev, regs[i].addr, regs[i].data);

	/* for DOL mode, set RHS registers */
	if (format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if (format->video_mode == AMBA_VIDEO_MODE_1080P) {
			pinfo->rhs1 = (format->bits == AMBA_VIDEO_BITS_10) ?
				IMX290_1080P_2X_RHS1 : IMX290_1080P_2X_12B_RHS1;
		} else if (format->video_mode == AMBA_VIDEO_MODE_1080P_A) {
			pinfo->rhs1 = IMX290_1080P_2X_RATIO_RHS1;
		} else {
			pinfo->rhs1 = IMX290_720P_2X_RHS1;
		}
		pinfo->rhs2 = 0;

		imx290_write_reg(vdev, IMX290_RHS1_HSB, pinfo->rhs1 >> 16);
		imx290_write_reg(vdev, IMX290_RHS1_MSB, pinfo->rhs1 >> 8);
		imx290_write_reg(vdev, IMX290_RHS1_LSB, pinfo->rhs1 & 0xff);
	} else if (format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if (format->video_mode == AMBA_VIDEO_MODE_1080P) {
			pinfo->rhs1 = IMX290_1080P_3X_RHS1;
			pinfo->rhs2 = IMX290_1080P_3X_RHS2;
		} else {
			pinfo->rhs1 = IMX290_720P_3X_RHS1;
			pinfo->rhs2 = IMX290_720P_3X_RHS2;
		}
		imx290_write_reg(vdev, IMX290_RHS1_HSB, pinfo->rhs1 >> 16);
		imx290_write_reg(vdev, IMX290_RHS1_MSB, pinfo->rhs1 >> 8);
		imx290_write_reg(vdev, IMX290_RHS1_LSB, pinfo->rhs1 & 0xff);

		imx290_write_reg(vdev, IMX290_RHS2_HSB, pinfo->rhs2 >> 16);
		imx290_write_reg(vdev, IMX290_RHS2_MSB, pinfo->rhs2 >> 8);
		imx290_write_reg(vdev, IMX290_RHS2_LSB, pinfo->rhs2 & 0xff);
	}

	rval = imx290_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx290_get_line_time(vdev);

	/* TG reset release ( Enable Streaming ) */
	imx290_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx290_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx290_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 2) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line - 1;
	imx290_write_reg(vdev, IMX290_SHS1_HSB, blank_lines >> 16);
	imx290_write_reg(vdev, IMX290_SHS1_MSB, blank_lines >> 8);
	imx290_write_reg(vdev, IMX290_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx290_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx290_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx290_set_fps(struct vin_device *vdev, int fps)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;
	u64 v_lines, vb_time;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	pinfo->fsc = v_lines;

	if(vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		v_lines = pinfo->fsc >> 1; /* FSC = VMAX * 2 */
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		v_lines = pinfo->fsc >> 2; /* FSC = VMAX * 4 */
	}

	imx290_write_reg(vdev, IMX290_VMAX_HSB, (v_lines & 0x030000) >> 16);
	imx290_write_reg(vdev, IMX290_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx290_write_reg(vdev, IMX290_VMAX_LSB, v_lines & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(pinfo->fsc - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx290_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	struct imx290_priv *pinfo;
	u32 tmp;

	pinfo = (struct imx290_priv *)vdev->priv;

	/* if gain >= 30db, enable HCG, HCG is 6db */
	if (agc_idx > 100) {
		imx290_read_reg(vdev, IMX290_FRSEL, &tmp);
		tmp |= IMX290_HI_GAIN_MODE;
		imx290_write_reg(vdev, IMX290_FRSEL, tmp);
		vin_debug("high gain mode\n");
		agc_idx -= 20;
	} else {
		imx290_read_reg(vdev, IMX290_FRSEL, &tmp);
		tmp &= ~IMX290_HI_GAIN_MODE;
		imx290_write_reg(vdev, IMX290_FRSEL, tmp);
		vin_debug("low gain mode\n");
	}

	if (agc_idx > IMX290_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX290_GAIN_MAX_DB);
		agc_idx = IMX290_GAIN_MAX_DB;
	}

	imx290_write_reg(vdev, IMX290_GAIN, (u8)agc_idx);

	return 0;
}

static int imx290_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;
	u32 again_index;

	/* long frame */
	again_index = p_again_gp->l;
	imx290_set_agc_index(vdev, again_index);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int imx290_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx290_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx290_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx290_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;
	u32 fsc, rhs1, rhs2;
	int shutter_long, shutter_short1, shutter_short2;
	int errCode = 0;

	rhs1 = pinfo->rhs1;
	rhs2 = pinfo->rhs2;
	fsc = pinfo->fsc;

	/* long shutter */
	shutter_long = p_shutter_gp->l;
	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->s1;
	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->s2;

	/* shutter limitation check */
	if(vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if(fsc < shutter_long) {
			vin_error("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long - 1;
		}
		if(rhs1 < shutter_short1) {
			vin_error("short shutter row(%d) must be less than rhs(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1 - 1;
		}
		vin_debug("fsc:%d, shs1:%d, shs2:%d, rhs1:%d\n",
			fsc, shutter_short1, shutter_long, rhs1);

		/* short shutter check */
		if((shutter_short1 >= 2) && (shutter_short1 <= rhs1 - 2)){
			imx290_write_reg(vdev, IMX290_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx290_write_reg(vdev, IMX290_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx290_write_reg(vdev, IMX290_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_error("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs1 + 2) && (shutter_long <= fsc - 2)) {
			imx290_write_reg(vdev, IMX290_SHS2_LSB, (u8)(shutter_long & 0xFF));
			imx290_write_reg(vdev, IMX290_SHS2_MSB, (u8)(shutter_long >> 8));
			imx290_write_reg(vdev, IMX290_SHS2_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_error("shs2 exceeds limitation! shs2:%d, shs1:%d, rhs1:%d, fsc:%d\n",
					shutter_long, shutter_short1, rhs1, fsc);
			return -EPERM;
		}

	} else if(vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if(fsc < shutter_long) {
			vin_error("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long - 1;
		}
		if(rhs1 < shutter_short1) {
			vin_error("short shutter 1 row(%d) must be less than rhs1(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1 - 1;
		}
		if(rhs2 < shutter_short2) {
			vin_error("short shutter 2 row(%d) must be less than rhs2(%d)!\n", shutter_short2, rhs2);
			return -EPERM;
		} else {
			shutter_short2 = rhs2 - shutter_short2;
		}
		vin_debug("fsc:%d, shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
			fsc, shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);

		/* short shutter 1 check */
		if((shutter_short1 >= 3) && (shutter_short1 <= rhs1 - 2)) {
			imx290_write_reg(vdev, IMX290_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx290_write_reg(vdev, IMX290_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx290_write_reg(vdev, IMX290_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_error("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* short shutter 2 check */
		if((shutter_short2 >= rhs1 + 3) && (shutter_short2 <= rhs2 - 2)) {
			imx290_write_reg(vdev, IMX290_SHS2_LSB, (u8)(shutter_short2 & 0xFF));
			imx290_write_reg(vdev, IMX290_SHS2_MSB, (u8)(shutter_short2 >> 8));
			imx290_write_reg(vdev, IMX290_SHS2_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		} else {
			vin_error("shs2 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs2 + 3) && (shutter_long <= fsc - 2)) {
			imx290_write_reg(vdev, IMX290_SHS3_LSB, (u8)(shutter_long & 0xFF));
			imx290_write_reg(vdev, IMX290_SHS3_MSB, (u8)(shutter_long >> 8));
			imx290_write_reg(vdev, IMX290_SHS3_HSB, (u8)((shutter_long >> 16) & 0xF));
		} else {
			vin_error("shs3 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs2:%d, fsc:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs2, fsc);
			return -EPERM;
		}
	} else {
		vin_error("Non WDR mode can't support this API: %s!\n", __func__);
	}

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	return errCode;
}

static int imx290_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int imx290_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u64 exposure_time_q9;
	int errCode = 0;
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	/* long shutter */
	exposure_time_q9 = p_shutter2row->l;
	exposure_time_q9 = exposure_time_q9 * (u64)vdev->cur_pll->pixelclk;
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, pinfo->line_length);
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, 512000000);
	p_shutter2row->l = (u32)exposure_time_q9;

	/* short shutter 1 */
	exposure_time_q9 = p_shutter2row->s1;
	exposure_time_q9 = exposure_time_q9 * (u64)vdev->cur_pll->pixelclk;
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, pinfo->line_length);
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, 512000000);
	p_shutter2row->s1 = (u32)exposure_time_q9;

	/* short shutter 2 */
	exposure_time_q9 = p_shutter2row->s2;
	exposure_time_q9 = exposure_time_q9 * (u64)vdev->cur_pll->pixelclk;
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, pinfo->line_length);
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, 512000000);
	p_shutter2row->s2 = (u32)exposure_time_q9;

	return errCode;
}

static int imx290_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX290_H_MIRROR | IMX290_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX290_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX290_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx290_read_reg(vdev, IMX290_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX290_H_MIRROR | IMX290_V_FLIP);
	tmp_reg |= readmode;
	imx290_write_reg(vdev, IMX290_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int imx290_get_aaa_info(struct vin_device *vdev,
		struct vindev_aaa_info *aaa_info)
{
	struct imx290_priv *pinfo = (struct imx290_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->fsc - 4;
	aaa_info->sht1_max = pinfo->rhs1 - 2;
	aaa_info->sht2_max = (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) ?
		(pinfo->rhs2 - pinfo->rhs1 - 4) : 0;

	return 0;
}

#ifdef CONFIG_PM
static int imx290_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx290_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int imx290_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx290_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops imx290_ops = {
	.init_device		= imx290_init_device,
	.set_format		= imx290_set_format,
	.set_pll			= imx290_set_pll,
	.set_shutter_row 	= imx290_set_shutter_row,
	.shutter2row 		= imx290_shutter2row,
	.set_frame_rate	= imx290_set_fps,
	.set_agc_index		= imx290_set_agc_index,
	.set_hold_mode		= imx290_set_hold_mode,
	.set_mirror_mode	= imx290_set_mirror_mode,
	.read_reg			= imx290_read_reg,
	.write_reg		= imx290_write_reg,
	.get_aaa_info		= imx290_get_aaa_info,
#ifdef CONFIG_PM
	.suspend 			= imx290_suspend,
	.resume 			= imx290_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp	= imx290_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp	= imx290_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp	= imx290_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp	= imx290_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp	= imx290_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp	= imx290_get_wdr_shutter_row_group,
	.wdr_shutter2row		= imx290_wdr_shutter2row,
};

/* ========================================================================== */
static int imx290_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx290_priv *imx290;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_IMX290, sizeof(struct imx290_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x48000000;	/* 72dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x004CCCCC;	/* 0.3dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = IMX290_GAIN_MAX_DB;

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	imx290 = (struct imx290_priv *)vdev->priv;
	imx290->control_data = client;

	rval = ambarella_vin_register_device(vdev, &imx290_ops,
		imx290_formats, ARRAY_SIZE(imx290_formats),
		imx290_plls, ARRAY_SIZE(imx290_plls));

	if (rval < 0)
		goto imx290_probe_err;

	vin_info("IMX290 init(8-lane lvds)\n");

	return 0;

imx290_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx290_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx290_idtable[] = {
	{ "imx290", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx290_idtable);

static struct i2c_driver i2c_driver_imx290 = {
	.driver = {
		.name	= "imx290",
	},

	.id_table	= imx290_idtable,
	.probe		= imx290_probe,
	.remove		= imx290_remove,

};

static int __init imx290_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx290", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx290);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit imx290_exit(void)
{
	i2c_del_driver(&i2c_driver_imx290);
}

module_init(imx290_init);
module_exit(imx290_exit);

MODULE_DESCRIPTION("IMX290 1/2.8 -Inch, 1945x1097, 2.13-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

