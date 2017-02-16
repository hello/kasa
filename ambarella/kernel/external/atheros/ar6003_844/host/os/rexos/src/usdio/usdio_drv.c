/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

static const char athId[] __attribute__ ((unused)) = "$Id: //depot/sw/releases/olca3.1-RC/host/os/rexos/src/usdio/usdio_drv.c#3 $";

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/* SDIO stuff */
#include "ctsystem.h"
#include "sdio_busdriver.h"
#include "_sdio_defs.h"
#include "sdio_lib.h"

#define MANUFACTURER_ID_AR6001_BASE        0x100
#define MANUFACTURER_ID_AR6002_BASE        0x200
#define FUNCTION_CLASS                     0x0
#define MANUFACTURER_CODE                  0x271

#define BUS_REQUEST_MAX_NUM                32

#define SDIO_CLOCK_FREQUENCY_DEFAULT       25000000
#define SDIO_CLOCK_FREQUENCY_REDUCED       12000000

#define SDWLAN_ENABLE_DISABLE_TIMEOUT      20
#define FLAGS_CARD_ENAB                    0x02
#define FLAGS_CARD_IRQ_UNMSK               0x04

#define USDIO_MBOX_START_ADDR(mbox, mboxwidth, mboxbaseaddress)  \
	mboxbaseaddress + mbox * mboxwidth 

#define USDIO_MBOX_END_ADDR(mbox, mboxwidth, mboxbaseaddress)  \
	USDIO_MBOX_START_ADDR(mbox, mboxwidth, mboxbaseaddress) + mboxwidth - 1

#define	REQ_DATA_NUM	(BUS_REQUEST_MAX_NUM + 10)
#define REQ_DATA_SIZE	0x800	/* 2k */

typedef struct target_function_context {
    SDFUNCTION           function; /* function description of the bus driver */
    OS_SEMAPHORE         instanceSem; /* instance lock. Unused */
    SDLIST               instanceList; /* list of instances. Unused */
} TARGET_FUNCTION_CONTEXT;

typedef struct bus_request {
    BOOL free;
    SDREQUEST *request;
	struct req_data *req;	/* MATS - added this */
} BUS_REQUEST;

struct req_data
{
	struct req_data *next;
	unsigned char data[REQ_DATA_SIZE];
	int length;
	int refcnt;
	void *req_context;
	unsigned int req_length;
	void *req_buffer;
	BUS_REQUEST *br;
};

static struct req_data s_req_data_items[REQ_DATA_NUM];

static struct req_data *s_req_data_free_list = NULL;
static struct req_data *s_complete_queue_head = NULL;
static struct req_data *s_complete_queue_tail = NULL;
static spinlock_t s_complete_queue_lock;

/* usdio stuff */
#include "usdio.h"

static SDFUNCTION *s_function = NULL;
static SDDEVICE *s_handle = NULL;

BOOL hifDeviceInserted(SDFUNCTION *function, SDDEVICE *handle);
void hifDeviceRemoved(SDFUNCTION *function, SDDEVICE *handle);
void hifRWCompletionHandler(SDREQUEST *request);

static struct usdio_cb *s_cb = NULL;

#define DBG_ALL			0xffffffff

#define DBG_DRIVER		0x00000001
#define DBG_TRACE		0x00000002
#define DBG_ERROR		0x00000004
#define DBG_IOCTL		0x00000008
#define DBG_RW			0x00000010
#define DBG_WARNING		0x00000020
#define DBG_EVENT		0x00000040
#define DBG_QUEUE		0x00000080
#define DBG_RW_OP		0x00000100
#define DBG_DATA		0x00000200
#define DBG_COMPLETE	0x00000400
#define DBG_INIT		0x00000800
#define DBG_READ		0x00001000
#define DBG_WRITE		0x00002000
#define DBG_POLL		0x00004000
#define DBG_REQ			0x00008000
#define DBG_IRQ			0x00010000
#define DBG_RW_BUFF		0x00020000
#define DBG_IOCTL_OP	0x00040000
#define DBG_REVENT		0x00080000

#if 0
int g_debug_flags = DBG_DRIVER | DBG_WARNING | DBG_ERROR | DBG_REVENT | 
					DBG_RW_OP | DBG_DATA | DBG_COMPLETE | DBG_IOCTL_OP |
					DBG_INIT;
#else
int g_debug_flags = DBG_ERROR | DBG_WARNING;
#endif

static int s_events = 0;

static void dump_data(char *d, int l);

#undef DEBUG
#define DEBUG

#define PRINTX_ARG(arg...) arg
#define DPRINTF(flags, args) do {        \
    if (g_debug_flags & (flags)) {                    \
        printk(KERN_ALERT PRINTX_ARG args);      \
    }                                            \
} while (0)


int debuglevel = 0;
int debugdriver = 0;
int onebitmode = 0;
int busspeedlow = 0;

module_param(debuglevel, int, 0644);
module_param(debugdriver, int, 0644);
module_param(onebitmode, int, 0644);
module_param(busspeedlow, int, 0644);


/* ------ Global Variable Declarations ------- */
SD_PNP_INFO Ids[] = {
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6001_BASE | 0xB,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6001_BASE | 0xA,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6001_BASE | 0x9,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6001_BASE | 0x8,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6002_BASE | 0x0,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
        .SDIO_ManufacturerID = MANUFACTURER_ID_AR6002_BASE | 0x1,
        .SDIO_ManufacturerCode = MANUFACTURER_CODE,
        .SDIO_FunctionClass = FUNCTION_CLASS,
        .SDIO_FunctionNo = 1
    },
    {
    }                      //list is null termintaed
};

