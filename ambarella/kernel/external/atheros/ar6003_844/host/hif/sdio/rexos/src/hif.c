/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 * This file contains the routines handling the interaction with the SDIO
 * driver
 */

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <htc.h>
#include <htc_api.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "sdcc_api.h"
#include "wlan_adp_oem.h"
#include "wlan_trp_oem.h"

/* Atheros platform include files */
#include "hif_internal.h"
#ifdef ATHR_EMULATION
#include "usdio.h"
#endif
#include "a_debug.h"
#include "qcom_htc.h"


/* Define this to deal with 32 bit buffer alignment requirements */
#define BUFFER_32BIT_ALIGNMENT_FIX
#undef NEW_CREDIT_COUNTER_PROCESSING

#ifdef ATHR_EMULATION
void sdcc_sdio_ack_intr(uint8 devfn);
#endif
static int  HifSdioIsrhandler(void * param);

/* ------ Constant and defines ------ */

#define MANUFACTURER_ID		(MANUFACTURER_AR6001_ID_BASE | 0xB)

/* These are also defined in htc_internal.h */
#define HTC_REG_REQUEST_LIST_SIZE          16
#define HTC_DATA_REQUEST_RING_BUFFER_SIZE  30 

/* ------ Static Variables ------ */

/* This can be a pointer (non NULL) to any data structure that the */
/* underlaying SDIO layer needs.                                   */
static int s_sdio_data = 0;
static int *s_hif_handle = &s_sdio_data;

/* The queue size is the sum of the register request and data request lists */
/* We need one extra to distinguish the empty from full case                */
#define CONTEXT_QUEUE_SIZE	\
	(HTC_DATA_REQUEST_RING_BUFFER_SIZE + HTC_REG_REQUEST_LIST_SIZE + 1)

static void *s_context_queue[CONTEXT_QUEUE_SIZE];
static int s_context_queue_head = CONTEXT_QUEUE_SIZE - 1;
static int s_context_queue_tail = CONTEXT_QUEUE_SIZE - 1;
static A_MUTEX_T s_context_queue_lock;

static HIF_DEVICE hifDevice[HIF_MAX_DEVICES];
static OSDRV_CALLBACKS osdrvCallbacks; 

/*****************************************************************************/
/**  Local functions                                                        **/
/*****************************************************************************/

static HIF_DEVICE *
addHifDevice(void *handle)
{
	A_ASSERT(handle != NULL);
	hifDevice[0].handle = handle;
	return &hifDevice[0];
}

static HIF_DEVICE *
getHifDevice(void *handle)
{
	A_ASSERT(handle != NULL);
	return &hifDevice[0];
}

static void
delHifDevice(void *handle)
{
	A_ASSERT(handle != NULL);
	hifDevice[0].handle = NULL;
}

/* To be called after a Read/Write completes */
static void 
hifRWCompletionHandler(void *request) 
{
	A_STATUS status;	/* A_OK or A_ERROR */
	void *context = request;
	HIF_DEVICE *device;
	
	DPRINTF(DBG_HIF, (DBGFMT "Enter\n", DBGARG));

	status = A_OK;
	device = getHifDevice(s_hif_handle);
	status = device->htcCallbacks.rwCompletionHandler(context, status);

	if(status != A_OK)
	{
		DPRINTF(DBG_HIF, (DBGFMT "rwCompletionHandler() failed.\n", DBGARG));
		htc_set_failure(FAIL_NONFATAL);
	}

	DPRINTF(DBG_HIF, (DBGFMT "Exit\n", DBGARG));

	return;
}

