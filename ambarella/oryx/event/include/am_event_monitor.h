/*******************************************************************************
 * am_event_monitor.h
 *
 * Histroy:
 *  2012-8-9 2012 - [Zhikan Yang] created file
 *  2014-10- 21     [Louis Sun] changed the name to am_event_monitor.h for Oryx
 *  2014-11-20     [Dongge Wu] reconstruct and refine
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_EVENT_MONITOR_H_
#define AM_EVENT_MONITOR_H_

#include <atomic>

class AMEventMonitor: public AMIEventMonitor
{
    friend AMIEventMonitor;

  public:
    bool register_module(EVENT_MODULE_ID module_id, const char *module_path);
    bool is_module_registered(EVENT_MODULE_ID module_id);
    bool start_event_monitor(EVENT_MODULE_ID module_id);
    bool stop_event_monitor(EVENT_MODULE_ID module_id);
    bool destroy_event_monitor(EVENT_MODULE_ID module_id);

    bool start_all_event_monitor();
    bool stop_all_event_monitor();
    bool destroy_all_event_monitor();

    bool set_monitor_config(EVENT_MODULE_ID module_id,
                            EVENT_MODULE_CONFIG *config);
    bool get_monitor_config(EVENT_MODULE_ID module_id,
                            EVENT_MODULE_CONFIG *config);

  private:
    AMIEventPlugin* get_event_plugin(EVENT_MODULE_ID module_id);
    bool valid_module_id(EVENT_MODULE_ID module_id);

    /* Constructor && Destructor */
  private:
    AMEventMonitor();
    bool construct();
    virtual ~AMEventMonitor();

    AMEventMonitor(AMEventMonitor const &copy) = delete; // Not Implemented
    AMEventMonitor& operator=(AMEventMonitor const &copy) = delete;  // Not Implemented

  private:
    static AMEventMonitor *this_instance;
    std::atomic_int ref_counter;

  private:
    const char *m_module_files[EV_ALL_MODULE_NUM];
    void *m_module_handles[EV_ALL_MODULE_NUM];
    AMIEventPlugin *m_event_plugin[EV_ALL_MODULE_NUM];

  protected:
    static AMEventMonitor* get_instance();
    void release();
    void inc_ref();
};

#endif //AM_EVENT_MONITOR_H_
