/*
 * alsa_audio_renderer.cpp
 *
 * History:
 *    2013/05/17 - [Zhi He] create file
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

#include "alsa_audio_renderer.h"

IAudioRenderer *gfCreateAudioRendererAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAudioRendererAlsa::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAudioRendererModule
//
//-----------------------------------------------------------------------
CAudioRendererAlsa::CAudioRendererAlsa(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpAudioAL(NULL)
{
    memset(&mfAudioAL, 0x0, sizeof(mfAudioAL));
}

IAudioRenderer *CAudioRendererAlsa::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAudioRendererAlsa *result = new CAudioRendererAlsa(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("CAudioRendererModule->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

void CAudioRendererAlsa::Destroy()
{
    Delete();
}

EECode CAudioRendererAlsa::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRenderer);

    gfSetupAlsaAlContext(&mfAudioAL);

    return EECode_OK;
}

CAudioRendererAlsa::~CAudioRendererAlsa()
{
}

EECode CAudioRendererAlsa::SetupContext(SAudioParams *param)
{
    DASSERT(mfAudioAL.f_open_playback);

    LOGM_INFO("mfAudioAL.f_open_playback...\n");
    mpAudioAL = mfAudioAL.f_open_playback((TChar *)mpPersistMediaConfig->audio_device.audio_device_name, param->sample_rate, param->channel_number, param->frame_size);
    if (!mpAudioAL) {
        LOGM_ERROR("mfAudioAL.f_open_playback fail\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CAudioRendererAlsa::DestroyContext()
{
    if (mfAudioAL.f_close && mpAudioAL) {
        mfAudioAL.f_close(mpAudioAL);
    }
    mpAudioAL = NULL;
    return EECode_OK;
}

EECode CAudioRendererAlsa::Start(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererAlsa::Stop(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererAlsa::Flush(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererAlsa::Render(CIBuffer *p_buffer, TUint index)
{
    TU8 *data = (TU8 *)(p_buffer->GetDataPtr());
    TUint size = p_buffer->GetDataSize();
    TUint num = 0;
    TInt ret = 0;

    DASSERT(mfAudioAL.f_render_data);

    ret = mfAudioAL.f_render_data(mpAudioAL, data, size / 2, &num);
    if (!ret) {
        return EECode_OK;
    }
    LOGM_ERROR("rener fail, ret %d\n", ret);

    return EECode_Error;
}

EECode CAudioRendererAlsa::Pause(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererAlsa::Resume(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererAlsa::SyncTo(TTime pts, TUint index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

EECode CAudioRendererAlsa::SyncMultipleStream(TUint master_index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

#endif

