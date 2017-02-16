/*
 * kernel/private/drivers/ambarella/debug/debug_core.c
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <plat/debug.h>
#include <iav_devnum.h>
#include <amba_debug.h>
#include <plat/service.h>

#define AMBA_DEBUG_NAME			"ambad"
#define AMBA_DEBUG_DEV_NUM		1
#define AMBA_DEBUG_DEV_MAJOR		AMBA_DEV_MAJOR
#define AMBA_DEBUG_DEV_MINOR		(AMBA_DEV_MINOR_PUBLIC_END + 8)
#define DDR_START			DEFAULT_MEM_START
#define DDR_SIZE			(0x40000000)

static struct cdev amba_debug_cdev;
static dev_t amba_debug_dev_id =
	MKDEV(AMBA_DEBUG_DEV_MAJOR, AMBA_DEBUG_DEV_MINOR);
DEFINE_MUTEX(amba_debug_mutex);

static int amba_debug_vin_srcid = 0;

static long amba_debug_ioctl(struct file *filp,
	unsigned int cmd, unsigned long args)
{
	int				errorCode = 0;

	mutex_lock(&amba_debug_mutex);

	switch (cmd) {
	case AMBA_DEBUG_IOC_GET_DEBUG_FLAG:
		errorCode = put_user(ambarella_debug_level, (u32 __user *)args);
		break;

	case AMBA_DEBUG_IOC_SET_DEBUG_FLAG:
		errorCode = get_user(ambarella_debug_level, (u32 __user *)args);
		break;

	case AMBA_DEBUG_IOC_VIN_SET_SRC_ID:
		amba_debug_vin_srcid = (int)args;
		break;

	case AMBA_DEBUG_IOC_GET_GPIO:
	{
		struct amba_vin_test_gpio	gpio_data;
		struct ambsvc_gpio		gpio_svc;

		errorCode = copy_from_user(&gpio_data,
			(struct amba_vin_test_gpio __user *)args,
			sizeof(gpio_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

		gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
		gpio_svc.gpio = gpio_data.id;
		errorCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
		if (errorCode < 0)
			break;

		gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
		gpio_svc.gpio = gpio_data.id;
		errorCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, &gpio_data.data);
		if (errorCode < 0)
			break;

		errorCode = copy_to_user(
			(struct amba_vin_test_gpio __user *)args,
			&gpio_data, sizeof(gpio_data)) ? -EFAULT : 0;
		if (errorCode < 0)
			break;

		gpio_svc.svc_id = AMBSVC_GPIO_FREE;
		gpio_svc.gpio = gpio_data.id;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}
		break;

	case AMBA_DEBUG_IOC_SET_GPIO:
	{
		struct amba_vin_test_gpio	gpio_data;
		struct ambsvc_gpio		gpio_svc;

		errorCode = copy_from_user(&gpio_data,
			(struct amba_vin_test_gpio __user *)args,
			sizeof(gpio_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

		gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
		gpio_svc.gpio = gpio_data.id;
		errorCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
		if (errorCode < 0)
			break;

		gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
		gpio_svc.gpio = gpio_data.id;
		gpio_svc.value = gpio_data.data;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);

		gpio_svc.svc_id = AMBSVC_GPIO_FREE;
		gpio_svc.gpio = gpio_data.id;
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
	}
		break;

	default:
		errorCode = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&amba_debug_mutex);

	return errorCode;
}

static int amba_debug_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int		errorCode = 0;

	mutex_lock(&amba_debug_mutex);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,	vma->vm_pgoff,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			errorCode = -EINVAL;
	}

	mutex_unlock(&amba_debug_mutex);
	return errorCode;
}

static int amba_debug_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int amba_debug_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations amba_debug_fops = {
	.owner		= THIS_MODULE,
	.open		= amba_debug_open,
	.release	= amba_debug_release,
	.unlocked_ioctl	= amba_debug_ioctl,
	.mmap		= amba_debug_mmap,
};

static void __exit amba_debug_exit(void)
{
	cdev_del(&amba_debug_cdev);
	unregister_chrdev_region(amba_debug_dev_id, AMBA_DEBUG_DEV_NUM);
}

static int __init amba_debug_init(void)
{
	int			errorCode = 0;

	errorCode = register_chrdev_region(amba_debug_dev_id,
		AMBA_DEBUG_DEV_NUM, AMBA_DEBUG_NAME);
	if (errorCode) {
		printk(KERN_ERR "amba_debug_init failed to get dev region.\n");
		goto amba_debug_init_exit;
	}

	cdev_init(&amba_debug_cdev, &amba_debug_fops);
	amba_debug_cdev.owner = THIS_MODULE;
	errorCode = cdev_add(&amba_debug_cdev,
		amba_debug_dev_id, AMBA_DEBUG_DEV_NUM);
	if (errorCode) {
		printk(KERN_ERR "amba_debug_init cdev_add failed %d.\n",
			errorCode);
		goto amba_debug_init_exit;
	}
	printk(KERN_INFO "amba_debug_init %d:%d.\n",
		MAJOR(amba_debug_dev_id), MINOR(amba_debug_dev_id));

amba_debug_init_exit:
	return errorCode;
}

module_init(amba_debug_init);
module_exit(amba_debug_exit);

MODULE_DESCRIPTION("Ambarella kernel debug driver");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

