/*******************************************************************************
 * am_video_types.h
 *
 * Histroy:
 *  2012-2-20 2012 - [ypchang] created file
 *  2014-6-24      - [Louis ] modified and created am_video_types.h, removed non video stuffs
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

#ifndef AM_VIDEO_TYPES_H_
#define AM_VIDEO_TYPES_H_

#include "am_base_include.h"
#include "am_result.h"
#include <vector>
#include <map>

using std::pair;

enum AM_VIN_ID
{
  AM_VIN_ID_INVALID = -1,
  AM_VIN_0          = 0,     /* Main VIN */
  AM_VIN_1          = 1,     /* Second VIN, usually NULL */
  AM_VIN_MAX_NUM    = 2
};

enum AM_VIN_MODE
{
  AM_VIN_MODE_AUTO = 0,   // Auto Adaptive to the sensor
  AM_VIN_MODE_VGA,        // 640x480
  AM_VIN_MODE_720P,       //1280x720
  AM_VIN_MODE_960P,       //1280x960
  AM_VIN_MODE_1024P,      //1280x1024
  AM_VIN_MODE_1080P,      //1920x1080
  AM_VIN_MODE_1920X1200,  //1920x1200
  AM_VIN_MODE_3M_4_3,     //2048x1536
  AM_VIN_MODE_3M_16_9,    //2304x1296
  AM_VIN_MODE_QHD,        //2560x1440
  AM_VIN_MODE_4M_16_9,    //2688x1512
  AM_VIN_MODE_2688X1520,  //2688x1520
  AM_VIN_MODE_5M_4_3,     //2592x1944
  AM_VIN_MODE_6M_3_2,     //3072x2048
  AM_VIN_MODE_QFHD,       //3840x2160
  AM_VIN_MODE_4096X2160,  //4096x2160
  AM_VIN_MODE_12M,        //4000x3000
  AM_VIN_MODE_14M,        //4000x3000
  AM_VIN_MODE_16M,        //4000x3000
  AM_VIN_MODE_18M,        //4000x3000

  AM_VIN_MODE_CURRENT,
  AM_VIN_MODE_NUM,

  AM_VIN_MODE_OFF,
};

enum AM_VIN_BITS
{
  AM_VIN_BITS_AUTO = 0,
  AM_VIN_BITS_8,
  AM_VIN_BITS_10,
  AM_VIN_BITS_12,
  AM_VIN_BITS_14,
  AM_VIN_BITS_16,
  AM_VIN_BITS_NUM,
};

enum AM_VIN_TYPE_CUR
{
  AM_VIN_TYPE_AUTO       = 0,
  AM_VIN_TYPE_YUV_601    = 1,
  AM_VIN_TYPE_YUV_656    = 2,
  AM_VIN_TYPE_RGB_601    = 3,
  AM_VIN_TYPE_RGB_656    = 4,
  AM_VIN_TYPE_RGB_RAW    = 5,
  AM_VIN_TYPE_YUV_BT1120 = 6,
  AM_VIN_TYPE_RGB_BT1120 = 7,
};
enum AM_VIN_TYPE
{
  AM_VIN_TYPE_INVALID = -1,
  AM_VIN_TYPE_SENSOR  = 0,
  AM_VIN_TYPE_DECODE,
};

enum AM_SENSOR_TYPE
{
  AM_SENSOR_TYPE_NONE = 0,
  AM_SENSOR_TYPE_RGB,
  AM_SENSOR_TYPE_YUV,
  AM_SENSOR_TYPE_YUV_GENERIC
};

enum AM_VIN_BAYER_PATTERN
{
  AM_VIN_BAYER_PATTERN_AUTO = 0,
  AM_VIN_BAYER_PATTERN_RG = 1,
  AM_VIN_BAYER_PATTERN_BG = 2,
  AM_VIN_BAYER_PATTERN_GR = 3,
  AM_VIN_BAYER_PATTERN_GB = 4,
};

