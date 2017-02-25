/*******************************************************************************
 * am_platform_iav2.cpp
 *
 * History:
 *   May 7, 2015 - [ypchang] created file
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

#include "am_encode_config_param.h"
#include "am_encode_config.h"
#include "am_platform_iav2_config.h"
#include "am_video_utility.h"
#include "am_platform_iav2_utility.h"
#include "am_platform_iav2.h"

#include <sys/mman.h>
#include <sys/ioctl.h>

#define NORMAL_MODE_RSRC_LMT_CONFIG_FILE    "normal_mode_resource_limit.acs"
#define ADV_ISO_MODE_RSRC_LMT_CONFIG_FILE   "adv_iso_mode_resource_limit.acs"
#define ADV_HDR_MODE_RSRC_LMT_CONFIG_FILE   "adv_hdr_mode_resource_limit.acs"
#define DEWARP_MODE_RSRC_LMT_CONFIG_FILE    "dewarp_mode_resource_limit.acs"

#define AUTO_LOCK_PLATFORM(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMPlatformIav2 *AMPlatformIav2::m_instance = nullptr;
std::recursive_mutex AMPlatformIav2::m_mtx;

AMIPlatformPtr AMIPlatform::get_instance()
{
  return AMPlatformIav2::get_instance();
}

AM_RESULT AMPlatformIav2::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_feature_config->get_config(m_feature_param)) != AM_RESULT_OK) {
      ERROR("Failed to get features!");
      break;
    }
    if ((result = select_mode_by_features(m_feature_param)) != AM_RESULT_OK) {
      break;
    }

    std::string file_name;
    switch (m_feature_param.mode.second) {
      case AM_ENCODE_MODE_0:
        INFO("Set camera to Normal mode!");
        file_name = NORMAL_MODE_RSRC_LMT_CONFIG_FILE;
        break;
      case AM_ENCODE_MODE_1:
        INFO("Set camera to Dewarp mode!");
        file_name = DEWARP_MODE_RSRC_LMT_CONFIG_FILE;
        break;
      case AM_ENCODE_MODE_4:
        INFO("Set camera to Advanced ISO mode!");
        file_name = ADV_ISO_MODE_RSRC_LMT_CONFIG_FILE;
        break;
      case AM_ENCODE_MODE_5:
        INFO("Set camera to Advanced HDR mode!");
        file_name = ADV_HDR_MODE_RSRC_LMT_CONFIG_FILE;
        break;
      default:
        result = AM_RESULT_ERR_INVALID;
        ERROR("Invalid mode!");
        break;
    }

    m_resource_config->set_config_file(file_name);
    if ((result = m_resource_config->get_config(m_resource_param))
        != AM_RESULT_OK) {
      ERROR("Failed to get resource config!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::feature_config_set(const AMFeatureParam &param)
{
  return m_feature_config->set_config(param);
}

AM_RESULT AMPlatformIav2::feature_config_get(AMFeatureParam &param)
{
  return m_feature_config->get_config(param);
}

AM_RESULT AMPlatformIav2::vin_params_get(AMVinInfo &info)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_video_info video_info = { 0 };
  vindev_devinfo dev_info = { 0 };
  do {
    if (ioctl(m_iav, IAV_IOC_VIN_GET_DEVINFO, &dev_info) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_DEVINFO");
      break;
    }
    video_info.vsrc_id = dev_info.vsrc_id;
    video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
      break;
    }
    info.vin_id = video_info.vsrc_id;
    info.width = video_info.info.width;
    info.height = video_info.info.height;
    info.fps = AMVideoTrans::q9fps_to_fps(video_info.info.fps);
    info.hdr_mode = video_info.info.hdr_mode;
    info.type = video_info.info.type;
    info.bits = video_info.info.bits;
    info.ratio = video_info.info.ratio;
    info.system = video_info.info.system;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_number_get(uint32_t &number)
{
  /* todo: add actual code to query VIN device numbers */
  number = 1;
  return AM_RESULT_OK;
}

AM_RESULT AMPlatformIav2::vin_mode_set(const AMPlatformVinMode &mode)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_mode vsrc_mode = {0};
  do {
    vsrc_mode.vsrc_id = mode.id;
    vsrc_mode.video_mode = AMVinTransIav2::mode_mw_to_iav(mode.mode);
    vsrc_mode.hdr_mode = AMVideoTransIav2::hdr_type_mw_to_iav(mode.hdr_type);
    if (ioctl(m_iav, IAV_IOC_VIN_SET_MODE, &vsrc_mode) < 0) {
      PERROR("IAV_IOC_VIN_SET_MODE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_mode_get(AMPlatformVinMode &mode)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while(0);

  return result;
}

AM_RESULT AMPlatformIav2::vin_fps_set(const AMPlatformVinFps &fps)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_fps vsrc_fps = {0};
  do {
    vsrc_fps.vsrc_id = fps.id;
    vsrc_fps.fps = AMVideoTrans::fps_to_q9fps(fps.fps);
    if (ioctl(m_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_SET_FPS");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::vin_fps_get(AMPlatformVinFps &fps)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    vindev_fps vsrc_fps = {0};

    vsrc_fps.vsrc_id = fps.id;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_SET_FPS");
      break;
    }
    fps.fps = AMVideoTrans::q9fps_to_fps(vsrc_fps.fps);
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_flip_set(const AMPlatformVinFlip &flip)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_mirror mirror_mode = {0};
  do {
    mirror_mode.vsrc_id = flip.id;
    mirror_mode.pattern = AMVinTransIav2::flip_mw_to_iav(flip.flip);
    mirror_mode.bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_MIRROR, &mirror_mode) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_SET_FLIP");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::vin_flip_get(AMPlatformVinFlip &flip)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_mirror mirror_mode = {0};
  do {
    mirror_mode.vsrc_id = flip.id;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_MIRROR, &mirror_mode) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_FLIP");
      break;
    }
    flip.flip = AMVinTransIav2::flip_iav_to_mw(amba_vindev_mirror_pattern_e
                                               (mirror_mode.pattern));
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::vin_shutter_set(const AMPlatformVinShutter &shutter)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_shutter vsrc_shutter = {0};
  do {
    vsrc_shutter.vsrc_id = shutter.id;
    vsrc_shutter.shutter = 512000000U /
        shutter.shutter_time.den * shutter.shutter_time.num;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_SHUTTER, &vsrc_shutter) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_SET_SHUTTER");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_shutter_get(AMPlatformVinShutter &shutter)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_agc_set(const AMPlatformVinAGC &agc)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_agc vsrc_agc = {0};
  do {
    vsrc_agc.vsrc_id = agc.id;
    vsrc_agc.agc = agc.agc_info.agc;
    vsrc_agc.agc_min = agc.agc_info.agc_min;
    vsrc_agc.agc_max = agc.agc_info.agc_max;
    vsrc_agc.agc_step = agc.agc_info.agc_step;
    vsrc_agc.wdr_again_idx_max = agc.agc_info.wdr_again_idx_max;
    vsrc_agc.wdr_again_idx_min = agc.agc_info.wdr_again_idx_min;
    vsrc_agc.wdr_dgain_idx_max = agc.agc_info.wdr_dgain_idx_max;
    vsrc_agc.wdr_dgain_idx_min = agc.agc_info.wdr_dgain_idx_min;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_AGC, &vsrc_agc) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_SET_AGC");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_agc_get(AMPlatformVinAGC &agc)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    vindev_agc vsrc_agc = {0};
    vsrc_agc.vsrc_id = agc.id;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_AGC, &vsrc_agc) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_AGC");
      break;
    }
    agc.id = AM_VIN_ID(vsrc_agc.vsrc_id);
    agc.agc_info.agc = vsrc_agc.agc;
    agc.agc_info.agc_max = vsrc_agc.agc_max;
    agc.agc_info.agc_min = vsrc_agc.agc_min;
    agc.agc_info.agc_step = vsrc_agc.agc_step;
    agc.agc_info.wdr_again_idx_max = vsrc_agc.wdr_again_idx_max;
    agc.agc_info.wdr_again_idx_min = vsrc_agc.wdr_again_idx_min;
    agc.agc_info.wdr_dgain_idx_max = vsrc_agc.wdr_dgain_idx_max;
    agc.agc_info.wdr_dgain_idx_min = vsrc_agc.wdr_dgain_idx_min;
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::vin_info_get(AMPlatformVinInfo &info)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_video_info video_info = {0};
  do {
    video_info.vsrc_id = info.id;
    video_info.info.mode = AMVinTransIav2::mode_mw_to_iav(info.mode);
    video_info.info.hdr_mode =
        AMVideoTransIav2::hdr_type_mw_to_iav(info.hdr_type);
    video_info.info.bits = AMVinTrans::bits_mw_to_iav(info.bits);
    if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
      break;
    }
    info.id = AM_VIN_ID(video_info.vsrc_id);
    info.size.width = video_info.info.width;
    info.size.height = video_info.info.height;
    info.mode =
        AMVinTransIav2::mode_iav_to_mw(amba_video_mode(video_info.info.mode));
    info.fps = AMVideoTrans::q9fps_to_fps(video_info.info.fps);
    info.hdr_type =
        AMVideoTransIav2::hdr_type_iav_to_mw(video_info.info.hdr_mode);
    info.type = AMVinTransIav2::sensor_type_iav_to_mw(video_info.info.type);
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::vin_wait_next_frame()
{
  AM_RESULT result = AM_RESULT_OK;

  if (ioctl(m_iav, IAV_IOC_WAIT_NEXT_FRAME, 0) < 0) {
    PERROR("IAV_IOC_WAIT_NEXT_FRAME");
    result = AM_RESULT_ERR_PERM;
  }

  return result;
}

AM_RESULT AMPlatformIav2::vin_info_list_get(AM_VIN_ID id,
                                            AMPlatformVinInfoList &list)
{
  AM_RESULT result = AM_RESULT_OK;
  bool ok = true;

  for (int32_t mode = AM_VIN_MODE_AUTO + 1; mode < AM_VIN_MODE_NUM; ++ mode) {
    vindev_video_info video_info = {0};
    video_info.vsrc_id = id;
    video_info.info.mode = AMVinTransIav2::mode_mw_to_iav(AM_VIN_MODE(mode));
    for (int32_t hdr = AM_HDR_SINGLE_EXPOSURE; hdr < AM_HDR_TYPE_NUM; ++ hdr) {
      video_info.info.hdr_mode =
          AMVideoTransIav2::hdr_type_mw_to_iav(AM_HDR_TYPE(hdr));
      for (int32_t bits = 0; bits < AM_VIN_BITS_NUM; ++ bits) {
        video_info.info.bits = AMVinTrans::bits_mw_to_iav(AM_VIN_BITS(bits));
        if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
          if (AM_LIKELY(errno == EINVAL)) {
            continue;
          } else {
            PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
            result  = AM_RESULT_ERR_DSP;
            ok = false;
            break;
          }
        } else {
          AMPlatformVinInfo info;
          info.id = id;
          info.size.width = video_info.info.width;
          info.size.height = video_info.info.height;
          info.mode = AMVinTransIav2::
              mode_iav_to_mw(amba_video_mode(video_info.info.mode));
          info.fps = AMVideoTrans::q9fps_to_fps(video_info.info.fps);
          info.hdr_type =
              AMVideoTransIav2::hdr_type_iav_to_mw(video_info.info.hdr_mode);
          info.type =
              AMVinTransIav2::sensor_type_iav_to_mw(video_info.info.type);
          list.push_back(info);
        }
      }
      if (AM_UNLIKELY(!ok)) {
        break;
      }
    }
    if (AM_UNLIKELY(!ok)) {
      break;
    }
  }

  return result;
}

