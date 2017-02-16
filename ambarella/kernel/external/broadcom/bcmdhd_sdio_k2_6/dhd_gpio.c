/*
* Customer code to add GPIO control during WLAN start/stop
* Copyright (C) 1999-2011, Broadcom Corporation
* 
*         Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2 (the "GPL"),
* available at http://www.broadcom.com/licenses/GPLv2.php, with the
* following added to such license:
* 
*      As a special exception, the copyright holders of this software give you
* permission to link this software with independent modules, and to copy and
* distribute the resulting executable under terms of your choice, provided that
* you also meet, for each linked independent module, the terms and conditions of
* the license of that module.  An independent module is a module which is not
* derived from this software.  The special exception does not apply to any
* modifications of the software.
* 
*      Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*
* $Id: dhd_custom_gpio.c,v 1.2.42.1 2010-10-19 00:41:09 Exp $
*/

#include <osl.h>

#include <mach/gpio.h>
//#include <mach/regs-gpio.h>
//#include <plat/gpio-cfg.h>
#include <mach/board.h>
#include <plat/sd.h>

#if CUSTOMER_HW

//#include <plat/sdhci.h>
//#include <plat/devs.h>	// modifed plat-samsung/dev-hsmmcX.c EXPORT_SYMBOL(s3c_device_hsmmcx) added

//#define	sdmmc_channel	s3c_device_hsmmc0
#define DEFAULT_GPIO_IRQ  22

#ifdef CUSTOM_BCMDHD_GPIO_IRQ
#define GPIO_IRQ CUSTOM_BCMDHD_GPIO_IRQ
#else
#define GPIO_IRQ DEFAULT_GPIO_IRQ
#endif

#ifdef CUSTOMER_OOB
int bcm_wlan_get_oob_irq(void)
{
	int host_oob_irq = 0;

	printk("GPIO(WL_HOST_WAKE) = GPIO_IRQ = %d\n", GPIO(GPIO_IRQ));
	host_oob_irq = gpio_to_irq(GPIO(GPIO_IRQ));
    ambarella_gpio_config(GPIO(GPIO_IRQ), GPIO_FUNC_SW_INPUT);
	//gpio_direction_input(GPIO(22));
	printk("host_oob_irq: %d \r\n", host_oob_irq);

	return host_oob_irq;
}
#endif

void bcm_wlan_power_on(int flag)
{
	if (flag == 1) {
		printk("======== PULL WL_REG_ON HIGH! ========\n");
    	ambarella_gpio_set(GPIO(GPIO_IRQ), GPIO_HIGH);
		mdelay(100);
		printk("======== Enable sdhci presence change! ========\n");
		ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
			ambarella_board_generic.wifi_sd_slot, 1);
		//sdhci_s3c_force_presence_change(&sdmmc_channel, 1);
	}
	else {
		printk("======== PULL WL_REG_ON HIGH! (flag = %d) ========\n", flag);
		ambarella_gpio_set(GPIO(GPIO_IRQ), GPIO_HIGH);
	}
}

void bcm_wlan_power_off(int flag)
{
	if (flag == 1) {
		printk("======== Disable sdhci presence change! ========\n");
		ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
			ambarella_board_generic.wifi_sd_slot, 0);
		//sdhci_s3c_force_presence_change(&sdmmc_channel, 0);
		mdelay(100);
		printk("======== PULL WL_REG_ON LOW! ========\n");
    	ambarella_gpio_set(GPIO(GPIO_IRQ), GPIO_LOW);
	}
	else {
		printk("======== PULL WL_REG_ON LOW! (flag = %d) ========\n", flag);
    	ambarella_gpio_set(GPIO(GPIO_IRQ), GPIO_LOW);
	}
}

#endif /* CUSTOMER_HW */
