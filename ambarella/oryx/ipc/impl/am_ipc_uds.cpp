/*******************************************************************************
 * am_ipc_uds.cpp
 *
 * History:
 *   Jul 28, 2016 - [Shupeng Ren] created file
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

#include <assert.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <algorithm>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_io.h"

#include "am_ipc_uds.h"

#define AM_IPC_CLOSE_FD(fd) \
  do { \
    if (fd > 0) { \
      close(fd); \
      fd = -1; \
    } \
  } while (0)

/*
 * UNIX Domain Socket Server
 */

#define BACKLOG 10

AMIPCServerUDSPtr AMIPCServerUDS::create(const char *path)
{
  AMIPCServerUDS *result = new AMIPCServerUDS();
  if (result && (result->construct(path) != AM_RESULT_OK)) {
    delete result;
    result = nullptr;
  }
  return AMIPCServerUDSPtr(result, [](AMIPCServerUDS *p){delete p;});
}

AM_RESULT AMIPCServerUDS::construct(const char *path)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketpair_fd) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("socketpair:");
      break;
    }

    remove(path);
    if ((m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("open socket:");
      break;
    }

    sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(m_sockfd, (sockaddr*)&addr, sizeof(sockaddr_un)) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("bind:");
      break;
    } else {
      m_sock_path = std::string(path);
    }

    if (listen(m_sockfd, BACKLOG) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("listen:");
      break;
    }

    if (!(m_thread = std::shared_ptr<std::thread>(
                       new std::thread(static_socket_process, this),
                       [](std::thread *p){p->join();delete p;}))) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create socket process thread!");
      break;
    }
  } while (0);
  return result;
}

AMIPCServerUDS::AMIPCServerUDS()
{
}

AMIPCServerUDS::~AMIPCServerUDS()
{
  ssize_t ret = am_write(m_socketpair_fd[0], "quit", 4, 10);
  if (ret != 4) {
    ERROR("Failed to send \"quit\" command to client with fd %d: %s!",
          m_socketpair_fd[0], strerror(errno));
  }
  m_thread = nullptr;
  m_recv_cond.notify_all();
  for (auto &v : m_connected_fds) {
    close(v);
  }
  m_connected_fds.clear();
  AM_IPC_CLOSE_FD(m_sockfd);
  remove(m_sock_path.c_str());
  AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
  AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
}

void AMIPCServerUDS::static_socket_process(AMIPCServerUDS *p)
{
  if (p) {
    p->socket_process();
  }
}

#define MAX_EVENT_NUM 20
void AMIPCServerUDS::socket_process()
{
  int epfd = -1;
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);

  do {
    epoll_event ev = {0}, events[MAX_EVENT_NUM] = {0};
    if ((epfd = epoll_create1(0)) < 0) {
      PERROR("epoll_create1");
      break;
    }
    //Add control fd to epoll
    ev.data.fd = m_socketpair_fd[1];
    ev.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, m_socketpair_fd[1], &ev) < 0) {
      PERROR("epoll_ctl");
      ERROR("Server process thread is out of control!");
    }
    //Add socket fd to epoll
    ev.data.fd = m_sockfd;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, m_sockfd, &ev) < 0) {
      PERROR("epoll_ctl");
      break;
    }

    bool run_flag = true;
    while (run_flag) {
      int nfds = epoll_pwait(epfd, events, MAX_EVENT_NUM, -1, &sigmask);
      if ((nfds < 0 && errno == EINTR) || (nfds == 0)) {
        continue;
      } else if (nfds < 0) {
        PERROR("epoll_pwait, quit!");
      }

      for (int i = 0; i < nfds; ++i) {
        if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
          if (events[i].data.fd == m_sockfd) {
            AM_IPC_CLOSE_FD(m_sockfd);
            run_flag = false;
            ERROR("Fatal error: socket fd invalid! exit!");
            break;
          }
          close_and_remove_clientfd(events[i].data.fd);
          continue;
        }

        if (events[i].data.fd == m_sockfd) {
          int connected_fd = accept(m_sockfd, nullptr, nullptr);
          if (connected_fd < 0) {
            PERROR("accept:");
            continue;
          } else {
            m_reading[connected_fd] = false;
            AUTO_MEM_LOCK(m_connect_cb_lock);
            if (connect_callback) {
              connect_callback(m_connect_callback_context, connected_fd);
            }
          }
          m_connected_fds.push_back(connected_fd);
          NOTICE("Client[%d] connected! Total: %d",
                 connected_fd, m_connected_fds.size());
          ev.data.fd = connected_fd;
          ev.events = EPOLLIN | EPOLLRDHUP;
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, connected_fd, &ev) < 0) {
            PERROR("epoll_ctl add:");
            ERROR("Failed to add %d", connected_fd);
            continue;
          }
        } else if (events[i].events & EPOLLIN) {
          if (events[i].data.fd == m_socketpair_fd[1]) {
            run_flag = false;
            INFO("Received quit command, exit socket process thread!");
            break;
          }

          int recv_fd = events[i].data.fd;
          {
            AUTO_MEM_LOCK(m_receive_cb_lock);
            if (receive_callback) {
              receive_callback(m_receive_callback_context, recv_fd);
            }
          }
          {
            std::unique_lock<std::mutex> lk(m_recv_mutex);
            while (m_reading[recv_fd]) {
              wait_recv_done = true;
              m_recv_cond.wait(lk);
            }
            wait_recv_done = false;
          }
        }
      }
    }
  } while (0);
  if (epfd > 0) {
    close(epfd);
    epfd = -1;
  }

  INFO("UDS Server process thread is exit!");
}

