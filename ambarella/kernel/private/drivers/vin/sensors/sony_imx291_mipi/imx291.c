/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx291_mipi/imx291.c
 *
 * History:
 *    2016/08/23 - [Hao Zeng] created file
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
#include <linux/pm.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx291.h"
#include "imx291_table.c"

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

struct imx291_priv {
	void *control_data;
	u32 frame_length_lines;
	u32 line_length;
};

static int imx291_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct imx291_priv *imx291;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	imx291 = (struct imx291_priv *)vdev->priv;
	client = imx291->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX291_SWRESET))
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

static int imx291_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct imx291_priv *imx291;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[1];

	imx291 = (struct imx291_priv *)vdev->priv;
	client = imx291->control_data;

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
	if (rval < 0) {
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = pbuf[0];

	return 0;
}

static int imx291_set_hold_mode(struct vin_device *vdev, u32 hold_mode)
{
	imx291_write_reg(vdev, IMX291_REGHOLD, hold_mode);
	return 0;
}

static int imx291_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx291_config;

	memset(&imx291_config, 0, sizeof(imx291_config));

	imx291_config.interface_type = SENSOR_MIPI;
	imx291_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	imx291_config.slvds_cfg.lane_number = SENSOR_2_LANE;

	imx291_config.cap_win.x = format->def_start_x;
	imx291_config.cap_win.y = format->def_start_y;
	imx291_config.cap_win.width = format->def_width;
	imx291_config.cap_win.height = format->def_height;

	imx291_config.sensor_id	= GENERIC_SENSOR;
	imx291_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx291_config.bayer_pattern = format->bayer_pattern;
	imx291_config.video_format = format->format;
	imx291_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx291_config);
}

static void imx291_sw_reset(struct vin_device *vdev)
{
	imx291_write_reg(vdev, IMX291_STANDBY, 0x1);/* STANDBY */
	imx291_write_reg(vdev, IMX291_SWRESET, 0x1);
	msleep(10);
}

static void imx291_start_streaming(struct vin_device *vdev)
{
	imx291_write_reg(vdev, IMX291_XMSTA, 0x0); /* master mode start */
	imx291_write_reg(vdev, IMX291_STANDBY, 0x0); /* cancel standby */
	msleep(30);
}

static int imx291_init_device(struct vin_device *vdev)
{
	imx291_sw_reset(vdev);
	return 0;
}

static int imx291_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx291_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx291_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx291_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx291_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx291_priv *pinfo = (struct imx291_priv *)vdev->priv;

	imx291_read_reg(vdev, IMX291_HMAX_MSB, &val_high);
	imx291_read_reg(vdev, IMX291_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx291_read_reg(vdev, IMX291_VMAX_HSB, &val_high);
	imx291_read_reg(vdev, IMX291_VMAX_MSB, &val_mid);
	imx291_read_reg(vdev, IMX291_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx291_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx291_priv *pinfo = (struct imx291_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx291_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx291_share_regs;
	regs_num = ARRAY_SIZE(imx291_share_regs);
	for (i = 0; i < regs_num; i++)
		imx291_write_reg(vdev, regs[i].addr, regs[i].data);

	regs = imx291_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx291_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		imx291_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = imx291_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx291_get_line_time(vdev);

	/* TG reset release ( Enable Streaming ) */
	imx291_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx291_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx291_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx291_priv *pinfo = (struct imx291_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 2) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line - 1;
	imx291_write_reg(vdev, IMX291_SHS1_HSB, blank_lines >> 16);
	imx291_write_reg(vdev, IMX291_SHS1_MSB, blank_lines >> 8);
	imx291_write_reg(vdev, IMX291_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx291_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx291_priv *pinfo = (struct imx291_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx291_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx291_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct imx291_priv *pinfo = (struct imx291_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx291_write_reg(vdev, IMX291_VMAX_HSB, (v_lines & 0x030000) >> 16);
	imx291_write_reg(vdev, IMX291_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx291_write_reg(vdev, IMX291_VMAX_LSB, v_lines & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx291_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	struct imx291_priv *pinfo;
	u32 tmp;

	pinfo = (struct imx291_priv *)vdev->priv;

	/* if gain >= 30db, enable HCG, HCG is 6db */
	if (agc_idx > 100) {
		imx291_read_reg(vdev, IMX291_FRSEL, &tmp);
		tmp |= IMX291_HI_GAIN_MODE;
		imx291_write_reg(vdev, IMX291_FRSEL, tmp);
		vin_debug("high gain mode\n");
		agc_idx -= 20;
	} else {
		imx291_read_reg(vdev, IMX291_FRSEL, &tmp);
		tmp &= ~IMX291_HI_GAIN_MODE;
		imx291_write_reg(vdev, IMX291_FRSEL, tmp);
		vin_debug("low gain mode\n");
	}

	if (agc_idx > IMX291_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX291_GAIN_MAX_DB);
		agc_idx = IMX291_GAIN_MAX_DB;
	}

	imx291_write_reg(vdev, IMX291_GAIN, (u8)agc_idx);

	return 0;
}

static int imx291_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX291_H_MIRROR | IMX291_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX291_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX291_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx291_read_reg(vdev, IMX291_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX291_H_MIRROR | IMX291_V_FLIP);
	tmp_reg |= readmode;
	imx291_write_reg(vdev, IMX291_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

#ifdef CONFIG_PM
static int imx291_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx291_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}
static int imx291_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx291_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops imx291_ops = {
	.init_device		= imx291_init_device,
	.set_format		= imx291_set_format,
	.set_pll			= imx291_set_pll,
	.set_shutter_row 	= imx291_set_shutter_row,
	.shutter2row 		= imx291_shutter2row,
	.set_frame_rate	= imx291_set_fps,
	.set_agc_index		= imx291_set_agc_index,
	.set_mirror_mode	= imx291_set_mirror_mode,
	.set_hold_mode		= imx291_set_hold_mode,
	.read_reg			= imx291_read_reg,
	.write_reg		= imx291_write_reg,
#ifdef CONFIG_PM
	.suspend 			= imx291_suspend,
	.resume 			= imx291_resume,
#endif
};

/* ========================================================================== */
static int imx291_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx291_priv *imx291;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_IMX291, sizeof(struct imx291_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x3F000000;  /* 63dB */
	vdev->agc_db_min = 0x00000000;  /* 0dB */
	vdev->agc_db_step = 0x004CCCCC; /* 0.3dB */

	/* mode switch needs hw reset */
	vdev->reset_for_mode_switch = true;

	i2c_set_clientdata(client, vdev);

	imx291 = (struct imx291_priv *)vdev->priv;
	imx291->control_data = client;

	rval = ambarella_vin_register_device(vdev, &imx291_ops,
		imx291_formats, ARRAY_SIZE(imx291_formats),
		imx291_plls, ARRAY_SIZE(imx291_plls));

	if (rval < 0)
		goto imx291_probe_err;

	vin_info("IMX291 init(2-lane mipi)\n");

	return 0;

imx291_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx291_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx291_idtable[] = {
	{ "imx291", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx291_idtable);

static struct i2c_driver i2c_driver_imx291 = {
	.driver = {
		.name	= "imx291",
	},

	.id_table	= imx291_idtable,
	.probe		= imx291_probe,
	.remove		= imx291_remove,
};

static int __init imx291_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx291", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx291);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit imx291_exit(void)
{
	i2c_del_driver(&i2c_driver_imx291);
}

module_init(imx291_init);
module_exit(imx291_exit);

MODULE_DESCRIPTION("IMX291 1/2.8-Inch, 1945x1097, 2.13-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