/* To be called for an ISR */
static void
hifIRQHandler(void *context)
{
	A_STATUS status;
	HIF_DEVICE *device;

	device = (HIF_DEVICE *) context;

	DPRINTF(DBG_HIF,
		(DBGFMT "Enter - device 0x%08x\n", DBGARG, (unsigned int) device));

	status = device->htcCallbacks.dsrHandler(device->htcCallbacks.context);

	if(status != A_OK)
	{
		DPRINTF(DBG_HIF, (DBGFMT "dsrHandler() failed.\n", DBGARG));
		htc_set_failure(FAIL_NONFATAL);
	}

	DPRINTF(DBG_HIF, (DBGFMT "Exit\n", DBGARG));
}

static A_BOOL
hifDeviceInserted(void *handle)
{
	HIF_DEVICE *device;

	DPRINTF(DBG_HIF, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(handle != NULL);

	device = addHifDevice(handle);

	DPRINTF(DBG_HIF,
		(DBGFMT "Device: 0x%08x\n", DBGARG, (unsigned int) device));

	/* Do SDIO initialization */
	if(sdcc_sdio_init(MANUFACTURER_CODE) != 0)
	{
		DPRINTF(DBG_HIF, (DBGFMT "Failed to initialize SDIO\n", DBGARG));
		return FALSE;
	}

    /* 
     * Adding a wait of around a second before we issue the very first 
     * command to dragon. During the process of loading/unloading the 
     * driver repeatedly it was observed that we get a data timeout
     * while accessing function 1 registers in the chip. The theory at
     * this point is that some initialization delay in dragon is 
     * causing the SDIO state in dragon core to be not ready even after
     * the ready bit indicates that function 1 is ready. Accomodating
     * for this behavior by adding some delay in the driver before it
     * issues the first command after switching on dragon. Need to 
     * investigate this a bit more - TODO
     */
    A_MDELAY(1000);
    
	/* Inform HTC */
	if ((osdrvCallbacks.deviceInsertedHandler(osdrvCallbacks.context, device)) != A_OK)
	{
		DPRINTF(DBG_HIF, (DBGFMT "Device rejected\n", DBGARG));
		return FALSE;
	}

	DPRINTF(DBG_HIF, (DBGFMT "Exit\n", DBGARG));

	return TRUE;
}

static void
hifDeviceRemoved(void *handle)
{
	A_STATUS status;
	HIF_DEVICE *device;

	DPRINTF(DBG_HIF, (DBGFMT "Enter\n", DBGARG));

	A_ASSERT(handle != NULL);

	device = getHifDevice(handle);
	status = osdrvCallbacks.deviceRemovedHandler(device->claimedContext, device);

	if(status != A_OK)
	{
		DPRINTF(DBG_HIF, (DBGFMT "deviceRemovedHandler() failed.\n", DBGARG));
		htc_set_failure(FAIL_NONFATAL);
	}
	delHifDevice(handle);

	DPRINTF(DBG_HIF, (DBGFMT "Exit\n", DBGARG));
}

/*****************************************************************************/
/**  Global functions                                                       **/
/*****************************************************************************/

void enqueue_context(void *context)
{
	int position;

	/* We need mutual exclusion */
	A_MUTEX_LOCK(&s_context_queue_lock);

	/* Use temp variable so we don't have to reverse calculation if full */
	position = (s_context_queue_head + 1) % CONTEXT_QUEUE_SIZE;

	/* Is there room in the queue? The full case not handled since the */
	/* size of the queue should be the aggregate sum of all potential  */
	/* outstanding requests.                                           */
	if(position != s_context_queue_tail)
	{
		/* Save it away */
		s_context_queue[position] = context;

		/* This is the new position */
		s_context_queue_head = position;

		DPRINTF(DBG_HIF,
			(DBGFMT "Enqueue     - Head %d Tail %d Pos %d\n", DBGARG,
			s_context_queue_head, s_context_queue_tail, position));
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Queue full  - Head %d Tail %d Pos %d\n", DBGARG,
			s_context_queue_head, s_context_queue_tail, position));
	}

	/* We are done */
	A_MUTEX_UNLOCK(&s_context_queue_lock);
}



