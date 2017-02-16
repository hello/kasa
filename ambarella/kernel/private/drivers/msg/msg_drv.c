/*
 *  msg_drv.c
 *
 * History:
 *	2014/07/22 - [Jian Tang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/syslog.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include "msg_print.h"

extern wait_queue_head_t log_wait;

static int msg_drv_open(struct inode * inode, struct file * file)
{
	return do_drv_syslog(SYSLOG_ACTION_OPEN, NULL, 0, SYSLOG_FROM_PROC);
}

static int msg_drv_release(struct inode * inode, struct file * file)
{
	(void) do_drv_syslog(SYSLOG_ACTION_CLOSE, NULL, 0, SYSLOG_FROM_PROC);
	return 0;
}

static ssize_t msg_drv_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	if ((file->f_flags & O_NONBLOCK) &&
	    !do_drv_syslog(SYSLOG_ACTION_SIZE_UNREAD, NULL, 0, SYSLOG_FROM_PROC))
		return -EAGAIN;
	return do_drv_syslog(SYSLOG_ACTION_READ, buf, count, SYSLOG_FROM_PROC);
}

static unsigned int msg_drv_poll(struct file *file, poll_table *wait)
{
	/*poll_wait(file, &log_wait, wait);*/
	if (do_drv_syslog(SYSLOG_ACTION_SIZE_UNREAD, NULL, 0, SYSLOG_FROM_PROC))
		return POLLIN | POLLRDNORM;
	return 0;
}


static const struct file_operations proc_msg_drv_operations = {
	.read		= msg_drv_read,
	.poll		= msg_drv_poll,
	.open		= msg_drv_open,
	.release	= msg_drv_release,
	.llseek		= generic_file_llseek,
};

static int __init proc_msg_drv_init(void)
{
	proc_create("amkmsg", S_IRUSR, NULL, &proc_msg_drv_operations);
	return 0;
}
static void __exit proc_msg_drv_exit(void)
{
	remove_proc_entry("amkmsg", NULL);
}
module_init(proc_msg_drv_init);
module_exit(proc_msg_drv_exit);

