/*******************************************************************************
 * am_demuxer_unix_domain.cpp
 *
 * History:
 *   2016-7-18 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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

#include "am_file.h"
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
#include "am_demuxer_unix_domain.h"
#include "rtp.h"
#include "adts.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/stat.h>


int ctrl_unix_domain[2] = { -1 };
#define CTRL_READ_UNIX_DOMAIN    ctrl_unix_domain[0]
#define CTRL_WRITE_UNIX_DOMAIN   ctrl_unix_domain[1]
#define MAX_CONNECTION_NUMBER 1
#define PACKET_POOL_SIZE   48
#define PACKET_CHUNK_SIZE  1024
#define SOCKET_BUFFER_SIZE 8192

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_UNIX_DOMAIN;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerUnixDomain::create(streamid);
}

AMDemuxerUnixDomain::AMDemuxerUnixDomain(uint32_t streamid) :
    inherited(AM_DEMUXER_UNIX_DOMAIN, streamid)
{
}

AMDemuxerUnixDomain::~AMDemuxerUnixDomain()
{
  if (AM_LIKELY(CTRL_READ_UNIX_DOMAIN >= 0)) {
    close(CTRL_READ_UNIX_DOMAIN);
    CTRL_READ_UNIX_DOMAIN = -1;
  }
  if (AM_LIKELY(CTRL_WRITE_UNIX_DOMAIN >= 0)) {
    close(CTRL_WRITE_UNIX_DOMAIN);
    CTRL_WRITE_UNIX_DOMAIN = -1;
  }
  delete   m_audio_info;
  delete   m_packet_queue;
  delete[] m_recv_buf;
  INFO("AMDemuxerUnixDomain has been destroyed!");
}

AMIDemuxerCodec* AMDemuxerUnixDomain::create(uint32_t streamid)
{
  AMDemuxerUnixDomain* demuxer = new AMDemuxerUnixDomain(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = NULL;
  }
  return ((AMIDemuxerCodec*) demuxer);
}

void AMDemuxerUnixDomain::stop()
{
  if (AM_LIKELY(m_run.load() && CTRL_WRITE_UNIX_DOMAIN >= 0)) {
    write(CTRL_WRITE_UNIX_DOMAIN, "s", 1);
    INFO("Write stop cmd to unix domain demuxer mainloop!");
  }
  AM_DESTROY(m_thread);
}

void AMDemuxerUnixDomain::destroy()
{
  enable(false);
  inherited::destroy();
}

void AMDemuxerUnixDomain::enable(bool enabled)
{
  if(!enabled) {
    stop();
  }
  AUTO_MEM_LOCK(m_lock);
  if (AM_LIKELY(m_packet_pool && (m_is_enabled != enabled))) {
    m_is_enabled = enabled;
    m_packet_pool->enable(enabled);
    NOTICE("%s AMDemuxerUnixDomain!", enabled ? "Enable" : "Disable");
  }
}

bool AMDemuxerUnixDomain::is_drained()
{
  return (m_packet_pool->get_avail_packet_num() == PACKET_POOL_SIZE);
}

bool AMDemuxerUnixDomain::is_play_list_empty()
{
  bool ret = (m_connect_fd < 0);
  return ret;
}

bool AMDemuxerUnixDomain::add_uri(const AMPlaybackUri& uri)
{
  bool ret = true;
  do{
    if(uri.type != AM_PLAYBACK_URI_UNIX_DOMAIN) {
      ERROR("uri type error.");
      ret = false;
      break;
    }
    if (m_run.load()) {
      ERROR("The AMDemuxerUnixDomain is running, please stop it first.");
      ret = false;
      break;
    }
    NOTICE("Add uri in AMDemuxerUnixDomain :\n"
           "unix domain name is %s\n"
           "audio type is %s\n"
           "audio sample rate is %d\n"
           "audio channel is %d",
           uri.media.unix_domain.name,
           audio_type_to_string(uri.media.unix_domain.audio_type).c_str(),
           uri.media.unix_domain.sample_rate,
           uri.media.unix_domain.channel);
    const AMPlaybackUnixDomainUri& unix_domain_info = uri.media.unix_domain;
    m_audio_info->sample_rate = unix_domain_info.sample_rate;
    m_audio_info->channels = unix_domain_info.channel;
    m_audio_info->type = unix_domain_info.audio_type;
    m_audio_info->codec_info = nullptr;
    switch (m_audio_info->type) {
      case AM_AUDIO_G711A:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_PCMA;
        m_audio_codec_type = AM_AUDIO_CODEC_G711;
        m_audio_info->sample_format = AM_SAMPLE_ALAW;
        m_audio_info->sample_size = sizeof(int8_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g711A.");
        break;
      case AM_AUDIO_G711U:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_PCMU;
        m_audio_codec_type = AM_AUDIO_CODEC_G711;
        m_audio_info->sample_format = AM_SAMPLE_ULAW;
        m_audio_info->sample_size = sizeof(int8_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g711U.");
        break;
      case AM_AUDIO_G726_16:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_16;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g726_16.");
        break;
      case AM_AUDIO_G726_24:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_24;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g726_24.");
        break;
      case AM_AUDIO_G726_32:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_32;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g726_32.");
        break;
      case AM_AUDIO_G726_40:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_G726_40;
        m_audio_codec_type = AM_AUDIO_CODEC_G726;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        m_audio_info->chunk_size = PACKET_CHUNK_SIZE;
        INFO("Audio type is g726_40.");
        break;
      case AM_AUDIO_OPUS:
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_OPUS;
        m_audio_codec_type = AM_AUDIO_CODEC_OPUS;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
        INFO("Audio type is opus.");
        break;
      case AM_AUDIO_AAC : {
        m_audio_rtp_type = RTP_PAYLOAD_TYPE_AAC;
        m_audio_codec_type = AM_AUDIO_CODEC_AAC;
        m_audio_info->sample_format = AM_SAMPLE_S16LE;
        m_audio_info->sample_size = sizeof(int16_t);
      } break;
      default:
        ERROR("Not support the audio format %d.", m_audio_info->type);
        m_audio_codec_type = AM_AUDIO_CODEC_NONE;
        ret = false;
        break;
    }
    if (!ret) {
      break;
    }
    m_sock_name = uri.media.unix_domain.name;
    if (!create_resource()) {
      ERROR("Failed to create resource.");
      release_resource();
      ret = false;
      break;
    }
    AM_DESTROY(m_thread);
    m_thread = AMThread::create("DemuxerUnixDomain", thread_entry, this);
    if (AM_UNLIKELY(!m_thread)) {
      ERROR("Failed to create thread in DemuxerUnixDomain!");
      ret = false;
      m_thread = nullptr;
      break;
    }
    /* AAC audio codec will be ready when the first data packet is received */
    m_info_ready = (m_audio_codec_type != AM_AUDIO_CODEC_AAC);
  }while(0);
  return ret;
}

