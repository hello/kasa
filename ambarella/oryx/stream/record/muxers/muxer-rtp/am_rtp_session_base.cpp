/*******************************************************************************
 * am_rtp_session_base.cpp
 *
 * History:
 *   2015-1-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_rtp_session_base.h"
#include "am_rtp_client.h"
#include "am_rtp_data.h"
#include "am_muxer_rtp_config.h"

#include "am_mutex.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <iterator>

#define NTP_OFFSET    2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)
#define HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")

extern uint32_t random_number();

std::string to_hex_string(uint32_t num)
{
  char hexnum[32] = {0};
  sprintf(hexnum, "%X", num);
  return hexnum;
}

/*
 * AMRtpSessionBase
 */
std::string& AMRtpSession::get_session_name()
{
  return m_name;
}

std::string& AMRtpSession::get_sdp(std::string& host_ip_addr, uint16_t port,
                                   AM_IP_DOMAIN domain)
{
  generate_sdp(host_ip_addr, port, domain);
  return m_sdp;
}

bool AMRtpSession::add_client(AMRtpClient *client)
{
  bool ret = false;
  if (AM_LIKELY(client && ((uint32_t)-1) != client->ssrc())) {
    uint32_t ssrc = client->ssrc();
    AMClientDataMap::iterator cli = m_client_data_map.find(ssrc);
    bool need_add = true;
    if (AM_UNLIKELY(cli != m_client_data_map.end())) {
      AMRtpClient *added = find_client_by_ssrc(ssrc);
      if (AM_LIKELY(added == client)) {
        WARN("Client with SSRC %08X is already added!", ssrc);
        ret = true;
        need_add = false;
      } else {
        WARN("New client with the same SSRC(%08X) is added, delete old one!",
             ssrc);
        delete_client_by_ssrc(ssrc);
      }
    }
    if (AM_LIKELY(need_add)) {
      AUTO_SPIN_LOCK(m_lock_client);
      AMRtpDataQ *queue = new AMRtpDataQ();
      if (AM_LIKELY(queue)) {
        client->m_start_clock_90k = get_clock_count_90k();
        client->inc_ref();
        client->set_session(this);
        m_client_list.push_back(client);
        m_client_data_map[ssrc] = queue;
        ret = client->start();
        NOTICE("RTP session %s added client with SSRC %08X!",
               m_name.c_str(), ssrc);
      } else {
        ERROR("Failed to allocate data queue for client %08X!", ssrc);
      }
    }
  } else {
    ERROR("Invalid client %p, SSRC: %08X!",
          client, client ? client->ssrc() : (uint32_t)-1);
  }

  return ret;
}

bool AMRtpSession::del_client(uint32_t ssrc)
{
  return delete_client_by_ssrc(ssrc);
}

void AMRtpSession::set_client_enable(uint32_t ssrc, bool enabled)
{
  AMRtpClient *client = find_client_by_ssrc(ssrc);
  if (AM_LIKELY(client)) {
    client->set_enable(enabled);
  }
}

std::string AMRtpSession::get_client_seq_ntp(uint32_t ssrc)
{
  std::string seq_ntp;
  AMRtpClient *client = find_client_by_ssrc(ssrc);
  if (AM_LIKELY(client)) {
    seq_ntp = "seq=" + std::to_string(m_sequence_num) +
        ";rtptime=" + std::to_string(m_curr_time_stamp);
  } else {
    seq_ntp.clear();
  }

  return seq_ntp;
}

AM_SESSION_STATE AMRtpSession::process_data(AMPacket &packet,
                                            uint32_t max_packet_size)
{
  ERROR("Use inherited method!");
  return AM_SESSION_ERROR;
}

AM_SESSION_TYPE AMRtpSession::type()
{
  return m_session_type;
}

std::string AMRtpSession::type_string()
{
  std::string type;
  switch(m_session_type) {
    case AM_SESSION_AAC:   type = "AAC";   break;
    case AM_SESSION_OPUS:  type = "OPUS";  break;
    case AM_SESSION_SPEEX: type = "SPEEX"; break;
    case AM_SESSION_G726:  type = "G.726"; break;
    case AM_SESSION_G711:  type = "G.711"; break;
    case AM_SESSION_H264:  type = "H.264"; break;
    case AM_SESSION_H265:  type = "H.265"; break;
    case AM_SESSION_MJPEG: type = "MJPEG"; break;
    default: type = "Unknown"; break;
  }

  return type;
}

