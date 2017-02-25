/*******************************************************************************
 * audio_input_alsa.cpp
 *
 * History:
 *    2014/01/05 - [Zhi He] create file
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
#include "audio_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "audio_input_alsa.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IAudioInput *gfCreateAudioInputAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAudioInputAlsa::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}
//-----------------------------------------------------------------------
//
// CAudioInputAlsa
//
//-----------------------------------------------------------------------
IAudioInput *CAudioInputAlsa::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    CAudioInputAlsa *result = new CAudioInputAlsa(pName, pPersistMediaConfig, pEngineMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CAudioInputAlsa::Destroy()
{
    Delete();
}

CAudioInputAlsa::CAudioInputAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
    : inherited(pName)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pEngineMsgSink)
    , mpAudio(NULL)
    , mLatency(0)
    , mBufferSize(0)
{
}

EECode CAudioInputAlsa::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioInput);
    mpAudio = gfAudioHALFactory(EAudioPlatform_Alsa, mpPersistMediaConfig);
    if (mpAudio == NULL) {
        LOGM_FATAL("gfAudioHALFactory return NULL\n");
        return EECode_Error;
    }

    return EECode_OK;
}

CAudioInputAlsa::~CAudioInputAlsa()
{
    if (mpAudio) {
        mpAudio->CloseDevice();
        mpAudio->Delete();
        mpAudio = NULL;
    }
}

EECode CAudioInputAlsa::SetupContext(SAudioParams *param)
{
    return mpAudio->OpenDevice(param, &mLatency, &mBufferSize, 0);
}

EECode CAudioInputAlsa::DestroyContext()
{
    return mpAudio->CloseDevice();
}

TUint CAudioInputAlsa::GetChunkSize()
{
    return mpAudio->GetChunkSize();
}

TUint CAudioInputAlsa::GetBitsPerFrame()
{
    return mpAudio->GetBitsPerFrame();
}

EECode CAudioInputAlsa::Start(TUint index)
{
    return mpAudio->Start();
}

EECode CAudioInputAlsa::Stop(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputAlsa::Flush(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputAlsa::Read(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp, TUint index)
{
    return mpAudio->ReadStream(pData, dataSize, pNumFrames, pTimeStamp);
}

EECode CAudioInputAlsa::Pause(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputAlsa::Resume(TUint index)
{
    return EECode_OK;
}

EECode CAudioInputAlsa::Mute()
{
    return EECode_OK;
}

EECode CAudioInputAlsa::UnMute()
{
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END