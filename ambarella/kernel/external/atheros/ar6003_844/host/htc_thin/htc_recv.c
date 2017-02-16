//------------------------------------------------------------------------------
// <copyright file="htc_recv.c" company="Atheros">
//    Copyright (c) 2007-2008 Atheros Corporation.  All rights reserved.
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


void HTCUnblockRecv(HTC_HANDLE HTCHandle)
{
    /* TODO  find the Need in new model*/  
}


void HTCEnableRecv(HTC_HANDLE HTCHandle)
{

    /* TODO  find the Need in new model*/  
}

void HTCDisableRecv(HTC_HANDLE HTCHandle)
{

    /* TODO  find the Need in new model*/  
}

int HTCGetNumRecvBuffers(HTC_HANDLE      HTCHandle,
                         HTC_ENDPOINT_ID Endpoint)
{

    /* TODO  find the Need in new model*/  
    return 0;
}

static A_STATUS HTCProcessTargetReady(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    a_uint8_t MaxEps; /* It seems useless for now */
    A_STATUS status = A_OK;
    HTC_ENDPOINT *pEndpoint;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_READY_MSG *HtcRdy;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);
    HtcRdy = (HTC_READY_MSG *)netdata;

    /* Retrieve information in the HTC_Ready message */
    target->TargetCredits = adf_os_ntohs(HtcRdy->CreditCount);
    target->TargetCreditSize = adf_os_ntohs(HtcRdy->CreditSize);
    MaxEps = HtcRdy->MaxEndpoints;

	#ifdef HTC_TEST
	if (target->InitHtcTestInfo.TargetReadyEvent) {
	    HTC_READY_MSG HtcReadyMsg;

	    adf_os_mem_zero(&HtcReadyMsg, sizeof(HTC_READY_MSG));
	    HtcReadyMsg.MessageID = HTC_MSG_READY_ID;
	    HtcReadyMsg.CreditCount = (a_uint16_t)target->TargetCredits;
	    HtcReadyMsg.CreditSize = (a_uint16_t)target->TargetCreditSize;
	    HtcReadyMsg.MaxEndpoints = MaxEps;

	    target->InitHtcTestInfo.TargetReadyEvent(target->host_handle, HtcHdr, HtcReadyMsg);
	}
	#else

    #ifdef MAGPIE_SINGLE_CPU_CASE
    if (target->HTCInitInfo.TargetReadyEvent) {
       /* Notify upper layer driver of the total credits */
       target->HTCInitInfo.TargetReadyEvent(target->host_handle, target->TargetCredits);
    }
    #endif /* MAGPIE_SINGLE_CPU_CASE */

    #endif

    /* setup reserved endpoint 0 for HTC control message communications */
    pEndpoint = &target->EndPoint[ENDPOINT0];
    pEndpoint->ServiceID = HTC_CTRL_RSVD_SVC;
    pEndpoint->MaxMsgLength = HTC_MAX_CONTROL_MESSAGE_LENGTH;

    //adf_os_wake_waitq(&target->wq);
#ifndef MAGPIE_SINGLE_CPU_CASE
    adf_os_mutex_release(&target->htc_rdy_mutex);
    adf_os_print("[%s %d] wakeup mutex HTCWaitForHtcRdy!\n", adf_os_function, __LINE__);
#endif

    return status;
}