bool AMDemuxerUnixDomain::create_resource()
{
  bool ret = true;
  int sock_length = 0;
  struct sockaddr_un un;
  do {
    AUTO_MEM_LOCK(m_sock_lock);
    if (AM_UNLIKELY(m_listen_fd >= 0)) {
      close(m_listen_fd);
      m_listen_fd = -1;
    }
    if(AM_UNLIKELY(((m_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0))) {
      ERROR("Create unix socket domain error.");
      m_listen_fd = -1;
      break;
    }
    if(AMFile::exists(m_sock_name.c_str())) {
      ERROR("The file is exist, please use another file name.");
      ret = false;
      break;
    }
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    if(AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(m_sock_name.c_str()).c_str()))) {
      ERROR("Failed to create %s.", AMFile::dirname(m_sock_name.c_str()).c_str());
      break;
    }
    strcpy(un.sun_path, m_sock_name.c_str());
    sock_length = offsetof(struct sockaddr_un, sun_path) + m_sock_name.size();
    if(AM_UNLIKELY(bind(m_listen_fd, (struct sockaddr*)&un, sock_length) < 0)) {
      ERROR("Unix socket domain bind error");
      PERROR("bind");
      break;
    }
    if(AM_UNLIKELY(listen(m_listen_fd, MAX_CONNECTION_NUMBER) < 0)) {
      ERROR("Unix socket domain listen error.");
      break;
    }
  } while(0);
  return ret;
}

