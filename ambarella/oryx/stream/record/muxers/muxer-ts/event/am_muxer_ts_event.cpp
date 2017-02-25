/*
 * am_muxer_ts.cpp
 *
 * 11/09/2012 [Hanbo Xiao] [Created]
 * 17/12/2014 [Chengcai Jing] [Modified]
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
 */

#include "am_base_include.h"
#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_packet.h"
#include "am_log.h"
#include "am_define.h"
#include "am_mutex.h"
#include "am_thread.h"

#include "mpeg_ts_defs.h"
#include "am_muxer_ts_builder.h"
#include "am_muxer_ts_file_writer.h"
#include "am_muxer_ts_event.h"
#include "am_audio_define.h"
#include "am_record_event_sender.h"

AMIMuxerCodec* get_muxer_codec(const char* config)
{
  return AMTsMuxerEvent::create(config);
}

AMIMuxerCodec *AMTsMuxerEvent::create(const char* config_file)
{
  AMTsMuxerEvent *result = new AMTsMuxerEvent();
  if (result != NULL && result->init(config_file) != AM_STATE_OK) {
    ERROR("Failed to init ts event muxer.");
    delete result;
    result = NULL;
  }
  return ((AMIMuxerCodec*) result);
}

bool AMTsMuxerEvent::start_file_writing()
{
  bool ret = true;
  INFO("Begin to start file writing in %s.", m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_file_param_lock);
    if (m_file_writing) {
      NOTICE("File writing is already startted in %s.", m_muxer_name.c_str());
      break;
    }
    m_file_writing = true;
  } while (0);
  if (!ret) {
    NOTICE("Start file writing success in %s.", m_muxer_name.c_str());
  }
  return ret;
}

AM_MUXER_ATTR AMTsMuxerEvent::get_muxer_attr()
{
  return AM_MUXER_FILE_EVENT;
}

void AMTsMuxerEvent::clear_params_for_new_file()
{
  m_last_video_pts = 0;
  m_is_first_audio = true;
  m_is_first_video = true;
  m_audio_enable = false;
  m_pcr_base = 0;
  m_pcr_ext = 0;
  m_video_frame_count = 0;
  m_pts_base_video = 0LL;
  m_pts_base_audio = 0LL;
  m_first_video_pts = 0;
  m_adts.clear();
}

void AMTsMuxerEvent::feed_data(AMPacket* packet)
{
  bool add = false;
  bool audio_enable = (m_curr_config.audio_type == AM_AUDIO_NULL) ?
      false : true;
  do {
    if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) == 0) {
      /*just need event packet*/
      break;
    }
    if ((!audio_enable) && (packet->get_attr() ==
        AMPacket::AM_PAYLOAD_ATTR_AUDIO)) {
      break;
    }
    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EVENT) {
      AMEventStruct* event = (AMEventStruct*)(packet->get_data_ptr());
      if ((event->attr == AM_EVENT_H26X) &&
          (event->h26x.stream_id == m_curr_config.video_id)) {
        NOTICE("%s receive event packet, Event attr is AM_EVENT_H26X, "
            "video id is %u", m_muxer_name.c_str(), event->h26x.stream_id);
      } else {
        break;
      }
    }
    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
      /* todo: Only support H.264 add H.265, add Mjpeg support ? */
      switch (packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
          AM_VIDEO_INFO* video_info =
              (AM_VIDEO_INFO*) (packet->get_data_ptr());
          if (!video_info) {
            ERROR("%s received video info is null", m_muxer_name.c_str());
            add = false;
            break;
          }
          if ((packet->get_stream_id() == m_curr_config.video_id)
              && ((video_info->type == AM_VIDEO_H264)
                  || (video_info->type == AM_VIDEO_H265))) {
            add = true;
          }
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
          AM_AUDIO_INFO *ainfo = (AM_AUDIO_INFO*)packet->get_data_ptr();
          if (ainfo->type == m_curr_config.audio_type) {
            if (!m_audio_sample_rate_set) {
              m_curr_config.audio_sample_rate = ainfo->sample_rate;
              m_set_config.audio_sample_rate = ainfo->sample_rate;
              m_audio_sample_rate_set = true;
            }
            if (ainfo->sample_rate == m_curr_config.audio_sample_rate) {
              add = true;
            }
          }
        } break;
        default: {
          NOTICE("%s received invalid eos or info packet", m_muxer_name.c_str());
        } break;
      }
    } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
      /*audio and video data packet*/
      switch (packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
          /* todo: Only support H.264 add H.265, add Mjpeg support ? */
          AM_VIDEO_TYPE vtype = AM_VIDEO_TYPE(packet->get_video_type());
          add = (packet->get_stream_id() == m_curr_config.video_id)
                  && ((vtype == AM_VIDEO_H264) || (vtype == AM_VIDEO_H265));
          if (add) {
            m_video_feeded = true;
          }
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
          add = ((AM_AUDIO_TYPE(packet->get_frame_type())
              == m_curr_config.audio_type) && m_video_feeded &&
              (packet->get_frame_attr() == m_curr_config.audio_sample_rate));
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_GPS: {
          add = false;
        } break;
        default:
          break;
      }
    } else {
      break;
    }
  } while (0);
  if (AM_LIKELY(add)) {
    packet->add_ref();
    m_packet_queue->push_back(packet);
  }
  packet->release();
}

