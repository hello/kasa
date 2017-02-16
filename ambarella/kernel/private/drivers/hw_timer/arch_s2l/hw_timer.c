/*
 * kernel/private/drivers/hw_timer/arch_s2l/hw_timer.c
 *
 * History:
 *	2014/04/28 - [Zhaoyang Chen] created file
 *
 * Copyright (C) 2012-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
#include <plat/debug.h>
#include <plat/ambcache.h>
#include <mach/init.h>


#define AMBARELLA_HWTIMER_NAME				"ambarella_hwtimer"
#define AMBARELLA_HWTIMER_OUTFREQ			"ambarella_hwtimer_outfreq"

#define AMBARELLA_HWTIMER_STATUS_REG		TIMER4_STATUS_REG
#define AMBARELLA_HWTIMER_RELOAD_REG		TIMER4_RELOAD_REG
#define AMBARELLA_HWTIMER_MATCH1_REG		TIMER4_MATCH1_REG
#define AMBARELLA_HWTIMER_MATCH2_REG		TIMER4_MATCH2_REG
#define AMBARELLA_HWTIMER_CTR_EN			TIMER_CTR_EN4
#define AMBARELLA_HWTIMER_CTR_OF			TIMER_CTR_OF4
#define AMBARELLA_HWTIMER_CTR_CSL			TIMER_CTR_CSL4
#define AMBARELLA_HWTIMER_CTR_MASK			0x0000000F

#define AMBARELLA_HWTIMER_IRQ				TIMER4_IRQ
#define AMBARELLA_HWTIMER_INPUT_FREQ		(clk_get_rate(clk_get(NULL, "gclk_apb")))

#define HWTIMER_OVERFLOW_SECONDS			30
#define HWTIMER_DEFAULT_OUTPUT_FREQ			90000

static u32 hwtimer_enable_flag			= 0;
static u64 hwtimer_overflow_count 		= 0;
static u64 hwtimer_init_output_value 	= 0;
static u32 hwtimer_init_overflow_value 	= 0;
static u32 hwtimer_input_freq			= 0;
static u32 hwtimer_curr_outfreq			= 0;

static spinlock_t timer_isr_lock;

static int hwtimer_reset(void);

static inline void ambarella_hwtimer_disable(void)
{
	amba_clrbitsl(TIMER_CTR_REG, AMBARELLA_HWTIMER_CTR_EN);
}

static inline void ambarella_hwtimer_enable(void)
{
	amba_setbitsl(TIMER_CTR_REG, AMBARELLA_HWTIMER_CTR_EN);
}

static inline void ambarella_hwtimer_misc(void)
{
	amba_setbitsl(TIMER_CTR_REG, AMBARELLA_HWTIMER_CTR_OF);
	amba_clrbitsl(TIMER_CTR_REG, AMBARELLA_HWTIMER_CTR_CSL);
}

static inline void ambarella_hwtimer_config(void)
{
	if (hwtimer_init_overflow_value) {
		amba_writel(AMBARELLA_HWTIMER_STATUS_REG, hwtimer_init_overflow_value);
		amba_writel(AMBARELLA_HWTIMER_RELOAD_REG, hwtimer_init_overflow_value);
	} else {
		hwtimer_init_overflow_value = hwtimer_input_freq;
		amba_writel(AMBARELLA_HWTIMER_STATUS_REG, hwtimer_init_overflow_value);
		amba_writel(AMBARELLA_HWTIMER_RELOAD_REG, hwtimer_init_overflow_value);
	}

	amba_writel(AMBARELLA_HWTIMER_MATCH1_REG, 0x0);
	amba_writel(AMBARELLA_HWTIMER_MATCH2_REG, 0x0);
	ambarella_hwtimer_misc();
}

static inline void ambarella_hwtimer_init(void)
{
	ambarella_hwtimer_disable();
	ambarella_hwtimer_config();
	ambarella_hwtimer_enable();
	hwtimer_enable_flag = 1;
}

static u32 ambarella_hwtimer_read(void)
{
	return (u32)amba_readl(AMBARELLA_HWTIMER_STATUS_REG);
}

static inline int get_gcd(int a, int b)
{
	if ((a == 0) || (b == 0)) {
		printk("wrong input for gcd \n");
		return 1;
	}
	while ((a != 0) && (b != 0)) {
		if (a > b) {
			a = a % b;
		} else {
			b = b % a;
		}
	}
	return (a == 0) ? b : a;
}

static int calc_output_ticks(u64 *out_tick)
{
	u64 total_ticks;
	u64 overflow_ticks;
	u64 overflow_count;
	u64 overflow_count0;
	u64 hwtimer_ticks;
	unsigned long flags;
	u32 curr_reg_value;
	u32 curr_reg_value0;
	int input_freq = hwtimer_input_freq;
	int output_freq = hwtimer_curr_outfreq;

	int gcd;

	curr_reg_value0 = ambarella_hwtimer_read();
	overflow_count0 = hwtimer_overflow_count;

	spin_lock_irqsave(&timer_isr_lock, flags);
	curr_reg_value = ambarella_hwtimer_read();
	overflow_count = hwtimer_overflow_count;
	spin_unlock_irqrestore(&timer_isr_lock, flags);

	// update overflow count for register value rollback case
	if ((curr_reg_value0 < curr_reg_value) &&
		(overflow_count == overflow_count0)) {
		overflow_count = overflow_count + 1;
	}

	overflow_ticks = overflow_count * hwtimer_init_overflow_value;
	hwtimer_ticks = hwtimer_init_overflow_value - curr_reg_value;
	total_ticks = overflow_ticks + hwtimer_ticks;

	// change frequency from apb clock to current output frequency
	gcd = get_gcd(output_freq, (int)input_freq);
	total_ticks = total_ticks * (output_freq / gcd) +
		input_freq / (gcd * 2);
	if (hwtimer_init_overflow_value) {
		do_div(total_ticks, input_freq / gcd);
	} else {
		printk("HWTimer: calculate output ticks failed! Can not divide zero!\n");
		return -EINVAL;
	}
	// generate output ticks
	*out_tick = total_ticks + hwtimer_init_output_value;

	return 0;
}

static ssize_t hwtimer_write_proc(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	ssize_t retval = 0;
	char buf[50];

	if (count >= 50) {
		printk("HWTimer: %s: count %d out of size!\n", __func__, (u32)count);
		retval = -ENOSPC;
		goto hwtimer_write_proc_exit;
	}
	if (count > 1) {
		if (copy_from_user(buf, buffer, count)) {
			printk("HWTimer: %s: copy_from_user fail!\n", __func__);
			retval = -EFAULT;
			goto hwtimer_write_proc_exit;
		}
		buf[count] = '\0';
		hwtimer_init_output_value = simple_strtoull(buf, NULL, 10);
	}
	spin_lock_irq(&timer_isr_lock);
	hwtimer_reset();
	spin_unlock_irq(&timer_isr_lock);

	retval = count;

hwtimer_write_proc_exit:
	return retval;
}

int get_hwtimer_output_ticks(u64 *out_tick)
{
	int ret = 0;
	if (hwtimer_enable_flag) {
		ret = calc_output_ticks(out_tick);
	} else {
		*out_tick = 0;
	}

	return ret;
}
EXPORT_SYMBOL(get_hwtimer_output_ticks);

static ssize_t hwtimer_read_proc(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
{
	int ret = 0;
	u64 final_ticks = 0;
	char str_ticks[32] = {0};

	if (size < 32) {
		printk("HWTimer: hw timer read buf size %d should be >= 32!\n", size);
        return -EINVAL;
	}

	if (hwtimer_enable_flag) {
		ret = calc_output_ticks(&final_ticks);
		ret = sprintf(str_ticks, "%lld\n", final_ticks);
	} else {
		ret = sprintf(str_ticks, "%d\n", 0);
	}

	if(copy_to_user(buf, str_ticks, ret))
		ret = -EFAULT;

	return ret;
}

static const struct file_operations hwtimer_proc_fops = {
	.owner		= THIS_MODULE,
	.read = hwtimer_read_proc,
	.write = hwtimer_write_proc,
};

static int hwtimer_create_proc(void)
{
	int err_code = 0;
	struct proc_dir_entry *hwtimer_entry;

	hwtimer_entry = proc_create(AMBARELLA_HWTIMER_NAME,	S_IRUGO | S_IWUGO,
		get_ambarella_proc_dir(), &hwtimer_proc_fops);

	if (!hwtimer_entry) {
		err_code = -EINVAL;
	}

	return err_code;
}

int get_hwtimer_output_freq(u32 *out_freq)
{
	int ret = 0;
	if (hwtimer_enable_flag) {
		*out_freq = hwtimer_curr_outfreq;
	} else {
		*out_freq = 0;
	}

	return ret;
}
EXPORT_SYMBOL(get_hwtimer_output_freq);


static ssize_t hwtimer_outfreq_read_proc(struct file *file, char __user *buf,
	size_t size, loff_t *ppos)
{
	int ret = 0;
	char str_freq[16] = {0};

	if (size < 16) {
		printk("HWTimer: out freq read buf size %d should be >= 16!\n", size);
		return -EINVAL;
	}

	if (hwtimer_enable_flag) {
		ret = sprintf(str_freq, "%u\n", hwtimer_curr_outfreq);
	} else {
		ret = sprintf(str_freq, "%u\n", 0);
	}

	if(copy_to_user(buf, str_freq, ret))
		ret = -EFAULT;

	return ret;
}

static ssize_t hwtimer_outfreq_write_proc(struct file *file,
	const char __user *buffer, size_t count, loff_t *data)
{
	ssize_t retval = 0;
	char buf[50];

	if (count >= 50) {
		printk("HWTimer: %s: count %d out of size!\n", __func__, (u32)count);
		retval = -ENOSPC;
		goto hwtimer_outfreq_write_proc_exit;
	}
	if (count > 1) {
		if (copy_from_user(buf, buffer, count)) {
			printk("HWTimer: %s: copy_from_user fail!\n", __func__);
			retval = -EFAULT;
			goto hwtimer_outfreq_write_proc_exit;
		}
		buf[count] = '\0';
		hwtimer_curr_outfreq= simple_strtoull(buf, NULL, 10);
	}

	retval = count;

hwtimer_outfreq_write_proc_exit:
	return retval;
}

static const struct file_operations hwtimer_freq_proc_fops =
{
	.owner	= THIS_MODULE,
	.read 	= hwtimer_outfreq_read_proc,
	.write 	= hwtimer_outfreq_write_proc,
};

static int hwtimer_create_outfreq_proc(void)
{
	int err_code = 0;
	struct proc_dir_entry *outfreq_entry;

	outfreq_entry = proc_create(AMBARELLA_HWTIMER_OUTFREQ, S_IRUGO | S_IWUGO,
		get_ambarella_proc_dir(), &hwtimer_freq_proc_fops);

	if (!outfreq_entry) {
		printk("HWTimer: %s: create outfreq proc failed!\n", __func__);
		err_code = -EINVAL;
	}

	return err_code;
}

static irqreturn_t hwtimer_isr(int irq, void *dev_id)
{
	spin_lock(&timer_isr_lock);
	hwtimer_overflow_count++;
	spin_unlock(&timer_isr_lock);

	return IRQ_HANDLED;
}

static void hwtimer_remove_proc(void)
{
	remove_proc_entry(AMBARELLA_HWTIMER_NAME, get_ambarella_proc_dir());
}

static void hwtimer_remove_outfreq_proc(void)
{
	remove_proc_entry(AMBARELLA_HWTIMER_OUTFREQ, get_ambarella_proc_dir());
}

int hwtimer_reset()
{
	hwtimer_overflow_count = 0;
	hwtimer_input_freq = AMBARELLA_HWTIMER_INPUT_FREQ;
	hwtimer_init_overflow_value = hwtimer_input_freq * HWTIMER_OVERFLOW_SECONDS;
	hwtimer_curr_outfreq = HWTIMER_DEFAULT_OUTPUT_FREQ;
	hwtimer_enable_flag = 0;
	ambarella_hwtimer_init();

	return 0;
}

static int __init hwtimer_init(void)
{
	int err_code = 0;
	err_code = hwtimer_create_proc();
	if (err_code) {
		printk("HWTimer: create proc file for hw timer failed!\n");
		goto hwtimer_init_err_create_proc;
	}
	err_code = hwtimer_create_outfreq_proc();
	if (err_code) {
		printk("HWTimer: create outfreq proc file for hw timer failed!\n");
		goto hwtimer_init_err_create_outfreq_proc;
	}

	hwtimer_overflow_count = 0;
	err_code = request_irq(AMBARELLA_HWTIMER_IRQ, hwtimer_isr,
					IRQF_TRIGGER_RISING, AMBARELLA_HWTIMER_NAME, NULL);
	if (err_code) {
		printk("HWTimer: request irq for hw timer failed!\n");
		goto hwtimer_init_err_request_irq;
	}

	hwtimer_input_freq = AMBARELLA_HWTIMER_INPUT_FREQ;
	hwtimer_init_overflow_value = hwtimer_input_freq * HWTIMER_OVERFLOW_SECONDS;
	hwtimer_curr_outfreq = HWTIMER_DEFAULT_OUTPUT_FREQ;
	hwtimer_enable_flag = 0;
	ambarella_hwtimer_init();
	spin_lock_init(&timer_isr_lock);

	goto hwtimer_init_err_na;

hwtimer_init_err_request_irq:
	free_irq(AMBARELLA_HWTIMER_IRQ, NULL);

hwtimer_init_err_create_outfreq_proc:
	hwtimer_remove_outfreq_proc();

hwtimer_init_err_create_proc:
	hwtimer_remove_proc();

hwtimer_init_err_na:
	return err_code;
}

static void __exit hwtimer_exit(void)
{
	if (hwtimer_enable_flag) {
		ambarella_hwtimer_disable();
		hwtimer_enable_flag = 0;
	}
	free_irq(AMBARELLA_HWTIMER_IRQ, NULL);
	hwtimer_remove_outfreq_proc();
	hwtimer_remove_proc();
}

module_init(hwtimer_init);
module_exit(hwtimer_exit);

MODULE_DESCRIPTION("Ambarella hardware timer driver");
MODULE_AUTHOR("Zhaoyang Chen <zychen@ambarella.com>");
MODULE_LICENSE("Proprietary");

