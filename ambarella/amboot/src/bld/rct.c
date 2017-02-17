/**
 * bld/rct.c
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


#include <bldfunc.h>
#include <ambhw/dma.h>
#include <ambhw/fio.h>
#include <ambhw/i2s.h>
#include <ambhw/timer.h>
#include <sdmmc.h>

/* ==========================================================================*/
#define PLL_CTRL_INTPROG(x)			((x >> 24) & 0x7F)
#define PLL_CTRL_SOUT(x)			((x >> 16) & 0xF)
#define PLL_CTRL_SDIV(x)			((x >> 12) & 0xF)
#define PLL_CTRL_FRAC_MODE(x)			(x & 0x8)
#define PLL_FRAC_VAL(f)				(f & 0x7FFFFFFF)
#define PLL_FRAC_VAL_NEGA(f)			(f & 0x80000000)
#define PLL_SCALER_JDIV(x)			(((x >> 4) & 0xF) + 1)

u32 rct_get_integer_pll_freq(u32 c, u32 pres, u32 posts)
{
	u32 intprog;
	u32 sout;
	u32 sdiv;

	if (c & 0x20)
		return 0;

	if (c & 0x00200004) {
		intprog = REF_CLK_FREQ;
		intprog /= pres;
		intprog /= posts;
		return intprog;
	}

	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);

	intprog++;
	sdiv++;
	sout++;

	intprog *= REF_CLK_FREQ;
	intprog /= sout;
	intprog *= sdiv;
	intprog /= pres;
	intprog /= posts;

	return intprog;

}

u32 rct_get_frac_pll_freq(u32 c, u32 f, u32 pres, u32 posts)
{
	u32 intprog;
	u32 sout;
	u32 sdiv;
	u32 frac;

	if (c & 0x20)
		return 0;

	if (c & 0x00200004) {
		intprog = REF_CLK_FREQ;
		intprog /= pres;
		intprog /= posts;
		return intprog;
	}

	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);
	frac = PLL_FRAC_VAL(f);

	intprog++;
	sdiv++;
	sout++;

	intprog *= REF_CLK_FREQ;
	intprog /= sout;
	intprog *= sdiv;
	intprog /= pres;
	intprog /= posts;

	if (PLL_CTRL_FRAC_MODE(c)) {
		if (PLL_FRAC_VAL_NEGA(f)) {
			frac = 0x80000000 - frac;
		}

		frac >>= 16;
		frac *= (REF_CLK_FREQ >> 8);
		frac /= sout;
		frac /= pres;
		frac /= posts;
		frac >>= 8;
		frac *= sdiv;

		if (PLL_FRAC_VAL_NEGA(f)) {
			intprog -= frac;
		} else {
			intprog += frac;
		}
	}

	return intprog;
}

/* ==========================================================================*/
u32 get_core_bus_freq_hz(void)
{
	return (rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG), 1, 1) >> 1);
}

u32 get_ahb_bus_freq_hz(void)
{
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	return (get_core_bus_freq_hz() >> (readl(CORE_CLK_RATIO_1X_REG)^0x1));
#elif (CHIP_REV == A7L)
	return (get_core_bus_freq_hz());
#else
	return (get_core_bus_freq_hz() >> 1);
#endif
}

u32 get_apb_bus_freq_hz(void)
{
	return get_ahb_bus_freq_hz() >> 1;
}

/* ==========================================================================*/
u32 get_idsp_freq_hz(void)
{
	return rct_get_integer_pll_freq(readl(PLL_IDSP_CTRL_REG),
		1, PLL_SCALER_JDIV(readl(SCALER_IDSP_POST_REG)));
}

/* ==========================================================================*/
u32 get_ddr_freq_hz(void)
{
	return rct_get_integer_pll_freq(readl(PLL_DDR_CTRL_REG), 1, 1) >> 1;
}

/* ==========================================================================*/
u32 get_arm_bus_freq_hz(void)
{
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	return rct_get_integer_pll_freq(readl(PLL_IDSP_CTRL_REG),
		1, readl(SCALER_ARM_ASYNC_REG));
#else
	return 0;
#endif
}

/* ==========================================================================*/
u32 get_cortex_freq_hz(void)
{
#if (CHIP_REV == A5S)
	return 0;
#else
	return rct_get_frac_pll_freq(readl(PLL_CORTEX_CTRL_REG),
		readl(PLL_CORTEX_FRAC_REG), 1, 1);
#endif
}

