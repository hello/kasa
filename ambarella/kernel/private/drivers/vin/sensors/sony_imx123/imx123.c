/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx123/imx123.c
 *
 * History:
 *    2014/08/05 - [Long Zhao] Create
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
#include "imx123.h"

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static bool dual_gain = 0;
module_param(dual_gain, bool, 0644);
MODULE_PARM_DESC(dual_gain, " Use dual gain mode 0:normal mode 1:dual gain mode");

struct imx123_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
	u32 fsc, rhs1, rhs2;
};

#include "imx123_table.c"

static int imx123_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct imx123_priv *imx123;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	imx123 = (struct imx123_priv *)vdev->priv;
	client = imx123->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX123_SWRESET))
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

static int imx123_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct imx123_priv *imx123;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	imx123 = (struct imx123_priv *)vdev->priv;
	client = imx123->control_data;

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

static int imx123_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	imx123_write_reg(vdev, IMX123_REGHOLD, hold_mode);
	return 0;
}

static int imx123_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx123_config;

	memset(&imx123_config, 0, sizeof (imx123_config));

	imx123_config.interface_type = SENSOR_SERIAL_LVDS;
	imx123_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE || dual_gain) {
		imx123_config.slvds_cfg.lane_number = SENSOR_4_LANE;
		imx123_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;
	} else {
		imx123_config.slvds_cfg.lane_number = SENSOR_8_LANE;
		imx123_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY_DOL;
		/* use split width to make VIN divide the long expo lines */
		imx123_config.hdr_cfg.split_width = IMX123_H_PERIOD;
	}

	imx123_config.cap_win.x = format->def_start_x;
	imx123_config.cap_win.y = format->def_start_y;
	imx123_config.cap_win.width = format->def_width;
	imx123_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	imx123_config.hdr_cfg.act_win.x = format->act_start_x;
	imx123_config.hdr_cfg.act_win.y = format->act_start_y;
	imx123_config.hdr_cfg.act_win.width = format->act_width;
	imx123_config.hdr_cfg.act_win.height = format->act_height;
	imx123_config.hdr_cfg.act_win.max_width = format->max_act_width;
	imx123_config.hdr_cfg.act_win.max_height = format->max_act_height;

	imx123_config.sensor_id	= GENERIC_SENSOR;
	imx123_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx123_config.bayer_pattern = format->bayer_pattern;
	imx123_config.video_format = format->format;
	imx123_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx123_config);
}

static void imx123_sw_reset(struct vin_device *vdev)
{
	imx123_write_reg(vdev, IMX123_STANDBY, 0x1);//STANDBY
	imx123_write_reg(vdev, IMX123_SWRESET, 0x1);
	msleep(10);
}

static void imx123_start_streaming(struct vin_device *vdev)
{
	u32 val;
	imx123_write_reg(vdev, IMX123_XMSTA, 0x0); //master mode start
	imx123_write_reg(vdev, IMX123_STANDBY, 0x0); //cancel standby
	msleep(20);

	imx123_read_reg(vdev, IMX123_DCKRST, &val);
	val |= IMX123_DCKRST_BIT;
	imx123_write_reg(vdev, IMX123_DCKRST, val); /* DCKRST */
	val &= ~IMX123_DCKRST_BIT;
	imx123_write_reg(vdev, IMX123_DCKRST, val); /* DCKRST */
}

static int imx123_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	imx123_sw_reset(vdev);

	regs = imx123_share_regs;
	regs_num = ARRAY_SIZE(imx123_share_regs);

	for (i = 0; i < regs_num; i++)
		imx123_write_reg(vdev, regs[i].addr, regs[i].data);

	if (dual_gain) {
		regs = imx123_dual_gain_share_regs;
		regs_num = ARRAY_SIZE(imx123_dual_gain_share_regs);

		for (i = 0; i < regs_num; i++)
			imx123_write_reg(vdev, regs[i].addr, regs[i].data);
	}

	return 0;
}

