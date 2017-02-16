/*******************************************************************************
 * am_rtsp_client_session.cpp
 *
 * History:
 *   2014-12-29 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "rtp.h"
#include "rtcp.h"
#include "am_rtp_msg.h"
#include "am_fd.h"

#include "am_rtsp_server_config.h"
#include "am_rtsp_authenticator.h"
#include "am_rtsp_client_session.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <queue>
#include <sys/time.h>

#include "am_thread.h"
#include "am_event.h"

#define  SEND_BUF_SIZE             (8*1024*1024)
#define  SOCK_TIMEOUT_SECOND       5
#define  RTSP_PARAM_STRING_MAX     4096
#define  RTCP_TIMEOUT_THRESHOLD    30

#define  CMD_CLIENT_ABORT          'a'
#define  CMD_CLIENT_STOP           'e'

#define  ALLOWED_COMMAND \
  "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER"

const char* media_type_to_str(AM_RTP_MEDIA_TYPE type)
{
  const char *str = "Unknown";
  switch(type) {
    case AM_RTP_MEDIA_AUDIO: str = "audio"; break;
    case AM_RTP_MEDIA_VIDEO: str = "video"; break;
    default: break;
  }
  return str;
}

AMRtspClientSession::MediaInfo::MediaInfo() :
    is_alive(false),
    event_msg_seqntp(NULL),
    event_ssrc(NULL),
    event_kill(NULL),
    ssrc(0),
    stream_id(0)
{
  media.clear();
  seq_ntp.clear();
  sdp.clear();
  memset(udp_port, 0, sizeof(udp_port));
}

AMRtspClientSession::MediaInfo::~MediaInfo()
{
  AM_DESTROY(event_msg_seqntp);
  AM_DESTROY(event_ssrc);
  AM_DESTROY(event_kill);
}

AMRtspClientSession::AMRtspClientSession(AMRtspServer *server,
                                         uint16_t udp_rtp_port) :
  m_authenticator(NULL),
  m_rtsp_server(server),
  m_client_thread(NULL),
  m_event_msg_sdp(NULL),
  m_session_state(CLIENT_SESSION_INIT),
  m_tcp_sock(-1),
  m_client_session_id(0),
  m_rtcp_rr_ssrc(0),
  m_rtcp_rr_interval(0),
  m_identify(0),
  m_client_id(0),
  m_dynamic_timeout_sec(CLIENT_TIMEOUT_THRESHOLD)

{
  memset(&m_client_addr, 0, sizeof(m_client_addr));
  memset(m_client_ctrl_sock, 0xff, sizeof(m_client_ctrl_sock));
  memset(&m_rtcp_rr_last_recv_time, 0, sizeof(timeval));
  m_client_name.clear();
  for (uint16_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
    m_media_info[i].udp_port[PORT_RTP]  = udp_rtp_port + i * 2;
    m_media_info[i].udp_port[PORT_RTCP] = udp_rtp_port + i * 2 + 1;
  }
}

AMRtspClientSession::~AMRtspClientSession()
{
  close_all_socket();
  AM_DESTROY(m_client_thread);
  AM_DESTROY(m_event_msg_sdp);
  delete m_authenticator;
}

bool AMRtspClientSession::init_client_session(int32_t server_tcp_sock)
{
  bool ret = false;
  m_session_state = CLIENT_SESSION_INIT;

  do {
    m_event_msg_sdp = AMEvent::create();
    if (AM_UNLIKELY(!m_event_msg_sdp)) {
      ERROR("Failed to create AMEvent object for SDP message!");
      m_session_state = CLIENT_SESSION_FAILED;
      break;
    }

    for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
      m_media_info[i].event_msg_seqntp = AMEvent::create();
      if (AM_UNLIKELY(!m_media_info[i].event_msg_seqntp)) {
        ERROR("Failed to create AMEvent object for %s seq_ntp string!",
              media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
        m_session_state = CLIENT_SESSION_FAILED;
        break;
      }
      m_media_info[i].event_ssrc = AMEvent::create();
      if (AM_UNLIKELY(!m_media_info[i].event_ssrc)) {
        ERROR("Failed to create AMEvent object for %s client SSRC!",
              media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
        m_session_state = CLIENT_SESSION_FAILED;
        break;
      }
      m_media_info[i].event_kill = AMEvent::create();
      if (AM_UNLIKELY(!m_media_info[i].event_kill)) {
        ERROR("Failed to create AMEvent object for %s client kill command!",
              media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
        m_session_state = CLIENT_SESSION_FAILED;
        break;
      }
    }
    if (AM_UNLIKELY(m_session_state == CLIENT_SESSION_FAILED)) {
      break;
    }

    if (AM_UNLIKELY(pipe(m_client_ctrl_sock) < 0)) {
      PERROR("rtsp client session pipe");
      m_session_state = CLIENT_SESSION_FAILED;
      break;
    }

    if (AM_LIKELY(!m_rtsp_server)) {
      ERROR("Invalid RTSP server!");
      m_session_state = CLIENT_SESSION_FAILED;
      break;
    }

    if (AM_UNLIKELY(server_tcp_sock < 0)) {
      ERROR("Invalid RTSP server socket!");
      m_session_state = CLIENT_SESSION_FAILED;
      break;
    } else {
      uint32_t accept_cnt = 0;
      socklen_t sock_len = sizeof(m_client_addr);

      m_client_id = m_rtsp_server->get_random_number();
      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        m_media_info[i].stream_id = m_rtsp_server->get_random_number();
      }
      m_identify = ((m_client_id & 0x0000ffff) << 16);

      m_client_session_id = m_rtsp_server->get_random_number();
      do {
        m_tcp_sock = accept(server_tcp_sock, (sockaddr*)&(m_client_addr),
                            &sock_len);
      } while ((++ accept_cnt < 5) && (m_tcp_sock < 0) &&
          ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
              (errno == EINTR)));

      if (AM_LIKELY(m_tcp_sock < 0)) {
        PERROR("accept");
        m_session_state = CLIENT_SESSION_FAILED;
        break;
      } else {
        int32_t send_buf = SEND_BUF_SIZE;
        int32_t no_delay = 1;
        timeval timeout = {SOCK_TIMEOUT_SECOND, 0};
        if (AM_UNLIKELY((setsockopt(m_tcp_sock,
                                    IPPROTO_TCP, TCP_NODELAY,
                                    &no_delay, sizeof(no_delay)) != 0))) {
          PERROR("setsockopt");
          m_session_state = CLIENT_SESSION_FAILED;
          break;
        }
        if (AM_UNLIKELY((setsockopt(m_tcp_sock,
                                    SOL_SOCKET, SO_SNDBUF,
                                    &send_buf,sizeof(send_buf)) != 0))) {
          PERROR("setsockopt");
          m_session_state = CLIENT_SESSION_FAILED;
          break;
        }
        if (AM_UNLIKELY((setsockopt(m_tcp_sock,
                                           SOL_SOCKET, SO_RCVTIMEO,
                                           (char*)&timeout,
                                           sizeof(timeout)) != 0))) {
          PERROR("setsockopt");
          m_session_state = CLIENT_SESSION_FAILED;
          break;
        }
        if (AM_UNLIKELY((setsockopt(m_tcp_sock,
                                           SOL_SOCKET, SO_SNDTIMEO,
                                           (char*)&timeout,
                                           sizeof(timeout)) != 0))) {
          PERROR("setsockopt");
          m_session_state = CLIENT_SESSION_FAILED;
          break;
        } else {
          char client_name[128] = {0};
          sprintf(client_name, "%s-%hu", inet_ntoa(m_client_addr.sin_addr),
                  ntohs(m_client_addr.sin_port));
          set_client_name(client_name);

          m_client_thread = AMThread::create(m_client_name,
                                             static_client_thread,
                                             this);
          if (AM_UNLIKELY(!m_client_thread)) {
            ERROR("Failed to create client thread for %s", client_name);
            m_session_state = CLIENT_SESSION_FAILED;
            break;
          }
          m_session_state = CLIENT_SESSION_THREAD_RUN;
          m_authenticator = new AMRtspAuthenticator();
          ret = (NULL != m_authenticator);
          if (AM_UNLIKELY(!m_authenticator)) {
            ERROR("Failed to create RTSP server authenticator!");
          } else if (m_rtsp_server->m_rtsp_config->need_auth) {
            m_authenticator->init(m_rtsp_server->m_rtsp_config->\
                                  rtsp_server_user_db.c_str());
          }
          m_session_state = ret ? CLIENT_SESSION_OK : m_session_state;
        }
      }
    }
  }while(0);

  return ret;
}

void AMRtspClientSession::abort_client()
{
  while (m_session_state == CLIENT_SESSION_INIT) {
    usleep(100000);
  }
  if (AM_LIKELY((m_session_state == CLIENT_SESSION_OK) ||
                 m_session_state == CLIENT_SESSION_THREAD_RUN)) {
    char cmd[1] = {CMD_CLIENT_ABORT};
    int count = 0;
    while ((++ count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1))) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        PERROR("write");
        break;
      }
    }
    NOTICE("Sent abort command to client: %s", m_client_name.c_str());
  } else {
    ERROR("This client is not initialized successfully!");
  }

}

bool AMRtspClientSession::kill_client()
{
  bool ret = true;
  while (m_session_state == CLIENT_SESSION_INIT) {
    usleep(100000);
  }
  switch(m_session_state) {
    case CLIENT_SESSION_OK: {
      char cmd[1] = {CMD_CLIENT_STOP};
      int count = 0;
      while ((++count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1))) {
        ret = false;
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          PERROR("write");
          break;
        }
      }
      if(AM_LIKELY(ret && ( false == m_client_thread->\
          set_priority(AMThread::AM_THREAD_PRIO_HIGHEST)))) {
        /* We need the client session to handle this as soon as possible */
        WARN("Failed to change thread %s to highest priority!");
      }
    }break;
    case CLIENT_SESSION_FAILED: {
      WARN("This client is not initialized successfully!");
    }break;
    case CLIENT_SESSION_STOPPED: {
      NOTICE("This client is already stopped, no need to kill again!");
      ret = true;
    }break;
    default: break;

  }
  return ret;
}

