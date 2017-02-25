/*
 * kernel/private/drivers/lens/evetar_mz128bp2810icr/mz128bp2810icr.c
 *
 * History:
 *    2014/10/10 - [Peter Jiao]
 *
 *	Note that this driver will use below S2L(M) hardware resource, should avoid conflict before insmod it.
 *	GPIO: (98)(103)(97)(104)(108)(100)(105)(99)(106)(107)(96)(101)(95)(102)(93)
 *	HW_Timer: timer1, timer2 and timer3.
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
#include <linux/ctype.h>
#include <asm/dma.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <mach/hardware.h>
#include <plat/fb.h>
#include <plat/ambsyncproc.h>
#include <plat/ambasyncproc.h>
#include <plat/ambevent.h>
#include <plat/ambcache.h>
#include <plat/iav_helper.h>
#include <mach/init.h>

#include <amba_lens_interface.h>
#include "../amba_lens.h"
#include "mz128bp2810icr.h"


#define F_CK (98)
#define F_EN (104)
#define F_RE (97)
#define F_CW (103)
#define F_MO (108)
#define Z_CK (100)
#define Z_EN (105)
#define Z_RE (99)
#define Z_CW (106)
#define Z_MO (107)
#define P_CK (96)
#define P_EN (101)
#define P_RE (95)
#define P_CW (102)
#define IRCUT (93)

#define ZOOM_TIMER_STATUS_REG		TIMER1_STATUS_REG
#define ZOOM_TIMER_RELOAD_REG		TIMER1_RELOAD_REG
#define ZOOM_TIMER_MATCH1_REG		TIMER1_MATCH1_REG
#define ZOOM_TIMER_MATCH2_REG		TIMER1_MATCH2_REG
#define ZOOM_TIMER_CTR_EN			TIMER_CTR_EN1
#define ZOOM_TIMER_CTR_OF			TIMER_CTR_OF1
#define ZOOM_TIMER_CTR_CSL			TIMER_CTR_CSL1
#define ZOOM_TIMER_IRQ				TIMER1_IRQ

#define FOCUS_TIMER_STATUS_REG		TIMER2_STATUS_REG
#define FOCUS_TIMER_RELOAD_REG		TIMER2_RELOAD_REG
#define FOCUS_TIMER_MATCH1_REG		TIMER2_MATCH1_REG
#define FOCUS_TIMER_MATCH2_REG		TIMER2_MATCH2_REG
#define FOCUS_TIMER_CTR_EN			TIMER_CTR_EN2
#define FOCUS_TIMER_CTR_OF			TIMER_CTR_OF2
#define FOCUS_TIMER_CTR_CSL			TIMER_CTR_CSL2
#define FOCUS_TIMER_IRQ				TIMER2_IRQ

#define PIRIS_TIMER_STATUS_REG		TIMER3_STATUS_REG
#define PIRIS_TIMER_RELOAD_REG		TIMER3_RELOAD_REG
#define PIRIS_TIMER_MATCH1_REG		TIMER3_MATCH1_REG
#define PIRIS_TIMER_MATCH2_REG		TIMER3_MATCH2_REG
#define PIRIS_TIMER_CTR_EN			TIMER_CTR_EN3
#define PIRIS_TIMER_CTR_OF			TIMER_CTR_OF3
#define PIRIS_TIMER_CTR_CSL			TIMER_CTR_CSL3
#define PIRIS_TIMER_IRQ				TIMER3_IRQ

DEFINE_MUTEX(mz128bp2810icr_mutex);

static spinlock_t zoom_lock;
static spinlock_t focus_lock;
static spinlock_t piris_lock;

static const u32 GPIO_BASE[GPIO_INSTANCES] =
{
	0xe8009000,
	0xe800a000,
	0xe800e000,
	0xe8010000,
};

static u8 zoom_cw = 0;
static u8 focus_cw = 0;
static u8 piris_cw = 0;
static u8 zoom_ck = 0;
static u8 focus_ck = 0;
static u8 piris_ck = 0;
static u8 zoom_en = 0;
static u8 piris_en = 0;
static u8 focus_en = 0;
static u8 tdn_status = 0;
static u8 piris_status = 0;
static u8 zoom_status = 0;
static u8 focus_status = 0;

static u32 zoom_shift_count = 0;
static u32 focus_shift_count = 0;
static u32 piris_shift_count = 0;
static u32 zoom_const_count = 0;
static u32 focus_const_count = 0;
static u32 piris_const_count = 0;
static u32 timer_input_freq = 0;
static u32 zfp_timer_match = 0;

static s16 zoom_step_move = 0;
static s16 zoom_step_init = 0;
static s16 focus_step_move = 0;
static s16 focus_step_init = 0;
static s16 piris_step_move = 0;
static s16 piris_step_init = 0;

static struct ambsvc_gpio gpio_svc;

/* Note that it's only used for this module */
static void amba_lens_gpio_direction(int pin, int input)
{
	void __iomem *regbase;
	u32 bank, offset, mask;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	regbase = (void __iomem *)GPIO_BASE[bank];
	mask = (0x1 << offset);

	amba_clrbitsl(regbase + GPIO_AFSEL_OFFSET, mask);
	if (input)
		amba_clrbitsl(regbase + GPIO_DIR_OFFSET, mask);
	else
		amba_setbitsl(regbase + GPIO_DIR_OFFSET, mask);
}