enum AM_HDR_TYPE
{
  AM_HDR_SINGLE_EXPOSURE  = 0, //!< NO HDR, which is for non HDR scene (linear)
  AM_HDR_2_EXPOSURE       = 1, //!< HDR 2x
  AM_HDR_3_EXPOSURE       = 2, //!< HDR 3x
  AM_HDR_4_EXPOSURE       = 3, //!< HDR 4x
  AM_HDR_SENSOR_INTERNAL  = 15, //!< Sensor built-in WDR
  AM_HDR_TYPE_NUM
};

enum AM_VIN_RATIO
{
  AM_VIN_RATIO_AUTO = 0,
  AM_VIN_RATIO_4_3  = 1,
  AM_VIN_RATIO_16_9 = 2,
  AM_VIN_RATIO_1_1  = 4,
};

enum AM_VIN_SYSTEM
{
  AM_VIN_SYSTEM_AUTO  = 0,
  AM_VIN_SYSTEM_NTSC  = 1,
  AM_VIN_SYSTEM_PAL   = 2,
  AM_VIN_SYSTEM_SECAM = 4,
  AM_VIN_SYSTEM_ALL   = 15,
};

enum AM_VOUT_ID
{
  AM_VOUT_ID_INVALID  = -1,
  AM_VOUT_ID_HDMI     = 0,  //VOUT-B, which is usually associated with Preview-B
  AM_VOUT_ID_LCD      = 1,  //VOUT-A, which is usually associated with Preview-A
  AM_VOUT_A           = AM_VOUT_ID_LCD,
  AM_VOUT_B           = AM_VOUT_ID_HDMI,
  AM_VOUT_MAX_NUM     = 2,
};

enum AM_VOUT_TYPE
{
  AM_VOUT_TYPE_NONE     = 0,
  AM_VOUT_TYPE_LCD      = 1,  //LCD interface, digital video
  AM_VOUT_TYPE_HDMI     = 2,  //HDMI
  AM_VOUT_TYPE_CVBS     = 3,  //composite video
  AM_VOUT_TYPE_YPBPR    = 4,  //component video
};

enum AM_VOUT_VIDEO_TYPE
{
  AM_VOUT_VIDEO_TYPE_NONE     = 0,
  AM_VOUT_VIDEO_TYPE_YUV601   = 1,
  AM_VOUT_VIDEO_TYPE_YUV656   = 2,
  AM_VOUT_VIDEO_TYPE_YUV1120  = 3,
  AM_VOUT_VIDEO_TYPE_RGB601   = 4,
  AM_VOUT_VIDEO_TYPE_RGB656   = 5,
  AM_VOUT_VIDEO_TYPE_RGB1120  = 6,
};

enum AM_VOUT_STATE
{
  AM_VOUT_STATE_NOT_INITIALIZED = 0,
  AM_VOUT_STATE_RUNNING = 1,
  AM_VOUT_STATE_STOPPED = 2,
};

enum AM_VOUT_MODE
{
  AM_VOUT_MODE_AUTO = 0, //Auto Adaptive to the vout device

  AM_VOUT_MODE_480P = 1, //480p60
  AM_VOUT_MODE_576P = 2, //576p50

  AM_VOUT_MODE_480I = 3,
  AM_VOUT_MODE_576I = 4,

  AM_VOUT_MODE_NTSC = 3,
  AM_VOUT_MODE_PAL  = 4,

  AM_VOUT_MODE_720P60 = 10,
  AM_VOUT_MODE_720P50 = 11,

  AM_VOUT_MODE_720P30 = 12,
  AM_VOUT_MODE_720P25 = 13,

  AM_VOUT_MODE_1080P60 = 20,
  AM_VOUT_MODE_1080P50 = 21,
  AM_VOUT_MODE_1080P30 = 22,
  AM_VOUT_MODE_1080P25 = 23,
  AM_VOUT_MODE_1080P24 = 24,
  AM_VOUT_MODE_1080I60 = 25,
  AM_VOUT_MODE_1080I50 = 26,

  AM_VOUT_MODE_QFHD30 = 40, // 3840x2160p30
  AM_VOUT_MODE_QFHD24 = 41, // 3840x2160p24
};