void AMRtspClientSession::set_client_name(const char *name)
{
  m_client_name = name ? name : "";
}

void AMRtspClientSession::static_client_thread(void *data)
{
  ((AMRtspClientSession*)data)->client_thread();
}

void AMRtspClientSession::client_thread()
{
  bool run  = true;
  bool enable_timeout = true;
  int maxfd = -1;
  fd_set allset;
  fd_set fdset;

  char rtsp_req_str[8192] = {0};
  PARSE_STATE state = PARSE_COMPLETE;
  bool ret = true;
  int32_t received = 0;
  int32_t read_ret = 0;
  timeval timeout = {CLIENT_TIMEOUT_THRESHOLD, 0};

  FD_ZERO(&allset);
  FD_SET(m_tcp_sock, &allset);
  FD_SET(CLIENT_CTRL_READ, &allset);
  maxfd = AM_MAX(m_tcp_sock, CLIENT_CTRL_READ);

  INFO("New Client: %s, port %hu\n", inet_ntoa(m_client_addr.sin_addr),
       ntohs(m_client_addr.sin_port));

  while (run) {
    int ret_val = -1;
    timeval *tm = enable_timeout ? &timeout : NULL;
    timeout.tv_sec = m_dynamic_timeout_sec;

    fdset = allset;
    if (AM_LIKELY((ret_val = select(maxfd + 1 , &fdset, NULL, NULL, tm)) > 0)) {
      if (FD_ISSET(CLIENT_CTRL_READ, &fdset)) {
        char      cmd[1] = {0};
        uint32_t read_cnt = 0;
        ssize_t read_ret = 0;
        do {
          read_ret = read(CLIENT_CTRL_READ, &cmd, sizeof(cmd));
        } while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
                 ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                 (errno == EINTR)))));
        if (AM_UNLIKELY(read_ret <= 0)) {
          PERROR("read");
        } else if (cmd[0] == CMD_CLIENT_STOP) {
          NOTICE("Client:%s received stop signal!", m_client_name.c_str());
          run = false;
          teardown();
          close_all_socket();
          break;
        } else if (cmd[0] == CMD_CLIENT_ABORT) {
          NOTICE("Client:%s received abort signal!", m_client_name.c_str());
          break;
        }
      } else if (FD_ISSET(m_tcp_sock, &fdset)) {
        RtspRequest rtsp_req;
        uint32_t read_cnt = 0;
        if (AM_LIKELY(ret && ((state == PARSE_COMPLETE) ||
                              (state == PARSE_RTCP)))) {
          received = 0;
          read_ret = 0;
          memset(rtsp_req_str, 0, sizeof(rtsp_req_str));
        }
        do {
          /* m_tcp_sock is always in blocking mode */
          read_ret = recv(m_tcp_sock, &rtsp_req_str[received],
                          sizeof(rtsp_req_str) - received, 0);
        } while ((++ read_cnt < 5) && ((read_ret == 0) ||
            ((read_ret < 0) && ((errno == EINTR) || (errno == EAGAIN) ||
                (errno == EWOULDBLOCK)))));
        if (AM_LIKELY(read_ret > 0 )) {
          received += read_ret;
          ret = parse_client_request(rtsp_req, state,
                                     rtsp_req_str, received);
          if (AM_LIKELY(ret && (state == PARSE_COMPLETE))) {
            INFO("\nReceived request from client %s, port %hu, "
                 "length %d, request:\n%s",
                 inet_ntoa(m_client_addr.sin_addr),
                 ntohs(m_client_addr.sin_port),
                 received, rtsp_req_str);
            rtsp_req.print_info();
          } else if (AM_LIKELY(ret && (state == PARSE_RTCP))) {
            received = 0;
          } else if (AM_LIKELY(!ret)) {
            NOTICE("\nReceived unknown request from client %s, port %hu, "
                   "length %d, request:\n%s",
                   inet_ntoa(m_client_addr.sin_addr),
                   ntohs(m_client_addr.sin_port),
                   received, rtsp_req_str);
          }
        } else if (AM_UNLIKELY(read_ret <= 0)) {
          if (AM_LIKELY((errno == ECONNRESET) || (read_ret == 0))) {
            if( AM_LIKELY(m_tcp_sock >= 0)) {
              /* socket maybe closed */
              close(m_tcp_sock);
              m_tcp_sock = -1;
            }
            WARN("client closed socket!");
          } else {
            PERROR("recv");
            ERROR("Server is going to close this socket!");
          }
          run = false;
          teardown();
          continue;
        }
        if (AM_UNLIKELY(ret && ((state == PARSE_INCOMPLETE) ||
                               (state == PARSE_RTCP)))) {
          continue;
        }

        /* Handle requests from client */
        if (AM_LIKELY(ret && (PARSE_COMPLETE == state))) {
          if (AM_LIKELY(rtsp_req.command)) {
            if (AM_LIKELY(m_rtsp_server->need_authentication() &&
                          parse_authentication_header(m_rtsp_auth_hdr,
                                                      rtsp_req_str,
                                                      received))) {
              m_rtsp_auth_hdr.print_info();
            }
            if (AM_LIKELY(is_str_equal(rtsp_req.command, "DESCRIBE"))) {
              AMRtpClientMsgSDP sdp_msg;
              sockaddr_in host_ip;
              memset(&host_ip, 0, sizeof(host_ip));
              char full_ip[256] = {0};
              sprintf(full_ip, "%s:%s", "IPV4",
                      m_rtsp_server->get_source_addr(m_client_addr, host_ip) ?
                          inet_ntoa(host_ip.sin_addr) : "0.0.0.0");
              for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                if (AM_LIKELY(rtsp_req.media[i])) {
                  m_media_info[i].media = rtsp_req.media[i];
                  strcpy(sdp_msg.media[i], (char*)rtsp_req.media[i]);
                }
              }
              strcpy(sdp_msg.media[AM_RTP_MEDIA_HOSTIP], full_ip);
              sdp_msg.session_id = m_identify;
              sdp_msg.length = sizeof(sdp_msg);

              /* Send query SDP message to RTP muxer*/
              if (AM_UNLIKELY(!m_rtsp_server->send_rtp_control_data(
                  (uint8_t*)&sdp_msg, sizeof(sdp_msg)))) {
                ERROR("Send message to RTP muxer error!");
              } else {
                INFO("Send SDP messages to RTP muxer.");
                if (AM_LIKELY(m_event_msg_sdp->wait(5000))) {
                  NOTICE("RTP muxer has returned SDP info!");
                } else {
                  ERROR("Failed to get SDP info from RTP muxer!");
                  m_media_info[AM_RTP_MEDIA_AUDIO].sdp.clear();
                  m_media_info[AM_RTP_MEDIA_VIDEO].sdp.clear();
                }
              }
            } else if (AM_LIKELY(is_str_equal(rtsp_req.command, "SETUP"))) {
              if (AM_LIKELY(parse_transport_header(m_rtsp_trans_hdr,
                                                   rtsp_req_str,
                                                   received))) {
                AMRtpClientMsgInfo client_info;
                AMRtpClientInfo &info = client_info.info;
                MediaInfo *media_info = NULL;

                /* Only enable timeout in TCP mode,
                 * In UDP mode, client is handled by RTP muxer
                 */
                info.client_rtp_mode = m_rtsp_trans_hdr.mode;
                enable_timeout = (m_rtsp_trans_hdr.mode == AM_RTP_TCP);
                /* todo: Add IPv6 support */
                info.client_ip_domain = AM_IPV4;//default
                info.tcp_channel_rtp  = m_rtsp_trans_hdr.rtp_channel_id;
                info.tcp_channel_rtcp = m_rtsp_trans_hdr.rtcp_channel_id;
                info.send_tcp_fd = true;
                client_info.session_id = m_identify;
                for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                  if (AM_LIKELY(rtsp_req.media[i])) {
                    info.port_srv_rtp  = m_media_info[i].udp_port[PORT_RTP];
                    info.port_srv_rtcp = m_media_info[i].udp_port[PORT_RTCP];
                    client_info.session_id =
                        (m_identify | m_media_info[i].stream_id);
                    strcpy(info.media, rtsp_req.media[i]);
                    media_info = &m_media_info[i];
                    break;
                  }
                }
                /* client UDP RTP address info */
                memcpy(&info.udp.rtp.ipv4, &m_client_addr,
                       sizeof(m_client_addr));
                info.udp.rtp.ipv4.sin_port   =
                    htons(m_rtsp_trans_hdr.client_rtp_port);
                /* client UDP RTCP address info */
                memcpy(&info.udp.rtcp.ipv4, &m_client_addr,
                       sizeof(m_client_addr));
                info.udp.rtcp.ipv4.sin_port =
                    htons(m_rtsp_trans_hdr.client_rtcp_port);
                /* client TCP address info */
                memcpy(&info.tcp.ipv4, &m_client_addr, sizeof(m_client_addr));
                client_info.print();

                /* Send RTP client info message to RTP muxer */
                if (AM_UNLIKELY(!m_rtsp_server->send_rtp_control_data(
                    (uint8_t*)&client_info, sizeof(client_info)))) {
                  ERROR("Send message to RTP muxer error!");
                } else {
                  INFO("Send INFO messages to RTP muxer.");
                }

                /* Send RTSP TCP fd to RTP muxer */
                if (AM_UNLIKELY(!send_fd(m_rtsp_server->m_unix_sock_fd,
                                         m_tcp_sock))) {
                  ERROR("Failed to send RTSP TCP fd");
                } else {
                  if (AM_LIKELY(media_info)) {
                    if (AM_LIKELY(media_info->event_ssrc->wait(5000))) {
                      NOTICE("RTP Muxer has returned %s RTP client SSRC %08X!",
                             media_info->media.c_str(),
                             media_info->ssrc);
                    } else {
                      ERROR("Failed to get SSRC for %s RTP client!",
                            media_info->media.c_str());
                    }
                  }
                }
                m_rtsp_trans_hdr.print_info();
              }
            } else if (AM_LIKELY(is_str_equal(rtsp_req.command, "PLAY"))) {
              for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                if (AM_LIKELY(m_media_info[i].ssrc > 0)) {
                  AMRtpClientMsgControl ctrl_msg(AM_RTP_CLIENT_MSG_START,
                                                 m_media_info[i].ssrc,
                                                 (m_identify |
                                                  m_media_info[i].stream_id));

                  if (AM_UNLIKELY(!m_rtsp_server->send_rtp_control_data(
                      (uint8_t*)&ctrl_msg, sizeof(ctrl_msg)))) {
                    ERROR("Send control start msg to RTP muxer error: %s",
                          strerror(errno));
                  } else {
                    INFO("Send start messages to RTP muxer.");
                    if (AM_LIKELY(m_media_info[i].\
                                  event_msg_seqntp->wait(5000))) {
                      NOTICE("RTP Muxer has returned %s RTP packet seq ntp!",
                             media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
                      m_media_info[i].is_alive = true;
                    } else {
                      ERROR("Failed to get video RTP packet seq ntp!");
                      m_media_info[i].seq_ntp.clear();
                    }
                  }
                }
              }
            }
          } else {
            /* Received NULL command, possible junk data */
            WARN("Received NULL request from client!");
            for (int32_t i = 0;
                 (i < received) && ((size_t)i < sizeof(rtsp_req_str));
                 ++ i) {
              printf("%c", rtsp_req_str[i]);
            }
            printf("\n");
            rtsp_req.set_command((char*)"BAD_REQUEST");
          }
        } else {
          /* Fake a BAD_REQUEST command */
          rtsp_req.set_command((char*)"BAD_REQUEST");
        }
        if (AM_UNLIKELY((!m_rtsp_server->m_unix_sock_con) &&
                        (m_rtsp_server->m_unix_sock_fd < 0))) {
          /* NOT connected to RTP muxer and fake a BAD_REQUEST command */
          rtsp_req.set_command((char*)"BAD_REQUEST");
          WARN("RTSP Server has not connected to RTP muxer!");
        }
        run = handle_client_request(rtsp_req);
      }
    } else if ((ret_val == 0)) {
      WARN("Client: %s is not responding within %d seconds, shutdown!",
           m_client_name.c_str(), m_dynamic_timeout_sec);
      run = false;
      teardown();
      close_all_socket();
    } else {
      PERROR("select");
      run = false;
    }
  }
  m_session_state = CLIENT_SESSION_STOPPED;
   if (AM_LIKELY(!run)) {
     int count = 0;
     bool val  = false;
     while ((++count < 5) &&
            !(val = m_rtsp_server->delete_client_session(m_identify))) {
       usleep(5000);
     }
     if (AM_UNLIKELY((count >= 5) && !val)) {
       WARN("RtspServer is dead before client session exit!");
     }
   }
}

