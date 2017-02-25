/*
 * Filename : mn34210pl.c
 *
 * History:
 *    2015/02/05 - [Hao Zeng] Create
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
#include <plat/spi.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "mn34210pl.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int hdr_mode = 0;
module_param(hdr_mode, int, 0644);
MODULE_PARM_DESC(hdr_mode, "Set HDR mode 0:linear 1:2x HDR 2:3x HDR 3:4x HDR");

#define GAIN_160_STEPS (1)

struct mn34210pl_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
};

#include "mn34210pl_table.c"

static int mn34210pl_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct mn34210pl_priv *mn34210pl;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	mn34210pl = (struct mn34210pl_priv *)vdev->priv;
	client = mn34210pl->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}

	return 0;
}

static int mn34210pl_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct mn34210pl_priv *mn34210pl;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	mn34210pl = (struct mn34210pl_priv *)vdev->priv;
	client = mn34210pl->control_data;

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

static int mn34210pl_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config mn34210pl_config;

	memset(&mn34210pl_config, 0, sizeof (mn34210pl_config));

	mn34210pl_config.interface_type = SENSOR_SERIAL_LVDS;
	mn34210pl_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	mn34210pl_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	if(format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		mn34210pl_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;
	} else {
		mn34210pl_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_PANASONIC;
	}

	mn34210pl_config.cap_win.x = format->def_start_x;
	mn34210pl_config.cap_win.y = format->def_start_y;
	mn34210pl_config.cap_win.width = format->def_width;
	mn34210pl_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	mn34210pl_config.hdr_cfg.act_win.x = format->act_start_x;
	mn34210pl_config.hdr_cfg.act_win.y = format->act_start_y;
	mn34210pl_config.hdr_cfg.act_win.width = format->act_width;
	mn34210pl_config.hdr_cfg.act_win.height = format->act_height;
	mn34210pl_config.hdr_cfg.act_win.max_width = format->max_act_width;
	mn34210pl_config.hdr_cfg.act_win.max_height = format->max_act_height;

	mn34210pl_config.sensor_id	= GENERIC_SENSOR;
	mn34210pl_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	mn34210pl_config.bayer_pattern	= format->bayer_pattern;
	mn34210pl_config.video_format	= format->format;
	mn34210pl_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &mn34210pl_config);
}

static void mn34210pl_sw_reset(struct vin_device *vdev)
{
	mn34210pl_write_reg(vdev, 0x3001, 0x0000);
}

static int mn34210pl_init_device(struct vin_device *vdev)
{
	mn34210pl_sw_reset(vdev);

	return 0;
}

static int mn34210pl_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	mn34210pl_read_reg(vdev, 0x0343, &val_low);	/* HCYCLE_LSB */
	mn34210pl_read_reg(vdev, 0x0342, &val_high);	/* HCYCLE_MSB */
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	mn34210pl_read_reg(vdev, 0x0341, &val_low);/* VCYCLE_LSB */
	mn34210pl_read_reg(vdev, 0x0340, &val_high);/* VCYCLE_MSB */
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int mn34210pl_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int mn34210pl_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs = NULL;
	int i, regs_num = 0;
	int rval;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		if (format->device_mode == 0) {
			regs = mn34210pl_linear_mode_regs;
			regs_num = ARRAY_SIZE(mn34210pl_linear_mode_regs);
		} else if (format->device_mode == 1) {
			regs = mn34210pl_720p_mode_regs;
			regs_num = ARRAY_SIZE(mn34210pl_720p_mode_regs);
		}
		vin_info("Linear mode\n");
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		regs = mn34210pl_2x_wdr_mode_regs;
		regs_num = ARRAY_SIZE(mn34210pl_2x_wdr_mode_regs);
		vin_info("2x hdr mode\n");
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		regs = mn34210pl_3x_wdr_mode_regs;
		regs_num = ARRAY_SIZE(mn34210pl_3x_wdr_mode_regs);
		vin_info("3x hdr mode\n");
		break;
	case AMBA_VIDEO_4X_HDR_MODE:
		regs = mn34210pl_4x_wdr_mode_regs;
		regs_num = ARRAY_SIZE(mn34210pl_4x_wdr_mode_regs);
		vin_info("4x hdr mode\n");
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_info("Unknown mode\n");
		break;
	}

	for (i = 0; i < regs_num; i++)
		mn34210pl_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = mn34210pl_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	mn34210pl_get_line_time(vdev);

	return mn34210pl_set_vin_mode(vdev, format);
}

