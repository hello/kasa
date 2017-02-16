//------------------------------------------------------------------------------
// <copyright file="hif.c" company="Atheros">
//    Copyright (c) 2008 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
 
 
 /* 
 *   This driver uses the Atheros SPI stack. The SPI stack framework is based on the SDIO bus driver
 * source code to support the SPI bus for the following reasons:
 *    
 *    1). There are very few SPI bus implementations (i.e. bus drivers).
 *    2). SPI bus implementations on some linux distributions are designed for slow peripherals
 *        such as SPI EEPROMS, slow ADC/DACs where performance was not a design requirement.
 *    3). The spi stack  provides a model for abstracting host controller drivers. This
 *        allows the HIF layer to remain largely intact between customer implementations 
 *        (making it much easier to support) and abstract enough to require only that the customer
 *        create/port the SPI host driver layer.
 *    4). The spi stack provides a packet-based I/O model with synchronous and asynchronous
 *        request processing (with recursion protection), with provisions for 
 *        queueing/overlapping I/O requests.  There is an I/O dispatch model between platform ISRs 
 *        and driver threads. This can be re-used for SPI.
 *    5). The spi stack provides out-of-band signaling mechanism (SPI interrupts) that maps well
 *        with SPI interrupts.
 * 
 *    
 *       -----------------------
 *       |  WMI and HTC layers |
 *       -----------------------
 *       ------------------------
 *       | AR6002 SPI HIF layer |
 *       ------------------------
 *       ------------------------
 *       | SPI Bus Core         |
 *       | SPI Host Driver      |
 *       ------------------------
 * 
 *    The HIF layer issues bus requests and host controller configuration commands through the
 *    bus driver. The SPI bus requst structure is used to pass SPI request parameters to the 
 *    SPI host driver (through the SPI bus driver).
 *    
 *    The SPI bus driver manages bus request queuing and provides methods for synchronous and
 *    asynchronous processing.  This relieves much of the HIF code and SPI host driver code from 
 *    providing this infrastructure.
 * 
 * 
 * */
 
 
#include "hif_internal.h"
#define ATH_MODULE_NAME hif
#include "a_debug.h"
#include <htc.h>


#define ATH_DEBUG_IRQ ATH_DEBUG_MAKE_MODULE_MASK(0)

#ifdef DEBUG

