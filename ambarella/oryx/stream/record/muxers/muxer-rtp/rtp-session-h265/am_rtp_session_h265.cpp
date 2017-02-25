/*******************************************************************************
 * am_rtp_session_h265.cpp
 *
 * History:
 *   2015-3-18 - [ypchang] created file
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

#include "am_amf_types.h"
#include "am_amf_packet.h"
#include "am_video_types.h"

#include "h265.h"
#include "rtp.h"

#include "am_rtp_session_h265.h"
#include "am_rtp_data.h"

#include "am_mutex.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
AMIRtpSession* create_rtp_session(const char *name,
                                  MuxerRtpConfig *muxer_rtp_config,
                                  void *vinfo)
{
  return AMRtpSessionH265::create(name,
                                  muxer_rtp_config,
                                  (AM_VIDEO_INFO*)vinfo);
}
#ifdef __cplusplus
}
#endif

AMIRtpSession* AMRtpSessionH265::create(const std::string &name,
                                        MuxerRtpConfig *muxer_rtp_config,
                                        AM_VIDEO_INFO *vinfo)
{
  AMRtpSessionH265 *result = new AMRtpSessionH265(name, muxer_rtp_config);
  if (AM_UNLIKELY(result && !result->init(vinfo))) {
    delete result;
    result = nullptr;
  }

  return ((AMIRtpSession*)result);
}

uint32_t AMRtpSessionH265::get_session_clock()
{
  return 90000;
}

AM_SESSION_STATE AMRtpSessionH265::process_data(AMPacket &packet,
                                                uint32_t max_tcp_payload_size)
{
  AM_SESSION_STATE state = AM_SESSION_OK;
  int64_t curr_pts = packet.get_pts();

  if (AM_LIKELY(find_nalu(packet.get_data_ptr(),  packet.get_data_size()))) {
    AMRtpData *rtp_data = alloc_data(curr_pts);

    if (AM_LIKELY(rtp_data)) {
      uint32_t pkt_num = 0;
      uint32_t max_packet_size = max_tcp_payload_size -
          sizeof(AMRtpTcpHeader) - sizeof(AMRtpHeader);
      uint32_t total_data_size = 0;
      uint32_t tile_id = packet.get_frame_count() & 0x000000ff;
      uint32_t slice_id = (packet.get_frame_count() & 0x0000ff00) >> 16;
      rtp_data->set_key_frame(
          (packet.get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR) &&
          (tile_id == 0) &&(slice_id == 0));
      for (uint32_t i = 0; i < m_nalu_list.size(); ++ i) {
        pkt_num += calc_h265_packet_num(m_nalu_list[i], max_packet_size);
      }
      total_data_size = pkt_num * max_tcp_payload_size;
      if (AM_LIKELY(rtp_data->create(this, total_data_size,
                                     packet.get_data_size(), pkt_num))) {
        int32_t pkt_pts_incr_90k = (m_prev_pts > 0) ?
            (int32_t)(curr_pts - m_prev_pts) : 0;

        DEBUG("FrameNum: %u, Preview PTS: %lld, Current PTS: %lld, "
              "PTS diff in 90K is %d",
              packet.get_frame_number(),
              m_prev_pts,
              curr_pts,
              pkt_pts_incr_90k);
        assemble_packet(rtp_data, pkt_pts_incr_90k,
                        max_tcp_payload_size,
                        max_packet_size);
        add_rtp_data_to_client(rtp_data);
      }
      rtp_data->release();
    } else {
      state = AM_SESSION_IO_SLOW;
      ERROR("Network too slow? Not enough data buffer! Drop data!");
    }
  } else {
    state = AM_SESSION_IO_SLOW;
    WARN("No NALUs are found! Corrupted bit stream? "
          "FrameNumber: %u, PTS: %u, Last PTS: %u, "
          "Address: %p, size: %u",
          packet.get_frame_number(), curr_pts, m_prev_pts,
          packet.get_data_ptr(), packet.get_data_size());
  }
  m_prev_pts = curr_pts;

  return state;
}

AMRtpSessionH265::AMRtpSessionH265(const std::string &name,
                                   MuxerRtpConfig *muxer_rtp_config) :
    inherited(name, muxer_rtp_config, AM_SESSION_H265),
    m_video_info(nullptr),
    m_profile_space(0),
    m_profile_id(0),
    m_tier_flag(0),
    m_level_id(0)
{
  m_nalu_list.clear();
  m_sps.clear();
  m_pps.clear();
  m_vps.clear();
  m_profile_compatibility_indicator.clear();
  m_interop_constraints.clear();
  memset(m_profile_tier_level, 0, sizeof(m_profile_tier_level));
}

AMRtpSessionH265::~AMRtpSessionH265()
{
  m_nalu_list.clear();
  delete m_video_info;
}

bool AMRtpSessionH265::init(AM_VIDEO_INFO *vinfo)
{
  bool ret = false;

  do {
    if (AM_UNLIKELY(!inherited::init())) {
      ERROR("Failed to init AMRtpSession!");
      break;
    }

    m_video_info = new AM_VIDEO_INFO();
    if (AM_UNLIKELY(!m_video_info)) {
      ERROR("Failed to allocate memory for video info!");
      break;
    }
    memcpy(m_video_info, vinfo, sizeof(*m_video_info));
    ret = true;
  }while(0);
  return ret;
}

void AMRtpSessionH265::generate_sdp(std::string& host_ip_addr, uint16_t port,
                                    AM_IP_DOMAIN domain)
{
  m_sdp.clear();
  m_sdp = "m=video " + std::to_string(port) + " RTP/AVP " +
      get_payload_type() + "\r\n" +
      "c=IN " + ((domain == AM_IPV4)?"IP4 ":"IP6 ") + host_ip_addr + "\r\n" +
      "a=sendonly\r\n" + get_param_sdp() +
      "a=cliprect:0,0," + std::to_string(m_video_info->height) + "," +
      std::to_string(m_video_info->width) + "\r\n" +
      "a=control:" + "rtsp://" + host_ip_addr + "/video=" + m_name + "\r\n";
}

std::string AMRtpSessionH265::get_param_sdp()
{
  while(m_vps.empty()){usleep(10000);}
  return std::string("a=rtpmap:" + std::to_string(RTP_PAYLOAD_TYPE_H265) +
                     " H265/90000" + "\r\n" +
                     "a=fmtp:" + std::to_string(RTP_PAYLOAD_TYPE_H265) +
                     " profile-space=" + std::to_string(m_profile_space) +
                     "; profile-id=" + std::to_string(m_profile_id) +
                     "; tier-flag=" + std::to_string(m_tier_flag) +
                     "; level-id=" + std::to_string(m_level_id) +
                     "; profile-compatibility-indicator=" +
                     m_profile_compatibility_indicator +
                     "; interop-constraints=" + m_interop_constraints +
                     "; sprop-vps=" + vps() +
                     "; sprop-sps=" + sps() +
                     "; sprop-pps=" + pps() + ";\r\n");
}

std::string AMRtpSessionH265::get_payload_type()
{
  return std::to_string(RTP_PAYLOAD_TYPE_H265);
}

std::string& AMRtpSessionH265::sps()
{
  while(m_sps.empty()){usleep(10000);}
  AUTO_MEM_LOCK(m_lock);
  return m_sps;
}

std::string& AMRtpSessionH265::pps()
{
  while(m_pps.empty()){usleep(10000);}
  AUTO_MEM_LOCK(m_lock);
  return m_pps;
}

std::string& AMRtpSessionH265::vps()
{
  while(m_vps.empty()){usleep(10000);}
  AUTO_MEM_LOCK(m_lock);
  return m_vps;
}

void AMRtpSessionH265::parse_vps(uint8_t *vps, uint32_t size)
{
  enum {
    PARSE_STAGE_0, /* Normal byte */
    PARSE_STAGE_1, /* 1st 0 */
    PARSE_STAGE_2, /* 2nd 0 */
  } parse_stage = PARSE_STAGE_0;
  uint32_t count = 0;
  uint32_t index = 0;

  memset(m_profile_tier_level, 0, sizeof(m_profile_tier_level));
  while((index < size) && (count < sizeof(m_profile_tier_level))) {
    switch(parse_stage) {
      case PARSE_STAGE_0: {
        m_profile_tier_level[count] = vps[index];
        parse_stage = (vps[index] == 0) ? PARSE_STAGE_1 : PARSE_STAGE_0;
        ++ count;
        ++ index;
      }break;
      case PARSE_STAGE_1: {
        m_profile_tier_level[count] = vps[index];
        parse_stage = (vps[index] == 0) ? PARSE_STAGE_2 : PARSE_STAGE_0;
        ++ count;
        ++ index;
      }break;
      case PARSE_STAGE_2: {
        if (vps[index] != 3) { /* Skip emulation byte */
          m_profile_tier_level[count] = vps[index];
          ++ count;
        }
        ++ index;
        parse_stage = PARSE_STAGE_0;
      }break;
    }
  }
  if (AM_LIKELY((index <= size) && (count == sizeof(m_profile_tier_level)))) {
    uint8_t *profile_tier_level = &m_profile_tier_level[6];
    char hex[8] = {0};
    m_profile_space = (0xC0 & profile_tier_level[0]) >> 6;
    m_tier_flag = (0x20 & profile_tier_level[0]) >> 5;
    m_profile_id = 0x1F & profile_tier_level[0];
    m_level_id = profile_tier_level[H265_PROFILE_TIER_LEVEL_SIZE - 1];
    m_profile_compatibility_indicator.clear();
    for (uint32_t i = 1; i < 5; ++ i) {
      sprintf(hex, "%02X", profile_tier_level[i]);
      m_profile_compatibility_indicator.append(hex);
    }
    m_interop_constraints.clear();
    for (uint32_t i = 5; i < 11; ++ i) {
      sprintf(hex, "%02X", profile_tier_level[i]);
      m_interop_constraints.append(hex);
    }
  }
}