enum AM_VIDEO_FPS
{
  AM_VIDEO_FPS_AUTO = 0,
  AM_VIDEO_FPS_1 = 1,
  AM_VIDEO_FPS_2 = 2,
  AM_VIDEO_FPS_3 = 3,
  AM_VIDEO_FPS_4 = 4,
  AM_VIDEO_FPS_5 = 5,
  AM_VIDEO_FPS_6 = 6,
  AM_VIDEO_FPS_7 = 7,
  AM_VIDEO_FPS_8 = 8,
  AM_VIDEO_FPS_9 = 9,
  AM_VIDEO_FPS_10 = 10,
  AM_VIDEO_FPS_11 = 11,
  AM_VIDEO_FPS_12 = 12,
  AM_VIDEO_FPS_13 = 13,
  AM_VIDEO_FPS_14 = 14,
  AM_VIDEO_FPS_15 = 15,
  AM_VIDEO_FPS_16 = 16,
  AM_VIDEO_FPS_17 = 17,
  AM_VIDEO_FPS_18 = 18,
  AM_VIDEO_FPS_19 = 19,
  AM_VIDEO_FPS_20 = 20,
  AM_VIDEO_FPS_21 = 21,
  AM_VIDEO_FPS_22 = 22,
  AM_VIDEO_FPS_23 = 23,
  AM_VIDEO_FPS_24 = 24,
  AM_VIDEO_FPS_25 = 25,
  AM_VIDEO_FPS_26 = 26,
  AM_VIDEO_FPS_27 = 27,
  AM_VIDEO_FPS_28 = 28,
  AM_VIDEO_FPS_29 = 29,
  AM_VIDEO_FPS_30 = 30,
  AM_VIDEO_FPS_45 = 45,
  AM_VIDEO_FPS_50 = 50,
  AM_VIDEO_FPS_60 = 60,
  AM_VIDEO_FPS_90 = 90,
  AM_VIDEO_FPS_120 = 120,
  AM_VIDEO_FPS_240 = 240,

  //below are fraction
  AM_VIDEO_FPS_29_97 = 1000,
  AM_VIDEO_FPS_59_94 = 1001,
  AM_VIDEO_FPS_23_976 = 1002,
  AM_VIDEO_FPS_12_5 = 1003,
  AM_VIDEO_FPS_6_25 = 1004,
  AM_VIDEO_FPS_3_125 = 1005,
  AM_VIDEO_FPS_7_5 = 1006,
  AM_VIDEO_FPS_3_75 = 1007,
};

enum AM_VIDEO_ROTATE
{
  AM_VIDEO_ROTATE_NONE  = 0,
  AM_VIDEO_ROTATE_90    = 1
};

enum AM_VIDEO_FLIP
{
  AM_VIDEO_FLIP_NONE        = 0,
  AM_VIDEO_FLIP_VERTICAL    = 1,
  AM_VIDEO_FLIP_HORIZONTAL  = 2,
  AM_VIDEO_FLIP_VH_BOTH     = 3,
  AM_VIDEO_FLIP_AUTO        = 255 //auto flip , decided by device driver
};

enum AM_SOURCE_BUFFER_ID
{
  AM_SOURCE_BUFFER_INVALID  = -1,
  AM_SOURCE_BUFFER_MAIN     = 0,
  AM_SOURCE_BUFFER_2ND      = 1,
  AM_SOURCE_BUFFER_3RD      = 2,
  AM_SOURCE_BUFFER_4TH      = 3,
  AM_SOURCE_BUFFER_5TH      = 4,
  AM_SOURCE_BUFFER_PMN      = 5,
  AM_SOURCE_BUFFER_EFM      = 6,

  AM_SOURCE_BUFFER_PREVIEW_A   = AM_SOURCE_BUFFER_4TH,
  AM_SOURCE_BUFFER_PREVIEW_B   = AM_SOURCE_BUFFER_3RD,
  AM_SOURCE_BUFFER_PREVIEW_C   = AM_SOURCE_BUFFER_2ND,
  AM_SOURCE_BUFFER_PREVIEW_D   = AM_SOURCE_BUFFER_5TH,
};