static ATH_DEBUG_MASK_DESCRIPTION spi2_hif_debug_desc[] = {
    { ATH_DEBUG_IRQ , "SPI IRQ Processing"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(hif,
                                 "hif",
                                 "(SPI2) Host Interconnect Framework",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 ATH_DEBUG_DESCRIPTION_COUNT(spi2_hif_debug_desc),
                                 spi2_hif_debug_desc);

#endif

#define A_SWAPBYTES16(_x) \
    ((A_UINT16)( \
        (((A_UINT16)(_x) & (A_UINT16)0x00ffU) << 8) | \
        (((A_UINT16)(_x) & (A_UINT16)0xff00U) >> 8) ))
        
/* ------ Static Variables ------ */

/* ------ Global Variable Declarations ------- */
SD_PNP_INFO g_Ids[2];

SDFUNCTION g_Function;


OSDRV_CALLBACKS osdrvCallbacks;

extern A_UINT32 busspeedlow;
extern A_UINT32 debughif;
static void hifCleanupDevice(HIF_DEVICE *device);
static A_STATUS hifGetPendingEvents(HIF_DEVICE              *device, 
                                    HIF_PENDING_EVENTS_INFO *pEvents,
                                    void                    *AsyncContext);
static A_STATUS hifGetLookAhead(HIF_DEVICE *device,  HIF_PENDING_EVENTS_INFO *pEvents);
static A_STATUS hifMaskUnmaskRecvMsg(HIF_DEVICE              *device,
                                     A_BOOL                  Mask,
                                     void                    *AsyncContext);
                              
#define LOOK_AHEAD_LENGTH       4
#define MAX_BYTES_AVAIL_POLL    10
#define SPI_RDBUF_WATER_MARK_LEVEL    1500 /* roughly a max packet */
                        
static A_STATUS PowerChangeNotify(HIF_DEVICE *device, HIF_DEVICE_POWER_CHANGE_TYPE PowerChange);
/* ------ Functions ------ */
A_STATUS
HIFInit(OSDRV_CALLBACKS *callbacks)
{
    SDIO_STATUS status;
    AR_DEBUG_ASSERT(callbacks != NULL);

    A_REGISTER_MODULE_DEBUG_INFO(hif);

    A_MEMZERO(g_Ids, sizeof(g_Ids));
    A_MEMZERO(&g_Function,sizeof(g_Function));
    
    g_Ids[0].CardFlags = CARD_RAW;
    g_Function.Version    = CT_SDIO_STACK_VERSION_CODE;
    g_Function.pName      = "spi_wlan";
    g_Function.MaxDevices = 1;
    g_Function.NumDevices = 0;
    g_Function.pIds       = g_Ids;
    g_Function.pProbe     = hifDeviceInserted; 
    g_Function.pRemove    = hifDeviceRemoved;
    g_Function.pSuspend   = NULL;
    g_Function.pResume    = NULL;
    g_Function.pWake      = NULL;
    g_Function.pContext   = NULL;


    /* Store the callback and event handlers */
    osdrvCallbacks = *callbacks;

    /* Register with bus driver core */
    status = SDIO_RegisterFunction(&g_Function);
    AR_DEBUG_ASSERT(SDIO_SUCCESS(status));

    return SDIO_SUCCESS(status) ? A_OK : A_ERROR;
}

static SDIO_STATUS IssueConfig(PSDDEVICE        pDevice,
                               SDCONFIG_COMMAND Command,
                               PVOID            pData,
                               INT              Length)
{
    SDCONFIG  configHdr; 
    SET_SDCONFIG_CMD_INFO(&configHdr,Command,pData,Length);
    return SDDEVICE_CALL_CONFIG_FUNC(pDevice,&configHdr);
} 

/* read write an internal SPI register- ALWAYS synchronous */
static A_STATUS SPIReadWriteInternal(HIF_DEVICE *device, 
                                     UINT16      address,
                                     UINT16      *pValue,
                                     BOOL        Read)
{
    SDIO_STATUS status;
    SDREQUEST   *sdrequest;
    BUS_REQUEST *busrequest = NULL;
    A_STATUS    a_status = A_OK;
    
    do {
            /* Allocate a new bus request */ 
        busrequest = hifAllocateBusRequest(device);
        if (busrequest == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Unable to allocate bus request\n"));
            a_status = A_ERROR;
            break;
        }
    
        sdrequest = busrequest->request;
        
        if (Read) {
            ATH_SET_PIO_INTERNAL_READ_OPERATION(sdrequest,address); 
        } else {
            ATH_SET_PIO_INTERNAL_WRITE_OPERATION(sdrequest,address,*pValue);     
        }
        
            /* always synchronous */
        sdrequest->Flags = SDREQ_FLAGS_RAW;
        sdrequest->pCompleteContext = NULL;
        sdrequest->pCompletion = NULL;
        
            /* issue the request */
        status = SDDEVICE_CALL_REQUEST_FUNC(device->handle, sdrequest);
        if (!SDIO_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("Spi Internal Read/Write failure: %d\n", status));
            a_status = A_ERROR;
        } else {
            if (Read) {
                    /* get the read result */
                *pValue = ATH_GET_PIO_INTERNAL_READ_RESULT(sdrequest);
            }   
        }
       
    } while (FALSE);
    
    if (busrequest != NULL) {
        hifFreeBusRequest(device, busrequest);
    }   
    
    return a_status;
}
                               

A_STATUS HIFReadWrite(HIF_DEVICE *device, 
                      A_UINT32 address, 
                      A_UCHAR *buffer, 
                      A_UINT32 length, 
                      A_UINT32 request, 
                      void *context) 
{
    SDIO_STATUS status;
    SDREQUEST   *sdrequest;
    BUS_REQUEST *busrequest = NULL;
    A_STATUS    a_status = A_OK;
    
    /* Return any new requests if the shutdown is already in progress */
    if (device->shutdownInProgress) {
        if (request & HIF_ASYNCHRONOUS) {
            device->htcCallbacks.rwCompletionHandler(context, A_ERROR);
            return A_PENDING;
        }
        return A_ERROR;
    }

    AR_DEBUG_ASSERT(device != NULL);
    AR_DEBUG_ASSERT(device->handle != NULL);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Device: %p\n", device));

    if (length > device->curBlockSize) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Invalid data length: %d\n", length));
        return A_ERROR;
    }

    /* 
     * We do not differentiate between the extended and the basic I/O so 
     * we do not process the request type.
     */

    /*
     * We do not differentiate between the byte and block data mode so
     * we do not process the request dmode.
     */

    do {
        /* Addressing mode */
        if (request & HIF_FIXED_ADDRESS) {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Address mode: Fixed\n"));
        } else if (request & HIF_INCREMENTAL_ADDRESS) {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Address mode: Incremental\n"));
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("Invalid address mode: 0x%08x\n", request));
            a_status = A_ERROR;
            break;
        }

        /* 
         * Mailbox read/write. Adjust the address so that the last byte 
         * falls on the EOM address.
         */
        if (request & HIF_WRITE) { 
            if ((address >= HIF_MBOX_START_ADDR(0)) && 
                (address <= HIF_MBOX_END_ADDR(3)))
            {
                AR_DEBUG_ASSERT(length <= HIF_MBOX_WIDTH);
                address += (HIF_MBOX_WIDTH - length);
            }
        }

        /* Allocate a new bus request */ 
        busrequest = hifAllocateBusRequest(device);
        if (busrequest == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Unable to allocate bus request\n"));
            a_status = A_ERROR;
            break;
        }
    
        sdrequest = busrequest->request;
        sdrequest->pDataBuffer = buffer;
    
        if (request & HIF_SYNCHRONOUS) {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Execution mode: Synchronous\n"));
            sdrequest->Flags = SDREQ_FLAGS_RAW;
            sdrequest->pCompleteContext = NULL;
            sdrequest->pCompletion = NULL;
        } else if (request & HIF_ASYNCHRONOUS) {
                /* Populate the bus request to be passed in as context */
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Execution mode: Asynchronous\n"));
                /* setup async context */
            busrequest->device = device;
            busrequest->context = context;
            sdrequest->pCompleteContext = busrequest;
            sdrequest->Flags = SDREQ_FLAGS_RAW | SDREQ_FLAGS_TRANS_ASYNC;
            sdrequest->pCompletion = hifRWCompletionHandler;
        }
        
            /* Indicate to the bus driver if its a read or a write */
        if (request & HIF_WRITE) {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Direction: Write\n"));
            if (((address >= HIF_MBOX_START_ADDR(0)) &&
                 (address <= HIF_MBOX_END_ADDR(3)))) {  
                
                /* trapping HTC WRITE to mailbox, these will use the special DMA operation */                               
                AR_DEBUG_PRINTF(ATH_DEBUG_TRC, 
                            ("--- MAILBOX WRITE ACCESS!!!!\n"));
                            
                ATH_SET_DMA_OPERATION(sdrequest,ATH_TRANS_WRITE,address,length);

            } else {
                ATH_SET_PIO_EXTERNAL_WRITE_OPERATION(sdrequest,
                                                     address,
                                                     (request & HIF_INCREMENTAL_ADDRESS) ? TRUE : FALSE,
                                                     length);
                
            }       
        } else if (request & HIF_READ) {
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Direction: Read\n"));
            if (((address >= HIF_MBOX_START_ADDR(0)) &&
                 (address <= HIF_MBOX_END_ADDR(3)))) {   
                                     
                 AR_DEBUG_PRINTF(ATH_DEBUG_TRC, 
                            (" --- MAILBOX READ ACCESS!!!!\n"));
                /*  trapping on HTC READ mailbox , these will use the special DMA operation */
               ATH_SET_DMA_OPERATION(sdrequest,ATH_TRANS_READ,address,length);
            } else { 
                ATH_SET_PIO_EXTERNAL_READ_OPERATION(sdrequest,
                                                    address,
                                                    request & HIF_INCREMENTAL_ADDRESS ? TRUE : FALSE,
                                                    length);
            }
            
        } else {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("Invalid direction: 0x%08x\n", request));
            a_status = A_ERROR;
            break;
        }

        if ((request & HIF_SYNCHRONOUS) && (length <= MAX_BUFFERED_SYNC_IO_BYTES)) {
            /* check if the request is synchronous and the length is small.  This is a
             * workaround for stack-allocated buffers that will get past to the HCD driver.
             * Stack allocated buffers may come from a process outside device.exe.  This will cause
             * an access violation in the HCD driver when the I/O is completed in an IST or worker
             * thread. One possible fix is for the HCD to save and change process permissions before
             * accessing the buffer, however this adds overhead to EVERY I/O operation 
             * A better fix is to buffer small SYNC I/O requests since these are most likely to be stack 
             * allocated buffers.*/
                          
                /* save caller buffer */ 
            busrequest->pCallerBuffer = buffer;
                /* use  temp buffer that is part of the request */
            sdrequest->pDataBuffer = busrequest->buffer; 
            if (request & HIF_WRITE) {
                A_MEMCPY(sdrequest->pDataBuffer, buffer, length);    
            }    
        }
        
            /* issue the request */
        status = SDDEVICE_CALL_REQUEST_FUNC(device->handle, sdrequest);
        if (!SDIO_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("HIF Read/Write failure: %d\n", status));
            a_status = A_ERROR;
        }
        
        if ((request & HIF_SYNCHRONOUS) && (busrequest->pCallerBuffer != NULL)) {
            if (SDIO_SUCCESS(status) && (request & HIF_READ)) {
                    /* copy result back */
                A_MEMCPY(busrequest->pCallerBuffer, busrequest->buffer, length);   
            }                
            busrequest->pCallerBuffer = NULL;
        }       
                  
    } while (FALSE);
    
    if ((busrequest != NULL) && (request & HIF_SYNCHRONOUS)) {
        hifFreeBusRequest(device,busrequest);
    }   

    return a_status;
}

