/*
 * Filename : mn34041pl.c
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
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
#include "mn34041pl.h"

static int bus_addr = 0;
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and cs: bit16~bit31: bus, bit0~bit15: cs");

struct mn34041pl_priv {
	u32 spi_bus;
	u32 spi_cs;
	u32 line_length;
	u32 frame_length_lines;
};

#include "mn34041pl_table.c"

static int mn34041pl_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	u8 pbuf[4];
	amba_spi_cfg_t config;
	amba_spi_write_t write;
	struct mn34041pl_priv *mn34041pl;

	mn34041pl = (struct mn34041pl_priv *)vdev->priv;

	pbuf[0] = subaddr & 0xff;
	pbuf[1] = (subaddr & 0xff00) >> 8;
	pbuf[2] = data & 0xff;
	pbuf[3] = (data & 0xff00) >> 8;

	config.cfs_dfs = 16;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 4;
	write.cs_id = mn34041pl->spi_cs;
	write.bus_id = mn34041pl->spi_bus;

	ambarella_spi_write(&config, &write);

	return 0;
}

static int mn34041pl_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	u8 pbuf[2], tmp[2];
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;
	struct mn34041pl_priv *mn34041pl;

	mn34041pl = (struct mn34041pl_priv *)vdev->priv;

	pbuf[0] = subaddr & 0xff;
	pbuf[1] = ((subaddr & 0xff00) >> 8) | 0x80;

	config.cfs_dfs = 16;//bits
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.w_buffer = pbuf;
	write.w_size = 2;
	write.r_buffer = tmp;
	write.r_size = 2;
	write.cs_id = mn34041pl->spi_cs;
	write.bus_id = mn34041pl->spi_bus;

	ambarella_spi_write_then_read(&config, &write);

	*data = ((u16 *)tmp)[0];

	return 0;
}

static int mn34041pl_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config mn34041pl_config;

	memset(&mn34041pl_config, 0, sizeof (mn34041pl_config));

	mn34041pl_config.interface_type = SENSOR_SERIAL_LVDS;
	mn34041pl_config.sync_mode = SENSOR_SYNC_MODE_MASTER;

	mn34041pl_config.slvds_cfg.lane_number = SENSOR_4_LANE;
	mn34041pl_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_ITU656;

	mn34041pl_config.cap_win.x = format->def_start_x;
	mn34041pl_config.cap_win.y = format->def_start_y;
	mn34041pl_config.cap_win.width = format->def_width;
	mn34041pl_config.cap_win.height = format->def_height;

	mn34041pl_config.sensor_id	= GENERIC_SENSOR;
	mn34041pl_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	mn34041pl_config.bayer_pattern	= format->bayer_pattern;
	mn34041pl_config.video_format	= format->format;
	mn34041pl_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &mn34041pl_config);
}

static void mn34041pl_sw_reset(struct vin_device *vdev)
{
	mn34041pl_write_reg(vdev, 0x0000, 0x0000);	//TG reset release
}

static int mn34041pl_init_device(struct vin_device *vdev)
{
	struct vin_reg_16_16 *regs;
	u32 i, regs_num;
	u32 sensor_ver;

	mn34041pl_read_reg(vdev, 0x0036, &sensor_ver);
	if (sensor_ver == 0xFFFF) {
		vin_error("Invalid sensor version!\n");
		return -EIO;
	}

	vin_info("MN34041PL sensor version is 0x%x\n", sensor_ver);

	mn34041pl_sw_reset(vdev);

	/* fill common registers */
	regs = mn34041pl_share_regs;
	regs_num = ARRAY_SIZE(mn34041pl_share_regs);

	for (i = 0; i < regs_num; i++)
		mn34041pl_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}


static int mn34041pl_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = mn34041pl_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(mn34041pl_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		mn34041pl_write_reg(vdev, regs[i].addr, regs[i].data);

	/* FIXME: is it a must to wait? */
	msleep(10);

	return 0;
}

static int mn34041pl_update_hv_info(struct vin_device *vdev)
{
	u32 val_high, val_low;
	struct mn34041pl_priv *pinfo = (struct mn34041pl_priv *)vdev->priv;

	pinfo->line_length = 2400;

	mn34041pl_read_reg(vdev, 0x01A0, &val_low);/* VCYCLE_LSB */
	mn34041pl_read_reg(vdev, 0x01A1, &val_high);/* VCYCLE_MSB */
	pinfo->frame_length_lines = ((val_high & 0x01) << 16) + val_low + 1;

	return 0;
}

static int mn34041pl_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct mn34041pl_priv *pinfo = (struct mn34041pl_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int mn34041pl_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;

	rval = mn34041pl_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	mn34041pl_get_line_time(vdev);

	return mn34041pl_set_vin_mode(vdev, format);
}

