/*******************************************************************************
 * media_mw_utils.cpp
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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

#include "media_mw_if.h"

#include "media_mw_utils.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

typedef struct {
    TChar format_name[8];
    StreamFormat format;
} SFormatStringPair;

typedef struct {
    char entropy_name[8];
    EntropyType type;
} SEntropyTypeStringPair;

typedef struct {
    char container_name[8];
    ContainerType container_type;
} SContainerStringPair;

typedef struct {
    char sampleformat_name[8];
    AudioSampleFMT sampleformat;
} SSampleFormatStringPair;

static SFormatStringPair g_FormatStringPair[] = {
    {"h264", StreamFormat_H264},
    {"avc", StreamFormat_H264},
    {"aac", StreamFormat_AAC},
    {"mp2", StreamFormat_MP2},
    {"mp3", StreamFormat_MP2},
    {"ac3", StreamFormat_AC3},
    {"adpcm", StreamFormat_ADPCM},
    {"amrnb", StreamFormat_AMR_NB},
    {"amrwb", StreamFormat_AMR_WB},
    {"amr", StreamFormat_AMR_NB},
};

static SEntropyTypeStringPair g_EntropyTypeStringPair[] = {
    {"cabac", EntropyType_H264_CABAC},
    {"cavlc", EntropyType_H264_CAVLC},
};

static SContainerStringPair g_ContainerStringPair[] = {
    {"mp4", ContainerType_MP4},
    {"3gp", ContainerType_3GP},
    {"ts", ContainerType_TS},
    {"mov", ContainerType_MOV},
    {"mkv", ContainerType_MKV},
    {"avi", ContainerType_AVI},
    {"amr", ContainerType_AMR},
};

static SSampleFormatStringPair g_SampleFormatStringPair[] = {
    {"u8", AudioSampleFMT_U8},
    {"s16", AudioSampleFMT_S16},
    {"s32", AudioSampleFMT_S32},
    {"float", AudioSampleFMT_FLT},
    {"double", AudioSampleFMT_DBL},
};

static EECode calculateRectTableLayoutWindow(SDSPMUdecWindow *p_window, TU16 display_width, TU16 display_height, TU8 window_start_index, TU8 window_end_index, TU8 rows, TU8 cols)
{
    TUint cindex, rindex;
    TUint rect_w, rect_h;

    LOG_CONFIGURATION("[calculateRectTableLayoutWindow]: display_width %hu, display_height %hu, window_start_index %d, window_end_index %d, rows %d, cols %d\n", display_width, display_height, window_start_index, window_end_index, rows, cols);

    if (!p_window || !display_width || !display_height || !rows || !cols) {
        LOG_FATAL("zero input!!\n");
        return EECode_InternalLogicalBug;
    }

    rect_w = display_width / cols;
    rect_h = display_height / rows;

    for (rindex = 0; rindex < rows; rindex++) {
        for (cindex = 0; cindex < cols; cindex++) {

            if (window_start_index > window_end_index) {
                break;
            }

            p_window->win_config_id = window_start_index;

            p_window->target_win_offset_x = cindex * rect_w;
            p_window->target_win_offset_y = rindex * rect_h;
            p_window->target_win_width = rect_w;
            p_window->target_win_height = rect_h;

            p_window ++;
            window_start_index ++;
        }
    }

    return EECode_OK;
}

static TInt nextRectInBoundary(TU16 sub_window_width, TU16 sub_window_height, TU16 sub_window_offset_x, TU16 sub_window_offset_y, TS16 display_width, TS16 display_height)
{
    if ((sub_window_width + sub_window_offset_x) > display_width) {
        LOG_ERROR("width exceed!\n");
        return 1;
    } else if ((sub_window_height + sub_window_offset_y) > display_height) {
        LOG_ERROR("height exceed!\n");
        return 2;
    }

    return 0;
}

static EECode calculateTelePresenceLayoutWindow(SDSPMUdecWindow *p_window, TU16 display_width, TU16 display_height, TU8 window_start_index, TU8 window_end_index, TU8 sub_window_layout, TU16 sub_window_width, TU16 sub_window_height, TU16 sub_window_offset_x, TU16 sub_window_offset_y, TU16 additional_x, TU16 additional_y)
{
    SDSPMUdecWindow *p_last_window = NULL;
    //TUint direction = sub_window_layout;//0: right, 1: down, 2: left, 3: up, todo

    LOG_CONFIGURATION("[calculateTelePresenceLayoutWindow]: p_window %p, display_width %hu, display_height %hu, window_start_index %d, window_end_index %d, sub_window_width %u, sub_window_height %hu\n", p_window, display_width, display_height, window_start_index, window_end_index, sub_window_width, sub_window_height);

    if (!p_window || !display_width || !display_height || !sub_window_width || !sub_window_height) {
        LOG_FATAL("zero input!!\n");
        return EECode_InternalLogicalBug;
    }

    if (window_end_index < window_start_index) {
        LOG_FATAL("window_end_index (%hu) must greater than window_start_index (%hu)\n", window_end_index, window_start_index);
        return EECode_InternalLogicalBug;
    }

    //to do
    DASSERT(EDsplaySubWindowLayout_Horizontal_to_Right == sub_window_layout);

    p_last_window = p_window;
    p_last_window->win_config_id = window_start_index;
    p_last_window->target_win_offset_x = 0;
    p_last_window->target_win_offset_y = 0;
    p_last_window->target_win_width = display_width;
    p_last_window->target_win_height = display_height;

    LOG_CONFIGURATION("p_window %p, win_id %d, off_x %d, off_y %d, width %d, height %d\n", p_last_window, p_last_window->win_config_id, p_last_window->target_win_offset_x, p_last_window->target_win_offset_y, p_last_window->target_win_width, p_last_window->target_win_height);

    window_start_index += 1;
    p_window ++;

    for (; window_start_index <= window_end_index; window_start_index++) {

        if (nextRectInBoundary(sub_window_width, sub_window_height, sub_window_offset_x, sub_window_offset_y, display_width, display_height)) {
            break;
        }

        p_window->win_config_id = window_start_index;

        p_window->target_win_offset_x = sub_window_offset_x;
        p_window->target_win_offset_y = sub_window_offset_y;
        p_window->target_win_width = sub_window_width;
        p_window->target_win_height = sub_window_height;

        LOG_CONFIGURATION("p_window %p, win_id %d, off_x %d, off_y %d, width %d, height %d\n", p_window, p_window->win_config_id, p_window->target_win_offset_x, p_window->target_win_offset_y, p_window->target_win_width, p_window->target_win_height);

        p_window ++;

        if (EDsplaySubWindowLayout_Horizontal_to_Right == sub_window_layout) {
            sub_window_offset_x += additional_x;
        } else if (EDsplaySubWindowLayout_Vertical_to_Down == sub_window_layout) {
            sub_window_offset_y += additional_y;
        } else {
            LOG_FATAL("please implement sub_window_layout %d\n", sub_window_layout);
            return EECode_NoImplement;
        }

    }

    return EECode_OK;
}

static EECode calculateBottomLeftHighLightenLayoutWindow(SDSPMUdecWindow *p_window, TU16 display_width, TU16 display_height, TU8 window_start_index, TU8 window_end_index, TU8 rows, TU8 cols)
{
    TUint cindex, rindex;
    TUint rect_w, rect_h;

    LOG_CONFIGURATION("[calculateBottomLeftHighLightenLayoutWindow]: p_window %p, display_width %hu, display_height %hu, window_start_index %d, window_end_index %d, rows %d, cols %d\n", p_window, display_width, display_height, window_start_index, window_end_index, rows, cols);
    if (!p_window || !display_width || !display_height || !rows || !cols) {
        LOG_FATAL("zero input!!\n");
        return EECode_InternalLogicalBug;
    }

    rect_w = display_width / cols;
    rect_h = display_height / rows;

    //highlighten window
    p_window->win_config_id = window_start_index;
    p_window->target_win_offset_x = 0;
    p_window->target_win_offset_y = rect_h;
    p_window->target_win_width = rect_w * (cols - 1);
    p_window->target_win_height = rect_h * (rows - 1);
    p_window ++;
    window_start_index ++;

    //windows in top row
    for (cindex = 0; cindex < cols; cindex++) {

        if (window_start_index > window_end_index) {
            return EECode_OK;
        }

        p_window->win_config_id = window_start_index;
        p_window->target_win_offset_x = cindex * rect_w;
        p_window->target_win_offset_y = 0;
        p_window->target_win_width = rect_w;
        p_window->target_win_height = rect_h;

        p_window ++;
        window_start_index ++;
    }

    //windows in right colomn
    for (rindex = 1; rindex <= rows; rindex++) {
        if (window_start_index > window_end_index) {
            return EECode_OK;
        }

        p_window->win_config_id = window_start_index;
        p_window->target_win_offset_x = (cols - 1) * rect_w;
        p_window->target_win_offset_y = rindex * rect_h;
        p_window->target_win_width = rect_w;
        p_window->target_win_height = rect_h;

        p_window ++;
        window_start_index ++;
    }

    return EECode_OK;
}

EECode calculateDisplayLayoutWindow(SDSPMUdecWindow *p_window, TU16 display_width, TU16 display_height, TU8 layout, TU8 window_number, TU8 window_start_index)
{
    EECode err = EECode_OK;

    //debug assert
    DASSERT(p_window);
    DASSERT(display_width && display_height);
    DASSERT(window_number);

    if (p_window && display_width && display_height && window_number) {

        switch (layout) {

            case EDisplayLayout_Rectangle: {
                    TU8 rows = 1, cols = 1;

                    if (1 == window_number) {
                        rows = 1;
                        cols = 1;
                    } else if (2 == window_number) {
                        rows = 1;
                        cols = 2;
                    } else if (5 > window_number) {
                        rows = 2;
                        cols = 2;
                    } else if (7 > window_number) {
                        rows = 2;
                        cols = 3;
                    } else if (10 > window_number) {
                        rows = 3;
                        cols = 3;
                    } else if (13 > window_number) {
                        rows = 3;
                        cols = 4;
                    } else if (17 > window_number) {
                        rows = 4;
                        cols = 4;
                    } else {
                        LOG_FATAL("too many windows %d\n", window_number);
                        return EECode_BadParam;
                    }

                    err = calculateRectTableLayoutWindow(p_window, display_width, display_height, window_start_index, window_start_index + window_number - 1, rows, cols);
                }
                break;

            case EDisplayLayout_TelePresence: {
                    TUint sub_window_width = 1, sub_window_height = 1;
                    TUint sub_window_offset_x = 0, sub_window_offset_y = 0;
                    TUint additional_x = 1, additional_y = 1;
                    TU8 sub_window_layout = EDsplaySubWindowLayout_Horizontal_to_Right;
                    TUint gap_x = display_width / 10, gap_y = display_height / 10;
                    TUint sub_window_num = 0;

                    if (window_number > 1) {
                        sub_window_num = window_number - 1;
                        sub_window_width = (display_width - (gap_x * (sub_window_num + 1))) / sub_window_num;
                        sub_window_height = (display_height - (gap_y * (sub_window_num + 1))) / sub_window_num;
                        sub_window_width &= ~0x3;

                        sub_window_offset_x = gap_x;
                        sub_window_offset_y = display_height - gap_y - sub_window_height;

                        //hard code here for previous setting of ipad app
                        if ((sub_window_offset_y + 40 + sub_window_height) < display_height) {
                            sub_window_offset_y += 40;
                        }

                        additional_x = sub_window_width + gap_x;
                        additional_y = sub_window_height + gap_y;
                    } else {
                        LOG_WARN("window_number(%d) is not greater than 1 in EDisplayLayout_TelePresence's case\n", window_number);
                    }

                    //debug height less than 64
#if 0
                    sub_window_width = 208;
                    sub_window_height = 96;
                    additional_x = 248;
                    additional_y = 0;
                    sub_window_offset_y = 584;
                    sub_window_offset_x = 40;
#endif

                    if (DUnlikely(sub_window_height < 64)) {
                        LOG_WARN("sub window height(%d) less than 64\n", sub_window_height);
                        sub_window_height = 64;
                    }

                    err = calculateTelePresenceLayoutWindow(p_window, display_width, display_height, window_start_index, window_start_index + window_number - 1, sub_window_layout, sub_window_width, sub_window_height, sub_window_offset_x, sub_window_offset_y, additional_x, additional_y);
                }
                break;

            case EDisplayLayout_BottomLeftHighLighten: {
                    TU8 rows = 1, cols = 1;
                    if (4 >= window_number) {//mdf to fix bug#2942
                        rows = 2;
                        cols = 2;
                    } else if (6 >= window_number) {
                        rows = 3;
                        cols = 3;
                    } else if (8 >= window_number) {
                        rows = 4;
                        cols = 4;
                    } else if (10 >= window_number) {
                        rows = 5;
                        cols = 5;
                    } else {
                        LOG_FATAL("windows %d no support highlighten mode.\n", window_number);
                        return EECode_BadParam;
                    }

                    err = calculateBottomLeftHighLightenLayoutWindow(p_window, display_width, display_height, window_start_index, window_start_index + window_number - 1, rows, cols);
                }
                break;

            default:
                LOG_FATAL("BAD layout type %d\n", layout);
                return EECode_InternalLogicalBug;
                break;
        }

        return err;
    }

    LOG_FATAL("p_window %p, display_width %hu, display_height %hu, window_number %d\n", p_window, display_width, display_height, window_number);
    return EECode_InternalLogicalBug;
}

EECode gfPresetDisplayLayout(SVideoPostPGlobalSetting *p_global_setting, SVideoPostPDisplayLayout *p_input_param, SVideoPostPConfiguration *p_result)
{
    TUint i = 0, max_i = 0;
    TU8 window_number = 0, window_start_index = 0, tot_window_number = 0;
    SDSPMUdecWindow *p_window;
    EECode err;

    DASSERT(p_global_setting);
    DASSERT(p_input_param);
    DASSERT(p_result);

    if (p_global_setting && p_input_param && p_result) {

        p_result->display_width = p_global_setting->display_width;
        p_result->display_height = p_global_setting->display_height;
        LOG_PRINTF("[gfPresetDisplayLayout]: display_width %hu, display_height %hu.\n", p_result->display_width, p_result->display_height);

        p_result->voutA_enabled = p_global_setting->cur_display_mask & DCAL_BITMASK(EDisplayDevice_LCD);
        p_result->voutB_enabled = p_global_setting->cur_display_mask & DCAL_BITMASK(EDisplayDevice_HDMI);
        LOG_PRINTF("[gfPresetDisplayLayout]: voutA_enabled %d, voutB_enabled %d.\n", p_result->voutA_enabled, p_result->voutB_enabled);

        DASSERT(p_input_param->layer_number <= DMaxDisplayLayerNumber);
        if (p_input_param->layer_number <= DMaxDisplayLayerNumber) {
            max_i = p_input_param->layer_number;
            p_result->layer_number = p_input_param->layer_number;
        } else if (!p_input_param->layer_number) {
            LOG_ERROR("SDisplayLayoutConfiguration.layer_number(0) not specified!\n");
            max_i = 1;
        } else {
            LOG_ERROR("SDisplayLayoutConfiguration.layer_number(%d) exceed max value, DMaxDisplayLayerNumber %d\n", p_input_param->layer_number, DMaxDisplayLayerNumber);
            max_i = 1;
        }

        LOG_PRINTF("[gfPresetDisplayLayout]: window_start_index %d, window_number %d\n", window_start_index, window_number);
        p_window = &p_result->window[0];
        for (i = 0; i < max_i; i ++) {
            window_number = p_input_param->layer_window_number[i];
            p_result->layer_window_number[i] = p_input_param->layer_window_number[i];

            tot_window_number += window_number;
            if (tot_window_number > DMaxPostPWindowNumber) {
                LOG_FATAL("tot_window_number %d, too many\n", tot_window_number);
                return EECode_BadParam;
            }

            err = calculateDisplayLayoutWindow(p_window + window_start_index, p_result->display_width, p_result->display_height, p_input_param->layer_layout_type[i], window_number, window_start_index);
            window_start_index += window_number;

            DASSERT_OK(err);
            if (EECode_OK != err) {
                LOG_ERROR("calculateDisplayLayoutWindow return err %d\n", err);
                return EECode_InternalLogicalBug;
            }

        }

        p_result->total_window_number = p_input_param->total_window_number;
        p_result->total_render_number = p_input_param->total_render_number;
        p_result->total_decoder_number = p_input_param->total_decoder_number;

        p_result->cur_window_start_index = p_input_param->cur_window_start_index;
        p_result->cur_render_start_index = p_input_param->cur_render_start_index;
        p_result->cur_decoder_start_index = p_input_param->cur_decoder_start_index;

        p_result->cur_window_number = p_input_param->cur_window_number;
        p_result->cur_render_number = p_input_param->cur_render_number;
        p_result->cur_decoder_number = p_input_param->cur_decoder_number;

        return EECode_OK;
    }

    LOG_ERROR("gfPresetDisplayLayout NULL params: p_global_setting %p, p_input_param %p, p_result %p\n", p_global_setting, p_input_param, p_result);
    return EECode_BadParam;
}

EECode gfDefaultRenderConfig(SDSPMUdecRender *render, TUint number_1, TUint number_2)
{
    TUint i;
    DASSERT(render);
    DASSERT(DMaxPostPRenderNumber >= (number_1 + number_2));

    if ((render) && (DMaxPostPRenderNumber >= (number_1 + number_2))) {
        for (i = 0; i < number_1; i++) {
            render[i].render_id = i;
            render[i].win_config_id = i;
            render[i].win_config_id_2nd = 0xff;
            render[i].udec_id = i;
            render[i].first_pts_low = 0;
            render[i].first_pts_high = 0;
            render[i].input_source_type = 1;//hard code here
        }

        for (i = 0; i < number_2; i++) {
            render[i + number_1].render_id = i;
            render[i + number_1].win_config_id = i + number_1;
            render[i + number_1].win_config_id_2nd = 0xff;
            render[i + number_1].udec_id = i + number_1;
            render[i + number_1].first_pts_low = 0;
            render[i + number_1].first_pts_high = 0;
            render[i + number_1].input_source_type = 1;//hard code here
        }
    } else {
        LOG_FATAL("BAD parameters\n");
        return EECode_BadParam;
    }

    return EECode_OK;

}

EECode gfUpdateHighlightenDisplayLayout(SVideoPostPGlobalSetting *p_global_setting, SVideoPostPDisplayLayout *cur_config, SVideoPostPDisplayHighLightenWindowSize *highlighten_window_size, SVideoPostPConfiguration *p_result)
{
    TUint i = 0, top_or_right_window_num = 0;
    TU8 window_number = 0, window_start_index = 0;
    SDSPMUdecWindow *p_window = NULL;
    TUint rect_w, rect_h;

    DASSERT(p_global_setting);
    DASSERT(cur_config);
    DASSERT(highlighten_window_size);
    DASSERT(p_result);

    window_number = cur_config->layer_window_number[0];
    top_or_right_window_num = (((window_number + 1) & (~1)) - 2) >> 1; //windows number should upround to 2 firstly to calculate the right top_or_right_window_num: bottomleft--highlighten window, topright--corner window, top line--half of windows left, right line--half of windows left
    /*if (top_or_right_window_num < 2)//fix 4+1 mode --layout 2 issue
    {
        top_or_right_window_num = 2;
    }*///remove to fix bug#2942
    p_window = &p_result->window[0];

    //update top line windows
    rect_w = (highlighten_window_size->window_width) / top_or_right_window_num;
    rect_h = p_global_setting->display_height - highlighten_window_size->window_height;
    for (i = 0; i < top_or_right_window_num; i++) {
        if (window_start_index > (window_number - 1)) {
            return EECode_OK;
        }

        p_window->win_config_id = window_start_index;
        p_window->target_win_offset_x = i * rect_w;
        p_window->target_win_offset_y = 0;
        p_window->target_win_width = rect_w;
        p_window->target_win_height = rect_h;

        p_window++;
        window_start_index++;
    }

    //update topright corner window
    p_window->win_config_id = window_start_index;
    p_window->target_win_offset_x = highlighten_window_size->window_width;
    p_window->target_win_offset_y = 0;
    p_window->target_win_width = p_global_setting->display_width - highlighten_window_size->window_width;
    p_window->target_win_height = rect_h;

    p_window++;
    window_start_index++;

    //update right line windows
    rect_w = p_global_setting->display_width - highlighten_window_size->window_width;
    rect_h = (highlighten_window_size->window_height) / top_or_right_window_num;
    for (i = 0; i <= top_or_right_window_num; i++) {
        if (window_start_index > (window_number - 1)) {
            return EECode_OK;
        }

        //update highlighten window
        if (window_start_index == (window_number - 1)) {
            p_window->win_config_id = window_start_index;
            p_window->target_win_offset_x = 0;
            p_window->target_win_offset_y = p_global_setting->display_height - highlighten_window_size->window_height;
            p_window->target_win_width = highlighten_window_size->window_width;
            p_window->target_win_height = highlighten_window_size->window_height;
            return EECode_OK;
        }

        p_window->win_config_id = window_start_index;
        p_window->target_win_offset_x = highlighten_window_size->window_width;
        p_window->target_win_offset_y = p_global_setting->display_height - highlighten_window_size->window_height + i * rect_h;
        p_window->target_win_width = rect_w;
        p_window->target_win_height = rect_h;

        p_window ++;
        window_start_index ++;
    }

    return EECode_OK;
}

