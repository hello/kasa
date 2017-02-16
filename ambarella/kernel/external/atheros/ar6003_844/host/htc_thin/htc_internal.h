//------------------------------------------------------------------------------
// <copyright file="htc_internal.h" company="Atheros">
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
#ifndef _HTC_INTERNAL_H_
#define _HTC_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */









/* Header files */
//#include <oal_dt.h>
//#include <oal_marc.h>

#include <adf_os_types.h>
#include <adf_os_dma.h>
#include <adf_os_timer.h>
#include <adf_os_time.h>
#include <adf_os_lock.h>
#include <adf_os_io.h>
#include <adf_os_mem.h>
#include <adf_os_module.h>

#include <adf_os_util.h>
#include <adf_os_stdtypes.h>
#include <adf_os_defer.h>
#include <adf_os_atomic.h>
//#include <adf_os_bitops.h>
#include <adf_nbuf.h>
#include <adf_net.h>

#include <athdefs.h>
//#include "a_types.h"
//#include "athdefs.h"
#include "a_osapi.h"
//#include "a_debug.h"
//#include "htc.h"
#include <htc.h>
#include "htc_host_api.h"
#include <hif.h>

#include "htc_packet.h"
#ifdef HTC_HOST_CREDIT_DIST
#include <miscdrv.h>
#endif
//#include "dev.h"

//#ifdef HTC_TEST
//#include "htc_test.h"
//#endif

/* HTC operational parameters */
#define HTC_TARGET_RESPONSE_TIMEOUT        2000 /* in ms */
#define HTC_TARGET_DEBUG_INTR_MASK         0x01
#define HTC_TARGET_CREDIT_INTR_MASK        0xF0

#define HTC_TX_QUEUE_SIZE                  256//128

struct HtcTxQ
{
    adf_nbuf_t         hdr_buf;
    adf_nbuf_t         buf;
    a_uint16_t           hdr_bufLen;
    a_uint16_t           bufLen;
};

typedef struct _HTC_ENDPOINT {
    HTC_SERVICE_ID              ServiceID;      /* service ID this endpoint is bound to
                                                   non-zero value means this endpoint is in use */
    HTC_PACKET_QUEUE            RxBuffers;      /* HTC frame buffer RX list */
#ifdef HTC_HOST_CREDIT_DIST
    HTC_ENDPOINT_CREDIT_DIST    CreditDist;     /* credit distribution structure (exposed to driver layer) */
#endif
    HTC_EP_CALLBACKS            EpCallBacks;    /* callbacks associated with this endpoint */
    a_uint32_t                    MaxTxQueueDepth;   /* max depth of the TX queue */
    a_uint32_t                    CurrentTxQueueDepth; /* current TX queue depth */
    int                         MaxMsgLength;        /* max length of endpoint message */
#ifdef HTC_EP_STAT_PROFILING
    HTC_ENDPOINT_STATS          EndPointStats;  /* endpoint statistics */
#endif
    //HTC_AGGRNUMREC_QUEUE        AggrNumRecQueue;
    //int                         CompletedTxCnt;
    //void                        **txBufArray;
    a_uint8_t                     UL_PipeID;
    a_uint8_t                     DL_PipeID;

#ifdef HTC_HOST_CREDIT_DIST
    /* TxQueue related */
    a_uint16_t                    TxBufCnt;
    a_uint16_t                    TxQHead;
    a_uint16_t                    TxQTail;
    struct HtcTxQ               HtcTxQueue[HTC_TX_QUEUE_SIZE];
    a_uint16_t                    EpSeqNum;
    a_uint16_t                    LastCreditSeq;
#endif
} HTC_ENDPOINT;

#ifdef HTC_EP_STAT_PROFILING
#define INC_HTC_EP_STAT(p,stat,count) (p)->EndPointStats.stat += (count);
#else
#define INC_HTC_EP_STAT(p,stat,count)
#endif

#if 1	/* shihhung */
#define NUM_AGGRNUM_REC_BUFFERS 128
#endif

#define HTC_SERVICE_TX_PACKET_TAG  HTC_TX_PACKET_TAG_INTERNAL

#define NUM_CONTROL_BUFFERS     8
#define NUM_CONTROL_TX_BUFFERS  2
#define NUM_CONTROL_RX_BUFFERS  (NUM_CONTROL_BUFFERS - NUM_CONTROL_TX_BUFFERS)

#define HTC_CONTROL_BUFFER_SIZE (HTC_MAX_CONTROL_MESSAGE_LENGTH + HTC_HDR_LENGTH)

typedef struct HTC_CONTROL_BUFFER {
    HTC_PACKET    HtcPacket;
    a_uint8_t       Buffer[HTC_CONTROL_BUFFER_SIZE];
} HTC_CONTROL_BUFFER;

