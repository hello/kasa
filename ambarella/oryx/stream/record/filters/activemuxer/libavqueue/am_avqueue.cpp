/*******************************************************************************
 * am_avqueue.cpp
 *
 * History:
 *   Sep 21, 2016 - [Shupeng Ren] created file
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
#include "am_base_include.h"

#include "am_log.h"
#include "am_define.h"
#include "am_thread.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_packet.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet_pool.h"
#include "h265.h"

#include "am_video_types.h"
#include "am_audio_define.h"
#include "am_record_event_sender.h"
#include "am_ring_queue.h"
#include "am_avqueue.h"

#define PTS_SCALE 90000
#define NORMAL_BUF_DURATION 4 //unit: second
enum {
  VIDEO_MAX_NUM = 4,
  AUDIO_MAX_NUM = 16,
};

using namespace std::placeholders;

static std::mutex m_mutex;

AMAVQueuePtr AMAVQueue::create(AVQueueConfig &config,
                               std::string name,
                               bool start_send_normal_pkt,
                               recv_cb_t callback)
{
  AMAVQueue *result = new AMAVQueue();
  if (result && (result->construct(config, name, start_send_normal_pkt,
                                   callback) != AM_STATE_OK)) {
    delete result;
    result = nullptr;
  }
  return AMAVQueuePtr(result, [](AMAVQueue *p){delete p;});
}

AM_STATE AMAVQueue::construct(AVQueueConfig &config,
                              std::string name,
                              bool start_send_normal_pkt,
                              recv_cb_t callback)
{
  AM_STATE state = AM_STATE_OK;
  do {
    m_name = name + std::string("-avqueue");
    if (!(m_recv_cb = callback)) {
      state = AM_STATE_NOT_SUPPORTED;
      ERROR("Callback is NULL in %s, please check!", m_name.c_str());
      break;
    }
    m_config = config;
    m_is_sending_normal_pkt = start_send_normal_pkt;
    std::string pool_name = m_name + "Pool";
    if (!(m_packet_pool = AMAVQueuePacketPool::create(this, pool_name.c_str(),
                                               m_config.pkt_pool_size))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create avqueue packet pool in %s!", m_name.c_str());
      break;
    }
    if (!(m_event_wait = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent in %s!", m_name.c_str());
      break;
    }
    if (!(m_send_normal_pkt_wait = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent in %s!",  m_name.c_str());
      break;
    }
    if (!(m_info_pkt_process_event = AMEvent::create())) {
      state = AM_STATE_ERROR;
      ERROR("Failed to create AMEvent in %s!",  m_name.c_str());
      break;
    }
    std::string normal_thread_name = m_name + std::string("normal_pkt_thread");
    if (!(m_send_normal_pkt_thread = AMThread::create(normal_thread_name.c_str(),
                                               static_send_normal_packet_thread,
                                               this))) {
      state = AM_STATE_OS_ERROR;
      ERROR("Failed to create send_normal_packet_thread!");
      break;
    }
    std::string event_thread_name = m_name + std::string("event_pkt_thread");
    if (!(m_send_event_pkt_thread = AMThread::create(event_thread_name.c_str(),
                                              static_send_event_packet_thread,
                                              this))) {
      state = AM_STATE_OS_ERROR;
      ERROR("Failed to create send_event_packet_thread in %s!", m_name.c_str());
      break;
    }
  } while (0);
  return state;
}

AM_STATE AMAVQueue::process_packet(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;

  if (!m_stop.load()) {
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        packet->add_ref();
        state = on_info(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        packet->add_ref();
        state = on_data(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EVENT: {
        packet->add_ref();
        state = on_event(packet);
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        packet->add_ref();
        state = on_eos(packet);
      } break;
      default:
        break;
    }
  }
  packet->release();
  return state;
}

void AMAVQueue::release_packet(AMPacket *packet)
{
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
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR : {
      if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        m_gsensor_queue[stream_id]->event_release((ExPayload*)packet->get_payload());
      } else {
        m_gsensor_queue[stream_id]->normal_release((ExPayload*)packet->get_payload());
      }
    } break;
    default: break;
  }
}

void AMAVQueue::static_send_normal_packet_thread(void *p)
{
  ((AMAVQueue*)p)->send_normal_packet_thread();
}

void AMAVQueue::static_send_event_packet_thread(void *p)
{
  ((AMAVQueue*)p)->send_event_packet_thread();
}

void AMAVQueue::send_normal_packet_thread()
{
  if (!m_is_sending_normal_pkt.load()) {
    m_send_normal_pkt_wait->wait();
  }
  while (true) {
    //Send normal packets
    static uint32_t count = 0;
    if (!m_stop.load()) {
      if (send_normal_packet() != AM_STATE_OK) {
        break;
      }
    } else {
      uint32_t video_count = 0;
      uint32_t audio_count = 0;
      uint32_t gsensor_count = 0;
      for (auto &m : m_video_queue) {
        if (!m.second->in_use()) {
          ++ video_count;
        }
      }
      for (auto &m : m_audio_queue) {
        if (!m.second->in_use()) {
          ++ audio_count;
        }
      }
      for (auto &m : m_gsensor_queue) {
        if (!m.second->in_use()) {
          ++ gsensor_count;
        }
      }
      if (video_count == m_video_queue.size() &&
          audio_count == m_audio_queue.size() &&
          gsensor_count == m_gsensor_queue.size()) {
        break;
      } else {
        usleep(50000);
        if (++ count > 20) {
          NOTICE("Count is bigger than 20, exit send normal pkt thread in %s",
                 m_name.c_str());
          break;
        }
      }
    }
  };
  INFO("%s exits!", m_send_normal_pkt_thread->name());
}

void AMAVQueue::send_event_packet_thread()
{
  while (true) {
    //Send event packets
    if (m_in_event.load()) {
      if (send_event_packet() != AM_STATE_OK) {
        break;
      }
      if (m_event_video_eos || m_stop.load()) {
        m_in_event = false;
        m_event_video_eos = false;
      }
    } else {
      m_event_wait->wait();
    }
    if (m_stop.load()) {
      break;
    }
  };
  INFO("%s exits!", m_send_event_pkt_thread->name());
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
  do {
    while(m_info_pkt_queue.size() > 0) {
      AMPacket *info_pkt = m_info_pkt_queue.front_and_pop();
      if (process_info_pkt(info_pkt) != AM_STATE_OK) {
        ERROR("Process info pkt error in %s.", m_name.c_str());
        state = AM_STATE_ERROR;
        m_info_pkt_process_event->signal();
        return state;
      }
      m_info_pkt_process_event->signal();
    }
    if (m_video_come.load()) {
      /* compare all kinds of data packets, find a minimum PTS packet to send
       */
      /*process audio data*/
      for (auto &m : m_audio_queue) {
        if (!m_first_audio[m.first].first || !m.second) {
          /*audio is not coming*/
          continue;
        }
        if (!(m_run & (1 << (m.first + VIDEO_MAX_NUM)))) {
          continue;
        }
        if (!m_audio_info_sent[m.first]) {
          if (send_normal_info(AM_AVQUEUE_MEDIA_AUDIO, m.first) != AM_STATE_OK) {
            ERROR("Failed to send normal info pkt in avqueue.");
            state = AM_STATE_ERROR;
            return state;
          }
          m_audio_info_sent[m.first] = true;
        }
        {
          std::unique_lock<std::mutex> lk(m_send_mutex);
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
                  m_send_cond.wait(lk);
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
        }
        if (m_stop) {
          return state;
        }

        if (m_audio_last_pts[m.first] < min_pts) {
          min_pts = m_audio_last_pts[m.first];
          queue_ptr = m.second;
        }
      }
      /*process gsensor data*/
      for (auto &m : m_gsensor_queue) {
        if (!m_first_gsensor[m.first].first || !m.second) {
          /*gsensor is not coming*/
          continue;
        }
        if (!(m_run & (1 << (m.first + VIDEO_MAX_NUM + AUDIO_MAX_NUM)))) {
          continue;
        }
        if (!m_gsensor_info_sent[m.first]) {
          if (send_normal_info(AM_AVQUEUE_MEDIA_GSENSOR, m.first) != AM_STATE_OK) {
            ERROR("Failed to send normal info pkt in avqueue.");
            state = AM_STATE_ERROR;
            return state;
          }
          m_gsensor_info_sent[m.first] = true;
        }
        {
          std::unique_lock<std::mutex> lk(m_send_mutex);
          if (m.second->empty(queue_mode::normal)) {
            switch (m_send_gsensor_state[m.first]) {
              case 0: //Normal
                m_send_gsensor_state[m.first] = 1;
                m_gsensor_last_pts[m.first] += m_gsensor_pts_increment - 100;
                break;
              case 1: //Try to get gsensor
                break;
              case 2: //Need block to wait gsensor packet
                while (!m_stop && m.second->empty(queue_mode::normal)) {
                  m_gsensor_send_block.first = m.first;
                  m_gsensor_send_block.second = true;
                  m_send_cond.wait(lk);
                  m_gsensor_send_block.second = false;
                }
                m_gsensor_last_pts[m.first] = m.second->get()->m_data.m_payload_pts;
                m_send_gsensor_state[m.first] = 0;
                break;
              default:
                break;
            }
          } else {
            m_gsensor_last_pts[m.first] = m.second->get()->m_data.m_payload_pts;
            if (m_send_gsensor_state[m.first]) {
              m_send_gsensor_state[m.first] = 0;
            }
          }
        }
        if (m_stop) {
          return state;
        }

        if (m_gsensor_last_pts[m.first] < min_pts) {
          min_pts = m_gsensor_last_pts[m.first];
          queue_ptr = m.second;
        }
      }
      /*process video data*/
      for (auto &m : m_video_queue) {
        if (!m_first_video[m.first].first || !m.second) {
          continue;
        }
        if (!(m_run & (1 << m.first))) {
          continue;
        }
        if (!m_video_info_sent[m.first]) {
          if (send_normal_info(AM_AVQUEUE_MEDIA_VIDEO, m.first) != AM_STATE_OK) {
            ERROR("Failed to send normal info pkt in avqueue.");
            state = AM_STATE_ERROR;
            return state;
          }
          m_video_info_sent[m.first] = true;
        }
        {
          std::unique_lock<std::mutex> lk(m_send_mutex);
          while (!m_stop && m.second->empty(queue_mode::normal)) {
            m_video_send_block.first = m.first;
            m_video_send_block.second = true;
            m_send_cond.wait(lk);
            m_video_send_block.second = false;
          }
        }
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
      for (auto &m : m_gsensor_queue) {
        if (queue_ptr == m.second && m_send_gsensor_state[m.first]) {
          m_send_gsensor_state[m.first] = 2; //Need block to wait
          return state;
        }
      }
      if (queue_ptr) {
        if (!m_packet_pool->alloc_packet(send_packet)) {
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
              m_run &= ~(1 << (stream_id + VIDEO_MAX_NUM));
              break;
            case AMPacket::AM_PAYLOAD_ATTR_GSENSOR:
              m_run &= ~(1 << (stream_id + VIDEO_MAX_NUM + AUDIO_MAX_NUM));
              break;
            default:
              break;
          }
        }
        queue_ptr->read_pos_inc(queue_mode::normal);
        m_recv_cb(send_packet);
      } else {
        usleep(20000);
      }
    } else {
      usleep(20000);
    }
  } while(0);
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
    /*process video*/
    while (!m_stop.load() &&
        m_video_queue[m_event_video_id]->empty(queue_mode::event)) {
      std::unique_lock<std::mutex> lk(m_event_send_mutex);
      m_event_video_block = true;
      m_event_send_cond.wait(lk);
      m_event_video_block = false;
    }
    if (m_stop.load()) {break;}

    send_payload = m_video_queue[m_event_video_id]->event_get();
    if (send_payload->m_data.m_payload_pts < min_pts) {
      min_pts = send_payload->m_data.m_payload_pts;
      queue_ptr = m_video_queue[m_event_video_id];
    }
    /*process audio*/
    while (!m_stop.load() &&
        m_audio_queue[m_event_audio_id]->empty(queue_mode::event)) {
      std::unique_lock<std::mutex> lk(m_event_send_mutex);
      m_event_audio_block = true;
      m_event_send_cond.wait(lk);
      m_event_audio_block = false;
    }
    if (m_stop.load()) {break;}

    send_payload = m_audio_queue[m_event_audio_id]->event_get();
    if (send_payload->m_data.m_payload_pts < min_pts) {
      min_pts = send_payload->m_data.m_payload_pts;
      queue_ptr = m_audio_queue[m_event_audio_id];
    }
    /*process G-sensor*/
    if (m_config.event_gsensor_enable) {
      while (!m_stop.load() &&
          m_gsensor_queue[m_event_gsensor_id]->empty(queue_mode::event)) {
        std::unique_lock<std::mutex> lk(m_event_send_mutex);
        m_event_gsensor_block = true;
        m_event_send_cond.wait(lk);
        m_event_gsensor_block = false;
      }
      if (m_stop.load()) {break;}

      send_payload = m_gsensor_queue[m_event_gsensor_id]->event_get();
      if (send_payload->m_data.m_payload_pts < min_pts) {
        min_pts = send_payload->m_data.m_payload_pts;
        queue_ptr = m_gsensor_queue[m_event_gsensor_id];
      }
    }
    send_payload = queue_ptr->event_get();
    if (queue_ptr) {
      if (!m_packet_pool->alloc_packet(send_packet)) {
        WARN("Failed to allocate event packet in %s!", m_name.c_str());
        return AM_STATE_ERROR;
      }
      send_packet->set_payload(send_payload);
      if ((min_pts >= m_event_end_pts) || (m_event_stop)) {
        m_event_video_eos = true;
        send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT |
                                     AMPacket::AM_PACKET_TYPE_STOP);
        INFO("Event stop PTS: %lld in %s.", min_pts, m_name.c_str());
      } else {
        send_packet->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      }
      queue_ptr->read_pos_inc(queue_mode::event);
      m_recv_cb(send_packet);
    }
  } while (0);
  return state;
}

