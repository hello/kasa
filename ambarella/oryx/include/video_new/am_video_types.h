/*******************************************************************************
 * am_video_types.h
 *
 * Histroy:
 *  2012-2-20 2012 - [ypchang] created file
 *  2014-6-24      - [Louis ] modified and created am_video_types.h, removed non video stuffs
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_NEW_TYPES_H_
#define AM_VIDEO_NEW_TYPES_H_

#include "am_base_include.h"

enum AM_RESULT
{
  AM_RESULT_OK            = 0,      //OK is same as NO_ERROR
  AM_RESULT_ERR_INVALID   = -1,     //invalid argument
  AM_RESULT_ERR_PERM      = -2,     //operation is not permitted by state
  AM_RESULT_ERR_BUSY      = -3,     //too busy to handle it
  AM_RESULT_ERR_AGAIN     = -4,     //not ready now, but please try again
  AM_RESULT_ERR_NO_ACCESS = -5,     //not allowed to access this by access right
  AM_RESULT_ERR_DSP       = -6,     //some DSP related error happened
  AM_RESULT_ERR_MEM       = -7,     //some DRAM related error happened
  AM_RESULT_ERR_IO        = -8,     //some I/O related error happened
  AM_RESULT_ERR_UNKNOWN   = -9,     //some unknown error happened
};

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
  AM_VIN_MODE_3M_4_3,     //2048x1536
  AM_VIN_MODE_3M_16_9,    //2304x1296
  AM_VIN_MODE_QHD,        //2560x1440
  AM_VIN_MODE_4M_16_9,    //2688x1512
  AM_VIN_MODE_5M_4_3,     //2592x1944
  AM_VIN_MODE_6M_3_2,     //3072x2048
  AM_VIN_MODE_QFHD,       //3840x2160
  AM_VIN_MODE_4096X2160,  //4096x2160
  AM_VIN_MODE_12M,        //4000x3000
  AM_VIN_MODE_14M,        //4000x3000
  AM_VIN_MODE_16M,        //4000x3000
  AM_VIN_MODE_18M,        //4000x3000

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
  AM_HDR_SENSOR_INTERNAL  = 4, //!< Sensor built-in WDR
  AM_HDR_TYPE_NUM
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

enum AM_H264_GOP_MODEL
{
  AM_H264_GOP_MODEL_SIMPLE = 0, //simpe GOP
  AM_H264_GOP_MODEL_ADVANCED = 1, //hierachical
  AM_H264_GOP_MODEL_SVCT_FIRST = 2, //please don't change this value
  AM_H264_GOP_MODEL_SVCT_LAST = 5, //please don't change this value
};

enum AM_H264_RATE_CONTROL
{
  AM_H264_RC_CBR = 0, //SCBR version
  AM_H264_RC_VBR = 1, //SCBR version
  AM_H264_RC_CBR_QUALITY = 2, //SCBR version
  AM_H264_RC_VBR_QUALITY = 3, //SCBR version
  AM_H264_RC_CBR2 = 4, //old style CBR
  AM_H264_RC_VBR2 = 5, //old style VBR
  AM_H264_RC_LBR = 6, //CBR + ZMV
};

enum AM_H264_PROFILE
{
  AM_H264_PROFILE_BASELINE = 0,
  AM_H264_PROFILE_MAIN = 1,
  AM_H264_PROFILE_HIGH = 2,
};

enum AM_H264_AU_TYPE
{
  AM_H264_AU_TYPE_NO_AUD_NO_SEI = 0,
  AM_H264_AU_TYPE_AUD_BEFORE_SPS_WITH_SEI = 1,
  AM_H264_AU_TYPE_AUD_AFTER_SPS_WITH_SEI = 2,
  AM_H264_AU_TYPE_NO_AUD_WITH_SEI = 3,
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

enum AM_DEWARP_TYPE
{
    AM_DEWARP_NONE  = 0,
    AM_DEWARP_LDC   = 1,
    AM_DEWARP_FULL  = 2,
};

enum AM_DATA_FRAME_TYPE
{
  AM_DATA_FRAME_TYPE_VIDEO = 0,
  AM_DATA_FRAME_TYPE_YUV,
  AM_DATA_FRAME_TYPE_RAW,
  AM_DATA_FRAME_TYPE_ME1,
  AM_DATA_FRAME_TYPE_VCA,
};

enum AM_VIDEO_FRAME_TYPE
{
  AM_VIDEO_FRAME_TYPE_NONE     = 0x0,
  AM_VIDEO_FRAME_TYPE_H264_IDR = 0x1,
  AM_VIDEO_FRAME_TYPE_H264_I   = 0x2,   //regular I-frame (non IDR)
  AM_VIDEO_FRAME_TYPE_H264_P   = 0x3,
  AM_VIDEO_FRAME_TYPE_H264_B   = 0x4,
  AM_VIDEO_FRAME_TYPE_MJPEG    = 0x9,
  AM_VIDEO_FRAME_TYPE_SJPEG    = 0xa,    //still JPEG
};

enum AM_VIDEO_TYPE
{
  AM_VIDEO_NULL  = -1,
  AM_VIDEO_H264  = 0,
  AM_VIDEO_H265  = 1,
  AM_VIDEO_MJPEG = 2,
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
};

struct AMOffset
{
    int32_t x;
    int32_t y;
};

struct AMResolution
{
    int32_t width;
    int32_t height;
};

struct AMRect
{
    AMOffset offset; //start offset (left, top)
    AMResolution size;
};

struct AMFps
{
    uint32_t mul;
    uint32_t div;
};

struct AMAddress
{
    uint8_t *data;
    uint32_t max_size;
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

struct AMVideoFrameDesc
{
    uint32_t stream_id;
    AM_VIDEO_FRAME_TYPE type;
    uint32_t width;
    uint32_t height;
    uint32_t frame_num;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t session_id;
    uint8_t  jpeg_quality;      //1 ~ 99
    uint8_t  stream_end_flag;   //0 or 1
    uint8_t  reserved[2];
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
    AM_DATA_FRAME_TYPE  type;
    int64_t             pts;
    union
    {
        AMVideoFrameDesc    video;
        AMYUVFrameDesc      yuv;
        AMYUVFrameDesc      vca;
        AMMEFrameDesc       me;
        AMRawFrameDesc      raw;
    };

    AMQueryFrameDesc():
      type(AM_DATA_FRAME_TYPE_VIDEO),
      pts(0)
    {
    }
};

#endif /* AM_VIDEO_TYPES_H_ */