const TChar *gfGetContainerStringFromType(ContainerType type)
{
    switch (type) {
        case ContainerType_MP4://default
            return "mp4";
        case ContainerType_3GP:
            return "3gp";
        case ContainerType_TS:
            return "ts";
        case ContainerType_MOV:
            return "mov";
        case ContainerType_AVI:
            return "avi";
        case ContainerType_AMR:
            return "amr";
        case ContainerType_MKV:
            return "mkv";
        default:
            LOG_FATAL("unknown container type.\n");
            break;
    }

    return "???";
}

const TChar *gfGetComponentStringFromGenericComponentType(TComponentType type)
{
    switch (type) {

        case EGenericComponentType_Demuxer:
            return "EGenericComponentType_Demuxer";

        case EGenericComponentType_VideoDecoder:
            return "EGenericComponentType_VideoDecoder";

        case EGenericComponentType_AudioDecoder:
            return "EGenericComponentType_AudioDecoder";

        case EGenericComponentType_VideoEncoder:
            return "EGenericComponentType_VideoEncoder";

        case EGenericComponentType_AudioEncoder:
            return "EGenericComponentType_AudioEncoder";

        case EGenericComponentType_VideoPostProcessor:
            return "EGenericComponentType_VideoPostProcessor";

        case EGenericComponentType_AudioPostProcessor:
            return "EGenericComponentType_AudioPostProcessor";

        case EGenericComponentType_VideoPreProcessor:
            return "EGenericComponentType_VideoPreProcessor";

        case EGenericComponentType_AudioPreProcessor:
            return "EGenericComponentType_AudioPreProcessor";

        case EGenericComponentType_VideoCapture:
            return "EGenericComponentType_VideoCapture";

        case EGenericComponentType_AudioCapture:
            return "EGenericComponentType_AudioCapture";

        case EGenericComponentType_VideoRenderer:
            return "EGenericComponentType_VideoRenderer";

        case EGenericComponentType_AudioRenderer:
            return "EGenericComponentType_AudioRenderer";

        case EGenericComponentType_Muxer:
            return "EGenericComponentType_Muxer";

        case EGenericComponentType_StreamingServer:
            return "EGenericComponentType_StreamingServer";

        case EGenericComponentType_StreamingTransmiter:
            return "EGenericComponentType_StreamingTransmiter";

        case EGenericComponentType_ConnecterPinMuxer:
            return "EGenericComponentType_ConnecterPinMuxer";

        case EGenericComponentType_CloudConnecterServerAgent:
            return "EGenericComponentType_CloudConnecterServerAgent";

        case EGenericComponentType_CloudConnecterClientAgent:
            return "EGenericComponentType_CloudConnecterClientAgent";

        case EGenericComponentType_CloudConnecterCmdAgent:
            return "EGenericComponentType_CloudConnecterCmdAgent";

        case EGenericComponentType_StreamingContent:
            return "EGenericComponentType_StreamingContent";

        case EGenericComponentType_FlowController:
            return "EGenericComponentType_FlowController";

        case EGenericComponentType_VideoOutput:
            return "EGenericComponentType_VideoOutput";

        case EGenericComponentType_AudioOutput:
            return "EGenericComponentType_AudioOutput";

        case EGenericComponentType_ExtVideoESSource:
            return "EGenericComponentType_ExtVideoESSource";
            break;

        case EGenericComponentType_ExtAudioESSource:
            return "EGenericComponentType_ExtAudioESSource";
            break;

        case EGenericComponentType_ConnectionPin:
            return "EGenericComponentType_ConnectionPin";

        case EGenericComponentType_CloudServer:
            return "EGenericComponentType_CloudServer";

        case EGenericComponentType_CloudReceiverContent:
            return "EGenericComponentType_CloudReceiverContent";

        case EGenericPipelineType_Playback:
            return "EGenericPipelineType_Playback";
            break;

        case EGenericPipelineType_Recording:
            return "EGenericPipelineType_Recording";
            break;

        case EGenericPipelineType_Streaming:
            return "EGenericPipelineType_Streaming";
            break;

        case EGenericPipelineType_Cloud:
            return "EGenericPipelineType_Cloud";
            break;

        case EGenericPipelineType_NativeCapture:
            return "EGenericPipelineType_NativeCapture";
            break;

        case EGenericPipelineType_NativePush:
            return "EGenericPipelineType_NativePush";
            break;

        case EGenericPipelineType_VOD:
            return "EGenericPipelineType_VOD";

        default:
            LOG_FATAL("unknown EGenericComponentType %d.\n", type);
            break;
    }

    return "UnknownComponentType";
}

