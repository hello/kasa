/*******************************************************************************
 * am_dptz_warp.cpp
 *
 * History:
 *   2014-11-13 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/


#include "iav_ioctl.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "am_base_include.h"
#include "am_define.h"
#include "am_encode_device.h"
#include "am_encode_buffer.h"
#include "am_log.h"
#include "am_dptz_warp.h"

#include "lib_dewarp_header.h"
#include "lib_dewarp.h"

AMDPTZWarp::AMDPTZWarp():
  m_encode_device(NULL),
  m_iav(-1),
  m_init_done(false),
  m_is_warp_mapped(false),
  m_pixel_width_um(0),
  m_zoom_source(AM_WARP_ZOOM_DPTZ),
  m_is_ldc_enable(false),
  m_is_ldc_checked(false)
{
  m_dptz_warp_buf_id = AM_ENCODE_SOURCE_BUFFER_MAIN;
  memset(&m_dptz_param, 0, sizeof(m_dptz_param));
  memset(&m_warp_param, 0, sizeof(m_warp_param));
  memset(&m_warp_mem, 0, sizeof(m_warp_mem));
  memset(&m_lens_warp_vector, 0, sizeof(m_lens_warp_vector));
  memset(&m_dewarp_init_param, 0, sizeof(m_dewarp_init_param));
  memset(&m_distortion_lut, 0, sizeof(int) * MAX_DISTORTION_LOOKUP_TABLE_ENTRY_NUM);
}

AMDPTZWarp::~AMDPTZWarp()
{
}

bool AMDPTZWarp::is_ldc_enable()
{
  struct iav_system_resource resource;
  if (!m_is_ldc_checked) {
    memset(&resource, 0, sizeof(resource));
    resource.encode_mode = DSP_CURRENT_MODE;
    ioctl(m_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
    m_is_ldc_enable = resource.lens_warp_enable;
    m_is_ldc_checked = true;
  }
  return m_is_ldc_enable;
}

AM_RESULT AMDPTZWarp::init(int fd_iav, AMEncodeDevice *encode_device)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!encode_device) {
      ERROR("AMDPTZWarp:: init error \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (fd_iav < 0) {
      ERROR("AMDPTZWarp: wrong iav handle to init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_iav = fd_iav;
    m_encode_device = encode_device;
    m_init_done = true;
  } while (0);
  return result;
}

AM_RESULT AMDPTZWarp::set_config(AMDPTZWarpConfig *dptz_warp_config)
{
  AM_RESULT result = AM_RESULT_OK;
  uint32_t i = 0;
  do {
    if (!dptz_warp_config) {
      ERROR("AMDPTZWarp::load_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_dptz_warp_buf_id = (AM_ENCODE_SOURCE_BUFFER_ID)dptz_warp_config->buf_cfg;
    if (m_dptz_warp_buf_id < AM_ENCODE_SOURCE_BUFFER_FIRST ||
        m_dptz_warp_buf_id > AM_ENCODE_SOURCE_BUFFER_LAST ) {
      ERROR("AMDPTZWarp::buf_id %u is invalid, should be 0~3\n",
             m_dptz_warp_buf_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    i = m_dptz_warp_buf_id;
    if (dptz_warp_config->dptz_cfg[i].pan_ratio > 1.0
       || dptz_warp_config->dptz_cfg[i].pan_ratio < -1.0
       || dptz_warp_config->dptz_cfg[i].tilt_ratio > 1.0
       || dptz_warp_config->dptz_cfg[i].tilt_ratio < -1.0
       || dptz_warp_config->dptz_cfg[i].zoom_ratio > 8.0
       || dptz_warp_config->dptz_cfg[i].zoom_ratio < 0.1
       || dptz_warp_config->warp_cfg.ldc_strength > 20.0
       || dptz_warp_config->warp_cfg.ldc_strength < 0.0
       || dptz_warp_config->warp_cfg.pano_hfov_degree > 180.0
       || dptz_warp_config->warp_cfg.pano_hfov_degree < 1.0) {
        ERROR("AMDPTZWarp:: dptz warp parameters invalid");
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    m_warp_param.warp_mode =
        (AM_WARP_MODE) (dptz_warp_config->warp_cfg.warp_mode);
    m_warp_param.lens_param.max_hfov_degree =
        dptz_warp_config->warp_cfg.ldc_strength * 10;
    m_warp_param.lens_param.pano_hfov_degree =
        dptz_warp_config->warp_cfg.pano_hfov_degree;
    m_warp_param.lens_param.efl_mm = 2.1;
    m_warp_param.proj_mode =
        (AM_WARP_PROJECTION_MODE) dptz_warp_config->warp_cfg.proj_mode;
    if (is_ldc_enable()) {
      result = set_warp_param(&m_warp_param);
      if (result != AM_RESULT_OK) {
        ERROR("AMDPTZWarp:: set warp parameters failed");
        break;
      }
    }
    m_dptz_param[i].dptz_ratio.zoom_center_pos_x =
        dptz_warp_config->dptz_cfg[i].pan_ratio;
    m_dptz_param[i].dptz_ratio.zoom_center_pos_y =
        dptz_warp_config->dptz_cfg[i].tilt_ratio;
    m_dptz_param[i].dptz_ratio.zoom_factor_x =
        dptz_warp_config->dptz_cfg[i].zoom_ratio;
    m_dptz_param[i].dptz_ratio.zoom_factor_y =
        dptz_warp_config->dptz_cfg[i].zoom_ratio;
    result = set_dptz_ratio((AM_ENCODE_SOURCE_BUFFER_ID)i,
                            &m_dptz_param[i].dptz_ratio);
    if (result != AM_RESULT_OK) {
      ERROR("AMDPTZWarp:: set dptz ratio failed");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::set_config_all(AMDPTZWarpConfig *dptz_warp_config)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!dptz_warp_config) {
      ERROR("AMDPTZWarp::load_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_dptz_warp_buf_id = (AM_ENCODE_SOURCE_BUFFER_ID)dptz_warp_config->buf_cfg;
    if (m_dptz_warp_buf_id < AM_ENCODE_SOURCE_BUFFER_FIRST ||
        m_dptz_warp_buf_id > AM_ENCODE_SOURCE_BUFFER_LAST ) {
      ERROR("AMDPTZWarp::buf_id %u is invalid, should be 0~3\n", m_dptz_warp_buf_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (dptz_warp_config->warp_cfg.ldc_strength > 20.0
       || dptz_warp_config->warp_cfg.ldc_strength < 0.0) {
        ERROR("AMDPTZWarp:: warp parameters invalid");
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    m_warp_param.warp_mode =
        (AM_WARP_MODE) (dptz_warp_config->warp_cfg.warp_mode);
    m_warp_param.lens_param.max_hfov_degree =
        dptz_warp_config->warp_cfg.ldc_strength * 10;
    m_warp_param.lens_param.pano_hfov_degree =
        dptz_warp_config->warp_cfg.pano_hfov_degree;
    m_warp_param.lens_param.efl_mm = 2.1;
    m_warp_param.proj_mode =
        (AM_WARP_PROJECTION_MODE) dptz_warp_config->warp_cfg.proj_mode;
    if (is_ldc_enable()) {
      result = set_warp_param(&m_warp_param);
      if (result != AM_RESULT_OK) {
        ERROR("AMDPTZWarp:: set warp parameters failed");
        break;
      }
    }

    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
      if (dptz_warp_config->dptz_cfg[i].pan_ratio > 1.0
         || dptz_warp_config->dptz_cfg[i].pan_ratio < -1.0
         || dptz_warp_config->dptz_cfg[i].tilt_ratio > 1.0
         || dptz_warp_config->dptz_cfg[i].tilt_ratio < -1.0
         || dptz_warp_config->dptz_cfg[i].zoom_ratio > 8.0
         || dptz_warp_config->dptz_cfg[i].zoom_ratio < 0.1) {
          ERROR("AMDPTZWarp:: dptz parameters invalid");
          result = AM_RESULT_ERR_INVALID;
          break;
      }
      m_dptz_param[i].dptz_ratio.zoom_center_pos_x =
          dptz_warp_config->dptz_cfg[i].pan_ratio;
      m_dptz_param[i].dptz_ratio.zoom_center_pos_y =
          dptz_warp_config->dptz_cfg[i].tilt_ratio;
      m_dptz_param[i].dptz_ratio.zoom_factor_x =
          dptz_warp_config->dptz_cfg[i].zoom_ratio;
      m_dptz_param[i].dptz_ratio.zoom_factor_y =
          dptz_warp_config->dptz_cfg[i].zoom_ratio;
      result = set_dptz_ratio((AM_ENCODE_SOURCE_BUFFER_ID)i,
                              &m_dptz_param[i].dptz_ratio);
      if (result != AM_RESULT_OK) {
        ERROR("AMDPTZWarp:: set dptz ratio failed");
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::get_config(AMDPTZWarpConfig *dptz_warp_config)
{
  AM_RESULT result = AM_RESULT_OK;
  uint32_t bid = 0;
  do {
    if (!dptz_warp_config) {
      ERROR("AMDPTZWarp::get_config error");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    bid = dptz_warp_config->buf_cfg;
    if (bid < AM_ENCODE_SOURCE_BUFFER_FIRST ||
        bid > AM_ENCODE_SOURCE_BUFFER_LAST ) {
      ERROR("AMDPTZWarp::get_config buf_id %u is invalid, should be 0~3\n", bid);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    dptz_warp_config->warp_cfg.warp_mode = (uint32_t) m_warp_param.warp_mode;
    dptz_warp_config->warp_cfg.ldc_strength =
        m_warp_param.lens_param.max_hfov_degree / 10;
    dptz_warp_config->warp_cfg.pano_hfov_degree =
        m_warp_param.lens_param.pano_hfov_degree;
    dptz_warp_config->warp_cfg.proj_mode = (uint32_t) m_warp_param.proj_mode;

    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
      dptz_warp_config->dptz_cfg[i].pan_ratio =
          m_dptz_param[i].dptz_ratio.zoom_center_pos_x;
      dptz_warp_config->dptz_cfg[i].tilt_ratio =
          m_dptz_param[i].dptz_ratio.zoom_center_pos_y;
      dptz_warp_config->dptz_cfg[i].zoom_ratio =
          m_dptz_param[i].dptz_ratio.zoom_factor_x;
    }
  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::set_dptz_ratio(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                     AMDPTZRatio *dptz_ratio)
{
  AM_RESULT result = AM_RESULT_OK;
  AMDPTZParam dptz_param;
  do {
    if (!dptz_ratio) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(&dptz_param, 0, sizeof(dptz_param));
    dptz_param.dptz_method = AM_DPTZ_CONTROL_BY_DPTZ_RATIO;
    dptz_param.dptz_ratio = *dptz_ratio;


    //Validate Data Range before set

    if (!is_ldc_enable() || (is_ldc_enable() && buf_id != AM_ENCODE_SOURCE_BUFFER_MAIN)) {
      //ZOOM 1X is defined to be full FOV without any zoom-in.
      //so ZOOM can never be smaller than 1X
      if ((dptz_ratio->zoom_factor_x < 1) || (dptz_ratio->zoom_factor_y < 1)) {
        ERROR("AMDPTZWarp: unable to support zoom factor smaller than 1 \n");
        ERROR("AMDPTZWarp: buf_id = %d, zoom_factor_x =%f, zoom_factor_y=%f \n", buf_id, dptz_ratio->zoom_factor_x, dptz_ratio->zoom_factor_y);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    }
    //limit max zoom factor, so that it won't let input window to be zero size
    if ((dptz_ratio->zoom_factor_x > AM_MAX_DZOOM_FACTOR_X ) ||
        (dptz_ratio->zoom_factor_y > AM_MAX_DZOOM_FACTOR_Y)) {
      ERROR("AMDPTZWarp: unable to support zoom factor smaller than 1 \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //check zoom center pos
    if ((AM_ABS(dptz_ratio->zoom_center_pos_x) > 1) ||
        (AM_ABS(dptz_ratio->zoom_center_pos_y) > 1) ){
      ERROR("AMDPTZWarp: unable to support zoom offset out of (-1,1) scope \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = set_dptz_param(buf_id, &dptz_param);
    m_zoom_source = AM_WARP_ZOOM_DPTZ;
  } while (0);
  return result;
}

AMEncodeSourceBufferFormat *AMDPTZWarp::get_source_buffer_format(AM_ENCODE_SOURCE_BUFFER_ID id)
{
  AMIEncodeDeviceConfig *dev_cfg = NULL;
  AMEncodeParamAll *param = NULL;
  AMEncodeSourceBufferFormat *buf_fmt = NULL;
  do {
    dev_cfg = m_encode_device->get_encode_config();
    if (!dev_cfg) {
      ERROR("AMDPTZWarp: get encode config error\n");
      break;
    }
    param = dev_cfg->get_encode_param();
    if (!param) {
      ERROR("AMDPTZWarp: get encode param error\n");
      break;
    }
    buf_fmt = &param->buffer_format.source_buffer_format[id];
  } while (0);
  return buf_fmt;
}

AM_RESULT AMDPTZWarp::get_input_buffer_size(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                            AMResolution *input_res)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    AMResolution vin_res;
    AMEncodeSourceBufferFormat buffer_format;
    AMEncodeSourceBuffer *source_buf = NULL;
    if (!input_res) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
      vin_res = m_encode_device->get_vin_resolution(AM_VIN_0);
      INFO("AMDPTZWarp: VIN resolution: %dx%d \n",
           vin_res.width,
           vin_res.height);
      *input_res = vin_res;
    } else {
      source_buf = m_encode_device->get_source_buffer(AM_ENCODE_SOURCE_BUFFER_MAIN);
      if (!source_buf) {
        ERROR("AMDPTZWarp: get source buf error\n");
        result = AM_RESULT_ERR_DSP;
        break;
      }
      result = source_buf->get_source_buffer_format(&buffer_format);
      if (result != AM_RESULT_OK) {
        break;
      }
      *input_res = buffer_format.size;
    }
  } while (0);
  return result;
}

AM_RESULT AMDPTZWarp::set_dptz_input_window(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                            AMRect *input_window)
{
  AM_RESULT result = AM_RESULT_OK;
  AMResolution input_res;
  AMDPTZParam dptz_param;
  do {
    if (!input_window) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    /* Main Input Window is calculated by dewarp library, should not be set by Oryx */
    if (is_ldc_enable() && buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
      ERROR("Main Input Window can not be set when LDC is enabled !\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(&dptz_param, 0, sizeof(dptz_param));
    dptz_param.dptz_method = AM_DPTZ_CONTROL_BY_INPUT_WINDOW;
    dptz_param.dptz_input_window = *input_window;

    //valid date before setup
    result = get_input_buffer_size(buf_id, &input_res);
    if (result != AM_RESULT_OK) {
      break;
    }

    if ((dptz_param.dptz_input_window.x < 0)
        || (dptz_param.dptz_input_window.width <= AM_MIN_DZOOM_INPUT_RECT_WIDTH)
        || (dptz_param.dptz_input_window.y < 0)
        || (dptz_param.dptz_input_window.height <= AM_MIN_DZOOM_INPUT_RECT_HEIGHT)
        || (dptz_param.dptz_input_window.x + dptz_param.dptz_input_window.width
            > input_res.width)
            || (dptz_param.dptz_input_window.y + dptz_param.dptz_input_window.height
                > input_res.height)) {
      ERROR("AMDPTZWarp: input window is wrong \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    result = set_dptz_param(buf_id, &dptz_param);
  } while (0);
  return result;
}

AM_RESULT AMDPTZWarp::set_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                     AMDPTZParam *dptz_param)
{
  AM_RESULT result = AM_RESULT_OK;
  int id;
  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    //check buf id
    if (((int) (buf_id) < (int) AM_ENCODE_SOURCE_BUFFER_FIRST)
        || ((int) (buf_id) > (int) AM_ENCODE_SOURCE_BUFFER_LAST)) {
      ERROR("AMDPTZWarp: buffer id wrong\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!dptz_param) {
      ERROR("AMDPTZWarp: wrong dptz_param\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    id = (int) buf_id;
    memcpy(&m_dptz_param[id], dptz_param, sizeof(AMDPTZParam));
  } while (0);
  return result;
}


AM_RESULT AMDPTZWarp::get_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                     AMDPTZParam *dptz_param) /* both ratio and input_window will be filled when calling get_xxx */
{
  AMResolution input_res;
  AM_RESULT result = AM_RESULT_OK;
  int id;
  struct iav_digital_zoom digital_zoom;
  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    //check buf id
    if (((int) (buf_id) < (int) AM_ENCODE_SOURCE_BUFFER_FIRST)
        || ((int) (buf_id) > (int) AM_ENCODE_SOURCE_BUFFER_LAST)) {
      ERROR("AMDPTZWarp: buffer id wrong\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!dptz_param) {
      ERROR("AMDPTZWarp: wrong dptz_param\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    id = (int) buf_id;
    memset(&digital_zoom, 0, sizeof(digital_zoom));
    digital_zoom.buf_id = id;
    if (ioctl(m_iav, IAV_IOC_GET_DIGITAL_ZOOM, &digital_zoom) < 0) {
      PERROR("IAV_IOC_GET_DIGITAL_ZOOM");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    result = get_input_buffer_size(buf_id, &input_res);
    if (result != AM_RESULT_OK) {
      break;
    }

    //refresh m_dptz_param from IAV driver too
    m_dptz_param[id].dptz_input_window.x = digital_zoom.input.x;
    m_dptz_param[id].dptz_input_window.y = digital_zoom.input.y;
    m_dptz_param[id].dptz_input_window.width = digital_zoom.input.width;
    m_dptz_param[id].dptz_input_window.height = digital_zoom.input.height;

    //check value validity
    if ((digital_zoom.input.width < AM_MIN_DZOOM_INPUT_RECT_WIDTH) ||
        (digital_zoom.input.height < AM_MIN_DZOOM_INPUT_RECT_HEIGHT)) {
      ERROR("AMDPTZWarp: digital zoom input window too small, abnormal\n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    //calculate ratio

    if (id == AM_ENCODE_SOURCE_BUFFER_MAIN && is_ldc_enable()) {
      if (m_zoom_source == AM_WARP_ZOOM_LDC) {
        m_dptz_param[id].dptz_ratio.zoom_factor_x = (m_warp_param.lens_param.zoom.num + 0.0)
          / m_warp_param.lens_param.zoom.denom;
        m_dptz_param[id].dptz_ratio.zoom_factor_y = m_dptz_param[id].dptz_ratio.zoom_factor_x;
      } else if (m_zoom_source == AM_WARP_ZOOM_DPTZ) {
        m_dptz_param[id].dptz_ratio.zoom_factor_x = m_dptz_param[AM_ENCODE_SOURCE_BUFFER_MAIN].dptz_ratio.zoom_factor_x;
        m_dptz_param[id].dptz_ratio.zoom_factor_y = m_dptz_param[id].dptz_ratio.zoom_factor_x;
      } else {
        m_dptz_param[id].dptz_ratio.zoom_factor_x = m_dptz_param[id].dptz_ratio.zoom_factor_y = 1;
      }
    } else {
      m_dptz_param[id].dptz_ratio.zoom_factor_x = ((float)input_res.width)/digital_zoom.input.width;
      m_dptz_param[id].dptz_ratio.zoom_factor_y = ((float)input_res.height)/digital_zoom.input.height;

      if ((digital_zoom.input.x > 0) && (m_dptz_param[id].dptz_ratio.zoom_factor_x > 1)) {
        m_dptz_param[id].dptz_ratio.zoom_center_pos_x =
            (digital_zoom.input.x * m_dptz_param[id].dptz_ratio.zoom_factor_x/
                (m_dptz_param[id].dptz_ratio.zoom_factor_x -1.0f)) * 2 /input_res.width - 1.0f;
      } else {
        m_dptz_param[id].dptz_ratio.zoom_center_pos_x = 0;
      }

      if ((digital_zoom.input.y > 0) && (m_dptz_param[id].dptz_ratio.zoom_factor_y > 1)) {
        m_dptz_param[id].dptz_ratio.zoom_center_pos_y =
            (digital_zoom.input.y * m_dptz_param[id].dptz_ratio.zoom_factor_y/
                (m_dptz_param[id].dptz_ratio.zoom_factor_y -1.0f)) * 2 /input_res.height - 1.0f;
      } else {
        m_dptz_param[id].dptz_ratio.zoom_center_pos_y = 0;
      }
    }

    memcpy(dptz_param, &m_dptz_param[id], sizeof(AMDPTZParam));
  } while (0);
  return result;
}


AM_RESULT AMDPTZWarp::set_dptz_easy(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                    AM_EASY_ZOOM_MODE mode) /* just set, no get, because easy set only has two choices */
{
  AM_RESULT result = AM_RESULT_OK;
  int id;
  AMDPTZParam param;
  AMResolution input_res, current_buf_res;
  AMEncodeSourceBuffer *source_buf = NULL;
  AMEncodeSourceBufferFormat buffer_format;
  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    /* Main Input Window is calculated by dewarp library, should not be set my Oryx */
    if (is_ldc_enable() && buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
      ERROR("Main Input Window can not be set when LDC is enabled !\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    id = (int) buf_id;

    //check buf id
    if ((id < (int) AM_ENCODE_SOURCE_BUFFER_FIRST)
        || (id > (int) AM_ENCODE_SOURCE_BUFFER_LAST)) {
      ERROR("AMDPTZWarp: buffer id wrong\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //calculate the dptz param by mode
    memset(&param, 0, sizeof(param));
    param.dptz_method = AM_DPTZ_CONTROL_BY_INPUT_WINDOW;
    result = get_input_buffer_size(buf_id, &input_res);
    if (result != AM_RESULT_OK) {
      break;
    }

    if (mode == AM_EASY_ZOOM_MODE_FULL_FOV) {
      //for full FOV, x/f offset must be zero
      param.dptz_input_window.x = 0;
      param.dptz_input_window.y = 0;
      param.dptz_input_window.width = input_res.width;
      param.dptz_input_window.height = input_res.height;
    } else if (mode == AM_EASY_ZOOM_MODE_PIXEL_TO_PIXEL) {
      source_buf = m_encode_device->get_source_buffer(buf_id);
      if (!source_buf) {
        ERROR("AMDPTZWarp: get source buf error\n");
        result = AM_RESULT_ERR_DSP;
        break;
      }
      result = source_buf->get_source_buffer_format(&buffer_format);
      if (result != AM_RESULT_OK) {
        break;
      }
      current_buf_res = buffer_format.size;
      //for Pixel to Pixel DZ, put it center oriented
      param.dptz_input_window.x = (input_res.width - current_buf_res.width)/2;
      param.dptz_input_window.y = (input_res.height - current_buf_res.height)/2;
      param.dptz_input_window.width = current_buf_res.width;
      param.dptz_input_window.height = current_buf_res.height;
    } else {
      WARN("AMDPTZWarp: AM_EASY_ZOOM_MODE_AMPLIFY not supported yet \n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    m_dptz_param[id] = param;
  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::set_warp_param(AMWarpParam *warp_param)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_querybuf querybuf;
  uint32_t warp_size;
  void *warp_mem;
  AMEncodeSourceBufferFormat buffer_format;
  struct vindev_aaa_info vindev_aaa_info;
  FILE *fp = NULL;
  char line[1024] = {0};
  int i = 0;

  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!is_ldc_enable()) {
      ERROR("LDC is not Enabled \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    // do something about set warp
    if (!warp_param) {
      ERROR("AMDPTZWarp: empty warp_param\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //Map warp partition
    if (!m_is_warp_mapped) {
      querybuf.buf = IAV_BUFFER_WARP;
      if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
        ERROR("IAV_IOC_QUERY_BUF");
        result = AM_RESULT_ERR_DSP;
        break;
      }

      warp_size = querybuf.length;
      warp_mem = mmap(NULL, warp_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_iav, querybuf.offset);

      if (warp_mem == MAP_FAILED) {
        ERROR("mmap (%d) failed: %s\n");
        result = AM_RESULT_ERR_IO;
        break;
      }

      m_warp_mem.addr = (uint8_t *) warp_mem;
      m_warp_mem.length = warp_size;
      m_is_warp_mapped = true;
      INFO("warp_mem = 0x%x, size = 0x%x\n", (uint32_t ) warp_mem, warp_size);
    }

    if (!m_pixel_width_um) {
      if (ioctl(m_iav, IAV_IOC_VIN_GET_AAAINFO, &vindev_aaa_info) < 0) {
        ERROR("IAV_IOC_VIN_GET_AAAINFO failed");
        result = AM_RESULT_ERR_INVALID;
        break;
      }

      m_pixel_width_um = (vindev_aaa_info.pixel_size >> 16) + (1.0 / 0x10000) * (vindev_aaa_info.pixel_size & 0xFFFF);
    }

    m_lens_warp_vector.hor_map.addr = (data_t*) m_warp_mem.addr;
    m_lens_warp_vector.ver_map.addr = (data_t*) (m_warp_mem.addr
      + MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t));
    m_lens_warp_vector.me1_ver_map.addr = (data_t*) (m_warp_mem.addr
      + MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t) * 2);

    m_dewarp_init_param.max_input_width = m_encode_device->get_vin_resolution(AM_VIN_0).width;
    m_dewarp_init_param.max_input_height = m_encode_device->get_vin_resolution(AM_VIN_0).height;
    m_encode_device->get_source_buffer(AM_ENCODE_SOURCE_BUFFER_MAIN)->get_source_buffer_format(&buffer_format);
    m_dewarp_init_param.main_buffer_width = buffer_format.size.width;

    m_dewarp_init_param.projection_mode = (PROJECTION_MODE)warp_param->proj_mode;
    m_dewarp_init_param.max_fov = warp_param->lens_param.max_hfov_degree;
    m_dewarp_init_param.max_radius = m_dewarp_init_param.max_input_width / 2;
    m_dewarp_init_param.lut_focal_length = (int) (warp_param->lens_param.efl_mm * 1000 / m_pixel_width_um);

    if (warp_param->lens_param.lens_center_in_max_input.x) {
      m_dewarp_init_param.lens_center_in_max_input.x = warp_param->lens_param.lens_center_in_max_input.x;
    } else {
      m_dewarp_init_param.lens_center_in_max_input.x = m_dewarp_init_param.max_input_width / 2;
    }

    if (warp_param->lens_param.lens_center_in_max_input.y) {
      m_dewarp_init_param.lens_center_in_max_input.y = warp_param->lens_param.lens_center_in_max_input.y;
    } else {
      m_dewarp_init_param.lens_center_in_max_input.y = m_dewarp_init_param.max_input_height / 2;
    }

    // Load LUT
    if (warp_param->proj_mode == AM_WARP_PROJECTION_LOOKUP_TABLE && warp_param->lens_param.lut_file[0]) {
      if ((fp = fopen(warp_param->lens_param.lut_file, "r")) == NULL) {
        ERROR("Failed to open file [%s].\n", warp_param->lens_param.lut_file);
        result = AM_RESULT_ERR_IO;
        break;
      }

      while (fgets(line, sizeof(line), fp) != NULL) {
        m_distortion_lut[i] = (int)(atof(line) * m_dewarp_init_param.max_input_width);
        i++;
      }
      if (fp) {
        fclose(fp);
        fp = NULL;
      }
      m_dewarp_init_param.lut_radius = (int *)m_distortion_lut;
    }

    if (result == AM_RESULT_OK) {
      m_warp_param = *warp_param;
      m_zoom_source = AM_WARP_ZOOM_LDC;
    }

  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::get_warp_param(AMWarpParam *warp_param)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    // do something about get warp
    if (!warp_param) {
      ERROR("AMDPTZWarp: empty warp_param\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    *warp_param = m_warp_param;

  } while (0);
  return result;
}

AM_RESULT AMDPTZWarp::dz_ratio_to_input_window(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                               AMDPTZRatio *dz_ratio,
                                               AMRect *input_window)
{
  AMResolution input_res, zoom_input_res;
  AM_RESULT result = AM_RESULT_OK;
  float zoom_center_x, zoom_center_y;
  int offset_x, offset_y;
  int tmp;

  do {

    if ((!dz_ratio) ||(!input_window)) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    result = get_input_buffer_size(buf_id, &input_res);
    if (result != AM_RESULT_OK) {
      break;
    }
    //width and height must be round UP to multiple of 4
    tmp = FLOAT_TO_INT(input_res.width/ dz_ratio->zoom_factor_x);
    zoom_input_res.width = ROUND_UP(tmp, 4);
    tmp = FLOAT_TO_INT(input_res.height/ dz_ratio->zoom_factor_y);
    zoom_input_res.height = ROUND_UP(tmp, 4);

    //check for exception, usually unlikely
    if ((zoom_input_res.width > input_res.width) ||
        (zoom_input_res.height > input_res.height)) {
      ERROR("AMDPTZWarp: unexpected, zoom input res %dx%d bigger than input\n",
            zoom_input_res.width, zoom_input_res.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //convert float zoom center into actual zoom_center, this could be half pixel
    // when pos_x = -1, it means left most, when pos_x = 1, it means right most
    // when pos_y = -1, it means top most, when  pos_y = 1, it means bottom most
    zoom_center_x = (dz_ratio->zoom_center_pos_x * 0.5f + 0.5f ) * input_res.width;
    zoom_center_y = (dz_ratio->zoom_center_pos_y * 0.5f + 0.5f ) * input_res.height;

    //after zoom, new
    offset_x = FLOAT_TO_INT(zoom_center_x - (zoom_center_x /dz_ratio->zoom_factor_x));
    offset_y = FLOAT_TO_INT(zoom_center_y - (zoom_center_y /dz_ratio->zoom_factor_y));

    //data range clamp
    if (offset_x < 0)
      offset_x = 0;
    if (offset_x > (int)input_res.width)
      offset_x = input_res.width;
    if (offset_y < 0)
      offset_y = 0;
    if (offset_y > (int)input_res.height)
      offset_y = input_res.height;

    //round down to even number
    offset_x = ROUND_DOWN(offset_x, 2);
    offset_y = ROUND_DOWN(offset_y, 2);

    input_window->x = offset_x;
    input_window->y = offset_y;
    input_window->width = zoom_input_res.width;
    input_window->height = zoom_input_res.height;

    //for debug, dump
    INFO("AMDPTZWarp: zoom input window x=%d, y=%d, w=%d, h=%d\n",
         input_window->x, input_window->y,
         input_window->width, input_window->height);
  } while (0);

  return result;
}

int AMDPTZWarp::get_grid_spacing(const int spacing)
{
  int grid_space = GRID_SPACING_PIXEL_64;
  switch (spacing) {
    case 16:
      grid_space = GRID_SPACING_PIXEL_16;
      break;
    case 32:
      grid_space = GRID_SPACING_PIXEL_32;
      break;
    case 64:
      grid_space = GRID_SPACING_PIXEL_64;
      break;
    case 128:
      grid_space = GRID_SPACING_PIXEL_128;
      break;
    default:
      grid_space = GRID_SPACING_PIXEL_64;
      break;
  }
  return grid_space;
}

AM_RESULT AMDPTZWarp::update_warp_control(struct iav_warp_main* p_control)
{
  struct iav_warp_area* p_area = &p_control->area[0];
  warp_vector_t *p_vector = &m_lens_warp_vector;

  p_area->enable = 1;
  p_area->input.width = p_vector->input.width;
  p_area->input.height = p_vector->input.height;
  p_area->input.x = p_vector->input.upper_left.x;
  p_area->input.y = p_vector->input.upper_left.y;
  p_area->output.width = p_vector->output.width;
  p_area->output.height = p_vector->output.height;
  p_area->output.x = p_vector->output.upper_left.x;
  p_area->output.y = p_vector->output.upper_left.y;
  p_area->rotate_flip = p_vector->rotate_flip;

  p_area->h_map.enable = (p_vector->hor_map.rows > 0
    && p_vector->hor_map.cols > 0);
  p_area->h_map.h_spacing =
    get_grid_spacing(p_vector->hor_map.grid_width);
  p_area->h_map.v_spacing =
    get_grid_spacing(p_vector->hor_map.grid_height);
  p_area->h_map.output_grid_row = p_vector->hor_map.rows;
  p_area->h_map.output_grid_col = p_vector->hor_map.cols;
  p_area->h_map.data_addr_offset = 0;

  p_area->v_map.enable = (p_vector->ver_map.rows > 0
      && p_vector->ver_map.cols > 0);
  p_area->v_map.h_spacing =
    get_grid_spacing(p_vector->ver_map.grid_width);
  p_area->v_map.v_spacing =
    get_grid_spacing(p_vector->ver_map.grid_height);
  p_area->v_map.output_grid_row = p_vector->ver_map.rows;
  p_area->v_map.output_grid_col = p_vector->ver_map.cols;
  p_area->v_map.data_addr_offset = MAX_WARP_TABLE_SIZE_LDC * sizeof(s16);

  // ME1 Warp grid / spacing
  p_area->me1_v_map.h_spacing =
    get_grid_spacing(p_vector->me1_ver_map.grid_width);
  p_area->me1_v_map.v_spacing =
    get_grid_spacing(p_vector->me1_ver_map.grid_height);
  p_area->me1_v_map.output_grid_row = p_vector->me1_ver_map.rows;
  p_area->me1_v_map.output_grid_col = p_vector->me1_ver_map.cols;
  p_area->me1_v_map.data_addr_offset = MAX_WARP_TABLE_SIZE_LDC * sizeof(s16) * 2;

  return AM_RESULT_OK;
}

AM_RESULT AMDPTZWarp::apply_dptz(AM_ENCODE_SOURCE_BUFFER_ID buf_id, AMDPTZParam *dptz_param)
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeSourceBufferFormat *buf_fmt = NULL;
  AMRect dptz_input_window;
  struct iav_digital_zoom digital_zoom;

  //convert ratio into input window
  //now we have input window
  do {
    buf_fmt = get_source_buffer_format(buf_id);
    if (buf_fmt->type == AM_ENCODE_SOURCE_BUFFER_TYPE_OFF) {
        WARN("source buffer type is \"off\"");
        break;
    }

    if (is_ldc_enable() && m_dptz_warp_buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
      if (buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
        WARN("LDC is enabled, DPTZ-I should not be set\n");
        break;
      }
    }
    if (dptz_param->dptz_method == AM_DPTZ_CONTROL_BY_DPTZ_RATIO) {
      dz_ratio_to_input_window(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                               &dptz_param->dptz_ratio,
                               &dptz_input_window);
    } else if (dptz_param->dptz_method == AM_DPTZ_CONTROL_BY_INPUT_WINDOW) {
      dptz_input_window = dptz_param->dptz_input_window;
    } else {
      //no change, skip it
      continue;
    }

    memset(&digital_zoom, 0, sizeof(digital_zoom));
    digital_zoom.buf_id = buf_id;
    digital_zoom.input.width = dptz_input_window.width;
    digital_zoom.input.height = dptz_input_window.height;
    digital_zoom.input.x = dptz_input_window.x;
    digital_zoom.input.y = dptz_input_window.y;

    //INFO
    INFO("AMDPTZWarp: source buffer %d, dz input width=%d, height=%d, x=%d, y=%d\n",
         digital_zoom.buf_id,
         digital_zoom.input.width,
         digital_zoom.input.height,
         digital_zoom.input.x,
         digital_zoom.input.y);

    if (ioctl(m_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
      PERROR("IAV_IOC_SET_DIGITAL_ZOOM");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMDPTZWarp::apply_warp()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeSourceBufferFormat buffer_format;
  struct iav_warp_ctrl warp_control;
  struct iav_warp_main* warp_main = &warp_control.arg.main;

  memset(&warp_control, 0, sizeof(struct iav_warp_ctrl));
  warp_control.cid = IAV_WARP_CTRL_MAIN;
  u32 flags = (1 << IAV_WARP_CTRL_MAIN);

  do {
    if (is_ldc_enable() && m_dptz_warp_buf_id == AM_ENCODE_SOURCE_BUFFER_MAIN) {
      m_encode_device->get_source_buffer(AM_ENCODE_SOURCE_BUFFER_MAIN)->get_source_buffer_format(&buffer_format);

      //init dewarp library
      if (dewarp_init(&m_dewarp_init_param) < 0) {
        result = AM_RESULT_ERR_INVALID;
        break;
      }

      m_lens_warp_region.output.width = buffer_format.size.width;
      m_lens_warp_region.output.height = buffer_format.size.height;
      m_lens_warp_region.output.upper_left.x = 0;
      m_lens_warp_region.output.upper_left.y = 0;

      /* Zoom factor */
      if (m_zoom_source == AM_WARP_ZOOM_LDC) {
        m_lens_warp_region.zoom.num = m_warp_param.lens_param.zoom.num;
        m_lens_warp_region.zoom.denom = m_warp_param.lens_param.zoom.denom;
      } else if (m_zoom_source == AM_WARP_ZOOM_DPTZ) {
        m_lens_warp_region.zoom.num = (int)(m_dptz_param[AM_ENCODE_SOURCE_BUFFER_MAIN].dptz_ratio.zoom_factor_x * 1000);
        m_lens_warp_region.zoom.denom = 1000;
        m_warp_param.lens_param.zoom.num = m_lens_warp_region.zoom.num;
        m_warp_param.lens_param.zoom.denom = m_lens_warp_region.zoom.denom;
      } else {
        m_lens_warp_region.zoom.num = m_lens_warp_region.zoom.denom = 1;
      }

      switch (m_warp_param.warp_mode) {
        case AM_WARP_MODE_RECTLINEAR:
          /* create vector */
          if (lens_wall_normal(&m_lens_warp_region, &m_lens_warp_vector) <= 0) {
            result = AM_RESULT_ERR_INVALID;
          }
          break;
        case AM_WARP_MODE_PANORAMA:
          /* create vector */
          if (lens_wall_panorama(&m_lens_warp_region, m_warp_param.lens_param.pano_hfov_degree, &m_lens_warp_vector) <= 0) {
            result = AM_RESULT_ERR_INVALID;
          }
          break;
        case AM_WARP_MODE_SUBREGION:
        case AM_WARP_MODE_NO_TRANSFORM:
        default:
          ERROR("Warp mode not supported For LDC!\n");
          result = AM_RESULT_ERR_INVALID;
          break;
      }
      if (result != AM_RESULT_OK) {
        break;
      }

      update_warp_control(warp_main);

      ioctl(m_iav, IAV_IOC_CFG_WARP_CTRL, &warp_control);
      ioctl(m_iav, IAV_IOC_APPLY_WARP_CTRL, &flags);
      dewarp_deinit();
    } else {
      INFO("ldc can only be set on main buffer");
    }
  } while (0);
  return result;
}

AM_RESULT AMDPTZWarp::apply_dptz_warp_all()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    result = apply_warp();
    if (result < 0) {
      ERROR("apply_warp");
      break;
    }
    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
      result = apply_dptz((AM_ENCODE_SOURCE_BUFFER_ID)i, &m_dptz_param[i]);
      if (result < 0) {
        ERROR("apply_dptz failed buffer_id = %d", i);
      }
    }
  } while (0);
  return result;
}

//dptz and warp must apply together
AM_RESULT AMDPTZWarp::apply_dptz_warp() /* apply DPTZ to DSP.   just set_dptz_xxx will only save data but do not auto apply */
{
  AM_RESULT result = AM_RESULT_OK;
  AM_ENCODE_SOURCE_BUFFER_ID i = m_dptz_warp_buf_id;
  do {
    if (!m_init_done) {
      ERROR("AMDPTZWarp: not init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    result = apply_warp();
    if (result < 0) {
      ERROR("apply_warp");
      break;
    }
    result = apply_dptz(i, &m_dptz_param[i]);
    if (result < 0) {
      ERROR("apply_dptz failed");
      break;
    }

  } while (0);
  return result;
}
