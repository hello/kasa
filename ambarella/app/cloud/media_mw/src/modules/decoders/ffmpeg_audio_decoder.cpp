/*******************************************************************************
 * ffmpeg_audio_decoder.h
 *
 * History:
 *    2013/5/21 - [Roy Su]  create file
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

#include "ffmpeg_audio_decoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IAudioDecoder *gfCreateFFMpegAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CFFMpegAudioDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
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


IAudioDecoder *CFFMpegAudioDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CFFMpegAudioDecoder *result = new CFFMpegAudioDecoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CFFMpegAudioDecoder::Destroy()
{
    Delete();
}

CFFMpegAudioDecoder::CFFMpegAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpCodec(NULL)
    , mpDecoder(NULL)
    , mCodecID(AV_CODEC_ID_AAC)
    , mSamplerate(DDefaultAudioSampleRate)
    , mBitrate(48000)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mbDecoderSetup(0)
    , mChannelNumber(DDefaultAudioChannelNumber)
    , mSampleFormat(AV_SAMPLE_FMT_S16)
{
}

EECode CFFMpegAudioDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFFMpegAudioDecoder);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);

    if (mpPersistMediaConfig->p_global_mutex) {
        mpPersistMediaConfig->p_global_mutex->Lock();

        avcodec_register_all();

        mpPersistMediaConfig->p_global_mutex->Unlock();
    } else {
        avcodec_register_all();
    }

    return EECode_OK;
}

void CFFMpegAudioDecoder::Delete()
{
    destroyDecoder();

    inherited::Delete();
}

CFFMpegAudioDecoder::~CFFMpegAudioDecoder()
{

}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
EECode CFFMpegAudioDecoder::SetupContext(SAudioParams *param)
{

    switch (param->codec_format) {

        case StreamFormat_AAC:
            mCodecID = AV_CODEC_ID_AAC;
            break;

        case StreamFormat_PCMU:
            mCodecID = AV_CODEC_ID_PCM_MULAW;
            break;

        case StreamFormat_PCMA:
            mCodecID = AV_CODEC_ID_PCM_ALAW;
            break;

        case StreamFormat_FFMpegCustomized:
            mCodecID = (AVCodecID) param->customized_codec_type;
            break;

        default:
            LOGM_ERROR("codec_format=%u not supported.\n", param->codec_format);
            return EECode_NotSupported;
    }

    mChannelNumber = param->channel_number;
    mSamplerate = (param->sample_rate == 0) ? DDefaultAudioSampleRate : param->sample_rate;
    mSampleFormat = (TU8) __convert_audio_sample_format(param->sample_format);
    mBitrate = param->bitrate;

    LOGM_DEBUG("audio parameters: mChannelNumber %d, mSamplerate %d, mSampleFormat %d, mBitrate %d, mCodecID %08x\n", mChannelNumber, mSamplerate, mSampleFormat, mBitrate, mCodecID);

    return setupDecoder(mCodecID);
}

EECode CFFMpegAudioDecoder::Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes)
{
    AVPacket pkt;
    TInt out_size;
    TS16 *out_ptr;

    DASSERT(in_buffer);
    DASSERT(out_buffer);
    DASSERT(mpCodec);

    if (!mpCodec) {
        LOGM_FATAL("NULL mpCodec\n");
        return EECode_InternalLogicalBug;
    } else if (!in_buffer || !out_buffer) {
        LOGM_FATAL("NULL in_buffer %p or NULL out_buffer %p\n", in_buffer, out_buffer);
        return EECode_InternalLogicalBug;
    }

    mDebugHeartBeat ++;

    out_size = out_buffer->GetDataMemorySize();
    out_ptr = (TS16 *)out_buffer->GetDataPtrBase();

    memset(&pkt, 0x0, sizeof(pkt));
    pkt.data = in_buffer->GetDataPtr();
    pkt.size = in_buffer->GetDataSize();

    consumed_bytes = avcodec_decode_audio3(mpCodec, out_ptr, &out_size, &pkt);

    if (consumed_bytes < 0) {
        LOGM_ERROR("avcodec_decode_audio3() fail, ret %d\n", consumed_bytes);
        return EECode_Error;
    }

    out_buffer->SetDataPtr((TU8 *)out_ptr);
    out_buffer->SetDataSize((TUint)out_size);
    out_buffer->mAudioChannelNumber = mpCodec->channels;

    return EECode_OK;
}

void CFFMpegAudioDecoder::DestroyContext()
{
    destroyDecoder();
}

EECode CFFMpegAudioDecoder::Start()
{
    return EECode_OK;
}

EECode CFFMpegAudioDecoder::Stop()
{
    return EECode_OK;
}

EECode CFFMpegAudioDecoder::Flush()
{
    return EECode_OK;
}

EECode CFFMpegAudioDecoder::Suspend()
{
    return EECode_OK;
}

EECode CFFMpegAudioDecoder::SetExtraData(TU8 *p, TUint size)
{
    DASSERT(!mpAudioExtraData);
    DASSERT(!mAudioExtraDataSize);

    if (DUnlikely(!p || !size)) {
        return EECode_BadParam;
    }

    mAudioExtraDataSize = size;
    LOGM_INFO("audio extra data size %d, first 2 bytes: 0x%02x 0x%02x\n", mAudioExtraDataSize, p[0], p[1]);
    mpAudioExtraData = (TU8 *)av_malloc(mAudioExtraDataSize + 4);
    if (mpAudioExtraData) {
        memcpy(mpAudioExtraData, p, size);
    } else {
        LOGM_FATAL("NO memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CFFMpegAudioDecoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

EECode CFFMpegAudioDecoder::setupDecoder(enum AVCodecID codec_id)
{
    if (DUnlikely(mbDecoderSetup)) {
        LOGM_WARN("decoder already setup?\n");
        return EECode_BadState;
    }

    mpDecoder = avcodec_find_decoder(codec_id);
    if (DUnlikely(NULL == mpDecoder)) {
        LOGM_ERROR("can not find codec_id %d\n", codec_id);
        return EECode_BadParam;
    }

    mpCodec = avcodec_alloc_context3((const AVCodec *) mpDecoder);
    if (DUnlikely(NULL == mpCodec)) {
        LOGM_ERROR("avcodec_alloc_context3 fail\n");
        return EECode_NoMemory;
    }

    mpCodec->codec_type = AVMEDIA_TYPE_AUDIO;
    mpCodec->channels = mChannelNumber;
    mpCodec->sample_rate = mSamplerate;
    mpCodec->sample_fmt = (enum AVSampleFormat) mSampleFormat;
    mpCodec->bit_rate = mBitrate;

    if (!mpAudioExtraData || !mAudioExtraDataSize) {
        if (mpAudioExtraData) {
            free(mpAudioExtraData);
        }

        //hard code here
        DASSERT(AV_CODEC_ID_AAC == mCodecID);

        TU32 size = 0;
        TU8 *ptmp = gfGenerateAACExtraData(mSamplerate, (TU32) mChannelNumber, mAudioExtraDataSize);

        if (mAudioExtraDataSize < 8) {
            size = 8;
        } else {
            size = mAudioExtraDataSize;
        }
        mpAudioExtraData = (TU8 *)av_malloc(size);
        if (mpAudioExtraData) {
            memcpy(mpAudioExtraData, ptmp, mAudioExtraDataSize);
        } else {
            LOGM_FATAL("NO memory\n");
            free(ptmp);
            return EECode_NoMemory;
        }
    }

    mpCodec->extradata = mpAudioExtraData;
    mpCodec->extradata_size = mAudioExtraDataSize;

    LOGM_NOTICE("avcodec_find_decoder(%d), %s\n", codec_id, mpDecoder->name);

    TInt rval = avcodec_open2(mpCodec, mpDecoder, NULL);
    if (DUnlikely(0 > rval)) {
        LOGM_ERROR("avcodec_open failed ret %d\n", rval);
        return EECode_Error;
    }

    LOGM_NOTICE("setupDecoder(%d), %s success.\n", codec_id, mpDecoder->name);

    mbDecoderSetup = 1;

    return EECode_OK;
}

void CFFMpegAudioDecoder::destroyDecoder()
{
    if (mbDecoderSetup) {
        mbDecoderSetup = 0;
        avcodec_close(mpCodec);
        av_free(mpCodec);
        mpCodec = NULL;
        mpDecoder = NULL;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