/* Note that it's only used for this module */
static void amba_lens_gpio_set(int pin, int value)
{
	void __iomem *regbase;
	u32 bank, offset, mask;

	bank = PINID_TO_BANK(pin);
	offset = PINID_TO_OFFSET(pin);
	regbase = (void __iomem *)GPIO_BASE[bank];
	mask = (0x1 << offset);

	amba_writel(regbase + GPIO_MASK_OFFSET, mask);
	if (value == GPIO_LOW)
		mask = 0;
	amba_writel(regbase + GPIO_DATA_OFFSET, mask);
}

static int request_gpio(int gpio_id, int input)
{
	printk(KERN_DEBUG "Request GPIO %d\n", gpio_id);

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = GPIO(gpio_id);
	if(ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL))
		return -1;

	amba_lens_gpio_direction(GPIO(gpio_id), input);

	return 0;
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
#if 1
	amba_lens_gpio_set(GPIO(gpio_id), gpio_val);
#else
	gpio_svc.svc_id = AMBSVC_GPIO_OUTPUT;
	gpio_svc.gpio = GPIO(gpio_id);
	gpio_svc.value = gpio_val;
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
#endif
}

static int read_gpio(int gpio_id)
{
	int value = 0;

	gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
	gpio_svc.gpio = GPIO(gpio_id);
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, &value);

  	//printk(KERN_DEBUG "read GPIO %d, value =%d \n", gpio_id,  value);
 	return value;
}

static int init_zfp_gpio(void)
{
	int ret = 0;

	ret |= request_gpio(F_CK, 0);
	ret |= request_gpio(F_EN, 0);
	ret |= request_gpio(F_RE, 0);
	ret |= request_gpio(F_CW, 0);
	ret |= request_gpio(F_MO, 1);
	ret |= request_gpio(Z_CK, 0);
	ret |= request_gpio(Z_EN, 0);
	ret |= request_gpio(Z_RE, 0);
	ret |= request_gpio(Z_CW, 0);
	ret |= request_gpio(Z_MO, 1);
	ret |= request_gpio(P_CK, 0);
	ret |= request_gpio(P_EN, 0);
	ret |= request_gpio(P_RE, 0);
	ret |= request_gpio(P_CW, 0);
	ret |= request_gpio(IRCUT, 0);

	if(ret<0)
		goto ERROR_REQUEST_GPIO;

	read_gpio(F_MO); //only init as input
	read_gpio(Z_MO); //only init as input
	write_gpio(F_CK, 0);
	write_gpio(F_EN, 0);
	write_gpio(F_RE, 0);
	write_gpio(F_CW, 0);
	write_gpio(Z_CK, 0);
	write_gpio(Z_EN, 0);
	write_gpio(Z_RE, 0);
	write_gpio(Z_CW, 0);
	write_gpio(P_CK, 0);
	write_gpio(P_EN, 0);
	write_gpio(P_RE, 0);
	write_gpio(P_CW, 0);
	write_gpio(IRCUT, 0);

	return 0;

ERROR_REQUEST_GPIO:
	release_gpio(F_CK);
	release_gpio(F_EN);
	release_gpio(F_RE);
	release_gpio(F_CW);
	release_gpio(F_MO);
	release_gpio(Z_CK);
	release_gpio(Z_EN);
	release_gpio(Z_RE);
	release_gpio(Z_CW);
	release_gpio(Z_MO);
	release_gpio(P_CK);
	release_gpio(P_EN);
	release_gpio(P_RE);
	release_gpio(P_CW);
	release_gpio(IRCUT);

	return -1;
}