AM_RESULT AMPlatformIav2::sensor_info_get(AMPlatformSensorInfo &info)
{
  AM_RESULT result = AM_RESULT_OK;
  vindev_devinfo vsrc_info = {0};
  do {
    vsrc_info.vsrc_id = info.id;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_DEVINFO, &vsrc_info) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_VIN_GET_DEVINFO");
      break;
    }
    info.name = vsrc_info.name;
    info.sensor_id = vsrc_info.sensor_id;
    info.dev_type = AMVinTransIav2::type_iav_to_mw(vsrc_info.dev_type);
  } while (0);
  return result;
}

int32_t   AMPlatformIav2::vout_sink_id_get(AM_VOUT_ID id, AM_VOUT_TYPE type)
{
  int32_t sink_id = -1;

  do {
    int32_t num = 0;
    if (ioctl(m_iav, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
      PERROR("IAV_IOC_VOUT_GET_SINK_NUM");
      break;
    }
    if (num < 1) {
      ERROR("AMVoutPort: Please load vout driver!\n");
      break;
    }

    amba_vout_sink_info sink_info = {0};
    for (int32_t i = num - 1; i >= 0; --i) {
      sink_info.id = i;
      if (ioctl(m_iav, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
        PERROR("IAV_IOC_VOUT_GET_SINK_INFO");
        break;
      }
      DEBUG("sink %d is %s\n", sink_info.id, sink_info.name);

      //check vout id with VOUT sink id
      if ((sink_info.sink_type == AMVoutTransIav2::sink_type_mw_to_iav(type)) &&
          (sink_info.source_id == AMVoutTransIav2::id_mw_to_iav(id))) {
        INFO("sink %d is %s, it is type: %d, source id: %d\n",sink_info.id,
             sink_info.name, sink_info.sink_type, sink_info.source_id);
        sink_id = sink_info.id;
        break;
      }
    }
  } while (0);

  return sink_id;
}

AM_RESULT AMPlatformIav2::vout_active_sink_set(int32_t sink_id)
{
  AM_RESULT result = AM_RESULT_OK;
  if (ioctl(m_iav, IAV_IOC_VOUT_SELECT_DEV, sink_id) < 0) {
    PERROR("IAV_IOC_VOUT_SELECT_DEV");
    result = AM_RESULT_ERR_DSP;
  }

  return result;
}

AM_RESULT AMPlatformIav2::vout_sink_config_set(const AMPlatformVoutSinkConfig &sink)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //configure VOUT
    amba_video_sink_mode sink_cfg = {0};
    sink_cfg.id = sink.sink_id;
    sink_cfg.mode = AMVoutTransIav2::mode_mw_to_iav(sink.mode);
    sink_cfg.frame_rate = AMVoutTrans::mode_to_fps(sink.mode);
    sink_cfg.sink_type = AMVoutTransIav2::sink_type_mw_to_iav(sink.sink_type);
    INFO("vout sink%d: vout mode %d, video fps %d \n", sink_cfg.id,
         sink_cfg.mode, sink_cfg.frame_rate);
    sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
    sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
    if (sink.mode == AM_VOUT_MODE_480I ||
        sink.mode == AM_VOUT_MODE_576I||
        sink.mode == AM_VOUT_MODE_1080I60 ||
        sink.mode == AM_VOUT_MODE_1080I50) {
      sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
    } else {
      sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
    }

    sink_cfg.type = AMVoutTransIav2::video_type_mw_to_iav(sink.video_type);
    sink_cfg.bg_color.y = 0x10;
    sink_cfg.bg_color.cb = 0x80;
    sink_cfg.bg_color.cr = 0x80;
    sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

    sink_cfg.csc_en = 1;
    sink_cfg.hdmi_color_space = AMBA_VOUT_HDMI_CS_AUTO;

    sink_cfg.hdmi_3d_structure = DDD_RESERVED; //TODO: hardcode
    sink_cfg.hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO; //TODO: hardcode
    sink_cfg.video_en = 1; //enable it

    sink_cfg.video_flip = AMVoutTransIav2::flip_mw_to_iav(sink.flip);
    sink_cfg.video_rotate = AMVoutTransIav2::rotate_mw_to_iav(sink.rotate);

    //fill video size config to VOUT SINK

    amba_vout_video_size video_size = {0};
    AMResolution size = AMVoutTrans::mode_to_resolution(sink.mode);
    video_size.specified = 1;
    video_size.vout_width = size.width;
    video_size.vout_height = size.height;
    video_size.video_width = size.width;
    video_size.video_height = size.height;
    sink_cfg.video_size = video_size;

    sink_cfg.fb_id = 0;
    sink_cfg.mixer_csc = AMBA_VOUT_MIXER_ENABLE_FOR_OSD;

    sink_cfg.direct_to_dsp = 0;
    if (ioctl(m_iav, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
      PERROR("IAV_IOC_VOUT_CONFIGURE_SINK");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    INFO("set vout sink configure done\n");
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::vout_halt(AM_VOUT_ID id)
{
  AM_RESULT result = AM_RESULT_OK;

  if (ioctl(m_iav, IAV_IOC_VOUT_HALT, AMVoutTransIav2::id_mw_to_iav(id)) < 0) {
    PERROR("IAV_IOC_VOUT_HALT");
    result = AM_RESULT_ERR_DSP;
  }

  return result;
}

AM_RESULT AMPlatformIav2::buffer_state_get(AM_SOURCE_BUFFER_ID id,
                                           AM_SRCBUF_STATE &state)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_srcbuf_info info = {0};

  do {
    info.buf_id = AMVideoTransIav2::buffer_id_mw_to_iav(id);
    if (ioctl(m_iav, IAV_IOC_GET_SOURCE_BUFFER_INFO, &info) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_SOURCE_BUFFER_INFO");
      break;
    }
    state = AMVideoTransIav2::buffer_state_iav_to_mw(info.state);
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::buffer_setup_set(const AMPlatformBufferFormatMap &format)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_srcbuf_setup  buf_setup = {0};

  do {
    for (uint32_t i = 0; i < IAV_SRCBUF_LAST; ++i) {
      AM_SOURCE_BUFFER_ID buffer_id =
          AMVideoTransIav2::buffer_id_iav_to_mw(iav_srcbuf_id(i));
      if (format.find(buffer_id) != format.end() &&
          format.at(buffer_id).type != AM_SOURCE_BUFFER_TYPE_OFF) {
        buf_setup.type[i] =
            AMVideoTransIav2::buffer_type_mw_to_iav(format.at(buffer_id).type);
        buf_setup.input[i].x = format.at(buffer_id).input.offset.x;
        buf_setup.input[i].y = format.at(buffer_id).input.offset.y;
        buf_setup.input[i].width = format.at(buffer_id).input.size.width;
        buf_setup.input[i].height = format.at(buffer_id).input.size.height;

        buf_setup.size[i].width = format.at(buffer_id).size.width;
        buf_setup.size[i].height = format.at(buffer_id).size.height;
      } else {
        buf_setup.type[i] = IAV_SRCBUF_TYPE_OFF;
      }
    }

    if (ioctl(m_iav, IAV_IOC_SET_SOURCE_BUFFER_SETUP, &buf_setup) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_SETUP");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::buffer_setup_get(AMPlatformBufferFormatMap &format)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_srcbuf_setup  buf_setup = {0};

  do {
    if (ioctl(m_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &buf_setup) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
      break;
    }

    for (uint32_t i = 0; i < IAV_SRCBUF_LAST; ++i) {
      AM_SOURCE_BUFFER_ID buffer_id =
          AMVideoTransIav2::buffer_id_iav_to_mw(iav_srcbuf_id(i));
      format[buffer_id].type =
          AMVideoTransIav2::buffer_type_iav_to_mw(buf_setup.type[i]);
      format[buffer_id].input.offset.x = buf_setup.input[i].x;
      format[buffer_id].input.offset.y = buf_setup.input[i].y;
      format[buffer_id].input.size.width = buf_setup.input[i].width;
      format[buffer_id].input.size.height = buf_setup.input[i].height;
      format[buffer_id].size.width = buf_setup.size[i].width;
      format[buffer_id].size.height = buf_setup.size[i].height;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::buffer_format_set(const AMPlatformBufferFormat &format)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_srcbuf_format buf_format = {0};
  do {
    buf_format.buf_id = AMVideoTransIav2::buffer_id_mw_to_iav(format.id);
    buf_format.input.x = format.input.offset.x;
    buf_format.input.y = format.input.offset.y;
    buf_format.input.width = format.input.size.width;
    buf_format.input.height = format.input.size.height;
    buf_format.size.width = format.size.width;
    buf_format.size.height = format.size.height;
    if (ioctl(m_iav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_FORMAT");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::buffer_format_get(AMPlatformBufferFormat &format)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_srcbuf_format buf_format = {0};
  do {
    buf_format.buf_id = AMVideoTransIav2::buffer_id_mw_to_iav(format.id);
    if (ioctl(m_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
      break;
    }
    format.input.offset.x = buf_format.input.x;
    format.input.offset.y = buf_format.input.y;
    format.input.size.width = buf_format.input.width;
    format.input.size.height = buf_format.input.height;
    format.size.width = buf_format.size.width;
    format.size.height = buf_format.size.height;
  } while (0);
  return result;
}

//For EFM Buffer
AM_RESULT AMPlatformIav2::efm_frame_request(AMPlatformEFMRequestFrame &frame)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_efm_request_frame request = {0};
  do {
    request.frame_idx = frame.frame_index;
    request.yuv_luma_offset = frame.yuv_luma_offset;
    request.yuv_chroma_offset = frame.yuv_chroma_offset;
    request.me1_offset = frame.me1_offset;

    if (ioctl(m_iav, IAV_IOC_EFM_REQUEST_FRAME, &request) < 0) {
      if (errno == EINTR) {
        result = AM_RESULT_ERR_IO;
      } else if (errno == EAGAIN) {
        result = AM_RESULT_ERR_AGAIN;
      } else {
        result = AM_RESULT_ERR_DSP;
      }
      if (errno != EAGAIN) {
        PERROR("IAV_IOC_EFM_HANDSHAKE_FRAME");
      }
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::efm_frame_handshake(AMPlatformEFMHandshakeFrame &frame)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_efm_handshake_frame handshake = {0};
  do {
    handshake.frame_idx = frame.frame_index;
    handshake.frame_pts = frame.frame_pts;
    if (ioctl(m_iav, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake) < 0) {
      if (errno == EINTR) {
        result = AM_RESULT_ERR_IO;
      } else if (errno == EAGAIN) {
        result = AM_RESULT_ERR_AGAIN;
      } else {
        result = AM_RESULT_ERR_DSP;
      }
      if (errno != EAGAIN) {
        PERROR("IAV_IOC_EFM_HANDSHAKE_FRAME");
      }
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_format_set(AMPlatformStreamFormat &format)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_config = {0};
  iav_stream_format stream_format = {0};
  do {
    stream_format.id = format.id;
    stream_format.buf_id = AMVideoTransIav2::buffer_id_mw_to_iav(format.source);
    stream_format.type = AMVideoTransIav2::stream_type_mw_to_iav(format.type);
    stream_format.enc_win.x = format.enc_win.offset.x;
    stream_format.enc_win.y = format.enc_win.offset.y;
    stream_format.enc_win.width = format.enc_win.size.width;
    stream_format.enc_win.height = format.enc_win.size.height;
    stream_format.rotate_cw = format.rotate ? 1 : 0;
    switch (format.flip) {
      case AM_VIDEO_FLIP_AUTO:
      case AM_VIDEO_FLIP_NONE:
        stream_format.hflip = 0;
        stream_format.vflip = 0;
        break;
      case AM_VIDEO_FLIP_VERTICAL:
        if (format.rotate) {
          stream_format.hflip = 1;
          stream_format.vflip = 0;
        } else {
          stream_format.hflip = 0;
          stream_format.vflip = 1;
        }
        break;
      case AM_VIDEO_FLIP_HORIZONTAL:
        if (format.rotate) {
          stream_format.hflip = 0;
          stream_format.vflip = 1;
        } else {
          stream_format.hflip = 1;
          stream_format.vflip = 0;
        }
        break;
      case AM_VIDEO_FLIP_VH_BOTH:
        stream_format.hflip = 1;
        stream_format.vflip = 1;
        break;
      default:
        WARN("Invalid flip type: %d, set to no flip", format.flip);
        break;
    }
    stream_format.duration = 0;
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_FORMAT, &stream_format) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_STREAM_FORMAT");
      break;
    }
    memset(&stream_config, 0, sizeof(stream_config));
    int32_t mul = 0;
    int32_t div = 0;
    if (format.fps.fps >= 0) {
      AMPlatformVinFps fps;
      fps.id = AM_VIN_0;
      if ((result = vin_fps_get(fps)) != AM_RESULT_OK) {
        break;
      }
      AMVideoTrans::fps_to_factor(fps.fps, format.fps.fps, mul, div);
    } else {
      mul = format.fps.mul;
      div = format.fps.div;
    }

    stream_config.id = format.id;
    stream_config.cid = IAV_STMCFG_FPS;
    stream_config.arg.fps.fps_multi = mul;
    stream_config.arg.fps.fps_div = div;
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_config) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_STREAM_CONFIG");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_format_get(AMPlatformStreamFormat &format)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameRate fps;
    fps.id = format.id;
    stream_framerate_get(fps);
    format.fps.fps = fps.fps;
    format.fps.mul = fps.mul;
    format.fps.div = fps.div;

    iav_stream_format stream_fmt = {0};
    stream_fmt.id = format.id;
    if (AM_UNLIKELY(ioctl(m_iav, IAV_IOC_GET_STREAM_FORMAT, &stream_fmt) < 0)) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_STREAM_FORMAT");
      break;
    }
    format.source = AMVideoTransIav2::buffer_id_iav_to_mw(stream_fmt.buf_id);
    format.type   = AMVideoTransIav2::stream_type_iav_to_mw(stream_fmt.type);
    format.enc_win.offset.x = stream_fmt.enc_win.x;
    format.enc_win.offset.y = stream_fmt.enc_win.y;
    format.enc_win.size.width = stream_fmt.enc_win.width;
    format.enc_win.size.height = stream_fmt.enc_win.height;
    format.rotate = (stream_fmt.rotate_cw != 0);
    stream_fmt.hflip = stream_fmt.hflip ? 1 : 0;
    stream_fmt.vflip = stream_fmt.vflip ? 1 : 0;
    if (format.rotate) {
      int32_t tmp = stream_fmt.hflip;
      stream_fmt.hflip = stream_fmt.vflip;
      stream_fmt.vflip = tmp;
    }
    format.flip = AM_VIDEO_FLIP((stream_fmt.hflip<<1) | stream_fmt.vflip);
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_config_set()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_config_get()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_h264_config_set(
    const AMPlatformH26xConfig &h264)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_h264_cfg h264_config = {0};
  iav_stream_cfg stream_config = {0};
  iav_bitrate bitrate = {0};
  do {
    h264_config.id = h264.id;
    if (ioctl(m_iav, IAV_IOC_GET_H264_CONFIG, &h264_config) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      break;
    }
    h264_config.id = h264.id;
    h264_config.gop_structure = h264.gop_model;
    h264_config.M = h264.M;
    h264_config.N = h264.N;
    h264_config.idr_interval = h264.idr_interval;
    h264_config.profile = h264.profile;
    h264_config.mv_threshold = h264.mv_threshold;
    h264_config.au_type = h264.au_type;
    h264_config.flat_area_improve = h264.flat_area_improve ? 1 : 0;
    h264_config.multi_ref_p = h264.multi_ref_p ? 1 : 0;
    h264_config.fast_seek_intvl = h264.fast_seek_intvl;
    h264_config.chroma_format =
        AMVideoTransIav2::h26x_chroma_mw_to_iav(h264.chroma_format);
    if (ioctl(m_iav, IAV_IOC_SET_H264_CONFIG, &h264_config) < 0) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_STREAM_CONFIG");
      break;
    }

    stream_config.id = h264.id;
    stream_config.cid = IAV_H264_CFG_BITRATE;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_config) < 0) {
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    bitrate = stream_config.arg.h264_rc;
    switch (h264.bitrate_control) {
      case AM_RC_CBR:
        bitrate.vbr_setting = IAV_BRC_SCBR;
        bitrate.average_bitrate = h264.target_bitrate;
        bitrate.max_i_size_KB = h264.i_frame_max_size;
        bitrate.qp_min_on_I = 10;
        bitrate.qp_max_on_I = 51;
        bitrate.qp_min_on_P = 10;
        bitrate.qp_max_on_P = 51;
        bitrate.qp_min_on_B = 10;
        bitrate.qp_max_on_B = 51;
        bitrate.i_qp_reduce = 6; //hard code i_qp_reduce
        bitrate.p_qp_reduce = 3; //hard code p_qp_reduce
        bitrate.adapt_qp = 0;    //disable aqp by default
        bitrate.skip_flag = 0;
        break;
      default:
        ERROR("Rate control %d not supported now!",
              h264.bitrate_control);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    if (result != AM_RESULT_OK) {
      break;
    }

    stream_config.id = h264.id;
    stream_config.cid = IAV_H264_CFG_BITRATE;
    stream_config.arg.h264_rc = bitrate;
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_config) < 0) {
      PERROR("IAV_IOC_SET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_h264_config_get(AMPlatformH26xConfig &h264)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    iav_h264_cfg h264_cfg = {0};
    iav_stream_cfg stream_cfg = {0};
    h264_cfg.id = h264.id;

    if (AM_UNLIKELY(ioctl(m_iav, IAV_IOC_GET_H264_CONFIG, &h264_cfg) < 0)) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_H264_CONFIG");
      break;
    }

    h264.gop_model = AM_GOP_MODEL(h264_cfg.gop_structure);
    h264.M = h264_cfg.M;
    h264.N = h264_cfg.N;
    h264.idr_interval = h264_cfg.idr_interval;
    h264.profile = AM_PROFILE(h264_cfg.profile);
    h264.mv_threshold = h264_cfg.mv_threshold;
    h264.au_type = AM_AU_TYPE(h264_cfg.au_type);
    h264.flat_area_improve = (h264_cfg.flat_area_improve != 0);
    h264.multi_ref_p = (h264_cfg.multi_ref_p != 0);
    h264.fast_seek_intvl = h264_cfg.fast_seek_intvl;
    h264.chroma_format = AMVideoTransIav2::h26x_chroma_iav_to_mw(
        iav_chroma_format(h264_cfg.chroma_format));
    h264.rate = h264_cfg.pic_info.rate;
    h264.scale = h264_cfg.pic_info.scale;

    stream_cfg.id = h264.id;
    stream_cfg.cid = IAV_H264_CFG_BITRATE;
    if (AM_UNLIKELY(ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0)) {
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    iav_bitrate &bitrate = stream_cfg.arg.h264_rc;
    h264.target_bitrate = bitrate.average_bitrate;
    h264.i_frame_max_size = bitrate.max_i_size_KB;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_h265_config_set(
    const AMPlatformH26xConfig &h265)
{
  ERROR("H.265 is not supported on this platform!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMPlatformIav2::stream_h265_config_get(AMPlatformH26xConfig &h265)
{
  ERROR("H.265 is not supported on this platform!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMPlatformIav2::stream_mjpeg_config_set(
    const AMPlatformMJPEGConfig &mjpeg)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_mjpeg_cfg mjpeg_config = {0};
  do {
    mjpeg_config.id = mjpeg.id;
    mjpeg_config.quality = mjpeg.quality;
    mjpeg_config.chroma_format =
        AMVideoTransIav2::mjpeg_chroma_mw_to_iav(mjpeg.chroma_format);

    if (ioctl(m_iav, IAV_IOC_SET_MJPEG_CONFIG, &mjpeg_config) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_MJPEG_CONFIG");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_mjpeg_config_get(AMPlatformMJPEGConfig &mjpeg)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_mjpeg_quality_set(
    const AMPlatformMJPEGConfig &mjpeg)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  do {
    stream_cfg.id = mjpeg.id;
    stream_cfg.cid = IAV_MJPEG_CFG_QUALITY;
    stream_cfg.arg.mjpeg_quality = mjpeg.quality;

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_SET_STREAM_CONFIG");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_mjpeg_quality_get(AMPlatformMJPEGConfig &mjpeg)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  do {
    stream_cfg.id = mjpeg.id;
    stream_cfg.cid = IAV_MJPEG_CFG_QUALITY;

    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    mjpeg.quality = stream_cfg.arg.mjpeg_quality;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_offset_get(AM_STREAM_ID id, AMOffset &offset)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  do {
    stream_cfg.id = id;
    stream_cfg.cid = IAV_STMCFG_OFFSET;

    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    offset.x = stream_cfg.arg.enc_offset.x;
    offset.y = stream_cfg.arg.enc_offset.y;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_offset_set(AM_STREAM_ID id, const AMOffset &offset)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  do {
    stream_cfg.id = id;
    stream_cfg.cid = IAV_STMCFG_OFFSET;
    stream_cfg.arg.enc_offset.x = offset.x;
    stream_cfg.arg.enc_offset.y = offset.y;

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_h26x_gop_get(AMPlatformH26xConfig &h26x)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  AMPlatformStreamFormat format;
  do {
    format.id = h26x.id;
    result = stream_format_get(format);
    if (result != AM_RESULT_OK) {
      ERROR("Failed to get stream format!");
      break;
    }

    if (format.type != AM_STREAM_TYPE_H264) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Only H264 stream is supported!");
      break;
    }

    stream_cfg.id = h26x.id;
    stream_cfg.cid = IAV_H264_CFG_GOP;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    h26x.N = stream_cfg.arg.h264_gop.N;
    h26x.idr_interval = stream_cfg.arg.h264_gop.idr_interval;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_h26x_gop_set(const AMPlatformH26xConfig &h26x)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};
  AMPlatformStreamFormat format;
  do {
    format.id = h26x.id;
    result = stream_format_get(format);
    if (result != AM_RESULT_OK) {
      ERROR("Failed to get stream format!");
      break;
    }

    if (format.type != AM_STREAM_TYPE_H264) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Only H264 stream is supported!");
      break;
    }

    stream_cfg.id = h26x.id;
    stream_cfg.cid = IAV_H264_CFG_GOP;
    stream_cfg.arg.h264_gop.N = h26x.N;
    stream_cfg.arg.h264_gop.idr_interval = h26x.idr_interval;

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_bitrate_set(const AMPlatformBitrate &bitrate)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_bitrate *innerbr = nullptr;
  iav_stream_cfg stream_cfg;
  memset(&stream_cfg, 0, sizeof(stream_cfg));

  do {
    //get stream format first H264 or H265
    AMPlatformStreamFormat format;
    format.id = bitrate.id;
    result = stream_format_get(format);
    if (result != AM_RESULT_OK) {
      ERROR("stream_format_get failed");
      break;
    }

    //get inner bitrate
    switch (format.type) {
      case AM_STREAM_TYPE_H264:
        stream_cfg.id = bitrate.id;
        stream_cfg.cid = IAV_H264_CFG_BITRATE;
        if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
          result = AM_RESULT_ERR_DSP;
          break;
        }
        innerbr = &stream_cfg.arg.h264_rc;
        break;
      case AM_STREAM_TYPE_H265:
      case AM_STREAM_TYPE_NONE:
      case AM_STREAM_TYPE_MJPEG:
      default:
        ERROR("stream type unsupport bitrate control");
        break;
    }

    innerbr->id = bitrate.id;
    //innerbr.vbr_setting = bitrate.rate_control_mode;
    if (bitrate.target_bitrate >= 0) {
      innerbr->average_bitrate = bitrate.target_bitrate;
    }
    innerbr->max_i_size_KB = bitrate.i_frame_max_size;

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::stream_bitrate_get(AMPlatformBitrate &bitrate)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_bitrate innerbr;
  iav_stream_cfg stream_cfg;
  memset(&stream_cfg, 0, sizeof(stream_cfg));
  memset(&innerbr, 0, sizeof(innerbr));
  do {
    //get stream format first H264 or H265
    AMPlatformStreamFormat format;
    format.id = bitrate.id;
    result = stream_format_get(format);
    if (result != AM_RESULT_OK) {
      ERROR("stream_format_get failed");
      break;
    }

    //get inner bitrate
    switch (format.type) {
      case AM_STREAM_TYPE_H264:
        stream_cfg.id = bitrate.id;
        stream_cfg.cid = IAV_H264_CFG_BITRATE;
        if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
          result = AM_RESULT_ERR_DSP;
          break;
        }
        memcpy(&innerbr, &stream_cfg.arg.h264_rc, sizeof(innerbr));
        break;
      case AM_STREAM_TYPE_H265:
      case AM_STREAM_TYPE_NONE:
      case AM_STREAM_TYPE_MJPEG:
      default:
        WARN("stream type unsupport bitrate control");
        break;
    }
    bitrate.rate_control_mode = AM_RATE_CONTROL(innerbr.vbr_setting);
    bitrate.target_bitrate = innerbr.average_bitrate;
    bitrate.i_frame_max_size = innerbr.max_i_size_KB;

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::stream_framerate_set(const AMPlatformFrameRate
                                               &framerate)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};

  do {
    AMPlatformVinFps fps;
    int32_t mul = 0;
    int32_t div = 0;
    if (framerate.fps >= 0) {
      AMPlatformVinFps fps;
      fps.id = AM_VIN_0;
      if ((result = vin_fps_get(fps)) != AM_RESULT_OK) {
        break;
      }
      AMVideoTrans::fps_to_factor(fps.fps, framerate.fps, mul, div);
    } else {
      mul = framerate.mul;
      div = framerate.div;
    }

    stream_cfg.id = framerate.id;
    stream_cfg.cid = IAV_STMCFG_FPS;
    stream_cfg.arg.fps.fps_multi = mul;
    stream_cfg.arg.fps.fps_div = div;

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::stream_framerate_get(AMPlatformFrameRate &framerate)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_cfg stream_cfg = {0};

  do {
    AMPlatformVinFps fps;
    fps.id = AM_VIN_0;
    if ((result = vin_fps_get(fps)) != AM_RESULT_OK) {
      break;
    }
    stream_cfg.id = framerate.id;
    stream_cfg.cid = IAV_STMCFG_FPS;
    if (AM_UNLIKELY(ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0)) {
      result  = AM_RESULT_ERR_DSP;
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      break;
    }

    framerate.mul = stream_cfg.arg.fps.fps_multi;
    framerate.div = stream_cfg.arg.fps.fps_div;
    framerate.fps = AMVideoTrans::factor_to_encfps(fps.fps,
                                                   framerate.mul,
                                                   framerate.div);
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::stream_state_get(AM_STREAM_ID id,
                                           AM_STREAM_STATE &state)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_stream_info info = {0};
  do {
    info.id = id;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_INFO, &info) < 0) {
      PERROR("IAV_IOC_GET_STREAM_INFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    state = AMVideoTransIav2::stream_state_iav_to_mw(info.state);
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::system_resource_set(const AMBufferParamMap &buffer,
                                              const AMStreamParamMap &stream)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_system_resource resource = {0};
  iav_enc_mode_cap cap = {0};

  do {
    resource.encode_mode =
        AMVideoTransIav2::encode_mode_mw_to_iav(m_feature_param.mode.second);
    if (ioctl(m_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
      PERROR("IAV_IOC_SET_SYSTEM_RESOURCE");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    cap.encode_mode =
        AMVideoTransIav2::encode_mode_mw_to_iav(m_feature_param.mode.second);
    if (ioctl(m_iav, IAV_IOC_QUERY_ENC_MODE_CAP, &cap) < 0) {
      PERROR("IAV_IOC_QUERY_ENC_MODE_CAP");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    for (uint32_t i = 0; i < m_resource_param.max_num_cap_sources.second; ++i) {
      uint32_t w = 0;
      uint32_t h = 0;
      int32_t extra_buf = 0;
      if (AM_LIKELY((m_resource_param.buf_max_size.second[i].width >= 0) &&
                    (m_resource_param.buf_max_size.second[i].height >= 0))) {
        w = m_resource_param.buf_max_size.second[i].width;
        h = m_resource_param.buf_max_size.second[i].height;
      } else {
        AMBufferParamMap::const_iterator iter =
            buffer.find(AM_SOURCE_BUFFER_ID(i));
        if (iter != buffer.end()) {
          w = iter->second.size.second.width;
          h = iter->second.size.second.height;
          NOTICE("Buffer[%u] uses user default setting!", i);
        } else {
          NOTICE("Buffer[%u] uses IAV default setting!", i);
        }
      }
      if (AMVideoTransIav2::buffer_id_iav_to_mw(iav_srcbuf_id(i)) ==
          AM_SOURCE_BUFFER_MAIN) {
        if (w < cap.min_main.width) {
          WARN("Main buffer max width[%d] is too small, reset to %d.",
               w, cap.min_main.width);
          w = cap.min_main.width;
        } else if (w > cap.max_main.width) {
          WARN("Main buffer max width[%d] is too large, reset to %d.",
               w, cap.max_main.width);
          w = cap.max_main.width;
        }

        if (h < cap.min_main.height) {
          WARN("Main buffer max height[%d] is too small, reset to %d.",
               h, cap.min_main.height);
          h = cap.min_main.height;
        } else if (h > cap.max_main.height) {
          WARN("Main buffer max height[%d] is too large, reset to %d.",
               h, cap.max_main.height);
          h = cap.max_main.height;
        }
      }
      if (AM_LIKELY((m_resource_param.buf_extra_dram.second[i] >= 0) &&
                    (m_resource_param.buf_extra_dram.second[i] <= 7))) {
        extra_buf = m_resource_param.buf_extra_dram.second[i];
      } else {
        WARN("extra DRAM buffer number valid range is from 0 to 7,"
              " default is 0");
        extra_buf = 0;
      }

      NOTICE("buffer%d max size = %d x %d\n", i, w, h);
      resource.buf_max_size[i].width = w;
      resource.buf_max_size[i].height = h;
      resource.extra_dram_buf[i] = extra_buf;
    }

    AMBufferParamMap::const_iterator iter = buffer.find(AM_SOURCE_BUFFER_EFM);
    if (iter != buffer.end()) {
      resource.efm_size.width = iter->second.size.second.width;
      resource.efm_size.height = iter->second.size.second.height;
    }

    for (uint32_t i = 0; i < m_resource_param.max_num_encode.second; ++i) {
      uint32_t width = m_resource_param.stream_max_size.second[i].width;
      uint32_t height = m_resource_param.stream_max_size.second[i].height;

      if (width < cap.min_enc.width) {
        WARN("Stream[%d] width: %d is small, set to %d.",
             i, width, cap.min_enc.width);
        width = cap.min_enc.width;
      }

      if (height < cap.min_enc.height) {
        WARN("Stream[%d] height: %d is small, set to %d.",
             i, height, cap.min_enc.height);
        height = cap.min_enc.height;
      }

      if (stream.at(AM_STREAM_ID(i)).stream_format.second.rotate_90_cw.second) {
        resource.stream_max_size[i].width = height;
        resource.stream_max_size[i].height = width;
      } else {
        resource.stream_max_size[i].width = width;
        resource.stream_max_size[i].height = height;
      }

      if (stream.at(AM_STREAM_ID(i)).stream_format.second.type.second ==
          AM_STREAM_TYPE_MJPEG) {
        resource.stream_max_M[i] = 0;
        resource.stream_max_N[i] = 0;
        resource.stream_long_ref_enable[i] = false;
        resource.stream_max_advanced_quality_model[i] = 0;
      } else {
        resource.stream_max_M[i] = m_resource_param.stream_max_M.second[i];
        resource.stream_max_N[i] = m_resource_param.stream_max_N.second[i];
        resource.stream_long_ref_enable[i] =
            m_resource_param.stream_long_ref_possible.second[i] ? 1 : 0;
        resource.stream_max_advanced_quality_model[i] =
            m_resource_param.stream_max_advanced_quality_model.second[i];
      }
    }

    resource.encode_mode =
        AMVideoTransIav2::encode_mode_mw_to_iav(m_feature_param.mode.second);
    resource.exposure_num =
        AMVideoTrans::get_hdr_expose_num(m_feature_param.hdr.second);
    uint32_t max_stream_num = m_resource_param.max_num_encode.second;
    if (max_stream_num > cap.max_streams_num) {
      WARN("Max stream number: %d is large, set to %d.",
           max_stream_num, cap.max_streams_num);
      max_stream_num = cap.max_streams_num;
    }
    resource.max_num_encode = max_stream_num;
    resource.max_num_cap_sources = m_resource_param.max_num_cap_sources.second;
    resource.dsp_partition_map = m_resource_param.dsp_partition_map.first ?
        m_resource_param.dsp_partition_map.second : 0;
    resource.rotate_enable = cap.rotate_possible ?
        m_resource_param.rotate_possible.second : false;
    resource.raw_capture_enable = cap.raw_cap_possible ?
        m_resource_param.raw_capture_possible.second : false;
    resource.enc_raw_rgb = cap.enc_raw_rgb_possible ?
        m_resource_param.enc_raw_rgb_possible.second : false;
    resource.enc_raw_yuv = cap.enc_raw_yuv_possible ?
        m_resource_param.enc_raw_yuv_possible.second : false;
    resource.enc_from_mem = cap.enc_from_mem_possible ?
        m_resource_param.enc_from_mem_possible.second : false;
    resource.efm_buf_num = m_resource_param.efm_buf_num.second;
    resource.mixer_a_enable = m_resource_param.mixer_a_enable.second;
    resource.mixer_b_enable = cap.mixer_b_possible ?
        m_resource_param.mixer_b_enable.second : false;
    resource.raw_size.width = m_resource_param.raw_max_size.second.width;
    resource.raw_size.height = m_resource_param.raw_max_size.second.height;

    resource.v_warped_main_max_width =
        m_resource_param.v_warped_main_max_size.second.width;
    resource.v_warped_main_max_height =
        m_resource_param.v_warped_main_max_size.second.height;
    resource.max_warp_output_width = m_resource_param.max_warp_output_width.second;
    resource.max_warp_input_width = m_resource_param.max_warp_input_size.second.width;
    resource.max_warp_input_height = m_resource_param.max_warp_input_size.second.height;
    resource.max_padding_width = m_resource_param.max_padding_width.second;
    resource.enc_dummy_latency = m_resource_param.enc_dummy_latency.second;
    resource.idsp_upsample_type = m_resource_param.idsp_upsample_type.second;
    resource.vout_swap_enable = cap.vout_swap_possible ?
        m_resource_param.vout_swap_possible.second : false;
    resource.lens_warp_enable = cap.lens_warp_possible ?
        m_resource_param.lens_warp_possible.second : false;
    resource.eis_delay_count = m_resource_param.eis_delay_count.second;
    resource.me0_scale = m_resource_param.me0_scale.second;

    if (m_feature_param.mode.second == AM_ENCODE_MODE_4){
      if (m_resource_param.debug_iso_type.second != -1) {
        resource.debug_iso_type = m_resource_param.debug_iso_type.second;
        resource.debug_enable_map |= DEBUG_TYPE_ISO_TYPE;
      } else {
        resource.debug_enable_map &= ~DEBUG_TYPE_ISO_TYPE;
      }
    }

    if (m_resource_param.debug_chip_id.second != -1) {
      resource.debug_chip_id = m_resource_param.debug_chip_id.second;
      resource.debug_enable_map |= DEBUG_TYPE_CHIP_ID;
    } else {
      resource.debug_enable_map &= ~DEBUG_TYPE_CHIP_ID;
    }

    if (m_resource_param.debug_stitched.second != -1) {
      resource.debug_stitched = m_resource_param.debug_stitched.second;
      resource.debug_enable_map |= DEBUG_TYPE_STITCH;
    } else {
      resource.debug_enable_map &= ~DEBUG_TYPE_STITCH;
    }

    if (ioctl(m_iav, IAV_IOC_SET_SYSTEM_RESOURCE, &resource) < 0) {
      PERROR("IAV_IOC_SET_SYSTEM_RESOURCE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::system_resource_get(AMPlatformResourceLimit &res)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

uint32_t  AMPlatformIav2::system_max_stream_num_get()
{
  return IAV_STREAM_MAX_NUM_ALL;
}

uint32_t  AMPlatformIav2::system_max_buffer_num_get()
{
  return IAV_SRCBUF_LAST;
}

AM_RESULT AMPlatformIav2::goto_idle()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (ioctl(m_iav, IAV_IOC_ENTER_IDLE, 0) < 0) {
      PERROR("IAV_IOC_ENTER_IDLE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::goto_preview()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (ioctl(m_iav, IAV_IOC_ENABLE_PREVIEW, 0) < 0) {
      PERROR("IAV_IOC_ENABLE_PREVIEW");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (m_resource_param.dsp_partition_map.second) {
    /* call map_dsp function to remmap dsp sub buffer partition, because other
     * modules maybe call map_dsp before to mmap dsp buffer not dsp sub buffer*/
      if (m_is_dsp_mapped) {
        unmap_dsp();
      }
      if (!m_is_dsp_sub_mapped) {
        map_dsp();
      }
    } else {
      //If preview state is dsp sub buf partition, we need do dsp mmap here
      map_dsp();
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::encode_start(uint32_t stream_bits)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (ioctl(m_iav, IAV_IOC_START_ENCODE, stream_bits) < 0) {
      PERROR("IAV_IOC_START_ENCODE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::encode_stop(uint32_t stream_bits)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (ioctl(m_iav, IAV_IOC_STOP_ENCODE, stream_bits) < 0) {
      PERROR("IAV_IOC_STOP_ENCODE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::decode_start()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::decode_stop()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::iav_state_get(AM_IAV_STATE &state)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_state iavstate;

  do {
    if (ioctl(m_iav, IAV_IOC_GET_IAV_STATE, &iavstate) < 0) {
      PERROR("IAV_IOC_GET_IAV_STATE");
      state = AM_IAV_STATE_ERROR;
      break;
    }
    state = AMVideoTransIav2::iav_state_iav_to_mw(iavstate);
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::get_power_mode(AM_POWER_MODE &mode)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    //TODO:
  }while (0);

  return result;
}

AM_RESULT AMPlatformIav2::set_power_mode(AM_POWER_MODE mode)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    //TODO:
  }while (0);

  return result;
}


AM_RESULT AMPlatformIav2::query_frame(AMPlatformFrameInfo &frame)
{
  AM_RESULT result = AM_RESULT_OK;
  iav_querydesc query_desc;
  memset(&query_desc, 0, sizeof(iav_querydesc));

  do {
    bool is_ok = true;
    switch (frame.type) {
      case AM_DATA_FRAME_TYPE_VIDEO: {
        query_desc.qid = IAV_DESC_FRAME;
        query_desc.arg.frame.id = (uint32_t)-1;
        query_desc.arg.frame.time_ms = frame.timeout;
      } break;
      case AM_DATA_FRAME_TYPE_YUV: {
        query_desc.qid = IAV_DESC_YUV;
        query_desc.arg.yuv.buf_id =
            AMVideoTransIav2::buffer_id_mw_to_iav(frame.buf_id);
        if (frame.latest) {
          query_desc.arg.yuv.flag |= IAV_BUFCAP_NONBLOCK;
        } else {
          query_desc.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
        }
      } break;
      case AM_DATA_FRAME_TYPE_RAW: {
        query_desc.qid = IAV_DESC_RAW;
        query_desc.arg.raw.flag &= ~IAV_BUFCAP_NONBLOCK;
      } break;
      case AM_DATA_FRAME_TYPE_ME0: {
        query_desc.qid = IAV_DESC_ME0;
        query_desc.arg.me0.buf_id =
            AMVideoTransIav2::buffer_id_mw_to_iav(frame.buf_id);
        if (frame.latest) {
          query_desc.arg.me0.flag |= IAV_BUFCAP_NONBLOCK;
        } else {
          query_desc.arg.me0.flag &= ~IAV_BUFCAP_NONBLOCK;
        }
      } break;
      case AM_DATA_FRAME_TYPE_ME1: {
        query_desc.qid = IAV_DESC_ME1;
        query_desc.arg.me1.buf_id =
            AMVideoTransIav2::buffer_id_mw_to_iav(frame.buf_id);
        if (frame.latest) {
          query_desc.arg.me1.flag |= IAV_BUFCAP_NONBLOCK;
        } else {
          query_desc.arg.me1.flag &= ~IAV_BUFCAP_NONBLOCK;
        }
      } break;
      case AM_DATA_FRAME_TYPE_VCA: {
        ERROR("Video Frame Type: %d is not supported!", frame.type);
        is_ok = false;
        result = AM_RESULT_ERR_INVALID;
      } break;
      default:
        break;
    }

    if (AM_UNLIKELY(!is_ok)) {
      break;
    }

    if (ioctl(m_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
      if (errno == EINTR) {
        result = AM_RESULT_ERR_IO;
      } else if (errno == EAGAIN) {
        result = AM_RESULT_ERR_AGAIN;
      } else {
        result = AM_RESULT_ERR_DSP;
      }
      if (errno != EAGAIN) {
        PERROR("IAV_IOC_QUERY_DESC");
      }
      break;
    }

    frame.desc.type = frame.type;
    switch (frame.type) {
      case AM_DATA_FRAME_TYPE_VIDEO: {
        iav_framedesc &iav_frame = query_desc.arg.frame;
        frame.desc.video.type =
            AMVideoTransIav2::frame_type_iav_to_mw(iav_frame.pic_type);
        frame.desc.video.stream_type =
            AMVideoTransIav2::stream_type_iav_to_mw(iav_frame.stream_type);
        frame.desc.video.data_offset = iav_frame.data_addr_offset;
        frame.desc.video.data_size = iav_frame.size;
        frame.desc.video.frame_num = iav_frame.frame_num;
        frame.desc.video.stream_id = iav_frame.id;
        frame.desc.video.width = iav_frame.reso.width;
        frame.desc.video.height = iav_frame.reso.height;
        frame.desc.video.jpeg_quality = iav_frame.jpeg_quality;
        frame.desc.video.session_id = iav_frame.session_id;
        frame.desc.video.stream_end_flag = iav_frame.stream_end;
        frame.desc.video.tile_slice.slice_num = 1;
        frame.desc.video.tile_slice.slice_id  = 0;
        frame.desc.video.tile_slice.tile_num  = 1;
        frame.desc.video.tile_slice.tile_id   = 0;
        frame.desc.pts = iav_frame.arm_pts;
      } break;
      case AM_DATA_FRAME_TYPE_YUV: {
        frame.desc.yuv.non_block_flag = 0;
        frame.desc.yuv.buffer_id = AMVideoTransIav2::buffer_id_iav_to_mw(
            iav_srcbuf_id(query_desc.arg.yuv.buf_id));
        frame.desc.yuv.format = AMVideoTransIav2::chroma_iav_to_mw(
            (iav_chroma_format)query_desc.arg.yuv.format);
        frame.desc.yuv.height = query_desc.arg.yuv.height;
        frame.desc.yuv.width = query_desc.arg.yuv.width;
        frame.desc.yuv.pitch = query_desc.arg.yuv.pitch;
        frame.desc.yuv.seq_num = query_desc.arg.yuv.seq_num;
        frame.desc.yuv.uv_offset = query_desc.arg.yuv.uv_addr_offset;
        frame.desc.yuv.y_offset = query_desc.arg.yuv.y_addr_offset;
        frame.desc.pts = query_desc.arg.yuv.mono_pts;
      } break;
      case AM_DATA_FRAME_TYPE_RAW: {
        frame.desc.raw.width = query_desc.arg.raw.width;
        frame.desc.raw.height = query_desc.arg.raw.height;
        frame.desc.raw.pitch = query_desc.arg.raw.pitch;
        frame.desc.raw.data_offset = query_desc.arg.raw.raw_addr_offset;
        frame.desc.pts = query_desc.arg.raw.mono_pts;
      } break;
      case AM_DATA_FRAME_TYPE_ME0: {
        frame.desc.me.non_block_flag = 0;
        frame.desc.me.buffer_id = AMVideoTransIav2::buffer_id_iav_to_mw(
            iav_srcbuf_id(query_desc.arg.me0.buf_id));
        frame.desc.me.data_offset = query_desc.arg.me0.data_addr_offset;
        frame.desc.me.width = query_desc.arg.me0.width;
        frame.desc.me.height = query_desc.arg.me0.height;
        frame.desc.me.pitch = query_desc.arg.me0.pitch;
        frame.desc.me.seq_num = query_desc.arg.me0.seq_num;
        frame.desc.pts = query_desc.arg.me0.mono_pts;
      } break;
      case AM_DATA_FRAME_TYPE_ME1: {
        frame.desc.me.non_block_flag = 0;
        frame.desc.me.buffer_id = AMVideoTransIav2::buffer_id_iav_to_mw(
            iav_srcbuf_id(query_desc.arg.me1.buf_id));
        frame.desc.me.data_offset = query_desc.arg.me1.data_addr_offset;
        frame.desc.me.width = query_desc.arg.me1.width;
        frame.desc.me.height = query_desc.arg.me1.height;
        frame.desc.me.pitch = query_desc.arg.me1.pitch;
        frame.desc.me.seq_num = query_desc.arg.me1.seq_num;
        frame.desc.pts = query_desc.arg.me1.mono_pts;
      } break;
      default:
        break;
    }
  } while (0);

  return result;
}

AM_RESULT AMPlatformIav2::force_idr(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_stream_cfg stream_cfg;
  memset(&stream_cfg, 0, sizeof(stream_cfg));

  stream_cfg.id = id;
  stream_cfg.cid = IAV_H264_CFG_FORCE_IDR;
  if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
    result = AM_RESULT_ERR_DSP;
  }

  return result;
}

AM_RESULT AMPlatformIav2::overlay_set(const AMOverlayInsert &overlay)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_IAV_STATE state;
    if ((result=iav_state_get(state)) != AM_RESULT_OK) {
      break;
    } else {
      if ((state != AM_IAV_STATE_PREVIEW) && (state != AM_IAV_STATE_ENCODING)) {
        result = AM_RESULT_ERR_PERM;
        WARN("IAV must be in PREVIEW or ENCODE to set overlay.\n");
        break;
      }
    }

    struct iav_overlay_insert overlay_insert = {0};
    overlay_insert.id = overlay.stream_id;
    if (ioctl(m_iav, IAV_IOC_GET_OVERLAY_INSERT, &overlay_insert) < 0) {
      result = AM_RESULT_ERR_IO;
      perror("IAV_IOC_SET_OVERLAY_INSERT");
      break;
    }
    struct iav_overlay_area *area_insert = nullptr;
    overlay_insert.enable = overlay.enable;
    for (uint32_t i = 0; i<OVERLAY_AREA_MAX_NUM && i<overlay.area.size(); ++i) {
      area_insert = &overlay_insert.area[i];
      const AMOverlayAreaInsert &area = overlay.area[i];
      area_insert->enable = area.enable;
      area_insert->start_x = area.start_x;
      area_insert->start_y = area.start_y;
      area_insert->width = area.width;
      area_insert->height = area.height;
      area_insert->pitch = area.pitch;
      area_insert->total_size = area.total_size;
      area_insert->clut_addr_offset = area.clut_addr_offset;
      area_insert->data_addr_offset = area.data_addr_offset;
    }

    if (ioctl(m_iav, IAV_IOC_SET_OVERLAY_INSERT, &overlay_insert) < 0) {
      result = AM_RESULT_ERR_IO;
      perror("IAV_IOC_SET_OVERLAY_INSERT");
      break;
    }
  } while(0);

  return result;
}

AM_RESULT AMPlatformIav2::overlay_get(AMOverlayInsert &overlay)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_IAV_STATE state;
    if ((result=iav_state_get(state)) != AM_RESULT_OK) {
      break;
    } else {
      if ((state != AM_IAV_STATE_PREVIEW) && (state != AM_IAV_STATE_ENCODING)) {
        result = AM_RESULT_ERR_PERM;
        ERROR("IAV must be in PREVIEW or ENCODE to get overlay.\n");
        break;
      }
    }

    struct iav_overlay_insert overlay_insert;
    struct iav_overlay_area *area_insert = nullptr;
    memset(&overlay_insert, 0, sizeof(overlay_insert));
    overlay_insert.id = overlay.stream_id;
    if (ioctl(m_iav, IAV_IOC_GET_OVERLAY_INSERT, &overlay_insert) < 0) {
      result = AM_RESULT_ERR_IO;
      perror("IAV_IOC_GET_OVERLAY_INSERT");
      break;
    }
    for (int i = 0; i < OVERLAY_AREA_MAX_NUM; ++i) {
      area_insert = &overlay_insert.area[i];
      AMOverlayAreaInsert &area = overlay.area[i];
      area.enable = area_insert->enable;
      area.start_x = area_insert->start_x;
      area.start_y = area_insert->start_y;
      area.width = area_insert->width;
      area.height = area_insert->height;
      area.pitch = area_insert->pitch;
      area.total_size = area_insert->total_size;
      area.clut_addr_offset = area_insert->clut_addr_offset;
      area.data_addr_offset = area_insert->data_addr_offset;

      INFO("\nstream:%d area:%d is %s, offset(%d x %d), W x H = %d x %d, "
           "pitch = %d, total_size = %d, clut_offset = %d, data_offset = %d",
           overlay_insert.id,
           i,
           (area_insert->enable == 1) ? "enable" : "disable",
           area_insert->start_x,
           area_insert->start_y,
           area_insert->width,
           area_insert->height,
           area_insert->pitch,
           area_insert->total_size,
           area_insert->clut_addr_offset,
           area_insert->data_addr_offset);
    }
  } while(0);

  return result;
}

uint32_t   AMPlatformIav2::overlay_max_area_num()
{
  return OVERLAY_AREA_MAX_NUM;
}

AM_RESULT AMPlatformIav2::map_overlay(AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AUTO_LOCK_PLATFORM(m_mtx);
    if (m_is_overlay_mapped) {
      mem = m_overlay_mem;
      break;
    }

    iav_querybuf query_buf;
    query_buf.buf = IAV_BUFFER_OVERLAY;
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &query_buf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    uint32_t overlay_size = query_buf.length;
    void *overlay_mem = mmap(nullptr, overlay_size, PROT_WRITE,
                             MAP_SHARED, m_iav, query_buf.offset);
    if (overlay_mem == MAP_FAILED) {
      PERROR("mmap");
      result = AM_RESULT_ERR_IO;
      break;
    }
    m_overlay_mem.addr = (uint8_t*)overlay_mem;
    m_overlay_mem.length = overlay_size;
    mem = m_overlay_mem;
    m_is_overlay_mapped = true;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::unmap_overlay()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_overlay_mapped && m_overlay_mem.addr) {
      if (munmap(m_overlay_mem.addr, m_overlay_mem.length) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_overlay_mem.addr = 0;
      m_overlay_mem.length = 0;
      m_is_overlay_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::map_dsp()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_dsp_mapped || m_is_dsp_sub_mapped) {
      break;
    }

    AM_IAV_STATE state = AM_IAV_STATE_INIT;
    iav_state_get(state);
    iav_system_resource resource = {0};
    resource.encode_mode = DSP_CURRENT_MODE;
    if (ioctl(m_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
      PERROR("IAV_IOC_GET_SYSTEM_RESOURCE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (resource.dsp_partition_map) {
      if ((state != AM_IAV_STATE_PREVIEW) &&
          (state != AM_IAV_STATE_ENCODING)) {
        /* dsp sub buf partition just can set in preview or encoding state,
         * so do mmap in goto_preview() later */
        break;
      }
      iav_query_subbuf query;
      memset(&query, 0, sizeof(query));
      query.buf_id = IAV_BUFFER_DSP;
      query.sub_buf_map = m_resource_param.dsp_partition_map.second;
      if (ioctl(m_iav, IAV_IOC_QUERY_SUB_BUF, &query) < 0) {
        PERROR("IAV_IOC_QUERY_SUB_BUF");
        result = AM_RESULT_ERR_DSP;
        break;
      }
      for (int32_t i = 0; i < IAV_DSP_SUB_BUF_NUM; ++i) {
        if (query.sub_buf_map & (1 << i)) {
          iav_dsp_sub_buffer_id id = iav_dsp_sub_buffer_id(i);
          m_dsp_sub_mem[id].length = query.length[i];
          m_dsp_sub_mem[id].addr = (uint8_t*)mmap(NULL, query.length[i],
                                                 PROT_READ, MAP_SHARED,
                                                 m_iav, query.daddr[i]);
          if (m_dsp_sub_mem[id].addr == MAP_FAILED) {
            PERROR("mmap");
            m_dsp_sub_mem.erase(m_dsp_sub_mem.find(id));
            result = AM_RESULT_ERR_IO;
            break;
          }
        }
      }
      if (result != AM_RESULT_OK) {
        for (auto &m : m_dsp_sub_mem) {
          munmap(m.second.addr, m.second.length);
        }
        m_dsp_sub_mem.clear();
        break;
      }
      m_is_dsp_sub_mapped = true;
    } else {
      iav_querybuf query_buf = {IAV_BUFFER_DSP};
      if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &query_buf) < 0) {
        PERROR("IAV_IOC_QUERY_BUF");
        result = AM_RESULT_ERR_DSP;
        break;
      }
      m_dsp_mem.length = query_buf.length;
      m_dsp_mem.addr = (uint8_t*)mmap(nullptr, query_buf.length,
                                      PROT_READ, MAP_SHARED,
                                      m_iav, query_buf.offset);
      if (m_dsp_mem.addr == MAP_FAILED) {
        PERROR("mmap");
        m_dsp_mem.length = 0;
        result = AM_RESULT_ERR_IO;
        break;
      }
      m_is_dsp_mapped = true;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::unmap_dsp()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_dsp_mapped && m_dsp_mem.addr) {
      if (munmap(m_dsp_mem.addr, m_dsp_mem.length) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_dsp_mem.addr = 0;
      m_dsp_mem.length = 0;
      m_is_dsp_mapped = false;
    } else if (m_is_dsp_sub_mapped) {
      for (auto &m : m_dsp_sub_mem) {
        if (munmap(m.second.addr, m.second.length) < 0) {
          PERROR("munmap");
          result = AM_RESULT_ERR_MEM;
          break;
        }
      }
      m_dsp_sub_mem.clear();
      m_is_dsp_sub_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::get_dsp_mmap_info(AM_DSP_SUB_BUF_ID id,
                                            AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_dsp_mapped) {
      mem = m_dsp_mem;
    } else if (m_is_dsp_sub_mapped) {
      iav_dsp_sub_buffer_id iav_id =
          AMVideoTransIav2::dsp_sub_buf_id_mw_to_iav(id);
      AMDspSubMemMapInfoMap::iterator itr = m_dsp_sub_mem.find(iav_id);
      if (itr == m_dsp_sub_mem.end()) {
        ERROR("Dsp sub buf%d is not mmaped!",id);
        result = AM_RESULT_ERR_MEM;
        break;
      }
      mem = itr->second;
    } else {
      ERROR("Dsp memory is not mmaped!");
      result = AM_RESULT_ERR_MEM;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::map_bsb(AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_bsb_mapped) {
      mem = m_bsb_mem;
      break;
    }

    iav_querybuf query_buf = {IAV_BUFFER_BSB};
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &query_buf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    uint32_t bsb_size = query_buf.length;
    void *bsb_mem = mmap(nullptr, bsb_size * 2,
                         PROT_READ,
                         MAP_SHARED,
                         m_iav, query_buf.offset);
    if (bsb_mem == MAP_FAILED) {
      PERROR("mmap");
      result = AM_RESULT_ERR_IO;
      break;
    }
    m_bsb_mem.addr = (uint8_t*)bsb_mem;
    m_bsb_mem.length = bsb_size;
    mem = m_bsb_mem;
    m_is_bsb_mapped = true;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::unmap_bsb()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_bsb_mapped && m_bsb_mem.addr) {
      if (munmap(m_bsb_mem.addr, m_bsb_mem.length * 2) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_bsb_mem.addr = 0;
      m_bsb_mem.length = 0;
      m_is_bsb_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::map_warp(AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_warp_mapped) {
      mem = m_warp_mem;
      break;
    }

    iav_querybuf querybuf = {IAV_BUFFER_WARP};
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    uint32_t warp_size = querybuf.length;
    void *warp_mem = mmap(nullptr, warp_size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          m_iav, querybuf.offset);
    if (warp_mem == MAP_FAILED) {
      PERROR("mmap");
      result = AM_RESULT_ERR_IO;
      break;
    }

    m_warp_mem.addr = (uint8_t *) warp_mem;
    m_warp_mem.length = warp_size;
    mem = m_warp_mem;
    m_is_warp_mapped = true;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::unmap_warp()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_warp_mapped && m_warp_mem.addr) {
      if (munmap(m_warp_mem.addr, m_warp_mem.length) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_warp_mem.addr = 0;
      m_warp_mem.length = 0;
      m_is_warp_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::map_usr(AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_usr_mapped) {
      mem = m_usr_mem;
      break;
    }

    iav_querybuf querybuf = {IAV_BUFFER_USR};
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    uint32_t usr_size = querybuf.length;
    void *usr_mem = mmap(nullptr, usr_size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          m_iav, querybuf.offset);
    if (usr_mem == MAP_FAILED) {
      PERROR("mmap");
      result = AM_RESULT_ERR_IO;
      break;
    }

    m_usr_mem.addr = (uint8_t *) usr_mem;
    m_usr_mem.length = usr_size;
    mem = m_usr_mem;
    m_is_usr_mapped = true;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::unmap_usr()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_usr_mapped && m_usr_mem.addr) {
      if (munmap(m_usr_mem.addr, m_usr_mem.length) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_usr_mem.addr = 0;
      m_usr_mem.length = 0;
      m_is_usr_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::gdmacpy(void *dst, const void *src,
                                  size_t width, size_t height, size_t pitch)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_gdma_copy gdma_param;

  do {
    gdma_param.src_offset = (u32)src;
    gdma_param.dst_offset = (u32)dst;
    gdma_param.src_mmap_type = (u16)IAV_BUFFER_DSP;
    gdma_param.dst_mmap_type = (u16)IAV_BUFFER_USR;

    gdma_param.src_pitch = (u16)pitch;
    gdma_param.dst_pitch = (u16)width;
    gdma_param.width = (u16)width;
    gdma_param.height = (u16)height;
    if (ioctl(m_iav, IAV_IOC_GDMA_COPY, &gdma_param) < 0) {
      ERROR("iav gdma copy failed:%d:%s", errno, strerror(errno));
      ERROR("iav gdma copy dst=%p, src=%p, w=%d, h=%d, p=%d",
             dst, src, width, height, pitch);
      result = AM_RESULT_ERR_MEM;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::encode_mode_get(AM_ENCODE_MODE &mode)
{
  mode = m_feature_param.mode.second;
  return AM_RESULT_OK;
}

AM_RESULT AMPlatformIav2::hdr_type_get(AM_HDR_TYPE &hdr)
{
  hdr = m_feature_param.hdr.second;
  return AM_RESULT_OK;
}

AM_RESULT AMPlatformIav2::check_ldc_enable(bool &enable)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_system_resource resource;
  memset(&resource, 0, sizeof(resource));
  do {
    resource.encode_mode = DSP_CURRENT_MODE;
    if (ioctl(m_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
      PERROR("IAV_IOC_GET_SYSTEM_RESOURCE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    enable = resource.lens_warp_enable;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::get_aaa_info(AMVinAAAInfo &info)
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_aaa_info aaa;
  memset(&aaa, 0, sizeof(aaa));
  aaa.vsrc_id = 0;

  do {
    if (ioctl(m_iav, IAV_IOC_VIN_GET_AAAINFO, &aaa) < 0) {
      ERROR("IAV_IOC_VIN_GET_AAAINFO failed");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    info.vsrc_id = aaa.vsrc_id;
    info.sensor_id = aaa.sensor_id;
    info.bayer_pattern = aaa.bayer_pattern;
    info.agc_step = aaa.agc_step;
    info.hdr_mode = aaa.hdr_mode;
    info.hdr_long_offset = aaa.hdr_long_offset;
    info.hdr_short1_offset = aaa.hdr_short1_offset;
    info.hdr_short2_offset = aaa.hdr_short2_offset;
    info.hdr_short3_offset = aaa.hdr_short3_offset;
    info.pixel_size = aaa.pixel_size;
    info.dual_gain_mode = aaa.dual_gain_mode;
    info.line_time = aaa.line_time;
    info.vb_time = aaa.vb_time;
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::set_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_digital_zoom dptz;
  memset(&dptz, 0, sizeof(dptz));
  dptz.buf_id = id;
  dptz.input.width = rect.size.width;
  dptz.input.height = rect.size.height;
  dptz.input.x = rect.offset.x;
  dptz.input.y = rect.offset.y;
  INFO("AMDPTZWarp: source buffer %d, dz input width=%d, height=%d, x=%d, y=%d\n",
         dptz.buf_id,
         dptz.input.width,
         dptz.input.height,
         dptz.input.x,
         dptz.input.y);

  do {
    if (ioctl(m_iav, IAV_IOC_SET_DIGITAL_ZOOM, &dptz) < 0) {
      PERROR("IAV_IOC_SET_DIGITAL_ZOOM");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMPlatformIav2::get_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_digital_zoom dptz;

  do {
    memset(&dptz, 0, sizeof(dptz));
    dptz.buf_id = id;
    if (ioctl(m_iav, IAV_IOC_GET_DIGITAL_ZOOM, &dptz) < 0) {
      PERROR("IAV_IOC_GET_DIGITAL_ZOOM");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    rect.size.width = dptz.input.width;
    rect.size.height = dptz.input.height;
    rect.offset.x = dptz.input.x;
    rect.offset.y = dptz.input.y;

  } while (0);
  return result;
}

bool AMPlatformIav2::has_warp()
{
  return (m_feature_param.dewarp_func.second != AM_DEWARP_NONE);
}

AM_DEWARP_FUNC_TYPE AMPlatformIav2::get_dewarp_func()
{
  return m_feature_param.dewarp_func.second;
}

AM_DPTZ_TYPE AMPlatformIav2::get_dptz()
{
  return m_feature_param.dptz.second;
}

AM_OVERLAY_TYPE AMPlatformIav2::get_overlay()
{
  return m_feature_param.overlay.second;
}

AM_VIDEO_MD_TYPE AMPlatformIav2::get_video_md()
{
  return m_feature_param.video_md.second;
}

AM_RESULT AMPlatformIav2::get_avail_cpu_clks(vector<int32_t> &cpu_clks)
{
  return AM_RESULT_OK;
}

AM_RESULT AMPlatformIav2::get_cur_cpu_clk(int32_t &cur_clk)
{
  return AM_RESULT_OK;
}
AM_RESULT AMPlatformIav2::set_cpu_clk(const int32_t &index)
{
  return AM_RESULT_OK;
}

AM_BITRATE_CTRL_METHOD AMPlatformIav2::get_bitrate_ctrl_method()
{
  return m_feature_param.bitrate_ctrl.second;
}

AM_RESULT AMPlatformIav2::map_eis_warp(AMMemMapInfo &mem)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_eis_warp_mapped) {
      mem = m_eis_warp_mem;
      break;
    }

    iav_querybuf querybuf = {IAV_BUFFER_WARP};
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    uint32_t warp_size = querybuf.length;
    void *warp_mem = mmap(nullptr, warp_size,
                          PROT_WRITE,
                          MAP_SHARED,
                          m_iav, querybuf.offset);
    if (warp_mem == MAP_FAILED) {
      PERROR("mmap");
      result = AM_RESULT_ERR_IO;
      break;
    }

    memset(warp_mem, 0, warp_size);
    m_eis_warp_mem.addr = (uint8_t *) warp_mem;
    m_eis_warp_mem.length = warp_size;
    mem = m_eis_warp_mem;
    m_is_eis_warp_mapped = true;
  } while(0);

  return result;
}

AM_RESULT AMPlatformIav2::unmap_eis_warp()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_is_eis_warp_mapped && m_eis_warp_mem.addr) {
      if (munmap(m_eis_warp_mem.addr, m_eis_warp_mem.length) < 0) {
        PERROR("munmap");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      m_eis_warp_mem.addr = 0;
      m_eis_warp_mem.length = 0;
      m_is_eis_warp_mapped = false;
    }
  } while(0);

  return result;
}

static AMDSPCapability g_dsp_cap_by_modes[AM_ENCODE_MODE_NUM] =
{
 /*
    bool basic_hdr;
    bool advanced_hdr;
    bool normal_iso;
    bool normal_plus_iso;
    bool advanced_iso;
    bool single_dewarp;
    bool multi_dewarp;
  */
 { 1, 0, 1, 0,   0, 1, 0}, //mode 0, simplest mode
 { 0, 0, 0, 0,   0, 0, 0}, //mode 1 (future)
 { 0, 0, 0, 0,   0, 0, 0}, //mode 2 (reserved)
 { 0, 0, 0, 0,   0, 0, 0}, //mode 3 (not used)
 { 1, 0, 0, 1,   1, 1, 0}, //mode 4, advanced ISO
 { 0, 1, 0, 1,   0, 0, 0}, //mode 5, advanced HDR
};

AM_RESULT AMPlatformIav2::select_mode_by_features(AMFeatureParam &param)
{
  AM_RESULT result = AM_RESULT_OK;

  if (param.mode.second == AM_ENCODE_MODE_AUTO) {
    AMDSPCapability dsp_cap;

    dsp_cap.basic_hdr = (param.hdr.second == AM_HDR_2_EXPOSURE);
    dsp_cap.advanced_hdr = (param.hdr.second == AM_HDR_3_EXPOSURE);
    dsp_cap.normal_iso = (param.iso.second == AM_IMAGE_NORMAL_ISO);
    dsp_cap.normal_iso_plus = (param.iso.second == AM_IMAGE_ISO_PLUS);
    dsp_cap.advanced_iso = (param.iso.second == AM_IMAGE_ADVANCED_ISO);
    dsp_cap.single_dewarp = (param.dewarp_func.second == AM_DEWARP_LDC);

    for (uint32_t i = AM_ENCODE_MODE_FIRST; i < AM_ENCODE_MODE_NUM; ++i) {
      if ((!dsp_cap.basic_hdr || g_dsp_cap_by_modes[i].basic_hdr) &&
          (!dsp_cap.advanced_hdr || g_dsp_cap_by_modes[i].advanced_hdr) &&
          (!dsp_cap.normal_iso || g_dsp_cap_by_modes[i].normal_iso) &&
          (!dsp_cap.normal_iso_plus || g_dsp_cap_by_modes[i].normal_iso_plus) &&
          (!dsp_cap.advanced_iso || g_dsp_cap_by_modes[i].advanced_iso) &&
          (!dsp_cap.single_dewarp || g_dsp_cap_by_modes[i].single_dewarp)) {
        param.mode.second = AM_ENCODE_MODE(i);
        break;
      }
    }
    if (param.mode.second == AM_ENCODE_MODE_AUTO) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Failed to find encode mode!");
    }
  }

  return result;
}

AMPlatformIav2* AMPlatformIav2::get_instance()
{
  AUTO_LOCK_PLATFORM(m_mtx);
  if (!m_instance) {
    m_instance = AMPlatformIav2::create();
  }
  return m_instance;
}

AMPlatformIav2* AMPlatformIav2::create()
{
  AMPlatformIav2 *result = new AMPlatformIav2();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMPlatformIav2::inc_ref()
{
  ++ m_ref_cnt;
}

void AMPlatformIav2::release()
{
  AUTO_LOCK_PLATFORM(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AM_RESULT AMPlatformIav2::init()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((m_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("open /dev/iav");
      break;
    }

    if (!(m_feature_config = AMFeatureConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to get AMFeatureConfig instance!");
      break;
    }

    if (!(m_resource_config = AMResourceConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMResourceConfig!");
      break;
    }
  } while (0);
  return result;
}

AMPlatformIav2::AMPlatformIav2()
{
  DEBUG("AMPlatform is created!");
}

AMPlatformIav2::~AMPlatformIav2()
{
  if (m_iav >= 0) {
    close(m_iav);
  }

  unmap_dsp();
  unmap_overlay();
  DEBUG("AMPlatform is destroyed!");
}