static int imx123_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx123_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx123_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx123_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx123_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	imx123_read_reg(vdev, IMX123_HMAX_MSB, &val_high);
	imx123_read_reg(vdev, IMX123_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx123_read_reg(vdev, IMX123_VMAX_HSB, &val_high);
	imx123_read_reg(vdev, IMX123_VMAX_MSB, &val_mid);
	imx123_read_reg(vdev, IMX123_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx123_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx123_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE || dual_gain) {
		regs = imx123_linear_mode_regs[format->device_mode];
		regs_num = ARRAY_SIZE(imx123_linear_mode_regs[format->device_mode]);

		for (i = 0; i < regs_num; i++) {
			/* dual gain mode is based on linear mode */
			if (unlikely (dual_gain && (regs[i].addr == 0x3007)))
				regs[i].data |= 0x0C;
			imx123_write_reg(vdev, regs[i].addr, regs[i].data);
		}

		if (dual_gain)
			vin_info("Dual gain hdr mode\n");

	} else if (format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if (format->device_mode == 2) { /* 3M 2x WDR */
			regs = imx123_3m_2x_regs;
			regs_num = ARRAY_SIZE(imx123_3m_2x_regs);

			for (i = 0; i < regs_num; i++)
				imx123_write_reg(vdev, regs[i].addr, regs[i].data);

			vin_info("3M 2X hdr mode\n");
		}
	} else if (format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if (format->device_mode == 3) { /* 3M 3x WDR */
			regs = imx123_3m_3x_regs;
			regs_num = ARRAY_SIZE(imx123_3m_3x_regs);

			for (i = 0; i < regs_num; i++)
				imx123_write_reg(vdev, regs[i].addr, regs[i].data);

			vin_info("3M 3X hdr mode\n");
		}
	}

	/* for DOL mode, set RHS registers */
	if (format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE && !dual_gain) {
		imx123_write_reg(vdev, IMX123_RHS1_HSB, IMX123_QXGA_2X_RHS1 >> 16);
		imx123_write_reg(vdev, IMX123_RHS1_MSB, IMX123_QXGA_2X_RHS1 >> 8);
		imx123_write_reg(vdev, IMX123_RHS1_LSB, IMX123_QXGA_2X_RHS1 & 0xff);

		pinfo->rhs1 = IMX123_QXGA_2X_RHS1;
	} else if (format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE){
		imx123_write_reg(vdev, IMX123_RHS1_HSB, IMX123_QXGA_3X_RHS1 >> 16);
		imx123_write_reg(vdev, IMX123_RHS1_MSB, IMX123_QXGA_3X_RHS1 >> 8);
		imx123_write_reg(vdev, IMX123_RHS1_LSB, IMX123_QXGA_3X_RHS1 & 0xff);

		imx123_write_reg(vdev, IMX123_RHS2_HSB, IMX123_QXGA_3X_RHS2 >> 16);
		imx123_write_reg(vdev, IMX123_RHS2_MSB, IMX123_QXGA_3X_RHS2 >> 8);
		imx123_write_reg(vdev, IMX123_RHS2_LSB, IMX123_QXGA_3X_RHS2 & 0xff);

		pinfo->rhs1 = IMX123_QXGA_3X_RHS1;
		pinfo->rhs2 = IMX123_QXGA_3X_RHS2;
	}

	rval = imx123_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx123_get_line_time(vdev);

	/* TG reset release ( Enable Streaming )*/
	imx123_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx123_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx123_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 3 ~ (Frame format(V) - 4) */
	min_line = 3;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line;
	imx123_write_reg(vdev, IMX123_SHS1_HSB, blank_lines >> 16);
	imx123_write_reg(vdev, IMX123_SHS1_MSB, blank_lines >> 8);
	imx123_write_reg(vdev, IMX123_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx123_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx123_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx123_set_fps( struct vin_device *vdev, int fps)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;
	u64 v_lines, vb_time;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	pinfo->fsc = v_lines;

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE || dual_gain) {
		v_lines = pinfo->fsc;/* FSC = VMAX */
	} else if(vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		v_lines = pinfo->fsc >> 1; /* FSC = VMAX * 2 */
	} else if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		v_lines = pinfo->fsc >> 2; /* FSC = VMAX * 4 */
	}

	imx123_write_reg(vdev, IMX123_VMAX_HSB, (v_lines & 0x010000) >> 16);
	imx123_write_reg(vdev, IMX123_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx123_write_reg(vdev, IMX123_VMAX_LSB, v_lines & 0x0000FF);

	imx123_write_reg(vdev, 0x3105, ((v_lines - 1) & 0x010000) >> 16);
	imx123_write_reg(vdev, 0x3104, ((v_lines - 1) & 0x00FF00) >> 8);
	imx123_write_reg(vdev, 0x3103, (v_lines - 1) & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(pinfo->fsc - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx123_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX123_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX123_GAIN_MAX_DB);
		agc_idx = IMX123_GAIN_MAX_DB;
	}

	imx123_write_reg(vdev, IMX123_GAIN_LSB, (u8)(agc_idx&0xFF));
	imx123_write_reg(vdev, IMX123_GAIN_MSB, (u8)(agc_idx>>8)&0x3);

	return 0;
}

