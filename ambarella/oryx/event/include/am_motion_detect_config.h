/*******************************************************************************
 * am_motion_detect_config.h
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
#ifndef AM_MOTION_DETECT_CONFIG_H_
#define AM_MOTION_DETECT_CONFIG_H_

#include "am_log.h"
#include "am_base_event_plugin.h"
#include "am_event_types.h"
#include "am_configure.h"

#define MAX_ROI_NUM 4

struct MotionDetectParam
{
    bool enable;
    uint32_t buf_id;
    AM_EVENT_MD_ROI roi_info[MAX_ROI_NUM];
    AM_EVENT_MD_THRESHOLD th[MAX_ROI_NUM];
    AM_EVENT_MD_LEVEL_CHANGE_DELAY lc_delay[MAX_ROI_NUM];
    /*MotionDetectParam():
    enable(false),
    buf_id(0),
    roi_info[MAX_ROI_NUM]({{0,0,0,0,0,false},{1,0,0,0,0,false},{2,0,0,0,0,false},{3,0,0,0,0,false}}),
    th[MAX_ROI_NUM]({{0,0,0},{1,0,0},{2,0,0},{3,0,0}}),
    lc_delay[MAX_ROI_NUM]({{0,0,0},{1,0,0},{2,0,0},{3,0,0}})
    {
    }*/
};

class AMMotionDetectConfig
{
  public:
    AMMotionDetectConfig();
    virtual ~AMMotionDetectConfig();
    MotionDetectParam* get_config(const std::string& cfg_file_path);
    bool set_config(MotionDetectParam *md_config,
                    const std::string& cfg_file_path);
  private:
    AMConfig *m_config;
    MotionDetectParam *m_md_config;
};

#endif /* AM_MOTION_DETECT_CONFIG_H_ */