void AMRtspClientSession::close_all_socket()
{
  if(AM_LIKELY(m_tcp_sock >= 0)) {
    close(m_tcp_sock);
    m_tcp_sock = -1;
  }

  for (uint32_t i = 0; i < (sizeof(m_client_ctrl_sock)/sizeof(int32_t)); ++ i) {
    if (AM_LIKELY(m_client_ctrl_sock[i] >= 0)) {
      close(m_client_ctrl_sock[i]);
      m_client_ctrl_sock[i] = -1;
    }
  }
}

bool AMRtspClientSession::parse_transport_header(RtspTransHeader& header,
                                                 char            *req_str,
                                                 uint32_t          req_len)
{
  bool ret = false;

  if (AM_LIKELY(req_str && (req_len > 0))) {
    char *req = req_str;
    char *field = NULL;
    int32_t len = req_len;

    while (len >= 11) {
      if (AM_UNLIKELY(is_str_n_equal(req, "Transport: ", 11))) {
        break;
      }
      ++ req;
      -- len;
    }
    req += 11;
    len -= 11;
    field = ((len > 0) && (strlen(req) > 0)) ? new char[strlen(req) + 1] : NULL;
    if (AM_LIKELY(field)) {
      int32_t rtp_port, rtcp_port;
      int32_t rtp_channel, rtcp_channel;
      int32_t ttl;
      header.rtp_channel_id  = 0xff;
      header.rtcp_channel_id = 0xff;
      header.client_dst_ttl  = 255;
      while (sscanf(req, "%[^;]", field) == 1) {
        INFO("FIELD: %s", field);
        if (is_str_same(field, "RTP/AVP/TCP")) {
          header.mode = AM_RTP_TCP;
          header.set_streaming_mode_str(field);
        } else if (is_str_same(field, "RAW/RAW/UDP") ||
            is_str_same(field, "MP2T/H2221/UDP")) {
          header.mode = AM_RAW_UDP;
          header.set_streaming_mode_str(field);
        } else if (is_str_same(field, "RTP/AVP") ||
            is_str_same(field, "RTP/AVP/UDP")) {
          header.mode = AM_RTP_UDP;
          header.set_streaming_mode_str(field);
        } else if (is_str_same(field, "destination=")) {
          header.set_client_dst_addr_str(&field[12]);
        } else if (sscanf(field, "ttl%d", &ttl) == 1) {
          header.client_dst_ttl = ttl;
        } else if (sscanf(field, "client_port=%d-%d",
                          &rtp_port, &rtcp_port) == 2) {
          header.client_rtp_port  = (uint16_t)rtp_port;
          header.client_rtcp_port = (uint16_t)rtcp_port;
        } else if (sscanf(field, "client_port=%d", &rtp_port) == 1) {
          header.client_rtp_port = (uint16_t)rtp_port;
          header.client_rtcp_port = (header.mode == AM_RAW_UDP) ?
              0 : header.client_rtp_port + 1;
        } else if (sscanf(field, "interleaved=%d-%d",
                          &rtp_channel, &rtcp_channel) == 2) {
          header.rtp_channel_id = rtp_channel;
          header.rtcp_channel_id = rtcp_channel;
        }

        req += strlen(field);
        len -= strlen(field);
        while ((len > 0) && (*req == ';')) {
          ++ req;
          -- len;
        }
        if ((*req == '\0') || (*req == '\r') || (*req == '\n')) {
          break;
        }
      }
      delete[] field;
      ret = true;
    }
  }

  return ret;
}

