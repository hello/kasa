//------------------------------------------------------------------------------
// <copyright file="htc.c" company="Atheros">
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

#ifdef DEBUG
static ATH_DEBUG_MASK_DESCRIPTION g_HTCDebugDescription[] = {
    { ATH_DEBUG_SEND , "Send"},
    { ATH_DEBUG_RECV , "Recv"},
    { ATH_DEBUG_SYNC , "Sync"},
    { ATH_DEBUG_DUMP , "Dump Data (RX or TX)"},
}; 

ATH_DEBUG_INSTANTIATE_MODULE_VAR(htc,
                                 "htc",
                                 "Host Target Communications",
                                 ATH_DEBUG_MASK_DEFAULTS,
                                 ATH_DEBUG_DESCRIPTION_COUNT(g_HTCDebugDescription),
                                 g_HTCDebugDescription);
                                 
#endif


/* cleanup the HTC instance */
static void HTCCleanup(HTC_TARGET *target)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);


    HIF_Cleanup(target->hif_dev);  /*Todo HIF should cleanup if any buffers are there*/

#ifdef HTC_HOST_CREDIT_DIST
    adf_os_timer_cancel(&target->host_htc_credit_debug_timer);
#endif

    /* release htc_rdy_mutex */
    adf_os_mutex_release(&target->htc_rdy_mutex);

    /* free our instance */
    adf_os_mem_free(target);
    /* TODO : other cleanup */

    /* free our instance */
    A_FREE(target);
}

/* registered target arrival callback from the HIF layer */
HTC_HANDLE HTCCreate(void *hHIF, HTC_INIT_INFO *pInfo)
{
    A_STATUS    status = A_OK;
    HTC_TARGET  *target = NULL;
    
    do {
        
        if ((target = (HTC_TARGET *)A_MALLOC(sizeof(HTC_TARGET))) == NULL) {
            AR_DEBUG_PRINTF(ATH_DEBUG_ERR, ("HTC : Unable to allocate memory\n"));
            status = A_ERROR;
            break;
        }    
        
        A_MEMCPY(&target->HTCInitInfo,pInfo,sizeof(HTC_INIT_INFO));


        adf_os_mem_zero(target, sizeof(HTC_TARGET));

        adf_os_spinlock_init(&target->HTCLock);
        adf_os_spinlock_init(&target->HTCRxLock);
        adf_os_spinlock_init(&target->HTCTxLock);

        /* setup HIF layer callbacks */
        adf_os_mem_zero(&htcCallbacks, sizeof(completion_callbacks_t));
        htcCallbacks.Context = target;
        htcCallbacks.rxCompletionHandler = HTCRxCompletionHandler;
        htcCallbacks.txCompletionHandler = HTCTxCompletionHandler;

        HIFPostInit(hHIF, target, &htcCallbacks);
        
        target->hif_dev = hHIF;

        adf_os_init_mutex(&target->htc_rdy_mutex);
        adf_os_mutex_acquire(&target->htc_rdy_mutex);

        /* Get HIF default pipe for HTC message exchange */
        pEndpoint = &target->EndPoint[ENDPOINT0];
        HIFGetDefaultPipe(hHIF, &pEndpoint->UL_PipeID, &pEndpoint->DL_PipeID);
        adf_os_print("[Default Pipe]UL: %x, DL: %x\n", pEndpoint->UL_PipeID, pEndpoint->DL_PipeID);


        
        /* TODO : other init */
        
    } while (FALSE);


    target->host_handle = htcInfo.pContext;
   /* TODO INTEGRATION supply from host os handle for any os specific calls*/

    target->os_handle = NULL;

    if (A_FAILED(status)) {
        HTCCleanup(target); 
        target = NULL;
    }
    
    return (HTC_HANDLE)target;
}

void  HTCDestroy(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HTCDestroy ..  Destroying :0x%X \n",(A_UINT32)target));
    HTCCleanup(target);
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HTCDestroy \n"));
}

/* get the low level HIF device for the caller , the caller may wish to do low level
 * HIF requests */
