/*******************************************************************************
 * am_image_service_msg_action.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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

#include "mw_struct.h"
#include "mw_api.h"

#include "am_image_quality_if.h"
#include "am_service_frame_if.h"
#include "am_api_image.h"

extern AM_SERVICE_STATE   g_service_state;
extern AMIImageQualityPtr g_image_quality;
extern AMIServiceFrame   *g_service_frame;

#define IMAGE_STYLE_LEVEL_NUM 5
#define IMAGE_STYLE_LEVEL_SHIFT 2
#define LOCAL_EXPOSURE_LEVEL_NUM 4
const int32_t saturation_tbl[IMAGE_STYLE_LEVEL_NUM] =
{ 44, 54, 64, 74, 88 };
const int32_t sharpness_tbl[IMAGE_STYLE_LEVEL_NUM] =
{ 2, 4, 6, 8, 10 };
const int32_t brightness_tbl[IMAGE_STYLE_LEVEL_NUM] =
{ -16, -8, 0, 8, 16 };
const int32_t contrast_tbl[IMAGE_STYLE_LEVEL_NUM] =
{ 44, 54, 64, 70, 76 };
const int32_t hue_tbl[IMAGE_STYLE_LEVEL_NUM] =
{ -10, -5, 0, 5, 10 };
const uint32_t le_tbl[LOCAL_EXPOSURE_LEVEL_NUM] =
{ 0, 64, 96, 128 };

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("Event service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize Evet service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Evet service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON IMAGE SERVICE DESTROY.");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
  g_service_frame->quit(); /* make run() in main quit */
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int ret = 0;
  PRINTF("ON IMAGE SERVICE START.");
  if (!g_image_quality) {
    ERROR("Image Service: failed to get AMIImageQuality instance\n");
    ret = -1;
  } else {
    if (g_service_state == AM_SERVICE_STATE_STARTED) {
      ret = 0;
    } else {
      if (!g_image_quality->start()) {
        ERROR("Image Service: start failed\n");
        ret = -2;
        g_service_state = AM_SERVICE_STATE_ERROR;
      } else {
        g_service_state = AM_SERVICE_STATE_STARTED;
      }
    }
  }
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int ret = 0;
  PRINTF("ON IMAGE SERVICE STOP.");
  if (!g_image_quality) {
    ERROR("Image Service: fail to get AMIImageQuality instance\n");
    ret = -1;
  } else {
    if (!g_image_quality->stop()) {
      ERROR("Image Service: stop failed\n");
      ret = -2;
    } else {
      g_service_state = AM_SERVICE_STATE_STOPPED;
    }
  }
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  INFO("Image Service restarted\n");
}

void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("Image Service get status\n");
}

void ON_IMAGE_TEST(void *msg_data,
                   int msg_data_size,
                   void *result_addr,
                   int result_max_size)
{
  INFO("Image Service test\n");
}