bool AMRtspClientSession::parse_authentication_header(RtspAuthHeader& header,
                                                      char           *req_str,
                                                      uint32_t         req_len)
{
  bool ret = false;

  if (AM_LIKELY(req_str && (req_len > 0))) {
    int len = req_len;
    char* req = req_str;
    char parameter[128] = {0};
    char value[128] = {0};

    while (len >= 22) {
      if (AM_UNLIKELY(is_str_n_equal(req, "Authorization: Digest ", 22))) {
        break;
      }
      ++ req;
      -- len;
    }
    req += 22;
    len -= 22;
    /* Skip all white space */
    while ((len > 0) && ((*req == ' ') || (*req == '\t'))) {
      ++ req;
      -- len;
    }
    while (len > 0) {
      memset(parameter, 0, sizeof(parameter));
      memset(value, 0, sizeof(value));
      if (AM_UNLIKELY((sscanf(req, "%[^=]=\"%[^\"]\"", parameter, value) != 2)&&
                      (sscanf(req, "%[^=]=\"\"", parameter) != 1))) {
        break;
      }
      if (AM_LIKELY(is_str_equal(parameter, "username"))) {
        header.set_username(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "realm"))) {
        header.set_realm(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "nonce"))) {
        header.set_nonce(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "uri"))) {
        header.set_uri(value);
      } else if (AM_LIKELY(is_str_equal(parameter, "response"))) {
        header.set_response(value);
      }
      req += strlen(parameter) + 2 /* =" */ + strlen(value) + 1 /* " */;
      len -= strlen(parameter) + 2 /* =" */ + strlen(value) + 1 /* " */;
      while ((len > 0) && ((*req == ',') || (*req == ' ') || (*req == '\t'))) {
        ++ req;
        -- len;
      }
    }

    ret = (header.realm && header.nonce &&
           header.username && header.uri && header.response);
  }

  return ret;
}