void
HIFShutDownDevice(HIF_DEVICE *device)
{
    SDIO_STATUS status;

    if (device != NULL) {
        
        device->shutdownInProgress = TRUE;
        AR_DEBUG_ASSERT(device->handle != NULL);
    
    } else {
        /* 
         * Unregister with bus driver core. This should flush the pending 
         * requests in the HCD's queues.
         */
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Unregistering with the bus driver\n"));
        status = SDIO_UnregisterFunction(&g_Function);
        AR_DEBUG_ASSERT(SDIO_SUCCESS(status));

    }
}

void
HIFAckInterrupt(HIF_DEVICE *device)
{
    SDIO_STATUS status;

    /* Acknowledge our function IRQ */
    status = IssueConfig(device->handle, SDCONFIG_FUNC_ACK_IRQ, NULL, 0);
    AR_DEBUG_ASSERT(SDIO_SUCCESS(status));
}

void
HIFUnMaskInterrupt(HIF_DEVICE *device)
{
    SDIO_STATUS status;

    AR_DEBUG_ASSERT(device != NULL);
    AR_DEBUG_ASSERT(device->handle != NULL);
    
    /* Unmask our function IRQ */
    status = IssueConfig(device->handle, SDCONFIG_FUNC_UNMASK_IRQ, NULL, 0);
    AR_DEBUG_ASSERT(SDIO_SUCCESS(status));
   
    /*
     * It was observed that if ar6000 module was removed while an interrupt
     * was pending then when its reloaded subsequently, the hcd/bus driver
     * expects an explicit acknowledgment before it can start reporting
     * interrupts. Under normal conditions, this should be harmless.
     */
    HIFAckInterrupt(device);
}

