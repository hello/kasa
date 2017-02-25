/*******************************************************************************
 * ffmpeg_audio_encoder.h
 *
 * History:
 *    2014/10/27 - [Zhi He]  create file
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

#ifdef BUILD_MODULE_FFMPEG

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "ffmpeg_audio_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IAudioEncoder *gfCreateFFMpegAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CFFMpegAudioEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

static AVSampleFormat __convert_audio_sample_format(AudioSampleFMT audio_sample_format)
{
    switch (audio_sample_format) {
        case AudioSampleFMT_S16:
            return AV_SAMPLE_FMT_S16;
            break;
        case AudioSampleFMT_U8:
            return AV_SAMPLE_FMT_U8;
            break;
        case AudioSampleFMT_S32:
            return AV_SAMPLE_FMT_S32;
            break;
        case AudioSampleFMT_FLT:
            return AV_SAMPLE_FMT_FLT;
            break;
        case AudioSampleFMT_DBL:
            return AV_SAMPLE_FMT_DBL;
            break;
        default:
            break;
    }

    LOG_FATAL("BAD audio_sample_format %d\n", audio_sample_format);
    return AV_SAMPLE_FMT_S16;
}


IAudioEncoder *CFFMpegAudioEncoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CFFMpegAudioEncoder *result = new CFFMpegAudioEncoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CFFMpegAudioEncoder::CFFMpegAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpCodec(NULL)
    , mpEncoder(NULL)
    , mCodecID(AV_CODEC_ID_AAC)
    , mSamplerate(DDefaultAudioSampleRate)
    , mFrameSize(1024)
    , mBitrate(48000)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mbEncoderSetup(0)
    , mChannelNumber(DDefaultAudioChannelNumber)
    , mSampleFormat(AV_SAMPLE_FMT_S16)
{
}

EECode CFFMpegAudioEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFFMpegAudioDecoder);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);
    memset(&mFrame, 0x0, sizeof(mFrame));

    if (mpPersistMediaConfig->p_global_mutex) {
        mpPersistMediaConfig->p_global_mutex->Lock();

        avcodec_register_all();

        mpPersistMediaConfig->p_global_mutex->Unlock();
    } else {
        avcodec_register_all();
    }

    return EECode_OK;
}

void CFFMpegAudioEncoder::Destroy()
{
    Delete();
}

void CFFMpegAudioEncoder::Delete()
{
    destroyEncoder();

    inherited::Delete();
}

void CFFMpegAudioEncoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

CFFMpegAudioEncoder::~CFFMpegAudioEncoder()
{

}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
EECode CFFMpegAudioEncoder::SetupContext(SAudioParams *param)
{
    TU32 size = 0;
    TU8 *p_extra = NULL;

    mChannelNumber = param->channel_number;
    mSamplerate = (param->sample_rate == 0) ? DDefaultAudioSampleRate : param->sample_rate;
    mSampleFormat = (TU8) __convert_audio_sample_format(param->sample_format);
    mBitrate = param->bitrate;
    mFrameSize = 1024;

    switch (param->codec_format) {

        case StreamFormat_AAC:
            mCodecID = AV_CODEC_ID_AAC;
            p_extra = gfGenerateAACExtraData(mSamplerate, mChannelNumber, size);
            mpAudioExtraData = (TU8 *)av_malloc(size);
            if (DLikely(p_extra && mpAudioExtraData)) {
                memcpy(mpAudioExtraData, p_extra, size);
                mAudioExtraDataSize = size;
            }
            free(p_extra);
            break;

        case StreamFormat_PCMU:
            mCodecID = AV_CODEC_ID_PCM_MULAW;
            break;

        case StreamFormat_PCMA:
            mCodecID = AV_CODEC_ID_PCM_ALAW;
            break;

        default:
            LOGM_ERROR("codec_format=%u not supported.\n", param->codec_format);
            return EECode_NotSupported;
    }

    LOGM_DEBUG("audio parameters: mChannelNumber %d, mSamplerate %d, mSampleFormat %d, mBitrate %d, mCodecID %08x\n", mChannelNumber, mSamplerate, mSampleFormat, mBitrate, mCodecID);

    return setupEncoder(mCodecID);
}

void CFFMpegAudioEncoder::DestroyContext()
{
    destroyEncoder();
}


EECode CFFMpegAudioEncoder::Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size)
{
    if (!mpCodec) {
        LOGM_FATAL("NULL mpCodec\n");
        return EECode_InternalLogicalBug;
    } else if (!p_in || !p_out) {
        LOGM_FATAL("NULL p_in %p or p_out %p\n", p_in, p_out);
        return EECode_InternalLogicalBug;
    }

    mDebugHeartBeat ++;

    TInt out_size;
    TInt get_out_ptr = 0;

    av_init_packet(&mPacket);
    mPacket.data = NULL;
    mPacket.size = 0;

    mFrame.data[0] = p_in;
    mFrame.nb_samples = mFrameSize;
    mFrame.channels = mChannelNumber;
    mFrame.sample_rate = mSamplerate;
    mFrame.channel_layout = AV_CH_LAYOUT_STEREO;

    output_size = 0;
    out_size = avcodec_encode_audio2(mpCodec, &mPacket, &mFrame, &get_out_ptr);
    DASSERT(0 <= out_size);
    if (DLikely(get_out_ptr)) {
        if (DLikely(mPacket.data && mPacket.size)) {
            memcpy(p_out, mPacket.data, mPacket.size);
            output_size = mPacket.size;
        }
    }

    return EECode_OK;
}

EECode CFFMpegAudioEncoder::GetExtraData(TU8 *&p, TU32 &size)
{
    p = mpAudioExtraData;
    size = mAudioExtraDataSize;

    return EECode_OK;
}

EECode CFFMpegAudioEncoder::setupEncoder(enum AVCodecID codec_id)
{
    if (DUnlikely(mbEncoderSetup)) {
        LOGM_WARN("decoder already setup?\n");
        return EECode_BadState;
    }

    mpEncoder = avcodec_find_encoder(codec_id);
    if (DUnlikely(NULL == mpEncoder)) {
        LOGM_ERROR("can not find codec_id %d\n", codec_id);
        return EECode_BadParam;
    }

    mpCodec = avcodec_alloc_context3((const AVCodec *) mpEncoder);
    if (DUnlikely(NULL == mpCodec)) {
        LOGM_ERROR("avcodec_alloc_context3 fail\n");
        return EECode_NoMemory;
    }

    mpCodec->extradata = mpAudioExtraData;
    mpCodec->extradata_size = mAudioExtraDataSize;

    mpCodec->codec_type = AVMEDIA_TYPE_AUDIO;
    mpCodec->channels = mChannelNumber;
    mpCodec->sample_rate = mSamplerate;
    mpCodec->sample_fmt = (enum AVSampleFormat) mSampleFormat;
    mpCodec->bit_rate = mBitrate;
    mpCodec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    LOGM_NOTICE("avcodec_find_decoder(%d), %s\n", codec_id, mpEncoder->name);

    TInt rval = avcodec_open2(mpCodec, mpEncoder, NULL);
    if (DUnlikely(0 > rval)) {
        LOGM_ERROR("avcodec_open failed ret %d\n", rval);
        return EECode_Error;
    }

    LOGM_NOTICE("setupDecoder(%d), %s success.\n", codec_id, mpEncoder->name);

    mbEncoderSetup = 1;

    return EECode_OK;
}

void CFFMpegAudioEncoder::destroyEncoder()
{
    if (mbEncoderSetup) {
        mbEncoderSetup = 0;
        avcodec_close(mpCodec);
        av_free(mpCodec);
        mpCodec = NULL;
        mpEncoder = NULL;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