void *dequeue_context(void)
{
	void *val;
	int position = -1;

	/* We need mutual exclusion */
	A_MUTEX_LOCK(&s_context_queue_lock);

	/* Check if we have anyting in the queue */
	if(s_context_queue_head == s_context_queue_tail)
	{
		/* Looks like it was empty */
		val = NULL;

		DPRINTF(DBG_HIF,
			(DBGFMT "Queue empty - Head %d Tail %d Pos %d\n", DBGARG,
			s_context_queue_head, s_context_queue_tail, position));
	}
	else
	{
		position = (s_context_queue_tail + 1) % CONTEXT_QUEUE_SIZE;

		DPRINTF(DBG_HIF,
			(DBGFMT "Dequeue     - Head %d Tail %d Pos %d\n", DBGARG,
			s_context_queue_head, s_context_queue_tail, position));

		/* Dequeue it */
		val = s_context_queue[position];

		/* Point to next guy */
		s_context_queue_tail = position;
	}

	/* We are done */
	A_MUTEX_UNLOCK(&s_context_queue_lock);

	/* Return it */
	return(val);
}

A_STATUS
HIFInit(OSDRV_CALLBACKS *callbacks)
{
	A_ASSERT(callbacks != NULL);

	/* Store the callback and event handlers */
	osdrvCallbacks = *callbacks;
	
    A_MUTEX_INIT(&s_context_queue_lock);
    return (A_OK);
}

A_STATUS 
HIFReadWrite(HIF_DEVICE *device, 
             A_UINT32 address, 
             A_UCHAR *buffer, 
             A_UINT32 length, 
             A_UINT32 request, 
             void *context) 
{
	A_STATUS status;
	int len;		/* Number of bytes  */
	int count;		/* Number of blocks */
	int iolen;		/* Number of io items (i.e. blocks or bytes) */
#ifdef ATHR_EMULATION
	unsigned int flags = 0;
#endif
	int block_mode;
	int op_code;
	unsigned int process_flags = 0;
#define PROCESS_CREDIT_COUNTER	0x00000001
#ifdef BUFFER_32BIT_ALIGNMENT_FIX
#define PROCESS_32BIT_ALIGNMENT	0x00000002
#define ATH_RW_MAX_BYTES    256
	static uint32 rw_byte_buffer[ATH_RW_MAX_BYTES/sizeof(uint32)];
#endif
#ifndef NEW_CREDIT_COUNTER_PROCESSING
	int loop;
#endif
	A_UCHAR *bp = buffer;

	A_ASSERT(device != NULL);
	A_ASSERT(device->handle != NULL);
 
	DPRINTF(DBG_HIF,
		(DBGFMT "Enter - device 0x%08x\n", DBGARG, (unsigned int) device));

	/* Print a warning for non 32 bit aligned buffers */
	if((unsigned int) buffer & 0x03)
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "BUFFER_WARNING 0x%08x is not 32 bit aligned.\n", DBGARG,
			(unsigned int) buffer));
	}

	if (request & HIF_SYNCHRONOUS)
	{
#ifdef ATHR_EMULATION
		flags |= RW_SYNCHRONOUS;
#endif
	}
	else if (request & HIF_ASYNCHRONOUS)
	{
#ifdef ATHR_EMULATION
		flags |= RW_ASYNCHRONOUS;
#endif
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Invalid execution mode: %d\n", DBGARG, request & HIF_EMODE_MASK));
		return A_ERROR;
	}

	if (request & HIF_EXTENDED_IO)
	{
#ifdef ATHR_EMULATION
		flags |= RW_EXTENDED_IO;
#endif
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Invalid command type: %d\n", DBGARG, request & HIF_TYPE_MASK));
		return A_ERROR;
	}

	if (request & HIF_BLOCK_BASIS)
	{
		/* HTC has adjusted length so it is a multiple of the block size */
		if(length <= HIF_MBOX_BLOCK_SIZE)
		{
			len = length;
			iolen = count = 1;
		}
		else
		{
			count = length / HIF_MBOX_BLOCK_SIZE;
			len = count * HIF_MBOX_BLOCK_SIZE;
			iolen = count;
		}

#ifdef ATHR_EMULATION
		flags |= RW_BLOCK_BASIS;
#else
		block_mode = 1;
#endif
	}
	else if (request & HIF_BYTE_BASIS)
	{
		iolen = len = length;
		count = 1;
#ifdef ATHR_EMULATION
		flags |= RW_BYTE_BASIS;
#else
		block_mode = 0;
#endif
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Invalid data mode: %d\n", DBGARG, request & HIF_DMODE_MASK));
		return A_ERROR;
	}

	/*********************************************************************/
	/** This section of code converts byte transfers to block transfers **/
	/*********************************************************************/
	if (request & HIF_BYTE_BASIS)
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "WARNING WARNING - Byte mode\n", DBGARG));

