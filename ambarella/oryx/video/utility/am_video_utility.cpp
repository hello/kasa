/*******************************************************************************
 * am_video_utility.cpp
 *
 * History:
 *   2015-7-17 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_video_utility.h"

#include "basetypes.h"
#if defined(CONFIG_AMBARELLA_ORYX_VIDEO_IAV1)
#include "ambas_common.h"
#include "ambas_vout.h"
#elif defined(CONFIG_AMBARELLA_ORYX_VIDEO_IAV2) || \
      defined(CONFIG_AMBARELLA_ORYX_VIDEO_IAV3)
#include "iav_common.h"
#include "iav_vout_ioctl.h"
#endif
/* namespace AMVinTrans */
namespace AMVinTrans
{
static AMVinModeTable g_vin_mode_list[] = {
    { "auto",      0, AM_VIN_MODE_AUTO,      0,       0 },
    { "vga",       0, AM_VIN_MODE_VGA,       640,   480 },
    { "720p",      0, AM_VIN_MODE_720P,      1280,  720 },
    { "960p",      0, AM_VIN_MODE_960P,      1280,  960 },
    { "1.2m",      0, AM_VIN_MODE_960P,      1280,  960 },
    { "1024p",     0, AM_VIN_MODE_1024P,     1280, 1024 },
    { "1.3m",      0, AM_VIN_MODE_1024P,     1280, 1024 },
    { "1080p",     0, AM_VIN_MODE_1080P,     1920, 1080 },
    { "3m_16_9",   0, AM_VIN_MODE_3M_16_9,   2304, 1296 },
    { "3m_4_3",    0, AM_VIN_MODE_3M_4_3,    2048, 1536 },
    { "qhd",       0, AM_VIN_MODE_QHD,       2560, 1440 },
    { "4m",        0, AM_VIN_MODE_4M_16_9,   2688, 1520 },
    { "5m",        0, AM_VIN_MODE_5M_4_3,    2592, 1944 },
    { "6m",        0, AM_VIN_MODE_6M_3_2,    3072, 2048 },
    { "qfhd",      0, AM_VIN_MODE_QFHD,      3840, 2160 },
    { "12m",       0, AM_VIN_MODE_12M,       4000, 3000 },
    //by resolution name
    { "640x480",   0, AM_VIN_MODE_VGA,        640,  480 },
    { "1280x720",  0, AM_VIN_MODE_720P,      1280,  720 },
    { "1280x960",  0, AM_VIN_MODE_960P,      1280,  960 },
    { "1280x1024", 0, AM_VIN_MODE_1024P,     1280, 1024 },
    { "1920x1080", 0, AM_VIN_MODE_1080P,     1920, 1080 },
    { "1920x1200", 0, AM_VIN_MODE_1920X1200, 1920, 1200 },
    { "2304x1296", 0, AM_VIN_MODE_3M_16_9,   2304, 1296 },
    { "2048x1536", 0, AM_VIN_MODE_3M_4_3,    2048, 1536 },
    { "2560x1440", 0, AM_VIN_MODE_QHD,       2560, 1440 },
    //2688x1512 is accurate 16:9
    { "2688x1512", 0, AM_VIN_MODE_4M_16_9,   2688, 1512 },
    { "2688x1520", 0, AM_VIN_MODE_2688X1520, 2688, 1520 },
    { "2592x1944", 0, AM_VIN_MODE_5M_4_3,    2592, 1944 },
    { "3072x2048", 0, AM_VIN_MODE_6M_3_2,    3072, 2048 },
    { "3840x2160", 0, AM_VIN_MODE_QFHD,      3840, 2160 },
    { "4000x3000", 0, AM_VIN_MODE_12M,       4000, 3000 },
    { "4096x2160", 0, AM_VIN_MODE_4096X2160, 4096, 2160 },
};

std::string type_enum_to_str(AM_SENSOR_TYPE type)
{
  std::string ret = "none";
  switch (type) {
    case AM_SENSOR_TYPE_RGB:
      ret = "rgb_sensor";
      break;
    case AM_SENSOR_TYPE_YUV:
      ret = "yuv_sensor";
      break;
    case AM_SENSOR_TYPE_YUV_GENERIC:
      ret = "yuv_generic";
      break;
    case AM_SENSOR_TYPE_NONE:
    default:
      break;
  }
  return ret;
}

AM_SENSOR_TYPE type_str_to_enum(const std::string &type_str)
{
  AM_SENSOR_TYPE type = AM_SENSOR_TYPE_NONE;
  if (AM_LIKELY(!type_str.empty())) {
    const char *str = type_str.c_str();
    if (is_str_equal(str, "rgb_sensor")) {
      type = AM_SENSOR_TYPE_RGB;
    } else if (is_str_equal(str, "yuv_sensor")) {
      type = AM_SENSOR_TYPE_YUV;
    } else if (is_str_equal(str, "yuv_generic")) {
      type = AM_SENSOR_TYPE_YUV_GENERIC;
    } else if (is_str_equal(str, "none")) {
      type = AM_SENSOR_TYPE_NONE;
    } else {
      WARN("Invalid VIN type: %s! Use default value \"none\"", str);
    }
  }
  return type;
}

std::string mode_enum_to_str(AM_VIN_MODE mode)
{
  std::string ret = "auto";
  uint32_t list_len = sizeof(g_vin_mode_list) / sizeof(g_vin_mode_list[0]);
  for (uint32_t i = 0; i < list_len; ++ i) {
    if (AM_LIKELY(g_vin_mode_list[i].mode == mode)) {
      ret = g_vin_mode_list[i].name;
      break;
    }
  }
  return ret;
}

AM_VIN_MODE mode_str_to_enum(const std::string &mode_str)
{
  AM_VIN_MODE mode = AM_VIN_MODE_AUTO;
  bool found = false;
  if (AM_LIKELY(!mode_str.empty())) {
    uint32_t list_len = sizeof(g_vin_mode_list) / sizeof(g_vin_mode_list[0]);
    for (uint32_t i = 0; i < list_len; ++ i) {
      if (AM_LIKELY(is_str_equal(mode_str.c_str(), g_vin_mode_list[i].name))) {
        mode = g_vin_mode_list[i].mode;
        found = true;
        break;
      }
    }
  }
  if (AM_UNLIKELY(!found)) {
    WARN("Unknown VIN mode: %s! "
         "Use default value \"auto\"",
         mode_str.c_str());
  }

  return mode;
}

std::string hdr_enum_to_str(AM_HDR_TYPE hdr_type)
{
  std::string ret;
  switch (hdr_type) {
    case AM_HDR_2_EXPOSURE: {
      ret = "2x";
    }
      break;
    case AM_HDR_3_EXPOSURE: {
      ret = "3x";
    }
      break;
    case AM_HDR_4_EXPOSURE: {
      ret = "4x";
    }
      break;
    case AM_HDR_SENSOR_INTERNAL: {
      ret = "sensor";
    }
      break;
    case AM_HDR_SINGLE_EXPOSURE:
    default: {
      ret = "linear";
    }
      break;
  }

  return ret;
}

AM_HDR_TYPE hdr_str_to_enum(const std::string &hdr_str)
{
  AM_HDR_TYPE type = AM_HDR_SINGLE_EXPOSURE;

  if (AM_LIKELY(!hdr_str.empty())) {
    const char *hdr = hdr_str.c_str();
    if (is_str_equal(hdr, "linear") ||
        is_str_equal(hdr, "single") ||
        is_str_equal(hdr, "hdr-none") ||
        is_str_equal(hdr, "none")) {
      type = AM_HDR_SINGLE_EXPOSURE;
    } else if (is_str_equal(hdr, "2x") ||
               is_str_equal(hdr, "hdr-2x") ||
               is_str_n_equal(hdr, "2-expo", strlen("2-expo"))) {
      type = AM_HDR_2_EXPOSURE;
    } else if (is_str_equal(hdr, "3x") ||
               is_str_equal(hdr, "hdr-3x") ||
               is_str_n_equal(hdr, "3-expo", strlen("3-expo"))) {
      type = AM_HDR_3_EXPOSURE;
    } else if (is_str_equal(hdr, "4x") ||
               is_str_equal(hdr, "hdr-4x") ||
               is_str_n_equal(hdr, "4-expo", strlen("4-expo"))) {
      type = AM_HDR_4_EXPOSURE;
    } else if (is_str_equal(hdr, "sensor") ||
               is_str_equal(hdr, "hdr-sensor") ||
               is_str_n_equal(hdr, "int", strlen("int"))) {
      type = AM_HDR_SENSOR_INTERNAL;
    } else {
      WARN("Unknown HDR type: %d! Use default value \"linear\"", hdr);
    }
  }

  return type;
}

AMResolution mode_to_resolution(AM_VIN_MODE mode)
{
  AMResolution size = { 0 };
  uint32_t list_len = sizeof(g_vin_mode_list) / sizeof(g_vin_mode_list[0]);

  for (uint32_t i = 0; i < list_len; ++ i) {
    if (AM_LIKELY(g_vin_mode_list[i].mode == mode)) {
      size.width = g_vin_mode_list[i].width;
      size.height = g_vin_mode_list[i].height;
      break;
    }
  }

  return size;
}

AM_VIN_MODE resolution_to_mode(const AMResolution &size)
{
  AM_VIN_MODE mode = AM_VIN_MODE_AUTO;
  uint32_t list_len = sizeof(g_vin_mode_list) / sizeof(g_vin_mode_list[0]);
  bool ok = false;

  for (uint32_t i = 0; i < list_len; ++ i) {
    if (AM_LIKELY(g_vin_mode_list[i].width == size.width
        && g_vin_mode_list[i].height == size.height)) {
      mode = g_vin_mode_list[i].mode;
      ok = true;
      break;
    }
  }
  if (AM_UNLIKELY(!ok)) {
    WARN("Unknown resolution: %ux%u! "
         "Use default value \"auto\"",
         size.width, size.height);
  }

  return mode;
}

int bits_mw_to_iav(AM_VIN_BITS bits)
{
  int vin_bits;
  switch (bits) {
    case AM_VIN_BITS_AUTO:
      vin_bits = 0;
      break;
    case AM_VIN_BITS_8:
      vin_bits = 8;
      break;
    case AM_VIN_BITS_10:
      vin_bits = 10;
      break;
    case AM_VIN_BITS_12:
      vin_bits = 12;
      break;
    case AM_VIN_BITS_14:
      vin_bits = 14;
      break;
    case AM_VIN_BITS_16:
      vin_bits = 16;
      break;
    default:
      WARN("Unknown VIN bits: %d! Use default value \"auto\"!", bits);
      vin_bits = 0;
      break;
  }
  return vin_bits;
}

AM_VIN_BITS bits_iav_to_mw(int bits)
{
  AM_VIN_BITS vin_bits;
  switch (bits) {
    case 0:
      vin_bits = AM_VIN_BITS_AUTO;
      break;
    case 8:
      vin_bits = AM_VIN_BITS_8;
      break;
    case 10:
      vin_bits = AM_VIN_BITS_10;
      break;
    case 12:
      vin_bits = AM_VIN_BITS_12;
      break;
    case 14:
      vin_bits = AM_VIN_BITS_14;
      break;
    case 16:
      vin_bits = AM_VIN_BITS_16;
      break;
    default:
      WARN("Unknown VIN bits: %d! Use default value \"auto\"!", bits);
      vin_bits = AM_VIN_BITS_AUTO;
      break;
  }
  return vin_bits;
}

} /* namespace AMVinTrans */

