//------------------------------------------------------------------------------

// <copyright file="ar6kap_common.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
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

// This file contains the definitions of common AP mode data structures.
//
// Author(s): ="Atheros"
//==============================================================================

#ifndef _AR6KAP_COMMON_H_
#define _AR6KAP_COMMON_H_
/*
 * Used with AR6000_XIOCTL_AP_GET_STA_LIST
 */
typedef struct {
    A_UINT8     mac[ATH_MAC_LEN];
    A_UINT8     aid;
    A_UINT8     keymgmt;
    A_UINT8     ucipher;
    A_UINT8     auth;
    A_UINT8     wmode;
} station_t;

typedef struct {
    station_t sta[AP_MAX_NUM_STA];
} ap_get_sta_t;
#endif /* _AR6KAP_COMMON_H_ */
