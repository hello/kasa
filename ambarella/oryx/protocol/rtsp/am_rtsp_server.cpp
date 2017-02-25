/*******************************************************************************
 * am_rtsp_server.cpp
 *
 * History:
 *   2014-12-19 - [Shiming Dong] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include "am_file.h"
#include "am_log.h"

#include "am_rtsp_server.h"
#include "am_rtsp_server_config.h"
#include "am_rtsp_client_session.h"

#include "am_io.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_rtp_msg.h"

#include <sys/un.h>
#include <sys/time.h>
#include <sys/epoll.h>
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
    m_run = (setup_server_socket_tcp(m_rtsp_config->rtsp_server_port) &&
             start_connect_unix_thread() && start_server_thread());
    m_event->signal();
  }

  return m_run;
}


void AMRtspServer::stop()
{
  abort_all_client_session();
  while (m_run) {
    INFO("Sending stop to %s", m_server_thread->name());
    if (AM_LIKELY(SERVER_CTRL_W >= 0)) {
      ServerCtrlMsg msg =
      {
        .code = CMD_STOP_ALL,
        .data = 0
      };
      ssize_t ret = am_write(SERVER_CTRL_W, &msg, sizeof(msg), 10);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send stop command to RTSP.server thread! %s",
              strerror(errno));
      }
      usleep(30000);
    }
  }

  while (m_unix_sock_run) {
    INFO("Sending stop to %s", m_sock_thread->name());
    if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg =
      {
        .code = CMD_STOP_ALL,
        .data = 0
      };
      ssize_t ret = am_write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg), 10);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send stop command to RTSP.Unix thread! %s",
              strerror(errno));
      }
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
  AMRtspServer *svr = ((AMRtspServer*)data);
  svr->m_rtsp_config->use_epoll ?
      svr->server_thread_epoll() : svr->server_thread();
}

void AMRtspServer::static_unix_thread(void *data)
{
  AMRtspServer *svr = ((AMRtspServer*)data);
  svr->m_rtsp_config->use_epoll ?
      svr->connect_unix_thread_epoll() : svr->connect_unix_thread();
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
        if (AM_UNLIKELY(!process_ctrl_data(SERVER_CTRL_R))) {
          break;
        }
      } else if (FD_ISSET(m_srv_sock_tcp, &fdset)) {
        if (AM_UNLIKELY(!process_client_connect_request(m_srv_sock_tcp))) {
          break;
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

void AMRtspServer::server_thread_epoll()
{
  sigset_t mask;
  sigset_t mask_orig;
  struct epoll_event ev[2];
  int epfd = epoll_create1(EPOLL_CLOEXEC);

  do {
    if (AM_UNLIKELY(epfd < 0)) {
      PERROR("epoll_create1");
      m_run = false;
      break;
    }

    ev[0].data.fd = m_srv_sock_tcp;
    ev[0].events  = EPOLLIN | EPOLLRDHUP;
    if (AM_UNLIKELY(epoll_ctl(epfd, EPOLL_CTL_ADD,
                              m_srv_sock_tcp, &ev[0]) < 0)) {
      PERROR("epoll_ctrl EPOLL_CTL_ADD m_srv_sock_tcp");
      m_run = false;
      break;
    }

    ev[1].data.fd = SERVER_CTRL_R;
    ev[1].events  = EPOLLIN | EPOLLRDHUP;
    if (AM_UNLIKELY(epoll_ctl(epfd, EPOLL_CTL_ADD,
                              SERVER_CTRL_R, &ev[1]) < 0)) {
      PERROR("epoll_ctrl EPOLL_CTL_ADD SERVER_CTRL_R");
      m_run = false;
      break;
    }
  }while(0);

  if (AM_LIKELY(m_run)) {
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig))) {
      PERROR("sigprocmask");
    }
  }

  m_event->wait();

  while(m_run) {
    int nfds = epoll_pwait(epfd, ev, sizeof(ev)/sizeof(epoll_event), -1, &mask);
    if (AM_LIKELY(nfds > 0)) {
      bool need_break = false;
      for (int i = 0; i < nfds; ++ i) {
        if (AM_LIKELY(ev[i].events & EPOLLIN)) {
          if (ev[i].data.fd == SERVER_CTRL_R) {
            need_break = !process_ctrl_data(SERVER_CTRL_R);
          } else if (ev[i].data.fd == m_srv_sock_tcp) {
            need_break = !process_client_connect_request(m_srv_sock_tcp);
          } else {
            ERROR("Unknown socket!");
          }
        } else if (AM_LIKELY((ev[i].events & EPOLLHUP) ||
                             (ev[i].events & EPOLLRDHUP))) {
          if (ev[i].data.fd == SERVER_CTRL_R) {
            WARN("SERVER_CTRL socket is closed!");
          } else if (ev[i].data.fd == m_srv_sock_tcp) {
            ERROR("Server is down!");
          } else {
            ERROR("Unknown socket!");
          }
        } else {
          if (ev[i].data.fd == SERVER_CTRL_R) {
            PERROR("SERVER_CTRL");
          } else if (ev[i].data.fd == m_srv_sock_tcp) {
            PERROR("RTSP");
          } else {
            ERROR("Unknown socket!");
          }
        }
      }
      if (AM_LIKELY(need_break)) {
        break;
      }
    } else {
      if (AM_UNLIKELY(errno != EINTR)) {
        PERROR("epoll_pwait");
        break;
      }
    }
  }

  if (AM_LIKELY(epfd >= 0)) {
    close(epfd);
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
        isok = process_ctrl_data(UNIX_SOCK_CTRL_R);
        if (AM_LIKELY(!isok)) {
          NOTICE("Close unix socket connection with RTP Muxer!");
          unix_socket_close();
          break;
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
      NOTICE("Destroy all clients!");
      abort_all_client_session(false);
    }
  } /* while */

  if (AM_LIKELY(m_unix_sock_run)) {
    INFO("RTSP.Unix exits due to server abort!");
  } else {
    INFO("RTSP.Unix exits mainloop!");
  }
  m_unix_sock_run = false;
}

