/*
 * Filename : ov9710.c
 *
 * History:
 *    2014/11/11 - [Hao Zeng] Create
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
#include "ov9710.h"

static int bus_addr = (0 << 16) | (0x60 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct ov9710_priv {
	void *control_data;
	u32 line_length;
	u32 frame_length_lines;
};

#include "ov9710_table.c"

static int ov9710_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct ov9710_priv *ov9710;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	ov9710 = (struct ov9710_priv *)vdev->priv;
	client = ov9710->control_data;

	pbuf[0] = subaddr;
	pbuf[1] = data;

	msgs[0].len = 2;
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

static int ov9710_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct ov9710_priv *ov9710;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf[1];
	u8 pbuf0[1];

	ov9710 = (struct ov9710_priv *)vdev->priv;
	client = ov9710->control_data;

	pbuf0[0] = subaddr;

	msgs[0].len = 1;
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

static int ov9710_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ov9710_config;

	memset(&ov9710_config, 0, sizeof (ov9710_config));

	ov9710_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ov9710_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	ov9710_config.input_mode = SENSOR_RGB_1PIX;

	ov9710_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	ov9710_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;
	ov9710_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	ov9710_config.cap_win.x = format->def_start_x;
	ov9710_config.cap_win.y = format->def_start_y;
	ov9710_config.cap_win.width = format->def_width;
	ov9710_config.cap_win.height = format->def_height;

	ov9710_config.sensor_id	= GENERIC_SENSOR;
	ov9710_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	ov9710_config.bayer_pattern	= format->bayer_pattern;
	ov9710_config.video_format	= format->format;
	ov9710_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &ov9710_config);
}

static void ov9710_sw_reset(struct vin_device *vdev)
{

}

static void ov9710_fill_share_regs(struct vin_device *vdev)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	/* fill common registers */
	regs = ov9710_share_regs;
	regs_num = ARRAY_SIZE(ov9710_share_regs);
	for (i = 0; i < regs_num; i++)
		ov9710_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int ov9710_init_device(struct vin_device *vdev)
{
	ov9710_sw_reset(vdev);
	ov9710_fill_share_regs(vdev);

	return 0;
}

static int ov9710_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = ov9710_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(ov9710_pll_regs[pll_idx]);

	for (i = 0; i < regs_num; i++)
		ov9710_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void ov9710_start_streaming(struct vin_device *vdev)
{

}

static int ov9710_update_hv_info(struct vin_device *vdev)
{
	u32 data_h, data_l;
	struct ov9710_priv *pinfo = (struct ov9710_priv *)vdev->priv;

	ov9710_read_reg(vdev, OV9710_HMAX_MSB, &data_h);
	ov9710_read_reg(vdev, OV9710_HMAX_LSB, &data_l);
	pinfo->line_length = (data_h<<8) + data_l;
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	ov9710_read_reg(vdev, OV9710_VMAX_MSB, &data_h);
	ov9710_read_reg(vdev, OV9710_VMAX_LSB, &data_l);
	pinfo->frame_length_lines = (data_h<<8) + data_l;

	return 0;
}

static int ov9710_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct ov9710_priv *pinfo = (struct ov9710_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int ov9710_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = ov9710_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(ov9710_mode_regs[format->device_mode]);

	for (i = 0; i < regs_num; i++)
		ov9710_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = ov9710_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	ov9710_get_line_time(vdev);

	/* Enable Streaming */
	ov9710_start_streaming(vdev);

	/* communiate with IAV */
	rval = ov9710_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ov9710_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	u32 num_line, min_line, max_line;
	struct ov9710_priv *pinfo = (struct ov9710_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 2) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 2;
	num_line = clamp(num_line, min_line, max_line);

	ov9710_write_reg(vdev, OV9710_EXPO0_MSB, (num_line >> 8) & 0xFF);
	ov9710_write_reg(vdev, OV9710_EXPO0_LSB, num_line & 0xFF);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int ov9710_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct ov9710_priv *pinfo = (struct ov9710_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = ov9710_update_hv_info(vdev);
		if (rval < 0)
			return rval;

		ov9710_get_line_time(vdev);
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int ov9710_set_fps(struct vin_device *vdev, int fps)
{
	u64 v_lines, vb_time;
	struct ov9710_priv *pinfo = (struct ov9710_priv *)vdev->priv;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	ov9710_write_reg(vdev, OV9710_VMAX_MSB, (v_lines >> 8) & 0xFF);
	ov9710_write_reg(vdev, OV9710_VMAX_LSB, v_lines & 0xFF);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int ov9710_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > OV9710_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, OV9710_GAIN_0DB);
		agc_idx = OV9710_GAIN_0DB;
	}

	agc_idx = OV9710_GAIN_0DB - agc_idx;

	ov9710_write_reg(vdev, OV9710_GAIN, OV9710_GLOBAL_GAIN_TABLE[agc_idx][OV9710_GAIN_COL_REG]);

	return 0;
}

