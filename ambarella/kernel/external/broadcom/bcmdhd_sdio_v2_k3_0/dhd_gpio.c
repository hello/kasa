
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
	gpio_direction_input(GPIO(gpio_id));
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
	gpio_direction_input(GPIO(gpio_id));
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

#ifdef CUSTOMER_HW_ALLWINNER
#include <linux/gpio.h>
#include <mach/sys_config.h>
static int sdc_id = 0;
extern void sunxi_mci_rescan_card(unsigned id, unsigned insert);
extern void wifi_pm_power(int on);
#endif


struct wifi_platform_data {
	int (*set_power)(bool val);
	int (*set_carddetect)(bool val);
	void *(*mem_prealloc)(int section, unsigned long size);
	int (*get_mac_addr)(unsigned char *buf);
	void *(*get_country_code)(char *ccode);
};

struct resource dhd_wlan_resources = {0};
struct wifi_platform_data dhd_wlan_control = {0};


#ifdef CUSTOMER_OOB
uint bcm_wlan_get_oob_irq(void)
{
	uint host_oob_irq = 0;

#ifdef CONFIG_MACH_ODROID_4210
	printk("GPIO(WL_HOST_WAKE) = EXYNOS4_GPX0(7) = %d\n", EXYNOS4_GPX0(7));
	host_oob_irq = gpio_to_irq(EXYNOS4_GPX0(7));
	gpio_direction_input(EXYNOS4_GPX0(7));
#endif
#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
	int wl_host_wake = 0;

	type = script_get_item("wifi_para", "ap6xxx_wl_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("get bcmdhd wl_host_wake gpio failed\n");
		return 0;
	} else {
		wl_host_wake = val.gpio.gpio;
	}
	printk("GPIO(WL_HOST_WAKE) = %d\n", wl_host_wake);

	host_oob_irq = gpio_to_irq(wl_host_wake);
	if (IS_ERR_VALUE(host_oob_irq)) {
		printk("map gpio [%d] to virq failed, errno = %d\n",wl_host_wake, host_oob_irq);
		return 0;
	}
#endif

#ifdef CONFIG_PLAT_AMBARELLA
		printk("GPIO(WL_HOST_WAKE) = %d\n", GPIO_BCM_WL_HOST_WAKE);
		amba_gpio_irq(GPIO_BCM_WL_HOST_WAKE, &host_oob_irq);
#endif

	printk("host_oob_irq: %d \r\n", host_oob_irq);

	return host_oob_irq;
}

uint bcm_wlan_get_oob_irq_flags(void)
{
	uint host_oob_irq_flags = 0;

#ifdef CONFIG_MACH_ODROID_4210
	host_oob_irq_flags = (IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE) & IRQF_TRIGGER_MASK;
#endif
#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
	int host_wake_invert = 0;

	type = script_get_item("wifi_para", "wl_host_wake_invert", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
		printk("has no wl_host_wake_invert\n");
	else
		host_wake_invert = val.val;

	if(!host_wake_invert)
		host_oob_irq_flags = (IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE) & IRQF_TRIGGER_MASK;
	else
		host_oob_irq_flags = (IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE) & IRQF_TRIGGER_MASK;
#endif

#ifdef CONFIG_PLAT_AMBARELLA
		host_oob_irq_flags = (IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE) & IRQF_TRIGGER_MASK;
#endif

	printk("host_oob_irq_flags=%d\n", host_oob_irq_flags);

	return host_oob_irq_flags;
}
#endif

int bcm_wlan_set_power(bool on)
{
	int err = 0;

	if (on) {
		printk("======== PULL WL_REG_ON HIGH! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = gpio_set_value(EXYNOS4_GPK1(0), 1);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		wifi_pm_power(1);
#endif

#ifdef CONFIG_PLAT_AMBARELLA
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 1);
		free_gpio(GPIO_BCM_WL_REG_ON);
#endif
		/* Lets customer power to get stable */
		mdelay(100);
	} else {
		printk("======== PULL WL_REG_ON LOW! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = gpio_set_value(EXYNOS4_GPK1(0), 0);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		wifi_pm_power(0);
#endif

#ifdef CONFIG_PLAT_AMBARELLA
		amba_gpio_set(GPIO_BCM_WL_REG_ON, 0);
		free_gpio(GPIO_BCM_WL_REG_ON);

		#ifdef CUSTOMER_OOB
		free_gpio(GPIO_BCM_WL_HOST_WAKE);
		#endif
#endif
	}

	return err;
}

int bcm_wlan_set_carddetect(bool present)
{
	int err = 0;

	if (present) {
		printk("======== Card detection to detect SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = sdhci_s3c_force_presence_change(&sdmmc_channel, 1);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		sunxi_mci_rescan_card(sdc_id, 1);
#endif

#ifdef CONFIG_PLAT_AMBARELLA
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
			ambarella_detect_sd_slot(0, 1);
#else
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 1);
#endif
#endif
	} else {
		printk("======== Card detection to remove SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = sdhci_s3c_force_presence_change(&sdmmc_channel, 0);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		sunxi_mci_rescan_card(sdc_id, 0);
#endif

#ifdef CONFIG_PLAT_AMBARELLA
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
			ambarella_detect_sd_slot(0, 0);
#else
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 0);
#endif
#endif
	}

	return err;
}

#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *bcmdhd_mem_prealloc(int section, unsigned long size);
void* bcm_wlan_prealloc(int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	alloc_ptr = bcmdhd_mem_prealloc(section, size);
	if (alloc_ptr) {
		printk("success alloc section %d, size %ld\n", section, size);
		if (size != 0L)
			bzero(alloc_ptr, size);
		return alloc_ptr;
	}
	printk("can't alloc section %d\n", section);
	return NULL;
}
#endif

int bcm_wlan_set_plat_data(void) {
#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
#endif
	printk("======== %s ========\n", __FUNCTION__);
#ifdef CUSTOMER_HW_ALLWINNER
	type = script_get_item("wifi_para", "wifi_sdc_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(("failed to fetch sdio card's sdcid\n"));
		return -1;
	}
	sdc_id = val.val;
#endif
	dhd_wlan_control.set_power = bcm_wlan_set_power;
	dhd_wlan_control.set_carddetect = bcm_wlan_set_carddetect;
#ifdef CONFIG_DHD_USE_STATIC_BUF
	dhd_wlan_control.mem_prealloc = bcm_wlan_prealloc;
#endif
	return 0;
}

#endif /* CUSTOMER_HW */
