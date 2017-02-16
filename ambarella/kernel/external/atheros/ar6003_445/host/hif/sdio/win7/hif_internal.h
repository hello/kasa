//------------------------------------------------------------------------------
// <copyright file="hif_internal.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------
//==============================================================================
//
// Author(s): ="Atheros"
//==============================================================================

#ifndef HIF_INTERNAL_H
#define HIF_INTERNAL_H

#include <mp_main.h>


#ifndef AR_DEBUG_PRINTF
#define AR_DEBUG_PRINTF
#endif

#ifndef ATH_DEBUG_TRACE
#define ATH_DEBUG_TRACE
#endif

typedef struct hif_device HIF_DEVICE;
static INLINE PATH_ADAPTER HIFGetAdapter(HIF_DEVICE *pHif) {
    return (PATH_ADAPTER)pHif;
}
typedef struct _WDF_DEVICE_INFO{

    PATH_ADAPTER pAdapter;

} WDF_DEVICE_INFO, *PWDF_DEVICE_INFO;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(WDF_DEVICE_INFO, GetWdfDeviceInfo);

#define SD_IO_BYTE_MODE              0
#define SD_IO_BLOCK_MODE             1
#define SD_IO_FIXED_ADDRESS          0
#define SD_IO_INCREMENT_ADDRESS      1
typedef struct _SDRAW_CMD53 {
    ULONG Address;          //address to read/write
    UCHAR Opcode;           // 0 - fixed address 1 - incremetal
    UCHAR Mode;             // 0 - byte mode 1 - block mode
    ULONG BlockCount;       // number of blocks to transfer
    ULONG BlockLength;      // block length of transfer
}HIF_CMD53, *PHIF_CMD53;

#if (defined(HIF_SINGLE_BLOCK_WAR) && defined(HIF_SDIO_LARGE_BLOCK_MODE))

    /* reduce block size to minimize padding */
#define HIF_MBOX_BLOCK_SIZE                 64
#define WLAN_MAX_SEND_FRAME                 1536
#define WLAN_MAX_RECV_FRAME                 (1536 + HIF_MBOX_BLOCK_SIZE) /* rx + lookaheads and credit rpts */

    /* the block size for large block mode is the set to the largest frame size */
#define HIF_SDIO_MAX_BYTES_LARGE_BLOCK_MODE  WLAN_MAX_RECV_FRAME

#else

#define HIF_MBOX_BLOCK_SIZE                128

#endif

#define HIF_MBOX_BASE_ADDR                 0x800
#define HIF_MBOX_WIDTH                     0x800
#define HIF_MBOX0_BLOCK_SIZE               1
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

#define HIF_MBOX_START_ADDR(mbox)                        \
    HIF_MBOX_BASE_ADDR + mbox * HIF_MBOX_WIDTH

#define HIF_MBOX_END_ADDR(mbox)                          \
    HIF_MBOX_START_ADDR(mbox) + HIF_MBOX_WIDTH - 1


A_STATUS
HifAR6KTargetAvailableEventHandler(HIF_DEVICE *pHifDevice);

HTC_HANDLE
HifGetHTCHandle(HIF_DEVICE *pHifDevice);

VOID
HifGetTargetInfo(HIF_DEVICE *pHifDevice, A_UINT32 *pTargetType, A_UINT32 *pTargetId);

NTSTATUS
HIFDeviceInit(HIF_DEVICE *pAdapter);

NTSTATUS
HIFDeviceReInit (HIF_DEVICE *pAdapter);

#endif

