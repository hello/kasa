/*******************************************************************************
 * am_platform_iav2_utility.cpp
 *
 * History:
 *   Aug 12, 2015 - [ypchang] created file
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

#include "am_platform_iav2_utility.h"

namespace AMVinTransIav2
{
amba_vindev_mirror_pattern_e flip_mw_to_iav(AM_VIDEO_FLIP flip)
{
  amba_vindev_mirror_pattern_e pattern;
  switch (flip) {
    case AM_VIDEO_FLIP_NONE: {
      pattern = VINDEV_MIRROR_NONE;
    }break;
    case AM_VIDEO_FLIP_VERTICAL: {
      pattern = VINDEV_MIRROR_VERTICALLY;
    }break;
    case AM_VIDEO_FLIP_HORIZONTAL: {
      pattern = VINDEV_MIRROR_HORRIZONTALLY;
    }break;
    case AM_VIDEO_FLIP_VH_BOTH: {
      pattern = VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY;
    }break;
    case AM_VIDEO_FLIP_AUTO:
    default: {
      pattern = VINDEV_MIRROR_AUTO;
    }break;
  }

  return pattern;
}

AM_VIDEO_FLIP flip_iav_to_mw(amba_vindev_mirror_pattern_e mirror)
{
  AM_VIDEO_FLIP flip;
  switch (mirror) {
    case VINDEV_MIRROR_NONE: {
      flip = AM_VIDEO_FLIP_NONE;
    }break;
    case VINDEV_MIRROR_VERTICALLY: {
      flip = AM_VIDEO_FLIP_VERTICAL;
    }break;
    case VINDEV_MIRROR_HORRIZONTALLY: {
      flip = AM_VIDEO_FLIP_HORIZONTAL;
    }break;
    case VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY: {
      flip = AM_VIDEO_FLIP_VH_BOTH;
    }break;
    case VINDEV_MIRROR_AUTO:
    default: {
      flip = AM_VIDEO_FLIP_AUTO;
    }break;
  }

  return flip;
}

amba_video_mode mode_mw_to_iav(AM_VIN_MODE mode)
{
  amba_video_mode video_mode;
  switch (mode) {
    case AM_VIN_MODE_OFF:
      video_mode = AMBA_VIDEO_MODE_OFF;
      break;
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
    case AM_VIN_MODE_1920X1200:
      video_mode = AMBA_VIDEO_MODE_WUXGA;
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
    case AM_VIN_MODE_CURRENT:
      video_mode = AMBA_VIDEO_MODE_CURRENT;
      break;
    default:
      WARN("Video mode %d not supported yet, use auto!", mode);
      video_mode = AMBA_VIDEO_MODE_AUTO;
      break;
  }

  return video_mode;
}

AM_VIN_MODE mode_iav_to_mw(amba_video_mode mode)
{
  AM_VIN_MODE video_mode;
  switch (mode) {
    case AMBA_VIDEO_MODE_OFF:
      video_mode = AM_VIN_MODE_OFF;
      break;
    case AMBA_VIDEO_MODE_AUTO:
      video_mode = AM_VIN_MODE_AUTO;
      break;
    case AMBA_VIDEO_MODE_VGA:
      video_mode = AM_VIN_MODE_VGA;
      break;
    case AMBA_VIDEO_MODE_720P:
      video_mode = AM_VIN_MODE_720P;
      break;
    case AMBA_VIDEO_MODE_1280_960:
      video_mode = AM_VIN_MODE_960P;
      break;
    case AMBA_VIDEO_MODE_SXGA:
      video_mode = AM_VIN_MODE_1024P;
      break;
    case AMBA_VIDEO_MODE_1080P:
      video_mode = AM_VIN_MODE_1080P;
      break;
    case AMBA_VIDEO_MODE_QXGA:
      video_mode = AM_VIN_MODE_3M_4_3;
      break;
    case AMBA_VIDEO_MODE_2304_1296:
      video_mode = AM_VIN_MODE_3M_16_9;
      break;
    case AMBA_VIDEO_MODE_2560_1440: //2560x1440
      video_mode = AM_VIN_MODE_QHD;
      break;
    case AMBA_VIDEO_MODE_2688_1512: //2688x1512
      video_mode = AM_VIN_MODE_4M_16_9;
      break;
    case AMBA_VIDEO_MODE_QSXGA:
      video_mode = AM_VIN_MODE_5M_4_3;
      break;
    case AMBA_VIDEO_MODE_3072_2048:
      video_mode = AM_VIN_MODE_6M_3_2;
      break;
    case AMBA_VIDEO_MODE_3840_2160:
      video_mode = AM_VIN_MODE_QFHD;
      break;
    case AMBA_VIDEO_MODE_4096_2160:
      video_mode = AM_VIN_MODE_4096X2160;
      break;
    case AMBA_VIDEO_MODE_4000_3000:
      video_mode = AM_VIN_MODE_12M;
      break;
    case AMBA_VIDEO_MODE_CURRENT:
      video_mode = AM_VIN_MODE_CURRENT;
      break;
    default:
      WARN("Video mode %d not supported yet, use auto!", mode);
      video_mode = AM_VIN_MODE_AUTO;
      break;
  }
  return video_mode;
}

int type_mw_to_iav(AM_VIN_TYPE type)
{
  int vin_type = VINDEV_TYPE_SENSOR;
  switch (type) {
    case AM_VIN_TYPE_SENSOR:
      vin_type = VINDEV_TYPE_SENSOR;
      break;
    case AM_VIN_TYPE_DECODE:
      vin_type = VINDEV_TYPE_DECODER;
      break;
    default:
      WARN("Unknown VIN type: %s! Use default value \"sensor\"", type);
      break;
  }
  return vin_type;
}

AM_VIN_TYPE type_iav_to_mw(int type)
{
  AM_VIN_TYPE vin_type = AM_VIN_TYPE_SENSOR;
  switch (type) {
    case VINDEV_TYPE_SENSOR:
      vin_type = AM_VIN_TYPE_SENSOR;
      break;
    case VINDEV_TYPE_DECODER:
      vin_type = AM_VIN_TYPE_DECODE;
      break;
    default:
      WARN("Unknown VIN type: %s! Use default value \"sensor\"", type);
      break;
  }
  return vin_type;
}

int sensor_type_mw_to_iav(AM_SENSOR_TYPE type)
{
  return 0;
}

AM_SENSOR_TYPE sensor_type_iav_to_mw(int type)
{
  return AM_SENSOR_TYPE_RGB;
}

} /* namespace AMVinTransIav2 */

