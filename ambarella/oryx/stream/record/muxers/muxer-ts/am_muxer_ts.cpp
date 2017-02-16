/*
 * AM_MUXER_CODEC_TS.cpp
 *
 * 11/09/2012 [Hanbo Xiao] [Created]
 * 17/12/2014 [Chengcai Jing] [Modified]
 *
 * Copyright (C) 2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
#include "am_file_sink_if.h"
#include "am_muxer_ts_builder.h"
#include "am_muxer_ts_file_writer.h"
#include "am_muxer_ts.h"
#include "am_audio_define.h"
#include "am_video_types.h"

#include <time.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <iostream>
#include <fstream>

AMIMuxerCodec* get_muxer_codec(const char* config)
{
  return AMTsMuxer::create(config);
}

AMIMuxerCodec *AMTsMuxer::create(const char* config_file)
{
  AMTsMuxer *result = new AMTsMuxer();
  if (result != NULL && result->init(config_file) != AM_STATE_OK) {
    delete result;
    result = NULL;
  }

  return ((AMIMuxerCodec*) result);
}

/*interface of AMIMuxerCodec*/
AM_STATE AMTsMuxer::start()
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE ret = AM_STATE_OK;
  do {
    bool need_break = false;
    switch(get_state()) {
      case AM_MUXER_CODEC_RUNNING: {
        NOTICE("The muxer is already running!");
        need_break = true;
      }break;
      case AM_MUXER_CODEC_ERROR: {
        ERROR("Muxer state is AM_MUXER_CODEC_ERROR! Need to be re-created!");
        need_break = true;
      }break;
      default:break;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
    AM_DESTROY(m_thread);
    if (AM_LIKELY(!m_thread)) {
      m_thread = AMThread::create(m_thread_name.c_str(), thread_entry, this);
      if (AM_UNLIKELY(!m_thread)) {
        ERROR("Failed to create m_thread.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    while ((m_state == AM_MUXER_CODEC_STOPPED) ||
           (m_state == AM_MUXER_CODEC_INIT)) {
      usleep(1000);
    }
    if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      NOTICE("start a normal ts file muxer.");
    } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
      NOTICE("start an event ts file muxer.");
    } else {
      ERROR("muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxer::stop()
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE ret = AM_STATE_OK;
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("stop  ts muxer successfully.");
  return ret;
}

bool AMTsMuxer::start_file_writing()
{
  bool ret = true;
  do{
    if(m_start_file_writing) {
      NOTICE("file writing is already startted.");
      break;
    }
    if(AM_UNLIKELY(m_file_writer->on_eof(VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer on video stream eof error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->on_eof(AUDIO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer on audio stream eof error");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(build_pat_pmt() != AM_STATE_OK)) {
      ERROR("Build pat pmt error in ts muxer.");
      ret = false;
      break;
    }
    m_start_file_writing = true;
  }while(0);
  return ret;
}

bool AMTsMuxer::stop_file_writing()
{
  bool ret = true;
  do{
    AUTO_SPIN_LOCK(m_file_writing_lock);
    if(!m_start_file_writing) {
      NOTICE("file writing is already stopped");
      break;
    }
    m_start_file_writing = false;
    m_file_writer->close_file();
    m_last_video_pts = 0;
    m_is_first_audio = true;
    m_is_first_video = true;
    m_audio_enable = false;
    m_pcr_base = (90000 * m_h264_info.rate * m_h264_info.div) /
        (m_h264_info.scale * m_h264_info.mul);
    m_pcr_ext = 0;
    m_video_frame_count = 0;
    m_pts_base_video = 0LL;
    m_pts_base_audio = 0LL;
  }while(0);
  return ret;
}

bool AMTsMuxer::is_running()
{
  return m_run;
}

void AMTsMuxer::reset_parameter()
{
  m_stream_id = 0;
  m_audio_enable = false;
  m_is_first_audio = true;
  m_is_first_video = true;
  m_need_splitted = false;
  m_event_flag = false;
  m_event_normal_sync = true;
  m_av_info_map = 0;
  m_is_new_info = false;
  m_eos_map = 0;
  m_splitted_duration = 0LLU;
  m_next_file_boundary = 0LLU;
  m_pts_base_video = 0LL;
  m_pts_base_audio = 0LL;
  m_pcr_base = 0;
  m_pcr_ext = 0;
  m_pcr_inc_base = 0;
  m_pcr_inc_ext = 0;
  m_audio_chunk_buf_wr_ptr = NULL;
  m_audio_chunk_buf_avail = AUDIO_CHUNK_BUF_SIZE;
  m_last_video_pts = 0;
  m_file_video_frame_count = 0;
  m_video_frame_count = 0;
  m_event_audio_type = AM_AUDIO_NULL;
}

AM_MUXER_ATTR AMTsMuxer::get_muxer_attr()
{
  return m_muxer_attr;
}

AM_MUXER_CODEC_STATE AMTsMuxer::get_state()
{
  AUTO_SPIN_LOCK(m_state_lock);
  return m_state;
}

void AMTsMuxer::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      if((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL) == 0) {
        /*just need normal packet*/
        break;
      }
      if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) ||
          (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO)) {
        /*info and eos packet*/
        if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) &&
            (packet->get_stream_id() == m_muxer_ts_config->video_id)) {
          add = true;
          break;
        } else if((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) &&
                  (AM_AUDIO_TYPE(packet->get_frame_type()) ==
                   m_muxer_ts_config->audio_type)) {
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
            add = (packet->get_stream_id() == m_muxer_ts_config->video_id) &&
                  ((vtype == AM_VIDEO_FRAME_TYPE_H264_IDR) ||
                   (vtype == AM_VIDEO_FRAME_TYPE_H264_I)   ||
                   (vtype == AM_VIDEO_FRAME_TYPE_H264_P)   ||
                   (vtype == AM_VIDEO_FRAME_TYPE_H264_B));
          } break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
            add = (AM_AUDIO_TYPE(packet->get_frame_type()) ==
                   m_muxer_ts_config->audio_type);
          } break;
          default:
            break;
        }
      } else {
        /*event packet, do not need it in normal file*/
        break;
      }
    } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
      if (packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT) {
        add = true;
      }
      if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
        /* todo: Only support H.264 now, add H.265 or Mjpeg support ? */
        if ((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)) {
          if (((AM_VIDEO_INFO*) (packet->get_data_ptr()))->type
              != AM_VIDEO_H264) {
            add = false;
            ERROR("Mp4 event recording Only support H.264.");
            break;
          } else {
            m_stream_id = packet->get_stream_id();
          }
        }
        if ((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO)) {
          if (((AM_AUDIO_INFO*) (packet->get_data_ptr()))->type
              != AM_AUDIO_AAC) {
            add = false;
            ERROR("Mp4 event recording only support aac.");
            break;
          } else {
            m_event_audio_type = AM_AUDIO_AAC;
          }
        }
      } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
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
          }
            break;
          case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
            add =
                (AM_AUDIO_TYPE(packet->get_frame_type()) == m_event_audio_type);
          }
            break;
          default:
            break;
        }
      } else {
        add = false;
        break;
      }
    } else {
      ERROR("File type error! Stop ts muxer.");
      break;
    }
  } while (0);
  if (AM_LIKELY(add)) {
    m_packet_queue->push_back(packet);
  } else {
    packet->release();
  }
}