namespace AMVoutTrans
{
static AMVoutModeTable g_vout_mode_list[] =
{
{ "auto", 0, AM_VOUT_MODE_AUTO, AM_VIDEO_FPS_AUTO, 0, 0 },
  { "480i", 0, AM_VOUT_MODE_480I, AM_VIDEO_FPS_59_94, 720, 480 },
  { "576i", 0, AM_VOUT_MODE_576I, AM_VIDEO_FPS_50, 720, 576 },
  { "480p", 0, AM_VOUT_MODE_480P, AM_VIDEO_FPS_59_94, 720, 480 },
  { "576p", 0, AM_VOUT_MODE_576P, AM_VIDEO_FPS_50, 720, 480 },
  { "720p", 0, AM_VOUT_MODE_720P60, AM_VIDEO_FPS_59_94, 1280, 720 },
  { "720p60", 0, AM_VOUT_MODE_720P60, AM_VIDEO_FPS_59_94, 1280, 720 },
  { "720p50", 0, AM_VOUT_MODE_720P50, AM_VIDEO_FPS_50, 1280, 720 },
  { "720p30", 0, AM_VOUT_MODE_720P30, AM_VIDEO_FPS_29_97, 1280, 720 },
  { "720p25", 0, AM_VOUT_MODE_720P25, AM_VIDEO_FPS_25, 1280, 720 },
  { "1080i", 0, AM_VOUT_MODE_1080I60, AM_VIDEO_FPS_59_94, 1920, 1080 },
  { "1080i60", 0, AM_VOUT_MODE_1080I60, AM_VIDEO_FPS_59_94, 1920, 1080 },
  { "1080i50", 0, AM_VOUT_MODE_1080I50, AM_VIDEO_FPS_50, 1920, 1080 },
  { "1080p", 0, AM_VOUT_MODE_1080P60, AM_VIDEO_FPS_59_94, 1920, 1080 },
  { "1080p60", 0, AM_VOUT_MODE_1080P60, AM_VIDEO_FPS_59_94, 1920, 1080 },
  { "1080p50", 0, AM_VOUT_MODE_1080P50, AM_VIDEO_FPS_50, 1920, 1080 },
  { "1080p30", 0, AM_VOUT_MODE_1080P30, AM_VIDEO_FPS_29_97, 1920, 1080 },
  { "1080p25", 0, AM_VOUT_MODE_1080P25, AM_VIDEO_FPS_25, 1920, 1080 },
  { "1080p24", 0, AM_VOUT_MODE_1080P24, AM_VIDEO_FPS_24, 1920, 1080 },
  { "qfhd30", 0, AM_VOUT_MODE_QFHD30, AM_VIDEO_FPS_29_97, 3840, 2160 },
  { "qfhd24", 0, AM_VOUT_MODE_QFHD24, AM_VIDEO_FPS_24, 3840, 2160 },
};

std::string sink_type_enum_to_str(AM_VOUT_TYPE type)
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
    case AM_VOUT_TYPE_YPBPR:
      ret = "ypbpr";
      break;
    case AM_VOUT_TYPE_NONE:
    default:
      ret = "none";
      break;
  }

  return ret;
}

