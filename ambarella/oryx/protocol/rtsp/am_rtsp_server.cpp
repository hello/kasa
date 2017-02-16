/*******************************************************************************
 * am_rtsp_server.cpp
 *
 * History:
 *   2014-12-19 - [Shiming Dong] created file
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
#include "am_file.h"
#include "am_log.h"

#include "am_rtsp_server.h"
#include "am_rtsp_server_config.h"
#include "am_rtsp_client_session.h"

#include "am_event.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_rtp_msg.h"

#include <sys/un.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <ifaddrs.h>
#include <signal.h>

#include <queue>

#define  SERVER_CTRL_R             m_pipe[0]
#define  SERVER_CTRL_W             m_pipe[1]

#define  UNIX_SOCK_CTRL_R          m_ctrl_unix_fd[0]
#define  UNIX_SOCK_CTRL_W          m_ctrl_unix_fd[1]

#define  CMD_DEL_CLIENT            'd'
#define  CMD_ABRT_ALL              'a'
#define  CMD_STOP_ALL              'e'

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define RTSP_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR"/apps")
#else
#define RTSP_CONF_DIR ((const char*)"/etc/oryx/apps")
#endif

AMRtspServer *AMRtspServer::m_instance = nullptr;
std::mutex AMRtspServer::m_lock;

struct ServerCtrlMsg
{
  uint32_t code;
  uint32_t data;
};

static bool is_fd_valid(int fd)
{
  return (fd >= 0) && ((fcntl(fd, F_GETFD) != -1) || (errno != EBADF));
}

AMIRtspServerPtr AMIRtspServer::create()
{
  return AMRtspServer::get_instance();
}

AMIRtspServer* AMRtspServer::get_instance()
{
  m_lock.lock();
  if (AM_LIKELY(!m_instance)) {
    m_instance = new AMRtspServer();
    if (AM_UNLIKELY(m_instance && (!m_instance->construct()))) {
      delete m_instance;
      m_instance = nullptr;
    }
  }
  m_lock.unlock();

  return m_instance;
}

bool AMRtspServer::start()
{
  if (AM_LIKELY(m_run == false)) {
    m_run = (start_server_thread() && start_connect_unix_thread()
             && setup_server_socket_tcp(m_rtsp_config->rtsp_server_port));
    m_event->signal();
  }

  return m_run;
}


void AMRtspServer::stop()
{
  destroy_all_client_session();

  while (m_run) {
    INFO("Sending stop to %s", m_server_thread->name());
    if (AM_LIKELY(SERVER_CTRL_W >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_STOP_ALL;
      msg.data = 0;
      write(SERVER_CTRL_W, &msg, sizeof(msg));
      usleep(30000);
    }
  }

  while (m_unix_sock_run) {
    INFO("Sending stop to %s", m_sock_thread->name());
    if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_STOP_ALL;
      msg.data = 0;
      write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg));
      usleep(30000);
    }
  }

  AM_DESTROY(m_server_thread); /* Wait RTSP.server to exit */
  AM_DESTROY(m_sock_thread);   /* Wait RTSP.Unix to exit */
}

uint32_t AMRtspServer::version()
{
  return RTSP_LIB_VERSION;
}

void AMRtspServer::inc_ref()
{
  ++ m_ref_count;
}

void AMRtspServer::release()
{
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    NOTICE("Last reference of AMRtspServer's object %p, release it!",
           m_instance);
    delete m_instance;
    m_instance = nullptr;
    m_ref_count = 0;
  }
}

uint16_t AMRtspServer::get_server_port_tcp()
{
  return m_port_tcp;
}

uint32_t AMRtspServer::get_random_number()
{
  struct timeval current = {0};
  gettimeofday(&current, NULL);
  srand(current.tv_usec);
  return (rand() % ((uint32_t)-1));
}

void AMRtspServer::static_server_thread(void *data)
{
  ((AMRtspServer*)data)->server_thread();
}

void AMRtspServer::static_unix_thread(void *data)
{
  ((AMRtspServer*)data)->connect_unix_thread();
}

