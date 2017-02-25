/*******************************************************************************
 * am_encode_overlay_config.cpp
 *
 * History:
 *   2016-3-15 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_encode_overlay_config.h"
#include "am_configure.h"

#define OVERLAY_CONFIG_FILE         "overlay.acs"
#define DEFAULT_ORYX_CONFIG_DIR     "/etc/oryx/video/"

AMOverlayConfig *AMOverlayConfig::m_instance = nullptr;
std::recursive_mutex AMOverlayConfig::m_mtx;

AMOverlayConfigPtr AMOverlayConfig::get_instance()
{
  OL_AUTO_LOCK(m_mtx);
  if (!m_instance) {
    m_instance = new AMOverlayConfig();
  }
  return m_instance;
}

void AMOverlayConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMOverlayConfig::release()
{
  OL_AUTO_LOCK(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMOverlayConfig::AMOverlayConfig():
    m_ref_cnt(0)
{
  DEBUG("AMOverlayConfig is created!");
}

AMOverlayConfig::~AMOverlayConfig()
{
  DEBUG("AMOverlayConfig is destroyed!");
}

AM_RESULT AMOverlayConfig::get_config(AMOverlayParamMap &config)
{
  OL_AUTO_LOCK(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config.empty()) {
      m_config.clear();
    }
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMOverlayConfig::set_config(const AMOverlayParamMap &config)
{
  OL_AUTO_LOCK(m_mtx);
  m_config = config;

  return save_config();
}

AM_RESULT AMOverlayConfig::overwrite_config(const AMOverlayParamMap &config)
{
  OL_AUTO_LOCK(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(OVERLAY_CONFIG_FILE);
    AMConfig *config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMEncodeDeviceConfig: fail to open overlay AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& overlay_config = *config_ptr;
    //delete all streams config first
    for (int32_t i = overlay_config["s_config"].length()-1; i >= 0; --i) {
      overlay_config[i].remove();
    }
    if (!overlay_config.save()) {
      ERROR("Failed to save overlay config\n ");
      result = AM_RESULT_ERR_IO;
      delete config_ptr;
      break;
    }
    delete config_ptr;

    if (!m_config.empty()) {
      m_config.clear();
    }
    m_config = config;

    result = save_config();
  } while(0);

  return result;
}

AM_RESULT AMOverlayConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  do {
    char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(OVERLAY_CONFIG_FILE);
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("Failed to create Overlay AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& config = *config_ptr;
    AMOverlayConfigParam param;
    if (config["s_num_max"].exists()) {
      param.limit.s_num_max.second = config["s_num_max"].get<int32_t>(0);
      param.limit.s_num_max.first = true;
    }
    if (config["a_num_max"].exists()) {
      param.limit.a_num_max.second = config["a_num_max"].get<int32_t>(0);
      param.limit.a_num_max.first = true;
    }

    if (!config["s_config"].exists()) {
      ERROR("invalid overlay configuration!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    INFO("stream max number = %d\n",param.limit.s_num_max.second);
    INFO("stream real size = %d\n",config["s_config"].length());
    for (uint32_t i = 0, valid_stream_num = 0;
        (i < config["s_config"].length()) &&
            ((int32_t )valid_stream_num < param.limit.s_num_max.second);
        ++i) {
      /* user may insert overlay to stream 0 and stream 2,
       * so must skip stream 1 configure here */
      if (!(config["s_config"][i]["area_num"].exists()) ||
          !(config["s_config"][i]["a_config"].exists()) ) {
        INFO("Blank overlay stream parameter, skip it!!!\n");
        continue;
      }
      ++valid_stream_num;

      uint32_t area_num = 0;
      if (config["s_config"][i]["area_num"].exists()) {
        area_num = config["s_config"][i]["area_num"].get<int32_t>(0);
      }
      if (config["s_config"][i]["a_config"].exists() &&
          (area_num > config["s_config"][i]["a_config"].length())) {
        area_num = config["s_config"][i]["a_config"].length();
      }
      if (int32_t(area_num) > param.limit.a_num_max.second) {
        area_num = param.limit.a_num_max.second;
      }
      param.num.second = area_num;
      param.num.first = true;
      INFO("stream%d have %d areas\n",i, area_num);

      for (uint32_t j = 0; j < area_num; ++j) {
        AMOverlayConfigAreaParam area;

        if (config["s_config"][i]["a_config"][j]["enable"].exists()) {
          area.state.second =
              config["s_config"][i]["a_config"][j]["enable"].get<int32_t>(1);
          area.state.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["rotate"].exists()) {
          area.rotate.second =
              config["s_config"][i]["a_config"][j]["rotate"].get<int32_t>(1);
          area.rotate.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["w"].exists()) {
          area.width.second =
              config["s_config"][i]["a_config"][j]["w"].get<int32_t>(0);
          area.width.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["h"].exists()) {
          area.height.second =
              config["s_config"][i]["a_config"][j]["h"].get<int32_t>(0);
          area.height.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["x"].exists()) {
          area.offset_x.second =
              config["s_config"][i]["a_config"][j]["x"].get<int32_t>(0);
          area.offset_x.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["y"].exists()) {
          area.offset_y.second =
              config["s_config"][i]["a_config"][j]["y"].get<int32_t>(0);
          area.offset_y.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["bg_color"].exists()) {
          area.bg_color.second =
              config["s_config"][i]["a_config"][j]["bg_color"].get<uint32_t>(0);
          area.bg_color.first = true;
        }

        if (config["s_config"][i]["a_config"][j]["buf_num"].exists()) {
          area.buf_num.second =
              config["s_config"][i]["a_config"][j]["buf_num"].get<int32_t>(0);
          area.buf_num.first = true;
        }

        uint32_t data_num = 0;
        if (config["s_config"][i]["a_config"][j]["data_num"].exists()) {
          data_num =
              config["s_config"][i]["a_config"][j]["data_num"].get<int32_t>(0);
        }
        if (config["s_config"][i]["a_config"][j]["d_config"].exists() &&
            (data_num >
          config["s_config"][i]["a_config"][j]["d_config"].length())) {
          data_num =
              config["s_config"][i]["a_config"][j]["d_config"].length();
        }
        area.num.second = data_num;
        area.num.first = true;

        for (uint32_t k = 0; k < data_num; ++k) {
          AMOverlayConfigDataParam data;

          if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k]["w"].exists()) {
            data.width.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k]["w"].get<int32_t>(0);
            data.width.first = true;
          }

          if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k]["h"].exists()) {
            data.height.second = config["s_config"][i]["a_config"][j]
                                       ["d_config"][k]["h"].get<int32_t>(0);
            data.height.first = true;
          }

          if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k]["x"].exists()) {
            data.offset_x.second = config["s_config"][i]["a_config"][j]
                                         ["d_config"][k]["x"].get<int32_t>(0);
            data.offset_x.first = true;
          }

          if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k]["y"].exists()) {
            data.offset_y.second = config["s_config"][i]["a_config"][j]
                                         ["d_config"][k]["y"].get<int32_t>(0);
            data.offset_y.first = true;
          }

          if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k]["type"].exists()) {
            data.type.second = AM_OVERLAY_DATA_TYPE(
                config["s_config"][i]["a_config"][j]
                      ["d_config"][k]["type"].get<int32_t>(-1));
            data.type.first = true;
          }

          if (AM_OVERLAY_DATA_TYPE_STRING == data.type.second ||
              AM_OVERLAY_DATA_TYPE_TIME == data.type.second) {
            std::string op;
            if (AM_OVERLAY_DATA_TYPE_STRING == data.type.second) {
              op = "string";
            } else {
              op = "time";
            }
            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["spacing"].exists()) {
              data.spacing.second = config["s_config"][i]["a_config"][j]
                                          ["d_config"][k][op.c_str()]
                                          ["spacing"].get<int32_t>(0);
              data.spacing.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["en_msec"].exists()) {
              data.en_msec.second = config["s_config"][i]["a_config"][j]
                                          ["d_config"][k][op.c_str()]
                                          ["en_msec"].get<int32_t>(0);
              data.en_msec.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["format"].exists()) {
              data.format.second = config["s_config"][i]["a_config"][j]
                                          ["d_config"][k][op.c_str()]
                                          ["format"].get<int32_t>(0);
              data.format.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["is_12h"].exists()) {
              data.is_12h.second = config["s_config"][i]["a_config"][j]
                                          ["d_config"][k][op.c_str()]
                                          ["is_12h"].get<int32_t>(0);
              data.is_12h.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["str"].exists()) {
              data.str.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["str"].get<std::string>("");
              data.str.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["pre_str"].exists()) {
              data.pre_str.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["pre_str"].get<std::string>("");
              data.pre_str.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["suf_str"].exists()) {
              data.suf_str.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["suf_str"].get<std::string>("");
              data.suf_str.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["bg_color"].exists()) {
              data.bg_color.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["bg_color"].get<uint32_t>(0);
              data.bg_color.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["ol_color"].exists()) {
              data.ol_color.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["ol_color"].get<uint32_t>(0);
              data.ol_color.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_name"].exists()) {
              data.font_name.second = config["s_config"][i]["a_config"][j]
                                            ["d_config"][k][op.c_str()]
                                            ["font_name"].get<std::string>("");
              data.font_name.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_size"].exists()) {
              data.font_size.second = config["s_config"][i]["a_config"][j]
                                            ["d_config"][k][op.c_str()]
                                            ["font_size"].get<int32_t>(0);
              data.font_size.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_color"].exists()) {
              data.font_color.second = config["s_config"][i]["a_config"][j]
                                             ["d_config"][k][op.c_str()]
                                             ["font_color"].get<uint32_t>(0);
              data.font_color.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_outline_w"].exists()) {
              data.font_outwidth.second = config["s_config"][i]["a_config"][j]
                                                ["d_config"][k][op.c_str()]
                                                ["font_outline_w"].get<int32_t>(0);
              data.font_outwidth.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_ver_bold"].exists()) {
              data.font_verbold.second = config["s_config"][i]["a_config"][j]
                                               ["d_config"][k][op.c_str()]
                                               ["font_ver_bold"].get<int32_t>(0);
              data.font_verbold.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_hor_bold"].exists()) {
              data.font_horbold.second = config["s_config"][i]["a_config"][j]
                                             ["d_config"][k][op.c_str()]
                                             ["font_hor_bold"].get<int32_t>(0);
              data.font_horbold.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["font_italic"].exists()) {
              data.font_italic.second = config["s_config"][i]["a_config"][j]
                                            ["d_config"][k][op.c_str()]
                                            ["font_italic"].get<int32_t>(0);
              data.font_italic.first = true;
            }

          } else if (AM_OVERLAY_DATA_TYPE_PICTURE == data.type.second ||
                     AM_OVERLAY_DATA_TYPE_ANIMATION == data.type.second) {
            std::string op;
            if (AM_OVERLAY_DATA_TYPE_PICTURE == data.type.second) {
              op = "picture";
            } else {
              op = "animation";
            }
            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["ckey"].exists()) {
              data.color_key.second = config["s_config"][i]["a_config"][j]
                                            ["d_config"][k][op.c_str()]
                                            ["ckey"].get<uint32_t>(0);
              data.color_key.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["crange"].exists()) {
              data.color_range.second = config["s_config"][i]["a_config"][j]
                                              ["d_config"][k][op.c_str()]
                                              ["crange"].get<int32_t>(0);
              data.color_range.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["pic"].exists()) {
              data.bmp.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["pic"].get<std::string>("/usr/local/bin/"
                                      "data/Ambarella-256x128-8bit.bmp");
              data.bmp.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["bmp_num"].exists()) {
              data.bmp_num.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["bmp_num"].get<int32_t>(0);
              data.bmp_num.first = true;
            }

            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["interval"].exists()) {
              data.interval.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["interval"].get<int32_t>(0);
              data.interval.first = true;
            }
          } else if (AM_OVERLAY_DATA_TYPE_LINE == data.type.second) {
            std::string op = "line";
            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["line_color"].exists()) {
              data.line_color.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["line_color"].get<uint32_t>(0);
              data.line_color.first = true;
            }
            if (config["s_config"][i]["a_config"][j]
                    ["d_config"][k][op.c_str()]["line_thickness"].exists()) {
              data.line_tn.second = config["s_config"][i]["a_config"][j]
                                      ["d_config"][k][op.c_str()]
                                      ["line_thickness"].get<int32_t>(0);
              data.line_tn.first = true;
            }
            int32_t min = AM_MIN(config["s_config"][i]["a_config"][j]["d_config"]
                               [k][op.c_str()]["p_x"].length(),
                               config["s_config"][i]["a_config"][j]["d_config"]
                               [k][op.c_str()]["p_y"].length());
            for (int32_t n = 0; n < min; ++n) {
              data.line_p.second.push_back(
                        AMPoint(config["s_config"][i]["a_config"][j]["d_config"]
                               [k][op.c_str()]["p_x"][n],
                               config["s_config"][i]["a_config"][j]["d_config"]
                               [k][op.c_str()]["p_y"][n]));
            }
            if (data.line_p.second.size() > 0) {
              data.line_p.first = true;
            }
          }
          area.data.push_back(data);
        }

        param.area.push_back(area);
      }

      m_config[(AM_STREAM_ID)i] = param;
      param.num.second = 0;
      param.num.first = false;
      if (!(param.area.empty())) {
        param.area.clear();
      }
    }
  } while (0);

  delete config_ptr;
  return result;
}

