//------------------------------------------------------------------------------
// <copyright file="dbglog_api.h" company="Atheros">
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
// This file contains host side debug primitives.
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef _DBGLOG_API_H_
#define _DBGLOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dbglog.h"

#define DBGLOG_HOST_LOG_BUFFER_SIZE            DBGLOG_LOG_BUFFER_SIZE

#define DBGLOG_GET_DBGID(arg) \
    ((arg & DBGLOG_DBGID_MASK) >> DBGLOG_DBGID_OFFSET)

#define DBGLOG_GET_MODULEID(arg) \
    ((arg & DBGLOG_MODULEID_MASK) >> DBGLOG_MODULEID_OFFSET)

#define DBGLOG_GET_NUMARGS(arg) \
    ((arg & DBGLOG_NUM_ARGS_MASK) >> DBGLOG_NUM_ARGS_OFFSET)

#define DBGLOG_GET_TIMESTAMP(arg) \
    ((arg & DBGLOG_TIMESTAMP_MASK) >> DBGLOG_TIMESTAMP_OFFSET)

/** 
  @param lv 0->RAW info, 1->Breif translated info, 2->Full info
  @param logbuf dbglog buffer
 */
int dbg_formater(int lv, char *output, size_t len, A_UINT32 ts, A_INT32 *logbuf);

#ifdef __cplusplus
}
#endif

#endif /* _DBGLOG_API_H_ */
