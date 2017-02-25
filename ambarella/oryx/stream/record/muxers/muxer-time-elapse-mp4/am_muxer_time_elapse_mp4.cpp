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
#include "am_muxer_time_elapse_mp4.h"
#include "am_time_elapse_mp4_builder.h"
#include "am_time_elapse_mp4_file_writer.h"

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerTimeElapseMp4::create(config_file));
}

AMMuxerTimeElapseMp4* AMMuxerTimeElapseMp4::create(const char* config_file)
{
  AMMuxerTimeElapseMp4* result = new AMMuxerTimeElapseMp4();
  if (result && result->init(config_file) != AM_STATE_OK) {
    ERROR("Failed to create Muxer Time Elapse Mp4.");
    delete result;
    result = nullptr;
  }
  return result;
}

AM_STATE AMMuxerTimeElapseMp4::start()
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

const char* AMMuxerTimeElapseMp4::name()
{
  return m_muxer_name.c_str();
}


void* AMMuxerTimeElapseMp4::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIFileOperation) ? ((AMIFileOperation*)this) :
            ((refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
                ((void*)nullptr));
}

AM_STATE AMMuxerTimeElapseMp4::stop()
{
  INFO("Stopping %s", m_muxer_name.c_str());
  AUTO_MEM_LOCK(m_interface_lock);
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("Stop %s success.", m_muxer_name.c_str());
  return AM_STATE_OK;
}

bool AMMuxerTimeElapseMp4::start_file_writing()
{
  bool ret = true;
  char file_name[strlen(m_curr_config.file_location.c_str())
                 + strlen(m_curr_config.file_name_prefix.c_str()) + 128];
  memset(file_name, 0, sizeof(file_name));
  INFO("Begin to start file writing in %s.", m_muxer_name.c_str());
  do {
    if (m_file_writing) {
      NOTICE("File writing is already started in %s.", m_muxer_name.c_str());
      break;
    }
    clear_params_for_new_file();
    if (m_file_writer) {
      if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
        ERROR("Generate file name error in %s.", m_muxer_name.c_str());
        ret = false;
        break;
      }
      if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name)
                      != AM_STATE_OK)) {
        ERROR("Failed to set file name to m_file_writer in %s!",
              m_muxer_name.c_str());
        ret = false;
        break;
      }
    }
    update_config_param();
    if (m_file_writer) {
      if (AM_UNLIKELY(m_file_writer->create_next_file() != AM_STATE_OK)) {
        ERROR("Failed to create next new file in %s.", m_muxer_name.c_str());
        ret = false;
        break;
      }
    } else {
      ERROR("The file writer is not created in %s", m_muxer_name.c_str());
      ret = false;
      break;
    }
    if (m_mp4_builder) {
      if (AM_UNLIKELY(m_mp4_builder->begin_file() != AM_STATE_OK)) {
        ERROR("Failed to start building file in %s.", m_muxer_name.c_str());
        ret = false;
        break;
      }
    } else {
      ERROR("The builder is not created in %s", m_muxer_name.c_str());
      ret = false;
      break;
    }
    m_file_writing = true;
  } while (0);
  if (!ret) {
    NOTICE("start file writing success in %s.", m_muxer_name.c_str());
  }
  return ret;
}

bool AMMuxerTimeElapseMp4::stop_file_writing()
{
  bool ret = true;
  INFO("Stopping file writing in %s.", m_muxer_name.c_str());
  do {
    AUTO_MEM_LOCK(m_file_writing_lock);
    if (!m_file_writing) {
      NOTICE("File writing is already stopped in %s", m_muxer_name.c_str());
      break;
    }
    m_file_writing = false;
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
    NOTICE("Stop file writing success in %s.", m_muxer_name.c_str());
  } while (0);
  return ret;
}

