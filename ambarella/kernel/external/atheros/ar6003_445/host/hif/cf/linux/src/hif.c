/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
 *
 * This file contains the routines handling the interaction with the SDIO
 * driver
 */

#include "hif_internal.h"

/* ------ Static Variables ------ */

/* ------ Global Variable Declarations ------- */
CF_PNP_INFO Ids[] = {
    {
        .CF_ManufacturerID = MANUFACTURER_ID_BASE | 0xB,  
        .CF_ManufacturerCode = MANUFACTURER_CODE,
        .CF_FunctionClass = FUNCTION_CLASS,
        .CF_FunctionNo = 1
    },
    {
        .CF_ManufacturerID = MANUFACTURER_ID_BASE | 0xA,  
        .CF_ManufacturerCode = MANUFACTURER_CODE,
        .CF_FunctionClass = FUNCTION_CLASS,
        .CF_FunctionNo = 1
    },
    {
        .CF_ManufacturerID = MANUFACTURER_ID_BASE | 0x9,  
        .CF_ManufacturerCode = MANUFACTURER_CODE,
        .CF_FunctionClass = FUNCTION_CLASS,
        .CF_FunctionNo = 1
    },
    {
        .CF_ManufacturerID = MANUFACTURER_ID_BASE | 0x8,  
        .CF_ManufacturerCode = MANUFACTURER_CODE,
        .CF_FunctionClass = FUNCTION_CLASS,
        .CF_FunctionNo = 1
    },
    {
    }                      //list is null termintaed
};

TARGET_FUNCTION_CONTEXT FunctionContext = {
    .function.pName      = "cf_wlan",
    .function.MaxDevices = 1,
    .function.NumDevices = 0,
    .function.pIds       = Ids,
    .function.pProbe     = hifDeviceInserted, 
    .function.pRemove    = hifDeviceRemoved,
/*
    .function.pSuspend   = NULL,
    .function.pResume    = NULL,
    .function.pWake      = NULL,
*/
    .function.pContext   = &FunctionContext,
};

HIF_DEVICE hifDevice[HIF_MAX_DEVICES];
HTC_CALLBACKS htcCallbacks; 
BUS_REQUEST busRequest[BUS_REQUEST_MAX_NUM];
A_MUTEX_T lock;
#ifdef DEBUG
extern A_UINT32 debughif;
extern A_UINT32 debugzonelevel;
#define ATH_DEBUG_ERROR 1  
#define ATH_DEBUG_WARN  2  
#define ATH_DEBUG_TRACE 3  
#define _AR_DEBUG_PRINTX_ARG(arg...) arg 
#define AR_DEBUG_PRINTF(lvl, args)\
    {if (lvl <= debughif)\
        A_PRINTF(KERN_ALERT _AR_DEBUG_PRINTX_ARG args);\
    }
#else
#define AR_DEBUG_PRINTF(lvl, args)
#endif


/* ------ Functions ------ */

int
HIFInit(HTC_CALLBACKS *callbacks)
{
    CF_STATUS status;

    /* Store the callback and event handlers */
    htcCallbacks.deviceInsertedHandler = callbacks->deviceInsertedHandler;
    htcCallbacks.deviceRemovedHandler = callbacks->deviceRemovedHandler;
    htcCallbacks.deviceSuspendHandler = callbacks->deviceSuspendHandler;
    htcCallbacks.deviceResumeHandler = callbacks->deviceResumeHandler;
    htcCallbacks.deviceWakeupHandler = callbacks->deviceWakeupHandler;
    htcCallbacks.rwCompletionHandler = callbacks->rwCompletionHandler;
	htcCallbacks.deviceInterruptDisabler = callbacks->deviceInterruptDisabler;
    htcCallbacks.deviceInterruptEnabler = callbacks->deviceInterruptEnabler;
    htcCallbacks.dsrHandler = callbacks->dsrHandler;

	A_MUTEX_INIT(&lock);

    /* Register with bus driver core */
    status = CF_RegisterFunction(&FunctionContext.function);
    A_ASSERT(CF_SUCCESS(status));

    return(0);
}

