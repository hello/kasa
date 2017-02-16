/*
 * dsp_printk_timer.c
 *
 * History:
 *	2013/09/27 - [Louis Sun] created file
 *
 * Copyright (C) 2012-2018, Ambarella, Inc.
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

#include "iav_utils.h"
#include "dsplog_priv.h"

typedef struct idsp_printf_s {
	u32 seq_num;		/**< Sequence number */
	u32 dsp_core;		/**< 0 - DSP, 1 - MEMD */
	u32 format_addr;	/**< Address (offset) to find '%s' arg */
	u32 arg1;		/**< 1st var. arg */
	u32 arg2;		/**< 2nd var. arg */
	u32 arg3;		/**< 3rd var. arg */
	u32 arg4;		/**< 4th var. arg */
	u32 arg5;		/**< 5th var. arg */
} idsp_printf_t;


//DSP log buffer is usually a Fixed size ( most case it's 128KB) buffer, with fixed physical address
//used to do log data exchange between DSP and ARM.
static u8 *G_dsppf_base = NULL;
static idsp_printf_t *G_dsppf_current = NULL;
static u32 G_dsplog_buf_size = 0;
static u32 G_last_seq_num = 0;
#define DSPLOG_ENTRY_SIZE       sizeof(idsp_printf_t)

static int G_dsplog_state = 0;   //0: not started  1: started
static int G_dsplog_read_finish = 0;   //0: not finished 1: finished


//Ring buffer (Cache) is used to store the DSP log data, to cache the DSP log to a bigger buffer,
//in order to avoid data loss if user process is unable to read out the DSP log immediately
//especially when CPU is very busy.
//Because of this reason, DSP log should not use kernel thread to read dsp log buffer.
//and this program uses kernel timer to copy log data from "dsp log buffer" into "ring buffer"

typedef struct dsp_log_ringbuf_s {
	u8 * start;
	u8 * end;
	u8 * read_ptr;
	u8 * write_ptr;
	int full;
	struct completion fill_compl;
	int   num_reader;
} dsp_log_ringbuf_t;

static dsp_log_ringbuf_t G_ringbuf = {
	.start = NULL,
	.end = NULL,
	.read_ptr = NULL,
	.write_ptr = NULL,
	.full = 0,
	.num_reader = 0,
};


//increase DSP printk buffer size to 1MB to keep more data
#define DSP_PRINTK_BUF_SIZE	(1024 * 1024)
//when buffer data is beyond DUMP_THRESHOLD, it outputs
#define DSP_PRINTK_BUF_DUMP_THRESHOLD_SIZE      (128*1024)
#define DSP_LOG_TIMER_PERIOD        10


static int dsp_printk_timer(void *arg);


/* User Kernel Timer to implement periodical dsp log copy,
 * The granularity for Kernel Timer for A5s is 10ms.
 */
#include <linux/timer.h>
typedef int (* TIMER_CALLBACK) (void * timer_data);
struct timer_control_s {
	struct timer_list  timer;
	int timer_initialized;
	TIMER_CALLBACK timer_callback;
};

static struct timer_control_s G_timer_control = {
	.timer_initialized = 0,
	.timer_callback = NULL,
};

static void kernel_timer_callback(unsigned long data)
{
	//get the object pointer
	TIMER_CALLBACK cb = G_timer_control.timer_callback;

	//run user callback if installed
	if (cb) {
		//DRV_PRINT(KERN_DEBUG "user callback\n");
		cb((void *)data);
	}

	//DRV_PRINT(KERN_DEBUG " dsplog timer \n");

	//auto restart
	mod_timer(&G_timer_control.timer, jiffies + DSP_LOG_TIMER_PERIOD *HZ/1000L);
}


int dsp_printk_init_timer(void *p_user_callback, unsigned long data)
{
	if (G_timer_control.timer_initialized) {
		DRV_PRINT(KERN_DEBUG "timer already init, skip. \n");
	} else {
		G_timer_control.timer_callback = (TIMER_CALLBACK)p_user_callback ;
		G_timer_control.timer.expires = jiffies + DSP_LOG_TIMER_PERIOD *HZ/1000L;
		G_timer_control.timer.data = (unsigned long) data;
		G_timer_control.timer.function = &kernel_timer_callback;
		init_timer(&G_timer_control.timer);
		G_timer_control.timer_initialized = 1;
		add_timer(&G_timer_control.timer);
	}
	return 0;
}


