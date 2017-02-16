//------------------------------------------------------------------------------
// <copyright file="htc_services.c" company="Atheros">
//    Copyright (c) 2007-2010 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#include "htc_internal.h"


A_STATUS HTCConnectService(HTC_HANDLE               HTCHandle,
                           HTC_SERVICE_CONNECT_REQ  *pConnectReq,
                           HTC_SERVICE_CONNECT_RESP *pConnectResp)
{

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HTCConnectService, target:0x%X SvcID:0x%X \n",
               (A_UINT32)HTCHandle, pConnectReq->ServiceID));

    /* TODO */

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HTCConnectService \n"));

    return A_ERROR;
}


void HTCSetCreditDistribution(HTC_HANDLE               HTCHandle,
                              void                     *pCreditDistContext,
                              HTC_CREDIT_DIST_CALLBACK CreditDistFunc,
                              HTC_CREDIT_INIT_CALLBACK CreditInitFunc,
                              HTC_SERVICE_ID           ServicePriorityOrder[],
                              int                      ListLength)
{
   /* TODO */

}



