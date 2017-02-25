/*
 * kernel/private/drivers/iav/arch_s2/iav_vin.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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


#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <plat/highres_timer.h>
#include <plat/iav_helper.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "iav_vin.h"
#include "iav_dsp_cmd.h"

static LIST_HEAD(vin_controller_list);
static DECLARE_BITMAP(vin_device_map, 32);

#define FRAME_RATE_COMP_DELTA (10)

struct vin_controller *vin_get_controller(struct vin_device *vdev)
{
	int found = 0;
	struct vin_controller *vinc;

	list_for_each_entry(vinc, &vin_controller_list, list) {
		if (vinc->id == vdev->intf_id) {
			found = 1;
			break;
		}
	}

	if (!found)
		vinc = NULL;

	return vinc;
}

static void vin_exit_pwr_rst_gpio(struct vin_controller *vinc)
{
	struct ambsvc_gpio gpio_svc;
	int i;

	if (vinc->rst_gpio >= 0) {
		gpio_svc.svc_id = AMBSVC_GPIO_FREE;
		gpio_svc.gpio = vinc->rst_gpio;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}
	for (i = 0; i < 3; i++) {
		if (vinc->pwr_gpio[i] >= 0) {
			gpio_svc.svc_id = AMBSVC_GPIO_FREE;
			gpio_svc.gpio = vinc->pwr_gpio[i];
			ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
		}
	}
}

static int vin_init_pwr_rst_gpio(struct vin_controller *vinc, struct device_node *np)
{
	enum of_gpio_flags flags;
	struct ambsvc_gpio gpio_svc;
	int i, rval;

	if (np == NULL)
		return -ENOENT;

	vinc->rst_gpio = of_get_named_gpio_flags(np, "vinrst-gpios", 0, &flags);
	vinc->rst_gpio_active = !!(flags & OF_GPIO_ACTIVE_LOW);
	if (vinc->rst_gpio >= 0) {
		gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
		gpio_svc.gpio = vinc->rst_gpio;
		rval = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
		if (rval < 0) {
			vin_error("can't request gpio[%d]!\n", vinc->rst_gpio);
			goto pwr_rst_error;
		}
	}

	for (i = 0; i < 3; i++) {
		vinc->pwr_gpio[i] =
			of_get_named_gpio_flags(np, "vinpwr-gpios", i, &flags);
		vinc->pwr_gpio_active[i] = !!(flags & OF_GPIO_ACTIVE_LOW);
		if (vinc->pwr_gpio[i] >= 0) {
			gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
			gpio_svc.gpio = vinc->pwr_gpio[i];
			rval = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
						&gpio_svc, NULL);
			if (rval < 0) {
				vin_error("can't request gpio[%d]!\n", vinc->pwr_gpio[i]);
				goto pwr_rst_error;
			}
		}
	}

	return 0;

pwr_rst_error:
	vin_exit_pwr_rst_gpio(vinc);
	return -EINVAL;
}

static int vin_hw_poweron(struct vin_device *vdev)
{
	struct vin_controller *vinc;
	struct ambsvc_gpio gpio_svc;
	int i;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	for (i = 0; i < 3; i++) {
		if (vinc->pwr_gpio[i] >= 0) {
			gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
			gpio_svc.gpio = vinc->pwr_gpio[i];
			gpio_svc.value = vinc->pwr_gpio_active[i];
			ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
		}
	}

	if (vinc->rst_gpio >= 0) {
		gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
		gpio_svc.gpio = vinc->rst_gpio;
		gpio_svc.value = vinc->rst_gpio_active;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}

	msleep(5);

	if (vinc->rst_gpio >= 0) {
		gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
		gpio_svc.gpio = vinc->rst_gpio;
		gpio_svc.value = !vinc->rst_gpio_active;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}

	return 0;
}

static int vin_hw_poweroff(struct vin_device *vdev)
{
	struct vin_controller *vinc;
	struct ambsvc_gpio gpio_svc;
	int i;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	if (vinc->rst_gpio >= 0) {
		gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
		gpio_svc.gpio = vinc->rst_gpio;
		gpio_svc.value = vinc->rst_gpio_active;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}

	for (i = 0; i < 3; i++) {
		if (vinc->pwr_gpio[i] >= 0) {
			gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
			gpio_svc.gpio = vinc->pwr_gpio[i];
			gpio_svc.value = !vinc->pwr_gpio_active[i];
			ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
		}
	}

	return 0;
}

static int ambarella_vin_set_pll(struct vin_device *vdev, int pll_idx)
{
	struct vin_video_pll *pll;

	if (pll_idx >= vdev->num_plls) {
		vin_error("invalid pll index (%d/%d)\n", vdev->num_plls, pll_idx);
		return -EINVAL;
	}

	pll = &vdev->plls[pll_idx];
	if (pll == vdev->cur_pll)
		return 0;

	vdev->cur_pll = pll;
	rct_set_so_clk_src(pll->mode);
	rct_set_so_freq_hz(pll->clk_si);
	/* wait 5ms for pll stable */
	msleep(5);

	return 0;
}

static int ambarella_vin_set_pll_reg(struct vin_device *vdev, int pll_idx)
{
	if (pll_idx >= vdev->num_plls) {
		vin_error("invalid pll index (%d/%d)\n", vdev->num_plls, pll_idx);
		return -EINVAL;
	}

	/* set device specific configuration if necessary */
	if (vdev->ops->set_pll)
		return vdev->ops->set_pll(vdev, pll_idx);

	return 0;
}

static int ambarella_vin_set_clk(struct vin_device *vdev, int pll_idx)
{
	int rval;

	rval = ambarella_vin_set_pll(vdev, pll_idx);
	if (rval < 0)
		return rval;

	rval = ambarella_vin_set_pll_reg(vdev, pll_idx);
	if (rval < 0)
		return rval;

	return 0;
}

static int ambarella_vin_set_phy(u32 intf_id, u8 interface_type, u8 mipi_bit_rate)
{
	if ((intf_id != VIN_PRIMARY) && (intf_id != VIN_PIP)) {
		vin_error("unsupport interface id %d\n", intf_id);
		return -EINVAL;
	}

	/* phy setting */
	switch (interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		amba_writel(RCT_REG(0x474), 0x3FFFFFF);/* bit 9:8 for PIP VIN */
		break;
	case SENSOR_PARALLEL_LVDS:
		vin_error("S2L doesn't support parallel lvds interface!\n");
		break;
	case SENSOR_SERIAL_LVDS:
		if (intf_id == VIN_PRIMARY) {
			amba_writel(RCT_REG(0x474), 0x0);
			amba_writel(RCT_REG(0x478), 0x145C1);
			msleep(5);
			amba_writel(RCT_REG(0x478), 0x145C0);
		} else {
			amba_writel(RCT_REG(0x474), 0x0);
			amba_writel(RCT_REG(0x478), 0x14401);
			msleep(5);
			amba_writel(RCT_REG(0x478), 0x14400);
		}
		break;
	case SENSOR_MIPI:
		if (intf_id == VIN_PRIMARY) {
			amba_writel(RCT_REG(0x474), 0x0);
			amba_writel(RCT_REG(0x478), 0x14403);
			msleep(5);
			amba_writel(RCT_REG(0x478), 0x14402);

			if (mipi_bit_rate == SENSOR_MIPI_BIT_RATE_H) {
				amba_writel(RCT_REG(0x47C), 0x1FDD964A);
				amba_writel(RCT_REG(0x480), 0x3C1F | (1 << 9)); /* force mipi clk to be HS mode */
			} else {
				amba_writel(RCT_REG(0x47C), 0x1FDD9326);
				amba_writel(RCT_REG(0x480), 0x3C1F | (1 << 9)); /* force mipi clk to be HS mode */
			}
		} else {
			amba_writel(RCT_REG(0x474), 0x0);
			amba_writel(RCT_REG(0x478), 0x14409);
			msleep(5);
			amba_writel(RCT_REG(0x478), 0x14408);
			amba_writel(RCT_REG(0x488), 0x1094F46C);
			amba_writel(RCT_REG(0x48c), 0x101F | (1 << 9)); /* force mipi clk to be HS mode */
		}
		break;
	default:
		vin_error("unsupported interface %d for vin%d!\n", interface_type, intf_id);
		return -EINVAL;
	}

	return 0;
}

void ambarella_vin_add_precise_fps(struct vin_device *vdev,
		struct vin_precise_fps *p_fps, u32 num_p_fps)
{
	vdev->p_fps = p_fps;
	vdev->num_p_fps = num_p_fps;
}
EXPORT_SYMBOL(ambarella_vin_add_precise_fps);

int ambarella_vin_vsync_delay(struct vin_device *vdev, u32 vsync_delay)
{
	struct vin_controller *vinc;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	vinc->vsync_delay = vsync_delay;

	return 0;
}
EXPORT_SYMBOL(ambarella_vin_vsync_delay);