void HIFMaskInterrupt(HIF_DEVICE *device)
{
    SDIO_STATUS status;
    AR_DEBUG_ASSERT(device != NULL);
    AR_DEBUG_ASSERT(device->handle != NULL);

    /* Mask our function IRQ */
    status = IssueConfig(device->handle, SDCONFIG_FUNC_MASK_IRQ,NULL, 0);
    AR_DEBUG_ASSERT(SDIO_SUCCESS(status));

}

A_STATUS
HIFConfigureDevice(HIF_DEVICE *device, HIF_DEVICE_CONFIG_OPCODE opcode, 
                   void *config, A_UINT32 configLen)
{
    A_UINT32 count;
    A_STATUS status = A_OK;

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

        case HIF_DEVICE_GET_PENDING_EVENTS_FUNC:
                /* set pending events */
            *((HIF_PENDING_EVENTS_FUNC *)config) = hifGetPendingEvents;
            break;
        
        case HIF_DEVICE_GET_RECV_EVENT_MASK_UNMASK_FUNC:
                /* set recv masking function */
            *((HIF_MASK_UNMASK_RECV_EVENT *)config) = hifMaskUnmaskRecvMsg;
            break;   
            
        case HIF_DEVICE_GET_IRQ_PROC_MODE:
            // TODO for now lets do sync-only (it's simplier), to opimize we should implement ASYNC 
            // processing in our get pending events function
            *((HIF_DEVICE_IRQ_PROCESSING_MODE *)config) = HIF_DEVICE_IRQ_SYNC_ONLY;
            AR_DEBUG_PRINTF(ATH_DEBUG_TRC, (" SPI2 HIF requires sync mode IRQ processing \n"));
            break;
        case HIF_DEVICE_POWER_STATE_CHANGE:             
            status = PowerChangeNotify(device, *(HIF_DEVICE_POWER_CHANGE_TYPE *)config);
            break;    
        default:
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, 
                            ("Invalid configuration opcode: %d\n", opcode));
            return A_ERROR;
    }

    return status;
}

