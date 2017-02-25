/*******************************************************************************
 * video_post_processor.cpp
 *
 * History:
 *    2013/04/05 - [Zhi He] create file
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

#include "video_post_processor.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateVideoPostProcessorFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoPostProcessor::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EVideoPostPModuleID _guess_video_postp_type_from_string(TChar *string)
{
    if (!string) {
        LOG_WARN("NULL input in _guess_video_postp_type_from_string, choose default\n");
        return EVideoPostPModuleID_AMBADSP;
    }

    if (!strncmp(string, "Direct", strlen("Direct"))) {
        return EVideoPostPModuleID_DirectPostP;
    } else if (!strncmp(string, "AMBA", strlen("AMBA"))) {
        return EVideoPostPModuleID_AMBADSP;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_postp_type_from_string, choose default\n", string);
    return EVideoPostPModuleID_AMBADSP;
}

CVideoPostProcessorInput *CVideoPostProcessorInput::Create(const TChar *name, IFilter *pFilter, CIQueue *queue, TU32 index)
{
    CVideoPostProcessorInput *result = new CVideoPostProcessorInput(name, pFilter, index);
    if (result && result->Construct(queue) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CVideoPostProcessorInput::Construct(CIQueue *queue)
{
    LOGM_DEBUG("CVideoPostProcessorInput::Construct.\n");
    EECode err = inherited::Construct(queue);
    return err;
}

CVideoPostProcessorInput::~CVideoPostProcessorInput()
{
    LOGM_RESOURCE("CVideoPostProcessorInput::~CVideoPostProcessorInput.\n");
}

//-----------------------------------------------------------------------
//
// CVideoPostProcessor
//
//-----------------------------------------------------------------------
IFilter *CVideoPostProcessor::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CVideoPostProcessor *result = new CVideoPostProcessor(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CVideoPostProcessor::CVideoPostProcessor(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpVideoPostProcessor(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mCurVideoPostProcessorID(EVideoPostPModuleID_Invalid)
    , mpBuffer(NULL)
    , mbVideoPostPContextSetup(0)
    , mbRecievedEosSignal(0)
    , mbPaused(0)
    , mbSetGlobalSetting(0)
{
    TUint i = 0;
    for (i = 0; i < EConstVideoPostProcessorMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }

    memset(&mPostPGlobalSetting, 0x0, sizeof(mPostPGlobalSetting));
}

EECode CVideoPostProcessor::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoPostPFilter);
    return inherited::Construct();
}

CVideoPostProcessor::~CVideoPostProcessor()
{
    TUint i = 0;

    LOGM_RESOURCE("CVideoPostProcessor::~CVideoPostProcessor(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoPostProcessorMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CVideoPostProcessor::~CVideoPostProcessor(), before mpVideoPostProcessor->Delete().\n");
    if (mpVideoPostProcessor) {
        mpVideoPostProcessor->Destroy();
        mpVideoPostProcessor = NULL;
    }

    LOGM_RESOURCE("CVideoPostProcessor::~CVideoPostProcessor(), end.\n");
}

void CVideoPostProcessor::Delete()
{
    inherited::Delete();
}

EECode CVideoPostProcessor::Initialize(TChar *p_param)
{
    EVideoPostPModuleID id;
    id = _guess_video_postp_type_from_string(p_param);

    LOGM_NOTICE("EVideoDecoderModuleID %d\n", id);

    if (mbVideoPostPContextSetup) {

        DASSERT(mpVideoPostProcessor);
        if (!mpVideoPostProcessor) {
            LOGM_FATAL("[Internal bug], why the mpVideoPostProcessor is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_NOTICE("before mpVideoPostProcessor->DestroyContext()\n");
        if (mbVideoPostPContextSetup) {
            mpVideoPostProcessor->DestroyContext();
            mbVideoPostPContextSetup = 0;
        }

        if (id != mCurVideoPostProcessorID) {
            LOGM_NOTICE("before mpVideoPostProcessor->Delete(), cur id %d, request id %d\n", mCurVideoPostProcessorID, id);
            mpVideoPostProcessor->Destroy();
            mpVideoPostProcessor = NULL;
        }
    }

    if (!mpVideoPostProcessor) {
        LOGM_NOTICE("before gfModuleFactoryVideoPostProcessor(%d)\n", id);
        mpVideoPostProcessor = gfModuleFactoryVideoPostProcessor(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpVideoPostProcessor) {
            mCurVideoPostProcessorID = id;
        } else {
            mCurVideoPostProcessorID = EVideoPostPModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoPostProcessor(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_NOTICE("[VideoPostP flow], before mpVideoPostProcessor->SetupContext()\n");
    mpVideoPostProcessor->SetupContext();
    mbVideoPostPContextSetup = 1;

    LOGM_NOTICE("[VideoPostP flow], CVideoPostProcessor::Initialize() done\n");

    return EECode_OK;
}

EECode CVideoPostProcessor::Finalize()
{
    if (mpVideoPostProcessor) {

        LOGM_NOTICE("before mpVideoPostProcessor->DestroyContext()\n");
        DASSERT(mbVideoPostPContextSetup);
        if (mbVideoPostPContextSetup) {
            mpVideoPostProcessor->DestroyContext();
            mbVideoPostPContextSetup = 0;
        }

        LOGM_NOTICE("before mpVideoPostProcessor->Delete(), cur id %d\n", mCurVideoPostProcessorID);
        mpVideoPostProcessor->Destroy();
        mpVideoPostProcessor = NULL;
    }

    return EECode_OK;
}

EECode CVideoPostProcessor::Start()
{
    //debug assert
    DASSERT(mpVideoPostProcessor);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoPostPContextSetup);

    return EECode_OK;
}

EECode CVideoPostProcessor::Stop()
{
    //debug assert
    DASSERT(mpVideoPostProcessor);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoPostPContextSetup);

    return EECode_OK;
}

void CVideoPostProcessor::Pause()
{
    //debug assert
    DASSERT(mpVideoPostProcessor);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoPostPContextSetup);

    DASSERT(!mbPaused);

    mbPaused = 1;

    return;
}

void CVideoPostProcessor::Resume()
{
    //debug assert
    DASSERT(mpVideoPostProcessor);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoPostPContextSetup);

    DASSERT(mbPaused);

    mbPaused = 0;

    return;
}

void CVideoPostProcessor::Flush()
{
    //debug assert
    DASSERT(mpVideoPostProcessor);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoPostPContextSetup);

    return;
}

void CVideoPostProcessor::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CVideoPostProcessor::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoPostProcessor::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoPostProcessor::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoPostProcessor::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoPostProcessor::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    if (StreamType_Video == type) {
        if (!mpOutputPin) {
            mpOutputPin = COutputPin::Create("[VideoPostProcessor video output pin]", (IFilter *) this);
            DASSERT(mpOutputPin);
            mpBufferPool = CBufferPool::Create("[VideoPostProcessor video buffer pool]", 16);
            DASSERT(mpBufferPool);
            mpOutputPin->SetBufferPool(mpBufferPool);

            EECode ret = mpOutputPin->AddSubPin(sub_index);
            if (DLikely(EECode_OK == ret)) {
                index = 0;
            } else {
                LOGM_FATAL("mpOutputPin->AddSubPin fail, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
                return ret;
            }
        } else {
            LOGM_FATAL("only one pin\n");
            return EECode_InternalLogicalBug;
        }
    }  else {
        LOGM_ERROR("BAD type %d\n", type);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CVideoPostProcessor::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Video == type) {
        if (mnInputPinsNumber >= EConstVideoPostProcessorMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CVideoPostProcessorInput::Create("[input pin for CVideoPostProcessor]", (IFilter *) this, mpWorkQ->MsgQ(), index);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IOutputPin *CVideoPostProcessor::GetOutputPin(TUint index, TUint sub_index)
{
    if (index || sub_index) {
        LOGM_FATAL("BAD index, sub_index should all equal zero\n");
        return NULL;
    }

    return mpOutputPin;
}

IInputPin *CVideoPostProcessor::GetInputPin(TUint index)
{
    if (EConstVideoPostProcessorMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CVideoPostProcessor::GetInputPin()\n", index, EConstVideoPostProcessorMaxInputPinNumber);
    }

    return NULL;
}

void CVideoPostProcessor::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_ERROR("TO DO\n");
}

EECode CVideoPostProcessor::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
    EECode err = EECode_OK;

    DASSERT(mpVideoPostProcessor);

    switch (property) {

        case EFilterPropertyType_vpostp_configure: {
                SVideoPostPConfiguration *configure = (SVideoPostPConfiguration *)p_param;
                const SVideoPostPConfiguration *current_configure;
                if (set_or_get) {
                    err = mpVideoPostProcessor->ConfigurePostP(configure, NULL, NULL);
                    DASSERT_OK(err);
                } else {
                    err = mpVideoPostProcessor->QueryVideoPostPInfo(current_configure);
                    DASSERT_OK(err);
                    *configure = *current_configure;
                }
            }
            break;

        case EFilterPropertyType_vpostp_global_setting: {
                SVideoPostPGlobalSetting *setting = (SVideoPostPGlobalSetting *)p_param;
                if (setting) {
                    mPostPGlobalSetting = *setting;
                    mbSetGlobalSetting = 1;
                } else {
                    LOGM_FATAL("NULL p_param in EFilterPropertyType_vpostp_global_setting\n");
                    err = EECode_BadParam;
                }
            }
            break;

        case EFilterPropertyType_vpostp_initial_configure: {
                SVideoPostPDisplayLayout *configure = (SVideoPostPDisplayLayout *)p_param;
                DASSERT(mbSetGlobalSetting);
                if (set_or_get) {
                    err = mpVideoPostProcessor->ConfigurePostP(NULL, configure, &mPostPGlobalSetting, 1);
                    DASSERT_OK(err);
                } else {
                    LOGM_ERROR("not support here\n");
                    err = EECode_NotSupported;
                }
            }
            break;

        case EFilterPropertyType_vpostp_display_rect:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vpostp_set_window:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vpostp_set_render:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vpostp_set_dec_source:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vpostp_stream_switch: {
                SVideoPostPStreamSwitch *stream_switch = (SVideoPostPStreamSwitch *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->StreamSwitch(stream_switch);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_mask: {
                SVideoPostPDisplayMask *mask = (SVideoPostPDisplayMask *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->UpdateDisplayMask(mask->new_display_vout_mask);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_layout: {
                SVideoPostPDisplayLayout *layout = (SVideoPostPDisplayLayout *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->UpdateToPreSetDisplayLayout(layout);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_layer: {
                SVideoPostPDisplayLayer *layer = (SVideoPostPDisplayLayer *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->UpdateToLayer(layer->request_display_layer);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_focus: {
                //SVideoPostPDisplayFocus* focus = (SVideoPostPDisplayFocus*)p_param;
                LOGM_FATAL("TO DO\n");
                //err = mpVideoPostProcessor->UpdateFocusIndex(focus->request_display_focus);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_swap_window_content: {
                SVideoPostPSwap *swap = (SVideoPostPSwap *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->SwapContent(swap);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_window_circular_shift: {
                SVideoPostPShift *shift = (SVideoPostPShift *)p_param;
                LOGM_FATAL("TO DO\n");
                err = mpVideoPostProcessor->CircularShiftContent(shift);
                DASSERT_OK(err);
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            err = EECode_InternalLogicalBug;
            break;
    }

    return err;
}

void CVideoPostProcessor::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CVideoPostProcessor::PrintStatus()
{
    LOGM_ERROR("TO DO\n");
}

void CVideoPostProcessor::postMsg(TUint msg_code)
{
    DASSERT(mpEngineMsgSink);
    if (mpEngineMsgSink) {
        SMSG msg;
        msg.code = msg_code;
        LOGM_NOTICE("Post msg_code %d to engine.\n", msg_code);
        mpEngineMsgSink->MsgProc(msg);
        LOGM_NOTICE("Post msg_code %d to engine done.\n", msg_code);
    }
}

bool CVideoPostProcessor::readInputData(CVideoPostProcessorInput *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer?\n");
        return false;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_NOTICE("CVideoPostProcessor %p get EOS.\n", this);
        DASSERT(!inputpin->GetEOS());
        inputpin->SetEOS(1);
        return true;
    } else {
        LOGM_ERROR("mpBuffer->GetBufferType() %d, mpBuffer->mFlags %d.\n", mpBuffer->GetBufferType(), mpBuffer->mFlags);
        return true;
    }

    return true;
}

EECode CVideoPostProcessor::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_NOTICE("before purge input pins\n");

    for (index = 0; index < EConstMuxerMaxInputPinNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge();
        }
    }

    LOGM_NOTICE("after purge input pins\n");

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

