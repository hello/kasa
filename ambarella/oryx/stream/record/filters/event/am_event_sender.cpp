/**
 * am_event_sender.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_event.h"

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
    m_last_pts(0),
    m_config(nullptr),
    m_event_config(nullptr),
    m_event(nullptr),
    m_hw_timer_fd(-1),
    m_input_num(0),
    m_output_num(0),
    m_run(false)
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
      ERROR("%s: %s", HW_TIMER, strerror(errno));
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
        output->set_packet_pool(pool);
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

bool AMEventSender::send_event(AMEventStruct& event)
{
  bool ret = true;
  AMPacket *send_packet = nullptr;
  do {
    if (!check_event_params(event)) {
      ERROR("Event params is not valid.");
      ret = false;
      break;
    }
    AUTO_MEM_LOCK(m_mutex);
    for (auto &v : m_output) {
      if (!v->alloc_packet(send_packet)) {
        ret = false;
        continue;
      }
      send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      if (event.attr == AM_EVENT_H26X) {
        send_packet->set_stream_id(event.h26x.stream_id);
      } else if (event.attr == AM_EVENT_MJPEG) {
        send_packet->set_stream_id(event.mjpeg.stream_id);
      } else if (event.attr == AM_EVENT_PERIODIC_MJPEG) {
        send_packet->set_stream_id(event.periodic_mjpeg.stream_id);
      } else if (event.attr == AM_EVENT_STOP_CMD) {
        send_packet->set_stream_id(event.stop_cmd.stream_id);
      } else {
        ERROR("The event attr error.");
        ret = false;
        break;
      }
      send_packet->set_type(AMPacket::AM_PAYLOAD_TYPE_EVENT);
      send_packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_EVENT_EMG);
      send_packet->set_data_size(sizeof(AMEventStruct));
      uint8_t* data_ptr = send_packet->get_data_ptr();
      memcpy(data_ptr, (uint8_t*)(&event), sizeof(event));
      send_packet->set_pts(get_current_pts());
      v->send_packet(send_packet);
    }
  } while(0);
  return ret;
}

AM_STATE AMEventSender::start()
{
  AM_STATE state = AM_STATE_OK;
  AUTO_MEM_LOCK(m_mutex);
  m_run = true;
  return state;
}

AM_STATE AMEventSender::stop()
{
  AUTO_MEM_LOCK(m_mutex);
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
  } while (m_run.load());
  INFO("%s exits mainloop!", m_name);
}

bool AMEventSender::check_event_params(AMEventStruct& event)
{
  bool ret = true;
  time_t timep;
  struct tm *local_time;
  do {
    time(&timep);
    local_time = localtime(&timep);
    uint32_t current_second = local_time->tm_hour * 3600 +
        local_time->tm_min * 60 + local_time->tm_sec;
    switch (event.attr) {
      case AM_EVENT_H26X : {
        if (event.h26x.stream_id < 0) {
          ERROR("AM_EVENT_H26X event id is invalid.");
          ret = false;
          break;
        }
      } break;
      case AM_EVENT_MJPEG : {
        if ((event.mjpeg.stream_id < 0) || (event.mjpeg.pre_cur_pts_num < 0) ||
            (event.mjpeg.after_cur_pts_num < 0) ||
            (event.mjpeg.closest_cur_pts_num < 0)) {
          ERROR("AM_EVENT_MJPEG params are invalid.");
          ret = false;
          break;
        }
      } break;
      case AM_EVENT_PERIODIC_MJPEG : {
        if ((event.periodic_mjpeg.stream_id < 0) ||
            (event.periodic_mjpeg.interval_second <= 0) ||
            (event.periodic_mjpeg.once_jpeg_num <= 0)) {
          ERROR("AM_EVENT_PERIODIC_MJPEG params are invalid.");
          ret = false;
          break;
        }
        uint32_t start_second = event.periodic_mjpeg.start_time_hour * 3600 +
            event.periodic_mjpeg.start_time_minute * 60 +
            event.periodic_mjpeg.start_time_second;
        uint32_t end_second = event.periodic_mjpeg.end_time_hour * 3600 +
            event.periodic_mjpeg.end_time_minute * 60 +
            event.periodic_mjpeg.end_time_second;
        if (start_second >= end_second) {
          ERROR("Start time should be smaller than end time.");
          ret = false;
          break;
        }
        if (current_second > start_second) {
          ERROR("The start time should be smaller than current time.");
          ret = false;
          break;
        }
      } break;
      case AM_EVENT_STOP_CMD : {
        ret = true;
      } break;
      default : {
        ERROR("Event attr error.");
        ret = false;
      } break;
    }
  } while(0);
  return ret;
}
