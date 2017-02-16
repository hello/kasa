// Copyright Atheros Comunications, Inc. 2007
// HIF interface for sd_raw driver.
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
//#include "devioctl.h"
//#include <initguid.h>
#include "ntddsd_raw.h"
#include "hif.h"
#include "hif_internal.h"

#ifdef DEBUG

ATH_DEBUG_INSTANTIATE_MODULE_VAR(hif,
                                 "hif",
                                 "(Atheros SDIO) Host Interconnect Framework",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 0,
                                 NULL);
                                 
#endif //DEBUG

#define SD_IO_BLOCK_MODE 1
#define SD_IO_BYTE_MODE  0
#define SD_IO_FIXED_ADDRESS 0
#define SD_IO_INCREMENT_ADDRESS 1
#define SD_WRITE         0
#define SD_READ          1

extern A_UINT32 irqprocmode;

HIF_DEVICE hifDevice[HIF_MAX_DEVICES];

A_STATUS
HIFReadWrite(HIF_DEVICE    *hDevice,
             A_UINT32       address,
             A_UCHAR       *buffer,
             A_UINT32       length,
             A_UINT32       request,
             void          *context)
{
    //UCHAR               rw;
    UCHAR               mode;
    UCHAR               opcode;
    DWORD               blockLen, blockCount, count;
    PSDRAW_CMD53_WRITE  pBusRequest;
    A_STATUS            status = A_OK;
	DWORD               transferClass;
	//DWORD               dwArg;
	//UCHAR               command;
    DWORD               bytesreturned;
    A_BOOL              async = FALSE;
    HANDLE              handle = hDevice->handle;

	//printf("--- HIFReadWrite: Enter\n");
//    HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("HIFReadWrite:Enter\n"));
//	HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("Address 0x%x\n", address));
//    printf("Address:0x%x\n",address);
    
    // Emulate ASYNC behavior by setting the request as SYNC, and call rwCompletionHandler after the write
    if (((request & HIF_WRITE) == HIF_WRITE) && ((request & HIF_ASYNCHRONOUS) == HIF_ASYNCHRONOUS))
    {
        async = TRUE;
        request &= (~HIF_EMODE_MASK);
        request |= HIF_SYNCHRONOUS;
    }

    //this version of HIFReadWrite is restricted to cmd53 and synchronous use only       
	if (((request & HIF_SYNCHRONOUS) != HIF_SYNCHRONOUS) && ((request & HIF_EXTENDED_IO) == HIF_EXTENDED_IO))
    {
		printf("--- ***only synchronous cmd53 requests supported.\n");
		HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("***only synchronous cmd53 requests supported.\n"));
		return A_ERROR;
	}

	if (((request & HIF_BLOCK_BASIS) == HIF_BLOCK_BASIS) && ((request & HIF_EXTENDED_IO) != HIF_EXTENDED_IO))
    {
		printf ("--- Block mode not allowed for this type of command\n");
		HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Block mode not allowed for this type of command\n"));
		return A_ERROR;
	}

    if ((request & HIF_BLOCK_BASIS) == HIF_BLOCK_BASIS)
    {
        mode = SD_IO_BLOCK_MODE;
        blockLen = HIF_MBOX_BLOCK_SIZE;
        blockCount = length / HIF_MBOX_BLOCK_SIZE;
		count = blockCount;
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC,
//                        ("Block mode (BlockLen: %d, BlockCount: %d)\n",
//                        blockLen, blockCount));
    }
    else if ((request & HIF_BYTE_BASIS) == HIF_BYTE_BASIS)
    {
        mode = SD_IO_BYTE_MODE;
        blockLen = length;
        blockCount = 1;
		count = blockLen;
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC,
//                        ("Byte mode (BlockLen: %d, BlockCount: %d)\n",
//                        blockLen, blockCount));
    }
    else
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, 
                        ("Invalid data mode: %d\n", (request & HIF_DMODE_MASK)));
        return A_ERROR;
	}

    if ((request & HIF_FIXED_ADDRESS) == HIF_FIXED_ADDRESS)
    {
        opcode = SD_IO_FIXED_ADDRESS;
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("Fixed       \n"));
    }
    else if ((request & HIF_INCREMENTAL_ADDRESS) == HIF_INCREMENTAL_ADDRESS)
    {
        opcode = SD_IO_INCREMENT_ADDRESS;
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("Incremental \n"));
    }
    else
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, 
                        ("Invalid address mode: %d\n", (request & HIF_AMODE_MASK)));
        return A_ERROR;
    }

    if ((blockCount == 0) || (blockLen == 0))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, 
                        ("Invalid block count (%d) or length (%d): %d\n", blockCount, blockLen));
        return A_ERROR;
    }
    
    if ((request & HIF_WRITE) == HIF_WRITE)
    {
        transferClass = SD_WRITE;
		if ((address >= HIF_MBOX_START_ADDR(0)) && (address <= HIF_MBOX_END_ADDR(3)))
        {
            /* Mailbox write. Adjust the address so that the last byte 
               falls on the EOM address */
            address = address + HIF_MBOX_WIDTH - length;
        }
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("[Write]\n"));
	}
    else
    {
		transferClass = SD_READ;
//		HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("[Read ]\n"));
	}
        

