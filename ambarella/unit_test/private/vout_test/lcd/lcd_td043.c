/*
 * lcd_td043.c
 *
 * History:
 *	2010/09/28 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#include <linux/spi/spidev.h>

#define TD043_PWM_PATH(x)		"/sys/class/backlight/pwm-backlight.0/"x

#define TD043_WRITE_REGISTER(addr, val)	\
	cmd = ((addr) << 8) | (val);	\
	write(spi_fd, &cmd, 2);

/* ========================================================================== */
static void lcd_td043_config_wvga()
{
	/* Power On */
	lcd_power_on();

	/* Hardware Reset */
	//lcd_reset();

#ifdef LCD_BRIGHEST
	/* Backlight on */
	lcd_backlight_on();
#else
{
	int		ret;
	int		pwm_fd1, pwm_fd2;
	int		lcd_brightness;
	char		buf[64];

	pwm_fd1 = open(TD043_PWM_PATH("max_brightness"), O_RDONLY);
	if (pwm_fd1 < 0) {
		perror("Can't open max_brightness to read");
		goto lcd_td043_config_wvga_exit;
	}
	ret = read(pwm_fd1, buf, sizeof(buf));
	close(pwm_fd1);
	if (ret <= 0) {
		perror("Can't read max_brightness");
		goto lcd_td043_config_wvga_exit;
	} else {
		lcd_brightness = atoi(buf) / 2;
	}

	pwm_fd2 = open(TD043_PWM_PATH("brightness"), O_WRONLY);
	if (pwm_fd2 < 0) {
		perror("Can't open brightness to write");
		goto lcd_td043_config_wvga_exit;
	}
	sprintf(buf, "%d", lcd_brightness);
	ret = write(pwm_fd2, buf, sizeof(buf));
	close(pwm_fd2);
	if (ret <= 0) {
		perror("Can't write brightness");
		goto lcd_td043_config_wvga_exit;
	}
}
#endif

lcd_td043_config_wvga_exit:
	return;
}

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

	lcd_td043_config_wvga();

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

