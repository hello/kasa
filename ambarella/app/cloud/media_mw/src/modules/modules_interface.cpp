/*******************************************************************************
 * modules_interface.cpp
 *
 * History:
 *  2013/01/01 - [Zhi He] create file
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
#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"

#include "modules_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

#ifdef BUILD_MODULE_AMBA_DSP
extern IVideoDecoder *gfCreateAmbaVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoPostProcessor *gfCreateAmbaVideoPostProcessorModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoRenderer *gfCreateAmbaVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoEncoder *gfCreateAmbaVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_OPTCODEC_ENC_AVC
extern IVideoEncoder *gfCreateOPTCAVCVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_X264
extern IVideoEncoder *gfCreateX264VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_X265
extern IVideoEncoder *gfCreateX265VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_FFMPEG
extern IVideoDecoder* gfCreateFFMpegVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig* pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern IMuxer *gfCreateFFMpegMuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif
extern IDemuxer *gfCreateFFMpegDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
extern IAudioDecoder *gfCreateFFMpegAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_OPTCODEC_DEC
extern IVideoDecoder *gfCreateOPTCVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
extern IAudioDecoder *gfCreateOPTCAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef DCONFIG_COMPILE_OBSOLETE
extern IDemuxer *gfCreatePrivateRTSPDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
#endif

extern IDemuxer *gfCreatePrivateRTSPDemuxerScheduledModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);

#ifdef BUILD_MODULE_ALSA
extern IAudioRenderer *gfCreateAudioRendererAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IAudioInput *gfCreateAudioInputAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_WDS
extern IAudioInput *gfCreateAudioInputWinDS(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, void *pDev);
#endif

#ifdef BUILD_MODULE_WMMDC
extern IAudioInput *gfCreateAudioInputWinMMDevice(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, void *pDev);
#endif

#ifdef BUILD_MODULE_LIBAAC
extern IAudioDecoder *gfCreateAACAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IAudioEncoder *gfCreateAacAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
extern IStreamingTransmiter *gfCreateH264RtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingTransmiter *gfCreateAACRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingTransmiter *gfCreatePCMRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingTransmiter *gfCreateMpeg12AudioRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingTransmiter *gfCreateVideoRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index);
extern IStreamingTransmiter *gfCreateAudioRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
#endif

extern IStreamingTransmiter *gfCreateVideoRtpRtcpTransmiterV2(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index);
extern IStreamingTransmiter *gfCreateAudioRtpRtcpTransmiterV2(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);

extern IStreamingServer *gfCreateRtspStreamingServer(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);
extern IStreamingServer *gfCreateRtspStreamingServerV2(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);
extern ICloudServer *gfCreateSACPCloudServer(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port);

extern IMuxer *gfCreateTSMuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IMuxer *gfCreateMP4MuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);

#ifdef DCONFIG_COMPILE_OBSOLETE
extern IDemuxer *gfCreateUploadingReceiver(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
extern IAudioRenderer *gfCreateAudioRendererDummy(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_HEVC_HM_DEC
extern IVideoDecoder *gfCreateHMHEVCVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
#endif

#ifdef BUILD_MODULE_FFMPEG
extern IAudioEncoder *gfCreateFFMpegAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoDecoder *gfCreateFFMpegVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
#endif

#ifdef BUILD_MODULE_OPTCODEC_ENC_AAC
extern IAudioEncoder *gfCreateOPTCAACAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_FAAC
extern IAudioEncoder *gfCreateFAACAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

extern IAudioEncoder *gfCreateCustomizedAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IAudioDecoder *gfCreateCustomizedAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IDemuxer *gfCreateTSDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
extern IDemuxer *gfCreateMP4DemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

#ifdef BUILD_MODULE_WDUP
extern IVideoInput *gfCreateVideoInputWinDeskDup(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

IDemuxer *gfModuleFactoryDemuxer(EDemuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    IDemuxer *thiz = NULL;
    switch (id) {

#ifdef DCONFIG_COMPILE_OBSOLETE
        case EDemuxerModuleID_PrivateRTSP:
            thiz = gfCreatePrivateRTSPDemuxerModule("CRTSPDemuxer", pPersistMediaConfig, pMsgSink, index);
            break;
#endif

        case EDemuxerModuleID_Auto:
        case EDemuxerModuleID_PrivateRTSPScheduled:
            thiz = gfCreatePrivateRTSPDemuxerScheduledModule("CScheduledRTSPDemuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EDemuxerModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
            thiz = gfCreateFFMpegDemuxerModule("CFFMpegDemuxer", pPersistMediaConfig, pMsgSink, index);
#else
            LOG_FATAL("not compile with ffmpeg\n");
#endif
            break;

#ifdef DCONFIG_COMPILE_OBSOLETE
        case EDemuxerModuleID_UploadingReceiver:
            thiz = gfCreateUploadingReceiver("CUploadingReceiver", pPersistMediaConfig, pMsgSink, index);
            break;
#endif

        case EDemuxerModuleID_PrivateTS:
            thiz = gfCreateTSDemuxerModule("CTSDemuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        case EDemuxerModuleID_PrivateMP4:
            thiz = gfCreateMP4DemuxerModule("CMP4Demuxer", pPersistMediaConfig, pMsgSink, index);
            break;

        default:
            LOG_FATAL("BAD EDemuxerModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IVideoDecoder *gfModuleFactoryVideoDecoder(EVideoDecoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IVideoDecoder *thiz = NULL;

    switch (id) {

        case EVideoDecoderModuleID_Auto:
        case EVideoDecoderModuleID_OPTCDEC:
#ifdef BUILD_MODULE_OPTCODEC_DEC
            thiz = gfCreateOPTCVideoDecoderModule("SOPTCVideoDecoder", pPersistMediaConfig, pMsgSink, 0);
#endif
            break;

        case EVideoDecoderModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            thiz = gfCreateAmbaVideoDecoderModule("CAmbaVideoDecoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with amba dsp\n");
#endif
            break;

        case EVideoDecoderModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
            thiz = gfCreateFFMpegVideoDecoderModule("CFFMpegDecoder", pPersistMediaConfig, pMsgSink, 0);
#endif
            break;

        case EVideoDecoderModuleID_HEVC_HM:
#ifdef BUILD_MODULE_HEVC_HM_DEC
            thiz = gfCreateHMHEVCVideoDecoderModule("SHMVideoDecoder", pPersistMediaConfig, pMsgSink, 0);
#endif
            break;

        default:
            LOG_FATAL("BAD EVideoDecoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IAudioDecoder *gfModuleFactoryAudioDecoder(EAudioDecoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IAudioDecoder *thiz = NULL;

    switch (id) {

        case EAudioDecoderModuleID_Auto:
        case EVideoDecoderModuleID_OPTCDEC:
#ifdef BUILD_MODULE_OPTCODEC_DEC
            thiz = gfCreateOPTCAudioDecoderModule("SOPTCAudioDecoder", pPersistMediaConfig, pMsgSink);
#endif
            break;

        case EAudioDecoderModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
            thiz = gfCreateFFMpegAudioDecoderModule("CFFMpegAudioDecoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with ffmpeg\n");
#endif
            break;

        case EAudioDecoderModuleID_CustomizedADPCM:
            thiz = gfCreateCustomizedAudioDecoderModule("CCustomizedAudioDecoder", pPersistMediaConfig, pMsgSink);
            break;

        case EAudioDecoderModuleID_AAC:
#ifdef BUILD_MODULE_LIBAAC
            thiz = gfCreateAACAudioDecoderModule("CAACAudioDecoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with libaac\n");
#endif
            break;

        default:
            LOG_FATAL("BAD EAudioDecoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

extern IVideoRenderer *gfModuleFactoryVideoRenderer(EVideoRendererModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IVideoRenderer *thiz = NULL;

    switch (id) {

        case EVideoRendererModuleID_Auto:
        case EVideoRendererModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            thiz = gfCreateAmbaVideoRendererModule("CAmbaVideoRenderer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with amba dsp\n");
#endif
            break;

        case EVideoRendererModuleID_DirectDraw:
            break;

        case EVideoRendererModuleID_DirectFrameBuffer:
            break;

        case EVideoRendererModuleID_AndroidSurface:
            break;

        default:
            LOG_FATAL("BAD EVideoRendererModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IAudioRenderer *gfModuleFactoryAudioRenderer(EAudioRendererModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IAudioRenderer *thiz = NULL;
    switch (id) {

        case EAudioRendererModuleID_Auto:
        case EAudioRendererModuleID_ALSA:
#ifdef BUILD_MODULE_ALSA
            thiz = gfCreateAudioRendererAlsa("CAudioAlsaRenderer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with alsa\n");
#endif
            break;

        case EAudioRendererModuleID_DirectSound:
            break;

        case EAudioRendererModuleID_AndroidAudioOutput:
            break;

        default:
            LOG_FATAL("BAD EAudioRendererModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    if (!thiz) {
        LOG_WARN("use dummy audio renderer\n");
#ifdef DCONFIG_COMPILE_OBSOLETE
        thiz = gfCreateAudioRendererDummy("CAudioRendererDummy", pPersistMediaConfig, pMsgSink);
#endif
    }

    return thiz;
}

IVideoEncoder *gfModuleFactoryVideoEncoder(EVideoEncoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IVideoEncoder *thiz = NULL;

    switch (id) {

        case EVideoEncoderModuleID_Auto:
        case EVideoEncoderModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            thiz = gfCreateAmbaVideoEncoderModule("CAmbaVideoEncoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with amba dsp\n");
#endif
            break;

        case EVideoEncoderModuleID_OPTCAVC:
#ifdef BUILD_MODULE_OPTCODEC_ENC_AVC
            thiz = gfCreateOPTCAVCVideoEncoderModule("COPTCAVCVideoEncoder", pPersistMediaConfig, pMsgSink, 0);
#else
            LOG_FATAL("not compile with BUILD_MODULE_OPTCODEC_ENC_AVC\n");
#endif
            break;

        case EVideoEncoderModuleID_FFMpeg:
            break;

        case EVideoEncoderModuleID_X264:
#ifdef BUILD_MODULE_X264
            thiz = gfCreateX264VideoEncoder("CX264VideoEncoder", pPersistMediaConfig, pMsgSink, 0);
#else
            LOG_FATAL("not compile with X264\n");
#endif
            break;

        case EVideoEncoderModuleID_X265:
#ifdef BUILD_MODULE_X265
            thiz = gfCreateX265VideoEncoder("CX265VideoEncoder", pPersistMediaConfig, pMsgSink, 0);
#else
            LOG_FATAL("not compile with X265\n");
#endif
            break;

        default:
            LOG_FATAL("BAD EVideoEncoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IAudioEncoder *gfModuleFactoryAudioEncoder(EAudioEncoderModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IAudioEncoder *thiz = NULL;

    switch (id) {

        case EAudioEncoderModuleID_Auto:
        case EAudioEncoderModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
            thiz = gfCreateFFMpegAudioEncoderModule("CFFMpegAudioEncoder", pPersistMediaConfig, pMsgSink);
#endif
            break;

        case EAudioEncoderModuleID_OPTCAAC:
#ifdef BUILD_MODULE_OPTCODEC_ENC_AAC
            thiz = gfCreateOPTCAACAudioEncoderModule("COPTCAACAudioEncoder", pPersistMediaConfig, pMsgSink);
#endif
            break;

        case EAudioEncoderModuleID_CustomizedADPCM:
            thiz = gfCreateCustomizedAudioEncoderModule("CCustomizedAudioEncoder", pPersistMediaConfig, pMsgSink);
            break;

        case EAudioEncoderModuleID_PrivateLibAAC:
#ifdef BUILD_MODULE_LIBAAC
            thiz = gfCreateAacAudioEncoderModule("CAacAudioEncoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with libaac\n");
#endif
            break;

        case EAudioEncoderModuleID_FAAC:
#ifdef BUILD_MODULE_FAAC
            thiz = gfCreateFAACAudioEncoderModule("CFAACAudioEncoder", pPersistMediaConfig, pMsgSink);
#endif
            break;

        default:
            LOG_FATAL("BAD EAudioEncoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IAudioInput *gfModuleFactoryAudioInput(EAudioInputModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, void *pDev)
{
    IAudioInput *thiz = NULL;
    switch (id) {

        case EAudioInputModuleID_Auto:
        case EAudioInputModuleID_ALSA:
#ifdef BUILD_MODULE_ALSA
            thiz = gfCreateAudioInputAlsa("CAudioInputAlsa", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with alsa\n");
#endif
            break;

        case EAudioInputModuleID_AndroidAudioInput:
            break;

        case EAudioInputModuleID_WinDS:
#ifdef BUILD_MODULE_WDS
            thiz = gfCreateAudioInputWinDS("CAudioInputDS", pPersistMediaConfig, pMsgSink, pDev);
#else
            LOG_FATAL("not compile with WinDS\n");
#endif
            break;

        case EAudioInputModuleID_WinMMDevice:
#ifdef BUILD_MODULE_WMMDC
            thiz = gfCreateAudioInputWinMMDevice("CAudioInputMMDCore", pPersistMediaConfig, pMsgSink, pDev);
#else
            LOG_FATAL("not compile with MMDevice\n");
#endif
            break;

        default:
            LOG_FATAL("BAD EVideoEncoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IVideoInput *gfModuleFactoryVideoInput(EVideoInputModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IVideoInput *thiz = NULL;
    switch (id) {

        case EVideoInputModuleID_Auto:
        case EVideoInputModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            //
#else
            LOG_FATAL("not compile with amba dsp\n");
#endif
            break;

        case EVideoInputModuleID_WinDeskDup:
#ifdef BUILD_MODULE_WDUP
            thiz = gfCreateVideoInputWinDeskDup("CVideoInputWinDeskDup", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with windeskdup\n");
#endif
            break;

        default:
            LOG_FATAL("BAD EVideoInputModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IMuxer *gfModuleFactoryMuxer(EMuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IMuxer *thiz = NULL;

    switch (id) {

        case EMuxerModuleID_Auto:
        case EMuxerModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
            thiz = gfCreateFFMpegMuxerModule("CFFMpegMuxer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with ffmpeg muxer\n");
#endif
#else
            LOG_FATAL("not compile with ffmpeg\n");
#endif
            break;

        case EMuxerModuleID_PrivateTS:
            thiz = gfCreateTSMuxerModule("CTSMuxer", pPersistMediaConfig, pMsgSink);
            break;

        case EMuxerModuleID_PrivateMP4:
            thiz = gfCreateMP4MuxerModule("CMP4Muxer", pPersistMediaConfig, pMsgSink);
            break;

        default:
            LOG_FATAL("BAD EMuxerModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IVideoPostProcessor *gfModuleFactoryVideoPostProcessor(EVideoPostPModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IVideoPostProcessor *thiz = NULL;

    switch (id) {

        case EVideoPostPModuleID_Auto:
        case EVideoPostPModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            thiz = gfCreateAmbaVideoPostProcessorModule("CAmbaVideoPostProcessor", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with amba dsp\n");
#endif
            break;

        case EVideoPostPModuleID_DirectPostP:
            LOG_FATAL("need implement software video postp/mixer\n");
            break;

        default:
            LOG_FATAL("BAD EVideoPostPModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
IStreamingTransmiter *gfModuleFactoryStreamingTransmiter(EStreamingTransmiterModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index)
{
    IStreamingTransmiter *thiz = NULL;

    switch (id) {

        case EStreamingTransmiterModuleID_RTPRTCP_Video:
            thiz = gfCreateVideoRtpRtcpTransmiter("CVideoRTPRTCPSink", pPersistMediaConfig, pMsgSink, index);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_Audio:
            thiz = gfCreateAudioRtpRtcpTransmiter("CAudioRTPRTCPSink", pPersistMediaConfig, pMsgSink);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_MPEG12Audio:
            thiz = gfCreateMpeg12AudioRtpRtcpTransmiter("CMpeg12AudioRTPRTCPSink", pPersistMediaConfig, pMsgSink);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_H264:
            thiz = gfCreateH264RtpRtcpTransmiter("CH264RTPRTCPSink", pPersistMediaConfig, pMsgSink);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_AAC:
            thiz = gfCreateAACRtpRtcpTransmiter("CAACRTPRTCPSink", pPersistMediaConfig, pMsgSink);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_PCM:
            thiz = gfCreatePCMRtpRtcpTransmiter("CPCMRTPRTCPSink", pPersistMediaConfig, pMsgSink);
            break;

        default:
            LOG_FATAL("BAD EStreamingTransmiterModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}
#endif

IStreamingTransmiter *gfModuleFactoryStreamingTransmiterV2(EStreamingTransmiterModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index)
{
    IStreamingTransmiter *thiz = NULL;

    switch (id) {

        case EStreamingTransmiterModuleID_RTPRTCP_Video:
            thiz = gfCreateVideoRtpRtcpTransmiterV2("CVideoRTPRTCPSinkV2", pPersistMediaConfig, pMsgSink, index);
            break;

        case EStreamingTransmiterModuleID_RTPRTCP_Audio:
            thiz = gfCreateAudioRtpRtcpTransmiterV2("CAudioRTPRTCPSinkV2", pPersistMediaConfig, pMsgSink);
            break;

        default:
            LOG_FATAL("BAD EStreamingTransmiterModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

ICloudConnecterServerAgent *gfModuleFactoryCloudConnecterServerAgent(ECloudConnecterServerAgentModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    ICloudConnecterServerAgent *thiz = NULL;

    switch (id) {

        case ECloudConnecterServerAgentModuleID_Auto:
        case ECloudConnecterServerAgentModuleID_SACP:
            LOG_FATAL("gfModuleFactoryCloudConnecterServerAgent TO DO\n");
            break;

        default:
            LOG_FATAL("BAD ECloudConnecterServerAgentModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

ICloudConnecterClientAgent *gfModuleFactoryCloudConnecterClientAgent(ECloudConnecterClientAgentModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    ICloudConnecterClientAgent *thiz = NULL;

    switch (id) {

        case ECloudConnecterClientAgentModuleID_Auto:
        case ECloudConnecterClientAgentModuleID_SACP:
            LOG_FATAL("gfModuleFactoryCloudConnecterClientAgent TO DO\n");
            break;

        default:
            LOG_FATAL("BAD ECloudConnecterClientAgentModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IStreamingServer *gfModuleFactoryStreamingServer(StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio)
{
    IStreamingServer *thiz = NULL;

    switch (type) {

        case StreamingServerType_RTSP:
            if (!pPersistMediaConfig->engine_config.streaming_server_version) {
                thiz = gfCreateRtspStreamingServer("CRTSPStreamingServer", type, mode, pPersistMediaConfig, pMsgSink, server_port, enable_video, enable_audio);
            } else {
                thiz = gfCreateRtspStreamingServerV2("CRTSPStreamingServerV2", type, mode, pPersistMediaConfig, pMsgSink, server_port, enable_video, enable_audio);
            }
            break;

        default:
            LOG_FATAL("BAD StreamingServerType %d\n", type);
            thiz = NULL;
            break;
    }

    return thiz;
}

ICloudServer *gfModuleFactoryCloudServer(CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port)
{
    ICloudServer *thiz = NULL;

    switch (type) {

        case CloudServerType_SACP:
            thiz = gfCreateSACPCloudServer("CSACPCouldServer", type, pPersistMediaConfig, pMsgSink, server_port);
            break;

        default:
            LOG_FATAL("BAD CloudServerType %d\n", type);
            thiz = NULL;
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

