/*
 * Copyright (c) 2004-2012 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef CONFIG_ANDROID_8960_SDIO
#include "core.h"

/* BeginMMC polling stuff */
#ifdef CONFIG_MMC_MSM_SDC3_POLLING
#define MMC_MSM_DEV "msm_sdcc.3"
#else
#define MMC_MSM_DEV "msm_sdcc.4"
#endif
/* End MMC polling stuff */
#endif /* #ifdef CONFIG_ANDROID_8960_SDIO */

#include "core.h"
#include "debug.h"
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wlan_plat.h>
#include <mach/msm_bus.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>

#define GET_INODE_FROM_FILEP(filp) ((filp)->f_path.dentry->d_inode)

int android_readwrite_file(const char *filename, char *rbuf,
	const char *wbuf, size_t length)
{
	int ret = 0;
	struct file *filp = (struct file *)-ENOENT;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	do {
		int mode = (wbuf) ? O_WRONLY : O_RDONLY;
		filp = filp_open(filename, mode, S_IRUSR);

		if (IS_ERR(filp) || !filp->f_op) {
			ret = -ENOENT;
			break;
		}

		if (!filp->f_op->write || !filp->f_op->read) {
			filp_close(filp, NULL);
			ret = -ENOENT;
			break;
		}

		if (length == 0) {
			/* Read the length of the file only */
			struct inode    *inode;

			inode = GET_INODE_FROM_FILEP(filp);
			if (!inode) {
				printk(KERN_ERR
					"android_readwrite_file: Error 2\n");
				ret = -ENOENT;
				break;
			}
			ret = i_size_read(inode->i_mapping->host);
			break;
		}

		if (wbuf) {
			ret = filp->f_op->write(
				filp, wbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"android_readwrite_file: Error 3\n");
				break;
			}
		} else {
			ret = filp->f_op->read(
				filp, rbuf, length, &filp->f_pos);
			if (ret < 0) {
				printk(KERN_ERR
					"android_readwrite_file: Error 4\n");
				break;
			}
		}
	} while (0);

	if (!IS_ERR(filp))
		filp_close(filp, NULL);

	set_fs(oldfs);
	printk(KERN_ERR "android_readwrite_file: ret=%d\n", ret);

	return ret;
}

#ifdef CONFIG_ANDROID_8960_SDIO
static struct wifi_platform_data *wifi_control_data;
struct semaphore wifi_control_sem;

int wifi_set_power(int on, unsigned long msec)
{
	if (wifi_control_data && wifi_control_data->set_power)
		wifi_control_data->set_power(on);

	if (msec)
		mdelay(msec);
	return 0;
}

static int wifi_probe(struct platform_device *pdev)
{
	struct wifi_platform_data *wifi_ctrl =
			(struct wifi_platform_data *)(pdev->dev.platform_data);

	wifi_control_data = wifi_ctrl;

	wifi_set_power(1, 0);	/* Power On */

	up(&wifi_control_sem);
	return 0;
}

static int wifi_remove(struct platform_device *pdev)
{
	struct wifi_platform_data *wifi_ctrl =
		(struct wifi_platform_data *)(pdev->dev.platform_data);

	wifi_control_data = wifi_ctrl;

	wifi_set_power(0, 0);	/* Power Off */

	up(&wifi_control_sem);
	return 0;
}

static struct platform_driver wifi_device = {
	.probe	= wifi_probe,
	.remove	= wifi_remove,
	.driver	= {
		.name	= "ath6kl_power",
	}
};

void __init ath6kl_sdio_init_msm(void)
{
	char buf[3];
	int length;
	int ret;

	sema_init(&wifi_control_sem, 1);
	down(&wifi_control_sem);

	ret = platform_driver_probe(&wifi_device, wifi_probe);
	if (ret) {
		printk(KERN_INFO "platform_driver_register failed\n");
		return;
	}

	/* Waiting callback after platform_driver_register */
	if (down_timeout(&wifi_control_sem,  msecs_to_jiffies(5000)) != 0) {
		ret = -EINVAL;
		printk(KERN_INFO "platform_driver_register timeout\n");
		return;
	}

	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV
		"/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV
		"/polling", NULL, buf, length);

	mdelay(500);
}