int ambarella_set_vin_config(struct vin_device *vdev, struct vin_device_config *cfg)
{
	struct ambarella_iav *iav;
	struct vin_dsp_config *dsp_config;
	struct vin_controller *vinc;
	u8 sync_code_style, data_edge, lane_number;
	u16 lane_mux[3] = {0, 0, 0};
	int i, lane_mask = 0;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	if (!cfg)
		return 0;

	/* main buffer is input from VIN. */
	iav = vinc->iav;
	iav->main_buffer->input.x = 0;
	iav->main_buffer->input.y = 0;
	iav->main_buffer->input.width = cfg->cap_win.width;
	iav->main_buffer->input.height = cfg->cap_win.height;

	/* update VIN info. */
	vinc->vin_format.interface_type = cfg->interface_type;
	vinc->vin_format.vin_win.width = cfg->cap_win.width;
	vinc->vin_format.vin_win.height= cfg->cap_win.height;
	vinc->vin_format.vin_win.x = cfg->cap_win.x;
	vinc->vin_format.vin_win.y = cfg->cap_win.y;
	vinc->vin_format.sensor_id = cfg->sensor_id;
	vinc->vin_format.input_format = cfg->input_format;
	vinc->vin_format.bayer_pattern = cfg->bayer_pattern;
	vinc->vin_format.video_format = cfg->video_format;
	vinc->vin_format.bit_resolution = cfg->bit_resolution;
	vinc->vin_format.act_win.x = cfg->hdr_cfg.act_win.x;
	vinc->vin_format.act_win.y = cfg->hdr_cfg.act_win.y;
	vinc->vin_format.act_win.width = cfg->hdr_cfg.act_win.width;
	vinc->vin_format.act_win.height = cfg->hdr_cfg.act_win.height;
	vinc->vin_format.max_act_win.width = cfg->hdr_cfg.act_win.max_width;
	vinc->vin_format.max_act_win.height = cfg->hdr_cfg.act_win.max_height;
	vinc->vin_format.readout_mode = cfg->readout_mode;
	vinc->vin_format.sync_mode = cfg->sync_mode;
	iav->vin_enabled = 1;

	/* update VIN config used by DSP cmd. */
	dsp_config = vinc->dsp_config;

	memset(dsp_config, 0, sizeof(struct vin_dsp_config));
	sync_code_style = 0;
	data_edge = 0;
	lane_number = 0;

	/* for MIPI I/F, if dsp loading is heavy enough,
	* VIN may be configured in the middle of a frame,
	* the data packet will be treated as a sync header,
	* and it can cause a hang. Please set swreset to 0
	* if this happens.
	*/
	dsp_config->r0.swreset = 0x1;
	dsp_config->r0.enable = 0x1;
	dsp_config->r0.output_enable = 0x1;

	switch (cfg->interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		sync_code_style = cfg->plvcmos_cfg.sync_code_style;
		data_edge = cfg->plvcmos_cfg.data_edge;
		break;
	case SENSOR_PARALLEL_LVDS:
		sync_code_style = cfg->plvds_cfg.sync_code_style;
		break;
	case SENSOR_SERIAL_LVDS:
		sync_code_style = cfg->slvds_cfg.sync_code_style;
		lane_number = cfg->slvds_cfg.lane_number;
		lane_mux[0] = cfg->slvds_cfg.lane_mux_0;
		lane_mux[1] = cfg->slvds_cfg.lane_mux_1;
		lane_mux[2] = cfg->slvds_cfg.lane_mux_2;
		break;
	case SENSOR_MIPI:
		lane_number = cfg->mipi_cfg.lane_number;
		break;
	}

	if (cfg->interface_type == SENSOR_PARALLEL_LVCMOS ||
		cfg->interface_type == SENSOR_PARALLEL_LVDS) {
		dsp_config->r1.p_pad = (cfg->interface_type == SENSOR_PARALLEL_LVDS) ? 1:0;
		dsp_config->r1.p_rate = (cfg->interface_type == SENSOR_PARALLEL_LVDS) ? 1:0;
		dsp_config->r1.p_width = (cfg->input_mode == SENSOR_RGB_2PIX	 ||
			cfg->input_mode == SENSOR_YUV_2PIX) ? 1:0;
		dsp_config->r1.p_edge = (data_edge == SENSOR_DATA_FALLING_EDGE) ? 1:0;
		dsp_config->r1.p_sync = (cfg->plvcmos_cfg.paralle_sync_type == SENSOR_PARALLEL_SYNC_601) ? 1:0;

		dsp_config->r1.vsync_polarity = !(cfg->plvcmos_cfg.vs_hs_polarity & SENSOR_VS_HIGH);
		dsp_config->r1.hsync_polarity = !(cfg->plvcmos_cfg.vs_hs_polarity & SENSOR_HS_HIGH);

		/* FIXME: this setting is depend on board design */
		dsp_config->r2.hsync_pin_sel = 20;
		dsp_config->r2.vsync_pin_sel = 21;
		dsp_config->r2.field_pin_sel = 22;
	}

	dsp_config->r1.rgb_yuv_mode = (cfg->input_mode == SENSOR_YUV_2PIX ||
		cfg->input_mode == SENSOR_YUV_1PIX) ? 1:0;

	switch (cfg->yuv_pixel_order) {
	case SENSOR_CR_Y0_CB_Y1:
		dsp_config->r1.pix_reorder = 0;
		break;
	case SENSOR_CB_Y0_CR_Y1:
		dsp_config->r1.pix_reorder = 1;
		break;
	case SENSOR_Y0_CR_Y1_CB:
		dsp_config->r1.pix_reorder = 2;
		break;
	case SENSOR_Y0_CB_Y1_CR:
		dsp_config->r1.pix_reorder = 3;
		break;
	}

	/* actually, it's continuous mode when disable_after_vsync is set to 1 */
	dsp_config->r1.disable_after_vsync = 0x1;
	dsp_config->r1.double_buff_enable = 0x1;

	switch (cfg->bit_resolution) {
	case AMBA_VIDEO_BITS_8:
		dsp_config->r0.word_size = 0x0;
		break;
	case AMBA_VIDEO_BITS_10:
		dsp_config->r0.word_size = 0x1;
		break;
	case AMBA_VIDEO_BITS_12:
		dsp_config->r0.word_size = 0x2;
		break;
	case AMBA_VIDEO_BITS_14:
		dsp_config->r0.word_size = 0x3;
		break;
	case AMBA_VIDEO_BITS_16:
		dsp_config->r0.word_size = 0x4;
		break;
	}

	dsp_config->r14.sync_correction = 0x1;
	dsp_config->r14.deskew_enable = 0x1;
	dsp_config->r14.pat_mask_align = 0x1;
	dsp_config->r14.unlock_on_timeout = 0x1;
	dsp_config->r14.unlock_on_deskew_err = 0x1;

	if (cfg->interface_type == SENSOR_SERIAL_LVDS ||
		(cfg->interface_type == SENSOR_PARALLEL_LVCMOS &&
			cfg->plvcmos_cfg.paralle_sync_type == SENSOR_PARALLEL_SYNC_656)) {
		switch (sync_code_style) {
		case SENSOR_SYNC_STYLE_HISPI:
			dsp_config->r14.sync_type = 1;
			if (lane_number == 2)
				dsp_config->r13.sync_interleaving = 1;
			else if (lane_number >= 4)
				dsp_config->r13.sync_interleaving = 2;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.sov_en = 1;
			dsp_config->r18.sync_detect_mask = 0x8000;
			dsp_config->r19.sync_detect_pat = 0x8000;
			dsp_config->r20.sync_compare_mask = 0xFF00;
			dsp_config->r21.sol_pat = 0x8000;
			dsp_config->r25.sov_pat = 0xAB00;
			break;
		case SENSOR_SYNC_STYLE_HISPI_PSP:
			dsp_config->r14.sync_type = 1;
			dsp_config->r13.sync_interleaving = 0;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.eol_en = 1;
			dsp_config->r15.sof_en = 1;
			dsp_config->r15.eof_en = 1;
			dsp_config->r18.sync_detect_mask = 0x8000;
			dsp_config->r19.sync_detect_pat = 0x8000;
			dsp_config->r20.sync_compare_mask = 0xF000;
			dsp_config->r21.sol_pat = 0x8000;
			dsp_config->r22.eol_pat = 0xA000;
			dsp_config->r23.sof_pat = 0xC000;
			dsp_config->r24.eof_pat = 0xE000;
			break;
		case SENSOR_SYNC_STYLE_SONY:
			dsp_config->r14.sync_type = 1;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.eol_en = 1;
			dsp_config->r15.sov_en = 1;
			dsp_config->r15.eov_en = 1;
			dsp_config->r18.sync_detect_mask = 0x8000;
			dsp_config->r19.sync_detect_pat = 0x8000;
			dsp_config->r20.sync_compare_mask = 0xFF00;
			dsp_config->r21.sol_pat = 0x8000;
			dsp_config->r22.eol_pat = 0x9D00;
			dsp_config->r25.sov_pat = 0xAB00;
			dsp_config->r26.eov_pat = 0xB600;
			break;
		case SENSOR_SYNC_STYLE_ITU656:
			dsp_config->r14.sync_type = 1;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.eol_en = 1;
			dsp_config->r15.sov_en = 1;
			dsp_config->r15.eov_en = 1;
			dsp_config->r18.sync_detect_mask = 0x8000;
			dsp_config->r19.sync_detect_pat = 0x8000;
			dsp_config->r20.sync_compare_mask = 0xFF00;
			dsp_config->r21.sol_pat = 0x8000;
			dsp_config->r22.eol_pat = 0x9D00;
			dsp_config->r25.sov_pat = 0xB600;
			dsp_config->r26.eov_pat = 0xAB00;
			break;
		case SENSOR_SYNC_STYLE_PANASONIC:
			dsp_config->r14.sync_type = 1;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.eol_en = 1;
			dsp_config->r15.sof_en = 1;
			dsp_config->r15.eof_en = 1;
			dsp_config->r18.sync_detect_mask = 0xFC00;
			dsp_config->r19.sync_detect_pat = 0x0000;
			dsp_config->r20.sync_compare_mask = 0x0003;
			dsp_config->r21.sol_pat = 0x0;
			dsp_config->r22.eol_pat = 0x1;
			dsp_config->r23.sof_pat = 0x2;
			dsp_config->r24.eof_pat = 0x3;
			dsp_config->r14.pat_mask_align = 0x0;
			break;
		case SENSOR_SYNC_STYLE_SONY_DOL:
			dsp_config->r14.sync_type = 1;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.eol_en = 1;
			dsp_config->r15.sov_en = 1;
			dsp_config->r15.eov_en = 1;
			dsp_config->r18.sync_detect_mask = 0x0000;
			dsp_config->r19.sync_detect_pat = 0x0000;
			dsp_config->r20.sync_compare_mask = 0x3F00;
			dsp_config->r21.sol_pat = 0x0000;
			dsp_config->r22.eol_pat = 0x1D00;
			dsp_config->r25.sov_pat = 0x2B00;
			dsp_config->r26.eov_pat = 0x3600;
			break;
		case SENSOR_SYNC_STYLE_INTERLACE:
			dsp_config->r14.sync_type = 1;
			dsp_config->r15.sol_en = 1;
			dsp_config->r15.sov_en = 1;
			dsp_config->r18.sync_detect_mask = 0x0000;
			dsp_config->r19.sync_detect_pat = 0x0000;
			dsp_config->r20.sync_compare_mask = 0xB000;
			dsp_config->r21.sol_pat = 0x8000;
			dsp_config->r25.sov_pat = 0xA000;
			break;
		default:
			vin_error("Unsupported sync code style!\n");
			return -EINVAL;
		}
	}

	dsp_config->r8.split_width = cfg->hdr_cfg.split_width;
	dsp_config->r9.crop_start_col = cfg->cap_win.x;
	dsp_config->r10.crop_start_row = cfg->cap_win.y;
	dsp_config->r11.crop_end_col = cfg->cap_win.x + cfg->cap_win.width - 1;
	dsp_config->r12.crop_end_row = cfg->cap_win.y + cfg->cap_win.height - 1;

	dsp_config->r3.lane_select0 = 0x0;
	dsp_config->r3.lane_select1 = 0x1;
	dsp_config->r3.lane_select2 = 0x2;
	dsp_config->r3.lane_select3 = 0x3;
	dsp_config->r4.lane_select4 = 0x4;
	dsp_config->r4.lane_select5 = 0x5;
	dsp_config->r4.lane_select6 = 0x6;
	dsp_config->r4.lane_select7 = 0x7;
	dsp_config->r5.lane_select8 = 0x8;
	dsp_config->r5.lane_select9 = 0x9;

	if (lane_mux[0] || lane_mux[1] || lane_mux[2]) {
		dsp_config->r3.lane_select0 = lane_mux[0] & 0xf;
		dsp_config->r3.lane_select1 = (lane_mux[0] >> 4) & 0xf;
		dsp_config->r3.lane_select2 = (lane_mux[0] >> 8) & 0xf;
		dsp_config->r3.lane_select3 = (lane_mux[0] >> 12) & 0xf;

		dsp_config->r4.lane_select4 = lane_mux[1] & 0xf;
		dsp_config->r4.lane_select5 = (lane_mux[1] >> 4) & 0xf;
		dsp_config->r4.lane_select6 = (lane_mux[1] >> 8) & 0xf;
		dsp_config->r4.lane_select7 = (lane_mux[1] >> 12) & 0xf;

		dsp_config->r5.lane_select8 = lane_mux[2] & 0xf;
		dsp_config->r5.lane_select9 = (lane_mux[2] >> 4) & 0xf;
	} else {
		lane_mux[0] = 0x3210;
		lane_mux[1] = 0x7654;
		lane_mux[2] = 0x0098;
	}

	for (i = 0; i < lane_number; i++) {
		if (i < 4)
			lane_mask |= 1 << ((lane_mux[0]  >> (i%4 * 4)) & 0xf) ;
		else if (i < 8)
			lane_mask |= 1 << ((lane_mux[1]  >> (i%4 * 4)) & 0xf) ;
		else if (i < 12)
			lane_mask |= 1 << ((lane_mux[2]  >> (i%4 * 4)) & 0xf) ;
	}
	dsp_config->r0.lane_enable = lane_mask;

	dsp_config->r35.delayed_vsync_l = 0x1;
	dsp_config->r36.delayed_vsync_h = 0x0;
	dsp_config->r37.delayed_vsync_irq_l = 0x1;
	dsp_config->r38.delayed_vsync_irq_h = 0x0;

	if (cfg->interface_type == SENSOR_MIPI) {
		dsp_config->r27.mipi_vc_mask = 0x3;
		dsp_config->r27.mipi_dt_mask = 0x2f;
		dsp_config->r28.mipi_ecc_enable = 1;
	}

	ambarella_vin_set_phy(vdev->intf_id, cfg->interface_type, cfg->mipi_cfg.bit_rate);

	return 0;
}
EXPORT_SYMBOL(ambarella_set_vin_config);

