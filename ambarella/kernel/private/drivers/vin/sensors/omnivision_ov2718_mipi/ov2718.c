/*
 * Filename : ov2718.c
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
#include "ov2718.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ov2718_priv {
	void *control_data;
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 line_length;
	u32 frame_length_lines;
	u8 hold_mode;
	u32 dgain_r_ratio;
	u32 dgain_gr_ratio;
	u32 dgain_gb_ratio;
	u32 dgain_b_ratio;
};

#include "ov2718_table.c"

static int ov2718_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2718_priv *ov2718;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov2718 = (struct ov2718_priv *)vdev->priv;
	client = ov2718->control_data;

	pbuf[0] = (subaddr >> 8) | (ov2718->hold_mode << 7);
	pbuf[1] = (subaddr & 0xff);
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;

	if (unlikely(subaddr == OV2718_SWRESET))
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

static int ov2718_write_reg2(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov2718_priv *ov2718;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ov2718 = (struct ov2718_priv *)vdev->priv;
	client = ov2718->control_data;

	pbuf[0] = (subaddr >> 8) | (ov2718->hold_mode << 7);
	pbuf[1] = (subaddr & 0xff);
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

static int ov2718_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov2718_priv *ov2718;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov2718 = (struct ov2718_priv *)vdev->priv;
	client = ov2718->control_data;

	pbuf0[0] = (subaddr >> 8);
	pbuf0[1] = (subaddr & 0xff);

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

static int ov2718_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov2718_config;

	memset(&ov2718_config, 0, sizeof (ov2718_config));

	ov2718_config.interface_type = SENSOR_MIPI;
	ov2718_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov2718_config.mipi_cfg.lane_number = SENSOR_4_LANE;

	ov2718_config.cap_win.x = format->def_start_x;
	ov2718_config.cap_win.y = format->def_start_y;
	ov2718_config.cap_win.width = format->def_width;
	ov2718_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	ov2718_config.hdr_cfg.act_win.x = format->act_start_x;
	ov2718_config.hdr_cfg.act_win.y = format->act_start_y;
	ov2718_config.hdr_cfg.act_win.width = format->act_width;
	ov2718_config.hdr_cfg.act_win.height = format->act_height;

	ov2718_config.sensor_id	= GENERIC_SENSOR;
	ov2718_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov2718_config.bayer_pattern	= format->bayer_pattern;
	ov2718_config.video_format	= format->format;
	ov2718_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov2718_config);
}

static void ov2718_sw_reset(struct vin_device *vdev)
{
	ov2718_write_reg(vdev, OV2718_SWRESET, 0x01);
	msleep(10);
}

static int ov2718_init_device(struct vin_device *vdev)
{
	ov2718_sw_reset(vdev);
	return 0;
}

static void ov2718_start_streaming(struct vin_device *vdev)
{
	ov2718_write_reg(vdev, OV2718_STANDBY, 0x01); /* cancel stanby */
}

static int ov2718_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	if (hold_mode) {/* select group 0 for record */
		ov2718_write_reg(vdev, OV2718_OPERATION_CTRL, 0x00);
		ov2718_write_reg(vdev, OV2718_GROUP_CTRL, 0x00);
		pinfo->hold_mode = 1;
	} else {	/* single launch group 0 */
		pinfo->hold_mode = 0;
		ov2718_write_reg(vdev, OV2718_GROUP_CTRL, 0x10);
		ov2718_write_reg(vdev, OV2718_OPERATION_CTRL, 0x01);
	}

	return 0;
}

static int ov2718_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov2718_update_hv_info(struct vin_device *vdev)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u32 data_h, data_l;

	ov2718_read_reg(vdev, OV2718_HTS_H, &data_h);
	ov2718_read_reg(vdev, OV2718_HTS_L, &data_l);
	pinfo->line_length = (data_h<<8) + data_l;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov2718_read_reg(vdev, OV2718_VTS_H, &data_h);
	ov2718_read_reg(vdev, OV2718_VTS_L, &data_l);
	pinfo->frame_length_lines = (data_h<<8) + data_l;

	pinfo->dgain_r_ratio = 1024; /* 1x */
	pinfo->dgain_gr_ratio = 1024; /* 1x */
	pinfo->dgain_gb_ratio = 1024; /* 1x */
	pinfo->dgain_b_ratio = 1024; /* 1x */

	vin_debug("line_length:%d, frame_length_lines:%d\n",
		pinfo->line_length, pinfo->frame_length_lines);

	return 0;
}

