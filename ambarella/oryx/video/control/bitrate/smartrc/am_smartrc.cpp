/*******************************************************************************
 * am_smartrc.cpp
 *
 * History:
 *   Jul 4, 2016 - [binwang] created file
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

#include "am_encode_config.h"
#include "am_encode_stream.h"
#include "am_smartrc.h"
#include "am_motion_detect_types.h"
#include "am_video_utility.h"

#include "am_thread.h"
#include "am_vin.h"

#define MB_SIZE (16)
#define CTB_SIZE (32)

#define NOISE_NONE_DELAY 120
#define NOISE_LOW_DELAY  2
#define NOISE_HIGH_DELAY 0

#define LOCK_GUARD(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

#ifdef __cplusplus
extern "C"
{
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  AMPluginData *plugin_data = ((AMPluginData*) data);
  return AMSmartRC::create(plugin_data->vin, plugin_data->stream);
}
#ifdef __cplusplus
}
#endif

AMSmartRC* AMSmartRC::create(AMVin *vin, AMEncodeStream *stream)
{
  AMSmartRC *result = new AMSmartRC();
  if (AM_UNLIKELY(result && !result->init(vin, stream))) {
    ERROR("Failed to create AMSmartRC object!");
    delete result;
    result = nullptr;
  } else {
    INFO("AMSmartRC::create successfully");
  }

  return result;
}

void AMSmartRC::destroy()
{
  stop();
  m_config->save_config();

  delete this;
}

bool AMSmartRC::start(AM_STREAM_ID UNUSED(id))
{
  bool ret = true;

  //Sync stream format parameters when start smartrc
  param_config_t smartrc_params_cfg =
  { 0 };
  AMStreamParamMap stream_param;
  AMVinParamMap vin_param;
  bitrate_target_t bitrate_target =
  { 0 };

  if (AM_UNLIKELY(m_stream->get_param(stream_param) != AM_RESULT_OK)) {
    ERROR("get stream parameters failed");
    return false;
  }

  const AMStreamStateMap &stream_state = m_stream->stream_state();

  if (AM_UNLIKELY(m_vin->get_param(vin_param) != AM_RESULT_OK)) {
    ERROR("get vin parameters failed");
    return false;
  }
  for (AMStreamParamMap::iterator iter = stream_param.begin();
      iter != stream_param.end(); ++ iter) {
    const AM_STREAM_ID &id = iter->first;

    if ((stream_param.at(id).stream_format.second.type.second !=
          AM_STREAM_TYPE_H264 ||
        stream_param.at(id).stream_format.second.type.second!=
          AM_STREAM_TYPE_H265) &&
        stream_state.at(id) == AM_STREAM_STATE_ENCODING &&
        stream_param.at(id).stream_format.second.enable.second) {

      smartrc_params_cfg.stream_id = iter->first;
      smartrc_params_cfg.enc_cfg.fps =
          AMVideoTrans::fps_to_q9fps(vin_param[AM_VIN_ID(0)].fps.second);

      switch (stream_param.at(id).stream_format.second.type.second) {
        case AM_STREAM_TYPE_NONE:
          smartrc_params_cfg.enc_cfg.codec_type = IAV_STREAM_TYPE_NONE;
          break;
        case AM_STREAM_TYPE_H264:
          smartrc_params_cfg.enc_cfg.codec_type = IAV_STREAM_TYPE_H264;
          break;
        case AM_STREAM_TYPE_H265:
          smartrc_params_cfg.enc_cfg.codec_type = IAV_STREAM_TYPE_H265;
          break;
        case AM_STREAM_TYPE_MJPEG:
          smartrc_params_cfg.enc_cfg.codec_type = IAV_STREAM_TYPE_MJPEG;
          break;
        default:
          break;
      }

      if (stream_param.at(id).stream_format.second.rotate_90_cw.second) {
        smartrc_params_cfg.enc_cfg.width =
                        stream_param.at(id).stream_format.second
            .enc_win.second.size.height;
        smartrc_params_cfg.enc_cfg.height =
                        stream_param.at(id).stream_format.second
            .enc_win.second.size.width;
        smartrc_params_cfg.enc_cfg.x = stream_param.at(id).stream_format.second
            .enc_win.second.offset.y;
        smartrc_params_cfg.enc_cfg.y = stream_param.at(id).stream_format.second
            .enc_win.second.offset.x;
      } else {
        smartrc_params_cfg.enc_cfg.width =
                        stream_param.at(id).stream_format.second
            .enc_win.second.size.width;
        smartrc_params_cfg.enc_cfg.height =
                        stream_param.at(id).stream_format.second
            .enc_win.second.size.height;
        smartrc_params_cfg.enc_cfg.x = stream_param.at(id).stream_format.second
            .enc_win.second.offset.x;
        smartrc_params_cfg.enc_cfg.y = stream_param.at(id).stream_format.second
            .enc_win.second.offset.y;
      }

      if (stream_param.at(id).stream_format.second.type.second ==
                      AM_STREAM_TYPE_H264) {
        smartrc_params_cfg.enc_cfg.roi_width =
        ROUND_UP(smartrc_params_cfg.enc_cfg.width, MB_SIZE) / MB_SIZE;
        smartrc_params_cfg.enc_cfg.roi_height =
        ROUND_UP(smartrc_params_cfg.enc_cfg.height, MB_SIZE) / MB_SIZE;
      } else {
        smartrc_params_cfg.enc_cfg.roi_width =
        ROUND_UP(smartrc_params_cfg.enc_cfg.width, CTB_SIZE) / CTB_SIZE;
        smartrc_params_cfg.enc_cfg.roi_height =
        ROUND_UP(smartrc_params_cfg.enc_cfg.height, CTB_SIZE) / CTB_SIZE;
      }
      smartrc_params_cfg.enc_cfg.M =
                      stream_param.at(id).h26x_config.second.M.second;
      smartrc_params_cfg.enc_cfg.N =
                      stream_param.at(id).h26x_config.second.N.second;

      if (AM_UNLIKELY(smartrc_param_config(&smartrc_params_cfg) < 0)) {
        ERROR("smartrc_param_config failed");
        ret = false;
        break;
      }

      if (AM_UNLIKELY(smartrc_set_stream_quality(QUALITY_LOW,
                                                 iter->first) < 0)) {
        ERROR("smartrc_set_stream_quality failed");
        ret = false;
        break;
      }

      bitrate_target.bitrate_ceiling = m_smartrc_param.stream_params[id]
          .bitrate_ceiling;
      if (AM_UNLIKELY(smartrc_set_bitrate_target(&bitrate_target, iter->first)
          < 0)) {
        ERROR("smartrc_set_bitrate_target failed");
        ret = false;
        break;
      }

      if (m_smartrc_param.stream_params[id].frame_drop) {
        if (AM_UNLIKELY(smartrc_set_style(STYLE_QUALITY_KEEP_FPS_AUTO_DROP,
                                          iter->first)
                        < 0)) {
          ERROR("smartrc_set_style failed");
          ret = false;
          break;
        }
      } else {
        if (AM_UNLIKELY(smartrc_set_style(STYLE_FPS_KEEP_BITRATE_AUTO,
                                          iter->first)
                        < 0)) {
          ERROR("smartrc_set_style failed");
          ret = false;
          break;
        }
      }

      if (m_smartrc_param.stream_params[id].MB_level_control) {
        if (AM_UNLIKELY(smartrc_start_roi(&m_roi_session[iter->first],
                                          iter->first) < 0)) {
          ERROR("smartrc_start_roi failed");
          ret = false;
          break;
        }
      }
    } else {
      NOTICE("stream %d is not H264/H265, nor it is not encoding. Ignore",
             iter->first);
    }
  }

  if (!ret) {
    return false;
  }

  if (AM_UNLIKELY(smartrc_set_log_level(m_smartrc_param.log_level) < 0)) {
    ERROR("smartrc_set_log_level failed!");
    return false;
  }
  if (AM_UNLIKELY(m_smartrc_thread)) {
    NOTICE("SmartRC is already running!");
  } else {
    m_smartrc_thread = AMThread::create("Video.SmartRC",
                                        static_smartrc_main,
                                        this);
    if (AM_UNLIKELY(!m_smartrc_thread)) {
      ERROR("Failed to start SmartRC thread!");
      return false;
    }
  }

  return true;
}

bool AMSmartRC::stop(AM_STREAM_ID id)
{
  bool need_stop = true;

  for (AMSmartRCStreamMap::iterator iter =
                  m_smartrc_param.stream_params.begin();
                  iter != m_smartrc_param.stream_params.end(); ++ iter) {
    if (m_roi_session[iter->first].roi_started) {
      if (AM_UNLIKELY(smartrc_stop_roi(&m_roi_session[iter->first],
                                       iter->first) < 0)) {
        ERROR("smartrc_stop_roi failed!");
      }
    }
  }

  if (AM_LIKELY(AM_STREAM_ID_MAX != id)) {
    /* When only one stream is disabled,
     * we need to check if this stream is the only one left to be disabled,
     * if it is, we need to stop this plug-in
     */
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    for (auto &m : stream_state) {
      if (AM_LIKELY(m.first != id)) {
        if (AM_LIKELY(m.second != AM_STREAM_STATE_IDLE)) {
          need_stop = false;
          break;
        }
      } else {
        if (AM_LIKELY((m.second != AM_STREAM_STATE_ENCODING)
            && (m.second != AM_STREAM_STATE_IDLE))) {
          need_stop = false;
          break;
        }
      }
    }
  }

  if (AM_LIKELY(need_stop)) {
    m_run = false;
    AM_DESTROY(m_smartrc_thread);
    INFO("Destroy <m_smartrc_thread> successfully");
  } else {
    INFO("No need to stop!");
  }

  return true;
}

