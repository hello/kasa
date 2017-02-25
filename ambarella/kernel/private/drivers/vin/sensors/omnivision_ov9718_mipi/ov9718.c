/*
 * Filename : ov9718.c
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
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
#include "ov9718.h"

#ifndef CONFIG_STRAWBERRY_BOARD
static int bus_addr = (0 << 16) | (0x20 >> 1);
#else
static int bus_addr = (0 << 16) | (0x6C >> 1);
#endif
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct ov9718_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov9718_table.c"

static int ov9718_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov9718_priv *ov9718;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov9718 = (struct ov9718_priv *)vdev->priv;
	client = ov9718->control_data;

	pbuf[0] = (subaddr >> 8);
	pbuf[1] = (subaddr & 0xff);
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

static int ov9718_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov9718_priv *ov9718;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov9718 = (struct ov9718_priv *)vdev->priv;
	client = ov9718->control_data;

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

static int ov9718_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov9718_config;

	memset(&ov9718_config, 0, sizeof (ov9718_config));

	ov9718_config.interface_type = SENSOR_MIPI;
	ov9718_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov9718_config.mipi_cfg.lane_number = SENSOR_2_LANE;

	ov9718_config.cap_win.x = format->def_start_x;
	ov9718_config.cap_win.y = format->def_start_y;
	ov9718_config.cap_win.width = format->def_width;
	ov9718_config.cap_win.height = format->def_height;

	ov9718_config.sensor_id	= GENERIC_SENSOR;
	ov9718_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov9718_config.bayer_pattern	= format->bayer_pattern;
	ov9718_config.video_format	= format->format;
	ov9718_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov9718_config);
}

static void ov9718_sw_reset(struct vin_device *vdev)
{
	ov9718_write_reg(vdev, OV9718_SWRESET, 0x01);
	ov9718_write_reg(vdev, OV9718_STANDBY, 0x00);/* Standby */
	msleep(10);
}

static void ov9718_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ov9718_share_regs;
	regs_num = ARRAY_SIZE(ov9718_share_regs);
	for (i = 0; i < regs_num; i++)
		ov9718_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov9718_init_device(struct vin_device *vdev)
{
	ov9718_sw_reset(vdev);
	ov9718_fill_share_regs(vdev);

	return 0;
}

static int ov9718_set_pll(struct vin_device *vdev, int pll_idx)
{
	return 0;
}

static void ov9718_start_streaming(struct vin_device *vdev)
{
	ov9718_write_reg(vdev, OV9718_STANDBY, 0x01); //streaming
}

static int ov9718_update_hv_info(struct vin_device *vdev)
{
	u32 data_h, data_l;
	struct ov9718_priv *pinfo = (struct ov9718_priv *)vdev->priv;

	ov9718_read_reg(vdev, OV9718_HMAX_MSB, &data_h);
	ov9718_read_reg(vdev, OV9718_HMAX_LSB, &data_l);
	pinfo->line_length = (data_h<<8) + data_l;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov9718_read_reg(vdev, OV9718_VMAX_MSB, &data_h);
	ov9718_read_reg(vdev, OV9718_VMAX_LSB, &data_l);
	pinfo->frame_length_lines = (data_h<<8) + data_l;

	return 0;
}

static int ov9718_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov9718_priv *pinfo = (struct ov9718_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov9718_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;

	rval = ov9718_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov9718_get_line_time(vdev);

	/* Enable Streaming */
	ov9718_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov9718_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov9718_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct ov9718_priv *pinfo = (struct ov9718_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 2) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov9718_write_reg(vdev, OV9718_EXPO0_HSB, (num_line >> 16) & 0x0F);
	ov9718_write_reg(vdev, OV9718_EXPO0_MSB, (num_line >> 8) & 0xFF);
	ov9718_write_reg(vdev, OV9718_EXPO0_LSB, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int ov9718_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov9718_priv *pinfo = (struct ov9718_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov9718_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov9718_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct ov9718_priv *pinfo = (struct ov9718_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ov9718_write_reg(vdev, OV9718_VMAX_MSB, (u8)((v_lines & 0xFF00) >> 8));
	ov9718_write_reg(vdev, OV9718_VMAX_LSB, (u8)(v_lines & 0xFF));

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov9718_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV9718_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV9718_GAIN_0DB);
		agc_idx = OV9718_GAIN_0DB;
	}

	agc_idx = OV9718_GAIN_0DB - agc_idx;

	ov9718_write_reg(vdev, OV9718_GRP_ACCESS, 0x00);

	ov9718_write_reg(vdev, OV9718_GAIN_MSB, OV9718_GAIN_TABLE[agc_idx][OV9718_GAIN_COL_REG350A]);
	ov9718_write_reg(vdev, OV9718_GAIN_LSB, OV9718_GAIN_TABLE[agc_idx][OV9718_GAIN_COL_REG350B]);

	ov9718_write_reg(vdev, OV9718_GRP_ACCESS, 0x10);
	ov9718_write_reg(vdev, OV9718_GRP_ACCESS, 0xA0);

	return 0;
}

static int ov9718_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	int errCode = 0;
	u32 tmp_reg, bayer_pattern, readmode = 0;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = OV9718_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = OV9718_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = OV9718_H_MIRROR + OV9718_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov9718_read_reg(vdev, OV9718_IMG_ORI, &tmp_reg);
	tmp_reg &= (~OV9718_MIRROR_MASK);
	tmp_reg |= readmode;
	ov9718_write_reg(vdev, OV9718_IMG_ORI, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}


static struct vin_ops ov9718_ops = {
	.init_device		= ov9718_init_device,
	.set_pll		= ov9718_set_pll,
	.set_format		= ov9718_set_format,
	.set_shutter_row = ov9718_set_shutter_row,
	.set_frame_rate		= ov9718_set_fps,
	.set_agc_index		= ov9718_set_agc_index,
	.set_mirror_mode	= ov9718_set_mirror_mode,
	.shutter2row = ov9718_shutter2row,
	.read_reg		= ov9718_read_reg,
	.write_reg		= ov9718_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov9718_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov9718_priv *ov9718;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV9718, sizeof(struct ov9718_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x24000000;	// 36dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00600000;	// 0.375db
	vdev->pixel_size = 0x00030000;	/* 3.0um */

	i2c_set_clientdata(client, vdev);

	ov9718 = (struct ov9718_priv *)vdev->priv;
	ov9718->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ov9718_ops,
			ov9718_formats, ARRAY_SIZE(ov9718_formats),
			ov9718_plls, ARRAY_SIZE(ov9718_plls));
	if (rval < 0)
		goto ov9718_probe_err;

	vin_info("OV9718 init(2-lane mipi)\n");

	return 0;

ov9718_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov9718_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov9718_idtable[] = {
	{ "ov9718", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9718_idtable);

static struct i2c_driver i2c_driver_ov9718 = {
	.driver = {
		.name	= "ov9718",
	},

	.id_table	= ov9718_idtable,
	.probe		= ov9718_probe,
	.remove		= ov9718_remove,

};

static int __init ov9718_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov9718", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov9718);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov9718_exit(void)
{
	i2c_del_driver(&i2c_driver_ov9718);
}

module_init(ov9718_init);
module_exit(ov9718_exit);

MODULE_DESCRIPTION("OV9718 1/4 -Inch, 1280x800, 1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