#ifdef BUFFER_32BIT_ALIGNMENT_FIX
		A_ASSERT(ATH_RW_MAX_BYTES >= len);
		bp = (uint8 *) rw_byte_buffer;
		/* A_MEMZERO(bp, ATH_RW_MAX_BYTES); */
#endif

		if ( address >= 0x420 && address < 0x460 )
		{
			DPRINTF(DBG_HIF, (DBGFMT "Credit counter access\n", DBGARG));

			process_flags |= PROCESS_CREDIT_COUNTER;
			len = 4;
			count = 1;
			iolen = 4;
		}
		else
		{
			/* Need to calculate number of blocks and new length */
			count = ((length + (HIF_MBOX0_BLOCK_SIZE - 1)) / HIF_MBOX0_BLOCK_SIZE);
			iolen = len = count * HIF_MBOX0_BLOCK_SIZE;
		}
		
		if(request & HIF_WRITE)
		{
			DPRINTF(DBG_HIF, (DBGFMT "Force incremental\n", DBGARG));

			/* We also must make sure that we use incremental addressing so */
			/* we don't end up writing pad bytes to the same address.       */
			request = HIF_INCREMENTAL_ADDRESS | (request & ~HIF_AMODE_MASK);
#ifdef BUFFER_32BIT_ALIGNMENT_FIX
			memcpy(bp, buffer, len);
#endif
		}

		if(request & HIF_READ)
		{
#ifdef BUFFER_32BIT_ALIGNMENT_FIX
			process_flags |= PROCESS_32BIT_ALIGNMENT;
#endif
			if (!((address >= HIF_MBOX_START_ADDR(0)) && 
				(address <= HIF_MBOX_END_ADDR(3))))
			{
				DPRINTF(DBG_HIF, (DBGFMT "Force incremental\n", DBGARG));

				request = HIF_INCREMENTAL_ADDRESS | (request & ~HIF_AMODE_MASK);
			}
		}

#ifdef NEW_CREDIT_COUNTER_PROCESSING
		if(process_flags & PROCESS_CREDIT_COUNTER)
		{
			request = HIF_FIXED_ADDRESS | (request & ~HIF_AMODE_MASK);
		}
