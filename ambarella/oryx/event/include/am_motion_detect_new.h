/*******************************************************************************
 * am_motion_detect_new.h
 *
 * History:
 *   Sep 14, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MOTION_DETECT_NEW_H_
#define AM_MOTION_DETECT_NEW_H_

#include "am_base_event_plugin.h"

class AMThread;
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
    AMMotionDetect(EVENT_MODULE_ID mid);
    bool construct();
    void print_md_config();
    bool check_motion(const AMQueryFrameDesc &me1_buf);
    static void md_main(void *data);
    bool set_md_state(bool enable);
    bool get_md_state(bool *enable);
    bool set_md_buffer_id(AM_SOURCE_BUFFER_ID source_buf_id);
    bool get_md_buffer_id(AM_SOURCE_BUFFER_ID *source_buf_id);
    bool check_md_roi_format(AM_EVENT_MD_ROI *roi_info);
    bool set_roi_info(AM_EVENT_MD_ROI *roi_info);
    bool get_roi_info(AM_EVENT_MD_ROI *roi_info);
    bool set_threshold_info(AM_EVENT_MD_THRESHOLD *threshold);
    bool get_threshold_info(AM_EVENT_MD_THRESHOLD *threshold);
    bool set_level_change_delay_info(
        AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay);
    bool get_level_change_delay_info(
        AM_EVENT_MD_LEVEL_CHANGE_DELAY *level_change_delay);
    bool set_md_callback(AM_EVENT_CALLBACK callback);

  private:
    AMQueryFrameDesc                m_frame_desc;
    mdet_session_t                  mdet_session;
    mdet_cfg                        mdet_config;
    std::string                     m_conf_path;
    bool                            m_main_loop_exit;
    bool                            m_started;
    bool                            m_enable;
    EVENT_MODULE_ID                 m_plugin_id;
    AM_SOURCE_BUFFER_ID             m_source_buffer_id;
    AMIVideoReaderPtr               m_video_reader;
    AMIVideoAddressPtr              m_video_address;
    AM_EVENT_CALLBACK               m_callback;
    AMThread                       *m_md_thread;
    mdet_instance                  *m_inst;
    AMMotionDetectConfig           *m_config;
    AM_EVENT_MD_ROI                *m_roi_info;
    AM_EVENT_MD_THRESHOLD          *m_threshold;
    AM_EVENT_MD_LEVEL_CHANGE_DELAY *m_level_change_delay;
};

#endif /* AM_MOTION_DETECT_NEW_H_ */