void AMRtspServer::server_thread()
{
  m_event->wait();

  while(m_run) {
    int maxfd = -1;
    fd_set fdset;

    FD_ZERO(&fdset);

    FD_SET(m_srv_sock_tcp, &fdset);
    FD_SET(SERVER_CTRL_R, &fdset);

    maxfd = AM_MAX(m_srv_sock_tcp, SERVER_CTRL_R);

    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
      if (FD_ISSET(SERVER_CTRL_R, &fdset)) {
        ServerCtrlMsg msg  = {0};
        uint32_t   read_cnt = 0;
        ssize_t   read_ret = 0;
        do {
          read_ret = read(SERVER_CTRL_R, &msg, sizeof(msg));
        } while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
                 ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                  (errno == EINTR)))));
        if (AM_UNLIKELY(read_ret < 0)) {
          PERROR("read");
        } else {
          if (msg.code == CMD_DEL_CLIENT) {
            AUTO_LOCK(m_client_mutex);
            AMRtspClientMap::iterator iter = m_client_map.find(msg.data);
            if (AM_LIKELY(iter != m_client_map.end())) {
              AMRtspClientSession *&client = iter->second;
              NOTICE("RTSP server is asked to delete client %s",
                     client->m_client_name.c_str());
              delete client;
              m_client_map.erase(msg.data);
            }
          } else if ((msg.code == CMD_ABRT_ALL) || (msg.code == CMD_STOP_ALL)) {
            AUTO_LOCK(m_client_mutex);
            close(m_srv_sock_tcp);

            m_srv_sock_tcp = -1;
            switch (msg.code) {
              case CMD_ABRT_ALL:
                ERROR("Fatal error occurred, RTSP server abort!");
                break;
              case CMD_STOP_ALL:
                m_run = false;
                NOTICE("RTSP server received stop signal!");
                break;
              default:
                break;
            }
            break;
          }
        }
      } else if (FD_ISSET(m_srv_sock_tcp, &fdset)) {
        AUTO_LOCK(m_client_mutex);
        AMRtspClientSession *session = new AMRtspClientSession(
            this, (m_rtsp_config->rtp_stream_port_base +
                   m_client_map.size() * AM_RTP_MEDIA_NUM * 2));
        if (AM_LIKELY(session)) {
          bool need_abort = false;
          if (AM_UNLIKELY(!session->init_client_session(m_srv_sock_tcp))) {
            ERROR("Failed to initialize client session");
            need_abort = true;
          } else if (AM_LIKELY(m_client_map.size() <
                               m_rtsp_config->max_client_num)) {
            m_client_map[session->m_identify] = session;
          } else {
            NOTICE("Maximum client number(%u) has reached!",
                   m_rtsp_config->max_client_num);
            need_abort = true;
          }
          if (AM_UNLIKELY(need_abort)) {
            abort_client(*session);
            delete session;
          }
        } else {
          ERROR("Failed to create AMRtspClientSession!");
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        break;
      }
    }
  }

  if (AM_UNLIKELY(m_run)) {
    ERROR("RTSP Server exited abnormally.");
  } else {
    INFO("RTSP Server exit mainloop.");
  }
  m_run = false;
}

