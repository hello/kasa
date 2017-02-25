/**
 * am_rest_api_image.cpp
 *
 *  History:
 *		2015/08/19 - [Huaiqing Wang] created file
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
#include <string.h>

#include "am_rest_api_image.h"
#include "am_api_image.h"
#include "am_define.h"

using std::string;

AM_REST_RESULT AMRestAPIImage::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, "no media cmp group parameter");
      break;
    }
    if ("style" == cmd_group) {
      ret = image_style_handle();
    } else if ("denoise" == cmd_group) {
      ret = image_denoise_handle();
    } else if ("ae" == cmd_group) {
      ret = image_ae_handle();
    } else if ("awb" == cmd_group) {
      ret = image_awb_handle();
    } else if ("af" == cmd_group) {
      ret = image_af_handle();
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image cmp group: %s", cmd_group.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::image_style_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    m_utils->get_value("action", action);

    if ("set" == action) {
      ret = style_set_handle();
    } else if ("get" == action) {
      ret = style_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image style action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_STYLE_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::style_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_image_style_config_s image_style_cfg = {0};
    string value;

    if (m_utils->get_value("hue", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<-2 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "hue value: %d is out of range: [-2, 2]",tmp);
        m_utils->set_response_msg(AM_REST_STYLE_HUE_ERR, msg);
        break;
      }
      SET_BIT(image_style_cfg.enable_bits, AM_IMAGE_STYLE_CONFIG_HUE);
      image_style_cfg.hue = tmp;
    }
    if (m_utils->get_value("saturation", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<-2 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "saturation value: %d is out of range: [-2, 2]",tmp);
        m_utils->set_response_msg(AM_REST_STYLE_SATURATION_ERR, msg);
        break;
      }
      SET_BIT(image_style_cfg.enable_bits, AM_IMAGE_STYLE_CONFIG_SATURATION);
      image_style_cfg.saturation = tmp;
    }
    if (m_utils->get_value("sharpness", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<-2 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "sharpness value: %d is out of range: [-2, 2]",tmp);
        m_utils->set_response_msg(AM_REST_STYLE_SHARPNESS_ERR, msg);
        break;
      }
      SET_BIT(image_style_cfg.enable_bits, AM_IMAGE_STYLE_CONFIG_SHARPNESS);
      image_style_cfg.sharpness = tmp;
    }
    if (m_utils->get_value("brightness", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<-2 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "brightness value: %d is out of range: [-2, 2]",tmp);
        m_utils->set_response_msg(AM_REST_STYLE_BRIGHTNESS_ERR, msg);
        break;
      }
      SET_BIT(image_style_cfg.enable_bits, AM_IMAGE_STYLE_CONFIG_BRIGHTNESS);
      image_style_cfg.brightness = tmp;
    }
    if (m_utils->get_value("contrast", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<-2 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "contrast value: %d is out of range: [-2, 2]",tmp);
        m_utils->set_response_msg(AM_REST_STYLE_CONTRAST_ERR, msg);
        break;
      }
      SET_BIT(image_style_cfg.enable_bits, AM_IMAGE_STYLE_CONFIG_CONTRAST);
      image_style_cfg.contrast = tmp;
    }

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET,
                &image_style_cfg, sizeof(image_style_cfg),
                &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_STYLE_HANDLE_ERR, "server set image style cfg failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::style_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_image_style_config_s cfg = {0};

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_STYLE_HANDLE_ERR, "server get image style cfg failed");
      break;
    }
    memcpy(&cfg, service_result.data, sizeof(am_image_style_config_s));
    m_utils->set_response_data("hue", cfg.hue);
    m_utils->set_response_data("saturation", cfg.saturation);
    m_utils->set_response_data("brightness", cfg.brightness);
    m_utils->set_response_data("sharpness", cfg.sharpness);
    m_utils->set_response_data("contrast", cfg.contrast);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::image_denoise_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    m_utils->get_value("action", action);

    if ("set" == action) {
      ret = denoise_set_handle();
    } else if ("get" == action) {
      ret = denoise_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image denoise action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_DENOISE_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::denoise_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_noise_filter_config_s noise_filter_cfg = {0};
    string value;

    if (m_utils->get_value("mctf_strength", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>11) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "mctf strength value: %d is out of range: [0, 11]",tmp);
        m_utils->set_response_msg(AM_REST_DENOISE_MCTF_STRENGTH_ERR, msg);
        break;
      }
      SET_BIT(noise_filter_cfg.enable_bits, AM_NOISE_FILTER_CONFIG_MCTF_STRENGTH);
      noise_filter_cfg.mctf_strength = tmp;
    }

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET,
                &noise_filter_cfg, sizeof(noise_filter_cfg),
                &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DENOISE_HANDLE_ERR, "server set image denoise cfg failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::denoise_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_noise_filter_config_s cfg = {0};

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DENOISE_HANDLE_ERR, "server get image denoise cfg failed");
      break;
    }
    memcpy(&cfg, service_result.data, sizeof(am_noise_filter_config_s));
    m_utils->set_response_data("mctf_strength", (int32_t)cfg.mctf_strength);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::image_ae_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    m_utils->get_value("action", action);

    if ("set" == action) {
      ret = ae_set_handle();
    } else if ("get" == action) {
      ret = ae_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image AE action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_AE_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::ae_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_ae_config_s ae_cfg = {0};
    string value;

    if (m_utils->get_value("ae_metering_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>3) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ae_metering_mode value: %d is out of range: [0, 3]",tmp);
        m_utils->set_response_msg(AM_REST_AE_METERING_MODE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_AE_METERING_MODE);
      ae_cfg.ae_metering_mode = tmp;
    }
    if (m_utils->get_value("day_night_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "day_night_mode value: %d is out of range: [0, 1]",tmp);
        m_utils->set_response_msg(AM_REST_AE_DAY_NIGHT_MODE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_DAY_NIGHT_MODE);
      ae_cfg.day_night_mode = tmp;
    }
    if (m_utils->get_value("slow_shutter_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "slow_shutter_mode value: %d is out of range: [0, 1]",tmp);
        m_utils->set_response_msg(AM_REST_AE_SLOW_SHUTTER_MODE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SLOW_SHUTTER_MODE);
      ae_cfg.slow_shutter_mode = tmp;
    }
    if (m_utils->get_value("anti_flicker_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "anti_flicker_mode value: %d is out of range: [0, 1]",tmp);
        m_utils->set_response_msg(AM_REST_AE_ANTI_FLICKER_MODE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_ANTI_FLICKER_MODE);
      ae_cfg.anti_flicker_mode = tmp;
    }
    if (m_utils->get_value("ae_target_ratio", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<25 || tmp>400) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ae_target_ratio value: %d is out of range: [25, 400]",tmp);
        m_utils->set_response_msg(AM_REST_AE_TARGET_RATIO_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_AE_TARGET_RATIO);
      ae_cfg.ae_target_ratio = tmp;
    }
    if (m_utils->get_value("back_light_comp", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "back_light_comp value: %d is out of range: [0, 1]",tmp);
        m_utils->set_response_msg(AM_REST_AE_BACK_LIGHT_COMP_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_BACKLIGHT_COMP_ENABLE);
      ae_cfg.backlight_comp_enable = tmp;
    }
    if (m_utils->get_value("local_exposure", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>3) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "local_exposure value: %d is out of range: [0, 3]",tmp);
        m_utils->set_response_msg(AM_REST_AE_LOCAL_EXPOSURE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_LOCAL_EXPOSURE);
      ae_cfg.local_exposure = tmp;
    }
    if (m_utils->get_value("dc_iris_enable", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "dc_iris_enable value: %d is out of range: [0, 1]",tmp);
        m_utils->set_response_msg(AM_REST_AE_DC_IRIS_ENABLE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_DC_IRIS_ENABLE);
      ae_cfg.dc_iris_enable = tmp;
    }
    if (m_utils->get_value("ir_led_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>2) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ir_led_mode value: %d is out of range: [0, 2]",tmp);
        m_utils->set_response_msg(AM_REST_AE_IR_LED_MODE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_IR_LED_MODE);
      ae_cfg.ir_led_mode = tmp;
    }
    if (m_utils->get_value("sensor_gain_max", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>48) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "sensor_gain_max value: %d is out of range: [0, 48]",tmp);
        m_utils->set_response_msg(AM_REST_AE_SENSOR_GAIN_MAX_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MAX);
      ae_cfg.sensor_gain_max = tmp;
    }
    if (m_utils->get_value("sensor_shutter_min", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "sensor_shutter_min value: %d is less than 1",tmp);
        m_utils->set_response_msg(AM_REST_AE_SENSOR_SHUTTER_MIN_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MIN);
      ae_cfg.sensor_shutter_min = tmp;
    }
    if (m_utils->get_value("sensor_shutter_max", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "sensor_shutter_max value: %d is less than 1",tmp);
        m_utils->set_response_msg(AM_REST_AE_SENSOR_SHUTTER_MAX_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MAX);
      ae_cfg.sensor_shutter_max = tmp;
    }
    if (m_utils->get_value("ae_enable", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ae_enable value: %d is less than 1",tmp);
        m_utils->set_response_msg(AM_REST_AE_ENABLE_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_AE_ENABLE);
      ae_cfg.ae_enable = tmp;
    }

    if (m_utils->get_value("sensor_gain_manual", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "sensor_gain_manual value: %d is less than 0",tmp);
        m_utils->set_response_msg(AM_REST_AE_SENSOR_GAIN_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MANUAL);
      ae_cfg.sensor_gain = tmp;
    }
    if (m_utils->get_value("shutter_time_manual", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<1) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "shutter_time_manual value: %d is less than 1",tmp);
        m_utils->set_response_msg(AM_REST_AE_SENSOR_SHUTTER_ERR, msg);
        break;
      }
      SET_BIT(ae_cfg.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MANUAL);
      ae_cfg.sensor_shutter = tmp;
    }

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET, &ae_cfg,
                sizeof(ae_cfg), &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AE_HANDLE_ERR, "server set image AE cfg failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::ae_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_ae_config_s cfg = {0};

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AE_HANDLE_ERR, "server get image AE cfg failed");
      break;
    }
    memcpy(&cfg, service_result.data, sizeof(am_ae_config_s));
    m_utils->set_response_data("ae_metering_mode", (int32_t)cfg.ae_metering_mode);
    m_utils->set_response_data("day_night_mode", (int32_t)cfg.day_night_mode);
    m_utils->set_response_data("slow_shutter_mode", (int32_t)cfg.slow_shutter_mode);
    m_utils->set_response_data("anti_flicker_mode", (int32_t)cfg.anti_flicker_mode);
    m_utils->set_response_data("ae_target_ratio", (int32_t)cfg.ae_target_ratio);
    m_utils->set_response_data("back_light_comp", (int32_t)cfg.backlight_comp_enable);
    m_utils->set_response_data("local_exposure", (int32_t)cfg.local_exposure);
    m_utils->set_response_data("dc_iris_enable", (int32_t)cfg.dc_iris_enable);
    m_utils->set_response_data("ir_led_mode", (int32_t)cfg.ir_led_mode);
    m_utils->set_response_data("sensor_gain_max", (int32_t)cfg.sensor_gain_max);
    m_utils->set_response_data("sensor_shutter_min", (int32_t)cfg.sensor_shutter_min);
    m_utils->set_response_data("sensor_shutter_max", (int32_t)cfg.sensor_shutter_max);
    m_utils->set_response_data("ae_enable", (int32_t)cfg.ae_enable);
    m_utils->set_response_data("sensor_gain_manual", (int32_t)cfg.sensor_gain);
    m_utils->set_response_data("shutter_time_manual", (int32_t)cfg.sensor_shutter);
    m_utils->set_response_data("RGB_luma", (int32_t)cfg.luma_value[0]);
    m_utils->set_response_data("CFA_luma", (int32_t)cfg.luma_value[1]);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::image_awb_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    m_utils->get_value("action", action);

    if ("set" == action) {
      ret = awb_set_handle();
    } else if ("get" == action) {
      ret = awb_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image AWB action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_AWB_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::awb_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_awb_config_s awb_cfg = {0};
    string value;

    if (m_utils->get_value("wb_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>9) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "wb_mode value: %d is out of range: [0, 9]",tmp);
        m_utils->set_response_msg(AM_REST_AWB_MODE_ERR, msg);
        break;
      }
      SET_BIT(awb_cfg.enable_bits, AM_AWB_CONFIG_WB_MODE);
      awb_cfg.wb_mode = tmp;
    }

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET,&awb_cfg, sizeof(awb_cfg),
                &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AWB_HANDLE_ERR, "server set image AWB cfg failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::awb_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_awb_config_s cfg ={0} ;

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AWB_HANDLE_ERR, "server get image AWB cfg failed");
      break;
    }
    memcpy(&cfg, service_result.data, sizeof(am_awb_config_s));
    m_utils->set_response_data("wb_mode", (int32_t)cfg.wb_mode);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::image_af_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    m_utils->get_value("action", action);

    if ("set" == action) {
      ret = af_set_handle();
    } else if ("get" == action) {
      ret = af_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid image AF action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_AF_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::af_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_af_config_s af_cfg = {0};
    string value;

    if (m_utils->get_value("af_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>5) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "af_mode value: %d is out of range: [0, 5]",tmp);
        m_utils->set_response_msg(AM_REST_AF_MODE_ERR, msg);
        break;
      }
      SET_BIT(af_cfg.enable_bits, AM_AF_CONFIG_AF_MODE);
      af_cfg.af_mode = tmp;
    }

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET, &af_cfg, sizeof(af_cfg),
                &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AWB_HANDLE_ERR, "server set image AWB cfg failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIImage::af_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result = {0};
    am_af_config_s cfg = {0};

    METHOD_CALL(AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_AWB_HANDLE_ERR, "server get image AF cfg failed");
      break;
    }
    memcpy(&cfg, service_result.data, sizeof(am_af_config_s));
    m_utils->set_response_data("af_mode", (int32_t)cfg.af_mode);
  } while(0);

  return ret;
}