/*
Parameters:
type - can be of the basic or extended. For SDIO it maps to using a CMD52 or CMD53. For wince SDIO it maps to SD_CMD_IO_RW_DIRECT and SD_CMD_IO_RW_EXTENDED. For CF this paramter is unused.
dmode - byte or block basis. For wince SDIO driver, it maps to SD_IO_BYTE_MODE and SD_IO_BLOCK_MODE. This mode is unused for CF.
amode - type of addressing on the target (fixed/incremental). This maps to writing to a REGISTER or a FIFO for CF.
*/
A_STATUS 
HIFReadWrite(HIF_DEVICE *device, 
             A_UINT32 address, 
             A_UCHAR *buffer, 
             A_UINT32 length, 
             A_UINT32 request, 
             void *context) 
{
    CFREQUEST cfrequest;
    CF_STATUS status;
    A_UINT32  remainingLen;
    A_UINT32  curPos;
 
	AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Device: %p\n", device));

    A_MEMZERO(&cfrequest, sizeof(CFREQUEST));

    cfrequest.pDataBuffer = buffer;
	cfrequest.length = length;

    /* Changes made to compensate for the changes in htc. For all the mbox
       reads we are now passing the beginning of the address of the mailbox
       and the addressing mode is incremental - TODO */
    if ((request & HIF_READ) && HIF_IS_MBOX_ADDR(address)) {
        address += (HIF_MBOX_WIDTH - 1);
        request |= HIF_FIXED_ADDRESS;
        request &= ~HIF_INCREMENTAL_ADDRESS;
    }

	if (request & HIF_SYNCHRONOUS) {
		AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Execution mode: Synchronous\n"));
	} else if (request & HIF_ASYNCHRONOUS) {
		AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Execution mode:Asynchronous\n"));
	} else {
		AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
					("Invalid execution mode: 0x%08x\n", request));
		return A_ERROR;
	}

    if (request & HIF_WRITE) {
        cfrequest.Flags |= CFREQ_FLAGS_DATA_WRITE;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Direction: Write\n"));
    } else if (request & HIF_READ) {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Direction: Read\n"));
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                        ("Invalid direction: 0x%08x\n", request));
        return A_ERROR;
    }

    if (request & HIF_FIXED_ADDRESS) {
		cfrequest.Flags |= CFREQ_FLAGS_FIXED_ADDRESS;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Address mode: Fixed\n"));
    } else if (request & HIF_INCREMENTAL_ADDRESS) {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Address mode: Incremental\n"));
    } else {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                        ("Invalid address mode: 0x%08x\n", request));
        return A_ERROR;
    }

	/* We need to do multiple writes in the case of CF interface
	* as the MBOX size is limited to 256 bytes
	*/
	if ( (request & HIF_WRITE) && HIF_IS_MBOX_ADDR(address) ) {

		remainingLen = length;
#define BLK_SZ  (HIF_MBOX_WIDTH - 2)
        
        curPos = 0;

        while (remainingLen > BLK_SZ) {
		    cfrequest.address = address;
		    cfrequest.length = BLK_SZ;
			cfrequest.pDataBuffer = &buffer[curPos];

			status = CFDEVICE_CALL_REQUESTW_FUNC(device->handle, &cfrequest);

    		if (status != CF_STATUS_SUCCESS) {
        		AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("HIF Write failed\n"));
        		return status;
    		}

            curPos += BLK_SZ;
            remainingLen -= BLK_SZ;
		}

		cfrequest.pDataBuffer = &buffer[curPos];
        cfrequest.address = address + HIF_MBOX_WIDTH - remainingLen;
		cfrequest.length = remainingLen;

		status = CFDEVICE_CALL_REQUESTW_FUNC(device->handle, &cfrequest);

   		if (status != CF_STATUS_SUCCESS) {
        	AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("HIF Write failed\n"));
        	return status;
   		}
	}
	else if ( (request & HIF_READ) && HIF_IS_MBOX_ADDR(address) ) {
		// Read from an even address
		cfrequest.address = address - 1;
    	status = CFDEVICE_CALL_REQUESTW_FUNC(device->handle, &cfrequest);
	}
	else {
		cfrequest.address = address;
    	status = CFDEVICE_CALL_REQUESTB_FUNC(device->handle, &cfrequest);
	}

   	if (status == CF_STATUS_SUCCESS) {
		status = A_OK;
   	} else {
		status = A_ERROR;
	}

    return status;
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

        default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("Invalid configuration opcode: %d\n", opcode));
            return A_ERROR;
    }

    return A_OK;
}

