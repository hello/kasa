/*******************************************************************************
 * audio_capturer.cpp
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

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

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

#include "audio_capturer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateAudioCapturerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CAudioCapturer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EAudioInputModuleID _guess_audio_capturer_type_from_string(TChar *string)
{
    if (!string) {
        return EAudioInputModuleID_ALSA;
    }

    if (!strncmp(string, DNameTag_ALSA, strlen(DNameTag_ALSA))) {
        return EAudioInputModuleID_ALSA;
    } else if (!strncmp(string, DNameTag_AndroidAudioInput, strlen(DNameTag_AndroidAudioInput))) {
        return EAudioInputModuleID_AndroidAudioInput;
    }

    return EAudioInputModuleID_ALSA;
}
//-----------------------------------------------------------------------
//
// CAudioCapturer
//
//-----------------------------------------------------------------------
IFilter *CAudioCapturer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CAudioCapturer *result = new CAudioCapturer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CAudioCapturer::CAudioCapturer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpAudioInput(NULL)
    , mCurInputID(EAudioInputModuleID_Invalid)
    , mBytesPerFrame(2)
    , mChunkSize(1024)
    , mbInputContentSetup(0)
    , mbInputStart(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mPCMBlockSize(MAX_AUDIO_BUFFER_SIZE)
    , mpOutputBuffer(NULL)
    , mpReserveBuffer(NULL)
    , mbUseReserveBuffer(0)
    , mSpecifiedFrameSize(0)
    , mbSendSyncPoint(0)
{
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
}

EECode CAudioCapturer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioCapturerFilter);

    return inherited::Construct();
}

CAudioCapturer::~CAudioCapturer()
{
    if (mbInputStart) {
        mpAudioInput->Stop();
        mbInputStart = 0;
    }

    if (mbInputContentSetup) {
        mpAudioInput->DestroyContext();
        mbInputContentSetup = 0;
    }

    if (mpAudioInput) {
        mpAudioInput->Delete();
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

    LOGM_RESOURCE("CAudioCapturer::Delete(), before delete memory pool.\n");
    if (mpMemPool) {
        mpMemPool->Delete();
        mpMemPool = NULL;
    }

    if (mpReserveBuffer) {
        delete[] mpReserveBuffer;
        mpReserveBuffer = NULL;
    }
}

void CAudioCapturer::Delete()
{
    if (mbInputStart) {
        mpAudioInput->Stop();
        mbInputStart = 0;
    }

    if (mbInputContentSetup) {
        mpAudioInput->DestroyContext();
        mbInputContentSetup = 0;
    }

    if (mpAudioInput) {
        mpAudioInput->Delete();
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

    LOGM_RESOURCE("CAudioCapturer::Delete(), before delete memory pool.\n");
    if (mpMemPool) {
        mpMemPool->Delete();
        mpMemPool = NULL;
    }

    if (mpReserveBuffer) {
        delete[] mpReserveBuffer;
        mpReserveBuffer = NULL;
    }

    inherited::Delete();
}

EECode CAudioCapturer::Initialize(TChar *p_param)
{
    EAudioInputModuleID id;
    id = _guess_audio_capturer_type_from_string(p_param);

    LOGM_INFO("EAudioInputModuleID %d\n", id);

    if (mbInputContentSetup) {
        DASSERT(mpAudioInput);
        if (!mpAudioInput) {
            LOGM_FATAL("[Internal bug], why the mpAudioInput is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        //
        if (mbInputStart) {
            mpAudioInput->Stop();
            mbInputStart = 0;
        }
        mpAudioInput->DestroyContext();
        mbInputContentSetup = 0;

        //check id
        if (id != mCurInputID) {
            mpAudioInput->Delete();
            mpAudioInput = NULL;
            mCurInputID = EAudioInputModuleID_Invalid;
        }
    }

    if (!mpAudioInput) {
        mpAudioInput = gfModuleFactoryAudioInput(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpAudioInput) {
            mCurInputID = id;
        } else {
            LOGM_FATAL("[Internal bug], request gfCreateAudioInputAlsa(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    //setup content
    if (mpAudioInput->SetupContext(NULL) != EECode_OK) {
        LOGM_FATAL("[Internal bug], failed to mpAudioInput->SetupContext!\n");
        return EECode_InternalLogicalBug;
    }
    mBytesPerFrame = mpAudioInput->GetBitsPerFrame() >> 3;
    mChunkSize = mpAudioInput->GetChunkSize();
    mbInputContentSetup = 1;

    LOGM_INFO("[CAudioCapturer flow], CAudioCapturer::Initialize() done\n");
    return EECode_OK;
}

EECode CAudioCapturer::Finalize()
{
    if (mpAudioInput) {
        if (mbInputStart) {
            mpAudioInput->Stop();
            mbInputStart = 0;
        }
        if (mbInputContentSetup) {
            mpAudioInput->DestroyContext();
            mbInputContentSetup = 0;
        }
        if (mpAudioInput) {
            mpAudioInput->Delete();
            mpAudioInput = NULL;
        }
        mCurInputID = EAudioInputModuleID_Invalid;
    }

    return EECode_OK;
}

EECode CAudioCapturer::Run()
{
    DASSERT(mpWorkQ);
    DASSERT(mpAudioInput);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpOutputPin);

    mpWorkQ->SendCmd(ECMDType_StartRunning);
    return EECode_OK;
}

EECode CAudioCapturer::Start()
{
    DASSERT(mpWorkQ);
    DASSERT(mpAudioInput);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpOutputPin);

    mpWorkQ->SendCmd(ECMDType_Start);
    return EECode_OK;
}

EECode CAudioCapturer::Stop()
{
    DASSERT(mpWorkQ);
    DASSERT(mpAudioInput);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpOutputPin);

    DASSERT(mbInputContentSetup);

    mpWorkQ->SendCmd(ECMDType_Stop);
    return EECode_OK;
}

void CAudioCapturer::Pause()
{
    return;
}

void CAudioCapturer::Resume()
{
    return;
}

void CAudioCapturer::Flush()
{
    return;
}

void CAudioCapturer::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CAudioCapturer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CAudioCapturer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CAudioCapturer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Audio == type);

    if (StreamType_Audio == type) {

        if (!mpOutputPin) {
            DASSERT(!mpBufferPool);
            DASSERT(!mpMemPool);

            mpOutputPin = COutputPin::Create("[Audio output pin for CAudioCapturer filter]", (IFilter *) this);
            if (!mpOutputPin) {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpBufferPool = CBufferPool::Create("[Buffer pool for audio output pin in CAudioCapturer filter]", 96);
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpMemPool = CFixedMemPool::Create("[Memory pool for audio output in CAudioCapturer filter]", MAX_AUDIO_BUFFER_SIZE, 96);
            if (!mpMemPool)  {
                LOGM_FATAL("mpMemPool::Create() fail?\n");
                return EECode_NoMemory;
            }
            mpOutputPin->SetBufferPool(mpBufferPool);
            mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
        }

        EECode ret = mpOutputPin->AddSubPin(sub_index);
        if (DLikely(EECode_OK == ret)) {
            index = 0;
        } else {
            LOGM_FATAL("mpOutputPin->AddSubPin fail, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
            return ret;
        }

    } else {
        LOGM_ERROR("BAD output pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioCapturer::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("CAudioCapturer do not have output pin\n");
    return EECode_InternalLogicalBug;
}

IOutputPin *CAudioCapturer::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CAudioCapturer::GetInputPin(TUint index)
{
    LOGM_FATAL("CAudioCapturer do not have output pin\n");
    return NULL;
}

EECode CAudioCapturer::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;
    switch (property_type) {

        case EFilterPropertyType_audio_parameters:
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CAudioCapturer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CAudioCapturer::PrintStatus()
{
    LOGM_PRINTF("msState=%d, %s, heart beat %d\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat);

    if (mpOutputPin) {
        LOGM_PRINTF("       mpOutputPin[%p]'s status:\n", mpOutputPin);
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;
}

void CAudioCapturer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);

    return;
}

void CAudioCapturer::processCmd(SCMD cmd)
{
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;
        case ECMDType_Stop:
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Pause:
            DASSERT(!mbPaused);
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
                LOGM_INFO("from EModuleState_Pending.\n");
            }
            mbPaused = 0;
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Idle;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Suspend: {
                msState = EModuleState_Pending;
                if (mpAudioInput) {
                    if (mbInputStart) {
                        //mpAudioInput->Stop();
                        mbInputStart = 0;
                    }
                    //mpAudioInput->DestroyContext();
                    mbInputContentSetup = 0;
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_ResumeSuspend: {
                msState = EModuleState_Idle;

                if (mpAudioInput) {
                    if (!mbInputContentSetup) {
                        mbInputContentSetup = 1;
                    }

                    if (!mbInputStart) {
                        mbInputStart = 1;
                        //mpAudioInput->Start();//TODO, will cause ALSA segment fault
                    }
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

void CAudioCapturer::OnRun()
{
    SCMD cmd;

    DASSERT(mpAudioInput);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mbInputContentSetup);
    DASSERT(mpOutputPin);

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = 1;
    msState = EModuleState_WaitCmd;

    IBufferPool *pBufferPool = mpOutputPin->GetBufferPool();
    DASSERT(pBufferPool);

    DASSERT(!mpReserveBuffer);
    mpReserveBuffer = new TU8[MAX_AUDIO_BUFFER_SIZE];

    mDebugHeartBeat = 0;
    TUint numOfFrames = 0;
    TU64 timeStamp = 0;
    TUint bufferSize = mChunkSize * mBytesPerFrame;
    TU8 *pData = NULL;

    while (mbRun) {
        LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Halt:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                if (mbPaused) {
                    msState = EModuleState_Pending;
                    break;
                }

                if (mpWorkQ->PeekCmd(cmd)) {
                    processCmd(cmd);
                } else {
                    while (pBufferPool->GetFreeBufferCnt() == 0) {
                        LOGM_WARN("There is no free buffer in buffer pool, use the reserve buffer!!\n");
                        if (mpAudioInput->Read(mpReserveBuffer, bufferSize, &numOfFrames, &timeStamp) != EECode_OK) {
                            LOGM_ERROR("failed to read audio!\n");
                            mbUseReserveBuffer = 0;
                            break;
                        }
                        mbUseReserveBuffer = 1;
                    }

                    DASSERT(!mpOutputBuffer);
                    if (mpOutputPin->AllocBuffer(mpOutputBuffer)) {
                        DASSERT(mpOutputBuffer);
                        if (!mpOutputBuffer->GetDataPtrBase(0)) {
                            TU8 *mem = mpMemPool->RequestMemBlock(MAX_AUDIO_BUFFER_SIZE);
                            LOGM_INFO("alloc memory for pcm buffer, ret %p\n", mem);
                            if (mem) {
                                mpOutputBuffer->SetDataPtrBase(mem);
                                mpOutputBuffer->SetDataMemorySize(MAX_AUDIO_BUFFER_SIZE);
                            } else {
                                LOGM_FATAL("no memory?\n");
                                break;
                            }
                        }

                        pData = mpOutputBuffer->GetDataPtrBase(0);
                        if (mbUseReserveBuffer) {
                            memcpy(pData, mpReserveBuffer, numOfFrames * mBytesPerFrame);
                            mbUseReserveBuffer = 0;
                        } else {
                            if (mpAudioInput->Read(pData, bufferSize, &numOfFrames, &timeStamp) != EECode_OK) {
                                LOGM_ERROR("failed to read audio!\n");
                                msState = EModuleState_Error;
                                //send eos
                                sendFlowControlBuffer(EBufferType_FlowControl_EOS);
                                break;
                            }
                        }
                        mpOutputBuffer->SetDataPtr(pData);
                        mpOutputBuffer->SetDataSize((TMemSize)(numOfFrames * mBytesPerFrame));
                        //mpMemPool->ReturnBackMemBlock((MAX_AUDIO_BUFFER_SIZE - numOfFrames * mBytesPerFrame), pData);
                        mpOutputBuffer->SetBufferType(EBufferType_AudioPCMBuffer);
                        mpOutputBuffer->SetBufferPTS((TTime)timeStamp);
                        mpOutputBuffer->mContentFormat = StreamFormat_PCM_S16;
                        if (!mbSendSyncPoint) {
                            LOGM_INFO("Send sync point buffer!\n");
                            mpOutputBuffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
                            mpOutputBuffer->mAudioChannelNumber = mpPersistMediaConfig->audio_prefer_setting.channel_number;
                            mpOutputBuffer->mAudioSampleRate = mpPersistMediaConfig->audio_prefer_setting.sample_rate;
                            mpOutputBuffer->mAudioSampleFormat = mpPersistMediaConfig->audio_prefer_setting.sample_format;
                            mbSendSyncPoint = 1;
                        }

                        mpOutputPin->SendBuffer(mpOutputBuffer);
                        mpOutputBuffer = NULL;
                    }
                }
                break;

            case EModuleState_Pending:
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("CAudioCapturer: OnRun: state invalid:  %d\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
        mDebugHeartBeat++;
    }

    LOGM_PRINTF("CAudioCapturer::OnRun() end!\n");
}

void CAudioCapturer::sendFlowControlBuffer(EBufferType flowcontrol_type)
{
    if (EBufferType_FlowControl_EOS == flowcontrol_type) {
        mpOutputPin->SendBuffer(&mPersistEOSBuffer);
    }
    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

