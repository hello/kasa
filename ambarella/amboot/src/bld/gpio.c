/**
 * bld/gpio.c
 *
 * Vector interrupt controller related utilities.
 *
 * History:
 *    2015/11/30 - [Cao Rongrong] created file
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

#include <amboot.h>
#include <bldfunc.h>
#include <ambhw/gpio.h>
#include <ambhw/uart.h>
#include <ambhw/nand.h>
#include <ambhw/sd.h>
#include <ambhw/spinor.h>

#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
#ifndef DEFAULT_GPIO0_CTRL_ENA
#define DEFAULT_GPIO0_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO0_CTRL_DIR
#define DEFAULT_GPIO0_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_ENA
#define DEFAULT_GPIO1_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_DIR
#define DEFAULT_GPIO1_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_ENA
#define DEFAULT_GPIO2_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_DIR
#define DEFAULT_GPIO2_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_ENA
#define DEFAULT_GPIO3_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_DIR
#define DEFAULT_GPIO3_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_ENA
#define DEFAULT_GPIO4_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_DIR
#define DEFAULT_GPIO4_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_ENA
#define DEFAULT_GPIO5_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_DIR
#define DEFAULT_GPIO5_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO6_CTRL_DIR
#define DEFAULT_GPIO6_CTRL_DIR	0x00000000
#endif
#endif

#if (GPIO_PAD_DS_SUPPORT == 1)
#ifndef DEFAULT_GPIO_DS0_REG_0
#define DEFAULT_GPIO_DS0_REG_0	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_0
#define DEFAULT_GPIO_DS1_REG_0	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_1
#define DEFAULT_GPIO_DS0_REG_1	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_1
#define DEFAULT_GPIO_DS1_REG_1	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_2
#define DEFAULT_GPIO_DS0_REG_2	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_2
#define DEFAULT_GPIO_DS1_REG_2	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_3
#define DEFAULT_GPIO_DS0_REG_3	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_3
#define DEFAULT_GPIO_DS1_REG_3	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_4
#define DEFAULT_GPIO_DS0_REG_4	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_4
#define DEFAULT_GPIO_DS1_REG_4	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_5
#define DEFAULT_GPIO_DS0_REG_5	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_5
#define DEFAULT_GPIO_DS1_REG_5	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_6
#define DEFAULT_GPIO_DS1_REG_6	0xFFFFFFFF
#endif
#endif

static inline u32 gpio_bank_to_base(u32 bank)
{
	u32 regbase;

	switch (bank) {
	case 0:
		regbase = GPIO0_BASE;
		break;
	case 1:
		regbase = GPIO1_BASE;
		break;
	case 2:
		regbase = GPIO2_BASE;
		break;
	case 3:
		regbase = GPIO3_BASE;
		break;
	case 4:
		regbase = GPIO4_BASE;
		break;
	case 5:
		regbase = GPIO5_BASE;
		break;
	case 6:
		regbase = GPIO6_BASE;
		break;
	default:
		return -1;
	}

	return regbase;
}

static void gpio_cfg_mode(int gpio, int mode)
{
	u32 regbase, bank, offset;

	bank = PINID_TO_BANK(gpio);
	offset = PINID_TO_OFFSET(gpio);
	regbase = gpio_bank_to_base(bank);

	if (mode == GPIO_FUNC_HW) {
		setbitsl(regbase + GPIO_AFSEL_OFFSET, 0x1 << offset);
		clrbitsl(regbase + GPIO_MASK_OFFSET, 0x1 << offset);
	} else {
		if (mode == GPIO_FUNC_SW_INPUT)
			clrbitsl(regbase + GPIO_DIR_OFFSET, 0x1 << offset);
		else
			setbitsl(regbase + GPIO_DIR_OFFSET, 0x1 << offset);
		clrbitsl(regbase + GPIO_AFSEL_OFFSET, 0x1 << offset);
		setbitsl(regbase + GPIO_MASK_OFFSET, 0x1 << offset);
	}
}

static void gpio_iomux_set_altfunc(int gpio, int alt_func)
{
#if (IOMUX_SUPPORT > 0)
	u32 bank, offset, data, i;

	bank = PINID_TO_BANK(gpio);
	offset = PINID_TO_OFFSET(gpio);

	for (i = 0; i < 3; i++) {
		data = readl(IOMUX_REG(IOMUX_REG_OFFSET(bank, i)));
		data &= (~(0x1 << offset));
		data |= (((alt_func >> i) & 0x1) << offset);
		writel(IOMUX_REG(IOMUX_REG_OFFSET(bank, i)), data);
	}
#endif
}

static void gpio_iomux_enable_altfunc(void)
{
#if (IOMUX_SUPPORT > 0)
	writel(IOMUX_REG(IOMUX_CTRL_SET_OFFSET), 0x1);
	writel(IOMUX_REG(IOMUX_CTRL_SET_OFFSET), 0x0);
#endif
}

/* ==========================================================================*/

