/** @file wlan_linux.h
 *  @brief This file contains Linux OS related definitions and declarations
 *     
 *  Copyright (C) 2003-2008, Marvell International Ltd. 
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */
/********************************************************
Change log:
	05/30/07: Initial creation
********************************************************/

#ifndef _WLAN_LINUX_H
#define _WLAN_LINUX_H

/** True */
#ifndef	TRUE
#define TRUE			1
#endif
/** False */
#ifndef	FALSE
#define	FALSE			0
#endif

/** Bit definitions */
#ifndef BIT
#define BIT(x)  (1UL << (x))
#endif

#ifndef __KERNEL__
/** Character, 1 byte */
typedef char s8;
/** Unsigned character, 1 byte */
typedef unsigned char u8;

/** Short integer */
typedef signed short s16;
/** Unsigned short integer */
typedef unsigned short u16;

/** Long integer */
typedef signed long s32;
/** Unsigned long integer */
typedef unsigned long u32;

/** Long long integer */
typedef signed long long s64;
/** Unsigned long long integer */
typedef unsigned long long u64;

/** Unsigned long integer */
typedef u32 dma_addr_t;
/** Unsigned long integer */
typedef u32 dma64_addr_t;
#endif

/** Dma addresses are 32-bits wide.  */
#ifndef __ATTRIB_ALIGN__
#define __ATTRIB_ALIGN__ __attribute__((aligned(4)))
#endif

#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__ __attribute__ ((packed))
#endif

/** Debug Macro definition*/
#ifdef	DEBUG_LEVEL1

extern u32 drvdbg;
extern u32 ifdbg;

/** Debug message control bit definition for drvdbg */
#define	DBG_MSG		BIT(0)
#define DBG_FATAL	BIT(1)
#define DBG_ERROR	BIT(2)
#define DBG_DATA	BIT(3)
#define DBG_CMND	BIT(4)
#define DBG_EVENT	BIT(5)
#define DBG_INTR	BIT(6)

#define DBG_DAT_D	BIT(16)
#define DBG_CMD_D	BIT(17)
#define DBG_FW_D	BIT(18)

#define DBG_ENTRY	BIT(28)
#define DBG_WARN	BIT(29)
#define DBG_INFO	BIT(30)

/** Debug message control bit definition for ifdbg */
#define DBG_IF_D	BIT(0)

#ifdef	DEBUG_LEVEL2
#define	PRINTM_INFO(msg...)  do {if (drvdbg & DBG_INFO) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_WARN(msg...)  do {if (drvdbg & DBG_WARN) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_ENTRY(msg...) do {if (drvdbg & DBG_ENTRY) printk(KERN_DEBUG msg);} while(0)
#else
#define	PRINTM_INFO(msg...)  do {} while (0)
#define	PRINTM_WARN(msg...)  do {} while (0)
#define	PRINTM_ENTRY(msg...) do {} while (0)
#endif /* DEBUG_LEVEL2 */

#define	PRINTM_FW_D(msg...) do {if (drvdbg & DBG_FW_D) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_CMD_D(msg...) do {if (drvdbg & DBG_CMD_D) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_DAT_D(msg...) do {if (drvdbg & DBG_DAT_D) printk(KERN_DEBUG msg);} while(0)

#define	PRINTM_INTR(msg...)  do {if (drvdbg & DBG_INTR) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_EVENT(msg...) do {if (drvdbg & DBG_EVENT) printk(msg);} while(0)
#define	PRINTM_CMND(msg...)  do {if (drvdbg & DBG_CMND) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_DATA(msg...)  do {if (drvdbg & DBG_DATA) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_ERROR(msg...) do {if (drvdbg & DBG_ERROR) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_FATAL(msg...) do {if (drvdbg & DBG_FATAL) printk(KERN_DEBUG msg);} while(0)
#define	PRINTM_MSG(msg...)   do {if (drvdbg & DBG_MSG) printk(KERN_ALERT msg);} while(0)

