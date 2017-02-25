/*
 * kernel/private/drivers/lens/tamron_m13vp288ir/m13vp288ir_piris.c
 *
 * History:
 *	2012/06/29 - [Louis Sun]
 *    2014/10/17 - [Peter Jiao] Mod for S2L on old S2 version
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
#include <mach/hardware.h>
#include <plat/fb.h>
#include <plat/ambsyncproc.h>
#include <plat/ambasyncproc.h>
#include <plat/ambevent.h>

#include <plat/ambcache.h>
#include <mach/init.h>

#include <amba_lens_interface.h>
#include "../amba_lens.h"
#include "m13vp288ir_piris_priv.h"


//static u8 * amba_p_iris_buffer = NULL;

amba_p_iris_controller_t amba_p_iris_controller = {
	.cfg = {
				.gpio_id[0] = 101,	// IN1 for LB1909M , on sensor daughter board
				.gpio_id[1] = 102,	// IN2 for LB1909M , on sensor daughter board
				.gpio_id[2] = 95,	// EN for LB1909M , on sensor daughter board
				.max_mechanical = MAX_OPEN_CLOSE_STEP + 2,
				.max_optical = MAX_OPEN_CLOSE_STEP + 1,
				.min_mechanical = 0,
				.min_optical = 0,
				.timer_period = 10,	//every 10ms
				.gpio_val = 0,
			},
	.pos = P_IRIS_DEFAULT_POS,
	.state = P_IRIS_WORK_STATE_NOT_INIT,
	.steps_to_move = 0,
};


DEFINE_MUTEX(amba_p_iris_mutex);

void amba_p_iris_lock(void)
{
	mutex_lock(&amba_p_iris_mutex);
}

void amba_p_iris_unlock(void)
{
	mutex_unlock(&amba_p_iris_mutex);
}

static long amba_p_iris_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	amba_p_iris_lock();

	switch (cmd) {
		case AMBA_IOC_P_IRIS_SET_POS:
			rval = amba_p_iris_set_position(filp->private_data, (int)arg);
			break;
		case AMBA_IOC_P_IRIS_RESET:
			rval = amba_p_iris_reset(filp->private_data, (int)arg);
			break;
		case AMBA_IOC_P_IRIS_MOVE_STEPS:
			rval = amba_p_iris_move_steps(filp->private_data, (int)arg);
			break;
		case AMBA_IOC_P_IRIS_GET_POS:
			rval = amba_p_iris_get_position(filp->private_data, (int *)arg);
			break;
		case AMBA_IOC_P_IRIS_GET_STATE:
			rval = amba_p_iris_get_state(filp->private_data, (int *)arg);
			break;
		case AMBA_IOC_P_IRIS_CONFIG:
			rval = amba_p_iris_config(filp->private_data, (int)arg);
			break;
		default:
			rval = -ENOIOCTLCMD;
			break;
	}

	amba_p_iris_unlock();
	return rval;
}

static atomic_t lens_reference_count = ATOMIC_INIT(0);

static int amba_p_iris_open(struct inode *inode, struct file *filp)
{
	amba_p_iris_context_t *context = NULL;

	if(atomic_inc_return(&lens_reference_count) != 1) {
			printk(KERN_ERR "lens: device is already openend\n");
			return -EBUSY;
	}

	context = kzalloc(sizeof(amba_p_iris_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_p_iris_mutex;
	//context->buffer = amba_p_iris_buffer;
	context->controller = &amba_p_iris_controller;

	filp->private_data = context;

	return 0;
}

static int amba_p_iris_release(struct inode *inode, struct file *filp)
{
	if(!atomic_dec_and_test(&lens_reference_count)) {
		printk(KERN_ERR "Open lens time more than once, but it's impossible! \n");
		atomic_set(&lens_reference_count, 0);
		return 0;
	}

	kfree(filp->private_data);
	return 0;
}

static struct amba_lens_operations dev_fops = {
	.lens_dev_name = "tamron_m13vp288ir",
	.lens_dev_ioctl = amba_p_iris_ioctl,
	.lens_dev_open = amba_p_iris_open,
	.lens_dev_release = amba_p_iris_release
};

static int __init amba_p_iris_init(void)
{
/*	amba_p_iris_buffer = (void *)__get_free_page(GFP_KERNEL);
	if (amba_p_iris_buffer == NULL) {
		return -ENOMEM;
	}
*/
	if(amba_p_iris_init_config(&amba_p_iris_controller)<0) {
		printk(KERN_DEBUG "failed to init piris config for %s.\n", dev_fops.lens_dev_name);
		return -1;
	}

	if(amba_lens_register(&dev_fops)<0)
		return -1;

	return 0;
}

static void __exit amba_p_iris_exit(void)
{
	amba_lens_logoff();

	amba_p_iris_deinit(&amba_p_iris_controller);
}

module_init(amba_p_iris_init);
module_exit(amba_p_iris_exit);

MODULE_DESCRIPTION("Ambarella P-Iris driver for Tamron m13vp288ir with LB1909M");
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Louis Sun, <lysun@ambarella.com>");
MODULE_ALIAS("p-iris-driver");

