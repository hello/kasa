/*
 * Filename : ov5653.c
 *
 * History:
 *    2014/08/18 - [Hao Zeng] Create
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
#include "ov5653.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct ov5653_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov5653_table.c"

static int ov5653_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval = 0;
	struct ov5653_priv *ov5653;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov5653 = (struct ov5653_priv *)vdev->priv;
	client = ov5653->control_data;

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

static int ov5653_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov5653_priv *ov5653;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov5653 = (struct ov5653_priv *)vdev->priv;
	client = ov5653->control_data;

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
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int ov5653_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov5653_config;

	memset(&ov5653_config, 0, sizeof (ov5653_config));

	ov5653_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ov5653_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	ov5653_config.input_mode = SENSOR_RGB_1PIX;

	ov5653_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	ov5653_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;
	ov5653_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	ov5653_config.cap_win.x = format->def_start_x;
	ov5653_config.cap_win.y = format->def_start_y;
	ov5653_config.cap_win.width = format->def_width;
	ov5653_config.cap_win.height = format->def_height;

	ov5653_config.sensor_id = GENERIC_SENSOR;
	ov5653_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov5653_config.bayer_pattern = format->bayer_pattern;
	ov5653_config.video_format = format->format;
	ov5653_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &ov5653_config);
}

static void ov5653_start_streaming(struct vin_device *vdev)
{
	ov5653_write_reg(vdev, OV5653_SYSTEM_CTRL0, 0x02);
}

static void ov5653_sw_reset(struct vin_device *vdev)
{
	u8 reset_value = 0xFF;
	ov5653_write_reg(vdev, OV5653_SYSTEM_RESET00, reset_value);
	ov5653_write_reg(vdev, OV5653_SYSTEM_RESET01, reset_value);
	ov5653_write_reg(vdev, OV5653_SYSTEM_RESET02, reset_value);
	ov5653_write_reg(vdev, OV5653_SYSTEM_RESET03, reset_value);
	msleep(10);
}

static void ov5653_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ov5653_share_regs;
	regs_num = ARRAY_SIZE(ov5653_share_regs);

	for (i = 0; i < regs_num; i++)
		ov5653_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov5653_init_device(struct vin_device *vdev)
{
	u32 sensor_id, pidh, pidl;

	ov5653_sw_reset(vdev);

	/* query sensor id */
	ov5653_read_reg(vdev, OV5653_CHIP_ID_H, &pidh);
	if (pidh < 0)
		return -EIO;

	ov5653_read_reg(vdev, OV5653_CHIP_ID_L, &pidl);
	if (pidl < 0)
		return -EIO;

	sensor_id = (pidh << 8)| pidl;

	vin_info("OV5653 sensor ID is 0x%x\n", sensor_id);

	ov5653_fill_share_regs(vdev);

	return 0;
}

static int ov5653_set_pll(struct vin_device *vdev, int pll_idx)
{
	return 0;
}

