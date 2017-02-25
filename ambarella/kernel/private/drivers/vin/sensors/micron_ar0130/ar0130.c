/*
 * Filename : ar0130.c
 *
 * History:
 *    2012/05/23 - [Long Zhao] Create
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
#include "ar0130.h"

static int bus_addr = (0 << 16) | (0x30 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct ar0130_priv {
	void *control_data;
	u32 frame_length_lines;
	u32 line_length;
};

#include "ar0130_table.c"

static int ar0130_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ar0130_priv *ar0130;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	ar0130 = (struct ar0130_priv *)vdev->priv;
	client = ar0130->control_data;

	pbuf[0] = (subaddr & 0xff00) >> 8;
	pbuf[1] = subaddr & 0xff;
	pbuf[2] = (data & 0xff00) >> 8;
	pbuf[3] = data & 0xff;

	msgs[0].len = 4;
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

static int ar0130_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ar0130_priv *ar0130;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	ar0130 = (struct ar0130_priv *)vdev->priv;
	client = ar0130->control_data;

	pbuf0[0] = (subaddr & 0xff00) >> 8;
	pbuf0[1] = subaddr & 0xff;

	msgs[0].len = 2;
	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].buf = pbuf0;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].buf = pbuf;
	msgs[1].len = 2;

	rval = i2c_transfer(client->adapter, msgs, 2);
	if (rval < 0){
		vin_error("failed(%d): [0x%x]\n", rval, subaddr);
		return rval;
	}

	*data = (pbuf[0] << 8) | pbuf[1];

	return 0;
}

static int ar0130_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ar0130_config;

	memset(&ar0130_config, 0, sizeof (ar0130_config));

	ar0130_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ar0130_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	ar0130_config.input_mode = SENSOR_RGB_1PIX;

	ar0130_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	ar0130_config.plvcmos_cfg.data_edge = SENSOR_DATA_FALLING_EDGE;
	ar0130_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	ar0130_config.cap_win.x = format->def_start_x;
	ar0130_config.cap_win.y = format->def_start_y;
	ar0130_config.cap_win.width = format->def_width;
	ar0130_config.cap_win.height = format->def_height;

	ar0130_config.sensor_id	= GENERIC_SENSOR;
	ar0130_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ar0130_config.bayer_pattern	= format->bayer_pattern;
	ar0130_config.video_format	= format->format;
	ar0130_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ar0130_config);
}

static void ar0130_sw_reset(struct vin_device *vdev)
{
	ar0130_write_reg(vdev, AR0130_RESET_REGISTER, 0x0001);//Register RESET_REGISTER
	msleep(10);
	ar0130_write_reg(vdev, AR0130_RESET_REGISTER, 0x10D8);//Register RESET_REGISTER
}

static void ar0130_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ar0130_share_regs_linear_mode;
	regs_num = ARRAY_SIZE(ar0130_share_regs_linear_mode);
	for (i = 0; i < regs_num; i++) {
		if (regs[i].addr == 0xffff) {
			break;
		} else if (regs[i].addr == 0xff00) {
			msleep(regs[i].data);
		} else {
			ar0130_write_reg(vdev, regs[i].addr, regs[i].data);
		}
	}
}

static int ar0130_init_device(struct vin_device *vdev)
{
	ar0130_sw_reset(vdev);

	ar0130_fill_share_regs(vdev);

	return 0;
}

static int ar0130_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = ar0130_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ar0130_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		ar0130_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void ar0130_start_streaming(struct vin_device *vdev)
{
	u32 data;
	ar0130_read_reg(vdev, AR0130_RESET_REGISTER, &data);
	data = (data | 0x0004);
	ar0130_write_reg(vdev, AR0130_RESET_REGISTER, data);//start streaming
}

static int ar0130_update_hv_info(struct vin_device *vdev)
{
	struct ar0130_priv *pinfo = (struct ar0130_priv *)vdev->priv;

	ar0130_read_reg(vdev, AR0130_LINE_LENGTH_PCK, &pinfo->line_length);
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ar0130_read_reg(vdev, AR0130_FRAME_LENGTH_LINES, &pinfo->frame_length_lines);

	return 0;
}

static int ar0130_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ar0130_priv *pinfo = (struct ar0130_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ar0130_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num, rval;

	regs = ar0130_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ar0130_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		ar0130_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ar0130_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ar0130_get_line_time(vdev);

	/* Enable Streaming */
	ar0130_start_streaming(vdev);

	/* communiate with IAV */
	rval = ar0130_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ar0130_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	int errCode = 0;
	struct ar0130_priv *pinfo = (struct ar0130_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ Frame format(V) */
	min_line = 1;
	max_line = pinfo->frame_length_lines;
	num_line = clamp(num_line, min_line, max_line);

	ar0130_write_reg(vdev, AR0130_COARSE_INTEGRATION_TIME, num_line);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int ar0130_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ar0130_priv *pinfo = (struct ar0130_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ar0130_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ar0130_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct ar0130_priv *pinfo = (struct ar0130_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	ar0130_write_reg(vdev, AR0130_FRAME_LENGTH_LINES, v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ar0130_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > AR0130_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, AR0130_GAIN_0DB);
		agc_idx = AR0130_GAIN_0DB;
	}

	agc_idx = AR0130_GAIN_0DB - agc_idx;

	ar0130_write_reg(vdev, AR0130_AE_CTRL_REG,
		AR0130_GLOBAL_GAIN_TABLE[agc_idx][AR0130_GAIN_COL_DCG]);
	ar0130_write_reg(vdev, AR0130_DIGITAL_TEST,
		AR0130_GLOBAL_GAIN_TABLE[agc_idx][AR0130_GAIN_COL_COL_GAIN]);
	ar0130_write_reg(vdev, AR0130_GLOBAL_GAIN,
		AR0130_GLOBAL_GAIN_TABLE[agc_idx][AR0130_GAIN_COL_DIGITAL_GAIN]);

	return 0;
}

static int ar0130_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = AR0130_H_MIRROR + AR0130_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = AR0130_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = AR0130_V_FLIP;
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

	ar0130_read_reg(vdev, AR0130_READ_MODE, &tmp_reg);
	tmp_reg &= (~AR0130_MIRROR_MASK);
	tmp_reg |= readmode;
	ar0130_write_reg(vdev, AR0130_READ_MODE, tmp_reg);

	ar0130_write_reg(vdev, AR0130_RESET_REGISTER, 0x01d8);/* stop streaming */

	/* enable column_correction when mirror mode is within H_MIRROR */
	ar0130_read_reg(vdev, 0x30d4, &tmp_reg);
	tmp_reg &= ~AR0130_COLUMN_CORRECTION;
	ar0130_write_reg(vdev, 0x30d4, tmp_reg);
	msleep(50);
	tmp_reg |= AR0130_COLUMN_CORRECTION;
	ar0130_write_reg(vdev, 0x30d4, tmp_reg);

	ar0130_write_reg(vdev, AR0130_RESET_REGISTER, 0x01dc);/* start streaming */

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops ar0130_ops = {
	.init_device		= ar0130_init_device,
	.set_pll		= ar0130_set_pll,
	.set_format		= ar0130_set_format,
	.set_shutter_row = ar0130_set_shutter_row,
	.shutter2row = ar0130_shutter2row,
	.set_frame_rate		= ar0130_set_fps,
	.set_agc_index		= ar0130_set_agc_index,
	.set_mirror_mode	= ar0130_set_mirror_mode,
	.read_reg		= ar0130_read_reg,
	.write_reg		= ar0130_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ar0130_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	int i;
	struct vin_device *vdev;
	struct ar0130_priv *ar0130;
	u32 version;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_AR0130, sizeof(struct ar0130_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_1280_960;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2D000000;	// 45dB
	vdev->agc_db_min = 0x00000000;	// 0dB
	vdev->agc_db_step = 0x00300000;	// 0.1875dB

	i2c_set_clientdata(client, vdev);

	ar0130 = (struct ar0130_priv *)vdev->priv;
	ar0130->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(ar0130_formats); i++)
				ar0130_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &ar0130_ops,
			ar0130_formats, ARRAY_SIZE(ar0130_formats),
			ar0130_plls, ARRAY_SIZE(ar0130_plls));
	if (rval < 0)
		goto ar0130_probe_err;

	ambarella_vin_add_precise_fps(vdev,
		ar0130_precise_fps, ARRAY_SIZE(ar0130_precise_fps));

	/* query sensor id */
	ar0130_read_reg(vdev, AR0130_CHIP_VERSION_REG, &version);
	vin_info("AR0130 init(parallel), sensor ID: 0x%x\n", version);

	return 0;

ar0130_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ar0130_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ar0130_idtable[] = {
	{ "ar0130", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ar0130_idtable);

static struct i2c_driver i2c_driver_ar0130 = {
	.driver = {
		.name	= "ar0130",
	},

	.id_table	= ar0130_idtable,
	.probe		= ar0130_probe,
	.remove		= ar0130_remove,

};

static int __init ar0130_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ar0130", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ar0130);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ar0130_exit(void)
{
	i2c_del_driver(&i2c_driver_ar0130);
}

module_init(ar0130_init);
module_exit(ar0130_exit);

MODULE_DESCRIPTION("AR0130 1/3.2-Inch 3.4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

