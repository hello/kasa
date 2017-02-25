/*
 * kernel/private/drivers/ambarella/lens/tamron_m13vp288ir/m13vp288ir_piris_api.c
 *
 * History:
 *	2012/06/29 - [Louis Sun]
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

static int reset_flag = 0;

static inline void p_iris_range_check(amba_p_iris_controller_t *p_controller)
{
        if (p_controller->pos > p_controller->cfg.max_mechanical)
                p_controller->pos = p_controller->cfg.max_mechanical;
        else if (p_controller->pos <   p_controller->cfg.min_mechanical)
                p_controller->pos = p_controller->cfg.min_mechanical;
}



/* callback runned by the high resolution timer */
static int p_iris_timer_callback(void * p_iris_controller)
{
        amba_p_iris_controller_t *p_controller = (amba_p_iris_controller_t*) p_iris_controller;
	if (!p_controller) {
		printk(KERN_DEBUG "Error: null arg in p_iris_timer_callback!\n");
		return -1;
	}
	//lock mutex and then move, avoid the case of being interrupted by another user IOCTL
	if( p_controller->steps_to_move > 0) {
            p_controller->state = P_IRIS_WORK_STATE_RUNNING;
           // printk(KERN_DEBUG "move 1\n");
            amba_p_iris_impl_move_one_step(p_controller, 1);
            p_controller->steps_to_move--;
            p_controller->pos ++;
            //position range check.
            p_iris_range_check(p_controller);
 	} else if (p_controller->steps_to_move < 0) {
            p_controller->state = P_IRIS_WORK_STATE_RUNNING;
           // printk(KERN_DEBUG "move -1\n");
            amba_p_iris_impl_move_one_step(p_controller, -1);
            p_controller->steps_to_move++;
            p_controller->pos --;
             //position range check.
            p_iris_range_check(p_controller);
  	} else {
            if (p_controller->state == P_IRIS_WORK_STATE_RUNNING) {
                p_controller->state = P_IRIS_WORK_STATE_READY;
				if(reset_flag)
                	complete(&(p_controller->move_compl));
            }

	}

	return 0;
}


static int p_iris_reset(amba_p_iris_controller_t * p_controller)
{
	reset_flag = 1;

 	amba_p_iris_impl_reset(p_controller, -(MAX_OPEN_CLOSE_STEP + MORE_STEPS_TO_MOVE));

	//move to assure init stat A: H-L, B:L-H (IN1=0, IN2=0) at present motor wiring
	while(amba_p_iris_impl_check(p_controller)) {
		p_controller->steps_to_move = -1;
		wait_for_completion(&(p_controller->move_compl));
	}

  	printk(KERN_DEBUG "P-Iris reset done,  reset pos = 0 \n");
   	p_controller->pos = P_IRIS_DEFAULT_POS;
	reset_flag = 0;

  	return 0;
}


/* the first function to init P-iris and set default confg and state to make it ready to run */
int amba_p_iris_init_config(amba_p_iris_controller_t * p_controller)
{
	if (p_controller->state != P_IRIS_WORK_STATE_NOT_INIT) {
		printk(KERN_DEBUG "P-iris state = %d, cannot init again \n", p_controller->state);
		return -EINVAL;
	}

  	//init completion
 	init_completion(&(p_controller->move_compl));

	//iris device init
	if (amba_p_iris_impl_init(p_controller) < 0) {
		printk(KERN_DEBUG "init p_iris device failed\n");
		return -1;
	}

	//iris timer init
	if (amba_p_iris_init_timer(p_controller, p_iris_timer_callback) < 0) {
		printk(KERN_DEBUG "init p_iris timer failed\n");
		return -1;
	}

	if (amba_p_iris_start_timer(p_controller) < 0) {
		printk(KERN_DEBUG "start p_iris timer failed\n");
		return -1;
	}

	p_controller->state = P_IRIS_WORK_STATE_PREPARING;

	return 0;
}

/* reset iris to default position and state, likes the 'init again', however, no need arg */
int amba_p_iris_reset(amba_p_iris_context_t * context, int arg)
{
	//check state
	amba_p_iris_controller_t * p_controller = context->controller;
	if (p_controller->state == P_IRIS_WORK_STATE_NOT_INIT) {
            printk(KERN_DEBUG "cannot reset non initialized p-iris \n");
            return -EINVAL;
	} else  if (p_controller->state != P_IRIS_WORK_STATE_READY) {
            return -EAGAIN;
	}else   {
		return p_iris_reset(p_controller);
	}
}

/* make moves */
int amba_p_iris_move_steps(amba_p_iris_context_t * context, int steps)
{
	amba_p_iris_controller_t * p_controller = context->controller;

	//check state
	if (p_controller->state != P_IRIS_WORK_STATE_READY)
		return -EAGAIN;
	//valid steps, if it has no move, then don't move
        if (steps == 0)
            return 0;
	//if OK, then move
	p_controller->steps_to_move += steps;

    return 0;
}

/* Set P-IRIS position */
int amba_p_iris_set_position(amba_p_iris_context_t * context, int position)
{
	amba_p_iris_controller_t * p_controller = context->controller;
	int piris_pos, steps = 0;

	//check state
	if(p_controller->state != P_IRIS_WORK_STATE_READY)
		return -EAGAIN;

	/* use full step drive in LB1909M*/
	piris_pos = position / 2;
	steps = piris_pos - p_controller->pos;

	//valid steps, if it has no move, then don't move
	if(steps == 0)
		return 0;

	//if OK, then move
	p_controller->steps_to_move += steps;
	//don't use hold wait for ae_loop!!!
	p_controller->state = P_IRIS_WORK_STATE_RUNNING;

	return 0;
}

/* report position */
int amba_p_iris_get_position(amba_p_iris_context_t * context, int __user * pos)
{
	amba_p_iris_controller_t * p_controller = context->controller;
	if (p_controller->state == P_IRIS_WORK_STATE_NOT_INIT)
		return -EAGAIN;
	return copy_to_user(pos, &p_controller->pos, sizeof(*pos)) ? -EFAULT : 0;
}

/* report state */
int amba_p_iris_get_state(amba_p_iris_context_t * context, int __user * state)
{
	amba_p_iris_controller_t * p_controller = context->controller;

	return copy_to_user(state, &p_controller->state, sizeof(*state)) ? -EFAULT : 0;
}

/* config p-iris */
int amba_p_iris_config(amba_p_iris_context_t * context, int enable)
{
	amba_p_iris_controller_t * p_controller = context->controller;

	if(p_controller->state == P_IRIS_WORK_STATE_NOT_INIT) {
            printk(KERN_DEBUG "cannot config non initialized p-iris\n");
            return -EINVAL;
	} else  if(p_controller->state == P_IRIS_WORK_STATE_RUNNING) {
            return -EAGAIN;
	} else {
		if(enable)
			amba_p_iris_impl_config(p_controller, 1);
		else
			amba_p_iris_impl_config(p_controller, 0);
		return 0;
	}
}

int amba_p_iris_deinit(amba_p_iris_controller_t * p_controller)
{
	/* should call deinit, here,
	  auto called when module removed
	 */

	amba_p_iris_impl_disable(p_controller);

	if (p_controller->state != P_IRIS_WORK_STATE_NOT_INIT) {
		return amba_p_iris_deinit_timer(p_controller);
	} else {
		return 0;
	}
}