#define	PRINTM_IF_D(msg...) do {if (ifdbg & DBG_IF_D) printk(KERN_DEBUG msg);} while(0)

#define	PRINTM(level,msg...) PRINTM_##level(msg)

#else

#define	PRINTM(level,msg...) do {} while (0)

#endif /* DEBUG_LEVEL1 */

/** Wait until a condition becomes true */
#define ASSERT(cond)						\
do {								\
	if (!(cond))						\
		PRINTM(INFO, "ASSERT: %s, %s:%i\n",		\
		       __FUNCTION__, __FILE__, __LINE__);	\
} while(0)

/** Log entry point for debugging */
#define	ENTER()			PRINTM(ENTRY, "Enter: %s, %s:%i\n", __FUNCTION__, \
							__FILE__, __LINE__)
/** Log exit point for debugging */
#define	LEAVE()			PRINTM(ENTRY, "Leave: %s, %s:%i\n", __FUNCTION__, \
							__FILE__, __LINE__)

#if defined(DEBUG_LEVEL1) && defined(__KERNEL__)
#define DBG_DUMP_BUF_LEN 	64
#define MAX_DUMP_PER_LINE	16
#define MAX_DATA_DUMP_LEN	48

static inline void
hexdump(char *prompt, u8 * buf, int len)
{
    int i;
    char dbgdumpbuf[DBG_DUMP_BUF_LEN];
    char *ptr = dbgdumpbuf;

    printk(KERN_DEBUG "%s:\n", prompt);
    for (i = 1; i <= len; i++) {
        ptr += sprintf(ptr, "%02x ", *buf);
        buf++;
        if (i % MAX_DUMP_PER_LINE == 0) {
            *ptr = 0;
            printk(KERN_DEBUG "%s\n", dbgdumpbuf);
            ptr = dbgdumpbuf;
        }
    }
    if (len % MAX_DUMP_PER_LINE) {
        *ptr = 0;
        printk(KERN_DEBUG "%s\n", dbgdumpbuf);
    }
}

#define DBG_HEXDUMP_CMD_D(x,y,z)    do {if (drvdbg & DBG_CMD_D) hexdump(x,y,z);} while(0)
#define DBG_HEXDUMP_DAT_D(x,y,z)    do {if (drvdbg & DBG_DAT_D) hexdump(x,y,z);} while(0)
#define DBG_HEXDUMP_IF_D(x,y,z)     do {if (ifdbg & DBG_IF_D) hexdump(x,y,z);} while(0)
#define DBG_HEXDUMP_FW_D(x,y,z)     do {if (drvdbg & DBG_FW_D) hexdump(x,y,z);} while(0)
#define	DBG_HEXDUMP(level,x,y,z)    DBG_HEXDUMP_##level(x,y,z)

#else
/** Do nothing since debugging is not turned on */
#define DBG_HEXDUMP(level,x,y,z)    do {} while (0)
#endif

#if defined(DEBUG_LEVEL2) && defined(__KERNEL__)
#define HEXDUMP(x,y,z)              do {if (drvdbg & DBG_INFO) hexdump(x,y,z);} while(0)
#else
/** Do nothing since debugging is not turned on */
#define HEXDUMP(x,y,z)              do {} while (0)
#endif

/*
 * Typedefs
 */

/** Long */
typedef long LONG;
/** Unsigned long long */
typedef unsigned long long ULONGLONG;
/** WLAN object ID as unsigned long */
typedef u32 WLAN_OID;

