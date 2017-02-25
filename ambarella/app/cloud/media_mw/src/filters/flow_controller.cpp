/*******************************************************************************
 * flow_control.cpp
 *
 * History:
 *    2014/05/20 - [Zhi He] create file
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

#include "flow_controller.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateFlowControllerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CFlowController::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CFlowController
//
//-----------------------------------------------------------------------

IFilter *CFlowController::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CFlowController *result = new CFlowController(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CFlowController::GetObject0() const
{
    return (CObject *) this;
}

CFlowController::CFlowController(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpPersistSystemClockManager(NULL)
    , mpWorkClockReference(NULL)
    , mpMutex(NULL)
    , mpClockListener(NULL)
    , mbForward(1)
    , mSpeed(0)
    , mSpeedControlMechanism(0)
    , mbStartChecking(0)
    , mnTotalInputPinNumber(0)
    , mnTotalOutputPinNumber(0)
    , mNavigationBeginTime(0)
    , mNavigationCurrentTime(0)
    , mNavigationBeginPTS(0)
    , mNavigationCurrentPTS(0)
    , mTimeThreshold(-1)
{
    TU32 j;
    for (j = 0; j < EConstMaxDemuxerMuxerStreamNumber; j++) {
        mpInputPins[j] = NULL;
        mpOutputPins[j] = NULL;
        mpCachedBuffer[j] = NULL;
    }
}

EECode CFlowController::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFlowController);
    //mConfigLogLevel = ELogLevel_Debug;

    if ((mpCmdQueue = CIQueue::Create(NULL, this, sizeof(CIBuffer *), 16)) == NULL) {
        LOGM_FATAL("CIQueue::Create fail.\n");
        return EECode_NoMemory;
    }

    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    if (DLikely(mpPersistMediaConfig)) {
        if (DLikely(mpPersistMediaConfig->p_system_clock_manager)) {
            mpPersistSystemClockManager = mpPersistMediaConfig->p_system_clock_manager;
            mpWorkClockReference = CIClockReference::Create();
            if (DLikely(mpWorkClockReference)) {
                mpWorkClockReference->SetBeginTime(0);
                mpWorkClockReference->SetDirection(1);
                mpWorkClockReference->SetSpeed(1, 0);
                mpPersistSystemClockManager->RegisterClockReference(mpWorkClockReference);
                return EECode_OK;
            } else {
                LOGM_FATAL("CIClockReference::Create() fail\n");
            }
        } else {
            LOGM_FATAL("NULL mpPersistMediaConfig->p_system_clock_manager\n");
        }
    } else {
        LOGM_FATAL("NULL mpPersistMediaConfig\n");
    }

    return EECode_BadState;
}

CFlowController::~CFlowController()
{
    TUint i = 0;

    LOGM_RESOURCE("~CFlowController.\n");

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }

        if (mpOutputPins[i]) {
            mpOutputPins[i]->Delete();
            mpOutputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("~CFlowController done.\n");
}

void CFlowController::Delete()
{
    TUint i = 0;
    LOGM_RESOURCE("CFlowController::Delete(), before mpDemuxer->Delete().\n");

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }

        if (mpOutputPins[i]) {
            mpOutputPins[i]->Delete();
            mpOutputPins[i] = NULL;
        }
    }
    LOGM_RESOURCE("CFlowController::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CFlowController::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CFlowController::Finalize()
{
    return EECode_OK;
}

EECode CFlowController::Run()
{
    DASSERT(mpEngineMsgSink);

    return EECode_OK;
}

EECode CFlowController::Exit()
{
    return EECode_OK;
}

EECode CFlowController::Start()
{
    DASSERT(mpEngineMsgSink);

    return EECode_OK;
}

EECode CFlowController::Stop()
{
    DASSERT(mpEngineMsgSink);

    return EECode_OK;
}

void CFlowController::Pause()
{
    return;
}

void CFlowController::Resume()
{
    return;
}

void CFlowController::Flush()
{
    TInt i = 0;
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i++) {
        if (mpInputPins[i]) {
            if (mpCachedBuffer[i]) {
                mpCachedBuffer[i]->Release();
                mpCachedBuffer[i] = NULL;
            }
            mpInputPins[i]->Purge(0);
        }
    }
    return;
}

void CFlowController::ResumeFlush()
{
    return;
}

EECode CFlowController::Suspend(TUint release_context)
{
    return EECode_OK;
}

EECode CFlowController::ResumeSuspend(TComponentIndex input_index)
{
    return EECode_OK;
}

EECode CFlowController::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFlowController::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CFlowController::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CFlowController::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CFlowController::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    EECode ret;

    if (StreamType_Video == type) {
        if (!mpOutputPins[EDemuxerVideoOutputPinIndex]) {
            mpOutputPins[EDemuxerVideoOutputPinIndex] = COutputPin::Create("[flow controller output pin]", (IFilter *) this);
            if (!mpOutputPins[EDemuxerVideoOutputPinIndex]) {
                LOGM_FATAL("flow controller output pin::Create() fail\n");
                return EECode_NoMemory;
            }
        }

        if (mpOutputPins[EDemuxerVideoOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerVideoOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerVideoOutputPinIndex;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_Audio == type) {
        if (!mpOutputPins[EDemuxerAudioOutputPinIndex]) {
            mpOutputPins[EDemuxerAudioOutputPinIndex] = COutputPin::Create("[Demuxer audio output pin]", (IFilter *) this);
            if (!mpOutputPins[EDemuxerAudioOutputPinIndex]) {
                LOGM_FATAL("Demuxer audio output pin::Create() fail\n");
                return EECode_NoMemory;
            }
        }
        if (mpOutputPins[EDemuxerAudioOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerAudioOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerAudioOutputPinIndex;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_Subtitle == type) {
        if (!mpOutputPins[EDemuxerSubtitleOutputPinIndex]) {
            mpOutputPins[EDemuxerSubtitleOutputPinIndex] = COutputPin::Create("[Demuxer subtitle output pin]", (IFilter *) this);
        }
        if (mpOutputPins[EDemuxerSubtitleOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerSubtitleOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerSubtitleOutputPinIndex;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else if (StreamType_PrivateData == type) {
        if (!mpOutputPins[EDemuxerPrivateDataOutputPinIndex]) {
            mpOutputPins[EDemuxerPrivateDataOutputPinIndex] = COutputPin::Create("[Demuxer private data output pin]", (IFilter *) this);
        }
        if (mpOutputPins[EDemuxerPrivateDataOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerPrivateDataOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerPrivateDataOutputPinIndex;
                return EECode_OK;
            }
        } else {
            LOGM_FATAL("Why the pin is NULL?\n");
        }
    } else {
        LOGM_ERROR("BAD type %d\n", type);
    }

    return EECode_BadParam;
}

EECode CFlowController::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (!mpInputPins[EDemuxerVideoOutputPinIndex]) {
            mpInputPins[EDemuxerVideoOutputPinIndex] = CQueueInputPin::Create("[flow controller input pin]", (IFilter *) this, StreamType_Video, mpCmdQueue);
            if (!mpInputPins[EDemuxerVideoOutputPinIndex]) {
                LOGM_FATAL("flow controller input pin::Create() fail\n");
                return EECode_NoMemory;
            }
            index = EDemuxerVideoOutputPinIndex;
        } else {
            LOGM_FATAL("already have input pin, type %d\n", type);
            return EECode_InternalLogicalBug;
        }
    } else if (StreamType_Audio == type) {
        if (!mpInputPins[EDemuxerAudioOutputPinIndex]) {
            mpInputPins[EDemuxerAudioOutputPinIndex] = CQueueInputPin::Create("[flow controller input pin]", this, StreamType_Audio, mpCmdQueue);
            if (!mpInputPins[EDemuxerAudioOutputPinIndex]) {
                LOGM_FATAL("flow controller input pin::Create() fail\n");
                return EECode_NoMemory;
            }
            index = EDemuxerAudioOutputPinIndex;
        } else {
            LOGM_FATAL("already have input pin, type %d\n", type);
            return EECode_InternalLogicalBug;
        }
    } else if (StreamType_Subtitle == type) {
        if (!mpInputPins[EDemuxerSubtitleOutputPinIndex]) {
            mpInputPins[EDemuxerSubtitleOutputPinIndex] = CQueueInputPin::Create("[flow controller input pin]", this, StreamType_Subtitle, mpCmdQueue);
            if (!mpInputPins[EDemuxerSubtitleOutputPinIndex]) {
                LOGM_FATAL("flow controller input pin::Create() fail\n");
                return EECode_NoMemory;
            }
            index = EDemuxerSubtitleOutputPinIndex;
        } else {
            LOGM_FATAL("already have input pin, type %d\n", type);
            return EECode_InternalLogicalBug;
        }
    } else if (StreamType_PrivateData == type) {
        if (!mpInputPins[EDemuxerPrivateDataOutputPinIndex]) {
            mpInputPins[EDemuxerPrivateDataOutputPinIndex] = CQueueInputPin::Create("[flow controller input pin]", this, StreamType_PrivateData, mpCmdQueue);
            if (!mpInputPins[EDemuxerPrivateDataOutputPinIndex]) {
                LOGM_FATAL("flow controller input pin::Create() fail\n");
                return EECode_NoMemory;
            }
            index = EDemuxerPrivateDataOutputPinIndex;
        } else {
            LOGM_FATAL("already have input pin, type %d\n", type);
            return EECode_InternalLogicalBug;
        }
    } else {
        LOGM_ERROR("BAD type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IOutputPin *CFlowController::GetOutputPin(TUint index, TUint sub_index)
{
    TUint max_sub_index = 0;

    if (EDemuxerOutputPinNumber > index) {
        if (mpOutputPins[index]) {
            max_sub_index = mpOutputPins[index]->NumberOfPins();
            if (max_sub_index > sub_index) {
                return mpOutputPins[index];
            } else {
                LOGM_ERROR("BAD sub_index %d, max value %d, in CFlowController::GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
            }
        } else {
            LOGM_FATAL("Why the pointer is NULL? in CFlowController::GetOutputPin(%u, %u)\n", index, sub_index);
        }
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CFlowController::GetOutputPin(%u, %u)\n", index, EDemuxerOutputPinNumber, index, sub_index);
    }

    return NULL;
}

IInputPin *CFlowController::GetInputPin(TUint index)
{
    if (EDemuxerOutputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CFlowController::GetInputPin(%u)\n", index, EDemuxerOutputPinNumber, index);
    }

    return NULL;
}

EECode CFlowController::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    AUTO_LOCK(mpMutex);

    switch (property_type) {

        case EFilterPropertyType_flow_controller_start: {
                SFlowControlSetting *setting = (SFlowControlSetting *) p_param;

                mbForward = setting->direction;
                mSpeed = setting->speed;
                mbStartChecking = 1;

                mNavigationBeginTime = setting->navigation_begin_time;
                mpWorkClockReference->RemoveAllTimers(this);
                TTime source_time = mpPersistMediaConfig->p_clock_source->GetClockTime();
                mpWorkClockReference->Reset(source_time, mNavigationBeginTime, 1, 1, mSpeed, setting->speed_frac, mbForward);

                mpClockListener = mpWorkClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mNavigationBeginTime + 100000, 100000, 0);//TODO,hardcode
                DASSERT(mpClockListener);
            }
            break;

        case EFilterPropertyType_flow_controller_update_speed: {
                SFlowControlSetting *setting = (SFlowControlSetting *) p_param;

                mbForward = setting->direction;
                mSpeed = setting->speed;

                mpWorkClockReference->SetSpeed(setting->speed, setting->speed_frac);
            }
            break;

        case EFilterPropertyType_flow_controller_stop: {
                if (DUnlikely(!mpClockListener)) {
                    LOGM_ERROR("NULL mpClockListener\n");
                } else {
                    mpWorkClockReference->RemoveTimer(mpClockListener);
                    mpClockListener = NULL;
                    mTimeThreshold = -1;
                }
            }
            break;

        case EFilterPropertyType_purge_channel:
            purgeAllChannels(1);
            break;

        case EFilterPropertyType_resume_channel:
            resumeAllChannels();
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CFlowController::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = mnTotalOutputPinNumber;

    info.pName = mpModuleName;
}

void CFlowController::PrintStatus()
{
    TUint i = 0;
    LOGM_PRINTF("mnTotalOutputPinNumber %d\n", mnTotalOutputPinNumber);

    for (i = 0; i < mnTotalOutputPinNumber; i ++) {
        DASSERT(mpOutputPins[i]);
        if (mpOutputPins[i]) {
            mpOutputPins[i]->PrintStatus();
        }
    }

}

void CFlowController::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    AUTO_LOCK(mpMutex);

    switch (type) {
        case EEventType_Timer:
            ConveyBuffers((TTime)param1);
            break;

        default:
            LOGM_FATAL("not processed event! %d\n", type);
            break;
    }
}

void CFlowController::ConveyBuffers(TTime time_threshold)
{
    TInt i = 0;
    CIBuffer *p_buffer = NULL;
    TTime time = 0;
    //TTime timegap = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i++) {
        if (mpInputPins[i] && mpOutputPins[i]) {
            while (1) {
                if (mpCachedBuffer[i]) {
                    p_buffer = mpCachedBuffer[i];
                } else if (!mpInputPins[i]->PeekBuffer(p_buffer)) {
                    break;
                }

                time = p_buffer->GetBufferPTS();
                //LOG_PRINTF("flow control, buffer pts %lld, time_threshold %lld\n", time, time_threshold);
                if (mTimeThreshold == -1) { mTimeThreshold = time_threshold; }

                if (time > (time_threshold - mTimeThreshold)) {
                    mpCachedBuffer[i] = p_buffer;
                    break;
                }
                mpOutputPins[i]->SendBuffer(p_buffer);
                mpCachedBuffer[i] = NULL;
            }
        }
    }
}

void CFlowController::purgeAllChannels(TU8 disable_pin)
{
    TInt i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            if (mpCachedBuffer[i]) {
                mpCachedBuffer[i]->Release();
                mpCachedBuffer[i] = NULL;
            }
            mpInputPins[i]->Purge(disable_pin);
        }
    }
}

void CFlowController::resumeAllChannels()
{
    TInt i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Enable(1);
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

