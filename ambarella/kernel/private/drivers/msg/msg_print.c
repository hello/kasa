/*******************************************************************************
 * print_drv.c
 *
 * Histroy:
 *  July 23, 2014 - [Lei Hong] created file
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
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/smp.h>
#include <linux/security.h>
#include <linux/kern_levels.h>
#include <linux/syscalls.h>
#include <linux/syslog.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/version.h>

MODULE_AUTHOR("Lei Hong<lhong@ambarella.com>");
MODULE_DESCRIPTION("Ambarella print private driver");
MODULE_LICENSE("GPL");

enum drv_log_flags {
	LOG_NOCONS	= 1,	/* already flushed, do not print to console */
	LOG_NEWLINE	= 2,	/* text ended with a newline */
	LOG_PREFIX	= 4,	/* text started with a prefix */
	LOG_CONT	= 8,	/* text is a fragment of a continuation line */
};

struct drv_log {
	u64 ts_nsec;		/* timestamp in nanoseconds */
	u16 len;		/* length of entire record */
	u16 text_len;		/* length of text buffer */
	u16 dict_len;		/* length of dictionary buffer */
	u8 facility;		/* syslog facility */
	u8 flags:5;		/* internal record flags */
	u8 level:3;		/* syslog level */
};

extern u64 local_clock(void);

/* printk's without a loglevel use this.. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0)
#define DEFAULT_MESSAGE_LOGLEVEL CONFIG_MESSAGE_LOGLEVEL_DEFAULT
#else
#define DEFAULT_MESSAGE_LOGLEVEL CONFIG_DEFAULT_MESSAGE_LOGLEVEL
#endif

/*
 * The drv_logbuf_lock protects drvmsg buffer, indices, counters. It is also
 * used in interesting ways to provide interlocking in console_unlock();
 */
static DEFINE_RAW_SPINLOCK(drv_logbuf_lock);


/* the next printk record to read by syslog(READ) or /proc/drvmsg */
static u64 drv_syslog_seq;
static u32 drv_syslog_idx;
static enum drv_log_flags drv_syslog_prev;
static size_t drv_syslog_partial;

/* index and sequence number of the first record stored in the buffer */
static u64 drv_log_first_seq;
static u32 drv_log_first_idx;

/* index and sequence number of the next record to store in the buffer */
static u64 drv_log_next_seq;
static u32 drv_log_next_idx;

/* the next printk record to read after the last 'clear' command */
static u64 drv_clear_seq;
static u32 drv_clear_idx;

#define DRV_PREFIX_MAX		32
#define DRV_LOG_LINE_MAX		1024 - DRV_PREFIX_MAX

/* record buffer */
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
#define DRV_LOG_ALIGN 4
#else
#define DRV_LOG_ALIGN __alignof__(struct drv_log)
#endif
#define __DRV_LOG_BUF_LEN (1 << CONFIG_LOG_BUF_SHIFT)
static char __drv_log_buf[__DRV_LOG_BUF_LEN] __aligned(DRV_LOG_ALIGN);
static char *drv_log_buf = __drv_log_buf;
static u32 drv_log_buf_len = __DRV_LOG_BUF_LEN;

/* cpu currently holding drv_logbuf_lock */
/*static volatile unsigned int logbuf_cpu = UINT_MAX;*/

static inline int print_drv_get_level(const char *buffer)
{
	if (buffer[0] == KERN_SOH_ASCII && buffer[1]) {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'd':	/* KERN_DEFAULT */
			return buffer[1];
		}
	}
	return 0;
}

static inline const char *print_drv_skip_level(const char *buffer)
{
	if (print_drv_get_level(buffer)) {
		switch (buffer[1]) {
		case '0' ... '7':
		case 'd':	/* KERN_DEFAULT */
			return buffer + 2;
		}
	}
	return buffer;
}

/* human readable text of the record */
static char *drv_log_text(const struct drv_log *msg)
{
	return (char *)msg + sizeof(struct drv_log);
}

