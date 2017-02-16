/*******************************************************************************
 * am_encode_device.cpp
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
#include <sys/time.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_encode_buffer.h"
#include "am_encode_device.h"
#include "am_dptz_warp.h"
#include "am_lbr_control.h"
#include "am_event_types.h"



extern AMResolution g_source_buffer_max_size_default[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
#include "iav_ioctl.h"

//this is a BIG LOCK to EncodeDevice, so it prevents user to use multiple threads
//to call the AMEncodeDevice's methods simultaneously
//so that AMEncodeDevice can be thread safe
//lock_guard and recursive_mutex are C++11 features,
//the mutex auto locked and auto unlocked
#define  DECLARE_MUTEX   std::lock_guard<std::recursive_mutex> lck (m_mtx);

AMEncodeDevice::AMEncodeDevice():
    m_init_flag(false),
    m_goto_idle_needed(false),
    m_source_buffer(NULL),
    m_max_source_buffer_num(AM_ENCODE_SOURCE_BUFFER_MAX_NUM),
    m_encode_stream(NULL),
    m_max_encode_stream_num(AM_STREAM_MAX_NUM),
    m_dptzwarp(NULL),
    m_pm(NULL),
    m_lbr_ctrl(NULL),
    m_jpeg_snapshot_handler(NULL),
    m_osd_overlay(NULL),
    m_config(NULL),
    m_encode_mode(AM_VIDEO_ENCODE_INVALID_MODE),
    m_rsrc_lmt_cfg(NULL)
{

}

AM_RESULT AMEncodeDevice::construct()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_source_buffer = new AMEncodeSourceBuffer[m_max_source_buffer_num];
    if (!m_source_buffer) {
      ERROR("AMEncodeDevice:: new m_source_buffer error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_encode_stream = new AMEncodeStream[m_max_encode_stream_num];
    if (!m_encode_stream) {
      ERROR("AMEncodeDevice:: new m_encode_stream error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config = new AMEncodeDeviceConfig(this);
    if (!m_config) {
      ERROR("AMEncodeDevice:: new m_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_dptzwarp = new AMDPTZWarp();
    if (!m_dptzwarp) {
      ERROR("AMEncodeDevice:: new m_dptzwarp error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_lbr_ctrl = new AMLBRControl();
    if (!m_lbr_ctrl) {
      ERROR("AMEncodeDevice:: new m_lbr_ctrl error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_jpeg_snapshot_handler = AMIJpegSnapshot::get_instance();
    if (!m_jpeg_snapshot_handler) {
      ERROR("AMEncodeDevice:: create m_jpeg_snapshot_handler error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_osd_overlay = AMIOSDOverlay::create();
    if (!m_osd_overlay) {
      ERROR("AMEncodeDevide:: new m_osd_overlay error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

  } while (0);

  return result;
}

AMEncodeDevice *AMEncodeDevice::create()
{
  AMEncodeDevice *result = new AMEncodeDevice;
  if (!result || result->construct() != AM_RESULT_OK) {
    ERROR("AMEncodeDevice::get_instance error\n");
    delete result;
    result = NULL;
  }

  return result;
}

AMEncodeDevice::~AMEncodeDevice()
{
  //restore the state by destroy() before delete
  INFO("AMEncodeDevice::~AMEncodeDevice()\n");
  destroy();
}

AMEncodeSourceBuffer *AMEncodeDevice::get_source_buffer(
    AM_ENCODE_SOURCE_BUFFER_ID buf_id)
{
  if (!m_init_flag) {
    return NULL;
  }

  //verify buf_id
  if (((int) buf_id < (int) AM_ENCODE_SOURCE_BUFFER_FIRST)
      || ((int) buf_id > (int) AM_ENCODE_SOURCE_BUFFER_LAST)) {
    ERROR("AMEncodeDevice: buf id out of range \n");
    return NULL;
  }

  if (!m_source_buffer) {
    ERROR("AMEncodeDevice: source buffer pointer is NULL \n");
    return NULL;
  }
  return (m_source_buffer + (int) buf_id);
}

//set all of the encode formats, ususally used for initial setup
//user should use other functions to do like digital zoom type II
AM_RESULT AMEncodeDevice::set_encode_stream_format_all(AMEncodeStreamFormatAll
                                                       *stream_format_all)
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    //always check dsp state, this operation must be done in idle
    if ((dsp_state != AM_DSP_STATE_IDLE)
        && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: stream format can only be setup in IDLE, preview"
          "or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!stream_format_all) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (i = 0; i < m_max_encode_stream_num; i ++) {
      if (stream_format_all->encode_format[i].enable) {
        result = (m_encode_stream + i)->set_encode_format(&stream_format_all->\
                                                          encode_format[i]);
        if (result != AM_RESULT_OK) {
          break;
        }
      }
    }
  } while (0);
  return result;
}


AM_RESULT AMEncodeDevice::get_encode_stream_format_all(AMEncodeStreamFormatAll
                                                       *stream_format_all)
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_IDLE)
        && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: stream format can only be got in IDLE, preview"
          "or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!stream_format_all) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (i = 0; i < m_max_encode_stream_num; i ++) {
      if (stream_format_all->encode_format[i].enable) {
        result = (m_encode_stream + i)->get_encode_format(&stream_format_all->\
                                                          encode_format[i]);
        if (result != AM_RESULT_OK)
          break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::set_encode_stream_config_all(
    AMEncodeStreamConfigAll *stream_config_all)
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_IDLE)
        && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: stream encode config can only be got in IDLE, "
          "preview or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!stream_config_all) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (i = 0; i < m_max_encode_stream_num; i ++) {
      if (stream_config_all->encode_config[i].apply) {
        result =
            (m_encode_stream + i)->set_encode_config(&stream_config_all->\
                                                     encode_config[i]);
        if (result != AM_RESULT_OK)
          break;
      }
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::get_encode_stream_config_all(
    AMEncodeStreamConfigAll *stream_config_all)
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_IDLE)
        && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: stream config can only be got in IDLE, preview"
          "or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!stream_config_all) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (i = 0; i < m_max_encode_stream_num; i ++) {
      if (stream_config_all->encode_config[i].apply) {
        result = (m_encode_stream + i)->get_encode_config(&stream_config_all->\
                                                          encode_config[i]);

        if (result != AM_RESULT_OK)
          break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::set_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }
    if (!dptz_warp_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_dptzwarp->set_config(dptz_warp_config);
    if (result != AM_RESULT_OK) {
      ERROR("m_dptzwarp->set_config failed");
      break;
    }
    result = m_dptzwarp->apply_dptz_warp();
    if (result != AM_RESULT_OK) {
      ERROR("m_dptzwarp->apply_dptz_warp failed");
      break;
    }
    memcpy(&m_config->m_loaded.dptz_warp_config, dptz_warp_config, sizeof(m_config->m_loaded.dptz_warp_config));
    m_config->sync();
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::get_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_IDLE)
          && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
          && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: dptz warp config can only be got in IDLE, preview"
            "or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!dptz_warp_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_dptzwarp->get_config(dptz_warp_config);
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::set_lbr_config(AMEncodeLBRConfig *lbr_config)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!lbr_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    result = m_lbr_ctrl->set_config(lbr_config);
    m_config->m_loaded.lbr_config = *lbr_config;
    m_config->sync();
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::get_lbr_config(AMEncodeLBRConfig *lbr_config)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_IDLE)
          && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
          && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMEncodeDevice: lbr config can only be got in IDLE, preview"
            "or encoding\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!lbr_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_lbr_ctrl->get_config(lbr_config);
  } while (0);

  return result;
}

bool AMEncodeDevice::is_motion_enable()
{
  return m_jpeg_snapshot_handler->is_enable();
}

void AMEncodeDevice::update_motion_state(void *msg)
{
  return m_jpeg_snapshot_handler->update_motion_state((AM_EVENT_MESSAGE*)msg);
}

/* Stage 2 , Start/stop encoding and on the fly change during Encoding*/
//start encoding for all configured streams
AM_RESULT AMEncodeDevice::start_encode()
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot start encode\n",
            dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }

    for (i = 0; i < m_max_encode_stream_num; i ++) {
      result = (m_encode_stream + i)->start_encode();
      if (result != AM_RESULT_OK) {
        ERROR("AMEncodeDevice: start encode error for stream %d\n", i);
        break;
      }
    }
  } while (0);
  return result;
}

