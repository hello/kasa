/*******************************************************************************
 * video_encoder.cpp
 *
 * History:
 *    2013/11/07 - [Zhi He] create file
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

#include "video_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateVideoEncoderFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoEncoderModuleID _guess_video_encoder_type_from_string(TChar *string)
{
    if (!string) {
        LOG_NOTICE("NULL input in _guess_video_encoder_type_from_string, choose default\n");
        return EVideoEncoderModuleID_AMBADSP;
    }

    if (!strncmp(string, DNameTag_AMBA, strlen(DNameTag_AMBA))) {
        return EVideoEncoderModuleID_AMBADSP;
    } else if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
        return EVideoEncoderModuleID_FFMpeg;
    } else if (!strncmp(string, DNameTag_X264, strlen(DNameTag_X264))) {
        return EVideoEncoderModuleID_X264;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_encoder_type_from_string, choose default\n", string);
    return EVideoEncoderModuleID_AMBADSP;
}

//-----------------------------------------------------------------------
//
// CVideoEncoder
//
//-----------------------------------------------------------------------

IFilter *CVideoEncoder::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CVideoEncoder *result = new CVideoEncoder(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CVideoEncoder::CVideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpEncoder(NULL)
    , mCurEncoderID(EVideoEncoderModuleID_Auto)
    , mpEncoderParams(NULL)
    , mbEncoderContentSetup(0)
    , mbWaitKeyFrame(1)
    , mbEncoderRunning(0)
    , mbEncoderStarted(0)
    , mnCurrentOutputPinNumber(0)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpBuffer(NULL)
{
    DASSERT(mpPersistMediaConfig);
    if (mpPersistMediaConfig) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
    }
    DASSERT(mpSystemClockReference);
}

EECode CVideoEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoDecoderFilter);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
    DASSERT(!mpEncoderParams);
    mpEncoderParams = (SVideoEncoderParam *) DDBG_MALLOC(sizeof(SVideoEncoderParam), "VEPR");
    if (DLikely(mpEncoderParams)) {
        memset(mpEncoderParams, 0, sizeof(SVideoEncoderParam));
        mpEncoderParams->id = 0;
        mpEncoderParams->index = mIndex;
        mpEncoderParams->format = StreamFormat_H264;
        mpEncoderParams->pic_width = mpPersistMediaConfig->dsp_config.modeConfigMudec.video_win_width;
        mpEncoderParams->pic_height = mpPersistMediaConfig->dsp_config.modeConfigMudec.video_win_height;
        DASSERT(1280 == mpEncoderParams->pic_width);
        DASSERT(720 == mpEncoderParams->pic_height);
        if (mpPersistMediaConfig->dsp_config.modeConfigMudec.avg_bitrate) {
            mpEncoderParams->bitrate = mpPersistMediaConfig->dsp_config.modeConfigMudec.avg_bitrate;
        } else {
            mpEncoderParams->bitrate = 1 * 1024 * 1024;
        }
        mpEncoderParams->bitrate_pts = 0;
        if ((mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick < (DDefaultVideoFramerateDen + 5)) && (mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick > (DDefaultVideoFramerateDen + 5))) {
            mpEncoderParams->framerate = (float)(DDefaultVideoFramerateNum) / (float)(DDefaultVideoFramerateDen);
            mpEncoderParams->framerate_reduce_factor = 0;
        } else if (mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick) {
            mpEncoderParams->framerate = (float)DDefaultTimeScale / (float)mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick;
            mpEncoderParams->framerate_reduce_factor = 0;
        } else {
            mpEncoderParams->framerate = (float)(DDefaultVideoFramerateNum) / (float)(DDefaultVideoFramerateDen);
            mpEncoderParams->framerate_reduce_factor = 0;
        }
        LOGM_PRINTF("default video transcode bitrate %d, framerate %f, frame tick %d\n", mpEncoderParams->bitrate, mpEncoderParams->framerate, mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick);
        mpEncoderParams->gop_M = 1;
        mpEncoderParams->gop_N = 30;
        mpEncoderParams->gop_idr_interval = 4;
        mpEncoderParams->gop_structure = 0;
        mpEncoderParams->demand_idr_pts = 0;
        mpEncoderParams->demand_idr_strategy = 0;
        LOGM_INFO("Set Default Video Encoder Params:\nindex=%u\nformat=%u\npic_width=%u\npic_height=%u\nbitrate=%u\nframerate=%f\ngop_M=%u\ngop_N=%u\ngop_idr_interval=%u\ndemand_idr_strategy=%u\n"
                  , mpEncoderParams->index
                  , mpEncoderParams->format
                  , mpEncoderParams->pic_width
                  , mpEncoderParams->pic_height
                  , mpEncoderParams->bitrate
                  , mpEncoderParams->framerate
                  , mpEncoderParams->gop_M
                  , mpEncoderParams->gop_N
                  , mpEncoderParams->gop_idr_interval
                  , mpEncoderParams->demand_idr_strategy);
    } else {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    return inherited::Construct();
}

CVideoEncoder::~CVideoEncoder()
{
}

void CVideoEncoder::Delete()
{
    LOGM_RESOURCE("CVideoEncoder::Delete(), before mpEncoder->Delete().\n");
    if (mpEncoder) {
        mpEncoder->Delete();
        mpEncoder = NULL;
    }

    LOGM_RESOURCE("CVideoEncoder::Delete(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    LOGM_RESOURCE("CVideoEncoder::Delete(), before delete buffer pool.\n");
    if (mpBufferPool) {
        mpBufferPool->Delete();
        mpBufferPool = NULL;
    }

    if (mpEncoderParams) {
        DDBG_FREE(mpEncoderParams, "VEPR");
        mpEncoderParams = NULL;
    }

    LOGM_RESOURCE("CVideoEncoder::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CVideoEncoder::Initialize(TChar *p_param)
{
    EVideoEncoderModuleID id;
    EECode err = EECode_OK;
    id = _guess_video_encoder_type_from_string(p_param);

    LOGM_INFO("[Video Encoder flow], CVideoEncoder::Initialize() start, id %d\n", id);

    if (mbEncoderContentSetup) {

        LOGM_INFO("[Video Encoder flow], CVideoEncoder::Initialize() start, there's a decoder already\n");

        DASSERT(mpEncoder);
        if (!mpEncoder) {
            LOGM_FATAL("[Internal bug], why the mpEncoder is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        if (mbEncoderRunning) {
            mbEncoderRunning = 0;
            LOGM_INFO("[EncoderFilter flow], before mpEncoder->Stop()\n");
            mpEncoder->Stop();
        }

        LOGM_INFO("[EncoderFilter flow], before mpEncoder->DestroyContext()\n");
        mpEncoder->DestroyContext();
        mbEncoderContentSetup = 0;

        if (id != mCurEncoderID) {
            LOGM_INFO("[EncoderFilter flow], before mpEncoder->Delete(), cur id %d, request id %d\n", mCurEncoderID, id);
            mpEncoder->Delete();
            mpEncoder = NULL;
        }
    }

    if (!mpEncoder) {
        LOGM_INFO("[Video Encoder flow], CVideoEncoder::Initialize() start, before gfModuleFactoryVideoEncoder(%d)\n", id);
        mpEncoder = gfModuleFactoryVideoEncoder(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpEncoder) {
            mCurEncoderID = id;
        } else {
            mCurEncoderID = EVideoEncoderModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoEncoder(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[EncoderFilter flow], before mpEncoder->SetupContext()\n");
    DASSERT(mpEncoderParams);
    SVideoEncoderInputParam param = {0};
    param.index = mpEncoderParams->index;
    param.format = mpEncoderParams->format;
    param.prefer_dsp_mode = EDSPOperationMode_MultipleWindowUDEC;
    param.cap_max_width = mpEncoderParams->pic_width;
    param.cap_max_height = mpEncoderParams->pic_height;
    param.bitrate = mpEncoderParams->bitrate;
    param.platform = EDSPPlatform_AmbaI1;
    param.framerate = mpEncoderParams->framerate;
    err = mpEncoder->SetupContext(&param);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("mpEncoder->SetupContext() failed.\n");
        return err;
    }
    mbEncoderContentSetup = 1;

    LOGM_INFO("[EncoderFilter flow], CVideoEncoder::Initialize() done\n");
    return EECode_OK;
}

EECode CVideoEncoder::Finalize()
{
    if (mpEncoder) {
        if (mbEncoderRunning) {
            mbEncoderRunning = 0;
            LOGM_INFO("[EncoderFilter flow], before mpEncoder->Stop()\n");
            mpEncoder->Stop();
        }

        LOGM_INFO("[EncoderFilter flow], before mpEncoder->DestroyContext()\n");
        mpEncoder->DestroyContext();
        mbEncoderContentSetup = 0;

        LOGM_INFO("[EncoderFilter flow], before mpEncoder->Delete(), cur id %d\n", mCurEncoderID);
        mpEncoder->Delete();
        mpEncoder = NULL;
    }

    return EECode_OK;
}

EECode CVideoEncoder::Run()
{
    //debug assert
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);

    DASSERT(mbEncoderContentSetup);
    DASSERT(!mbEncoderRunning);
    DASSERT(!mbEncoderStarted);

    mbEncoderRunning = 1;
    inherited::Run();

    return EECode_OK;
}

EECode CVideoEncoder::Start()
{
    EECode err;

    //debug assert
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);

    DASSERT(mbEncoderContentSetup);
    DASSERT(mbEncoderRunning);
    DASSERT(!mbEncoderStarted);

    if (mpEncoder) {
        LOGM_INFO("[EncoderFilter flow], CVideoEncoder::Start(), before mpEncoder->Start()\n");
        mpEncoder->Start();
        LOGM_INFO("[EncoderFilter flow], CVideoEncoder::Start(), after mpEncoder->Start()\n");
        mbEncoderStarted = 1;
    } else {
        LOGM_FATAL("NULL mpEncoder in CVideoEncoder::Start().\n");
    }

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Start)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Start);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Start), ret %d\n", err);

    return EECode_OK;
}

EECode CVideoEncoder::Stop()
{
    //debug assert
    DASSERT(mpEncoder);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpOutputPin);

    DASSERT(mbEncoderContentSetup);
    DASSERT(mbEncoderRunning);
    DASSERT(mbEncoderStarted);

    if (mpEncoder) {
        LOGM_INFO("[EncoderFilter flow], CVideoEncoder::Stop(), before mpEncoder->Stop()\n");
        mpEncoder->Stop();
        LOGM_INFO("[EncoderFilter flow], CVideoEncoder::Stop(), after mpEncoder->Stop()\n");
        mbEncoderRunning = 0;
    } else {
        LOGM_FATAL("NULL mpEncoder in CVideoEncoder::Stop().\n");
    }

    inherited::Stop();

    return EECode_OK;
}

EECode CVideoEncoder::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoEncoder::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoEncoder::AddInputPin(TUint &index, TUint type)
{
    LOGM_FATAL("Not support yet\n");

    return EECode_InternalLogicalBug;
}

EECode CVideoEncoder::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    DASSERT(StreamType_Video == type);

    if (StreamType_Video == type) {
        if (mpOutputPin == NULL) {
            mpOutputPin = COutputPin::Create("[Video output pin for CVideoEncoder filter]", (IFilter *) this);

            if (!mpOutputPin)  {
                LOGM_FATAL("COutputPin::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpBufferPool = CBufferPool::Create("[Buffer pool for video output pin in CVideoEncoder filter]", 32);
            if (!mpBufferPool)  {
                LOGM_FATAL("CBufferPool::Create() fail?\n");
                return EECode_NoMemory;
            }

            mpOutputPin->SetBufferPool(mpBufferPool);
            mpBufferPool->AddBufferNotifyListener((IEventListener *) this);
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

IOutputPin *CVideoEncoder::GetOutputPin(TUint index, TUint sub_index)
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

IInputPin *CVideoEncoder::GetInputPin(TUint index)
{
    return NULL;
}

EECode CVideoEncoder::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_video_parameters:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);

            if (p_param) {
                if (set_or_get) {
                    DASSERT(mpEncoder);
                    if (mpEncoder) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *) p_param;
                        mpEncoder->VideoEncoderProperty(property_type, 1, p_param);
                        DASSERT(mpEncoderParams->index == DCOMPONENT_INDEX_FROM_GENERIC_ID(parameters->id));
                        *mpEncoderParams = *parameters;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoder\n");
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *) p_param;
                        *parameters = *mpEncoderParams;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_size:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    LOGM_ERROR("set video size not supported now\n");
                    ret = EECode_NotSupported;
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->pic_width = mpEncoderParams->pic_width;
                        parameters->pic_height = mpEncoderParams->pic_height;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_framerate:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    DASSERT(mpEncoder);
                    if (mpEncoder) {
                        mpEncoder->VideoEncoderProperty(property_type, 1, p_param);
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        DASSERT(mpEncoderParams->index == DCOMPONENT_INDEX_FROM_GENERIC_ID(parameters->id));
                        mpEncoderParams->id = parameters->id;
                        mpEncoderParams->framerate = parameters->framerate;
                        mpEncoderParams->framerate_reduce_factor = parameters->framerate_reduce_factor;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoder\n");
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->framerate = mpEncoderParams->framerate;
                        parameters->framerate_reduce_factor = mpEncoderParams->framerate_reduce_factor;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_format:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    LOGM_ERROR("set video format not supported now\n");
                    ret = EECode_NotSupported;
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->format = mpEncoderParams->format;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_bitrate:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    DASSERT(mpEncoder);
                    if (mpEncoder) {
                        mpEncoder->VideoEncoderProperty(property_type, 1, p_param);
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        DASSERT(mpEncoderParams->index == DCOMPONENT_INDEX_FROM_GENERIC_ID(parameters->id));
                        mpEncoderParams->id = parameters->id;
                        mpEncoderParams->bitrate = parameters->bitrate;
                        mpEncoderParams->bitrate_pts = parameters->bitrate_pts;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoder\n");
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->bitrate = mpEncoderParams->bitrate;
                        parameters->bitrate_pts = mpEncoderParams->bitrate_pts;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_demandIDR:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    DASSERT(mpEncoder);
                    if (mpEncoder) {
                        mpEncoder->VideoEncoderProperty(property_type, 1, p_param);
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        DASSERT(mpEncoderParams->index == DCOMPONENT_INDEX_FROM_GENERIC_ID(parameters->id));
                        mpEncoderParams->id = parameters->id;
                        mpEncoderParams->demand_idr_strategy = parameters->demand_idr_strategy;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoder\n");
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->demand_idr_strategy = mpEncoderParams->demand_idr_strategy;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        case EFilterPropertyType_video_gop:
            DASSERT(p_param);
            DASSERT(mbEncoderContentSetup);
            if (p_param) {
                if (set_or_get) {
                    DASSERT(mpEncoder);
                    if (mpEncoder) {
                        mpEncoder->VideoEncoderProperty(property_type, 1, p_param);
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        DASSERT(mpEncoderParams->index == DCOMPONENT_INDEX_FROM_GENERIC_ID(parameters->id));
                        mpEncoderParams->id = parameters->id;
                        mpEncoderParams->gop_M = parameters->gop_M;
                        mpEncoderParams->gop_N = parameters->gop_N;
                        mpEncoderParams->gop_idr_interval = parameters->gop_idr_interval;
                        mpEncoderParams->gop_structure = parameters->gop_structure;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoder\n");
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    DASSERT(mpEncoderParams);
                    if (mpEncoderParams) {
                        SVideoEncoderParam *parameters = (SVideoEncoderParam *)p_param;
                        parameters->id = mpEncoderParams->id;
                        parameters->gop_M = mpEncoderParams->gop_M;
                        parameters->gop_N = mpEncoderParams->gop_N;
                        parameters->gop_idr_interval = mpEncoderParams->gop_idr_interval;
                        parameters->gop_structure = mpEncoderParams->gop_structure;
                        ret = EECode_OK;
                    } else {
                        LOGM_FATAL("NULL mpEncoderParams\n");
                        ret = EECode_InternalLogicalBug;
                    }
                }
            } else {
                LOGM_FATAL("NULL p_param\n");
                ret = EECode_InternalLogicalBug;
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CVideoEncoder::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 0;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CVideoEncoder::PrintStatus()
{
    LOGM_PRINTF("msState=%d, %s, heart beat %d\n", msState, gfGetModuleStateString(msState), mDebugHeartBeat);

    if (mpOutputPin) {
        LOGM_PRINTF("       mpOutputPin[%p]'s status:\n", mpOutputPin);
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;
    return;
}

void CVideoEncoder::OnRun()
{
    SCMD cmd;
    IBufferPool *pBufferPool = NULL;
    EECode err = EECode_OK;
    TU32 current_remaining = 0, cached_frames = 0;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = 1;
    msState = EModuleState_WaitCmd;

    DASSERT(mpOutputPin);
    pBufferPool = mpOutputPin->GetBufferPool();
    DASSERT(pBufferPool);

    while (mbRun) {
        LOGM_STATE("OnRun: start switch, msState=%d, %s\n", msState, gfGetModuleStateString(msState));

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
            case EModuleState_HasOutputBuffer: {
                    while (mpWorkQ->PeekCmd(cmd)) {
                        processCmd(cmd);
                        if (DUnlikely(mbPaused)) {
                            msState = EModuleState_Pending;
                            break;
                        }
                    }

                    if (DUnlikely(!mbRun)) {
                        break;
                    }

                    DASSERT(!mpBuffer);
                    DASSERT(mpBufferPool);
                    if (!mpBufferPool) {
                        LOGM_FATAL("NULL mpBufferPool, must have internal bugs.\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    if (!mpBufferPool->AllocBuffer(mpBuffer, sizeof(CIBuffer))) {
                        LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    DASSERT(mpBuffer);
                    if (!mpBuffer) {
                        LOGM_FATAL("NULL mpBuffer, must have internal bugs.\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    err = mpEncoder->Encode(NULL, mpBuffer, current_remaining, cached_frames);
                    if (DLikely(mpBuffer->GetDataPtr() && EECode_OK == err)) {
                        /*if (1) {
                            FILE* file = fopen("transcode_es_dump.data", "ab");

                            if (file) {
                                fwrite(mpBuffer->GetDataPtr(), 1, mpBuffer->GetDataSize(), file);
                                fclose(file);
                                file = NULL;
                            } else {
                                LOGM_ALWAYS("open transcode_es_dump.data fail.\n");
                            }
                        }*/