AM_RESULT AMIPCServerUDS::send(int fd, const uint8_t *data, int32_t size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!check_clientfd(fd)) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("fd: %d is invalid!");
      break;
    }
    int sent_size = 0;
    if ((sent_size = ::write(fd, data, size)) != size) {
      if (sent_size <= 0) {
        close_and_remove_clientfd(fd);
        result = AM_RESULT_ERR_IO;
        PERROR("Failed to send data to client! Shutdown connection!");
        break;
      }
      ERROR("Write incomplete! Target: %d, real: %d", size, sent_size);
    }
  } while (0);
  return result;
}

AM_RESULT AMIPCServerUDS::send_to_all(const uint8_t *data, int32_t size)
{
  AM_RESULT result = AM_RESULT_OK;
  for (auto &v : m_connected_fds) {
    send(v, data, size);
  }
  return result;
}

AM_RESULT AMIPCServerUDS::recv(int fd, uint8_t *data, int32_t &size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!check_clientfd(fd)) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("fd: %d is invalid!");
      break;
    }
    std::unique_lock<std::mutex> lk(m_recv_mutex);
    m_reading[fd] = true;
    int read_size = 0;
    if ((read_size = ::read(fd, data, size)) != size) {
      if ((size = read_size) <= 0) {
        close_and_remove_clientfd(fd);
        result = AM_RESULT_ERR_IO;
        PERROR("read:");
        ERROR("Failed to reveive data from client[%d]! Shutdown connection!",
              fd);
        break;
      }
    }
    m_reading[fd] = false;
    if (wait_recv_done) {
      m_recv_cond.notify_one();
    }
  } while (0);

  return result;
}

AM_RESULT AMIPCServerUDS::register_connect_cb(notify_cb_t cb, int32_t context)
{
  AM_RESULT result = AM_RESULT_OK;
  AUTO_MEM_LOCK(m_connect_cb_lock);
  if (cb) {
    m_connect_callback_context = context;
    connect_callback = cb;
  }

  return result;
}

AM_RESULT AMIPCServerUDS::register_disconnect_cb(notify_cb_t cb, int32_t context)
{
  AM_RESULT result = AM_RESULT_OK;
  AUTO_MEM_LOCK(m_disconnect_cb_lock);
  if (cb) {
    m_disconnect_callback_context = context;
    disconnect_callback = cb;
  }

  return result;
}

AM_RESULT AMIPCServerUDS::register_receive_cb(notify_cb_t cb, int32_t context)
{
  AM_RESULT result = AM_RESULT_OK;
  AUTO_MEM_LOCK(m_receive_cb_lock);
  if (cb) {
    m_receive_callback_context = context;
    receive_callback = cb;
  }
  return result;
}

void AMIPCServerUDS::close_and_remove_clientfd(int fd)
{
  auto it = std::find_if(m_connected_fds.begin(), m_connected_fds.end(),
                         [=](int f) -> bool {return (f == fd);});
  if (it != m_connected_fds.end()) {
    m_connected_fds.erase(it);
    m_reading.erase(fd);
    close(fd);
    NOTICE("Client[%d] is removed! Total: %d", fd, m_connected_fds.size());
    AUTO_MEM_LOCK(m_disconnect_cb_lock);
    if (disconnect_callback) {
      disconnect_callback(m_disconnect_callback_context, fd);
    }
  }
}

