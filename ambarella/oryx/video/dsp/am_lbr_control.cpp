/**
 * am_lbr_control.cpp
 *
 *  History:
 *  Nov 18, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "lbr_api.h"
#include "am_encode_device.h"
#include "am_video_types.h"
#include "am_log.h"
#include "am_lbr_control.h"
#include "am_event_types.h"

AMLBRControl::AMLBRControl() :
    m_main_loop_exit(false),
    m_noise_low_threshold(0),
    m_noise_high_threshold(0),
    m_event_type(0),
    m_motion_value(0),
    m_noise_value(0),
    m_fd_iav(-1),
    m_init_done(false),
    m_log_level(0),
    m_lbr_ctrl_thread(NULL),
    m_encode_device(NULL)
{
  for (int32_t i = 0; i < LBR_CTRL_PROFILE_NUM; i ++) {
    m_profile_bt_sf[i].denominator = 1;
    m_profile_bt_sf[i].numerator = 1;
  }
  for (int32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
    m_lbr_stream_format[i] = NULL;
    m_lbr_motion_level[i] = LBR_MOTION_HIGH;
    m_lbr_noise_level[i] = LBR_NOISE_NONE;
    m_lbr_style[i] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
    m_stream_params[i].auto_target = true;
    m_stream_params[i].bitrate_ceiling = 0;
    m_stream_params[i].enable_lbr = false;
    m_stream_params[i].frame_drop = false;
    m_stream_params[i].motion_control = false;
    m_stream_params[i].noise_control = false;
  }
}

AMLBRControl::~AMLBRControl()
{
  lbr_close();
}

AM_RESULT AMLBRControl::load_config(AMEncodeLBRConfig *lbr_config)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!lbr_config) {
      ERROR("AMLBRControl::load_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_log_level = lbr_config->log_level;
    m_noise_low_threshold = lbr_config->noise_low_threshold;
    m_noise_high_threshold = lbr_config->noise_high_threshold;
    memcpy(m_profile_bt_sf,
           &lbr_config->profile_bt_sf,
           sizeof(m_profile_bt_sf));
    memcpy(m_stream_params,
           &lbr_config->stream_params,
           sizeof(m_stream_params));

    //print_lbr_config();
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::set_config(AMEncodeLBRConfig *lbr_config)
{
  INFO("AMLBRControl::set_config\n");
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!lbr_config) {
      ERROR("AMLBRControl::set_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (m_log_level != lbr_config->log_level) {
      set_lbr_log_level(lbr_config->log_level);
      m_log_level = lbr_config->log_level;
    }
    m_noise_low_threshold = lbr_config->noise_low_threshold;
    m_noise_high_threshold = lbr_config->noise_high_threshold;

    memcpy(m_profile_bt_sf,
           &lbr_config->profile_bt_sf,
           sizeof(m_profile_bt_sf));
    for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
      if (lbr_config->stream_params[i].enable_lbr
          != m_stream_params[i].enable_lbr) {
        set_enable_lbr(i, (uint32_t) lbr_config->stream_params[i].enable_lbr);
        m_stream_params[i].enable_lbr = lbr_config->stream_params[i].enable_lbr;
      }

      if (lbr_config->stream_params[i].auto_target
          != m_stream_params[i].auto_target) {
        lbr_bitrate_target_t bt;
        get_lbr_bitrate_target(i, &bt);
        bt.auto_target = lbr_config->stream_params[i].auto_target;
        set_lbr_bitrate_target(i, bt);
        m_stream_params[i].auto_target =
            lbr_config->stream_params[i].auto_target;
      }

      if (lbr_config->stream_params[i].bitrate_ceiling
          != m_stream_params[i].bitrate_ceiling) {
        lbr_bitrate_target_t bt;
        get_lbr_bitrate_target(i, &bt);
        bt.bitrate_ceiling = lbr_config->stream_params[i].bitrate_ceiling;
        set_lbr_bitrate_target(i, bt);
        m_stream_params[i].bitrate_ceiling =
            lbr_config->stream_params[i].bitrate_ceiling;
      }

      if (lbr_config->stream_params[i].frame_drop
          != m_stream_params[i].frame_drop) {
        set_lbr_drop_frame(i,
                           (uint32_t) lbr_config->stream_params[i].frame_drop);
        m_stream_params[i].frame_drop = lbr_config->stream_params[i].frame_drop;
      }
    }
    //print_lbr_config();
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::get_config(AMEncodeLBRConfig *lbr_config)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!lbr_config) {
      ERROR("AMLBRControl::get_config error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(lbr_config, 0, sizeof(*lbr_config));
    lbr_config->log_level = m_log_level;
    lbr_config->noise_low_threshold = m_noise_low_threshold;
    lbr_config->noise_high_threshold = m_noise_high_threshold;
    memcpy(&lbr_config->profile_bt_sf,
           &m_profile_bt_sf,
           sizeof(lbr_config->profile_bt_sf));
    memcpy(&lbr_config->stream_params,
           &m_stream_params,
           sizeof(lbr_config->stream_params));
  } while (0);
  //print_lbr_config();

  return result;
}

void AMLBRControl::print_lbr_config(void)
{
  PRINTF("LogLevel            = %d\n", m_log_level);
  PRINTF("NoiseLowThreshold   = %d\n", m_noise_low_threshold);
  PRINTF("NoiseHighThreshold  = %d\n", m_noise_high_threshold);
  for (uint32_t i = 0; i < LBR_CTRL_PROFILE_NUM; i ++) {
    PRINTF("profile_bt_sf[%d]   = {%d, %d}\n",
           i,
           m_profile_bt_sf[i].numerator,
           m_profile_bt_sf[i].denominator);
  }
  for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
    PRINTF("EnableLBR%d         = %d\n", i, m_stream_params[i].enable_lbr);
    PRINTF("FrameDrop%d         = %d\n", i, m_stream_params[i].frame_drop);
    PRINTF("MotionControl%d     = %d\n", i, m_stream_params[i].motion_control);
    PRINTF("NoiseControl%d      = %d\n", i, m_stream_params[i].noise_control);
    PRINTF("AutoBitrateTarget%d = %d\n", i, m_stream_params[i].auto_target);
    PRINTF("BitrateCeiling%d    = %d\n", i, m_stream_params[i].bitrate_ceiling);
  }
}

AM_RESULT AMLBRControl::init(int32_t fd_iav, AMEncodeDevice *encode_device)
{
  AM_RESULT result = AM_RESULT_OK;
  lbr_init_t lbr;
  do {
    if (!encode_device) {
      ERROR("AMLBRControl:: init error \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (fd_iav < 0) {
      ERROR("AMLBRControl:: wrong iav handle to init \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_fd_iav = fd_iav;
    for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
      m_lbr_stream_format[i] =
          &(encode_device->get_encode_stream_list() + i)->m_stream_format;
    }
    m_encode_device = encode_device;

    if (lbr_open() < 0) {
      ERROR("AMLBRControl::lbr open failed\n");
      result = AM_RESULT_ERR_NO_ACCESS;
      break;
    }

    memset(&lbr, 0, sizeof(lbr));
    lbr.fd_iav = m_fd_iav;
    if (lbr_init(&lbr) < 0) {
      ERROR("AMLBRControl::lbr init failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  m_init_done = (result == AM_RESULT_OK);

  return result;
}

void AMLBRControl::receive_motion_info_from_ipc_lbr(void *msg_data)
{
  AM_EVENT_MESSAGE msg;
  memcpy(&msg, msg_data, sizeof(AM_EVENT_MESSAGE));
  m_event_type = msg.event_type;
  m_motion_value = msg.md_msg.mt_value;
  static LBR_MOTION_LEVEL motion_level = LBR_MOTION_LOW;
  static LBR_MOTION_LEVEL real_motion_level[MAX_LBR_STREAM_NUM];

  /*PRINTF("AMLBRControl::pts = %llu, ROI#%d, motion value = %d, motion_event_type = %d, motion level = %d.\n",
   msg.pts,
   msg.md_msg.roi_id,
   m_motion_value,
   msg.md_msg.mt_type,
   msg.md_msg.mt_level);*/
  //Use ROI#0 motion event to do motion control
  if (msg.md_msg.roi_id == 0 && m_lbr_ctrl_thread) {
    switch (msg.md_msg.mt_level) {
      case 0:
        motion_level = LBR_MOTION_NONE;
        break;
      case 1:
        motion_level = LBR_MOTION_LOW;
        break;
      case 2:
        motion_level = LBR_MOTION_HIGH;
        break;
      default:
        motion_level = LBR_MOTION_HIGH;
        break;
    }

    for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
      lbr_get_motion_level(&real_motion_level[i], i);
      if ((m_stream_params[i].enable_lbr)
          && (m_lbr_stream_format[i]->type == AM_ENCODE_STREAM_TYPE_H264)) {
        if (motion_level != real_motion_level[i]) {
          if (m_stream_params[i].motion_control) {
            lbr_set_motion_level(motion_level, i);
          }
        }
      }
    }
  }
}

