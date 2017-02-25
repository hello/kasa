/*
 * Filename : ov2710.c
 *
 * History:
 *    2009/06/19 - [Qiao Wang] Create
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
#include "ov2710.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct ov2710_priv {
	struct i2c_client *client;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov2710_table.c"

static int ov2710_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval = 0;
	struct ov2710_priv *ov2710;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov2710 = (struct ov2710_priv *)vdev->priv;
	client = ov2710->client;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data & 0xff;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 3;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}

	return 0;
}

static int ov2710_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov2710_priv *ov2710;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[1];
	u8 pbuf0[2];


	ov2710 = (struct ov2710_priv *)vdev->priv;
	client = ov2710->client;

	pbuf0[1] = subaddr & 0xff;
	pbuf0[0] = (subaddr & 0xff00) >> 8;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 2;
	msgs[0].buf = pbuf0;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags | I2C_M_RD;
	msgs[0].len = 1;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int ov2710_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov2710_config;

	memset(&ov2710_config, 0, sizeof (ov2710_config));

	ov2710_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ov2710_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	ov2710_config.input_mode = SENSOR_RGB_1PIX;

	ov2710_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_LOW | SENSOR_HS_HIGH;
	ov2710_config.plvcmos_cfg.data_edge = SENSOR_DATA_FALLING_EDGE;
	ov2710_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	ov2710_config.cap_win.x = format->def_start_x;
	ov2710_config.cap_win.y = format->def_start_y;
	ov2710_config.cap_win.width = format->def_width;
	ov2710_config.cap_win.height = format->def_height;

	ov2710_config.sensor_id = GENERIC_SENSOR;
	ov2710_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov2710_config.bayer_pattern = format->bayer_pattern;
	ov2710_config.video_format = format->format;
	ov2710_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &ov2710_config);
}

static void ov2710_set_streaming(struct vin_device *vdev)
{
	u32 val;

	ov2710_read_reg(vdev, OV2710_SYSTEM_CONTROL00, &val);
	val &= 0xbf;
	ov2710_write_reg(vdev, OV2710_SYSTEM_CONTROL00, val);
}

static void ov2710_sw_reset(struct vin_device *vdev)
{
	u32 val;

	ov2710_read_reg(vdev, OV2710_SYSTEM_CONTROL00, &val);
	val |= 0x80;
	ov2710_write_reg(vdev, OV2710_SYSTEM_CONTROL00, val);
}

static int ov2710_init_device(struct vin_device *vdev)
{
	int sensor_id, pidh, pidl;

	ov2710_sw_reset(vdev);

	/* query sensor id */
	ov2710_read_reg(vdev, OV2710_PIDH, &pidh);
	if (pidh < 0)
		return -EIO;

	ov2710_read_reg(vdev, OV2710_PIDL, &pidl);
	if (pidl < 0)
		return -EIO;

	sensor_id = (pidh << 8)| pidl;

	vin_info("OV2710 sensor ID is 0x%x\n", sensor_id);

	return 0;
}