bool AMDemuxerUnixDomain::release_resource()
{
  bool ret = true;
  do {
    m_sock_lock.lock();
    if (m_connect_fd > 0) {
      close(m_connect_fd);
      m_connect_fd = -1;
    }
    if (m_listen_fd > 0) {
      close(m_listen_fd);
      m_listen_fd = -1;
    }
    m_sock_lock.unlock();
    m_info_sent = false;
    m_info_ready = false;
    if (AMFile::exists(m_sock_name.c_str())) {
      if (AM_UNLIKELY(unlink(m_sock_name.c_str()) < 0)) {
        PERROR("unlink");
      }
      m_sock_name.clear();
    }
    while (AM_LIKELY(!m_packet_queue->empty())) {
      m_packet_pool->release(m_packet_queue->front_pop());
    }
  } while(0);
  return ret;
}

AM_DEMUXER_STATE AMDemuxerUnixDomain::get_packet(AMPacket* &packet)
{
  AM_DEMUXER_STATE ret = AM_DEMUXER_OK;
  packet = nullptr;
  if (AM_UNLIKELY(m_info_sent)) {
    if (AM_LIKELY(!m_packet_queue->empty())) {
      packet = m_packet_queue->front_pop();
    } else {
      AUTO_MEM_LOCK(m_sock_lock);
      ret = ((m_connect_fd < 0) || (!m_run.load())) ? AM_DEMUXER_NO_FILE :
          AM_DEMUXER_NO_PACKET;
    }
  } else {
    if(m_info_ready) {
      if (AM_LIKELY(allocate_packet(packet))) {
        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->set_frame_type(m_audio_codec_type);
        packet->set_stream_id(m_stream_id);
        packet->set_pts(0LL);
        packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
        packet->set_data_size(sizeof(AM_AUDIO_INFO));
        memcpy(packet->get_data_ptr(), m_audio_info, sizeof(AM_AUDIO_INFO));
        AM_AUDIO_INFO* audio_info = ((AM_AUDIO_INFO*) packet->get_data_ptr());
        INFO("\nDemuxer RTP info :"
             "\n      channels: %d"
             "\n  sample rate : %d"
             "\n  sample size : %d"
             "\n   chunk size : %d"
             "\nsample format : %s"
             "\n         type : %s",
             audio_info->channels,
             audio_info->sample_rate,
             audio_info->sample_size,
             audio_info->chunk_size,
             sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT(audio_info->
                                                            sample_format)).
                                                            c_str(),
             audio_type_to_string(audio_info->type).c_str());
        if (AM_LIKELY(m_audio_codec_type == AM_AUDIO_CODEC_AAC)) {
          INFO("AAC bit stream type: %s", (const char*)audio_info->codec_info);
        }
        m_info_sent = true;
      } else {
        ERROR("Failed to allocate packet.");
        ret = AM_DEMUXER_NO_PACKET;
      }
    } else {
      ret = AM_DEMUXER_NO_PACKET;
    }
  }

  return ret;
}

AM_STATE AMDemuxerUnixDomain::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(AM_STATE_OK != (state = inherited::init()))) {
      break;
    }
    m_packet_pool = AMFixedPacketPool::create("UnixDemuxerPacketPool",
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
    if (AM_UNLIKELY(pipe(ctrl_unix_domain) < 0)) {
      ERROR("Failed to create pipe!");
      state = AM_STATE_ERROR;
      break;
    }
  } while (0);

  return state;
}

void AMDemuxerUnixDomain::thread_entry(void *p)
{
  ((AMDemuxerUnixDomain*)p)->main_loop();
}

