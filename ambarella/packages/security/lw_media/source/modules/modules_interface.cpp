/**
 * modules_interface.cpp
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
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
#include "modules_interface.h"

#ifdef BUILD_MODULE_AMBA_DSP
extern IVideoDecoder *gfCreateAmbaVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoRenderer *gfCreateAmbaVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoEncoder *gfCreateAmbaVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_FFMPEG
extern IVideoDecoder *gfCreateFFMpegVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);
#endif

#ifdef BUILD_MODULE_LINUX_FB
extern IVideoRenderer *gfCreateLinuxFBVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_DEMUXER_RTSP
extern IDemuxer *gfCreatePrivateRTSPDemuxerScheduledModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_ALSA
extern IAudioRenderer *gfCreateAudioRendererAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IAudioInput *gfCreateAudioInputAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_LIBAAC
extern IAudioDecoder *gfCreateAACAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IAudioEncoder *gfCreateAacAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

extern IStreamingTransmiter *gfCreateVideoRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index);
extern IStreamingTransmiter *gfCreateAudioRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
extern IStreamingServer *gfCreateRtspStreamingServer(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);

#ifdef BUILD_MODULE_MUXER_TS
extern IMuxer *gfCreateTSMuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_MUXER_MP4
extern IMuxer *gfCreateMP4MuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_DEMUXER_TS
extern IDemuxer *gfCreateTSDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_DEMUXER_MP4
extern IDemuxer *gfCreateMP4DemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

IDemuxer *gfModuleFactoryDemuxer(EDemuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
{
    IDemuxer *thiz = NULL;
    switch (id) {

        case EDemuxerModuleID_Auto:
        case EDemuxerModuleID_PrivateRTSPScheduled:
#ifdef BUILD_MODULE_DEMUXER_RTSP
            thiz = gfCreatePrivateRTSPDemuxerScheduledModule("CScheduledRTSPDemuxer", pPersistMediaConfig, pMsgSink, index);
#else
            LOG_FATAL("not compile with private rtsp demuxer\n");
#endif
            break;

        case EDemuxerModuleID_PrivateTS:
#ifdef BUILD_MODULE_DEMUXER_TS
            thiz = gfCreateTSDemuxerModule("CTSDemuxer", pPersistMediaConfig, pMsgSink, index);
#else
            LOG_FATAL("not compile with private TS demuxer\n");
#endif
            break;

        case EDemuxerModuleID_PrivateMP4:
#ifdef BUILD_MODULE_DEMUXER_MP4
            thiz = gfCreateMP4DemuxerModule("CMP4Demuxer", pPersistMediaConfig, pMsgSink, index);
#else
            LOG_FATAL("not compile with private MP4 demuxer\n");
#endif
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
        case EVideoDecoderModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
            thiz = gfCreateAmbaVideoDecoderModule("CAmbaVideoDecoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with amba DSP\n");
#endif
            break;

        case EVideoDecoderModuleID_FFMpeg:
#ifdef BUILD_MODULE_FFMPEG
            thiz = gfCreateFFMpegVideoDecoderModule("CFFMpegVideoDecoder", pPersistMediaConfig, pMsgSink, 0);
#else
            LOG_FATAL("not compile with ffmpeg\n");
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

        case EVideoRendererModuleID_LinuxFrameBuffer:
#ifdef BUILD_MODULE_LINUX_FB
            thiz = gfCreateLinuxFBVideoRendererModule("CLinuxFBVideoRenderer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with linux frame buffer\n");
#endif
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

        default:
            LOG_FATAL("BAD EAudioRendererModuleID %d\n", id);
            thiz = NULL;
            break;
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
        case EAudioEncoderModuleID_PrivateLibAAC:
#ifdef BUILD_MODULE_LIBAAC
            thiz = gfCreateAacAudioEncoderModule("CAacAudioEncoder", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with libaac\n");
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

        default:
            LOG_FATAL("BAD EVideoEncoderModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

IMuxer *gfModuleFactoryMuxer(EMuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    IMuxer *thiz = NULL;

    switch (id) {

        case EMuxerModuleID_PrivateTS:
#ifdef BUILD_MODULE_MUXER_TS
            thiz = gfCreateTSMuxerModule("CTSMuxer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with private TS muxer\n");
#endif
            break;

        case EMuxerModuleID_PrivateMP4:
#ifdef BUILD_MODULE_MUXER_MP4
            thiz = gfCreateMP4MuxerModule("CMP4Muxer", pPersistMediaConfig, pMsgSink);
#else
            LOG_FATAL("not compile with private MP4 muxer\n");
#endif
            break;

        default:
            LOG_FATAL("BAD EMuxerModuleID %d\n", id);
            thiz = NULL;
            break;
    }

    return thiz;
}

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

        default:
            LOG_FATAL("BAD EStreamingTransmiterModuleID %d\n", id);
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
            thiz = gfCreateRtspStreamingServer("CRTSPStreamingServer", type, mode, pPersistMediaConfig, pMsgSink, server_port, enable_video, enable_audio);
            break;

        default:
            LOG_FATAL("BAD StreamingServerType %d\n", type);
            thiz = NULL;
            break;
    }

    return thiz;
}


