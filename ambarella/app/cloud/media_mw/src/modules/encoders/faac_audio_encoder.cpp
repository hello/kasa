/*******************************************************************************
 * faac_audio_encoder.cpp
 *
 * History:
 *    2014/11/05 - [Zhi He]  create file
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

#ifdef BUILD_MODULE_FAAC

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
#include "faac.h"
#include "faaccfg.h"
}

#include "faac_audio_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IAudioEncoder *gfCreateFAACAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CFAACAudioEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IAudioEncoder *CFAACAudioEncoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CFAACAudioEncoder *result = new CFAACAudioEncoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CFAACAudioEncoder::Destroy()
{
    Delete();
}

CFAACAudioEncoder::CFAACAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEncoder(NULL)
    , mSamplerate(DDefaultAudioSampleRate)
    , mFrameSize(1024)
    , mBitrate(32000)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mbEncoderSetup(0)
    , mChannelNumber(DDefaultAudioChannelNumber)
    , mSampleFormat(AudioSampleFMT_S16)
{

}

CFAACAudioEncoder::~CFAACAudioEncoder()
{

}

EECode CFAACAudioEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFFMpegAudioDecoder);
    mSampleNumber = mFrameSize * mChannelNumber;
    return EECode_OK;
}

void CFAACAudioEncoder::Delete()
{
    destroyEncoder();

    inherited::Delete();
}

void CFAACAudioEncoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

EECode CFAACAudioEncoder::SetupContext(SAudioParams *param)
{
#if 0
    mChannelNumber = mpPersistMediaConfig->audio_prefer_setting.channel_number;
    mSamplerate = mpPersistMediaConfig->audio_prefer_setting.sample_rate;
    mBitrate = mpPersistMediaConfig->audio_prefer_setting.bitrate;
    mFrameSize = mpPersistMediaConfig->audio_prefer_setting.framesize;
#endif

    mChannelNumber = param->channel_number;
    mSamplerate = param->sample_rate;
    mBitrate = param->bitrate;
    mFrameSize = param->frame_size;
    mSampleFormat = param->sample_format;

    mSampleNumber = mFrameSize * mChannelNumber;
    //LOGM_DEBUG("audio parameters: mChannelNumber %d, mSamplerate %d, mSampleFormat %d, mBitrate %d\n", mChannelNumber, mSamplerate, mSampleFormat, mBitrate);

    return setupEncoder();
}

void CFAACAudioEncoder::DestroyContext()
{
    destroyEncoder();
}

EECode CFAACAudioEncoder::Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size)
{
    if (DUnlikely(!mpEncoder)) {
        LOGM_FATAL("NULL mpEncoder\n");
        return EECode_InternalLogicalBug;
    } else if (!p_in || !p_out) {
        LOGM_FATAL("NULL p_in %p or p_out %p\n", p_in, p_out);
        return EECode_InternalLogicalBug;
    }

    TInt bytes_written = faacEncEncode(mpEncoder, (int32_t *)p_in, mSampleNumber, p_out, output_size);
    if (DUnlikely(bytes_written < 0)) {
        LOGM_FATAL("faacEncEncode() error, ret %d\n", bytes_written);
        return EECode_Error;
    }
    output_size = bytes_written;

    mDebugHeartBeat ++;

    return EECode_OK;
}

EECode CFAACAudioEncoder::GetExtraData(TU8 *&p, TU32 &size)
{
    p = mpAudioExtraData;
    size = mAudioExtraDataSize;

    return EECode_OK;
}

EECode CFAACAudioEncoder::setupEncoder()
{
    if (DUnlikely(mbEncoderSetup)) {
        LOGM_WARN("decoder already setup?\n");
        return EECode_BadState;
    }

    TULong samples_input, max_bytes_output;
    TInt ret;

    mpEncoder = faacEncOpen(mSamplerate, mChannelNumber, &samples_input, &max_bytes_output);
    if (DUnlikely(NULL == mpEncoder)) {
        LOGM_ERROR("faacEncOpen fail\n");
        return EECode_NoMemory;
    }

    mpEncConfig = faacEncGetCurrentConfiguration(mpEncoder);

    mpEncConfig->aacObjectType = LOW;
    mpEncConfig->mpegVersion = MPEG4;
    mpEncConfig->useTns = 0;
    mpEncConfig->allowMidside = 1;
    mpEncConfig->bitRate = mBitrate / mChannelNumber;
    mpEncConfig->bandWidth = 0;
    mpEncConfig->outputFormat = 1;
    if (AudioSampleFMT_FLT == mSampleFormat) {
        mpEncConfig->inputFormat = 4;
    } else if (AudioSampleFMT_S16 == mSampleFormat) {
        mpEncConfig->inputFormat = 1;
    } else {
        LOG_ERROR("NOT supported sample format %d\n", mSampleFormat);
        return EECode_BadParam;
    }

    if (DUnlikely(samples_input != (mFrameSize * mChannelNumber))) {
        LOGM_ERROR("invalid setting, samples_input %d, mFrameSize %d, mChannelNumber %d\n", samples_input, mFrameSize, mChannelNumber);
        return EECode_Error;
    }
    LOGM_NOTICE("samples_input %ld, mFrameSize %d, mChannelNumber %d\n", samples_input, mFrameSize, mChannelNumber);

    faacEncGetDecoderSpecificInfo(mpEncoder, &mpAudioExtraData, &mAudioExtraDataSize);
    if (!mpAudioExtraData || !mAudioExtraDataSize) {
        TU32 size = 0;
        mpAudioExtraData = gfGenerateAACExtraData(mSamplerate, mChannelNumber, size);
        mAudioExtraDataSize = size;
        LOGM_NOTICE("generate for hacking\n");
    } else {
        LOGM_NOTICE("get from faac\n");
    }
    LOGM_NOTICE("aac extra data %02x %02x, size %ld\n", mpAudioExtraData[0], mpAudioExtraData[1], mAudioExtraDataSize);

    //mpEncConfig->outputFormat = 0;

    ret = faacEncSetConfiguration(mpEncoder, mpEncConfig);
    if (DUnlikely(0 > ret)) {
        LOGM_ERROR("faacEncSetConfiguration fail\n");
        return EECode_Error;
    }

    mbEncoderSetup = 1;

    return EECode_OK;
}

void CFAACAudioEncoder::destroyEncoder()
{
    if (mbEncoderSetup) {
        mbEncoderSetup = 0;
        if (mpEncoder) {
            faacEncClose(mpEncoder);
            mpEncoder = NULL;
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif
