/*
 * arch/arm/mach-ambarella/pll.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <mach/init.h>
#include <plat/clk.h>
#include <plat/fio.h>
#include <plat/sd.h>
#include <plat/spi.h>

struct clk_ops ambarella_rct_pll_ops = {
	.enable		= ambarella_rct_clk_enable,
	.disable	= ambarella_rct_clk_disable,
	.get_rate	= ambarella_rct_clk_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_rct_clk_set_rate,
	.set_parent	= NULL,
};
EXPORT_SYMBOL(ambarella_rct_pll_ops);

struct clk_ops ambarella_rct_scaler_ops = {
	.enable		= NULL,
	.disable	= NULL,
	.get_rate	= ambarella_rct_scaler_get_rate,
	.round_rate	= NULL,
	.set_rate	= ambarella_rct_scaler_set_rate,
	.set_parent	= NULL,
};
EXPORT_SYMBOL(ambarella_rct_scaler_ops);

static struct clk pll_out_core = {
	.parent		= NULL,
	.name		= "pll_out_core",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_CORE_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_CORE_FRAC_REG,
	.ctrl2_reg	= PLL_CORE_CTRL2_REG,
	.ctrl3_reg	= PLL_CORE_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 6,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk gclk_core = {
	.parent		= &pll_out_core,
	.name		= "gclk_core",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
#if ((CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E))
	.post_reg	= SCALER_CORE_POST_REG,
#else
	.post_reg	= -1,
#endif
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
#if ((CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E))
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
#else
	.divider	= 2,
	.max_divider	= 0,
#endif
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ahb = {
	.parent		= &gclk_core,
	.name		= "gclk_ahb",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
#if (CHIP_REV == A5S)
	.divider	= 1,
#else
	.divider	= 2,
#endif
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_apb = {
	.parent		= &gclk_ahb,
	.name		= "gclk_apb",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 2,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ddr = {
	.parent		= NULL,
	.name		= "gclk_ddr",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_DDR_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_DDR_FRAC_REG,
	.ctrl2_reg	= PLL_DDR_CTRL2_REG,
	.ctrl3_reg	= PLL_DDR_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 5,
	.divider	= 2,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};

/* ==========================================================================*/
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
static struct clk gclk_cortex = {
	.parent		= NULL,
	.name		= "gclk_cortex",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_CORTEX_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_CORTEX_FRAC_REG,
	.ctrl2_reg	= PLL_CORTEX_CTRL2_REG,
	.ctrl3_reg	= PLL_CORTEX_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 2,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};