enum AM_DSP_SUB_BUF_ID
{
  AM_DSP_SUB_BUF_INVALID  = -1,
  AM_DSP_SUB_BUF_RAW      = 0,
  AM_DSP_SUB_BUF_MAIN_YUV,
  AM_DSP_SUB_BUF_MAIN_ME,
  AM_DSP_SUB_BUF_2ND_YUV,
  AM_DSP_SUB_BUF_2ND_ME,
  AM_DSP_SUB_BUF_3RD_YUV,
  AM_DSP_SUB_BUF_3RD_ME,
  AM_DSP_SUB_BUF_4TH_YUV,
  AM_DSP_SUB_BUF_4TH_ME,
  AM_DSP_SUB_BUF_5TH_YUV,
  AM_DSP_SUB_BUF_EFM_YUV,
  AM_DSP_SUB_BUF_EFM_ME,
  AM_DSP_SUB_BUF_POST_MAIN_YUV,
  AM_DSP_SUB_BUF_POST_MAIN_ME,
  AM_DSP_SUB_BUF_INT_MAIN_YUV,
  AM_DSP_SUB_BUF_CFA_AAA,
  AM_DSP_SUB_BUF_RGB_AAA,
  AM_DSP_SUB_BUF_VIN_STATS,

  AM_DSP_SUB_BUF_PREV_A_YUV  = AM_DSP_SUB_BUF_4TH_YUV,
  AM_DSP_SUB_BUF_PREV_A_ME   = AM_DSP_SUB_BUF_4TH_ME,
  AM_DSP_SUB_BUF_PREV_B_YUV  = AM_DSP_SUB_BUF_3RD_YUV,
  AM_DSP_SUB_BUF_PREV_B_ME   = AM_DSP_SUB_BUF_3RD_ME,
  AM_DSP_SUB_BUF_PREV_C_YUV  = AM_DSP_SUB_BUF_2ND_YUV,
  AM_DSP_SUB_BUF_PREV_C_ME   = AM_DSP_SUB_BUF_2ND_ME,
  AM_DSP_SUB_BUF_PREV_D_YUV  = AM_DSP_SUB_BUF_5TH_YUV,
};

enum AM_SRCBUF_STATE
{
  AM_SRCBUF_STATE_UNKNOWN = -1,
  AM_SRCBUF_STATE_ERROR   = -2,
  AM_SRCBUF_STATE_IDLE    = 0,
  AM_SRCBUF_STATE_BUSY    = 1,
};

enum AM_SOURCE_BUFFER_TYPE
{
  AM_SOURCE_BUFFER_TYPE_OFF     = 0,
  AM_SOURCE_BUFFER_TYPE_ENCODE  = 1,
  AM_SOURCE_BUFFER_TYPE_PREVIEW = 2,
};

enum AM_STREAM_ID
{
  AM_STREAM_ID_INVALID = -1,  //for initial invalid value
  AM_STREAM_ID_0       = 0,
  AM_STREAM_ID_1,
  AM_STREAM_ID_2,
  AM_STREAM_ID_3,
  AM_STREAM_ID_4,
  AM_STREAM_ID_5,
  AM_STREAM_ID_6,
  AM_STREAM_ID_7,
  AM_STREAM_ID_8,
  AM_STREAM_ID_MAX,
};

enum AM_STREAM_TYPE
{
  AM_STREAM_TYPE_NONE = 0, //none means not configured
  AM_STREAM_TYPE_H264 = 1,
  AM_STREAM_TYPE_H265 = 2,
  AM_STREAM_TYPE_MJPEG = 3,
};

enum AM_STREAM_STATE
{
  AM_STREAM_STATE_IDLE = 0,
  AM_STREAM_STATE_STARTING = 1,
  AM_STREAM_STATE_ENCODING = 2,
  AM_STREAM_STATE_STOPPING = 3,
  AM_STREAM_STATE_ERROR = 4,
};
typedef std::map<AM_STREAM_ID, AM_STREAM_STATE> AMStreamStateMap;

enum AM_GOP_MODEL
{
  AM_GOP_MODEL_SIMPLE = 0, //simpe GOP
  AM_GOP_MODEL_ADVANCED = 1, //hierachical
  AM_GOP_MODEL_SVCT_FIRST = 2, //please don't change this value
  AM_GOP_MODEL_SVCT_LAST = 5, //please don't change this value
};

