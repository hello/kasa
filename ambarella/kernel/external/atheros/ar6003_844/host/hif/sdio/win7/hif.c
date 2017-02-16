//------------------------------------------------------------------------------
// <copyright file="hif.c" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// This file contains the routines handling the interaction with the SDIO driver
//
// Author(s): ="Atheros"
//==============================================================================
#include "a_osapi.h"
#include <initguid.h> // required for GUID definitions
#include <ntddsd.h>
#define ATH_MODULE_NAME hif
#include "a_debug.h"
#include "hif.h"
#include "hif_internal.h"
#include "htc_packet.h"
#include "bmi_msg.h"
#include "htc_api.h"
#include <common.h>
#include "common_drv.h"
#include "targaddrs.h"
#include "AR6002\hw\mbox_host_reg.h"
#include "debug_win.h"
#include "Ath_hw.h"

#define ATH_DEBUG_HIF  ATH_DEBUG_MAKE_MODULE_MASK(0)
static ATH_DEBUG_MASK_DESCRIPTION g_HIFDebugDescription[] = {
    { ATH_DEBUG_HIF , "HIF Tracing"},
};

ATH_DEBUG_INSTANTIATE_MODULE_VAR(hif,
                                 "hif",
                                 "Host Interface",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 ATH_DEBUG_DESCRIPTION_COUNT(g_HIFDebugDescription),
                                 g_HIFDebugDescription);

static NTSTATUS SdioGetProperty(
               IN HIF_DEVICE *pAdapter,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length);
static NTSTATUS SdioSetProperty(
               IN HIF_DEVICE *pAdapter,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length);
static NTSTATUS SdioCmd53ReadWriteAsync(
                            IN HIF_DEVICE *pAdapter,
                            IN PHIF_CMD53  pParams,
                            IN OUT PUCHAR  pData,
                            IN BOOLEAN     WriteToDevice,
                            IN PVOID       pContext,
                            IN PNDIS_EVENT pEvent);
static NTSTATUS SdioCmd53ReadWriteSync(
                            IN HIF_DEVICE *pAdapter,
                            IN PHIF_CMD53  pParams,
                            IN OUT PUCHAR  pData,
                            IN BOOLEAN     WriteToDevice);
static NTSTATUS SdioCmd52ReadWriteSync(
                              IN HIF_DEVICE *pAdapter,
                              IN OUT PUCHAR  pData,
                              IN CHAR        Function,
                              IN ULONG       Address,
                              IN BOOLEAN     WriteToDevice);
static NTSTATUS SdioSetClock(IN HIF_DEVICE *pAdapter,
                             IN ULONG       Clock);
static SDBUS_CALLBACK_ROUTINE HifCIRQEventCallback;
static A_STATUS HifAR6KTargetUnavailableEventHandler(HIF_DEVICE *pHifDevice);
static IO_COMPLETION_ROUTINE HifAsyncCompletionRoutine;

#define MEM_TAG '6hta'



#ifdef HIF_PROFILE_IO_PROCESSING

typedef struct _HIF_STATS {
    LONG   HIFSyncRef;
    LONG   HIFAsyncRef;
    LONG   InAsyncCount;
}HIF_STATS;

HIF_STATS g_HIFStats = {0,0,0};

#define HIF_INC_SYNC_REF() InterlockedIncrement(&g_HIFStats.HIFSyncRef)
#define HIF_DEC_SYNC_REF() InterlockedDecrement(&g_HIFStats.HIFSyncRef)
#define HIF_INC_ASYNC_REF() InterlockedIncrement(&g_HIFStats.HIFAsyncRef)
#define HIF_DEC_ASYNC_REF() InterlockedDecrement(&g_HIFStats.HIFAsyncRef)
#define HIF_INC_ASYNC_CB_REF() InterlockedIncrement(&g_HIFStats.InAsyncCount)
#define HIF_DEC_ASYNC_CB_REF() InterlockedDecrement(&g_HIFStats.InAsyncCount)

#else

#define HIF_INC_SYNC_REF()
#define HIF_DEC_SYNC_REF()
#define HIF_INC_ASYNC_REF()
#define HIF_DEC_ASYNC_REF()
#define HIF_INC_ASYNC_CB_REF()
#define HIF_DEC_ASYNC_CB_REF()

#endif



/* ------ Functions ------ */
void hif_register_tbl_attach(A_UINT32 hif_type);