int ambarella_set_vin_master_sync(struct vin_device *vdev, struct vin_master_sync *master_cfg, bool by_dbg_bus)
{
	struct vin_controller *vinc;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	if (!master_cfg)
		return 0;

	/* update master config */
	vinc->master_config->r0.hsync_period_l = master_cfg->hsync_period ;
	vinc->master_config->r1.hsync_period_h = master_cfg->hsync_period >> 16;
	vinc->master_config->r2.hsync_width = master_cfg->hsync_width;
	vinc->master_config->r3.hsync_offset = 0;
	vinc->master_config->r4.vsync_period = master_cfg->vsync_period;
	vinc->master_config->r5.vsync_width = master_cfg->vsync_width;
	vinc->master_config->r6.vsync_offset = 0;
	vinc->master_config->r7.hsync_polarity = 0;
	vinc->master_config->r7.vsync_polarity = 0;
	vinc->master_config->r7.no_vb_hsync = 0;
	vinc->master_config->r7.intr_mode = 0;
	vinc->master_config->r7.vsync_width_unit = 0;
	vinc->master_config->r7.num_vsync = 1;
	vinc->master_config->r7.continuous = 1;
	vinc->master_config->r7.preempt = 0;

	if (by_dbg_bus) {
		amba_writel(DBGBUS_BASE + 0x118000, 0x1000);
		amba_writel(DBGBUS_BASE + 0x110400, vinc->master_config->r0.hsync_period_l);
		amba_writel(DBGBUS_BASE + 0x110404, vinc->master_config->r1.hsync_period_h);
		amba_writel(DBGBUS_BASE + 0x110408, vinc->master_config->r2.hsync_width);
		amba_writel(DBGBUS_BASE + 0x11040C, vinc->master_config->r3.hsync_offset);
		amba_writel(DBGBUS_BASE + 0x110410, vinc->master_config->r4.vsync_period);
		amba_writel(DBGBUS_BASE + 0x110414, vinc->master_config->r5.vsync_width);
		amba_writel(DBGBUS_BASE + 0x110418, vinc->master_config->r6.vsync_offset);
		amba_writel(DBGBUS_BASE + 0x11041C, 0x2020);
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_set_vin_master_sync);

struct vin_device *ambarella_vin_create_device(const char *name, u32 sensor_id, u32 priv_size)
{
	struct vin_device *vdev;

	vdev = kzalloc(sizeof(struct vin_device) + priv_size, GFP_KERNEL);
	if (!vdev)
		return NULL;

	vdev->name = name;
	vdev->sensor_id = sensor_id;

	return vdev;
}
EXPORT_SYMBOL(ambarella_vin_create_device);

void ambarella_vin_free_device (struct vin_device *vdev)
{
	if (!vdev)
		kfree(vdev);
}
EXPORT_SYMBOL(ambarella_vin_free_device);

int ambarella_vin_register_device(struct vin_device *vdev, struct vin_ops *ops,
		struct vin_video_format *formats, u32 num_formats,
		struct vin_video_pll *plls, u32 num_plls)
{
	unsigned long vsrc_id;
	struct vin_controller *vinc;
	struct vin_video_format *format;
	int i;

	if (!vdev)
		return -EINVAL;

	if (!ops || !ops->set_format) {
		vin_error("invalid ops!\n");
		return -EINVAL;
	}

	if ((!ops->set_frame_rate || !ops->set_agc_index || !ops->set_shutter_row)
		&& (vdev->dev_type == VINDEV_TYPE_SENSOR)) {
		vin_error("invalid ops, sensor?\n");
		return -EINVAL;
	} else if ((!ops->get_format) &&  (vdev->dev_type == VINDEV_TYPE_DECODER)){
		vin_error("invalid ops, decoder?\n");
		return -EINVAL;
	}

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	mutex_lock(&vinc->dev_mutex);

	vdev->ops = ops;
	vdev->formats = formats;
	vdev->num_formats = num_formats;
	vdev->plls = plls;
	vdev->num_plls = num_plls;
	vdev->pre_video_mode = AMBA_VIDEO_MODE_OFF;
	vdev->pre_hdr_mode = AMBA_VIDEO_LINEAR_MODE;

	vsrc_id = find_first_zero_bit(vin_device_map, AMBA_VINDEV_MAX_NUM);
	if (vsrc_id >= AMBA_VINDEV_MAX_NUM) {
		vin_error("Man, really so many vin sources?\n");
		mutex_unlock(&vinc->dev_mutex);
		return -ENOSPC;
	}
	set_bit(vsrc_id, vin_device_map);
	vdev->vsrc_id = vsrc_id;

	/* add to the device list of corresponding vin controller */
	list_add_tail(&vdev->list, &vinc->dev_list);

	mutex_unlock(&vinc->dev_mutex);

	/* active the first registered vdev */
	if (!vinc->dev_active) {
		vinc->dev_active = vdev;
		if (vinc->iav->vin_enabled == 0) {
			vin_hw_poweron(vdev);
			if (vdev->ops->init_device) {
				/* setup default pll clock source needed by device to work */
				ambarella_vin_set_pll(vdev, 0);
				vdev->ops->init_device(vdev);
			}
		} else {
			/* FIXME: In fastboot case,  IAV in preview mode by amboot/iav already
			 *  After load IAV, Sensor driver (Don't reinit preview).
			 *  Restart encode will case encode state abnormal, such as fps.
			 *  TODO: For IAV some init param load by "test_encode".
			 *  Need do this in fastboot case.
			*/
			if (vdev->dev_type == VINDEV_TYPE_SENSOR) {
				for (i = 0; i < vdev->num_formats; i++) {
					format = &vdev->formats[i];
					if((format->video_mode == vinc->iav->vin_probe_format->video_mode) &&
						(format->hdr_mode == vinc->iav->vin_probe_format->hdr_mode))
						break;
				}
			} else {
				for (i = 0; i < vdev->num_formats; i++) {
					format = &vdev->formats[i];
					if(format->video_mode == vinc->iav->vin_probe_format->video_mode)
						break;
				}
			}

			if (i >= vdev->num_formats) {
				vin_error("do not support mode %d!\n", vinc->iav->vin_probe_format->video_mode);
				return -EINVAL;
			}

			vdev->cur_format = format;
			vdev->pre_video_mode = format->video_mode;
			vdev->pre_hdr_mode = format->hdr_mode;
			vdev->frame_rate = format->default_fps;
			vdev->cur_pll = &vdev->plls[format->pll_idx];
			format->bayer_pattern = format->default_bayer_pattern;

			vinc->vin_format.vin_win.x = format->def_start_x;
			vinc->vin_format.vin_win.y = format->def_start_y;
			vinc->vin_format.vin_win.width = format->def_width;
			vinc->vin_format.vin_win.height = format->def_height;

			/* for hdr sensor */
			vinc->vin_format.act_win.x = format->act_start_x;
			vinc->vin_format.act_win.y = format->act_start_y;
			vinc->vin_format.act_win.width = format->act_width;
			vinc->vin_format.act_win.height = format->act_height;
			vinc->vin_format.max_act_win.width = format->max_act_width;
			vinc->vin_format.max_act_win.height = format->max_act_height;

			vinc->vin_format.frame_rate = vdev->frame_rate;
			vinc->vin_format.video_format = format->format;
			vinc->vin_format.bit_resolution = format->bits;
			vinc->vin_format.bayer_pattern = format->bayer_pattern;
			vinc->vin_format.readout_mode = format->readout_mode;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_vin_register_device);

int ambarella_vin_unregister_device(struct vin_device *vdev)
{
	struct vin_controller *vinc;

	if (!vdev)
		return -EINVAL;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	mutex_lock(&vinc->dev_mutex);
	list_del_init(&vdev->list);
	clear_bit(vdev->vsrc_id, vin_device_map);
	mutex_unlock(&vinc->dev_mutex);

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	if (vinc->dev_active == vdev) {
		vinc->dev_active = NULL;
		vin_hw_poweroff(vdev);
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_vin_unregister_device);

static int vin_dev_get_dev_info(struct vin_device *vdev, unsigned long args)
{
	struct vindev_devinfo devinfo;

	memset(&devinfo, sizeof(struct vindev_devinfo), 0);
	devinfo.vsrc_id = vdev->vsrc_id;;
	devinfo.intf_id = vdev->intf_id;
	devinfo.dev_type = vdev->dev_type;
	devinfo.sub_type = vdev->sub_type;
	devinfo.sensor_id = vdev->sensor_id;
	strlcpy(devinfo.name, vdev->name, sizeof(devinfo.name));

	if (copy_to_user((void __user *)args, &devinfo, sizeof(devinfo)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_video_info(struct vin_device *vdev, unsigned long args)
{
	struct vindev_video_info video_info;
	struct vin_video_format *format;
	int i;

	if (copy_from_user(&video_info, (void __user *)args, sizeof(video_info)))
		return -EFAULT;

	if (video_info.info.mode == AMBA_VIDEO_MODE_CURRENT) {
		/* get current format, format == NULL means device has
		 * not been initialized */
		format = vdev->cur_format;
		if (!format)
			return -EPERM;

		video_info.info.mode = format->video_mode;
		if(vdev->dev_type == VINDEV_TYPE_SENSOR)
			video_info.info.hdr_mode = format->hdr_mode;
	} else {
		if (video_info.info.mode == AMBA_VIDEO_MODE_AUTO) {
			video_info.info.mode = vdev->default_mode;

			if(vdev->dev_type == VINDEV_TYPE_SENSOR)
				video_info.info.hdr_mode = vdev->default_hdr_mode;
		}

		if(vdev->dev_type == VINDEV_TYPE_SENSOR) {
			for (i = 0; i < vdev->num_formats; i++) {
				format = &vdev->formats[i];
				if ((video_info.info.mode == format->video_mode) &&
					(video_info.info.hdr_mode == format->hdr_mode) &&
					((video_info.info.bits == AMBA_VIDEO_BITS_AUTO) ||
					(video_info.info.bits == format->bits))) {
					break;
				}
			}
			if (i >= vdev->num_formats)
				return -EINVAL;
		} else {
			/* for decoder, run-time detect video format */
			if (vdev->ops->get_format(vdev))
				return -EINVAL;
			format = vdev->formats;
		}
	}

	if(format->act_width && format->act_height) {
		video_info.info.width = format->act_width;
		video_info.info.height = format->act_height;
		video_info.info.max_act_width = format->max_act_width;
		video_info.info.max_act_height = format->max_act_height;
	} else {
		video_info.info.width = format->def_width;
		video_info.info.height = format->def_height;
	}

	video_info.info.max_fps = format->max_fps;
	video_info.info.fps = format->default_fps;
	video_info.info.format = format->format;
	video_info.info.type = format->type;
	video_info.info.bits = format->bits;
	video_info.info.ratio = format->ratio;
	video_info.info.system = AMBA_VIDEO_SYSTEM_AUTO;
	video_info.info.rev = 0;

	if (copy_to_user((void __user *)args, &video_info, sizeof(video_info)))
		return -EFAULT;

	return 0;
}

static int vin_precise_fps_pll(struct vin_device *vdev, int fps)
{
	struct vin_precise_fps *p_fps;
	struct vin_video_format *format;
	int i;

	if (vdev->num_p_fps == 0)
		return -EINVAL;

	format = vdev->cur_format;

	for (i = 0; i < vdev->num_p_fps; i++) {
		p_fps = &vdev->p_fps[i];
		if (p_fps->video_mode == format->video_mode && p_fps->fps == fps)
			return p_fps->pll_idx;
	}

	for (i = 0; i < vdev->num_p_fps; i++) {
		p_fps = &vdev->p_fps[i];
		if (p_fps->video_mode == 0 && p_fps->fps == fps)
			return p_fps->pll_idx;
	}

	return -EINVAL;
}

static int vin_shutter_agc_check(struct vin_device *vdev, u32 shutter_row, int agc_idx)
{
	u32 shutter_time;
	int agc_db;

	shutter_time = shutter_row * vdev->cur_format->line_time;
	agc_db = agc_idx * vdev->agc_db_step;

	if (agc_db >= CLAMP_GAIN && shutter_time < CLAMP_SHUTTER) {
		vin_warn("gain(%d db) should < %d db when shutter time(1/%d s) <= 1/%ds\n",
			agc_db >> 24, CLAMP_GAIN >> 24, 512000000/shutter_time, 512000000/CLAMP_SHUTTER);
		return -EINVAL;
	}

	return 0;
}

static int vin_set_shutter_time(struct vin_device *vdev, int shutter)
{
	int rval;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->ops->shutter2row)
		return -EINVAL;

	if (!vdev->ops->set_shutter_row)
		return -EINVAL;

	rval = vdev->ops->shutter2row(vdev, &shutter);
	if (rval < 0)
		return rval;

	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 1);
	rval = vdev->ops->set_shutter_row(vdev, shutter);
	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 0);
	if (rval < 0)
		return rval;

	return 0;
}

static int vin_dev_set_shutter_time(struct vin_device *vdev, unsigned long args)
{
	struct vindev_shutter vsrc_shutter;

	if (copy_from_user(&vsrc_shutter, (void __user *)args, sizeof(vsrc_shutter)))
		return -EFAULT;

	if (vin_shutter_agc_check(vdev,
		vsrc_shutter.shutter/vdev->cur_format->line_time,
		vdev->agc_db/vdev->agc_db_step) < 0)
	return -EINVAL;

	return vin_set_shutter_time(vdev, vsrc_shutter.shutter);
}

static int vin_set_frame_rate(struct vin_device *vdev, int frame_rate)
{
	int rval = 0, pll_idx;
	struct vin_controller *vinc;
	struct ambarella_iav *iav;
	u32 idsp_out_frame_rate;
	u32 idsp_upsample_type;
	vinc = vin_get_controller(vdev);
	iav = vinc->iav;

	if (!vdev->cur_format || !vdev->cur_pll) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	mutex_lock(&iav->iav_mutex);
	do {
		if (iav->state == IAV_STATE_PREVIEW || iav->state == IAV_STATE_ENCODING) {
			if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
				rval = -EINVAL;
				break;
			}
			idsp_upsample_type = iav->system_config[iav->encode_mode].idsp_upsample_type;

			if ((idsp_upsample_type != IDSP_UPSAMPLE_TYPE_OFF) &&
				(frame_rate - FRAME_RATE_COMP_DELTA > MIN_VIN_UPSAMPLE_FRAME_RATE)) {
				vin_error("VIN fps [%d] can not be smaller than [%d] when Idsp"
					" upsampling enabled.\n", DIV_ROUND_CLOSEST(FPS_Q9_BASE, frame_rate),
					DIV_ROUND_CLOSEST(FPS_Q9_BASE, MIN_VIN_UPSAMPLE_FRAME_RATE));
				rval = -EINVAL;
				break;
			}

			if ((idsp_upsample_type != IDSP_UPSAMPLE_TYPE_OFF) &&
				(frame_rate + FRAME_RATE_COMP_DELTA < idsp_out_frame_rate)) {
				iav_error("Vin fps [%d] shoule not be larger than idsp out fps [%d]"
					" when Idsp upsampling enabled.\n", DIV_ROUND_CLOSEST(FPS_Q9_BASE, frame_rate),
					DIV_ROUND_CLOSEST(FPS_Q9_BASE, idsp_out_frame_rate));
				rval = -EINVAL;
				break;
			}
		}
	} while (0);

	mutex_unlock(&iav->iav_mutex);

	if (rval < 0) {
		return rval;
	}

	if (frame_rate == AMBA_VIDEO_FPS_AUTO)
		frame_rate = vdev->cur_format->default_fps;

	/* here fps = 512000000/FPS, so we use "<" rather than ">" */
	if (frame_rate < vdev->cur_format->max_fps) {
		vin_error("invalid fps (%d)\n", frame_rate);
		return -EINVAL;
	}

	/* check if specific pll is needed for precise fps */
	pll_idx = vin_precise_fps_pll(vdev, frame_rate);
	if (pll_idx < 0)
		pll_idx = vdev->cur_format->pll_idx;

	ambarella_vin_set_clk(vdev, pll_idx);

	rval = vdev->ops->set_frame_rate(vdev, frame_rate);
	if (rval < 0) {
		vin_error("set_frame_rate failed (%d)\n", rval);
		return rval;
	}

	vdev->frame_rate = frame_rate;
	vinc->vin_format.frame_rate = frame_rate;

	/* Modify idsp frame rate factor */
	mutex_lock(&iav->iav_mutex);
	cmd_update_idsp_factor(iav, NULL);
	mutex_unlock(&iav->iav_mutex);

	rval = iav_vin_update_stream_framerate(iav);
	if (rval < 0)
		return rval;

	/* fps changed, so we must adjust shutter time */
	rval = vin_set_shutter_time(vdev, vdev->shutter_time);
	if (rval < 0) {
		vin_error("set_shutter_time failed (%d)\n", rval);
		return rval;
	}

	return 0;
}

static int vin_dev_set_frame_rate(struct vin_device *vdev, unsigned long args)
{
	struct vindev_fps vsrc_fps;

	if (copy_from_user(&vsrc_fps, (void __user *)args, sizeof(vsrc_fps)))
		return -EFAULT;

	return vin_set_frame_rate(vdev, vsrc_fps.fps);
}

static int vin_dev_get_frame_rate(struct vin_device *vdev, unsigned long args)
{
	struct vindev_fps vsrc_fps;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vsrc_fps.vsrc_id = vdev->vsrc_id;
	vsrc_fps.fps = vdev->frame_rate;

	if (copy_to_user((void __user *)args, &vsrc_fps, sizeof(vsrc_fps)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_shutter_time(struct vin_device *vdev, unsigned long args)
{
	struct vindev_shutter vsrc_shutter;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vsrc_shutter.vsrc_id = vdev->vsrc_id;
	vsrc_shutter.shutter = vdev->shutter_time;

	if (copy_to_user((void __user *)args, &vsrc_shutter, sizeof(vsrc_shutter)))
		return -EFAULT;

	return 0;
}

static int vin_set_agc_db(struct vin_device *vdev, int agc_db)
{
	int rval, agc_index;

	if (!vdev->ops->set_agc_index)
		return -EINVAL;

	if (agc_db > vdev->agc_db_max || agc_db < vdev->agc_db_min) {
		vin_error("invalid agc_db!\n");
		return -EINVAL;
	}

	agc_index = (agc_db - vdev->agc_db_min) / vdev->agc_db_step;
	rval = vdev->ops->set_agc_index(vdev, agc_index);
	if (rval < 0)
		return rval;

	vdev->agc_db = agc_db;

	return 0;
}


static int vin_dev_set_agc_db(struct vin_device *vdev, unsigned long args)
{
	struct vindev_agc vsrc_agc;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (copy_from_user(&vsrc_agc, (void __user *)args, sizeof(vsrc_agc)))
		return -EFAULT;

	if (vin_shutter_agc_check(vdev,
		vdev->shutter_time/vdev->cur_format->line_time,
		vsrc_agc.agc/vdev->agc_db_step) < 0)
	return -EINVAL;

	return vin_set_agc_db(vdev, vsrc_agc.agc);
}

static int vin_dev_get_agc_db(struct vin_device *vdev, unsigned long args)
{
	struct vindev_agc vsrc_agc;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vsrc_agc.vsrc_id = vdev->vsrc_id;
	vsrc_agc.agc = vdev->agc_db;
	vsrc_agc.agc_max = vdev->agc_db_max;
	vsrc_agc.agc_min = vdev->agc_db_min;
	vsrc_agc.agc_step = vdev->agc_db_step;
	vsrc_agc.wdr_again_idx_min = vdev->wdr_again_idx_min;
	vsrc_agc.wdr_again_idx_max = vdev->wdr_again_idx_max;
	vsrc_agc.wdr_dgain_idx_min = vdev->wdr_dgain_idx_min;
	vsrc_agc.wdr_dgain_idx_max = vdev->wdr_dgain_idx_max;

	if (copy_to_user((void __user *)args, &vsrc_agc, sizeof(vsrc_agc)))
		return -EFAULT;

	return 0;
}

static int vin_dev_set_mirror_mode(struct vin_device *vdev, unsigned long args)
{
	struct vindev_mirror mirror_mode;
	struct vin_controller *vinc;
	int rval;

	/* find the corresponding vin controller */
	vinc = vin_get_controller(vdev);
	if (!vinc) {
		vin_error("can not find vin controller!\n");
		return -ENODEV;
	}

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (copy_from_user(&mirror_mode, (void __user *)args, sizeof(mirror_mode)))
		return -EFAULT;

	if (vdev->ops->set_mirror_mode) {
		rval = vdev->ops->set_mirror_mode(vdev, &mirror_mode);
		if (rval < 0)
			return rval;
	}

	vdev->cur_format->mirror_pattern = mirror_mode.pattern;
	if (mirror_mode.bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
		vdev->cur_format->bayer_pattern = mirror_mode.bayer_pattern;
		vinc->vin_format.bayer_pattern = vdev->cur_format->bayer_pattern;
	}

	return 0;
}

static int vin_dev_get_mirror_mode(struct vin_device *vdev, unsigned long args)
{
	struct vindev_mirror mirror_mode;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	mirror_mode.vsrc_id = vdev->vsrc_id;
	mirror_mode.pattern = vdev->cur_format->mirror_pattern;
	mirror_mode.bayer_pattern = vdev->cur_format->bayer_pattern;

	if (copy_to_user((void __user *)args, &mirror_mode, sizeof(mirror_mode)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_eis_info(struct vin_device *vdev, unsigned long args)
{
	struct vindev_eisinfo eis_info = {0};
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->get_eis_info)
		return -EINVAL;

	rval = vdev->ops->get_eis_info(vdev, &eis_info);
	if (rval < 0)
		return rval;

	eis_info.vsrc_id = vdev->vsrc_id;

	rval = copy_to_user((void __user *)args, &eis_info, sizeof(eis_info));
	if (rval)
		return -EFAULT;

	return 0;
}

static int vin_dev_read_reg(struct vin_device *vdev, unsigned long args)
{
	struct vindev_reg reg;
	int rval;

	if (!vdev->ops->read_reg)
		return -EINVAL;

	if (copy_from_user(&reg, (void __user *)args, sizeof(reg)))
		return -EFAULT;

	rval = vdev->ops->read_reg(vdev, reg.addr, &reg.data);
	if (rval < 0)
		return rval;

	reg.vsrc_id = vdev->vsrc_id;

	rval = copy_to_user((void __user *)args, &reg, sizeof(reg));
	if (rval)
		return -EFAULT;

	return 0;
}

static int vin_dev_write_reg(struct vin_device *vdev, unsigned long args)
{
	struct vindev_reg reg;
	int rval;

	if (!vdev->ops->write_reg)
		return -EINVAL;

	if (copy_from_user(&reg, (void __user *)args, sizeof(reg)))
		return -EFAULT;

	rval = vdev->ops->write_reg(vdev, reg.addr, reg.data);
	if (rval < 0)
		return rval;

	return 0;
}

static int vin_dev_set_agc_index(struct vin_device *vdev, unsigned long args)
{
	struct vindev_agc_idx vsrc_agc_idx;
	struct vin_controller *vinc;
	int rval;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vinc = vin_get_controller(vdev);

	if (!vdev->ops->set_agc_index)
		return -EINVAL;

	if (copy_from_user(&vsrc_agc_idx, (void __user *)args, sizeof(vsrc_agc_idx)))
		return -EFAULT;

	atomic_set(&vinc->wq_flag, 0);
	wait_event_interruptible_timeout(vinc->wq, atomic_read(&vinc->wq_flag), vinc->tmo);

	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 1);
	rval = vdev->ops->set_agc_index(vdev, vsrc_agc_idx.agc_idx);
	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 0);
	if (rval < 0)
		return rval;

	vdev->agc_db = vsrc_agc_idx.agc_idx * vdev->agc_db_step;

	return 0;
}

static int vin_dev_set_shutter_row(struct vin_device *vdev, unsigned long args)
{
	struct vindev_shutter_row vsrc_shutter_row;
	struct vin_controller *vinc;
	int rval;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vinc = vin_get_controller(vdev);

	if (!vdev->ops->set_shutter_row)
		return -EINVAL;

	if (copy_from_user(&vsrc_shutter_row, (void __user *)args, sizeof(vsrc_shutter_row)))
		return -EFAULT;

	atomic_set(&vinc->wq_flag, 0);
	wait_event_interruptible_timeout(vinc->wq, atomic_read(&vinc->wq_flag), vinc->tmo);

	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 1);
	rval = vdev->ops->set_shutter_row(vdev, vsrc_shutter_row.shutter_row);
	if (vdev->ops->set_hold_mode)
		vdev->ops->set_hold_mode(vdev, 0);
	if (rval < 0)
		return rval;

	return 0;
}

static int vin_dev_get_wdr_shutter_row_gp(struct vin_device *vdev, unsigned long args)
{
	struct vindev_wdr_gp_s shutter_gp = {0};
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->get_wdr_shutter_row_gp)
		return -EINVAL;

	rval = vdev->ops->get_wdr_shutter_row_gp(vdev, &shutter_gp);
	if (rval < 0)
		return rval;

	shutter_gp.vsrc_id = vdev->vsrc_id;

	if (copy_to_user((void __user *)args, &shutter_gp, sizeof(shutter_gp)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_wdr_again_index_gp(struct vin_device *vdev, unsigned long args)
{
	struct vindev_wdr_gp_s again_idx_gp = {0};
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->get_wdr_again_idx_gp)
		return -EINVAL;

	rval = vdev->ops->get_wdr_again_idx_gp(vdev, &again_idx_gp);
	if (rval < 0)
		return rval;

	again_idx_gp.vsrc_id = vdev->vsrc_id;

	if (copy_to_user((void __user *)args, &again_idx_gp, sizeof(again_idx_gp)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_wdr_dgain_index_gp(struct vin_device *vdev, unsigned long args)
{
	struct vindev_wdr_gp_s dgain_idx_gp = {0};
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->get_wdr_dgain_idx_gp)
		return -EINVAL;

	rval = vdev->ops->get_wdr_dgain_idx_gp(vdev, &dgain_idx_gp);
	if (rval < 0)
		return rval;

	dgain_idx_gp.vsrc_id = vdev->vsrc_id;

	if (copy_to_user((void __user *)args, &dgain_idx_gp, sizeof(dgain_idx_gp)))
		return -EFAULT;

	return 0;
}

static int vin_dev_shutter2row(struct vin_device *vdev, unsigned long args)
{
	struct vindev_shutter_row vsrc_shutter_row;
	int rval;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->shutter2row)
		return -EINVAL;

	if (copy_from_user(&vsrc_shutter_row, (void __user *)args, sizeof(vsrc_shutter_row)))
		return -EFAULT;

	rval = vdev->ops->shutter2row(vdev, &vsrc_shutter_row.shutter_row);
	if (rval < 0)
		return rval;

	if (copy_to_user((void __user *)args, &vsrc_shutter_row, sizeof(vsrc_shutter_row)))
		return -EFAULT;

	return 0;
}

static int vin_dev_wdr_shutter2row(struct vin_device *vdev, unsigned long args)
{
	struct vindev_wdr_gp_s shutter2row;
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->wdr_shutter2row)
		return -EINVAL;

	if (copy_from_user(&shutter2row, (void __user *)args, sizeof(shutter2row)))
		return -EFAULT;

	rval = vdev->ops->wdr_shutter2row(vdev, &shutter2row);
	if (rval < 0)
		return rval;

	shutter2row.vsrc_id = vdev->vsrc_id;

	if (copy_to_user((void __user *)args, &shutter2row, sizeof(shutter2row)))
		return -EFAULT;

	return 0;
}

#define UPDATE_SHUTTER		(1<<0)
#define UPDATE_AGAIN		(1<<1)
#define UPDATE_DGAIN		(1<<2)
static int vin_dev_set_wdr_gp(struct vin_device *vdev, unsigned long args)
{
	struct vindev_wdr_gp_info wdr_gp;
	struct vin_controller *vinc;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (vdev->cur_format->hdr_mode == AMBA_VIDEO_LINEAR_MODE ||
		vdev->cur_format->hdr_mode == AMBA_VIDEO_INT_HDR_MODE) {
		vin_error("%s can only be applied for HDR modes\n", __func__);
		return -EPERM;
	}

	if (!vdev->ops->set_wdr_shutter_row_gp &&
		!vdev->ops->set_wdr_again_idx_gp &&
		!vdev->ops->set_wdr_dgain_idx_gp)
		return -EINVAL;

	vinc = vin_get_controller(vdev);

	if (copy_from_user(&wdr_gp, (void __user *)args, sizeof(wdr_gp)))
		return -EFAULT;

	if (wdr_gp.shutter_gp.l != vdev->wdr_gp.shutter_gp.l ||
		wdr_gp.shutter_gp.s1 != vdev->wdr_gp.shutter_gp.s1 ||
		wdr_gp.shutter_gp.s2 != vdev->wdr_gp.shutter_gp.s2 ||
		wdr_gp.shutter_gp.s3 != vdev->wdr_gp.shutter_gp.s3) {
		memcpy(&vdev->wdr_gp.shutter_gp, &wdr_gp.shutter_gp, sizeof(wdr_gp.shutter_gp));
		vdev->wdr_gp.update_flag |= UPDATE_SHUTTER;
	}

	if (wdr_gp.again_gp.l!= vdev->wdr_gp.again_gp.l ||
		wdr_gp.again_gp.s1 != vdev->wdr_gp.again_gp.s1||
		wdr_gp.again_gp.s2 != vdev->wdr_gp.again_gp.s2 ||
		wdr_gp.again_gp.s3 != vdev->wdr_gp.again_gp.s3) {
		memcpy(&vdev->wdr_gp.again_gp, &wdr_gp.again_gp, sizeof(wdr_gp.again_gp));
		vdev->wdr_gp.update_flag |= UPDATE_AGAIN;
	}

	if (wdr_gp.dgain_gp.l!= vdev->wdr_gp.dgain_gp.l ||
		wdr_gp.dgain_gp.s1 != vdev->wdr_gp.dgain_gp.s1||
		wdr_gp.dgain_gp.s2 != vdev->wdr_gp.dgain_gp.s2 ||
		wdr_gp.dgain_gp.s3 != vdev->wdr_gp.dgain_gp.s3) {
		memcpy(&vdev->wdr_gp.dgain_gp, &wdr_gp.dgain_gp, sizeof(wdr_gp.dgain_gp));
		vdev->wdr_gp.update_flag |= UPDATE_DGAIN;
	}

	if (vdev->wdr_gp.update_flag) {
		if (!atomic_read(&vinc->wdr_wq_flag) && !vdev->wdr_gp.wake_flag) {
			atomic_inc(&vinc->wdr_wq_flag);
			wake_up_interruptible(&vinc->wq);
		} else {
			vdev->wdr_gp.wake_flag = 1;
		}
	}

	return 0;
}

static int update_wdr_gp_task(void *arg)
{
	int rval = 0;
	struct vin_device *vdev = (struct vin_device *)arg;
	struct vin_controller *vinc = vin_get_controller(vdev);
	struct vindev_wdr_gp_s shutter_gp, again_gp, dgain_gp;
	u32 update_flag;

	while (1) {
		/* wait for wdr setting ioctl */
		atomic_set(&vinc->wdr_wq_flag, 0);
		rval = wait_event_interruptible(vinc->wq, atomic_read(&vinc->wdr_wq_flag) || vdev->wdr_gp.killing_kthread || vdev->wdr_gp.wake_flag);
		if (vdev->wdr_gp.update_flag) {
			memcpy(&shutter_gp, &vdev->wdr_gp.shutter_gp, sizeof(shutter_gp));
			memcpy(&again_gp, &vdev->wdr_gp.again_gp, sizeof(again_gp));
			memcpy(&dgain_gp, &vdev->wdr_gp.dgain_gp, sizeof(dgain_gp));
			update_flag = vdev->wdr_gp.update_flag;
			vdev->wdr_gp.update_flag = 0;
		} else {
			vdev->wdr_gp.wake_flag = 0;
			update_flag = 0;
		}

		/* wait for next vsync */
		atomic_set(&vinc->wq_flag, 0);
		rval = wait_event_interruptible_timeout(vinc->wq, atomic_read(&vinc->wq_flag) || vdev->wdr_gp.killing_kthread, vinc->tmo);
		if (vdev->wdr_gp.killing_kthread || !rval) {
			vdev->wdr_gp.killing_kthread = 0;
			vdev->wdr_gp.kthread = 0;
			break;
		}

		if (update_flag) {
			if (vdev->ops->set_hold_mode)
				vdev->ops->set_hold_mode(vdev, 1);

			if ((update_flag & UPDATE_SHUTTER) && vdev->ops->set_wdr_shutter_row_gp)
				vdev->ops->set_wdr_shutter_row_gp(vdev, &shutter_gp);

			if ((update_flag & UPDATE_AGAIN) && vdev->ops->set_wdr_again_idx_gp)
				vdev->ops->set_wdr_again_idx_gp(vdev, &again_gp);

			if ((update_flag & UPDATE_DGAIN) && vdev->ops->set_wdr_dgain_idx_gp)
				vdev->ops->set_wdr_dgain_idx_gp(vdev, &dgain_gp);

			if (vdev->ops->set_hold_mode)
				vdev->ops->set_hold_mode(vdev, 0);
		}
	}
	return 0;
}

static int vin_dev_set_video_mode(struct vin_device *vdev, unsigned long args)
{
	struct vin_video_format *format;
	struct vindev_mode mode;
	int rval, i;

	if (copy_from_user(&mode, (void __user *)args, sizeof(mode)))
		return -EFAULT;

	if (mode.video_mode == AMBA_VIDEO_MODE_OFF) {
		if (vdev->cur_format) {
			vdev->cur_format = NULL;
			vin_hw_poweroff(vdev);
		}

		return 0;
	} else if ((!vdev->cur_format && mode.video_mode != AMBA_VIDEO_MODE_OFF) ||
		vdev->reset_for_mode_switch){
		vin_hw_poweron(vdev);
		if (vdev->ops->init_device) {
			/* setup default pll clock source needed by device to work */
			ambarella_vin_set_pll(vdev, 0);
			vdev->ops->init_device(vdev);
		}
	}

	if (mode.video_mode == AMBA_VIDEO_MODE_AUTO)
		mode.video_mode = vdev->default_mode;

	/* check which registered format supports this mode */
	if (vdev->dev_type == VINDEV_TYPE_SENSOR) {
		for (i = 0; i < vdev->num_formats; i++) {
			format = &vdev->formats[i];
			if((format->video_mode == mode.video_mode) &&
				(format->hdr_mode == mode.hdr_mode)) {
				if ((mode.bits == AMBA_VIDEO_BITS_AUTO) ||
					(mode.bits == format->bits)) {
					break;
				}
			}
		}
	} else {
		for (i = 0; i < vdev->num_formats; i++) {
			format = &vdev->formats[i];
			if(format->video_mode == mode.video_mode)
				break;
		}
	}

	if (i >= vdev->num_formats) {
		vin_error("do not support mode %d!\n", mode.video_mode);
		return -EINVAL;
	}

	/* setup corresponding pll clock source and regs */
	rval = ambarella_vin_set_clk(vdev, format->pll_idx);
	if (rval < 0)
		return rval;

	vdev->cur_format = format;
	vdev->pre_video_mode = mode.video_mode;
	vdev->pre_hdr_mode = mode.hdr_mode;
	vdev->pre_bits = mode.bits;
	format->bayer_pattern = format->default_bayer_pattern;

	rval = vdev->ops->set_format(vdev, format);
	if (rval < 0)
		return rval;

	/* set default fps, shutter time and agc if necessary. do not change the order. */
	if (format->default_shutter_time) {
		rval = vin_set_shutter_time(vdev, format->default_shutter_time);
		if (rval < 0)
			return rval;
	}
	if (format->default_agc) {
		rval = vin_dev_set_agc_db(vdev, format->default_agc);
		if (rval < 0)
			return rval;
	}
	if (format->default_fps) {
		rval = vin_set_frame_rate(vdev, format->default_fps);
		if (rval < 0)
			return rval;
	}

	if (format->hdr_mode != AMBA_VIDEO_LINEAR_MODE &&
		format->hdr_mode != AMBA_VIDEO_INT_HDR_MODE) {
		/* initial wdr gp value for each mode switching */
		memset(&vdev->wdr_gp.shutter_gp, 0, sizeof(struct vindev_wdr_gp_s));
		memset(&vdev->wdr_gp.again_gp, 0xFFFFFFFF, sizeof(struct vindev_wdr_gp_s));
		memset(&vdev->wdr_gp.dgain_gp, 0xFFFFFFFF, sizeof(struct vindev_wdr_gp_s));

		if (!vdev->wdr_gp.kthread)
			vdev->wdr_gp.kthread = kthread_run(update_wdr_gp_task, vdev, "vin_update_wdr");
	} else {
		if (vdev->wdr_gp.kthread) {
			vdev->wdr_gp.killing_kthread = 1;
			kthread_stop(vdev->wdr_gp.kthread);
		}
	}

	return 0;
}

static int vin_dev_get_video_mode(struct vin_device *vdev, unsigned long args)
{
	struct vindev_mode mode;

	mode.vsrc_id = vdev->vsrc_id;

	if (!vdev->cur_format) {
		mode.video_mode = vdev->pre_video_mode;
		mode.hdr_mode = vdev->pre_hdr_mode;
		mode.bits = vdev->pre_bits;
	} else {
		mode.video_mode = vdev->cur_format->video_mode;
		mode.hdr_mode = vdev->cur_format->hdr_mode;
		mode.bits = vdev->cur_format->bits;
	}

	if (copy_to_user((void __user *)args, &mode, sizeof(mode)))
		return -EFAULT;

	return 0;
}

static int vin_dev_get_aaa_info(struct vin_device *vdev, unsigned long args)
{
	struct vindev_aaa_info vsrc_aaa_info;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	memset(&vsrc_aaa_info, 0, sizeof(struct vindev_aaa_info));
	if (vdev->ops->get_aaa_info) {
		vdev->ops->get_aaa_info(vdev, &vsrc_aaa_info);
	}
	vsrc_aaa_info.vsrc_id = vdev->vsrc_id;
	vsrc_aaa_info.sensor_id = vdev->sensor_id;
	vsrc_aaa_info.bayer_pattern = vdev->cur_format->bayer_pattern;
	vsrc_aaa_info.agc_step = vdev->agc_db_step;
	vsrc_aaa_info.hdr_mode = vdev->cur_format->hdr_mode;
	vsrc_aaa_info.hdr_long_offset = vdev->cur_format->hdr_long_offset;
	vsrc_aaa_info.hdr_short1_offset = vdev->cur_format->hdr_short1_offset;
	vsrc_aaa_info.hdr_short2_offset = vdev->cur_format->hdr_short2_offset;
	vsrc_aaa_info.hdr_short3_offset = vdev->cur_format->hdr_short3_offset;
	vsrc_aaa_info.dual_gain_mode = vdev->cur_format->dual_gain_mode;
	vsrc_aaa_info.line_time = vdev->cur_format->line_time;
	vsrc_aaa_info.vb_time = vdev->cur_format->vb_time;
	vsrc_aaa_info.pixel_size = vdev->pixel_size;

	if (copy_to_user((void __user *)args, &vsrc_aaa_info, sizeof(vsrc_aaa_info)))
		return -EFAULT;

	return 0;
}

static int vin_dev_set_agc_shutter(struct vin_device *vdev, unsigned long args)
{
	struct vin_controller *vinc;
	struct vindev_agc_shutter agc_shutter;
	int rval;

	if (vdev->dev_type == VINDEV_TYPE_DECODER)
		return 0;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	vinc = vin_get_controller(vdev);

	if (copy_from_user(&agc_shutter, (void __user *)args, sizeof(agc_shutter)))
		return -EFAULT;

	if ((!vdev->ops->set_agc_index) && (!vdev->ops->set_shutter_row))
		return -EINVAL;

	if (!vdev->cur_format->line_time) {
		vin_error("line time is 0, please check sensor driver!\n");
		return -EINVAL;
	}

	atomic_set(&vinc->wq_flag, 0);
	wait_event_interruptible_timeout(vinc->wq, atomic_read(&vinc->wq_flag), vinc->tmo);

	switch(agc_shutter.mode) {
	case VINDEV_SET_AGC_ONLY:
		if (vin_shutter_agc_check(vdev,
			vdev->shutter_time/vdev->cur_format->line_time,
			agc_shutter.agc_idx) < 0)
			return -EINVAL;

		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 1);
		rval = vdev->ops->set_agc_index(vdev, agc_shutter.agc_idx);
		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 0);
		if (rval < 0)
			return rval;
		vdev->agc_db = agc_shutter.agc_idx * vdev->agc_db_step;
		break;
	case VINDEV_SET_SHT_ONLY:
		if (vin_shutter_agc_check(vdev,
			agc_shutter.shutter_row,
			vdev->agc_db/vdev->agc_db_step) < 0)
			return -EINVAL;

		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 1);
		rval = vdev->ops->set_shutter_row(vdev, agc_shutter.shutter_row);
		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 0);
		if (rval < 0)
			return rval;
		break;
	case VINDEV_SET_AGC_SHT:
		if (vin_shutter_agc_check(vdev,
			agc_shutter.shutter_row,
			agc_shutter.agc_idx) < 0)
			return -EINVAL;

		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 1);
		rval = vdev->ops->set_agc_index(vdev, agc_shutter.agc_idx);
		rval |= vdev->ops->set_shutter_row(vdev, agc_shutter.shutter_row);
		if (vdev->ops->set_hold_mode)
			vdev->ops->set_hold_mode(vdev, 0);

		vdev->agc_db = agc_shutter.agc_idx * vdev->agc_db_step;
		if (rval < 0)
			return rval;
		break;
	default:
		vin_error("%s can't support mode %d\n", __func__, agc_shutter.mode);
		break;
	}

	return 0;
}

static int vin_dev_set_dgain_ratio(struct vin_device *vdev, unsigned long args)
{
	struct vindev_dgain_ratio dgain_ratio;
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->set_dgain_ratio)
		return -EINVAL;

	if (copy_from_user(&dgain_ratio, (void __user *)args, sizeof(dgain_ratio)))
		return -EFAULT;

	rval = vdev->ops->set_dgain_ratio(vdev, &dgain_ratio);
	if (rval < 0)
		return rval;

	return 0;
}

