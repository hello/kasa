/*
 * kernel/private/drivers/lens/amba_lens.c
 *
 * History:
 *    2014/10/10 - [Peter Jiao]
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
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <asm/dma.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <iav_devnum.h>

#include "amba_lens.h"


static const char * amba_lens_name = "amb_lens";
static int amba_lens_major = AMBA_DEV_MAJOR;
static int amba_lens_minor = (AMBA_DEV_MINOR_PUBLIC_START + 18);
static int amba_lens_registered = 0;
static struct cdev amba_lens_cdev;

static struct amba_lens_operations impl_fops = {
	.lens_dev_name = NULL,
	.lens_dev_ioctl = NULL,
	.lens_dev_open = NULL,
	.lens_dev_release = NULL
};

int amba_lens_register(struct amba_lens_operations *lens_fops)
{
	if((!lens_fops->lens_dev_name)||(!lens_fops->lens_dev_ioctl)||(!lens_fops->lens_dev_open)||(!lens_fops->lens_dev_release)) {
		printk(KERN_ERR "Invalid lens params, failed %s device register\n", amba_lens_name);
		return -1;
	}

	if(amba_lens_registered) {
		printk(KERN_ERR "%s already has device, failed %s register\n", amba_lens_name, lens_fops->lens_dev_name);
		return -1;
	}

	impl_fops.lens_dev_name = lens_fops->lens_dev_name;
	impl_fops.lens_dev_ioctl = lens_fops->lens_dev_ioctl;
	impl_fops.lens_dev_open = lens_fops->lens_dev_open;
	impl_fops.lens_dev_release = lens_fops->lens_dev_release;

	amba_lens_registered = 1;

	printk("%s registered on %s.\n", impl_fops.lens_dev_name, amba_lens_name);

	return 0;
}
EXPORT_SYMBOL(amba_lens_register);

void amba_lens_logoff(void)
{
	impl_fops.lens_dev_name = NULL;
	impl_fops.lens_dev_ioctl = NULL;
	impl_fops.lens_dev_open = NULL;
	impl_fops.lens_dev_release = NULL;

	amba_lens_registered = 0;
}
EXPORT_SYMBOL(amba_lens_logoff);

static long amba_lens_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if(!impl_fops.lens_dev_ioctl) {
		printk(KERN_ERR "No registered device, failed %s ioctl\n", amba_lens_name);
		return -1;
	}

	return impl_fops.lens_dev_ioctl(filp, cmd, arg);
}

static int amba_lens_open(struct inode *inode, struct file *filp)
{
	if(!impl_fops.lens_dev_open) {
		printk(KERN_ERR "No registered device, failed %s open\n", amba_lens_name);
		return -1;
	}

	return impl_fops.lens_dev_open(inode, filp);
}

static int amba_lens_release(struct inode *inode, struct file *filp)
{
	if(!impl_fops.lens_dev_release) {
		printk(KERN_ERR "No registered device, failed %s release\n", amba_lens_name);
		return -1;
	}

	return impl_fops.lens_dev_release(inode, filp);
}

static struct file_operations amba_lens_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_lens_ioctl,
	.open = amba_lens_open,
	.release = amba_lens_release
};

static int __init amba_lens_init(void)
{
	dev_t dev_id;
	int rval;

	amba_lens_logoff(); //clear off registered lens

	if (amba_lens_major) {
		dev_id = MKDEV(amba_lens_major, amba_lens_minor);
		rval = register_chrdev_region(dev_id, 1, amba_lens_name);
	} else {
		rval = alloc_chrdev_region(&dev_id, 0, 1, amba_lens_name);
		amba_lens_major = MAJOR(dev_id);
	}

	if (rval) {
		printk(KERN_DEBUG "failed to get dev region for %s.\n", amba_lens_name);
		return rval;
	}

	cdev_init(&amba_lens_cdev, &amba_lens_fops);
	amba_lens_cdev.owner = THIS_MODULE;
	rval = cdev_add(&amba_lens_cdev, dev_id, 1);
	if (rval) {
		printk(KERN_DEBUG "cdev_add failed for %s, error = %d.\n", amba_lens_name, rval);
		unregister_chrdev_region(dev_id, 1);
		return rval;
	}

	printk("%s_created %d:%d.\n", amba_lens_name, MAJOR(dev_id), MINOR(dev_id));

	return 0;
}

static void __exit amba_lens_exit(void)
{
	dev_t dev_id;

	cdev_del(&amba_lens_cdev);

	dev_id = MKDEV(amba_lens_major, amba_lens_minor);
	unregister_chrdev_region(dev_id, 1);
}

module_init(amba_lens_init);
module_exit(amba_lens_exit);

MODULE_DESCRIPTION("Ambarella lens driver");
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Peter Jiao <hgjiao@ambarella.com>");
MODULE_ALIAS("lens-driver");

