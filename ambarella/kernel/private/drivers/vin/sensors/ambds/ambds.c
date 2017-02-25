/*
 * kernel/private/drivers/ambarella/vin/sensors/ambds/ambds.c
 *
 * History:
 *    2015/01/30 - [Long Zhao] Create
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
#include "ambds.h"

static int spi = 0;
module_param(spi, int, 0644);
MODULE_PARM_DESC(spi, " Use SPI interface 0:I2C 1:SPI");

static int bus_addr = (0 << 16) | (0x34 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr/cs: bit16~bit31: bus, bit0~bit15: addr/cs");

static int lane = 8;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "Set MIPI lane number 8:8 lane, 10:10 lane");

static int yuv_mode = 0;
module_param(yuv_mode, int, 0644);
MODULE_PARM_DESC(yuv_mode, "Set RGB/YUV mode 0:RGB 1:YUV");

static int yuv_pix_order = 1;
module_param(yuv_pix_order, int, 0644);
MODULE_PARM_DESC(yuv_pix_order, "0: CR_Y0_CB_Y1, 1: CB_Y0_CR_Y1, 2: Y0_CR_Y1_CB, 3: Y0_CB_Y1_CR");

static int sync_duplicate = 1;
module_param(sync_duplicate, int, 0644);
MODULE_PARM_DESC(sync_duplicate, "sync code sytle: duplicate on each lane");
/* ========================================================================== */
struct ambds_priv {
	void *control_data;
	/* for spi I/F */
	u32 spi_bus;
	u32 spi_cs;
};

#include "ambds_table.c"

static int ambds_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ambds_priv *ambds;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[3];

	ambds = (struct ambds_priv *)vdev->priv;

	if (spi) {
		amba_spi_cfg_t config;
		amba_spi_write_t write;

		pbuf[0] = subaddr>>0x08;
		pbuf[1] = subaddr&0xFF;
		pbuf[2] = data;

		config.cfs_dfs = 8;
		config.baud_rate = 1000000;
		config.cs_change = 0;
		config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

		write.buffer = pbuf;
		write.n_size = 3;
		write.cs_id = ambds->spi_cs;
		write.bus_id = ambds->spi_bus;

		/*
		ambarella_spi_write(&config, &write);
		*/
	} else { /* I2C */
		client = ambds->control_data;

		pbuf[0] = (subaddr & 0xff00) >> 8;
		pbuf[1] = subaddr & 0xff;
		pbuf[2] = data;

		msgs[0].len = 3;
		msgs[0].addr = client->addr;
		msgs[0].flags = client->flags;
		msgs[0].buf = pbuf;

		rval = 0;
		/*
		rval = i2c_transfer(client->adapter, msgs, 1);
		if (rval < 0) {
			vin_error("failed(%d): [0x%x:0x%x]\n", rval, subaddr, data);
			return rval;
		}
		*/
	}

	return 0;
}

static int ambds_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ambds_priv *ambds;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[3], tmp = 0;

	ambds = (struct ambds_priv *)vdev->priv;

	if (spi) {
		amba_spi_cfg_t config;
		amba_spi_write_then_read_t write;

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
		write.cs_id = ambds->spi_cs;
		write.bus_id = ambds->spi_bus;

		/*
		ambarella_spi_write_then_read(&config, &write);
		*/

		*data = tmp;
	} else { /* I2C */
		client = ambds->control_data;

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

		(void)(rval);
		pbuf[0] = 0; /* please remove this in actual use */
		/*
		rval = i2c_transfer(client->adapter, msgs, 2);
		if (rval < 0){
			vin_error("failed(%d): [0x%x]\n", rval, subaddr);
			return rval;
		}
		*/

		*data = pbuf[0];
	}

	return 0;
}