bool AMSmartRC::set_enable(uint32_t stream_id, bool enable)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::get_enable(uint32_t stream_id)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::set_bitrate_ceiling(uint32_t stream_id, uint32_t ceiling)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::get_bitrate_ceiling(uint32_t stream_id, uint32_t& ceiling)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::set_drop_frame_enable(uint32_t stream_id, bool enable)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::get_drop_frame_enable(uint32_t stream_id)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::set_dynanic_gop_n(uint32_t stream_id, bool enable)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::get_dynanic_gop_n(uint32_t stream_id)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::set_mb_level_control(uint32_t stream_id, bool enable)
{
  bool ret = true;
  return ret;
}
bool AMSmartRC::get_mb_level_control(uint32_t stream_id)
{
  bool ret = true;
  return ret;
}

std::string& AMSmartRC::name()
{
  return m_name;
}

void* AMSmartRC::get_interface()
{
  return ((AMISmartRC*) this);
}

AMSmartRC::AMSmartRC() :
    m_vin(nullptr),
    m_stream(nullptr),
    m_config(nullptr),
    m_smartrc_thread(nullptr),
    m_mutex(nullptr),
    m_iav_fd(-1),
    m_motion_value(0),
    m_name("smartrc"),
    m_run(false)
{
  m_roi_session = new roi_session_t[AM_STREAM_ID_MAX];
  for (uint32_t i = 0; i < AM_STREAM_ID_MAX; i ++) {
    m_roi_session[i].count = 0;
    memset(m_roi_session[i].daddr, 0, sizeof(m_roi_session[i].daddr));
    m_roi_session[i].dsp_pts = 0;
    m_roi_session[i].motion_matrix = nullptr;
    m_roi_session[i].prev1 = nullptr;
    m_roi_session[i].prev2 = nullptr;
    m_roi_session[i].roi_started = 0;
    m_roi_session[i].tmpdata = nullptr;
  }
}