/*
    HIFDeviceInit - initialize the WDF and SDbus devices that we will communicate with
    pAdapter - miniports context
    return - status
*/
NTSTATUS
HIFDeviceInit(HIF_DEVICE *pAdapter)
{
    WDF_OBJECT_ATTRIBUTES       attributes;
    SDBUS_INTERFACE_PARAMETERS  interfaceParameters = {0};
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    UCHAR state = 0;
    USHORT maxBlockLength;
    USHORT hostBlockLength;
    ULONG  clock;
    ULONG  width;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WDF_DEVICE_INFO);
    ntStatus = WdfDeviceMiniportCreate(WdfGetDriver(),
                                       &attributes,
                                       HIFGetAdapter(pAdapter)->Fdo,
                                       HIFGetAdapter(pAdapter)->NextDeviceObject,
                                       HIFGetAdapter(pAdapter)->Pdo,
                                       &HIFGetAdapter(pAdapter)->WdfDevice);
    if (!NT_SUCCESS (ntStatus))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("WdfDeviceMiniportCreate failed  status:(0x%x)", ntStatus));
        return ntStatus;
    }

    //initialize the bus device we will communicate with
    // set WDF device context
    GetWdfDeviceInfo(HIFGetAdapter(pAdapter)->WdfDevice)->pAdapter = HIFGetAdapter(pAdapter);

    //
    //connect to the sdbus stack
    //
    ATHR_DISPLAY_MSG(TRUE, (TEXT_MSG("WDK::HIFGetAdapter(pAdapter)->Pdo=0x%X, HIFGetAdapter(pAdapter)->BusInterface=0x%x\n"), HIFGetAdapter(pAdapter)->Pdo,HIFGetAdapter(pAdapter)->BusInterface));
    ATHR_DISPLAY_MSG(TRUE, (TEXT_MSG("WDK::HIFGetAdapter(pAdapter)->BusInterface.Version = 0x%X\n"),HIFGetAdapter(pAdapter)->BusInterface.Version));
    ntStatus = SdBusOpenInterface(HIFGetAdapter(pAdapter)->Pdo,
                                  &HIFGetAdapter(pAdapter)->BusInterface,
                                  sizeof(SDBUS_INTERFACE_STANDARD),
                                  SDBUS_INTERFACE_VERSION);

    if (!NT_SUCCESS(ntStatus)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, (" IS Failed to open SD Bus status:(0x%x)", ntStatus));
        ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("Error in SdBusOpenInterface!!!\n")));
        return ntStatus;
    }

    interfaceParameters.Size                        = sizeof(SDBUS_INTERFACE_PARAMETERS);
    interfaceParameters.TargetObject                = HIFGetAdapter(pAdapter)->NextDeviceObject;
    interfaceParameters.DeviceGeneratesInterrupts   = TRUE;
    interfaceParameters.CallbackRoutine             = HifCIRQEventCallback;
    interfaceParameters.CallbackRoutineContext      = pAdapter;
    interfaceParameters.CallbackAtDpcLevel          = FALSE;
    ntStatus = STATUS_SUCCESS;
    if (HIFGetAdapter(pAdapter)->BusInterface.InitializeInterface) {
        ntStatus = (HIFGetAdapter(pAdapter)->BusInterface.InitializeInterface)(HIFGetAdapter(pAdapter)->BusInterface.Context,
                                                              &interfaceParameters);
    }
    if (!NT_SUCCESS(ntStatus)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("SD bus InitializeInterface failed  status:(0x%x)", ntStatus));
        return ntStatus;
    }

    // fill in the function number
    ntStatus = SdioGetProperty(pAdapter,
                         SDP_FUNCTION_NUMBER,
                         &HIFGetAdapter(pAdapter)->FunctionNumber,
                         sizeof(HIFGetAdapter(pAdapter)->FunctionNumber));
    if (!NT_SUCCESS(ntStatus)) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("SdioGetProperty failed  status:(0x%x)", ntStatus));
        return ntStatus;
    }

    HIFGetAdapter(pAdapter)->SDbusDriverVersion = SDBUS_DRIVER_VERSION_1;

    SdioGetProperty(pAdapter,
                    SDP_BUS_DRIVER_VERSION,
                    &HIFGetAdapter(pAdapter)->SDbusDriverVersion,
                    sizeof(HIFGetAdapter(pAdapter)->SDbusDriverVersion));

    HIFGetAdapter(pAdapter)->FunctionFocus = HIFGetAdapter(pAdapter)->FunctionNumber;

    if (HIFGetAdapter(pAdapter)->SDbusDriverVersion < SDBUS_DRIVER_VERSION_2) {
        HIFGetAdapter(pAdapter)->BlockMode = 0;
    } else {
        HIFGetAdapter(pAdapter)->BlockMode = 1;
    }

    // mask the interrupts
    SdioSetProperty(pAdapter,
                    SDP_FUNCTION_INT_ENABLE,
                    &state,
                    sizeof(UCHAR));

    // get/set the block length
    // Get the host buffer block size
    ntStatus = SdioGetProperty(pAdapter,
                             SDP_HOST_BLOCK_LENGTH,
                             &hostBlockLength,
                             sizeof(hostBlockLength));
    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }
    HIFGetAdapter(pAdapter)->BlockLength = hostBlockLength;

    maxBlockLength = HIF_MBOX_BLOCK_SIZE;

    if (hostBlockLength < maxBlockLength) {
        maxBlockLength = hostBlockLength;
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("SD bus InitializeInterface failed  block length requested 0x%X, max 0x%X)", maxBlockLength, hostBlockLength));
    }

    HIFGetAdapter(pAdapter)->BlkLengthSet = maxBlockLength;

    ntStatus = SdioSetProperty(pAdapter,
                             SDP_FUNCTION_BLOCK_LENGTH,
                             &maxBlockLength,
                             sizeof(maxBlockLength));

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("SdioSetProperty maxBlockLength = 0x%X\n"), maxBlockLength));

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    if (((PATH_ADAPTER)pAdapter)->pNic->m_Config.sdioclock)
    {
		clock = ((PATH_ADAPTER)pAdapter)->pNic->m_Config.sdioclock;

		ntStatus = SdioSetClock (pAdapter, clock);

		if (!NT_SUCCESS(ntStatus)) {
			return ntStatus;
		}
	}

	clock = 0;
	ntStatus = SdioGetProperty (pAdapter,
							 	SDP_BUS_CLOCK,
							 	&clock,
							 	sizeof(clock));

	ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("Clock value = %d, status = 0x%X 0x%X\n"), clock, ((PATH_ADAPTER)pAdapter)->pNic->m_Config.sdioclock,
																						((PATH_ADAPTER)pAdapter)->pNic->m_Config.sdiowidth));

    if (!HIFGetAdapter(pAdapter)->sdioCLOCK)
    {
        HIFGetAdapter(pAdapter)->sdioCLOCK = clock;
    }

    width = ((PATH_ADAPTER)pAdapter)->pNic->m_Config.sdiowidth;

    ntStatus = SdioSetProperty(pAdapter,
                             SDP_BUS_WIDTH,
                             &width,
                             sizeof(width));

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("Width value = 0x%X, status = 0x%X\n"), width, ntStatus));

	HIFGetAdapter(pAdapter)->sdioWidth = width;

	hif_register_tbl_attach (HIF_TYPE_AR6003);

    return ntStatus;
}