void 
hifRWCompletionHandler(SDREQUEST *request) 
{
    void *context;
    A_STATUS status = A_OK;
    HIF_DEVICE *device;
    BUS_REQUEST *busrequest;

    busrequest = (BUS_REQUEST *)request->pCompleteContext;
    AR_DEBUG_ASSERT(busrequest->request == request);
    device = busrequest->device;
    context = busrequest->context;
    if (!SDIO_SUCCESS(request->Status)) {
        status = A_ERROR;    
    }
    
        /* free the request, we have all the info we need */
    hifFreeBusRequest(device,busrequest);
    busrequest = NULL;
    
    if (device->shutdownInProgress) {
        device->htcCallbacks.rwCompletionHandler(context, A_ERROR);
        return;
    }
    
    device->htcCallbacks.rwCompletionHandler(context, status);
}

void
hifIRQHandler(void *context)
{
    A_STATUS status;
    HIF_DEVICE *device;
    
    device = (HIF_DEVICE *)context;
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Device: %p\n", device));

    CriticalSectionAcquire(&device->dsrExecutionLock);
    status = device->htcCallbacks.dsrHandler(device->htcCallbacks.context);        
    CriticalSectionRelease(&device->dsrExecutionLock);   
    
    AR_DEBUG_ASSERT(status == A_OK);
}

static void hifCleanupDevice(HIF_DEVICE *device)
{
    int count;
    
    CriticalSectionDelete(&device->lock);
    CriticalSectionDelete(&device->dsrExecutionLock);
    
    for (count = 0; count < BUS_REQUEST_MAX_NUM_TOTAL; count++) {
        if (device->busrequestblob[count].request != NULL){
            SDDeviceFreeRequest(device->handle,device->busrequestblob[count].request);
            device->busrequestblob[count].request = NULL;
        }
    }
    
    free(device);
}

BOOL
hifDeviceInserted(SDFUNCTION *function, SDDEVICE *handle)
{
    A_UINT32 count;
    HIF_DEVICE *device;
    BOOL accepted = FALSE;
    AR_DEBUG_ASSERT(function != NULL);
    AR_DEBUG_ASSERT(handle != NULL);

    do {
        
        device = (HIF_DEVICE *)malloc(sizeof(HIF_DEVICE));
    
        if (NULL == device) {
            break;    
        }
    
        AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Device: %p\n", device));
     
        ZERO_POBJECT(device);
        
        CriticalSectionInit(&device->lock);
        CriticalSectionInit(&device->dsrExecutionLock);
        device->busrequestfreelist = NULL;
        device->handle = handle;
        device->enabledSpiInts = ATH_SPI_INTR_PKT_AVAIL | ATH_SPI_INTR_CPU_INTR;
            /* set the IRQ Handler which associates the instance with the SDDEVICE */
        SDDEVICE_SET_IRQ_HANDLER(device->handle, hifIRQHandler, device);
    
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Device: %p\n", device));
       
        if (handle->pId[0].CardFlags & CARD_RAW) {
            if (strstr(SDDEVICE_GET_HCDNAME(handle), SDIO_RAW_BD_BASE) == NULL) {
                /* Not supported */
                break;
            }
        }
    
        /* Card identification */
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, 
                        ("SPI card: %s\n", SDDEVICE_GET_HCDNAME(handle)));
    
        /* Version information */
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Stack version: %d.%d\n", 
                                          SDDEVICE_GET_VERSION_MAJOR(handle), 
                                          SDDEVICE_GET_VERSION_MINOR(handle)));
    
        /* Get the maximum block size supported */
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Maximum block length: %d bytes\n",
                        SDDEVICE_GET_MAX_BLOCK_LEN(handle)));
                        
        device->curBlockSize = SDDEVICE_GET_OPER_BLOCK_LEN(handle);
        
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Current block length: %d bytes\n",
                        device->curBlockSize));
    
            /* Allocate the bus requests to be used later */
        for (count = 0; count < BUS_REQUEST_MAX_NUM_TOTAL; count ++) {
            if ((device->busrequestblob[count].request = SDDeviceAllocRequest(handle)) == NULL){
                AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Unable to allocate bus request\n"));
                break;
            }
                /* free to the list */
            hifFreeBusRequest(device, &device->busrequestblob[count]);
        }
        
        if (count != BUS_REQUEST_MAX_NUM_TOTAL) {
            break;   
        }
        
            /* Configure the SPI interface */
        if ((hifConfigureSPI(device)) != A_OK) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Failed to configure the device\n"));
            break;
        }
    
        /* Inform OS drv layer */
        if ((osdrvCallbacks.deviceInsertedHandler(osdrvCallbacks.context,(void *) device)) != A_OK) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("Device rejected\n"));
            return FALSE;
        }

        accepted = TRUE;
        
    } while (FALSE);
    
    if (!accepted & (device != NULL)) {
        /* cleanup device */
        hifCleanupDevice(device);   
    }
    
    return accepted;
}