static int ov5653_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov5653_priv *pinfo = (struct ov5653_priv *)vdev->priv;

	ov5653_read_reg(vdev, OV5653_TIMING_HTS_H, &val_high);
	ov5653_read_reg(vdev, OV5653_TIMING_HTS_L, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov5653_read_reg(vdev, OV5653_TIMING_VTS_H, &val_high);
	ov5653_read_reg(vdev, OV5653_TIMING_VTS_L, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int ov5653_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov5653_priv *pinfo = (struct ov5653_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov5653_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = ov5653_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ov5653_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		ov5653_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ov5653_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov5653_get_line_time(vdev);

	/* Enable Streaming */
	ov5653_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov5653_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov5653_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct ov5653_priv *pinfo = (struct ov5653_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 0 ~ (Frame format(V) - 3) */
	min_line = 0;
	max_line = pinfo->frame_length_lines - 3;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4;
	ov5653_write_reg(vdev, OV5653_LONG_EXPO_H, (num_line >> 16) & 0x0F);
	ov5653_write_reg(vdev, OV5653_LONG_EXPO_M, (num_line >> 8) & 0xFF);
	ov5653_write_reg(vdev, OV5653_LONG_EXPO_L, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int ov5653_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov5653_priv *pinfo = (struct ov5653_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov5653_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov5653_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct ov5653_priv *pinfo = (struct ov5653_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ov5653_write_reg(vdev,  OV5653_TIMING_VTS_H, (v_lines >> 8) & 0xFF);
	ov5653_write_reg(vdev,  OV5653_TIMING_VTS_L, (v_lines >> 0) & 0xFF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov5653_agc_parse_virtual_index(struct vin_device *vdev, u32 virtual_index, u32 *agc_index, u32 *summing_gain)
{
	if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P) {
		u32 index_0dB = OV5653_GAIN_0DB;
		u32 index_6dB = OV5653_GAIN_0DB - OV5653_GAIN_DOUBLE * 1;
		u32 index_12dB = OV5653_GAIN_0DB - OV5653_GAIN_DOUBLE * 2;

		if((index_0dB >= virtual_index) && (virtual_index > index_6dB)) { /* 0dB <= gain < 6dB */
			*agc_index = virtual_index;
			*summing_gain = 1;
		} else if((index_6dB >= virtual_index) && (virtual_index > index_12dB)) { /* 6dB <= gain < 12dB */
			*agc_index = virtual_index + OV5653_GAIN_DOUBLE * 1;	/* AGC(dB) = Virtual(dB) - 6dB */
			*summing_gain = 2;
		} else { /* 12 dB <= gain */
			*agc_index = virtual_index + OV5653_GAIN_DOUBLE * 2;	/* AGC(dB) = Virtual(dB) - 12dB */
			*summing_gain = 4;
		}
	} else {
		*agc_index = virtual_index;
		*summing_gain = 0;
	}

	vin_debug("ov5653_agc_index: %d, virtual_index: %d, summing_gain: %d\n", *agc_index, *summing_gain);

	return 0;
}

static void ov5653_set_binning_summing(struct vin_device *vdev, u32 bin_sum_config) /* toggle between 1x, 2x, 4x sum */
{
	u32 reg_0x3613;
	u32 reg_0x3621;

	ov5653_read_reg(vdev, 0x3613, &reg_0x3613);
	ov5653_read_reg(vdev, 0x3621, &reg_0x3621);

	switch(bin_sum_config) {
	case 0:	/* do nothing */
		break;

	case 1:
		reg_0x3621 |= 0x40;  /* Reg0x3621[6]=1 for H-binning off: 1x <H-skip> */
		reg_0x3613 = 0x44;   /* 1x-gain */
		break;

	case 2:
		reg_0x3621 &= ~0x40; /* Reg0x3621[6]=0 for H-binning sum on: 2x <H-bin sum> */
		reg_0x3613 = 0x44;   /* 1x-gain */
		break;

	case 4:
		reg_0x3621 &= ~0x40; /* Reg0x3621[6]=0 for H-binning sum on: 2x <H-bin sum> */
		reg_0x3613 = 0xC4;   /* 2x-gain */
		break;

	default:
		break;
	}

	ov5653_write_reg(vdev, 0x3613, reg_0x3613);
	ov5653_write_reg(vdev, 0x3621, reg_0x3621);
}

static int ov5653_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	u32 virtual_gain_index, agc_index = 0, summing_gain = 0;

	virtual_gain_index = agc_idx;

	if (virtual_gain_index > OV5653_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV5653_GAIN_0DB);
		virtual_gain_index = OV5653_GAIN_0DB;
	}

	virtual_gain_index = OV5653_GAIN_0DB - virtual_gain_index;

	ov5653_write_reg(vdev, OV5653_SRM_GRUP_ACCESS, 0x00);
	ov5653_agc_parse_virtual_index(vdev, virtual_gain_index, &agc_index, &summing_gain);
	ov5653_set_binning_summing(vdev, summing_gain);
	ov5653_write_reg(vdev, OV5653_AGC_ADJ_H, ov5653_gains[agc_index][OV5653_GAIN_COL_REG350A]);
	ov5653_write_reg(vdev, OV5653_AGC_ADJ_L, ov5653_gains[agc_index][OV5653_GAIN_COL_REG350B]);
	ov5653_write_reg(vdev, OV5653_SRM_GRUP_ACCESS, 0x10);
	ov5653_write_reg(vdev, OV5653_SRM_GRUP_ACCESS, 0xA0);

	return 0;
}

static int ov5653_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 bayer_pattern;
	u32 reg3621, reg505a, reg505b, reg3827, reg3818;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P) {
			reg3621 = 0xbf;
			reg3818 = 0xa1;
		} else {
			reg3621 = 0x3f;
			reg3818 = 0xa0;
		}
		reg505a = 0x00;
		reg505b = 0x12;
		reg3827 = 0x0b;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P) {
			reg3621 = 0xbf;
			reg3818 = 0x81;
		} else {
			reg3621 = 0x3f;
			reg3818 = 0x80;
		}
		reg505a = 0x00;
		reg505b = 0x12;
		reg3827 = 0x0c;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P) {
			reg3621 = 0xaf;
			reg3818 = 0xe1;
		} else {
			reg3621 = 0x2f;
			reg3818 = 0xe0;
		}
		reg505a = 0x0a;
		reg505b = 0x2e;
		reg3827 = 0x0b;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_NONE:
		if (vdev->cur_format->video_mode == AMBA_VIDEO_MODE_720P) {
			reg3621 = 0xaf;
			reg3818 = 0xc1;
		} else {
			reg3621 = 0x2f;
			reg3818 = 0xc0;
		}
		reg505a = 0x0a;
		reg505b = 0x2e;
		reg3827 = 0x0c;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov5653_write_reg(vdev, OV5653_ARRAY_CONTROL, reg3621);
	ov5653_write_reg(vdev, 0x505a, reg505a);
	ov5653_write_reg(vdev, 0x505b, reg505b);
	ov5653_write_reg(vdev, 0x3827, reg3827);
	ov5653_write_reg(vdev, OV5653_TIMING_TC_REG_18, reg3818);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops ov5653_ops = {
	.init_device		= ov5653_init_device,
	.set_pll			= ov5653_set_pll,
	.set_format		= ov5653_set_format,
	.set_shutter_row	= ov5653_set_shutter_row,
	.shutter2row		= ov5653_shutter2row,
	.set_frame_rate	= ov5653_set_fps,
	.set_agc_index		= ov5653_set_agc_index,
	.set_mirror_mode	= ov5653_set_mirror_mode,
	.read_reg			= ov5653_read_reg,
	.write_reg			= ov5653_write_reg,
};

/* 	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov5653_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	int i;
	struct vin_device *vdev;
	struct ov5653_priv *ov5653;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV5653, 	sizeof(struct ov5653_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x24000000;	/* 36dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00600000;	/* 0.375dB */

	/* I2c Client */
	i2c_set_clientdata(client, vdev);

	ov5653 = (struct ov5653_priv *)vdev->priv;
	ov5653->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(ov5653_formats); i++)
				ov5653_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &ov5653_ops,
			ov5653_formats, ARRAY_SIZE(ov5653_formats),
			ov5653_plls, ARRAY_SIZE(ov5653_plls));

	if (rval < 0)
		goto ov5653_probe_err;

	vin_info("OV5653 init(parallel)\n");

	return 0;

ov5653_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov5653_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct i2c_device_id ov5653_idtable[] = {
	{ "ov5653", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov5653_idtable);

static struct i2c_driver i2c_driver_ov5653 = {
	.driver = {
		   .name = "ov5653",
		   },
	.id_table	= ov5653_idtable,
	.probe		= ov5653_probe,
	.remove		= ov5653_remove,
};

static int __init ov5653_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov5653", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov5653);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov5653_exit(void)
{
	i2c_del_driver(&i2c_driver_ov5653);
}

module_init(ov5653_init);
module_exit(ov5653_exit);

MODULE_DESCRIPTION("OV5653 1/3.2-Inch 5-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

