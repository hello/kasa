/*******************************************************************************
 * am_video_camera.cpp
 *
 * History:
 *   2014-8-6- [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_log.h"
#include "am_video_camera.h"
#include "am_video_normal_mode.h"
#include "am_video_dewarp_mode.h"
#include "am_video_adv_hdr_mode.h"
#include "am_video_adv_iso_mode.h"
#include "am_encode_device.h"
#include "am_encode_system_cap.h"

#include "am_define.h"



static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);

//protect video camera's API, so that only one to be called at a time,
//just to prevent the APIs are being called together without protection.

AMVideoCamera *AMVideoCamera::m_instance = NULL;
AMVideoCamera::AMVideoCamera():
    m_camera_config(NULL),
    m_system_capability(NULL),
    m_encode_device(NULL),
    m_ref_count(0),
    m_encode_mode(AM_VIDEO_ENCODE_INVALID_MODE),
    m_camera_state(AM_CAMERA_STATE_NOT_INIT)

{
    memset(&m_dsp_cap, 0, sizeof(m_dsp_cap));
}

AMIVideoCameraPtr AMIVideoCamera::get_instance()
{
  return AMVideoCamera::get_instance();
}

AMVideoCamera *AMVideoCamera::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMVideoCamera;
  }
  return m_instance;
}

void AMVideoCamera::inc_ref()
{
  ++ m_ref_count;
}

void AMVideoCamera::release()
{
  DECLARE_MUTEX;
  DEBUG("AMVideoCamera release is called!");
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    DEBUG("This is the last reference of AMVideoCamera's object, "
          "delete object instance %p!", m_instance);
    delete m_instance;
    m_instance = NULL;
  }
}

AMVideoCamera::~AMVideoCamera()
{
  destroy();
  m_instance = NULL;
  DEBUG("~AMVideoCamera");
}

//use DSP feature to init camera
AM_RESULT AMVideoCamera::init(AMEncodeDSPCapability *dsp_cap)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {
      ERROR("AMVideoCamera: init failed, camera state %d \n",
            (int )m_camera_state);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!dsp_cap) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_dsp_cap = *dsp_cap;

    m_system_capability = new AMEncodeSystemCapability;
    if (!m_system_capability) {
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if (NULL == get_encode_device()) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMVideoCamera: unable to get encode device\n");
      break;
    }


    //init basic data structure
    result = m_encode_device->init();
    if (result != AM_RESULT_OK) {
      break;
    }

    //set HDR as a property to m_encode_device,
    //this is because HDR is an option that affects camera and mode choice
    //globally, encode device object should got this as one of default option
    m_encode_device->set_vin_hdr_type(m_camera_config->m_loaded.hdr);

  } while (0);

  if (result == AM_RESULT_OK)
    m_camera_state = AM_CAMERA_STATE_INIT_DONE;

  return result;
}

//use mode to init camera
AM_RESULT AMVideoCamera::init(AM_ENCODE_MODE mode)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoCamera: init with mode %d \n", (int )mode);
  do {
    if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {
      ERROR("AMVideoCamera: init failed, camera state %d \n",
            (int )m_camera_state);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_encode_mode = mode;

    m_system_capability = new AMEncodeSystemCapability;
    if (!m_system_capability) {
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if (NULL == get_encode_device()) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMVideoCamera: unable to get encode device\n");
      break;
    }
    result = m_encode_device->init();
    if (result != AM_RESULT_OK) {
      break;
    }
  } while (0);

  if (result == AM_RESULT_OK)
    m_camera_state = AM_CAMERA_STATE_INIT_DONE;

  return result;
}

AM_RESULT AMVideoCamera::init() //init with config file to specify mode
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeDSPCapability dsp_cap;
  do {
    if (!m_camera_config) {
      m_camera_config = new AMCameraConfig();
      if (!m_camera_config) {
        result = AM_RESULT_ERR_MEM;
        break;
      }
    }
    result = m_camera_config->load_config(&dsp_cap);
    if (result!= AM_RESULT_OK) {
      ERROR("AMVideoCamera: unable to init from config file\n");
      break;
    }
    m_encode_mode = m_camera_config->m_loaded.mode;
    result = init(&dsp_cap);
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoCamera: init dsp_cap failed\n");
      break;
    }
    result = load_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoCamera: load config failed\n");
      break;
    }
  }while(0);
  return result;
}

AM_RESULT AMVideoCamera::destroy()
{
  DECLARE_MUTEX;
  if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {

    delete m_encode_device;
    m_encode_device = NULL;

    delete m_system_capability;
    m_system_capability = NULL;

    delete m_camera_config;
    m_camera_config = NULL;

    m_encode_mode = AM_VIDEO_ENCODE_INVALID_MODE;
    m_camera_state = AM_CAMERA_STATE_NOT_INIT;
  }

  return AM_RESULT_OK;
}

AM_RESULT AMVideoCamera::create_encode_device()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeDevice* encode_device = NULL;
  do {
    if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {
      ERROR("AMVideoCamera: cannot create encode device again!\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }
    switch (m_encode_mode) {
      case AM_VIDEO_ENCODE_NORMAL_MODE:
        encode_device = AMVideoNormalMode::create();
        break;

      case AM_VIDEO_ENCODE_DEWARP_MODE:
        encode_device = AMVideoDewarpMode::create();
        break;

      case AM_VIDEO_ENCODE_ADV_ISO_MODE:
        encode_device = AMVideoAdvISOMode::create();
        break;

      case AM_VIDEO_ENCODE_ADV_HDR_MODE:
        encode_device = AMVideoAdvHDRMode::create();
        break;

      case AM_VIDEO_ENCODE_RESERVED1_MODE:
      default:
        ERROR("AMVideoCamera: encode mode invalid %d \n");
        encode_device = NULL;
        break;
    }

    if (!encode_device) {
      ERROR("AMVideoCamera:  create null encode device \n");
      result = AM_RESULT_ERR_MEM;
    }

    m_encode_device = encode_device;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::find_encode_mode_by_feature(AM_ENCODE_MODE *mode)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_ENCODE_MODE encode_mode;
  do {
    if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {
      result = AM_RESULT_ERR_PERM;
      break;
    }
    result = m_system_capability->find_mode_for_dsp_capability(&m_dsp_cap,
                                                               &encode_mode);
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoCamera: unable to find proper mode to run dsp feature \n");
    } else {
      *mode = encode_mode;
    }
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::load_config() //load from config
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoCamera: load_config \n");

  do {
    if (m_camera_state == AM_CAMERA_STATE_NOT_INIT) {
      ERROR("AMVideoCamera: Camera not init, cannot load_config!\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!m_encode_device) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_encode_device->load_config();
    if (result != AM_RESULT_OK) {
      ERROR("Unable to load config in AMEncodeDevice\n");
      break;
    }
  } while (0);
  return result;
}

bool AMVideoCamera::is_mode_change_required()
{
  return false;
}

AM_RESULT AMVideoCamera::update()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoCamera: update \n");
  do {
    if (!m_encode_device) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    switch (m_camera_state) {
      case AM_CAMERA_STATE_NOT_INIT:
        ERROR("AMVideoCamera: Camera not init, cannot update!\n");
        result = AM_RESULT_ERR_PERM;
        break;

      case AM_CAMERA_STATE_INIT_DONE:
      case AM_CAMERA_STATE_STOPPED:
        //simple update
        result = m_encode_device->start();
        if (result != AM_RESULT_OK) {
          ERROR("Unable to update in AMEncodeDevice\n");
          break;
        }
        m_camera_state = AM_CAMERA_STATE_RUNNING;
        break;
      case AM_CAMERA_STATE_RUNNING:
        //update dynamic changes.
        if (is_mode_change_required()) {
          //destroy current camera first
          destroy();
          //init with new camera mode
          init(&m_dsp_cap);

          //after mode change, encode device needs to start again
          result = m_encode_device->start();
          if (result != AM_RESULT_OK) {
            ERROR("Unable to update in AMEncodeDevice\n");
          }
        } else {
          //if no mode change required, encode_device just do its auto update
          result = m_encode_device->update();
          if (result != AM_RESULT_OK) {
            ERROR("Unable to update in AMEncodeDevice\n");
          }
        }
        break;

      case AM_CAMERA_STATE_ERROR:
      default:
        ERROR("AMVideoCamera: Camera state %d , cannot update!\n",
              (int )m_camera_state);
        result = AM_RESULT_ERR_PERM;
        break;
    }
  } while (0);
  if (result != AM_RESULT_OK) {
    m_camera_state = AM_CAMERA_STATE_ERROR;
  }

  return result;
}

AM_RESULT AMVideoCamera::save_config()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoCamera: save_config \n");
  do {
    if (m_camera_state == AM_CAMERA_STATE_NOT_INIT) {
      ERROR("AMVideoCamera: Camera not init, cannot save_config!\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!m_encode_device) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_encode_device->save_config();
    if (result != AM_RESULT_OK) {
      ERROR("Unable to save_config in AMEncodeDevice\n");
      break;
    }
  } while (0);
  return result;
}

AMIEncodeDevice *AMVideoCamera::get_encode_device()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_camera_state != AM_CAMERA_STATE_NOT_INIT) {
       result = AM_RESULT_ERR_PERM;
       break;
    }

    //if we already have, then just return
    if (!m_encode_device) {
      if (m_encode_mode == AM_VIDEO_ENCODE_INVALID_MODE) {
        //only when m_encode_mode is invalid, find it, otherwise, use it.
        //otherwise, we need to find the mode and create it
        result = find_encode_mode_by_feature(&m_encode_mode);
        if (result != AM_RESULT_OK) {
          ERROR("unable to find right encode mode for feature\n");
          break;
        }
      }
      //create device by current mode
      result = create_encode_device();
      if (result != AM_RESULT_OK) {
        m_encode_device = NULL;
      }
    }
  } while (0);

  return m_encode_device;
}

//high level camera interface
//it will try to load default config file, and find a mode, init that mode,
//and call that device's update method.
AM_RESULT AMVideoCamera::start()
{
  PRINT_CURRENT_TIME("before start");

  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMVideoCamera: start \n");
  switch (m_camera_state) {
    case AM_CAMERA_STATE_NOT_INIT:
      ERROR("AMVideoCamera: Camera not init, cannot start!\n");
      result = AM_RESULT_ERR_PERM;
      break;
    case AM_CAMERA_STATE_RUNNING:
      WARN("AMVideoCamera: Camera running , cannot run again, ignore this!\n");
      result = AM_RESULT_OK;
      break;
    case AM_CAMERA_STATE_INIT_DONE:
    case AM_CAMERA_STATE_STOPPED:
      PRINT_CURRENT_TIME("before load_config");
      result = load_config();
      if (result != AM_RESULT_OK) {
        break;
      }
      PRINT_CURRENT_TIME("before encode device start");
      result = m_encode_device->start();
      if (result != AM_RESULT_OK) {
        ERROR("Unable to start in AMEncodeDevice\n");
        break;
      }
      break;
    case AM_CAMERA_STATE_ERROR:
    default:
      ERROR("AMVideocamera: camera state %d not supported to start\n",
            (int )m_camera_state);
      result = AM_RESULT_ERR_PERM;
      break;
  }

  if (result == AM_RESULT_OK)
    m_camera_state = AM_CAMERA_STATE_RUNNING;
  else
    m_camera_state = AM_CAMERA_STATE_ERROR;

  PRINT_CURRENT_TIME("after start");
  return result;
}

//try to call that device's stop method.
AM_RESULT AMVideoCamera::stop()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_camera_state == AM_CAMERA_STATE_STOPPED) {
      break;
    }
    INFO("AMVideoCamera: stop \n");
    switch (m_camera_state) {
      case AM_CAMERA_STATE_NOT_INIT:
      ERROR("AMVideoCamera: Camera not init, cannot stop!\n");
      result = AM_RESULT_ERR_PERM;
      break;
      case AM_CAMERA_STATE_INIT_DONE:
      case AM_CAMERA_STATE_STOPPED:
      INFO("AMVideoCamera: Camera not running, just stop it!\n");
      //there is no difference for "AMVideoCamera" between init done and stopped
      result = AM_RESULT_OK;
      break;
      case AM_CAMERA_STATE_RUNNING:
      case AM_CAMERA_STATE_ERROR:
      //stop it even if camera state is ERROR
      if (!m_encode_device) {
        result = AM_RESULT_ERR_INVALID;
        break;
      }
      result = m_encode_device->stop();
      if (result != AM_RESULT_OK) {
        ERROR("Unable to stop in AMEncodeDevice\n");
        break;
      }
      break;
      default:
      ERROR("AMVideocamera: camera state %d not supported to stop\n",
          (int )m_camera_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }
  }while(0);

  if (result == AM_RESULT_OK)
    m_camera_state = AM_CAMERA_STATE_STOPPED;
  else
    m_camera_state = AM_CAMERA_STATE_ERROR;
  return result;
}

/*
 *
 * AMVideo Camera High Level API Interface
 *
 */
AM_RESULT AMVideoCamera::set_hdr_enable(bool enable)
{
  return AM_RESULT_OK;
}

AM_RESULT AMVideoCamera::set_advanced_iso_enable(bool enable)
{

  return AM_RESULT_OK;
}

AM_RESULT AMVideoCamera::set_ldc_enable(bool enable)
{
  return AM_RESULT_OK;
}

AM_RESULT AMVideoCamera::set_dewarp_enable(bool enable)
{
  return AM_RESULT_OK;
}
