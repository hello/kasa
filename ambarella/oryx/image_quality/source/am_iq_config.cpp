/*******************************************************************************
 * am_iq_config.cpp
 *
 * History:
 *   Dec 29, 2014 - [binwang] created file
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
#include "am_iq_param.h"
#include "am_configure.h"
#include "am_image_quality.h"
#include <string>   //C++ string

#define IQ_CONFIG_FILE "iq_config.acs"

#ifdef ORYX_IQ_CONF_DIR
#define ORYX_IQ_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR)
#else
#define ORYX_IQ_CONF_DIR ((const char*)"/etc/oryx")
#endif

#define ORYX_IQ_CONF_SUB_DIR ((const char*)"/image/")

AMIQConfig::AMIQConfig() :
    m_changed(false),
    m_loaded(
    { AM_ERROR_LEVEL,
    { AM_AE_CENTER_METERING, AM_DAY_MODE, AM_SLOW_SHUTTER_ON,
      AM_ANTI_FLICKER_60HZ, 100, AM_BACKLIGHT_COMP_OFF, 0,
      AM_DC_IRIS_DISABLE, 0, 0, 0, AM_IRLED_AUTO, 0, 0, 0, AM_AE_ENABLE },
      { AM_WB_AUTO },
      { 0 },
      { 0, 0, 0, 0, 0 } })
{
  m_using = m_loaded;
  for (uint32_t i = 0; i < AM_FILE_TYPE_TOTAL_NUM; i ++) {
    m_aeb_adj_file_flag[i] = 0;
  }
}

AMIQConfig::~AMIQConfig()
{
}

bool AMIQConfig::load_config()
{
  bool result = true;
  AMIQParam iq_param;
  AMConfig *config = nullptr;
  std::string config_file_name;
  std::string aeb_fn;
  std::string adj_fn;

  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir) {
      oryx_conf_dir = (char*)ORYX_IQ_CONF_DIR;
    }
    tmp = std::string(oryx_conf_dir).append(ORYX_IQ_CONF_SUB_DIR) + std::string(IQ_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMIQConfig: fail to create IQ AMConfig\n");
      result = false;
      break;
    }

    AMConfig& iq_config = *config;
    memset(&iq_param, 0, sizeof(iq_param));

    /*AE configure*/
    config_file_name = iq_config["name"].get<std::string>("");
    if (iq_config["log_level"].exists()) {
      iq_param.log_level = (AM_IQ_LOG_LEVEL) iq_config["log_level"].get<int>(0);
    }

    if (iq_config["aeb_bin_file_name"].exists()) {
      aeb_fn = iq_config["aeb_bin_file_name"].get<std::string>("");
      strcpy(m_aeb_adj_file_name[AM_FILE_TYPE_AEB], aeb_fn.c_str());
      if (strcmp(m_aeb_adj_file_name[AM_FILE_TYPE_AEB], "") > 0) {
        m_aeb_adj_file_flag[AM_FILE_TYPE_AEB] = 1;
      }
    }
    if (iq_config["adj_bin_file_name"].exists()) {
      adj_fn = iq_config["adj_bin_file_name"].get<std::string>("");
      strcpy(m_aeb_adj_file_name[AM_FILE_TYPE_ADJ], adj_fn.c_str());
      if (strcmp(m_aeb_adj_file_name[AM_FILE_TYPE_ADJ], "") > 0) {
        m_aeb_adj_file_flag[AM_FILE_TYPE_ADJ] = 1;
      }
    }

    if (iq_config["notify_3A_to_media_svc"].exists()) {
      iq_param.notify_3A_to_media_svc =
        iq_config["notify_3A_to_media_svc"].get<bool>(false);
    }

    if (iq_config["ae"]["anti_flicker_mode"].exists()) {
      iq_param.ae.anti_flicker_mode =
          (AM_ANTI_FLICK_MODE) iq_config["ae"]["anti_flicker_mode"].get<int>(0);
    }
    if (iq_config["ae"]["sensor_shutter_min"].exists()) {
      iq_param.ae.sensor_shutter_min =
          iq_config["ae"]["sensor_shutter_min"].get<int>(0);
    }
    if (iq_config["ae"]["sensor_shutter_max"].exists()) {
      iq_param.ae.sensor_shutter_max =
          iq_config["ae"]["sensor_shutter_max"].get<int>(0);
    }
    if (iq_config["ae"]["sensor_gain_max"].exists()) {
      iq_param.ae.sensor_gain_max =
          iq_config["ae"]["sensor_gain_max"].get<int>(0);
    }
    if (iq_config["ae"]["slow_shutter_mode"].exists()) {
      iq_param.ae.slow_shutter_mode =
          (AM_SLOW_SHUTTER_MODE) iq_config["ae"]["slow_shutter_mode"].get<int>(1);
    }
    if (iq_config["ae"]["ir_led_mode"].exists()) {
      iq_param.ae.ir_led_mode =
          (AM_IR_LED_MODE) iq_config["ae"]["ir_led_mode"].get<int>(2);
    }
    if (iq_config["ae"]["ae_metering_mode"].exists()) {
      iq_param.ae.ae_metering_mode =
          (AM_AE_METERING_MODE) iq_config["ae"]["ae_metering_mode"].get<int>(0);
    }
    if (iq_config["ae"]["ae_metering_table"].exists()) {
      for (int32_t i = 0; i < AE_METERING_TABLE_LEN; i++) {
        iq_param.ae.ae_metering_table.metering_weight[i] =
            iq_config["ae"]["ae_metering_table"][i].get<int>(0);
      }
    }
    if (iq_config["ae"]["day_night_mode"].exists()) {
      iq_param.ae.day_night_mode =
          (AM_DAY_NIGHT_MODE) iq_config["ae"]["day_night_mode"].get<int>(0);
    }
    if (iq_config["ae"]["ae_target_ratio"].exists()) {
      iq_param.ae.ae_target_ratio =
          iq_config["ae"]["ae_target_ratio"].get<int>(100);
    }
    if (iq_config["ae"]["backlight_comp_enable"].exists()) {
      iq_param.ae.backlight_comp_enable =
          (AM_BACKLIGHT_COMP_MODE) iq_config["ae"]["backlight_comp_enable"].get<
              int>(0);
    }
    if (iq_config["ae"]["local_exposure"].exists()) {
      iq_param.ae.local_exposure =
          iq_config["ae"]["local_exposure"].get<int>(0);
    }
    if (iq_config["ae"]["dc_iris_enable"].exists()) {
      iq_param.ae.dc_iris_enable =
          (AM_DC_IRIS_MODE) iq_config["ae"]["dc_iris_enable"].get<int>(0);
    }
    if (iq_config["ae"]["ae_enable"].exists()) {
      iq_param.ae.ae_enable =
          (AM_AE_MODE) iq_config["ae"]["ae_enable"].get<int>(0);
    }

    /*AWB configure*/
    if (iq_config["awb"]["wb_mode"].exists()) {
      iq_param.awb.wb_mode =
          (AM_WB_MODE) iq_config["awb"]["wb_mode"].get<int>(0);
    }

    /*Noise Filter(NF) configure*/
    if (iq_config["nf"]["mctf_strength"].exists()) {
      iq_param.nf.mctf_strength = iq_config["nf"]["mctf_strength"].get<int>(6);
    }

    /*IQ style configure*/
    if (iq_config["style"]["saturation"].exists()) {
      iq_param.style.saturation = iq_config["style"]["saturation"].get<int>(64);
    }
    if (iq_config["style"]["brightness"].exists()) {
      iq_param.style.brightness = iq_config["style"]["brightness"].get<int>(0);
    }
    if (iq_config["style"]["hue"].exists()) {
      iq_param.style.hue = iq_config["style"]["hue"].get<int>(0);
    }
    if (iq_config["style"]["contrast"].exists()) {
      iq_param.style.contrast = iq_config["style"]["contrast"].get<int>(64);
    }
    if (iq_config["style"]["sharpness"].exists()) {
      iq_param.style.sharpness = iq_config["style"]["sharpness"].get<int>(6);
    }
    if (iq_config["style"]["auto_contrast_mode"].exists()) {
      iq_param.style.auto_contrast_mode = iq_config["style"]["auto_contrast_mode"].get<int>(0);
    }

    m_loaded = iq_param;
    m_changed = !!memcmp(&m_using, &m_loaded, sizeof(m_using));

  } while (0);

  delete config;
  return result;
}

