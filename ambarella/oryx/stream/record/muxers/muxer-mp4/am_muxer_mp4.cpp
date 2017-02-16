/*******************************************************************************
 * am_muxer_mp4.cpp
 *
 * History:
 *   2015-1-9 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
#include "am_muxer_mp4_builder.h"
#include "am_muxer_mp4_file_writer.h"
#include "am_muxer_mp4.h"

#include <time.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <iostream>
#include <fstream>
#include <ctype.h>

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerMp4::create(config_file));
}

AMMuxerMp4* AMMuxerMp4::create(const char* config_file)
{
  AMMuxerMp4* result = new AMMuxerMp4();
  if(AM_UNLIKELY(result && result->init(config_file) != AM_STATE_OK)) {
    ERROR("Failde to create mp4 muxer codec.");
    delete result;
    result = NULL;
  }
  return result;
}

AMMuxerMp4::AMMuxerMp4() :
    m_thread(NULL),
    m_state_lock(NULL),
    m_lock(NULL),
    m_file_writing_lock(NULL),
    m_muxer_mp4_config(NULL),
    m_config(NULL),
    m_packet_queue(NULL),
    m_sei_queue(NULL),
    m_config_file(NULL),
    m_mp4_builder(NULL),
    m_file_writer(NULL),
    m_last_video_pts(0),
    m_splitted_duration(0LLU),
    m_next_file_boundary(0LLU),
    m_eos_map(0),
    m_av_info_map(0),
    m_file_video_frame_count(0),
    m_video_frame_count(0),
    m_stream_id(0),
    m_state(AM_MUXER_CODEC_INIT),
    m_muxer_attr(AM_MUXER_FILE_NORMAL),
    m_event_audio_type(AM_AUDIO_NULL),
    m_run(false),
    m_audio_enable(false),
    m_is_first_audio(true),
    m_is_first_video(true),
    m_need_splitted(false),
    m_is_new_info(false),
    m_start_file_writing(true)
{
}

AM_STATE AMMuxerMp4::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(config_file == NULL)) {
      ERROR("config_file is NULL, should input valid config_file");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config_file = amstrdup(config_file);
    if (AM_UNLIKELY((m_packet_queue = new packet_queue()) == NULL)) {
      ERROR("Failed to create packet_queue.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    if (AM_UNLIKELY((m_sei_queue = new packet_queue()) == NULL)) {
      ERROR("Failed to create sei_queue.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    if (AM_UNLIKELY((m_lock = AMSpinLock::create()) == NULL)) {
      ERROR("Failed to create lock.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_state_lock = AMSpinLock::create()) == NULL)) {
      ERROR("Failed to create m_state_lock.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_file_writing_lock = AMSpinLock::create()) == NULL)) {
      ERROR("Failed to create m_file_writing_lock.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_config = new AMMuxerMp4Config()) == NULL)) {
      ERROR("Failed to create mp4 config class.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_muxer_mp4_config = m_config->get_config(std::string(m_config_file));
    if (AM_UNLIKELY(!m_muxer_mp4_config)) {
      ERROR("Failed to get config");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_muxer_mp4_config->hls_enable) {
      if(m_muxer_mp4_config->file_duration >= 300) {
        WARN("Hls is enabled, the file duration is larger than 300s, set it 300s");
        m_muxer_mp4_config->file_duration = 300;
      }
    }
    m_start_file_writing = m_muxer_mp4_config->auto_file_writing;
    /*find the storage sdaX path*/
    m_stream_id = m_muxer_mp4_config->video_id;
    m_muxer_attr = m_muxer_mp4_config->muxer_attr;
    if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      m_thread_name = "Mp4MuxerNormal";
    } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
      m_thread_name = "Mp4MuxerEvent";
    } else {
      ERROR("muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AMMuxerMp4::~AMMuxerMp4()
{
  stop();
  delete m_config;
  delete m_packet_queue;
  delete m_sei_queue;
  AM_DESTROY(m_lock);
  AM_DESTROY(m_state_lock);
  AM_DESTROY(m_file_writing_lock);
  delete m_config_file;
  INFO("AMMp4Muxer object destroyed.");
}
/*interface of AMIMuxerCodec*/
AM_STATE AMMuxerMp4::start()
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE ret = AM_STATE_OK;
  do {
    bool need_break = false;
    switch (get_state()) {
      case AM_MUXER_CODEC_RUNNING: {
        NOTICE("The mp4 muxer is already running.");
        need_break = true;
      }break;
      case AM_MUXER_CODEC_ERROR: {
        NOTICE("Mp4 muxer state is error! Need to be re-created!");
        need_break = true;
      }break;
      default:
        break;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
    AM_DESTROY(m_thread);
    if (AM_LIKELY(!m_thread)) {
      m_thread = AMThread::create(m_thread_name.c_str(), thread_entry, this);
      if (AM_UNLIKELY(!m_thread)) {
        ERROR("Failed to create thread.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    while ((get_state() == AM_MUXER_CODEC_STOPPED)
        || (get_state() == AM_MUXER_CODEC_INIT)) {
      usleep(5000);
    }
    if(get_state() == AM_MUXER_CODEC_RUNNING) {
      if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
        NOTICE("start a normal mp4 file muxer success.");
      } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
        NOTICE("start an event mp4 file muxer success.");
      } else {
        ERROR("muxer attr error.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      ERROR("Failed to start mp4 muxer.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_MUXER_CODEC_STATE AMMuxerMp4::create_resource()
{
  char file_name[strlen(m_muxer_mp4_config->file_location.c_str())
        + strlen(m_muxer_mp4_config->file_name_prefix.c_str())
        + 128];
  memset(file_name, 0, sizeof(file_name));
  INFO("Begin to create resource.");
  AM_MUXER_CODEC_STATE ret = AM_MUXER_CODEC_RUNNING;
  do {
    reset_parameter();
    if (AM_UNLIKELY(set_media_sink(file_name) != AM_STATE_OK)) {
      ERROR("Mp4 muxer set media sink error, exit main loop.");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    AM_DESTROY(m_file_writer);
    if (AM_UNLIKELY((m_file_writer = AMMp4FileWriter::create (
        m_muxer_mp4_config, &m_video_info)) == NULL)) {
      ERROR("Failed to create m_file_writer!");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    AM_DESTROY(m_mp4_builder);
    if (AM_UNLIKELY((m_mp4_builder = AMMuxerMp4Builder::create(m_file_writer,
                                 m_muxer_mp4_config->file_duration,
                                 m_muxer_mp4_config->hls_enable))
                    == NULL)) {
      ERROR("Failde to create mp4 builder.");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name) != AM_STATE_OK)) {
      ERROR("Failed to set media sink for m_file_writer!");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if(m_muxer_mp4_config->auto_file_writing) {
      if(AM_UNLIKELY(m_file_writer->create_next_split_file() != AM_STATE_OK)) {
        ERROR("Failed to create mp4 file.");
        ret = AM_MUXER_CODEC_ERROR;
        break;
      }

    }
//    /*for restart the muxer*/
//    if ((m_av_info_map >> 1 & 0x01) == 0x01) {
//      if (AM_UNLIKELY(m_mp4_builder->set_video_info(&m_video_info)
//          != AM_STATE_OK)) {
//        ERROR("Failed to set video info to mp4 builder.");
//        ret = AM_MUXER_CODEC_ERROR;
//        break;
//      }
//      m_file_video_frame_count = (((m_splitted_duration * m_video_info.scale)
//          << 1) + m_video_info.rate * 90000)
//          / ((m_video_info.rate * 90000) << 1) * m_video_info.mul
//          / m_video_info.div;
//    }
//    if ((m_av_info_map & 0x01) == 0x01) {
//      if (AM_UNLIKELY(m_mp4_builder->set_audio_info(&m_audio_info)
//          != AM_STATE_OK)) {
//        ERROR("Failed to set audio info to mp4 file.");
//        ret = AM_MUXER_CODEC_ERROR;
//        break;
//      }
//    }
//    if (m_av_info_map == 3) {
//      if (AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
//        ERROR("Failed to begin file.");
//        ret = AM_MUXER_CODEC_ERROR;
//        break;
//      }
//    }
  } while (0);
  AUTO_SPIN_LOCK(m_state_lock);
  m_state = ret;
  return ret;
}

AM_STATE AMMuxerMp4::stop()
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE ret = AM_STATE_OK;
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("stop success.");
  return ret;
}

bool AMMuxerMp4::start_file_writing()
{
  bool ret = true;
  INFO("begin to start file writing in mp4 muxer.");
  do{
    if(m_start_file_writing) {
      NOTICE("file writing is already startted.");
      break;
    }
    if(AM_UNLIKELY(m_file_writer->create_next_split_file() != AM_STATE_OK)) {
      ERROR("Failed to create next split file.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
      ERROR("Failed to begin mp4 file.");
      ret = false;
      break;
    }
    m_start_file_writing = true;
    INFO("start file writing success in mp4 muxer.");
  }while(0);
  return ret;
}

bool AMMuxerMp4::stop_file_writing()
{
  bool ret = true;
  INFO("begin to stop file writing in mp4 muxer.");
  do{
    AUTO_SPIN_LOCK(m_file_writing_lock);
    if(!m_start_file_writing) {
      NOTICE("file writing is already stopped");
      break;
    }
    m_start_file_writing = false;
    if(AM_UNLIKELY(m_mp4_builder->end_file() != AM_STATE_OK)) {
      ERROR("Failed to end mp4 file.");
      ret =false;
      break;
    }
    if(AM_UNLIKELY(m_file_writer->on_eos() != AM_STATE_OK)) {
      ERROR("Failed to on eos for mp4 muxer.");
      ret = false;
      break;
    }
    m_last_video_pts = 0;
    m_is_first_video = true;
    m_is_first_audio = true;
    m_video_frame_count = 0;
    m_audio_enable = false;
    INFO("stop file writing success in mp4 muxer.");
  }while(0);
  return ret;
}

bool AMMuxerMp4::is_running()
{
  return m_run;
}

void AMMuxerMp4::release_resource()
{
  while (!(m_packet_queue->empty())) {
    m_packet_queue->front()->release();
    m_packet_queue->pop_front();
  }
  while (!(m_sei_queue->empty())) {
    m_sei_queue->front()->release();
    m_sei_queue->pop_front();
  }
  if(m_file_writer && m_mp4_builder) {
    if(m_file_writer->is_file_open()) {
      m_mp4_builder->end_file();
    }
  }
  AM_DESTROY(m_mp4_builder);
  AM_DESTROY(m_file_writer);
  INFO("release resource success.");
}

AM_MUXER_ATTR AMMuxerMp4::get_muxer_attr()
{
  return m_muxer_attr;
}

AM_MUXER_CODEC_STATE AMMuxerMp4::get_state()
{
  AUTO_SPIN_LOCK(m_state_lock);
  return m_state;
}

void AMMuxerMp4::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL) == 0) {
        /*just need normal packet*/
        break;
      }
      if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) ||
          (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO)) {
        /*info and eos packet*/
        if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) &&
            (packet->get_stream_id() == m_muxer_mp4_config->video_id)) {
          add = true;
          break;
        } else if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) &&
            ((AM_AUDIO_TYPE(packet->get_frame_type())
                == m_muxer_mp4_config->audio_type))) {
          add = true;
          break;
        } else {
          break;
        }
      } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
        /*audio and video data packet*/
        switch (packet->get_attr()) {
          case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
            /* todo: Only support H.264 now, add H.265 or Mjpeg support ? */
            AM_VIDEO_FRAME_TYPE vtype =
                AM_VIDEO_FRAME_TYPE(packet->get_frame_type());
            add = (packet->get_stream_id() == m_muxer_mp4_config->video_id)
                && ((vtype == AM_VIDEO_FRAME_TYPE_H264_IDR)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_I)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_P)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_B));
          } break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
            add = (AM_AUDIO_TYPE(packet->get_frame_type())
                == m_muxer_mp4_config->audio_type);
          } break;
          default:
            break;
        }
      } else {
        /*event packet, do not need it in normal file*/
        break;
      }
    } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
      //INFO("Event file receive packet.");
      if(packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        add = true;
      }
      if(packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
        /* todo: Only support H.264 now, add H.265 or Mjpeg support ? */
        if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)) {
          if(((AM_VIDEO_INFO*)(packet->get_data_ptr()))->type != AM_VIDEO_H264) {
            add = false;
            ERROR("Mp4 event recording Only support H.264.");
            break;
          } else {
            m_stream_id = packet->get_stream_id();
          }
        }
        if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO)) {
          if(((AM_AUDIO_INFO*)(packet->get_data_ptr()))->type != AM_AUDIO_AAC) {
            add = false;
            ERROR("Mp4 event recording only support aac.");
            break;
          }else {
            m_event_audio_type = AM_AUDIO_AAC;
          }
        }
      } else if(packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA){
        switch (packet->get_attr()) {
          case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
            /* todo: Only support H.264 now, add H.265 or Mjpeg support ? */
            AM_VIDEO_FRAME_TYPE vtype =
                AM_VIDEO_FRAME_TYPE(packet->get_frame_type());
            add = (packet->get_stream_id() == m_stream_id)
                && ((vtype == AM_VIDEO_FRAME_TYPE_H264_IDR)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_I)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_P)
                    || (vtype == AM_VIDEO_FRAME_TYPE_H264_B));
          } break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
            add =
                (AM_AUDIO_TYPE(packet->get_frame_type()) == m_event_audio_type);
          } break;
          default:
            break;
        }
      } else {
        add = false;
        break;
      }
    } else {
      ERROR("File type error!");
      break;
    }
  } while (0);
  if (AM_LIKELY(add)) {
    packet->add_ref();
    m_packet_queue->push_back(packet);
  }

  packet->release();
}