enum AM_RATE_CONTROL
{
  AM_RC_NONE = -1,
  AM_RC_CBR = 0, //SCBR version
  AM_RC_VBR = 1, //SCBR version
  AM_RC_CBR_QUALITY = 2, //SCBR version
  AM_RC_VBR_QUALITY = 3, //SCBR version
  AM_RC_CBR2 = 4, //old style CBR
  AM_RC_VBR2 = 5, //old style VBR
};

enum AM_PROFILE
{
  AM_PROFILE_BASELINE = 0,
  AM_PROFILE_MAIN = 1,
  AM_PROFILE_HIGH = 2,
};

enum AM_AU_TYPE
{
  AM_AU_TYPE_NO_AUD_NO_SEI = 0,
  AM_AU_TYPE_AUD_BEFORE_SPS_WITH_SEI = 1,
  AM_AU_TYPE_AUD_AFTER_SPS_WITH_SEI = 2,
  AM_AU_TYPE_NO_AUD_WITH_SEI = 3,
};

enum AM_CHROMA_FORMAT
{
  AM_CHROMA_FORMAT_YUV420 = 0,
  AM_CHROMA_FORMAT_YUV422 = 1,
  AM_CHROMA_FORMAT_Y8 = 2,
};

enum AM_ENCODE_MODE
{
  AM_ENCODE_INVALID_MODE    = -1, //Invalid for init
  AM_ENCODE_MODE_0          = 0,
  AM_ENCODE_MODE_1          = 1,
  AM_ENCODE_MODE_2          = 2,
  AM_ENCODE_MODE_3          = 3,
  AM_ENCODE_MODE_4          = 4,
  AM_ENCODE_MODE_5          = 5,
  AM_ENCODE_MODE_6          = 6,
  AM_ENCODE_MODE_7          = 7,
  AM_ENCODE_MODE_8          = 8,
  AM_ENCODE_MODE_9          = 9,
  AM_ENCODE_MODE_10         = 10,
  AM_ENCODE_MODE_NUM,

  AM_ENCODE_MODE_FIRST = AM_ENCODE_MODE_0,
  AM_ENCODE_MODE_LAST = AM_ENCODE_MODE_10,
  AM_ENCODE_MODE_AUTO
};

enum AM_IMAGE_ISO_TYPE
{
   AM_IMAGE_NORMAL_ISO    = 0,
   AM_IMAGE_ISO_PLUS      = 1,
   AM_IMAGE_ADVANCED_ISO  = 2,
};

enum AM_DEWARP_FUNC_TYPE
{
    AM_DEWARP_NONE  = 0,
    AM_DEWARP_LDC   = 1,
    AM_DEWARP_FULL  = 2,
    AM_DEWARP_EIS   = 3,
};

enum AM_DPTZ_TYPE
{
  AM_DPTZ_DISABLE = 0,
  AM_DPTZ_ENABLE  = 1,
};

enum AM_OVERLAY_TYPE
{
  AM_OVERLAY_PLUGIN_DISABLE = 0,
  AM_OVERLAY_PLUGIN_ENABLE  = 1,
};

enum AM_BITRATE_CTRL_METHOD
{
  AM_BITRATE_CTRL_NONE = 0,
  AM_BITRATE_CTRL_LBR  = 1,
  AM_BITRATE_CTRL_SMARTRC = 2,
};

enum AM_VIDEO_MD_TYPE
{
  AM_MD_PLUGIN_DISABLE = 0,
  AM_MD_PLUGIN_ENABLE  = 1
};

enum AM_HEVC_CLOCK_TYPE
{
  AM_HEVC_CLOCK_UNSUPPORTED = -1,
  AM_HEVC_CLOCK_DISABLE = 0,
  AM_HEVC_CLOCK_ENABLE  = 1
};

enum AM_DATA_FRAME_TYPE
{
  AM_DATA_FRAME_TYPE_VIDEO = 0,
  AM_DATA_FRAME_TYPE_YUV,
  AM_DATA_FRAME_TYPE_RAW,
  AM_DATA_FRAME_TYPE_ME0,
  AM_DATA_FRAME_TYPE_ME1,
  AM_DATA_FRAME_TYPE_VCA,
};