static int mn34041pl_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int shutter_width;
	struct mn34041pl_priv *pinfo = (struct mn34041pl_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 0 ~ (Frame format(V) - 3) */
	min_line = 0;
	max_line = pinfo->frame_length_lines - 3;
	num_line = clamp(num_line, min_line, max_line);

	shutter_width = pinfo->frame_length_lines - num_line - 3;/* reg(H) = 1(V)-3+0.6-exposure(H) */

	mn34041pl_write_reg(vdev, 0x00A1, shutter_width & 0xFFFF);
	mn34041pl_write_reg(vdev, 0x00A2, (shutter_width & 0x10000) >> 16);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int mn34041pl_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct mn34041pl_priv *pinfo = (struct mn34041pl_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = mn34041pl_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int mn34041pl_set_fps(struct vin_device *vdev, int fps)
{
	u64	v_lines, vb_time;
	u16 reg_lsb, reg_msb;
	struct mn34041pl_priv *pinfo = (struct mn34041pl_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	reg_msb = (u16)(((v_lines - 1) >> 16) & 0x1);
	reg_lsb = (u16)v_lines - 1;

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	/* mode 1 */
	mn34041pl_write_reg(vdev, 0x01A0, reg_lsb);
	mn34041pl_write_reg(vdev, 0x01A1, reg_msb);
	mn34041pl_write_reg(vdev, 0x01A3, reg_lsb);
	mn34041pl_write_reg(vdev, 0x01A4, reg_msb);
	mn34041pl_write_reg(vdev, 0x01A7, reg_lsb);
	mn34041pl_write_reg(vdev, 0x01A8, reg_msb);
	/* mode 2 */
	mn34041pl_write_reg(vdev, 0x02A0, reg_lsb);
	mn34041pl_write_reg(vdev, 0x02A1, reg_msb);
	mn34041pl_write_reg(vdev, 0x02A3, reg_lsb);
	mn34041pl_write_reg(vdev, 0x02A4, reg_msb);
	mn34041pl_write_reg(vdev, 0x02A7, reg_lsb);
	mn34041pl_write_reg(vdev, 0x02A8, reg_msb);

	return 0;
}

static int mn34041pl_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	u16 val;

	if (agc_idx > MN34041PL_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, MN34041PL_GAIN_0DB);
		agc_idx = MN34041PL_GAIN_0DB;
	}

	agc_idx = MN34041PL_GAIN_0DB - agc_idx;

	val = mn34041pl_gains[agc_idx][MN34041PL_CGAIN_COL] +
			mn34041pl_gains[agc_idx][MN34041PL_AGAIN_COL];
	mn34041pl_write_reg(vdev, 0x0020, val);

	val = mn34041pl_gains[agc_idx][MN34041PL_DGAIN_COL];
	mn34041pl_write_reg(vdev, 0x0021, val);

	return 0;
}

static int mn34041pl_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = MN34041PL_MIRROR_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
	case VINDEV_MIRROR_HORRIZONTALLY:
	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	mn34041pl_read_reg(vdev, 0x00A0, &tmp_reg);
	tmp_reg &= (~MN34041PL_MIRROR_FLIP);
	tmp_reg |= readmode;
	mn34041pl_write_reg(vdev, 0x00A0, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int mn34041pl_get_eis_info(struct vin_device *vdev,
		struct vindev_eisinfo *eis_info)
{
	eis_info->sensor_cell_width = 275;// 2.75 um
	eis_info->sensor_cell_height = 275;// 2.75 um
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;
	eis_info->vb_time = vdev->cur_format->vb_time;

	return 0;
}

static struct vin_ops mn34041pl_ops = {
	.init_device		= mn34041pl_init_device,
	.set_pll		= mn34041pl_set_pll,
	.set_format		= mn34041pl_set_format,
	.set_shutter_row	= mn34041pl_set_shutter_row,
	.shutter2row		= mn34041pl_shutter2row,
	.set_frame_rate		= mn34041pl_set_fps,
	.set_agc_index		= mn34041pl_set_agc_index,
	.set_mirror_mode	= mn34041pl_set_mirror_mode,
	.get_eis_info		= mn34041pl_get_eis_info,
	.read_reg		= mn34041pl_read_reg,
	.write_reg		= mn34041pl_write_reg,
};

static int mn34041pl_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct mn34041pl_priv *mn34041pl;

	vdev = ambarella_vin_create_device(ambdev->name,
			SENSOR_MN34041PL, sizeof(struct mn34041pl_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1080P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2a000000;	// 42dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00180000;	// 0.09375dB

	mn34041pl = (struct mn34041pl_priv *)vdev->priv;
	mn34041pl->spi_bus = bus_addr >> 16;
	mn34041pl->spi_cs = bus_addr & 0xffff;

	rval = ambarella_vin_register_device(vdev, &mn34041pl_ops,
		mn34041pl_formats, ARRAY_SIZE(mn34041pl_formats),
		mn34041pl_plls, ARRAY_SIZE(mn34041pl_plls));
	if (rval < 0)
		goto mn34041pl_probe_err;

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("mn34041pl Init\n");

	return 0;

mn34041pl_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int mn34041pl_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver mn34041pl_driver = {
	.probe = mn34041pl_drv_probe,
	.remove = mn34041pl_drv_remove,
	.driver = {
		.name = "mn34041pl",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *mn34041pl_device;
static int __init mn34041pl_init(void)
{
	int rval = 0;

	mn34041pl_device = ambpriv_create_bundle(&mn34041pl_driver, NULL, -1, NULL, -1);

	if (IS_ERR(mn34041pl_device))
		rval = PTR_ERR(mn34041pl_device);

	return rval;
}

static void __exit mn34041pl_exit(void)
{
	ambpriv_device_unregister(mn34041pl_device);
	ambpriv_driver_unregister(&mn34041pl_driver);
}

module_init(mn34041pl_init);
module_exit(mn34041pl_exit);

MODULE_DESCRIPTION("MN34041PL 1/3-Inch 2.1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

