/*******************************************************************************
 * video_decoder.cpp
 *
 * History:
 *    2013/01/07 - [Zhi He] create file
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
#include "mw_internal.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "video_decoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IFilter *gfCreateScheduledVideoDecoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CScheduledVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoDecoderModuleID _guess_video_decoder_type_from_string(TChar *string)
{
    if (!string) {
        LOG_NOTICE("NULL input in _guess_video_decoder_type_from_string, choose default\n");
        return EVideoDecoderModuleID_AMBADSP;
    }

    if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
        return EVideoDecoderModuleID_AMBADSP;
    } else if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
        return EVideoDecoderModuleID_FFMpeg;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_decoder_type_from_string, choose default\n", string);
    return EVideoDecoderModuleID_AMBADSP;
}

IScheduler *getVideoDecoderScheduler(const volatile SPersistMediaConfig *p, TUint index)
{
    DASSERT(p);

    if (p) {
        TUint num = 1;

        DASSERT(p->number_scheduler_video_decoder);
        DASSERT(p->number_scheduler_video_decoder <= DMaxSchedulerGroupNumber);

        if (p->number_scheduler_video_decoder > DMaxSchedulerGroupNumber) {
            num = 1;
            LOG_FATAL("BAD p->number_scheduler_video_decoder %d\n", p->number_scheduler_video_decoder);
        } else {
            num = p->number_scheduler_video_decoder;
            if (!num) {
                num = 1;
                LOG_FATAL("BAD p->number_scheduler_video_decoder %d\n", p->number_scheduler_video_decoder);
            }
        }

        index = index % num;

        return p->p_scheduler_video_decoder[index];
    } else {
        LOG_FATAL("NULL p\n");
    }

    return NULL;
}

//-----------------------------------------------------------------------
//
// CVideoDecoder
//
//-----------------------------------------------------------------------

IFilter *CVideoDecoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CVideoDecoder *result = new CVideoDecoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CVideoDecoder::CVideoDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpDecoder(NULL)
    , mpWatchDog(NULL)
    , mCurDecoderID(EVideoDecoderModuleID_Auto)
    , mbDecoderContentSetup(0)
    , mbWaitKeyFrame(1)
    , mbDecoderRunning(0)
    , mbDecoderStarted(0)
    , mbDecoderSuspended(0)
    , mbDecoderPaused(0)
    , mbBackward(0)
    , mScanMode(0)
    , mPrefetchCount(6)
    , mCurPrefetchCount(0)
    , mbEnablePrefetch(1)
    , mbPrefetchDone(0)
    , mbLiveMode(1)
    , mbTuningPlayback(1)
    , mIdentifyerCount(0)
    , mTuningPlaybackFrameTime(DDefaultVideoFramerateDen)
    , mTuningCooldown(32)
    , mTuningCurrentTick(0)
    , mTuningCooldownRefVaule(32)
    , mBufferFullnessThreashold(16 * 1024)
    , mBufferFullnessThreasholdRefValue(16 * 1024)
    , mBufferPrefetchDataSize(0)
    , mLastSendDataSize(0)
    , mLastBufferCtlTime(0)
    , mTuningMaxTimeGap(DDefaultVideoFramerateDen * 16)
    , mLastSendFrameTime(0)
    , mLastDisplayedFrameTime(0)
    , mLastCheckedSendPTS(0)
    , mLastCheckedDisplayedPTS(0)
    , mnCurrentInputPinNumber(0)
    , mpCurInputPin(NULL)
    , mnCurrentOutputPinNumber(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpBuffer(NULL)
    , mCurVideoWidth(0)
    , mCurVideoHeight(0)
{
    TUint i = 0;
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }
    DASSERT(mpPersistMediaConfig);
    if (mpPersistMediaConfig) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
        mbTuningPlayback = mpPersistMediaConfig->compensate_from_jitter;
    }
    DASSERT(mpSystemClockReference);
}

EECode CVideoDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    if (mpPersistMediaConfig) {
        mPrefetchCount = mpPersistMediaConfig->playback_prefetch_number;
        //LOGM_INFO("mPrefetchCount %d\n", mPrefetchCount);
    }

    return inherited::Construct();
}

CVideoDecoder::~CVideoDecoder()
{

}

void CVideoDecoder::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CVideoDecoder::Delete(), before mpDecoder->Delete().\n");
    if (mpDecoder) {
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    LOGM_RESOURCE("CVideoDecoder::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    LOGM_RESOURCE("CVideoDecoder::Delete(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    LOGM_RESOURCE("CVideoDecoder::Delete(), before delete buffer pool.\n");
    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    LOGM_RESOURCE("CVideoDecoder::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CVideoDecoder::Initialize(TChar *p_param)
{
    EVideoDecoderModuleID id;
    id = _guess_video_decoder_type_from_string(p_param);

    LOGM_INFO("[Video Decoder flow], CVideoDecoder::Initialize() start, id %d\n", id);

    if (mbDecoderContentSetup) {

        LOGM_INFO("[Video Decoder flow], CVideoDecoder::Initialize() start, there's a decoder already\n");

        DASSERT(mpDecoder);
        if (!mpDecoder) {
            LOGM_FATAL("[Internal bug], why the mpDecoder is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
        mpDecoder->DestroyContext();
        mbDecoderContentSetup = 0;

        if (id != mCurDecoderID) {
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
            mpDecoder->GetObject0()->Delete();
            mpDecoder = NULL;
        }
    }

    if (!mpDecoder) {
        LOGM_INFO("[Video Decoder flow], CVideoDecoder::Initialize() start, before gfModuleFactoryVideoDecoder(%d)\n", id);
        mpDecoder = gfModuleFactoryVideoDecoder(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpDecoder) {
            mCurDecoderID = id;
        } else {
            mCurDecoderID = EVideoDecoderModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoDecoder(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[DecoderFilter flow], before mpDecoder->SetupContext()\n");
    SVideoDecoderInputParam decoder_param;
    memset(&decoder_param, 0x0, sizeof(decoder_param));
    decoder_param.index = mIndex;
    decoder_param.format = StreamFormat_H264;
    decoder_param.prefer_dsp_mode = EDSPOperationMode_UDEC;
    decoder_param.platform = EDSPPlatform_AmbaI1;
    decoder_param.cap_max_width = 1920;
    decoder_param.cap_max_height = 1080;

    mpDecoder->SetBufferPool(mpBufferPool);

    EECode err = mpDecoder->SetupContext(&decoder_param);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("SetupContext(%d), %d, %s fail\n", id, err, gfGetErrorCodeString(err));
        return err;
    }

    mbDecoderContentSetup = 1;

    LOGM_INFO("[DecoderFilter flow], CVideoDecoder::Initialize() done\n");

    return EECode_OK;
}

EECode CVideoDecoder::Finalize()
{
    if (mpDecoder) {
        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
        mpDecoder->DestroyContext();
        mbDecoderContentSetup = 0;

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->Delete(), cur id %d\n", mCurDecoderID);
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    return EECode_OK;
}

EECode CVideoDecoder::Run()
{
    //debug assert
    DASSERT(mpDecoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);

    DASSERT(mbDecoderContentSetup);
    DASSERT(!mbDecoderRunning);
    DASSERT(!mbDecoderStarted);

    mbDecoderRunning = 1;
    inherited::Run();

    return EECode_OK;
}

EECode CVideoDecoder::Start()
{
    EECode err;

    //debug assert
    DASSERT(mpDecoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);

    DASSERT(mbDecoderContentSetup);
    DASSERT(mbDecoderRunning);
    DASSERT(!mbDecoderStarted);

    if (mpDecoder) {
        LOGM_INFO("[DecoderFilter flow], CVideoDecoder::Start(), before mpDecoder->Start()\n");
        mpDecoder->Start();
        LOGM_INFO("[DecoderFilter flow], CVideoDecoder::Start(), after mpDecoder->Start()\n");
        mbDecoderStarted = 1;
    } else {
        LOGM_FATAL("NULL mpDecoder in CVideoDecoder::Start().\n");
    }

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Start)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Start);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Start), ret %d\n", err);

    return EECode_OK;
}

EECode CVideoDecoder::Stop()
{
    //debug assert
    DASSERT(mpDecoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);

    DASSERT(mbDecoderContentSetup);
    DASSERT(mbDecoderRunning);
    //DASSERT(mbDecoderStarted);

    if (mpDecoder) {
        LOGM_INFO("[DecoderFilter flow], CVideoDecoder::Stop(), before mpDecoder->Stop()\n");
        mpDecoder->Stop();
        LOGM_INFO("[DecoderFilter flow], CVideoDecoder::Stop(), after mpDecoder->Stop()\n");
        mbDecoderRunning = 0;
    } else {
        LOGM_FATAL("NULL mpDecoder in CVideoDecoder::Stop().\n");
    }

    inherited::Stop();

    return EECode_OK;
}

EECode CVideoDecoder::SwitchInput(TComponentIndex focus_input_index)
{
    EECode err;
    SCMD cmd;

    cmd.code = ECMDType_SwitchInput;
    cmd.flag = focus_input_index;
    err = mpWorkQ->SendCmd(cmd);
    return err;
}

EECode CVideoDecoder::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoDecoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin) {
            LOGM_ERROR("there's already a output pin in CVideoDecoder\n");
            return EECode_InternalLogicalBug;
        }

        mpOutputPin = COutputPin::Create("[Video output pin for CVideoDecoder filter]", (IFilter *) this);

        if (!mpOutputPin)  {
            LOGM_FATAL("COutputPin::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoDecoder filter]", 16);
        if (!mpBufferPool)  {
            LOGM_FATAL("CBufferPool::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpOutputPin->SetBufferPool(mpBufferPool);
        mpBufferPool->AddBufferNotifyListener((IEventListener *) this);

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

EECode CVideoDecoder::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (mnCurrentInputPinNumber >= EConstVideoDecoderMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached, mnCurrentInputPinNumber %d.\n", mnCurrentInputPinNumber);
            return EECode_InternalLogicalBug;
        }

        index = mnCurrentInputPinNumber;
        DASSERT(!mpInputPins[mnCurrentInputPinNumber]);
        if (mpInputPins[mnCurrentInputPinNumber]) {
            LOGM_FATAL("mpInputPins[mnCurrentInputPinNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create("[Video input pin for CVideoDecoder]", (IFilter *) this, (StreamType) type, mpWorkQ->MsgQ());
        DASSERT(mpInputPins[mnCurrentInputPinNumber]);

        if (!mpCurInputPin) {
            mpCurInputPin = mpInputPins[mnCurrentInputPinNumber];
        } else {
            mpInputPins[mnCurrentInputPinNumber]->Enable(0);
        }

        mnCurrentInputPinNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CVideoDecoder::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CVideoDecoder::GetInputPin(TUint index)
{
    if (index < mnCurrentInputPinNumber) {
        return mpInputPins[index];
    }

    LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
    return NULL;
}

EECode CVideoDecoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_video_size:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_framerate:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_format:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_bitrate:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_vdecoder_pb_speed_feedingrule: {
                DASSERT(mpWorkQ);
                DASSERT(p_param);
                if (!p_param) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }

                SPbFeedingRule *rule = (SPbFeedingRule *) p_param;
                SCMD cmd;
                cmd.code = ECMDType_UpdatePlaybackSpeed;
                cmd.res32_1 = rule->direction;
                cmd.res64_1 = rule->feeding_rule;
                cmd.res64_2 = rule->speed;
                ret = mpWorkQ->PostMsg(cmd);
                DASSERT_OK(ret);
            }
            break;

        case EFilterPropertyType_vdecoder_zoom: {
                DASSERT(mpWorkQ);
                DASSERT(p_param);
                if (!p_param) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }

                LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_DecoderZoom)...\n");
                SPlaybackZoomOnDisplay *videoZoomParas = (SPlaybackZoomOnDisplay *)p_param;
                SCMD cmd;
                cmd.code = ECMDType_DecoderZoom;
                cmd.reserved1 = videoZoomParas->render_id;
                cmd.res32_1 = ((TU32)videoZoomParas->window_width << 16) | (TU32)videoZoomParas->window_height;
                cmd.res64_1 = ((TU64)videoZoomParas->input_center_x << 48) | ((TU64)videoZoomParas->input_center_y << 32) | ((TU64)videoZoomParas->input_width << 16) | (TU64)videoZoomParas->input_height;
                ret = mpWorkQ->PostMsg(cmd);
                LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_DecoderZoom), ret %d\n", ret);

                DASSERT_OK(ret);
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CVideoDecoder::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnCurrentInputPinNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CVideoDecoder::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, mnCurrentInputPinNumber %d, mBufferFullnessThreashold %d, heart beat %d\n", msState, gfGetModuleStateString(msState), mnCurrentInputPinNumber, mBufferFullnessThreashold, mDebugHeartBeat);
    LOGM_PRINTF("mLastCheckedSendPTS %lld, mLastCheckedDisplayedPTS %lld, current gap %lld\n", mLastCheckedSendPTS, mLastCheckedDisplayedPTS, mLastCheckedSendPTS - mLastCheckedDisplayedPTS);

    for (i = 0; i < mnCurrentInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpCurInputPin) {
        mpCurInputPin->PrintStatus();
    }

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;

    return;
}

void CVideoDecoder::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    IBufferPool *pBufferPool = NULL;
    EECode err = EECode_OK;
    CIBuffer *p_out = NULL;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = 1;
    msState = EModuleState_WaitCmd;

    DASSERT(mpOutputPin);
    pBufferPool = mpOutputPin->GetBufferPool();
    DASSERT(pBufferPool);

    mpCurInputPin = mpInputPins[0];//hard code here
    if (!mpCurInputPin) {
        LOGM_ERROR("No input pin\n");
        msState = EModuleState_Error;
    }

    while (mbRun) {
        LOGM_STATE("OnRun: start switch, msState=%d, %s, mpCurInputPin %p, base %p\n", msState, gfGetModuleStateString(msState), mpCurInputPin, &mpInputPins[0]);

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Preparing:
                DASSERT(mbEnablePrefetch);
                if (mbPrefetchDone) {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue + mBufferPrefetchDataSize;
                    msState = EModuleState_Idle;
                } else {
                    if (mbPaused) {
                        msState = EModuleState_Pending;
                        break;
                    }

                    DASSERT(mpCurInputPin);
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        DASSERT(!mpBuffer);
                        if (mpCurInputPin->PeekBuffer(mpBuffer)) {
                            //LOGM_DEBUG("get buffer in prepare stage, %p, data %p, size %d, flag 0x%x, type %d\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());
                            bufferSyncPointCheck();
                            if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
                                msState = EModuleState_Completed;
                                LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                                mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else if (DUnlikely(EBufferType_VideoExtraData == mpBuffer->GetBufferType())) {
                                DASSERT(mpDecoder);
                                if (mpDecoder) {
                                    mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                                }
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else if (DUnlikely((mpBuffer->GetBufferFlags() & CIBuffer::BROKEN_FRAME))) {
                                if (mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
                                    LOGM_INFO("broken idr frame detected, flag 0x%x\n", mpBuffer->GetBufferFlags());
                                    mbWaitKeyFrame = 1;
                                }
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else if (DUnlikely(mbWaitKeyFrame)) {
                                if (DLikely((mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME))) {
                                    mbWaitKeyFrame = 0;
                                    LOGM_INFO("wait key frame done in prepare stage\n");
                                } else {
                                    LOGM_INFO("discard frame in prepare stage, wait key frame, data %p, size %ld, flag 0x%x, type %d\n", mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    break;
                                }
                            }
                        } else {
                            LOGM_FATAL("cannot PeekBuffer, ERROR here\n");
                            msState = EModuleState_Error;
                            break;
                        }

                        TU32 free_zoom = 0;
                        TU32 fullness = 0;
                        mCurPrefetchCount ++;

                        DASSERT(mpDecoder);
                        DASSERT(!mbWaitKeyFrame);
                        if (mpDecoder) {
                            mpDecoder->QueryFreeZoom(free_zoom, fullness);
                            if (DLikely(free_zoom > (mpBuffer->GetDataSize() + 128))) {
                                if (mCurPrefetchCount >= mPrefetchCount) {
                                    mBufferPrefetchDataSize += mpBuffer->GetDataSize();
                                    mpDecoder->PushData(mpBuffer, 1);
                                    mbPrefetchDone = 1;
                                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue + mBufferPrefetchDataSize;
                                    msState = EModuleState_Idle;
                                    LOGM_INFO("prefetch done, mCurPrefetchCount %d, mPrefetchCount %d\n", mCurPrefetchCount, mPrefetchCount);
                                } else {
                                    LOGM_INFO("prefetching, mCurPrefetchCount %d, mPrefetchCount %d\n", mCurPrefetchCount, mPrefetchCount);
                                    mpDecoder->PushData(mpBuffer, 0);
                                    mBufferPrefetchDataSize += mpBuffer->GetDataSize();
                                }
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            } else {
                                LOGM_ERROR("check here, free_zoom %d, mpBuffer->GetDataSize()%ld, mCurPrefetchCount %d\n", free_zoom, mpBuffer->GetDataSize(), mCurPrefetchCount);
                                mpDecoder->PushData(mpBuffer, 1);
                                mLastSendFrameTime = mpBuffer->GetBufferLinearPTS();//mpBuffer->GetBufferPTS();
                                mBufferPrefetchDataSize += mpBuffer->GetDataSize();
                                mbPrefetchDone = 1;
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue + mBufferPrefetchDataSize;
                                msState = EModuleState_Idle;
                            }
                        }
                    }

                }
                break;

            case EModuleState_Idle:

                if (mbPaused) {
                    msState = EModuleState_Pending;
                    break;
                }

                if (pBufferPool->GetFreeBufferCnt() > 0) {
                    msState = EModuleState_HasOutputBuffer;
                } else {
                    DASSERT(mpCurInputPin);
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        DASSERT(!mpBuffer);
                        if (mpCurInputPin->PeekBuffer(mpBuffer)) {
                            bufferSyncPointCheck();
                            msState = EModuleState_HasInputData;
                        }
                    }
                }
                break;

            case EModuleState_HasOutputBuffer:
                DASSERT(pBufferPool->GetFreeBufferCnt() > 0);
                DASSERT(mpCurInputPin);
                type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    DASSERT(!mpBuffer);
                    if (mpCurInputPin->PeekBuffer(mpBuffer)) {
                        bufferSyncPointCheck();
                        msState = EModuleState_Running;
                    }
                }
                break;

            case EModuleState_Error:
            case EModuleState_Completed:
            case EModuleState_Pending:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_HasInputData:
                DASSERT(mpBuffer);
                if (pBufferPool->GetFreeBufferCnt() > 0) {
                    msState = EModuleState_Running;
                } else {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                }
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer);
                DASSERT(mpDecoder);

                //LOGM_VERBOSE("get buffer in running stage, %p, data %p, size %d, flag 0x%x, type %d\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());

                if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
                    msState = EModuleState_Completed;
                    LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                    mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                } else if (DUnlikely(EBufferType_VideoExtraData == mpBuffer->GetBufferType())) {
                    DASSERT(mpDecoder);
                    if (mpDecoder) {
                        mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = EModuleState_Idle;
                    break;
                } else if (DUnlikely((mpBuffer->GetBufferFlags() & CIBuffer::BROKEN_FRAME))) {
                    if (mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
                        LOGM_INFO("broken idr frame detected, flag 0x%x\n", mpBuffer->GetBufferFlags());
                        mbWaitKeyFrame = 1;
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = EModuleState_Idle;
                    break;
                }

                if (DUnlikely(mbWaitKeyFrame)) {
                    if (mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {
                        mbWaitKeyFrame = 0;
                        LOGM_INFO("wait first key frame done\n");
                    } else {
                        LOGM_INFO("wait first key frame ... mpBuffer->GetBufferFlags() 0x%08x, data size %ld\n", mpBuffer->GetBufferFlags(), mpBuffer->GetDataSize());
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = EModuleState_Idle;
                        break;
                    }
                }

                err = mpDecoder->Decode(mpBuffer, p_out);
                mLastSendFrameTime = mpBuffer->GetBufferLinearPTS();//mpBuffer->GetBufferPTS();

                mLastSendDataSize = mpBuffer->GetDataSize() + 128;
                if (DUnlikely(EECode_OK != err)) {
                    if (mpPersistMediaConfig->app_start_exit) {
                        LOGM_NOTICE("[program start exit]: Decode return err=%d, %s\n", err, gfGetErrorCodeString(err));
                        msState = EModuleState_Completed;
                    } else {
                        LOGM_ERROR("Decode Fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        msState = EModuleState_Error;
                    }
                    mpCurInputPin->Purge(1);
                } else {
                    msState = EModuleState_Idle;
                }
                mpBuffer->Release();
                mpBuffer = NULL;

                if (mbLiveMode && mbTuningPlayback) {
                    if (DUnlikely(mTuningCurrentTick > mTuningCooldown)) {
                        mTuningCurrentTick = 0;
                        //checkBufferFullness();
                        checkDelayFromPTS();
                    } else {
                        mTuningCurrentTick ++;
                    }
                }
                break;

            default:
                LOGM_ERROR("OnRun: state invalid:  %d\n", (TUint)msState);
                break;
        }

        mDebugHeartBeat ++;
    }
}

void CVideoDecoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);

    switch (type) {
        case EEventType_BufferReleaseNotify:
            mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
            break;
        default:
            LOGM_ERROR("event type unsupported:  %d", (TInt)type);
    }
}

void CVideoDecoder::processCmd(SCMD &cmd)
{
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            flushInputData();
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            break;
        case ECMDType_Stop:
            flushInputData();
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
                if (mbEnablePrefetch && mPrefetchCount && (!mbPrefetchDone)) {
                    msState = EModuleState_Preparing;
                } else {
                    msState = EModuleState_Idle;
                }
            } else if (msState == EModuleState_Completed) {
                if (mbEnablePrefetch && mPrefetchCount && (!mbPrefetchDone)) {
                    msState = EModuleState_Preparing;
                } else {
                    msState = EModuleState_Idle;
                }
            } else if (msState == EModuleState_Error) {
                if (mbEnablePrefetch && mPrefetchCount && (!mbPrefetchDone)) {
                    msState = EModuleState_Preparing;
                } else {
                    msState = EModuleState_Idle;
                }
                LOGM_ERROR("from EModuleState_Error.\n");
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpCurInputPin) {
                mpCurInputPin->Purge();
            }
            if (mpDecoder) {
                mpDecoder->Flush();
            }
            msState = EModuleState_Pending;
            mbWaitKeyFrame = 1;
            mBufferPrefetchDataSize = 0;
            mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
            mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            if (EModuleState_Pending == msState) {
                if (mbEnablePrefetch && mPrefetchCount) {
                    mbPrefetchDone = 0;
                    mCurPrefetchCount = 0;
                    mBufferPrefetchDataSize = 0;
                    msState = EModuleState_Preparing;
                } else {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
                    msState = EModuleState_Idle;
                }
            } else if (msState == EModuleState_Completed) {
                if (mbEnablePrefetch && mPrefetchCount) {
                    mbPrefetchDone = 0;
                    mCurPrefetchCount = 0;
                    mBufferPrefetchDataSize = 0;
                    msState = EModuleState_Preparing;
                } else {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
                    msState = EModuleState_Idle;
                }
            } else if (msState == EModuleState_Error) {
                if (mbEnablePrefetch && mPrefetchCount) {
                    mbPrefetchDone = 0;
                    mCurPrefetchCount = 0;
                    mBufferPrefetchDataSize = 0;
                    msState = EModuleState_Preparing;
                } else {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
                    msState = EModuleState_Idle;
                }
            } else {
                if (mbEnablePrefetch && mPrefetchCount) {
                    mbPrefetchDone = 0;
                    mCurPrefetchCount = 0;
                    mBufferPrefetchDataSize = 0;
                    msState = EModuleState_Preparing;
                } else {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
                    msState = EModuleState_Idle;
                }
                LOGM_FATAL("invalid msState=%d in ECMDType_ResumeFlush\n", msState);
            }
            mbWaitKeyFrame = 1;
            mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Suspend: {
                //LOGM_DEBUG("suspend comes ECMDType_Suspend\n");

                msState = EModuleState_Pending;
                mbWaitKeyFrame = 1;
                mBufferPrefetchDataSize = 0;
                if (mpBuffer) {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                LOGM_NOTICE("mpCurInputPin %p, mpInputPins[0] %p, mpInputPins[1] %p, cmd.flag %d\n", mpCurInputPin, mpInputPins[0], mpInputPins[1], cmd.flag);
                DASSERT(mpCurInputPin);
                if (DLikely(mpCurInputPin)) {
                    mpCurInputPin->Purge(1);
                    mpCurInputPin = NULL;
                }
                if (mpDecoder) {
                    if ((EReleaseFlag_None != cmd.flag) && mbDecoderRunning) {
                        mbDecoderRunning = 0;
                        mpDecoder->Stop();
                    }

                    if ((EReleaseFlag_Destroy == cmd.flag) && mbDecoderContentSetup) {
                        mpDecoder->DestroyContext();
                        mbDecoderContentSetup = 0;
                    }
                }
                mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_ResumeSuspend: {
                //LOGM_DEBUG("resume suspend comes ECMDType_ResumeSuspend\n");
                //DASSERT(cmd.flag < mnCurrentInputPinNumber);
                if (DUnlikely(cmd.flag >= mnCurrentInputPinNumber)) {
                    LOGM_ERROR("cmd.flag(%d) exceed max value(%d)\n", cmd.flag, mnCurrentInputPinNumber);
                    cmd.flag = 0;
                }
                msState = EModuleState_Idle;
                mbWaitKeyFrame = 1;

                CQueueInputPin *p_input = mpInputPins[cmd.flag];

                if (DUnlikely(mpCurInputPin)) {
                    LOGM_WARN("mpCurInputPin not NULL, not suspend yet? mpCurInputPin %p, mpInputPins[0] %p, mpInputPins[1] %p, cmd.flag %d\n", mpCurInputPin, mpInputPins[0], mpInputPins[1], cmd.flag);
                    if (mpCurInputPin != p_input) {
                        mpCurInputPin->Purge(1);
                        mpCurInputPin = NULL;
                    }
                }

                if (DLikely(p_input)) {
                    TU8 *p_extra_data = NULL;
                    TMemSize extra_data_size = 0;
                    TU32 session_number = 0;
                    if (mpDecoder) {
                        p_input->GetExtraData(p_extra_data, extra_data_size, session_number);
                        mpDecoder->SetExtraData(p_extra_data, extra_data_size);
                    }
                    if (!mpCurInputPin) {
                        mpCurInputPin = p_input;
                        mpCurInputPin->Enable(1);
                    }
                    //get video paras to notify App
                    StreamFormat format;
                    TU32 video_width = 0, video_height = 0;
                    mpCurInputPin->GetVideoParams(format, video_width, video_height, p_extra_data, extra_data_size, session_number);
                    if (mpEngineMsgSink && video_width && video_height) {
                        SMSG msg;
                        memset(&msg, 0x0, sizeof(SMSG));
                        msg.code = EMSGType_NotifyUDECUpdateResolution;
                        msg.p_owner = 0;
                        msg.owner_index = mIndex;
                        msg.owner_id = 0;
                        msg.identifyer_count = mIdentifyerCount;
                        msg.p0 = video_width;
                        msg.p1 = video_height;
                        LOGM_DEBUG("ECMDType_ResumeSuspend, resolution update width %d, height %d\n", video_width, video_height);
                        mpEngineMsgSink->MsgProc(msg);
                    }
                } else {
                    LOGM_ERROR("NULL p_input\n");
                    mpWorkQ->CmdAck(EECode_Error);
                    break;
                }

                if (DLikely(mpDecoder)) {
                    if (!mbDecoderContentSetup) {
                        SVideoDecoderInputParam decoder_param;
                        memset(&decoder_param, 0x0, sizeof(decoder_param));
                        decoder_param.index = mIndex;
                        decoder_param.format = StreamFormat_H264;
                        decoder_param.prefer_dsp_mode = EDSPOperationMode_UDEC;
                        decoder_param.platform = EDSPPlatform_AmbaI1;
                        decoder_param.cap_max_width = 1920;
                        decoder_param.cap_max_height = 1080;

                        mpDecoder->SetupContext(&decoder_param);
                        mbDecoderContentSetup = 1;
                    }

                    if (!mbDecoderRunning) {
                        mbDecoderRunning = 1;
                        mpDecoder->Start();
                    }

                } else {
                    LOGM_WARN("NULL mpDecoder\n");
                }
                mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_SwitchInput:
            DASSERT(cmd.flag < mnCurrentInputPinNumber);
            mbWaitKeyFrame = 1;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (cmd.flag < mnCurrentInputPinNumber) {
                if (mpCurInputPin != mpInputPins[cmd.flag]) {
                    if (mpCurInputPin) {
                        TU8 *p_extra_data = NULL;
                        TMemSize extra_data_size = 0;
                        TU32 session_number = 0;
                        mpCurInputPin->Purge(1);
                        mpCurInputPin = mpInputPins[cmd.flag];
                        mpCurInputPin->Enable(1);
                        if (mpDecoder) {
                            mpCurInputPin->GetExtraData(p_extra_data, extra_data_size, session_number);
                            mpDecoder->SetExtraData(p_extra_data, extra_data_size);
                        }
                        //get video paras to notify App
                        StreamFormat format;
                        TU32 video_width = 0, video_height = 0;
                        mpCurInputPin->GetVideoParams(format, video_width, video_height, p_extra_data, extra_data_size, session_number);
                        if (mpEngineMsgSink && video_width && video_height) {
                            SMSG msg;
                            memset(&msg, 0x0, sizeof(SMSG));
                            msg.code = EMSGType_NotifyUDECUpdateResolution;
                            msg.p_owner = 0;
                            msg.owner_index = mIndex;
                            msg.owner_id = 0;
                            msg.identifyer_count = mIdentifyerCount;
                            msg.p0 = video_width;
                            msg.p1 = video_height;
                            LOGM_DEBUG("ECMDType_SwitchInput, resolution update width %d, height %d\n", video_width, video_height);
                            mpEngineMsgSink->MsgProc(msg);
                        }
                    }
                } else {
                    LOGM_INFO("ECMDType_SwitchInput, focus_input_index %u not changed, do nothing.\n", cmd.flag);
                }
                mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
                mpWorkQ->CmdAck(EECode_OK);
            } else {
                LOGM_ERROR("ECMDType_SwitchInput, input pin index %u invalid, mnCurrentInputPinNumber %u \n", cmd.flag, mnCurrentInputPinNumber);
                mpWorkQ->CmdAck(EECode_BadParam);
            }
            break;

        case ECMDType_NotifySynced:
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            DASSERT(msState == EModuleState_WaitCmd);
            DASSERT(!mpBuffer);
            if (msState != EModuleState_Error) {
                if (mbEnablePrefetch && mPrefetchCount) {
                    DASSERT(!mbPrefetchDone);
                    msState = EModuleState_Preparing;
                    mTuningMaxTimeGap = DDefaultVideoFramerateDen * (6 + mPrefetchCount);
                } else {
                    mBufferFullnessThreashold = mBufferFullnessThreasholdRefValue;
                    msState = EModuleState_Idle;
                    mTuningMaxTimeGap = DDefaultVideoFramerateDen * 12;
                }
                //LOGM_INFO("mTuningMaxTimeGap %lld, mPrefetchCount %d, mbLiveMode %d, mbTuningPlayback %d\n", mTuningMaxTimeGap, mPrefetchCount, mbLiveMode, mbTuningPlayback);
                mbPaused = 0;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_NotifySourceFilterBlocked:
            LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
            break;

        case ECMDType_NotifyBufferRelease:
            if (mpOutputPin->GetBufferPool()->GetFreeBufferCnt() > 0) {
                if (msState == EModuleState_Idle)
                { msState = EModuleState_HasOutputBuffer; }
                else if (msState == EModuleState_HasInputData)
                { msState = EModuleState_Running; }
            }
            break;

        case ECMDType_UpdatePlaybackSpeed: {
                if (mpDecoder) {
                    mpDecoder->SetPbRule((TU8)cmd.res32_1, (TU8)cmd.res64_1, (TU8)cmd.res64_2);
                }
            }
            break;

        case ECMDType_DecoderZoom: {
                if (DLikely(mpPersistMediaConfig)) {
                    IDSPAPI *mpDSPAPI = (IDSPAPI *)mpPersistMediaConfig->dsp_config.p_dsp_handler;
                    if (DLikely(mpDSPAPI)) {
                        SDSPControlParams params;
                        TU16 window_input_width, window_input_height, window_input_center_x, window_input_center_y, window_width, window_height;
                        TU16 video_input_width, video_input_height, video_input_center_x, video_input_center_y;
                        window_input_width = (TU16)((cmd.res64_1 >> 16) & 0xffff);
                        window_input_height = (TU16)(cmd.res64_1 & 0xffff);
                        window_input_center_x = (TU16)((cmd.res64_1 >> 48) & 0xffff);
                        window_input_center_y = (TU16)((cmd.res64_1 >> 32) & 0xffff);
                        window_width = (TU16)((cmd.res32_1 >> 16) & 0xffff);
                        window_height = (TU16)(cmd.res32_1 & 0xffff);

                        video_input_center_x = (TU16)(window_input_center_x * mCurVideoWidth / window_width);
                        video_input_center_y = (TU16)(window_input_center_y * mCurVideoHeight / window_height);
                        video_input_width = (TU16)(window_input_width * mCurVideoWidth / window_width);
                        video_input_height = (TU16)(window_input_height * mCurVideoHeight / window_height);

                        if (video_input_width < window_width) { //for video resolution > window size case, the min focus area size is window size, for 1:1 display
                            video_input_width = window_width;
                        }
                        if (video_input_height < window_height) { //for video resolution > window size case, the min focus area size is window size, for 1:1 display
                            video_input_height = window_height;
                        }

                        if ((TUint)(video_input_center_x + (video_input_width + 1) / 2) > mCurVideoWidth) {
                            video_input_center_x = mCurVideoWidth - (video_input_width + 1) / 2;
                        } else if (video_input_center_x < (video_input_width + 1) / 2) {
                            video_input_center_x = (video_input_width + 1) / 2;
                        }

                        if ((TUint)(video_input_center_y + (video_input_height + 1) / 2) > mCurVideoHeight) {
                            video_input_center_y = mCurVideoHeight - (video_input_height + 1) / 2;
                        } else if (video_input_center_y < (video_input_height + 1) / 2) {
                            video_input_center_y = (video_input_height + 1) / 2;
                        }

                        params.u8_param[0] = cmd.reserved1;
                        params.u8_param[1] = 0;
                        params.u16_param[0] = video_input_center_x;
                        params.u16_param[1] = video_input_center_y;
                        params.u16_param[2] = video_input_width;
                        params.u16_param[3] = video_input_height;
                        LOGM_INFO("decoder filter zoom, render_id=%hu, width=%hu, height=%hu, center_x=%hu, center_y=%hu. (window %hu %hu) [stream %u %u] -> x=%hu,y=%hu,w=%hu,h=%hu\n",
                                  params.u8_param[0], window_input_width,  window_input_height,  window_input_center_x,  window_input_center_y,
                                  window_width,  window_height, mCurVideoWidth, mCurVideoHeight,
                                  params.u16_param[0], params.u16_param[1] , params.u16_param[2] , params.u16_param[3]);

                        mpDSPAPI->DSPControl(EDSPControlType_UDEC_zoom, (void *)&params);
                    } else {
                        LOGM_ERROR("NULL mpDSPAPI\n");
                    }
                } else {
                    LOGM_ERROR("NULL mpPersistMediaConfig\n");
                }
            }
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

void CVideoDecoder::checkBufferFullness()
{
    TU32 free_zoom = 0;
    TU32 fullness = 0;
    TTime t = 0;

    if (DLikely(mpDecoder)) {
        mpDecoder->QueryFreeZoom(free_zoom, fullness);
        if (DUnlikely(fullness > (mLastSendDataSize + mBufferFullnessThreashold))) {
            t = mpSystemClockReference->GetCurrentTime();
            if (t < (mLastBufferCtlTime + 600000)) {
                return;
            }
            mpDecoder->TuningPB(1, mTuningPlaybackFrameTime);
            mLastBufferCtlTime = t;
            LOGM_NOTICE("tuning, current fullness %d, threashold %d, mLastSendDataSize %d, mLastBufferCtlTime %lld\n", fullness, mBufferFullnessThreashold, mLastSendDataSize, mLastBufferCtlTime);
        }
    } else {
        LOGM_ERROR("NULL mpDecoder\n");
    }
}

void CVideoDecoder::checkDelayFromPTS()
{
    TTime t = 0;

    if (DLikely(mpDecoder)) {
        mpDecoder->QueryLastDisplayedPTS(mLastDisplayedFrameTime);
        //LOGM_DEBUG("DEBUG 2, mLastDisplayedFrameTime %lld, mTuningMaxTimeGap %lld, mLastSendFrameTime %lld\n", mLastDisplayedFrameTime, mTuningMaxTimeGap, mLastSendFrameTime);
        if (DUnlikely((mLastDisplayedFrameTime + mTuningMaxTimeGap) < mLastSendFrameTime)) {
            t = mpSystemClockReference->GetCurrentTime();
            if (t < (mLastBufferCtlTime + 600000)) {
                return;
            }
            mpDecoder->TuningPB(1, mTuningPlaybackFrameTime);
            mLastBufferCtlTime = t;
            LOGM_INFO("tuning, mLastDisplayedFrameTime %lld, mLastSendFrameTime %lld, mTuningMaxTimeGap %lld, current gap %lld\n", mLastDisplayedFrameTime, mLastSendFrameTime, mTuningMaxTimeGap, mLastSendFrameTime - mLastDisplayedFrameTime);
        }
        mLastCheckedSendPTS = mLastSendFrameTime;
        mLastCheckedDisplayedPTS = mLastDisplayedFrameTime;
    } else {
        LOGM_ERROR("NULL mpDecoder\n");
    }
}

EECode CVideoDecoder::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_INFO("before purge input pins\n");

    for (index = 0; index < EConstVideoDecoderMaxInputPinNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge(1);
        }
    }

    LOGM_INFO("after purge input pins\n");

    return EECode_OK;
}

void CVideoDecoder::bufferSyncPointCheck()
{
    if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
        //update resolution
        if (mpEngineMsgSink && mpBuffer->mVideoWidth && mpBuffer->mVideoHeight) {
            SMSG msg;
            memset(&msg, 0x0, sizeof(SMSG));
            msg.code = EMSGType_NotifyUDECUpdateResolution;
            msg.p_owner = 0;
            msg.owner_index = mIndex;
            msg.owner_id = 0;
            msg.identifyer_count = mIdentifyerCount;
            msg.p0 = mpBuffer->mVideoWidth;
            msg.p1 = mpBuffer->mVideoHeight;
            mCurVideoWidth = mpBuffer->mVideoWidth;
            mCurVideoHeight = mpBuffer->mVideoHeight;
            LOGM_VERBOSE("bufferSyncPointCheck, resolution update width %d, height %d\n", mpBuffer->mVideoWidth, mpBuffer->mVideoHeight);
            mpEngineMsgSink->MsgProc(msg);
        }
        //update frame rate
        if (mpDecoder && mpBuffer->mVideoFrameRateNum && mpBuffer->mVideoFrameRateDen) {
            mpDecoder->SetFrameRate(mpBuffer->mVideoFrameRateNum, mpBuffer->mVideoFrameRateDen);
        }
    }
}

//-----------------------------------------------------------------------
//
// CScheduledVideoDecoder
//
//-----------------------------------------------------------------------

IFilter *CScheduledVideoDecoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CScheduledVideoDecoder *result = new CScheduledVideoDecoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CScheduledVideoDecoder::CScheduledVideoDecoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpDecoder(NULL)
    , mpScheduler(NULL)
    , mCurDecoderID(EVideoDecoderModuleID_Auto)
    , mbDecoderContentSetup(0)
    , mbWaitKeyFrame(1)
    , mbDecoderRunning(0)
    , mbDecoderStarted(0)
    , mbDecoderSuspended(0)
    , mbDecoderPaused(0)
    , mbBackward(0)
    , mScanMode(0)
    , mPrefetchCount(6)
    , mCurPrefetchCount(0)
    , mbEnablePrefetch(1)
    , mPriority(0)
    , mnCurrentInputPinNumber(0)
    , mpCurInputPin(NULL)
    , mnCurrentOutputPinNumber(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpBuffer(NULL)
{
    TUint i = 0;
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }

}

EECode CScheduledVideoDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    mpScheduler = getVideoDecoderScheduler((const volatile SPersistMediaConfig *)mpPersistMediaConfig, mIndex);
    DASSERT(mpScheduler);

    return inherited::Construct();
}

CScheduledVideoDecoder::~CScheduledVideoDecoder()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledVideoDecoder::~CScheduledVideoDecoder(), before mpDecoder->Delete().\n");
    if (mpDecoder) {
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::~CScheduledVideoDecoder(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::~CScheduledVideoDecoder(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before delete buffer pool.\n");
    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::~CScheduledVideoDecoder(), end.\n");
}

void CScheduledVideoDecoder::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before mpDecoder->Delete().\n");
    if (mpDecoder) {
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before delete buffer pool.\n");
    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    LOGM_RESOURCE("CScheduledVideoDecoder::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CScheduledVideoDecoder::Initialize(TChar *p_param)
{
    EVideoDecoderModuleID id;
    id = _guess_video_decoder_type_from_string(p_param);

    LOGM_INFO("[Video Decoder flow], CScheduledVideoDecoder::Initialize() start, id %d\n", id);

    if (mbDecoderContentSetup) {

        LOGM_INFO("[Video Decoder flow], CScheduledVideoDecoder::Initialize() start, there's a decoder already\n");

        DASSERT(mpDecoder);
        if (!mpDecoder) {
            LOGM_FATAL("[Internal bug], why the mpDecoder is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
        mpDecoder->DestroyContext();
        mbDecoderContentSetup = 0;

        if (id != mCurDecoderID) {
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
            mpDecoder->GetObject0()->Delete();
            mpDecoder = NULL;
        }
    }

    if (!mpDecoder) {
        LOGM_INFO("[Video Decoder flow], CScheduledVideoDecoder::Initialize() start, before gfModuleFactoryVideoDecoder(%d)\n", id);
        mpDecoder = gfModuleFactoryVideoDecoder(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpDecoder) {
            mCurDecoderID = id;
        } else {
            mCurDecoderID = EVideoDecoderModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoDecoder(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[DecoderFilter flow], before mpDecoder->SetupContext()\n");
    SVideoDecoderInputParam decoder_param;
    memset(&decoder_param, 0x0, sizeof(decoder_param));
    decoder_param.index = mIndex;
    decoder_param.format = StreamFormat_H264;
    decoder_param.prefer_dsp_mode = EDSPOperationMode_UDEC;
    decoder_param.platform = EDSPPlatform_AmbaI1;
    decoder_param.cap_max_width = 1920;
    decoder_param.cap_max_height = 1080;
    mpDecoder->SetupContext(&decoder_param);
    mbDecoderContentSetup = 1;

    LOGM_INFO("[DecoderFilter flow], CScheduledVideoDecoder::Initialize() done\n");

    return EECode_OK;
}

EECode CScheduledVideoDecoder::Finalize()
{
    if (mpDecoder) {
        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
        mpDecoder->DestroyContext();
        mbDecoderContentSetup = 0;

        LOGM_INFO("[DecoderFilter flow], before mpDecoder->Delete(), cur id %d\n", mCurDecoderID);
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    return EECode_OK;
}

EECode CScheduledVideoDecoder::Start()
{
    //debug assert
    DASSERT(mpCmdQueue);
    DASSERT(mpDecoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(mbDecoderContentSetup);
    DASSERT(mpScheduler);
    DASSERT(mpOutputPin);
    DASSERT(mpBufferPool);
    DASSERT(!mbStarted);

    mpCurInputPin = mpInputPins[0];//hard code here
    if (mpCurInputPin) {
        LOGM_ERROR("No inputpin\n");
        msState = EModuleState_Error;
    }

    if ((!mbStarted) && mpScheduler && mpCurInputPin) {
        mbStarted = 1;

        msState = EModuleState_Running;

        return mpScheduler->AddScheduledCilent((IScheduledClient *)this);
    }

    LOGM_ERROR("NULL mpScheduler or NULL mpCurInputPin\n");
    return EECode_BadState;
}

EECode CScheduledVideoDecoder::Stop()
{
    //debug assert
    DASSERT(mpCmdQueue);
    DASSERT(mpDecoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(mbDecoderContentSetup);
    DASSERT(mpScheduler);
    DASSERT(mpOutputPin);
    DASSERT(mpBufferPool);
    DASSERT(mbStarted);

    if ((mbStarted) && mpScheduler) {
        mbStarted = 0;
        LOGM_FATAL("[DEBUG]: before mpScheduler->RemoveScheduledCilent\n");
        return mpScheduler->RemoveScheduledCilent((IScheduledClient *)this);
    }

    LOGM_FATAL("NULL mpScheduler\n");
    return EECode_BadState;
}

EECode CScheduledVideoDecoder::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledVideoDecoder::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CScheduledVideoDecoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin) {
            LOGM_ERROR("there's already a output pin in CScheduledVideoDecoder\n");
            return EECode_InternalLogicalBug;
        }

        mpOutputPin = COutputPin::Create("[Video output pin for CScheduledVideoDecoder filter]", (IFilter *) this);

        if (!mpOutputPin)  {
            LOGM_FATAL("COutputPin::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CScheduledVideoDecoder filter]", 16);
        if (!mpBufferPool)  {
            LOGM_FATAL("CBufferPool::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpOutputPin->SetBufferPool(mpBufferPool);
        mpBufferPool->AddBufferNotifyListener((IEventListener *) this);

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

EECode CScheduledVideoDecoder::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (mnCurrentInputPinNumber >= EConstVideoDecoderMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnCurrentInputPinNumber;
        DASSERT(!mpInputPins[mnCurrentInputPinNumber]);
        if (mpInputPins[mnCurrentInputPinNumber]) {
            LOGM_FATAL("mpInputPins[mnCurrentInputPinNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create("[Video input pin for CScheduledVideoDecoder]", (IFilter *) this, (StreamType) type, mpCmdQueue);
        DASSERT(mpInputPins[mnCurrentInputPinNumber]);

        if (!mpCurInputPin) {
            mpCurInputPin = mpInputPins[mnCurrentInputPinNumber];
        } else {
            mpInputPins[mnCurrentInputPinNumber]->Enable(0);
        }

        mnCurrentInputPinNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CScheduledVideoDecoder::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CScheduledVideoDecoder::GetInputPin(TUint index)
{
    if (index < mnCurrentInputPinNumber) {
        return mpInputPins[index];
    }

    LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
    return NULL;
}

EECode CScheduledVideoDecoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_video_size:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_framerate:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_format:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_video_bitrate:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_vdecoder_pb_speed_feedingrule: {
                SPbFeedingRule *p_feedingrule = (SPbFeedingRule *)p_param;
                DASSERT(p_feedingrule);
                if (!p_feedingrule) {
                    LOGM_FATAL("INVALID p_param=%p\n", p_param);
                    ret = EECode_InternalLogicalBug;
                    break;
                }
                LOGM_INFO("decoder filter change pb speed, %s, %hu, rule=%hu\n", p_feedingrule->direction ? "bw" : "fw", p_feedingrule->speed, p_feedingrule->feeding_rule);
                if (mpDecoder) {
                    mpDecoder->SetPbRule(p_feedingrule->direction, p_feedingrule->feeding_rule, p_feedingrule->speed);
                }
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CScheduledVideoDecoder::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnCurrentInputPinNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CScheduledVideoDecoder::PrintStatus()
{
    TUint i = 0;

    LOGM_ALWAYS("msState=%d, %s, mnCurrentInputPinNumber %d\n", msState, gfGetModuleStateString(msState), mnCurrentInputPinNumber);

    for (i = 0; i < mnCurrentInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpCurInputPin) {
        mpCurInputPin->PrintStatus();
    }

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }

    return;
}

EECode CScheduledVideoDecoder::Scheduling(TUint times, TU32 inout_mask)
{
    SCMD cmd;
    EECode err = EECode_OK;
    TU32 free_zoom = 0;
    TU32 fullness = 0;
    TUint send_data = 0;
    EBufferType buffer_type;

    LOGM_INFO("Scheduling: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
    while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
        processCmd(cmd);
    }

    switch (msState) {

        case EModuleState_Running:

            if (mbPaused) {
                msState = EModuleState_Pending;
                break;
            }

            if (mpBuffer) {
                buffer_type = mpBuffer->GetBufferType();
                if (EBufferType_FlowControl_EOS == buffer_type) {
                    msState = EModuleState_Completed;
                    LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                    DASSERT(mpOutputPin);
                    mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    break;
                } else if (EBufferType_VideoExtraData == mpBuffer->GetBufferType()) {
                    DASSERT(mpDecoder);
                    if (mpDecoder) {
                        mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
            }

            mpDecoder->QueryFreeZoom(free_zoom, fullness);

            if (mpBuffer) {
                if ((mpBuffer->GetDataSize() + 128) < free_zoom) {
                    free_zoom -= mpBuffer->GetDataSize() + 128;
                    send_data ++;

                    err = mpDecoder->PushData(mpBuffer, 0);
                    DASSERT_OK(err);
                } else {
                    return EECode_NotRunning;
                }
            }

            times = mpCurInputPin->GetBufferQ()->GetDataCnt();
            LOGM_INFO("times %d\n", times);
            while (times --) {
                mpCurInputPin->PeekBuffer(mpBuffer);
                DASSERT(mpBuffer);
                if (mpBuffer) {

                    buffer_type = mpBuffer->GetBufferType();
                    if (EBufferType_FlowControl_EOS == buffer_type) {
                        msState = EModuleState_Completed;
                        LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                        DASSERT(mpOutputPin);
                        mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        break;
                    } else if (EBufferType_VideoExtraData == mpBuffer->GetBufferType()) {
                        DASSERT(mpDecoder);
                        if (mpDecoder) {
                            mpDecoder->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                        }
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        continue;
                    }

                    if ((mpBuffer->GetDataSize() + 128) < free_zoom) {
                        free_zoom -= mpBuffer->GetDataSize() + 128;
                        send_data ++;

                        err = mpDecoder->PushData(mpBuffer, 0);
                        DASSERT_OK(err);
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    } else {
                        break;
                    }
                }
            }

            if (!mbEnablePrefetch) {
                if (send_data) {
                    err = mpDecoder->PushData(NULL, 1);
                    DASSERT_OK(err);
                }
            } else {
                mCurPrefetchCount += send_data;
                if (mCurPrefetchCount >= mPrefetchCount) {
                    mbEnablePrefetch = 0;
                    mCurPrefetchCount = 0;
                    LOGM_INFO("prefetch done!\n");
                    err = mpDecoder->PushData(NULL, 1);
                    DASSERT_OK(err);
                }
            }
            break;

        case EModuleState_Error:
        case EModuleState_Completed:
        case EModuleState_Pending:
            mpCmdQueue->WaitMsg(&cmd, sizeof(cmd));
            processCmd(cmd);
            break;

        default:
            LOGM_ERROR("OnRun: state invalid:  %d", (TUint)msState);
            break;
    }

    mDebugHeartBeat ++;

    return EECode_OK;
}

void CScheduledVideoDecoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
{

}

void CScheduledVideoDecoder::processCmd(SCMD &cmd)
{
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {

        case ECMDType_Pause:
            DASSERT(!mbPaused);
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
                LOGM_INFO("from EModuleState_Pending.\n");
            } else if (msState == EModuleState_Completed) {
                msState = EModuleState_Idle;
                LOGM_INFO("from EModuleState_Completed.\n");
            } else if (msState == EModuleState_Error) {
                msState = EModuleState_Idle;
                LOGM_INFO("from EModuleState_Error.\n");
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpCurInputPin) {
                LOGM_INFO("ECMDType_Flush, before mpCurInputPin->Purge()\n");
                mpCurInputPin->Purge();
                LOGM_INFO("ECMDType_Flush, after mpCurInputPin->Purge()\n");
            }
            if (mpDecoder) {
                LOGM_INFO("ECMDType_Flush, before mpDecoder->Flush()\n");
                mpDecoder->Flush();
                LOGM_INFO("ECMDType_Flush, after mpDecoder->Flush()\n");
            }
            msState = EModuleState_Pending;
            mbWaitKeyFrame = 1;
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
                LOGM_INFO("ResumeFlush from EModuleState_Pending.\n");
            } else if (msState == EModuleState_Completed) {
                msState = EModuleState_Idle;
                LOGM_INFO("ResumeFlush from EModuleState_Completed.\n");
            } else if (msState == EModuleState_Error) {
                msState = EModuleState_Idle;
                LOGM_INFO("ResumeFlush from EModuleState_Error.\n");
            } else {
                msState = EModuleState_Idle;
                LOGM_FATAL("invalid msState=%d in ECMDType_ResumeFlush\n", msState);
            }
            //            mbWaitKeyFrame = 0;
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Suspend:
            msState = EModuleState_Pending;
            mbWaitKeyFrame = 1;

            DASSERT(mpCurInputPin);
            if (mpCurInputPin) {
                mpCurInputPin->Enable(0);

                mpCurInputPin->Purge();
                if (mpBuffer) {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                LOGM_INFO("ECMDType_Suspend, inputPin %p disabled.\n", mpCurInputPin);
            } else {
                LOGM_FATAL("ECMDType_Suspend, invalid mpCurInputPin %p, can't disable it.\n", mpCurInputPin);
            }

            if (cmd.flag) {
                if (mpDecoder) {
                    if (mbDecoderRunning) {
                        mbDecoderRunning = 0;
                        LOGM_INFO("[DecoderFilter flow], before mpDecoder->Stop()\n");
                        mpDecoder->Stop();
                    }

                    if (mbDecoderContentSetup) {
                        LOGM_INFO("[DecoderFilter flow], before mpDecoder->DestroyContext()\n");
                        mpDecoder->DestroyContext();
                        mbDecoderContentSetup = 0;
                    }
                }
            }
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_ResumeSuspend:
            msState = EModuleState_Idle;
            mbWaitKeyFrame = 1;
            mpInputPins[0]->Enable(1);

            if (mpDecoder) {
                if (!mbDecoderContentSetup) {
                    SVideoDecoderInputParam decoder_param;
                    memset(&decoder_param, 0x0, sizeof(decoder_param));
                    decoder_param.index = mIndex;
                    decoder_param.format = StreamFormat_H264;
                    decoder_param.prefer_dsp_mode = EDSPOperationMode_UDEC;
                    decoder_param.platform = EDSPPlatform_AmbaI1;
                    decoder_param.cap_max_width = 1920;
                    decoder_param.cap_max_height = 1080;

                    LOGM_INFO("[DecoderFilter flow], before mpDecoder->SetupContext()\n");
                    mpDecoder->SetupContext(&decoder_param);
                    mbDecoderContentSetup = 1;
                }

                if (!mbDecoderRunning) {
                    mbDecoderRunning = 1;
                    LOGM_INFO("[DecoderFilter flow], before mpDecoder->Start()\n");
                    mpDecoder->Start();
                }

            }

            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_NotifySynced:
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_NotifySourceFilterBlocked:
            LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

TInt CScheduledVideoDecoder::IsPassiveMode() const
{
    return 1;
}

TSchedulingHandle CScheduledVideoDecoder::GetWaitHandle() const
{
    return (-1);
}

TSchedulingUnit CScheduledVideoDecoder::HungryScore() const
{
    TUint data_number = 0, cmd_number = 0;

    if (mpCmdQueue) {
        cmd_number = mpCmdQueue->GetDataCnt();
    }

    if (msState == EModuleState_Running) {

        if (mpCurInputPin) {
            data_number += mpCurInputPin->GetBufferQ()->GetDataCnt();
        }

        if (mpBuffer) {
            data_number ++;
        }

        return (data_number + cmd_number);
    } else {
        return cmd_number;
    }

}

TU8 CScheduledVideoDecoder::GetPriority() const
{
    return mPriority;
}

void CScheduledVideoDecoder::Destroy()
{
    Delete();
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

