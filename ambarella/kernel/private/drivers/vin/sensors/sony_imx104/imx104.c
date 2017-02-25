/*
 * kernel/private/drivers/ambarella/vin/sensors/sony_imx104/imx104.c
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
#include <plat/spi.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx104.h"

static int bus_addr = 0;
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and cs: bit16~bit31: bus, bit0~bit15: cs");

struct imx104_priv {
	u32 spi_bus;
	u32 spi_cs;
	u32 frame_length_lines;
	u32 line_length;
};

#include "imx104_table.c"

static int imx104_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	u8 pbuf[3];
	amba_spi_cfg_t config;
	amba_spi_write_t write;
	struct imx104_priv *imx104;

	imx104 = (struct imx104_priv *)vdev->priv;

	pbuf[0] = (subaddr & 0xff00) >> 8;;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = data & 0xff;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 3;
	write.cs_id = imx104->spi_cs;
	write.bus_id = imx104->spi_bus;

	ambarella_spi_write(&config, &write);

	return 0;
}

static int imx104_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	u8 pbuf[3], tmp;
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;
	struct imx104_priv *imx104;

	imx104 = (struct imx104_priv *)vdev->priv;

	pbuf[0] = ((subaddr & 0xff00) >> 8) | 0x80;
	pbuf[1] = subaddr & 0xff;

	config.cfs_dfs = 8;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.w_buffer = pbuf;
	write.w_size = 2;
	write.r_buffer = &tmp;
	write.r_size = 1;
	write.cs_id = imx104->spi_cs;
	write.bus_id = imx104->spi_bus;

	ambarella_spi_write_then_read(&config, &write);

	/* reverse the bit, LSB first */
	*data = tmp;

	return 0;
}

/*
 * It is advised by sony, that they only guarantee the sync code. However, if we
 * want to use external sync signal, use the DCK sync mode. Try to use sync code
 * first!!!
 */
static int imx104_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config imx104_config;

	memset(&imx104_config, 0, sizeof (imx104_config));

	imx104_config.interface_type = SENSOR_SERIAL_LVDS;
	imx104_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	imx104_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	imx104_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;

	imx104_config.cap_win.x = format->def_start_x;
	imx104_config.cap_win.y = format->def_start_y;
	imx104_config.cap_win.width = format->def_width;
	imx104_config.cap_win.height = format->def_height;

	imx104_config.sensor_id	= GENERIC_SENSOR;
	imx104_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx104_config.bayer_pattern = format->bayer_pattern;
	imx104_config.video_format = format->format;
	imx104_config.bit_resolution = format->bits;
	imx104_config.readout_mode = format->readout_mode;

	return ambarella_set_vin_config(vdev, &imx104_config);
}

static void imx104_sw_reset(struct vin_device *vdev)
{
	imx104_write_reg(vdev, IMX104_SWRESET, 0x01);
	imx104_write_reg(vdev, IMX104_STANDBY, 0x01);/* Standby */
	msleep(10);
}

static void imx104_start_streaming(struct vin_device *vdev)
{
	imx104_write_reg(vdev, IMX104_STANDBY, 0x00); //cancel standby
	msleep(10);
	/* master mode */
	imx104_write_reg(vdev, IMX104_XMSTA, 0x00);//master mode
}

static int imx104_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	imx104_sw_reset(vdev);

	regs = imx104_share_regs;
	regs_num = ARRAY_SIZE(imx104_share_regs);

	for (i = 0; i < regs_num; i++)
		imx104_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx104_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_mid, val_low;
	struct imx104_priv *pinfo = (struct imx104_priv *)vdev->priv;

	imx104_read_reg(vdev, IMX104_HMAX_MSB, &val_high);
	imx104_read_reg(vdev, IMX104_HMAX_LSB, &val_low);
	pinfo->line_length = (val_high << 8) + val_low;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	imx104_read_reg(vdev, IMX104_VMAX_HSB, &val_high);
	imx104_read_reg(vdev, IMX104_VMAX_MSB, &val_mid);
	imx104_read_reg(vdev, IMX104_VMAX_LSB, &val_low);
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + (val_mid << 8) + val_low;

	return 0;
}

static int imx104_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx104_priv *pinfo = (struct imx104_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx104_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx104_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(imx104_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		imx104_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = imx104_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	imx104_get_line_time(vdev);

	/* TG reset release ( Enable Streaming )*/
	imx104_start_streaming(vdev);

	/* communiate with IAV */
	rval = imx104_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int imx104_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx104_priv *pinfo = (struct imx104_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	min_line = 1;
	max_line = pinfo->frame_length_lines;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = pinfo->frame_length_lines - num_line;
	imx104_write_reg(vdev, IMX104_SHS1_MSB, blank_lines >> 8);
	imx104_write_reg(vdev, IMX104_SHS1_LSB, blank_lines & 0xff);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx104_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx104_priv *pinfo = (struct imx104_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = imx104_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx104_set_fps( struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct imx104_priv *pinfo = (struct imx104_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	imx104_write_reg(vdev, IMX104_VMAX_HSB, (v_lines & 0x030000) >> 16);
	imx104_write_reg(vdev, IMX104_VMAX_MSB, (v_lines & 0x00FF00) >> 8);
	imx104_write_reg(vdev, IMX104_VMAX_LSB, v_lines & 0x0000FF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int imx104_set_agc_index( struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX104_GAIN_42DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, IMX104_GAIN_42DB);
		agc_idx = IMX104_GAIN_42DB;
	}

	imx104_write_reg(vdev, IMX104_GAIN, agc_idx);

	return 0;
}

static struct vin_ops imx104_ops = {
	.init_device		= imx104_init_device,
	.set_format		= imx104_set_format,
	.set_shutter_row	= imx104_set_shutter_row,
	.shutter2row		= imx104_shutter2row,
	.set_frame_rate	= imx104_set_fps,
	.set_agc_index	= imx104_set_agc_index,
	.read_reg			= imx104_read_reg,
	.write_reg		= imx104_write_reg,
};

static int imx104_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx104_priv *imx104;

	vdev = ambarella_vin_create_device(ambdev->name,
			SENSOR_IMX104, 	sizeof(struct imx104_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2A000000;	// 42dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x004CCCCC;	// 0.3dB

	imx104 = (struct imx104_priv *)vdev->priv;
	imx104->spi_bus = bus_addr >> 16;
	imx104->spi_cs = bus_addr & 0xffff;

	rval = ambarella_vin_register_device(vdev, &imx104_ops,
			imx104_formats, ARRAY_SIZE(imx104_formats),
			imx104_plls, ARRAY_SIZE(imx104_plls));
	if (rval < 0) {
		ambarella_vin_free_device(vdev);
		return rval;
	}

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("IMX104 init(4-lane lvds)\n");

	return 0;
}

static int imx104_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver imx104_driver = {
	.probe = imx104_drv_probe,
	.remove = imx104_drv_remove,
	.driver = {
		.name = "imx104",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *imx104_device;
static int __init imx104_init(void)
{
	int rval = 0;

	imx104_device = ambpriv_create_bundle(&imx104_driver, NULL, -1, NULL, -1);

	if (IS_ERR(imx104_device))
		rval = PTR_ERR(imx104_device);

	return rval;
}

static void __exit imx104_exit(void)
{
	ambpriv_device_unregister(imx104_device);
	ambpriv_driver_unregister(&imx104_driver);
}

module_init(imx104_init);
module_exit(imx104_exit);

MODULE_DESCRIPTION("IMX104 1/3 -Inch, 1280x1024, 1.31-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Long Zhao, <longzhao@ambarella.com>");
MODULE_LICENSE("Proprietary");

