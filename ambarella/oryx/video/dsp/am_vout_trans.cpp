/*******************************************************************************
 * am_vout_trans.cpp
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
#include "am_vout_trans.h"
#include "am_video_trans.h"

static AMVoutModeTable gVoutModeList[] = {
 {     "auto",0,  AM_VOUT_MODE_AUTO    , AM_VIDEO_FPS_AUTO,     0,    0},
 {     "480i",0,  AM_VOUT_MODE_480I    , AM_VIDEO_FPS_59_94,  720,  480},
 {     "576i",0,  AM_VOUT_MODE_576I    , AM_VIDEO_FPS_50,     720,  576},
 {     "480p",0,  AM_VOUT_MODE_480P    , AM_VIDEO_FPS_59_94,  720,  480},
 {     "576p",0,  AM_VOUT_MODE_576P    , AM_VIDEO_FPS_50,     720,  480},
 {     "720p",0,  AM_VOUT_MODE_720P60  , AM_VIDEO_FPS_59_94, 1280,  720},
 {   "720p60",0,  AM_VOUT_MODE_720P60  , AM_VIDEO_FPS_59_94, 1280,  720},
 {   "720p50",0,  AM_VOUT_MODE_720P50  , AM_VIDEO_FPS_50,    1280,  720},
 {   "720p30",0,  AM_VOUT_MODE_720P30  , AM_VIDEO_FPS_29_97, 1280,  720},
 {   "720p25",0,  AM_VOUT_MODE_720P25  , AM_VIDEO_FPS_25,    1280,  720},
 {    "1080i",0,  AM_VOUT_MODE_1080I60 , AM_VIDEO_FPS_59_94, 1920, 1080},
 {  "1080i60",0,  AM_VOUT_MODE_1080I60 , AM_VIDEO_FPS_59_94, 1920, 1080},
 {  "1080i50",0,  AM_VOUT_MODE_1080I50 , AM_VIDEO_FPS_50,    1920, 1080},
 {    "1080p",0,  AM_VOUT_MODE_1080P60 , AM_VIDEO_FPS_59_94, 1920, 1080},
 {  "1080p60",0,  AM_VOUT_MODE_1080P60 , AM_VIDEO_FPS_59_94, 1920, 1080},
 {  "1080p50",0,  AM_VOUT_MODE_1080P50 , AM_VIDEO_FPS_50,    1920, 1080},
 {  "1080p30",0,  AM_VOUT_MODE_1080P30 , AM_VIDEO_FPS_29_97, 1920, 1080},
 {  "1080p25",0,  AM_VOUT_MODE_1080P25 , AM_VIDEO_FPS_25,    1920, 1080},
 {  "1080p24",0,  AM_VOUT_MODE_1080P24 , AM_VIDEO_FPS_24,    1920, 1080},
 {  "qfhd30" ,0,  AM_VOUT_MODE_QFHD30  , AM_VIDEO_FPS_29_97, 3840, 2160},
 {  "qfhd24" ,0,  AM_VOUT_MODE_QFHD24  , AM_VIDEO_FPS_24,    3840, 2160},
};

int get_amba_vout_fps(AM_VOUT_MODE mode)
{
  int vout_fps = get_amba_video_fps(AM_VIDEO_FPS_AUTO);
  uint32_t i = 0;
  for (i = 0; i < sizeof(gVoutModeList) / sizeof(gVoutModeList[0]); ++i) {
    if (mode == gVoutModeList[i].mode) {
      vout_fps = get_amba_video_fps(gVoutModeList[i].video_fps);
      break;
    }
  }
  if (i >= sizeof(gVoutModeList) / sizeof(gVoutModeList[0])) {
    vout_fps = get_amba_video_fps(AM_VIDEO_FPS_AUTO);
    NOTICE("Vout mode not found! Set to auto mode!");
  }
  return vout_fps;
}

int get_amba_vout_mode(AM_VOUT_MODE mode)
{
  int vout_mode;
  switch (mode) {
    case AM_VOUT_MODE_480P:
      vout_mode = AMBA_VIDEO_MODE_D1_NTSC;
      break;
    case AM_VOUT_MODE_576P:
      vout_mode = AMBA_VIDEO_MODE_D1_PAL;
      break;
    case AM_VOUT_MODE_480I:
      vout_mode = AMBA_VIDEO_MODE_480I;
      break;
    case AM_VOUT_MODE_576I:
      vout_mode = AMBA_VIDEO_MODE_576I;
      break;
    case AM_VOUT_MODE_720P60:
      vout_mode = AMBA_VIDEO_MODE_720P;
      break;
    case AM_VOUT_MODE_720P50:
      vout_mode = AMBA_VIDEO_MODE_720P50;
      break;
    case AM_VOUT_MODE_720P30:
      vout_mode = AMBA_VIDEO_MODE_720P30;
      break;
    case AM_VOUT_MODE_720P25:
      vout_mode = AMBA_VIDEO_MODE_720P25;
      break;
    case AM_VOUT_MODE_1080P60:
      vout_mode = AMBA_VIDEO_MODE_1080P;
      break;
    case AM_VOUT_MODE_1080P50:
      vout_mode = AMBA_VIDEO_MODE_1080P50;
      break;
    case AM_VOUT_MODE_1080P30:
      vout_mode = AMBA_VIDEO_MODE_1080P30;
      break;
    case AM_VOUT_MODE_1080P25:
      vout_mode = AMBA_VIDEO_MODE_1080P25;
      break;
    case AM_VOUT_MODE_1080P24:
      vout_mode = AMBA_VIDEO_MODE_1080P24;
      break;
    case AM_VOUT_MODE_1080I60:
      vout_mode = AMBA_VIDEO_MODE_1080I;
      break;
    case AM_VOUT_MODE_1080I50:
      vout_mode = AMBA_VIDEO_MODE_1080I50;
      break;
    case AM_VOUT_MODE_QFHD30:
      vout_mode = AMBA_VIDEO_MODE_2160P30;
      break;
    case AM_VOUT_MODE_QFHD24:
      vout_mode = AMBA_VIDEO_MODE_2160P24;
      break;
    case AM_VOUT_MODE_AUTO:
    default:
      vout_mode = AMBA_VIDEO_MODE_AUTO;
      break;
  }
  return vout_mode;
}

int get_vout_sink_type(AM_VOUT_TYPE  type)
{
  int ret = 0;

  switch (type) {
    case AM_VOUT_TYPE_LCD:
      ret = AMBA_VOUT_SINK_TYPE_DIGITAL;
      break;
    case AM_VOUT_TYPE_HDMI:
      ret = AMBA_VOUT_SINK_TYPE_HDMI;
      break;
    case AM_VOUT_TYPE_CVBS:
      ret = AMBA_VOUT_SINK_TYPE_CVBS;
      break;
    case AM_VOUT_TYPE_DIGITAL:
      ret = AMBA_VOUT_SINK_TYPE_DIGITAL;
      break;
    case AM_VOUT_TYPE_NONE:
    default:
      ret = AMBA_VOUT_SINK_TYPE_AUTO;
      break;
  }
  return ret;
}

int get_vout_source_id(AM_VOUT_ID am_vout_id)
{
  int vout_source_id; //the valud used in iav for VOUT source id
  switch (am_vout_id) {
    case AM_VOUT_A:
      vout_source_id = 0;
      break;
    case AM_VOUT_B:
    default:
      vout_source_id = 1;
      break;
  }
  return vout_source_id;
}

std::string vout_type_enum_to_str(AM_VOUT_TYPE type)
{
  std::string ret = "none";
  switch (type) {
    case AM_VOUT_TYPE_HDMI:
      ret = "hdmi";
      break;
    case AM_VOUT_TYPE_CVBS:
      ret = "cvbs";
      break;
    case AM_VOUT_TYPE_LCD:
      ret = "lcd";
      break;
    case AM_VOUT_TYPE_DIGITAL:
      ret = "digital";
      break;
    case AM_VOUT_TYPE_NONE:
    default:
      break;
  }
  return ret;
}

std::string vout_mode_enum_to_str(AM_VOUT_MODE mode)
{
  uint32_t i;
  std::string ret = "auto";
  for (i = 0; i < sizeof(gVoutModeList) / sizeof(gVoutModeList[0]); i ++) {
    if (gVoutModeList[i].mode == mode) {
      ret = gVoutModeList[i].name;
      break;
    }
  }
  return ret;
}

AM_VOUT_MODE vout_mode_str_to_enum(const char *mode_str)
{
  AM_VOUT_MODE mode = AM_VOUT_MODE_AUTO;
  uint32_t i;
  //NULL
  if (mode_str && (mode_str[0] != '\0')) {
    for (i = 0; i < sizeof(gVoutModeList) / sizeof(gVoutModeList[0]); i ++) {
      if (!strcmp(mode_str, gVoutModeList[i].name)) {
        //found
        mode = gVoutModeList[i].mode;
        break;
      }
    }
  }
  return mode;
}

AM_VOUT_TYPE vout_type_str_to_enum(const char *type_str)
{
  AM_VOUT_TYPE type = AM_VOUT_TYPE_NONE;
  if ((type_str) && (type_str[0] != '\0')) {
    if (!strcmp(type_str, "hdmi")) {
      type = AM_VOUT_TYPE_HDMI;
    } else if (!strcmp(type_str, "cvbs")) {
      type = AM_VOUT_TYPE_CVBS;
    } else if (!strcmp(type_str, "lcd")) {
      type = AM_VOUT_TYPE_LCD;
    } else if (!strcmp(type_str, "digital")) {
      type = AM_VOUT_TYPE_DIGITAL;
    } else {
      type = AM_VOUT_TYPE_NONE;
    }
  }
  return type;
}

AM_VOUT_MODE resolution_to_vout_mode(const AMResolution *size)
{
  AM_VOUT_MODE mode = AM_VOUT_MODE_AUTO;
  do {
    if (!size) {
      ERROR("The pointer of AMResolution is NULL");
      break;
    }

    uint32_t i = 0;
    AMResolution s = *size;
    for (i = 0; i < (sizeof(gVoutModeList) / sizeof(gVoutModeList[0])); ++i) {
      if (s.width == gVoutModeList[i].width &&
          s.height == gVoutModeList[i].height) {
        mode = gVoutModeList[i].mode;
        break;
      }
    }
    if (i >= (sizeof(gVoutModeList) / sizeof(gVoutModeList[0]))) {
      ERROR("Vout mode not found, set to auto mode");
    }
  } while (0);
  return mode;
}

AMResolution vout_mode_to_resolution(AM_VOUT_MODE mode)
{
  AMResolution size = {0};
  do {
    uint32_t i = 0;
    for (i = 0; i < sizeof(gVoutModeList) / sizeof(gVoutModeList[0]); ++i) {
      if (mode == gVoutModeList[i].mode) {
        size.width = gVoutModeList[i].width;
        size.height = gVoutModeList[i].height;
        PRINTF("get vout mode: %d, width: %d, height: %d",
               mode, size.width, size.height);
        break;
      }
    }
    if (i >= sizeof(gVoutModeList) / sizeof(gVoutModeList[0])) {
      ERROR("Vout mode not found, please check!");
    }
  } while (0);
  return size;
}