/* optional key/value pair dictionary attached to the record */
static char *drv_log_dict(const struct drv_log *msg)
{
	return (char *)msg + sizeof(struct drv_log) + msg->text_len;
}

/* get record by index; idx must point to valid msg */
static struct drv_log *drv_log_from_idx(u32 idx)
{
	struct drv_log *msg = (struct drv_log *)(drv_log_buf + idx);

	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer.
	 */
	if (!msg->len)
		return (struct drv_log *)drv_log_buf;
	return msg;
}

/* get next record; idx must point to valid msg */
static u32 drv_log_next(u32 idx)
{
	struct drv_log *msg = (struct drv_log *)(drv_log_buf + idx);

	/* length == 0 indicates the end of the buffer; wrap */
	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer as *this* one, and
	 * return the one after that.
	 */
	if (!msg->len) {
		msg = (struct drv_log *)drv_log_buf;
		return msg->len;
	}
	return idx + msg->len;
}

/* insert record into the buffer, discard old ones, update heads */
static void drv_log_store(int facility, int level,
		      enum drv_log_flags flags, u64 ts_nsec,
		      const char *dict, u16 dict_len,
		      const char *text, u16 text_len)
{
	struct drv_log *msg;
	u32 size, pad_len;

	/* number of '\0' padding bytes to next message */
	size = sizeof(struct drv_log) + text_len + dict_len;
	pad_len = (-size) & (DRV_LOG_ALIGN - 1);
	size += pad_len;

	while (drv_log_first_seq < drv_log_next_seq) {
		u32 free;

		if (drv_log_next_idx > drv_log_first_idx)
			free = max(drv_log_buf_len - drv_log_next_idx, drv_log_first_idx);
		else
			free = drv_log_first_idx - drv_log_next_idx;

		if (free > size + sizeof(struct drv_log))
			break;

		/* drop old messages until we have enough contiuous space */
		drv_log_first_idx = drv_log_next(drv_log_first_idx);
		drv_log_first_seq++;
	}

	if (drv_log_next_idx + size + sizeof(struct drv_log) >= drv_log_buf_len) {
		/*
		 * This message + an additional empty header does not fit
		 * at the end of the buffer. Add an empty header with len == 0
		 * to signify a wrap around.
		 */
		memset(drv_log_buf + drv_log_next_idx, 0, sizeof(struct drv_log));
		drv_log_next_idx = 0;
	}

	/* fill message */
	msg = (struct drv_log *)(drv_log_buf + drv_log_next_idx);
	memcpy(drv_log_text(msg), text, text_len);
	msg->text_len = text_len;
	memcpy(drv_log_dict(msg), dict, dict_len);
	msg->dict_len = dict_len;
	msg->facility = facility;
	msg->level = level & 7;
	msg->flags = flags & 0x1f;
	if (ts_nsec > 0)
		msg->ts_nsec = ts_nsec;
	else
		msg->ts_nsec = local_clock();
	memset(drv_log_dict(msg) + dict_len, 0, pad_len);
	msg->len = sizeof(struct drv_log) + text_len + dict_len + pad_len;

	/* insert message */
	drv_log_next_idx += msg->len;
	drv_log_next_seq++;
}

static size_t drv_print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec;

	/*if (!printk_time)
		return 0;*/

	rem_nsec = do_div(ts, 1000000000);

	if (!buf)
		return snprintf(NULL, 0, "[%5lu.000000] ", (unsigned long)ts);

	return sprintf(buf, "[%5lu.%06lu] ",
		       (unsigned long)ts, rem_nsec / 1000);
}

static size_t drv_print_prefix(const struct drv_log *msg, bool syslog, char *buf)
{
	size_t len = 0;
	unsigned int prefix = (msg->facility << 3) | msg->level;

	if (syslog) {
		if (buf) {
			len += sprintf(buf, "<%u>", prefix);
		} else {
			len += 3;
			if (prefix > 999)
				len += 3;
			else if (prefix > 99)
				len += 2;
			else if (prefix > 9)
				len++;
		}
	}

	len += drv_print_time(msg->ts_nsec, buf ? buf + len : NULL);
	return len;
}