static struct clk gclk_axi = {
	.parent		= &gclk_cortex,
	.name		= "gclk_axi",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 3,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#if defined(CONFIG_HAVE_ARM_TWD)
static struct clk clk_smp_twd = {
	.parent		= &gclk_axi,
	.name		= "smp_twd",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 1,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif
#endif

static struct clk gclk_idsp = {
	.parent		= NULL,
	.name		= "gclk_idsp",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= PLL_IDSP_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= SCALER_IDSP_POST_REG,
	.frac_reg	= PLL_IDSP_FRAC_REG,
	.ctrl2_reg	= PLL_IDSP_CTRL2_REG,
	.ctrl3_reg	= PLL_IDSP_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 4,
	.divider	= 0,
	.max_divider	= (1 << 4) - 1,
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
	.extra_scaler	= 1,
#else
	.extra_scaler	= 0,
#endif
	.table		= ambarella_pll_frac_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_frac_table),
	.ops		= &ambarella_rct_pll_ops,
};

static struct clk gclk_uart = {
#if (CHIP_REV == S2E)
	.parent		= &gclk_idsp,
#else
	.parent		= NULL,
#endif
	.name		= "gclk_uart",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_UART_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_audio = {
	.parent		= NULL,
	.name		= "gclk_audio",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_AUDIO_CTRL_REG,
	.pres_reg	= SCALER_AUDIO_PRE_REG,
	.post_reg	= SCALER_AUDIO_POST_REG,
	.frac_reg	= PLL_AUDIO_FRAC_REG,
	.ctrl2_reg	= PLL_AUDIO_CTRL2_REG,
	.ctrl3_reg	= PLL_AUDIO_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 7,
	.divider	= 0,
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
	.max_divider	= (1 << 4) - 1,
	.extra_scaler	= 1,
#else
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
#endif
	.table		= ambarella_pll_frac_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_frac_table),
	.ops		= &ambarella_rct_pll_ops,
};

#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3)
static struct clk pll_out_sd = {
	.parent		= NULL,
	.name		= "pll_out_sd",
	.rate		= 0,
	.frac_mode	= 1,
	.ctrl_reg	= PLL_SD_CTRL_REG,
	.pres_reg	= -1,
	.post_reg	= -1,
	.frac_reg	= PLL_SD_FRAC_REG,
	.ctrl2_reg	= PLL_SD_CTRL2_REG,
	.ctrl3_reg	= PLL_SD_CTRL3_REG,
	.lock_reg	= PLL_LOCK_REG,
	.lock_bit	= 12,
	.divider	= 0,
	.max_divider	= 0,
	.extra_scaler	= 0,
	.table		= ambarella_pll_int_table,
	.table_size	= ARRAY_SIZE(ambarella_pll_int_table),
	.ops		= &ambarella_rct_pll_ops,
};
#endif

#if (SD_INSTANCES >= 3)
static struct clk gclk_sdxc = {
	.parent		= &pll_out_sd,
	.name		= "gclk_sdxc",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SDXC_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

#if (SD_INSTANCES >= 2)
static struct clk gclk_sdio = {
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3)
	.parent		= &pll_out_sd,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_sdio",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SDIO_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

static struct clk gclk_sd = {
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3)
	.parent		= &pll_out_sd,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_sd",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_SD48_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ir = {
	.parent		= NULL,
	.name		= "gclk_ir",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_IR_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_adc = {
	.parent		= NULL,
	.name		= "gclk_adc",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= SCALER_ADC_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 2,
	.max_divider	= (1 << 16) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ssi = {	/* for SSI master */
#if (CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E)
	.parent		= &gclk_apb,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_ssi",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

static struct clk gclk_ssi2 = {	/* for SSI slave */
#if (CHIP_REV == A5S) || (CHIP_REV == S2) || (CHIP_REV == S2E)
	.parent		= &gclk_apb,
#else
	.parent		= &pll_out_core,
#endif
	.name		= "gclk_ssi2",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI2_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

#if (CHIP_REV == S2L) || (CHIP_REV == S3)
static struct clk gclk_ssi3 = {	/* for SPINOR */
	/* TODO: parent is determined by CLK_REF_SSI3_REG */
	.parent		= &pll_out_core,
	.name		= "gclk_ssi3",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_SSI3_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};
#endif

static struct clk gclk_pwm = {
	.parent		= &gclk_apb,
	.name		= "gclk_pwm",
	.rate		= 0,
	.frac_mode	= 0,
	.ctrl_reg	= -1,
	.pres_reg	= -1,
	.post_reg	= CG_PWM_REG,
	.frac_reg	= -1,
	.ctrl2_reg	= -1,
	.ctrl3_reg	= -1,
	.lock_reg	= -1,
	.lock_bit	= 0,
	.divider	= 0,
	.max_divider	= (1 << 24) - 1,
	.extra_scaler	= 0,
	.ops		= &ambarella_rct_scaler_ops,
};

void ambarella_init_early(void)
{
	ambarella_clk_add(&pll_out_core);
#if (CHIP_REV == S2E) || (CHIP_REV == S2L) || (CHIP_REV == S3)
	ambarella_clk_add(&pll_out_sd);
#endif
#if defined(CONFIG_PLAT_AMBARELLA_CORTEX)
	ambarella_clk_add(&gclk_cortex);
	ambarella_clk_add(&gclk_axi);
#if defined(CONFIG_HAVE_ARM_TWD)
	ambarella_clk_add(&clk_smp_twd);
#endif
#endif
	ambarella_clk_add(&gclk_ddr);
	ambarella_clk_add(&gclk_core);
	ambarella_clk_add(&gclk_ahb);
	ambarella_clk_add(&gclk_apb);
	ambarella_clk_add(&gclk_idsp);

#if (CHIP_REV == S2E)
	amba_rct_writel(UART_CLK_SRC_SEL_REG, UART_CLK_SRC_IDSP);
#endif
	ambarella_clk_add(&gclk_uart);
	ambarella_clk_add(&gclk_audio);
#if (SD_INSTANCES >= 3)
	ambarella_clk_add(&gclk_sdxc);
#endif
#if (SD_INSTANCES >= 2)
	ambarella_clk_add(&gclk_sdio);
#endif
	ambarella_clk_add(&gclk_sd);
	ambarella_clk_add(&gclk_ir);
	ambarella_clk_add(&gclk_adc);
	ambarella_clk_add(&gclk_ssi);
	ambarella_clk_add(&gclk_ssi2);
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
	ambarella_clk_add(&gclk_ssi3);
#endif
	ambarella_clk_add(&gclk_pwm);
}