static int ov2718_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		regs = ov2718_linear_regs;
		regs_num = ARRAY_SIZE(ov2718_linear_regs);
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		regs = ov2718_dual_gain_hdr_regs;
		regs_num = ARRAY_SIZE(ov2718_dual_gain_hdr_regs);
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		regs = ov2718_dcg_vs_hdr_regs;
		regs_num = ARRAY_SIZE(ov2718_dcg_vs_hdr_regs);
		break;
	case AMBA_VIDEO_INT_HDR_MODE:
		regs = ov2718_12b_combined_hdr_regs;
		regs_num = ARRAY_SIZE(ov2718_12b_combined_hdr_regs);
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_error("Unknown mode\n");
		return -EINVAL;
	}

	for (i = 0; i < regs_num; i++) {
		ov2718_write_reg(vdev, regs[i].addr, regs[i].data);
	}

	rval = ov2718_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov2718_get_line_time(vdev);

	/* Enable Streaming */
	ov2718_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov2718_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov2718_set_shutter_row(struct vin_device *vdev, u32 row)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;

	num_line = row;

	/* FIXME: shutter width: 1 ~(Frame format(V) - 5) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 5;
	num_line = clamp(num_line, min_line, max_line);

	ov2718_write_reg2(vdev, OV2718_CEXP_DCG_H, num_line&0xFFFF);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ov2718_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(!pinfo->line_length) {
		rval = ov2718_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov2718_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov2718_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u16 shutter_long, shutter_short;

	/* DCG shutter */
	shutter_long = p_shutter_gp->l;

	/* VS shutter */
	shutter_short = p_shutter_gp->s2;

	if ((shutter_long + shutter_short >= pinfo->frame_length_lines - 5) ||
		(1080 + shutter_short + 1 > pinfo->frame_length_lines - 22)) {
		vin_error("shutter exceeds limitation! long:%d, short1:%d, V:%d\n",
			shutter_long, shutter_short, pinfo->frame_length_lines);
		return -EPERM;
	}

	ov2718_write_reg2(vdev, OV2718_CEXP_DCG_H, shutter_long&0xFFFF);

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {
		ov2718_write_reg2(vdev, OV2718_CEXP_VS_H, shutter_short&0xFFFF);
	}

	memcpy(&(pinfo->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("shutter long:%d, short:%d\n", p_shutter_gp->l, p_shutter_gp->s2);

	return 0;
}

static int ov2718_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(pinfo->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int ov2718_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u32 exposure_lines;
	int rval = 0;

	/* long shutter */
	exposure_lines = p_shutter2row->l;
	ov2718_shutter2row(vdev, &exposure_lines);
	p_shutter2row->l = exposure_lines;

	exposure_lines = p_shutter2row->s1;
	ov2718_shutter2row(vdev, &exposure_lines);
	p_shutter2row->s1 = exposure_lines;

	return rval;
}

static int ov2718_set_fps(struct vin_device *vdev, int fps)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u64 pixelclk, v_lines, vb_time;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ov2718_write_reg2(vdev, OV2718_VTS_H, v_lines&0xFFFF);

	pinfo->frame_length_lines = v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov2718_convert_dgain_ratio(u32 ratio, u16 *new_dgain)
{
	u32 tmp_dgain;

	tmp_dgain = ratio >> 2;/* equals to *256/1024, 256 is dgain base, 1024 is ratio unit */

	if (ratio < 4) {/* dgain value should be no less than 0x01 */
		ratio = 4;
		vin_warn("ratio should not be less than 4!\n");
	}
	*new_dgain = tmp_dgain;

	vin_debug("ratio=%d, new_dgain=%d\n", ratio, *new_dgain);

	return 0;
}

static int ov2718_set_dgain_ratio(struct vin_device *vdev, struct vindev_dgain_ratio *p_dgain_ratio)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u16 new_dgain;
	u32 blc_offset;
	int errCode = 0;

	/* r */
	ov2718_convert_dgain_ratio(p_dgain_ratio->r_ratio, &new_dgain);
	ov2718_write_reg2(vdev, OV2718_R_GAIN_HCG_H, new_dgain);
	ov2718_write_reg2(vdev, OV2718_R_GAIN_LCG_H, new_dgain);
	pinfo->dgain_r_ratio = p_dgain_ratio->r_ratio;
	blc_offset = (new_dgain - 1) << 6;/* FIXME: *blc_taget:0x40 */
	ov2718_write_reg2(vdev, OV2718_R_OFFSET_HCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_R_OFFSET_HCG_L, blc_offset&0xFF);
	ov2718_write_reg2(vdev, OV2718_R_OFFSET_LCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_R_OFFSET_LCG_L, blc_offset&0xFF);

	/* gr */
	ov2718_convert_dgain_ratio(p_dgain_ratio->gr_ratio, &new_dgain);
	ov2718_write_reg2(vdev, OV2718_GR_GAIN_HCG_H, new_dgain);
	ov2718_write_reg2(vdev, OV2718_GR_GAIN_LCG_H, new_dgain);
	pinfo->dgain_gr_ratio = p_dgain_ratio->gr_ratio;
	blc_offset = (new_dgain - 1) << 6;/* FIXME: *blc_taget:0x40 */
	ov2718_write_reg2(vdev, OV2718_GR_OFFSET_HCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_GR_OFFSET_HCG_L, blc_offset&0xFF);
	ov2718_write_reg2(vdev, OV2718_GR_OFFSET_LCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_GR_OFFSET_LCG_L, blc_offset&0xFF);

	/* gb */
	ov2718_convert_dgain_ratio(p_dgain_ratio->gb_ratio, &new_dgain);
	ov2718_write_reg2(vdev, OV2718_GB_GAIN_HCG_H, new_dgain);
	ov2718_write_reg2(vdev, OV2718_GB_GAIN_LCG_H, new_dgain);
	pinfo->dgain_gb_ratio = p_dgain_ratio->gb_ratio;
	blc_offset = (new_dgain - 1) << 6;/* FIXME: *blc_taget:0x40 */
	ov2718_write_reg2(vdev, OV2718_GB_OFFSET_HCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_GB_OFFSET_HCG_L, blc_offset&0xFF);
	ov2718_write_reg2(vdev, OV2718_GB_OFFSET_LCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_GB_OFFSET_LCG_L, blc_offset&0xFF);

	/* b */
	ov2718_convert_dgain_ratio(p_dgain_ratio->b_ratio, &new_dgain);
	ov2718_write_reg2(vdev, OV2718_B_GAIN_HCG_H, new_dgain);
	ov2718_write_reg2(vdev, OV2718_B_GAIN_LCG_H, new_dgain);
	pinfo->dgain_b_ratio = p_dgain_ratio->b_ratio;
	blc_offset = (new_dgain - 1) << 6;/* FIXME: *blc_taget:0x40 */
	ov2718_write_reg2(vdev, OV2718_B_OFFSET_HCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_B_OFFSET_HCG_L, blc_offset&0xFF);
	ov2718_write_reg2(vdev, OV2718_B_OFFSET_LCG_H, (blc_offset>>8)&0xFFFF);
	ov2718_write_reg(vdev, OV2718_B_OFFSET_LCG_L, blc_offset&0xFF);

	return errCode;
}

