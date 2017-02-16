//------------------------------------------------------------------------------
// <copyright file="htc_send.c" company="Atheros">
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


A_STATUS HTCSendPktsMultiple(HTC_HANDLE HTCHandle, adf_nubf_queue_t *nbufQueue ,HTC_ENDPOINT_ID Ep)
{

    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(hHTC);
    A_STATUS status = A_OK;
    HTC_ENDPOINT *pEndpoint;
    a_uint16_t payloadLen;
    a_uint8_t SendFlags = 0;

    a_uint32_t CreditAvailable;
    adf_nubf_queue_t nbufQueueTemp;
    adf_nbuf_t nbuf;


    AR_DEBUG_PRINTF(ATH_DEBUG_SEND, ("+HTCSendPktsMultiple: Queue: 0x%X, Pkts %d \n",
                    (A_UINT32)pPktQueue, HTC_PACKET_QUEUE_DEPTH(pPktQueue)));
    
    /* TODO send HTC packets to HIF layer */

    pEndpoint = &target->EndPoint[Ep];
#ifndef HTC_HOST_CREDIT_DIST
   hiffreeq = HIFGetFreeQueueNumber(target->hif_dev, pEndpoint->UL_PipeID) ;
#endif   
   
   adf_nbuf_queue_init(&nbufQueueTemp);
   do {

       if ( HIFGetFreeQueueNumber(target->hif_dev, pEndpoint->UL_PipeID) == 0 )
           break;
       nbuf  = adf_nbuf_queue_first(nbufQueue);
       if(nbuf ==NULL)
           break;
       payloadLen = (a_uint16_t) adf_nbuf_len(buf);

#ifdef HTC_HOST_CREDIT_DIST
       /* check if credits are enough for sending this packet */
#ifdef WMI_RETRY
       if(Ep != 1 )
#endif
       {
           status = HTCCheckCredits(target, HTC_HDR_LENGTH + payloadLen, pEndpoint, &SendFlags);
           if (A_FAILED(status)) {
               //adf_os_print("credits are not enough for sending the message!\n"); 
               break;
           }
       }
#endif
       status = HTCSendPrepare(target, buf, SendFlags, payloadLen, Ep);


       nbuf = adf_nbuf_queue_remove(nbufQueue);
       adf_nbuf_queue_add(&nbufQueueTemp,nbuf);
   }while(TRUE);

   status = HIFSendMultiple(target->hif_dev, pipeID, &nbufQueueTemp);

   if((status !=A_OK) || (!adf_nbuf_is_queue_empty(&nbufQueueTemp)))
   {
       adf_os_printf("Not to be expected as before sending we have checked the availibility \n");
       while(!adf_nbuf_is_queue_empty(&nbufQueueTemp)){
           nbuf = adf_nbuf_queue_remove(&nbufQueueTemp)

#ifdef HTC_HOST_CREDIT_DIST
               /* reclaim the credit due to failure to send */
#ifdef MAGPIE_HIF_USB
               if (pEndpoint->UL_PipeID == 1) /* only handle endpoints through USB LP pipe
                                                 case */
#endif
               {   

                   HTCReclaimCredits(target, adf_nbuf_len(nbuf), pEndpoint);
               }

#endif

           adf_nbuf_free(nbuf);
       }
   } 


   if(!adf_nbuf_is_queue_empty(&nbufQueueTemp) )
     status = A_ERROR;

    AR_DEBUG_PRINTF(ATH_DEBUG_SEND, ("-HTCSendPktsMultiple \n"));

    return status;   
}

