/*
 * Filename : mt9t002.c
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
#include <linux/i2c.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "mt9t002.h"

static int bus_addr = (0 << 16) | (0x20 >> 1);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
module_param(bayer_pattern, int, 0644);
MODULE_PARM_DESC(bayer_pattern, "set bayer pattern: 0:RG, 1:BG, 2:GR, 3:GB, 255:default");

struct mt9t002_priv {
	void *control_data;
	u32 frame_length_lines;
	u32 line_length;
};

#include "mt9t002_table.c"

static int mt9t002_write_reg( struct vin_device *vdev, u32 subaddr, u32 data)
{
	int rval;
	struct mt9t002_priv *mt9t002;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[4];

	mt9t002 = (struct mt9t002_priv *)vdev->priv;
	client = mt9t002->control_data;

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

static int mt9t002_read_reg( struct vin_device *vdev, u32 subaddr, u32 *data)
{
	int rval = 0;
	struct mt9t002_priv *mt9t002;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf0[2];
	u8 pbuf[6];

	mt9t002 = (struct mt9t002_priv *)vdev->priv;
	client = mt9t002->control_data;

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

static int mt9t002_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config mt9t002_config;

	memset(&mt9t002_config, 0, sizeof (mt9t002_config));

	mt9t002_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	mt9t002_config.sync_mode = SENSOR_SYNC_MODE_MASTER;
	mt9t002_config.input_mode = SENSOR_RGB_1PIX;

	mt9t002_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	mt9t002_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;
	mt9t002_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	mt9t002_config.cap_win.x = format->def_start_x;
	mt9t002_config.cap_win.y = format->def_start_y;
	mt9t002_config.cap_win.width = format->def_width;
	mt9t002_config.cap_win.height = format->def_height;

	mt9t002_config.sensor_id	= GENERIC_SENSOR;
	mt9t002_config.input_format	= AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	mt9t002_config.bayer_pattern	= format->bayer_pattern;
	mt9t002_config.video_format	= format->format;
	mt9t002_config.bit_resolution	= format->bits;

	return ambarella_set_vin_config(vdev, &mt9t002_config);
}

static void _mt9t002_set_reg_0x301A(struct vin_device *vdev, u32 restart_frame, u32 streaming, u32 reset)
{
	u16 reg_0x301A;

	reg_0x301A =
		1 << 12                 |    /* 1: disable serial interface (HiSPi) */
		0 << 11                 |    /* force_pll_on */
		0 << 10                 |    /* DONT restart if bad frame is detected */
		0 << 9                  |    /* The sensor will produce bad frames as some register changed */
		0 << 8                  |    /* input buffer related to GPI0/1/2/3 inputs are powered down & disabled */
		1 << 7                  |    /* Parallel data interface is enabled (dep on bit[6]) */
		1 << 6                  |    /* Parallel interface is driven */
		1 << 4                  |    /*reset_reg_unused */
		1 << 3                  |    /* Forbids to change value of SMIA registers */
		(streaming>0) << 2      |    /* Put the sensor in streaming mode */
		(restart_frame>0) << 1  |    /* Causes sensor to truncate frame at the end of current row and start integrating next frame */
		(reset>0) << 0;              /* Set the bit initiates a reset sequence */
	/* Typically, in normal streamming mode (group_hold=0, restart_frame=1, standby=0), the value is 0x50DE */
	mt9t002_write_reg(vdev, 0x301A, reg_0x301A);
}

static void mt9t002_sw_reset(struct vin_device *vdev)
{
	_mt9t002_set_reg_0x301A(vdev, 0/*restart_frame*/, 0/*streaming*/, 0/*reset*/); /* stop streaming */
	msleep(10);
}

static void mt9t002_fill_share_regs(struct vin_device *vdev, u32 otpm_ver)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;
	u32  reg_val;

	/* fill registers for each specific version mt9t002 */
	switch(otpm_ver) {
	case 1:	/* AR0330 Rev1, OTPM_Ver1 */
		regs_num = ARRAY_SIZE(mt9t002_rev1_regs);
		regs = mt9t002_rev1_regs;
		break;
	case 2:	/* AR0330 Rev2.0, OTPM_Ver2  */
		regs_num = ARRAY_SIZE(mt9t002_rev2_regs);
		regs = mt9t002_rev2_regs;
		break;
	case 3:	/* AR0330 Rev2.1, OTPM_Ver3 */
		regs_num = ARRAY_SIZE(mt9t002_rev3_regs);
		regs = mt9t002_rev3_regs;
		break;
	case 4:	/* AR0330 Rev2.1, OTPM_Ver4 */
		regs_num = ARRAY_SIZE(mt9t002_rev4_regs);
		regs = mt9t002_rev4_regs;
		break;
	case 5:	/* AR0330 Rev2.1, OTPM_Ver5 */
		regs_num = ARRAY_SIZE(mt9t002_rev5_regs);
		regs = mt9t002_rev5_regs;
		break;
	default:
		regs_num = 0;
		vin_info("Unsupported OTPM version, update driver?\n");
		return;
	}

	for (i = 0; i < regs_num; i++) {
		if(unlikely(regs[i].addr == 0x3ECE)){
			mt9t002_read_reg(vdev, 0x3ECE, &reg_val);
			reg_val |= 0x00FF;
			mt9t002_write_reg(vdev, 0x3ECE, reg_val);
		} else {
			mt9t002_write_reg(vdev, regs[i].addr, regs[i].data);
		}
	}

	/* fill common registers */
	regs = mt9t002_share_regs;
	regs_num = ARRAY_SIZE(mt9t002_share_regs);
	for (i = 0; i < regs_num; i++)
		mt9t002_write_reg(vdev, regs[i].addr, regs[i].data);
}

