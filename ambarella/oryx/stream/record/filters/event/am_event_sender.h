/**
 * am_event_sender.h
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
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
#ifndef _AM_EVENT_SENDER_H_
#define _AM_EVENT_SENDER_H_

#include "am_event_sender_if.h"
#include "am_mutex.h"
#include <atomic>
#include <vector>

class AMIEventSender;
class AMEventSenderOutput;
using std::vector;
class AMEventSender: public AMPacketActiveFilter, public AMIEventSender
{
    typedef AMPacketActiveFilter inherited;

  public:
    static AMIEventSender* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);
    virtual void* get_interface(AM_REFIID ref_iid) override;
    virtual AM_STATE start() override;
    virtual AM_STATE stop() override;
    virtual void destroy() override;
    virtual void get_info(INFO& info) override;
    virtual AMIPacketPin* get_input_pin(uint32_t index) override;
    virtual AMIPacketPin* get_output_pin(uint32_t index) override;
    virtual bool send_event(AMEventStruct& event) override;
    virtual uint32_t version() override;

  private:
    AMEventSender(AMIEngine *engine);
    virtual ~AMEventSender();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    virtual void on_run() override;
    AM_PTS get_current_pts();
    bool check_event_params(AMEventStruct& event);

  private:
    AM_PTS                            m_last_pts;
    AMEventSenderConfig              *m_config;
    EventSenderConfig                *m_event_config;
    AMEvent                          *m_event;
    int                               m_hw_timer_fd;
    uint32_t                          m_input_num;
    uint32_t                          m_output_num;
    std::atomic_bool                  m_run;
    std::vector<AMEventSenderOutput*> m_output;
    std::vector<AMFixedPacketPool*>   m_packet_pool;
    AMMemLock                         m_mutex;
};

class AMEventSenderOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend AMEventSender;

  public:
    static AMEventSenderOutput* create(AMPacketFilter *filter)
    {
      AMEventSenderOutput *result = new AMEventSenderOutput(filter);
      if (result && (AM_STATE_OK != result->init())) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMEventSenderOutput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMEventSenderOutput(){}
    AM_STATE init()
    {
      return AM_STATE_OK;
    }
};

#endif /* _AM_EVENT_SENDER_H_ */