#ifdef CONFIG_AMBOOT_ENABLE_GPIO

int gpio_config_hw(int gpio, int alt_func)
{
	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_config_hw: Invalid gpio: %d\n", gpio);
		return -1;
	}

	gpio_cfg_mode(gpio, GPIO_FUNC_HW);
	gpio_iomux_set_altfunc(gpio, alt_func);
	gpio_iomux_enable_altfunc();

	return 0;
}

int gpio_config_sw_in(int gpio)
{
	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_config_sw_in: Invalid gpio: %d\n", gpio);
		return -1;
	}

	gpio_cfg_mode(gpio, GPIO_FUNC_SW_INPUT);
	gpio_iomux_set_altfunc(gpio, 0);
	gpio_iomux_enable_altfunc();

	return 0;
}

int gpio_config_sw_out(int gpio)
{
	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_config_sw_out: Invalid gpio: %d\n", gpio);
		return -1;
	}

	gpio_cfg_mode(gpio, GPIO_FUNC_SW_OUTPUT);
	gpio_iomux_set_altfunc(gpio, 0);
	gpio_iomux_enable_altfunc();

	return 0;
}

int gpio_set(int gpio)
{
	u32 regbase, bank, offset;

	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_set: Invalid gpio: %d\n", gpio);
		return -1;
	}

	bank = PINID_TO_BANK(gpio);
	offset = PINID_TO_OFFSET(gpio);
	regbase = gpio_bank_to_base(bank);

	setbitsl(regbase + GPIO_DATA_OFFSET, 0x1 << offset);

	return 0;
}

int gpio_clr(int gpio)
{
	u32 regbase, bank, offset;

	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_clr: Invalid gpio: %d\n", gpio);
		return -1;
	}

	bank = PINID_TO_BANK(gpio);
	offset = PINID_TO_OFFSET(gpio);
	regbase = gpio_bank_to_base(bank);

	clrbitsl(regbase + GPIO_DATA_OFFSET, 0x1 << offset);

	return 0;
}

int gpio_get(int gpio)
{
	u32 regbase, bank, offset, data;

	if (gpio >= AMBGPIO_SIZE) {
		printf("gpio_get: Invalid gpio: %d\n", gpio);
		return -1;
	}

	bank = PINID_TO_BANK(gpio);
	offset = PINID_TO_OFFSET(gpio);
	regbase = gpio_bank_to_base(bank);

	data = readl(regbase + GPIO_DATA_OFFSET);

	clrbitsl(regbase + GPIO_DATA_OFFSET, 0x1 << offset);

	return !!(data & (0x1 << offset));
}