u32 get_enet_freq_hz(void)
{
#if (CHIP_REV == S2)
	return (get_cortex_freq_hz() / readl(SCALER_GTX_POST_REG));
#elif (CHIP_REV == S2L) || (CHIP_REV == S3L)
	return rct_get_frac_pll_freq(readl(PLL_ENET_CTRL_REG),
		readl(PLL_ENET_FRAC_REG), 1,
		PLL_SCALER_JDIV(readl(SCALER_ENET_POST_REG)));
#elif (CHIP_REV == S2E)
        if(SDVCO_FOR_GTX_SOURCE && readl(FUNC_MISC_CTRL_REG)){
                return (rct_get_frac_pll_freq(readl(PLL_SD_CTRL_REG),
                        readl(PLL_SD_FRAC_REG),1, 1)
                        / readl(SCALER_GTX_POST_REG));
        }else{
	        return (get_cortex_freq_hz() / readl(SCALER_GTX_POST_REG));
        }
#else
	return 0;
#endif
}

/* ==========================================================================*/
void rct_set_uart_pll(void)
{
	writel(CG_UART_REG, 0x1);
}

u32 get_uart_freq_hz(void)
{
	return (REF_CLK_FREQ / readl(CG_UART_REG));
}

/* ==========================================================================*/
u32 rct_timer_tick2ms(u32 s_tck, u32 e_tck)
{
#if (RCT_TIMER_INSTANCES >= 1)
	return (e_tck - s_tck) / (REF_CLK_FREQ / 1000);
#else
	return (s_tck - e_tck) / (get_apb_bus_freq_hz() / 1000);
#endif
}

void rct_timer_reset_count()
{
	/* reset timer */
#if (RCT_TIMER_INSTANCES >= 1)
	writel(RCT_TIMER_CTRL_REG, 0x1);
#else
	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) & (~0xF0)));
	writel(TIMER2_STATUS_REG, 0xFFFFFFFF);
	writel(TIMER2_RELOAD_REG, 0x00000000);
	writel(TIMER2_MATCH1_REG, 0x00000000);
	writel(TIMER2_MATCH2_REG, 0x00000000);
#endif

	/* enable timer */
#if (RCT_TIMER_INSTANCES >= 1)
	writel(RCT_TIMER_CTRL_REG, 0x0);
#else
	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) | 0x10));
#endif
}

u32 rct_timer_get_count()
{
#if (RCT_TIMER_INSTANCES >= 1)
	return rct_timer_tick2ms(0x00000000, readl(RCT_TIMER_REG));
#else
	return rct_timer_tick2ms(0xFFFFFFFF, readl(TIMER2_STATUS_REG));
#endif
}

u32 rct_timer_get_tick()
{
#if (RCT_TIMER_INSTANCES >= 1)
	return readl(RCT_TIMER_REG);
#else
	return readl(TIMER2_STATUS_REG);
#endif
}

void rct_timer_dly_ms(u32 dly_tim)
{
	u32 cur_tim;

	rct_timer_reset_count();
	while (1) {
		cur_tim = rct_timer_get_count();
		if (cur_tim >= dly_tim)
			break;
	}
}

u32 rct_timer2_tick2ms(u32 s_tck, u32 e_tck)
{
#if (RCT_TIMER_INSTANCES >= 2)
	return (e_tck - s_tck) / (REF_CLK_FREQ / 1000);
#else
	return (s_tck - e_tck) / (get_apb_bus_freq_hz() / 1000);
#endif
}

void rct_timer2_reset_count()
{
	/* reset timer */
#if (RCT_TIMER_INSTANCES >= 2)
	writel(RCT_TIMER2_CTRL_REG, 0x1);
#else
	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) & (~0xF00)));
	writel(TIMER3_STATUS_REG, 0xFFFFFFFF);
	writel(TIMER3_RELOAD_REG, 0x00000000);
	writel(TIMER3_MATCH1_REG, 0x00000000);
	writel(TIMER3_MATCH2_REG, 0x00000000);
#endif

	/* enable timer */
#if (RCT_TIMER_INSTANCES >= 2)
	writel(RCT_TIMER2_CTRL_REG, 0x0);
#else
	writel(TIMER_CTR_REG, (readl(TIMER_CTR_REG) | 0x100));
#endif
}

