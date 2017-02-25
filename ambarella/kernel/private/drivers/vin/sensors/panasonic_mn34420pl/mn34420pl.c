/*
 * Filename : mn34420pl.c
 *
 * History:
 *    2015/12/15 - [Hao Zeng] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include "mn34420pl.h"
#include "mn34420pl_table.c"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct mn34420pl_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
};

static int mn34420pl_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct mn34420pl_priv *mn34420pl;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	mn34420pl = (struct mn34420pl_priv *)vdev->priv;
	client = mn34420pl->control_data;

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

static int mn34420pl_write_reg2(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct mn34420pl_priv *mn34420pl;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	mn34420pl = (struct mn34420pl_priv *)vdev->priv;
	client = mn34420pl->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
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

static int mn34420pl_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct mn34420pl_priv *mn34420pl;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	mn34420pl = (struct mn34420pl_priv *)vdev->priv;
	client = mn34420pl->control_data;

	pbuf0[0] = (subaddr & 0xff00) >> 8;
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
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int mn34420pl_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config mn34420pl_config;

	memset(&mn34420pl_config, 0, sizeof(mn34420pl_config));

	mn34420pl_config.interface_type = SENSOR_SERIAL_LVDS;
	mn34420pl_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	if (format->video_mode == AMBA_VIDEO_MODE_WUXGA) {
		mn34420pl_config.slvds_cfg.lane_number = SENSOR_2_LANE;
	} else {
		if (format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE ||
			format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
			mn34420pl_config.slvds_cfg.lane_number = SENSOR_4_LANE;
		} else {
			mn34420pl_config.slvds_cfg.lane_number = SENSOR_6_LANE;
		}
	}

	mn34420pl_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_PANASONIC;

	mn34420pl_config.cap_win.x = format->def_start_x;
	mn34420pl_config.cap_win.y = format->def_start_y;
	mn34420pl_config.cap_win.width = format->def_width;
	mn34420pl_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	mn34420pl_config.hdr_cfg.act_win.x = format->act_start_x;
	mn34420pl_config.hdr_cfg.act_win.y = format->act_start_y;
	mn34420pl_config.hdr_cfg.act_win.width = format->act_width;
	mn34420pl_config.hdr_cfg.act_win.height = format->act_height;

	mn34420pl_config.sensor_id	= GENERIC_SENSOR;
	mn34420pl_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	mn34420pl_config.bayer_pattern	= format->bayer_pattern;
	mn34420pl_config.video_format	= format->format;
	mn34420pl_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &mn34420pl_config);
}

static int mn34420pl_init_device(struct vin_device *vdev)
{
	return 0;
}

static int mn34420pl_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	mn34420pl_write_reg(vdev, MN34420PL_VLATCH_HOLD, 0X70 | (hold_mode & 0x01));
	return 0;
}

static int mn34420pl_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	mn34420pl_read_reg(vdev, MN34420PL_HCYCLE_H, &val_high);
	mn34420pl_read_reg(vdev, MN34420PL_HCYCLE_L, &val_low);
	pinfo->line_length = (val_high << 8) + val_low + 1;
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	mn34420pl_read_reg(vdev, MN34420PL_VCYCLE_H, &val_high);
	mn34420pl_read_reg(vdev, MN34420PL_VCYCLE_L, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low + 1;

	return 0;
}

static int mn34420pl_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int mn34420pl_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;
	int rval;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		if (format->video_mode == AMBA_VIDEO_MODE_WUXGA) {
			regs = mn34420pl_2lane_mode_regs;
			regs_num = ARRAY_SIZE(mn34420pl_2lane_mode_regs);
		} else {
			regs = mn34420pl_linear_mode_regs;
			regs_num = ARRAY_SIZE(mn34420pl_linear_mode_regs);
		}
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		regs = mn34420pl_2x_hdr_mode_regs;
		regs_num = ARRAY_SIZE(mn34420pl_2x_hdr_mode_regs);
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		regs = mn34420pl_3x_hdr_mode_regs;
		regs_num = ARRAY_SIZE(mn34420pl_3x_hdr_mode_regs);
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_error("Unknown mode\n");
		return -EPERM;
	}

	for (i = 0; i < regs_num; i++)
		mn34420pl_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = mn34420pl_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	mn34420pl_get_line_time(vdev);

	return mn34420pl_set_vin_mode(vdev, format);
}

static int mn34420pl_set_fps(struct vin_device *vdev, int fps)
{
	u64	v_lines, vb_time;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	mn34420pl_write_reg2(vdev, MN34420PL_VCYCLE_H, (v_lines - 1)&0xFFFF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int mn34420pl_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (vdev->cur_format->hdr_mode != AMBA_VIDEO_LINEAR_MODE) {
		return 0;
	}

	if (agc_idx > MN34420PL_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, MN34420PL_GAIN_MAXDB);
		agc_idx = MN34420PL_GAIN_MAXDB;
	}

	/* analog gain */
	mn34420pl_write_reg2(vdev, MN34420PL_AGAIN_H, MN34420PL_GAIN_TABLE[agc_idx][MN34420PL_GAIN_COL_AGAIN]&0x3FFF);
	/* didgital gain */
	mn34420pl_write_reg2(vdev, MN34420PL_DGAIN_H, MN34420PL_GAIN_TABLE[agc_idx][MN34420PL_GAIN_COL_DGAIN]&0x3FFF);

	return 0;
}