AM_VOUT_TYPE sink_type_str_to_enum(const std::string &type_str)
{
  AM_VOUT_TYPE vout_type = AM_VOUT_TYPE_NONE;
  if (AM_LIKELY(!type_str.empty())) {
    const char *type = type_str.c_str();
    if (is_str_equal(type, "hdmi")) {
      vout_type = AM_VOUT_TYPE_HDMI;
    } else if (is_str_equal(type, "cvbs")) {
      vout_type = AM_VOUT_TYPE_CVBS;
    } else if (is_str_equal(type, "lcd")) {
      vout_type = AM_VOUT_TYPE_LCD;
    } else if (is_str_equal(type, "ypbpr")) {
      vout_type = AM_VOUT_TYPE_YPBPR;
    } else if (is_str_equal(type, "none")) {
      vout_type = AM_VOUT_TYPE_NONE;
    } else {
      WARN("Unknown VOUT type: %s! Use default value \"none\"!", type);
    }
  }
  return vout_type;
}

std::string video_type_enum_to_str(AM_VOUT_VIDEO_TYPE type)
{
  std::string ret = "none";
  switch (type) {
    case AM_VOUT_VIDEO_TYPE_YUV601:
      ret = "yuv601";
      break;
    case AM_VOUT_VIDEO_TYPE_YUV656:
      ret = "yuv656";
      break;
    case AM_VOUT_VIDEO_TYPE_YUV1120:
      ret = "yuv1120";
      break;
    case AM_VOUT_VIDEO_TYPE_RGB601:
      ret = "rgb601";
      break;
    case AM_VOUT_VIDEO_TYPE_RGB656:
      ret = "rgb656";
      break;
    case AM_VOUT_VIDEO_TYPE_RGB1120:
      ret = "RGB1120";
      break;
    case AM_VOUT_VIDEO_TYPE_NONE:
    default:
      ret = "none";
      break;
  }

  return ret;
}

