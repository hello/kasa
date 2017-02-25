/*******************************************************************************
 * am_rtp_data.cpp
 *
 * History:
 *   2015-1-6 - [ypchang] created file
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

#include "am_rtp_data.h"
#include "am_rtp_session_if.h"

AMRtpData::AMRtpData()
{}

AMRtpData::~AMRtpData()
{
  clear();
}

void AMRtpData::clear()
{
  AUTO_MEM_LOCK(m_lock);
  delete[] m_buffer;
  delete[] m_packet;
  m_buffer = nullptr;
  m_packet = nullptr;
  m_owner  = nullptr;
  m_pts = 0LL;
  m_pkt_size = 0;
  m_pkt_num = 0;
  m_buffer_size = 0;
  m_ref_count = 0;
}

bool AMRtpData::create(AMIRtpSession *session,
                       uint32_t datasize,
                       uint32_t payloadsize,
                       uint32_t packet_num)
{
  AUTO_MEM_LOCK(m_lock);
  m_owner = session;
  m_payload_size = payloadsize;
  if (AM_LIKELY(datasize > 0)) {
    if (AM_UNLIKELY(datasize > m_buffer_size)) {
      delete[] m_buffer;
      m_buffer = nullptr;
      if (AM_LIKELY(m_buffer_size)) {
        INFO("ID: %02u, Prev buffer size: %u, current buffer size: %u",
             m_id, m_buffer_size, datasize);
      }
      m_buffer_size = ROUND_UP((5 * datasize / 4), 4);
    }
    if (AM_LIKELY(!m_buffer)) {
      m_buffer = new uint8_t[m_buffer_size];
    }
  }
  if (AM_LIKELY(packet_num > 0)) {
    m_pkt_num = packet_num;
    if (AM_UNLIKELY(m_pkt_num > m_pkt_size)) {
      delete[] m_packet;
      m_packet = nullptr;
      if (AM_LIKELY(m_pkt_size)) {
        INFO("ID: %02u, Prev packet num: %u, current packet num: %u",
             m_id, m_pkt_size, packet_num);
      }
      m_pkt_size = 5 * m_pkt_num / 4;
    }
    if (AM_LIKELY(!m_packet)) {
      m_packet = new AMRtpPacket[m_pkt_size];
    }
  }
  return (m_buffer && m_packet);
}

void AMRtpData::add_ref()
{
  AUTO_MEM_LOCK(m_ref_lock);
  ++ m_ref_count;
}

void AMRtpData::release()
{
  AUTO_MEM_LOCK(m_ref_lock);
  if (--m_ref_count == 0) {
    m_owner->put_back(this);
    m_ref_count = 0;
  } else if (m_ref_count < 0) {
    m_ref_count = 0;
    ERROR("BUG!!! %p is already released!!!", this);
  }
}

int64_t AMRtpData::get_pts()
{
  return m_pts;
}

uint8_t* AMRtpData::get_buffer()
{
  return m_buffer;
}

AMRtpPacket* AMRtpData::get_packet()
{
  return m_packet;
}

uint32_t AMRtpData::get_packet_number()
{
  return m_pkt_num;
}

uint32_t AMRtpData::get_payload_size()
{
  return m_payload_size;
}

bool AMRtpData::is_key_frame()
{
  return m_is_key_frame.load();
}

void AMRtpData::set_key_frame(bool key)
{
  m_is_key_frame = key;
}