A_STATUS HTCSendPrepare(HTC_TARGET *target,
        adf_nbuf_t netbuf,
        a_uint8_t SendFlags,
        a_uint16_t len,
        a_uint8_t EpID)
{
    A_STATUS status = A_OK;
    HTC_ENDPOINT *pEndpoint = &target->EndPoint[EpID];
    HTC_FRAME_HDR *HtcHdr;
#ifdef HTC_HOST_CREDIT_DIST  
    a_uint8_t creditsused ,remainder ;
#endif


    /* setup HTC frame header */
    HtcHdr = (HTC_FRAME_HDR *)adf_nbuf_push_head(netbuf, sizeof(HTC_FRAME_HDR));

    adf_os_assert(HtcHdr);

    HtcHdr->EndpointID = EpID;
    HtcHdr->Flags = SendFlags;
    HtcHdr->PayloadLen = adf_os_htons(len);

#ifdef HTC_HOST_CREDIT_DIST
    creditsused = (len / target->TargetCreditSize);
    remainder   = (len % target->TargetCreditSize);

    if (remainder) {
        creditsused++;
    }

    HTC_SEQADD(pEndpoint->EpSeqNum,creditsused,HTC_SEQ_MAX);
    HtcHdr->HostSeqNum = adf_os_htons(pEndpoint->EpSeqNum);
#endif

    return A_OK;
}



/* HTC API - HTCSendPkt */
A_STATUS    HTCSendPkt(HTC_HANDLE HTCHandle, adf_nbuf_t nbuf,HTC_ENDPOINT_ID Ep)

{
    adf_nubf_queue_t nbufQueueTemp;
    adf_nbuf_queue_init(&nbufQueueTemp); 

    AR_DEBUG_PRINTF(ATH_DEBUG_SEND,
                    ("+-HTCSendPkt: Enter endPointId: %d, buffer: 0x%X, length: %d \n",
                    pPacket->Endpoint, (A_UINT32)pPacket->pBuffer, pPacket->ActualLength));                   

    adf_nbuf_queue_add(&nbufQueueTemp,nbuf);


    return A_STATUS HTCSendPktsMultiple(HTCHandle, &nbufQueueTemp ,Ep)
}


hif_status_t HTCTxCompMultHandler(void *Context, adf_nbuf_queue_t * nbufqueue){

    adf_nbuf_t nbuf;

    HTC_FRAME_HDR *HtcHdr;
    a_uint8_t EpID;


    
    nbuf  = adf_nbuf_queue_first(nbufQueue);
    if(nbuf==NULL)
        return 0;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    HtcHdr = (HTC_FRAME_HDR *)netdata;
    EpID = HtcHdr->EndpointID;


    if (EpID == ENDPOINT0) {
        while( nbuf= adf_nbuf_queue_remove(nbufqueue))
        adf_nbuf_free(nbuf);
    }
    else {
        /* gather tx completion counts */
        HTC_ENDPOINT *pEndpoint = &target->EndPoint[EpID];
        /* nofity upper layer */            
        if (pEndpoint->EpCallBacks.EpTxCompleteMultiple) {
            /* give the packet to the upper layer */
            pEndpoint->EpCallBacks.EpTxCompleteMultiple(pEndpoint->EpCallBacks.pContext, nbufqueue , EpID);

        }
        else {
            while( nbuf= adf_nbuf_queue_remove(nbufqueue))
                adf_nbuf_free(netbuf);
        }

    }

}


hif_status_t HTCTxCompletionHandler(void *Context, adf_nbuf_t netbuf)
{
    HTC_TARGET *target = (HTC_TARGET *)Context;
    a_uint8_t *netdata;
    a_uint32_t netlen;
    HTC_FRAME_HDR *HtcHdr;
    a_uint8_t EpID;

    adf_nbuf_peek_header(netbuf, &netdata, &netlen);

    HtcHdr = (HTC_FRAME_HDR *)netdata;

    EpID = HtcHdr->EndpointID;

    if (EpID == ENDPOINT0) {
        adf_nbuf_free(netbuf);
    }
    else {
        /* gather tx completion counts */
        HTC_ENDPOINT *pEndpoint = &target->EndPoint[EpID];
        /* nofity upper layer */            
        if (pEndpoint->EpCallBacks.EpTxComplete) {
            /* give the packet to the upper layer */
            pEndpoint->EpCallBacks.EpTxComplete(/*dev*/pEndpoint->EpCallBacks.pContext, &netbuf, EpID);

        }
        else {
            adf_nbuf_free(netbuf);
        }

}

return HIF_OK;
}



