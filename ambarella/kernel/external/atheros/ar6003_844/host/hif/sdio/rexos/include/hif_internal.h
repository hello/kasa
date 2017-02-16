/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _HIF_INTERNAL_H
#define _HIF_INTERNAL_H

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>

/* Atheros platform include files */
#include "hif.h"

#define MANUFACTURER_AR6001_ID_BASE        0x100
#define FUNCTION_CLASS                     0x0
#define MANUFACTURER_CODE                  0x271

#define BUS_REQUEST_MAX_NUM                32

#define SDIO_CLOCK_FREQUENCY_DEFAULT       24000000
#define SDIO_CLOCK_FREQUENCY_REDUCED       12000000

#define SDWLAN_ENABLE_DISABLE_TIMEOUT      20
#define FLAGS_CARD_ENAB                    0x02
#define FLAGS_CARD_IRQ_UNMSK               0x04

#define HIF_MBOX_BLOCK_SIZE                512 
#define HIF_MBOX_BASE_ADDR                 0x800
#define HIF_MBOX_WIDTH                     0x800
#define HIF_MBOX0_BLOCK_SIZE               4
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

#define HIF_MBOX_START_ADDR(mbox)                        \
    HIF_MBOX_BASE_ADDR + mbox * HIF_MBOX_WIDTH

#define HIF_MBOX_END_ADDR(mbox)                          \
    HIF_MBOX_START_ADDR(mbox) + HIF_MBOX_WIDTH - 1

struct hif_device {
    void *handle;
    void     *claimedContext;
    HTC_CALLBACKS htcCallbacks;
};

#endif
