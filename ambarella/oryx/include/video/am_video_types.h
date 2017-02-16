/*******************************************************************************
 * am_video_types.h
 *
 * Histroy:
 *  2012-2-20 2012 - [ypchang] created file
 *  2014-6-24         - [Louis ] modified and created am_video_types.h, removed non video stuffs
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_TYPES_H_
#define AM_VIDEO_TYPES_H_

#include "am_base_include.h"

/*
 * some functions may need AM_RESULT type of return value,
 * but not simple bool type,
 * for example, if some operation not allowed, it cannot run but the
 * "failure" is actually expected behavior, in this case, the function should
 * return "not allowed" instead of error
 */

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

/*
 * Encode related
 */
enum AM_ENCODE_CHROMA_FORMAT
{
  AM_ENCODE_CHROMA_FORMAT_YUV420 = 0,
  AM_ENCODE_CHROMA_FORMAT_YUV422 = 1,
  AM_ENCODE_CHROMA_FORMAT_Y8 = 2, //gray scale (or monochrome)
};

enum AM_ENCODE_SOURCE_BUFFER_ID
{
  AM_ENCODE_SOURCE_BUFFER_INVALID = -1, //for init
  AM_ENCODE_SOURCE_BUFFER_MAIN = 0,
  AM_ENCODE_SOURCE_BUFFER_2ND = 1,
  AM_ENCODE_SOURCE_BUFFER_3RD = 2,
  AM_ENCODE_SOURCE_BUFFER_4TH = 3,

  AM_ENCODE_SOURCE_BUFFER_PREVIEW_A = AM_ENCODE_SOURCE_BUFFER_4TH,
  AM_ENCODE_SOURCE_BUFFER_PREVIEW_B = AM_ENCODE_SOURCE_BUFFER_3RD,
  AM_ENCODE_SOURCE_BUFFER_PREVIEW_C = AM_ENCODE_SOURCE_BUFFER_2ND,

  AM_ENCODE_SOURCE_BUFFER_FIRST = AM_ENCODE_SOURCE_BUFFER_MAIN,
  AM_ENCODE_SOURCE_BUFFER_LAST = AM_ENCODE_SOURCE_BUFFER_4TH,

  AM_ENCODE_SOURCE_BUFFER_MAX_NUM = (AM_ENCODE_SOURCE_BUFFER_LAST + 1),
};

enum AM_ENCODE_MODE
{
  AM_VIDEO_ENCODE_INVALID_MODE = -1, //mode -1, invalid for init
  AM_VIDEO_ENCODE_NORMAL_MODE = 0, //mode 0, simplest mode
  AM_VIDEO_ENCODE_DEWARP_MODE = 1, //mode 1, multi-dewarp
  AM_VIDEO_ENCODE_RESERVED1_MODE = 2, //mode 2, not used
  //AM_VIDEO_ENCODE_LDC_MODE = 3, //mode 3, single-dewarp
  AM_VIDEO_ENCODE_ADV_ISO_MODE = 4, //mode 4, advanced ISO
  AM_VIDEO_ENCODE_ADV_HDR_MODE = 5, //mode 5, advanced HDR
  AM_VIDEO_ENCODE_MODE_NUM = 6,   //from 0~5, all 6 modes, includine 1 reserved

  AM_VIDEO_ENCODE_MODE_FIRST = AM_VIDEO_ENCODE_NORMAL_MODE,
  AM_VIDEO_ENCODE_MODE_LAST  = AM_VIDEO_ENCODE_ADV_HDR_MODE,
};

enum AM_STREAM_ID
{
  AM_STREAM_ID_INVALID = -1,  //for initial invalid value
  AM_STREAM_ID_0 = 0,
  AM_STREAM_ID_1,
  AM_STREAM_ID_2,
  AM_STREAM_ID_3,
  AM_STREAM_ID_LAST = AM_STREAM_ID_3,

  AM_STREAM_MAX_NUM = (AM_STREAM_ID_LAST + 1),
};

/*
 * VIN related
 */
enum AM_VIN_ID
{
  AM_VIN_ID_INVALID = -1,    /* invalid */
  AM_VIN_0 = 0,      /* Main VIN */
  AM_VIN_1 = 1,     /* Second VIN, usually NULL */
  AM_VIN_MAX_NUM = 2
};

