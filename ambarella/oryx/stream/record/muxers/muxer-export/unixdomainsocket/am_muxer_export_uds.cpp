/*******************************************************************************
 * am_muxer_export_uds.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-01 - [Shupeng Ren] modified file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <map>
#include <queue>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_thread.h"
#include "am_event.h"
#include "am_mutex.h"

#include "am_amf_types.h"
#include "am_export_if.h"
#include "am_amf_packet.h"
#include "am_muxer_codec_if.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_muxer_export_uds.h"
#include "am_file.h"

enum
{
  EExportState_not_inited           = 0x0,
  EExportState_no_client_connected  = 0x1,
  EExportState_running              = 0x2,
  EExportState_error                = 0x3,
  EExportState_halt                 = 0x4,
};

AMIMuxerCodec *am_create_muxer_export_uds()
{
  return AMMuxerExportUDS::create();
}

AMMuxerExportUDS *AMMuxerExportUDS::create()
{
  AMMuxerExportUDS *result = new AMMuxerExportUDS();

  if (result && !result->init()) {
    delete result;
    result = nullptr;
  }

  return result;
}

bool AMMuxerExportUDS::init()
{
  bool ret = false;

  do {
    if (!(m_send_mutex = AMMutex::create())) {
      ERROR("Failed to create m_send_mutex!");
      break;
    }
    if (!(m_send_cond = AMCondition::create())) {
      ERROR("Failed to create m_send_cond!");
      break;
    }

    if (!(m_thread_wait = AMEvent::create())) {
      ERROR("Failed to create m_thread_wait!");
      break;
    }

    if (pipe(m_control_fd) < 0) {
      ERROR("Failed to create control fd!");
      break;
    }

    if (!(m_accept_thread = AMThread::create("export_thread",
                                             thread_entry,
                                             this))) {
      ERROR("Failed to create accept_thread!");
      break;
    }
    ret = true;
  } while (0);

  return ret;
}

AMMuxerExportUDS::AMMuxerExportUDS() :
    m_export_state(EExportState_not_inited),
    m_socket_fd(-1),
    m_connect_fd(-1),
    m_max_fd(-1),
    m_running(false),
    m_thread_exit(false),
    m_client_connected(false),
    m_video_map(0),
    m_audio_map(0),
    m_thread_wait(nullptr),
    m_send_mutex(nullptr),
    m_accept_thread(nullptr),
    m_send_cond(nullptr),
    m_send_block(false),
    m_video_send_block(0, false),
    m_audio_send_block(0, false),
    m_audio_pts_increment(0)
{
  m_control_fd[0] = m_control_fd[1] = -1;
  m_config.need_sort = 0;
}

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
  m_thread_wait->signal();
  AM_DESTROY(m_accept_thread);
  AM_DESTROY(m_thread_wait);
  AM_DESTROY(m_send_mutex);
  AM_DESTROY(m_send_cond);
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

  m_audio_last_pts.clear();
  m_audio_state.clear();
  m_video_infos.clear();
  m_audio_infos.clear();
  m_video_export_infos.clear();
  m_audio_export_infos.clear();
  m_video_export_packets.clear();
  m_audio_export_packets.clear();
  m_video_info_send_flag.clear();
  m_audio_info_send_flag.clear();
  while (!m_packet_queue.empty()) {
    m_packet_queue.pop();
  }
  m_video_queue.clear();
  m_audio_queue.clear();
  m_video_map = 0;
  m_audio_map = 0;
  m_send_block = false;
  m_video_send_block.second = false;
  m_audio_send_block.second = false;
  m_send_cond->signal();
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
      break;
    }

    FD_SET(m_socket_fd, &m_all_set);
    m_max_fd = AM_MAX(m_socket_fd, m_control_fd[0]);

    if (AMFile::exists(DEXPORT_PATH)) {
      NOTICE("%s already exists! Remove it first!", DEXPORT_PATH);
      if (unlink(DEXPORT_PATH) < 0) {
        PERROR("unlink");
        break;
      }
    }

    if (!AMFile::create_path(AMFile::dirname(DEXPORT_PATH).c_str())) {
      ERROR("Failed to create path %s", AMFile::dirname(DEXPORT_PATH).c_str());
      break;
    }

    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", DEXPORT_PATH);

    if (bind(m_socket_fd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ERROR ("bind(%d) failed, ret %d", m_socket_fd);
      break;
    }

    if (listen(m_socket_fd, 5) < 0) {
      ERROR ("listen(%d) failed", m_socket_fd);
      break;
    }

    m_running = true;
    m_client_connected = false;
    m_thread_wait->signal();
    ret_state = AM_STATE_OK;
  } while(0);
  return ret_state;
}

AM_STATE AMMuxerExportUDS::stop()
{
  NOTICE("AMMuxerExportUDS stop is called!");
  if (m_socket_fd > 0) {
    m_running = false;
    if (m_control_fd[1] > 0) {
      write(m_control_fd[1], "q", 1);
    }
    shutdown(m_socket_fd, SHUT_RD);
    reset_resource();
  }

  return AM_STATE_OK;
}

bool AMMuxerExportUDS::start_file_writing()
{
  WARN("Should not call start file writing function in export muxer.");
  return true;
}

bool AMMuxerExportUDS::stop_file_writing()
{
  WARN("Should not call stop file writing function in export muxer.");
  return true;
}

bool AMMuxerExportUDS::is_running()
{
  return m_running;
}

AM_STATE AMMuxerExportUDS::set_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_OK;
}

AM_STATE AMMuxerExportUDS::get_config(AMMuxerCodecConfig *config)
{
  config->type = AM_MUXER_CODEC_EXPORT;
  return AM_STATE_OK;
}

AM_MUXER_ATTR AMMuxerExportUDS::get_muxer_attr()
{
  return AM_MUXER_EXPORT_NORMAL;
}

AM_MUXER_CODEC_STATE AMMuxerExportUDS::get_state()
{
  if (m_running) {
    return AM_MUXER_CODEC_RUNNING;
  }

  return AM_MUXER_CODEC_STOPPED;
}

void AMMuxerExportUDS::feed_data(AMPacket* packet)
{
  uint32_t stream_id = packet->get_stream_id();
  switch (packet->get_type()) {
    case AMPacket::AM_PAYLOAD_TYPE_INFO: {
      save_info(packet);
    } break;
    case AMPacket::AM_PAYLOAD_TYPE_DATA:
    case AMPacket::AM_PAYLOAD_TYPE_EOS: {
      if (!m_client_connected) {
        break;
      }

      if (!m_config.need_sort) {
        if (m_packet_queue.size() < 32) {
          packet->add_ref();
          m_packet_queue.push(packet);
          m_send_mutex->lock();
          if (m_send_block) {
            m_send_cond->signal();
          }
          m_send_mutex->unlock();
        }
        break;
      }

      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        if ((m_video_queue.find(stream_id) != m_video_queue.end()) &&
            (m_video_queue[stream_id].size() > 32)) {
          //TODO: if queue is full, do something...
          ERROR("Drop Video[%d] data", stream_id);
          break;
        }
        packet->add_ref();
        m_video_queue[stream_id].push(packet);
        m_send_mutex->lock();
        if (m_video_send_block.second &&
            m_video_send_block.first == stream_id) {
          m_send_cond->signal();
        }
        m_send_mutex->unlock();
      } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
        if ((m_audio_queue.find(stream_id) != m_audio_queue.end()) &&
            (m_audio_queue[stream_id].size() > 32)) {
          //TODO: if queue is full, do something...
          ERROR("Drop Audio[%d] data", stream_id);
          break;
        }
        packet->add_ref();
        m_audio_queue[stream_id].push(packet);
        m_send_mutex->lock();
        if (m_audio_send_block.second &&
            m_audio_send_block.first == stream_id) {
          m_send_cond->signal();
        }
        m_send_mutex->unlock();
      }
    } break;
    default:
      break;
  }
  packet->release();
}

void AMMuxerExportUDS::thread_entry(void *arg)
{
  if (!arg) {
    ERROR("Thread argument is null!");
    return;
  }
  AMMuxerExportUDS *p = (AMMuxerExportUDS*)arg;
  do {
    p->m_thread_wait->wait();
    p->main_loop();
  } while (!p->m_thread_exit);
}

void AMMuxerExportUDS::main_loop()
{
  int ret = 0;
  m_export_state = EExportState_no_client_connected;

  while (m_running) {
    switch (m_export_state) {
      case EExportState_no_client_connected:
        m_read_set = m_all_set;
        if ((ret = select(m_max_fd + 1, &m_read_set,
                          nullptr, nullptr, nullptr)) < 0) {
          m_export_state = EExportState_error;
          FD_CLR(m_socket_fd, &m_all_set);
          break;
        } else if (ret == 0) {
          break;
        }

        if (FD_ISSET(m_control_fd[0], &m_read_set)) {
          char read_char;
          read(m_control_fd[0], &read_char, 1);
        }

        if (FD_ISSET(m_socket_fd, &m_read_set)) {
          socklen_t addr_len;
          if (m_connect_fd > 0) {
            close(m_connect_fd);
            m_connect_fd = -1;
          }
          if ((m_connect_fd = accept(m_socket_fd,
                                     (sockaddr*)&m_addr,
                                     &addr_len)) < 0) {
            NOTICE("muxer-export will exit!");
            m_export_state = EExportState_halt;
            break;
          }
          NOTICE("client connected, fd %d\n", m_connect_fd);
          if (read(m_connect_fd, &m_config, sizeof(m_config)) !=
              sizeof(m_config)) {
            ERROR("Failed to read config from client!");
            break;
          }
          m_client_connected = true;
          m_export_state = EExportState_running;
        }
        break;

      case EExportState_running:
        if (!send_info(m_connect_fd) || !send_packet(m_connect_fd)) {
          NOTICE("send failed, client exit!");
          m_client_connected = false;
          m_export_state = EExportState_no_client_connected;

          for (auto &m : m_video_queue) {
            while (!m.second.empty()) {
              m.second.front()->release();
              m.second.pop();
            }
          }

          for (auto &m : m_audio_queue) {
            while (!m.second.empty()) {
              m.second.front()->release();
              m.second.pop();
            }
          }

          while (!m_packet_queue.empty()) {
            m_packet_queue.front()->release();
            m_packet_queue.pop();
          }

          for (auto &m : m_video_info_send_flag) {
            m.second = false;
          }
          for (auto &m : m_audio_info_send_flag) {
            m.second = false;
          }
        } break;

      case EExportState_error:
      case EExportState_halt:
        break;
      default:
        break;
    }
  }
  INFO("Export.Main exits mainloop!");
}

void AMMuxerExportUDS::save_info(AMPacket *packet)
{
  uint32_t stream_id = packet->get_stream_id();
  AMExportPacket export_packet = {0};
  export_packet.stream_index = stream_id;
  export_packet.is_direct_mode = 0;

  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO*)packet->get_data_ptr();
      m_audio_infos[stream_id] = *audio_info;
      AMExportAudioInfo export_audio_info;
      export_audio_info.samplerate = audio_info->sample_rate;
      export_audio_info.channels = audio_info->channels;
      export_audio_info.sample_size = audio_info->sample_size;
      export_audio_info.pts_increment = audio_info->pkt_pts_increment;
      m_audio_pts_increment = audio_info->pkt_pts_increment;
      m_audio_export_infos[stream_id] = export_audio_info;
      export_packet.packet_type = AM_EXPORT_PACKET_TYPE_AUDIO_INFO;
      export_packet.data_size = sizeof(AMExportAudioInfo);
      switch (audio_info->type) {
        case AM_AUDIO_AAC:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_AAC;
          break;
        case AM_AUDIO_OPUS:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_OPUS;
          break;
        case AM_AUDIO_G711A:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G711ALaw;
          break;
        case AM_AUDIO_G711U:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G711MuLaw;
          break;
        case AM_AUDIO_G726_16:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G726_16;
          break;
        case AM_AUDIO_G726_24:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G726_24;
          break;
        case AM_AUDIO_G726_32:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G726_32;
          break;
        case AM_AUDIO_G726_40:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_G726_40;
          break;
        case AM_AUDIO_LPCM:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_PCM;
          break;
        case AM_AUDIO_BPCM:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_BPCM;
          break;
        case AM_AUDIO_SPEEX:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_SPEEX;
          break;
        default:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
          break;
      }
      m_audio_export_packets[stream_id] = export_packet;
      m_audio_info_send_flag[stream_id] = false;
      m_audio_map |= 1 << stream_id;
      m_audio_state[stream_id] = 0;
      INFO("Save Audio[%d] INFO: "
          "samplerate: %d, framesize: %d, "
          "bitrate: %d, channels: %d, samplerate: d",
          stream_id,
          m_audio_export_infos[stream_id].samplerate,
          m_audio_export_infos[stream_id].frame_size,
          m_audio_export_infos[stream_id].bitrate,
          m_audio_export_infos[stream_id].channels,
          m_audio_export_infos[stream_id].samplerate);
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      AM_VIDEO_INFO *video_info = (AM_VIDEO_INFO*)packet->get_data_ptr();
      m_video_infos[stream_id] = *video_info;
      AMExportVideoInfo export_video_info;
      export_video_info.width = video_info->width;
      export_video_info.height = video_info->height;
      export_video_info.framerate_num = video_info->mul;
      export_video_info.framerate_den = video_info->div;
      m_video_export_infos[stream_id] = export_video_info;
      export_packet.packet_type = AM_EXPORT_PACKET_TYPE_VIDEO_INFO;
      export_packet.data_size = sizeof(AMExportVideoInfo);
      switch (video_info->type) {
        case AM_VIDEO_H264:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
          break;
        case AM_VIDEO_H265:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_HEVC;
          break;
        case AM_VIDEO_MJPEG:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_MJPEG;
          break;
        default:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
          break;
      }
      m_video_export_packets[stream_id] = export_packet;
      m_video_info_send_flag[stream_id] = false;
      m_video_map |= 1 << stream_id;
      INFO("Save Video[%d] INFO: "
          "width: %d, height: %d, frame factor %d/%d",
          stream_id,
          m_video_export_infos[stream_id].width,
          m_video_export_infos[stream_id].height,
          m_video_export_infos[stream_id].framerate_num,
          m_video_export_infos[stream_id].framerate_den);
    } break;
    default:
      break;
  }
}

bool AMMuxerExportUDS::send_info(int client_fd)
{
  bool ret = true;

  do {
    for (auto &m : m_video_info_send_flag) {
      if (m.second) {
        continue;
      }

      if (write(client_fd, &m_video_export_packets[m.first],
                sizeof(AMExportPacket)) != sizeof(AMExportPacket)) {
        ERROR("write header failed, close socket!");
        ret = false;
        break;
      }
      if (write(client_fd, &m_video_export_infos[m.first],
                sizeof(AMExportVideoInfo)) != sizeof(AMExportVideoInfo)) {
        ERROR("write video info[%d] failed, close socket!", m.first);
        ret = false;
        break;
      }
      m.second = true;
    }
    if (!ret) {
      break;
    }

    for (auto &m: m_audio_info_send_flag) {
      if (m.second) {
        continue;
      }

      if (write(client_fd, &m_audio_export_packets[m.first],
                sizeof(AMExportPacket)) != sizeof(AMExportPacket)) {
        NOTICE("write header failed, close socket!");
        ret = false;
        break;
      }
      if (write(client_fd, &m_audio_export_infos[m.first],
                sizeof(AMExportAudioInfo)) != sizeof(AMExportAudioInfo)) {
        NOTICE("write audio info[%d] failed, close socket!", m.first);
        ret = false;
        break;
      }
      m.second = true;
    }
    if (!ret) {
      break;
    }
  } while (0);

  return ret;
}

bool AMMuxerExportUDS::send_packet(int client_fd)
{
  bool                ret             = true;
  AM_PTS              min_pts         = INT64_MAX;
  AMPacket           *am_packet       = nullptr;
  AMExportPacket      export_packet;

  do {
    if (m_config.need_sort) {
      for (auto &m : m_audio_queue) {
        if (!(m_audio_map & (1 << m.first))) {
          continue;
        }
        m_send_mutex->lock();
        if ((m.second.empty()) || !m.second.front()) {
          switch (m_audio_state[m.first]) {
            case 0: //Normal
              m_audio_state[m.first] = 1;
              m_audio_last_pts[m.first] += m_audio_pts_increment - 100;
              break;
            case 1: //Try to get audio
              break;
            case 2: //Need block to wait audio packet
              m_audio_send_block.first = m.first;
              m_audio_send_block.second = true;
              m_send_cond->wait(m_send_mutex);
              m_audio_send_block.second = false;
              m_audio_last_pts[m.first] = m.second.front()->get_pts();
              m_audio_state[m.first] = 0;
              break;
            default:
              break;
          }
        } else {
          m_audio_last_pts[m.first] = m.second.front()->get_pts();
          if (m_audio_state[m.first]) {
            m_audio_state[m.first] = 0;
          }
        }
        m_send_mutex->unlock();
        if (!m_running) {
          break;
        }
        if (m_audio_last_pts[m.first] < min_pts) {
          min_pts = m_audio_last_pts[m.first];
          am_packet = m.second.front();
        }
      }

      for (auto &m : m_video_queue) {
        if (!(m_video_map & (1 << m.first))) {
          continue;
        }
        m_send_mutex->lock();
        if ((m.second.empty()) || !m.second.front()) {
          m_video_send_block.first = m.first;
          m_video_send_block.second = true;
          m_send_cond->wait(m_send_mutex);
          m_video_send_block.second = false;
        }
        m_send_mutex->unlock();
        if (!m_running) {
          break;
        }
        if (m.second.front()->get_pts() < min_pts) {
          min_pts = m.second.front()->get_pts();
          am_packet = m.second.front();
        }
      }

      for (auto &m : m_audio_queue) {
        if ((am_packet == m.second.front()) && (m_audio_state[m.first])) {
          m_audio_state[m.first] = 2; //Need block to wait
          return ret;
        }
      }

      if (am_packet &&
          (am_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)) {
        m_video_queue[am_packet->get_stream_id()].pop();
      } else if (am_packet &&
          (am_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO)) {
        m_audio_queue[am_packet->get_stream_id()].pop();
      }
    } else {
      m_send_mutex->lock();
      if (m_packet_queue.empty() || !m_packet_queue.front()) {
        m_send_block = true;
        m_send_cond->wait(m_send_mutex);
      }
      m_send_block = false;
      m_send_mutex->unlock();
      if (!m_running) {
        break;
      }
      am_packet = m_packet_queue.front();
      m_packet_queue.pop();
    }

    if (!am_packet) {
      //usleep(1000);
      break;
    }

    if (am_packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
      uint32_t stream_id = am_packet->get_stream_id();
      switch (am_packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
          m_video_map &= ~(1 << stream_id);
          m_video_infos.erase(stream_id);
          m_video_export_packets.erase(stream_id);
          m_video_info_send_flag.erase(stream_id);
          if (m_config.need_sort) {
            for (auto &m : m_video_queue) {
              while (!m.second.empty()) {
                m.second.front()->release();
                m.second.pop();
              }
              m_video_queue.erase(m.first);
            }
          }
          break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
          m_audio_map &= ~(1 << stream_id);
          m_audio_infos.erase(stream_id);
          m_audio_export_packets.erase(stream_id);
          m_audio_info_send_flag.erase(stream_id);
          if (m_config.need_sort) {
            for (auto &m : m_audio_queue) {
              while (!m.second.empty()) {
                m.second.front()->release();
                m.second.pop();
              }
              m_audio_queue.erase(m.first);
            }
          }
          break;
        default:
          break;
      }
    }

    fill_export_packet(am_packet, &export_packet);
    if (write(client_fd, &export_packet, sizeof(AMExportPacket)) !=
        sizeof(AMExportPacket)) {
      ret = false;
      NOTICE("write header failed, close socket!");
    }

    if (!export_packet.is_direct_mode) {
      if ((uint32_t)write(client_fd, export_packet.data_ptr,
                          export_packet.data_size) != export_packet.data_size) {
        ret = false;
        NOTICE("write data(%d) failed, close socket!",
               export_packet.data_size);
      }
    }
    am_packet->release();
  } while (0);
  return ret;
}

void AMMuxerExportUDS::fill_export_packet(AMPacket* packet,
                                          AMExportPacket* export_packet)
{
  uint32_t t = 0;
  export_packet->data_ptr = nullptr;
  export_packet->data_size = packet->get_data_size();
  export_packet->pts = packet->get_pts();
  export_packet->stream_index = packet->get_stream_id();
  export_packet->is_key_frame = 0;
  export_packet->is_direct_mode = 1;

  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
    case AMPacket::AM_PAYLOAD_ATTR_SEI: {
      export_packet->packet_type = AM_EXPORT_PACKET_TYPE_VIDEO_DATA;
      t = packet->get_frame_type();
      switch (t) {
        case AM_VIDEO_FRAME_TYPE_H264_IDR:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_IDR;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
          export_packet->is_key_frame = 1;
          break;
        case AM_VIDEO_FRAME_TYPE_H264_I:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_I;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
          break;
        case AM_VIDEO_FRAME_TYPE_H264_P:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_P;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
          break;
        case AM_VIDEO_FRAME_TYPE_H264_B:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_B;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
          break;
        case AM_VIDEO_FRAME_TYPE_MJPEG:
        case AM_VIDEO_FRAME_TYPE_SJPEG:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_I;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_MJPEG;
          export_packet->is_key_frame = 1;
          break;
        default:
          ERROR("not supported video frame type %d\n", t);
          break;
      }
      export_packet->data_ptr = (uint8_t*)packet->get_addr_offset();
      export_packet->is_direct_mode = 1;
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      export_packet->packet_type = AM_EXPORT_PACKET_TYPE_AUDIO_DATA;
      t = packet->get_frame_type();
      switch (t) {
        case AM_AUDIO_G711A:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G711ALaw;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G711ALaw;
          export_packet->is_key_frame = 1;
          break;
        case AM_AUDIO_G711U:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G711MuLaw;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G711MuLaw;
          break;
        case AM_AUDIO_G726_40:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G726_40;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_40;
          break;
        case AM_AUDIO_G726_32:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G726_32;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_32;
          break;
        case AM_AUDIO_G726_24:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G726_24;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_24;
          break;
        case AM_AUDIO_G726_16:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_G726_16;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_16;
          break;
        case AM_AUDIO_AAC:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_AAC;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AAC;
          break;
        case AM_AUDIO_OPUS:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_OPUS;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_OPUS;
          break;
        case AM_AUDIO_LPCM:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_PCM;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_PCM;
          break;
        case AM_AUDIO_BPCM:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_BPCM;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_BPCM;
          break;
        case AM_AUDIO_SPEEX:
          export_packet->frame_type = AM_EXPORT_PACKET_FORMAT_SPEEX;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_SPEEX;
          break;
        default:
          ERROR("not supported audio frame type %d\n", t);
          break;
      }
      export_packet->is_direct_mode = 0;
      export_packet->data_ptr = (uint8_t*)packet->get_data_ptr();
    } break;

    default:
      ERROR("not supported payload attr %d\n", t);
      break;
  }
}