AM_STATE AMTsMuxer::set_config(AMMuxerCodecConfig *config)
{
  AM_STATE ret = AM_STATE_OK;
  if (AM_LIKELY(config->type == AM_MUXER_CODEC_TS)) {
    memcpy(&m_muxer_ts_config, &config->data, config->size);
    if (AM_UNLIKELY(!(m_config->set_config(m_muxer_ts_config)))) {
      ERROR("Failed to write config information into config file.");
      ret = AM_STATE_ERROR;
    }
  } else {
    ret = AM_STATE_ERROR;
    ERROR("Input config's type is not AM_MUXER_CODEC_TS");
  }
  return ret;
}

AM_STATE AMTsMuxer::get_config(AMMuxerCodecConfig *config)
{
  AM_STATE ret = AM_STATE_OK;
  config->type = AM_MUXER_CODEC_TS;
  config->size = sizeof(AMMuxerCodecConfig);
  memcpy(&config->data, &m_muxer_ts_config, sizeof(AMMuxerCodecConfig));
  return ret;
}

void AMTsMuxer::thread_entry(void* p)
{
  ((AMTsMuxer*) p)->main_loop();
}

void AMTsMuxer::main_loop()
{
  bool is_ok = true;
  m_run = true;
  uint32_t statfs_number = 0;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());

  while (m_run && is_ok) {
    AMPacket *packet = NULL;
    if (m_packet_queue->empty()) {
      DEBUG("packet queue is empty, sleep 50 ms.");
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
            ERROR("On AV info error, exit the main loop.");
            AUTO_SPIN_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        }break;
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
            ++ error_count;
            WARN("On normal data packet error, error count is %u!", error_count);
            if(error_count == 5) {
              ERROR("On normal data packet error, error count is 5, exit mainloop");
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
            if (statfs(m_file_location.c_str(), &disk_statfs)
                < 0) {
              NOTICE("statfs error.");
            } else {
              free_space = ((uint64_t) disk_statfs.f_bsize
                  * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
              DEBUG("free space is %llu M", free_space);
              if (AM_UNLIKELY(free_space <=
                              m_muxer_ts_config->smallest_free_space)) {
                ERROR("The free space is smaller than %d M, "
                    "will stop writing data to ts file and close it ",
                    m_muxer_ts_config->smallest_free_space);
                AUTO_SPIN_LOCK(m_state_lock);
                m_state = AM_MUXER_CODEC_ERROR;
                is_ok = false;
                break;
              }
            }
          }
        }break;
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
            ERROR("On EOS error, exit the main loop!");
            AUTO_SPIN_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        }break;
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
  NOTICE("TS Muxer exit mainloop");
}

