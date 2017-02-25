/*******************************************************************************
 * am_muxer_mp4_base.cpp
 *
 * History:
 *   2015-12-28 - [ccjing] created file
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
#include "am_muxer_mp4_builder.h"
#include "am_muxer_mp4_file_writer.h"
#include "am_muxer_mp4_base.h"

std::string AMMuxerMp4Base::audio_type_to_string(AM_AUDIO_TYPE type)
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

AMMuxerMp4Base::AMMuxerMp4Base()
{
}

AM_STATE AMMuxerMp4Base::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(config_file == NULL)) {
      ERROR("config_file is NULL, should input valid config_file");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config_file = amstrdup(config_file);
    if (AM_UNLIKELY((m_config = new AMMuxerMp4Config()) == NULL)) {
      ERROR("Failed to create MP4 config class.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_muxer_mp4_config = m_config->get_config(std::string(m_config_file));
    if (AM_UNLIKELY(!m_muxer_mp4_config)) {
      ERROR("Failed to get config");
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_muxer_mp4_config->hls_enable) {
      if (m_muxer_mp4_config->file_duration >= 300) {
        WARN("HLS is enabled and the file duration is larger than 300s, "
            "reset it to 300s.");
        m_muxer_mp4_config->file_duration = 300;
      }
    }
    m_curr_config = *m_muxer_mp4_config;
    m_set_config = m_curr_config;
    m_need_splitted = (m_curr_config.file_duration > 0);
    m_file_writing = m_curr_config.auto_file_writing;
    if (m_curr_config.audio_sample_rate > 0) {
      m_audio_sample_rate_set = true;
    } else {
      m_audio_sample_rate_set = false;
    }
  } while (0);
  return ret;
}

AMMuxerMp4Base::~AMMuxerMp4Base()
{
  INFO("Begin to destroy %s", m_muxer_name.c_str());
  stop();
  delete m_config;
  delete m_config_file;
  INFO("%s is destroyed successfully.");
}

AM_STATE AMMuxerMp4Base::start()
{
  INFO("Begin to start %s", m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_interface_lock);
  AM_STATE ret = AM_STATE_OK;
  do {
    bool need_break = false;
    switch (get_state()) {
      case AM_MUXER_CODEC_RUNNING: {
        NOTICE("The %s is running already.", m_muxer_name.c_str());
        need_break = true;
      } break;
      case AM_MUXER_CODEC_ERROR: {
        NOTICE("%s state is error! Need to be re-created!",
               m_muxer_name.c_str());
        need_break = true;
      } break;
      default:
        break;
    }
    if (AM_UNLIKELY(need_break)) {
      break;
    }
    AM_DESTROY(m_thread);
    if (AM_LIKELY(!m_thread)) {
      m_thread = AMThread::create(m_muxer_name.c_str(), thread_entry, this);
      if (AM_UNLIKELY(!m_thread)) {
        ERROR("Failed to create thread: %s", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    while ((get_state() == AM_MUXER_CODEC_STOPPED)
        || (get_state() == AM_MUXER_CODEC_INIT)) {
      usleep(5000);
    }
    if (get_state() == AM_MUXER_CODEC_RUNNING) {
      NOTICE("Start %s success", m_muxer_name.c_str());
    } else {
      ERROR("Failed to start %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerMp4Base::get_proper_file_location(std::string& file_location)
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
              "or usb strorage, %s will auto parse a proper file location",
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

void AMMuxerMp4Base::clear_all_params()
{
  INFO("Begin to clear all params in %s", m_muxer_name.c_str());
  memset(&m_audio_info, 0, sizeof(AM_AUDIO_INFO));
  memset(&m_video_info, 0, sizeof(AM_VIDEO_INFO));
  m_file_location.clear();
  while(!m_packet_queue.empty()) {
    m_packet_queue.front_and_pop()->release();
  }
  AM_DESTROY(m_mp4_builder);
  AM_DESTROY(m_file_writer);
  m_last_video_pts = 0;
  m_first_video_pts = 0;
  m_eos_map = 0;
  m_av_info_map = 0;
  m_video_frame_count = 0;
  m_last_frame_number = 0;
  m_is_audio_accepted = false;
  m_is_video_arrived = false;
  m_is_first_video = true;
  m_file_counter = 0;
  INFO("Clear all params in %s success", m_muxer_name.c_str());
}

AM_MUXER_CODEC_STATE AMMuxerMp4Base::create_resource()
{
  std::string file_name;
  INFO("Begin to create resource in %s.", m_muxer_name.c_str());
  AM_MUXER_CODEC_STATE ret = AM_MUXER_CODEC_RUNNING;
  do {
    clear_all_params();
    if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
      ERROR("%s generate file name error, exit main loop.",
            m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    AM_DESTROY(m_file_writer);
    if (AM_UNLIKELY((m_file_writer = AMMp4FileWriter::create (
        &m_curr_config, &m_video_info)) == nullptr)) {
      ERROR("Failed to create m_file_writer in %s!",  m_muxer_name.c_str());
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
    AM_DESTROY(m_mp4_builder);
    if (AM_UNLIKELY((m_mp4_builder = AMMuxerMp4Builder::create(
        m_file_writer, &m_curr_config)) == nullptr)) {
      ERROR("Failde to create MP4 builder in %s.", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name.c_str())
                    != AM_STATE_OK)) {
      ERROR("Failed to set media sink for m_file_writer in %s.",
            m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    INFO("%s create resource success.", m_muxer_name.c_str());
  } while (0);
  AUTO_MEM_LOCK(m_state_lock);
  m_state = ret;
  return ret;
}

AM_STATE AMMuxerMp4Base::stop()
{
  INFO("Stopping %s", m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_interface_lock);
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("Stop %s successfully.",  m_muxer_name.c_str());
  return AM_STATE_OK;
}

const char* AMMuxerMp4Base::name()
{
  return m_muxer_name.c_str();
}

void* AMMuxerMp4Base::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIFileOperation) ? ((AMIFileOperation*)this) :
            ((refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
                ((void*)nullptr));
}

bool AMMuxerMp4Base::stop_file_writing()
{
  bool ret = true;
  INFO("Stopping file writing in %s.", m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_file_param_lock);
    if (!m_file_writing) {
      NOTICE("File writing is already stopped in %s", m_muxer_name.c_str());
      break;
    }
    m_file_writing = false;
    m_file_counter = 0;
    m_stop_recording_pts = 0;
    m_param_set_lock.lock();
    m_set_config.recording_duration = 0;
    m_set_config.recording_file_num = 0;
    m_param_set_lock.unlock();
    if (m_file_writer && m_file_writer->is_file_open()) {
      if (m_mp4_builder) {
        if (AM_UNLIKELY(m_mp4_builder->end_file() != AM_STATE_OK)) {
          ERROR("Failed to end MP4 file in %s.", m_muxer_name.c_str());
          ret = false;
          break;
        }
        m_mp4_builder->clear_video_data();
      }
      if (m_file_writer) {
        if (m_file_writer->close_file(m_curr_config.write_sync_enable)
            != AM_STATE_OK) {
          ERROR("Failed to close file %s in %s.",
                m_file_writer->get_current_file_name(),
                m_muxer_name.c_str());
          ret = false;
          break;
        }
      }
    }
    clear_params_for_new_file();
    NOTICE("Stop file writing success in %s.", m_muxer_name.c_str());
  } while (0);
  return ret;
}

bool AMMuxerMp4Base::set_recording_file_num(uint32_t file_num)
{
  NOTICE("Set recording file num %u success in %s", file_num,
         m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_param_set_lock);
  m_set_config.recording_file_num = file_num;
  m_file_counter = 0;//reset the file counter.
  return true;
}

bool AMMuxerMp4Base::set_recording_duration(int32_t duration)
{
  NOTICE("Set recording duration %u seconds in %s", duration,
         m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_param_set_lock);
  m_set_config.recording_duration = duration;
  return true;
}

bool AMMuxerMp4Base::set_file_duration(int32_t file_duration,
                                       bool apply_conf_file)
{
  bool ret = true;
  NOTICE("Set file duration %u seconds in %s", file_duration,
         m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_param_set_lock);
    m_set_config.file_duration = file_duration;
    if ((file_duration > 0) && (!m_need_splitted)) {
      m_need_splitted = true;
      m_curr_file_boundary = 0;
    }
    if ((file_duration <= 0) && (m_need_splitted)) {
      m_need_splitted = false;
    }
    if (apply_conf_file) {
      m_muxer_mp4_config->file_duration = file_duration;
      if (AM_UNLIKELY(!m_config->set_config(m_muxer_mp4_config))) {
        ERROR("Failed to set mp4 config file.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMMuxerMp4Base::set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
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

bool AMMuxerMp4Base::is_running()
{
  return m_run.load();
}

void AMMuxerMp4Base::release_resource()
{
  INFO("Begin to release resource in %s", m_muxer_name.c_str());
  if (m_file_writer && m_mp4_builder) {
    if (m_file_writer->is_file_open()) {
      m_mp4_builder->end_file();
      m_file_writer->close_file(m_curr_config.write_sync_enable);
    }
  }
  AM_DESTROY(m_mp4_builder);
  AM_DESTROY(m_file_writer);
  m_file_create_cb = nullptr;
  m_file_finish_cb = nullptr;
  m_stop_recording_pts = 0;
  m_set_config.recording_duration = 0;
  m_set_config.recording_file_num = 0;
  while (!(m_packet_queue.empty())) {
    m_packet_queue.front_and_pop()->release();
  }
  NOTICE("Release resource success in %s.", m_muxer_name.c_str());
}

uint8_t AMMuxerMp4Base::get_muxer_codec_stream_id()
{
  return (uint8_t)((0x01) << (m_curr_config.video_id));
}

uint32_t AMMuxerMp4Base::get_muxer_id()
{
  return m_curr_config.muxer_id;
}

AM_MUXER_CODEC_STATE AMMuxerMp4Base::get_state()
{
  AUTO_MEM_LOCK(m_state_lock);
  return m_state;
}

AM_STATE AMMuxerMp4Base::set_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMMuxerMp4Base::get_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

void AMMuxerMp4Base::thread_entry(void* p)
{
  ((AMMuxerMp4Base*) p)->main_loop();
}

bool AMMuxerMp4Base::get_current_time_string(char *time_str, int32_t len)
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

#define DELIMITER_SIZE 3
bool AMMuxerMp4Base::is_h265_IDR_first_nalu(AMPacket* packet)
{
  bool ret = false;
  uint8_t *data = packet->get_data_ptr();
  uint32_t len = packet->get_data_size();
  do {
    if (AM_LIKELY(data && (len > 2 * DELIMITER_SIZE))) {
      uint8_t *last_header = data + len - 2 * DELIMITER_SIZE;
      while (data <= last_header) {
        if (AM_UNLIKELY((0x00000001
            == (0 | (data[0] << 16) | (data[1] << 8) | data[2]))
            && (((int32_t )((0x7E & data[3]) >> 1) == H265_IDR_W_RADL)
                || ((int32_t )((0x7E & data[3]) >> 1) == H265_IDR_N_LP)))) {
          data += DELIMITER_SIZE;
          if (((data[2] & 0x80) >> 7) == 1) {
            ret = true;
            break;
          } else {
            ret = false;
            break;
          }
        } else if (data[2] != 0) {
          data += 3;
        } else if (data[1] != 0) {
          data += 2;
        } else {
          data += 1;
        }
      }
    } else {
      if (AM_LIKELY(!data)) {
        ERROR("Invalid bit stream!");
        ret = false;
        break;
      }
      if (AM_LIKELY(len <= DELIMITER_SIZE)) {
        ERROR("Bit stream is less equal than %d bytes!", DELIMITER_SIZE);
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Base::on_eof_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      if ((ret = m_mp4_builder->end_file()) != AM_STATE_OK) {
        ERROR("End file error in %s.", m_muxer_name.c_str());
      }
      if ((ret = m_file_writer->close_file(m_curr_config.write_sync_enable))
          != AM_STATE_OK) {
        ERROR("Failed to close file %s in %s.",
              m_file_writer->get_current_file_name(), m_muxer_name.c_str());
        break;
      }
      if ((m_curr_config.recording_file_num > 0) &&
          (m_file_counter >= m_curr_config.recording_file_num)) {
        INFO("File number is equal to  the value of m_max_file_num_per_recording,"
            " stop file writing in %s.", m_muxer_name.c_str());
        m_file_writing = false;
        m_file_counter = 0;
        m_stop_recording_pts = 0;
        m_set_config.recording_duration = 0;
        m_set_config.recording_file_num = 0;
        if (m_mp4_builder) {
          m_mp4_builder->clear_video_data();
        }
        break;
      }
      update_config_param();
      if ((ret = m_file_writer->create_next_file()) != AM_STATE_OK) {
        ERROR("Failed to create file %s in %s.",
              m_file_writer->get_current_file_name(), m_muxer_name.c_str());
        break;
      }
      ++ m_file_counter;
      if((ret = m_mp4_builder->begin_file()) != AM_STATE_OK) {
        ERROR("Failed to start building file in %s.", m_muxer_name.c_str());
        break;
      }
    } else {
      ERROR("Currently, just support video packet in on_eof function in %s.",
            m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  clear_params_for_new_file();
  return ret;
}

AM_STATE AMMuxerMp4Base::on_eos_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!m_file_writing) {
      break;
    }
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
      NOTICE("audio eos is received in %s.", m_muxer_name.c_str());
      m_eos_map |= 1 << 0;
    } else if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      NOTICE("video eos is received in %s.", m_muxer_name.c_str());
      m_eos_map |= 1 << 1;
    } else {
      ERROR("Currently %s just support audio and video eos packet.",
            m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_eos_map == 0x03) {
      if ((ret = m_mp4_builder->end_file()) != AM_STATE_OK) {
        ERROR("End file error in %s.", m_muxer_name.c_str());
      }
      if ((ret = m_file_writer->close_file(m_curr_config.write_sync_enable))
          != AM_STATE_OK) {
        ERROR("Failed to close file %s in %s.",
              m_file_writer->get_current_file_name(),
              m_muxer_name.c_str());
      }
      m_run = false;
      NOTICE("receive both audio and video eos in %s, exit the main loop",
             m_muxer_name.c_str());
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Base::write_video_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (((m_av_info_map >> 1) & 0x01) != 0x01) {
      INFO("Video info packet has not been received in %s, av_info_map is %u"
           "drop this data packet!", m_muxer_name.c_str(), m_av_info_map);
      break;
    }
    if (!m_file_writer->is_file_open()) {
      std::string file_name;
      file_name.clear();
      clear_params_for_new_file();
      if (m_file_writer) {
        if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
          ERROR("Generate file name error in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name.c_str())
                        != AM_STATE_OK)) {
          ERROR("Failed to set file name to m_file_writer in %s!",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
      }
      update_config_param();
      if (m_file_writer) {
        if (AM_UNLIKELY(m_file_writer->create_next_file() != AM_STATE_OK)) {
          ERROR("Failed to create next new file in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_file_counter = 1;
      } else {
        ERROR("The file writer is not created in %s", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      if (m_mp4_builder) {
        if (AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
          ERROR("Failed to start building file in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
      } else {
        ERROR("The file builder is not created in %s", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(m_is_first_video)) {
      /*first frame must be video frame,
       * and the first video frame must be I frame*/
      if (m_video_info.type == AM_VIDEO_H264) {
        if (AM_UNLIKELY(packet->get_frame_type() != AM_VIDEO_FRAME_TYPE_IDR)) {
          INFO("First frame must be video IDR frame, "
               "drop this packet in %s", m_muxer_name.c_str());
          break;
        }
      } else if (m_video_info.type == AM_VIDEO_H265) {
        if (AM_UNLIKELY(!((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR)
            && is_h265_IDR_first_nalu(packet)))) {
          INFO("First frame must be video hevc IDR frame start, "
               "drop this packet in %s", m_muxer_name.c_str());
          break;
        }
      } else {
        ERROR("video type error in %s.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      m_file_writer->set_begin_packet_pts(packet->get_pts());
      if ((m_curr_config.recording_duration > 0) && (m_stop_recording_pts == 0)) {
        m_stop_recording_pts = packet->get_pts() +
            (int64_t)(m_curr_config.recording_duration * PTS_TIME_FREQUENCY);
      }
      m_is_first_video = false;
      m_is_video_arrived = true;
      m_curr_file_boundary = packet->get_pts() +
          (int64_t)m_curr_config.file_duration * PTS_TIME_FREQUENCY
          - m_frame_pts_diff;
      m_first_video_pts = packet->get_pts();
    } else {
      m_file_writer->set_end_packet_pts(packet->get_pts());
    }

    if (AM_UNLIKELY((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR)
        && ((m_file_writer->get_file_offset() >=
             m_curr_config.max_file_size * 1024 * 1024) ||
            (m_need_splitted &&
            (packet->get_pts() >= m_curr_file_boundary))))) {
      /*
       * if (m_need_splitted && reached file_boundary && m_video_frame_count > 0)
       *  close the last file and create a new file
       */
      if (m_need_splitted && (packet->get_pts() >= m_curr_file_boundary)) {
        if (m_video_frame_count == 0) {
          NOTICE("Data packet pts in %s reach the file bounday,"
                 "but the video frame count is zero, update file boundary, "
                 "continue writing.", m_muxer_name.c_str());
          m_curr_file_boundary = packet->get_pts() +
                   (int64_t)m_curr_config.file_duration * PTS_TIME_FREQUENCY
                   - m_frame_pts_diff;
        } else {
          if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
            ERROR("On eof error in %s.", m_muxer_name.c_str());
            break;
          }
        }
      }
      if (m_file_writer->get_file_offset() >=
          m_curr_config.max_file_size * 1024 * 1024) {
        NOTICE("The file size reach max file size %u MB, close current file and "
            "create a new file", m_curr_config.max_file_size);
        if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
          ERROR("On eof error in %s.", m_muxer_name.c_str());
          break;
        }
      }
      /* This packet should be written into next file,
       * push it back into the front of the queue
       */
      packet->add_ref();
      m_packet_queue.push_front(packet);
      break;
    }
    if ((m_stop_recording_pts > 0) && (packet->get_pts() >= m_stop_recording_pts)) {
      NOTICE("Packet pts reach stop recording pts, stop file writing in %s",
             m_muxer_name.c_str());
      m_file_writing = false;
      m_file_counter = 0;
      m_stop_recording_pts = 0;
      m_set_config.recording_duration = 0;
      m_set_config.recording_file_num = 0;
      if (AM_UNLIKELY(m_mp4_builder->end_file() != AM_STATE_OK)) {
        ERROR("Failed to end MP4 file in %s.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
      }
      m_mp4_builder->clear_video_data();
      if (m_file_writer->close_file(m_curr_config.write_sync_enable)
          != AM_STATE_OK) {
        ERROR("Failed to close file %s in %s.",
              m_file_writer->get_current_file_name(),
              m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
      }
      clear_params_for_new_file();
      break;
    }
    if (AM_UNLIKELY(((ret = m_mp4_builder->write_video_data(packet)))
        != AM_STATE_OK)) {
      ERROR("Failed to write video data in %s.", m_muxer_name.c_str());
      break;
    }
    ++ m_video_frame_count;
    m_last_video_pts = packet->get_pts();
    m_last_frame_number = packet->get_frame_number();
  } while(0);
  return ret;
}