static void deinit_zfp_gpio(void)
{
	write_gpio(F_CK, 0);
	write_gpio(F_EN, 0);
	write_gpio(F_RE, 0);
	write_gpio(F_CW, 0);
	write_gpio(Z_CK, 0);
	write_gpio(Z_EN, 0);
	write_gpio(Z_RE, 0);
	write_gpio(Z_CW, 0);
	write_gpio(P_CK, 0);
	write_gpio(P_EN, 0);
	write_gpio(P_RE, 0);
	write_gpio(P_CW, 0);
	write_gpio(IRCUT, 0);

	release_gpio(F_CK);
	release_gpio(F_EN);
	release_gpio(F_RE);
	release_gpio(F_CW);
	release_gpio(F_MO);
	release_gpio(Z_CK);
	release_gpio(Z_EN);
	release_gpio(Z_RE);
	release_gpio(Z_CW);
	release_gpio(Z_MO);
	release_gpio(P_CK);
	release_gpio(P_EN);
	release_gpio(P_RE);
	release_gpio(P_CW);
	release_gpio(IRCUT);
}

static void zoom_timer_disable(void)
{
	amba_clrbitsl(TIMER_CTR_REG, ZOOM_TIMER_CTR_EN);
}

static void zoom_timer_enable(void)
{
	amba_setbitsl(TIMER_CTR_REG, ZOOM_TIMER_CTR_EN);
}

static void focus_timer_disable(void)
{
	amba_clrbitsl(TIMER_CTR_REG, FOCUS_TIMER_CTR_EN);
}

static void focus_timer_enable(void)
{
	amba_setbitsl(TIMER_CTR_REG, FOCUS_TIMER_CTR_EN);
}

static void piris_timer_disable(void)
{
	amba_clrbitsl(TIMER_CTR_REG, PIRIS_TIMER_CTR_EN);
}

static void piris_timer_enable(void)
{
	amba_setbitsl(TIMER_CTR_REG, PIRIS_TIMER_CTR_EN);
}

static void zoom_timer_config(void)
{
	amba_writel(ZOOM_TIMER_RELOAD_REG, 0x0);

	amba_writel(ZOOM_TIMER_MATCH1_REG, zfp_timer_match);
	amba_writel(ZOOM_TIMER_MATCH2_REG, 0x1);

	amba_writel(ZOOM_TIMER_STATUS_REG, 0x0);

	amba_setbitsl(TIMER_CTR_REG, ZOOM_TIMER_CTR_OF);
	amba_clrbitsl(TIMER_CTR_REG, ZOOM_TIMER_CTR_CSL);
}

static void focus_timer_config(void)
{
	amba_writel(FOCUS_TIMER_RELOAD_REG, 0x0);

	amba_writel(FOCUS_TIMER_MATCH1_REG, zfp_timer_match);
	amba_writel(FOCUS_TIMER_MATCH2_REG, 0x1);

	amba_writel(FOCUS_TIMER_STATUS_REG, 0x0);

	amba_setbitsl(TIMER_CTR_REG, FOCUS_TIMER_CTR_OF);
	amba_clrbitsl(TIMER_CTR_REG, FOCUS_TIMER_CTR_CSL);
}

