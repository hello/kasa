/*******************************************************************************
 * am_vin_trans.cpp
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
#include "am_log.h"
#include "am_video_param.h"
#include "am_vin_trans.h"

static AMVinModeTable gVinModeList[] = {
 {           "auto",0,  AM_VIN_MODE_AUTO      ,  0,     0},
 {            "vga",0,  AM_VIN_MODE_VGA       ,640,   480},
 {           "720p",0,  AM_VIN_MODE_720P      ,1280,  720},
 {           "960p",0,  AM_VIN_MODE_960P      ,1280,  960},
 {           "1.2m",0,  AM_VIN_MODE_960P      ,1280,  960},
 {          "1024p",0,  AM_VIN_MODE_1024P     ,1280, 1024},
 {           "1.3m",0,  AM_VIN_MODE_1024P     ,1280, 1024},
 {          "1080p",0,  AM_VIN_MODE_1080P     ,1920, 1080},
 {        "3m_16_9",0,  AM_VIN_MODE_3M_16_9   ,2304, 1296},
 {         "3m_4_3",0,  AM_VIN_MODE_3M_4_3    ,2048, 1536},
 {            "qhd",0,  AM_VIN_MODE_QHD       ,2560, 1440},
 {             "4m",0,  AM_VIN_MODE_4M_16_9   ,2688, 1520},
 {             "5m",0,  AM_VIN_MODE_5M_4_3    ,2592, 1944},
 {             "6m",0,  AM_VIN_MODE_6M_3_2    ,3072, 2048},
 {           "qfhd",0,  AM_VIN_MODE_QFHD      ,3840, 2160},
 {            "12m",0,  AM_VIN_MODE_12M       ,4000, 3000},
 //by resolution name
 {        "640x480",0,  AM_VIN_MODE_VGA       ,640,   480},
 {       "1280x720",0,  AM_VIN_MODE_720P      ,1280,  720},
 {       "1280x960",0,  AM_VIN_MODE_960P      ,1280,  960},
 {      "1280x1024",0,  AM_VIN_MODE_1024P     ,1280, 1024},
 {      "1920x1080",0,  AM_VIN_MODE_1080P     ,1920, 1080},
 {      "2304x1296",0,  AM_VIN_MODE_3M_16_9   ,2304, 1296},
 {      "2048x1536",0,  AM_VIN_MODE_3M_4_3    ,2048, 1536},
 {      "2560x1440",0,  AM_VIN_MODE_QHD       ,2560, 1440},
 //2688x1512 is accurate 16:9
 {      "2688x1512",0,  AM_VIN_MODE_4M_16_9   ,2688, 1512},
 {      "2592x1944",0,  AM_VIN_MODE_5M_4_3    ,2592, 1944},
 {      "3072x2048",0,  AM_VIN_MODE_6M_3_2    ,3072, 2048},
 {      "3840x2160",0,  AM_VIN_MODE_QFHD      ,3840, 2160},
 {      "4000x3000",0,  AM_VIN_MODE_12M       ,4000, 3000},
};

std::string vin_hdr_enum_to_str(AM_HDR_EXPOSURE_TYPE hdr_type)
{
  std::string ret;
  switch(hdr_type)
  {
    case AM_HDR_2_EXPOSURE:
      ret = "2-expo";
      break;

    case AM_HDR_3_EXPOSURE:
      ret = "3-expo";
      break;

    case AM_HDR_4_EXPOSURE:
      ret = "4-expo";
      break;

    case AM_HDR_SINGLE_EXPOSURE:
    default:
      ret = "none";
      break;
  }
  return ret;
}

std::string vin_mode_enum_to_str(AM_VIN_MODE mode)
{
  uint32_t i;
  std::string ret = "auto";
  for (i = 0; i < sizeof(gVinModeList) / sizeof(gVinModeList[0]); i ++) {
    if (gVinModeList[i].mode == mode) {
      ret = gVinModeList[i].name;
      break;
    }
  }
  return ret;
}

std::string vin_type_enum_to_str(AM_VIN_TYPE type)
{
  std::string ret ="none";
  switch (type)
  {
    case AM_VIN_TYPE_RGB_SENSOR:
      ret = "rgb_sensor";
      break;
    case AM_VIN_TYPE_YUV_SENSOR:
      ret = "yuv_sensor";
      break;
    case AM_VIN_TYPE_YUV_GENERIC:
      ret = "yuv_generic";
      break;
    case AM_VIN_TYPE_NONE:
    default:
      break;
  }
  return ret;
}

AM_VIN_TYPE vin_type_str_to_enum(const char *type_str)
{
  AM_VIN_TYPE type = AM_VIN_TYPE_NONE;
  //NULL
  if ((type_str) && (type_str[0] != '\0')) {
    if (!strcmp(type_str, "rgb_sensor")) {
      type = AM_VIN_TYPE_RGB_SENSOR;
    } else if (!strcmp(type_str, "yuv_sensor")) {
      type = AM_VIN_TYPE_YUV_SENSOR;
    } else if (!strcmp(type_str, "yuv_generic")) {
      type = AM_VIN_TYPE_YUV_GENERIC;
    } else {
      type = AM_VIN_TYPE_NONE;
    }
  }
  return type;
}

AM_VIN_MODE vin_mode_str_to_enum(const char *mode_str)
{
  AM_VIN_MODE mode = AM_VIN_MODE_AUTO;
  uint32_t i;
  //NULL
  if (mode_str && (mode_str[0] != '\0')) {
    for (i = 0; i < sizeof(gVinModeList) / sizeof(gVinModeList[0]); i ++) {
      if (!strcmp(mode_str, gVinModeList[i].name)) {
        //found
        mode = gVinModeList[i].mode;
        break;
      }
    }
  }
  return mode;
}

int get_mirror_pattern(AM_VIDEO_FLIP flip)
{
  enum amba_vindev_mirror_pattern_e pattern;
  switch (flip) {
    case AM_VIDEO_FLIP_NONE:
      pattern = VINDEV_MIRROR_NONE;
      break;

    case AM_VIDEO_FLIP_VERTICAL:
      pattern = VINDEV_MIRROR_VERTICALLY;
      break;

    case AM_VIDEO_FLIP_HORIZONTAL:
      pattern = VINDEV_MIRROR_HORRIZONTALLY;
      break;

    case AM_VIDEO_FLIP_VH_BOTH:
      pattern = VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY;
      break;

    case AM_VIDEO_FLIP_AUTO:
    default:
      pattern = VINDEV_MIRROR_AUTO;
      break;
  }
  return pattern;
}

AMResolution vin_mode_to_resolution(AM_VIN_MODE mode)
{
  AMResolution size = {0};

  uint32_t i;
  for (i = 0; i < sizeof(gVinModeList) / sizeof(gVinModeList[0]); ++i) {
    if (gVinModeList[i].mode == mode) {
      size.width = gVinModeList[i].width;
      size.height = gVinModeList[i].height;
      break;
    }
  }

  return size;
}

AM_VIN_MODE resolution_to_vin_mode(const AMResolution *size)
{
  AM_VIN_MODE mode = AM_VIN_MODE_AUTO;
  do {
    if (!size) {
      ERROR("The pointer of AMResolution is NULL");
      break;
    }

    for (uint32_t i = 0; i < sizeof(gVinModeList) / sizeof(gVinModeList[0]); ++i) {
      if (gVinModeList[i].width == size->width &&
          gVinModeList[i].height == size->height) {
        mode = gVinModeList[i].mode;
        break;
      }
    }
  } while (0);

  return mode;
}

int get_hdr_expose_num(AM_HDR_EXPOSURE_TYPE hdr_type)
{
  int num = 1;
  switch (hdr_type) {
    case AM_HDR_2_EXPOSURE:
      num = 2;
      break;
    case AM_HDR_3_EXPOSURE:
      num = 3;
      break;
    case AM_HDR_4_EXPOSURE:
      num = 4;
      break;
    default:
      break;
  }
  return num;
}

int get_hdr_mode(AM_HDR_EXPOSURE_TYPE hdr_type)
{
  int mode;
  switch(hdr_type)
  {
    case AM_HDR_SINGLE_EXPOSURE:
      mode = AMBA_VIDEO_LINEAR_MODE;
      break;
    case AM_HDR_2_EXPOSURE:
      mode = AMBA_VIDEO_2X_HDR_MODE;
      break;
    case AM_HDR_3_EXPOSURE:
      mode = AMBA_VIDEO_3X_HDR_MODE;
      break;
    case AM_HDR_4_EXPOSURE:
      mode = AMBA_VIDEO_4X_HDR_MODE;
      break;
    case AM_HDR_SENSOR_INTERNAL:
      mode = AMBA_VIDEO_INT_HDR_MODE;
      break;
    default:
      PRINTF("error of get_hdr_mode\n");
      mode = AMBA_VIDEO_LINEAR_MODE;
      break;
  }
  return mode;
}

amba_video_mode get_amba_video_mode(AM_VIN_MODE mode)
{
  amba_video_mode video_mode;
  switch (mode) {
    case AM_VIN_MODE_AUTO:
      video_mode = AMBA_VIDEO_MODE_AUTO;
      break;
    case AM_VIN_MODE_VGA:
      video_mode = AMBA_VIDEO_MODE_VGA;
      break;
    case AM_VIN_MODE_720P:
      video_mode = AMBA_VIDEO_MODE_720P;
      break;
    case AM_VIN_MODE_960P:
      video_mode = AMBA_VIDEO_MODE_1280_960;
      break;
    case AM_VIN_MODE_1024P:
      video_mode = AMBA_VIDEO_MODE_SXGA;
      break;
    case AM_VIN_MODE_1080P:
      video_mode = AMBA_VIDEO_MODE_1080P;
      break;
    case AM_VIN_MODE_3M_4_3:
      video_mode = AMBA_VIDEO_MODE_QXGA;
      break;
    case AM_VIN_MODE_3M_16_9:
      video_mode = AMBA_VIDEO_MODE_2304_1296;
      break;
    case AM_VIN_MODE_QHD: //2560x1440
      video_mode = AMBA_VIDEO_MODE_2560_1440;
      break;
    case AM_VIN_MODE_4M_16_9: //2688x1512
      video_mode = AMBA_VIDEO_MODE_2688_1512;
      break;
    case AM_VIN_MODE_5M_4_3:
      video_mode = AMBA_VIDEO_MODE_QSXGA;
      break;
    case AM_VIN_MODE_6M_3_2:
      video_mode = AMBA_VIDEO_MODE_3072_2048;
      break;
    case AM_VIN_MODE_QFHD:
      video_mode = AMBA_VIDEO_MODE_3840_2160;
      break;
    case AM_VIN_MODE_4096X2160:
      video_mode = AMBA_VIDEO_MODE_4096_2160;
      break;
    case AM_VIN_MODE_12M:
      video_mode = AMBA_VIDEO_MODE_4000_3000;
      break;
    default:
      ERROR("video mode %d not supported yet, use auto \n", mode);
      video_mode = AMBA_VIDEO_MODE_AUTO;
      break;
  }

  return video_mode;
}
