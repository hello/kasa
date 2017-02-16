/*
 * lcd_digital.c
 *
 * History:
 *	2009/05/20 - [Anthony Ginger] created file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static int lcd_digital601_init(struct amba_video_sink_mode *pvout_cfg, int mode)
{
	pvout_cfg->mode = mode;
	pvout_cfg->ratio = AMBA_VIDEO_RATIO_AUTO;
	pvout_cfg->bits = AMBA_VIDEO_BITS_AUTO;
	pvout_cfg->type = AMBA_VIDEO_TYPE_YUV_601;
	pvout_cfg->format = AMBA_VIDEO_FORMAT_AUTO;
	pvout_cfg->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y = 0x10;
	pvout_cfg->bg_color.cb = 0x80;
	pvout_cfg->bg_color.cr = 0x80;

	pvout_cfg->lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

	return 0;
}

int lcd_digital601_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int					errCode = 0;

	errCode = lcd_digital601_init(pcfg, mode);

	return errCode;
}