static int ov9710_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg0, tmp_reg1, readmode, bayer_pattern;
	u32 vstartLSBs;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		vstartLSBs = 0x0A;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = OV9710_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		vstartLSBs = 0x01;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = OV9710_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		vstartLSBs = 0x00;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = (OV9710_H_MIRROR | OV9710_V_FLIP);
		bayer_pattern = VINDEV_BAYER_PATTERN_BG;
		vstartLSBs = 0x01;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	ov9710_read_reg(vdev, OV9710_MIRROR_MODE, &tmp_reg0);
	tmp_reg0 |= 0x01; /* group latch enable */
	ov9710_write_reg(vdev, OV9710_MIRROR_MODE, tmp_reg0);
	tmp_reg0 &= ~(OV9710_H_MIRROR | OV9710_V_FLIP);
	tmp_reg0 |= readmode;

	/* when using flip function, the starting line of VREF must be adjusted */
	ov9710_read_reg(vdev, OV9710_REG03, &tmp_reg1);
	tmp_reg1 &= (~0x0F);
	tmp_reg1 |= vstartLSBs;

	ov9710_write_reg(vdev, OV9710_MIRROR_MODE, tmp_reg0);
	ov9710_write_reg(vdev, OV9710_REG03, tmp_reg1);
	ov9710_write_reg(vdev, 0xC3, 0x21); /* restart to output frame */
	ov9710_write_reg(vdev, 0xFF, 0xFF); /* latch trigger */

	msleep(DIV64_CLOSEST(1000 * vdev->frame_rate, 512000000)); /* delay 1 frame to cancel group latch*/

	ov9710_read_reg(vdev, OV9710_MIRROR_MODE, &tmp_reg0);
	tmp_reg0 &= 0xFE; /* group latch disable */
	ov9710_write_reg(vdev, OV9710_MIRROR_MODE, tmp_reg0);
	ov9710_write_reg(vdev, 0xC3, 0x21); /* restart to output frame */
	ov9710_write_reg(vdev, 0xFF, 0xFF); /* latch trigger */

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int ov9710_query_sensor_id(struct vin_device *vdev, u16 *ss_id)
{
	u32 pidh = 0, pidl = 0;

	ov9710_read_reg(vdev, OV9710_PIDH, &pidh);
	ov9710_read_reg(vdev, OV9710_PIDL, &pidl);
	*ss_id = (u16)((pidh << 8) | pidl);

	return 0;
}

static struct vin_ops ov9710_ops = {
	.init_device		= ov9710_init_device,
	.set_pll			= ov9710_set_pll,
	.set_format		= ov9710_set_format,
	.set_shutter_row	= ov9710_set_shutter_row,
	.shutter2row		= ov9710_shutter2row,
	.set_frame_rate	= ov9710_set_fps,
	.set_agc_index		= ov9710_set_agc_index,
	.set_mirror_mode	= ov9710_set_mirror_mode,
	.read_reg			= ov9710_read_reg,
	.write_reg			= ov9710_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int ov9710_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	int i;
	u16 sen_id;
	struct vin_device *vdev;
	struct ov9710_priv *ov9710;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_OV9710, sizeof(struct ov9710_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_720P;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x1e000000;	/* 30dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x00600000;	/* 0.375db */

	i2c_set_clientdata(client, vdev);

	ov9710 = (struct ov9710_priv *)vdev->priv;
	ov9710->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(ov9710_formats); i++)
				ov9710_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &ov9710_ops,
			ov9710_formats, ARRAY_SIZE(ov9710_formats),
			ov9710_plls, ARRAY_SIZE(ov9710_plls));
	if (rval < 0)
		goto ov9710_probe_err;

	rval = ov9710_query_sensor_id(vdev, &sen_id);
	if (rval)
		goto ov9710_unreg_dev;
	vin_info("OV9710 sensor ID is 0x%x\n", sen_id);

	vin_info("OV9710 init(parallel)\n");

	return 0;

ov9710_unreg_dev:
	ambarella_vin_unregister_device(vdev);

ov9710_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int ov9710_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id ov9710_idtable[] = {
	{ "ov9710", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov9710_idtable);

static struct i2c_driver i2c_driver_ov9710 = {
	.driver = {
		.name	= "ov9710",
	},

	.id_table	= ov9710_idtable,
	.probe		= ov9710_probe,
	.remove		= ov9710_remove,

};

static int __init ov9710_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("ov9710", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_ov9710);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit ov9710_exit(void)
{
	i2c_del_driver(&i2c_driver_ov9710);
}

module_init(ov9710_init);
module_exit(ov9710_exit);

MODULE_DESCRIPTION("OV9710 1/4 -Inch, 1280x800, 1-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

