/**
 * am_event_sender.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_event.h"

#include <vector>
#include <mutex>

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"
#include "am_muxer_codec_info.h"

#include "am_event_sender_config.h"
#include "am_event_sender.h"
#include "am_event_sender_version.h"

#ifndef AUTO_LOCK
#define AUTO_LOCK(mtx) std::lock_guard<std::mutex> lck(mtx)
#endif

#define HW_TIMER  ((const char*)"/proc/ambarella/ambarella_hwtimer")

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMEventSender::create(engine, config,
                                              input_num, output_num);
}

AMIEventSender* AMEventSender::create(AMIEngine *engine,
                                      const std::string& config,
                                      uint32_t input_num,
                                      uint32_t output_num)
{
  AMEventSender *result = new AMEventSender(engine);
  if (result && (AM_STATE_OK != result->init(config, input_num, output_num))) {
    delete result;
    result = nullptr;
  }
  return result;
}

void* AMEventSender::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIEventSender) ?
      (AMIEventSender*)this :
      inherited::get_interface(ref_iid);
}

void AMEventSender::destroy()
{
  inherited::destroy();
}

void AMEventSender::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMEventSender::get_input_pin(uint32_t index)
{
  ERROR("%s doesn't have input pin!", m_name);
  return nullptr;
}

AMIPacketPin* AMEventSender::get_output_pin(uint32_t index)
{
  AMIPacketPin * pin = (index < m_output_num) ? m_output[index] : nullptr;
  if (!pin) {
    ERROR("No such output pin [index: %u]!", index);
  }
  return pin;
}

uint32_t AMEventSender::version()
{
  return EVENT_SENDER_VERSION_NUMBER;
}

AMEventSender::AMEventSender(AMIEngine *engine) :
    inherited(engine),
    m_hw_timer_fd(-1),
    m_run(false),
    m_last_pts(0),
    m_input_num(0),
    m_output_num(0),
    m_config(nullptr),
    m_event_config(nullptr),
    m_event(nullptr)
{

}

AMEventSender::~AMEventSender()
{
  if (m_hw_timer_fd > 0) {
    close(m_hw_timer_fd);
    m_hw_timer_fd = -1;
  }

  for (auto &v : m_output) {
    AM_DESTROY(v);
  }
  m_output.clear();

  for (auto &v : m_packet_pool) {
    AM_DESTROY(v);
  }
  m_packet_pool.clear();

  AM_DESTROY(m_event);
  delete m_config;
}

AM_STATE AMEventSender::init(const std::string& config,
              uint32_t input_num,
              uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;

  do {
    if ((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0) {
      state = AM_STATE_ERROR;
      break;
    }

    m_input_num = input_num;
    m_output_num = output_num;
    if (!(m_config = new AMEventSenderConfig())) {
      ERROR("Failed to create config module for EventSender filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    if (!(m_event_config = m_config->get_config(config))) {
      ERROR("Can't get configuration file: %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }

    if ((state = inherited::init((const char*)m_event_config->name.c_str(),
                                 m_event_config->real_time.enabled,
                                 m_event_config->real_time.priority))
        != AM_STATE_OK) {
      break;
    }

    for (uint32_t i = 0; i < m_output_num; ++i) {
      AMEventSenderOutput *output = nullptr;
      AMFixedPacketPool *pool = nullptr;

      if (!(output = AMEventSenderOutput::create(this))) {
        state = AM_STATE_ERROR;
        ERROR("Failed to create output pin[%d]", i);
        break;
      } else {
        m_output.push_back(output);
      }

      if (!(pool =
          AMFixedPacketPool::create("EventSenderPool",
                                    m_event_config->packet_pool_size,
                                    64))) {
        state = AM_STATE_NO_MEMORY;
        ERROR("Failed to create Event Sender packet pool[%d]!", i);
        break;
      } else {
        output[i].set_packet_pool(pool);
        m_packet_pool.push_back(pool);
      }
    }

    if (state != AM_STATE_OK) {
      break;
    }

    if (!(m_event = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent!");
      break;
    }
  } while (0);

  if (state != AM_STATE_OK) {
    for (auto &v : m_output) {
      AM_DESTROY(v);
    }
    m_output.clear();

    for (auto &v : m_packet_pool) {
      AM_DESTROY(v);
    }
    m_packet_pool.clear();

    if (m_config) {
      delete m_config;
      m_config = nullptr;
    }
    AM_DESTROY(m_event);
  }
  return state;
}

AM_PTS AMEventSender::get_current_pts()
{
  uint8_t pts[32] = {0};
  AM_PTS current_pts = m_last_pts;
  if (m_hw_timer_fd  < 0) {
    return current_pts;
  }
  if (read(m_hw_timer_fd, pts, sizeof(pts)) < 0) {
    PERROR("read");
  } else {
    current_pts = strtoull((const char*)pts, (char**)NULL, 10);
    m_last_pts = current_pts;
  }
  return current_pts;
}

bool AMEventSender::send_event()
{
  bool ret = true;
  AMPacket *send_packet = nullptr;

  AUTO_LOCK(m_mutex);
  for (auto &v : m_output) {
    if (!v->alloc_packet(send_packet)) {
      ret = false;
      continue;
    }
    send_packet->set_type(AMPacket::AM_PAYLOAD_TYPE_EVENT);
    send_packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_EVENT_EMG);
    send_packet->set_data_size(0);
    send_packet->set_pts(get_current_pts());
    v->send_packet(send_packet);
  }

  return ret;
}

AM_STATE AMEventSender::start()
{
  AM_STATE state = AM_STATE_OK;
  AUTO_LOCK(m_mutex);
  m_run = true;
  return state;
}

AM_STATE AMEventSender::stop()
{
  AUTO_LOCK(m_mutex);
  m_run = false;
  m_event->signal();
  return inherited::stop();
}

void AMEventSender::on_run()
{
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));
  ack(AM_STATE_OK);
  post_engine_msg(engine_msg);

  do {
    m_event->wait();
  } while (m_run);
  INFO("%s exits mainloop!", m_name);
}
