/*******************************************************************************
 * am_image_quality.cpp
 *
 * History:
 *   Dec 30, 2014 - [binwang] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_thread.h"
#include "am_image_quality.h"

#if defined(CONFIG_ARCH_S3L)
#include "img_struct_arch.h"
#else
#include "img_adv_struct_arch.h"
#endif
#include "mw_struct.h"
#include "mw_api.h"

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);

static mw_sys_info sys_info =
{ 0 };

AMImageQuality *AMImageQuality::m_instance = nullptr;
AMImageQuality::AMImageQuality() :
    m_iq_config(nullptr),
    m_aaa_start(nullptr),
    m_ref_count(0),
    m_iq_state(AM_IQ_STATE_NOT_INIT),
    m_fd_iav(-1)
{
}

AMIImageQualityPtr AMIImageQuality::get_instance()
{
  return AMImageQuality::get_instance();
}

AMImageQuality *AMImageQuality::get_instance()
{
  DECLARE_MUTEX
  if (!m_instance) {
    m_instance = new AMImageQuality;
    if (!m_instance->init()) {
      m_instance->destroy();
      delete m_instance;
      m_instance = nullptr;
    }
  }
  return m_instance;
}

void AMImageQuality::inc_ref()
{
  ++ m_ref_count;
}

void AMImageQuality::release()
{
  DECLARE_MUTEX
  DEBUG("AMImageQuality:: release is called\n");
  if ((m_ref_count >= 0) && (-- m_ref_count <= 0)) {
    DEBUG("AMImageQuality::this is the last reference of AMImageQuality's "
          "object, delete object instance %p ", m_instance);
    m_instance->destroy();
    delete m_instance;
    m_instance = nullptr;
  }
}

AMImageQuality::~AMImageQuality()
{
  destroy();
  m_instance = nullptr;
  DEBUG("AMImageQuality::~AMImageQuality called\n");
}

bool AMImageQuality::init()
{
  DECLARE_MUTEX
  bool result = true;

  do {
    if (m_iq_state != AM_IQ_STATE_NOT_INIT) {
      ERROR("AMImageQuality::init failed, IQ state is %d\n", (int )m_iq_state);
      result = false;
      break;
    }
    if ((m_fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
      ERROR("AMImageQuality::open /dev/iav\n");
      result = false;
      break;
    }
    if (!m_iq_config) {
      m_iq_config = new AMIQConfig;
      if (!m_iq_config) {
        result = false;
        break;
      }
    }
    //get config from /etc/oryx/image/iq_config.acs, init need these config data
    result = m_iq_config->load_config();
    if (!result) {
      ERROR("AMImageQuality::unable to load config file to init\n");
      break;
    }

    if (mw_set_log_level((int) m_iq_config->m_loaded.log_level) < 0) {
      ERROR("AMImageQuality::mw_set_log_level failed\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_FILE_TYPE_TOTAL_NUM; i ++) {
      if ((i != AM_FILE_TYPE_CFG) && m_iq_config->m_aeb_adj_file_flag[i]) {
        if (mw_load_aaa_param_file(m_iq_config->m_aeb_adj_file_name[i], i)
            < 0) {
          ERROR("AMImageQuality::load aeb adj file failed\n");
          result = false;
          break;
        }
      }
    }
    m_aaa_start = AMThread::create("start_aaa_thread",
                                   prepare_to_run_server,
                                   this);
    if (!m_aaa_start) {
      ERROR("AMImageQuality::create start_aaa_thread failed\n");
      result = false;
      break;
    }
  } while (0);

  if (result) {
    m_iq_state = AM_IQ_STATE_INIT_DONE;
  }

  INFO("AMImageQuality: sync current config to using\n");
  m_iq_config->sync();

  return result;
}

bool AMImageQuality::destroy()
{
  bool result = true;

  if (m_iq_state != AM_IQ_STATE_NOT_INIT) {
    if (mw_stop_aaa() < 0) {
      ERROR("AMImageQuality::mw_stop_aaa failed\n");
      result = false;
    }
    m_iq_state = AM_IQ_STATE_NOT_INIT;
  }
  AM_DESTROY(m_aaa_start);
  if (m_fd_iav >= 0) {
    close(m_fd_iav);
    m_fd_iav = -1;
  }

  return result;
}

bool AMImageQuality::start()
{
  bool result = true;
  switch (m_iq_state) {
    case AM_IQ_STATE_NOT_INIT:
      ERROR("AMImageQuality::IQ not init, cannot start!\n");
      result = false;
      break;
    case AM_IQ_STATE_RUNNING:
      WARN("AMImageQuality:: IQ running , cannot run again, ignore this!\n");
      result = true;
      break;
    case AM_IQ_STATE_INIT_DONE:
      //init also started AE and AWB, so here do not need to do again
      break;
    case AM_IQ_STATE_STOPPED:
      if (mw_enable_ae(1) < 0) {
        ERROR("AMImageQuality::enable AE error\n");
        result = false;
        break;
      }
      if (mw_enable_awb(1) < 0) {
        ERROR("AMImageQuality::enable AWB error\n");
        result = false;
        break;
      }
      break;
    case AM_IQ_STATE_ERROR:
    default:
      ERROR("AMImageQuality::IQ state %d not supported to start\n",
            (int )m_iq_state);
      result = false;
      break;
  }
  if (result) {
    m_iq_state = AM_IQ_STATE_RUNNING;
  } else {
    m_iq_state = AM_IQ_STATE_ERROR;
  }

  return result;
}

bool AMImageQuality::stop()
{
  DECLARE_MUTEX
  bool result = true;
  switch (m_iq_state) {
    case AM_IQ_STATE_NOT_INIT:
      ERROR("AMImageQuality::IQ not init, cannot stop!\n");
      result = false;
      break;
    case AM_IQ_STATE_INIT_DONE:
    case AM_IQ_STATE_STOPPED:
      INFO("AMImageQuality: IQ not running, just stop it!\n");
      result = true;
      break;
    case AM_IQ_STATE_RUNNING:
    case AM_IQ_STATE_ERROR:
      //stop it even if IQ state is ERROR
      if (m_iq_state == AM_IQ_STATE_STOPPED) {
        WARN("AMImageQuality::IQ has already stopped\n");
        break;
      }
      if (mw_enable_ae(0) < 0) {
        ERROR("AMImageQuality::disable AE error\n");
        result = false;
        break;
      }
      if (mw_enable_awb(0) < 0) {
        ERROR("AMImageQuality::disable AWB error\n");
        result = false;
        break;
      }
      break;
    default:
      ERROR("AMImageQuality: IQ state %d not supported to stop\n",
            (int )m_iq_state);
      result = false;
      break;
  }

  if (result) {
    m_iq_state = AM_IQ_STATE_STOPPED;
  } else {
    m_iq_state = AM_IQ_STATE_ERROR;
  }

  return result;
}

void AMImageQuality::print_config(AMIQParam *iq_param)
{
  PRINTF("AE: \n");
  PRINTF("ae_metering_mode = %d\n"
         "day_night_mode = %d\n"
         "slow_shutter_mode = %d\n"
         "anti_flicker_mode = %d\n"
         "ae_target_ratio = %d\n"
         "backlight_comp_enable = %d\n"
         "local_exposure = %d\n"
         "dc_iris_enable = %d\n"
         "ae_enable = %d\n"
         "sensor_gain_max = %d\n"
         "sensor_shutter_min = %d\n"
         "sensor_shutter_max = %d\n"
         "ir_led_mode = %d\n",
         iq_param->ae.ae_metering_mode, iq_param->ae.day_night_mode,
         iq_param->ae.slow_shutter_mode, iq_param->ae.anti_flicker_mode,
         iq_param->ae.ae_target_ratio, iq_param->ae.backlight_comp_enable,
         iq_param->ae.local_exposure, iq_param->ae.dc_iris_enable,
         iq_param->ae.ae_enable, iq_param->ae.sensor_gain_max,
         iq_param->ae.sensor_shutter_min, iq_param->ae.sensor_shutter_max,
         iq_param->ae.ir_led_mode);

  PRINTF("\nAWB: \n");
  PRINTF("wb_mode = %d\n", iq_param->awb.wb_mode);

  PRINTF("\nNoise Filter: \n");
  PRINTF("mctf_strength = %d\n", iq_param->nf.mctf_strength);

  PRINTF("\nStyle: \n");
  PRINTF("saturation = %d\n"
         "brightness = %d\n"
         "hue = %d\n"
         "contrast = %d\n"
         "sharpness = %d\n",
         "auto_contrast_mode = %d\n",
         iq_param->style.saturation, iq_param->style.brightness,
         iq_param->style.hue, iq_param->style.contrast,
         iq_param->style.sharpness,
         iq_param->style.auto_contrast_mode);
}

bool AMImageQuality::load_config()
{
  DECLARE_MUTEX
  bool result = true;
  do {
    if (m_iq_state == AM_IQ_STATE_NOT_INIT) {
      ERROR("AMImageQuality: IQ not init, cannot load_config!\n");
      result = false;
      break;
    }
    result = m_iq_config->load_config();
    if (!result) {
      ERROR("AMImageQuality::load_config failed\n");
      break;
    }
    print_config(&m_iq_config->m_loaded);
  } while (0);

  return result;
}

bool AMImageQuality::save_config()
{
  DECLARE_MUTEX
  bool result = true;
  do {
    if (m_iq_state == AM_IQ_STATE_NOT_INIT) {
      ERROR("AMImageQuality: IQ not init, cannot save_config!\n");
      result = false;
      break;
    }
    result = m_iq_config->save_config();
    if (!result) {
      ERROR("AMImageQuality::save_config failed\n");
      break;
    }
  } while (0);

  return result;
}

void AMImageQuality::prepare_to_run_server(void *data)
{
  AMImageQuality *iq = (AMImageQuality *) data;
  int32_t iav_state = 0;
  do {
    int ret = MW_AAA_SUCCESS;
    if ((ret = mw_start_aaa(iq->m_fd_iav)) != MW_AAA_SUCCESS) {
      switch(ret) {
        case MW_AAA_INIT_FAIL:
          ERROR("AMImageQuality: Failed to init!");
          break;
        case MW_AAA_NL_INIT_FAIL:
          ERROR("AMImageQuality: Failed to create netlink!");
          break;
        case MW_AAA_INTERRUPTED:
          NOTICE("AMImageQuality: Operation is interrupted!");
          break;
        default:
          ERROR("AMImageQuality::mw_start_aaa error\n");
          break;
      }
      break;
    }
    if (mw_get_iav_state(iq->m_fd_iav, &iav_state) < 0) {
      ERROR("mw_get_iav_state failed");
      break;
    }
    if (iav_state == IQ_IAV_STATE_IDLE || iav_state == IQ_IAV_STATE_INIT) {
      WARN("AMImageQuality::IAV state is not ready, start AAA interrupted!");
      break;
    }
    if (mw_get_sys_info(&sys_info) < 0) {
      ERROR("AMImageQuality::mw_get_sensor_param_for_3A error\n");
      break;
    }
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      INFO("AMImageQuality::Sensor is Normal mode\n");
    } else {
      INFO("AMImageQuality::Sensor is %dX HDR mode", sys_info.res.hdr_expo_num);
    }
    if (mw_enable_ae(iq->m_iq_config->m_loaded.ae.ae_enable) < 0) {
      ERROR("AMImageQuality::mw_enable_ae error\n");
      break;
    }
    if (mw_enable_awb(1) < 0) {
      ERROR("AMImageQuality::mw_enable_awb error\n");
      break;
    }
    usleep(66000); //fix me, add delay to make sure enable AWB take effect in 3A lib
    if (mw_is_dc_iris_supported()) {
      iq->set_dc_iris_enable(&iq->m_iq_config->m_loaded.ae.dc_iris_enable);
    }
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      if (!iq->set_ae_metering_mode(&iq->m_iq_config->m_loaded.ae.ae_metering_mode)) {
        ERROR("AMImageQuality::set_ae_metering_mode error, start failed\n");
        break;
      }
    }
    if (!iq->set_day_night_mode(&iq->m_iq_config->m_loaded.ae.day_night_mode)) {
      ERROR("AMImageQuality::set_day_night_mode error, start failed\n");
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);

    ae_param.anti_flicker_mode =
        (mw_anti_flicker_mode) iq->m_iq_config->m_loaded.ae.anti_flicker_mode;
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      ae_param.slow_shutter_enable = iq->m_iq_config->m_loaded.ae.slow_shutter_mode;
    }
    ae_param.sensor_gain_max = iq->m_iq_config->m_loaded.ae.sensor_gain_max;
    ae_param.shutter_time_min =
        ROUND_DIV(512000000, iq->m_iq_config->m_loaded.ae.sensor_shutter_min);
      ae_param.shutter_time_max =
          ROUND_DIV(512000000, iq->m_iq_config->m_loaded.ae.sensor_shutter_max);
    ae_param.ir_led_mode = iq->m_iq_config->m_loaded.ae.ir_led_mode;
    if (mw_set_ae_param(&ae_param) < 0) {
      WARN("AMImageQuality::mw_set_ae_param ignored\n");
    }

    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      if (!iq->set_ae_target_ratio(&iq->m_iq_config->m_loaded.ae.ae_target_ratio)) {
        ERROR("AMImageQuality::set_ae_target_ratio error, start failed\n");
        break;
      }
    }
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      if (!iq->set_local_exposure(&iq->m_iq_config->m_loaded.ae.local_exposure)) {
        ERROR("AMImageQuality::set_local_exposure error, start failed\n");
        break;
      }
    }
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      if (!iq->set_backlight_comp_enable(&iq->m_iq_config->m_loaded.ae.backlight_comp_enable)) {
        ERROR("AMImageQuality::set_backlight_comp_enable error, start failed\n");
        break;
      }
    }
    /*NF config*/
    if (!iq->set_mctf_strength(&iq->m_iq_config->m_loaded.nf.mctf_strength)) {
      ERROR("AMImageQuality::set_mctf_strength error, start failed\n");
      break;
    }
    /*IQ style config*/
    mw_set_saturation(iq->m_iq_config->m_loaded.style.saturation);
    mw_set_contrast(iq->m_iq_config->m_loaded.style.contrast);
    mw_set_sharpness(iq->m_iq_config->m_loaded.style.sharpness);
    if (sys_info.res.hdr_expo_num <= MIN_HDR_EXPOSURE_NUM) {
      mw_set_brightness(iq->m_iq_config->m_loaded.style.brightness);
      if (!iq->set_auto_contrast_mode(&iq->m_iq_config->m_loaded.style.auto_contrast_mode)) {
        ERROR("AMImageQuality::set_auto_contrast_mode error, start failed\n");
        break;
      }
    }

    /*AWB config*/
    if (!iq->set_wb_mode(&iq->m_iq_config->m_loaded.awb.wb_mode)) {
      ERROR("AMImageQuality::set_wb_mode error, start failed\n");
      break;
    }
  } while (0);
}