AM_VOUT_VIDEO_TYPE video_type_str_to_enum(const std::string &type_str)
{
  AM_VOUT_VIDEO_TYPE video_type = AM_VOUT_VIDEO_TYPE_NONE;
  if (AM_LIKELY(!type_str.empty())) {
    const char *type = type_str.c_str();
    if (is_str_equal(type, "yuv601")) {
      video_type = AM_VOUT_VIDEO_TYPE_YUV601;
    } else if (is_str_equal(type, "yuv656")) {
      video_type = AM_VOUT_VIDEO_TYPE_YUV656;
    } else if (is_str_equal(type, "yuv1120")) {
      video_type = AM_VOUT_VIDEO_TYPE_YUV1120;
    } else if (is_str_equal(type, "rgb601")) {
      video_type = AM_VOUT_VIDEO_TYPE_RGB601;
    } else if (is_str_equal(type, "rgb656")) {
      video_type = AM_VOUT_VIDEO_TYPE_RGB656;
    } else if (is_str_equal(type, "rgb1120")) {
      video_type = AM_VOUT_VIDEO_TYPE_RGB1120;
    } else if (is_str_equal(type, "none")) {
      video_type = AM_VOUT_VIDEO_TYPE_NONE;
    } else {
      WARN("Unknown VOUT video type: %s! Use default value \"none\"!", type);
    }
  }
  return video_type;
}

std::string mode_enum_to_str(AM_VOUT_MODE mode)
{
  std::string ret = "auto";
  uint32_t list_size = sizeof(g_vout_mode_list) / sizeof(g_vout_mode_list[0]);
  for (uint32_t i = 0; i < list_size; ++ i) {
    if (AM_LIKELY(g_vout_mode_list[i].mode == mode)) {
      ret = g_vout_mode_list[i].name;
      break;
    }
  }
  return ret;
}

AM_VOUT_MODE mode_str_to_enum(const std::string &mode_str)
{
  AM_VOUT_MODE mode = AM_VOUT_MODE_AUTO;
  bool found = false;

  if (AM_LIKELY(!mode_str.empty())) {
    uint32_t list_size = sizeof(g_vout_mode_list) / sizeof(g_vout_mode_list[0]);
    for (uint32_t i = 0; i < list_size; ++ i) {
      if (AM_LIKELY(is_str_equal(mode_str.c_str(), g_vout_mode_list[i].name))) {
        mode = g_vout_mode_list[i].mode;
        found = true;
        break;
      }
    }
  }
  if (AM_UNLIKELY(!found)) {
    WARN("VOUT mode %s can NOT be found! Use default value \"auto\"!",
         mode_str.c_str());
    mode = AM_VOUT_MODE_AUTO;
  }

  return mode;
}

AM_VOUT_MODE resolution_to_mode(const AMResolution &size)
{
  AM_VOUT_MODE mode = AM_VOUT_MODE_AUTO;
  uint32_t list_size = sizeof(g_vout_mode_list) / sizeof(g_vout_mode_list[0]);
  bool found = false;

  for (uint32_t i = 0; i < list_size; ++ i) {
    if (AM_LIKELY((size.width == g_vout_mode_list[i].width)
        && (size.height == g_vout_mode_list[i].height))) {
      mode = g_vout_mode_list[i].mode;
      found = true;
      break;
    }
  }
  if (AM_UNLIKELY(!found)) {
    WARN("VOUT mode %dx%d not found, set to auto mode!",
         size.width,
         size.height);
    mode = AM_VOUT_MODE_AUTO;
  }

  return mode;
}

