/*******************************************************************************
 * am_motion_detect_config.cpp
 *
 * History:
 *   Jan 22, 2015 - [binwang] created file
 *
 * Copyright (C) 2015-2019, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_configure.h"
#include "am_video_types.h"
#include "am_video_reader_if.h"
#include "am_event_types.h"
#include "am_motion_detect_config.h"

AMMotionDetectConfig::AMMotionDetectConfig() :
    m_config(NULL),
    m_md_config(NULL)
{
}

AMMotionDetectConfig::~AMMotionDetectConfig()
{
  delete m_config;
  delete m_md_config;
}

MotionDetectParam* AMMotionDetectConfig::get_config(const std::string& cfg_file_path)
{
  uint32_t i;

  do {
    delete m_config;
    m_config = NULL;
    m_config = AMConfig::create(cfg_file_path.c_str());
    if (!m_config) {
      ERROR("AMMotionDetectConfig::Create AMConfig failed!\n");
      break;
    }

    if (!m_md_config) {
      m_md_config = new MotionDetectParam();
      if (!m_md_config) {
        ERROR("AMMotionDetectConfig::new motion detect config failed!\n");
        break;
      }
    }

    AMConfig &md_config = *m_config;

    if (md_config["md_enable"].exists()) {
      m_md_config->enable = md_config["md_enable"].get<bool>(true);
    } else {
      m_md_config->enable = true;
    }

    if (md_config["md_source_buffer"].exists()) {
      m_md_config->buf_id = md_config["md_source_buffer"].get<uint32_t>(0);
    } else {
      m_md_config->buf_id = 0;
    }

    for (i = 0; i < MAX_ROI_NUM; i ++) {

      m_md_config->roi_info[i].roi_id = i;
      m_md_config->th[i].roi_id = i;
      m_md_config->lc_delay[i].roi_id = i;

      if (md_config["md_roi"][i]["valid"].exists()) {
        m_md_config->roi_info[i].valid =
            md_config["md_roi"][i]["valid"].get<bool>(false);
      } else {
        m_md_config->roi_info[i].valid = false;
      }
      if (md_config["md_roi"][i]["left"].exists()) {
        m_md_config->roi_info[i].left =
            md_config["md_roi"][i]["left"].get<uint32_t>(0);
      } else {
        m_md_config->roi_info[i].left = 0;
      }
      if (md_config["md_roi"][i]["right"].exists()) {
        m_md_config->roi_info[i].right = md_config["md_roi"][i]["right"].get<
            uint32_t>(0);
      } else {
        m_md_config->roi_info[i].right = 0;
      }
      if (md_config["md_roi"][i]["top"].exists()) {
        m_md_config->roi_info[i].top =
            md_config["md_roi"][i]["top"].get<uint32_t>(0);
      } else {
        m_md_config->roi_info[i].top = 0;
      }
      if (md_config["md_roi"][i]["bottom"].exists()) {
        m_md_config->roi_info[i].bottom = md_config["md_roi"][i]["bottom"].get<
            uint32_t>(0);
      } else {
        m_md_config->roi_info[i].bottom = 0;
      }

      if (md_config["md_th"][i]["th1"].exists()) {
        m_md_config->th[i].threshold[0] =
            md_config["md_th"][i]["th1"].get<uint32_t>(100);
      } else {
        m_md_config->th[i].threshold[0] = 100;
      }
      if (md_config["md_th"][i]["th2"].exists()) {
        m_md_config->th[i].threshold[1] =
            md_config["md_th"][i]["th2"].get<uint32_t>(500);
      } else {
        m_md_config->th[i].threshold[1] = 500;
      }

      if (md_config["md_lc_delay"][i]["ml0_delay"].exists()) {
        m_md_config->lc_delay[i].mt_level_change_delay[0] =
            md_config["md_lc_delay"][i]["ml0_delay"].get<uint32_t>(30);
      } else {
        m_md_config->lc_delay[i].mt_level_change_delay[0] = 30;
      }
      if (md_config["md_lc_delay"][i]["ml1_delay"].exists()) {
        m_md_config->lc_delay[i].mt_level_change_delay[1] =
            md_config["md_lc_delay"][i]["ml1_delay"].get<uint32_t>(6);
      } else {
        m_md_config->lc_delay[i].mt_level_change_delay[1] = 6;
      }

    }
  } while (0);

  return m_md_config;
}

bool AMMotionDetectConfig::set_config(MotionDetectParam *md_config,
                                      const std::string& cfg_file_path)
{
  bool result = true;
  uint32_t i;

  do {
    delete m_config;
    m_config = NULL;
    m_config = AMConfig::create(cfg_file_path.c_str());
    if (!m_config) {
      ERROR("AMMotionDetectConfig::Create AMConfig failed!\n");
      break;
    }

    if (!m_md_config) {
      m_md_config = new MotionDetectParam();
      if (!m_md_config) {
        ERROR("AMMotionDetectConfig::new motion detect config failed!\n");
        break;
      }
    }

    memcpy(m_md_config, md_config, sizeof(MotionDetectParam));
    AMConfig &md_config = *m_config;

    md_config["md_enable"] = m_md_config->enable;

    md_config["md_source_buffer"] = m_md_config->buf_id;

    for (i = 0; i < MAX_ROI_NUM; i ++) {

      m_md_config->roi_info[i].roi_id = i;
      m_md_config->th[i].roi_id = i;
      m_md_config->lc_delay[i].roi_id = i;

      if (md_config["md_roi"][i]["valid"].exists()) {
        md_config["md_roi"][i]["valid"] = m_md_config->roi_info[i].valid;
      }
      if (md_config["md_roi"][i]["left"].exists()) {
        md_config["md_roi"][i]["left"] = m_md_config->roi_info[i].left;
      }
      if (md_config["md_roi"][i]["right"].exists()) {
        md_config["md_roi"][i]["right"] = m_md_config->roi_info[i].right;
      }
      if (md_config["md_roi"][i]["top"].exists()) {
        md_config["md_roi"][i]["top"] = m_md_config->roi_info[i].top;
      }
      if (md_config["md_roi"][i]["bottom"].exists()) {
        md_config["md_roi"][i]["bottom"] = m_md_config->roi_info[i].bottom;
      }

      if (md_config["md_th"][i]["th1"].exists()) {
        md_config["md_th"][i]["th1"] = m_md_config->th[i].threshold[0];
      }
      if (md_config["md_th"][i]["th2"].exists()) {
        md_config["md_th"][i]["th2"] = m_md_config->th[i].threshold[1];
      }

      if (md_config["md_lc_delay"][i]["ml0_delay"].exists()) {
        md_config["md_lc_delay"][i]["ml0_delay"] = m_md_config->lc_delay[i].mt_level_change_delay[0];
      }
      if (md_config["md_lc_delay"][i]["ml1_delay"].exists()) {
        md_config["md_lc_delay"][i]["ml1_delay"] = m_md_config->lc_delay[i].mt_level_change_delay[1];
      }
    }

    if (!md_config.save()) {
      ERROR("AMMotionDetectConfig: failed to save_config\n");
      result = false;
      break;
    }

  } while (0);

  return result;
}
