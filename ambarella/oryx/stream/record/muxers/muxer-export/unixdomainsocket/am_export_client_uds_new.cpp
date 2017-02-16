/*******************************************************************************
 * am_export_client_uds.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-02 - [Shupeng Ren] modified file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
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
#include "am_event.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_amf_types.h"
#include "am_video_address_if.h"

#include "am_export_if.h"
#include "am_export_client_uds_new.h"

#include <sys/types.h>
#include <sys/socket.h>

AMIExportClient* am_create_export_client_uds(AMExportConfig *config)
{
  return AMExportClientUDS::create(config);
}

AMExportClientUDS::AMExportClientUDS() :
  m_iav_fd(-1),
  m_socket_fd(-1),
  m_max_fd(-1),
  m_run(false),
  m_block(false),
  m_is_connected(false),
  m_event(nullptr),
  m_mutex(nullptr),
  m_cond(nullptr),
  m_recv_thread(nullptr),
  m_video_address(nullptr)
{
  m_config.need_sort = 0;
  m_control_fd[0] = m_control_fd[1] = -1;
}

AMExportClientUDS::AMExportClientUDS(AMExportConfig *config) :
  m_iav_fd(-1),
  m_socket_fd(-1),
  m_max_fd(-1),
  m_run(false),
  m_block(false),
  m_is_connected(false),
  m_event(nullptr),
  m_mutex(nullptr),
  m_cond(nullptr),
  m_recv_thread(nullptr),
  m_config(*config),
  m_video_address(nullptr)
{
  m_control_fd[0] = m_control_fd[1] = -1;
}

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
  if (result && !result->init()) {
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMExportClientUDS::init()
{
  bool ret = false;
  do {
    if (!(m_event = AMEvent::create())) {
      ERROR("Failed to create event");
      break;
    }

    if (!(m_mutex = AMMutex::create())) {
      ERROR("Failed to create mutex!");
      break;
    }

    if (!(m_cond = AMCondition::create())) {
      ERROR("Failed to create condition!");
      break;
    }

    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, m_control_fd) < 0) {
      ERROR("Failed to create control fd!");
      break;
    }

    if (!(m_video_address = AMIVideoAddress::get_instance())) {
      ERROR("Failed to create m_video_address!");
      break;
    }

    if (!(m_recv_thread = AMThread::create("uds_recv_data",
                                           thread_entry,
                                           this))) {
      ERROR("Failed to create receive thread!");
      break;
    }

    ret = true;
  } while (0);
  return ret;
}

void AMExportClientUDS::destroy()
{
  delete this;
}

bool AMExportClientUDS::connect_server(const char* url)
{
  bool ret = false;
  do {
    if (AM_UNLIKELY(!url)) {
      ERROR("Connect(url == NULL)");
      break;;
    }

    if (m_is_connected) {
      NOTICE("already connected!");
      ret = true;
      break;
    }

    if (access(url, F_OK)) {
      ERROR("Failed to access(%s, F_OK). server does not run?", url);
      break;
    }

    if ((m_socket_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR("socket(PF_UNIX, SOCK_SEQPACKET, 0) failed");
      break;;
    }
    memset(&m_addr, 0, sizeof(sockaddr_un));

    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", url);
    if (connect(m_socket_fd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ERROR("connect failed: %s", strerror(errno));
      break;
    }

    if (write(m_socket_fd, &m_config, sizeof(m_config)) != sizeof(m_config) ) {
      ERROR("Failed to write config to server!");
      break;
    }
    FD_ZERO(&m_all_set);
    FD_ZERO(&m_read_set);
    FD_SET(m_control_fd[0], &m_all_set);
    FD_SET(m_socket_fd, &m_all_set);

    m_max_fd = AM_MAX(m_socket_fd, m_control_fd[0]);
    m_is_connected = true;
    m_event->signal();
    ret = true;
  } while (0);
  return ret;
}

void AMExportClientUDS::disconnect_server()
{
  if (m_is_connected) {
    char write_char = 'q';
    if (write(m_control_fd[1], &write_char, 1) != 1) {
      ERROR("Write socketpair error!");
    } else {
      char read_char;
      if (read(m_control_fd[1], &read_char, 1) != 1) {
        ERROR("Read socketpair error!");
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

bool AMExportClientUDS::receive(AMExportPacket *packet)
{
  bool ret = false;
  do {
    if (!packet) {
      ERROR("ReceivePacket(packet == NULL)");
      break;
    }
    if (!m_is_connected) {
      break;
    }
    m_mutex->lock();
    while (m_packet_queue.empty()) {
      m_block = true;
      m_cond->wait(m_mutex);
      if (!m_is_connected) {
        m_mutex->unlock();
        return false;
      }
    }
    m_block = false;
    *packet = m_packet_queue.front();
    m_packet_queue.pop_front();
    m_mutex->unlock();
    ret = true;
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
      if (read(m_control_fd[0], &read_char, 1) == 1) {
        if ((read_char == 'q') &&
            (write(m_control_fd[0], &read_char, 1) == 1)) {
          INFO("read command: %c, quit", read_char);
          break;
        }
      }
    } else if (FD_ISSET(m_socket_fd, &m_read_set)) {
      int val = read(m_socket_fd, &packet, sizeof(AMExportPacket));
      if (sizeof(AMExportPacket) != (uint32_t) val) {
        NOTICE("Failed to read header, ret %d, server quit", val);
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
        val = read(m_socket_fd, packet.data_ptr, packet.data_size);
        if (AM_UNLIKELY(packet.data_size != (uint32_t )val)) {
          NOTICE("Failed to read data, ret %d, server quit", ret);
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
            release((AMExportPacket*) it._M_node);
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
