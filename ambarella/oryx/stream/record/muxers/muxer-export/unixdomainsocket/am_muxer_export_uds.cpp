/*******************************************************************************
 * am_muxer_export_uds.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-01 - [Shupeng Ren] modified file
 *   2016-07-08 - [Guohua Zheng] modified file
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "am_video_reader_if.h"
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_file.h"
#include "am_thread.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_io.h"

#include "am_amf_types.h"
#include "am_export_if.h"
#include "am_amf_packet.h"
#include "am_muxer_codec_if.h"
#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_muxer_export_uds.h"

AMIMuxerCodec* am_create_muxer_export_uds()
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

void AMMuxerExportUDS::destroy()
{
  delete this;
}

AMMuxerExportUDS::AMMuxerExportUDS()
{}

AMMuxerExportUDS::~AMMuxerExportUDS()
{
  clean_resource();
}

AM_STATE AMMuxerExportUDS::start()
{
  AM_STATE ret_state = AM_STATE_OK;

  m_running = true;

  return ret_state;
}

AM_STATE AMMuxerExportUDS::stop()
{
  NOTICE("AMMuxerExportUDS stop is called!");
  if (m_socket_fd > 0) {
    m_running = false;
  }

  return AM_STATE_OK;
}

const char* AMMuxerExportUDS::name()
{
  return "muxer-export";
}

void* AMMuxerExportUDS::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
      ((void*)nullptr);
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

uint8_t AMMuxerExportUDS::get_muxer_codec_stream_id()
{
  return 0x0F;
}

uint32_t AMMuxerExportUDS::get_muxer_id()
{
  return 18;// There is no config file in export muxer, so set 18 as default.
}

AM_MUXER_CODEC_STATE AMMuxerExportUDS::get_state()
{
  return m_running ? AM_MUXER_CODEC_RUNNING : AM_MUXER_CODEC_STOPPED;
}

void AMMuxerExportUDS::feed_data(AMPacket *packet)
{
  if (!packet) {return;}
  uint32_t stream_id = packet->get_stream_id();
  switch (packet->get_type()) {
    case AMPacket::AM_PAYLOAD_TYPE_INFO: {
      save_info(packet);
    } break;
    case AMPacket::AM_PAYLOAD_TYPE_EOS:
      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        m_video_infos.erase(stream_id);
        m_video_export_packets.erase(stream_id);
        m_video_info_send_flag.erase(stream_id);
      } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
        m_audio_infos.erase(stream_id);
        m_audio_export_packets.erase(stream_id);
        m_audio_info_send_flag.erase(stream_id);
      }
    case AMPacket::AM_PAYLOAD_TYPE_DATA: {
      if (!m_client_connected) {break;}
      if (!m_sort_mode.first) {
        std::unique_lock<std::mutex> lk(m_send_mutex);
        if (m_packet_queue.size() < 32) {
          packet->add_ref();
          m_packet_queue.push(packet);
          if (m_send_block) {m_send_cond.notify_one();}
        } else {
          NOTICE("Too much data in queue, drop data!");
        }
      } else {
        switch(packet->get_attr()) {
          case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
            std::unique_lock<std::mutex> lk(m_send_mutex);
            if ((m_video_queue.find(stream_id) == m_video_queue.end()) ||
                (m_video_queue[stream_id].size() < 60)) {
              packet->add_ref();
              m_video_queue[stream_id].push(packet);
              if (m_video_send_block.second &&
                  (m_video_send_block.first == stream_id)) {
                m_send_cond.notify_one();
              }
            } else {
              ERROR("Too much data in Video[%u] queue, drop data!", stream_id);
            }
          }break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
            std::unique_lock<std::mutex> lk(m_send_mutex);
            if ((m_audio_queue.find(stream_id) == m_audio_queue.end()) ||
                (m_audio_queue[stream_id].size() < 20)) {
              packet->add_ref();
              m_audio_queue[stream_id].push(packet);
              if (m_audio_send_block.second &&
                  (m_audio_send_block.first == stream_id)) {
                m_send_cond.notify_one();
              }
            } else {
              ERROR("Too much data in Audio[%u] queue, drop data!", stream_id);
            }
          }break;
          default: {
            WARN("Current packet with attribute %d is not handled!",
                 packet->get_attr());
          }break;
        }
      }
    }break;
    default: break;
  }
  packet->release();
}

bool AMMuxerExportUDS::init()
{
  bool ret = false;
  m_export_running = true;

  do {
    if (!(m_thread_export_wait = AMEvent::create())) {
      ERROR("Failed to create m_thread_export_wait!");
      break;
    }

    if (pipe(m_control_fd) < 0) {
      ERROR("Failed to create control fd!");
      break;
    }

    if (!(m_export_thread = AMThread::create("export_thread",
                                             thread_export_entry, this))) {
      ERROR("Failed to create export_thread");
      break;
    }
    m_video_reader = AMIVideoReader::get_instance();
    if ((m_epoll_fd = epoll_create1(0)) == -1) {
      PERROR("create epoll error");
      break;
    }

    epoll_event ev = {0};
    ev.data.fd = m_control_fd[0];
    ev.events = EPOLLIN;
    if ((epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_control_fd[0], &ev)) == -1) {
      PERROR("epoll_ctl error");
      break;
    }

    INFO("ExportUDS start!");
    if ((m_socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR("Failed to create socket!\n");
      break;
    }

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

    sockaddr_un addr;
    memset(&addr, 0, sizeof(sockaddr_un));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", DEXPORT_PATH);

    if (bind(m_socket_fd, (sockaddr*)&addr, sizeof(sockaddr_un)) < 0) {
      ERROR("bind(%d) failed!", m_socket_fd);
      break;
    }

    if (listen(m_socket_fd, MAX_CLIENTS_NUM) < 0) {
      ERROR("listen(%d) failed", m_socket_fd);
      break;
    }

    epoll_event tem_ev = {0};
    tem_ev.data.fd = m_socket_fd;
    tem_ev.events = EPOLLIN;
    if ((epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_socket_fd, &tem_ev)) == -1) {
      PERROR("epoll_ctl error");
      break;
    }

    m_running = true;
    m_export_running = true;
    m_client_connected = false;
    m_thread_export_wait->signal();
    ret = true;
  } while (0);

  return ret;
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
      m_audio_pts_increment[stream_id] = audio_info->pkt_pts_increment;
      m_audio_export_infos[stream_id] = export_audio_info;
      export_packet.packet_type = AM_EXPORT_PACKET_TYPE_AUDIO_INFO;
      export_packet.data_size = sizeof(AMExportAudioInfo);
      switch (audio_info->type) {
        case AM_AUDIO_AAC: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_16KHZ;
            }break;
            case 48000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_48KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_OPUS: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_16KHZ;
            }break;
            case 48000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_48KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_G711A: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711ALaw_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711ALaw_16KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_G711U: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711MuLaw_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711MuLaw_16KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_G726_16:
          export_packet.packet_format =
              AM_EXPORT_PACKET_FORMAT_G726_16KBPS;
          break;
        case AM_AUDIO_G726_24:
          export_packet.packet_format =
              AM_EXPORT_PACKET_FORMAT_G726_24KBPS;
          break;
        case AM_AUDIO_G726_32:
          export_packet.packet_format =
              AM_EXPORT_PACKET_FORMAT_G726_32KBPS;
          break;
        case AM_AUDIO_G726_40:
          export_packet.packet_format =
              AM_EXPORT_PACKET_FORMAT_G726_40KBPS;
          break;
        case AM_AUDIO_LPCM: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_16KHZ;
            }break;
            case 48000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_48KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_BPCM: {
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_16KHZ;
            }break;
            case 48000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_48KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_SPEEX:{
          switch(audio_info->sample_rate) {
            case 8000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_8KHZ;
            }break;
            case 16000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_16KHZ;
            }break;
            case 48000: {
              export_packet.packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_48KHZ;
            }break;
            default: {
              export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", audio_info->sample_rate);
            }break;
          }
        } break;
        default:
          export_packet.packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
          break;
      }
      m_audio_export_packets[stream_id] = export_packet;
      m_audio_info_send_flag[stream_id] = false;
      m_audio_map |= 1 << stream_id;
      m_audio_state[stream_id] = EExportAudioState_Normal;
      INFO("Save Audio[%d] INFO: "
          "samplerate: %d, framesize: %d, "
          "bitrate: %d, channels: %d, samplerate: %d",
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
    default: break;
  }
}

void AMMuxerExportUDS::data_export_loop()
{
  int connect_num = 0;
  int export_index = 0;
  AMPacket *hold_packet = nullptr;
  epoll_event ev[MAX_EPOLL_FDS];
  uint32_t check_err = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  while (m_export_running) {

    if ((connect_num = epoll_wait(m_epoll_fd, ev, MAX_EPOLL_FDS, -1)) > 0) {

      if ((export_index = m_connect_num) > 0) {
        hold_packet = get_export_packet();
      }


      for (int i = 0; i < connect_num; i++) {

        if (ev[i].data.fd == m_socket_fd) {

          if ((m_connect_fd = accept(m_socket_fd, nullptr, nullptr)) <= 0) {
            PERROR("accept error");
            continue;
          } else {
            AMExportConfig tem_config;
            if (!read_config(m_connect_fd, &tem_config)) {
              continue;
            }
            if (!m_sort_mode.second) {
              m_sort_mode.first = tem_config.need_sort;
              m_sort_mode.second = true;
              char tem_buffer = 'R';
              if ((am_write(m_connect_fd, &tem_buffer, sizeof(tem_buffer))) !=
                  1) {
                ERROR("write signal run to client failed ");
              }
            } else {
              if (m_sort_mode.first != tem_config.need_sort) {
                ERROR("The sort mode has been set to %u", m_sort_mode.first);
                char tem_buffer = 'Q';
                if ((am_write(m_connect_fd, &tem_buffer, sizeof(tem_buffer))) !=
                    1) {
                  ERROR("write signal quit to client failed ");
                }
              } else {
                char tem_buffer = 'R';
                if ((am_write(m_connect_fd, &tem_buffer, sizeof(tem_buffer))) !=
                    1) {
                  ERROR("write signal run to client failed ");
                }
              }
            }
            m_config_map[m_connect_fd] = tem_config;
            epoll_event ev;
            ev.data.fd = m_connect_fd;
            ev.events = EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
            if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_connect_fd, &ev) == -1) {
              PERROR("epoll_ctl error");
              continue;
            }
            m_client_connected = true;
            m_connect_num++;
            m_client_info_send_flag[m_connect_fd] = false;
            m_export_update_send_flag[m_connect_fd] = true;
            NOTICE("Connect clients number is %d", m_connect_num.load());
          }
          continue;
        } else if (ev[i].data.fd == m_control_fd[0]) {
          m_export_running = false;
        } else if((ev[i].events & check_err)) {

          if (close(ev[i].data.fd) == -1) {
            PERROR("close error");
          }

          m_config_map.erase(ev[i].data.fd);

          if (m_connect_num > 0) {
            m_connect_num--;
          }

          if (m_connect_num == 0){

            m_sort_mode.second = false;

            m_client_connected = false;
            for (auto &m : m_video_queue) {
              while (!m.second.empty()) {
                m.second.front_pop()->release();
              }
            }
            for (auto &m : m_audio_queue) {
              while (!m.second.empty()) {
                m.second.front_pop()->release();
              }
            }
            while (!m_packet_queue.empty()) {
              m_packet_queue.front_pop()->release();
            }

            for (auto &m : m_video_info_send_flag) {
              m.second = false;
            }
            for (auto &m : m_audio_info_send_flag) {
              m.second = false;
            }
          }
          NOTICE("Connect clients number is %d", m_connect_num.load());
          continue;
        } else if ((ev[i].events & EPOLLIN)) {

          AMExportConfig tem_config;
          if (!(read_config(ev[i].data.fd, &tem_config))) {
            continue;
          }
          m_config_map[ev[i].data.fd] = tem_config;

          m_export_update_send_flag[ev[i].data.fd] = false;

          for (auto &m : m_video_info_send_flag) {
            m.second = false;
          }

          for (auto &m : m_audio_info_send_flag) {
            m.second = false;
          }

          continue;
        } else if ((ev[i].events & EPOLLOUT)) {

          if (!m_export_update_send_flag[ev[i].data.fd]) {

            if(!send_info(ev[i].data.fd)) {
              NOTICE("send packet failed");
            }
            m_export_update_send_flag[ev[i].data.fd] = true;
          }

          if (!m_client_info_send_flag[ev[i].data.fd]) {
            for (auto &m : m_video_info_send_flag) {
              m.second = false;
            }

            for (auto &m : m_audio_info_send_flag) {
              m.second = false;
            }

            if (!send_info(ev[i].data.fd)) {
              NOTICE("send info failed");
            }

            m_client_info_send_flag[ev[i].data.fd] = true;

          }
          if(!send_packet(ev[i].data.fd, hold_packet)) {
            NOTICE("send packet failed");
          }

          export_index --;
          if (export_index == 0) {
            if (m_sort_mode.first == 0) {
              if (!m_packet_queue.empty()) {
                m_packet_queue.pop();
              }

            } else {
              if (hold_packet) {
                if (hold_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
                  if (!m_video_queue[hold_packet->get_stream_id()].empty()) {
                    m_video_queue[hold_packet->get_stream_id()].pop();
                  }
                } else if (hold_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
                  if (!m_audio_queue[hold_packet->get_stream_id()].empty()) {
                    m_audio_queue[hold_packet->get_stream_id()].pop();
                  }
                }
              }
            }
          }
        }
      }


      if (export_index != 0) {

        if (m_sort_mode.first == 0) {
          WARN("Has %d clients didn't get packet, m_packet_queue need pop",
               export_index);
          if (!m_packet_queue.empty()) {
            m_packet_queue.pop();
          }
        } else {
          WARN("Has %d clients didn't get packet");
          if (hold_packet) {
            if (hold_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
              if (!m_video_queue[hold_packet->get_stream_id()].empty()) {
                m_video_queue[hold_packet->get_stream_id()].pop();
              }
            } else if (hold_packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
              if (!m_audio_queue[hold_packet->get_stream_id()].empty()) {
                m_audio_queue[hold_packet->get_stream_id()].pop();
              }
            }
          }
        }
      }

      AM_RELEASE(hold_packet);
    } else if (connect_num < 0) {
      PERROR("epoll_wait error");
    }
  }
}

bool AMMuxerExportUDS::fill_export_packet(int client_fd, AMPacket *packet_fill,
                                          AMExportPacket *export_packet)
{
  bool ret = true;
  uint32_t t = 0;
  uint32_t t_sample_rate = 0;
  export_packet->data_ptr = nullptr;
  export_packet->data_size = packet_fill->get_data_size();
  export_packet->pts = packet_fill->get_pts();
  export_packet->stream_index = packet_fill->get_stream_id();
  export_packet->is_key_frame = 0;
  export_packet->is_direct_mode = 1;
  switch (packet_fill->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
      if (!(m_config_map[client_fd].video_map &
          (1 << (packet_fill->get_stream_id() )))) {
        ret = false;
        break;
      }
    case AMPacket::AM_PAYLOAD_ATTR_SEI: {
      export_packet->packet_type = AM_EXPORT_PACKET_TYPE_VIDEO_DATA;
      t = packet_fill->get_frame_type();
      switch (t) {
        case AM_VIDEO_FRAME_TYPE_IDR: {
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_IDR;
          switch(AM_VIDEO_TYPE(packet_fill->get_video_type())) {
            case AM_VIDEO_H264:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
              break;
            case AM_VIDEO_H265:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_HEVC;
              break;
            default:break;
          }
          export_packet->is_key_frame = 1;
        } break;
        case AM_VIDEO_FRAME_TYPE_I: {
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_I;
          switch(AM_VIDEO_TYPE(packet_fill->get_video_type())) {
            case AM_VIDEO_H264:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
              break;
            case AM_VIDEO_H265:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_HEVC;
              break;
            default:break;
          }
        } break;
        case AM_VIDEO_FRAME_TYPE_P: {
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_P;
          switch(AM_VIDEO_TYPE(packet_fill->get_video_type())) {
            case AM_VIDEO_H264:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
              break;
            case AM_VIDEO_H265:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_HEVC;
              break;
            default:break;
          }
        } break;
        case AM_VIDEO_FRAME_TYPE_B: {
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_B;
          switch(AM_VIDEO_TYPE(packet_fill->get_video_type())) {
            case AM_VIDEO_H264:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_AVC;
              break;
            case AM_VIDEO_H265:
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_HEVC;
              break;
            default:break;
          }
        } break;
        case AM_VIDEO_FRAME_TYPE_MJPEG:
        case AM_VIDEO_FRAME_TYPE_SJPEG:
          export_packet->frame_type = AM_EXPORT_VIDEO_FRAME_TYPE_I;
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_MJPEG;
          export_packet->is_key_frame = 1;
          break;
        default:
          ERROR("Not supported video frame type %d\n", t);
          ret = false;
          break;
      }
      export_packet->data_ptr = (uint8_t*)packet_fill->get_addr_offset();
      export_packet->is_direct_mode = 1;
    } break;

    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      export_packet->packet_type = AM_EXPORT_PACKET_TYPE_AUDIO_DATA;
      t = packet_fill->get_frame_type();
      t_sample_rate = packet_fill->get_frame_attr();
      switch (t) {
        case AM_AUDIO_G711A: {
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711ALaw_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711ALaw_16KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_G711U:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711MuLaw_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_G711MuLaw_16KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_G726_40:
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_40KBPS;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          break;
        case AM_AUDIO_G726_32:
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_32KBPS;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          break;
        case AM_AUDIO_G726_24:
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_24KBPS;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          break;
        case AM_AUDIO_G726_16:
          export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_G726_16KBPS;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          break;
        case AM_AUDIO_AAC:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_16KHZ;
            }break;
            case 48000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_AAC_48KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_OPUS:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_16KHZ;
            }break;
            case 48000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_OPUS_48KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_LPCM:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_16KHZ;
            }break;
            case 48000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_PCM_48KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_BPCM:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_16KHZ;
            }break;
            case 48000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_BPCM_48KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        case AM_AUDIO_SPEEX:{
          export_packet->is_key_frame = 1;
          export_packet->audio_sample_rate = t_sample_rate/1000;
          switch(t_sample_rate) {
            case 8000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_8KHZ;
            }break;
            case 16000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_16KHZ;
            }break;
            case 48000: {
              export_packet->packet_format =
                  AM_EXPORT_PACKET_FORMAT_SPEEX_48KHZ;
            }break;
            default: {
              export_packet->packet_format = AM_EXPORT_PACKET_FORMAT_INVALID;
              ERROR("Unsupported samplerate: %u", t_sample_rate);
            }break;
          }
        } break;
        break;
        default:
          ERROR("Not supported audio frame type %d\n", t);
          ret = false;
          break;
      }
      uint8_t tem_check = export_packet->packet_format;
      if (!(m_config_map[client_fd].audio_map &
          (1LL << tem_check))) {
        ret = false;
        break;
      }

      export_packet->is_direct_mode = 0;
      export_packet->data_ptr = packet_fill->get_data_ptr();
    } break;
    default:
      ERROR("Not supported payload attr %d\n", packet_fill->get_attr());
      ret = false;
      break;
  }

  return ret;
}

bool AMMuxerExportUDS::send_info(int client_fd)
{
  bool ret = true;

  do {
    for (auto &m : m_video_info_send_flag) {
      if (m.second) {
        continue;
      }

      if ((m_config_map[client_fd].video_map) &
          (1 << (m_video_export_packets[m.first].stream_index))) {

        if (am_write(client_fd, &m_video_export_packets[m.first],
                  sizeof(AMExportPacket)) != sizeof(AMExportPacket)) {
          ret = false;
          break;
        }

        /* update stream info because stream service may not update it
         * when user dynamic reset some stream info parameters*/
        AMStreamInfo stream_info;
        stream_info.stream_id =
            AM_STREAM_ID(m_video_export_packets[m.first].stream_index);
        if (AM_LIKELY(AM_RESULT_OK
            == m_video_reader->query_stream_info(stream_info))) {
        }
        m_video_export_infos[m.first].framerate_num = stream_info.mul;
        m_video_export_infos[m.first].framerate_den = stream_info.div;
        if (am_write(client_fd,
                  &m_video_export_infos[m.first],
                  sizeof(AMExportVideoInfo)) != sizeof(AMExportVideoInfo)) {
          ERROR("Write video info[%d] failed, close socket!", m.first);
          ret = false;
          break;
        }
        m.second = true;
      }
      if (!ret) {break;}
    }

    for (auto &m: m_audio_info_send_flag) {
      if (m.second) {
        continue;
      }

      if (m_config_map[client_fd].audio_map &
          (1LL << m_audio_export_packets[m.first].packet_format)) {

        if (am_write(client_fd, &m_audio_export_packets[m.first],
                  sizeof(AMExportPacket)) != sizeof(AMExportPacket)) {
          ERROR("Write header failed, close socket!");
          ret = false;
          break;
        }
        if (am_write(client_fd, &m_audio_export_infos[m.first],
                  sizeof(AMExportAudioInfo)) != sizeof(AMExportAudioInfo)) {
          ERROR("Write audio info[%d] failed, close socket!", m.first);
          ret = false;
          break;
        }
        m.second = true;
      }
    }
    if (!ret) {break;}
  } while (0);

  return ret;
}