bool AMMuxerTimeElapseMp4::set_recording_file_num(uint32_t file_num)
{
  ERROR("Should not set recording file number in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerTimeElapseMp4::set_recording_duration(int32_t duration)
{
  ERROR("Should not set recording duration in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerTimeElapseMp4::set_file_duration(int32_t file_duration,
                                             bool apply_conf_file)
{
  ERROR("Should not set file duration in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerTimeElapseMp4::set_file_operation_callback(
    AM_FILE_OPERATION_CB_TYPE type, AMFileOperationCB callback)
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

bool AMMuxerTimeElapseMp4::is_running()
{
  return m_run.load();
}

AM_MUXER_CODEC_STATE AMMuxerTimeElapseMp4::get_state()
{
  AUTO_MEM_LOCK(m_state_lock);
  return m_state;
}

AM_MUXER_ATTR AMMuxerTimeElapseMp4::get_muxer_attr()
{
  return AM_MUXER_FILE_NORMAL;
}

uint8_t AMMuxerTimeElapseMp4::get_muxer_codec_stream_id()
{
  return (uint8_t) ((0x01) << (m_curr_config.video_id));
}

uint32_t AMMuxerTimeElapseMp4::get_muxer_id()
{
  return m_curr_config.muxer_id;
}

void AMMuxerTimeElapseMp4::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL) == 0) {
      /*just need normal packet*/
      break;
    }
    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS)
        || (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO)) {
      /*info and eos packet*/
      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) {
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
        } else {
          if (packet->get_stream_id() == m_curr_config.video_id) {
            add = true;
          }
        }
      }
    } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) {
      /*video data packet*/
      if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
        AM_VIDEO_TYPE vtype = AM_VIDEO_TYPE(packet->get_video_type());
        add = (packet->get_stream_id() == m_curr_config.video_id)
              && ((vtype == AM_VIDEO_H264) || (vtype == AM_VIDEO_H265));
      }
    }
  } while (0);
  if (AM_LIKELY(add)) {
    packet->add_ref();
    m_packet_queue->push_back(packet);
  }
  packet->release();
}

AM_STATE AMMuxerTimeElapseMp4::set_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMMuxerTimeElapseMp4::get_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

AMMuxerTimeElapseMp4::AMMuxerTimeElapseMp4()
{}

AM_STATE AMMuxerTimeElapseMp4::init(const char* config_file)
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
    if (AM_UNLIKELY((m_config = new AMMuxerTimeElapseMp4Config()) == NULL)) {
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
    m_file_writing = m_curr_config.auto_file_writing;
    m_muxer_name = "TimeElapseMp4";
    INFO("Init %s success.", m_muxer_name.c_str());
  } while (0);
  return ret;
}

AMMuxerTimeElapseMp4::~AMMuxerTimeElapseMp4()
{
  INFO("Begin to destroy %s", m_muxer_name.c_str());
  stop();
  delete m_config;
  delete m_packet_queue;
  delete m_config_file;
  INFO("%s is destroyed success.");
}

void AMMuxerTimeElapseMp4::thread_entry(void* p)
{
  ((AMMuxerTimeElapseMp4*)p)->main_loop();
}

void AMMuxerTimeElapseMp4::main_loop()
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
  while (m_run && is_ok) {
    AMPacket* packet = NULL;
    if (m_packet_queue->empty()) {
      DEBUG("Packet queue is empty, sleep 100 ms.");
      usleep(100000);
      continue;
    }
    packet = m_packet_queue->front();
    m_packet_queue->pop_front();
    AUTO_MEM_LOCK(m_file_writing_lock);
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
          if (packet_process_state == AM_STATE_IO_ERROR) {
            ERROR("File system IO error in %s, stop file recording.",
                  m_muxer_name.c_str());
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
          } else {
            ++ error_count;
            WARN("On normal data packet error in %s, error count is %u!",
                 m_muxer_name.c_str(),
                 error_count);
            if (error_count == ON_DATA_PKT_ERROR_NUM) {
              ERROR("On normal data packet error in %s, exit the main loop",
                    m_muxer_name.c_str());
              AUTO_MEM_LOCK(m_state_lock);
              m_state = AM_MUXER_CODEC_ERROR;
              is_ok = false;
            }
          } break;
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
        if (AM_UNLIKELY(on_eos_pkt(packet) != AM_STATE_OK)) {
          ERROR("On eos packet error in %s, exit the main loop.",
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
  NOTICE("%s exit mainloop.", m_muxer_name.c_str());
}

AM_STATE AMMuxerTimeElapseMp4::generate_file_name(char* file_name)
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
        PERROR("Failed to symlink hls file location in AMMuxerTimeElapseMp4.");
      }
    }
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string, sizeof(time_string))))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    sprintf(file_name,
            "%s/%s_%s_stream%u",
            m_file_location.c_str(),
            m_curr_config.file_name_prefix.c_str(),
            time_string,
            m_curr_config.video_id);
    INFO("Generate file success: %s in %s", file_name, m_muxer_name.c_str());
  } while (0);
  return ret;
}