//stop encoding for all configured streams
AM_RESULT AMEncodeDevice::stop_encode()
{
  DECLARE_MUTEX;
  int i;
  uint32_t streamid;
  struct iav_stream_info streaminfo;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if (dsp_state != AM_DSP_STATE_ENCODING) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot stop encode\n", dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }
    /*
   stop one by one
    for (i = 0; i < m_max_encode_stream_num; i ++) {
      result = (m_encode_stream + i)->stop_encode();
      if (result != AM_RESULT_OK) {
        ERROR("AMEncodeDevice: stop encode error for stream %d\n", i);
        break;
      }
     }
     */
    // stop all of the streams together
   // struct timeval tv1, tv2;
    streamid = 0xFFFFFFFF;
    for (i = 0; i < m_max_encode_stream_num; ++i) {
      streaminfo.id = i;
      if (ioctl(get_iav(), IAV_IOC_GET_STREAM_INFO, &streaminfo) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
      if (streaminfo.state != IAV_STREAM_STATE_ENCODING) {
        streamid &= ~(1 << i);
      }
    }
    //gettimeofday(&tv1, NULL);
    if (ioctl(get_iav(), IAV_IOC_STOP_ENCODE, streamid) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    /*gettimeofday(&tv2, NULL);
    PRINTF("Stop Encode costs %ld micro seconds \n",
           (tv2.tv_usec + tv2.tv_sec * 1000000L)
           - (tv1.tv_usec + tv1.tv_sec * 1000000L));
     */
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::start_stop_encode()
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_ENCODING)
        && (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)) {
      ERROR("AMEncodeDevice: state not ready \n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    //get all stream encoding state first
    INFO("AMEncodeDevice: start_stop_encode\n");
    for (i = 0; i < m_max_encode_stream_num; i ++) {
      if (is_stream_enabled(i)) {
        INFO("Trying to start stream%d", i);
        result = (m_encode_stream + i)->start_encode();
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: start encode error for stream %d\n", i);
          break;
        }
      } else {
        result = (m_encode_stream + i)->stop_encode();
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: stop encode error for stream %d\n", i);
          break;
        }
      }
    } //END for
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::start_encode_stream(AM_STREAM_ID stream_id)
{
  DECLARE_MUTEX;
  int id = stream_id;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if ((dsp_state != AM_DSP_STATE_READY_FOR_ENCODE)
        && (dsp_state != AM_DSP_STATE_ENCODING)) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot start encode\n",
            dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }

    //don't check whether the stream can be started or not here
    //let AMEncodeStream class to get ret
    result = (m_encode_stream + id)->start_encode();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: start encode for stream id %d failed\n ", id);
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::stop_encode_stream(AM_STREAM_ID stream_id)
{
  DECLARE_MUTEX;
  int id = stream_id;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if (dsp_state != AM_DSP_STATE_ENCODING) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot stop encode\n", dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }

    //don't check whether the stream is in encoding or not here,
    //let AMEncodeStream class to get ret
    result = (m_encode_stream + id)->stop_encode();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: stop encode for stream id %d failed\n ", id);
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::show_stream_info(AM_STREAM_ID stream_id)
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  if (!m_init_flag) {
    result = AM_RESULT_ERR_PERM;
  }
  INFO("AMEncodeDevice: show info for stream %d \n", (int) stream_id);
  return result;
}

AM_RESULT AMEncodeDevice::set_stream_bitrate(AM_STREAM_ID stream_id,
                                             uint32_t bitrate)
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  if (!m_init_flag) {
     result = AM_RESULT_ERR_PERM;
  }
  INFO("AMEncodeDevice: set bitrate for stream %u\n", stream_id);

  return result;
}

