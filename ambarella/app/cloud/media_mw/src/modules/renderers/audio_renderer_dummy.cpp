/*******************************************************************************
 * audio_renderer_dummy.cpp
 *
 * History:
 *    2014/01/09 - [Zhi He] create file
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

#ifdef DCONFIG_COMPILE_OBSOLETE

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

#include "audio_renderer_dummy.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IAudioRenderer *gfCreateAudioRendererDummy(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAudioRendererDummy::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAudioRendererModule
//
//-----------------------------------------------------------------------
CAudioRendererDummy::CAudioRendererDummy(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpAudio(NULL)
{
}

IAudioRenderer *CAudioRendererDummy::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAudioRendererDummy *result = new CAudioRendererDummy(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("CAudioRendererDummy->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

void CAudioRendererDummy::Destroy()
{
    Delete();
}

EECode CAudioRendererDummy::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRenderer);

    return EECode_OK;

    mpAudio = gfAudioHALFactory(EAudioPlatform_Alsa, mpPersistMediaConfig);
    if (mpAudio == NULL) {
        LOGM_FATAL("gfAudioHALFactory return NULL\n");
        return EECode_Error;
    }

    return EECode_OK;
}

CAudioRendererDummy::~CAudioRendererDummy()
{
    //LOGM_RESOURCE("~CAudioRendererModule.\n");
    if (mpAudio != NULL) {
        mpAudio->CloseDevice();
        delete mpAudio;
    }
    //LOGM_RESOURCE("~CAudioRendererModule done.\n");
}

EECode CAudioRendererDummy::SetupContext(SAudioParams *param)
{
    DASSERT(mpPersistMediaConfig);

    if (!mpAudio) {
        return EECode_OK;
    }

    LOGM_INFO("[audio flow]: before mpAudio->OpenDevice\n");
    if (mpAudio->OpenDevice(param, &mLatency, &mBufferSize, 1) != EECode_OK) {
        LOGM_ERROR("mpAudio->OpenDevice fail\n");
        return EECode_Error;
    }

    //start imme
    mpAudio->Start();
    return EECode_OK;
}

EECode CAudioRendererDummy::DestroyContext()
{
    return EECode_OK;
}

EECode CAudioRendererDummy::Start(TUint index)
{
    if (!mpAudio) {
        return EECode_OK;
    }

    if (mpAudio) {
        mpAudio->Start();
    } else {
        LOGM_FATAL("NULL mpAudio\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

EECode CAudioRendererDummy::Stop(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererDummy::Flush(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererDummy::Render(CIBuffer *p_buffer, TUint index)
{
    if (!mpAudio) {
        return EECode_OK;
    }

    EECode err;
    TU8 *data = (TU8 *)(p_buffer->GetDataPtr());
    TUint size = p_buffer->GetDataSize();
    TUint num = 0;

    err = mpAudio->RenderAudio(data, size, &num);

    return err;
}

EECode CAudioRendererDummy::Pause(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererDummy::Resume(TUint index)
{
    return EECode_OK;
}

EECode CAudioRendererDummy::SyncTo(TTime pts, TUint index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

EECode CAudioRendererDummy::SyncMultipleStream(TUint master_index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