u32 rct_timer2_get_count()
{
#if (RCT_TIMER_INSTANCES >= 2)
	return rct_timer2_tick2ms(0x00000000, readl(RCT_TIMER2_REG));
#else
	return rct_timer2_tick2ms(0xFFFFFFFF, readl(TIMER3_STATUS_REG));
#endif
}

u32 rct_timer2_get_tick()
{
#if (RCT_TIMER_INSTANCES >= 2)
	return readl(RCT_TIMER2_REG);
#else
	return readl(TIMER3_STATUS_REG);
#endif
}

void rct_timer2_dly_ms(u32 dly_tim)
{
	u32 cur_tim;

	rct_timer2_reset_count();
	while (1) {
		cur_tim = rct_timer2_get_count();
		if (cur_tim >= dly_tim)
			break;
	}
}

/* ==========================================================================*/

/*
 * Bit 2 is to never suspend usbphy. So if this bit is set, usbphy power
 * consumption will not be reduced.
 *
 * Bit 1: this bit has 2 purposes:
 *  A) initiate usbphy power on sequence, can be done only once after cold boot.
 *  B) after the bit has been set once, if you clear it, it will suspend
 *     the usbphy when bit 2 is also cleared.
 */
void rct_enable_usb(void)
{
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 0x3006);
#else
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 0x6);
#endif

#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	/* force to set USBPHY0 as device */
	writel(USBP0_SEL_REG, readl(USBP0_SEL_REG) | 0x3);
#endif
}

void rct_suspend_usb(void)
{
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x3006);
#else
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x6);
#endif
}

void rct_usb_reset(void)
{
	u32 pwr_val;

	pwr_val = readl(ANA_PWR_REG);

	writel(ANA_PWR_REG, pwr_val | 0x4);
	rct_timer_dly_ms(1);

	/* reset udc */
	setbitsl(UDC_SOFT_RESET_REG, UDC_SOFT_RESET_MASK);
	rct_timer_dly_ms(1);
	clrbitsl(UDC_SOFT_RESET_REG, UDC_SOFT_RESET_MASK);
	rct_timer_dly_ms(1);

	writel(ANA_PWR_REG, pwr_val);
	rct_timer_dly_ms(1);
}

/* ==========================================================================*/
u32 rct_boot_from(void)
{
	u32 sysconfig;

	sysconfig = readl(SYS_CONFIG_REG);
	sysconfig &= POC_BOOT_FROM_MASK;

#if (POC_BOOT_MAP_TYPE == 0)
	if ((sysconfig & POC_BOOT_FROM_BYPASS) == POC_BOOT_FROM_BYPASS) {
		return RCT_BOOT_FROM_BYPASS;
	}
	if (sysconfig == POC_BOOT_FROM_USB) {
		return RCT_BOOT_FROM_USB;
	}
	if ((sysconfig & POC_BOOT_FROM_EMMC) == POC_BOOT_FROM_EMMC) {
		return RCT_BOOT_FROM_EMMC;
	}
	if ((sysconfig & POC_BOOT_FROM_NAND) == POC_BOOT_FROM_NAND) {
		return RCT_BOOT_FROM_NAND;
	}
	if ((sysconfig & POC_BOOT_FROM_SPINOR) == POC_BOOT_FROM_SPINOR) {
		return RCT_BOOT_FROM_SPINOR;
	}
#else
	if ((sysconfig & POC_BOOT_FROM_BYPASS) == POC_BOOT_FROM_BYPASS) {
		return RCT_BOOT_FROM_BYPASS;
	}
	if ((sysconfig & POC_BOOT_FROM_USB) == POC_BOOT_FROM_USB) {
		return RCT_BOOT_FROM_USB;
	}
	sysconfig &= (~POC_BOOT_FROM_BYPASS);
	sysconfig &= (~POC_BOOT_FROM_USB);
	if (sysconfig == POC_BOOT_FROM_NAND) {
		return RCT_BOOT_FROM_NAND;
	}
	if (sysconfig == POC_BOOT_FROM_SPINOR) {
		return RCT_BOOT_FROM_SPINOR;
	}
	if (sysconfig == POC_BOOT_FROM_EMMC) {
		return RCT_BOOT_FROM_EMMC;
	}
	if (sysconfig == POC_BOOT_FROM_HIF) {
		return RCT_BOOT_FROM_HIF;
	}
#endif

	return RCT_BOOT_FROM_NAND;
}

