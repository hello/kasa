
#include <osl.h>

#ifdef CUSTOMER_HW

#ifdef CONFIG_PLAT_AMBARELLA
#include <config.h>
#include <linux/gpio.h>
#include <plat/sd.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
#include <linux/of_gpio.h>
#include <plat/service.h>
#else
#include <mach/board.h>
#endif
//#define GPIO_BCM_WL_REG_ON 102
//#define GPIO_BCM_WL_HOST_WAKE 4
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
int amba_gpio_set(int gpio_id, int value)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	/* Kernel 3.10 */
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = gpio_id;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
	if (errCode < 0) {
		printk("can't request gpio[%d]!\n", gpio_id);
		return errCode;
	}

	gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
	gpio_svc.value = value;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, NULL);
	if (errCode < 0)
		return errCode;

	return errCode;
#else
	/* Kernel 3.8 */
	gpio_set_value(GPIO(gpio_id), value);
	return 0;
#endif
}

int amba_gpio_set_input(int gpio_id)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	/* Kernel 3.10 */
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = gpio_id;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
	if (errCode < 0) {
		printk("can't request gpio[%d]!\n", gpio_id);
		return errCode;
	}

	gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, NULL);
	if (errCode < 0)
		return errCode;
	return errCode;
#else
	/* Kernel 3.8 */
	ambarella_gpio_config(GPIO(gpio_id), GPIO_FUNC_SW_INPUT);
	return 0;
#endif
}

int amba_gpio_irq(int gpio_id, int *irq_num)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	/* Kernel 3.10 */
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = gpio_id;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
	if (errCode < 0) {
		printk("can't request gpio[%d]!\n", gpio_id);
		return errCode;
	}

	gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, NULL);
	if (errCode < 0)
		return errCode;

	gpio_svc.svc_id = AMBSVC_GPIO_TO_IRQ;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, irq_num);
	if (errCode < 0) {
		printk("can't get irq from gpio[%d]!\n", gpio_id);
		return errCode;
	}
	return errCode;
#else
	/* Kernel 3.8 */
	*irq_num = gpio_to_irq(GPIO(gpio_id));
	ambarella_gpio_config(GPIO(gpio_id), GPIO_FUNC_SW_INPUT);
	return 0;
#endif
}

int free_gpio(int gpio_id)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_FREE;
	gpio_svc.gpio = gpio_id;
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);

	return errCode;
#else
	return 0;
#endif
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
	printk("GPIO(WL_HOST_WAKE) = %d\n", GPIO_BCM_WL_HOST_WAKE);
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
		printk("======== PULL WL_REG_ON HIGH! WL_REG_ON=%d. ========\n",
			GPIO_BCM_WL_REG_ON);
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 1);
		mdelay(100);
		free_gpio(GPIO_BCM_WL_REG_ON);
		printk("======== Enable sdhci presence change! ========\n");

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		ambarella_detect_sd_slot(0, 1);
#else
		ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
			ambarella_board_generic.wifi_sd_slot, 1);
#endif

	} else {
		printk("======== PULL WL_REG_ON HIGH! (flag = %d) ========\n", flag);
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 1);
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
		printk("======== Disable sdhci presence change! ========\n");
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		ambarella_detect_sd_slot(0, 0);
#else
		ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
			ambarella_board_generic.wifi_sd_slot, 0);
#endif
		mdelay(100);
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 0);
		mdelay(50);
		free_gpio(GPIO_BCM_WL_REG_ON);

		#ifdef CUSTOMER_OOB
		free_gpio(GPIO_BCM_WL_HOST_WAKE);
		#endif

		printk("======== PULL WL_REG_ON LOW! ========\n");
	} else {
		printk("======== PULL WL_REG_ON LOW! (flag = %d) ========\n", flag);
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 0);
	}
#endif
}

#endif /* CUSTOMER_HW */