static size_t drv_msg_print_text(const struct drv_log *msg, enum drv_log_flags prev,
			     bool syslog, char *buf, size_t size)
{
	const char *text = drv_log_text(msg);
	size_t text_size = msg->text_len;
	bool prefix = true;
	bool newline = true;
	size_t len = 0;

	if ((prev & LOG_CONT) && !(msg->flags & LOG_PREFIX))
		prefix = false;

	if (msg->flags & LOG_CONT) {
		if ((prev & LOG_CONT) && !(prev & LOG_NEWLINE))
			prefix = false;

		if (!(msg->flags & LOG_NEWLINE))
			newline = false;
	}

	do {
		const char *next = memchr(text, '\n', text_size);
		size_t text_len;

		if (next) {
			text_len = next - text;
			next++;
			text_size -= next - text;
		} else {
			text_len = text_size;
		}

		if (buf) {
			if (drv_print_prefix(msg, syslog, NULL) +
			    text_len + 1 >= size - len)
				break;

			if (prefix)
				len += drv_print_prefix(msg, syslog, buf + len);
			memcpy(buf + len, text, text_len);
			len += text_len;
			if (next || newline)
				buf[len++] = '\n';
		} else {
			/* SYSLOG_ACTION_* buffer size only calculation */
			if (prefix)
				len += drv_print_prefix(msg, syslog, NULL);
			len += text_len;
			if (next || newline)
				len++;
		}

		prefix = true;
		text = next;
	} while (text);

	return len;
}

static int drv_syslog_print(char __user *buf, int size)
{
	char *text;
	struct drv_log *msg;
	int len = 0;

	text = kmalloc(DRV_LOG_LINE_MAX + DRV_PREFIX_MAX, GFP_KERNEL);
	if (!text)
		return -ENOMEM;

	while (size > 0) {
		size_t n;
		size_t skip;

		raw_spin_lock_irq(&drv_logbuf_lock);
		if (drv_syslog_seq < drv_log_first_seq) {
			/* messages are gone, move to first one */
			drv_syslog_seq = drv_log_first_seq;
			drv_syslog_idx = drv_log_first_idx;
			drv_syslog_prev = 0;
			drv_syslog_partial = 0;
		}
		if (drv_syslog_seq == drv_log_next_seq) {
			raw_spin_unlock_irq(&drv_logbuf_lock);
			break;
		}

		skip = drv_syslog_partial;
		msg = drv_log_from_idx(drv_syslog_idx);
		n = drv_msg_print_text(msg, drv_syslog_prev, true, text,
				   DRV_LOG_LINE_MAX + DRV_PREFIX_MAX);
		if (n - drv_syslog_partial <= size) {
			/* message fits into buffer, move forward */
			drv_syslog_idx = drv_log_next(drv_syslog_idx);
			drv_syslog_seq++;
			drv_syslog_prev = msg->flags;
			n -= drv_syslog_partial;
			drv_syslog_partial = 0;
		} else if (!len){
			/* partial read(), remember position */
			n = size;
			drv_syslog_partial += n;
		} else
			n = 0;
		raw_spin_unlock_irq(&drv_logbuf_lock);

		if (!n)
			break;

		if (copy_to_user(buf, text + skip, n)) {
			if (!len)
				len = -EFAULT;
			break;
		}

		len += n;
		size -= n;
		buf += n;
	}

	kfree(text);
	return len;
}