static int imx123_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX123_H_MIRROR | IMX123_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE && !dual_gain) {
			imx123_write_reg(vdev, IMX123_WINMODE, 0x40);
			imx123_write_reg(vdev, IMX123_WINWV_OB, 0x0b);
			imx123_write_reg(vdev, IMX123_WINPV_LSB, 0x01);
		}
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX123_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE && !dual_gain) {
			imx123_write_reg(vdev, IMX123_WINMODE, 0x00);
			imx123_write_reg(vdev, IMX123_WINWV_OB, 0x0c);
			imx123_write_reg(vdev, IMX123_WINPV_LSB, 0x00);
		}
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX123_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE && !dual_gain) {
			imx123_write_reg(vdev, IMX123_WINMODE, 0x40);
			imx123_write_reg(vdev, IMX123_WINWV_OB, 0x0b);
			imx123_write_reg(vdev, IMX123_WINPV_LSB, 0x01);
		}
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE && !dual_gain) {
			imx123_write_reg(vdev, IMX123_WINMODE, 0x00);
			imx123_write_reg(vdev, IMX123_WINWV_OB, 0x0c);
			imx123_write_reg(vdev, IMX123_WINPV_LSB, 0x00);
		}
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx123_read_reg(vdev, IMX123_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX123_H_MIRROR | IMX123_V_FLIP);
	tmp_reg |= readmode;
	imx123_write_reg(vdev, IMX123_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int imx123_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;
	u32 again_index;

	/* long frame */
	again_index = p_again_gp->l;
	imx123_write_reg(vdev, IMX123_GAIN_LSB, (u8)(again_index&0xFF));
	imx123_write_reg(vdev, IMX123_GAIN_MSB, (u8)(again_index>>8)&0x3);

	if (dual_gain) {
		/* short frame 1 */
		again_index = p_again_gp->s1;
		imx123_write_reg(vdev, IMX123_GAIN2_LSB, (u8)(again_index&0xFF));
		imx123_write_reg(vdev, IMX123_GAIN2_MSB, (u8)(again_index>>8)&0x3);
	}

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int imx123_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx123_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx123_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx123_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;
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
		}

		if (dual_gain) {
			imx123_set_shutter_row(vdev, shutter_long);
		} else {
			shutter_long = fsc - shutter_long;

			if(rhs1 < shutter_short1) {
				vin_error("short shutter row(%d) must be less than rhs(%d)!\n", shutter_short1, rhs1);
				return -EPERM;
			} else {
				shutter_short1 = rhs1 - shutter_short1;
			}
			vin_debug("fsc:%d, shs1:%d, shs2:%d, rhs1:%d\n",
				fsc, shutter_short1, shutter_long, rhs1);

			/* short shutter check */
			if((shutter_short1 >= 7) && (shutter_short1 != 9) && (shutter_short1 != 10) &&
				(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 3) && (shutter_short1 != rhs1 - 4)) {
				imx123_write_reg(vdev, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
				imx123_write_reg(vdev, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
				imx123_write_reg(vdev, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
			} else {
				vin_error("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
						shutter_short1, rhs1);
				return -EPERM;
			}
			/* long shutter check */
			if((shutter_long >= rhs1 + 7) && (shutter_long != rhs1 + 9) && (shutter_long != rhs1 + 10) &&
				(shutter_long < fsc)  && (shutter_long != fsc - 3) && (shutter_long != fsc - 4) &&
				(shutter_long != shutter_short1 + 9) && (shutter_long != fsc + shutter_short1 - 9)) {
				imx123_write_reg(vdev, IMX123_SHS2_LSB, (u8)(shutter_long & 0xFF));
				imx123_write_reg(vdev, IMX123_SHS2_MSB, (u8)(shutter_long >> 8));
				imx123_write_reg(vdev, IMX123_SHS2_HSB, (u8)((shutter_long >> 16) & 0xF));
			} else {
				vin_error("shs2 exceeds limitation! shs2:%d, shs1:%d, rhs1:%d, fsc:%d\n",
						shutter_long, shutter_short1, rhs1, fsc);
				return -EPERM;
			}
		}

	} else if(vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		if(fsc < shutter_long) {
			vin_error("long shutter row(%d) must be less than fsc(%d)!\n", shutter_long, fsc);
			return -EPERM;
		} else {
			shutter_long = fsc - shutter_long;
		}
		if(rhs1 < shutter_short1) {
			vin_error("short shutter 1 row(%d) must be less than rhs1(%d)!\n", shutter_short1, rhs1);
			return -EPERM;
		} else {
			shutter_short1 = rhs1 - shutter_short1;
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
		if((shutter_short1 >= 9) && (shutter_short1 != 13) && (shutter_short1 != 14) &&
			(shutter_short1 < rhs1)  && (shutter_short1 != rhs1 - 5) && (shutter_short1 != rhs1 - 6)) {
			imx123_write_reg(vdev, IMX123_SHS1_LSB, (u8)(shutter_short1 & 0xFF));
			imx123_write_reg(vdev, IMX123_SHS1_MSB, (u8)(shutter_short1 >> 8));
			imx123_write_reg(vdev, IMX123_SHS1_HSB, (u8)((shutter_short1 >> 16) & 0xF));
		} else {
			vin_error("shs1 exceeds limitation! shs1:%d, rhs1:%d\n",
					shutter_short1, rhs1);
			return -EPERM;
		}
		/* short shutter 2 check */
		if((shutter_short2 >= rhs1 + 9) && (shutter_short2 != rhs1 + 13) && (shutter_short2 != rhs1 + 14) &&
			(shutter_short2 < rhs2) && (shutter_short2 != rhs2 - 5) && (shutter_short2 != rhs2 - 6) &&
			(shutter_short2 != shutter_short1 + 13) && (shutter_short2 != shutter_long - 13)) {
			imx123_write_reg(vdev, IMX123_SHS2_LSB, (u8)(shutter_short2 & 0xFF));
			imx123_write_reg(vdev, IMX123_SHS2_MSB, (u8)(shutter_short2 >> 8));
			imx123_write_reg(vdev, IMX123_SHS2_HSB, (u8)((shutter_short2 >> 16) & 0xF));
		} else {
			vin_error("shs2 exceeds limitation! shs1:%d, shs2:%d, shs3:%d, rhs1:%d, rhs2:%d\n",
					shutter_short1, shutter_short2, shutter_long, rhs1, rhs2);
			return -EPERM;
		}
		/* long shutter check */
		if((shutter_long >= rhs2 + 9) && (shutter_long != rhs2 + 13) && (shutter_long != rhs2 + 14) &&
			(shutter_long < fsc) && (shutter_long != fsc - 5) && (shutter_long != fsc - 6) &&
			(shutter_long != shutter_short2 + 13) && (shutter_long != fsc + shutter_short1 - 13)) {
			imx123_write_reg(vdev, IMX123_SHS3_LSB, (u8)(shutter_long & 0xFF));
			imx123_write_reg(vdev, IMX123_SHS3_MSB, (u8)(shutter_long >> 8));
			imx123_write_reg(vdev, IMX123_SHS3_HSB, (u8)((shutter_long >> 16) & 0xF));
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

static int imx123_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int imx123_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u64 exposure_time_q9;
	int errCode = 0;
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

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

static int imx123_get_eis_info(struct vin_device *vdev,
		struct vindev_eisinfo *eis_info)
{
	eis_info->sensor_cell_width = 250;/* 2.5 um */
	eis_info->sensor_cell_height = 250;/* 2.5 um */
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;
	eis_info->vb_time = vdev->cur_format->vb_time;

	return 0;
}

static int imx123_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct imx123_priv *pinfo = (struct imx123_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 14;
	aaa_info->sht1_max = pinfo->rhs1 - 7;
	aaa_info->sht2_max = (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) ?
		(pinfo->rhs2 - pinfo->rhs1 - 9) : 0;

	return 0;
}

static struct vin_ops imx123_ops = {
	.init_device		= imx123_init_device,
	.set_format		= imx123_set_format,
	.set_pll			= imx123_set_pll,
	.set_shutter_row	= imx123_set_shutter_row,
	.shutter2row 		= imx123_shutter2row,
	.set_frame_rate	= imx123_set_fps,
	.set_agc_index		= imx123_set_agc_index,
	.set_mirror_mode	= imx123_set_mirror_mode,
	.read_reg			= imx123_read_reg,
	.write_reg		= imx123_write_reg,
	.set_hold_mode		= imx123_set_hold_mode,
	.get_eis_info		= imx123_get_eis_info,
	.get_aaa_info		= imx123_get_aaa_info,

	/* for wdr sensor */
	.set_wdr_again_idx_gp = imx123_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = imx123_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = imx123_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = imx123_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = imx123_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = imx123_get_wdr_shutter_row_group,
	.wdr_shutter2row = imx123_wdr_shutter2row,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int imx123_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx123_priv *imx123;

	if (dual_gain)
		vdev = ambarella_vin_create_device(client->name,
			SENSOR_IMX123_DCG, sizeof(struct imx123_priv));
	else
		vdev = ambarella_vin_create_device(client->name,
			SENSOR_IMX123, sizeof(struct imx123_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_QXGA;
	if (dual_gain)
		vdev->default_hdr_mode = AMBA_VIDEO_2X_HDR_MODE;

	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	// 48dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00199999;	// 0.1dB

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	if (dual_gain) {
		vdev->wdr_again_idx_min = 0;
		vdev->wdr_again_idx_max = IMX123_DUAL_GAIN_MAX_DB;
	} else {
		vdev->wdr_again_idx_min = 0;
		vdev->wdr_again_idx_max = IMX123_GAIN_MAX_DB;
	}

	i2c_set_clientdata(client, vdev);

	imx123 = (struct imx123_priv *)vdev->priv;
	imx123->control_data = client;

	if (dual_gain) {
		rval = ambarella_vin_register_device(vdev, &imx123_ops,
			imx123_dual_gain_formats, ARRAY_SIZE(imx123_dual_gain_formats),
			imx123_plls, ARRAY_SIZE(imx123_plls));
	} else {
		rval = ambarella_vin_register_device(vdev, &imx123_ops,
			imx123_formats, ARRAY_SIZE(imx123_formats),
			imx123_plls, ARRAY_SIZE(imx123_plls));
	}
	if (rval < 0)
		goto imx123_probe_err;

	if (dual_gain)
		vin_info("IMX123(dual gain) init(4-lane lvds)\n");
	else
		vin_info("IMX123 init(4/8-lane lvds)\n");

	return 0;

imx123_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx123_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx123_idtable[] = {
	{ "imx123", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx123_idtable);

static struct i2c_driver i2c_driver_imx123 = {
	.driver = {
		.name	= "imx123",
	},

	.id_table	= imx123_idtable,
	.probe		= imx123_probe,
	.remove		= imx123_remove,

};

static int __init imx123_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx123", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx123);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit imx123_exit(void)
{
	i2c_del_driver(&i2c_driver_imx123);
}

module_init(imx123_init);
module_exit(imx123_exit);

MODULE_DESCRIPTION("IMX123 1/2.8 -Inch, 2065x1565, 3.20-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