void ON_IMAGE_AE_SETTING_GET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  INFO("image service:ON_IMAGE_AE_SETTING_GET \n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  am_ae_config_s *ae_config = (am_ae_config_s *)service_result->data;

  AM_IQ_CONFIG iq_config;
  uint32_t le = 0;

  do {
    if (!result_addr) {
      ERROR("result_addr is NULL!\n");
      ret = -1;
      break;
    }
    if ((uint32_t) result_max_size < sizeof(ae_config)) {
      ERROR("result_max_size is not enough!\n");
      ret = -1;
      break;
    }

    iq_config.value = malloc(sizeof(uint32_t));

    iq_config.key = AM_IQ_AE_METERING_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_METERING_MODE config error\n");
      ret = -1;
      break;
    }
    ae_config->ae_metering_mode = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_DAY_NIGHT_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_DAY_NIGHT_MODE config error\n");
      ret = -1;
      break;
    }
    ae_config->day_night_mode = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SLOW_SHUTTER_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_SLOW_SHUTTER_MODE config error\n");
      ret = -1;
      break;
    }
    ae_config->slow_shutter_mode = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_ANTI_FLICKER_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_ANTI_FLICKER_MODE config error\n");
      ret = -1;
      break;
    }
    ae_config->anti_flicker_mode = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_TARGET_RATIO;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_TARGET_RATIO config error\n");
      ret = -1;
      break;
    }
    ae_config->ae_target_ratio = *(int*) iq_config.value;

    iq_config.key = AM_IQ_AE_BACKLIGHT_COMP_ENABLE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_BACKLIGHT_COMP_ENABLE config error\n");
      ret = -1;
      break;
    }
    ae_config->backlight_comp_enable = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_LOCAL_EXPOSURE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_LOCAL_EXPOSURE config error\n");
      ret = -1;
      break;
    }
    le = *(uint32_t*) iq_config.value;
    for (uint32_t i = 0; i < LOCAL_EXPOSURE_LEVEL_NUM; i ++) {
      if (le == le_tbl[i]) {
        ae_config->local_exposure = i;
        break;
      } else {
        ae_config->local_exposure = 0;
      }
    }

    iq_config.key = AM_IQ_AE_DC_IRIS_ENABLE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_DC_IRIS_ENABLE config error\n");
      ret = -1;
      break;
    }
    ae_config->dc_iris_enable = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SENSOR_GAIN_MAX;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_SENSOR_GAIN_MAX config error\n");
      ret = -1;
      break;
    }
    ae_config->sensor_gain_max = *(uint16_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MIN;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MIN config error\n");
      ret = -1;
      break;
    }
    ae_config->sensor_shutter_min = *(uint32_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MAX;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MAX config error\n");
      ret = -1;
      break;
    }
    ae_config->sensor_shutter_max = *(uint32_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_IR_LED_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_IR_LED_MODE config error\n");
      ret = -1;
      break;
    }
    ae_config->ir_led_mode = *(uint8_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SENSOR_GAIN;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_SENSOR_GAIN config error\n");
      ret = -1;
      break;
    }
    ae_config->sensor_gain = *(uint32_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_SHUTTER_TIME;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_SHUTTER_TIME config error\n");
      ret = -1;
      break;
    }
    ae_config->sensor_shutter = *(uint32_t*) iq_config.value;

    iq_config.key = AM_IQ_AE_AE_ENABLE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_AE_ENABLE config error\n");
      ret = -1;
      break;
    }
    ae_config->ae_enable = *(uint8_t*) iq_config.value;

    free(iq_config.value);

    iq_config.key = AM_IQ_AE_LUMA;
    iq_config.value = malloc(sizeof(AMAELuma));
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AE_LUMA config error\n");
      ret = -1;
      break;
    }
    ae_config->luma_value[0] = ((AMAELuma *) iq_config.value)->rgb_luma;
    ae_config->luma_value[1] = ((AMAELuma *) iq_config.value)->cfa_luma;

    free(iq_config.value);

  } while (0);
  service_result->ret = ret;
}