const TChar *gfGetMediaDeviceWorkingModeString(EMediaWorkingMode mode)
{
    switch (mode) {

        case EMediaDeviceWorkingMode_Invalid:
            return "EMediaDeviceWorkingMode_Invalid";
            break;

        case EMediaDeviceWorkingMode_SingleInstancePlayback:
            return "EMediaDeviceWorkingMode_SingleInstancePlayback";
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback:
            return "EMediaDeviceWorkingMode_MultipleInstancePlayback";
            break;

        case EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding:
            return "EMediaDeviceWorkingMode_MultipleInstancePlayback_Transcoding";
            break;

        case EMediaDeviceWorkingMode_MultipleInstanceRecording:
            return "EMediaDeviceWorkingMode_MultipleInstanceRecording";
            break;

        case EMediaDeviceWorkingMode_FullDuplex_SingleRecSinglePlayback:
            return "EMediaDeviceWorkingMode_FullDuplex_SingleRecSinglePlayback";
            break;

        case EMediaDeviceWorkingMode_FullDuplex_SingleRecMultiplePlayback:
            return "EMediaDeviceWorkingMode_FullDuplex_SingleRecMultiplePlayback";
            break;

        case EMediaDeviceWorkingMode_NVR_RTSP_Streaming:
            return "EMediaDeviceWorkingMode_NVR_RTSP_Streaming";
            break;

        default:
            LOG_FATAL("BAD EMediaDeviceWorkingMode %d\n", mode);
            break;
    }

    return "unkown EMediaDeviceWorkingMode";
}

const TChar *gfGetPlaybackModeString(EPlaybackMode mode)
{
    switch (mode) {

        case EPlaybackMode_Invalid:
            return "EPlaybackMode_Invalid";
            break;

        case EPlaybackMode_AutoDetect:
            return "EPlaybackMode_AutoDetect";
            break;

        case EPlaybackMode_1x1080p:
            return "EPlaybackMode_1x1080p";
            break;

        case EPlaybackMode_1x1080p_4xD1:
            return "EPlaybackMode_1x1080p_4xD1";
            break;

        case EPlaybackMode_1x1080p_6xD1:
            return "EPlaybackMode_1x1080p_6xD1";
            break;

        case EPlaybackMode_1x1080p_8xD1:
            return "EPlaybackMode_1x1080p_8xD1";
            break;

        case EPlaybackMode_1x1080p_9xD1:
            return "EPlaybackMode_1x1080p_9xD1";
            break;

        case EPlaybackMode_4x720p:
            return "EPlaybackMode_4x720p";
            break;

        case EPlaybackMode_4xD1:
            return "EPlaybackMode_4xD1";
            break;

        case EPlaybackMode_6xD1:
            return "EPlaybackMode_6xD1";
            break;

        case EPlaybackMode_8xD1:
            return "EPlaybackMode_8xD1";
            break;

        case EPlaybackMode_9xD1:
            return "EPlaybackMode_9xD1";
            break;

        case EPlaybackMode_1x3M:
            return "EPlaybackMode_1x3M";
            break;

        case EPlaybackMode_1x3M_4xD1:
            return "EPlaybackMode_1x3M_4xD1";
            break;

        case EPlaybackMode_1x3M_6xD1:
            return "EPlaybackMode_1x3M_6xD1";
            break;

        case EPlaybackMode_1x3M_8xD1:
            return "EPlaybackMode_1x3M_8xD1";
            break;

        case EPlaybackMode_1x3M_9xD1:
            return "EPlaybackMode_1x3M_9xD1";
            break;

        case ENoPlaybackInstance:
            return "ENoPlaybackInstance";
            break;

        default:
            LOG_FATAL("BAD EPlaybackMode %d\n", mode);
            break;
    }

    return "unkown EPlaybackMode";
}