void AMLBRControl::lbr_main(void *data)
{
  AMLBRControl *lbr_ctrl = (AMLBRControl *) data;
  AMVin* vin = NULL;
  struct AMVinAGC agc;
  uint32_t i0_light = 0, i1_light = 0, i2_light = 0;
  LBR_NOISE_LEVEL noise_level = LBR_NOISE_LOW;
  LBR_NOISE_LEVEL real_noise_level[MAX_LBR_STREAM_NUM];
  bool lbr_style_changed[MAX_LBR_STREAM_NUM];
  bool lbr_bc_changed[MAX_LBR_STREAM_NUM];
  lbr_bitrate_target_t bitrate_target;

  for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
    lbr_style_changed[i] = true;
    lbr_bc_changed[i] = true;
  }

  if (lbr_set_log_level(lbr_ctrl->m_log_level) < 0) {
    ERROR("AMLBRControl::lbr_set_log_level failed \n");
  }

  for (uint32_t i = 0; i < LBR_CTRL_PROFILE_NUM; i ++) {
    if ((lbr_ctrl->m_profile_bt_sf[i].numerator == 0)
        || (lbr_ctrl->m_profile_bt_sf[i].denominator == 0)) {
      ERROR("AMLBRControl::denominator or numerator can not be 0\n");
    } else {
      lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE(i),
                                       lbr_ctrl->m_profile_bt_sf[i].numerator,
                                       lbr_ctrl->m_profile_bt_sf[i].denominator);
    }
  }

  while (!lbr_ctrl->m_main_loop_exit) {
    vin = lbr_ctrl->m_encode_device->get_primary_vin();
    if (!vin) {
      WARN("AMLBRControl::vin is not ready\n");
      usleep(33000);
      continue;
    }
    if (vin->wait_frame_sync() != AM_RESULT_OK) {
      ERROR("AMLBRControl::wait frame sync error\n");
    }
    /*get noise statistics*/
    if (vin->get_AGC(&agc) != AM_RESULT_OK) {
      INFO("AMLBRControl::get AGC no success\n");
      continue;
    } else {
      lbr_ctrl->m_noise_value = agc.agc >> 24;
    }
    //PRINTF("AMLBRControl::sensor gain = %ddB\n\n", lbr_ctrl->m_noise_value);
    if (lbr_ctrl->m_noise_value > lbr_ctrl->m_noise_high_threshold) {
      i0_light ++;
      i1_light = 0;
      i2_light = 0;
      if (i0_light >= LBR_NOISE_HIGH_DELAY) {
        noise_level = LBR_NOISE_HIGH;
        i0_light = 0;
      }
    } else if (lbr_ctrl->m_noise_value > lbr_ctrl->m_noise_low_threshold) {
      i0_light = 0;
      i1_light ++;
      i2_light = 0;
      if (i1_light >= LBR_NOISE_LOW_DELAY) {
        noise_level = LBR_NOISE_LOW;
        i1_light = 0;
      }
    } else {
      i0_light = 0;
      i1_light = 0;
      i2_light ++;
      if (i2_light >= LBR_NOISE_NONE_DELAY) {
        noise_level = LBR_NOISE_NONE;
        i2_light = 0;
      }
    }
    for (uint32_t i = 0; i < MAX_LBR_STREAM_NUM; i ++) {
      //PRINTF("stream%d: type=%d\n", i, lbr_ctrl->m_lbr_stream_format[i]->type);
      if (lbr_ctrl->m_lbr_stream_format[i]->type
          == AM_ENCODE_STREAM_TYPE_H264) {
        if (!lbr_ctrl->m_stream_params[i].enable_lbr) {
          lbr_ctrl->m_lbr_style[i] = LBR_STYLE_FPS_KEEP_CBR_ALIKE;
          if (lbr_style_changed[i]) {
            if (lbr_set_style(lbr_ctrl->m_lbr_style[i], i) < 0) {
              ERROR("AMLBRControl::lbr set style %d for stream %d failed\n",
                    lbr_ctrl->m_lbr_style[i],
                    i);
            } else {
              lbr_style_changed[i] = false;
            }
          }
        } else {
          if (lbr_ctrl->m_stream_params[i].frame_drop) {
            lbr_ctrl->m_lbr_style[i] = LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP;
          } else {
            lbr_ctrl->m_lbr_style[i] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
          }
          if (lbr_style_changed[i]) {
            if (lbr_set_style(lbr_ctrl->m_lbr_style[i], i) < 0) {
              ERROR("AMLBRControl::lbr set style %d for stream %d failed\n",
                    lbr_ctrl->m_lbr_style[i],
                    i);
            } else {
              lbr_style_changed[i] = false;
            }
          }
        }
        if (lbr_bc_changed[i]) {
          //transfer bitrate from bps/MB to normal format
          bitrate_target.auto_target =
              (uint32_t) lbr_ctrl->m_stream_params[i].auto_target;
          bitrate_target.bitrate_ceiling =
              ((lbr_ctrl->m_lbr_stream_format[i]->width
                  * lbr_ctrl->m_lbr_stream_format[i]->height) >> 8)
                  * lbr_ctrl->m_stream_params[i].bitrate_ceiling;
          /*PRINTF("AMLBRControl::stream%d: resolution is %dx%d, bitrate ceiling is %d\n",
           i, lbr_ctrl->m_lbr_stream_format[i]->width,
           lbr_ctrl->m_lbr_stream_format[i]->height,
           lbr_ctrl->m_stream_params[i].bitrate_ceiling);*/
          if (lbr_set_bitrate_ceiling(&bitrate_target, i) < 0) {
            ERROR("AMLBRControl::lbr set bitrate ceiling %d for stream %d failed\n",
                  lbr_ctrl->m_stream_params[i].bitrate_ceiling,
                  i);
          } else {
            lbr_bc_changed[i] = false;
          }
        }
      }
      lbr_get_noise_level(&real_noise_level[i], i);
      if ((lbr_ctrl->m_stream_params[i].enable_lbr)
          && (lbr_ctrl->m_lbr_stream_format[i]->type
              == AM_ENCODE_STREAM_TYPE_H264)) {
        if (noise_level != real_noise_level[i]) {
          if (lbr_ctrl->m_stream_params[i].noise_control) {
            lbr_set_noise_level(noise_level, i);
          }
        }
      }
    }
  }

}