#endif

		DPRINTF(DBG_HIF,
			(DBGFMT "length %d len %d count %d iolen %d\n", DBGARG,
			length, len, count, iolen));
	}

	/* Set the right block mode */
	sdcc_sdio_set(0, SD_SET_BLOCK_MODE, &block_mode);

	if (request & HIF_WRITE)
	{
		if ((address >= HIF_MBOX_START_ADDR(0)) && 
			(address <= HIF_MBOX_END_ADDR(3)))
		{
            DPRINTF(DBG_HIF,
                    (DBGFMT "new address 0x%08x old address 0x%08x length %d\n",
                    DBGARG, address + HIF_MBOX_WIDTH - len, address, length));
            
			/* 
			 * Mailbox write. Adjust the address so that the last byte 
			 * falls on the EOM address.
			 */
			address = address + HIF_MBOX_WIDTH - len;
		}

#ifdef ATHR_EMULATION
		flags |= RW_WRITE;
#endif
	}
	else if (request & HIF_READ)
	{
#ifdef ATHR_EMULATION
		flags |= RW_READ;
#endif
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Invalid direction: %d\n", DBGARG, request & HIF_DIR_MASK));
		return A_ERROR;
	}

	if (request & HIF_FIXED_ADDRESS)
	{
#ifdef ATHR_EMULATION
		flags |= RW_FIXED_ADDRESS;
#else
		op_code = 0;
#endif
	}
	else if (request & HIF_INCREMENTAL_ADDRESS)
	{
#ifdef ATHR_EMULATION
		flags |= RW_INCREMENTAL_ADDRESS;
#else
		op_code = 1;
#endif
	}
	else
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Invalid address mode: %d\n", DBGARG, request & HIF_AMODE_MASK));
		return A_ERROR;
	}

	/* Set the right op code */
	sdcc_sdio_set(0, SD_SET_OP_CODE, &op_code);

	/* If we have a context, store it in our FIFO completion queue */
	if(context != NULL)
	{
		if(request & HIF_SYNCHRONOUS)
		{
			DPRINTF(DBG_HIF,
				(DBGFMT "WARNING WARNING context in sync mode.\n", DBGARG));
		}
		enqueue_context(context);
	}

	/* Send the command out */
	DPRINTF(DBG_HIF|DBG_INFO, 
		(DBGFMT "%s %s %s @ 0x%08x (%s) %d (%d) bytes %d blks %d iolen.\n",
		DBGARG,
		request & HIF_BYTE_BASIS ? "Byte": "Block",
		request & HIF_SYNCHRONOUS ? "Sync" : "Async",
		request & HIF_READ ? "Read" : "Write",
		address,
		request & HIF_FIXED_ADDRESS ? "Fix" : "Inc",
		length, len, count, iolen));

	/* If byte mode, we transmits chunks of data whose length are
	 * power of 2. e.g If length=24, There will be 2 SDIO transfers
	 * of size 16 and 8.
	 */

	if(request & HIF_BYTE_BASIS)
	{
		A_UCHAR *tbp = bp;
		A_UINT32 taddress = address;
		int tlen = (iolen + 0x3) & ~0x3;	/* Pad to 4 byte boundry */
		int tiolen;
		int x;

#ifdef NEW_CREDIT_COUNTER_PROCESSING
		if(process_flags & PROCESS_CREDIT_COUNTER)
		{
			tlen = length;
#if 0
			DPRINTF(DBG_HIF|DBG_INFO, 
				(DBGFMT "PROCESS_CREDIT_COUNTER %d credits.\n", DBGARG, tlen));
#endif
		}
#endif

		while(tlen > 0)
		{
			x = tlen;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			tiolen = x ^ (x >> 1);

#ifdef ATHR_EMULATION
			status = (request & HIF_WRITE) ?
				sdcc_sdio_bwrite(1, taddress, tiolen, tbp, flags) :
				sdcc_sdio_bread(1, taddress, tiolen, tbp, flags);
#else
			status = (request & HIF_WRITE) ?
				sdcc_sdio_write(1, taddress, tiolen, tbp) :
				sdcc_sdio_read(1, taddress, tiolen, tbp);
#endif

			if(request & HIF_INCREMENTAL_ADDRESS)
			{
				taddress = taddress + tiolen;
			}

            DPRINTF(DBG_HIF|DBG_INFO, 
                    (DBGFMT "Write @ 0x%08x 0x%08x %d bytes 0x%08x buffer %d remaining %d flags %d x.\n",
                     DBGARG, address, taddress, tiolen, (unsigned int) tbp, tlen, flags, x));
            
			tbp = tbp + tiolen;
			tlen &= ~tiolen;
		}
	}
	else
	{
#ifdef ATHR_EMULATION
		status = (request & HIF_READ) ?
			sdcc_sdio_bread(1, address, iolen, bp, flags) :
			sdcc_sdio_bwrite(1, address, iolen, bp, flags);
#else
		status = (request & HIF_READ) ?
			sdcc_sdio_read(1, address, iolen, bp) :
			sdcc_sdio_write(1, address, iolen, bp);
#endif
	}