void ON_IMAGE_AE_SETTING_SET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  INFO("image service:ON_IMAGE_AE_SETTING_SET \n");
  int32_t ret = 0;
  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    ERROR("NULL pointer\n");
    return;
  }

  am_ae_config_s *ae_config = (am_ae_config_s *) msg_data;

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_AE_METERING_MODE)) {
    INFO("metering_mode set is %d\n", ae_config->ae_metering_mode);
    iq_config.key = AM_IQ_AE_METERING_MODE;
    *(int32_t*) iq_config.value = ae_config->ae_metering_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("set AM_IQ_AE_METERING_MODE config error\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_DAY_NIGHT_MODE)) {
    INFO("day_night_mode is %d!\n", ae_config->day_night_mode);
    iq_config.key = AM_IQ_AE_DAY_NIGHT_MODE;
    *(uint8_t*) iq_config.value = ae_config->day_night_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("day_night_mode set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SLOW_SHUTTER_MODE)) {
    INFO("slow_shutter_mode is %d!\n", ae_config->slow_shutter_mode);
    iq_config.key = AM_IQ_AE_SLOW_SHUTTER_MODE;
    *(uint8_t*) iq_config.value = ae_config->slow_shutter_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("slow_shutter_mode set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_ANTI_FLICKER_MODE)) {
    INFO("anti_flicker_mode is %d!\n", ae_config->anti_flicker_mode);
    iq_config.key = AM_IQ_AE_ANTI_FLICKER_MODE;
    *(int32_t*) iq_config.value = ae_config->anti_flicker_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("anti_flicker_mode failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_AE_TARGET_RATIO)) {
    INFO("ae_target_ratio set is %d!\n", ae_config->ae_target_ratio);
    iq_config.key = AM_IQ_AE_TARGET_RATIO;
    *(int32_t*) iq_config.value = ae_config->ae_target_ratio;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("ae_target_ratio set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_BACKLIGHT_COMP_ENABLE)) {
    INFO("backlight_compensation_enable set is %d\n",
         ae_config->backlight_comp_enable);
    iq_config.key = AM_IQ_AE_BACKLIGHT_COMP_ENABLE;
    *(int32_t*) iq_config.value = ae_config->backlight_comp_enable;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("backlight_compensation_enable failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_LOCAL_EXPOSURE)) {
    INFO("local_exposure set is %d!\n", ae_config->local_exposure);
    iq_config.key = AM_IQ_AE_LOCAL_EXPOSURE;
    *(int32_t*) iq_config.value = le_tbl[ae_config->local_exposure];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("local_exposure set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_DC_IRIS_ENABLE)) {
    INFO("dc_iris_enable set is %d!\n", ae_config->dc_iris_enable);
    iq_config.key = AM_IQ_AE_DC_IRIS_ENABLE;
    *(int32_t*) iq_config.value = ae_config->dc_iris_enable;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("dc_iris_enable set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MAX)) {
    INFO("sensor_gain_max is %d!\n", ae_config->sensor_gain_max);
    iq_config.key = AM_IQ_AE_SENSOR_GAIN_MAX;
    *(int32_t*) iq_config.value = ae_config->sensor_gain_max;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sensor_gain_max set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MIN)) {
    INFO("sensor_shutter_min set is %d!\n", ae_config->sensor_shutter_min);
    iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MIN;
    *(int32_t*) iq_config.value = ae_config->sensor_shutter_min;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sensor_shutter_min set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MAX)) {
    INFO("sensor_shutter_max is %d!\n", ae_config->sensor_shutter_max);
    iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MAX;
    *(int32_t*) iq_config.value = ae_config->sensor_shutter_max;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sensor_shutter_max set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_IR_LED_MODE)) {
    INFO("ir_led_mode set is %d!\n", ae_config->ir_led_mode);
    iq_config.key = AM_IQ_AE_IR_LED_MODE;
    *(int32_t*) iq_config.value = ae_config->ir_led_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("ir_led_mode set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MANUAL)) {
    INFO("sensor_shutter_manual set is %d!\n", ae_config->sensor_shutter);
    iq_config.key = AM_IQ_AE_SHUTTER_TIME;
    *(int32_t*) iq_config.value = ae_config->sensor_shutter;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sensor_shutter_manual set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MANUAL)) {
    INFO("sensor_gain_manual set is %d!\n", ae_config->sensor_gain);
    iq_config.key = AM_IQ_AE_SENSOR_GAIN;
    *(int32_t*) iq_config.value = ae_config->sensor_gain;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sensor_gain_manual set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(ae_config->enable_bits, AM_AE_CONFIG_AE_ENABLE)) {
    INFO("ae_enable set is %d!\n", ae_config->ae_enable);
    iq_config.key = AM_IQ_AE_AE_ENABLE;
    *(int32_t*) iq_config.value = ae_config->ae_enable;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("ae_enable set failed!\n");
      ret = -1;
    }
  }
  if (!g_image_quality->save_config()) {
    ERROR("Save config error!\n");
    ret = -1;
  }

  service_result->ret = ret;
}

void ON_IMAGE_AWB_SETTING_GET(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  INFO("image service:ON_IMAGE_AWB_SETTING_GET \n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  am_awb_config_s *awb_config = (am_awb_config_s *) service_result->data;

  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;

  do {
    if (!result_addr) {
      ERROR("result_addr is NULL!\n");
      ret = -1;
      break;
    }
    if ((uint32_t) result_max_size < sizeof(awb_config)) {
      ERROR("result_max_size is not enough!\n");
      ret = -1;
      break;
    }

    iq_config.key = AM_IQ_AWB_WB_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_AWB_WB_MODE config error\n");
      ret = -1;
      break;
    }
    awb_config->wb_mode = *(uint32_t*) iq_config.value;
  } while (0);
  service_result->ret = ret;
}

void ON_IMAGE_AWB_SETTING_SET(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  INFO("image service:ON_IMAGE_AWB_SETTING_SET \n");
  int32_t ret = 0;
  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    ERROR("NULL pointer\n");
    return;
  }

  am_awb_config_s *awb_config = (am_awb_config_s *) msg_data;

  if (TEST_BIT(awb_config->enable_bits, AM_AWB_CONFIG_WB_MODE)) {
    INFO("wb_mode set is %d!\n", awb_config->wb_mode);
    iq_config.key = AM_IQ_AWB_WB_MODE;
    *(int32_t*) iq_config.value = awb_config->wb_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("wb_mode set failed!\n");
      ret = -1;
    }
  }
  if (!g_image_quality->save_config()) {
    ERROR("Save config error!\n");
    ret = -1;
  }

  service_result->ret = ret;
}

void ON_IMAGE_AF_SETTING_GET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  WARN("image service:ON_IMAGE_AF_SETTING_GET, empty\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
}

void ON_IMAGE_AF_SETTING_SET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  WARN("image service:ON_IMAGE_AF_SETTING_SET ,empty\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
}

void ON_IMAGE_NR_SETTING_GET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  INFO("image service:ON_IMAGE_NF_SETTING_GET \n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  am_noise_filter_config_s *nf_config = (am_noise_filter_config_s *) service_result->data;

  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;

  do {
    if (!result_addr) {
      ERROR("result_addr is NULL!\n");
      ret = -1;
      break;
    }
    if ((uint32_t) result_max_size < sizeof(nf_config)) {
      ERROR("result_max_size is not enough!\n");
      ret = -1;
      break;
    }

    iq_config.key = AM_IQ_NF_MCTF_STRENGTH;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_NF_MCTF_STRENGTH config error\n");
      ret = -1;
      break;
    }
    nf_config->mctf_strength = *(uint32_t*) iq_config.value;
  } while (0);
  service_result->ret = ret;
}