namespace AMVoutTransIav2
{
amba_vout_sink_type sink_type_mw_to_iav(AM_VOUT_TYPE  type)
{
  amba_vout_sink_type ret = AMBA_VOUT_SINK_TYPE_AUTO;

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
    case AM_VOUT_TYPE_YPBPR:
      ret = AMBA_VOUT_SINK_TYPE_YPBPR;
      break;
    case AM_VOUT_TYPE_NONE:
      ret = AMBA_VOUT_SINK_TYPE_AUTO;
      break;
    default:
      ret = AMBA_VOUT_SINK_TYPE_AUTO;
      break;
  }

  return ret;
}

uint32_t video_type_mw_to_iav(AM_VOUT_VIDEO_TYPE  type)
{
  uint32_t ret = AMBA_VIDEO_TYPE_AUTO;

  switch (type) {
    case AM_VOUT_VIDEO_TYPE_YUV601:
      ret = AMBA_VIDEO_TYPE_YUV_601;
      break;
    case AM_VOUT_VIDEO_TYPE_YUV656:
      ret = AMBA_VIDEO_TYPE_YUV_656;
      break;
    case AM_VOUT_VIDEO_TYPE_YUV1120:
      ret = AMBA_VIDEO_TYPE_YUV_BT1120;
      break;
    case AM_VOUT_VIDEO_TYPE_RGB601:
      ret = AMBA_VIDEO_TYPE_RGB_601;
      break;
    case AM_VOUT_VIDEO_TYPE_RGB656:
      ret = AMBA_VIDEO_TYPE_RGB_656;
      break;
    case AM_VOUT_VIDEO_TYPE_RGB1120:
      ret = AMBA_VIDEO_TYPE_RGB_BT1120;
      break;
    case AM_VOUT_VIDEO_TYPE_NONE:
      ret = AMBA_VIDEO_TYPE_AUTO;
      break;
    default:
      ret = AMBA_VIDEO_TYPE_AUTO;
      break;
  }

  return ret;
}

int32_t id_mw_to_iav(AM_VOUT_ID id)
{
  int32_t ret = 1; //the valud used in iav for VOUT source id
  switch (id) {
    case AM_VOUT_A:
      ret = 0;
      break;
    case AM_VOUT_B:
      ret = 1;
      break;
    default:
      ret = 1;
      break;
  }

  return ret;
}

amba_video_mode mode_mw_to_iav(AM_VOUT_MODE mode)
{
  amba_video_mode ret = AMBA_VIDEO_MODE_AUTO;
  switch (mode) {
    case AM_VOUT_MODE_480P:
      ret = AMBA_VIDEO_MODE_D1_NTSC;
      break;
    case AM_VOUT_MODE_576P:
      ret = AMBA_VIDEO_MODE_D1_PAL;
      break;
    case AM_VOUT_MODE_480I:
      ret = AMBA_VIDEO_MODE_480I;
      break;
    case AM_VOUT_MODE_576I:
      ret = AMBA_VIDEO_MODE_576I;
      break;
    case AM_VOUT_MODE_720P60:
      ret = AMBA_VIDEO_MODE_720P;
      break;
    case AM_VOUT_MODE_720P50:
      ret = AMBA_VIDEO_MODE_720P50;
      break;
    case AM_VOUT_MODE_720P30:
      ret = AMBA_VIDEO_MODE_720P30;
      break;
    case AM_VOUT_MODE_720P25:
      ret = AMBA_VIDEO_MODE_720P25;
      break;
    case AM_VOUT_MODE_1080P60:
      ret = AMBA_VIDEO_MODE_1080P;
      break;
    case AM_VOUT_MODE_1080P50:
      ret = AMBA_VIDEO_MODE_1080P50;
      break;
    case AM_VOUT_MODE_1080P30:
      ret = AMBA_VIDEO_MODE_1080P30;
      break;
    case AM_VOUT_MODE_1080P25:
      ret = AMBA_VIDEO_MODE_1080P25;
      break;
    case AM_VOUT_MODE_1080P24:
      ret = AMBA_VIDEO_MODE_1080P24;
      break;
    case AM_VOUT_MODE_1080I60:
      ret = AMBA_VIDEO_MODE_1080I;
      break;
    case AM_VOUT_MODE_1080I50:
      ret = AMBA_VIDEO_MODE_1080I50;
      break;
    case AM_VOUT_MODE_QFHD30:
      ret = AMBA_VIDEO_MODE_2160P30;
      break;
    case AM_VOUT_MODE_QFHD24:
      ret = AMBA_VIDEO_MODE_2160P24;
      break;
    case AM_VOUT_MODE_AUTO:
      ret = AMBA_VIDEO_MODE_AUTO;
      break;
    default:
      ret = AMBA_VIDEO_MODE_AUTO;
      break;
  }

  return ret;
}

amba_vout_flip_info flip_mw_to_iav(AM_VIDEO_FLIP flip)
{
  amba_vout_flip_info ret = AMBA_VOUT_FLIP_NORMAL;;
  switch (flip) {
    case AM_VIDEO_FLIP_NONE:
      ret = AMBA_VOUT_FLIP_NORMAL;
      break;
    case AM_VIDEO_FLIP_VERTICAL:
      ret = AMBA_VOUT_FLIP_VERTICAL;
      break;
    case AM_VIDEO_FLIP_HORIZONTAL:
      ret = AMBA_VOUT_FLIP_HORIZONTAL;
      break;
    case AM_VIDEO_FLIP_VH_BOTH:
      ret = AMBA_VOUT_FLIP_HV;
      break;
    default:
      ret = AMBA_VOUT_FLIP_NORMAL;
      break;
  }

  return ret;
}