static int ov2710_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov2710_priv *pinfo = (struct ov2710_priv *)vdev->priv;

	ov2710_read_reg(vdev, OV2710_TIMING_CONTROL_HTS_HIGHBYTE, &val_high);
	ov2710_read_reg(vdev, OV2710_TIMING_CONTROL_HTS_LOWBYTE, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov2710_read_reg(vdev, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, &val_high);
	ov2710_read_reg(vdev, OV2710_TIMING_CONTROL_VTS_LOWBYTE, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int ov2710_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov2710_priv *pinfo = (struct ov2710_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov2710_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	if (format->device_mode == 0) { /* 720p */
		regs = ov2710_720p_share_regs;
		regs_num = ARRAY_SIZE(ov2710_720p_share_regs);

		for (i = 0; i < regs_num; i++)
			ov2710_write_reg(vdev, regs[i].addr, regs[i].data);
	} else if (format->device_mode == 1) { /* 1080p */
		regs = ov2710_1080p_share_regs;
		regs_num = ARRAY_SIZE(ov2710_1080p_share_regs);

		for (i = 0; i < regs_num; i++)
			ov2710_write_reg(vdev, regs[i].addr, regs[i].data);
	}

	rval = ov2710_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov2710_get_line_time(vdev);

	/* start streaming */
	ov2710_set_streaming(vdev);

	/* communiate with IAV */
	rval = ov2710_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov2710_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct ov2710_priv *pinfo = (struct ov2710_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 3) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 3;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov2710_write_reg(vdev, OV2710_AEC_PK_EXPO_H, (num_line >> 16) & 0x0F);
	ov2710_write_reg(vdev, OV2710_AEC_PK_EXPO_M, (num_line >> 8) & 0xFF);
	ov2710_write_reg(vdev, OV2710_AEC_PK_EXPO_L, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int ov2710_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov2710_priv *pinfo = (struct ov2710_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov2710_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov2710_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct ov2710_priv *pinfo = (struct ov2710_priv *)vdev->priv;

	/* calculate line number per frame */
	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ov2710_write_reg(vdev, OV2710_TIMING_CONTROL_VTS_HIGHBYTE, (v_lines >> 8) & 0xFF);
	ov2710_write_reg(vdev, OV2710_TIMING_CONTROL_VTS_LOWBYTE, v_lines & 0xFF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov2710_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV2710_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV2710_GAIN_0DB);
		agc_idx = OV2710_GAIN_0DB;
	}

	agc_idx = OV2710_GAIN_0DB - agc_idx;

	ov2710_write_reg(vdev, OV2710_GROUP_ACCESS, 0x00);

	ov2710_write_reg(vdev, OV2710_AEC_AGC_ADJ_H,
		ov2710_gains[agc_idx][OV2710_GAIN_COL_REG300A]);
	ov2710_write_reg(vdev, OV2710_AEC_AGC_ADJ_L,
		ov2710_gains[agc_idx][OV2710_GAIN_COL_REG300B]);

	ov2710_write_reg(vdev, OV2710_GROUP_ACCESS, 0x10);
	ov2710_write_reg(vdev, OV2710_GROUP_ACCESS, 0xA0);

	return 0;
}

static int ov2710_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 readmode, tmp_reg, ana_reg, vstart_reg;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = OV2710_MIRROR_ROW + OV2710_MIRROR_COLUMN;
		ana_reg = 0x14;
		vstart_reg = 0x09;
		break;
	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = OV2710_MIRROR_ROW;
		ana_reg = 0x14;
		vstart_reg = 0x0a;
		break;
	case VINDEV_MIRROR_VERTICALLY:
		readmode = OV2710_MIRROR_COLUMN;
		ana_reg = 0x04;
		vstart_reg = 0x09;
		break;
	case VINDEV_MIRROR_NONE:
		readmode = 0;
		ana_reg = 0x04;
		vstart_reg = 0x0a;
		break;
	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov2710_read_reg(vdev, OV2710_TIMING_CONTROL18, &tmp_reg);
	tmp_reg &= (~OV2710_MIRROR_MASK);
	tmp_reg |= readmode;

	ov2710_write_reg(vdev, OV2710_TIMING_CONTROL18, tmp_reg);
	ov2710_write_reg(vdev, OV2710_ANA_ARRAY_01, ana_reg);
	ov2710_write_reg(vdev, OV2710_TIMING_CONTROL_VS_LOWBYTE, vstart_reg);

	return 0;
}

static struct vin_ops ov2710_ops = {
	.init_device		= ov2710_init_device,
	.set_format		= ov2710_set_format,
	.set_shutter_row	= ov2710_set_shutter_row,
	.shutter2row		= ov2710_shutter2row,
	.set_frame_rate		= ov2710_set_fps,
	.set_agc_index		= ov2710_set_agc_index,
	.set_mirror_mode	= ov2710_set_mirror_mode,
	.read_reg		= ov2710_read_reg,
	.write_reg		= ov2710_write_reg,
};

/* 	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov2710_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct vin_device *vdev;
	struct ov2710_priv *ov2710;
	int i, rval;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV2710, 	sizeof(struct ov2710_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x24000000;	// 36dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00600000;	// 0.375dB

	/* I2c Client */
	i2c_set_clientdata(client, vdev);

	ov2710 = (struct ov2710_priv *)vdev->priv;
	ov2710->client = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(ov2710_formats); i++)
				ov2710_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &ov2710_ops,
			ov2710_formats, ARRAY_SIZE(ov2710_formats),
			ov2710_plls, ARRAY_SIZE(ov2710_plls));
	if (rval < 0)
		goto ov2710_probe_err;

	return 0;

ov2710_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov2710_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct i2c_device_id ov2710_idtable[] = {
	{ "ov2710", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2710_idtable);

static struct i2c_driver i2c_driver_ov2710 = {
	.driver = {
		   .name = "ov2710",
		   },
	.id_table	= ov2710_idtable,
	.probe		= ov2710_probe,
	.remove		= ov2710_remove,
};

static int __init ov2710_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov2710", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov2710);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov2710_exit(void)
{
	i2c_del_driver(&i2c_driver_ov2710);
}

module_init(ov2710_init);
module_exit(ov2710_exit);

MODULE_DESCRIPTION("OV2710 1/3-Inch 2-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Qiao Wang, <qwang@ambarella.com>");
MODULE_LICENSE("Proprietary");