/** Char */
typedef char CHAR;
/** Pointer to a char */
typedef char *PCHAR;
/** Pointer to an unsigned char */
typedef u8 *PUCHAR;
/** Pointer to an unsigned short */
typedef u16 *PUSHORT;
/** Pointer to a long */
typedef long *PLONG;
/** Pointer to a long */
typedef PLONG LONG_PTR;
/** Pointer to an unsigned long */
typedef u32 *ULONG_PTR;
/** Pointer to an unsigned long */
typedef u32 *Pu32;
/** Unsigned int */
typedef unsigned int UINT;
/** Pointer to an unsigned int */
typedef UINT *PUINT;
/** Void */
typedef void VOID;
/** Void pointer */
typedef VOID *PVOID;
/** Wlan status as int */
typedef int WLAN_STATUS;
/** Unsigned char */
typedef u8 BOOLEAN;
/** Pointer to an unsigned char */
typedef BOOLEAN *PBOOLEAN;
/** Pointer to a driver object as a void pointer */
typedef PVOID PDRIVER_OBJECT;
/** Pointer to a unicoded string as pointer to an unsigned char */
typedef PUCHAR PUNICODE_STRING;
/** Long long */
typedef long long LONGLONG;
/** Pointer to a long long */
typedef LONGLONG *PLONGLONG;
/** Pointer to an unsigned long long */
typedef unsigned long long *PULONGLONG;
/** An ANSI string as pointer to an unsigned char */
typedef PUCHAR ANSI_STRING;
/** Pointer to an ANSI string as pointer to a pointer to an unsigned char */
typedef ANSI_STRING *PANSI_STRING;
/** Unsigned short */
typedef unsigned short WCHAR;
/** Pointer to an unsigned short */
typedef WCHAR *PWCHAR;
/** Pointer to an unsigned short */
typedef WCHAR *LPWCH;
/** Pointer to an unsigned short */
typedef WCHAR *PWCH;
/** Pointer to an unsigned short */
typedef WCHAR *NWPSTR;
/** Pointer to an unsigned short */
typedef WCHAR *LPWSTR;
/** Pointer to an unsigned short */
typedef WCHAR *PWSTR;
/** Semaphore structure */
typedef struct semaphore SEMAPHORE;

#ifdef __KERNEL__
typedef irqreturn_t IRQ_RET_TYPE;
#define IRQ_RET		return IRQ_HANDLED

/*
 * OS macro definitions
 */

#define os_time_get()	jiffies

extern unsigned long driver_flags;
#define OS_INT_DISABLE(x,y)	spin_lock_irqsave(&(x->adapter->driver_lock), y)
#define	OS_INT_RESTORE(x,y)	do { spin_unlock_irqrestore(&(x->adapter->driver_lock), y); \
			x->adapter->driver_lock = SPIN_LOCK_UNLOCKED; } while(0)

#define UpdateTransStart(dev) { \
	dev->trans_start = jiffies; \
}

#define OS_SET_THREAD_STATE(x)		set_current_state(x)

#define MODULE_GET	try_module_get(THIS_MODULE)
#define MODULE_PUT	module_put(THIS_MODULE)

#define OS_INIT_SEMAPHORE(x)    	init_MUTEX(x)
#define OS_ACQ_SEMAPHORE_BLOCK(x)	down_interruptible(x)
#define OS_ACQ_SEMAPHORE_NOBLOCK(x)	down_trylock(x)
#define OS_REL_SEMAPHORE(x) 		up(x)

/** Definitions below are needed for other OS like threadx */
#define	TX_DISABLE
#define TX_RESTORE
#define	ConfigureThreadPriority()
#define OS_INTERRUPT_SAVE_AREA
#define OS_FREE_LOCK(x)
#define TX_EVENT_FLAGS_SET(x, y, z)

/** Sleep until a condition gets true or a timeout elapses */
#define os_wait_interruptible_timeout(waitq, cond, timeout) \
	wait_event_interruptible_timeout(waitq, cond, ((timeout) * HZ / 1000))

static inline void
os_sched_timeout(u32 millisec)
{
    set_current_state(TASK_INTERRUPTIBLE);

    schedule_timeout((millisec * HZ) / 1000);
}

