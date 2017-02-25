/*******************************************************************************
 * am_rtp_session_aac.cpp
 *
 * History:
 *   2015-1-7 - [ypchang] created file
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

#include "am_rtp_session_aac.h"
#include "am_rtp_data.h"
#include "am_muxer_rtp_config.h"
#include "am_audio_define.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_mutex.h"
#include "adts.h"
#include "rtp.h"

#define AU_HEADER_LEN     2
#define AU_HEADER_SIZE    2

#define AAC_SINGLE_AU     1

#define STRIP_ADTS_HEADER 0

extern std::string to_hex_string(uint32_t num);

uint32_t index_to_freq[] =
{
  96000,
  88200,
  64000,
  48000,
  44100,
  32000,
  24000,
  22050,
  16000,
  12000,
  11025,
  8000,
  7350,
  0,
  0,
  0
};

#ifdef __cplusplus
extern "C" {
#endif
AMIRtpSession* create_rtp_session(const char *name,
                                  MuxerRtpConfig *muxer_rtp_config,
                                  void *ainfo)
{
  return AMRtpSessionAac::create(name, muxer_rtp_config, (AM_AUDIO_INFO*)ainfo);
}
#ifdef __cplusplus
}
#endif

AMIRtpSession* AMRtpSessionAac::create(const std::string &name,
                                       MuxerRtpConfig *muxer_rtp_config,
                                       AM_AUDIO_INFO *ainfo)
{
  AMRtpSessionAac *result = new AMRtpSessionAac(name, muxer_rtp_config);
  if (AM_UNLIKELY(result && !result->init(ainfo))) {
    delete result;
    result = nullptr;
  }

  return ((AMIRtpSession*)result);
}

uint32_t AMRtpSessionAac::get_session_clock()
{
  AUTO_MEM_LOCK(m_lock_sample_rate);
  return m_aac_sample_rate == 0 ? m_audio_info->sample_rate : m_aac_sample_rate;
}

AM_SESSION_STATE AMRtpSessionAac::process_data(AMPacket &packet,
                                   uint32_t max_tcp_payload_size)
{
  AM_SESSION_STATE state = AM_SESSION_OK;

  int64_t curr_pts = packet.get_pts();
  AMRtpData *rtp_data = alloc_data(curr_pts);
  if (AM_LIKELY(rtp_data)) {
    uint32_t frame_count = packet.get_frame_count();
#if AAC_SINGLE_AU
    uint32_t total_data_size = packet.get_data_size() +
        frame_count * (AU_HEADER_LEN + AU_HEADER_SIZE +
                       sizeof(AMRtpHeader) + sizeof(AMRtpTcpHeader));
#else
    uint32_t total_data_size = packet.get_data_size() + AU_HEADER_LEN +
        frame_count * AU_HEADER_SIZE + sizeof(AMRtpHeader) +
        sizeof(AMRtpTcpHeader);
#endif
#if AAC_SINGLE_AU
    if (AM_LIKELY(rtp_data->create(this, total_data_size,
                                   packet.get_data_size(),
                                   frame_count))) {
#else
    if (AM_LIKELY(rtp_data->create(this, total_data_size,
                                   packet.get_data_size(),
                                   1))) {
#endif
      int64_t pts_diff = ((m_config_rtp &&
                           m_config_rtp->dynamic_audio_pts_diff) &&
                          (m_prev_pts > 0)) ?
                   (curr_pts - m_prev_pts) : m_audio_info->pkt_pts_increment;
#if AAC_SINGLE_AU
      int32_t pkt_pts_incr = (int32_t)
          ((2 * pts_diff * get_session_clock() + 90000LL) / (2 * 90000LL) /
              frame_count);
#else
      int32_t pkt_pts_incr = (int32_t)
          ((2 * pts_diff * get_session_clock() + 90000LL) / (2 * 90000LL));
#endif

      DEBUG("PTS diff in 90K is %lld, PTS diff in %uK is %d",
            pts_diff, get_session_clock() / 1000, pkt_pts_incr);

      assemble_packet(packet.get_data_ptr(),
                      packet.get_data_size(),
                      frame_count,
                      rtp_data,
                      pkt_pts_incr);
      add_rtp_data_to_client(rtp_data);
    }
    rtp_data->release();
  } else {
    state = AM_SESSION_IO_SLOW;
    ERROR("Network too slow? Not enough data buffer! Drop data!");
  }
  m_prev_pts = curr_pts;

  return state;
}

AMRtpSessionAac::AMRtpSessionAac(const std::string &name,
                                 MuxerRtpConfig *muxer_rtp_config) :
    inherited(name, muxer_rtp_config, AM_SESSION_AAC),
    m_audio_info(nullptr),
    m_aac_config(0),
    m_aac_sample_rate(0)
{
}

AMRtpSessionAac::~AMRtpSessionAac()
{
  delete m_audio_info;
}

bool AMRtpSessionAac::init(AM_AUDIO_INFO *ainfo)
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(!inherited::init())) {
      ERROR("Failed to init AMRtpSession!");
      break;
    }

    m_audio_info = new AM_AUDIO_INFO();
    if (AM_UNLIKELY(!m_audio_info)) {
      ERROR("Failed to allocate memory for audio info!");
      break;
    }
    memcpy(m_audio_info, ainfo, sizeof(*m_audio_info));
    ret = true;
  }while(0);

  return ret;
}

void AMRtpSessionAac::generate_sdp(std::string& host_ip_addr, uint16_t port,
                                   AM_IP_DOMAIN domain)
{
  m_sdp.clear();
  m_sdp = "m=audio " + std::to_string(port) + " RTP/AVP " +
      get_payload_type() + "\r\n" +
      "c=IN " + ((domain == AM_IPV4)?"IP4 ":"IP6 ") +
      " " + host_ip_addr + "\r\n" + get_param_sdp() +
      "a=control:" + "rtsp://" + host_ip_addr + "/audio=" + m_name + "\r\n";
}

std::string AMRtpSessionAac::get_param_sdp()
{
  return std::string("a=rtpmap:" + std::to_string(RTP_PAYLOAD_TYPE_AAC) +
      " mpeg4-generic/" + std::to_string(get_session_clock()) + "/" +
      std::to_string(m_audio_info->channels) + "\r\n" +
      "a=fmtp:" + std::to_string(RTP_PAYLOAD_TYPE_AAC) +
      " streamtype=5; profile-level-id=41; mode=AAC-hbr; config=" +
      to_hex_string(aac_config()) +
      "; SizeLength=13; IndexLength=3; IndexDeltaLength=3; Profile=" +
      std::to_string(((aac_config() >> 11) - 1)) + ";\r\n" );
}

std::string AMRtpSessionAac::get_payload_type()
{
  return std::to_string(RTP_PAYLOAD_TYPE_AAC);
}

uint32_t AMRtpSessionAac::aac_config()
{
  while(0 == m_aac_config) {usleep(10000);}
  AUTO_MEM_LOCK(m_lock);
  return m_aac_config;
}

#if AAC_SINGLE_AU
void AMRtpSessionAac::assemble_packet(uint8_t *data,
                                      uint32_t data_size,
                                      uint32_t frame_count,
                                      AMRtpData *rtp_data,
                                      int32_t pts_incr)
{
  uint8_t *raw = data;
  uint8_t *buf = rtp_data->get_buffer();
  AMRtpPacket *packet = rtp_data->get_packet();
  uint32_t count = 0;

  while (count < frame_count) {
    AdtsHeader *adts = (AdtsHeader*)raw;
    uint8_t *tcp_header = buf;
    uint8_t *rtp_header = tcp_header + sizeof(AMRtpTcpHeader);
    uint8_t *au_header_len = rtp_header + sizeof(AMRtpHeader);
    uint8_t *au_header = au_header_len + AU_HEADER_LEN;
    uint8_t *au_data = au_header + AU_HEADER_SIZE;
    uint16_t frame_len = adts->frame_length();
#if STRIP_ADTS_HEADER
    uint16_t data_len = frame_len - sizeof(AdtsHeader);
    uint8_t *data_ptr = raw + sizeof(AdtsHeader);
#else
    uint16_t data_len = frame_len;
    uint8_t *data_ptr = raw;
#endif
    uint32_t rtp_tcp_payload_size = AU_HEADER_LEN + AU_HEADER_SIZE +
        sizeof(AMRtpHeader) + data_len;

    if (AM_UNLIKELY(0 == m_aac_config)) {
      AUTO_MEM_LOCK(m_lock);
      uint8_t config0 = (adts->aac_audio_object_type() << 3) |
          (adts->aac_frequency_index() >> 1);
      uint8_t config1 = (adts->aac_frequency_index() << 7) |
          (adts->aac_channel_conf() << 3);
      m_aac_config = ((config0 << 8) | config1);
    }
    m_lock_sample_rate.lock();
    m_aac_sample_rate = index_to_freq[adts->aac_frequency_index()];
    m_lock_sample_rate.unlock();

    ++ m_sequence_num;

    /* Build RTP TCP Header */
    tcp_header[0] = 0x24;
    /* tcp_header[1] should be set to client RTP channel ID */
    tcp_header[2] = (rtp_tcp_payload_size & 0xff00) >> 8;
    tcp_header[3] = rtp_tcp_payload_size & 0x00ff;

    /* Build RTP Header */
    rtp_header[0] = 0x80;
    rtp_header[1] = 0x80 | (RTP_PAYLOAD_TYPE_AAC & 0x7F);
    rtp_header[2] = (m_sequence_num & 0xff00) >> 8;
    rtp_header[3] = m_sequence_num & 0x00ff;
    rtp_header[4] = (m_curr_time_stamp & 0xff000000) >> 24;
    rtp_header[5] = (m_curr_time_stamp & 0x00ff0000) >> 16;
    rtp_header[6] = (m_curr_time_stamp & 0x0000ff00) >> 8;
    rtp_header[7] = (m_curr_time_stamp & 0x000000ff);

    AMRtpSession::m_lock_ts.lock();
    m_curr_time_stamp += pts_incr;
    AMRtpSession::m_lock_ts.unlock();

    rtp_header[8]  = 0; // SSRC[3] filled by Client when sending this packet
    rtp_header[9]  = 0; // SSRC[2]
    rtp_header[10] = 0; // SSRC[1]
    rtp_header[11] = 0; // SSRC[0]
    au_header_len[0] = ((AU_HEADER_SIZE * 8) >> 8) & 0xff;
    au_header_len[1] = (AU_HEADER_SIZE * 8) & 0xff;
    au_header[0] = (data_len >> 5) & 0xff;
    au_header[1] = (data_len & 0x1f) << 3;
    memcpy(au_data, data_ptr, data_len);
    packet[count].tcp_data = tcp_header;
    packet[count].udp_data = rtp_header;
    packet[count].total_size = rtp_tcp_payload_size +
                                         sizeof(AMRtpTcpHeader);
    raw += frame_len;
    buf += packet[count].total_size;
    ++ count;
  }
}

