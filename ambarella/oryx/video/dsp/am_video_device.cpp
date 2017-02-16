/*******************************************************************************
 * am_video_device.cpp
 *
 * History:
 *   2014-7-14 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "am_base_include.h"
#include "am_define.h"
#include "iav_ioctl.h"
#include "am_video_device.h"
#include "am_video_param.h"
#include "am_log.h"




AMVideoDevice::AMVideoDevice() :
    m_init_ready(false),
    m_hdr_type(AM_HDR_SINGLE_EXPOSURE),
    m_dsp_state(AM_DSP_STATE_NOT_INIT),
    m_max_vin_num(AM_VIN_MAX_NUM),
    m_max_vout_num(AM_VOUT_MAX_NUM),
    m_vin_config(NULL),
    m_vout_config(NULL),
    m_iav(-1),
    m_iav_state(IAV_STATE_INIT),
    m_first_create_vin(true),
    m_first_create_vout(true)
{
  int i;
  for (i = 0; i < m_max_vin_num; ++ i) {
    m_vin[i] = NULL;
  }

  for (i = 0; i < m_max_vout_num; ++ i) {
    m_vout[i] = NULL;
  }
}

AMVideoDevice::~AMVideoDevice()
{
  INFO("AMVideoDevice:: destructor called \n");
  destroy();
}

AM_RESULT AMVideoDevice::check_init()
{
  if (m_init_ready == false) {
    ERROR("AMVideoDevice: device init not ready\n");
    return AM_RESULT_ERR_BUSY;
  } else {
    return AM_RESULT_OK;
  }
}



AM_RESULT AMVideoDevice::set_vin_hdr_type(AM_HDR_EXPOSURE_TYPE hdr_type)
{
  AM_RESULT result = AM_RESULT_OK;
  m_hdr_type = hdr_type;
  return result;
}

AM_RESULT AMVideoDevice::get_vin_hdr_type(AM_HDR_EXPOSURE_TYPE *hdr_type)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (hdr_type) {
      *hdr_type = m_hdr_type;
    }
  } while (0);
  return result;
}


AM_RESULT AMVideoDevice::init()
{

  AM_RESULT result = AM_RESULT_OK;
  //open IAV

  do {
    INFO("AMVideoDevice: init \n");
    if (m_init_ready) {
      ERROR("AMVideoDevice: already initialized, cannot reinit\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (m_iav >= 0) {
      ERROR("AMVideoDevice: iav is already opened, strange \n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    m_iav = open("/dev/iav", O_RDWR, 0);
    if (m_iav < 0) {
      PERROR("/dev/iav");
      result = AM_RESULT_ERR_IO;
      break;
    }
    m_vin_config = new AMVinConfig();
    if (AM_UNLIKELY(!m_vin_config)) {
      ERROR("Failed to create AMVinConfig!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    m_vout_config = new AMVoutConfig();
    if (AM_UNLIKELY(!m_vout_config)) {
      ERROR("Failed to create AMVoutConfig!");
      result = AM_RESULT_ERR_MEM;
      break;
    }
  } while (0);

  if (result == AM_RESULT_OK)
    m_init_ready = true;

  return result;
}

AM_RESULT AMVideoDevice::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoDevice: try load_config \n");
  do {
    INFO("AMVideoDevice: vin load_config called \n");
    result = m_vin_config->load_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: load vin config error\n");
      break;
    }

    INFO("AMVideoDevide:: vout load_config called \n");
    result = m_vout_config->load_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: load vout config error\n");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoDevice::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoDevice:: save config file \n");

  do {
    if (m_vin_config) {
      result = m_vin_config->save_config();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: save vin config error\n");
        break;
      }
    }

    if (m_vout_config) {
      result = m_vout_config->save_config();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: save vout config error\n");
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoDevice::vin_create(AMVinParamAll *vin_param)
{
  AM_RESULT result = AM_RESULT_OK;

  AMVin *vin;
  int i;

  do {
    if (!vin_param) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //VIN can be created by type m_vin_type
    for (i = 0; i < m_max_vin_num; i ++) {
      if (m_vin[i]) {
        ERROR("AMVideoDevice: Vin is not NULL, cannot create!\n");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      switch (vin_param->vin[i].type) {
        case AM_VIN_TYPE_RGB_SENSOR:
          vin = new AMRGBSensorVin;
          break;
        case AM_VIN_TYPE_YUV_SENSOR:
          vin = new AMYUVSensorVin;
          break;
        case AM_VIN_TYPE_YUV_GENERIC:
          vin = new AMYUVGenericVin;
          break;
        case AM_VIN_TYPE_NONE:
        default:
          vin = NULL;
          break;
      }

      m_vin[i] = vin;

      INFO("AMVideoDevice: call VIN init \n");
      if (m_vin[i] != NULL) {
        result = m_vin[i]->init(m_iav, i, &vin_param->vin[i]);
        m_vin[i]->set_hdr_type(m_hdr_type);
        if (result != AM_RESULT_OK) {
          ERROR("AMVideoDevice: init vin error\n");
          break;
        }
      }
    }
  } while (0);

  if (result == AM_RESULT_OK) {
    INFO("AMVideoDevice: Vin create done!\n");
  }

  return result;
}

AM_RESULT AMVideoDevice::vin_destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vin_num; i ++) {
    delete m_vin[i];
    m_vin[i] = NULL;
  }
  DEBUG("AMVideoDevice: Vin destroy done!\n");
  return result;
}

AM_RESULT AMVideoDevice::vin_change(AMVinParamAll *vin_param)
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vin_num; i ++) {
    if (m_vin[i] != NULL) {
      result = m_vin[i]->change(&vin_param->vin[i]);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update VIN error\n");
        break;
      }
    }
  }
  return result;
}

AM_RESULT AMVideoDevice::vin_setup()
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vin_num; i ++) {
    if (m_vin[i] != NULL) {
      result = m_vin[i]->update();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update VIN error\n");
        break;
      }
    }
  }
  return result;
}

AM_RESULT AMVideoDevice::vout_create(AMVoutParamAll *vout_param)
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vout_num; i ++) {
    if (vout_param->vout[i].type == AM_VOUT_TYPE_NONE) {
      m_vout[i] = NULL;
    } else {
      m_vout[i] = new AMVout;
    }

    INFO("AMVideoDevice: call VOUT init \n");
    if (m_vout[i] != NULL) {
      result = m_vout[i]->init(m_iav, i, &vout_param->vout[i]);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: vout init error \n");
        break;
      }
    }
  }

  if (result == AM_RESULT_OK) {
    INFO("AMVideoDevice: vout create done!\n");
  }
  return result;
}

AM_RESULT AMVideoDevice::vout_destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vout_num; i ++) {
    delete m_vout[i];
    m_vout[i] = NULL;
  }
  DEBUG("AMVideoDevice: Vout destroy done!\n");
  return result;
}

AM_RESULT AMVideoDevice::vout_change(AMVoutParamAll *vout_param)
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vout_num; i ++) {
    if (m_vout[i] != NULL) {
      result = m_vout[i]->change(&vout_param->vout[i]);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update VOUT error\n");
        break;
      }
    }
  }
  return result;
}

AM_RESULT AMVideoDevice::vout_setup()
{
  AM_RESULT result = AM_RESULT_OK;
  int i;
  for (i = 0; i < m_max_vout_num; i ++) {
    if (m_vout[i] != NULL) {
      result = m_vout[i]->update();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update VOUT error\n");

        break;
      }
    }
  }
  return result;
}

AM_RESULT AMVideoDevice::update()
{
  INFO("AMVideoDevice: update \n");
  return AM_RESULT_OK;
}

AM_RESULT AMVideoDevice::stop()
{
  INFO("AMVideoDevice: stop \n");
  return AM_RESULT_OK;
}

AM_RESULT AMVideoDevice::start()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //must goto idle before VideoDevice update
    result = goto_idle();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: goto idle failed \n");
      break;
    }

    if (!m_init_ready) {
      ERROR("AMVideoDevice: dev not created \n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    PRINT_CURRENT_TIME("AMVideoDevice::start() 1");
    if (m_vin_config->need_restart() || m_first_create_vin) {
      result = vin_destroy();
      if (result != AM_RESULT_OK) {
        break;
      }
      //update HDR here
      result = vin_create(&m_vin_config->m_loaded);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: create vin error\n");
        break;
      }
      PRINT_CURRENT_TIME("AMVideoDevice::start() 2");
      result = vin_setup();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: vin_setup failed \n");
        break;
      }
      m_first_create_vin = false;
    } else {

      PRINT_CURRENT_TIME("AMVideoDevice::start() 3");
      result = vin_change(&m_vin_config->m_loaded);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update vin error\n");
        break;
      }
    }
    PRINT_CURRENT_TIME("AMVideoDevice::start() 4");

    if (result == AM_RESULT_OK) {
      m_vin_config->sync();
    }

    PRINT_CURRENT_TIME("AMVideoDevice::start() 5");

    if (m_vout_config->need_restart() || m_first_create_vout) {
      result = vout_destroy();
      if (result != AM_RESULT_OK) {
        break;
      }
      result = vout_create(&m_vout_config->m_loaded);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: create vout error\n");
        break;
      }
      result = vout_setup();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: vout_setup failed \n");
        break;
      }
      m_first_create_vout = false;
      PRINT_CURRENT_TIME("AMVideoDevice::start() 6");

    } else {

      result = vout_change(&m_vout_config->m_loaded);
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoDevice: update vout error\n");
        break;
      }

      PRINT_CURRENT_TIME("AMVideoDevice::start() 7");
    }

    if (result == AM_RESULT_OK) {
      m_vout_config->sync();
    }

    PRINT_CURRENT_TIME("AMVideoDevice::start() 8");
  } while (0);
  return result;
}

AM_RESULT AMVideoDevice::destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoDevice::destroy()\n");
  do {
    if (!m_init_ready) {
      //if not init ready (or destroyed), then just returns harmlessly
      break;
    }

    //goto_idle function first
    result = goto_idle();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: goto idle failed \n");
      break;
    }
    //stop function first
    result = stop();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: stop failed \n");
      break;
    }

    //close config classes
    delete m_vin_config;
    m_vin_config = NULL;

    delete m_vout_config;
    m_vout_config = NULL;

    //delete vin and vout
    result = vin_destroy();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: vin destory failed \n");
      break;
    }

    result = vout_destroy();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: vout destory failed \n");
      break;
    }

    //close iav
    close(m_iav);
    m_init_ready = false;
  } while (0);

  return result;
}

AM_RESULT AMVideoDevice::goto_idle()
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_STATE dsp_state = get_dsp_state();
  do {
    if (dsp_state == AM_DSP_STATE_ENCODING) {
      //force stop all encoding before calling ENTER IDLE
      if (ioctl(m_iav, IAV_IOC_STOP_ENCODE, 0xFFFFFFFF) < 0) {
        ERROR("AMVideoDevice: force stop all stream encoding\n");
        result = AM_RESULT_ERR_DSP;
        break;
      }
    }

    //call enter IDLE
    if (ioctl(m_iav, IAV_IOC_ENTER_IDLE, 0) < 0) {
      ERROR("AMVideoDevice: goto_idle failed\n");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoDevice::get_version(AMVideoVersion *version)
{

  return AM_RESULT_OK;
}

AM_RESULT AMVideoDevice::update_dsp_state()
{
  int32_t state;
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (ioctl(m_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
      PERROR("IAV_IOC_GET_IAV_STATE");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    m_iav_state = state;
    //convert to AM_DSP_STATE, only AM_DSP_STATE can be understood by APP
    switch (m_iav_state) {
      case IAV_STATE_IDLE:
        m_dsp_state = AM_DSP_STATE_IDLE;
        break;
      case IAV_STATE_PREVIEW:
        m_dsp_state = AM_DSP_STATE_READY_FOR_ENCODE;
        break;
      case IAV_STATE_ENCODING:
        m_dsp_state = AM_DSP_STATE_ENCODING;
        break;
      case IAV_STATE_DECODING:
        m_dsp_state = AM_DSP_STATE_PLAYING;
        break;
      case IAV_STATE_INIT:
        m_dsp_state = AM_DSP_STATE_NOT_INIT;
        break;
      case IAV_STATE_EXITING_PREVIEW:
        m_dsp_state = AM_DSP_STATE_EXITING_ENCODING;
        break;
      default:
        m_dsp_state = AM_DSP_STATE_ERROR;
        break;
    }
  } while (0);

  return result;
}

int AMVideoDevice::get_iav()
{
  return m_iav;
}

AM_DSP_STATE AMVideoDevice::get_dsp_state()
{
  update_dsp_state();
  return m_dsp_state;
}

//assume HDMI VOUT is on now , hard code
bool AMVideoDevice::is_vout_enabled()
{

  return true;
}

AM_ENCODE_SOURCE_BUFFER_ID AMVideoDevice::get_vout_related_buffer_id()
{
  return AM_ENCODE_SOURCE_BUFFER_PREVIEW_B;
}

//FIXME:  hard code VOUT to be 480p
AMResolution AMVideoDevice::get_vout_resolution()
{
  AMResolution x = { 720, 480 };
  return x;
}

AMResolution AMVideoDevice::get_vin_resolution(AM_VIN_ID id)
{
  AMResolution size;
  m_vin[id]->get_resolution(&size);
  return size;
}

AMVinConfig* AMVideoDevice::get_vin_config()
{
  return m_vin_config;
}

AMVoutConfig* AMVideoDevice::get_vout_config()
{
  return m_vout_config;
}

AMVin* AMVideoDevice::get_primary_vin()
{
  return m_vin[0];
}