AMSmartRC::~AMSmartRC()
{
  stop();
  smartrc_deinit();
  AM_RELEASE(m_config);
  delete m_mutex;
  delete[] m_roi_session;
  if (AM_LIKELY(m_iav_fd) >= 0) {
    close(m_iav_fd);
  }
}

bool AMSmartRC::init(AMVin *vin, AMEncodeStream *stream)
{
  bool ret = true;
  do {
    m_vin = vin;
    if (AM_UNLIKELY(!m_vin)) {
      ERROR("Invalid VIN device!");
      break;
    }

    m_stream = stream;
    if (AM_UNLIKELY(!m_stream)) {
      ERROR("Invalid encode stream!");
      break;
    }

    m_mutex = new std::recursive_mutex();
    if (AM_UNLIKELY(!m_mutex)) {
      ERROR("Failed to allocate memory for mutex!");
      ret = false;
      break;
    }

    m_config = AMSmartRCConfig::get_instance();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to get AMSmartRCConfig instance");
      ret = false;
      break;
    } else {
      if (AM_UNLIKELY(m_config->load_config() != AM_RESULT_OK)) {
        ERROR("Failed to load smartrc config");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(m_config->get_config(m_smartrc_param) != AM_RESULT_OK)) {
        ERROR("Failed to get smartrc config");
        ret = false;
        break;
      }
    }

    if (AM_UNLIKELY((m_iav_fd = open("/dev/iav", O_RDWR)) < 0)) {
      ERROR("Failed to open /dev/iav");
      ret = false;
      break;
    }

    init_t init =
    { -1 };
    init.fd_iav = m_iav_fd;
    if (AM_UNLIKELY(smartrc_init(&init) < 0)) {
      ERROR("Failed to init smartrc");
      ret = false;
      break;
    } else {
      INFO("Init SmartRC done");
    }

  } while (0);
  return ret;
}

void AMSmartRC::static_smartrc_main(void *data)
{
  ((AMSmartRC *) data)->smartrc_main();
}

