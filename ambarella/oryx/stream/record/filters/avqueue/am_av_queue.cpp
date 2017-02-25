/**
 * am_av_queue.cpp
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
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

#include <unistd.h>
#include <map>
#include <mutex>
#include <vector>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"
#include "am_muxer_codec_info.h"
#include "am_thread.h"
#include "am_event.h"
#include "am_mutex.h"
#include "h265.h"
#include "am_video_types.h"

#include "am_av_queue_builder.h"
#include "am_av_queue_config.h"
#include "am_av_queue.h"
#include "am_av_queue_version.h"

#include "am_plugin.h"
#include "am_muxer_if.h"

#define STREAM_MAX_NUM 4
#define H264_SCALE 90000

#ifndef _AUTO_LOCK
#define _AUTO_LOCK(mtx) std::lock_guard<std::recursive_mutex> lck (mtx)
#endif

static std::recursive_mutex m_mutex;

/*
 * AMAVQueue
 */

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMAVQueue::create(engine, config,
                                          input_num, output_num);
}

void* AMAVQueue::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIAVQueue) ? (AMIAVQueue*)this :
      inherited::get_interface(refiid);
}

void AMAVQueue::get_info(INFO& info)
{
  info.num_in   = m_input_num;
  info.num_out  = m_output_num;
  info.name     = m_name;
}

AMIPacketPin* AMAVQueue::get_input_pin(uint32_t index)
{
  AMIPacketPin *pin = nullptr;

  if (index >= 0 && index < m_input_num) {
    pin = m_input[index];
  } else {
    ERROR("No such input pin [index: %u]", index);
  }

  return pin;
}

AMIPacketPin* AMAVQueue::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = nullptr;

  if (index >= 0 && index < m_output_num) {
    pin = m_output[index];
  } else {
    ERROR("No such output pin [index: %u]", index);
  }

  return pin;
}

AM_STATE AMAVQueue::stop()
{
  INFO("%s begin to stop", m_name);
  m_stop = true;
  m_send_cond->signal();
  m_event_send_cond->signal();
  return inherited::stop();
}

AMIAVQueue* AMAVQueue::create(AMIEngine *engine,
                              const std::string& config,
                              uint32_t input_num,
                              uint32_t output_num)
{
  AMAVQueue *result = new AMAVQueue(engine);
  if (AM_UNLIKELY(result &&
                  (AM_STATE_OK != result->init(config,
                                               input_num,
                                               output_num)))) {
    delete result;
    result = NULL;
  }

  return (AMIAVQueue*)result;
}

uint32_t AMAVQueue::version()
{
  return AV_QUEUE_VERSION_NUMBER;
}

AMAVQueue::AMAVQueue(AMIEngine *engine) :
    inherited(engine),
    m_event_end_pts(0),
    m_avqueue_config(nullptr),
    m_config(nullptr),
    m_direct_out(nullptr),
    m_file_out(nullptr),
    m_send_mutex(nullptr),
    m_send_cond(nullptr),
    m_event_thread(nullptr),
    m_event_wait(nullptr),
    m_event_send_mutex(nullptr),
    m_event_send_cond(nullptr),
    m_input_num(0),
    m_output_num(0),
    m_run(~0),
    m_audio_pts_increment(0),
    m_video_payload_count(80),
    m_audio_payload_count(18),
    m_event_id(0),
    m_event_video_id(0),
    m_event_audio_id(0),
    m_is_in_event(false),
    m_event_video_eos(false),
    m_event_video_block(false),
    m_event_audio_block(false),
    m_stop(false),
    m_video_come(false),
    m_video_send_block(0, false),
    m_audio_send_block(0, false)
{
}

AMAVQueue::~AMAVQueue()
{
  m_event_wait->signal();
  AM_DESTROY(m_event_thread);
  AM_DESTROY(m_event_wait);
  AM_DESTROY(m_event_send_mutex);
  AM_DESTROY(m_event_send_cond);
  AM_DESTROY(m_send_mutex);
  AM_DESTROY(m_send_cond);

  for (auto &v : m_input) {
    AM_DESTROY(v);
  }
  m_input.clear();

  for (auto &v : m_output) {
    AM_DESTROY(v);
  }
  m_output.clear();

  for (auto &v : m_packet_pool) {
    AM_DESTROY(v);
  }
  m_packet_pool.clear();

  for (auto &m : m_video_queue) {
    AM_DESTROY(m.second);
  }
  m_video_queue.clear();
  for (auto &m : m_audio_queue) {
    AM_DESTROY(m.second);
  }

  for (auto &m : m_video_info_payload) {
    delete m.second;
  }
  m_video_info_payload.clear();
  for (auto &m : m_audio_info_payload) {
    delete m.second;
  }
  m_audio_info_payload.clear();

  m_video_info.clear();
  m_audio_info.clear();
  m_first_video.clear();
  m_first_audio.clear();
  m_audio_queue.clear();
  m_audio_last_pts.clear();
  m_send_audio_state.clear();
  m_write_video_state.clear();
  m_write_audio_state.clear();
  m_event_audio_id_map.clear();
  delete m_config;
  DEBUG("~AMAVQueue");
}