bool AMImageQuality::set_config(AM_IQ_CONFIG *config)
{
  bool result = true;
  int32_t iav_state = 0;

  if (!config || !config->value) {
    ERROR("AMImageQuality::Invalid argument!\n");
    return false;
  }

  if (mw_get_iav_state(m_fd_iav, &iav_state) < 0) {
    ERROR("mw_get_iav_state failed");
    return false;
  }
  if (iav_state == IQ_IAV_STATE_IDLE || iav_state == IQ_IAV_STATE_INIT) {
    WARN("AMImageQuality::IAV state is not ready, start AAA interrupted!");
    return false;
  }
  if (mw_get_sys_info(&sys_info) < 0) {//update sys_info.res.hdr_expo_num
    ERROR("AMImageQuality::mw_get_sensor_param_for_3A error\n");
    return false;
  }

  switch (config->key) {
    /*log level config*/
    case AM_IQ_LOGLEVEL:
      result = set_log_level((AM_IQ_LOG_LEVEL *) config->value);
      break;
      /*AE config*/
    case AM_IQ_AE_METERING_MODE:
      result = set_ae_metering_mode((AM_AE_METERING_MODE *) config->value);
      break;
    case AM_IQ_AE_DAY_NIGHT_MODE:
      result = set_day_night_mode((AM_DAY_NIGHT_MODE *) config->value);
      break;
    case AM_IQ_AE_SLOW_SHUTTER_MODE:
      result = set_slow_shutter_mode((AM_SLOW_SHUTTER_MODE *) config->value);
      break;
    case AM_IQ_AE_ANTI_FLICKER_MODE:
      result = set_anti_flick_mode((AM_ANTI_FLICK_MODE *) config->value);
      break;
    case AM_IQ_AE_TARGET_RATIO:
      result = set_ae_target_ratio((int32_t *) config->value);
      break;
    case AM_IQ_AE_BACKLIGHT_COMP_ENABLE:
      result =
          set_backlight_comp_enable((AM_BACKLIGHT_COMP_MODE *) config->value);
      break;
    case AM_IQ_AE_LOCAL_EXPOSURE:
      result = set_local_exposure((uint32_t *) config->value);
      break;
    case AM_IQ_AE_DC_IRIS_ENABLE:
      if (mw_is_dc_iris_supported()) {
        result = set_dc_iris_enable((AM_DC_IRIS_MODE *) config->value);
      }
      break;
    case AM_IQ_AE_SENSOR_GAIN_MAX:
      result = set_sensor_gain_max((uint32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_SHUTTER_MIN:
      result = set_sensor_shutter_min((uint32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_SHUTTER_MAX:
      result = set_sensor_shutter_max((uint32_t *) config->value);
      break;
    case AM_IQ_AE_IR_LED_MODE:
      result = set_ir_led_mode((AM_IR_LED_MODE *) config->value);
      break;
    case AM_IQ_AE_SHUTTER_TIME:
      result = set_shutter_time((int32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_GAIN:
      result = set_sensor_gain((int32_t *) config->value);
      break;
    case AM_IQ_AE_AE_ENABLE:
      result = set_ae_enable((AM_AE_MODE *) config->value);
      break;
      /*AWB config*/
    case AM_IQ_AWB_WB_MODE:
      result = set_wb_mode((AM_WB_MODE *) config->value);
      break;
      /*NF config*/
    case AM_IQ_NF_MCTF_STRENGTH:
      result = set_mctf_strength((uint32_t *) config->value);
      break;
      /*IQ style config*/
    case AM_IQ_STYLE_SATURATION:
      result = set_saturation((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_BRIGHTNESS:
      result = set_brightness((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_HUE:
      result = set_hue((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_CONTRAST:
      result = set_contrast((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_SHARPNESS:
      result = set_sharpness((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_AUTO_CONTRAST_MODE:
      result = set_auto_contrast_mode((int32_t *) config->value);
      break;
    case AM_IQ_AEB_ADJ_BIN_LOAD: {
      result = load_adj_bin((char *) config->value);
    }break;
    default:
      ERROR("AMImageQuality::set_config,unknown key\n");
      result = false;
      break;
  }
  //All set can be done on the fly. Every set should be sync if it returns successfully.
  if (result) {
    INFO("AMImageQuality: sync current config to using\n");
    m_iq_config->sync();
  } else {
    ERROR("AMImageQuality::set_config error, key_num %d\n", config->key);
  }

  return result;
}

bool AMImageQuality::get_config(AM_IQ_CONFIG *config)
{
  bool result = true;
  if (!config || !config->value) {
    ERROR("AMImageQuality::Invalid argument!\n");
    return false;
  }

  switch (config->key) {
    /*log level config*/
    case AM_IQ_LOGLEVEL:
      result = get_log_level((AM_IQ_LOG_LEVEL *) config->value);
      break;
      /*AE config*/
    case AM_IQ_AE_METERING_MODE:
      result = get_ae_metering_mode((AM_AE_METERING_MODE *) config->value);
      break;
    case AM_IQ_AE_DAY_NIGHT_MODE:
      result = get_day_night_mode((AM_DAY_NIGHT_MODE *) config->value);
      break;
    case AM_IQ_AE_SLOW_SHUTTER_MODE:
      result = get_slow_shutter_mode((AM_SLOW_SHUTTER_MODE *) config->value);
      break;
    case AM_IQ_AE_ANTI_FLICKER_MODE:
      result = get_anti_flick_mode((AM_ANTI_FLICK_MODE *) config->value);
      break;
    case AM_IQ_AE_TARGET_RATIO:
      result = get_ae_target_ratio((int32_t *) config->value);
      break;
    case AM_IQ_AE_BACKLIGHT_COMP_ENABLE:
      result =
          get_backlight_comp_enable((AM_BACKLIGHT_COMP_MODE *) config->value);
      break;
    case AM_IQ_AE_LOCAL_EXPOSURE:
      result = get_local_exposure((uint32_t *) config->value);
      break;
    case AM_IQ_AE_DC_IRIS_ENABLE:
      if (mw_is_dc_iris_supported()) {
        result = get_dc_iris_enable((AM_DC_IRIS_MODE *) config->value);
      }
      break;
    case AM_IQ_AE_SENSOR_GAIN_MAX:
      result = get_sensor_gain_max((uint32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_SHUTTER_MIN:
      result = get_sensor_shutter_min((uint32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_SHUTTER_MAX:
      result = get_sensor_shutter_max((uint32_t *) config->value);
      break;
    case AM_IQ_AE_IR_LED_MODE:
      result = get_ir_led_mode((AM_IR_LED_MODE *) config->value);
      break;
    case AM_IQ_AE_SHUTTER_TIME:
      result = get_shutter_time((int32_t *) config->value);
      break;
    case AM_IQ_AE_SENSOR_GAIN:
      result = get_sensor_gain((int32_t *) config->value);
      break;
    case AM_IQ_AE_LUMA:
      result = get_luma((AMAELuma *) config->value);
      break;
    case AM_IQ_AE_AE_ENABLE:
      result = get_ae_enable((AM_AE_MODE *) config->value);
      break;
      /*AWB config*/
    case AM_IQ_AWB_WB_MODE:
      result = get_wb_mode((AM_WB_MODE *) config->value);
      break;
      /*NF config*/
    case AM_IQ_NF_MCTF_STRENGTH:
      result = get_mctf_strength((uint32_t *) config->value);
      break;
      /*IQ style config*/
    case AM_IQ_STYLE_SATURATION:
      result = get_saturation((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_BRIGHTNESS:
      result = get_brightness((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_HUE:
      result = get_hue((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_CONTRAST:
      result = get_contrast((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_SHARPNESS:
      result = get_sharpness((int32_t *) config->value);
      break;
    case AM_IQ_STYLE_AUTO_CONTRAST_MODE:
      result = get_auto_contrast_mode((int32_t *) config->value);
      break;
    default:
      ERROR("AMImageQuality::get_config,unknown key\n");
      result = false;
      break;
  }

  return result;
}



/*log level config*/
bool AMImageQuality::set_log_level(AM_IQ_LOG_LEVEL *log_level)
{
  bool result = true;
  do {
    if (!log_level) {
      ERROR("AMImageQuality::set_log_level, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_log_level((mw_log_level) *log_level) < 0) {
      ERROR("AMImageQuality::mw_set_log_level error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.log_level = *log_level;
  } while (0);

  return result;
}

/*AE config*/
bool AMImageQuality::set_ae_metering_mode(AM_AE_METERING_MODE *ae_metering_mode)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set AE metering mode in HDR mode\n");
      break;
    }
    if (!ae_metering_mode) {
      ERROR("AMImageQuality::set_ae_metering_mode, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_ae_metering_mode((mw_ae_metering_mode) *ae_metering_mode) < 0) {
      ERROR("AMImageQuality::set_ae_metering_mode error\n");
      result = false;
      break;
    }
    if (*ae_metering_mode == AM_AE_CUSTOM_METERING) {
      mw_set_ae_metering_table(
        (mw_ae_metering_table*)&m_iq_config->m_loaded.ae.ae_metering_table);
    }
    m_iq_config->m_loaded.ae.ae_metering_mode = *ae_metering_mode;
  } while (0);

  return result;
}

bool AMImageQuality::set_day_night_mode(AM_DAY_NIGHT_MODE *day_night_mode)
{
  bool result = true;
  do {
    if (!day_night_mode) {
      ERROR("AMImageQuality::set_day_night_mode, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_enable_day_night_mode((int) *day_night_mode) < 0) {
      ERROR("AMImageQuality::set_day_night_mode error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.day_night_mode = *day_night_mode;
  } while (0);

  return result;
}

bool AMImageQuality::set_slow_shutter_mode(AM_SLOW_SHUTTER_MODE *slow_shutter_mode)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set slow shutter in HDR mode\n");
      break;
    }
    if (!slow_shutter_mode) {
      ERROR("AMImageQuality::set_slow_shutter_mode, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.slow_shutter_enable = (uint32_t) *slow_shutter_mode;
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_slow_shutter_mode error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.slow_shutter_mode = *slow_shutter_mode;
  } while (0);

  return result;
}

bool AMImageQuality::set_anti_flick_mode(AM_ANTI_FLICK_MODE *anti_flicker_mode)
{
  bool result = true;
  do {
    if (!anti_flicker_mode) {
      ERROR("AMImageQuality::anti_flicker_mode, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.anti_flicker_mode = (mw_anti_flicker_mode) *anti_flicker_mode;
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_anti_flick_mode error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.anti_flicker_mode = *anti_flicker_mode;
  } while (0);

  return result;
}

bool AMImageQuality::set_ae_target_ratio(int32_t *ae_target_ratio)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set AE target ratio in HDR mode\n");
      break;
    }
    if (!ae_target_ratio) {
      ERROR("AMImageQuality::set_ae_target_ratio, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_exposure_level(ae_target_ratio) < 0) {
      ERROR("AMImageQuality::set_ae_target_ratio error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.ae_target_ratio = *ae_target_ratio;
  } while (0);

  return result;
}

bool AMImageQuality::set_backlight_comp_enable(AM_BACKLIGHT_COMP_MODE *backlight_comp_enable)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set backlight composition in HDR mode\n");
      break;
    }
    if (!backlight_comp_enable) {
      ERROR("AMImageQuality::set_backlight_comp_enable, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_enable_backlight_compensation((AM_BACKLIGHT_COMP_MODE) *backlight_comp_enable)
        < 0) {
      ERROR("AMImageQuality::set_backlight_comp_enable error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.backlight_comp_enable = *backlight_comp_enable;
  } while (0);

  return result;
}

bool AMImageQuality::set_local_exposure(uint32_t *local_exposure)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set local exposure in HDR mode\n");
      break;
    }
    if (!local_exposure) {
      ERROR("AMImageQuality::set_local_exposure, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_auto_local_exposure_mode(*local_exposure)
        < 0) {
      ERROR("AMImageQuality::set_local_exposure error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.local_exposure = *local_exposure;
  } while (0);

  return result;
}

bool AMImageQuality::set_dc_iris_enable(AM_DC_IRIS_MODE *dc_iris_enable)
{
  bool result = true;
  do {
    if (!dc_iris_enable) {
      ERROR("AMImageQuality::set_dc_iris_enable, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_is_dc_iris_supported()) {
      if (mw_enable_dc_iris_control((int32_t) *dc_iris_enable) < 0) {
        ERROR("AMImageQuality::set_dc_iris_enable error\n");
        result = false;
        break;
      }
    }
    m_iq_config->m_loaded.ae.dc_iris_enable = *dc_iris_enable;
  } while (0);

  return result;
}

bool AMImageQuality::set_sensor_gain_max(uint32_t *sensor_gain_max)
{
  bool result = true;
  do {
    if (!sensor_gain_max) {
      ERROR("AMImageQuality::set_sensor_gain_max, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.sensor_gain_max = *sensor_gain_max;
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_sensor_gain_max error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.sensor_gain_max = *sensor_gain_max;
  } while (0);

  return result;
}

bool AMImageQuality::set_sensor_shutter_min(uint32_t *sensor_shutter_min)
{
  bool result = true;
  do {
    if (!sensor_shutter_min) {
      ERROR("AMImageQuality::set_sensor_shutter_min, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.shutter_time_min = ROUND_DIV(512000000, *sensor_shutter_min);
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_sensor_shutter_min error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.sensor_shutter_min = *sensor_shutter_min;
  } while (0);

  return result;
}

bool AMImageQuality::set_sensor_shutter_max(uint32_t *sensor_shutter_max)
{
  bool result = true;
  do {
    if (!sensor_shutter_max) {
      ERROR("AMImageQuality::set_sensor_shutter_max, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.shutter_time_max = ROUND_DIV(512000000, *sensor_shutter_max);
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_sensor_shutter_max error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.sensor_shutter_max = *sensor_shutter_max;
  } while (0);

  return result;
}

bool AMImageQuality::set_ir_led_mode(AM_IR_LED_MODE *ir_led_mode)
{
  bool result = true;
  do {
    if (!ir_led_mode) {
      ERROR("AMImageQuality::set_ir_led_mode, NULL pointer\n");
      result = false;
      break;
    }
    mw_ae_param ae_param;
    mw_get_ae_param(&ae_param);
    ae_param.ir_led_mode = (uint32_t) *ir_led_mode;
    if (mw_set_ae_param(&ae_param) < 0) {
      ERROR("AMImageQuality::set_ir_led_mode error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.ir_led_mode = *ir_led_mode;
  } while (0);

  return result;
}

bool AMImageQuality::set_shutter_time(int32_t *shutter_time)
{
  bool result = true;
  do {
    if (!shutter_time) {
      ERROR("AMImageQuality::set_shutter_time, NULL pointer\n");
      result = false;
      break;
    }
    AM_AE_MODE ae_enable;
    result = get_ae_enable(&ae_enable);
    if (!result) {
      ERROR("AMImageQuality::set_shutter_time error, get_ae_enable failed\n");
      break;
    }

    if (ae_enable == AM_AE_ENABLE) {
      ERROR("AMImageQuality::set_shutter_time,"
            " can not set shutter time when AE is running, disable AE first\n");
      result = false;
      break;
    }
    int32_t st = ROUND_DIV(512000000, *shutter_time);
    if (mw_set_shutter_time(m_fd_iav, &st) < 0) {
      ERROR("AMImageQuality::set_shutter_time set failed!\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.shutter_time = *shutter_time;
  } while (0);

  return result;
}

bool AMImageQuality::set_sensor_gain(int32_t *sensor_gain)
{
  bool result = true;
  do {
    if (!sensor_gain) {
      ERROR("AMImageQuality::set_sensor_gain, NULL pointer\n");
      result = false;
      break;
    }
    AM_AE_MODE ae_enable;
    result = get_ae_enable(&ae_enable);
    if (!result) {
      ERROR("AMImageQuality::set_sensor_gain error, get_ae_enable failed\n");
      break;
    }

    if (ae_enable == AM_AE_ENABLE) {
      ERROR("AMImageQuality::set_sensor_gain,"
            " can not set sensor gain when AE is running, disable AE first\n");
      result = false;
      break;
    }
    int32_t gain = *sensor_gain << 24;
    if (mw_set_sensor_gain(m_fd_iav, &gain) < 0) {
      ERROR("AMImageQuality::set_sensor_gain set failed!\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.sensor_gain = *sensor_gain;
  } while (0);

  return result;
}

bool AMImageQuality::set_ae_enable(AM_AE_MODE *ae_enable)
{
  bool result = true;
  do {
    if (!ae_enable) {
      ERROR("AMImageQuality::set_ae_enable, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_enable_ae((int32_t) *ae_enable) < 0) {
      ERROR("AMImageQuality::set_ae_enable error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.ae.ae_enable = *ae_enable;
  } while (0);

  return result;
}

/*AWB config*/
bool AMImageQuality::set_wb_mode(AM_WB_MODE *wb_mode)
{
  bool result = true;
  do {
    if (!wb_mode) {
      ERROR("AMImageQuality::set_wb_mode, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_white_balance_mode((mw_white_balance_mode) *wb_mode) < 0) {
      ERROR("AMImageQuality::set_wb_mode error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.awb.wb_mode = *wb_mode;
  } while (0);

  return result;
}

/*NF config*/
bool AMImageQuality::set_mctf_strength(uint32_t *mctf_strength)
{
  bool result = true;
  do {
    if (!mctf_strength) {
      ERROR("AMImageQuality::set_mctf_strength, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_set_mctf_strength(*mctf_strength) < 0) {
      ERROR("AMImageQuality::set_mctf_strength error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.nf.mctf_strength = *mctf_strength;
  } while (0);

  return result;
}

/*IQ style config*/
bool AMImageQuality::set_saturation(int32_t *saturation)
{
  bool result = true;
  do {
    if (!saturation) {
      ERROR("AMImageQuality::set_saturation, NULL pointer\n");
      result = false;
      break;
    }
    mw_image_param style_params;
    mw_get_image_param(&style_params);
    style_params.saturation = *saturation;
    if (mw_set_image_param(&style_params) < 0) {
      ERROR("AMImageQuality::set_saturation error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.style.saturation = *saturation;
  } while (0);

  return result;
}

bool AMImageQuality::set_brightness(int32_t *brightness)
{
  bool result = true;
  do {
    if (sys_info.res.hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
      WARN("Can not set brightness in HDR mode\n");
      break;
    }
    if (!brightness) {
      ERROR("AMImageQuality::set_brightness, NULL pointer\n");
      result = false;
      break;
    }
    mw_image_param style_params;
    mw_get_image_param(&style_params);
    style_params.brightness = *brightness;
    if (mw_set_image_param(&style_params) < 0) {
      ERROR("AMImageQuality::set_brightness error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.style.brightness = *brightness;
  } while (0);

  return result;
}

bool AMImageQuality::set_hue(int32_t *hue)
{
  bool result = true;
  do {
    if (!hue) {
      ERROR("AMImageQuality::set_hue, NULL pointer\n");
      result = false;
      break;
    }
    mw_image_param style_params;
    mw_get_image_param(&style_params);
    style_params.hue = *hue;
    if (mw_set_image_param(&style_params) < 0) {
      ERROR("AMImageQuality::set_hue error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.style.hue = *hue;
  } while (0);

  return result;
}

bool AMImageQuality::set_contrast(int32_t *contrast)
{
  bool result = true;
  do {
    if (!contrast) {
      ERROR("AMImageQuality::set_contrast, NULL pointer\n");
      result = false;
      break;
    }
    mw_image_param style_params;
    mw_get_image_param(&style_params);
    style_params.contrast = *contrast;
    if (mw_set_image_param(&style_params) < 0) {
      ERROR("AMImageQuality::set_contrast error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.style.contrast = *contrast;
  } while (0);

  return result;
}

bool AMImageQuality::set_sharpness(int32_t *sharpness)
{
  bool result = true;
  do {
    if (!sharpness) {
      ERROR("AMImageQuality::set_sharpness, NULL pointer\n");
      result = false;
      break;
    }
    mw_image_param style_params;
    mw_get_image_param(&style_params);
    style_params.sharpness = *sharpness;
    if (mw_set_image_param(&style_params) < 0) {
      ERROR("AMImageQuality::set_sharpness error\n");
      result = false;
      break;
    }
    m_iq_config->m_loaded.style.sharpness = *sharpness;
  } while (0);

  return result;
}

bool AMImageQuality::set_auto_contrast_mode(int32_t *ac_mode)
{
  bool result = true;
  do {
    if (!ac_mode) {
      ERROR("AMImageQuality::set_auto_contrast_mode, NULL pointer\n");
      result = false;
      break;
    }
    uint32_t mode = *ac_mode;
    if (mode == 0) {
      if (mw_set_auto_color_contrast(0) < 0) {
        ERROR("AMImageQuality::mw_set_auto_color_contrast error\n");
        result = false;
        break;
      }
    } else {
      if (mw_set_auto_color_contrast(1) < 0) {
        ERROR("AMImageQuality::mw_set_auto_color_contrast error\n");
        result = false;
        break;
      }
       if (mw_set_auto_color_contrast_strength(mode) < 0) {
         ERROR("AMImageQuality::mw_set_auto_color_contrast_strength error\n");
         result = false;
         break;
       }
    }
    m_iq_config->m_loaded.style.auto_contrast_mode = mode;
  } while (0);

  return result;
}

/*log level config*/
bool AMImageQuality::get_log_level(AM_IQ_LOG_LEVEL *log_level)
{
  bool result = true;
  do {
    if (!log_level) {
      ERROR("AMImageQuality::get_log_level, NULL pointer\n");
      result = false;
      break;
    }
    *log_level = m_iq_config->m_loaded.log_level;
  } while (0);

  return result;
}

/*AE config*/
bool AMImageQuality::get_ae_metering_mode(AM_AE_METERING_MODE *ae_metering_mode)
{
  bool result = true;
  do {
    if (!ae_metering_mode) {
      ERROR("AMImageQuality::get_ae_metering_mode, NULL pointer\n");
      result = false;
      break;
    }
    *ae_metering_mode = m_iq_config->m_loaded.ae.ae_metering_mode;
  } while (0);

  return result;
}

bool AMImageQuality::get_day_night_mode(AM_DAY_NIGHT_MODE *day_night_mode)
{
  bool result = true;
  do {
    if (!day_night_mode) {
      ERROR("AMImageQuality::get_day_night_mode, NULL pointer\n");
      result = false;
      break;
    }
    *day_night_mode = m_iq_config->m_loaded.ae.day_night_mode;
  } while (0);

  return result;
}

bool AMImageQuality::get_slow_shutter_mode(AM_SLOW_SHUTTER_MODE *slow_shutter_mode)
{
  bool result = true;
  do {
    if (!slow_shutter_mode) {
      ERROR("AMImageQuality::get_slow_shutter_mode, NULL pointer\n");
      result = false;
      break;
    }
    *slow_shutter_mode = m_iq_config->m_loaded.ae.slow_shutter_mode;
  } while (0);

  return result;
}

bool AMImageQuality::get_anti_flick_mode(AM_ANTI_FLICK_MODE *anti_flicker_mode)
{
  bool result = true;
  do {
    if (!anti_flicker_mode) {
      ERROR("AMImageQuality::get_anti_flick_mode, NULL pointer\n");
      result = false;
      break;
    }
    *anti_flicker_mode = m_iq_config->m_loaded.ae.anti_flicker_mode;
  } while (0);

  return result;
}

bool AMImageQuality::get_ae_target_ratio(int32_t *ae_target_ratio)
{
  bool result = true;
  do {
    if (!ae_target_ratio) {
      ERROR("AMImageQuality::get_ae_target_ratio, NULL pointer\n");
      result = false;
      break;
    }
    *ae_target_ratio = m_iq_config->m_loaded.ae.ae_target_ratio;
  } while (0);

  return result;
}

bool AMImageQuality::get_backlight_comp_enable(AM_BACKLIGHT_COMP_MODE *backlight_comp_enable)
{
  bool result = true;
  do {
    if (!backlight_comp_enable) {
      ERROR("AMImageQuality::get_backlight_comp_enable, NULL pointer\n");
      result = false;
      break;
    }
    *backlight_comp_enable = m_iq_config->m_loaded.ae.backlight_comp_enable;
  } while (0);

  return result;
}

bool AMImageQuality::get_local_exposure(uint32_t *local_exposure)
{
  bool result = true;
  do {
    if (!local_exposure) {
      ERROR("AMImageQuality::get_local_exposure, NULL pointer\n");
      result = false;
      break;
    }
    *local_exposure = m_iq_config->m_loaded.ae.local_exposure;
  } while (0);

  return result;
}

bool AMImageQuality::get_dc_iris_enable(AM_DC_IRIS_MODE *dc_iris_enable)
{
  bool result = true;
  do {
    if (!dc_iris_enable) {
      ERROR("AMImageQuality::get_dc_iris_enable, NULL pointer\n");
      result = false;
      break;
    }
    *dc_iris_enable = m_iq_config->m_loaded.ae.dc_iris_enable;
  } while (0);

  return result;
}

bool AMImageQuality::get_sensor_gain_max(uint32_t *sensor_gain_max)
{
  bool result = true;
  do {
    if (!sensor_gain_max) {
      ERROR("AMImageQuality::get_sensor_gain_max, NULL pointer\n");
      result = false;
      break;
    }
    *sensor_gain_max = m_iq_config->m_loaded.ae.sensor_gain_max;
  } while (0);

  return result;
}

bool AMImageQuality::get_sensor_shutter_min(uint32_t *sensor_shutter_min)
{
  bool result = true;
  do {
    if (!sensor_shutter_min) {
      ERROR("AMImageQuality::get_sensor_shutter_min, NULL pointer\n");
      result = false;
      break;
    }
    *sensor_shutter_min = m_iq_config->m_loaded.ae.sensor_shutter_min;
  } while (0);

  return result;
}

bool AMImageQuality::get_sensor_shutter_max(uint32_t *sensor_shutter_max)
{
  bool result = true;
  do {
    if (!sensor_shutter_max) {
      ERROR("AMImageQuality::sensor_shutter_max, NULL pointer\n");
      result = false;
      break;
    }
    *sensor_shutter_max = m_iq_config->m_loaded.ae.sensor_shutter_max;
  } while (0);

  return result;
}

bool AMImageQuality::get_ir_led_mode(AM_IR_LED_MODE *ir_led_mode)
{
  bool result = true;
  do {
    if (!ir_led_mode) {
      ERROR("AMImageQuality::get_ir_led_mode, NULL pointer\n");
      result = false;
      break;
    }
    *ir_led_mode = m_iq_config->m_loaded.ae.ir_led_mode;
  } while (0);

  return result;
}

bool AMImageQuality::get_shutter_time(int32_t *shutter_time)
{
  bool result = true;
  do {
    if (!shutter_time) {
      ERROR("AMImageQuality::get_shutter_time, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_get_shutter_time(m_fd_iav, shutter_time) < 0) {
      ERROR("AMImageQuality::get_shutter_time error\n");
      result = false;
      break;
    }
    *shutter_time = ROUND_DIV(512000000, *shutter_time);
  } while (0);

  return result;
}

bool AMImageQuality::get_sensor_gain(int32_t *sensor_gain)
{
  bool result = true;
  do {
    if (!sensor_gain) {
      ERROR("AMImageQuality::get_sensor_gain, NULL pointer\n");
      result = false;
      break;
    }
    if (mw_get_sensor_gain(m_fd_iav, sensor_gain) < 0) {
      ERROR("AMImageQuality::get_sensor_gain error\n");
      result = false;
      break;
    }
    *sensor_gain = *sensor_gain >> 24;
  } while (0);

  return result;
}

bool AMImageQuality::get_luma(AMAELuma *luma)
{
  bool result = true;
  do {
    if (!luma) {
      ERROR("AMImageQuality::get_luma, NULL pointer\n");
      result = false;
      break;
    }
    mw_luma_value mw_luma;
    if (mw_get_ae_luma_value(&mw_luma) < 0) {
      ERROR("AMImageQuality::get_ae_luma_value error\n");
      result = false;
      break;
    }
    luma->cfa_luma = mw_luma.cfa_luma;
    luma->rgb_luma = mw_luma.rgb_luma;

  } while (0);

  return result;
}

bool AMImageQuality::get_ae_enable(AM_AE_MODE *ae_enable)
{
  bool result = true;
  do {
    if (!ae_enable) {
      ERROR("AMImageQuality::get_ae_enable, NULL pointer\n");
      result = false;
      break;
    }
    *ae_enable = m_iq_config->m_loaded.ae.ae_enable;
  } while (0);

  return result;
}

/*AWB config*/
bool AMImageQuality::get_wb_mode(AM_WB_MODE *wb_mode)
{
  bool result = true;
  do {
    if (!wb_mode) {
      ERROR("AMImageQuality::get_wb_mode, NULL pointer\n");
      result = false;
      break;
    }
    *wb_mode = m_iq_config->m_loaded.awb.wb_mode;
  } while (0);

  return result;
}

/*NF config*/
bool AMImageQuality::get_mctf_strength(uint32_t *mctf_strength)
{
  bool result = true;
  do {
    if (!mctf_strength) {
      ERROR("AMImageQuality::get_mctf_strength, NULL pointer\n");
      result = false;
      break;
    }
    *mctf_strength = m_iq_config->m_loaded.nf.mctf_strength;
  } while (0);

  return result;
}

/*IQ style config*/
bool AMImageQuality::get_saturation(int32_t *saturation)
{
  bool result = true;
  do {
    if (!saturation) {
      ERROR("AMImageQuality::get_saturation, NULL pointer\n");
      result = false;
      break;
    }
    *saturation = m_iq_config->m_loaded.style.saturation;
  } while (0);

  return result;
}

bool AMImageQuality::get_brightness(int32_t *brightness)
{
  bool result = true;
  do {
    if (!brightness) {
      ERROR("AMImageQuality::get_brightness, NULL pointer\n");
      result = false;
      break;
    }
    *brightness = m_iq_config->m_loaded.style.brightness;
  } while (0);
  return result;
}

bool AMImageQuality::get_hue(int32_t *hue)
{
  bool result = true;
  do {
    if (!hue) {
      ERROR("AMImageQuality::get_hue, NULL pointer\n");
      result = false;
      break;
    }
    *hue = m_iq_config->m_loaded.style.hue;
  } while (0);
  return result;
}

bool AMImageQuality::get_contrast(int32_t *contrast)
{
  bool result = true;
  do {
    if (!contrast) {
      ERROR("AMImageQuality::get_contrast, NULL pointer\n");
      result = false;
      break;
    }
    *contrast = m_iq_config->m_loaded.style.contrast;
  } while (0);
  return result;
}

bool AMImageQuality::get_sharpness(int32_t *sharpness)
{
  bool result = true;
  do {
    if (!sharpness) {
      ERROR("AMImageQuality::get_sharpness, NULL pointer\n");
      result = false;
      break;
    }
    *sharpness = m_iq_config->m_loaded.style.sharpness;
  } while (0);
  return result;
}

bool AMImageQuality::get_auto_contrast_mode(int32_t *ac_mode)
{
  bool result = true;
  do {
    if (!ac_mode) {
      ERROR("AMImageQuality::get_auto_contrast_mode, NULL pointer\n");
      result = false;
      break;
    }
    *ac_mode = m_iq_config->m_loaded.style.auto_contrast_mode;
  } while (0);
  return result;
}

/*load bin*/
bool AMImageQuality::load_adj_bin(const char *file_name)
{
  bool result = true;
  mw_adj_file_param contents;

  do {
    uint32_t file_name_len = strlen(file_name);

    memset(&contents, 0, sizeof(contents));

    if (file_name_len > 0 && file_name_len < FILE_NAME_LENGTH) {
      strncpy(contents.filename, file_name, file_name_len);
      contents.filename[file_name_len] = '\0';
    } else {
      ERROR("Invalid length of %d,"
            "please input file length [0,%d]",
            file_name_len, FILE_NAME_LENGTH - 1);
      result = false;
      break;
    }

    contents.flag[ADJ_AE_TARGET].apply = 1;
    contents.flag[ADJ_BLC].apply = 1;

    if (mw_load_adj_param(&contents) < 0) {
      ERROR("mw_reload_adj_param error");
      result = false;
      break;
    }

    if (result == true) {
      if (contents.flag[ADJ_AE_TARGET].done) {
        NOTICE("Load ADJ_AE_TARGET success");
      }

      if (contents.flag[ADJ_BLC].done) {
        NOTICE("Load ADJ_BLC success");
      }
    }

  } while(0);

  return result;
}

bool AMImageQuality::need_notify()
{
  return m_iq_config? m_iq_config->m_loaded.notify_3A_to_media_svc: false;
}