enum AM_VIN_MODE
{
  AM_VIN_MODE_AUTO = 0, // Auto Adaptive to the sensor
  AM_VIN_MODE_VGA = 3, // 640x480
  AM_VIN_MODE_720P = 10, //1280x720
  AM_VIN_MODE_960P = 11, //1280x960
  AM_VIN_MODE_1024P = 12, //1280x1024
  AM_VIN_MODE_1080P = 20, //1920x1080
  AM_VIN_MODE_3M_4_3 = 30, //2048x1536
  AM_VIN_MODE_3M_16_9 = 31, //2304x1296
  AM_VIN_MODE_QHD = 32, //2560x1440
  AM_VIN_MODE_4M_16_9 = 40, //2688x1512
  AM_VIN_MODE_5M_4_3 = 50, //2592x1944
  AM_VIN_MODE_6M_3_2 = 60, //3072x2048
  AM_VIN_MODE_QFHD = 80, //3840x2160
  AM_VIN_MODE_4096X2160 = 81, //4096x2160
  AM_VIN_MODE_12M = 120, //4000x3000
  AM_VIN_MODE_14M = 140, //4000x3000
  AM_VIN_MODE_16M = 160, //4000x3000
  AM_VIN_MODE_18M = 180, //4000x3000
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

/*! @enum AM_HDR_EXPOSURE_TYPE
 *  Defines sensor HDR type
 */
enum AM_HDR_EXPOSURE_TYPE
{
  AM_HDR_SINGLE_EXPOSURE = 0, //!< NO HDR, which is for non HDR scene (linear)
  AM_HDR_2_EXPOSURE = 1,      //!< HDR 2x
  AM_HDR_3_EXPOSURE = 2,      //!< HDR 3x
  AM_HDR_4_EXPOSURE = 3,      //!< HDR 4x
  AM_HDR_SENSOR_INTERNAL = 4, //!< Sensor built-in WDR
};

enum AM_VIDEO_FLIP
{
  AM_VIDEO_FLIP_NONE = 0,
  AM_VIDEO_FLIP_VERTICAL = 1,
  AM_VIDEO_FLIP_HORIZONTAL = 2,
  AM_VIDEO_FLIP_VH_BOTH = 3, //flip for both vertical and horizontal
  AM_VIDEO_FLIP_AUTO  = 255   //auto flip , decided by device driver
};

enum AM_VIN_BAYER_PATTERN
{
  AM_VIN_BAYER_PATTERN_AUTO = 0,
  AM_VIN_BAYER_PATTERN_RG = 1,
  AM_VIN_BAYER_PATTERN_BG = 2,
  AM_VIN_BAYER_PATTERN_GR = 3,
  AM_VIN_BAYER_PATTERN_GB = 4,
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

enum AM_DATA_FRAME_TYPE
{
  AM_DATA_FRAME_TYPE_VIDEO = 0,
  AM_DATA_FRAME_TYPE_YUV = 1,
  AM_DATA_FRAME_TYPE_LUMA = 2,    //ME1
  AM_DATA_FRAME_TYPE_BAYER_PATTERN_RAW = 3,
  AM_DATA_FRAME_TYPE_GENERIC_DATA = 4,  //reserved for just data dump
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

struct AMResolution
{
    uint32_t width;
    uint32_t height;
};

struct AMRect
{
    uint32_t width;
    uint32_t height;
    int      x;      //start offset (left, top)  X
    int      y;      //start offset (left, top)  Y
};

struct AMVideoVersion
{
    uint8_t major;
    uint8_t minor;
    uint8_t trivial;
    uint8_t reserved;
};

struct AMStreamFps
{
    uint32_t fps_multi;
    uint32_t fps_div;
};

struct AMOffset
{
    uint32_t x;
    uint32_t y;
};

struct AMGOP
{
    uint32_t N;
    uint32_t idr_interval;
};

struct AMScaleFactor
{
    uint32_t numerator;
    uint32_t denominator;
};

struct AMEncodeDSPCapability
{
    bool basic_hdr;
    bool advanced_hdr;
    bool normal_iso;
    bool normal_plus_iso;
    bool advanced_iso;
    bool single_dewarp;
    bool multi_dewarp;
};

struct AMMemMapInfo
{
    uint8_t *addr;
    uint32_t length;
};


struct AMVinAGC {
    uint32_t agc;
    uint32_t agc_max;
    uint32_t agc_min;
    uint32_t agc_step;
    uint32_t wdr_again_idx_min;
    uint32_t wdr_again_idx_max;
    uint32_t wdr_dgain_idx_min;
    uint32_t wdr_dgain_idx_max;
};
#endif /* AM_VIDEO_TYPES_H_ */
