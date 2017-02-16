/*******************************************************************************
 * am_muxer_rtp.cpp
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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
#include "am_fd.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_rtp_client.h"
#include "am_muxer_rtp.h"
#include "am_rtp_session_if.h"
#include "am_muxer_rtp_config.h"

#include "am_audio_type.h"

#include "am_file.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_thread.h"

#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <algorithm>

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define MUXER_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/muxer")
#else
#define MUXER_CONF_DIR ((const char*)"/etc/oryx/stream/muxer")
#endif

static const char* audio_type_to_string(AM_AUDIO_TYPE type)
{
  const char *name = "";
  switch(type) {
    case AM_AUDIO_NULL:   name = "NULL";       break;
    case AM_AUDIO_LPCM:   name = "PCM";        break;
    case AM_AUDIO_BPCM:   name = "BPCM";       break;
    case AM_AUDIO_G711A:  name = "G.711 Alaw"; break;
    case AM_AUDIO_G711U:  name = "G.711 Ulaw"; break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: name = "G.726";     break;
    case AM_AUDIO_AAC:     name = "AAC";       break;
    case AM_AUDIO_OPUS:    name = "OPUS";      break;
    case AM_AUDIO_SPEEX:   name = "SPEEX";     break;
    default:break;
  }
  return name;
}

static const char* audio_codec_type_to_key(AM_AUDIO_CODEC_TYPE type)
{
  const char *name = "";
  switch(type) {
    case AM_AUDIO_CODEC_NONE:  name = "none"; break;
    case AM_AUDIO_CODEC_AAC:   name = "aac";  break;
    case AM_AUDIO_CODEC_OPUS:  name = "opus"; break;
    case AM_AUDIO_CODEC_LPCM:  name = "pcm";  break;
    case AM_AUDIO_CODEC_BPCM:  name = "bpcm"; break;
    case AM_AUDIO_CODEC_G726:  name = "g726"; break;
    case AM_AUDIO_CODEC_G711:  name = "g711"; break;
    case AM_AUDIO_CODEC_SPEEX: name = "speex"; break;
    default: break;
  }
  return name;
}

static const char* audio_type_to_key(AM_AUDIO_TYPE type)
{
  const char *name = "";
  switch(type) {
    case AM_AUDIO_NULL:   name = "null";  break;
    case AM_AUDIO_LPCM: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_LPCM);
    }break;
    case AM_AUDIO_BPCM: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_BPCM);
    }break;
    case AM_AUDIO_G711A:
    case AM_AUDIO_G711U: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_G711);
    }break;
    case AM_AUDIO_G726_40:
    case AM_AUDIO_G726_32:
    case AM_AUDIO_G726_24:
    case AM_AUDIO_G726_16: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_G726);
    }break;
    case AM_AUDIO_AAC: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_AAC);
    }break;
    case AM_AUDIO_OPUS: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_OPUS);
    }break;
    case AM_AUDIO_SPEEX: {
      name = audio_codec_type_to_key(AM_AUDIO_CODEC_SPEEX);
    }break;
    default:break;
  }
  return name;
}

static const char* proto_to_string(AM_RTP_CLIENT_PROTO proto)
{
  const char *str = "Unknown";
  switch(proto) {
    case AM_RTP_CLIENT_PROTO_RTSP: str = "RTSP"; break;
    case AM_RTP_CLIENT_PROTO_SIP:  str = "SIP";  break;
    default:break;
  }

  return str;
}

AMIMuxerCodec* get_muxer_codec(const char* config)
{
  return (AMIMuxerCodec*)AMMuxerRtp::create(config);
}

AMMuxerRtp* AMMuxerRtp::create(const char *conf_file)
{
  AMMuxerRtp *result = new AMMuxerRtp();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(conf_file)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

AM_STATE AMMuxerRtp::start()
{
  AM_STATE state = AM_STATE_ERROR;
  do {
    m_run = true;
    m_thread_work = AMThread::create("RTP.Work", static_work_thread, this);
    if (AM_UNLIKELY(!m_thread_work)) {
      ERROR("Failed to create RTP work thread!");
      m_run = false;
      break;
    }
    m_thread_server = AMThread::create("RTP.Server",
                                       static_server_thread, this);
    if (AM_UNLIKELY(!m_thread_server)) {
      ERROR("Failed to create RTP server thread!");
      m_run = false;
      break;
    }
    m_event_work->signal();
    while (!m_work_run) {
      usleep(100000);
    }
    m_event_server->signal();
    while(!m_server_run) {
      usleep(100000);
    }
    AUTO_SPIN_LOCK(m_lock);
    m_muxer_state = AM_MUXER_CODEC_RUNNING;
    state = AM_STATE_OK;
  }while(0);

  return state;
}

AM_STATE AMMuxerRtp::stop()
{
  if (AM_LIKELY(AM_MUXER_CODEC_STOPPED != m_muxer_state)) {
    AMRtpClientMsgClose closecmd;
    /* Make server thread exit */
    send_client_msg(CTRL_FD_CLIENT, (uint8_t*)&closecmd, sizeof(closecmd));
    m_run = false; /* Make work thread exit */
    if (AM_LIKELY(AM_MUXER_CODEC_INIT == m_muxer_state)) {
      m_event_work->signal();
      m_event_server->signal();
    }
    m_lock->lock();
    m_muxer_state = AM_MUXER_CODEC_STOPPED;
    m_lock->unlock();
    AM_DESTROY(m_thread_work);
    AM_DESTROY(m_thread_server);
  }

  return AM_STATE_OK;
}

