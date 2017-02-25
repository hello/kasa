/*
 * am_muxer_ts_base.cpp
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
#include "am_thread.h"
#include "am_file.h"

#include "mpeg_ts_defs.h"
#include "am_file_sink_if.h"
#include "am_muxer_ts_builder.h"
#include "am_muxer_ts_file_writer.h"
#include "am_muxer_ts_base.h"
#include "am_audio_define.h"
#include "am_video_types.h"

#include <time.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <iostream>
#include <fstream>

std::string AMTsMuxerBase::audio_type_to_string(AM_AUDIO_TYPE type)
{
  std::string type_string;
  switch (type) {
    case AM_AUDIO_NULL : {
      type_string = "AM_AUDIO_NULL";
    } break;
    case AM_AUDIO_LPCM : {
      type_string = "AM_AUDIO_LPCM";
    } break;
    case AM_AUDIO_BPCM : {
      type_string = "AM_AUDIO_BPCM";
    } break;
    case AM_AUDIO_G711A : {
      type_string = "AM_AUDIO_G711A";
    } break;
    case AM_AUDIO_G711U : {
      type_string = "AM_AUDIO_G711U";
    } break;
    case AM_AUDIO_G726_40 : {
      type_string = "AM_AUDIO_G726_40";
    } break;
    case AM_AUDIO_G726_32 : {
      type_string = "AM_AUDIO_G726_32";
    } break;
    case AM_AUDIO_G726_24 : {
      type_string = "AM_AUDIO_G726_24";
    } break;
    case AM_AUDIO_G726_16 : {
      type_string = "AM_AUDIO_G726_16";
    } break;
    case AM_AUDIO_AAC : {
      type_string = "AM_AUDIO_AAC";
    } break;
    case AM_AUDIO_OPUS : {
      type_string = "AM_AUDIO_OPUS";
    } break;
    case AM_AUDIO_SPEEX : {
      type_string = "AM_AUDIO_SPEEX";
    } break;
    default : {
      type_string = "Invalid audio type";
    } break;
  }
  return type_string;
}

/*interface of AMIMuxerCodec*/
AM_STATE AMTsMuxerBase::start()
{
  INFO("Begin to start %s", m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_lock);
  AM_STATE ret = AM_STATE_OK;
  do {
    bool need_break = false;
    switch(get_state()) {
      case AM_MUXER_CODEC_RUNNING: {
        NOTICE("%s is already running!", m_muxer_name.c_str());
        need_break = true;
      }break;
      case AM_MUXER_CODEC_ERROR: {
        ERROR("%s state is AM_MUXER_CODEC_ERROR! Need to be re-created!",
              m_muxer_name.c_str());
        need_break = true;
      }break;
      default:break;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
    AM_DESTROY(m_thread);
    if (AM_LIKELY(!m_thread)) {
      m_thread = AMThread::create(m_muxer_name.c_str(), thread_entry, this);
      if (AM_UNLIKELY(!m_thread)) {
        ERROR("Failed to create m_thread : %s.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    while ((m_state == AM_MUXER_CODEC_STOPPED) ||
           (m_state == AM_MUXER_CODEC_INIT)) {
      usleep(5000);
    }
    if (get_state() == AM_MUXER_CODEC_RUNNING) {
      INFO("Start %s success", m_muxer_name.c_str());
    } else {
      ERROR("Failed to start %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerBase::stop()
{
  INFO("Stopping %s", m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_lock);
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("Stop  %s successfully.");
  return AM_STATE_OK;
}

const char* AMTsMuxerBase::name()
{
  return m_muxer_name.c_str();
}

void* AMTsMuxerBase::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIFileOperation) ? ((AMIFileOperation*)this) :
      ((refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
          ((void*)nullptr));
}

bool AMTsMuxerBase::stop_file_writing()
{
  bool ret = true;
  INFO("Stopping file writing in %s.", m_muxer_name.c_str());
  do{
    AUTO_MEM_LOCK(m_file_param_lock);
    if(!m_file_writing) {
      NOTICE("File writing is already stopped in %s", m_muxer_name.c_str());
      break;
    }
    m_file_writing = false;
    if (m_file_writer && m_file_writer->is_file_open()) {
      m_file_writer->close_file();
    }
    while(!m_hevc_pkt_queue->empty()) {
      m_hevc_pkt_queue->front()->release();
      m_hevc_pkt_queue->pop_front();
    }
    clear_params_for_new_file();
    m_file_counter = 0;
    m_stop_recording_pts = 0;
    m_param_set_lock.lock();
    m_set_config.recording_duration = 0;
    m_set_config.recording_file_num = 0;
    m_param_set_lock.unlock();
    INFO("File writing have been stopped in %s", m_muxer_name.c_str());
  }while(0);
  return ret;
}

bool AMTsMuxerBase::set_recording_file_num(uint32_t file_num)
{
  NOTICE("Set recording file num %u success in %s", file_num,
           m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_param_set_lock);
  m_set_config.recording_file_num = file_num;
  return true;
}

bool AMTsMuxerBase::set_recording_duration(int32_t duration)
{
  NOTICE("Set recording duration %u seconds in %s", duration,
           m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_param_set_lock);
  m_set_config.recording_duration = duration;
  return true;
}

bool AMTsMuxerBase::set_file_duration(int32_t file_duration, bool apply_conf_file)
{
  bool ret = true;
  NOTICE("Set file duration %u seconds in %s", file_duration,
         m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_param_set_lock);
    m_set_config.file_duration = file_duration;
    if ((file_duration > 0) && (!m_need_splitted)) {
      ERROR("need split is false");
      m_need_splitted = true;
      m_curr_file_boundary = 0;
    }
    if ((file_duration <= 0) && (m_need_splitted)) {
      m_need_splitted = false;
    }
    if (apply_conf_file) {
      m_muxer_ts_config->file_duration = file_duration;
      if (AM_UNLIKELY(!m_config->set_config(m_muxer_ts_config))) {
        ERROR("Failed to set mp4 config file.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMTsMuxerBase::is_running()
{
  return m_run.load();
}

void AMTsMuxerBase::reset_parameter()
{
  m_audio_enable = false;
  m_is_first_audio = true;
  m_is_first_video = true;
  m_av_info_map = 0;
  m_is_new_info = false;
  m_eos_map = 0;
  m_curr_file_boundary = 0LL;
  m_pts_base_video = 0LL;
  m_pts_base_audio = 0LL;
  m_pcr_base = 0;
  m_pcr_ext = 0;
  m_pcr_inc_base = 0;
  m_pcr_inc_ext = 0;
  m_last_video_pts = 0;
  m_first_video_pts = 0;
  m_video_frame_count = 0;
  m_file_counter = 0;
  m_adts.clear();
}

AM_MUXER_CODEC_STATE AMTsMuxerBase::get_state()
{
  AUTO_MEM_LOCK(m_state_lock);
  return m_state;
}

uint8_t AMTsMuxerBase::get_muxer_codec_stream_id()
{
  return (uint8_t)((0x01) << (m_curr_config.video_id));
}

uint32_t AMTsMuxerBase::get_muxer_id()
{
  return m_curr_config.muxer_id;
}

AM_STATE AMTsMuxerBase::set_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMTsMuxerBase::get_config(AMMuxerCodecConfig *config)
{
  return AM_STATE_ERROR;
}

void AMTsMuxerBase::thread_entry(void* p)
{
  ((AMTsMuxerBase*) p)->main_loop();
}

AM_MUXER_CODEC_STATE AMTsMuxerBase::create_resource()
{
  std::string file_name;
  AUTO_MEM_LOCK(m_state_lock);
  INFO("Begin to create resource in %s", m_muxer_name.c_str());
  AM_MUXER_CODEC_STATE ret = AM_MUXER_CODEC_RUNNING;
  do {
    reset_parameter();
    if (AM_UNLIKELY((m_ts_builder = new AMTsBuilder ()) == NULL)) {
      ERROR("Failed to create ts builder in %s", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_file_writer = AMTsFileWriter::create (
        &m_curr_config, &m_video_info)) == NULL)) {
      ERROR("Failed to create m_file_writer in %s!", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (m_file_create_cb) {
      if (!m_file_writer->set_file_operation_cb(
          AM_OPERATION_CB_FILE_CREATE, m_file_create_cb)) {
        ERROR("Failed to set file create callback function to file writer"
            "in %s", m_muxer_name.c_str());
      }
    }
    if (m_file_finish_cb) {
      if (!m_file_writer->set_file_operation_cb(
          AM_OPERATION_CB_FILE_FINISH, m_file_finish_cb)) {
        ERROR("Failed to set file finish callback function to file writer"
            "in %s", m_muxer_name.c_str());
      }
    }
    if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
      ERROR("Failed to generate file name in %s.", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name.c_str()) != AM_STATE_OK)) {
      ERROR("Failed to set media sink for m_file_writer in %s",
            m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (init_ts() != AM_STATE_OK) {
      ERROR("Failed to init ts in %s", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    m_state = ret;
    INFO("Create resource success in %s", m_muxer_name.c_str());
  }while(0);

  return ret;
}

void AMTsMuxerBase::release_resource()
{
  INFO("Begin to release resource in %s.", m_muxer_name.c_str());
  while (!(m_packet_queue->empty())) {
    m_packet_queue->front_and_pop()->release();
  }
  while(!m_hevc_pkt_queue->empty()) {
    m_hevc_pkt_queue->front_and_pop()->release();
  }
  if (m_file_writer) {
    m_file_writer->close_file(true);
  }
  m_set_config.recording_duration = 0;
  m_set_config.recording_file_num = 0;
  AM_DESTROY(m_ts_builder);
  AM_DESTROY(m_file_writer);
  m_file_create_cb = nullptr;
  m_file_finish_cb = nullptr;
  INFO("Release resource successfully in %s", m_muxer_name.c_str());
}

AMTsMuxerBase::AMTsMuxerBase()
{
  m_adts.clear();
  m_muxer_name.clear();
  m_file_location.clear();
}

AM_STATE AMTsMuxerBase::init(const char* config_file)
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
    if (AM_UNLIKELY((m_hevc_pkt_queue = new packet_queue()) == NULL)) {
      ERROR("Failed to create packet_queue.");
      ret = AM_STATE_NO_MEMORY;
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
        WARN("HLS is enabled, the file duration is larger than 300s, set it 300s");
        m_muxer_ts_config->file_duration = 300;
      }
    }
    m_curr_config = *m_muxer_ts_config;
    m_set_config = m_curr_config;
    m_file_writing = m_curr_config.auto_file_writing;
    m_need_splitted = (m_curr_config.file_duration > 0);
    if (m_curr_config.audio_sample_rate > 0) {
      m_audio_sample_rate_set = true;
    } else {
      m_audio_sample_rate_set = false;
    }
  } while (0);

  return ret;
}

AMTsMuxerBase::~AMTsMuxerBase()
{
  stop();
  delete m_hevc_pkt_queue;
  delete m_packet_queue;
  delete m_config_file;
  delete m_config;
  DEBUG("%s destroyed", m_muxer_name.c_str());
}

bool AMTsMuxerBase::get_proper_file_location(std::string& file_location)
{
  bool ret = true;
  uint64_t max_free_space = 0;
  uint64_t free_space = 0;
  std::ifstream file;
  std::string storage_str = "/storage";
  std::string sdcard_str = "/sdcard";
  std::string config_path;
  string_list location_list;
  location_list.clear();
  size_t find_str_position = 0;
  INFO("Begin to get proper file location in %s.", m_muxer_name.c_str());
  do {
    if (m_curr_config.file_location_auto_parse) {
      std::string file_location_suffix;
      std::string::size_type pos = 0;
      if ((pos = m_curr_config.file_location.find(storage_str, pos))
          != std::string::npos) {
        pos += storage_str.size() + 1;
        config_path = storage_str + "/";
      } else {
        pos = 1;
        config_path = "/";
      }
      bool parse_config_path = false;
      for (uint32_t i = pos;i < m_curr_config.file_location.size(); ++ i) {
        if (m_curr_config.file_location.substr(i, 1) != "/") {
          config_path += m_curr_config.file_location.substr(i, 1);
        } else {
          INFO("Get config path is %s", config_path.c_str());
          parse_config_path = true;
          break;
        }
      }
      if (!parse_config_path) {
        WARN("The file location in config file is not on sdcard or storage, "
            "use file location specified in config file.");
        file_location = m_curr_config.file_location;
        break;
      }
      pos = m_curr_config.file_location.find('/', pos);
      file_location_suffix = m_curr_config.file_location.substr(pos);
      file.open("/proc/self/mounts");
      std::string read_line;
      while (getline(file, read_line)) {
        std::string temp_location;
        temp_location.clear();
        if ((find_str_position = read_line.find(storage_str))
            != std::string::npos) {
          for (uint32_t i = find_str_position;; ++ i) {
            if (read_line.substr(i, 1) != " ") {
              temp_location += read_line.substr(i, 1);
            } else {
              location_list.push_back(temp_location);
              INFO("find a storage str : %s", temp_location.c_str());
              break;
            }
          }
        } else if ((find_str_position = read_line.find(sdcard_str))
            != std::string::npos) {
          for (uint32_t i = find_str_position;; ++ i) {
            if (read_line.substr(i, 1) != " ") {
              temp_location += read_line.substr(i, 1);
            } else {
              location_list.push_back(temp_location);
              INFO("find a sdcard str : %s", temp_location.c_str());
              break;
            }
          }
        }
      }
      if (!location_list.empty()) {
        string_list::iterator it = location_list.begin();
        for (; it != location_list.end(); ++ it) {
          if ((*it) == config_path) {
            NOTICE("File location specified by config file is on %s, great."
                , config_path.c_str());
            file_location = config_path + file_location_suffix;
            break;
          }
        }
        if (it == location_list.end()) {
          WARN("File location specified by config file is not on sdcard "
              "or usb storage, %s will auto parse a proper file location",
              m_muxer_name.c_str());
          string_list::iterator max_free_space_it = location_list.begin();
          for (string_list::iterator i = location_list.begin();
              i != location_list.end(); ++ i) {
            struct statfs disk_statfs;
            if (statfs((*i).c_str(), &disk_statfs) < 0) {
              PERROR("file location statfs");
              ret = false;
              break;
            } else {
              free_space = ((uint64_t) disk_statfs.f_bsize
                  * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
              if (free_space > max_free_space) {
                max_free_space = free_space;
                max_free_space_it = i;
              }
            }
          }
          if (!ret) {
            break;
          }
          struct statfs disk_statfs;
          if (statfs(config_path.c_str(), &disk_statfs) < 0) {
            NOTICE("The file location in config file is not exist in %s.",
                   m_muxer_name.c_str());
            file_location = (*max_free_space_it) + file_location_suffix;
          } else {
            free_space = ((uint64_t) disk_statfs.f_bsize
                * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
            if (free_space > max_free_space) {
              NOTICE("The free space of file location in config file is larger"
                  "than sdcard free space, use file location in config file.");
              file_location = config_path + file_location_suffix;
            } else {
              file_location = (*max_free_space_it) + file_location_suffix;
              NOTICE("The free space of file location in config file is smaller "
                  "than sdcard free space, set file location on %s.",
                  (*max_free_space_it).c_str());
            }
          }
        }
      } else {
        NOTICE("Do not find storage or sdcard string in mount information.");
        if (!AMFile::exists(m_curr_config.file_location.c_str())) {
          if (!AMFile::create_path(m_curr_config.file_location.c_str())) {
            ERROR("Failed to create file path: %s!",
                  m_curr_config.file_location.c_str());
            ret = false;
            break;
          }
        }
        struct statfs disk_statfs;
        if (statfs(m_curr_config.file_location.c_str(), &disk_statfs)
            < 0) {
          PERROR("file location in config file statfs");
          ret = false;
          break;
        } else {
          free_space = ((uint64_t) disk_statfs.f_bsize
              * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
          if (free_space >= 20) {
            file_location = m_curr_config.file_location;
            NOTICE("Free space is larger than 20M, use it.");
            break;
          } else {
            ERROR("free space is smaller than 20M, please"
                "set file location on sdcard or usb storage.");
            ret = false;
            break;
          }
        }
      }
    } else { //file_location_auto_parse is false
      file_location = m_curr_config.file_location;
      break;
    }
    if (!ret) {
      break;
    }
    NOTICE("Get proper file location: %s success in %s.",
           file_location.c_str(),
           m_muxer_name.c_str());
  } while (0);
  if (file.is_open()) {
    file.close();
  }
  return ret;
}

bool AMTsMuxerBase::get_current_time_string(char *time_str, int32_t len)
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

AM_STATE AMTsMuxerBase::set_audio_info(AM_AUDIO_INFO* audio_info)
{
  AM_STATE ret = AM_STATE_OK;
  m_ts_builder->set_audio_info(audio_info);
  switch(audio_info->type) {
    case  AM_AUDIO_AAC: {
      m_audio_stream_info.type = MPEG_SI_STREAM_TYPE_AAC;
      m_audio_stream_info.descriptor_tag = 0x52;
      m_audio_stream_info.descriptor_len = 0;
      m_audio_stream_info.descriptor = nullptr;
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
      ERROR("Currently, we just support aac and bpcm audio in %s",
            m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
    }break;
  }
  return ret;
}

AM_STATE AMTsMuxerBase::set_video_info(AM_VIDEO_INFO* video_info)
{
  m_ts_builder->set_video_info(video_info);
  pcr_calc_pkt_duration(video_info->rate, video_info->scale);
  m_pcr_base = 0;
  m_pcr_ext = 0;
  return AM_STATE_OK;
}

AM_STATE AMTsMuxerBase::on_eof_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      INFO("On eof in %s", m_muxer_name.c_str());
      if(AM_UNLIKELY(m_file_writer->close_file(m_curr_config.write_sync_enable)
                     != AM_STATE_OK)) {
        ERROR("Failed to close file %s in %s",
              m_file_writer->get_current_file_name(), m_muxer_name.c_str());
        break;
      }
      clear_params_for_new_file();
      if ((m_curr_config.recording_file_num > 0) &&
          (m_file_counter >= m_curr_config.recording_file_num)) {
        NOTICE("File number is equal to  the value of m_recording_file_num,"
            " stop file writing in %s.", m_muxer_name.c_str());
        m_file_writing = false;
        m_file_counter = 0;
        m_stop_recording_pts = 0;
        m_set_config.recording_duration = 0;
        m_set_config.recording_file_num = 0;
        while(!m_hevc_pkt_queue->empty()) {
          m_hevc_pkt_queue->front()->release();
          m_hevc_pkt_queue->pop_front();
        }
        break;
      }
      update_config_param();
      if (AM_UNLIKELY(m_file_writer->create_next_file() != AM_STATE_OK)) {
        ERROR("Failed to create new file in %s", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      ++ m_file_counter;
    } else {
      ERROR("Currently, %s just support video eof", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerBase::on_eos_packet(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!m_file_writing) {
      break;
    }
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      NOTICE("Audio eos is received in %s.", m_muxer_name.c_str());
      m_eos_map |= 1 << 0;
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      NOTICE("Video eos is received in %s", m_muxer_name.c_str());
      m_eos_map |= 1 << 1;
    } else {
      NOTICE("Currently, %s just support audio and video stream.\n",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_eos_map == 0x03) {
      if (AM_UNLIKELY(m_file_writer->close_file(m_curr_config.write_sync_enable)
                      != AM_STATE_OK)) {
        ERROR("Failed to close file %s in %s",
              m_file_writer->get_current_file_name(),
              m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      m_run = false;
      NOTICE("Receive both audio and video eos in %s, exit the main loop",
             m_muxer_name.c_str());
    }
  } while (0);

  return ret;
}

AM_STATE AMTsMuxerBase::init_ts()
{
  m_pat_buf.pid = 0;
  m_pmt_buf.pid = 0x100;
  m_video_pes_buf.pid = 0x200;
  m_audio_pes_buf.pid = 0x400;
  m_pat_info.total_prg = 1;
  m_video_stream_info.pid = m_video_pes_buf.pid;
  m_audio_stream_info.pid = m_audio_pes_buf.pid;
  m_video_stream_info.descriptor_tag = 0x51;
  m_video_stream_info.descriptor = (uint8_t*) &m_video_info;
  m_video_stream_info.descriptor_len = sizeof(m_video_info);

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

AM_STATE AMTsMuxerBase::build_and_write_audio(AMPacket* packet, uint32_t frame_num)
{
  AM_STATE ret = AM_STATE_OK;
  AM_TS_MUXER_PES_PAYLOAD_INFO audio_payload_info = { 0 };
  uint8_t *data = packet->get_data_ptr();
  uint32_t size = packet->get_data_size();
  if (frame_num > 0) {
    find_adts(packet);
    data = m_adts[frame_num].addr;
    for(uint8_t i = 0; i < frame_num; ++ i) {
      size -= m_adts[i].size;
    }
    m_adts.clear();
  }
  audio_payload_info.first_frame  = m_is_first_audio;
  audio_payload_info.with_pcr     = 0;
  audio_payload_info.first_slice  = 1;
  audio_payload_info.p_payload    = data;
  audio_payload_info.payload_size = size;
    audio_payload_info.pcr_base     = 0;
    audio_payload_info.pcr_ext      = 0;
  if (m_is_first_audio) {
    audio_payload_info.pts = 0;
  } else {
    int64_t first_frame_pts = packet->get_pts() -
        ((packet->get_frame_count() - 1) * (m_audio_info.pkt_pts_increment /
            packet->get_frame_count()));
    audio_payload_info.pts          = (first_frame_pts - m_pts_base_audio)
                                          & 0x1FFFFFFFF;
  }
  audio_payload_info.dts          = audio_payload_info.pts;
  while (audio_payload_info.payload_size > 0) {
    int write_len = m_ts_builder->create_transport_packet(&m_audio_stream_info,
                                                          &audio_payload_info,
                                                          m_audio_pes_buf.buf);
    if (write_len <= 0) {
      ERROR("Failed to create TS packet.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_file_writer->write_data (m_audio_pes_buf.buf,
                                  MPEG_TS_TP_PACKET_SIZE)
                                  != AM_STATE_OK)) {
      ERROR("Failed to write data in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    audio_payload_info.first_slice = 0;
    audio_payload_info.p_payload += write_len;
    audio_payload_info.payload_size -= write_len;
  }
  return ret;
}

AM_STATE AMTsMuxerBase::build_and_write_video(uint8_t *data,
                                             uint32_t size,
                                             AM_PTS pts,
                                             bool frame_begin)
{
  AM_STATE ret = AM_STATE_OK;
  AM_TS_MUXER_PES_PAYLOAD_INFO video_payload_info = { 0 };
  video_payload_info.first_frame = m_is_first_video;
  if (frame_begin) {
    video_payload_info.with_pcr = 1;
    video_payload_info.first_slice = 1;
  } else {
    video_payload_info.with_pcr = 0;
    video_payload_info.first_slice = 0;
  }
  video_payload_info.p_payload = data;
  video_payload_info.payload_size = size;
  video_payload_info.pcr_base = m_pcr_base;
  video_payload_info.pcr_ext = m_pcr_ext;
  video_payload_info.pts = (pts - m_pts_base_video) & 0x1FFFFFFFF;
  video_payload_info.dts = ( (m_video_info.M == 1) ?
                           video_payload_info.pts : m_pcr_base);
  while (video_payload_info.payload_size > 0) {
    int write_len = m_ts_builder->create_transport_packet(&m_video_stream_info,
                                                          &video_payload_info,
                                                          m_video_pes_buf.buf);
    if (write_len <= 0) {
      ERROR("Failed to create TS packet.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_file_writer->write_data (m_video_pes_buf.buf,
                   MPEG_TS_TP_PACKET_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write data in %s.", m_muxer_name.c_str());
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

AM_STATE AMTsMuxerBase::build_and_write_pat_pmt()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_ts_builder->create_pat(&m_pat_info, m_pat_buf.buf)
        != AM_STATE_OK)) {
      ERROR("%s create pat error", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_ts_builder->create_pmt(&m_pmt_info, m_pmt_buf.buf)
                   != AM_STATE_OK)) {
      ERROR("%s create pmt error.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }

    if(AM_UNLIKELY(m_file_writer->write_data (m_pat_buf.buf,
                   MPEG_TS_TP_PACKET_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write data in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_data (m_pmt_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write data in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerBase::update_and_write_pat_pmt()
{
  AM_STATE ret = AM_STATE_OK;
  m_ts_builder->update_psi_cc(m_pat_buf.buf);
  m_ts_builder->update_psi_cc(m_pmt_buf.buf);
  do {
    if (AM_UNLIKELY(m_file_writer->write_data (m_pat_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write data in %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_data (m_pmt_buf.buf,
                    MPEG_TS_TP_PACKET_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write data in %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsMuxerBase::process_h264_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  bool is_first_slice = true;
  do {
    /* Video Data */
    if((m_av_info_map >> 1 & 0x01) != 0x01) {
      INFO("Not receive video info packet, drop this packet.");
      break;
    }
    if (AM_UNLIKELY(m_is_first_video)) {
      /* first frame must be video frame,
       * and the first video frame must be I frame */
      m_first_video_pts = packet->get_pts();
      m_file_writer->set_begin_packet_pts(packet->get_pts());
      if(AM_UNLIKELY(!((packet->get_frame_type() ==
          AM_VIDEO_FRAME_TYPE_IDR) && m_is_first_audio))) {
        INFO("First frame must be video IDR frame, drop this packet.");
        break;
      }
      if ((m_curr_config.recording_duration > 0) && (m_stop_recording_pts == 0)) {
        m_stop_recording_pts = packet->get_pts() +
            (int64_t)(m_curr_config.recording_duration * 90000);
      }
      m_curr_file_boundary = packet->get_pts() + m_curr_config.file_duration * 90000;
      m_audio_enable = true;
      m_pts_base_video = packet->get_pts();
    } else {
      pcr_increment(packet->get_pts());
      m_file_writer->set_end_packet_pts(packet->get_pts());
    }
    if (m_is_new_info) {
      if (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR) {
        /*audio ro video info is changed, create a new file*/
        m_is_new_info = false;
        if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
          ERROR("On eof packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        /* Write in new file */
        if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
          ERROR("build pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
        packet->add_ref();
        m_packet_queue->push_front(packet);
        break;
      } else {
        NOTICE("New info is comming, but the first packet is not IDR frame, "
            "drop it");
        break;
      }
    }
    if ((m_stop_recording_pts > 0) && (packet->get_pts() >= m_stop_recording_pts)) {
      NOTICE("Packet pts reach stop recording pts, Stopping file writing in %s.",
           m_muxer_name.c_str());
      m_file_writing = false;
      if (m_file_writer) {
        m_file_writer->close_file();
      }
      clear_params_for_new_file();
      m_file_counter = 0;
      m_stop_recording_pts = 0;
      m_set_config.recording_duration = 0;
      m_set_config.recording_file_num = 0;
      break;
    }
    if (AM_UNLIKELY(m_need_splitted &&
                    ((packet->get_pts() >= m_curr_file_boundary)) &&
                     (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR))) {
      /* Duplicate this packet to last file
       * to work around TS file duration calculation issue
       */
      if(AM_UNLIKELY(update_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("Update pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
      if(AM_UNLIKELY(build_and_write_video(packet->get_data_ptr(),
                                           packet->get_data_size(),
                                           packet->get_pts(),
                                           is_first_slice) != AM_STATE_OK)) {
        ERROR("Build video ts packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      INFO("Video EOF is reached, PTS: %lld, Video frame count: %u",
           packet->get_pts(), m_video_frame_count);
      if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
        ERROR("On eof packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      /*reach the file number per recording, stop file writing.*/
      if (!m_file_writing) {
        break;
      }
      /* Write in new file */
      if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("build pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
      packet->add_ref();
      m_packet_queue->push_front(packet);
      break;
    }
    if (AM_UNLIKELY(packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR)) {
      if (AM_UNLIKELY(update_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("update pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if(AM_UNLIKELY(build_and_write_video(packet->get_data_ptr(),
                                         packet->get_data_size(),
                                         packet->get_pts(),
                                         is_first_slice) != AM_STATE_OK)) {
      ERROR("build video ts packet error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_is_first_video) {
      m_is_first_video = false;
    }
    ++ m_video_frame_count;
    m_last_video_pts = packet->get_pts();
  } while(0);
  return ret;
}

AM_STATE AMTsMuxerBase::process_h265_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  bool is_first_slice = true;
  do {
    if((m_av_info_map >> 1 & 0x01) != 0x01) {
      INFO("Not receive video info packet, drop this packet.");
      break;
    }
    uint32_t frame_count = packet->get_frame_count();
    uint8_t slice_id = (uint8_t)((frame_count & 0x00ff0000) >> 16);
    uint8_t tile_id = (uint8_t)((frame_count & 0x000000ff));
    if ((slice_id == 0) && (tile_id == 0)) {
      while(!m_hevc_pkt_queue->empty()) {
        m_hevc_pkt_queue->front()->release();
        m_hevc_pkt_queue->pop_front();
      }
      m_hevc_slice_num = ((uint8_t)((frame_count & 0xff000000) >> 24)
          == 0) ? 1 : (uint8_t)((frame_count & 0xff000000) >> 24);
      m_hevc_tile_num = ((uint8_t)((frame_count & 0x0000ff00) >> 8)
          == 0) ? 1 : (uint8_t)((frame_count & 0x0000ff00) >> 8);
    }
    if ((m_hevc_pkt_queue->empty()) && ((slice_id != 0) || (tile_id != 0))) {
      INFO("The first nalu is not the begin nalu in %s, drop it.",
           m_muxer_name.c_str());
      break;
    }
    packet->add_ref();
    m_hevc_pkt_queue->push_back(packet);
    if (!((slice_id == (m_hevc_slice_num - 1)) &&
                    (tile_id == (m_hevc_tile_num - 1)))) {
      break;
    }
    if (AM_UNLIKELY(m_is_first_video)) {
      /* first frame must be video frame,
       * and the first video frame must be I frame */
      if(AM_UNLIKELY(!((packet->get_frame_type() ==
          AM_VIDEO_FRAME_TYPE_IDR) && m_is_first_audio))) {
        INFO("First frame must be video IDR frame, drop this packet.");
        while(!m_hevc_pkt_queue->empty()) {
          m_hevc_pkt_queue->front_and_pop()->release();
        }
        break;
      }
      if ((m_curr_config.recording_duration > 0) && (m_stop_recording_pts == 0)) {
        m_stop_recording_pts = packet->get_pts() +
            (int64_t)(m_curr_config.recording_duration * 90000);
      }
      m_first_video_pts = packet->get_pts();
      m_file_writer->set_begin_packet_pts(packet->get_pts());
      m_audio_enable = true;
      m_pts_base_video = packet->get_pts();
      m_curr_file_boundary = packet->get_pts() + m_curr_config.file_duration * 90000;
    } else {
      pcr_increment(packet->get_pts());
      m_file_writer->set_end_packet_pts(packet->get_pts());
    }
    if (m_is_new_info) {
      if (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR) {
        //audio ro video info is changed, create a new file
        m_is_new_info = false;
        if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
          ERROR("On eof packet error.");
          ret = AM_STATE_ERROR;
          break;
        }
        // Write in new file
        if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
          ERROR("build pat pmt error.");
          ret = AM_STATE_ERROR;
          break;
        }
        while (!m_hevc_pkt_queue->empty()) {
          m_packet_queue->push_front(m_hevc_pkt_queue->back_and_pop());
        }
        break;
      } else {
        NOTICE("New info is comming, but the first packet is not IDR frame, "
            "drop it");
        break;
      }
    }
    if ((m_stop_recording_pts > 0) && (packet->get_pts() >= m_stop_recording_pts)) {
      INFO("Packet pts reach stop recording pts, Stopping file writing in %s.",
           m_muxer_name.c_str());
      m_file_writing = false;
      if (m_file_writer) {
        m_file_writer->close_file();
      }
      while(!m_hevc_pkt_queue->empty()) {
        m_hevc_pkt_queue->front()->release();
        m_hevc_pkt_queue->pop_front();
      }
      clear_params_for_new_file();
      m_file_counter = 0;
      m_stop_recording_pts = 0;
      m_set_config.recording_duration = 0;
      m_set_config.recording_file_num = 0;
      break;
    }
    if (AM_UNLIKELY(m_need_splitted &&
                    ((packet->get_pts() >= m_curr_file_boundary)) &&
                    (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR))) {
      /* Duplicate this packet to last file
       * to work around TS file duration calculation issue
       */
      if(AM_UNLIKELY(update_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("Update pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
      is_first_slice = true;
      uint8_t *vdata = nullptr;
      uint32_t queue_size = m_hevc_pkt_queue->size();
      uint32_t vdata_size = 0;
      for (uint32_t i = 0; i < queue_size; ++ i) {
        AMPacket *cur_pkt = m_hevc_pkt_queue->front_and_pop();
        vdata_size += cur_pkt->get_data_size();
        m_hevc_pkt_queue->push_back(cur_pkt);
      }
      vdata = new uint8_t[vdata_size];
      if (!vdata) {
        ERROR("Failed to malloc memory.");
        ret = AM_STATE_ERROR;
        break;
      }
      uint8_t *cur_data = vdata;
      for (uint32_t i = 0; i < queue_size; ++ i) {
        AMPacket *cur_pkt = m_hevc_pkt_queue->front_and_pop();
        uint32_t pkt_data_size = cur_pkt->get_data_size();
        memcpy(cur_data, cur_pkt->get_data_ptr(), pkt_data_size);
        cur_data += pkt_data_size;
        m_hevc_pkt_queue->push_back(cur_pkt);
      }
      if(AM_UNLIKELY(build_and_write_video(vdata,
                                           vdata_size,
                                           m_hevc_pkt_queue->front()->get_pts(),
                                           is_first_slice) != AM_STATE_OK)) {
        ERROR("Build video ts packet error.");
        ret = AM_STATE_ERROR;
      }
      delete[] vdata;
      if (ret == AM_STATE_ERROR) {
        break;
      }
      INFO("Video EOF is reached, PTS: %lld, Video frame count: %u",
           packet->get_pts(), m_video_frame_count);
      if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
        ERROR("On eof packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      /*reach the file number per recording, stop file writing.*/
      if (!m_file_writing) {
        break;
      }
      /* Write in new file */
      if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("build pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
      while (!m_hevc_pkt_queue->empty()) {
        m_packet_queue->push_front(m_hevc_pkt_queue->back_and_pop());
      }
      break;
    }
    if (AM_UNLIKELY((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR) &&
                    (!m_is_first_video))) {
      if (AM_UNLIKELY(update_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("update pat pmt error.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    AMPacket* tmp_pkt = nullptr;
    is_first_slice = true;
    uint32_t size = m_hevc_pkt_queue->size();
    uint8_t *data = nullptr;
    uint32_t data_size = 0;
    for (uint32_t i = 0; i < size; ++ i) {
      tmp_pkt = m_hevc_pkt_queue->front_and_pop();
      data_size += tmp_pkt->get_data_size();
      m_hevc_pkt_queue->push_back(tmp_pkt);
    }
    data = new uint8_t[data_size];
    if (!data) {
      ERROR("Failed to malloc memory.");
      ret = AM_STATE_ERROR;
      break;
    }
    uint8_t* tmp_data = data;
    int64_t pts = m_hevc_pkt_queue->front()->get_pts();
    while (!m_hevc_pkt_queue->empty()) {
      tmp_pkt = m_hevc_pkt_queue->front_and_pop();
      uint32_t pkt_data_size = tmp_pkt->get_data_size();
      memcpy(tmp_data, tmp_pkt->get_data_ptr(), pkt_data_size);
      tmp_data += pkt_data_size;
      tmp_pkt->release();
    }
    if(AM_UNLIKELY(build_and_write_video(data,
                                         data_size,
                                         pts,
                                         is_first_slice) != AM_STATE_OK)) {
      ERROR("Build video ts packet error.");
      ret = AM_STATE_ERROR;
    }
    delete[] data;
    if (ret == AM_STATE_ERROR) {
      break;
    }
    if (m_is_first_video) {
      m_is_first_video = false;
    }
    ++ m_video_frame_count;
    m_last_video_pts = packet->get_pts();
  } while(0);
  return ret;
}

AM_STATE AMTsMuxerBase::process_audio_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(!m_audio_enable)) {
      INFO("Video frame has not been received, drop current audio packet "
          "in %s", m_muxer_name.c_str());
      break;
    }
    uint32_t frame_num = 0;
    if (packet->get_pts() < m_first_video_pts) {
      INFO("First audio pkt pts is larger than first video pts, drop it.");
      break;
    }
    if (AM_UNLIKELY(m_is_first_audio)) {
      uint32_t frame_count = packet->get_frame_count();
      m_pts_base_audio = packet->get_pts();
      if (frame_count > 1) {
        int32_t pts_diff = packet->get_pts() - m_first_video_pts;
        int32_t pts_diff_frame = m_audio_info.pkt_pts_increment / frame_count;
        int32_t frame_number_tmp = frame_count - pts_diff / pts_diff_frame - 1;
        frame_num = (frame_number_tmp >= 0) ? (uint32_t) frame_number_tmp : 0;
        m_pts_base_audio = packet->get_pts() - pts_diff_frame *
            (frame_count - frame_number_tmp - 1);
      }
    }
    if(AM_UNLIKELY(build_and_write_audio(packet, frame_num) != AM_STATE_OK)) {
      ERROR("Build audio ts packet error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_is_first_audio) {
      m_is_first_audio = false;
    }
  } while(0);
  return ret;
}

void AMTsMuxerBase::pcr_increment(int64_t cur_video_PTS)
{
  // mPcrBase = (mPcrBase + mPcrIncBase) %  (1LL << 33);
  m_pcr_base = (m_pcr_base + cur_video_PTS - m_last_video_pts) % (1LL << 33);
  m_pcr_base += (m_pcr_ext + m_pcr_inc_ext) / 300;
  m_pcr_ext = (m_pcr_ext + m_pcr_inc_ext) % 300;
}

void AMTsMuxerBase::pcr_calc_pkt_duration(uint32_t rate, uint32_t scale)
{
  AM_PTS inc = 27000000LL * rate / scale;
  m_pcr_inc_base = inc / 300;
  m_pcr_inc_ext = inc % 300;
}

void AMTsMuxerBase::check_storage_free_space()
{
  uint64_t free_space = 0;
  struct statfs disk_statfs;
  if(statfs(m_file_location.c_str(), &disk_statfs) < 0) {
    PERROR("statfs");
    ERROR("%s stafs error in %s.", m_file_location.c_str(),
          m_muxer_name.c_str());
  } else {
    free_space = ((uint64_t)disk_statfs.f_bsize *
        (uint64_t)disk_statfs.f_bfree) / (uint64_t)(1024 * 1024);
    DEBUG("Free space is %llu M in %s", free_space, m_muxer_name.c_str());
    if(AM_UNLIKELY(free_space <=
                   m_curr_config.smallest_free_space)) {
      ERROR("The free space is smaller than %d M in %s, "
          "will stop writing data to ts file and close it ",
          m_curr_config.smallest_free_space, m_muxer_name.c_str());
      m_file_writing = false;
      if (m_file_writer) {
        if (m_file_writer->close_file(m_curr_config.write_sync_enable)
            != AM_STATE_OK) {
          ERROR("Failed to close file %s in %s.",
                m_file_writer->get_current_file_name(),
                m_muxer_name.c_str());
        }
      }
    }
  }
}

bool AMTsMuxerBase::check_pcr_overflow(AMPacket* packet)
{
  bool ret = true;
  do {
    if ((m_pcr_base >= (AM_PTS) ((1LL << 33) - (m_video_info.scale << 1)))
        && ((packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)
            && (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR))) {
      if (AM_UNLIKELY(on_eof_packet(packet) != AM_STATE_OK)) {
        ERROR("On eof packet error.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
        ERROR("Build pat pmt error in ts muxer.");
        ret = AM_STATE_ERROR;
        break;
      }
      packet->add_ref();
      m_packet_queue->push_back(packet);
    }
  } while(0);
  return ret;
}

void AMTsMuxerBase::find_adts(AMPacket* packet)
{
  m_adts.clear();
  uint8_t *bs = packet->get_data_ptr();
  for (uint32_t i = 0; i < packet->get_frame_count(); ++ i) {
    ADTS adts;
    adts.addr = bs;
    adts.size = ((AdtsHeader*)bs)->frame_length();
    bs += adts.size;
    m_adts.push_back(adts);
  }
}

bool AMTsMuxerBase::create_new_file()
{
  bool ret = true;
  std::string file_name;
  do {
    clear_params_for_new_file();
    update_config_param();
    if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
      ERROR("Generate file name error in %s.", m_muxer_name.c_str());
      ret = false;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name.c_str())
                    != AM_STATE_OK)) {
      ERROR("Failed to set file name to m_file_writer in %s!",
            m_muxer_name.c_str());
      ret = false;
      break;
    }
    if (m_file_writer->create_next_file() != AM_STATE_OK) {
      ERROR("Failed to create next file in %s", m_muxer_name.c_str());
      ret = false;
      break;
    }
    m_file_counter = 1;
    if (AM_UNLIKELY(build_and_write_pat_pmt() != AM_STATE_OK)) {
      ERROR("Build pat pmt error in ts muxer.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMTsMuxerBase::set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
                                                AMFileOperationCB callback)
{
  bool ret = true;
  switch (type) {
    case AM_OPERATION_CB_FILE_FINISH : {
      m_file_finish_cb = callback;
      if (m_file_writer) {
        if (!m_file_writer->set_file_operation_cb(type, callback)) {
          ERROR("Failed to set file operation callback function to file writer"
              "in %s", m_muxer_name.c_str());
          ret = false;
          break;
        }
      }
    } break;
    case AM_OPERATION_CB_FILE_CREATE : {
      m_file_create_cb = callback;
      if (m_file_writer) {
        if (!m_file_writer->set_file_operation_cb(type, callback)) {
          ERROR("Failed to set file operation callback function to file writer"
              "in %s", m_muxer_name.c_str());
          ret = false;
          break;
        }
      }
    } break;
    default : {
      ERROR("File operation callback type error.");
      ret = false;
    } break;
  }
  return ret;
}

bool AMTsMuxerBase::set_muxer_param(AMMuxerParam &param)
{
  bool ret = true;
  do {
    if ((param.muxer_id_bit_map & (0x00000001 << m_curr_config.muxer_id)) == 0) {
      break;
    }
    AUTO_MEM_LOCK(m_param_set_lock);
    if (param.file_duration_int32.is_set) {
      if (!m_need_splitted) {
        ERROR("The file split flag is false, can not set file duration "
            "parameter in %s, this param will be ignored.", m_muxer_name.c_str());
        break;
      }
      m_set_config.file_duration = param.file_duration_int32.value.v_int32;
      INFO("Set file duration : %d in %s", m_set_config.file_duration,
           m_muxer_name.c_str());
      if ((m_set_config.file_duration <= 0) && (m_need_splitted)) {
        m_need_splitted = false;
      }
      if (param.save_to_config_file) {
        m_muxer_ts_config->file_duration = m_set_config.file_duration;
      }
    }

    if (param.recording_file_num_u32.is_set) {
      if (m_muxer_ts_config->muxer_attr == AM_MUXER_FILE_EVENT) {
        ERROR("Set recording_file_num is not supported in %s, this parameter"
            "will be ignored", m_muxer_name.c_str());
        break;
      }
      m_set_config.recording_file_num = param.recording_file_num_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_ts_config->recording_file_num = m_set_config.recording_file_num;
      }
      m_file_counter = 0;
      INFO("Set recording file number : %u in %s",
           m_set_config.recording_file_num, m_muxer_name.c_str());
    }

    if (param.recording_duration_u32.is_set) {
      if (m_muxer_ts_config->muxer_attr == AM_MUXER_FILE_EVENT) {
        ERROR("Set recording_duration is not supported in %s, this parameter"
            "will be ignored", m_muxer_name.c_str());
        break;
      }
      m_set_config.recording_duration = param.recording_duration_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_ts_config->recording_duration = m_set_config.recording_duration;
      }
      INFO("Set recording duration : %u in %s", m_set_config.recording_duration,
           m_muxer_name.c_str());
    }

    if (param.digital_sig_enable_bool.is_set) {
      ERROR("The digital signature enable is not supported in %s,"
          "this parameter will be ignored.", m_muxer_name.c_str());
    }

    if (param.gsensor_enable_bool.is_set) {
      ERROR("The gsensor enable is not supported in %s, this parameter will"
          "be ignored.", m_muxer_name.c_str());
    }

    if (param.reconstruct_enable_bool.is_set) {
      ERROR("Reconstruct function is not supported in %s, this parameter"
          "will be ignored", m_muxer_name.c_str());
    }

    if (param.max_file_size_u32.is_set) {
      m_set_config.max_file_size = param.max_file_size_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_ts_config->max_file_size = m_set_config.max_file_size;
      }
      INFO("Set max file size : %u in %s", m_set_config.max_file_size,
           m_muxer_name.c_str());
    }

    if (param.write_sync_enable_bool.is_set) {
      m_set_config.write_sync_enable = param.write_sync_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_ts_config->write_sync_enable = m_set_config.write_sync_enable;
      }
      INFO("%s write sync flag in %s", m_set_config.write_sync_enable ?
          "Enable" : "Disable", m_muxer_name.c_str());
    }

    if (param.hls_enable_bool.is_set) {
      m_set_config.hls_enable = param.hls_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_ts_config->hls_enable = param.hls_enable_bool.value.v_bool;
      }
      INFO("%s hls in %s", m_set_config.hls_enable ? "Enable" : "Disable",
          m_muxer_name.c_str());
    }

    if (param.scramble_enable_bool.is_set) {
      ERROR("The scramble enable flag is not supported in %s", m_muxer_name.c_str());
    }

    if (param.save_to_config_file) {
      INFO("Save to config file in %s", m_muxer_name.c_str());
      if (AM_UNLIKELY(!m_config->set_config(m_muxer_ts_config))) {
        ERROR("Failed to save param into config file in %s.", m_muxer_name.c_str());
        ret = false;
        break;
      }
    }

  } while(0);
  return ret;
}

void AMTsMuxerBase::update_config_param()
{
  AUTO_MEM_LOCK(m_param_set_lock);
  m_curr_config = m_set_config;
}
