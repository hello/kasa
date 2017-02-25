/*******************************************************************************
 * am_encode_eis_config.cpp
 *
 * History:
 *   Feb 26, 2016 - [smdong] created file
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

#include "am_encode_eis_config.h"
#include "am_configure.h"

AMEISConfig *AMEISConfig::m_instance = nullptr;
std::recursive_mutex AMEISConfig::m_mtx;
#define AUTO_LOCK_EIS(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMEISConfig::AMEISConfig() :
    m_ref_cnt(0)
{
}

AMEISConfig::~AMEISConfig()
{
  DEBUG("~AMEISConfig");
}

AM_RESULT AMEISConfig::load_config()
{
  AUTO_LOCK_EIS(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = EIS_CONF_DIR;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + EIS_CONFIG_FILE;
    if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &config = *config_ptr;

    if (AM_LIKELY(config["eis_mode"].exists())) {
      int32_t mode = config["eis_mode"].get<int>(0);
      m_config.eis_mode.first = true;
      m_config.eis_mode.second = EIS_FLAG(mode);
      if (AM_UNLIKELY(mode < 0)) {
        ERROR("Invalid EIS mode %d", mode);
      }
    }

    if (AM_LIKELY(config["cali_num"].exists())) {
      m_config.cali_num.first = true;
      m_config.cali_num.second = config["cali_num"].get<int>(3000);
    }

    if (AM_LIKELY(config["accel_full_scale_range"].exists())) {
      m_config.accel_full_scale_range.first = true;
      m_config.accel_full_scale_range.second =
          config["accel_full_scale_range"].get<double>(0.0);
    }

    if (AM_LIKELY(config["accel_lsb"].exists())) {
      m_config.accel_lsb.first = true;
      m_config.accel_lsb.second = config["accel_lsb"].get<double>(0.0);
    }

    if (AM_LIKELY(config["gyro_full_scale_range"].exists())) {
      m_config.gyro_full_scale_range.first = true;
      m_config.gyro_full_scale_range.second =
          config["gyro_full_scale_range"].get<double>(0.0);
    }

    if (AM_LIKELY(config["gyro_lsb"].exists())) {
      m_config.gyro_lsb.first = true;
      m_config.gyro_lsb.second = config["gyro_lsb"].get<double>(0.0);
    }

    if (AM_LIKELY(config["gyro_sample_rate_in_hz"].exists())) {
      m_config.gyro_sample_rate_in_hz.first = true;
      m_config.gyro_sample_rate_in_hz.second =
          config["gyro_sample_rate_in_hz"].get<int>(0);
    }

    if (AM_LIKELY(config["gyro_shift"].exists())) {
      m_config.gyro_shift.first = true;
      if (AM_LIKELY(config["gyro_shift"]["gyro_x"].exists())) {
        m_config.gyro_shift.second.gyro_x =
            config["gyro_shift"]["gyro_x"].get<int>(0);
      }
      if (AM_LIKELY(config["gyro_shift"]["gyro_y"].exists())) {
        m_config.gyro_shift.second.gyro_y =
            config["gyro_shift"]["gyro_y"].get<int>(0);
      }
      if (AM_LIKELY(config["gyro_shift"]["gyro_z"].exists())) {
        m_config.gyro_shift.second.gyro_z =
            config["gyro_shift"]["gyro_z"].get<int>(0);
      }
      if (AM_LIKELY(config["gyro_shift"]["accel_x"].exists())) {
        m_config.gyro_shift.second.accel_x =
            config["gyro_shift"]["accel_x"].get<int>(0);
      }
      if (AM_LIKELY(config["gyro_shift"]["accel_y"].exists())) {
        m_config.gyro_shift.second.accel_y =
            config["gyro_shift"]["accel_y"].get<int>(0);
      }
      if (AM_LIKELY(config["gyro_shift"]["accel_z"].exists())) {
        m_config.gyro_shift.second.accel_z =
            config["gyro_shift"]["accel_z"].get<int>(0);
      }
    }

    if (AM_LIKELY(config["gravity"].exists())) {
      int32_t axis = config["gravity"].get<int>(0);
      m_config.gravity.first = true;
      m_config.gravity.second = AXIS(axis);
    }

    if (AM_LIKELY(config["file_name"].exists())) {
      m_config.file_name.first = true;
      m_config.file_name.second = config["file_name"].get<std::string>("");
    }

    if (AM_LIKELY(config["lens_focal_length_in_um"].exists())) {
      m_config.lens_focal_length_in_um.first = true;
      m_config.lens_focal_length_in_um.second =
          config["lens_focal_length_in_um"].get<int>(0);
    }

    if (AM_LIKELY(config["threshhold"].exists())) {
      m_config.threshhold.first = true;
      m_config.threshhold.second = config["threshhold"].get<double>(0.0);
    }

    if (AM_LIKELY(config["frame_buffer_num"].exists())) {
      m_config.frame_buffer_num.first = true;
      m_config.frame_buffer_num.second = config["frame_buffer_num"].get<int>(0);
    }

    if (AM_LIKELY(config["avg_mode"].exists())) {
      int32_t avg_mode = config["avg_mode"].get<int>(0);
      m_config.avg_mode.first = true;
      m_config.avg_mode.second = EIS_AVG_MODE(avg_mode);
    }

  } while(0);

  delete config_ptr;
  return result;

}

AM_RESULT AMEISConfig::save_config()
{
  AUTO_LOCK_EIS(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  const char *default_dir = EIS_CONF_DIR;
  AMConfig *config_ptr = nullptr;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + EIS_CONFIG_FILE;
    if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig& config = *config_ptr;

    config["eis_mode"] = (int)m_config.eis_mode.second;
    config["cali_num"] = m_config.cali_num.second;

    config["accel_full_scale_range"] = m_config.accel_full_scale_range.second;
    config["accel_lsb"] = m_config.accel_lsb.second;
    config["gyro_full_scale_range"] = m_config.gyro_full_scale_range.second;
    config["gyro_lsb"] = m_config.gyro_lsb.second;
    config["gyro_sample_rate_in_hz"] = m_config.gyro_sample_rate_in_hz.second;

    config["gyro_shift"]["gyro_x"] = m_config.gyro_shift.second.gyro_x;
    config["gyro_shift"]["gyro_y"] = m_config.gyro_shift.second.gyro_y;
    config["gyro_shift"]["gyro_z"] = m_config.gyro_shift.second.gyro_z;
    config["gyro_shift"]["accel_x"] = m_config.gyro_shift.second.accel_x;
    config["gyro_shift"]["accel_y"] = m_config.gyro_shift.second.accel_y;
    config["gyro_shift"]["accel_z"] = m_config.gyro_shift.second.accel_z;

    config["gravity"] = (int)m_config.gravity.second;

    config["file_name"] = m_config.file_name.second;
    config["lens_focal_length_in_um"] = m_config.lens_focal_length_in_um.second;
    config["threshhold"] = m_config.threshhold.second;
    config["frame_buffer_num"] = m_config.frame_buffer_num.second;
    config["avg_mode"] = (int)m_config.avg_mode.second;

    if (AM_UNLIKELY(!config_ptr->save())) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while(0);
  delete config_ptr;
  return result;
}

AM_RESULT AMEISConfig::get_config(AMEISConfigParam &config)
{
  AUTO_LOCK_EIS(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMEISConfig::set_config(const AMEISConfigParam &config)
{
  AUTO_LOCK_EIS(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_config = config;
  } while (0);
  return result;
}

AMEISConfigPtr AMEISConfig::get_instance()
{
  AUTO_LOCK_EIS(m_mtx);
  if (!m_instance) {
    m_instance = new AMEISConfig();
  }
  return m_instance;
}

void AMEISConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMEISConfig::release()
{
  AUTO_LOCK_EIS(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}