AM_STATE AMAVQueue::send_event_info()
{
  AM_STATE state = AM_STATE_OK;
  do {
    /*send video info*/
    AMPacket *video_info_pkt = nullptr;
    if (!m_packet_pool->alloc_packet(video_info_pkt)) {
      WARN("Failed to allocate event packet in %s!", m_name.c_str());
      state = AM_STATE_ERROR;
      break;
    }

    video_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
    video_info_pkt->set_payload(&m_video_info_payload[m_event_video_id]);
    video_info_pkt->set_data_ptr((uint8_t*)&m_video_info[m_event_video_id]);
    video_info_pkt->set_data_size(sizeof(AM_VIDEO_INFO));
    video_info_pkt->set_stream_id(m_event_video_id);
    m_recv_cb(video_info_pkt);
    /*send audio info*/
    AMPacket *audio_info_pkt = nullptr;
    if (!m_packet_pool->alloc_packet(audio_info_pkt)) {
      WARN("Failed to allocate event packet in %s!", m_name.c_str());
      state = AM_STATE_ERROR;
      break;
    }
    audio_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
    audio_info_pkt->set_payload(&m_audio_info_payload[m_event_audio_id]);
    audio_info_pkt->set_data_ptr((uint8_t*)&m_audio_info[m_event_audio_id]);
    audio_info_pkt->set_data_size(sizeof(AM_AUDIO_INFO));
    audio_info_pkt->set_stream_id(m_event_audio_id);
    m_recv_cb(audio_info_pkt);
    /*send g-sensor info*/
    if (m_config.event_gsensor_enable) {
      AMPacket *gsensor_info_pkt = nullptr;
      if (!m_packet_pool->alloc_packet(gsensor_info_pkt)) {
        WARN("Failed to allocate event packet in %s!", m_name.c_str());
        state = AM_STATE_ERROR;
        break;
      }
      gsensor_info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_EVENT);
      gsensor_info_pkt->set_payload(&m_gsensor_info_payload[m_event_gsensor_id]);
      gsensor_info_pkt->set_data_ptr((uint8_t*)&m_gsensor_info[m_event_gsensor_id]);
      gsensor_info_pkt->set_data_size(sizeof(AM_GSENSOR_INFO));
      gsensor_info_pkt->set_stream_id(m_event_gsensor_id);
      m_recv_cb(gsensor_info_pkt);
    }
  } while (0);

  return state;
}