const TChar *gfGetDSPOperationModeString(EDSPOperationMode mode)
{
    switch (mode) {

        case EDSPOperationMode_Invalid:
            return "EDSPOperationMode_Invalid";
            break;

        case EDSPOperationMode_IDLE:
            return "EDSPOperationMode_IDLE";
            break;

        case EDSPOperationMode_UDEC:
            return "EDSPOperationMode_UDEC";
            break;

        case EDSPOperationMode_MultipleWindowUDEC:
            return "EDSPOperationMode_MultipleWindowUDEC";
            break;

        case EDSPOperationMode_MultipleWindowUDEC_Transcode:
            return "EDSPOperationMode_MultipleWindowUDEC_Transcode";
            break;
        case EDSPOperationMode_FullDuplex:
            return "EDSPOperationMode_FullDuplex";
            break;

        case EDSPOperationMode_CameraRecording:
            return "EDSPOperationMode_CameraRecording";
            break;

        case EDSPOperationMode_Transcode:
            return "EDSPOperationMode_Transcode";
            break;

        default:
            LOG_FATAL("BAD EDSPOperationMode %d\n", mode);
            break;
    }

    return "unkown EDSPOperationMode";
}

StreamFormat gfGetStreamFormatFromString(TChar *str)
{
    TUint i, tot;
    tot = sizeof(g_FormatStringPair) / sizeof(SFormatStringPair);
    for (i = 0; i < tot; i++) {
        if (0 == strncmp(str, g_FormatStringPair[i].format_name, strlen(g_FormatStringPair[i].format_name))) {
            return g_FormatStringPair[i].format;
        }
    }
    return StreamFormat_Invalid;
}

EntropyType gfGetEntropyTypeFromString(char *str)
{
    TUint i, tot;
    tot = sizeof(g_EntropyTypeStringPair) / sizeof(SEntropyTypeStringPair);
    for (i = 0; i < tot; i++) {
        if (0 == strncmp(str, g_EntropyTypeStringPair[i].entropy_name, strlen(g_EntropyTypeStringPair[i].entropy_name))) {
            return g_EntropyTypeStringPair[i].type;
        }
    }
    return EntropyType_NOTSet;
}

ContainerType gfGetContainerTypeFromString(char *str)
{
    TUint i, tot;
    tot = sizeof(g_ContainerStringPair) / sizeof(SContainerStringPair);
    for (i = 0; i < tot; i++) {
        if (0 == strncmp(str, g_ContainerStringPair[i].container_name, strlen(g_ContainerStringPair[i].container_name))) {
            return g_ContainerStringPair[i].container_type;
        }
    }

    return ContainerType_Invalid;
}

AudioSampleFMT gfGetSampleFormatFromString(char *str)
{
    TUint i, tot;
    tot = sizeof(g_SampleFormatStringPair) / sizeof(SSampleFormatStringPair);
    for (i = 0; i < tot; i++) {
        if (0 == strncmp(str, g_SampleFormatStringPair[i].sampleformat_name, strlen(g_SampleFormatStringPair[i].sampleformat_name))) {
            return g_SampleFormatStringPair[i].sampleformat;
        }
    }
    //default s16
    return AudioSampleFMT_S16;
}

bool gfGetSpsPps(TU8 *pBuffer, TUint size, TU8 *&p_spspps, TUint &spspps_size, TU8 *&p_IDR, TU8 *&p_ret_data)
{
    TUint checking_size = 0;
    TU32 start_code;
    bool find_sps = false;
    bool find_pps = false;
    bool find_IDR = false;
    bool find_SEI = false;
    TU8 type;

    DASSERT(pBuffer);
    if (!pBuffer) {
        LOG_FATAL("NULL pointer in _get_sps_pps.\n");
        return false;
    }

    LOG_NOTICE("[debug info], input %p, size %d, data [%02x %02x %02x %02x %02x]\n", pBuffer, size, pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);

    p_spspps = pBuffer;
    p_IDR = pBuffer;
    p_ret_data = NULL;
    spspps_size = 0;

    start_code = 0xffffffff;
    while ((checking_size + 1) < size) {
        start_code <<= 8;
        start_code |= pBuffer[checking_size];

        if (start_code == 0x00000001) {
            type = (0x1f & pBuffer[checking_size + 1]);
            if (0x05 == type) {
                DASSERT(find_sps);
                DASSERT(find_pps);
                DASSERT(!find_IDR);
                p_IDR = pBuffer + checking_size - 3;
                find_IDR = true;
                LOG_WARN("[debug info], find IDR, checking_size - 3 %d\n", checking_size - 3);
                break;
            } else if (0x06 == type) {
                //to do
                find_SEI = true;
                LOG_WARN("[debug info], find SEI, checking_size - 3 %d\n", checking_size - 3);
            } else if (0x07 == type) {
                DASSERT(!find_sps);
                DASSERT(!find_pps);
                DASSERT(!find_IDR);
                p_spspps = pBuffer + checking_size - 3;
                find_sps = true;
                LOG_WARN("[debug info], find sps, checking_size - 3 %d\n", checking_size - 3);
            } else if (0x08 == type) {
                DASSERT(find_sps);
                DASSERT(!find_pps);
                DASSERT(!find_IDR);
                find_pps = true;
                LOG_WARN("[debug info], find pps, checking_size - 3 %d\n", checking_size - 3);
            } else if (0x09 == type) {
                p_ret_data = pBuffer + checking_size - 3;
                LOG_WARN("[debug info], find delimiter, checking_size - 3 %d\n", checking_size - 3);
            } else {
                LOG_WARN("unknown type %d.\n", type);
            }
        }

        checking_size ++;
    }

    if (find_sps && find_IDR) {
        DASSERT(find_pps);
        DASSERT(((TMemSize)p_IDR) > ((TMemSize)p_spspps));
        if (((TMemSize)p_IDR) > ((TMemSize)p_spspps)) {
            spspps_size = ((TMemSize)p_IDR) - ((TMemSize)p_spspps);
        } else {
            LOG_FATAL("internal error!!\n");
            return false;
        }
        LOG_WARN("[debug info], input %p, find IDR %p, sps %p, seq size %d, checking_size %d\n", pBuffer, p_IDR, p_spspps, spspps_size, checking_size);
        return true;
    } else if (find_SEI) {
        //todo
    }

    LOG_WARN("cannot find sps(%d), pps(%d), IDR header(%d), please check code!!\n", find_sps, find_pps, find_IDR);
    return false;
}

