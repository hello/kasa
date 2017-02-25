/**
 * am_encode_config.cpp
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_video_types.h"
#include "am_configure.h"
#include "am_encode_types.h"
#include "am_video_utility.h"
#include "am_encode_config.h"

#define VIN_CONFIG_FILE             "vin.acs"
#define VOUT_CONFIG_FILE            "vout.acs"
#define STREAM_FORMAT_FILE          "stream_fmt.acs"
#define STREAM_CONFIG_FILE          "stream_cfg.acs"
#define SOURCE_BUFFER_CONFIG_FILE   "source_buffer.acs"

#define DEFAULT_ORYX_CONFIG_DIR     "/etc/oryx/video/"

#define AUTO_LOCK_ENC_CFG(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMVinConfig *AMVinConfig::m_instance = nullptr;
std::recursive_mutex AMVinConfig::m_mtx;
AMVoutConfig *AMVoutConfig::m_instance = nullptr;
std::recursive_mutex AMVoutConfig::m_mtx;
AMBufferConfig *AMBufferConfig::m_instance = nullptr;
std::recursive_mutex AMBufferConfig::m_mtx;
AMStreamConfig *AMStreamConfig::m_instance = nullptr;
std::recursive_mutex AMStreamConfig::m_mtx;

AMVinConfigPtr AMVinConfig::get_instance()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMVinConfig();
  }
  return m_instance;
}

void AMVinConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMVinConfig::release()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMVinConfig::AMVinConfig() :
    m_ref_cnt(0)
{
  DEBUG("AMVinConfig is created!");
}

AMVinConfig::~AMVinConfig()
{
  DEBUG("AMVinConfig is destroyed!");
}

AM_RESULT AMVinConfig::get_config(AMVinParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMVinConfig::set_config(const AMVinParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  do {
    m_config = config;
    if ((result = save_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVinConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }

    tmp = std::string(oryx_config_dir) + std::string(VIN_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;
    for (int32_t i = 0; i < config.length(); ++i) {
      AMVinParam param;
      if (config[i]["close_on_idle"].exists()) {
        param.close_on_idle.second = config[i]["close_on_idle"].get<bool>(true);
      }

      if (config[i]["type"].exists()) {
        tmp = config[i]["type"].get<std::string>("");
        param.type.second = AMVinTrans::type_str_to_enum(tmp);
      }

      if (config[i]["mode"].exists()) {
        tmp = config[i]["mode"].get<std::string>("");
        param.mode.second = AMVinTrans::mode_str_to_enum(tmp);
      }

      if (config[i]["flip"].exists()) {
        tmp = config[i]["flip"].get<std::string>("");
        param.flip.second = AMVideoTrans::flip_str_to_enum(tmp);
      }

      if (config[i]["fps"].exists()) {
        tmp = config[i]["fps"].get<std::string>("");
        param.fps.second = AMVideoTrans::fps_str_to_enum(tmp);
      }
      m_config[AM_VIN_ID(i)] = param;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMVinConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(VIN_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    for (auto &m : m_config) {
      if (m.second.type.first) {
        config[m.first]["type"] =
            AMVinTrans::type_enum_to_str(m.second.type.second);
      }
      if (m.second.mode.first) {
        config[m.first]["mode"] =
            AMVinTrans::mode_enum_to_str(m.second.mode.second);
      }
      if (m.second.flip.first) {
        config[m.first]["flip"] =
            AMVideoTrans::flip_enum_to_str(m.second.flip.second);
      }
      if (m.second.fps.first) {
        config[m.first]["fps"] =
            AMVideoTrans::fps_enum_to_str(m.second.fps.second);
      }
    }

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AMVoutConfigPtr AMVoutConfig::get_instance()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMVoutConfig();
  }
  return m_instance;
}

void AMVoutConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMVoutConfig::release()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMVoutConfig::AMVoutConfig() :
    m_ref_cnt(0)
{
  DEBUG("AMVoutConfig is created!");
}

AMVoutConfig::~AMVoutConfig()
{
  DEBUG("AMVoutConfig is destroyed!");
  m_config.clear();
}

AM_RESULT AMVoutConfig::get_config(AMVoutParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);

  return result;
}

AM_RESULT AMVoutConfig::set_config(const AMVoutParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  m_config = config;

  return save_config();
}

AM_RESULT AMVoutConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }

    tmp = std::string(oryx_config_dir) + std::string(VOUT_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;
    for (uint32_t i = 0; i < AM_VOUT_MAX_NUM; ++i) {
      AMVoutParam param;
      if (config[i]["type"].exists()) {
        tmp = config[i]["type"].get<std::string>("");
        param.type.second = AMVoutTrans::sink_type_str_to_enum(tmp.c_str());
        INFO("vout%d type = %d", i, param.type.second);
      }

      if (config[i]["video_type"].exists()) {
        tmp = config[i]["video_type"].get<std::string>("");
        param.video_type.second = AMVoutTrans::video_type_str_to_enum(tmp.c_str());
        INFO("vout%d video type = %d", i, param.video_type.second);
      }

      if (config[i]["mode"].exists()) {
        tmp = config[i]["mode"].get<std::string>("");
        param.mode.second = AMVoutTrans::mode_str_to_enum(tmp.c_str());
      }

      if (config[i]["flip"].exists()) {
        tmp = config[i]["flip"].get<std::string>("");
        param.flip.second = AMVideoTrans::flip_str_to_enum(tmp.c_str());
      }

      if (config[i]["rotate"].exists()) {
        tmp = config[i]["rotate"].get<std::string>("");
        param.rotate.second = AMVideoTrans::rotate_str_to_enum(tmp.c_str());
      }

      if (config[i]["fps"].exists()) {
        tmp = config[i]["fps"].get<std::string>("");
        param.fps.second = AMVideoTrans::fps_str_to_enum(tmp.c_str());
      }

      m_config[AM_VOUT_ID(i)] = param;
    }
  } while (0);
  delete config_ptr;

  return result;
}

AM_RESULT AMVoutConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;

  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  AMConfig *config_ptr = nullptr;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(VOUT_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;

    for (auto &m : m_config) {
      if (m.first >= AM_VOUT_MAX_NUM || m.first <= AM_VOUT_ID_INVALID) {
        continue;
      }
      if (m.second.type.first) {
        config[m.first]["type"] =
            AMVoutTrans::sink_type_enum_to_str(m.second.type.second);
      }
      if (m.second.video_type.first) {
        config[m.first]["video_type"] =
            AMVoutTrans::video_type_enum_to_str(m.second.video_type.second);
      }
      if (m.second.mode.first) {
        config[m.first]["mode"] =
            AMVoutTrans::mode_enum_to_str(m.second.mode.second);
      }
      if (m.second.flip.first) {
        config[m.first]["flip"] =
            AMVideoTrans::flip_enum_to_str(m.second.flip.second);
      }
      if (m.second.rotate.first) {
        config[m.first]["rotate"] =
            AMVideoTrans::rotate_enum_to_str(m.second.rotate.second);
      }
      if (m.second.fps.first) {
        config[m.first]["fps"] =
            AMVideoTrans::fps_enum_to_str(m.second.fps.second);
      }
    }

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;

  return result;
}

AMBufferConfigPtr AMBufferConfig::get_instance()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMBufferConfig();
  }
  return m_instance;
}

void AMBufferConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMBufferConfig::release()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMBufferConfig::AMBufferConfig() :
    m_ref_cnt(0)
{
  DEBUG("AMBufferConfig is created!");
}

AMBufferConfig::~AMBufferConfig()
{
  DEBUG("AMBufferConfig is destroyed!");
}

AM_RESULT AMBufferConfig::get_config(AMBufferParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMBufferConfig::set_config(const AMBufferParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  do {
    m_config = config;
    if ((result = save_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMBufferConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(SOURCE_BUFFER_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;
    if (!config["buffer"].exists()) {
      ERROR("Invalid configuration!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    for (uint32_t i = 0; i < config["buffer"].length(); ++i) {
      AMBufferConfigParam param;
      if (config["buffer"][i]["type"].exists()) {
        tmp = config["buffer"][i]["type"].get<std::string>("");
        param.type.second = AMVideoTrans::buffer_type_str_to_enum(tmp);
      }

      if (config["buffer"][i]["size"].exists()) {
        param.size.second.width = config["buffer"][i]["size"][0].get<int>(0);
        param.size.second.height = config["buffer"][i]["size"][1].get<int>(0);
      }

      if (config["buffer"][i]["input_crop"].exists() &&
          (param.input.first =
              config["buffer"][i]["input_crop"].get<bool>(false)) &&
              config["buffer"][i]["input_rect"].exists()) {
        param.input.second.size.width =
            config["buffer"][i]["input_rect"][0].get<int>(0);
        param.input.second.size.height =
            config["buffer"][i]["input_rect"][1].get<int>(0);
        param.input.second.offset.x =
            config["buffer"][i]["input_rect"][2].get<int>(0);
        param.input.second.offset.y =
            config["buffer"][i]["input_rect"][3].get<int>(0);
      }

      if (config["buffer"][i]["prewarp"].exists()) {
        param.prewarp.second = config["buffer"][i]["prewarp"].get<bool>(false);
      }
      m_config[AM_SOURCE_BUFFER_ID(i)] = param;
    }

    if (config["buffer"]["efm"].exists()) {
      AMBufferConfigParam param;
      if (config["buffer"]["efm"]["type"].exists()) {
        tmp = config["buffer"]["efm"]["type"].get<std::string>("");
        param.type.second = AMVideoTrans::buffer_type_str_to_enum(tmp);
      }

      if (config["buffer"]["efm"]["size"].exists()) {
        param.size.second.width = config["buffer"]["efm"]["size"][0].get<int>(-1);
        param.size.second.height = config["buffer"]["efm"]["size"][1].get<int>(-1);
      }
      m_config[AM_SOURCE_BUFFER_EFM] = param;
    }

    if (config["buffer"]["pre_main"].exists()) {
      AMBufferConfigParam param;
      if (config["buffer"]["pre_main"]["type"].exists()) {
        tmp = config["buffer"]["pre_main"]["type"].get<std::string>("");
        param.type.second = AMVideoTrans::buffer_type_str_to_enum(tmp);
      }

      if (config["buffer"]["pre_main"]["input_crop"].exists() &&
          config["buffer"]["pre_main"]["input_crop"].get<bool>(true) &&
          config["buffer"]["pre_main"]["input_rect"].exists()) {
        param.input.second.size.width =
            config["buffer"]["pre_main"]["input_rect"][0].get<int>(-1);
        param.input.second.size.height =
            config["buffer"]["pre_main"]["input_rect"][1].get<int>(-1);
        param.input.second.offset.x =
            config["buffer"]["pre_main"]["input_rect"][2].get<int>(-1);
        param.input.second.offset.y =
            config["buffer"]["pre_main"]["input_rect"][3].get<int>(-1);
      }

      if (config["buffer"]["pre_main"]["size"].exists()) {
        param.size.second.width = config["buffer"]["pre_main"]["size"][0].get
                                  <int>(-1);
        param.size.second.height = config["buffer"]["pre_main"]["size"][1].get
                                   <int>(-1);
      }

      m_config[AM_SOURCE_BUFFER_PMN] = param;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMBufferConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(SOURCE_BUFFER_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    for (auto &m : m_config) {
      if (m.second.type.first) {
        config["buffer"][m.first]["type"] =
            AMVideoTrans::buffer_type_to_str(m.second.type.second);
      }
      if (m.second.input_crop.first) {
        config["buffer"][m.first]["input_crop"] = m.second.input_crop.second;
      }
      if (m.second.input.first) {
        if (m.second.input.second.size.width != -1) {
          config["buffer"][m.first]["input_rect"][0] = m.second.input.second.size.width;
        }
        if (m.second.input.second.size.height != -1) {
          config["buffer"][m.first]["input_rect"][1] = m.second.input.second.size.height;
        }
        if (m.second.input.second.offset.x != -1) {
          config["buffer"][m.first]["input_rect"][2] = m.second.input.second.offset.x;
        }
        if (m.second.input.second.offset.y != -1) {
          config["buffer"][m.first]["input_rect"][3] = m.second.input.second.offset.y;
        }
      }
      if (m.second.size.first) {
        if (m.second.size.second.width !=  -1) {
          config["buffer"][m.first]["size"][0] = m.second.size.second.width;
        }
        if (m.second.size.second.height !=  -1) {
          config["buffer"][m.first]["size"][1] = m.second.size.second.height;
        }
      }
      if (m.second.prewarp.first) {
        config["buffer"][m.first]["prewarp"] = m.second.prewarp.second;
      }
    }

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AMStreamConfigPtr AMStreamConfig::get_instance()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMStreamConfig();
  }
  return m_instance;
}

void AMStreamConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMStreamConfig::release()
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMStreamConfig::AMStreamConfig() :
    m_ref_cnt(0)
{
  DEBUG("AMStreamConfig is created!");
}

AMStreamConfig::~AMStreamConfig()
{
  DEBUG("AMStreamConfig is destroyed!");
}

AM_RESULT AMStreamConfig::get_config(AMStreamParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMStreamConfig::set_config(const AMStreamParamMap &config)
{
  AUTO_LOCK_ENC_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  do {
    m_config = config;
    if ((result = save_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMStreamConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = load_stream_format()) != AM_RESULT_OK) {
      break;
    }
    if ((result = load_stream_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMStreamConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = save_stream_format()) != AM_RESULT_OK) {
      break;
    }
    if ((result = save_stream_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMStreamConfig::load_stream_format()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(STREAM_FORMAT_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;
    for (int32_t i = 0; i < config.length(); ++i) {
      AMStreamFormatConfig param;
      if (config[i]["enable"].exists()) {
          param.enable.second = config[i]["enable"].get<bool>(false);
      }

      if (config[i]["type"].exists()) {
        tmp = config[i]["type"].get<std::string>("");
        param.type.second = AMVideoTrans::stream_type_str_to_enum(tmp);
      }

      if (config[i]["source"].exists()) {
        param.source.second = AM_SOURCE_BUFFER_ID(config[i]["source"].get<int>(0));
      }

      if (config[i]["frame_rate"].exists()) {
        param.fps.second = config[i]["frame_rate"].get<int>(0);
      }

      if (config[i]["enc_rect"].exists()) {
        param.enc_win.second.size.width = config[i]["enc_rect"][0].get<int>(0);
        param.enc_win.second.size.height = config[i]["enc_rect"][1].get<int>(0);
        param.enc_win.second.offset.x = config[i]["enc_rect"][2].get<int>(0);
        param.enc_win.second.offset.y = config[i]["enc_rect"][3].get<int>(0);
      }

      if (config[i]["flip"].exists()) {
        std::string flip = config[i]["flip"].get<std::string>("none");
        if (is_str_equal(flip.c_str(), "hflip")) {
          param.flip.second = AM_VIDEO_FLIP_HORIZONTAL;
        } else if (is_str_equal(flip.c_str(), "vflip")) {
          param.flip.second = AM_VIDEO_FLIP_VERTICAL;
        } else if (is_str_equal(flip.c_str(), "both")) {
          param.flip.second = AM_VIDEO_FLIP_VH_BOTH;
        }
      }

      if (config[i]["rotate_90_cw"].exists()) {
        param.rotate_90_cw.second = config[i]["rotate_90_cw"].get<bool>(false);
      }
      m_config[AM_STREAM_ID(i)].stream_format.second = param;
      m_config[AM_STREAM_ID(i)].stream_format.first = true;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMStreamConfig::save_stream_format()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(STREAM_FORMAT_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    for (auto &m : m_config) {
      if (m.second.stream_format.first) {
        if (m.second.stream_format.second.enable.first) {
          config[m.first]["enable"] =
              m.second.stream_format.second.enable.second;
        }
        if (m.second.stream_format.second.type.first) {
          config[m.first]["type"] =
              AMVideoTrans::stream_type_enum_to_str(
                  m.second.stream_format.second.type.second);
        }
        if (m.second.stream_format.second.source.first) {
          config[m.first]["source"] =
              (uint32_t)m.second.stream_format.second.source.second;
        }
        if (m.second.stream_format.second.fps.first) {
          config[m.first]["frame_rate"] =
                  m.second.stream_format.second.fps.second;
        }
        if (m.second.stream_format.second.enc_win.first) {
          if (m.second.stream_format.second.enc_win.second.size.width != -1) {
            config[m.first]["enc_rect"][0] =
                m.second.stream_format.second.enc_win.second.size.width;
          }
          if (m.second.stream_format.second.enc_win.second.size.height != -1) {
            config[m.first]["enc_rect"][1] =
                m.second.stream_format.second.enc_win.second.size.height;
          }
          if (m.second.stream_format.second.enc_win.second.offset.x != -1) {
            config[m.first]["enc_rect"][2] =
                m.second.stream_format.second.enc_win.second.offset.x;
          }
          if (m.second.stream_format.second.enc_win.second.offset.y != -1) {
            config[m.first]["enc_rect"][3] =
                m.second.stream_format.second.enc_win.second.offset.y;
          }
        }
        if (m.second.stream_format.second.flip.first) {
          switch (m.second.stream_format.second.flip.second) {
            case AM_VIDEO_FLIP_HORIZONTAL:
              config[m.first]["flip"] = std::string("hflip");
              break;
            case AM_VIDEO_FLIP_VERTICAL:
              config[m.first]["flip"] = std::string("vflip");
              break;
            case AM_VIDEO_FLIP_VH_BOTH:
              config[m.first]["flip"] = std::string("both");
              break;
            default:
              config[m.first]["flip"] = std::string("none");
              break;
          }
        }
        if (m.second.stream_format.second.rotate_90_cw.first) {
          config[m.first]["rotate_90_cw"] =
              m.second.stream_format.second.rotate_90_cw.second;
        }
      }
    }

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMStreamConfig::load_stream_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(STREAM_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;
    for (int32_t i = 0; i < config.length(); ++i) {
      AMStreamH26xConfig h26x_param;
      if (config[i]["h26x_config"]["gop_model"].exists()) {
        h26x_param.gop_model.second =
            AM_GOP_MODEL(config[i]["h26x_config"]["gop_model"].get<int>(0));
      }

      if (config[i]["h26x_config"]["bitrate_control"].exists()) {
        tmp = config[i]["h26x_config"]["bitrate_control"].get<std::string>("");
        h26x_param.bitrate_control.second =
            AMVideoTrans::stream_bitrate_control_str_to_enum(tmp);
      }

      if (config[i]["h26x_config"]["profile_level"].exists()) {
        tmp = config[i]["h26x_config"]["profile_level"].get<std::string>("");
        h26x_param.profile_level.second =
            AMVideoTrans::stream_profile_str_to_enum(tmp);
      }

      if (config[i]["h26x_config"]["au_type"].exists()) {
        h26x_param.au_type.second =
            AM_AU_TYPE(config[i]["h26x_config"]["au_type"].get<int>(0));
      }

      if (config[i]["h26x_config"]["chroma_format"].exists()) {
        tmp = config[i]["h26x_config"]["chroma_format"].get<std::string>("");
        h26x_param.chroma_format.second =
            AMVideoTrans::stream_chroma_str_to_enum(tmp);
      }

      if (config[i]["h26x_config"]["M"].exists()) {
        h26x_param.M.second = config[i]["h26x_config"]["M"].get<int>(0);
      }

      if (config[i]["h26x_config"]["N"].exists()) {
        h26x_param.N.second = config[i]["h26x_config"]["N"].get<int>(0);
      }

      if (config[i]["h26x_config"]["idr_interval"].exists()) {
        h26x_param.idr_interval.second =
            config[i]["h26x_config"]["idr_interval"].get<int>(0);
      }

      if (config[i]["h26x_config"]["target_bitrate"].exists()) {
        h26x_param.target_bitrate.second =
            config[i]["h26x_config"]["target_bitrate"].get<int>(0);
      }

      if (config[i]["h26x_config"]["mv_threshold"].exists()) {
        h26x_param.mv_threshold.second =
            config[i]["h26x_config"]["mv_threshold"].get<int>(0);
      }

      if (config[i]["h26x_config"]["flat_area_improve"].exists()) {
        h26x_param.flat_area_improve.second =
            config[i]["h26x_config"]["flat_area_improve"].get<bool>(false);
      }

      if (config[i]["h26x_config"]["multi_ref_p"].exists()) {
        h26x_param.multi_ref_p.second =
            config[i]["h26x_config"]["multi_ref_p"].get<bool>(false);
      }

      if (config[i]["h26x_config"]["fast_seek_intvl"].exists()) {
        h26x_param.fast_seek_intvl.second =
            config[i]["h26x_config"]["fast_seek_intvl"].get<int>(0);
      }

      if (config[i]["h26x_config"]["sar"].exists()) {
        int32_t num = 0, den = 0;
        if (sscanf(config[i]["h26x_config"]["sar"].get<std::string>("1/1").c_str(),
                   "%d/%d", &num, &den) != 2 || num < 0 || den < 0) {
          WARN("Invalid sar value:%s, set to default 1/1",
               config[i]["h26x_config"]["sar"].get<std::string>("").c_str());
          num = 1;
          den = 1;
        }
        h26x_param.sar.second.width = num;
        h26x_param.sar.second.height = den;
      }

      if (config[i]["h26x_config"]["i_frame_max_size"].exists()) {
        h26x_param.i_frame_max_size.second =
            config[i]["h26x_config"]["i_frame_max_size"].get<int>(-1);
      }

      m_config[AM_STREAM_ID(i)].h26x_config.first = true;
      m_config[AM_STREAM_ID(i)].h26x_config.second = h26x_param;

      AMStreamMJPEGConfig mjpeg_param;
      if (config[i]["mjpeg_config"]["quality"].exists()) {
        mjpeg_param.quality.second =
            config[i]["mjpeg_config"]["quality"].get<int>(0);
      }

      if (config[i]["mjpeg_config"]["chroma_format"].exists()) {
        tmp = config[i]["mjpeg_config"]["chroma_format"].get<std::string>("");
        mjpeg_param.chroma_format.second =
            AMVideoTrans::stream_chroma_str_to_enum(tmp);
      }

      m_config[AM_STREAM_ID(i)].mjpeg_config.first = true;
      m_config[AM_STREAM_ID(i)].mjpeg_config.second = mjpeg_param;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMStreamConfig::save_stream_config()
{

  AM_RESULT result = AM_RESULT_OK;

  char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
  AMConfig *config_ptr = nullptr;

  do {
    std::string tmp;
    char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_config_dir) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + std::string(STREAM_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    for (auto &m : m_config) {
      if (m.second.h26x_config.first) {
        if (m.second.h26x_config.second.gop_model.first) {
          config[m.first]["h26x_config"]["gop_model"] =
              (uint32_t)m.second.h26x_config.second.gop_model.second;
        }
        if (m.second.h26x_config.second.bitrate_control.first) {
          config[m.first]["h26x_config"]["bitrate_control"] =
              AMVideoTrans::stream_bitrate_control_enum_to_str(
                  m.second.h26x_config.second.bitrate_control.second);
        }
        if (m.second.h26x_config.second.profile_level.first) {
          config[m.first]["h26x_config"]["profile_level"] =
              AMVideoTrans::stream_profile_enum_to_str(
                  m.second.h26x_config.second.profile_level.second);
        }
        if (m.second.h26x_config.second.au_type.first) {
          config[m.first]["h26x_config"]["au_type"] =
              (uint32_t)m.second.h26x_config.second.au_type.second;
        }
        if (m.second.h26x_config.second.chroma_format.first) {
          config[m.first]["h26x_config"]["chroma_format"] =
              AMVideoTrans::stream_chroma_to_str(
                  m.second.h26x_config.second.chroma_format.second);
        }
        if (m.second.h26x_config.second.M.first) {
          config[m.first]["h26x_config"]["M"] =
              m.second.h26x_config.second.M.second;
        }
        if (m.second.h26x_config.second.N.first) {
          config[m.first]["h26x_config"]["N"] =
              m.second.h26x_config.second.N.second;
        }
        if (m.second.h26x_config.second.idr_interval.first) {
          config[m.first]["h26x_config"]["idr_interval"] =
              m.second.h26x_config.second.idr_interval.second;
        }
        if (m.second.h26x_config.second.target_bitrate.first) {
          config[m.first]["h26x_config"]["target_bitrate"] =
              m.second.h26x_config.second.target_bitrate.second;
        }
        if (m.second.h26x_config.second.mv_threshold.first) {
          config[m.first]["h26x_config"]["mv_threshold"] =
              m.second.h26x_config.second.mv_threshold.second;
        }
        if (m.second.h26x_config.second.fast_seek_intvl.first) {
          config[m.first]["h26x_config"]["fast_seek_intvl"] =
              m.second.h26x_config.second.fast_seek_intvl.second;
        }
        if (m.second.h26x_config.second.flat_area_improve.first) {
          config[m.first]["h26x_config"]["flat_area_improve"] =
              m.second.h26x_config.second.flat_area_improve.second;
        }
        if (m.second.h26x_config.second.multi_ref_p.first) {
          config[m.first]["h26x_config"]["multi_ref_p"] =
              m.second.h26x_config.second.multi_ref_p.second;
        }
        if (m.second.h26x_config.second.sar.first) {
          char sar[128];
          snprintf(sar, 128, "%d/%d",
                   m.second.h26x_config.second.sar.second.width,
                   m.second.h26x_config.second.sar.second.height);
          config[m.first]["h26x_config"]["sar"] = std::string(sar);
        }
        if (m.second.h26x_config.second.i_frame_max_size.first) {
          config[m.first]["h26x_config"]["i_frame_max_size"] =
              m.second.h26x_config.second.i_frame_max_size.second;
        }
      }

      if (m.second.mjpeg_config.first) {
        if (m.second.mjpeg_config.second.chroma_format.first) {
          config[m.first]["mjpeg_config"]["chroma_format"] =
              AMVideoTrans::stream_chroma_to_str(
                  m.second.mjpeg_config.second.chroma_format.second);
        }
        if (m.second.mjpeg_config.second.quality.first) {
          config[m.first]["mjpeg_config"]["quality"] =
              m.second.mjpeg_config.second.quality.second;
        }
      }
    }

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;
  return result;
}
