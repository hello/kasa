/*******************************************************************************
 * am_rtp_session_base.cpp
 *
 * History:
 *   2015-1-5 - [ypchang] created file
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

#include <sys/syscall.h>

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
 * RtpClientDataQ
 */
RtpClientDataQ::RtpClientDataQ()
{
}

RtpClientDataQ::~RtpClientDataQ()
{
  clear();
}

void RtpClientDataQ::clear()
{
  while (!data_q.empty()) {
    data_q.front_pop()->release();
  }
}

void RtpClientDataQ::push(AMRtpData *data)
{
  data_q.push(data);
}

AMRtpData* RtpClientDataQ::front_pop()
{
  return data_q.empty() ? nullptr : data_q.front_pop();
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
    bool need_add = true;
    if (AM_UNLIKELY(0 != m_client_data_map.count(ssrc))) {
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
      RtpClientDataQ *queue = new RtpClientDataQ();
      if (AM_LIKELY(queue)) {
        client->m_start_clock_90k = get_clock_count_90k();
        client->inc_ref();
        client->set_session(this);
        m_lock_client.lock();
        m_client_list.push_back(client);
        m_client_data_map[ssrc] = queue;
        m_lock_client.unlock();
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

void AMRtpSession::del_clients_all(bool notify)
{
  delete_all_clients(notify);
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

void AMRtpSession::put_back(AMRtpData *data)
{
  if (AM_LIKELY(data)) {
    if (m_config_rtp->del_buffer_on_release) {
      data->clear();
    }
    m_data_queue.push(data);
  }
}

uint64_t AMRtpSession::fake_ntp_time_us(uint64_t *clock_count_90k,
                                        uint64_t offset)
{
  uint64_t clock_90k = get_clock_count_90k();
  uint64_t sys_time_mono = get_sys_time_mono();
  if (AM_LIKELY(clock_count_90k)) {
    *clock_count_90k = clock_90k;
  }
  return (m_config_rtp->ntp_use_hw_timer ?
      (clock_90k * 1000000ULL / 90000ULL + NTP_OFFSET_US + offset) :
      (sys_time_mono + NTP_OFFSET_US + offset));
}

AMRtpData* AMRtpSession::alloc_data(int64_t pts)
{
  AMRtpData *data = nullptr;
  uint32_t count = 0;
  while(m_data_queue.empty() && (count < m_config_rtp->max_alloc_wait_count)) {
    ++ count;
    usleep(m_config_rtp->alloc_wait_interval * 1000);
  }
  if (AM_LIKELY(!m_data_queue.empty())) {
    data = m_data_queue.front_pop();
    data->add_ref();
    data->m_pts = pts;
  }

  return data;
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

bool AMRtpSession::is_client_ready(uint32_t ssrc)
{
  bool ret = (0 != m_client_data_map.count(ssrc));
  if (AM_UNLIKELY(!ret)) {
    NOTICE("No such client %08X, removed already?", ssrc);
  }
  return ret;
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
  AUTO_MEM_LOCK(m_lock_ts);
  return m_curr_time_stamp;
}

AMRtpSession::AMRtpSession(const std::string &name,
                           MuxerRtpConfig *config,
                           AM_SESSION_TYPE type) :
        m_prev_pts(0LL),
        m_data_buffer(nullptr),
        m_config_rtp(config),
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
  delete_all_clients(false);
  for (uint32_t i = 0; i < m_config_rtp->max_send_queue_size; ++ i) {
    m_data_queue.pop();
  }
  delete[] m_data_buffer;
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    close(m_hw_timer_fd);
  }
}

bool AMRtpSession::init()
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(!m_config_rtp)) {
      ERROR("Invalid RTP muxer config!");
      break;
    }
    if (AM_UNLIKELY((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0)) {
      ERROR("Failed to open %s: %s", HW_TIMER, strerror(errno));
      break;
    }
    m_data_buffer = new AMRtpData[m_config_rtp->max_send_queue_size];
    if (AM_UNLIKELY(!m_data_buffer)) {
      ERROR("Failed to allocate memory for AMRtpData[%u]!",
            m_config_rtp->max_send_queue_size);
      break;
    }
    for (uint32_t i = 0; i < m_config_rtp->max_send_queue_size; ++ i) {
      m_data_buffer[i].m_id = i;
      put_back(&m_data_buffer[i]);
    }
   ret = true;
  }while(0);

  return ret;
}

void AMRtpSession::add_rtp_data_to_client(AMRtpData *data)
{
  if (AM_LIKELY(data)) {
    AUTO_MEM_LOCK(m_lock_client);
    for (AMClientDataMap::iterator iter = m_client_data_map.begin();
         iter != m_client_data_map.end();) {
      AMRtpClient *client = find_client_by_ssrc(iter->first);
      if (AM_LIKELY(client && !client->is_abort() &&
                    client->is_alive() && client->is_enable() &&
                    (client->m_start_clock_90k <= (uint64_t)data->m_pts))) {
        if (AM_LIKELY(!client->is_new_client() ||
                      (data->is_key_frame() && client->is_new_client()))) {
          client->set_new_client(false);
          data->add_ref();
          iter->second->push(data);
        }
        ++ iter;
      } else {
#if 0
        if (AM_UNLIKELY(client->m_start_clock_90k > (uint64_t)data->pts)) {
          NOTICE("%s data with PTS %llu is older than client requested(%lld)! "
                 "Ignore...", type_string().c_str(),
                 data->pts, client->m_start_clock_90k);
        }
#endif
        iter->second->clear();
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

AMRtpData* AMRtpSession::get_rtp_data_by_ssrc(uint32_t ssrc)
{
  AMRtpData *data = nullptr;
  if (AM_LIKELY(m_client_data_map.count(ssrc))) {
    data = m_client_data_map.at(ssrc)->front_pop();
  }

  return data;
}

bool AMRtpSession::delete_client_by_ssrc(uint32_t ssrc)
{
  AUTO_MEM_LOCK(m_lock_client);
  bool ret = false;
  for (uint32_t i = 0; i < m_client_list.size(); ++ i) {
    AMRtpClient* &client = m_client_list[i];
    if (AM_LIKELY(client && (client->ssrc() == ssrc))) {
      AMClientDataMap::iterator iter = m_client_data_map.find(ssrc);
      client->destroy(true); /* Need to send notify to the caller */
      m_client_list.erase(m_client_list.begin() + i);
      if (AM_LIKELY(iter != m_client_data_map.end())) {
        iter->second->clear();
        delete iter->second;
        m_client_data_map.erase(iter);
        INFO("Deleted client with SSRC %08X from session %s",
             ssrc, m_name.c_str());
      }
      ret = true;
      break;
    }
  }
  return ret;
}

void AMRtpSession::delete_all_clients(bool send_notify)
{
  AUTO_MEM_LOCK(m_lock_client);
  for (uint32_t i = 0; i < m_client_list.size(); ++ i) {
    if (m_client_list[i]) {
      /* This is called in destructor, no need to send notify */
      m_client_list[i]->destroy(send_notify);
    }
  }
  m_client_list.clear();

  for (AMClientDataMap::iterator iter = m_client_data_map.begin();
       iter != m_client_data_map.end(); ++ iter) {
    iter->second->clear();
    delete iter->second;
  }
  m_client_data_map.clear();
}
