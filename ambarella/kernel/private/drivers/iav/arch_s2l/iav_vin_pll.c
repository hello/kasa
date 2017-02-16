/*
 * kernel/private/drivers/ambarella/vin/vin_pll.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <plat/clk.h>
#include <iav_utils.h>
#include <vin_api.h>
#include "iav_vin.h"

/* ==========================================================================*/
static struct clk gclk_so = {
	.parent		= NULL,
	.name		= "gclk_so",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_SENSOR_CTRL_REG,
	.pres_reg	= SCALER_SENSOR_PRE_REG,
	.post_reg	= SCALER_SENSOR_POST_REG,
	.frac_reg	= PLL_SENSOR_FRAC_REG,
	.ctrl2_reg	= PLL_SENSOR_CTRL2_REG,
	.ctrl3_reg	= PLL_SENSOR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 3,
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
	.table		= ambarella_pll_frac_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_frac_table),
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk *rct_register_clk_so(void)
{
	struct clk *pgclk_so = NULL;

	pgclk_so = clk_get(NULL, "gclk_so");
	if (IS_ERR(pgclk_so)) {
		ambarella_clk_add(&gclk_so);
		pgclk_so = &gclk_so;
		pr_info("SYSCLK:SO[%lu]\n", clk_get_rate(pgclk_so));
	}

	return pgclk_so;
}

/* ========================================================================== */
void rct_set_so_clk_src(u32 mode)
{
	amba_rct_writel(USE_CLK_SI_INPUT_MODE_REG, (mode & 0x1));
}

void rct_set_so_freq_hz(u32 freq_hz)
{
	clk_set_rate(rct_register_clk_so(), freq_hz);
}

u32 rct_get_so_freq_hz(void)
{
	return clk_get_rate(rct_register_clk_so());
}

