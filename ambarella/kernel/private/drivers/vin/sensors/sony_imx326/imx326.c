/*
 * kernel/private/drivers/vin/sensors/sony_imx326/imx326.c
 *
 * History:
 *    2016/10/17 - [Hao Zeng] created file
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
#include <plat/spi.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "imx326.h"
#include "imx326_table.c"

#define IMX326_WRITE_ADDR 0x81
#define IMX326_READ_ADDR  0x82

static int bus_addr = 0;
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and cs: bit16~bit31: bus, bit0~bit15: cs");

static int lane = 8;
module_param(lane, int, 0644);
MODULE_PARM_DESC(lane, "Set lvds lane number, 8: 8 lane, 10: 10 lane");

struct imx326_priv {
	struct vindev_wdr_gp_s wdr_again_gp;
	struct vindev_wdr_gp_s wdr_dgain_gp;
	struct vindev_wdr_gp_s wdr_shutter_gp;
	u32 spi_bus;
	u32 spi_cs;
	u32 v_lines;
	u32 h_clks;
	u32 ori_h_clks;
	u32 rhs1;
	u8 lane_num;
};

static int imx326_write_reg(struct vin_device *vdev, u32 subaddr, u32 data)
{
	u8 pbuf[4];
	amba_spi_cfg_t config;
	amba_spi_write_t write;
	struct imx326_priv *imx326;

	imx326 = (struct imx326_priv *)vdev->priv;

	pbuf[0] = IMX326_WRITE_ADDR;
	pbuf[1] = (subaddr & 0xff00) >> 8;
	pbuf[2] = subaddr & 0xff;
	pbuf[3] = data;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 4;
	write.cs_id = imx326->spi_cs;
	write.bus_id = imx326->spi_bus;

	ambarella_spi_write(&config, &write);

	return 0;
}

static int imx326_write_reg2(struct vin_device *vdev, u16 subaddr, u16 data)
{
	u8 pbuf[5];
	amba_spi_cfg_t config;
	amba_spi_write_t write;
	struct imx326_priv *imx326;

	imx326 = (struct imx326_priv *)vdev->priv;

	pbuf[0] = IMX326_WRITE_ADDR;
	pbuf[1] = (subaddr & 0xff00) >> 8;
	pbuf[2] = subaddr & 0xff;
	pbuf[3] = data & 0xff;
	pbuf[4] = data >> 8;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.buffer = pbuf;
	write.n_size = 5;
	write.cs_id = imx326->spi_cs;
	write.bus_id = imx326->spi_bus;

	ambarella_spi_write(&config, &write);

	return 0;
}

static int imx326_read_reg(struct vin_device *vdev, u32 subaddr, u32 *pdata)
{
	u8 pbuf[4], tmp;
	amba_spi_cfg_t config;
	amba_spi_write_then_read_t write;
	struct imx326_priv *imx326;

	imx326 = (struct imx326_priv *)vdev->priv;

	pbuf[0] = IMX326_READ_ADDR;
	pbuf[1] = (subaddr & 0xff00) >> 8;
	pbuf[2] = subaddr&0xFF;

	config.cfs_dfs = 8;
	config.baud_rate = 1000000;
	config.cs_change = 0;
	config.spi_mode = SPI_MODE_3 | SPI_LSB_FIRST;

	write.w_buffer = pbuf;
	write.w_size = 3;
	write.r_buffer = &tmp;
	write.r_size = 1;
	write.cs_id = imx326->spi_cs;
	write.bus_id = imx326->spi_bus;

	ambarella_spi_write_then_read(&config, &write);

	*pdata = tmp;

	return 0;
}

static int imx326_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	struct vin_device_config imx326_config;

	memset(&imx326_config, 0, sizeof(imx326_config));

	imx326_config.interface_type = SENSOR_SERIAL_LVDS;
	imx326_config.sync_mode = SENSOR_SYNC_MODE_SLAVE;

	if (format->hdr_mode == AMBA_VIDEO_LINEAR_MODE) {
		imx326_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY;
	} else {
		imx326_config.slvds_cfg.sync_code_style = SENSOR_SYNC_STYLE_SONY_DOL;
		/* use split width to make VIN divide the long expo lines */
		imx326_config.hdr_cfg.split_width =
			(lane == SENSOR_8_LANE) ? IMX326_H_PERIOD_8LANE : IMX326_H_PERIOD_10LANE;
		//imx326_config.hdr_cfg.num_splits = format->hdr_mode;
	}

	imx326_config.slvds_cfg.lane_number = imx326->lane_num;
	switch (imx326->lane_num) {
	case SENSOR_10_LANE:
		imx326_config.slvds_cfg.lane_mux_0 = 0x8046;
		imx326_config.slvds_cfg.lane_mux_1 = 0x3921;
		imx326_config.slvds_cfg.lane_mux_2 = 0xba75;
		break;
	case SENSOR_8_LANE:
		imx326_config.slvds_cfg.lane_mux_0 = 0x1046;
		imx326_config.slvds_cfg.lane_mux_1 = 0x7532;
		imx326_config.slvds_cfg.lane_mux_2 = 0xba98;
		break;
	case SENSOR_6_LANE:
		imx326_config.slvds_cfg.lane_mux_0 = 0x2104;
		imx326_config.slvds_cfg.lane_mux_1 = 0x7653;
		imx326_config.slvds_cfg.lane_mux_2 = 0xba98;
		break;
	case SENSOR_4_LANE:
		imx326_config.slvds_cfg.lane_mux_0 = 0x3210;
		imx326_config.slvds_cfg.lane_mux_1 = 0x7654;
		imx326_config.slvds_cfg.lane_mux_2 = 0xba98;
		break;
	default:
		vin_error("unsupported lane number %d\n", imx326->lane_num);
		return -EINVAL;
	}

	imx326_config.cap_win.x = format->def_start_x;
	imx326_config.cap_win.y = format->def_start_y;
	imx326_config.cap_win.width = format->def_width;
	imx326_config.cap_win.height = format->def_height;

	/* for hdr sensor */
	imx326_config.hdr_cfg.act_win.x = format->act_start_x;
	imx326_config.hdr_cfg.act_win.y = format->act_start_y;
	imx326_config.hdr_cfg.act_win.width = format->act_width;
	imx326_config.hdr_cfg.act_win.height = format->act_height;

	imx326_config.sensor_id = GENERIC_SENSOR;
	imx326_config.input_format = AMBA_VIN_INPUT_FORMAT_RGB_RAW;
	imx326_config.bayer_pattern = format->bayer_pattern;
	imx326_config.video_format = format->format;
	imx326_config.bit_resolution = format->bits;

	return ambarella_set_vin_config(vdev, &imx326_config);
}