static int ambds_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = ambds_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ambds_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		ambds_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int ambds_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ambds_config;

	memset(&ambds_config, 0, sizeof (ambds_config));

	ambds_config.interface_type = SENSOR_SERIAL_LVDS;
	/* set master sensor/fpga */
	ambds_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	ambds_config.slvds_cfg.lane_number = lane;
	/* you can set lane mapping sequence here */
	ambds_config.slvds_cfg.lane_mux_0= 0x3210;
	ambds_config.slvds_cfg.lane_mux_1= 0x7654;
	ambds_config.slvds_cfg.lane_mux_2= 0xba98;

	/* set different sync code style */
	if (sync_duplicate)
		ambds_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;
	else
		ambds_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_HISPI;

	ambds_config.cap_win.x = format->def_start_x;
	ambds_config.cap_win.y = format->def_start_y;
	ambds_config.cap_win.width = format->def_width;

	if (yuv_mode) {
		if (format->format == AMBA_VIDEO_FORMAT_INTERLACE) {
			ambds_config.cap_win.height = format->def_height >> 1;
			ambds_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
		} else {
			ambds_config.cap_win.height = format->def_height;
			ambds_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
		}

		ambds_config.input_mode = SENSOR_YUV_2PIX;
		/* please set different yuv pixel order here */
		ambds_config.yuv_pixel_order = yuv_pix_order;
		/* for yuv mode, always set this bit_resolution to 8 bits */
		ambds_config.bit_resolution = AMBA_VIDEO_BITS_8;
	} else {
		ambds_config.input_mode = SENSOR_RGB_1PIX;
		ambds_config.cap_win.height = format->def_height;
		ambds_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
		ambds_config.bit_resolution = format->bits;
	}

	ambds_config.sensor_id	= GENERIC_SENSOR;
	ambds_config.bayer_pattern = format->bayer_pattern;
	ambds_config.video_format = format->format;

	return ambarella_set_vin_config(vdev, &ambds_config);
}

static void ambds_sw_reset(struct vin_device *vdev)
{
	/* set software reset register */
}

static void ambds_start_streaming(struct vin_device *vdev)
{
	/* set sensor streaming/cancel standby register */
}

static int ambds_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	ambds_sw_reset(vdev);

	regs = ambds_share_regs;
	regs_num = ARRAY_SIZE(ambds_share_regs);

	for (i = 0; i < regs_num; i++)
		ambds_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int ambds_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = ambds_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ambds_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		ambds_write_reg(vdev, regs[i].addr, regs[i].data);

	/* TG reset release ( Enable Streaming )*/
	ambds_start_streaming(vdev);

	/* communiate with IAV */
	rval = ambds_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ambds_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 val_high, val_mid, val_low;
	u32 v_lines, h_clks, blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	int errCode = 0;

	ambds_read_reg(vdev, AMBDS_HMAX_MSB, &val_high);
	ambds_read_reg(vdev, AMBDS_HMAX_LSB, &val_low);
	h_clks = (val_high << 8) + val_low;
	h_clks = 0xFFFF;
	if(unlikely(!h_clks)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ambds_read_reg(vdev, AMBDS_VMAX_HSB, &val_high);
	ambds_read_reg(vdev, AMBDS_VMAX_MSB, &val_mid);
	ambds_read_reg(vdev, AMBDS_VMAX_LSB, &val_low);
	v_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 1) */
	min_line = 1;
	max_line = v_lines - 1;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = v_lines - num_line;
	ambds_write_reg(vdev, AMBDS_SHS1_HSB, blank_lines >> 16);
	ambds_write_reg(vdev, AMBDS_SHS1_MSB, blank_lines >> 8);
	ambds_write_reg(vdev, AMBDS_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)h_clks * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ambds_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u32 val_high, val_low, h_clks;
	u64 exposure_lines;
	int errCode = 0;

	ambds_read_reg(vdev, AMBDS_HMAX_MSB, &val_high);
	ambds_read_reg(vdev, AMBDS_HMAX_LSB, &val_low);
	h_clks = (val_high << 8) + val_low;
	h_clks = 0xFFFF;
	if(unlikely(!h_clks)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, h_clks);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return errCode;
}

static int ambds_set_fps( struct vin_device *vdev, int fps)
{
	u32 h_clks, val_high, val_low;
	u64 v_lines;

	ambds_read_reg(vdev, AMBDS_HMAX_MSB, &val_high);
	ambds_read_reg(vdev, AMBDS_HMAX_LSB, &val_low);
	h_clks = (val_high << 8) + val_low;
	h_clks = 0xFFFF;
	BUG_ON(h_clks == 0);

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, h_clks);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ambds_write_reg(vdev, AMBDS_VMAX_HSB, (v_lines & 0x010000) >> 16);
	ambds_write_reg(vdev, AMBDS_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	ambds_write_reg(vdev, AMBDS_VMAX_LSB, v_lines & 0x0000FF);

	return 0;
}