AM_MUXER_CODEC_STATE AMTsMuxer::create_resource()
{
  char file_name[strlen(m_muxer_ts_config->file_location.c_str())
        + strlen(m_muxer_ts_config->file_name_prefix.c_str())
        + 128];
  memset(file_name, 0, sizeof(file_name));
  AUTO_SPIN_LOCK(m_state_lock);
  AM_MUXER_CODEC_STATE ret = AM_MUXER_CODEC_RUNNING;
  do {
    if (m_state == AM_MUXER_CODEC_STOPPED) {
      reset_parameter();
    }
    if (AM_UNLIKELY((m_ts_builder = new AMTsBuilder ()) == NULL)) {
      ERROR("Failed to create ts builder.\n");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_file_writer = AMTsFileWriter::create (
        m_muxer_ts_config, &m_h264_info)) == NULL)) {
      ERROR("Failed to create m_file_writer!");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(set_media_sink(file_name) != AM_STATE_OK)) {
      ERROR("Ts muxer set media sink error, Exit main loop.");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name) != AM_STATE_OK)) {
      ERROR("Failed to set media sink for m_file_writer!");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if(m_start_file_writing) {
      if(AM_UNLIKELY(m_file_writer->create_next_split_file() != AM_STATE_OK)) {
        ERROR("Failed to create next split file.");
        ret = AM_MUXER_CODEC_ERROR;
        break;
      }
    }
    init_ts();
    /*for restart ts muxer*/
    /*if ((m_av_info_map >> 1 & 0x01) == 0x01) {
      if (AM_UNLIKELY(set_video_info(&m_h264_info) != AM_STATE_OK)) {
        ret = AM_MUXER_CODEC_ERROR;
        ERROR("Failed to set video info.");
        break;
      }
    }
    if ((m_av_info_map & 0x01) == 0x01) {
      if (AM_UNLIKELY(set_audio_info(&m_audio_info) != AM_STATE_OK)) {
        ERROR("Failed to set audio info.");
        ret = AM_MUXER_CODEC_ERROR;
        break;
      }
    }
    if (m_av_info_map == 3) {
      if (AM_UNLIKELY(build_pat_pmt() != AM_STATE_OK)) {
        ERROR("Build pat pmt error.");
        ret = AM_MUXER_CODEC_ERROR;
        break;
      }
    }
    */
    m_audio_chunk_buf =  new uint8_t[AUDIO_CHUNK_BUF_SIZE];
    if (AM_UNLIKELY(m_audio_chunk_buf == NULL)) {
      ERROR("Failed to allocate memory for audio chunk.\n");
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    m_audio_chunk_buf_wr_ptr = m_audio_chunk_buf;
    m_state = ret;
  }while(0);

  return ret;
}

