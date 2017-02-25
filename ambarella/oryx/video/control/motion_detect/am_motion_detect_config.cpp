/*******************************************************************************
 * am_motion_detect_config.cpp
 *
 * History:
 *   May 3, 2016 - [binwang] created file
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
#include "am_motion_detect_types.h"
#include "am_motion_detect_config.h"

AMMotionDetectConfig *AMMotionDetectConfig::m_instance = nullptr;
std::recursive_mutex AMMotionDetectConfig::m_mtx;
#define AUTO_LOCK_MD_CFG(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMMotionDetectConfig::AMMotionDetectConfig() :
     m_ref_cnt(0)
{
}

AMMotionDetectConfig::~AMMotionDetectConfig()
{
}

AM_RESULT AMMotionDetectConfig::load_config()
{
  AUTO_LOCK_MD_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  uint32_t i;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = MD_CONF_DIR;

  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + MD_CONFIG_FILE;
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMMotionDetectConfig::Create AMConfig failed!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig &md_config = *config_ptr;

    if (md_config["md_enable"].exists()) {
      m_md_config.enable = md_config["md_enable"].get<bool>(true);
    } else {
      m_md_config.enable = true;
    }

    if (md_config["md_source_buffer_id"].exists()) {
      m_md_config.buf.buf_id =
        (AM_SOURCE_BUFFER_ID)md_config["md_source_buffer_id"].get<uint32_t>(0);
    } else {
      m_md_config.buf.buf_id = AM_SOURCE_BUFFER_2ND;
    }

    if (md_config["md_source_buffer_type"].exists()) {
      std::string tmp;
      tmp = md_config["md_source_buffer_type"].get<std::string>("");
      m_md_config.buf.buf_type = buf_type_str_to_enum(tmp);
    } else {
      m_md_config.buf.buf_type = AM_DATA_FRAME_TYPE_ME1;
    }

    for (i = 0; i < MAX_ROI_NUM; i ++) {
      m_md_config.roi_info[i].roi_id = i;
      m_md_config.th[i].roi_id = i;
      m_md_config.lc_delay[i].roi_id = i;

      if (md_config["roi_config"][i]["valid"].exists()) {
        m_md_config.roi_info[i].valid =
            md_config["roi_config"][i]["valid"].get<bool>(false);
      } else {
        m_md_config.roi_info[i].valid = false;
      }
      if (md_config["roi_config"][i]["left"].exists()) {
        m_md_config.roi_info[i].left =
            md_config["roi_config"][i]["left"].get<uint32_t>(0);
      } else {
        m_md_config.roi_info[i].left = 0;
      }
      if (md_config["roi_config"][i]["right"].exists()) {
        m_md_config.roi_info[i].right =
                        md_config["roi_config"][i]["right"].get<uint32_t>(0);
      } else {
        m_md_config.roi_info[i].right = 0;
      }
      if (md_config["roi_config"][i]["top"].exists()) {
        m_md_config.roi_info[i].top =
            md_config["roi_config"][i]["top"].get<uint32_t>(0);
      } else {
        m_md_config.roi_info[i].top = 0;
      }
      if (md_config["roi_config"][i]["bottom"].exists()) {
        m_md_config.roi_info[i].bottom =
                        md_config["roi_config"][i]["bottom"].get<uint32_t>(0);
      } else {
        m_md_config.roi_info[i].bottom = 0;
      }
      if (md_config["roi_config"][i]["th1"].exists()) {
        m_md_config.th[i].threshold[0] =
            md_config["roi_config"][i]["th1"].get<uint32_t>(100);
      } else {
        m_md_config.th[i].threshold[0] = 100;
      }
      if (md_config["roi_config"][i]["th2"].exists()) {
        m_md_config.th[i].threshold[1] =
            md_config["roi_config"][i]["th2"].get<uint32_t>(500);
      } else {
        m_md_config.th[i].threshold[1] = 500;
      }
      if (md_config["roi_config"][i]["ml0c_delay"].exists()) {
        m_md_config.lc_delay[i].mt_level_change_delay[0] =
            md_config["roi_config"][i]["ml0c_delay"].get<uint32_t>(30);
      } else {
        m_md_config.lc_delay[i].mt_level_change_delay[0] = 30;
      }
      if (md_config["roi_config"][i]["ml1c_delay"].exists()) {
        m_md_config.lc_delay[i].mt_level_change_delay[1] =
            md_config["roi_config"][i]["ml1c_delay"].get<uint32_t>(6);
      } else {
        m_md_config.lc_delay[i].mt_level_change_delay[1] = 6;
      }
    }
  } while (0);

  delete config_ptr;
  return result;
}

AM_RESULT AMMotionDetectConfig::save_config()
{
  AUTO_LOCK_MD_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  uint32_t i;
  AMConfig *config_ptr = nullptr;
  const char *default_dir = MD_CONF_DIR;
  do {
    std::string tmp;
    const char *oryx_config_dir = getenv("AMBARELLA_ORYX_CONFIG_DIR");
    if (AM_UNLIKELY(!oryx_config_dir)) {
      oryx_config_dir = default_dir;
    }
    tmp = std::string(oryx_config_dir) + "/" + MD_CONFIG_FILE;
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMMotionDetectConfig::Create AMConfig failed!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &md_config = *config_ptr;

    if (md_config["md_enable"].exists()) {
      md_config["md_enable"] = m_md_config.enable;
    }

    if (md_config["md_source_buffer_id"].exists()) {
      md_config["md_source_buffer_id"] = (uint32_t)m_md_config.buf.buf_id;
    }

    if (md_config["md_source_buffer_type"].exists()) {
      md_config["md_source_buffer_type"] =
          buf_type_enum_to_str(m_md_config.buf.buf_type);
    }

    for (i = 0; i < MAX_ROI_NUM; i ++) {
      m_md_config.roi_info[i].roi_id = i;
      m_md_config.th[i].roi_id = i;
      m_md_config.lc_delay[i].roi_id = i;

      if (md_config["roi_config"][i]["valid"].exists()) {
        md_config["roi_config"][i]["valid"] = m_md_config.roi_info[i].valid;
      }
      if (md_config["roi_config"][i]["left"].exists()) {
        md_config["roi_config"][i]["left"] = m_md_config.roi_info[i].left;
      }
      if (md_config["roi_config"][i]["right"].exists()) {
        md_config["roi_config"][i]["right"] = m_md_config.roi_info[i].right;
      }
      if (md_config["roi_config"][i]["top"].exists()) {
        md_config["roi_config"][i]["top"] = m_md_config.roi_info[i].top;
      }
      if (md_config["roi_config"][i]["bottom"].exists()) {
        md_config["roi_config"][i]["bottom"] = m_md_config.roi_info[i].bottom;
      }
      if (md_config["roi_config"][i]["th1"].exists()) {
        md_config["roi_config"][i]["th1"] = m_md_config.th[i].threshold[0];
      }
      if (md_config["roi_config"][i]["th2"].exists()) {
        md_config["roi_config"][i]["th2"] = m_md_config.th[i].threshold[1];
      }
      if (md_config["roi_config"][i]["ml0c_delay"].exists()) {
        md_config["roi_config"][i]["ml0c_delay"] =
                        m_md_config.lc_delay[i].mt_level_change_delay[0];
      }
      if (md_config["roi_config"][i]["ml1c_delay"].exists()) {
        md_config["roi_config"][i]["ml1c_delay"] =
                        m_md_config.lc_delay[i].mt_level_change_delay[1];
      }
    }

    if (!md_config.save()) {
      ERROR("AMMotionDetectConfig: failed to save_config\n");
      result = AM_RESULT_ERR_IO;
      break;
    }

  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMMotionDetectConfig::get_config(MotionDetectParam &config)
{
  AUTO_LOCK_MD_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    config = m_md_config;
  } while (0);
  return result;
}

AM_RESULT AMMotionDetectConfig::set_config(const MotionDetectParam &config)
{
  AUTO_LOCK_MD_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_md_config = config;
  } while (0);

  return result;
}

AMMotionDetectConfig* AMMotionDetectConfig::get_instance()
{
  AUTO_LOCK_MD_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMMotionDetectConfig();
  }

  return m_instance;
}

void AMMotionDetectConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMMotionDetectConfig::release()
{
  AUTO_LOCK_MD_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

string AMMotionDetectConfig::buf_type_enum_to_str(AM_DATA_FRAME_TYPE buf_type)
{
  std::string buf_str;
  switch (buf_type) {
    case AM_DATA_FRAME_TYPE_ME0:
      buf_str = "ME0";
      break;
    case AM_DATA_FRAME_TYPE_ME1:
      buf_str = "ME1";
      break;
    case AM_DATA_FRAME_TYPE_YUV:
      buf_str = "YUV";
      break;
    default:
      buf_str = "ME1";
      break;
  }

  return buf_str;

}

AM_DATA_FRAME_TYPE AMMotionDetectConfig::buf_type_str_to_enum (string buf_str)
{
  AM_DATA_FRAME_TYPE buf_type = AM_DATA_FRAME_TYPE_ME1;
  if (AM_LIKELY(!buf_str.empty())) {
    const char *type = buf_str.c_str();
    if (is_str_equal(type, "ME0") || is_str_equal(type, "me0")) {
      buf_type = AM_DATA_FRAME_TYPE_ME0;
    } else if (is_str_equal(type, "ME1") || is_str_equal(type, "me1")) {
      buf_type = AM_DATA_FRAME_TYPE_ME1;
    } else if (is_str_equal(type, "YUV") || is_str_equal(type, "yuv")) {
      buf_type = AM_DATA_FRAME_TYPE_YUV;
    } else {
      WARN("Invalid video motion detect buffer type, use default type ME1");
      buf_type = AM_DATA_FRAME_TYPE_YUV;
    }
  }

  return buf_type;
}