static int vin_dev_get_dgain_ratio(struct vin_device *vdev, unsigned long args)
{
	struct vindev_dgain_ratio dgain_ratio = {0};
	int rval;

	if (!vdev->cur_format) {
		vin_error("please set proper format first\n");
		return -EPERM;
	}

	if (!vdev->ops->get_dgain_ratio)
		return -EINVAL;

	rval = vdev->ops->get_dgain_ratio(vdev, &dgain_ratio);
	if (rval < 0)
		return rval;

	dgain_ratio.vsrc_id = vdev->vsrc_id;

	rval = copy_to_user((void __user *)args, &dgain_ratio, sizeof(dgain_ratio));
	if (rval)
		return -EFAULT;

	return 0;
}

static int vin_dev_get_chip_status(struct vin_device *vdev, unsigned long args)
{
	struct vindev_chip_status chip_status;
	int rval;

	if (!vdev->ops->get_chip_status)
		return -EINVAL;

	if (copy_from_user(&chip_status, (void __user *)args, sizeof(chip_status)))
		return -EFAULT;

	rval = vdev->ops->get_chip_status(vdev, &chip_status);
	if (rval < 0)
		return rval;

	chip_status.vsrc_id = vdev->vsrc_id;
	rval = copy_to_user((void __user *)args, &chip_status, sizeof(chip_status));
	if (rval)
		return -EFAULT;

	return 0;
}