void __exit ath6kl_sdio_exit_msm(void)
{
	char buf[3];
	int length;
	int ret;

	platform_driver_unregister(&wifi_device);

	/* Waiting callback after platform_driver_register */
	if (down_timeout(&wifi_control_sem,  msecs_to_jiffies(5000)) != 0) {
		ret = -EINVAL;
		printk(KERN_INFO "platform_driver_unregister timeout\n");
		return;
	}

	length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV
		"/polling", NULL, buf, length);
	length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
	/* fall back to polling */
	android_readwrite_file("/sys/devices/platform/" MMC_MSM_DEV
		"/polling", NULL, buf, length);
	mdelay(1000);

}
#else

#ifdef ATH6KL_BUS_VOTE
static u32 bus_perf_client;
static struct msm_bus_scale_pdata *ath6kl_bus_scale_pdata;

u8 *platform_has_vreg;
#endif

#define VDD_PA_MAX_VOLTAGE         3300000
#define VDD_PA_MIN_VOLTAGE         3000000
#define VDD_IO_MAX_VOLTAGE         1800000
#define VDD_IO_MIN_VOLTAGE         1800000

struct ath6kl_power_vreg_data {
	/* voltage regulator handle */
	struct regulator *reg;

	/* regulator name */
	const char *name;

	/* voltage levels to be set */
	unsigned int low_vol_level;
	unsigned int high_vol_level;

	/*
	 * is set voltage supported for this regulator?
	 * false => set voltage is not supported
	 * true  => set voltage is supported
	 *
	 * Some regulators (like gpio-regulators, LVS (low voltage swtiches)
	 * PMIC regulators) dont have the capability to call
	 * regulator_set_voltage or regulator_set_optimum_mode
	 * Use this variable to indicate if its a such regulator or not
	 */
	bool set_voltage_sup;

	/* is this regulator enabled? */
	bool is_enabled;
};

struct ath6kl_reg_data {
	/* Regulator Name */
	const char *name;

	/* voltage level at which AR chip can operate */
	u32 low_vol_level;
	u32 high_vol_level;

	/* Voltage regulator handle */
	struct regulator *reg;
};

struct ath6kl_platform_data {
	struct platform_device *pdev;
	struct ath6kl_power_vreg_data *wifi_chip_pwd;
	struct ath6kl_power_vreg_data *wifi_vddpa;
	struct ath6kl_power_vreg_data *wifi_vddio;
};

struct ath6kl_platform_data *gpdata;

#define MAX_PROP_SIZE 32
static int ath6kl_dt_parse_vreg_info(struct device *dev,
	struct ath6kl_power_vreg_data **vreg_data, const char *vreg_name)
{
	int len, ret = 0;
	const __be32 *prop;
	char prop_name[MAX_PROP_SIZE];
	struct ath6kl_power_vreg_data *vreg;
	struct device_node *np = dev->of_node;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "vreg dev tree parse for %s\n", vreg_name);

	snprintf(prop_name, MAX_PROP_SIZE, "%s-supply", vreg_name);
	if (of_parse_phandle(np, prop_name, 0)) {
		vreg = devm_kzalloc(dev, sizeof(*vreg), GFP_KERNEL);
		if (!vreg) {
			ath6kl_err("No memory for vreg: %s\n", vreg_name);
			ret = -ENOMEM;
			goto err;
		}

		vreg->name = vreg_name;

		snprintf(prop_name, MAX_PROP_SIZE,
				"%s-voltage-level", vreg_name);
		prop = of_get_property(np, prop_name, &len);
		if (!prop || (len != (2 * sizeof(__be32)))) {
			ath6kl_dbg(ATH6KL_DBG_BOOT, "%s %s property\n",
				prop ? "invalid format" : "no", prop_name);
		} else {
			vreg->low_vol_level = be32_to_cpup(&prop[0]);
			vreg->high_vol_level = be32_to_cpup(&prop[1]);
		}

		*vreg_data = vreg;
		ath6kl_dbg(ATH6KL_DBG_BOOT, "%s: vol=[%d %d]uV\n",
			vreg->name, vreg->low_vol_level,
			vreg->high_vol_level);
	}  else
		ath6kl_info("%s: is not provided in device tree\n", vreg_name);