AM_RESULT AMEncodeDevice::set_stream_fps(AM_STREAM_ID stream_id,
                                         uint32_t fps)
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  if (!m_init_flag) {
     result = AM_RESULT_ERR_PERM;
  }
  INFO("AMEncodeDevice: set fps for stream %u\n", stream_id);

  return result;
}

AM_RESULT AMEncodeDevice::get_stream_fps(AM_STREAM_ID stream_id, uint32_t *fps)
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  if (!m_init_flag) {
    result = AM_RESULT_ERR_PERM;
  }
  INFO("AMEncodeDevice: get fps for stream %u\n", stream_id);

  return result;
}

AM_RESULT AMEncodeDevice::show_source_buffer_info(
    AM_ENCODE_SOURCE_BUFFER_ID buf_id)
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
  }
  INFO("AMEncodeDevice: show info for buffer %d \n", (int)buf_id);
  return result;
}

AM_RESULT AMEncodeDevice::set_force_idr(AM_STREAM_ID stream_id)
{
  AMEncodeStreamDynamicConfig dynamic_config;
  memset(&dynamic_config, 0, sizeof(dynamic_config));

  dynamic_config.h264_force_idr_immediately = true;

  return set_encode_stream_dynamic_config(stream_id,  &dynamic_config);
}

AMIOSDOverlay* AMEncodeDevice::get_osd_overlay()
{
  return m_osd_overlay;
}

AMIDPTZWarp* AMEncodeDevice::get_dptz_warp()
{
  return m_dptzwarp;
}

AMIVinConfig* AMEncodeDevice::get_vin_config()
{
  return AMVideoDevice::get_vin_config();
}

AMIEncodeDeviceConfig* AMEncodeDevice::get_encode_config()
{
  return m_config;
}

AM_RESULT AMEncodeDevice::set_encode_stream_dynamic_config(AM_STREAM_ID stream_id,
                                                           AMEncodeStreamDynamicConfig *dynamic_config)
{
  AM_RESULT result = AM_RESULT_OK;
  int id;
  DECLARE_MUTEX;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }
    id = stream_id;
    result = (m_encode_stream + id)->set_encode_dynamic_config(dynamic_config);
  } while (0);
  return result;
}

/***** data init/deinit *****/
AM_RESULT AMEncodeDevice::init()
{
  DECLARE_MUTEX;
  int i;
  AM_RESULT result = AM_RESULT_OK;
  do {
    //init iav
    INFO("AMEncodeDevice: init starts \n");
    result = AMVideoDevice::init();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice init failed\n");
      break;
    }

    if (!m_source_buffer) {
      result = AM_RESULT_ERR_MEM;
      break;
    }

    //init buffers
    INFO("AMEncodeDevice: init source buffers \n");
    for (i = 0; i < m_max_source_buffer_num; i ++) {
      //using enum's copy constructor to create it,
      //another technique is to use static_cast <type> (i), however,ugly
      AM_ENCODE_SOURCE_BUFFER_ID id = AM_ENCODE_SOURCE_BUFFER_ID(i);
      result = (m_source_buffer + i)->init(this, id, get_iav());
      if (result != AM_RESULT_OK)
        break;
    }

    if (result != AM_RESULT_OK)
      break;

    //init streams
    if (!m_encode_stream) {
      result = AM_RESULT_ERR_MEM;
      break;
    }
    INFO("AMEncodeDevice: init encode streams \n");
    for (i = 0; i < m_max_encode_stream_num; i ++) {
      AM_STREAM_ID id = AM_STREAM_ID(i);
      result = (m_encode_stream + i)->init(this, id, get_iav());
      if (result != AM_RESULT_OK)
        break;
    }
    if (result != AM_RESULT_OK)
      break;

    //init dptz warp
    if (!m_dptzwarp) {
      result = AM_RESULT_ERR_MEM;
      break;
    }
    result = m_dptzwarp->init(get_iav(), this);
    if (result != AM_RESULT_OK) {
      break;
    }

    //init lbr ctrl
    if (!m_lbr_ctrl) {
      result = AM_RESULT_ERR_MEM;
      break;
    }
    result = m_lbr_ctrl->init(get_iav(), this);
    if (result != AM_RESULT_OK) {
      break;
    }

    if (!m_osd_overlay) {
      result = AM_RESULT_ERR_MEM;
      break;
    }
    result = m_osd_overlay->init(get_iav(), this);
    if (result != AM_RESULT_OK) {
      break;
    }

    m_init_flag = true;
    m_goto_idle_needed = false;
  }while (0);

  return result;
}

AM_RESULT AMEncodeDevice::destroy()
{
  DECLARE_MUTEX;
  INFO("AMEncodeDevice::destroy()\n");
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
      INFO("AMEncodeDevice::not init, no need to destroy()\n");
      break;
    }

    result = quit();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: quit error\n");
      break;
    }

    m_config->save_config();

    delete[] m_source_buffer;
    m_source_buffer = NULL;
    delete[] m_encode_stream;
    m_encode_stream = NULL;
    delete m_config;
    m_config = NULL;
    delete m_dptzwarp;
    m_dptzwarp = NULL;
    delete m_lbr_ctrl;
    m_lbr_ctrl = NULL;
    delete m_rsrc_lmt_cfg;
    m_rsrc_lmt_cfg = NULL;

    if (m_osd_overlay) {
      m_osd_overlay->destroy();
      m_osd_overlay = NULL;
    }

    m_init_flag = false;
  }while(0);

  return result;
}

