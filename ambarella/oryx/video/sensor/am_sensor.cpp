/*******************************************************************************
 * am_sensor_ov4689.cpp
 *
 * History:
 *   May 7, 2015 - [ypchang] created file
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
#include "am_pointer.h"

#include "am_video_utility.h"
#include "am_configure.h"
#include "am_sensor.h"

#include <algorithm>

#define SENSOR_CONFIG_PATH "/etc/oryx/sensor"

AMSensor* AMSensor::create(AM_VIN_ID id)
{
  AMSensor* result = new AMSensor();
  if (result && (AM_RESULT_OK != result->init(id))) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMSensor::init(AM_VIN_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_source_id = id;
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }
  } while (0);

  return result;
}

void AMSensor::destroy()
{
  delete this;
}

AMSensor::AMSensor() :
  m_sensor_drv_parameter(nullptr),
  m_sensor_cfg_parameter(nullptr),
  m_source_id(AM_VIN_ID_INVALID)
{
  DEBUG("AMSensor is created!");
}

AMSensor::~AMSensor()
{
  delete m_sensor_drv_parameter;
  delete m_sensor_cfg_parameter;
  DEBUG("AMSensor is destroyed!");
}

AMSensor::AMSensorParameter* AMSensor::get_sensor_driver_parameters()
{
  AMSensorParameter *sensor_param = nullptr;
  AMPlatformSensorInfo sensor_info;
  AMPlatformVinInfoList vin_info_list;
  AM_RESULT result = AM_RESULT_OK;

  do {
    uint32_t vin_number = 0;
    if (AM_UNLIKELY(m_platform->vin_number_get(vin_number) != AM_RESULT_OK)) {
      ERROR("Failed to get VIN device number!");
      break;
    }
    if (AM_UNLIKELY((uint32_t)m_source_id >= vin_number)) {
      ERROR("Invalid VIN source ID %u, exceeds max number: %u!",
            m_source_id, vin_number);
      break;
    }

    sensor_info.id = AM_VIN_ID(m_source_id);
    if (m_platform->sensor_info_get(sensor_info) != AM_RESULT_OK) {
      ERROR("");
      break;
    }

    if (AM_UNLIKELY(sensor_info.dev_type != AM_VIN_TYPE_SENSOR)) {
      ERROR("Not sensor device! Only sensor is supported!");
      break;
    }

    if (AM_UNLIKELY(!m_sensor_drv_parameter)) {
      m_sensor_drv_parameter = new AMSensorParameter();
      if (AM_UNLIKELY(!m_sensor_drv_parameter)) {
        ERROR("Failed to allocate memory for sensor driver parameters!");
        break;
      }
    }
    m_sensor_drv_parameter->id = sensor_info.sensor_id;
    m_sensor_drv_parameter->name = sensor_info.name;
    m_sensor_drv_parameter->cap.clear();

    result = m_platform->vin_info_list_get(AM_VIN_ID(m_source_id),
                                           vin_info_list);
    if (AM_RESULT_OK != result) {
      ERROR("Failed to list VIN device information!");
    }

    for (size_t i = 0; i < vin_info_list.size(); ++ i) {
      AMPlatformVinInfo &info = vin_info_list.at(i);
      AMSensorCapability cap;
      cap.res.width = info.size.width;
      cap.res.height = info.size.height;
      cap.fps = info.fps;
      cap.type = info.type;
      cap.hdr = info.hdr_type;
      cap.bits = info.bits;
      m_sensor_drv_parameter->cap.push_back(cap);
    }
    sensor_param = m_sensor_drv_parameter;
  }while(0);

  return sensor_param;
}

AMSensor::AMSensorParameter* AMSensor::get_sensor_config_parameters()
{
  AMSensorParameter *sensor_param = nullptr;
  AMPlatformSensorInfo sensor_info;

  do {
    uint32_t vin_number = 0;
    if (AM_UNLIKELY(AM_RESULT_OK != m_platform->vin_number_get(vin_number))) {
      ERROR("Failed to get VIN device number!");
      break;
    }
    if (AM_UNLIKELY((uint32_t)m_source_id >= vin_number)) {
      ERROR("Invalid VIN source ID %u, exceeds max number: %u!",
            m_source_id, vin_number);
      break;
    }

    sensor_info.id = AM_VIN_ID(m_source_id);
    if (m_platform->sensor_info_get(sensor_info) != AM_RESULT_OK) {
      ERROR("");
      break;
    }

    if (AM_UNLIKELY(sensor_info.dev_type != AM_VIN_TYPE_SENSOR)) {
      ERROR("Not sensor device! Only sensor is supported!");
      break;
    }

    if (AM_UNLIKELY(!m_sensor_cfg_parameter)) {
      m_sensor_cfg_parameter = new AMSensorParameter();
      if (AM_UNLIKELY(!m_sensor_cfg_parameter)) {
        ERROR("Failed to allocate memory for sensor config parameters!");
        break;
      }
    }
    m_sensor_cfg_parameter->id = sensor_info.sensor_id;
    m_sensor_cfg_parameter->name = sensor_info.name;
    m_sensor_cfg_parameter->cap.clear();
    std::string config_name = m_sensor_cfg_parameter->name;
    std::string config = std::string(SENSOR_CONFIG_PATH) + "/";
    AMConfig *sensor_cfg = nullptr;

    std::transform(config_name.begin(), config_name.end(),
                   config_name.begin(), ::tolower);
    config.append(config_name + ".acs");

    sensor_cfg = AMConfig::create(config.c_str());
    if (AM_LIKELY(!sensor_cfg)) {
      ERROR("Failed to load sensor %s configuration file!",
            m_sensor_cfg_parameter->name.c_str());
      break;
    } else {
      bool is_ok = true;
      AMConfig &sensor = *sensor_cfg;
      uint32_t caps_size = sensor["caps"].length();
      std::string value;

      for (uint32_t i = 0; i < caps_size; ++ i) {
        AMSensorCapability cap;
        std::string type = sensor["caps"][i]["type"].get<std::string>("rgb");
        if (is_str_equal(type.c_str(), "rgb")) {
          cap.type = AM_SENSOR_TYPE_RGB;
        } else if (is_str_equal(type.c_str(), "yuv")) {
          cap.type = AM_SENSOR_TYPE_YUV;
        } else {
          ERROR("Unknown Sensor type: %s!", type.c_str());
          is_ok = false;
          break;
        }

        value = sensor["caps"][i]["fps"].get<std::string>("auto");
        cap.fps = AMVideoTrans::fps_str_to_enum(value);

        value = sensor["caps"][i]["hdr"].get<std::string>("linear");
        cap.hdr = AMVinTrans::hdr_str_to_enum(value);

        value = sensor["caps"][i]["mode"].get<std::string>("auto");
        cap.mode = AMVinTrans::mode_str_to_enum(value);
        cap.res  = AMVinTrans::mode_to_resolution(cap.mode);
        cap.bits = AMVinTrans::bits_iav_to_mw(sensor["caps"][i]["bits"].
                                              get<unsigned>(10));

        m_sensor_cfg_parameter->cap.push_back(cap);
      }
      if (AM_LIKELY(is_ok)) {
        m_sensor_cfg_parameter->index = sensor["default"].get<unsigned>();
        if (AM_UNLIKELY(m_sensor_cfg_parameter->index >= caps_size)) {
          ERROR("Invalid default index: %u!", m_sensor_cfg_parameter->index);
          break;
        }
      }
      sensor_param = m_sensor_cfg_parameter;
    }
  }while(0);

  return sensor_param;
}
