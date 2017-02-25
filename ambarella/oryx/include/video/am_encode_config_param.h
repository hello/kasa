/**
 * am_encode_config_param.h
 *
 *  History:
 *    Jul 28, 2015 - [Shupeng Ren] created file
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
 */
#ifndef ORYX_INCLUDE_VIDEO_AM_ENCODE_CONFIG_PARAM_H_
#define ORYX_INCLUDE_VIDEO_AM_ENCODE_CONFIG_PARAM_H_

#include "am_video_types.h"
#include "am_pointer.h"
#include <map>

//For VIN
struct AMVinParam
{
    std::pair<bool, bool>        close_on_idle = {false, true};
    std::pair<bool, AM_SENSOR_TYPE>       type = {false, AM_SENSOR_TYPE_NONE};
    std::pair<bool, AM_VIN_MODE>          mode = {false, AM_VIN_MODE_AUTO};
    std::pair<bool, AM_VIDEO_FLIP>        flip = {false, AM_VIDEO_FLIP_NONE};
    std::pair<bool, AM_VIDEO_FPS>          fps = {false, AM_VIDEO_FPS_AUTO};
    std::pair<bool, AM_VIN_BAYER_PATTERN> bayer_pattern =
                                             {false, AM_VIN_BAYER_PATTERN_AUTO};
};
typedef std::map<AM_VIN_ID, AMVinParam> AMVinParamMap;

//For VOUT
struct AMVoutParam
{
    std::pair<bool, AM_VOUT_TYPE>        type = {false, AM_VOUT_TYPE_NONE};
    std::pair<bool, AM_VOUT_VIDEO_TYPE>  video_type = {false, AM_VOUT_VIDEO_TYPE_NONE};
    std::pair<bool, AM_VOUT_MODE>        mode = {false, AM_VOUT_MODE_AUTO};
    std::pair<bool, AM_VIDEO_FLIP>       flip = {false, AM_VIDEO_FLIP_NONE};
    std::pair<bool, AM_VIDEO_ROTATE>     rotate = {false, AM_VIDEO_ROTATE_NONE};
    std::pair<bool, AM_VIDEO_FPS>        fps = {false, AM_VIDEO_FPS_AUTO};
};
typedef std::map<AM_VOUT_ID, AMVoutParam> AMVoutParamMap;

//For BUFFER
struct AMBufferConfigParam
{
    AM_SOURCE_BUFFER_ID                        id = AM_SOURCE_BUFFER_INVALID;
    std::pair<bool, AM_SOURCE_BUFFER_TYPE>     type = {false, AM_SOURCE_BUFFER_TYPE_OFF};
    std::pair<bool, AMRect>                    input = {false, {}};
    std::pair<bool, AMResolution>              size = {false, {}};
    std::pair<bool, bool>                      input_crop = {false, false};
    std::pair<bool, bool>                      prewarp = {false, false};
};
typedef std::map<AM_SOURCE_BUFFER_ID, AMBufferConfigParam> AMBufferParamMap;

//For STREAM
struct AMStreamFormatConfig
{
    std::pair<bool, bool>                    enable = {false, false};
    std::pair<bool, AM_STREAM_TYPE>          type = {false, AM_STREAM_TYPE_NONE};
    std::pair<bool, AM_SOURCE_BUFFER_ID>     source = {false, AM_SOURCE_BUFFER_INVALID};
    std::pair<bool, int32_t>                 fps = {false, 0};
    std::pair<bool, AMRect>                  enc_win = {false, {}};
    std::pair<bool, AM_VIDEO_FLIP>           flip = {false, AM_VIDEO_FLIP_NONE};
    std::pair<bool, bool>                    rotate_90_cw = {false, false};
};

struct AMStreamH26xConfig
{
    std::pair<bool, AM_GOP_MODEL>           gop_model = {false, AM_GOP_MODEL_SIMPLE};
    std::pair<bool, AM_RATE_CONTROL>        bitrate_control = {false, AM_RC_CBR};
    std::pair<bool, AM_PROFILE>             profile_level = {false, AM_PROFILE_BASELINE};
    std::pair<bool, AM_AU_TYPE>             au_type = {false, AM_AU_TYPE_NO_AUD_NO_SEI};
    std::pair<bool, AM_CHROMA_FORMAT>       chroma_format = {false, AM_CHROMA_FORMAT_YUV420};
    std::pair<bool, uint32_t>               M = {false, 0};
    std::pair<bool, uint32_t>               N = {false, 0};
    std::pair<bool, uint32_t>               idr_interval = {false, 0};
    std::pair<bool, uint32_t>               target_bitrate = {false, 0};
    std::pair<bool, uint32_t>               mv_threshold = {false, 0};
    std::pair<bool, uint32_t>               fast_seek_intvl = {false, 0};
    std::pair<bool, int32_t>                i_frame_max_size = {false, 0};
    std::pair<bool, bool>                   flat_area_improve = {false, false};
    std::pair<bool, bool>                   multi_ref_p = {false, false};
    std::pair<bool, AMResolution>           sar = {false,{1,1}};
};

struct AMStreamMJPEGConfig
{
    std::pair<bool, AM_CHROMA_FORMAT>  chroma_format = {false, AM_CHROMA_FORMAT_YUV420};
    std::pair<bool, uint32_t>          quality = {false, 0};
};

struct AMStreamConfigParam
{
    std::pair<bool, AMStreamFormatConfig>  stream_format = {false, {}};
    std::pair<bool, AMStreamH26xConfig>    h26x_config = {false, {}};
    std::pair<bool, AMStreamMJPEGConfig>   mjpeg_config = {false, {}};
};
typedef std::map<AM_STREAM_ID, AMStreamConfigParam> AMStreamParamMap;

#endif /* ORYX_INCLUDE_VIDEO_AM_ENCODE_CONFIG_PARAM_H_ */