u32 rct_is_eth_enabled(void)
{
	u32 sys_config = readl(SYS_CONFIG_REG);

#if (CHIP_REV == S2) || (CHIP_REV == S2E)
	return ((sys_config & 0x00800000) != 0x0);
#elif (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	return ((sys_config & 0x00000001) != 0x0);
#else
	return ((sys_config & 0x00000080) != 0x0);
#endif
}

u32 rct_get_nand_poc(void)
{
	u32 ret_val = RCT_BOOT_NAND_AUTO;
	u32 sysconfig = readl(SYS_CONFIG_REG);

	if (sysconfig & SYS_CONFIG_NAND_ECC_BCH_EN) {
		ret_val |= RCT_BOOT_NAND_ECC_BCH_EN;
	}
	if (sysconfig & SYS_CONFIG_NAND_ECC_SPARE_2X) {
		ret_val |= RCT_BOOT_NAND_ECC_SPARE_2X;
	}

	if (sysconfig & SYS_CONFIG_NAND_PAGE_SIZE) {
		ret_val |= RCT_BOOT_NAND_PAGE_SIZE;
	}
	if (sysconfig & SYS_CONFIG_NAND_READ_CONFIRM) {
		ret_val |= RCT_BOOT_NAND_READ_CONFIRM;
	}

	return ret_val;
}

u32 rct_get_emmc_poc(void)
{
	u32 ret_val = RCT_BOOT_EMMC_AUTO;
	u32 sysconfig = readl(SYS_CONFIG_REG);

	if (sysconfig & SYS_CONFIG_MMC_HS) {
		ret_val |= RCT_BOOT_EMMC_HS;
	}
	if (sysconfig & SYS_CONFIG_MMC_DDR) {
		ret_val |= RCT_BOOT_EMMC_DDR;
	}
	if (sysconfig & SYS_CONFIG_MMC_4BIT) {
		ret_val |= RCT_BOOT_EMMC_4BIT;
	}
	if (sysconfig & SYS_CONFIG_MMC_8BIT) {
		ret_val |= RCT_BOOT_EMMC_8BIT;
	}

	return ret_val;
}

/* ==========================================================================*/
void rct_pll_init(void)
{
}

void rct_reset_chip(void)
{
	if (amboot_rct_reset_chip_pre != NULL) {
		amboot_rct_reset_chip_pre();
	} else {
		if (amboot_rct_reset_chip_sdmmc != NULL) {
			amboot_rct_reset_chip_sdmmc();
		}
	}
	writel(SOFT_OR_DLL_RESET_REG, 0x2);
	writel(SOFT_OR_DLL_RESET_REG, 0x3);
}

/* ==========================================================================*/
void enable_fio_dma(void)
{
	u32 val;

	val = readl(I2S_24BITMUX_MODE_REG);
	val &= ~(I2S_24BITMUX_MODE_DMA_BOOTSEL);
	val &= ~(I2S_24BITMUX_MODE_FDMA_BURST_DIS);
	writel(I2S_24BITMUX_MODE_REG, val);
}

void rct_reset_fio(void)
{
	volatile int c;

	writel(FIO_RESET_REG, (FIO_RESET_FIO_RST | FIO_RESET_CF_RST |
		FIO_RESET_XD_RST | FIO_RESET_FLASH_RST));
	for (c = 0; c < 0xffff; c++);
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);
}

void fio_exit_random_mode(void)
{
	u32 fio_ctr;

	/* Exit random read mode */
	fio_ctr = readl(FIO_CTR_REG);
	fio_ctr &= (~FIO_CTR_RR);
	writel(FIO_CTR_REG, fio_ctr);
}

/* ==========================================================================*/
void rct_set_sd_pll(u32 freq_hz)
{
	u32 parent_rate, divider, post_scaler;
	u32 max_divider  = (1 << 16) - 1;

#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	parent_rate = rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG), 1, 1);
#else
	parent_rate = rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG), 1, 1);
#endif
	divider = (parent_rate + freq_hz - 1) / freq_hz ;

	if(divider > max_divider)
		divider = max_divider;

	post_scaler = divider;
	post_scaler &= 0xffff;
	writel(SCALER_SD48_REG, post_scaler);
}

u32 get_sd_freq_hz(void)
{
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	return rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG),
		1, readl(SCALER_SD48_REG));
#else
	return rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG),
		1, readl(SCALER_SD48_REG));