int dsp_printk_deinit_timer(void)
{
	 //cancel timer
	if (G_timer_control.timer_initialized) {
		del_timer_sync(&G_timer_control.timer);
		//clean data
		G_timer_control.timer_callback = NULL;
		//deinit
		G_timer_control.timer_initialized = 0;
	}
	return 0;
}



//buffer is user allocated mem buffer,  size is max read bytes.
//return value of dsp_read_log is actual output.
//actual output should be > 0 and <=  MIN ( ring buffer size,  max_size of user buffer)
int dsplog_read(char __user *buffer, size_t max_size)
{
	u8 * cur_write_ptr;
	u8 * cur_read_ptr;
	int readout_size = 0;
	int cur_log_size;

	//check size, must be multiple of DSPLOG_ENTRY_SIZE, and size >  128KB
	if ((max_size % DSPLOG_ENTRY_SIZE) || (max_size < DSP_PRINTK_BUF_SIZE)) {
		DRV_PRINT(KERN_DEBUG "dsp_printk: please read dsp log with a buffer over %d size  and size is multiple of 32\n", DSP_PRINTK_BUF_SIZE);
		return -EINVAL;
	}
	//user buffer is big, can always flush simply to user space by one syscall "read"

	if (G_dsplog_read_finish)  {
		DRV_PRINT(KERN_DEBUG "dsp_printk: log has stopped and read finished , no more logs \n");
		return 0;
	}
	G_ringbuf.num_reader++;
	//unlock before waiting
	dsplog_unlock();
	wait_for_completion(&G_ringbuf.fill_compl);
	dsplog_lock();

	cur_write_ptr = G_ringbuf.write_ptr;
	cur_read_ptr = G_ringbuf.read_ptr;

	cur_log_size = cur_write_ptr - cur_read_ptr;
	if (cur_log_size < 0) {
		cur_log_size  += DSP_PRINTK_BUF_SIZE;
	}

	//read out all
	if (cur_write_ptr < cur_read_ptr) {
		//copy 2 segments
		memcpy(buffer,  cur_read_ptr,  G_ringbuf.end - cur_read_ptr);
		readout_size += G_ringbuf.end - cur_read_ptr;
		memcpy(buffer + readout_size,  G_ringbuf.start,   cur_write_ptr - G_ringbuf.start);
		readout_size += (cur_write_ptr - G_ringbuf.start);
	} else if (cur_write_ptr >cur_read_ptr) {
		//copy 1 segment
		memcpy(buffer, cur_read_ptr, cur_write_ptr - cur_read_ptr);
		readout_size += (cur_write_ptr - cur_read_ptr);
	}
	//read out all, so change the read ptr position, note that G_ringbuf.write_ptr may still
	//move ahead during readout, so here, we only set G_ringbuf.read_ptr to saved write ptr position
	G_ringbuf.read_ptr = cur_write_ptr;
	G_ringbuf.full = 0;

	if (cur_log_size < DSP_PRINTK_BUF_DUMP_THRESHOLD_SIZE) {
		DRV_PRINT(KERN_DEBUG "dsp_printk: log may have stopped. \n");
		if (G_dsplog_state == 0)
			G_dsplog_read_finish = 1;
	}

	if (G_dsplog_read_finish) {
		DRV_PRINT(KERN_DEBUG "last flush data size is %d \n", readout_size);
	}

	return readout_size;
}

static int init_buffer(void)
{
	if ((G_ringbuf.start = kzalloc(DSP_PRINTK_BUF_SIZE, GFP_KERNEL)) == NULL) {
		DRV_PRINT(KERN_DEBUG "init DSP printk buffer error, insufficient kernel mem\n");
		return -ENOMEM;
	}

	G_ringbuf.read_ptr = G_ringbuf.start;
	G_ringbuf.write_ptr = G_ringbuf.start;
	G_ringbuf.end = G_ringbuf.start + DSP_PRINTK_BUF_SIZE;
	init_completion(&G_ringbuf.fill_compl);

	return 0;
}