AM_RESULT AMOverlayConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMConfig *config_ptr = nullptr;
  do {
    char default_dir[] = DEFAULT_ORYX_CONFIG_DIR;
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(OVERLAY_CONFIG_FILE);
    config_ptr = AMConfig::create(tmp.c_str());
    if (!config_ptr) {
      ERROR("AMEncodeDeviceConfig: fail to open overlay AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& config = *config_ptr;
    for (auto &m : m_config) {
      const AMOverlayConfigParam &param = m.second;

      if (param.limit.s_num_max.first) {
        config["s_num_max"] = param.limit.s_num_max.second;
      }
      if (param.limit.a_num_max.first) {
        config["a_num_max"] = param.limit.a_num_max.second;
      }

      int32_t i = m.first;
      if (param.num.first) {
        config["s_config"][i]["area_num"] = param.num.second;
      }

      for (uint32_t j = 0; j < param.area.size(); ++j) {
        const AMOverlayConfigAreaParam &area = param.area[j];
        INFO("stream: %d area :%d \n",i, j);
        if (area.state.first) {
          config["s_config"][i]["a_config"][j]["enable"] = area.state.second;
        }

        if (area.rotate.first) {
          config["s_config"][i]["a_config"][j]["rotate"] = area.rotate.second;
        }

        if (area.width.first) {
          config["s_config"][i]["a_config"][j]["w"] = area.width.second;
        }

        if (area.height.first) {
          config["s_config"][i]["a_config"][j]["h"] = area.height.second;
        }

        if (area.offset_x.first) {
          config["s_config"][i]["a_config"][j]["x"] = area.offset_x.second;
        }

        if (area.offset_y.first) {
          config["s_config"][i]["a_config"][j]["y"] = area.offset_y.second;
        }

        if (area.bg_color.first) {
          config["s_config"][i]["a_config"][j]["bg_color"] = area.bg_color.second;
        }

        if (area.buf_num.first) {
          config["s_config"][i]["a_config"][j]["buf_num"] = area.buf_num.second;
        }

        if (area.num.first) {
          config["s_config"][i]["a_config"][j]["data_num"] = area.num.second;
        }

        for (uint32_t k = 0; k < area.data.size(); ++k) {
          const AMOverlayConfigDataParam &data = area.data[k];
          INFO("stream: %d area :%d data :%d\n",i, j, k);

          if (data.width.first) {
            config["s_config"][i]["a_config"][j]["d_config"][k]["w"] =
                data.width.second;
          }

          if (data.height.first) {
            config["s_config"][i]["a_config"][j]["d_config"][k]["h"] =
                data.height.second;
          }

          if (data.offset_x.first) {
            config["s_config"][i]["a_config"][j]["d_config"][k]["x"] =
                data.offset_x.second;
          }

          if (data.offset_y.first) {
            config["s_config"][i]["a_config"][j]["d_config"][k]["y"] =
                data.offset_y.second;
          }

          if (data.type.first) {
            config["s_config"][i]["a_config"][j]["d_config"][k]["type"] =
                int32_t(data.type.second);
          }

          if (AM_OVERLAY_DATA_TYPE_STRING == data.type.second ||
              AM_OVERLAY_DATA_TYPE_TIME == data.type.second) {
            std::string op;
            if (AM_OVERLAY_DATA_TYPE_STRING == data.type.second) {
              op = "string";
            } else {
              op = "time";
            }

            if (data.str.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["str"] = data.str.second;
            }

            if (data.pre_str.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["pre_str"] = data.pre_str.second;
            }

            if (data.suf_str.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["suf_str"] = data.suf_str.second;
            }

            if (data.bg_color.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["bg_color"] = data.bg_color.second;
            }

            if (data.ol_color.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["ol_color"] = data.ol_color.second;
            }

            if (data.spacing.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["spacing"] = data.spacing.second;
            }

            if (data.en_msec.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["en_msec"] = data.en_msec.second;
            }

            if (data.format.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["format"] = data.format.second;
            }

            if (data.is_12h.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["is_12h"] = data.is_12h.second;
            }

            if (data.font_name.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_name"] = data.font_name.second;
            }

            if (data.font_size.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_size"] = data.font_size.second;
            }

            if (data.font_outwidth.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_outline_w"] = data.font_outwidth.second;
            }

            if (data.font_horbold.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_hor_bold"] = data.font_horbold.second;
            }

            if (data.font_verbold.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_ver_bold"] = data.font_verbold.second;
            }

            if (data.font_italic.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_italic"] = data.font_italic.second;
            }

            if (data.font_color.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["font_color"] = data.font_color.second;
            }
          } else if (AM_OVERLAY_DATA_TYPE_PICTURE == data.type.second ||
                     AM_OVERLAY_DATA_TYPE_ANIMATION == data.type.second) {
            std::string op;
            if (AM_OVERLAY_DATA_TYPE_PICTURE == data.type.second) {
              op = "picture";
            } else {
              op = "animation";
            }
            if (data.color_key.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["ckey"] = data.color_key.second;
            }

            if (data.color_range.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["crange"] = data.color_range.second;
            }

            if (data.bmp.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["pic"] = data.bmp.second;
            }

            if (data.bmp_num.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["bmp_num"] = data.bmp_num.second;
            }

            if (data.interval.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["interval"] = data.interval.second;
            }
          } else if (AM_OVERLAY_DATA_TYPE_LINE == data.type.second) {
            std::string op = "line";
            if (data.line_color.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["line_color"] = data.line_color.second;
            }
            if (data.line_tn.first) {
              config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                    ["line_thickness"] = data.line_tn.second;
            }
            if (data.line_p.first) {
              for (uint32_t n = 0; n < data.line_p.second.size(); ++n) {
                config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                      ["p_x"][n] = data.line_p.second[n].x;
                config["s_config"][i]["a_config"][j]["d_config"][k][op.c_str()]
                      ["p_y"][n] = data.line_p.second[n].y;
              }
            }
          }
        }
      }
    }

    if (!config.save()) {
      ERROR("Failed to save overlay config\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);

  //delete config class
  delete config_ptr;
  return result;
}