#define DELIMITER_SIZE 3
bool AMRtpSessionH265::find_nalu(uint8_t *data, uint32_t len)
{
  bool ret = false;
  m_nalu_list.clear();
  if (AM_LIKELY(data && (len > DELIMITER_SIZE))) {
    int32_t index = -1;
    uint8_t *bs = data;
    uint8_t *last_header = bs + len - DELIMITER_SIZE;

    while (bs <= last_header) {
      if (AM_UNLIKELY(0x00000001 ==
          (0 | (bs[0] << 16) | (bs[1] << 8) | bs[2]))) {
        AM_H265_NALU nalu;

        ++ index;
        bs += DELIMITER_SIZE;
        nalu.type = (int32_t)((0x7E & bs[0]) >> 1);
        nalu.addr = bs;
        m_nalu_list.push_back(nalu);
        if (AM_LIKELY(index > 0)) {
          AM_H265_NALU &prev_nalu = m_nalu_list[index - 1];
          AM_H265_NALU &curr_nalu = m_nalu_list[index];
          /* calculate the previous NALU's size */
          prev_nalu.size = curr_nalu.addr - prev_nalu.addr - DELIMITER_SIZE;
          switch(prev_nalu.type) {
            case H265_SPS: {
              AUTO_MEM_LOCK(m_lock);
              m_sps = base64_encode(prev_nalu.addr, prev_nalu.size);
            }break;
            case H265_PPS: {
              AUTO_MEM_LOCK(m_lock);
              m_pps = base64_encode(prev_nalu.addr, prev_nalu.size);
            }break;
            case H265_VPS: {
              AUTO_MEM_LOCK(m_lock);
              m_vps = base64_encode(prev_nalu.addr, prev_nalu.size);
              parse_vps(prev_nalu.addr, prev_nalu.size);
            }break;
            default: break;
          }
        }
      } else if (bs[2] != 0) {
        bs += 3;
      } else if (bs[1] != 0) {
        bs += 2;
      } else {
        bs += 1;
      }
    }
    if (AM_LIKELY(index >= 0)) {
      /* calculate the last NALU's size */
      AM_H265_NALU &last_nalu = m_nalu_list[index];
      last_nalu.size = data + len - last_nalu.addr;
      switch(last_nalu.type) {
        case H265_SPS: {
          AUTO_MEM_LOCK(m_lock);
          m_sps = base64_encode(last_nalu.addr, last_nalu.size);
        }break;
        case H265_PPS: {
          AUTO_MEM_LOCK(m_lock);
          m_pps = base64_encode(last_nalu.addr, last_nalu.size);
        }break;
        case H265_VPS: {
          AUTO_MEM_LOCK(m_lock);
          m_vps = base64_encode(last_nalu.addr, last_nalu.size);
          parse_vps(last_nalu.addr, last_nalu.size);
        }break;
        default: break;
      }
      ret = true;
    }
  } else {
    if (AM_LIKELY(!data)) {
      ERROR("Invalid bit stream!");
    }
    if (AM_LIKELY(len <= DELIMITER_SIZE)) {
      ERROR("Bit stream is less equal than %d bytes!", DELIMITER_SIZE);
    }
  }

  return ret;
}