bool AMMuxerRtp::start_file_writing()
{
  WARN("Should not call start file writing function in rtp muxer.");
  return true;
}

bool AMMuxerRtp::stop_file_writing()
{
  WARN("Should not call stop file writing function in rtp muxer.");
  return true;
}

bool AMMuxerRtp::is_running()
{
  return m_run;
}

AM_STATE AMMuxerRtp::set_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMMuxerRtp::get_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

AM_MUXER_ATTR AMMuxerRtp::get_muxer_attr()
{
  return AM_MUXER_NETWORK_NORMAL;
}

AM_MUXER_CODEC_STATE AMMuxerRtp::get_state()
{
  AUTO_SPIN_LOCK(m_lock);
  return m_muxer_state;
}

void AMMuxerRtp::feed_data(AMPacket* packet)
{
  if (AM_LIKELY(packet)) {
    m_packet_q.push(packet);
  }
}

void AMMuxerRtp::kill_client(uint32_t client_ssrc)
{
  m_client_kill_q.push(client_ssrc);
}

AMMuxerRtp::AMMuxerRtp() :
    m_muxer_config(nullptr),
    m_config(nullptr),
    m_lock(nullptr),
    m_event_work(nullptr),
    m_event_server(nullptr),
    m_thread_work(nullptr),
    m_thread_server(nullptr),
    m_run(true),
    m_work_run(false),
    m_server_run(false),
    m_active_kill(false),
    m_muxer_state(AM_MUXER_CODEC_INIT),
    m_rtp_unix_fd(-1),
    m_support_media_type(0)
{
  CTRL_FD_CLIENT = -1;
  CTRL_FD_MUXER  = -1;
  for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
    m_client_proto_fd[i] = -1;
  }
  memset(&m_client_msg, 0, sizeof(m_client_msg));
  m_unix_sock_name.clear();
}

AMMuxerRtp::~AMMuxerRtp()
{
  stop();
  clear_client_kill_queue();
  clear_all_resources();
  delete m_config;
  AM_DESTROY(m_lock);
  AM_DESTROY(m_thread_work);
  AM_DESTROY(m_thread_server);
  AM_DESTROY(m_event_work);
  AM_DESTROY(m_event_server);
  if (AM_LIKELY(m_rtp_unix_fd >= 0)) {
    close(m_rtp_unix_fd);
    unlink(m_unix_sock_name.c_str());
  }
  if (AM_LIKELY(CTRL_FD_CLIENT >= 0)) {
    close(CTRL_FD_CLIENT);
  }
  if (AM_LIKELY(CTRL_FD_MUXER >= 0)) {
    close(CTRL_FD_MUXER);
  }
}