void *HTCGetHifDevice(HTC_HANDLE HTCHandle)
{
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    return target->hif_dev;
}




A_STATUS HTCWaitTarget(HTC_HANDLE HTCHandle)
{
    A_STATUS    status = A_OK;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCWaitTarget - Enter (target:0x%X) \n", (A_UINT32)HTCHandle));

    A_STATUS status = A_OK;
    /* a_status_t wait_status;*/
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);

    HIFStart(target->hif_dev);
    adf_os_print("[%s %d] Starting mutex waiting...\n",adf_os_function, __LINE__);

    adf_os_mutex_acquire(&target->htc_rdy_mutex);

    adf_os_print("[%s %d] Finish mutex waiting...\n",adf_os_function, __LINE__); 

    return status;

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCWaitTarget - Exit\n"));

    return status;
}



/*TODO Move to specifc HIF*/
#define HIF_TXPIPE_LP 1
#define HIF_TXPIPE_MP 6
#define HIF_TXPIPE_HP 5
#define HIF_TXPIPE_HP_CREDIT 4
#define HIF_TXPIPE_MP_CREDIT 4



A_STATUS ConfigPipeCredits(HTC_TARGET *target, a_uint8_t pipeID, a_uint8_t credits);
A_STATUS HTCConfigTargetHIFPipe( HTC_HANDLE HTCHandle);



/* start HTC, this is called after all services are connected */
A_STATUS HTCConfigTargetHIFPipe( HTC_HANDLE HTCHandle){



    a_uint32_t credits_lp ;

    credits_lp = target->TargetCredits - HIF_TXPIPE_HP_CREDIT -HIF_TXPIPE_MP_CREDIT ;

    ConfigPipeCredits(target, HIF_TXPIPE_LP, (a_uint8_t)credits_lp); /* Pipe 1 = LP pipe */
    ConfigPipeCredits(target, HIF_TXPIPE_MP, (a_uint8_t)credits_mp); /* Pipe 6 = MP pipe */
    ConfigPipeCredits(target, HIF_TXPIPE_HP, (a_uint8_t)credits_hp); /* Pipe 5 = HP pipe */


}


A_STATUS ConfigPipeCredits(HTC_TARGET *target, a_uint8_t pipeID, a_uint8_t credits)
{
    A_STATUS status = A_OK;
    adf_nbuf_t netbuf;
    HTC_CONFIG_PIPE_MSG *CfgPipeCdt;


    do {
        /* allocate a buffer to send */
        //netbuf = adf_nbuf_alloc(anet, sizeof(HTC_CONFIG_PIPE_MSG), HTC_HDR_LENGTH, 0);
        netbuf = adf_nbuf_alloc(50, HTC_HDR_LENGTH, 0);

        if (netbuf == ADF_NBUF_NULL) {
            status = A_NO_MEMORY;
            break;
        }

        /* assemble config pipe message */
        CfgPipeCdt = (HTC_CONFIG_PIPE_MSG *)adf_nbuf_put_tail(netbuf, sizeof(HTC_CONFIG_PIPE_MSG));
        CfgPipeCdt->MessageID = adf_os_htons(HTC_MSG_CONFIG_PIPE_ID);
        CfgPipeCdt->PipeID = pipeID;
        CfgPipeCdt->CreditCount = credits;

        /* assemble the HTC header and send to HIF layer */
        
#ifndef MAGPIE_SINGLE_CPU_CASE
        htc_spin_prep(&target->spin);
#endif
        status = HTCIssueSend(target, 
                              ADF_NBUF_NULL,
                              netbuf, 
                              0, 
                              sizeof(HTC_CONFIG_PIPE_MSG),
                              ENDPOINT0);
        
        if (A_FAILED(status)) {
            break;    
        }
        
        htc_spin( &target->spin );
        
        if (target->cfg_pipe_rsp_stat == HTC_CONFIGPIPE_SUCCESS) {
            status = A_OK;
        }
        else {
            status = A_ERROR;
        }

    } while(FALSE);

    return status;
    
}