enum AM_VIDEO_FRAME_TYPE
{
  AM_VIDEO_FRAME_TYPE_NONE  = 0x0,
  AM_VIDEO_FRAME_TYPE_IDR   = 0x1,
  AM_VIDEO_FRAME_TYPE_I     = 0x2,   //regular I-frame (non IDR)
  AM_VIDEO_FRAME_TYPE_P     = 0x3,
  AM_VIDEO_FRAME_TYPE_B     = 0x4,
  AM_VIDEO_FRAME_TYPE_MJPEG = 0x9,
  AM_VIDEO_FRAME_TYPE_SJPEG = 0xa,    //still JPEG
};

enum AM_VIDEO_TYPE
{
  AM_VIDEO_NULL  = -1,
  AM_VIDEO_H264  = 0,
  AM_VIDEO_H265  = 1,
  AM_VIDEO_MJPEG = 2,
};

enum AM_DPTZ_CONTROL_METHOD
{
  //default, simple control method.
  AM_DPTZ_CONTROL_NO_CHANGE       = 0,
  //default, simple control method.
  AM_DPTZ_CONTROL_BY_DPTZ_RATIO   = 1,
  //accurate control, but needs app to calculate the ratio
  AM_DPTZ_CONTROL_BY_INPUT_WINDOW = 2,
};

enum AM_EASY_ZOOM_MODE
{
  AM_EASY_ZOOM_MODE_FULL_FOV       = 0,
  AM_EASY_ZOOM_MODE_PIXEL_TO_PIXEL = 1,
  AM_EASY_ZOOM_MODE_AMPLIFY        = 2,
};

enum AM_WARP_PROJECTION_MODE
{
  AM_WARP_PROJRECTION_EQUIDISTANT  = 0,
  AM_WARP_PROJECTION_STEREOGRAPHIC = 1,
  AM_WARP_PROJECTION_LOOKUP_TABLE  = 2
};

enum AM_WARP_MODE
{
  AM_WARP_MODE_NO_TRANSFORM    = 0,
  //"normal" correciton, both H/V lines are corrected to straight
  AM_WARP_MODE_RECTLINEAR      = 1,
  //"V" lines are corrected to be straight, used in wall mount
  AM_WARP_MODE_PANORAMA        = 2,
  //reserved for more advanced mode
  AM_WARP_MODE_SUBREGION       = 3,
  AM_WARP_MODE_MERCATOR        = 4,
  AM_WARP_MODE_CYLINDER_PANOR  = 5,
  AM_WARP_MODE_COPY            = 6,
  AM_WARP_MODE_EQUIRECTANGULAR = 7,
  AM_WARP_MODE_FULLFRAME       = 8,
};

enum AM_CPU_CLK_MODE
{
  AM_CPU_CLK_MODE_LOW    = 0, //low cpu clock
  AM_CPU_CLK_MODE_MID    = 1, //moderation
  AM_CPU_CLK_MODE_HIGH   = 2, //high cpu clock
};

enum AM_POWER_MODE
{
  //full performance with most power consumed
  AM_POWER_MODE_HIGH = 0,
  //medium performance with medium power consumed
  AM_POWER_MODE_MEDIUM,
  //lowest performance with lowest power consumed
  AM_POWER_MODE_LOW,

  AM_POWER_MODE_FIRST = AM_POWER_MODE_HIGH,
  AM_POWER_MODE_LAST = AM_POWER_MODE_LOW,
  AM_POWER_MODE_NUM = AM_POWER_MODE_LAST + 1,
};

struct AM_VIDEO_INFO
{
    uint32_t      stream_id; /* Stream ID */
    uint32_t      fps;       /* Framerate = 512000000 / fps */
    uint32_t      rate;      /* Video rate */
    uint32_t      scale;     /* Video scale, framerate = scale/rate */
    AM_VIDEO_TYPE type;      /* Video type, H.264, H.265 or Mjpeg */
    uint16_t      mul;       /* Video framerate numerator */
    uint16_t      div;       /* Video framerate denominator */
    uint16_t      width;
    uint16_t      height;
    uint16_t      M;
    uint16_t      N;
    uint16_t      slice_num;
    uint16_t      tile_num;
};

