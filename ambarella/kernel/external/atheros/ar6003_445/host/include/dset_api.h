//------------------------------------------------------------------------------
// <copyright file="dset_api.h" company="Atheros">
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
// Host-side DataSet API.
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef _DSET_API_H_
#define _DSET_API_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Host-side DataSet support is optional, and is not
 * currently required for correct operation.  To disable
 * Host-side DataSet support, set this to 0.
 */
#ifndef CONFIG_HOST_DSET_SUPPORT
#define CONFIG_HOST_DSET_SUPPORT 1
#endif

/* Called to send a DataSet Open Reply back to the Target. */
A_STATUS wmi_dset_open_reply(struct wmi_t *wmip,
                             A_UINT32 status,
                             A_UINT32 access_cookie,
                             A_UINT32 size,
                             A_UINT32 version,
                             A_UINT32 targ_handle,
                             A_UINT32 targ_reply_fn,
                             A_UINT32 targ_reply_arg);

/* Called to send a DataSet Data Reply back to the Target. */
A_STATUS wmi_dset_data_reply(struct wmi_t *wmip,
                             A_UINT32 status,
                             A_UINT8 *host_buf,
                             A_UINT32 length,
                             A_UINT32 targ_buf,
                             A_UINT32 targ_reply_fn,
                             A_UINT32 targ_reply_arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _DSET_API_H_ */