void gfLoadDefaultMediaConfig(volatile SPersistMediaConfig *pconfig)
{
    if (DUnlikely(!pconfig)) {
        return;
    }

    TU32 i;

    pconfig->engine_config.engine_version = 1;
    pconfig->engine_config.filter_version = 1;

    pconfig->streaming_timeout_threashold = 10000000;
    pconfig->streaming_autoconnect_interval = 10000000;
    pconfig->playback_prefetch_number = 6;

    //mode config
    pconfig->dsp_config.modeConfig.postp_mode = 2;
    pconfig->dsp_config.modeConfig.enable_deint = 0;
    pconfig->dsp_config.modeConfig.pp_chroma_fmt_max = 2;
    pconfig->dsp_config.modeConfig.vout_mask = DCAL_BITMASK(EDisplayDevice_HDMI);
    pconfig->dsp_config.modeConfig.num_udecs = 1;

    pconfig->dsp_config.modeConfig.pp_background_Y = 0;
    pconfig->dsp_config.modeConfig.pp_background_Cb = 128;
    pconfig->dsp_config.modeConfig.pp_background_Cr = 128;

    pconfig->dsp_config.modeConfigMudec.enable_buffering_ctrl = 0;
    pconfig->dsp_config.modeConfigMudec.pre_buffer_len = 0;

    //de-interlacing config
    pconfig->dsp_config.tiled_mode = 5;
    pconfig->dsp_config.frm_chroma_fmt_max = 1;
    pconfig->dsp_config.max_frm_num = 7;
    pconfig->dsp_config.max_frm_width = 1920;
    pconfig->dsp_config.max_frm_height = 1088;
    pconfig->dsp_config.max_fifo_size = 2 * 1024 * 1024;
    pconfig->dsp_config.enable_pp = 1;
    pconfig->dsp_config.enable_deint = 0;
    pconfig->dsp_config.interlaced_out = 0;
    pconfig->dsp_config.packed_out = 0;
    pconfig->dsp_config.enable_err_handle = 1;

    pconfig->dsp_config.bits_fifo_size = 2 * 1024 * 1024;
    pconfig->dsp_config.ref_cache_size = 0;
    pconfig->dsp_config.concealment_mode = 0;
    pconfig->dsp_config.concealment_ref_frm_buf_id = 0;

    pconfig->dsp_config.deinterlaceConfig.init_tff = 1;
    pconfig->dsp_config.deinterlaceConfig.deint_lu_en = 1;
    pconfig->dsp_config.deinterlaceConfig.deint_ch_en = 1;
    pconfig->dsp_config.deinterlaceConfig.osd_en = 0;
    pconfig->dsp_config.deinterlaceConfig.deint_mode = 0;
    pconfig->dsp_config.deinterlaceConfig.deint_spatial_shift = 2;
    pconfig->dsp_config.deinterlaceConfig.deint_lowpass_shift = 5;

    pconfig->dsp_config.deinterlaceConfig.deint_lowpass_center_weight = 16;
    pconfig->dsp_config.deinterlaceConfig.deint_lowpass_hor_weight = 2;
    pconfig->dsp_config.deinterlaceConfig.deint_lowpass_ver_weight = 4;

    pconfig->dsp_config.deinterlaceConfig.deint_gradient_bias = 15;
    pconfig->dsp_config.deinterlaceConfig.deint_predict_bias = 15;
    pconfig->dsp_config.deinterlaceConfig.deint_candidate_bias = 10;

    pconfig->dsp_config.deinterlaceConfig.deint_spatial_score_bias = -5;
    pconfig->dsp_config.deinterlaceConfig.deint_temporal_score_bias = 5;

    //for each udec instance
    for (i = 0; i < DMaxUDECInstanceNumber; i++) {
        //udec config
        pconfig->dsp_config.udecInstanceConfig[i].tiled_mode = 5;
        pconfig->dsp_config.udecInstanceConfig[i].frm_chroma_fmt_max = UDEC_CFG_FRM_CHROMA_FMT_420;

        pconfig->dsp_config.udecInstanceConfig[i].max_frm_num = 7;
        pconfig->dsp_config.udecInstanceConfig[i].max_frm_width = 1920;
        pconfig->dsp_config.udecInstanceConfig[i].max_frm_height = 1088;
        pconfig->dsp_config.udecInstanceConfig[i].max_fifo_size = 2 * 1024 * 1024;

        pconfig->dsp_config.udecInstanceConfig[i].bits_fifo_size = 2 * 1024 * 1024;

        //error handling
        pconfig->dsp_config.errorHandlingConfig[i].enable_udec_error_handling = 1;
        pconfig->dsp_config.errorHandlingConfig[i].error_concealment_mode = 0;
        pconfig->dsp_config.errorHandlingConfig[i].error_concealment_frame_id = 0;
        pconfig->dsp_config.errorHandlingConfig[i].error_handling_app_behavior = 0;
    }

    //deblocking config
    pconfig->dsp_config.deblockingConfig.pquant_mode = 1;

    //default table value
    for (i = 0; i < 32; i++) {
        pconfig->dsp_config.deblockingConfig.pquant_table[i] = i;
    }

    //vout config
    pconfig->dsp_config.voutConfigs.vout_mask = DCAL_BITMASK(EDisplayDevice_HDMI);

    //streaming related
    pconfig->rtsp_server_config.rtsp_listen_port = DefaultRTSPServerPort;
    pconfig->rtsp_server_config.rtp_rtcp_port_start = DefaultRTPServerPortBase;
    pconfig->rtsp_streaming_enable = 1;
    pconfig->rtsp_streaming_video_enable = 1;
    pconfig->rtsp_streaming_audio_enable = 1;

    pconfig->rtsp_client_config.parse_multiple_access_unit = 1;

    //audio capturer default params
    pconfig->audio_prefer_setting.sample_rate = DDefaultAudioSampleRate;
    pconfig->audio_prefer_setting.sample_format = AudioSampleFMT_S16;
    pconfig->audio_prefer_setting.sample_size = sizeof(TU16);
    pconfig->audio_prefer_setting.channel_number = DDefaultAudioChannelNumber;
    pconfig->audio_prefer_setting.framesize = 1024;

    pconfig->video_encoding_config.bitrate = 1000000;
    pconfig->audio_encoding_config.bitrate = 128000;

    pconfig->number_scheduler_network_reciever = 0;
    pconfig->number_scheduler_network_tcp_reciever = 0;
    pconfig->number_scheduler_network_sender = 0;
    pconfig->number_scheduler_io_writer = 0;
    pconfig->number_scheduler_io_reader = 0;

    pconfig->pb_cache.precache_video_frames = DDefaultAVSYNCVideoLatencyFrameCount;
    pconfig->pb_cache.max_video_frames = DDefaultAVSYNCVideoMaxAccumulatedFrameCount;
    pconfig->pb_cache.precache_audio_frames = 0;
    pconfig->pb_cache.max_audio_frames = 0;

    pconfig->pb_decode.prealloc_buffer_number = 8;
    pconfig->pb_decode.prefer_thread_number = 1;
    pconfig->pb_decode.prefer_parallel_frame = 1;
    pconfig->pb_decode.prefer_parallel_slice = 1;

    pconfig->pb_decode.prefer_official_reference_model_decoder = 0;

    //debug config
    pconfig->debug_config.pre_send_audio_extradata = 1;
}

void gfLoadDefaultMultipleWindowPlaybackConfig(volatile SPersistMediaConfig *pconfig)
{
    LOG_FATAL("TO DO\n");
}

static void setLogConfig(ELogLevel level, TUint option, TUint output, SLogConfig *p_config, TUint max_element_number)
{
    TUint i = 0;

    DASSERT(p_config);
    if (!p_config) {
        LOG_FATAL("NULL p_config\n");
        return;
    }

    DASSERT(max_element_number <= ELogModuleListEnd);
    DASSERT(max_element_number > 0);
    if (max_element_number > ELogModuleListEnd) {
        max_element_number = ELogModuleListEnd;
    }

    for (i = 0; i < max_element_number; i ++, p_config ++) {
        p_config->log_level = level;
        p_config->log_option = option;
        p_config->log_output = output;
    }

}

void gfSetLogConfigLevel(ELogLevel level, SLogConfig *p_config, TUint max_element_number)
{
    setLogConfig(level, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File), p_config, max_element_number);
}

void gfLoadDefaultLogConfig(SLogConfig *p_config, TUint max_element_number)
{
    setLogConfig(ELogLevel_Notice, DCAL_BITMASK(ELogOption_Congifuration), DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File), p_config, max_element_number);
}

EECode gfGenerateMediaConfigFile(ERunTimeConfigType config_type, const volatile SPersistMediaConfig *p_config, const TChar *config_file_name)
{
    EECode err = EECode_OK;
    TUint i = 0;
    void *p_root = NULL;
    void *p_node = NULL;
    void *p_child = NULL;
    TChar tmp[128] = {0};

    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(config_type);
    if (NULL == api) {
        LOG_FATAL("gfRunTimeConfigAPIFactory fail\n");
        return EECode_NoMemory;
    }

    err = api->NewFile("MediaConfig", p_root);
    DASSERT_OK(err);
    if (EECode_OK != err) {
        return err;
    }

    DASSERT(p_root);

    p_node = api->AddContent(p_root, "StreamingConfig");
    DASSERT(p_node);
    if (NULL == p_node) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->streaming_enable);
    p_child = api->AddContentChild(p_node, "streaming_enable", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    p_node = api->AddContent(p_root, "RTSPStreamingConfig");
    DASSERT(p_node);
    if (NULL == p_node) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->rtsp_streaming_enable);
    p_child = api->AddContentChild(p_node, "rtsp_streaming_enable", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->rtsp_streaming_video_enable);
    p_child = api->AddContentChild(p_node, "rtsp_streaming_video_enable", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->rtsp_streaming_audio_enable);
    p_child = api->AddContentChild(p_node, "rtsp_streaming_audio_enable", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    p_node = api->AddContent(p_root, "RTSPServerConfig");
    DASSERT(p_node);
    if (NULL == p_node) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->rtsp_server_config.rtsp_listen_port);
    p_child = api->AddContentChild(p_node, "rtsp_listen_port", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->rtsp_server_config.rtp_rtcp_port_start);
    p_child = api->AddContentChild(p_node, "rtp_rtcp_port_start", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    p_node = api->AddContent(p_root, "DSPConfig");
    DASSERT(p_node);
    if (NULL == p_node) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->dsp_config.tiled_mode);
    p_child = api->AddContentChild(p_node, "tiled_mode", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->dsp_config.max_frm_num);
    p_child = api->AddContentChild(p_node, "max_frm_num", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->dsp_config.enable_deint);
    p_child = api->AddContentChild(p_node, "enable_deint", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->dsp_config.max_fifo_size);
    p_child = api->AddContentChild(p_node, "max_fifo_size", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    p_node = api->AddContent(p_root, "RecordingConfig");
    DASSERT(p_node);
    if (NULL == p_node) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->cutfile_with_precise_pts);
    p_child = api->AddContentChild(p_node, "cutfile_with_precise_pts", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->muxer_skip_video);
    p_child = api->AddContentChild(p_node, "muxer_skip_video", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->muxer_skip_audio);
    p_child = api->AddContentChild(p_node, "muxer_skip_audio", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    sprintf(tmp, "%u", p_config->muxer_skip_pridata);
    p_child = api->AddContentChild(p_node, "muxer_skip_pridata", (const TChar *)tmp);
    DASSERT(p_child);
    if (NULL == p_child) {
        return EECode_NoMemory;
    }

    for (i = 0; i < EComponentMaxNumber_Muxer; i++) {
        sprintf(tmp, "STargetRecorderConfig_%d", i);

        p_node = api->AddContent(p_root, tmp);
        DASSERT(p_node);
        if (NULL == p_node) {
            return EECode_NoMemory;
        }

        sprintf(tmp, "%u", p_config->target_recorder_config[i].container_format);
        p_child = api->AddContentChild(p_node, "container_format", (const TChar *)tmp);
        DASSERT(p_child);
        if (NULL == p_child) {
            return EECode_NoMemory;
        }
    }

    err = api->FinalizeFile(config_file_name);
    DASSERT_OK(err);

    return err;
}

