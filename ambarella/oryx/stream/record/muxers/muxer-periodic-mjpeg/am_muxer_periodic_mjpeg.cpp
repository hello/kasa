/*******************************************************************************
 * am_muxer_periodic_mjpeg.cpp
 *
 * History:
 *   2016-04-20 - [ccjing] created file
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

#include "am_muxer_periodic_mjpeg.h"
#include "am_muxer_periodic_mjpeg_writer.h"

#define HW_TIMER  ((const char*)"/proc/ambarella/ambarella_hwtimer")

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerPeriodicMjpeg::create(config_file));
}

AMMuxerPeriodicMjpeg* AMMuxerPeriodicMjpeg::create(const char* config_name)
{
  AMMuxerPeriodicMjpeg* result = new AMMuxerPeriodicMjpeg();
  if (result && (result->init(config_name) != AM_STATE_OK)) {
    ERROR("Failed to create AMMuxerPeriodicMjpeg.");
    delete result;
    result = nullptr;
  }
  return result;
}

AM_STATE AMMuxerPeriodicMjpeg::start()
{
  AUTO_MEM_LOCK(m_interface_lock);
  AM_STATE ret = AM_STATE_OK;
  do {
    bool need_break = false;
    switch (get_state()) {
      case AM_MUXER_CODEC_RUNNING: {
        NOTICE("The periodic jpeg muxer is already running.");
        need_break = true;
      }break;
      case AM_MUXER_CODEC_ERROR: {
        NOTICE("Periodic Jpeg muxer state is error! Need to be re-created!");
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
      m_thread = AMThread::create(m_muxer_name.c_str(), thread_entry, this);
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
      NOTICE("Start %s success.", m_muxer_name.c_str());
    } else {
      ERROR("Failed to start %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerPeriodicMjpeg::stop()
{
  AUTO_MEM_LOCK(m_interface_lock);
  AM_STATE ret = AM_STATE_OK;
  m_run = false;
  AM_DESTROY(m_thread);
  NOTICE("Stop %s success.", m_muxer_name.c_str());
  return ret;
}

const char* AMMuxerPeriodicMjpeg::name()
{
  return m_muxer_name.c_str();
}

void* AMMuxerPeriodicMjpeg::get_muxer_codec_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIFileOperation) ? ((AMIFileOperation*)this) :
            ((refiid == IID_AMIMuxerCodec) ? ((AMIMuxerCodec*)this) :
                ((void*)nullptr));
}

bool AMMuxerPeriodicMjpeg::start_file_writing()
{
  bool ret = true;
  INFO("Begin to start file writing in %s.", m_muxer_name.c_str());
  do{
    if(m_file_writing) {
      NOTICE("File writing is already startted in %s.", m_muxer_name.c_str());
      break;
    }
    m_file_writing = true;
    INFO("Start file writing success in %s.", m_muxer_name.c_str());
  }while(0);
  return ret;
}


bool AMMuxerPeriodicMjpeg::stop_file_writing()
{
  bool ret = true;
  INFO("Stopping file writing in %s.", m_muxer_name.c_str());
  do{
    AUTO_MEM_LOCK(m_file_writing_lock);
    if(!m_file_writing) {
      NOTICE("File writing is already stopped in %s", m_muxer_name.c_str());
      break;
    }
    m_file_writer->close_files();
    m_file_writing = false;
    INFO("Stop file writing success in %s.", m_muxer_name.c_str());
  }while(0);
  return ret;
}

bool AMMuxerPeriodicMjpeg::set_recording_file_num(uint32_t file_num)
{
  ERROR("Should not set recording file num in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerPeriodicMjpeg::set_recording_duration(int32_t duration)
{
  ERROR("Should not set recording duration in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerPeriodicMjpeg::set_file_duration(int32_t file_duration,
                                            bool apply_conf_file)
{
  ERROR("Should not set file duration in %s", m_muxer_name.c_str());
  return false;
}

bool AMMuxerPeriodicMjpeg::is_running()
{
  return m_run.load();
}

AM_MUXER_CODEC_STATE AMMuxerPeriodicMjpeg::get_state()
{
  AUTO_MEM_LOCK(m_state_lock);
  return m_state;
}

AM_MUXER_ATTR AMMuxerPeriodicMjpeg::get_muxer_attr()
{
  return AM_MUXER_FILE_EVENT;
}

uint8_t AMMuxerPeriodicMjpeg::get_muxer_codec_stream_id()
{
  return (uint8_t)((0x01) << (m_curr_config.video_id));
}

uint32_t AMMuxerPeriodicMjpeg::get_muxer_id()
{
  return m_curr_config.muxer_id;
}

void AMMuxerPeriodicMjpeg::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EVENT) {
      AMEventStruct* event = (AMEventStruct*)(packet->get_data_ptr());
      if ((event->attr == AM_EVENT_PERIODIC_MJPEG) &&
          (packet->get_stream_id() == m_curr_config.video_id)) {
        NOTICE("%s receive event packet, Event attr is AM_EVENT_PERIODIC_MJPEG, "
            "stream id is %u, start time is %u-%u-%u, end time is %u-%u-%u, "
            "interval second is %u, once_frame_number is %u",
            m_muxer_name.c_str(), event->mjpeg.stream_id,
            event->periodic_mjpeg.start_time_hour,
            event->periodic_mjpeg.start_time_minute,
            event->periodic_mjpeg.start_time_second,
            event->periodic_mjpeg.end_time_hour,
            event->periodic_mjpeg.end_time_minute,
            event->periodic_mjpeg.end_time_second,
            event->periodic_mjpeg.interval_second,
            event->periodic_mjpeg.once_jpeg_num);
        if (parse_event_param(packet)) {
          m_jpeg_enable = true;
        } else {
          ERROR("Failed to parse event param in %s", m_muxer_name.c_str());
        }
        break;
      }
    }
    if (!m_jpeg_enable) {
      break;
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
    m_packet_queue->push_back(packet);
  }
  packet->release();
}

AM_STATE AMMuxerPeriodicMjpeg::set_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

AM_STATE AMMuxerPeriodicMjpeg::get_config(AMMuxerCodecConfig* config)
{
  return AM_STATE_ERROR;
}

AMMuxerPeriodicMjpeg::AMMuxerPeriodicMjpeg()
{
}

AM_STATE AMMuxerPeriodicMjpeg::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(config_file == nullptr)) {
      ERROR("Config_file is NULL, should input valid config_file");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config_file = amstrdup(config_file);
    if (AM_UNLIKELY((m_packet_queue = new packet_queue()) == nullptr)) {
      ERROR("Failed to create packet_queue in AMMuxerPeriodicMjpeg.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    if (AM_UNLIKELY((m_config = new AMMuxerPeriodicMjpegConfig()) == nullptr)) {
      ERROR("Failed to create jpeg config in AMMuxerPeriodicMjpeg.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    m_periodic_mjpeg_config = m_config->get_config(std::string(m_config_file));
    if (AM_UNLIKELY(!m_periodic_mjpeg_config)) {
      ERROR("Failed to get config in AMMuxerPeriodicMjpeg");
      ret = AM_STATE_ERROR;
      break;
    }
    m_curr_config = *m_periodic_mjpeg_config;
    m_set_config = m_curr_config;
    if ((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0) {
      ERROR("%s: %s", HW_TIMER, strerror(errno));
      ret = AM_STATE_ERROR;
      break;
    }
    m_muxer_name = "PeriodicJpegMuxer";
    m_file_writing = m_curr_config.auto_file_writing;
  } while (0);
  return ret;
}

AMMuxerPeriodicMjpeg::~AMMuxerPeriodicMjpeg()
{
  stop();
  if (m_hw_timer_fd > 0) {
    close(m_hw_timer_fd);
    m_hw_timer_fd = -1;
  }
  delete m_config;
  delete m_packet_queue;
  delete m_config_file;
  INFO("%s object destroyed.", m_muxer_name.c_str());
}

void AMMuxerPeriodicMjpeg::main_loop()
{
  bool is_ok = true;
  uint32_t statfs_number = 0;
  m_run = true;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  while(m_run.load() && is_ok) {
    AMPacket* packet = NULL;
    if (m_packet_queue->empty()) {
      DEBUG("Packet queue is empty, sleep 100 ms.");
      usleep(100000);
      continue;
    }
    packet = m_packet_queue->front();
    m_packet_queue->pop_front();
    if (AM_LIKELY(is_ok && m_file_writing)) {
      AUTO_MEM_LOCK(m_file_writing_lock);
      switch (packet->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          ERROR("receive info packet.");
          if (AM_UNLIKELY(on_info_packet(packet) != AM_STATE_OK)) {
            ERROR("On video info error, exit the main loop.");
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
            ++ error_count;
            WARN("On normal data packet error, error count is %u!", error_count);
            if(error_count == PERIODIC_MJPEG_DATA_PKT_ERROR_NUM) {
              ERROR("On normal data packet error, exit the main loop");
              AUTO_MEM_LOCK(m_state_lock);
              m_state = AM_MUXER_CODEC_ERROR;
              is_ok = false;
            }
            break;
          }
          error_count = 0;
          if (m_file_size > 0) {
            ++ statfs_number;
            if ((statfs_number >= CHECK_FREE_SPACE_FREQUENCY_FOR_PERIODIC_MJPEG) &&
                m_file_writing) {
              statfs_number = 0;
              check_storage_free_space();
            }
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          NOTICE("Receive eos packet in %s", m_muxer_name.c_str());
          if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
            ERROR("On eos packet error, exit the main loop.");
            AUTO_MEM_LOCK(m_state_lock);
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
  if (AM_UNLIKELY(!m_run.load())) {
    AUTO_MEM_LOCK(m_state_lock);
    m_state = AM_MUXER_CODEC_STOPPED;
  }
  NOTICE("Jpeg muxer exit mainloop.");
}

AM_MUXER_CODEC_STATE AMMuxerPeriodicMjpeg::create_resource()
{
  INFO("Begin to create resource in %s.", m_muxer_name.c_str());
  AM_MUXER_CODEC_STATE ret = AM_MUXER_CODEC_RUNNING;
  do {
    AM_DESTROY(m_file_writer);
    if (AM_UNLIKELY((m_file_writer = AMPeriodicMjpegWriter::create (
        &m_curr_config)) == nullptr)) {
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
  } while (0);
  AUTO_MEM_LOCK(m_state_lock);
  m_state = ret;
  return ret;
}

AM_STATE AMMuxerPeriodicMjpeg::generate_file_name(char file_name[])
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
    sprintf(file_name, "%s/%s_%s_stream%u",
            m_file_location.c_str(),
            m_curr_config.file_name_prefix.c_str(),
            time_string, m_curr_config.video_id);
    INFO("Generate file success: %s in %s", file_name, m_muxer_name.c_str());
  } while(0);
  return ret;
}

void AMMuxerPeriodicMjpeg::thread_entry(void* p)
{
  ((AMMuxerPeriodicMjpeg*) p)->main_loop();
}

void AMMuxerPeriodicMjpeg::release_resource()
{
  while (!(m_packet_queue->empty())) {
    m_packet_queue->front()->release();
    m_packet_queue->pop_front();
  }
  AM_DESTROY(m_file_writer);
  m_file_create_cb = nullptr;
  m_file_finish_cb = nullptr;
  INFO("Release resource success in %s.", m_muxer_name.c_str());
}

bool AMMuxerPeriodicMjpeg::get_current_time_string(char *time_str, int32_t len)
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

bool AMMuxerPeriodicMjpeg::get_proper_file_location(std::string& file_location)
{
  bool ret = true;
  uint64_t max_free_space = 0;
  uint64_t free_space = 0;
  std::ifstream file;
  std::string read_line;
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
      INFO("mount information :");
      while (getline(file, read_line)) {
        std::string temp_location;
        temp_location.clear();
        INFO("%s", read_line.c_str());
        if ((find_str_position = read_line.find(storage_str))
            != std::string::npos) {
          for (uint32_t i = find_str_position;; ++ i) {
            if (read_line.substr(i, 1) != " ") {
              temp_location += read_line.substr(i, 1);
            } else {
              location_list.push_back(temp_location);
              INFO("Find a storage str : %s in %s", temp_location.c_str(),
                   m_muxer_name.c_str());
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
              INFO("Find a sdcard str : %s in %s", temp_location.c_str(),
                   m_muxer_name.c_str());
              break;
            }
          }
        }
      }
      if (!location_list.empty()) {
        string_list::iterator it = location_list.begin();
        for (; it != location_list.end(); ++ it) {
          if ((*it) == config_path) {
            NOTICE("File location specified by config file is on %s in %s, great.",
                   config_path.c_str(),  m_muxer_name.c_str());
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
              PERROR("File location statfs");
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
            WARN("File location in config file is not exist in %s.",
                 m_muxer_name.c_str());
            file_location = (*max_free_space_it) + file_location_suffix;
          } else {
            free_space = ((uint64_t) disk_statfs.f_bsize
                * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
            if (free_space > max_free_space) {
              NOTICE("The free space of file location in config file is larger"
                  "than sdcard free space in %s, use file location in "
                  "config file.", m_muxer_name.c_str());
              file_location = config_path + file_location_suffix;
            } else {
              file_location = (*max_free_space_it) + file_location_suffix;
              NOTICE("The free space of file location in config file is smaller"
                  "than sdcard free space, set file location on %s in %s.",
                  (*max_free_space_it).c_str(), m_muxer_name.c_str());
            }
          }
        }
      } else {
        NOTICE("Do not find storage or sdcard string in mount information in"
            " %s.", m_muxer_name.c_str());
        if (!AMFile::exists(m_curr_config.file_location.c_str())) {
          if (!AMFile::create_path(m_curr_config.file_location.c_str())) {
            ERROR("Failed to create file path: %s in %s!",
                  m_curr_config.file_location.c_str(),
                  m_muxer_name.c_str());
            ret = false;
            break;
          }
        }
        struct statfs disk_statfs;
        if (statfs(m_curr_config.file_location.c_str(), &disk_statfs)
            < 0) {
          PERROR("File location in config file statfs");
          ret = false;
          break;
        } else {
          free_space = ((uint64_t) disk_statfs.f_bsize
              * (uint64_t) disk_statfs.f_bfree) / (uint64_t) (1024 * 1024);
          if (free_space >= 20) {
            file_location = m_curr_config.file_location;
            NOTICE("Free space is larger than 20M, use it in %s.",
                   m_muxer_name.c_str());
            break;
          } else {
            ERROR("Free space is smaller than 20M, please"
                "set file location on sdcard or usb storage in %s.",
                m_muxer_name.c_str());
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
           file_location.c_str(), m_muxer_name.c_str());
  } while (0);
  if (file.is_open()) {
    file.close();
  }
  return ret;
}

void AMMuxerPeriodicMjpeg::check_storage_free_space()
{
  uint64_t free_space = 0;
  struct statfs disk_statfs;
  if(statfs(m_file_location.c_str(), &disk_statfs) < 0) {
    PERROR("Statfs");
    ERROR("%s statfs error in %s", m_file_location.c_str(),
          m_muxer_name.c_str());
  } else {
    free_space = ((uint64_t)disk_statfs.f_bsize *
        (uint64_t)disk_statfs.f_bfree) / (uint64_t)(1024 * 1024);
    DEBUG("Free space is %llu M in %s", free_space, m_muxer_name.c_str());
    if(AM_UNLIKELY(free_space <=
                   m_curr_config.smallest_free_space)) {
      ERROR("The free space is smaller than %d M in %s, "
          "will stop writing data to jpeg file",
          m_curr_config.smallest_free_space, m_muxer_name.c_str());
      m_file_writing = false;
    }
  }
}

AM_STATE AMMuxerPeriodicMjpeg::on_info_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  NOTICE("Receive info packet in %s", m_muxer_name.c_str());
  do {
    AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*) (packet->get_data_ptr());
    if (!video_info) {
      ERROR("%s received video info is null", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
    }
    INFO("\n%s receive INFO:\n"
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
        video_info->stream_id, video_info->width,
        video_info->height, video_info->M, video_info->N,
        video_info->rate, video_info->scale, video_info->fps,
        video_info->mul, video_info->div);
  } while(0);
  return ret;
}

AM_STATE AMMuxerPeriodicMjpeg::on_data_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  do{
    if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
      if ((m_start_pts > 0) && (packet->get_pts() >= m_start_pts) &&
          (!m_file_writer->is_file_open())) {
        update_config_param();
        char file_name[strlen(m_curr_config.file_location.c_str())
              + strlen(m_curr_config.file_name_prefix.c_str()) + 128];
        memset(file_name, 0, sizeof(file_name));
        if (AM_UNLIKELY(generate_file_name(file_name) != AM_STATE_OK)) {
          ERROR("%s generate file name error, exit main loop.",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(m_file_writer->set_file_name(file_name) != AM_STATE_OK)) {
          ERROR("Failed to set file name for m_file_writer in %s!",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_file_writer->create_next_files();
      }
      if ((packet->get_pts() >= m_next_period_pts) && (m_tmp_jpeg_num > 0)) {
        if(AM_UNLIKELY(m_file_writer->write_jpeg_data(packet->get_data_ptr(),
                                    packet->get_data_size()) != AM_STATE_OK)) {
          ERROR("Failed to write data to jpeg file in %s.", m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        MjpegFrameInfo info;
        time_t timep;
        struct tm *local_time;
        time(&timep);
        local_time = localtime(&timep);
        info.cur_second = local_time->tm_hour * 3600 +
            local_time->tm_min * 60 + local_time->tm_sec;
        info.cur_second = ((packet->get_pts() - m_start_pts) / 90000) +
            m_start_second;
        info.offset = m_file_size;
        info.frame_size = packet->get_data_size();
        info.frame_counter = m_frame_counter ++;
        INFO("current time is %u-%u-%u, offset is %u, frame size is %u,"
            " frame counter is %u", info.cur_second / 3600,
            ((info.cur_second % 3600) / 60), ((info.cur_second % 3600) % 60),
            info.offset, info.frame_size, info.frame_counter);
        if(AM_UNLIKELY(m_file_writer->write_text_data((uint8_t*)(&info),
                          sizeof(info)) != AM_STATE_OK)) {
          ERROR("Failed to write frame info data to text file in %s",
                m_muxer_name.c_str());
          ret = AM_STATE_ERROR;
          break;
        }
        m_file_size += packet->get_data_size();
        -- m_tmp_jpeg_num;
        if (!check_file_size()) {
          ERROR("Failed to check file size");
          ret = AM_STATE_ERROR;
          break;
        }
      }
      if ((m_tmp_jpeg_num == 0) &&
          ((m_next_period_pts + m_interval_pts) <= m_end_pts)) {
        m_next_period_pts += m_interval_pts;
        m_tmp_jpeg_num = m_once_jpeg_num;
      }
      if ((m_tmp_jpeg_num == 0) &&
          ((m_next_period_pts + m_interval_pts) > m_end_pts)) {
        m_jpeg_enable = false;
        m_start_pts = 0;
        m_start_second = 0;
        m_end_pts = 0;
        m_interval_pts = 0;
        m_next_period_pts = 0;
        m_once_jpeg_num = 0;
        m_tmp_jpeg_num = 0;
        m_file_writer->close_files();
      }
    } else {
      NOTICE("%s just support video stream.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  }while(0);
  return ret;
}

AM_STATE AMMuxerPeriodicMjpeg::on_eos_packet(AMPacket* packet)
{
  AM_STATE ret = AM_STATE_OK;
  m_file_writer->close_files();
  m_run = false;
  m_start_pts = 0;
  m_start_second = 0;
  m_end_pts = 0;
  m_interval_pts = 0;
  m_next_period_pts = 0;
  m_once_jpeg_num = 0;
  m_tmp_jpeg_num = 0;
  m_file_size = 0;
  m_jpeg_enable = false;
  NOTICE("Receive eos packet in %s, exit the main loop", m_muxer_name.c_str());
  return ret;
}

bool AMMuxerPeriodicMjpeg::parse_event_param(AMPacket* packet)
{
  bool ret = true;
  time_t timep;
  struct tm *local_time;
  AMEventStruct* event = (AMEventStruct*)(packet->get_data_ptr());
  do {
    time(&timep);
    local_time = localtime(&timep);
    uint32_t local_second = local_time->tm_hour * 3600 +
        local_time->tm_min * 60 + local_time->tm_sec;
    m_start_second = event->periodic_mjpeg.start_time_hour * 3600 +
        event->periodic_mjpeg.start_time_minute * 60 +
        event->periodic_mjpeg.start_time_second;
    uint32_t end_second = event->periodic_mjpeg.end_time_hour * 3600 +
        event->periodic_mjpeg.end_time_minute * 60 +
        event->periodic_mjpeg.end_time_second;
    if (m_start_second < local_second) {
      ERROR("The begin second should be bigger than current second, "
          "begin time is %u-%u-%u, current time is %u-%u-%u",
          event->periodic_mjpeg.start_time_hour,
          event->periodic_mjpeg.start_time_minute,
          event->periodic_mjpeg.start_time_second,
          local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
      ret = false;
      break;
    }
    if (m_start_second >= end_second) {
      ERROR("The begin second should be smaller than end second,"
          "begin time is %u-%u-%u, end time is %u-%u-%u",
          event->periodic_mjpeg.start_time_hour,
          event->periodic_mjpeg.start_time_minute,
          event->periodic_mjpeg.start_time_second,
          event->periodic_mjpeg.end_time_hour,
          event->periodic_mjpeg.end_time_minute,
          event->periodic_mjpeg.end_time_second);
      ret = false;
      break;
    }
    m_start_pts = get_current_pts() +
        (int64_t)((m_start_second - local_second) * 90000);
    m_end_pts = get_current_pts() +
        (int64_t)((end_second - local_second) * 90000);
    m_interval_pts = event->periodic_mjpeg.interval_second * 90000;
    m_once_jpeg_num = event->periodic_mjpeg.once_jpeg_num;
    m_next_period_pts = m_start_pts;
    m_tmp_jpeg_num = m_once_jpeg_num;
  } while(0);
  return ret;
}

bool AMMuxerPeriodicMjpeg::check_file_size()
{
  bool ret = true;
  do {
    if (m_file_size >= m_curr_config.max_file_size * 1024 * 1024) {
      NOTICE("The file size reach %u MB, create new file",
             m_curr_config.max_file_size);
      m_file_writer->close_files();
      update_config_param();
      if (m_file_writer->create_next_files() != AM_STATE_OK) {
        ERROR("Failed to create next files.");
        ret = false;
        break;
      }
      m_file_size = 0;
      m_frame_counter = 0;
    }
  } while(0);
  return ret;
}

AM_PTS AMMuxerPeriodicMjpeg::get_current_pts()
{
  uint8_t pts[32] = {0};
  AM_PTS current_pts = 0;
  if (m_hw_timer_fd  < 0) {
    return current_pts;
  }
  if (read(m_hw_timer_fd, pts, sizeof(pts)) < 0) {
    PERROR("read");
  } else {
    current_pts = strtoull((const char*)pts, (char**)NULL, 10);
  }
  return current_pts;
}

bool AMMuxerPeriodicMjpeg::set_file_operation_callback(AM_FILE_OPERATION_CB_TYPE type,
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

bool AMMuxerPeriodicMjpeg::set_muxer_param(AMMuxerParam &param)
{
  bool ret = true;
  do {
    if ((param.muxer_id_bit_map &
        (0x00000001 << m_curr_config.muxer_id)) == 0) {
      break;
    }
    AUTO_MEM_LOCK(m_param_set_lock);
    if (param.max_file_size_u32.is_set) {
      m_set_config.max_file_size = param.max_file_size_u32.value.v_u32;
      if (param.save_to_config_file) {
        m_periodic_mjpeg_config->max_file_size = m_set_config.max_file_size;
      }
      INFO("Set max file size : %u in %s", m_set_config.max_file_size,
           m_muxer_name.c_str());
    }
    if (param.save_to_config_file) {
      INFO("Save to config file in %s", m_muxer_name.c_str());
      if (AM_UNLIKELY(!m_config->set_config(m_periodic_mjpeg_config))) {
        ERROR("Failed to save param into config file in %s.", m_muxer_name.c_str());
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

void AMMuxerPeriodicMjpeg::update_config_param()
{
  AUTO_MEM_LOCK(m_param_set_lock);
  m_curr_config = m_set_config;
}
