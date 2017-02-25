/*******************************************************************************
 * audio_encoder.cpp
 *
 * History:
 *    2014/01/07 - [Zhi He] create file
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

#include "audio_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static EECode __release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
            IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
            DASSERT(pool);
            if (pool) {
                pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IFilter *gfCreateAudioEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CAudioEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EAudioEncoderModuleID _guess_audio_encoder_type_from_string(TChar *string)
{
    if (!string) {
        LOG_NOTICE("NULL input in _guess_audio_decoder_type_from_string, choose default\n");
        return EAudioEncoderModuleID_FAAC;
    }

    if (!strncmp(string, DNameTag_OPTCAac, strlen(DNameTag_OPTCAac))) {
        return EAudioEncoderModuleID_OPTCAAC;
    }

    if (!strncmp(string, DNameTag_FAAC, strlen(DNameTag_FAAC))) {
        return EAudioEncoderModuleID_FAAC;
    }

    if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FAAC))) {
        return EAudioEncoderModuleID_FFMpeg;
    }

    if (!strncmp(string, DNameTag_FAAC, strlen(DNameTag_FAAC))) {
#ifdef BUILD_MODULE_LIBAAC
        return EAudioEncoderModuleID_PrivateLibAAC;
#endif
    }

    LOG_WARN("unknown string tag(%s) in _guess_audio_encoder_type_from_string, choose default\n", string);
    return EAudioEncoderModuleID_OPTCAAC;
}

//-----------------------------------------------------------------------
//
// CAudioEncoder
//
//-----------------------------------------------------------------------
IFilter *CAudioEncoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CAudioEncoder *result = new CAudioEncoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CAudioEncoder::CAudioEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpEncoder(NULL)
    , mCurEncoderID(EAudioEncoderModuleID_Auto)
    , mbEncoderContentSetup(0)
    , mbEncoderStarted(0)
    , mbSendExtradata(0)
    , mnCurrentInputPinNumber(0)
    , mpInputPin(NULL)
    , mnCurrentOutputPinNumber(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mpBuffer(NULL)
    , mpOutputBuffer(NULL)
    , mAudioCodecFormat(StreamFormat_Invalid)
    , mAudioSampleFormat(AudioSampleFMT_FLT)
    , mAudioSampleRate(DDefaultAudioSampleRate)
    , mAudioChannelNumber(DDefaultAudioChannelNumber)
    , mAudioFrameSize(1024)
    , mAudioBitrate(128000)
{
}

EECode CAudioEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioEncoderFilter);

    //mConfigLogLevel = ELogLevel_Info;
    //mConfigLogOption = 0xff;
    if (mpPersistMediaConfig->audio_encoding_config.bitrate) {
        mAudioBitrate = mpPersistMediaConfig->audio_encoding_config.bitrate;
    }

    return inherited::Construct();
}

CAudioEncoder::~CAudioEncoder()
{
    if (mpEncoder) {
        mpEncoder->Destroy();
        mpEncoder = NULL;
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
}

void CAudioEncoder::Delete()
{
    if (mpEncoder) {
        mpEncoder->Destroy();
        mpEncoder = NULL;
    }

    if (mpInputPin) {
        mpInputPin->Delete();
        mpInputPin = NULL;
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

EECode CAudioEncoder::Initialize(TChar *p_param)
{
    EAudioEncoderModuleID id;
    id = _guess_audio_encoder_type_from_string(p_param);

    LOGM_INFO("[CAudioEncoder flow]: Initialize() start, id %d\n", id);

    if (mbEncoderContentSetup) {

        LOGM_INFO("[CAudioEncoder flow]: Initialize() start, there's a encoder already\n");

        DASSERT(mpEncoder);
        if (!mpEncoder) {
            LOGM_FATAL("[Internal bug], why the mpEncoder is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        if (mbEncoderStarted) {
            //TODO
            mbEncoderStarted = 0;
        }

        mpEncoder->DestroyContext();
        mbEncoderContentSetup = 0;

        if (id != mCurEncoderID) {
            LOGM_INFO("[CAudioEncoder flow], before mpEncoder->Delete(), cur id %d, request id %d\n", mCurEncoderID, id);
            mpEncoder->Destroy();
            mpEncoder = NULL;
        }
    }

    if (!mpEncoder) {
        LOGM_INFO("[CAudioEncoder flow]: Initialize() start, before gfModuleFactoryAudioEncoder(%d)\n", id);
        mpEncoder = gfModuleFactoryAudioEncoder(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpEncoder) {
            mCurEncoderID = id;
            switch (mCurEncoderID) {

                case EAudioEncoderModuleID_OPTCAAC:
                    mAudioCodecFormat = StreamFormat_AAC;
                    break;

                case EAudioEncoderModuleID_FAAC:
                    mAudioCodecFormat = StreamFormat_AAC;
                    break;

                case EAudioEncoderModuleID_FFMpeg:
                    mAudioCodecFormat = StreamFormat_AAC;
                    break;

                case EAudioEncoderModuleID_PrivateLibAAC:
                    mAudioCodecFormat = StreamFormat_AAC;
                    break;

                case EAudioEncoderModuleID_CustomizedADPCM:
                    mAudioCodecFormat = StreamFormat_CustomizedADPCM_1;
                    break;

                default:
                    LOGM_ERROR("CAudioEncoder::Initialize: not expected mCurEncoderID %d\n", mCurEncoderID);
                    return EECode_InternalLogicalBug;
                    break;
            }
        } else {
            mCurEncoderID = EAudioEncoderModuleID_Invalid;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryAudioEncoder(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }

        //setup content
    }

    LOGM_INFO("[CAudioEncoder flow]: Initialize() done\n");
    return EECode_OK;
}

EECode CAudioEncoder::Finalize()
{
    return EECode_OK;
}

EECode CAudioEncoder::Run()
{
    //debug assert
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPin);
    DASSERT(!mbEncoderStarted);

    //mbEncoderRunning = 1;

    inherited::Run();

    return EECode_OK;
}

EECode CAudioEncoder::Start()
{
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPin);
    DASSERT(!mbEncoderStarted);

    mpWorkQ->SendCmd(ECMDType_Start);
    mbEncoderStarted = 1;

    return EECode_OK;
}

EECode CAudioEncoder::Stop()
{
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);
    DASSERT(mpInputPin);
    //DASSERT(mbEncoderStarted);

    inherited::Stop();

    return EECode_OK;
}

void CAudioEncoder::Pause()
{
}

void CAudioEncoder::Resume()
{
}

EECode CAudioEncoder::SwitchInput(TComponentIndex focus_input_index)
{
    return EECode_NoImplement;
}

EECode CAudioEncoder::FlowControl(EBufferType flowcontrol_type)
{
    return EECode_NoImplement;
}

EECode CAudioEncoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Audio == type);

    if (StreamType_Audio == type) {

        if (!mpOutputPin) {
            DASSERT(!mpBufferPool);
            DASSERT(!mpMemPool);

            mpOutputPin = COutputPin::Create("[Audio output pin for CAudioEncoder filter]", (IFilter *) this);
            if (!mpOutputPin) {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpBufferPool = CBufferPool::Create("[Buffer pool for audio output pin in CAudioEncoder filter]", DDefaultAudioBufferNumber);
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpMemPool = CRingMemPool::Create(512 * 1024);
            if (!mpMemPool)  {
                LOGM_FATAL("mpMemPool::Create() fail?\n");
                return EECode_NoMemory;
            }
            mpOutputPin->SetBufferPool(mpBufferPool);
            mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
            mpBufferPool->SetReleaseBufferCallBack(__release_ring_buffer);
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

EECode CAudioEncoder::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Audio == type) {
        if (mpInputPin) {
            LOGM_ERROR("there's already a input pin in CAudioEncoder\n");
            return EECode_InternalLogicalBug;
        }

        mpInputPin = CQueueInputPin::Create((const TChar *)"[Audio input pin for CAudioEncoder]", (IFilter *) this, StreamType_Audio, mpWorkQ->MsgQ());
        if (!mpInputPin) {
            LOGM_FATAL("CQueueInputPin::Create() fail?\n");
            return EECode_NoMemory;
        }
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IOutputPin *CAudioEncoder::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CAudioEncoder::GetInputPin(TUint index)
{
    return mpInputPin;
}

EECode CAudioEncoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
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

void CAudioEncoder::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 1;

    info.pName = mpModuleName;
    return;
}

void CAudioEncoder::PrintStatus()
{
    LOGM_ALWAYS("msState=%d, %s, mDebugHeartBeat %d\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat);

    if (mpInputPin) {
        LOGM_ALWAYS("       mpInputPin[%p]'s status:\n", mpInputPin);
        mpInputPin->PrintStatus();
    }

    if (mpOutputPin) {
        LOGM_ALWAYS("       mpOutputPin[%p]'s status:\n", mpOutputPin);
        mpOutputPin->PrintStatus();
    }

    return;
}

void CAudioEncoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_FLOW("EventNotify, event type %d.\n", (TInt)type);

    switch (type) {
        case EEventType_BufferReleaseNotify:
            mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
            break;
        default:
            LOGM_ERROR("event type unsupported:  %d", (TInt)type);
    }
    return;
}

void CAudioEncoder::OnRun(void)
{
    mbRun = 1;
    msState = EModuleState_WaitCmd;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_NOTICE("OnRun loop, start\n");

    DASSERT(mpInputPin);
    DASSERT(mpOutputPin);
    DASSERT(mpEncoder);

    SCMD cmd;
    CIQueue::QType type;
    TU32 output_size = 0;
    EECode err = EECode_OK;
    TU8 *ptr = NULL;
    TTime native_pts = 0;
    TTime linear_pts = 0;

    while (mbRun) {

        LOGM_STATE("CAudioEncoder OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));
        mDebugHeartBeat ++;

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

                if (mpBufferPool->GetFreeBufferCnt() > 0) {
                    msState = EModuleState_HasOutputBuffer;
                } else {
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpInputPin->GetBufferQ());
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        DASSERT(!mpBuffer);
                        if (mpInputPin->PeekBuffer(mpBuffer)) {
                            msState = EModuleState_HasInputData;
                        }
                    }
                }
                break;

            case EModuleState_HasOutputBuffer:
                DASSERT(mpBufferPool->GetFreeBufferCnt() > 0);
                type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpInputPin->GetBufferQ());
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    DASSERT(!mpBuffer);
                    if (mpInputPin->PeekBuffer(mpBuffer)) {
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
                //DASSERT(mpBuffer);
                if (mpBufferPool->GetFreeBufferCnt() > 0) {
                    msState = EModuleState_Running;
                } else {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                }
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer);
                DASSERT(!mpOutputBuffer);

                if (EBufferType_AudioPCMBuffer == mpBuffer->GetBufferType()) {
                    if (DUnlikely(!mbEncoderContentSetup)) {
                        DASSERT(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT);
                        if (mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT) {

                            mAudioSampleFormat = (AudioSampleFMT) mpBuffer->mAudioSampleFormat;
                            mAudioSampleRate = mpBuffer->mAudioSampleRate;
                            mAudioChannelNumber = mpBuffer->mAudioChannelNumber;
                            mAudioFrameSize = mpBuffer->mAudioFrameSize;

                            SAudioParams params;
                            memset(&params, 0x0, sizeof(params));
                            params.codec_format = mAudioCodecFormat;
                            params.sample_rate = mAudioSampleRate;
                            params.sample_format = mAudioSampleFormat;
                            params.channel_number = mAudioChannelNumber;
                            params.bitrate = mAudioBitrate;
                            params.frame_size = mAudioFrameSize;
                            LOGM_CONFIGURATION("[AudioEncoder receive sync point buffer]: params.codec_format %d, params.sample_rate %d, params.channel_number %d, params.bitrate %d, params.frame_size %d\n", params.codec_format, params.sample_rate, params.channel_number, params.bitrate, params.frame_size);

                            err = mpEncoder->SetupContext(&params);
                            if (DUnlikely(EECode_OK != err)) {
                                msState = EModuleState_Error;
                                LOGM_ERROR("SetupContext fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                                break;
                            }
                            mbEncoderContentSetup = 1;
                        }
                    }
                } else if (DUnlikely(EBufferType_FlowControl_EOS == mpBuffer->GetBufferType())) {
                    msState = EModuleState_Pending;
                    LOGM_INFO("bit-stream comes to end, sending EOS to down filter...\n");
                    mpOutputPin->SendBuffer(&mPersistEOSBuffer);
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    break;
                } else if (DUnlikely(EBufferType_FlowControl_PassParameters == mpBuffer->GetBufferType())) {
                    mAudioSampleFormat = (AudioSampleFMT) mpBuffer->mAudioSampleFormat;
                    mAudioSampleRate = mpBuffer->mAudioSampleRate;
                    mAudioChannelNumber = mpBuffer->mAudioChannelNumber;
                    mAudioFrameSize = mpBuffer->mAudioFrameSize;
                    mpBuffer->Release();
                    mpBuffer = NULL;

                    TU32 exsize = 0;
                    TU8 *pex = gfGenerateAACExtraData(mAudioSampleRate, mAudioChannelNumber, exsize);
                    if (DLikely(9 > exsize)) {
                        memcpy(mpPreSendAudioExtradata, pex, exsize);
                    }
                    CIBuffer *p_outbuffer = NULL;
                    mpOutputPin->AllocBuffer(p_outbuffer);
                    if (DUnlikely(!p_outbuffer)) {
                        LOGM_FATAL("failed to alloc output buffer!\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    p_outbuffer->SetBufferType(EBufferType_AudioExtraData);
                    p_outbuffer->SetBufferFlags(CIBuffer::SYNC_POINT);
                    p_outbuffer->mContentFormat = mAudioCodecFormat;
                    p_outbuffer->mAudioSampleRate = mAudioSampleRate;
                    p_outbuffer->mAudioSampleFormat = mAudioSampleFormat;
                    p_outbuffer->mAudioChannelNumber = mAudioChannelNumber;
                    p_outbuffer->mAudioBitrate = mAudioBitrate;
                    p_outbuffer->mAudioFrameSize = mAudioFrameSize;

                    p_outbuffer->SetDataPtrBase(NULL);
                    p_outbuffer->SetDataPtr(mpPreSendAudioExtradata);
                    p_outbuffer->SetDataMemorySize(0);
                    p_outbuffer->SetDataSize((TMemSize)exsize);
                    p_outbuffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
                    p_outbuffer->mpCustomizedContent = NULL;
                    //LOGM_NOTICE("audio encoder, send sync buffer\n");
                    mpOutputPin->SendBuffer(p_outbuffer);
                    p_outbuffer = NULL;
                    DDBG_FREE(pex, "ACEX");
                    msState = EModuleState_Idle;
                    break;
                }

                ptr = mpMemPool->RequestMemBlock(MAX_AUDIO_BUFFER_SIZE);
                if (DUnlikely(!ptr)) {
                    LOGM_FATAL("failed to alloc memory for output buffer!\n");
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    msState = EModuleState_Error;
                    break;
                }

                output_size = MAX_AUDIO_BUFFER_SIZE;
                err = mpEncoder->Encode(mpBuffer->GetDataPtr(), ptr, output_size);
                native_pts = mpBuffer->GetBufferNativePTS();
                linear_pts = mpBuffer->GetBufferLinearPTS();

                mpBuffer->Release();
                mpBuffer = NULL;
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("OnRun: Encode Fail.\n");
                    msState = EModuleState_Error;
                    mpMemPool->ReturnBackMemBlock(MAX_AUDIO_BUFFER_SIZE, ptr);
                    break;
                } else if (DUnlikely(!mbSendExtradata)) {
                    mbSendExtradata = 1;
                    TU8 *p;
                    TU32 size;
                    err = mpEncoder->GetExtraData(p, size);
                    if (DLikely(EECode_OK == err)) {
                        mpOutputPin->AllocBuffer(mpOutputBuffer);
                        if (DUnlikely(!mpOutputBuffer)) {
                            LOGM_FATAL("failed to alloc output buffer!\n");
                            msState = EModuleState_Error;
                            break;
                        }
                        mpOutputBuffer->SetBufferType(EBufferType_AudioExtraData);
                        mpOutputBuffer->SetBufferFlags(CIBuffer::SYNC_POINT);
                        mpOutputBuffer->mContentFormat = mAudioCodecFormat;
                        mpOutputBuffer->mAudioSampleRate = mAudioSampleRate;
                        mpOutputBuffer->mAudioSampleFormat = mAudioSampleFormat;
                        mpOutputBuffer->mAudioChannelNumber = mAudioChannelNumber;
                        mpOutputBuffer->mAudioBitrate = mAudioBitrate;
                        mpOutputBuffer->mAudioFrameSize = mAudioFrameSize;

                        mpOutputBuffer->SetDataPtrBase(NULL);
                        mpOutputBuffer->SetDataPtr(p);
                        mpOutputBuffer->SetDataMemorySize(0);
                        mpOutputBuffer->SetDataSize((TMemSize)size);
                        mpOutputBuffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
                        mpOutputBuffer->mpCustomizedContent = NULL;
                        mpOutputBuffer->SetBufferNativePTS(native_pts);
                        mpOutputBuffer->SetBufferLinearPTS(linear_pts);
                        mpOutputPin->SendBuffer(mpOutputBuffer);
                        mpOutputBuffer = NULL;
                    }
                }

                if (DUnlikely(!output_size)) {
                    mpMemPool->ReturnBackMemBlock(MAX_AUDIO_BUFFER_SIZE, ptr);
                } else {
                    if (output_size < MAX_AUDIO_BUFFER_SIZE) {
                        mpMemPool->ReturnBackMemBlock(MAX_AUDIO_BUFFER_SIZE - output_size, ptr + output_size);
                    } else if (output_size > MAX_AUDIO_BUFFER_SIZE) {
                        mpMemPool->ReturnBackMemBlock(MAX_AUDIO_BUFFER_SIZE, ptr);
                        LOGM_FATAL("MAX_AUDIO_BUFFER_SIZE %ld too small??? output_size %d\n", (TULong)MAX_AUDIO_BUFFER_SIZE, output_size);
                        msState = EModuleState_Error;
                        break;
                    }
                    mpOutputPin->AllocBuffer(mpOutputBuffer);
                    if (DUnlikely(!mpOutputBuffer)) {
                        LOGM_FATAL("failed to alloc output buffer!\n");
                        msState = EModuleState_Error;
                        break;
                    }

                    mpOutputBuffer->SetBufferType(EBufferType_AudioES);
                    mpOutputBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                    mpOutputBuffer->mContentFormat = mAudioCodecFormat;
                    mpOutputBuffer->mAudioSampleRate = mAudioSampleRate;
                    mpOutputBuffer->mAudioChannelNumber = mAudioChannelNumber;
                    mpOutputBuffer->mAudioSampleFormat = mAudioSampleFormat;
                    mpOutputBuffer->mAudioBitrate = mAudioBitrate;
                    mpOutputBuffer->mAudioFrameSize = mAudioFrameSize;

                    mpOutputBuffer->SetDataPtrBase(ptr);
                    mpOutputBuffer->SetDataPtr(ptr);
                    mpOutputBuffer->SetDataMemorySize((TMemSize)output_size);
                    mpOutputBuffer->SetDataSize((TMemSize)output_size);
                    mpOutputBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    mpOutputBuffer->mpCustomizedContent = mpMemPool;
                    mpOutputBuffer->SetBufferNativePTS(native_pts);
                    mpOutputBuffer->SetBufferLinearPTS(linear_pts);
                    mpOutputPin->SendBuffer(mpOutputBuffer);
                    mpOutputBuffer = NULL;
                }

                msState = EModuleState_Idle;
                break;

            default:
                LOGM_ERROR("CAudioEncoder: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
    }
    LOGM_PRINTF("CAudioEncoder::OnRun() end!\n");
}

void CAudioEncoder::processCmd(SCMD &cmd)
{
    LOGM_FLOW("processCmd, cmd.code %d, %s, state %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code), msState, gfGetModuleStateString(msState));

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_ExitRunning.\n");
            break;

        case ECMDType_Stop:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Stop.\n");
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Idle;
            }
            mpWorkQ->CmdAck(EECode_OK);
            LOGM_INFO("processCmd, ECMDType_Start.\n");
            break;

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
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpInputPin) {
                LOGM_INFO("ECMDType_Flush, before mpCurInputPin->Purge()\n");
                mpInputPin->Purge();
                LOGM_INFO("ECMDType_Flush, after mpCurInputPin->Purge()\n");
            }
            msState = EModuleState_Pending;
            mpWorkQ->CmdAck(EECode_OK);
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
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Suspend: {
                msState = EModuleState_Pending;
                if (mpBuffer) {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }

                if (mpEncoder) {
                    if (mbEncoderStarted) {
                        mbEncoderStarted = 0;
                    }

                    /*if (mbEncoderContentSetup) {
                        mpEncoder->DestroyContext();
                        mbEncoderContentSetup = 0;
                    }*///TODO, will cause OnRun segment fault
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_ResumeSuspend: {
                msState = EModuleState_Idle;

                if (mpEncoder) {
                    if (!mbEncoderStarted) {
                        mbEncoderStarted = 1;
                    }
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_NotifySynced:
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_NotifySourceFilterBlocked:
            break;

        case ECMDType_NotifyBufferRelease:
            if (mpOutputPin->GetBufferPool()->GetFreeBufferCnt() > 0) {
                if (msState == EModuleState_Idle)
                { msState = EModuleState_HasOutputBuffer; }
                else if (msState == EModuleState_HasInputData)
                { msState = EModuleState_Running; }
            }
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
