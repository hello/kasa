/*
 * Filename : ov9750.c
 *
 * History:
 *    2014/12/04 - [Hao Zeng] Create
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
#include "ov9750.h"

static int bus_addr = (0 << 16) | (0x6C >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static bool ir_mode = 0;
module_param(ir_mode, bool, 0644);
MODULE_PARM_DESC(ir_mode, " Use IR mode, 0:RGB mode, 1:RGB/IR mode");

struct ov9750_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov9750_table.c"

static int ov9750_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov9750_priv *ov9750;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ov9750 = (struct ov9750_priv *)vdev->priv;
	client = ov9750->control_data;

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

static int ov9750_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov9750_priv *ov9750;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	ov9750 = (struct ov9750_priv *)vdev->priv;
	client = ov9750->control_data;

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

static int ov9750_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov9750_config;

	memset(&ov9750_config, 0, sizeof (ov9750_config));

	ov9750_config.interface_type = SENSOR_MIPI;
	ov9750_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ov9750_config.mipi_cfg.lane_number = SENSOR_2_LANE;

	ov9750_config.cap_win.x = format->def_start_x;
	ov9750_config.cap_win.y = format->def_start_y;
	ov9750_config.cap_win.width = format->def_width;
	ov9750_config.cap_win.height = format->def_height;

	ov9750_config.sensor_id	= GENERIC_SENSOR;
	ov9750_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov9750_config.bayer_pattern	= format->bayer_pattern;
	ov9750_config.video_format	= format->format;
	ov9750_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov9750_config);
}

static void ov9750_sw_reset(struct vin_device *vdev)
{
	ov9750_write_reg(vdev, OV9750_SWRESET, 0x01);/* Software reset */
	ov9750_write_reg(vdev, OV9750_STANDBY, 0x00);/* Standby */
}

static void ov9750_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ov9750_share_regs;
	regs_num = ARRAY_SIZE(ov9750_share_regs);
	for (i = 0; i < regs_num; i++)
		ov9750_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov9750_init_device(struct vin_device *vdev)
{
	ov9750_sw_reset(vdev);
	return 0;
}

static int ov9750_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct ov9750_priv *pinfo = (struct ov9750_priv *)vdev->priv;

	ov9750_read_reg(vdev, OV9750_HMAX_MSB, &val_high);
	ov9750_read_reg(vdev, OV9750_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov9750_read_reg(vdev, OV9750_VMAX_MSB, &val_high);
	ov9750_read_reg(vdev, OV9750_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int ov9750_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov9750_priv *pinfo = (struct ov9750_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static void ov9750_start_streaming(struct vin_device *vdev)
{
	ov9750_write_reg(vdev, OV9750_STANDBY, 0x01); /* start streaming */
}

static int ov9750_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	ov9750_fill_share_regs(vdev);

	regs = ov9750_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ov9750_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		ov9750_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ov9750_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov9750_get_line_time(vdev);

	/* Enable Streaming */
	ov9750_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov9750_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov9750_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	if (hold_mode)
		ov9750_write_reg(vdev, OV9750_GRP_ACCESS, 0x00);
	else {
		ov9750_write_reg(vdev, OV9750_GRP_ACCESS, 0x10);
		ov9750_write_reg(vdev, OV9750_GRP_ACCESS, 0xA0);
	}

	return 0;
}

static int ov9750_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;
	struct ov9750_priv *pinfo = (struct ov9750_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 0 ~(Frame format(V) - 3) */
	min_line = 0;
	max_line = pinfo->frame_length_lines - 3;
	num_line = clamp(num_line, min_line, max_line);

	num_line <<= 4; /* the register value should be exposure time * 16 */
	ov9750_write_reg(vdev, OV9750_EXPO0_HSB, (num_line >> 16) & 0x0F);
	ov9750_write_reg(vdev, OV9750_EXPO0_MSB, (num_line >> 8) & 0xFF);
	ov9750_write_reg(vdev, OV9750_EXPO0_LSB, num_line & 0xFF);

	num_line >>= 4;
	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ov9750_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov9750_priv *pinfo = (struct ov9750_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov9750_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov9750_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;

	struct ov9750_priv *pinfo = (struct ov9750_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ov9750_write_reg(vdev, OV9750_VMAX_MSB, (u8)((v_lines >> 8) & 0xFF));
	ov9750_write_reg(vdev, OV9750_VMAX_LSB, (u8)(v_lines & 0xFF));

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov9750_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	u32 tmp_reg0, tmp_reg1, tmp_reg2;

	if (agc_idx > OV9750_GAIN_MAXDB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV9750_GAIN_MAXDB);
		agc_idx = OV9750_GAIN_MAXDB;
	}

	ov9750_read_reg(vdev, OV9750_GAIN_DCG, &tmp_reg0);
	ov9750_read_reg(vdev, OV9750_R_GAIN_MSB, &tmp_reg1);
	ov9750_read_reg(vdev, OV9750_R_GAIN_LSB, &tmp_reg2);

	/* Analog Gain */
	ov9750_write_reg(vdev, OV9750_GAIN_MSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_AGAIN_MSB]);
	ov9750_write_reg(vdev, OV9750_GAIN_LSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_AGAIN_LSB]);

	ov9750_write_reg(vdev, 0x366A, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_REG_366A]);

	/* Dual Conversion Gain */
	ov9750_write_reg(vdev, OV9750_GAIN_DCG, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DCG]);

	/* Digital Gain */
	/* WB-R */
	ov9750_write_reg(vdev, OV9750_R_GAIN_MSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_MSB]);
	ov9750_write_reg(vdev, OV9750_R_GAIN_LSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_LSB]);
	/* WB-G */
	ov9750_write_reg(vdev, OV9750_G_GAIN_MSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_MSB]);
	ov9750_write_reg(vdev, OV9750_G_GAIN_LSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_LSB]);
	/* WB-B */
	ov9750_write_reg(vdev, OV9750_B_GAIN_MSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_MSB]);
	ov9750_write_reg(vdev, OV9750_B_GAIN_LSB, OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_LSB]);

	/* only change analog gain will trigger BLC
	 * when modify conversion gain or digital gain, it need manually BLC trigger
	 * by set register 0x4010 to 0x44 then 0x40.
	 */
	if (tmp_reg0 != OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DCG] ||
		tmp_reg1 != OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_MSB] ||
		tmp_reg2 != OV9750_GAIN_TABLE[agc_idx][OV9750_GAIN_COL_DGAIN_LSB]) {
		ov9750_write_reg(vdev, OV9750_BLC_CTRL10, 0x44);
		ov9750_write_reg(vdev, OV9750_BLC_CTRL10, 0x40);
	}

	return 0;
}