static int vin_device_ioctl(struct vin_controller *vinc,
		unsigned int cmd, unsigned long args)
{
	struct vin_device *vdev;
	int rval, vsrc_id, found = 0;

	if (get_user(vsrc_id, (__user unsigned int *)args))
		return -EFAULT;

	/* find the vin source */
	mutex_lock(&vinc->dev_mutex);
	list_for_each_entry(vdev, &vinc->dev_list, list) {
		if (vdev->vsrc_id == vsrc_id) {
			found = 1;
			break;
		}
	}

	if (!found) {
		mutex_unlock(&vinc->dev_mutex);
		return -ENODEV;
	}

	/* parse the ioctl command */
	switch (cmd) {
	case IAV_IOC_VIN_GET_DEVINFO:
		rval = vin_dev_get_dev_info(vdev, args);
		break;
	case IAV_IOC_VIN_GET_VIDEOINFO:
		rval = vin_dev_get_video_info(vdev, args);
		break;
	case IAV_IOC_VIN_SET_MODE:
		rval = vin_dev_set_video_mode(vdev, args);
		break;
	case IAV_IOC_VIN_GET_MODE:
		rval = vin_dev_get_video_mode(vdev, args);
		break;
	case IAV_IOC_VIN_SET_FPS:
		rval = vin_dev_set_frame_rate(vdev, args);
		break;
	case IAV_IOC_VIN_GET_FPS:
		rval = vin_dev_get_frame_rate(vdev, args);
		break;
	case IAV_IOC_VIN_SET_SHUTTER:
		rval = vin_dev_set_shutter_time(vdev, args);
		break;
	case IAV_IOC_VIN_GET_SHUTTER:
		rval = vin_dev_get_shutter_time(vdev, args);
		break;
	case IAV_IOC_VIN_SET_AGC:
		rval = vin_dev_set_agc_db(vdev, args);
		break;
	case IAV_IOC_VIN_GET_AGC:
		rval = vin_dev_get_agc_db(vdev, args);
		break;
	case IAV_IOC_VIN_SET_MIRROR:
		rval = vin_dev_set_mirror_mode(vdev, args);
		break;
	case IAV_IOC_VIN_GET_MIRROR:
		rval = vin_dev_get_mirror_mode(vdev, args);
		break;
	case IAV_IOC_VIN_GET_EISINFO:
		rval = vin_dev_get_eis_info(vdev, args);
		break;
	case IAV_IOC_VIN_GET_REG:
		rval = vin_dev_read_reg(vdev, args);
		break;
	case IAV_IOC_VIN_SET_REG:
		rval = vin_dev_write_reg(vdev, args);
		break;
	case IAV_IOC_VIN_SET_AGC_INDEX:
		rval = vin_dev_set_agc_index(vdev, args);
		break;
	case IAV_IOC_VIN_SHUTTER2ROW:
		rval = vin_dev_shutter2row(vdev, args);
		break;
	case IAV_IOC_VIN_SET_SHUTTER_ROW:
		rval = vin_dev_set_shutter_row(vdev, args);
		break;
	case IAV_IOC_VIN_GET_WDR_SHUTTER_ROW_GP:
		rval = vin_dev_get_wdr_shutter_row_gp(vdev, args);
		break;
	case IAV_IOC_VIN_GET_WDR_AGAIN_INDEX_GP:
		rval = vin_dev_get_wdr_again_index_gp(vdev, args);
		break;
	case IAV_IOC_VIN_GET_WDR_DGAIN_INDEX_GP:
		rval = vin_dev_get_wdr_dgain_index_gp(vdev, args);
		break;
	case IAV_IOC_VIN_WDR_SHUTTER2ROW:
		rval = vin_dev_wdr_shutter2row(vdev, args);
		break;
	case IAV_IOC_VIN_GET_AAAINFO:
		rval = vin_dev_get_aaa_info(vdev, args);
		break;
	case IAV_IOC_VIN_SET_AGC_SHUTTER:
		rval = vin_dev_set_agc_shutter(vdev, args);
		break;
	case IAV_IOC_VIN_SET_DGAIN_RATIO:
		rval = vin_dev_set_dgain_ratio(vdev, args);
		break;
	case IAV_IOC_VIN_GET_DGAIN_RATIO:
		rval = vin_dev_get_dgain_ratio(vdev, args);
		break;
	case IAV_IOC_VIN_GET_CHIP_STATUS:
		rval = vin_dev_get_chip_status(vdev, args);
		break;
	case IAV_IOC_VIN_SET_WDR_GP:
		rval = vin_dev_set_wdr_gp(vdev, args);
		break;

	default:
		rval = -ENOIOCTLCMD;
		vin_error("do not support cmd %d!\n", cmd);
		break;
	}

	mutex_unlock(&vinc->dev_mutex);

	return rval;
}

