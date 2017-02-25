/*
 * kernel/private/drivers/ambarella/crypto/amba_crypto.c
 *
 * History:
 *	2011/05/03 - [Jian Tang]
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <iav_devnum.h>
#include <amba_crypto.h>
#include "crypto_api.h"


static const char * amba_crypto_name = "ambac";	// Ambarella Media Processor Cryptography API
static int amba_crypto_major = AMBA_DEV_MAJOR;
static int amba_crypto_minor = (AMBA_DEV_MINOR_PUBLIC_START + 16);
static struct cdev amba_crypto_cdev;
static u8 * amba_crypto_buffer = NULL;


DEFINE_MUTEX(amba_crypto_mutex);

void amba_crypto_lock(void)
{
	mutex_lock(&amba_crypto_mutex);
}

void amba_crypto_unlock(void)
{
	mutex_unlock(&amba_crypto_mutex);
}

static long amba_crypto_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	amba_crypto_lock();

	switch (cmd) {
	case AMBA_IOC_CRYPTO_SET_ALGO:
		rval = amba_crypto_set_algo(filp->private_data, (amba_crypto_algo_t __user *) arg);
		break;
	case AMBA_IOC_CRYPTO_SET_KEY:
		rval = amba_crypto_set_key(filp->private_data, (amba_crypto_key_t __user *) arg);
		break;
	case AMBA_IOC_CRYPTO_ENCRYPTE:
		rval = amba_crypto_encrypt(filp->private_data, (amba_crypto_info_t __user *) arg);
		break;
	case AMBA_IOC_CRYPTO_DECRYPTE:
		rval = amba_crypto_decrypt(filp->private_data, (amba_crypto_info_t __user *) arg);
		break;
	case AMBA_IOC_CRYPTO_SHA:
		rval = amba_crypto_sha(filp->private_data, (amba_crypto_sha_md5_t __user *) arg);
		break;
	case AMBA_IOC_CRYPTO_MD5:
		rval = amba_crypto_md5(filp->private_data, (amba_crypto_sha_md5_t __user *) arg);
		break;
	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	amba_crypto_unlock();
	return rval;
}

static int amba_crypto_open(struct inode *inode, struct file *filp)
{
	amba_crypto_context_t *context = kzalloc(sizeof(amba_crypto_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_crypto_mutex;
	context->buffer = amba_crypto_buffer;

	filp->private_data = context;

	return 0;
}

static int amba_crypto_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static struct file_operations amba_crypto_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_crypto_ioctl,
	.open = amba_crypto_open,
	.release = amba_crypto_release
};

static int __init amba_crypto_create_dev(int *major, int minor, int numdev,
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
		printk(KERN_DEBUG "failed to get dev  region for %s.\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		printk(KERN_DEBUG "cdev_add failed for %s, error = %d.\n", name, rval);
		return rval;
	}

	printk(KERN_DEBUG "%s dev init done, dev_id = %d:%d.\n",
		name, MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static int __init amba_crypto_init(void)
{
	amba_crypto_buffer = (void *)__get_free_page(GFP_KERNEL);
	if (amba_crypto_buffer == NULL) {
		return -ENOMEM;
	}
	return amba_crypto_create_dev(&amba_crypto_major,
		amba_crypto_minor, 1, amba_crypto_name,
		&amba_crypto_cdev, &amba_crypto_fops);
}

static void __exit amba_crypto_exit(void)
{
	free_page((unsigned long)amba_crypto_buffer);
}

module_init(amba_crypto_init);
module_exit(amba_crypto_exit);

MODULE_DESCRIPTION("Ambarella Cryptography API");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jian Tang, <jtang@ambarella.com>");
MODULE_ALIAS("crypo-api");

