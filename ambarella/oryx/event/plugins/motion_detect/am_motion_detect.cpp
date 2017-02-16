/**
 * am_motion_detect.cpp
 *
 *  History:
 *		Dec 11, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_video_types.h"
#include "am_video_reader_if.h"
#include "am_event_types.h"
#include "mdet_lib.h"
#include "am_motion_detect.h"

enum AM_MOTIONT_STATUS
{
  MD_NO_MOTION = 0,
  MD_IN_MOTION,
};

#define MOTION_DETECT_CONFIG ((const char*)"event-motion-detect.acs")
#define SKIP_CNT 1
#define MAX_MOTION_INDICATOR 1000000
#define ROI_POINTS 4 //4 points decide a roi, left, right, top. bottom
#undef AUTO_LOCK
#define AUTO_LOCK(mtx) std::lock_guard<std::recursive_mutex> lck (mtx)
static std::recursive_mutex motion_detect_mtx;

DECLARE_EVENT_PLUGIN_INIT_FINIT(AMMotionDetect, EV_MOTION_DECT)

AMMotionDetect::AMMotionDetect(EVENT_MODULE_ID mid) :
    m_enable(false),
    m_source_buffer_id(AM_ENCODE_SOURCE_BUFFER_MAIN),
    m_roi_info(NULL),
    m_threshold(NULL),
    m_level_change_delay(NULL),
    m_main_loop_exit(true),
    m_plugin_id(mid),
    m_video_reader(NULL),
    m_md_thread(NULL),
    m_inst(NULL),
    m_callback(NULL),
    m_config(NULL),
    m_conf_path(ORYX_EVENT_CONF_DIR),
    m_started(false)
{
  memset(&m_dsp_mem, 0, sizeof(AMMemMapInfo));
  memset(&m_frame_desc, 0, sizeof(AMQueryDataFrameDesc));
  memset(&mdet_session, 0, sizeof(mdet_session_t));
  memset(&mdet_config, 0, sizeof(mdet_cfg));
}

AMMotionDetect::~AMMotionDetect()
{
  if (m_roi_info) {
    delete m_roi_info;
  }
  if (m_threshold) {
    delete m_threshold;
  }
  if (m_level_change_delay) {
    delete m_level_change_delay;
  }
  if (m_config) {
    delete m_config;
  }

  if (m_inst) {
    mdet_destroy_instance(m_inst);
    m_inst = NULL;
  }
}

bool AMMotionDetect::construct()
{
  bool result = true;
  do {
    m_roi_info = new AM_EVENT_MD_ROI[MAX_ROI_NUM];
    m_threshold = new AM_EVENT_MD_THRESHOLD[MAX_ROI_NUM];
    m_level_change_delay = new AM_EVENT_MD_LEVEL_CHANGE_DELAY[MAX_ROI_NUM];

    m_config = new AMMotionDetectConfig();
    if (!m_config) {
      ERROR("AMMotionDetect:: new m_config error\n");
      result = false;
      break;
    }

    m_conf_path.append(ORYX_EVENT_CONF_SUB_DIR).append(MOTION_DETECT_CONFIG);
    MotionDetectParam *motion_detect_param = m_config->get_config(m_conf_path);
    if (!motion_detect_param) {
      ERROR("AMMotionDetect::get config failed!\n");
      result = false;
      break;
    }

    m_enable = motion_detect_param->enable;
    m_source_buffer_id =
        (AM_ENCODE_SOURCE_BUFFER_ID) motion_detect_param->buf_id;
    memcpy(m_roi_info,
           motion_detect_param->roi_info,
           sizeof(motion_detect_param->roi_info));
    memcpy(m_threshold,
           motion_detect_param->th,
           sizeof(motion_detect_param->th));
    memcpy(m_level_change_delay,
           motion_detect_param->lc_delay,
           sizeof(motion_detect_param->lc_delay));
    m_inst = mdet_create_instance(MDET_ALGO_DIFF);
    if (!m_inst) {
      ERROR("AMMotionDetect::mdet_create_instance error\n");
      break;
    }
  } while (0);

  return result;
}

AMIEventPlugin *AMMotionDetect::create(EVENT_MODULE_ID mid)
{
  INFO("AMMotionDetect::create \n");
  AMMotionDetect *result = new AMMotionDetect(mid);
  if (result && result->construct() < 0) {
    ERROR("AMMotionDetect::Failed to create an instance of AMMotionDetect\n");
    delete result;
    result = NULL;
  } else {
    INFO("AMMotionDetect::create successfully!\n");
  }

  return result;
}

bool AMMotionDetect::start_plugin()
{
  AUTO_LOCK(motion_detect_mtx);
  INFO("AMMotionDetect::start motion detect plugin\n");
  bool result = true;
  do {
    if (m_started) {
      NOTICE("motion detect has already started!\n");
      break;
    }
    m_video_reader = AMIVideoReader::get_instance();
    if (!m_video_reader) {
      ERROR("AMMotionDetect::Unable to get AMVideoReader instance\n");
      result = false;
      break;
    }
    if (m_video_reader->init() != AM_RESULT_OK) {
      ERROR("AMMotionDetect::m_video_reader->init() failed\n");
      result = false;
      break;
    }
    if (m_video_reader->get_dsp_mem(&m_dsp_mem) != AM_RESULT_OK) {
      ERROR("AMMotionDetect::m_video_reader->get_dsp_mem failed\n");
      result = false;
      break;
    }
    //get one frame data from ME1 buffer and do mdet_start.
    if (m_video_reader->query_luma_frame(&m_frame_desc,
                                         (AM_ENCODE_SOURCE_BUFFER_ID) m_source_buffer_id,
                                         false) != AM_RESULT_OK) { //use main buffer here
      ERROR("AMMotionDetect::from buffer %d failed\n", m_source_buffer_id);
      result = false;
      break;
    }
    INFO("ME1 buf[%d]: m_frame_desc.luma.height=%d, m_frame_desc.luma.width=%d, m_frame_desc.luma.pitch=%d\n",
         m_source_buffer_id,
         m_frame_desc.luma.height,
         m_frame_desc.luma.width,
         m_frame_desc.luma.pitch);
    mdet_config.roi_info.num_roi = 0;
    mdet_config.fm_dim.height = m_frame_desc.luma.height;
    mdet_config.fm_dim.width = m_frame_desc.luma.width;
    mdet_config.fm_dim.pitch = m_frame_desc.luma.pitch;
    for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
      m_roi_info[i].roi_id = i;
      set_roi_info(&m_roi_info[i]);
      if (m_roi_info[i].left != 0 || m_roi_info[i].right != 0
          || m_roi_info[i].top != 0 || m_roi_info[i].bottom != 0) {
        mdet_config.roi_info.num_roi ++;
      }
    }
    mdet_cfg mdet_config_get;
    (*m_inst->md_get_config)(&mdet_config_get);
    mdet_config.threshold = mdet_config_get.threshold;
    /*print_md_config();
     PRINTF("mdet_config.roi_info.num_roi=%d\n",mdet_config.roi_info.num_roi);*/
    (*m_inst->md_set_config)(&mdet_config);
    if ((*m_inst->md_start)(&mdet_session) < 0) {
      ERROR("AMMotionDetect::mdet_start failed\n");
      result = false;
      break;
    }

    m_main_loop_exit = false;
    m_md_thread = AMThread::create("Event.Motion", md_main, this);
    if (!m_md_thread) {
      ERROR("AMMotionDetect::create m_md_thread failed\n");
      result = false;
      break;
    }

    m_started = true;
  } while (0);

  return result;
}

