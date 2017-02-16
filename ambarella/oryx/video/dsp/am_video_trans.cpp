/*******************************************************************************
 * am_video_trans.cpp
 *
 * Histroy:
 *   Sep 1, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_video_param.h"
#include "am_video_trans.h"
#include "iav_ioctl.h"

static AMVideoFPSTable gVideoFpsList[] =
{
 {  "auto", 0, AM_VIDEO_FPS_AUTO},
 {     "0", 0, AM_VIDEO_FPS_AUTO},
 {     "1", 0, AM_VIDEO_FPS_1}   ,
 {     "2", 0, AM_VIDEO_FPS_2}   ,
 {     "3", 0, AM_VIDEO_FPS_3}   ,
 {     "4", 0, AM_VIDEO_FPS_4}   ,
 {     "5", 0, AM_VIDEO_FPS_5}   ,
 {     "6", 0, AM_VIDEO_FPS_6}   ,
 {    "10", 0, AM_VIDEO_FPS_10}  ,
 {    "12", 0, AM_VIDEO_FPS_12}  ,
 {    "13", 0, AM_VIDEO_FPS_13}  ,
 {    "15", 0, AM_VIDEO_FPS_15}  ,
 {    "20", 0, AM_VIDEO_FPS_20}  ,
 {    "24", 0, AM_VIDEO_FPS_24}  ,
 {    "25", 0, AM_VIDEO_FPS_25}  ,
 {    "30", 0, AM_VIDEO_FPS_30}  ,
 {    "50", 0, AM_VIDEO_FPS_50}  ,
 {    "60", 0, AM_VIDEO_FPS_60}  ,
 {   "120", 0, AM_VIDEO_FPS_120} ,
 { "29.97", 0, AM_VIDEO_FPS_29_97},
 { "59.94", 0, AM_VIDEO_FPS_59_94},
 {"23.976", 0, AM_VIDEO_FPS_23_976},
 {  "12.5", 0, AM_VIDEO_FPS_12_5},
 {  "6.25", 0, AM_VIDEO_FPS_6_25},
 { "3.125", 0, AM_VIDEO_FPS_3_125},
 {   "7.5", 0, AM_VIDEO_FPS_7_5},
 {  "3.75", 0, AM_VIDEO_FPS_3_75}
};

AM_VIDEO_FPS video_fps_str_to_enum(const char *fps_str)
{
  AM_VIDEO_FPS fps = AM_VIDEO_FPS_AUTO;
  uint32_t i;
  if (fps_str) {
    for (i = 0; i < sizeof(gVideoFpsList)/sizeof(gVideoFpsList[0]); i++){
      if (is_str_equal(fps_str, gVideoFpsList[i].name)) {
        fps = gVideoFpsList[i].video_fps;
        break;
      }
    }
  }
  return fps;
}

std::string video_flip_enum_to_str(AM_VIDEO_FLIP flip)
{
  std::string ret;
  switch(flip)
  {
    case AM_VIDEO_FLIP_NONE:
      ret = "none";
      break;
    case AM_VIDEO_FLIP_VERTICAL:
      ret = "v-flip";
      break;
    case AM_VIDEO_FLIP_HORIZONTAL:
      ret = "h-flip";
      break;
    case AM_VIDEO_FLIP_VH_BOTH:
      ret = "both-flip";
      break;
    case AM_VIDEO_FLIP_AUTO:
    default:
      ret = "auto";
      break;
  }
  return ret;
}

std::string video_fps_enum_to_str(AM_VIDEO_FPS  fps)
{
  uint32_t i;
  std::string ret = "auto";
  for (i = 0; i < sizeof(gVideoFpsList)/sizeof(gVideoFpsList[0]); i++){
    if (gVideoFpsList[i].video_fps == fps) {
      ret = gVideoFpsList[i].name;
      break;
    }
  }
  return ret;
}

AM_VIDEO_FLIP video_flip_str_to_enum(const char *flip_str)
{
  AM_VIDEO_FLIP type = AM_VIDEO_FLIP_NONE;
  if (flip_str) {
    if (is_str_equal(flip_str, "h-flip")) {
      type = AM_VIDEO_FLIP_HORIZONTAL;
    } else if (is_str_equal(flip_str, "v-flip")) {
      type = AM_VIDEO_FLIP_VERTICAL;
    } else if (is_str_equal(flip_str, "both-flip")) {
      type = AM_VIDEO_FLIP_VH_BOTH;
    } else {
      type = AM_VIDEO_FLIP_NONE;
    }
  }
  return type;
}

AM_VIDEO_ROTATE video_rotate_str_to_enum(const char *rotate_str)
{
  AM_VIDEO_ROTATE type = AM_VIDEO_ROTATE_NONE;
  if (rotate_str) {
    if (is_str_equal(rotate_str, "rotate-90")) {
      type = AM_VIDEO_ROTATE_90;
    }
  }
  return type;
}

AMStreamFps stream_frame_factor_str_to_fps(const char *frame_factor_str)
{
  AMStreamFps fps = {1, 1};
  int numerator, denominator;
  char tmp[256];
  char *slash;
  do {
    if ((frame_factor_str) && (frame_factor_str[0] != '\0')) {
      strcpy(tmp, frame_factor_str);
      slash = strchr(tmp, '/');
      if (!slash) {
        ERROR("frame_factor_str %s is wrong\n", tmp);
        break;
      }
      denominator = atoi(slash + 1);
      if (denominator == 0) {
        ERROR("frame_factor_str %s is wrong\n", tmp);
        break;
      }
      slash[0] = '\0';
      numerator = atoi(tmp);
      fps.fps_multi = numerator;
      fps.fps_div = denominator;
    }
  } while (0);
  return fps;
}

std::string stream_format_enum_to_str(AM_ENCODE_STREAM_TYPE type)
{
  std::string ret;
  switch (type) {
    case AM_ENCODE_STREAM_TYPE_H264:
      ret = "h264";
      break;
    case AM_ENCODE_STREAM_TYPE_MJPEG:
      ret = "mjpeg";
      break;
    case AM_ENCODE_STREAM_TYPE_NONE:
    default:
      ret = "none";
      break;
  }

  return ret;
}

std::string stream_fps_to_str(AMStreamFps *fps)
{
  std::string ret;
  ret = std::to_string(fps->fps_multi) + std::string ("/") +
      std::to_string(fps->fps_div);
  return ret;
}

AM_ENCODE_H264_RATE_CONTROL stream_bitrate_control_str_to_enum(const char *bc_str)
{
  AM_ENCODE_H264_RATE_CONTROL ret = AM_ENCODE_H264_RC_CBR;

  if (bc_str) {
    if (is_str_equal(bc_str, "cbr")) {
      ret = AM_ENCODE_H264_RC_CBR;
    } else if (is_str_equal(bc_str, "vbr")) {
      ret = AM_ENCODE_H264_RC_VBR;
    } else if (is_str_equal(bc_str, "cbr2")) {
      ret = AM_ENCODE_H264_RC_CBR2;
    } else if (is_str_equal(bc_str, "vbr2")) {
      ret = AM_ENCODE_H264_RC_VBR2;
    } else if (is_str_equal(bc_str, "lbr")) {
      ret = AM_ENCODE_H264_RC_LBR;
    } else if (is_str_equal(bc_str, "cbr_quality")) {
      ret = AM_ENCODE_H264_RC_CBR_QUALITY;
    } else if (is_str_equal(bc_str, "vbr_quality")) {
      ret = AM_ENCODE_H264_RC_VBR_QUALITY;
    } else {
      ret = AM_ENCODE_H264_RC_CBR;
    }
  }
  return ret;
}

AM_ENCODE_H264_PROFILE stream_profile_str_to_enum(const char *profile_str)
{
  AM_ENCODE_H264_PROFILE ret = AM_ENCODE_H264_PROFILE_MAIN;
  if (profile_str) {
    if (is_str_equal(profile_str, "main")) {
      ret = AM_ENCODE_H264_PROFILE_MAIN;
    } else if (is_str_equal(profile_str, "baseline")) {
      ret = AM_ENCODE_H264_PROFILE_BASELINE;
    } else if (is_str_equal(profile_str, "high")) {
      ret = AM_ENCODE_H264_PROFILE_HIGH;
    } else {
      ret = AM_ENCODE_H264_PROFILE_MAIN;
    }
  }
  return ret;
}

AM_ENCODE_CHROMA_FORMAT stream_chroma_str_to_enum(const char *chroma_str)
{
  AM_ENCODE_CHROMA_FORMAT ret = AM_ENCODE_CHROMA_FORMAT_YUV420;
  if (chroma_str) {
    if (is_str_equal(chroma_str, "420")) {
      ret = AM_ENCODE_CHROMA_FORMAT_YUV420;
    } else if (is_str_equal(chroma_str, "422")) {
      ret = AM_ENCODE_CHROMA_FORMAT_YUV422;
    } else if (is_str_equal(chroma_str, "mono")) {
      ret = AM_ENCODE_CHROMA_FORMAT_Y8;
    } else {
      ret = AM_ENCODE_CHROMA_FORMAT_YUV420;
    }
  }
  return ret;
}

std::string stream_bitrate_control_enum_to_str(AM_ENCODE_H264_RATE_CONTROL rate_control)
{
  std::string ret;
  switch (rate_control) {
    case AM_ENCODE_H264_RC_VBR:
      ret = "vbr";
      break;
    case AM_ENCODE_H264_RC_CBR_QUALITY:
      ret = "cbr_quality";
      break;
    case AM_ENCODE_H264_RC_VBR_QUALITY:
      ret = "vbr_quality";
      break;
    case AM_ENCODE_H264_RC_CBR2:
      ret = "cbr2";
      break;
    case AM_ENCODE_H264_RC_VBR2:
      ret = "vbr2";
      break;
    case AM_ENCODE_H264_RC_LBR:
      ret = "lbr";
      break;
    case AM_ENCODE_H264_RC_CBR:
    default:
      ret = "cbr";
      break;
  }
  return ret;
}

std::string stream_profile_level_enum_to_str(AM_ENCODE_H264_PROFILE profile_level)
{
  std::string ret;
  switch (profile_level) {
    case AM_ENCODE_H264_PROFILE_BASELINE:
      ret = "baseline";
      break;
    case AM_ENCODE_H264_PROFILE_HIGH:
      ret = "high";
      break;
    case AM_ENCODE_H264_PROFILE_MAIN:
    default:
      ret = "main";
      break;
  }
  return ret;
}

std::string stream_chroma_to_str(AM_ENCODE_CHROMA_FORMAT chroma_format)
{
  std::string ret;
  switch (chroma_format) {
    case AM_ENCODE_CHROMA_FORMAT_YUV422:
      ret = "422";
      break;
    case AM_ENCODE_CHROMA_FORMAT_Y8:
      ret = "mono";
      break;
    case AM_ENCODE_CHROMA_FORMAT_YUV420:
    default:
      ret = "420";
      break;
  }
  return ret;
}

AM_ENCODE_SOURCE_BUFFER_TYPE buffer_type_str_to_enum(const char *type_str)
{
  AM_ENCODE_SOURCE_BUFFER_TYPE buf_type = AM_ENCODE_SOURCE_BUFFER_TYPE_OFF;

  if (type_str) {
    if (is_str_equal(type_str, "off")) {
      buf_type = AM_ENCODE_SOURCE_BUFFER_TYPE_OFF;
    } else if (is_str_equal(type_str, "encode")) {
      buf_type = AM_ENCODE_SOURCE_BUFFER_TYPE_ENCODE;
    } else if (is_str_equal(type_str, "preview")) {
      buf_type = AM_ENCODE_SOURCE_BUFFER_TYPE_PREVIEW;
    } else {
      buf_type = AM_ENCODE_SOURCE_BUFFER_TYPE_OFF;
    }
  }

  return buf_type;
}

std::string source_buffer_type_to_str(AM_ENCODE_SOURCE_BUFFER_TYPE buf_type)
{
  std::string ret;
  switch (buf_type) {
    case AM_ENCODE_SOURCE_BUFFER_TYPE_ENCODE:
      ret = "encode";
      break;
    case AM_ENCODE_SOURCE_BUFFER_TYPE_PREVIEW:
      ret = "preview";
      break;
    case AM_ENCODE_SOURCE_BUFFER_TYPE_OFF:
    default:
      ret = "off";
      break;
  }
  return ret;
}

AMVideoFpsFormatQ9 get_amba_video_fps(AM_VIDEO_FPS fps)
{
  AMVideoFpsFormatQ9 amba_fps;
  switch (fps) {
    case AM_VIDEO_FPS_AUTO:
      amba_fps = AMBA_VIDEO_FPS_AUTO;
      break;
    case AM_VIDEO_FPS_29_97:
      amba_fps = AMBA_VIDEO_FPS_29_97;
      break;
    case AM_VIDEO_FPS_59_94:
      amba_fps = AMBA_VIDEO_FPS_59_94;
      break;
    case AM_VIDEO_FPS_23_976:
      amba_fps = AMBA_VIDEO_FPS_23_976;
      break;
    case AM_VIDEO_FPS_12_5:
      amba_fps = AMBA_VIDEO_FPS_12_5;
      break;
    case AM_VIDEO_FPS_6_25:
      amba_fps = AMBA_VIDEO_FPS_6_25;
      break;
    case AM_VIDEO_FPS_3_125:
      amba_fps = AMBA_VIDEO_FPS_3_125;
      break;
    case AM_VIDEO_FPS_7_5:
      amba_fps = AMBA_VIDEO_FPS_7_5;
      break;
    case AM_VIDEO_FPS_3_75:
      amba_fps = AMBA_VIDEO_FPS_3_75;
      break;
    default:
      //all other enum values are same as fps number
      amba_fps = (512000000L / fps);
      break;
  }
  return amba_fps;
}

AM_ENCODE_MODE camera_param_mode_str_to_enum(const char *mode_str)
{
  AM_ENCODE_MODE mode = AM_VIDEO_ENCODE_INVALID_MODE;
  if (is_str_equal(mode_str, "mode0")) {
    mode = AM_VIDEO_ENCODE_NORMAL_MODE;
  } else if (is_str_equal(mode_str, "mode1")) {
    mode = AM_VIDEO_ENCODE_DEWARP_MODE;
  } else if (is_str_equal(mode_str, "mode2")) {
    mode = AM_VIDEO_ENCODE_RESERVED1_MODE;
  } else if (is_str_equal(mode_str, "mode3")) {
  } else if (is_str_equal(mode_str, "mode4")) {
    mode = AM_VIDEO_ENCODE_ADV_ISO_MODE;
  } else if (is_str_equal(mode_str, "mode5")) {
    mode = AM_VIDEO_ENCODE_ADV_HDR_MODE;
  }
  return mode;
}

AM_HDR_EXPOSURE_TYPE camera_param_hdr_str_to_enum(const char *hdr_str)
{
  AM_HDR_EXPOSURE_TYPE hdr_type = AM_HDR_SINGLE_EXPOSURE;
  if (hdr_str) {
    if (is_str_equal(hdr_str, "2x")) {
      hdr_type = AM_HDR_2_EXPOSURE;
    } else if (is_str_equal(hdr_str, "3x")) {
      hdr_type = AM_HDR_3_EXPOSURE;
    } else if (is_str_equal(hdr_str, "4x")) {
      hdr_type = AM_HDR_4_EXPOSURE;
    }else if (is_str_equal(hdr_str, "sensor")) {
      hdr_type = AM_HDR_SENSOR_INTERNAL;
    }
  }
  return hdr_type;
}

AM_IMAGE_ISO_TYPE camera_param_iso_str_to_enum(const char *iso_str)
{
  AM_IMAGE_ISO_TYPE iso_type = AM_IMAGE_PIPELINE_NORMAL_ISO;
  if (iso_str) {
    if (is_str_equal(iso_str, "plus")) {
      iso_type = AM_IMAGE_PIPELINE_ISO_PLUS;
    } else if (is_str_equal(iso_str, "advanced")) {
      iso_type = AM_IMAGE_PIPELINE_ADVANCED_ISO;
    }
  }
  return iso_type;
}

AM_DEWARP_TYPE camera_param_dewarp_str_to_enum(const char *dewarp_str)
{
  AM_DEWARP_TYPE dewarp_type = AM_DEWARP_NONE;
  if (dewarp_str) {
    if (is_str_equal(dewarp_str, "ldc")) {
      dewarp_type = AM_DEWARP_LDC;
    } else if (is_str_equal(dewarp_str, "full")) {
      dewarp_type = AM_DEWARP_FULL;
    }
  }
  return dewarp_type;
}

std::string camera_param_mode_enum_to_str(AM_ENCODE_MODE mode)
{
  std::string ret;
  switch (mode) {
    case AM_VIDEO_ENCODE_NORMAL_MODE:
      ret = "mode0";
      break;
    case AM_VIDEO_ENCODE_DEWARP_MODE:
      ret = "mode1";
      break;
    case AM_VIDEO_ENCODE_RESERVED1_MODE:
      ret = "mode2";
      break;
    case AM_VIDEO_ENCODE_ADV_ISO_MODE:
      ret = "mode4";
      break;
    case AM_VIDEO_ENCODE_ADV_HDR_MODE:
      ret = "mode5";
      break;
    default:
      ret = "auto";
      break;
  }
  return ret;
}

std::string camera_param_hdr_enum_to_str(AM_HDR_EXPOSURE_TYPE hdr)
{
   std::string ret;
    switch (hdr) {
      case AM_HDR_2_EXPOSURE:
        ret = "2x";
        break;
      case AM_HDR_3_EXPOSURE:
        ret = "3x";
        break;
      case AM_HDR_4_EXPOSURE:
        ret = "4x";
        break;
      case AM_HDR_SENSOR_INTERNAL:
        ret = "sensor";
        break;
      default:
        ret = "none";
        break;
    }
    return ret;
}

std::string camera_param_iso_enum_to_str(AM_IMAGE_ISO_TYPE iso)
{
   std::string ret;
    switch (iso) {
      case AM_IMAGE_PIPELINE_ISO_PLUS:
        ret = "plus";
        break;
      case AM_IMAGE_PIPELINE_ADVANCED_ISO:
        ret = "advanced";
        break;
      default:
        ret = "normal";
        break;
    }
    return ret;
}

std::string camera_param_dewarp_enum_to_str(AM_DEWARP_TYPE dewarp)
{
   std::string ret;
    switch (dewarp) {
      case AM_DEWARP_LDC:
        ret = "ldc";
        break;
      case AM_DEWARP_FULL:
        ret = "full";
        break;
      default:
        ret = "none";
        break;
    }
    return ret;
}