#if 0
                        TU8 *pp = mpBuffer->GetDataPtr();
                        LOGM_PRINTF("encoder send size %ld, flags %08x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), pp[0], pp[1], pp[2], pp[3], pp[4], pp[5], pp[6], pp[7], pp[8], pp[9], pp[10], pp[11]);
#endif
                        mpOutputPin->SendBuffer(mpBuffer);
                        mpBuffer = NULL;
                    } else {
                        if (mpPersistMediaConfig->app_start_exit) {
                            LOGM_NOTICE("[program start exit]: mpBuffer_data=%p, err=%d, %s\n", mpBuffer->GetDataPtr(), err, gfGetErrorCodeString(err));
                            msState = EModuleState_Completed;
                        } else {
                            LOGM_ERROR("encoding failed, mpBuffer_data=%p, err=%d, %s\n", mpBuffer->GetDataPtr(), err, gfGetErrorCodeString(err));
                        }
                        mpBufferPool->Release(mpBuffer);
                        mpBuffer = NULL;
                    }
                }
                break;

            case EModuleState_Error:
            case EModuleState_Completed:
            case EModuleState_Pending:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("CVideoEncoder: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }
}

void CVideoEncoder::EventNotify(EEventType type, TU64 param1, TPointer param2)
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

void CVideoEncoder::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;
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

        case ECMDType_Pause:
            DASSERT(!mbPaused);
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (msState == EModuleState_Completed) {
                msState = EModuleState_Idle;
            } else if (msState == EModuleState_Error) {
                msState = EModuleState_Idle;
                LOGM_ERROR("from EModuleState_Error.\n");
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            msState = EModuleState_Pending;
            mbWaitKeyFrame = 1;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (msState == EModuleState_Error) {
                msState = EModuleState_Idle;
            } else {
                msState = EModuleState_Idle;
                LOGM_FATAL("invalid msState=%d in ECMDType_ResumeFlush\n", msState);
            }
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

                if (mpEncoder) {
                    if ((EReleaseFlag_None != cmd.flag) && mbEncoderRunning) {
                        mbEncoderRunning = 0;
                        mpEncoder->Stop();
                    }

                    if ((EReleaseFlag_Destroy == cmd.flag) && mbEncoderContentSetup) {
                        mpEncoder->DestroyContext();
                        mbEncoderContentSetup = 0;
                    }
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_ResumeSuspend: {
                msState = EModuleState_Idle;
                mbWaitKeyFrame = 1;

                if (mpEncoder) {
                    if (!mbEncoderContentSetup) {
                        DASSERT(mpEncoderParams);
                        SVideoEncoderInputParam param = {0};
                        param.index = mpEncoderParams->index;
                        param.format = mpEncoderParams->format;
                        param.prefer_dsp_mode = EDSPOperationMode_MultipleWindowUDEC;
                        param.cap_max_width = mpEncoderParams->pic_width;
                        param.cap_max_height = mpEncoderParams->pic_height;
                        param.bitrate = mpEncoderParams->bitrate;
                        param.platform = EDSPPlatform_AmbaI1;
                        param.framerate = mpEncoderParams->framerate;
                        err = mpEncoder->SetupContext(&param);
                        if (DUnlikely(EECode_OK != err)) {
                            LOGM_ERROR("mpEncoder->SetupContext() failed.\n");
                        }
                        mbEncoderContentSetup = 1;
                    }

                    if (!mbEncoderRunning) {
                        mbEncoderRunning = 1;
                        mpEncoder->Start();
                    }

                }
                mpWorkQ->CmdAck(EECode_OK);
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

#endif

