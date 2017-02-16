/*
 * am_key_input.h
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] Create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AM_KEY_INPUT_H
#define AM_KEY_INPUT_H

#define EVENT_PATH "/dev/input/event"
#define EVENT_MAX_CHANNEL 2
#define LONG_PRESS_MAX_TIME	2000	// millisecond
#define LONG_PRESS_MIN_TIME	20	// millsecond for removing jitter

struct AM_KEY_NODE
{
    AM_KEY_STATE key_state;
    struct timeval time_stamp;
};

class AMKeyInput: public AMIEventPlugin
{

  public:
    DECLARE_EVENT_PLUGIN_INTERFACE
    bool get_key_state(AM_KEY_CODE key_code, AM_KEY_STATE *state);
    bool wait_key_pressed(AM_KEY_CODE key_code, AM_KEY_STATE *pResult);
    bool set_key_callback(AM_KEY_CODE key_code, AM_EVENT_CALLBACK callback);
    bool set_long_pressed_time(uint32_t ms);
    uint32_t get_long_pressed_time();
    bool sync_config();

    //AMIEventPlugin
    virtual bool start_plugin();
    virtual bool stop_plugin();
    virtual bool set_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual bool get_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual EVENT_MODULE_ID get_plugin_ID();

  public:
    virtual ~AMKeyInput();

  private:
    AMKeyInput(EVENT_MODULE_ID mid);
    int32_t construct();
    void key_event_capture();
    void event_process(const struct input_event &event);
    static void static_key_event_capture(void* arg);
    void key_event_timer();
    static void static_key_event_timer(void *arg);
    static int64_t time_diff(const struct timeval *start_time,
                             const struct timeval *end_time);
    uint64_t get_current_pts();

  private:
    EVENT_MODULE_ID m_plugin_id;
    uint64_t m_seq_num;
    int m_hw_timer_fd;
    uint32_t m_long_pressed_time;
    AMThread *m_event_capture_thread;
    AMThread *m_event_timer_thread;
    bool m_key_event_running;
    bool m_wait_key_pressed;
    bool m_key_event_timer;
    AMMutex *m_key_input_mutex;
    AMMutex *m_key_map_mutex;
    AMMutex *m_callback_map_mutex;
    AMCondition *m_key_pressed_cond;
    AMCondition *m_key_released_cond;
    std::map<AM_KEY_CODE, AM_KEY_NODE> m_pressed_key_map;
    std::map<AM_KEY_CODE, AM_EVENT_CALLBACK> m_key_callback_map;
    AMKeyInputConfig *m_key_input_config;
    std::string m_conf_path;
};

#endif /*AM_KEY_INPUT_H*/

