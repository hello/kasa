/*
 * dsp_log.c
 *
 * History:
 *	2012/10/25 - [Cao Rongrong] Created
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

#include <linux/module.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <plat/ambcache.h>
#include <mach/init.h>
#include <asm/uaccess.h>
#include <iav_ucode_ioctl.h>
#include <iav_devnum.h>
#include "dsp.h"

static struct cdev ucode_cdev;

static void *ucode_user_ptr = NULL;

static const ucode_load_info_t dsp_ucode_info = {
	.map_size	= DSP_UCODE_SIZE,
	.nr_item	= 3,
	.items 		= {
		[0] = {
			.addr_offset	= DSP_CODE_MEMORY_OFFSET,
			.filename	= "orccode.bin",
		},
		[1] = {
			.addr_offset	= DSP_MEMD_MEMORY_OFFSET,
			.filename	= "orcme.bin",
		},
		[2] = {
			.addr_offset	= DSP_BINARY_DATA_MEMORY_OFFSET,
			.filename	= "default_binary.bin",
		},
	}
};

static const struct ucode_version_s *dsp_get_ucode_version(void)
{
	#define	TIME_STRING_OFFSET		(36)
	#define	VERSION_STRING_OFFSET	(32)
	#define	IDSP_STRING_OFFSET		(40)
	#define	UCODE_ARCH_OFFSET		(0)

	static ucode_version_t ucode_version;
	void __iomem *base = NULL;
	u32 tmp, start = DSP_UCODE_START + DSP_CODE_MEMORY_OFFSET;

	if (!request_mem_region(start, 64, "ucode_ver")) {
		pr_err("request_mem_region failed\n");
		return NULL;
	}

	base = ioremap(start, 64);
	if (base == NULL) {
		pr_err("ioremap() failed\n");
		return NULL;
	}

	tmp = *(u32*)(base + TIME_STRING_OFFSET);
	ucode_version.year = (tmp & 0xFFFF0000) >> 16;
	ucode_version.month = (tmp & 0x0000FF00) >> 8;
	ucode_version.day = (tmp & 0x000000FF);

	tmp = *(u32*)(base + VERSION_STRING_OFFSET);
	ucode_version.edition_ver = tmp;

	tmp = *(u32*)(base + IDSP_STRING_OFFSET);
	ucode_version.edition_num = tmp;

	iounmap(base);
	release_mem_region(start, 64);

	start = DSP_BINARY_DATA_START;
	if (!request_mem_region(start, 4, "ucode_arch")) {
		pr_err("request_mem_region failed\n");
		return NULL;
	}

	base = ioremap(start, 4);
	if (base == NULL) {
		pr_err("ioremap() failed\n");
		return NULL;
	}
	tmp = *(u32*)(base + UCODE_ARCH_OFFSET);
	ucode_version.chip_arch = tmp ? tmp : UCODE_ARCH_S2L;

	iounmap(base);
	release_mem_region(start, 4);

	return &ucode_version;
}

static long ucode_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;

	switch (cmd) {
	case IAV_IOC_GET_UCODE_INFO:
		rval = copy_to_user((ucode_load_info_t __user*)arg,
			&dsp_ucode_info, sizeof(ucode_load_info_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_GET_UCODE_VERSION:
		rval = copy_to_user((ucode_version_t __user*)arg,
			dsp_get_ucode_version(), sizeof(ucode_version_t)) ? -EFAULT : 0;
		break;

	case IAV_IOC_UPDATE_UCODE:
		if (ucode_user_ptr == NULL) {
			rval = -EPERM;
		} else {
			clean_d_cache(ucode_user_ptr, DSP_UCODE_SIZE);
			rval = 0;
		}
		break;

	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	return rval;
}

static int ucode_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int rval;
	unsigned long size;

	size = vma->vm_end - vma->vm_start;
	if (size != DSP_UCODE_SIZE) {
		rval = -EINVAL;
		goto Done;
	}

	vma->vm_pgoff = DSP_UCODE_START >> PAGE_SHIFT;
	if ((rval = remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) < 0) {
		goto Done;
	}

	ucode_user_ptr = (void *)vma->vm_start;
	rval = 0;

Done:
	return rval;
}

static int ucode_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ucode_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations ucode_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ucode_ioctl,
	.mmap = ucode_mmap,
	.open = ucode_open,
	.release = ucode_release
};

int ucode_dev_init(struct ambarella_dsp *dsp)
{
	dev_t dev_id;
	int rval;

	dev_id = MKDEV(UCODE_DEV_MAJOR, UCODE_DEV_MINOR);

	rval = register_chrdev_region(dev_id, 1, "ucode");
	if (rval < 0) {
		iav_error("ucode: failed to create device\n");
		goto ucode_init_err1;
	}

	cdev_init(&ucode_cdev, &ucode_fops);
	ucode_cdev.owner = THIS_MODULE;

	rval = cdev_add(&ucode_cdev, dev_id, 1);
	if (rval < 0) {
		iav_error("ucode: cdev_add failed, err = %d\n", rval);
		goto ucode_init_err2;
	}

	return 0;

ucode_init_err2:
	unregister_chrdev_region(dev_id, 1);

ucode_init_err1:
	return rval;
}

void ucode_dev_exit(void)
{
	cdev_del(&ucode_cdev);
	unregister_chrdev_region(ucode_cdev.dev, 1);
}


int dsp_init_logbuf(u8 **print_buffer, u32 * buffer_size)
{
	void __iomem *base = NULL;

	base = ioremap_nocache(DSP_LOG_AREA_PHYS, DSP_LOG_SIZE);
	if (!base) {
		DRV_PRINT("Failed to call ioremap() for DSP LOG.\n");
		return -ENOMEM;
	}
	*print_buffer = (u8 *)base;
	*buffer_size = DSP_LOG_SIZE;

        return 0;
}
EXPORT_SYMBOL(dsp_init_logbuf);

int dsp_deinit_logbuf(u8 * print_buffer, u32 buffer_size)
{
	if (print_buffer) {
		iounmap((void __iomem *)print_buffer);
	}
	return 0;
}
EXPORT_SYMBOL(dsp_deinit_logbuf);