AMRtpClient* AMRtpSession::find_client_by_ssrc(uint32_t ssrc)
{
  AMRtpClient *client = nullptr;
  for (uint32_t i = 0; i < m_client_list.size(); ++ i) {
    if (AM_LIKELY(m_client_list[i]->ssrc() == ssrc)) {
      client = m_client_list[i];
      break;
    }
  }
  return client;
}

void AMRtpSession::set_config(void *config)
{
  m_config_rtp = (MuxerRtpConfig*)config;
}

AMRtpData* AMRtpSession::alloc_data(int64_t pts)
{
  AMRtpData *data = m_data_queue.empty() ? nullptr : m_data_queue.front();
  if (AM_LIKELY(data)) {
    m_data_queue.pop();
    data->add_ref();
    data->pts = pts;
  }

  return data;
}

void AMRtpSession::put_back(AMRtpData *data)
{
  if (AM_LIKELY(data)) {
    m_data_queue.push(data);
  }
}

uint64_t AMRtpSession::fake_ntp_time_us(uint64_t offset)
{
  return (m_config_rtp->ntp_use_hw_timer ?
      (get_clock_count_90k() * 1000000ULL / 90000ULL + NTP_OFFSET_US + offset) :
      (get_sys_time_mono() + NTP_OFFSET_US + offset));
}

uint64_t AMRtpSession::get_clock_count_90k()
{
  uint8_t pts[32] = {0};
  uint64_t count_90k = 0;
  if (AM_UNLIKELY(read(m_hw_timer_fd, pts, sizeof(pts)) < 0)) {
    PERROR("read");
  } else {
    count_90k = (uint64_t)strtoull((const char*)pts, (char**)NULL, 10);
  }

  return count_90k;
}

uint64_t AMRtpSession::get_sys_time_mono()
{
  timespec tm;
  uint64_t mono = 0;
  if (AM_UNLIKELY(clock_gettime(CLOCK_MONOTONIC_RAW, &tm) < 0)) {
    PERROR("clock_gettime CLOCK_MONOTONIC_RAW");
  } else {
    mono = tm.tv_sec * 1000000ULL + tm.tv_nsec / 1000ULL;
  }

  return mono;
}

uint32_t AMRtpSession::current_rtp_timestamp()
{
  AUTO_SPIN_LOCK(m_lock_ts);
  return m_curr_time_stamp;
}

AMRtpSession::AMRtpSession(const std::string &name,
                           uint32_t queue_size,
                           AM_SESSION_TYPE type) :
  m_lock_ts(nullptr),
  m_lock_client(nullptr),
  m_data_buffer(nullptr),
  m_config_rtp(nullptr),
  m_prev_pts(0LL),
  m_queue_size(queue_size),
  m_curr_time_stamp(random_number()),
  m_hw_timer_fd(-1),
  m_session_type(type),
  m_sequence_num(0),
  m_name(name)
{
  m_sdp.clear();
}

AMRtpSession::~AMRtpSession()
{
  delete_all_clients();
  delete[] m_data_buffer;
  for (uint32_t i = 0; i < m_queue_size; ++ i) {
    m_data_queue.pop();
  }
  if (AM_LIKELY(m_lock_ts)) {
    m_lock_ts->destroy();
  }
  if (AM_LIKELY(m_lock_client)) {
    m_lock_client->destroy();
  }
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    close(m_hw_timer_fd);
  }
}

bool AMRtpSession::init()
{
  bool ret = false;
  do {
    if (AM_UNLIKELY((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0)) {
      ERROR("Failed to open %s: %s", HW_TIMER, strerror(errno));
      break;
    }
    m_lock_ts = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock_ts)) {
      ERROR("Failed to create lock for time stamp locking!");
      break;
    }
    m_lock_client = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock_client)) {
      ERROR("Failed t create lock for client locking!");
      break;
    }
    m_data_buffer = new AMRtpData[m_queue_size];
    if (AM_UNLIKELY(!m_data_buffer)) {
      ERROR("Failed to allocate memory for AMRtpData[%u]!", m_queue_size);
      break;
    }
    for (uint32_t i = 0; i < m_queue_size; ++ i) {
      m_data_buffer[i].id = i;
      put_back(&m_data_buffer[i]);
    }
   ret = true;
  }while(0);

  return ret;
}

