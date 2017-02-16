/*******************************************************************************
 * am_video_adv_hdr_mode.cpp
 *
 * History:
 *   2014-8-7 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/ioctl.h>
#include "am_base_include.h"
#include "am_video_dsp.h"
#include "am_video_adv_hdr_mode.h"
#include "am_vin_trans.h"
#include "am_log.h"
#include "iav_ioctl.h"

#define ADV_HDR_MODE_RSRC_LMT_CONFIG_FILE "adv_hdr_mode_resource_limit.acs"

AMVideoAdvHDRMode::AMVideoAdvHDRMode()
{
  m_encode_mode = AM_VIDEO_ENCODE_ADV_HDR_MODE;
  memset(&m_rsrc_lmt, 0, sizeof(m_rsrc_lmt));
}

AMVideoAdvHDRMode::~AMVideoAdvHDRMode()
{
}

AMVideoAdvHDRMode *AMVideoAdvHDRMode::create()
{
  AMVideoAdvHDRMode *result = new AMVideoAdvHDRMode;
  if (!result || result->construct() != AM_RESULT_OK) {
    ERROR("AMVideoAdvHDRMode::get_instance error\n");
    delete result;
    result = NULL;
  }

  return result;
}

AM_RESULT AMVideoAdvHDRMode::construct()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    result = AMEncodeDevice::construct();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoAdvHDRMode::AMEncodeDevice::construct error\n");
      break;
    }
    m_rsrc_lmt_cfg =
        new AMResourceLimitConfig(ADV_HDR_MODE_RSRC_LMT_CONFIG_FILE);
    if (!m_rsrc_lmt_cfg) {
      ERROR("AMVideoAdvHDRMode::can not find resource limit config file for advanced HDR mode\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAdvHDRMode::load_config()
{
  return AMEncodeDevice::load_config();
}

AM_RESULT AMVideoAdvHDRMode::save_config()
{
  return AMEncodeDevice::save_config();
}

AM_RESULT AMVideoAdvHDRMode::update()
{
  return AMEncodeDevice::update();
}

AM_RESULT AMVideoAdvHDRMode::set_system_resource_limit()
{
  AM_RESULT result = AM_RESULT_OK;

  AMEncodeParamAll *encode_param = NULL;
  struct iav_system_resource resource;
  uint32_t i;
  AM_ENCODE_STREAM_TYPE stream_type;
  uint32_t width, height;
  do {
    //m_resource_config
    if (m_config->m_buffer_alloc_style
        == AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL) {
      encode_param = &m_config->m_loaded;
    } else {
      encode_param = &m_auto_encode_param;
    }
    //read config info from config file
    result = m_rsrc_lmt_cfg->load_config(&m_rsrc_lmt);
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoAdvHDRMode::load config from resource limit config file failed\n");
      break;
    }
    memset(&resource, 0, sizeof(resource));
    if (ioctl(get_iav(), IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    //fill the buffer max size to it
    for (i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; i ++) {
      resource.buf_max_size[i].width = m_rsrc_lmt.buf_max_width[i];
      resource.buf_max_size[i].height = m_rsrc_lmt.buf_max_height[i];
    }

    //setup stream limit
    for (i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      width = m_rsrc_lmt.stream_max_width[i];
      height = m_rsrc_lmt.stream_max_height[i];

      if (!encode_param->stream_format.encode_format[i].rotate_90_ccw) {
        //handle rotation, if not rotated 90
        resource.stream_max_size[i].width = width;
        resource.stream_max_size[i].height = height;
      } else {
        //handle rotation, if rotated 90
        resource.stream_max_size[i].width = height;
        resource.stream_max_size[i].height = width;
      }

      stream_type = encode_param->stream_format.encode_format[i].type;
      PRINTF("stream %d type  is %d \n", i, stream_type);
      if (stream_type == AM_ENCODE_STREAM_TYPE_H264) {
        resource.stream_max_M[i] = m_rsrc_lmt.stream_max_M[i];
        resource.stream_max_N[i] = m_rsrc_lmt.stream_max_N[i];
        resource.stream_long_ref_enable[i] =
            m_rsrc_lmt.stream_long_ref_possible[i];
        resource.stream_max_advanced_quality_model[i] =
            m_rsrc_lmt.stream_max_advanced_quality_model[i];
      } else {
        resource.stream_max_M[i] = 0;
        resource.stream_max_N[i] = 0;
        resource.stream_long_ref_enable[i] = 0;
        resource.stream_max_advanced_quality_model[i] = 0;
      }
    }

    resource.encode_mode = DSP_HDR_LINE_INTERLEAVED_MODE;
    //encode mode DSP_HDR_LINE_INTERLEAVED_MODE cannot support mixer B
    resource.mixer_b_enable = 0;
    resource.rotate_enable = m_rsrc_lmt.rotate_possible;
    resource.raw_capture_enable = m_rsrc_lmt.raw_capture_possible;
    //setup HDR exposure num, for non HDR or internal WDR, it is still 1
    resource.exposure_num = get_hdr_expose_num(m_hdr_type);
    //setup LDC related args
    resource.lens_warp_enable = m_rsrc_lmt.lens_warp_possible;
    resource.max_padding_width = m_rsrc_lmt.max_padding_width;

    //setup encode from raw related args
    resource.enc_raw_rgb = m_rsrc_lmt.enc_from_raw_possible;
    //setup idsp upsampling type
    resource.idsp_upsample_type = m_rsrc_lmt.idsp_upsample_type;

    if (m_rsrc_lmt.debug_iso_type != -1) {
      resource.debug_iso_type = m_rsrc_lmt.debug_iso_type;
      resource.debug_enable_map |= DEBUG_TYPE_ISO_TYPE;
    } else {
      resource.debug_enable_map &= ~DEBUG_TYPE_ISO_TYPE;
    }
    if (m_rsrc_lmt.debug_stitched != -1) {
      resource.debug_stitched = m_rsrc_lmt.debug_stitched;
      resource.debug_enable_map |= DEBUG_TYPE_STITCH;
    } else {
      resource.debug_enable_map &= ~DEBUG_TYPE_STITCH;
    }
    if (m_rsrc_lmt.debug_chip_id != -1) {
      resource.debug_chip_id = m_rsrc_lmt.debug_chip_id;
      resource.debug_enable_map |= DEBUG_TYPE_CHIP_ID;
    } else {
      resource.debug_enable_map &= ~DEBUG_TYPE_CHIP_ID;
    }

    if (ioctl(get_iav(), IAV_IOC_SET_SYSTEM_RESOURCE, &resource) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}