AM_STATE AMMuxerRtp::init(const char *conf)
{
  AM_STATE state = AM_STATE_OK;

  do {
    m_config = new AMMuxerRtpConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to allocate memory for AMuxerRtpConfig!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_muxer_config = m_config->get_config(conf);
    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Failed to get RTP Muxer configration from %s", conf);
      state = AM_STATE_ERROR;
      break;
    }

    m_lock = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock)) {
      ERROR("Failed to create lock!");
      break;
    }

    m_unix_sock_name = RTP_CONTROL_SOCKET;
    m_rtp_unix_fd = rtp_unix_socket(m_unix_sock_name.c_str());
    if (AM_UNLIKELY(m_rtp_unix_fd < 0)) {
      ERROR("Failed to create server thread listening socket!");
      state = AM_STATE_ERROR;
      break;
    }

    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM,
                               IPPROTO_IP, m_client_ctrl_fd) < 0)) {
      PERROR("socketpair");
      state = AM_STATE_ERROR;
      break;
    }

    m_event_work = AMEvent::create();
    if (AM_UNLIKELY(!m_event_work)) {
      ERROR("Failed to create AMEvent for work thread!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_event_server = AMEvent::create();
    if (AM_UNLIKELY(!m_event_server)) {
      ERROR("Failed to create AMEvent for server thread!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
  }while(0);

  return state;
}

void AMMuxerRtp::static_work_thread(void *data)
{
  ((AMMuxerRtp*)data)->work_thread();
}

void AMMuxerRtp::static_server_thread(void *data)
{
  ((AMMuxerRtp*)data)->server_thread();
}

void AMMuxerRtp::work_thread()
{
  m_event_work->wait();
  m_work_run = true;
  m_support_media_type = 0;
  while (m_run) {
    AMPacket *packet = NULL;

    clear_client_kill_queue();
    if (AM_UNLIKELY(m_packet_q.empty())) {
      usleep(10000);
      continue;
    }
    packet = m_packet_q.front();
    m_packet_q.pop();
    if (AM_LIKELY(packet)) {
      switch(packet->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              AM_AUDIO_INFO *ainfo = (AM_AUDIO_INFO*)packet->get_data_ptr();
              const char *key = audio_type_to_key(ainfo->type);
              AMIRtpSession *session = nullptr;
              AMRtpSessionMap::iterator iter = m_session_map.find(key);

              m_support_media_type |= (1 << ainfo->type);
              switch(ainfo->type) {
                case AM_AUDIO_AAC: {
                  session = create_aac_session(
                      key, m_muxer_config->max_send_queue_size, ainfo);
                }break;
                case AM_AUDIO_OPUS:  {
                  session = create_opus_session(
                      key, m_muxer_config->max_send_queue_size, ainfo);
                }break;
                case AM_AUDIO_SPEEX: {
                  session = create_speex_session(
                      key, m_muxer_config->max_send_queue_size, ainfo);
                }break;
                case AM_AUDIO_G726_40:
                case AM_AUDIO_G726_32:
                case AM_AUDIO_G726_24:
                case AM_AUDIO_G726_16: {
                  session = create_g726_session(
                      key, m_muxer_config->max_send_queue_size, ainfo);
                }break;
                case AM_AUDIO_G711A:
                case AM_AUDIO_G711U: {
                  session = create_g711_session(
                      key, m_muxer_config->max_send_queue_size, ainfo);
                }break;
                default: {
                  ERROR("RTP doesn't support audio type %s",
                        audio_type_to_string(ainfo->type));
                }break;
              }
              if (AM_LIKELY(session && (iter == m_session_map.end()))) {
                session->set_config(m_muxer_config);
                m_session_map[key] = session;
                NOTICE("RTP session for %s is created!", key);
                SupportAudioInfo *audio_info = new SupportAudioInfo;
                if (AM_LIKELY(audio_info)) {
                  audio_info->audio_type = ainfo->type;
                  m_rtp_audio_list.push_back(audio_info);
                } else {
                  ERROR("Failed to create SupportAudioInfo struct!");
                }
              } else if (AM_LIKELY(session)) {
                NOTICE("RTP session for %s is already created!", key);
                delete session;
              } else {
                ERROR("Failed to create session for %s!", key);
              }
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              AM_VIDEO_INFO *vinfo = (AM_VIDEO_INFO*)packet->get_data_ptr();
              AMIRtpSession *session = nullptr;
              std::string key = "video" + std::to_string(vinfo->stream_id);
              AMRtpSessionMap::iterator iter = m_session_map.find(key);
              std::string video_name;
              m_support_media_type |= (1 << vinfo->type);
              switch(vinfo->type) {
                case AM_VIDEO_H264: {
                  session = create_h264_session(
                      key.c_str(), m_muxer_config->max_send_queue_size, vinfo);
                  video_name = "H.264";
                }break;
                case AM_VIDEO_H265: /* todo: add H.265 support */ break;
                case AM_VIDEO_MJPEG: {
                  session = create_mjpeg_session(
                      key.c_str(), m_muxer_config->max_send_queue_size, vinfo);
                  video_name = "MJPEG";
                }break;
                default: break;

              }
              if (AM_LIKELY(session && (iter == m_session_map.end()))) {
                session->set_config(m_muxer_config);
                m_session_map[key] = session;
                NOTICE("Rtp session for %s stream%u{%s} is created!",
                       key.c_str(), vinfo->stream_id, video_name.c_str());
                bool find = false;
                for (uint8_t i = 0; i < m_rtp_video_list.size(); ++i) {
                  if (vinfo->type == m_rtp_video_list.at(i)->video_type) {
                    find = true;
                  }
                }
                if (!find) {
                  SupportVideoInfo *video_info = new SupportVideoInfo;
                  if (AM_LIKELY(video_info)) {
                    video_info->video_type = vinfo->type;
                    /* Always use stream ID 1(640*480) */
                    video_info->stream_id = 1; /* vinfo->stream_id */
                    m_rtp_video_list.push_back(video_info);
                  } else {
                    ERROR("Failed to create SupportVideoInfo struct!");
                  }
                }
              } else if (AM_LIKELY(session && m_session_map[key])) {
                if (AM_LIKELY(session->type() == m_session_map[key]->type())) {
                  NOTICE("RTP session for %s is already created!", key.c_str());
                } else {
                  NOTICE("Video%u has change its type from %s to %s!",
                         m_session_map[key]->type_string().c_str(),
                         session->type_string().c_str());
                  delete m_session_map[key];
                  m_session_map[key] = session;
                }
              } else {
                ERROR("Failed to create session for %s!", video_name.c_str());
              }
            }break;
            default: break;
          }
        }break; /* case AMPacket::AM_PAYLOAD_TYPE_INFO: */
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          AM_SESSION_STATE state = AM_SESSION_OK;
          AMRtpSessionMap::iterator iter;
          std::string key;
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              key = audio_type_to_key(AM_AUDIO_TYPE(packet->get_frame_type()));
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              key = "video" + std::to_string(packet->get_stream_id());
            }break;
            default: break;
          }
          iter = m_session_map.find(key);
          if (AM_LIKELY(iter != m_session_map.end())) {
            AMIRtpSession *&session = iter->second;
            state = session->process_data(*packet,
                                          m_muxer_config->max_tcp_payload_size);
          } else {
            state = AM_SESSION_ERROR;
          }
          if (AM_UNLIKELY(state == AM_SESSION_ERROR)) {
            if (AM_LIKELY(iter != m_session_map.end())) {
              ERROR("Error occurs in RTP session %s, delete it!",
                    iter->second->type_string().c_str());
              delete iter->second;
            }
            m_session_map.erase(key);
          }
        }break; /* case AMPacket::AM_PAYLOAD_TYPE_DATA: */
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          std::string key;
          switch(packet->get_attr()) {
            case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
              key = audio_type_to_key(AM_AUDIO_TYPE(packet->get_frame_type()));
            }break;
            case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
              key = "video" + std::to_string(packet->get_stream_id());
            }break;
            default: break;
          }
          delete m_session_map[key];
          m_session_map.erase(key);
          NOTICE("RTP session %s received EOS!", key.c_str());
          if (AM_LIKELY(m_session_map.size() == 0)) {
            NOTICE("All streams are stopped!");
          }
        }break; /* case AMPacket::AM_PAYLOAD_TYPE_EOS: */
        default: {
          ERROR("Unsupported packet type: %u!", packet->get_type());
        }break;
      }
      packet->release();
    }
  }
  clear_client_kill_queue();
  clear_all_resources();
  m_work_run = false;
  NOTICE("RTP Muxer work thread exits!");
}

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
}