NTSTATUS
HIFDeviceReInit (HIF_DEVICE *pAdapter)
{
    NTSTATUS ntStatus = NDIS_STATUS_SUCCESS;
    ULONG    clock = 0;
    ULONG    width = 0;

    ntStatus = SdioSetProperty(pAdapter,
                             SDP_FUNCTION_BLOCK_LENGTH,
                             &HIFGetAdapter(pAdapter)->BlkLengthSet,
                             sizeof(USHORT));

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("SdioSetProperty BlockLength = 0x%X\n"), HIFGetAdapter(pAdapter)->BlkLengthSet));

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    clock = HIFGetAdapter(pAdapter)->sdioCLOCK;

    ntStatus = SdioSetClock (pAdapter, clock);

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    width = HIFGetAdapter (pAdapter)->sdioWidth;

    ntStatus = SdioSetProperty(pAdapter,
                             SDP_BUS_WIDTH,
                             &width,
                             sizeof(width));

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("Width value = %d, status = 0x%X\n"), width, ntStatus));

    return ntStatus;
}

#define MAX_BYTES_PER_TRANS_BYTE_MODE 512


A_STATUS
HIFReadWrite(HIF_DEVICE  *pDevice,
             A_UINT32     address,
             A_UCHAR     *pBuffer,
             A_UINT32     length,
             A_UINT32     request,
             void        *context)
{
    A_UINT8             mode        = SD_IO_BYTE_MODE;
    A_UINT8             opcode      = SD_IO_FIXED_ADDRESS;
    A_UINT32            blockLen    = 0;
    A_UINT32            blockCount  = 0;
    A_UINT32            count       = 0;
    A_STATUS            status      = A_OK;
    HIF_CMD53           cmd53params;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    A_BOOL              isMbox = FALSE;
    USHORT              setBLen=0;
#ifdef HIF_SDIO_LARGE_BLOCK_MODE
    A_UINT32            regAddress = address;
#endif

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HIFReadWrite:Enter\n"));
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Address 0x%x\n", address));

    if (request & HIF_SYNCHRONOUS) {
        HIF_INC_SYNC_REF();
    } else {
        HIF_INC_ASYNC_REF();
    }

    if ((request & HIF_BLOCK_BASIS) &&
       !(request & HIF_EXTENDED_IO))
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Block mode not allowed for this type of command\n"));
        return A_ERROR;
    }

    if (request & HIF_BLOCK_BASIS)
    {
        mode = SD_IO_BLOCK_MODE;
        blockLen = HIF_MBOX_BLOCK_SIZE;
        blockCount = length / HIF_MBOX_BLOCK_SIZE;
        count = blockCount;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC,
                        ("Block mode (BlockLen: %d, BlockCount: %d)\n",
                          blockLen, blockCount));
    }
    else if (request & HIF_BYTE_BASIS)
    {
#ifndef CONVERT_BLK_MODE
        mode = SD_IO_BYTE_MODE;
        blockLen = length;
        blockCount = 1;
        count = blockLen;
#else
        mode = SD_IO_BLOCK_MODE;
        blockLen = length;
        blockCount = 1;
        count = blockLen;
#endif
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC,
                        ("Byte mode (BlockLen: %d, BlockCount: %d)\n",
                          blockLen, blockCount));

    }

    if (request & HIF_FIXED_ADDRESS)
    {
        opcode = SD_IO_FIXED_ADDRESS;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Fixed       "));
    }
    else if (request & HIF_INCREMENTAL_ADDRESS)
    {
        opcode = SD_IO_INCREMENT_ADDRESS;
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Incremental "));
    }

    if (request & HIF_WRITE)
    {
        if ((address >= HIF_MBOX_START_ADDR(0)) &&
            (address <= HIF_MBOX_END_ADDR(3)))
        {
            /* Mailbox write. Adjust the address so that the last byte
               falls on the EOM address */
            address = address + HIF_MBOX_WIDTH - length;
            isMbox = TRUE;
        }
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("[Write]"));
    }
    else
    {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("[Read ]"));
        if ((address >= HIF_MBOX_START_ADDR(0)) &&
            (address <= HIF_MBOX_END_ADDR(3))) {
            isMbox = TRUE;
        }
    }

#ifdef HIF_SDIO_LARGE_BLOCK_MODE
        /* filter all block mode requests */
    if ((request & HIF_BLOCK_BASIS)) {
        if (length <= MAX_BYTES_PER_TRANS_BYTE_MODE) {
                /* this can be done in byte mode */
                /* force to byte mode */
            mode = SD_IO_BYTE_MODE;
            blockLen = length;
            blockCount = 1;
            count = (length == MAX_BYTES_PER_TRANS_BYTE_MODE) ? 0 : length;

        } else if (length >= WLAN_MAX_SEND_FRAME) {

                /* Special case to check for large-single block operations:
                 *
                 * -- for WRITE operations, if the length is greater than WLAN_MAX_SEND_FRAME,
                 *    we add additional padding to round it up to the
                 *    programmed block size (HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE) so we can
                 *    transfer in a single block
                 *
                 * -- for READ operations the length must be exactly equal to the
                 *    large block size (HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE).
                 *
                 * The rationale for this:
                 *    To minimize changes in the firmware, the firmware credit size is
                 *    actually at least HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE bytes each,
                 *    however we keep I/O block size padding to a minimum value (HIF_MBOX_BLOCK_SIZE).
                 *    We convert any WLAN_MAX_SEND_FRAME SDIO writes to a single MAX block write.
                 *    The receive direction uses (max) HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE byte
                 *    packets because we optimized the target receive buffers to provide 1 maximum 802.3
                 *    frame + lookahead and credit reports. This pushes the largest recv packet to
                 *    HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE bytes.
                 *    Thus, in choosing the common block size to use, we are using the recv side.
                 *
                 * */

            if ((request & HIF_WRITE) ||
                (!(request & HIF_WRITE) && (length == HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE))) {

                A_ASSERT(length <= HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE);
                length = HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE;
                blockLen = HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE;
                blockCount = 1;
                count = 1;
                if (request & HIF_WRITE) {
                        /* since we increased the actual length we need to re-adjust the
                         * address so the last byte falls in the right place */
                    address = regAddress + HIF_MBOX_WIDTH - length;
                }
            } else {
                /* empty case... fall through, it'll get picked up by our reduced block mode transfer */
                A_ASSERT(blockCount > 1);
            }

        } else {
                /* if it falls through here, there BETTER be multiblock transfers
                 * these will get sent out using the  reduced single block transfer mode since the
                 * SDIO card is currently using a large value for the block size */
            A_ASSERT(blockCount > 1);
        }

#ifdef HIF_USE_SOFTBLOCK
        if ((blockCount > 1) && g_SoftBlockAvail) {
            /* catch multi-block operations that will use "soft block"
             * Soft block has one disadvantage that it will use the block size as
             * the size of each single block transfer.
             * Here we check to see if the total amount of data is a multiple of 512
             * or 256 bytes and let softblock transfer using the larger block size */
            if ((length % 512) == 0) {
                blockLen = 512;
                blockCount = length / 512;
                count = blockCount;
            } else if ((length % 256) == 0) {
                blockLen = 256;
                blockCount = length / 256;
                count = blockCount;
            }
        }
#endif

    }