static void piris_timer_config(void)
{
	amba_writel(PIRIS_TIMER_RELOAD_REG, 0x0);

	amba_writel(PIRIS_TIMER_MATCH1_REG, zfp_timer_match);
	amba_writel(PIRIS_TIMER_MATCH2_REG, 0x1);

	amba_writel(PIRIS_TIMER_STATUS_REG, 0x0);

	amba_setbitsl(TIMER_CTR_REG, PIRIS_TIMER_CTR_OF);
	amba_clrbitsl(TIMER_CTR_REG, PIRIS_TIMER_CTR_CSL);
}

static void zoom_motor_reset(void)
{
	zoom_en = 1;
	write_gpio(Z_EN, zoom_en);
	msleep(10);
	write_gpio(Z_RE, 0);
	zoom_cw = 1;
	write_gpio(Z_CW, zoom_cw);
	zoom_ck = 0;
	write_gpio(Z_CK, zoom_ck);
	msleep(10);
	write_gpio(Z_RE, 1);
	msleep(10);
	zoom_en = 0;
	write_gpio(Z_EN, zoom_en);
}

static void zoom_motor_release(void)
{
	zoom_en = 0;
	write_gpio(Z_EN, zoom_en);
}

static void focus_motor_reset(void)
{
	focus_en = 1;
	write_gpio(F_EN, focus_en);
	msleep(10);
	write_gpio(F_RE, 0);
	focus_cw = 0;
	write_gpio(F_CW, focus_cw);
	focus_ck = 0;
	write_gpio(F_CK, focus_ck);
	msleep(10);
	write_gpio(F_RE, 1);
	msleep(10);
	focus_en = 0;
	write_gpio(F_EN, focus_en);
}

static void focus_motor_release(void)
{
	focus_en = 0;
	write_gpio(F_EN, focus_en);
}

static void piris_motor_reset(void)
{
	piris_en = 1;
	write_gpio(P_EN, piris_en);
	msleep(10);
	write_gpio(P_RE, 0);
	piris_cw = 1;
	write_gpio(P_CW, piris_cw);
	piris_ck = 0;
	write_gpio(P_CK, piris_ck);
	msleep(10);
	write_gpio(P_RE, 1);
	msleep(10);
	piris_en = 0;
	write_gpio(P_EN, piris_en);
}

static void piris_motor_release(void)
{
	piris_en = 0;
	write_gpio(P_EN, piris_en);
}

static void zoom_motor_move(void)
{
	if(!zoom_step_move)
		return;

   	zoom_status = 1;
	zoom_step_init = zoom_step_move;

	if(zoom_step_move>0) {
		zoom_cw = 0;
		write_gpio(Z_CW, zoom_cw);
	} else {
		zoom_cw = 1;
		write_gpio(Z_CW, zoom_cw);
	}

	zoom_ck = 1;
	write_gpio(Z_CK, zoom_ck);

	amba_writel(ZOOM_TIMER_STATUS_REG, zoom_shift_count);
	zoom_timer_enable();
}

static void focus_motor_move(void)
{
	if(!focus_step_move)
		return;

   	focus_status = 1;
	focus_step_init = focus_step_move;

	if(focus_step_move>0) {
		focus_cw = 1;
		write_gpio(F_CW, focus_cw);
	} else {
		focus_cw = 0;
		write_gpio(F_CW, focus_cw);
	}

	focus_ck = 1;
	write_gpio(F_CK, focus_ck);

	amba_writel(FOCUS_TIMER_STATUS_REG, focus_shift_count);
	focus_timer_enable();
}

static void piris_motor_move(void)
{
	if(!piris_step_move)
		return;

   	piris_status = 1;
	piris_step_init = piris_step_move;

	if(piris_step_move>0) {
		piris_cw = 0;
		write_gpio(P_CW, piris_cw);
	} else {
		piris_cw = 1;
		write_gpio(P_CW, piris_cw);
	}

	piris_ck = 1;
	write_gpio(P_CK, piris_ck);

	amba_writel(PIRIS_TIMER_STATUS_REG, piris_shift_count);
	piris_timer_enable();
}