bool AMIQConfig::save_config()
{
  bool result = true;
  AMConfig *config = nullptr;
  std::string config_file_name;

  do {
    std::string tmp;
    INFO("AMIQConfig: save config(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir) {
      oryx_conf_dir = (char*)ORYX_IQ_CONF_DIR;
    }
    tmp = std::string(oryx_conf_dir).append(ORYX_IQ_CONF_SUB_DIR) + std::string(IQ_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMIQConfig: fail to open IQ AMConfig\n");
      result = false;
      break;
    }

    AMConfig& iq_config = *config;
    AMIQParam *iq_param = &m_using;
    iq_config["log_level"] = (int32_t) iq_param->log_level;

    iq_config["ae"]["anti_flicker_mode"] =
        (int32_t) iq_param->ae.anti_flicker_mode;
    iq_config["ae"]["sensor_shutter_min"] = iq_param->ae.sensor_shutter_min;
    iq_config["ae"]["sensor_shutter_max"] = iq_param->ae.sensor_shutter_max;
    iq_config["ae"]["sensor_gain_max"] = iq_param->ae.sensor_gain_max;
    iq_config["ae"]["slow_shutter_mode"] =
        (int32_t) iq_param->ae.slow_shutter_mode;
    iq_config["ae"]["ir_led_mode"] = (int32_t) iq_param->ae.ir_led_mode;
    iq_config["ae"]["ae_metering_mode"] =
        (int32_t) iq_param->ae.ae_metering_mode;
    iq_config["ae"]["day_night_mode"] = (int32_t) iq_param->ae.day_night_mode;
    iq_config["ae"]["ae_target_ratio"] = iq_param->ae.ae_target_ratio;
    iq_config["ae"]["backlight_comp_enable"] =
        (int32_t) iq_param->ae.backlight_comp_enable;
    iq_config["ae"]["local_exposure"] = (int32_t) iq_param->ae.local_exposure;
    iq_config["ae"]["dc_iris_enable"] = (int32_t) iq_param->ae.dc_iris_enable;
    iq_config["ae"]["ae_enable"] = (int32_t) iq_param->ae.ae_enable;

    iq_config["awb"]["wb_mode"] = (int32_t) iq_param->awb.wb_mode;

    iq_config["nf"]["mctf_strength"] = iq_param->nf.mctf_strength;

    iq_config["style"]["saturation"] = iq_param->style.saturation;
    iq_config["style"]["brightness"] = iq_param->style.brightness;
    iq_config["style"]["hue"] = iq_param->style.hue;
    iq_config["style"]["contrast"] = iq_param->style.contrast;
    iq_config["style"]["sharpness"] = iq_param->style.sharpness;
    iq_config["style"]["auto_contrast_mode"] = iq_param->style.auto_contrast_mode;

    if (!iq_config.save()) {
      ERROR("AMIQConfig::failed to save_config\n");
      result = false;
      break;
    }
  } while (0);

  delete config;
  return result;
}

bool AMIQConfig::sync()
{
  m_using = m_loaded;
  m_changed = false;
  return true;
}