void AMSmartRC::smartrc_main()
{
  uint32_t i_light[3] =
  { 0 };
  noise_level_t noise_level = NOISE_LOW;
  m_run = true;
  AMVinAGC agc;
  uint32_t noise_value = 0;

  while (m_run) {
    if (AM_UNLIKELY(!m_vin)) {
      ERROR("m_vin NULL");
    } else {
      if (AM_UNLIKELY(AM_RESULT_OK != m_vin->wait_frame_sync())) {
        ERROR("Failed to wait frame sync!");
      }

      if (AM_LIKELY(AM_RESULT_OK == m_vin->get_agc(agc))) {
        noise_value = agc.agc >> 24;
      } else {
        NOTICE("Failed to get VIN AGC value!");
        continue;
      }
    }

    if (noise_value > m_smartrc_param.noise_high_threshold) {
      ++ i_light[0];
      i_light[1] = 0;
      i_light[2] = 0;
      if (i_light[0] >= NOISE_HIGH_DELAY) {
        noise_level = NOISE_HIGH;
        i_light[0] = 0;
      }
    } else if (noise_value > m_smartrc_param.noise_low_threshold) {
      i_light[0] = 0;
      ++ i_light[1];
      i_light[2] = 0;
      if (i_light[1] >= NOISE_LOW_DELAY) {
        noise_level = NOISE_LOW;
        i_light[0] = 0;
      }
    } else {
      i_light[0] = 0;
      i_light[1] = 0;
      ++ i_light[2];
      if (i_light[2] >= NOISE_NONE_DELAY) {
        noise_level = NOISE_NONE;
        i_light[2] = 0;
      }
    }

    for (AMSmartRCStreamMap::iterator iter =
        m_smartrc_param.stream_params.begin();
        iter != m_smartrc_param.stream_params.end(); ++ iter) {
      const AM_STREAM_ID &id = iter->first;
      const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
      const AMStreamStateMap &stream_state = m_stream->stream_state();
      if (AM_LIKELY(iter->second.enable
          && stream_fmt.enable.second
          && (stream_state.at(id) == AM_STREAM_STATE_ENCODING)
          && ((stream_fmt.type.second == AM_STREAM_TYPE_H264) ||
                          (stream_fmt.type.second == AM_STREAM_TYPE_H265)))) {
        noise_level_t real_noise_level;
        if (AM_UNLIKELY(smartrc_get_noise_level(&real_noise_level, iter->first)
            < 0)) {
          ERROR("smartrc_get_noise_level failed");
          break;
        }
        if (AM_UNLIKELY(noise_level != real_noise_level)) {
          if (AM_UNLIKELY(smartrc_set_noise_level(noise_level, iter->first)
              < 0)) {
            ERROR("smartrc_set_noise_level failed");
            break;
          }
        }
      }
    }
  }
}

void AMSmartRC::process_motion_info(void *msg_data)
{
  AMMDMessage *msg = ((AMMDMessage*) msg_data);

  if (msg && (msg->roi_id == 0) && m_run && m_smartrc_thread) {
    motion_level_t motion_level = MOTION_HIGH;
    switch (msg->mt_level) {
      case 0:
        motion_level = MOTION_NONE;
        break;
      case 1:
        motion_level = MOTION_LOW;
        break;
      case 2:
      default:
        motion_level = MOTION_HIGH;
        break;
    }

    for (AMSmartRCStreamMap::iterator iter =
        m_smartrc_param.stream_params.begin();
        iter != m_smartrc_param.stream_params.end(); ++ iter) {
      const AM_STREAM_ID &id = iter->first;
      const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
      const AMStreamStateMap &stream_state = m_stream->stream_state();
      //Get motion matrix.
      m_roi_session[iter->first].motion_matrix = (uint8_t *)msg->motion_matrix;
      m_roi_session[iter->first].dsp_pts = msg->pts;
      if (AM_LIKELY(iter->second.enable
          && stream_fmt.enable.second
          && (stream_state.at(id) == AM_STREAM_STATE_ENCODING)
          && ((stream_fmt.type.second == AM_STREAM_TYPE_H264) ||
                          (stream_fmt.type.second
                                          == AM_STREAM_TYPE_H265)))) {
        motion_level_t real_motion_level;
        if (AM_UNLIKELY(smartrc_get_motion_level(&real_motion_level,
                                                 iter->first)
                        < 0)) {
          ERROR("smartrc_get_motion_level failed");
          break;
        }
        if (motion_level != real_motion_level) {
          if (AM_UNLIKELY(smartrc_set_motion_level(motion_level, iter->first)
              < 0)) {
            ERROR("smartrc_set_motion_level");
            break;
          }
        }
        if (AM_LIKELY(m_smartrc_param.stream_params[id].MB_level_control &&
                      m_roi_session[iter->first].roi_started)) {
          if (AM_UNLIKELY(smartrc_update_roi(&m_roi_session[iter->first],
                                             iter->first) < 0)) {
            ERROR("smartrc_update_roi failed");
            break;
          }
        }
      }
    }
  }
}