void AMRtspServer::connect_unix_thread()
{
  m_unix_sock_run = true;

  while (m_unix_sock_run) {
    bool isok = true;
    int retval = -1;
    timeval *tm = NULL;
    timeval timeout = {0};

    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(UNIX_SOCK_CTRL_R, &fdset);

    if (AM_LIKELY(m_unix_sock_fd >= 0)) {
      if (AM_UNLIKELY(!m_unix_sock_con)) { /* Just connected */
        int send_bytes = -1;
        AMRtpClientMsgProto proto_msg;
        proto_msg.proto = AM_RTP_CLIENT_PROTO_RTSP;

        if (AM_UNLIKELY((send_bytes = send(m_unix_sock_fd,
                                           &proto_msg,
                                           sizeof(proto_msg),
                                           0)) == -1)) {
          ERROR("Send message to RTP muxer error!");
          isok = false;
        } else {
          INFO("Send %d bytes protocol messages to RTP muxer.", send_bytes);
          m_unix_sock_con = true;
        }
      }
      FD_SET(m_unix_sock_fd,   &fdset);
    } else {
      timeout.tv_sec = m_rtsp_config->rtsp_retry_interval / 1000;
      tm = &timeout;
    }

    retval = select((AM_MAX(m_unix_sock_fd, UNIX_SOCK_CTRL_R) + 1),
                    &fdset, NULL, NULL, tm);
    if (AM_LIKELY(retval > 0)) {
      if (FD_ISSET(UNIX_SOCK_CTRL_R, &fdset)) {
        ServerCtrlMsg msg = {0};
        uint32_t read_cnt = 0;
        ssize_t  read_ret = 0;
        do {
          read_ret = read(UNIX_SOCK_CTRL_R, &msg, sizeof(msg));
        }while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
                ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                    (errno == EINTR)))));

        if (AM_UNLIKELY(read_ret < 0)) {
          PERROR("read");
        } else if (AM_LIKELY((msg.code == CMD_ABRT_ALL) ||
                             (msg.code == CMD_STOP_ALL))) {
          switch (msg.code) {
            case CMD_ABRT_ALL:
              ERROR("Fatal error occurred, RTSP.unix abort!");
              break;
            case CMD_STOP_ALL:
              m_unix_sock_run = false;
              NOTICE("RTSP.unix received stop signal!");
              break;
            default:
              break;
          }
          isok = false;
          NOTICE("Close unix socket connection with RTP Muxer!");
          unix_socket_close();
          break; /* break while() */
        }
      } else if (FD_ISSET(m_unix_sock_fd, &fdset)) {
        INFO("Received message from RTP muxer!");
        isok = recv_rtp_control_data();
      }
    } else if (retval < 0) {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
      }
      isok = is_fd_valid(m_unix_sock_fd);
    }
    if (AM_UNLIKELY(!m_unix_sock_con)) {
      m_unix_sock_fd = -1;
      m_unix_sock_fd = unix_socket_conn(RTP_CONTROL_SOCKET);
    }
    if (AM_UNLIKELY(!isok)) {
      NOTICE("Close unix socket connection with RTP Muxer!");
      unix_socket_close();
    }
  } /* while */

  if (AM_LIKELY(m_unix_sock_run)) {
    INFO("RTSP.Unix exits due to server abort!");
  } else {
    INFO("RTSP.Unix exits mainloop!");
  }
  m_unix_sock_run = false;
}

bool AMRtspServer::start_server_thread()
{
  bool ret = true;
  AM_DESTROY(m_server_thread);
  m_server_thread = AMThread::create("RTSP.server",
                                     static_server_thread,
                                     this);
  if (AM_UNLIKELY(!m_server_thread)) {
    ERROR("Failed to create RTSP Server thread!");
    ret = false;
  }

  return ret;
}

bool AMRtspServer::start_connect_unix_thread()
{
  bool ret = true;
  AM_DESTROY(m_sock_thread);
  m_sock_thread = AMThread::create("RTSP.unix",
                                   static_unix_thread,
                                   this);
  if (AM_UNLIKELY(!m_sock_thread)) {
    ERROR("Failed to create connect unix thread!");
    ret = false;
  }

  return ret;
}

void AMRtspServer::server_abort()
{
  destroy_all_client_session();
  while (m_run) {
    if (AM_LIKELY(SERVER_CTRL_W >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_ABRT_ALL;
      msg.data = 0;
      write(SERVER_CTRL_W, &msg, sizeof(msg));
      usleep(30000);
    }
  }
  while (m_unix_sock_run) {
    if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg;
      msg.code = CMD_ABRT_ALL;
      msg.data = 0;
      write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg));
      usleep(30000);
    }
  }
  AM_DESTROY(m_server_thread); /* Wait RTSP.server to exit */
  AM_DESTROY(m_sock_thread);   /* Wait RTSP.Unix to exit */
}

bool AMRtspServer::setup_server_socket_tcp(uint16_t server_port)
{
  bool ret  = false;
  int reuse = 1;
  m_port_tcp = server_port;

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port        = htons(m_port_tcp);

  if (AM_UNLIKELY((m_srv_sock_tcp = socket(AF_INET, SOCK_STREAM,
                                          IPPROTO_TCP)) < 0 )) {
    PERROR("RTSP tcp socket");
  } else {
    if (AM_LIKELY(setsockopt(m_srv_sock_tcp, SOL_SOCKET,
                             SO_REUSEADDR, &reuse, sizeof(reuse)) == 0)) {
      int flag = fcntl(m_srv_sock_tcp, F_GETFL, 0);
      flag |= O_NONBLOCK;

      if (AM_UNLIKELY(fcntl(m_srv_sock_tcp, F_SETFL, flag) != 0)) {
        PERROR("fcntl");
      } else if (AM_UNLIKELY(bind(m_srv_sock_tcp,
                                  (struct sockaddr*)&server_addr,
                                  sizeof(server_addr)) != 0)) {
        PERROR("bind");
      } else if (AM_UNLIKELY(listen(m_srv_sock_tcp,
                                    m_rtsp_config->max_client_num) < 0)) {
        PERROR("listen");
      } else {
        ret = true;
      }
    } else {
      PERROR("setsockopt");
    }
  }
  return ret;
}

