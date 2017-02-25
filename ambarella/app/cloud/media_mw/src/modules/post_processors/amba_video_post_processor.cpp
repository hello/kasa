/*******************************************************************************
 * amba_video_decoder.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
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

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "amba_video_post_processor.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IVideoPostProcessor *gfCreateAmbaVideoPostProcessorModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoPostProcessor::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}
/*
static SDSPMUdecWindow* __findWindow(SDSPMUdecWindow* windows, TUint total_number, TUint win_id)
{
    TUint i = 0;
    DASSERT(windows);

    for (i = 0; i < total_number; i ++) {
        if (windows->win_config_id == win_id) {
            return windows;
        }

        windows++;
    }

    LOG_WARN("do not found window\n");
    return NULL;
}
*/
static SDSPMUdecRender *__findRender(SDSPMUdecRender *renders, TUint total_number, TUint render_id)
{
    TUint i = 0;
    DASSERT(renders);

    for (i = 0; i < total_number; i ++) {
        if (renders->render_id == render_id) {
            return renders;
        }

        renders++;
    }

    LOG_WARN("do not found render\n");
    return NULL;
}

static SDSPMUdecRender *__findRenderViaWindow(SDSPMUdecRender *renders, TUint total_number, TUint win_id)
{
    TUint i = 0;
    DASSERT(renders);

    for (i = 0; i < total_number; i ++) {
        if (renders->win_config_id == win_id) {
            return renders;
        }

        renders++;
    }

    LOG_WARN("do not found render\n");
    return NULL;
}