static irqreturn_t zoom_isr(int irq, void *dev_id)
{
	spin_lock(&zoom_lock);

	zoom_timer_disable();

	if(!zoom_cw) {
		if(!zoom_ck)
			zoom_step_move--;
		if(zoom_step_move<=FULL_SHIFT_STEP)
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_shift_count);
		else if(zoom_step_init-zoom_step_move>=FULL_SHIFT_STEP)
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_const_count);
		else
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_shift_count);
	} else {
		if(!zoom_ck)
			zoom_step_move++;
       	if(zoom_step_move>=-FULL_SHIFT_STEP)
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_shift_count);
       	else if(zoom_step_move-zoom_step_init>=FULL_SHIFT_STEP)
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_const_count);
      	else
			amba_writel(ZOOM_TIMER_STATUS_REG, zoom_shift_count);
	}

  	if(!zoom_step_move) {
		zoom_status = 0;
   	} else {
		zoom_ck = (zoom_ck) ? 0:1;
		write_gpio(Z_CK, zoom_ck);
		zoom_timer_enable();
	}

	spin_unlock(&zoom_lock);

	return IRQ_HANDLED;
}

static irqreturn_t focus_isr(int irq, void *dev_id)
{
	spin_lock(&focus_lock);

	focus_timer_disable();

   	if(focus_cw) {
		if(!focus_ck)
         	focus_step_move--;
      	if(focus_step_move<=FULL_SHIFT_STEP)
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_shift_count);
       	else if(focus_step_init-focus_step_move>=FULL_SHIFT_STEP)
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_const_count);
      	else
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_shift_count);
  	} else {
        if(!focus_ck)
            focus_step_move++;
      	if(focus_step_move>=-FULL_SHIFT_STEP)
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_shift_count);
       	else if(focus_step_move-focus_step_init>=FULL_SHIFT_STEP)
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_const_count);
      	else
			amba_writel(FOCUS_TIMER_STATUS_REG, focus_shift_count);
  	}

   	if(!focus_step_move) {
		focus_status = 0;
   	} else {
      	focus_ck = (focus_ck) ? 0:1;
      	write_gpio(F_CK, focus_ck);
		focus_timer_enable();
  	}

	spin_unlock(&focus_lock);

	return IRQ_HANDLED;
}

static irqreturn_t piris_isr(int irq, void *dev_id)
{
	spin_lock(&piris_lock);

	piris_timer_disable();

   	if(!piris_cw) {
		if(!piris_ck)
         	piris_step_move--;
      	if(piris_step_move<=FULL_SHIFT_STEP)
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_shift_count);
       	else if(piris_step_init-piris_step_move>=FULL_SHIFT_STEP)
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_const_count);
      	else
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_shift_count);
  	} else {
        if(!piris_ck)
            piris_step_move++;
      	if(piris_step_move>=-FULL_SHIFT_STEP)
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_shift_count);
       	else if(piris_step_move-piris_step_init>=FULL_SHIFT_STEP)
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_const_count);
      	else
			amba_writel(PIRIS_TIMER_STATUS_REG, piris_shift_count);
  	}

   	if(!piris_step_move) {
		piris_status = 0;
   	} else {
      	piris_ck = (piris_ck) ? 0:1;
      	write_gpio(P_CK, piris_ck);
		piris_timer_enable();
  	}

	spin_unlock(&piris_lock);

	return IRQ_HANDLED;
}