void AMRtspServer::abort_client(AMRtspClientSession &client)
{
  RtspRequest rtsp_req;
  rtsp_req.set_command((char*)"TEARDOWN");

  client.handle_client_request(rtsp_req);
  client.abort_client();
  while(client.m_client_thread->is_running()) {
    NOTICE("Wait for client %s to exit", client.m_client_name.c_str());
    usleep(10000);
  }
}

bool AMRtspServer::delete_client_session(uint32_t identify)
{
  ServerCtrlMsg msg;
  msg.code = CMD_DEL_CLIENT;
  msg.data = identify;
  return ((ssize_t)sizeof(msg) == write(SERVER_CTRL_W, &msg, sizeof(msg)));
}

bool AMRtspServer::find_client_and_kill(struct in_addr &addr)
{
  AUTO_LOCK(m_client_mutex);
  AMRtspClientSession *client = NULL;
  for (AMRtspClientMap::iterator iter = m_client_map.begin();
      iter != m_client_map.end(); ++ iter) {
    if (AM_LIKELY(addr.s_addr == client->m_client_addr.sin_addr.s_addr)) {
      client = iter->second;
      break;
    }
  }
  return (client ? client->kill_client() : true);
}

bool AMRtspServer::get_source_addr(sockaddr_in& client_addr,
                                   sockaddr_in& source_addr)
{
  bool ret = false;
  struct ifaddrs *interfaces = 0;
  struct ifaddrs *temp_if    = 0;

  if (getifaddrs(&interfaces) < 0) {
    ret = false;
    PERROR("getifaddrs");
  } else {
    for (temp_if = interfaces; temp_if; temp_if = temp_if->ifa_next) {
      if (AM_LIKELY(temp_if->ifa_netmask)) {
        uint32_t mask = ((sockaddr_in*)temp_if->ifa_netmask)->sin_addr.s_addr;
        if (AM_LIKELY((mask&((sockaddr_in*)temp_if->ifa_addr)->sin_addr.s_addr)
                      == (mask & client_addr.sin_addr.s_addr))) {
          memcpy(&source_addr, ((sockaddr_in*)(temp_if->ifa_addr)),
                 sizeof(sockaddr_in));
          ret = true;
          break;
        }
      }
    }
    if (AM_UNLIKELY(!temp_if)) {
      ERROR("Can not find the source address!");
    }
  }
  return ret;
}

bool AMRtspServer::get_rtsp_url(const char *stream_name,
                                char *buf,
                                uint32_t size,
                                struct sockaddr_in &client_addr)
{
  bool ret = false;
  char *host = NULL;
  sockaddr_in source_addr;
  memset(&source_addr, 0, sizeof(source_addr));

  if (AM_LIKELY(get_source_addr(client_addr, source_addr))) {
    host = inet_ntoa(source_addr.sin_addr);
  } else {
    host = (char*)"0.0.0.0";
  }
  if (AM_LIKELY(buf && (size > 0))) {
    if (AM_LIKELY(get_server_port_tcp() == 554)) {
      snprintf(buf, size, "rtsp://%s/%s", host, stream_name);
    } else {
      snprintf(buf, size, "rtsp://%s:%hu/%s", host, get_server_port_tcp(),
               stream_name);
    }
    INFO("RTSP URL: %s", buf);
    ret = true;
  }

  return ret;
}