void AMRtspClientSession::handle_rtcp_packet(char *rtcp)
{
  if (AM_LIKELY(rtcp)) {
    AMRtcpHeader *rtcp_hdr = (AMRtcpHeader*)rtcp;
    switch(rtcp[1]) {
      case AM_RTCP_RR: { /* Receiver Report */
        timeval rtcp_recv_time;
        gettimeofday(&rtcp_recv_time, NULL);
        uint32_t  rtcp_cur_ssrc = rtcp_hdr->get_ssrc();
        if (AM_UNLIKELY(m_rtcp_rr_ssrc == 0)) {
          m_rtcp_rr_ssrc = rtcp_cur_ssrc;
        }
        if (AM_LIKELY(rtcp_cur_ssrc == m_rtcp_rr_ssrc)) {
          if (AM_LIKELY((m_rtcp_rr_last_recv_time.tv_sec > 0) &&
                 (rtcp_recv_time.tv_sec > m_rtcp_rr_last_recv_time.tv_sec))) {
            m_rtcp_rr_interval = rtcp_recv_time.tv_sec -
                                               m_rtcp_rr_last_recv_time.tv_sec;
          }
          memcpy(&m_rtcp_rr_last_recv_time, &rtcp_recv_time, sizeof(timeval));
          m_dynamic_timeout_sec = m_rtcp_rr_interval ?
              (m_rtcp_rr_interval * 3) : RTCP_TIMEOUT_THRESHOLD;
          INFO("RTCP Dynamic TimeOut Sec is %d", m_dynamic_timeout_sec);
        }
      } break;
      default:
        /* Handle other RTCP packet */
        break;
    }
  }
}

bool AMRtspClientSession::parse_client_request(RtspRequest     &request,
                                               PARSE_STATE     &state,
                                               char*            req_str,
                                               uint32_t         req_len)
{
  char result[RTSP_PARAM_STRING_MAX] = {0};
  uint32_t cnt  = 0;
  bool succeed = false;
  state = PARSE_INCOMPLETE;

  while ((cnt < req_len) && (req_str[cnt] == '$')) {
    /* This is a RTCP packet over TCP! */
    if ((req_len - cnt) <= sizeof(AMRtpTcpHeader)) {
      /* this RTCP packet is in-complete  */
      return true;
    } else {
      uint16_t rtcp_pkt_size = (req_str[cnt + 2] << 8) | req_str[cnt + 3];
      if ((req_len - cnt - sizeof(AMRtpTcpHeader)) < rtcp_pkt_size) {
        /* RTCP packet is in-complete */
        return true;
      } else {
        char rtcp[rtcp_pkt_size];
        memset(rtcp, 0, sizeof(rtcp));
        memcpy(rtcp, &req_str[cnt + 4], rtcp_pkt_size);
        handle_rtcp_packet(rtcp);
        /* Skip over RTCP packet */
        cnt += (sizeof(AMRtpTcpHeader) + rtcp_pkt_size);
      }
    }
  }
  if (cnt >= req_len) {
    /* Only RTCP packet is received */
    state = PARSE_RTCP;
    return true;
  }

  for (; cnt < req_len && cnt < RTSP_PARAM_STRING_MAX; ++ cnt) {
    if (AM_UNLIKELY((req_str[cnt] == ' ') || (req_str[cnt] == '\t'))) {
      succeed = true;
      break;
    }
    result[cnt] = req_str[cnt];
  }
  result[cnt] = '\0';
  if (AM_LIKELY(succeed)) {
    request.set_command(result);
  } else {
    return false;
  }

  /* skip over any additional white space */
  while ((cnt < req_len) && ((req_str[cnt] == ' ') || (req_str[cnt] == '\t'))) {
    ++ cnt;
  }

  /* Skip over rtsp://url:port/ */
  for (; cnt < (req_len - 8); ++ cnt) {
    if (AM_LIKELY((0 == strncasecmp(&req_str[cnt], "rtsp://", 7)) ||
                  (0 == strncasecmp(&req_str[cnt], "*", 1)))) {
      if (AM_LIKELY(0 == strncasecmp(&req_str[cnt], "rtsp://", 7))) {
        cnt += 7;
        while ((cnt < req_len) && (req_str[cnt] != '/') &&
               (req_str[cnt] != ' ') && (req_str[cnt] != '\t')) {
          ++ cnt;
        }
      } else {
        cnt += 1;
      }
      if (AM_UNLIKELY(cnt == req_len)) {
        return false;
      }
      break;
    }
  }
  succeed = false;

  for (uint32_t i = cnt; i < (req_len - 5); ++ i) {
    if (AM_LIKELY(0 == strncmp(&req_str[i], "RTSP/", 5))) {
      uint32_t lineEnd = i + strlen("RTSP/"); /* Skip RTSP/ */
      uint32_t endMark = 0;
      char    endChar = 0;
      while ((lineEnd < req_len) &&
          (req_str[lineEnd] != '\n')) { /* Find line end */
        ++ lineEnd;
      }
      /* Found "RTSP/", back skip all white spaces */
      -- i;
      while ((i > cnt) && ((req_str[i] == ' ') || (req_str[i] == '\t') ||
          (req_str[i] == '/'))) {
        -- i;
      }
      if (AM_UNLIKELY(i <= cnt)) {
        /* No Suffix found */
        succeed = true;
        break;
      } else {
        endMark = ++ i;
        endChar = req_str[endMark];
        req_str[endMark] = '\0';
      }

      while ((i > cnt) && (req_str[i] != '/')) {
        -- i;
      }
      request.set_url_suffix(&req_str[i + 1]);
      req_str[endMark] = endChar; /* Reset the string end */
      if (i > cnt) {
        /* Go on looking for pre-suffix */
        endMark = i;
        endChar = req_str[endMark];
        req_str[i] = '\0';
        while ((i > cnt) && (req_str[i] != '/')) {
          -- i;
        }
        request.set_url_pre_suffix(&req_str[i + 1]);
        req_str[endMark] = endChar; /* Reset the string end */
      }
      succeed = true;
      cnt = lineEnd + 1;
      break;
    }
  }

  if (AM_UNLIKELY(!succeed)) {
    return false;
  }
  if (AM_LIKELY(request.url_suffix)) {
    char suffix[strlen(request.url_suffix) + 2];
    char field[strlen(request.url_suffix) + 1];
    char *url_suffix = NULL;
    bool got_media = false;

    memset(suffix, 0, sizeof(suffix));
    strcpy(suffix, request.url_suffix);
    suffix[strlen(request.url_suffix)] = ','; /* add a terminator */

    url_suffix = suffix;
    do {
      int field_num = 0;

      memset(field, 0, sizeof(field));
      field_num = sscanf(url_suffix, "%[^,.:- \n]%*[,.:- \n]", field);

      if (AM_LIKELY(1 == field_num)) {
        char type[strlen(field) + 1];
        char name[strlen(field) + 1];
        memset(type, 0, sizeof(type));
        memset(name, 0, sizeof(name));

        if (AM_LIKELY(2 == sscanf(field, "%[^=]%*[=]%[^ \n]", type, name))) {
          INFO("Parsed media: %s, type: %s!", type, name);
          if (AM_LIKELY(is_str_n_equal(type, "video", 5))) {
            request.set_media(AM_RTP_MEDIA_VIDEO, name);
            got_media = true;
          } else if (AM_LIKELY(is_str_n_equal(type, "audio", 5))) {
            request.set_media(AM_RTP_MEDIA_AUDIO, name);
            got_media = true;
          } else {
            ERROR("Unrecognized media: %s, type: %s!", type, name);
          }
        } else if (is_str_n_equal(field, "stream", 6)) {
          uint32_t stream_id = strtoul(&field[6], NULL, 10);
          RtspStreamList &streams = m_rtsp_server->m_rtsp_config->stream;
          if (AM_LIKELY(stream_id < streams.size())) {
            request.set_media(AM_RTP_MEDIA_VIDEO,
                              (char*)streams[stream_id].video.c_str());
            request.set_media(AM_RTP_MEDIA_AUDIO,
                              (char*)streams[stream_id].audio.c_str());
            got_media = true;
          } else {
            ERROR("Invalid request! No such stream: %s", field);
          }
        } else {
          ERROR("Invalid request! Cannot recognize: %s", field);
        }
        url_suffix += strlen(field);
      }
      while(((size_t)(url_suffix - suffix) < strlen(suffix)) &&
            ((url_suffix[0] == ',') || (url_suffix[0] == '.') ||
             (url_suffix[0] == ':') || (url_suffix[0] == '-') ||
             (url_suffix[0] == ' ') || (url_suffix[0] == '\n'))) {
        ++ url_suffix;
      }
    }while((size_t)(url_suffix - suffix) < strlen(suffix));

    succeed = got_media;
    if (AM_UNLIKELY(!succeed)) {
      ERROR("Invalid URL suffix: %s", request.url_suffix);
    }
  }
  if (!succeed) {
    return false;
  }
  /* Find CSeq */
  succeed = false;
  for (; cnt < (req_len - 5); ++ cnt) {
    if (AM_LIKELY(0 == strncasecmp(&req_str[cnt], "CSeq:", 5))) {
      cnt += 5;

      /* Skip all additional white spaces */
      while ((cnt < req_len) &&
             ((req_str[cnt] == ' ') || (req_str[cnt] == '\t'))) {
        ++ cnt;
      }

      for (uint32_t j = cnt; j < req_len; ++ j) {
        if (AM_LIKELY((req_str[j] == '\r') || (req_str[j] == '\n'))) {
          char temp = req_str[j];
          req_str[j] = '\0';
          if (AM_LIKELY(strlen(&req_str[cnt]) > 0)) {
            request.set_cseq(&req_str[cnt]);
            req_str[j] = temp;
            succeed = true;
            state = PARSE_COMPLETE;
          }
          break;
        }
      }
      break;
    }
  }

  if (AM_UNLIKELY(!succeed)) {
    NOTICE("Received incomplete client request!");
  }

  return true;
}

