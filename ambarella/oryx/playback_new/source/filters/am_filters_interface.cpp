/**
 * am_filters_interface.cpp
 *
 * History:
 *  2012/12/12 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#include "am_filters_interface.h"

extern IFilter *gfCreateDemuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoCapturerActiveFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateScheduledMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
extern IFilter *gfCreateVideoInjecterFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoRawSinkFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoESSinkFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

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
    case EFilterType_ScheduledMuxer:
      thiz = gfCreateScheduledMuxerFilter("CScheduledMuxer", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoRenderer:
      thiz = gfCreateVideoRendererFilter("CVideoRenderer", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_AudioRenderer:
      thiz = gfCreateAudioRendererFilter("CAudioRenderer", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoCapture:
      thiz = gfCreateVideoCapturerActiveFilter("CVideoCapturerActive", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoInjecter:
      thiz = gfCreateVideoInjecterFilter("CVideoInjecter", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoEncoder:
      thiz = gfCreateVideoEncoderFilter("CVideoEncoder", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoRawSink:
      thiz = gfCreateVideoRawSinkFilter("CVideoRawSink", pPersistMediaConfig, pMsgSink, index);
      break;
    case EFilterType_VideoESSink:
      thiz = gfCreateVideoESSinkFilter("CVideoESSink", pPersistMediaConfig, pMsgSink, index);
      break;
    default:
      LOG_FATAL("not supported filter type 0x%x\n", type);
      break;
  }
  return thiz;
}