static A_STATUS HTCProcessConnectionRsp(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    A_STATUS status = A_OK;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_CONNECT_SERVICE_RESPONSE_MSG *ConnSvcRsp;
    a_uint8_t ConnectRespCode;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    ConnSvcRsp = (HTC_CONNECT_SERVICE_RESPONSE_MSG *)netdata;
    ConnectRespCode = ConnSvcRsp->Status;

    /* check response status */
    if (HTC_SERVICE_SUCCESS == ConnectRespCode) {
        a_uint16_t maxMsgSize = adf_os_ntohs(ConnSvcRsp->MaxMsgSize);
        HTC_ENDPOINT_ID assignedEndpoint = ConnSvcRsp->EndpointID;
        HTC_ENDPOINT *pAssignedEp;
        HTC_SERVICE_ID rspSvcID = adf_os_ntohs(ConnSvcRsp->ServiceID);
        HTC_ENDPOINT_ID tmpEpID;
        HTC_ENDPOINT *pEndpoint = NULL;

        #ifdef HTC_TEST
        HTC_CONNECT_SERVICE_RESPONSE_MSG HtcConnSvcRspMsg;

        HtcConnSvcRspMsg.MessageID = HTC_MSG_CONNECT_SERVICE_RESPONSE_ID;
        HtcConnSvcRspMsg.ServiceID = rspSvcID;
        HtcConnSvcRspMsg.Status = ConnectRespCode;
        HtcConnSvcRspMsg.EndpointID = assignedEndpoint;
        HtcConnSvcRspMsg.MaxMsgSize = maxMsgSize;
        HtcConnSvcRspMsg.ServiceMetaLength = ConnSvcRsp->ServiceMetaLength;
        #endif

        LOCK_HTC(target);
        pAssignedEp = &target->EndPoint[assignedEndpoint];

        /* find the temporal endpoint in which service configurations are saved */
        for (tmpEpID = (HST_ENDPOINT_MAX - 1); tmpEpID > ENDPOINT0; tmpEpID--) {
            pEndpoint = &target->EndPoint[tmpEpID];
            if (pEndpoint->ServiceID == rspSvcID) {
                /* clear the service id */
                pEndpoint->ServiceID = 0;
                break;
            }
        }
       
        if (pEndpoint == NULL) {
            status = A_ERROR;
            UNLOCK_HTC(target);
            goto out;
        }

        /* Copy the configurations saved in temporal endpoint to the specified endpoint */
        pAssignedEp->ServiceID = rspSvcID;
        pAssignedEp->MaxTxQueueDepth = pEndpoint->MaxTxQueueDepth;
        pAssignedEp->EpCallBacks = pEndpoint->EpCallBacks;
        
        pAssignedEp->UL_PipeID = pEndpoint->UL_PipeID;
        pAssignedEp->DL_PipeID = pEndpoint->DL_PipeID;

        pAssignedEp->MaxMsgLength = maxMsgSize;

#if 0
        /* initialize queues related to this endpoint */
        INIT_HTC_PACKET_QUEUE(&pAssignedEp->TxQueue);
#endif

#ifdef HTC_HOST_CREDIT_DIST
        /* set the credit distribution info for this endpoint, this information is
         * passed back to the credit distribution callback function */
        pAssignedEp->CreditDist.ServiceID = pAssignedEp->ServiceID;
        pAssignedEp->CreditDist.pHTCReserved = target;
        pAssignedEp->CreditDist.Endpoint = assignedEndpoint;
        pAssignedEp->CreditDist.TxCreditSize = target->TargetCreditSize;
        pAssignedEp->CreditDist.TxCreditsPerMaxMsg = maxMsgSize/target->TargetCreditSize;

        //adf_os_print("%s: pAssignedEp->CreditDist.TxCreditsPerMaxMsg: %u, maxMsgSize: %u, target->TargetCreditSize: %u\n", adf_os_function, pAssignedEp->CreditDist.TxCreditsPerMaxMsg, maxMsgSize, target->TargetCreditSize);

        if (0 == pAssignedEp->CreditDist.TxCreditsPerMaxMsg) {
            pAssignedEp->CreditDist.TxCreditsPerMaxMsg = 1;  
        }
#endif

        UNLOCK_HTC(target);

        #ifdef HTC_TEST
        //if (target->InitHtcTestInfo.ConnectRspEvent) {
        //    target->InitHtcTestInfo.ConnectRspEvent(target->host_handle, HtcHdr, HtcConnSvcRspMsg);
        //}
        #else
        //if (target->HTCInitInfo.ConnectRspEvent) {
        //    target->HTCInitInfo.ConnectRspEvent(target->host_handle, assignedEndpoint, rspSvcID);
        //}
        #endif

        target->conn_rsp_epid = assignedEndpoint;

    }
    else {
        
        #ifdef HTC_TEST
        //HTC_CONNECT_SERVICE_RESPONSE_MSG HtcConnSvcRspMsg;

        //HtcConnSvcRspMsg.MessageID = HTC_MSG_CONNECT_SERVICE_RESPONSE_ID;
        //HtcConnSvcRspMsg.ServiceID = adf_os_ntohs(ConnSvcRsp->ServiceID);
        //HtcConnSvcRspMsg.Status = ConnectRespCode;
        //HtcConnSvcRspMsg.EndpointID = ConnSvcRsp->EndpointID;
        //HtcConnSvcRspMsg.MaxMsgSize = adf_os_ntohs(ConnSvcRsp->MaxMsgSize);
        //HtcConnSvcRspMsg.ServiceMetaLength = ConnSvcRsp->ServiceMetaLength;
        //if (target->InitHtcTestInfo.ConnectRspEvent) {
        //    target->InitHtcTestInfo.ConnectRspEvent(target->host_handle, HtcHdr, HtcConnSvcRspMsg);
        //}
        #endif

        target->conn_rsp_epid = ENDPOINT_UNUSED;
        status = A_EPROTO;
    }
out:

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
    htc_spin_over(&target->spin);
#endif
    return status;
    
}