/* our HTC target state */
typedef struct _HTC_TARGET {
    hif_handle_t                *hif_dev;
    HTC_ENDPOINT                EndPoint[HST_ENDPOINT_MAX];
	HTC_CONTROL_BUFFER          HTCControlBuffers[NUM_CONTROL_BUFFERS];
#ifdef HTC_HOST_CREDIT_DIST
    HTC_ENDPOINT_CREDIT_DIST   *EpCreditDistributionListHead;
    HTC_CREDIT_DIST_CALLBACK    DistributeCredits;
    HTC_CREDIT_INIT_CALLBACK    InitCredits;
    void                       *pCredDistContext;
#endif
    a_uint16_t                  TargetCredits;
    a_uint16_t                  TargetCreditSize;
    adf_os_spinlock_t           HTCLock;
    adf_os_spinlock_t           HTCRxLock;
    adf_os_spinlock_t           HTCTxLock;
    a_uint32_t                    HTCFlags;
    a_uint32_t                    HTCRxFlags;
    a_uint32_t                    HTCTxFlags;

    a_uint32_t                    HTCStateFlags;
    HTC_ENDPOINT_ID             EpWaitingForBuffers;
    void                       *host_handle;
    adf_os_handle_t             os_handle;
#define HTC_STATE_WAIT_BUFFERS  (1 << 0)
#define HTC_STATE_STOPPING      (1 << 1)
#ifdef HTC_CAPTURE_LAST_FRAME
    HTC_FRAME_HDR               LastFrameHdr;  /* useful for debugging */
    a_uint8_t                     LastTrailer[256];
    a_uint8_t                     LastTrailerLength;
#endif
    #ifdef HTC_TEST
    INIT_HTCTEST_INFO           InitHtcTestInfo;
    #else
    HTC_INIT_INFO               HTCInitInfo;
    #endif /* HTC_TEST */
    //HTC_AGGRNUMREC_QUEUE        FreeAggrNumRecQueue;

#ifdef MAGPIE_SINGLE_CPU_CASE
#else
    htc_spin_t                  spin;
#endif
    adf_os_mutex_t              htc_rdy_mutex;
    HTC_ENDPOINT_ID             conn_rsp_epid;
    a_uint32_t                    cfg_pipe_rsp_stat;

#ifdef HTC_HOST_CREDIT_DIST
    adf_os_timer_t              host_htc_credit_debug_timer;
#endif
} HTC_TARGET;

#define HTC_STOPPING(t) ((t)->HTCStateFlags & HTC_STATE_STOPPING)
#define LOCK_HTC(t)      adf_os_spin_lock_irq(&(t)->HTCLock, &(t)->HTCFlags);
#define UNLOCK_HTC(t)    adf_os_spin_unlock_irq(&(t)->HTCLock, &(t)->HTCFlags);
#define LOCK_HTC_RX(t)   adf_os_spin_lock_irq(&(t)->HTCRxLock, &(t)->HTCRxFlags);
#define UNLOCK_HTC_RX(t) adf_os_spin_unlock_irq(&(t)->HTCRxLock, &(t)->HTCRxFlags);
#define LOCK_HTC_TX(t)   adf_os_spin_lock_irq(&(t)->HTCTxLock, &(t)->HTCTxFlags);
#define UNLOCK_HTC_TX(t) adf_os_spin_unlock_irq(&(t)->HTCTxLock, &(t)->HTCTxFlags);

#define GET_HTC_TARGET_FROM_HANDLE(hnd) ((HTC_TARGET *)(hnd))

/* internal HTC functions */
A_STATUS HTCIssueSend(HTC_TARGET *target, 
                      adf_nbuf_t hdr_buf,
                      adf_nbuf_t netbuf, 
                      a_uint8_t SendFlags, 
                      a_uint16_t len, 
                      a_uint8_t EpID);
#if 1	/* shihhung */
hif_status_t HTCRxCompletionHandler(void *Context, adf_nbuf_t netbuf, a_uint8_t pipeID);
hif_status_t HTCTxCompletionHandler(void *Context, adf_nbuf_t netbuf);
#endif
#ifdef MAGPIE_SINGLE_CPU_CASE
void HTCWlanTxCompletionHandler(void *Context, a_uint8_t epnum);
#endif

#ifdef HTC_HOST_CREDIT_DIST
void HTCProcessCreditRpt(HTC_TARGET *target, HTC_CREDIT_REPORT *pRpt, a_uint32_t NumEntries, HTC_ENDPOINT_ID FromEndpoint);
#endif

a_uint8_t MapSvc2ULPipe(HTC_SERVICE_ID ServiceID);
a_uint8_t MapSvc2DLPipe(HTC_SERVICE_ID ServiceID);

#ifdef __cplusplus
}
#endif


#endif	/* !_HTC_HOST_INTERNAL_H_ */

