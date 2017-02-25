/*******************************************************************************
 * video_output.cpp
 *
 * History:
 *    2014/10/14 - [Zhi He] create file
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
#include "mw_internal.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "video_output.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateVideoOutputFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoOutput::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CVideoOutput
//
//-----------------------------------------------------------------------
IFilter *CVideoOutput::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CVideoOutput *result = new CVideoOutput(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CVideoOutput::CVideoOutput(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpRenderer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mpBuffer(NULL)
    , mpRenderingBuffer(NULL)
    , mpTimer(NULL)
    , mbSendFPSStatistics(0)
    , mbPaused(0)
    , mbEOS(0)
    , mbStepMode(0)
    , mSyncStrategy(0)
    , mbSyncStarted(0)
    , mSyncOverflowThreshold(0)
    , mSyncInitialCachedFrameCount(0)
    , mSyncPlaybackCurTime(0)
    , mSyncBaseTime(0)
    , mSyncDriftTime(0)
    , mSyncEstimatedNextFrameTime(0)
    , mSyncFrameDurationNum((TTime)DDefaultVideoFramerateDen * (TTime)1000000)
    , mSyncFrameDurationDen(DDefaultVideoFramerateNum)
    , mCurrentFramCount(0)
    , mStasticsFramCount(0)
    , mStasticsBeginTime(0)
    , mStasticsEndTime(0)
{
    TUint i = 0;

    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        mpInputPins[i] = 0;
    }
}

EECode CVideoOutput::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoRendererFilter);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);
    mSyncInitialCachedFrameCount = mpPersistMediaConfig->pb_cache.precache_video_frames;
    if (mpPersistMediaConfig->pb_cache.b_constrain_latency) {
        mSyncOverflowThreshold = mpPersistMediaConfig->pb_cache.max_video_frames + mpPersistMediaConfig->pb_cache.precache_video_frames;
    } else {
        mSyncOverflowThreshold = 0;
    }

    return inherited::Construct();
}

CVideoOutput::~CVideoOutput()
{

}

void CVideoOutput::Delete()
{
    TUint i = 0;

    //LOGM_RESOURCE("CVideoOutput::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    inherited::Delete();
}

EECode CVideoOutput::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CVideoOutput::Finalize()
{
    return EECode_OK;
}

void CVideoOutput::PrintState()
{
    LOGM_FATAL("TO DO\n");
}

EECode CVideoOutput::Run()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Run();

    return EECode_OK;
}

EECode CVideoOutput::Start()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    return inherited::Start();
}

EECode CVideoOutput::Stop()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Stop();

    return EECode_OK;
}

void CVideoOutput::Pause()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Pause();

    return;
}

void CVideoOutput::Resume()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Resume();

    return;
}

void CVideoOutput::Flush()
{
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    inherited::Flush();

    return;
}

void CVideoOutput::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CVideoOutput::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoOutput::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoOutput::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoOutput::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

IInputPin *CVideoOutput::GetInputPin(TUint index)
{
    if (EConstVideoRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CVideoOutput::GetInputPin()\n", index, EConstVideoRendererMaxInputPinNumber);
    }

    return NULL;
}

IOutputPin *CVideoOutput::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CVideoOutput do not have output pin\n");
    return NULL;
}

EECode CVideoOutput::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (mnInputPinsNumber >= EConstVideoRendererMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CQueueInputPin::Create("[video input pin for CVideoOutput]", (IFilter *) this, StreamType_Video, mpWorkQ->MsgQ());
        DASSERT(mpInputPins[mnInputPinsNumber]);
        if (!mnInputPinsNumber) {
            mpCurrentInputPin = mpInputPins[0];
        }
        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CVideoOutput::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CVideoOutput can not add a output pin\n");
    return EECode_OK;
}

void CVideoOutput::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    switch (type) {

        case EEventType_Timer:
            mpWorkQ->PostMsg(EMSGType_TimeNotify, NULL);
            break;

        default:
            LOG_FATAL("to do\n");
            break;
    }
}

EECode CVideoOutput::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
    EECode err = EECode_OK;

    switch (property) {

        case EFilterPropertyType_vrenderer_trickplay_pause:
            mpWorkQ->PostMsg(ECMDType_Pause, NULL);
            break;

        case EFilterPropertyType_vrenderer_trickplay_resume:
            mpWorkQ->PostMsg(ECMDType_Resume, NULL);
            break;

        case EFilterPropertyType_vrenderer_trickplay_step:
            mpWorkQ->PostMsg(ECMDType_Step, NULL);
            break;

        case EFilterPropertyType_assign_external_module:
            mpRenderer = (IVisualDirectRendering *) p_param;
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            err = EECode_InternalLogicalBug;
            break;
    }

    return err;
}

void CVideoOutput::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 0;

    info.pName = mpModuleName;
}

void CVideoOutput::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("\t[PrintStatus] CVideoOutput[%d]: msState=%d, %s\n", mIndex, msState, gfGetModuleStateString(msState));

    for (i = 0; i < mnInputPinsNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    return;
}

EECode CVideoOutput::flushInputData()
{
    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_FLOW("before purge input pins\n");

    if (mpCurrentInputPin) {
        mpCurrentInputPin->Purge();
    }

    LOGM_FLOW("after purge input pins\n");

    return EECode_OK;
}

EECode CVideoOutput::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            flushInputData();
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Idle;
            }
            mbSendFPSStatistics = 0;
            mStasticsFramCount = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            if (!mbSendFPSStatistics) {
                sendFPSStatistics();
                mbSendFPSStatistics = 1;
            }
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_Stopped;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case EMSGType_TimeNotify:
            if (mpBuffer) {
                renderBuffer();
                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
            }
            mpTimer = NULL;
            break;

        case ECMDType_Step:
            if (!mbStepMode) {
                mbStepMode = 1;
                if (mpBuffer) {
                    renderBuffer();
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                msState = EModuleState_Step;
            }
            break;

        case ECMDType_Pause:
            if (mpBuffer) {
                renderBuffer();
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            mbPaused = 1;
            mbStepMode = 0;
            msState = EModuleState_Pending;
            break;

        case ECMDType_Resume:
            if (mpBuffer) {
                renderBuffer();
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            mbPaused = 0;
            mbStepMode = 0;
            mbSyncStarted = 0;
            msState = EModuleState_Idle;
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }

    return err;
}

void CVideoOutput::renderBuffer()
{
    if (DLikely(mpRenderer)) {
        SVisualDirectRenderingContent rendering_context;
        rendering_context.data[0] = mpBuffer->GetDataPtr(0);
        rendering_context.data[1] = mpBuffer->GetDataPtr(1);
        rendering_context.data[2] = mpBuffer->GetDataPtr(2);
        rendering_context.off_x = 0;
        rendering_context.off_y = 0;
        rendering_context.size_x = mpBuffer->mVideoWidth;
        rendering_context.size_y = mpBuffer->mVideoHeight;
        rendering_context.linesize[0] = mpBuffer->GetDataLineSize(0);
        rendering_context.linesize[1] = mpBuffer->GetDataLineSize(1);
        rendering_context.linesize[2] = mpBuffer->GetDataLineSize(2);

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
        if (DUnlikely(mpPersistMediaConfig->common_config.debug_dump.debug_print_video_realtime)) {
            static TTime last_time = 0;
            TTime cur_time = mpClockReference->GetCurrentTime();
            LOGM_PRINTF("[VO]: video output: real time stamp gap %lld\n", cur_time - last_time);
            last_time = cur_time;
        }
#endif

#ifdef DCONFIG_TEST_END2END_DELAY
        rendering_context.debug_count = mpBuffer->mDebugCount;
        rendering_context.debug_time = mpBuffer->mDebugTime;
#endif

        EECode err = mpRenderer->Render(&rendering_context, 1);
        if (EECode_OK == err) {
            if (mpRenderingBuffer) {
                mpRenderingBuffer->Release();
            }
            mpRenderingBuffer = mpBuffer;
            mpRenderingBuffer->AddRef();
        }
    }
}

void CVideoOutput::initSync()
{
    LOG_NOTICE("[config]: video frame rate, duration %lld\n", mSyncFrameDurationNum / mSyncFrameDurationDen);
    mSyncBaseTime = mpClockReference->GetCurrentTime();
    mSyncDriftTime = 0;
    mCurrentFramCount = mSyncInitialCachedFrameCount;
}

TTime CVideoOutput::getEstimatedNextFrameTime()
{
    mSyncPlaybackCurTime = mpClockReference->GetCurrentTime();
    mSyncEstimatedNextFrameTime = mSyncBaseTime + mSyncDriftTime + mCurrentFramCount * mSyncFrameDurationNum / mSyncFrameDurationDen;
    //LOG_NOTICE("[video sync debug]: mSyncPlaybackCurTime %lld, mSyncEstimatedNextFrameTime %lld\n", mSyncPlaybackCurTime, mSyncEstimatedNextFrameTime);
    return mSyncEstimatedNextFrameTime - mSyncPlaybackCurTime;
}

void CVideoOutput::sendFPSStatistics()
{
    if (mStasticsFramCount) {
        mStasticsEndTime = mpClockReference->GetCurrentTime();
        float avg_fps = ((float)mStasticsFramCount * (float)1000000) / (float)(mStasticsEndTime - mStasticsBeginTime);
        SMSG msg_fps;
        msg_fps.code = EMSGType_PlaybackStatisticsFPS;
        msg_fps.p_owner = (TULong)((IFilter *)this);
        msg_fps.f0 = avg_fps;
        mpEngineMsgSink->MsgProc(msg_fps);
    }
}

void CVideoOutput::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    CIQueue *data_queue = NULL;

    mbRun = true;
    msState = EModuleState_WaitCmd;

    mpWorkQ->CmdAck(EECode_OK);
    //LOGM_INFO("OnRun loop, start\n");

    while (mbRun) {

        //LOGM_STATE("CVideoOutput: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

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

                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    mpCurrentInputPin = (CQueueInputPin *)result.pOwner;
                    DASSERT(!mpBuffer);
                    if (mpCurrentInputPin->PeekBuffer(mpBuffer)) {
                        if (!mpPersistMediaConfig->pb_cache.b_render_video_nodelay) {
                            data_queue = mpCurrentInputPin->GetBufferQ();
                            msState = EModuleState_Running;
                        } else {
                            renderBuffer();
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                    } else {
                        LOGM_FATAL("mpCurrentInputPin->PeekBuffer(mpBuffer) fail?\n");
                        msState = EModuleState_Error;
                    }
                }
                break;

            case EModuleState_Completed:
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    msState = EModuleState_Idle;
                }
                break;

            case EModuleState_Stopped:
            case EModuleState_Pending:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Step:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                if (mpCurrentInputPin->PeekBuffer(mpBuffer)) {
                    renderBuffer();
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                break;

            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer && mpRenderer);

                if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                    DASSERT(mpEngineMsgSink);
                    if (mpEngineMsgSink) {
                        if (!mbSendFPSStatistics) {
                            sendFPSStatistics();
                            mbSendFPSStatistics = 1;
                        }
                        SMSG msg;
                        msg.code = EMSGType_PlaybackEOS;
                        msg.p_owner = (TULong)((IFilter *)this);
                        mpEngineMsgSink->MsgProc(msg);
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = EModuleState_Completed;
                    break;
                } else {
                    if (DUnlikely(!mStasticsFramCount)) {
                        mStasticsBeginTime = mpClockReference->GetCurrentTime();
                    }
                    mStasticsFramCount ++;

                    if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::NEW_SEQUENCE)) {
                        TTime cur_frame_duration_num = (TTime)mpBuffer->mVideoFrameRateDen * (TTime)1000000;
                        TTime cur_frame_duration_den = mpBuffer->mVideoFrameRateNum;
                        mSyncFrameDurationNum = cur_frame_duration_num;
                        mSyncFrameDurationDen = cur_frame_duration_den;
                        LOGM_NOTICE("Force sync, new sequence comes, buffer framerate den %d, num %lld, den %lld\n", mpBuffer->mVideoFrameRateDen, mSyncFrameDurationNum, mSyncFrameDurationDen);
                        initSync();
                        mbSyncStarted = 1;
                    } else if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
                        TTime cur_frame_duration_num = (TTime)mpBuffer->mVideoFrameRateDen * (TTime)1000000;
                        TTime cur_frame_duration_den = mpBuffer->mVideoFrameRateNum;
                        if ((cur_frame_duration_num != mSyncFrameDurationNum) || (cur_frame_duration_den != mSyncFrameDurationDen)) {
                            mSyncFrameDurationNum = cur_frame_duration_num;
                            mSyncFrameDurationDen = cur_frame_duration_den;
                            initSync();
                            mbSyncStarted = 1;
                        }
                        LOGM_NOTICE("sync point, buffer framerate den %d, num %lld, den %lld\n", mpBuffer->mVideoFrameRateDen, mSyncFrameDurationNum, mSyncFrameDurationDen);
                    }

                    if (DUnlikely(!mbSyncStarted)) {
                        initSync();
                        mbSyncStarted = 1;
                    }

                    if (DUnlikely(mSyncOverflowThreshold && (data_queue->GetDataCnt() > mSyncOverflowThreshold))) {
                        mSyncDriftTime -= 10000;
                        LOGM_NOTICE("[video sync]: decrease drift, cur drift %lld\n", mSyncDriftTime);
                    }

                    TTime time_gap = getEstimatedNextFrameTime();
                    mCurrentFramCount ++;
                    //LOG_NOTICE("[video sync debug]: time_gap %lld, data number %d\n", time_gap, data_queue->GetDataCnt());
                    if (time_gap < 20000) {
                        renderBuffer();
                    } else {
                        if (DUnlikely(mpTimer)) {
                            LOGM_WARN("mpTimer not NULL\n");
                            mpClockReference->RemoveTimer(mpTimer);
                            mpTimer = NULL;
                        }
                        mpTimer = mpClockReference->AddTimer(this, EClockTimerType_once, mSyncEstimatedNextFrameTime, 0, 1);
                        msState = EModuleState_WaitTiming;
                        break;
                    }
                }

                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
                break;

            case EModuleState_WaitTiming:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("CVideoOutput: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

