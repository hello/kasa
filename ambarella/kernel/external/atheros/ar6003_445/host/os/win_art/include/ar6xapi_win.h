#ifndef _AR6XAPI_WIN_H_
#define _AR6XAPI_WIN_H_
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

#ifdef __cplusplus
extern "C" {
#endif

struct ar6_softc;

void ar6000_dbglog_init_done(struct ar6_softc *ar);

#ifdef SEND_EVENT_TO_APP
void ar6000_send_event_to_app(struct ar6_softc *ar, A_UINT16 eventId, A_UINT8 *datap, int len);
#endif

void ar6000_dbglog_event(struct ar6_softc *ar, A_UINT32 dropped,
                         A_INT8 *buffer, A_UINT32 length);

int ar6000_dbglog_get_debug_logs(struct ar6_softc *ar);


HTC_ENDPOINT_ID  ar6000_ac2_endpoint_id ( void * devt, A_UINT8 ac);
A_UINT8 ar6000_endpoint_id2_ac (void * devt, HTC_ENDPOINT_ID ep );


#ifdef __cplusplus
}
#endif

#endif //_AR6XAPI_WIN_H_
