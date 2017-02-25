/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx322/imx322.c
 *
 * History:
 *    2014/08/15 - [Long Zhao] Create
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
#include <plat/spi.h>
#include <linux/i2c.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx322.h"

#ifdef CONFIG_SENSOR_IMX322_SPI
static int bus_addr = 0;
#else
static int bus_addr = (0 << 16) | (0x34 >> 1);
#endif
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and cs: bit16~bit31: bus, bit0~bit15: cs");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct imx322_priv {
	void *control_data;
	u32 spi_bus;
	u32 spi_cs;
	u32 line_length;
	u32 frame_length_lines;
};

#include "imx322_table.c"

static int imx322_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	struct imx322_priv *imx322;
	u8 pbuf[3];

#ifdef CONFIG_SENSOR_IMX322_SPI
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	imx322 = (struct imx322_priv *)vdev->priv;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data & 0xff;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 3;
	write.cs_id = imx322->spi_cs;
	write.bus_id = imx322->spi_bus;

	ambarella_spi_write(&config, &write);
#else
	int rval;
	struct i2c_client *client;
	struct i2c_msg msgs[1];

	imx322 = (struct imx322_priv *)vdev->priv;
	client = imx322->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data & 0xff;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf;

	rval = i2c_transfer(client->adapter, msgs, 1);
	if (rval < 0) {
		vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
		return rval;
	}
#endif

	return 0;
}

static int imx322_read_reg(struct vin_device *vdev, u32 subaddr, u32 *data)
{
	struct imx322_priv *imx322;
	u8 pbuf[2];

#ifdef CONFIG_SENSOR_IMX322_SPI
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;
	u8 tmp;

	imx322 = (struct imx322_priv *)vdev->priv;

	pbuf[0] = ((subaddr & 0xff00) >> 8) | 0x80;
	pbuf[1] = subaddr & 0xff;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.w_buffer = pbuf;
	write.w_size = 2;
	write.r_buffer = &tmp;
	write.r_size = 1;
	write.cs_id = imx322->spi_cs;
	write.bus_id = imx322->spi_bus;

	ambarella_spi_write_then_read(&config, &write);

	/* reverse the bit, LSB first */
	*data = tmp;
#else
	int rval;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];

	imx322 = (struct imx322_priv *)vdev->priv;
	client = imx322->control_data;

	pbuf0[0] = (subaddr & 0xff00) >> 8;
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
#endif

	return 0;
}

static int imx322_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx322_config;

	memset(&imx322_config, 0, sizeof(imx322_config));

	imx322_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	imx322_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	imx322_config.input_mode = SENSOR_RGB_1PIX;

	imx322_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	imx322_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;
	imx322_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	imx322_config.cap_win.x = format->def_start_x;
	imx322_config.cap_win.y = format->def_start_y;
	imx322_config.cap_win.width = format->def_width;
	imx322_config.cap_win.height = format->def_height;

	imx322_config.sensor_id	= GENERIC_SENSOR;
	imx322_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx322_config.bayer_pattern = format->bayer_pattern;
	imx322_config.video_format = format->format;
	imx322_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx322_config);
}

static int imx322_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = imx322_share_regs;
	regs_num = ARRAY_SIZE(imx322_share_regs);

	for (i = 0; i < regs_num; i++)
		imx322_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void imx322_start_streaming(struct vin_device *vdev)
{
	imx322_write_reg(vdev, IMX322_STANDBY, IMX322_STREAMING); /* cancel standby */
	msleep(20);
	imx322_write_reg(vdev, IMX322_XMSTA, 0x00); /* master mode */
}