void AMMuxerRtp::server_thread()
{
  m_event_server->wait();
  m_server_run = true;
  while(m_server_run) {
    int retval = -1;
    int maxfd = -1;
    int fd = -1;
    AM_NET_STATE ret = AM_NET_OK;
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(CTRL_FD_MUXER, &fdset);
    FD_SET(m_rtp_unix_fd, &fdset);
    maxfd = AM_MAX(CTRL_FD_MUXER, m_rtp_unix_fd);
    for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
      if (AM_LIKELY(is_fd_valid(m_client_proto_fd[i]))) {
        FD_SET(m_client_proto_fd[i], &fdset);
        maxfd = AM_MAX(maxfd, m_client_proto_fd[i]);
      } else if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
        NOTICE("%s is now disconnected!",
               proto_to_string(AM_RTP_CLIENT_PROTO(i)));
        close(m_client_proto_fd[i]);
        m_client_proto_fd[i] = -1;
      }
    }
    if (AM_LIKELY((retval = select(maxfd + 1, &fdset, NULL, NULL, NULL)) > 0)) {
      if (FD_ISSET(CTRL_FD_MUXER, &fdset)) {
        fd = CTRL_FD_MUXER;
      } else if (FD_ISSET(m_rtp_unix_fd, &fdset)) {
        sockaddr_un clientaddr;
        socklen_t len = sizeof(clientaddr);
        uint32_t accept_cnt = 0;
        do {
          fd = accept(m_rtp_unix_fd, (sockaddr*)&clientaddr, &len);
        }while((++ accept_cnt < 5) && (fd < 0) && ((errno == EAGAIN) ||
            (errno == EWOULDBLOCK) || (errno == EINTR)));
        if (AM_UNLIKELY(fd < 0)) {
          PERROR("accept");
        } else {
          NOTICE("Client: %s connected, fd is %d!", clientaddr.sun_path, fd);
        }
      } else {
        for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
          if (AM_LIKELY((m_client_proto_fd[i] >= 0) &&
                        FD_ISSET(m_client_proto_fd[i], &fdset))) {
            NOTICE("Received message from %s(%d)...",
                   proto_to_string(AM_RTP_CLIENT_PROTO(i)),
                                   m_client_proto_fd[i]);
            fd = m_client_proto_fd[i];
            break;
          }
        }
      }
      ret = recv_client_msg(fd, &m_client_msg);
      switch(ret) {
        case AM_NET_OK:
          if (AM_LIKELY(process_client_msg(fd, m_client_msg))) {
            break;
          }
          WARN("Failed to process client message!");
          /* no break */
        case AM_NET_ERROR:
        case AM_NET_PEER_SHUTDOWN:
          if (AM_LIKELY(fd != CTRL_FD_MUXER)) {
            NOTICE("close connection with %s", fd_to_proto_string(fd).c_str());
            close(fd);
          }
          /* no break */
        case AM_NET_BADFD: {
          for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
            if (AM_LIKELY(fd == m_client_proto_fd[i])) {
              m_client_proto_fd[i] = -1;
              break;
            }
          }
        }break;
        default:break;
      }
    } else {
      if (AM_LIKELY(errno != EAGAIN)) {
        bool need_break = false;
        if (AM_LIKELY(errno == EBADF)) {
          for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
            if (AM_LIKELY((m_client_proto_fd[i] >= 0) &&
                          !is_fd_valid(m_client_proto_fd[i]))) {
              NOTICE("%s is now disconnected!",
                     proto_to_string(AM_RTP_CLIENT_PROTO(i)));
              close(m_client_proto_fd[i]);
              m_client_proto_fd[i] = -1;
            }
          }
        } else {
          need_break = true;
          PERROR("select");
        }
        if (AM_LIKELY(need_break)) {
          break;
        }
      }
    }

  } /* while(m_run) */

  close_all_connections();
  m_server_run = false;
  NOTICE("RTP Muxer server thread exits!");
}