TARGET_FUNCTION_CONTEXT FunctionContext = {
    .function.Version    = CT_SDIO_STACK_VERSION_CODE,
    .function.pName      = "sdio_wlan",
    .function.MaxDevices = 1,
    .function.NumDevices = 0,
    .function.pIds       = Ids,
    .function.pProbe     = hifDeviceInserted,
    .function.pRemove    = hifDeviceRemoved,
    .function.pSuspend   = NULL,
    .function.pResume    = NULL,
    .function.pWake      = NULL,
    .function.pContext   = &FunctionContext,
};

BUS_REQUEST busRequest[BUS_REQUEST_MAX_NUM];
OS_CRITICALSECTION lock;

struct usdio_cb
{
	spinlock_t lock;
	wait_queue_head_t wq;
};

/* Function declarations */
static int usdio_init_module(void);
static void usdio_cleanup_module(void);
struct file_operations usdio_fops;

static void set_event(struct usdio_cb *cb, unsigned int event)
{
	unsigned long flags;

	DPRINTF(DBG_EVENT|DBG_TRACE,
		("%s() Enter event 0x%08x.\n", __func__, event));

	if(cb == NULL)
	{
		/* Driver is not open but remember event anyway */
		DPRINTF(DBG_WARNING, ("%s() Not open.\n", __func__));
    	s_events |= event;
		return;
	}

    spin_lock_irqsave(&cb->lock, flags);
    s_events |= event;
    spin_unlock_irqrestore(&cb->lock, flags);
	wake_up(&cb->wq);

	return;
}

static struct req_data *alloc_req_data(void)
{
	struct req_data *p;

#if 0
	/* Acquire lock */
	CriticalSectionAcquire(&dflock);
#endif

	p = s_req_data_free_list;
	if(p != NULL)
	{
		s_req_data_free_list = p->next;
	}

#if 0
	/* Release lock */
	CriticalSectionRelease(&dflock);
#endif

	/* Make sure he is cleared out */
	p->next = NULL;

	return(p);
}

static void free_req_data(struct req_data *p)
{
	/* Only free if not on any queue */
	if(p->refcnt != 0)
	{
		return;
	}

#if 0
	/* Acquire lock */
	CriticalSectionAcquire(&dlock);
#endif

	p->next = s_req_data_free_list;
	s_req_data_free_list = p;

#if 0
	/* Release lock */
	CriticalSectionRelease(&dlock);
#endif

	return;
}

void enqueue_complete(struct req_data *r)
{
	unsigned long flags;

    spin_lock_irqsave(&s_complete_queue_lock, flags);

#if 0
	DPRINTF(DBG_QUEUE, ("%s() Enter head 0x%08x tail 0x%08x r 0x%08x.\n",
		__func__, (unsigned int) s_complete_queue_head,
		(unsigned int) s_complete_queue_tail, (unsigned int) r));
#endif

	if(s_complete_queue_head == NULL)
	{
		s_complete_queue_head = r;
	}
	else
	{
		s_complete_queue_tail->next = r;
	}
	s_complete_queue_tail = r;

#if 0
	DPRINTF(DBG_QUEUE, ("%s() Exit head 0x%08x tail 0x%08x r 0x%08x.\n",
		__func__, (unsigned int) s_complete_queue_head,
		(unsigned int) s_complete_queue_tail, (unsigned int) r));
#endif

    spin_unlock_irqrestore(&s_complete_queue_lock, flags);
}

struct req_data *dequeue_complete(void)
{
	unsigned long flags;
	struct req_data *r;

    spin_lock_irqsave(&s_complete_queue_lock, flags);

#if 0
	DPRINTF(DBG_QUEUE, ("%s() Enter head 0x%08x tail 0x%08x.\n",
		__func__, (unsigned int) s_complete_queue_head,
		(unsigned int) s_complete_queue_tail));
#endif

	if(s_complete_queue_head == NULL)
	{
		return(NULL);
	}

	r = s_complete_queue_head;
	s_complete_queue_head = r->next;

	/* Make sure to reset tail - not really needed but we want to be tidy */
	if(s_complete_queue_head == NULL)
	{
		s_complete_queue_tail = NULL;
	}

#if 0
	DPRINTF(DBG_QUEUE, ("%s() Exit head 0x%08x tail 0x%08x r 0x%08x.\n",
		__func__, (unsigned int) s_complete_queue_head,
		(unsigned int) s_complete_queue_tail, (unsigned int) r));
#endif

    spin_unlock_irqrestore(&s_complete_queue_lock, flags);

	return(r);
}

void hifFreeDeviceRequest(SDREQUEST *request)
{
	int count;
	DBG_ASSERT(request != NULL);

	/* Acquire lock */
	CriticalSectionAcquire(&lock);

	for(count = 0; count < BUS_REQUEST_MAX_NUM; count ++)
	{
		if(busRequest[count].request == request)
		{
			DPRINTF(DBG_REQ, ("%s() busRequest[%d] 0x%08x req 0x%08x\n",
				__func__, count, (unsigned int) &busRequest[count],
				(unsigned int) busRequest[count].req));
			busRequest[count].free = TRUE;
			free_req_data(busRequest[count].req);	/* MATS added */
			break;
		}
	}

	/* Release lock */
	CriticalSectionRelease(&lock);

	return;
}

SDREQUEST *hifAllocateDeviceRequest(SDDEVICE *device)
{
	SDREQUEST *request;
	int count;
	DBG_ASSERT(device != NULL);

	/* Acquire lock */
	CriticalSectionAcquire(&lock);

	request = NULL;
	for(count = 0; count < BUS_REQUEST_MAX_NUM; count++)
	{
#if 0
		DPRINTF(DBG_REQ,
			("busRequest[%d].request = 0x%p, busRequest[%d].free = %d\n",
			count, busRequest[count].request, count, busRequest[count].free));
#endif

		if(busRequest[count].free)
		{
			request = busRequest[count].request;
			busRequest[count].free = FALSE;
			busRequest[count].req = alloc_req_data();	/* MATS added */

			DPRINTF(DBG_REQ,
				("Allocate busRequest[%d].request = 0x%p, busRequest[%d].free = %d\n",
				count, busRequest[count].request, count, busRequest[count].free));
			break;
		}
	}

	/* Release lock */
	CriticalSectionRelease(&lock);

	return(request);
}

