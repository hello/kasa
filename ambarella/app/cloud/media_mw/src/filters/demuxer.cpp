/*******************************************************************************
 * demuxer.cpp
 *
 * History:
 *    2012/06/08 - [Zhi He] create file
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

#include "demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static EDemuxerModuleID _get_demuxer_type_from_string(TChar *string, TU8 is_scheduled)
{
    if (string) {
        if (!strncmp(string, DNameTag_PrivateRTSP, strlen(DNameTag_PrivateRTSP))) {
            if (is_scheduled) {
                return EDemuxerModuleID_PrivateRTSPScheduled;
            } else {
                return EDemuxerModuleID_PrivateRTSP;
            }
        } else if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
            return EDemuxerModuleID_FFMpeg;
        } else if (!strncmp(string, DNameTag_PrivateTS, strlen(DNameTag_PrivateTS))) {
            return EDemuxerModuleID_PrivateTS;
        } else if (!strncmp(string, DNameTag_PrivateMP4, strlen(DNameTag_PrivateMP4))) {
            return EDemuxerModuleID_PrivateMP4;
        } else if (!strncmp(string, "Uploader", strlen("Uploader"))) {
            return EDemuxerModuleID_UploadingReceiver;
        } else if (!strncmp(string, DNonePerferString, strlen(DNonePerferString))) {
            return EDemuxerModuleID_Auto;
        }
        LOG_WARN("unknown string tag(%s), choose default\n", string);
    } else {
        LOG_ERROR("NULL input string\n");
    }

    return EDemuxerModuleID_Invalid;
}

static EDemuxerModuleID _guess_demuxer_type_from_url(TChar *string, TU8 is_scheduled)
{
    if (string) {
        if (!strncmp(string, "rtsp://", strlen("rtsp://"))) {
            if (!is_scheduled) {
                return EDemuxerModuleID_PrivateRTSP;
            } else {
                return EDemuxerModuleID_PrivateRTSPScheduled;
            }
        } else if (!strncmp(string, "uploading://", strlen("uploading://"))) {
            return EDemuxerModuleID_UploadingReceiver;
        } else {
            TChar *p_ext = strrchr(string, '.');
            if (p_ext) {
                if ((!strncmp(p_ext, ".ts", strlen(".ts"))) || (!strncmp(p_ext, ".TS", strlen(".TS")))) {
                    return EDemuxerModuleID_PrivateTS;
                } else if ((!strncmp(p_ext, ".mp4", strlen(".mp4"))) || (!strncmp(p_ext, ".MP4", strlen(".MP4")))) {
                    return EDemuxerModuleID_PrivateMP4;
                }
            }
        }
    } else {
        LOG_FATAL("NULL input string\n");
    }

#ifdef BUILD_OS_WINDOWS
#ifdef BUILD_MODULE_FFMPEG
    return EDemuxerModuleID_FFMpeg;
#endif
#endif

    return EDemuxerModuleID_PrivateTS;
}

IFilter *gfCreateDemuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CDemuxer
//
//-----------------------------------------------------------------------

IFilter *CDemuxer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CDemuxer *result = new CDemuxer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CDemuxer::GetObject0() const
{
    return (CObject *) this;
}

CDemuxer::CDemuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpStreamCodecInfos(NULL)
    , mpDemuxer(NULL)
    , mpClockListener(NULL)
    , mCurDemexerID(EDemuxerModuleID_Auto)
    , mPresetDemuxerID(EDemuxerModuleID_Invalid)
    , mbDemuxerContentSetup(0)
    , mbDemuxerOutputSetup(0)
    , mbDemuxerRunning(0)
    , mbDemuxerStarted(0)
    , mbDemuxerSuspended(0)
    , mbDemuxerPaused(0)
    , mbBackward(0)
    , mScanMode(0)
    , mbEnableAudio(0)
    , mbEnableVideo(0)
    , mbEnableSubtitle(0)
    , mbEnablePrivateData(0)
    , mPriority(1)
    , mbPreSetReceiveBufferSize(0)
    , mbPreSetSendBufferSize(0)
    , mbReconnectDone(0)
    , mnRetryUrlMaxCount(536870912)
    , mnCurrentRetryUrlCount(0)
    , mReceiveBufferSize(128 * 1024)
    , mSendBufferSize(128 * 1024)
    , mnTotalOutputPinNumber(0)
    , mpInputString(NULL)
    , mInputStringBufferSize(0)
{
    TUint j;
    for (j = 0; j < EConstMaxDemuxerMuxerStreamNumber; j++) {
        mpOutputPins[j] = NULL;
        mpBufferPool[j] = NULL;
        mpMemPool[j] = NULL;
    }

    mpExternalVideoPostProcessingCallbackContext = NULL;
    mpExternalVideoPostProcessingCallback = NULL;
}

EECode CDemuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleDemuxerFilter);
    return EECode_OK;
}

CDemuxer::~CDemuxer()
{

}

void CDemuxer::Delete()
{
    TUint i = 0;

    if (mpDemuxer) {
        mpDemuxer->GetObject0()->Delete();
        mpDemuxer = NULL;
    }
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpOutputPins[i]) {
            mpOutputPins[i]->Delete();
            mpOutputPins[i] = NULL;
        }

        if (mpBufferPool[i]) {
            mpBufferPool[i]->Delete();
            mpBufferPool[i] = NULL;
        }

        if (mpMemPool[i]) {
            mpMemPool[i]->GetObject0()->Delete();
            mpMemPool[i] = NULL;
        }
    }

    if (mpInputString) {
        DDBG_FREE(mpInputString, "DMIS");
        mpInputString = NULL;
    }

    inherited::Delete();
}

void CDemuxer::finalize(TUint delete_demuxer)
{
    if (DUnlikely(!mpDemuxer)) {
        LOGM_WARN("NULL mpDemuxer\n");
        return;
    }

    if (mbDemuxerRunning) {
        mbDemuxerRunning = 0;
        LOGM_INFO("before mpDemuxer->Stop()\n");
        mpDemuxer->Stop();
    }

    LOGM_INFO("before mpDemuxer->DestroyContext()\n");
    mpDemuxer->DestroyContext();
    mbDemuxerContentSetup = 0;

    if (delete_demuxer) {
        LOGM_INFO("before mpDemuxer->Delete(), cur id %d\n", mCurDemexerID);
        mbDemuxerOutputSetup = 0;
        mpDemuxer->GetObject0()->Delete();
        mpDemuxer = NULL;
    }
}

EECode CDemuxer::initialize(TChar *p_param, void *p_agent)
{
    EDemuxerModuleID id;
    EECode err = EECode_OK;

    if ((EDemuxerModuleID_Auto == mPresetDemuxerID) || (EDemuxerModuleID_Invalid == mPresetDemuxerID)) {
        id = _guess_demuxer_type_from_url(p_param, (TU8)mpPersistMediaConfig->number_scheduler_network_reciever);
    } else {
        id = _guess_demuxer_type_from_url(p_param, (TU8)mpPersistMediaConfig->number_scheduler_network_reciever);
        if (EDemuxerModuleID_PrivateRTSPScheduled != id) {
            id = mPresetDemuxerID;
        }
    }

    if (mbDemuxerContentSetup) {
        finalize(id != mCurDemexerID);
    }

    if (!mpDemuxer) {
        mpDemuxer = gfModuleFactoryDemuxer(id, mpPersistMediaConfig, mpEngineMsgSink, mIndex);
        if (mpDemuxer) {
            mCurDemexerID = id;
        } else {
            mCurDemexerID = EDemuxerModuleID_Auto;
            LOGM_FATAL("[Internal bug], gfModuleFactoryDemuxer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    if (!mbDemuxerOutputSetup) {
        mpDemuxer->SetupOutput(mpOutputPins, mpBufferPool, mpMemPool, mpEngineMsgSink);
        mbDemuxerOutputSetup = 1;
    }

    err = mpDemuxer->SetupContext(p_param, p_agent);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("SetupContext(%s) fail\n", p_param);
        return err;
    }

    if (mpExternalVideoPostProcessingCallbackContext && mpExternalVideoPostProcessingCallback) {
        mpDemuxer->SetVideoPostProcessingCallback(mpExternalVideoPostProcessingCallbackContext, mpExternalVideoPostProcessingCallback);
    }

    mbDemuxerContentSetup = 1;
    return EECode_OK;
}

EECode CDemuxer::initialize_vod(TChar *p_param, void *p_agent)
{
    EDemuxerModuleID id = EDemuxerModuleID_PrivateTS;
    EECode err = EECode_OK;

    if (mbDemuxerContentSetup) {
        finalize(id != mCurDemexerID);
    }

    if (!mpDemuxer) {
        mpDemuxer = gfModuleFactoryDemuxer(id, mpPersistMediaConfig, mpEngineMsgSink, mIndex);
        if (mpDemuxer) {
            mCurDemexerID = id;
        } else {
            mCurDemexerID = EDemuxerModuleID_Auto;
            LOGM_FATAL("[Internal bug], gfModuleFactoryDemuxer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    if (!mbDemuxerOutputSetup) {
        mpDemuxer->SetupOutput(mpOutputPins, mpBufferPool, mpMemPool, mpEngineMsgSink);
        mbDemuxerOutputSetup = 1;
    }

    err = mpDemuxer->SetupContext(p_param, p_agent);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("SetupContext(%s) fail\n", p_param);
        return err;
    }

    if (mpExternalVideoPostProcessingCallbackContext && mpExternalVideoPostProcessingCallback) {
        mpDemuxer->SetVideoPostProcessingCallback(mpExternalVideoPostProcessingCallbackContext, mpExternalVideoPostProcessingCallback);
    }

    mbDemuxerContentSetup = 1;
    return EECode_OK;
}

EECode CDemuxer::copyString(TChar *p_param)
{
    TU32 len = strlen(p_param);
    if (DUnlikely(len >= mInputStringBufferSize)) {
        mInputStringBufferSize = len + 1;
        if (mpInputString) {
            DDBG_FREE(mpInputString, "DMIS");
            mpInputString = NULL;
        }
        mpInputString = (TChar *) DDBG_MALLOC(mInputStringBufferSize, "DMIS");
        if (DUnlikely(!mpInputString)) {
            LOG_FATAL("Initialize no memory, request size %d\n", mInputStringBufferSize);
            mInputStringBufferSize = 0;
            return EECode_NoMemory;
        }
    }

    snprintf(mpInputString, mInputStringBufferSize, "%s", p_param);

    return EECode_OK;
}

EECode CDemuxer::Initialize(TChar *p_param)
{
    mPresetDemuxerID = _get_demuxer_type_from_string(p_param, (TU8)mpPersistMediaConfig->number_scheduler_network_reciever);
    return EECode_OK;
#if 0
    if (!p_param) {
        LOGM_PRINTF("Initialize(NULL), not setup demuxer at this time\n");
        return EECode_OK;
    }

    return copyString(p_param);
#endif
}

EECode CDemuxer::Finalize()
{
    finalize(1);

    return EECode_OK;
}

EECode CDemuxer::Run()
{
    return EECode_OK;
}

EECode CDemuxer::Exit()
{
    return EECode_OK;
}

EECode CDemuxer::Start()
{
    mbDemuxerStarted = 1;
    if (!mpInputString) {
        return EECode_OK;
    }

    if (mpDemuxer) {
        finalize(1);
    }

    EECode err = initialize(mpInputString, NULL);
    SMSG msg;
    memset(&msg, 0x0, sizeof(SMSG));
    if (EECode_OK != err) {
        LOG_NOTICE("initialize fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        msg.code = EMSGType_OpenSourceFail;
        mpEngineMsgSink->MsgProc(msg);
        return err;
    } else {
        msg.code = EMSGType_OpenSourceDone;
        mpEngineMsgSink->MsgProc(msg);
    }

    mpDemuxer->Start();
    mbDemuxerRunning = 1;

    return EECode_OK;
}

EECode CDemuxer::Stop()
{
    if (mpDemuxer) {
        if (mbDemuxerRunning) {
            mbDemuxerRunning = 0;
            //LOGM_INFO("before mpDemuxer->Stop()\n");
            mpDemuxer->Stop();
        }
    }

    return EECode_OK;
}

void CDemuxer::Pause()
{
    if (mpDemuxer) {
        DASSERT(!mbDemuxerPaused);
        mpDemuxer->Pause();
        mbDemuxerPaused = 1;
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::Pause().\n");
    }

    return;
}

void CDemuxer::Resume()
{
    if (mpDemuxer) {
        mpDemuxer->Resume();
        mbDemuxerPaused = 0;
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::Resume().\n");
    }

    return;
}

void CDemuxer::Flush()
{
    if (mpDemuxer) {
        mpDemuxer->Flush();
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::Flush().\n");
    }

    return;
}

void CDemuxer::ResumeFlush()
{
    if (mpDemuxer) {
        mpDemuxer->ResumeFlush();
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::ResumeFlush().\n");
    }
    return;
}

EECode CDemuxer::Suspend(TUint release_context)
{
    if (mpDemuxer) {
        mpDemuxer->Flush();
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::Suspend().\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

EECode CDemuxer::ResumeSuspend(TComponentIndex input_index)
{
    if (mpDemuxer) {
        mpDemuxer->ResumeFlush();
    } else {
        LOGM_NOTICE("NULL mpDemuxer in CDemuxer::ResumeSuspend().\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

EECode CDemuxer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CDemuxer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CDemuxer::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CDemuxer::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CDemuxer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    EECode ret;

    if (StreamType_Video == type) {
        if (!mpOutputPins[EDemuxerVideoOutputPinIndex]) {
            mpOutputPins[EDemuxerVideoOutputPinIndex] = COutputPin::Create("[Demuxer video output pin]", (IFilter *) this);
            if (!mpOutputPins[EDemuxerVideoOutputPinIndex]) {
                LOGM_FATAL("Demuxer video output pin::Create() fail\n");
                return EECode_NoMemory;
            }
            DASSERT(!mpBufferPool[EDemuxerVideoOutputPinIndex]);
            mpBufferPool[EDemuxerVideoOutputPinIndex] = CBufferPool::Create("[Demuxer buffer pool for video output pin]", 96);
            if (!mpBufferPool[EDemuxerVideoOutputPinIndex]) {
                LOGM_FATAL("CBufferPool::Create() fail\n");
                return EECode_NoMemory;
            }
            mpOutputPins[EDemuxerVideoOutputPinIndex]->SetBufferPool(mpBufferPool[EDemuxerVideoOutputPinIndex]);
            mpMemPool[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(12 * 1024 * 1024);
            mnTotalOutputPinNumber ++;
        }

        if (mpOutputPins[EDemuxerVideoOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerVideoOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerVideoOutputPinIndex;
                mbEnableVideo = 1;
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
            DASSERT(!mpBufferPool[EDemuxerAudioOutputPinIndex]);
            mpBufferPool[EDemuxerAudioOutputPinIndex] = CBufferPool::Create("[Demuxer buffer pool for audio output pin]", 192);
            if (!mpBufferPool[EDemuxerAudioOutputPinIndex]) {
                LOGM_FATAL("CSimpleBufferPool::Create() fail\n");
                return EECode_NoMemory;
            }
            mpOutputPins[EDemuxerAudioOutputPinIndex]->SetBufferPool(mpBufferPool[EDemuxerAudioOutputPinIndex]);
            mpMemPool[EDemuxerAudioOutputPinIndex] = CRingMemPool::Create(256 * 1024);
            mnTotalOutputPinNumber ++;
        }
        if (mpOutputPins[EDemuxerAudioOutputPinIndex]) {
            ret = mpOutputPins[EDemuxerAudioOutputPinIndex]->AddSubPin(sub_index);
            DASSERT_OK(ret);

            if (EECode_OK == ret) {
                index = EDemuxerAudioOutputPinIndex;
                mbEnableAudio = 1;
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
                mbEnableSubtitle = 1;
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
                mbEnablePrivateData = 1;
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

EECode CDemuxer::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("Demuxer do not have input pin\n");
    return EECode_InternalLogicalBug;
}

IOutputPin *CDemuxer::GetOutputPin(TUint index, TUint sub_index)
{
    TUint max_sub_index = 0;

    if (EDemuxerOutputPinNumber > index) {
        if (mpOutputPins[index]) {
            max_sub_index = mpOutputPins[index]->NumberOfPins();
            if (max_sub_index > sub_index) {
                return mpOutputPins[index];
            } else {
                LOGM_ERROR("BAD sub_index %d, max value %d, in CDemuxer::GetOutputPin(%u, %u)\n", sub_index, max_sub_index, index, sub_index);
            }
        } else {
            LOGM_FATAL("Why the pointer is NULL? in CDemuxer::GetOutputPin(%u, %u)\n", index, sub_index);
        }
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CDemuxer::GetOutputPin(%u, %u)\n", index, EDemuxerOutputPinNumber, index, sub_index);
    }

    return NULL;
}

IInputPin *CDemuxer::GetInputPin(TUint index)
{
    LOGM_FATAL("Demuxer do not have input pin\n");
    return NULL;
}

EECode CDemuxer::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_demuxer_reconnect_server:
            mnCurrentRetryUrlCount = 0;
            mbReconnectDone = 0;
            if (mpDemuxer) {
                ret = mpDemuxer->ReconnectServer();
                if (EECode_OK == ret) {
                    LOGM_NOTICE("ReconnectServer success\n");
                    mnCurrentRetryUrlCount = 0;
                    mbReconnectDone = 1;
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(SMSG));
                    msg.code = EMSGType_ClientReconnectDone;
                    mpEngineMsgSink->MsgProc(msg);
                } else {
                    LOGM_NOTICE("ReconnectServer fail, retry mnCurrentRetryUrlCount %d, mnRetryUrlMaxCount %d\n", mnCurrentRetryUrlCount, mnRetryUrlMaxCount);
                    if (DLikely(mpPersistMediaConfig->p_system_clock_reference)) {
                        DASSERT(!mnCurrentRetryUrlCount);

                        if (DLikely(mnCurrentRetryUrlCount < mnRetryUrlMaxCount)) {
                            mnCurrentRetryUrlCount ++;
                            //LOGM_NOTICE("set timer\n");
                            mpClockListener = mpPersistMediaConfig->p_system_clock_reference->AddTimer(this, EClockTimerType_once, mpPersistMediaConfig->p_system_clock_reference->GetCurrentTime() + mpPersistMediaConfig->streaming_autoconnect_interval, 5000000);
                        } else {
                            LOGM_ERROR("max retry count reached\n");
                        }
                    } else {
                        LOGM_FATAL("NULL mpPersistMediaConfig->p_system_clock_reference\n");
                    }
                }
            }
            break;

        case EFilterPropertyType_update_source_url:
            ret = copyString((TChar *)p_param);

            if (DUnlikely(EECode_OK != ret)) {
                LOG_ERROR("copyString ret %d, %s\n", ret, gfGetErrorCodeString(ret));
                return ret;
            }

            if (!mbDemuxerStarted) {
                break;
            }

            if (mpDemuxer) {
                finalize(1);
            }
            ret = initialize((TChar *)p_param, NULL);
            if (DUnlikely(EECode_OK != ret)) {
                SMSG msg;
                memset(&msg, 0x0, sizeof(SMSG));
                msg.code = EMSGType_OpenSourceFail;
                mpEngineMsgSink->MsgProc(msg);
                return ret;
            } else {
                SMSG msg;
                memset(&msg, 0x0, sizeof(SMSG));
                msg.code = EMSGType_OpenSourceDone;
                mpEngineMsgSink->MsgProc(msg);
            }
            mpDemuxer->Start();
            mbDemuxerRunning = 1;
            break;

        case EFilterPropertyType_demuxer_enable_video:
            mbEnableVideo = 1;
            mpDemuxer->EnableVideo(1);
            break;

        case EFilterPropertyType_demuxer_enable_audio:
            mbEnableAudio = 1;
            mpDemuxer->EnableAudio(1);
            break;

        case EFilterPropertyType_demuxer_disable_video:
            mbEnableVideo = 0;
            mpDemuxer->EnableVideo(0);
            break;

        case EFilterPropertyType_demuxer_disable_audio:
            mbEnableAudio = 0;
            mpDemuxer->EnableAudio(0);
            break;

        case EFilterPropertyType_demuxer_pb_speed_feedingrule: {
                SPbFeedingRule *p_feedingrule = (SPbFeedingRule *)p_param;
                DASSERT(p_feedingrule);
                if (!p_feedingrule) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }
                LOGM_INFO("demuxer filter change pb speed, %s, %04x, rule=%hu\n", p_feedingrule->direction ? "bw" : "fw", p_feedingrule->speed, p_feedingrule->feeding_rule);
                if (mpDemuxer) {
                    ret = mpDemuxer->SetPbRule(p_feedingrule->direction, p_feedingrule->feeding_rule, p_feedingrule->speed);
                }
            }
            break;

        case EFilterPropertyType_demuxer_seek: {
                SPbSeek *p_seektarget = (SPbSeek *)p_param;
                DASSERT(p_seektarget);
                if (!p_seektarget) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }
                LOGM_INFO("demuxer filter seek target %lld ms, position %d\n", p_seektarget->target, p_seektarget->position);
                if (mpDemuxer) {
                    ret = mpDemuxer->Seek(p_seektarget->target, p_seektarget->position);
                }
            }
            break;

        case EFilterPropertyType_playback_loop_mode: {
                TU32 *p_loop_mode = (TU32 *)p_param;
                DASSERT(p_loop_mode);
                if (!p_loop_mode) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }
                LOGM_INFO("demuxer filter set playback loop mode %u\n", *p_loop_mode);
                if (mpDemuxer) {
                    ret = mpDemuxer->SetPbLoopMode(p_loop_mode);
                }
            }
            break;

        case EFilterPropertyType_demuxer_priority: {
                SPriority *priority = (SPriority *)p_param;
                DASSERT(priority);
                if (set_or_get) {
                    mPriority = priority->priority;
                } else {
                    priority->priority = mPriority;
                }
            }
            break;

        case EFilterPropertyType_demuxer_receive_buffer_size: {
                SBufferSetting *buffer_setting = (SBufferSetting *)p_param;
                DASSERT(buffer_setting);
                if (set_or_get) {
                    mReceiveBufferSize = buffer_setting->size;
                    mbPreSetReceiveBufferSize = 1;
                } else {
                    if (mbPreSetReceiveBufferSize) {
                        buffer_setting->size = mReceiveBufferSize;
                    } else {
                        buffer_setting->size = 0;
                    }
                }
            }
            break;

        case EFilterPropertyType_demuxer_send_buffer_size: {
                SBufferSetting *buffer_setting = (SBufferSetting *)p_param;
                DASSERT(buffer_setting);
                if (set_or_get) {
                    mSendBufferSize = buffer_setting->size;
                    mbPreSetSendBufferSize = 1;
                } else {
                    if (mbPreSetSendBufferSize) {
                        buffer_setting->size = mSendBufferSize;
                    } else {
                        buffer_setting->size = 0;
                    }
                }
            }
            break;

        case EFilterPropertyType_demuxer_update_source_url: {
                //vod
                if (!mpDemuxer) {
                    ret = initialize_vod(mpInputString, NULL);
                    if (DUnlikely(EECode_OK != ret)) {
                        LOGM_ERROR("initialize_vod() fail\n");
                        return ret;
                    }
                }
                SContextInfo *pContext = (SContextInfo *)p_param;
                DASSERT(pContext->pChannelName);
                mpDemuxer->UpdateContext(pContext);
            }
            break;

        case EFilterPropertyType_demuxer_getextradata: {
                DASSERT(p_param);
                if (!mpDemuxer) {
                    ret = initialize_vod(mpInputString, NULL);
                    if (DUnlikely(EECode_OK != ret)) {
                        LOGM_ERROR("initialize_vod() fail\n");
                        return ret;
                    }
                }
                SStreamingSessionContent *pContent = (SStreamingSessionContent *)p_param;
                ret = mpDemuxer->GetExtraData(pContent);
            }
            break;

        case EFilterPropertyType_demuxer_navigation_seek:
            mpDemuxer->NavigationSeek((SContextInfo *)p_param);
            break;

        case EFilterPropertyType_resume_channel:
            mpDemuxer->ResumeChannel();
            break;

        case EFilterPropertyType_set_post_processing_callback: {
                SExternalDataProcessingCallback *set_callback = (SExternalDataProcessingCallback *) p_param;
                if (mpDemuxer) {
                    mpDemuxer->SetVideoPostProcessingCallback(set_callback->callback_context, set_callback->callback);
                }
                mpExternalVideoPostProcessingCallbackContext = set_callback->callback_context;
                mpExternalVideoPostProcessingCallback = set_callback->callback;
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CDemuxer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = mnTotalOutputPinNumber;

    info.pName = mpModuleName;
}

void CDemuxer::PrintStatus()
{
    TUint i = 0;
    LOGM_PRINTF("\t[PrintStatus] CDemuxer[%d]: mnTotalOutputPinNumber %d\n", mIndex, mnTotalOutputPinNumber);

    for (i = 0; i < mnTotalOutputPinNumber; i ++) {
        DASSERT(mpOutputPins[i]);
        if (mpOutputPins[i]) {
            mpOutputPins[i]->PrintStatus();
        }
        if (mpBufferPool[i]) {
            mpBufferPool[i]->PrintStatus();
        }
    }

    if (mpDemuxer) {
        mpDemuxer->GetObject0()->PrintStatus();
    }
}

void CDemuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    EECode ret = EECode_OK;

    switch (type) {
        case EEventType_Timer:
            DASSERT(mnCurrentRetryUrlCount);
            LOGM_NOTICE("handle timer\n");
            if (!mpClockListener) {
                DASSERT(mbReconnectDone);
                LOGM_WARN("no mpClockListener, why comes here\n");
                break;
            }
            if (DLikely(mpDemuxer)) {
                ret = mpDemuxer->ReconnectServer();
                if (EECode_OK == ret) {
                    LOGM_NOTICE("ReconnectServer success\n");
                    mnCurrentRetryUrlCount = 0;
                    mpClockListener = NULL;
                    mbReconnectDone = 1;
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(SMSG));
                    msg.code = EMSGType_ClientReconnectDone;
                    mpEngineMsgSink->MsgProc(msg);
                } else {
                    LOGM_NOTICE("ReconnectServer fail, retry mnCurrentRetryUrlCount %d, mnRetryUrlMaxCount %d\n", mnCurrentRetryUrlCount, mnRetryUrlMaxCount);
                    if (DLikely(mpPersistMediaConfig->p_system_clock_reference)) {

                        if (DLikely(mnCurrentRetryUrlCount < mnRetryUrlMaxCount)) {
                            mnCurrentRetryUrlCount ++;
                            LOGM_NOTICE("add timer again\n");
                            mpClockListener = mpPersistMediaConfig->p_system_clock_reference->AddTimer(this, EClockTimerType_once, mpPersistMediaConfig->p_system_clock_reference->GetCurrentTime() + mpPersistMediaConfig->streaming_autoconnect_interval, 5000000);
                        } else {
                            LOGM_ERROR("retry %d fail, server died.\n", mnCurrentRetryUrlCount);
                        }
                    } else {
                        LOGM_FATAL("NULL mpPersistMediaConfig->p_system_clock_reference\n");
                    }
                }
            } else {
                LOGM_FATAL("NULL mpDemuxer\n");
            }
            break;

        default:
            LOGM_FATAL("not processed event! %d\n", type);
            break;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

