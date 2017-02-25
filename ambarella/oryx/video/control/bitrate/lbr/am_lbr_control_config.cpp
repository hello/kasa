/*******************************************************************************
 * am_lbr_control_config.cpp
 *
 * History:
 *   Nov 10, 2015 - [ypchang] created file
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

#include "am_lbr_control_config.h"
#include "am_configure.h"
#include <mutex>

#define LBR_CONF_AUTO_LOCK(mtx) std::lock_guard<std::mutex> lck(mtx)
static std::mutex g_conf_mutex;

AMLbrControlConfig::AMLbrControlConfig() :
  m_lbr_config(nullptr),
  m_config(nullptr)
{}

AMLbrControlConfig::~AMLbrControlConfig()
{
  delete m_lbr_config;
  delete m_config;
}

AMLbrConfig* AMLbrControlConfig::get_config(const std::string& conf)
{
  AMLbrConfig *config = nullptr;

  do {
    LBR_CONF_AUTO_LOCK(g_conf_mutex);
    if (AM_LIKELY(!m_lbr_config)) {
      m_lbr_config = new AMLbrConfig();
    }
    if (AM_UNLIKELY(!m_lbr_config)) {
      ERROR("Failed to create AMLbrConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig& lbr_config = *m_config;
      AMLbrConfig &lbr_param = *m_lbr_config;

      if (lbr_config["LogLevel"].exists()) {
        lbr_param.log_level = lbr_config["LogLevel"].get<uint32_t>(1);
      }
      if (lbr_config["NoiseStatistics"]["NoiseLowThreshold"].exists()) {
        lbr_param.noise_low_threshold = lbr_config
            ["NoiseStatistics"]["NoiseLowThreshold"].get<uint32_t>(1);
      }
      if (lbr_config["NoiseStatistics"]["NoiseHighThreshold"].exists()) {
        lbr_param.noise_high_threshold = lbr_config
            ["NoiseStatistics"]["NoiseHighThreshold"].get<uint32_t>(6);
      }
      if (lbr_config["ProfileBTScaleFactor"]["Static"].exists()) {
        lbr_param.profile_bt_sf[0].num = lbr_config
            ["ProfileBTScaleFactor"]["Static"][0].get<uint32_t>(1);
        lbr_param.profile_bt_sf[0].den = lbr_config
            ["ProfileBTScaleFactor"]["Static"][1].get<uint32_t>(1);
      }
      if (lbr_config["ProfileBTScaleFactor"]["SmallMotion"].exists()) {
        lbr_param.profile_bt_sf[1].num = lbr_config
            ["ProfileBTScaleFactor"]["SmallMotion"][0].get<uint32_t>(1);
        lbr_param.profile_bt_sf[1].den = lbr_config
            ["ProfileBTScaleFactor"]["SmallMotion"][1].get<uint32_t>(1);
      }
      if (lbr_config["ProfileBTScaleFactor"]["BigMotion"].exists()) {
        lbr_param.profile_bt_sf[2].num = lbr_config
            ["ProfileBTScaleFactor"]["BigMotion"][0].get<uint32_t>(1);
        lbr_param.profile_bt_sf[2].den = lbr_config
            ["ProfileBTScaleFactor"]["BigMotion"][1].get<uint32_t>(1);
      }
      if (lbr_config["ProfileBTScaleFactor"]["LowLight"].exists()) {
        lbr_param.profile_bt_sf[3].num = lbr_config
            ["ProfileBTScaleFactor"]["LowLight"][0].get<uint32_t>(1);
        lbr_param.profile_bt_sf[3].den = lbr_config
            ["ProfileBTScaleFactor"]["LowLight"][1].get<uint32_t>(1);
      }
      if (lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"].exists()) {
        lbr_param.profile_bt_sf[4].num = lbr_config
            ["ProfileBTScaleFactor"]["BigMotionWithFD"][0].get<uint32_t>(1);
        lbr_param.profile_bt_sf[4].den = lbr_config
            ["ProfileBTScaleFactor"]["BigMotionWithFD"][1].get<uint32_t>(1);
      }
      if (lbr_config["StreamConfig"].exists()) {
        for (uint32_t i = 0; i < lbr_config["StreamConfig"].length(); ++ i) {
          AM_STREAM_ID id = AM_STREAM_ID(i);
          if (lbr_config["StreamConfig"][i]["EnableLBR"].exists()) {
            lbr_param.stream_params[id].enable_lbr = lbr_config
                ["StreamConfig"][i]["EnableLBR"].get<bool>(true);
          }
          if (lbr_config["StreamConfig"][i]["MotionControl"].exists()) {
            lbr_param.stream_params[id].motion_control = lbr_config
                ["StreamConfig"][i]["MotionControl"].get<bool>(true);
          }
          if (lbr_config["StreamConfig"][i]["NoiseControl"].exists()) {
            lbr_param.stream_params[id].noise_control = lbr_config
                ["StreamConfig"][i]["NoiseControl"].get<bool>(true);
          }
          if (lbr_config["StreamConfig"][i]["FrameDrop"].exists()) {
            lbr_param.stream_params[id].frame_drop = lbr_config
                ["StreamConfig"][i]["FrameDrop"].get<bool>(true);
          }
          if (lbr_config["StreamConfig"][i]["AutoBitrateTarget"].exists()) {
            lbr_param.stream_params[id].auto_target = lbr_config
                ["StreamConfig"][i]["AutoBitrateTarget"].get<bool>(false);
          }
          if (lbr_config["StreamConfig"][i]["BitrateCeiling"].exists()) {
            lbr_param.stream_params[id].bitrate_ceiling = lbr_config
                ["StreamConfig"][i]["BitrateCeiling"].get<uint32_t>(142);
          }
        }
      }
      config = m_lbr_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }

  }while(0);

  return config;
}

bool AMLbrControlConfig::save_config(const std::string& conf)
{
  bool ret = true;
  do {
    LBR_CONF_AUTO_LOCK(g_conf_mutex);
    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &lbr_config = *m_config;
      lbr_config["LogLevel"] = m_lbr_config->log_level;
      lbr_config["NoiseStatistics"]["NoiseLowThreshold"] =
          m_lbr_config->noise_low_threshold;
      lbr_config["NoiseStatistics"]["NoiseHighThreshold"] =
          m_lbr_config->noise_high_threshold;
      lbr_config["ProfileBTScaleFactor"]["Static"][0] =
          m_lbr_config->profile_bt_sf[0].num;
      lbr_config["ProfileBTScaleFactor"]["Static"][1] =
          m_lbr_config->profile_bt_sf[0].den;
      lbr_config["ProfileBTScaleFactor"]["SmallMotion"][0] =
          m_lbr_config->profile_bt_sf[1].num;
      lbr_config["ProfileBTScaleFactor"]["SmallMotion"][1] =
          m_lbr_config->profile_bt_sf[1].den;
      lbr_config["ProfileBTScaleFactor"]["BigMotion"][0] =
          m_lbr_config->profile_bt_sf[2].num;
      lbr_config["ProfileBTScaleFactor"]["BigMotion"][1] =
          m_lbr_config->profile_bt_sf[2].den;
      lbr_config["ProfileBTScaleFactor"]["LowLight"][0] =
          m_lbr_config->profile_bt_sf[3].num;
      lbr_config["ProfileBTScaleFactor"]["LowLight"][1] =
          m_lbr_config->profile_bt_sf[3].den;
      lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"][0] =
          m_lbr_config->profile_bt_sf[4].num;
      lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"][1] =
          m_lbr_config->profile_bt_sf[4].den;
      for (AMLbrStreamMap::iterator iter = m_lbr_config->stream_params.begin();
           iter != m_lbr_config->stream_params.end();
           ++ iter) {
        lbr_config["StreamConfig"][iter->first]["EnableLBR"] =
            iter->second.enable_lbr;
        lbr_config["StreamConfig"][iter->first]["MotionControl"] =
            iter->second.motion_control;
        lbr_config["StreamConfig"][iter->first]["NoiseControl"] =
            iter->second.noise_control;
        lbr_config["StreamConfig"][iter->first]["FrameDrop"] =
            iter->second.frame_drop;
        lbr_config["StreamConfig"][iter->first]["AutoBitrateTarget"] =
            iter->second.auto_target;
        lbr_config["StreamConfig"][iter->first]["BitrateCeiling"] =
            iter->second.bitrate_ceiling;
      }
      if (AM_UNLIKELY(!lbr_config.save())) {
        ERROR("Failed to save config file: %s!", conf.c_str());
        ret = false;
        break;
      } else {
        INFO("Config file %s is saved!", conf.c_str());
      }
    } else {
      ERROR("Failed to open config file: %s", conf.c_str());
      ret = false;
      break;
    }
  }while(0);

  return ret;
}

bool AMLbrControlConfig::sync_config(AMLbrConfig *cfg)
{
  if (AM_LIKELY(cfg != nullptr)) {
    memcpy(m_lbr_config, cfg, sizeof(AMLbrConfig));
  } else {
    ERROR("NULL pointer, sync config failed");
    return false;
  }

  return true;
}