#endif
#ifdef CONVERT_BLK_MODE
    setBLen = blockLen;
    SdioSetProperty(pDevice,
		    SDP_FUNCTION_BLOCK_LENGTH,
		    &setBLen,
		    sizeof(setBLen));
#endif

    if (request & HIF_EXTENDED_IO)
    {
        cmd53params.Address = address;          //address to read/write
        cmd53params.Opcode = opcode;            // 0 - fixed address 1 - incremetal
        cmd53params.Mode = mode;                // 0 - byte mode 1 - block mode
        cmd53params.BlockCount = blockCount;    // number of blocks to transfer
        cmd53params.BlockLength = blockLen;     // block length of transfer
    } else if (request & HIF_BASIC_IO) {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("Basic I/O not supported\n"));
        return A_ERROR;
    }

    if (request & HIF_SYNCHRONOUS) {
        AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("Synchronous\n"));
        VERIFY_IS_IRQL_PASSIVE_LEVEL();
        ntStatus = SdioCmd53ReadWriteSync(pDevice,
                                      &cmd53params,
                                      pBuffer,
                                      (request & HIF_WRITE) ? 1 : 0);
    } else {

        /* ASYNC operation */
        ntStatus = SdioCmd53ReadWriteAsync(pDevice,
                                      &cmd53params,
                                      pBuffer,
                                      (request & HIF_WRITE) ? 1 : 0,
                                      context,
                                      NULL);
    }

    if (!NT_SUCCESS(ntStatus))  {
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("SDBusRequest (%s) failed 0x%x\n",
            (request & HIF_SYNCHRONOUS) ? "SYNC" : "ASYNC", ntStatus));

        ATHR_DISPLAY_MSG (TRUE, (TEXT("SDBusRequest (%s) failed 0x%x\n"), (request & HIF_SYNCHRONOUS)?"SYNC":"ASYNC", ntStatus));

        AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("    ....(%s) address:0x%4.4X blocks:%d, blksize:%d 0x%x  \n",
                (request & HIF_WRITE) ? "WRITE" : "READ",
                address,
                blockCount, blockLen,
                ntStatus));

        ATHR_DISPLAY_MSG (TRUE, (TEXT("    ....(%s) address:0x%4.4X blocks:%d, blksize:%d 0x%x  \n"),
                         (request & HIF_WRITE)? "WRITE":"READ", address, blockCount, blockLen, ntStatus));

        status = A_ERROR;
    }
#ifdef CONVERT_BLK_MODE
    setBLen = HIFGetAdapter(pDevice)->BlkLengthSet;

    SdioSetProperty(pDevice,
		    SDP_FUNCTION_BLOCK_LENGTH,
		    &setBLen,
		    sizeof(setBLen));
#endif
    return status;
}

VOID
HIFShutDownDevice (HIF_DEVICE  *pAdapter)
{
    do
    {
        HifAR6KTargetUnavailableEventHandler (pAdapter);

        // unregister with the sd bus
        if (HIFGetAdapter(pAdapter)->BusInterface.InterfaceDereference)
        {
            (HIFGetAdapter(pAdapter)->BusInterface.InterfaceDereference)(HIFGetAdapter(pAdapter)->BusInterface.Context);
            HIFGetAdapter(pAdapter)->BusInterface.InterfaceDereference = NULL;
        }

        // remove the WDF object
        if (NULL != HIFGetAdapter(pAdapter)->WdfDevice)
        {
            WdfObjectDelete (HIFGetAdapter(pAdapter)->WdfDevice);
        }

    }while (FALSE);

    return;
}



// Interrupt callback routine
static VOID
HifCIRQEventCallback(
   IN PVOID pContext,
   IN ULONG InterruptType
   )
{
    UNREFERENCED_PARAMETER(InterruptType);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HifCIRQEventCallback\n"));
    if ((HIFGetAdapter(pContext))->HTCcallbacks.dsrHandler != NULL) {
        (HIFGetAdapter(pContext))->HTCcallbacks.dsrHandler((HIFGetAdapter(pContext))->HTCcallbacks.context);
    }
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HifCIRQEventCallback\n"));
}

void
HIFAckInterrupt(HIF_DEVICE *pDevice)
{
	if (HIFGetAdapter(pDevice))
	{
    	HIFGetAdapter(pDevice)->BusInterface.AcknowledgeInterrupt(HIFGetAdapter(pDevice)->BusInterface.Context);
	}

    return;
}

void
HIFUnMaskInterrupt(HIF_DEVICE *pDevice)
{
    NTSTATUS status;
    UCHAR state = 1;

    //unmask the interrupts
    status = SdioSetProperty(pDevice,
                             SDP_FUNCTION_INT_ENABLE,
                             &state,
                             sizeof(UCHAR));

    AR_DEBUG_ASSERT(NT_SUCCESS(status));

    return;
}

void HIFMaskInterrupt(HIF_DEVICE *pDevice)
{
    NTSTATUS status;
    UCHAR state = 0;

    //mask the interrupts
    status = SdioSetProperty(pDevice,
                             SDP_FUNCTION_INT_ENABLE,
                             &state,
                             sizeof(UCHAR));

    AR_DEBUG_ASSERT(NT_SUCCESS(status));
    return;
}