bool AMMotionDetect::stop_plugin()
{
  AUTO_LOCK(motion_detect_mtx);
  bool result = true;

  if (m_started) {
    m_main_loop_exit = true;
    m_md_thread->destroy();
    m_md_thread = NULL;
    INFO("AMMotionDetect::Destroy <m_md_thread> successfully\n");
    if ((*m_inst->md_stop)(&mdet_session) < 0) {
      ERROR("AMMotionDetect::mdet_stop error\n");
      result = false;
    }
    m_started = false;
  } else {
    NOTICE("motion detect has already stopped!\n");
  }

  return result;
}

bool AMMotionDetect::set_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  AUTO_LOCK(motion_detect_mtx);
  bool result = true;
  if (!pConfig || !pConfig->value) {
    ERROR("AMMotionDetect::Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_MD_ENABLE:
      result = set_md_state(*(bool*) pConfig->value);
      break;
    case AM_MD_BUFFER_ID:
      result = set_md_buffer_id(*(AM_ENCODE_SOURCE_BUFFER_ID*) pConfig->value);
      break;
    case AM_MD_ROI:
      result = set_roi_info((AM_EVENT_MD_ROI *) pConfig->value);
      break;
    case AM_MD_THRESHOLD:
      result = set_threshold_info((AM_EVENT_MD_THRESHOLD *) pConfig->value);
      break;
    case AM_MD_LEVEL_CHANGE_DELAY:
      result =
          set_level_change_delay_info((AM_EVENT_MD_LEVEL_CHANGE_DELAY *) pConfig->value);
      break;
    case AM_MD_CALLBACK:
      result = set_md_callback((AM_EVENT_CALLBACK) pConfig->value);
      break;
    case AM_MD_SYNC_CONFIG:
      result = sync_config();
      break;
    default:
      ERROR("Unknown key\n");
      result = false;
      break;
  }

  return result;
}

