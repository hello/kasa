/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx178/imx178.c
 *
 * History:
 *    2012/02/21 - [Long Zhao] Create
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
#include <plat/spi.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx178.h"

#ifdef CONFIG_SENSOR_IMX178_SPI
static int bus_addr = 0;
#else
static int bus_addr = (0 << 16) | (0x34 >> 1);
#endif
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr/cs: bit16~bit31: bus, bit0~bit15: addr/cs");

static int lane = 8;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "Set lvds lane number 8:8 lane, 10:10 lane");

struct imx178_priv {
	void *control_data;
	/* for spi I/F */
	u32 spi_bus;
	u32 spi_cs;
	u32 frame_length_lines;
	u32 line_length;
};

#include "imx178_table.c"

static int imx178_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	struct imx178_priv *imx178;
	u8 pbuf[3];

#ifdef CONFIG_SENSOR_IMX178_SPI
	amba_spi_cfg_t config;
	amba_spi_write_t write;

	imx178 = (struct imx178_priv *)vdev->priv;

	pbuf[0] = subaddr>>0x08;
	pbuf[1] = subaddr&0xFF;
	pbuf[2] = data;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 3;
	write.cs_id = imx178->spi_cs;
	write.bus_id = imx178->spi_bus;

	ambarella_spi_write(&config, &write);
#else
	int rval;
	struct i2c_client *client;
	struct i2c_msg msgs[1];

	imx178 = (struct imx178_priv *)vdev->priv;
	client = imx178->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data;

	msgs[0].len = 3;
	msgs[0].addr = client->addr;
	if (unlikely(subaddr == IMX178_SWRESET))
		msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	else
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

static int imx178_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	struct imx178_priv *imx178;
	u8 pbuf[2];

#ifdef CONFIG_SENSOR_IMX178_SPI
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;
	u8 tmp;

	imx178 = (struct imx178_priv *)vdev->priv;

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
	write.cs_id = imx178->spi_cs;
	write.bus_id = imx178->spi_bus;

	ambarella_spi_write_then_read(&config, &write);

	*data = tmp;
#else
	int rval = 0;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];

	imx178 = (struct imx178_priv *)vdev->priv;
	client = imx178->control_data;

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
#endif

	return 0;
}

static int imx178_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx178_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx178_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx178_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

/*
 * It is advised by sony, that they only guarantee the sync code. However, if we
 * want to use external sync signal, use the DCK sync mode. Try to use sync code
 * first!!!
 */
static int imx178_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx178_config;

	memset(&imx178_config, 0, sizeof (imx178_config));

	imx178_config.interface_type = SENSOR_SERIAL_LVDS;
	imx178_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	if (lane == 10) {
		imx178_config.slvds_cfg.lane_number = SENSOR_10_LANE;
	} else {
		imx178_config.slvds_cfg.lane_number = SENSOR_8_LANE;
	}

	imx178_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;

	imx178_config.cap_win.x = format->def_start_x;
	imx178_config.cap_win.y = format->def_start_y;
	imx178_config.cap_win.width = format->def_width;
	imx178_config.cap_win.height = format->def_height;

	imx178_config.sensor_id	= GENERIC_SENSOR;
	imx178_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx178_config.bayer_pattern = format->bayer_pattern;
	imx178_config.video_format = format->format;
	imx178_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx178_config);
}

static void imx178_sw_reset(struct vin_device *vdev)
{
	imx178_write_reg(vdev, IMX178_STANDBY, 0x07);/* STANDBY  */
	imx178_write_reg(vdev, IMX178_SWRESET, 0x01);/* SW_RESET  */
	msleep(30);
}

static void imx178_start_streaming(struct vin_device *vdev)
{
	imx178_write_reg(vdev, IMX178_XMSTA, 0x00); /* master mode start */
	imx178_write_reg(vdev, IMX178_STANDBY, 0x00); /* cancel standby */
}