#ifdef BUFFER_32BIT_ALIGNMENT_FIX
	if(process_flags & PROCESS_32BIT_ALIGNMENT)
	{
		/* Copy in original buffer */
		memcpy(buffer, bp, length);
	}
#endif

#if 0
#ifndef NEW_CREDIT_COUNTER_PROCESSING
	/* This can only be true for byte mode */
	if((process_flags & PROCESS_CREDIT_COUNTER) && length > 1)
	{
		DPRINTF(DBG_HIF,
			(DBGFMT "Process credit counters.\n", DBGARG));

		for ( loop = 0; loop < length - 1; loop++)
		{
#ifdef ATHR_EMULATION
			status = (request & HIF_READ) ?
				sdcc_sdio_bread(1, address, HIF_MBOX0_BLOCK_SIZE, bp, flags) :
				sdcc_sdio_bwrite(1, address, HIF_MBOX0_BLOCK_SIZE, bp, flags);
#else
			status = (request & HIF_READ) ?
				sdcc_sdio_read (1, address, HIF_MBOX0_BLOCK_SIZE, bp) :
				sdcc_sdio_write(1, address, HIF_MBOX0_BLOCK_SIZE, bp) ;
#endif

			if ( status != 0)
			{
				break;
			}

#ifdef BUFFER_32BIT_ALIGNMENT_FIX
			if(process_flags & PROCESS_32BIT_ALIGNMENT)
			{
				buffer++;
				*buffer = bp[0];
			}
#endif
		}
	}
#endif
#endif

	/* Generate a RW complete interrupt if we are asyncronous */
	if(request & HIF_ASYNCHRONOUS)
	{
		/* Our SDIO calls are syncronous so call our rw */
		/* completion handler directly                  */
		hifRWCompletionHandler(dequeue_context());
	}

	DPRINTF(DBG_HIF,
		(DBGFMT "Exit status \"%s\"\n", DBGARG,
		status != 0 ? "A_ERROR" : "A_OK"));

	/* Return status A_OK or A_ERROR */
	return(status != 0 ? A_ERROR : A_OK);
}

A_STATUS
HIFConfigureDevice(HIF_DEVICE *device, HIF_DEVICE_CONFIG_OPCODE opcode, 
                   void *config, A_UINT32 configLen)
{
    A_UINT32 count;

    switch(opcode) {
        case HIF_DEVICE_GET_MBOX_BLOCK_SIZE:
            ((A_UINT32 *)config)[0] = HIF_MBOX0_BLOCK_SIZE;
            ((A_UINT32 *)config)[1] = HIF_MBOX1_BLOCK_SIZE;
            ((A_UINT32 *)config)[2] = HIF_MBOX2_BLOCK_SIZE;
            ((A_UINT32 *)config)[3] = HIF_MBOX3_BLOCK_SIZE;
            break;

        case HIF_DEVICE_GET_MBOX_ADDR:
            for (count = 0; count < 4; count ++) {
                ((A_UINT32 *)config)[count] = HIF_MBOX_START_ADDR(count);
            }
            break;
        case HIF_DEVICE_GET_IRQ_PROC_MODE:
                /* the SDIO stack allows the interrupts to be processed either way, ASYNC or SYNC */
            *((HIF_DEVICE_IRQ_PROCESSING_MODE *)config) = HIF_DEVICE_IRQ_SYNC_ONLY;
            break;
        default:
            DPRINTF(DBG_HIF, (DBGFMT "Invalid configuration opcode: %d\n", DBGARG, opcode));
            return A_ERROR;
    }

    return A_OK;
}

