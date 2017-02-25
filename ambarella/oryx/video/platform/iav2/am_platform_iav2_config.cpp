/**
 * am_platform_iav2_config.cpp
 *
 *  History:
 *    Aug 25, 2015 - [Shupeng Ren] created file
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
#include "am_log.h"
#include "iav_ioctl.h"
#include "am_configure.h"
#include "am_platform_iav2_utility.h"
#include "am_platform_iav2_config.h"

#define LOCK_PLATFORM_CFG(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

#define DEFAULT_ORYX_CONFIG_DIR     "/etc/oryx/video/"
#define FEATURE_CONFIG_FILE         "features.acs"

AMResourceConfig *AMResourceConfig::m_instance = nullptr;
std::recursive_mutex AMResourceConfig::m_mtx;
AMFeatureConfig *AMFeatureConfig::m_instance = nullptr;
std::recursive_mutex AMFeatureConfig::m_mtx;

AMFeatureConfigPtr AMFeatureConfig::get_instance()
{
  LOCK_PLATFORM_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMFeatureConfig();
  }
  return m_instance;
}

void AMFeatureConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMFeatureConfig::release()
{
  LOCK_PLATFORM_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMFeatureConfig::AMFeatureConfig() :
     m_ref_cnt(0)
{
  DEBUG("AMFeatureConfig is created!");
}

AMFeatureConfig::~AMFeatureConfig()
{
  DEBUG("AMFeatureConfig is destroyed!");
}

AM_RESULT AMFeatureConfig::get_config(AMFeatureParam &config)
{
  LOCK_PLATFORM_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMFeatureConfig::set_config(const AMFeatureParam &config)
{
  LOCK_PLATFORM_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  do {
    m_config = config;
    if ((result = save_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMFeatureConfig::load_config()
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
    tmp = std::string(oryx_config_dir) + std::string(FEATURE_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    //clean m_config preview state
    m_config = AMFeatureParam();

    AMConfig &config = *config_ptr;
    if (config["iav_ver"].exists()) {
      m_config.version.second = config["iav_ver"].get<int>(-1);
      m_config.version.first = true;
    }
    if (!m_config.version.first || (m_config.version.second < 0)) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid IAV version: %d, please check!", m_config.version.second);
      break;
    }

    if (config["mode"].exists()) {
      tmp = config["mode"].get<std::string>("");
      m_config.mode.second =
          AMVideoTransIav2::camera_param_mode_str_to_enum(tmp);
      m_config.mode.first = true;
    }

    if (config["hdr"].exists()) {
      tmp = config["hdr"].get<std::string>("");
      m_config.hdr.second = AMVideoTransIav2::camera_param_hdr_str_to_enum(tmp);
      m_config.hdr.first = true;
    }

    if (config["iso"].exists()) {
      tmp = config["iso"].get<std::string>("");
      m_config.iso.second = AMVideoTransIav2::camera_param_iso_str_to_enum(tmp);
      m_config.iso.first = true;
    }

    if (config["dewarp_func"].exists()) {
      tmp = config["dewarp_func"].get<std::string>("");
      m_config.dewarp_func.second =
          AMVideoTransIav2::camera_param_dewarp_str_to_enum(tmp);
      m_config.dewarp_func.first = true;
    }

    if (config["dptz"].exists()) {
      tmp = config["dptz"].get<std::string>("");
      m_config.dptz.second = AMVideoTransIav2::camera_param_dptz_str_to_enum(tmp);
      m_config.dptz.first = true;
    }

    if (config["bitrate_ctrl"].exists()) {
      tmp = config["bitrate_ctrl"].get<std::string>("none");
      m_config.bitrate_ctrl.second =
          AMVideoTransIav2::bitrate_ctrl_method_str_to_enum(tmp);
      m_config.bitrate_ctrl.first = true;
    }

    if (config["overlay"].exists()) {
      tmp = config["overlay"].get<std::string>("none");
      m_config.overlay.second =
          AMVideoTransIav2::camera_param_overlay_str_to_enum(tmp);
      m_config.overlay.first = true;
    }

    if (config["video_md"].exists()) {
      tmp = config["video_md"].get<std::string>("none");
      m_config.video_md.second =
          AMVideoTransIav2::camera_param_md_str_to_enum(tmp);
      m_config.video_md.first = true;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMFeatureConfig::save_config()
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
    tmp = std::string(oryx_config_dir) + std::string(FEATURE_CONFIG_FILE);
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMConfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    if (m_config.mode.first) {
      config["mode"] =
         AMVideoTransIav2::camera_param_mode_enum_to_str(m_config.mode.second);
    }
    if (m_config.hdr.first) {
      config["hdr"] =
           AMVideoTransIav2::camera_param_hdr_enum_to_str(m_config.hdr.second);
    }
    if (m_config.iso.first) {
      config["iso"] =
           AMVideoTransIav2::camera_param_iso_enum_to_str(m_config.iso.second);
    }
    if (m_config.dewarp_func.first) {
      config["dewarp_func"] = AMVideoTransIav2::
                  camera_param_dewarp_enum_to_str(m_config.dewarp_func.second);
    }
    if (m_config.dptz.first) {
      config["dptz"] = AMVideoTransIav2::
                camera_param_dptz_enum_to_str(m_config.dptz.second);
    }
    if (m_config.bitrate_ctrl.first) {
      config["bitrate_ctrl"] = AMVideoTransIav2::
                 bitrate_ctrl_method_enum_to_str(m_config.bitrate_ctrl.second);
    }
    if (m_config.overlay.first) {
      config["overlay"] = AMVideoTransIav2::
                camera_param_overlay_enum_to_str(m_config.overlay.second);
    }
    if (m_config.video_md.first) {
      config["video_md"] = AMVideoTransIav2::
                 camera_param_md_enum_to_str(m_config.video_md.second);
    }
    /*
    iav_ver in features.acs is read-only, should not be changed.
    if (m_config.version.first) {
      config["iav_ver"] = (int)m_config.version.second;
    }
    */

    if (!config.save()) {
      ERROR("Failed to save config: %s", tmp.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);

  return result;
}

