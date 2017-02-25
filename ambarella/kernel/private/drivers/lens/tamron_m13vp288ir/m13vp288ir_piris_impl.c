/*
 * kernel/private/drivers/ambarella/lens/tamron_m13vp288ir/m13vp288ir_piris_impl.c
 *
 * History:
 *	2012/07/2 - [Louis Sun]
 *	2014/10/22 - [Peter Jiao] Mod for S2L on old S2 version
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
#include <plat/iav_helper.h>

#include <amba_lens_interface.h>
#include "m13vp288ir_piris_priv.h"
#include <asm/gpio.h>

static struct ambsvc_gpio gpio_svc;

static int request_gpio(int gpio_id)
{
        printk(KERN_DEBUG "Request GPIO %d\n", gpio_id);

		gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
		gpio_svc.gpio = GPIO(gpio_id);
		return ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
}

static void release_gpio(int gpio_id)
{
        printk(KERN_DEBUG "Release GPIO %d\n", gpio_id);

		gpio_svc.svc_id = AMBSVC_GPIO_FREE;
		gpio_svc.gpio = GPIO(gpio_id);
		ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
}

static void write_gpio(int gpio_id, int gpio_val)
{
	//printk(KERN_DEBUG "write GPIO %d, value =%d \n", gpio_id, gpio_val);

	gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
	gpio_svc.gpio = GPIO(gpio_id);
	gpio_svc.value = gpio_val;
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
}
/*
static int read_gpio(int gpio_id)
{
	int value = 0;

	gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
	gpio_svc.gpio = GPIO(gpio_id);
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, &value);

  	//printk(KERN_DEBUG "read GPIO %d, value =%d \n", gpio_id,  value);
 	return value;
}
*/
/* move one step,  direction 1:  move forward,  -1:backward   all other values: invalid */
int amba_p_iris_impl_move_one_step(amba_p_iris_controller_t * p_controller, int direction)
{
	//check the lens driver datasheet, to use the Stepper motor 2 input pins to do move on direction.
	int logic_bits;
	int target_bits = 0xFF;

	logic_bits = p_controller->cfg.gpio_val & 0x3;

	if (direction > 0) {

		switch (logic_bits)
		{
			case 0x2: //IN1 = 0 , IN2 = 1
				target_bits = 0x3; //IN1 = 1 , IN2 = 1
				break;
			case 0x3:
				target_bits = 0x1;
				break;
			case 0x1:
				target_bits = 0x0;
				break;
			case 0x0:
				target_bits = 0x2;
				break;
			default:
				printk("wrong bits 0x%x \n", logic_bits);
				break;
		}
	} else {
		switch (logic_bits)
		{
			case 0x2://IN1 = 0 , IN2 = 1
				target_bits = 0x0;//IN1 = 0 , IN2 = 0
				break;
			case 0x0:
				target_bits = 0x1;
				break;
			case 0x1:
				target_bits = 0x3;
				break;
			case 0x3:
				target_bits = 0x2;
				break;
			default:
				printk("wrong bits 0x%x \n", logic_bits);
				break;
		}

	}

	//log for debugging
	//printk(KERN_DEBUG "DIR %d, CURRENT 0x%x, TARGET 0x%x\n", direction, logic_bits, target_bits);

	//In stepper motor control, it's not possible for both bits changed

	//update P_IRIS_GPIO_1
	if ((target_bits & 1) != (p_controller->cfg.gpio_val & 1)) {
		write_gpio(p_controller->cfg.gpio_id[0], target_bits & 1);
	}

	//update P_IRIS_GPIO_2
	if ((target_bits & 2) != (p_controller->cfg.gpio_val & 2)) {
		write_gpio(p_controller->cfg.gpio_id[1], (target_bits >> 1) & 1);
	}

	//update gpio value cache
	p_controller->cfg.gpio_val = target_bits;

	return 0;
}


/* config the p-iris motor power */
void amba_p_iris_impl_config(amba_p_iris_controller_t * p_controller, int enable)
{
	write_gpio(p_controller->cfg.gpio_id[2], enable);

	if(enable) {
		msleep(10);
		p_controller->state = P_IRIS_WORK_STATE_READY;
	}
	else {
		p_controller->state = P_IRIS_WORK_STATE_PREPARING;
	}
}


/* low level driver to change iris */
int	amba_p_iris_impl_init(amba_p_iris_controller_t * p_controller)
{

        int i;
        //enable lens control chip

        for (i=0; i< P_IRIS_CONTROLLER_MAX_GPIO_NUM; i++)
     {
         if(request_gpio(p_controller->cfg.gpio_id[i]) < 0)
		 	return -1;
     }

        write_gpio(p_controller->cfg.gpio_id[2], 0);    //set enable = low
        write_gpio(p_controller->cfg.gpio_id[0], 0);    //set IN1 = Low
        write_gpio(p_controller->cfg.gpio_id[1], 0);    //set IN2 = Low

	return 0;
}


/* shut down the iris power */
int	amba_p_iris_impl_disable(amba_p_iris_controller_t * p_controller)
{
	int i;

 	write_gpio(p_controller->cfg.gpio_id[2], 0);    //disable

	for (i=0; i< P_IRIS_CONTROLLER_MAX_GPIO_NUM; i++)
		release_gpio(p_controller->cfg.gpio_id[i]);

	return 0;
}


/* try reset and init wait state */
int amba_p_iris_impl_reset(amba_p_iris_controller_t * p_controller, int steps)
{
	p_controller->steps_to_move = steps;

 	//wait for some time,
  	printk(KERN_DEBUG "P Iris reset, wait %d ms till it finishes \n", (-steps) * p_controller->cfg.timer_period );
  	wait_for_completion(&(p_controller->move_compl));

	printk(KERN_DEBUG "P-Iris reset done, gpio state 0x%x \n",  p_controller->cfg.gpio_val);

	return 0;
}


/* check the motor drive status */
int amba_p_iris_impl_check(amba_p_iris_controller_t * p_controller)
{
	int gpio_val1, gpio_val2;

	gpio_val1 = p_controller->cfg.gpio_val & 1;
	gpio_val2 = p_controller->cfg.gpio_val & 2;

	if((!gpio_val1)&&(!gpio_val2)) {
		return 0;
	} else {
		return 1;
	}
}