void AMMuxerRtp::clear_client_kill_queue()
{
  while(!m_client_kill_q.empty()) {
    uint32_t ssrc = m_client_kill_q.front();
    m_client_kill_q.pop();
    kill_client_by_ssrc(ssrc);
  }
}

void AMMuxerRtp::clear_all_resources()
{
  while(!m_packet_q.empty()) {
    m_packet_q.front()->release();
    m_packet_q.pop();
  }
  for (AMRtpSessionMap::iterator iter = m_session_map.begin();
      iter != m_session_map.end(); ++ iter) {
    delete iter->second;
  }
  m_session_map.clear();

  for (AMRtpAudioList::iterator iter = m_rtp_audio_list.begin();
      iter != m_rtp_audio_list.end(); iter++) {
    if (NULL != *iter) {
      delete *iter;
      *iter = NULL;
    }
  }
  m_rtp_audio_list.clear();
  for (AMRtpVideoList::iterator iter = m_rtp_video_list.begin();
      iter != m_rtp_video_list.end(); iter++) {
    if (NULL != *iter) {
      delete *iter;
      *iter = NULL;
    }
  }
  m_rtp_video_list.clear();

}

void AMMuxerRtp::kill_client_by_ssrc(uint32_t ssrc)
{
  for (AMRtpSessionMap::iterator iter = m_session_map.begin();
      iter != m_session_map.end(); ++ iter) {
    iter->second->del_client(ssrc);
  }
}

void AMMuxerRtp::close_all_connections()
{
  AMRtpClientMsgClose msg_close;
  for (int32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
    if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
      send_client_msg(m_client_proto_fd[i], (uint8_t*)&msg_close,
                      sizeof(msg_close));
      close(m_client_proto_fd[i]);
      m_client_proto_fd[i] = -1;
    }
  }
}

AM_NET_STATE AMMuxerRtp::recv_client_msg(int fd,
                                         AMRtpClientMsgBlock *client_msg)
{
  AM_NET_STATE ret = AM_NET_OK;

  if (AM_LIKELY((fd >= 0) && client_msg)) {
    int read_ret = 0;
    int read_cnt = 0;
    uint32_t received = 0;

    do {
      read_ret = recv(fd, ((uint8_t *)client_msg) + received,
                      sizeof(AMRtpClientMsgBase) - received, 0);
      if (AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    }while((++ read_cnt < 5) &&
           (((read_ret >= 0) && (read_ret < (int)sizeof(AMRtpClientMsgBase))) ||
            ((read_ret < 0) && ((errno == EINTR) ||
                                (errno == EWOULDBLOCK) ||
                                (errno == EAGAIN)))));

    if (AM_LIKELY(received == sizeof(AMRtpClientMsgBase))) {
      AMRtpClientMsg *msg = (AMRtpClientMsg*)client_msg;
      read_ret = 0;
      read_cnt = 0;
      while ((received < msg->length) && (5 > read_cnt ++) &&
             ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                                                     (errno == EWOULDBLOCK) ||
                                                     (errno == EAGAIN))))) {
        read_ret = recv(fd,
                        ((uint8_t *)client_msg) + received,
                        msg->length - received, 0);
        if (AM_LIKELY(read_ret > 0)) {
          received += read_ret;
        }
      }
    }

    if (AM_LIKELY(read_ret <= 0)) {
      if (AM_LIKELY((errno == ECONNRESET) || (read_ret == 0))) {
        NOTICE("%s closed its connection while reading data!",
               fd_to_proto_string(fd).c_str());
        ret = AM_NET_PEER_SHUTDOWN;
      } else {
        if (AM_LIKELY(errno == EBADF)) {
          ret = AM_NET_BADFD;
        } else {
          ret = AM_NET_ERROR;
        }
        ERROR("Failed to receive message(%d): %s!", errno,  strerror(errno));
      }
    }
  }

  return ret;
}