static int drv_syslog_print_all(char __user *buf, int size, bool clear)
{
	char *text;
	int len = 0;

	text = kmalloc(DRV_LOG_LINE_MAX + DRV_PREFIX_MAX, GFP_KERNEL);
	if (!text)
		return -ENOMEM;

	raw_spin_lock_irq(&drv_logbuf_lock);
	if (buf) {
		u64 next_seq;
		u64 seq;
		u32 idx;
		enum drv_log_flags prev;

		if (drv_clear_seq < drv_log_first_seq) {
			/* messages are gone, move to first available one */
			drv_clear_seq = drv_log_first_seq;
			drv_clear_idx = drv_log_first_idx;
		}

		/*
		 * Find first record that fits, including all following records,
		 * into the user-provided buffer for this dump.
		 */
		seq = drv_clear_seq;
		idx = drv_clear_idx;
		prev = 0;
		while (seq < drv_log_next_seq) {
			struct drv_log *msg = drv_log_from_idx(idx);

			len += drv_msg_print_text(msg, prev, true, NULL, 0);
			prev = msg->flags;
			idx = drv_log_next(idx);
			seq++;
		}

		/* move first record forward until length fits into the buffer */
		seq = drv_clear_seq;
		idx = drv_clear_idx;
		prev = 0;
		while (len > size && seq < drv_log_next_seq) {
			struct drv_log *msg = drv_log_from_idx(idx);

			len -= drv_msg_print_text(msg, prev, true, NULL, 0);
			prev = msg->flags;
			idx = drv_log_next(idx);
			seq++;
		}

		/* last message fitting into this dump */
		next_seq = drv_log_next_seq;
		len = 0;
		prev = 0;
		while (len >= 0 && seq < next_seq) {
			struct drv_log *msg = drv_log_from_idx(idx);
			int textlen;

			textlen = drv_msg_print_text(msg, prev, true, text,
						 DRV_LOG_LINE_MAX + DRV_PREFIX_MAX);
			if (textlen < 0) {
				len = textlen;
				break;
			}
			idx = drv_log_next(idx);
			seq++;
			prev = msg->flags;

			raw_spin_unlock_irq(&drv_logbuf_lock);
			if (copy_to_user(buf + len, text, textlen))
				len = -EFAULT;
			else
				len += textlen;
			raw_spin_lock_irq(&drv_logbuf_lock);

			if (seq < drv_log_first_seq) {
				/* messages are gone, move to next one */
				seq = drv_log_first_seq;
				idx = drv_log_first_idx;
				prev = 0;
			}
		}
	}

	if (clear) {
		drv_clear_seq = drv_log_next_seq;
		drv_clear_idx = drv_log_next_idx;
	}
	raw_spin_unlock_irq(&drv_logbuf_lock);

	kfree(text);
	return len;
}

#ifdef CONFIG_SECURITY_DMESG_RESTRICT
static int drv_dmesg_restrict = 1;
#else
static int drv_dmesg_restrict = 0;
#endif

static int drv_syslog_action_restricted(int type)
{
	if (drv_dmesg_restrict)
		return 1;
	/* Unless restricted, we allow "read all" and "get buffer size" for everybody */
	return type != SYSLOG_ACTION_READ_ALL && type != SYSLOG_ACTION_SIZE_BUFFER;
}

static int check_drv_syslog_permissions(int type, bool from_file)
{
	/*
	 * If this is from /proc/kmsg and we've already opened it, then we've
	 * already done the capabilities checks at open time.
	 */
	if (from_file && type != SYSLOG_ACTION_OPEN)
		return 0;

	if (drv_syslog_action_restricted(type)) {
		if (capable(CAP_SYSLOG))
			return 0;
		/* For historical reasons, accept CAP_SYS_ADMIN too, with a warning */
		if (capable(CAP_SYS_ADMIN)) {
			printk_once(KERN_WARNING "%s (%d): "
				 "Attempt to access syslog with CAP_SYS_ADMIN "
				 "but no CAP_SYSLOG (deprecated).\n",
				 current->comm, task_pid_nr(current));
			return 0;
		}
		return -EPERM;
	}
	return 0;
}