static int mt9t002_init_device(struct vin_device *vdev)
{
	int otpm_ver = 0, otpm_ver1, otpm_ver2;

	mt9t002_sw_reset(vdev);

	/* query sensor version */
	mt9t002_read_reg(vdev, 0x30F0, &otpm_ver1);
	if (otpm_ver1 < 0)
		return -EIO;

	mt9t002_read_reg(vdev, 0x3072, &otpm_ver2);
	if (otpm_ver2 < 0)
		return -EIO;

	/* FIXME, update it if there is new version */
	if (otpm_ver1 == 0x1200) {
		otpm_ver = 1;
	} else if (otpm_ver1 == 0x1208) {
		switch(otpm_ver2){
		case 0x0:
			otpm_ver = 2;
			break;
		case 0x6:
			otpm_ver = 3;
			break;
		case 0x7:
			otpm_ver = 4;
			break;
		case 0x8:
			otpm_ver = 5;
			break;
		default:
			vin_error("Unsupported OTPM version 0x%x:0x%x, "
				"update driver?\n", otpm_ver1, otpm_ver2);
			return -ENODEV;
		}
	}

	vin_info("OTPM version: %d\n", otpm_ver);

	mt9t002_fill_share_regs(vdev, otpm_ver);

	return 0;
}

static int mt9t002_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num;

	regs = mt9t002_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(mt9t002_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		mt9t002_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static void mt9t002_start_streaming(struct vin_device *vdev)
{
	_mt9t002_set_reg_0x301A(vdev, 0/*restart_frame*/, 1/*streaming*/, 0/*reset*/); /*Enable Streaming */
}

static int mt9t002_update_hv_info(struct vin_device *vdev)
{
	struct mt9t002_priv *pinfo = (struct mt9t002_priv *)vdev->priv;

	mt9t002_read_reg(vdev, MT9T002_LINE_LENGTH_PCK, &pinfo->line_length);
	if(unlikely(!pinfo->line_length)) {
		vin_error("line length is 0!\n");
		return -EIO;
	}

	mt9t002_read_reg(vdev, MT9T002_FRAME_LENGTH_LINES, &pinfo->frame_length_lines);

	return 0;
}

static int mt9t002_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct mt9t002_priv *pinfo = (struct mt9t002_priv *)vdev->priv;

	h_clks = (u64)pinfo->line_length * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int mt9t002_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_reg_16_16 *regs;
	int i, regs_num, rval;

	regs = mt9t002_mode_regs[format->device_mode];
	regs_num = ARRAY_SIZE(mt9t002_mode_regs[format->device_mode]);
	for (i = 0; i < regs_num; i++)
		mt9t002_write_reg(vdev, regs[i].addr, regs[i].data);

	rval = mt9t002_update_hv_info(vdev);
	if (rval < 0)
		return rval;

	mt9t002_get_line_time(vdev);

	/* Enable Streaming */
	mt9t002_start_streaming(vdev);

	/* communiate with IAV */
	rval = mt9t002_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int mt9t002_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u64 exposure_lines;
	int errCode = 0;
	u32 num_line, min_line, max_line;
	struct mt9t002_priv *pinfo = (struct mt9t002_priv *)vdev->priv;

	num_line = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 1) */
	min_line = 1;
	max_line = pinfo->frame_length_lines - 1;
	num_line = clamp(num_line, min_line, max_line);

	mt9t002_write_reg(vdev, MT9T002_COARSE_INTEGRATION_TIME, num_line);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)pinfo->line_length * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return errCode;
}

static int mt9t002_shutter2row(struct vin_device *vdev, u32* shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct mt9t002_priv *pinfo = (struct mt9t002_priv *)vdev->priv;

	/* for fast boot, it may call set shutter time directly, so we must read line length/frame line */
	if(unlikely(!pinfo->line_length)) {
		rval = mt9t002_update_hv_info(vdev);
		if (rval < 0)
			return rval;
	}

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, pinfo->line_length);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}