A_STATUS
hifConfigureSPI(HIF_DEVICE *device)
{
    A_STATUS a_status = A_OK;
    A_UINT16 regValue;
    
    do {
        
            /* set watermark */
        regValue = SPI_RDBUF_WATER_MARK_LEVEL;
        a_status = SPIWriteInternal(device, 
                                    ATH_SPI_RDBUF_WATERMARK_REG,
                                    &regValue);
        
#if 0
        UINT8    regBuffer[32];
        int i;
    
        for (i = 0; i < 8; i++) {
            regBuffer[i] = (UINT8)(255-i);   
        }
    
        SDLIB_PrintBuffer(regBuffer, 8, "Pattern OUT");
    
        a_status = HIFReadWrite(device, 
                                SCRATCH_ADDRESS, 
                                regBuffer, 
                                8, 
                                HIF_WRITE | HIF_SYNCHRONOUS | HIF_INCREMENTAL_ADDRESS, 
                                NULL);
                 
        if (a_status != A_OK) {
            break;    
        }
        
        memset(regBuffer,0,8);
        
        a_status = HIFReadWrite(device, 
                                SCRATCH_ADDRESS, 
                                regBuffer, 
                                8, 
                                HIF_READ | HIF_SYNCHRONOUS | HIF_INCREMENTAL_ADDRESS, 
                                NULL);
                 
        if (a_status != A_OK) {
            break;   
        }
        
        SDLIB_PrintBuffer(regBuffer, 8, "Pattern IN");
        
        a_status = HIFReadWrite(device, 
                                0x424, 
                                regBuffer, 
                                1, 
                                HIF_READ | HIF_SYNCHRONOUS | HIF_FIXED_ADDRESS, 
                                NULL);
                 
        if (a_status != A_OK) {
            break;   
        }
        
        AR_DEBUG_PRINTF(ATH_DEBUG_ERROR, ("COUTN Read: 0x%X \n",regBuffer[0]));
#endif    
    
    } while (FALSE);
    
    return a_status;
}

void
hifDeviceRemoved(SDFUNCTION *function, SDDEVICE *handle)
{
    A_STATUS   status;
    HIF_DEVICE *device;
    AR_DEBUG_ASSERT(function != NULL);
    AR_DEBUG_ASSERT(handle != NULL);

        /* our device is the IRQ context we stored */
    device = (HIF_DEVICE *)handle->IrqContext;
    if (device->claimedContext != NULL) {
        status = osdrvCallbacks.deviceRemovedHandler(device->claimedContext, device);
    }
        /* cleanup the instance */
    hifCleanupDevice(device);
    AR_DEBUG_ASSERT(status == A_OK);
}

BUS_REQUEST *
hifAllocateBusRequest(HIF_DEVICE *device) 
{
    BUS_REQUEST *request = NULL;

    /* Acquire Lock */
    CriticalSectionAcquire(&device->lock);

    if (device->busrequestfreelist != NULL) {
            /* remove from head */
        request = device->busrequestfreelist;
        device->busrequestfreelist = request->next;
        request->next = NULL; 
    }
    
    /* Release lock */
    CriticalSectionRelease(&device->lock);

    return request;
}

void
hifFreeBusRequest(HIF_DEVICE *device, BUS_REQUEST *request)
{
    AR_DEBUG_ASSERT(request != NULL);

    /* Acquire lock */
    CriticalSectionAcquire(&device->lock);
        /* get the head and chain it */
    request->next = device->busrequestfreelist;
        /* set new head */
    device->busrequestfreelist = request;
    
    /* Release lock */
    CriticalSectionRelease(&device->lock);
}

void HIFSpiDumpRegs(HIF_DEVICE *device) {
    INT dummy;
    IssueConfig(device->handle, ATH_SPI_CONFIG_DUMP_SPI_INTERNAL_REGISTERS, &dummy, sizeof(dummy));
}

