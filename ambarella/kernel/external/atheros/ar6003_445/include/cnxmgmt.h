//------------------------------------------------------------------------------
// <copyright file="cnxmgmt.h" company="Atheros">
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
// Author(s): ="Atheros"
//==============================================================================

#ifndef _CNXMGMT_H_
#define _CNXMGMT_H_

typedef enum {
    CM_CONNECT_WITHOUT_SCAN             = 0x0001,
    CM_CONNECT_ASSOC_POLICY_USER        = 0x0002,
    CM_CONNECT_SEND_REASSOC             = 0x0004,
    CM_CONNECT_WITHOUT_ROAMTABLE_UPDATE = 0x0008,
    CM_CONNECT_DO_WPA_OFFLOAD           = 0x0010,
    CM_CONNECT_DO_NOT_DEAUTH            = 0x0020,
} CM_CONNECT_TYPE;

#endif  /* _CNXMGMT_H_ */
