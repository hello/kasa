/*
 * lcd_td043.c
 *
 * History:
 *	2010/09/28 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

/* ========================================================================== */
#include <linux/spi/spidev.h>

/* ========================================================================== */
static int lcd_td043_wvga(struct amba_video_sink_mode *pvout_cfg,
	enum amba_vout_lcd_mode_info lcd_mode)
{
	pvout_cfg->mode		= AMBA_VIDEO_MODE_WVGA;
	pvout_cfg->ratio	= AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits		= AMBA_VIDEO_BITS_16;
	pvout_cfg->type		= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format	= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y	= 0x10;
	pvout_cfg->bg_color.cb	= 0x80;
	pvout_cfg->bg_color.cr	= 0x80;

	pvout_cfg->lcd_cfg.mode	= lcd_mode;
	pvout_cfg->lcd_cfg.seqt	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 27000000;
	pvout_cfg->lcd_cfg.hvld_pol = AMBA_VOUT_LCD_HVLD_POL_HIGH;

	return 0;
}

int lcd_td043_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_WVGA:
		errCode = lcd_td043_wvga(pcfg, AMBA_VOUT_LCD_MODE_RGB888);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

int lcd_td043_16_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_WVGA:
		errCode = lcd_td043_wvga(pcfg, AMBA_VOUT_LCD_MODE_RGB565);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