int AMRtspServer::unix_socket_conn(const char *server_name)
{
  int fd    = -1;
  bool isok = true;
  const char *rtsp_server_name = m_rtsp_config->rtsp_unix_domain_name.c_str();
  do {
    socklen_t len = 0;
    sockaddr_un un;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      PERROR("unix domain socket");
      isok = false;
      break;
    }

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, rtsp_server_name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
    if (AM_UNLIKELY(AMFile::exists(rtsp_server_name))) {
      NOTICE("%s alreay exists! Remove it first!", rtsp_server_name);
      if (AM_UNLIKELY(unlink(rtsp_server_name) < 0)) {
        PERROR("unlink");
        isok = false;
        break;
      }
    }
    if (AM_UNLIKELY(!AMFile::create_path(AMFile::dirname(rtsp_server_name).
                                         c_str()))) {
      ERROR("Failed to create path %s",
            AMFile::dirname(rtsp_server_name).c_str());
      isok = false;
      break;
    }

    if (bind(fd, (struct sockaddr *)&un, len) < 0) {
      PERROR("unix domain bind");
      isok = false;
      break;
    }

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, server_name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_name);
    if (AM_UNLIKELY(connect(fd, (struct sockaddr *)&un, len) < 0)) {
      if (AM_LIKELY(errno == ENOENT)) {
        NOTICE("RTP Muxer is not working, wait and try again...");
      } else {
        PERROR("connect");
      }
      isok = false;
      break;
    }
  } while (0);

  if (AM_UNLIKELY(!isok)) {
    if (AM_LIKELY(fd >= 0)) {
      close(fd);
      fd = -1;
    }
    if (AM_UNLIKELY(unlink(rtsp_server_name) < 0)) {
      PERROR("unlink");
    }
  }

  return fd;
}

void AMRtspServer::unix_socket_close()
{
  if (AM_LIKELY(m_unix_sock_fd >= 0)) {
    close(m_unix_sock_fd);
  }
  if (AM_UNLIKELY(AMFile::exists(m_rtsp_config->\
                                 rtsp_unix_domain_name.c_str()) &&
                  (unlink(m_rtsp_config->rtsp_unix_domain_name.c_str()) < 0))) {
    PERROR("unlink");
  }
  m_unix_sock_fd = -1;
  m_unix_sock_con = false;
}