static void imx326_sw_reset(struct vin_device *vdev)
{
	imx326_write_reg(vdev, IMX326_REG_00, 0x1A);/* STANDBY:0, STBLOGIC:1, STBMIPI:1, STBDV:1 */
	msleep(10);
}

static void imx326_start_streaming(struct vin_device *vdev)
{
	imx326_write_reg(vdev, IMX326_PLL_CKEN, 0x01);
	imx326_write_reg(vdev, IMX326_PACKEN, 0x01);
	imx326_write_reg(vdev, IMX326_REG_00, 0x18);/* STANDBY:0, STBLOGIC:0, STBMIPI:1, STBDV:1 */
	msleep(10);
	imx326_write_reg(vdev, IMX326_CLPSQRST, 0x10);
	imx326_write_reg(vdev, IMX326_DCKRST, 0x01);
	msleep(10);
}

static int imx326_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_reg_16_8 *regs;
	int i, regs_num;

	regs = imx326_pll_regs[pll_idx];
	regs_num = ARRAY_SIZE(imx326_pll_regs[pll_idx]);
	for (i = 0; i < regs_num; i++)
		imx326_write_reg(vdev, regs[i].addr, regs[i].data);

	return 0;
}

static int imx326_init_device(struct vin_device *vdev)
{
	imx326_sw_reset(vdev);
	return 0;
}