static int imx322_update_hv_info(struct vin_device *vdev)
{
	struct imx322_priv *pinfo = (struct imx322_priv *)vdev->priv;
	u32 val_high, val_low;

	imx322_read_reg(vdev, IMX322_HMAX_MSB, &val_high);
	imx322_read_reg(vdev, IMX322_HMAX_LSB, &val_low);
	pinfo->line_length = ((val_high & 0x3F) << 8) + val_low;
	if (unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx322_read_reg(vdev, IMX322_VMAX_MSB, &val_high);
	imx322_read_reg(vdev, IMX322_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = (val_high << 8) + val_low;

	return 0;
}

static int imx322_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx322_priv *pinfo = (struct imx322_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx322_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx322_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx322_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++) {
		if (regs[i].addr == 0xffff)
			break;
		imx322_write_reg(vdev, regs[i].addr, regs[i].data);
	}

	rval = imx322_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx322_get_line_time(vdev);

	imx322_start_streaming(vdev);

	rval = imx322_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx322_set_shutter_row(struct vin_device *vdev, u32 row)
{
	struct imx322_priv *pinfo = (struct imx322_priv *)vdev->priv;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	int blank_lines;

	num_line = row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	min_line = 1;
	max_line = pinfo->frame_length_lines;
	num_line = clamp(num_line, min_line, max_line);

	blank_lines = pinfo->frame_length_lines - num_line; /* get the shutter sweep time */

	imx322_write_reg(vdev, IMX322_SHS1_MSB, blank_lines >> 8);
	imx322_write_reg(vdev, IMX322_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx322_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	struct imx322_priv *pinfo = (struct imx322_priv *)vdev->priv;
	u64 exposure_lines;
	int rval = 0;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if (unlikely(!pinfo->line_length)) {
		rval = imx322_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		imx322_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx322_set_fps(struct vin_device *vdev, int fps)
{
	struct imx322_priv *pinfo = (struct imx322_priv *)vdev->priv;
	u64 v_lines, vb_time;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx322_write_reg(vdev, IMX322_VMAX_MSB, (v_lines >> 8) & 0xFF);
	imx322_write_reg(vdev, IMX322_VMAX_LSB, v_lines & 0xFF);

	pinfo->frame_length_lines = v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx322_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX322_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX322_GAIN_MAX_DB);
		agc_idx = IMX322_GAIN_MAX_DB;
	}

	imx322_write_reg(vdev, IMX322_REG_GAIN, agc_idx);
	return 0;
}

static int imx322_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX322_H_MIRROR | IMX322_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX322_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX322_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx322_read_reg(vdev, IMX322_HVREVERSE, &tmp_reg);
	tmp_reg &= ~IMX322_V_FLIP;
	tmp_reg |= readmode;
	imx322_write_reg(vdev, IMX322_HVREVERSE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

#ifdef CONFIG_PM
static int imx322_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx322_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int imx322_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++)
		imx322_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);

	return 0;
}
#endif

static struct vin_ops imx322_ops = {
	.init_device		= imx322_init_device,
	.set_format		= imx322_set_format,
	.set_shutter_row	= imx322_set_shutter_row,
	.shutter2row		= imx322_shutter2row,
	.set_frame_rate		= imx322_set_fps,
	.set_agc_index		= imx322_set_agc_index,
	.set_mirror_mode	= imx322_set_mirror_mode,
	.read_reg		= imx322_read_reg,
	.write_reg		= imx322_write_reg,
#ifdef CONFIG_PM
	.suspend		= imx322_suspend,
	.resume			= imx322_resume,
#endif
};

#ifdef CONFIG_SENSOR_IMX322_SPI
/************************ for SPI *****************************/
static int imx322_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	int i;
	struct vin_device *vdev;
	struct imx322_priv *imx322;

	vdev = ambarella_vin_create_device(ambdev->name,
		SENSOR_IMX322, sizeof(struct imx322_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2A000000;	/* 42dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x004ccccd;	/* 0.3dB */

	imx322 = (struct imx322_priv *)vdev->priv;
	imx322->spi_bus = bus_addr >> 16;
	imx322->spi_cs = bus_addr & 0xffff;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(imx322_formats); i++)
				imx322_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &imx322_ops,
		imx322_formats, ARRAY_SIZE(imx322_formats),
		imx322_plls, ARRAY_SIZE(imx322_plls));

	if (rval < 0) {
		ambarella_vin_free_device(vdev);
		return rval;
	}

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("IMX322-SPI init(parallel)\n");

	return 0;
}

static int imx322_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver imx322_driver = {
	.probe = imx322_drv_probe,
	.remove = imx322_drv_remove,
	.driver = {
		.name = "imx322",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *imx322_device;
#else
/************************ for I2C********************/
static int imx322_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	int i;
	struct vin_device *vdev;
	struct imx322_priv *imx322;

	vdev = ambarella_vin_create_device(client->name,
		SENSOR_IMX322, sizeof(struct imx322_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2A000000;	/* 42dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x004ccccd;	/* 0.3dB */

	i2c_set_clientdata(client, vdev);

	imx322 = (struct imx322_priv *)vdev->priv;
	imx322->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(imx322_formats); i++)
				imx322_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &imx322_ops,
		imx322_formats, ARRAY_SIZE(imx322_formats),
		imx322_plls, ARRAY_SIZE(imx322_plls));

	if (rval < 0)
		goto imx322_probe_err;

	vin_info("IMX322-I2C init(parallel)\n");

	return 0;

imx322_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx322_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx322_idtable[] = {
	{ "imx322", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx322_idtable);

static struct i2c_driver i2c_driver_imx322 = {
	.driver = {
		.name	= "imx322",
	},

	.id_table	= imx322_idtable,
	.probe		= imx322_probe,
	.remove		= imx322_remove,

};
#endif

static int __init imx322_init(void)
{
	int rval = 0;

#ifdef CONFIG_SENSOR_IMX322_SPI
	imx322_device = ambpriv_create_bundle(&imx322_driver, NULL, -1,  NULL, -1);
	if (IS_ERR(imx322_device))
		rval = PTR_ERR(imx322_device);
#else
	int bus, addr;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx322", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx322);
#endif

	return rval;
}

static void __exit imx322_exit(void)
{
#ifdef CONFIG_SENSOR_IMX322_SPI
	ambpriv_device_unregister(imx322_device);
	ambpriv_driver_unregister(&imx322_driver);
#else
	i2c_del_driver(&i2c_driver_imx322);
#endif
}

module_init(imx322_init);
module_exit(imx322_exit);

MODULE_DESCRIPTION("IMX322 1/2.9 -Inch, 1936x1097, 2.12-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