static int ov9750_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	int errCode = 0;
	u32 tmp_reg, bayer_pattern, vflip = 0, hflip = 0, r450b = 0x00;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		if (ir_mode)
			bayer_pattern = VINDEV_BAYER_PATTERN_BG_GI;
		else
			bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		r450b = 0x00;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		vflip = OV9750_V_FLIP;
		if (ir_mode)
			bayer_pattern = VINDEV_BAYER_PATTERN_BG_GI;
		else
			bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		r450b = 0x20;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		hflip = OV9750_H_MIRROR;
		if (ir_mode)
			bayer_pattern = VINDEV_BAYER_PATTERN_BG_GI;
		else
			bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		r450b = 0x00;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		vflip = OV9750_V_FLIP;
		hflip = OV9750_H_MIRROR;
		if (ir_mode)
			bayer_pattern = VINDEV_BAYER_PATTERN_BG_GI;
		else
			bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		r450b = 0x20;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov9750_read_reg(vdev, OV9750_V_FORMAT, &tmp_reg);
	tmp_reg &= (~OV9750_V_FLIP);
	tmp_reg |= vflip;
	ov9750_write_reg(vdev, OV9750_V_FORMAT, tmp_reg);

	ov9750_read_reg(vdev, OV9750_H_FORMAT, &tmp_reg);
	tmp_reg &= (~OV9750_H_MIRROR);
	tmp_reg |= hflip;
	ov9750_write_reg(vdev, OV9750_H_FORMAT, tmp_reg);

	ov9750_write_reg(vdev, 0x450b, r450b);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return errCode;
}

#ifdef CONFIG_PM
static int ov9750_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ov9750_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int ov9750_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		ov9750_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops ov9750_ops = {
	.init_device		= ov9750_init_device,
	.set_format		= ov9750_set_format,
	.set_shutter_row	= ov9750_set_shutter_row,
	.shutter2row		= ov9750_shutter2row,
	.set_frame_rate	= ov9750_set_fps,
	.set_agc_index		= ov9750_set_agc_index,
	.set_mirror_mode	= ov9750_set_mirror_mode,
	.set_hold_mode		= ov9750_set_hold_mode,
	.shutter2row		= ov9750_shutter2row,
	.read_reg		= ov9750_read_reg,
	.write_reg		= ov9750_write_reg,
#ifdef CONFIG_PM
	.suspend 			= ov9750_suspend,
	.resume 			= ov9750_resume,
#endif
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov9750_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int i, rval = 0;
	struct vin_device *vdev;
	struct ov9750_priv *ov9750;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV9750, sizeof(struct ov9750_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1280_960;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2B080000;	/* 43.031250dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00180000;	/* 0.09375dB */

	i2c_set_clientdata(client, vdev);

	ov9750 = (struct ov9750_priv *)vdev->priv;
	ov9750->control_data = client;

	if (ir_mode) {
		for (i = 0; i < ARRAY_SIZE(ov9750_formats); i++)
			ov9750_formats[i].default_bayer_pattern = VINDEV_BAYER_PATTERN_BG_GI;
	}

	rval = ambarella_vin_register_device(vdev, &ov9750_ops,
			ov9750_formats, ARRAY_SIZE(ov9750_formats),
			ov9750_plls, ARRAY_SIZE(ov9750_plls));
	if (rval < 0)
		goto ov9750_probe_err;

	if (ir_mode)
		vin_info("OV9756 init(2-lane mipi)\n");
	else
		vin_info("OV9750 init(2-lane mipi)\n");

	return 0;

ov9750_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov9750_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov9750_idtable[] = {
	{ "ov9750", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9750_idtable);

static struct i2c_driver i2c_driver_ov9750 = {
	.driver = {
		.name	= "ov9750",
	},

	.id_table	= ov9750_idtable,
	.probe		= ov9750_probe,
	.remove		= ov9750_remove,

};

static int __init ov9750_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov9750", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov9750);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov9750_exit(void)
{
	i2c_del_driver(&i2c_driver_ov9750);
}

module_init(ov9750_init);
module_exit(ov9750_exit);

MODULE_DESCRIPTION("OV9750 1/3 -Inch, 1280x960, 1.2-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