AMResourceConfigPtr AMResourceConfig::get_instance()
{
  LOCK_PLATFORM_CFG(m_mtx);
  if (!m_instance) {
    m_instance = new AMResourceConfig();
  }
  return m_instance;
}

void AMResourceConfig::inc_ref()
{
  ++m_ref_cnt;
}

void AMResourceConfig::release()
{
  LOCK_PLATFORM_CFG(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMResourceConfig::AMResourceConfig() :
    m_ref_cnt(0)
{
  DEBUG("AMResourceConfig is created!");
}

AMResourceConfig::~AMResourceConfig()
{
  DEBUG("AMResourceConfig is destroyed!");
}

AM_RESULT AMResourceConfig::set_config_file(const string &file)
{
  m_config_file = file;
  return AM_RESULT_OK;
}

AM_RESULT AMResourceConfig::get_config(AMResourceParam &config)
{
  LOCK_PLATFORM_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    config = m_config;
  } while (0);
  return result;
}

AM_RESULT AMResourceConfig::set_config(const AMResourceParam &config)
{
  LOCK_PLATFORM_CFG(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }
    /* todo: implement set config */
//    for (auto &m : config) {
//
//    }
    if ((result = save_config()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMResourceConfig::load_config()
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

    if (m_config_file.empty()) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Config file is not specified!");
      break;
    }

    tmp = std::string(oryx_config_dir) + m_config_file;
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }

    m_config = AMResourceParam();
    m_config.buf_max_size.second.clear();
    m_config.buf_extra_dram.second.clear();
    m_config.stream_max_size.second.clear();
    m_config.stream_max_M.second.clear();
    m_config.stream_max_N.second.clear();
    m_config.stream_max_advanced_quality_model.second.clear();
    m_config.stream_long_ref_possible.second.clear();

    tmp = std::string(oryx_config_dir) + m_config_file;
    if (!(config_ptr = AMConfig::create(tmp.c_str()))) {
      ERROR("Failed to create AMconfig: %s", tmp.c_str());
      result = AM_RESULT_ERR_MEM;
      break;
    }
    AMConfig &config = *config_ptr;
    uint32_t stream_num = 0;
    uint32_t buffer_num = 0;
    if (config["max_streams"].exists()) {
      m_config.max_num_encode.second = config["max_streams"].get<int>(0);
      m_config.max_num_encode.first = true;
      stream_num = m_config.max_num_encode.second;
      if (stream_num > IAV_STREAM_MAX_NUM_IMPL) {
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    } else {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (config["max_buffers"].exists()) {
      m_config.max_num_cap_sources.second = config["max_buffers"].get<int>(0);
      m_config.max_num_cap_sources.first = true;
      buffer_num = m_config.max_num_cap_sources.second;
      if (buffer_num > IAV_SRCBUF_NUM) {
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    } else {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (uint32_t i = 0; i < buffer_num; ++i) {
      if (config["source_buffer_max_size"][i]["width"].exists() &&
          config["source_buffer_max_size"][i]["height"].exists()) {
        AMResolution size = {0};
        size.width = config["source_buffer_max_size"][i]["width"].get<int>(0);
        size.height = config["source_buffer_max_size"][i]["height"].get<int>(0);
        m_config.buf_max_size.second.push_back(size);
        m_config.buf_max_size.first = true;
      }
      if (config["source_buffer_extra_dram"][i].exists()) {
        uint32_t extra_buf = config["source_buffer_extra_dram"][i].get<int>(0);
        m_config.buf_extra_dram.second.push_back(extra_buf);
        m_config.buf_extra_dram.first = true;
      }
    }

    for (uint32_t i = 0; i < stream_num; ++i) {
      if (config["stream_max_size"][i]["width"].exists() &&
          config["stream_max_size"][i]["height"].exists()) {
        AMResolution size = {0};
        size.width = config["stream_max_size"][i]["width"].get<int>(0);
        size.height = config["stream_max_size"][i]["height"].get<int>(0);
        m_config.stream_max_size.second.push_back(size);
        m_config.stream_max_size.first = true;
      }

      if (config["stream_max_gop_M"][i].exists()) {
        uint32_t gop_m = config["stream_max_gop_M"][i].get<int>(0);
        m_config.stream_max_M.second.push_back(gop_m);
        m_config.stream_max_M.first = true;
      }

      if (config["stream_max_gop_N"][i].exists()) {
        uint32_t gop_n = config["stream_max_gop_N"][i].get<int>(0);
        m_config.stream_max_N.second.push_back(gop_n);
        m_config.stream_max_N.first = true;
      }

      if (config["stream_max_advanced_quality_model"][i].exists()) {
        uint32_t model = config["stream_max_advanced_quality_model"][i].get<int>(0);
        m_config.stream_max_advanced_quality_model.second.push_back(model);
        m_config.stream_max_advanced_quality_model.first = true;
      }

      if (config["stream_long_ref_possible"][i].exists()) {
        bool possible = config["stream_long_ref_possible"][i].get<bool>(false);
        m_config.stream_long_ref_possible.second.push_back(possible);
        m_config.stream_long_ref_possible.first = true;
      }
    }

    if (config["dsp_partition_possible"].exists()) {
      m_config.dsp_partition_map.second = 0;
      if (config["dsp_partition_possible"]["sub_buf_raw"].exists() &&
          config["dsp_partition_possible"]["sub_buf_raw"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_RAW;
      }
      if (config["dsp_partition_possible"]["sub_buf_main_yuv"].exists() &&
          config["dsp_partition_possible"]["sub_buf_main_yuv"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_MAIN_YUV;
      }
      if (config["dsp_partition_possible"]["sub_buf_main_me1"].exists() &&
          config["dsp_partition_possible"]["sub_buf_main_me1"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_MAIN_ME1;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_A_yuv"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_A_yuv"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_A_YUV;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_A_me1"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_A_me1"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_A_ME1;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_B_yuv"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_B_yuv"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_B_YUV;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_B_me1"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_B_me1"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_B_ME1;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_C_yuv"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_C_yuv"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_C_YUV;
      }
      if (config["dsp_partition_possible"]["sub_buf_prev_C_me1"].exists() &&
          config["dsp_partition_possible"]["sub_buf_prev_C_me1"].get<bool>(false)) {
        m_config.dsp_partition_map.second |= 1 << IAV_DSP_SUB_BUF_PREV_C_ME1;
      }
      m_config.dsp_partition_map.first = true;
    }

    if (config["rotate_possible"].exists()) {
      m_config.rotate_possible.second =
          config["rotate_possible"].get<bool>(false);
      m_config.rotate_possible.first = true;
    }

    if (config["raw_capture_possible"].exists()) {
      m_config.raw_capture_possible.second =
          config["raw_capture_possible"].get<bool>(false);
      m_config.raw_capture_possible.first = true;
    }

    if (config["vout_swap_possible"].exists()) {
      m_config.vout_swap_possible.second =
          config["vout_swap_possible"].get<bool>(false);
      m_config.vout_swap_possible.first = true;
    }

    if (config["lens_warp_possible"].exists()) {
      m_config.lens_warp_possible.second =
          config["lens_warp_possible"].get<bool>(false);
      m_config.lens_warp_possible.first = true;
    }

    if (config["enc_raw_rgb_possible"].exists()) {
      m_config.enc_raw_rgb_possible.second =
          config["enc_raw_rgb_possible"].get<bool>(false);
      m_config.enc_raw_rgb_possible.first = true;
    }

    if (config["enc_raw_yuv_possible"].exists()) {
      m_config.enc_raw_yuv_possible.second =
          config["enc_raw_yuv_possible"].get<bool>(false);
      m_config.enc_raw_yuv_possible.first = true;
    }

    if (config["enc_from_mem_possible"].exists()) {
      m_config.enc_from_mem_possible.second =
          config["enc_from_mem_possible"].get<bool>(false);
      m_config.enc_from_mem_possible.first = true;
    }

    if (config["efm_buf_num"].exists()) {
      m_config.efm_buf_num.second =
          config["efm_buf_num"].get<int>(5);
      m_config.efm_buf_num.first = true;
    }

    if (config["mixer_a_enable"].exists()) {
      m_config.mixer_a_enable.second =
          config["mixer_a_enable"].get<bool>(false);
      m_config.mixer_a_enable.first = true;
    }

    if (config["mixer_b_enable"].exists()) {
      m_config.mixer_b_enable.second =
          config["mixer_b_enable"].get<bool>(false);
      m_config.mixer_b_enable.first = true;
    }

    if (config["me0_scale"].exists()) {
      m_config.me0_scale.second = config["me0_scale"].get<int>(0);
      m_config.me0_scale.first = true;
    }

    if (config["raw_max_width"].exists() &&
        config["raw_max_height"].exists()) {
      m_config.raw_max_size.second.width = config["raw_max_width"].get<int>(0);
      m_config.raw_max_size.second.height = config["raw_max_height"].get<int>(0);
      m_config.raw_max_size.first = true;
    }

    if (config["max_warp_input_width"].exists() &&
        config["max_warp_input_height"].exists()) {
      m_config.max_warp_input_size.second.width =
          config["max_warp_input_width"].get<int>(0);
      m_config.max_warp_input_size.second.height =
          config["max_warp_input_height"].get<int>(0);
      m_config.max_warp_input_size.first = true;
    }

    if (config["max_warp_output_width"].exists()) {
      m_config.max_warp_output_width.second =
          config["max_warp_output_width"].get<bool>(0);
      m_config.max_warp_output_width.first = true;
    }

    if (config["max_padding_width"].exists()) {
      m_config.max_padding_width.second = config["max_padding_width"].get<int>(0);
      m_config.max_padding_width.first = true;
    }

    if (config["v_warped_main_max_width"].exists() &&
        config["v_warped_main_max_height"].exists()) {
      m_config.v_warped_main_max_size.second.width =
          config["v_warped_main_max_width"].get<int>(0);
      m_config.v_warped_main_max_size.second.height =
          config["v_warped_main_max_height"].get<int>(0);
      m_config.v_warped_main_max_size.first = true;
    }

    if (config["enc_dummy_latency"].exists()) {
      m_config.enc_dummy_latency.second = config["enc_dummy_latency"].get<int>(0);
      m_config.enc_dummy_latency.first = true;
    }

    if (config["idsp_upsample_type"].exists()) {
      m_config.idsp_upsample_type.second = config["idsp_upsample_type"].get<int>(0);
      m_config.idsp_upsample_type.first = true;
    }

    if (config["eis_delay_count"].exists()) {
      m_config.eis_delay_count.second = config["eis_delay_count"].get<int>(2);
      m_config.eis_delay_count.first = true;
    }

    if (config["debug_iso_type"].exists()) {
      m_config.debug_iso_type.second = config["debug_iso_type"].get<int>(-1);
      m_config.debug_iso_type.first = true;
    }

    if (config["debug_stitched"].exists()) {
      m_config.debug_stitched.second = config["debug_stitched"].get<int>(-1);
      m_config.debug_stitched.first = true;
    }

    if (config["debug_chip_id"].exists()) {
      m_config.debug_chip_id.second = config["debug_chip_id"].get<int>(-1);
      m_config.debug_chip_id.first = true;
    }
  } while (0);
  delete config_ptr;
  return result;
}

AM_RESULT AMResourceConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    /* todo: implement save_config */
  } while (0);

  return result;
}