AM_STATE AMMuxerMp4::set_config(AMMuxerCodecConfig* config)
{
  AM_STATE ret = AM_STATE_OK;
  if (AM_LIKELY(config->type == AM_MUXER_CODEC_MP4)) {
    memcpy(&m_muxer_mp4_config, &config->data, config->size);
    if (AM_UNLIKELY(!(m_config->set_config(m_muxer_mp4_config)))) {
      ERROR("Failed to write config information into config file.");
      ret = AM_STATE_ERROR;
    }
  } else {
    ret = AM_STATE_ERROR;
    ERROR("Input config's type is not AM_MUXER_CODEC_MP4");
  }
  return ret;
}

AM_STATE AMMuxerMp4::get_config(AMMuxerCodecConfig* config)
{
  AM_STATE ret = AM_STATE_OK;
  config->type = AM_MUXER_CODEC_MP4;
  config->size = sizeof(AMMuxerCodecConfig);
  memcpy(&config->data, &m_muxer_mp4_config, sizeof(AMMuxerCodecConfig));
  return ret;
}

void AMMuxerMp4::thread_entry(void* p)
{
  ((AMMuxerMp4*) p)->main_loop();
}

void AMMuxerMp4::main_loop()
{
  bool is_ok = true;
  uint32_t statfs_number = 0;
  m_run = true;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  while(m_run && is_ok) {
    AMPacket* packet = NULL;
    if(m_packet_queue->empty()) {
      DEBUG("Packet queue is empty, sleep 100 ms.");
      usleep(100000);
      continue;
    }
    packet = m_packet_queue->front();
    m_packet_queue->pop_front();
    if(AM_UNLIKELY((!m_start_file_writing) &&
                   (packet->get_type() != AMPacket::AM_PAYLOAD_TYPE_INFO))) {
      packet->release();
      continue;
    }
    if (AM_LIKELY(is_ok)) {
      AUTO_SPIN_LOCK(m_file_writing_lock);
      switch (packet->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          if (AM_UNLIKELY(on_info_packet(packet) != AM_STATE_OK)) {
            ERROR("On av info error, exit the main loop.");
            AUTO_SPIN_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
            ++ error_count;
            WARN("On normal data packet error, error count is %u!", error_count);
            if(error_count == 5) {
              ERROR("On normal data packet error, exit the main loop");
              AUTO_SPIN_LOCK(m_state_lock);
              m_state = AM_MUXER_CODEC_ERROR;
              is_ok = false;
            }
            break;
          }
          error_count = 0;
          ++ statfs_number;
          if((m_av_info_map == 3) && (statfs_number >= 10)) {
            statfs_number = 0;
            uint64_t free_space = 0;
            struct statfs disk_statfs;
            if(statfs(m_file_location.c_str(),
                      &disk_statfs) < 0) {
              PERROR("statfs");
            } else {
              free_space = ((uint64_t)disk_statfs.f_bsize *
                  (uint64_t)disk_statfs.f_bfree) / (uint64_t)(1024 * 1024);
              DEBUG("free space is %llu M", free_space);
              if(AM_UNLIKELY(free_space <=
                             m_muxer_mp4_config->smallest_free_space)) {
                ERROR("The free space is smaller than %d M, "
                    "will stop writing data to mp4 file and close it ",
                    m_muxer_mp4_config->smallest_free_space);
                AUTO_SPIN_LOCK(m_state_lock);
                m_state = AM_MUXER_CODEC_ERROR;
                is_ok = false;
                break;
              }
            }
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          NOTICE("receive eos packet");
          if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
            ERROR("On eos packet error, exit the main loop.");
            AUTO_SPIN_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        } break;
        default: {
          WARN("Unknown packet type: %u!", packet->get_type());
        } break;
      }
    }
    packet->release();
  }
  release_resource();
  if (AM_UNLIKELY(!m_run)) {
    AUTO_SPIN_LOCK(m_state_lock);
    m_state = AM_MUXER_CODEC_STOPPED;
  }
  NOTICE("Mp4 muxer exit mainloop.");
}