AMResolution mode_to_resolution(AM_VOUT_MODE mode)
{
  AMResolution size = { 0 };
  uint32_t list_size = sizeof(g_vout_mode_list) / sizeof(g_vout_mode_list[0]);
  bool found = false;
  for (uint32_t i = 0; i < list_size; ++ i) {
    if (AM_LIKELY(mode == g_vout_mode_list[i].mode)) {
      size.width = g_vout_mode_list[i].width;
      size.height = g_vout_mode_list[i].height;
      NOTICE("Get VOUT mode: %d, width: %d, height: %d",
             mode,
             size.width,
             size.height);
      found = true;
      break;
    }
  }
  if (AM_UNLIKELY(!found)) {
    ERROR("VOUT mode not found, please check!");
  }

  return size;
}

uint32_t mode_to_fps(AM_VOUT_MODE mode)
{
  uint32_t fps = AMVideoTrans::fps_to_q9fps(AM_VIDEO_FPS_AUTO);
  uint32_t list_size = sizeof(g_vout_mode_list) / sizeof(g_vout_mode_list[0]);
  uint32_t i;
  for (i = 0; i < list_size; ++ i) {
    if (mode == g_vout_mode_list[i].mode) {
      fps = AMVideoTrans::fps_to_q9fps(g_vout_mode_list[i].video_fps);
      break;
    }
  }
  if (i == list_size) {
    NOTICE("VOUT mode not found! Use auto mode!");
  }
  return fps;
}

} /* namespace AMVoutTrans */