EECode gfSaveLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SLogConfig *p_config, TUint max_element_number)
{
    EECode err = EECode_OK;
    TUint i = 0;
    void *p_root = NULL;
    void *p_node = NULL;
    void *p_child = NULL;
    TChar tmp[128] = {0};
    const TChar *module_string = NULL;

    IRunTimeConfigAPI *api = NULL;

    DASSERT(max_element_number <= ELogModuleListEnd);
    DASSERT(max_element_number > 0);
    if (max_element_number > ELogModuleListEnd) {
        max_element_number = ELogModuleListEnd;
    }

    if (DUnlikely((!p_config) || (!config_file_name))) {
        LOG_FATAL("p_config %p NULL, or config_file_name %p NULL\n", p_config, config_file_name);
        return EECode_NoMemory;
    }

    api = gfRunTimeConfigAPIFactory(config_type);
    if (NULL == api) {
        LOG_WARN("gfRunTimeConfigAPIFactory fail\n");
        return EECode_NotSupported;
    }

    err = api->NewFile("LogConfig", p_root);
    DASSERT_OK(err);
    if (EECode_OK != err) {
        return err;
    }

    DASSERT(p_root);

    for (i = 0; i < max_element_number; i++, p_config++) {
        err = gfGetLogModuleString((ELogModule)i, module_string);
        if (EECode_OK != err) {
            LOG_FATAL("please check code\n");
            return EECode_InternalLogicalBug;
        }

        p_node = api->AddContent(p_root, module_string);
        DASSERT(p_node);
        if (NULL == p_node) {
            return EECode_NoMemory;
        }

        sprintf(tmp, "%u", p_config->log_level);
        p_child = api->AddContentChild(p_node, "log_level", (const TChar *)tmp);
        DASSERT(p_child);
        if (NULL == p_child) {
            return EECode_NoMemory;
        }

        sprintf(tmp, "%u", p_config->log_option);
        p_child = api->AddContentChild(p_node, "log_option", (const TChar *)tmp);
        DASSERT(p_child);
        if (NULL == p_child) {
            return EECode_NoMemory;
        }

        sprintf(tmp, "%u", p_config->log_output);
        p_child = api->AddContentChild(p_node, "log_output", (const TChar *)tmp);
        DASSERT(p_child);
        if (NULL == p_child) {
            return EECode_NoMemory;
        }
    }

    err = api->FinalizeFile(config_file_name);
    DASSERT_OK(err);

    api->Destroy();

    return err;
}

EECode gfLoadLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SLogConfig *p_config, TUint max_element_number)
{
    TChar name[256] = {0}, value[256] = {0};
    IRunTimeConfigAPI *api = NULL;
    EECode err = EECode_OK;
    TUint i = 0, module_index = 0;
    const TChar *module_string = NULL;
    //const TChar* level_string = NULL;
    TUint count;

    DASSERT(max_element_number <= ELogModuleListEnd);
    DASSERT(max_element_number > 0);
    if (max_element_number > ELogModuleListEnd) {
        max_element_number = ELogModuleListEnd;
    }

    if (DUnlikely((!p_config) || (!config_file_name))) {
        LOG_FATAL("p_config %p NULL, or config_file_name %p NULL\n", p_config, config_file_name);
        return EECode_BadParam;
    }

    api = gfRunTimeConfigAPIFactory(config_type);
    if (NULL == api) {
        LOG_NOTICE("gfRunTimeConfigAPIFactory fail\n");
        return EECode_NotSupported;
    }

    err = api->OpenConfigFile(config_file_name);
    if (EECode_OK != err) {
        api->Destroy();
        api = NULL;
        LOG_ERROR("open (%s) fail\n", config_file_name);
        return EECode_Error;
    }

    for (i = 0; i < max_element_number; i++, p_config ++) {

        err = gfGetLogModuleString((ELogModule)i, module_string);
        if (EECode_OK != err) {
            LOG_FATAL("please check code\n");
            return EECode_InternalLogicalBug;
        }

        err = gfGetLogModuleIndex(module_string, module_index);
        if (EECode_OK != err) {
            LOG_FATAL("please check code\n");
            return EECode_InternalLogicalBug;
        }

        DASSERT(module_index < max_element_number);

        memset(name, 0x0, sizeof(name));
        snprintf((TChar *)name, sizeof(name) - 1 , "%s/log_level", module_string);
        memset(value, 0x0, sizeof(value));
        err = api->GetContentValue((const TChar *)name, value);
        if (EECode_OK != err) {
            LOG_ERROR("load %s fail\n", name);
            return EECode_Error;
        }

        count = sscanf((const TChar *)value, "%u", &p_config->log_level);
        if (1 != count) {
            LOG_ERROR("load %s's value fail, %u\n", name, count);
            return EECode_Error;
        }

        memset(name, 0x0, sizeof(name));
        snprintf((TChar *)name, sizeof(name) - 1 , "%s/log_option", module_string);
        memset(value, 0x0, sizeof(value));
        err = api->GetContentValue((const TChar *)name, value);
        if (EECode_OK != err) {
            LOG_ERROR("load %s fail\n", name);
            return EECode_Error;
        }

        count = sscanf((const TChar *)value, "%u", &p_config->log_option);
        if (1 != count) {
            LOG_ERROR("load %s's value fail\n", name);
            return EECode_Error;
        }

        memset(name, 0x0, sizeof(name));
        snprintf((TChar *)name, sizeof(name) - 1 , "%s/log_output", module_string);
        memset(value, 0x0, sizeof(value));
        err = api->GetContentValue((const TChar *)name, value);
        if (EECode_OK != err) {
            LOG_ERROR("load %s fail\n", name);
            return EECode_Error;
        }

        count = sscanf((const TChar *)value, "%u", &p_config->log_output);
        if (1 != count) {
            LOG_ERROR("load %s's value fail\n", name);
            return EECode_Error;
        }

#if 0
        err = gfGetLogLevelString((ELogLevel)p_config->log_level, level_string);
        if (EECode_OK != err) {
            LOG_ERROR("gfGetLogLevelString fail, ret %s, %d\n", gfGetErrorCodeString(err), err);
            return EECode_BadParam;
        }

        LOG_CONFIGURATION("\tload module config %s:\tlog_level %s(%d), \tlog_option 0x%08x, \tlog_output 0x%08x\n", module_string, level_string, p_config->log_level, p_config->log_option, p_config->log_output);
#endif

    }

    err = api->CloseConfigFile();
    if (EECode_OK != err) {
        LOG_ERROR("CloseConfigFile fail\n");
    }

    api->Destroy();
    api = NULL;
    return err;
}

EECode gfLoadSimpleLogConfigFile(TChar *simple_configfile, TUint &loglevel, TUint &logoption, TUint &logoutput)
{
    if (DLikely(simple_configfile)) {
        FILE *file = fopen(simple_configfile, "rt");
        if (file) {
            TChar buf[128] = {0};
            TInt ret = fread(buf, 1, 128, file);
            if (ret > 0) {
                if (3 == sscanf(buf, "logconfig:%d,%d,%d", &loglevel, &logoption, &logoutput)) {
                    LOG_NOTICE("read simple logconfig done, loglevel %d, logoption 0x%08x, logoutput 0x%08x\n", loglevel, logoption, logoutput);
                    fclose(file);
                    return EECode_OK;
                }
            }
            fclose(file);
        }
    }

    return EECode_BadParam;
}

EECode gfPrintLogConfig(SLogConfig *p_config, TUint max_element_number)
{
    TUint i = 0;
    EECode err;
    const TChar *module_string = NULL;
    const TChar *level_string = NULL;

    DASSERT(max_element_number <= ELogModuleListEnd);
    DASSERT(max_element_number > 0);
    if (max_element_number > ELogModuleListEnd) {
        max_element_number = ELogModuleListEnd;
    }

    DASSERT(p_config);
    if (!p_config) {
        LOG_FATAL("p_config %p NULL\n", p_config);
        return EECode_NoMemory;
    }

    LOG_CONFIGURATION("\tprint log config:\n");
    for (i = 0; i < max_element_number; i++, p_config ++) {

        err = gfGetLogModuleString((ELogModule) i, module_string);
        if (EECode_OK != err) {
            LOG_FATAL("gfGetLogModuleString fail, ret %s, %d\n", gfGetErrorCodeString(err), err);
            return EECode_InternalLogicalBug;
        }

        err = gfGetLogLevelString((ELogLevel)p_config->log_level, level_string);
        if (EECode_OK != err) {
            LOG_ERROR("gfGetLogLevelString fail, ret %s, %d\n", gfGetErrorCodeString(err), err);
            return EECode_BadParam;
        }

        LOG_CONFIGURATION("\t\tmodule %s:\tlog_level %s(%d), \tlog_option 0x%08x, \tlog_output 0x%08x\n", module_string, level_string, p_config->log_level, p_config->log_option, p_config->log_output);

        LOG_CONFIGURATION("\t\t\tenabled options:\n");
        if (p_config->log_option & DCAL_BITMASK(ELogOption_State)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_State, flag 0x%08x\n", DCAL_BITMASK(ELogOption_State));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_Congifuration)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_Congifuration, flag 0x%08x\n", DCAL_BITMASK(ELogOption_Congifuration));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_Flow)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_Flow, flag 0x%08x\n", DCAL_BITMASK(ELogOption_Flow));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_PTS)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_PTS, flag 0x%08x\n", DCAL_BITMASK(ELogOption_PTS));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_Resource)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_Resource, flag 0x%08x\n", DCAL_BITMASK(ELogOption_Resource));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_Timing)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_Timing, flag 0x%08x\n", DCAL_BITMASK(ELogOption_Timing));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_BinaryHeader)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_BinaryHeader, flag 0x%08x\n", DCAL_BITMASK(ELogOption_BinaryHeader));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_WholeBinary2WholeFile)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_WholeBinary2WholeFile, flag 0x%08x\n", DCAL_BITMASK(ELogOption_WholeBinary2WholeFile));
        }
        if (p_config->log_option & DCAL_BITMASK(ELogOption_WholeBinary2SeparateFile)) {
            LOG_CONFIGURATION("\t\t\t\tELogOption_WholeBinary2SeparateFile, flag 0x%08x\n", DCAL_BITMASK(ELogOption_WholeBinary2SeparateFile));
        }

        LOG_CONFIGURATION("\t\t\tenabled output:\n");
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_Console)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_Console, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_Console));
        }
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_File)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_File, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_File));
        }
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_ModuleFile)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_ModuleFile, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_ModuleFile));
        }
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_AndroidLogcat)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_AndroidLogcat, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_AndroidLogcat));
        }
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_BinaryTotalFile)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_BinaryTotalFile, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_BinaryTotalFile));
        }
        if (p_config->log_output & DCAL_BITMASK(ELogOutput_BinarySeperateFile)) {
            LOG_CONFIGURATION("\t\t\t\tELogOutput_BinarySeperateFile, flag 0x%08x\n", DCAL_BITMASK(ELogOutput_BinarySeperateFile));
        }

    }

    return EECode_OK;
}

