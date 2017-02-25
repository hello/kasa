/*******************************************************************************
 * am_export_client_uds.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-02 - [Shupeng Ren] modified file
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
#include "am_io.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_amf_types.h"
#include "am_video_address_if.h"

#include "am_export_if.h"
#include "am_export_client_uds.h"

#include <sys/types.h>
#include <sys/socket.h>

AMIExportClient* am_create_export_client_uds(AMExportConfig *config)
{
  return AMExportClientUDS::create(config);
}

AMExportClientUDS::AMExportClientUDS()
{}

AMExportClientUDS::AMExportClientUDS(AMExportConfig *config) :
  m_config(*config)
{}

AMExportClientUDS::~AMExportClientUDS()
{
  m_run = false;
  disconnect_server();
  m_event->signal();
  if (0 < m_control_fd[0]) {
    close(m_control_fd[0]);
    m_control_fd[0] = -1;
  }

  if (0 < m_control_fd[1]) {
    close(m_control_fd[1]);
    m_control_fd[1] = -1;
  }
  AM_DESTROY(m_recv_thread);
  AM_DESTROY(m_cond);
  AM_DESTROY(m_mutex);
  AM_DESTROY(m_event);
  m_video_address = nullptr;
}

AMExportClientUDS* AMExportClientUDS::create(AMExportConfig *config)
{
  AMExportClientUDS *result = new AMExportClientUDS(config);
  if (result && (result->init() != AM_RESULT_OK)) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMExportClientUDS::init()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!(m_event = AMEvent::create())) {
      ERROR("Failed to create event");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(m_mutex = AMMutex::create())) {
      ERROR("Failed to create mutex!");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(m_cond = AMCondition::create())) {
      ERROR("Failed to create condition!");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, m_control_fd) < 0) {
      ERROR("Failed to create control fd!");
      ret = AM_RESULT_ERR_IO;
      break;
    }

    if (!(m_video_address = AMIVideoAddress::get_instance())) {
      ERROR("Failed to create m_video_address!");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(m_recv_thread = AMThread::create("uds_recv_data",
                                           thread_entry,
                                           this))) {
      ERROR("Failed to create receive thread!");
      ret = AM_RESULT_ERR_MEM;
      break;
    }
  } while (0);

  return ret;
}

void AMExportClientUDS::destroy()
{
  delete this;
}

AM_RESULT AMExportClientUDS::connect_server(const char *url)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (AM_UNLIKELY(!url)) {
      ERROR("Connect(url == NULL)");
      ret = AM_RESULT_ERR_DATA_POINTER;
      break;
    }

    if (m_is_connected) {
      NOTICE("already connected!");
      ret = AM_RESULT_ERR_ALREADY;
      break;
    }

    if (access(url, F_OK)) {
      ERROR("Failed to access(%s, F_OK). server does not run?", url);
      ret = AM_RESULT_ERR_FILE_EXIST;
      break;
    }

    if ((m_socket_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR("socket(PF_UNIX, SOCK_SEQPACKET, 0) failed");
      ret = AM_RESULT_ERR_IO;
      break;
    }
    memset(&m_addr, 0, sizeof(sockaddr_un));

    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", url);
    if (connect(m_socket_fd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ERROR("connect failed: %s", strerror(errno));
      ret = AM_RESULT_ERR_CONN_REFUSED;
      break;
    }

    if (am_write(m_socket_fd, &m_config, sizeof(m_config)) != sizeof(m_config)) {
      ERROR("Failed to write config to server!: %s", strerror(errno));
      ret = AM_RESULT_ERR_IO;
      break;
    }
    if (!check_sort_mode()) {
      ret = AM_RESULT_ERR_INVALID;
      ERROR("The sort mode has been set, please change");
      break;
    }

    FD_ZERO(&m_all_set);
    FD_ZERO(&m_read_set);
    FD_SET(m_control_fd[0], &m_all_set);
    FD_SET(m_socket_fd, &m_all_set);
    m_max_fd = AM_MAX(m_socket_fd, m_control_fd[0]);
    m_is_connected = true;
    m_event->signal();
  } while (0);
  return ret;
}

void AMExportClientUDS::disconnect_server()
{
  if (m_is_connected) {
    char write_char = 'q';
    if (am_write(m_control_fd[1], &write_char, 1) != 1) {
      ERROR("Write socketpair error!");
    } else {
      char read_char;
      if (am_read(m_control_fd[1], &read_char, 1) != 1) {
        ERROR("read socketpair error!");
      }
    }
  }

  if (m_socket_fd > 0) {
    close(m_socket_fd);
    m_socket_fd = -1;
  }

  while (!m_packet_queue.empty()) {
    release(&m_packet_queue.front());
    m_packet_queue.pop_front();
  }

  m_cond->signal();
}

AM_RESULT AMExportClientUDS::set_config(AMExportConfig *config)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
      if (!config) {
        ERROR("config parameter is a null pointer!");
        ret = AM_RESULT_ERR_DATA_POINTER;
        break;
      }
      if (!m_is_connected) {
        ERROR("No connection to server!");
        ret = AM_RESULT_ERR_NO_CONN;
        break;
      }
      am_write(m_socket_fd, config, sizeof(*config), 5);
  } while (0);

  return ret;
}

AM_RESULT AMExportClientUDS::receive(AMExportPacket *packet)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (!packet) {
      ERROR("ReceivePacket(packet == NULL)");
      ret = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (!m_is_connected) {
      ERROR("No connection to server!");
      ret = AM_RESULT_ERR_NO_CONN;
      break;
    }
    m_mutex->lock();
    while (m_packet_queue.empty()) {
      m_block = true;
      m_cond->wait(m_mutex);
      if (!m_is_connected) {
        m_mutex->unlock();
        ERROR("Connection is broken for server quit!");
        ret = AM_RESULT_ERR_SERVER_DOWN;
        break;
      }
    }
    if (ret != AM_RESULT_OK) {
      break;
    }
    m_block = false;
    *packet = m_packet_queue.front();
    m_packet_queue.pop_front();
    m_mutex->unlock();
  } while (0);

  return ret;
}

void AMExportClientUDS::release(AMExportPacket *packet)
{
  if (!packet->is_direct_mode) {
    delete [] packet->data_ptr;
  }
  packet->data_ptr = nullptr;
  packet->data_size = 0;
}

void AMExportClientUDS::thread_entry(void *arg)
{
  if (!arg) {
    ERROR("Thread argument is null!");
    return;
  }

  AMExportClientUDS *p = (AMExportClientUDS*)arg;
  do {
    p->m_event->wait();
    if (!p->get_data()) {
      ERROR("Get export packet data error!");
      break;
    }
  } while (p->m_run);
}

bool AMExportClientUDS::get_data()
{
  bool ret = false;
  do {
    AMExportPacket packet;
    if (!m_is_connected) {
      ERROR("Not connected!");
      break;
    }
    m_read_set = m_all_set;
    if (select(m_max_fd + 1, &m_read_set, nullptr, nullptr, nullptr) < 0) {
      ERROR("select failed, quit!");
      break;
    }

    if (FD_ISSET(m_control_fd[0], &m_read_set)) {
      char read_char = 0;
      if (am_read(m_control_fd[0], &read_char, 1) == 1) {
        if ((read_char == 'q') &&
            (am_write(m_control_fd[0], &read_char, 1) == 1)) {
          INFO("read command: %c, quit", read_char);
          break;
        }
      }
    } else if (FD_ISSET(m_socket_fd, &m_read_set)) {
      int val = am_read(m_socket_fd, &packet, sizeof(AMExportPacket));
      if (sizeof(AMExportPacket) != (uint32_t) val) {
        ERROR("Failed to read header, ret %d, server quit", val);
        m_is_connected = false;
        break;
      }
      if (!packet.is_direct_mode) {
        if (packet.data_size == 0) {
          ret = true;
          break;
        }
        if (!(packet.data_ptr = new uint8_t[packet.data_size])) {
          ERROR("Failed to new data!");
          break;
        }
        val = am_read(m_socket_fd, packet.data_ptr, packet.data_size);
        if (AM_UNLIKELY(packet.data_size != (uint32_t )val)) {
          ERROR("Failed to read data, ret %d, server quit", ret);
          m_is_connected = false;
          break;
        }
      } else {
        AM_DATA_FRAME_TYPE type = AM_DATA_FRAME_TYPE_VIDEO;
        AMAddress addr;
        m_video_address->addr_get(type, (uint32_t)packet.data_ptr, addr);
        packet.data_ptr = addr.data;
      }

      m_mutex->lock();
      while (m_packet_queue.size() > 64) {
        for (auto it = m_packet_queue.begin(); it != m_packet_queue.end();
            ++ it) {
          if ((it->packet_type == AM_EXPORT_PACKET_TYPE_VIDEO_DATA) ||
              (it->packet_type == AM_EXPORT_PACKET_TYPE_AUDIO_DATA)) {
            AMExportPacket data = *it;
            release(&data);
            m_packet_queue.erase(it);
            WARN("Drop one packet!");
            break;
          }
        }
      }
      m_packet_queue.push_back(packet);
      if (m_block) {
        m_cond->signal();
      }
      m_mutex->unlock();
      ret = true;
    }
  } while (m_is_connected);
  m_is_connected = false;
  m_cond->signal();
  return ret;
}

bool AMExportClientUDS::check_sort_mode()
{
  char tem_check;
  bool ret = true;
  do {
    if ((am_read(m_socket_fd, &tem_check, sizeof(tem_check))) != 1) {
      PERROR("read failed");
      ret = false;
      break;
    }
    if (tem_check == 'Q') {
      ret = false;
      break;
    }
  } while(0);
  return ret;
}