A_STATUS
HIFConfigureDevice(HIF_DEVICE *pDevice, HIF_DEVICE_CONFIG_OPCODE opcode,
                   void *config, A_UINT32 configLen)
{
    A_UINT32 count;
    A_STATUS status = A_OK;

    UNREFERENCED_PARAMETER(configLen);
    UNREFERENCED_PARAMETER(pDevice);

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
            *((HIF_DEVICE_IRQ_PROCESSING_MODE *)config) = HIF_DEVICE_IRQ_SYNC_ONLY; //??HIF_DEVICE_IRQ_SYNC_ONLY; //??HIF_DEVICE_IRQ_ASYNC_SYNC;
            break;

        case HIF_DEVICE_POWER_STATE_CHANGE:
            //??status = PowerChangeNotify(device, *(HIF_DEVICE_POWER_CHANGE_TYPE *)config);
            break;

        default:
            return A_ERROR;
    }

    return status;
}





void HIFClaimDevice(HIF_DEVICE  *pDevice, void *pContext)
{
    UNREFERENCED_PARAMETER(pDevice);
    UNREFERENCED_PARAMETER(pContext);
}

void HIFReleaseDevice(HIF_DEVICE  *pDevice)
{
    UNREFERENCED_PARAMETER(pDevice);
}

A_STATUS HIFAttachHTC(HIF_DEVICE *pDevice, HTC_CALLBACKS *callbacks)
{
    HIFGetAdapter(pDevice)->HTCcallbacks = *callbacks;
    return A_OK;
}

void HIFDetachHTC(HIF_DEVICE *pDevice)
{
    A_MEMZERO(&HIFGetAdapter(pDevice)->HTCcallbacks,sizeof(HIFGetAdapter(pDevice)->HTCcallbacks));
}

NTSTATUS
SdioGetProperty(
               IN HIF_DEVICE *pAdapter,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length
               )
{
    PSDBUS_REQUEST_PACKET   sdrp = NULL;
    PVOID                   pNonPagedData = NULL;
    NTSTATUS                status;

    do {
        sdrp = ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), MEM_TAG);

        if (sdrp == NULL) {
            status = STATUS_NO_MEMORY;
            break;
        }

        pNonPagedData = ExAllocatePoolWithTag(NonPagedPool, Length, MEM_TAG);

        if (pNonPagedData == NULL) {
             status = STATUS_NO_MEMORY;
             break;
        }

        RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

        sdrp->RequestFunction                    = SDRF_GET_PROPERTY;
        sdrp->Parameters.GetSetProperty.Property = Property;
        sdrp->Parameters.GetSetProperty.Buffer   = pNonPagedData;
        sdrp->Parameters.GetSetProperty.Length   = Length;

        status = SdBusSubmitRequest(HIFGetAdapter(pAdapter)->BusInterface.Context, sdrp);

        RtlCopyMemory(Buffer, pNonPagedData, Length);
    }while(FALSE);

    if (sdrp != NULL) {
       ExFreePool(sdrp);
    }

    if (pNonPagedData != NULL) {
       ExFreePool(pNonPagedData);
    }

    return status;
}


NTSTATUS
SdioSetProperty(
               IN HIF_DEVICE *pAdapter,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length
               )
{
    PSDBUS_REQUEST_PACKET   sdrp = NULL;
    PVOID                   pNonPagedData = NULL;
    NTSTATUS                status;

    do {
        sdrp = ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), MEM_TAG);

        if (sdrp == NULL) {
            status = STATUS_NO_MEMORY;
            break;
        }

        pNonPagedData = ExAllocatePoolWithTag(NonPagedPool, Length, MEM_TAG);

        if (pNonPagedData == NULL) {
             status = STATUS_NO_MEMORY;
             break;
        }

        RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

        RtlCopyMemory(pNonPagedData, Buffer, Length);

        sdrp->RequestFunction = SDRF_SET_PROPERTY;
        sdrp->Parameters.GetSetProperty.Property = Property;
        sdrp->Parameters.GetSetProperty.Buffer = pNonPagedData;
        sdrp->Parameters.GetSetProperty.Length = Length;

        status = SdBusSubmitRequest(HIFGetAdapter(pAdapter)->BusInterface.Context, sdrp);
    }while(FALSE);

    if (sdrp != NULL) {
       ExFreePool(sdrp);
    }

    if (pNonPagedData != NULL) {
       ExFreePool(pNonPagedData);
    }

    return status;
}


typedef struct _COMPLETION_CONTEXT {
    HIF_DEVICE                 *pAdapter;
    SDBUS_REQUEST_PACKET        Sdrp;
    PMDL                        pMdl;
    PVOID                       pUserContext;
    PNDIS_EVENT                 pEvent;

    PVOID                       pData;
    PVOID                       pNonPagedData;
    ULONG                       Length;
}COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

