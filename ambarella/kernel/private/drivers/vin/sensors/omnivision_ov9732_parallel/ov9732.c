/*
 * Filename : ov9732.c
 *
 * History:
 *    2015/07/28 - [Hao Zeng] Create
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
#include "ov9732.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct ov9732_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov9732_table.c"

static int ov9732_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov9732_priv *ov9732;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov9732 = (struct ov9732_priv *)vdev->priv;
	client = ov9732->control_data;

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

static int ov9732_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov9732_priv *ov9732;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov9732 = (struct ov9732_priv *)vdev->priv;
	client = ov9732->control_data;

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

static int ov9732_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov9732_config;

	memset(&ov9732_config, 0, sizeof (ov9732_config));

	ov9732_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ov9732_config.input_mode = SENSOR_RGB_1PIX;

	ov9732_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_LOW | SENSOR_HS_HIGH;
	ov9732_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;
	ov9732_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	ov9732_config.cap_win.x = format->def_start_x;
	ov9732_config.cap_win.y = format->def_start_y;
	ov9732_config.cap_win.width = format->def_width;
	ov9732_config.cap_win.height = format->def_height;

	ov9732_config.sensor_id	= GENERIC_SENSOR;
	ov9732_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov9732_config.bayer_pattern	= format->bayer_pattern;
	ov9732_config.video_format	= format->format;
	ov9732_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov9732_config);
}

static void ov9732_sw_reset(struct vin_device *vdev)
{
	ov9732_write_reg(vdev, OV9732_SWRESET, 0x01);/* Software reset */
	ov9732_write_reg(vdev, OV9732_STANDBY, 0x00);/* Standby */
}

static void ov9732_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ov9732_share_regs;
	regs_num = ARRAY_SIZE(ov9732_share_regs);
	for (i = 0; i < regs_num; i++)
		ov9732_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov9732_init_device(struct vin_device *vdev)
{
	ov9732_sw_reset(vdev);
	return 0;
}

static int ov9732_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov9732_priv *pinfo = (struct ov9732_priv *)vdev->priv;

	ov9732_read_reg(vdev, OV9732_HTS_MSB, &val_high);
	ov9732_read_reg(vdev, OV9732_HTS_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov9732_read_reg(vdev, OV9732_VTS_MSB, &val_high);
	ov9732_read_reg(vdev, OV9732_VTS_LSB, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int ov9732_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov9732_priv *pinfo = (struct ov9732_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static void ov9732_start_streaming(struct vin_device *vdev)
{
	ov9732_write_reg(vdev, OV9732_STANDBY, 0x01); /* start streaming */
}

static int ov9732_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;

	ov9732_fill_share_regs(vdev);

	rval = ov9732_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov9732_get_line_time(vdev);

	/* Enable Streaming */
	ov9732_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov9732_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov9732_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;
	struct ov9732_priv *pinfo = (struct ov9732_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 4) */
	min_line = 4;
	max_line = pinfo->frame_length_lines - 4;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov9732_write_reg(vdev, OV9732_L_EXPO_HSB, (num_line >> 16) & 0x0F);
	ov9732_write_reg(vdev, OV9732_L_EXPO_MSB, (num_line >> 8) & 0xFF);
	ov9732_write_reg(vdev, OV9732_L_EXPO_LSB, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ov9732_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov9732_priv *pinfo = (struct ov9732_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov9732_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov9732_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;

	struct ov9732_priv *pinfo = (struct ov9732_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ov9732_write_reg(vdev, OV9732_VTS_MSB, (u8)((v_lines >> 8) & 0xFF));
	ov9732_write_reg(vdev, OV9732_VTS_LSB, (u8)(v_lines & 0xFF));

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov9732_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV9732_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV9732_GAIN_MAXDB);
		agc_idx = OV9732_GAIN_MAXDB;
	}

	/* Analog Gain */
	ov9732_write_reg(vdev, OV9732_GAIN_LSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_AGAIN_LSB]);

	/* Digital Gain */
	/* WB-R */
	ov9732_write_reg(vdev, OV9732_R_GAIN_MSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_MSB]);
	ov9732_write_reg(vdev, OV9732_R_GAIN_LSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_LSB]);
	/* WB-G */
	ov9732_write_reg(vdev, OV9732_G_GAIN_MSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_MSB]);
	ov9732_write_reg(vdev, OV9732_G_GAIN_LSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_LSB]);
	/* WB-B */
	ov9732_write_reg(vdev, OV9732_B_GAIN_MSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_MSB]);
	ov9732_write_reg(vdev, OV9732_B_GAIN_LSB, OV9732_GAIN_TABLE[agc_idx][OV9732_GAIN_COL_DGAIN_LSB]);

	return 0;
}

static int ov9732_set_mirror_mode(struct vin_device *vdev,
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
		readmode = OV9732_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = OV9732_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = OV9732_V_FLIP + OV9732_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov9732_read_reg(vdev, OV9732_FORMAT0, &tmp_reg);
	tmp_reg &= (~OV9732_MIRROR_MASK);
	tmp_reg |= readmode;
	ov9732_write_reg(vdev, OV9732_FORMAT0, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}

static struct vin_ops ov9732_ops = {
	.init_device		= ov9732_init_device,
	.set_format		= ov9732_set_format,
	.set_shutter_row	= ov9732_set_shutter_row,
	.shutter2row		= ov9732_shutter2row,
	.set_frame_rate	= ov9732_set_fps,
	.set_agc_index		= ov9732_set_agc_index,
	.set_mirror_mode	= ov9732_set_mirror_mode,
	.shutter2row		= ov9732_shutter2row,
	.read_reg		= ov9732_read_reg,
	.write_reg		= ov9732_write_reg,
};

/* ========================================================================== */
static int ov9732_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int i, rval = 0;
	struct vin_device *vdev;
	struct ov9732_priv *ov9732;
	u32 cid_l, cid_h, chip_rev;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV9732, sizeof(struct ov9732_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x23D00000;	/* 35.81250dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */

	i2c_set_clientdata(client, vdev);

	ov9732 = (struct ov9732_priv *)vdev->priv;
	ov9732->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(ov9732_formats); i++)
				ov9732_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &ov9732_ops,
			ov9732_formats, ARRAY_SIZE(ov9732_formats),
			ov9732_plls, ARRAY_SIZE(ov9732_plls));
	if (rval < 0)
		goto ov9732_probe_err;

	/* query sensor id and revision */
	ov9732_read_reg(vdev, OV9732_CHIP_ID_H, &cid_h);
	ov9732_read_reg(vdev, OV9732_CHIP_ID_L, &cid_l);
	ov9732_read_reg(vdev, OV9732_CHIP_REV, &chip_rev);

	vin_info("OV9732 init(parallel)\n");
	vin_info("Chip ID:0x%x, Chip Revision:0x%x\n", (cid_h<<8)+cid_l, chip_rev);

	return 0;

ov9732_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov9732_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov9732_idtable[] = {
	{ "ov9732", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9732_idtable);

static struct i2c_driver i2c_driver_ov9732 = {
	.driver = {
		.name	= "ov9732",
	},

	.id_table	= ov9732_idtable,
	.probe		= ov9732_probe,
	.remove		= ov9732_remove,

};

static int __init ov9732_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov9732", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov9732);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov9732_exit(void)
{
	i2c_del_driver(&i2c_driver_ov9732);
}

module_init(ov9732_init);
module_exit(ov9732_exit);

MODULE_DESCRIPTION("OV9732 1/4 -Inch, 1288x728, 0.95-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