static int imx326_get_line_time(struct vin_device *vdev)
{
	u64 h_clks;
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	h_clks = (u64)imx326->h_clks * 512000000;
	h_clks = DIV64_CLOSEST(h_clks, vdev->cur_pll->pixelclk);

	vdev->cur_format->line_time = (u32)h_clks;

	return 0;
}

static int imx326_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	struct vin_master_sync master_cfg;
	struct vin_reg_16_8 *regs;
	int i, regs_num, rval;

	regs = imx326_share_regs;
	regs_num = ARRAY_SIZE(imx326_share_regs);
	for (i = 0; i < regs_num; i++)
		imx326_write_reg(vdev, regs[i].addr, regs[i].data);

	switch (format->hdr_mode) {
	case AMBA_VIDEO_LINEAR_MODE:
		regs = imx326_linear_mode_regs[format->device_mode];
		regs_num = ARRAY_SIZE(imx326_linear_mode_regs[format->device_mode]);

		switch (format->device_mode) {
		case 0:
		case 2:
		case 4:
		case 6:
		case 8:
			imx326->h_clks = 500;
			imx326->lane_num = SENSOR_6_LANE;
			break;
		case 1:
		case 3:
		case 7:
		case 9:
			imx326->h_clks = 269;
			imx326->lane_num = lane;
			break;
		case 5:
			imx326->h_clks = 320;
			imx326->lane_num = lane;
			break;
		default:
			vin_error("Unsupported device_mode %d\n", format->device_mode);
			return -EINVAL;
		}
		break;
	case AMBA_VIDEO_2X_HDR_MODE:
		regs = imx326_hdr_mode_regs[format->device_mode];
		regs_num = ARRAY_SIZE(imx326_hdr_mode_regs[format->device_mode]);

		/* for DOL mode, set RHS registers */
		if (format->video_mode == AMBA_VIDEO_MODE_3072_2048) {
			imx326->rhs1 = IMX326_6M_2X_RHS1;
		} else if (format->video_mode == AMBA_VIDEO_MODE_QSXGA) {
			imx326->rhs1 = IMX326_5M_2X_RHS1;
		} else if (format->video_mode == AMBA_VIDEO_MODE_3072_1728) {
			imx326->rhs1 = IMX326_5_3M_2X_RHS1;
		} else if (format->video_mode == AMBA_VIDEO_MODE_2048_2048) {
			imx326->rhs1 = IMX326_4_2M_2X_RHS1;
		} else if (format->video_mode == AMBA_VIDEO_MODE_2688_1520) {
			imx326->rhs1 = IMX326_4M_2X_RHS1;
		} else {
			vin_error("Unknown mode\n");
			return -EINVAL;
		}
		imx326->h_clks = 1042;
		imx326->lane_num = lane;
		imx326_write_reg2(vdev, IMX326_RHS1_LSB, imx326->rhs1);
		break;
	default:
		regs = NULL;
		regs_num = 0;
		vin_error("Unknown mode\n");
		return -EINVAL;
	}
	for (i = 0; i < regs_num; i++)
		imx326_write_reg(vdev, regs[i].addr, regs[i].data);

	imx326->ori_h_clks = imx326->h_clks;
	if (imx326->lane_num == SENSOR_8_LANE)
		imx326_write_reg(vdev, IMX326_REG_03, 0x11);
	else if (imx326->lane_num == SENSOR_10_LANE)
		imx326_write_reg(vdev, IMX326_REG_03, 0x00);

	/* we must enable master sync first before sensor cancels standby */
	master_cfg.hsync_width = 8;
	master_cfg.vsync_width = 8;
	master_cfg.hsync_period = imx326->h_clks;
	master_cfg.vsync_period = 4000;
	ambarella_set_vin_master_sync(vdev, &master_cfg, true);

	/* TG reset release (Enable Streaming) */
	imx326_start_streaming(vdev);

	/* communicate with IAV */
	rval = imx326_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	imx326_get_line_time(vdev);

	return 0;
}