void
HIFShutDownDevice(HIF_DEVICE *device)
{
	/* Close controller */
	sdcc_close(0);

	return;
}

void
HIFAckInterrupt(HIF_DEVICE *device)
{
	DPRINTF(DBG_HIF,
		(DBGFMT "Enter - device 0x%08x\n", DBGARG, (unsigned int) device));

	A_ASSERT(device != NULL);
	A_ASSERT(device->handle != NULL);

	/* "Acknowledge" our function IRQ to reenable interrupts */
#ifdef ATHR_EMULATION
	sdcc_sdio_ack_intr(1);
#else
	(sdcc_enable_int(HWIO_MCI_INT_MASKn_MASK22_BMSK));
#endif

	return;
}

void
HIFUnMaskInterrupt(HIF_DEVICE *device)
{
	DPRINTF(DBG_HIF,
		(DBGFMT "Enter - device 0x%08x\n", DBGARG, (unsigned int) device));

	A_ASSERT(device != NULL);
	A_ASSERT(device->handle != NULL);

	/* Register the IRQ Handler */ /* Unmask our function IRQ */
	sdcc_sdio_connect_intr(1, (void *) HifSdioIsrhandler, NULL);

	return;
}

void HIFMaskInterrupt(HIF_DEVICE *device)
{
	DPRINTF(DBG_HIF,
		(DBGFMT "Enter - device 0x%08x\n", DBGARG, (unsigned int) device));

	A_ASSERT(device != NULL);
	A_ASSERT(device->handle != NULL);

	/* Mask our function IRQ */ /* Unregister the IRQ Handler */
	sdcc_sdio_disconnect_intr(1);

	return;
}

/*****************************************************************************/
/**  External event functions                                               **/
/*****************************************************************************/

int HIFHandleInserted(void)
{
	/* Run the device inserted handler for our device */
	if(hifDeviceInserted(s_hif_handle) != TRUE)
	{
		DPRINTF(DBG_HIF, (DBGFMT "Failed to initialize device.\n", DBGARG));
		htc_set_failure(FAIL_FATAL);
		return(-1);
	}

	return(0);
}

void HIFHandleRemoved(void)
{
	/* Run the device removed handler for our device */
	hifDeviceRemoved(s_hif_handle);
}

void HIFHandleIRQ(void)
{
	/* Run the IRQ for our device */
	hifIRQHandler(&hifDevice[0]);
}

void HIFHandleRW(void)
{
	/* Run the RW complete for this device */
	hifRWCompletionHandler(dequeue_context());
}

int HifSdioIsrhandler(void * param)
{

#ifndef ATHR_EMULATION
	/* Disable further interrupts until we say otherwise */
	(sdcc_disable_int(HWIO_MCI_INT_MASKn_MASK22_BMSK));
#endif

	/*
	** Just post HTC_HIFDSR_SIG to HTC task
	*/

	htc_task_signal(HTC_HIFDSR_SIG);

	/* Returning 1 arbitrary for now */
	return 1;
} /* HifSdioIsrhandler() */

void HIFClaimDevice(HIF_DEVICE  *device, void *context)
{
    device->claimedContext = context;   
}

void HIFReleaseDevice(HIF_DEVICE  *device)
{
    device->claimedContext = NULL;    
}

A_STATUS HIFAttachHTC(HIF_DEVICE *device, HTC_CALLBACKS *callbacks)
{
    if (device->htcCallbacks.context != NULL) {
        /* already in use! */
        return A_ERROR;    
    }
    device->htcCallbacks = *callbacks; 
    return A_OK;
}

void HIFDetachHTC(HIF_DEVICE *device)
{
    A_MEMZERO(&device->htcCallbacks,sizeof(device->htcCallbacks));
}