#endif
}

void rct_set_sdio_pll(u32 freq_hz)
{
	u32 divider, post_scaler;
	unsigned long parent_rate;
	u32 max_divider  = (1 << 16) - 1;

#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	parent_rate = rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG), 1, 1);
#else
	parent_rate = rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG), 1, 1);
#endif

	divider = (parent_rate + freq_hz - 1) / freq_hz ;

	if(divider > max_divider)
		divider = max_divider;

	post_scaler = divider;
	writel(SCALER_SDIO_REG, post_scaler);
}

u32 get_sdio_freq_hz(void)
{
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
	return rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG),
		1, readl(SCALER_SDIO_REG));
#elif (CHIP_REV == S3L)
	return 0;
#else
	return rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG),
		1, readl(SCALER_SDIO_REG));
#endif
}

void rct_set_sdxc_pll(u32 freq_hz)
{
	u32 divider, post_scaler;
	unsigned long parent_rate;
	u32 max_divider  = (1 << 16) - 1;

#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	parent_rate = rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG), 1, 1);
#else
	parent_rate = rct_get_integer_pll_freq(readl(PLL_CORE_CTRL_REG), 1, 1);
#endif

	divider = (parent_rate + freq_hz - 1) / freq_hz ;

	if(divider > max_divider)
		divider = max_divider;

	post_scaler = divider;
	writel(SCALER_SDXC_REG, post_scaler);
}

u32 get_sdxc_freq_hz(void)
{
#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
	return rct_get_integer_pll_freq(readl(PLL_SD_CTRL_REG),
		1, readl(SCALER_SDXC_REG));
#else
	return 0;
#endif
}

/* ==========================================================================*/
void rct_set_ir_pll(void)
{
        //default set ir clock to 13K
        writel(CG_IR_REG, 0x736);
}

/* ==========================================================================*/
void rct_set_ssi_pll(void)
{
#if (CHIP_REV == S2L) || (CHIP_REV == S3L)
	writel(CLK_REF_SSI_REG,0x1);  /* use core_2x as reference clock */
	writel(CG_SSI_REG, 2); /* use the same frequency as core clock */
#endif
}

u32 get_ssi_freq_hz(void)
{
	u32 val;

	val = readl(CG_SSI_REG);
	if (val & 0x01000000)
		return 0;

	if (val == 0)
		val = 1;
#if (CHIP_REV == S2L) || (CHIP_REV == S3L)
	return (get_core_bus_freq_hz() << 1) / val;
#else
	return 0;
#endif
}

u32 get_ssi2_freq_hz(void)
{
	return 0;
}
#if (CHIP_REV == S2L)
#define PLL_OUT_ENET	300000000
#endif
u32 get_ssi3_freq_hz(void)
{
	u32 val;

	val = readl(CG_SSI3_REG);
	if (val & 0x01000000)
		return 0;

	if (val == 0)
		val = 1;
#if (CHIP_REV == S2L)
	return (PLL_OUT_ENET) / val;
#else
	return (get_core_bus_freq_hz() << 1) / val;
#endif
}

void rct_set_ssi3_pll(void)
{
#if (CHIP_REV == S2L) || (CHIP_REV == S3L)
	writel(CLK_REF_SSI3_REG,0x0);  /* use pll_out_enet as reference clock */
	writel(CG_SSI3_REG, 2); /* use pll_out_enet/2 as spi nor reference clock */
#else
	writel(CLK_REF_SSI3_REG,0x1);  /* use core_2x as reference clock */
	writel(CG_SSI3_REG, 2); /* use the same frequency as core clock */
#endif
}

/* ==========================================================================*/
void rct_set_so_pll(void)
{
	// FIXME: Currently fixed so pll at 24MHz
	writel(CLK_SI_INPUT_MODE_REG, 0x0);
	writel(PLL_SENSOR_FRAC_REG, 0xf8780000);
	writel(PLL_SENSOR_CTRL_REG, 0x111b1108);
	writel(PLL_SENSOR_CTRL_REG, 0x111b1109);
	writel(PLL_SENSOR_CTRL_REG, 0x111b1108);
	writel(SCALER_SENSOR_POST_REG, 0x00000021);
	writel(SCALER_SENSOR_POST_REG, 0x00000020);
	writel(PLL_SENSOR_CTRL2_REG, 0x3f770000);
	writel(PLL_SENSOR_CTRL3_REG, 0x00069300);
}

