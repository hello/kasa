/*******************************************************************************
 * am_event_monitor.h
 *
 * Histroy:
 *  2012-8-9 2012 - [Zhikan Yang] created file
 *  2014-10- 21     [Louis Sun] changed the name to am_event_monitor.h for Oryx
 *  2014-11-20     [Dongge Wu] reconstruct and refine
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
