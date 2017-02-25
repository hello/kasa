/*******************************************************************************
 * am_demuxer_rtp.cpp
 *
 * History:
 *   2014-11-16 - [ccjing] created file
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

#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_mutex.h"
#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_pool.h"
#include "am_thread.h"

#include "am_playback_info.h"
#include "am_audio_define.h"
#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_demuxer_rtp.h"
#include "rtp.h"

#define PACKET_POOL_SIZE   64
#define SOCKET_BUFFER_SIZE 8192
#define PACKET_CHUNK_SIZE  512

int ctrl_rtp[2] = { -1 };
#define CTRL_READ_RTP    ctrl_rtp[0]
#define CTRL_WRITE_RTP   ctrl_rtp[1]

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_RTP;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerRtp::create(streamid);
}

AMDemuxerRtp::AMDemuxerRtp(uint32_t streamid) :
    inherited(AM_DEMUXER_RTP, streamid)
{
}

AMDemuxerRtp::~AMDemuxerRtp()
{
  delete   m_audio_info;
  delete   m_packet_queue;
  delete[] m_recv_buf;
}

void AMDemuxerRtp::stop()
{
  if (AM_LIKELY(m_run.load() && CTRL_WRITE_RTP >= 0)) {
    write(CTRL_WRITE_RTP, "s", 1);
    INFO("Write stop cmd to rtp demuxer mainloop!");
  }
  AM_DESTROY(m_thread);/*already set m_thread NULL inside of destroy*/
  m_sock_lock.lock();
  if (m_sock_fd >= 0) {
    close(m_sock_fd);
    m_sock_fd = -1;
  }
  m_sock_lock.unlock();
  while (AM_LIKELY(!m_packet_queue->empty())) {
    m_packet_pool->release(m_packet_queue->front_pop());
  }
  if (AM_LIKELY(CTRL_READ_RTP >= 0)) {
    close(CTRL_READ_RTP);
    CTRL_READ_RTP = -1;
  }
  if (AM_LIKELY(CTRL_WRITE_RTP >= 0)) {
    close(CTRL_WRITE_RTP);
    CTRL_WRITE_RTP = -1;
  }
  m_is_new_sock = true;
}

AMIDemuxerCodec* AMDemuxerRtp::create(uint32_t streamid)
{
  AMDemuxerRtp* demuxer = new AMDemuxerRtp(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = NULL;
  }
  return ((AMIDemuxerCodec*) demuxer);
}

void AMDemuxerRtp::destroy()
{
  enable(false);
  inherited::destroy();
}

void AMDemuxerRtp::enable(bool enabled)
{
  if(!enabled) {
    stop();
  }
  AUTO_MEM_LOCK(m_lock);
  if (AM_LIKELY(m_packet_pool && (m_is_enabled != enabled))) {
    m_is_enabled = enabled;
    m_packet_pool->enable(enabled);
    NOTICE("%s AMDemuxerRtp!", enabled ? "Enable" : "Disable");
  }
}

bool AMDemuxerRtp::is_drained()
{
  return (m_packet_pool->get_avail_packet_num() == PACKET_POOL_SIZE);
}

bool AMDemuxerRtp::is_play_list_empty()
{
  return (m_sock_fd == -1);
}