bool AMMuxerTimeElapseMp4::get_proper_file_location(std::string& file_location)
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

bool AMMuxerTimeElapseMp4::get_current_time_string(char *time_str, int32_t len)
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

void AMMuxerTimeElapseMp4::clear_params_for_new_file()
{
  m_eos_map = 0;
  m_video_frame_count = 0;
  m_is_first_video = true;
  m_new_info_coming = false;
  m_video_duration = 0;
}

AM_STATE AMMuxerTimeElapseMp4::on_info_pkt(AMPacket* packet)
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
      if (m_av_info_map == 1) {
        /*video info is set already, check new video info. If it is not
         * changed, discard it. if it is changed, close current file,
         * init the mp4 builder, write media data to new file*/
        if (memcmp(&m_video_info, video_info, sizeof(AM_VIDEO_INFO)) == 0) {
          NOTICE("%s receives new info pkt, but is not changed. Discard it.",
                 m_muxer_name.c_str());
          break;
        }
        NOTICE("%s receives new video info pkt and it's value is changed, "
            "close current file, create new file.",
            m_muxer_name.c_str());
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_mp4_builder->set_video_info(video_info)
                        != AM_STATE_OK)) {
          ERROR("Failed to set video info to MP4 builder in MP4 normal muxer.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_new_info_coming = true;
      } else {
        /*first video info packet*/
        memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
        if (AM_UNLIKELY(m_mp4_builder->set_video_info(video_info)
                        != AM_STATE_OK)) {
          ERROR("Failed to set video info to MP4 builder in %s.",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        uint64_t file_duration = m_curr_config.file_duration *
            PTS_TIME_FREQUENCY;
        if (file_duration <= 0) {
          m_need_splitted = false;
        } else {
          m_need_splitted = true;
        }
        m_av_info_map = 1;
        INFO("%s av info map is %u", m_muxer_name.c_str(), m_av_info_map);
        if (m_file_writing && (!m_file_writer->is_file_open())) {
          update_config_param();
          if (m_file_writer->create_next_file() != AM_STATE_OK) {
            ERROR("Failed to create new file %s in %s",
                  m_file_writer->get_current_file_name(),
                  m_muxer_name.c_str());
            ret = AM_STATE_ERROR;
            break;
          }
          if (AM_UNLIKELY((ret = m_mp4_builder->begin_file()) != AM_STATE_OK)) {
            ERROR("Failed to start building mp4 file in %s",
                  m_muxer_name.c_str());
            break;
          }
        }
      }
    } else {
      NOTICE("Currently, %s only support video stream.",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerTimeElapseMp4::on_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  switch (packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      if (!m_file_writing) {
        break;
      }
      if (AM_UNLIKELY((ret = write_video_data_pkt(packet)) != AM_STATE_OK)) {
        ERROR("Failed to write video data packet in %s", m_muxer_name.c_str());
        break;
      }
    } break;
    case AMPacket::AM_PAYLOAD_ATTR_SEI: {
      INFO("Get usr sei data in %s", m_muxer_name.c_str());
      if (!m_file_writing) {
        break;
      }
      if (AM_UNLIKELY((ret = m_mp4_builder->write_SEI_data(packet))
                      != AM_STATE_OK)) {
        ERROR("Failed to write SEI data in %s", m_muxer_name.c_str());
        break;
      }
    } break;
    default: {
      NOTICE("Currently, %s just support video packet",
             m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
    } break;
  }
  return ret;
}

AM_STATE AMMuxerTimeElapseMp4::on_eos_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!m_file_writing) {
      break;
    }
    if (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      NOTICE("video eos is received in %s.", m_muxer_name.c_str());
      m_eos_map = 1;
    } else {
      ERROR("Currently %s just support video eos packet.",
            m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_eos_map == 1) {
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

AM_STATE AMMuxerTimeElapseMp4::on_eof_pkt(AMPacket* packet)
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
              m_file_writer->get_current_file_name(),
              m_muxer_name.c_str());
        break;
      }
      update_config_param();
      if ((ret = m_file_writer->create_next_file()) != AM_STATE_OK) {
        ERROR("Failed to create file %s in %s.",
              m_file_writer->get_current_file_name(),
              m_muxer_name.c_str());
        break;
      }
      if ((ret = m_mp4_builder->begin_file()) != AM_STATE_OK) {
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

AM_STATE AMMuxerTimeElapseMp4::write_video_data_pkt(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (m_av_info_map != 1) {
      INFO("Video info packet has not been received in %s, av_info_map is %u"
          "drop this data packet!", m_muxer_name.c_str(), m_av_info_map);
      break;
    }
    if (AM_UNLIKELY(m_is_first_video)) {
      /*first frame must be video frame,
       * and the first video frame must be I frame*/
      if (m_video_info.type == AM_VIDEO_H264) {
        if (AM_UNLIKELY(packet->get_frame_type() != AM_VIDEO_FRAME_TYPE_IDR)) {
          INFO("First frame must be video IDR frame, "
              "drop this packet in %s",
              m_muxer_name.c_str());
          break;
        }
      } else if (m_video_info.type == AM_VIDEO_H265) {
        if (AM_UNLIKELY(!((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR)
            && is_h265_IDR_first_nalu(packet)))) {
          INFO("First frame must be video hevc IDR frame start, "
              "drop this packet in %s",
              m_muxer_name.c_str());
          break;
        }
      } else {
        ERROR("video type error in %s.", m_muxer_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
      m_file_writer->set_begin_packet_pts(m_video_duration);
      m_is_first_video = false;
      m_curr_file_boundary = m_curr_config.file_duration * PTS_TIME_FREQUENCY;
    } else {
      m_file_writer->set_end_packet_pts(m_video_duration);
    }
    m_video_duration += (PTS_TIME_FREQUENCY / m_curr_config.frame_rate);
    if (AM_UNLIKELY((packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_IDR)
                    && (m_new_info_coming
                    || (m_file_writer->get_file_offset()
                      >= m_curr_config.max_file_size * 1024 * 1024)
       || (m_need_splitted && (m_video_duration >= m_curr_file_boundary))))) {
      /* if (m_is_new_info == true)
       *  video or audio info has been changed, close the current file,
       *  create a new file
       * if (m_need_splitted && reached file_boundary && m_video_frame_count > 0)
       *  close the last file and create a new file
       */
      if (m_new_info_coming) {
        m_new_info_coming = false;
        if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
          ERROR("On eof error in %s.", m_muxer_name.c_str());
          break;
        }
      }
      if (m_need_splitted && (m_video_duration >= m_curr_file_boundary)) {
        if (m_video_frame_count == 0) {
          NOTICE("Data packet pts in %s reach the file bounday,"
              "but the video frame count is zero, update file boundary, "
              "continue writing.",
              m_muxer_name.c_str());
          m_video_duration = 0;
        } else {
          if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
            ERROR("On eof error in %s.", m_muxer_name.c_str());
            break;
          }
        }
      }
      if (m_file_writer->get_file_offset()
          >= m_curr_config.max_file_size * 1024 * 1024) {
        NOTICE("The file size reach max file size %u MB, close current file and "
            "create a new file",
            m_curr_config.max_file_size);
        if (AM_UNLIKELY((ret = on_eof_pkt(packet)) != AM_STATE_OK)) {
          ERROR("On eof error in %s.", m_muxer_name.c_str());
          break;
        }
      }
      /* This packet should be written into next file,
       * push it back into the front of the queue
       */
      packet->add_ref();
      m_packet_queue->push_front(packet);
      break;
    }
    if (AM_UNLIKELY(((ret = m_mp4_builder->write_video_data(packet)))
                    != AM_STATE_OK)) {
      ERROR("Failed to write video data in %s.", m_muxer_name.c_str());
      break;
    }
    ++ m_video_frame_count;
  } while (0);
  return ret;
}

AM_MUXER_CODEC_STATE AMMuxerTimeElapseMp4::create_resource()
{
  char file_name[strlen(m_curr_config.file_location.c_str())
                 + strlen(m_curr_config.file_name_prefix.c_str()) + 128];
  memset(file_name, 0, sizeof(file_name));
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
    if (AM_UNLIKELY((m_file_writer = AMTimeElapseMp4FileWriter::create (
        &m_curr_config, &m_video_info)) == nullptr)) {
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
    AM_DESTROY(m_mp4_builder);
    if (AM_UNLIKELY((m_mp4_builder = AMTimeElapseMp4Builder::create(
        m_file_writer, &m_curr_config)) == nullptr)) {
      ERROR("Failde to create MP4 builder in %s.", m_muxer_name.c_str());
      ret = AM_MUXER_CODEC_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->set_media_sink(file_name) != AM_STATE_OK)) {
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

void AMMuxerTimeElapseMp4::release_resource()
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
  while (!(m_packet_queue->empty())) {
    m_packet_queue->front()->release();
    m_packet_queue->pop_front();
  }
  NOTICE("Release resource success in %s.", m_muxer_name.c_str());
}

void AMMuxerTimeElapseMp4::clear_all_params()
{
  INFO("Begin to clear all params in %s", m_muxer_name.c_str());
  memset(&m_video_info, 0, sizeof(AM_VIDEO_INFO));
  m_file_location.clear();
  while (!m_packet_queue->empty()) {
    m_packet_queue->front()->release();
    m_packet_queue->pop_front();
  }
  AM_DESTROY(m_mp4_builder);
  AM_DESTROY(m_file_writer);
  m_eos_map = 0;
  m_av_info_map = 0;
  m_video_frame_count = 0;
  m_is_first_video = true;
  m_new_info_coming = false;
  m_video_duration = 0;
  INFO("Clear all params in %s success", m_muxer_name.c_str());
}

#define DELIMITER_SIZE 3
bool AMMuxerTimeElapseMp4::is_h265_IDR_first_nalu(AMPacket* packet)
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

void AMMuxerTimeElapseMp4::check_storage_free_space()
{
  uint64_t free_space = 0;
  struct statfs disk_statfs;
  if (statfs(m_file_location.c_str(), &disk_statfs) < 0) {
    PERROR("statfs");
    ERROR("%s stafs error in %s.",
          m_file_location.c_str(),
          m_muxer_name.c_str());
  } else {
    free_space = ((uint64_t) disk_statfs.f_bsize
        * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
    DEBUG("Free space is %llu M in %s", free_space, m_muxer_name.c_str());
    if (AM_UNLIKELY(free_space <= m_curr_config.smallest_free_space)) {
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

bool AMMuxerTimeElapseMp4::set_muxer_param(AMMuxerParam &param)
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
      INFO("Set file duration : %llu in %s", m_set_config.file_duration,
           m_muxer_name.c_str());
      if ((m_set_config.file_duration <= 0) && (m_need_splitted)) {
        m_need_splitted = false;
      }
      if (param.save_to_config_file) {
        m_muxer_mp4_config->file_duration = m_set_config.file_duration;
      }
    }
    if (param.video_frame_rate_u32.is_set) {
      m_set_config.frame_rate = param.video_frame_rate_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->frame_rate = m_set_config.frame_rate;
      }
      INFO("Set frame rate %u in %s", m_set_config.frame_rate,
           m_muxer_name.c_str());
    }
    if (param.hls_enable_bool.is_set) {
      m_set_config.hls_enable = param.hls_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->hls_enable = m_set_config.hls_enable;
      }
      INFO("%s hls in %s", m_set_config.hls_enable ? "Enable" : "Disable",
          m_muxer_name.c_str());
    }
    if (param.write_sync_enable_bool.is_set) {
      m_set_config.write_sync_enable = param.write_sync_enable_bool.value.v_bool;
      if (param.save_to_config_file) {
        m_muxer_mp4_config->write_sync_enable = m_set_config.write_sync_enable;
      }
      INFO("%s write sync in %s", m_set_config.write_sync_enable ?
          "Enable" : "Disable", m_muxer_name.c_str());
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

void AMMuxerTimeElapseMp4::update_config_param()
{
  AUTO_MEM_LOCK(m_param_set_lock);
  m_curr_config = m_set_config;
}

