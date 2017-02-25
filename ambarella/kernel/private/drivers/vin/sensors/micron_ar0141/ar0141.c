/*
 * Filename : ar0141.c
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
#include "ar0141.h"

static int bus_addr = (0 << 16) | (0x20 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ar0141_priv {
	void *control_data;
	u32 frame_length_lines;
	u32 line_length;
};

#include "ar0141_table.c"

static int ar0141_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ar0141_priv *ar0141;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ar0141 = (struct ar0141_priv *)vdev->priv;
	client = ar0141->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = (data & 0xff00) >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == 0x3054))
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

static int ar0141_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ar0141_priv *ar0141;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	ar0141 = (struct ar0141_priv *)vdev->priv;
	client = ar0141->control_data;

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
	if (rval < 0){
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = (pbuf[0] << 8) | pbuf[1];

	return 0;
}

static int ar0141_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ar0141_config;

	memset(&ar0141_config, 0, sizeof (ar0141_config));

	ar0141_config.interface_type = SENSOR_SERIAL_LVDS;
	ar0141_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ar0141_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	ar0141_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_HISPI;

	ar0141_config.cap_win.x = format->def_start_x;
	ar0141_config.cap_win.y = format->def_start_y;
	ar0141_config.cap_win.width = format->def_width;
	ar0141_config.cap_win.height = format->def_height;

	ar0141_config.sensor_id	= GENERIC_SENSOR;
	ar0141_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ar0141_config.bayer_pattern	= format->bayer_pattern;
	ar0141_config.video_format	= format->format;
	ar0141_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ar0141_config);
}

static void ar0141_sw_reset(struct vin_device *vdev)
{
	ar0141_write_reg(vdev, AR0141_RESET_REGISTER, 0x0001);//Register RESET_REGISTER
	msleep(10);
	ar0141_write_reg(vdev, AR0141_RESET_REGISTER, 0x10D8);//Register RESET_REGISTER
}

static void ar0141_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ar0141_share_regs;
	regs_num = ARRAY_SIZE(ar0141_share_regs);
	for (i = 0; i < regs_num; i++) {
		if (regs[i].addr == 0xffff)
			msleep(regs[i].data);
		else
			ar0141_write_reg(vdev, regs[i].addr, regs[i].data);
	}
}

static int ar0141_init_device(struct vin_device *vdev)
{
	ar0141_sw_reset(vdev);

	ar0141_fill_share_regs(vdev);

	return 0;
}

static int ar0141_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = ar0141_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ar0141_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		ar0141_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void ar0141_start_streaming(struct vin_device *vdev)
{
	u32 data;
	ar0141_read_reg(vdev, AR0141_RESET_REGISTER, &data);
	data = (data | 0x0004);
	ar0141_write_reg(vdev, AR0141_RESET_REGISTER, data);//start streaming
}

static int ar0141_update_hv_info(struct vin_device *vdev)
{
	struct ar0141_priv *pinfo = (struct ar0141_priv *)vdev->priv;

	ar0141_read_reg(vdev, AR0141_LINE_LENGTH_PCK, &pinfo->line_length);
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ar0141_read_reg(vdev, AR0141_FRAME_LENGTH_LINES, &pinfo->frame_length_lines);

	return 0;
}

static int ar0141_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ar0141_priv *pinfo = (struct ar0141_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ar0141_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num, rval;

	regs = ar0141_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ar0141_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		ar0141_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ar0141_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ar0141_get_line_time(vdev);

	/* Enable Streaming */
	ar0141_start_streaming(vdev);

	/* communiate with IAV */
	rval = ar0141_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ar0141_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;
	struct ar0141_priv *pinfo = (struct ar0141_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	min_line = 1;
	max_line = pinfo->frame_length_lines;
	num_line = clamp(num_line, min_line, max_line);

	ar0141_write_reg(vdev, AR0141_COARSE_INTEGRATION_TIME, num_line);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ar0141_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ar0141_priv *pinfo = (struct ar0141_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ar0141_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ar0141_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct ar0141_priv *pinfo = (struct ar0141_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ar0141_write_reg(vdev, AR0141_FRAME_LENGTH_LINES, v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ar0141_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > AR0141_GAIN_48DB) {
		vin_error("agc index %d exceeds maximum %d\n", agc_idx, AR0141_GAIN_48DB);
		return -EINVAL;
	}

	ar0141_write_reg(vdev, AR0141_AGAIN,
		AR0141_GAIN_TABLE[agc_idx][AR0141_GAIN_COL_AGAIN]);
	ar0141_write_reg(vdev, AR0141_DGAIN,
		AR0141_GAIN_TABLE[agc_idx][AR0141_GAIN_COL_DGAIN]);
	ar0141_write_reg(vdev, AR0141_DCG_CTL,
		AR0141_GAIN_TABLE[agc_idx][AR0141_GAIN_COL_DCG]);

	return 0;
}

static int ar0141_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = AR0141_H_MIRROR + AR0141_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = AR0141_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = AR0141_V_FLIP;
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

	ar0141_read_reg(vdev, AR0141_READ_MODE, &tmp_reg);
	tmp_reg &= (~AR0141_MIRROR_MASK);
	tmp_reg |= readmode;
	ar0141_write_reg(vdev, AR0141_READ_MODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops ar0141_ops = {
	.init_device		= ar0141_init_device,
	.set_pll		= ar0141_set_pll,
	.set_format		= ar0141_set_format,
	.set_shutter_row = ar0141_set_shutter_row,
	.shutter2row = ar0141_shutter2row,
	.set_frame_rate		= ar0141_set_fps,
	.set_agc_index		= ar0141_set_agc_index,
	.set_mirror_mode	= ar0141_set_mirror_mode,
	.read_reg		= ar0141_read_reg,
	.write_reg		= ar0141_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ar0141_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ar0141_priv *ar0141;
	u32 version;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_AR0141, sizeof(struct ar0141_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2BE00000;	/* 43.875dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */

	i2c_set_clientdata(client, vdev);

	ar0141 = (struct ar0141_priv *)vdev->priv;
	ar0141->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ar0141_ops,
			ar0141_formats, ARRAY_SIZE(ar0141_formats),
			ar0141_plls, ARRAY_SIZE(ar0141_plls));
	if (rval < 0)
		goto ar0141_probe_err;

	/* query sensor id */
	ar0141_read_reg(vdev, AR0141_CHIP_VERSION_REG, &version);
	vin_info("AR0141 init(4-lane HISPI), sensor ID: 0x%x\n", version);

	return 0;

ar0141_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ar0141_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ar0141_idtable[] = {
	{ "ar0141", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0141_idtable);

static struct i2c_driver i2c_driver_ar0141 = {
	.driver = {
		.name	= "ar0141",
	},

	.id_table	= ar0141_idtable,
	.probe		= ar0141_probe,
	.remove		= ar0141_remove,

};

static int __init ar0141_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ar0141", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ar0141);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ar0141_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0141);
}

module_init(ar0141_init);
module_exit(ar0141_exit);

MODULE_DESCRIPTION("AR0141 1/4 -Inch, 1280x800, 1.0-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