int AMMuxerRtp::rtp_unix_socket(const char *name)
{
  int fd = -1;
  if (AM_LIKELY(name)) {
    bool isok = true;
    do {
      if (AM_UNLIKELY(AMFile::exists(name))) {
        NOTICE("%s already exists! Remove it first!", name);
        if (AM_UNLIKELY(unlink(name) < 0)) {
          PERROR("unlink");
          isok = false;
          break;
        }
      }
      if (AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(name).c_str()))) {
        ERROR("Failed to create path %s", AMFile::dirname(name).c_str());
        isok = false;
        break;
      }

      if (AM_UNLIKELY((fd = socket(AF_UNIX, SOCK_STREAM, IPPROTO_IP)) < 0)) {
        PERROR("socket");
        isok = false;
        break;
      } else {
        struct sockaddr_un un;
        socklen_t socklen = offsetof(struct sockaddr_un, sun_path) +
                            strlen(name);
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        strcpy(un.sun_path, name);

        if (AM_UNLIKELY(bind(fd, (struct sockaddr*)&un, socklen) < 0)) {
          PERROR("bind");
          isok = false;
          break;
        }

        if (AM_UNLIKELY(listen(fd, m_muxer_config->max_proto_client) < 0)) {
          PERROR("listen");
          isok = false;
          break;
        }
      }

    }while(0);

    if (AM_UNLIKELY(!isok)) {
      if (AM_LIKELY(fd >= 0)) {
        close(fd);
        fd = -1;
      }
      if (AM_UNLIKELY(AMFile::exists(name) && (unlink(name) < 0))) {
        PERROR("unlink");
      }
    }
  }
  return fd;
}

