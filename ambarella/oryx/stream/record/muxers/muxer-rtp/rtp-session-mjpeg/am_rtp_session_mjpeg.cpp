/*******************************************************************************
 * am_muxer_rtp_mjpeg.cpp
 *
 * History:
 *   2015-1-16 - [ypchang] created file
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

#include "am_rtp_session_mjpeg.h"
#include "am_rtp_data.h"
#include "am_video_types.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_mutex.h"
#include "mjpeg.h"
#include "rtp.h"

#ifdef __cplusplus
extern "C" {
#endif
AMIRtpSession* create_rtp_session(const char *name,
                                  MuxerRtpConfig *muxer_rtp_config,
                                  void *vinfo)
{
  return AMRtpSessionMjpeg::create(name,
                                   muxer_rtp_config,
                                   (AM_VIDEO_INFO*)vinfo);
}
#ifdef __cplusplus
}
#endif

AMIRtpSession* AMRtpSessionMjpeg::create(const std::string &name,
                                         MuxerRtpConfig *muxer_rtp_config,
                                         AM_VIDEO_INFO *vinfo)
{
  AMRtpSessionMjpeg *result = new AMRtpSessionMjpeg(name, muxer_rtp_config);
  if (AM_UNLIKELY(result && !result->init(vinfo))) {
    delete result;
    result = nullptr;
  }

  return result;
}

uint32_t AMRtpSessionMjpeg::get_session_clock()
{
  return 90000;
}

AM_SESSION_STATE AMRtpSessionMjpeg::process_data(AMPacket &packet,
                                                 uint32_t max_tcp_payload_size)
{
  AM_SESSION_STATE state = AM_SESSION_OK;
  int64_t curr_pts = packet.get_pts();
  if (AM_LIKELY(parse_jpeg(m_jpeg_param,
                           packet.get_data_ptr(),
                           packet.get_data_size()))) {
    AMRtpData *rtp_data = alloc_data(curr_pts);

    if (AM_LIKELY(rtp_data)) {
      uint8_t q_factor = (uint8_t)(packet.get_frame_attr());
      uint32_t max_packet_size = max_tcp_payload_size -
          sizeof(AMRtpTcpHeader) - sizeof(AMRtpHeader);
      uint32_t total_payload_size = m_jpeg_param->len +
          ((q_factor >= 128) ? m_jpeg_param->q_table_total_len : 0);
      uint32_t payload_size = max_packet_size - sizeof(AMJpegHeader);
      uint32_t pkt_num = (total_payload_size / payload_size) +
          ((total_payload_size % payload_size) ? 1 : 0);
      uint32_t total_data_size = pkt_num * max_tcp_payload_size;
      if (AM_LIKELY(total_data_size &&
                    rtp_data->create(this,
                                     total_data_size,
                                     packet.get_data_offset(),
                                     pkt_num))) {
        int32_t pkt_pts_incr_90k = (m_prev_pts > 0) ?
            (int32_t)(curr_pts - m_prev_pts) : 0;

        DEBUG("FrameNum: %u, Preview PTS: %lld, Current PTS: %lld, "
              "PTS diff in 90K is %d",
              packet.get_frame_number(),
              m_prev_pts,
              curr_pts,
              pkt_pts_incr_90k);
        assemble_packet(rtp_data,
                        pkt_pts_incr_90k,
                        max_tcp_payload_size,
                        q_factor,
                        (q_factor >= 128));
        add_rtp_data_to_client(rtp_data);
      }
      rtp_data->release();
    } else {
      state = AM_SESSION_IO_SLOW;
      ERROR("Network too slow? Not enough data buffer! Drop data!");
    }
  } else {
    state = AM_SESSION_IO_SLOW;
    WARN("Failed to parse MJPEG data! Corrupted bit stream?");
  }
  m_prev_pts = curr_pts;

  return state;
}

AMRtpSessionMjpeg::AMRtpSessionMjpeg(const std::string &name,
                                     MuxerRtpConfig *muxer_rtp_config) :
  inherited(name, muxer_rtp_config, AM_SESSION_MJPEG),
  m_video_info(nullptr),
  m_jpeg_param(nullptr)
{}

AMRtpSessionMjpeg::~AMRtpSessionMjpeg()
{
  delete m_video_info;
  delete m_jpeg_param;
}

bool AMRtpSessionMjpeg::init(AM_VIDEO_INFO *vinfo)
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

    m_jpeg_param = new AMJpegParams();
    if (AM_UNLIKELY(!m_jpeg_param)) {
      ERROR("Failed to allocate memory for MJPEG info!");
      break;
    }
    ret = true;
  }while(0);

  return ret;
}

void AMRtpSessionMjpeg::generate_sdp(std::string& host_ip_addr, uint16_t port,
                                     AM_IP_DOMAIN domain)
{
  m_sdp.clear();
  m_sdp =
      "m=video " + std::to_string(port) + " RTP/AVP " +
      get_payload_type() + "\r\n" +
      "c=IN " + ((domain == AM_IPV4)?"IP4" : "IP6") +
      " " + host_ip_addr + "\r\n" +
      "a=sendonly\r\n" + get_param_sdp() +
      "a=cliprect:0,0," + std::to_string(m_video_info->height) + "," +
      std::to_string(m_video_info->width) + "\r\n" +
      "a=control:" + "rtsp://" + host_ip_addr + "/video=" + m_name + "\r\n";
}

std::string AMRtpSessionMjpeg::get_param_sdp()
{
  return std::string("a=rtpmap:" + std::to_string(RTP_PAYLOAD_TYPE_JPEG) +
                     " JPEG/90000\r\n");
}

std::string AMRtpSessionMjpeg::get_payload_type()
{
  return std::to_string(RTP_PAYLOAD_TYPE_JPEG);
}

bool AMRtpSessionMjpeg::parse_jpeg(AMJpegParams *jpeg,
                                   uint8_t *data,
                                   uint32_t len)
{
  bool ret = false;
  if (AM_LIKELY(jpeg && data && len > 4)) {
    do {
      uint8_t *start  = data;
      uint8_t *end    = data + len - 2;
      uint32_t count  = 0;

      jpeg->clear();
      while (end > start) {
        /* Check picture end mark 0xFFD9 */
        if (AM_LIKELY(((end[0] << 8) | end[1]) == 0xFFD9)) {
          break;
        }
        -- end;
      }
      if (AM_UNLIKELY(((end[0] << 8) | end[1]) != 0xFFD9)) {
        ERROR("Failed to check end mark 0xffd9!");
        break;
      }

      if (AM_UNLIKELY(((start[0] << 8) | start[1]) != 0xFFD8)) {
        /* Check picture start mark 0xFFD8 SOI */
        ERROR("Failed to check start mark 0xffd8!");
        break;
      } else {
        start += 2; /* Skip 0xFFD8 */
      }

      /* Skip APP0 ~ APPn */
      while (((end - start) > 2) &&
          (((start[0] << 8) | (start[1] & 0xF0)) == 0xFFE0)) {
        start += 2; /* Skip 0xFFEx */
        start += ((start[0] << 8) | start[1]);
      }

      while (((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFDB)) {
        /* Check DQT mark 0xFFDB */
        AMJpegQtable q_table;
        uint8_t precision = 0;

        start += 2; /* Skip 0xFFDB */
        precision = start[2] >> 4;

        if (0 == precision) {
          jpeg->precision &= ~(1 << count);
          q_table.len = 64;
        } else if (1 == precision) {
          jpeg->precision |= (1 << count);
          q_table.len = 64 << 1;
        } else {
          ERROR("Invalid precision value: %hhu", precision);
          start = end; /* Make an error to quit parsing */
          break;
        }
        jpeg->q_table_total_len += q_table.len;
        q_table.data = &start[3];
        jpeg->q_table_list.push_back(q_table);
        start += ((start[0] << 8) | start[1]);
        ++ count;
      }

      if (AM_UNLIKELY(((end - start) <= 2) ||
                      (((start[0] << 8) | start[1]) != 0xFFC0))) {
        /* Check SOF0 mark 0xFFC0 */
        ERROR("Failed to check SOF0 0xffc0: end - start = %u "
              "start[0] == %02x, start[1] == %02x",
              end - start, start[0], start[1]);
        break;
      } else {
        start += 2;
      }
      if (AM_UNLIKELY((end - start) < 17)) {
        ERROR("Invalid data length!");
        break;
      }
      if (AM_UNLIKELY(start[2] != 8)) {
        /* Need precision to be 8 */
        ERROR("Invalid precision value: %hhu!", start[2]);
        break;
      }
      jpeg->height = ((start[3] << 8) | start[4]);
      jpeg->width  = ((start[5] << 8) | start[6]);
      if (AM_UNLIKELY(start[7] != 3)) {
        /* component number must be 3 */
        ERROR("Invalid component number: %hhu", start[7]);
        break;
      } else {
        uint8_t id[3]        = {0};
        uint8_t factor[3]    = {0};
        uint8_t qTableNum[3] = {0};
        for (uint32_t i = 0; i < 3; ++ i) {
          id[i]        = start[8 + i * 3];
          factor[i]    = start[9 + i * 3];
          qTableNum[i] = start[10 + i * 3];
        }
        if (((id[0] == 1) && (factor[0] == 0x21) && (qTableNum[0] == 0)) &&
            ((id[1] == 2) && (factor[1] == 0x11) && (qTableNum[1] == 1)) &&
            ((id[2] == 3) && (factor[2] == 0x11) && (qTableNum[2] == 1))) {
          jpeg->type = 0;
        } else if (((id[0]==1) && (factor[0]==0x22) && (qTableNum[0]==0)) &&
                   ((id[1]==2) && (factor[1]==0x11) && (qTableNum[1]==1)) &&
                   ((id[2]==3) && (factor[2]==0x11) && (qTableNum[2]==1))) {
          jpeg->type = 1;
        } else {
          ERROR("Invalid type!");
          break;
        }
      }
      start += ((start[0] << 8) | start[1]);

      while(((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFC4)) {
        /* Skip DHT */
        start += 2; /* Skip 0xFFC4 */
        start += ((start[0] << 8) | start[1]);
      }

      if (((end - start) > 2) && (((start[0] << 8) | start[1]) == 0xFFDD)) {
        /* DRI exists */
        start += 2; /* Skip 0xFFDD */
        start += ((start[0] << 8) | start[1]);
      }

      if (AM_UNLIKELY(((end - start) <= 2) ||
                      (((start[0] << 8) | start[1]) != 0xFFDA))) {
        /* Check SOS mark */
        ERROR("Failed to check SOS mark 0xffda!");
        break;
      } else {
        start += 2; /* Skip 0xFFDA */
      }
      start += ((start[0] << 8) | start[1]);
      if (AM_UNLIKELY(start >= end)) {
        ERROR("Invalid data length!");
        break;
      }
      jpeg->data = start;
      jpeg->len  = end - start;
      ret = true;
    } while(0);
  }

  return ret;
}