void ON_IMAGE_NR_SETTING_SET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  INFO("image service:ON_IMAGE_NF_SETTING_SET \n");
  int32_t ret = 0;
  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    ERROR("NULL pointer\n");
    return;
  }

  am_noise_filter_config_s *nf_config = (am_noise_filter_config_s *) msg_data;

  if (TEST_BIT(nf_config->enable_bits, AM_NOISE_FILTER_CONFIG_MCTF_STRENGTH)) {
    INFO("mctf_strength is %d!\n", nf_config->mctf_strength);
    iq_config.key = AM_IQ_NF_MCTF_STRENGTH;
    *(int32_t*) iq_config.value = nf_config->mctf_strength;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("mctf_strength set failed!\n");
      ret = -1;
    }
  }
  if (!g_image_quality->save_config()) {
    ERROR("Save config error!\n");
    ret = -1;
  }

  service_result->ret = ret;
}

void ON_IMAGE_STYLE_SETTING_GET(void *msg_data,
                                int msg_data_size,
                                void *result_addr,
                                int result_max_size)
{
  INFO("image service:ON_IMAGE_STYLE_SETTING_GET \n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  am_image_style_config_s *style_config =
      (am_image_style_config_s *) service_result->data;

  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;
  int32_t saturation = 0;
  int32_t brightness = 0;
  int32_t hue = 0;
  int32_t contrast = 0;
  int32_t sharpness = 0;

  do {
    if (!result_addr) {
      ERROR("result_addr is NULL!\n");
      ret = -1;
      break;
    }
    if ((uint32_t) result_max_size < sizeof(style_config)) {
      ERROR("result_max_size is not enough!\n");
      ret = -1;
      break;
    }

    iq_config.key = AM_IQ_STYLE_SATURATION;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_SATURATION config error\n");
      ret = -1;
      break;
    }
    saturation = *(int32_t*) iq_config.value;

    iq_config.key = AM_IQ_STYLE_BRIGHTNESS;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_BRIGHTNESS config error\n");
      ret = -1;
      break;
    }
    brightness = *(int32_t*) iq_config.value;

    iq_config.key = AM_IQ_STYLE_HUE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_HUE config error\n");
      ret = -1;
      break;
    }
    hue = *(int32_t*) iq_config.value;

    iq_config.key = AM_IQ_STYLE_CONTRAST;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_CONTRAST config error\n");
      ret = -1;
      break;
    }
    contrast = *(int32_t*) iq_config.value;

    iq_config.key = AM_IQ_STYLE_SHARPNESS;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_SHARPNESS config error\n");
      ret = -1;
      break;
    }
    sharpness = *(int32_t*) iq_config.value;

    iq_config.key = AM_IQ_STYLE_AUTO_CONTRAST_MODE;
    if (!g_image_quality->get_config(&iq_config)) {
      ERROR("get AM_IQ_STYLE_AUTO_CONTRAST_MODE config error\n");
      ret = -1;
      break;
    }
    style_config->auto_contrast_mode = *(int32_t*) iq_config.value;
  } while (0);

  for (uint32_t i = 0; i < IMAGE_STYLE_LEVEL_NUM; i ++) {
    if (saturation == saturation_tbl[i]) {
      style_config->saturation = i - IMAGE_STYLE_LEVEL_SHIFT;
      break;
    } else {
      style_config->saturation = 0;
    }
  }
  for (uint32_t i = 0; i < IMAGE_STYLE_LEVEL_NUM; i ++) {
    if (brightness == brightness_tbl[i]) {
      style_config->brightness = i - IMAGE_STYLE_LEVEL_SHIFT;
      break;
    } else {
      style_config->brightness = 0;
    }
  }
  for (uint32_t i = 0; i < IMAGE_STYLE_LEVEL_NUM; i ++) {
    if (hue == hue_tbl[i]) {
      style_config->hue = i - IMAGE_STYLE_LEVEL_SHIFT;
      break;
    } else {
      style_config->hue = 0;
    }
  }
  for (uint32_t i = 0; i < IMAGE_STYLE_LEVEL_NUM; i ++) {
    if (contrast == contrast_tbl[i]) {
      style_config->contrast = i - IMAGE_STYLE_LEVEL_SHIFT;
      break;
    } else {
      style_config->contrast = 0;
    }
  }
  for (uint32_t i = 0; i < IMAGE_STYLE_LEVEL_NUM; i ++) {
    if (sharpness == sharpness_tbl[i]) {
      style_config->sharpness = i - IMAGE_STYLE_LEVEL_SHIFT;
      break;
    } else {
      style_config->sharpness = 0;
    }
  }
  service_result->ret = ret;
}