#else
void AMRtpSessionAac::assemble_packet(uint8_t *data,
                                      uint32_t data_size,
                                      uint32_t frame_count,
                                      AMRtpData *rtp_data,
                                      int32_t    pts_incr)
{
  AdtsHeader *adts = (AdtsHeader*)data;
  AMRtpPacket *packet = rtp_data->get_packet();
  uint8_t *buf = rtp_data->get_buffer();
  uint8_t *tcp_header = &buf[0];
  uint8_t *rtp_header = &buf[sizeof(AMRtpTcpHeader)];
  uint8_t *au_header_len = rtp_header + sizeof(AMRtpHeader);
  uint8_t *au_header = au_header_len + AU_HEADER_LEN;
  uint8_t *payload_addr = au_header + AU_HEADER_SIZE * frame_count;
  uint32_t rtp_tcp_payload_size = AU_HEADER_LEN + frame_count * AU_HEADER_SIZE +
      sizeof(AMRtpHeader);

  if (AM_UNLIKELY(0 == m_aac_config)) {
    AUTO_MEM_LOCK(m_lock);
    uint8_t config0 = (adts->aac_audio_object_type() << 3) |
        (adts->aac_frequency_index() >> 1);
    uint8_t config1 = (adts->aac_frequency_index() << 7) |
        (adts->aac_channel_conf() << 3);
    m_aac_config = ((config0 << 8) | config1);
  }
  m_lock_sample_rate.lock();
  m_aac_sample_rate = index_to_freq[adts->aac_frequency_index()];
  m_lock_sample_rate.unlock();

  ++ m_sequence_num;

  /* Build RTP TCP Header */
  tcp_header[0] = 0x24;
  /* tcp_header[1] should be set to client RTP channel ID */

  /* Build RTP Header */
  rtp_header[0] = 0x80;
  rtp_header[1] = 0x80 | (RTP_PAYLOAD_TYPE_AAC & 0x7f);
  rtp_header[2] = (m_sequence_num & 0xff00) >> 8;
  rtp_header[3] = m_sequence_num & 0x00ff;
  rtp_header[4] = (m_curr_time_stamp & 0xff000000) >> 24;
  rtp_header[5] = (m_curr_time_stamp & 0x00ff0000) >> 16;
  rtp_header[6] = (m_curr_time_stamp & 0x0000ff00) >> 8;
  rtp_header[7] = (m_curr_time_stamp & 0x000000ff);
  rtp_header[8]  = 0; // SSRC[3] filled by Client when sending this packet
  rtp_header[9]  = 0; // SSRC[2]
  rtp_header[10] = 0; // SSRC[1]
  rtp_header[11] = 0; // SSRC[0]
  au_header_len[0] = (((AU_HEADER_SIZE * frame_count) * 8) >> 8) & 0xff;
  au_header_len[1] = ((AU_HEADER_SIZE * frame_count) * 8) & 0xff;

  AMRtpSession::m_lock_ts.lock();
  m_curr_time_stamp += pts_incr;
  AMRtpSession::m_lock_ts.unlock();

  for (uint32_t i = 0; i < frame_count; ++ i) {
    uint16_t frame_len = adts->frame_length();

    au_header[0] = (frame_len >> 5) & 0xff;
    au_header[1] = (frame_len & 0x1f) << 3;
    memcpy(payload_addr, adts, frame_len);
    adts = (AdtsHeader*)((uint8_t*)adts + frame_len);
    au_header += AU_HEADER_SIZE;
    payload_addr += frame_len;
    rtp_tcp_payload_size += frame_len;
  }
  tcp_header[2] = (rtp_tcp_payload_size & 0xff00) >> 8;
  tcp_header[3] = rtp_tcp_payload_size & 0x00ff;
  packet[0].tcp_data = buf;
  packet[0].udp_data = rtp_header;
  packet[0].total_size = rtp_tcp_payload_size + sizeof(AMRtpTcpHeader);
}
#endif