bool AMRtspServer::recv_rtp_control_data()
{
  bool isok = true;
  if (AM_LIKELY(m_unix_sock_fd >= 0)) {
    int read_ret      = 0;
    int read_cnt      = 0;
    uint32_t received = 0;
    AMRtpClientMsgBlock union_msg;

    do {
      read_ret = recv(m_unix_sock_fd,
                      ((uint8_t*)&union_msg) + received,
                      sizeof(AMRtpClientMsgBase) - received,
                      0);
      if (AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    } while ((++ read_cnt < 5) &&
             (((read_ret >= 0) &&
               ((size_t)read_ret < sizeof(AMRtpClientMsgBase))) ||
               ((read_ret < 0) && ((errno == EINTR) ||
                                   (errno == EWOULDBLOCK) ||
                                   (errno == EAGAIN)))));
    if (AM_LIKELY(received == sizeof(AMRtpClientMsgBase))) {
      AMRtpClientMsg *msg = (AMRtpClientMsg*)&union_msg;
      read_ret = 0;
      read_cnt = 0;
      while ((received < msg->length) && (5 > read_cnt ++) &&
             ((read_ret == 0) || ((read_ret < 0) && ((errno == EINTR) ||
                                                     (errno == EWOULDBLOCK) ||
                                                     (errno == EAGAIN))))) {
        read_ret = recv(m_unix_sock_fd,
                        ((uint8_t*) &union_msg) + received,
                        msg->length - received,
                        0);
        if (AM_LIKELY(read_ret > 0)) {
          received += read_ret;
        }
      }
      if (AM_LIKELY(received == msg->length)) {
        uint32_t identify = msg->session_id & 0xffff0000;
        AMRtspClientMap::iterator iter = m_client_map.find(identify);
        AMRtspClientSession *rtsp_client =
            (iter != m_client_map.end()) ? iter->second : NULL;
        switch (msg->type) {
          case AM_RTP_CLIENT_MSG_SDP: {
            if (AM_LIKELY(rtsp_client)) {
              rtsp_client->set_media_sdp(
                  AM_RTP_MEDIA_VIDEO,
                  union_msg.msg_sdp.media[AM_RTP_MEDIA_VIDEO]);
              rtsp_client->set_media_sdp(
                  AM_RTP_MEDIA_AUDIO,
                  union_msg.msg_sdp.media[AM_RTP_MEDIA_AUDIO]);
              rtsp_client->sdp_ack();
            } else {
              ERROR("Received message: %u for client with session id: %08X, "
                    "but can not find such client!",
                    msg->type, msg->session_id);
            }
          }break;
          case AM_RTP_CLIENT_MSG_SSRC: {
            if (AM_LIKELY(rtsp_client)) {
              AMRtpClientMsgControl &msg_ctrl = union_msg.msg_ctrl;
              uint16_t id = (uint16_t)(msg_ctrl.session_id & 0x0000ffff);
              for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                if (AM_LIKELY(rtsp_client->
                              media_stream_id(AM_RTP_MEDIA_TYPE(i)) == id)) {
                  rtsp_client->set_media_ssrc(AM_RTP_MEDIA_TYPE(i),
                                              msg_ctrl.ssrc);
                  rtsp_client->ssrc_ack(AM_RTP_MEDIA_TYPE(i));
                  break;
                }
              }
              NOTICE("Client with identify %08X received its SSRC %08X",
                     (rtsp_client->m_identify & 0xffff0000u),
                     msg_ctrl.ssrc);
            } else {
              ERROR("Received message: %u for client with session id: %08X, "
                  "but can not find such client!",
                  msg->type, msg->session_id);
            }
          }break;
          case AM_RTP_CLIENT_MSG_START: {
            if (AM_LIKELY(rtsp_client)) {
              AMRtpClientMsgControl &msg_ctrl = union_msg.msg_ctrl;
              uint16_t id = (uint16_t)(msg_ctrl.session_id & 0x0000ffff);
              for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                if (AM_LIKELY(rtsp_client->\
                              media_stream_id(AM_RTP_MEDIA_TYPE(i)) == id)) {
                  rtsp_client->set_media_seq_ntp(AM_RTP_MEDIA_TYPE(i),
                                                 msg_ctrl.seq_ntp);
                  rtsp_client->seq_ntp_ack(AM_RTP_MEDIA_TYPE(i));
                  break;
                }
              }
              NOTICE("Client with SSRC %08X respond play command, "
                     "seq-ntp: \"%s\"!",
                     union_msg.msg_ctrl.ssrc,
                     union_msg.msg_ctrl.seq_ntp);
            } else {
              ERROR("Received message: %u for client with session id: %08X, "
                  "but can not find such client!",
                  msg->type, msg->session_id);
            }
          }break;
          case AM_RTP_CLIENT_MSG_KILL: {/* kill ACK msg */
            NOTICE("Client with identify %08X, SSRC %08X is killed.",
                               union_msg.msg_kill.session_id,
                               union_msg.msg_kill.ssrc);
            if (AM_LIKELY(rtsp_client)) {
              AMRtpClientMsgKill &msg_kill = union_msg.msg_kill;
              uint16_t id = (uint16_t)(msg_kill.session_id & 0x0000ffff);

              for (uint32_t i = 0; i < AM_RTP_MEDIA_NUM; ++ i) {
                if (AM_LIKELY(rtsp_client->\
                              media_stream_id(AM_RTP_MEDIA_TYPE(i)) == id)) {
                  rtsp_client->kill_ack(AM_RTP_MEDIA_TYPE(i));
                }
              }
              if (AM_LIKELY(!rtsp_client->is_alive())) {
                m_client_map.erase(iter);
              }
            }
          }break;
          case AM_RTP_CLIENT_MSG_CLOSE: {
            NOTICE("RTP Muxer is stopped, disconnect with it!");
            isok = false;
          }break;
          default: {
            ERROR("Invalid message type received!");
          }break;
        }
      } else {
        ERROR("Failed to receive message(%u bytes), type: %u",
              msg->length, msg->type);
      }
    } else {
      if (AM_LIKELY((errno != EINTR) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EAGAIN))) {
        ERROR("Failed to receive message(recv: %s)!", strerror(errno));
        isok = false;
      }
    }
  }
  return isok;
}

void AMRtspServer::destroy_all_client_session(bool client_abort)
{
  for (AMRtspClientMap::iterator iter = m_client_map.begin();
      iter != m_client_map.end(); ++ iter) {
    if (AM_LIKELY(client_abort)) {
      abort_client(*iter->second);
    }
    delete iter->second;
    m_client_map.erase(iter);
  }
  m_client_map.clear();
}