bool AMMotionDetect::get_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  AUTO_LOCK(motion_detect_mtx);
  bool result = true;
  if (!pConfig || !pConfig->value) {
    ERROR("AMMotionDetect::Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_MD_ENABLE:
      result = get_md_state((bool *) pConfig->value);
      break;
    case AM_MD_BUFFER_ID:
      result = get_md_buffer_id((AM_ENCODE_SOURCE_BUFFER_ID *) pConfig->value);
      break;
    case AM_MD_ROI:
      result = get_roi_info((AM_EVENT_MD_ROI *) pConfig->value);
      break;
    case AM_MD_THRESHOLD:
      result = get_threshold_info((AM_EVENT_MD_THRESHOLD *) pConfig->value);
      break;
    case AM_MD_LEVEL_CHANGE_DELAY:
      result =
          get_level_change_delay_info((AM_EVENT_MD_LEVEL_CHANGE_DELAY *) pConfig->value);
      break;
    default:
      ERROR("Unknown key\n");
      result = false;
      break;
  }

  return result;
}

EVENT_MODULE_ID AMMotionDetect::get_plugin_ID()
{
  AUTO_LOCK(motion_detect_mtx);
  return m_plugin_id;
}

bool AMMotionDetect::set_md_state(bool enable)
{
  m_enable = enable;
  return true;
}

bool AMMotionDetect::get_md_buffer_id(AM_ENCODE_SOURCE_BUFFER_ID *source_buf_id)
{
  bool result = true;
  do {
    if (!source_buf_id) {
      ERROR("AMMotionDetect::get_md_buffer_id NULL pointer\n");
      result = false;
      break;
    }
    *source_buf_id = (AM_ENCODE_SOURCE_BUFFER_ID) m_source_buffer_id;
  } while (0);

  return result;
}

bool AMMotionDetect::set_md_buffer_id(AM_ENCODE_SOURCE_BUFFER_ID source_buf_id)
{
  m_source_buffer_id = source_buf_id;
  return true;
}

bool AMMotionDetect::get_md_state(bool *enable)
{
  bool result = true;
  do {
    if (!enable) {
      ERROR("AMMotionDetect::get_md_state NULL pointer\n");
      result = false;
      break;
    }
    *enable = m_enable;
  } while (0);

  return result;
}

bool AMMotionDetect::check_md_roi_format(AM_EVENT_MD_ROI *roi_info)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::check_md_roi_format, NULL pointer\n");
      result = false;
      break;
    }
    if ((roi_info->right > m_frame_desc.luma.width - 1)
        || (roi_info->bottom > m_frame_desc.luma.height - 1)) {
      ERROR("AMMotionDetect::roi%d can not bigger than ME1 buffer's window \n",
            roi_info->roi_id);
      result = false;
      break;
    }
    if ((roi_info->left > roi_info->right)
        || (roi_info->top > roi_info->bottom)) {
      ERROR("AMMotionDetect::roi%d must follow rule: left <= right, top <= bottom \n",
            roi_info->roi_id);
      result = false;
      break;
    }

  } while (0);
  return result;

}