namespace AMVideoTrans
{
static AMVideoFPSTable g_video_fps_list[] =
{
{ "auto", 0, AM_VIDEO_FPS_AUTO },
  { "0", 0, AM_VIDEO_FPS_AUTO },
  { "1", 0, AM_VIDEO_FPS_1 },
  { "2", 0, AM_VIDEO_FPS_2 },
  { "3", 0, AM_VIDEO_FPS_3 },
  { "3.125", 0, AM_VIDEO_FPS_3_125 },
  { "3.75", 0, AM_VIDEO_FPS_3_75 },
  { "4", 0, AM_VIDEO_FPS_4 },
  { "5", 0, AM_VIDEO_FPS_5 },
  { "6", 0, AM_VIDEO_FPS_6 },
  { "6.25", 0, AM_VIDEO_FPS_6_25 },
  { "7", 0, AM_VIDEO_FPS_7 },
  { "7.5", 0, AM_VIDEO_FPS_7_5 },
  { "8", 0, AM_VIDEO_FPS_8 },
  { "9", 0, AM_VIDEO_FPS_9 },
  { "10", 0, AM_VIDEO_FPS_10 },
  { "11", 0, AM_VIDEO_FPS_11 },
  { "12", 0, AM_VIDEO_FPS_12 },
  { "12.5", 0, AM_VIDEO_FPS_12_5 },
  { "13", 0, AM_VIDEO_FPS_13 },
  { "14", 0, AM_VIDEO_FPS_14 },
  { "15", 0, AM_VIDEO_FPS_15 },
  { "16", 0, AM_VIDEO_FPS_16 },
  { "17", 0, AM_VIDEO_FPS_17 },
  { "18", 0, AM_VIDEO_FPS_18 },
  { "19", 0, AM_VIDEO_FPS_19 },
  { "20", 0, AM_VIDEO_FPS_20 },
  { "21", 0, AM_VIDEO_FPS_21 },
  { "22", 0, AM_VIDEO_FPS_22 },
  { "23", 0, AM_VIDEO_FPS_23 },
  { "23.976", 0, AM_VIDEO_FPS_23_976 },
  { "24", 0, AM_VIDEO_FPS_24 },
  { "25", 0, AM_VIDEO_FPS_25 },
  { "26", 0, AM_VIDEO_FPS_26 },
  { "27", 0, AM_VIDEO_FPS_27 },
  { "28", 0, AM_VIDEO_FPS_28 },
  { "29", 0, AM_VIDEO_FPS_29 },
  { "29.97", 0, AM_VIDEO_FPS_29_97 },
  { "30", 0, AM_VIDEO_FPS_30 },
  { "45", 0, AM_VIDEO_FPS_45 },
  { "50", 0, AM_VIDEO_FPS_50 },
  { "59.94", 0, AM_VIDEO_FPS_59_94 },
  { "60", 0, AM_VIDEO_FPS_60 },
  { "90", 0, AM_VIDEO_FPS_90 },
  { "120", 0, AM_VIDEO_FPS_120 },
  { "240", 0, AM_VIDEO_FPS_240 }
};

std::string stream_type_enum_to_str(AM_STREAM_TYPE type)
{
  std::string ret;
  switch (type) {
    case AM_STREAM_TYPE_H264:
      ret = "h264";
      break;
    case AM_STREAM_TYPE_H265:
      ret = "h265";
      break;
    case AM_STREAM_TYPE_MJPEG:
      ret = "mjpeg";
      break;
    case AM_STREAM_TYPE_NONE:
    default:
      ret = "none";
      break;
  }

  return ret;
}

AM_STREAM_TYPE stream_type_str_to_enum(string type)
{
  AM_STREAM_TYPE stream_type = AM_STREAM_TYPE_NONE;
  if (!type.empty()) {
    const char *type_str = type.c_str();
    if (is_str_equal(type_str, "h264") ||
    is_str_equal(type_str, "h.264") ||
    is_str_equal(type_str, "avc")) {
      stream_type = AM_STREAM_TYPE_H264;
    } else if (is_str_equal(type_str, "h265") ||
    is_str_equal(type_str, "h.265") ||
    is_str_equal(type_str, "hevc")) {
      stream_type = AM_STREAM_TYPE_H265;
    } else if (is_str_equal(type_str, "mjpeg")) {
      stream_type = AM_STREAM_TYPE_MJPEG;
    } else {
      stream_type = AM_STREAM_TYPE_NONE;
    }
  }
  return stream_type;
}

AM_VIDEO_FPS fps_str_to_enum(const std::string &fps_str)
{
  AM_VIDEO_FPS fps = AM_VIDEO_FPS_AUTO;
  if (AM_LIKELY(!fps_str.empty())) {
    uint32_t list_size = sizeof(g_video_fps_list) / sizeof(g_video_fps_list[0]);
    bool ok = false;
    for (uint32_t i = 0; i < list_size; ++ i) {
      if (is_str_equal(fps_str.c_str(), g_video_fps_list[i].name)) {
        fps = g_video_fps_list[i].video_fps;
        ok = true;
        break;
      }
    }
    if (AM_UNLIKELY(!ok)) {
      WARN("Invalid FPS: %s! Use default value \"auto\"", fps_str.c_str());
    }
  }

  return fps;
}

std::string flip_enum_to_str(AM_VIDEO_FLIP flip)
{
  std::string ret;
  switch (flip) {
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

std::string fps_enum_to_str(AM_VIDEO_FPS fps)
{
  std::string ret = "auto";
  uint32_t list_size = sizeof(g_video_fps_list) / sizeof(g_video_fps_list[0]);
  for (uint32_t i = 0; i < list_size; ++ i) {
    if (AM_LIKELY(g_video_fps_list[i].video_fps == fps)) {
      ret = g_video_fps_list[i].name;
      break;
    }
  }

  return ret;
}

AM_VIDEO_FLIP flip_str_to_enum(const std::string &flip_str)
{
  AM_VIDEO_FLIP type = AM_VIDEO_FLIP_NONE;

  if (AM_LIKELY(!flip_str.empty())) {
    const char *flip = flip_str.c_str();
    if (is_str_equal(flip, "h-flip")) {
      type = AM_VIDEO_FLIP_HORIZONTAL;
    } else if (is_str_equal(flip, "v-flip")) {
      type = AM_VIDEO_FLIP_VERTICAL;
    } else if (is_str_equal(flip, "both-flip")) {
      type = AM_VIDEO_FLIP_VH_BOTH;
    } else if (is_str_equal(flip, "none")) {
      type = AM_VIDEO_FLIP_NONE;
    } else {
      WARN("Unknown flip type: %s! Use default value \"none\"!", flip);
    }
  }

  return type;
}

string rotate_enum_to_str(AM_VIDEO_ROTATE rotate)
{
  string ret;
  switch (rotate) {
    case AM_VIDEO_ROTATE_NONE:
      ret = "none";
      break;
    case AM_VIDEO_ROTATE_90:
      ret = "rotate-90";
      break;
    default:
      ret = "none";
      break;
  }

  return ret;
}

AM_VIDEO_ROTATE rotate_str_to_enum(const std::string &rotate_str)
{
  AM_VIDEO_ROTATE type = AM_VIDEO_ROTATE_NONE;

  if (AM_LIKELY(!rotate_str.empty())) {
    const char *rotate = rotate_str.c_str();
    if (is_str_equal(rotate, "rotate-90")) {
      type = AM_VIDEO_ROTATE_90;
    } else if (is_str_equal(rotate, "none")) {
      type = AM_VIDEO_ROTATE_NONE;
    } else {
      WARN("Unknown rotation type: %s! Use default value \"none\"!", rotate);
    }
  }

  return type;
}

AM_RATE_CONTROL stream_bitrate_control_str_to_enum(const std::string &bc_str)
{
  AM_RATE_CONTROL ret = AM_RC_CBR;

  if (AM_LIKELY(!bc_str.empty())) {
    const char *bc = bc_str.c_str();
    if (is_str_equal(bc, "cbr")) {
      ret = AM_RC_CBR;
    } else if (is_str_equal(bc, "vbr")) {
      ret = AM_RC_VBR;
    } else if (is_str_equal(bc, "cbr2")) {
      ret = AM_RC_CBR2;
    } else if (is_str_equal(bc, "vbr2")) {
      ret = AM_RC_VBR2;
    } else if (is_str_equal(bc, "cbr_quality")) {
      ret = AM_RC_CBR_QUALITY;
    } else if (is_str_equal(bc, "vbr_quality")) {
      ret = AM_RC_VBR_QUALITY;
    } else {
      WARN("Invalid rate control type: %s! Use default value \"CBR\"!", bc);
      ret = AM_RC_CBR;
    }
  }
  return ret;
}

string stream_bitrate_control_enum_to_str(AM_RATE_CONTROL rate_control)
{
  std::string ret;
  switch (rate_control) {
    case AM_RC_VBR:
      ret = "vbr";
      break;
    case AM_RC_CBR_QUALITY:
      ret = "cbr_quality";
      break;
    case AM_RC_VBR_QUALITY:
      ret = "vbr_quality";
      break;
    case AM_RC_CBR2:
      ret = "cbr2";
      break;
    case AM_RC_VBR2:
      ret = "vbr2";
      break;
    case AM_RC_CBR:
    default:
      ret = "cbr";
      break;
  }
  return ret;
}

string stream_profile_enum_to_str(AM_PROFILE profile_level)
{
  std::string ret;
  switch (profile_level) {
    case AM_PROFILE_BASELINE:
      ret = "baseline";
      break;
    case AM_PROFILE_MAIN:
      ret = "main";
      break;
    case AM_PROFILE_HIGH:
    default:
      ret = "high";
      break;
  }
  return ret;
}

AM_PROFILE stream_profile_str_to_enum(const string &profile_str)
{
  AM_PROFILE ret = AM_PROFILE_MAIN;
  if (AM_LIKELY(!profile_str.empty())) {
    const char *profile = profile_str.c_str();
    if (is_str_equal(profile, "baseline")) {
      ret = AM_PROFILE_BASELINE;
    } else if (is_str_equal(profile, "main")) {
      ret = AM_PROFILE_MAIN;
    } else if (is_str_equal(profile, "high")) {
      ret = AM_PROFILE_HIGH;
    } else {
      WARN("Invalid profile type: %s! Reset to main profile!", profile);
      ret = AM_PROFILE_MAIN;
    }
  }
  return ret;
}

AM_CHROMA_FORMAT stream_chroma_str_to_enum(const std::string &chroma_str)
{
  AM_CHROMA_FORMAT ret = AM_CHROMA_FORMAT_YUV420;
  if (AM_LIKELY(!chroma_str.empty())) {
    const char *chroma = chroma_str.c_str();
    if (is_str_equal(chroma, "420")) {
      ret = AM_CHROMA_FORMAT_YUV420;
    }
#if 0
    //422 is not supported
    else if (is_str_equal(chroma, "422")) {
      ret = AM_CHROMA_FORMAT_YUV422;
    }
#endif
    else if (is_str_equal(chroma, "mono")) {
      ret = AM_CHROMA_FORMAT_Y8;
    } else {
      WARN("Invalid chroma type: %s! Use default value \"420\"!", chroma);
      ret = AM_CHROMA_FORMAT_YUV420;
    }
  }

  return ret;
}

std::string stream_chroma_to_str(AM_CHROMA_FORMAT chroma_format)
{
  std::string ret;
  switch (chroma_format) {
#if 0
    case AM_CHROMA_FORMAT_YUV422:
      ret = "422";
      break;
#endif
    case AM_CHROMA_FORMAT_Y8:
      ret = "mono";
      break;
    case AM_CHROMA_FORMAT_YUV420:
    default:
      ret = "420";
      break;
  }
  return ret;
}

AM_SOURCE_BUFFER_TYPE buffer_type_str_to_enum(const std::string &type_str)
{
  AM_SOURCE_BUFFER_TYPE buf_type = AM_SOURCE_BUFFER_TYPE_OFF;

  if (AM_LIKELY(!type_str.empty())) {
    const char *type = type_str.c_str();
    if (is_str_equal(type, "off")) {
      buf_type = AM_SOURCE_BUFFER_TYPE_OFF;
    } else if (is_str_equal(type, "encode")) {
      buf_type = AM_SOURCE_BUFFER_TYPE_ENCODE;
    } else if (is_str_equal(type, "preview")) {
      buf_type = AM_SOURCE_BUFFER_TYPE_PREVIEW;
    } else {
      WARN("Invalid buffer type: %s! Use default value \"off\"!", type);
      buf_type = AM_SOURCE_BUFFER_TYPE_OFF;
    }
  }

  return buf_type;
}

int get_hdr_expose_num(AM_HDR_TYPE hdr_type)
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
    case AM_HDR_SINGLE_EXPOSURE:
    case AM_HDR_SENSOR_INTERNAL:
    default:
      num = 1;
      break;
  }

  return num;
}

std::string buffer_type_to_str(AM_SOURCE_BUFFER_TYPE buf_type)
{
  std::string ret;
  switch (buf_type) {
    case AM_SOURCE_BUFFER_TYPE_ENCODE:
      ret = "encode";
      break;
    case AM_SOURCE_BUFFER_TYPE_PREVIEW:
      ret = "preview";
      break;
    case AM_SOURCE_BUFFER_TYPE_OFF:
    default:
      ret = "off";
      break;
  }
  return ret;
}

string iav_state_to_str(AM_IAV_STATE state)
{
  string ret;
  switch (state) {
    case AM_IAV_STATE_INIT:
      ret = "init";
      break;
    case AM_IAV_STATE_IDLE:
      ret = "idle";
      break;
    case AM_IAV_STATE_PREVIEW:
      ret = "preview";
      break;
    case AM_IAV_STATE_PREVIEW_LOW:
      ret = "preview low";
      break;
    case AM_IAV_STATE_ENCODING:
      ret = "encoding";
      break;
    case AM_IAV_STATE_EXITING_PREVIEW:
      ret = "exiting preview";
      break;
    case AM_IAV_STATE_DECODING:
      ret = "decoding!";
      break;
    default:
      ret = "unknown";
      break;
  }
  return ret;
}

string stream_state_to_str(AM_STREAM_STATE state)
{
  string ret;
  switch(state) {
    case AM_STREAM_STATE_IDLE:
      ret = "idle";
      break;
    case AM_STREAM_STATE_STARTING:
      ret = "starting";
      break;
    case AM_STREAM_STATE_ENCODING:
      ret = "encoding";
      break;
    case AM_STREAM_STATE_STOPPING:
      ret = "stopping";
      break;
    case AM_STREAM_STATE_ERROR:
      ret = "error";
      break;
    default:
      ret = "unknown";
      break;
  }

  return ret;
}

AMVideoFpsFormatQ9 fps_to_q9fps(AM_VIDEO_FPS fps)
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
      //all other enum values are same as fps number
    default:
      amba_fps = ROUND_DIV(512000000UL, fps);
      break;
  }
  return amba_fps;
}