static int imx326_set_shutter_row(struct vin_device *vdev, u32 row)
{
	u32 blank_lines;
	u64 exposure_lines;
	u32 num_line, max_line, min_line;
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	struct vin_video_format *format;

	num_line = row;

	format = vdev->cur_format;

	/* FIXME: shutter width: 4 ~ (Frame format(V) - 12) */
	min_line = 4;
	max_line = imx326->v_lines - 12;
	num_line = clamp(num_line, min_line, max_line);

	/* get the shutter sweep time */
	blank_lines = imx326->v_lines - num_line;
	imx326_write_reg2(vdev, IMX326_SHR_LSB, blank_lines);

	exposure_lines = num_line;
	exposure_lines = exposure_lines * (u64)imx326->h_clks * 512000000;
	exposure_lines = DIV64_CLOSEST(exposure_lines, vdev->cur_pll->pixelclk);

	vdev->shutter_time = (u32)exposure_lines;
	vin_debug("shutter_time:%d, row:%d\n", vdev->shutter_time, num_line);

	return 0;
}

static int imx326_shutter2row(struct vin_device *vdev, u32 *shutter_time)
{
	u64 exposure_lines;
	int rval = 0;
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	exposure_lines = (*shutter_time) * (u64)vdev->cur_pll->pixelclk;
	exposure_lines = DIV64_CLOSEST(exposure_lines, imx326->h_clks);
	exposure_lines = DIV64_CLOSEST(exposure_lines, 512000000);

	*shutter_time = exposure_lines;

	return rval;
}

static int imx326_set_fps(struct vin_device *vdev, int fps)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	struct vin_master_sync master_cfg;
	u64 v_lines, vb_time;
	u32 factor;

	v_lines = fps * (u64)vdev->cur_pll->pixelclk;
	v_lines = DIV64_CLOSEST(v_lines, imx326->ori_h_clks);
	v_lines = DIV64_CLOSEST(v_lines, 512000000);

	if (v_lines > 0xFFFF) {
		vin_debug("h_clks:%d, v_lines:%lld\n", imx326->ori_h_clks, v_lines);

		factor = ((u32)v_lines) / 0xFFFF;
		v_lines = DIV64_CLOSEST(v_lines, factor + 1);
		imx326->h_clks = imx326->ori_h_clks * (factor + 1);
	} else if (imx326->h_clks != imx326->ori_h_clks) {
		imx326->h_clks = imx326->ori_h_clks;
	}
	imx326->v_lines = v_lines;

	imx326_write_reg2(vdev, IMX326_PSLVDS1_LSB, (u16)(v_lines - 0x32));
	imx326_write_reg2(vdev, IMX326_PSLVDS2_LSB, (u16)(v_lines - 0x32));
	imx326_write_reg2(vdev, IMX326_PSLVDS3_LSB, (u16)(v_lines - 0x32));
	imx326_write_reg2(vdev, IMX326_PSLVDS4_LSB, (u16)(v_lines - 0x32));

	vb_time = imx326->h_clks * (u64)(v_lines - vdev->cur_format->height) * 1000000000;
	vb_time = DIV64_CLOSEST(vb_time, vdev->cur_pll->pixelclk);
	vdev->cur_format->vb_time = vb_time;

	/* for slave sensor, we must set master sync to set the frame rate */
	master_cfg.hsync_width = 8;
	master_cfg.vsync_width = 8;
	master_cfg.hsync_period = imx326->h_clks;
	master_cfg.vsync_period = imx326->v_lines;
	return ambarella_set_vin_master_sync(vdev, &master_cfg, false);
}

