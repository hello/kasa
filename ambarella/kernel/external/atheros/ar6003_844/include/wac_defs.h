//------------------------------------------------------------------------------
// Copyright (c) 2010 Atheros Corporation.  All rights reserved.
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
// 	Wifi driver for AR6003
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef __WAC_API_H__
#define __WAC_API_H__

typedef enum {
    WAC_SET,
    WAC_GET,
} WAC_REQUEST_TYPE;

typedef enum {
    WAC_ADD,
    WAC_DEL,
    WAC_GET_STATUS,
    WAC_GET_IE,
} WAC_COMMAND;

typedef enum {
    PRBREQ,
    PRBRSP,
    BEACON,
} WAC_FRAME_TYPE;

typedef enum {
    WAC_FAILED_NO_WAC_AP = -4,
    WAC_FAILED_LOW_RSSI = -3,
    WAC_FAILED_INVALID_PARAM = -2,
    WAC_FAILED_REJECTED = -1,
    WAC_SUCCESS = 0,
    WAC_DISABLED = 1,
    WAC_PROCEED_FIRST_PHASE,
    WAC_PROCEED_SECOND_PHASE,
} WAC_STATUS;


#endif
