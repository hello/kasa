/*******************************************************************************
 * am_muxer_AV3_normal.cpp
 *
 * History:
 *   2016-09-07 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#include "am_muxer_av3_builder.h"
#include "am_muxer_av3_file_writer.h"
#include "am_muxer_av3_normal.h"

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerAv3Normal::create(config_file));
}

AMMuxerAv3Normal* AMMuxerAv3Normal::create(const char* config_file)
{
  AMMuxerAv3Normal* result = new AMMuxerAv3Normal();
  if (result && result->init(config_file) != AM_STATE_OK) {
    ERROR("Failed to create AV3 muxer normal");
    delete result;
    result = nullptr;
  }
  return result;
}

AM_STATE AMMuxerAv3Normal::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  INFO("Begin to init AV3MuxerNormal.");
  do {
    if(AM_UNLIKELY((ret = inherited::init(config_file)) != AM_STATE_OK)) {
      ERROR("Failed to init AMMuxerAV3Base object");
      break;
    }
    m_muxer_name = std::string("AV3MuxerNormal-stream") +
                   std::to_string(m_curr_config.video_id);
    INFO("Init %s success.", m_muxer_name.c_str());
  } while(0);
  return ret;
}

AMMuxerAv3Normal::AMMuxerAv3Normal()
{
}

AMMuxerAv3Normal::~AMMuxerAv3Normal()
{
  INFO("%s has been destroyed.", m_muxer_name.c_str());
}

AM_STATE AMMuxerAv3Normal::generate_file_name(std::string &file_name)
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
                                              sizeof(time_string))))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    file_name = m_file_location + "/" + m_curr_config.file_name_prefix +
        "_" + time_string + "_stream" + std::to_string(m_curr_config.video_id);
    INFO("Generate file success: %s in %s", file_name.c_str(),
         m_muxer_name.c_str());
  } while(0);
  return ret;
}

void AMMuxerAv3Normal::clear_params_for_new_file()
{
  m_last_video_pts = 0;
  m_first_video_pts = 0;
  m_eos_map = 0;
  m_video_frame_count = 0;
  m_last_frame_number = 0;
  m_is_audio_accepted = false;
  m_is_video_arrived = false;
  m_is_first_video = true;
}

bool AMMuxerAv3Normal::start_file_writing()
{
  bool ret = true;
  INFO("Begin to start file writing in %s.", m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_file_param_lock);
    if (m_file_writing) {
      NOTICE("File writing is already started in %s.", m_muxer_name.c_str());
      break;
    }
    m_file_writing = true;
  } while (0);
  if (ret) {
    NOTICE("start file writing success in %s.", m_muxer_name.c_str());
  }
  return ret;
}

AM_MUXER_ATTR AMMuxerAv3Normal::get_muxer_attr()
{
  return AM_MUXER_FILE_NORMAL;
}

void AMMuxerAv3Normal::feed_data(AMPacket* packet)
{
  bool add = false;
  bool audio_enable = (m_curr_config.audio_type == AM_AUDIO_NULL) ?
                      false : true;
  do {
    if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL) == 0) {
      /*just need normal packet*/
      break;
    }
    if ((!audio_enable) && (packet->get_attr() ==
        AMPacket::AM_PAYLOAD_ATTR_AUDIO)) {
      break;
    }
    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) ||
        (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO)) {
      /*info and eos packet*/
      switch (packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO : {
          if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
            AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*) (packet->get_data_ptr());
            if (!video_info) {
              ERROR("%s received video info is null", m_muxer_name.c_str());
              add = false;
              break;
            }
            if((packet->get_stream_id() == m_curr_config.video_id) &&
                ((video_info->type == AM_VIDEO_H264) ||
                    (video_info->type == AM_VIDEO_H265))) {
              add = true;
            }
          } else {
            if (packet->get_stream_id() == m_curr_config.video_id) {
              add = true;
            }
          }
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO : {
          if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
            AM_AUDIO_INFO *ainfo = (AM_AUDIO_INFO*)packet->get_data_ptr();
            if (ainfo->type == m_curr_config.audio_type) {
              if (!m_audio_sample_rate_set) {
                NOTICE("Set audio frame rate as %u in %s", ainfo->sample_rate,
                       m_muxer_name.c_str());
                m_curr_config.audio_sample_rate = ainfo->sample_rate;
                m_set_config.audio_sample_rate = ainfo->sample_rate;
                m_audio_sample_rate_set = true;
              }
              if (ainfo->sample_rate == m_curr_config.audio_sample_rate) {
                add = true;
              }
            }
          } else {
            add = ((AM_AUDIO_TYPE(packet->get_frame_type())
                          == m_curr_config.audio_type) &&
                (packet->get_frame_attr() == m_curr_config.audio_sample_rate));
          }
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_GSENSOR : {
          add = true;
        } break;
        default : {
          NOTICE("%s received invalid eos or info packet", m_muxer_name.c_str());
        } break;
      }
    } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
      /*audio and video data packet*/
      switch (packet->get_attr()) {
        case AMPacket::AM_PAYLOAD_ATTR_VIDEO : {
          /* todo: Only support H.264 add H.265, add Mjpeg support ? */
          AM_VIDEO_TYPE vtype = AM_VIDEO_TYPE(packet->get_video_type());
          add = (packet->get_stream_id() == m_curr_config.video_id) &&
              ((vtype == AM_VIDEO_H264) || (vtype == AM_VIDEO_H265));
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_AUDIO : {
          add = ((AM_AUDIO_TYPE(packet->get_frame_type())
              == m_curr_config.audio_type) &&
              (packet->get_frame_attr() == m_curr_config.audio_sample_rate));
        } break;
        case AMPacket::AM_PAYLOAD_ATTR_GSENSOR : {
          add = true;
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

void AMMuxerAv3Normal::main_loop()
{
  bool is_ok = true;
  AM_STATE packet_process_state = AM_STATE_OK;
  uint32_t statfs_number = 0;
  m_run = true;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  if (m_run && is_ok) {
    INFO("%s entry mainloop success.", m_muxer_name.c_str());
  }
  while(m_run && is_ok) {
    AMPacket* packet = NULL;
    if(m_packet_queue->empty()) {
      DEBUG("Packet queue is empty, sleep 100 ms.");
      usleep(100000);
      continue;
    }
    packet = m_packet_queue->front();
    m_packet_queue->pop_front();
    AUTO_MEM_LOCK(m_file_param_lock);
    switch (packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        if (AM_UNLIKELY(on_info_pkt(packet) != AM_STATE_OK)) {
          ERROR("On info packet error in %s, exit the main loop.",
                m_muxer_name.c_str());
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          is_ok = false;
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        packet_process_state = on_data_pkt(packet);
        if (AM_UNLIKELY(packet_process_state != AM_STATE_OK)) {
          if(packet_process_state == AM_STATE_IO_ERROR) {
            ERROR("File system IO error in %s, stop file recording.",
                  m_muxer_name.c_str());
            m_file_writing = false;
            if (m_av3_builder) {
              if (AM_UNLIKELY(!end_file())) {
                ERROR("Failed to end AV3 file in %s.", m_muxer_name.c_str());
              }
            }
            if (m_file_writer) {
              if (m_file_writer->close_file(m_curr_config.write_sync_enable)
                  != AM_STATE_OK) {
                ERROR("Failed to close file %s in %s.",
                 m_file_writer->get_current_file_name(), m_muxer_name.c_str());
              }
            }
          } else {
            ++ error_count;
            WARN("On normal data packet error in %s, error count is %u!",
                 m_muxer_name.c_str(), error_count);
            if (error_count == ON_DATA_PKT_ERROR_NUM) {
              ERROR("On normal data packet error in %s, exit the main loop",
                    m_muxer_name.c_str());
              AUTO_MEM_LOCK(m_state_lock);
              m_state = AM_MUXER_CODEC_ERROR;
              is_ok = false;
            }
          }
          break;
        }
        error_count = 0;
        ++ statfs_number;
        if ((statfs_number >= CHECK_FREE_SPACE_FREQUENCY) &&
            m_file_writing) {
          statfs_number = 0;
          check_storage_free_space();
        }
      } break;
      case AMPacket::AM_PAYLOAD_TYPE_EOS: {
        NOTICE("Received eos packet in %s", m_muxer_name.c_str());
        if (AM_UNLIKELY(on_eos_pkt(packet) != AM_STATE_OK)) {
          ERROR("On eos packet error in %s, exit the main loop.",
                m_muxer_name.c_str());
          AUTO_MEM_LOCK(m_state_lock);
          m_state = AM_MUXER_CODEC_ERROR;
          is_ok = false;
        }
      } break;
      default: {
        WARN("Unknown packet type: %u in %s!", packet->get_type(),
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
  NOTICE("%s exit mainloop.", m_muxer_name.c_str());
}

AM_STATE AMMuxerAv3Normal::on_info_pkt(AMPacket* packet)
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
          video_info->stream_id, video_info->width,
          video_info->height, video_info->M, video_info->N,
          video_info->rate, video_info->scale, video_info->fps,
          video_info->mul, video_info->div);
      if ((video_info->width == 0) || (video_info->height == 0)
          || (video_info->scale == 0) || (video_info->div == 0)) {
        ERROR("%s received invalid video info.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      if (((m_av_info_map >> 1) & 0x01) == 0x01) {
        /*video info is set already, check new video info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the AV3 builder, write media data to new file*/
        if(memcmp(&m_video_info, video_info, sizeof(AM_VIDEO_INFO)) == 0) {
          NOTICE("%s receives new info pkt, but is not changed. Discard it.",
                 m_muxer_name.c_str());
          break;
        }
        NOTICE("%s receives new video info pkt and it's value is changed, "
               "close current file, create new file.", m_muxer_name.c_str());
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_av3_builder->set_video_info(&m_video_info)
                        != AM_STATE_OK)) {
          ERROR("Failed to set video info to AV3 builder in AV3 normal muxer.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(!m_file_writer->set_video_info(&m_video_info))) {
          ERROR("Failed to set video info in %s", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_av3_builder->reset_video_params();
      } else {
        /*first video info packet*/
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_av3_builder->set_video_info(video_info)
            != AM_STATE_OK)) {
          ERROR("Failed to set video info to AV3 builder in %s.",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= (0x01 << 1);
        INFO("%s av info map is %u", m_muxer_name.c_str(), m_av_info_map);
        if (m_file_writing && (!m_file_writer->is_file_open())) {
          update_config_param();
          if (m_file_writer->create_next_file() != AM_STATE_OK) {
            ERROR("Failed to create new file %s in %s",
                  m_file_writer->get_current_file_name(), m_muxer_name.c_str());
            ret = AM_STATE_ERROR;
            break;
          }
          ++ m_file_counter;
          if (AM_UNLIKELY((ret = m_av3_builder->begin_file()) != AM_STATE_OK)) {
            ERROR("Failed to start building AV3 file in %s",
                  m_muxer_name.c_str());
            break;
          }
        }
      }
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /*audio_info*/
      AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO*) (packet->get_data_ptr());
      if (!audio_info) {
        ERROR("%s received audio info is null", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      INFO("\n%s receives Audio INFO: \n"
          "      sample_rate: %u\n"
          "       chuck_size: %u\n"
          "         channels: %u\n"
          "pkt_pts_increment: %u\n"
          "      sample_size: %u\n"
          "       audio_type: %s",
          m_muxer_name.c_str(),
          audio_info->sample_rate, audio_info->chunk_size,
          audio_info->channels, audio_info->pkt_pts_increment,
          audio_info->sample_size,
          audio_type_to_string(audio_info->type).c_str());
      if ((m_av_info_map & 0x01) == 0x01) {
        /*audio info is set already, check new audio info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the AV3 builder, write media data to new file*/
        if (memcmp(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO)) == 0) {
          NOTICE("%s receivs new audio info, but it is not changed, ignore it.",
                 m_muxer_name.c_str());
          break;
        }
        NOTICE("%s receives new audio info pkt and it's value is changed, "
            "close current file, create new file.", m_muxer_name.c_str());
        memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
        if (AM_UNLIKELY(m_av3_builder->set_audio_info(&m_audio_info)
                        != AM_STATE_OK)) {
          ERROR("Failed to set audio info to AV3 builder in %s.",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
      } else {
        /*first audio info packet*/
        memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
        m_av3_builder->set_audio_info(audio_info);
        m_av_info_map |= 0x01;
        INFO("%s av info map is %u", m_muxer_name.c_str(), m_av_info_map);
      }
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_GSENSOR) {
      if (!m_curr_config.gsensor_enable) {
        break;
      }
      AM_GSENSOR_INFO *gsensor_info = (AM_GSENSOR_INFO*)(packet->get_data_ptr());
      INFO("%s receive gsensor info:\n"
           "              stream_id: %u\n"
           "            sample_rate: %u\n"
           "            sample_size: %u\n"
           "      pkt_pts_increment: %u.",
           m_muxer_name.c_str(),
           gsensor_info->stream_id,
           gsensor_info->sample_rate,
           gsensor_info->sample_size,
           gsensor_info->pkt_pts_increment);
      break;
    } else {
      NOTICE("Currently, %s only support audio and video stream.",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerAv3Normal::on_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (!m_file_writing) {
        break;
      }
      if (AM_UNLIKELY((ret = check_video_pts(packet)) != AM_STATE_OK)) {
        break;
      }
      if (AM_UNLIKELY((ret = write_video_data_pkt(packet)) != AM_STATE_OK)) {
        ERROR("Failed to write video data packet in %s", m_muxer_name.c_str());
        break;
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      if ((!m_file_writing) || (!m_is_video_arrived)) {
        break;
      }
      packet->add_ref();
      m_audio_pkt_list->push_back(packet);
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_GSENSOR: {
      if ((!m_file_writing) || (!m_is_video_arrived) ||
          (!m_curr_config.gsensor_enable)) {
        break;
      }
      packet->add_ref();
      m_meta_pkt_list->push_back(packet);
    } break;
    default: {
      NOTICE("Currently, %s just support audio video and gsensor packet",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
    } break;
  }
  return ret;
}

AM_STATE AMMuxerAv3Normal::check_video_pts(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if ((packet->get_pts() - m_last_video_pts) < 0) {
      if ((packet->get_frame_type() != AM_VIDEO_FRAME_TYPE_B)) {
        WARN("video pts abnormal in %s :\n "
            "last pts : %lld, current pts : %lld,"
            "pts diff : %lld,\n last frame number : %u, "
            "current frame number : %u, frame number diff : %d.",
            m_muxer_name.c_str(),
            m_last_video_pts, packet->get_pts(),
            (packet->get_pts() - m_last_video_pts),
            m_last_frame_number, packet->get_frame_number(),
            (int32_t )(packet->get_frame_number() - m_last_frame_number));
        ret = AM_STATE_ERROR;
#if 0
        if (m_video_frame_count > 0) {
          ERROR("m_video_frame_count is %u", m_video_frame_count);
          if (AM_UNLIKELY(on_eof_pkt(packet) != AM_STATE_OK)) {
            ERROR("On eof error.");
          }
        }
#endif
      }
      break;
    }
    if ((m_last_video_pts > 0)
        && ((packet->get_pts() - m_last_video_pts) > 15000)) {
      WARN("video pts anomaly in %s : "
          "last pts : %lld, current pts : %lld,"
          "pts diff : %lld, \nlast frame number : %u, "
          "current frame number: %u, frame number diff : %d",
          m_muxer_name.c_str(),
          m_last_video_pts, packet->get_pts(),
          (packet->get_pts() - m_last_video_pts),
          m_last_frame_number, packet->get_frame_number(),
          (int32_t )(packet->get_frame_number() - m_last_frame_number));
      if (packet->get_frame_number() == 1) {
        NOTICE("vsync loss recovery in %s, close current AV3 file, "
            "create a new file.", m_muxer_name.c_str());
        if (m_video_frame_count > 0) {
          if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
            ERROR("On eof error in %s.", m_muxer_name.c_str());
            ret = AM_STATE_ERROR;
          }
        }
      }
    }
  } while (0);
#if 0
  if (ret != AM_STATE_OK) {
    m_last_video_pts = packet->get_pts();
    m_last_frame_number = packet->get_frame_number();
  }
#endif
  return ret;
}
