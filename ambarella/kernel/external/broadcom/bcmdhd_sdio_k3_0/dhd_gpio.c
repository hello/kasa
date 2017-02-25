
#include <osl.h>

#ifdef CUSTOMER_HW

#ifdef CONFIG_PLAT_AMBARELLA
#define WIFI_DRIVER_NAME "bcmdhd"

#include <config.h>
#include <linux/gpio.h>
#include <plat/sd.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
#include <mach/board.h>
#endif
#endif

#ifdef CONFIG_MACH_ODROID_4210
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/devs.h>	// modifed plat-samsung/dev-hsmmcX.c EXPORT_SYMBOL(s3c_device_hsmmcx) added

#define	sdmmc_channel	s3c_device_hsmmc0
#endif

#ifdef CONFIG_PLAT_AMBARELLA
int amba_gpio_request(int gpio_id)
{
	int ret = -1;
	ret = gpio_request(GPIO(gpio_id), WIFI_DRIVER_NAME);
	if (ret < 0) {
		printk("gpio request (%d) failed.\n", gpio_id);
		return ret;
	}
	return ret;
}

int amba_gpio_output(int gpio_id, int value)
{
	return gpio_direction_output(GPIO(gpio_id), value);;
}

int amba_gpio_irq(int gpio_id, int *irq_num)
{
	*irq_num = gpio_to_irq(GPIO(gpio_id));
	return gpio_direction_input(GPIO(gpio_id));
}

int amba_gpio_free(int gpio_id)
{
	gpio_free(GPIO(gpio_id));
	return 0;
}
#endif

#ifdef CUSTOMER_OOB
int bcm_wlan_get_oob_irq(void)
{
	int host_oob_irq = 0;

#if 0
	printk("GPIO(WL_HOST_WAKE) = EXYNOS4_GPX0(7) = %d\n", EXYNOS4_GPX0(7));
	host_oob_irq = gpio_to_irq(EXYNOS4_GPX0(7));
	gpio_direction_input(EXYNOS4_GPX0(7));
#endif

#ifdef CONFIG_PLAT_AMBARELLA
	printk("GPIO(WL_HOST_WAKE) [%d]\n", GPIO_BCM_WL_HOST_WAKE);
	amba_gpio_request(GPIO_BCM_WL_HOST_WAKE);
	amba_gpio_irq(GPIO_BCM_WL_HOST_WAKE, &host_oob_irq);
#endif

	printk("host_oob_irq: %d \r\n", host_oob_irq);
	return host_oob_irq;
}
#endif

void bcm_wlan_power_on(int flag)
{
#if 0
	if (flag == 1) {
		printk("======== PULL WL_REG_ON HIGH! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		gpio_set_value(EXYNOS4_GPK1(0), 1);
		/* Lets customer power to get stable */
		mdelay(100);
		printk("======== Card detection to detect SDIO card! ========\n");
		sdhci_s3c_force_presence_change(&sdmmc_channel, 1);
#endif
	} else {
		printk("======== PULL WL_REG_ON HIGH! (flag = %d) ========\n", flag);
#ifdef CONFIG_MACH_ODROID_4210
		gpio_set_value(EXYNOS4_GPK1(0), 1);
#endif
#endif

#ifdef CONFIG_PLAT_AMBARELLA
	if (flag == 1) {
		printk("======== PULL WL_REG_ON[%d], active[%d], HIGH! ========\n",
			GPIO_BCM_WL_REG_ON, GPIO_BCM_WL_REG_ON_ACTIVE);
		amba_gpio_request(GPIO_BCM_WL_REG_ON);
		amba_gpio_output(GPIO_BCM_WL_REG_ON, GPIO_BCM_WL_REG_ON_ACTIVE);
		mdelay(100);
		printk("======== Enable sdhci presence change! Slot.num[%d] ========\n",
			WIFI_CONN_SD_SLOT_NUM);

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		if (WIFI_CONN_SD_SLOT_NUM >= 0 && WIFI_CONN_SD_SLOT_NUM < 5) {
			ambarella_detect_sd_slot(WIFI_CONN_SD_SLOT_NUM, 1);
		}
#else
		if (ambarella_board_generic.wifi_sd_bus >= 0 && ambarella_board_generic.wifi_sd_bus < 5) {
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 1);
		}
#endif

	} else {
		printk("======== PULL WL_REG_ON[%d], active[%d], HIGH! (flag = %d) ========\n",
			GPIO_BCM_WL_REG_ON, GPIO_BCM_WL_REG_ON_ACTIVE, flag);
		amba_gpio_request(GPIO_BCM_WL_REG_ON);
		amba_gpio_output(GPIO_BCM_WL_REG_ON, GPIO_BCM_WL_REG_ON_ACTIVE);
	}
#endif
}

void bcm_wlan_power_off(int flag)
{
#if 0
	if (flag == 1) {
		printk("======== Card detection to remove SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		sdhci_s3c_force_presence_change(&sdmmc_channel, 0);
		mdelay(100);
		printk("======== PULL WL_REG_ON LOW! ========\n");
		gpio_set_value(EXYNOS4_GPK1(0), 0);
#endif
	} else {
		printk("======== PULL WL_REG_ON LOW! (flag = %d) ========\n", flag);
#ifdef CONFIG_MACH_ODROID_4210
		gpio_set_value(EXYNOS4_GPK1(0), 0);
#endif
#endif

#ifdef CONFIG_PLAT_AMBARELLA
	if (flag == 1) {
		printk("======== Disable sdhci presence change! Slot.num[%d] ========\n",
			WIFI_CONN_SD_SLOT_NUM);
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		if (WIFI_CONN_SD_SLOT_NUM >= 0 && WIFI_CONN_SD_SLOT_NUM < 5) {
			ambarella_detect_sd_slot(WIFI_CONN_SD_SLOT_NUM, 0);
		}
#else
		if (ambarella_board_generic.wifi_sd_bus >= 0 && ambarella_board_generic.wifi_sd_bus < 5) {
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 0);
		}
#endif
		mdelay(100);
		amba_gpio_output(GPIO_BCM_WL_REG_ON, !GPIO_BCM_WL_REG_ON_ACTIVE);
		mdelay(50);
		amba_gpio_free(GPIO_BCM_WL_REG_ON);
#ifdef CUSTOMER_OOB
		amba_gpio_free(GPIO_BCM_WL_HOST_WAKE);
#endif
		printk("======== PULL WL_REG_ON[%d], inactive[%d], LOW! ========\n",
			GPIO_BCM_WL_REG_ON, !GPIO_BCM_WL_REG_ON_ACTIVE);
	} else {
		printk("======== PULL WL_REG_ON[%d], inactive[%d], LOW! (flag = %d) ========\n",
			GPIO_BCM_WL_REG_ON, !GPIO_BCM_WL_REG_ON_ACTIVE, flag);
		amba_gpio_output(GPIO_BCM_WL_REG_ON, !GPIO_BCM_WL_REG_ON_ACTIVE);
		amba_gpio_free(GPIO_BCM_WL_REG_ON);
	}
#endif
}

#endif /* CUSTOMER_HW */