err:
	return ret;
}

static int ath6kl_vreg_disable(struct ath6kl_power_vreg_data *vreg)
{
	int rc = 0;

	if (!vreg)
		return rc;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "vreg_disable for : %s\n", vreg->name);

	if (vreg->is_enabled) {
		rc = regulator_disable(vreg->reg);
		if (rc) {
			ath6kl_err("regulator_disable(%s) failed. rc=%d\n",
					vreg->name, rc);
			goto out;
		}
		vreg->is_enabled = false;

		if (vreg->set_voltage_sup) {
			/* Set the min voltage to 0 */
			rc = regulator_set_voltage(vreg->reg,
						0,
						vreg->high_vol_level);
			if (rc) {
				ath6kl_err("vreg_set_vol(%s) failed rc=%d\n",
						vreg->name, rc);
				goto out;

			}
		}
	}

out:
	return rc;
}

static int ath6kl_vreg_init(struct ath6kl_power_vreg_data *vreg)
{
	int rc = 0;
	struct device *dev = &(gpdata->pdev->dev);

	ath6kl_dbg(ATH6KL_DBG_BOOT, "vreg_get for : %s\n", vreg->name);

	/* Get the regulator handle */
	vreg->reg = regulator_get(dev, vreg->name);
	if (IS_ERR(vreg->reg)) {
		rc = PTR_ERR(vreg->reg);
		ath6kl_err("%s: regulator_get(%s) failed. rc=%d\n",
			__func__, vreg->name, rc);
		goto out;
	}

	if ((regulator_count_voltages(vreg->reg) > 0)
			&& (vreg->low_vol_level) && (vreg->high_vol_level))
		vreg->set_voltage_sup = 1;

out:
	return rc;
}

static int ath6kl_vreg_enable(struct ath6kl_power_vreg_data *vreg)
{
	int rc = 0;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "vreg_en for : %s\n", vreg->name);

	if (!vreg->is_enabled) {
		if (vreg->set_voltage_sup) {
			rc = regulator_set_voltage(vreg->reg,
						vreg->low_vol_level,
						vreg->high_vol_level);
			if (rc) {
				ath6kl_err("vreg_set_vol(%s) failed rc=%d\n",
						vreg->name, rc);
				goto out;
			}
		}

		rc = regulator_enable(vreg->reg);
		if (rc) {
			ath6kl_err("regulator_enable(%s) failed. rc=%d\n",
					vreg->name, rc);
			goto out;
		}
		vreg->is_enabled = true;
	}
out:
	return rc;
}

static int ath6kl_configure_vreg(struct ath6kl_power_vreg_data *vreg)
{
	int rc = 0;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "config %s\n", vreg->name);

	/* Get the regulator handle for vreg */
	if (!(vreg->reg)) {
		rc = ath6kl_vreg_init(vreg);
		if (rc < 0)
			return rc;
	}

	rc = ath6kl_vreg_enable(vreg);
	if (rc < 0)
		return rc;

	return rc;
}

