/**
 * am_encode_config_param.h
 *
 *  History:
 *    Jul 28, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_INCLUDE_VIDEO_NEW_AM_ENCODE_CONFIG_PARAM_H_
#define ORYX_INCLUDE_VIDEO_NEW_AM_ENCODE_CONFIG_PARAM_H_

#include <map>
#include <mutex>
#include <atomic>
#include <vector>
#include "am_video_types.h"
#include "am_pointer.h"

using std::map;
using std::pair;
using std::vector;
using std::string;
using std::atomic_int;
using std::recursive_mutex;

//For VIN
struct AMVinParam
{
    pair<bool, AM_SENSOR_TYPE>        type;
    pair<bool, AM_VIN_MODE>           mode;
    pair<bool, AM_VIDEO_FLIP>         flip;
    pair<bool, AM_VIDEO_FPS>          fps;
    pair<bool, AM_VIN_BAYER_PATTERN>  bayer_pattern;

    AMVinParam():
      type(false, AM_SENSOR_TYPE_NONE),
      mode(false, AM_VIN_MODE_AUTO),
      flip(false, AM_VIDEO_FLIP_NONE),
      fps(false, AM_VIDEO_FPS_AUTO),
      bayer_pattern(false, AM_VIN_BAYER_PATTERN_AUTO)
    {}
};
typedef map<AM_VIN_ID, AMVinParam> AMVinParamMap;

//For BUFFER
struct AMBufferConfigParam
{
    pair<bool, AM_SOURCE_BUFFER_TYPE>     type;
    pair<bool, AMRect>                    input;
    pair<bool, AMResolution>              size;
    pair<bool, bool>                      input_crop;
    pair<bool, bool>                      prewarp;

    AMBufferConfigParam():
      type(false, AM_SOURCE_BUFFER_TYPE_OFF),
      input_crop(false, false),
      prewarp(false, false)
    {
      input.first = false;
      input.second = {0};
      size.first = false;
      size.second = {0};
    }
};
typedef map<AM_SOURCE_BUFFER_ID, AMBufferConfigParam> AMBufferParamMap;

//For STREAM
struct AMStreamFormatConfig
{
    pair<bool, bool>                    enable;
    pair<bool, AM_STREAM_TYPE>          type;
    pair<bool, AM_SOURCE_BUFFER_ID>     source;
    pair<bool, AMFps>                   fps;
    pair<bool, AMRect>                  enc_win;
    pair<bool, AM_VIDEO_FLIP>           flip;
    pair<bool, bool>                    rotate_90_ccw;

    AMStreamFormatConfig():
      enable(false, false),
      type(false, AM_STREAM_TYPE_NONE),
      source(false, AM_SOURCE_BUFFER_INVALID),
      flip(false, AM_VIDEO_FLIP_NONE),
      rotate_90_ccw(false, false)
    {
      fps.first = true;
      fps.second = {0};
      enc_win.first = false;
      enc_win.second = {0};
    }
};

struct AMStreamH264Config
{
    //For H.264
    pair<bool, AM_H264_GOP_MODEL>      gop_model;
    pair<bool, AM_H264_RATE_CONTROL>   bitrate_control;
    pair<bool, AM_H264_PROFILE>        profile_level;
    pair<bool, AM_H264_AU_TYPE>        au_type;
    pair<bool, AM_CHROMA_FORMAT>       chroma_format;
    pair<bool, uint32_t>               M;
    pair<bool, uint32_t>               N;
    pair<bool, uint32_t>               idr_interval;
    pair<bool, uint32_t>               target_bitrate;
    pair<bool, uint32_t>               mv_threshold;
    pair<bool, uint32_t>               long_term_intvl;
    pair<bool, bool>                   encode_improve;
    pair<bool, bool>                   multi_ref_p;

    AMStreamH264Config():
      gop_model(false, AM_H264_GOP_MODEL_SIMPLE),
      bitrate_control(false, AM_H264_RC_CBR),
      profile_level(false, AM_H264_PROFILE_BASELINE),
      au_type(false, AM_H264_AU_TYPE_NO_AUD_NO_SEI),
      chroma_format(false, AM_CHROMA_FORMAT_YUV420),
      M(false, 0),
      N(false, 0),
      idr_interval(false, 0),
      target_bitrate(false, 0),
      mv_threshold(false, 0),
      long_term_intvl(false, 0),
      encode_improve(false, false),
      multi_ref_p(false, false)
    {}
};

struct AMStreamMJPEGConfig
{
    pair<bool, AM_CHROMA_FORMAT>  chroma_format;
    pair<bool, uint32_t>          quality;

    AMStreamMJPEGConfig():
      chroma_format(false, AM_CHROMA_FORMAT_YUV420),
      quality(false, 0)
    {}
};

struct AMStreamConfigParam
{
    pair<bool, AMStreamFormatConfig>  stream_format;
    pair<bool, AMStreamH264Config>    h264_config;
    pair<bool, AMStreamMJPEGConfig>   mjpeg_config;

    AMStreamConfigParam()
    {
      stream_format.first = false;
      h264_config.first = false;
      mjpeg_config.first = false;
    }
};
typedef map<AM_STREAM_ID, AMStreamConfigParam> AMStreamParamMap;

#endif /* ORYX_INCLUDE_VIDEO_NEW_AM_ENCODE_CONFIG_PARAM_H_ */