//??	if (request->type == HIF_EXTENDED_IO) {
//??		dwArg = BUILD_IO_RW_EXTENDED_ARG(rw, mode, funcNo, 
//??			address, opcode, count);
//??		command = SD_IO_RW_EXTENDED;
//??
//??    } else if (request->type == HIF_BASIC_IO) {
//??		dwArg = BUILD_IO_RW_DIRECT_ARG(rw, SD_IO_RW_NORMAL, 
//??			funcNo, address, 0);
//??		command = SD_IO_RW_NORMAL;
//??
//??	} else {
//??        HIF_DEBUG_PRINTF(ATH_LOG_ERR, 
//??                        "Invalid command type: %d\n", request->type);
//??        return A_ERROR;
//??	}

    if ((request & HIF_SYNCHRONOUS) == HIF_SYNCHRONOUS)
    {
//        HIF_DEBUG_PRINTF(ATH_LOG_TRC, ("Synchronous\n"));

        pBusRequest = malloc(sizeof(SDRAW_CMD53) + ((transferClass == SD_READ)? 0:length));
        if (pBusRequest == NULL)
        {
            HIF_DEBUG_PRINTF(ATH_LOG_ERR, 
                        ("Can't allocate bus request\n"));
            return A_ERROR;        
        }
        pBusRequest->Params.Address = address;
        pBusRequest->Params.Opcode = opcode;
        pBusRequest->Params.Mode = mode;
        pBusRequest->Params.BlockCount = blockCount;
        pBusRequest->Params.BlockLength = blockLen;

        status = A_OK;
        if (transferClass == SD_READ)
        {
            if (!DeviceIoControl(handle, IOCTL_SDRAW_CMD53_READ, 
                                 pBusRequest, sizeof(SDRAW_CMD53),
                                 buffer, length, &bytesreturned, NULL))
            {
                HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Read failed, status: %d\n", GetLastError()));
                status =  A_ERROR;        
//HACK
printf("\n>> Unable to read Module MEMORY <<\n\n Press any key to exit\n");
//getchar();
//exit(1);
memset(buffer,0,length);
return A_ERROR;
//HACK
            }
/*
* TONY: simulate ART command error.  buffer[6] is ART cmd ID
*/
//if (buffer[6] == 0x1a)
//{
//    memset(buffer,0,length);
//    return A_ERROR;
//}
        }
        else
        {
            //write
            memcpy(pBusRequest->Data, buffer, length);
            if (!DeviceIoControl(handle, IOCTL_SDRAW_CMD53_WRITE, 
                                 pBusRequest, sizeof(SDRAW_CMD53)+length,
                                 NULL, 0, &bytesreturned, NULL))
            {
                HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Write failed, status: %d\n", GetLastError()));
                status =  A_ERROR;        
            }
        }
        if (pBusRequest != NULL)
        {
            free(pBusRequest);        
        }
        if (async == TRUE)
        {
            hDevice->htcCallbacks.rwCompletionHandler(context, status);
        }
	} 
//??    else {
//??       	HIF_DEBUG_PRINTF(ATH_LOG_TRC, "Asynchronous\n");
//??		sdStatus = SDBusRequest(device->handle, command, dwArg, transferClass,
//??					ResponseR5, blockCount, blockLen, buffer,
//??					hifRWCompletionHandler, (DWORD) context, &busRequest, 0);
//??		
//??        if (!SD_API_SUCCESS(sdStatus)) {
//??            status = A_ERROR;
//??        }
//??}

	//printf("--- HIFReadWrite: Exit OK\n");
    return status;
}

A_STATUS HIFGetBlockLength(HANDLE handle, PDWORD pBlockLength)
{
    DWORD bytesreturned;
    USHORT temp;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_GET_BLOCKLEN, 
                         NULL, 0,
                         &temp, sizeof(USHORT),
                         &bytesreturned, NULL))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to get block length, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    else
    {
        *pBlockLength = temp;
    }

    return A_OK;
}

A_STATUS HIFSetBlockLength(HANDLE handle, DWORD BlockLength)
{
    DWORD bytesreturned;
    USHORT temp = (USHORT)BlockLength;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_SET_BLOCKLEN, 
                         &temp, sizeof(USHORT),
                         NULL, 0,
                         &bytesreturned, NULL)) 
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to set block length, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    return A_OK;
}

A_STATUS HIFGetClock(HANDLE handle, PDWORD pClock)
{
    DWORD bytesreturned;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_GET_BUS_CLOCK, 
                         NULL, 0,
                         pClock, sizeof(DWORD),
                         &bytesreturned, NULL))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to get clock, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    return A_OK;
}

A_STATUS HIFSetClock(HANDLE handle, DWORD Clock)
{
    DWORD bytesreturned;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_SET_BUS_CLOCK, 
                         &Clock, sizeof(DWORD),
                         NULL, 0,
                         &bytesreturned, NULL))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to set clock, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    return A_OK;
}