static int ath6kl_platform_power(struct ath6kl_platform_data *pdata, int on)
{
	int rc = 0;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "%s on: %d\n", __func__, on);

	if (on) {
		if (pdata->wifi_vddpa != NULL) {
			/* Set voltage to 3.3v */
			rc = ath6kl_configure_vreg(pdata->wifi_vddpa);
			if (rc < 0) {
				ath6kl_err("power on wifi_vddpa error\n");
				goto chip_pwd_fail;
			}

			regulator_set_voltage(pdata->wifi_vddpa->reg,
				VDD_PA_MAX_VOLTAGE, VDD_PA_MAX_VOLTAGE);
		}

		if (pdata->wifi_vddio != NULL) {
			/* Set voltage to 1.8v */
			rc = ath6kl_configure_vreg(pdata->wifi_vddio);
			if (rc < 0) {
				ath6kl_err("power on wifi_vddio error\n");
				goto chip_pwd_fail;
			}

			regulator_set_voltage(pdata->wifi_vddio->reg,
				VDD_IO_MAX_VOLTAGE, VDD_IO_MAX_VOLTAGE);
		}

		/* delay a while for regulator setting done */
		mdelay(100);

		rc = ath6kl_configure_vreg(pdata->wifi_chip_pwd);
		if (rc < 0) {
			ath6kl_err("power on chip_pwd error\n");
			goto chip_pwd_fail;
		}

	} else {
		rc = ath6kl_vreg_disable(pdata->wifi_chip_pwd);

		/* Set voltage to 3.0v */
		if (pdata->wifi_vddpa != NULL &&
			!IS_ERR(pdata->wifi_vddpa->reg)) {
			regulator_set_voltage(pdata->wifi_vddpa->reg,
				VDD_PA_MIN_VOLTAGE, VDD_PA_MAX_VOLTAGE);
			rc = ath6kl_vreg_disable(pdata->wifi_vddpa);
		}

		/* Set voltage to 1.8v */
		if (pdata->wifi_vddio != NULL &&
			!IS_ERR(pdata->wifi_vddio->reg)) {
			regulator_set_voltage(pdata->wifi_vddio->reg,
				VDD_IO_MIN_VOLTAGE, VDD_IO_MAX_VOLTAGE);
			rc = ath6kl_vreg_disable(pdata->wifi_vddio);
		}
	}

	return rc;

chip_pwd_fail:
	ath6kl_vreg_disable(pdata->wifi_chip_pwd);

	return rc;
}

#ifdef ATH6KL_HSIC_RECOVER
void ath6kl_hsic_rediscovery(void)
{
#ifdef ATH6KL_BUS_VOTE
	mdelay(100);
	ath6kl_hsic_bind(0);

	/* delay a while */
	mdelay(1000);
	ath6kl_hsic_bind(1);
#endif
}

#endif

#ifdef ATH6KL_BUS_VOTE

static bool previous;
static int ath6kl_toggle_radio(void *data, int on)
{
	int ret = 0;
	int (*power_control)(int enable);

	ath6kl_dbg(ATH6KL_DBG_BOOT, "%s toggle ratio %s\n",
					__func__, on ? "on" : "off");

	power_control = data;
	if (previous != on)
		ret = (*power_control)(on);
	if (!ret)
		previous = on;
	return ret;
}

int ath6kl_hsic_bind(int bind)
{
	char buf[16];
	int length;

	ath6kl_dbg(ATH6KL_DBG_BOOT, "%s, bind: %d\n", __func__, bind);

	if (bind) {
		length = snprintf(buf, sizeof(buf), "%s\n", "msm_hsic_host");

		android_readwrite_file(
			"/sys/bus/platform/drivers/msm_hsic_host/bind",
			NULL, buf, length);
	} else {
		length = snprintf(buf, sizeof(buf), "%s\n", "msm_hsic_host");

		android_readwrite_file(
			"/sys/bus/platform/drivers/msm_hsic_host/unbind",
			NULL, buf, length);
	}

	return 0;
}