int do_drv_syslog(int type, char __user *buf, int len, bool from_file)
{
	bool clear = false;
	int error;

	error = check_drv_syslog_permissions(type, from_file);
	if (error)
		goto out;

	error = security_syslog(type);
	if (error)
		return error;

	switch (type) {
	case SYSLOG_ACTION_CLOSE:	/* Close log */
		break;
	case SYSLOG_ACTION_OPEN:	/* Open log */
		break;
	case SYSLOG_ACTION_READ:	/* Read from log */
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		/*error = wait_event_interruptible(log_wait,
						 drv_syslog_seq != drv_log_next_seq);
		if (error)
			goto out;*/
		error = drv_syslog_print(buf, len);
		break;
	/* Read/clear last kernel messages */
	case SYSLOG_ACTION_READ_CLEAR:
		clear = true;
		/* FALL THRU */
	/* Read last kernel messages */
	case SYSLOG_ACTION_READ_ALL:
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		error = drv_syslog_print_all(buf, len, clear);
		break;
	/* Clear ring buffer */
	case SYSLOG_ACTION_CLEAR:
		drv_syslog_print_all(NULL, 0, true);
		break;
	/* Number of chars in the log buffer */
	case SYSLOG_ACTION_SIZE_UNREAD:
		raw_spin_lock_irq(&drv_logbuf_lock);

		if (drv_syslog_seq < drv_log_first_seq) {
			/* messages are gone, move to first one */
			drv_syslog_seq = drv_log_first_seq;
			drv_syslog_idx = drv_log_first_idx;
			drv_syslog_prev = 0;
			drv_syslog_partial = 0;
		}
		if (from_file) {
			/*
			 * Short-cut for poll(/"proc/drvmsg") which simply checks
			 * for pending data, not the size; return the count of
			 * records, not the length.
			 */
			error = drv_log_next_idx - drv_syslog_idx;
		} else {
			u64 seq = drv_syslog_seq;
			u32 idx = drv_syslog_idx;
			enum drv_log_flags prev = drv_syslog_prev;
			error = 0;
			while (seq < drv_log_next_seq) {
				struct drv_log *msg = drv_log_from_idx(idx);
				error += drv_msg_print_text(msg, prev, true, NULL, 0);
				idx = drv_log_next(idx);
				seq++;
				prev = msg->flags;
			}
			error -= drv_syslog_partial;
		}
		raw_spin_unlock_irq(&drv_logbuf_lock);
		break;
	/* Size of the log buffer */
	case SYSLOG_ACTION_SIZE_BUFFER:
		error = drv_log_buf_len;
		break;
	default:
		error = -EINVAL;
		break;
	}
out:
	return error;
}

int print_drv_emit(int facility, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	static char textbuf[DRV_LOG_LINE_MAX];
	char *text = textbuf;
	size_t text_len;
	enum drv_log_flags lflags = 0;
	unsigned long flags;
	int printed_len = 0;

	local_irq_save(flags);
	raw_spin_lock(&drv_logbuf_lock);

	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

	/* mark and strip a trailing newline */
	if (text_len && text[text_len-1] == '\n') {
		text_len--;
		lflags |= LOG_NEWLINE;
	}

	/* strip kernel syslog prefix and extract log level or control flags */
	if (facility == 0) {
		int kern_level = print_drv_get_level(text);

		if (kern_level) {
			const char *end_of_header = print_drv_skip_level(text);
			switch (kern_level) {
			case '0' ... '7':
				if (level == -1)
					level = kern_level - '0';
			case 'd':	/* KERN_DEFAULT */
				lflags |= LOG_PREFIX;
			case 'c':	/* KERN_CONT */
				break;
			}
			text_len -= end_of_header - text;
			text = (char *)end_of_header;
		}
	}

	if (level == -1)
		level = DEFAULT_MESSAGE_LOGLEVEL;

	if (dict)
		lflags |= LOG_PREFIX|LOG_NEWLINE;

	drv_log_store(facility, level, lflags, 0, dict, dictlen, text, text_len);

	printed_len += text_len;

	raw_spin_unlock(&drv_logbuf_lock);
	local_irq_restore(flags);

	return printed_len;
}
EXPORT_SYMBOL(print_drv_emit);

int print_drv(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = print_drv_emit(0, -1, NULL, 0, fmt, args);
	va_end(args);

	return r;
}
EXPORT_SYMBOL(print_drv);