A_STATUS HIFGetBusWidth(HANDLE handle, char* width)
{
    DWORD bytesreturned;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_GET_BUS_WIDTH, 
                         NULL, 0,
                         width, sizeof(UCHAR),
                         &bytesreturned, NULL))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to get clock, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    return A_OK;
}

A_STATUS HIFSetBusWidth(HANDLE handle, char width)
{
    DWORD bytesreturned;

    if (!DeviceIoControl(handle, IOCTL_SDRAW_SET_BUS_WIDTH, 
                         &width, sizeof(UCHAR),
                         NULL, 0,
                         &bytesreturned, NULL))
    {
        HIF_DEBUG_PRINTF(ATH_LOG_ERR, ("Failed to set block length, status: %d\n", GetLastError()));
        return A_ERROR;        
    }
    return A_OK;
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

            //if (configLen >= sizeof(HIF_DEVICE_MBOX_INFO)) {    
            //    SetExtendedMboxWindowInfo(SDDEVICE_GET_SDIO_MANFID(device->handle),
            //                              (HIF_DEVICE_MBOX_INFO *)config);
            //}
              
            break;
        case HIF_DEVICE_GET_IRQ_PROC_MODE:
                /* the SDIO stack allows the interrupts to be processed either way, ASYNC or SYNC */
            *((HIF_DEVICE_IRQ_PROCESSING_MODE *)config) = irqprocmode;
            break;
        //case HIF_CONFIGURE_QUERY_SCATTER_REQUEST_SUPPORT:
        //    if (nohifscattersupport) {
        //        return A_ERROR;    
        //    }
        //    return SetupHIFScatterSupport(device, (HIF_DEVICE_SCATTER_SUPPORT_INFO *)config);
        //case HIF_DEVICE_GET_OS_DEVICE:
        //    if (SD_GET_HCD_OS_DEVICE(device->handle) == NULL) {
        //        AR_DEBUG_PRINTF(ATH_DEBUG_ERR,("**** HCD OS device is NULL \n"));
        //        return A_ERROR;
        //    }
        //    ((HIF_DEVICE_OS_DEVICE_INFO *)config)->pOSDevice = SD_GET_HCD_OS_DEVICE(device->handle);
        //    break;               
        default:
            AR_DEBUG_PRINTF(ATH_DEBUG_WARN, ("Unsupported configuration opcode: %d\n", opcode));
            return A_ERROR;
    }

    return A_OK;
}

void
HIFAckInterrupt(HIF_DEVICE *device)
{
#if 0
	SDIO_STATUS status;
    DBG_ASSERT(device != NULL);
    DBG_ASSERT(device->handle != NULL);

    /* Acknowledge our function IRQ */
    status = SDLIB_IssueConfig(device->handle, SDCONFIG_FUNC_ACK_IRQ,
                               NULL, 0);
    DBG_ASSERT(SDIO_SUCCESS(status));
#endif
}

void
HIFUnMaskInterrupt(HIF_DEVICE *device)
{
#if 0
    SDIO_STATUS status;

    DBG_ASSERT(device != NULL);
    DBG_ASSERT(device->handle != NULL);

    /* Register the IRQ Handler */
    SDDEVICE_SET_IRQ_HANDLER(device->handle, hifIRQHandler, device);

    /* Unmask our function IRQ */
    status = SDLIB_IssueConfig(device->handle, SDCONFIG_FUNC_UNMASK_IRQ,
                               NULL, 0);
    DBG_ASSERT(SDIO_SUCCESS(status));
#endif
}

void HIFMaskInterrupt(HIF_DEVICE *device)
{
#if 0
    SDIO_STATUS status;
    DBG_ASSERT(device != NULL);
    DBG_ASSERT(device->handle != NULL);

    /* Mask our function IRQ */
    status = SDLIB_IssueConfig(device->handle, SDCONFIG_FUNC_MASK_IRQ,
                               NULL, 0);
    DBG_ASSERT(SDIO_SUCCESS(status));

    /* Unregister the IRQ Handler */
    SDDEVICE_SET_IRQ_HANDLER(device->handle, NULL, NULL);
#endif 
}
HIF_DEVICE *
addHifDevice(HANDLE handle)
{
    DBG_ASSERT(handle != NULL);
    hifDevice[0].handle = handle;
    return &hifDevice[0];
}

HIF_DEVICE *
getHifDevice(HANDLE handle)
{
    DBG_ASSERT(handle != NULL);
    return &hifDevice[0];
}

void
delHifDevice(HANDLE handle)
{
    DBG_ASSERT(handle != NULL);
    hifDevice[0].handle = NULL;
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
            //already in use! 
        return A_ERROR;    
    }
    device->htcCallbacks = *callbacks; 
    return A_OK;
}

void HIFDetachHTC(HIF_DEVICE *device)
{
    A_MEMZERO(&device->htcCallbacks,sizeof(device->htcCallbacks));
}