static int mn34420pl_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	u32 blank_lines;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 2) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	blank_lines = pinfo->frame_length_lines - num_line;
	mn34420pl_write_reg2(vdev, MN34420PL_SHTPOS_H, blank_lines);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int mn34420pl_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	if (unlikely(!pinfo->line_length)) {
		rval = mn34420pl_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int mn34420pl_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;
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
	switch (vdev->cur_format->hdr_mode) {
	case AMBA_VIDEO_2X_HDR_MODE:
		if (shutter_long + shutter_short1 + 4 > pinfo->frame_length_lines) {
			vin_error("shutter exceeds limitation! long:%d, short:%d, V:%d\n",
				shutter_long, shutter_short1, pinfo->frame_length_lines);
			return -EPERM;
		} else if (shutter_short1 + 2 > pinfo->frame_length_lines - active_lines) {
			vin_error("short frame offset exceeds limitation! short:%d, VB:%d\n",
				shutter_short1, pinfo->frame_length_lines - active_lines);
			return -EPERM;
		}
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		if (shutter_long + shutter_short1 + shutter_short2 + 6 > pinfo->frame_length_lines) {
			vin_error("shutter exceeds limitation! long:%d, short1:%d, short2:%d, V:%d\n",
				shutter_long, shutter_short1, shutter_short2, pinfo->frame_length_lines);
			return -EPERM;
		} else if (shutter_short1 + shutter_short2 + 4 > pinfo->frame_length_lines - active_lines) {
			vin_error("short frame offset exceeds limitation! short1:%d, short2:%d, VB:%d\n",
				shutter_short1, shutter_short2, pinfo->frame_length_lines - active_lines);
			return -EPERM;
		}
		break;
	case AMBA_VIDEO_4X_HDR_MODE:
		if (shutter_long + shutter_short1 + shutter_short2 + 8 > pinfo->frame_length_lines) {
			vin_error("shutter exceeds limitation! long:%d, short1:%d, short2:%d, short3:%d, V:%d\n",
				shutter_long, shutter_short1, shutter_short2, shutter_short3, pinfo->frame_length_lines);
			return -EPERM;
		} else if (shutter_short1 + shutter_short2 + shutter_short3 + 6 > pinfo->frame_length_lines - active_lines) {
			vin_error("short frame offset exceeds limitation! short1:%d, short2:%d, short3:%d, VB:%d\n",
				shutter_short1, shutter_short2, shutter_short3, pinfo->frame_length_lines - active_lines);
			return -EPERM;
		}
		break;
	case AMBA_VIDEO_LINEAR_MODE:
	default:
		vin_error("Unsupported mode\n");
		return -EPERM;
	}

	/* long shutter */
	shutter_long = pinfo->frame_length_lines - shutter_long;
	mn34420pl_write_reg2(vdev, MN34420PL_SHTPOS_H, shutter_long);
	/* short shutter 1 */
	mn34420pl_write_reg2(vdev, MN34420PL_SHTPOS_WDR1, shutter_short1);
	/* short shutter 2 */
	mn34420pl_write_reg2(vdev, MN34420PL_SHTPOS_WDR2, shutter_short2);
	/* short shutter 3 */
	mn34420pl_write_reg2(vdev, MN34420PL_SHTPOS_WDR3, shutter_short3);

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	switch (vdev->cur_format->hdr_mode) {
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

static int mn34420pl_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int mn34420pl_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;
	u32 again_index;

	again_index = p_again_gp->l;
	mn34420pl_write_reg2(vdev, MN34420PL_AGAIN_H, MN34420PL_GAIN_TABLE[again_index][MN34420PL_GAIN_COL_AGAIN]&0x3FFF);
	again_index = p_again_gp->s1;
	mn34420pl_write_reg2(vdev, MN34420PL_AGAIN_WDR1, MN34420PL_GAIN_TABLE[again_index][MN34420PL_GAIN_COL_AGAIN]&0x3FFF);
	again_index = p_again_gp->s2;
	mn34420pl_write_reg2(vdev, MN34420PL_AGAIN_WDR2, MN34420PL_GAIN_TABLE[again_index][MN34420PL_GAIN_COL_AGAIN]&0x3FFF);
	again_index = p_again_gp->s3;
	mn34420pl_write_reg2(vdev, MN34420PL_AGAIN_WDR3, MN34420PL_GAIN_TABLE[again_index][MN34420PL_GAIN_COL_AGAIN]&0x3FFF);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int mn34420pl_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int mn34420pl_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;
	int dgain_index;

	/* long frame */
	dgain_index = p_dgain_gp->l + MN34420PL_WDR_GAIN_30DB;
	mn34420pl_write_reg2(vdev, MN34420PL_DGAIN_H, MN34420PL_GAIN_TABLE[dgain_index][MN34420PL_GAIN_COL_DGAIN]&0x3FFF);
	dgain_index = p_dgain_gp->s1 + MN34420PL_WDR_GAIN_30DB;
	mn34420pl_write_reg2(vdev, MN34420PL_DGAIN_WDR1, MN34420PL_GAIN_TABLE[dgain_index][MN34420PL_GAIN_COL_DGAIN]&0x3FFF);
	dgain_index = p_dgain_gp->s2 + MN34420PL_WDR_GAIN_30DB;
	mn34420pl_write_reg2(vdev, MN34420PL_DGAIN_WDR2, MN34420PL_GAIN_TABLE[dgain_index][MN34420PL_GAIN_COL_DGAIN]&0x3FFF);
	dgain_index = p_dgain_gp->s3 + MN34420PL_WDR_GAIN_30DB;
	mn34420pl_write_reg2(vdev, MN34420PL_DGAIN_WDR3, MN34420PL_GAIN_TABLE[dgain_index][MN34420PL_GAIN_COL_DGAIN]&0x3FFF);

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int mn34420pl_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int mn34420pl_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u64 exposure_lines;
	int errCode = 0;
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

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

static int mn34420pl_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = MN34420PL_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
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

	mn34420pl_read_reg(vdev, MN34420PL_MODE, &tmp_reg);
	tmp_reg &= (~MN34420PL_V_FLIP);
	tmp_reg |= readmode;
	mn34420pl_write_reg(vdev, MN34420PL_MODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int mn34420pl_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct mn34420pl_priv *pinfo = (struct mn34420pl_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->frame_length_lines - 6;
	aaa_info->sht1_max = pinfo->frame_length_lines - vdev->cur_format->height - 2;
	aaa_info->sht2_max = (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) ?
		(aaa_info->sht1_max - 2) : 0;

	return 0;
}

#ifdef CONFIG_PM
static int mn34420pl_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		mn34420pl_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (tmp<<8);
		mn34420pl_read_reg(vdev, (pm_regs[i].addr)+1, &tmp);
		pm_regs[i].data |= (u16)tmp;
	}

	return 0;
}

static int mn34420pl_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++)
		mn34420pl_write_reg2(vdev, pm_regs[i].addr, pm_regs[i].data);

	return 0;
}
#endif

