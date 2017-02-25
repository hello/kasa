/*
 * kernel/private/drivers/ambarella/lens/tamron_m13vp288ir/m13vp288ir_piris_timer.c
 *
 * History:
 *	2012/07/2 - [Louis Sun]
 *	2014/06/11 - [Peter Jiao] Mod
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
#include "m13vp288ir_piris_priv.h"





#ifdef P_IRIS_USE_HRTIMER

/* User Kernel HR Timer to implement stepper motor control timer,
     HR Timer has better granularity than normal Timers.
     A5s does not have enough HW interval timer for this , but new chips
     like S2 can also use a dedicate HW interval timer for the control
  */

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>

#define MS_TO_NS(x)	((u32)(((u32)x) * (u32)1000000L))
/* HR timer call back */

struct hrtimer_control_s{
	struct hrtimer hr_timer;
	void *timer_data;
	int timer_initialized;
	TIMER_CALLBACK timer_callback;
} ;

static struct hrtimer_control_s hrtimer_control = {
    .timer_data = NULL,
    .timer_initialized = 0,
    .timer_callback = NULL,
    };
static enum hrtimer_restart hrtimer_callback( struct hrtimer *timer )
{
	ktime_t ktime;

	//get the object pointer
	amba_p_iris_controller_t * p_controller = (struct amba_p_iris_controller_s *)container_of(timer,
		struct amba_p_iris_controller_s, hr_timer);
	TIMER_CALLBACK cb = hrtimer_control.timer_callback;
	if (cb) {
		//printk(KERN_DEBUG "hr callback\n");
		cb(hrtimer_control.timer_data);
	}

//auto restart
	ktime = ktime_set(0, MS_TO_NS(P_IRIS_CONTROL_DEFAULT_PERIOD));

	if (hrtimer_control.timer_initialized) {
		return highres_timer_start(timer, ktime, HRTIMER_MODE_REL);
	} else {
		return HRTIMER_NORESTART;
	}
}


/* init high res timer */
int amba_p_iris_init_timer(amba_p_iris_controller_t * p_controller, void *pcallback)
{
	if (!p_controller || !pcallback)
		return -1;
	hrtimer_control.timer_callback = (TIMER_CALLBACK)pcallback ;
	highres_timer_init(&hrtimer_control.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_control.hr_timer.function = &hrtimer_callback;
	hrtimer_control.timer_data = p_controller;	//prepare for hr timer callback arg

	hrtimer_control.timer_initialized = 1;
	return 0;
}

/* start high res timer */
int amba_p_iris_start_timer(amba_p_iris_controller_t * p_controller)
{
	ktime_t ktime;
	if (!hrtimer_control.timer_initialized)
		return -1;

	ktime = ktime_set(0, MS_TO_NS(P_IRIS_CONTROL_DEFAULT_PERIOD));
	return highres_timer_start(&hrtimer_control.hr_timer, ktime, HRTIMER_MODE_REL);
}


/* deinit high res timer */
int amba_p_iris_deinit_timer(amba_p_iris_controller_t * p_controller)
{
	 //cancel timer
	 if (hrtimer_control.timer_initialized) {
		 highres_timer_cancel(&hrtimer_control.hr_timer);

		 //clean data
		hrtimer_control.timer_callback = NULL;
		hrtimer_control.timer_data = NULL;
		//deinit
		hrtimer_control.timer_initialized = 0;
	 }
	 return 0;
}

#else
/* User Kernel Timer to implement stepper motor control timer,
    The granularity for Kernel Timer for A5s is 10ms.
  */
#include <linux/timer.h>

struct timer_control_s{
	struct timer_list  timer;
	int timer_initialized;
	TIMER_CALLBACK timer_callback;
};

static struct timer_control_s timer_control = {
    .timer_initialized = 0,
    .timer_callback = NULL,
};

static void kernel_timer_callback(unsigned long data)
{

	//get the object pointer
	amba_p_iris_controller_t * p_controller = (amba_p_iris_controller_t *) data;
        TIMER_CALLBACK cb = timer_control.timer_callback;

        //run user callback if installed
	if (cb) {
		//printk(KERN_DEBUG "user callback\n");
		cb((void *)data);
	}
//auto restart
        mod_timer(&timer_control.timer, jiffies + p_controller->cfg.timer_period*HZ/1000L);
}



int amba_p_iris_init_timer(amba_p_iris_controller_t * p_controller, void *p_user_callback)
{
	if (!p_controller || !p_user_callback)
		return -1;
	timer_control.timer_callback = (TIMER_CALLBACK)p_user_callback ;
        timer_control.timer.expires = jiffies + p_controller->cfg.timer_period*HZ/1000L;
        timer_control.timer.data = (unsigned long) p_controller;
	 timer_control.timer.function = &kernel_timer_callback;
        init_timer(&timer_control.timer);
	timer_control.timer_initialized = 1;

	return 0;
}


int amba_p_iris_start_timer(amba_p_iris_controller_t * p_controller)
{
	if (!timer_control.timer_initialized)
		return -1;
        add_timer(&timer_control.timer);

        return 0;
 }



int amba_p_iris_deinit_timer(amba_p_iris_controller_t * p_controller)
{
	 //cancel timer
	 if (timer_control.timer_initialized) {
              del_timer_sync(&timer_control.timer);
		 //clean data
		timer_control.timer_callback = NULL;
		//deinit
		timer_control.timer_initialized = 0;
	 }
	 return 0;
}

#endif