EECode gfReadMediaConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name, SPersistMediaConfig *p_config)
{
    TChar name[256], value[256];
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(config_type);
    if (!api) {
        return EECode_Error;
    }
    if (EECode_OK != api->OpenConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"streaming_enable", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->streaming_enable);
    }

    if (EECode_OK == api->GetContentValue((const TChar *)"rtsp_streaming_enable", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->rtsp_streaming_enable);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"rtsp_streaming_video_enable", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->rtsp_streaming_video_enable);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"rtsp_streaming_audio_enable", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->rtsp_streaming_audio_enable);
    }

    if (EECode_OK == api->GetContentValue((const TChar *)"rtsp_listen_port", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->rtsp_server_config.rtsp_listen_port);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"rtp_rtcp_port_start", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->rtsp_server_config.rtp_rtcp_port_start);
    }

    if (EECode_OK == api->GetContentValue((const TChar *)"tiled_mode", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->dsp_config.tiled_mode);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"max_frm_num", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->dsp_config.max_frm_num);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"enable_deint", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->dsp_config.enable_deint);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"max_fifo_size", value)) {
        sscanf((const TChar *)value, "%u", &p_config->dsp_config.max_fifo_size);
    }

    if (EECode_OK == api->GetContentValue((const TChar *)"cutfile_with_precise_pts", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->cutfile_with_precise_pts);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"muxer_skip_video", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->muxer_skip_video);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"muxer_skip_audio", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->muxer_skip_audio);
    }
    if (EECode_OK == api->GetContentValue((const TChar *)"muxer_skip_pridata", value)) {
        sscanf((const TChar *)value, "%u", (TUint *)&p_config->muxer_skip_pridata);
    }
    for (TInt idx = 0; idx < EComponentMaxNumber_Muxer; idx++) {
        sprintf((TChar *)name, "STargetRecorderConfig_%d/container_format", idx);
        if (EECode_OK == api->GetContentValue((const TChar *)name, value)) {
            sscanf((const TChar *)value, "%u", &p_config->target_recorder_config[idx].container_format);
        }
    }

    if (EECode_OK != api->CloseConfigFile()) {
        api->Destroy();
        return EECode_Error;
    }

    api->Destroy();
    return EECode_OK;
}

EECode gfReadLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name)
{
    TChar name[256], value[256];
    EECode err;
    const TChar *module_string;
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(config_type);
    if (!api) {
        return EECode_Error;
    }
    if (EECode_OK != api->OpenConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }

    for (TInt module = ELogGlobal; module < ELogModuleListEnd; module++) {

        err = gfGetLogModuleString((ELogModule)module, module_string);
        if (EECode_OK != err) {
            LOG_FATAL("please check code\n");
            return EECode_InternalLogicalBug;
        }

        sprintf((TChar *)name, "%s/log_level", module_string);
        if (EECode_OK == api->GetContentValue((const TChar *)name, value)) {
            sscanf((const TChar *)value, "%u", &gmModuleLogConfig[module].log_level);
        }
        sprintf((TChar *)name, "%s/log_option", module_string);
        if (EECode_OK == api->GetContentValue((const TChar *)name, value)) {
            sscanf((const TChar *)value, "%u", &gmModuleLogConfig[module].log_option);
        }
        sprintf((TChar *)name, "%s/log_output", module_string);
        if (EECode_OK == api->GetContentValue((const TChar *)name, value)) {
            sscanf((const TChar *)value, "%u", &gmModuleLogConfig[module].log_output);
        }
    }

    if (EECode_OK != api->CloseConfigFile()) {
        api->Destroy();
        return EECode_Error;
    }
    api->Destroy();
    return EECode_OK;
}