static int ov2718_get_dgain_ratio(struct vin_device *vdev, struct vindev_dgain_ratio *p_dgain_ratio)
{
	int errCode = 0;
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	p_dgain_ratio->r_ratio = pinfo->dgain_r_ratio;
	p_dgain_ratio->gr_ratio = pinfo->dgain_gr_ratio;
	p_dgain_ratio->gb_ratio = pinfo->dgain_gb_ratio;
	p_dgain_ratio->b_ratio = pinfo->dgain_b_ratio;

	return errCode;
}

static int ov2718_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV2718_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV2718_GAIN_MAXDB);
		agc_idx = OV2718_GAIN_MAXDB;
	}

	/* analog gain */
	ov2718_write_reg(vdev, OV2718_CG_AGAIN, OV2718_GAIN_TABLE[agc_idx][OV2718_GAIN_COL_AGAIN]);

	/* digital gain */
	ov2718_write_reg2(vdev, OV2718_DIG_GAIN_HCG_H, OV2718_GAIN_TABLE[agc_idx][OV2718_GAIN_COL_DGAIN]);

	return 0;
}

static int ov2718_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;
	u32 cg_again, gain_index;

	if (p_again_gp->l >= 222) {
		gain_index = p_again_gp->l - 222;/* HCG has 11X sensitivity */
	} else {
		vin_warn("long gain index should not be less than 222!\n");
		gain_index = 0;
	}

	/* HCG frame */
	/* digital gain */
	ov2718_write_reg2(vdev, OV2718_DIG_GAIN_HCG_H,
		OV2718_HDR_GAIN_TABLE[gain_index][OV2718_HDR_GAIN_COL_DGAIN]);
	cg_again = OV2718_HDR_GAIN_TABLE[gain_index][OV2718_HDR_GAIN_COL_AGAIN];

	/* LCG frame */
	/* digital gain */
	ov2718_write_reg2(vdev, OV2718_DIG_GAIN_LCG_H,
		OV2718_HDR_GAIN_TABLE[p_again_gp->s1][OV2718_HDR_GAIN_COL_DGAIN]);
	cg_again |= (OV2718_HDR_GAIN_TABLE[p_again_gp->s1][OV2718_HDR_GAIN_COL_AGAIN]<<2);

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_3X_HDR_MODE) {/* VS frame */
		/* digital gain */
		ov2718_write_reg2(vdev, OV2718_DIG_GAIN_VS_H,
			OV2718_HDR_GAIN_TABLE[p_again_gp->s2][OV2718_HDR_GAIN_COL_DGAIN]);
		cg_again |= (OV2718_HDR_GAIN_TABLE[p_again_gp->s2][OV2718_HDR_GAIN_COL_AGAIN]<<2);
	}

	/* analog gain */
	ov2718_write_reg(vdev, OV2718_CG_AGAIN, cg_again);

	memcpy(&(pinfo->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d, short2 again index:%d\n",
		p_again_gp->l, p_again_gp->s1, p_again_gp->s2);

	return 0;
}

