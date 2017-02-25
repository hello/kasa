/*******************************************************************************
 * audio_capturer_v2.cpp
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
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
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "audio_capturer_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static EAudioInputModuleID _guess_audio_input_type_from_string(TChar *string)
{
    if (!string) {
        LOG_FATAL("NULL input in _guess_audio_input_type_from_string\n");
        return EAudioInputModuleID_WinMMDevice;
    }

    if (!strncmp(string, DNameTag_MMD, strlen(DNameTag_MMD))) {
        return EAudioInputModuleID_WinMMDevice;
    } else if (!strncmp(string, DNameTag_DirectSound, strlen(DNameTag_DirectSound))) {
        return EAudioInputModuleID_WinDS;
    } else if (!strncmp(string, DNameTag_ALSA, strlen(DNameTag_ALSA))) {
        return EAudioInputModuleID_ALSA;
    }

    return EAudioInputModuleID_WinMMDevice;
}

IFilter *gfCreateAudioCapturerFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CAudioCapturerV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CAudioCapturerV2
//
//-----------------------------------------------------------------------

IFilter *CAudioCapturerV2::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CAudioCapturerV2 *result = new CAudioCapturerV2(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CAudioCapturerV2::GetObject0() const
{
    return (CObject *) this;
}

CAudioCapturerV2::CAudioCapturerV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpAudioInput(NULL)
    , mCurCapturerID(EAudioInputModuleID_WinDS)
    , mbContentSetup(0)
    , mbOutputSetup(0)
    , mbRunning(0)
    , mbStarted(0)
    , mbSuspended(0)
    , mbPaused(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpDevExternal(NULL)
{

}

EECode CAudioCapturerV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioCapturerFilter);

    mpMemPool = CRingMemPool::Create(1024 * 1024);
    if (DUnlikely(!mpMemPool)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CAudioCapturerV2::~CAudioCapturerV2()
{

}

void CAudioCapturerV2::Delete()
{
    if (mpAudioInput) {
        mpAudioInput->Destroy();
        mpAudioInput = NULL;
    }

    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    if (mpMemPool) {
        mpMemPool->GetObject0()->Delete();
        mpMemPool = NULL;
    }

    inherited::Delete();
}

void CAudioCapturerV2::finalize()
{
    if (DUnlikely(!mpAudioInput)) {
        LOGM_WARN("NULL mpAudioInput\n");
        return;
    }

    if (mbStarted) {
        mbStarted = 0;
        mpAudioInput->Stop();
    }

    mpAudioInput->DestroyContext();
    mbContentSetup = 0;

    mbOutputSetup = 0;
    mpAudioInput->Destroy();
    mpAudioInput = NULL;

    gfClearBufferMemory((IBufferPool *)mpBufferPool);

}

EECode CAudioCapturerV2::initialize(TChar *p_param)
{
    EAudioInputModuleID id;
    EECode err = EECode_OK;

    DASSERT(p_param);

    id = _guess_audio_input_type_from_string(p_param);

    if (mbContentSetup) {
        finalize();
    }

    if (!mpAudioInput) {
        mpAudioInput = gfModuleFactoryAudioInput(id, mpPersistMediaConfig, mpEngineMsgSink, mpDevExternal);
        if (mpAudioInput) {
            mCurCapturerID = id;
        } else {
            mCurCapturerID = EAudioInputModuleID_Auto;
            LOGM_FATAL("[Internal bug], gfModuleFactoryAudioInput(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    if (!mbOutputSetup) {
        mpAudioInput->SetupOutput(mpOutputPin, mpBufferPool, mpEngineMsgSink);
        mbOutputSetup = 1;
    }

    err = mpAudioInput->SetupContext(NULL);
    if (EECode_OK != err) {
        LOGM_ERROR("SetupContext() fail\n");
        return err;
    }

    mbContentSetup = 1;

    return EECode_OK;
}

EECode CAudioCapturerV2::Initialize(TChar *p_param)
{
    LOGM_NOTICE("Initialize()\n");

    return initialize(p_param);
}

EECode CAudioCapturerV2::Finalize()
{
    finalize();

    return EECode_OK;
}

EECode CAudioCapturerV2::Run()
{
    //DASSERT(mpEngineMsgSink);

    //DASSERT(!mbRunning);
    //DASSERT(!mbStarted);

    mbRunning = 1;
    return EECode_OK;
}

EECode CAudioCapturerV2::Exit()
{
    return EECode_OK;
}

EECode CAudioCapturerV2::Start()
{
    if (DLikely(mpAudioInput)) {
        if (DLikely(!mbStarted)) {
            //LOGM_INFO("before mpAudioInput->Start()\n");
            mpAudioInput->Start();
            //LOGM_INFO("after mpAudioInput->Start()\n");
            mbStarted = 1;
        } else {
            LOGM_WARN("already started\n");
        }
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAudioCapturerV2::Stop()
{
    if (DLikely(mpAudioInput)) {
        if (DLikely(mbStarted)) {
            //LOGM_INFO("before mpAudioInput->Stop()\n");
            mpAudioInput->Stop();
            //LOGM_INFO("after mpAudioInput->Stop()\n");
            mbStarted = 0;
        }
        mbStarted = 0;
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CAudioCapturerV2::Pause()
{
    if (DLikely(mpAudioInput)) {
        if (DLikely(!mbPaused)) {
            //LOGM_INFO("before mpAudioInput->Pause()\n");
            mpAudioInput->Pause();
            mbPaused = 1;
            //LOGM_INFO("after mpAudioInput->Pause()\n");
        }
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
    }
}

void CAudioCapturerV2::Resume()
{
    if (DLikely(mpAudioInput)) {
        if (DLikely(mbPaused)) {
            //LOGM_INFO("before mpAudioInput->Resume()\n");
            mpAudioInput->Resume();
            mbPaused = 0;
            //LOGM_INFO("after mpAudioInput->Resume()\n");
        }
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
    }
}

void CAudioCapturerV2::Flush()
{
    if (DLikely(mpAudioInput)) {
        //LOGM_INFO("before mpAudioInput->Flush()\n");
        mpAudioInput->Flush();
        //LOGM_INFO("after mpAudioInput->Flush()\n");
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
    }
}

void CAudioCapturerV2::ResumeFlush()
{
    if (DLikely(mpAudioInput)) {
        //LOGM_INFO("before mpAudioInput->Resume()\n");
        mpAudioInput->Resume();
        //LOGM_INFO("after mpAudioInput->Resume()\n");
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
    }
}

EECode CAudioCapturerV2::Suspend(TUint release_context)
{
    if (DLikely(mpAudioInput)) {
        //LOGM_INFO("before mpAudioInput->Flush()\n");
        mpAudioInput->Flush();
        //LOGM_INFO("after mpAudioInput->Flush()\n");
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioCapturerV2::ResumeSuspend(TComponentIndex input_index)
{
    if (DLikely(mpAudioInput)) {
        //LOGM_INFO("before mpAudioInput->Resume()\n");
        mpAudioInput->Resume();
        //LOGM_INFO("after mpAudioInput->Resume()\n");
    } else {
        LOGM_FATAL("NULL mpAudioInput\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioCapturerV2::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioCapturerV2::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CAudioCapturerV2::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CAudioCapturerV2::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CAudioCapturerV2::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    EECode ret;

    if (StreamType_Audio == type) {
        if (DLikely(!mpOutputPin)) {
            mpOutputPin = COutputPin::Create("[audio capture output pin]", (IFilter *) this);
            if (!mpOutputPin) {
                LOGM_FATAL("[audio capture output pin]::Create() fail\n");
                return EECode_NoMemory;
            }
            DASSERT(!mpBufferPool);
            mpBufferPool = CBufferPool::Create("[audio capture buffer pool for output pin]", DDefaultAudioBufferNumber);
            if (!mpBufferPool) {
                LOGM_FATAL("CSimpleBufferPool::Create() fail\n");
                return EECode_NoMemory;
            }
            mpOutputPin->SetBufferPool(mpBufferPool);
        }

        ret = mpOutputPin->AddSubPin(sub_index);
        if (DLikely(EECode_OK == ret)) {
            index = 0;
        } else {
            LOGM_FATAL("AddSubPin fail\n");
            return EECode_InternalLogicalBug;
        }

    } else {
        LOGM_ERROR("BAD type %d\n", type);
    }

    return EECode_OK;
}

EECode CAudioCapturerV2::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("do not have input pin\n");
    return EECode_InternalLogicalBug;
}

IOutputPin *CAudioCapturerV2::GetOutputPin(TUint index, TUint sub_index)
{
    if (DLikely(!index)) {
        if (DLikely(mpOutputPin)) {
            if (DLikely(sub_index < mpOutputPin->NumberOfPins())) {
                return mpOutputPin;
            } else {
                LOGM_FATAL("BAD sub_index %d\n", sub_index);
            }
        } else {
            LOGM_FATAL("NULL mpOutputPin\n");
        }
    } else {
        LOGM_FATAL("BAD index %d\n", index);
    }

    return NULL;
}

IInputPin *CAudioCapturerV2::GetInputPin(TUint index)
{
    LOGM_FATAL("do not have input pin\n");
    return NULL;
}

EECode CAudioCapturerV2::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_assign_external_module:
            mpDevExternal = p_param;
            break;

        case EFilterPropertyType_audio_capture_mute:
            if (mpAudioInput) {
                mpAudioInput->Mute();
            }
            break;

        case EFilterPropertyType_audio_capture_unmute:
            if (mpAudioInput) {
                mpAudioInput->UnMute();
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CAudioCapturerV2::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CAudioCapturerV2::PrintStatus()
{
    LOGM_PRINTF("PrintStatus()\n");

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }
    if (mpBufferPool) {
        mpBufferPool->PrintStatus();
    }
}

void CAudioCapturerV2::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    switch (type) {

        default:
            LOGM_FATAL("not processed event! %d\n", type);
            break;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

