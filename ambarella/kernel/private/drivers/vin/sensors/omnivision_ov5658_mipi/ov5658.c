/*
 * Filename : ov5658.c
 *
 * History:
 *    2014/05/28 - [Long Zhao] Create
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
#include "ov5658.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int lane = 4;
module_param(lane, int, S_IRUGO);

struct ov5658_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov5658_table.c"

static int ov5658_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov5658_priv *ov5658;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov5658 = (struct ov5658_priv *)vdev->priv;
	client = ov5658->control_data;

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

static int ov5658_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov5658_priv *ov5658;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov5658 = (struct ov5658_priv *)vdev->priv;
	client = ov5658->control_data;

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

static int ov5658_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov5658_config;

	memset(&ov5658_config, 0, sizeof (ov5658_config));

	ov5658_config.interface_type = SENSOR_MIPI;
	ov5658_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	if (lane == 2) {
		ov5658_config.mipi_cfg.lane_number = SENSOR_2_LANE;
	} else {
		ov5658_config.mipi_cfg.lane_number = SENSOR_4_LANE;
	}

	ov5658_config.cap_win.x = format->def_start_x;
	ov5658_config.cap_win.y = format->def_start_y;
	ov5658_config.cap_win.width = format->def_width;
	ov5658_config.cap_win.height = format->def_height;

	ov5658_config.sensor_id	= GENERIC_SENSOR;
	ov5658_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov5658_config.bayer_pattern	= format->bayer_pattern;
	ov5658_config.video_format	= format->format;
	ov5658_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov5658_config);
}

static void ov5658_sw_reset(struct vin_device *vdev)
{
	ov5658_write_reg(vdev, OV5658_SWRESET, 0x01);
	ov5658_write_reg(vdev, OV5658_STANDBY, 0x00);/* Standby */
	msleep(10);
}

static void ov5658_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs = NULL;
	int i, regs_num;

	/* fill common registers */
	if (lane == 2) {
		regs = ov5658_share_regs_2lane;
		regs_num = ARRAY_SIZE(ov5658_share_regs_2lane);
	} else if (lane == 4){
		regs = ov5658_share_regs_4lane;
		regs_num = ARRAY_SIZE(ov5658_share_regs_4lane);
	}
	for (i = 0; i < regs_num; i++)
		ov5658_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov5658_init_device(struct vin_device *vdev)
{
	ov5658_sw_reset(vdev);
	ov5658_fill_share_regs(vdev);

	return 0;
}

static int ov5658_set_pll(struct vin_device *vdev, int pll_idx)
{
	return 0;
}

static void ov5658_start_streaming(struct vin_device *vdev)
{
	ov5658_write_reg(vdev, OV5658_STANDBY, 0x01); //streaming
}