AM_STATE AMMuxerMp4Base::write_audio_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!m_file_writing) {
      break;
    }
    if (AM_UNLIKELY(!m_is_video_arrived)) {
      DEBUG("Audio packet is arrived before video coming in %s, drop it",
            m_muxer_name.c_str());
      break;
    }
    if ((m_av_info_map & 0x01) == 0) {
      INFO("%s has not received audio info packet,av_info_map is %u,"
          " drop current audio data pkt.", m_muxer_name.c_str(), m_av_info_map);
      break;
    }
    if (packet->get_pts() < m_first_video_pts) {
      INFO("The first audio pts is smaller than first video pts, drop it.");
      break;
    }
    if (AM_UNLIKELY(!m_is_audio_accepted)) {
      NOTICE("Audio is not accepted in %s!", m_muxer_name.c_str());
      int32_t pts_diff_frame = m_audio_info.pkt_pts_increment
          / packet->get_frame_count();
      int32_t pts_diff = packet->get_pts() - m_first_video_pts;
      int32_t frame_number_tmp = packet->get_frame_count()
                                      - (pts_diff / pts_diff_frame) - 1;
      uint32_t frame_number = (frame_number_tmp >= 0) ?
          (uint32_t) frame_number_tmp : 0;
      m_is_audio_accepted = true;
      NOTICE("Audio is accepted: first frame number in "
          "packet is %u in %s, PTS: %lld",
          frame_number,
          m_muxer_name.c_str(),
          (packet->get_pts() -
              ((packet->get_frame_count() - frame_number - 1) *
                  (m_audio_info.pkt_pts_increment / packet->get_frame_count()))));
      /*frame_number in write_audio_data func count from 1 2 ...*/
      if (AM_UNLIKELY((ret = m_mp4_builder->write_audio_data(packet,
                                                             frame_number + 1))
                      != AM_STATE_OK)) {
        ERROR("Failed to write audio data to file in %s.",
              m_muxer_name.c_str());
      }
      break;
    }
    if (AM_UNLIKELY((ret = m_mp4_builder->write_audio_data(packet))
        != AM_STATE_OK)) {
      ERROR("Failed to write audio data to file in %s.", m_muxer_name.c_str());
      break;
    }
  } while(0);
  return ret;
}

