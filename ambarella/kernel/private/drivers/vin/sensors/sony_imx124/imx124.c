/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx124/imx124.c
 *
 * History:
 *    2014/07/23 - [Long Zhao] Create
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
#include "imx124.h"

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct imx124_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "imx124_table.c"

static int imx124_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct imx124_priv *imx124;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	imx124 = (struct imx124_priv *)vdev->priv;
	client = imx124->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX124_SWRESET))
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

static int imx124_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct imx124_priv *imx124;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	imx124 = (struct imx124_priv *)vdev->priv;
	client = imx124->control_data;

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

static int imx124_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx124_config;

	memset(&imx124_config, 0, sizeof (imx124_config));

	imx124_config.interface_type = SENSOR_SERIAL_LVDS;
	imx124_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	imx124_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	imx124_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;

	imx124_config.cap_win.x = format->def_start_x;
	imx124_config.cap_win.y = format->def_start_y;
	imx124_config.cap_win.width = format->def_width;
	imx124_config.cap_win.height = format->def_height;

	imx124_config.sensor_id	= GENERIC_SENSOR;
	imx124_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx124_config.bayer_pattern = format->bayer_pattern;
	imx124_config.video_format = format->format;
	imx124_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx124_config);
}

static void imx124_sw_reset(struct vin_device *vdev)
{
	imx124_write_reg(vdev, IMX124_STANDBY, 0x1);//STANDBY
	imx124_write_reg(vdev, IMX124_SWRESET, 0x1);
	msleep(10);
}

static void imx124_start_streaming(struct vin_device *vdev)
{
	u32 val;
	imx124_write_reg(vdev, IMX124_XMSTA, 0x0); //master mode start
	imx124_write_reg(vdev, IMX124_STANDBY, 0x0); //cancel standby
	msleep(20);

	imx124_read_reg(vdev, IMX124_DCKRST, &val);
	val |= IMX124_DCKRST_BIT;
	imx124_write_reg(vdev, IMX124_DCKRST, val); /* DCKRST */
	val &= ~IMX124_DCKRST_BIT;
	imx124_write_reg(vdev, IMX124_DCKRST, val); /* DCKRST */
}

static int imx124_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	imx124_sw_reset(vdev);

	regs = imx124_share_regs;
	regs_num = ARRAY_SIZE(imx124_share_regs);

	for (i = 0; i < regs_num; i++)
		imx124_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx124_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx124_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx124_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx124_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx124_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx124_priv *pinfo = (struct imx124_priv *)vdev->priv;

	imx124_read_reg(vdev, IMX124_HMAX_MSB, &val_high);
	imx124_read_reg(vdev, IMX124_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx124_read_reg(vdev, IMX124_VMAX_HSB, &val_high);
	imx124_read_reg(vdev, IMX124_VMAX_MSB, &val_mid);
	imx124_read_reg(vdev, IMX124_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx124_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx124_priv *pinfo = (struct imx124_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx124_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx124_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx124_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		imx124_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = imx124_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx124_get_line_time(vdev);

	/* TG reset release ( Enable Streaming )*/
	imx124_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx124_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx124_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx124_priv *pinfo = (struct imx124_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 2 ~ (Frame format(V) - 2) */
	min_line = 2;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line;
	imx124_write_reg(vdev, IMX124_SHS1_HSB, blank_lines >> 16);
	imx124_write_reg(vdev, IMX124_SHS1_MSB, blank_lines >> 8);
	imx124_write_reg(vdev, IMX124_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx124_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx124_priv *pinfo = (struct imx124_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx124_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx124_set_fps( struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct imx124_priv *pinfo = (struct imx124_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx124_write_reg(vdev, IMX124_VMAX_HSB, (v_lines & 0x010000) >> 16);
	imx124_write_reg(vdev, IMX124_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx124_write_reg(vdev, IMX124_VMAX_LSB, v_lines & 0x0000FF);

	imx124_write_reg(vdev, 0x3105, ((v_lines - 1) & 0x010000) >> 16);
	imx124_write_reg(vdev, 0x3104, ((v_lines - 1) & 0x00FF00) >> 8);
	imx124_write_reg(vdev, 0x3103, (v_lines - 1) & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx124_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX124_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX124_GAIN_MAX_DB);
		agc_idx = IMX124_GAIN_MAX_DB;
	}

	imx124_write_reg(vdev, IMX124_GAIN_LSB, (u8)(agc_idx&0xFF));
	imx124_write_reg(vdev, IMX124_GAIN_MSB, (u8)(agc_idx>>8)&0x3);

	return 0;
}

static int imx124_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern, reg_31f6;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX124_H_MIRROR | IMX124_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		reg_31f6 = 0x49;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX124_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		reg_31f6 = 0x49;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX124_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		reg_31f6 = 0x59;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		reg_31f6 = 0x59;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx124_read_reg(vdev, IMX124_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX124_H_MIRROR | IMX124_V_FLIP);
	tmp_reg |= readmode;
	imx124_write_reg(vdev, IMX124_WINMODE, tmp_reg);

	imx124_write_reg(vdev, 0x31F6, reg_31f6);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops imx124_ops = {
	.init_device		= imx124_init_device,
	.set_format		= imx124_set_format,
	.set_pll			= imx124_set_pll,
	.set_shutter_row = imx124_set_shutter_row,
	.shutter2row = imx124_shutter2row,
	.set_frame_rate		= imx124_set_fps,
	.set_agc_index		= imx124_set_agc_index,
	.set_mirror_mode	= imx124_set_mirror_mode,
	.read_reg		= imx124_read_reg,
	.write_reg		= imx124_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int imx124_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx124_priv *imx124;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_IMX124, sizeof(struct imx124_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_QXGA;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x33000000;	// 51dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00199999;	// 0.1dB

	i2c_set_clientdata(client, vdev);

	imx124 = (struct imx124_priv *)vdev->priv;
	imx124->control_data = client;

	rval = ambarella_vin_register_device(vdev, &imx124_ops,
		imx124_formats, ARRAY_SIZE(imx124_formats),
		imx124_plls, ARRAY_SIZE(imx124_plls));

	if (rval < 0)
		goto imx124_probe_err;

	vin_info("IMX124 init(4-lane lvds)\n");

	return 0;

imx124_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx124_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx124_idtable[] = {
	{ "imx124", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx124_idtable);

static struct i2c_driver i2c_driver_imx124 = {
	.driver = {
		.name	= "imx124",
	},

	.id_table	= imx124_idtable,
	.probe		= imx124_probe,
	.remove		= imx124_remove,

};

static int __init imx124_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx124", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx124);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit imx124_exit(void)
{
	i2c_del_driver(&i2c_driver_imx124);
}

module_init(imx124_init);
module_exit(imx124_exit);

MODULE_DESCRIPTION("IMX124 1/2.8 -Inch, 2065x1565, 3.20-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