BUS_REQUEST *hifFindBusReq(SDREQUEST *request)
{
	int count;
	BUS_REQUEST *p = NULL;
	DBG_ASSERT(request != NULL);

	/* Acquire lock */
	CriticalSectionAcquire(&lock);

	for(count = 0; count < BUS_REQUEST_MAX_NUM; count ++)
	{
		if(busRequest[count].request == request)
		{
			p = &busRequest[count];
			break;
		}
	}

	/* Release lock */
	CriticalSectionRelease(&lock);

	return(p);
}

BOOL hifDeviceInserted(SDFUNCTION *function, SDDEVICE *handle)
{
	int status;

	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

	/* Save this device in our static variables */
	s_function = function;
	s_handle = handle;

	status = register_chrdev(USDIO_MAJOR_NUMBER, THIS_DEV_NAME, &usdio_fops);
	if(status < 0)
	{
		DPRINTF(DBG_WARNING,
			("%s() Failed to register device.\n", __func__));
	    return FALSE;
	}

	/* Signal that an event is available */
	set_event(s_cb, USDIO_EVENT_INSERTED);

	return(TRUE);
}

void hifDeviceRemoved(SDFUNCTION *function, SDDEVICE *handle)
{
	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

	/* Clear this device from our static variables */
	s_function = NULL;
	s_handle = NULL;

	unregister_chrdev(USDIO_MAJOR_NUMBER, THIS_DEV_NAME);

	/* Signal that an event is available */
	set_event(s_cb, USDIO_EVENT_REMOVED);

	return;
}

void
hifRWCompletionHandler(SDREQUEST *request)
{
	BUS_REQUEST *br;
	
#if 0
	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));
#endif

	DPRINTF(DBG_RW, ("%s() Status 0x%08x Flags 0x%08x %s %s\n",
		__func__, request->Status, request->Flags,
		request->Flags & SDREQ_FLAGS_DATA_WRITE ? "Write" : "Read",
		request->Flags & SDREQ_FLAGS_DATA_TRANS ? "Data" : ""));
#if 0
	DPRINTF(DBG_DRIVER, ("%s() pDataBuffer 0x%08x = 0x%02x\n",
		__func__, (unsigned int) request->pDataBuffer,
		((unsigned char *) request->pDataBuffer)[0]));
	DPRINTF(DBG_DRIVER, ("%s() pCompleteContext 0x%08x\n",
		__func__, (unsigned int) request->pCompleteContext));
	DPRINTF(DBG_DRIVER, ("%s() BlockLen 0x%08x BlockCount 0x%08x\n",
		__func__, request->BlockLen, request->BlockCount));
	DPRINTF(DBG_DRIVER, ("%s() pCompleteContext 0x%08x\n",
		__func__, (unsigned int) request->pCompleteContext));
#endif

	br = (BUS_REQUEST *) request->pCompleteContext;

	DPRINTF(DBG_RW, ("%s() context 0x%08x address 0x%08x length 0x%08x\n",
		__func__, (unsigned int) br->req->req_context,
		(unsigned int) br->req->req_buffer, br->req->req_length));

	if(SDIO_SUCCESS(request->Status))
	{
		if(!(request->Flags & SDREQ_FLAGS_DATA_WRITE))
		{
			dump_data(request->pDataBuffer, br->req->req_length);

			/* Enqueue on our completion queue */
			enqueue_complete(br->req);

			/* Signal that an event is available */
			set_event(s_cb, USDIO_EVENT_ROK);
		}
		else
		{
			/* Signal that an event is available */
			set_event(s_cb, USDIO_EVENT_WOK);

			hifFreeDeviceRequest(request);
		}
	}
	else
	{
		if(!(request->Flags & SDREQ_FLAGS_DATA_WRITE))
		{
			/* Signal that an event is available */
			set_event(s_cb, USDIO_EVENT_RERROR);
		}
		else
		{
			set_event(s_cb, USDIO_EVENT_WERROR);
		}

		hifFreeDeviceRequest(request);
	}

	return;
}

void hifIRQHandler(void *context)
{
	DPRINTF(DBG_IRQ, ("%s() Enter\n", __func__));

	/* Signal that an event is available */
	set_event(s_cb, USDIO_EVENT_IRQ);

	return;
}

static int __init
usdio_init_module(void)
{
    static int probed = 0;
	int i;
	SDIO_STATUS status;

    g_debug_flags = debuglevel | DBG_ERROR | DBG_WARNING;
    
	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

    if (probed)
	{
        return(-ENODEV);
    }
    probed++;

	spin_lock_init(&s_complete_queue_lock);

	CriticalSectionInit(&lock);

	/* Clear and free these things */
	memset(&s_req_data_items[0], 0, sizeof(s_req_data_items));
	for(i = 0; i < REQ_DATA_NUM; i++)
	{
		free_req_data(&s_req_data_items[i]);
	}

	/* Register with bus driver core */
	status = SDIO_RegisterFunction(&FunctionContext.function);
	DBG_ASSERT(SDIO_SUCCESS(status));

	DPRINTF(DBG_DRIVER, ("%s() Probe = %d\n", __func__, probed));
	
    return 0;
}