bool AMMuxerRtp::process_client_msg(int fd, AMRtpClientMsgBlock &client_msg)
{
  bool ret = true;
  AMRtpClientMsg *msg = (AMRtpClientMsg*)&client_msg;

  switch(msg->type) {
    case AM_RTP_CLIENT_MSG_ACK: {
      AMRtpClientMsgACK &ack = client_msg.msg_ack;
      std::string media(ack.media);
      AMRtpSessionMap::iterator iter;

      std::transform(media.begin(), media.end(), media.begin(), ::tolower);
      iter = m_session_map.find(ack.media);
      if (AM_LIKELY(iter != m_session_map.end())) {
        AMIRtpSession *&session = iter->second;
        AMRtpClient *client = session->find_client_by_ssrc(ack.ssrc);
        if (AM_LIKELY(client)) {
          client->send_ack();
        } else {
          NOTICE("Strange, RTP muxer received ACK of client %08X "
                 "for session %s, but cannot find such client!",
                 ack.ssrc, media.c_str());
        }
      }
    }break;
    case AM_RTP_CLIENT_MSG_SDP: {
      const char *addr = client_msg.msg_sdp.media[AM_RTP_MEDIA_NUM];
      std::string hostip = &addr[5];
      AM_IP_DOMAIN domain = AM_UNKNOWN;
      if (AM_LIKELY(is_str_n_equal(addr, "ipv4", strlen("ipv4")))) {
        domain = AM_IPV4;
      } else if (AM_LIKELY(is_str_n_equal(addr, "ipv6", strlen("ipv6")))) {
        domain = AM_IPV6;
      }
      uint16_t rtp_port[AM_RTP_MEDIA_NUM] = {0};
      memcpy(rtp_port, client_msg.msg_sdp.port, sizeof(rtp_port));
      NOTICE("Received SDP request from %s", hostip.c_str());
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        if (AM_LIKELY(strlen(client_msg.msg_sdp.media[i]))) {
          bool found = false;
          std::string media(client_msg.msg_sdp.media[i]);
          std::string media_name("Unknown");
          AMRtpSessionMap::iterator iter;

          switch(i) {
            case AM_RTP_MEDIA_AUDIO: media_name = "Audio"; break;
            case AM_RTP_MEDIA_VIDEO: media_name = "Video"; break;
            default:break;
          }

          NOTICE("Checking %s media type %s",
                 media_name.c_str(), media.c_str());
          client_msg.msg_sdp.media[i][0] = '\0';
          std::transform(media.begin(), media.end(), media.begin(), ::tolower);
          iter = m_session_map.find(media);

          if (AM_LIKELY(iter != m_session_map.end())) {
            AMIRtpSession *&session = iter->second;
            AM_RTP_MEDIA_TYPE media_type = AM_RTP_MEDIA_NUM;

            switch(session->type()) {
              case AM_SESSION_AAC:
              case AM_SESSION_OPUS:
              case AM_SESSION_G726:
              case AM_SESSION_SPEEX:
              case AM_SESSION_G711: media_type = AM_RTP_MEDIA_AUDIO; break;
              case AM_SESSION_H264:
              case AM_SESSION_H265:
              case AM_SESSION_MJPEG: media_type = AM_RTP_MEDIA_VIDEO; break;
            }
            if (AM_LIKELY(media_type == AM_RTP_MEDIA_TYPE(i))) {
              std::string &sdp = session->get_sdp(hostip, rtp_port[i], domain);
              strncpy(client_msg.msg_sdp.media[i], sdp.c_str(), sdp.size());
              NOTICE("Get session %s SDP:\n%s",
                     iter->first.c_str(),
                     sdp.c_str());
              found = true;
            }
          }
          if (AM_UNLIKELY(!found)) {
            memset(client_msg.msg_sdp.media[i], 0, MAX_SDP_LEN);
            NOTICE("Can NOT find media: %s in %s session!",
                   media.c_str(), media_name.c_str());
          }
        }
      }
      ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_sdp,
                            sizeof(client_msg.msg_sdp));
    }break;
    case AM_RTP_CLIENT_MSG_SDPS: {
      std::string payloads;
      std::string sdp_param;
      std::string sdps;
      for (uint8_t i = 0; i < m_rtp_audio_list.size(); ++i) {
        const char *key = audio_type_to_key(m_rtp_audio_list.at(i)->audio_type);
        if (m_session_map.find(key) != m_session_map.end()) {
          payloads.append(" " + m_session_map[key]->get_payload_type());
          sdp_param.append(m_session_map[key]->get_param_sdp());
        } else {
          ERROR("No matched RTP audio session %s was found!", key);
        }
      }
      sdps = "m=audio " +
             std::to_string(client_msg.msg_sdps.port[AM_RTP_MEDIA_AUDIO]) +
             " RTP/AVP" + payloads + "\r\n" + sdp_param;
      if (client_msg.msg_sdps.enable_video && !m_rtp_video_list.empty()) {
        payloads.clear();
        sdp_param.clear();
        for (uint8_t i = 0; i < m_rtp_video_list.size(); ++i) {
          std::string key = "video" +
              std::to_string(m_rtp_video_list.at(i)->stream_id);
          if (m_session_map.find(key) != m_session_map.end()) {
            payloads.append(" " + m_session_map[key]->get_payload_type());
            sdp_param.append(m_session_map[key]->get_param_sdp());
            sdps.append("m=video " +
               std::to_string(client_msg.msg_sdps.port[AM_RTP_MEDIA_VIDEO]) +
               " RTP/AVP" + payloads + "\r\n" + "a=sendonly\r\n" + sdp_param);
          } else {
            ERROR("No matched RTP video session %s was found!", key.c_str());
          }
        }
      }
      strncpy(client_msg.msg_sdps.media_sdp, sdps.c_str(), MAX_SDP_LEN);
      ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_sdps,
                            sizeof(client_msg.msg_sdps));
      NOTICE("Return the SDPS info to SIP server.");
    }break;
    case AM_RTP_CLIENT_MSG_KILL: {
      if (AM_LIKELY(fd == CTRL_FD_MUXER)) {
        NOTICE("Session is asking to shut down client(SSRC: %08X) "
               "with session id: %08X",
               client_msg.msg_kill.ssrc,
               client_msg.msg_kill.session_id);
        client_msg.msg_kill.active_kill = m_active_kill;
        ret = send_client_msg(client_msg.msg_kill.fd_from,
                              (uint8_t*)&client_msg.msg_kill,
                              sizeof(client_msg.msg_kill));
        m_active_kill = false;
      } else {
        m_active_kill = true;
        kill_client(client_msg.msg_kill.ssrc);
      }
    }break;
    case AM_RTP_CLIENT_MSG_INFO: {
      int client_tcp_fd = -1;
      if (AM_LIKELY(client_msg.msg_info.info.send_tcp_fd)) {
        if (AM_UNLIKELY(!recv_fd(fd, &client_tcp_fd))) {
          ERROR("Failed to get client TCP file descriptor!");
          ret = false;
          break;
        }
      }
      std::string rtp_client_conf =
          std::string(MUXER_CONF_DIR) + "/rtp-client.acs";
      AMRtpClientMsgInfo &msg_info = client_msg.msg_info;
      AMRtpSessionMap::iterator iter =
          m_session_map.find(msg_info.info.media);
      AMIRtpSession *session =
          (iter != m_session_map.end()) ? iter->second : nullptr;
      AMRtpClient *client = session ? AMRtpClient::create(rtp_client_conf,
                                                          fd,
                                                          client_tcp_fd,
                                                          CTRL_FD_CLIENT,
                                                          msg_info.session_id,
                                                          this,
                                                          msg_info.info) :
                                      nullptr;
      NOTICE("Received client information: session_id %08X!",
             msg_info.session_id);
      if (AM_LIKELY(session && client && session->add_client(client))) {
        AMRtpClientMsgControl control(AM_RTP_CLIENT_MSG_SSRC,
                                      client->ssrc(),
                                      msg_info.session_id);
        ret = send_client_msg(fd, (uint8_t*)&control, sizeof(control));
        NOTICE("Send SSRC %08X to client with session_id %08X!",
               client->ssrc(), msg_info.session_id);
      } else {
        if (AM_LIKELY(!session)) {
          ERROR("Cannot find such media type: %s", msg_info.info.media);
        } else if (!client) {
          ERROR("Failed to create client!");
        }
        ret = false;
      }
    }break;
    case AM_RTP_CLIENT_MSG_START: {
      for (AMRtpSessionMap::iterator iter = m_session_map.begin();
           iter != m_session_map.end(); ++ iter) {
        AMIRtpSession *&rtp = iter->second;
        std::string seq_ntp = rtp->get_client_seq_ntp(client_msg.msg_ctrl.ssrc);

        if (AM_LIKELY(!seq_ntp.empty())) {
          NOTICE("Session %s is going to enable client with SSRC %08X",
                 rtp->get_session_name().c_str(), client_msg.msg_ctrl.ssrc);
          strncpy(client_msg.msg_ctrl.seq_ntp, seq_ntp.c_str(), seq_ntp.size());
          ret = send_client_msg(fd, (uint8_t*)&client_msg.msg_ctrl,
                                sizeof(client_msg.msg_ctrl));
          if (AM_LIKELY(ret)) {
            rtp->set_client_enable(client_msg.msg_ctrl.ssrc, true);
          }
          break;
        }
      }
    }break;
    case AM_RTP_CLIENT_MSG_STOP: {
      for (AMRtpSessionMap::iterator iter = m_session_map.begin();
          iter != m_session_map.end(); ++ iter) {
        iter->second->set_client_enable(client_msg.msg_ctrl.ssrc, false);
      }
    }break;
    case AM_RTP_CLIENT_MSG_PROTO: {
      if (AM_LIKELY((client_msg.msg_proto.proto >= 0) &&
                    (client_msg.msg_proto.proto < AM_RTP_CLIENT_PROTO_NUM))) {
        m_client_proto_fd[client_msg.msg_proto.proto] = fd;
        NOTICE("%s server is connected with fd %d!",
               proto_to_string(client_msg.msg_proto.proto),
               m_client_proto_fd[client_msg.msg_proto.proto]);
      } else {
        ERROR("Invalid protocol!");
        ret = false;
      }
    }break;
    case AM_RTP_CLIENT_MSG_CLOSE: {
      if (AM_LIKELY(fd == CTRL_FD_MUXER)) {
        /* This command is from RTP muxer itself, send it to all the client */
        for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
          if (AM_LIKELY(m_client_proto_fd[i] >= 0)) {
            send_client_msg(m_client_proto_fd[i],
                            (uint8_t*)&client_msg.msg_close,
                            sizeof(client_msg.msg_close));
            NOTICE("Sent MessageClose to %s(%d)",
                   fd_to_proto_string(m_client_proto_fd[i]).c_str(),
                   m_client_proto_fd[i]);
          }
        }
      }
      m_server_run = false;
      NOTICE("Received close message!");
    }break;
    case AM_RTP_CLIENT_MSG_SUPPORT: {
      AMRtpClientMsgSupport &support = client_msg.msg_support;
      support.support_media_map = m_support_media_type;
      send_client_msg(fd, (uint8_t*)&client_msg.msg_support,
                      sizeof(client_msg.msg_support));
      NOTICE("Sent support media type %x to sip_ua_server.",
             support.support_media_map);
    }break;
    case AM_RTP_CLIENT_MSG_SSRC:
    default:break;
  }

  return ret;
}