static int imx178_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	imx178_sw_reset(vdev);

	regs = imx178_share_regs;
	regs_num = ARRAY_SIZE(imx178_share_regs);

	for (i = 0; i < regs_num; i++)
		imx178_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx178_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx178_priv *pinfo = (struct imx178_priv *)vdev->priv;

	imx178_read_reg(vdev, IMX178_HMAX_MSB, &val_high);
	imx178_read_reg(vdev, IMX178_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx178_read_reg(vdev, IMX178_VMAX_HSB, &val_high);
	imx178_read_reg(vdev, IMX178_VMAX_MSB, &val_mid);
	imx178_read_reg(vdev, IMX178_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx178_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx178_priv *pinfo = (struct imx178_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx178_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	if (lane == 10) {
		regs = imx178_mode_regs_10lane[format->device_mode];
		regs_num = ARRAY_SIZE(imx178_mode_regs_10lane[format->device_mode]);
	} else {
		regs = imx178_mode_regs_8lane[format->device_mode];
		regs_num = ARRAY_SIZE(imx178_mode_regs_8lane[format->device_mode]);
	}

	for (i = 0; i < regs_num; i++)
		imx178_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = imx178_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx178_get_line_time(vdev);

	/* TG reset release ( Enable Streaming )*/
	imx178_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx178_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx178_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	int errCode = 0;
	struct imx178_priv *pinfo = (struct imx178_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 1) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 1;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line;
	imx178_write_reg(vdev, IMX178_SHS1_HSB, blank_lines >> 16);
	imx178_write_reg(vdev, IMX178_SHS1_MSB, blank_lines >> 8);
	imx178_write_reg(vdev, IMX178_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int imx178_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx178_priv *pinfo = (struct imx178_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx178_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * ((u64)vdev->cur_pll->pixelclk);
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx178_set_fps( struct vin_device *vdev, int fps)
{
	u64 v_lines;
	struct imx178_priv *pinfo = (struct imx178_priv *)vdev->priv;

	v_lines = fps * ((u64)vdev->cur_pll->pixelclk);
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx178_write_reg(vdev, IMX178_VMAX_HSB, (v_lines & 0x010000) >> 16);
	imx178_write_reg(vdev, IMX178_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx178_write_reg(vdev, IMX178_VMAX_LSB, v_lines & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	return 0;
}

static int imx178_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	u16 val = agc_idx;
	if (val > IMX178_GAIN_MAX_DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX178_GAIN_MAX_DB);
		return -EPERM;
	}

	imx178_write_reg(vdev, IMX178_GAIN_LSB, (u8)(val&0xFF));
	imx178_write_reg(vdev, IMX178_GAIN_MSB, (u8)(val>>8));

	return 0;
}

static int imx178_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = IMX178_H_MIRROR | IMX178_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = IMX178_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX178_V_FLIP;
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

	imx178_read_reg(vdev, IMX178_WINMODE, &tmp_reg);
	tmp_reg &= ~(IMX178_H_MIRROR | IMX178_V_FLIP);
	tmp_reg |= readmode;
	imx178_write_reg(vdev, IMX178_WINMODE, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops imx178_ops = {
	.init_device		= imx178_init_device,
	.set_pll		= imx178_set_pll,
	.set_format		= imx178_set_format,
	.set_shutter_row	= imx178_set_shutter_row,
	.shutter2row		= imx178_shutter2row,
	.set_frame_rate		= imx178_set_fps,
	.set_agc_index		= imx178_set_agc_index,
	.set_mirror_mode	= imx178_set_mirror_mode,
	.read_reg		= imx178_read_reg,
	.write_reg		= imx178_write_reg,
};

#ifdef CONFIG_SENSOR_IMX178_SPI
/************************ for SPI *****************************/
static int imx178_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx178_priv *imx178;

	vdev = ambarella_vin_create_device(ambdev->name,
			SENSOR_IMX178, 	sizeof(struct imx178_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_3072_2048;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	/* 48dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00199999;	/* 0.1dB */
	vdev->pixel_size = 0x00026666;	/* 2.4um */

	imx178 = (struct imx178_priv *)vdev->priv;
	imx178->spi_bus = bus_addr >> 16;
	imx178->spi_cs = bus_addr & 0xffff;

	/* workaround: 6.3M 8lane 10bits max fps is 30,
		5.0M 8lane 12bits max fps is 30 */
	if (lane == 8) {
		imx178_formats[4].max_fps = AMBA_VIDEO_FPS_30;
		imx178_formats[8].max_fps = AMBA_VIDEO_FPS_30;
	}

	rval = ambarella_vin_register_device(vdev, &imx178_ops,
		imx178_formats, ARRAY_SIZE(imx178_formats),
		imx178_plls, ARRAY_SIZE(imx178_plls));

	if (rval < 0) {
		ambarella_vin_free_device(vdev);
		return rval;
	}

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("IMX178-SPI Init(%d-lane lvds)\n", lane);

	return 0;
}

static int imx178_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver imx178_driver = {
	.probe = imx178_drv_probe,
	.remove = imx178_drv_remove,
	.driver = {
		.name = "imx178",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *imx178_device;
#else
/************************ for I2C *****************************/
static int imx178_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx178_priv *imx178;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_IMX178, sizeof(struct imx178_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_3072_2048;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	/* 48dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00199999;	/* 0.1dB */
	vdev->pixel_size = 0x00026666;	/* 2.4um */

	i2c_set_clientdata(client, vdev);

	imx178 = (struct imx178_priv *)vdev->priv;
	imx178->control_data = client;

	/* workaround: 6.3M 8lane 10bits max fps is 30,
		5.0M 8lane 12bits max fps is 30 */
	if (lane == 8) {
		imx178_formats[4].max_fps = AMBA_VIDEO_FPS_30;
		imx178_formats[8].max_fps = AMBA_VIDEO_FPS_30;
	}

	rval = ambarella_vin_register_device(vdev, &imx178_ops,
		imx178_formats, ARRAY_SIZE(imx178_formats),
		imx178_plls, ARRAY_SIZE(imx178_plls));

	if (rval < 0)
		goto imx178_probe_err;

	vin_info("IMX178-I2C Init(%d-lane lvds)\n", lane);

	return 0;

imx178_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx178_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id imx178_idtable[] = {
	{ "imx178", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx178_idtable);

static struct i2c_driver i2c_driver_imx178 = {
	.driver = {
		.name	= "imx178",
	},

	.id_table	= imx178_idtable,
	.probe		= imx178_probe,
	.remove		= imx178_remove,

};
#endif

static int __init imx178_init(void)
{
	int rval = 0;
#ifdef CONFIG_SENSOR_IMX178_SPI
	imx178_device = ambpriv_create_bundle(&imx178_driver, NULL, -1, NULL, -1);
	if (IS_ERR(imx178_device))
		rval = PTR_ERR(imx178_device);
#else
	int bus, addr;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("imx178", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_imx178);
#endif

	return rval;
}

static void __exit imx178_exit(void)
{
#ifdef CONFIG_SENSOR_IMX178_SPI
	ambpriv_device_unregister(imx178_device);
	ambpriv_driver_unregister(&imx178_driver);
#else
	i2c_del_driver(&i2c_driver_imx178);
#endif
}

module_init(imx178_init);
module_exit(imx178_exit);

MODULE_DESCRIPTION("IMX178 1/1.8 -Inch, 3096x2080, 6.44-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