static void __exit
usdio_cleanup_module(void)
{
	SDIO_STATUS status;

    DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

	/* Unregister with bus driver core */
	status = SDIO_UnregisterFunction(&FunctionContext.function);
	DBG_ASSERT(SDIO_SUCCESS(status));

	return;
}

static int
usdio_open(struct inode *inode, struct file *file)
{
    int minor;
    int major;
	struct usdio_cb *cb;
    
	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

    major = MAJOR(inode->i_rdev);
    minor = MINOR(inode->i_rdev);
    minor = minor & 0x0f;

	/* Allocate memory the device control structure */
	if((s_cb = cb = kmalloc(sizeof(struct usdio_cb), GFP_KERNEL)) == NULL)
	{
		DPRINTF(DBG_ERROR, ("%s() No memory.\n", __func__));
		return -ENOMEM;
	}

	/* Make sure it is initialized */
	memset(cb, 0, sizeof(struct usdio_cb));

	/* Initialize lock and wait queue */
	spin_lock_init(&cb->lock);
	init_waitqueue_head(&cb->wq);

	/* Save the data */
	file->private_data = (void *) cb;

	DPRINTF(DBG_DRIVER,("%s() Exit\n", __func__));

    return 0;
}

static int
usdio_release(struct inode *inode, struct file *file)
{
	struct usdio_cb *cb = (struct usdio_cb *) file->private_data;

	DPRINTF(DBG_DRIVER, ("%s() Enter\n", __func__));

	s_cb = NULL;
	kfree(cb);

	DPRINTF(DBG_DRIVER, ("%s() Exit\n", __func__));

	return 0;
}

static unsigned int
usdio_poll(struct file *file, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	int events = 0;
	struct usdio_cb *cb = (struct usdio_cb *) file->private_data;
	unsigned long flags;

	/* DPRINTF(DBG_POLL, ("%s() Enter\n", __func__)); */

    poll_wait(file, &cb->wq, wait);

	/* DPRINTF(DBG_POLL, ("%s() Wait\n", __func__)); */

    spin_lock_irqsave(&cb->lock, flags);
    events = s_events;
    spin_unlock_irqrestore(&cb->lock, flags);

	/* DPRINTF(DBG_EVENT|DBG_POLL,
		("%s() Events 0x%08x\n", __func__, events)); */

    if(events > 0)
	{
		mask |= POLLIN | POLLRDNORM;			/* Read  */
		/* mask |= POLLOUT | POLLWRNORM; */		/* Write */
		/* mask |= POLLPRI; */					/* Exceptional */
		DPRINTF(DBG_REVENT|DBG_POLL,
			("%s() Return events 0x%08x\n", __func__, events));
    }

	/* DPRINTF(DBG_POLL, ("%s() Exit\n", __func__)); */

	return(mask);
}

static ssize_t
usdio_write(struct file *file, const char *buf, size_t length, loff_t *offset)
{
#ifdef MATS
	struct usdio_cb *cb = (struct usdio_cb *) file->private_data;
#endif

	DPRINTF(DBG_WRITE, ("%s() Enter\n", __func__));

#ifdef MATS
	if(copy_from_user(&buffer[0], (unsigned char *)buf, length))
	{
		return(-EFAULT);
	}
#endif

    return(length);
}

ssize_t usdio_read(struct file *file, char *buf, size_t length, loff_t *offset)
{
	struct usdio_cb *cb = (struct usdio_cb *) file->private_data;
	unsigned long flags;
	int events = 0;

	DPRINTF(DBG_READ, ("%s() Enter\n", __func__));

	/* Wait until we have an event               */
	/* We will wait until condition is satisfied */
	wait_event_interruptible(cb->wq, s_events != 0);

	spin_lock_irqsave(&cb->lock, flags);
	events = s_events;	/* Get the events   */
	s_events = 0;		/* Reset the events */
	spin_unlock_irqrestore(&cb->lock, flags);

	DPRINTF(DBG_EVENT, ("%s() Value 0x%08x\n", __func__, events));

	/* Not very portable - but o well */
	if(copy_to_user(buf, (unsigned char *) &events, sizeof(unsigned int)))
	{
		return(-EFAULT);
	}

	DPRINTF(DBG_READ, ("%s() Exit\n", __func__));

	return(sizeof(unsigned int));
}