void rct_set_vo_pll(void)
{
	// FIXME: Currently fixed vo pll at 28397174Hz
	writel(SCALER_VIDEO2_PRE_REG, 0x00000030);
	writel(SCALER_VIDEO2_POST_REG, 0x00000090);
	writel(SCALER_VIDEO2_PRE_REG, 0x00000031);
	writel(SCALER_VIDEO2_POST_REG, 0x00000091);
	writel(SCALER_VIDEO2_PRE_REG, 0x00000030);
	writel(SCALER_VIDEO2_POST_REG, 0x00000090);
	writel(PLL_VIDEO2_CTRL_REG, 0x3E132108);
	writel(PLL_VIDEO2_FRAC_REG, 0x1AD63632);
	writel(PLL_VIDEO2_CTRL2_REG, 0x3F770000);
	writel(PLL_VIDEO2_CTRL3_REG, 0x00068306);
	writel(PLL_VIDEO2_CTRL_REG, 0x3E132109);
	writel(PLL_VIDEO2_CTRL_REG, 0x3E132108);
}

void rct_set_vo2_pll(void)
{
	// FIXME: Currently fixed vo2 pll at 27MHz
	writel(SCALER_HDMI_PRE_REG, 0x00000030);
	writel(SCALER_HDMI_PRE_REG, 0x00000031);
	writel(SCALER_HDMI_PRE_REG, 0x00000030);
	writel(PLL_HDMI_CTRL_REG, 0x2C100100);
	writel(PLL_HDMI_FRAC_REG, 0x00000000);
	writel(PLL_HDMI_CTRL2_REG, 0x3F770000);
	writel(PLL_HDMI_CTRL3_REG, 0x00068302);
	writel(PLL_HDMI_CTRL_REG, 0x2C100101);
	writel(PLL_HDMI_CTRL_REG, 0x2C100100);
}

/* ==========================================================================*/
void rct_set_audio_pll(void)
{
	// FIXME: Currently fixed audio pll at 12.288MHz
	writel(SCALER_AUDIO_POST_REG, 0x00000040);
	writel(SCALER_AUDIO_POST_REG, 0x00000041);
	writel(SCALER_AUDIO_POST_REG, 0x00000040);
	writel(PLL_AUDIO_CTRL_REG, 0x111d1108);
	writel(PLL_AUDIO_FRAC_REG, 0xeb852237);
	writel(PLL_AUDIO_CTRL2_REG, 0x3f770000);
	writel(PLL_AUDIO_CTRL3_REG, 0x00069300);
	writel(PLL_AUDIO_CTRL_REG, 0x111d1109);
	writel(PLL_AUDIO_CTRL_REG, 0x111d1108);
}

/* ==========================================================================*/
void rct_show_pll(void)
{
	int pll_freq;
	u32 sysconfig;

	sysconfig = readl(SYS_CONFIG_REG);
	putstr("SYS_CONFIG: 0x");
	puthex(sysconfig);
	putstr(" POC: ");
	putbin(((sysconfig >> 1) & 0x7), 3, 1);
	putstr("\r\n");

	pll_freq = get_cortex_freq_hz();
	if (pll_freq) {
		putstr("Cortex freq: ");
		putdec(pll_freq / 1000);
		putstr("000\r\n");
	}
	pll_freq = get_enet_freq_hz();
	if (pll_freq) {
		putstr("ENET freq: ");
		putdec(pll_freq / 1000);
		putstr("000\r\n");
	}
	pll_freq = get_arm_bus_freq_hz();
	if (pll_freq) {
		putstr("ARM freq: ");
		putdec(pll_freq / 1000);
		putstr("000\r\n");
	}
	putstr("iDSP freq: ");
	putdec(get_idsp_freq_hz());
	putstr("\r\n");
	pll_freq = get_ddr_freq_hz();
	if (pll_freq) {
		putstr("Dram freq: ");
		putdec(pll_freq / 1000);
		putstr("000\r\n");
	}
	putstr("Core freq: ");
	putdec(get_core_bus_freq_hz());
	putstr("\r\n");
	putstr("AHB freq: ");
	putdec(get_ahb_bus_freq_hz());
	putstr("\r\n");
	putstr("APB freq: ");
	putdec(get_apb_bus_freq_hz());
	putstr("\r\n");
	putstr("UART freq: ");
	putdec(get_uart_freq_hz());
	putstr("\r\n");
	pll_freq = get_sd_freq_hz();
	if (pll_freq) {
		putstr("SD freq: ");
		putdec(pll_freq);
		putstr("\r\n");
	}
	pll_freq = get_sdio_freq_hz();
	if (pll_freq) {
		putstr("SDIO freq: ");
		putdec(pll_freq);
		putstr("\r\n");
	}
	pll_freq = get_sdxc_freq_hz();
	if (pll_freq) {
		putstr("SDXC freq: ");
		putdec(pll_freq);
		putstr("\r\n");
	}
}