// completion routine for SdioCmd53ReadWrite
//  this completes request that were originally async from HIF caller as well as synch calls made into HIF.
NTSTATUS  HifAsyncCompletionRoutine(
    IN PDEVICE_OBJECT  pDeviceObject,
    IN PIRP            pIrp,
    IN PVOID           pCallbackContext
    )
{
    PCOMPLETION_CONTEXT         pCompletionContext;
    A_STATUS                    status = A_OK;

    UNREFERENCED_PARAMETER(pDeviceObject);
    //retrieve the saved info
    pCompletionContext = (PCOMPLETION_CONTEXT)pCallbackContext;
    if (!NT_SUCCESS(pIrp->IoStatus.Status)) {
        status = A_ERROR;
        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("HifAsyncCompletionRoutine, Irp completed with error: 0x%X\n", pIrp->IoStatus.Status));
        ATHR_DISPLAY_MSG (TRUE, (TEXT("HifAsyncCompletionRoutine, Irp completed with error: 0x%X\n"), pIrp->IoStatus.Status));
    }

    if (pCompletionContext)
    {
        if (pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.TransferDirection == SDTD_READ) {
            RtlCopyMemory(pCompletionContext->pData, pCompletionContext->pNonPagedData, pCompletionContext->Length);
        }

        if (pCompletionContext->pEvent == NULL) {
            //complete using upper layers completion routine
            A_ASSERT(pCompletionContext->pAdapter);
            A_ASSERT(HIFGetAdapter(pCompletionContext->pAdapter)->HTCcallbacks.rwCompletionHandler);

            HIFGetAdapter(pCompletionContext->pAdapter)->HTCcallbacks.rwCompletionHandler(pCompletionContext->pUserContext, status);
        } else {
            //complete by setting event of internal caller
            A_ASSERT(pCompletionContext->pUserContext);
            *(NTSTATUS*)pCompletionContext->pUserContext = pIrp->IoStatus.Status;
            NdisSetEvent(pCompletionContext->pEvent);
        }
	}
    // clean up
    if (pCompletionContext != NULL) {
        if (pCompletionContext->pMdl != NULL) {
            IoFreeMdl(pCompletionContext->pMdl);
        }

        if (pCompletionContext->pNonPagedData != NULL) {
                ExFreePool(pCompletionContext->pNonPagedData);
        }

        ExFreePool(pCompletionContext);
    }
    if (pIrp != NULL) {
        IoFreeIrp(pIrp);
    }
    //make sure I/O manager doesn't touch Irp
    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS SdioCmd53ReadWriteAsync(
                            IN HIF_DEVICE *pAdapter,
                            IN PHIF_CMD53  pParams,
                            IN OUT PUCHAR  pData,
                            IN BOOLEAN     WriteToDevice,
                            IN PVOID       pContext,
                            IN PNDIS_EVENT pEvent)
{
    NTSTATUS                    status;
    SD_RW_EXTENDED_ARGUMENT     sdExtndArgument;
    ULONG                       length;
    PCOMPLETION_CONTEXT         pCompletionContext = NULL;
    PIRP                        pIrp = NULL;

    length = pParams->BlockLength * pParams->BlockCount;

    do {

		if (!length) {
            status = STATUS_NO_MEMORY;
            break;
		}

        //allocate a completion context for the call
        pCompletionContext = ExAllocatePoolWithTag (NonPagedPool, sizeof(COMPLETION_CONTEXT), MEM_TAG);
        if (pCompletionContext  == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWrite, Could not allocate CompletionContext \n"));
            status = STATUS_NO_MEMORY;
            break;
        }
        RtlZeroMemory(pCompletionContext, sizeof(COMPLETION_CONTEXT));

        pCompletionContext->pNonPagedData = ExAllocatePoolWithTag(NonPagedPool, length, MEM_TAG);
        if (pCompletionContext->pNonPagedData == NULL) {
             status = STATUS_NO_MEMORY;
             break;
        }

        if (WriteToDevice) {
            RtlCopyMemory(pCompletionContext->pNonPagedData, pData, length);
        }

        // need to create an MDL for the data buffer
        pCompletionContext->pMdl = IoAllocateMdl(pCompletionContext->pNonPagedData, length, FALSE, FALSE, NULL);
        if (pCompletionContext->pMdl == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWrite, Could not allocate Mdl \n"));
            status = STATUS_NO_MEMORY;
            break;
        }
        MmBuildMdlForNonPagedPool (pCompletionContext->pMdl);
        pIrp = IoAllocateIrp(HIFGetAdapter(pAdapter)->Fdo->StackSize, FALSE);
        if (pIrp == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWrite, Could not allocate Irp \n"));
            status = STATUS_NO_MEMORY;
            break;
        }
        //save the completion contexts
        pCompletionContext->pEvent = pEvent;
        pCompletionContext->pUserContext = pContext;
        pCompletionContext->pAdapter = pAdapter;

        pCompletionContext->pData  = pData;
        pCompletionContext->Length = length;

        pCompletionContext->Sdrp.Parameters.DeviceCommand.Mdl = pCompletionContext->pMdl;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.Length = length;

        pCompletionContext->Sdrp.RequestFunction = SDRF_DEVICE_COMMAND;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.Cmd = SDCMD_IO_RW_EXTENDED;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.CmdClass = SDCC_STANDARD;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.TransferDirection = (WriteToDevice)? SDTD_WRITE : SDTD_READ;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.TransferType = (pParams->BlockCount > 1) ? SDTT_MULTI_BLOCK_NO_CMD12  : SDTT_SINGLE_BLOCK;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.CmdDesc.ResponseType = SDRT_5;
        sdExtndArgument.u.bits.Count = 0;
        sdExtndArgument.u.bits.Address = pParams->Address;
        sdExtndArgument.u.bits.OpCode = pParams->Opcode;
        sdExtndArgument.u.bits.BlockMode = pParams->Mode;
        sdExtndArgument.u.bits.Function = HIFGetAdapter(pAdapter)->FunctionFocus;
        sdExtndArgument.u.bits.WriteToDevice = (WriteToDevice)? 1 : 0;
        pCompletionContext->Sdrp.Parameters.DeviceCommand.Argument = sdExtndArgument.u.AsULONG;

        // Submit the request
        status = SdBusSubmitRequestAsync(HIFGetAdapter(pAdapter)->BusInterface.Context, &pCompletionContext->Sdrp, pIrp,
                                         HifAsyncCompletionRoutine, pCompletionContext);

        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWriteAsync, failed: 0x%X\n", status));
        }
    }while(FALSE);
    if (!NT_SUCCESS (status)) {
        if (pCompletionContext != NULL) {
            if (pCompletionContext->pMdl != NULL) {
                IoFreeMdl(pCompletionContext->pMdl);
            }

            if (pCompletionContext->pNonPagedData != NULL) {
                ExFreePool(pCompletionContext->pNonPagedData);
            }
            ExFreePool(pCompletionContext);
        }
        if (pIrp != NULL) {
            IoFreeIrp(pIrp);
        }
    }
    return status;
}

