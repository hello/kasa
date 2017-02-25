/*******************************************************************************
 * video_decoder_v2.cpp
 *
 * History:
 *    2014/10/23 - [Zhi He] create file
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

#include "video_decoder_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateVideoDecoderFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoDecoderV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoDecoderModuleID _guess_video_decoder_type_from_string(TChar *string)
{
    if (!string) {
        LOG_NOTICE("NULL input in _guess_video_decoder_type_from_string, choose default\n");
        return EVideoDecoderModuleID_OPTCDEC;
    }

    if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
        return EVideoDecoderModuleID_AMBADSP;
    } else if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
        return EVideoDecoderModuleID_FFMpeg;
    } else if (!strncmp(string, DNameTag_OPTCDec, strlen(DNameTag_OPTCDec))) {
        return EVideoDecoderModuleID_OPTCDEC;
    } else if (!strncmp(string, DNameTag_HEVCHM, strlen(DNameTag_HEVCHM))) {
        return EVideoDecoderModuleID_HEVC_HM;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_decoder_type_from_string, choose default\n", string);
    return EVideoDecoderModuleID_OPTCDEC;
}

//-----------------------------------------------------------------------
//
// CVideoDecoderV2
//
//-----------------------------------------------------------------------

IFilter *CVideoDecoderV2::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CVideoDecoderV2 *result = new CVideoDecoderV2(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CVideoDecoderV2::CVideoDecoderV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpDecoder(NULL)
    , mCurDecoderID(EVideoDecoderModuleID_Auto)
    , mCurDecoderMode(EDecoderMode_Invalid)
    , mCurrentFormat(StreamFormat_H264)
    , mCustomizedContentFormat(0)
    , mbDecoderContentSetup(0)
    , mbWaitKeyFrame(1)
    , mbDecoderRunning(0)
    , mbDecoderStarted(0)
    , mbDecoderSuspended(0)
    , mbDecoderPaused(0)
    , mbBackward(0)
    , mScanMode(0)
    , mbNeedSendSyncPoint(0)
    , mbNewSequence(0)
    , mnCurrentInputPinNumber(0)
    , mpCurInputPin(NULL)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpBuffer(NULL)
    , mCurVideoWidth(0)
    , mCurVideoHeight(0)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate((float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen)
{
    TUint i = 0;
    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }
}

EECode CVideoDecoderV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    return inherited::Construct();
}

CVideoDecoderV2::~CVideoDecoderV2()
{

}

void CVideoDecoderV2::Delete()
{
    TUint i = 0;

    if (mpDecoder) {
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;
    }

    for (i = 0; i < EConstVideoDecoderMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    inherited::Delete();
}

EECode CVideoDecoderV2::Initialize(TChar *p_param)
{
    EVideoDecoderModuleID id;
    id = _guess_video_decoder_type_from_string(p_param);

    //hard code here
    if (mpPersistMediaConfig->pb_decode.prefer_official_reference_model_decoder) {
        id = EVideoDecoderModuleID_HEVC_HM;
        LOGM_NOTICE("[debug]: choose hm decoder for h265\n");
    }

    LOGM_INFO("Initialize() start, id %d\n", id);

    if (mbDecoderContentSetup) {

        LOGM_INFO("Initialize() start, there's a decoder already\n");

        DASSERT(mpDecoder);
        if (!mpDecoder) {
            LOGM_FATAL("[Internal bug], why the mpDecoder is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        if (mbDecoderContentSetup) {
            LOGM_INFO("before mpDecoder->DestroyContext()\n");
            mpDecoder->DestroyContext();
            mbDecoderContentSetup = 0;
        }

        if (id != mCurDecoderID) {
            LOGM_INFO("before mpDecoder->Delete(), cur id %d, request id %d\n", mCurDecoderID, id);
            mpDecoder->GetObject0()->Delete();
            mpDecoder = NULL;
        }
    }

    if (!mpDecoder) {
        mpDecoder = gfModuleFactoryVideoDecoder(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpDecoder) {
            mCurDecoderID = id;
            mCurDecoderMode = mpDecoder->GetDecoderMode();
        } else {
            mCurDecoderID = EVideoDecoderModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoDecoder(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    if (EDecoderMode_Normal == mCurDecoderMode) {
        mpDecoder->SetBufferPool(mpBufferPool);
    } else {
        mpDecoder->SetBufferPoolDirect(mpOutputPin, mpBufferPool);
    }

    LOGM_INFO("Initialize() done\n");

    return EECode_OK;
}

EECode CVideoDecoderV2::Finalize()
{
    if (mpDecoder) {
        if (mbDecoderRunning) {
            mbDecoderRunning = 0;
            LOGM_INFO("before mpDecoder->Stop()\n");
            mpDecoder->Stop();
        }

        if (mbDecoderContentSetup) {
            LOGM_INFO("before mpDecoder->DestroyContext()\n");
            mpDecoder->DestroyContext();
            mbDecoderContentSetup = 0;
        }

        LOGM_INFO("before mpDecoder->Destroy(), cur id %d\n", mCurDecoderID);
        mpDecoder->GetObject0()->Delete();
        mpDecoder = NULL;

        if (mpBufferPool) {
            gfClearBufferMemory((IBufferPool *) mpBufferPool);
        }
    }

    return EECode_OK;
}

EECode CVideoDecoderV2::Run()
{
    mbDecoderRunning = 1;
    inherited::Run();

    return EECode_OK;
}

EECode CVideoDecoderV2::Start()
{
    EECode err;

    if (mpBufferPool) {
        mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
    }

    if (mpDecoder) {
        mpDecoder->Start();
        mbDecoderStarted = 1;
    } else {
        LOGM_FATAL("NULL mpDecoder.\n");
    }

    err = mpWorkQ->SendCmd(ECMDType_Start);

    return err;
}

EECode CVideoDecoderV2::Stop()
{
    mbDecoderStarted = 0;

    if (mpBufferPool) {
        mpBufferPool->AddBufferNotifyListener(NULL);
    }

    if (mpDecoder) {
        mpDecoder->Stop();
    } else {
        LOGM_FATAL("NULL mpDecoder.\n");
    }

    inherited::Stop();

    return EECode_OK;
}

EECode CVideoDecoderV2::SwitchInput(TComponentIndex focus_input_index)
{
    EECode err;
    SCMD cmd;

    cmd.code = ECMDType_SwitchInput;
    cmd.flag = focus_input_index;
    err = mpWorkQ->SendCmd(cmd);
    return err;
}

EECode CVideoDecoderV2::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoDecoderV2::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin) {
            LOGM_ERROR("there's already a output pin in CVideoDecoderV2\n");
            return EECode_InternalLogicalBug;
        }

        mpOutputPin = COutputPin::Create("[Video output pin for CVideoDecoderV2 filter]", (IFilter *) this);

        if (!mpOutputPin)  {
            LOGM_FATAL("COutputPin::Create() fail?\n");
            return EECode_NoMemory;
        }

        TU32 buffer_number = 8;
        if (mpPersistMediaConfig->pb_decode.prealloc_buffer_number) {
            buffer_number = mpPersistMediaConfig->pb_decode.prealloc_buffer_number;
        }
        LOG_NOTICE("[config]: video decoder buffer number %d\n", buffer_number);
        mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoDecoderV2 filter]", buffer_number);
        if (!mpBufferPool)  {
            LOGM_FATAL("CBufferPool::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpOutputPin->SetBufferPool(mpBufferPool);

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

EECode CVideoDecoderV2::AddInputPin(TUint &index, TUint type)
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

        mpInputPins[mnCurrentInputPinNumber] = CQueueInputPin::Create("[Video input pin for CVideoDecoderV2]", (IFilter *) this, (StreamType) type, mpWorkQ->MsgQ());
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

IOutputPin *CVideoDecoderV2::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CVideoDecoderV2::GetInputPin(TUint index)
{
    if (index < mnCurrentInputPinNumber) {
        return mpInputPins[index];
    }

    LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
    return NULL;
}

EECode CVideoDecoderV2::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
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

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CVideoDecoderV2::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnCurrentInputPinNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CVideoDecoderV2::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, heart beat %d, mnCurrentInputPinNumber %d\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat, mnCurrentInputPinNumber);

    for (i = 0; i < mnCurrentInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            if (mpCurInputPin == mpInputPins[i]) {
                LOGM_PRINTF("\t\t!!current input pin:\n");
            }
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;

    return;
}

void CVideoDecoderV2::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    IBufferPool *pBufferPool = NULL;
    EECode err = EECode_OK;
    CIBuffer *p_out = NULL;
    //TU32 in_cnt = 0;
    //TU32 out_cnt = 0;
    //TTime before_dec;
    //TTime after_dec;

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
        LOGM_STATE("CVideoDecoderV2: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
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
                            msState = EModuleState_HasInputData;
                        }
                    }
                }
                break;

            case EModuleState_HasOutputBuffer:
                //DASSERT(pBufferPool->GetFreeBufferCnt() > 0);
                //DASSERT(mpCurInputPin);
                type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpCurInputPin->GetBufferQ());
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    DASSERT(!mpBuffer);
                    if (mpCurInputPin->PeekBuffer(mpBuffer)) {
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

                if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
                    mCurVideoWidth = mpBuffer->mVideoWidth;
                    mCurVideoHeight = mpBuffer->mVideoWidth;
                    mVideoFramerateNum = mpBuffer->mVideoFrameRateNum;
                    mVideoFramerateDen = mpBuffer->mVideoFrameRateDen;
                    mVideoFramerate = mpBuffer->mVideoRoughFrameRate;
                    mCurrentFormat = (StreamFormat) mpBuffer->mContentFormat;
                    mCustomizedContentFormat = mpBuffer->mCustomizedContentFormat;
                    mbNeedSendSyncPoint = 1;
                    if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::NEW_SEQUENCE)) {
                        mbNewSequence = 1;
                    } else {
                        mbNewSequence = 0;
                    }
                }

                if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
                    msState = EModuleState_Completed;
                    LOGM_NOTICE("bit-stream comes to end, sending EOS to down filter...\n");
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
                        LOGM_NOTICE("broken idr frame detected, flag 0x%x\n", mpBuffer->GetBufferFlags());
                        if (mpDecoder) {
                            //mpDecoder->Flush();
                        }
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
                        LOGM_NOTICE("wait first key frame done\n");
                    } else {
                        LOGM_NOTICE("wait first key frame ... mpBuffer->GetBufferFlags() 0x%08x, data size %ld\n", mpBuffer->GetBufferFlags(), mpBuffer->GetDataSize());
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = EModuleState_Idle;
                        break;
                    }
                }

                if (DUnlikely(!mbDecoderContentSetup)) {
                    SVideoDecoderInputParam input_param;
                    memset(&input_param, 0x0, sizeof(input_param));
                    input_param.format = mCurrentFormat;
                    input_param.cuustomized_content_format = mCustomizedContentFormat;
                    input_param.cap_max_width = mCurVideoWidth;
                    input_param.cap_max_height = mCurVideoHeight;
                    LOGM_NOTICE("decoder: video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);
                    err = mpDecoder->SetupContext(&input_param);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_ERROR("SetupContext(), %d, %s fail\n", err, gfGetErrorCodeString(err));
                        msState = EModuleState_Error;
                        break;
                    }

                    mbDecoderContentSetup = 1;
                }

                if (DLikely(EDecoderMode_Normal == mCurDecoderMode)) {
                    //before_dec = mpClockReference->GetCurrentTime();
                    err = mpDecoder->Decode(mpBuffer, p_out);
                    //in_cnt ++;
                    if (DUnlikely(EECode_OK != err)) {
                        if (mpPersistMediaConfig->app_start_exit) {
                            LOGM_NOTICE("[program start exit]: Decode return err=%d, %s\n", err, gfGetErrorCodeString(err));
                            msState = EModuleState_Completed;
                        } else {
                            LOGM_ERROR("Decode Fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            msState = EModuleState_Error;
                        }
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        mpCurInputPin->Purge(1);
                        if (p_out) {
                            p_out->Release();
                            p_out = NULL;
                        }
                        break;
                    } else {
                        if (p_out) {
                            if (DUnlikely(mbNeedSendSyncPoint)) {
                                p_out->mVideoFrameRateNum = mVideoFramerateNum;
                                p_out->mVideoFrameRateDen = mVideoFramerateDen;
                                p_out->mVideoRoughFrameRate = mVideoFramerate;
                                if (mbNewSequence) {
                                    p_out->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::NEW_SEQUENCE);
                                    mbNewSequence = 0;
                                } else {
                                    p_out->SetBufferFlags(CIBuffer::SYNC_POINT);
                                }
                                mbNeedSendSyncPoint = 0;
                            } else {
                                p_out->SetBufferFlags(0);
                            }
                            //out_cnt ++;
                            //after_dec = mpClockReference->GetCurrentTime();
                            mpOutputPin->SendBuffer(p_out);
#ifdef DCONFIG_TEST_END2END_DELAY
                            p_out->mDebugTime = mpBuffer->mDebugTime;
                            p_out->mDebugCount = mpBuffer->mDebugCount;
#endif
                            //LOGM_PRINTF("in %d, out %d, time gap %lld, time %lld\n", in_cnt, out_cnt, after_dec - before_dec, after_dec);
                        }
                        msState = EModuleState_Idle;
                    }
                } else {
                    err = mpDecoder->DecodeDirect(mpBuffer);
                    msState = EModuleState_Idle;
                }
                mpBuffer->Release();
                mpBuffer = NULL;
                break;

            default:
                LOGM_ERROR("CVideoDecoderV2:: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }

    mpBufferPool->AddBufferNotifyListener(NULL);
}

void CVideoDecoderV2::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);

    if (mbDecoderStarted) {
        switch (type) {
            case EEventType_BufferReleaseNotify:
                mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
                break;
            default:
                LOGM_ERROR("event type unsupported: %d\n", (TInt)type);
        }
    }
}

void CVideoDecoderV2::processCmd(SCMD &cmd)
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
            msState = EModuleState_Idle;
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
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            msState = EModuleState_Idle;
            mbWaitKeyFrame = 1;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Suspend: {
                msState = EModuleState_Pending;
                mbWaitKeyFrame = 1;
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
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_ResumeSuspend: {
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
                } else {
                    LOGM_ERROR("NULL p_input\n");
                    mpWorkQ->CmdAck(EECode_Error);
                    break;
                }

                if (DLikely(mpDecoder)) {
                    if (!mbDecoderContentSetup) {
                        SVideoDecoderInputParam decoder_param;
                        memset(&decoder_param, 0x0, sizeof(decoder_param));
                        decoder_param.format = mCurrentFormat;
                        decoder_param.cuustomized_content_format = mCustomizedContentFormat;
                        decoder_param.cap_max_width = mCurVideoWidth;
                        decoder_param.cap_max_height = mCurVideoHeight;
                        LOGM_NOTICE("decoder: video resolution %dx%d\n", mCurVideoWidth, mCurVideoHeight);
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
                    }
                } else {
                    LOGM_INFO("ECMDType_SwitchInput, focus_input_index %u not changed, do nothing.\n", cmd.flag);
                }
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
            if (msState == EModuleState_WaitCmd) {
                msState = EModuleState_Idle;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_NotifySourceFilterBlocked:
            LOGM_INFO("processCmd, got ECMDType_NotifySourceFilterBlocked\n");
            break;

        case ECMDType_NotifyBufferRelease:
            if (mpOutputPin) {
                if (mpOutputPin->GetBufferPool()->GetFreeBufferCnt() > 0) {
                    if (msState == EModuleState_Idle) {
                        msState = EModuleState_HasOutputBuffer;
                    } else if (msState == EModuleState_HasInputData) {
                        msState = EModuleState_Running;
                    }
                }
            }
            break;

        case ECMDType_UpdatePlaybackSpeed: {
                if (mpDecoder) {
                    mpDecoder->SetPbRule((TU8)cmd.res32_1, (TU8)cmd.res64_1, (TU16)cmd.res64_2);
                }
            }
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

EECode CVideoDecoderV2::flushInputData()
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