static int mn34210pl_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 2 ~ (Frame format(V) - 4) */
	min_line = 2;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	mn34210pl_write_reg(vdev, 0x0202, (num_line >> 8) & 0xFF);
	mn34210pl_write_reg(vdev, 0x0203, num_line & 0xFF);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int mn34210pl_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	if(unlikely(!pinfo->line_length)) {
		rval = mn34210pl_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int mn34210pl_set_fps(struct vin_device *vdev, int fps)
{
	u64	v_lines, vb_time;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	mn34210pl_write_reg(vdev, 0x0340, (u8)(v_lines >> 8));
	mn34210pl_write_reg(vdev, 0x0341, (u8)v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int mn34210pl_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE) {
		vin_debug("hdr mode doesn't use this API %s!\n", __func__);
		return 0;
	}

	if (agc_idx > MN34210PL_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, MN34210PL_GAIN_0DB);
		agc_idx = MN34210PL_GAIN_0DB;
	}

	agc_idx = MN34210PL_GAIN_0DB - agc_idx;

	if (vdev->cur_format->bits == AMBA_VIDEO_BITS_10) {
		/*  for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
		if(MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
			mn34210pl_write_reg(vdev, 0x3097,  0x10);
			mn34210pl_write_reg(vdev, 0x3098,  0x00);
		} else {
			mn34210pl_write_reg(vdev, 0x3097,  0xD1);
			mn34210pl_write_reg(vdev, 0x3098,  0x80);
		}
	} else {
			mn34210pl_write_reg(vdev, 0x3097,  0x00);
			mn34210pl_write_reg(vdev, 0x3098,  0x00);
	}

	/* AGAIN */
	mn34210pl_write_reg(vdev, 0x0204, (MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0205, (u8)MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_AGAIN]);
	/* DGAIN-Gr */
	mn34210pl_write_reg(vdev, 0x020E, (MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x020F, (u8)MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-R */
	mn34210pl_write_reg(vdev, 0x0210, (MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0211, (u8)MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-B */
	mn34210pl_write_reg(vdev, 0x0212, (MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0213, (u8)MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-Gb */
	mn34210pl_write_reg(vdev, 0x0214, (MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0215, (u8)MN34210PL_GAIN_TABLE[agc_idx][MN34210PL_GAIN_COL_DGAIN]);

	return 0;
}

static int mn34210pl_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = MN34210PL_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
	case VINDEV_MIRROR_HORRIZONTALLY:
	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	mn34210pl_read_reg(vdev, 0x0101, &tmp_reg);
	tmp_reg &= (~MN34210PL_V_FLIP);
	tmp_reg |= readmode;
	mn34210pl_write_reg(vdev, 0x0101, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int mn34210pl_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;
	int shutter_long, shutter_short1, shutter_short2, shutter_short3;
	u32 active_lines;
	u64 vb_time;
	int errCode = 0;

	active_lines = vdev->cur_format->height;

	/* long shutter */
	shutter_long = p_shutter_gp->l;

	/* short shutter 1 */
	shutter_short1 = p_shutter_gp->s1;

	/* short shutter 2 */
	shutter_short2 = p_shutter_gp->s2;

	/* short shutter 3 */
	shutter_short3 = p_shutter_gp->s3;

	/* shutter limitation check */
	switch(vdev->cur_format->hdr_mode){
		case AMBA_VIDEO_2X_HDR_MODE:
			if(shutter_long + shutter_short1 + 5 > pinfo->frame_length_lines){
				vin_error("shutter exceeds limitation! long:%d, short:%d, V:%d\n",
					shutter_long, shutter_short1, pinfo->frame_length_lines);
				return -EPERM;
			}else if(shutter_short1 + 2 > pinfo->frame_length_lines - active_lines) {
				vin_error("short frame offset exceeds limitation! short:%d, VB:%d\n",
					shutter_short1, pinfo->frame_length_lines - active_lines);
			}
			break;
		case AMBA_VIDEO_3X_HDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 7 > pinfo->frame_length_lines){
				vin_error("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, pinfo->frame_length_lines);
				return -EPERM;
			}else if(shutter_short1 + shutter_short2 + 4 > pinfo->frame_length_lines - active_lines) {
				vin_error("short frame offset exceeds limitation! short1:%d, short2:%d, VB:%d\n",
					shutter_short1, shutter_short2, pinfo->frame_length_lines - active_lines);
			}
			break;
		case AMBA_VIDEO_4X_HDR_MODE:
			if(shutter_long + shutter_short1 + shutter_short2 + 9 > pinfo->frame_length_lines){
				vin_error("shutter exceeds limitation! long:%d, short1:%d, short2:%d, short3:%d, V:%d\n",
					shutter_long, shutter_short1, shutter_short2, shutter_short3, pinfo->frame_length_lines);
				return -EPERM;
			}else if(shutter_short1 + shutter_short2 + shutter_short3 + 6 > pinfo->frame_length_lines - active_lines) {
				vin_error("short frame offset exceeds limitation! short1:%d, short2:%d, short3:%d, VB:%d\n",
					shutter_short1, shutter_short2, shutter_short3, pinfo->frame_length_lines - active_lines);
			}
			break;
		case AMBA_VIDEO_LINEAR_MODE:
		default:
			vin_error("Unsupported mode\n");
			return -EPERM;
	}

	/* long shutter */
	mn34210pl_write_reg(vdev, 0x0203, (u8)(shutter_long & 0xFF));
	mn34210pl_write_reg(vdev, 0x0202, (u8)(shutter_long >> 8));

	/* short shutter 1 */
	mn34210pl_write_reg(vdev, 0x3127, (u8)(shutter_short1 & 0xFF));
	mn34210pl_write_reg(vdev, 0x3126, (u8)(shutter_short1 >> 8));

	/* short shutter 2 */
	mn34210pl_write_reg(vdev, 0x312B, (u8)(shutter_short2 & 0xFF));
	mn34210pl_write_reg(vdev, 0x312A, (u8)(shutter_short2 >> 8));

	/* short shutter 3 */
	mn34210pl_write_reg(vdev, 0x312F, (u8)(shutter_short3 & 0xFF));
	mn34210pl_write_reg(vdev, 0x312E, (u8)(shutter_short3 >> 8));

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	switch(vdev->cur_format->hdr_mode) {
	case AMBA_VIDEO_2X_HDR_MODE:
		vb_time = pinfo->frame_length_lines - vdev->cur_format->height - pinfo->wdr_shutter_gp.s1;
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		vb_time = pinfo->frame_length_lines - vdev->cur_format->height - pinfo->wdr_shutter_gp.s1
			- pinfo->wdr_shutter_gp.s2;
		break;
	case AMBA_VIDEO_4X_HDR_MODE:
		vb_time = pinfo->frame_length_lines - vdev->cur_format->height - pinfo->wdr_shutter_gp.s1
			- pinfo->wdr_shutter_gp.s2 - pinfo->wdr_shutter_gp.s3;
		break;
	default:
		vin_error("Unsupported mode\n");
		return -EPERM;
	}

	vb_time = pinfo->line_length * (u64)vb_time * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	vin_debug("shutter long:%d, short1:%d, short2:%d, short3:%d, vb_time:%d\n", p_shutter_gp->l,
		p_shutter_gp->s1, p_shutter_gp->s2, p_shutter_gp->s3, vdev->cur_format->vb_time);

	return errCode;
}

static int mn34210pl_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int mn34210pl_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;
	u32 again_index;

	again_index = MN34210PL_GAIN_0DB - p_again_gp->l;

	if (vdev->cur_format->bits == AMBA_VIDEO_BITS_10) {
		/*  for 10bits, if gain >=16.5dB, set 0x3097 and 0x3098 according to Pana FAE's suggestion */
		if(MN34210PL_GAIN_TABLE[again_index][MN34210PL_GAIN_COL_AGAIN] >= 0x1B0) {
			mn34210pl_write_reg(vdev, 0x3097,  0x10);
			mn34210pl_write_reg(vdev, 0x3098,  0x00);
		} else {
			mn34210pl_write_reg(vdev, 0x3097,  0xD1);
			mn34210pl_write_reg(vdev, 0x3098,  0x80);
		}
	} else {
			mn34210pl_write_reg(vdev, 0x3097,  0x00);
			mn34210pl_write_reg(vdev, 0x3098,  0x00);
	}
	/* AGAIN */
	mn34210pl_write_reg(vdev, 0x0204, (MN34210PL_GAIN_TABLE[again_index][MN34210PL_GAIN_COL_AGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0205, (u8)MN34210PL_GAIN_TABLE[again_index][MN34210PL_GAIN_COL_AGAIN]);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int mn34210pl_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int mn34210pl_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;
	int gain_index;

	/* long frame */
	gain_index = MN34210PL_WDR_GAIN_30DB - p_dgain_gp->l;
	/* DGAIN-Gr */
	mn34210pl_write_reg(vdev, 0x020E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x020F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-R */
	mn34210pl_write_reg(vdev, 0x0210, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0211, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-B */
	mn34210pl_write_reg(vdev, 0x0212, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0213, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-Gb */
	mn34210pl_write_reg(vdev, 0x0214, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x0215, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

	/* short frame 1 */
	gain_index = MN34210PL_WDR_GAIN_30DB - p_dgain_gp->s1;
	/* DGAIN-Gr */
	mn34210pl_write_reg(vdev, 0x310A, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x310B, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-R */
	mn34210pl_write_reg(vdev, 0x310C, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x310D, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-B */
	mn34210pl_write_reg(vdev, 0x310E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x310F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-Gb */
	mn34210pl_write_reg(vdev, 0x3110, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3111, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

	/* short frame 2 */
	gain_index = MN34210PL_WDR_GAIN_30DB - p_dgain_gp->s2;
	/* DGAIN-Gr */
	mn34210pl_write_reg(vdev, 0x3112, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3113, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-R */
	mn34210pl_write_reg(vdev, 0x3114, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3115, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-B */
	mn34210pl_write_reg(vdev, 0x3116, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3117, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-Gb */
	mn34210pl_write_reg(vdev, 0x3118, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3119, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

	/* short frame 3 */
	gain_index = MN34210PL_WDR_GAIN_30DB - p_dgain_gp->s3;
	/* DGAIN-Gr */
	mn34210pl_write_reg(vdev, 0x311A, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x311B, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-R */
	mn34210pl_write_reg(vdev, 0x311C, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x311D, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-B */
	mn34210pl_write_reg(vdev, 0x311E, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x311F, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);
	/* DGAIN-Gb */
	mn34210pl_write_reg(vdev, 0x3120, (MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN] >> 8) & 0x3);
	mn34210pl_write_reg(vdev, 0x3121, (u8)MN34210PL_GAIN_TABLE[gain_index][MN34210PL_GAIN_COL_DGAIN]);

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int mn34210pl_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int mn34210pl_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u64 exposure_lines;
	int errCode = 0;
	struct mn34210pl_priv *pinfo = (struct mn34210pl_priv *)vdev->priv;

	/* long shutter */
	exposure_lines = p_shutter2row->l * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->l = (u32)exposure_lines;

	/* short shutter 1 */
	exposure_lines = p_shutter2row->s1 * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->s1 = (u32)exposure_lines;

	/* short shutter 2 */
	exposure_lines = p_shutter2row->s2 * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->s2 = (u32)exposure_lines;

	/* short shutter 3 */
	exposure_lines = p_shutter2row->s3 * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);
	p_shutter2row->s3 = (u32)exposure_lines;

	return errCode;
}

static struct vin_ops mn34210pl_ops = {
	.init_device		= mn34210pl_init_device,
	.set_format		= mn34210pl_set_format,
	.set_shutter_row = mn34210pl_set_shutter_row,
	.shutter2row = mn34210pl_shutter2row,
	.set_frame_rate		= mn34210pl_set_fps,
	.set_agc_index		= mn34210pl_set_agc_index,
	.set_mirror_mode	= mn34210pl_set_mirror_mode,
	.read_reg		= mn34210pl_read_reg,
	.write_reg		= mn34210pl_write_reg,

	/* for wdr sensor */
	.set_wdr_again_idx_gp = mn34210pl_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = mn34210pl_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = mn34210pl_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = mn34210pl_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = mn34210pl_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = mn34210pl_get_wdr_shutter_row_group,
	.wdr_shutter2row = mn34210pl_wdr_shutter2row,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int mn34210pl_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct mn34210pl_priv *mn34210pl;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_MN34210PL, sizeof(struct mn34210pl_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x3C000000;	/* 60dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
#if GAIN_160_STEPS
	vdev->agc_db_step = 0x00600000;	/* 0.375dB */
#else
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
#endif
	vdev->pixel_size = 0x0003CCCD;	/* 3.8um */

	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = MN34210PL_WDR_GAIN_30DB;
	vdev->wdr_dgain_idx_min = 0;
	vdev->wdr_dgain_idx_max = MN34210PL_WDR_GAIN_30DB;

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	mn34210pl = (struct mn34210pl_priv *)vdev->priv;
	mn34210pl->control_data = client;

	rval = ambarella_vin_register_device(vdev, &mn34210pl_ops,
			mn34210pl_formats, ARRAY_SIZE(mn34210pl_formats),
			mn34210pl_plls, ARRAY_SIZE(mn34210pl_plls));
	if (rval < 0)
		goto mn34210pl_probe_err;

	vin_info("MN34210PL init(4-lane lvds)\n");

	return 0;

mn34210pl_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int mn34210pl_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id mn34210pl_idtable[] = {
	{ "mn34210pl", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mn34210pl_idtable);

static struct i2c_driver i2c_driver_mn34210pl = {
	.driver = {
		.name	= "mn34210pl",
	},

	.id_table	= mn34210pl_idtable,
	.probe		= mn34210pl_probe,
	.remove		= mn34210pl_remove,

};

static int __init mn34210pl_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("mn34210pl", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_mn34210pl);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit mn34210pl_exit(void)
{
	i2c_del_driver(&i2c_driver_mn34210pl);
}

module_init(mn34210pl_init);
module_exit(mn34210pl_exit);

MODULE_DESCRIPTION("MN34210PL 1/3 -Inch, 1296x1032, 1.3-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

