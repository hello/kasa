/*
 * Copyright (c) 2010 Atheros Communications, Inc.
 * All rights reserved.
 *
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

#ifdef WIN_MOBILE7
#include <ntddk.h>
#endif

#include "common_drv.h"
#include "bmi_msg.h"

#include "targetdef.h"
#include "hostdef.h"
#include "hif.h"


/* Target-dependent addresses and definitions */
struct targetdef_s *targetdef;
/* HIF-dependent addresses and definitions */
struct hostdef_s *hostdef;

void target_register_tbl_attach(A_UINT32 target_type)
{
    switch (target_type) {
    case TARGET_TYPE_AR6002:
        targetdef = AR6002_TARGETdef;
        break;
    case TARGET_TYPE_AR6003:
        targetdef = AR6003_TARGETdef;
        break;
#ifndef ATHR_NWIFI
    case TARGET_TYPE_MCKINLEY:
        targetdef = MCKINLEY_TARGETdef;
        break;
#endif
    default:
        break;
    }
}

void hif_register_tbl_attach(A_UINT32 hif_type)
{
    switch (hif_type) {
    case HIF_TYPE_AR6002:
        hostdef = AR6002_HOSTdef;
        break;
    case HIF_TYPE_AR6003:
        hostdef = AR6003_HOSTdef;
        break;
#ifndef ATHR_NWIFI
    case HIF_TYPE_MCKINLEY:
        hostdef = MCKINLEY_HOSTdef;
        break;
#endif
    default:
        break;
    }
}

#ifndef ATHR_NWIFI
EXPORT_SYMBOL(targetdef);
EXPORT_SYMBOL(hostdef);
#endif