static int init_zfp_timer(void)
{
	timer_input_freq = clk_get_rate(clk_get(NULL, "gclk_apb"));
	if(!timer_input_freq) {
		printk(KERN_ERR "Get timer input clock failed!\n");
		return -1;
	}

	zfp_timer_match = timer_input_freq>>1;

	zoom_timer_disable();
	focus_timer_disable();
	piris_timer_disable();

	zoom_timer_config();
	focus_timer_config();
	piris_timer_config();

	spin_lock_init(&zoom_lock);
	spin_lock_init(&focus_lock);
	spin_lock_init(&piris_lock);

	if(request_irq(ZOOM_TIMER_IRQ, zoom_isr, IRQF_DISABLED, "zoom_isr", NULL)) {
		printk(KERN_ERR "Request irq for zoom timer failed!\n");
		goto ERROR_ZOOM_IRQ;
	}
	if(request_irq(FOCUS_TIMER_IRQ, focus_isr, IRQF_DISABLED, "focus_isr", NULL)) {
		printk(KERN_ERR "Request irq for focus timer failed!\n");
		goto ERROR_FOCUS_IRQ;
	}
	if(request_irq(PIRIS_TIMER_IRQ, piris_isr, IRQF_DISABLED, "piris_isr", NULL)) {
		printk(KERN_ERR "Request irq for piris timer failed!\n");
		goto ERROR_PIRIS_IRQ;
	}

	return 0;

ERROR_PIRIS_IRQ:
	free_irq(PIRIS_TIMER_IRQ, NULL);
ERROR_FOCUS_IRQ:
	free_irq(FOCUS_TIMER_IRQ, NULL);
ERROR_ZOOM_IRQ:
	free_irq(ZOOM_TIMER_IRQ, NULL);

	return -1;
}

static void deinit_zfp_timer(void)
{
	zoom_timer_disable();
	focus_timer_disable();
	piris_timer_disable();

	free_irq(ZOOM_TIMER_IRQ, NULL);
	free_irq(FOCUS_TIMER_IRQ, NULL);
	free_irq(PIRIS_TIMER_IRQ, NULL);
}