static int ath6kl_hsic_probe(struct platform_device *pdev)
{
	struct ath6kl_platform_data *pdata = NULL;
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (machine_is_apq8064_dma()) {
		ath6kl_dbg(ATH6KL_DBG_BOOT, "%s\n", __func__);
		previous = 0;
		ath6kl_toggle_radio(pdev->dev.platform_data, 1);
	} else {

		ath6kl_bus_scale_pdata = msm_bus_cl_get_pdata(pdev);
		bus_perf_client =
			msm_bus_scale_register_client(
				ath6kl_bus_scale_pdata);
		msm_bus_scale_client_update_request(bus_perf_client, 4);

		pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);

		if (!pdata) {
			ath6kl_err("%s: Could not allocate memory for platform data\n",
				__func__);
			return -ENOMEM;
		}

		if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_chip_pwd,
				"qca,wifi-chip-pwd") != 0) {
			ath6kl_err("%s: parse vreg info for %s error\n",
				"chip_pwd", __func__);
			goto err;
		}

		if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_vddpa,
				"qca,wifi-vddpa") != 0) {
			ath6kl_err("%s: parse vreg info for %s error\n",
				"vddpa", __func__);
			goto err;
		}

		if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_vddio,
				"qca,wifi-vddio") != 0) {
			ath6kl_err("%s: parse vreg info for %s error\n",
				"vddio", __func__);
			goto err;
		}

		pdata->pdev = pdev;
		platform_set_drvdata(pdev, pdata);
		gpdata = pdata;

		if (pdata->wifi_chip_pwd != NULL) {
			ret = ath6kl_platform_power(pdata, 1);

			if (ret == 0 && ath6kl_bt_on == 0)
				ath6kl_hsic_bind(1);

			*platform_has_vreg = 1;
		}
	}

	return ret;

err:
	if (pdata != NULL)
		devm_kfree(dev, pdata);

	return -EINVAL;
}

static int ath6kl_hsic_remove(struct platform_device *pdev)
{
	struct ath6kl_platform_data *pdata = platform_get_drvdata(pdev);

	if (machine_is_apq8064_dma()) {
		ath6kl_toggle_radio(pdev->dev.platform_data, 0);
	} else {
		msm_bus_scale_client_update_request(bus_perf_client, 1);
		if (bus_perf_client)
			msm_bus_scale_unregister_client(bus_perf_client);

		if (pdata->wifi_chip_pwd != NULL)  {
			int ret;

			ret = ath6kl_platform_power(pdata, 0);

			if (pdata->wifi_chip_pwd->reg)
				regulator_put(pdata->wifi_chip_pwd->reg);

			if (pdata->wifi_vddpa != NULL && pdata->wifi_vddpa->reg)
				regulator_put(pdata->wifi_vddpa->reg);

			if (pdata->wifi_vddio != NULL && pdata->wifi_vddio->reg)
				regulator_put(pdata->wifi_vddio->reg);

			if (ret == 0 && ath6kl_bt_on == 0)
				ath6kl_hsic_bind(0);
		}
	}

	return 0;
}

static const struct of_device_id ath6kl_hsic_dt_match[] = {
	{ .compatible = "qca,ar6004-hsic",},
	{}
};

MODULE_DEVICE_TABLE(of, ath6kl_hsic_dt_match);

static struct platform_driver ath6kl_hsic_device = {
	.probe  = ath6kl_hsic_probe,
	.remove = ath6kl_hsic_remove,
	.driver = {
		.name   = "ath6kl_hsic",
		.of_match_table = ath6kl_hsic_dt_match,
	}
};

int ath6kl_hsic_init_msm(u8 *has_vreg)
{
	int ret;

	platform_has_vreg = has_vreg;
	ret = platform_driver_register(&ath6kl_hsic_device);

	return ret;
}

void ath6kl_hsic_exit_msm(void)
{
	platform_driver_unregister(&ath6kl_hsic_device);
}
#else

struct semaphore wifi_control_sem;