void AMTsMuxer::release_resource()
{
  INFO("begin to release resource.");
  while (!(m_packet_queue->empty())) {
    m_packet_queue->front()->release();
    m_packet_queue->pop_front();
  }
  if ((m_eos_map & 0x01) == 0) {
    if (AM_UNLIKELY(m_file_writer->on_eos(AUDIO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer on audio stream eos error.");
    }
  }
  if (((m_eos_map >> 1) & 0x01) == 0) {
    if (AM_UNLIKELY(m_file_writer->on_eos(VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer on video stream eos error.");
    }
  }
  AM_DESTROY(m_ts_builder);
  AM_DESTROY(m_file_writer);

  delete[] m_audio_chunk_buf;
  m_audio_chunk_buf = NULL;
  m_audio_chunk_buf_wr_ptr = NULL;
  INFO("Release resource successfully");
}

void AMTsMuxer::destroy()
{
  delete this;
}

AMTsMuxer::AMTsMuxer() :
    m_thread(NULL),
    m_lock(NULL),
    m_state_lock(NULL),
    m_file_writing_lock(NULL),
    m_muxer_ts_config(NULL),
    m_config(NULL),
    m_packet_queue(NULL),
    m_ts_builder(NULL),
    m_file_writer(NULL),
    m_last_video_pts(0),
    m_pcr_base(0),
    m_pcr_inc_base(0),
    m_splitted_duration(0LLU),
    m_next_file_boundary(0LLU),
    m_pts_base_video(0LL),
    m_pts_base_audio(0LL),
    m_av_info_map(0),
    m_eos_map(0),
    m_audio_chunk_buf_avail(AUDIO_CHUNK_BUF_SIZE),
    m_file_video_frame_count(0),
    m_video_frame_count(0),
    m_stream_id(0),
    m_pcr_ext(0),
    m_pcr_inc_ext(0),
    m_audio_chunk_buf(NULL),
    m_audio_chunk_buf_wr_ptr(NULL),
    m_config_file(NULL),
    m_event_audio_type(AM_AUDIO_NULL),
    m_muxer_attr(AM_MUXER_FILE_NORMAL),
    m_state(AM_MUXER_CODEC_INIT),
    m_run(false),
    m_audio_enable(false),
    m_is_first_audio(true),
    m_is_first_video(true),
    m_need_splitted(false),
    m_event_flag(false),
    m_event_normal_sync(true),
    m_is_new_info(false),
    m_start_file_writing(true)
{
}

AM_STATE AMTsMuxer::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(config_file == NULL)) {
      ERROR("Config_file is NULL, should input valid config_file.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config_file = amstrdup(config_file);
    if (AM_UNLIKELY((m_packet_queue = new packet_queue()) == NULL)) {
      ERROR("Failed to create packet_queue.");
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
    if (AM_UNLIKELY((m_config = new AMMuxerTsConfig()) == NULL)) {
      ERROR("Failed to create AMMuxerTsConfig.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_muxer_ts_config = m_config->get_config(std::string(m_config_file));
    if (AM_UNLIKELY(!m_muxer_ts_config)) {
      ERROR("Failed to get config.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_muxer_ts_config->hls_enable) {
      if(m_muxer_ts_config->file_duration >= 300) {
        WARN("Hls is enabled, the file duration is larger than 300s, set it 300s");
        m_muxer_ts_config->file_duration = 300;
      }
    }
    m_stream_id = m_muxer_ts_config->video_id;
    m_muxer_attr = m_muxer_ts_config->muxer_attr;
    m_start_file_writing = m_muxer_ts_config->auto_file_writing;
    if (m_muxer_attr == AM_MUXER_FILE_NORMAL) {
      m_thread_name = "TSMuxerNormal";
    } else if (m_muxer_attr == AM_MUXER_FILE_EVENT) {
      m_thread_name = "TSMuxerEvent";
    } else {
      ERROR("muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);

  return ret;
}

AMTsMuxer::~AMTsMuxer()
{
  stop();
  delete m_packet_queue;
  AM_DESTROY(m_lock);
  AM_DESTROY(m_state_lock);
  AM_DESTROY(m_file_writing_lock);
  delete m_config_file;
  delete m_config;
  DEBUG("!AMTsMuxer");
}

AM_STATE AMTsMuxer::set_media_sink(char* file_name)
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
        if(m_muxer_ts_config->file_location_auto_parse) {
          find_str_position =
              m_muxer_ts_config->file_location.find(sd_index_indicator);
          if(find_str_position != std::string::npos) {
            if(isdigit(
              m_muxer_ts_config->file_location.c_str()[find_str_position + 3])) {
              config_index_number = strtol(
               &(m_muxer_ts_config->file_location.c_str()[find_str_position + 3]),
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
              m_muxer_ts_config->file_location.c_str()[find_str_position + 3])) {
              config_index_char =
                  m_muxer_ts_config->file_location.c_str()[find_str_position + 3];
              index_type = 2;
              NOTICE("config index char is %c", config_index_char);
            } else if(
              (m_muxer_ts_config->file_location.c_str()[find_str_position + 3]
                                                        == '/') ||
              (m_muxer_ts_config->file_location.c_str()[find_str_position + 3]
                                         == '\0')) {
              index_type = 3;
              NOTICE("file location is the path of /storage/sda/.");
            } else {
              ERROR("The file location is not correct in ts config file");
              ret = AM_STATE_ERROR;
              break;
            }
          } else {
            WARN("file location specified in config file is"
                " not in \"/storage/sdax directory\", can not auto parse.");
            memcpy(file_location, m_muxer_ts_config->file_location.c_str(),
                   m_muxer_ts_config->file_location.size());
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
            INFO("The ts file location is same with the place which is writen "
                "in config file");
            memcpy(file_location, m_muxer_ts_config->file_location.c_str(),
                   m_muxer_ts_config->file_location.size());
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
          memcpy(file_location, m_muxer_ts_config->file_location.c_str(),
                 m_muxer_ts_config->file_location.size());
        }
      } while(0);
      if(ret != AM_STATE_OK) {
        break;
      }
      m_file_location = file_location;
      if(m_muxer_ts_config->hls_enable) {
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
                m_muxer_ts_config->file_name_prefix.c_str(),
                time_string,
                m_stream_id);
      } else if(m_muxer_attr == AM_MUXER_FILE_EVENT) {
        sprintf(file_name,
                "%s/%s_event_%s",
                file_location,
                m_muxer_ts_config->file_name_prefix.c_str(),
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

bool AMTsMuxer::get_current_time_string(char *time_str, int32_t len)
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

AM_STATE AMTsMuxer::set_split_duration(uint64_t duration_in90k_base)
{
  do {
    if (duration_in90k_base == 0) {
      m_need_splitted = false;
      INFO("Do not need splitted!");
      break;
    } else {
      INFO("File duration is set to %llu", duration_in90k_base);
    }

    m_need_splitted = true;
    m_splitted_duration = duration_in90k_base;
  } while (0);

  return AM_STATE_OK;
}

inline AM_STATE AMTsMuxer::set_audio_info(AM_AUDIO_INFO* audio_info)
{
  AM_STATE ret = AM_STATE_OK;

  m_ts_builder->set_audio_info(audio_info);
  switch(audio_info->type) {
    case  AM_AUDIO_AAC: {
      m_audio_stream_info.type = MPEG_SI_STREAM_TYPE_AAC;
      m_audio_stream_info.descriptor_tag = 0x52;
      m_audio_stream_info.descriptor_len = 0;
      m_audio_stream_info.descriptor = NULL;
    }break;
    case AM_AUDIO_BPCM: {
      uint8_t channel_idx =
          (audio_info->channels == 1) ? 1 :
              ((audio_info->channels == 2) ? 3 : 6);
      uint8_t sample_freq_idx =
          (audio_info->sample_rate == 48000) ?
              1 : ((audio_info->sample_rate == 96000) ? 4 : 5);
      m_audio_stream_info.type = MPEG_SI_STREAM_TYPE_LPCM_AUDIO;
      m_audio_stream_info.descriptor_len = 8;
      m_audio_stream_info.descriptor = m_lpcm_descriptor;
      m_audio_stream_info.descriptor[0] = 'H'; // 0x48
      m_audio_stream_info.descriptor[1] = 'D'; // 0x44
      m_audio_stream_info.descriptor[2] = 'M'; // 0x4d
      m_audio_stream_info.descriptor[3] = 'V'; // 0x56
      m_audio_stream_info.descriptor[4] = 0x00;
      m_audio_stream_info.descriptor[5] = MPEG_SI_STREAM_TYPE_LPCM_AUDIO;
      m_audio_stream_info.descriptor[6] = (channel_idx << 4)
                  | (sample_freq_idx & 0xF);
      m_audio_stream_info.descriptor[7] = (1 << 6) | 0x3F;
    }break;
    /* todo: Add other Audio type support */
    default: {
      ERROR("Currently, we just support aac and bpcm audio.\n");
      ret = AM_STATE_ERROR;
    }break;
  }

  return ret;
}

inline AM_STATE AMTsMuxer::set_video_info(AM_VIDEO_INFO* video_info)
{
  m_ts_builder->set_video_info(video_info);
  pcr_calc_pkt_duration(video_info->rate, video_info->scale);
  m_pcr_base = (90000 * m_h264_info.rate * m_h264_info.div) /
               (m_h264_info.scale * m_h264_info.mul);
  m_pcr_ext = 0;
  set_split_duration(m_muxer_ts_config->file_duration *
                     (270000000ULL / ((uint64_t)
                         (100ULL * video_info->scale / video_info->rate))));
  m_file_video_frame_count = (((m_splitted_duration * video_info->scale) << 1)
      + video_info->rate * 90000) / ((video_info->rate * 90000) << 1)
      * video_info->mul / video_info->div;
  return AM_STATE_OK;
}

AM_STATE AMTsMuxer::on_info_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (m_av_info_map != 3) {
      /*Receive first two info packet at the beginning of the stream
       * and set audio and video info*/
      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        /* Video info */
        AM_VIDEO_INFO *video_info = (AM_VIDEO_INFO*) packet->get_data_ptr();
        NOTICE("TS Muxer received H264 INFO: "
               "size %dx%d, M %d, N %d, rate %d, scale %d, mul %d, div %d, "
               "fps %u\n",
               video_info->width, video_info->height, video_info->M,
               video_info->N, video_info->rate, video_info->scale,
               video_info->mul, video_info->div, video_info->fps);
        if((video_info->width == 0) || (video_info->height == 0) ||
            (video_info->scale == 0) || (video_info->div == 0)) {
          WARN("video info packet error, drop it.");
          break;
        }
        memcpy(&m_h264_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(set_video_info(&m_h264_info) != AM_STATE_OK)) {
          ERROR("Failed to set video info.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 1;
      } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
        /* Audio info */
        AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO *) packet->get_data_ptr();
        NOTICE("TS Muxer received Audio INFO: freq %d, chuck %d, channel %d,"
               "sample size %u",
               audio_info->sample_rate,
               audio_info->chunk_size,
               audio_info->channels,
               audio_info->sample_size);
        memcpy(&m_audio_info, audio_info, sizeof(m_audio_info));
        if (AM_UNLIKELY(set_audio_info(&m_audio_info) != AM_STATE_OK)) {
          ERROR("Failed to set audio info.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 0;
      } else {
        NOTICE("Currently, ts muxer just support audio and video stream.\n");
        ret = AM_STATE_ERROR;
        break;
      }

      if ((m_av_info_map == 3) && (m_start_file_writing)) {
        NOTICE("Ts muxer has received both audio and video info.\n");
        if(AM_UNLIKELY(build_pat_pmt() != AM_STATE_OK)) {
          ERROR("build pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
      }
    } else {
      /*After both audio and video info have been initialized, audio or
       * video info packet is received, we should closed current file,
       * and create the new file base the new info packet*/
      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        /* Video info */
        AM_VIDEO_INFO *video_info = (AM_VIDEO_INFO *) packet->get_data_ptr();
        NOTICE("Ts Muxer receive a new H264 INFO: "
               "size %dx%d, M %d, N %d, rate %d, scale %d, mul %d, div %d\n",
               video_info->width, video_info->height, video_info->M,
               video_info->N, video_info->rate, video_info->scale,
               video_info->mul, video_info->div);
        if((video_info->width == 0) || (video_info->height == 0) ||
            (video_info->scale == 0) || (video_info->div == 0)) {
          WARN("video info packet error, drop it.");
          break;
        }
        if ((video_info->M == m_h264_info.M) && (video_info->N == m_h264_info.N)
            && (video_info->stream_id == m_h264_info.stream_id)
            && (video_info->fps == m_h264_info.fps)
            && (video_info->rate == m_h264_info.rate)
            && (video_info->scale == m_h264_info.scale)
            && (video_info->type == m_h264_info.type)
            && (video_info->mul == m_h264_info.mul)
            && (video_info->div == m_h264_info.div)
            && (video_info->width == m_h264_info.width)
            && (video_info->height == m_h264_info.height)) {
          NOTICE("Video info is not changed.");
          break;
        }
        memcpy(&m_h264_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(set_video_info(&m_h264_info) != AM_STATE_OK)) {
          ERROR("Failed to set video info.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_av_info_map |= 1 << 1;
        m_is_new_info = true;
      } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
        /* Audio info */
        AM_AUDIO_INFO *audio_info = (AM_AUDIO_INFO *) packet->get_data_ptr();
        NOTICE("Ts Muxer receive a new Audio INFO: freq %d, chuck %d, channel %d\n",
               audio_info->sample_rate,
               audio_info->chunk_size,
               audio_info->channels);
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
        memcpy(&m_audio_info, audio_info, sizeof(m_audio_info));
        if (AM_UNLIKELY(set_audio_info(&m_audio_info) != AM_STATE_OK)) {
          ERROR("Failed to set audio info.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_is_new_info = true;
        m_av_info_map |= 1 << 0;
      } else {
        NOTICE("Currently, ts muxer just support audio and video stream.\n");
        ret = AM_STATE_ERROR;
        break;
      }

    }
  } while (0);

  return ret;
}

AM_STATE AMTsMuxer::on_data_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if ((m_pcr_base >= (AM_PTS) ((1LL << 33) - (m_h264_info.scale << 1)))
            && ((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)
            && (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_H264_IDR))) {

      if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
        ERROR("On eof packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(build_pat_pmt() != AM_STATE_OK)) {
        ERROR("Build pat pmt error in ts muxer.");
        ret = AM_STATE_ERROR;
        break;
      }
      packet->add_ref();
      m_packet_queue->push_back(packet);
      break;
    }
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      /* Video Data */
      if((m_av_info_map >> 1 & 0x01) != 0x01) {
        INFO("Not receive video info packet, drop this packet.");
        break;
      }
      if(m_last_video_pts == 0) {
        m_next_file_boundary = packet->get_pts() + m_splitted_duration;
      }
      if (AM_UNLIKELY(m_is_first_video)) {
        /* first frame must be video frame,
         * and the first video frame must be I frame */
        m_file_writer->set_begin_packet_pts(packet->get_pts());
        if(AM_UNLIKELY(!((packet->get_frame_type() ==
                         AM_VIDEO_FRAME_TYPE_H264_IDR) && m_is_first_audio))) {
          INFO("First frame must be video IDR frame, drop this packet.");
          break;
        }
        m_audio_enable = true;
        m_is_first_video = false;
        m_pts_base_video = packet->get_pts();
      } else {
        pcr_increment(packet->get_pts());
        m_file_writer->set_end_packet_pts(packet->get_pts());
      }
      if (m_is_new_info
          && (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_H264_IDR)) {
        /*audio ro video info is changed, create a new file*/
        m_is_new_info = false;
        if (AM_UNLIKELY(update_pat_pmt() != AM_STATE_OK)) {
          ERROR("Update pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(build_video_ts_packet(packet->get_data_ptr(),
                                              packet->get_data_size(),
                                              packet->get_pts())
            != AM_STATE_OK)) {
          ERROR("Build video ts packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
          ERROR("On eof packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        packet->add_ref();
        m_packet_queue->push_front(packet);
        break;
      }

      if (AM_UNLIKELY(m_need_splitted &&
                  ((m_video_frame_count >= m_file_video_frame_count) ||
                   (packet->get_pts() >= m_next_file_boundary)) &&
                  (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_H264_IDR))) {
        /* Duplicate this packet to last file
         * to work around TS file duration calculation issue
         */
        if(AM_UNLIKELY(update_pat_pmt() != AM_STATE_OK)) {
          ERROR("Update pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
        if(AM_UNLIKELY(build_video_ts_packet(packet->get_data_ptr(),
                              packet->get_data_size(),
                              packet->get_pts()) != AM_STATE_OK)) {
          ERROR("Build video ts packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        INFO("### Video EOF is reached, PTS: %llu, Video frame count: %u",
             packet->get_pts(), m_video_frame_count);
        if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
          ERROR("On eof packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        /* Write in new file */
        if (AM_UNLIKELY(build_pat_pmt() != AM_STATE_OK)) {
          ERROR("build pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
        packet->add_ref();
        m_packet_queue->push_front(packet);
        break;
      }
      if (AM_UNLIKELY(packet->get_frame_type() ==
                      AM_VIDEO_FRAME_TYPE_H264_IDR)) {
        if (AM_LIKELY(!m_is_first_video)) {
          if (AM_UNLIKELY(update_pat_pmt() != AM_STATE_OK)) {
            ERROR("update pat pmt error.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      }
      if(AM_UNLIKELY(build_video_ts_packet(packet->get_data_ptr(),
                            packet->get_data_size(),
                            packet->get_pts()) != AM_STATE_OK)) {
        ERROR("build video ts packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      ++ m_video_frame_count;
      m_last_video_pts = packet->get_pts();
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      if (AM_UNLIKELY(!m_audio_enable)) {
        INFO("Video frame has not been received, "
             "drop current audio packet");
        break;
      }
      if (AM_UNLIKELY(m_is_first_audio)) {
        m_pts_base_audio = packet->get_pts();
      }
      if(AM_UNLIKELY(build_audio_ts_packet(packet->get_data_ptr(),
                            packet->get_data_size(),
                            packet->get_pts()) != AM_STATE_OK)) {
        ERROR("build audio ts packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_is_first_audio = false;
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_SEI) {
      NOTICE("Got SEI packet, which is useless in TS muxer!");
    } else {
      NOTICE("Currently, ts muxer just support audio and video stream.\n");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY((m_muxer_attr == AM_MUXER_FILE_EVENT) &&
       ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_STOP) != 0))) {
      if (AM_UNLIKELY(m_file_writer->on_eos(AUDIO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on audio stream eos error.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(m_file_writer->on_eos(VIDEO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on video stream eos error.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_run = false;
      NOTICE("Receive evnet stop packet, stop writing ts event file.");
    }
  } while (0);

  return ret;
}

AM_STATE AMTsMuxer::on_eof_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      INFO("Ts muxer on eof");
      if(AM_UNLIKELY(m_file_writer->on_eof(VIDEO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on video stream eof error.");
      }
      if (AM_UNLIKELY(m_file_writer->on_eof(AUDIO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on audio stream eof error");
        ret = AM_STATE_ERROR;
        break;
      }
      m_next_file_boundary = m_last_video_pts + m_splitted_duration;
      m_is_first_audio = true;
      m_is_first_video = true;
      m_audio_enable = false;
      m_pcr_base = (90000 * m_h264_info.rate * m_h264_info.div) /
                   (m_h264_info.scale * m_h264_info.mul);
      m_pcr_ext = 0;
      m_video_frame_count = 0;
      m_pts_base_video = 0LL;
      m_pts_base_audio = 0LL;
    } else {
      ERROR("Currently, ts muxer just support video eof.\n");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);

  return ret;
}

AM_STATE AMTsMuxer::on_eos_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      NOTICE("audio eos is received.");
      if (AM_UNLIKELY(build_and_flush_audio(packet->get_pts())
          != AM_STATE_OK)) {
        ERROR("Build and flush audio error.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_eos_map |= 1 << 0;
      if (AM_UNLIKELY(m_file_writer->on_eos(AUDIO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on audio stream eos error.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      NOTICE("video eos is received");
      m_eos_map |= 1 << 1;
      if (AM_UNLIKELY(m_file_writer->on_eos(VIDEO_STREAM) != AM_STATE_OK)) {
        ERROR("File writer on video stream eos error.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      NOTICE("Currently, ts muxer just support audio and video stream.\n");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_eos_map == 0x03) {
      m_run =false;
      NOTICE("receive both audio and video eos, exit the main loop");
    }
  } while (0);

  return ret;
}

AM_STATE AMTsMuxer::init_ts()
{
  m_pat_buf.pid = 0;
  m_pmt_buf.pid = 0x100;
  m_video_pes_buf.pid = 0x200;
  m_audio_pes_buf.pid = 0x400;
  m_pat_info.total_prg = 1;
  m_video_stream_info.pid = m_video_pes_buf.pid;
  m_audio_stream_info.pid = m_audio_pes_buf.pid;

  m_video_stream_info.type = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
  m_video_stream_info.descriptor_tag = 0x51;
  m_video_stream_info.descriptor = (uint8_t*) &m_h264_info;
  m_video_stream_info.descriptor_len = sizeof(m_h264_info);

  m_program_info.pid_pmt = m_pmt_buf.pid;
  m_program_info.pid_pcr = m_video_pes_buf.pid;
  m_program_info.prg_num = 1;
  m_pat_info.prg_info = &m_program_info;

  m_pmt_info.total_stream = 2;
  m_pmt_info.prg_info = &m_program_info;
  m_pmt_info.stream[0] = &m_video_stream_info;
  m_pmt_info.stream[1] = &m_audio_stream_info;
  m_pmt_info.descriptor_tag = 5;
  m_pmt_info.descriptor_len = 4;
  m_pmt_info.descriptor = m_pmt_descriptor;
  m_pmt_info.descriptor[0] = 'H'; //0x48;
  m_pmt_info.descriptor[1] = 'D'; //0x44;
  m_pmt_info.descriptor[2] = 'M'; //0x4d;
  m_pmt_info.descriptor[3] = 'V'; //0x56;
  return AM_STATE_OK;
}

inline AM_STATE AMTsMuxer::build_audio_ts_packet(uint8_t *data,
                                             uint32_t size,
                                             AM_PTS pts)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    /* buffer the incoming audio data */
    if (AM_UNLIKELY(m_audio_chunk_buf_avail <= size)) {
      ERROR("Data size bigger than buf available size");
      ret = AM_STATE_ERROR;
      break;
    }
    memcpy(m_audio_chunk_buf_wr_ptr, data, size);
    m_audio_chunk_buf_wr_ptr += size;
    m_audio_chunk_buf_avail -= size;

    if (AM_LIKELY(m_audio_chunk_buf_avail < MAX_CODED_AUDIO_FRAME_SIZE)) {
      ret = build_and_flush_audio(pts);
      m_is_first_audio = false;
    }
  } while (0);
  return ret;
}

inline AM_STATE AMTsMuxer::build_and_flush_audio(AM_PTS pts)
{
  AM_STATE ret = AM_STATE_OK;
  AM_TS_MUXER_PES_PAYLOAD_INFO audio_payload_info = { 0 };
  audio_payload_info.first_frame  = m_is_first_audio;
  audio_payload_info.with_pcr     = 0;
  audio_payload_info.first_slice  = 1;
  audio_payload_info.p_payload    = m_audio_chunk_buf;
  audio_payload_info.payload_size =
      m_audio_chunk_buf_wr_ptr - m_audio_chunk_buf;
  audio_payload_info.pcr_base     = m_pcr_base;
  audio_payload_info.pcr_ext      = m_pcr_ext;
  audio_payload_info.pts          = (pts - m_pts_base_audio) & 0x1FFFFFFFF;
  audio_payload_info.dts          = audio_payload_info.pts;
  /*INFO("------------");*/

  while (audio_payload_info.payload_size > 0) {
    int write_len = m_ts_builder->create_transport_packet(&m_audio_stream_info,
                                                          &audio_payload_info,
                                                          m_audio_pes_buf.buf);
    if(AM_UNLIKELY(m_file_writer->write_data (m_audio_pes_buf.buf,
                                  MPEG_TS_TP_PACKET_SIZE, AUDIO_STREAM)
                                  != AM_STATE_OK)) {
      ERROR("File writer write data error.");
      ret = AM_STATE_ERROR;
      break;
    }
    audio_payload_info.first_slice = 0;
    audio_payload_info.p_payload += write_len;
    audio_payload_info.payload_size -= write_len;
  }

  m_audio_chunk_buf_wr_ptr = m_audio_chunk_buf;
  m_audio_chunk_buf_avail = AUDIO_CHUNK_BUF_SIZE;
  return ret;
}

inline AM_STATE AMTsMuxer::build_video_ts_packet(uint8_t *data,
                                             uint32_t size,
                                             AM_PTS pts)
{
  AM_STATE ret = AM_STATE_OK;
  AM_TS_MUXER_PES_PAYLOAD_INFO video_payload_info = { 0 };
  video_payload_info.first_frame = m_is_first_video;
  video_payload_info.with_pcr = 1;
  video_payload_info.first_slice = 1;
  video_payload_info.p_payload = data;
  video_payload_info.payload_size = size;
  video_payload_info.pcr_base = m_pcr_base;
  video_payload_info.pcr_ext = m_pcr_ext;
  video_payload_info.pts = (pts - m_pts_base_video) & 0x1FFFFFFFF;
  video_payload_info.dts = ( (m_h264_info.M == 1) ?
                           video_payload_info.pts : m_pcr_base);

  while (video_payload_info.payload_size > 0) {
    int write_len = m_ts_builder->create_transport_packet(&m_video_stream_info,
                                                          &video_payload_info,
                                                          m_video_pes_buf.buf);
    if(AM_UNLIKELY(m_file_writer->write_data (m_video_pes_buf.buf,
                   MPEG_TS_TP_PACKET_SIZE, VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer write data error.");
      ret = AM_STATE_ERROR;
      break;
    }
    video_payload_info.with_pcr = 0;
    video_payload_info.first_slice = 0;
    video_payload_info.p_payload += write_len;
    video_payload_info.payload_size -= write_len;
  }
  return ret;
}

inline AM_STATE AMTsMuxer::build_pat_pmt()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_ts_builder->create_pat(&m_pat_info, m_pat_buf.buf)
        != AM_STATE_OK)) {
      ERROR("Ts builder create pat error");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_ts_builder->create_pmt(&m_pmt_info, m_pmt_buf.buf)
                   != AM_STATE_OK)) {
      ERROR("Ts builder create pmt error.");
      ret = AM_STATE_ERROR;
      break;
    }

    if(AM_UNLIKELY(m_file_writer->write_data (m_pat_buf.buf,
                   MPEG_TS_TP_PACKET_SIZE, VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer write data error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_data (m_pmt_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE, VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer write data error.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

inline AM_STATE AMTsMuxer::update_pat_pmt()
{
  AM_STATE ret = AM_STATE_OK;
  m_ts_builder->update_psi_cc(m_pat_buf.buf);
  m_ts_builder->update_psi_cc(m_pmt_buf.buf);
  do {
    if (AM_UNLIKELY(m_file_writer->write_data (m_pat_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE, VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer write data error");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_data (m_pmt_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE, VIDEO_STREAM) != AM_STATE_OK)) {
      ERROR("File writer write data error");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

//inline void AMTsMuxer::pcr_sync(AM_PTS pts)
//{
//  m_pcr_base = pts - m_h264_info.rate * m_h264_info.div / m_h264_info.mul;
//  m_pcr_ext = 0;
//}

inline void AMTsMuxer::pcr_increment(int64_t cur_video_PTS)
{
  // mPcrBase = (mPcrBase + mPcrIncBase) %  (1LL << 33);
  m_pcr_base = (m_pcr_base + cur_video_PTS - m_last_video_pts) % (1LL << 33);
  m_pcr_base += (m_pcr_ext + m_pcr_inc_ext) / 300;
  m_pcr_ext = (m_pcr_ext + m_pcr_inc_ext) % 300;
}

inline void AMTsMuxer::pcr_calc_pkt_duration(uint32_t rate, uint32_t scale)
{
  AM_PTS inc = 27000000LL * rate / scale;
  m_pcr_inc_base = inc / 300;
  m_pcr_inc_ext = inc % 300;
}
