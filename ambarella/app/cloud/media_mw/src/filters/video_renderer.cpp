/*******************************************************************************
 * video_renderer.cpp
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

#include "video_renderer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN


IFilter *gfCreateVideoRendererFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}


static EVideoRendererModuleID _guess_video_renderer_type_from_string(TChar *string)
{
    if (!string) {
        LOG_WARN("NULL input in _guess_video_renderer_type_from_string, choose default\n");
        return EVideoRendererModuleID_AMBADSP;
    }

    if (!strncmp(string, "AMBA", strlen("AMBA"))) {
        return EVideoRendererModuleID_AMBADSP;
    } else if (!strncmp(string, "Direct", strlen("Direct"))) {
        return EVideoRendererModuleID_DirectDraw;
    } else if (!strncmp(string, "DirectFrameBuffer", strlen("DirectFrameBuffer"))) {
        return EVideoRendererModuleID_DirectFrameBuffer;
    }

    LOG_WARN("unknown string tag(%s) in _guess_video_renderer_type_from_string, choose default\n", string);
    return EVideoRendererModuleID_AMBADSP;
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

CVideoRendererInput *CVideoRendererInput::Create(const TChar *name, IFilter *pFilter, CIQueue *queue, TUint index)
{
    CVideoRendererInput *result = new CVideoRendererInput(name, pFilter, index);
    if (result && result->Construct(queue) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CVideoRendererInput::Construct(CIQueue *queue)
{
    LOGM_DEBUG("CVideoRendererInput::Construct.\n");
    EECode err = inherited::Construct(queue);
    return err;
}

CVideoRendererInput::~CVideoRendererInput()
{
    LOGM_RESOURCE("CVideoRendererInput::~CVideoRendererInput.\n");
}

//-----------------------------------------------------------------------
//
// CVideoRenderer
//
//-----------------------------------------------------------------------
IFilter *CVideoRenderer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CVideoRenderer *result = new CVideoRenderer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CVideoRenderer::CVideoRenderer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpVideoRenderer(NULL)
    , mpVideoPostProcessor(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mCurVideoRendererID(EVideoRendererModuleID_Invalid)
    , mCurVideoPostpID(EVideoPostPModuleID_Invalid)
    , mpBuffer(NULL)
    , mWakeVoutIndex(0)
    , mbRecievedEosSignal(0)
    , mbPaused(0)
    , mbEOS(0)
    , mbVideoRendererSetup(0)
    , mbVideoPostPContextSetup(0)
    , mbSetGlobalSetting(0)
{
    TUint i = 0;

    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        mpInputPins[i] = 0;
    }
}

EECode CVideoRenderer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleVideoRendererFilter);

    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);

    return inherited::Construct();
}

CVideoRenderer::~CVideoRenderer()
{

}

void CVideoRenderer::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before delete inputpins.\n");
    for (i = 0; i < EConstVideoRendererMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before mpVideoRenderer->Delete().\n");
    if (mpVideoRenderer) {
        mpVideoRenderer->Destroy();
        mpVideoRenderer = NULL;
    }

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), before mpVideoPostProcessor->Delete().\n");
    if (mpVideoPostProcessor) {
        mpVideoPostProcessor->Destroy();
        mpVideoPostProcessor = NULL;
    }

    LOGM_RESOURCE("CVideoRenderer::~CVideoRenderer(), end.\n");

    inherited::Delete();
}

EECode CVideoRenderer::Initialize(TChar *p_param)
{
    EVideoRendererModuleID id;
    EVideoPostPModuleID postp_id;
    id = _guess_video_renderer_type_from_string(p_param);
    postp_id = _guess_video_postp_type_from_string(p_param);

    LOGM_INFO("EVideoDecoderModuleID %d, EVideoPostPModuleID %d\n", id, postp_id);

    if (mbVideoRendererSetup) {

        DASSERT(mpVideoRenderer);
        if (!mpVideoRenderer) {
            LOGM_FATAL("[Internal bug], why the mpVideoRenderer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
        mpVideoRenderer->DestroyContext();
        mbVideoRendererSetup = 0;

        if (id != mCurVideoRendererID) {
            LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d, request id %d\n", mCurVideoRendererID, id);
            mpVideoRenderer->Destroy();
            mpVideoRenderer = NULL;
        }

    }

    if (mbVideoPostPContextSetup) {

        DASSERT(mpVideoPostProcessor);
        if (!mpVideoPostProcessor) {
            LOGM_FATAL("[Internal bug], why the mpVideoPostProcessor is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("before mpVideoPostProcessor->DestroyContext()\n");
        mpVideoPostProcessor->DestroyContext();
        mbVideoPostPContextSetup = 0;

        if (postp_id != mCurVideoPostpID) {
            LOGM_INFO("before mpVideoPostProcessor->Delete(), cur id %d, request id %d\n", mCurVideoPostpID, id);
            mpVideoPostProcessor->Destroy();
            mpVideoPostProcessor = NULL;
        }

    }

    if (!mpVideoRenderer) {
        LOGM_INFO("before gfModuleFactoryVideoRenderer(%d)\n", id);
        mpVideoRenderer = gfModuleFactoryVideoRenderer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpVideoRenderer) {
            mCurVideoRendererID = id;
        } else {
            mCurVideoRendererID = EVideoRendererModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoRenderer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[VideoRenderer flow], before mpVideoRenderer->SetupContext()\n");
    mpVideoRenderer->SetupContext();
    mbVideoRendererSetup = 1;

    if (!mpVideoPostProcessor) {
        LOGM_INFO("before gfModuleFactoryVideoPostProcessor(%d)\n", postp_id);
        mpVideoPostProcessor = gfModuleFactoryVideoPostProcessor(postp_id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpVideoPostProcessor) {
            mCurVideoPostpID = postp_id;
        } else {
            mCurVideoPostpID = EVideoPostPModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryVideoPostProcessor(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[VideoPostP flow], before mpVideoPostProcessor->SetupContext()\n");
    mpVideoPostProcessor->SetupContext();
    mbVideoPostPContextSetup = 1;

    LOGM_INFO("[VideoRenderer flow], CVideoRenderer::Initialize() done\n");

    return EECode_OK;
}

EECode CVideoRenderer::Finalize()
{
    if (mpVideoRenderer) {

        LOGM_INFO("before mpVideoRenderer->DestroyContext()\n");
        DASSERT(mbVideoRendererSetup);
        if (mbVideoRendererSetup) {
            mpVideoRenderer->DestroyContext();
            mbVideoRendererSetup = 0;
        }

        LOGM_INFO("before mpVideoRenderer->Delete(), cur id %d\n", mCurVideoRendererID);
        mpVideoRenderer->Destroy();
        mpVideoRenderer = NULL;
    }

    if (mpVideoPostProcessor) {

        LOGM_INFO("before mpVideoPostProcessor->DestroyContext()\n");
        DASSERT(mbVideoPostPContextSetup);
        if (mbVideoPostPContextSetup) {
            mpVideoPostProcessor->DestroyContext();
            mbVideoPostPContextSetup = 0;
        }

        LOGM_INFO("before mpVideoPostProcessor->Delete(), cur id %d\n", mCurVideoPostpID);
        mpVideoPostProcessor->Destroy();
        mpVideoPostProcessor = NULL;
    }

    return EECode_OK;
}

void CVideoRenderer::PrintState()
{
    LOGM_FATAL("TO DO\n");
}

EECode CVideoRenderer::Run()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Run();

    return EECode_OK;
}

EECode CVideoRenderer::Start()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Start();

    return EECode_OK;
}

EECode CVideoRenderer::Stop()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Stop();

    return EECode_OK;
}

void CVideoRenderer::Pause()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Pause();

    return;
}

void CVideoRenderer::Resume()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Resume();

    return;
}

void CVideoRenderer::Flush()
{
    //debug assert
    DASSERT(mpVideoRenderer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbVideoRendererSetup);

    inherited::Flush();

    return;
}

void CVideoRenderer::ResumeFlush()
{
    inherited::ResumeFlush();
    return;
}

EECode CVideoRenderer::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CVideoRenderer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CVideoRenderer::AddInputPin(TUint &index, TUint type)
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

        mpInputPins[mnInputPinsNumber] = CVideoRendererInput::Create("[Audio input pin for CVideoRenderer]", (IFilter *) this, mpWorkQ->MsgQ(), index);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

IInputPin *CVideoRenderer::GetInputPin(TUint index)
{
    if (EConstVideoRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CVideoRenderer::GetInputPin()\n", index, EConstVideoRendererMaxInputPinNumber);
    }

    return NULL;
}

void CVideoRenderer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    LOGM_ERROR("TO DO\n");
}

EECode CVideoRenderer::FilterProperty(EFilterPropertyType property, TUint set_or_get, void *p_param)
{
    EECode err = EECode_OK;

    //DASSERT(mpVideoRenderer);
    //DASSERT(mpVideoPostProcessor);

    switch (property) {

        case EFilterPropertyType_vpostp_configure: {
                SVideoPostPConfiguration *configure = (SVideoPostPConfiguration *)p_param;
                const SVideoPostPConfiguration *current_configure;
                if (set_or_get) {
                    err = mpVideoPostProcessor->ConfigurePostP(configure, NULL, NULL, 0, configure->set_config_direct_to_dsp);
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
                LOGM_FLOW("before EFilterPropertyType_vpostp_global_setting\n");
                if (setting) {
                    mPostPGlobalSetting = *setting;
                    mbSetGlobalSetting = 1;
                } else {
                    LOGM_FATAL("NULL p_param in EFilterPropertyType_vpostp_global_setting\n");
                    err = EECode_BadParam;
                }
                LOGM_FLOW("after EFilterPropertyType_vpostp_global_setting\n");
            }
            break;

        case EFilterPropertyType_vpostp_initial_configure: {
                SVideoPostPDisplayLayout *configure = (SVideoPostPDisplayLayout *)p_param;
                DASSERT(mbSetGlobalSetting);
                LOGM_FLOW("before EFilterPropertyType_vpostp_initial_configure\n");
                if (set_or_get) {
                    err = mpVideoPostProcessor->ConfigurePostP(NULL, configure, &mPostPGlobalSetting, 1);
                    DASSERT_OK(err);
                } else {
                    LOGM_ERROR("not support here\n");
                    err = EECode_NotSupported;
                }
                LOGM_FLOW("after EFilterPropertyType_vpostp_initial_configure\n");
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
                err = mpVideoPostProcessor->UpdateDisplayMask(mask->new_display_vout_mask);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_layout: {
                DASSERT(set_or_get);
                SVideoPostPDisplayLayout *layout = (SVideoPostPDisplayLayout *)p_param;
                err = mpVideoPostProcessor->UpdateToPreSetDisplayLayout(layout);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_highlightenwindowsize: {
                SVideoPostPDisplayHighLightenWindowSize *size = (SVideoPostPDisplayHighLightenWindowSize *)p_param;
                err = mpVideoPostProcessor->UpdateHighlightenWindowSize(size);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_update_display_layer: {
                SVideoPostPDisplayLayer *layer = (SVideoPostPDisplayLayer *)p_param;
                err = mpVideoPostProcessor->UpdateToLayer(layer->request_display_layer);
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_switch_window_to_HD: {
                SVideoPostPDisplayHD *hd = (SVideoPostPDisplayHD *)p_param;
                err = mpVideoPostProcessor->SwitchWindowToHD(hd);
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

        case EFilterPropertyType_vpostp_query_display: {
                SVideoPostPConfiguration *postp = (SVideoPostPConfiguration *)p_param;
                const SVideoPostPConfiguration *cur_postp = NULL;
                if (postp && mpVideoPostProcessor) {
                    err = mpVideoPostProcessor->QueryVideoPostPInfo(cur_postp);
                    DASSERT_OK(err);
                    DASSERT(cur_postp);
                    *postp = *cur_postp;
                }
            }
            break;

        case EFilterPropertyType_vrenderer_trickplay_pause:
            DASSERT(mpVideoRenderer);
            if (mpVideoRenderer) {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)p_param;
                TUint index = trickplay->id;
                //LOGM_FLOW("index %d\n", index);
                err = mpVideoRenderer->Pause(index);
                DASSERT_OK(err);
            } else {
                LOGM_FATAL("NULL mpVideoRenderer\n");
                err = EECode_BadState;
            }
            break;

        case EFilterPropertyType_vrenderer_trickplay_resume:
            DASSERT(mpVideoRenderer);
            if (mpVideoRenderer) {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)p_param;
                TUint index = trickplay->id;
                //LOGM_FLOW("index %d\n", index);
                err = mpVideoRenderer->Resume(index);
                DASSERT_OK(err);
            } else {
                LOGM_FATAL("NULL mpVideoRenderer\n");
                err = EECode_BadState;
            }
            break;

        case EFilterPropertyType_vrenderer_trickplay_step:
            DASSERT(mpVideoRenderer);
            if (mpVideoRenderer) {
                SVideoPostPDisplayTrickPlay *trickplay = (SVideoPostPDisplayTrickPlay *)p_param;
                TUint index = trickplay->id;
                //LOGM_FLOW("index %d\n", index);
                err = mpVideoRenderer->StepPlay(index);
                DASSERT_OK(err);
            } else {
                LOGM_FATAL("NULL mpVideoRenderer\n");
                err = EECode_BadState;
            }
            break;

        case EFilterPropertyType_vrenderer_sync_to_audio:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vrenderer_align_all_video_streams:
            LOGM_FATAL("TO DO\n");
            break;

        case EFilterPropertyType_vrenderer_wake_vout:
            if (p_param) {
                SDSPControlParams *param = (SDSPControlParams *)p_param;
                SCMD cmd;

                cmd.code = ECMDType_NotifyUDECInRuningState;
                cmd.res32_1 = param->u8_param[0];
                mpWorkQ->PostMsg(cmd);
            }
            break;

        case EFilterPropertyType_vpostp_playback_capture:
            if (p_param && mpVideoPostProcessor) {
                err = mpVideoPostProcessor->PlaybackCapture((SPlaybackCapture *)p_param);
            } else {
                LOGM_ERROR("NULL p_param %p, or NULL mpVideoPostProcessor %p\n", p_param, mpVideoPostProcessor);
                return EECode_BadParam;
            }
            break;

        case EFilterPropertyType_vpostp_playback_zoom:
            if (p_param && mpVideoPostProcessor) {
                err = mpVideoPostProcessor->PlaybackZoom((SPlaybackZoom *)p_param);
            } else {
                LOGM_ERROR("NULL p_param %p, or NULL mpVideoPostProcessor %p\n", p_param, mpVideoPostProcessor);
                return EECode_BadParam;
            }
            break;

        case EFilterPropertyType_vpostp_query_window: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)p_param;
                err = mpVideoPostProcessor->QueryCurrentWindow(mapping->window_id, mapping->decoder_id, mapping->render_id);
                mapping->window_id_2rd = DINVALID_VALUE_TAG;
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_query_render: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)p_param;
                err = mpVideoPostProcessor->QueryCurrentRender(mapping->render_id, mapping->decoder_id, mapping->window_id, mapping->window_id_2rd);
                mapping->window_id_2rd = DINVALID_VALUE_TAG;
                DASSERT_OK(err);
            }
            break;

        case EFilterPropertyType_vpostp_query_dsp_decoder: {
                SVideoPostPDisplayMWMapping *mapping = (SVideoPostPDisplayMWMapping *)p_param;
                err = mpVideoPostProcessor->QueryCurrentUDEC(mapping->decoder_id, mapping->window_id, mapping->render_id, mapping->window_id_2rd);
                mapping->window_id_2rd = DINVALID_VALUE_TAG;
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

void CVideoRenderer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = 1;
    info.nOutput = 0;

    info.pName = mpModuleName;
}

void CVideoRenderer::PrintStatus()
{
    TUint i = 0;

    LOGM_ALWAYS("msState=%d, %s, mnCurrentInputPinNumber\n", msState, gfGetModuleStateString(msState));

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_ALWAYS("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    return;
}

void CVideoRenderer::postMsg(TUint msg_code)
{
    DASSERT(mpEngineMsgSink);
    if (mpEngineMsgSink) {
        SMSG msg;
        msg.code = msg_code;
        LOGM_FLOW("Post msg_code %d to engine.\n", msg_code);
        mpEngineMsgSink->MsgProc(msg);
        LOGM_FLOW("Post msg_code %d to engine done.\n", msg_code);
    }
}

bool CVideoRenderer::readInputData(CVideoRendererInput *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer?\n");
        return false;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_FLOW("CVideoRenderer %p get EOS.\n", this);
        DASSERT(!mbEOS);
        mbEOS = 1;
        return true;
    } else {
        LOGM_ERROR("mpBuffer->GetBufferType() %d, mpBuffer->mFlags() %d.\n", mpBuffer->GetBufferType(), mpBuffer->mFlags);
        return true;
    }

    return true;
}

EECode CVideoRenderer::flushInputData()
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

EECode CVideoRenderer::processCmd(SCMD &cmd)
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
            if (EDSPOperationMode_UDEC == mpPersistMediaConfig->dsp_config.request_dsp_mode) {
                msState = EModuleState_WaitCmd;
            } else {
                DASSERT(EDSPOperationMode_MultipleWindowUDEC == mpPersistMediaConfig->dsp_config.request_dsp_mode || EDSPOperationMode_MultipleWindowUDEC_Transcode == mpPersistMediaConfig->dsp_config.request_dsp_mode);
                msState = EModuleState_Idle;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Completed == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Stopped == msState) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            if (mpCurrentInputPin) {
                mpCurrentInputPin->Purge(0);
            }
            msState = EModuleState_Stopped;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            msState = EModuleState_Idle;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_NotifyUDECInRuningState:
            if (EDSPOperationMode_UDEC == mpPersistMediaConfig->dsp_config.request_dsp_mode) {
                //LOGM_FLOW("ECMDType_NotifyUDECInRuningState, msState %d\n", msState);
                mWakeVoutIndex = cmd.res32_1;
                DASSERT(EModuleState_WaitCmd == msState);
                msState = EModuleState_Renderer_WaitVoutDormant;
            }
            break;

        default:
            LOGM_ERROR("processCmd, wrong cmd %d, %s.\n", cmd.code, gfGetCMDTypeString((ECMDType)cmd.code));
            break;
    }

    return err;
}

void CVideoRenderer::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    EECode err;

    msState = EModuleState_WaitCmd;
    mbRun = 1;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = true;

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

            case EModuleState_Renderer_WaitVoutDormant:
                DASSERT(mpVideoRenderer);
                if (mpVideoRenderer) {
                    LOGM_FLOW("before mpVideoRenderer->WaitVoutDormant(%d)\n", mWakeVoutIndex);
                    err = mpVideoRenderer->WaitVoutDormant((TUint)mWakeVoutIndex);
                    LOGM_FLOW("mpVideoRenderer->WaitVoutDormant(%d) return %d, %s\n", mWakeVoutIndex, err, gfGetErrorCodeString(err));
                    if (EECode_OK == err) {
                        err = mpVideoRenderer->WakeVout((TUint)mWakeVoutIndex);
                        if (EECode_OK == err) {
                            msState = EModuleState_Idle;
                            break;
                        } else {
                            LOGM_FATAL("mpVideoRenderer->WakeVout((TUint)mWakeVoutIndex %d) fail, ret %d, %s\n", mWakeVoutIndex, err, gfGetErrorCodeString(err));
                            msState = EModuleState_Error;
                            break;
                        }
                    } else if (EECode_TryAgain == err) {
                        LOGM_INFO("try again(WaitVoutDormant)\n");
                    } else {
                        LOGM_FATAL("mpVideoRenderer->WaitVoutDormant((TUint)mWakeVoutIndex %d) fail, ret %d, %s\n", mWakeVoutIndex, err, gfGetErrorCodeString(err));
                        msState = EModuleState_Error;
                        break;
                    }
                } else {
                    LOGM_FATAL("NULL mpVideoRenderer\n");
                    msState = EModuleState_Error;
                    break;
                }

                if (mpWorkQ->PeekCmd(cmd)) {
                    processCmd(cmd);
                }
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
                    mpCurrentInputPin = (CVideoRendererInput *)result.pOwner;
                    DASSERT(!mpBuffer);
                    if (mpCurrentInputPin->PeekBuffer(mpBuffer)) {
                        msState = EModuleState_Running;
                    } else {
                        LOGM_FATAL("mpCurrentInputPin->PeekBuffer(mpBuffer) fail?\n");
                        msState = EModuleState_Error;
                    }
                }
                break;

            case EModuleState_Completed:
            case EModuleState_Stopped:
            case EModuleState_Pending:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer && mpVideoRenderer);

                if (EBufferType_FlowControl_EOS == mpBuffer->GetBufferType()) {
                    DASSERT(mpEngineMsgSink);
                    if (mpEngineMsgSink) {
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
                    //TO DO
                    LOGM_FATAL("TO DO!\n");
                }

                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Idle;
                break;

            default:
                LOGM_ERROR("CVideoRenderer: OnRun: state invalid: 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