bool AMMotionDetect::set_roi_info(AM_EVENT_MD_ROI *roi_info)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::set_roi_info NULL pointer\n");
      result = false;
      break;
    }
    if (!check_md_roi_format(roi_info)) {
      ERROR("AMMotionDetect::set_roi_info roi[%d] format wrong\n",
            roi_info->roi_id);
      result = false;
      break;
    }
    m_roi_info[roi_info->roi_id].roi_id = roi_info->roi_id;
    m_roi_info[roi_info->roi_id].left = roi_info->left;
    m_roi_info[roi_info->roi_id].right = roi_info->right;
    m_roi_info[roi_info->roi_id].top = roi_info->top;
    m_roi_info[roi_info->roi_id].bottom = roi_info->bottom;
    m_roi_info[roi_info->roi_id].valid = roi_info->valid;
    mdet_config.roi_info.roi[roi_info->roi_id].type = MDET_REGION_POLYGON;
    mdet_config.roi_info.roi[roi_info->roi_id].num_points = ROI_POINTS;
    mdet_config.roi_info.roi[roi_info->roi_id].points[0].x = roi_info->left;
    mdet_config.roi_info.roi[roi_info->roi_id].points[0].y = roi_info->top;
    mdet_config.roi_info.roi[roi_info->roi_id].points[1].x = roi_info->right;
    mdet_config.roi_info.roi[roi_info->roi_id].points[1].y = roi_info->top;

    mdet_config.roi_info.roi[roi_info->roi_id].points[2].x = roi_info->left;
    mdet_config.roi_info.roi[roi_info->roi_id].points[2].y = roi_info->bottom;
    mdet_config.roi_info.roi[roi_info->roi_id].points[3].x = roi_info->right;
    mdet_config.roi_info.roi[roi_info->roi_id].points[3].y = roi_info->bottom;
  } while (0);

  return result;
}

bool AMMotionDetect::get_roi_info(AM_EVENT_MD_ROI *roi_info)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::get_roi_info NULL pointer\n");
      result = false;
      break;
    }
    roi_info->left = m_roi_info[roi_info->roi_id].left;
    roi_info->right = m_roi_info[roi_info->roi_id].right;
    roi_info->top = m_roi_info[roi_info->roi_id].top;
    roi_info->bottom = m_roi_info[roi_info->roi_id].bottom;
    roi_info->valid = m_roi_info[roi_info->roi_id].valid;
  } while (0);

  return result;
}

