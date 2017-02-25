/**
 * am_encode_types.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_TYPES_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_TYPES_H_

#include "am_video_types.h"

#define MAX_VIDEO_MODE_STRING_LENGTH  64
#define MAX_VIDEO_FPS_STRING_LENGTH   12

enum AM_VIN_STATE
{
  AM_VIN_STATE_NOT_INITIALIZED = 0,
  AM_VIN_STATE_RUNNING = 1,
  AM_VIN_STATE_STOPPED = 2,
};

enum AM_VOUT_COLOR_CONVERSION
{
  AM_VOUT_COLOR_CONVERSION_NONE = 0,
};

enum AM_LCD_PANEL_TYPE
{
  AM_LCD_PANEL_NONE     = 0,
  AM_LCD_PANEL_DIGITAL  = 1,
  AM_LCD_PANEL_OTHERS
};

enum AM_OSD_BLEND_MIXER
{
  AM_OSD_BLEND_MIXER_OFF  = 0,
  AM_OSD_BLEND_MIXER_A    = 1,
  AM_OSD_BLEND_MIXER_B    = 2,
};

enum AM_ROTATE_MODE
{
  AM_NO_ROTATE_FLIP   = 0,
  AM_HORIZONTAL_FLIP  = (1 << 0),
  AM_VERTICAL_FLIP    = (1 << 1),
  AM_ROTATE_90        = (1 << 2),

  AM_CW_ROTATE_90     = AM_ROTATE_90,
  AM_CW_ROTATE_180    = AM_HORIZONTAL_FLIP | AM_VERTICAL_FLIP,
  AM_CW_ROTATE_270    = AM_CW_ROTATE_90 | AM_CW_ROTATE_180,
};

enum AM_IAV_STATE
{
  AM_IAV_STATE_INIT             = -1,
  AM_IAV_STATE_IDLE             = 0,
  AM_IAV_STATE_PREVIEW          = 1,
  AM_IAV_STATE_PREVIEW_LOW      = 2,
  AM_IAV_STATE_ENCODING         = 3,
  AM_IAV_STATE_EXITING_PREVIEW  = 4,
  AM_IAV_STATE_DECODING         = 5,
  AM_IAV_STATE_ERROR            = 6,
};

struct AMFps
{
    int32_t mul = 0;
    int32_t div = 0;
    int32_t fps = -1;
};

struct AMScaleFactor
{
    uint32_t num = 0;
    uint32_t den = 0;
};

struct AMMemMapInfo
{
    uint8_t *addr = nullptr;
    uint32_t length = 0;
};

struct AMVinAGC {
    uint32_t agc = 0;
    uint32_t agc_max = 0;
    uint32_t agc_min = 0;
    uint32_t agc_step = 0;
    uint32_t wdr_again_idx_min = 0;
    uint32_t wdr_again_idx_max = 0;
    uint32_t wdr_dgain_idx_min = 0;
    uint32_t wdr_dgain_idx_max = 0;
};

typedef uint32_t AMVideoFpsFormatQ9;

struct AMVinModeTable
{
    const char    name[MAX_VIDEO_MODE_STRING_LENGTH];
    uint32_t      hash_value;
    AM_VIN_MODE   mode;
    int32_t       width;
    int32_t       height;
};

struct AMVoutModeTable
{
    const char      name[MAX_VIDEO_MODE_STRING_LENGTH];
    uint32_t        hash_value;
    AM_VOUT_MODE    mode;
    AM_VIDEO_FPS    video_fps;
    int32_t         width;
    int32_t         height;
};

struct AMVideoFPSTable
{
    const char    name[MAX_VIDEO_FPS_STRING_LENGTH];
    uint32_t      hash_value;
    AM_VIDEO_FPS  video_fps;
    uint32_t      video_fps_q9_format;
};

struct AMVinAAAInfo
{
    uint32_t vsrc_id;
    uint32_t sensor_id;
    uint32_t bayer_pattern;
    uint32_t agc_step;
    uint32_t hdr_mode;
    uint32_t hdr_long_offset;
    uint32_t hdr_short1_offset;
    uint32_t hdr_short2_offset;
    uint32_t hdr_short3_offset;
    uint32_t pixel_size;
    uint32_t slow_shutter_support;
    uint32_t dual_gain_mode;
    uint32_t line_time;
    uint32_t vb_time;
};
#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_TYPES_H_ */