bool AMDemuxerRtp::add_uri(const AMPlaybackUri& uri)
{
  bool ret = true;
  do{
    if(uri.type != AM_PLAYBACK_URI_RTP) {
      ERROR("uri type error.");
      ret = false;
      break;
    }
    const AMPlaybackRtpUri& rtp_info = uri.media.rtp;
    m_audio_info->sample_rate = rtp_info.sample_rate;
    m_audio_info->channels = rtp_info.channel;
    m_audio_info->type = rtp_info.audio_type;
    switch(rtp_info.audio_type) {
      case AM_AUDIO_G711A:
        m_audio_info->sample_format = AM_SAMPLE_ALAW;
        m_audio_info->sample_size = sizeof(int8_t);
        break;
      case AM_AUDIO_G711U:
        m_audio_info->sample_format = AM_SAMPLE_ULAW;
        m_audio_info->sample_size = sizeof(int8_t);
        break;
      case AM_AUDIO_BPCM:
        m_audio_info->sample_format = AM_SAMPLE_S16BE;
        m_audio_info->sample_size = sizeof(int16_t);
        break;
      case AM_AUDIO_LPCM:
      default:
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        break;
    }
    if (AM_LIKELY(rtp_info.udp_port >= 0)) {
      ret = add_udp_port(rtp_info);
    } else {
      ERROR("Udp port below zero!");
      ret = false;
      break;
    }
  }while(0);
  return ret;
}