int do_init_ioctl(struct usdio_cb *cb, struct usdio_req *r)
{
	BOOL enabled;
	unsigned char data;
	int count;
	SDIO_STATUS status;
	unsigned short maxBlocks;
	unsigned short maxBlockSize;
	SDDEVICE *handle = (SDDEVICE *) r->handle;
	SDCONFIG_BUS_MODE_DATA busSettings;
	SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;
	SDCONFIG_FUNC_SLOT_CURRENT_DATA slotCurrent;
    SD_BUSCLOCK_RATE currentBusClock;
    
	DPRINTF(DBG_INIT, ("%s() Enter\n", __func__));

	DBG_ASSERT(handle != NULL);

    /*
     * Issue commands to get the manufacturer ID and stuff and compare it
     * against the rev Id derived from the ID registered during the
     * initialization process. Report the device only in the case there
     * is a match. In the case od SDIO, the bus driver has already queried
     * these details so we just need to use their data structures to get the
     * relevant values. Infact, the driver has already matched it against
     * the Ids that we registered with it so we dont need to the step here.
     */

    /* Configure the SDIO Bus Width */
    if (onebitmode) {
        data = SDIO_BUS_WIDTH_1_BIT;
        status = SDLIB_IssueCMD52(handle, 0, SDIO_BUS_IF_REG, &data, 1, 1);
        if (!SDIO_SUCCESS(status)) {
            DPRINTF(DBG_ERROR,
                            ("Unable to set the bus width to 1 bit\n"));
            return FALSE;
        }
    }

    /* Get current bus flags */
    ZERO_OBJECT(busSettings);

    busSettings.BusModeFlags = SDDEVICE_GET_BUSMODE_FLAGS(handle);
    if (onebitmode) {
        SDCONFIG_SET_BUS_WIDTH(busSettings.BusModeFlags,
                               SDCONFIG_BUS_WIDTH_1_BIT);
    }

        /* get the current operating clock, the bus driver sets us up based
         * on what our CIS reports and what the host controller can handle
         * we can use this to determine whether we want to drop our clock rate
         * down */
    currentBusClock = SDDEVICE_GET_OPER_CLOCK(handle);
    busSettings.ClockRate = currentBusClock;

    DPRINTF(DBG_TRACE,
                        ("HIF currently running at: %d \n",currentBusClock));

        /* see if HIF wants to run at a lower clock speed, we may already be
         * at that lower clock speed */
    if (currentBusClock > (SDIO_CLOCK_FREQUENCY_DEFAULT >> busspeedlow)) {
        busSettings.ClockRate = SDIO_CLOCK_FREQUENCY_DEFAULT >> busspeedlow;
        DPRINTF(DBG_WARNING,
                        ("HIF overriding clock to %d \n",busSettings.ClockRate));
    }

    /* Issue config request to override clock rate */
    status = SDLIB_IssueConfig(handle, SDCONFIG_FUNC_CHANGE_BUS_MODE, &busSettings,
                               sizeof(SDCONFIG_BUS_MODE_DATA));
    if (!SDIO_SUCCESS(status)) {
        DPRINTF(DBG_ERROR,
                        ("Unable to configure the host clock\n"));
        return FALSE;
    } else {
        DPRINTF(DBG_TRACE,
                        ("Configured clock: %d, Maximum clock: %d\n",
                        busSettings.ActualClockRate,
                        SDDEVICE_GET_MAX_CLOCK(handle)));
    }

    /*
     * Check if the target supports block mode. This result of this check
     * can be used to implement the HIFReadWrite API.
     */
    if (SDDEVICE_GET_SDIO_FUNC_MAXBLKSIZE(handle)) {
        /* Limit block size to operational block limit or card function
           capability */
        maxBlockSize = min(SDDEVICE_GET_OPER_BLOCK_LEN(handle),
                           SDDEVICE_GET_SDIO_FUNC_MAXBLKSIZE(handle));

        /* check if the card support multi-block transfers */
        if (!(SDDEVICE_GET_SDIOCARD_CAPS(handle) & SDIO_CAPS_MULTI_BLOCK)) {
            DPRINTF(DBG_TRACE, ("Byte basis only\n"));

            /* Limit block size to max byte basis */
            maxBlockSize =  min(maxBlockSize,
                                (unsigned short)SDIO_MAX_LENGTH_BYTE_BASIS);
            maxBlocks = 1;
        }
		else
		{
			DPRINTF(DBG_TRACE|DBG_INIT,
				("Multi-block capable (%d)\n", r->u.init.blocksize));

			maxBlocks = SDDEVICE_GET_OPER_BLOCKS(handle);
			status = SDLIB_SetFunctionBlockSize(handle, r->u.init.blocksize);

			if (!SDIO_SUCCESS(status))
			{
				DPRINTF(DBG_ERROR,
					("Failed to set block size. Err:%d\n", status));
				return FALSE;
			}
		}

		DPRINTF(DBG_TRACE|DBG_INIT,
			("Bytes Per Block: %d bytes, Block Count:%d \n",
			maxBlockSize, maxBlocks));
	}
	else
	{
		DPRINTF(DBG_ERROR,
			("Function does not support Block Mode!\n"));
		return FALSE;
	}

    /* Allocate the slot current */
    status = SDLIB_GetDefaultOpCurrent(handle, &slotCurrent.SlotCurrent);
    
    if (SDIO_SUCCESS(status)) {
        DPRINTF(DBG_TRACE, ("Allocating Slot current: %d mA\n",
                                slotCurrent.SlotCurrent));
        status = SDLIB_IssueConfig(handle, SDCONFIG_FUNC_ALLOC_SLOT_CURRENT,
                                   &slotCurrent, sizeof(slotCurrent));
        if (!SDIO_SUCCESS(status)) {
            DPRINTF(DBG_ERROR,
                            ("Failed to allocate slot current %d\n", status));
            return FALSE;
        }
    }

	/* Enable the dragon function */
	count = 0;
	enabled = FALSE;
	fData.TimeOut = 1;
	fData.EnableFlags = SDCONFIG_ENABLE_FUNC;
	while ((count++ < SDWLAN_ENABLE_DISABLE_TIMEOUT) && !enabled)
	{
		/* Enable dragon */
		status = SDLIB_IssueConfig(handle, SDCONFIG_FUNC_ENABLE_DISABLE,
			   &fData, sizeof(fData));
		if (!SDIO_SUCCESS(status))
		{
			DPRINTF(DBG_TRACE|DBG_INIT,
				("Attempting to enable the card again\n"));
			continue;
		}

		/* Mark the status as enabled */
		enabled = TRUE;
	}

	/* Check if we were succesful in enabling the target */
	if (!enabled)
	{
		DPRINTF(DBG_ERROR,
			("Failed to communicate with the target\n"));
		return FALSE;
	}

	/* Allocate the bus requests to be used later */
    ZERO_OBJECT(busRequest);
	for (count = 0; count < BUS_REQUEST_MAX_NUM; count ++)
	{
		if ((busRequest[count].request = SDDeviceAllocRequest(handle)) == NULL)
		{
			DPRINTF(DBG_ERROR, ("Unable to allocate memory\n"));
			/* TODO: Free the memory that has already been allocated */
			return FALSE;
		}

		busRequest[count].free = TRUE;
		busRequest[count].req = NULL;
		DPRINTF(DBG_REQ,
			("busRequest[%d].request = 0x%p, busRequest[%d].free = %d\n",
			count, busRequest[count].request, count, busRequest[count].free));
	}

	return TRUE;
}