static int imx326_set_agc_index(struct vin_device *vdev, int agc_idx)
{
	if (agc_idx > IMX326_GAIN_MAXDB) {
		vin_error("agc index %d exceeds maximum %d\n", agc_idx, IMX326_GAIN_MAXDB);
		return -EINVAL;
	}

	imx326_write_reg2(vdev, IMX326_PGC_LSB, IMX326_GAIN_TABLE[agc_idx][IMX326_GAIN_COL_AGAIN]);
	imx326_write_reg(vdev, IMX326_DGAIN, IMX326_GAIN_TABLE[agc_idx][IMX326_GAIN_COL_DGAIN]);

	return 0;
}

static int imx326_set_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	u32 again_index;

	/* long frame */
	again_index = p_again_gp->l;
	imx326_set_agc_index(vdev, again_index);

	memcpy(&(imx326->wdr_again_gp), p_again_gp, sizeof(struct vindev_wdr_gp_s));

	vin_debug("long again index:%d, short1 again index:%d\n",
		p_again_gp->l, p_again_gp->s1);

	return 0;
}

static int imx326_get_wdr_again_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_again_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	memcpy(p_again_gp, &(imx326->wdr_again_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx326_set_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	memcpy(&(imx326->wdr_dgain_gp), p_dgain_gp, sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx326_get_wdr_dgain_idx_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_dgain_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	memcpy(p_dgain_gp, &(imx326->wdr_dgain_gp), sizeof(struct vindev_wdr_gp_s));
	return 0;
}

static int imx326_set_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	u32 fsc, rhs1;
	u32 shutter_long, shutter_short1;
	int rval = 0;
	u32 ratio;
	u64 fixed_sht_long;

	rhs1 = imx326->rhs1;
	fsc = imx326->v_lines;

	/* shutter limitation check */
	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_2X_HDR_MODE) {
		if (unlikely(!imx326->h_clks))
			return -EINVAL;

		/* long shutter */
		shutter_long = p_shutter_gp->l;
		/* short shutter 1 */
		shutter_short1 = p_shutter_gp->s1;

		/* short shutter check */
		if ((shutter_short1 < 2) || (shutter_short1 > rhs1 - 6)) {
			vin_error("short shutter:%d exceeds limitation:[2 ~ %d]!\n",
				shutter_short1, rhs1 - 6);
			return -EPERM;
		}
		/* long shutter check */
		if ((shutter_long < 4) || (shutter_long > fsc - (rhs1 + 6))) {
			vin_error("long shutter:%d exceeds limitation:[4 ~ %d]!\n",
				shutter_long, fsc - (rhs1 + 6));
			return -EPERM;
		}

		/* calculate fixed long shutter for expected shutter ratio */
		ratio = shutter_long / shutter_short1;
		fixed_sht_long = ((shutter_short1 * 100 - 25) * imx326->h_clks + 11200) * ratio - 11200;
		fixed_sht_long = DIV64_CLOSEST(fixed_sht_long, imx326->h_clks) + 25;
		fixed_sht_long = DIV64_CLOSEST(fixed_sht_long, 100);

		shutter_short1 = rhs1 - shutter_short1;
		imx326_write_reg2(vdev, IMX326_SHR_DOL1_LSB, shutter_short1);

		shutter_long = fsc - (u32)fixed_sht_long;
		imx326_write_reg2(vdev, IMX326_SHR_DOL2_LSB, shutter_long);

		vin_debug("fsc:%d, shs1:%d, shs2:%d, rhs1:%d, ratio:%d\n",
			fsc, shutter_short1, shutter_long, rhs1, ratio);
	} else {
		vin_error("Non WDR mode can't support this API: %s!\n", __func__);
		return -EPERM;
	}

	memcpy(&(imx326->wdr_shutter_gp),  p_shutter_gp, sizeof(struct vindev_wdr_gp_s));

	return rval;
}

static int imx326_get_wdr_shutter_row_group(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter_gp)
{
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;
	memcpy(p_shutter_gp, &(imx326->wdr_shutter_gp), sizeof(struct vindev_wdr_gp_s));

	return 0;
}