void AMTsMuxerEvent::main_loop()
{
  bool is_ok = true;
  m_run = true;
  uint32_t statfs_number = 0;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  if (m_run.load() && is_ok) {
    INFO("%s entry mainloop success.", m_muxer_name.c_str());
  }
  while (m_run.load() && is_ok) {
    AMPacket *packet = NULL;
    if (m_packet_queue->empty()) {
      DEBUG("packet queue is empty, sleep 100 ms.");
      usleep(100000);
      continue;
    }
    packet = m_packet_queue->front_and_pop();
    AUTO_MEM_LOCK(m_file_param_lock);
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        if (AM_UNLIKELY(on_info_packet(packet) != AM_STATE_OK)) {
          ERROR("On AV info error, exit the main loop.");
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          is_ok = false;
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
          ++ error_count;
          WARN("On normal data packet error, error count is %u!", error_count);
          if (error_count == ON_DATA_PKT_ERROR_NUM) {
            ERROR("On normal data packet error, error count is %u,"
                " exit mainloop",
                ON_DATA_PKT_ERROR_NUM);
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
          break;
        }
        error_count = 0;
        ++ statfs_number;
        if ((statfs_number >= CHECK_FREE_SPACE_FREQUENCY) && m_file_writing) {
          statfs_number = 0;
          check_storage_free_space();
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        NOTICE("Received eos packet in %s", m_muxer_name.c_str());
        if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
          ERROR("On EOS error in %s, exit the main loop!",
                m_muxer_name.c_str());
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          is_ok = false;
        }
      } break;
      default: {
        WARN("Unknown packet type: %u in %s!",
             packet->get_type(),
             m_muxer_name.c_str());
      } break;
    }
    packet->release();
  }
  release_resource();
  if (AM_UNLIKELY(!m_run.load())) {
    AUTO_MEM_LOCK(m_state_lock);
    m_state = AM_MUXER_CODEC_STOPPED;
  }
  NOTICE("%s exit mainloop", m_muxer_name.c_str());
}

AMTsMuxerEvent::AMTsMuxerEvent() : m_video_feeded(false)
{
}

AM_STATE AMTsMuxerEvent::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY((ret = inherited::init(config_file)) != AM_STATE_OK)) {
      ERROR("Failed to init AMMuxerTsBase object");
      break;
    }
    m_muxer_name = std::string("TsMuxerEvent") +
        std::to_string(m_curr_config.video_id);
    INFO("Init %s success, video id is %u",
         m_muxer_name.c_str(), m_curr_config.video_id);
  } while (0);
  return ret;
}

AMTsMuxerEvent::~AMTsMuxerEvent()
{
  INFO("%s has been destroyed.", m_muxer_name.c_str());
}