static int ov5658_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov5658_priv *pinfo = (struct ov5658_priv *)vdev->priv;

	ov5658_read_reg(vdev, OV5658_HMAX_MSB, &val_high);
	ov5658_read_reg(vdev, OV5658_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov5658_read_reg(vdev, OV5658_VMAX_MSB, &val_high);
	ov5658_read_reg(vdev, OV5658_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int ov5658_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov5658_priv *pinfo = (struct ov5658_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov5658_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = ov5658_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ov5658_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		ov5658_write_reg(vdev, regs[i].addr, regs[i].data);

	/* FIX ME */
	if ( lane == 2 && format->device_mode == 0)// 2 lane, 30fps
		ov5658_write_reg(vdev, 0x3600, 0x66);

	rval = ov5658_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov5658_get_line_time(vdev);

	/* Enable Streaming */
	ov5658_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov5658_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov5658_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct ov5658_priv *pinfo = (struct ov5658_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 3) */
	min_line = 1;
	max_line = pinfo->frame_length_lines  - 3;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov5658_write_reg(vdev, OV5658_LONG_EXPO_H, (num_line >> 16) & 0x0F);
	ov5658_write_reg(vdev, OV5658_LONG_EXPO_M, (num_line >> 8) & 0xFF);
	ov5658_write_reg(vdev, OV5658_LONG_EXPO_L, num_line & 0xFF);

	num_line  >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int ov5658_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov5658_priv *pinfo = (struct ov5658_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov5658_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov5658_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct ov5658_priv *pinfo = (struct ov5658_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ov5658_write_reg(vdev, OV5658_VMAX_MSB, (u8)((v_lines & 0xFF00) >> 8));
	ov5658_write_reg(vdev, OV5658_VMAX_LSB, (u8)(v_lines & 0xFF));

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov5658_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV5658_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV5658_GAIN_0DB);
		agc_idx = OV5658_GAIN_0DB;
	}

	agc_idx = OV5658_GAIN_0DB - agc_idx;

	ov5658_write_reg(vdev, OV5658_GRP_ACCESS, 0x00);
	ov5658_write_reg(vdev, OV5658_AGC_ADJ_H, OV5658_GAIN_TABLE[agc_idx][OV5658_GAIN_COL_REG350A]);
	ov5658_write_reg(vdev, OV5658_AGC_ADJ_L, OV5658_GAIN_TABLE[agc_idx][OV5658_GAIN_COL_REG350B]);
	ov5658_write_reg(vdev, OV5658_GRP_ACCESS, 0x10);
	ov5658_write_reg(vdev, OV5658_GRP_ACCESS, 0xA0);

	return 0;
}

static int ov5658_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	int errCode = 0;
	u32 tmp_reg, bayer_pattern, vflip = 0,hflip = 0;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		vflip = 0x42;/* set bit 2 to zero and bit 7 to 1 if flip */
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		hflip = OV5658_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		vflip = 0x42;/* set bit 2 to zero and bit 7 to 1 if flip */
		hflip = OV5658_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= ov5658_read_reg(vdev,OV5658_V_FORMAT,&tmp_reg);
	tmp_reg &= (~OV5658_V_FLIP);
	tmp_reg |= vflip;
	ov5658_write_reg(vdev, OV5658_V_FORMAT, tmp_reg);

	errCode |= ov5658_read_reg(vdev,OV5658_H_FORMAT,&tmp_reg);
	tmp_reg |= OV5658_H_MIRROR;
	tmp_reg ^= hflip;
	ov5658_write_reg(vdev, OV5658_H_FORMAT, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}

static struct vin_ops ov5658_ops = {
	.init_device		= ov5658_init_device,
	.set_pll		= ov5658_set_pll,
	.set_format		= ov5658_set_format,
	.set_shutter_row = ov5658_set_shutter_row,
	.set_frame_rate		= ov5658_set_fps,
	.set_agc_index		= ov5658_set_agc_index,
	.set_mirror_mode	= ov5658_set_mirror_mode,
	.shutter2row = ov5658_shutter2row,
	.read_reg		= ov5658_read_reg,
	.write_reg		= ov5658_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov5658_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ov5658_priv *ov5658;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV5658, sizeof(struct ov5658_priv));
	if (!vdev)
		return -ENOMEM;

	if (lane !=2 && lane !=4) {
		vin_error("ov5658 can only support 2 or 4 lane!\n");
		goto ov5658_probe_err;
	}

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_QSXGA;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2A000000;	// 42dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x0060C183;	// 0.377953 db

	i2c_set_clientdata(client, vdev);

	ov5658 = (struct ov5658_priv *)vdev->priv;
	ov5658->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ov5658_ops,
			ov5658_formats, ARRAY_SIZE(ov5658_formats),
			ov5658_plls, ARRAY_SIZE(ov5658_plls));
	if (rval < 0)
		goto ov5658_probe_err;

	vin_info("OV5658 init(%d-lane mipi)\n", lane);

	return 0;

ov5658_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov5658_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov5658_idtable[] = {
	{ "ov5658", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov5658_idtable);

static struct i2c_driver i2c_driver_ov5658 = {
	.driver = {
		.name	= "ov5658",
	},

	.id_table	= ov5658_idtable,
	.probe		= ov5658_probe,
	.remove		= ov5658_remove,

};

static int __init ov5658_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov5658", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov5658);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov5658_exit(void)
{
	i2c_del_driver(&i2c_driver_ov5658);
}

module_init(ov5658_init);
module_exit(ov5658_exit);

MODULE_DESCRIPTION("OV5658 1/3.2-Inch 5-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