void
HIFShutDownDevice(HIF_DEVICE *device)
{
    A_UINT32 count;
    CF_STATUS status = CF_STATUS_SUCCESS;
    
    /* Free the bus requests */
    for (count = 0; count < BUS_REQUEST_MAX_NUM; count ++) {
        busRequest[count].free = TRUE;
    }

    /* Unregister with bus driver core */
	if (device == NULL) {
    	status = CF_UnregisterFunction(&FunctionContext.function);
	}

    A_ASSERT(CF_SUCCESS(status));
}

void
hifISRHandler(void *context, A_BOOL *callDSR)
{
    A_STATUS status;
    HIF_DEVICE *device;

    device = (HIF_DEVICE *)context;
    AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Device: %p\n", device));
	status = htcCallbacks.deviceInterruptDisabler(device->htc_handle, callDSR);
    A_ASSERT(status == A_OK);
}

void
hifDSRHandler(unsigned long context)
{
    A_STATUS status;
    HIF_DEVICE *device;

    device = (HIF_DEVICE *)context;
    AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Device: %p\n", device));
    status = htcCallbacks.dsrHandler(device->htc_handle);
    A_ASSERT(status == A_OK);
}


static A_BOOL
hifDeviceInserted(CFFUNCTION *function, PCFDEVICE handle)
{
    A_UINT32 count;
    HIF_DEVICE *device;
    TARGET_FUNCTION_CONTEXT *functionContext;

    device = addHifDevice(handle);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Device: %p\n", device));
    functionContext =  (TARGET_FUNCTION_CONTEXT *)function->pContext;

    /* Allocate the bus requests to be used later */
    for (count = 0; count < BUS_REQUEST_MAX_NUM; count ++) {
        busRequest[count].free = TRUE;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, 
                        ("busRequest[%d].request = 0x%p, busRequest[%d].free = %d\n", count, &busRequest[count].request, count, busRequest[count].free));
    }

    /* Inform HTC */
    if ((htcCallbacks.deviceInsertedHandler((void *) device)) != A_OK) {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("Device rejected\n"));
        return FALSE;
    }

    return TRUE;
}

void
HIFAckInterrupt(HIF_DEVICE *device)
{
	htcCallbacks.deviceInterruptEnabler(device->htc_handle);
	return;
	/* enable interrupts */
	//enable_irq(device->handle->irq);
}

void
HIFUnMaskInterrupt(HIF_DEVICE *device)
{
	AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("HIFUnMaskInterrupt"));

    CFDEVICE_SET_IRQ_HANDLER(device->handle, hifISRHandler, hifDSRHandler, device);
}

void HIFMaskInterrupt(HIF_DEVICE *device)
{
	AR_DEBUG_PRINTF(ATH_DEBUG_TRACE, ("HIFMaskInterrupt"));

    CFDEVICE_SET_IRQ_HANDLER(device->handle, NULL, NULL, device);
}

static void
hifDeviceRemoved(CFFUNCTION *function, PCFDEVICE handle)
{
    A_STATUS status;
    HIF_DEVICE *device;

    device = getHifDevice(handle);
    status = htcCallbacks.deviceRemovedHandler(device->htc_handle, A_OK);
    delHifDevice(handle);
    A_ASSERT(status == A_OK);
}

static HIF_DEVICE *
addHifDevice(PCFDEVICE handle)
{
    hifDevice[0].handle = handle;
    return &hifDevice[0];
}

static HIF_DEVICE *
getHifDevice(PCFDEVICE handle)
{
    return &hifDevice[0];
}

static void
delHifDevice(PCFDEVICE handle)
{
    hifDevice[0].handle = NULL;
}

void HIFSetHandle(void *hif_handle, void *handle)
{
    HIF_DEVICE *device = (HIF_DEVICE *) hif_handle;
    
    device->htc_handle = handle;

    return;
}