int do_uninit_ioctl(struct usdio_cb *cb, struct usdio_req *r)
{
    unsigned char data;
    int count;
    SDIO_STATUS status;
    SDCONFIG_BUS_MODE_DATA busSettings;
    SDCONFIG_FUNC_ENABLE_DISABLE_DATA fData;
	SDDEVICE *handle = (SDDEVICE *) r->handle;

	DPRINTF(DBG_INIT, ("%s() Enter\n", __func__));

	if (handle != NULL)
	{
		/* Remove the allocated current if any */
		status = SDLIB_IssueConfig(handle,
			                       SDCONFIG_FUNC_FREE_SLOT_CURRENT, NULL, 0);
		DBG_ASSERT(SDIO_SUCCESS(status));

		/* Disable the card */
		fData.EnableFlags = SDCONFIG_DISABLE_FUNC;
		fData.TimeOut = 1;
		status = SDLIB_IssueConfig(handle, SDCONFIG_FUNC_ENABLE_DISABLE,
			                       &fData, sizeof(fData));
		DBG_ASSERT(SDIO_SUCCESS(status));

		/* Perform a soft I/O reset */
		data = SDIO_IO_RESET;
		status = SDLIB_IssueCMD52(handle, 0, SDIO_IO_ABORT_REG,
			&data, 1, 1);
		DBG_ASSERT(SDIO_SUCCESS(status));

		/* 
		* WAR - Codetelligence driver does not seem to shutdown correctly in 1
		* bit mode. By default it configures the HC in the 4 bit. Its later in
		* our driver that we switch to 1 bit mode. If we try to shutdown, the
		* driver hangs so we revert to 4 bit mode, to be transparent to the 
		* underlying bus driver.
		*/
		if(onebitmode)
		{
			ZERO_OBJECT(busSettings);
			busSettings.BusModeFlags = SDDEVICE_GET_BUSMODE_FLAGS(handle);
			SDCONFIG_SET_BUS_WIDTH(busSettings.BusModeFlags,
				                   SDCONFIG_BUS_WIDTH_4_BIT);

			/* Issue config request to change the bus width to 4 bit */
			status = SDLIB_IssueConfig(handle, SDCONFIG_BUS_MODE_CTRL,
				                       &busSettings,
                                       sizeof(SDCONFIG_BUS_MODE_DATA));
			DBG_ASSERT(SDIO_SUCCESS(status));
		}

		/* Free the bus requests */
		for (count = 0; count < BUS_REQUEST_MAX_NUM; count ++)
		{
			SDDeviceFreeRequest(handle, busRequest[count].request);
			busRequest[count].free = TRUE;
		}
	}
	else
	{
		/* Unregister with bus driver core */
		status = SDIO_UnregisterFunction(&FunctionContext.function);
		DBG_ASSERT(SDIO_SUCCESS(status));
	}

	return(0);
}

static char s_table[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'b', 'c', 'd', 'e', 'f'
};

static void dump_data(char *d, int l)
{
    int i;
    char buff[128];
    char *s = buff;
    int n = 0;

    DPRINTF(DBG_DATA, ("******************************\n"));

    for(i = 0; i < l; i++)
    {
        if(i > 0 && (i % 16) == 0)
        {
            DPRINTF(DBG_DATA, ("%4d - %s\n", n, buff));
            s = buff;
            n = i;
        }

        *s++ = s_table[(*d >> 4) & 0xf];
        *s++ = s_table[*d & 0xf];
        *s++ = ' ';
        *s = '\0';
        d++;
    }

    if(s != buff)
    {
        DPRINTF(DBG_DATA, ("%4d - %s\n", n, buff));
    }

    DPRINTF(DBG_DATA, ("******************************\n"));
}