//-----------------------------------------------------------------------
//
// CAmbaVideoPostProcessor
//
//-----------------------------------------------------------------------
CAmbaVideoPostProcessor::CAmbaVideoPostProcessor(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mpDSP(NULL)
    , mConsistantConfigVoutMask(DCAL_BITMASK(EDisplayDevice_HDMI))
    , mConsistantConfigVoutPrimaryIndex(EDisplayDevice_HDMI)
    , mVoutMask(DCAL_BITMASK(EDisplayDevice_HDMI))
    , mbWaitSwitchMsg(0)
    , mbResetLayoutWhenChanged(0)
    , mCurrentConfiguration(0)
    , mbUseCustomizedConfiguration(0)
    , mbCustomizedConfigurationSetup(0)
{

}

IVideoPostProcessor *CAmbaVideoPostProcessor::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoPostProcessor *result = new CAmbaVideoPostProcessor(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        //        LOGM_ERROR("CAmbaVideoPostProcessor->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

void CAmbaVideoPostProcessor::Destroy()
{
    Delete();
}

EECode CAmbaVideoPostProcessor::Construct()
{
    TUint i = 0;

    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaVideoPostProcessor);

    memset(&mDisplayLayoutConfiguration, 0x0, sizeof(mDisplayLayoutConfiguration));
    memset(&mVideoPostPConfiguration, 0x0, sizeof(mVideoPostPConfiguration));
    memset(&mCustomizedVideoPostPConfiguration, 0x0, sizeof(mCustomizedVideoPostPConfiguration));

    for (i = 0; i < DMaxDisplayPatternNumber; i ++) {
        mbVideoPostPConfigurationSetup[i] = 0;
    }

    mpMutex = gfCreateMutex();
    DASSERT(mpMutex);

    return EECode_OK;
}

CAmbaVideoPostProcessor::~CAmbaVideoPostProcessor()
{
    //LOGM_RESOURCE("~CAmbaVideoPostProcessor.\n");
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
    //LOGM_RESOURCE("~CAmbaVideoPostProcessor done.\n");
}

EECode CAmbaVideoPostProcessor::SetupContext()
{
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpPersistMediaConfig->dsp_config.p_dsp_handler);

    mpDSP = (IDSPAPI *)mpPersistMediaConfig->dsp_config.p_dsp_handler;

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::DestroyContext()
{
    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::findDisplayLaout(TU8 &index, SVideoPostPDisplayLayout *p_layout_config)
{
    TU8 i = 0, free_index = 0xff;
    for (i = 0; i < DMaxDisplayPatternNumber; i++) {
        if (mbVideoPostPConfigurationSetup[i]) {
            if (mDisplayLayoutConfiguration[i].layer_layout_type[0] == p_layout_config->layer_layout_type[0] && \
                    mDisplayLayoutConfiguration[i].layer_layout_type[1] == p_layout_config->layer_layout_type[1] && \
                    mDisplayLayoutConfiguration[i].layer_number == p_layout_config->layer_number && \
                    mDisplayLayoutConfiguration[i].layer_window_number[0] == p_layout_config->layer_window_number[0] && \
                    mDisplayLayoutConfiguration[i].layer_window_number[1] == p_layout_config->layer_window_number[1]) {
                index = i;
                return EECode_OK;
            }
        } else {
            LOGM_INFO("find free slot %d\n", i);
            free_index = i;
            break;
        }
    }

    if (0xff != free_index) {
        LOGM_INFO("new layout request, return free slot %d\n", free_index);
        index = free_index;
        return EECode_NotInitilized;
    }

    LOGM_ERROR("too many request layout number\n");
    return EECode_TooMany;
}

EECode CAmbaVideoPostProcessor::setupDisplayLayout(TU8 index, SVideoPostPDisplayLayout *config)
{
    EECode err;

    DASSERT(index < DMaxDisplayPatternNumber);

    if (index < DMaxDisplayPatternNumber) {
        err = gfPresetDisplayLayout(&mGlobalSetting, config, &mVideoPostPConfiguration[index]);
        DASSERT_OK(err);
        err = gfDefaultRenderConfig(&mVideoPostPConfiguration[index].render[0], mVideoPostPConfiguration[index].cur_render_number, mVideoPostPConfiguration[index].total_render_number - mVideoPostPConfiguration[index].cur_render_number);
        DASSERT_OK(err);
        return err;
    }

    LOGM_FATAL("BAD index %d, exceed max value %d\n", index, DMaxDisplayPatternNumber);
    return EECode_BadParam;
}

EECode CAmbaVideoPostProcessor::updateDisplay()
{
    //    EECode err = EECode_OK;

    DASSERT(mpDSP);
    DASSERT(mCurrentConfiguration < DMaxDisplayPatternNumber);
    DASSERT(mbVideoPostPConfigurationSetup[mCurrentConfiguration]);

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::updateToLayer(SVideoPostPConfiguration *p_cur_config, TU8 layer)
{
    //    EECode err = EECode_OK;
    TU8 win_start_index = 0, i = 0;

    DASSERT(p_cur_config);
    if (p_cur_config) {
        p_cur_config->cur_window_number = p_cur_config->layer_window_number[layer];

        DASSERT(p_cur_config->total_window_number <= DMaxPostPWindowNumber);

        p_cur_config->single_view_mode = 0;

        win_start_index = 0;
        i = 0;
        while (i < layer) {
            win_start_index += p_cur_config->layer_window_number[i];
            i ++;
        }

        DASSERT(win_start_index < p_cur_config->total_window_number);
        DASSERT((win_start_index +  p_cur_config->cur_window_number) <= p_cur_config->total_window_number);

        p_cur_config->cur_render_start_index = win_start_index;
        p_cur_config->cur_render_number = p_cur_config->cur_window_number;

    } else {
        LOGM_FATAL("NULL pointer p_cur_config\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::UpdateToPreSetDisplayLayout(SVideoPostPDisplayLayout *config)
{
    EECode err = EECode_OK;
    TU8 index = 0;

    AUTO_LOCK(mpMutex);

    DASSERT(mpDSP);
    DASSERT(config);
    if (!config) {
        LOGM_FATAL("NULL input param\n");
        return EECode_BadParam;
    }

    err = findDisplayLaout(index, config);
    if (EECode_OK == err) {
        if (mCurrentConfiguration == index) {
            LOGM_INFO("layout not changed, do nothing\n");
            return EECode_OK;
        }
        mCurrentConfiguration = index;

        LOGM_NOTICE("config->layer_layout_type[0] %d, config->layer_layout_type[1] %d\n", config->layer_layout_type[0], config->layer_layout_type[1]);
        LOGM_NOTICE("config->layer_window_number[0] %d, config->layer_window_number[1] %d\n", config->layer_window_number[0], config->layer_window_number[1]);
        LOGM_NOTICE("config->layer_number %d, ret index %d\n", config->layer_number, index);

        if (mpDSP) {
            err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, &mVideoPostPConfiguration[mCurrentConfiguration]);
        } else {
            LOGM_FATAL("NULL mpDSP\n");
            return EECode_InternalLogicalBug;
        }
    } else if (EECode_NotInitilized == err) {
        DASSERT(0 == mbVideoPostPConfigurationSetup[index]);

        err = setupDisplayLayout(index, config);
        DASSERT_OK(err);
        if (EECode_OK != err) {
            return err;
        }

        LOGM_NOTICE("init config->layer_layout_type[0] %d, config->layer_layout_type[1] %d\n", config->layer_layout_type[0], config->layer_layout_type[1]);
        LOGM_NOTICE("init config->layer_window_number[0] %d, config->layer_window_number[1] %d\n", config->layer_window_number[0], config->layer_window_number[1]);
        LOGM_NOTICE("init config->layer_number %d, ret index %d\n", config->layer_number, index);

        mbVideoPostPConfigurationSetup[index] = 1;
        mDisplayLayoutConfiguration[index] = *config;
        mCurrentConfiguration = index;

        if (mpDSP) {
            err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, &mVideoPostPConfiguration[mCurrentConfiguration]);
        } else {
            LOGM_FATAL("NULL mpDSP\n");
            return EECode_InternalLogicalBug;
        }
    } else {
        LOGM_FATAL("too many layout request\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::UpdateHighlightenWindowSize(SVideoPostPDisplayHighLightenWindowSize *size)
{
    EECode err = EECode_OK;

    AUTO_LOCK(mpMutex);

    DASSERT(mpDSP);
    DASSERT(size);
    if (!size) {
        LOGM_FATAL("NULL input param\n");
        return EECode_BadParam;
    }

    if (EDisplayLayout_BottomLeftHighLighten != mDisplayLayoutConfiguration[mCurrentConfiguration].layer_layout_type[0]) {
        LOGM_ERROR("Not EDisplayLayout_BottomLeftHighLighten, UpdateHighlightenWindowSize not supported.\n");
        return EECode_BadState;
    }
    if (size->window_width > mGlobalSetting.display_width
            || size->window_height > mGlobalSetting.display_height) {
        LOGM_ERROR("HighlightenWindowSize %hux%hu out of range of VOUT size %hux%hu\n", size->window_width, size->window_height, mGlobalSetting.display_width, mGlobalSetting.display_height);
        return EECode_BadParam;
    }
    err = gfUpdateHighlightenDisplayLayout(&mGlobalSetting, &mDisplayLayoutConfiguration[mCurrentConfiguration], size, &mVideoPostPConfiguration[mCurrentConfiguration]);
    DASSERT_OK(err);
    if (EECode_OK != err) {
        return err;
    }

    if (mpDSP) {
        err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, &mVideoPostPConfiguration[mCurrentConfiguration]);
    } else {
        LOGM_FATAL("NULL mpDSP\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::ConfigurePostP(SVideoPostPConfiguration *p_config, SVideoPostPDisplayLayout *p_layout_config, SVideoPostPGlobalSetting *p_global_setting, TUint b_set_initial_value, TUint b_p_config_direct_to_dsp)
{
    TU8 i = 0, index = 0;
    EECode err = EECode_OK;

    AUTO_LOCK(mpMutex);

    LOGM_INFO("before ConfigurePostP(%p, %p, %p, %d, %d)\n", p_config, p_layout_config, p_global_setting, b_set_initial_value, b_p_config_direct_to_dsp);

    if (p_global_setting) {
        mGlobalSetting = *p_global_setting;
        //need check here?
    }

    if (p_layout_config) {
        DASSERT(!p_config);

        err = findDisplayLaout(index, p_layout_config);
        if (EECode_OK == err) {
            if (mCurrentConfiguration == index) {
                LOGM_INFO("layout not changed, do nothing\n");
                return EECode_OK;
            }
            mDisplayLayoutConfiguration[index] = *p_layout_config;
            mCurrentConfiguration = index;
            if (!b_set_initial_value) {
                if (mpDSP) {
                    err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (SDSPControlParams *)(&mVideoPostPConfiguration[mCurrentConfiguration]));
                } else {
                    LOGM_FATAL("NULL mpDSP\n");
                    return EECode_InternalLogicalBug;
                }
            } else {
                //set initialized value, should not comes here
                LOGM_WARN("set initialized video_post_p value, some one has set before?\n");
            }
        } else if (EECode_NotInitilized == err) {
            DASSERT(0 == mbVideoPostPConfigurationSetup[index]);

            LOGM_INFO("in ConfigurePostP(%p, %p, %p, %d, %d), before setupDisplayLayout()\n", p_config, p_layout_config, p_global_setting, b_set_initial_value, b_p_config_direct_to_dsp);

            err = setupDisplayLayout(index, p_layout_config);
            DASSERT_OK(err);
            if (EECode_OK != err) {
                return err;
            }

            LOGM_INFO("in ConfigurePostP(%p, %p, %p, %d, %d), after setupDisplayLayout(), index %d\n", p_config, p_layout_config, p_global_setting, b_set_initial_value, b_p_config_direct_to_dsp, index);
            mbVideoPostPConfigurationSetup[index] = 1;
            mCurrentConfiguration = index;
            mDisplayLayoutConfiguration[index] = *p_layout_config;

            if (!b_set_initial_value) {
                if (mpDSP) {
                    err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (void *)(&mVideoPostPConfiguration[mCurrentConfiguration]));
                } else {
                    LOGM_FATAL("NULL mpDSP\n");
                    return EECode_InternalLogicalBug;
                }
            } else {
                //set initialized value here
                LOGM_INFO("set initialized video_post_p value here\n");
            }
        } else {
            LOGM_FATAL("too many layout request\n");
            return EECode_InternalLogicalBug;
        }
    } else if (p_config) {
        if (b_p_config_direct_to_dsp) {
            if (mpDSP) {
                mVideoPostPConfiguration[mCurrentConfiguration] = *p_config;
                err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (void *)(&mVideoPostPConfiguration[mCurrentConfiguration]));
                LOGM_NOTICE("direct update mCurrentConfiguration %u to DSP done.\n", mCurrentConfiguration);
            } else {
                LOGM_FATAL("NULL mpDSP\n");
                return EECode_InternalLogicalBug;
            }
        } else {
            for (i = 0; i < DMaxDisplayPatternNumber; i++) {
                if (!mbVideoPostPConfigurationSetup[i]) {
                    mbVideoPostPConfigurationSetup[i] = 1;
                    mVideoPostPConfiguration[i] = *p_config;
                    mCurrentConfiguration = i;
                    return EECode_OK;
                } else {
                    LOGM_FATAL("must not comes here\n");
                    return EECode_InternalLogicalBug;
                }
            }
        }
    } else {
        LOGM_FATAL("NULL p_config and p_layout_config\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::PlaybackCapture(SPlaybackCapture *p_capture)
{
    EECode err;

    if (mpDSP) {
        err = mpDSP->DSPControl(EDSPControlType_UDEC_pb_still_capture, (void *)p_capture);
    } else {
        LOGM_FATAL("NULL mpDSP\n");
        return EECode_BadState;
    }

    return err;
}

EECode CAmbaVideoPostProcessor::PlaybackZoom(SPlaybackZoom *p_zoom)
{
    EECode err;

    if (mpDSP) {
        SDSPControlParams params;
        params.u8_param[0] = p_zoom->render_id;
        params.u8_param[1] = p_zoom->zoom_mode;
        params.u16_param[0] = p_zoom->input_center_x;
        params.u16_param[1] = p_zoom->input_center_y;
        params.u16_param[2] = p_zoom->input_width;
        params.u16_param[3] = p_zoom->input_height;
        params.u32_param[0] = p_zoom->zoom_factor_x;
        params.u32_param[1] = p_zoom->zoom_factor_y;
        err = mpDSP->DSPControl(EDSPControlType_UDEC_zoom, (void *)&params);
    } else {
        LOGM_FATAL("NULL mpDSP\n");
        return EECode_BadState;
    }

    return err;
}

EECode CAmbaVideoPostProcessor::ConfigureWindow(SDSPMUdecWindow *p_window, TUint start_index, TUint number)
{
    LOGM_FATAL("This API is not implemented yet\n");
    return EECode_NoImplement;
}

EECode CAmbaVideoPostProcessor::ConfigureRender(SDSPMUdecRender *p_render, TUint start_index, TUint number)
{
    LOGM_FATAL("This API is not implemented yet\n");
    return EECode_NoImplement;
}

EECode CAmbaVideoPostProcessor::UpdateDisplayMask(TU8 new_mask)
{
    SVideoPostPConfiguration *p_cur_config = NULL;
    EECode err;

    AUTO_LOCK(mpMutex);

    if (new_mask & (~(mConsistantConfigVoutMask))) {
        LOGM_ERROR("BAD request vout mask %02x, mConsistantConfigVoutMask %02x, exceed the vout mask range\n", new_mask, mConsistantConfigVoutMask);
        return EECode_BadParam;
    } else if (!(new_mask & (DCAL_BITMASK(mConsistantConfigVoutPrimaryIndex)))) {
        LOGM_ERROR("BAD request vout mask %02x, not include primary vout, mConsistantConfigVoutPrimaryIndex %02x\n", new_mask, mConsistantConfigVoutPrimaryIndex);
        return EECode_BadParam;
    }

    if (new_mask != mVoutMask) {
        LOGM_INFO("new request vout mask %02x, ori mask %02x\n", new_mask, mVoutMask);
        if (mbUseCustomizedConfiguration) {
            DASSERT(mbCustomizedConfigurationSetup);
            p_cur_config = &mCustomizedVideoPostPConfiguration;
        } else {
            DASSERT(mCurrentConfiguration < DMaxDisplayPatternNumber);
            p_cur_config = &mVideoPostPConfiguration[mCurrentConfiguration];
        }

        if (mpDSP) {
            p_cur_config->voutA_enabled = new_mask & (DCAL_BITMASK(EDisplayDevice_LCD));
            p_cur_config->voutB_enabled = new_mask & (DCAL_BITMASK(EDisplayDevice_HDMI));
            err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (SDSPControlParams *)p_cur_config);
            DASSERT_OK(err);
            if (EECode_OK == err) {
                LOGM_INFO("vout mask %02x ---> %02x done\n", mVoutMask, new_mask);
                mVoutMask = new_mask;
            } else {
                p_cur_config->voutA_enabled = mVoutMask & (DCAL_BITMASK(EDisplayDevice_LCD));
                p_cur_config->voutB_enabled = mVoutMask & (DCAL_BITMASK(EDisplayDevice_HDMI));
                LOGM_FATAL("mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display() fail, ret %d\n", err);
                return err;
            }
        } else {
            LOGM_FATAL("NULL mpDSP\n");
            return EECode_InternalLogicalBug;
        }
    } else {
        LOGM_INFO("vout mask not changed!\n");
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::UpdateToLayer(TUint layer, TUint seamless)
{
    SVideoPostPConfiguration *p_cur_config = NULL;
    EECode err;

    AUTO_LOCK(mpMutex);

    LOGM_INFO("update to layer %d, seamless %d\n", layer, seamless);
    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        p_cur_config = &mCustomizedVideoPostPConfiguration;
    } else {
        DASSERT(mCurrentConfiguration < DMaxDisplayPatternNumber);
        p_cur_config = &mVideoPostPConfiguration[mCurrentConfiguration];
    }

    DASSERT(p_cur_config->layer_number > p_cur_config->current_layer);
    if (layer < p_cur_config->layer_number) {

        err = updateToLayer(p_cur_config, (TU8)layer);
        DASSERT_OK(err);

        if (EECode_OK == err) {
            if (mpDSP) {
                err = mpDSP->DSPControl(EDSPControlType_UDEC_render_config, (void *)p_cur_config);
                DASSERT_OK(err);
            } else {
                LOGM_ERROR("BAD state mpDSP is NULL\n");
            }
        } else {
            return err;
        }
    } else {
        LOGM_ERROR("BAD layer %d, exceed max value %d\n", layer, p_cur_config->layer_number);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::SwitchWindowToHD(SVideoPostPDisplayHD *phd)
{
    SVideoPostPConfiguration *p_cur_config = &mVideoPostPConfiguration[mCurrentConfiguration];
    EECode err;
    TU8 check_layer_index = 0;

    AUTO_LOCK(mpMutex);

    LOGM_NOTICE("switch window %d to HD\n", phd->sd_window_index_for_hd);

    if (mpDSP) {
        p_cur_config->render[phd->sd_window_index_for_hd].udec_id = phd->tobeHD ? p_cur_config->layer_window_number[check_layer_index] : phd->sd_window_index_for_hd;
        err = mpDSP->DSPControl(EDSPControlType_UDEC_render_config, (void *)p_cur_config);
        DASSERT_OK(err);
    } else {
        LOGM_ERROR("BAD state mpDSP is NULL\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::configureRender(SVideoPostPConfiguration *p_cur_config, TU8 render_id, TU8 win_id, TU8 win_id_2rd, TU8 dec_id)
{
    TU8 use_2rd_win = 0;
    SDSPMUdecRender *p_render;

    //debug assert
    DASSERT(p_cur_config);
    if (!p_cur_config) {
        LOGM_FATAL("NULL p_cur_config\n");
        return EECode_InternalLogicalBug;
    }

    DASSERT(render_id < p_cur_config->total_render_number);
    DASSERT(win_id < p_cur_config->total_window_number);
    if (0xff != win_id_2rd) {
        LOGM_INFO("enable win_id_2rd %d\n", win_id_2rd);
        use_2rd_win = 1;
        DASSERT(win_id_2rd < p_cur_config->total_window_number);
    } else {
        use_2rd_win = 0;
    }
    DASSERT(dec_id < p_cur_config->total_decoder_number);

    if ((render_id < p_cur_config->total_render_number) && (win_id < p_cur_config->total_window_number) && (dec_id < p_cur_config->total_decoder_number)) {
        if (use_2rd_win && (win_id_2rd < p_cur_config->total_window_number)) {
            p_render = &p_cur_config->render[render_id];

            //render
            p_render->render_id = render_id;
            p_render->win_config_id = win_id;
            p_render->win_config_id_2nd = win_id_2rd;
            p_render->udec_id = dec_id;

            //window
            //            p_cur_config->window[win_id].render_id = render_id;
            p_cur_config->window[win_id].win_config_id = win_id;
            p_cur_config->window[win_id].input_offset_x = 0;
            p_cur_config->window[win_id].input_offset_y = 0;
            p_cur_config->window[win_id].input_width = p_cur_config->decoder[dec_id].max_frm_width;
            p_cur_config->window[win_id].input_height = p_cur_config->decoder[dec_id].max_frm_height;

            // 2rd window
            if (use_2rd_win) {
                //                p_cur_config->window[win_id_2rd].render_id = render_id;
                p_cur_config->window[win_id_2rd].win_config_id = win_id_2rd;
                p_cur_config->window[win_id_2rd].input_offset_x = 0;
                p_cur_config->window[win_id_2rd].input_offset_y = 0;
                p_cur_config->window[win_id_2rd].input_width = p_cur_config->decoder[dec_id].max_frm_width;
                p_cur_config->window[win_id_2rd].input_height = p_cur_config->decoder[dec_id].max_frm_height;
            }
        } else {
            LOGM_FATAL("BAD win_id_2rd %d\n", win_id_2rd);
            return EECode_InternalLogicalBug;
        }
    } else {
        LOGM_FATAL("BAD render id %d, win id %d, win id 2rd %d, dec id %d\n", render_id, win_id, win_id_2rd, dec_id);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::SwapContent(SVideoPostPSwap *swap)
{
    SVideoPostPConfiguration *p_cur_config = NULL;
    EECode err;
    TU8 i, dec_0 = 0xff, dec_1 = 0xff, render_0 = 0xff, render_1 = 0xff;

    AUTO_LOCK(mpMutex);

    DASSERT(swap);
    if (!swap) {
        LOGM_ERROR("NULL input param: swap\n");
        return EECode_BadParam;
    }

    LOGM_INFO("swap: win0_id %d, win1_id %d\n", swap->swap_win0_id, swap->swap_win1_id);
    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        p_cur_config = &mCustomizedVideoPostPConfiguration;
    } else {
        DASSERT(mCurrentConfiguration < DMaxDisplayPatternNumber);
        p_cur_config = &mVideoPostPConfiguration[mCurrentConfiguration];
    }

    DASSERT(swap->swap_win0_id < p_cur_config->total_window_number);
    DASSERT(swap->swap_win1_id < p_cur_config->total_window_number);
    DASSERT(swap->swap_win0_id != swap->swap_win1_id);

    if (swap->swap_win0_id == swap->swap_win1_id) {
        LOGM_WARN("swap win id is the same!\n");
        return EECode_BadParam;
    } else if ((swap->swap_win0_id >= p_cur_config->total_window_number) || (swap->swap_win1_id >= p_cur_config->total_window_number)) {
        LOGM_FATAL("BAD swap win id, win_0 %d, win_1 %d, p_cur_config->total_window_number %d!\n", swap->swap_win0_id, swap->swap_win1_id, p_cur_config->total_window_number);
        return EECode_BadParam;
    }

    for (i = 0; i < p_cur_config->total_render_number; i++) {
        if (p_cur_config->render[i].win_config_id == swap->swap_win0_id) {
            dec_0 = p_cur_config->render[i].udec_id;
            render_0 = i;
        } else if (p_cur_config->render[i].win_config_id == swap->swap_win1_id) {
            dec_1 = p_cur_config->render[i].udec_id;
            render_1 = i;
        }
    }

    DASSERT(0xff != dec_0);
    DASSERT(0xff != dec_1);
    DASSERT(0xff != render_0);
    DASSERT(0xff != render_1);
    DASSERT(dec_0 < p_cur_config->total_decoder_number);
    DASSERT(dec_1 < p_cur_config->total_decoder_number);
    DASSERT(render_0 < p_cur_config->total_render_number);
    DASSERT(render_1 < p_cur_config->total_render_number);

    if ((0xff != dec_0) && (0xff != dec_1)) {
        err = configureRender(p_cur_config, render_0, swap->swap_win0_id, 0xff, dec_1);
        DASSERT_OK(err);
        err = configureRender(p_cur_config, render_1, swap->swap_win1_id, 0xff, dec_0);
        DASSERT_OK(err);

        if (mpDSP) {
            err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (SDSPControlParams *)p_cur_config);
            DASSERT_OK(err);
        } else {
            LOGM_ERROR("BAD state mpDSP is NULL\n");
        }
    } else {
        LOGM_FATAL("BAD swap win id, win_0 %d, win_1 %d, p_cur_config->total_window_number %d!\n", swap->swap_win0_id, swap->swap_win1_id, p_cur_config->total_window_number);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::CircularShiftContent(SVideoPostPShift *shift)
{
    SVideoPostPConfiguration *p_cur_config = NULL;
    SDSPMUdecRender *p_render;
    SDSPMUdecRender render_1;
    SDSPMUdecRender render_reserved;
    //    EECode err;
    TU8 i, shift_count, shift_direction, render_start_index, render_number;
    TU8 render_index;

    AUTO_LOCK(mpMutex);

    DASSERT(shift);
    if (!shift) {
        LOGM_ERROR("NULL input param: shift\n");
        return EECode_BadParam;
    }

    LOGM_INFO("shift: count %d, direction %d\n", shift->shift_count, shift->shift_direction);
    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        p_cur_config = &mCustomizedVideoPostPConfiguration;
    } else {
        DASSERT(mCurrentConfiguration < DMaxDisplayPatternNumber);
        p_cur_config = &mVideoPostPConfiguration[mCurrentConfiguration];
    }

    shift_count = shift->shift_count;
    shift_direction = shift->shift_direction;
    render_start_index = p_cur_config->cur_render_start_index;
    render_number = p_cur_config->cur_render_number;

    if (render_number > 1) {
        p_render = &(p_cur_config->render[render_start_index]);

        render_reserved = *p_render;

        if (shift_direction) {
            for (i = 0; i < render_number; i ++) {
                if (shift_direction) {
                    render_index = render_start_index + ((i * shift_count) % render_number);
                } else {
                    render_index = render_start_index + ((i * (render_number - shift_count)) % render_number);
                }
                render_1 = p_cur_config->render[render_index];

                p_render->udec_id = render_1.udec_id;
            }
        } else {

        }
    }

#if 0
    DASSERT(swap->swap_win0_id < p_cur_config->total_window_number);
    DASSERT(swap->swap_win1_id < p_cur_config->total_window_number);
    DASSERT(swap->swap_win0_id != swap->swap_win1_id);

    if (swap->swap_win0_id == swap->swap_win1_id) {
        LOGM_WARN("swap win id is the same!\n");
        return EECode_BadParam;
    } else if ((swap->swap_win0_id >= p_cur_config->total_window_number) || (swap->swap_win1_id >= p_cur_config->total_window_number)) {
        LOGM_FATAL("BAD swap win id, win_0 %d, win_1 %d, p_cur_config->total_window_number %d!\n", swap->swap_win0_id, swap->swap_win1_id, p_cur_config->total_window_number);
        return EECode_BadParam;
    }

    for (i = 0; i < p_cur_config->total_render_number; i++) {
        if (p_cur_config->render[i].win_config_id == swap->swap_win0_id) {
            dec_0 = p_cur_config->render[i].udec_id;
            render_0 = i;
        } else if (p_cur_config->render[i].win_config_id == swap->swap_win1_id) {
            dec_1 = p_cur_config->render[i].udec_id;
            render_1 = i;
        }
    }

    DASSERT(0xff != dec_0);
    DASSERT(0xff != dec_1);
    DASSERT(0xff != render_0);
    DASSERT(0xff != render_1);
    DASSERT(dec_0 < p_cur_config->total_decoder_number);
    DASSERT(dec_1 < p_cur_config->total_decoder_number);
    DASSERT(render_0 < p_cur_config->total_render_number);
    DASSERT(render_1 < p_cur_config->total_render_number);

    if ((0xff != dec_0) && (0xff != dec_1)) {
        err = configureRender(p_cur_config, render_0, swap->swap_win0_id, 0xff, dec_1);
        DASSERT_OK(err);
        err = configureRender(p_cur_config, render_1, swap->swap_win1_id, 0xff, dec_0);
        DASSERT_OK(err);

        if (mpDSP) {
            err = mpDSP->DSPControl(EDSPControlType_UDEC_update_multiple_window_display, (void *)p_cur_config);
            DASSERT_OK(err);
        } else {
            LOGM_ERROR("BAD state mpDSP is NULL\n");
        }
    } else {
        LOGM_FATAL("BAD swap win id, win_0 %d, win_1 %d, p_cur_config->total_window_number %d!\n", swap->swap_win0_id, swap->swap_win1_id, p_cur_config->total_window_number);
        return EECode_BadParam;
    }
#endif
    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::StreamSwitch(SVideoPostPStreamSwitch *stream_switch)
{
    LOGM_WARN("not implement yet.\n");
    return EECode_NoImplement;
}

EECode CAmbaVideoPostProcessor::QueryCurrentWindow(TUint window_id, TUint &dec_id, TUint &render_id) const
{
    SVideoPostPConfiguration *p_cur_postp;
    SDSPMUdecRender *p_render;

    AUTO_LOCK(mpMutex);

    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        p_cur_postp = (SVideoPostPConfiguration *)&mCustomizedVideoPostPConfiguration;
    } else {
        p_cur_postp = (SVideoPostPConfiguration *)&mVideoPostPConfiguration[mCurrentConfiguration];
    }

    if (p_cur_postp) {
        dec_id = DINVALID_VALUE_TAG;
        render_id = DINVALID_VALUE_TAG;

        p_render = __findRenderViaWindow(&p_cur_postp->render[p_cur_postp->cur_render_start_index], (TUint)p_cur_postp->cur_render_number, window_id);
        if (p_render) {
            dec_id = p_render->udec_id;
            render_id = p_render->render_id;
        } else {
            LOGM_ERROR("BAD window id %d, do not find related render\n", window_id);
            return EECode_BadState;
        }

    } else {
        LOGM_FATAL("NULL p_cur_postp\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::QueryCurrentRender(TUint render_id, TUint &dec_id, TUint &window_id, TUint &window_id_2rd) const
{
    SVideoPostPConfiguration *p_cur_postp;
    SDSPMUdecRender *p_render;

    AUTO_LOCK(mpMutex);

    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        p_cur_postp = (SVideoPostPConfiguration *)&mCustomizedVideoPostPConfiguration;
    } else {
        p_cur_postp = (SVideoPostPConfiguration *)&mVideoPostPConfiguration[mCurrentConfiguration];
    }

    if (p_cur_postp) {
        dec_id = DINVALID_VALUE_TAG;
        window_id = DINVALID_VALUE_TAG;
        window_id_2rd = DINVALID_VALUE_TAG;

        p_render = __findRender(&p_cur_postp->render[p_cur_postp->cur_render_start_index], (TUint)p_cur_postp->cur_render_number, render_id);
        if (p_render) {
            dec_id = p_render->udec_id;
            window_id = p_render->win_config_id;
            window_id_2rd = p_render->win_config_id_2nd;
        } else {
            LOGM_ERROR("BAD render id %d, do not find related render\n", render_id);
            return EECode_BadState;
        }

    } else {
        LOGM_FATAL("NULL p_cur_postp\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoPostProcessor::QueryCurrentUDEC(TUint dec_id, TUint &window_id, TUint &render_id, TUint &window_id_2rd) const
{
    LOGM_WARN("not implement yet.\n");
    window_id = DINVALID_VALUE_TAG;
    render_id = DINVALID_VALUE_TAG;
    window_id_2rd = DINVALID_VALUE_TAG;
    return EECode_NoImplement;
}

EECode CAmbaVideoPostProcessor::QueryVideoPostPInfo(const SVideoPostPConfiguration *&postp_info) const
{
    AUTO_LOCK(mpMutex);

    if (mbUseCustomizedConfiguration) {
        DASSERT(mbCustomizedConfigurationSetup);
        postp_info = (const SVideoPostPConfiguration *)&mCustomizedVideoPostPConfiguration;
    } else {
        postp_info = (const SVideoPostPConfiguration *)&mVideoPostPConfiguration[mCurrentConfiguration];
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