void AMRtpSessionMjpeg::assemble_packet(AMRtpData *rtp_data,
                                        int32_t pts_incr,
                                        uint32_t max_tcp_payload_size,
                                        uint8_t q_factor,
                                        bool need_q_table)
{
  uint8_t *buf = rtp_data->get_buffer();
  uint32_t pkt_num = rtp_data->get_packet_number();
  AMRtpPacket *packet = rtp_data->get_packet();
  m_curr_time_stamp += pts_incr;

  for (uint32_t count = 0; count < pkt_num; ++ count) {
    uint8_t *tcp_header   = buf;
    uint8_t *rtp_header   = tcp_header + sizeof(AMRtpTcpHeader);
    uint8_t *jpg_header   = rtp_header + sizeof(AMRtpHeader);
    uint8_t *data_ptr     = jpg_header + sizeof(AMJpegHeader);
    uint32_t pkt_hdr_size = sizeof(AMRtpTcpHeader) + sizeof(AMRtpHeader) +
        sizeof(AMJpegHeader) + ((need_q_table && (count == 0)) ?
            (sizeof(AMJpegQuantizationHeader) +
                m_jpeg_param->q_table_total_len) : 0);
    uint32_t remain_size = (m_jpeg_param->len - m_jpeg_param->offset);
    uint32_t max_payload_size = (max_tcp_payload_size - pkt_hdr_size);
    uint32_t payload_data_size = (remain_size > max_payload_size) ?
        max_payload_size : remain_size;
    uint32_t rtp_tcp_payload_size = pkt_hdr_size - sizeof(AMRtpTcpHeader) +
        payload_data_size;
    ++ m_sequence_num;

    /* Build RTP TCP header */
    tcp_header[0]  = 0x24;
    /* tcp_header[1] should be set to client rtp channel id */
    tcp_header[1]  = (rtp_tcp_payload_size & 0xff00) >> 8;
    tcp_header[2]  = (rtp_tcp_payload_size & 0x00ff);

    /* Build RTP header */
    rtp_header[0]  = 0x80;
    rtp_header[1]  = (count == (pkt_num - 1)) ?
        (0x80 | (RTP_PAYLOAD_TYPE_JPEG & 0x7f)) : RTP_PAYLOAD_TYPE_JPEG;
    rtp_header[2]  = (m_sequence_num & 0xff00) >> 8;
    rtp_header[3]  = (m_sequence_num & 0x00ff);
    rtp_header[4]  = (m_curr_time_stamp & 0xff000000) >> 24;
    rtp_header[5]  = (m_curr_time_stamp & 0x00ff0000) >> 16;
    rtp_header[6]  = (m_curr_time_stamp & 0x0000ff00) >>  8;
    rtp_header[7]  = (m_curr_time_stamp & 0x000000ff);
    rtp_header[8]  = 0;
    rtp_header[9]  = 0;
    rtp_header[10] = 0;
    rtp_header[11] = 0;

    /* Build JPEG header */
    jpg_header[0]  = 0;
    jpg_header[1]  = (m_jpeg_param->offset >> 16) & 0xff;
    jpg_header[2]  = (m_jpeg_param->offset >>  8) & 0xff;
    jpg_header[3]  = (m_jpeg_param->offset & 0xff);
    jpg_header[4]  = m_jpeg_param->type;
    jpg_header[5]  = q_factor;
    jpg_header[6]  = m_jpeg_param->width  >> 3;
    jpg_header[7]  = m_jpeg_param->height >> 3;
    if (AM_LIKELY(need_q_table && (count == 0))) {
      AMJpegQuantizationHeader *quantization_hdr =
          (AMJpegQuantizationHeader*)data_ptr;
      quantization_hdr->mbz       = 0;
      quantization_hdr->precision = m_jpeg_param->precision;
      quantization_hdr->length2   = (m_jpeg_param->q_table_total_len>>8) & 0xff;
      quantization_hdr->length1   = (m_jpeg_param->q_table_total_len & 0xff);
      data_ptr += sizeof(AMJpegQuantizationHeader);
      for (uint32_t i = 0; i < m_jpeg_param->q_table_list.size(); ++ i) {
        memcpy(data_ptr, m_jpeg_param->q_table_list[i].data,
               m_jpeg_param->q_table_list[i].len);
        data_ptr += m_jpeg_param->q_table_list[i].len;
      }
    }
    memcpy(data_ptr, m_jpeg_param->data + m_jpeg_param->offset,
           payload_data_size);
    m_jpeg_param->offset += payload_data_size;
    packet[count].tcp_data = tcp_header;
    packet[count].udp_data = rtp_header;
    packet[count].total_size = pkt_hdr_size + payload_data_size;
    buf = data_ptr + payload_data_size;
  }
}

