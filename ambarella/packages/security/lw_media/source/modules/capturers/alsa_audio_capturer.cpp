/**
 * audio_input_alsa.cpp
 *
 * History:
 *    2014/01/05 - [Zhi He] create file
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

#include "audio_al.h"

#ifdef BUILD_MODULE_ALSA

#include "alsa_audio_capturer.h"

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
    , mpAudioAL(NULL)
    , mLatency(0)
    , mBufferSize(0)
{
    memset(&mfAudioAL, 0x0, sizeof(mfAudioAL));
}

EECode CAudioInputAlsa::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioInput);
    gfSetupAlsaAlContext(&mfAudioAL);

    return EECode_OK;
}

CAudioInputAlsa::~CAudioInputAlsa()
{
}

EECode CAudioInputAlsa::SetupContext(SAudioParams *param)
{
    DASSERT(mfAudioAL.f_open_capture);

    LOGM_INFO("mfAudioAL.f_open_capture...\n");
    mpAudioAL = mfAudioAL.f_open_capture((TChar *) mpPersistMediaConfig->audio_device.audio_device_name, param->sample_rate, param->channel_number, param->frame_size);
    if (!mpAudioAL) {
        LOGM_ERROR("mfAudioAL.f_open_capture fail\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CAudioInputAlsa::DestroyContext()
{
    if (mfAudioAL.f_close && mpAudioAL) {
        mfAudioAL.f_close(mpAudioAL);
    }
    mpAudioAL = NULL;
    return EECode_OK;
}

EECode CAudioInputAlsa::SetupOutput(COutputPin *p_output_pin, CBufferPool *p_bufferpool, IMsgSink *p_msg_sink)
{
    return EECode_OK;
}

TUint CAudioInputAlsa::GetChunkSize()
{
    return 1024;
}

TUint CAudioInputAlsa::GetBitsPerFrame()
{
    return 16;
}

EECode CAudioInputAlsa::Start(TUint index)
{
    if (mfAudioAL.f_start_capture && mpAudioAL) {
        mfAudioAL.f_start_capture(mpAudioAL);
    }
    return EECode_OK;
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
    TInt ret = 0;

    DASSERT(mfAudioAL.f_read_data);

    ret = mfAudioAL.f_read_data(mpAudioAL, pData, dataSize, pNumFrames, pTimeStamp);
    if (!ret) {
        return EECode_OK;
    }
    LOGM_ERROR("read fail, ret %d\n", ret);

    return EECode_Error;
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

#endif