bool AMRtspClientSession::handle_client_request(RtspRequest& request)
{
  char response_buf[8192] = {0};
  bool ret = false;
  if (is_str_same((const char*)request.command, "OPTIONS")) {
    ret = cmd_options(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "DESCRIBE")) {
    ret = cmd_describe(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "SETUP")) {
    ret = cmd_setup(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "TEARDOWN")) {
    ret = cmd_teardown(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "PLAY")) {
    ret = cmd_play(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "GET_PARAMETER")) {
    ret = cmd_get_parameter(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "SET_PARAMETER")) {
    ret = cmd_set_patameter(request, response_buf, sizeof(response_buf));
  } else if (is_str_same((const char*)request.command, "BAD_REQUEST")) {
    ret = cmd_bad_requests(request, response_buf, sizeof(response_buf));
  } else {
    ret = cmd_not_supported(request, response_buf, sizeof(response_buf));
  }

  INFO("\nServer respond to %s:\n%s", m_client_name.c_str(), response_buf);
  if (AM_UNLIKELY((m_tcp_sock >= 0) && ((ssize_t)strlen(response_buf) !=
      write(m_tcp_sock, response_buf, strlen(response_buf))))) {
    if (AM_LIKELY(errno == ECONNRESET)) {
      WARN("Client closed socket!");
    } else {
      PERROR("write");
    }
    ret = false;
  }
  return ret;
}