static void rct_show_nand_boot(void)
{
	u32 nand_poc;

	nand_poc = rct_get_nand_poc();
	if (nand_poc & RCT_BOOT_NAND_PAGE_SIZE) {
		putstr(" 2048");
	} else {
		putstr(" 512");
	}
	if (nand_poc & RCT_BOOT_NAND_READ_CONFIRM) {
		putstr(" RC");
	}
	if (nand_poc & RCT_BOOT_NAND_ECC_BCH_EN) {
		putstr(" BCH");
		if (nand_poc & RCT_BOOT_NAND_ECC_SPARE_2X) {
			putstr(" 8bit");
		} else {
			putstr(" 6bit");
		}
	}
}

static void rct_show_emmc_boot(void)
{
	u32 emmc_poc;

	emmc_poc = rct_get_emmc_poc();
	if (emmc_poc & RCT_BOOT_EMMC_8BIT) {
		putstr(" 8Bit");
	} else if (emmc_poc & RCT_BOOT_EMMC_4BIT) {
		putstr(" 4Bit");
	} else {
		putstr(" 1Bit");
	}
	if (emmc_poc & RCT_BOOT_EMMC_DDR) {
		putstr(" DDR");
	} else if (emmc_poc & RCT_BOOT_EMMC_HS) {
		putstr(" HS");
	} else {
		putstr(" SDR");
	}
}

void rct_show_boot_from(u32 boot_from)
{
	putstr("Boot From: ");

	switch (boot_from) {
	case RCT_BOOT_FROM_NAND:
		putstr("NAND");
		rct_show_nand_boot();
		break;
	case RCT_BOOT_FROM_EMMC:
		putstr("EMMC");
		rct_show_emmc_boot();
		break;
	case RCT_BOOT_FROM_HIF:
		putstr("HIF");
		break;
	case RCT_BOOT_FROM_BYPASS:
		putstr("BYPASS");
		break;
	case RCT_BOOT_FROM_USB:
		putstr("USB");
		break;
	case RCT_BOOT_FROM_SPINOR:
		#if defined(CONFIG_BOOT_MEDIA_SPINAND)
		putstr("SPI NAND");
		#else
		putstr("SPI NOR");
		#endif
		break;
	default:
		putstr("Unknown");
		break;
	}
	putstr(" \r\n");
}

void dma_channel_select(void)
{
#if (DMA_SUPPORT_SELECT_CHANNEL == 1)
	u32 val, i;

	val = 0;
	for (i = 0; i < NUM_DMA_CHANNELS; i++) {
		switch(i) {
		case NOR_SPI_TX_DMA_CHAN:
			val |= NOR_SPI_TX_DMA_REQ_IDX << (i * 4);
			break;
		case NOR_SPI_RX_DMA_CHAN:
			val |= NOR_SPI_RX_DMA_REQ_IDX << (i * 4);
			break;
		case SSI1_TX_DMA_CHAN:
			val |= SSI1_TX_DMA_REQ_IDX << (i * 4);
			break;
		case SSI1_RX_DMA_CHAN:
			val |= SSI1_RX_DMA_REQ_IDX << (i * 4);
			break;
		case UART_TX_DMA_CHAN:
			val |= UART_TX_DMA_REQ_IDX << (i * 4);
			break;
		case UART_RX_DMA_CHAN:
			val |= UART_RX_DMA_REQ_IDX << (i * 4);
			break;
		case I2S_RX_DMA_CHAN:
			val |= I2S_RX_DMA_REQ_IDX << (i * 4);
			break;
		case I2S_TX_DMA_CHAN:
			val |= I2S_TX_DMA_REQ_IDX << (i * 4);
			break;
		}
	}
	writel(AHBSP_DMA_CHANNEL_SEL_REG, val);
#endif
}

