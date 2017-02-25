/*******************************************************************************
 * am_muxer_jpeg_event.cpp
 *
 * History:
 *   2016-1-27 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_log.h"
#include "am_define.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_file_sink_if.h"
#include "am_muxer_codec_info.h"
#include "am_file.h"
#include <time.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include "am_muxer_jpeg_file_writer.h"
#include "am_muxer_jpeg_event.h"
#include "am_amf_packet_pool.h"

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerJpegEvent::create(config_file));
}

AMMuxerJpegEvent* AMMuxerJpegEvent::create(const char* config_name)
{
  AMMuxerJpegEvent* result = new AMMuxerJpegEvent();
  if (result && (result->init(config_name) != AM_STATE_OK)) {
    ERROR("Failed to create AMMuxerJpegEvent.");
    delete result;
    result = nullptr;
  }
  return result;
}

AMMuxerJpegEvent::AMMuxerJpegEvent()
{
  m_last_time_stamp.clear();
}

AMMuxerJpegEvent::~AMMuxerJpegEvent()
{
  m_packet_pool->enable(false);
  AM_DESTROY(m_packet_pool);
  INFO("%s has been destroyed.", m_muxer_name.c_str());
}

AM_STATE AMMuxerJpegEvent::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY((ret = inherited::init(config_file)) != AM_STATE_OK)) {
      ERROR("Failed to init AMMuxerJpegEvent object");
      break;
    }
    std::string pool_name = "JpegEventPacketPool";
    uint8_t buffer_size = m_muxer_jpeg_config->buffer_size;
    m_packet_count = m_muxer_jpeg_config->buffer_pkt_num + 2;
    m_packet_size = buffer_size * 1024 * 1024 / m_packet_count;
    m_packet_pool = AMFixedPacketPool::create(pool_name.c_str(),
                    m_packet_count, m_packet_size);
    if (!m_packet_pool) {
      ERROR("Failed to create packet pool");
      ret = AM_STATE_ERROR;
      break;
    }
    m_packet_pool->enable(true);
    m_muxer_name = "JpegMuxerEvent";
    INFO("Init %s success.", m_muxer_name.c_str());
  } while(0);
  return ret;
}

AM_MUXER_ATTR AMMuxerJpegEvent::get_muxer_attr()
{
  return AM_MUXER_FILE_EVENT;
}

void AMMuxerJpegEvent::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EVENT) {
      AMEventStruct* event = (AMEventStruct*)(packet->get_data_ptr());
      if ((event->attr == AM_EVENT_MJPEG) &&
          (packet->get_stream_id() == m_curr_config.video_id)) {
        NOTICE("%s receive event packet, Event attr is AM_EVENT_MJPEG, "
            "stream id is %u, pre num is %u, after num is %u,"
            "closest num is %u", m_muxer_name.c_str(), event->mjpeg.stream_id,
            event->mjpeg.pre_cur_pts_num, event->mjpeg.after_cur_pts_num,
            event->mjpeg.closest_cur_pts_num);
        add = true;
        break;
      }
    }
    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) &&
        (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) &&
        (packet->get_stream_id() == m_curr_config.video_id)) {
      AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*)(packet->get_data_ptr());
      m_exif_image_width = video_info->width;
      m_exif_image_height = video_info->height;
    }

    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) &&
        (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)) {
      AM_VIDEO_FRAME_TYPE vtype =
          AM_VIDEO_FRAME_TYPE(packet->get_frame_type());
      add = (((vtype == AM_VIDEO_FRAME_TYPE_MJPEG) ||
          (vtype == AM_VIDEO_FRAME_TYPE_SJPEG)) &&
          (packet->get_stream_id() == m_curr_config.video_id));
    }
  } while(0);
  if (AM_LIKELY(add)) {
    packet->add_ref();
    m_packet_queue.push_back(packet);
  }
  packet->release();
}

AM_STATE AMMuxerJpegEvent::generate_file_name(char file_name[])
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(!get_proper_file_location(m_file_location))) {
      ERROR("Failed to get proper file location in %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string,
                                              sizeof(time_string),
                                              FILENAME_TIME_FORMAT)))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    sprintf(file_name, "%s/%s_event_%s",
            m_file_location.c_str(),
            m_curr_config.file_name_prefix.c_str(),
            time_string);
    INFO("Generate file success: %s in %s", file_name, m_muxer_name.c_str());
  } while(0);
  return ret;
}

void AMMuxerJpegEvent::main_loop()
{
  bool is_ok = true;
  m_run = true;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  while(m_run.load() && is_ok) {
    AMPacket* packet = nullptr;
    AMPacket* local_pkt = nullptr;
    if (m_packet_queue.empty()) {
      DEBUG("Packet queue is empty, sleep 30 ms.");
      usleep(30000);
      continue;
    }
    packet = m_packet_queue.front_and_pop();
    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
      if ((m_pts_diff == 0)) {
        if (m_last_pts > 0) {
          m_pts_diff = packet->get_pts() - m_last_pts;
        } else {
          m_last_pts = packet->get_pts();
        }
      }
      while (m_buf_pkt_queue.size() >= m_curr_config.buffer_pkt_num) {
        m_buf_pkt_queue.front_and_pop()->release();
      }
      if (m_packet_pool->get_avail_packet_num() > 0) {
        if (!m_packet_pool->alloc_packet(local_pkt, 0)) {
          ERROR("Failed to alloc packet.");
          is_ok = false;
          continue;
        }
      } else {
        ERROR("There is not enough packet in packet pool in %s",
              m_muxer_name.c_str());
        is_ok = false;
        continue;
      }
      if (packet->get_data_size() > m_packet_size) {
        ERROR("Jpeg packet data size %u is bigger than buffer packet size %u",
              packet->get_data_size(), m_packet_size);
        is_ok = false;
        continue;
      } else {
        memcpy(local_pkt->get_data_ptr(), packet->get_data_ptr(),
               packet->get_data_size());
        local_pkt->set_data_size(packet->get_data_size());
        local_pkt->set_frame_type(packet->get_frame_type());
        local_pkt->set_pts(packet->get_pts());
        local_pkt->set_type(packet->get_type());
        local_pkt->set_frame_attr(packet->get_frame_attr());
        m_buf_pkt_queue.push_back(local_pkt);
      }
    }
    if (AM_LIKELY(is_ok && m_file_writing)) {
      is_ok = process_packet(packet);
    }
    packet->release();
  }
  clear_event_jpeg_params();
  release_resource();
  if (AM_UNLIKELY(!m_run.load())) {
    AUTO_MEM_LOCK(m_state_lock);
    m_state = AM_MUXER_CODEC_STOPPED;
  }
  NOTICE("%s exit mainloop.", m_muxer_name.c_str());
}

void AMMuxerJpegEvent::clear_event_jpeg_params()
{
  while(!m_buf_pkt_queue.empty()) {
    m_buf_pkt_queue.front_and_pop()->release();
  }
  if (m_event_packet) {
    m_event_packet->release();
  }
  m_event_info.after_jpeg_num = 0;
  m_event_info.pre_jpeg_num = 0;
  m_event_info.after_jpeg_count = 0;
  m_event_info.pre_jpeg_count = 0;
  m_event_info.enable = false;
  m_pts_diff = 0;
  m_last_pts = 0;
  m_last_time_stamp.clear();
  m_time_stamp_count = 0;
  INFO("%s params have been cleared success.", m_muxer_name.c_str());
}

AM_STATE AMMuxerJpegEvent::on_event_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    int64_t event_pts = packet->get_pts();
    if (m_buf_pkt_queue.size() > 0) {
      if (event_pts > m_buf_pkt_queue.back()->get_pts()) {
        if (m_event_packet) {
          WARN("The %s is in event now.", m_muxer_name.c_str());
          break;
        }
        m_event_packet = packet;
        packet->add_ref();
        break;
      } else if (event_pts == m_buf_pkt_queue.back()->get_pts()) {
        if (!parse_event_packet(packet)) {
          ERROR("Failed to parse event packet.");
          ret = AM_STATE_ERROR;
          break;
        }
      } else {
        while(!m_buf_pkt_queue.empty()) {
          if (event_pts < m_buf_pkt_queue.back()->get_pts()) {
            m_packet_queue.push_front(m_buf_pkt_queue.back_and_pop());
          } else {
            break;
          }
        }
        if (!parse_event_packet(packet)) {
          ERROR("Failed to parse event packet.");
          ret = AM_STATE_ERROR;
          break;
        }
      }
    } else {
      if (!m_event_packet) {
        packet->add_ref();
        m_event_packet = packet;
      }
      break;
    }
  } while(0);
  return ret;
}

bool AMMuxerJpegEvent::process_packet(AMPacket* packet)
{
  bool ret = true;
  uint32_t error_count = 0;
  do {
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        if (AM_UNLIKELY(on_info_packet(packet) != AM_STATE_OK)) {
          ERROR("On av info error, exit the main loop.");
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          ret = false;
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
          ++ error_count;
          WARN("On normal data packet error, error count is %u!", error_count);
          if(error_count == JPEG_DATA_PKT_ERROR_NUM) {
            ERROR("On normal data packet error, exit the main loop");
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            ret = false;
          }
          break;
        }
        error_count = 0;
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EVENT : {
        if (m_event_packet || m_event_info.enable) {
          if (m_event_packet) {
            WARN("Last event packet have not been processsed in %s, "
                "pts is %lld. This event will be ignored",
                m_muxer_name.c_str(), packet->get_pts());
          } else {
            WARN("There are event jpeg files which are not been written in "
                "the previous event process in %s, pts is %lld. "
                "This event will be ignored",
                m_muxer_name.c_str(), packet->get_pts());
          }
        } else {
          if (AM_UNLIKELY(on_event_packet(packet) != AM_STATE_OK)) {
            ERROR("On event packet error, exit the main loop.");
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            ret = false;
          }
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        NOTICE("receive eos packet");
        if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
          ERROR("On eos packet error, exit the main loop.");
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          ret = false;
        }
      } break;
      default: {
        WARN("Unknown packet type: %u!", packet->get_type());
      } break;
    }
  } while(0);
  return ret;
}

bool AMMuxerJpegEvent::update_filename_time_stamp()
{
  bool ret = true;
  char file_name[512] = { 0 };
  do {
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string,
                                              sizeof(time_string),
                                              FILENAME_TIME_FORMAT)))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = false;
      break;
    }
    if (m_last_time_stamp == std::string(time_string)) {
      sprintf(file_name, "%s/%s_event_%s_%u",
              m_file_location.c_str(),
              m_curr_config.file_name_prefix.c_str(),
              time_string, m_time_stamp_count);
      ++ m_time_stamp_count;
    } else {
      m_time_stamp_count = 0;
      sprintf(file_name, "%s/%s_event_%s",
              m_file_location.c_str(),
              m_curr_config.file_name_prefix.c_str(),
              time_string);
    }
    m_last_time_stamp = std::string(time_string);
    INFO("Last time stamp is %s", m_last_time_stamp.c_str());
    INFO("Generate file success: %s in %s", file_name, m_muxer_name.c_str());
    if (m_file_writer->set_media_sink(file_name) != AM_STATE_OK) {
      ERROR("Failed to set media sink to file writer.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

AM_STATE AMMuxerJpegEvent::on_data_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (m_event_info.enable) {
      ++ m_event_info.after_jpeg_count;
      if (m_event_info.after_jpeg_count <= m_event_info.after_jpeg_num) {
        if (AM_UNLIKELY(m_file_writer->write_data(packet->get_data_ptr(),
                        packet->get_data_size()) != AM_STATE_OK)) {
          ERROR("Failed to write data to jpeg file.");
          ret = AM_STATE_ERROR;
          break;
        } else {
          INFO("Write jpeg event file success");
        }
        if (m_exif_config->enable) {
          uint8_t *exif_data = nullptr;
          uint32_t exif_data_len = 0;
          if (AM_UNLIKELY(create_exif_data(exif_data, exif_data_len)
                          != AM_STATE_OK)) {
            ERROR("Failed to create exif data file!");
          } else if (AM_UNLIKELY(m_file_writer->write_exif_data(exif_data,
                                 exif_data_len) != AM_STATE_OK)) {
            ERROR("Failed to write exif data to file");
          }
          if (AM_LIKELY(exif_data)) {
            free(exif_data);
          }
        }
      }
      if ((m_event_info.after_jpeg_count == m_event_info.after_jpeg_num) ||
          (m_event_info.after_jpeg_num == 0)) {
        INFO("Stop write jpeg event file success.");
        m_event_info.enable = false;
        m_event_info.after_jpeg_count = 0;
        m_event_info.after_jpeg_num = 0;
        m_event_info.pre_jpeg_count = 0;
        m_event_info.pre_jpeg_num = 0;
      }
    } else {
      if (m_event_packet) {
        if (m_event_packet->get_pts() <  packet->get_pts()) {
          m_packet_queue.push_front(m_event_packet);
          m_event_packet = nullptr;
        } else {
          break;
        }
      } else {
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMMuxerJpegEvent::parse_event_packet(AMPacket* event_packet)
{
  bool ret = true;
  do {
    m_event_info.after_jpeg_count = 0;
    m_event_info.pre_jpeg_count = 0;
    m_event_info.enable = false;
    m_event_info.pre_jpeg_num = 0;
    m_event_info.after_jpeg_num = 0;
    AMEventStruct* event = (AMEventStruct*)event_packet->get_data_ptr();
    if (event->attr != AM_EVENT_MJPEG) {
      ERROR("Event attr error in %s", m_muxer_name.c_str());
      ret = false;
      break;
    }
    if ((event->mjpeg.pre_cur_pts_num > 0) ||
        (event->mjpeg.after_cur_pts_num > 0)) {
      m_event_info.enable = true;
      if (event->mjpeg.pre_cur_pts_num > m_buf_pkt_queue.size()) {
        WARN("The pre current pts jpeg number : %u should not be bigger than "
            "the size of buffer queue : %u, set it %u as default.",
            event->mjpeg.pre_cur_pts_num, m_buf_pkt_queue.size(),
            m_buf_pkt_queue.size());
        m_event_info.pre_jpeg_num = m_buf_pkt_queue.size();
      } else {
        m_event_info.pre_jpeg_num = event->mjpeg.pre_cur_pts_num;
      }
      m_event_info.after_jpeg_num = event->mjpeg.after_cur_pts_num;
    } else if ((event->mjpeg.pre_cur_pts_num == 0) &&
        (event->mjpeg.after_cur_pts_num == 0)
        && (event->mjpeg.closest_cur_pts_num > 0)) {
      m_event_info.enable = true;
      if ((event->mjpeg.closest_cur_pts_num % 2) == 0) {
        m_event_info.pre_jpeg_num = event->mjpeg.closest_cur_pts_num / 2;
      } else {
        if ((event_packet->get_pts() - m_buf_pkt_queue.back()->get_pts())
            < m_pts_diff / 2) {
          m_event_info.pre_jpeg_num = event->mjpeg.closest_cur_pts_num / 2 + 1;
        } else {
          m_event_info.pre_jpeg_num = event->mjpeg.closest_cur_pts_num / 2;
        }
      }
      if (m_event_info.pre_jpeg_num > m_buf_pkt_queue.size()) {
        WARN("The pre jpeg num is bigger than buffer packet number, set pre jpeg"
            "num as %u", m_buf_pkt_queue.size());
        m_event_info.pre_jpeg_num = m_buf_pkt_queue.size();
      }
      m_event_info.after_jpeg_num = event->mjpeg.closest_cur_pts_num -
          m_event_info.pre_jpeg_num;
    } else {
      m_event_info.enable = false;
      m_event_info.pre_jpeg_num = 0;
      m_event_info.pre_jpeg_count = 0;
      m_event_info.after_jpeg_num = 0;
      m_event_info.after_jpeg_count = 0;
      ret = false;
      break;
    }
    INFO("%s receive event packet,\n"
        "pre current pts jpeg number is %u, pre count is %u\n "
        "after current pts jpeg number is %u, after count is %u",
        m_muxer_name.c_str(),
        m_event_info.pre_jpeg_num, m_event_info.pre_jpeg_count,
        m_event_info.after_jpeg_num, m_event_info.after_jpeg_count);
    if (!update_filename_time_stamp()) {
      ERROR("Failed to update file name time stamp.");
      ret = false;
      break;
    }
    AMPacket* pre_packet = nullptr;
    for (uint32_t i = 1; i <= m_buf_pkt_queue.size(); ++ i) {
      pre_packet = m_buf_pkt_queue.front_and_pop();
      m_buf_pkt_queue.push_back(pre_packet);
      if ((m_buf_pkt_queue.size() - i) >= m_event_info.pre_jpeg_num) {
        continue;
      }
      if (AM_UNLIKELY(m_file_writer->write_data(pre_packet->get_data_ptr(),
                      pre_packet->get_data_size()) != AM_STATE_OK)) {
        ERROR("Failed to write data to jpeg file.");
        ret = false;
        break;
      } else {
        INFO("Write pre jpeg event file success.");
      }
      if (m_exif_config->enable) {
        uint8_t *exif_data = nullptr;
        uint32_t exif_data_len = 0;
        if (AM_UNLIKELY(create_exif_data(exif_data, exif_data_len)
                        != AM_STATE_OK)) {
          ERROR("Failed to create exif data file!");
        } else if (AM_UNLIKELY(m_file_writer->write_exif_data(exif_data,
                               exif_data_len) != AM_STATE_OK)) {
          ERROR("Failed to write exif data to file");
        }
        if (AM_LIKELY(exif_data)) {
          free(exif_data);
        }
      }
    }
  } while(0);
  return ret;
}
