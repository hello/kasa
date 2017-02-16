//------------------------------------------------------------------------------
// <copyright file="hif_internal.h" company="Atheros">
//    Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
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
//------------------------------------------------------------------------------
//==============================================================================
// internal header file for hif layer
//
// Author(s): ="Atheros"
//==============================================================================
#include "a_config.h"
#include "ctsystem.h"
#include "sdio_busdriver.h"
#include "_sdio_defs.h"
#include "sdio_lib.h"
#define ATH_MODULE_NAME hif
#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "a_debug.h"
#include "hif.h"
#include "../../../common/hif_sdio_common.h"


#define BUS_REQUEST_MAX_NUM                64

#define SDIO_CLOCK_FREQUENCY_DEFAULT       25000000
#define SDIO_CLOCK_HS_FREQUENCY_DEFAULT    50000000
#define SDWLAN_ENABLE_DISABLE_TIMEOUT      20
#define FLAGS_CARD_ENAB                    0x02
#define FLAGS_CARD_IRQ_UNMSK               0x04

#define HIF_MBOX_BLOCK_SIZE                HIF_DEFAULT_IO_BLOCK_SIZE
#define HIF_MBOX0_BLOCK_SIZE               1
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

#define MAX_SCATTER_REQUESTS             4
#define MAX_SCATTER_ENTRIES_PER_REQ      16
#define MAX_SCATTER_REQ_TRANSFER_SIZE    16*1024

    /* full scatter gather support */
typedef struct _HIF_SCATTER_DMA_REAL_INFO {
    SDDMA_DESCRIPTOR SDSGList[MAX_SCATTER_ENTRIES_PER_REQ];   
} HIF_SCATTER_DMA_REAL_INFO;

    /* SG using DMA bounce buffer */
typedef struct _HIF_SCATTER_DMA_BOUNCE_INFO {
    SDDMA_DESCRIPTOR   SGList[1];        /* only need one for the bounce buffer, on linux the bounce buffer 
                                            is contiguous */
    A_UINT8            *pBounceBuffer;   /* bounce buffer */
    A_UINT32           BufferSize;       /* dma buffer size */
    A_UINT32           AlignmentOffset;  /* any alignment offset that needs to be applied */             
} HIF_SCATTER_DMA_BOUNCE_INFO;


#define SET_SDREQUEST_SR(p, info)       (p)->HIFPrivate[0] = (info)
#define SET_DMA_INFO_SR(p, info)        (p)->HIFPrivate[1] = (info)
#define SET_DEVICE_INFO_SR(p, info)     (p)->HIFPrivate[2] = (info)

#define GET_DMA_INFO_SR(p)              ((p)->HIFPrivate[1])
#define GET_DMA_BOUNCE_INFO_SR(p)       ((HIF_SCATTER_DMA_BOUNCE_INFO  *)GET_DMA_INFO_SR(p))
#define GET_DMA_REAL_INFO_SR(p)         ((HIF_SCATTER_DMA_REAL_INFO *)GET_DMA_INFO_SR(p))

#define GET_SDREQUEST_SR(p)             ((SDREQUEST *)((p)->HIFPrivate[0]))
#define GET_HIFDEVICE_SR(p)             ((HIF_DEVICE *)((p)->HIFPrivate[2]))

struct hif_device {
    SDDEVICE *handle;
    void     *claimedContext;
    HTC_CALLBACKS htcCallbacks;
    OSKERNEL_HELPER insert_helper;
    BOOL  helper_started;
    DL_LIST      ScatterReqHead;   
    HIF_SCATTER_METHOD  ScatterMethod;
};

typedef struct target_function_context {
    SDFUNCTION           function; /* function description of the bus driver */
    OS_SEMAPHORE         instanceSem; /* instance lock. Unused */
    SDLIST               instanceList; /* list of instances. Unused */
} TARGET_FUNCTION_CONTEXT;

typedef struct bus_request {
    struct bus_request *next;
    SDREQUEST *request;
    void *context;
    HIF_DEVICE *hifDevice;
} BUS_REQUEST;

BOOL
hifDeviceInserted(SDFUNCTION *function, SDDEVICE *device);

void
hifDeviceRemoved(SDFUNCTION *function, SDDEVICE *device);

SDREQUEST *
hifAllocateDeviceRequest(SDDEVICE *device);

void
hifFreeDeviceRequest(SDREQUEST *request);

void
hifRWCompletionHandler(SDREQUEST *request);

void
hifIRQHandler(void *context);

HIF_DEVICE *
addHifDevice(SDDEVICE *handle);

HIF_DEVICE *
getHifDevice(SDDEVICE *handle);

void
delHifDevice(SDDEVICE *handle);


A_STATUS SetupHIFScatterSupport(HIF_DEVICE *device, HIF_DEVICE_SCATTER_SUPPORT_INFO *pInfo);
void CleanupHIFScatterResources(HIF_DEVICE *device);