#ifdef HTC_HOST_CREDIT_DIST
A_STATUS HTCReclaimCredits(HTC_TARGET *target,
                           a_uint32_t msglen,
                           HTC_ENDPOINT *pEndpoint)
{
    a_uint32_t creditsRequired, remainder;

    /* figure out how many credits this message requires */
    creditsRequired = (msglen / target->TargetCreditSize);
    remainder = (msglen % target->TargetCreditSize);

    if (remainder) {
        creditsRequired++;
    }

    LOCK_HTC_TX(target);

    pEndpoint->CreditDist.TxCredits += creditsRequired;

    UNLOCK_HTC_TX(target);

    return A_OK;
}

A_STATUS HTCCheckCredits(HTC_TARGET *target,
                         a_uint32_t msglen,
                         HTC_ENDPOINT *pEndpoint,
                         a_uint8_t *pSendFlags)
{
    a_uint32_t creditsRequired, remainder;

    /* figure out how many credits this message requires */
    creditsRequired = (msglen / target->TargetCreditSize);
    remainder = (msglen % target->TargetCreditSize);

    if (remainder) {
        creditsRequired++;
    }

    LOCK_HTC_TX(target);

    //adf_os_print("creditsRequired: %u, msglen: %u, target->TargetCreditSize: %u, pEndpoint->CreditDist.TxCredits: %u\n", creditsRequired, msglen, target->TargetCreditSize, pEndpoint->CreditDist.TxCredits);

    /* not enough credits */
    if (pEndpoint->CreditDist.TxCredits < creditsRequired) {

        /* set how many credits we need  */
        pEndpoint->CreditDist.TxCreditsSeek = creditsRequired - pEndpoint->CreditDist.TxCredits;

        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEEK_CREDITS,
                        "Seek Credits",
                        &pEndpoint->CreditDist);

        pEndpoint->CreditDist.TxCreditsSeek = 0;

        if (pEndpoint->CreditDist.TxCredits < creditsRequired) {
            /* still not enough credits to send, leave packet in the queue */
            UNLOCK_HTC_TX(target);
            //adf_os_print("htc: credit is unavailable: %u, pEndpoint->CreditDist.TxCredits: %u, creditsRequired: %u\n", A_CREDIT_UNAVAILABLE, pEndpoint->CreditDist.TxCredits, creditsRequired);
            return A_CREDIT_UNAVAILABLE;
        }
    }

    pEndpoint->CreditDist.TxCredits -= creditsRequired;
    /* check if we need credits back from the target */
    if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
        /* we are getting low on credits, see if we can ask for more from the distribution function */
        pEndpoint->CreditDist.TxCreditsSeek = pEndpoint->CreditDist.TxCreditsPerMaxMsg - pEndpoint->CreditDist.TxCredits;

        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEEK_CREDITS,
                        "Seek Credits",
                        &pEndpoint->CreditDist);

        pEndpoint->CreditDist.TxCreditsSeek = 0;

        if (pEndpoint->CreditDist.TxCredits < pEndpoint->CreditDist.TxCreditsPerMaxMsg) {
            /* tell the target we need credits ASAP! */
            *pSendFlags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
            //adf_os_print("SendFlags: %u\n", *pSendFlags);
        }
    }

    UNLOCK_HTC_TX(target);
    return A_OK;
}