bool AMRtspServer::need_authentication()
{
  return m_rtsp_config ? m_rtsp_config->need_auth : false;
}

bool AMRtspServer::send_rtp_control_data(uint8_t *data, size_t len)
{
  bool ret = false;

  if (AM_LIKELY((m_unix_sock_fd >= 0) && data && (len > 0))) {
    int        send_ret   = -1;
    uint32_t total_sent   = 0;
    uint32_t resend_count = 0;
    do {
      send_ret = write(m_unix_sock_fd, data + total_sent, len - total_sent);
      if (AM_UNLIKELY(send_ret < 0)) {
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          if (AM_LIKELY(errno == ECONNRESET)) {
            WARN("RTP muxer closed it's connection while sending data!");
          } else {
            PERROR("send");
          }
          unix_socket_close();
          break;
        }
      } else {
        total_sent += send_ret;
      }
      ret = (len == total_sent);
      if (AM_UNLIKELY(!ret)) {
        if (AM_LIKELY(++ resend_count >=
                      m_rtsp_config->rtsp_send_retry_count)) {
          ERROR("Connection to RTP muxer is unstable!");
          break;
        }
        usleep(m_rtsp_config->rtsp_send_retry_interval * 1000);
      }
    }while(!ret);
  } else if (AM_LIKELY(m_unix_sock_fd < 0)) {
    ERROR("Connection to RTP Muxer is already closed!");
  }

  return ret;
}

AMRtspServer::AMRtspServer() :
    m_rtsp_config(nullptr),
    m_config(nullptr),
    m_server_thread(nullptr),
    m_sock_thread(nullptr),
    m_event(nullptr),
    m_client_mutex(nullptr),
    m_port_tcp(0),
    m_run(false),
    m_unix_sock_con(false),
    m_unix_sock_run(false),
    m_ref_count(0),
    m_srv_sock_tcp(-1),
    m_unix_sock_fd(-1)
{
  SERVER_CTRL_R = -1;
  SERVER_CTRL_W = -1;
  UNIX_SOCK_CTRL_R = -1;
  UNIX_SOCK_CTRL_W = -1;
  m_client_map.clear();
}

AMRtspServer::~AMRtspServer()
{
  destroy_all_client_session(false);
  AM_DESTROY(m_server_thread);
  AM_DESTROY(m_sock_thread);
  AM_DESTROY(m_event);
  AM_DESTROY(m_client_mutex);

  if (AM_LIKELY(SERVER_CTRL_R >= 0)) {
    close(SERVER_CTRL_R);
  }
  if (AM_LIKELY(SERVER_CTRL_W >= 0)) {
    close(SERVER_CTRL_W);
  }
  if (AM_LIKELY(UNIX_SOCK_CTRL_R >= 0)) {
    close(UNIX_SOCK_CTRL_R);
  }
  if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
    close(UNIX_SOCK_CTRL_W);
  }
  DEBUG("~AMRtspServer");
}

bool AMRtspServer::construct()
{
  bool state = true;
  do {
    m_config = new AMRtspServerConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create AMRtspServerConfig object!");
      state = false;
      break;
    }

    m_config_file = std::string(RTSP_CONF_DIR) + "/rtsp_server.acs";
    m_rtsp_config = m_config->get_config(m_config_file);
    if (AM_UNLIKELY(!m_rtsp_config)) {
      ERROR("Failed to load rtsp server config!");
      state = false;
      break;
    }
    if (AM_UNLIKELY(pipe(m_pipe) < 0)) {
      PERROR("pipe");
      state = false;
      break;
    }

    if (AM_UNLIKELY(pipe(m_ctrl_unix_fd) < 0)) {
      PERROR("pipe");
      state = false;
      break;
    }

    if (AM_UNLIKELY(NULL == (m_event = AMEvent::create()))) {
      ERROR("Failed to create event!");
      state = false;
      break;
    }

    if (AM_UNLIKELY(NULL == (m_client_mutex = AMMutex::create()))) {
      ERROR("Failed to create client mutex!");
      state = false;
      break;
    }
    signal(SIGPIPE, SIG_IGN);
  }while(0);

  return state;
}