AM_RESULT AMEncodeDevice::load_config()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    result = AMVideoDevice::load_config();

    if (result != AM_RESULT_OK) {
      break;
    }

    result = m_config->load_config();

    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: load config failed \n");
      break;
    }

    result = m_config->sync();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: sync config failed \n");
      break;
    }

    INFO("AMEncodeDevice: try set_lbr_config\n");
    result = m_lbr_ctrl->load_config(&m_config->m_loaded.lbr_config);
    if (result != AM_RESULT_OK) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeDevice: load lbr config failed \n");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::save_config()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    result = AMVideoDevice::save_config();

    if (result != AM_RESULT_OK) {
      break;
    }

    result = m_config->save_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: save config failed \n");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::get_encode_mode(AM_ENCODE_MODE *mode)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    if (!mode) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    *mode = m_encode_mode;
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::start()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;

  if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      return result;
  }

  AM_DSP_STATE dsp_state = get_dsp_state();
  switch(dsp_state)
  {
    case AM_DSP_STATE_NOT_INIT:
    case AM_DSP_STATE_IDLE:
      result = idle_to_encode();
      if (result == AM_RESULT_OK) {
        m_goto_idle_needed = false;
      }
      break;

    case AM_DSP_STATE_READY_FOR_ENCODE:
      do {
        if (is_goto_idle_needed()) {
          result = goto_idle();
          if (result != AM_RESULT_OK) {
            break;
          }

          result = idle_to_encode();

          if (result != AM_RESULT_OK) {
            break;
          }
        } else {
          result = preview_to_encode();
          if (result != AM_RESULT_OK) {
            break;
          }
        }
      } while (0);
      break;

    case AM_DSP_STATE_ENCODING:
      WARN("AMEncodeDevice:try to start encoding, but already encoding, ignore\n");
      break;

    case AM_DSP_STATE_EXITING_ENCODING:
      ERROR("AMEncodeDevice: busy now, exiting to idle \n");
      result = AM_RESULT_ERR_PERM;
      break;
    default:
      ERROR("AMEncodeDevice: cannot start in state %d \n", (int)dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
  }

  if (result == AM_RESULT_OK) {
    INFO("AMEncodeDevice:  sync current config to using \n");
    m_config->sync();
  }

  return result;
}

//stop means stop all encoding activities, but don't go to idle
AM_RESULT AMEncodeDevice::stop()
{
  DECLARE_MUTEX;
  INFO("AMEncodeDevice::stop()\n");
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    switch (dsp_state) {
      case AM_DSP_STATE_NOT_INIT:
        INFO("AMEncodeDevice: not init, no need to stop \n");
        break;

      case AM_DSP_STATE_IDLE:
        INFO("AMEncodeDevice: it's idle, no need to stop \n");
        break;

      case AM_DSP_STATE_READY_FOR_ENCODE:
        INFO("AMEncodeDevice: it's preview, no need to stop \n");
        break;

      case AM_DSP_STATE_EXITING_ENCODING:
        INFO("AMEncodeDevice: it's going back to idle, no need to stop \n");
        break;

      case AM_DSP_STATE_ENCODING:
        INFO("AMEncodeDevice: stop lbr\n");
        result = m_lbr_ctrl->lbr_ctrl_stop();
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: try lbr_ctrl_stop failed\n");
          break;
        }

        if (!m_osd_overlay->destroy()) {
          ERROR("AMEncodeDevice: try destroy overlay failed\n");
          break;
        }

        result = m_jpeg_snapshot_handler->stop();
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: try motion handler stop failed\n");
          break;
        }
        INFO("AMEncodeDevice: stop encoding\n");
        result = stop_encode();
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: stop encoding failed \n");
        }
        INFO("AMEncodeDevice:  sync current config to using \n");
        m_config->sync();
        break;

      default:
        ERROR("AMEncodeDevice: cannot stop in state %d \n", (int )dsp_state);
        result = AM_RESULT_ERR_PERM;
        break;
    }
  }while (0);
  return result;
}

//quit =  "stop encoding" + "go to idle"
AM_RESULT AMEncodeDevice::quit()
{
  DECLARE_MUTEX;
  INFO("AMEncodeDevice::quit()\n");
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    result = stop();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: stop error \n");
      break;
    }

    result = goto_idle();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: goto_idle error \n");
      break;
    }

  }while (0);

  return result;
}

//'Update' means use new config to refresh all states
//so it can be 'start' or 'stop', or configuration changes.
//Note that update will not automatically go to idle.
//if idle cycle is required, then update will report error,
//and then user needs to stop and start.

//update may be thought to be "dynamic update" purpose.