void AMRtpSession::add_rtp_data_to_client(AMRtpData *data)
{
  if (AM_LIKELY(data)) {
    AUTO_SPIN_LOCK(m_lock_client);
    for (AMClientDataMap::iterator iter = m_client_data_map.begin();
         iter != m_client_data_map.end();) {
      AMRtpClient *client = find_client_by_ssrc(iter->first);
      if (AM_LIKELY(client && !client->is_abort() &&
                    client->is_alive() && client->is_enable() &&
                    (client->m_start_clock_90k <= (uint64_t)data->pts))) {
        data->add_ref();
        iter->second->push(data);
        ++ iter;
      } else {
#if 0
        if (AM_UNLIKELY(client->m_start_clock_90k > (uint64_t)data->pts)) {
          NOTICE("%s data with PTS %llu is older than client requested(%lld)! "
                 "Ignore...", type_string().c_str(),
                 data->pts, client->m_start_clock_90k);
        }
#endif
        empty_data_queue(*iter->second);
        if (AM_LIKELY(!client)) {
          NOTICE("Client %08X is probably deleted!", iter->first);
          delete iter->second;
          iter = m_client_data_map.erase(iter);
        } else {
          ++ iter;
        }
      }
    }
  }
}

AMRtpDataQ* AMRtpSession::get_data_queue_by_ssrc(uint32_t ssrc)
{
  AMRtpDataQ *dataq = NULL;
  AMClientDataMap::iterator iter = m_client_data_map.find(ssrc);
  if (AM_LIKELY(iter != m_client_data_map.end())) {
    dataq = iter->second;
  } else {
    NOTICE("No such client %08X, removed already?", ssrc);
  }
  return dataq;
}

AMRtpData* AMRtpSession::get_rtp_data_by_ssrc(uint32_t ssrc)
{
  AUTO_SPIN_LOCK(m_lock_client);
  AMRtpData *data = nullptr;
  AMRtpDataQ *data_q = get_data_queue_by_ssrc(ssrc);
  if (AM_LIKELY(data_q && !data_q->empty())) {
    data = data_q->front();
    data_q->pop();
  }

  return data;
}

bool AMRtpSession::delete_client_by_ssrc(uint32_t ssrc)
{
  AUTO_SPIN_LOCK(m_lock_client);
  bool ret = false;
  for (uint32_t i = 0; i < m_client_list.size(); ++ i) {
    AMRtpClient* &client = m_client_list[i];
    if (AM_LIKELY(client && (client->ssrc() == ssrc))) {
      AMClientDataMap::iterator iter = m_client_data_map.find(ssrc);
      client->destroy();
      m_client_list.erase(m_client_list.begin() + i);
      if (AM_LIKELY(iter != m_client_data_map.end())) {
        INFO("Deleting client with SSRC %08X from session %s",
             ssrc, m_name.c_str());
        empty_data_queue(*iter->second);
        delete iter->second;
        m_client_data_map.erase(iter);
      }
      ret = true;
      break;
    }
  }
  return ret;
}

void AMRtpSession::delete_all_clients()
{
  AUTO_SPIN_LOCK(m_lock_client);
  for (uint32_t i = 0; i < m_client_list.size(); ++ i) {
    if (m_client_list[i]) {
      m_client_list[i]->destroy();
    }
  }
  m_client_list.clear();

  for (AMClientDataMap::iterator iter = m_client_data_map.begin();
       iter != m_client_data_map.end(); ++ iter) {
    empty_data_queue(*iter->second);
    delete iter->second;
  }
  m_client_data_map.clear();
}

void AMRtpSession::empty_data_queue(AMRtpDataQ& queue)
{
  while (!queue.empty()) {
    queue.front()->release();
    queue.pop();
  }
}