void AMDemuxerUnixDomain::main_loop()
{
  AMPacket* packet = nullptr;
  fd_set read_fd;
  fd_set reset_fd;
  uint32_t data_len = 0;
  //AMRtpHeader* rtp_header = nullptr;
  //uint16_t seq_number = -1;
  char* read_data = nullptr;
  uint32_t read_data_len = 0;
  //uint32_t ssrc = 0;
  int max_fd = -1;
  do {
    m_run = true;
    FD_ZERO(&reset_fd);
    FD_SET(m_listen_fd, &reset_fd);
    FD_SET(CTRL_READ_UNIX_DOMAIN, &reset_fd);
    max_fd = AM_MAX(m_listen_fd, CTRL_READ_UNIX_DOMAIN);
    while (m_run.load()) {
      read_fd = reset_fd;
      int sret = select(max_fd + 1, &read_fd, nullptr, nullptr, nullptr);
      if (AM_UNLIKELY(sret <= 0)) {
        PERROR("select");
        if (errno != EAGAIN) {
          m_run = false;
        }
        continue;
      }
      if (AM_UNLIKELY(FD_ISSET(CTRL_READ_UNIX_DOMAIN, &read_fd))) {
        char cmd[1] = { 0 };
        if (AM_LIKELY(read(CTRL_READ_UNIX_DOMAIN, cmd, sizeof(cmd)) < 0)) {
          PERROR("Read form CTRL_READ_UNIX_DOMAIN");
          continue;
        } else {
          if (cmd[0] == 's') {
            INFO("Receive stop cmd in demuxer unix domain, exit mainloop.");
            m_run = false;
            continue;
          } else {
            ERROR("Receive unvalid cmd : %c in demuxer unix domain.", cmd);
          }
        }
      }
      if (AM_LIKELY(FD_ISSET(m_listen_fd, &read_fd))) {
        if (m_connect_fd > 0) {
          ERROR("Only one connection is supported currently.");
          continue;
        }
        struct sockaddr_un un;
        socklen_t socket_len = sizeof(un);
        if(AM_UNLIKELY((m_connect_fd = accept(m_listen_fd, (struct sockaddr*)&un,
                                              &socket_len)) < 0)) {
          ERROR("Unix socket domain accept error.");
          m_connect_fd =  -1;
          continue;
        }
        FD_SET(m_connect_fd, &reset_fd);
        max_fd = AM_MAX(max_fd, m_connect_fd);
        continue;
      }
      if (AM_LIKELY(FD_ISSET(m_connect_fd, &read_fd))) {
        if (data_len >= SOCKET_BUFFER_SIZE) {
          NOTICE("Receive buf already full, will clear the data in receive buf.");
          data_len = 0;
        }
        int recv_len = recv(m_connect_fd, m_recv_buf + data_len,
                            SOCKET_BUFFER_SIZE - data_len, 0);
        if(AM_UNLIKELY(recv_len < 0)) {
          if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            continue;
          } else {
            PERROR("recv");
            m_run = false;
            break;
          }
        }
        if (recv_len == 0) {
          NOTICE("The unix domain client has closed the connection.");
          m_run = false;
          break;
        }
        data_len += recv_len;
        if (data_len <= 2 * (sizeof(AMRtpHeader) + 8)) {
          continue;
        }
        read_data = m_recv_buf;
        read_data_len = data_len;
        while(true) {
          uint32_t rtp_pkt_size = 0;
          bool is_data_valid = true;
          while (read_data_len > 8) {
            if (AM_LIKELY((read_data[0] == 'R') &&
                          (read_data[1] == 'T') &&
                          (read_data[2] == 'P') &&
                          (read_data[3] == ' '))) {
              break;
            } else {
              INFO("RTP header error.");
              ++ read_data;
              -- read_data_len;
            }
          }
          if (AM_UNLIKELY(read_data_len <= 8)) {
            data_len = read_data_len;
            if (data_len > 0) {
              memmove(m_recv_buf, read_data, data_len);
            }
            break;
          }
          rtp_pkt_size = ((read_data[4] & 0x000000ff) << 24) |
                         ((read_data[5] & 0x000000ff) << 16) |
                         ((read_data[6] & 0x000000ff) << 8)  |
                         ((read_data[7] & 0x000000ff));
          if ((read_data_len - 8) < rtp_pkt_size) {
            data_len = read_data_len;
            memmove(m_recv_buf, read_data, data_len);
            break;
          }
          if(rtp_pkt_size > PACKET_CHUNK_SIZE) {
            ERROR("The length of rtp payload is %d, "
                "the PACKET_CHUNK_SIZE is %d, "
                "the length of rtp payload is larger than "
                "PACKET_CHUNK_SIZE. Exit the demuxer rtp mainloop",
                rtp_pkt_size, PACKET_CHUNK_SIZE);
            m_run = false;
            break;
          }
          /*
          rtp_header = (AMRtpHeader*)(read_data + 8);
          if (((ssrc != 0) && (rtp_header->ssrc != ssrc)) ||
              (ntohs(rtp_header->sequencenumber) == seq_number)) {
            NOTICE("SSRC or seq_number is error, skip this pkt.");
            read_data_len = read_data_len - rtp_pkt_size - 8;
            read_data += (rtp_pkt_size + 8);
            continue;
          } else {
            seq_number = ntohs(rtp_header->sequencenumber);
            ssrc = rtp_header->ssrc;
          }
          */
          if (AM_UNLIKELY(!allocate_packet(packet))){
            NOTICE("Failed to allocate packet.");
            continue;
          }
          read_data_len -= 8;
          read_data += 8;
          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->set_frame_type(m_audio_codec_type);
          packet->set_stream_id(m_stream_id);
          packet->set_pts(0LL);
          packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
          packet->set_data_offset(0);
          switch(m_audio_codec_type) {
            case AM_AUDIO_CODEC_G711:
            case AM_AUDIO_CODEC_OPUS:
            case AM_AUDIO_CODEC_G726: {
              memcpy(packet->get_data_ptr(),
                     &read_data[sizeof(AMRtpHeader)],
                     (rtp_pkt_size - sizeof(AMRtpHeader)));
              packet->set_data_size(rtp_pkt_size - sizeof(AMRtpHeader));
            }break;
            case AM_AUDIO_CODEC_AAC: {
              /* AU_HEADER_SIZE is 16 bits */
              char *au = &read_data[sizeof(AMRtpHeader)];
              char *au_header = &au[2];
              uint16_t frame_number = ((au[0] << 8) | au[1]) / 16;
              char *frame = au_header + frame_number * 2;
              uint8_t *buf = packet->get_data_ptr();
              uint32_t data_size = 0;
              for (uint32_t i = 0; i < frame_number; ++ i) {
                uint32_t frame_len = ((au_header[0] << 8) | au_header[1]) >> 3;
                if (AM_UNLIKELY(!m_info_ready)) {
                  AdtsHeader *adts = (AdtsHeader*)frame;
                  m_audio_info->codec_info = (void*)(adts->is_sync_word_ok() ?
                      "adts" : "raw");
                  m_info_ready = true;
                }
                memcpy(buf, frame, frame_len);
                data_size += frame_len;
                buf += frame_len;
                frame += frame_len;
                au_header += 2;
              }
              packet->set_data_size(data_size);
            }break;
            default: {
              is_data_valid = false;
            }break;
          }
          if (AM_LIKELY(is_data_valid)) {
            m_packet_queue->push(packet);
          } else {
            packet->release();
          }
          packet = nullptr;
          read_data += rtp_pkt_size;
          read_data_len -= rtp_pkt_size;
        }
      }
    }
  } while(0);
  if (!release_resource()) {
    ERROR("Failed to release resource.");
  }
  if(packet) {
    packet->release();
  }
  INFO("dumuxer_rtp thread exit!");
}

std::string AMDemuxerUnixDomain::sample_format_to_string(
                             AM_AUDIO_SAMPLE_FORMAT format)
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

std::string AMDemuxerUnixDomain::audio_type_to_string(AM_AUDIO_TYPE type)
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