struct AMOffset
{
    int32_t x = -1;
    int32_t y = -1;
    AMOffset(){}
    AMOffset(int32_t all) :
      x(all),
      y(all)
    {}
    AMOffset(int32_t xx, int32_t yy) :
      x(xx),
      y(yy)
    {}
    AMOffset(const AMOffset &a) :
      x(a.x),
      y(a.y)
    {}
};

struct AMResolution
{
    int32_t width = -1;
    int32_t height = -1;
    AMResolution(){}
    AMResolution(int32_t all) :
      width(all),
      height(all)
    {}
    AMResolution(int32_t w, int32_t h) :
      width(w),
      height(h)
    {}
    AMResolution(const AMResolution &res) :
      width(res.width),
      height(res.height)
    {}
};

struct AMRect
{
    AMOffset offset; //start offset (left, top)
    AMResolution size;
    AMRect(){}
    AMRect(int32_t all) :
      offset(all),
      size(all)
    {}
    AMRect(const AMRect& rect) :
      offset(rect.offset),
      size(rect.size)
    {}
};

struct AMAddress
{
    uint8_t *data;
    uint32_t max_size;
};

struct AMVinInfo
{
    uint32_t vin_id;
    uint32_t width;
    uint32_t height;
    uint16_t fps;
    uint8_t format;
    uint8_t type;
    uint8_t bits;
    uint8_t ratio;
    uint8_t system;
    uint8_t hdr_mode;
};

struct AMStreamInfo
{
    AM_STREAM_ID stream_id;
    uint32_t m;
    uint32_t n;
    uint32_t mul;
    uint32_t div;
    uint32_t rate;
    uint32_t scale;
};

struct AMVideoTileSlice
{
    uint8_t slice_num : 4;
    uint8_t slice_id  : 4;
    uint8_t tile_num  : 4;
    uint8_t tile_id   : 4;
};

struct AMVideoFrameDesc
{
    uint32_t            stream_id;
    AM_VIDEO_FRAME_TYPE type;
    AM_STREAM_TYPE      stream_type;
    uint32_t            width;
    uint32_t            height;
    uint32_t            frame_num;
    uint32_t            data_offset;
    uint32_t            data_size;
    uint32_t            session_id;
    AMVideoTileSlice    tile_slice;
    uint8_t             jpeg_quality;      //1 ~ 99
    uint8_t             stream_end_flag;   //0 or 1
};

struct AMYUVFrameDesc
{
    AM_SOURCE_BUFFER_ID buffer_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t y_offset;
    uint32_t uv_offset;    //NV12 format, (UV interleaved)
    uint32_t seq_num;
    AM_CHROMA_FORMAT format;
    uint32_t non_block_flag;
};

struct AMMEFrameDesc
{
    AM_SOURCE_BUFFER_ID buffer_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t data_offset;
    uint32_t seq_num;
    uint32_t non_block_flag;
};

struct AMRawFrameDesc
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t data_offset;
    uint32_t non_block_flag;
};

struct AMQueryFrameDesc
{
    int64_t             pts = 0;
    AM_DATA_FRAME_TYPE  type = AM_DATA_FRAME_TYPE_VIDEO;
    union
    {
        AMVideoFrameDesc    video;
        AMYUVFrameDesc      yuv;
        AMYUVFrameDesc      vca;
        AMMEFrameDesc       me;
        AMRawFrameDesc      raw;
    };
};

struct AMFrac
{
    int32_t num = 0;
    int32_t denom = 0;
};

struct AMPoint
{
    int32_t x = 0;
    int32_t y = 0;

    AMPoint(){}
    AMPoint(int32_t px,
            int32_t py) :
      x(px),
      y(py)
    {}
};

struct AMPointF
{
  float x = 0.0;
  float y = 0.0;
};

struct AMRectInMain
{
    int32_t width = 0;
    int32_t height = 0;
    AMPoint upper_left;
};