static int ath6kl_sdio_probe(struct platform_device *pdev)
{
	struct ath6kl_platform_data *pdata = NULL;
	struct device *dev = &pdev->dev;
	int ret = 0;
	int length;
	char buf[3];

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);

	if (!pdata) {
		ath6kl_err("%s: Could not allocate memory for platform data\n",
				__func__);
		up(&wifi_control_sem);
		return -ENOMEM;
	}

	if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_chip_pwd,
		"qca,wifi-chip-pwd") != 0) {
		ath6kl_err("%s: parse vreg info error\n", __func__);
		goto err;
	}

	if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_vddpa,
					"qca,wifi-vddpa") != 0) {
					ath6kl_err("%s: parse vreg info for %s error\n",
					"vddpa", __func__);
					goto err;
	}

	if (ath6kl_dt_parse_vreg_info(dev, &pdata->wifi_vddio,
					"qca,wifi-vddio") != 0) {
					ath6kl_err("%s: parse vreg info for %s error\n",
					"vddio", __func__);
					goto err;
	}

	pdata->pdev = pdev;
	platform_set_drvdata(pdev, pdata);
	gpdata = pdata;

	if (pdata->wifi_chip_pwd != NULL) {
		ret = ath6kl_platform_power(pdata, 1);
		if (ret == 0) {
			mdelay(50);
			length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
			android_readwrite_file(
				"/sys/devices/msm_sdcc.3/polling",
					NULL, buf, length);
			length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
			android_readwrite_file(
				"/sys/devices/msm_sdcc.3/polling",
					NULL, buf, length);
			mdelay(500);
		}
	}

	up(&wifi_control_sem);
	return ret;

err:
	if (pdata != NULL)
		devm_kfree(dev, pdata);

	up(&wifi_control_sem);
	return -EINVAL;
}

static int ath6kl_sdio_remove(struct platform_device *pdev)
{
	char buf[3];
	int length;
	struct ath6kl_platform_data *pdata = platform_get_drvdata(pdev);

	if (pdata->wifi_chip_pwd != NULL &&
		!IS_ERR(pdata->wifi_chip_pwd->reg))  {

		ath6kl_platform_power(pdata, 0);
		regulator_put(pdata->wifi_chip_pwd->reg);

		if (pdata->wifi_vddpa != NULL &&
			pdata->wifi_vddpa->reg)
			regulator_put(pdata->wifi_vddpa->reg);

		if (pdata->wifi_vddio != NULL &&
			pdata->wifi_vddio->reg)
			regulator_put(pdata->wifi_vddio->reg);

		mdelay(50);
		length = snprintf(buf, sizeof(buf), "%d\n", 1 ? 1 : 0);
		android_readwrite_file("/sys/devices/msm_sdcc.3/polling",
			NULL, buf, length);
		length = snprintf(buf, sizeof(buf), "%d\n", 0 ? 1 : 0);
		android_readwrite_file("/sys/devices/msm_sdcc.3/polling",
			NULL, buf, length);
		mdelay(500);
	}



	up(&wifi_control_sem);
	return 0;
}

static const struct of_device_id ath6kl_sdio_dt_match[] = {
	{ .compatible = "qca,ar6004-sdio",},
	{}
};

MODULE_DEVICE_TABLE(of, ath6kl_sdio_dt_match);

static struct platform_driver ath6kl_sdio_device = {
	.probe  = ath6kl_sdio_probe,
	.remove = ath6kl_sdio_remove,
	.driver = {
		.name   = "ath6kl_sdio",
		.of_match_table = ath6kl_sdio_dt_match,
	}
};

void ath6kl_sdio_init_msm(void)
{
	int ret;

	sema_init(&wifi_control_sem, 1);
	down(&wifi_control_sem);

	ret = platform_driver_register(&ath6kl_sdio_device);

	/* Waiting callback after platform_driver_register */
	if (down_timeout(&wifi_control_sem,  msecs_to_jiffies(5000)) != 0) {
		ret = -EINVAL;
		printk(KERN_INFO "platform_driver_register timeout\n");
		return;
	}

	return;
}

void ath6kl_sdio_exit_msm(void)
{
	platform_driver_unregister(&ath6kl_sdio_device);

	/* Waiting callback after platform_driver_register */
	if (down_timeout(&wifi_control_sem,  msecs_to_jiffies(5000)) != 0) {
		printk(KERN_INFO "platform_driver_unregister timeout\n");
		return;
	}
}

#endif /* #ifdef ATH6KL_BUS_VOTE */

#endif