static int ov2718_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	memcpy(p_again_gp, &(pinfo->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov2718_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	memcpy(&(pinfo->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long dgain index:%d, short1 dgain index:%d\n",
		p_dgain_gp->l, p_dgain_gp->s1);

	return 0;
}

static int ov2718_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct ov2718_priv *pinfo = (struct ov2718_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(pinfo->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int ov2718_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	int errCode = 0;
	u32 tmp_reg, bayer_pattern, read_mode = 0;
	u32 h_offs, cfa_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		read_mode = 0;
		h_offs = 0x05;
		cfa_pattern = 0x21;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		read_mode = OV2718_V_FLIP;
		h_offs = 0x05;
		cfa_pattern = 0x23;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		read_mode = OV2718_H_MIRROR;
		h_offs = 0x04;
		cfa_pattern = 0x20;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		read_mode = OV2718_H_MIRROR + OV2718_V_FLIP;
		h_offs = 0x04;
		cfa_pattern = 0x22;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= ov2718_read_reg(vdev,OV2718_READ_MODE,&tmp_reg);
	tmp_reg |= OV2718_H_MIRROR;
	tmp_reg &= (~OV2718_V_FLIP);
	tmp_reg ^= read_mode;
	ov2718_write_reg(vdev, OV2718_READ_MODE, tmp_reg);
	ov2718_write_reg(vdev, OV2718_ODP_H_OFFS_L, h_offs);
	ov2718_write_reg(vdev, OV2718_ISP_SETTING, cfa_pattern);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}

static struct vin_ops ov2718_ops = {
	.init_device		= ov2718_init_device,
	.set_format		= ov2718_set_format,
	.set_shutter_row 	= ov2718_set_shutter_row,
	.shutter2row 		= ov2718_shutter2row,
	.set_frame_rate	= ov2718_set_fps,
	.set_agc_index		= ov2718_set_agc_index,
	.set_mirror_mode	= ov2718_set_mirror_mode,
	.set_hold_mode		= ov2718_set_hold_mode,
	.read_reg			= ov2718_read_reg,
	.write_reg		= ov2718_write_reg,

	/* for build-in hdr */
	.set_dgain_ratio	= ov2718_set_dgain_ratio,
	.get_dgain_ratio	= ov2718_get_dgain_ratio,

	/* for wdr sensor */
	.set_wdr_again_idx_gp = ov2718_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = ov2718_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = ov2718_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = ov2718_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = ov2718_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = ov2718_get_wdr_shutter_row_group,
	.wdr_shutter2row = ov2718_wdr_shutter2row,
};

/* ========================================================================== */
static int ov2718_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov2718_priv *ov2718;
	u32 cid_l, cid_h;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV2718, sizeof(struct ov2718_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x42180000; /* 66.09375dB */
	vdev->agc_db_min = 0x00000000; /* 0dB */
	vdev->agc_db_step = 0x00180000; /* 0.09375dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = OV2718_HDR_GAIN_MAXDB;

	i2c_set_clientdata(client, vdev);

	ov2718 = (struct ov2718_priv *)vdev->priv;
	ov2718->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ov2718_ops,
				ov2718_formats, ARRAY_SIZE(ov2718_formats),
				ov2718_plls, ARRAY_SIZE(ov2718_plls));
	if (rval < 0)
		goto ov2718_probe_err;

	/* query sensor id */
	ov2718_read_reg(vdev, OV2718_CHIP_ID_H, &cid_h);
	ov2718_read_reg(vdev, OV2718_CHIP_ID_L, &cid_l);
	vin_info("OV2718 init(4-lane mipi), sensor ID: 0x%x\n", (cid_h<<8)+cid_l);

	return 0;

ov2718_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov2718_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov2718_idtable[] = {
	{ "ov2718", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2718_idtable);

static struct i2c_driver i2c_driver_ov2718 = {
	.driver = {
		.name	= "ov2718",
	},

	.id_table	= ov2718_idtable,
	.probe		= ov2718_probe,
	.remove		= ov2718_remove,

};

static int __init ov2718_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov2718", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov2718);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov2718_exit(void)
{
	i2c_del_driver(&i2c_driver_ov2718);
}

module_init(ov2718_init);
module_exit(ov2718_exit);

MODULE_DESCRIPTION("OV2718 1/3 -Inch, 1936x1096, 2.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