amba_vout_rotate_info rotate_mw_to_iav(AM_VIDEO_ROTATE rotate)
{
  amba_vout_rotate_info ret = AMBA_VOUT_ROTATE_NORMAL;

    switch (rotate) {
      case AM_VIDEO_ROTATE_NONE:
        ret = AMBA_VOUT_ROTATE_NORMAL;
        break;
      case AM_VIDEO_ROTATE_90:
        ret = AMBA_VOUT_ROTATE_90;
        break;
      default:
        ret = AMBA_VOUT_ROTATE_NORMAL;
        break;
    }

    return ret;
}

} /* namespace AMVoutTransIav2 */


namespace AMVideoTransIav2
{
AM_ENCODE_MODE camera_param_mode_str_to_enum(const std::string &mode_str)
{
  AM_ENCODE_MODE encode_mode = AM_ENCODE_INVALID_MODE;
  if (AM_LIKELY(!mode_str.empty())) {
    const char *mode = mode_str.c_str();
    if (is_str_equal(mode, "mode0")) {
      encode_mode = AM_ENCODE_MODE_0;
    } else if (is_str_equal(mode, "mode1")) {
      encode_mode = AM_ENCODE_MODE_1;
    } else if (is_str_equal(mode, "mode2")) {
      encode_mode = AM_ENCODE_MODE_2;
    } else if (is_str_equal(mode, "mode3")) {
      encode_mode = AM_ENCODE_MODE_3;
    } else if (is_str_equal(mode, "mode4")) {
      encode_mode = AM_ENCODE_MODE_4;
    } else if (is_str_equal(mode, "mode5")) {
      encode_mode = AM_ENCODE_MODE_5;
    } else if (is_str_equal(mode, "mode6")) {
      encode_mode = AM_ENCODE_MODE_6;
    } else if (is_str_equal(mode, "mode7")) {
      encode_mode = AM_ENCODE_MODE_7;
    } else if (is_str_equal(mode, "auto")) {
      encode_mode = AM_ENCODE_MODE_AUTO;
    } else {
      ERROR("Invalid mode: %s!", mode);
    }
  }

  return encode_mode;
}

string camera_param_hdr_enum_to_str(AM_HDR_TYPE type)
{
  string hdr_type;
  switch (type) {
    case AM_HDR_2_EXPOSURE:
      hdr_type = "2x";
      break;
    case AM_HDR_3_EXPOSURE:
      hdr_type = "3x";
      break;
    case AM_HDR_4_EXPOSURE:
      hdr_type = "4x";
      break;
    case AM_HDR_SENSOR_INTERNAL:
      hdr_type = "sensor";
      break;
    default:
      hdr_type = "none";
      break;
  }

  return hdr_type;
}

AM_HDR_TYPE camera_param_hdr_str_to_enum(const string &type)
{
  AM_HDR_TYPE hdr_type = AM_HDR_SINGLE_EXPOSURE;

  if (!type.empty()) {
    const char *hdr = type.c_str();
    if (is_str_equal(hdr, "2x")) {
      hdr_type = AM_HDR_2_EXPOSURE;
    } else if (is_str_equal(hdr, "3x")) {
      hdr_type = AM_HDR_3_EXPOSURE;
    } else if (is_str_equal(hdr, "4x")) {
      hdr_type = AM_HDR_4_EXPOSURE;
    }else if (is_str_equal(hdr, "sensor")) {
      hdr_type = AM_HDR_SENSOR_INTERNAL;
    }
  }

  return hdr_type;
}

AM_IMAGE_ISO_TYPE camera_param_iso_str_to_enum(const std::string &iso_str)
{
  AM_IMAGE_ISO_TYPE iso_type = AM_IMAGE_NORMAL_ISO;
  if (AM_LIKELY(!iso_str.empty())) {
    const char *iso = iso_str.c_str();
    if (is_str_equal(iso, "plus")) {
      iso_type = AM_IMAGE_ISO_PLUS;
    } else if (is_str_equal(iso, "advanced")) {
      iso_type = AM_IMAGE_ADVANCED_ISO;
    } else if (is_str_equal(iso, "normal")) {
      iso_type = AM_IMAGE_NORMAL_ISO;
    } else {
      WARN("Invalid ISO type: %s! Use default value \"normal\"!");
      iso_type = AM_IMAGE_NORMAL_ISO;
    }
  }
  return iso_type;
}

AM_DEWARP_FUNC_TYPE camera_param_dewarp_str_to_enum(const std::string &dewarp_str)
{
  AM_DEWARP_FUNC_TYPE dewarp_type = AM_DEWARP_NONE;
  if (AM_LIKELY(!dewarp_str.empty())) {
    const char *dewarp = dewarp_str.c_str();
    if (is_str_equal(dewarp, "ldc")) {
      dewarp_type = AM_DEWARP_LDC;
    } else if (is_str_equal(dewarp, "eis")) {
      dewarp_type = AM_DEWARP_EIS;
    } else if (is_str_equal(dewarp, "none")) {
      dewarp_type = AM_DEWARP_NONE;
    } else {
      WARN("Invalid dewarp type: %s! Use default value \"none\"!");
      dewarp_type = AM_DEWARP_NONE;
    }
  }

  return dewarp_type;
}

AM_DPTZ_TYPE camera_param_dptz_str_to_enum(const std::string &dptz_str)
{
  AM_DPTZ_TYPE dptz_type = AM_DPTZ_DISABLE;
  if (AM_LIKELY(!dptz_str.empty())) {
    const char *dptz = dptz_str.c_str();
    if (is_str_equal(dptz, "enable") || is_str_equal(dptz, "yes") ||
        is_str_equal(dptz, "on") || is_str_equal(dptz, "true")) {
      dptz_type = AM_DPTZ_ENABLE;
    } else if (is_str_equal(dptz, "disable") || is_str_equal(dptz, "no") ||
               is_str_equal(dptz, "off") || is_str_equal(dptz, "false") ||
               is_str_equal(dptz, "none")) {
      dptz_type = AM_DPTZ_DISABLE;
    } else {
      WARN("Invalid dptz type: %s! Use default value \"disable\"!");
      dptz_type = AM_DPTZ_DISABLE;
    }
  }

  return dptz_type;
}

string bitrate_ctrl_method_enum_to_str(AM_BITRATE_CTRL_METHOD bitrate_ctrl)
{
  std::string ret = "none";
  switch(bitrate_ctrl) {
    case AM_BITRATE_CTRL_LBR:
      ret = "lbr";
      break;
    case AM_BITRATE_CTRL_SMARTRC:
      ret = "smartrc";
      break;
    case AM_BITRATE_CTRL_NONE:
    default:
      ret = "none";
      break;
  }

  return ret;
}

AM_BITRATE_CTRL_METHOD bitrate_ctrl_method_str_to_enum(const string &str)
{
  AM_BITRATE_CTRL_METHOD method = AM_BITRATE_CTRL_NONE;

  if (is_str_equal(str.c_str(), "lbr")) {
    method = AM_BITRATE_CTRL_LBR;
  }else if (is_str_equal(str.c_str(), "smartrc")) {
    method = AM_BITRATE_CTRL_SMARTRC;
  } else if (is_str_equal(str.c_str(), "none")) {
    method = AM_BITRATE_CTRL_NONE;
  } else {
    WARN("Unknown bitrate control method: %s", str.c_str());
    method = AM_BITRATE_CTRL_NONE;
  }

  return method;
}

AM_OVERLAY_TYPE camera_param_overlay_str_to_enum(const std::string &overlay_str)
{
  AM_OVERLAY_TYPE overlay_type = AM_OVERLAY_PLUGIN_DISABLE;
  if (AM_LIKELY(!overlay_str.empty())) {
    const char *overlay = overlay_str.c_str();
    if (is_str_equal(overlay, "enable") || is_str_equal(overlay, "yes") ||
        is_str_equal(overlay, "on") || is_str_equal(overlay, "true")) {
      overlay_type = AM_OVERLAY_PLUGIN_ENABLE;
    } else if (is_str_equal(overlay, "disable") || is_str_equal(overlay, "no")
             || is_str_equal(overlay, "off") || is_str_equal(overlay, "false")
             || is_str_equal(overlay, "none")) {
      overlay_type = AM_OVERLAY_PLUGIN_DISABLE;
    } else {
      WARN("Invalid overlay type: %s! Use default value \"disable\"!");
      overlay_type = AM_OVERLAY_PLUGIN_DISABLE;
    }
  }

  return overlay_type;
}

AM_VIDEO_MD_TYPE camera_param_md_str_to_enum(const std::string &md_str)
{
  AM_VIDEO_MD_TYPE md_type = AM_MD_PLUGIN_DISABLE;
  if (AM_LIKELY(!md_str.empty())) {
    const char *md = md_str.c_str();
    if (is_str_equal(md, "enable") || is_str_equal(md, "yes") ||
        is_str_equal(md, "on") || is_str_equal(md, "true")) {
      md_type = AM_MD_PLUGIN_ENABLE;
    } else if (is_str_equal(md, "disable") || is_str_equal(md, "no")
             || is_str_equal(md, "off") || is_str_equal(md, "false")
             || is_str_equal(md, "none")) {
      md_type = AM_MD_PLUGIN_DISABLE;
    } else {
      WARN("Invalid video_md type: %s! Use default value \"disable\"!");
      md_type = AM_MD_PLUGIN_DISABLE;
    }
  }

  return md_type;
}

std::string camera_param_mode_enum_to_str(AM_ENCODE_MODE mode)
{
  std::string ret;
  switch (mode) {
    case AM_ENCODE_MODE_0:    ret = "mode0"; break;
    case AM_ENCODE_MODE_1:    ret = "mode1"; break;
    case AM_ENCODE_MODE_2:    ret = "mode2"; break;
    case AM_ENCODE_MODE_4:    ret = "mode4"; break;
    case AM_ENCODE_MODE_5:    ret = "mode5"; break;
    case AM_ENCODE_MODE_6:    ret = "mode6"; break;
    case AM_ENCODE_MODE_7:    ret = "mode7"; break;
    default:                  ret = "auto";  break;
  }
  return ret;
}

std::string camera_param_iso_enum_to_str(AM_IMAGE_ISO_TYPE iso)
{
   std::string ret;
    switch (iso) {
      case AM_IMAGE_ISO_PLUS:     ret = "plus";     break;
      case AM_IMAGE_ADVANCED_ISO: ret = "advanced"; break;
      default:                    ret = "normal";   break;
    }
    return ret;
}

std::string camera_param_dewarp_enum_to_str(AM_DEWARP_FUNC_TYPE dewarp)
{
   std::string ret;
    switch (dewarp) {
      case AM_DEWARP_LDC:  ret = "ldc";  break;
      case AM_DEWARP_EIS:  ret = "eis"; break;
      case AM_DEWARP_NONE:
      default:             ret = "none"; break;
    }
    return ret;
}

std::string camera_param_dptz_enum_to_str(AM_DPTZ_TYPE dptz)
{
   std::string ret;
    switch (dptz) {
      case AM_DPTZ_ENABLE:  ret = "enable";  break;
      case AM_DPTZ_DISABLE:
      default:              ret = "disable"; break;
    }
    return ret;
}

std::string camera_param_overlay_enum_to_str(AM_OVERLAY_TYPE overlay)
{
   std::string ret;
    switch (overlay) {
      case AM_OVERLAY_PLUGIN_ENABLE:  ret = "enable";  break;
      case AM_OVERLAY_PLUGIN_DISABLE:
      default:                        ret = "disable"; break;
    }
    return ret;
}

string camera_param_md_enum_to_str(AM_VIDEO_MD_TYPE md)
{
  std::string ret;
  switch (md) {
      case AM_MD_PLUGIN_ENABLE: ret = "enable"; break;
      case AM_MD_PLUGIN_DISABLE:
      default:                  ret = "disable"; break;
  }
  return ret;
}

iav_srcbuf_type buffer_type_mw_to_iav(AM_SOURCE_BUFFER_TYPE type)
{
  iav_srcbuf_type buffer_type = IAV_SRCBUF_TYPE_OFF;

  switch (type) {
    case AM_SOURCE_BUFFER_TYPE_ENCODE:
      buffer_type = IAV_SRCBUF_TYPE_ENCODE;
      break;
    case AM_SOURCE_BUFFER_TYPE_PREVIEW:
      buffer_type = IAV_SRCBUF_TYPE_PREVIEW;
      break;
    case AM_SOURCE_BUFFER_TYPE_OFF:
      buffer_type = IAV_SRCBUF_TYPE_OFF;
      break;
    default:
      WARN("Invalid buffer type: %d! Use default value \"off\"!", type);
      break;
  }

  return buffer_type;
}

AM_SOURCE_BUFFER_TYPE buffer_type_iav_to_mw(iav_srcbuf_type type)
{
  AM_SOURCE_BUFFER_TYPE buffer_type = AM_SOURCE_BUFFER_TYPE_OFF;

  switch (type) {
    case IAV_SRCBUF_TYPE_ENCODE:
      buffer_type = AM_SOURCE_BUFFER_TYPE_ENCODE;
      break;
    case IAV_SRCBUF_TYPE_PREVIEW:
      buffer_type = AM_SOURCE_BUFFER_TYPE_PREVIEW;
      break;
    case IAV_SRCBUF_TYPE_OFF:
      buffer_type = AM_SOURCE_BUFFER_TYPE_OFF;
      break;
    default:
      WARN("Invalid buffer type: %d! Use default value \"off\"!", type);
      break;
  }

  return buffer_type;
}

iav_srcbuf_id buffer_id_mw_to_iav(AM_SOURCE_BUFFER_ID id)
{
  iav_srcbuf_id buf_id = IAV_SRCBUF_MN;

  switch (id) {
    case AM_SOURCE_BUFFER_MAIN:
      buf_id = IAV_SRCBUF_1;
      break;
    case AM_SOURCE_BUFFER_2ND:
    //case AM_SOURCE_BUFFER_PREVIEW_C:
      buf_id = IAV_SRCBUF_2;
      break;
    case AM_SOURCE_BUFFER_3RD:
    //case AM_SOURCE_BUFFER_PREVIEW_B:
      buf_id = IAV_SRCBUF_3;
      break;
    case AM_SOURCE_BUFFER_4TH:
    //case AM_SOURCE_BUFFER_PREVIEW_A:
      buf_id = IAV_SRCBUF_4;
      break;
    case AM_SOURCE_BUFFER_PMN:
      buf_id = IAV_SRCBUF_PMN;
      break;
    case AM_SOURCE_BUFFER_EFM:
      buf_id = IAV_SRCBUF_EFM;
      break;
    default:
      WARN("Invalid buffer id: %d! Use default value \"off\"!", id);
      break;
  }

  return buf_id;
}

AM_SOURCE_BUFFER_ID buffer_id_iav_to_mw(iav_srcbuf_id id)
{
  AM_SOURCE_BUFFER_ID buf_id = AM_SOURCE_BUFFER_INVALID;

  switch (id) {
    case IAV_SRCBUF_1:
    //case IAV_SRCBUF_MN:
      buf_id = AM_SOURCE_BUFFER_MAIN;
      break;
    case IAV_SRCBUF_2:
    //case IAV_SRCBUF_PC:
      buf_id = AM_SOURCE_BUFFER_2ND;
      break;
    case IAV_SRCBUF_3:
    //case IAV_SRCBUF_PB:
      buf_id = AM_SOURCE_BUFFER_3RD;
      break;
    case IAV_SRCBUF_4:
    //case IAV_SRCBUF_PA:
      buf_id = AM_SOURCE_BUFFER_4TH;
      break;
    case IAV_SRCBUF_EFM:
      buf_id = AM_SOURCE_BUFFER_EFM;
      break;
    case IAV_SRCBUF_PMN:
      buf_id = AM_SOURCE_BUFFER_PMN;
      break;
    default:
      WARN("Invalid buffer id: %d! Use default value \"invalid\"!", id);
      break;
  }

  return buf_id;
};

iav_dsp_sub_buffer_id dsp_sub_buf_id_mw_to_iav(AM_DSP_SUB_BUF_ID id)
{
  iav_dsp_sub_buffer_id buf_id = IAV_DSP_SUB_BUF_RAW;

  switch (id) {
    case AM_DSP_SUB_BUF_RAW:
      buf_id = IAV_DSP_SUB_BUF_RAW;
      break;
    case AM_DSP_SUB_BUF_MAIN_YUV:
      buf_id = IAV_DSP_SUB_BUF_MAIN_YUV;
      break;
    case AM_DSP_SUB_BUF_MAIN_ME:
      buf_id = IAV_DSP_SUB_BUF_MAIN_ME1;
      break;
    case AM_DSP_SUB_BUF_PREV_A_YUV:
      buf_id = IAV_DSP_SUB_BUF_PREV_A_YUV;
      break;
    case AM_DSP_SUB_BUF_PREV_A_ME:
      buf_id = IAV_DSP_SUB_BUF_PREV_A_ME1;
      break;
    case AM_DSP_SUB_BUF_PREV_B_YUV:
      buf_id = IAV_DSP_SUB_BUF_PREV_B_YUV;
      break;
    case AM_DSP_SUB_BUF_PREV_B_ME:
      buf_id = IAV_DSP_SUB_BUF_PREV_B_ME1;
      break;
    case AM_DSP_SUB_BUF_PREV_C_YUV:
      buf_id = IAV_DSP_SUB_BUF_PREV_C_YUV;
      break;
    case AM_DSP_SUB_BUF_PREV_C_ME:
      buf_id = IAV_DSP_SUB_BUF_PREV_C_ME1;
      break;
    case AM_DSP_SUB_BUF_EFM_YUV:
      buf_id = IAV_DSP_SUB_BUF_EFM_YUV;
      break;
    case AM_DSP_SUB_BUF_EFM_ME:
      buf_id = IAV_DSP_SUB_BUF_EFM_ME1;
      break;
    case AM_DSP_SUB_BUF_POST_MAIN_YUV:
      buf_id = IAV_DSP_SUB_BUF_POST_MAIN_YUV;
      break;
    case AM_DSP_SUB_BUF_POST_MAIN_ME:
      buf_id = IAV_DSP_SUB_BUF_POST_MAIN_ME1;
      break;
    case AM_DSP_SUB_BUF_INT_MAIN_YUV:
      buf_id = IAV_DSP_SUB_BUF_INT_MAIN_YUV;
      break;

    default:
      WARN("Invalid dsp sub buffer id: %d! Use default value!", id);
      break;
  }

  return buf_id;
}

AM_DSP_SUB_BUF_ID dsp_sub_buf_id_iav_to_mw(iav_dsp_sub_buffer_id id)
{
  AM_DSP_SUB_BUF_ID buf_id = AM_DSP_SUB_BUF_RAW;

  switch (id) {
    case IAV_DSP_SUB_BUF_RAW:
      buf_id = AM_DSP_SUB_BUF_RAW;
      break;
    case IAV_DSP_SUB_BUF_MAIN_YUV:
      buf_id = AM_DSP_SUB_BUF_MAIN_YUV;
      break;
    case IAV_DSP_SUB_BUF_MAIN_ME1:
      buf_id = AM_DSP_SUB_BUF_MAIN_ME;
      break;
    case IAV_DSP_SUB_BUF_PREV_A_YUV:
      buf_id = AM_DSP_SUB_BUF_PREV_A_YUV;
      break;
    case IAV_DSP_SUB_BUF_PREV_A_ME1:
      buf_id = AM_DSP_SUB_BUF_PREV_A_ME;
      break;
    case IAV_DSP_SUB_BUF_PREV_B_YUV:
      buf_id = AM_DSP_SUB_BUF_PREV_B_YUV;
      break;
    case IAV_DSP_SUB_BUF_PREV_B_ME1:
      buf_id = AM_DSP_SUB_BUF_PREV_B_ME;
      break;
    case IAV_DSP_SUB_BUF_PREV_C_YUV:
      buf_id = AM_DSP_SUB_BUF_PREV_C_YUV;
      break;
    case IAV_DSP_SUB_BUF_PREV_C_ME1:
      buf_id = AM_DSP_SUB_BUF_PREV_C_ME;
      break;
    case IAV_DSP_SUB_BUF_EFM_YUV:
      buf_id = AM_DSP_SUB_BUF_EFM_YUV;
      break;
    case IAV_DSP_SUB_BUF_EFM_ME1:
      buf_id = AM_DSP_SUB_BUF_EFM_ME;
      break;
    case IAV_DSP_SUB_BUF_POST_MAIN_YUV:
      buf_id = AM_DSP_SUB_BUF_POST_MAIN_YUV;
      break;
    case IAV_DSP_SUB_BUF_POST_MAIN_ME1:
      buf_id = AM_DSP_SUB_BUF_POST_MAIN_ME;
      break;
    case IAV_DSP_SUB_BUF_INT_MAIN_YUV:
      buf_id = AM_DSP_SUB_BUF_INT_MAIN_YUV;
      break;

    default:
      WARN("Invalid dsp sub buffer id: %d! Use default value!", id);
      break;
  }

  return buf_id;
}

iav_stream_type stream_type_mw_to_iav(AM_STREAM_TYPE type)
{
  iav_stream_type stream_type = IAV_STREAM_TYPE_NONE;

  switch (type) {
    case AM_STREAM_TYPE_NONE:
      stream_type = IAV_STREAM_TYPE_NONE;
      break;
    case AM_STREAM_TYPE_H264:
      stream_type = IAV_STREAM_TYPE_H264;
      break;
    case AM_STREAM_TYPE_H265:
      stream_type = IAV_STREAM_TYPE_H265;
      break;
    case AM_STREAM_TYPE_MJPEG:
      stream_type = IAV_STREAM_TYPE_MJPEG;
      break;
    default:
      WARN("Invalid stream type: %d! Use default value \"none\"!", type);
      break;
  }

  return stream_type;
}

AM_STREAM_TYPE stream_type_iav_to_mw(iav_stream_type type)
{
  AM_STREAM_TYPE stream_type = AM_STREAM_TYPE_NONE;

  switch (type) {
    case IAV_STREAM_TYPE_NONE:
      stream_type = AM_STREAM_TYPE_NONE;
      break;
    case IAV_STREAM_TYPE_H264:
      stream_type = AM_STREAM_TYPE_H264;
      break;
    case IAV_STREAM_TYPE_H265:
      stream_type = AM_STREAM_TYPE_H265;
      break;
    case IAV_STREAM_TYPE_MJPEG:
      stream_type = AM_STREAM_TYPE_MJPEG;
      break;
    default:
      WARN("Invalid stream type: %d! Use default value \"none\"!", type);
      break;
  }

  return stream_type;
}

iav_srcbuf_state buffer_state_mw_to_iav(AM_SRCBUF_STATE state)
{
  iav_srcbuf_state buffer_state = IAV_SRCBUF_STATE_UNKNOWN;
  switch (state) {
    case AM_SRCBUF_STATE_ERROR:
      buffer_state = IAV_SRCBUF_STATE_ERROR;
      break;
    case AM_SRCBUF_STATE_IDLE:
      buffer_state = IAV_SRCBUF_STATE_IDLE;
      break;
    case AM_SRCBUF_STATE_BUSY:
      buffer_state = IAV_SRCBUF_STATE_BUSY;
      break;
    default:
      WARN("Invalid buffer state: %d, set to unknown!", state);
      break;
  }
  return buffer_state;
}

AM_SRCBUF_STATE buffer_state_iav_to_mw(iav_srcbuf_state state)
{
  AM_SRCBUF_STATE buffer_state = AM_SRCBUF_STATE_UNKNOWN;

  switch (state) {
    case IAV_SRCBUF_STATE_ERROR:
      buffer_state = AM_SRCBUF_STATE_ERROR;
      break;
    case IAV_SRCBUF_STATE_IDLE:
      buffer_state = AM_SRCBUF_STATE_IDLE;
      break;
    case IAV_SRCBUF_STATE_BUSY:
      buffer_state = AM_SRCBUF_STATE_BUSY;
      break;
    default:
      WARN("Invalid buffer state: %d, set to unknown!", state);
      break;
  }
  return buffer_state;
}

iav_stream_state stream_state_mw_to_iav(AM_STREAM_STATE state)
{
  iav_stream_state stream_state = IAV_STREAM_STATE_UNKNOWN;

  switch (state) {
    case AM_STREAM_STATE_IDLE:
      stream_state = IAV_STREAM_STATE_IDLE;
      break;
    case AM_STREAM_STATE_STARTING:
      stream_state = IAV_STREAM_STATE_STARTING;
      break;
    case AM_STREAM_STATE_ENCODING:
      stream_state = IAV_STREAM_STATE_ENCODING;
      break;
    case AM_STREAM_STATE_STOPPING:
      stream_state = IAV_STREAM_STATE_STOPPING;
      break;
    case AM_STREAM_STATE_ERROR:
    default:
      WARN("Invalid stream state: %d! Use default value \"unkown\"!", state);
      break;
  }

  return stream_state;
}

AM_STREAM_STATE stream_state_iav_to_mw(iav_stream_state state)
{
  AM_STREAM_STATE stream_state = AM_STREAM_STATE_ERROR;

  switch (state) {
    case IAV_STREAM_STATE_IDLE:
      stream_state = AM_STREAM_STATE_IDLE;
      break;
    case IAV_STREAM_STATE_STARTING:
      stream_state = AM_STREAM_STATE_STARTING;
      break;
    case IAV_STREAM_STATE_ENCODING:
      stream_state = AM_STREAM_STATE_ENCODING;
      break;
    case IAV_STREAM_STATE_STOPPING:
      stream_state = AM_STREAM_STATE_STOPPING;
      break;
    case IAV_STREAM_STATE_UNKNOWN:
    default:
      WARN("Invalid stream state: %d! Use default value \"unkown\"!", state);
      break;
  }

  return stream_state;
}

iav_chroma_format h26x_chroma_mw_to_iav(AM_CHROMA_FORMAT format)
{
  iav_chroma_format chroma_format = H264_CHROMA_YUV420;

  switch (format) {
    case AM_CHROMA_FORMAT_YUV420:
      chroma_format = H264_CHROMA_YUV420;
      break;
    case AM_CHROMA_FORMAT_Y8:
      chroma_format = H264_CHROMA_MONO;
      break;
    default:
      WARN("Invalid chroma format: %d! Use default value \"420\"!", format);
      break;
  }

  return chroma_format;
}

AM_CHROMA_FORMAT h26x_chroma_iav_to_mw(iav_chroma_format format)
{
  AM_CHROMA_FORMAT chroma = AM_CHROMA_FORMAT_YUV420;
  switch(format) {
    case H264_CHROMA_YUV420:
      chroma = AM_CHROMA_FORMAT_YUV420;
      break;
    case H264_CHROMA_MONO:
      chroma = AM_CHROMA_FORMAT_Y8;
      break;
    default:
      WARN("Invalid chroma format: %d! Use default value \"420\"!", format);
      break;
  }

  return chroma;
}

iav_chroma_format mjpeg_chroma_mw_to_iav(AM_CHROMA_FORMAT format)
{
  iav_chroma_format chroma_format = JPEG_CHROMA_YUV420;

  switch (format) {
    case AM_CHROMA_FORMAT_YUV420:
      chroma_format = JPEG_CHROMA_YUV420;
      break;
    case AM_CHROMA_FORMAT_YUV422:
      chroma_format = JPEG_CHROMA_YUV422;
      break;
    case AM_CHROMA_FORMAT_Y8:
      chroma_format = JPEG_CHROMA_MONO;
      break;
    default:
      WARN("Invalid chroma format: %d! Use default value \"420\"!", format);
      break;
  }

  return chroma_format;
}

AM_CHROMA_FORMAT chroma_iav_to_mw(iav_chroma_format format)
{
  AM_CHROMA_FORMAT chroma_format = AM_CHROMA_FORMAT_YUV420;

  switch (format) {
    //case H264_CHROMA_YUV420:
    case JPEG_CHROMA_YUV420:
      chroma_format = AM_CHROMA_FORMAT_YUV420;
      break;
    case JPEG_CHROMA_YUV422:
      chroma_format = AM_CHROMA_FORMAT_YUV422;
      break;
    //case H264_CHROMA_MONO:
    case JPEG_CHROMA_MONO:
      chroma_format = AM_CHROMA_FORMAT_Y8;
      break;
    default:
      WARN("Invalid chroma format: %d! Use default value \"420\"!", format);
      break;
  }

  return chroma_format;
}

iav_state iav_state_mw_to_iav(AM_IAV_STATE state)
{
  iav_state iav = IAV_STATE_INIT;

  switch (state) {
    case AM_IAV_STATE_INIT:
      iav = IAV_STATE_INIT;
      break;
    case AM_IAV_STATE_IDLE:
      iav = IAV_STATE_IDLE;
      break;
    case AM_IAV_STATE_PREVIEW:
      iav = IAV_STATE_PREVIEW;
      break;
    case AM_IAV_STATE_ENCODING:
      iav = IAV_STATE_ENCODING;
      break;
    case AM_IAV_STATE_EXITING_PREVIEW:
      iav = IAV_STATE_EXITING_PREVIEW;
      break;
    case AM_IAV_STATE_DECODING:
      iav = IAV_STATE_DECODING;
      break;
    default:
      WARN("Invalid iav state: %d! Use default value \"init\"!", state);
      break;
  }

  return iav;
}

AM_IAV_STATE iav_state_iav_to_mw(iav_state state)
{
  AM_IAV_STATE iav = AM_IAV_STATE_ERROR;

  switch (state) {
    case IAV_STATE_INIT:
      iav = AM_IAV_STATE_INIT;
      break;
    case IAV_STATE_IDLE:
      iav = AM_IAV_STATE_IDLE;
      break;
    case IAV_STATE_PREVIEW:
      iav = AM_IAV_STATE_PREVIEW;
      break;
    case IAV_STATE_ENCODING:
      iav = AM_IAV_STATE_ENCODING;
      break;
    case IAV_STATE_EXITING_PREVIEW:
      iav = AM_IAV_STATE_EXITING_PREVIEW;
      break;
    case IAV_STATE_DECODING:
      iav = AM_IAV_STATE_DECODING;
      break;
    default:
      WARN("Invalid iav state: %ds! Use default value \"init\"!", state);
      break;
  }

  return iav;
}

int encode_mode_mw_to_iav(AM_ENCODE_MODE mode)
{
  int encode_mode = DSP_NORMAL_ISO_MODE;

  switch (mode) {
    case AM_ENCODE_MODE_0:
      encode_mode = DSP_NORMAL_ISO_MODE;
      break;
    case AM_ENCODE_MODE_1:
      encode_mode = DSP_MULTI_REGION_WARP_MODE;
      break;
    case AM_ENCODE_MODE_2:
      encode_mode = DSP_BLEND_ISO_MODE;
      break;
    case AM_ENCODE_MODE_4:
      encode_mode = DSP_ADVANCED_ISO_MODE;
      break;
    case AM_ENCODE_MODE_5:
      encode_mode = DSP_HDR_LINE_INTERLEAVED_MODE;
      break;
    default:
      WARN("Invalid encode mode: %d! Use default value \"normal\"!", mode);
      break;
  }

  return encode_mode;
}

AM_ENCODE_MODE encode_mode_iav_to_mw(int mode)
{
  AM_ENCODE_MODE encode_mode = AM_ENCODE_INVALID_MODE;

  switch (mode) {
    case DSP_NORMAL_ISO_MODE:
      encode_mode = AM_ENCODE_MODE_0;
      break;
    case DSP_MULTI_REGION_WARP_MODE:
      encode_mode = AM_ENCODE_MODE_1;
      break;
    case DSP_BLEND_ISO_MODE:
      encode_mode = AM_ENCODE_MODE_2;
      break;
    case DSP_ADVANCED_ISO_MODE:
      encode_mode = AM_ENCODE_MODE_4;
      break;
    case DSP_HDR_LINE_INTERLEAVED_MODE:
      encode_mode = AM_ENCODE_MODE_5;
      break;
    default:
      WARN("Invalid encode mode: %d! Use default value \"invalid\"!", mode);
      break;
  }

  return encode_mode;
}
int hdr_type_mw_to_iav(AM_HDR_TYPE hdr_type)
{
  int mode;
  switch(hdr_type) {
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
      WARN("Unknown HDR type: %d! Use default value \"linear\"!", hdr_type);
      mode = AMBA_VIDEO_LINEAR_MODE;
      break;
  }
  return mode;
}

AM_HDR_TYPE hdr_type_iav_to_mw(int hdr_type)
{
  AM_HDR_TYPE mode;
  switch(hdr_type) {
    case AMBA_VIDEO_LINEAR_MODE:
      mode = AM_HDR_SINGLE_EXPOSURE;
      break;
    case AMBA_VIDEO_2X_HDR_MODE:
      mode = AM_HDR_2_EXPOSURE;
      break;
    case AMBA_VIDEO_3X_HDR_MODE:
      mode = AM_HDR_3_EXPOSURE;
      break;
    case AMBA_VIDEO_4X_HDR_MODE:
      mode = AM_HDR_4_EXPOSURE;
      break;
    case AMBA_VIDEO_INT_HDR_MODE:
      mode = AM_HDR_SENSOR_INTERNAL;
      break;
    default:
      WARN("Unknown HDR type: %d! Use default value \"linear\"!", hdr_type);
      mode = AM_HDR_SINGLE_EXPOSURE;
      break;
  }
  return mode;
}

uint32_t frame_type_mw_to_iav(AM_VIDEO_FRAME_TYPE type)
{
  iav_pic_type pic_type = IAV_PIC_TYPE_P_FAST_SEEK_FRAME;
  switch (type) {
    case AM_VIDEO_FRAME_TYPE_MJPEG:
      pic_type = IAV_PIC_TYPE_MJPEG_FRAME;
      break;
    case AM_VIDEO_FRAME_TYPE_IDR:
      pic_type = IAV_PIC_TYPE_IDR_FRAME;
      break;
    case AM_VIDEO_FRAME_TYPE_I:
      pic_type = IAV_PIC_TYPE_I_FRAME;
      break;
    case AM_VIDEO_FRAME_TYPE_P:
      pic_type = IAV_PIC_TYPE_P_FRAME;
      break;
    case AM_VIDEO_FRAME_TYPE_B:
      pic_type = IAV_PIC_TYPE_B_FRAME;
      break;
    default:
      ERROR("VIDEO FRAME TYPE %d is not supported!", type);
      break;
  }
  return pic_type;
}

AM_VIDEO_FRAME_TYPE frame_type_iav_to_mw(uint32_t type)
{
  AM_VIDEO_FRAME_TYPE frame_type = AM_VIDEO_FRAME_TYPE_NONE;
  switch (type) {
    case IAV_PIC_TYPE_MJPEG_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_MJPEG;
      break;
    case IAV_PIC_TYPE_IDR_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_IDR;
      break;
    case IAV_PIC_TYPE_I_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_I;
      break;
    case IAV_PIC_TYPE_P_FRAME:
    case IAV_PIC_TYPE_P_FAST_SEEK_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_P;
      break;
    case IAV_PIC_TYPE_B_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_B;
      break;
    case 7: /* 0x7 means end of stream */
      frame_type = AM_VIDEO_FRAME_TYPE_NONE;
      break;
    default:
      ERROR("VIDEO FRAME TYPE %d is not supported", type);
      frame_type = AM_VIDEO_FRAME_TYPE_NONE;
      break;
  }
  return frame_type;
}
} /* namespace AMVideoTransIav2 */
