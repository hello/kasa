/*******************************************************************************
 * filters_interface.cpp
 *
 * History:
 *  2012/12/12 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 *
 ******************************************************************************/

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern IFilter *gfCreateMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateScheduledMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateScheduledVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioCapturerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateStreamingTransmiterFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateCloudConnecterServerAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateCloudConnecterCmdAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateCloudConnecterClientAgentFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

extern IFilter *gfCreateDemuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateScheduledMuxerFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoDecoderFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoEncoderFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoPostProcessorFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoOutputFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioOutputFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateAudioCapturerFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateExternalVideoEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateExternalAudioEsSource(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateStreamingTransmiterFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateConnecterPinmuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateCloudConnecterServerAgentFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateFlowControllerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoCapturerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
extern IFilter *gfCreateVideoCapturerActiveFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

//extern IFilter* gfCreateDirectVideoOutputFilter(const TChar *pName, const volatile SPersistMediaConfig* pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

IFilter *gfFilterFactory(EFilterType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index, TU32 version)
{
    IFilter *thiz = NULL;

    switch (type) {

        case EFilterType_Demuxer:
            thiz = gfCreateDemuxerFilter("CDemuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoDecoder:
            thiz = gfCreateVideoDecoderFilterV2("CVideoDecoderV2", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioDecoder:
            thiz = gfCreateAudioDecoderFilter("CAudioDecoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoEncoder:
            thiz = gfCreateVideoEncoderFilterV2("CVideoEncoderV2", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioEncoder:
            thiz = gfCreateAudioEncoderFilter("CAudioEncoder", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_ScheduledMuxer:
            thiz = gfCreateScheduledMuxerFilterV2("CScheduledMuxerV2", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoPreProcessor:
            LOG_FATAL("EFilterType_VideoPreProcessor TO DO\n");
            break;

        case EFilterType_AudioPreProcessor:
            LOG_FATAL("EFilterType_AudioPreProcessor TO DO\n");
            break;

        case EFilterType_VideoPostProcessor:
            thiz = gfCreateVideoRendererFilter("CVideoPostProcessor", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioPostProcessor:
            LOG_FATAL("EFilterType_AudioPostProcessor TO DO\n");
            break;

        case EFilterType_VideoCapture:
            if (0) {
                thiz = gfCreateVideoCapturerFilter("CVideoCapturer", pPersistMediaConfig, pMsgSink, index);
            } else {
                thiz = gfCreateVideoCapturerActiveFilter("CVideoCapturerActive", pPersistMediaConfig, pMsgSink, index);
            }
            break;

        case EFilterType_AudioCapture:
            thiz = gfCreateAudioCapturerFilterV2("CAudioCapturerV2", pPersistMediaConfig, pMsgSink, index);;
            break;

        case EFilterType_VideoRenderer:
            thiz = gfCreateVideoRendererFilter("CVideoRenderer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioRenderer:
            thiz = gfCreateAudioRendererFilter("CAudioRenderer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_VideoOutput:
            thiz = gfCreateVideoOutputFilter("CVideoOutput", pPersistMediaConfig, pMsgSink, index);
            //thiz = gfCreateDirectVideoOutputFilter("CVideoDirectOutput", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_AudioOutput:
            thiz = gfCreateAudioOutputFilter("CAudioOutput", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_ExtVideoSource:
            thiz = gfCreateExternalVideoEsSource("CExtVideoEsSource", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_ExtAudioSource:
            thiz = gfCreateExternalAudioEsSource("CExtAudioEsSource", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_RTSPTransmiter:
            thiz = gfCreateStreamingTransmiterFilterV2("CStreamingTransmiterV2", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_ConnecterPinmuxer:
            thiz = gfCreateConnecterPinmuxerFilter("CConnecterPinmuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EFilterType_CloudConnecterServerAgent:
            thiz = gfCreateCloudConnecterServerAgentFilterV2("CCloudConnecterServerAgentV2", pPersistMediaConfig, pMsgSink, index);
            break;

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
        case EFilterType_CloudConnecterClientAgent:
            thiz = gfCreateCloudConnecterClientAgentFilter("CCloudConnecterClientAgent", pPersistMediaConfig, pMsgSink, index);
            break;
#endif

        case EFilterType_FlowController:
            thiz = gfCreateFlowControllerFilter("CFlowController", pPersistMediaConfig, pMsgSink, index);
            break;

        default:
            LOG_FATAL("not supported filter type %x\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

