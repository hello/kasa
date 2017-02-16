/**
 * am_event_sender.h
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_EVENT_SENDER_H_
#define _AM_EVENT_SENDER_H_

#include "am_event_sender_if.h"

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
    virtual bool send_event() override;
    virtual uint32_t version() override;

  private:
    AMEventSender(AMIEngine *engine);
    virtual ~AMEventSender();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    virtual void on_run() override;
    AM_PTS get_current_pts();

  private:
    int                             m_hw_timer_fd;
    bool                            m_run;
    AM_PTS                          m_last_pts;
    uint32_t                        m_input_num;
    uint32_t                        m_output_num;
    vector<AMEventSenderOutput*>    m_output;
    vector<AMFixedPacketPool*>      m_packet_pool;
    AMEventSenderConfig            *m_config;
    EventSenderConfig              *m_event_config;
    AMEvent                        *m_event;
    std::mutex                      m_mutex;
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