A_STATUS HTCStart(HTC_HANDLE HTCHandle)
{
    adf_nbuf_t netbuf;
    A_STATUS   status = A_OK;
    HTC_TARGET *target = GET_HTC_TARGET_FROM_HANDLE(HTCHandle);
    HTC_SETUP_COMPLETE_MSG *SetupComp;

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCStart Enter\n"));
    do {
         
        HTCConfigTargetHIFPipe(HTCHandle);

#ifdef HTC_HOST_CREDIT_DIST
        adf_os_assert(target->InitCredits != NULL);
        adf_os_assert(target->EpCreditDistributionListHead != NULL);
        adf_os_assert(target->EpCreditDistributionListHead->pNext != NULL);

        /* call init credits callback to do the distribution , 
         * NOTE: the first entry in the distribution list is ENDPOINT_0, so
         * we pass the start of the list after this one. */
        target->InitCredits(target->pCredDistContext, 
                            target->EpCreditDistributionListHead->pNext,
                            target->TargetCredits);

#if 1
        adf_os_timer_init(target->os_handle, &target->host_htc_credit_debug_timer, host_htc_credit_show, target);
        adf_os_timer_start(&target->host_htc_credit_debug_timer, 10000);
#endif
#endif

        /* allocate a buffer to send */
        //netbuf = adf_nbuf_alloc(anet, sizeof(HTC_SETUP_COMPLETE_MSG), HTC_HDR_LENGTH, 0);
        netbuf = adf_nbuf_alloc(50, HTC_HDR_LENGTH, 0);

        if (netbuf == ADF_NBUF_NULL) {
            status = A_NO_MEMORY;
            break;
        }

        /* assemble setup complete message */
        SetupComp = (HTC_SETUP_COMPLETE_MSG *)adf_nbuf_put_tail(netbuf, sizeof(HTC_SETUP_COMPLETE_MSG));
        SetupComp->MessageID = adf_os_htons(HTC_MSG_SETUP_COMPLETE_ID);

        /* assemble the HTC header and send to HIF layer */
        status = HTCIssueSend(target, 
                              ADF_NBUF_NULL, 
                              netbuf, 
                              0, 
                              sizeof(HTC_SETUP_COMPLETE_MSG),
                              ENDPOINT0);
        
        if (A_FAILED(status)) {
            break;    
        }

    } while (FALSE);

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("HTCStart Exit\n"));
    return status;
}


/* stop HTC communications, i.e. stop interrupt reception, and flush all queued buffers */
void HTCStop(HTC_HANDLE HTCHandle)
{
    
    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("+HTCStop \n"));
    HIF_Stop();

    AR_DEBUG_PRINTF(ATH_DEBUG_TRC, ("-HTCStop \n"));
}

#ifdef HTC_HOST_CREDIT_DIST
void host_htc_credit_show(void *arg)
{
    HTC_TARGET *target = (HTC_TARGET *)arg;
    HTC_ENDPOINT_CREDIT_DIST *pCurEpDist = target->EpCreditDistributionListHead; 
    a_uint32_t totalCredit = 0;

    while (pCurEpDist != NULL) {
        adf_os_print("ep %u: %u credits\n", pCurEpDist->Endpoint, pCurEpDist->TxCredits);
        totalCredit += pCurEpDist->TxCredits;
        pCurEpDist = pCurEpDist->pNext;
    }
    adf_os_print("host total credits: %u\n", totalCredit);

    adf_os_timer_start(&target->host_htc_credit_debug_timer, 10000);
}
#endif

void HTCDumpCreditStates(HTC_HANDLE HTCHandle)
{
    /* TODO */
}


A_BOOL HTCGetEndpointStatistics(HTC_HANDLE               HTCHandle,
                                HTC_ENDPOINT_ID          Endpoint,
                                HTC_ENDPOINT_STAT_ACTION Action,
                                HTC_ENDPOINT_STATS       *pStats)
{

#ifdef HTC_EP_STAT_PROFILING
   
    /* TODO */

    return TRUE;
#else
    return FALSE;
#endif
}