bool AMMuxerExportUDS::read_config(int fd, AMExportConfig *m_config)
{
  bool result = true;

  do {
    int read_ret = 0;
    int read_cnt = 0;
    uint32_t received = 0;

    do {
      read_ret = read(fd, m_config + received,
                      sizeof(*m_config) - received);
      if (AM_LIKELY(read_ret > 0)) {
        received += read_ret;
      }
    }while((++ read_cnt < 5) &&
        (((read_ret >= 0) &&
            (read_ret < (int)sizeof(*m_config))) || ((read_ret < 0) &&
                ((errno == EINTR) ||
                    (errno == EWOULDBLOCK ||
                        (errno == EAGAIN))))));
    if (received < sizeof(*m_config)) {
      if (read_ret <= 0) {
        PERROR("Read error");
        result = false;
        break;
      }
      if (read_cnt >= 5) {
        ERROR("Read to much times");
        result = false;
        break;
      }
    }

  } while (0);
  return result;
}

AMPacket* AMMuxerExportUDS::get_export_packet()
{
  AM_PTS         min_pts       = INT64_MAX - 1;
  bool empty_check = false;
  AMPacket *packet = nullptr;


  do {
    if (m_sort_mode.first) {
      std::unique_lock<std::mutex> lk(m_send_mutex);
      for (auto &m : m_audio_queue) {
        if (!(m_audio_map & (1 << m.first))) {continue;}

        while (m_running && m.second.empty()) {

          if (min_pts > m_predict_pts) {
            m_audio_send_block.first = m.first;
            m_audio_send_block.second = true;
            m_send_cond.wait(lk);
            m_audio_send_block.second = false;
          } else {

            m_predict_pts = m_audio_last_pts[m.first]
                                             + m_audio_pts_increment[m.first];
            empty_check = true;
            break;
          }
        }
        if (empty_check) {
          continue;
        }

        if (!m_running) {break;}
        if (m.second.front()->get_pts() < min_pts) {
          packet = m.second.front();
          min_pts = packet->get_pts();
        }
      }
      lk.unlock();

      lk.lock();
      for (auto &m : m_video_queue) {
        if (!(m_video_map & (1 << m.first))) {
          continue;
        }

        while (m_running && m.second.empty()) {
          m_video_send_block.first = m.first;
          m_video_send_block.second = true;
          m_send_cond.wait(lk);
          m_video_send_block.second = false;
        }

        if (!m_running) {break;}
        if (m.second.front()->get_pts() < min_pts) {
          packet = m.second.front();
          min_pts = packet->get_pts();
        }
      }
    } else {
      std::unique_lock<std::mutex> lk(m_send_mutex);
      while (m_running && m_packet_queue.empty()) {
        m_send_block = true;
        m_send_cond.wait(lk);
      }
      m_send_block = false;
      if (m_running && !m_packet_queue.empty()) {
        packet = m_packet_queue.front();

      }
    }

    if (!packet) {break;}


    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
      uint32_t stream_id = packet->get_stream_id();
      switch (packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO:
          m_video_map &= ~(1 << stream_id);
          if (m_sort_mode.first) {
            for (auto &m : m_video_queue) {
              while (!m.second.empty()) {
                m.second.front_pop()->release();
              }
              m_video_queue.erase(m.first);
            }
          }
          break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO:
          m_audio_map &= ~(1 << stream_id);

          if (m_sort_mode.first) {
            for (auto &m : m_audio_queue) {
              while (!m.second.empty()) {
                m.second.front_pop()->release();
              }
              m_audio_queue.erase(m.first);
            }
          }
          break;
        default:
          break;
      }
      packet = nullptr;
    }

  } while (0);

  return packet;
}