static int ambds_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	u16 val = agc_idx;

	ambds_write_reg(vdev, AMBDS_GAIN_LSB, (u8)(val&0xFF));
	ambds_write_reg(vdev, AMBDS_GAIN_MSB, (u8)(val>>8));

	return 0;
}

static int ambds_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = 1;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = 2;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 3;
		bayer_pattern = VINDEV_BAYER_PATTERN_GB;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	/* please set mirror/flip register here */
	ambds_write_reg(vdev, AMBDS_WINMODE, readmode);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops ambds_ops = {
	.init_device		= ambds_init_device,
	.set_pll			= ambds_set_pll,
	.set_format		= ambds_set_format,
	.set_shutter_row 	= ambds_set_shutter_row,
	.shutter2row 		= ambds_shutter2row,
	.set_frame_rate	= ambds_set_fps,
	.set_agc_index	= ambds_set_agc_index,
	.set_mirror_mode	= ambds_set_mirror_mode,
	.read_reg			= ambds_read_reg,
	.write_reg		= ambds_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ambds_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ambds_priv *ambds;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_AMBDS, sizeof(struct ambds_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_3840_2160;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	/* 48dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00199999;	/* 0.1dB */
	vdev->pixel_size = 0x00026666;	/* 2.4um */

	i2c_set_clientdata(client, vdev);

	ambds = (struct ambds_priv *)vdev->priv;
	ambds->control_data = client;

	rval = ambarella_vin_register_device(vdev, &ambds_ops,
		ambds_formats, ARRAY_SIZE(ambds_formats),
		ambds_plls, ARRAY_SIZE(ambds_plls));
	if (rval < 0)
		goto ambds_probe_err;

	vin_info("AMBDS Init, with I2C interface\n");

	return 0;

ambds_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ambds_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ambds_idtable[] = {
	{ "ambds", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ambds_idtable);

static struct i2c_driver i2c_driver_ambds = {
	.driver = {
		.name	= "ambds",
	},

	.id_table	= ambds_idtable,
	.probe		= ambds_probe,
	.remove		= ambds_remove,

};

/************************ for SPI *****************************/
static int ambds_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ambds_priv *ambds;

	vdev = ambarella_vin_create_device(ambdev->name,
			SENSOR_AMBDS, 	sizeof(struct ambds_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_3840_2160;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x30000000;	/* 48dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00199999;	/* 0.1dB */
	vdev->pixel_size = 0x00026666;	/* 2.4um */

	ambds = (struct ambds_priv *)vdev->priv;
	ambds->spi_bus = bus_addr >> 16;
	ambds->spi_cs = bus_addr & 0xffff;

	rval = ambarella_vin_register_device(vdev, &ambds_ops,
		ambds_formats, ARRAY_SIZE(ambds_formats),
		ambds_plls, ARRAY_SIZE(ambds_plls));
	if (rval < 0) {
		ambarella_vin_free_device(vdev);
		return rval;
	}

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("AMBDS Init, with SPI interface\n");

	return 0;
}

static int ambds_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver ambds_driver = {
	.probe = ambds_drv_probe,
	.remove = ambds_drv_remove,
	.driver = {
		.name = "ambds",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *ambds_device;
static int __init ambds_init(void)
{
	int rval = 0;

	if (spi) {
		ambds_device = ambpriv_create_bundle(&ambds_driver, NULL, -1, NULL, -1);
		if (IS_ERR(ambds_device))
			rval = PTR_ERR(ambds_device);
	} else { /* I2C */
		int bus, addr;

		bus = bus_addr >> 16;
		addr = bus_addr & 0xffff;

		rval = ambpriv_i2c_update_addr("ambds", bus, addr);
		if (rval < 0)
			return rval;

		rval = i2c_add_driver(&i2c_driver_ambds);
	}

	return rval;
}

static void __exit ambds_exit(void)
{
	if (spi) {
		ambpriv_device_unregister(ambds_device);
		ambpriv_driver_unregister(&ambds_driver);
	} else { /* I2C */
		i2c_del_driver(&i2c_driver_ambds);
	}
}

module_init(ambds_init);
module_exit(ambds_exit);

MODULE_DESCRIPTION("AMBDS 4096x2160, AMBA DUMMY SENSOR");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

