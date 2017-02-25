/*******************************************************************************
 * aac_audio_encoder.cpp
 *
 * History:
 *    2014/01/08 - [Zhi He] create file
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

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

extern "C" {
#include "aac_audio_enc.h"
}

#include "aac_audio_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define AAC_LIB_TEMP_ENC_BUF_SIZE  (106000)

IAudioEncoder *gfCreateAacAudioEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAacAudioEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}
//-----------------------------------------------------------------------
//
// CAacAudioEncoder
//
//-----------------------------------------------------------------------
IAudioEncoder *CAacAudioEncoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAacAudioEncoder *result = new CAacAudioEncoder(pName, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        result->Delete();
        result = NULL;
    }
    return result;
}

void CAacAudioEncoder::Destroy()
{
    Delete();
}

CAacAudioEncoder::CAacAudioEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pName)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpTmpEncBuf(NULL)
    , mSampleRate(DDefaultAudioSampleRate)
    , mNumOfChannels(DDefaultAudioChannelNumber)
    , mBitRate(128000)
    , mbEncoderStarted(0)
{
}

EECode CAacAudioEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAACAudioEncoder);

    mpTmpEncBuf = new TU8[AAC_LIB_TEMP_ENC_BUF_SIZE];

    return EECode_OK;
}

CAacAudioEncoder::~CAacAudioEncoder()
{
    if (mbEncoderStarted) {
        closeAACEncoder();
        mbEncoderStarted = 0;
    }

    if (mpTmpEncBuf) {
        delete[] mpTmpEncBuf;
        mpTmpEncBuf = NULL;
    }
}

EECode CAacAudioEncoder::SetupContext(SAudioParams *param)
{
    DASSERT(StreamFormat_AAC == param->codec_format);
    if (StreamFormat_AAC == param->codec_format) {
        //setupAACEncoder
        mBitRate = (param->bitrate == 0) ? 128000 : param->bitrate;
        mNumOfChannels = (param->channel_number == 0) ? 2 : param->channel_number;
        mSampleRate = (param->sample_rate == 0) ? DDefaultAudioSampleRate : param->sample_rate;
        openAACEncoder();
    } else {
        LOGM_FATAL("Bad codec_format! (%u)\n", param->codec_format);
        return EECode_BadParam;
    }
    mbEncoderStarted = 1;
    return EECode_OK;
}

void CAacAudioEncoder::DestroyContext()
{
    closeAACEncoder();
}

EECode CAacAudioEncoder::Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size)
{
    DASSERT(p_in);
    DASSERT(p_out);
    DASSERT(mbEncoderStarted == 1);
    LOGM_FATAL("todo check: CAacAudioEncoder::Encode\n");
    output_size = doEncode(p_in, p_out);

    return EECode_OK;
}

EECode CAacAudioEncoder::GetExtraData(TU8 *&p, TU32 &size)
{
    LOGM_FATAL("todo check: CAacAudioEncoder::GetExtraData\n");
    return EECode_NoImplement;
}

//////////////////////////////////////////////////////////////////////
EECode CAacAudioEncoder::openAACEncoder()
{
    LOGM_INFO("open AAC Audio Encoder: mBitRate %u, mNumOfChannels %u, mSampleRate %u\n", mBitRate, mNumOfChannels, mSampleRate);

    mAACEncConfig.enc_mode = 0;// 0: AAC; 1: AAC_PLUS; 3: AACPLUS_PS;
    mAACEncConfig.sample_freq       = mSampleRate;
    mAACEncConfig.coreSampleRate    = mSampleRate;
    mAACEncConfig.Src_numCh         = mNumOfChannels;
    mAACEncConfig.Out_numCh         = mNumOfChannels;
    mAACEncConfig.tns               = 1;
    mAACEncConfig.ffType            = 't';
    mAACEncConfig.bitRate           = mBitRate; // AAC: 128000; AAC_PLUS: 64000; AACPLUS_PS: 40000;
    mAACEncConfig.quantizerQuality  = 1;     // 0 - low, 1 - high, 2 - highest
    mAACEncConfig.codec_lib_mem_adr = (TU32 *)mpTmpEncBuf;

    aacenc_setup(&mAACEncConfig);
    aacenc_open(&mAACEncConfig);

    return EECode_OK;
}

TUint CAacAudioEncoder::doEncode(TU8 *pIn, TU8 *pOut)
{
    mAACEncConfig.enc_rptr = (TS32 *)pIn;
    mAACEncConfig.enc_wptr = pOut;

    aacenc_encode(&mAACEncConfig);
    TUint outSize = (mAACEncConfig.nBitsInRawDataBlock + 7) >> 3;

    return outSize;
}

EECode CAacAudioEncoder::closeAACEncoder()
{
    if (mbEncoderStarted) {
        aacenc_close();
        mbEncoderStarted = 0;
    }
    return EECode_OK;
}
DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