void gpio_init(void)
{
	writel(GPIO0_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO0_AFSEL);
	writel(GPIO0_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO0_DIR);
	writel(GPIO0_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO0_MASK);
	writel(GPIO0_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO0_DATA);
	writel(GPIO0_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#if (GPIO_INSTANCES >= 2)
	writel(GPIO1_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO1_AFSEL);
	writel(GPIO1_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO1_DIR);
	writel(GPIO1_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO1_MASK);
	writel(GPIO1_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO1_DATA);
	writel(GPIO1_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif
#if (GPIO_INSTANCES >= 3)
	writel(GPIO2_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO2_AFSEL);
	writel(GPIO2_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO2_DIR);
	writel(GPIO2_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO2_MASK);
	writel(GPIO2_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO2_DATA);
	writel(GPIO2_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif
#if (GPIO_INSTANCES >= 4)
	writel(GPIO3_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO3_AFSEL);
	writel(GPIO3_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO3_DIR);
	writel(GPIO3_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO3_MASK);
	writel(GPIO3_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO3_DATA);
	writel(GPIO3_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif
#if (GPIO_INSTANCES >= 5)
	writel(GPIO4_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO4_AFSEL);
	writel(GPIO4_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO4_DIR);
	writel(GPIO4_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO4_MASK);
	writel(GPIO4_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO4_DATA);
	writel(GPIO4_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif
#if (GPIO_INSTANCES >= 6)
	writel(GPIO5_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO5_AFSEL);
	writel(GPIO5_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO5_DIR);
	writel(GPIO5_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO5_MASK);
	writel(GPIO5_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO5_DATA);
	writel(GPIO5_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif
#if (GPIO_INSTANCES >= 7)
	writel(GPIO6_REG(GPIO_AFSEL_OFFSET), DEFAULT_GPIO6_AFSEL);
	writel(GPIO6_REG(GPIO_DIR_OFFSET), DEFAULT_GPIO6_DIR);
	writel(GPIO6_REG(GPIO_MASK_OFFSET), DEFAULT_GPIO6_MASK);
	writel(GPIO6_REG(GPIO_DATA_OFFSET), DEFAULT_GPIO6_DATA);
	writel(GPIO6_REG(GPIO_ENABLE_OFFSET), 0xffffffff);
#endif

	/* initialize IOMUX */
#if (IOMUX_SUPPORT == 1)
	writel(IOMUX_REG(IOMUX_REG0_0_OFFSET), DEFAULT_IOMUX_REG0_0);
	writel(IOMUX_REG(IOMUX_REG0_1_OFFSET), DEFAULT_IOMUX_REG0_1);
	writel(IOMUX_REG(IOMUX_REG0_2_OFFSET), DEFAULT_IOMUX_REG0_2);
#if (GPIO_INSTANCES >= 2)
	writel(IOMUX_REG(IOMUX_REG1_0_OFFSET), DEFAULT_IOMUX_REG1_0);
	writel(IOMUX_REG(IOMUX_REG1_1_OFFSET), DEFAULT_IOMUX_REG1_1);
	writel(IOMUX_REG(IOMUX_REG1_2_OFFSET), DEFAULT_IOMUX_REG1_2);
#endif
#if (GPIO_INSTANCES >= 3)
	writel(IOMUX_REG(IOMUX_REG2_0_OFFSET), DEFAULT_IOMUX_REG2_0);
	writel(IOMUX_REG(IOMUX_REG2_1_OFFSET), DEFAULT_IOMUX_REG2_1);
	writel(IOMUX_REG(IOMUX_REG2_2_OFFSET), DEFAULT_IOMUX_REG2_2);
#endif
#if (GPIO_INSTANCES >= 4)
	writel(IOMUX_REG(IOMUX_REG3_0_OFFSET), DEFAULT_IOMUX_REG3_0);
	writel(IOMUX_REG(IOMUX_REG3_1_OFFSET), DEFAULT_IOMUX_REG3_1);
	writel(IOMUX_REG(IOMUX_REG3_2_OFFSET), DEFAULT_IOMUX_REG3_2);
#endif
#if (GPIO_INSTANCES >= 5)
	writel(IOMUX_REG(IOMUX_REG4_0_OFFSET), DEFAULT_IOMUX_REG4_0);
	writel(IOMUX_REG(IOMUX_REG4_1_OFFSET), DEFAULT_IOMUX_REG4_1);
	writel(IOMUX_REG(IOMUX_REG4_2_OFFSET), DEFAULT_IOMUX_REG4_2);
#endif
#if (GPIO_INSTANCES >= 6)
	writel(IOMUX_REG(IOMUX_REG5_0_OFFSET), DEFAULT_IOMUX_REG5_0);
	writel(IOMUX_REG(IOMUX_REG5_1_OFFSET), DEFAULT_IOMUX_REG5_1);
	writel(IOMUX_REG(IOMUX_REG5_2_OFFSET), DEFAULT_IOMUX_REG5_2);
#endif
#if (GPIO_INSTANCES >= 7)
	writel(IOMUX_REG(IOMUX_REG6_0_OFFSET), DEFAULT_IOMUX_REG6_0);
	writel(IOMUX_REG(IOMUX_REG6_1_OFFSET), DEFAULT_IOMUX_REG6_1);
	writel(IOMUX_REG(IOMUX_REG6_2_OFFSET), DEFAULT_IOMUX_REG6_2);
#endif

	writel(IOMUX_REG(IOMUX_CTRL_SET_OFFSET), 0x1);
	writel(IOMUX_REG(IOMUX_CTRL_SET_OFFSET), 0x0);
#endif

	/* initialize pull up/down */
#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_0_OFFSET), DEFAULT_GPIO0_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_0_OFFSET), DEFAULT_GPIO0_CTRL_DIR);
#if (GPIO_INSTANCES >= 2)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_1_OFFSET), DEFAULT_GPIO1_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_1_OFFSET), DEFAULT_GPIO1_CTRL_DIR);
#endif
#if (GPIO_INSTANCES >= 3)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_2_OFFSET), DEFAULT_GPIO2_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_2_OFFSET), DEFAULT_GPIO2_CTRL_DIR);
#endif
#if (GPIO_INSTANCES >= 4)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_3_OFFSET), DEFAULT_GPIO3_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_3_OFFSET), DEFAULT_GPIO3_CTRL_DIR);
#endif
#if (GPIO_INSTANCES >= 5)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_4_OFFSET), DEFAULT_GPIO4_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_4_OFFSET), DEFAULT_GPIO4_CTRL_DIR);
#endif
#if (GPIO_INSTANCES >= 6)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_5_OFFSET), DEFAULT_GPIO5_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_5_OFFSET), DEFAULT_GPIO5_CTRL_DIR);
#endif
#if (GPIO_INSTANCES >= 7)
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_6_OFFSET), DEFAULT_GPIO6_CTRL_ENA);
	writel(GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_6_OFFSET), DEFAULT_GPIO6_CTRL_DIR);
#endif
#endif

	/* initialize drive strength */
#if (GPIO_PAD_DS_SUPPORT == 1)
	writel(RCT_REG(GPIO_DS0_0_OFFSET), DEFAULT_GPIO_DS0_REG_0);
	writel(RCT_REG(GPIO_DS1_0_OFFSET), DEFAULT_GPIO_DS1_REG_0);
#if (GPIO_INSTANCES >= 2)
	writel(RCT_REG(GPIO_DS0_1_OFFSET), DEFAULT_GPIO_DS0_REG_1);
	writel(RCT_REG(GPIO_DS1_1_OFFSET), DEFAULT_GPIO_DS1_REG_1);
#endif
#if (GPIO_INSTANCES >= 3)
	writel(RCT_REG(GPIO_DS0_2_OFFSET), DEFAULT_GPIO_DS0_REG_2);
	writel(RCT_REG(GPIO_DS1_2_OFFSET), DEFAULT_GPIO_DS1_REG_2);
#endif
#if (GPIO_INSTANCES >= 4)
	writel(RCT_REG(GPIO_DS0_3_OFFSET), DEFAULT_GPIO_DS0_REG_3);
	writel(RCT_REG(GPIO_DS1_3_OFFSET), DEFAULT_GPIO_DS1_REG_3);
#endif
#if (GPIO_INSTANCES >= 5)
	writel(RCT_REG(GPIO_DS0_4_OFFSET), DEFAULT_GPIO_DS0_REG_4);
	writel(RCT_REG(GPIO_DS1_4_OFFSET), DEFAULT_GPIO_DS1_REG_4);
#endif
#if (GPIO_INSTANCES >= 6)
	writel(RCT_REG(GPIO_DS0_5_OFFSET), DEFAULT_GPIO_DS0_REG_5);
	writel(RCT_REG(GPIO_DS1_5_OFFSET), DEFAULT_GPIO_DS1_REG_5);
#endif
#if (GPIO_INSTANCES >= 7)
	writel(RCT_REG(GPIO_DS0_6_OFFSET), DEFAULT_GPIO_DS0_REG_6);
	writel(RCT_REG(GPIO_DS1_6_OFFSET), DEFAULT_GPIO_DS1_REG_6);
#endif
#endif

	/* waiting for pin stable
	 * ps: uart may work abnormal at very beginnin without delay */
	rct_timer2_dly_ms(1);
}

#else

/* Initiate minimal pins of UART, NAND, eMMC and SPINOR for USB boot mode */
void gpio_mini_init(void)
{
	u32 i, boot_from = ambausb_boot_from[0];
	u32 regbase, gpio_enable = 0;
	u32 minipin_uart[] = MINIPIN_UART_PIN;
	u32 minipin_nand[] = MINIPIN_NAND_PIN;
	u32 minipin_emmc[] = MINIPIN_EMMC_PIN;
	u32 minipin_spinor[] = MINIPIN_SPINOR_PIN;

	for (i = 0; i < ARRAY_SIZE(minipin_uart); i++) {
		gpio_enable |= 1 << PINID_TO_BANK(minipin_uart[i]);
		gpio_cfg_mode(minipin_uart[i], GPIO_FUNC_HW);
		gpio_iomux_set_altfunc(minipin_uart[i], MINIPIN_UART_ALTFUNC);
	}

	for (i = 0; i < ARRAY_SIZE(minipin_nand); i++) {
		if (boot_from != RCT_BOOT_FROM_NAND) {
			break;
		}
		gpio_enable |= 1 << PINID_TO_BANK(minipin_nand[i]);
		gpio_cfg_mode(minipin_nand[i], GPIO_FUNC_HW);
		gpio_iomux_set_altfunc(minipin_nand[i], MINIPIN_NAND_ALTFUNC);
	}

	for (i = 0; i < ARRAY_SIZE(minipin_emmc); i++) {
		if (boot_from != RCT_BOOT_FROM_EMMC)
			break;
		gpio_enable |= 1 << PINID_TO_BANK(minipin_emmc[i]);
		gpio_cfg_mode(minipin_emmc[i], GPIO_FUNC_HW);
		gpio_iomux_set_altfunc(minipin_emmc[i], MINIPIN_EMMC_ALTFUNC);
	}

	for (i = 0; i < ARRAY_SIZE(minipin_spinor); i++) {
		if (boot_from != RCT_BOOT_FROM_SPINOR)
			break;
		gpio_enable |= 1 << PINID_TO_BANK(minipin_spinor[i]);
		gpio_cfg_mode(minipin_spinor[i], GPIO_FUNC_HW);
		gpio_iomux_set_altfunc(minipin_spinor[i], MINIPIN_SPINOR_ALTFUNC);
	}

	gpio_iomux_enable_altfunc();

	for (i = 0; i < GPIO_INSTANCES; i++) {
		if (gpio_enable & (1 << i)) {
			regbase = gpio_bank_to_base(i);
			writel(regbase + GPIO_ENABLE_OFFSET, 0xffffffff);
		}
	}

	/* waiting for pin stable
	 * ps: uart may work abnormal at very beginnin without delay */
	rct_timer2_dly_ms(1);
}
#endif