int do_rw_ioctl(struct usdio_cb *cb, struct usdio_req *r)
{
	unsigned char rw;
	unsigned char mode;
	unsigned char funcNo;
	unsigned char opcode;
	unsigned short count;
	SDREQUEST *sdrequest;
	SDIO_STATUS status;
	unsigned int address;
	BUS_REQUEST *br;

	DPRINTF(DBG_RW, ("%s() Enter\n", __func__));

	DBG_ASSERT(r->handle != NULL);

	sdrequest = hifAllocateDeviceRequest(r->handle);

	if(sdrequest == NULL)
	{
		DPRINTF(DBG_ERROR, ("Unable to allocate bus request\n"));
		return(-1);
	}

	/* Find our bus request */
	br = hifFindBusReq(sdrequest);

	/* Assign some space for data */
	sdrequest->pDataBuffer = br->req->data;
	br->req->req_context = r->u.rw.context;
	br->req->req_buffer = r->u.rw.buffer;
	br->req->req_length = r->u.rw.length;

	/* Remember ourselves */
	br->req->br = br;

	if(r->u.rw.flags & RW_SYNCHRONOUS)
	{
		sdrequest->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS;
		sdrequest->pCompleteContext = NULL;
		sdrequest->pCompletion = NULL;
		DPRINTF(DBG_RW, ("Execution mode: Synchronous\n"));
	}
	else if(r->u.rw.flags & RW_ASYNCHRONOUS)
	{
		sdrequest->Flags = SDREQ_FLAGS_RESP_SDIO_R5 | SDREQ_FLAGS_DATA_TRANS |
		   SDREQ_FLAGS_TRANS_ASYNC;
		sdrequest->pCompleteContext = br;
		sdrequest->pCompletion = hifRWCompletionHandler;
		DPRINTF(DBG_RW, ("Execution mode: Asynchronous\n"));
	}
	else
	{
		DPRINTF(DBG_ERROR,
				("Invalid execution mode: 0x%08x\n", r->u.rw.flags));
		return(-1);
	}

	if (r->u.rw.flags & RW_EXTENDED_IO)
	{
		DPRINTF(DBG_RW, ("Command type: CMD53\n"));
		sdrequest->Command = CMD53;
	}
	else
	{
		DPRINTF(DBG_ERROR,
			("Invalid command type: 0x%08x\n", r->u.rw.flags));
		return(-1);
	}

	if(r->u.rw.flags & RW_BLOCK_BASIS)
	{
		mode = CMD53_BLOCK_BASIS;
		sdrequest->BlockLen = r->u.rw.blocksize;
		sdrequest->BlockCount = r->u.rw.length / r->u.rw.blocksize;
		count = sdrequest->BlockCount;
		DPRINTF(DBG_RW,
			("Block mode (BlockLen: %d, BlockCount: %d)\n",
			sdrequest->BlockLen, sdrequest->BlockCount));
	}
	else if(r->u.rw.flags & RW_BYTE_BASIS)
	{
		mode = CMD53_BYTE_BASIS;
		sdrequest->BlockLen = r->u.rw.length;
		sdrequest->BlockCount = 1;
		count = sdrequest->BlockLen;
		DPRINTF(DBG_RW,
			("Byte mode (BlockLen: %d, BlockCount: %d)\n",
			sdrequest->BlockLen, sdrequest->BlockCount));
	}
	else
	{
		DPRINTF(DBG_ERROR,
			("Invalid data mode: 0x%08x\n", r->u.rw.flags));
		return(-1);
	}

	address = r->u.rw.address;

	DPRINTF(DBG_RW_BUFF, ("address 0x%08x, buffer 0x%08x, length 0x%08x\n",
		r->u.rw.address, (unsigned int) r->u.rw.buffer, r->u.rw.length));

	if(r->u.rw.flags & RW_WRITE)
	{
		rw = CMD53_WRITE;
		sdrequest->Flags |= SDREQ_FLAGS_DATA_WRITE;
		if(!(r->u.rw.flags & RW_RAW_ADDRESSING))
		{
			if ((address >= USDIO_MBOX_START_ADDR(0, r->u.rw.mboxwidth, r->u.rw.mboxbaseaddress)) &&
				(address <= USDIO_MBOX_END_ADDR(3, r->u.rw.mboxwidth, r->u.rw.mboxbaseaddress)))
			{
				/* 
				 * Mailbox write. Adjust the address so that the last byte 
				 * falls on the EOM address.
				 */
				address = address + r->u.rw.mboxwidth - r->u.rw.length;
			}
		}

		DPRINTF(DBG_RW, ("Direction: Write 0x%08x\n", address));
		
 		/* Get the data from user space */
		if(copy_from_user(sdrequest->pDataBuffer, r->u.rw.buffer, r->u.rw.length))
		{
			DPRINTF(DBG_ERROR, ("Failed to copy user space data.\n"));
			return(-1);
		}

		dump_data(sdrequest->pDataBuffer, r->u.rw.length);
	}
	else if(r->u.rw.flags & RW_READ)
	{
		rw = CMD53_READ;
		DPRINTF(DBG_RW, ("Direction: Read\n"));
	}
	else
	{
		DPRINTF(DBG_ERROR,
			("Invalid direction: 0x%08x\n", r->u.rw.flags));
		return(-1);
	}

	if (r->u.rw.flags & RW_FIXED_ADDRESS)
	{
		opcode = CMD53_FIXED_ADDRESS;
		DPRINTF(DBG_RW, ("Address mode: Fixed\n"));
	}
	else if(r->u.rw.flags & RW_INCREMENTAL_ADDRESS)
	{
		opcode = CMD53_INCR_ADDRESS;
		DPRINTF(DBG_RW, ("Address mode: Incremental\n"));
	}
	else
	{
		DPRINTF(DBG_ERROR,
			("Invalid address mode: 0x%08x\n", r->u.rw.flags));
		return(-1);
	}

	funcNo = SDDEVICE_GET_SDIO_FUNCNO((SDDEVICE *) r->handle);
	DPRINTF(DBG_RW, ("Function number: %d\n", funcNo));
	SDIO_SET_CMD53_ARG(sdrequest->Argument, rw, funcNo,
	   mode, opcode, address, count);

	DPRINTF(DBG_RW_OP,
		("%s %s %s fn %d @ 0x%08x (%s) %d bytes %d blks\n",
		r->u.rw.flags & RW_BYTE_BASIS ? "Byte": "Block",
		r->u.rw.flags & RW_SYNCHRONOUS ? "Sync" : "Async",
		r->u.rw.flags & RW_READ ? "Read" : "Write",
		funcNo,
		address,
		r->u.rw.flags & RW_FIXED_ADDRESS ? "Fix" : "Inc",
		sdrequest->BlockLen, sdrequest->BlockCount));

	/* Send the command out */
	status = SDDEVICE_CALL_REQUEST_FUNC((SDDEVICE *) r->handle, sdrequest);
	if(status == SDIO_STATUS_PENDING)
	{
		DBG_ASSERT(r->u.rw.flags & RW_ASYNCHRONOUS);
		return(0);
	}
	else if(status == SDIO_STATUS_SUCCESS)
	{
		DBG_ASSERT(r->u.rw.flags & RW_SYNCHRONOUS);
		hifFreeDeviceRequest(sdrequest);
		return(0);
	}

	return(-1);
}