bool AMMuxerRtp::send_client_msg(int fd, uint8_t *data, uint32_t size)
{
  int ret = 0;
  int count = 0;
  uint32_t send_ret = 0;

  do {
    ret = write(fd, data + send_ret, size - send_ret);
    if (AM_UNLIKELY(ret < 0)) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        uint32_t proto = 0;
        for (proto = 0; proto < AM_RTP_CLIENT_PROTO_NUM; ++ proto) {
          if (AM_LIKELY(fd == m_client_proto_fd[proto])) {
            NOTICE("%s has closed its connection!",
                   proto_to_string(AM_RTP_CLIENT_PROTO(proto)));
            break;
          }
        }
        if (AM_LIKELY(proto == AM_RTP_CLIENT_PROTO_NUM)) {
          ERROR("write: %s, fd is %d", strerror(errno), fd);
        }
        break;
      }
    } else {
      send_ret += ret;
    }
  }while((++ count < 5) && ((send_ret > 0) && (send_ret < size)));

  return (send_ret == size);
}

std::string AMMuxerRtp::fd_to_proto_string(int fd)
{
  std::string str = "Unknown";

  if (AM_LIKELY(fd >= 0)) {
    for (uint32_t i = 0; i < AM_RTP_CLIENT_PROTO_NUM; ++ i) {
      if (AM_LIKELY((fd == m_client_proto_fd[i]))) {
        str = proto_to_string(AM_RTP_CLIENT_PROTO(i));
      }
    }
  }

  return str;
}
