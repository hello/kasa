/*******************************************************************************
 * am_encode_warp_config_s2l.cpp
 *
 * History:
 *   2016/1/6 - [smdong] created file
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

#include "am_encode_warp_config_s2l.h"
#include "am_configure.h"

AMWarpConfigS2L *AMWarpConfigS2L::m_instance = nullptr;
std::recursive_mutex AMWarpConfigS2L::m_mtx;
#define AUTO_LOCK_WARP(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMWarpConfigS2L::AMWarpConfigS2L() :
    m_ref_cnt(0)
{
}

AMWarpConfigS2L::~AMWarpConfigS2L()
{
  DEBUG("~AMWarpConfigS2L");
}


AM_RESULT AMWarpConfigS2L::load_config()
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = WARP_CONF_DIR;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + WARP_CONFIG_FILE;
    if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& config = *config_ptr;

    if (AM_LIKELY(config["proj_mode"].exists())) {
      int32_t mode = config["proj_mode"].get<int>(0);
      m_config.proj_mode.first = true;
      m_config.proj_mode.second = AM_WARP_PROJECTION_MODE(mode);
    }
    if (AM_LIKELY(config["warp_mode"].exists())) {
      int32_t mode = config["warp_mode"].get<int>(0);
      m_config.warp_mode.first = true;
      m_config.warp_mode.second = AM_WARP_MODE(mode);
    }
    if (AM_LIKELY(config["max_radius"].exists())) {
      m_config.max_radius.first = true;
      m_config.max_radius.second = config["max_radius"].get<int>(0);
    }
    if (AM_LIKELY(config["ldc_strength"].exists())) {
      m_config.ldc_strength.first = true;
      m_config.ldc_strength.second = config["ldc_strength"].get<float>(0);
      if (AM_UNLIKELY((m_config.ldc_strength.second > 20.0) ||
                      (m_config.ldc_strength.second < 0.0))) {
        ERROR("Invalid LDC strength: %f!", m_config.ldc_strength.second);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    }
    if (AM_LIKELY(config["pano_hfov_degree"].exists())) {
      m_config.pano_hfov_degree.first = true;
      m_config.pano_hfov_degree.second =
          config["pano_hfov_degree"].get<float>(0);
      if (AM_UNLIKELY((m_config.pano_hfov_degree.second > 180.0) ||
                      (m_config.pano_hfov_degree.second < 1.0))) {
        ERROR("Invalid Pano hfvo degree: %f!",
              m_config.pano_hfov_degree.second);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    }

    if (AM_LIKELY(config["lens"].exists())) {
      if (AM_LIKELY(config["lens"]["lut"].exists())) {
        m_config.lens.second.lut_file =
            config["lens"]["lut"].get<std::string>("");
      }

      if (AM_LIKELY(config["lens"]["efl_mm"].exists())) {
        m_config.lens.second.efl_mm =
            config["lens"]["efl_mm"].get<float>(2.1);
      }

      if (AM_LIKELY(config["lens"]["pitch"].exists())) {
        m_config.lens.second.pitch = config["lens"]["pitch"].get<int>(0);
      }

      if (AM_LIKELY(config["lens"]["yaw"].exists())) {
        m_config.lens.second.yaw = config["lens"]["yaw"].get<int>(0);
      }

      if (AM_LIKELY(config["lens"]["hor_zoom"].exists())) {
        m_config.lens.second.hor_zoom.num =
            config["lens"]["hor_zoom"]["num"].get<int>(1);
        m_config.lens.second.hor_zoom.denom =
            config["lens"]["hor_zoom"]["den"].get<int>(1);
      }
      if (AM_LIKELY(config["lens"]["ver_zoom"].exists())) {
        m_config.lens.second.ver_zoom.num =
            config["lens"]["ver_zoom"]["num"].get<int>(1);
        m_config.lens.second.ver_zoom.denom =
            config["lens"]["ver_zoom"]["den"].get<int>(1);
      }

      if (AM_LIKELY(config["lens"]["lens_center_in_max_input"].exists())) {
        m_config.lens.second.lens_center_in_max_input.x =
            config["lens"]["lens_center_in_max_input"]["x"].get<int>(0);
        m_config.lens.second.lens_center_in_max_input.y =
            config["lens"]["lens_center_in_max_input"]["y"].get<int>(0);
      }
    } else {
      WARN("Lens configuration doesn't exist!");
    }
  } while (0);

  delete config_ptr;
  return result;
}

AM_RESULT AMWarpConfigS2L::save_config()
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  const char *default_dir = WARP_CONF_DIR;
  AMConfig *config_ptr = nullptr;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + WARP_CONFIG_FILE;
    if (AM_UNLIKELY(!(config_ptr = AMConfig::create(tmp.c_str())))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig& config = *config_ptr;
    config["proj_mode"] = (int)m_config.proj_mode.second;
    config["warp_mode"] = (int)m_config.warp_mode.second;
    config["max_radius"] = (int)m_config.max_radius.second;
    config["ldc_strength"] = m_config.ldc_strength.second;
    config["pano_hfov_degree"] = m_config.pano_hfov_degree.second;
    config["lens"]["hor_zoom"]["num"] = m_config.lens.second.hor_zoom.num;
    config["lens"]["hor_zoom"]["den"] = m_config.lens.second.hor_zoom.denom;
    config["lens"]["ver_zoom"]["num"] = m_config.lens.second.ver_zoom.num;
    config["lens"]["ver_zoom"]["den"] = m_config.lens.second.ver_zoom.denom;
    if (AM_UNLIKELY(!config_ptr->save())) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMWarpConfigS2L::get_config(AMWarpConfigS2LParam &config)
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMWarpConfigS2L::set_config(const AMWarpConfigS2LParam &config)
{
  AUTO_LOCK_WARP(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_config = config;
  } while (0);
  return result;
}

AMWarpConfigS2LPtr AMWarpConfigS2L::get_instance()
{
  AUTO_LOCK_WARP(m_mtx);
  if (!m_instance) {
    m_instance = new AMWarpConfigS2L();
  }
  return m_instance;
}

void AMWarpConfigS2L::inc_ref()
{
  ++ m_ref_cnt;
}

void AMWarpConfigS2L::release()
{
  AUTO_LOCK_WARP(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}