static long mz128bp2810icr_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	s32 retval = 0;
	u32 lens_pps = 0;
	u32 zfi_count = 0;
	s16 step_calc, step_delta = 0;

	mutex_lock(&mz128bp2810icr_mutex);

	switch (cmd) {
		case AMBA_IOC_MFZ_GET_STATUS:  /* GET_STATUS */
			*(u64 *)arg = ((u64)focus_status & MFZ_STATUS_MASK)
				+ (((u64)zoom_status & MFZ_STATUS_MASK) << MFZ_ZOOM_SHIFT)
				+ (((u64)piris_status & MFZ_STATUS_MASK) << MFZ_IRIS_SHIFT)
				+ (((u64)tdn_status & MFZ_STATUS_MASK) << MFZ_TDN_SHIFT)
				+ (((u64)focus_step_move & MFZ_STEP_MASK) << MFZ_FSTEP_SHIFT)
				+ (((u64)zoom_step_move & MFZ_STEP_MASK) << MFZ_ZSTEP_SHIFT);
			break;
		case AMBA_IOC_MFZ_SET_ENABLE: /* SET_ENABLE */
			if(arg & MFZ_CONFIG_MASK) {
				if(arg & MFZ_ENABLE_MASK) {
					focus_en = 1;
					write_gpio(F_EN, focus_en);
				} else {
					focus_en = 0;
					write_gpio(F_EN, focus_en);
				}
			}
			if((arg>>MFZ_ZOOM_SHIFT) & MFZ_CONFIG_MASK) {
				if((arg>>MFZ_ZOOM_SHIFT) & MFZ_ENABLE_MASK) {
					zoom_en = 1;
					write_gpio(Z_EN, zoom_en);
				} else {
					zoom_en = 0;
					write_gpio(Z_EN, zoom_en);
				}
			}
			if((arg>>MFZ_IRIS_SHIFT) & MFZ_CONFIG_MASK) {
				if((arg>>MFZ_IRIS_SHIFT) & MFZ_ENABLE_MASK) {
					piris_en = 1;
					write_gpio(P_EN, piris_en);
				} else {
					piris_en = 0;
					write_gpio(P_EN, piris_en);
				}
			}
			if((arg>>MFZ_TDN_SHIFT) & MFZ_CONFIG_MASK) {
				if((arg>>MFZ_TDN_SHIFT) & MFZ_ENABLE_MASK) {
					tdn_status = 1;
					write_gpio(IRCUT, tdn_status);
				} else {
					tdn_status = 0;
					write_gpio(IRCUT, tdn_status);
				}
			}
			break;
		case AMBA_IOC_MFZ_ZOOM_SET:	/* ZOOM_SET */
			if(!zoom_en) {
				printk(KERN_ERR "invalid zoom motor set\n");
				retval = -EPERM;
				break;
			}
			lens_pps = (arg>>MFZ_COUNT_SHIFT) & MFZ_COUNT_MASK;
			if(!lens_pps) {
				printk(KERN_ERR "invalid zoom set pps\n");
				retval = -EINVAL;
				break;
			}
			zfi_count = DIV_ROUND(timer_input_freq, lens_pps);
			if((arg>>MFZ_DOCMD_SHIFT) & MFZ_DOCMD_MASK)
				zoom_const_count = zfi_count + zfp_timer_match;
			else
				zoom_const_count = (zfi_count>>1) + zfp_timer_match;
			zoom_shift_count = zfi_count + zfp_timer_match;
			if(zoom_status)
				break;
			zoom_step_move = (s16)(arg & MFZ_STEP_MASK);
			zoom_motor_move();
			break;
		case AMBA_IOC_MFZ_FOCUS_SET:  /* FOCUS_SET */
			if(!focus_en) {
				printk(KERN_ERR "invalid focus motor set\n");
				retval = -EPERM;
				break;
			}
			lens_pps = (arg>>MFZ_COUNT_SHIFT) & MFZ_COUNT_MASK;
			if(!lens_pps) {
				printk(KERN_ERR "invalid focus set pps\n");
				retval = -EINVAL;
				break;
			}
			zfi_count = DIV_ROUND(timer_input_freq, lens_pps);
			if((arg>>MFZ_DOCMD_SHIFT) & MFZ_DOCMD_MASK)
				focus_const_count = zfi_count + zfp_timer_match;
			else
				focus_const_count = (zfi_count>>1) + zfp_timer_match;
			focus_shift_count = zfi_count + zfp_timer_match;
			if(focus_status)
				break;
			focus_step_move = (s16)(arg & MFZ_STEP_MASK);
			focus_motor_move();
			break;
		case AMBA_IOC_MFZ_IRIS_SET:	/* IRIS_SET */
			if(!piris_en) {
				printk(KERN_ERR "invalid iris motor set\n");
				retval = -EPERM;
				break;
			}
			lens_pps = (arg>>MFZ_COUNT_SHIFT) & MFZ_COUNT_MASK;
			if(!lens_pps) {
				printk(KERN_ERR "invalid iris set pps\n");
				retval = -EINVAL;
				break;
			}
			zfi_count = DIV_ROUND(timer_input_freq, lens_pps);
			if((arg>>MFZ_DOCMD_SHIFT) & MFZ_DOCMD_MASK)
				piris_const_count = zfi_count + zfp_timer_match;
			else
				piris_const_count = (zfi_count>>1) + zfp_timer_match;
			piris_shift_count = zfi_count + zfp_timer_match;
			if(piris_status)
				break;
			piris_step_move = (s16)(arg & MFZ_STEP_MASK);
			piris_motor_move();
			break;
		case AMBA_IOC_MFZ_ZOOM_STOP:	/* ZOOM_STOP */
			if(!zoom_en) {
				printk(KERN_ERR "invalid zoom motor stop\n");
				retval = -EPERM;
				break;
			}
			lens_pps = (*(u32 *)arg>>MFZ_COUNT_SHIFT) & MFZ_COUNT_MASK;
			if(!lens_pps) {
				printk(KERN_ERR "invalid focus set pps\n");
				retval = -EINVAL;
				break;
			}
			step_delta = *(u32 *)arg & MFZ_STEP_MASK;
			if(zoom_const_count >= DIV_ROUND(timer_input_freq, lens_pps)+zfp_timer_match)
				step_calc = 1;
			else
				step_calc = FULL_SHIFT_STEP;
			spin_lock(&zoom_lock);
			if(step_delta==(s16)MFZ_EXCEPT_FLAG) {
				if(zoom_step_move>step_calc) {
					*(u32 *)arg = (u32)(zoom_step_move - step_calc);
					zoom_step_move = step_calc;
				}
				else if(zoom_step_move<-step_calc) {
					*(u32 *)arg = (u32)(zoom_step_move + step_calc);
					zoom_step_move = -step_calc;
				}
				else
					*(u32 *)arg = 0;
			} else {
				if(step_delta==0 || (step_delta>0 && zoom_step_move-step_delta>=step_calc)
					|| (step_delta<0 && zoom_step_move-step_delta<=-step_calc))
					zoom_step_move -= step_delta;
				else
					retval = -EINVAL;
			}
			spin_unlock(&zoom_lock);
			break;
		case AMBA_IOC_MFZ_FOCUS_STOP:  /* FOCUS_STOP */
			if(!focus_en) {
				printk(KERN_ERR "invalid focus motor stop\n");
				retval = -EPERM;
				break;
			}
			lens_pps = (*(u32 *)arg>>MFZ_COUNT_SHIFT) & MFZ_COUNT_MASK;
			if(!lens_pps) {
				printk(KERN_ERR "invalid focus set pps\n");
				retval = -EINVAL;
				break;
			}
			step_delta = *(u32 *)arg & MFZ_STEP_MASK;
			if(focus_const_count >= DIV_ROUND(timer_input_freq, lens_pps)+zfp_timer_match)
				step_calc = 1;
			else
				step_calc = FULL_SHIFT_STEP;
			spin_lock(&focus_lock);
			if(step_delta==(s16)MFZ_EXCEPT_FLAG) {
				if(focus_step_move>step_calc) {
					*(u32 *)arg = (u32)(focus_step_move - step_calc);
					focus_step_move = step_calc;
				}
				else if(focus_step_move<-step_calc) {
					*(u32 *)arg = (u32)(focus_step_move + step_calc);
					focus_step_move = -step_calc;
				}
				else
					*(u32 *)arg = 0;
			} else {
				if(step_delta==0 || (step_delta>0 && focus_step_move-step_delta>=step_calc)
					|| (step_delta<0 && focus_step_move-step_delta<=-step_calc))
					focus_step_move -= step_delta;
				else
					retval = -EINVAL;
			}
			spin_unlock(&focus_lock);
			break;
		default:
			retval = -ENOIOCTLCMD;
			break;
	}

	mutex_unlock(&mz128bp2810icr_mutex);

	return retval;
}