bool AMIPCServerUDS::check_clientfd(int fd)
{
  return std::find_if(m_connected_fds.begin(), m_connected_fds.end(),
                      [=](int f) -> bool {return (f == fd);}) !=
                          m_connected_fds.end();
}

/*
 * UNIX Domain Socket Client
 */

enum UDS_CLIENT_CMD {
  UDS_CLIENT_QUIT   = 'Q',
  UDS_CLIENT_ADD    = 'A',
  UDS_CLIENT_CLOSE  = 'C',
};

AMIPCClientUDSPtr AMIPCClientUDS::create()
{
  AMIPCClientUDS *result = new AMIPCClientUDS();
  if (result && (result->construct() != AM_RESULT_OK)) {
    delete result;
    result = nullptr;
  }
  return AMIPCClientUDSPtr(result, [](AMIPCClientUDS *p){delete p;});
}

AM_RESULT AMIPCClientUDS::construct()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketpair_fd) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("socketpair:");
      break;
    }

    if ((m_epoll_fd = epoll_create1(0)) < 0) {
      PERROR("epoll_create1:");
      break;
    }

    if (!(m_thread = std::shared_ptr<std::thread>(
                       new std::thread(static_socket_process, this),
                       [](std::thread *p){p->join();delete p;}))) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create socket process thread!");
      break;
    }
  } while (0);
  return result;
}

AMIPCClientUDS::AMIPCClientUDS()
{
}

AMIPCClientUDS::~AMIPCClientUDS()
{
  m_recv_cond.notify_all();
  int cmd = UDS_CLIENT_QUIT;
  if (am_write(m_socketpair_fd[0], &cmd, 1, 5) != 1) {
    ERROR("Failed to send \"quit\" command to client with fd %d: %s!",
          m_socketpair_fd[0], strerror(errno));
  }
  m_thread = nullptr;
  AM_IPC_CLOSE_FD(m_sockfd);
  AM_IPC_CLOSE_FD(m_epoll_fd);
  AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
  AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
}

AM_RESULT AMIPCClientUDS::connect(const char *path,
                                  int32_t retry,
                                  int32_t retry_delay_ms)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AUTO_MEM_LOCK(m_connect_lock);
    if (m_sockfd > 0) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("State is connected, please disconnect it first!");
      break;
    }

    if ((m_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("open socket:");
      break;
    }

    sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    for (int i = 0; i < retry; ++i) {
      if (::connect(m_sockfd, (sockaddr*)&addr, sizeof(sockaddr_un)) < 0) {
        result = AM_RESULT_ERR_IO;
        NOTICE("Failed to connect to server: %s. Try again!", path);
      } else {
        break;
      }
      timeval tv = {retry_delay_ms / 1000, (retry_delay_ms % 1000) * 1000};
      select(0, nullptr, nullptr, nullptr, &tv);
    }
    if (result != AM_RESULT_OK) {
      ERROR("Tried %d times, can't connect to server: %s.", retry, path);
      break;
    }

    int cmd = UDS_CLIENT_ADD;
    if (am_write(m_socketpair_fd[0], &cmd, 1, 5) != 1) {
      ERROR("Failed to send \"add\" command to client with fd %d: %s!",
            m_socketpair_fd[0], strerror(errno));
    }
  } while (0);

  return result;
}

AM_RESULT AMIPCClientUDS::distconnect()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AUTO_MEM_LOCK(m_connect_lock);
    int cmd = UDS_CLIENT_CLOSE;
    if (am_write(m_socketpair_fd[0], &cmd, 1, 5) != 1) {
      ERROR("Failed to send \"close\" command to client with fd %d: %s!",
            m_socketpair_fd[0], strerror(errno));
    }
  } while (0);

  return result;
}