bool AMRtspClientSession::cmd_options(RtspRequest& req,
                                      char *buf,
                                      uint32_t size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 200 OK\r\n"
                      "CSeq: %s\r\n%s"
                      "Public: %s\r\n\r\n",
           req.cseq, get_date_string(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

bool AMRtspClientSession::cmd_describe(RtspRequest& req,
                                       char *buf,
                                       uint32_t size)
{
  bool ret = true;
  if (AM_LIKELY(authenticate_ok(req, buf, size))) {
    ret = false;
    char rtspurl[1024]   = {0};
    std::string sdp;
    std::string video;
    std::string audio;
    std::string stream_name;

    video.clear();
    audio.clear();
    stream_name.clear();

    for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
      if (AM_LIKELY(!m_media_info[i].media.empty())) {
        std::string type(media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
        stream_name.append(type).append("=").append(m_media_info[i].media);
        if (AM_LIKELY((i < (AM_RTP_MEDIA_NUM - 1)) &&
                      !m_media_info[i + 1].media.empty())) {
          stream_name.append(",");
        }
      }
    }

    if (AM_LIKELY(!stream_name.empty())) {
      m_rtsp_server->get_rtsp_url(stream_name.c_str(),
                                  rtspurl,
                                  sizeof(rtspurl),
                                  m_client_addr);
      sdp = get_sdp();
      if (AM_LIKELY(!sdp.empty())) {
        char date[200] = {0};
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
                 "%s"
                 "Content-Base: %s\r\n"
                 "Content-Type: application/sdp\r\n"
                 "Content-Length: %u\r\n\r\n"
                 "%s",
                 req.cseq,
                 get_date_string(date, sizeof(date)),
                 rtspurl,
                 sdp.size(),
                 sdp.c_str());
        ret = true;
      }
    }
    if (AM_UNLIKELY(sdp.empty())) {
      not_found(req, buf, size);
    }
  }

  return ret;
}

bool AMRtspClientSession::cmd_setup(RtspRequest& req,
                                    char *buf,
                                    uint32_t size)
{
  bool ret = true;
  if (AM_LIKELY(authenticate_ok(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      char date[200] = {0};
      sockaddr_in host_ip;
      memset(&host_ip, 0, sizeof(sockaddr_in));
      if (m_rtsp_trans_hdr.mode == AM_RTP_UDP) {
        uint16_t port_rtp = 0;
        uint16_t port_rtcp = 0;
        for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
          if (AM_LIKELY(req.media[i])) {
            port_rtp  = m_media_info[i].udp_port[PORT_RTP];
            port_rtcp = m_media_info[i].udp_port[PORT_RTCP];
            break;
          }
        }
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: %s;unicast;destination=%s;source=%s;"
                 "client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                 "Session: %08x\r\n\r\n",
                 req.cseq,
                 get_date_string(date, sizeof(date)),
                 m_rtsp_trans_hdr.mode_string,
                 inet_ntoa(m_client_addr.sin_addr),
                 m_rtsp_server->get_source_addr(m_client_addr, host_ip) ?
                                                inet_ntoa(host_ip.sin_addr) :
                                                "0.0.0.0",
                 m_rtsp_trans_hdr.client_rtp_port,
                 m_rtsp_trans_hdr.client_rtcp_port,
                 port_rtp, port_rtcp,
                 m_client_session_id);
        ret = true;
      } else if (m_rtsp_trans_hdr.mode == AM_RTP_TCP) {
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: %s;interleaved=%hhu-%hhu\r\n"
                 "Session: %08x\r\n\r\n",
                 req.cseq,
                 get_date_string(date, sizeof(date)),
                 m_rtsp_trans_hdr.mode_string,
                 m_rtsp_trans_hdr.rtp_channel_id,
                 m_rtsp_trans_hdr.rtcp_channel_id,
                 m_client_session_id);
        ret = true;

      } else {
        /* Currently we don't support RAW_UDP */
        cmd_bad_requests(req, buf, size);
      }
    }
  }
  return ret;
}

bool AMRtspClientSession::cmd_teardown(RtspRequest& req,
                                       char *buf,
                                       uint32_t size)
{
  bool ret = authenticate_ok(req, buf, size);
  if (AM_LIKELY(ret)) {
    if (AM_LIKELY(buf && (size > 0))) {
      INFO("Client %s teardown!", m_client_name.c_str());
      snprintf(buf, size, "RTSP/1.0 200 OK\r\nCSeq: %s\r\n\r\n",
               req.cseq);
      teardown();
    }
  }
  return !ret;

}

bool AMRtspClientSession::cmd_play(RtspRequest& req,
                                   char *buf,
                                   uint32_t size)
{
  bool ret = true;
  if (AM_LIKELY(authenticate_ok(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      char date[200] = { 0 };
      snprintf(buf,
               size,
               "RTSP/1.0 200 OK\r\n"
               "CSeq: %s\r\n"
               "%s"
               "Range: npt=0.000-\r\n"
               "Session: %08X\r\n"
               "RTP-Info: ",
               req.cseq,
               get_date_string(date, sizeof(date)),
               m_client_session_id);

      for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
        if (AM_LIKELY(!m_media_info[i].seq_ntp.empty())) {
          char rtsp_url[1024] = {0};
          std::string stream_name(media_type_to_str(AM_RTP_MEDIA_TYPE(i)));
          stream_name.append("=").append(m_media_info[i].media);
          m_rtsp_server->get_rtsp_url(stream_name.c_str(),
                                      rtsp_url,
                                      sizeof(rtsp_url),
                                      m_client_addr);
          snprintf(&buf[strlen(buf)], size - strlen(buf), "url=%s;%s,",
                   rtsp_url, m_media_info[i].seq_ntp.c_str());
          ret = true;
        }
      }
    }
    snprintf(&buf[strlen(buf) - 1], size - strlen(buf), "\r\n\r\n");
  }

  return ret;
}

bool AMRtspClientSession::cmd_get_parameter(RtspRequest& req,
                                            char *buf,
                                            uint32_t size)
{
  bool ret = true;
  if (AM_LIKELY(authenticate_ok(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      if (AM_LIKELY(!media_is_empty())) {
        char date[200] = {0};
        snprintf(buf, size,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "%s"
            "Session: %08X\r\n\r\n",
            req.cseq,
            get_date_string(date, sizeof(date)),
            m_client_session_id);
        ret = true;
        for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
          if (AM_LIKELY(req.media[i] && m_media_info[i].media.c_str() &&
                        is_str_equal(m_media_info[i].media.c_str(),
                                     req.media[i]))) {
            AMRtpClientMsgACK msg_ack(m_identify | m_media_info[i].stream_id,
                                      m_media_info[i].ssrc);
            strcpy(msg_ack.media, req.media[i]);
            m_rtsp_server->send_rtp_control_data((uint8_t*)&msg_ack,
                                                 sizeof(msg_ack));
          }
        }
      } else {
        not_found(req, buf, size);
      }
    }
  }
  return ret;
}

bool AMRtspClientSession::cmd_set_patameter(RtspRequest& req,
                                            char *buf,
                                            uint32_t size)
{
  bool ret = true;
  if (AM_LIKELY(authenticate_ok(req, buf, size))) {
    ret = false;
    if (AM_LIKELY(buf && (size > 0))) {
      if (AM_LIKELY(!media_is_empty())) {
        char date[200] = {0};
        snprintf(buf, size,
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Session: %08X\r\n\r\n",
                 req.cseq,
                 get_date_string(date, sizeof(date)),
                 m_client_session_id);
        ret = true;
      } else {
        not_found(req, buf, size);
      }
    }
  }
  return ret;
}