static struct vin_ops mn34420pl_ops = {
	.init_device		= mn34420pl_init_device,
	.set_format		= mn34420pl_set_format,
	.set_shutter_row	= mn34420pl_set_shutter_row,
	.shutter2row		= mn34420pl_shutter2row,
	.set_frame_rate		= mn34420pl_set_fps,
	.set_agc_index		= mn34420pl_set_agc_index,
	.set_mirror_mode	= mn34420pl_set_mirror_mode,
	.set_hold_mode		= mn34420pl_set_hold_mode,
	.get_aaa_info		= mn34420pl_get_aaa_info,
	.read_reg		= mn34420pl_read_reg,
	.write_reg		= mn34420pl_write_reg,
#ifdef CONFIG_PM
	.suspend		= mn34420pl_suspend,
	.resume 		= mn34420pl_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = mn34420pl_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = mn34420pl_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = mn34420pl_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = mn34420pl_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = mn34420pl_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = mn34420pl_get_wdr_shutter_row_group,
	.wdr_shutter2row = mn34420pl_wdr_shutter2row,
};

/* ========================================================================== */
static int mn34420pl_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct mn34420pl_priv *mn34420pl;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_MN34420PL, sizeof(struct mn34420pl_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x3C000000;	/* 60dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
#if GAIN_160_STEPS
	vdev->agc_db_step = 0x00600000;	/* 0.375dB */
#else
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */
#endif

	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = MN34420PL_WDR_GAIN_30DB;
	vdev->wdr_dgain_idx_min = 0;
	vdev->wdr_dgain_idx_max = MN34420PL_WDR_GAIN_30DB;

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	mn34420pl = (struct mn34420pl_priv *)vdev->priv;
	mn34420pl->control_data = client;

	rval = ambarella_vin_register_device(vdev, &mn34420pl_ops,
			mn34420pl_formats, ARRAY_SIZE(mn34420pl_formats),
			mn34420pl_plls, ARRAY_SIZE(mn34420pl_plls));
	if (rval < 0)
		goto mn34420pl_probe_err;

	vin_info("MN34420PL init(6-lane lvds)\n");

	return 0;

mn34420pl_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int mn34420pl_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id mn34420pl_idtable[] = {
	{ "mn34420pl", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mn34420pl_idtable);

static struct i2c_driver i2c_driver_mn34420pl = {
	.driver = {
		.name	= "mn34420pl",
	},

	.id_table	= mn34420pl_idtable,
	.probe		= mn34420pl_probe,
	.remove		= mn34420pl_remove,

};

static int __init mn34420pl_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("mn34420pl", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_mn34420pl);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit mn34420pl_exit(void)
{
	i2c_del_driver(&i2c_driver_mn34420pl);
}

module_init(mn34420pl_init);
module_exit(mn34420pl_exit);

MODULE_DESCRIPTION("MN34420PL 1/2.86 -Inch, 1944x1212, 2.4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