uint32_t AMRtpSessionH265::calc_h265_packet_num(AM_H265_NALU &nalu,
                                                uint32_t max_packet_size)
{
  uint32_t nal_payload_size = nalu.size - H265_NAL_HEADER_SIZE;
  uint32_t fua_payload_size = max_packet_size - H265_FU_INDICATOR_SIZE -
                              sizeof(H265_FU_HEADER);
  nalu.pkt_num = (nal_payload_size <= fua_payload_size) ? 1 :
      (((nal_payload_size % fua_payload_size) > 0) ?
          ((nal_payload_size / fua_payload_size) + 1) :
          (nal_payload_size / fua_payload_size));

  return nalu.pkt_num;
}

void AMRtpSessionH265::assemble_packet(AMRtpData *rtp_data,
                                       int32_t pts_incr,
                                       uint32_t max_tcp_payload_size,
                                       uint32_t max_packet_size)
{
  bool ts_incremented = false;
  uint8_t *buf = rtp_data->get_buffer();
  uint32_t packet_count = 0;

  for (uint32_t i = 0; i < m_nalu_list.size(); ++ i) {
    AMRtpPacket *packet = rtp_data->get_packet();
    AM_H265_NALU &nalu = m_nalu_list[i];
    uint8_t *addr = nalu.addr;
    uint32_t size = nalu.size;
    if (AM_UNLIKELY((nalu.type == H265_SEI_PREFIX) ||
                    (nalu.type == H265_SEI_SUFFIX) ||
                    (nalu.type == H265_AUD))) {
      continue;
    }
    if (AM_UNLIKELY(!ts_incremented)) {
      AUTO_MEM_LOCK(m_lock_ts);
      m_curr_time_stamp += pts_incr;
      ts_incremented = true;
    }
    if (AM_LIKELY(nalu.pkt_num > 1)) { /* FUA packetization */
      uint8_t        *temp_nalu = &addr[H265_NAL_HEADER_SIZE];
      uint32_t      remain_size = size - H265_NAL_HEADER_SIZE;
      uint32_t fua_payload_size = max_packet_size - H265_FU_INDICATOR_SIZE -
                                                    sizeof(H265_FU_HEADER);
      for (uint32_t j = 0; j < nalu.pkt_num; ++ j) {
        uint8_t *tcp_header = &buf[0];
        uint8_t *rtp_header = &buf[sizeof(AMRtpTcpHeader)];
        uint8_t *fua_start = &buf[sizeof(AMRtpTcpHeader) + sizeof(AMRtpHeader)];
        uint8_t *payload_addr = fua_start + H265_FU_INDICATOR_SIZE +
            sizeof(H265_FU_HEADER);
        uint16_t rtp_tcp_payload_size = 0;

        ++ m_sequence_num;
        packet[packet_count].tcp_data = tcp_header;
        packet[packet_count].udp_data = rtp_header;

        if (AM_LIKELY(remain_size >= fua_payload_size)) {
          memcpy(payload_addr, temp_nalu, fua_payload_size);
          packet[packet_count].total_size = max_tcp_payload_size;
          temp_nalu += fua_payload_size;
          remain_size -= fua_payload_size;
        } else {
          memcpy(payload_addr, temp_nalu, remain_size);
          packet[packet_count].total_size =
              remain_size + sizeof(AMRtpTcpHeader) + sizeof(AMRtpHeader) +
              H265_FU_INDICATOR_SIZE + sizeof(H265_FU_HEADER);
        }
        rtp_tcp_payload_size = (uint16_t)
            (packet[packet_count].total_size - sizeof(AMRtpTcpHeader));

        /* Build RTP TCP Header */
        tcp_header[0] = 0x24; /* Always 0x24 's' */
        /* tcp_header[1] should be set to client RTP channel id */
        tcp_header[2] = (rtp_tcp_payload_size & 0xff00) >> 8;
        tcp_header[3] = (rtp_tcp_payload_size & 0x00ff);

        /* Build RTP header */
        rtp_header[0] = 0x80;
        rtp_header[1] = (j == (nalu.pkt_num - 1)) ?
            0x80 | ((RTP_PAYLOAD_TYPE_H265) & 0x7f) :
            RTP_PAYLOAD_TYPE_H265;
        rtp_header[2]  = (m_sequence_num & 0xff00) >> 8;
        rtp_header[3]  = m_sequence_num & 0x00ff;
        rtp_header[4]  = (m_curr_time_stamp & 0xff000000) >> 24;
        rtp_header[5]  = (m_curr_time_stamp & 0x00ff0000) >> 16;
        rtp_header[6]  = (m_curr_time_stamp & 0x0000ff00) >> 8;
        rtp_header[7]  = (m_curr_time_stamp & 0x000000ff);
        rtp_header[8]  = 0; // SSRC[3] filled by Client when sending this packet
        rtp_header[9]  = 0; // SSRC[2]
        rtp_header[10] = 0; // SSRC[1]
        rtp_header[11] = 0; // SSRC[0]
        /* FUA Indicator */
        fua_start[0]   = ((0x81 & addr[0]) | (0x31 << 1));
        fua_start[1]   = addr[1];
        /* FUA Header */
        fua_start[2]   = ((0x7E & addr[0]) >> 1) |
            (((j > 0) && (j < (nalu.pkt_num - 1))) ? 0 :
                (j == 0) ? 0x80 /* start bit */ : 0x40 /* end bit */);
        buf += max_tcp_payload_size;
        ++ packet_count;
      }
    } else { /* Single Packetization */
      uint8_t *tcp_header = &buf[0];
      uint8_t *rtp_header = &buf[sizeof(AMRtpTcpHeader)];
      uint8_t *payload    = &buf[sizeof(AMRtpTcpHeader) + sizeof(AMRtpHeader)];
      uint32_t rtp_tcp_payload_size = 0;

      ++ m_sequence_num;
      memcpy(payload, nalu.addr, nalu.size);
      packet[packet_count].tcp_data = tcp_header;
      packet[packet_count].udp_data = rtp_header;
      packet[packet_count].total_size = nalu.size + sizeof(AMRtpTcpHeader) +
          sizeof(AMRtpHeader);

      rtp_tcp_payload_size =
          packet[packet_count].total_size - sizeof(AMRtpTcpHeader);
      /* Build RTP TCP Header */
      tcp_header[0] = 0x24; /* Always 0x24 's' */
      /* tcp_header[1] should be set to client RTP channel id */
      tcp_header[2] = (rtp_tcp_payload_size & 0xff00) >> 8;
      tcp_header[3] = (rtp_tcp_payload_size & 0x00ff);

      /* Build RTP Header */
      rtp_header[0] = 0x80;
      rtp_header[1] = 0x80 | ((RTP_PAYLOAD_TYPE_H265) & 0x7f);
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
      buf += max_tcp_payload_size;
      ++ packet_count;
    }
  }
}