void AMMuxerMp4::reset_parameter()
{
    m_stream_id = 0;
    m_audio_enable = false;
    m_is_first_audio = true;
    m_is_first_video = true;
    m_need_splitted = false;
    m_is_new_info = false;
    m_eos_map = 0;
    m_av_info_map = 0;
    m_event_audio_type = AM_AUDIO_NULL;
    m_splitted_duration = 0LLU;
    m_next_file_boundary = 0LLU;
    m_last_video_pts = 0;
    m_file_video_frame_count = 0;
    m_video_frame_count = 0;
}

bool AMMuxerMp4::get_current_time_string(char *time_str, int32_t len)
{
  time_t current = time(NULL);
  if (AM_UNLIKELY(strftime(time_str, len, "%Y%m%d%H%M%S", localtime(&current))
                  == 0)) {
    ERROR("Date string format error!");
    time_str[0] = '\0';
    return false;
  }

  return true;
}

AM_STATE AMMuxerMp4::set_media_sink(char* file_name)
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t config_index_number = 0;
  char config_index_char = '\0';
  int32_t index_type = -1;/*1 means number, 2 means alpha, 3 means sda*/
  uint64_t max_free_space = 0;
  uint64_t free_space = 0;
  uint32_t max_index_number = 0;
  int32_t max_index_type = -1;
  char   max_index_char = '\0';
  bool is_config_index = false;
  char* end_ptr = nullptr;
  std::string sd_index_indicator = "sda";
  std::ifstream file;
  std::string read_line;
  std::string find_str = "/storage/sda";
  size_t find_str_position = 0;
  char file_location[512] = { 0 };
  do {
    do {
      if(m_muxer_mp4_config->file_location_auto_parse) {
        find_str_position =
            m_muxer_mp4_config->file_location.find(sd_index_indicator);
        if(find_str_position != std::string::npos) {
          if(isdigit(
            m_muxer_mp4_config->file_location.c_str()[find_str_position + 3])) {
            config_index_number = strtol(
             &(m_muxer_mp4_config->file_location.c_str()[find_str_position + 3]),
                &end_ptr, 10);
            if(config_index_number >= 0 && end_ptr) {
              NOTICE("config index number is %u", config_index_number);
              index_type = 1;
            } else {
              ERROR("strtol config index number error.");
              ret = AM_STATE_ERROR;
              break;
            }
          } else if(isalpha(
            m_muxer_mp4_config->file_location.c_str()[find_str_position + 3])) {
            config_index_char =
                m_muxer_mp4_config->file_location.c_str()[find_str_position + 3];
            index_type = 2;
            NOTICE("config index char is %c", config_index_char);
          } else if(
            (m_muxer_mp4_config->file_location.c_str()[find_str_position + 3]
                                                      == '/') ||
            (m_muxer_mp4_config->file_location.c_str()[find_str_position + 3]
                                       == '\0')) {
            index_type = 3;
            NOTICE("file location is the path of /storage/sda/.");
          } else {
            ERROR("The file location is not correct in mp4 config file");
            ret = AM_STATE_ERROR;
            break;
          }
        } else {
          WARN("file location specified in config file is"
              " not in \"/storage/sdax directory\", can not auto parse.");
          memcpy(file_location, m_muxer_mp4_config->file_location.c_str(),
                 m_muxer_mp4_config->file_location.size());
          break;
        }
        file.open("/proc/self/mounts");
        INFO("mount information :");
        while(getline(file, read_line)) {
          uint32_t temp_index_number = 0;
          char temp_index_char = '\0';
          int32_t temp_index_type = -1;/*1 means number, 2 means alpha, 3 means sda*/
          INFO("%s", read_line.c_str());
          if((find_str_position = read_line.find(find_str))
              != std::string::npos) {
            if(isdigit(read_line.c_str()[find_str_position + 12])) {
              temp_index_number = strtol(
                  &(read_line.c_str()[find_str_position + 12]), &end_ptr, 10);
              if(temp_index_number >= 0 && end_ptr) {
                NOTICE("mount index is %u", temp_index_number);
                temp_index_type = 1;
              } else {
                NOTICE("strtol sd index error.");
                continue;
              }
            } else if(isalpha(read_line.c_str()[find_str_position + 12])) {
              temp_index_char = read_line.c_str()[find_str_position + 12];
              NOTICE("mount information index char is %c", temp_index_char);
              temp_index_type = 2;
            } else if((read_line.c_str()[find_str_position + 12] == '/') ||
                (read_line.c_str()[find_str_position + 12] == '\0') ||
                (read_line.c_str()[find_str_position + 12] == ' ')) {
              temp_index_type = 3;
              NOTICE("mount information index is sda.");
            } else {
              WARN("parse mount information error.");
              continue;
            }
            char temp_location[128];
            if(temp_index_type == 1) {
              snprintf(temp_location, 128, "/storage/sda%u", temp_index_number);
            } else if(temp_index_type == 2) {
              snprintf(temp_location, 128, "/storage/sda%c", temp_index_char);
            } else if(temp_index_type == 3) {
              snprintf(temp_location, 128, "/storage/sda");
            } else {
              WARN("parse mount information error");
              continue;
            }
            if((temp_index_type == index_type)) {
              if(((temp_index_type == 1) &&
                  (config_index_number == temp_index_number)) ||
                  ((temp_index_type == 2) &&
                      (config_index_char == temp_index_char)) ||
                  (temp_index_type == 3)) {
                is_config_index = true;
                break;
              }
            }
            free_space = 0;
            struct statfs disk_statfs;
            if(statfs(temp_location, &disk_statfs) < 0) {
              PERROR("statfs");
            } else {
              free_space = ((uint64_t)disk_statfs.f_bsize *
                  (uint64_t)disk_statfs.f_bfree) / (uint64_t)(1024 * 1024);
              DEBUG("free space is %llu M", free_space);
              if(free_space > max_free_space) {
                max_free_space = free_space;
                max_index_type = temp_index_type;
                if(temp_index_type == 1) {
                  max_index_number = temp_index_number;
                  NOTICE("max sda index is %u", max_index_number);
                } else if(temp_index_type == 2) {
                  max_index_char = temp_index_char;
                  NOTICE("max sda index is %c", max_index_char);
                } else if(temp_index_type == 3) {
                  DEBUG("location is sda");
                } else {
                  ERROR("parse mount information error.");
                  ret = AM_STATE_ERROR;
                  break;
                }
              }
            }
          }
        }
        file.close();
        if(is_config_index) {
          INFO("The mp4 file location is same with the place which is writen "
              "in config file");
          memcpy(file_location, m_muxer_mp4_config->file_location.c_str(),
                 m_muxer_mp4_config->file_location.size());
        } else {
          if(max_free_space > 20) {
            if(max_index_type == 1) {
              snprintf(file_location, 512, "/storage/sda%u/video",
                       max_index_number);
              WARN("The file location specified in the config file is not find, "
                  "set %s as default", file_location);
            } else if(max_index_type == 2) {
              snprintf(file_location, 512, "/storage/sda%c/video",
                       max_index_char);
              WARN("The file location specified in the config file is not find, "
                  "set %s as default", file_location);
            } else if(max_index_type == 3) {
              snprintf(file_location, 512, "/storage/sda/video");
              WARN("The file location specified in the config file is not find, "
                  "set %s as default", file_location);
            } else {
              ERROR("parse mount information error.");
              ret = AM_STATE_ERROR;
              break;
            }
          } else if(max_free_space > 0){
            ERROR("The free space of file location directory is less ."
                "than 20M bytes.");
            ret = AM_STATE_ERROR;
            break;
          } else {
            ERROR("Can not find proper file location.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      } else {
        memcpy(file_location, m_muxer_mp4_config->file_location.c_str(),
               m_muxer_mp4_config->file_location.size());
      }
    } while(0);
    if(ret != AM_STATE_OK) {
      break;
    }
    m_file_location = file_location;
    if(m_muxer_mp4_config->hls_enable) {
      if(AM_UNLIKELY((symlink(file_location, "/webSvr/web/media/media_file")
          < 0) && (errno != EEXIST))) {
        PERROR("Failed to symlink hls media location.");
      }
    }
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string,
                                              sizeof(time_string))))) {
      ERROR("Get current time string error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      sprintf(file_name,
              "%s/%s_%s_stream%u",
              file_location,
              m_muxer_mp4_config->file_name_prefix.c_str(),
              time_string,
              m_stream_id);
    } else if(m_muxer_attr == AM_MUXER_FILE_EVENT) {
      sprintf(file_name,
              "%s/%s_event_%s",
              file_location,
              m_muxer_mp4_config->file_name_prefix.c_str(),
              time_string);
    } else {
      ERROR("File type error.");
      ret = AM_STATE_ERROR;
      break;
    }
    INFO("set media sink success, file name = %s", file_name);
  } while (0);
  if(ret != AM_STATE_OK) {
    ERROR("set media sink eror.");
  }
  return ret;
}

