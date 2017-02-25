/**
 * am_modules_interface.cpp
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_playback_new_if.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#ifdef BUILD_MODULE_AMBA_DSP
extern IVideoDecoder *gfCreateAmbaVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoRenderer *gfCreateAmbaVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoEncoder *gfCreateAmbaVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoInjecter *gfCreateVideoAmbaEFMInjecter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
extern IVideoInput *gfCreateVideoInputAmbaDSP(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
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
#endif

#ifdef BUILD_MODULE_LIBAAC
extern IAudioDecoder *gfCreateAACAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_DEMUXER_TS
extern IDemuxer *gfCreateTSDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_DEMUXER_MP4
extern IDemuxer *gfCreateMP4DemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_MUXER_MP4
IMuxer *gfCreateMP4MuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
#endif

#ifdef BUILD_MODULE_DEMUXER_VIDEO_ES_FILE
extern IDemuxer *gfCreateVideoESFileDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
#endif

#ifdef BUILD_MODULE_LINUX_UVC
extern IVideoInput *gfCreateVideoInputLinuxUVC(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink);
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
      LOG_FATAL("not compile with private RSTP demuxer\n");
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
    case EDemuxerModuleID_PrivateVideoES:
#ifdef BUILD_MODULE_DEMUXER_VIDEO_ES_FILE
      thiz = gfCreateVideoESFileDemuxerModule("CVideoESFileDemuxer", pPersistMediaConfig, pMsgSink, index);
#else
      LOG_FATAL("not compile with private Video ES File demuxer\n");
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
      LOG_NOTICE("not compile with ffmpeg\n");
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
    case EAudioDecoderModuleID_LIBAAC:
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

IVideoInput *gfModuleFactoryVideoInput(EVideoInputModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  IVideoInput *thiz = NULL;
  switch (id) {
    case EVideoInputModuleID_Auto:
    case EVideoInputModuleID_AMBADSP:
#ifdef BUILD_MODULE_AMBA_DSP
      thiz = gfCreateVideoInputAmbaDSP("CVideoInputAmbaDSP", pPersistMediaConfig, pMsgSink);
#else
      LOG_FATAL("not compile with amba dsp\n");
#endif
      break;
    case EVideoInputModuleID_LinuxUVC:
#ifdef BUILD_MODULE_LINUX_UVC
      thiz = gfCreateVideoInputLinuxUVC("CVideoInputLinuxUVC", pPersistMediaConfig, pMsgSink);
#else
      LOG_FATAL("not compile with linux uvc\n");
#endif
      break;
    default:
      LOG_FATAL("BAD EVideoInputModuleID %d\n", id);
      thiz = NULL;
      break;
  }
  return thiz;
}

IVideoInjecter *gfModuleFactoryVideoInjecter(EVideoInjecterModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  IVideoInjecter *thiz = NULL;
  switch (id) {
    case EVideoInjecterModuleID_Auto:
    case EVideoInjecterModuleID_AMBAEFM:
#ifdef BUILD_MODULE_AMBA_DSP
      thiz = gfCreateVideoAmbaEFMInjecter("CAmbaVideoEFMInjecter", pPersistMediaConfig, pMsgSink);
#else
      LOG_FATAL("not compile with amba dsp\n");
#endif
      break;
    default:
      LOG_FATAL("BAD EVideoInjecterModuleID %d\n", id);
      thiz = NULL;
      break;
  }
  return thiz;
}

IMuxer *gfModuleFactoryMuxer(EMuxerModuleID id, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  IMuxer *thiz = NULL;
  switch (id) {
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

extern IMediaFileParser *gfCreateBasicAVCParser();
extern IMediaFileParser *gfCreateBasicAACParser();
extern IMediaFileParser *gfCreateBasicHEVCParser();
extern IMediaFileParser *gfCreateBasicMJPEGParser();

extern IMediaProbe *gfCreateMP4ProbeModule();

IMediaFileParser *gfCreateMediaFileParser(EMediaFileParser type)
{
  IMediaFileParser *thiz = NULL;
  switch (type) {
    case EMediaFileParser_BasicAVC:
      thiz = gfCreateBasicAVCParser();
      break;
    case EMediaFileParser_BasicAAC:
      thiz = gfCreateBasicAACParser();
      break;
    case EMediaFileParser_BasicHEVC:
      thiz = gfCreateBasicHEVCParser();
      break;
    case EMediaFileParser_BasicMJPEG:
      thiz = gfCreateBasicMJPEGParser();
      break;
    default:
      LOG_FATAL("NOT supported type %d\n", type);
      break;
  }
  return thiz;
}

IMediaProbe *gfCreateMediaProbe(EMediaProbeType type)
{
  IMediaProbe *thiz = NULL;
  switch (type) {
    case EMediaProbeType_MP4:
      thiz = gfCreateMP4ProbeModule();
      break;
    default:
      LOG_FATAL("NOT supported type %d\n", type);
      break;
  }
  return thiz;
}