void ON_IMAGE_STYLE_SETTING_SET(void *msg_data,
                                int msg_data_size,
                                void *result_addr,
                                int result_max_size)
{
  INFO("image service:ON_IMAGE_STYLE_SETTING_SET \n");
  int32_t ret = 0;
  AM_IQ_CONFIG iq_config;
  int config_value = 0;
  iq_config.value = &config_value;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    ERROR("NULL pointer\n");
    return;
  }

  am_image_style_config_s *style_config = (am_image_style_config_s*) msg_data;

  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_SATURATION)) {
    INFO("saturation is %d!\n", style_config->saturation);
    iq_config.key = AM_IQ_STYLE_SATURATION;
    *(int32_t*) iq_config.value = saturation_tbl[IMAGE_STYLE_LEVEL_SHIFT
        + style_config->saturation];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("saturation set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_BRIGHTNESS)) {
    INFO("brightness is %d!\n", style_config->brightness);
    iq_config.key = AM_IQ_STYLE_BRIGHTNESS;
    *(int32_t*) iq_config.value = brightness_tbl[IMAGE_STYLE_LEVEL_SHIFT
        + style_config->brightness];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("brightness set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_HUE)) {
    INFO("hue is %d!\n", style_config->hue);
    iq_config.key = AM_IQ_STYLE_HUE;
    *(int32_t*) iq_config.value = hue_tbl[IMAGE_STYLE_LEVEL_SHIFT
        + style_config->hue];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("hue set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_CONTRAST)) {
    INFO("contrast is %d!\n", style_config->contrast);
    iq_config.key = AM_IQ_STYLE_CONTRAST;
    *(int32_t*) iq_config.value = contrast_tbl[IMAGE_STYLE_LEVEL_SHIFT
        + style_config->contrast];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("contrast set failed!\n");
      ret = -1;
    }
  }

  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_SHARPNESS)) {
    INFO("sharpness is %d!\n", style_config->sharpness);
    iq_config.key = AM_IQ_STYLE_SHARPNESS;
    *(int32_t*) iq_config.value = sharpness_tbl[IMAGE_STYLE_LEVEL_SHIFT
        + style_config->sharpness];
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("sharpness set failed!\n");
      ret = -1;
    }
  }
  if (TEST_BIT(style_config->enable_bits, AM_IMAGE_STYLE_CONFIG_AUTO_CONTRAST_MODE)) {
    INFO("auto_contrast_mode is %d\n", style_config->auto_contrast_mode);
    iq_config.key = AM_IQ_STYLE_AUTO_CONTRAST_MODE;
    *(int32_t*) iq_config.value = style_config->auto_contrast_mode;
    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("auto_contrast_mode set failed!\n");
      ret = -1;
    }
  }
  if (!g_image_quality->save_config()) {
    ERROR("Save config error!\n");
    ret = -1;
  }

  service_result->ret = ret;
}

void ON_IMAGE_ADJ_BIN_LOAD(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  INFO("image service:ON_IMAGE_ADJ_BIN_LOAD \n");
  int32_t ret = 0;
  uint32_t len = 0;
  AM_IQ_CONFIG iq_config;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  len = strlen((char*)msg_data);
  char *config_value = (char*) malloc((len + 1)*sizeof(char));

  do {

    if (!msg_data) {
      ERROR("NULL pointer\n");
      ret = -1;
      break;
    }

    if (config_value == nullptr) {
      PERROR("melloc failed");
      ret = -1;
      break;
    }
    strncpy(config_value, (char*)msg_data, len);
    config_value[len] = '\0';
    iq_config.value = config_value;
    iq_config.key = AM_IQ_AEB_ADJ_BIN_LOAD;

    if (!g_image_quality->set_config(&iq_config)) {
      ERROR("load_adj_bin failed");
      ret = -1;
      break;
    }
  }while(0);

  if (config_value != nullptr) {
    free(config_value);
  }
  service_result->ret = ret;
}


