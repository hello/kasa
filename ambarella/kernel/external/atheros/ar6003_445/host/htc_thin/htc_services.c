//------------------------------------------------------------------------------
// <copyright file="htc_services.c" company="Atheros">
//    Copyright (c) 2007-2010 Atheros Corporation.  All rights reserved.
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