AM_STATE AMAVQueue::send_normal_info(AM_AVQUEUE_MEDIA_TYPE type, uint32_t stream_id)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    AMPacket *info_pkt = nullptr;
    if (!m_packet_pool->alloc_packet(info_pkt)) {
      WARN("Failed to allocate event packet in %s!", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    switch (type) {
      case AM_AVQUEUE_MEDIA_VIDEO : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(&m_video_info_payload[stream_id]);
        info_pkt->set_data_ptr((uint8_t*)&m_video_info[stream_id]);
        info_pkt->set_data_size(sizeof(AM_VIDEO_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      case AM_AVQUEUE_MEDIA_AUDIO : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(&m_audio_info_payload[stream_id]);
        info_pkt->set_data_ptr((uint8_t*)&m_audio_info[stream_id]);
        info_pkt->set_data_size(sizeof(AM_AUDIO_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      case AM_AVQUEUE_MEDIA_GSENSOR : {
        info_pkt->set_packet_type(AMPacket::AM_PACKET_TYPE_NORMAL);
        info_pkt->set_payload(&m_gsensor_info_payload[stream_id]);
        info_pkt->set_data_ptr((uint8_t*)&m_gsensor_info[stream_id]);
        info_pkt->set_data_size(sizeof(AM_GSENSOR_INFO));
        info_pkt->set_stream_id(stream_id);
      } break;
      default : {
        ERROR("media type error.");
        ret = AM_STATE_ERROR;
        info_pkt->release();
        info_pkt = nullptr;
      } break;
    }
    if (info_pkt) {
      m_recv_cb(info_pkt);
    }
  }  while(0);
  return ret;
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
        ERROR("Invalid bit stream in %s!", m_name.c_str());
        ret = false;
        break;
      }
      if (AM_LIKELY(len <= DELIMITER_SIZE)) {
        ERROR("Bit stream is less equal than %d bytes in %s!",
              DELIMITER_SIZE, m_name.c_str());
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMAVQueue::on_info(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!packet) {
      ERROR("Packet is null!");
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_is_sending_normal_pkt) {
      /*The info pkt will be processed in send_normal_packet function
       * to avoid race condition.*/
      m_info_pkt_queue.push_back(packet);
      m_info_pkt_process_event->wait();
    } else {
      ret = process_info_pkt(packet);
    }
  } while(0);
  return ret;
}

AM_STATE AMAVQueue::process_info_pkt(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;

  if (!packet) {
    ERROR("Packet is null!");
    return AM_STATE_ERROR;
  }
  uint32_t stream_id = packet->get_stream_id();
  AM_AUDIO_INFO *audio_info = nullptr;
  AM_VIDEO_INFO *video_info = nullptr;
  AM_GSENSOR_INFO *gsensor_info = nullptr;
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      video_info = (AM_VIDEO_INFO*)packet->get_data_ptr();
      if ((stream_id != m_config.video_id) ||
          (video_info->type != AM_VIDEO_H264 &&
              video_info->type != AM_VIDEO_H265)) {
        break;
      }
      m_video_info[stream_id] = *video_info;
      switch (video_info->type) {
        case AM_VIDEO_H264:
          INFO("%s receive H264 INFO[%d]: Width: %d, Height: %d",
               m_name.c_str(),
               stream_id,
               video_info->width,
               video_info->height);
          break;
        case AM_VIDEO_H265:
          INFO("%s receive H265 INFO[%d]: Width: %d, Height: %d",
               m_name.c_str(),
               stream_id,
               video_info->width,
               video_info->height);
          break;
        case AM_VIDEO_MJPEG:
          INFO("%s receive MJPEG INFO[%d]: Width: %d, Height: %d",
               m_name.c_str(),
               stream_id,
               video_info->width,
               video_info->height);
          break;
        default:
          state = AM_STATE_ERROR;
          ERROR("Unknown video type: %d in %s!",
                video_info->type, m_name.c_str());
          break;
      }
      if ((m_run != uint32_t(~0)) && (m_run & (1 << stream_id))) {
        break;
      }
      uint32_t video_payload_count = NORMAL_BUF_DURATION *
          ((video_info->scale * video_info->mul) /
          (video_info->rate * video_info->div));
      m_video_info_payload[stream_id] = *packet->get_payload();
      m_video_info_sent[stream_id] = false;//normal info pkt has not been sent.
      if (m_config.event_enable) {
        video_payload_count += (m_config.event_max_history_duration *
            ((video_info->scale * video_info->mul) /
             (video_info->rate * video_info->div)));
      }
      video_payload_count *= (video_info->slice_num * video_info->tile_num);
      AMRingQueue *ring_queue = AMRingQueue::create(video_payload_count);
      if (!ring_queue) {
        ERROR("Failed to create video queue[%d] in %s!",
              stream_id, m_name.c_str());
        state = AM_STATE_ERROR;
        break;
      }
      m_video_queue[stream_id] = ring_queue;
      INFO("Video Queue[%d] count: %d in %s", stream_id, video_payload_count
           , m_name.c_str());
      m_first_video[stream_id].first = false;
      m_write_video_state[stream_id] = 0;

      m_run = (m_run == (uint32_t)(~0)) ?
          (1 << stream_id) : (m_run | (1 << stream_id));
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      audio_info = (AM_AUDIO_INFO*)packet->get_data_ptr();
      bool valid_type = false;
      for (auto &m : m_config.audio_types) {
        if (m.first == audio_info->type) {
          for (auto &v : m.second) {
            if (v == audio_info->sample_rate) {
              m_audio_ids.push_back(v);
              valid_type = true;
            }
          }
        }
      }
      if ((m_config.event_audio.first == audio_info->type) &&
          (m_config.event_audio.second == audio_info->sample_rate)) {
        m_event_audio_id = packet->get_stream_id();
        valid_type = true;
      }
      if (!valid_type) {
        break;
      }

      m_audio_info[stream_id] = *audio_info;
      INFO("%s : Audio[%d]: channels: %d, sample rate: %d, "
          "chunk size: %d, pts_increment: %d, sample size : %d",
          m_name.c_str(),
          stream_id,
          audio_info->channels,
          audio_info->sample_rate,
          audio_info->chunk_size,
          audio_info->pkt_pts_increment,
          audio_info->sample_size);
      m_audio_pts_increment = audio_info->pkt_pts_increment;

      if ((m_run != uint32_t(~0)) &&
          (m_run & (1 << (stream_id + VIDEO_MAX_NUM)))) {
        break;
      }

      uint32_t audio_payload_count = NORMAL_BUF_DURATION *
          (PTS_SCALE / audio_info->pkt_pts_increment);
      m_audio_info_payload[stream_id] = *packet->get_payload();
      m_audio_info_sent[stream_id] = false;
      if (m_config.event_enable &&
          (m_config.event_audio.first == audio_info->type) &&
          (m_config.event_audio.second == audio_info->sample_rate)) {
        audio_payload_count +=  (PTS_SCALE / audio_info->pkt_pts_increment) *
                                 m_config.event_max_history_duration;
      }

      if (!(m_audio_queue[stream_id] =
          AMRingQueue::create(audio_payload_count))) {
        ERROR("Failed to create audio queue[%d] in %s!",
              stream_id, m_name.c_str());
        return AM_STATE_ERROR;
      }
      INFO("Audio Queue[%d] count: %d in %s",
           stream_id, audio_payload_count, m_name.c_str());
      m_send_audio_state[stream_id] = 0;
      m_write_audio_state[stream_id] = 0;
      m_first_audio[stream_id].first = false;

      m_run = (m_run == (uint32_t)(~0)) ?
          (1 << (stream_id + VIDEO_MAX_NUM)) :
          (m_run | (1 << (stream_id + VIDEO_MAX_NUM)));
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
      gsensor_info = (AM_GSENSOR_INFO*)packet->get_data_ptr();
      m_gsensor_info[stream_id] = *gsensor_info;
      INFO("%s : gsensor[%d]: sample rate: %d,"
          "sample size: %d, pkt_pts_increment: %d",
          m_name.c_str(),
          stream_id,
          gsensor_info->sample_rate,
          gsensor_info->sample_size,
          gsensor_info->pkt_pts_increment);
      m_gsensor_pts_increment = gsensor_info->pkt_pts_increment;
      if ((m_run != uint32_t(~0)) &&
          (m_run & (1 << (stream_id + VIDEO_MAX_NUM + AUDIO_MAX_NUM)))) {
        break;
      }
      uint32_t gsensor_payload_count = NORMAL_BUF_DURATION *
                 (PTS_SCALE / gsensor_info->pkt_pts_increment);
      m_gsensor_info_payload[stream_id] = *packet->get_payload();
      m_gsensor_info_sent[stream_id] = false;
      if (m_config.event_gsensor_enable) {
        m_event_gsensor_id = packet->get_stream_id();
        gsensor_payload_count += (PTS_SCALE / gsensor_info->pkt_pts_increment) *
         m_config.event_max_history_duration;
      }
      if (!(m_gsensor_queue[stream_id] =
          AMRingQueue::create(gsensor_payload_count))) {
        ERROR("Failed to create gsensor queue[%d] in %s!",
              stream_id, m_name.c_str());
        return AM_STATE_ERROR;
      }
      INFO("gsensor Queue[%d] count: %d in %s",
           stream_id, gsensor_payload_count, m_name.c_str());
      m_send_gsensor_state[stream_id] = 0;
      m_write_gsensor_state[stream_id] = 0;
      m_first_gsensor[stream_id].first = false;

      m_run = (m_run == (uint32_t)(~0)) ?
          (1 << (stream_id + VIDEO_MAX_NUM + AUDIO_MAX_NUM)) :
          (m_run | (1 << (stream_id + VIDEO_MAX_NUM + AUDIO_MAX_NUM)));
    } break;
    default: {
      ERROR("Packet attr error in %s.", m_name.c_str());
    } break;
  }
  packet->release();
  return state;
}

AM_STATE AMAVQueue::on_data(AMPacket* packet)
{
  std::unique_lock<std::mutex> lk(m_mutex);
  AM_STATE state = AM_STATE_OK;
  if (!packet) {
    ERROR("Packet is null in %s!", m_name.c_str());
    return AM_STATE_ERROR;
  }
  if (packet->get_pts() < 0) {
    ERROR("Packet pts: %lld in %s!", packet->get_pts(), m_name.c_str());
    packet->release();
    return AM_STATE_ERROR;
  }

  queue_mode event_mode;
  uint32_t stream_id = packet->get_stream_id();
  AMPacket::Payload *payload = packet->get_payload();
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_SEI: {
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_GPS: {
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
      if (m_gsensor_queue.find(stream_id) == m_gsensor_queue.end()) {
        break;
      }
      event_mode = (m_in_event && m_config.event_gsensor_enable) ?
          queue_mode::event : queue_mode::normal;

      switch (m_write_gsensor_state[stream_id]) {
        case 1: //gsensor queue is full, wait for sending
          if (m_gsensor_queue[stream_id]->full((event_mode))) {
            break;
          } else {
            m_write_gsensor_state[stream_id] = 0;
            WARN("gsensor[%d] stop dropping packet in %s!", stream_id,
                 m_name.c_str());
          }
        case 0: { //Normal write
          if (m_gsensor_queue[stream_id]->full(event_mode)) {
            WARN("gsensor[%d] queue is full, start to drop packet in %s",
                 stream_id, m_name.c_str());
            m_write_gsensor_state[stream_id] = 1;
          } else {
            if (!m_gsensor_queue[stream_id]->write(payload, event_mode)) {
              ERROR("Failed to write payload to gsensor queue[%d] in %s",
                    stream_id, m_name.c_str());
              state = AM_STATE_ERROR;
              break;
            }

            if ((m_first_gsensor.find(stream_id) == m_first_gsensor.end()) ||
                !m_first_gsensor[stream_id].first) {
              m_first_gsensor[stream_id].second = packet->get_pts();
              m_first_gsensor[stream_id].first = true;
              INFO("First gsensor PTS: %lld in %s.",
                   m_first_gsensor[stream_id].second, m_name.c_str());
            }

            {
              std::unique_lock<std::mutex> lk(m_send_mutex);
              if (m_gsensor_send_block.second &&
                  (m_gsensor_send_block.first == stream_id)) {
                m_send_cond.notify_one();
              }
            }

            {
              std::unique_lock<std::mutex> lk(m_event_send_mutex);
              if (m_event_gsensor_block && (stream_id == m_event_gsensor_id)) {
                m_event_send_cond.notify_one();
              }
            }
          }
        } break;
        default: break;
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (m_video_queue.find(stream_id) == m_video_queue.end()) {
        break;
      }
      event_mode = (m_in_event && (stream_id == m_event_video_id)) ?
          queue_mode::event : queue_mode::normal;

      switch (m_write_video_state[stream_id]) {
        case 1: //Video queue is full, wait for sending
          if (m_video_queue[stream_id]->full(event_mode)) {
            break;
          } else {
            m_write_video_state[stream_id] = 2;
            WARN("Video[%d] queue wait I frame in %s!",
                 stream_id, m_name.c_str());
          }
        case 2: //Video Queue is not full, wait I frame
          if (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR ||
              packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_I) {
            WARN("Video[%d] I frame comes, stop dropping packet in %s!",
                 stream_id, m_name.c_str());
            m_write_video_state[stream_id] = 0;
          } else {
            break;
          }
        case 0: //Normal write
          if (m_video_queue[stream_id]->full(event_mode)) {
            m_write_video_state[stream_id] = 1;
            WARN("Video[%d] queue is full, start to drop packet in %s",
                 stream_id, m_name.c_str());
          } else {
            if (!m_video_queue[stream_id]->write(payload, event_mode)) {
              ERROR("Failed to write payload to video queue[%d] in %s!",
                    stream_id, m_name.c_str());
              state = AM_STATE_ERROR;
              break;
            }

            if (!m_video_come.load()) {
              m_video_come = true;
            }

            if ((m_first_video.find(stream_id) == m_first_video.end()) ||
                !m_first_video[stream_id].first) {
              m_first_video[stream_id].second = packet->get_pts();
              m_first_video[stream_id].first = true;
              INFO("First video[%d] frame type: %d, PTS: %lld in %s.",
                   stream_id,
                   packet->get_frame_type(),
                   m_first_video[stream_id].second,
                   m_name.c_str());
            }

            {
              std::unique_lock<std::mutex> lk(m_send_mutex);
              if (m_video_send_block.second &&
                  (m_video_send_block.first == stream_id)) {
                m_send_cond.notify_one();
              }
            }

            {
              std::unique_lock<std::mutex> lk(m_event_send_mutex);
              if (m_event_video_block && (m_event_video_id == stream_id)) {
                m_event_send_cond.notify_one();
              }
            }
          }
          break;
        default: break;
      }
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      if (m_audio_queue.find(stream_id) == m_audio_queue.end()) {
        break;
      }
      event_mode = (m_in_event && (stream_id == m_event_audio_id)) ?
          queue_mode::event : queue_mode::normal;

      switch (m_write_audio_state[stream_id]) {
        case 1: //Audio queue is full, wait for sending
          if (m_audio_queue[stream_id]->full((event_mode))) {
            break;
          } else {
            m_write_audio_state[stream_id] = 0;
            WARN("Audio[%d] stop dropping packet in %s!", stream_id,
                 m_name.c_str());
          }
        case 0: { //Normal write
          if (m_audio_queue[stream_id]->full(event_mode)) {
            WARN("Audio[%d] queue is full, start to drop packet in %s",
                 stream_id, m_name.c_str());
            m_write_audio_state[stream_id] = 1;
          } else {
            if (!m_audio_queue[stream_id]->write(payload, event_mode)) {
              ERROR("Failed to write payload to audio queue[%d] in %s",
                    stream_id, m_name.c_str());
              state = AM_STATE_ERROR;
              break;
            }

            if ((m_first_audio.find(stream_id) == m_first_audio.end()) ||
                !m_first_audio[stream_id].first) {
              m_first_audio[stream_id].second = packet->get_pts();
              m_first_audio[stream_id].first = true;
              INFO("First audio PTS: %lld in %s.",
                   m_first_audio[stream_id].second, m_name.c_str());
            }

            {
              std::unique_lock<std::mutex> lk(m_send_mutex);
              if (m_audio_send_block.second &&
                  (m_audio_send_block.first == stream_id)) {
                m_send_cond.notify_one();
              }
            }

            {
              std::unique_lock<std::mutex> lk(m_event_send_mutex);
              if (m_event_audio_block && (stream_id == m_event_audio_id)) {
                m_event_send_cond.notify_one();
              }
            }
          }
        } break;
        default: break;
      }
    } break;

    default:
      ERROR("Invalid data type in %s!", m_name.c_str());
      state = AM_STATE_ERROR;
      break;
  }
  packet->release();
  return state;
}

AM_STATE AMAVQueue::process_h26x_event(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  do {
    AMEventStruct *event = (AMEventStruct*)(packet->get_data_ptr());
    m_event_video_id = event->h26x.stream_id;
    if (event->h26x.history_duration > m_config.event_max_history_duration) {
      event->h26x.history_duration = m_config.event_max_history_duration;
    }

    INFO("Event Video ID: %d, Audio ID: %d in %s", m_event_video_id,
         m_event_audio_id, m_name.c_str());

    if (!is_ready_for_event(*event)) {
      ERROR("AVQueue is not available for event in %s!", m_name.c_str());
      return state;
    }

    m_video_queue[m_event_video_id]->event_reset();
    m_audio_queue[m_event_audio_id]->event_reset();
    if (m_config.event_gsensor_enable) {
      m_gsensor_queue[m_event_gsensor_id]->event_reset();
    }
    AM_PTS event_pts = packet->get_pts();
    INFO("Event occurrence PTS: %lld in %s.", event_pts, m_name.c_str());

    AM_PTS video_start_pts = m_first_video[m_event_video_id].second;
    //AM_PTS audio_start_pts = m_first_audio[m_event_audio_id].second;

    if (event_pts > (video_start_pts +
                     event->h26x.history_duration * PTS_SCALE)) {
      video_start_pts = event_pts - event->h26x.history_duration * PTS_SCALE;
    }
    m_event_end_pts = event_pts + event->h26x.future_duration * PTS_SCALE;
    INFO("Event start PTS: %lld, end PTS: %lld in %s.",
         video_start_pts, m_event_end_pts, m_name.c_str());

    AM_PTS video_pts = 0;
    AM_PTS audio_pts = 0;
    AM_PTS gsensor_pts = 0;
    ExPayload *video_payload = nullptr;
    ExPayload *audio_payload = nullptr;
    ExPayload *gsensor_payload = nullptr;

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
      // Set gsensor ReadPos
      if (m_config.event_gsensor_enable) {
        while (!m_gsensor_queue[m_event_gsensor_id]->event_empty()) {
          gsensor_payload = m_gsensor_queue[m_event_gsensor_id]->event_get_prev();
          gsensor_pts = gsensor_payload->m_data.m_payload_pts;
          if (gsensor_pts < video_pts) {
            break;
          }
          m_gsensor_queue[m_event_gsensor_id]->event_backtrack();
        }
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
          ERROR("video type error in %s.", m_name.c_str());
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
    INFO("Event: current audio PTS: %lld, video PTS: %lld, "
        "g-sensor pts: %lld in %s ",
         audio_pts, video_pts, gsensor_pts, m_name.c_str());

    if (send_event_info() != AM_STATE_OK) {
      state = AM_STATE_ERROR;
      ERROR("Failed to send event information in %s!", m_name.c_str());
      return state;
    }
    m_in_event = true;
    m_event_stop = false;
    m_event_wait->signal();
  } while(0);
  return state;
}

AM_STATE AMAVQueue::on_event(AMPacket *packet)
{
  std::unique_lock<std::mutex> lk(m_mutex);
  AM_STATE state = AM_STATE_OK;
  do {
    AMEventStruct *event = (AMEventStruct*)(packet->get_data_ptr());
    switch (event->attr) {
      case AM_EVENT_MJPEG : {
        INFO("%s receive evnet pkt, event attr is AM_EVENT_MJPEG,"
            "stream id is %u, pre num is %u, after number is %u, "
            "closest num is %u", m_name.c_str(), event->mjpeg.stream_id,
            event->mjpeg.pre_cur_pts_num, event->mjpeg.after_cur_pts_num,
            event->mjpeg.closest_cur_pts_num);
      } break;
      case AM_EVENT_PERIODIC_MJPEG :{
        INFO("%s receive event packet, event attr is "
            "AM_EVENT_PERIODIC_MJPEG, stream id is %u, interval second is %u, "
            "start time is %u-%u-%u, end time is %u-%u-%u", m_name.c_str(),
            event->periodic_mjpeg.stream_id, event->periodic_mjpeg.interval_second,
            event->periodic_mjpeg.start_time_hour,
            event->periodic_mjpeg.start_time_minute,
            event->periodic_mjpeg.start_time_second,
            event->periodic_mjpeg.end_time_hour,
            event->periodic_mjpeg.end_time_minute,
            event->periodic_mjpeg.end_time_second);
      } break;
      case AM_EVENT_H26X : {
        INFO("%s receive event packet, event attr is AM_EVENT_H26X,"
            "stream id is %u, history duration is %u, future duration is %u",
            m_name.c_str(), event->h26x.stream_id, event->h26x.history_duration,
            event->h26x.future_duration);
        if (event->h26x.stream_id != m_config.video_id) {
          break;
        }
        state = process_h26x_event(packet);
      } break;
      case AM_EVENT_STOP_CMD : {
        if (event->stop_cmd.stream_id == m_config.video_id) {
          if (m_in_event) {
            INFO("%s is in event, receive stop cmd, stop it", m_name.c_str());
            m_event_stop = true;
          } else {
            ERROR("%s is not in event now, ignore the stop cmd.", m_name.c_str());
            break;
          }
        }
      } break;
      default : {
        ERROR("Event attr error in %s.", m_name.c_str());
        state = AM_STATE_ERROR;
      } break;
    }
  } while(0);
  if (packet) {
    packet->release();
  }
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
      if (m_config.video_id != event.h26x.stream_id) {
        NOTICE("The stream id[%u] of event is not same with event id[%u] in %s",
              event.h26x.stream_id, m_config.video_id, m_name.c_str());
        break;
      }
      m_event_video_id = m_config.video_id;
      if (m_first_video.find(m_event_video_id) == m_first_video.end()) {
        NOTICE("Can not find video[%u] in event buffer in %s.",
              m_event_video_id, m_name.c_str());
        break;
      }
      if (m_first_audio.find(m_event_audio_id) == m_first_audio.end()) {
        NOTICE("Can not find audio[%u] in event buffer in %s.",
              m_event_audio_id, m_name.c_str());
        break;
      }
      if (m_config.event_gsensor_enable) {
        if (m_first_gsensor.find(m_event_gsensor_id) == m_first_gsensor.end()) {
          NOTICE("Can not find gsensor[%u] in event buffer in %s.",
                 m_event_gsensor_id, m_name.c_str());
          break;
        }
      }
      ret = !m_in_event &&
          m_first_video[m_event_video_id].first &&
          m_first_audio[m_event_audio_id].first &&
          (!m_config.event_gsensor_enable || (m_config.event_gsensor_enable &&
            m_first_gsensor[m_event_gsensor_id].first)) &&
          m_config.event_enable;
      if (!ret) {
        if (m_in_event) {
          NOTICE("%s is in event", m_name.c_str());
        } else if (!m_first_video[m_event_video_id].first) {
          NOTICE("The video[%u] data is not received in %s.", m_event_video_id,
                m_name.c_str());
        } else if (!m_first_audio[m_event_audio_id].first) {
          NOTICE("The audio[%u] data is not receive in %s",m_event_audio_id,
                m_name.c_str());
        } else if (!m_config.event_enable) {
          NOTICE("Event is not enabled in %s", m_name.c_str());
        } if (!m_config.event_gsensor_enable || (m_config.event_gsensor_enable &&
            m_first_gsensor[m_event_gsensor_id].first)) {
          NOTICE("gsensor is not enabled or gsensor[%u] data is not received in %s",
                 m_event_gsensor_id, m_name.c_str());
        } else {
          NOTICE("ERROR in %s.", m_name.c_str());
        }
      }
    } else if (event.attr == AM_EVENT_STOP_CMD) {
      ret = (m_in_event && (event.stop_cmd.stream_id == m_config.video_id));
    } else {
      ERROR("Event attr error in %s.", m_name.c_str());
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMAVQueue::is_sending_normal_pkt()
{
  return m_is_sending_normal_pkt.load();
}

bool AMAVQueue::start_send_normal_pkt()
{
  bool ret = true;
  do {
    if (m_is_sending_normal_pkt.load()) {
      ERROR("Send normal pkt function has been started already in %s.",
            m_name.c_str());
      break;
    } else {
      m_is_sending_normal_pkt = true;
      m_send_normal_pkt_wait->signal();
    }
  } while(0);
  return ret;
}

AM_STATE AMAVQueue::on_eos(AMPacket* packet)
{
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
      INFO("Video[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
      INFO("Audio[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR:
      INFO("gsensor[%d] EOS in %s", packet->get_stream_id(), m_name.c_str());
      break;
    default:
      break;
  }
  return on_data(packet);
}

AMAVQueue::AMAVQueue()
{
}

AMAVQueue::~AMAVQueue()
{
  m_stop = true;
  m_send_cond.notify_one();
  m_info_pkt_process_event->signal();
  m_send_normal_pkt_wait->signal();
  m_event_wait->signal();
  m_event_send_cond.notify_one();
  AM_DESTROY(m_send_normal_pkt_thread);
  AM_DESTROY(m_send_event_pkt_thread);
  AM_DESTROY(m_send_normal_pkt_wait);
  AM_DESTROY(m_info_pkt_process_event);
  AM_DESTROY(m_packet_pool);
  AM_DESTROY(m_event_wait);
  for (auto &m : m_video_queue) {
    AM_DESTROY(m.second);
  }
  for (auto &m : m_gsensor_queue) {
    AM_DESTROY(m.second);
  }
  for (auto &m : m_audio_queue) {
    AM_DESTROY(m.second);
  }
  while(m_info_pkt_queue.size() > 0) {
    AMPacket *pkt = m_info_pkt_queue.front_and_pop();
    pkt->release();
  }
  INFO("%s is destroyed.", m_name.c_str());
}
