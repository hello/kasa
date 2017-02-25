/*******************************************************************************
 * am_lbr_control.cpp
 *
 * History:
 *   Nov 11, 2015 - [ypchang] created file
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
#include "am_lbr_control.h"
#include "am_motion_detect_types.h"

#include "am_thread.h"
#include "am_vin.h"

#define LBR_NOISE_NONE_DELAY 120
#define LBR_NOISE_LOW_DELAY  2
#define LBR_NOISE_HIGH_DELAY 0

#define LOCK_GUARD(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  AMPluginData *plugin_data = ((AMPluginData*)data);
  return AMLbrControl::create(plugin_data->vin, plugin_data->stream);
}
#ifdef __cplusplus
}
#endif

AMLbrControl* AMLbrControl::create(AMVin *vin, AMEncodeStream *stream)
{
  AMLbrControl *result = new AMLbrControl();
  if (AM_UNLIKELY(result && !result->init(vin, stream))) {
    ERROR("Failed to create AMLbrControl object!");
    delete result;
    result = nullptr;
  }

  return result;
}

void AMLbrControl::destroy()
{
  std::string conf(LBR_CONF_DIR);
  conf.append("/lbr.acs");
  stop();
  m_config->save_config(conf);

  delete this;
}

bool AMLbrControl::start(AM_STREAM_ID UNUSED(id))
{
  bool ret = true;

  //Sync stream format parameters when start LBR
  do {
    if (AM_UNLIKELY(m_lbr_thread)) {
      NOTICE("LBR is already running!");
      break;
    }
    m_lbr_thread = AMThread::create("LBR.ctrl", static_lbr_main, this);
    if (AM_UNLIKELY(!m_lbr_thread)) {
      ret = false;
      ERROR("Failed to start LBR Controlling thread!");
      break;
    }
  }while(0);

  return ret;
}

bool AMLbrControl::stop(AM_STREAM_ID id)
{
  bool need_stop = true;
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
        if (AM_LIKELY((m.second != AM_STREAM_STATE_ENCODING) &&
                      (m.second != AM_STREAM_STATE_IDLE))) {
          need_stop = false;
          break;
        }
      }
    }
  }

  if (AM_LIKELY(need_stop)) {
    m_run = false;
    AM_DESTROY(m_lbr_thread);
  } else {
    INFO("No need to stop!");
  }

  return true;
}

std::string& AMLbrControl::name()
{
  return m_name;
}

void* AMLbrControl::get_interface()
{
  return ((AMILBRControl*)this);
}

bool AMLbrControl::set_enable(uint32_t stream_id, bool enable)
{
  bool ret = true;
  LOCK_GUARD(*m_mutex);
  do {
    AM_STREAM_ID id = AM_STREAM_ID(stream_id);
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
    if (AM_UNLIKELY(!stream_fmt.enable.second)) {
      ERROR("Stream%u is not enabled!", stream_id);
      ret = false;
      break;
    }
    if (AM_UNLIKELY(stream_state.at(id) != AM_STREAM_STATE_ENCODING)) {
      ERROR("Stream%u is not encoding!", stream_id);
      ret = false;
      break;
    }
    if (AM_UNLIKELY(stream_fmt.type.second != AM_STREAM_TYPE_H264) &&
        AM_UNLIKELY(stream_fmt.type.second != AM_STREAM_TYPE_H265)) {
      ERROR("Stream%u is not H.264/H.265!", stream_id);
      ret = false;
      break;
    }
    m_lbr_config->stream_params[id].enable_lbr = enable;
    ret = apply_lbr_style(id);
  }while(0);

  return ret;
}

bool AMLbrControl::get_enable(uint32_t stream_id)
{
  LOCK_GUARD(*m_mutex);
  return m_lbr_config->stream_params[AM_STREAM_ID(stream_id)].enable_lbr;
}

bool AMLbrControl::set_bitrate_ceiling(uint32_t stream_id,
                                       uint32_t ceiling,
                                       bool is_auto)
{
  bool ret = true;
  LOCK_GUARD(*m_mutex);
  do {
    AM_STREAM_ID id = AM_STREAM_ID(stream_id);
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
    const AMResolution &stream_res = stream_fmt.enc_win.second.size;
    lbr_bitrate_target_t target = {0};

    if (AM_UNLIKELY(!stream_fmt.enable.second)) {
      ERROR("Stream%u is not enabled!", stream_id);
      break;
    }

    if (AM_UNLIKELY(stream_state.at(id) != AM_STREAM_STATE_ENCODING)) {
      ERROR("Stream%u is not encoding!", stream_id);
    }

    if (AM_UNLIKELY((stream_fmt.type.second != AM_STREAM_TYPE_H264) &&
                    (stream_fmt.type.second != AM_STREAM_TYPE_H265))) {
      ERROR("Stream%u is not H.264/H.265!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY(!is_auto && (ceiling <= 0))) {
      ERROR("Invalid bitrate ceiling when auto mode is disabled: %u", ceiling);
      ret = false;
      break;
    }

    m_lbr_config->stream_params[id].auto_target = is_auto;
    m_lbr_config->stream_params[id].bitrate_ceiling = ceiling;
    target.bitrate_ceiling = ceiling *
        ((stream_res.width * stream_res.height) >> 8);
    target.auto_target = is_auto ? 1 : 0;

    if (AM_UNLIKELY(lbr_set_bitrate_ceiling(&target, stream_id) < 0)) {
      ERROR("Failed to set bitrate ceiling of stream%u", stream_id);
      ret = false;
      break;
    }
  }while(0);

  return ret;
}

bool AMLbrControl::get_bitrate_ceiling(uint32_t stream_id,
                                       uint32_t& ceiling,
                                       bool& is_auto)
{
  bool ret = true;
  LOCK_GUARD(*m_mutex);
  do {
    AM_STREAM_ID id = AM_STREAM_ID(stream_id);
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
    const AMResolution &stream_res = stream_fmt.enc_win.second.size;
    lbr_bitrate_target_t target = {0};

    ceiling = 0;
    if (AM_UNLIKELY(!stream_fmt.enable.second)) {
      ERROR("Stream%u is not enabled!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY(stream_state.at(id) != AM_STREAM_STATE_ENCODING)) {
      ERROR("Stream%u is not encoding!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY((stream_fmt.type.second != AM_STREAM_TYPE_H264) &&
                    (stream_fmt.type.second != AM_STREAM_TYPE_H265))) {
      ERROR("Stream%u is not H.264/H.265!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY(lbr_get_bitrate_ceiling(&target, stream_id) < 0)) {
      ERROR("Failed to get bitrate ceiling of stream%u", stream_id);
      ret = false;
      break;
    }

    ceiling = target.bitrate_ceiling /
        ((stream_res.width * stream_res.height) >> 8);
    is_auto = target.auto_target;
  }while(0);

  return ret;
}

bool AMLbrControl::set_drop_frame_enable(uint32_t stream_id, bool enable)
{
  bool ret = true;
  LOCK_GUARD(*m_mutex);
  do {
    AM_STREAM_ID id = AM_STREAM_ID(stream_id);
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);

    if (AM_UNLIKELY(!stream_fmt.enable.second)) {
      ERROR("Stream%u is not enabled!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY(stream_state.at(id) != AM_STREAM_STATE_ENCODING)) {
      ERROR("Stream%u is not encoding!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY((stream_fmt.type.second != AM_STREAM_TYPE_H264) &&
                    (stream_fmt.type.second != AM_STREAM_TYPE_H265))) {
      ERROR("Stream%u is not H.264/H.265!", stream_id);
      ret = false;
      break;
    }

    if (AM_UNLIKELY(!m_lbr_config->stream_params[id].enable_lbr)) {
      WARN("LBR is not enabled for stream%u", stream_id);
      ret = false;
      break;
    }

    m_lbr_config->stream_params[id].frame_drop = enable;
    m_lbr_style[id].second = m_lbr_config->stream_params[id].frame_drop ?
        LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP :
        LBR_STYLE_FPS_KEEP_BITRATE_AUTO;

    if (AM_UNLIKELY(lbr_set_style(m_lbr_style[id].second, stream_id) < 0)) {
      ERROR("Failed to set stream%u LBR style of to %d",
            stream_id, m_lbr_style[id]);
      ret = false;
      break;
    }
  }while(0);

  return ret;
}

bool AMLbrControl::get_drop_frame_enable(uint32_t stream_id)
{
  LOCK_GUARD(*m_mutex);
  return m_lbr_config->stream_params[AM_STREAM_ID(stream_id)].frame_drop;
}

void AMLbrControl::process_motion_info(void *msg_data)
{
  AMMDMessage *msg = ((AMMDMessage*)msg_data);

  if (msg && (msg->roi_id == 0) && m_run && m_lbr_thread) {
    LBR_MOTION_LEVEL motion_level = LBR_MOTION_HIGH;
    switch(msg->mt_level) {
      case 0: motion_level = LBR_MOTION_NONE; break;
      case 1: motion_level = LBR_MOTION_LOW;  break;
      case 2:
      default: motion_level = LBR_MOTION_HIGH; break;
    }

    for (AMLbrStreamMap::iterator iter = m_lbr_config->stream_params.begin();
          iter != m_lbr_config->stream_params.end();
          ++ iter) {
      const AM_STREAM_ID &id = iter->first;
      const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
      const AMStreamStateMap &stream_state = m_stream->stream_state();
      if (AM_LIKELY(iter->second.enable_lbr &&
                    iter->second.motion_control &&
                    stream_fmt.enable.second &&
                    (stream_state.at(id) == AM_STREAM_STATE_ENCODING) &&
                    ((stream_fmt.type.second == AM_STREAM_TYPE_H264) ||
                    (stream_fmt.type.second == AM_STREAM_TYPE_H265)))) {
        LBR_MOTION_LEVEL real_motion_level;
        lbr_get_motion_level(&real_motion_level, iter->first);
        if (AM_LIKELY(motion_level != real_motion_level)) {
          lbr_set_motion_level(motion_level, iter->first);
        }
      }
    }
  }
}

bool AMLbrControl::save_current_config()
{
  bool ret = true;
  std::string conf(LBR_CONF_DIR);
  conf.append("/lbr.acs");
  do {
    if (AM_UNLIKELY(!m_config->sync_config(m_lbr_config))) {
      ERROR("sync config failed");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_config->save_config(conf))) {
      ERROR("save config to file failed");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

inline bool AMLbrControl::apply_lbr_style(AM_STREAM_ID id)
{
  bool ret = true;
  LBR_STYLE style = !m_lbr_config->stream_params[id].enable_lbr ?
      LBR_STYLE_FPS_KEEP_CBR_ALIKE :
      (m_lbr_config->stream_params[id].frame_drop ?
          LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP :
          LBR_STYLE_FPS_KEEP_BITRATE_AUTO);

  if (AM_LIKELY((m_lbr_style.find(id) == m_lbr_style.end()) ||
                (m_lbr_style[id].second != style))) {
    m_lbr_style[id] = std::pair<bool, LBR_STYLE>(true, style);
  } else {
    m_lbr_style[id].first = false;
  }

  if (AM_LIKELY(m_lbr_style[id].first)) {
    if (AM_UNLIKELY(lbr_set_style(m_lbr_style[id].second, id) < 0)) {
      ERROR("Failed to set stream%d LBR style to %d!", id, style);
      ret = false;
    } else {
      m_lbr_style[id].first = false;
    }
  }

  return ret;
}

void AMLbrControl::static_lbr_main(void *data)
{
  ((AMLbrControl*)data)->lbr_main();
}

void AMLbrControl::lbr_main()
{
  uint32_t i_light[3] = {0};
  LBR_NOISE_LEVEL noise_level = LBR_NOISE_LOW;

  if (AM_UNLIKELY(lbr_set_log_level(m_lbr_config->log_level) < 0)) {
    ERROR("lbr_set_log_level failed!");
  }

  for (int32_t i = 0; i < AM_LBR_PROFILE_NUM; ++ i) {
    AMScaleFactor &sf = m_lbr_config->profile_bt_sf[i];
    if (AM_LIKELY((sf.num > 0) && (sf.den > 0))) {
      lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE(i), sf.num, sf.den);
    } else {
      ERROR("Invalid frame factor: numerator and denominator cannot be 0!");
    }
  }

  for (AMLbrStreamMap::iterator iter = m_lbr_config->stream_params.begin();
      iter != m_lbr_config->stream_params.end();
      ++ iter) {
    lbr_bitrate_target_t target = {0};
    const AM_STREAM_ID &id = iter->first;
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
    const AMResolution &stream_res = stream_fmt.enc_win.second.size;
    AMLbrStream &lbr_stream = m_lbr_config->stream_params[iter->first];

    if (AM_UNLIKELY(((stream_fmt.type.second == AM_STREAM_TYPE_H264) ||
                     (stream_fmt.type.second == AM_STREAM_TYPE_H265)) &&
                    stream_fmt.enable.second &&
                    (stream_state.at(id) == AM_STREAM_STATE_ENCODING))) {
      apply_lbr_style(id);
      target.auto_target = lbr_stream.auto_target ? 1 : 0;
      target.bitrate_ceiling = ((stream_res.width * stream_res.height) >> 8) *
          lbr_stream.bitrate_ceiling;
      if (AM_UNLIKELY(lbr_set_bitrate_ceiling(&target, id) < 0)) {
        ERROR("Failed to et stream%d LBR bitrate ceiling to %u",
              iter->first, lbr_stream.bitrate_ceiling);
      }
    }
  }

  m_run = true;
  while(m_run) {
    AMVinAGC agc;
    uint32_t noise_value = 0;
    if (AM_UNLIKELY(AM_RESULT_OK != m_vin->wait_frame_sync())) {
      ERROR("Failed to wait frame sync!");
    }

    if (AM_LIKELY(AM_RESULT_OK == m_vin->get_agc(agc))) {
      noise_value = agc.agc >> 24;
    } else {
      NOTICE("Failed to get VIN AGC value!");
      continue;
    }

    if (noise_value > m_lbr_config->noise_high_threshold) {
      ++ i_light[0];
      i_light[1] = 0;
      i_light[2] = 0;
      if (i_light[0] >= LBR_NOISE_HIGH_DELAY) {
        noise_level = LBR_NOISE_HIGH;
        i_light[0] = 0;
      }
    } else if (noise_value > m_lbr_config->noise_low_threshold) {
      i_light[0] = 0;
      ++ i_light[1];
      i_light[2] = 0;
      if (i_light[1] >= LBR_NOISE_LOW_DELAY) {
        noise_level = LBR_NOISE_LOW;
        i_light[0] = 0;
      }
    } else {
      i_light[0] = 0;
      i_light[1] = 0;
      ++ i_light[2];
      if (i_light[2] >= LBR_NOISE_NONE_DELAY) {
        noise_level = LBR_NOISE_NONE;
        i_light[2] = 0;
      }
    }
    for (AMLbrStreamMap::iterator iter = m_lbr_config->stream_params.begin();
          iter != m_lbr_config->stream_params.end();
          ++ iter) {
      const AM_STREAM_ID &id = iter->first;
      const AMStreamFormatConfig &stream_fmt = m_stream->stream_config(id);
      const AMStreamStateMap &stream_state = m_stream->stream_state();
      if (AM_LIKELY(iter->second.enable_lbr &&
                    iter->second.noise_control &&
                    stream_fmt.enable.second &&
                    (stream_state.at(id) == AM_STREAM_STATE_ENCODING) &&
                    ((stream_fmt.type.second == AM_STREAM_TYPE_H264) ||
                     (stream_fmt.type.second == AM_STREAM_TYPE_H265)))) {
        LBR_NOISE_LEVEL real_noise_level;
        lbr_get_noise_level(&real_noise_level, iter->first);
        if (AM_LIKELY(noise_level != real_noise_level)) {
          lbr_set_noise_level(noise_level, iter->first);
        }
      }
    }
  }
}

AMLbrControl::AMLbrControl() :
    m_lbr_config(nullptr),
    m_vin(nullptr),
    m_stream(nullptr),
    m_config(nullptr),
    m_lbr_thread(nullptr),
    m_mutex(nullptr),
    m_iav_fd(-1),
    m_motion_value(0),
    m_name("LBR"),
    m_run(false)
{
  m_lbr_style.clear();
}

AMLbrControl::~AMLbrControl()
{
  stop();
  delete m_config;
  delete m_mutex;
  if (AM_LIKELY(m_iav_fd >= 0)) {
    close(m_iav_fd);
  }
  lbr_close();
}

bool AMLbrControl::init(AMVin *vin, AMEncodeStream *stream)
{
  bool ret = false;

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
      break;
    }

    m_config = new AMLbrControlConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create AMLbrControlConfig object!");
      break;
    } else {
      std::string conf(LBR_CONF_DIR);
      conf.append("/lbr.acs");

      m_lbr_config = m_config->get_config(conf);
      if (AM_UNLIKELY(!m_lbr_config)) {
        ERROR("Failed to get LBR config!");
        break;
      }
    }

    if (AM_UNLIKELY((m_iav_fd = open("/dev/iav", O_RDWR)) < 0)) {
      ERROR("Failed to open /dev/iav");
      break;
    }
    lbr_init_t lbr = {-1};
    lbr.fd_iav = m_iav_fd;

    if (AM_UNLIKELY(lbr_open() < 0)) {
      ERROR("lbr_open() failed!");
      break;
    }

    if (AM_UNLIKELY(lbr_init(&lbr) < 0)) {
      ERROR("lbr_init() failed!");
      break;
    }
    ret = true;
  }while(0);

  return ret;
}