void AMMuxerMp4Base::check_storage_free_space()
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
          "will stop writing data to MP4 file and close it ",
          m_curr_config.smallest_free_space, m_muxer_name.c_str());
      m_file_writing = false;
      if (m_mp4_builder) {
        if (AM_UNLIKELY(m_mp4_builder->end_file() != AM_STATE_OK)) {
          ERROR("Failed to end MP4 file in %s.", m_muxer_name.c_str());
        }
      }
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

void AMMuxerMp4Base::update_config_param()
{
  AUTO_MEM_LOCK(m_param_set_lock);
  m_curr_config = m_set_config;
}

bool AMMuxerMp4Base::set_muxer_param(AMMuxerParam &param)
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
        m_muxer_mp4_config->file_duration = m_set_config.file_duration;
      }
    }

    if (param.recording_file_num_u32.is_set) {
      if (m_muxer_mp4_config->muxer_attr == AM_MUXER_FILE_EVENT) {
        ERROR("Set recording_file_num is not supported in %s, this parameter"
            "will be ignored", m_muxer_name.c_str());
        break;
      }
      m_set_config.recording_file_num = param.recording_file_num_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->recording_file_num = m_set_config.recording_file_num;
      }
      m_file_counter = 0;
      INFO("Set recording file number : %u in %s",
           m_set_config.recording_file_num, m_muxer_name.c_str());
    }

    if (param.recording_duration_u32.is_set) {
      if (m_muxer_mp4_config->muxer_attr == AM_MUXER_FILE_EVENT) {
        ERROR("Set recording_duration is not supported in %s, this parameter"
            "will be ignored", m_muxer_name.c_str());
        break;
      }
      m_set_config.recording_duration = param.recording_duration_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->recording_duration = m_set_config.recording_duration;
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
      m_set_config.reconstruct_enable = param.reconstruct_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->reconstruct_enable =
            param.reconstruct_enable_bool.value.v_bool;
      }
      INFO("%s reconstruct in %s", m_set_config.reconstruct_enable ?
          "Enable" : "Disable", m_muxer_name.c_str());
    }

    if (param.max_file_size_u32.is_set) {
      m_set_config.max_file_size = param.max_file_size_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->max_file_size = m_set_config.max_file_size;
      }
      INFO("Set max file size : %u in %s", m_set_config.max_file_size,
           m_muxer_name.c_str());
    }

    if (param.write_sync_enable_bool.is_set) {
      m_set_config.write_sync_enable = param.write_sync_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->write_sync_enable = m_set_config.write_sync_enable;
      }
      INFO("%s write sync flag in %s", m_set_config.write_sync_enable ?
           "Enable" : "Disable", m_muxer_name.c_str());
    }

    if (param.hls_enable_bool.is_set) {
      m_set_config.hls_enable = param.hls_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->hls_enable = param.hls_enable_bool.value.v_bool;
      }
      INFO("%s hls in %s", m_set_config.hls_enable ? "Enable" : "Disable",
           m_muxer_name.c_str());
    }

    if (param.scramble_enable_bool.is_set) {
      ERROR("The scramble enable flag is not supported in %s", m_muxer_name.c_str());
    }

    if (param.save_to_config_file) {
      INFO("Save to config file in %s", m_muxer_name.c_str());
      if (AM_UNLIKELY(!m_config->set_config(m_muxer_mp4_config))) {
        ERROR("Failed to save param into config file in %s", m_muxer_name.c_str());
        ret = false;
        break;
      }
    }

  } while(0);
  return ret;
}

