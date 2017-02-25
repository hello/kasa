/*
 * dsplog_drv.c
 *
 * History:
 * 	2013/09/30 - [Louis Sun] create new dsplog driver
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

#include <config.h>

#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ambbus.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#include <iav_devnum.h>

#include "iav_utils.h"

#include "dsplog_priv.h"
#include "dsplog_drv.h"

/* Now separate dsplog driver from dsp driver.
    DSP driver is kernel mode only driver used by IAV driver.
    DSP log driver supports DSP logging and it is not called by dsp driver.
    DSP log driver is optional and only used for debugging
   */


MODULE_AUTHOR("Louis Sun <lysun@ambarella.com>");
MODULE_DESCRIPTION("Ambarella dsplog driver");
MODULE_LICENSE("Proprietary");


amba_dsplog_controller_t    g_dsplog_controller;

static int amba_dsplog_major = AMBA_DEV_MAJOR;
static int amba_dsplog_minor = (AMBA_DEV_MINOR_PUBLIC_END + 9);
static const char * amba_dsplog_name = "dsplog";
static struct cdev amba_dsplog_cdev;
int clean_dsplog_memory = 1; /* Set it into 0 in fastboot case */
module_param(clean_dsplog_memory, int, 0644);

static ssize_t amba_dsplog_read(struct file *filp,
	char __user *buffer, size_t count, loff_t *offp)
{
	return dsplog_read(buffer, count);
	return -1;
}

DEFINE_MUTEX(amba_dsplog_mutex);

void dsplog_lock(void)
{
	mutex_lock(&amba_dsplog_mutex);
}

void dsplog_unlock(void)
{
	mutex_unlock(&amba_dsplog_mutex);
}

static long amba_dsplog_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;

	dsplog_lock();
	switch (cmd) {
	case AMBA_IOC_DSPLOG_START_CAPUTRE:
		rval = dsplog_start_cap();
		break;

	case AMBA_IOC_DSPLOG_STOP_CAPTURE:
		rval = dsplog_stop_cap();
		break;

	case AMBA_IOC_DSPLOG_SET_LOG_LEVEL:
		rval = dsplog_set_level((int)arg);
		break;

	case AMBA_IOC_DSPLOG_GET_LOG_LEVEL:
		rval = dsplog_get_level((int *)arg);
		break;

	case AMBA_IOC_DSPLOG_PARSE_LOG:
		rval = dsplog_parse((int)arg);
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}
	dsplog_unlock();

	return rval;
}

static int amba_dsplog_open(struct inode *inode, struct file *filp)
{
	amba_dsplog_context_t *context;

	context = kzalloc(sizeof(amba_dsplog_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_dsplog_mutex;
	context->controller = &g_dsplog_controller;
	filp->private_data = context;

	return 0;
}

static int amba_dsplog_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

#ifdef CONFIG_PM
int amba_dsplog_suspend(void)
{
	dsplog_suspend();
	return 0;
}
EXPORT_SYMBOL(amba_dsplog_suspend);

int amba_dsplog_resume(void)
{
	dsplog_resume();
	return 0;
}
EXPORT_SYMBOL(amba_dsplog_resume);
#endif

static struct file_operations amba_dsplog_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_dsplog_ioctl,
	.open = amba_dsplog_open,
	.release = amba_dsplog_release,
	.read = amba_dsplog_read,
};

static int __init amba_dsplog_create_dev(int *major, int minor, int numdev,
	const char *name, struct cdev *cdev, struct file_operations *fops)
{
	dev_t dev_id;
	int rval;

	if (*major) {
		dev_id = MKDEV(*major, minor);
		rval = register_chrdev_region(dev_id, numdev, name);
	} else {
		rval = alloc_chrdev_region(&dev_id, 0, numdev, name);
		*major = MAJOR(dev_id);
	}

	if (rval) {
		DRV_PRINT(KERN_DEBUG "failed to get dev region for %s.\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		DRV_PRINT(KERN_DEBUG "cdev_add failed for %s, error = %d.\n", name, rval);
		unregister_chrdev_region(dev_id, 1);
		return rval;
	}

	DRV_PRINT(KERN_DEBUG "%s dev init done, dev_id = %d:%d.\n",
		name, MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static int __init amba_dsplog_init(void)
{
	if ( amba_dsplog_create_dev(&amba_dsplog_major,
		amba_dsplog_minor, 1, amba_dsplog_name,
		&amba_dsplog_cdev, &amba_dsplog_fops) < 0) {
            return -1;
        }

       return dsplog_init();
}


static void __exit amba_dsplog_exit(void)
{

	dev_t dev_id;
	dsplog_deinit(&g_dsplog_controller);
	cdev_del(&amba_dsplog_cdev);
	dev_id = MKDEV(amba_dsplog_major, amba_dsplog_minor);
	unregister_chrdev_region(dev_id, 1);
}

module_init(amba_dsplog_init);
module_exit(amba_dsplog_exit);