static atomic_t lens_reference_count = ATOMIC_INIT(0);

static int mz128bp2810icr_open(struct inode *inode, struct file *filp)
{
	if(atomic_inc_return(&lens_reference_count) != 1) {
			printk(KERN_ERR "lens: device is already opened\n");
			return -EBUSY;
	}

	zoom_motor_reset();
	focus_motor_reset();
	piris_motor_reset();

	return 0;
}

static int mz128bp2810icr_release(struct inode *inode, struct file *filp)
{
	if(!atomic_dec_and_test(&lens_reference_count)) {
		printk(KERN_ERR "Open lens time more than once, but it's impossible! \n");
		atomic_set(&lens_reference_count, 0);
		return 0;
	}

	zoom_motor_release();
	focus_motor_release();
	piris_motor_release();

	return 0;
}

static struct amba_lens_operations dev_fops = {
	.lens_dev_name = "evetar_mz128bp2810icr",
	.lens_dev_ioctl = mz128bp2810icr_ioctl,
	.lens_dev_open = mz128bp2810icr_open,
	.lens_dev_release = mz128bp2810icr_release
};

static int __init mz128bp2810icr_init(void)
{
	if(init_zfp_gpio()<0)
		return -1;

	if(init_zfp_timer()<0)
		return -1;

	if(amba_lens_register(&dev_fops)<0)
		return -1;

	return 0;
}

static void __exit mz128bp2810icr_exit(void)
{
	amba_lens_logoff();

	deinit_zfp_timer();

	deinit_zfp_gpio();
}

module_init(mz128bp2810icr_init);
module_exit(mz128bp2810icr_exit);

MODULE_DESCRIPTION("Ambarella driver for evetar lens mz128bp2810icr");
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Peter Jiao <hgjiao@ambarella.com>");
MODULE_ALIAS("lens-driver");