int vin_pm_suspend(struct vin_device *vdev)
{
	struct vin_controller *vinc = NULL;
	vinc = vin_get_controller(vdev);

	if (vinc->iav->vin_enabled) {
		/* suspend sensor */
		if (vdev->ops->suspend) {
			vdev->ops->suspend(vdev);
		}
	}
	vin_hw_poweroff(vdev);

	return 0;
}

int vin_pm_resume(struct vin_device *vdev)
{
	struct vin_controller *vinc = NULL;
	struct vindev_mirror mirror_mode;

	vin_hw_poweron(vdev);
	/* must clear cur_pll so that clk_si can be set again when vin resumes */
	vdev->cur_pll = NULL;

	vinc = vin_get_controller(vdev);
	if (vinc->iav->vin_enabled && vdev->ops->init_device && vdev->ops->set_format) {
		if (vdev->cur_format) {
			/* setup pll clock source needed by device to work */
			ambarella_vin_set_pll(vdev, vdev->cur_format->pll_idx);
		}
		vdev->ops->init_device(vdev);
		if (vdev->cur_format) {
			/* set sensor pll registers */
			ambarella_vin_set_pll_reg(vdev, vdev->cur_format->pll_idx);
			vdev->ops->set_format(vdev, vdev->cur_format);

			if (vdev->frame_rate)
				vdev->ops->set_frame_rate(vdev, vdev->frame_rate);
			if (vdev->ops->set_mirror_mode) {
				mirror_mode.vsrc_id = 0;
				mirror_mode.pattern = vdev->cur_format->mirror_pattern;
				mirror_mode.bayer_pattern = vdev->cur_format->bayer_pattern;
				vdev->ops->set_mirror_mode(vdev, &mirror_mode);
			}
		}
		/* resume sensor shutter and gain status */
		if (vdev->ops->resume) {
			vdev->ops->resume(vdev);
		}
	}

	return 0;
}