static inline u32
get_utimeofday(void)
{
    struct timeval t;
    u32 ut;

    do_gettimeofday(&t);
    ut = (u32) t.tv_sec * 1000000 + ((u32) t.tv_usec);
    return ut;
}

/*
 * OS timer specific
 */

typedef struct __WLAN_DRV_TIMER
{
    struct timer_list tl;
    void (*timer_function) (void *context);
    void *function_context;
    UINT time_period;
    BOOLEAN timer_is_periodic;
    BOOLEAN timer_is_canceled;
} __ATTRIB_PACK__ WLAN_DRV_TIMER, *PWLAN_DRV_TIMER;

static inline void
wlan_timer_handler(unsigned long fcontext)
{
    PWLAN_DRV_TIMER timer = (PWLAN_DRV_TIMER) fcontext;

    timer->timer_function(timer->function_context);

    if (timer->timer_is_periodic == TRUE) {
        mod_timer(&timer->tl, jiffies + ((timer->time_period * HZ) / 1000));
    }
}

static inline void
wlan_initialize_timer(PWLAN_DRV_TIMER timer,
                      void (*TimerFunction) (void *context),
                      void *FunctionContext)
{
    /* first, setup the timer to trigger the wlan_timer_handler proxy */
    init_timer(&timer->tl);
    timer->tl.function = wlan_timer_handler;
    timer->tl.data = (u32) timer;

    /* then tell the proxy which function to call and what to pass it */
    timer->timer_function = TimerFunction;
    timer->function_context = FunctionContext;
    timer->timer_is_canceled = FALSE;
}

static inline void
wlan_set_timer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = FALSE;
    timer->tl.expires = jiffies + (MillisecondPeriod * HZ) / 1000;
    add_timer(&timer->tl);
    timer->timer_is_canceled = FALSE;
}

static inline void
wlan_mod_timer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = FALSE;
    mod_timer(&timer->tl, jiffies + (MillisecondPeriod * HZ) / 1000);
    timer->timer_is_canceled = FALSE;
}

static inline void
wlan_set_periodic_timer(PWLAN_DRV_TIMER timer, UINT MillisecondPeriod)
{
    timer->time_period = MillisecondPeriod;
    timer->timer_is_periodic = TRUE;
    timer->tl.expires = jiffies + (MillisecondPeriod * HZ) / 1000;
    add_timer(&timer->tl);
    timer->timer_is_canceled = FALSE;
}

#define	FreeTimer(x)	do {} while (0)

static inline void
wlan_cancel_timer(WLAN_DRV_TIMER * timer)
{
    del_timer(&timer->tl);
    timer->timer_is_canceled = TRUE;
}

/*
 * OS Thread Specific
 */

#include	<linux/kthread.h>

typedef struct
{
    struct task_struct *task;
    wait_queue_head_t waitQ;
    pid_t pid;
    void *priv;
} wlan_thread;

static inline void
wlan_activate_thread(wlan_thread * thr)
{
        /** Record the thread pid */
    thr->pid = current->pid;

        /** Initialize the wait queue */
    init_waitqueue_head(&thr->waitQ);
}

static inline void
wlan_deactivate_thread(wlan_thread * thr)
{
    ENTER();

    /* Reset the pid */
    thr->pid = 0;
    LEAVE();
}

static inline void
wlan_create_thread(int (*wlanfunc) (void *), wlan_thread * thr, char *name)
{
    thr->task = kthread_run(wlanfunc, thr, "%s", name);
}

static inline int
wlan_terminate_thread(wlan_thread * thr)
{
    ENTER();

    /* Check if the thread is active or not */
    if (!thr->pid) {
        PRINTM(INFO, "Thread does not exist\n");
        return -1;
    }
    kthread_stop(thr->task);

    LEAVE();
    return 0;
}

#endif /* __KERNEL__ */

#endif /* _WLAN_LINUX_H */