bool AMMotionDetect::set_threshold_info(AM_EVENT_MD_THRESHOLD *threshold)
{
  bool result = true;
  do {
    if (!threshold) {
      ERROR("AMMotionDetect::set_theshold_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      m_threshold[threshold->roi_id].threshold[i] = threshold->threshold[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::get_threshold_info(AM_EVENT_MD_THRESHOLD *threshold)
{
  bool result = true;
  do {
    if (!threshold) {
      ERROR("AMMotionDetect::get_theshold_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      threshold->threshold[i] = m_threshold[threshold->roi_id].threshold[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::set_level_change_delay_info(AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay)
{
  bool result = true;
  do {
    if (!level_change_delay) {
      ERROR("AMMotionDetect::set_level_change_delay_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      m_level_change_delay[level_change_delay->roi_id].mt_level_change_delay[i] =
          level_change_delay->mt_level_change_delay[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::get_level_change_delay_info(AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay)
{
  bool result = true;
  do {
    if (!level_change_delay) {
      ERROR("AMMotionDetect::get_level_change_delay_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      level_change_delay->mt_level_change_delay[i] =
          m_level_change_delay[level_change_delay->roi_id].mt_level_change_delay[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::set_md_callback(AM_EVENT_CALLBACK callback)
{
  bool result = true;
  do {
    if (!callback) {
      ERROR("AMMotionDetect::set_md_callback error\n");
      result = false;
      break;
    }
    m_callback = callback;
  } while (0);

  return result;
}

#ifdef PRINT_MD_DATA
static inline void print_md_data(mdet_session_t md_data)
{
  PRINTF("\n\nmd_data: fm_dim.pitch=%d, fm_dim.width=%d, fm_dim.height=%d \n",
      md_data.fm_dim.pitch,
      md_data.fm_dim.width,
      md_data.fm_dim.height);

  PRINTF("roi_info.num_roi=%d\n", md_data.roi_info.num_roi);
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("\n========roi_info.roi[%d]========", i);
    PRINTF("type=%d, num_points=%d, ",
        md_data.roi_info.roi[i].type,
        md_data.roi_info.roi[i].num_points);
    for (uint32_t j = 0; j < md_data.roi_info.roi[i].num_points; j ++) {
      PRINTF("points[%d].x=%d, points[%d].y=%d, ",
          j,
          md_data.roi_info.roi[i].points[j].x,
          j,
          md_data.roi_info.roi[i].points[j].y);
    }
    PRINTF("motion=%f, ", md_data.motion[i]);
    PRINTF("fg_pxls=%d, pixels=%d\n", md_data.fg_pxls[i], md_data.pixels[i]);
  }
}
#endif

bool AMMotionDetect::check_motion(struct AMQueryDataFrameDesc *me1_buf)
{
  bool result = true;
  static int skip_cnt = SKIP_CNT;
  static AM_EVENT_MESSAGE msg =
  {EV_MOTION_DECT, 0, 0,
  {0, AM_MD_MOTION_NONE, AM_MOTION_L0}};
  static AM_MOTIONT_STATUS motion_status[MDET_MAX_ROIS] =
  {MD_IN_MOTION, MD_IN_MOTION, MD_IN_MOTION, MD_IN_MOTION};
  static AM_MOTION_LEVEL motion_level[MDET_MAX_ROIS] =
  {AM_MOTION_L2, AM_MOTION_L2, AM_MOTION_L2, AM_MOTION_L2};
  static uint32_t motion_level_change_check[MDET_MAX_ROIS][AM_MOTION_L_NUM - 1] =
  {0};
  static int ignore_first_diff[MDET_MAX_ROIS] =
  {0, 0, 0, 0};

  do {
    if (!me1_buf) {
      ERROR("AMMotionDetect::check_motion NULL pointer\n");
      result = false;
      break;
    }

    if (!m_enable) {
      break;
      //if disable motion detect this module reads ME1 data but does not detect motion
    }

    if (skip_cnt -- >= 0) { //Have to add frame skip to avoid too much CPU consumption.
      return true;
    } else {
      skip_cnt = SKIP_CNT;
    }
    if ((*m_inst->md_update_frame)(&mdet_session,
                          m_dsp_mem.addr + m_frame_desc.luma.data_addr_offset,
                          mdet_config.threshold) < 0) {
      ERROR("AMMotionDetect::mdet_update_frame error\n");
      result = false;
      break;
    }
#ifdef PRINT_MD_DATA
    print_md_data(mdet_session);
#endif

    for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
      if (!m_roi_info[i].valid) {
        continue;
        //ignore invalid roi
      }
      if (ignore_first_diff[i] == 0) {
        ignore_first_diff[i] = 1;
        continue;
      }
      mdet_session.motion[i] *= MAX_MOTION_INDICATOR;
      //PRINTF("ROI%d: motion=%f\n", i, mdet_session.motion[i]);
      if (motion_status[i] == MD_NO_MOTION) {
        motion_level_change_check[i][AM_MOTION_L0] = 0;
        motion_level_change_check[i][AM_MOTION_L1] = 0;
        if (mdet_session.motion[i] >= m_threshold[i].threshold[AM_MOTION_L0]) {
          motion_status[i] = MD_IN_MOTION;
          if (mdet_session.motion[i]
              >= m_threshold[i].threshold[AM_MOTION_L1]) {
            motion_level[i] = AM_MOTION_L2;
          } else {
            motion_level[i] = AM_MOTION_L1;
          }
          msg.seq_num ++;
          msg.event_type = m_plugin_id;
          msg.md_msg.roi_id = i;
          msg.md_msg.mt_value = mdet_session.motion[i];
          msg.pts = me1_buf->mono_pts;
          msg.md_msg.mt_type = AM_MD_MOTION_START;
          msg.md_msg.mt_level = motion_level[i];
          if (m_callback && m_callback(&msg) < 0) {
            ERROR("m_callback occur error!\n");
            result = false;
            break;
          }
        }
      } else {
        if (mdet_session.motion[i] < m_threshold[i].threshold[AM_MOTION_L0]) {
          motion_level_change_check[i][AM_MOTION_L0] ++;
          if (motion_level_change_check[i][AM_MOTION_L0]
              >= m_level_change_delay[i].mt_level_change_delay[AM_MOTION_L0]) {
            motion_level_change_check[i][AM_MOTION_L0] = 0;
            motion_level_change_check[i][AM_MOTION_L1] = 0;
            //prevent motion level jump from motion level2 to motion level0
            if (motion_level[i] == AM_MOTION_L2) {
              motion_level[i] = AM_MOTION_L1;
              msg.md_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            } else {
              motion_status[i] = MD_NO_MOTION;
              motion_level[i] = AM_MOTION_L0;
              msg.md_msg.mt_type = AM_MD_MOTION_END;
            }
            if (motion_level[i] != msg.md_msg.mt_level) {
              msg.seq_num ++;
              msg.event_type = m_plugin_id;
              msg.md_msg.roi_id = i;
              msg.md_msg.mt_value = mdet_session.motion[i];
              msg.pts = me1_buf->mono_pts;
              msg.md_msg.mt_level = motion_level[i];
              if (m_callback && m_callback(&msg) < 0) {
                ERROR("m_callback occur error!\n");
                result = false;
                break;
              }
            }
          }
        } else if (mdet_session.motion[i]
            < m_threshold[i].threshold[AM_MOTION_L1]) {
          motion_level_change_check[i][AM_MOTION_L1] ++;
          if (motion_level[i] == AM_MOTION_L2) {
            if (motion_level_change_check[i][AM_MOTION_L1]
                >= m_level_change_delay[i].mt_level_change_delay[AM_MOTION_L1]) {
              motion_level_change_check[i][AM_MOTION_L0] = 0;
              motion_level_change_check[i][AM_MOTION_L1] = 0;
              motion_level[i] = AM_MOTION_L1;
            }
          } else {
            motion_level_change_check[i][AM_MOTION_L0] = 0;
            motion_level_change_check[i][AM_MOTION_L1] = 0;
            motion_level[i] = AM_MOTION_L1;
          }
          if (motion_level[i] != msg.md_msg.mt_level) {
            msg.seq_num ++;
            msg.event_type = m_plugin_id;
            msg.md_msg.roi_id = i;
            msg.md_msg.mt_value = mdet_session.motion[i];
            msg.md_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            msg.pts = me1_buf->mono_pts;
            msg.md_msg.mt_level = motion_level[i];
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              result = false;
              break;
            }
          }
        } else {
          motion_level_change_check[i][AM_MOTION_L0] = 0;
          motion_level_change_check[i][AM_MOTION_L1] = 0;
          motion_level[i] = AM_MOTION_L2;
          if (motion_level[i] != msg.md_msg.mt_level) {
            msg.seq_num ++;
            msg.event_type = m_plugin_id;
            msg.md_msg.roi_id = i;
            msg.md_msg.mt_value = mdet_session.motion[i];
            msg.md_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            msg.pts = me1_buf->mono_pts;
            msg.md_msg.mt_level = motion_level[i];
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              result = false;
              break;
            }
          }
        }
      }
    }
  } while (0);
  return result;
}

void AMMotionDetect::md_main(void *data)
{
  INFO("AMMotionDetect::md_main start to run\n");
  if (!data) {
    ERROR("AMMotionDetect::md_main NULL pointer\n");
    return;
  }
  AMMotionDetect *md = (AMMotionDetect *) data;

  while (!md->m_main_loop_exit) {
    if (md->m_video_reader->query_luma_frame(&md->m_frame_desc,
                                             md->m_source_buffer_id,
                                             false) != AM_RESULT_OK) {
      WARN("AMMotionDetect::get me1 data time out\n");
      sleep(1);
      continue;
    }

    if (!md->check_motion(&md->m_frame_desc)) {
      ERROR("AMMotionDetect::check_motion error\n");
      break;
    }
  }
}

void AMMotionDetect::print_md_config()
{
  PRINTF("source_buffer_id = %d\n", (uint32_t )m_source_buffer_id);
  PRINTF("enable = %d\n", m_enable);
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("threshold info, roi[%d]: %d, %d\n",
           m_threshold[i].roi_id,
           m_threshold[i].threshold[0],
           m_threshold[i].threshold[1]);
  }
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("level_change delay info, roi[%d]: %d, %d\n",
           m_level_change_delay[i].roi_id,
           m_level_change_delay[i].mt_level_change_delay[0],
           m_level_change_delay[i].mt_level_change_delay[1]);
  }
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("m_roi_info, roi[%d]: left=%d, right=%d, top=%d, bottom=%d, valid=%d\n",
           m_roi_info[i].roi_id,
           m_roi_info[i].left,
           m_roi_info[i].right,
           m_roi_info[i].top,
           m_roi_info[i].bottom,
           m_roi_info[i].valid);
  }
}

bool AMMotionDetect::sync_config()
{
  MotionDetectParam *motion_detect_param = m_config->get_config(m_conf_path);
  if (!motion_detect_param) {
    ERROR("AMMotionDetect::motion detect get config failed!\n");
    return false;
  }
  motion_detect_param->enable = m_enable;
  motion_detect_param->buf_id = (uint32_t) m_source_buffer_id;
  memcpy(motion_detect_param->roi_info,
         m_roi_info,
         sizeof(motion_detect_param->roi_info));
  memcpy(motion_detect_param->th, m_threshold, sizeof(motion_detect_param->th));
  memcpy(motion_detect_param->lc_delay,
         m_level_change_delay,
         sizeof(motion_detect_param->lc_delay));

  return m_config->set_config(motion_detect_param, m_conf_path);
}