void AMRtspServer::connect_unix_thread_epoll()
{
  sigset_t mask;
  sigset_t mask_orig;
  struct epoll_event ev[2];
  int epfd = epoll_create1(EPOLL_CLOEXEC);
  int fdcount = 0;

  m_unix_sock_run = true;
  do {
    if (AM_UNLIKELY(epfd < 0)) {
      PERROR("epoll_create1");
      m_unix_sock_run = false;
      break;
    }

    struct epoll_event event;
    event.data.fd = UNIX_SOCK_CTRL_R;
    event.events  = EPOLLIN | EPOLLRDHUP;
    if (AM_UNLIKELY(epoll_ctl(epfd, EPOLL_CTL_ADD,
                              UNIX_SOCK_CTRL_R, &event) < 0)) {
      PERROR("epoll_ctrl EPOLL_CTL_ADD UNIX_SOCK_CTRL_R");
      m_unix_sock_run = false;
      break;
    }
    fdcount = 1;
  }while(0);

  if (AM_LIKELY(m_unix_sock_run)) {
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig))) {
      PERROR("sigprocmask");
    }
  }

  while (m_unix_sock_run) {
    bool       isok = true;
    bool need_break = false;
    int        nfds = -1;
    int     timeout = m_rtsp_config->rtsp_retry_interval;

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
          close(m_unix_sock_fd);
          m_unix_sock_fd = -1;
          m_unix_sock_con = false;
        } else {
          INFO("Send %d bytes protocol messages to RTP muxer.", send_bytes);
          m_unix_sock_con = true;
        }
        if (AM_LIKELY(m_unix_sock_con)) {
          struct epoll_event event;
          event.data.fd = m_unix_sock_fd;
          event.events  = EPOLLIN | EPOLLRDHUP;
          if (AM_UNLIKELY(epoll_ctl(epfd, EPOLL_CTL_ADD,
                                    m_unix_sock_fd, &event) < 0)) {
            PERROR("epoll_ctrl EPOLL_CTL_ADD m_unix_sock_fd");
            break;
          }
          fdcount = 2;
        } else {
          fdcount = 1;
        }
      }
    } else {
      fdcount = 1;
    }

    timeout = m_unix_sock_con ? -1 : m_rtsp_config->rtsp_retry_interval;
    nfds = epoll_pwait(epfd, ev, fdcount, timeout, &mask);
    switch(nfds) {
      case -1: {
        if (AM_UNLIKELY(errno != EINTR)) {
          PERROR("epoll_wait");
        }
      }break;
      case 0: break; /* Wait timed out */
      default: {
        for (int i = 0; i < nfds; ++ i) {
          if (ev[i].events & EPOLLIN) {
            if (ev[i].data.fd == UNIX_SOCK_CTRL_R) {
              isok = process_ctrl_data(UNIX_SOCK_CTRL_R);
              if (AM_LIKELY(!isok)) {
                need_break = true;
                NOTICE("Close unix socket connection with RTP Muxer!");
              }
            } else if ((ev[i].data.fd == m_unix_sock_fd) &&
                       (m_unix_sock_fd >= 0)) {
              INFO("Received message from RTP muxer!");
              isok = recv_rtp_control_data();
            }
          } else if ((ev[i].events & EPOLLHUP) ||
                     (ev[i].events & EPOLLRDHUP)) {
            if (ev[i].data.fd == UNIX_SOCK_CTRL_R) {
              NOTICE("RTSP.Unix Control socket is closed!");
            } else if ((ev[i].data.fd == m_unix_sock_fd) &&
                       (m_unix_sock_fd >= 0)) {
              NOTICE("RTP Muxer closed its connection with RTSP server!");
              isok = false;
            }
            if (AM_UNLIKELY(epoll_ctl(epfd, EPOLL_CTL_DEL,
                                      m_unix_sock_fd, &ev[i]) < 0)) {
              PERROR("epoll_ctl");
            }
          } else {
            if (ev[i].data.fd == UNIX_SOCK_CTRL_R) {
              PERROR("UNIX_SOCK_CTRL_R");
            } else if ((ev[i].data.fd == m_unix_sock_fd) &&
                       (m_unix_sock_fd >= 0)) {
              NOTICE("Error occurred between RTP Muxer and RTSP Muxer!");
              isok = false;
            }
          }
        }
      }break;
    }
    if (AM_UNLIKELY(!m_unix_sock_con)) {
      m_unix_sock_fd = -1;
      m_unix_sock_fd = unix_socket_conn(RTP_CONTROL_SOCKET);
    }
    if (AM_UNLIKELY(!isok)) {
      NOTICE("Close UNIX socket connection with RTP Muxer!");
      unix_socket_close();
      NOTICE("Destroy all clients!");
      abort_all_client_session(false);
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
  } /* while */

  if (AM_LIKELY(epfd >= 0)) {
    close(epfd);
  }
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
  abort_all_client_session();
  while (m_run) {
    if (AM_LIKELY(SERVER_CTRL_W >= 0)) {
      ServerCtrlMsg msg = {
        .code = CMD_ABRT_ALL,
        .data = 0
      };
      ssize_t ret = am_write(SERVER_CTRL_W, &msg, sizeof(msg), 10);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send abort command to RTSP.server thread! %s",
              strerror(errno));
      }
      usleep(30000);
    }
  }
  while (m_unix_sock_run) {
    if (AM_LIKELY(UNIX_SOCK_CTRL_W >= 0)) {
      ServerCtrlMsg msg = {
        .code = CMD_ABRT_ALL,
        .data = 0
      };
      ssize_t ret = am_write(UNIX_SOCK_CTRL_W, &msg, sizeof(msg), 10);
      if (AM_UNLIKELY(ret != (ssize_t)sizeof(msg))) {
        ERROR("Failed to send abort command to RTSP.Unix thread! %s",
              strerror(errno));
      }
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

void AMRtspServer::abort_client(AMRtspClientSession &client, bool wait)
{
  uint32_t identifier = client.m_identify;
  std::string name = client.m_client_name;
  client.abort_client();

  while(wait && (m_client_map.end() != m_client_map.find(identifier))) {
    NOTICE("Wait for client %s to be deleted!", name.c_str());
    usleep(50000);
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
  AUTO_MTX_LOCK(m_client_mutex);
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
    freeifaddrs(interfaces);
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

    if (AM_UNLIKELY((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)) {
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
    if (AM_UNLIKELY(AMFile::exists(rtsp_server_name) &&
                    (unlink(rtsp_server_name) < 0))) {
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
                      MSG_DONTWAIT);
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
                        MSG_DONTWAIT);
        if (AM_LIKELY(read_ret > 0)) {
          received += read_ret;
        }
      }
      if (AM_LIKELY(received == msg->length)) {
        AUTO_MTX_LOCK(m_client_mutex);
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
      } else {
        WARN("Tried %u times, but got only %u bytes of data(recv: %s)! ",
             read_cnt, received, strerror(errno));
      }
    }
  }
  return isok;
}

void AMRtspServer::destroy_all_client_session()
{
  for (AMRtspClientMap::iterator iter = m_client_map.begin();
      iter != m_client_map.end();) {
    delete iter->second;
    iter = m_client_map.erase(iter);
  }
  m_client_map.clear();
}

void AMRtspServer::abort_all_client_session(bool wait)
{
  for (AMRtspClientMap::iterator iter = m_client_map.begin();
      iter != m_client_map.end(); ++ iter) {
    /* Clients will be destroyed in server_thread()
     * by command CMD_DEL_CLIENT
     */
    abort_client(*iter->second, wait);
  }
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

void AMRtspServer::reject_client(int fd)
{
  sockaddr_in addr;
  uint32_t accept_cnt = 0;
  socklen_t sock_len = sizeof(addr);
  int client_tcp_sock = -1;
  do {
    client_tcp_sock = accept(fd, (sockaddr*)&(addr), &sock_len);
  } while ((++ accept_cnt < 5) && (client_tcp_sock < 0) &&
      ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
          (errno == EINTR)));

  if (AM_UNLIKELY(client_tcp_sock < 0)) {
    PERROR("accept");
  } else {
    close(client_tcp_sock);
    NOTICE("Closed connection from client: %s:%u",
           inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  }
}

bool AMRtspServer::process_ctrl_data(int fd)
{
  ServerCtrlMsg msg = {0};
  uint32_t read_cnt = 0;
  ssize_t  read_ret = 0;
  bool          ret = true;

  do {
    read_ret = read(fd, &msg, sizeof(msg));
  } while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
           ((errno == EAGAIN) || (errno == EINTR)))));
  if (AM_UNLIKELY(read_ret < 0)) {
    PERROR("read");
  } else {
    switch(msg.code) {
      case CMD_DEL_CLIENT: {
        if (AM_LIKELY(fd == SERVER_CTRL_R)) {
          AUTO_MTX_LOCK(m_client_mutex);
          AMRtspClientMap::iterator iter = m_client_map.find(msg.data);
          if (AM_LIKELY(iter != m_client_map.end())) {
            AMRtspClientSession *client = iter->second;
            NOTICE("RTSP server is asked to delete client %s",
                   client->m_client_name.c_str());
            m_client_map.erase(msg.data);
            if (AM_LIKELY(m_client_map.size())) {
              NOTICE("%u %s still connected now!",
                     m_client_map.size(),
                     (m_client_map.size()>1) ? "clients are" : "client is");
            } else {
              NOTICE("No clients are connected now!");
            }
            delete client;
          }
        } else {
          ERROR("Impossible!");
        }
      }break;
      case CMD_ABRT_ALL:
      case CMD_STOP_ALL: {
        if (fd == SERVER_CTRL_R) {
          AUTO_MTX_LOCK(m_client_mutex);
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
        } else if (fd == UNIX_SOCK_CTRL_R) {
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
        }
        ret = false;
      }break;
      default: {
        WARN("Unknown command!");
      }break;
    }
  }

  return ret;
}

