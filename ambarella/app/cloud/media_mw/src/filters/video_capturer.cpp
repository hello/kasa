/*******************************************************************************
 * video_capturer.cpp
 *
 * History:
 *    2014/10/17 - [Zhi He] create file
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

#include "video_capturer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateVideoCapturerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoCapturer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoInputModuleID _guess_video_capturer_type_from_string(TChar *string)
{
    if (!string) {
        LOG_NOTICE("NULL input in _guess_video_capturer_type_from_string, choose default\n");
        return EVideoInputModuleID_AMBADSP;
    }

    if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
        return EVideoInputModuleID_AMBADSP;
    } else if (!strncmp(string, DNameTag_WDup, strlen(DNameTag_WDup))) {
        return EVideoInputModuleID_WinDeskDup;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_capturer_type_from_string, choose default\n", string);
    return EVideoInputModuleID_AMBADSP;
}

//-----------------------------------------------------------------------
//
// CVideoCapturer
//
//-----------------------------------------------------------------------

IFilter *CVideoCapturer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CVideoCapturer *result = new CVideoCapturer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CVideoCapturer::GetObject0() const
{
    return (CObject *) this;
}

CVideoCapturer::CVideoCapturer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pEngineMsgSink)
    , mpMutex(NULL)
    , mpCapturer(NULL)
    , mpWatchDog(NULL)
    , mbContentSetup(0)
    , mbCaptureFirstFrame(0)
    , mbRunning(0)
    , mbStarted(0)
    , mnCurrentOutputPinNumber(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mCaptureBeginTime(0)
    , mCaptureDuration(100000)
    , mCaptureCurrentCount(0)
    , mCapturedFrameCount(0)
    , mFirstFrameTime(0)
    , mLastCapTime(0)
    , mCurModuleID(EVideoInputModuleID_Auto)
{
    DASSERT(mpPersistMediaConfig);
    if (mpPersistMediaConfig) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
    }
    DASSERT(mpSystemClockReference);
}

EECode CVideoCapturer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoCapturerFilter);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    memset(&mModuleParams, 0, sizeof(SVideoCaptureParams));

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CVideoCapturer::~CVideoCapturer()
{

}

void CVideoCapturer::Delete()
{
    if (mpCapturer) {
        mpCapturer->Destroy();
        mpCapturer = NULL;
    }

    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    inherited::Delete();
}

EECode CVideoCapturer::Initialize(TChar *p_param)
{
    EVideoInputModuleID id;
    EECode err = EECode_OK;
    id = _guess_video_capturer_type_from_string(p_param);

    LOGM_INFO("Initialize() start, id %d\n", id);

    if (mbContentSetup) {

        LOGM_INFO("Initialize() start, there's a decoder already\n");

        DASSERT(mpCapturer);
        if (!mpCapturer) {
            LOGM_FATAL("[Internal bug], why the mpCapturer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("before mpCapturer->DestroyContext()\n");
        mpCapturer->DestroyContext();
        mbContentSetup = 0;

        if (id != mCurModuleID) {
            LOGM_INFO("before mpCapturer->Delete(), cur id %d request id %d\n", (TU32)mCurModuleID, (TU32)id);
            mpCapturer->Destroy();
            mpCapturer = NULL;
        }
    }

    if (!mpCapturer) {
        LOGM_INFO("Initialize() start, before gfModuleFactoryVideoInput(%d)\n", (TU32)id);
        mpCapturer = gfModuleFactoryVideoInput(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpCapturer) {
            mCurModuleID = id;
        } else {
            mCurModuleID = EVideoInputModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoInput(%d) fail\n", (TU32)id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("before mpCapturer->SetupContext()\n");
    err = mpCapturer->SetupContext(&mModuleParams);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("mpCapturer->SetupContext() failed.\n");
        return err;
    }
    mbContentSetup = 1;

    LOGM_INFO("Initialize() done\n");
    return EECode_OK;
}

EECode CVideoCapturer::Finalize()
{
    if (mpCapturer) {

        LOGM_INFO("before mpCapturer->DestroyContext()\n");
        mpCapturer->DestroyContext();
        mbContentSetup = 0;

        LOGM_INFO("before mpCapturer->Destroy(), cur id %d\n", mCurModuleID);
        mpCapturer->Destroy();
        mpCapturer = NULL;
    }

    return EECode_OK;
}

EECode CVideoCapturer::Run()
{
    return EECode_OK;
}

EECode CVideoCapturer::Exit()
{
    return EECode_OK;
}

EECode CVideoCapturer::Start()
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbStarted)) {
        LOGM_WARN("already started\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpSystemClockReference)) {
        LOGM_FATAL("NULL mpSystemClockReference\n");
        return EECode_InternalLogicalBug;
    }

    DASSERT(!mpWatchDog);
    mCaptureBeginTime = mpSystemClockReference->GetCurrentTime();

    if (mpPersistMediaConfig->video_capture_config.framerate_num && mpPersistMediaConfig->video_capture_config.framerate_den) {
        mCaptureDuration = (TTime)(((TTime)mpPersistMediaConfig->video_capture_config.framerate_den * (TTime)1000000) / (TTime)mpPersistMediaConfig->video_capture_config.framerate_num);
    } else {
        mCaptureDuration = 100000;//hard code here
    }

    LOGM_NOTICE("[config]: CVideoCapturer, capture duration %lld\n", mCaptureDuration);

    mpWatchDog = mpSystemClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mCaptureBeginTime + mCaptureDuration, mCaptureDuration, 0);
    DASSERT(mpWatchDog);
    mbStarted = 1;

    return EECode_OK;
}

void CVideoCapturer::Pause()
{
    LOGM_ERROR("TO DO\n");
}

void CVideoCapturer::Resume()
{
    LOGM_ERROR("TO DO\n");
}

void CVideoCapturer::Flush()
{
    LOGM_ERROR("TO DO\n");
}
void CVideoCapturer::ResumeFlush()
{
    LOGM_ERROR("TO DO\n");
}

EECode CVideoCapturer::Suspend(TUint release_flag)
{
    LOGM_ERROR("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoCapturer::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_ERROR("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoCapturer::Stop()
{
    if (DUnlikely(!mbStarted)) {
        LOGM_WARN("not started\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpSystemClockReference)) {
        LOGM_FATAL("NULL mpSystemClockReference\n");
        return EECode_InternalLogicalBug;
    }

    if (DLikely(mpWatchDog)) {
        mpSystemClockReference->RemoveTimer(mpWatchDog);
        mpWatchDog = NULL;
    } else {
        LOG_NOTICE("[DEBUG]: CVideoCapturer: no timer?\n");
    }

    mbStarted = 0;
    return EECode_OK;
}

EECode CVideoCapturer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoCapturer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoCapturer::SendCmd(TUint cmd)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

void CVideoCapturer::PostMsg(TUint cmd)
{
    LOGM_FATAL("TO DO\n");
}

EECode CVideoCapturer::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("Not support yet\n");

    return EECode_InternalLogicalBug;
}

EECode CVideoCapturer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin == NULL) {
            mpOutputPin = COutputPin::Create("[Video output for CVideoCapturer filter]", (IFilter *) this);

            if (!mpOutputPin)  {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }
            if (!mpPersistMediaConfig->encode.prealloc_buffer_number) {
                mpBufferPool = CBufferPool::Create("[Buffer pool for video output CVideoCapturer filter]", 16);
            } else {
                mpBufferPool = CBufferPool::Create("[Buffer pool for video output CVideoCapturer filter]", mpPersistMediaConfig->encode.prealloc_buffer_number);
            }
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpOutputPin->SetBufferPool(mpBufferPool);
        }

        index = 0;
        if (mpOutputPin->AddSubPin(sub_index) != EECode_OK) {
            LOGM_FATAL("COutputPin::AddSubPin() fail?\n");
            return EECode_Error;
        }

    } else {
        LOGM_ERROR("BAD output pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IOutputPin *CVideoCapturer::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CVideoCapturer::GetInputPin(TUint index)
{
    return NULL;
}

EECode CVideoCapturer::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CVideoCapturer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CVideoCapturer::PrintStatus()
{
    LOGM_PRINTF("count %lld, captured %lld, heart beat %d\n", mCaptureCurrentCount, mCapturedFrameCount, mDebugHeartBeat);

    if (mpOutputPin) {
        LOGM_PRINTF("       mpOutputPin free buffer count %d\n", mpOutputPin->GetBufferPool()->GetFreeBufferCnt());
    }

    mDebugHeartBeat = 0;
}

void CVideoCapturer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    switch (type) {

        case EEventType_Timer:
            mDebugHeartBeat ++;
            mCaptureCurrentCount ++;
            if (DLikely(mbContentSetup)) {
                TTime cur_time = mpSystemClockReference->GetCurrentTime();
                if (DUnlikely(cur_time < (mLastCapTime + 10000))) {
                    LOGM_PRINTF("too close capture!\n");
                    return;
                }
                if (DLikely(mpBufferPool->GetFreeBufferCnt())) {
                    CIBuffer *p_buffer = NULL;
                    //LOGM_NOTICE("[debug flow]: start capture a frame\n");
                    mpBufferPool->AllocBuffer(p_buffer, sizeof(CIBuffer));
                    EECode err = mpCapturer->Capture(p_buffer);
                    if (DUnlikely(EECode_OK != err)) {
                        p_buffer->SetBufferFlags(CIBuffer::DUP_FRAME);
                        //LOGM_WARN("send duplicate frame\n");
                    }

                    if (mCapturedFrameCount) {
                        mLastCapTime = mpSystemClockReference->GetCurrentTime();
                        TTime cur_time = mLastCapTime - mFirstFrameTime;
                        p_buffer->SetBufferDTS(cur_time);
                        p_buffer->SetBufferPTS(500000 + cur_time);
                        cur_time = (cur_time * 9) / 100;
                        p_buffer->SetBufferNativeDTS(cur_time);
                        p_buffer->SetBufferNativePTS(45000 + cur_time);
                        p_buffer->SetBufferLinearDTS(cur_time);
                        p_buffer->SetBufferLinearPTS(45000 + cur_time);
                        //LOGM_PRINTF("capture, set pts %lld, dts %lld\n", 45000 + cur_time, cur_time);
                    } else {
                        mFirstFrameTime = mpSystemClockReference->GetCurrentTime();
                        mLastCapTime = mFirstFrameTime;
                        p_buffer->SetBufferDTS(0);
                        p_buffer->SetBufferPTS(500000);
                        p_buffer->SetBufferNativeDTS(0);
                        p_buffer->SetBufferNativePTS(45000);
                        p_buffer->SetBufferLinearDTS(0);
                        p_buffer->SetBufferLinearPTS(45000);
                    }
                    //LOGM_NOTICE("[debug flow]: capture done, send it\n");
                    p_buffer->SetBufferType(EBufferType_VideoFrameBuffer);
                    mpOutputPin->SendBuffer(p_buffer);
                    mCapturedFrameCount ++;
                    //LOGM_PRINTF("[buffer count]: send a buffer\n");
                } else {
                    LOGM_PRINTF("[buffer count]: skip due to no buffers\n");
                }
            }
            break;

        default:
            LOGM_ERROR("event type unsupported: %d\n", (TInt)type);
            break;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