bool AMMuxerExportUDS::send_packet(int client_fd, AMPacket *packet_send)
{
  bool ret = true;
  AMExportPacket export_packet = {0};
  do {
    if (!packet_send) {
      ret = false;
      break;
    }

    if (fill_export_packet(client_fd, packet_send, &export_packet)) {
      if (am_write(client_fd, &export_packet, sizeof(AMExportPacket)) !=
          sizeof(AMExportPacket)) {
        ret = false;
        NOTICE("Write header failed, close socket!");
        break;
      }

      if (!export_packet.is_direct_mode) {
        if ((uint32_t)am_write(client_fd,
                            export_packet.data_ptr,
                            export_packet.data_size) !=
                                export_packet.data_size) {
          ret = false;
          NOTICE("Write data(%d) failed, close socket!",
                 export_packet.data_size);
        }
      }
    }
  } while (0);

  return ret;
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
  m_export_running = false;
  m_thread_exit = true;
  m_thread_export_exit = true;
  m_thread_export_wait->signal();
  AM_DESTROY(m_export_thread);
  AM_DESTROY(m_thread_export_wait);
  m_video_reader = nullptr;
}

void AMMuxerExportUDS::reset_resource()
{
  if (m_control_fd[1] > 0) {
    am_write(m_control_fd[1], "q", 1);
  }
  if (m_socket_fd > 0) {
    close(m_socket_fd);
    m_socket_fd = -1;
    unlink(DEXPORT_PATH);
  }

  if (m_epoll_fd > 0) {
    close(m_epoll_fd);
    m_epoll_fd = -1;
  }

  m_config_map.clear();
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
    m_packet_queue.front_pop()->release();
  }
  for (auto &m : m_video_queue) {
    while (!m.second.empty()) {
      m.second.front_pop()->release();
    }
  }
  m_video_queue.clear();
  for (auto &m : m_audio_queue) {
    while (!m.second.empty()) {
      m.second.front_pop()->release();
    }
  }
  m_audio_queue.clear();
  m_video_map = 0;
  m_audio_map = 0;
  m_send_block = false;
  m_video_send_block.second = false;
  m_audio_send_block.second = false;
  m_send_cond.notify_all();
}