AM_VIDEO_FPS q9fps_to_fps(AMVideoFpsFormatQ9 q9fps)
{
  AM_VIDEO_FPS fps = AM_VIDEO_FPS_AUTO;
  switch (q9fps) {
    case AMBA_VIDEO_FPS_AUTO:
      fps = AM_VIDEO_FPS_AUTO;
      break;
    case AMBA_VIDEO_FPS_29_97:
      fps = AM_VIDEO_FPS_29_97;
      break;
    case AMBA_VIDEO_FPS_59_94:
      fps = AM_VIDEO_FPS_59_94;
      break;
    case AMBA_VIDEO_FPS_23_976:
      fps = AM_VIDEO_FPS_23_976;
      break;
    case AMBA_VIDEO_FPS_12_5:
      fps = AM_VIDEO_FPS_12_5;
      break;
    case AMBA_VIDEO_FPS_6_25:
      fps = AM_VIDEO_FPS_6_25;
      break;
    case AMBA_VIDEO_FPS_3_125:
      fps = AM_VIDEO_FPS_3_125;
      break;
    case AMBA_VIDEO_FPS_7_5:
      fps = AM_VIDEO_FPS_7_5;
      break;
    case AMBA_VIDEO_FPS_3_75:
      fps = AM_VIDEO_FPS_3_75;
      break;
    default:
      fps = AM_VIDEO_FPS(ROUND_DIV(512000000, q9fps));
      break;
  }

  return fps;
}

