/**
 * am_vin.cpp
 *
 *  History:
 *    Jul 15, 2015 - [Shupeng Ren] created file
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
#include "am_define.h"
#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_encode_config.h"
#include "am_video_utility.h"
#include "am_vin.h"
#include "am_thread.h"

AMVin* AMVin::create(AM_VIN_ID id)
{
  AMVin *result = new AMVin();
  if (result && (AM_RESULT_OK != result->init(id))) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMVin::destroy()
{
  delete this;
}

AM_RESULT AMVin::init(AM_VIN_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_id = id;
    if (!(m_config = AMVinConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMVinConfig!");
      break;
    }

    if (!(m_sensor = AMSensor::create(id))) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMSensor!");
      break;
    }

    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }

    if (AM_UNLIKELY(sem_init(&m_vin_sync_sem, 0, 0) < 0)) {
      PERROR("sem_init");
      result = AM_RESULT_ERR_PERM;
      break;
    } else {
      m_vin_sync_sem_inited = true;
    }

    m_vin_sync_thread = AMThread::create("VIN.sync", static_vin_sync, this);
    if (AM_UNLIKELY(!m_vin_sync_thread)) {
      ERROR("Failed to create VIN sync thread!");
      result = AM_RESULT_ERR_PERM;
      break;
    }
  } while (0);
  return result;
}

AMVin::AMVin():
    m_sensor(nullptr),
    m_sensor_param(nullptr),
    m_vin_sync_thread(nullptr),
    m_id(AM_VIN_ID_INVALID),
    m_state(AM_VIN_STATE_NOT_INITIALIZED),
    m_hdr_type(AM_HDR_SINGLE_EXPOSURE),
    m_hdr_type_changed(false),
    m_vin_sync_run(true),
    m_vin_sync_sem_inited(false),
    m_vin_sync_waiters_num(0),
    m_config(nullptr),
    m_platform(nullptr)
{
  m_size = {0};
  m_param.clear();
  DEBUG("AMVin is created!");
}

AMVin::~AMVin()
{
  m_vin_sync_run = false;
  AM_DESTROY(m_vin_sync_thread);
  if (AM_LIKELY(m_vin_sync_sem_inited)) {
    if (AM_UNLIKELY(sem_destroy(&m_vin_sync_sem) < 0)) {
      PERROR("sem_destroy");
    }
    m_vin_sync_sem_inited = false;
  }
  AM_DESTROY(m_sensor);
  m_platform = nullptr;
  m_config = nullptr;
  DEBUG("~AMVin");
}

bool AMVin::is_close_vin_needed()
{
  return m_param.at(m_id).close_on_idle.second || m_hdr_type_changed;
}

AM_RESULT AMVin::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->get_config(m_param)) != AM_RESULT_OK) {
      ERROR("Failed to get vin config!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVin::get_param(AMVinParamMap &param)
{
  param = m_param;
  return AM_RESULT_OK;
}

AM_RESULT AMVin::set_param(const AMVinParamMap &param)
{
  m_param = param;
  return AM_RESULT_OK;
}

AM_VIN_ID AMVin::id_get()
{
  return m_id;
}

AM_RESULT AMVin::start()
{
  AM_RESULT result = AM_RESULT_ERR_INVALID;

  do {
    if (AM_LIKELY(!m_sensor_param)) {
      m_sensor_param = m_sensor->get_sensor_config_parameters();
      if (AM_UNLIKELY(!m_sensor_param)) {
        result = AM_RESULT_ERR_MEM;
        ERROR("Failed to get sensor config parameters!");
        break;
      }
    }

    AMSensor::AMSensorCapability &sensor_def_cap =
        m_sensor_param->cap[m_sensor_param->index];
    AM_HDR_TYPE hdr_type;
    if ((result = m_platform->hdr_type_get(hdr_type)) != AM_RESULT_OK) {
      break;
    }
    if (m_hdr_type != hdr_type) {
      m_hdr_type = hdr_type;
      m_hdr_type_changed = true;
    }

    if (AM_LIKELY((m_param[m_id].fps.second == AM_VIDEO_FPS_AUTO) &&
                  (m_param[m_id].mode.second == AM_VIN_MODE_AUTO))) {
      if (AM_LIKELY(m_hdr_type == sensor_def_cap.hdr)) {
        NOTICE("Use default sensor mode: index[%u] specified in %s config!",
               m_sensor_param->index,
               m_sensor_param->name.c_str());
        m_param[m_id].fps.second = sensor_def_cap.fps;
        m_param[m_id].mode.second = sensor_def_cap.mode;
        result = AM_RESULT_OK;
      } else {
        NOTICE("\nSpecified default sensor mode: index[%u]"
               "(mode: %s, FPS: %s, HDR: %s) in %s config\n"
               "doesn't support HDR type %s, try to find one!",
               m_sensor_param->index,
               AMVinTrans::mode_enum_to_str(sensor_def_cap.mode).c_str(),
               AMVideoTrans::fps_enum_to_str(sensor_def_cap.fps).c_str(),
               AMVinTrans::hdr_enum_to_str(sensor_def_cap.hdr).c_str(),
               m_sensor_param->name.c_str(),
               AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str());
        for (uint32_t i = 0; i < m_sensor_param->cap.size(); ++ i) {
          AMSensor::AMSensorCapability &v = m_sensor_param->cap[i];
          if (AM_LIKELY(m_hdr_type == v.hdr)) {
            NOTICE("Select mode: index[%u] in %s config!",
                   i, m_sensor_param->name.c_str());
            m_param[m_id].fps.second = v.fps;
            m_param[m_id].mode.second = v.mode;
            result = AM_RESULT_OK;
            break;
          }
        }
        if (AM_UNLIKELY(AM_RESULT_OK != result)) {
          ERROR("Failed to find suitable mode from %s config!",
                m_sensor_param->name.c_str());
          break;
        }
      }
    } else if (AM_LIKELY(m_param[m_id].fps.second == AM_VIDEO_FPS_AUTO)) {
      if (AM_LIKELY((m_hdr_type == sensor_def_cap.hdr) &&
                    (m_param[m_id].mode.second == sensor_def_cap.mode))) {
        NOTICE("Use default sensor mode: index[%u] specified in %s config!",
               m_sensor_param->index,
               m_sensor_param->name.c_str());
        m_param[m_id].fps.second = sensor_def_cap.fps;
        result = AM_RESULT_OK;
      } else {
        NOTICE("\nSpecified default sensor mode: index[%u]"
               "(mode: %s, FPS: %s, HDR: %s) in %s config\n"
               "doesn't support mode: %s and HDR: %s, try to find one!",
               m_sensor_param->index,
               AMVinTrans::mode_enum_to_str(sensor_def_cap.mode).c_str(),
               AMVideoTrans::fps_enum_to_str(sensor_def_cap.fps).c_str(),
               AMVinTrans::hdr_enum_to_str(sensor_def_cap.hdr).c_str(),
               m_sensor_param->name.c_str(),
               AMVinTrans::mode_enum_to_str(m_param[m_id].mode.second).c_str(),
               AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str());
        for (uint32_t i = 0; i < m_sensor_param->cap.size(); ++ i) {
          AMSensor::AMSensorCapability &v = m_sensor_param->cap[i];
          if (AM_LIKELY((m_hdr_type == v.hdr) &&
                        (m_param[m_id].mode.second == v.mode))) {
            NOTICE("Select mode: index[%u] in %s config!",
                   i, m_sensor_param->name.c_str());
            m_param[m_id].fps.second = v.fps;
            result = AM_RESULT_OK;
            break;
          }
        }
        if (AM_UNLIKELY(AM_RESULT_OK != result)) {
          ERROR("Failed to find suitable mode from %s config!",
                m_sensor_param->name.c_str());
          break;
        }
      }
    } else if (AM_LIKELY(m_param[m_id].mode.second == AM_VIN_MODE_AUTO)) {
      if (AM_LIKELY((m_hdr_type == sensor_def_cap.hdr) &&
                    (m_param[m_id].fps.second == sensor_def_cap.fps))) {
        NOTICE("Use default sensor mode: index[%u] specified in %s config!",
               m_sensor_param->index,
               m_sensor_param->name.c_str());
        m_param[m_id].mode.second = sensor_def_cap.mode;
        result = AM_RESULT_OK;
      } else {
        NOTICE("\nSpecified default sensor mode: index[%u]"
               "(mode: %s, FPS: %s, HDR: %s) in %s config\n"
               "doesn't support FPS: %s and HDR: %s, try to find one!",
               m_sensor_param->index,
               AMVinTrans::mode_enum_to_str(sensor_def_cap.mode).c_str(),
               AMVideoTrans::fps_enum_to_str(sensor_def_cap.fps).c_str(),
               AMVinTrans::hdr_enum_to_str(sensor_def_cap.hdr).c_str(),
               m_sensor_param->name.c_str(),
               AMVideoTrans::fps_enum_to_str(m_param[m_id].fps.second).c_str(),
               AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str());
        for (uint32_t i = 0; i < m_sensor_param->cap.size(); ++ i) {
          AMSensor::AMSensorCapability &v = m_sensor_param->cap[i];
          float a = std::stof(AMVideoTrans::fps_enum_to_str(m_param[m_id].
                                                            fps.second));
          float b = std::stof(AMVideoTrans::fps_enum_to_str(v.fps));
          if (AM_LIKELY((m_hdr_type == v.hdr) && (a <= b))) {
            NOTICE("Select mode: index[%u] in %s config!",
                   i, m_sensor_param->name.c_str());
            m_param[m_id].mode.second = v.mode;
            result = AM_RESULT_OK;
            break;
          }
        }
        if (AM_UNLIKELY(AM_RESULT_OK != result)) {
          ERROR("Failed to find suitable mode from %s config!",
                m_sensor_param->name.c_str());
          break;
        }
      }
    } else {
      for (uint32_t i = 0; i < m_sensor_param->cap.size(); ++ i) {
        AMSensor::AMSensorCapability &v = m_sensor_param->cap[i];
        float a = std::stof(AMVideoTrans::fps_enum_to_str(m_param[m_id].
                                                          fps.second));
        float b = std::stof(AMVideoTrans::fps_enum_to_str(v.fps));
        if (AM_LIKELY((m_param[m_id].mode.second == v.mode) &&
                      (m_hdr_type == v.hdr) &&
                      (a <= b))) {
          NOTICE("Select mode: index[%u](mode: %s, FPS: %s, HDR: %s) "
                 "in %s config!",
                 i, AMVinTrans::mode_enum_to_str(v.mode).c_str(),
                 AMVideoTrans::fps_enum_to_str(v.fps).c_str(),
                 AMVinTrans::hdr_enum_to_str(v.hdr).c_str(),
                 m_sensor_param->name.c_str());
          NOTICE("VIN[%u] config requested mode: %s, FPS: %s",
                 m_id,
                 AMVinTrans::mode_enum_to_str(m_param[m_id].
                                              mode.second).c_str(),
                 AMVideoTrans::fps_enum_to_str(m_param[m_id].
                                               fps.second).c_str());
          NOTICE("Feature requested HDR: %s",
                 AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str());
          result = AM_RESULT_OK;
          break;
        }
      }
      if (result != AM_RESULT_OK) {
        ERROR("VIN[%d] config is invalid: mode: %s, HDR type: %s, FPS: %s!",
              m_id,
              AMVinTrans::mode_enum_to_str(m_param[m_id].mode.second).c_str(),
              AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str(),
              AMVideoTrans::fps_enum_to_str(m_param[m_id].fps.second).c_str());
        ERROR("VIN[%d](Sensor %s) supported mode list:",
              m_id, m_sensor_param->name.c_str());
        for (auto &v: m_sensor_param->cap) {
          ERROR("Mode: %s, HDR Type: %s, FPS: %s",
                AMVinTrans::mode_enum_to_str(v.mode).c_str(),
                AMVinTrans::hdr_enum_to_str(v.hdr).c_str(),
                AMVideoTrans::fps_enum_to_str(v.fps).c_str());
        }
        break;
      }
    }
    INFO("VIN[%d](Sensor %s) selects mode: %s, FPS: %s, HDR: %s",
         m_id, m_sensor_param->name.c_str(),
         AMVinTrans::mode_enum_to_str(m_param[m_id].mode.second).c_str(),
         AMVideoTrans::fps_enum_to_str(m_param[m_id].fps.second).c_str(),
         AMVinTrans::hdr_enum_to_str(m_hdr_type).c_str());

    if (AM_UNLIKELY((result = set_mode()) != AM_RESULT_OK)) {
      ERROR("Failed to set VIN[%d] mode!", m_id);
      break;
    }
    if (AM_UNLIKELY((result = set_fps()) != AM_RESULT_OK)) {
      ERROR("Failed to set VIN[%d] FPS!", m_id);
      break;
    }
    if (AM_UNLIKELY((result = set_flip()) != AM_RESULT_OK)) {
      ERROR("Failed to set VIN[%d] FLIP!", m_id);
      break;
    }
    m_state = AM_VIN_STATE_RUNNING;
    INFO("Start sensor: %s done!", m_sensor_param->name.c_str());
  } while (0);

  return result;
}

AM_RESULT AMVin::stop()
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformVinMode mode;

  do {
    mode.id = m_id;
    mode.mode = AM_VIN_MODE_OFF;
    mode.hdr_type = AM_HDR_SINGLE_EXPOSURE;
    if ((result = m_platform->vin_mode_set(mode)) != AM_RESULT_OK) {
      break;
    }
    m_state = AM_VIN_STATE_STOPPED;
  } while (0);

  return result;
}

AM_RESULT AMVin::set_mode()
{
  AM_RESULT result = AM_RESULT_ERR_INVALID;
  AMPlatformVinMode mode;
  AMPlatformVinInfo info;

  do {
    info.id = m_id;
    if ((m_state == AM_VIN_STATE_RUNNING) &&
        (result = m_platform->vin_info_get(info)) != AM_RESULT_OK) {
      break;
    }

    if ((m_state != AM_VIN_STATE_RUNNING) ||
        (m_param[m_id].mode.second != info.mode) ||
        (m_hdr_type != info.hdr_type)) {
      mode.id = m_id;
      mode.mode = m_param[m_id].mode.second;
      mode.hdr_type = m_hdr_type;
      if ((result = m_platform->vin_mode_set(mode)) != AM_RESULT_OK) {
        break;
      }

      info.mode = AM_VIN_MODE_CURRENT;
      if ((result = m_platform->vin_info_get(info)) != AM_RESULT_OK) {
        break;
      }

      m_size = info.size;
      INFO("Set [sensor: %s] mode(%s) HDR(%s) done!",
           m_sensor_param->name.c_str(),
           AMVinTrans::mode_enum_to_str(info.mode).c_str(),
           AMVinTrans::hdr_enum_to_str(info.hdr_type).c_str());
    } else {
      INFO("[sensor: %s] is already running in mode(%s) HDR(%s)!",
           m_sensor_param->name.c_str(),
           AMVinTrans::mode_enum_to_str(info.mode).c_str(),
           AMVinTrans::hdr_enum_to_str(info.hdr_type).c_str());
    }
  } while (0);

  return result;
}

AM_RESULT AMVin::set_fps()
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformVinFps fps;

  do {
    fps.id = m_id;
    fps.fps = m_param[m_id].fps.second;
    if ((result = m_platform->vin_fps_set(fps)) != AM_RESULT_OK) {
      break;
    }
    INFO("Set [sensor: %s] FPS(%s) done!",
         m_sensor_param->name.c_str(),
         AMVideoTrans::fps_enum_to_str(fps.fps).c_str());
  } while (0);
  return result;
}

AM_RESULT AMVin::size_get(AMResolution &size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    size = m_size;
    if (m_state != AM_VIN_STATE_RUNNING) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("VIN device is not running now!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVin::set_flip()
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformVinFlip flip;

  do {
    flip.id = m_id;
    flip.flip = m_param[m_id].flip.second;
    if ((result = m_platform->vin_flip_set(flip)) != AM_RESULT_OK) {
      break;
    }
    INFO("Set [sensor: %s] FLIP(%s) done!",
         m_sensor_param->name.c_str(),
         AMVideoTrans::flip_enum_to_str(flip.flip).c_str());
  } while (0);

  return result;
}

AM_RESULT AMVin::wait_frame_sync()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_vin_sync_run) {
      ERROR("VIN sync thread is not running!");
      result = AM_RESULT_ERR_PERM;
      break;
    }
    ++ m_vin_sync_waiters_num;
    if (sem_wait(&m_vin_sync_sem) < 0) {
      -- m_vin_sync_waiters_num;
      result = AM_RESULT_ERR_PERM;
      break;
    }
  } while(0);

  return result;
}

AM_RESULT AMVin::get_agc(AMVinAGC &agc)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformVinAGC platform_agc;
  platform_agc.id = m_id;

  result = m_platform->vin_agc_get(platform_agc);
  if (AM_LIKELY(AM_RESULT_OK == result)) {
    memcpy(&agc, &platform_agc.agc_info, sizeof(agc));
  } else {
    memset(&agc, 0, sizeof(agc));
  }

  return result;
}

void AMVin::static_vin_sync(void *data)
{
  ((AMVin*)data)->vin_sync();
}

void AMVin::vin_sync()
{
  m_vin_sync_run = true;
  while (m_vin_sync_run) {
    AM_IAV_STATE state = AM_IAV_STATE_INIT;
    if (AM_RESULT_OK != m_platform->iav_state_get(state)) {
      ERROR("Failed to get IAV state!");
      m_vin_sync_run = false;
      break;
    }
    if ((state != AM_IAV_STATE_PREVIEW) && (state != AM_IAV_STATE_ENCODING)) {
      /* IAV is not in preview or encoding state */
      usleep(10000);
      continue;
    }

    if (AM_RESULT_OK == m_platform->vin_wait_next_frame()) {
      while(m_vin_sync_waiters_num > 0) {
        if (sem_post(&m_vin_sync_sem) < 0) {
          PERROR("sem_post");
          m_vin_sync_run = false;
          break;
        }
        -- m_vin_sync_waiters_num;
      }
    } else {
      m_vin_sync_run = false;
      ERROR("Vin sync failed to wait next frame!");
      break;
    }
  }
}