AM_STATE AMTsMuxerEvent::on_info_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /*video info*/
      AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*) (packet->get_data_ptr());
      if (!video_info) {
        ERROR("%s received video info is null", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      if (video_info->type == AM_VIDEO_H264) {
        m_video_stream_info.type = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
      } else if (video_info->type == AM_VIDEO_H265) {
        m_video_stream_info.type = MPEG_SI_STREAM_TYPE_HEVC_VIDEO;
      } else {
        ERROR("Video type is unknown in %s.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      INFO("\n%s receive %s INFO:\n"
          "stream_id: %u\n"
          "     size: %d x %d\n"
          "        M: %d\n"
          "        N: %d\n"
          "     rate: %d\n"
          "    scale: %d\n"
          "      fps: %d\n"
          "      mul: %u\n"
          "      div: %u\n",
          m_muxer_name.c_str(),
          (video_info->type == AM_VIDEO_H264) ? "H.264" :
           (video_info->type == AM_VIDEO_H265) ? "H.265" : "Unknown",
          video_info->stream_id, video_info->width, video_info->height,
          video_info->M, video_info->N, video_info->rate,
          video_info->scale, video_info->fps, video_info->mul,
          video_info->div);
      if ((video_info->width == 0) || (video_info->height == 0)
          || (video_info->scale == 0) || (video_info->div == 0)) {
        ERROR("%s received invalid video info.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      if (((m_av_info_map >> 1) & 0x01) == 0x01) {
        /*video info is set already, check new video info. If it is not
         * changed, discard it. if it is changed, close current file,
         * create new ts file and write media data to new file*/
        if (memcmp(&m_video_info, video_info, sizeof(AM_VIDEO_INFO)) == 0) {
          NOTICE("%s receives new info pkt, but is not changed. Discard it.",
                 m_muxer_name.c_str());
          break;
        }
        NOTICE("%s receives new video info pkt and it's value is changed, "
            "close current file, create new file.",
            m_muxer_name.c_str());
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(set_video_info(&m_video_info) != AM_STATE_OK)) {
          ERROR("Failed to set video info in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_is_new_info = true;
      } else {
        /*first video info packet*/
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(set_video_info(&m_video_info) != AM_STATE_OK)) {
          ERROR("Failed to set video info in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 1;
        if (m_file_writing && (m_av_info_map == 3)) {
          if (!m_file_writer->is_file_open()) {
            update_config_param();
            if(update_filename_timestring() != AM_STATE_OK) {
              ERROR("Failed to update file name time string in %s.",
                    m_muxer_name.c_str());
              ret = AM_STATE_ERROR;
              break;
            }
            if (m_file_writer->create_next_file() != AM_STATE_OK) {
              ERROR("Failed to create new file %s in %s",
                    m_file_writer->get_current_file_name(),
                    m_muxer_name.c_str());
              ret = AM_STATE_ERROR;
              break;
            }
          }
          if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
            ERROR("build pat pmt error.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
        INFO("%s av info map is %u", m_muxer_name.c_str(), m_av_info_map);
      }
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /* Audio info */
      AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO *) packet->get_data_ptr();
      INFO("\n%s receives Audio INFO: \n"
          "      sample_rate: %u\n"
          "       chuck_size: %u\n"
          "         channels: %u\n"
          "pkt_pts_increment: %u\n"
          "      sample_size: %u\n"
          "       audio_type: %s",
          m_muxer_name.c_str(), audio_info->sample_rate, audio_info->chunk_size,
          audio_info->channels, audio_info->pkt_pts_increment,
          audio_info->sample_size,
          audio_type_to_string(audio_info->type).c_str());
      if ((m_av_info_map & 0x01) == 0x01) {
        /*audio info is set already, check new audio info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the mp4 builder, write media data to new file*/
        if (memcmp(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO)) == 0) {
          NOTICE("%s receivs new audio info, but it is not changed, ignore it.",
                 m_muxer_name.c_str());
          break;
        }
        NOTICE("%s receives new audio info pkt and it's value is changed, "
            "close current file, create new file.",
            m_muxer_name.c_str());
        memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
        if (AM_UNLIKELY(set_audio_info(&m_audio_info) != AM_STATE_OK)) {
          ERROR("Failed to set audio info in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_is_new_info = true;
      } else {
        memcpy(&m_audio_info, audio_info, sizeof(m_audio_info));
        if (AM_UNLIKELY(set_audio_info(&m_audio_info) != AM_STATE_OK)) {
          ERROR("Failed to set audio info.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 0;
        if (m_file_writing && (m_av_info_map == 3)) {
          if (!m_file_writer->is_file_open()) {
            update_config_param();
            if(update_filename_timestring() != AM_STATE_OK) {
              ERROR("Failed to update file name time string in %s.",
                    m_muxer_name.c_str());
              ret = AM_STATE_ERROR;
              break;
            }
            if (m_file_writer->create_next_file() != AM_STATE_OK) {
              ERROR("Failed to create new file %s in %s",
                    m_file_writer->get_current_file_name(),
                    m_muxer_name.c_str());
              ret = AM_STATE_ERROR;
              break;
            }
          }
          if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
            ERROR("build pat pmt error.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      }
    } else {
      NOTICE("Currently, %s just support audio and video stream.",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerEvent::on_data_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    /*PCR(program clock reference) overflow*/
    if (!check_pcr_overflow(packet)) {
      ERROR("Failed to check pcr overflow.");
      ret = AM_STATE_ERROR;
      break;
    }
    switch (packet->get_attr()) {
      case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
        if (!m_file_writing) {
          break;
        }
        if ((m_file_writer) && (!m_file_writer->is_file_open())) {
          if (!create_new_file()) {
            ERROR("Failed to create new file in %s", m_muxer_name.c_str());
            ret = AM_STATE_ERROR;
            break;
          }
        }
        if (m_video_info.type == AM_VIDEO_H265) {
          if (process_h265_data_pkt(packet) != AM_STATE_OK) {
            ERROR("Failed to write video data packet in %s",
                  m_muxer_name.c_str());
            ret = AM_STATE_IO_ERROR;
            break;
          }
        } else if (m_video_info.type == AM_VIDEO_H264) {
          if (process_h264_data_pkt(packet) != AM_STATE_OK) {
            ERROR("Failed to write video data packet in %s",
                  m_muxer_name.c_str());
            ret = AM_STATE_IO_ERROR;
            break;
          }
        } else {
          ERROR("Video type error in %s", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
      } break;
      case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
        if (!m_file_writing) {
          break;
        }
        if (process_audio_data_pkt(packet) != AM_STATE_OK) {
          ERROR("Failed to write audio data packet in %s",
                m_muxer_name.c_str());
          ret = AM_STATE_IO_ERROR;
          break;
        }
      }
      break;
      default: {
        DEBUG("Received unknown packet in %s.", m_muxer_name.c_str());
      }
      break;
    }
    if (AM_UNLIKELY(((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_STOP)
        != 0))) {
      if ((m_file_writer->close_file(m_curr_config.write_sync_enable)
          != AM_STATE_OK)) {
        ret = AM_STATE_IO_ERROR;
      }
      m_video_feeded = false;
      m_av_info_map = 0;
      while(!m_hevc_pkt_queue->empty()) {
        m_hevc_pkt_queue->front_and_pop()->release();
      }
      clear_params_for_new_file();
      NOTICE("Receive event stop packet in %s, stop writing TS file.",
             m_muxer_name.c_str());
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerEvent::generate_file_name(std::string &file_name)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(!get_proper_file_location(m_file_location))) {
      ERROR("Failed to get proper file location in %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_curr_config.hls_enable) {
      if (AM_UNLIKELY((symlink(m_file_location.c_str(),
         "/webSvr/web/media/media_file") < 0) && (errno != EEXIST))) {
        PERROR("Failed to symlink hls file location in TsMuxerNormal.");
      }
    }
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string, sizeof(time_string))))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    file_name = m_file_location + "/" + m_curr_config.file_name_prefix +
        "_" + time_string +"_event" + std::to_string(m_curr_config.video_id);
    INFO("Generate file success: %s in %s", file_name.c_str(),
         m_muxer_name.c_str());
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerEvent::update_filename_timestring()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    char file_name[strlen(m_curr_config.file_location.c_str())
        + strlen(m_curr_config.file_name_prefix.c_str()) + 128];
    memset(file_name, 0, sizeof(file_name));
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string, sizeof(time_string))))) {
      ERROR("Get current time string error.");
      ret = AM_STATE_ERROR;
      break;
    }
    sprintf(file_name, "%s/%s_event_%s", m_file_location.c_str(),
            m_curr_config.file_name_prefix.c_str(), time_string);
    if(AM_UNLIKELY(m_file_writer->set_media_sink(file_name) != AM_STATE_OK)) {
      ERROR("Failed to set media sink to file writer.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}