AM_STATE AMAVQueue::init(const std::string& config,
                         uint32_t input_num,
                         uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (!(m_config = new AMAVQueueConfig())) {
      ERROR("Failed to create AMAVQueueConfig!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    if (!(m_avqueue_config = m_config->get_config(config))) {
      ERROR("Cannot get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }

    m_event_config = m_avqueue_config->event_config;
    if ((state = inherited::init((const char*)m_avqueue_config->name.c_str(),
                                 m_avqueue_config->real_time.enabled,
                                 m_avqueue_config->real_time.priority))
        != AM_STATE_OK) {
      ERROR("Failed to init inherited!");
      break;
    }
    m_input_num = input_num;
    if ((int32_t)(m_input_num) <= 0) {
      ERROR("Invalid input num: %d!", m_input_num);
      break;
    }
    m_output_num = output_num;
    if ((int32_t)(m_output_num) <= 0) {
      ERROR("Invalid output num: %d!", m_output_num);
      break;
    }

    //Create input pins
    for (uint32_t i = 0; i < m_input_num; ++i) {
      AMAVQueueInput *input = nullptr;
      if (!(input = AMAVQueueInput::create(this, i))) {
        state = AM_STATE_ERROR;
        ERROR("Failed to create input pin[%d]!", i);
        break;
      }
      m_input.push_back(input);
    }
    if (state != AM_STATE_OK) {
      break;
    }
    //Create output pins and packet pools
    for (uint32_t i = 0; i < m_output_num; ++i) {
      AMAVQueueOutput     *output = nullptr;
      AMAVQueuePacketPool *pool   = nullptr;
      if (!(output = AMAVQueueOutput::create(this))) {
        state = AM_STATE_ERROR;
        ERROR("Failed to create output pin[%d]", i);
        break;
      } else {
        m_output.push_back(output);
      }

      pool = AMAVQueuePacketPool::create(this,
                                         "AVqueuePacketPool",
                                         m_avqueue_config->packet_pool_size);
      if (!pool) {
        state = AM_STATE_NO_MEMORY;
        ERROR("Failed to create avqueue packet pool[%d]", i);
        break;
      } else {
        m_output[i]->set_packet_pool(pool);
        m_packet_pool.push_back(pool);
      }
    }

    if (state != AM_STATE_OK) {
      break;
    }

    if (!(m_send_mutex = AMMutex::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMMutex!");
      break;
    }
    if (!(m_send_cond = AMCondition::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMCondition!");
      break;
    }

    if (!(m_event_send_mutex = AMMutex::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMMutex!");
      break;
    }
    if (!(m_event_send_cond = AMCondition::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMCondition!");
      break;
    }

    if (!(m_event_wait = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent!");
      break;
    }
  } while (0);
  return state;
}

void AMAVQueue::destroy()
{
  inherited::destroy();
}

void AMAVQueue::release_packet(AMPacket *packet)
{
  if (!m_file_out) {
    return;
  }
  uint32_t stream_id = packet->get_stream_id();
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        m_video_queue[stream_id]->event_release((ExPayload*)packet->get_payload());
      } else {
        m_video_queue[stream_id]->normal_release((ExPayload*)packet->get_payload());
      }
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        m_audio_queue[stream_id]->event_release((ExPayload*)packet->get_payload());
      } else {
        m_audio_queue[stream_id]->normal_release((ExPayload*)packet->get_payload());
      }
    } break;
    default: break;
  }
}

AM_STATE AMAVQueue::process_packet(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;

  if (!m_stop.load()) {
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        state = on_info(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        state = on_data(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EVENT: {
        state = on_event(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        state = on_eos(packet);
      } break;
      default:
        break;
    }
  }
  packet->release();
  return state;
}

bool AMAVQueue::is_ready_for_event(AMEventStruct& event)
{
  bool ret = false;
  do {
    if ((event.attr == AM_EVENT_MJPEG) || (event.attr == AM_EVENT_PERIODIC_MJPEG)) {
      ret = true;
      break;
    } else if (event.attr == AM_EVENT_H26X) {
      if (m_event_config.find(event.h26x.stream_id) == m_event_config.end() ||
     m_event_audio_id_map.find(event.h26x.stream_id) == m_event_audio_id_map.end()) {
        break;
      }
      uint32_t video_id = m_event_config[event.h26x.stream_id].video_id;
      uint32_t audio_id = m_event_audio_id_map[event.h26x.stream_id];

      if ((m_first_video.find(video_id) == m_first_video.end()) ||
          (m_first_audio.find(audio_id) == m_first_audio.end())) {
        break;
      }
      ret = !m_is_in_event &&
          m_first_video[video_id].first &&
          m_first_audio[audio_id].first &&
          m_event_config[event.h26x.stream_id].enable;
    } else {
      ERROR("Event attr error.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

void AMAVQueue::static_event_thread(void *p)
{
  ((AMAVQueue*)p)->event_thread();
}

void AMAVQueue::event_thread()
{
  do {
    m_event_wait->wait();
    process_event();
  } while (!m_stop.load());
  INFO("%s exits!", m_event_thread->name());
}

void AMAVQueue::process_event()
{
  while (true) {
    send_event_packet();
    if (m_event_video_eos || m_stop.load()) {
      m_is_in_event = false;
      m_event_video_eos = false;
      break;
    }
  }
}

AM_STATE AMAVQueue::send_normal_packet()
{
  AM_STATE       state          = AM_STATE_OK;
  AM_PTS         min_pts        = INT64_MAX;//(0x7fffffffffffffffLL)
  AMPacket      *send_packet    = nullptr;
  AMRingQueue   *queue_ptr      = nullptr;
  AMPacket::AM_PAYLOAD_TYPE payload_type;
  AMPacket::AM_PAYLOAD_ATTR payload_attr;
  uint32_t stream_id;
  if (m_video_come.load()) {
    /* compare every video and audio packets,
     * find a minimum PTS packet to send
     */
    for (auto &m : m_audio_queue) {
      if (!m_first_audio[m.first].first || !m.second) {
        continue;
      }
      if (!(m_run & (1 << (m.first + STREAM_MAX_NUM)))) {
        continue;
      }
      m_send_mutex->lock();
      if (m.second->empty(queue_mode::normal)) {
        switch (m_send_audio_state[m.first]) {
          case 0: //Normal
            m_send_audio_state[m.first] = 1;
            m_audio_last_pts[m.first] += m_audio_pts_increment - 100;
            break;
          case 1: //Try to get audio
            break;
          case 2: //Need block to wait audio packet
            while (!m_stop && m.second->empty(queue_mode::normal)) {
              m_audio_send_block.first = m.first;
              m_audio_send_block.second = true;
              m_send_cond->wait(m_send_mutex);
              m_audio_send_block.second = false;
            }
            m_audio_last_pts[m.first] = m.second->get()->m_data.m_payload_pts;
            m_send_audio_state[m.first] = 0;
            break;
          default:
            break;
        }
      } else {
        m_audio_last_pts[m.first] = m.second->get()->m_data.m_payload_pts;
        if (m_send_audio_state[m.first]) {
          m_send_audio_state[m.first] = 0;
        }
      }
      m_send_mutex->unlock();
      if (m_stop) {
        return state;
      }

      if (m_audio_last_pts[m.first] < min_pts) {
        min_pts = m_audio_last_pts[m.first];
        queue_ptr = m.second;
      }
    }
    for (auto &m : m_video_queue) {
      if (!m_first_video[m.first].first || !m.second) {
        continue;
      }
      if (!(m_run & (1 << m.first))) {
        continue;
      }
      m_send_mutex->lock();
      while (!m_stop && m.second->empty(queue_mode::normal)) {
        m_video_send_block.first = m.first;
        m_video_send_block.second = true;
        m_send_cond->wait(m_send_mutex);
        m_video_send_block.second = false;
      }
      m_send_mutex->unlock();
      if (m_stop) {
        return state;
      }
      if (m.second->get()->m_data.m_payload_pts < min_pts) {
        min_pts = m.second->get()->m_data.m_payload_pts;
        queue_ptr = m.second;
      }
    }
    for (auto &m : m_audio_queue) {
      if (queue_ptr == m.second && m_send_audio_state[m.first]) {
        m_send_audio_state[m.first] = 2; //Need block to wait
        return state;
      }
    }
    if (queue_ptr) {
      if (!m_file_out->alloc_packet(send_packet)) {
        WARN("Failed to allocate packet!");
        return AM_STATE_ERROR;
      }
      send_packet->set_payload(queue_ptr->get());
      payload_type = send_packet->get_type();
      payload_attr = send_packet->get_attr();
      stream_id = send_packet->get_stream_id();
      if (payload_type == AMPacket::AM_PAYLOAD_TYPE_EOS) {
        switch (payload_attr) {
          case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
            m_run &= ~(1 << stream_id);
            break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
            m_run &= ~(1 << (stream_id + STREAM_MAX_NUM));
            break;
          default:
            break;
        }
      }
      queue_ptr->read_pos_inc(queue_mode::normal);
      m_file_out->send_packet(send_packet);
    } else {
      usleep(20000);
    }
  } else {
    usleep(20000);
  }

  return state;
}

AM_STATE AMAVQueue::send_event_packet()
{
  AM_STATE state = AM_STATE_OK;

  do {
    AM_PTS       min_pts        = INT64_MAX;
    AMPacket    *send_packet    = nullptr;
    ExPayload   *send_payload   = nullptr;
    AMRingQueue *queue_ptr      = nullptr;

    while (!m_stop.load() &&
        m_video_queue[m_event_video_id]->empty(queue_mode::event)) {
      m_event_send_mutex->lock();
      m_event_video_block = true;
      m_event_send_cond->wait(m_event_send_mutex);
      m_event_video_block = false;
      m_event_send_mutex->unlock();
    }
    if (m_stop.load()) {break;}

    send_payload = m_video_queue[m_event_video_id]->event_get();
    if (send_payload->m_data.m_payload_pts < min_pts) {
      min_pts = send_payload->m_data.m_payload_pts;
      queue_ptr = m_video_queue[m_event_video_id];
    }

    while (!m_stop.load() &&
        m_audio_queue[m_event_audio_id]->empty(queue_mode::event)) {
      m_event_send_mutex->lock();
      m_event_audio_block = true;
      m_event_send_cond->wait(m_event_send_mutex);
      m_event_audio_block = false;
      m_event_send_mutex->unlock();
    }
    if (m_stop.load()) {break;}

    send_payload = m_audio_queue[m_event_audio_id]->event_get();
    if (send_payload->m_data.m_payload_pts < min_pts) {
      min_pts = send_payload->m_data.m_payload_pts;
      queue_ptr = m_audio_queue[m_event_audio_id];
    }
    send_payload = queue_ptr->event_get();

    if (queue_ptr) {
      if (!m_file_out->alloc_packet(send_packet)) {
        ERROR("Failed to allocate packet!");
        return state;
      }
      send_packet->set_payload(send_payload);
      send_packet->set_event_id(m_event_id);

      if (min_pts >= m_event_end_pts) {
        m_event_video_eos = true;
        send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT |
                                     AMPacket::AM_PACKET_TYPE_STOP);
        INFO("Event stop PTS: %lld.", min_pts);
      } else {
        send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      }

      queue_ptr->read_pos_inc(queue_mode::event);
      m_file_out->send_packet(send_packet);
    }
  } while (0);
  return state;
}

AM_STATE AMAVQueue::check_output_pins()
{
  AM_STATE state = AM_STATE_OK;

  for (uint32_t i = 0; i < m_output_num; ++i) {
    AMIPacketPin *peer_pin = m_output[i]->get_peer();
    if (AM_UNLIKELY(!peer_pin)) {
      NOTICE("Output PIN[%u] is not connected!", i);
      state = AM_STATE_ERROR;
      break;
    }
    AMIPacketFilter *peer_filter = peer_pin->get_filter();
    AM_MUXER_TYPE type = peer_filter ?
        (((AMIMuxer*)peer_filter->get_interface(IID_AMIMuxer))->type()) :
        AM_MUXER_TYPE_NONE;

    if (type == AM_MUXER_TYPE_DIRECT) {
      m_direct_out = m_output[i];
      INFO("%s is connected to Direct Muxer!", m_name);
    } else if (type == AM_MUXER_TYPE_FILE) {
      m_file_out = m_output[i];
      INFO("%s is connected to File Muxer!", m_name);
    } else {
      ERROR("Output pin: %s type error: %d!", m_name, type);
      state = AM_STATE_ERROR;
      break;
    }
  }

  if (state == AM_STATE_OK &&
      (m_output_num > 1) && (!m_direct_out || !m_file_out)) {
    ERROR("Output connections are not right!");
    state = AM_STATE_ERROR;
  }
  return state;
}

void AMAVQueue::on_run()
{
  bool is_ok = true;
  reset();
  AmMsg engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));
  ack(AM_STATE_OK);
  post_engine_msg(engine_msg);
  uint32_t count = 0;
  if (check_output_pins() != AM_STATE_OK) {
    engine_msg.code = AMIEngine::ENG_MSG_ERROR;
    post_engine_msg(engine_msg);
    return;
  }
  for (uint32_t i = 0; i < m_input_num; ++i) {
    m_input[i]->enable(true);
    m_input[i]->run();
  }

  /* Start Event Recording Thread */
  for (auto &m : m_event_config) {
    if (!m.second.enable) {
      continue;
    }
    if (!(m_event_thread = AMThread::create("process_event",
                                            static_event_thread,
                                            this))) {
      ERROR("Failed to create event thread!");
      is_ok = false;
      break;
    }
    break;
  }

  while (is_ok) {
    if (!m_file_out) {
      if (m_stop.load()) {break;}
      sleep(1); //Idle thread
      continue;
    }

    if (!m_stop.load()) {
      if (send_normal_packet() != AM_STATE_OK) {
        break;
      }
    } else {
      uint32_t video_count = 0;
      uint32_t audio_count = 0;
      for (auto &m : m_video_queue) {
        if (!m.second->in_use()) {
          ++video_count;
        }
      }
      for (auto &m : m_audio_queue) {
        if (!m.second->in_use()) {
          ++audio_count;
        }
      }
      if (video_count == m_video_queue.size() &&
          audio_count == m_audio_queue.size()) {
        break;
      } else {
        usleep(50000);
        if (++ count > 20) {
          NOTICE("Count is bigger than 20, exit");
          break;
        }
      }
    }
  }
  for (auto &v : m_input) {
    v->enable(false);
    v->stop();
  }

  m_event_wait->signal();
  AM_DESTROY(m_event_thread);
  m_event_wait->clear();
  if (!is_ok) {
    AmMsg engine_msg(AMIEngine::ENG_MSG_ABORT);
    post_engine_msg(engine_msg);
    NOTICE("%s posts ABORT!", m_name);
  } else {
    INFO("%s exits mainloop!", m_name);
  }
}

AM_STATE AMAVQueue::on_info(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;

  if (!packet) {
    ERROR("Packet is null!");
    return AM_STATE_ERROR;
  }

  if (m_direct_out) {
    packet->add_ref();
    m_direct_out->send_packet(packet);
  }
  if (m_file_out) {
    packet->add_ref();
    m_file_out->send_packet(packet);
  }

  uint32_t stream_id = packet->get_stream_id();
  AM_AUDIO_INFO *audio_info = nullptr;
  AM_VIDEO_INFO *video_info = nullptr;

  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      video_info = (AM_VIDEO_INFO*)packet->get_data_ptr();
      m_video_info[stream_id] = *video_info;
      switch (video_info->type) {
        case AM_VIDEO_H264:
          INFO("H264 INFO[%d]: Width: %d, Height: %d",
               stream_id,
               video_info->width,
               video_info->height);
          break;
        case AM_VIDEO_H265:
          INFO("H265 INFO[%d]: Width: %d, Height: %d",
               stream_id,
               video_info->width,
               video_info->height);
          break;
        case AM_VIDEO_MJPEG:
          INFO("MJPEG INFO[%d]: Width: %d, Height: %d",
               stream_id,
               video_info->width,
               video_info->height);
          break;
        default:
          state = AM_STATE_ERROR;
          ERROR("Unknown video type: %d!", video_info->type);
          break;
      }

      if (video_info->type == AM_VIDEO_MJPEG) {
        return AM_STATE_OK;
      }

      if (!m_file_out) {break;}

      if ((m_run != uint32_t(~0)) && (m_run & (1 << stream_id))) {
        break;
      }

      uint32_t video_payload_count = m_video_payload_count;
      uint32_t max_history_duration = 0;
      bool is_event = false;
      for (auto &m : m_event_config) {
        if (m.second.enable && (m.second.video_id == stream_id)) {
          is_event = true;
          if (m.second.history_duration > max_history_duration) {
            max_history_duration = m.second.history_duration;
          }
        }
      }

      if (is_event) {
        m_video_info_payload[stream_id] = new AMPacket::Payload();
        *m_video_info_payload[stream_id] = *(packet->get_payload());
        video_payload_count =
            (max_history_duration * video_info->scale / video_info->rate +
            4 * video_info->N) * video_info->slice_num * video_info->tile_num;
      }
      if (!(m_video_queue[stream_id] =
          AMRingQueue::create(video_payload_count))) {
        ERROR("Failed to create video queue[%d]!", stream_id);
        return AM_STATE_ERROR;
      }
      INFO("Video Queue[%d] count: %d", stream_id, video_payload_count);
      m_first_video[stream_id].first = false;
      m_write_video_state[stream_id] = 0;

      m_run = (m_run == (uint32_t)(~0)) ?
          (1 << stream_id) : (m_run | (1 << stream_id));
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      audio_info = (AM_AUDIO_INFO*)packet->get_data_ptr();
      m_audio_info[stream_id] = *audio_info;
      INFO("Audio[%d]: channels: %d, sample rate: %d, "
          "chunk size: %d, pts_increment: %d, sample size : %d",
          stream_id,
          audio_info->channels,
          audio_info->sample_rate,
          audio_info->chunk_size,
          audio_info->pkt_pts_increment,
          audio_info->sample_size);
      m_audio_pts_increment = audio_info->pkt_pts_increment;
      if (!m_file_out) {
        break;
      }
      if ((m_run != uint32_t(~0)) && (m_run & (1 << (stream_id + STREAM_MAX_NUM)))) {
        break;
      }

      uint32_t audio_payload_count = m_audio_payload_count;
      bool is_event = false;
      for (auto &m : m_event_config) {
        if (m.second.enable && (audio_info->type == m.second.audio_type)) {
          is_event = true;
          m_event_audio_id_map[m.first] = stream_id;
          INFO("Event[%d] audio type: %d, id: %d",
               m.first, audio_info->type, stream_id);
          uint32_t packet_per_sec = audio_info->sample_rate /
              audio_info->chunk_size *
              audio_info->sample_size *
              audio_info->channels + 1;
          uint32_t count = packet_per_sec *
              (m.second.history_duration + 20);
          if (count > audio_payload_count) {
            audio_payload_count = count;
          }
        }
      }

      if (is_event) {
        m_audio_info_payload[stream_id] = new AMPacket::Payload();
        *m_audio_info_payload[stream_id] = *packet->get_payload();
      }

      if (!(m_audio_queue[stream_id] =
          AMRingQueue::create(audio_payload_count))) {
        ERROR("Failed to create audio queue[%d]!", stream_id);
        return AM_STATE_ERROR;
      }
      INFO("Audio Queue[%d] count: %d", stream_id, audio_payload_count);
      m_send_audio_state[stream_id] = 0;
      m_write_audio_state[stream_id] = 0;
      m_first_audio[stream_id].first = false;

      m_run = (m_run == (uint32_t)(~0)) ?
          (1 << (stream_id + STREAM_MAX_NUM)) :
          (m_run | (1 << (stream_id + STREAM_MAX_NUM)));
    } break;
    default:
      break;
  }

  return state;
}

AM_STATE AMAVQueue::on_data(AMPacket* packet)
{
  AM_STATE state = AM_STATE_OK;
  if (!packet) {
    ERROR("Packet is null!");
    return AM_STATE_ERROR;
  }
  if (packet->get_pts() < 0) {
    ERROR("Packet pts: %d!", packet->get_pts());
    return AM_STATE_ERROR;
  }
  if (m_direct_out) {
    packet->add_ref();
    m_direct_out->send_packet(packet);
  }
  queue_mode event_mode;
  uint32_t stream_id = packet->get_stream_id();
  AMPacket::Payload *payload = packet->get_payload();
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_SEI:
      break;
    case AMPacket::AM_PAYLOAD_ATTR_GPS : {
      if (!m_file_out) {
        break;
      } else {
        packet->add_ref();
        m_file_out->send_packet(packet);
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (!m_file_out) {
        break;
      }
      if ((packet->get_video_type() == AM_VIDEO_MJPEG)) {
        break;
      }
      event_mode = (m_is_in_event && (stream_id == m_event_video_id)) ?
          queue_mode::event : queue_mode::normal;

      switch (m_write_video_state[stream_id]) {
        case 1: //Video queue is full, wait for sending
          if (m_video_queue[stream_id]->full(event_mode)) {
            break;
          } else {
            m_write_video_state[stream_id] = 2;
            WARN("Video[%d] queue wait I frame!", stream_id);
          }
        case 2: //Video Queue is not full, wait I frame
          if (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR ||
              packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_I) {
            WARN("Video[%d] I frame comes, stop dropping packet!", stream_id);
            m_write_video_state[stream_id] = 0;
          } else {
            break;
          }
        case 0: //Normal write
          if (m_video_queue[stream_id]->full(event_mode)) {
            m_write_video_state[stream_id] = 1;
            WARN("Video[%d] queue is full, start to drop packet", stream_id);
          } else {
            if (!m_video_queue[stream_id]->write(payload, event_mode)) {
              ERROR("Failed to write payload to video queue[%d]!", stream_id);
              state = AM_STATE_ERROR;
              break;
            }

            if (!m_video_come.load()) {m_video_come = true;}

            if ((m_first_video.find(stream_id) == m_first_video.end()) ||
               !m_first_video[stream_id].first) {
              m_first_video[stream_id].second = packet->get_pts();
              m_first_video[stream_id].first = true;
              INFO("First video[%d] frame type: %d, PTS: %lld.",
                   stream_id,
                   packet->get_frame_type(),
                   m_first_video[stream_id].second);
            }

            m_send_mutex->lock();
            if (m_video_send_block.second &&
                (m_video_send_block.first == stream_id)) {
              m_send_cond->signal();
            }
            m_send_mutex->unlock();

            m_event_send_mutex->lock();
            if (m_event_video_block && (m_event_video_id == stream_id)) {
              m_event_send_cond->signal();
            }
            m_event_send_mutex->unlock();
          }
          break;
        default: break;
      }
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      _AUTO_LOCK(m_mutex);
      if (!m_file_out) {
        break;
      }
      event_mode = (m_is_in_event && (stream_id == m_event_audio_id)) ?
          queue_mode::event : queue_mode::normal;

      switch (m_write_audio_state[stream_id]) {
        case 1: //Audio queue is full, wait for sending
          if (m_audio_queue[stream_id]->full((event_mode))) {
            break;
          } else {
            m_write_audio_state[stream_id] = 0;
            WARN("Audio[%d] stop dropping packet!", stream_id);
          }
        case 0: { //Normal write
          if (m_audio_queue[stream_id]->full(event_mode)) {
            WARN("Audio[%d] queue is full, start to drop packet", stream_id);
            m_write_audio_state[stream_id] = 1;
          } else {
            if (!m_audio_queue[stream_id]->write(payload, event_mode)) {
              ERROR("Failed to write payload to audio queue[%d]", stream_id);
              state = AM_STATE_ERROR;
              break;
            }

            if ((m_first_audio.find(stream_id) == m_first_audio.end()) ||
                !m_first_audio[stream_id].first) {
              m_first_audio[stream_id].second = packet->get_pts();
              m_first_audio[stream_id].first = true;
              INFO("First audio PTS: %lld.", m_first_audio[stream_id].second);
            }

            m_send_mutex->lock();
            if (m_audio_send_block.second &&
                (m_audio_send_block.first == stream_id)) {
              m_send_cond->signal();
            }
            m_send_mutex->unlock();

            m_event_send_mutex->lock();
            if (m_event_audio_block && (stream_id == m_event_audio_id)) {
              m_event_send_cond->signal();
            }
            m_event_send_mutex->unlock();
          }
        } break;
        default: break;
      }
    } break;

    default:
      ERROR("Invalid data type!");
      state = AM_STATE_ERROR;
      break;
  }

  return state;
}

AM_STATE AMAVQueue::send_event_info()
{
  AM_STATE state = AM_STATE_OK;
  AMPacket *send_packet = nullptr;

  do {
    if (!m_file_out->alloc_packet(send_packet)) {
      state = AM_STATE_ERROR;
      ERROR("Failed to allocate packet!");
      break;
    }
    send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
    send_packet->set_payload(m_video_info_payload[m_event_video_id]);
    send_packet->set_data_ptr((uint8_t*)&m_video_info[m_event_video_id]);
    send_packet->set_data_size(sizeof(AM_VIDEO_INFO));
    send_packet->set_event_id(m_event_id);
    m_file_out->send_packet(send_packet);

    if (!m_file_out->alloc_packet(send_packet)) {
      state = AM_STATE_ERROR;
      ERROR("Failed to allocate packet!");
      break;
    }
    send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
    send_packet->set_payload(m_audio_info_payload[m_event_audio_id]);
    send_packet->set_data_ptr((uint8_t*)&m_audio_info[m_event_audio_id]);
    send_packet->set_data_size(sizeof(AM_AUDIO_INFO));
    send_packet->set_event_id(m_event_id);
    m_file_out->send_packet(send_packet);
  } while (0);

  return state;
}

AM_STATE AMAVQueue::on_event(AMPacket *packet)
{
  _AUTO_LOCK(m_mutex);
  AM_STATE state = AM_STATE_OK;
  do {
    AMEventStruct* event = (AMEventStruct*)(packet->get_data_ptr());
    if (event->attr == AM_EVENT_MJPEG){
      INFO("AMEventSender send event packet, event attr is AM_EVENT_MJPEG,"
          "stream id is %u, pre num is %u, after number is %u, "
          "closest num is %u", event->mjpeg.stream_id,
          event->mjpeg.pre_cur_pts_num, event->mjpeg.after_cur_pts_num,
          event->mjpeg.closest_cur_pts_num);
      if (m_direct_out) {
        packet->add_ref();
        m_direct_out->send_packet(packet);
      }
      break;
    } else if (event->attr == AM_EVENT_H26X) {
      INFO("AMEventSender send event packet, event attr is AM_EVENT_H26X,"
          "event id is %u", event->h26x.stream_id);
    } else if (event->attr == AM_EVENT_PERIODIC_MJPEG) {
      INFO("AMEventSender send event packet, event attr is "
          "AM_EVENT_PERIODIC_MJPEG, stream id is %u, interval second is %u, "
          "start time is %u-%u-%u, end time is %u-%u-%u",
          event->periodic_mjpeg.stream_id, event->periodic_mjpeg.interval_second,
          event->periodic_mjpeg.start_time_hour,
          event->periodic_mjpeg.start_time_minute,
          event->periodic_mjpeg.start_time_second,
          event->periodic_mjpeg.end_time_hour,
          event->periodic_mjpeg.end_time_minute,
          event->periodic_mjpeg.end_time_second);
      if (m_direct_out) {
        packet->add_ref();
        m_direct_out->send_packet(packet);
      }
      break;
    } else {
      ERROR("Event attr error.");
      state = AM_STATE_ERROR;
      break;
    }
    m_event_id = event->h26x.stream_id;
    m_event_video_id = m_event_config[m_event_id].video_id;
    m_event_audio_id = m_event_audio_id_map[m_event_id];
    INFO("Event ID: %d, Video ID: %d, Audio ID: %d",
         m_event_id, m_event_video_id, m_event_audio_id);
    if (m_file_out) {
      packet->add_ref();
      m_file_out->send_packet(packet);
    }

    if (!is_ready_for_event(*event)) {
      ERROR("AVQueue is not available for event!");
      return state;
    }

    m_video_queue[m_event_video_id]->event_reset();
    m_audio_queue[m_event_audio_id]->event_reset();
    AM_PTS event_pts = packet->get_pts();
    INFO("Event occurrence PTS: %lld.", event_pts);

    AM_PTS video_start_pts = m_first_video[m_event_video_id].second;
    //AM_PTS audio_start_pts = m_first_audio[m_event_audio_id[m_event_video_id]].second;

    if (event_pts > (video_start_pts +
        m_event_config[m_event_id].history_duration * H264_SCALE)) {
      video_start_pts = event_pts -
          m_event_config[m_event_id].history_duration * H264_SCALE;
    }
    m_event_end_pts = event_pts +
        m_event_config[m_event_id].future_duration * H264_SCALE;
    INFO("Event start PTS: %lld, end PTS: %lld.",
         video_start_pts, m_event_end_pts);

    AM_PTS video_pts = 0;
    AM_PTS audio_pts = 0;
    ExPayload *video_payload = nullptr;
    ExPayload *audio_payload = nullptr;

    //Set Video ReadPos
    while (!m_video_queue[m_event_video_id]->event_empty()) {
      m_video_queue[m_event_video_id]->event_backtrack();
      video_payload = m_video_queue[m_event_video_id]->event_get();
      video_pts = video_payload->m_data.m_payload_pts;

      // Set Audio ReadPos
      while (!m_audio_queue[m_event_audio_id]->event_empty()) {
        audio_payload = m_audio_queue[m_event_audio_id]->event_get_prev();
        audio_pts = audio_payload->m_data.m_payload_pts;
        if (audio_pts < video_pts) {
          break;
        }
        m_audio_queue[m_event_audio_id]->event_backtrack();
      }
      if ((video_pts <= video_start_pts) &&
          (video_payload->m_data.m_frame_type == AM_VIDEO_FRAME_TYPE_IDR)) {
        if (m_video_info[m_event_video_id].type == AM_VIDEO_H264) {
          break;
        } else if (m_video_info[m_event_video_id].type == AM_VIDEO_H265) {
          if (is_h265_IDR_first_nalu(video_payload)) {
            break;
          }
        } else {
          ERROR("video type error.");
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
    INFO("Event: current audio PTS: %lld, video PTS: %lld.",
         audio_pts, video_pts);

    if (send_event_info() != AM_STATE_OK) {
      state = AM_STATE_ERROR;
      ERROR("Failed to send event information!");
      return state;
    }
    m_is_in_event = true;
    m_event_wait->signal();
  } while(0);
  return state;
}

AM_STATE AMAVQueue::on_eos(AMPacket* packet)
{
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
      INFO("Video[%d] EOS", packet->get_stream_id());
      break;
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
      INFO("Audio[%d] EOS", packet->get_stream_id());
      break;
    default:
      break;
  }

  return on_data(packet);
}

void AMAVQueue::reset()
{
  m_stop = false;
  m_run = (uint32_t)(~0);
  m_direct_out = nullptr;
  m_file_out = nullptr;

  m_audio_pts_increment = 0;
  m_audio_last_pts.clear();
  m_send_audio_state.clear();

  m_video_info.clear();
  m_audio_info.clear();

  for (auto &m : m_video_queue) {
    AM_DESTROY(m.second);
  }
  m_video_queue.clear();
  for (auto &m : m_audio_queue) {
    AM_DESTROY(m.second);
  }
  m_audio_queue.clear();

  for (auto &m : m_video_info_payload) {
    delete m.second;
  }
  m_video_info_payload.clear();
  for (auto &m: m_audio_info_payload) {
    delete m.second;
  }
  m_audio_info_payload.clear();

  m_is_in_event = false;
  m_event_video_eos = false;
  m_event_end_pts = 0;
  m_video_come = false;
  m_first_video.clear();
  m_first_audio.clear();
  m_write_video_state.clear();
  m_write_audio_state.clear();
  m_event_audio_id_map.clear();
  m_video_send_block.second = false;
  m_audio_send_block.second = false;
  m_event_video_block = false;
  m_event_audio_block = false;
}

/*
 * AMAVQueuePacketPool
 */

AMAVQueuePacketPool* AMAVQueuePacketPool::create(AMAVQueue *q,
                                                 const char*name,
                                                 uint32_t count)
{
  AMAVQueuePacketPool *result = new AMAVQueuePacketPool();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(q, name, count)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void AMAVQueuePacketPool::on_release_packet(AMPacket *packet)
{
  if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA ||
      packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
    m_avqueue->release_packet(packet);
  }
}

AMAVQueuePacketPool::AMAVQueuePacketPool() :
    m_avqueue(nullptr),
    m_payload(nullptr)
{
}

AMAVQueuePacketPool::~AMAVQueuePacketPool()
{
  DEBUG("~AMAVQueuePacketPool");
}

AM_STATE AMAVQueuePacketPool::init(AMAVQueue  *q,
                                   const char *name,
                                   uint32_t   count)
{
  AM_STATE state = inherited::init(name, count, sizeof(AMPacket));
  if (AM_LIKELY(AM_STATE_OK == state)) {
    m_avqueue = q;
  }
  return state;
}

/*
 * AMAVQueueInput
 */

AM_STATE AMAVQueueInput::process_packet(AMPacket *packet)
{
  return ((AMAVQueue*)get_filter())->process_packet(packet);
}

#define DELIMITER_SIZE 3
bool AMAVQueue::is_h265_IDR_first_nalu(ExPayload *video_payload)
{
  bool ret = false;
  uint8_t *data = video_payload->m_data.m_buffer;;
  uint32_t len = video_payload->m_data.m_size;
  do {
    if (AM_LIKELY(data && (len > 2 * DELIMITER_SIZE))) {
      uint8_t *last_header = data + len - 2 * DELIMITER_SIZE;
      while (data <= last_header) {
        if (AM_UNLIKELY((0x00000001
            == (0 | (data[0] << 16) | (data[1] << 8) | data[2]))
            && (((int32_t )((0x7E & data[3]) >> 1) == H265_IDR_W_RADL)
                || ((int32_t )((0x7E & data[3]) >> 1) == H265_IDR_N_LP)))) {
          data += DELIMITER_SIZE;
          if (((data[2] & 0x80) >> 7) == 1) {
            ret = true;
            break;
          } else {
            ret = false;
            break;
          }
        } else if (data[2] != 0) {
          data += 3;
        } else if (data[1] != 0) {
          data += 2;
        } else {
          data += 1;
        }
      }
    } else {
      if (AM_LIKELY(!data)) {
        ERROR("Invalid bit stream!");
        ret = false;
        break;
      }
      if (AM_LIKELY(len <= DELIMITER_SIZE)) {
        ERROR("Bit stream is less equal than %d bytes!", DELIMITER_SIZE);
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}