AM_RESULT AMLBRControl::lbr_ctrl_start()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_done) {
      ERROR("AMLBRControl: not init\n");
      result = AM_RESULT_ERR_NO_ACCESS;
      break;
    }
    m_main_loop_exit = false;
    m_lbr_ctrl_thread = AMThread::create("LBR.Ctrl", lbr_main, this);
    if (!m_lbr_ctrl_thread) {
      ERROR("AMLBRControl::create m_lbr_ctrl_thread failed\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::lbr_ctrl_stop()
{
  m_main_loop_exit = true;
  if (!m_lbr_ctrl_thread) {
    m_lbr_ctrl_thread->destroy();
    m_lbr_ctrl_thread = NULL;
    INFO("AMLBRControl::Destroy <m_lbr_ctrl_thread> successfully\n");
  }

  return AM_RESULT_OK;
}

AM_RESULT AMLBRControl::get_enable_lbr(uint32_t stream_id, uint32_t *enable_lbr)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!enable_lbr) {
      ERROR("AMLBRControl::stream%d get_lbr_enable_lbr error\n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    *enable_lbr = m_stream_params[stream_id].enable_lbr;
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::set_enable_lbr(uint32_t stream_id, uint32_t enable_lbr)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_lbr_stream_format[stream_id]->type != AM_ENCODE_STREAM_TYPE_H264) {
      ERROR("AMLBRControl::stream %d is not H.264 \n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_stream_params[stream_id].enable_lbr = enable_lbr;
    if (!m_stream_params[stream_id].enable_lbr) {
      m_lbr_style[stream_id] = LBR_STYLE_FPS_KEEP_CBR_ALIKE;
      if (lbr_set_style(m_lbr_style[stream_id], stream_id) < 0) {
        ERROR("AMLBRControl::lbr set style %d for stream %d failed\n",
              m_lbr_style[stream_id],
              stream_id);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    } else {
      if (m_stream_params[stream_id].frame_drop) {
        m_lbr_style[stream_id] = LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP;
      } else {
        m_lbr_style[stream_id] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
      }
      if (lbr_set_style(m_lbr_style[stream_id], stream_id) < 0) {
        ERROR("AMLBRControl::lbr set style %d for stream %d failed\n",
              m_lbr_style[stream_id],
              stream_id);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::get_lbr_bitrate_target(uint32_t stream_id,
                                               lbr_bitrate_target_t *bitrate_target)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!bitrate_target) {
      ERROR("AMLBRControl::stream%d get_lbr_bitrate_target error\n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (m_lbr_stream_format[stream_id]->type != AM_ENCODE_STREAM_TYPE_H264) {
      ERROR("AMLBRControl::stream %d is not H.264 \n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (lbr_get_bitrate_ceiling(bitrate_target, stream_id) < 0) {
      ERROR("AMLBRControl::Get lbr bitrate target failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    /*transfer bitrate from  normal format to bps/MB */
    bitrate_target->bitrate_ceiling = bitrate_target->bitrate_ceiling
        / ((m_lbr_stream_format[stream_id]->width
            * m_lbr_stream_format[stream_id]->height) >> 8);
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::set_lbr_bitrate_target(uint32_t stream_id,
                                               lbr_bitrate_target_t bitrate_target)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_lbr_stream_format[stream_id]->type != AM_ENCODE_STREAM_TYPE_H264) {
      ERROR("AMLBRControl::stream %d is not H.264 \n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    /*transfer bitrate from bps/MB to normal format*/
    bitrate_target.bitrate_ceiling = ((m_lbr_stream_format[stream_id]->width
        * m_lbr_stream_format[stream_id]->height) >> 8)
        * bitrate_target.bitrate_ceiling;
    /*PRINTF(
     "AMLBRControl::stream%d: resolution is %dx%d, bitrate ceiling is %d bps\n\n",
     stream_id, m_lbr_stream_format[stream_id]->width,
     m_lbr_stream_format[stream_id]->height,
     bitrate_target.bitrate_ceiling);*/
    if (lbr_set_bitrate_ceiling(&bitrate_target, stream_id) < 0) {
      ERROR("AMLBRControl::lbr set bitrate target for stream %d failed\n",
            stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::get_lbr_drop_frame(uint32_t stream_id, uint32_t *enable)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!enable) {
      ERROR("AMLBRControl::stream %d get_lbr_drop_frame error\n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    *enable = m_stream_params[stream_id].frame_drop;
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::set_lbr_drop_frame(uint32_t stream_id, uint32_t enable)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_lbr_stream_format[stream_id]->type != AM_ENCODE_STREAM_TYPE_H264) {
      ERROR("AMLBRControl::stream %d is not H.264 \n", stream_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_stream_params[stream_id].frame_drop = enable;
    if (m_stream_params[stream_id].enable_lbr) {
      if (m_stream_params[stream_id].frame_drop) {
        m_lbr_style[stream_id] = LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP;
      } else {
        m_lbr_style[stream_id] = LBR_STYLE_FPS_KEEP_BITRATE_AUTO;
      }
      if (lbr_set_style(m_lbr_style[stream_id], stream_id) < 0) {
        ERROR("AMLBRControl::lbr set style %d for stream %d failed\n",
              m_lbr_style[stream_id],
              stream_id);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::get_lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile,
                                                             AMScaleFactor *profile_bt_sf)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!profile_bt_sf) {
      ERROR("AMLBRControl::get_lbr_scale_profile_target_bitrate error\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    *profile_bt_sf = m_profile_bt_sf[profile];
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::set_lbr_scale_profile_target_bitrate(LBR_PROFILE_TYPE profile,
                                                             AMScaleFactor profile_bt_sf)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (lbr_scale_profile_target_bitrate(profile,
                                         profile_bt_sf.numerator,
                                         profile_bt_sf.denominator) < 0) {
      ERROR("AMLBRControl::set_lbr_scale_profile_target_bitrate failed\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMLBRControl::get_lbr_log_level(int32_t *log_level)
{
  AM_RESULT result = AM_RESULT_OK;
  if (lbr_get_log_level(log_level) < 0) {
    ERROR("AMLBRControl::get_lbr_log_level failed \n");
    result = AM_RESULT_ERR_INVALID;
  }

  return result;
}

AM_RESULT AMLBRControl::set_lbr_log_level(int32_t log_level)
{
  AM_RESULT result = AM_RESULT_OK;
  if (lbr_set_log_level(log_level) < 0) {
    ERROR("AMLBRControl::set_lbr_log_level failed \n");
    result = AM_RESULT_ERR_INVALID;
  }

  return result;
}