static A_STATUS HTCProcessConfigPipeRsp(HTC_TARGET *target, adf_nbuf_t netbuf, HTC_FRAME_HDR *HtcHdr)
{
    a_uint8_t *netdata;
    a_uint32_t netlen;
    A_STATUS status = A_OK;
    HTC_CONFIG_PIPE_RESPONSE_MSG *CfgPipeRsp;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    CfgPipeRsp = (HTC_CONFIG_PIPE_RESPONSE_MSG *)netdata;

    /* check the status of pipe configuration */
    //a_uint8_t ConfigPipeRspCode = CfgPipeRsp->Status;

#ifdef HTC_TEST
    //if (target->InitHtcTestInfo.ConfigPipeRspEvent)
    //{
    //    HTC_CONFIG_PIPE_RESPONSE_MSG HtcConfigPipeRspMsg;

    //    HtcConfigPipeRspMsg.MessageID = HTC_MSG_CONFIG_PIPE_RESPONSE_ID;
    //    HtcConfigPipeRspMsg.PipeID = CfgPipeRsp->PipeID;
    //    HtcConfigPipeRspMsg.Status = ConfigPipeRspCode;

    //    target->InitHtcTestInfo.ConfigPipeRspEvent(target->host_handle, HtcHdr, HtcConfigPipeRspMsg);
    //}
#else
    //if (HTC_CONFIGPIPE_SUCCESS == ConfigPipeRspCode && 
    //    target->HTCInitInfo.ConfigPipeRspEvent) {
        /* notify upper layer driver about the reception of pipe config response */
    //    target->HTCInitInfo.ConfigPipeRspEvent(target->host_handle, ConfigPipeRspCode);
    //}
    //else {
    //    status = A_EPROTO;
    //}
#endif

    target->cfg_pipe_rsp_stat = CfgPipeRsp->Status;
#ifdef MAGPIE_SINGLE_CPU_CASE
#else
    htc_spin_over(&target->spin);
#endif
    return status;
						
}