/* process credit reports and call distribution function */
void HTCProcessCreditRpt(HTC_TARGET *target, HTC_CREDIT_REPORT *pRpt0, a_uint32_t RecLen, HTC_ENDPOINT_ID FromEndpoint)
{
    a_uint32_t i;
    a_uint32_t NumEntries;
#ifdef HTC_HOST_CREDIT_DIST
    a_uint32_t totalCredits = 0;
    a_uint8_t doDist = FALSE;
    HTC_ENDPOINT *pEndpoint;
    a_uint16_t seq_diff;
    a_uint16_t tgt_seq;
    HTC_CREDIT_REPORT_1_1 *pRpt = (HTC_CREDIT_REPORT_1_1 *)pRpt0 ;
#else
    HTC_CREDIT_REPORT *pRpt = (HTC_CREDIT_REPORT *)pRpt0 ;
#endif    

#ifdef HTC_HOST_CREDIT_DIST
    NumEntries = RecLen / (sizeof(HTC_CREDIT_REPORT_1_1)) ;
#else
    NumEntries = RecLen / (sizeof(HTC_CREDIT_REPORT)) ;
#endif        
    /* lock out TX while we update credits */
    LOCK_HTC_TX(target);

    for (i = 0; i < NumEntries; i++, pRpt++) {

        if (pRpt->EndpointID >= HST_ENDPOINT_MAX) {
            adf_os_assert(0);
            break;
        }

#ifdef HTC_HOST_CREDIT_DIST
        pEndpoint = &target->EndPoint[pRpt->EndpointID];

        tgt_seq =  adf_os_ntohs(pRpt->TgtCreditSeqNo);
        if (ENDPOINT0 == pRpt->EndpointID) {
            /* always give endpoint 0 credits back */
            seq_diff =  (tgt_seq - pEndpoint->LastCreditSeq)  & (HTC_SEQ_MAX -1);

            pEndpoint->CreditDist.TxCredits += seq_diff;
            pEndpoint->LastCreditSeq = tgt_seq;

        }
        else {
            /* for all other endpoints, update credits to distribute, the distribution function
             * will handle giving out credits back to the endpoints */
            seq_diff =  (tgt_seq - pEndpoint->LastCreditSeq)  & (HTC_SEQ_MAX -1);
            pEndpoint->CreditDist.TxCreditsToDist += seq_diff;
            pEndpoint->LastCreditSeq = tgt_seq;
            /* flag that we have to do the distribution */
            doDist = TRUE;
        }

        totalCredits += seq_diff;
#endif

#if 0    /* Sched Next Event Triggering */
        UNLOCK_HTC_TX(target);

        /* HTC control endpoint & WMI endpoint should be excluded to trigger sched next event */
        if (target->HTCInitInfo.HTCSchedNextEvent && pRpt->EndpointID > ENDPOINT1) {
            /* trigger HTC Sched Next Event */
            target->HTCInitInfo.HTCSchedNextEvent(target->host_handle, pRpt->EndpointID);
        }

        LOCK_HTC_TX(target);
#endif
        
    }

#ifdef HTC_HOST_CREDIT_DIST
    if (doDist) {
        /* this was a credit return based on a completed send operations
         * note, this is done with the lock held */       
        DO_DISTRIBUTION(target,
                        HTC_CREDIT_DIST_SEND_COMPLETE,
                        "Send Complete",
                        target->EpCreditDistributionListHead->pNext);        
    }
#endif

    UNLOCK_HTC_TX(target);
    
#if 0    /* HTC queue has been removed. */
    if (totalCredits) {
        HTCCheckEndpointTxQueues(target);    
    }
#endif

}
#endif    /* HTC_HOST_CREDIT_DIST */



/* HTC API to flush an endpoint's TX queue*/
void HTCFlushEndpoint(HTC_HANDLE HTCHandle, HTC_ENDPOINT_ID Endpoint, HTC_TX_TAG Tag)
{
    /* TODO */
}

/* HTC API to indicate activity to the credit distribution function */
void HTCIndicateActivityChange(HTC_HANDLE      HTCHandle,
                               HTC_ENDPOINT_ID Endpoint,
                               A_BOOL          Active)
{
   /*  TODO */
}

A_BOOL HTCIsEndpointActive(HTC_HANDLE      HTCHandle,
                           HTC_ENDPOINT_ID Endpoint)
{
    /* TODO */
    return FALSE;
}