static int mt9t002_set_fps(struct vin_device *vdev, int fps)
{
	u64 pixelclk, v_lines, vb_time;
	struct mt9t002_priv *pinfo = (struct mt9t002_priv *)vdev->priv;

	pixelclk = vdev->cur_pll->pixelclk;

	v_lines = fps * pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, pinfo->line_length);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);
	mt9t002_write_reg(vdev, MT9T002_FRAME_LENGTH_LINES, v_lines);

	pinfo->frame_length_lines = (u32)v_lines;

	vb_time = pinfo->line_length * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, pixelclk);
	vdev->cur_format->vb_time = vb_time;

	return 0;
}

static int mt9t002_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > MT9T002_GAIN_0DB) {
		vin_warn("agc index %d exceeds maximum %d\n", agc_idx, MT9T002_GAIN_0DB);
		agc_idx = MT9T002_GAIN_0DB;
	}

	agc_idx = MT9T002_GAIN_0DB - agc_idx;

	mt9t002_write_reg(vdev, 0x3060,
		mt9t002_gains[agc_idx][MT9T002_GAIN_COL_REG_AGAIN]);

	mt9t002_write_reg(vdev, 0x305E,
		mt9t002_gains[agc_idx][MT9T002_GAIN_COL_REG_DGAIN]);

	return 0;
}

static int mt9t002_set_mirror_mode(struct vin_device *vdev,
		struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = MT9T002_H_MIRROR + MT9T002_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
		readmode = MT9T002_H_MIRROR;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = MT9T002_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_GR;
		break;

	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	mt9t002_read_reg(vdev, 0x3040, &tmp_reg);
	tmp_reg &= (~MT9T002_MIRROR_MASK);
	tmp_reg |= readmode;
	mt9t002_write_reg(vdev, 0x3040, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static struct vin_ops mt9t002_ops = {
	.init_device		= mt9t002_init_device,
	.set_pll		= mt9t002_set_pll,
	.set_format		= mt9t002_set_format,
	.set_shutter_row	= mt9t002_set_shutter_row,
	.shutter2row		= mt9t002_shutter2row,
	.set_frame_rate		= mt9t002_set_fps,
	.set_agc_index		= mt9t002_set_agc_index,
	.set_mirror_mode	= mt9t002_set_mirror_mode,
	.read_reg		= mt9t002_read_reg,
	.write_reg		= mt9t002_write_reg,
};

/*	< include init.c here for aptina sensor, which is produce by perl >  */
/* ========================================================================== */
static int mt9t002_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	int i;
	struct vin_device *vdev;
	struct mt9t002_priv *mt9t002;
	u32 version;

	vdev = ambarella_vin_create_device(client->name,
			SENSOR_MT9T002, sizeof(struct mt9t002_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_2304_1296;
	vdev->default_hdr_mode = AMBA_VIDEO_LINEAR_MODE;
	vdev->agc_db_max = 0x2A000000;	/* 42dB */
	vdev->agc_db_min = 0x00000000;	/* 0dB */
	vdev->agc_db_step = 0x000c0000;/* 0.046875dB */
	vdev->pixel_size = 0x00023333;	/* 2.2um */

	i2c_set_clientdata(client, vdev);

	mt9t002 = (struct mt9t002_priv *)vdev->priv;
	mt9t002->control_data = client;

	if (bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		if (bayer_pattern > VINDEV_BAYER_PATTERN_GB) {
			vin_error("invalid bayer pattern:%d\n", bayer_pattern);
			return -EINVAL;
		} else {
			for (i = 0; i < ARRAY_SIZE(mt9t002_formats); i++)
				mt9t002_formats[i].default_bayer_pattern = bayer_pattern;
		}
	}

	rval = ambarella_vin_register_device(vdev, &mt9t002_ops,
			mt9t002_formats, ARRAY_SIZE(mt9t002_formats),
			mt9t002_plls, ARRAY_SIZE(mt9t002_plls));
	if (rval < 0)
		goto mt9t002_probe_err;

	ambarella_vin_add_precise_fps(vdev,
		mt9t002_precise_fps, ARRAY_SIZE(mt9t002_precise_fps));

	/* query sensor id */
	mt9t002_read_reg(vdev, MT9T002_CHIP_VERSION_REG, &version);
	vin_info("MT9T002 init(parallel), sensor ID: 0x%x\n", version);

	return 0;

mt9t002_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int mt9t002_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id mt9t002_idtable[] = {
	{ "mt9t002", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mt9t002_idtable);

static struct i2c_driver i2c_driver_mt9t002 = {
	.driver = {
		.name	= "mt9t002",
	},

	.id_table	= mt9t002_idtable,
	.probe		= mt9t002_probe,
	.remove		= mt9t002_remove,

};

static int __init mt9t002_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("mt9t002", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_mt9t002);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit mt9t002_exit(void)
{
	i2c_del_driver(&i2c_driver_mt9t002);
}

module_init(mt9t002_init);
module_exit(mt9t002_exit);

MODULE_DESCRIPTION("MT9T002 1/3.2-Inch 3.4-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Haowei Lo, <hwlo@ambarella.com>");
MODULE_LICENSE("Proprietary");