static A_STATUS hifMaskUnmaskRecvMsg(HIF_DEVICE              *device,
                                     A_BOOL                  Mask,
                                     void                    *AsyncContext)
{
    A_BOOL receiverReEnabled = FALSE;
    
    CriticalSectionAcquire(&device->lock);
    
    if (Mask) {
        device->enabledSpiInts &= ~ATH_SPI_INTR_PKT_AVAIL;      
    } else {
            /* unmasking */
        if (!(device->enabledSpiInts & ATH_SPI_INTR_PKT_AVAIL)) {
                /* the receiver was previously disabled and then re-enabled
                 * again, we use this to trigger to re-check for interrupts again */
            receiverReEnabled = TRUE;    
        }
        device->enabledSpiInts |= ATH_SPI_INTR_PKT_AVAIL;          
    }
      
        /* Release lock */
    CriticalSectionRelease(&device->lock);  
    
    if (AsyncContext != NULL) {
            /* just callback to emulate async behavior */
        device->htcCallbacks.rwCompletionHandler(AsyncContext, A_OK);        
    }
    
    if (receiverReEnabled && (AsyncContext == NULL)) {
            /* recheck pending interrupts, this is to catch messages that may have
             * queued into the SPI buffer, but the host did not have any resources
             * to fetch them till now */
        CriticalSectionAcquire(&device->dsrExecutionLock);
        device->htcCallbacks.dsrHandler(device->htcCallbacks.context);        
        CriticalSectionRelease(&device->dsrExecutionLock);
    }
    
    return A_OK;
}

                                                                               
static A_STATUS hifGetPendingEvents(HIF_DEVICE              *device, 
                                    HIF_PENDING_EVENTS_INFO *pEvents,
                                    void                    *AsyncContext)
{
    A_STATUS   status = A_OK;
    A_UINT16   spiInterruptCause, regValue;
    
    AR_DEBUG_PRINTF(ATH_DEBUG_IRQ, 
                    ("+hifGetPendingEvents : (hifdev: 0x%X) \n", (A_UINT32)device));

    A_MEMZERO(pEvents,sizeof(HIF_PENDING_EVENTS_INFO));
    
    do {
        
        if (AsyncContext != NULL) {
            /* we reported that we are SYNC only, this should never be set */
            AR_DEBUG_ASSERT(FALSE);  
            status = A_ERROR;
            break;  
        }
        
            /* read the SPI interrupt cause register */
        status = SPIReadInternal(device, 
                                 ATH_SPI_INTR_CAUSE_REG,
                                 &spiInterruptCause);
                                      
        if (A_FAILED(status)) {
            break;    
        } 
        
            /* the spi interrupt cause register is a shadow of target-side interrupt state
             * It allows us to inspect it without having to wake up the target.
             * If the CPU_INTR bit is set, check and see if we need to get the target registers
               that provide more information on the interrupt */
        
        AR_DEBUG_PRINTF(ATH_DEBUG_IRQ, 
                            ("hifGetPendingEvents : SPI INTR_CAUSE: 0x%x\n",spiInterruptCause));
                                                  
        if (spiInterruptCause & ATH_SPI_INTR_CPU_INTR) {
                /* there are other events the upper layer needs to look at by reading the 
                 * interrupt status registers */
            pEvents->Events |= HIF_OTHER_EVENTS;
        }
    
            /* check for packet available */
        if (spiInterruptCause & ATH_SPI_INTR_PKT_AVAIL) {
                /* ack the packet available interrupt */
            regValue = ATH_SPI_INTR_PKT_AVAIL;        
            status = SPIWriteInternal(device, 
                                      ATH_SPI_INTR_CAUSE_REG,
                                      &regValue);
            
            if (A_FAILED(status)) {
                break;    
            } 
        }
        
            /* every time we are called we must always check to see if we can read the lookaheads
             * in the SPI interface */
        if (device->enabledSpiInts & ATH_SPI_INTR_PKT_AVAIL) {
                /* we do this whenever the host wants recv packets 
                 * regardless of whether the interrupt is pending or not.  By design the
                 * SPI hardware asserts the interrupt for the first packet (and holds the assertion)
                 * if multiple packets are in the spi receive buffer before we can ack the ATH_SPI_INTR_PKT_AVAIL 
                 * interrupt no more interrupts will be generated.  It is required by the host to
                 * make sure the buffer gets emptied.
                 * This relies on the upper layer to
                 * continuously call this to check for pending packets */
            status = hifGetLookAhead(device, pEvents);
            
            if (A_FAILED(status)) {
                break;    
            }
        }
                 
    } while (FALSE);

    AR_DEBUG_PRINTF(ATH_DEBUG_IRQ, ("-hifGetPendingEvents\n"));
    return status;
}

