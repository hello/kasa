/*******************************************************************************
 * customized_audio_decoder.cpp
 *
 * History:
 *    2014/02/20 - [Zhi He]  create file
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

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

#include "customized_audio_decoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IAudioDecoder *gfCreateCustomizedAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CCustomizedAudioDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IAudioDecoder *CCustomizedAudioDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CCustomizedAudioDecoder *result = new CCustomizedAudioDecoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CCustomizedAudioDecoder::Destroy()
{
    Delete();
}

void CCustomizedAudioDecoder::Delete()
{
    LOGM_RESOURCE("CCustomizedAudioDecoder::Delete().\n");
    if (mpCodec) {
        mpCodec->Destroy();
        mpCodec = NULL;
    }

    inherited::Delete();
}

CCustomizedAudioDecoder::CCustomizedAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpCodec(NULL)
    , mpPersistMediaConfig(pPersistMediaConfig)
{
}

EECode CCustomizedAudioDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleCustomizedAudioDecoder);

    mpCodec = gfCustomizedCodecFactory((TChar *)"customized_adpcm", mIndex);
    if (DUnlikely(!mpCodec)) {
        LOG_ERROR("gfCreateCustomizedADPCMCodec return fail\n");
        return EECode_Error;
    }

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);

    return EECode_OK;
}

CCustomizedAudioDecoder::~CCustomizedAudioDecoder()
{
    if (mpCodec) {
        mpCodec->Destroy();
        mpCodec = NULL;
    }
}

EECode CCustomizedAudioDecoder::SetupContext(SAudioParams *param)
{
    switch (param->codec_format) {

        case StreamFormat_CustomizedADPCM_1:
            break;

        default:
            LOGM_ERROR("codec_format=%u not supported.\n", param->codec_format);
            return EECode_Error;
    }

    return EECode_OK;
}

void CCustomizedAudioDecoder::DestroyContext()
{
}

EECode CCustomizedAudioDecoder::Start()
{
    return EECode_OK;
}

EECode CCustomizedAudioDecoder::Stop()
{
    return EECode_OK;
}

EECode CCustomizedAudioDecoder::Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes)
{
    EECode err = EECode_OK;

    mDebugHeartBeat ++;

    if (DUnlikely(!in_buffer || !out_buffer)) {
        LOGM_FATAL("NULL in_buffer %p or NULL out_buffer %p\n", in_buffer, out_buffer);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(!mpCodec)) {
        LOGM_FATAL("NULL mpCodec\n");
        return EECode_InternalLogicalBug;
    }

    TMemSize output_size = 0;
    TU8 *out_ptr = out_buffer->GetDataPtrBase();

    DASSERT(516 == in_buffer->GetDataSize());
    err = mpCodec->Decoding((void *) in_buffer->GetDataPtr(), (void *) out_ptr, in_buffer->GetDataSize(), output_size);
    DASSERT(2048 == output_size);
    DASSERT_OK(err);

    out_buffer->SetDataPtr((TU8 *)out_ptr);
    out_buffer->SetDataSize(output_size);

    return err;
}

EECode CCustomizedAudioDecoder::Flush()
{
    return EECode_OK;
}

EECode CCustomizedAudioDecoder::Suspend()
{
    return EECode_OK;
}

EECode CCustomizedAudioDecoder::SetExtraData(TU8 *p, TUint size)
{
    return EECode_OK;
}

void CCustomizedAudioDecoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);

    mDebugHeartBeat = 0;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

