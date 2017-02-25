/*******************************************************************************
 * am_smartrc_config.cpp
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
#include "am_log.h"
#include "am_define.h"
#include "am_configure.h"
#include "am_video_types.h"
#include "am_smartrc_config.h"

AMSmartRCConfig *AMSmartRCConfig::m_instance = nullptr;
std::recursive_mutex AMSmartRCConfig::m_mtx;
#define AUTO_LOCK_SMARTRC(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMSmartRCConfig *AMSmartRCConfig::get_instance()
{
  AUTO_LOCK_SMARTRC(m_mtx);
  if (!m_instance) {
    m_instance = new AMSmartRCConfig();
  }

  return m_instance;
}

AM_RESULT AMSmartRCConfig::get_config(AMSmartRCParam &config)
{
  AUTO_LOCK_SMARTRC(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMSmartRCConfig::set_config(const AMSmartRCParam &config)
{
  AUTO_LOCK_SMARTRC(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_config = config;
  } while (0);

  return result;
}

AM_RESULT AMSmartRCConfig::load_config()
{
  AUTO_LOCK_SMARTRC(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  uint32_t i;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = SMARTRC_CONF_DIR;
  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + SMARTRC_CONFIG_FILE;
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMSmartRCConfig::Create AMConfig failed!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &smartrc_config = *config_ptr;

    if (smartrc_config["LogLevel"].exists()) {
      m_config.log_level = smartrc_config["LogLevel"].get<uint32_t>(0);
    } else {
      m_config.log_level = 0;
    }
    if (smartrc_config["NoiseLowThreshold"].exists()) {
      m_config.noise_low_threshold =
          smartrc_config["NoiseLowThreshold"].get<uint32_t>(6);
    } else {
      m_config.noise_low_threshold = 6;
    }
    if (smartrc_config["NoiseHighThreshold"].exists()) {
      m_config.noise_high_threshold =
          smartrc_config["NoiseHighThreshold"].get<uint32_t>(12);
    } else {
      m_config.noise_high_threshold = 12;
    }
    if (smartrc_config["StreamConfig"].exists()) {
      for (i = 0; i < AM_STREAM_ID_MAX; i ++) {
        AM_STREAM_ID id = AM_STREAM_ID(i);
        if (smartrc_config["StreamConfig"][i]["Enable"].exists()) {
          m_config.stream_params[id].enable =
              smartrc_config["StreamConfig"][i]["Enable"].get<bool>(false);
        } else {
          m_config.stream_params[id].enable = false;
        }
        if (smartrc_config["StreamConfig"][i]["FrameDrop"].exists()) {
          m_config.stream_params[id].frame_drop =
              smartrc_config["StreamConfig"][i]["FrameDrop"].get<bool>(false);
        } else {
          m_config.stream_params[id].frame_drop = false;
        }
        if (smartrc_config["StreamConfig"][i]["DynamicGOPN"].exists()) {
          m_config.stream_params[id].dynamic_GOP_N =
              smartrc_config["StreamConfig"][i]["DynamicGOPN"].get<bool>(false);
        } else {
          m_config.stream_params[id].dynamic_GOP_N = false;
        }
        if (smartrc_config["StreamConfig"][i]["MBLevelControl"].exists()) {
          m_config.stream_params[id].MB_level_control =
              smartrc_config["StreamConfig"][i]["MBLevelControl"].get<bool>(false);
        } else {
          m_config.stream_params[id].MB_level_control = false;
        }
        if (smartrc_config["StreamConfig"][i]["BitrateCeiling"].exists()) {
          m_config.stream_params[id].bitrate_ceiling =
              smartrc_config["StreamConfig"][i]["BitrateCeiling"].get<uint32_t>(142);
        } else {
          m_config.stream_params[id].bitrate_ceiling = 142;
        }
      }
    }
  } while (0);

  delete config_ptr;
  return result;
}

AM_RESULT AMSmartRCConfig::save_config()
{
  AUTO_LOCK_SMARTRC(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  uint32_t i;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = SMARTRC_CONF_DIR;
  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + SMARTRC_CONFIG_FILE;
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMSmartRCConfig::Create AMConfig failed!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &smartrc_config = *config_ptr;

    if (smartrc_config["LogLevel"].exists()) {
      smartrc_config["LogLevel"] = m_config.log_level;
    }
    if (smartrc_config["NoiseLowThreshold"].exists()) {
      smartrc_config["NoiseLowThreshold"] = m_config.noise_low_threshold;
    }
    if (smartrc_config["NoiseHighThreshold"].exists()) {
      smartrc_config["NoiseHighThreshold"] = m_config.noise_high_threshold;
    }
    if (smartrc_config["StreamConfig"].exists()) {
      for (i = 0; i < AM_STREAM_ID_MAX; i ++) {
        AM_STREAM_ID id = AM_STREAM_ID(i);
        if (smartrc_config["StreamConfig"][i]["Enable"].exists()) {
          smartrc_config["StreamConfig"][i]["Enable"] =
              m_config.stream_params[id].enable;
        }
        if (smartrc_config["StreamConfig"][i]["FrameDrop"].exists()) {
          smartrc_config["StreamConfig"][i]["FrameDrop"] =
              m_config.stream_params[id].frame_drop;
        }
        if (smartrc_config["StreamConfig"][i]["DynamicGOPN"].exists()) {
          smartrc_config["StreamConfig"][i]["DynamicGOPN"] =
              m_config.stream_params[id].dynamic_GOP_N;
        }
        if (smartrc_config["StreamConfig"][i]["MBLevelControl"].exists()) {
          smartrc_config["StreamConfig"][i]["MBLevelControl"] =
              m_config.stream_params[id].MB_level_control;
        }
        if (smartrc_config["StreamConfig"][i]["BitrateCeiling"].exists()) {
          smartrc_config["StreamConfig"][i]["BitrateCeiling"] =
              m_config.stream_params[id].bitrate_ceiling;
        }
      }
    }

    if (!smartrc_config.save()) {
      ERROR("AMSmartRCConfig: failed to save_config\n");
      result = AM_RESULT_ERR_IO;
      break;
    }

  } while (0);
  delete config_ptr;
  return result;
}

AMSmartRCConfig::AMSmartRCConfig() :
  m_ref_cnt(0)
{
}

AMSmartRCConfig::~AMSmartRCConfig()
{
}

void AMSmartRCConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMSmartRCConfig::release()
{
  AUTO_LOCK_SMARTRC(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}