void fps_to_factor(AM_VIDEO_FPS vin_fps, int32_t encode_fps,
                   int32_t &mul, int32_t &div)
{
  mul = encode_fps;
  switch(vin_fps) {
    case AM_VIDEO_FPS_29_97:
      div = 30;
      break;
    case AM_VIDEO_FPS_59_94:
      div = 60;
      break;
    case AM_VIDEO_FPS_23_976:
      div = 24;
      break;
    case AM_VIDEO_FPS_12_5:
      mul *= 2;
      div = 25;
      break;
    case AM_VIDEO_FPS_6_25:
      mul *= 4;
      div = 25;
      break;
    case AM_VIDEO_FPS_3_125:
      mul *= 8;
      div = 25;
      break;
    case AM_VIDEO_FPS_7_5:
      mul *= 2;
      div = 15;
      break;
    case AM_VIDEO_FPS_3_75:
      mul *= 4;
      div = 15;
      break;
    case AM_VIDEO_FPS_AUTO:
      div = 30;
      break;
    default:
      div = vin_fps;
      break;
  }

  if ((mul == 0) || (mul > div)) {
    mul = div;
  }
}

int32_t factor_to_encfps(AM_VIDEO_FPS vin_fps, int32_t mul, int32_t div)
{
  int32_t fps = -1;
  switch(vin_fps) {
    case AM_VIDEO_FPS_29_97:
      vin_fps = AM_VIDEO_FPS_30;
      break;
    case AM_VIDEO_FPS_59_94:
      vin_fps = AM_VIDEO_FPS_60;
      break;
    case AM_VIDEO_FPS_23_976:
      vin_fps = AM_VIDEO_FPS_24;
      break;
    case AM_VIDEO_FPS_12_5:
      div *= 2;
      vin_fps = AM_VIDEO_FPS_25;
      break;
    case AM_VIDEO_FPS_6_25:
      div *= 4;
      vin_fps = AM_VIDEO_FPS_25;
      break;
    case AM_VIDEO_FPS_3_125:
      div *= 8;
      vin_fps = AM_VIDEO_FPS_25;
      break;
    case AM_VIDEO_FPS_7_5:
      div *= 2;
      vin_fps = AM_VIDEO_FPS_15;
      break;
    case AM_VIDEO_FPS_3_75:
      div *= 4;
      vin_fps = AM_VIDEO_FPS_15;
      break;
      break;
    default:
      break;
  }
  if (div != 0) {
    fps = int32_t((mul * vin_fps) / div);
  }

  return fps;
}
} /* namespace AMVideoTrans */
