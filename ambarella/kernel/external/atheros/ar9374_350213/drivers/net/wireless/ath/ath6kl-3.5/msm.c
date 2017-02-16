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
#ifdef CONFIG_ANDROID
#include "core.h"

/* BeginMMC polling stuff */
#ifdef CONFIG_MMC_MSM_SDC3_POLLING
#define MMC_MSM_DEV "msm_sdcc.3"
#else
#define MMC_MSM_DEV "msm_sdcc.4"
#endif
/* End MMC polling stuff */

#include <linux/platform_device.h>
#include <linux/wlan_plat.h>

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
		int mode = (wbuf) ? O_RDWR : O_RDONLY;
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
#endif