//RGBSensorVin
AMRGBSensorVin* AMRGBSensorVin::create(AM_VIN_ID id)
{
  AMRGBSensorVin *result = new AMRGBSensorVin();
  if (result && (AM_RESULT_OK != result->init(id))) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMRGBSensorVin::set_mode()
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformVinShutter shutter;
  AMPlatformVinAGC agc;

  do {
    if ((result = AMVin::set_mode()) != AM_RESULT_OK) {
      break;
    }

    shutter.id = m_id;
    shutter.shutter_time.num = 1;
    shutter.shutter_time.den = 60;
    if ((result = m_platform->vin_shutter_set(shutter)) != AM_RESULT_OK) {
      break;
    }

    agc.id = m_id;
    agc.agc_info.agc = 6 << 24;
    if ((result = m_platform->vin_agc_set(agc)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

void AMRGBSensorVin::destroy()
{
  AMVin::destroy();
}

AM_RESULT AMRGBSensorVin::init(AM_VIN_ID id)
{
  return AMVin::init(id);
}

AMRGBSensorVin::AMRGBSensorVin()
{
  DEBUG("AMRGBSensorVin is created!");
}

AMRGBSensorVin::~AMRGBSensorVin()
{
  DEBUG("~AMRGBSensorVin");
}

//YUVSensorVin
AMYUVSensorVin* AMYUVSensorVin::create(AM_VIN_ID id)
{
  AMYUVSensorVin *result = new AMYUVSensorVin();
  if (result && (AM_RESULT_OK != result->init(id))) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMYUVSensorVin::destroy()
{
  AMVin::destroy();
}

AM_RESULT AMYUVSensorVin::init(AM_VIN_ID id)
{
  return AMVin::init(id);
}

AMYUVSensorVin::AMYUVSensorVin()
{
  DEBUG("AMYUVSensorVin is created!");
}

AMYUVSensorVin::~AMYUVSensorVin()
{
  DEBUG("~AMYUVSensorVin");
}

//YUVGenericVin
AMYUVGenericVin* AMYUVGenericVin::create(AM_VIN_ID id)
{
  AMYUVGenericVin *result = new AMYUVGenericVin();
  if (result && (AM_RESULT_OK != result->init(id))) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMYUVGenericVin::destroy()
{
  AMVin::destroy();
}

AM_RESULT AMYUVGenericVin::init(AM_VIN_ID id)
{
  return AMVin::init(id);
}

AMYUVGenericVin::AMYUVGenericVin():
    m_vin_source(0)
{
  INFO("AMYUVGenericVin is created!");
}

AMYUVGenericVin::~AMYUVGenericVin()
{
  INFO("AMYUVGenericVin is destroyed!");
}