bool AMRtspClientSession::cmd_not_supported(RtspRequest& req,
                                            char *buf,
                                            uint32_t size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 405 Method Not Allowed\r\n"
                      "CSeq: %s\r\n%s"
                      "Allow: %s\r\n\r\n",
           req.cseq, get_date_string(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

bool AMRtspClientSession::cmd_bad_requests(RtspRequest& req,
                                           char *buf,
                                           uint32_t size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 400 Bad Request\r\n%sAllow: %s\r\n\r\n",
           get_date_string(date, sizeof(date)), ALLOWED_COMMAND);
  return true;
}

bool AMRtspClientSession::authenticate_ok(RtspRequest& req,
                                          char *buf,
                                          uint32_t size)
{
  bool auth_ok = true;
  if (AM_LIKELY(m_authenticator && m_rtsp_server->need_authentication())) {
    if (AM_LIKELY(m_rtsp_auth_hdr.is_ok() &&
                  is_str_same(m_rtsp_auth_hdr.realm,
                              m_authenticator->get_realm()) &&
                  is_str_same(m_rtsp_auth_hdr.nonce,
                              m_authenticator->get_nonce()))) {
      auth_ok = m_authenticator->authenticate(req, m_rtsp_auth_hdr,
                                              m_rtsp_server->m_rtsp_config->\
                                              rtsp_server_user_db.c_str());
    } else {
      auth_ok = false;
    }

    if (AM_UNLIKELY(!auth_ok)) {
      char nonce[128] = { 0 };
      char date[200] = { 0 };
      sprintf(nonce,
              "%x%x",
              m_rtsp_server->get_random_number(),
              m_rtsp_server->get_random_number());
      m_authenticator->set_nonce(nonce);
      snprintf(buf,
               size,
               "RTSP/1.0 401 Unauthorized\r\n"
               "CSeq: %s\r\n%s"
               "WWW-Authenticate: Digest realm=\"%s\", "
               "nonce=\"%s\"\r\n\r\n",
               req.cseq,
               get_date_string(date, sizeof(date)),
               m_authenticator->get_realm(),
               m_authenticator->get_nonce());
    }
    m_rtsp_auth_hdr.reset();
  }

  return auth_ok;

}

void AMRtspClientSession::not_found(RtspRequest& req,
                                    char *buf,
                                    uint32_t size)
{
  char date[200] = {0};
  snprintf(buf, size, "RTSP/1.0 404 Stream Not Found\r\nCSeq: %s\r\n%s\r\n",
           req.cseq, get_date_string(date, sizeof(date)));

}

char* AMRtspClientSession::get_date_string(char* buf, uint32_t size)
{
  time_t cur_time = time(NULL);
  strftime(buf, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&cur_time));

  return buf;

}

void AMRtspClientSession::teardown()
{
  for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
    if (AM_LIKELY(!m_media_info[i].media.empty() && m_media_info[i].is_alive)) {
      AMRtpClientMsgKill msg_kill(m_media_info[i].ssrc,
                                  (m_identify | m_media_info[i].stream_id),
                                  m_rtsp_server->m_unix_sock_fd);
      if (AM_UNLIKELY(!m_rtsp_server->send_rtp_control_data(
          (uint8_t*)&msg_kill, sizeof(msg_kill)))) {
        ERROR("Failed to send kill message to RTP Muxer to kill client"
              "(identify: %08X, SSRC: %08X)!",
              (m_identify | m_media_info[i].stream_id),
              m_media_info[i].ssrc);
      } else {
        INFO("Sent kill message to RTP Muxer to kill "
             "client(Identify: %08X, SSRC: %08X)!",
             (m_identify | m_media_info[i].stream_id),
             m_media_info[i].ssrc);
        if (AM_LIKELY(m_media_info[i].event_kill->wait(5000))) {
          NOTICE("Client with Identify: %08X, SSRC: %08X is killed!",
                 (m_identify | m_media_info[i].stream_id),
                 m_media_info[i].ssrc);
        } else {
          ERROR("Failed to receive response from RTP muxer!");
        }
      }
    }
  }
}

std::string AMRtspClientSession::get_sdp()
{
  std::string sdp;
  sdp.clear();
  if (AM_LIKELY(media_has_sdp())) {
    sdp = std::string("v=0\r\n") + "o=" +
        (m_rtsp_auth_hdr.username ? m_rtsp_auth_hdr.username : "-") + " " +
        std::to_string(m_client_session_id) + " 1 " + "IN IP4" + " " +
        inet_ntoa(m_client_addr.sin_addr) + "\r\n" +
        "s=Streamd by \"" + m_rtsp_server->m_rtsp_config->rtsp_server_name +
        "\"\r\n" +
        "i=session information\r\n" +
        "t=0 0\r\n" +
        "a=tool:" + m_rtsp_server->m_rtsp_config->rtsp_server_name +
        ":" + __DATE__ + " - " + __TIME__ + "\r\n" +
        "a=type:broadcast\r\n" +
        "a=control:*\r\n" +
        "a=range:npt=0-\r\n";
    for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
      if (AM_LIKELY(!m_media_info[i].sdp.empty())) {
        sdp.append(m_media_info[i].sdp);
      }
    }
  }

  return sdp;
}

void AMRtspClientSession::sdp_ack()
{
  m_event_msg_sdp->signal();
}

void AMRtspClientSession::seq_ntp_ack(AM_RTP_MEDIA_TYPE type)
{
  if (AM_LIKELY((type >= 0) && (type < AM_RTP_MEDIA_NUM))) {
    m_media_info[type].event_msg_seqntp->signal();
  }
}

void AMRtspClientSession::ssrc_ack(AM_RTP_MEDIA_TYPE type)
{
  if (AM_LIKELY((type >= 0) && (type < AM_RTP_MEDIA_NUM))) {
    m_media_info[type].event_ssrc->signal();
  }
}

void AMRtspClientSession::kill_ack(AM_RTP_MEDIA_TYPE type)
{
  if (AM_LIKELY((type >= 0) && (type < AM_RTP_MEDIA_NUM))) {
    m_media_info[type].event_kill->signal();
    m_media_info[type].is_alive = false;
  }
}

void AMRtspClientSession::set_media_sdp(AM_RTP_MEDIA_TYPE type,
                                        const char *sdp)
{
  if (AM_LIKELY(((type >= 0) && (type < AM_RTP_MEDIA_NUM)))) {
    m_media_info[type].sdp = sdp ? sdp : "";
  }
}

void AMRtspClientSession::set_media_seq_ntp(AM_RTP_MEDIA_TYPE type,
                                            const char *seq_ntp)
{
  if (AM_LIKELY(((type >= 0) && (type < AM_RTP_MEDIA_NUM)))) {
    m_media_info[type].seq_ntp = seq_ntp ? seq_ntp : "";
  }
}

void AMRtspClientSession::set_media_ssrc(AM_RTP_MEDIA_TYPE type,
                                         uint32_t ssrc)
{
  if (AM_LIKELY(((type >= 0) && (type < AM_RTP_MEDIA_NUM)))) {
    m_media_info[type].ssrc = ssrc;
  }
}

uint16_t AMRtspClientSession::media_stream_id(AM_RTP_MEDIA_TYPE type)
{
  return ((type >= 0) && (type < AM_RTP_MEDIA_NUM)) ?
      m_media_info[type].stream_id : ((uint16_t)-1);
}

bool AMRtspClientSession::media_has_sdp()
{
  bool ret = false;
  for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
    ret = !m_media_info[i].sdp.empty();
    if (AM_LIKELY(ret)) {
      break;
    }
  }

  return ret;
}

bool AMRtspClientSession::media_is_empty()
{
  bool ret = true;
  for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
    ret = m_media_info[i].media.empty();
    if (AM_LIKELY(!ret)) {
      break;
    }
  }
  return ret;
}

bool AMRtspClientSession::is_alive()
{
  bool alive = false;
  for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
    if (AM_LIKELY(!m_media_info[i].media.empty() && m_media_info[i].is_alive)) {
      alive = true;
      break;
    }
  }
  return alive;
}
