/**
 * filters_interface.cpp
 *
 * History:
 *  2012/12/12 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "osal.h"

#include "framework.h"
#include "filters_interface.h"

extern IFilter *gfCreateDemuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateScheduledMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateStreamingTransmiterFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

IFilter *gfFilterFactory(EFilterType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
{
    IFilter *thiz = NULL;

    switch (type) {

        case EFilterType_Demuxer:
            thiz = gfCreateDemuxerFilter("CDemuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoDecoder:
            thiz = gfCreateVideoDecoderFilter("CVideoDecoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioDecoder:
            thiz = gfCreateAudioDecoderFilter("CAudioDecoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoEncoder:
            thiz = gfCreateVideoEncoderFilter("CVideoEncoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioEncoder:
            thiz = gfCreateAudioEncoderFilter("CAudioEncoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_ScheduledMuxer:
            thiz = gfCreateScheduledMuxerFilter("CScheduledMuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoRenderer:
            thiz = gfCreateVideoRendererFilter("CVideoRenderer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioRenderer:
            thiz = gfCreateAudioRendererFilter("CAudioRenderer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_RTSPTransmiter:
            thiz = gfCreateStreamingTransmiterFilter("CStreamingTransmiter", pPersistMediaConfig, pMsgSink, index);
            break;

        default:
            LOG_FATAL("not supported filter type %x\n", type);
            break;
    }

    return thiz;
}