EECode gfWriteMediaConfigFile(ERunTimeConfigType config_type, const volatile SPersistMediaConfig *p_config, const TChar *config_file_name)
{
    TChar name[256], value[256];
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(config_type);
    if (!api) {
        return EECode_Error;
    }
    if (EECode_OK != api->OpenConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }

    sprintf((TChar *)value, "%u", p_config->streaming_enable);
    if (EECode_OK != api->SetContentValue((const TChar *)"streaming_enable", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }

    sprintf((TChar *)value, "%u", p_config->rtsp_streaming_enable);
    if (EECode_OK != api->SetContentValue((const TChar *)"rtsp_streaming_enable", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->rtsp_streaming_video_enable);
    if (EECode_OK != api->SetContentValue((const TChar *)"rtsp_streaming_video_enable", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->rtsp_streaming_audio_enable);
    if (EECode_OK != api->SetContentValue((const TChar *)"rtsp_streaming_audio_enable", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }

    sprintf((TChar *)value, "%u", p_config->rtsp_server_config.rtsp_listen_port);
    if (EECode_OK != api->SetContentValue((const TChar *)"rtsp_listen_port", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->rtsp_server_config.rtp_rtcp_port_start);
    if (EECode_OK != api->SetContentValue((const TChar *)"rtp_rtcp_port_start", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }

    sprintf((TChar *)value, "%u", p_config->dsp_config.tiled_mode);
    if (EECode_OK != api->SetContentValue((const TChar *)"tiled_mode", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->dsp_config.max_frm_num);
    if (EECode_OK != api->SetContentValue((const TChar *)"max_frm_num", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->dsp_config.enable_deint);
    if (EECode_OK != api->SetContentValue((const TChar *)"enable_deint", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->dsp_config.max_fifo_size);
    if (EECode_OK != api->SetContentValue((const TChar *)"max_fifo_size", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }

    sprintf((TChar *)value, "%u", p_config->cutfile_with_precise_pts);
    if (EECode_OK != api->SetContentValue((const TChar *)"cutfile_with_precise_pts", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->muxer_skip_video);
    if (EECode_OK != api->SetContentValue((const TChar *)"muxer_skip_video", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->muxer_skip_audio);
    if (EECode_OK != api->SetContentValue((const TChar *)"muxer_skip_audio", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    sprintf((TChar *)value, "%u", p_config->muxer_skip_pridata);
    if (EECode_OK != api->SetContentValue((const TChar *)"muxer_skip_pridata", value)) {
        LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
        api->Destroy();
        return EECode_Error;
    }
    for (TInt idx = 0; idx < EComponentMaxNumber_Muxer; idx++) {
        sprintf((TChar *)name, "STargetRecorderConfig_%d/container_format", idx);
        sprintf((TChar *)value, "%u", p_config->target_recorder_config[idx].container_format);
        if (EECode_OK != api->SetContentValue((const TChar *)name, value)) {
            LOG_ERROR("WriteMediaConfigFile SetContentValue failed.\n");
            api->Destroy();
            return EECode_Error;
        }
    }

    if (EECode_OK != api->SaveConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }
    if (EECode_OK != api->CloseConfigFile()) {
        api->Destroy();
        return EECode_Error;
    }
    api->Destroy();
    return EECode_OK;
}

EECode gfWriteLogConfigFile(ERunTimeConfigType config_type, const TChar *config_file_name)
{
    TChar name[256], value[256];
    EECode err;
    const TChar *module_string;
    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(config_type);
    if (!api) {
        return EECode_Error;
    }
    if (EECode_OK != api->OpenConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }

    for (TInt module = ELogGlobal; module < ELogModuleListEnd; module++) {

        err = gfGetLogModuleString((ELogModule)module, module_string);
        if (EECode_OK != err) {
            LOG_FATAL("please check code\n");
            return EECode_InternalLogicalBug;
        }

        sprintf((TChar *)name, "%s/log_level", module_string);
        sprintf((TChar *)value, "%u", gmModuleLogConfig[module].log_level);
        if (EECode_OK != api->SetContentValue((const TChar *)name, value)) {
            LOG_ERROR("WriteLogConfigFile SetContentValue failed.\n");
            api->Destroy();
            return EECode_Error;
        }
        sprintf((TChar *)name, "%s/log_option", module_string);
        sprintf((TChar *)value, "%u", gmModuleLogConfig[module].log_option);
        if (EECode_OK != api->SetContentValue((const TChar *)name, value)) {
            LOG_ERROR("WriteLogConfigFile SetContentValue failed.\n");
            api->Destroy();
            return EECode_Error;
        }
        sprintf((TChar *)name, "%s/log_output", module_string);
        sprintf((TChar *)value, "%u", gmModuleLogConfig[module].log_output);
        if (EECode_OK != api->SetContentValue((const TChar *)name, value)) {
            LOG_ERROR("WriteLogConfigFile SetContentValue failed.\n");
            api->Destroy();
            return EECode_Error;
        }
    }

    if (EECode_OK != api->SaveConfigFile(config_file_name)) {
        api->Destroy();
        return EECode_Error;
    }
    if (EECode_OK != api->CloseConfigFile()) {
        api->Destroy();
        return EECode_Error;
    }
    api->Destroy();
    return EECode_OK;
}

void gfPrintCodecInfoes(SStreamCodecInfos *pInfo)
{
    TUint i = 0;
    SStreamCodecInfo *info = NULL;

    DASSERT(pInfo);
    if (pInfo) {
        info = pInfo->info;
        for (i = 0; i < pInfo->total_stream_number; i ++, info++) {
            LOG_ALWAYS("codec_id %d, stream_index %d, stream_presence %d, stream_enabled %d, inited %d\n", info->codec_id, info->stream_index, info->stream_presence, info->stream_enabled, info->inited);
            if (StreamType_Video == info->stream_type) {
                LOG_ALWAYS("video: format %d, parameters as follow\n", info->stream_format);
                LOG_ALWAYS("    pic_width %d, pic_height %d, pic_offset_x %d, pic_offset_y %d\n", info->spec.video.pic_width, info->spec.video.pic_height, info->spec.video.pic_offset_x, info->spec.video.pic_offset_y);
                LOG_ALWAYS("    framerate_num %d, framerate_den %d, framerate %f bitrate %d\n", info->spec.video.framerate_num, info->spec.video.framerate_den, info->spec.video.framerate, info->spec.video.bitrate);
                LOG_ALWAYS("    M %d, N %d, IDRInterval %d\n", info->spec.video.M, info->spec.video.N, info->spec.video.IDRInterval);
                LOG_ALWAYS("    sample_aspect_ratio_num %d, sample_aspect_ratio_den %d, lowdelay %d, entropy_type %d\n", info->spec.video.sample_aspect_ratio_num, info->spec.video.sample_aspect_ratio_den, info->spec.video.lowdelay, info->spec.video.entropy_type);
            } else if (StreamType_Audio == info->stream_type) {
                LOG_ALWAYS("audio: format %d, parameters as follow\n", info->stream_format);
                LOG_ALWAYS("    sample_rate %d, sample_format %d, channel_number %d\n", info->spec.audio.sample_rate, info->spec.audio.sample_format, info->spec.audio.channel_number);
                LOG_ALWAYS("    channel_layout %d, frame_size %d, bitrate %d\n", info->spec.audio.channel_layout, info->spec.audio.frame_size, info->spec.audio.bitrate);
                LOG_ALWAYS("    need_skip_adts_header %d, pts_unit_num %d, pts_unit_den %d\n", info->spec.audio.need_skip_adts_header, info->spec.audio.pts_unit_num, info->spec.audio.pts_unit_den);
                LOG_ALWAYS("    is_channel_interlave %d, is_big_endian %d, codec_format %d, customized_codec_type %d\n", info->spec.audio.is_channel_interlave, info->spec.audio.is_big_endian, info->spec.audio.codec_format, info->spec.audio.customized_codec_type);
            } else if (StreamType_Subtitle == info->stream_type) {
                LOG_FATAL("TO DO\n");
            } else if (StreamType_PrivateData == info->stream_type) {
                LOG_FATAL("TO DO\n");
            } else {
                LOG_FATAL("BAD info->stream_type %d\n", info->stream_type);
            }
        }
    }
}

TU8 *gfGenerateAudioExtraData(StreamFormat format, TInt samplerate, TInt channel_number, TUint &size)
{
    SSimpleAudioSpecificConfig *p_simple_header = NULL;
    size = 0;

    switch (format) {

        case StreamFormat_AAC:
            size = 2;
            p_simple_header = (SSimpleAudioSpecificConfig *) DDBG_MALLOC((size + 3) & (~3), "GAE0");
            p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
            switch (samplerate) {
                case 44100:
                    samplerate = eSamplingFrequencyIndex_44100;
                    break;
                case 48000:
                    samplerate = eSamplingFrequencyIndex_48000;
                    break;
                case 24000:
                    samplerate = eSamplingFrequencyIndex_24000;
                    break;
                case 16000:
                    samplerate = eSamplingFrequencyIndex_16000;
                    break;
                case 8000:
                    samplerate = eSamplingFrequencyIndex_8000;
                    break;
                case 12000:
                    samplerate = eSamplingFrequencyIndex_12000;
                    break;
                case 32000:
                    samplerate = eSamplingFrequencyIndex_32000;
                    break;
                case 22050:
                    samplerate = eSamplingFrequencyIndex_22050;
                    break;
                case 11025:
                    samplerate = eSamplingFrequencyIndex_11025;
                    break;
                default:
                    LOG_ERROR("NOT support sample rate (%d) here.\n", samplerate);
                    break;
            }
            p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
            p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
            p_simple_header->channelConfiguration = channel_number;
            p_simple_header->bitLeft = 0;
            break;

        default:
            LOG_ERROR("NOT support audio codec (%d) here.\n", format);
            break;
    }

    return (TU8 *)p_simple_header;
}

IScheduler *gfGetNetworkReceiverScheduler(const volatile SPersistMediaConfig *p, TUint index)
{
    DASSERT(p);

    if (p) {
        TUint num = 1;

        DASSERT(p->number_scheduler_network_reciever);
        DASSERT(p->number_scheduler_network_reciever <= DMaxSchedulerGroupNumber);

        if (p->number_scheduler_network_reciever > DMaxSchedulerGroupNumber) {
            num = 1;
            LOG_FATAL("BAD p->number_scheduler_network_reciever %d\n", p->number_scheduler_network_reciever);
        } else {
            num = p->number_scheduler_network_reciever;
            if (!num) {
                num = 1;
                LOG_FATAL("BAD p->number_scheduler_network_reciever %d\n", p->number_scheduler_network_reciever);
            }
        }

        index = index % num;

        return p->p_scheduler_network_reciever[index];
    } else {
        LOG_FATAL("NULL p\n");
    }

    return NULL;
}

IScheduler *gfGetNetworkReceiverTCPScheduler(const volatile SPersistMediaConfig *p, TUint index)
{
    DASSERT(p);

    if (p) {
        TUint num = 1;

        DASSERT(p->number_scheduler_network_tcp_reciever);
        DASSERT(p->number_scheduler_network_tcp_reciever <= DMaxSchedulerGroupNumber);

        if (p->number_scheduler_network_tcp_reciever > DMaxSchedulerGroupNumber) {
            num = 1;
            LOG_FATAL("BAD p->number_scheduler_network_tcp_reciever %d\n", p->number_scheduler_network_tcp_reciever);
        } else {
            num = p->number_scheduler_network_tcp_reciever;
            if (!num) {
                num = 1;
                LOG_FATAL("BAD p->number_scheduler_network_tcp_reciever %d\n", p->number_scheduler_network_tcp_reciever);
            }
        }

        index = index % num;

        return p->p_scheduler_network_tcp_reciever[index];
    } else {
        LOG_FATAL("NULL p\n");
    }

    return NULL;
}

IScheduler *gfGetFileIOWriterScheduler(const volatile SPersistMediaConfig *p, TU32 index)
{
    DASSERT(p);

    if (p) {
        TUint num = 1;

        DASSERT(p->number_scheduler_io_writer);
        DASSERT(p->number_scheduler_io_writer <= DMaxSchedulerGroupNumber);

        if (p->number_scheduler_io_writer > DMaxSchedulerGroupNumber) {
            num = 1;
            LOG_FATAL("BAD p->number_scheduler_io_writer %d\n", p->number_scheduler_io_writer);
        } else {
            num = p->number_scheduler_io_writer;
            if (!num) {
                num = 1;
                LOG_FATAL("BAD p->number_scheduler_io_writer %d\n", p->number_scheduler_io_writer);
            }
        }

        index = index % num;

        return p->p_scheduler_io_writer[index];
    } else {
        LOG_FATAL("NULL p\n");
    }

    return NULL;
}

ITCPPulseSender *gfGetTCPPulseSender(const volatile SPersistMediaConfig *p, TUint index)
{
    DASSERT(p);

    if (!p->number_tcp_pulse_sender) {
        return NULL;
    }

    if (p) {
        TUint num = 1;

        DASSERT(p->number_tcp_pulse_sender <= DMaxTCPPulseSenderNumber);

        if (p->number_tcp_pulse_sender > DMaxTCPPulseSenderNumber) {
            num = 1;
            LOG_FATAL("BAD p->number_tcp_pulse_sender %d\n", p->number_tcp_pulse_sender);
        } else {
            num = p->number_tcp_pulse_sender;
            if (!num) {
                num = 1;
                LOG_FATAL("BAD p->number_tcp_pulse_sender %d\n", p->number_tcp_pulse_sender);
            }
        }

        index = index % num;

        return p->p_tcp_pulse_sender[index];
    } else {
        LOG_FATAL("NULL p\n");
    }

    return NULL;
}

TMemSize gfSkipDelimter(TU8 *p)
{
    if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (0x09 == p[4])) {
        return 6;
    }

    return 0;
}

TU32 gfSkipSEI(TU8 *p, TU32 len)
{
    TU8 *po = p;
    TUint state = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    if (ENalType_SEI > (p[1] & 0x1f)) {
                        return (TU32)((p - 2) - po);
                    }
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    if (ENalType_SEI > (p[1] & 0x1f)) {
                        return (TU32)((p - 3) - po);
                    }
                } else {
                    state = 0;
                }
                break;

            default:
                LOG_FATAL("impossible to comes here\n");
                break;

        }
        p ++;
        len --;
    }

    LOG_FATAL("data corruption\n");
    return 0;
}

//version related
#define DMediaMWVesionMajor 0
#define DMediaMWVesionMinor 6
#define DMediaMWVesionPatch 11
#define DMediaMWVesionYear 2015
#define DMediaMWVesionMonth 9
#define DMediaMWVesionDay 7

void gfGetMediaMWVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day)
{
    major = DMediaMWVesionMajor;
    minor = DMediaMWVesionMinor;
    patch = DMediaMWVesionPatch;

    year = DMediaMWVesionYear;
    month = DMediaMWVesionMonth;
    day = DMediaMWVesionDay;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

