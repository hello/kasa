/*
 * am_key_input.h
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] Create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AM_KEY_INPUT_H
#define AM_KEY_INPUT_H

#include <map>

#define EVENT_PATH "/dev/input/event"
#define EVENT_MAX_CHANNEL 1
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
    uint64_t          m_seq_num                = 0ULL;
    AMThread         *m_event_capture_thread   = nullptr;
    AMThread         *m_event_timer_thread     = nullptr;
    AMMutex          *m_key_input_mutex        = nullptr;
    AMMutex          *m_key_map_mutex          = nullptr;
    AMMutex          *m_callback_map_mutex     = nullptr;
    AMCondition      *m_key_pressed_cond       = nullptr;
    AMCondition      *m_key_released_cond      = nullptr;
    AMKeyInputConfig *m_key_input_config       = nullptr;
    int               m_event_cap_ctrl_sock[2] = {-1};
    int               m_hw_timer_fd            = -1;
    uint32_t          m_long_pressed_time      = LONG_PRESS_MAX_TIME;
    EVENT_MODULE_ID   m_plugin_id;
    bool              m_key_event_running      = false;
    bool              m_wait_key_pressed       = false;
    bool              m_key_event_timer        = false;
    AM_EVENT_CALLBACK m_default_callback       = nullptr;
    std::map<AM_KEY_CODE, AM_KEY_NODE>       m_pressed_key_map;
    std::map<AM_KEY_CODE, AM_EVENT_CALLBACK> m_key_callback_map;
    std::string                              m_conf_path = ORYX_EVENT_CONF_DIR;
#define EVT_CAP_CTRL_R m_event_cap_ctrl_sock[0]
#define EVT_CAP_CTRL_W m_event_cap_ctrl_sock[1]
};

#endif /*AM_KEY_INPUT_H*/