static void vin_update_vsync_proc(struct vin_controller *vinc)
{
	if (vinc) {
		if (!atomic_read(&vinc->wq_flag)) {
			atomic_inc(&vinc->wq_flag);
			wake_up_interruptible(&vinc->wq);
		}

		atomic_set(&vinc->proc_hinfo.sync_proc_flag, 0xFFFFFFFF);
		wake_up_all(&vinc->proc_hinfo.sync_proc_head);
	}
}

static enum hrtimer_restart vin_vsync_timer(struct hrtimer *timer)
{
	struct vin_controller *vinc;

	vinc = container_of(timer, struct vin_controller, vsync_timer);
	vin_update_vsync_proc(vinc);

	return HRTIMER_NORESTART;
}

static int vin_vsync_proc_show(char *start, void *data)
{
	struct vin_controller *vinc;
	struct vin_video_format *format;

	vinc = (struct vin_controller *)data;
	format = vinc->dev_active->cur_format;

	if (!format)
		vinc->irq_counter = -EPERM;

	return sprintf(start, "%08x", vinc->irq_counter);
}

static const struct file_operations vin_vsync_proc_fops = {
	.open = ambsync_proc_open,
	.read = ambsync_proc_read,
	.release = ambsync_proc_release,
	.write = ambsync_proc_write,
};