typedef int16_t AMData;
struct AMVectorMap
{
    AMData *addr = nullptr;
    int32_t cols = 0;
    int32_t rows = 0;
    int32_t grid_width = 0;
    int32_t grid_height = 0;
};

struct AMOverlayAreaInsert
{
    uint8_t *clut_addr;
    uint8_t *data_addr;
    uint32_t clut_addr_offset;
    uint32_t data_addr_offset;
    uint32_t rotate_mode;
    uint32_t total_size;
    uint32_t area_size_max;
    uint16_t enable;
    uint16_t width;
    uint16_t height;
    uint16_t start_x;
    uint16_t start_y;
    uint16_t pitch;
};

struct AMOverlayInsert
{
    AM_STREAM_ID        stream_id;
    uint32_t            enable;
    std::vector<AMOverlayAreaInsert> area;
};

struct AMFeatureParam
{
    pair<bool, int32_t> version = {false, -1};
    pair<bool, int32_t> cpu_clk = {false, -1};
    pair<bool, AM_ENCODE_MODE> mode = {false, AM_ENCODE_INVALID_MODE};
    pair<bool, AM_HDR_TYPE> hdr = {false, AM_HDR_SINGLE_EXPOSURE};
    pair<bool, AM_IMAGE_ISO_TYPE> iso = {false, AM_IMAGE_NORMAL_ISO};
    pair<bool, AM_DEWARP_FUNC_TYPE> dewarp_func = {false, AM_DEWARP_NONE};
    pair<bool, AM_DPTZ_TYPE> dptz = {false, AM_DPTZ_ENABLE};
    pair<bool, AM_BITRATE_CTRL_METHOD> bitrate_ctrl =
                                        {false, AM_BITRATE_CTRL_NONE};
    pair<bool, AM_OVERLAY_TYPE> overlay = {false, AM_OVERLAY_PLUGIN_ENABLE};
    pair<bool, AM_VIDEO_MD_TYPE> video_md = {false, AM_MD_PLUGIN_ENABLE};
    pair<bool, AM_HEVC_CLOCK_TYPE> hevc = {false, AM_HEVC_CLOCK_UNSUPPORTED};

    AMFeatureParam(){}
    AMFeatureParam(pair<bool, int32_t> ver,
                   pair<bool, int32_t> clk,
                   pair<bool, AM_ENCODE_MODE> m,
                   pair<bool, AM_HDR_TYPE> h,
                   pair<bool, AM_IMAGE_ISO_TYPE> is,
                   pair<bool, AM_DEWARP_FUNC_TYPE> dewarp,
                   pair<bool, AM_DPTZ_TYPE> dz,
                   pair<bool, AM_BITRATE_CTRL_METHOD> bc,
                   pair<bool, AM_OVERLAY_TYPE> ol,
                   pair<bool, AM_VIDEO_MD_TYPE> vm,
                   pair<bool, AM_HEVC_CLOCK_TYPE> hv) :
      version(ver),
      cpu_clk(clk),
      mode(m),
      hdr(h),
      iso(is),
      dewarp_func(dewarp),
      dptz(dz),
      bitrate_ctrl(bc),
      overlay(ol),
      video_md(vm),
      hevc(hv)
    {}
};

struct AMBitrate
{
    AM_STREAM_ID stream_id = AM_STREAM_ID_INVALID;
    AM_RATE_CONTROL rate_control_mode = AM_RC_NONE;
    int32_t target_bitrate = -1;
    int32_t vbr_min_bitrate = -1;
    int32_t vbr_max_bitrate = -1;
    int32_t i_frame_max_size = -1;
};

struct AMFramerate
{
    AM_STREAM_ID stream_id = AM_STREAM_ID_INVALID;
    int32_t fps = 0;
};

struct AMMJpegInfo
{
    AM_STREAM_ID stream_id = AM_STREAM_ID_INVALID;
    uint32_t quality;
};

struct AMGOP
{
    AM_STREAM_ID stream_id;
    uint32_t N = 0;
    uint32_t idr_interval = 0;
};

#endif /* AM_VIDEO_TYPES_H_ */
