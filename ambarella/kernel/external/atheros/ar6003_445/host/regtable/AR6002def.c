/*
 * Copyright (c) 2008 Atheros Communications, Inc.
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

#if defined(AR6002_HEADERS_DEF) || defined(ATHR_WIN_DEF)
#define AR6002 1
#define AR6002_REV 2

#define UNDEFINED                          0x0fffffff

#include "common_drv.h"
#include "AR6002/hw2.0/hw/apb_map.h"
#include "AR6002/hw2.0/hw/gpio_reg.h"
#include "AR6002/hw2.0/hw/rtc_reg.h"
#include "AR6002/hw2.0/hw/si_reg.h"
#include "AR6002/hw2.0/hw/mbox_reg.h"
#include "AR6002/hw2.0/hw/mbox_host_reg.h"

#define MY_TARGET_DEF AR6002_TARGETdef
#define MY_HOST_DEF AR6002_HOSTdef
#define MY_TARGET_BOARD_DATA_SZ AR6002_BOARD_DATA_SZ
#define MY_TARGET_BOARD_EXT_DATA_SZ AR6002_BOARD_EXT_DATA_SZ
#define RTC_WMAC_BASE_ADDRESS              RTC_BASE_ADDRESS
#define RTC_SOC_BASE_ADDRESS               RTC_BASE_ADDRESS
#define WLAN_SYSTEM_SLEEP_OFFSET           SYSTEM_SLEEP_OFFSET
#define WLAN_SYSTEM_SLEEP_DISABLE_LSB      SYSTEM_SLEEP_DISABLE_LSB
#define WLAN_SYSTEM_SLEEP_DISABLE_MASK     SYSTEM_SLEEP_DISABLE_MASK
#define WLAN_RESET_CONTROL_OFFSET          RESET_CONTROL_OFFSET
#define WLAN_RESET_CONTROL_COLD_RST_MASK   RESET_CONTROL_COLD_RST_MASK
#define WLAN_RESET_CONTROL_WARM_RST_MASK   RESET_CONTROL_WARM_RST_MASK


/* Should not be used for AR6002 */
#define CLOCK_GPIO_OFFSET                  UNDEFINED
#define CLOCK_GPIO_BT_CLK_OUT_EN_LSB       UNDEFINED
#define CLOCK_GPIO_BT_CLK_OUT_EN_MASK      UNDEFINED

#ifndef ATHR_NWIFI
#include "targetdef.h"
#include "hostdef.h"
#endif
#else
#include "common_drv.h"
#include "targetdef.h"
#include "hostdef.h"
struct targetdef_s *AR6002_TARGETdef=NULL;
struct hostdef_s *AR6002_HOSTdef=NULL;
#endif /* AR6002_HEADERS_DEF */