int do_complete_ioctl(struct usdio_cb *cb, struct usdio_req *r)
{
	struct req_data *req;

#if 0
	DPRINTF(DBG_COMPLETE, ("%s() Enter\n", __func__));
#endif

	DBG_ASSERT(r->handle != NULL);

	if((req = dequeue_complete()) == NULL)
	{
		DBG_ASSERT(req != NULL);
		return(-1);
	}

	DPRINTF(DBG_COMPLETE, ("%s() req 0x%08x copy 0x%08x => 0x%08x 0x%08x bytes\n",
		__func__, (unsigned int) req, (unsigned int) req->br,
		(unsigned int) req->req_buffer, req->req_length));

#if 0
	DPRINTF(DBG_DRIVER, ("%s() data 0x%08x\n",
		__func__, (unsigned int) req->data));
	DPRINTF(DBG_DRIVER, ("%s() request 0x%08x\n",
		__func__, (unsigned int) req->br->request));
	DPRINTF(DBG_DRIVER, ("%s() pDataBuffer 0x%08x\n",
		__func__, (unsigned int) req->br->request->pDataBuffer));
#endif

	dump_data(req->data, req->req_length);

	if(copy_to_user(req->req_buffer, req->data, req->req_length))
	{
		DPRINTF(DBG_ERROR,
			("%s() FAILED to copy to user.\n", __func__));
	}

	hifFreeDeviceRequest(req->br->request);

	return(-1);
}

static int
usdio_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = 0;
	struct usdio_req ureq;
	struct usdio_req *r = &ureq;
	SDIO_STATUS status;

	DPRINTF(DBG_IOCTL, ("%s() Enter\n", __func__));

	if(cmd != USDIO_IOCTL_CMD)
	{
		DPRINTF(DBG_ERROR, ("%s() Command 0x%08x != 0x%08x (fixed)\n",
			__func__, cmd, USDIO_IOCTL_CMD));

		return(-EOPNOTSUPP);
	}

	if(copy_from_user(r, (void *) arg, sizeof(struct usdio_req)))
	{
		DPRINTF(DBG_ERROR, ("%s() Copy from user failed.\n", __func__));
		return(-EFAULT);
	}

	switch(r->cmd)
	{
		case USDIO_UMASK_IRQ:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_UMASK_IRQ.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			/* For HIFUnMaskInterrupt(HIF_DEVICE *device) */
			/* Need handle from device->handle */
			/* Register the IRQ Handler */
			SDDEVICE_SET_IRQ_HANDLER(s_handle, hifIRQHandler, s_cb);
			/* Unmask our function IRQ */
			status = SDLIB_IssueConfig(s_handle, SDCONFIG_FUNC_UNMASK_IRQ,
				                       NULL, 0);
			DBG_ASSERT(SDIO_SUCCESS(status));
			break;
		case USDIO_MASK_IRQ:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_MASK_IRQ.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			/* For HIFMaskInterrupt(HIF_DEVICE *device) */
			/* Need handle from device->handle */
			/* Mask our function IRQ */
			status = SDLIB_IssueConfig(s_handle, SDCONFIG_FUNC_MASK_IRQ,
				                       NULL, 0);
			DBG_ASSERT(SDIO_SUCCESS(status));
			/* Unregister the IRQ Handler */
			SDDEVICE_SET_IRQ_HANDLER(s_handle, NULL, NULL);
			break;
		case USDIO_ACK_IRQ:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_ACK_IRQ.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			/* For HIFUnMaskInterrupt(HIF_DEVICE *device) */
			/* Need handle from device->handle */
			/* Acknowledge our function IRQ */
			status = SDLIB_IssueConfig(s_handle, SDCONFIG_FUNC_ACK_IRQ,
				                       NULL, 0);
			DBG_ASSERT(SDIO_SUCCESS(status));
			break;
		case USDIO_INIT:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_INIT.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			r->handle = s_handle;
			do_init_ioctl(s_cb, r);
			break;
		case USDIO_UNINIT:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_UNINIT.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			r->handle = s_handle;
			do_uninit_ioctl(s_cb, r);
			break;
		case USDIO_READ:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_READ.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			r->handle = s_handle;
			do_rw_ioctl(s_cb, r);
			break;
		case USDIO_WRITE:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_WRITE.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			r->handle = s_handle;
			do_rw_ioctl(s_cb, r);
			break;
		case USDIO_COMPLETE:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_COMPLETE.\n", __func__));
			/* Make sure we are open */
			if(s_cb == NULL)
			{
				ret = -EFAULT;
				break;
			}
			r->handle = s_handle;
			do_complete_ioctl(s_cb, r);
			break;
		case USDIO_DBG:
			DPRINTF(DBG_IOCTL_OP, ("%s() USDIO_DBG old 0x%08x new 0x%08x.\n",
				__func__, g_debug_flags, r->u.dbg.flags));
			g_debug_flags = r->u.dbg.flags;
			break;
		default:
			DPRINTF(DBG_ERROR|DBG_IOCTL_OP,
				("%s() Sub command 0x%08x not supported.\n",
				__func__, r->cmd));
			ret = -EOPNOTSUPP;
	}

	return(ret);
}

/* Define which file operations are supported */
struct file_operations usdio_fops =
{
	.owner		=	THIS_MODULE,
	.llseek		=	NULL,
	.read		=	usdio_read,
	.write		=	usdio_write,
	.readdir	=	NULL,
	.poll		=	usdio_poll,
	.ioctl		=	usdio_ioctl,
	.mmap		=	NULL,
	.open		=	usdio_open,
	.flush		=	NULL,
	.release	=	usdio_release,
	.fsync		=	NULL,
	.fasync		=	NULL,
	.lock		=	NULL
};

module_init(usdio_init_module);
module_exit(usdio_cleanup_module);
MODULE_AUTHOR("Atheros Communications Inc. - Mats Aretun");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("User Space SDIO Interface");