static A_STATUS hifGetLookAhead(HIF_DEVICE *device,  HIF_PENDING_EVENTS_INFO *pEvents)
{
    A_STATUS        status = A_OK;
    UINT16          lookAhead1, lookAhead2;
    UINT16          bytesAvail;
    HTC_FRAME_HDR   *pHTCHdr;
    A_UINT32        assembledLookAhead;
    
    do {
        
            /* get the bytes available in the SPI buffer */
        status = SPIReadInternal(device, 
                                 ATH_SPI_RDBUF_BYTE_AVA_REG,
                                 &bytesAvail);     
        if (A_FAILED(status)) {
            break;    
        }          
        
        AR_DEBUG_PRINTF(ATH_DEBUG_IRQ, ("hifGetLookAhead:  bytesAvail:%d\n",
            bytesAvail));
        
            /* give a copy to the caller */
        pEvents->AvailableRecvBytes = bytesAvail;
            
            /* is there a header's worth ?*/
        if (bytesAvail < sizeof(HTC_FRAME_HDR)) {
                /* not enough bytes for the look-ahead  */
            break;    
        }
        
            /* peak into the SPI read buffer using the look aheads since we
             * established that there is at least a look-ahead number of bytes */
        status = SPIReadInternal(device, 
                                 ATH_SPI_RDBUF_LOOKAHEAD1_REG,
                                 &lookAhead1);
                                      
        if (A_FAILED(status)) {
            break;    
        }
        
        status = SPIReadInternal(device, 
                                 ATH_SPI_RDBUF_LOOKAHEAD2_REG,
                                 &lookAhead2);
                                      
        if (A_FAILED(status)) {
            break;    
        } 
        
           /* assemble look-ahead , first we need to swap bytes */
        lookAhead1 = A_SWAPBYTES16(lookAhead1);
        lookAhead2 = A_SWAPBYTES16(lookAhead2);      
        assembledLookAhead = ((A_UINT32)lookAhead2 << 16) | ((A_UINT32)lookAhead1 & 0x0000FFFF); 
        AR_DEBUG_ASSERT(assembledLookAhead != 0);
        
            /* peak into the header */
        pHTCHdr = (HTC_FRAME_HDR *)&assembledLookAhead;
        
        if (bytesAvail < (pHTCHdr->PayloadLen + HTC_HDR_LENGTH)) {
                /* if the buffer has not filled yet, exit and wait for packet available interrupt */
            break;
        }
        
            /* spi buffer has our message now */            
        pEvents->Events |= HIF_RECV_MSG_AVAIL;
        pEvents->LookAhead = assembledLookAhead;
        
        AR_DEBUG_PRINTF(ATH_DEBUG_IRQ, ("hifGetLookAhead: bytesAvail:%d, lookahead:0x%4.4X\n",
            bytesAvail,pEvents->LookAhead));
                               
    } while (FALSE);
    
    return status;
}

static BOOL PowerUpDownSlot(HIF_DEVICE *device, A_BOOL PowerUp)
{
    UINT32 powerParams = PowerUp;
    SDIO_STATUS status = IssueConfig(device->handle, ATH_SPI_CONFIG_SET_POWER, 
                                     &powerParams, sizeof(powerParams));
    
    return status==SDIO_STATUS_SUCCESS;
}

static A_STATUS PowerChangeNotify(HIF_DEVICE  *device, HIF_DEVICE_POWER_CHANGE_TYPE  PowerChange)
{
#define POWER_OFF 1
#define POWER_ON  0
    A_STATUS      status = A_OK;

    switch (PowerChange) {
        case HIF_DEVICE_POWER_UP:
            if (InterlockedExchange(&device->PowerStateOff, POWER_ON) != POWER_OFF) {
                    /* shouldn't be calling this if we are not powered off */
                AR_DEBUG_ASSERT(FALSE);
                break;
            }
            
            if (device->SlotPowerRemoved) {
                /* power was removed from a previous power down attempt */
                device->SlotPowerRemoved = FALSE;    
                /* try powering up */
                PowerUpDownSlot(device, TRUE); 
            }           
            break;

        case HIF_DEVICE_POWER_DOWN:
        case HIF_DEVICE_POWER_CUT:
                /* set powered off flag, but check previous value */
            if (InterlockedExchange(&device->PowerStateOff, POWER_OFF) != POWER_ON) {
                    /* shouldn't be calling this if we are powered off already  */
                AR_DEBUG_ASSERT(FALSE);
                break;
            }
            
            if (PowerChange == HIF_DEVICE_POWER_CUT) {
                    /* caller wants the device to be completely off */
                device->SlotPowerRemoved = PowerUpDownSlot(device, FALSE);
            }
            
            break;

        default:
            AR_DEBUG_ASSERT(FALSE);
            return A_ERROR;
    }

    return status;
}

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