static int imx326_wdr_shutter2row(struct vin_device *vdev,
	struct vindev_wdr_gp_s *p_shutter2row)
{
	u64 exposure_time_q9;
	int rval = 0;
	struct imx326_priv *imx326 = (struct imx326_priv *)vdev->priv;

	/* long shutter */
	exposure_time_q9 = p_shutter2row->l;
	exposure_time_q9 = exposure_time_q9 * (u64)vdev->cur_pll->pixelclk;
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, imx326->h_clks);
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, 512000000);
	p_shutter2row->l = (u32)exposure_time_q9;

	/* short shutter 1 */
	exposure_time_q9 = p_shutter2row->s1;
	exposure_time_q9 = exposure_time_q9 * (u64)vdev->cur_pll->pixelclk;
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, imx326->h_clks);
	exposure_time_q9 = DIV64_CLOSEST(exposure_time_q9, 512000000);
	p_shutter2row->s1 = (u32)exposure_time_q9;

	return rval;
}

static int imx326_set_mirror_mode(struct vin_device *vdev,
	struct vindev_mirror *mirror_mode)
{
	u32 tmp_reg, readmode, bayer_pattern;
	u32 data_h, data_l;

	switch (mirror_mode->pattern) {
	case VINDEV_MIRROR_AUTO:
		return 0;

	case VINDEV_MIRROR_NONE:
		readmode = 0;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		break;

	case VINDEV_MIRROR_VERTICALLY:
		readmode = IMX326_V_FLIP;
		bayer_pattern = VINDEV_BAYER_PATTERN_RG;
		if (vdev->cur_format->mirror_pattern != VINDEV_MIRROR_VERTICALLY) {
			imx326_read_reg(vdev, IMX326_VWINPOS_LSB, &data_l);
			imx326_read_reg(vdev, IMX326_VWINPOS_MSB, &data_h);
			imx326_write_reg2(vdev, IMX326_VWINPOS_LSB, 4096 - (data_l + (data_h << 8)));
		}
		break;

	case VINDEV_MIRROR_HORRIZONTALLY:
	case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY:
	default:
		vin_error("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	imx326_read_reg(vdev, IMX326_MDVREV, &tmp_reg);
	tmp_reg &= ~IMX326_V_FLIP;
	tmp_reg |= readmode;
	imx326_write_reg(vdev, IMX326_MDVREV, tmp_reg);

	if (mirror_mode->bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
		mirror_mode->bayer_pattern = bayer_pattern;

	return 0;
}

static int imx326_get_eis_info(struct vin_device *vdev,
	struct vindev_eisinfo *eis_info)
{
	eis_info->sensor_cell_width = 162;/* 1.62 um */
	eis_info->sensor_cell_height = 162;/* 1.62 um */
	eis_info->column_bin = 1;
	eis_info->row_bin = 1;
	eis_info->vb_time = vdev->cur_format->vb_time;

	return 0;
}

static int imx326_get_aaa_info(struct vin_device *vdev,
	struct vindev_aaa_info *aaa_info)
{
	struct imx326_priv *pinfo = (struct imx326_priv *)vdev->priv;

	aaa_info->sht0_max = pinfo->v_lines - 2 * 6;
	aaa_info->sht1_max = pinfo->rhs1 - 6;
	aaa_info->sht2_max = 0;

	return 0;
}

#ifdef CONFIG_PM
static int imx326_suspend(struct vin_device *vdev)
{
	u32 i, tmp;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx326_read_reg(vdev, pm_regs[i].addr, &tmp);
		pm_regs[i].data = (u8)tmp;
	}

	return 0;
}

static int imx326_resume(struct vin_device *vdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pm_regs); i++) {
		imx326_write_reg(vdev, pm_regs[i].addr, pm_regs[i].data);
	}

	return 0;
}
#endif