AM_RESULT AMEncodeDevice::update()
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;

  if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      return result;
  }

  AM_DSP_STATE dsp_state = get_dsp_state();

  switch (dsp_state) {
    case AM_DSP_STATE_NOT_INIT:
      ERROR("AMEncodeDevice: not init, cannot update \n");
      result = AM_RESULT_ERR_PERM;
      break;

    case AM_DSP_STATE_IDLE:
      ERROR("AMEncodeDevice:update: cannot update, please start() first\n");
      result = AM_RESULT_ERR_PERM;
      break;

    case AM_DSP_STATE_READY_FOR_ENCODE:
      INFO("AMEncodeDevice:update: try update stream encoding status\n");
      do {
        if (is_goto_idle_needed()) {
          result = AM_RESULT_ERR_PERM;
          ERROR("AMEncodeDevice: goto idle required, please stop/start, "
              "update cannot handle this\n");
          break;
        }
        result = preview_to_encode();
      } while (0);
      break;

    case AM_DSP_STATE_ENCODING:
      INFO("AMEncodeDevice:update: try update encoding stop status\n");
      do {
        if (is_goto_idle_needed()) {
          result = AM_RESULT_ERR_PERM;
          ERROR("AMEncodeDevice: goto idle required, please stop/start, "
              "update cannot handle this\n");
          break;
        }
        if (is_stream_format_changed()) {
          m_goto_idle_needed = true;
          if (m_config->m_buffer_alloc_style == AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
            result = set_encode_stream_format_all(&m_config->\
                                                  m_loaded.stream_format);
          } else {
            result = set_encode_stream_format_all(&m_auto_encode_param.\
                                                  stream_format);
          }
          if (result != AM_RESULT_OK) {
            ERROR("AMEncodeDevice: set encode stream format failed \n");
            break;
          }
        }

        if (is_stream_config_changed()) {
          m_goto_idle_needed = true;
          if (m_config->m_buffer_alloc_style ==
              AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
            result = set_encode_stream_config_all(&m_config->\
                                                  m_loaded.stream_config);
          } else {
            result = set_encode_stream_config_all(&m_auto_encode_param.\
                                                  stream_config);
          }
          if (result != AM_RESULT_OK) {
            ERROR("AMEncodeDevice: set encode stream config failed \n");
            break;
          }
        }

        if (is_stream_encode_control_changed()) {
          m_goto_idle_needed = true;
          INFO("AMEncodeDevice: try start encoding\n");
          result = start_stop_encode();
          if (result != AM_RESULT_OK) {
            ERROR("AMEncodeDevice: try start encoding failed \n");
            break;
          }
        }

        if (is_lbr_config_changed()) {
          m_goto_idle_needed = true;
          result = set_lbr_config(&m_config->m_loaded.lbr_config);
          if (result != AM_RESULT_OK) {
            ERROR("AMEncodeDevice: set lbr config failed \n");
            break;
          }
        }

        result = set_dptz_warp_config(&m_config->m_loaded.dptz_warp_config);
        if (result != AM_RESULT_OK) {
          ERROR("AMEncodeDevice: set dptz warp config failed \n");
          break;
        }

        if (is_stream_encode_on_the_fly_changed()) {
          m_goto_idle_needed = true;
          INFO("AMEncodeDevice: try update on the fly parameters \n");
        }

      } while (0);
      break;

    case AM_DSP_STATE_EXITING_ENCODING:
      ERROR("AMEncodeDevice: cannot update in state %d \n", (int )dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;

    default:
      ERROR("AMEncodeDevice: cannot update in state %d \n", (int )dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
  }

  //save it
  if (result == AM_RESULT_OK) {
    INFO("AMEncodeDevice: save config to using \n");
    m_config->sync();
  }

  INFO("AMEncodeDevice: update done \n");

  return result;
}

/*******************************************************************************
 *
 *                   PUBLIC methods END
 *
 * ****************************************************************************/

//stream enable is attribute of "stream format"
bool AMEncodeDevice::is_stream_enabled(int i)
{
  return m_config->m_loaded.stream_format.encode_format[i].enable;
}

bool AMEncodeDevice::is_goto_idle_needed()
{
  //if buffer changed, VIN, VOUT config changed,  go to idle
  return m_goto_idle_needed;
}

bool AMEncodeDevice::is_stream_format_changed()
{

  return true;
}

bool AMEncodeDevice::is_stream_config_changed()
{

  return true;
}

bool AMEncodeDevice::is_stream_encode_control_changed()
{

  return true;
}

bool AMEncodeDevice::is_stream_encode_on_the_fly_changed()
{

    return true;
}

bool AMEncodeDevice::is_lbr_config_changed()
{
    return true;
}

AM_RESULT AMEncodeDevice::idle_to_encode()
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }
    PRINT_CURRENT_TIME("before idle to preview");

    result = idle_to_preview();
    if (result != AM_RESULT_OK) {
      break;
    }

    PRINT_CURRENT_TIME("before preview to encode");
    result = preview_to_encode();
    if (result != AM_RESULT_OK) {
      break;
    }
  } while(0);

  return result;
}