static inline void dsp_log_add_to_ring_buffer(void)
{
	int cur_log_size;

	//when ring buffer full, discard new record
	if (likely(!G_ringbuf.full)) {
		memcpy(G_ringbuf.write_ptr, G_dsppf_current, DSPLOG_ENTRY_SIZE);
		//move ring buffer write pointer and handle wrap around
		G_ringbuf.write_ptr += DSPLOG_ENTRY_SIZE;
		if (G_ringbuf.write_ptr >= G_ringbuf.end) {
			G_ringbuf.write_ptr = G_ringbuf.start;
		}

		//if ring buffer is full  (write ptr will NEVER catch read ptr)
		if (unlikely(((G_ringbuf.write_ptr == G_ringbuf.end -DSPLOG_ENTRY_SIZE) &&
			(G_ringbuf.read_ptr == G_ringbuf.start)) ||
			(G_ringbuf.write_ptr + DSPLOG_ENTRY_SIZE == G_ringbuf.read_ptr))) {
			DRV_PRINT(KERN_DEBUG"dsp_printk: SLOW start 0x%p, end0x%p,  write0x%p, read 0x%p\n",
				G_ringbuf.start,  G_ringbuf.end,  G_ringbuf.write_ptr, G_ringbuf.read_ptr);
			G_ringbuf.full = 1;
		}
	}

	if (G_ringbuf.num_reader) {
		if (!G_ringbuf.full) {
			cur_log_size = G_ringbuf.write_ptr  - G_ringbuf.read_ptr;
			if (cur_log_size < 0)
				cur_log_size += DSP_PRINTK_BUF_SIZE;
			if (cur_log_size > DSP_PRINTK_BUF_DUMP_THRESHOLD_SIZE) {
				G_ringbuf.num_reader--;
				complete(&G_ringbuf.fill_compl);
			}
		} else {
			G_ringbuf.num_reader--;
			complete(&G_ringbuf.fill_compl);
		}
	}
}


static int dsp_printk_timer(void *arg)
{
	static int wrap=0;  //sequence number wrap around

	while (1) {
		//invalidate DSP debug buffer before reading.
		invalidate_d_cache(G_dsppf_current, DSPLOG_ENTRY_SIZE);
		if ((G_dsppf_current->seq_num < G_last_seq_num) && (G_last_seq_num != -1)) {
			//DRV_PRINT(KERN_DEBUG "dsp_printk:  current seq num %d smaller than last %d \n",  G_dsppf_current->seq_num, G_last_seq_num);
			break;
		}
		if (G_last_seq_num == -1) {
			wrap=1;
		}
		G_last_seq_num = G_dsppf_current->seq_num;
		if ((G_last_seq_num == 0) && (wrap!=1)) {
			//DRV_PRINT(KERN_DEBUG "dsp_printk:  Zero sequence num\n");
			break;
		}
		dsp_log_add_to_ring_buffer();
		//advance read pointer and also handle wrap around
		G_dsppf_current++;
		if ((u8*)G_dsppf_current >= G_dsppf_base + G_dsplog_buf_size)
			G_dsppf_current = (idsp_printf_t *)G_dsppf_base;
	}

	//ENQUEUE  the dsp buffer into print buffer
	return 0;
}


//platform independant DSP printk init
int dsplog_init()
{
        //call DSP driver's function to init the dsplog
        dsp_init_logbuf(&G_dsppf_base, &G_dsplog_buf_size);
        G_dsppf_current = (idsp_printf_t *)G_dsppf_base;
        memset(G_dsppf_base, 0, G_dsplog_buf_size);
        init_buffer();
        DRV_PRINT(KERN_DEBUG "dsplog_printk: DSP log by Timer New\n");
        return 0;
}


int dsplog_deinit(amba_dsplog_controller_t * controller)
{
	DRV_PRINT(KERN_DEBUG "dsplog:  deinit timer.\n");
	dsp_printk_deinit_timer();
	//remove buffer
	DRV_PRINT(KERN_DEBUG "dsplog:  deinit ring buffer.\n");

	if (G_ringbuf.start) {
		kfree(G_ringbuf.start);
		G_ringbuf.start = NULL;
	}

	dsp_deinit_logbuf(G_dsppf_base, G_dsplog_buf_size);

	return 0;
}


int dsplog_start_cap()
{
	G_ringbuf.read_ptr = G_ringbuf.write_ptr;
	G_ringbuf.full = 0;
	G_ringbuf.num_reader = 0;
	//init and start timer
	dsp_printk_init_timer(dsp_printk_timer,  0);   //make the timer to issue for each HZ
	G_dsplog_state = 1; //started
	G_dsplog_read_finish = 0;  //clear finish state
	return 0;
}


int dsplog_stop_cap()
{
	//stop timer
	dsp_printk_deinit_timer();
	//flush data
	DRV_PRINT(KERN_DEBUG "dsplog_stop_cap flushes ring buf \n");
	G_ringbuf.num_reader--;
	G_dsplog_state = 0; //stopped
	complete(&G_ringbuf.fill_compl);
	return 0;
}

int dsplog_set_level(int level)
{
	return 0;
}


int dsplog_get_level(int * level)
{
	return 0;
}

int dsplog_parse(int arg)
{
	return 0;
}