bool AMDemuxerRtp::add_udp_port(const AMPlaybackRtpUri& rtp_uri)
{
  struct sockaddr* addr = NULL;
  struct sockaddr_in addr4 = { 0 };
  struct sockaddr_in6 addr6 = { 0 };
  socklen_t socket_len = sizeof(struct sockaddr);
  bool ret = true;
  if (AM_UNLIKELY(m_thread)) {
    NOTICE("The thread is running now, stop it first.");
    stop();
  }
  AUTO_MEM_LOCK(m_sock_lock);
  if (AM_UNLIKELY(m_sock_fd >= 0)) {
    close(m_sock_fd);
    m_sock_fd = -1;
  }
  do {
    switch (m_audio_info->type) {
      case AM_AUDIO_G711A:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_PCMA;
        m_audio_codec_type = AM_AUDIO_CODEC_G711;
        INFO("Audio type is g711A.");
        break;
      case AM_AUDIO_G711U:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_PCMU;
        m_audio_codec_type = AM_AUDIO_CODEC_G711;
        INFO("Audio type is g711U.");
        break;
      case AM_AUDIO_G726_16:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_16;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        INFO("Audio type is g726_16.");
        break;
      case AM_AUDIO_G726_24:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_24;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        INFO("Audio type is g726_24.");
        break;
      case AM_AUDIO_G726_32:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_32;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        INFO("Audio type is g726_32.");
        break;
      case AM_AUDIO_G726_40:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_40;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        INFO("Audio type is g726_40.");
        break;
      case AM_AUDIO_OPUS:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_OPUS;
        m_audio_codec_type = AM_AUDIO_CODEC_OPUS;
        INFO("Audio type is opus.");
        break;
      default:
        ERROR("Not support the audio format %d.", m_audio_info->type);
        m_audio_codec_type = AM_AUDIO_CODEC_NONE;
        ret = false;
        break;
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    switch(rtp_uri.ip_domain) {
      case AM_PLAYBACK_IPV4: {
        m_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(AM_LIKELY(m_sock_fd >= 0)) {
          addr4.sin_family = AF_INET;
          addr4.sin_addr.s_addr = htonl(INADDR_ANY);
          addr4.sin_port = htons(rtp_uri.udp_port);
          addr = (struct sockaddr*)&addr4;
          socket_len = sizeof(addr4);
        }else {
          PERROR("socket");
        }
      }break;
      case AM_PLAYBACK_IPV6: {
        m_sock_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        if(AM_LIKELY(m_sock_fd >= 0)) {
          addr6.sin6_family = AF_INET6;
          addr6.sin6_addr = in6addr_any;
          addr6.sin6_port = htons(rtp_uri.udp_port);
          addr = (struct sockaddr*)&addr6;
          socket_len = sizeof(addr6);
        } else {
          PERROR(socket);
        }
      }break;
      default:break;
    }
    if(AM_LIKELY(m_sock_fd >= 0)) {
      int recv_buf = 4 * SOCKET_BUFFER_SIZE;
      if (AM_UNLIKELY(setsockopt(m_sock_fd, SOL_SOCKET, SO_RCVBUF,
                                 &recv_buf, sizeof(int)) < 0)) {
        ERROR("Failed to set socket receive buf!");
        ret = false;
        break;
      }
      int reuse = 1;
      if (AM_UNLIKELY(setsockopt(m_sock_fd, SOL_SOCKET, SO_REUSEADDR,
                                 &reuse, sizeof(reuse)) < 0)) {
        ERROR("Failed to set addr reuse!");
        ret = false;
        break;
      }
      int flag = fcntl(m_sock_fd, F_GETFL, 0);
      flag |= O_NONBLOCK;
      if (AM_UNLIKELY(fcntl(m_sock_fd, F_SETFL, flag) != 0)) {
        PERROR("fcntl");
        ret = false;
        break;
      }
      if(AM_UNLIKELY((ret == false) && (m_sock_fd >= 0))) {
        close(m_sock_fd);
        m_sock_fd = -1;
        break;
      }
      if (AM_UNLIKELY(bind(m_sock_fd, addr, socket_len) != 0)) {
        close(m_sock_fd);
        m_sock_fd = -1;
        ERROR("Failed to bind socket");
        ret = false;
        break;
      }
    } else {
      ERROR("Add udp port error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(pipe(ctrl_rtp) < 0)) {
      ERROR("Failed to create pipe!");
      ret = false;
      break;
    }
    m_thread = AMThread::create("DemuxerRtp", thread_entry, this);
    if (AM_UNLIKELY(!m_thread)) {
      ERROR("Failed to create m_thread!");
      ret = false;
      m_thread = NULL;
      break;
    }
  } while (0);

  return ret;
}

AM_DEMUXER_STATE AMDemuxerRtp::get_packet(AMPacket* &packet)
{
  AM_DEMUXER_STATE ret = AM_DEMUXER_OK;
  packet = NULL;
  if (AM_UNLIKELY(m_is_new_sock)) {
    if(m_is_info_ready) {
      if (AM_LIKELY(allocate_packet(packet))) {
        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->set_frame_type(m_audio_codec_type);
        packet->set_stream_id(m_stream_id);
        packet->set_pts(0LL);
        packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
        packet->set_data_size(sizeof(AM_AUDIO_INFO));
        AM_AUDIO_INFO* audioInfo = ((AM_AUDIO_INFO*) packet->get_data_ptr());
        audioInfo->channels = m_audio_info->channels;
        audioInfo->sample_rate = m_audio_info->sample_rate;
        audioInfo->sample_size = m_audio_info->sample_size;
        audioInfo->sample_format = m_audio_info->sample_format;
        audioInfo->type = m_audio_info->type;
        audioInfo->chunk_size = m_audio_info->chunk_size;
        INFO("\ndemuxer rtp info :"
              "\n      channels: %d"
              "\n  sample rate : %d"
              "\n  sample size : %d"
              "\n   chunk size : %d"
              "\nsample format : %s"
              "\n         type : %s",
               audioInfo->channels,
               audioInfo->sample_rate,
               audioInfo->sample_size,
               audioInfo->chunk_size,
               sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT
                                       (audioInfo->sample_format)).c_str(),
               audio_type_to_string(audioInfo->type).c_str());
        m_is_new_sock = false;
      } else {
        ERROR("Failed to allocate packet.");
        ret = AM_DEMUXER_NO_PACKET;
      }
    }else {
      ret = AM_DEMUXER_NO_PACKET;
    }
  } else {
    if (AM_LIKELY(!(m_packet_queue->empty()))) {
      packet = m_packet_queue->front_pop();
    } else {
      AUTO_MEM_LOCK(m_sock_lock);
      if (AM_UNLIKELY((m_sock_fd == -1) || (!m_run.load()))) {
        ret = AM_DEMUXER_NO_FILE;
        NOTICE("Socket is closed or m_run is false, return AM_DEMUXER_NO_FILE");
      } else {
        ret = AM_DEMUXER_NO_PACKET;
      }
    }
  }
  return ret;
}

AM_STATE AMDemuxerRtp::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(AM_STATE_OK != (state = inherited::init()))) {
      break;
    }
    m_packet_pool = AMFixedPacketPool::create("RTPDemuxerPacketPool",
                                              PACKET_POOL_SIZE,
                                              PACKET_CHUNK_SIZE);
    if (AM_UNLIKELY(!m_packet_pool)) {
      ERROR("Failed to create packet pool for RTP demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_audio_info = new AM_AUDIO_INFO();
    if (AM_UNLIKELY(!m_audio_info)) {
      ERROR("Failed to create m_audio_info for RTP demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_recv_buf = new char[SOCKET_BUFFER_SIZE];
    if (AM_UNLIKELY(!m_recv_buf)) {
      ERROR("Failed to alloc m_recv_buf!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_packet_queue = new packet_queue();
    if (AM_UNLIKELY(!m_packet_queue)) {
      ERROR("Failed to create m_packet_queue!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

  } while (0);

  return state;
}

void AMDemuxerRtp::thread_entry(void *p)
{
  ((AMDemuxerRtp*)p)->main_loop();
}

void AMDemuxerRtp::main_loop()
{
  AMPacket* packet = NULL;
  fd_set rfds;
  fd_set zfds; //zero fd
  uint32_t data_len = 0;
  AMRtpHeader* rtp_header = NULL;
  uint16_t seq_number = -1;
  char* read_data = NULL;
  uint32_t read_data_len = 0;
  uint32_t ssrc = 0;
  int max_fd = -1;
  uint32_t packet_count = 0;
  if (AM_LIKELY(m_sock_fd >=0 && CTRL_READ_RTP >= 0)) {
    m_run = true;
    AUTO_MEM_LOCK(m_sock_lock);
    FD_ZERO(&zfds);
    FD_SET(m_sock_fd, &zfds);
    FD_SET(CTRL_READ_RTP, &zfds);
    max_fd = AM_MAX(m_sock_fd, CTRL_READ_RTP);
  } else {
    ERROR("m_socket_fd < 0 or CTRL_READ_RTP < 0. Stop it");
    m_run = false;
    stop();
  }
  while (m_run.load()) {
    rfds = zfds;
    int sret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
    if (AM_UNLIKELY(sret <= 0)) {
      PERROR("select");
      if (errno != EAGAIN) {
        m_run = false;
      }
      ssrc = 0;
      continue;
    }
    if (AM_UNLIKELY(FD_ISSET(CTRL_READ_RTP, &rfds))) {
      char cmd[1] = { 0 };
      if (AM_LIKELY(read(CTRL_READ_RTP, cmd, sizeof(cmd)) < 0)) {
        PERROR("Read form CTRL_READ_RTP");
        continue;
      } else if (cmd[0] == 's') {
        INFO("receive stop cmd in demuxer rtp mainloop.");
        m_run = false;
        continue;
      }
    }
    if (AM_LIKELY(FD_ISSET(m_sock_fd, &rfds))) {
      if (data_len >= SOCKET_BUFFER_SIZE) {
        NOTICE("Receive buf already full, will clear the data in receive buf.");
        data_len = 0;
        continue;
      } else {
        m_sock_lock.lock();
        int recv_len = recv(m_sock_fd, m_recv_buf + data_len,
                               SOCKET_BUFFER_SIZE - data_len, 0);
        m_sock_lock.unlock();
        if(AM_UNLIKELY(recv_len < 0)) {
          if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            continue;
          } else {
            PERROR("recvfrom");
            m_run = false;
            break;
          }
        }
        data_len += recv_len;
        DEBUG("recvfrom data size = %d, dataLen = %d", recv_len, data_len);
        if (data_len <= 2 * sizeof(AMRtpHeader)) {
          INFO("dataLen < rtp header length, continue receive.");
          continue;
        }
      }
      read_data = m_recv_buf;
      read_data_len = data_len;
      while (read_data_len > 1) {
        //ensure the top of the read_data is the begin of the rtp packet
        if (AM_LIKELY((read_data[0] == 0x80)
            && ((read_data[1] & 0x7f) == m_audio_rtp_type))) {
          break;
        } else {
          INFO("mReadData[0] == %d, mReadData[1] == %d. Rtp header error.",
                 read_data[0], read_data[1]);
          ++ read_data;
          -- read_data_len;
        }
      }
      if (AM_UNLIKELY(read_data_len == 1)) {
        if (read_data[0] == 0x80) {
          INFO("mReadDataLen == 1 && mReadData[0] == 0x80");
          data_len = 1;
          memmove(m_recv_buf, read_data, data_len);
          continue;
        }
        INFO("mReadDataLen == 1 && mReadData[0] != 0x80");
        data_len = 0;
        continue;
      }
      if (read_data_len <= 2 * sizeof(AMRtpHeader)) {
        INFO("mReadDataLen <= 2 * sizeof(AMRtpHeader)");
        data_len = read_data_len;
        memmove(m_recv_buf, read_data, data_len);
        continue;
      }
      while (true) {
        int rtp_payload_len = 0;
        while (read_data_len > 2 * sizeof(AMRtpHeader)) {
          /* find the next begin of the rtp packet and
           * get the length of the payload
           */
          if (AM_UNLIKELY((read_data[sizeof(AMRtpHeader) + rtp_payload_len]
                                     == 0x80) &&
                          ((read_data[sizeof(AMRtpHeader) + rtp_payload_len + 1]
                                      & 0x7f) == m_audio_rtp_type))) {
            rtp_header = (AMRtpHeader*)(&read_data[12 + rtp_payload_len]);
            if ((ssrc == 0) || (rtp_header->ssrc == ssrc)) {
              DEBUG("mReadData[0] == 0x80 && mReadData[1] == "
                  "%d, rtpPayloadLen = %d",
                  m_audio_rtp_type, rtp_payload_len);
              if(!m_is_info_ready) {
                if(rtp_payload_len > PACKET_CHUNK_SIZE) {
                  ERROR("The length of rtp payload is %d, "
                      "the PACKET_CHUNK_SIZE is %d, "
                      "the length of rtp payload is larger than "
                      "PACKET_CHUNK_SIZE. Exit the demuxer rtp mainloop",
                      rtp_payload_len, PACKET_CHUNK_SIZE);
                  m_run = false;
                  m_packet_count = 0;
                } else {
                  if(m_audio_codec_type == AM_AUDIO_CODEC_OPUS) {
                    m_packet_count = 1;
                  } else {
                    m_packet_count = PACKET_CHUNK_SIZE / rtp_payload_len;
                  }
                  m_audio_info->chunk_size = rtp_payload_len * m_packet_count;
                  NOTICE("Audio info chunk size is %d, packet count is %d",
                         m_audio_info->chunk_size, m_packet_count);
                  m_is_info_ready = true;
                }
              }
              break;
            }
          }
          ++ rtp_payload_len;
          -- read_data_len;
        }
        if (AM_UNLIKELY(read_data_len == 2 * sizeof(AMRtpHeader))) {
          data_len = read_data_len + rtp_payload_len;
          memmove(m_recv_buf, read_data, data_len);
          break;
        }
        if((packet_count == 0) && (!packet)) {
          if (AM_UNLIKELY(!allocate_packet(packet))){
            NOTICE("Failed to allocate packet.");
            data_len = read_data_len + rtp_payload_len;
            memmove(m_recv_buf, read_data, data_len);
            packet = NULL;
            break;
          }
        }
        if((!packet) || (m_packet_count == 0)) {
          break;
        }
        rtp_header = (AMRtpHeader*)(&read_data[0]);
        if (ntohs(rtp_header->sequencenumber) == seq_number) {
          NOTICE("seqNumber same, seqNumber = %d, drop it", seq_number);
          read_data += sizeof(AMRtpHeader) + rtp_payload_len;
          read_data_len -= sizeof(AMRtpHeader);
          continue;
        } else {
          seq_number = ntohs(rtp_header->sequencenumber);
          ssrc = rtp_header->ssrc;
          DEBUG("seqNumber = %d", seq_number);
        }
        if(packet_count == 0) {
          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->set_frame_type(m_audio_codec_type);
          packet->set_stream_id(m_stream_id);
          packet->set_pts(0LL);
          packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
          packet->set_data_offset(0);
        }
        memcpy(packet->get_data_ptr() + packet_count * rtp_payload_len,
               &read_data[sizeof(AMRtpHeader)], rtp_payload_len);
        ++ packet_count;
        if(packet_count == m_packet_count) {
          packet->set_data_size(rtp_payload_len * packet_count);
          m_packet_queue->push(packet);
          packet = NULL;
          packet_count = 0;
        }
        read_data += sizeof(AMRtpHeader) + rtp_payload_len;
        read_data_len -= sizeof(AMRtpHeader);
        if (read_data_len <= 2 * sizeof(AMRtpHeader)) {
          data_len = read_data_len;
          memmove(m_recv_buf, read_data, data_len);
          break;
        }
      } //end while 1
    }
  }
  if(packet) {
    packet->release();
  }
  INFO("dumuxer_rtp thread exit!");
}

std::string AMDemuxerRtp::sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT format)
{
  std::string format_str = "";
  switch(format) {
    case AM_SAMPLE_U8:
      format_str = "AM_SAMPLE_U8";
      break;
    case AM_SAMPLE_ALAW :
      format_str = "AM_SAMPLE_ALAW";
      break;
    case AM_SAMPLE_ULAW :
      format_str = "AM_SAMPLE_ULAW";
      break;
    case AM_SAMPLE_S16LE :
      format_str = "AM_SAMPLE_S16LE";
      break;
    case AM_SAMPLE_S16BE :
      format_str = "AM_SAMPLE_S16BE";
      break;
    case AM_SAMPLE_S32LE :
      format_str = "AM_SAMPLE_S32LE";
      break;
    case AM_SAMPLE_S32BE :
      format_str = "AM_SAMPLE_S32BE";
      break;
    case AM_SAMPLE_INVALID :
    default :
      format_str = "invalid";
      break;
  }
  return format_str;
}

std::string AMDemuxerRtp::audio_type_to_string(AM_AUDIO_TYPE type)
{
  std::string audio_type = "";
  switch(type) {
    case AM_AUDIO_NULL:
      audio_type = "AM_AUDIO_NULL";
      break;
    case AM_AUDIO_LPCM :
      audio_type = "AM_AUDIO_LPCM";
      break;
    case AM_AUDIO_BPCM :
      audio_type = "AM_AUDIO_BPCM";
      break;
    case AM_AUDIO_G711A :
      audio_type = "AM_AUDIO_G711A";
      break;
    case AM_AUDIO_G711U :
      audio_type = "AM_AUDIO_G711U";
      break;
    case AM_AUDIO_G726_40 :
      audio_type = "AM_AUDIO_G726_40";
      break;
    case AM_AUDIO_G726_32 :
      audio_type = "AM_AUDIO_G726_32";
      break;
    case AM_AUDIO_G726_24 :
      audio_type = "AM_AUDIO_G726_24";
      break;
    case AM_AUDIO_G726_16 :
      audio_type = "AM_AUDIO_G726_16";
      break;
    case AM_AUDIO_AAC :
      audio_type = "AM_AUDIO_AAC";
      break;
    case AM_AUDIO_OPUS :
      audio_type = "AM_AUDIO_OPUS";
      break;
    default :
      audio_type = "invalid";
      break;
  }
  return audio_type;
}