static irqreturn_t vin_idsp_sof_irq(int irqno, void *dev_id)
{
	struct vin_controller *vinc = dev_id;
	ktime_t ktime;

	vinc->irq_counter++;

	if (vinc->vsync_delay) {
		ktime = ktime_set(0, vinc->vsync_delay);
		highres_timer_start(&vinc->vsync_timer, ktime, HRTIMER_MODE_REL);
	} else {
		vin_update_vsync_proc(vinc);
	}

	return IRQ_HANDLED;
}

static irqreturn_t vin_idsp_vin(int irqno, void *dev_id)
{
	//DRV_PRINT(KERN_DEBUG "IRQ no = %d\n", irqno);
	return IRQ_HANDLED;
}

static struct ambarella_vin_irq vin0_irq[] = {
	//{"vin", VIN_IRQ, vin_idsp_vin,},
	{"idsp_last_pixel", IDSP_VIN_LAST_PIXEL_IRQ, vin_idsp_vin,},
	{"idsp", IDSP_VIN_VSYNC_IRQ, vin_idsp_vin,},
	{"idsp_sof", IDSP_VIN_SOF_IRQ, vin_idsp_sof_irq,},
	//{"idsp_dvsync", IDSP_VIN_DVSYNC_IRQ, vin_idsp_vin,},
	//{"idsp_mvsync", IDSP_VIN_MVSYNC_IRQ, vin_idsp_vin,},
};

static struct ambarella_vin_irq vin1_irq[] = {
	//{"vin", VIN_IRQ, vin_idsp_vin,},
	//{"idsp_last_pixel", IDSP_PIP_LAST_PIXEL_IRQ, vin_idsp_vin,},
	//{"idsp", IDSP_PIP_VSYNC_IRQ, vin_idsp_vin,},
	//{"idsp_sof", IDSP_PIP_SOF_IRQ, vin_idsp_vin,},
	//{"idsp_dvsync", IDSP_PIP_DVSYNC_IRQ, vin_idsp_vin,},
	//{"idsp_mvsync", IDSP_PIP_MVSYNC_IRQ, vin_idsp_vin,},
};

static struct vin_controller *vin_controller_init(struct ambarella_iav *iav,
		int id, struct ambarella_vin_irq *vin_irq, int num_irq)
{
	struct device_node *np;
	struct vin_controller *vinc;
	struct proc_dir_entry *vsync_proc;
	char name[32];
	int rval = 0, i = 0;

	vinc = kzalloc(sizeof(struct vin_controller), GFP_KERNEL);
	if (!vinc) {
		vin_error("Out of memory: vin_controller!\n");
		return NULL;
	}

	snprintf(vinc->name, sizeof(vinc->name), "vin%d", id);
	vinc->id = id;
	vinc->irq_counter = 0;
	vinc->dev_active = NULL;
	vinc->iav = iav;
	init_waitqueue_head(&vinc->wq);
	mutex_init(&vinc->dev_mutex);
	INIT_LIST_HEAD(&vinc->dev_list);

	vinc->rst_gpio = -EINVAL;
	for (i = 0; i < 3; i++)
		vinc->pwr_gpio[i] = -EINVAL;
	snprintf(name, sizeof(name), "vinc%d", vinc->id);
	np = of_get_child_by_name(iav->of_node, name);
	if (vinc->iav->vin_enabled == 0)
		vin_init_pwr_rst_gpio(vinc, np);

	vinc->dsp_config = kzalloc(sizeof(struct vin_dsp_config), GFP_KERNEL);
	if (!vinc->dsp_config) {
		vin_error("Out of memory: vin_dsp_config!\n");
		goto vin_init_error0;
	}

	vinc->master_config = kzalloc(sizeof(struct vin_master_config), GFP_KERNEL);
	if (!vinc->master_config) {
		vin_error("Out of memory: vin_master_config!\n");
		goto vin_init_error1;
		return NULL;
	}

	highres_timer_init(&vinc->vsync_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vinc->vsync_timer.function = vin_vsync_timer;
	vinc->vsync_delay = 0;
	vinc->tmo = 2 * HZ;

	for (i = 0; i < num_irq; i++) {
		if (!vin_irq[i].irq_handler)
			continue;

		snprintf(vin_irq[i].irq_name, sizeof(vin_irq[i].irq_name),
				"vin%d_%s", id, vin_irq[i].name);
		rval = request_irq(vin_irq[i].irq_no,
				vin_irq[i].irq_handler,
				IRQF_TRIGGER_RISING | IRQF_SHARED,
				vin_irq[i].irq_name, vinc);
		if (rval) {
			vin_error("can't request IRQ(%d)!\n", vin_irq[i].irq_no);
			goto vin_init_error2;
		}
	}

	vinc->ioctl = vin_device_ioctl;
	ambsync_proc_hinit(&vinc->proc_hinfo);
	vinc->proc_hinfo.sync_read_proc = vin_vsync_proc_show;
	vinc->proc_hinfo.sync_read_data = vinc;
	vinc->proc_hinfo.tmo = 2 * HZ;

	list_add_tail(&vinc->list, &vin_controller_list);

	snprintf(name, sizeof(name), "vin%d_idsp", id);
	vsync_proc = proc_create_data(name, S_IRUGO,
			get_ambarella_proc_dir(), &vin_vsync_proc_fops, &vinc->proc_hinfo);
	if (!vsync_proc) {
		vin_error("failed to create proc file (vsync)!\n");
		goto vin_init_error2;
	}

	if (vinc->iav->vin_enabled == 0)
		ambarella_vin_set_phy(id, SENSOR_PARALLEL_LVCMOS, 0);

	vin_info("%s: probed!\n", vinc->name);

	return vinc;

vin_init_error2:
	for (; i > 0; i--)
		free_irq(vin_irq[i-1].irq_no, vinc);
	kfree(vinc->master_config);
vin_init_error1:
	kfree(vinc->dsp_config);
vin_init_error0:
	kfree(vinc);

	return NULL;
}

void vin_controller_exit(struct vin_controller *vinc)
{
	struct ambarella_vin_irq *vin_irq;
	char name[32];
	int i;

	snprintf(name, sizeof(name), "vin%d_idsp", vinc->id);
	remove_proc_entry(name, get_ambarella_proc_dir());

	list_del(&vinc->list);

	vin_irq = vin0_irq;

	for (i = 0; i < ARRAY_SIZE(vin0_irq); i++) {
		if (!vin_irq->irq_handler)
			continue;

		free_irq(vin_irq->irq_no, vinc);
		vin_irq->irq_handler = NULL;
		vin_irq++;
	}

	vin_irq = vin1_irq;

	for (i = 0; i < ARRAY_SIZE(vin1_irq); i++) {
		if (!vin_irq->irq_handler)
			continue;

		free_irq(vin_irq->irq_no, vinc);
		vin_irq->irq_handler = NULL;
		vin_irq++;
	}

	vin_exit_pwr_rst_gpio(vinc);

	kfree(vinc->dsp_config);
	kfree(vinc);
}

int iav_vin_init(struct ambarella_iav *iav)
{
	iav->vinc[0] = vin_controller_init(iav, 0, vin0_irq, ARRAY_SIZE(vin0_irq));
	iav->vinc[1] = vin_controller_init(iav, 1, vin1_irq, ARRAY_SIZE(vin1_irq));

	return 0;
}

void iav_vin_exit(struct ambarella_iav *iav)
{
	vin_controller_exit(iav->vinc[0]);
	vin_controller_exit(iav->vinc[1]);
}

int iav_vin_get_idsp_frame_rate(struct ambarella_iav *iav, u32 *idsp_out_frame_rate)
{
	int idsp_upsample_type = iav->system_config[iav->encode_mode].idsp_upsample_type;
	u32 vin_frame_rate = iav->vinc[0]->vin_format.frame_rate;

	switch (idsp_upsample_type) {
		case IDSP_UPSAMPLE_TYPE_OFF:
			*idsp_out_frame_rate = vin_frame_rate;
			break;

		case IDSP_UPSAMPLE_TYPE_25FPS:
			*idsp_out_frame_rate = AMBA_VIDEO_FPS_25;
			break;

		case IDSP_UPSAMPLE_TYPE_30FPS:
			*idsp_out_frame_rate = AMBA_VIDEO_FPS_30;
			break;

		default:
			iav_error("Unsupported idsp upsample type [%d]!\n", idsp_upsample_type);
			return -1;
	}

	return 0;
}

int iav_vin_idsp_cross_check(struct ambarella_iav *iav)
{
	u32 idsp_out_frame_rate;
	u32 vin_frame_rate = iav->vinc[0]->vin_format.frame_rate;

	if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
		return -1;
	}

	if (idsp_out_frame_rate > vin_frame_rate) {
		iav_error("IDSP out fps [%d] should never be smaller than VIN fps [%d]!\n",
			DIV_ROUND_CLOSEST(FPS_Q9_BASE, idsp_out_frame_rate),
			DIV_ROUND_CLOSEST(FPS_Q9_BASE, vin_frame_rate));
		return -1;
	}

	return 0;
}

