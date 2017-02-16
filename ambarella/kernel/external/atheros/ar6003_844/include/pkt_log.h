//------------------------------------------------------------------------------
// Copyright (c) 2005-2010 Atheros Corporation.  All rights reserved.
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

#ifndef __PKT_LOG_H__
#define __PKT_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif


/* Pkt log info */
typedef PREPACK struct pkt_log_t {
    struct info_t {
        A_UINT16    st;
        A_UINT16    end;
        A_UINT16    cur;
    }info[4096];
    A_UINT16    last_idx;
}POSTPACK PACKET_LOG;


#ifdef __cplusplus
}
#endif
#endif  /* __PKT_LOG_H__ */
