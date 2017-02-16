#ifndef _HIF_INTERNAL_H
#define _HIF_INTERNAL_H
/*
 *
 * Copyright (c) 2004-2007 Atheros Communications Inc.
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
 */

#include "ar6000_cs.h"
#include "hif.h"

#define MANUFACTURER_ID_BASE               0x100
#define FUNCTION_CLASS                     0x0
#define MANUFACTURER_CODE                  0x271

#define HIF_MBOX_BLOCK_SIZE                2
#define HIF_MBOX_BASE_ADDR                 0x0
#define HIF_MBOX_WIDTH                     0x100
#define HIF_MBOX0_BLOCK_SIZE               1
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

#define BUS_REQUEST_MAX_NUM                32

#define HIF_MBOX_START_ADDR(mbox)                        \
    HIF_MBOX_BASE_ADDR + mbox * HIF_MBOX_WIDTH

#define HIF_MBOX_END_ADDR(mbox)	                         \
    HIF_MBOX_START_ADDR(mbox) + HIF_MBOX_WIDTH - 1

#define HIF_IS_MBOX_ADDR(addr) (addr >= HIF_MBOX_START_ADDR(0) && addr <= HIF_MBOX_END_ADDR(3)) ? 1:0

struct hif_device {
    PCFDEVICE handle;
    void *htc_handle;
};

typedef struct target_function_context {
    CFFUNCTION           function; /* function description of the bus driver */

//    OS_SEMAPHORE         instanceSem; /* instance lock. Unused */
//    SDLIST               instanceList; /* list of instances. Unused */

} TARGET_FUNCTION_CONTEXT;

typedef struct bus_request {
    A_BOOL     free;
    CFREQUEST  request;
} BUS_REQUEST;

static A_BOOL
hifDeviceInserted(CFFUNCTION *function, PCFDEVICE device);

static void
hifDeviceRemoved(CFFUNCTION *function, PCFDEVICE device);

static HIF_DEVICE *
addHifDevice(PCFDEVICE handle);

static HIF_DEVICE *
getHifDevice(PCFDEVICE handle);

static void
delHifDevice(PCFDEVICE handle);

#endif //_HIF_INTERNAL_H