AM_RESULT AMEncodeDevice::preview_to_encode()
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  do {
    if (!m_init_flag) {
        result = AM_RESULT_ERR_PERM;
        break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();
    if (dsp_state != AM_DSP_STATE_READY_FOR_ENCODE) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot go to encode if current "
            "state is not preview \n",
          dsp_state);
      result = AM_RESULT_ERR_PERM;
      break;
    }
    if (m_config->m_buffer_alloc_style ==
        AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
      result = set_encode_stream_format_all(&m_config->m_loaded.stream_format);
    } else {
      result = set_encode_stream_format_all(&m_auto_encode_param.stream_format);
    }
    if (result != AM_RESULT_OK) {
      //invalid stream format
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeDevice: set encode stream format failed \n");
      break;
    }
    if (m_config->m_buffer_alloc_style ==
        AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
      result = set_encode_stream_config_all(&m_config->m_loaded.stream_config);
    } else {
      result = set_encode_stream_config_all(&m_auto_encode_param.stream_config);
    }
    if (result != AM_RESULT_OK) {
      //invalid stream config
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeDevice: set encode stream config failed \n");
      break;
    }

    INFO("AMEncodeDevice: try start encoding\n");

    PRINT_CURRENT_TIME("before start stop encode");
    result = start_stop_encode();


    INFO("AMEncodeDevice: try set DPTZ Warp config\n");
    result = m_dptzwarp->set_config_all(&m_config->m_loaded.dptz_warp_config);
    if (result != AM_RESULT_OK) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeDevice: set DPTZ Warp config failed \n");
      break;
    }

    INFO("AMEncodeDevice: apply DPTZ Warp settings");
    result = m_dptzwarp->apply_dptz_warp_all();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: apply DPTZ warp settings failed");
    }

    INFO("AMEncodeDevice: try lbr_ctrl_start\n");
    result = m_lbr_ctrl->lbr_ctrl_start();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: try lbr_ctrl_start failed \n");
    }

    result = m_jpeg_snapshot_handler->start();
    if (result != AM_RESULT_OK) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeDevice: jpeg encoder start failed \n");
      break;
    }

    INFO("AMEncodeDevie: add OSDOverlay \n");
    for (int32_t i=0; i<m_max_encode_stream_num; ++i) {
      m_osd_overlay->add((AM_STREAM_ID)i,
                         &m_config->m_loaded.osd_overlay_config.osd_cfg[i]);
    }


  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::set_system_resource_limit()
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  do {
    PRINTF("AMEncodeDevice::set_system_resource_limit is empty, "
           "please use derived class\n");
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::idle_to_preview()
{
  AM_RESULT result = AM_RESULT_OK;
  DECLARE_MUTEX;
  do {
    if (!m_init_flag) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    AM_DSP_STATE dsp_state = get_dsp_state();

    if ((dsp_state != AM_DSP_STATE_IDLE) &&
        (dsp_state != AM_DSP_STATE_NOT_INIT)) {
      ERROR("AMENcodeDevice: DSP state is %d, cannot enter preview if current "
            "state is not idle or init \n", dsp_state);
      result = AM_RESULT_ERR_PERM;
      m_goto_idle_needed = true;
      break;
    }

    PRINT_CURRENT_TIME("before AMVideoDevice::start()");

    //if current PREVIEW config does not match m_config->m_loaded
    //then go to idle to config it first, and then go to preview again
    //call base class update method to goto idle and setup vin/vout again
    result = AMVideoDevice::start();

    if (result != AM_RESULT_OK) {
      ERROR("AMVideoDevice: start failed \n");
      break;
    }

    if (!m_config->m_loaded.buffer_format.\
        source_buffer_format[AM_ENCODE_SOURCE_BUFFER_MAIN].input_crop) {
      //hard code VIN id to VIN0
      AMResolution size = get_vin_resolution(AM_VIN_ID(0));
      m_config->m_loaded.buffer_format.\
      source_buffer_format[AM_ENCODE_SOURCE_BUFFER_MAIN].input.width =
          size.width;
      m_config->m_loaded.buffer_format.\
      source_buffer_format[AM_ENCODE_SOURCE_BUFFER_MAIN].input.height =
          size.height;
    }

    if ((m_config->m_buffer_alloc_style !=
        AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) &&
        ((result = auto_calc_encode_param()) != AM_RESULT_OK)) {
      ERROR("AMVideoDevice: automatically calculate encode parameters error!");
      break;
    }

    PRINT_CURRENT_TIME("before set_system_resource_limit");

    result = set_system_resource_limit();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideDevice: setup system resource limit failed \n");
      break;
    }

    //setup source buffer
    if (m_config->m_buffer_alloc_style ==
        AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
      result = m_source_buffer->setup_source_buffer_all(&m_config->m_loaded.buffer_format);
    } else {
      result = m_source_buffer->setup_source_buffer_all(&m_auto_encode_param.buffer_format);
    }
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevice: set source buffer failed \n");
      break;
    }

    PRINT_CURRENT_TIME("before IAV_IOC_ENABLE_PREVIEW");

    if (ioctl(get_iav(), IAV_IOC_ENABLE_PREVIEW, 0) < 0) {
      PERROR("AMENcodeDevice: enable preview error");
      result = AM_RESULT_ERR_DSP;
      m_goto_idle_needed = true;
      break;
    }
    m_goto_idle_needed = false;
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::assign_stream_to_buffer(AM_STREAM_ID stream,
                                                  AM_ENCODE_SOURCE_BUFFER_ID buffer)
{
  AM_RESULT result = AM_RESULT_OK;

  static AMResolution main_buffer_size = {0};

  AMResolution vin_size = get_vin_resolution(AM_VIN_ID(0)); //FIXME: Hard code VIN0
  uint32_t vin_gcd = get_gcd(vin_size.width, vin_size.height);
  uint32_t vin_ratio_num = vin_size.width / vin_gcd;
  uint32_t vin_ratio_den = vin_size.height / vin_gcd;

  AMRect *input_window =
      &(m_auto_encode_param.buffer_format.source_buffer_format[buffer].input);
  AMResolution *buffer_size =
      &(m_auto_encode_param.buffer_format.source_buffer_format[buffer].size);

  AMResolution stream_size;
  stream_size.width = m_auto_encode_param.\
      stream_format.encode_format[stream].width;

  stream_size.height = m_auto_encode_param.\
      stream_format.encode_format[stream].height;

  uint32_t stream_gcd = get_gcd(stream_size.width, stream_size.height);
  uint32_t stream_ratio_num = stream_size.width / stream_gcd;
  uint32_t stream_ratio_den = stream_size.height / stream_gcd;

  m_auto_encode_param.stream_format.encode_format[stream].source = buffer;
  m_auto_encode_param.buffer_format.source_buffer_format[buffer].type =
      AM_ENCODE_SOURCE_BUFFER_TYPE_ENCODE;
  m_auto_encode_param.buffer_format.\
  source_buffer_format[buffer].input_crop = false;
  m_auto_encode_param.buffer_format.\
  source_buffer_format[buffer].prewarp = false;

  bool stream_ratio_equal_vin = true;
  if (stream_ratio_num != vin_ratio_num ||
      stream_ratio_den != vin_ratio_den) {
    stream_ratio_equal_vin = false;
  }

  *buffer_size = stream_size;
  if (buffer == AM_ENCODE_SOURCE_BUFFER_MAIN) {
    input_window->width = vin_size.width;
    input_window->height = vin_size.height;
    main_buffer_size = stream_size;
  } else {
    assert(main_buffer_size.width);
    assert(main_buffer_size.height);

    if (stream_ratio_equal_vin) {
      input_window->width = main_buffer_size.width;
      input_window->height = main_buffer_size.height;
    } else {
      if (((float)vin_ratio_num / vin_ratio_den) >
      ((float)stream_ratio_num / stream_ratio_den)) {
        input_window->height = main_buffer_size.height;
        input_window->width =
            input_window->height * stream_ratio_num / stream_ratio_den;
        input_window->x = (main_buffer_size.width - input_window->width) / 2;
      } else {
        input_window->width = main_buffer_size.width;
        input_window->height =
            input_window->width * stream_ratio_den / stream_ratio_num;
        input_window->y = (main_buffer_size.height - input_window->height) / 2;
      }
    }
  }

  return result;
}

AM_RESULT AMEncodeDevice::auto_calc_encode_param()
{
  // For vin
  uint32_t vin_gcd;
  uint32_t vin_ratio_num;
  uint32_t vin_ratio_den;
  AMResolution vin_size;

  // For stream
  uint32_t stream_gcd[AM_STREAM_MAX_NUM];
  uint32_t stream_ratio_num[AM_STREAM_MAX_NUM];
  uint32_t stream_ratio_den[AM_STREAM_MAX_NUM];
  uint32_t *stream_width[AM_STREAM_MAX_NUM] = {0};
  uint32_t *stream_height[AM_STREAM_MAX_NUM] = {0};

  // For stream sort
  uint32_t h264_count = 0;
  uint32_t mjpeg_count = 0;
  uint32_t stream_order_h264[AM_STREAM_MAX_NUM] = {0};
  uint32_t stream_order_mjpeg[AM_STREAM_MAX_NUM] = {0};

  // For auto stream
  uint32_t auto_stream[AM_STREAM_MAX_NUM] = {0};
  uint32_t auto_stream_count = 0;
  uint32_t assigned_buffer_num = 0;

  struct __buffer_msg {
      bool enable;
      AM_STREAM_ID stream_id;
  } buffer_msg[AM_ENCODE_SOURCE_BUFFER_MAX_NUM] = {0};

  AM_ENCODE_SOURCE_BUFFER_ID
  buffer_assign_order[AM_ENCODE_SOURCE_BUFFER_MAX_NUM] =
  {AM_ENCODE_SOURCE_BUFFER_MAIN,
   AM_ENCODE_SOURCE_BUFFER_4TH,
   AM_ENCODE_SOURCE_BUFFER_2ND,
   AM_ENCODE_SOURCE_BUFFER_3RD};

  AMResolution buffer_auto_resolution[AM_ENCODE_SOURCE_BUFFER_MAX_NUM] =
  {
   {1920, 1080}, // main
   {640, 360},   // 2nd, MAX 720x576
   {640, 360},   // 3rd, MAX 1920x1080
   {1280, 720}   // 4th
  };

  vin_size = get_vin_resolution(AM_VIN_ID(0)); //FIXME: Hard code VIN0
  vin_gcd = get_gcd(vin_size.width, vin_size.height);
  vin_ratio_num = vin_size.width / vin_gcd;
  vin_ratio_den = vin_size.height / vin_gcd;

  // Main buffer can't exceed VIN size
  if ((buffer_auto_resolution[buffer_assign_order[0]].width >
        vin_size.width) ||
      (buffer_auto_resolution[buffer_assign_order[0]].height >
        vin_size.height)) {
    buffer_auto_resolution[buffer_assign_order[0]] = vin_size;
  }

  // Copy stream format and config only, set all other to be zero
  memset(&m_auto_encode_param, 0, sizeof(m_auto_encode_param));
  m_auto_encode_param.stream_format = m_config->m_loaded.stream_format;
  m_auto_encode_param.stream_config = m_config->m_loaded.stream_config;

  // Check stream
  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (!m_auto_encode_param.stream_format.encode_format[i].enable) {
      continue;
    }
    stream_width[i] =
        &(m_auto_encode_param.stream_format.encode_format[i].width);
    stream_height[i] =
        &(m_auto_encode_param.stream_format.encode_format[i].height);

    if (m_auto_encode_param.stream_format.encode_format[i].type ==
        AM_ENCODE_STREAM_TYPE_MJPEG) {
      stream_order_mjpeg[mjpeg_count++] = i;
      continue;
    }
    if (((int32_t)(*stream_width[i]) < 0) ||
        ((int32_t)(*stream_height[i]) < 0)) { //Auto Stream
      auto_stream[auto_stream_count++] = i;
    } else {
      if (*stream_width[i] % 16 != 0 ||
          *stream_height[i] % 8 != 0) {
        ERROR("Stream[%d]:"
            " Width must be multiple of 16 and height must be multiple of 8.",
            i);
        return AM_RESULT_ERR_INVALID;
      }
      stream_order_h264[h264_count++] = i;

      stream_gcd[i] = get_gcd(*stream_width[i], *stream_height[i]);
      stream_ratio_num[i] = *stream_width[i] / stream_gcd[i];
      stream_ratio_den[i] = *stream_height[i] / stream_gcd[i];

      if (stream_ratio_num[i] != vin_ratio_num ||
          stream_ratio_den[i] != vin_ratio_den) {
        WARN("Stream%d is not %d:%d, please check!",
             i, vin_ratio_num, vin_ratio_den);
      }
    }
  }

  if (mjpeg_count > 1) {
    ERROR("MJPEG can't more than 2 streams!");
    return AM_RESULT_ERR_INVALID;
  }

  // Bubble Sort for h264 stream
  for (int32_t j = 0; j < (int32_t)h264_count - 1; ++j) {
    for (int32_t i = 0; i < (int32_t)h264_count - 1 - j; ++i) {
      uint32_t width_1 =
          *stream_width[stream_order_h264[i]];
      uint32_t width_2 =
          *stream_width[stream_order_h264[i+1]];
      if (width_1 < width_2) {
        uint32_t tmp = stream_order_h264[i];
        stream_order_h264[i] = stream_order_h264[i+1];
        stream_order_h264[i+1] = tmp;
      }
    }
  }

  if (auto_stream_count > 0) {
    // Assign non-auto stream to buffer
    int32_t no_assign_stream;
    for (int32_t i = no_assign_stream = h264_count - 1,
        j = AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 2;
        (i >= 0) && (j >= 0);
        --j) {
      if ((*stream_width[stream_order_h264[i]] <=
          g_source_buffer_max_size_default[buffer_assign_order[j]].width) &&
          (*stream_height[stream_order_h264[i]] <=
              g_source_buffer_max_size_default[buffer_assign_order[j]].height)) {
        buffer_msg[buffer_assign_order[j]].enable = true;
        buffer_msg[buffer_assign_order[j]].stream_id =
            AM_STREAM_ID(stream_order_h264[i]);
        --i;
        --no_assign_stream;
        ++assigned_buffer_num;
      } else {
        continue;
      }
    }

    if (assigned_buffer_num < h264_count) {
      ERROR("Stream[%d]: %dx%d is too large! Please check!, ",
            stream_order_h264[no_assign_stream],
            *stream_width[stream_order_h264[no_assign_stream]],
            *stream_height[stream_order_h264[no_assign_stream]]);
      return AM_RESULT_ERR_INVALID;
    }

    // Assign auto stream to buffer
    for (uint32_t i = 0; i < auto_stream_count; ++i) {
      for (uint32_t j = 0; j < AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 1; ++j) {
        if (!buffer_msg[buffer_assign_order[j]].enable) {
          *stream_width[auto_stream[i]] =
              buffer_auto_resolution[buffer_assign_order[j]].width;
          *stream_height[auto_stream[i]] =
              buffer_auto_resolution[buffer_assign_order[j]].height;
          buffer_msg[buffer_assign_order[j]].enable = true;
          buffer_msg[buffer_assign_order[j]].stream_id =
              AM_STREAM_ID(auto_stream[i]);
          break;
        }
      }
    }
  } else {
    // assign h264 stream to buffer
    for (uint32_t i = 0,
        stream_id = stream_order_h264[i],
        buffer_id = buffer_assign_order[0];
        i < h264_count ;
        ++i, stream_id = stream_order_h264[i],
            buffer_id = buffer_assign_order[i]) {

      buffer_msg[buffer_id].enable = true;
      buffer_msg[buffer_id].stream_id = AM_STREAM_ID(stream_id);
    }
  }

  // Assign mjpeg to last order buffer, now it's 3rd buffer
  if (mjpeg_count == 1) {
    if ((*stream_width[stream_order_mjpeg[0]] >
    *stream_width[buffer_msg[buffer_assign_order[0]].stream_id]) ||
        (*stream_height[stream_order_mjpeg[0]] >
    *stream_height[buffer_msg[buffer_assign_order[0]].stream_id])) {
      WARN("MJPEG exceed MAX H264 stream size, please check!");
    }

    if (((int32_t)*stream_width[stream_order_mjpeg[0]] < 0) ||
        ((int32_t)*stream_height[stream_order_mjpeg[0]] < 0)) {
      *stream_width[stream_order_mjpeg[0]] =
          buffer_auto_resolution\
          [buffer_assign_order[AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 1]].width;
      *stream_height[stream_order_mjpeg[0]] =
          buffer_auto_resolution\
          [buffer_assign_order[AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 1]].height;
    }
    buffer_msg[buffer_assign_order\
               [AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 1]].enable = true;
    buffer_msg[buffer_assign_order\
               [AM_ENCODE_SOURCE_BUFFER_MAX_NUM - 1]].stream_id =
        AM_STREAM_ID(stream_order_mjpeg[0]);
  }

  // Real assign action
  for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
    if (buffer_msg[i].enable) {
      stream_gcd[buffer_msg[i].stream_id] =
          get_gcd(*stream_width[buffer_msg[i].stream_id],
                  *stream_height[buffer_msg[i].stream_id]);
      stream_ratio_num[buffer_msg[i].stream_id] =
          *stream_width[buffer_msg[i].stream_id] /
          stream_gcd[buffer_msg[i].stream_id];
      stream_ratio_den[buffer_msg[i].stream_id] =
          *stream_height[buffer_msg[i].stream_id] /
          stream_gcd[buffer_msg[i].stream_id];
      if (i == 0 &&
          (stream_ratio_num[buffer_msg[i].stream_id] != vin_ratio_num ||
              stream_ratio_den[buffer_msg[i].stream_id] != vin_ratio_den)) {
        ERROR("MAX Stream resolution: %dx%d must be %d:%d, please modify!",
              *stream_width[buffer_msg[i].stream_id],
              *stream_height[buffer_msg[i].stream_id],
              vin_ratio_num, vin_ratio_den);
        return AM_RESULT_ERR_INVALID;
      } else if ((*stream_width[buffer_msg[i].stream_id] >
      g_source_buffer_max_size_default[i].width)||
          (*stream_height[buffer_msg[i].stream_id] >
      g_source_buffer_max_size_default[i].height)) {
        ERROR("Stream[%d]: %dx%d can't exceed buffer[%d] max size: %dx%d",
              buffer_msg[i].stream_id,
              *stream_width[buffer_msg[i].stream_id],
              *stream_height[buffer_msg[i].stream_id],
              i,
              g_source_buffer_max_size_default[i].width,
              g_source_buffer_max_size_default[i].height);
        return AM_RESULT_ERR_INVALID;
      }

      assign_stream_to_buffer(buffer_msg[i].stream_id,
                              AM_ENCODE_SOURCE_BUFFER_ID(i));
    }
  }

#if 0
  // For debug
  for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
    if (!buffer_msg[buffer_assign_order[i]].enable) {
      continue;
    }
    PRINTF("\nSource[%d]: "
        "input: %dx%d, size: %dx%d, offset: %dx%d\n"
        "Stream[%d]: type: %d, size: %dx%d\n",
        buffer_assign_order[i],
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].input.width,
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].input.height,
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].size.width,
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].size.height,
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].input.x,
        m_auto_encode_param.buffer_format.\
        source_buffer_format[buffer_assign_order[i]].input.y,
        buffer_msg[buffer_assign_order[i]].stream_id,
        m_auto_encode_param.stream_format.\
        encode_format[buffer_msg[buffer_assign_order[i]].stream_id].type,
        *stream_width[buffer_msg[buffer_assign_order[i]].stream_id],
        *stream_height[buffer_msg[buffer_assign_order[i]].stream_id]);
  }
#endif

  return AM_RESULT_OK;
}