NTSTATUS SdioCmd53ReadWriteSync(
                            IN HIF_DEVICE *pAdapter,
                            IN PHIF_CMD53  pParams,
                            IN OUT PUCHAR  pData,
                            IN BOOLEAN     WriteToDevice)
{
    NTSTATUS                    status;
    SD_RW_EXTENDED_ARGUMENT     sdExtndArgument;
    ULONG                       length;
    PSDBUS_REQUEST_PACKET       sdrp = NULL;
    PMDL                        pMdl = NULL;
    PVOID                       pNonPagedData = NULL;

    length = pParams->BlockLength * pParams->BlockCount;

    do {

		if (!length) {
            status = STATUS_NO_MEMORY;
            break;
		}

        sdrp = ExAllocatePoolWithTag(NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), MEM_TAG);

        if (sdrp == NULL) {
             status = STATUS_NO_MEMORY;
             break;
        }

        pNonPagedData = ExAllocatePoolWithTag(NonPagedPool, length, MEM_TAG);

        if (pNonPagedData == NULL) {
             status = STATUS_NO_MEMORY;
             break;
        }

        RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

        if (WriteToDevice) {
            RtlCopyMemory(pNonPagedData, pData, length);
        }

        // need to create an MDL for the data buffer
        pMdl = IoAllocateMdl(pNonPagedData, length, FALSE, FALSE, NULL);
        if (pMdl == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWriteSync, Could not allocate Mdl \n"));
            status = STATUS_NO_MEMORY;
            break;
        }
        MmBuildMdlForNonPagedPool (pMdl);
        sdrp->Parameters.DeviceCommand.Mdl = pMdl;
        sdrp->Parameters.DeviceCommand.Length = length;

        sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
        sdrp->Parameters.DeviceCommand.CmdDesc.Cmd = SDCMD_IO_RW_EXTENDED;
        sdrp->Parameters.DeviceCommand.CmdDesc.CmdClass = SDCC_STANDARD;
        sdrp->Parameters.DeviceCommand.CmdDesc.TransferDirection = (WriteToDevice)? SDTD_WRITE : SDTD_READ;
        sdrp->Parameters.DeviceCommand.CmdDesc.TransferType = (pParams->BlockCount > 1) ? SDTT_MULTI_BLOCK_NO_CMD12  : SDTT_SINGLE_BLOCK;
        sdrp->Parameters.DeviceCommand.CmdDesc.ResponseType = SDRT_5;
        sdExtndArgument.u.bits.Count = 0;
        sdExtndArgument.u.bits.Address = pParams->Address;
        sdExtndArgument.u.bits.OpCode = pParams->Opcode;
        sdExtndArgument.u.bits.BlockMode = pParams->Mode;
        sdExtndArgument.u.bits.Function = HIFGetAdapter(pAdapter)->FunctionFocus;
        sdExtndArgument.u.bits.WriteToDevice = (WriteToDevice)? 1 : 0;
        sdrp->Parameters.DeviceCommand.Argument = sdExtndArgument.u.AsULONG;

        // Submit the request
        status = SdBusSubmitRequest(HIFGetAdapter(pAdapter)->BusInterface.Context, sdrp);

        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd53ReadWriteSync, failed: 0x%X\n", status));
        }
        else {
             if (!WriteToDevice) {
                RtlCopyMemory(pData, pNonPagedData, length);
            }
        }
     }while(FALSE);

    if (pMdl != NULL) {
        IoFreeMdl(pMdl);
    }

    if (pNonPagedData != NULL) {
        ExFreePool(pNonPagedData);
    }

    if (sdrp != NULL) {
        ExFreePool(sdrp);
    }

    return status;
}

NTSTATUS SdioCmd52ReadWriteSync(
                              IN HIF_DEVICE *pAdapter,
                              IN OUT PUCHAR  pData,
                              IN CHAR        Function,
                              IN ULONG       Address,
                              IN BOOLEAN     WriteToDevice)
{
    NTSTATUS                    status;
    SD_RW_DIRECT_ARGUMENT       sdDirectArgument;
    PSDBUS_REQUEST_PACKET       sdrp = NULL;

    do {

        sdrp = ExAllocatePoolWithTag (NonPagedPool, sizeof(SDBUS_REQUEST_PACKET), MEM_TAG);

        if (sdrp == NULL) {
            status = STATUS_NO_MEMORY;
            break;
        }

        RtlZeroMemory(sdrp, sizeof(SDBUS_REQUEST_PACKET));

        sdrp->RequestFunction = SDRF_DEVICE_COMMAND;
        sdrp->Parameters.DeviceCommand.CmdDesc.Cmd = SDCMD_IO_RW_DIRECT;
        sdrp->Parameters.DeviceCommand.CmdDesc.CmdClass = SDCC_STANDARD;
        sdrp->Parameters.DeviceCommand.CmdDesc.TransferDirection = (WriteToDevice)? SDTD_WRITE : SDTD_READ;
        sdrp->Parameters.DeviceCommand.CmdDesc.TransferType = SDTT_CMD_ONLY;
        sdrp->Parameters.DeviceCommand.CmdDesc.ResponseType = SDRT_5;

        sdDirectArgument.u.AsULONG       = 0;
        sdDirectArgument.u.bits.Function = Function;
        sdDirectArgument.u.bits.Address  = Address;

        if (WriteToDevice) {
            sdDirectArgument.u.bits.WriteToDevice = 1;
            sdDirectArgument.u.bits.Data = *pData;
        }

        sdrp->Parameters.DeviceCommand.Argument = sdDirectArgument.u.AsULONG;


        // Submit the request
        status = SdBusSubmitRequest(HIFGetAdapter(pAdapter)->BusInterface.Context, sdrp);

        if (!NT_SUCCESS(status)) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("SdioCmd52ReadWriteSync, failed: 0x%X\n", status));
        }

        if (!WriteToDevice) {
            *pData = sdrp->ResponseData.AsUCHAR[0];
        }
    }while(FALSE);

    if (sdrp != NULL) {
        ExFreePool(sdrp);
    }

    return status;

}