static struct vin_ops imx326_ops = {
	.init_device		= imx326_init_device,
	.set_pll			= imx326_set_pll,
	.set_format		= imx326_set_format,
	.set_shutter_row	= imx326_set_shutter_row,
	.shutter2row		= imx326_shutter2row,
	.set_frame_rate	= imx326_set_fps,
	.set_agc_index		= imx326_set_agc_index,
	.set_mirror_mode	= imx326_set_mirror_mode,
	.get_eis_info		= imx326_get_eis_info,
	.get_aaa_info		= imx326_get_aaa_info,
	.read_reg			= imx326_read_reg,
	.write_reg		= imx326_write_reg,
#ifdef CONFIG_PM
	.suspend		= imx326_suspend,
	.resume			= imx326_resume,
#endif

	/* for wdr sensor */
	.set_wdr_again_idx_gp = imx326_set_wdr_again_idx_group,
	.get_wdr_again_idx_gp = imx326_get_wdr_again_idx_group,
	.set_wdr_dgain_idx_gp = imx326_set_wdr_dgain_idx_group,
	.get_wdr_dgain_idx_gp = imx326_get_wdr_dgain_idx_group,
	.set_wdr_shutter_row_gp = imx326_set_wdr_shutter_row_group,
	.get_wdr_shutter_row_gp = imx326_get_wdr_shutter_row_group,
	.wdr_shutter2row = imx326_wdr_shutter2row,
};

/* ========================================================================== */
static int imx326_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct imx326_priv *imx326;

	vdev = ambarella_vin_create_device(ambdev->name,
		SENSOR_IMX326, sizeof(struct imx326_priv));

	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_SENSOR;
	vdev->sub_type = VINDEV_SUBTYPE_CMOS;
	vdev->default_mode = AMBA_VIDEO_MODE_3072_2048;
	vdev->frame_rate = AMBA_VIDEO_FPS_29_97;
	vdev->agc_db_max = 0x3F000000;  /* 63dB << 24 */
	vdev->agc_db_min = 0x00000000;  /* 0dB << 24 */
	vdev->agc_db_step = 0x00180000; /* 0.09375dB */
	vdev->wdr_again_idx_min = 0;
	vdev->wdr_again_idx_max = IMX326_GAIN_MAXDB;

	imx326 = (struct imx326_priv *)vdev->priv;
	imx326->spi_bus = bus_addr >> 16;
	imx326->spi_cs = bus_addr & 0xffff;

	if ((lane != SENSOR_8_LANE) && (lane != SENSOR_10_LANE)) {
		vin_error("This driver can only support maximum 8 or 10 lanes serial lvds!\n");
		rval = -EINVAL;
		goto imx326_probe_err;
	}

	rval = ambarella_vin_register_device(vdev, &imx326_ops,
		imx326_formats, ARRAY_SIZE(imx326_formats),
		imx326_plls, ARRAY_SIZE(imx326_plls));
	if (rval < 0)
		goto imx326_probe_err;

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("IMX326 init(%d-lane lvds)\n", lane);

	return 0;

imx326_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int imx326_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver imx326_driver = {
	.probe = imx326_drv_probe,
	.remove = imx326_drv_remove,
	.driver = {
		.name = "imx326",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *imx326_device;
static int __init imx326_init(void)
{
	int rval = 0;

	imx326_device = ambpriv_create_bundle(&imx326_driver, NULL, -1, NULL, -1);
	if (IS_ERR(imx326_device))
		rval = PTR_ERR(imx326_device);

	return rval;
}

static void __exit imx326_exit(void)
{
	ambpriv_device_unregister(imx326_device);
	ambpriv_driver_unregister(&imx326_driver);
}

module_init(imx326_init);
module_exit(imx326_exit);

MODULE_DESCRIPTION("IMX326 1/2.9-Inch, 3096x2196, 6.80-Megapixel CMOS Digital Image Sensor");
MODULE_AUTHOR("Hao Zeng, <haozeng@ambarella.com>");
MODULE_LICENSE("Proprietary");

