/*******************************************************************************
 * simple_api.cpp
 *
 * History:
 *    2013/12/28 - [Zhi He] create file
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

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "media_mw_if.h"

#include "simple_api.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;


CSimpleAPI::CSimpleAPI(TUint direct_access)
    : mpEngineControl(NULL)
    , mpContext(NULL)
    , mpMediaConfig(NULL)
    , mbStartRunning(0)
    , mbBuildGraghDone(0)
    , mbAssignContext(0)
    , mbDirectAccess(direct_access)
    , mpCommunicationBuffer(NULL)
    , mCommunicationBufferSize(0)
    , mSwitchLayeroutIdentifyerCount(0)
    , mbSmoothSwitchLayout(0)
    , mbSdInHdWindow(0)
    , mbHdInSdWindow(0)
    , mFocusIndex(0)
{
    memset(&mCurrentVideoPostPGlobalSetting, 0x0, sizeof(mCurrentVideoPostPGlobalSetting));
    memset(&mCurrentDisplayLayout, 0x0, sizeof(mCurrentDisplayLayout));
    memset(&mCurrentVideoPostPConfig, 0x0, sizeof(mCurrentVideoPostPConfig));

    memset(&mCurrentSyncStatus, 0x0, sizeof(mCurrentSyncStatus));
}

CSimpleAPI::~CSimpleAPI()
{
}

EECode CSimpleAPI::Construct()
{
    mpEngineControl = gfMediaEngineFactoryV2(EFactory_MediaEngineType_Generic);
    if (DUnlikely(!mpEngineControl)) {
        LOG_FATAL("gfMediaEngineFactoryV2 fail\n");
        return EECode_NoMemory;
    }

    EECode err = mpEngineControl->SetAppMsgCallback(MsgCallback, (void *) this);
    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("SetAppMsgCallback fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mCommunicationBufferSize = 4 * 1024;
    mpCommunicationBuffer = (TU8 *) malloc(mCommunicationBufferSize);

    if (DUnlikely(!mpCommunicationBuffer)) {
        LOG_FATAL("no memory, request %ld\n", mCommunicationBufferSize);
        mCommunicationBufferSize = 0;
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CSimpleAPI *CSimpleAPI::Create(TUint direct_access)
{
    CSimpleAPI *thiz = new CSimpleAPI(direct_access);
    if (DLikely(thiz)) {
        EECode err = thiz->Construct();
        if (DLikely(EECode_OK == err)) {
            return thiz;
        }
        delete thiz;
    }

    return NULL;
}

void CSimpleAPI::Destroy()
{
    if (mpEngineControl) {
        mpEngineControl->Destroy();
        mpEngineControl = NULL;
    }

    if (mpCommunicationBuffer) {
        free(mpCommunicationBuffer);
        mpCommunicationBuffer = NULL;
    }
    mCommunicationBufferSize = 0;

    this->~CSimpleAPI();
}

IGenericEngineControlV2 *CSimpleAPI::QueryEngineControl() const
{
    return mpEngineControl;
}

EECode CSimpleAPI::AssignContext(SSimpleAPIContxt *p_context)
{
    if (DUnlikely(!p_context)) {
        LOG_FATAL("NULL p_context\n");
        return EECode_BadParam;
    }

    if (DUnlikely(mbStartRunning || mbBuildGraghDone)) {
        LOG_FATAL("start running, or build gragh done, cannot invoke this API at this time\n");
        return EECode_BadState;
    }

    if (DUnlikely(mpContext || mbAssignContext)) {
        LOG_FATAL("mpContext %p, mbAssignContext %d, already set?\n", mpContext, mbAssignContext);
        return EECode_BadState;
    }

    if (mbDirectAccess) {
        mpContext = p_context;
    } else {
        mpContext = (SSimpleAPIContxt *) malloc(sizeof(SSimpleAPIContxt));
        if (!mpContext) {
            LOG_FATAL("No memory, request size %lu\n", (TULong)sizeof(SSimpleAPIContxt));
            return EECode_NoMemory;
        }
        memset(mpContext, 0x0, sizeof(SSimpleAPIContxt));
        *mpContext = *p_context;
    }
    mbAssignContext = 1;

    return EECode_OK;
}

EECode CSimpleAPI::StartRunning()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);
    EECode err = EECode_OK;

    if (!mbBuildGraghDone) {
        if (DUnlikely((!mbAssignContext) || (!mpContext))) {
            LOG_ERROR("need invoke AssignContext first\n");
            return EECode_BadState;
        }

        err = checkSetting();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("checkSetting return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = initialSetting();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("initialSetting return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (!mpContext->setting.disable_dsp_related) {
            err = setupInitialMediaConfig();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("setupInitialMediaConfig return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            err = activateDevice();
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("activateDevice return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            mpEngineControl->SetMediaConfig();
        }

        err = buildDataProcessingGragh();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("buildDataProcessingGragh return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        initializeSyncStatus();
    } else {
        DASSERT(mbAssignContext);
        DASSERT(mpContext);
    }

    if (DLikely(mpEngineControl)) {
        err = mpEngineControl->StartProcess();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("StartProcess return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        mbStartRunning = 1;

        err = setUrls();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("setUrls return %d, %s\n", err, gfGetErrorCodeString(err));
            //return err;
        }

    } else {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPI::ExitRunning()
{
    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (DLikely(mpEngineControl && mbStartRunning)) {
        mbStartRunning = 0;
        EECode err = mpEngineControl->StopProcess();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("StopProcess return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = deActivateDevice();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("deActivateDevice return %d, %s\n", err, gfGetErrorCodeString(err));
        }

    } else {
        LOG_ERROR("NULL mpEngineControl %p, or not started %d yet\n", mpEngineControl, mbStartRunning);
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPI::Control(EUserParamType type, void *param)
{
    EECode err = EECode_OK;

    switch (type) {

        case EUserParamType_PlaybackCapture: {
                SUserParamType_PlaybackCapture *capture = (SUserParamType_PlaybackCapture *) param;
                err = processPlaybackCapture(capture);
            }
            break;

        case EUserParamType_PlaybackTrickplay: {
                SUserParamType_PlaybackTrickplay *trickplay = (SUserParamType_PlaybackTrickplay *) param;
                err = processPlaybackTrickplay(trickplay);
            }
            break;

#if 0
        case EUserParamType_PlaybackFastFwBw:
            break;

        case EUserParamType_PlaybackSeek:
            break;
#endif

        case EUserParamType_QueryStatus: {
                SUserParamType_QueryStatus *status = (SUserParamType_QueryStatus *)param;
                if (DLikely(status)) {
                    DASSERT(EUserParamType_QueryStatus == status->check_field);
                    status->total_window_number = mCurrentSyncStatus.total_window_number;
                    status->current_display_window_number = mCurrentSyncStatus.current_display_window_number;
                    status->enc_w = mCurrentSyncStatus.encoding_width;
                    status->enc_h = mCurrentSyncStatus.encoding_height;
                    status->vout_w = mpMediaConfig->dsp_config.modeConfigMudec.video_win_width;
                    status->vout_h = mpMediaConfig->dsp_config.modeConfigMudec.video_win_height;
                    if (status->current_display_window_number > DQUERY_MAX_DISPLAY_WINDOW_NUMBER
                            || status->current_display_window_number > DQUERY_MAX_SOURCE_NUMBER) {
                        LOG_ERROR("current_display_window_number %u out of range\n", status->current_display_window_number);
                        break;
                    }
                    if (EDisplayLayout_SingleWindowView == mCurrentSyncStatus.display_layout) {
                        status->window[0].udec_index = mCurrentSyncStatus.window[0].udec_index;
                        status->window[0].render_index = mCurrentSyncStatus.window[0].render_index;
                        status->window[0].window_pos_x = mCurrentSyncStatus.window[0].window_pos_x;
                        status->window[0].window_pos_y = mCurrentSyncStatus.window[0].window_pos_y;
                        status->window[0].window_width = mCurrentSyncStatus.window[0].window_width;
                        status->window[0].window_height = mCurrentSyncStatus.window[0].window_height;
                        status->window[0].source_width = mCurrentSyncStatus.source_info[mCurrentSyncStatus.channel_number_per_group].pic_width;
                        status->window[0].source_height = mCurrentSyncStatus.source_info[mCurrentSyncStatus.channel_number_per_group].pic_height;
                    } else if (EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) {
                        status->window[0].udec_index = mCurrentSyncStatus.window[0].udec_index;//currently in telepresence, window 0 is HD
                        status->window[0].render_index = mCurrentSyncStatus.window[0].render_index;
                        status->window[0].window_pos_x = mCurrentSyncStatus.window[0].window_pos_x;
                        status->window[0].window_pos_y = mCurrentSyncStatus.window[0].window_pos_y;
                        status->window[0].window_width = mCurrentSyncStatus.window[0].window_width;
                        status->window[0].window_height = mCurrentSyncStatus.window[0].window_height;
                        status->window[0].source_width = mCurrentSyncStatus.source_info[mCurrentSyncStatus.channel_number_per_group].pic_width;
                        status->window[0].source_height = mCurrentSyncStatus.source_info[mCurrentSyncStatus.channel_number_per_group].pic_height;
                        for (TInt i = 1; i < status->current_display_window_number; i++) {
                            status->window[i].udec_index = mCurrentSyncStatus.window[i].udec_index;
                            status->window[i].render_index = mCurrentSyncStatus.window[i].render_index;
                            status->window[i].window_pos_x = mCurrentSyncStatus.window[i].window_pos_x;
                            status->window[i].window_pos_y = mCurrentSyncStatus.window[i].window_pos_y;
                            status->window[i].window_width = mCurrentSyncStatus.window[i].window_width;
                            status->window[i].window_height = mCurrentSyncStatus.window[i].window_height;
                            status->window[i].source_width = mCurrentSyncStatus.source_info[i].pic_width;
                            status->window[i].source_height = mCurrentSyncStatus.source_info[i].pic_height;
                        }
                    } else {//EDisplayLayout_Rectangle  or  EDisplayLayout_BottomLeftHighLighten
                        for (TInt i = 0; i < status->current_display_window_number; i++) {
                            status->window[i].udec_index = mCurrentSyncStatus.window[i].udec_index;
                            status->window[i].render_index = mCurrentSyncStatus.window[i].render_index;
                            status->window[i].window_pos_x = mCurrentSyncStatus.window[i].window_pos_x;
                            status->window[i].window_pos_y = mCurrentSyncStatus.window[i].window_pos_y;
                            status->window[i].window_width = mCurrentSyncStatus.window[i].window_width;
                            status->window[i].window_height = mCurrentSyncStatus.window[i].window_height;
                            status->window[i].source_width = mCurrentSyncStatus.source_info[i].pic_width;
                            status->window[i].source_height = mCurrentSyncStatus.source_info[i].pic_height;
                        }
                    }
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_VideoPlayback_Zoom_OnDisplay: {
                SUserParamType_Zoom_OnDisplay *zoom = (SUserParamType_Zoom_OnDisplay *)param;
                if (DLikely(zoom)) {
                    SPlaybackZoomOnDisplay playback_zoom;
                    playback_zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay;
                    playback_zoom.render_id = zoom->render_id;
                    playback_zoom.decoder_id = zoom->decoder_id;
                    playback_zoom.input_center_x = zoom->input_center_x;
                    playback_zoom.input_center_y = zoom->input_center_y;
                    playback_zoom.input_width = zoom->input_width;
                    playback_zoom.input_height =  zoom->input_height;
                    playback_zoom.window_width = zoom->window_width;
                    playback_zoom.window_height =  zoom->window_height;

                    if (DLikely(mpEngineControl)) {
                        EECode err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay, (void *)&playback_zoom);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_ERROR("EGenericEngineConfigure_VideoPlayback_Zoom_OnDisplay ret %d, %s\n", err, gfGetErrorCodeString(err));
                            return err;
                        }
                    } else {
                        LOG_FATAL("NULL mpEngineControl\n");
                        return EECode_BadParam;
                    }
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_UpdateDisplayLayout: {
                SUserParamType_UpdateDisplayLayout *layout = (SUserParamType_UpdateDisplayLayout *)param;
                if (DLikely(layout)) {
                    DASSERT(EUserParamType_UpdateDisplayLayout == layout->check_field);
                    if (mCurrentSyncStatus.have_dual_stream) {
                        err = processUpdateMudecDisplayLayout(layout->layout, layout->focus_stream_index);
                    } else {
                        err = processUpdateMudecDisplayLayout_no_dual_stream(layout->layout, layout->focus_stream_index);
                    }
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processUpdateMudecDisplayLayout(%d, %d), ret %d, %s\n", layout->layout, layout->focus_stream_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_DisplayZoom: {
                SUserParamType_DisplayZoom *zoom = (SUserParamType_DisplayZoom *)param;
                if (DLikely(zoom)) {
                    DASSERT(EUserParamType_DisplayZoom == zoom->check_field);
                    //fix point
                    TU32 fixed_point_zoom_factor = 0;
                    TU32 fixed_point_offset_x = 0;
                    TU32 fixed_point_offset_y = 0;

                    if (0 < zoom->zoom_factor) {
                        fixed_point_zoom_factor = zoom->zoom_factor * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_zoom_factor = (0 - zoom->zoom_factor) * DSACP_FIX_POINT_DEN;
                        fixed_point_zoom_factor |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    if (0 < zoom->gesture_offset_x) {
                        fixed_point_offset_x = zoom->gesture_offset_x * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_offset_x = (0 - zoom->gesture_offset_x) * DSACP_FIX_POINT_DEN;
                        fixed_point_offset_x |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    if (0 < zoom->gesture_offset_y) {
                        fixed_point_offset_y = zoom->gesture_offset_y * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_offset_y = (0 - zoom->gesture_offset_y) * DSACP_FIX_POINT_DEN;
                        fixed_point_offset_y |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    err = processZoom(zoom->window_id, fixed_point_zoom_factor, fixed_point_offset_x, fixed_point_offset_y);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processZoom(%d, %f, %f, %f), ret %d, %s\n", zoom->window_id, zoom->zoom_factor, zoom->gesture_offset_x, zoom->gesture_offset_y, err, gfGetErrorCodeString(err));
                        return err;
                    }
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_MoveWindow: {
                SUserParamType_MoveWindow *window = (SUserParamType_MoveWindow *)param;
                if (DLikely(window)) {
                    DASSERT(EUserParamType_MoveWindow == window->check_field);
                    //fix point
                    TU32 fixed_point_size_x = 0;
                    TU32 fixed_point_size_y = 0;
                    TU32 fixed_point_offset_x = 0;
                    TU32 fixed_point_offset_y = 0;

                    if (0 < window->offset_x) {
                        fixed_point_offset_x = window->offset_x * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_offset_x = (0 - window->offset_x) * DSACP_FIX_POINT_DEN;
                        fixed_point_offset_x |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    if (0 < window->offset_y) {
                        fixed_point_offset_y = window->offset_y * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_offset_y = (0 - window->offset_y) * DSACP_FIX_POINT_DEN;
                        fixed_point_offset_y |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    if (0 < window->size_x) {
                        fixed_point_size_x = window->size_x * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_size_x = (0 - window->size_x) * DSACP_FIX_POINT_DEN;
                        fixed_point_size_x |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    if (0 < window->size_y) {
                        fixed_point_size_y = window->size_y * DSACP_FIX_POINT_DEN;
                    } else {
                        fixed_point_size_y = (0 - window->size_y) * DSACP_FIX_POINT_DEN;
                        fixed_point_size_y |= DSACP_FIX_POINT_SYGN_FLAG;
                    }

                    err = processMoveWindow(window->window_id, fixed_point_offset_x, fixed_point_offset_y, fixed_point_size_x, fixed_point_offset_y);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processMoveWindow(%d, %f, %f, %f, %f), ret %d, %s\n", window->window_id, window->offset_x, window->offset_y, window->size_x, window->size_y, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_SwapWindowContent: {
                SUserParamType_SwapWindowContent *swap = (SUserParamType_SwapWindowContent *)param;
                if (DLikely(swap)) {
                    DASSERT(EUserParamType_SwapWindowContent == swap->check_field);
                    if (mCurrentSyncStatus.have_dual_stream) {
                        err = processSwapWindowContent(swap->window_id_0, swap->window_id_1);
                    } else {
                        err = processSwapWindowContent_no_dual_stream(swap->window_id_0, swap->window_id_1);
                    }

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processSwapWindowContent(%d, %d), ret %d, %s\n", swap->window_id_0, swap->window_id_1, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_DisplayShowOtherWindows: {
                SUserParamType_DisplayShowOtherWindows *display = (SUserParamType_DisplayShowOtherWindows *)param;
                if (DLikely(display)) {
                    DASSERT(EUserParamType_DisplayShowOtherWindows == display->check_field);

                    err = processShowOtherWindows(display->window_id, display->show_others);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processShowOtherWindows(%d, %d), ret %d, %s\n", display->window_id, display->show_others, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_SwitchGroup: {
                SUserParamType_SwitchGroup *switch_group = (SUserParamType_SwitchGroup *)param;
                if (DLikely(switch_group)) {
                    DASSERT(EUserParamType_SwitchGroup == switch_group->check_field);

                    if (mCurrentSyncStatus.have_dual_stream) {
                        err = processSwitchGroup(switch_group->group_id, switch_group->group_type);
                    } else {
                        err = processSwitchGroup_no_dual_stream(switch_group->group_id, switch_group->group_type);
                    }
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processSwitchGroup(%d, %d), ret %d, %s\n", switch_group->group_id, switch_group->group_type, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AddGroup: {
                SUserParamType_AddGroup *add_group = (SUserParamType_AddGroup *)param;
                if (DLikely(add_group && mpEngineControl)) {
                    TUint group_index = add_group->group_id;
                    TUint i = 0;
                    SGroupGragh *p_group_gragh = &mpContext->gragh_component[group_index];
                    SGroupUrl *p_group_url = &mpContext->group[group_index];
                    p_group_url->source_2rd_channel_number = add_group->source_2rd_channel_number;
                    p_group_url->source_channel_number = add_group->source_channel_number;
                    //set source urls
                    for (i = 0; i < p_group_url->source_2rd_channel_number; i ++) {
                        if (p_group_gragh->demuxer_2rd_id[i]) {
                            snprintf(&p_group_url->source_url_2rd[i][0], DMaxUrlLen, "%s", add_group->source_url_2rd[i]);
                            TChar *url = p_group_url->source_url_2rd[i];
                            if (strlen(url)) {
                                err = mpEngineControl->SetSourceUrl(p_group_gragh->demuxer_2rd_id[i], url);
                                LOG_PRINTF("SetSourceUrl 0x%08x, %s, ret %d %s\n", p_group_gragh->demuxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                            }
                        }
                    }

                    for (i = 0; i < p_group_url->source_channel_number; i ++) {
                        if (p_group_gragh->demuxer_id[i]) {
                            snprintf(&p_group_url->source_url[i][0], DMaxUrlLen, "%s", add_group->source_url[i]);
                            TChar *url = p_group_url->source_url[i];
                            if (strlen(url)) {
                                err = mpEngineControl->SetSourceUrl(p_group_gragh->demuxer_id[i], url);
                                LOG_PRINTF("SetSourceUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->demuxer_id[i], url, err, gfGetErrorCodeString(err));
                            }
                        }
                    }

                } else {
                    LOG_ERROR("NULL param, add_group=%p, mpEngineControl=%p\n", add_group, mpEngineControl);
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_QueryRecordedTimeline: {
                SUserParamType_QueryRecordedTimeline *query = (SUserParamType_QueryRecordedTimeline *)param;
                if (DLikely(query)) {
                    DASSERT(EUserParamType_QueryRecordedTimeline == query->check_field);

                    LOG_ERROR("not supported yet\n");
                    return EECode_NotSupported;
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioUpdateTargetMask: {
                SUserParamType_AudioUpdateTargetMask *audio = (SUserParamType_AudioUpdateTargetMask *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioUpdateTargetMask == audio->check_field);

                    LOG_ERROR("not supported yet\n");
                    return EECode_NotSupported;
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioUpdateSourceMask: {
                SUserParamType_AudioUpdateSourceMask *audio = (SUserParamType_AudioUpdateSourceMask *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioUpdateSourceMask == audio->check_field);

                    LOG_ERROR("not supported yet\n");
                    return EECode_NotSupported;
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioStartTalkTo: {
                SUserParamType_AudioStartTalkTo *audio = (SUserParamType_AudioStartTalkTo *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioStartTalkTo == audio->check_field);

                    if (DUnlikely(0 == mpContext->setting.enable_audio_capture)) {
                        LOG_ERROR("EUserParamType_AudioStartTalkTo, audio_capture disabled, can not start talk to camera.\n");
                        break;
                    }
                    if (DUnlikely(!mpEngineControl)) {
                        LOG_ERROR("EUserParamType_AudioStartTalkTo, NULL mpEngineControl.\n");
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStartTalkTo: resume native_capture_pipeline start\n");
                    err = mpEngineControl->ResumePipeline(mpContext->native_capture_pipeline);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EUserParamType_AudioStartTalkTo: resume native_capture_pipeline fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStartTalkTo: resume native_capture_pipeline end\n");
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioStopTalkTo: {
                SUserParamType_AudioStopTalkTo *audio = (SUserParamType_AudioStopTalkTo *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioStopTalkTo == audio->check_field);

                    if (DUnlikely(0 == mpContext->setting.enable_audio_capture)) {
                        LOG_ERROR("EUserParamType_AudioStopTalkTo, audio_capture disabled, can not stop talk to camera.\n");
                        break;
                    }
                    if (DUnlikely(!mpEngineControl)) {
                        LOG_ERROR("EUserParamType_AudioStopTalkTo, NULL mpEngineControl.\n");
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStopTalkTo: suspend native_capture_pipeline start\n");
                    err = mpEngineControl->SuspendPipeline(mpContext->native_capture_pipeline, 0);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EUserParamType_AudioStopTalkTo: suspend native_capture_pipeline fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }
                    DASSERT(mpContext);
                    if (DLikely(mpContext)) {
                        TUint index = 0;
                        SGroupGragh *current_group = &mpContext->gragh_component[mCurrentSyncStatus.current_group];

                        for (index = 0; index < DMaxChannelNumberInGroup; index ++) {
                            if (current_group->native_push_pipeline_connected[index]) {
                                err = mpEngineControl->ResumePipeline(current_group->native_push_pipeline_id[index], 1, 0);
                                if (DUnlikely(EECode_OK != err)) {
                                    LOG_ERROR("ResumePipeline(native push)[index = %d, group %d](%08x) ret %d, %s\n", index, mCurrentSyncStatus.current_group, current_group->native_push_pipeline_id[index], err, gfGetErrorCodeString(err));
                                }
                            }

                            if (current_group->cloud_pipeline_connected[index]) {
                                err = mpEngineControl->SuspendPipeline(current_group->cloud_pipeline_id[index], 0);
                                if (DUnlikely(EECode_OK != err)) {
                                    LOG_ERROR("SuspendPipeline(cloud)[index = %d, group %d](%08x) ret %d, %s\n", index, mCurrentSyncStatus.current_group, current_group->cloud_pipeline_id[index], err, gfGetErrorCodeString(err));
                                }
                            }

                        }
                    }
                    LOG_INFO("EUserParamType_AudioStopTalkTo: suspend native_capture_pipeline end\n");
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioStartListenFrom: {
                SUserParamType_AudioStartListenFrom *audio = (SUserParamType_AudioStartListenFrom *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioStartListenFrom == audio->check_field);

                    if (DUnlikely(!mpEngineControl)) {
                        LOG_ERROR("EUserParamType_AudioStartListenFrom, NULL mpEngineControl.\n");
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStartListenFrom: EGenericEngineConfigure_AudioPlayback_UnMute start\n");
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_AudioPlayback_UnMute, NULL);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EUserParamType_AudioStartListenFrom: EGenericEngineConfigure_AudioPlayback_UnMute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }

                    if (DLikely(mpContext)) {
                        TUint index = 0;
                        SGroupGragh *current_group = &mpContext->gragh_component[mCurrentSyncStatus.current_group];

                        for (index = 0; index < DMaxChannelNumberInGroup; index ++) {
                            if (current_group->native_push_pipeline_connected[index]) {
                                err = mpEngineControl->SuspendPipeline(current_group->native_push_pipeline_id[index], 0);
                                if (DUnlikely(EECode_OK != err)) {
                                    LOG_ERROR("SuspendPipeline(native push)[index = %d, group %d](%08x) ret %d, %s\n", index, mCurrentSyncStatus.current_group, current_group->native_push_pipeline_id[index], err, gfGetErrorCodeString(err));
                                }
                            }

                            if (current_group->cloud_pipeline_connected[index]) {
                                err = mpEngineControl->ResumePipeline(current_group->cloud_pipeline_id[index], 1, 0);
                                if (DUnlikely(EECode_OK != err)) {
                                    LOG_ERROR("ResumePipeline(cloud)[index = %d, group %d](%08x) ret %d, %s\n", index, mCurrentSyncStatus.current_group, current_group->cloud_pipeline_id[index], err, gfGetErrorCodeString(err));
                                }
                            }

                        }
                    }

                    LOG_INFO("EUserParamType_AudioStartListenFrom: EGenericEngineConfigure_AudioPlayback_UnMute end\n");
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_AudioStopListenFrom: {
                SUserParamType_AudioStopListenFrom *audio = (SUserParamType_AudioStopListenFrom *)param;
                if (DLikely(audio)) {
                    DASSERT(EUserParamType_AudioStopListenFrom == audio->check_field);

                    if (DUnlikely(!mpEngineControl)) {
                        LOG_ERROR("EUserParamType_AudioStopListenFrom, NULL mpEngineControl.\n");
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStopListenFrom: EGenericEngineConfigure_AudioPlayback_Mute start\n");
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_AudioPlayback_Mute, NULL);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EUserParamType_AudioStopListenFrom: EGenericEngineConfigure_AudioPlayback_Mute fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }
                    LOG_INFO("EUserParamType_AudioStopListenFrom: EGenericEngineConfigure_AudioPlayback_Mute end\n");
                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_VideoTranscoderUpdateBitrate: {
                SUserParamType_VideoTranscoderUpdateBitrate *p_param = (SUserParamType_VideoTranscoderUpdateBitrate *)param;
                if (DLikely(p_param)) {
                    DASSERT(EUserParamType_VideoTranscoderUpdateBitrate == p_param->check_field);

                    err = processUpdateBitrate(p_param->bitrate);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processUpdateBitrate(%d), ret %d, %s\n", p_param->bitrate, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_VideoTranscoderUpdateFramerate: {
                SUserParamType_VideoTranscoderUpdateFramerate *p_param = (SUserParamType_VideoTranscoderUpdateFramerate *)param;
                if (DLikely(p_param)) {
                    DASSERT(EUserParamType_VideoTranscoderUpdateFramerate == p_param->check_field);

                    err = processUpdateFramerate(p_param->framerate);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processUpdateFramerate(%d), ret %d, %s\n", p_param->framerate, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        case EUserParamType_VideoTranscoderDemandIDR: {
                SUserParamType_VideoTranscoderDemandIDR *p_param = (SUserParamType_VideoTranscoderDemandIDR *)param;
                if (DLikely(p_param)) {
                    DASSERT(EUserParamType_VideoTranscoderDemandIDR == p_param->check_field);

                    err = processDemandIDR(p_param->param0);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("processDemandIDR(%d), ret %d, %s\n", p_param->param0, err, gfGetErrorCodeString(err));
                        return err;
                    }

                } else {
                    LOG_ERROR("NULL param\n");
                    return EECode_BadParam;
                }
            }
            break;

        default:
            LOG_FATAL("unknown user cmd type %d\n", type);
            return EECode_NotSupported;
            break;

    }

    return err;
}

void CSimpleAPI::MsgCallback(void *context, SMSG &msg)
{
    if (DUnlikely(!context)) {
        LOG_FATAL("NULL context\n");
        return;
    }

    CSimpleAPI *thiz = (CSimpleAPI *) context;
    EECode err = thiz->ProcessMsg(msg);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("ProcessMsg(msg) return %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return;
}

EECode CSimpleAPI::ProcessMsg(SMSG &msg)
{
    EECode err = EECode_OK;

    switch (msg.code) {

            //internal msg
        case EMSGType_StorageError:
            LOG_ERROR("EMSGType_StorageError TO DO\n");
            break;

        case EMSGType_SystemError:
            LOG_ERROR("EMSGType_SystemError TO DO\n");
            break;

        case EMSGType_IOError:
            LOG_ERROR("EMSGType_IOError TO DO\n");
            break;

        case EMSGType_DeviceError:
            LOG_ERROR("EMSGType_DeviceError TO DO\n");
            break;

        case EMSGType_StreamingError_TCPSocketConnectionClose:
            LOG_ERROR("EMSGType_StreamingError_TCPSocketConnectionClose TO DO\n");
            break;

        case EMSGType_StreamingError_UDPSocketInvalidArgument:
            LOG_ERROR("EMSGType_StreamingError_UDPSocketInvalidArgument TO DO\n");
            break;

        case EMSGType_NotifyNewFileGenerated:
            LOG_ERROR("EMSGType_NotifyNewFileGenerated TO DO\n");
            break;

        case EMSGType_NotifyUDECStateChanges:
            processUdecStatusUpdateMsg(msg);
            break;

        case EMSGType_NotifyUDECUpdateResolution:
            processUdecResolutionUpdateMsg(msg);
            break;

            //external cmd:
        case ECMDType_CloudSourceClient_UnknownCmd:
            LOG_ERROR("ECMDType_CloudSourceClient_UnknownCmd, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_UpdateBitrate:
            LOG_NOTICE("ECMDType_CloudSinkClient_UpdateBitrate, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processUpdateBitrate((TU32)msg.p0);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processUpdateBitrate(%d) ret %d, %s\n", (TU32)msg.p0, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_UpdateFrameRate:
            LOG_NOTICE("ECMDType_CloudSinkClient_UpdateFrameRate, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processUpdateFramerate((TU32)msg.p0);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processUpdateFramerate(%d) ret %d, %s\n", (TU32)msg.p0, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_UpdateDisplayLayout:
            LOG_NOTICE("ECMDType_CloudSinkClient_UpdateDisplayLayout, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));

            if (mCurrentSyncStatus.have_dual_stream) {
                err = processUpdateMudecDisplayLayout((TU32)msg.p0, (TU32)msg.p1);
            } else {
                err = processUpdateMudecDisplayLayout_no_dual_stream((TU32)msg.p0, (TU32)msg.p1);
            }

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processUpdateMudecDisplayLayout(%d, %d) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_SelectAudioSource:
            LOG_NOTICE("ECMDType_CloudSinkClient_SelectAudioSource, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_SelectAudioTarget:
            LOG_NOTICE("ECMDType_CloudSinkClient_SelectAudioTarget, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_Zoom:
            LOG_NOTICE("ECMDType_CloudSinkClient_Zoom, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processZoom((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processZoom(id %d, 0x%08x, 0x%08x) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_ZoomV2:
            LOG_NOTICE("ECMDType_CloudSinkClient_ZoomV2, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processZoom((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processZoom(id %d, 0x%08x, 0x%08x, 0x%08x, 0x%08x) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_UpdateDisplayWindow:
            LOG_NOTICE("ECMDType_CloudSinkClient_UpdateDisplayWindow, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processMoveWindow((TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processMoveWindow(%d, %08x, %08x, %08x, %08x) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_SelectVideoSource:
            LOG_NOTICE("ECMDType_CloudSinkClient_SelectVideoSource, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudSinkClient_ShowOtherWindows:
            LOG_NOTICE("ECMDType_CloudSinkClient_ShowOtherWindows, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processShowOtherWindows((TU32)msg.p0, (TU32)msg.p1);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processShowOtherWindows(%d, %d) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_DemandIDR:
            LOG_NOTICE("ECMDType_CloudSinkClient_DemandIDR, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processDemandIDR((TU32)msg.p0);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processDemandIDR(%d) ret %d, %s\n", (TU32)msg.p0, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_SwapWindowContent:
            LOG_NOTICE("ECMDType_CloudSinkClient_SwapWindowContent, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));

            if (mCurrentSyncStatus.have_dual_stream) {
                err = processSwapWindowContent((TU32)msg.p0, (TU32)msg.p1);
            } else {
                err = processSwapWindowContent_no_dual_stream((TU32)msg.p0, (TU32)msg.p1);
            }
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processSwapWindowContent(%d, %d) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_CircularShiftWindowContent:
            LOG_NOTICE("ECMDType_CloudSinkClient_CircularShiftWindowContent, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            err = processCircularShiftWindowContent((TU32)msg.p0, (TU32)msg.p1);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processCircularShiftWindowContent(%d, %d) ret %d, %s\n", (TU32)msg.p0, (TU32)msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudSinkClient_SwitchGroup:
            LOG_NOTICE("ECMDType_CloudSinkClient_CircularShiftWindowContent, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            if (mCurrentSyncStatus.have_dual_stream) {
                err = processSwitchGroup((TU32)msg.p0, (TU32)msg.p1);
            } else {
                err = processSwitchGroup_no_dual_stream((TU32)msg.p0, (TU32)msg.p1);
            }
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("processSwitchGroup(%ld, %ld), ret %d, %s\n", msg.p0, msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudClient_PeerClose:
            LOG_NOTICE("ECMDType_CloudClient_PeerClose, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudClient_QueryVersion:
            LOG_NOTICE("ECMDType_CloudClient_QueryVersion, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            DASSERT(msg.p_agent_pointer);
            err = sendCloudAgentVersion((ICloudServerAgent *)msg.p_agent_pointer, (TU32)msg.p0, (TU32)msg.p1);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("sendCloudAgentVersion(%p, %08x, %08x) ret %d, %s\n", (ICloudServerAgent *)msg.p_agent_pointer, (TU32)msg.p0, (TU32)msg.p1, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudClient_QueryStatus:
            LOG_NOTICE("ECMDType_CloudClient_QueryStatus, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            DASSERT(msg.p_agent_pointer);
            err = sendSyncStatus((ICloudServerAgent *)msg.p_agent_pointer);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("sendSyncStatus(%p) ret %d, %s\n", (ICloudServerAgent *)msg.p_agent_pointer, err, gfGetErrorCodeString(err));
                return err;
            }
            break;

        case ECMDType_CloudClient_SyncStatus:
            LOG_NOTICE("ECMDType_CloudClient_SyncStatus, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            break;

        case ECMDType_CloudClient_CustomizedCommand:
            LOG_NOTICE("ECMDType_CloudClient_CustomizedCommand, owner_index %d, type %d, %s\n", msg.owner_index, msg.owner_type, gfGetComponentStringFromGenericComponentType(msg.owner_type));
            LOG_NOTICE("p0 %d, p1 %08x, p2 %08x, p3 %08x, p4 %08x\n", (TU32)msg.p0, (TU32)msg.p1, (TU32)msg.p2, (TU32)msg.p3, (TU32)msg.p4);
            break;

        default:
            LOG_ERROR("unknown msg code %d\n", msg.code);
            return EECode_NotSupported;
            break;
    }

    return EECode_OK;
}

EECode CSimpleAPI::checkSetting()
{
    if (DUnlikely(EMediaDeviceWorkingMode_Invalid == mpContext->setting.work_mode)) {
        LOG_ERROR("mpContext->setting.work_mode not set\n");
        return EECode_BadParam;
    }

    if (DUnlikely(EPlaybackMode_Invalid == mpContext->setting.playback_mode)) {
        LOG_ERROR("mpContext->setting.playback_mode not set\n");
        return EECode_BadParam;
    }

    if (DUnlikely(1 < mpContext->setting.udec_instance_number)) {
        LOG_ERROR("mpContext->setting.udec_instance_number %d, exceed 1? now only support 1x1080p\n", mpContext->setting.udec_instance_number);
        return EECode_BadParam;
    }

    if (DUnlikely(9 < mpContext->setting.udec_2rd_instance_number)) {
        LOG_ERROR("mpContext->setting.udec_2rd_instance_number %d, exceed 9? now only support 9x480p\n", mpContext->setting.udec_2rd_instance_number);
        return EECode_BadParam;
    }

    if (DUnlikely(DMaxGroupNumber < mpContext->group_number)) {
        LOG_ERROR("mpContext->group_number %d, exceed %d?\n", mpContext->group_number, DMaxGroupNumber);
        return EECode_BadParam;
    }

    TUint index = 0;
    for (index = 0; index < mpContext->group_number; index ++) {

        if (DMaxChannelNumberInGroup < mpContext->group[index].source_channel_number) {
            LOG_ERROR("mpContext->group[%d].source_channel_number %d, exceed %d?\n", index, mpContext->group[index].source_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].source_2rd_channel_number) {
            LOG_ERROR("mpContext->group[%d].source_2rd_channel_number %d, exceed %d?\n", index, mpContext->group[index].source_2rd_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].sink_channel_number) {
            LOG_ERROR("mpContext->group[%d].sink_channel_number %d, exceed %d?\n", index, mpContext->group[index].sink_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].sink_2rd_channel_number) {
            LOG_ERROR("mpContext->group[%d].sink_2rd_channel_number %d, exceed %d?\n", index, mpContext->group[index].sink_2rd_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].streaming_channel_number) {
            LOG_ERROR("mpContext->group[%d].streaming_channel_number %d, exceed %d?\n", index, mpContext->group[index].streaming_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].streaming_2rd_channel_number) {
            LOG_ERROR("mpContext->group[%d].streaming_2rd_channel_number %d, exceed %d?\n", index, mpContext->group[index].streaming_2rd_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }

        if (DMaxChannelNumberInGroup < mpContext->group[index].cloud_channel_number) {
            LOG_ERROR("mpContext->group[%d].cloud_channel_number %d, exceed %d?\n", index, mpContext->group[index].cloud_channel_number, DMaxChannelNumberInGroup);
            return EECode_BadParam;
        }
    }

    return EECode_OK;
}

EECode CSimpleAPI::initialSetting()
{
    EECode err = EECode_OK;
    SGenericPreConfigure pre_config;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (mpContext->setting.enable_force_log_level) {
        err = mpEngineControl->SetEngineLogLevel((ELogLevel)mpContext->setting.force_log_level);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetEngineLogLevel(%d), err %d, %s\n", mpContext->setting.force_log_level, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.net_receiver_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber;
        pre_config.number = mpContext->setting.net_receiver_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_NetworkReceiverSchedulerNumber, %d), err %d, %s\n", mpContext->setting.net_receiver_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.net_sender_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber;
        pre_config.number = mpContext->setting.net_sender_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_NetworkTransmiterSchedulerNumber, %d), err %d, %s\n", mpContext->setting.net_sender_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.fileio_reader_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_IOReaderSchedulerNumber;
        pre_config.number = mpContext->setting.fileio_reader_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_IOReaderSchedulerNumber, %d), err %d, %s\n", mpContext->setting.fileio_reader_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.fileio_writer_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_IOWriterSchedulerNumber;
        pre_config.number = mpContext->setting.fileio_writer_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_IOWriterSchedulerNumber, %d), err %d, %s\n", mpContext->setting.fileio_writer_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.video_decoder_scheduler_number) {
        pre_config.check_field = EGenericEnginePreConfigure_VideoDecoderSchedulerNumber;
        pre_config.number = mpContext->setting.video_decoder_scheduler_number;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_VideoDecoderSchedulerNumber, %d), err %d, %s\n", mpContext->setting.video_decoder_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.rtsp_client_try_tcp_mode_first) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst;
        pre_config.number = mpContext->setting.rtsp_client_try_tcp_mode_first;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientTryTCPModeFirst, %d), err %d, %s\n", mpContext->setting.net_receiver_scheduler_number, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    pre_config.check_field = EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit;
    pre_config.number = mpContext->setting.parse_multiple_access_unit;
    err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit, &pre_config);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("EGenericEnginePreConfigure_RTSPClientParseMultipleAccessUnit(%d), err %d, %s\n", mpContext->setting.parse_multiple_access_unit, err, gfGetErrorCodeString(err));
        return err;
    }

    if (mpContext->setting.decoder_prefetch_count) {
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetPreFetch;
        pre_config.number = mpContext->setting.decoder_prefetch_count;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetPreFetch, %d), err %d, %s\n", mpContext->setting.decoder_prefetch_count, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.dsp_decoder_total_buffer_count) {
        pre_config.check_field = EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber;
        pre_config.number = mpContext->setting.dsp_decoder_total_buffer_count;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PlaybackSetDSPDecoderBufferNumber, %d), err %d, %s\n", mpContext->setting.dsp_decoder_total_buffer_count, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.udec_not_feed_usequpes) {
        pre_config.check_field = EGenericEnginePreConfigure_DSPAddUDECWrapper;
        pre_config.number = mpContext->setting.udec_not_feed_usequpes;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_DSPAddUDECWrapper, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_DSPAddUDECWrapper, %d), err %d, %s\n", mpContext->setting.udec_not_feed_usequpes, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.udec_not_feed_pts) {
        pre_config.check_field = EGenericEnginePreConfigure_DSPFeedPTS;
        pre_config.number = mpContext->setting.udec_not_feed_pts;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_DSPFeedPTS, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_DSPFeedPTS, %d), err %d, %s\n", mpContext->setting.udec_not_feed_pts, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.udec_specify_fps) {
        pre_config.check_field = EGenericEnginePreConfigure_DSPSpecifyFPS;
        pre_config.number = mpContext->setting.udec_specify_fps;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_DSPSpecifyFPS, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_DSPSpecifyFPS, %d), err %d, %s\n", mpContext->setting.udec_specify_fps, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.rtsp_server_port) {
        pre_config.check_field = EGenericEnginePreConfigure_RTSPStreamingServerPort;
        pre_config.number = mpContext->setting.rtsp_server_port;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_RTSPStreamingServerPort, %d), err %d, %s\n", mpContext->setting.rtsp_server_port, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.cloud_server_port) {
        pre_config.check_field = EGenericEnginePreConfigure_SACPCloudServerPort;
        pre_config.number = mpContext->setting.cloud_server_port;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_SACPCloudServerPort, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_SACPCloudServerPort, %d), err %d, %s\n", mpContext->setting.cloud_server_port, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.compensate_delay_from_jitter) {
        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter, NULL);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEngineConfigure_VideoPlayback_CompensateDelayFromJitter, %d), err %d, %s\n", mpContext->setting.compensate_delay_from_jitter, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_ffmpeg_muxer) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_FFMpeg), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_private_ts_muxer) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_Muxer_TS;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_Muxer_TS), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_ffmpeg_audio_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_FFMpeg), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_aac_audio_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_AAC), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_aac_audio_encoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_AAC), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_customized_adpcm_encoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioEncoder_Customized_ADPCM), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (mpContext->setting.debug_prefer_customized_adpcm_decoder) {
        pre_config.check_field = EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM;
        pre_config.number = 0;
        err = mpEngineControl->GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM, &pre_config);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("GenericPreConfigure(EGenericEnginePreConfigure_PreferModule_AudioDecoder_Customized_ADPCM), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    if (!mpContext->setting.disable_dsp_related) {
        err = initializeDevice();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("initializeDevice ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //some debug options
    if (mpContext->setting.b_smooth_stream_switch) {
        mbSmoothSwitchLayout = 1;
    }

    //default setting
    TUint index = 0, index_group = 0;
    for (index_group = 0; index_group < DMaxGroupNumber; index_group ++) {
        SGroupGragh *current_group = &mpContext->gragh_component[index_group];
        current_group->cloud_pipeline_connected[0] = 1;
        current_group->native_push_pipeline_connected[0] = 1;
        for (index = 1; index < DMaxChannelNumberInGroup; index ++) {
            current_group->cloud_pipeline_connected[index] = 0;
            current_group->native_push_pipeline_connected[index] = 0;
        }
    }

    if (DLikely(mpMediaConfig)) {
        mpMediaConfig->dsp_config.prefer_udec_stop_flags = mpContext->setting.debug_prefer_udec_stop_flag;
    } else {
        LOG_FATAL("NULL mpMediaConfig\n");
    }

    return EECode_OK;
}

EECode CSimpleAPI::createGlobalComponents()
{
    EECode err = EECode_OK;
    TUint index = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: createGlobalComponents begin\n");

    //create streaming server and data transmiter
    if (mpContext->setting.enable_streaming_server) {
        err = mpEngineControl->NewComponent(EGenericComponentType_StreamingServer, mpContext->streaming_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_StreamingServer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = mpEngineControl->NewComponent(EGenericComponentType_StreamingTransmiter, mpContext->streaming_transmiter_id);
        DASSERT_OK(err);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_StreamingTransmiter) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //create cloud server and remote control connecter
    if (mpContext->setting.enable_cloud_server) {
        //cloud server
        err = mpEngineControl->NewComponent(EGenericComponentType_CloudServer, mpContext->cloud_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_CloudServer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterServerAgent, mpContext->remote_control_agent_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterServerAgent) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }

        err = mpEngineControl->SetCloudAgentTag(mpContext->remote_control_agent_id, NULL, mpContext->cloud_server_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetCloudAgentTag() fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    //create dsp related
    if ((!mpContext->setting.disable_video_path) && (!mpContext->setting.disable_dsp_related)) {

        //video encoder
        if (mpContext->setting.enable_trasncoding) {
            err = mpEngineControl->NewComponent(EGenericComponentType_VideoEncoder, mpContext->video_encoder_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoEncoder) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }

            //cloud uploading
            if (mpContext->setting.enable_cloud_upload) {
                err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterClientAgent, mpContext->cloud_client_agent_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterClientAgent) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                    return err;
                }
                err = mpEngineControl->SetCloudClientTag(mpContext->cloud_client_agent_id, mpContext->setting.cloud_upload_tag, mpContext->setting.cloud_upload_server_addr, mpContext->setting.cloud_upload_server_port);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetCloudAgentTag fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }

        LOG_PRINTF("[CSimpleAPI]: createGlobalComponents, before create decoders, mpContext->setting.udec_2rd_instance_number %d\n", mpContext->setting.udec_2rd_instance_number);
        //video decoder
        for (index = 0; index < mpContext->setting.udec_2rd_instance_number; index ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_VideoDecoder, mpContext->video_decoder_2rd_id[index]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoDecoder, 2rd [%d]) fail, ret %d, %s.\n", index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        LOG_PRINTF("[CSimpleAPI]: createGlobalComponents, before create decoders, mpContext->setting.udec_instance_number %d\n", mpContext->setting.udec_instance_number);
        for (index = 0; index < mpContext->setting.udec_instance_number; index ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_VideoDecoder, mpContext->video_decoder_id[index]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_VideoDecoder[%d]) fail, ret %d, %s.\n", index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        //video renderer
        err = mpEngineControl->NewComponent(EGenericComponentType_VideoRenderer, mpContext->video_renderer_id);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_VideoRenderer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }

    } else {
        LOG_WARN("video path is not enabled, disable_video_path %d, disable_dsp_related %d\n", mpContext->setting.disable_video_path, mpContext->setting.disable_dsp_related);
    }

    //audio path
    if (!mpContext->setting.disable_audio_path) {

        //audio pinmuxer
        if (mpContext->setting.enable_audio_pinmuxer) {
            err = mpEngineControl->NewComponent(EGenericComponentType_ConnecterPinMuxer, mpContext->audio_pin_muxer_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_ConnecterPinMuxer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->setting.enable_playback) {
            //audio decoder
            err = mpEngineControl->NewComponent(EGenericComponentType_AudioDecoder, mpContext->audio_decoder_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioDecoder) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }

            //audio renderer
            err = mpEngineControl->NewComponent(EGenericComponentType_AudioRenderer, mpContext->audio_renderer_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioRenderer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_NOTICE("audio playback is not enabled\n");
        }

        if (mpContext->setting.enable_audio_capture) {
            //audio capture
            err = mpEngineControl->NewComponent(EGenericComponentType_AudioCapture, mpContext->audio_capture_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_AudioCapture) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->setting.push_audio_type) {
                LOG_NOTICE("[CSimpleAPI]: audio push use compressed format\n");
                //audio encoder
                err = mpEngineControl->NewComponent(EGenericComponentType_AudioEncoder, mpContext->audio_encoder_id);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("NewComponent(EGenericComponentType_AudioEncoder) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            } else {
                LOG_NOTICE("[CSimpleAPI]: audio push use raw PCM format\n");
            }
        } else {
            LOG_NOTICE("audio capture is not enabled\n");
        }
    } else {
        LOG_WARN("audio path is not enabled, disable_video_path %d\n", mpContext->setting.disable_audio_path);
    }

    LOG_PRINTF("[CSimpleAPI]: createGlobalComponents end\n");

    return EECode_OK;
}

EECode CSimpleAPI::createGroupComponents(TUint index)
{
    EECode err = EECode_OK;
    TUint i = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);
    if (index > DMaxGroupNumber) {
        LOG_FATAL("BAD index %d > DMaxGroupNumber %d\n", index, DMaxGroupNumber);
        return EECode_InternalLogicalBug;
    }

    LOG_PRINTF("[CSimpleAPI]: createGroupComponents(%d) begin\n", index);

    SGroupGragh *p_group_gragh = &mpContext->gragh_component[index];
    SGroupUrl *p_group_url = &mpContext->group[index];

    //create demuxers
    if (DLikely(!mpContext->setting.debug_share_demuxer)) {
        for (i = 0; i < p_group_url->source_2rd_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, p_group_gragh->demuxer_2rd_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Demuxer) 2rd fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        for (i = 0; i < p_group_url->source_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, p_group_gragh->demuxer_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Demuxer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    } else {
        LOG_NOTICE("[debug]: shared demuxer!\n");
        err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, p_group_gragh->demuxer_2rd_id[0]);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_Demuxer) 2rd fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
        for (i = 1; i < p_group_url->source_2rd_channel_number; i ++) {
            p_group_gragh->demuxer_2rd_id[i] = p_group_gragh->demuxer_2rd_id[0];
        }
        err = mpEngineControl->NewComponent(EGenericComponentType_Demuxer, p_group_gragh->demuxer_id[0]);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("NewComponent(EGenericComponentType_Demuxer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
            return err;
        }
        for (i = 1; i < p_group_url->source_channel_number; i ++) {
            p_group_gragh->demuxer_id[i] = p_group_gragh->demuxer_id[0];
        }
    }

    //create muxers
    if (mpContext->setting.enable_recording) {
        for (i = 0; i < p_group_url->sink_2rd_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Muxer, p_group_gragh->muxer_2rd_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Muxer) 2rd fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        for (i = 0; i < p_group_url->sink_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_Muxer, p_group_gragh->muxer_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_Muxer) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    //create cloud agent
    if (mpContext->setting.enable_cloud_server) {
        for (i = 0; i < p_group_url->cloud_channel_number; i ++) {
            err = mpEngineControl->NewComponent(EGenericComponentType_CloudConnecterServerAgent, p_group_gragh->cloud_agent_id[i]);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("NewComponent(EGenericComponentType_CloudConnecterServerAgent) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
            err = mpEngineControl->SetCloudAgentTag(p_group_gragh->cloud_agent_id[i], NULL, mpContext->cloud_server_id);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetCloudAgentTag() fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    LOG_PRINTF("[CSimpleAPI]: createGroupComponents(%d) end\n", index);

    return EECode_OK;
}

EECode CSimpleAPI::connectPlaybackComponents()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: connectPlaybackComponents() begin\n");

    //video playback path
    if (DLikely((!mpContext->setting.disable_video_path) && (!mpContext->setting.disable_dsp_related) && mpContext->setting.enable_playback)) {

        //source to video decoder
        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            // second channel to D1 decoder
            max = mpContext->setting.udec_2rd_instance_number > mpContext->group[group_index].source_2rd_channel_number ? mpContext->group[group_index].source_2rd_channel_number : mpContext->setting.udec_2rd_instance_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                        mpContext->video_decoder_2rd_id[channel_index], \
                                                        StreamType_Video);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(demuxer[%d][%d] to video decoder[%d]), err %d, %s\n", group_index, channel_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }
            }

            // main channel to full HD decoder
            DASSERT(2 > mpContext->setting.udec_instance_number);
            if (mpContext->setting.udec_instance_number) {
                max = mpContext->group[group_index].source_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->video_decoder_id[0], \
                                                            StreamType_Video);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to video decoder hd[0]), err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }
        }

        //video decoder to video renderer
        max = mpContext->setting.udec_2rd_instance_number;
        for (channel_index = 0; channel_index < max; channel_index ++) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->video_decoder_2rd_id[channel_index], \
                                                    mpContext->video_renderer_id, \
                                                    StreamType_Video);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video decoder [%d] to video renderer, err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->setting.udec_instance_number) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->video_decoder_id[0], \
                                                    mpContext->video_renderer_id, \
                                                    StreamType_Video);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video decoder hd [%d] to video renderer, err %d, %s\n", channel_index, err, gfGetErrorCodeString(err));
                return err;
            }
        }

    }

    //audio path
    if ((!mpContext->setting.disable_audio_path) && (mpContext->setting.enable_playback)) {

        if (DLikely(mpContext->setting.enable_audio_pinmuxer)) {

            for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

                // second channel to audio pin muxer
                max = mpContext->group[group_index].source_2rd_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->audio_pin_muxer_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to audio pinmuxer), err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                // main channel to audio pin muxer
                max = mpContext->group[group_index].source_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->audio_pin_muxer_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to audio pinmuxer), err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }

            //pin muxer to audio decoder
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->audio_pin_muxer_id, \
                                                    mpContext->audio_decoder_id, \
                                                    StreamType_Audio);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio pin muxer to audio decoder), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

        } else {

            for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

                // second channel to audio decoder
                max = mpContext->group[group_index].source_2rd_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->audio_decoder_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to audio decoder), err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                // main channel to audio decoder
                max = mpContext->group[group_index].source_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->audio_decoder_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to audio decoder), err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }
        }

        //audio decoder to audio renderer
        err = mpEngineControl->ConnectComponent(connection_id, \
                                                mpContext->audio_decoder_id, \
                                                mpContext->audio_renderer_id, \
                                                StreamType_Audio);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("ConnectComponent(audio decoder to audio renderer), err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

    }

    LOG_PRINTF("[CSimpleAPI]: connectPlaybackComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPI::connectRecordingComponents()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: connectRecordingComponents() begin\n");

    //video playback path
    if (DLikely(mpContext->setting.enable_recording)) {

        //source to muxer
        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            if (!mpContext->group[group_index].enable_recording) {
                continue;
            }

            // second channel to muxer
            max = mpContext->group[group_index].sink_2rd_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (!mpContext->setting.disable_video_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->gragh_component[group_index].muxer_2rd_id[channel_index], \
                                                            StreamType_Video);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to muxer[%d][%d]), video path, err %d, %s\n", group_index, channel_index, group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                if (!mpContext->setting.disable_audio_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->gragh_component[group_index].muxer_2rd_id[channel_index], \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to muxer[%d][%d]), audio path, err %d, %s\n", group_index, channel_index, group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }

            // main channel to muxer
            max = mpContext->group[group_index].sink_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (!mpContext->setting.disable_video_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->gragh_component[group_index].muxer_id[channel_index], \
                                                            StreamType_Video);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to muxer, [%d][%d], main channel), video path, err %d, %s\n", group_index, channel_index, group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                if (!mpContext->setting.disable_audio_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->gragh_component[group_index].muxer_id[channel_index], \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to muxer, [%d][%d], main channel), audio path, err %d, %s\n", group_index, channel_index, group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }

        }

    }

    LOG_PRINTF("[CSimpleAPI]: connectRecordingComponents() end\n");

    return EECode_OK;
}


EECode CSimpleAPI::connectStreamingComponents()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: connectStreamingComponents() begin\n");

    if (DLikely(mpContext->setting.enable_streaming_server)) {

        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            if ((!mpContext->group[group_index].enable_streaming) || (!mpContext->group[group_index].enable_streaming_relay)) {
                continue;
            }

            // second channel to streaming transmiter
            max = mpContext->group[group_index].streaming_2rd_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (!mpContext->setting.disable_video_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->streaming_transmiter_id, \
                                                            StreamType_Video);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to streaming_transmiter), video path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                if (!mpContext->setting.disable_audio_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index], \
                                                            mpContext->streaming_transmiter_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to streaming_transmiter), audio path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

            }

            // main channel to streaming transmiter
            max = mpContext->group[group_index].streaming_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {
                if (!mpContext->setting.disable_video_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->streaming_transmiter_id, \
                                                            StreamType_Video);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to streaming_transmiter, main channel), video path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }

                if (!mpContext->setting.disable_audio_path) {
                    err = mpEngineControl->ConnectComponent(connection_id, \
                                                            mpContext->gragh_component[group_index].demuxer_id[channel_index], \
                                                            mpContext->streaming_transmiter_id, \
                                                            StreamType_Audio);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("ConnectComponent(demuxer[%d][%d] to streaming_transmiter, main channel), audio path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
            }

        }

        //video encoder to streamer
        if (mpContext->setting.enable_trasncoding) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->video_encoder_id, \
                                                    mpContext->streaming_transmiter_id, \
                                                    StreamType_Video);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder to streaming_transmiter), video path, err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        if (mpContext->audio_pin_muxer_id) {
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->audio_pin_muxer_id, \
                                                    mpContext->streaming_transmiter_id, \
                                                    StreamType_Audio);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(audio pin muxer to streaming_transmiter), video path, err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

    }

    LOG_PRINTF("[CSimpleAPI]: connectStreamingComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPI::connectCloudComponents()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;
    TGenericID connection_id;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: connectCloudComponents() begin\n");

    if (DLikely(mpContext->setting.enable_cloud_server)) {

        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            if ((!mpContext->group[group_index].enable_cloud_agent) || (!mpContext->remote_control_agent_id)) {
                continue;
            }

            max = mpContext->group[group_index].cloud_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                // cloud channel, push audio
                err = mpEngineControl->ConnectComponent(connection_id, \
                                                        mpContext->remote_control_agent_id, \
                                                        mpContext->gragh_component[group_index].cloud_agent_id[channel_index], \
                                                        StreamType_Audio);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("ConnectComponent(remote control agent to cloud agent[%d][%d]), audio path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

            }

        }

    }

    LOG_PRINTF("[CSimpleAPI]: connectCloudComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPI::connectNativeAudioCaptureAndPushComponents()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;
    TGenericID connection_id = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: connectNativeAudioCaptureAndPushComponents() begin\n");

    if (DLikely(mpContext->setting.enable_audio_capture)) {
        if (mpContext->setting.push_audio_type) {
            DASSERT(mpContext->audio_encoder_id);
            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->audio_capture_id, \
                                                    mpContext->audio_encoder_id, \
                                                    StreamType_Audio);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(native audio capture to audio encoder), err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

            if (DLikely(mpContext->setting.enable_cloud_server)) {

                for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

                    if ((!mpContext->group[group_index].enable_cloud_agent) || (!mpContext->remote_control_agent_id)) {
                        continue;
                    }

                    max = mpContext->group[group_index].cloud_channel_number;
                    for (channel_index = 0; channel_index < max; channel_index ++) {

                        // cloud channel, native push audio
                        err = mpEngineControl->ConnectComponent(connection_id, \
                                                                mpContext->audio_encoder_id, \
                                                                mpContext->gragh_component[group_index].cloud_agent_id[channel_index], \
                                                                StreamType_Audio);

                        if (DUnlikely(EECode_OK != err)) {
                            LOG_ERROR("ConnectComponent(native audio capture(encoder) to cloud agent[%d][%d]), audio path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                            return err;
                        }

                    }

                }

            } else {
                LOG_WARN("cloud server not enabled?\n");
            }

        } else {

            if (DLikely(mpContext->setting.enable_cloud_server)) {

                for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

                    if ((!mpContext->group[group_index].enable_cloud_agent) || (!mpContext->remote_control_agent_id)) {
                        continue;
                    }

                    max = mpContext->group[group_index].cloud_channel_number;
                    for (channel_index = 0; channel_index < max; channel_index ++) {

                        // cloud channel, native push audio
                        err = mpEngineControl->ConnectComponent(connection_id, \
                                                                mpContext->audio_capture_id, \
                                                                mpContext->gragh_component[group_index].cloud_agent_id[channel_index], \
                                                                StreamType_Audio);

                        if (DUnlikely(EECode_OK != err)) {
                            LOG_ERROR("ConnectComponent(native audio capture to cloud agent[%d][%d]), audio path, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                            return err;
                        }

                    }

                }

            } else {
                LOG_WARN("cloud server not enabled?\n");
            }

        }
    }

    LOG_PRINTF("[CSimpleAPI]: connectNativeAudioCaptureAndPushComponents() end\n");

    return EECode_OK;
}

EECode CSimpleAPI::connectCloudUploadComponents()
{
    EECode err = EECode_OK;
    TGenericID connection_id = 0;

    if ((!mpContext->setting.disable_video_path) && (!mpContext->setting.disable_dsp_related)) {

        if (mpContext->setting.enable_trasncoding && mpContext->setting.enable_cloud_upload) {
            DASSERT(mpContext->video_encoder_id);
            DASSERT(mpContext->cloud_client_agent_id);

            err = mpEngineControl->ConnectComponent(connection_id, \
                                                    mpContext->video_encoder_id, \
                                                    mpContext->cloud_client_agent_id, \
                                                    StreamType_Video);
            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("ConnectComponent(video encoder to cloud upload agent), video path, err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }

        }
    }
    return EECode_OK;
}

EECode CSimpleAPI::setupPlaybackPipelines()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPI]: setupPlaybackPipelines begin, disable_video_path %d, disable_dsp_related %d, enable_playback %d\n", mpContext->setting.disable_video_path, mpContext->setting.disable_dsp_related, mpContext->setting.enable_playback);

    if (DLikely((!mpContext->setting.disable_video_path) && (!mpContext->setting.disable_dsp_related) && mpContext->setting.enable_playback)) {
        TU8 running_at_startup = 0;

        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {
            if (!mpContext->group[group_index].enable_playback) {
                LOG_NOTICE("group %d, disable playback\n", group_index);
                continue;
            }

            if (!group_index) {
                running_at_startup = 1;
            } else {
                running_at_startup = 0;
            }

            TGenericID audio_source_id = 0;
            TGenericID video_source_id = 0;

            LOG_NOTICE("[CSimpleAPI]::setupPlaybackPipelines begin, source_2rd_channel_number %d, udec_2rd_instance_number %d\n", mpContext->group[group_index].source_2rd_channel_number, mpContext->setting.udec_2rd_instance_number);
            max = mpContext->setting.udec_2rd_instance_number > mpContext->group[group_index].source_2rd_channel_number ? mpContext->group[group_index].source_2rd_channel_number : mpContext->setting.udec_2rd_instance_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {
                if (!mpContext->setting.disable_audio_path) {
                    audio_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                } else {
                    audio_source_id = 0;
                }
                if (!mpContext->setting.disable_video_path) {
                    video_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                } else {
                    video_source_id = 0;
                }
                err = mpEngineControl->SetupPlaybackPipeline(mpContext->gragh_component[group_index].playback_2rd_pipeline_id[channel_index], \
                        video_source_id, \
                        audio_source_id, \
                        mpContext->video_decoder_2rd_id[channel_index], \
                        mpContext->audio_decoder_id, \
                        mpContext->video_renderer_id, \
                        mpContext->audio_renderer_id, \
                        mpContext->audio_pin_muxer_id, \
                        running_at_startup);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupPlaybackPipeline[%d][%d], D1, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].playback_pipeline_2rd_suspended[channel_index] = !running_at_startup;
            }

            LOG_NOTICE("[CSimpleAPI]::setupPlaybackPipelines begin, source_channel_number %d, udec_instance_number %d\n", mpContext->group[group_index].source_channel_number, mpContext->setting.udec_instance_number);
            if (mpContext->setting.udec_instance_number) {
                max = mpContext->group[group_index].source_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {
                    if (!mpContext->setting.disable_audio_path) {
                        audio_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                    } else {
                        audio_source_id = 0;
                    }
                    if (!mpContext->setting.disable_video_path) {
                        video_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                    } else {
                        video_source_id = 0;
                    }
                    err = mpEngineControl->SetupPlaybackPipeline(mpContext->gragh_component[group_index].playback_pipeline_id[channel_index], \
                            video_source_id, \
                            audio_source_id, \
                            mpContext->video_decoder_id[0], \
                            mpContext->audio_decoder_id, \
                            mpContext->video_renderer_id, \
                            mpContext->audio_renderer_id, \
                            mpContext->audio_pin_muxer_id, \
                            0);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("SetupPlaybackPipeline[%d][%d], Full HD, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->gragh_component[group_index].playback_pipeline_suspended[channel_index] = 1;
                }

            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPI]: playback not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPI::setupRecordingPipelines()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPI]: setupRecordingPipelines begin, enable_recording %d\n", mpContext->setting.enable_recording);

    if (DLikely(mpContext->setting.enable_recording)) {

        //source to muxer
        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            TGenericID video_source_id = 0;
            TGenericID audio_source_id = 0;

            //LOG_NOTICE("[DEBUG]::enable_recording %d, mpContext->setting.disable_video_path %d, mpContext->setting.disable_audio_path %d, sink_2rd_channel_number %d, sink_channel_number %d\n", mpContext->group[group_index].enable_recording, mpContext->setting.disable_video_path, mpContext->setting.disable_audio_path, mpContext->group[group_index].sink_2rd_channel_number, mpContext->group[group_index].sink_channel_number);

            if (!mpContext->group[group_index].enable_recording) {
                LOG_NOTICE("group %d, disable recording\n", group_index);
                continue;
            }

            // second channel
            max = mpContext->group[group_index].sink_2rd_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                //LOG_NOTICE("[DEBUG]::enable_recording %d, sink_2rd_channel_number %d, sink_channel_number %d\n", mpContext->group[group_index].enable_recording, mpContext->group[group_index].sink_2rd_channel_number, mpContext->group[group_index].sink_channel_number);

                if (DUnlikely(mpContext->setting.disable_video_path)) {
                    video_source_id = 0;
                } else {
                    video_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                }

                if (DUnlikely(mpContext->setting.disable_audio_path)) {
                    audio_source_id = 0;
                } else {
                    audio_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                }

                err = mpEngineControl->SetupRecordingPipeline(mpContext->gragh_component[group_index].recording_2rd_pipeline_id[channel_index], \
                        video_source_id, \
                        audio_source_id, \
                        mpContext->gragh_component[group_index].muxer_2rd_id[channel_index]);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupRecordingPipeline[%d][%d], D1, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].recording_pipeline_2rd_suspended[channel_index] = 0;
            }

            // main channel
            max = mpContext->group[group_index].sink_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (DUnlikely(mpContext->setting.disable_video_path)) {
                    video_source_id = 0;
                } else {
                    video_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                }

                if (DUnlikely(mpContext->setting.disable_audio_path)) {
                    audio_source_id = 0;
                } else {
                    audio_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                }

                err = mpEngineControl->SetupRecordingPipeline(mpContext->gragh_component[group_index].recording_pipeline_id[channel_index], \
                        video_source_id, \
                        audio_source_id, \
                        mpContext->gragh_component[group_index].muxer_id[channel_index]);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupRecordingPipeline[%d][%d], Full HD, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].recording_pipeline_suspended[channel_index] = 0;
            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPI]: recording not enabled\n");
    }

    return EECode_OK;
}


EECode CSimpleAPI::setupStreamingPipelines()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPI]: setupStreamingPipelines begin, enable_streaming_server %d\n", mpContext->setting.enable_streaming_server);

    if (DLikely(mpContext->setting.enable_streaming_server)) {

        DASSERT(mpContext->streaming_server_id);
        DASSERT(mpContext->streaming_transmiter_id);

        //streaming relay
        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            TGenericID video_source_id = 0;
            TGenericID audio_source_id = 0;

            if ((!mpContext->group[group_index].enable_streaming) || (!mpContext->group[group_index].enable_streaming_relay)) {
                LOG_NOTICE("group %d, disable streaming, enable_streaming %d, enable_streaming_relay %d\n", group_index, mpContext->group[group_index].enable_streaming, mpContext->group[group_index].enable_streaming_relay);
                continue;
            }

            // second channel
            max = mpContext->group[group_index].streaming_2rd_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (DUnlikely(mpContext->setting.disable_video_path)) {
                    video_source_id = 0;
                } else {
                    video_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                }

                if (DUnlikely(mpContext->setting.disable_audio_path)) {
                    audio_source_id = 0;
                } else {
                    audio_source_id = mpContext->gragh_component[group_index].demuxer_2rd_id[channel_index];
                }

                err = mpEngineControl->SetupStreamingPipeline(mpContext->gragh_component[group_index].streaming_2rd_pipeline_id[channel_index], \
                        mpContext->streaming_transmiter_id, \
                        mpContext->streaming_server_id, \
                        video_source_id, \
                        audio_source_id, \
                        mpContext->audio_pin_muxer_id);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupStreamingPipeline[%d][%d], D1, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].streaming_pipeline_2rd_suspended[channel_index] = 0;
            }

            // main channel
            max = mpContext->group[group_index].streaming_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {

                if (DUnlikely(mpContext->setting.disable_video_path)) {
                    video_source_id = 0;
                } else {
                    video_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                }

                if (DUnlikely(mpContext->setting.disable_audio_path)) {
                    audio_source_id = 0;
                } else {
                    audio_source_id = mpContext->gragh_component[group_index].demuxer_id[channel_index];
                }

                err = mpEngineControl->SetupStreamingPipeline(mpContext->gragh_component[group_index].streaming_pipeline_id[channel_index], \
                        mpContext->streaming_transmiter_id, \
                        mpContext->streaming_server_id, \
                        video_source_id, \
                        audio_source_id, \
                        mpContext->audio_pin_muxer_id);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupStreamingPipeline[%d][%d], Full HD, err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].streaming_pipeline_suspended[channel_index] = 0;
            }

        }

        if (mpContext->video_encoder_id) {
            //streaming transcode
            err = mpEngineControl->SetupStreamingPipeline(mpContext->transcoding_streaming_pipeline_id, \
                    mpContext->streaming_transmiter_id, \
                    mpContext->streaming_server_id, \
                    mpContext->video_encoder_id, \
                    0, \
                    mpContext->audio_pin_muxer_id);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupStreamingPipeline, transcode, err %d, %s\n",  err, gfGetErrorCodeString(err));
                return err;
            }

            //video only url
            err = mpEngineControl->SetupStreamingPipeline(mpContext->transcoding_streaming_video_only_pipeline_id, \
                    mpContext->streaming_transmiter_id, \
                    mpContext->streaming_server_id, \
                    mpContext->video_encoder_id, \
                    0, \
                    0);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupStreamingPipeline, transcode video only, err %d, %s\n",  err, gfGetErrorCodeString(err));
                return err;
            }

            if (mpContext->audio_pin_muxer_id) {
                //audio only url
                err = mpEngineControl->SetupStreamingPipeline(mpContext->transcoding_streaming_audio_only_pipeline_id, \
                        mpContext->streaming_transmiter_id, \
                        mpContext->streaming_server_id, \
                        0, \
                        0, \
                        mpContext->audio_pin_muxer_id);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupStreamingPipeline, transcode audio only, err %d, %s\n",  err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        }

    } else {
        LOG_NOTICE("[CSimpleAPI]: streaming not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPI::setupCloudPipelines()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPI]: setupCloudPipelines begin, enable_cloud_server %d\n", mpContext->setting.enable_cloud_server);

    if (DLikely(mpContext->setting.enable_cloud_server)) {

        for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

            if ((!mpContext->group[group_index].enable_cloud_agent) || (!mpContext->remote_control_agent_id)) {
                LOG_NOTICE("group %d, disable cloud, enable_cloud_agent %d, remote_control_agent_id 0x%08x\n", group_index, mpContext->group[group_index].enable_cloud_agent, mpContext->remote_control_agent_id);
                continue;
            }

            max = mpContext->group[group_index].cloud_channel_number;
            for (channel_index = 0; channel_index < max; channel_index ++) {
                TU8 startup_running = (!channel_index); // (!channel_index) && (!group_index);
                err = mpEngineControl->SetupCloudPipeline(mpContext->gragh_component[group_index].cloud_pipeline_id[channel_index], \
                        mpContext->remote_control_agent_id, \
                        mpContext->cloud_server_id, \
                        0, \
                        mpContext->gragh_component[group_index].cloud_agent_id[channel_index], \
                        startup_running);

                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("SetupCloudPipeline[%d][%d], err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                    return err;
                }

                mpContext->gragh_component[group_index].cloud_pipeline_suspended[channel_index] = 0;
            }

        }

    } else {
        LOG_NOTICE("[CSimpleAPI]: cloud server not enabled\n");
    }

    return EECode_OK;
}

EECode CSimpleAPI::setupNativeCapturePushPipelines()
{
    EECode err = EECode_OK;
    TUint group_index = 0;
    TUint channel_index = 0;
    TUint max = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_NOTICE("[CSimpleAPI]: setupNativeCapturePushPipelines begin, enable_audio_capture %d, enable_cloud_server %d\n", mpContext->setting.enable_audio_capture, mpContext->setting.enable_cloud_server);

    if (mpContext->setting.enable_audio_capture) {

        err = mpEngineControl->SetupNativeCapturePipeline(mpContext->native_capture_pipeline, \
                0, \
                mpContext->audio_capture_id, \
                0, \
                mpContext->audio_encoder_id, \
                0);

        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetupNativeCapturePipeline, err %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }

        if (DLikely(mpContext->setting.enable_cloud_server)) {

            TGenericID push_source_id = 0;
            if (mpContext->setting.push_audio_type) {
                push_source_id = mpContext->audio_encoder_id;
            } else {
                push_source_id = mpContext->audio_capture_id;
            }

            for (group_index = 0; group_index < mpContext->group_number; group_index ++) {

                if (!mpContext->group[group_index].enable_cloud_agent) {
                    LOG_NOTICE("group %d, disable cloud, enable_cloud_agent %d\n", group_index, mpContext->group[group_index].enable_cloud_agent);
                    continue;
                }

                max = mpContext->group[group_index].cloud_channel_number;
                for (channel_index = 0; channel_index < max; channel_index ++) {

                    err = mpEngineControl->SetupNativePushPipeline(mpContext->gragh_component[group_index].native_push_pipeline_id[channel_index], \
                            push_source_id, \
                            0, \
                            mpContext->gragh_component[group_index].cloud_agent_id[channel_index], \
                            0);

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("SetupNativePushPipeline[%d][%d], err %d, %s\n", group_index, channel_index, err, gfGetErrorCodeString(err));
                        return err;
                    }

                    mpContext->gragh_component[group_index].native_push_pipeline_suspended[channel_index] = 1;
                }

            }

        } else {
            LOG_NOTICE("[CSimpleAPI]: cloud server not enabled\n");
        }
    }

    return EECode_OK;
}

EECode CSimpleAPI::setupCloudUploadPipelines()
{
    EECode err = EECode_OK;

    if ((!mpContext->setting.disable_video_path) && (!mpContext->setting.disable_dsp_related)) {

        if (mpContext->setting.enable_trasncoding && mpContext->setting.enable_cloud_upload) {
            err = mpEngineControl->SetupCloudUploadPipeline(mpContext->cloud_upload_pipeline_id, \
                    mpContext->video_encoder_id, \
                    0, \
                    mpContext->cloud_client_agent_id, \
                    0);

            if (DUnlikely(EECode_OK != err)) {
                LOG_ERROR("SetupCloudUploadPipelines, err %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_NOTICE("[CSimpleAPI]: transcoding or cloud upload is not enabled!\n");
        }
    }

    return EECode_OK;
}

EECode CSimpleAPI::buildDataProcessingGragh()
{
    EECode err = EECode_OK;
    TUint index = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    LOG_PRINTF("[CSimpleAPI]: buildDataProcessingGragh begin\n");

    err = mpEngineControl->BeginConfigProcessPipeline(1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("BeginConfigProcessPipeline(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = createGlobalComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("createGlobalComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    for (index = 0; index < mpContext->group_number; index ++) {
        err = createGroupComponents(index);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("createGroupComponents(%d), err %d, %s\n", index, err, gfGetErrorCodeString(err));
            return err;
        }
    }

    err = connectPlaybackComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectPlaybackComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectRecordingComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectRecordingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectStreamingComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectStreamingComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectCloudComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectCloudComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectNativeAudioCaptureAndPushComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectNativeAudioCaptureAndPushComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = connectCloudUploadComponents();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("connectCloudUploadComponents(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupPlaybackPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupPlaybackPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupRecordingPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupRecordingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupStreamingPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupStreamingPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupCloudPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupCloudPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupNativeCapturePushPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupNativeCapturePushPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupCloudUploadPipelines();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupCloudUploadPipelines(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = preSetUrls();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("preSetUrls(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpEngineControl->FinalizeConfigProcessPipeline();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("FinalizeConfigProcessPipeline(), err %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    LOG_PRINTF("[CSimpleAPI]: buildDataProcessingGragh end\n");

    return EECode_OK;
}

EECode CSimpleAPI::preSetUrls()
{
    EECode err = EECode_OK;
    TUint i = 0, group_index = 0, max_group = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    max_group = mpContext->group_number;
    if (max_group > DMaxGroupNumber) {
        LOG_ERROR("bad mpContext->group_number %d, DMaxGroupNumber %d\n", mpContext->group_number, DMaxGroupNumber);
        max_group = DMaxGroupNumber;
    }

    for (group_index = 0; group_index < max_group; group_index ++) {

        SGroupGragh *p_group_gragh = &mpContext->gragh_component[group_index];
        SGroupUrl *p_group_url = &mpContext->group[group_index];

        //set sink urls
        for (i = 0; i < p_group_url->sink_2rd_channel_number; i ++) {
            if (p_group_gragh->muxer_2rd_id[i]) {
                TChar *url = p_group_url->sink_url_2rd[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_2rd_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default url
                    snprintf(url, DMaxUrlLen, "auto_2rd_%d_%d.ts", group_index, i);
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_2rd_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        for (i = 0; i < p_group_url->sink_channel_number; i ++) {
            if (p_group_gragh->muxer_id[i]) {
                TChar *url = p_group_url->sink_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default url
                    snprintf(url, DMaxUrlLen, "auto_%d_%d.ts", group_index, i);
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        //set streaming urls
        for (i = 0; i < p_group_url->streaming_2rd_channel_number; i ++) {
            if (p_group_gragh->streaming_2rd_pipeline_id[i]) {
                TChar *url = p_group_url->streaming_url_2rd[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_2rd_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 2rd 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_2rd_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default tag
                    snprintf(url, DMaxUrlLen, "live_%d_%d", group_index, i);
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_2rd_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 2rd 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_2rd_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        for (i = 0; i < p_group_url->streaming_channel_number; i ++) {
            if (p_group_gragh->streaming_pipeline_id[i]) {
                TChar *url = p_group_url->streaming_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default tag
                    snprintf(url, DMaxUrlLen, "live_%d_%d", group_index, i);
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        //cloud agent tag
        for (i = 0; i < p_group_url->cloud_channel_number; i ++) {
            if (p_group_gragh->cloud_agent_id[i]) {
                TChar *url = p_group_url->cloud_agent_tag[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetCloudAgentTag(p_group_gragh->cloud_agent_id[i], url);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", p_group_gragh->cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default agent tag
                    snprintf(url, DMaxUrlLen, "camera://%d/%d", group_index, i);
                    err = mpEngineControl->SetCloudAgentTag(p_group_gragh->cloud_agent_id[i], url);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", p_group_gragh->cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }
    }

    //transcode streaming
    if (mpContext->transcoding_streaming_pipeline_id) {
        TChar url[] = "transcode0";
        if (strlen(url)) {
            err = mpEngineControl->SetStreamingUrl(mpContext->transcoding_streaming_pipeline_id, (TChar *)url);
            LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->transcoding_streaming_pipeline_id, url, err, gfGetErrorCodeString(err));
        }
    }

    if (mpContext->transcoding_streaming_video_only_pipeline_id) {
        TChar url[] = "transcode0_video";
        if (strlen(url)) {
            err = mpEngineControl->SetStreamingUrl(mpContext->transcoding_streaming_video_only_pipeline_id, (TChar *)url);
            LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->transcoding_streaming_video_only_pipeline_id, url, err, gfGetErrorCodeString(err));
        }
    }

    if (mpContext->transcoding_streaming_audio_only_pipeline_id) {
        TChar url[] = "transcode0_audio";
        if (strlen(url)) {
            err = mpEngineControl->SetStreamingUrl(mpContext->transcoding_streaming_audio_only_pipeline_id, (TChar *)url);
            LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->transcoding_streaming_audio_only_pipeline_id, url, err, gfGetErrorCodeString(err));
        }
    }

    //remote control tag
    if (mpContext->remote_control_agent_id) {
        TChar url[] = "uploading://app0";
        if (strlen(url)) {
            err = mpEngineControl->SetCloudAgentTag(mpContext->remote_control_agent_id, (TChar *)url);
            LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->remote_control_agent_id, url, err, gfGetErrorCodeString(err));
        }
    }

    return err;
}

EECode CSimpleAPI::setUrls()
{
    EECode err = EECode_OK;
    TUint i = 0, group_index = 0, max_group = 0;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    max_group = mpContext->group_number;
    if (max_group > DMaxGroupNumber) {
        LOG_ERROR("bad mpContext->group_number %d, DMaxGroupNumber %d\n", mpContext->group_number, DMaxGroupNumber);
        max_group = DMaxGroupNumber;
    }

    for (group_index = 0; group_index < max_group; group_index ++) {

        SGroupGragh *p_group_gragh = &mpContext->gragh_component[group_index];
        SGroupUrl *p_group_url = &mpContext->group[group_index];

        //set source urls
        for (i = 0; i < p_group_url->source_2rd_channel_number; i ++) {
            if (p_group_gragh->demuxer_2rd_id[i]) {
                TChar *url = p_group_url->source_url_2rd[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSourceUrl(p_group_gragh->demuxer_2rd_id[i], url);
                    LOG_PRINTF("SetSourceUrl 0x%08x, %s, ret %d %s\n", p_group_gragh->demuxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        for (i = 0; i < p_group_url->source_channel_number; i ++) {
            if (p_group_gragh->demuxer_id[i]) {
                TChar *url = p_group_url->source_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSourceUrl(p_group_gragh->demuxer_id[i], url);
                    LOG_PRINTF("SetSourceUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->demuxer_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

#if 0
        //set sink urls
        for (i = 0; i < p_group_url->sink_2rd_channel_number; i ++) {
            if (p_group_gragh->muxer_2rd_id[i]) {
                TChar *url = p_group_url->sink_url_2rd[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_2rd_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default url
                    snprintf(url, DMaxUrlLen, "auto_2rd_%d_%d.ts", group_index, i);
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_2rd_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_2rd_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        for (i = 0; i < p_group_url->sink_channel_number; i ++) {
            if (p_group_gragh->muxer_id[i]) {
                TChar *url = p_group_url->sink_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default url
                    snprintf(url, DMaxUrlLen, "auto_%d_%d.ts", group_index, i);
                    err = mpEngineControl->SetSinkUrl(p_group_gragh->muxer_id[i], url);
                    LOG_PRINTF("SetSinkUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->muxer_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        //set streaming urls
        for (i = 0; i < p_group_url->streaming_2rd_channel_number; i ++) {
            if (p_group_gragh->streaming_2rd_pipeline_id[i]) {
                TChar *url = p_group_url->streaming_url_2rd[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_2rd_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 2rd 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_2rd_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default tag
                    snprintf(url, DMaxUrlLen, "live_%d_%d", group_index, i);
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_2rd_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 2rd 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_2rd_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        for (i = 0; i < p_group_url->streaming_channel_number; i ++) {
            if (p_group_gragh->streaming_pipeline_id[i]) {
                TChar *url = p_group_url->streaming_url[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default tag
                    snprintf(url, DMaxUrlLen, "live_%d_%d", group_index, i);
                    err = mpEngineControl->SetStreamingUrl(p_group_gragh->streaming_pipeline_id[i], url);
                    LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", p_group_gragh->streaming_pipeline_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }

        //cloud agent tag
        for (i = 0; i < p_group_url->cloud_channel_number; i ++) {
            if (p_group_gragh->cloud_agent_id[i]) {
                TChar *url = p_group_url->cloud_agent_tag[i];
                if (strlen(url)) {
                    err = mpEngineControl->SetCloudAgentTag(p_group_gragh->cloud_agent_id[i], url);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", p_group_gragh->cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                } else {
                    //default agent tag
                    snprintf(url, DMaxUrlLen, "camera://%d/%d", group_index, i);
                    err = mpEngineControl->SetCloudAgentTag(p_group_gragh->cloud_agent_id[i], url);
                    LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", p_group_gragh->cloud_agent_id[i], url, err, gfGetErrorCodeString(err));
                }
            }
        }
#endif

    }

#if 0
    //transcode streaming
    if (mpContext->transcoding_streaming_pipeline_id) {
        TChar url[] = "transcode0";
        if (strlen(url)) {
            err = mpEngineControl->SetStreamingUrl(mpContext->transcoding_streaming_pipeline_id, (TChar *)url);
            LOG_PRINTF("SetStreamingUrl 0x%08x, %s, ret %d, %s\n", mpContext->transcoding_streaming_pipeline_id, url, err, gfGetErrorCodeString(err));
        }
    }

    //remote control tag
    if (mpContext->remote_control_agent_id) {
        TChar url[] = "uploading://app0";
        if (strlen(url)) {
            err = mpEngineControl->SetCloudAgentTag(mpContext->remote_control_agent_id, (TChar *)url);
            LOG_PRINTF("SetCloudAgentTag 0x%08x, %s, ret %d, %s\n", mpContext->remote_control_agent_id, url, err, gfGetErrorCodeString(err));
        }
    }
#endif

    return err;
}

void CSimpleAPI::suspendPlaybackPipelines(TU8 start_index, TU8 end_index, TU8 group_index, TU8 is_second, TUint release_context)
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (DUnlikely((group_index >= mpContext->group_number) || (group_index >= DMaxGroupNumber))) {
        LOG_FATAL("Bad group index %d, group_number %d, DMaxGroupNumber %d\n", group_index, mpContext->group_number, DMaxGroupNumber);
        return;
    }

    if (DUnlikely((end_index < start_index) || (group_index >= DMaxGroupNumber))) {
        LOG_FATAL("Bad end_index %d, start_index %d\n", end_index, start_index);
        return;
    }

    if (is_second) {
        if (end_index >= mpContext->group[group_index].source_2rd_channel_number) {
            LOG_FATAL("Bad end_index %d, source_2rd_channel_number %d\n", end_index, mpContext->group[group_index].source_2rd_channel_number);
            return;
        }

        TU8 i = 0;
        for (i = start_index; i <= end_index; i ++) {
            if (!mpContext->gragh_component[group_index].playback_pipeline_2rd_suspended[i]) {
                err = mpEngineControl->SuspendPipeline(mpContext->gragh_component[group_index].playback_2rd_pipeline_id[i], release_context);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_WARN("SuspendPipeline(0x%08x) ret %d, %s\n", mpContext->gragh_component[group_index].playback_2rd_pipeline_id[i], err, gfGetErrorCodeString(err));
                }
                mpContext->gragh_component[group_index].playback_pipeline_2rd_suspended[i] = 1;
            }
        }
    } else {
        if (DUnlikely(end_index >= mpContext->group[group_index].source_channel_number)) {
            LOG_FATAL("Bad end_index %d, source_channel_number %d\n", end_index, mpContext->group[group_index].source_channel_number);
            return;
        }

        TU8 i = 0;
        for (i = start_index; i <= end_index; i ++) {
            if (!mpContext->gragh_component[group_index].playback_pipeline_suspended[i]) {
                err = mpEngineControl->SuspendPipeline(mpContext->gragh_component[group_index].playback_pipeline_id[i], release_context);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_WARN("SuspendPipeline(0x%08x, group_index %d, index %d) ret %d, %s\n", mpContext->gragh_component[group_index].playback_pipeline_id[i], group_index, i, err, gfGetErrorCodeString(err));
                }
                mpContext->gragh_component[group_index].playback_pipeline_suspended[i] = 1;
            }
        }
    }

}

void CSimpleAPI::resumePlaybackPipelines(TU8 start_index, TU8 end_index, TU8 group_index, TU8 is_second)
{
    EECode err = EECode_OK;

    DASSERT(mpEngineControl);
    DASSERT(mpContext);

    if (DUnlikely((group_index >= mpContext->group_number) || (group_index >= DMaxGroupNumber))) {
        LOG_FATAL("Bad group index %d, group_number %d, DMaxGroupNumber %d\n", group_index, mpContext->group_number, DMaxGroupNumber);
        return;
    }

    if (DUnlikely((end_index < start_index) || (group_index >= DMaxGroupNumber))) {
        LOG_FATAL("Bad end_index %d, start_index %d\n", end_index, start_index);
        return;
    }

    if (is_second) {
        if (end_index >= mpContext->group[group_index].source_2rd_channel_number) {
            LOG_FATAL("Bad end_index %d, source_2rd_channel_number %d\n", end_index, mpContext->group[group_index].source_2rd_channel_number);
            return;
        }

        TU8 i = 0;
        for (i = start_index; i <= end_index; i ++) {
            if (mpContext->gragh_component[group_index].playback_pipeline_2rd_suspended[i]) {
                if (mpContext->single_channel_audio_camera2nvr == i) {
                    err = mpEngineControl->ResumePipeline(mpContext->gragh_component[group_index].playback_2rd_pipeline_id[i], 1, 1);
                } else {
                    err = mpEngineControl->ResumePipeline(mpContext->gragh_component[group_index].playback_2rd_pipeline_id[i]);
                }

                if (DUnlikely(EECode_OK != err)) {
                    LOG_WARN("ResumePipeline(0x%08x) ret %d, %s\n", mpContext->gragh_component[group_index].playback_2rd_pipeline_id[i], err, gfGetErrorCodeString(err));
                }
                mpContext->gragh_component[group_index].playback_pipeline_2rd_suspended[i] = 0;
            }
        }
    } else {
        if (DUnlikely(end_index >= mpContext->group[group_index].source_channel_number)) {
            LOG_FATAL("Bad end_index %d, source_channel_number %d\n", end_index, mpContext->group[group_index].source_channel_number);
            return;
        }

        TU8 i = 0;
        for (i = start_index; i <= end_index; i ++) {
            if (mpContext->gragh_component[group_index].playback_pipeline_suspended[i]) {
                if (mpContext->single_channel_audio_camera2nvr == i) {
                    err = mpEngineControl->ResumePipeline(mpContext->gragh_component[group_index].playback_pipeline_id[i], 1, 1);
                } else {
                    err = mpEngineControl->ResumePipeline(mpContext->gragh_component[group_index].playback_pipeline_id[i]);
                }

                if (DUnlikely(EECode_OK != err)) {
                    LOG_WARN("ResumePipeline(0x%08x, group %d, index %d) ret %d, %s\n", mpContext->gragh_component[group_index].playback_pipeline_id[i], group_index, i, err, gfGetErrorCodeString(err));
                }
                mpContext->gragh_component[group_index].playback_pipeline_suspended[i] = 0;
            }
        }
    }

}

EECode CSimpleAPI::initializeDevice()
{
    EECode err = selectDSPMode(mpContext->setting.work_mode);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("selectDSPMode ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    if (DUnlikely(!mpEngineControl)) {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    err = mpEngineControl->InitializeDevice();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("InitializeDevice ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mpEngineControl->GetMediaConfig(mpMediaConfig);

    summarizeVoutParams();

    return EECode_OK;
}

EECode CSimpleAPI::setupInitialMediaConfig()
{
    EECode err = EECode_OK;

    if (DUnlikely(!mpEngineControl)) {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    mpMediaConfig->dsp_config.tiled_mode = mpContext->setting.dsp_tilemode;
    if (mpContext->setting.set_bg_color) {
        mpMediaConfig->dsp_config.modeConfig.pp_background_Y = mpContext->setting.bg_color_y;
        mpMediaConfig->dsp_config.modeConfig.pp_background_Cb = mpContext->setting.bg_color_cb;
        mpMediaConfig->dsp_config.modeConfig.pp_background_Cr = mpContext->setting.bg_color_cr;
    }

    if (EPlaybackMode_1x1080p_4xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(4, 1, 720, 480, 1920, 1080);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x1080p_6xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(6, 1, 720, 480, 1920, 1080);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x1080p_8xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(8, 1, 720, 480, 1920, 1080);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x1080p_9xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(9, 1, 720, 480, 1920, 1080);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x1080p == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(0, 1, 0, 0, 1920, 1080);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_4x720p == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(4, 0, 1280, 720, 0, 0);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_4xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(4, 0, 720, 480, 0, 0);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_6xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(6, 0, 720, 480, 0, 0);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_8xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(8, 0, 720, 480, 0, 0);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_9xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(9, 0, 720, 480, 0, 0);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else if (EPlaybackMode_1x3M_4xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(4, 1, 720, 480, 2048, 1536);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x3M_6xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(6, 1, 720, 480, 2048, 1536);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x3M_8xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(8, 1, 720, 480, 2048, 1536);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x3M_9xD1 == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(9, 1, 720, 480, 2048, 1536);
        mCurrentSyncStatus.have_dual_stream = 1;
    } else if (EPlaybackMode_1x3M == mpContext->setting.playback_mode) {
        err = presetPlaybackModeForDSP(0, 1, 0, 0, 2048, 1536);
        mCurrentSyncStatus.have_dual_stream = 0;
    } else {
        LOG_FATAL("BAD mpContext->setting.playback_mode %d\n", mpContext->setting.playback_mode);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("presetPlaybackModeForDSP ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    err = setupDSPConfig();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("setupDSPConfig ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mpEngineControl->SetMediaConfig();

    return EECode_OK;
}

EECode CSimpleAPI::activateDevice()
{
    if (DUnlikely(!mpEngineControl)) {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    EECode err = mpEngineControl->ActivateDevice(mRequestDSPMode);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("ActivateDevice(%d) ret %d, %s\n", mRequestDSPMode, err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CSimpleAPI::deActivateDevice()
{
    if (DUnlikely(!mpEngineControl)) {
        LOG_ERROR("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    EECode err = mpEngineControl->DeActivateDevice();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("DeActivateDevice() ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CSimpleAPI::setupDSPConfig()
{
    EECode err = EECode_OK;
    switch (mRequestDSPMode) {
        case EDSPOperationMode_MultipleWindowUDEC:
        case EDSPOperationMode_MultipleWindowUDEC_Transcode:
            err = setupMUdecDSPSettings();
            break;
        case EDSPOperationMode_UDEC:
            err = setupUdecDSPSettings();
            break;
        default:
            err = EECode_BadParam;
            LOG_ERROR("Unsupported dsp mode\n");
            break;
    }
    return err;
}

EECode CSimpleAPI::selectDSPMode(EMediaWorkingMode mode)
{
    switch (mode) {
        case EMediaDeviceWorkingMode_SingleInstancePlayback:
            mRequestDSPMode = EDSPOperationMode_UDEC;
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback:
            mRequestDSPMode = EDSPOperationMode_MultipleWindowUDEC;
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding:
            mRequestDSPMode = EDSPOperationMode_MultipleWindowUDEC_Transcode;
            break;
        case EMediaDeviceWorkingMode_MultipleInstanceRecording:
            mRequestDSPMode = EDSPOperationMode_CameraRecording;
            break;

        case EMediaDeviceWorkingMode_FullDuplex_SingleRecSinglePlayback:
            mRequestDSPMode = EDSPOperationMode_FullDuplex;
            break;

        default:
            LOG_FATAL("TODO mode %d\n", mode);
            return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

void CSimpleAPI::summarizeVoutParams()
{
    TInt i = 0;
    TU16 max_vout_width = 0;
    TU16 max_vout_height = 0;
    TU8 vout_mask = 0;
    TU8 primary_vout = 0;
    TU8 voutmask_0_primary_vout = 0;
    TU8 set_primary_vout = 0;

    DASSERT(mpMediaConfig);

    for (i = 0; i < EDisplayDevice_TotCount; i++) {

        if (mpMediaConfig->dsp_config.voutConfigs.voutConfig[i].failed) {
            LOG_NOTICE("vout %d not initialized.\n", i);
            continue;
        }

        if (mpMediaConfig->dsp_config.voutConfigs.voutConfig[i].width > max_vout_width) {
            max_vout_width = mpMediaConfig->dsp_config.voutConfigs.voutConfig[i].width;
        }

        if (mpMediaConfig->dsp_config.voutConfigs.voutConfig[i].height > max_vout_height) {
            max_vout_height = mpMediaConfig->dsp_config.voutConfigs.voutConfig[i].height;
        }

        if (!(DCAL_BITMASK(i) & mpContext->setting.voutmask)) {
            LOG_NOTICE("vout %d not requested.\n", i);
            voutmask_0_primary_vout = i;
            continue;
        }

        vout_mask |= DCAL_BITMASK(i);

        primary_vout = i;
        set_primary_vout = 1;
    }

    if (!set_primary_vout) {
        DASSERT(0 == vout_mask);
        primary_vout = voutmask_0_primary_vout;
    }

    mpMediaConfig->dsp_config.modeConfig.vout_mask = vout_mask;
    mpMediaConfig->dsp_config.modeConfig.primary_vout = primary_vout;

    mpMediaConfig->dsp_config.modeConfig.pp_max_frm_width = max_vout_width;
    mpMediaConfig->dsp_config.modeConfig.pp_max_frm_height = max_vout_height;

    LOG_NOTICE("vout_mask 0x%02x, primary_vout %d, max_vout_width %d, max_vout_height %d\n", vout_mask, primary_vout, max_vout_width, max_vout_height);
}

EECode CSimpleAPI::presetPlaybackModeForDSP(TU16 num_of_instances_0, TU16 num_of_instances_1, TUint max_width_0, TUint max_height_0, TUint max_width_1, TUint max_height_1)
{
    TU16 i;

    if ((num_of_instances_0 + num_of_instances_1) > DMaxUDECInstanceNumber) {
        LOG_FATAL("BAD request num_of_instances_0 %d, num_of_instances_1 %d, exceed max %d\n", num_of_instances_0, num_of_instances_1, DMaxUDECInstanceNumber);
        return EECode_BadParam;
    }

    for (i = 0; i < num_of_instances_0; i ++) {
        mpMediaConfig->dsp_config.udecInstanceConfig[i].tiled_mode = mpMediaConfig->dsp_config.tiled_mode;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = mpMediaConfig->dsp_config.frm_chroma_fmt_max;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_num = mpMediaConfig->dsp_config.max_frm_num;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_fifo_size = mpMediaConfig->dsp_config.max_fifo_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_pp = mpMediaConfig->dsp_config.enable_pp;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_deint = mpMediaConfig->dsp_config.enable_deint;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].interlaced_out = mpMediaConfig->dsp_config.interlaced_out;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].packed_out = mpMediaConfig->dsp_config.packed_out;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_err_handle = mpMediaConfig->dsp_config.enable_err_handle;

        mpMediaConfig->dsp_config.udecInstanceConfig[i].bits_fifo_size = mpMediaConfig->dsp_config.bits_fifo_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].ref_cache_size = mpMediaConfig->dsp_config.ref_cache_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].concealment_mode = mpMediaConfig->dsp_config.concealment_mode;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].concealment_ref_frm_buf_id = mpMediaConfig->dsp_config.concealment_ref_frm_buf_id;

        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_width = max_width_0;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_height = max_height_0;
        LOG_INFO("[%d] decoder, max_frm_width %d, max_frm_height %d\n", i, mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_width, mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_height);
    }

    mpMediaConfig->tmp_number_of_d1 = num_of_instances_0;

    for (i = num_of_instances_0; i < (num_of_instances_0 + num_of_instances_1); i ++) {
        mpMediaConfig->dsp_config.udecInstanceConfig[i].tiled_mode = mpMediaConfig->dsp_config.tiled_mode;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = mpMediaConfig->dsp_config.frm_chroma_fmt_max;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_num = mpMediaConfig->dsp_config.max_frm_num;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_fifo_size = mpMediaConfig->dsp_config.max_fifo_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_pp = mpMediaConfig->dsp_config.enable_pp;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_deint = mpMediaConfig->dsp_config.enable_deint;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].interlaced_out = mpMediaConfig->dsp_config.interlaced_out;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].packed_out = mpMediaConfig->dsp_config.packed_out;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].enable_err_handle = mpMediaConfig->dsp_config.enable_err_handle;

        mpMediaConfig->dsp_config.udecInstanceConfig[i].bits_fifo_size = mpMediaConfig->dsp_config.bits_fifo_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].ref_cache_size = mpMediaConfig->dsp_config.ref_cache_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].concealment_mode = mpMediaConfig->dsp_config.concealment_mode;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].concealment_ref_frm_buf_id = mpMediaConfig->dsp_config.concealment_ref_frm_buf_id;

        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_width = max_width_1;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_height = max_height_1;
        LOG_INFO("[%d] decoder, max_frm_width %d, max_frm_height %d\n", i, mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_width, mpMediaConfig->dsp_config.udecInstanceConfig[i].max_frm_height);
    }

    mpMediaConfig->dsp_config.modeConfig.num_udecs = num_of_instances_0 + num_of_instances_1;

    return EECode_OK;
}

EECode CSimpleAPI::setupUdecDSPSettings()
{
    DASSERT(EDSPOperationMode_UDEC == mRequestDSPMode);
    if (EDSPOperationMode_UDEC != mRequestDSPMode) {
        return EECode_BadState;
    }

    mpMediaConfig->dsp_config.request_dsp_mode = mRequestDSPMode;

    mpMediaConfig->dsp_config.modeConfig.num_udecs = 1;

    mpMediaConfig->dsp_config.modeConfig.postp_mode = 2;
    mpMediaConfig->dsp_config.enable_deint = 0;
    mpMediaConfig->dsp_config.modeConfig.pp_chroma_fmt_max = 2;
    mpMediaConfig->dsp_config.modeConfig.pp_max_frm_num = 5;

    mpMediaConfig->dsp_config.udecInstanceConfig[0].tiled_mode = mpMediaConfig->dsp_config.tiled_mode;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].frm_chroma_fmt_max = mpMediaConfig->dsp_config.frm_chroma_fmt_max;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].dec_types = 0x3f;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].max_frm_num = mpMediaConfig->dsp_config.max_frm_num;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].max_frm_width = mpMediaConfig->dsp_config.max_frm_width;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].max_frm_height = mpMediaConfig->dsp_config.max_frm_height;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].max_fifo_size = mpMediaConfig->dsp_config.max_fifo_size;
    mpMediaConfig->dsp_config.udecInstanceConfig[0].bits_fifo_size = mpMediaConfig->dsp_config.bits_fifo_size;

    mpMediaConfig->dsp_config.voutConfigs.src_pos_x = 0;
    mpMediaConfig->dsp_config.voutConfigs.src_pos_y = 0;
    mpMediaConfig->dsp_config.voutConfigs.src_size_x = mpMediaConfig->dsp_config.max_frm_width;
    mpMediaConfig->dsp_config.voutConfigs.src_size_y = mpMediaConfig->dsp_config.max_frm_height;

    LOG_NOTICE("[setupUdecDSPSettings]: udec_num %u, voutmask %08x\n", mpMediaConfig->dsp_config.modeConfig.num_udecs, mpMediaConfig->dsp_config.modeConfig.vout_mask);
    LOG_NOTICE("[setupUdecDSPSettings]: mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_width %u, mPersistMediaConfig.dsp_config.udecInstanceConfig[0].max_frm_height %u.\n", mpMediaConfig->dsp_config.udecInstanceConfig[0].max_frm_width, mpMediaConfig->dsp_config.udecInstanceConfig[0].max_frm_height);

    return EECode_OK;
}

EECode CSimpleAPI::setupMUdecDSPSettings()
{
    TUint i;
    EECode err;

    DASSERT((EMediaDeviceWorkingMode_MultipleInstancePlayback == mpContext->setting.work_mode) || (EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding == mpContext->setting.work_mode));

    memset(&mCurrentVideoPostPGlobalSetting, 0x0, sizeof(mCurrentVideoPostPGlobalSetting));
    memset(&mCurrentDisplayLayout, 0x0, sizeof(mCurrentDisplayLayout));
    memset(&mCurrentVideoPostPConfig, 0x0, sizeof(mCurrentVideoPostPConfig));

    //set initial display setting
    mCurrentDisplayLayout.cur_window_start_index = 0;
    mCurrentDisplayLayout.cur_render_start_index = 0;
    mCurrentDisplayLayout.cur_decoder_start_index = 0;

    switch (mpContext->setting.playback_mode) {

        case EPlaybackMode_1x1080p:
        case EPlaybackMode_1x3M:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 1;
            mCurrentDisplayLayout.total_window_number = 1;
            mCurrentDisplayLayout.total_render_number = 1;
            mCurrentDisplayLayout.total_decoder_number = 1;
            mCurrentDisplayLayout.cur_window_number = 1;
            mCurrentDisplayLayout.cur_render_number = 1;
            mCurrentDisplayLayout.cur_decoder_number = 1;
            break;

        case EPlaybackMode_1x1080p_4xD1:
        case EPlaybackMode_1x3M_4xD1:
            mCurrentDisplayLayout.layer_number = 2;
            mCurrentDisplayLayout.layer_window_number[0] = 4;
            mCurrentDisplayLayout.layer_window_number[1] = 1;
            mCurrentDisplayLayout.total_window_number = 5;
            mCurrentDisplayLayout.total_render_number = 5;
            mCurrentDisplayLayout.total_decoder_number = 5;
            mCurrentDisplayLayout.cur_window_number = 5;
            mCurrentDisplayLayout.cur_render_number = 4;
            mCurrentDisplayLayout.cur_decoder_number = 5;
            break;

        case EPlaybackMode_1x1080p_6xD1:
        case EPlaybackMode_1x3M_6xD1:
            mCurrentDisplayLayout.layer_number = 2;
            mCurrentDisplayLayout.layer_window_number[0] = 6;
            mCurrentDisplayLayout.layer_window_number[1] = 1;
            mCurrentDisplayLayout.total_window_number = 7;
            mCurrentDisplayLayout.total_render_number = 7;
            mCurrentDisplayLayout.total_decoder_number = 7;
            mCurrentDisplayLayout.cur_window_number = 7;
            mCurrentDisplayLayout.cur_render_number = 6;
            mCurrentDisplayLayout.cur_decoder_number = 7;
            break;

        case EPlaybackMode_1x1080p_8xD1:
        case EPlaybackMode_1x3M_8xD1:
            mCurrentDisplayLayout.layer_number = 2;
            mCurrentDisplayLayout.layer_window_number[0] = 8;
            mCurrentDisplayLayout.layer_window_number[1] = 1;
            mCurrentDisplayLayout.total_window_number = 9;
            mCurrentDisplayLayout.total_render_number = 9;
            mCurrentDisplayLayout.total_decoder_number = 9;
            mCurrentDisplayLayout.cur_window_number = 9;
            mCurrentDisplayLayout.cur_render_number = 8;
            mCurrentDisplayLayout.cur_decoder_number = 9;
            break;

        case EPlaybackMode_1x1080p_9xD1:
        case EPlaybackMode_1x3M_9xD1:
            mCurrentDisplayLayout.layer_number = 2;
            mCurrentDisplayLayout.layer_window_number[0] = 9;
            mCurrentDisplayLayout.layer_window_number[1] = 1;
            mCurrentDisplayLayout.total_window_number = 10;
            mCurrentDisplayLayout.total_render_number = 10;
            mCurrentDisplayLayout.total_decoder_number = 10;
            mCurrentDisplayLayout.cur_window_number = 10;
            mCurrentDisplayLayout.cur_render_number = 9;
            mCurrentDisplayLayout.cur_decoder_number = 10;
            break;

        case EPlaybackMode_4x720p:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 4;
            mCurrentDisplayLayout.total_window_number = 4;
            mCurrentDisplayLayout.total_render_number = 4;
            mCurrentDisplayLayout.total_decoder_number = 4;
            mCurrentDisplayLayout.cur_window_number = 4;
            mCurrentDisplayLayout.cur_render_number = 4;
            mCurrentDisplayLayout.cur_decoder_number = 4;
            break;

        case EPlaybackMode_4xD1:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 4;
            mCurrentDisplayLayout.total_window_number = 4;
            mCurrentDisplayLayout.total_render_number = 4;
            mCurrentDisplayLayout.total_decoder_number = 4;
            mCurrentDisplayLayout.cur_window_number = 4;
            mCurrentDisplayLayout.cur_render_number = 4;
            mCurrentDisplayLayout.cur_decoder_number = 4;
            break;

        case EPlaybackMode_6xD1:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 6;
            mCurrentDisplayLayout.total_window_number = 6;
            mCurrentDisplayLayout.total_render_number = 6;
            mCurrentDisplayLayout.total_decoder_number = 6;
            mCurrentDisplayLayout.cur_window_number = 6;
            mCurrentDisplayLayout.cur_render_number = 6;
            mCurrentDisplayLayout.cur_decoder_number = 6;
            break;

        case EPlaybackMode_8xD1:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 8;
            mCurrentDisplayLayout.total_window_number = 8;
            mCurrentDisplayLayout.total_render_number = 8;
            mCurrentDisplayLayout.total_decoder_number = 8;
            mCurrentDisplayLayout.cur_window_number = 8;
            mCurrentDisplayLayout.cur_render_number = 8;
            mCurrentDisplayLayout.cur_decoder_number = 8;
            break;

        case EPlaybackMode_9xD1:
            mCurrentDisplayLayout.layer_number = 1;
            mCurrentDisplayLayout.layer_window_number[0] = 9;
            mCurrentDisplayLayout.total_window_number = 9;
            mCurrentDisplayLayout.total_render_number = 9;
            mCurrentDisplayLayout.total_decoder_number = 9;
            mCurrentDisplayLayout.cur_window_number = 9;
            mCurrentDisplayLayout.cur_render_number = 9;
            mCurrentDisplayLayout.cur_decoder_number = 9;
            break;

        default:
            LOG_FATAL("BAD mpContext->setting.playback_mode %d\n", mpContext->setting.playback_mode);
            return EECode_InternalLogicalBug;
            break;
    }

    mCurrentDisplayLayout.layer_layout_type[0] = EDisplayLayout_Rectangle;
    mCurrentDisplayLayout.layer_layout_type[1] = EDisplayLayout_Rectangle;

    mpMediaConfig->dsp_config.modeConfig.enable_transcode = (EDSPOperationMode_MultipleWindowUDEC_Transcode == mRequestDSPMode);
    mpMediaConfig->dsp_config.modeConfig.postp_mode = 3;
    mpMediaConfig->dsp_config.enable_deint = 0;
    mpMediaConfig->dsp_config.modeConfig.pp_chroma_fmt_max = 2;
    mpMediaConfig->dsp_config.modeConfig.pp_max_frm_num = 5;

    if (mpContext->setting.udec_specify_transcode_framerate) {
        switch (mpContext->setting.transcode_framerate) {

            case 30:
                mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick = DDefaultVideoFramerateDen;
                break;

            case 15:
                mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick = DDefaultVideoFramerateDen * 2;
                break;

            case 10:
                mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick = DDefaultVideoFramerateDen * 3;
                break;

            case 7:
                mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick = DDefaultVideoFramerateDen * 4;
                break;

            default:
                LOG_WARN("not support framerate %d, use 29.97 as default\n", mpContext->setting.transcode_framerate);
                mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick = DDefaultVideoFramerateDen;
                break;
        }
    } else {
        mpContext->setting.transcode_framerate = DDefaultVideoFramerateDen;
    }

    if (mpContext->setting.udec_specify_transcode_bitrate) {
        DASSERT(mpContext->setting.transcode_bitrate);
        mpMediaConfig->dsp_config.modeConfigMudec.avg_bitrate = mpContext->setting.transcode_bitrate;
    } else {
        mpMediaConfig->dsp_config.modeConfigMudec.avg_bitrate = 1 * 1024 * 1024;
    }

    LOG_PRINTF("[CSimpleAPI]: transcode framerate %d, tick %d, bitrate %d\n", mpContext->setting.transcode_framerate, mpMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick, mpMediaConfig->dsp_config.modeConfigMudec.avg_bitrate);

    mpMediaConfig->dsp_config.modeConfigMudec.av_sync_enabled = 0;

    if (mpMediaConfig->dsp_config.modeConfig.vout_mask & (DCAL_BITMASK(EDisplayDevice_LCD))) {
        mpMediaConfig->dsp_config.modeConfigMudec.voutA_enabled = 1;
    }

    if (mpMediaConfig->dsp_config.modeConfig.vout_mask & (DCAL_BITMASK(EDisplayDevice_HDMI))) {
        mpMediaConfig->dsp_config.modeConfigMudec.voutB_enabled = 1;
    }

    mpMediaConfig->dsp_config.modeConfigMudec.audio_on_win_id = 0;

    if (mpMediaConfig->dsp_config.modeConfig.primary_vout < EDisplayDevice_TotCount) {
        mpMediaConfig->dsp_config.modeConfigMudec.video_win_width = mpMediaConfig->dsp_config.voutConfigs.voutConfig[mpMediaConfig->dsp_config.modeConfig.primary_vout].width;
        mpMediaConfig->dsp_config.modeConfigMudec.video_win_height = mpMediaConfig->dsp_config.voutConfigs.voutConfig[mpMediaConfig->dsp_config.modeConfig.primary_vout].height;
    } else {
        LOG_FATAL("Internal bug, BAD mpMediaConfig->dsp_config.modeConfig.primary_vout %d\n", mpMediaConfig->dsp_config.modeConfig.primary_vout);
        return EECode_BadParam;
    }

    for (i = 0; i < DMaxUDECInstanceNumber; i++) {
        mpMediaConfig->dsp_config.udecInstanceConfig[i].tiled_mode = mpMediaConfig->dsp_config.tiled_mode;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = mpMediaConfig->dsp_config.frm_chroma_fmt_max;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].dec_types = 0x17;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].max_fifo_size = mpMediaConfig->dsp_config.max_fifo_size;
        mpMediaConfig->dsp_config.udecInstanceConfig[i].bits_fifo_size = mpMediaConfig->dsp_config.bits_fifo_size;
    }

    mCurrentVideoPostPGlobalSetting.display_mask = mpMediaConfig->dsp_config.modeConfig.vout_mask;
    mCurrentVideoPostPGlobalSetting.primary_display_index = mpMediaConfig->dsp_config.modeConfig.primary_vout;
    mCurrentVideoPostPGlobalSetting.cur_display_mask = mpMediaConfig->dsp_config.modeConfig.vout_mask;
    mCurrentVideoPostPGlobalSetting.display_width = mpMediaConfig->dsp_config.modeConfigMudec.video_win_width;
    mCurrentVideoPostPGlobalSetting.display_height = mpMediaConfig->dsp_config.modeConfigMudec.video_win_height;

    err = gfPresetDisplayLayout(&mCurrentVideoPostPGlobalSetting, &mCurrentDisplayLayout, &mCurrentVideoPostPConfig);
    DASSERT_OK(err);

    err = gfDefaultRenderConfig((SDSPMUdecRender *)&mCurrentVideoPostPConfig.render[0], mCurrentVideoPostPConfig.cur_render_number, mCurrentVideoPostPConfig.total_render_number - mCurrentVideoPostPConfig.cur_render_number);
    DASSERT_OK(err);

    DASSERT(mCurrentVideoPostPConfig.total_window_number);//debug assert
    DASSERT(mCurrentVideoPostPConfig.total_window_number <= DMaxPostPWindowNumber);//debug assert

    //window config
    for (i = 0; i < mCurrentVideoPostPConfig.total_window_number; i++) {
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].win_config_id = mCurrentVideoPostPConfig.window[i].win_config_id;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].input_offset_x = mCurrentVideoPostPConfig.window[i].input_offset_x;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].input_offset_y = mCurrentVideoPostPConfig.window[i].input_offset_y;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].input_width = mCurrentVideoPostPConfig.window[i].input_width;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].input_height = mCurrentVideoPostPConfig.window[i].input_height;

        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].target_win_offset_x = mCurrentVideoPostPConfig.window[i].target_win_offset_x;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].target_win_offset_y = mCurrentVideoPostPConfig.window[i].target_win_offset_y;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].target_win_width = mCurrentVideoPostPConfig.window[i].target_win_width;
        mpMediaConfig->dsp_config.modeConfigMudec.windows_config[i].target_win_height = mCurrentVideoPostPConfig.window[i].target_win_height;
    }

    DASSERT(mCurrentVideoPostPConfig.total_render_number);//debug assert
    DASSERT(mCurrentVideoPostPConfig.total_render_number <= DMaxPostPRenderNumber);//debug assert
    //render config
    for (i = 0; i < mCurrentVideoPostPConfig.total_render_number; i++) {
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].render_id = mCurrentVideoPostPConfig.render[i].render_id;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].win_config_id = mCurrentVideoPostPConfig.render[i].win_config_id;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].win_config_id_2nd = mCurrentVideoPostPConfig.render[i].win_config_id_2nd;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].udec_id = mCurrentVideoPostPConfig.render[i].udec_id;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].first_pts_low = mCurrentVideoPostPConfig.render[i].first_pts_low;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].first_pts_high = mCurrentVideoPostPConfig.render[i].first_pts_high;
        mpMediaConfig->dsp_config.modeConfigMudec.render_config[i].input_source_type = mCurrentVideoPostPConfig.render[i].input_source_type;
    }

    mpMediaConfig->dsp_config.modeConfig.num_udecs = mpContext->setting.udec_instance_number + mpContext->setting.udec_2rd_instance_number;

    mpMediaConfig->dsp_config.modeConfigMudec.total_num_render_configs = mCurrentVideoPostPConfig.cur_render_number;
    mpMediaConfig->dsp_config.modeConfigMudec.total_num_win_configs = mCurrentVideoPostPConfig.total_window_number;
    mpMediaConfig->dsp_config.modeConfigMudec.max_num_windows = mCurrentVideoPostPConfig.total_window_number + 1;
    LOG_PRINTF("[CSimpleAPI]: udec_num %u, win_num %u, render_num %u, voutmask %08x.\n", mpMediaConfig->dsp_config.modeConfig.num_udecs, mpMediaConfig->dsp_config.modeConfigMudec.total_num_win_configs, mpMediaConfig->dsp_config.modeConfigMudec.total_num_render_configs, mpMediaConfig->dsp_config.modeConfig.vout_mask);

    return EECode_OK;
}

void CSimpleAPI::initializeSyncStatus()
{
    TU32 index = 0;

    mCurrentSyncStatus.version.native_major = DCloudLibVesionMajor;
    mCurrentSyncStatus.version.native_minor = DCloudLibVesionMinor;
    mCurrentSyncStatus.version.native_patch = DCloudLibVesionPatch;

    mCurrentSyncStatus.version.native_date_year = DCloudLibVesionYear;
    mCurrentSyncStatus.version.native_date_month = DCloudLibVesionMonth;
    mCurrentSyncStatus.version.native_date_day = DCloudLibVesionDay;

    mCurrentSyncStatus.channel_number_per_group = mpContext->channel_number_per_group;
    mCurrentSyncStatus.total_group_number = mpContext->group_number;
    mCurrentSyncStatus.current_group_index = mpContext->current_group_index;
    mCurrentSyncStatus.have_dual_stream = mpContext->have_dual_stream;

    mCurrentSyncStatus.is_vod_enabled = mpContext->is_vod_enabled;
    mCurrentSyncStatus.is_vod_ready = mpContext->is_vod_ready;

    LOG_NOTICE("channel_number_per_group %d, total_group_number %d\n", mCurrentSyncStatus.channel_number_per_group, mCurrentSyncStatus.channel_number_per_group);

    mCurrentSyncStatus.audio_source_index = 0;
    mCurrentSyncStatus.audio_target_index = 0;

    mCurrentSyncStatus.encoding_width = mCurrentVideoPostPGlobalSetting.display_width;
    mCurrentSyncStatus.encoding_height = mCurrentVideoPostPGlobalSetting.display_height;
    mCurrentSyncStatus.encoding_bitrate = 1024 * 1024; //hard code here
    mCurrentSyncStatus.encoding_framerate = 30;//hard code here

    mCurrentSyncStatus.display_layout = EDisplayLayout_Rectangle;
    mCurrentSyncStatus.display_hd_channel_index = 0;

    mCurrentSyncStatus.total_window_number = mCurrentDisplayLayout.total_window_number;
    mCurrentSyncStatus.current_display_window_number = mCurrentDisplayLayout.layer_window_number[0];

    for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
        mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
        mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
        mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
        mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
        mCurrentSyncStatus.window[index].udec_index = index;
        mCurrentSyncStatus.window[index].render_index = index;
        mCurrentSyncStatus.window[index].display_disable = 0;

        mCurrentSyncStatus.zoom[index].zoom_size_x = 720;
        mCurrentSyncStatus.zoom[index].zoom_size_y = 480;
        mCurrentSyncStatus.zoom[index].zoom_input_center_x = 720 / 2;
        mCurrentSyncStatus.zoom[index].zoom_input_center_y = 480 / 2;
        mCurrentSyncStatus.zoom[index].udec_index = index;

        mCurrentSyncStatus.source_info[index].pic_width = 720;
        mCurrentSyncStatus.source_info[index].pic_height = 480;
        mCurrentSyncStatus.source_info[index].bitrate = 1 * 1024 * 1024;
        mCurrentSyncStatus.source_info[index].framerate = 30;

        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].pic_width = 1920;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].pic_height = 1080;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].bitrate = 4 * 1024 * 1024;
        mCurrentSyncStatus.source_info[index + mCurrentSyncStatus.channel_number_per_group].framerate = 30;
    }

    for (; index < mCurrentSyncStatus.total_window_number; index ++) {
        mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
        mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
        mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
        mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
        mCurrentSyncStatus.window[index].udec_index = index;
        mCurrentSyncStatus.window[index].render_index = index;
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.zoom[index].zoom_size_x = 1920;
        mCurrentSyncStatus.zoom[index].zoom_size_y = 1080;
        mCurrentSyncStatus.zoom[index].zoom_input_center_x = 1920 / 2;
        mCurrentSyncStatus.zoom[index].zoom_input_center_y = 1080 / 2;
        mCurrentSyncStatus.zoom[index].udec_index = index;
    }

    resetZoomParameters();

}

EECode CSimpleAPI::receiveSyncStatus(ICloudServerAgent *p_agent, TMemSize payload_size)
{
    TU8 *p_payload = mpCommunicationBuffer;
    TMemSize read_length = 0;

    LOG_FATAL("not support!\n");

    if (DUnlikely((!p_agent) || (!payload_size))) {
        LOG_FATAL("NULL p_agent %p or zero size %ld\n", p_agent, payload_size);
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely(payload_size > mCommunicationBufferSize)) {
        LOG_WARN("buffer size not enough, request %ld, re-alloc\n", payload_size);
        if (mpCommunicationBuffer) {
            free(mpCommunicationBuffer);
            mpCommunicationBuffer = NULL;
        }

        mCommunicationBufferSize = payload_size + 128;
        mpCommunicationBuffer = (TU8 *)malloc(mCommunicationBufferSize);
        if (DUnlikely(!mpCommunicationBuffer)) {
            LOG_FATAL("no memory, request size %ld\n", mCommunicationBufferSize);
            return EECode_NoMemory;
        }
    }

    EECode err = p_agent->ReadData(mpCommunicationBuffer, payload_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("p_agent->ReadData(, %ld) error, ret %s\n", payload_size, gfGetErrorCodeString(err));
        return err;
    }

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    ESACPElementType element_type;
    TU32 window_index = 0;
    TU32 zoom_index = 0;

    while (payload_size) {

        read_length = gfReadSACPElement(p_payload, payload_size, element_type, param0, param1, param2, param3, param4, param5, param6, param7);
        if (payload_size < read_length) {
            LOG_ERROR("internal logic bug\n");
            return EECode_Error;
        }

        switch (element_type) {

            case ESACPElementType_GlobalSetting: {
                    mInputSyncStatus.channel_number_per_group = param0;
                    mInputSyncStatus.total_group_number = param1;
                    mInputSyncStatus.current_group_index = param2;
                    mInputSyncStatus.have_dual_stream = param3;
                    mInputSyncStatus.is_vod_enabled = param4;
                    mInputSyncStatus.is_vod_ready = param5;
                    mInputSyncStatus.total_window_number = param6;
                    mInputSyncStatus.current_display_window_number = param7;
                }
                break;

            case ESACPElementType_SyncFlags: {
                    mInputSyncStatus.update_group_info_flag = param0;
                    mInputSyncStatus.update_display_layout_flag = param1;
                    mInputSyncStatus.update_source_flag = param2;
                    mInputSyncStatus.update_display_flag = param3;
                    mInputSyncStatus.update_audio_flag = param4;
                    mInputSyncStatus.update_zoom_flag = param5;
                }
                break;

            case ESACPElementType_EncodingParameters:
                mInputSyncStatus.encoding_width = param0;
                mInputSyncStatus.encoding_height = param1;
                mInputSyncStatus.encoding_bitrate = param2;
                mInputSyncStatus.encoding_framerate = param3;
                break;

            case ESACPElementType_DisplaylayoutInfo:
                mInputSyncStatus.display_layout = param0;
                mInputSyncStatus.display_hd_channel_index = param1;
                mInputSyncStatus.audio_source_index = param2;
                mInputSyncStatus.audio_target_index = param3;
                break;

            case ESACPElementType_DisplayWindowInfo:
                if (DLikely(window_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    mInputSyncStatus.window[window_index].window_pos_x = (TU32)((((TU64)param0) * mInputSyncStatus.encoding_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_pos_y = (TU32)((((TU64)param1) * mInputSyncStatus.encoding_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_width = (TU32)((((TU64)param2) * mInputSyncStatus.encoding_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].window_height = (TU32)((((TU64)param3) * mInputSyncStatus.encoding_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.window[window_index].udec_index = param4;
                    mInputSyncStatus.window[window_index].render_index = param5;
                    mInputSyncStatus.window[window_index].display_disable = param6;
                    window_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            case ESACPElementType_DisplayZoomInfo:
                if (DLikely(zoom_index < DQUERY_MAX_DISPLAY_WINDOW_NUMBER)) {
                    mInputSyncStatus.zoom[zoom_index].zoom_size_x = (TU32)((((TU64)param0) * mInputSyncStatus.source_info[zoom_index].pic_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_size_y = (TU32)((((TU64)param1) * mInputSyncStatus.source_info[zoom_index].pic_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_input_center_x = (TU32)((((TU64)param2) * mInputSyncStatus.source_info[zoom_index].pic_width) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].zoom_input_center_y = (TU32)((((TU64)param0) * mInputSyncStatus.source_info[zoom_index].pic_height) / DSACP_FIX_POINT_DEN);
                    mInputSyncStatus.zoom[zoom_index].udec_index = param4;
                    mInputSyncStatus.zoom[zoom_index].is_valid = 1;
                    zoom_index ++;
                } else {
                    LOG_ERROR("too many windows\n");
                    //return EECode_Error;
                }
                break;

            default:
                LOG_FATAL("BAD ESACPElementType %d\n", element_type);
                break;
        }

        payload_size -= read_length;
        p_payload += read_length;
    }

    return EECode_OK;
}

EECode CSimpleAPI::sendSyncStatus(ICloudServerAgent *p_agent)
{
    DASSERT(mpCommunicationBuffer);
    DASSERT(mCommunicationBufferSize);

    TU8 *p_payload = mpCommunicationBuffer + sizeof(SSACPHeader);
    SSACPHeader *p_header = (SSACPHeader *) mpCommunicationBuffer;
    TMemSize write_length = 0;
    TMemSize total_payload_size = 0;
    TMemSize remain_payload_size = mCommunicationBufferSize - sizeof(SSACPHeader);

    memset(p_header, 0x0, sizeof(SSACPHeader));

    TU32 param0, param1, param2, param3, param4, param5, param6, param7;
    TU32 window_index = 0;
    TU32 zoom_index = 0;
    TU32 total_num = mCurrentSyncStatus.total_window_number;

    param0 = mCurrentSyncStatus.channel_number_per_group;
    param1 = mCurrentSyncStatus.total_group_number;
    param2 = mCurrentSyncStatus.current_group_index;
    param3 = mCurrentSyncStatus.have_dual_stream;
    param4 = mCurrentSyncStatus.is_vod_enabled;
    param5 = mCurrentSyncStatus.is_vod_ready;
    param6 = mCurrentSyncStatus.total_window_number;
    param7 = mCurrentSyncStatus.current_display_window_number;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_GlobalSetting, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.update_group_info_flag;
    param1 = mCurrentSyncStatus.update_display_layout_flag;
    param2 = mCurrentSyncStatus.update_source_flag;
    param3 = mCurrentSyncStatus.update_display_flag;
    param4 = mCurrentSyncStatus.update_audio_flag;
    param5 = mCurrentSyncStatus.update_zoom_flag;

    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_SyncFlags, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.encoding_width;
    param1 = mCurrentSyncStatus.encoding_height;
    param2 = mCurrentSyncStatus.encoding_bitrate;
    param3 = mCurrentSyncStatus.encoding_framerate;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_EncodingParameters, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    param0 = mCurrentSyncStatus.display_layout;
    param1 = mCurrentSyncStatus.display_hd_channel_index;
    param2 = mCurrentSyncStatus.audio_source_index;
    param3 = mCurrentSyncStatus.audio_target_index;
    write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplaylayoutInfo, param0, param1, param2, param3, param4, param5, param6, param7);
    total_payload_size += write_length;
    remain_payload_size -= write_length;
    p_payload += write_length;

    for (window_index = 0; window_index < total_num; window_index ++) {
        param0 = mCurrentSyncStatus.window[window_index].window_pos_x;
        param1 = mCurrentSyncStatus.window[window_index].window_pos_y;
        param2 = mCurrentSyncStatus.window[window_index].window_width;
        param3 = mCurrentSyncStatus.window[window_index].window_height;
        param4 = mCurrentSyncStatus.window[window_index].udec_index;
        param5 = mCurrentSyncStatus.window[window_index].render_index;
        param6 = mCurrentSyncStatus.window[window_index].display_disable;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayWindowInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    for (zoom_index = 0; zoom_index < total_num; zoom_index ++) {
        param0 = mCurrentSyncStatus.zoom[zoom_index].zoom_size_x;
        param1 = mCurrentSyncStatus.zoom[zoom_index].zoom_size_y;
        param2 = mCurrentSyncStatus.zoom[zoom_index].zoom_input_center_x;
        param3 = mCurrentSyncStatus.zoom[zoom_index].zoom_input_center_y;
        param4 = mCurrentSyncStatus.zoom[zoom_index].udec_index;

        write_length = gfWriteSACPElement(p_payload, remain_payload_size, ESACPElementType_DisplayZoomInfo, param0, param1, param2, param3, param4, param5, param6, param7);
        total_payload_size += write_length;
        remain_payload_size -= write_length;
        p_payload += write_length;
    }

    p_header->type_1 |= (DSACPTypeBit_Responce >> 24);

    p_header->size_1 = (total_payload_size >> 8) & 0xff;
    p_header->size_2 = total_payload_size & 0xff;
    p_header->size_0 = (total_payload_size >> 16) & 0xff;

    total_payload_size += sizeof(SSACPHeader);
    LOG_PRINTF("sendSyncStatus, total_payload_size %ld\n", total_payload_size);

    EECode err = p_agent->WriteData(mpCommunicationBuffer, total_payload_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("WriteData error %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    LOG_PRINTF("sendSyncStatus end\n");

    return EECode_OK;
}

EECode CSimpleAPI::updateSyncStatus()
{
    LOG_WARN("CSimpleAPI::updateSyncStatus TO DO\n");
    return EECode_NoImplement;
}

EECode CSimpleAPI::sendCloudAgentVersion(ICloudServerAgent *p_agent, TU32 param0, TU32 param1)
{
    if (DUnlikely(!p_agent)) {
        LOG_ERROR("NULL p_agent\n");
        return EECode_InternalLogicalBug;
    }

    if (DUnlikely((!mpCommunicationBuffer) || (!mCommunicationBufferSize))) {
        LOG_ERROR("NULL mpCommunicationBuffer %p or zero mCommunicationBufferSize %ld\n", mpCommunicationBuffer, mCommunicationBufferSize);
        return EECode_InternalLogicalBug;
    }

    mCurrentSyncStatus.version.peer_major = (param0 >> 16);
    mCurrentSyncStatus.version.peer_minor = (param0 >> 8) & 0xff;
    mCurrentSyncStatus.version.peer_patch = param0 & 0xff;

    mCurrentSyncStatus.version.peer_date_year = (param1 >> 16);
    mCurrentSyncStatus.version.peer_date_month = (param1 >> 8) & 0xff;
    mCurrentSyncStatus.version.peer_date_day = param1 & 0xff;

    //write back
    TU8 *p_payload = mpCommunicationBuffer + sizeof(SSACPHeader);
    SSACPHeader *p_header = (SSACPHeader *) mpCommunicationBuffer;
    TMemSize write_length = 0;

    memset(p_header, 0x0, sizeof(SSACPHeader));

    DBEW16(mCurrentSyncStatus.version.native_major, p_payload);
    p_payload += 2;
    *p_payload ++ = mCurrentSyncStatus.version.native_minor;
    *p_payload ++ = mCurrentSyncStatus.version.native_patch;
    DBEW16(mCurrentSyncStatus.version.native_date_year, p_payload);
    p_payload += 2;
    *p_payload ++ = mCurrentSyncStatus.version.native_date_month;
    *p_payload ++ = mCurrentSyncStatus.version.native_date_day;

    write_length = 8;

    p_header->type_1 |= (DSACPTypeBit_Responce >> 24);

    p_header->size_1 = (write_length >> 8) & 0xff;
    p_header->size_2 = write_length & 0xff;
    p_header->size_0 = (write_length >> 16) & 0xff;

    write_length += sizeof(SSACPHeader);
    LOG_PRINTF("sendCloudAgentVersion, write_length %ld\n", write_length);

    EECode err = p_agent->WriteData(mpCommunicationBuffer, write_length);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("WriteData error %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    LOG_PRINTF("sendCloudAgentVersion end\n");

    return EECode_OK;
}

EECode CSimpleAPI::processUdecStatusUpdateMsg(SMSG &msg)
{
    DASSERT(EMSGType_NotifyUDECStateChanges == msg.code);

    //LOG_DEBUG("[DEBUG]: processUdecStatusUpdateMsg, msg.identifyer_count %d, mSwitchLayeroutIdentifyerCount %d, mbSdInHdWindow %d, mFocusIndex %d\n", msg.identifyer_count, mSwitchLayeroutIdentifyerCount, mbSdInHdWindow, mFocusIndex);

    if (msg.identifyer_count == mSwitchLayeroutIdentifyerCount) {
        if (mbSdInHdWindow) {
            //LOG_DEBUG("[DEBUG]: mFocusIndex %d\n", mFocusIndex);
            mbSdInHdWindow = 0;
            mCurrentVideoPostPConfig.render[0].udec_id = mCurrentSyncStatus.channel_number_per_group;
            mCurrentSyncStatus.window[0].udec_index = mCurrentSyncStatus.channel_number_per_group;

            if (DLikely(mpEngineControl)) {
                SGenericVideoPostPConfiguration ppinfo;
                ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
                ppinfo.set_or_get = 1;
                ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

                mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
                mCurrentVideoPostPConfig.single_view_mode = 0;
                EECode err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
                mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

                if (DUnlikely(EECode_OK != err)) {
                    LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            } else {
                LOG_FATAL("NULL mpEngineControl\n");
                return EECode_InternalLogicalBug;
            }

            suspendPlaybackPipelines(mFocusIndex, mFocusIndex, mCurrentSyncStatus.current_group, 1, 1);
        } else {
            //to do

        }

    }

    return EECode_OK;
}

EECode CSimpleAPI::processUdecResolutionUpdateMsg(SMSG &msg)
{
    DASSERT(EMSGType_NotifyUDECUpdateResolution == msg.code);

    LOG_DEBUG("[DEBUG]: processUdecResolutionUpdateMsg, msg.owner_index %d, msg.p0 %ld, msg.p1 %ld\n", msg.owner_index, msg.p0, msg.p1);

    if (DLikely(msg.owner_index < DQUERY_MAX_SOURCE_NUMBER)) {
        mCurrentSyncStatus.source_info[msg.owner_index].pic_width = msg.p0;
        mCurrentSyncStatus.source_info[msg.owner_index].pic_height = msg.p1;
    } else {
        LOG_FATAL("BAD msg.owner_index %d\n", msg.owner_index);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processUpdateMudecDisplayLayout(TU32 layout, TU32 focus_index)
{
    EECode err = EECode_OK;
    TUint index = 0;
    TUint udec_index = 0;
    TUint hd_udec_index = mCurrentSyncStatus.channel_number_per_group;
    TUint update_window_params = 1;

    DASSERT(mCurrentSyncStatus.have_dual_stream);

    LOG_NOTICE("[DEBUG] processUpdateMudecDisplayLayout, input (layout %d, focus_index %d)\n", layout, focus_index);

    if (mCurrentSyncStatus.display_layout == layout) {
        if (mCurrentSyncStatus.display_hd_channel_index == focus_index) {
            LOG_WARN("not changed, return\n");
        }
        LOG_PRINTF("update hd index\n");
        update_window_params = 0;
    } else {
        resetZoomParameters();
    }

    if (EDisplayLayout_TelePresence == layout) {

        if (focus_index >= mCurrentSyncStatus.channel_number_per_group) {
            LOG_FATAL("focus_index %d exceed max %d\n", focus_index, mCurrentSyncStatus.channel_number_per_group);
            return EECode_BadParam;
        }

        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)mCurrentSyncStatus.channel_number_per_group, 0);
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        mCurrentSyncStatus.window[0].udec_index = hd_udec_index;
        mCurrentSyncStatus.window[0].render_index = 0;
        mCurrentSyncStatus.window[0].display_disable = 0;

        mCurrentVideoPostPConfig.render[0].render_id = 0;
        mCurrentVideoPostPConfig.render[0].udec_id = hd_udec_index;
        mCurrentVideoPostPConfig.render[0].win_config_id = 0;

        for (index = 1, udec_index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++, udec_index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            if (udec_index == focus_index) {
                udec_index ++;
            }
            mCurrentVideoPostPConfig.render[index].udec_id = udec_index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = udec_index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        suspendPlaybackPipelines(focus_index, focus_index, mCurrentSyncStatus.current_group, 1, 1);
        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            if (index != focus_index) {
                resumePlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 1);
            }
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            if (index != focus_index) {
                suspendPlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 0, 1);
            }
        }
        resumePlaybackPipelines(focus_index, focus_index, mCurrentSyncStatus.current_group, 0);

    } else if (EDisplayLayout_Rectangle == layout) {
        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)hd_udec_index, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }
        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 0, 1);
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);

    } else if (EDisplayLayout_BottomLeftHighLighten == layout) {
        if (focus_index >= mCurrentSyncStatus.channel_number_per_group) {
            LOG_FATAL("focus_index %d exceed max %d\n", focus_index, mCurrentSyncStatus.channel_number_per_group);
            return EECode_BadParam;
        }

        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)hd_udec_index, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 0, 1);
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);

    } else if (EDisplayLayout_SingleWindowView == layout) {
        mCurrentVideoPostPConfig.window[0].win_config_id = 0;

        mCurrentVideoPostPConfig.window[0].input_width = 0;
        mCurrentVideoPostPConfig.window[0].input_height = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_y = 0;

        mCurrentVideoPostPConfig.window[0].target_win_width = mCurrentVideoPostPGlobalSetting.display_width;
        mCurrentVideoPostPConfig.window[0].target_win_height = mCurrentVideoPostPGlobalSetting.display_height;
        mCurrentVideoPostPConfig.window[0].target_win_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].target_win_offset_y = 0;

        mCurrentSyncStatus.window[0].window_width = mCurrentVideoPostPConfig.window[0].target_win_width;
        mCurrentSyncStatus.window[0].window_height = mCurrentVideoPostPConfig.window[0].target_win_height;
        mCurrentSyncStatus.window[0].window_pos_x = mCurrentVideoPostPConfig.window[0].target_win_offset_x;
        mCurrentSyncStatus.window[0].window_pos_y = mCurrentVideoPostPConfig.window[0].target_win_offset_y;

        mCurrentVideoPostPConfig.total_num_win_configs = 2;
        mCurrentVideoPostPConfig.total_window_number = 1;
        mCurrentVideoPostPConfig.total_render_number = 1;
        mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

        if (mbSmoothSwitchLayout && (mCurrentSyncStatus.display_layout != EDisplayLayout_SingleWindowView) && (mpContext->gragh_component[mpContext->current_group_index].playback_2rd_pipeline_id[focus_index]) && (!mpContext->gragh_component[mpContext->current_group_index].playback_pipeline_2rd_suspended[focus_index])) {
            mCurrentVideoPostPConfig.render[0].udec_id = focus_index;
            mCurrentSyncStatus.window[0].udec_index = focus_index;
            mbSdInHdWindow = 1;
            mSwitchLayeroutIdentifyerCount ++;
            mpMediaConfig->identifyer_count = mSwitchLayeroutIdentifyerCount;
            LOG_FATAL("[DEBUG]: try smooth stream switch for better user experience\n");
            mFocusIndex = focus_index;
        } else {
            //hard code here
            mCurrentVideoPostPConfig.render[0].udec_id = mCurrentSyncStatus.channel_number_per_group;
            mCurrentSyncStatus.window[0].udec_index = mCurrentSyncStatus.channel_number_per_group;
            mbSdInHdWindow = 0;
            LOG_FATAL("[DEBUG]: try fast stream switch\n");
        }

        mCurrentVideoPostPConfig.cur_window_number = 1;
        mCurrentVideoPostPConfig.cur_render_number = 1;
        mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

        mCurrentVideoPostPConfig.cur_window_start_index = 0;
        mCurrentVideoPostPConfig.cur_render_start_index = 0;
        mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;

        for (index = 1; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentSyncStatus.window[index].display_disable = 1;
        }
        mCurrentSyncStatus.window[index].display_disable = 1;

        mCurrentSyncStatus.current_display_window_number = 1;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }
        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        if (!mbSdInHdWindow) {
            DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
            suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1, 1);
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                if (index != focus_index) {
                    suspendPlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 0, 1);
                }
            }
            resumePlaybackPipelines((TU8)focus_index, (TU8)focus_index, mCurrentSyncStatus.current_group, 0);
        } else {
            DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                if (index != focus_index) {
                    suspendPlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 1, 1);
                }
            }
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                if (index != focus_index) {
                    suspendPlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 0, 1);
                }
            }
            resumePlaybackPipelines((TU8)focus_index, (TU8)focus_index, mCurrentSyncStatus.current_group, 0);
        }

    } else {
        LOG_WARN("CSimpleAPI 1 TO DO\n");
        return EECode_NotSupported;
    }

    LOG_NOTICE("[DEBUG] processUpdateMudecDisplayLayout end, input (layout %d, focus_index %d)\n", layout, focus_index);
    mCurrentSyncStatus.display_layout = layout;
    mCurrentSyncStatus.display_hd_channel_index = focus_index;

    return EECode_OK;
}

void CSimpleAPI::resetZoomParameters()
{
    TU32 i = 0;

    for (i = 0; i < DQUERY_MAX_DISPLAY_WINDOW_NUMBER; i ++) {
        mCurrentSyncStatus.window_zoom_history[i].zoom_factor = 1;
        mCurrentSyncStatus.window_zoom_history[i].center_x = 0.5;
        mCurrentSyncStatus.window_zoom_history[i].center_y = 0.5;
    }
}

EECode CSimpleAPI::processZoom(TU32 window_id, TU32 width, TU32 height, TU32 input_center_x, TU32 input_center_y)
{
    TU32 render_id = 0;
    TU32 udec_id = 0;

    LOG_NOTICE("[DEBUG] processZoom, input (window_id %d, width %08x, height %08x, center_x %08x, center_y %08x)\n", render_id, width, height, input_center_x, input_center_y);

    if (DUnlikely(window_id >= mCurrentSyncStatus.current_display_window_number)) {
        LOG_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    render_id = mCurrentSyncStatus.window[window_id].render_index;
    udec_id = mCurrentSyncStatus.window[window_id].udec_index;

    input_center_x = (TU32)(((TU64)input_center_x * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_width) / DSACP_FIX_POINT_DEN);
    input_center_y = (TU32)(((TU64)input_center_y * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_height) / DSACP_FIX_POINT_DEN);
    width = (TU32)(((TU64)width * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_width) / DSACP_FIX_POINT_DEN);
    height = (TU32)(((TU64)height * (TU64)mCurrentSyncStatus.source_info[udec_id].pic_height) / DSACP_FIX_POINT_DEN);

    if (DUnlikely((2 * input_center_x) < width)) {
        LOG_WARN("input_center_x (%d) too small, set to width(%d)/2, %d\n", input_center_x, width, width / 2);
        input_center_x = width / 2;
    } else if (DUnlikely((input_center_x + width / 2) > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOG_WARN("input_center_x (%d) too large, set to tot_width(%d) - width(%d)/2, %d\n", input_center_x, mCurrentSyncStatus.source_info[udec_id].pic_width, width, mCurrentSyncStatus.source_info[udec_id].pic_width - width / 2);
        input_center_x = mCurrentSyncStatus.source_info[udec_id].pic_width - width / 2;
    }

    if (DUnlikely((2 * input_center_y) < height)) {
        LOG_WARN("input_center_y (%d) too small, set to height(%d)/2, %d\n", input_center_y, height, height / 2);
        input_center_y = height / 2;
    } else if (DUnlikely((input_center_y + height / 2) > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOG_WARN("input_center_y (%d) too large, set to tot_height(%d) - height(%d)/2, %d\n", input_center_y, mCurrentSyncStatus.source_info[udec_id].pic_height, height, mCurrentSyncStatus.source_info[udec_id].pic_height - height / 2);
        input_center_y = mCurrentSyncStatus.source_info[udec_id].pic_height - height / 2;
    }

    LOG_NOTICE("[DEBUG] processZoom, after convert params (render_id %d, udec_id %d, width %d, height %d, center_x %d, center_y %d)\n", render_id, udec_id, width, height, input_center_x, input_center_y);

    SPlaybackZoom playback_zoom;
    playback_zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
    playback_zoom.render_id = render_id;
    playback_zoom.input_center_x = input_center_x;
    playback_zoom.input_center_y = input_center_y;
    playback_zoom.input_width = width;
    playback_zoom.input_height = height;

    if (DLikely(mpEngineControl)) {
        EECode err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&playback_zoom);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("EGenericEngineConfigure_VideoPlayback_Zoom ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return EECode_BadParam;
    }

    mCurrentSyncStatus.zoom[udec_id].zoom_size_x = width;
    mCurrentSyncStatus.zoom[udec_id].zoom_size_y = height;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_x = input_center_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_y = input_center_y;

    return EECode_OK;
}

EECode CSimpleAPI::processZoom(TU32 window_id, TU32 zoom_factor, TU32 offset_center_x, TU32 offset_center_y)
{
    TU32 render_id = 0;
    TU32 udec_id = 0;

    LOG_NOTICE("[DEBUG] processZoom, input (window_id %d, zoom_factor %08x, center_x %08x, center_y %08x)\n", render_id, zoom_factor, offset_center_x, offset_center_y);

    if (window_id >= mCurrentSyncStatus.current_display_window_number) {
        LOG_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    render_id = mCurrentSyncStatus.window[window_id].render_index;
    udec_id = mCurrentSyncStatus.window[window_id].udec_index;

    float cur_zoom_factor = mCurrentSyncStatus.window_zoom_history[window_id].zoom_factor;
    float cur_center_x = mCurrentSyncStatus.window_zoom_history[window_id].center_x, cur_center_y = mCurrentSyncStatus.window_zoom_history[window_id].center_y;

    LOG_NOTICE("[DEBUG], current zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);
    if (offset_center_x & DSACP_FIX_POINT_SYGN_FLAG) {
        offset_center_x &= ~(DSACP_FIX_POINT_SYGN_FLAG);
        float offset = ((float)offset_center_x / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_x += offset;
        LOG_NOTICE("to right, offset %f, after %f\n", offset, cur_center_x);
    } else {
        float offset = ((float)offset_center_x / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        LOG_NOTICE("to left, offset %f, after %f\n", offset, cur_center_x);
        cur_center_x -= offset;
    }
    if (offset_center_y & DSACP_FIX_POINT_SYGN_FLAG) {
        offset_center_y &= ~(DSACP_FIX_POINT_SYGN_FLAG);
        float offset = ((float)offset_center_y / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_y += offset;
        LOG_NOTICE("to down, offset %f, after %f\n", offset, cur_center_y);
    } else {
        float offset = ((float)offset_center_y / (float)DSACP_FIX_POINT_DEN) / cur_zoom_factor;
        cur_center_y -= offset;
        LOG_NOTICE("to up, offset %f, after %f\n", offset, cur_center_y);
    }
    cur_zoom_factor *= (float)zoom_factor / (float)DSACP_FIX_POINT_DEN;
    LOG_NOTICE("[DEBUG], after caluculate: zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);

    if (DUnlikely(cur_zoom_factor < 1)) {
        LOG_NOTICE("zoom factor (%f) less than 1, reset to original size\n", cur_zoom_factor);
        cur_center_x = 0.5;
        cur_center_y = 0.5;
        cur_zoom_factor = 1.0;
    } else {
        float cur_half_size = 1.0 / cur_zoom_factor / 2.0;
        if (cur_half_size > cur_center_x) {
            cur_center_x = cur_half_size;
        }
        if ((cur_half_size + cur_center_x) > 1) {
            cur_center_x = 1 - cur_half_size;
        }

        if (cur_half_size > cur_center_y) {
            cur_center_y = cur_half_size;
        }

        if ((cur_half_size + cur_center_y) > 1) {
            cur_center_y = 1 - cur_half_size;
        }
    }
    LOG_NOTICE("[DEBUG], after adjust: zoom_factor %f, center_x %f, center_y %f\n", cur_zoom_factor, cur_center_x, cur_center_y);

    TU32 c_size_x = mCurrentSyncStatus.source_info[udec_id].pic_width / cur_zoom_factor;
    TU32 c_size_y = mCurrentSyncStatus.source_info[udec_id].pic_height / cur_zoom_factor;
    TU32 c_offset_x = mCurrentSyncStatus.source_info[udec_id].pic_width * cur_center_x;
    TU32 c_offset_y = mCurrentSyncStatus.source_info[udec_id].pic_height * cur_center_y;

    LOG_NOTICE("[DEBUG], udec %d source info: width %d, height %d\n", udec_id, mCurrentSyncStatus.source_info[udec_id].pic_width, mCurrentSyncStatus.source_info[udec_id].pic_height);

    c_offset_x &= ~(0x1);
    c_size_x &= ~(0x1);

    if (DUnlikely(c_offset_x > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOG_ERROR("reset!\n");
        c_offset_x = mCurrentSyncStatus.source_info[udec_id].pic_width / 2;
    }

    if (DUnlikely(c_offset_x < ((c_size_x + 1) / 2))) {
        c_size_x = c_offset_x * 2;
    }

    if (DUnlikely((c_offset_x + ((c_size_x + 1) / 2)) > mCurrentSyncStatus.source_info[udec_id].pic_width)) {
        LOG_WARN("x size exceed\n");
        c_size_x = (mCurrentSyncStatus.source_info[udec_id].pic_width - c_offset_x) * 2;
    }

    if (DUnlikely(c_offset_y > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOG_ERROR("reset!\n");
        c_offset_y = mCurrentSyncStatus.source_info[udec_id].pic_height / 2;
    }

    if (DUnlikely(c_offset_y < ((c_size_y + 1) / 2))) {
        c_size_y = c_offset_y * 2;
    }

    if (DUnlikely((c_offset_y + ((c_size_y + 1) / 2)) > mCurrentSyncStatus.source_info[udec_id].pic_height)) {
        LOG_WARN("y size exceed\n");
        c_size_y = (mCurrentSyncStatus.source_info[udec_id].pic_height - c_offset_y) * 2;
    }

    float zoom_factor_y = (float)mCurrentSyncStatus.source_info[udec_id].pic_height / (float)c_size_y;
    float zoom_factor_x = (float)mCurrentSyncStatus.source_info[udec_id].pic_width / (float)c_size_x;

    if ((zoom_factor_y + 0.1) < zoom_factor_x) {
        cur_zoom_factor = zoom_factor_y;
    } else {
        cur_zoom_factor = zoom_factor_x;
    }

    LOG_NOTICE("[DEBUG] processZoom, after convert params (render_id %d, udec_id %d, width %d, height %d, center_x %d, center_y %d)\n", render_id, udec_id, c_size_x, c_size_y, c_offset_x, c_offset_y);

    SPlaybackZoom playback_zoom;
    playback_zoom.check_field = EGenericEngineConfigure_VideoPlayback_Zoom;
    playback_zoom.render_id = render_id;
    playback_zoom.input_center_x = c_offset_x;
    playback_zoom.input_center_y = c_offset_y;
    playback_zoom.input_width = c_size_x;
    playback_zoom.input_height = c_size_y;

    if (DLikely(mpEngineControl)) {
        EECode err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_Zoom, (void *)&playback_zoom);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("EGenericEngineConfigure_VideoPlayback_Zoom ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return EECode_BadParam;
    }

    mCurrentSyncStatus.window_zoom_history[window_id].zoom_factor = cur_zoom_factor;
    mCurrentSyncStatus.window_zoom_history[window_id].center_x = cur_center_x;
    mCurrentSyncStatus.window_zoom_history[window_id].center_y = cur_center_y;

    mCurrentSyncStatus.zoom[udec_id].zoom_size_x = c_size_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_size_y = c_size_y;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_x = c_offset_x;
    mCurrentSyncStatus.zoom[udec_id].zoom_input_center_y = c_offset_y;

    return EECode_OK;
}

EECode CSimpleAPI::processMoveWindow(TU32 window_id, TU32 pos_x, TU32 pos_y, TU32 width, TU32 height)
{
    EECode err = EECode_OK;

    LOG_NOTICE("[DEBUG] processMoveWindow, input (window_id %d, width %08x, height %08x, pos_x %08x, pos_y %08x)\n", window_id, width, height, pos_x, pos_y);

    if (window_id >= mCurrentSyncStatus.current_display_window_number) {
        LOG_ERROR("BAD window id %d\n", window_id);
        return EECode_BadParam;
    }

    width = (((TU64)mCurrentSyncStatus.encoding_width) * (TU64)width) / DSACP_FIX_POINT_DEN;
    height = (((TU64)mCurrentSyncStatus.encoding_height) * (TU64)height) / DSACP_FIX_POINT_DEN;
    pos_x = (((TU64)mCurrentSyncStatus.encoding_width) * (TU64)pos_x) / DSACP_FIX_POINT_DEN;
    pos_y = (((TU64)mCurrentSyncStatus.encoding_height) * (TU64)pos_y) / DSACP_FIX_POINT_DEN;

    pos_x = (pos_x + 8) & (~15);
    width = (width + 8) & (~15);

    mCurrentVideoPostPConfig.window[window_id].target_win_width = width;
    mCurrentVideoPostPConfig.window[window_id].target_win_height = height;
    mCurrentVideoPostPConfig.window[window_id].target_win_offset_x = pos_x;
    mCurrentVideoPostPConfig.window[window_id].target_win_offset_y = pos_y;

    mCurrentSyncStatus.window[window_id].window_width = width;
    mCurrentSyncStatus.window[window_id].window_height = height;
    mCurrentSyncStatus.window[window_id].window_pos_x = pos_x;
    mCurrentSyncStatus.window[window_id].window_pos_y = pos_y;

    LOG_NOTICE("[DEBUG] processMoveWindow, after convert (window_id %d, width %d, height %d, pos_x %d, pos_y %d)\n", window_id, width, height, pos_x, pos_y);

    if (DLikely(mpEngineControl)) {
        SGenericVideoPostPConfiguration ppinfo;
        ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_UpdateOneWindow;
        ppinfo.set_or_get = 1;
        ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
        mCurrentVideoPostPConfig.single_view_mode = 0;
        mCurrentVideoPostPConfig.input_param_update_window_index = window_id;
        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_UpdateOneWindow, &ppinfo);
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

        if (DUnlikely(EECode_OK != err)) {
            LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return err;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processShowOtherWindows(TU32 window_index, TU32 show)
{
    EECode err = EECode_OK;

    if (window_index >= mCurrentSyncStatus.current_display_window_number) {
        LOG_ERROR("BAD window id %d\n", window_index);
        return EECode_BadParam;
    }

    if (!show) {
        LOG_NOTICE("hide others, window index %d\n", window_index);
        mCurrentVideoPostPConfig.single_view_mode = 1;
        mCurrentVideoPostPConfig.single_view_render.render_id = 0;
        mCurrentVideoPostPConfig.single_view_render.win_config_id = 0;
        mCurrentVideoPostPConfig.single_view_render.win_config_id_2nd = 0xff;
        mCurrentVideoPostPConfig.single_view_render.input_source_type = 1;
        mCurrentVideoPostPConfig.single_view_render.first_pts_low = 0;
        mCurrentVideoPostPConfig.single_view_render.first_pts_high = 0;
        mCurrentVideoPostPConfig.single_view_render.udec_id = mCurrentSyncStatus.window[window_index].udec_index;

        mCurrentVideoPostPConfig.single_view_window.win_config_id = 0;
        mCurrentVideoPostPConfig.single_view_window.input_offset_x = 0;
        mCurrentVideoPostPConfig.single_view_window.input_offset_y = 0;
        mCurrentVideoPostPConfig.single_view_window.input_width = 0;
        mCurrentVideoPostPConfig.single_view_window.input_height = 0;

        mCurrentVideoPostPConfig.single_view_window.target_win_offset_x = mCurrentSyncStatus.window[window_index].window_pos_x;
        mCurrentVideoPostPConfig.single_view_window.target_win_offset_y = mCurrentSyncStatus.window[window_index].window_pos_y;
        mCurrentVideoPostPConfig.single_view_window.target_win_width = mCurrentSyncStatus.window[window_index].window_width;
        mCurrentVideoPostPConfig.single_view_window.target_win_height = mCurrentSyncStatus.window[window_index].window_height;

        mCurrentSyncStatus.show_other_windows = 0;
    } else {
        LOG_NOTICE("show others, window index %d\n", window_index);
        mCurrentVideoPostPConfig.single_view_mode = 0;
        mCurrentSyncStatus.show_other_windows = 1;
    }

    if (DLikely(mpEngineControl)) {
        SGenericVideoPostPConfiguration ppinfo;
        ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
        ppinfo.set_or_get = 1;
        ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
        mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

        if (DUnlikely(EECode_OK != err)) {
            LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return err;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processDemandIDR(TU32 param1)
{
    EECode err = EECode_OK;

    if (DLikely(mpEngineControl)) {
        SGenericVideoEncoderParam params;
        memset(&params, 0x0, sizeof(params));
        params.check_field = EGenericEngineConfigure_VideoEncoder_DemandIDR;
        params.set_or_get = 1;

        params.video_params.index = 0;//hard code here
        params.video_params.demand_idr_strategy = 1;//hard code here

        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoEncoder_DemandIDR, (void *)&params);
        if (EECode_OK != err) {
            LOG_FATAL("EGenericEngineConfigure_VideoEncoder_DemandIDR return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processUpdateBitrate(TU32 param1)
{
    EECode err = EECode_OK;

    if (DLikely(mpEngineControl)) {
        SGenericVideoEncoderParam params;
        memset(&params, 0x0, sizeof(params));

        params.check_field = EGenericEngineConfigure_VideoEncoder_Bitrate;
        params.set_or_get = 1;
        params.video_params.index = 0;
        params.video_params.bitrate = param1;
        params.video_params.bitrate_pts = 0;

        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoEncoder_Bitrate, (void *)&params);
        if (EECode_OK != err) {
            LOG_FATAL("EGenericEngineConfigure_VideoEncoder_Bitrate return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpDSPAPI\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processUpdateFramerate(TU32 param1)
{
    EECode err = EECode_OK;

    if (DLikely(mpEngineControl)) {
        SGenericVideoEncoderParam params;
        memset(&params, 0x0, sizeof(params));

        params.check_field = EGenericEngineConfigure_VideoEncoder_Framerate;
        params.set_or_get = 1;

        params.video_params.index = 0;//hard code here
        params.video_params.framerate_reduce_factor = 0;
        if (param1 & 0xffff0000) {
            params.video_params.framerate_integer = 30 / (param1 >> 16);
            LOG_NOTICE("framerate reduction %d, framerate %d\n", (param1 >> 16), params.video_params.framerate_integer);
        } else {
            params.video_params.framerate_integer = (param1) & 0xff;
            LOG_NOTICE("framerate %d\n", params.video_params.framerate_integer);
        }

        err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoEncoder_Framerate, (void *)&params);
        if (EECode_OK != err) {
            LOG_FATAL("EGenericEngineConfigure_VideoEncoder_Framerate return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpEngineControl\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processSwapWindowContent(TU32 window_id_1, TU32 window_id_2)
{
    EECode err = EECode_OK;
    TComponentIndex udec_index_tmp_1 = 0;
    TComponentIndex udec_index_tmp_2 = 0;
    TComponentIndex hd_udec_index = mCurrentSyncStatus.display_hd_channel_index;

    LOG_NOTICE("[DEBUG] processSwapWindowContent (window_id_1 %d, window_id_2 %d)\n", window_id_1, window_id_2);

    if (DUnlikely(window_id_1 == window_id_2)) {
        LOG_ERROR("window_id is the same? %d %d\n", window_id_1, window_id_2);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_1 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOG_ERROR("window_id_1 %d exceed max value %d\n", window_id_1, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_2 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOG_ERROR("window_id_2 %d exceed max value %d\n", window_id_2, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) {
        LOG_NOTICE("in EDisplayLayout_TelePresence layout\n");
        udec_index_tmp_1 = mCurrentSyncStatus.window[window_id_1].udec_index;
        udec_index_tmp_2 = mCurrentSyncStatus.window[window_id_2].udec_index;
        TComponentIndex current_hd_index = 0;
        TComponentIndex current_sd_index = 0;

        if ((udec_index_tmp_1 != hd_udec_index) && (udec_index_tmp_2 != hd_udec_index)) {
            mCurrentSyncStatus.window[window_id_1].udec_index = udec_index_tmp_2;
            mCurrentSyncStatus.window[window_id_2].udec_index = udec_index_tmp_1;

            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = udec_index_tmp_2;
            mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = udec_index_tmp_1;

        } else {
            if (udec_index_tmp_1 == hd_udec_index) {
                current_hd_index = mCurrentSyncStatus.display_hd_channel_index;
                current_sd_index = udec_index_tmp_2;
                mCurrentSyncStatus.window[window_id_2].udec_index = current_hd_index;
                mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = current_hd_index;
            } else if (udec_index_tmp_2 == hd_udec_index) {
                current_hd_index = mCurrentSyncStatus.display_hd_channel_index;
                current_sd_index = udec_index_tmp_1;
                mCurrentSyncStatus.window[window_id_1].udec_index = current_hd_index;
                mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = current_hd_index;
            } else {
                LOG_ERROR("why comes here, udec_index_tmp_1 %d, udec_index_tmp_2 %d, hd_udec_index %d, mCurrentSyncStatus.display_hd_channel_index %d\n", udec_index_tmp_1, udec_index_tmp_2, hd_udec_index, mCurrentSyncStatus.display_hd_channel_index);
                return EECode_BadParam;
            }

            suspendPlaybackPipelines(current_hd_index, current_hd_index, mCurrentSyncStatus.current_group, 0, 1);
            suspendPlaybackPipelines(current_sd_index, current_sd_index, mCurrentSyncStatus.current_group, 1, 1);
            mCurrentSyncStatus.display_hd_channel_index = current_sd_index;
            resumePlaybackPipelines(current_hd_index, current_hd_index, mCurrentSyncStatus.current_group, 1);
            resumePlaybackPipelines(current_sd_index, current_sd_index, mCurrentSyncStatus.current_group, 0);

            mCurrentSyncStatus.window[window_id_2].udec_index = mCurrentSyncStatus.display_hd_channel_index;
        }

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

    } else if ((EDisplayLayout_Rectangle == mCurrentSyncStatus.display_layout) || (EDisplayLayout_BottomLeftHighLighten == mCurrentSyncStatus.display_layout)) {
        udec_index_tmp_1 = mCurrentSyncStatus.window[window_id_1].udec_index;
        udec_index_tmp_2 = mCurrentSyncStatus.window[window_id_2].udec_index;

        mCurrentSyncStatus.window[window_id_1].udec_index = udec_index_tmp_2;
        mCurrentSyncStatus.window[window_id_2].udec_index = udec_index_tmp_1;

        mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = udec_index_tmp_2;
        mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = udec_index_tmp_1;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

    } else {
        LOG_ERROR("not expected %d\n", mCurrentSyncStatus.display_layout);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processCircularShiftWindowContent(TU32 shift, TU32 is_ccw)
{
    LOG_WARN("processCircularShiftWindowContent TO DO\n");

    return EECode_OK;
}

EECode CSimpleAPI::processSwitchGroup(TU32 group, TU32 param_0)
{
    DASSERT(mCurrentSyncStatus.have_dual_stream);

    if (DUnlikely(group >= mCurrentSyncStatus.total_group_number)) {
        LOG_ERROR("BAD group number %d, total_group_number %d\n", group, mCurrentSyncStatus.total_group_number);
        return EECode_BadParam;
    }

    if (DUnlikely(group == mCurrentSyncStatus.current_group)) {
        LOG_WARN("group %d not changed\n", group);
        return EECode_OK;
    }

    //suspend previous
    suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 0, 1);
    suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1, 1);

    //resume current
    TU8 index = 0;
    mCurrentSyncStatus.current_group = group;
    if (EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) {

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            if (index != mCurrentSyncStatus.display_hd_channel_index) {
                resumePlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 1);
            } else {
                resumePlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 0);
            }
        }

    } else if (EDisplayLayout_Rectangle == mCurrentSyncStatus.display_layout) {
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);
    } else if (EDisplayLayout_BottomLeftHighLighten == mCurrentSyncStatus.display_layout) {
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);
    } else if (EDisplayLayout_SingleWindowView == mCurrentSyncStatus.display_layout) {
        resumePlaybackPipelines((TU8)mCurrentSyncStatus.display_hd_channel_index, (TU8)mCurrentSyncStatus.display_hd_channel_index, mCurrentSyncStatus.current_group, 0);
    } else {
        LOG_WARN("processSwitchGroup TO DO\n");
        return EECode_NotSupported;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processUpdateMudecDisplayLayout_no_dual_stream(TU32 layout, TU32 focus_index)
{
    EECode err = EECode_OK;
    TUint index = 0;
    TUint udec_index = 0;
    TUint update_window_params = 1;

    DASSERT(!mCurrentSyncStatus.have_dual_stream);

    LOG_NOTICE("[DEBUG] processUpdateMudecDisplayLayout_no_dual_stream, input (layout %d, focus_index %d)\n", layout, focus_index);

    if (mCurrentSyncStatus.display_layout == layout) {
        if (mCurrentSyncStatus.display_hd_channel_index == focus_index) {
            LOG_WARN("not changed, return\n");
        }
        LOG_PRINTF("update hd index\n");
        update_window_params = 0;
    } else {
        resetZoomParameters();
    }

    if (EDisplayLayout_TelePresence == layout) {

        if (focus_index >= mCurrentSyncStatus.channel_number_per_group) {
            LOG_FATAL("focus_index %d exceed max %d\n", focus_index, mCurrentSyncStatus.channel_number_per_group);
            return EECode_BadParam;
        }

        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)mCurrentSyncStatus.channel_number_per_group, 0);
            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group + 1;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        mCurrentSyncStatus.window[0].udec_index = focus_index;
        mCurrentSyncStatus.window[0].render_index = 0;
        mCurrentSyncStatus.window[0].display_disable = 0;

        mCurrentVideoPostPConfig.render[0].render_id = 0;
        mCurrentVideoPostPConfig.render[0].udec_id = focus_index;
        mCurrentVideoPostPConfig.render[0].win_config_id = 0;

        for (index = 1, udec_index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++, udec_index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            if (udec_index == focus_index) {
                udec_index ++;
            }
            mCurrentVideoPostPConfig.render[index].udec_id = udec_index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = udec_index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);

    } else if (EDisplayLayout_Rectangle == layout) {
        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)mCurrentSyncStatus.channel_number_per_group, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }
        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);

    } else if (EDisplayLayout_BottomLeftHighLighten == layout) {
        if (focus_index >= mCurrentSyncStatus.channel_number_per_group) {
            LOG_FATAL("focus_index %d exceed max %d\n", focus_index, mCurrentSyncStatus.channel_number_per_group);
            return EECode_BadParam;
        }

        if (update_window_params) {
            calculateDisplayLayoutWindow(&mCurrentVideoPostPConfig.window[0], mCurrentVideoPostPGlobalSetting.display_width, mCurrentVideoPostPGlobalSetting.display_height, (TU8)layout, (TU8)mCurrentSyncStatus.channel_number_per_group, 0);

            for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
                mCurrentSyncStatus.window[index].window_width = mCurrentVideoPostPConfig.window[index].target_win_width;
                mCurrentSyncStatus.window[index].window_height = mCurrentVideoPostPConfig.window[index].target_win_height;
                mCurrentSyncStatus.window[index].window_pos_x = mCurrentVideoPostPConfig.window[index].target_win_offset_x;
                mCurrentSyncStatus.window[index].window_pos_y = mCurrentVideoPostPConfig.window[index].target_win_offset_y;
            }

            mCurrentVideoPostPConfig.total_num_win_configs = mCurrentSyncStatus.channel_number_per_group + 1;
            mCurrentVideoPostPConfig.total_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group;

            mCurrentVideoPostPConfig.cur_window_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_render_number = mCurrentSyncStatus.channel_number_per_group;
            mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group;

            mCurrentVideoPostPConfig.cur_window_start_index = 0;
            mCurrentVideoPostPConfig.cur_render_start_index = 0;
            mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;
        }

        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentVideoPostPConfig.render[index].render_id = index;
            mCurrentVideoPostPConfig.render[index].udec_id = index;
            mCurrentVideoPostPConfig.render[index].win_config_id = index;

            mCurrentSyncStatus.window[index].udec_index = index;
            mCurrentSyncStatus.window[index].render_index = index;
            mCurrentSyncStatus.window[index].display_disable = 0;
        }

        mCurrentSyncStatus.current_display_window_number = mCurrentSyncStatus.channel_number_per_group;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);

    } else if (EDisplayLayout_SingleWindowView == layout) {
        mCurrentVideoPostPConfig.window[0].win_config_id = 0;

        mCurrentVideoPostPConfig.window[0].input_width = 0;
        mCurrentVideoPostPConfig.window[0].input_height = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].input_offset_y = 0;

        mCurrentVideoPostPConfig.window[0].target_win_width = mCurrentVideoPostPGlobalSetting.display_width;
        mCurrentVideoPostPConfig.window[0].target_win_height = mCurrentVideoPostPGlobalSetting.display_height;
        mCurrentVideoPostPConfig.window[0].target_win_offset_x = 0;
        mCurrentVideoPostPConfig.window[0].target_win_offset_y = 0;

        mCurrentSyncStatus.window[0].window_width = mCurrentVideoPostPConfig.window[0].target_win_width;
        mCurrentSyncStatus.window[0].window_height = mCurrentVideoPostPConfig.window[0].target_win_height;
        mCurrentSyncStatus.window[0].window_pos_x = mCurrentVideoPostPConfig.window[0].target_win_offset_x;
        mCurrentSyncStatus.window[0].window_pos_y = mCurrentVideoPostPConfig.window[0].target_win_offset_y;

        mCurrentVideoPostPConfig.total_num_win_configs = 2;
        mCurrentVideoPostPConfig.total_window_number = 1;
        mCurrentVideoPostPConfig.total_render_number = 1;
        mCurrentVideoPostPConfig.total_decoder_number = mCurrentSyncStatus.channel_number_per_group;

        //hard code here
        mCurrentVideoPostPConfig.render[0].udec_id = focus_index;
        mCurrentSyncStatus.window[0].udec_index = focus_index;

        mCurrentVideoPostPConfig.cur_window_number = 1;
        mCurrentVideoPostPConfig.cur_render_number = 1;
        mCurrentVideoPostPConfig.cur_decoder_number = mCurrentSyncStatus.channel_number_per_group;

        mCurrentVideoPostPConfig.cur_window_start_index = 0;
        mCurrentVideoPostPConfig.cur_render_start_index = 0;
        mCurrentVideoPostPConfig.cur_decoder_start_index  = 0;

        for (index = 1; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            mCurrentSyncStatus.window[index].display_disable = 1;
        }

        mCurrentSyncStatus.current_display_window_number = 1;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            mCurrentVideoPostPConfig.single_view_mode = 0;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }
        mCurrentSyncStatus.show_other_windows = 1;
        mCurrentSyncStatus.focus_window_id = 0;

        DASSERT(1 < mCurrentSyncStatus.channel_number_per_group);
        for (index = 0; index < mCurrentSyncStatus.channel_number_per_group; index ++) {
            if (index == focus_index) {
                continue;
            }
            suspendPlaybackPipelines(index, index, mCurrentSyncStatus.current_group, 1, 1);
        }
        resumePlaybackPipelines((TU8)focus_index, (TU8)focus_index, mCurrentSyncStatus.current_group, 1);

    } else {
        LOG_WARN("CSimpleAPI 2 TO DO\n");
        return EECode_NotSupported;
    }

    LOG_NOTICE("[DEBUG] processUpdateMudecDisplayLayout end, input (layout %d, focus_index %d)\n", layout, focus_index);
    mCurrentSyncStatus.display_layout = layout;
    mCurrentSyncStatus.display_hd_channel_index = focus_index;

    return EECode_OK;
}

EECode CSimpleAPI::processSwapWindowContent_no_dual_stream(TU32 window_id_1, TU32 window_id_2)
{
    EECode err = EECode_OK;
    TComponentIndex udec_index_tmp_1 = 0;
    TComponentIndex udec_index_tmp_2 = 0;

    DASSERT(!mCurrentSyncStatus.have_dual_stream);

    LOG_NOTICE("[DEBUG] processSwapWindowContent_no_dual_stream (window_id_1 %d, window_id_2 %d)\n", window_id_1, window_id_2);

    if (DUnlikely(window_id_1 == window_id_2)) {
        LOG_ERROR("window_id is the same? %d %d\n", window_id_1, window_id_2);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_1 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOG_ERROR("window_id_1 %d exceed max value %d\n", window_id_1, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (DUnlikely(window_id_2 >= mCurrentSyncStatus.channel_number_per_group)) {
        LOG_ERROR("window_id_2 %d exceed max value %d\n", window_id_2, mCurrentSyncStatus.channel_number_per_group);
        return EECode_BadParam;
    }

    if (DLikely((EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) || (EDisplayLayout_Rectangle == mCurrentSyncStatus.display_layout) || (EDisplayLayout_BottomLeftHighLighten == mCurrentSyncStatus.display_layout))) {

        udec_index_tmp_1 = mCurrentSyncStatus.window[window_id_1].udec_index;
        udec_index_tmp_2 = mCurrentSyncStatus.window[window_id_2].udec_index;

        mCurrentSyncStatus.window[window_id_1].udec_index = udec_index_tmp_2;
        mCurrentSyncStatus.window[window_id_2].udec_index = udec_index_tmp_1;

        mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_1].render_index].udec_id = udec_index_tmp_2;
        mCurrentVideoPostPConfig.render[mCurrentSyncStatus.window[window_id_2].render_index].udec_id = udec_index_tmp_1;

        if (DLikely(mpEngineControl)) {
            SGenericVideoPostPConfiguration ppinfo;
            ppinfo.check_field = EGenericEngineConfigure_VideoDisplay_PostPConfiguration;
            ppinfo.set_or_get = 1;
            ppinfo.p_video_postp_info = &mCurrentVideoPostPConfig;

            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 1;
            err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoDisplay_PostPConfiguration, &ppinfo);
            mCurrentVideoPostPConfig.set_config_direct_to_dsp = 0;

            if (DUnlikely(EECode_OK != err)) {
                LOG_FATAL("EGenericEngineConfigure_VideoDisplay_PostPConfiguration return %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOG_FATAL("NULL mpEngineControl\n");
            return err;
        }

    } else {
        LOG_ERROR("not expected %d\n", mCurrentSyncStatus.display_layout);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processSwitchGroup_no_dual_stream(TU32 group, TU32 param_0)
{
    DASSERT(!mCurrentSyncStatus.have_dual_stream);

    if (DUnlikely(group >= mCurrentSyncStatus.total_group_number)) {
        LOG_ERROR("BAD group number %d, total_group_number %d\n", group, mCurrentSyncStatus.total_group_number);
        return EECode_BadParam;
    }

    if (DUnlikely(group == mCurrentSyncStatus.current_group)) {
        LOG_WARN("group %d not changed\n", group);
        return EECode_OK;
    }

    //suspend previous
    suspendPlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1, 1);

    //resume current
    mCurrentSyncStatus.current_group = group;
    if ((EDisplayLayout_TelePresence == mCurrentSyncStatus.display_layout) || (EDisplayLayout_Rectangle == mCurrentSyncStatus.display_layout) || (EDisplayLayout_BottomLeftHighLighten == mCurrentSyncStatus.display_layout)) {
        resumePlaybackPipelines(0, mCurrentSyncStatus.channel_number_per_group - 1, mCurrentSyncStatus.current_group, 1);
    } else if (EDisplayLayout_SingleWindowView == mCurrentSyncStatus.display_layout) {
        resumePlaybackPipelines((TU8)mCurrentSyncStatus.display_hd_channel_index, (TU8)mCurrentSyncStatus.display_hd_channel_index, mCurrentSyncStatus.current_group, 1);
    } else {
        LOG_WARN("CSimpleAPI 3 TO DO\n");
        return EECode_NotSupported;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processPlaybackCapture(SUserParamType_PlaybackCapture *capture)
{
    if (DLikely(capture)) {
        SPlaybackCapture playback_capture;
        DASSERT(EUserParamType_PlaybackCapture == capture->check_field);

        memset(&playback_capture, 0x0, sizeof(playback_capture));
        playback_capture.check_field = EGenericEngineConfigure_VideoPlayback_Capture;

        playback_capture.capture_id = capture->channel_id;
        playback_capture.source_id = capture->source_id;
        playback_capture.capture_coded = capture->capture_coded;
        playback_capture.capture_screennail = capture->capture_screennail;
        playback_capture.capture_thumbnail = capture->capture_thumbnail;
        playback_capture.jpeg_quality = 50;
        playback_capture.capture_file_index = capture->file_index;

        if (capture->screennail_width && capture->screennail_height) {
            playback_capture.screennail_width = capture->screennail_width;
            playback_capture.screennail_height = capture->screennail_height;
        } else {
            playback_capture.screennail_width = mpMediaConfig->dsp_config.modeConfigMudec.video_win_width;
            playback_capture.screennail_height = mpMediaConfig->dsp_config.modeConfigMudec.video_win_height;
        }

        if (capture->coded_width && capture->coded_height) {
            playback_capture.coded_width = capture->coded_width;
            playback_capture.coded_height = capture->coded_height;
        } else {
            playback_capture.coded_width = playback_capture.screennail_width / 2;
            playback_capture.coded_height = playback_capture.screennail_height / 2;

        }

        if (capture->thumbnail_width && capture->thumbnail_height) {
            playback_capture.thumbnail_width = capture->thumbnail_width;
            playback_capture.thumbnail_height = capture->thumbnail_height;
        } else {
            playback_capture.thumbnail_width = 320;
            playback_capture.thumbnail_height = 240;
        }

        DASSERT(mpEngineControl);

        EECode err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_Capture, (void *) &playback_capture);
        if (EECode_OK != err) {
            LOG_ERROR("EGenericEngineConfigure_VideoPlayback_Capture fail, ret %d, %s", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL param in EUserParamType_PlaybackCapture\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CSimpleAPI::processPlaybackTrickplay(SUserParamType_PlaybackTrickplay *input_trickplay)
{
    if (DLikely(input_trickplay)) {
        SVideoPostPDisplayTrickPlay trickplay;
        EECode err = EECode_OK;

        if (input_trickplay->index >= (mpContext->setting.udec_instance_number + mpContext->setting.udec_2rd_instance_number)) {
            LOG_ERROR("BAD index %d\n", input_trickplay->index);
            return EECode_BadParam;
        }
        DASSERT((mpContext->setting.udec_instance_number + mpContext->setting.udec_2rd_instance_number) < (DMaxChannelNumberInGroup * 2));

        memset(&trickplay, 0x0, sizeof(trickplay));

        switch (input_trickplay->trickplay) {
            case SUserParamType_PlaybackTrickplay_Resume:
                if (DLikely((SUserParamType_PlaybackTrickplay_Pause == mpContext->udec_instance_current_trickplay[input_trickplay->index]) || (SUserParamType_PlaybackTrickplay_Step == mpContext->udec_instance_current_trickplay[input_trickplay->index]))) {
                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                    trickplay.id = input_trickplay->index;
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EGenericEngineConfigure_VideoPlayback_UDECResume fail ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->udec_instance_current_trickplay[input_trickplay->index] = SUserParamType_PlaybackTrickplay_Resume;
                } else {
                    LOG_ERROR("BAD mpContext->udec_instance_current_trickplay[%d] %d\n", input_trickplay->index, mpContext->udec_instance_current_trickplay[input_trickplay->index]);
                    return EECode_BadState;
                }
                break;

            case SUserParamType_PlaybackTrickplay_Pause:
                if (DLikely((SUserParamType_PlaybackTrickplay_Resume == mpContext->udec_instance_current_trickplay[input_trickplay->index]))) {
                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                    trickplay.id = input_trickplay->index;
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EGenericEngineConfigure_VideoPlayback_UDECPause fail ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->udec_instance_current_trickplay[input_trickplay->index] = SUserParamType_PlaybackTrickplay_Pause;
                } else {
                    LOG_ERROR("BAD mpContext->udec_instance_current_trickplay[%d] %d\n", input_trickplay->index, mpContext->udec_instance_current_trickplay[input_trickplay->index]);
                    return EECode_BadState;
                }
                break;

            case SUserParamType_PlaybackTrickplay_Step:
                if (DLikely((SUserParamType_PlaybackTrickplay_Resume == mpContext->udec_instance_current_trickplay[input_trickplay->index]) || (SUserParamType_PlaybackTrickplay_Step == mpContext->udec_instance_current_trickplay[input_trickplay->index]))) {
                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECStepPlay;
                    trickplay.id = input_trickplay->index;
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECStepPlay, (void *)&trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EGenericEngineConfigure_VideoPlayback_UDECStepPlay fail ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->udec_instance_current_trickplay[input_trickplay->index] = SUserParamType_PlaybackTrickplay_Step;
                } else {
                    LOG_ERROR("BAD mpContext->udec_instance_current_trickplay[%d] %d\n", input_trickplay->index, mpContext->udec_instance_current_trickplay[input_trickplay->index]);
                    return EECode_BadState;
                }
                break;

            case SUserParamType_PlaybackTrickplay_PauseResume:
                if ((SUserParamType_PlaybackTrickplay_Resume == mpContext->udec_instance_current_trickplay[input_trickplay->index])) {
                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECPause;
                    trickplay.id = input_trickplay->index;
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECPause, (void *)&trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EGenericEngineConfigure_VideoPlayback_UDECPause fail ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->udec_instance_current_trickplay[input_trickplay->index] = SUserParamType_PlaybackTrickplay_Pause;
                } else if ((SUserParamType_PlaybackTrickplay_Pause == mpContext->udec_instance_current_trickplay[input_trickplay->index]) || (SUserParamType_PlaybackTrickplay_Step == mpContext->udec_instance_current_trickplay[input_trickplay->index])) {
                    trickplay.check_field = EGenericEngineConfigure_VideoPlayback_UDECResume;
                    trickplay.id = input_trickplay->index;
                    err = mpEngineControl->GenericControl(EGenericEngineConfigure_VideoPlayback_UDECResume, (void *)&trickplay);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("EGenericEngineConfigure_VideoPlayback_UDECResume fail ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                    mpContext->udec_instance_current_trickplay[input_trickplay->index] = SUserParamType_PlaybackTrickplay_Resume;
                } else {
                    LOG_ERROR("BAD mpContext->udec_instance_current_trickplay[%d] %d\n", input_trickplay->index, mpContext->udec_instance_current_trickplay[input_trickplay->index]);
                    return EECode_BadState;
                }
                break;

            default:
                LOG_ERROR("BAD input_trickplay->trickplay %d\n", input_trickplay->trickplay);
                return EECode_BadParam;
                break;
        }
    } else {
        LOG_ERROR("NULL input_trickplay\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