AM_STATE AMMuxerMp4::set_split_duration(uint64_t split_duration)
{
  INFO("set split duration = %d", split_duration);
  do {
    if (split_duration == 0) {
      m_need_splitted = false;
      INFO("Do not need splitted!");
      break;
    }

    m_need_splitted = true;
    m_splitted_duration = split_duration;
    //m_next_file_boundary = split_duration;
  } while (0);

  return AM_STATE_OK;
}

AM_STATE AMMuxerMp4::on_info_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /*video info*/
      AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*) (packet->get_data_ptr());
      INFO("Mp4 Muxer receive H264 INFO: "
          "size %dx%d, M %d, N %d, rate %d, scale %d, fps %d\n",
          video_info->width, video_info->height, video_info->M,
          video_info->N, video_info->rate, video_info->scale, video_info->fps);
      if((video_info->width == 0) || (video_info->height == 0) ||
          (video_info->scale == 0) || (video_info->div == 0)) {
        WARN("video info packet error, drop it.");
        break;
      }
      if (((m_av_info_map >> 1) & 0x01) == 0x01) {
        /*video info is set already, check new video info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the mp4 builder, write media data to new file*/
        if ((video_info->M == m_video_info.M)
            && (video_info->N == m_video_info.N)
            && (video_info->stream_id == m_video_info.stream_id)
            && (video_info->fps == m_video_info.fps)
            && (video_info->rate == m_video_info.rate)
            && (video_info->scale == m_video_info.scale)
            && (video_info->type == m_video_info.type)
            && (video_info->mul == m_video_info.mul)
            && (video_info->div == m_video_info.div)
            && (video_info->width == m_video_info.width)
            && (video_info->height == m_video_info.height)) {
          NOTICE("Video info is not changed. Discard it.");
          break;
        }
        NOTICE("video info changed, close current file, create new file.");
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_mp4_builder->set_video_info(video_info)
            != AM_STATE_OK)) {
          ERROR("Failed to set video info to mp4 builder.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_file_video_frame_count = (((m_splitted_duration * video_info->scale)
            << 1) + video_info->rate * 90000)
            / ((video_info->rate * 90000) << 1) * video_info->mul
            / video_info->div;
        m_is_new_info = true;
      } else {
        /*first video info packet*/
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_mp4_builder->set_video_info(video_info)
            != AM_STATE_OK)) {
          ERROR("Failed to set video info to mp4 builder.");
          ret = AM_STATE_ERROR;
          break;
        }
        set_split_duration(m_muxer_mp4_config->file_duration
            * (270000000ULL
                / ((uint64_t) (100ULL * video_info->scale / video_info->rate))));
        m_file_video_frame_count = (((m_splitted_duration * video_info->scale)
            << 1) + video_info->rate * 90000)
            / ((video_info->rate * 90000) << 1) * video_info->mul
            / video_info->div;
        m_av_info_map |= 1 << 1;
        if ((m_av_info_map == 3) && m_start_file_writing) {
          NOTICE("Mp4 muxer has received both audio and video info.");
          if (AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
            ERROR("Failed to begin file");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      }
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /*audio_info*/
      AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO*) (packet->get_data_ptr());
      INFO("Mp4 Muxer receive Audio INFO: freq %d, chuck %d, channel %d, "
          "pkt_pts_increment %u\n",
          audio_info->sample_rate,
          audio_info->chunk_size,
          audio_info->channels,
          audio_info->pkt_pts_increment);
      if ((m_av_info_map & 0x01) == 0x01) {
        /*video info is set already, check new video info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the mp4 builder, write media data to new file*/
        if ((audio_info->sample_rate == m_audio_info.sample_rate)
            && (audio_info->channels == m_audio_info.channels)
            && (audio_info->pkt_pts_increment == m_audio_info.pkt_pts_increment)
            && (audio_info->sample_size == m_audio_info.sample_size)
            && (audio_info->chunk_size == m_audio_info.chunk_size)
            && (audio_info->sample_format == m_audio_info.sample_format)
            && (audio_info->type == m_audio_info.type)) {
          NOTICE("Audio info is not changed.");
          break;
        }
        if (audio_info->type != AM_AUDIO_AAC) {
          ERROR("Currently, Mp4 muxer only support aac audio.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(m_mp4_builder->set_audio_info(audio_info)
            != AM_STATE_OK)) {
          ERROR("Failed to set audio info to mp4 builder.");
          ret = AM_STATE_ERROR;
          break;
        }
        memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
        m_is_new_info = true;
      } else {
        /*first audio info packet*/
        memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
        m_mp4_builder->set_audio_info(audio_info);
        if (audio_info->type != AM_AUDIO_AAC) {
          ERROR("Currently, Mp4 muxer only support aac audio.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 0;
        if ((m_av_info_map == 3) && m_start_file_writing) {
          NOTICE("Mp4 muxer has received both audio and video info.");
          if (AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
            ERROR("Failed to begin file");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      }
    } else {
      NOTICE("Currently, mp4 muxer only support audio and video stream.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4::on_data_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;

  do{
    if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      AMPacket *sei = NULL;
      /*video data*/
      if((m_av_info_map >> 1 & 0x01) != 0x01) {
        INFO("Video info packet has not been received, drop this data packet!");
        break;
      }
      if (AM_UNLIKELY(m_last_video_pts == 0)) {
        m_next_file_boundary = packet->get_pts() + m_splitted_duration;
      }
      if(AM_UNLIKELY(m_is_first_video)) {
        /*first frame must be video frame,
         * and the first video frame must be I frame*/
        m_file_writer->set_begin_packet_pts(packet->get_pts());
        if (AM_UNLIKELY(!((packet->get_frame_type()
            == AM_VIDEO_FRAME_TYPE_H264_IDR) && m_is_first_audio))) {
          INFO("First frame must be video IDR frame, drop this packet");
          break;
        }
        m_is_first_video = false;
        m_audio_enable = true;
      } else {
        m_file_writer->set_end_packet_pts(packet->get_pts());
      }
      if (AM_UNLIKELY((packet->get_frame_type() ==
                       AM_VIDEO_FRAME_TYPE_H264_IDR) &&
                       (m_is_new_info ||
                        (m_need_splitted && (((m_video_frame_count >=
                                               m_file_video_frame_count) ||
                                               (packet->get_pts() >=
                                                m_next_file_boundary))))))) {
        m_is_new_info = false;
        /* if (m_is_new_info == true)
         *  this is a new file, should create a new file
         * if (m_need_splitted && reached file_boundary)
         *  close the last file and create a new file
         */
        if (AM_UNLIKELY(on_eof(packet) != AM_STATE_OK)) {
          ERROR("On eof error.");
          ret = AM_STATE_ERROR;
        } else {
          /* This packet should be written into next file,
           * push it back into the front of the queue
           */
          packet->add_ref();
          m_packet_queue->push_front(packet);
        }
        break;
      }
      if (AM_LIKELY(!m_sei_queue->empty())) {
        sei = m_sei_queue->front();
        m_sei_queue->pop_front();
      }
      if(AM_UNLIKELY((ret = m_mp4_builder->write_video_data(packet, sei)) !=
          AM_STATE_OK)) {
        ERROR("Failed to write video data.");
      }
      if(AM_UNLIKELY(sei)) {
        sei->release();
      }
      if (AM_UNLIKELY(ret != AM_STATE_OK)) {
        break;
      }
      ++ m_video_frame_count;
      m_last_video_pts = packet->get_pts();
    } else if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      /*audio data*/
      if(AM_UNLIKELY(!m_audio_enable)) {
        INFO("Video frame has not been received, drop current audio packet");
        break;
      }
      if(AM_UNLIKELY(m_mp4_builder->write_audio_data(packet->get_data_ptr(),
                                                    packet->get_data_size(),
                                                    packet->get_frame_count())
                                                       != AM_STATE_OK)) {
        ERROR("Failed to write audio data to file.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_is_first_audio = false;
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_SEI) {
      packet->add_ref();
      m_sei_queue->push_back(packet);
      INFO("Get usr sei data");
    } else {
      NOTICE("Currently, mp4 muxer just support audio and video stream.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_muxer_attr == AM_MUXER_FILE_EVENT) &&
        ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_STOP) != 0))) {
      m_mp4_builder->end_file();
      m_file_writer->on_eos();
      m_run = false;
      NOTICE("Receive event stop packet, stop writing mp4 event file.");
    }
  }while(0);

  return ret;
}

AM_STATE AMMuxerMp4::on_eof(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do{
    if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      m_mp4_builder->end_file();
      m_file_writer->on_eof();
      m_mp4_builder->begin_file();
      m_next_file_boundary = m_last_video_pts + m_splitted_duration;
      m_is_first_video = true;
      m_is_first_audio = true;
      m_video_frame_count = 0;
      m_audio_enable = false;
    }else {
      ERROR("Currently, just receive video eof.");
      ret = AM_STATE_ERROR;
      break;
    }
  }while(0);
  return ret;
}

AM_STATE AMMuxerMp4::on_eos_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do{
    if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      NOTICE("audio eos is received.");
      m_eos_map |= 1 << 0;
    } else if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      NOTICE("video eos is received");
      m_eos_map |= 1 << 1;
    } else {
      ERROR("Currently, mp4 muxer just support audio and video stream.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_eos_map == 0x03) {
      m_mp4_builder->end_file();
      m_file_writer->on_eos();
      m_run = false;
      NOTICE("receive both audio and video eos, exit the main loop");
    }
  }while(0);
  return ret;
}