bool AMRtspServer::process_client_connect_request(int fd)
{
  bool ret = true;
  if(AM_UNLIKELY(!m_unix_sock_con)) {
    NOTICE("RTSP server is not ready!");
    reject_client(fd);
  } else {
    uint16_t udp_rtp_port = 0;
    uint32_t count = 0;
    AMRtspClientSession *session = nullptr;

    AUTO_MTX_LOCK(m_client_mutex);
    while(udp_rtp_port == 0) {
      uint32_t identify = (m_rtsp_config->rtp_stream_port_base +
          count * AM_RTP_MEDIA_NUM * 2) << 16;
      if (AM_LIKELY(m_client_map.find((const uint32_t)identify) ==
          m_client_map.end())) {
        /* If is identify doesn't exist, this port is available */
        udp_rtp_port = (identify & 0xffff0000) >> 16;
      } else {
        udp_rtp_port = 0;
        ++ count;
      }
    }
    session = new AMRtspClientSession(this, udp_rtp_port);
    if (AM_LIKELY(session)) {
      bool need_abort = false;

      if (AM_UNLIKELY(!session->init_client_session(fd))) {
        ERROR("Failed to initialize client session");
        need_abort = true;
      } else if (AM_LIKELY(m_client_map.size() <
                           m_rtsp_config->max_client_num)) {
        /* session->m_identify == (udp_rtp_port << 16) */
        m_client_map[session->m_identify] = session;
        NOTICE("%u %s now connected!", m_client_map.size(),
               (m_client_map.size() > 1) ? "clients are" : "client is");
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

  return ret;
}

AMRtspServer::AMRtspServer() :
    m_rtsp_config(nullptr),
    m_config(nullptr),
    m_server_thread(nullptr),
    m_sock_thread(nullptr),
    m_event(nullptr),
    m_client_mutex(nullptr),
    m_srv_sock_tcp(-1),
    m_unix_sock_fd(-1),
    m_port_tcp(0),
    m_run(false),
    m_unix_sock_con(false),
    m_unix_sock_run(false),
    m_ref_count(0)
{
  SERVER_CTRL_R = -1;
  SERVER_CTRL_W = -1;
  UNIX_SOCK_CTRL_R = -1;
  UNIX_SOCK_CTRL_W = -1;
  m_client_map.clear();
}

AMRtspServer::~AMRtspServer()
{
  destroy_all_client_session();
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
    if (AM_UNLIKELY(pipe2(m_pipe, O_CLOEXEC) < 0)) {
      PERROR("pipe");
      state = false;
      break;
    }

    if (AM_UNLIKELY(pipe2(m_ctrl_unix_fd, O_CLOEXEC) < 0)) {
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
