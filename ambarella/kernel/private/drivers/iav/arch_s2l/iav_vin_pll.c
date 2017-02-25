/*
 * kernel/private/drivers/ambarella/vin/vin_pll.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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

