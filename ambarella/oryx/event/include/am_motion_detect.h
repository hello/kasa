/**
 * am_motion_detect.h
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

#ifndef AM_MOTION_DETECT_H_
#define AM_MOTION_DETECT_H_

#include "am_log.h"
#include "am_thread.h"
#include "am_base_event_plugin.h"
#include "am_motion_detect_config.h"

class AMMotionDetectConfig;

class AMMotionDetect: public AMIEventPlugin
{
  public:
    DECLARE_EVENT_PLUGIN_INTERFACE
    virtual bool start_plugin();
    virtual bool stop_plugin();
    virtual bool set_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual bool get_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual EVENT_MODULE_ID get_plugin_ID();
    virtual ~AMMotionDetect();
    bool sync_config();

  private:
    bool m_enable;
    AM_ENCODE_SOURCE_BUFFER_ID m_source_buffer_id;
    AM_EVENT_MD_ROI *m_roi_info;
    AM_EVENT_MD_THRESHOLD *m_threshold;
    AM_EVENT_MD_LEVEL_CHANGE_DELAY *m_level_change_delay;
  private:
    AMMotionDetect(EVENT_MODULE_ID mid);
    bool construct();
    void print_md_config();
    bool check_motion(struct AMQueryDataFrameDesc *me1_buf);
    static void md_main(void *data);
    bool set_md_state(bool enable);
    bool get_md_state(bool *enable);
    bool set_md_buffer_id(AM_ENCODE_SOURCE_BUFFER_ID source_buf_id);
    bool get_md_buffer_id(AM_ENCODE_SOURCE_BUFFER_ID *source_buf_id);
    bool check_md_roi_format(AM_EVENT_MD_ROI *roi_info);
    bool set_roi_info(AM_EVENT_MD_ROI *roi_info);
    bool get_roi_info(AM_EVENT_MD_ROI *roi_info);
    bool set_threshold_info(AM_EVENT_MD_THRESHOLD *threshold);
    bool get_threshold_info(AM_EVENT_MD_THRESHOLD *threshold);
    bool set_level_change_delay_info(AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay);
    bool get_level_change_delay_info(AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay);
    bool set_md_callback(AM_EVENT_CALLBACK callback);

  private:
    bool m_main_loop_exit;
    EVENT_MODULE_ID m_plugin_id;
    AMIVideoReaderPtr m_video_reader;
    AMMemMapInfo m_dsp_mem;
    AMQueryDataFrameDesc m_frame_desc;
    AMThread *m_md_thread;
    mdet_instance *m_inst;
    mdet_session_t mdet_session;
    mdet_cfg mdet_config;
    AM_EVENT_CALLBACK m_callback;
    AMMotionDetectConfig *m_config;
    std::string m_conf_path;
    bool m_started;
};

#endif /* AM_MOTION_DETECT_H_ */