static A_STATUS HTCProcessRxTrailer(HTC_TARGET *target,
                                    adf_nbuf_t netbuf,
                                    a_uint8_t *netdata, 
                                    a_uint32_t netlen, 
                                    HTC_FRAME_HDR *HtcHdr)
{
    A_STATUS status = A_OK;
    a_uint8_t trailerLen = HtcHdr->ControlBytes[0];
    a_uint16_t payloadLen = adf_os_ntohs(HtcHdr->PayloadLen);
    //adf_net_handle_t anet = target->pInstanceContext;
#ifdef HTC_HOST_CREDIT_DIST
    HTC_RECORD_HDR *HtcRecHdr; 
    a_uint8_t len_tmp;
    a_uint8_t *pRecordBuf;
#endif

    /* advances the pointer to beginning of the trailer */
    netdata += (HTC_HDR_LENGTH + payloadLen - trailerLen);

#ifdef HTC_HOST_CREDIT_DIST
    len_tmp = trailerLen;

    while (len_tmp > 0) {
        //a_uint8_t recordType, recordLen;

        if (len_tmp < sizeof(HTC_RECORD_HDR)) {
            status = A_EPROTO;
            break;
        }
        HtcRecHdr = (HTC_RECORD_HDR *)netdata;
        len_tmp -= sizeof(HTC_RECORD_HDR);
        netdata += sizeof(HTC_RECORD_HDR);

        if (len_tmp < HtcRecHdr->Length) {
            status = A_EPROTO;
            break;
        }

        pRecordBuf = netdata;

        switch(HtcRecHdr->RecordID) {
            case HTC_RECORD_CREDITS:
            case HTC_RECORD_CREDITS_1_1:
                /* Process credit report */
                HTCProcessCreditRpt(target,
                                    (HTC_CREDIT_REPORT *)pRecordBuf, 
                                    HtcRecHdr->Length ,
                                    HtcHdr->EndpointID);
                break;
        }

        /* Proceed to handle next record */
        len_tmp -= HtcRecHdr->Length;
        netdata += (/*sizeof(HTC_RECORD_HDR) + */ HtcRecHdr->Length);

    }
#endif    /* HTC_HOST_CREDIT_DIST */

    /* remove received trailer */
    adf_nbuf_trim_tail(netbuf, trailerLen);

    return status;

}
hif_status_t
HTCRxCompletionHandler(void *Context, adf_nbuf_t netbuf, a_uint8_t pipeID)
{
    A_STATUS status = A_OK;
    HTC_FRAME_HDR *HtcHdr;
    HTC_TARGET *target = (HTC_TARGET *)Context;
    //adf_os_handle_t os_hdl = target->os_handle;
    a_uint8_t *netdata;
    a_uint32_t netlen;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);
    //shliu: get correct whole nbuf length
    netlen = adf_nbuf_len(netbuf);

    HtcHdr = (HTC_FRAME_HDR *)netdata;

    if (HtcHdr->EndpointID >= HST_ENDPOINT_MAX) {
        adf_os_print("%s: invalid EndpointID=%d\n", adf_os_function ,HtcHdr->EndpointID);
        adf_nbuf_free(netbuf);
        return HIF_ERROR;
    }

    /* If the endpoint ID is 0, 
     * get the message ID and invoke the corresponding callbacks */
    if (HtcHdr->EndpointID == ENDPOINT0) {
    
        if (HtcHdr->Flags == 0) {
            HTC_UNKNOWN_MSG *HtcMsg = (HTC_UNKNOWN_MSG *)(netdata + sizeof(HTC_FRAME_HDR));
            
            adf_nbuf_pull_head(netbuf, sizeof(HTC_FRAME_HDR));  //RAY
            switch(adf_os_ntohs(HtcMsg->MessageID)) {
                case HTC_MSG_READY_ID:
                    HTCProcessTargetReady(target, netbuf, HtcHdr);
                    break;
                case HTC_MSG_CONNECT_SERVICE_RESPONSE_ID:
                    HTCProcessConnectionRsp(target, netbuf, HtcHdr);
                    break;
                case HTC_MSG_CONFIG_PIPE_RESPONSE_ID:
                    HTCProcessConfigPipeRsp(target, netbuf, HtcHdr);
                    break;
		    }
        }
        else if (HtcHdr->Flags & HTC_FLAGS_RECV_TRAILER) {
            #ifdef HTC_TEST
            if (target->InitHtcTestInfo.Ep0CreditRptEvent) {
                target->InitHtcTestInfo.Ep0CreditRptEvent(target->host_handle, netbuf);

                /* Because Ep0CreditRptEvent will free the buffer, 
                 * skip the action to free buffer 
                 */
                goto out;
            }
            #else
            status = HTCProcessRxTrailer(target, netbuf, netdata, netlen, HtcHdr);
            #endif
        }

		/* !!!!!! free the buffer of HTC control messages */
		adf_nbuf_free(netbuf);

	}
	else { /* If the endpoint ID is not 0, invoke the registered EP's callback */
        do {
            HTC_ENDPOINT *pEndpoint = &target->EndPoint[HtcHdr->EndpointID];

            #ifdef HTC_TEST
            #else
            if (HtcHdr->Flags & HTC_FLAGS_RECV_TRAILER) {
                status = HTCProcessRxTrailer(target, netbuf, netdata, netlen, HtcHdr);

                if (A_FAILED(status)) {
                    status = A_EPROTO;
                    break;
                }
            }

            /* remove HTC header */
            adf_nbuf_pull_head(netbuf, HTC_HDR_LENGTH);

            #endif

            if (pEndpoint->EpCallBacks.EpRecv) {
                #ifdef HTC_TEST
                pEndpoint->EpCallBacks.EpRecv(pEndpoint->EpCallBacks.pContext, netbuf, HtcHdr->EndpointID, pipeID);
                #else
                /* give the packet to the upper layer */
                pEndpoint->EpCallBacks.EpRecv(pEndpoint->EpCallBacks.pContext, netbuf, HtcHdr->EndpointID);
                #endif
            }
            else {
                adf_os_print("EpRecv callback is not registered! HTC is going to free the net buffer.\n");
                adf_nbuf_free(netbuf);
            }
        } while(FALSE);
    }

#ifdef HTC_TEST
out:
#endif
	return ((status == 0) ? HIF_OK : HIF_ERROR);
	
}