#define HIF_SDIO_HIGH_SPEED_CLOCK    50000
#define HIF_SDIO_FULL_SPEED_CLOCK    25000
#define HIF_SDIO_CCCR_HS_REG_ADDR    0x13

static NTSTATUS SdioSetClock(IN HIF_DEVICE *pAdapter,
                             IN ULONG       Clock)
{
    NTSTATUS ntStatus;
    UCHAR    highSpeedReg;
    UCHAR    ehs, shs;

    ntStatus = SdioSetProperty(pAdapter,
                                 SDP_BUS_CLOCK,
                                 &Clock,
                                 sizeof(Clock));
    if(!NT_SUCCESS(ntStatus)) {
       AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: failed to set clock, 0x%X\n", __FUNCTION__, ntStatus));
       return ntStatus;
    }

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("Set Clock  clock = %d\n"), Clock, ntStatus));

    ntStatus = SdioCmd52ReadWriteSync(pAdapter, &highSpeedReg, 0, HIF_SDIO_CCCR_HS_REG_ADDR, FALSE);

    if(!NT_SUCCESS(ntStatus)) {
       AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: failed to read HS Register, 0x%X\n", __FUNCTION__, ntStatus));
       return ntStatus;
    }

    ehs = (highSpeedReg >> 1) & 0x01;
    shs = highSpeedReg & 0x01;

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("%s:EHS Bit=%d, SHS Bit=%d\n"), __FUNCTION__, ehs, shs, ntStatus));

    if((Clock == HIF_SDIO_HIGH_SPEED_CLOCK) &&
       (shs == 1) &&
       (ehs != 1)) {

        /* Force to turn on the EHS bit */
        ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("%s:Force to turn on the EHS bit\n"), __FUNCTION__));

        highSpeedReg |= 0x02;

        ntStatus = SdioCmd52ReadWriteSync(pAdapter, &highSpeedReg, 0, HIF_SDIO_CCCR_HS_REG_ADDR, TRUE);

        if(!NT_SUCCESS(ntStatus)) {
           AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: failed to set EHS, 0x%X\n", __FUNCTION__, ntStatus));
           return ntStatus;
        }
    }
    else if((Clock < HIF_SDIO_HIGH_SPEED_CLOCK) &&
            (ehs == 1)) {

        /* Force to turn off the EHS bit */
        ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("%s:Force to turn off the EHS bit\n"), __FUNCTION__));

        highSpeedReg &= ~(0x02);

        ntStatus = SdioCmd52ReadWriteSync(pAdapter, &highSpeedReg, 0, HIF_SDIO_CCCR_HS_REG_ADDR, TRUE);

        if(!NT_SUCCESS(ntStatus)) {
           AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("%s: failed to clear EHS, 0x%X\n", __FUNCTION__, ntStatus));
           return ntStatus;
        }

    }
    HIFGetAdapter(pAdapter)->sdioCLOCK = Clock;

    return ntStatus;
}

static void AR6KTargetFailureEventHandler(HIF_DEVICE *pHifDevice,
                                          A_STATUS Status)
{
    Status = Status;

    ATHR_DISPLAY_MSG (TRUE, (TEXT_MSG("********Target ASSERTION***************0x%X\n"), Status));

    HIFGetAdapter(pHifDevice)->pNic->m_TargetAssertFlag = TRUE;

    GetDbgLogs(HIFGetAdapter(pHifDevice));

    //
    // get the dbg logs from the target before shutdown
    //
    ar6000_dump_target_assert_info (pHifDevice, HIFGetAdapter(pHifDevice)->pNic->m_TargetType);
}

//
//  This function processes an device insertion event indication
//  from the HIF layer.
//
void target_register_tbl_attach(A_UINT32 target_type);

A_STATUS
HifAR6KTargetAvailableEventHandler(HIF_DEVICE *pHifDevice)
{
    struct bmi_target_info targInfo;
    HTC_INIT_INFO htcInitInfo;


    BMIInit();

    if (BMIGetTargetInfo(pHifDevice, &targInfo) != A_OK)
    {
       return A_ERROR;
    }
    HIFGetAdapter(pHifDevice)->TargetId = targInfo.target_ver;
    HIFGetAdapter(pHifDevice)->TargetType = targInfo.target_type;

	target_register_tbl_attach (HIFGetAdapter(pHifDevice)->TargetType);

    early_init_ar6000(pHifDevice, HIFGetAdapter(pHifDevice)->TargetType, HIFGetAdapter(pHifDevice)->TargetId);

    memset(&htcInitInfo,0,sizeof(htcInitInfo));
    htcInitInfo.TargetFailure  = AR6KTargetFailureEventHandler;
    htcInitInfo.pContext = pHifDevice;
    HIFGetAdapter(pHifDevice)->HTCHandle = HTCCreate(pHifDevice, &htcInitInfo);

    if (HIFGetAdapter(pHifDevice)->HTCHandle == NULL) {
        return A_ERROR;
    }

        /* need to claim the device with a non-NULL context */
    HIFClaimDevice(pHifDevice, (void *)1);

    return A_OK;
}

//
//  This function processes a device removal event indication
//  from the HIF layer.
//
static A_STATUS
HifAR6KTargetUnavailableEventHandler(HIF_DEVICE *pHifDevice)
{
    if (HIFGetAdapter(pHifDevice)->HTCHandle != NULL) {
        HTCDestroy(HIFGetAdapter(pHifDevice)->HTCHandle);
    }
    return A_OK;
}
VOID
HifGetTargetInfo(HIF_DEVICE *pHifDevice, A_UINT32 *pTargetType, A_UINT32 *pTargetId)
{
    *pTargetType = HIFGetAdapter(pHifDevice)->TargetType;
    *pTargetId = HIFGetAdapter(pHifDevice)->TargetId;
    return;
}
HTC_HANDLE
HifGetHTCHandle(HIF_DEVICE *pHifDevice)
{
    return HIFGetAdapter(pHifDevice)->HTCHandle;
}

