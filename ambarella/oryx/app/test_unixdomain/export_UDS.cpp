/*******************************************************************************
 * export_UDS.cpp
 *
 * History:
 *   2016-07-18 - [Guohua Zheng]  created file
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
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <poll.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_file.h"
#include "am_thread.h"
#include "am_event.h"

#include "export_UDS.h"

#define DEXPORT_PATH ("/run/oryx/export.socket")
#define LARGE_NUM 512
#define SMALL_NUM 25

void AMMuxerExportUDS::destroy()
{
  delete this;
}

bool AMMuxerExportUDS::init()
{
  bool ret = false;
  m_export_running = true;
  m_monitor_running = true;

  do {
    if (!(m_thread_monitor_wait = AMEvent::create())) {
      printf("Failed to create m_thread_monitor_wait!");
      break;
    }
    if (!(m_thread_export_wait = AMEvent::create())) {
      printf("Failed to create m_thread_export_wait!");
    }

    if (pipe(m_control_fd) < 0) {
      printf("Failed to create control fd!");
      break;
    }

    if (!(m_monitor_thread = AMThread::create("monitor_thread",
                                              thread_monitor_entry, this))) {
      printf("Failed to create monitor_thread");
      break;
    }

    if (!(m_export_thread = AMThread::create("export_thread",
                                              thread_export_entry, this))) {
      printf("Failed to create export_thread");
      break;
    }

    ret = true;
  } while (0);

  return ret;
}

AMMuxerExportUDS::AMMuxerExportUDS()
{}

void AMMuxerExportUDS::clean_resource()
{
  reset_resource();
  if (m_control_fd[0] > 0) {
    close(m_control_fd[0]);
    m_control_fd[0] = -1;
  }
  if (m_control_fd[1] > 0) {
    close(m_control_fd[1]);
    m_control_fd[1] = -1;
  }
  m_thread_exit = true;
  m_thread_monitor_exit = true;
  m_thread_export_exit = true;
  m_thread_monitor_wait->signal();
  m_thread_export_wait->signal();
  AM_DESTROY(m_monitor_thread);
  AM_DESTROY(m_export_thread);
  AM_DESTROY(m_thread_monitor_wait);
  AM_DESTROY(m_thread_export_wait);
}

void AMMuxerExportUDS::reset_resource()
{
  if (m_connect_fd > 0) {
    close(m_connect_fd);
    m_connect_fd = -1;
  }

  if (m_socket_fd > 0) {
    close(m_socket_fd);
    m_socket_fd = -1;
    unlink(DEXPORT_PATH);
  }

  m_video_map = 0;
  m_audio_map = 0;
  m_send_block = false;
  m_send_cond.notify_all();
}

AMMuxerExportUDS::~AMMuxerExportUDS()
{
  clean_resource();
}

AM_STATE AMMuxerExportUDS::start()
{
  AM_STATE ret_state = AM_STATE_OK;

  do {
    INFO("ExportUDS start!");
    FD_ZERO(&m_all_set);
    FD_ZERO(&m_read_set);
    FD_SET(m_control_fd[0], &m_all_set);

    if ((m_socket_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR("Failed to create socket!\n");
      ret_state = AM_STATE_OS_ERROR;
      break;
    }

    FD_SET(m_socket_fd, &m_all_set);
    m_max_fd = AM_MAX(m_socket_fd, m_control_fd[0]);

    if (AMFile::exists(DEXPORT_PATH)) {
      NOTICE("%s already exists! Remove it first!", DEXPORT_PATH);
      if (unlink(DEXPORT_PATH) < 0) {
        PERROR("unlink");
        ret_state = AM_STATE_OS_ERROR;
        break;
      }
    }

    if (!AMFile::create_path(AMFile::dirname(DEXPORT_PATH).c_str())) {
      ERROR("Failed to create path %s", AMFile::dirname(DEXPORT_PATH).c_str());
      ret_state = AM_STATE_OS_ERROR;
      break;
    }

    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", DEXPORT_PATH);

    if (bind(m_socket_fd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ERROR("bind(%d) failed!", m_socket_fd);
      ret_state = AM_STATE_OS_ERROR;
      break;
    }

    if (listen(m_socket_fd, 5) < 0) {
      ERROR("listen(%d) failed", m_socket_fd);
      ret_state = AM_STATE_OS_ERROR;
      break;
    }

    m_running = true;
    m_monitor_running = true;
    m_export_running = true;
    m_client_connected = false;
    m_thread_monitor_wait->signal();
    m_thread_export_wait->signal();
  } while(0);

  return ret_state;
}

AM_STATE AMMuxerExportUDS::stop()
{
  if (m_socket_fd > 0) {
    m_running = false;
    m_monitor_running = false;
    m_export_running = false;
    m_export_cond.notify_all();
    if (m_control_fd[1] > 0) {
      write(m_control_fd[1], "q", 1);
    }
    shutdown(m_socket_fd, SHUT_RD);
    reset_resource();
  }

  return AM_STATE_OK;
}


bool AMMuxerExportUDS::is_running()
{
  return m_running;
}

void AMMuxerExportUDS::thread_monitor_entry(void *arg)
{
  if(!arg){
    ERROR("Thread monitor argument is null");
    return;
  }
  AMMuxerExportUDS *p = (AMMuxerExportUDS*)arg;
  do {
      p->m_thread_monitor_wait->wait();
      p->monitor_loop();
    }while (!p->m_thread_monitor_exit);
}

void AMMuxerExportUDS::thread_export_entry(void *arg)
{
  if (!arg) {
    ERROR("Thread data export argument is null");
    return;
  }
  AMMuxerExportUDS *p = (AMMuxerExportUDS*)arg;
  do {
      p->m_thread_export_wait->wait();
      p->data_export_loop();
    }while (!p->m_thread_export_exit);
}

void AMMuxerExportUDS::monitor_loop()
{
  socklen_t addr_len;
  while(m_monitor_running) {
    if ((m_connect_fd = accept(m_socket_fd,
                               (sockaddr*)&m_addr,
                               &addr_len)) < 0) {
      NOTICE("muxer-export will exit");
    } else {
      {
        std::unique_lock<std::mutex> lk(m_clients_mutex);
        struct pollfd tem;
        tem.fd = m_connect_fd;
        tem.events = POLLOUT;
        m_clients_fds.push_back(tem);
        m_client_connected = true;
        NOTICE("clients number is %d", m_clients_fds.size());
        m_max_clients_fd = AM_MAX(m_max_clients_fd, m_connect_fd);
        if (m_export_send_block) {
          m_export_cond.notify_one();
        }
      }
    }
  }
}
void AMMuxerExportUDS::data_export_loop()
{
  while (m_export_running) {
    {
      std::unique_lock<std::mutex> lk(m_clients_mutex);
      while (m_clients_fds.empty() && m_export_running) {
        m_export_send_block = true;
        m_max_clients_fd = -1;
        m_export_cond.wait(lk);
      }
      m_export_send_block = false;
    }
    struct pollfd *tem_fds_set;
    int tem_fds_len;
    int tem_count = 0;
    tem_fds_len = m_clients_fds.size();
    tem_fds_set = new pollfd[tem_fds_len + 1];
    for (std::list<pollfd>:: iterator it = m_clients_fds.begin();
        it != m_clients_fds.end(); ++it) {
      tem_fds_set[tem_count++] = *it;
    }
    tem_fds_set[tem_fds_len].fd = m_control_fd[0];
    char check_str[1024];
    if (poll(tem_fds_set, tem_fds_len + 1, 0)) {//block mode
    }
    {
      std::unique_lock<std::mutex> lk(m_clients_mutex);
      m_clients_fds.clear();
      for (int i = 0; i < tem_fds_len; i++) {
        if (!(tem_fds_set[i].revents & POLLHUP)) {
          m_clients_fds.push_back(tem_fds_set[i]);
        }
      }
    }

    for (int i = 0; i < tem_fds_len; i++) {
      if (!(tem_fds_set[i].revents & POLLHUP)) {
        if ((read(tem_fds_set[i].fd, check_str, sizeof(check_str)))) {
          if ((write(tem_fds_set[i].fd, check_str, sizeof(check_str)))) {
          }
        }
      } else {
        NOTICE("clients number is %d", m_clients_fds.size());
      }
    }
    delete [] tem_fds_set;
  }
}