AM_RESULT AMIPCClientUDS::send(const uint8_t *data, int32_t size)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    AUTO_MEM_LOCK(m_connect_lock);
    if (m_sockfd < 0) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Please connect to server first!");
      break;
    }
    int sent_size = 0;
    if ((sent_size = ::write(m_sockfd, data, size)) != size) {
      if (sent_size <= 0) {
        AM_IPC_CLOSE_FD(m_sockfd);
        result = AM_RESULT_ERR_IO;
        PERROR("Failed to send data to server! Shutdown connection!");
        break;
      }
      ERROR("Write incomplete! Target: %d, real: %d", size, sent_size);
    }
  } while (0);

  return result;
}

AM_RESULT AMIPCClientUDS::recv(uint8_t *data, int32_t &size)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AUTO_MEM_LOCK(m_connect_lock);
    if (m_sockfd < 0) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Please connect to server first!");
      break;
    }
    std::unique_lock<std::mutex> lk(m_recv_mutex);
    int read_size = 0;
    if ((read_size = ::read(m_sockfd, data, size)) != size) {
      if ((size = read_size) <= 0) {
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_sockfd, nullptr);
        AM_IPC_CLOSE_FD(m_sockfd);
        ERROR("Failed to read data from server!");
      }
    }
  } while (0);
  return result;
}

void AMIPCClientUDS::static_socket_process(AMIPCClientUDS *p)
{
  if (p) {p->socket_process();}
}

void AMIPCClientUDS::socket_process()
{
  sigset_t sigmask;
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);

  epoll_event ev = {0}, events[2] = {0};
  //Add control fd to epoll
  ev.data.fd = m_socketpair_fd[1];
  ev.events = EPOLLIN | EPOLLRDHUP;
  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_socketpair_fd[1], &ev) < 0) {
    PERROR("epoll_ctl:");
    ERROR("Client process thread is out of control!");
  }
  bool run_flag = true;
  while (run_flag) {
    int nfds = epoll_pwait(m_epoll_fd, events, 2, -1, &sigmask);
    if ((nfds < 0 && errno == EINTR) || (nfds == 0)) {
      continue;
    } else if (nfds < 0) {
      PERROR("epoll_pwait, quit!");
    }

    for (int i = 0; i < nfds; ++i) {
      int recv_fd = events[i].data.fd;
      if ((events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))) {
        AUTO_MEM_LOCK(m_connect_lock);
        if (recv_fd == m_sockfd) {
          epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_sockfd, nullptr);
          AM_IPC_CLOSE_FD(m_sockfd);
          ERROR("Server is shutdown!");
          continue;
        } else if (recv_fd == m_socketpair_fd[1])  {
          epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_socketpair_fd[1], nullptr);
          AM_IPC_CLOSE_FD(m_socketpair_fd[0]);
          AM_IPC_CLOSE_FD(m_socketpair_fd[1]);
          ERROR("Client process thread is out of control!");
        } else {
          ERROR("Unknown event fd: %d", recv_fd);
        }
      }
      if (events[i].events & EPOLLIN) {
        if (recv_fd == m_socketpair_fd[1]) {
          char cmd = 0;
          if(am_read(recv_fd, &cmd, 1) < 0) {
            PERROR("read");
            continue;
          }
          switch (cmd) {
            case UDS_CLIENT_QUIT: {
              NOTICE("Received command: %c", cmd);
              run_flag = false;
            } break;
            case UDS_CLIENT_ADD: {
              NOTICE("Received command: %c", cmd);
              epoll_event ev = {0};
              ev.data.fd = m_sockfd;
              ev.events = EPOLLIN | EPOLLRDHUP;
              if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_sockfd, &ev) < 0) {
                PERROR("epoll_ctl");
              }
            } break;
            case UDS_CLIENT_CLOSE: {
              NOTICE("Received command: %c", cmd);
              epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_sockfd, nullptr);
              AM_IPC_CLOSE_FD(m_sockfd);
            } break;
            default: {
              ERROR("Unknown command: %c", cmd);
            } break;
          }
          break;
        }

        assert(recv_fd == m_sockfd);
        m_read_available = true;
        {
          AUTO_MEM_LOCK(m_receive_cb_lock);
          if (receive_callback) {
            receive_callback(m_receive_callback_context, recv_fd);
          }
        }
      }
    }
  }
}

AM_RESULT AMIPCClientUDS::register_receive_cb(notify_cb_t cb, int32_t context)
{
  AM_RESULT result = AM_RESULT_OK;
  {
    AUTO_MEM_LOCK(m_receive_cb_lock);
    m_receive_callback_context = context;
    receive_callback = cb;
  }
  return result;
}
