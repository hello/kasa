/*
 * am_muxer_ts_file_writer.cpp
 *
 *  19/09/2012 [Hanbo Xiao]    [Created]
 *  17/12/2014 [Chengcai Jing] [Modified]
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
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>

#include "am_base_include.h"
#include "am_amf_types.h"
#include "am_log.h"
#include "am_define.h"

#include "am_muxer_codec_info.h"
#include "am_muxer_ts_builder.h"
#include "am_muxer_ts_file_writer.h"
#include "am_muxer_ts_config.h"

#include "am_file_sink_if.h"
#include "am_file.h"
#include "am_record_msg.h"

#ifndef EXTRA_NAME_LEN
#define EXTRA_NAME_LEN 64
#endif

AMTsFileWriter *AMTsFileWriter::create (AMMuxerCodecTSConfig * muxer_ts_config,
                                        AM_VIDEO_INFO *video_info)
{
  AMTsFileWriter *result = new AMTsFileWriter ();
  if (AM_UNLIKELY(result && result->init (muxer_ts_config, video_info)
                  != AM_STATE_OK)) {
    ERROR("Failed to create AMTsFileWriter!");
    delete result;
    result = NULL;
  }

  return result;
}

AMTsFileWriter::AMTsFileWriter ()
{}

AMTsFileWriter::~AMTsFileWriter ()
{
  std::ofstream file;
  if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
    file.open("/etc/oryx/stream/muxer/muxer_file.conf");
    file << "dir_number:" << m_dir_counter << std::endl;
    file << "file_number:" << m_file_counter << std::endl;
    file.close();
  }
  AM_DESTROY (m_file_writer);
  delete[] m_file_name;
  delete[] m_tmp_name;
  delete[] m_path_name;
  delete[] m_base_name;
}

void AMTsFileWriter::destroy ()
{
   delete this;
}

AM_STATE AMTsFileWriter::init (AMMuxerCodecTSConfig * muxer_ts_config,
                               AM_VIDEO_INFO *video_info)
{
  AM_STATE ret = AM_STATE_OK;
  std::ifstream file;
  std::string read_line;
  do {
    if (AM_UNLIKELY((m_file_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create an instance of AMFilerWriter.\n");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config = muxer_ts_config;
    m_video_info = video_info;
    if(AM_UNLIKELY(m_config->max_file_num_per_folder < 0)) {
      WARN("max file num_per_folder is invalid, set it 512 as default.");
      m_max_file_num_per_folder = 512;
    } else {
      m_max_file_num_per_folder = m_config->max_file_num_per_folder;
    }
    if(muxer_ts_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      file.open("/etc/oryx/stream/muxer/muxer_file.conf");
      if(getline(file, read_line)) {
        size_t find_str_position = 0;
        std::string find_str = "dir_number:";
        char* end_ptr = nullptr;
        if((find_str_position = read_line.find(find_str))
            != std::string::npos) {
          m_dir_counter = strtol(
              &(read_line.c_str()[find_str_position + 11]),
              &end_ptr, 10);
          INFO("m dir counter is %d", m_dir_counter);
        }
      }
      if(getline(file, read_line)) {
        size_t find_str_position = 0;
        std::string find_str = "file_number:";
        char* end_ptr = nullptr;
        if((find_str_position = read_line.find(find_str))
            != std::string::npos) {
          m_file_counter = strtol(
              &(read_line.c_str()[find_str_position + 12]),
              &end_ptr, 10);
          INFO("m file counter is %d", m_file_counter);
        }
      }
      file.close();
    }
    if(m_dir_counter < 0) {
      m_dir_counter = 0;
    }
    if(m_file_counter < 0) {
      m_file_counter = 0;
    }
  } while (0);

   return ret;
}

AM_STATE AMTsFileWriter::set_media_sink (const char *file_name)
{
   char *copy_name = NULL;
   char *str = NULL;
   AM_STATE ret = AM_STATE_OK;
   do {
    if (AM_UNLIKELY(file_name == NULL)) {
      ERROR("File's name should not be empty.\n");
      ret = AM_STATE_ERROR;
      break;
    }

    INFO("file_name = %s\n", file_name);
    delete[] m_file_name;
    if (AM_UNLIKELY(NULL == (m_file_name = amstrdup(file_name)))) {
      ERROR("Failed to duplicate file name string!");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    delete[] m_tmp_name;
    if (AM_UNLIKELY((m_tmp_name = new char [strlen (m_file_name)
                                            + EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for tmp name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_tmp_name, 0, strlen(m_file_name) + EXTRA_NAME_LEN);

    /* Allocate memory for copyName */
    if (AM_UNLIKELY((copy_name = new char [strlen (m_file_name)
                                           + EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for copy name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    }

    /* Allocate memory for mpPathName */
    delete[] m_path_name;
    if (AM_UNLIKELY((m_path_name = new char [strlen (m_file_name)
                                             + EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for m_path_name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch path name from pFileName. */
      strcpy(copy_name, m_file_name);
      str = dirname(copy_name);
      strcpy(m_path_name, str);
      INFO("Directory's path: %s", m_path_name);
    }

    /* Allocate memory for mpBaseName */
    delete[] m_base_name;
    if (AM_UNLIKELY((m_base_name = new char [strlen (m_file_name)]) == NULL)) {
      ERROR("Failed to allocate memory for m_base_name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch base name from pFileName. */
      strcpy(copy_name, m_file_name);
      str = basename(copy_name);
      strcpy(m_base_name, str);
      INFO("Base name: %s", m_base_name);
    }
  } while (0);

  if (AM_LIKELY(copy_name)) {
    delete[] copy_name;
  }
   return ret;
}

AM_STATE AMTsFileWriter::create_next_file()
{
  AM_STATE ret = AM_STATE_OK;
  char* copy_name = NULL;
  char* dir_name = NULL;
  do {
    if((m_file_counter % m_max_file_num_per_folder) == 0) {
      ++ m_dir_counter;
    }
    if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      snprintf(m_tmp_name,
               strlen(m_file_name) + EXTRA_NAME_LEN,
               "%s/%d/%s_%u_%u.ts",
               m_path_name, m_dir_counter, m_base_name,
               m_dir_counter,
               (m_file_counter ++) % m_max_file_num_per_folder);
    } else if(m_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      snprintf(m_tmp_name,
               strlen(m_file_name) + EXTRA_NAME_LEN,
               "%s/%s_%u.ts",
               m_path_name, m_base_name,
               m_file_counter ++);
    } else {
      ERROR("muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
    copy_name = amstrdup(m_tmp_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s!", dir_name);
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(!m_file_writer->create_file(m_tmp_name))) {
      ERROR("Failed to create %s: %s\n", m_tmp_name, strerror (errno));
      ret = AM_STATE_ERROR;
      break;
    }
    memset(m_time_string, 0, sizeof(m_time_string));
    if (AM_UNLIKELY(!get_current_time_string(m_time_string,
                                             sizeof(m_time_string)))) {
      ERROR("Failed to get current time string.");
    }
    if (AM_UNLIKELY(!file_create_cb())) {
      ERROR("File create callback function error.");
    }
    if (AM_UNLIKELY(!m_file_writer->set_buf(IO_TRANSFER_BLOCK_SIZE))) {
      ERROR("Failed to set IO buffer.\n");
    }
    INFO("File create: %s\n", m_tmp_name);
  } while (0);
  delete[] copy_name;
   return ret;
}

bool AMTsFileWriter::is_file_open()
{
  return m_file_writer->is_file_open();
}

char* AMTsFileWriter::get_current_file_name()
{
  return m_tmp_name;
}

AM_STATE AMTsFileWriter::create_m3u8_file()
{
  AM_STATE ret = AM_STATE_OK;
  AMIFileWriter *file_writer = NULL;
  do {
    if(AM_UNLIKELY((file_writer = AMIFileWriter::create())
                   == NULL)) {
      ERROR("Failed to create file writer.");
      ret = AM_STATE_ERROR;
      break;
    }
    char m3u8_name[512] = { 0 };
    if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      snprintf(m3u8_name, 512, "%s/%d/%s_%u_%u.m3u8",
               m_path_name,m_dir_counter, m_base_name,
               m_dir_counter,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else if(m_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      snprintf(m3u8_name, 512, "%s/%s_%u.m3u8",
               m_path_name, m_base_name,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else {
      ERROR("ts muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!file_writer->create_file(m3u8_name))) {
      ERROR("Failed to create file for m3u8 file.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_file_duration = (m_end_packet_pts - m_begin_packet_pts) /
        (270000000LL
         / ((int64_t) (100ULL * m_video_info->scale / m_video_info->rate)));
    if(m_file_duration < 0) {
      ERROR("ts file duration is error, pts is not correct?");
    }
    char ts_name[512] = { 0 };
    char file_content[512] = { 0 };
    if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      snprintf(ts_name, 512, "%d/%s_%u.ts",
               m_dir_counter, m_base_name,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else if(m_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      snprintf(ts_name, 512, "event/%s_%u.ts",
                m_base_name,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else {
      ERROR("ts muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
    snprintf(file_content, 512, "#EXTM3U\n"
             "#EXT-X-ALLOW-CACHE:NO\n"
             "#EXT-X-TARGETDURATION:%u\n"
             "#EXT-X-MEDIA-SEQUENCE:%u\n\n"
             "#EXTINF:%lld,\n"
             "media/media_file/%s\n"
             "#EXT-X-ENDLIST",
             m_config->file_duration + 20,
             m_hls_sequence,
             m_file_duration,
             ts_name);
    if(AM_UNLIKELY(!file_writer->write_file(file_content,
                           strlen(file_content)))) {
      ERROR("Failed to write file to m3u8 file.");
      ret = AM_STATE_ERROR;
      break;
    }
    char link_name[512] = { 0 };
    char temp_name[512] = { 0 };
    if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      snprintf(temp_name, 512, "%s/%d/%s_%u_%u.m3u8",
               m_path_name, m_dir_counter, m_base_name,
               m_dir_counter,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else if(m_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      snprintf(temp_name, 512, "%s/%s_%u.m3u8",
               m_path_name, m_base_name,
               (m_file_counter -1) % m_max_file_num_per_folder);
    } else {
      ERROR("ts muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
    snprintf(link_name, 512, "/webSvr/web/%s_%u_%u.m3u8",
             m_base_name, m_dir_counter,
             (m_file_counter - 1) % m_max_file_num_per_folder);
    if(AM_UNLIKELY((symlink(temp_name, link_name)
        < 0) && (errno != EEXIST))) {
      PERROR("Failed to symlink hls media location.");
      break;
    }
  } while(0);
  if(file_writer) {
    file_writer->close_file();
    file_writer->destroy();
  }
  return ret;
}

AM_STATE AMTsFileWriter::write_data (uint8_t *data_ptr, int data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(data_ptr == NULL || data_len <= 0)) {
      ERROR("Invalid parameter.\n");
      ret = AM_STATE_BAD_PARAM;
      break;
    }

    if (AM_UNLIKELY(m_file_writer == NULL)) {
      ERROR("File writer was not initialized.\n");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_file(data_ptr, data_len) != true)) {
      ERROR("Failed to write data to file.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTsFileWriter::close_file(bool need_sync)
{
  AM_STATE ret = AM_STATE_OK;
  if (m_file_writer && m_file_writer->is_file_open()) {
    m_file_writer->close_file(need_sync);
    if (AM_UNLIKELY(!file_finish_cb())) {
      ERROR("File finish callback function error.");
    }
    if (m_config->hls_enable) {
      if (AM_UNLIKELY(create_m3u8_file() != AM_STATE_OK)) {
        ERROR("Failed to create m3u8 file.");
        ret = AM_STATE_IO_ERROR;
      }
    }
  } else {
    NOTICE("The ts file has been closed already");
  }
  return ret;
}

void AMTsFileWriter::set_begin_packet_pts(int64_t pts)
{
  m_begin_packet_pts = pts;
}

void AMTsFileWriter::set_end_packet_pts(int64_t pts)
{
  m_end_packet_pts = pts;
}

bool AMTsFileWriter::set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
                                           AMFileOperationCB callback)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_callback_lock);
  switch (type) {
    case AM_OPERATION_CB_FILE_FINISH : {
      m_file_finish_cb = callback;
    } break;
    case AM_OPERATION_CB_FILE_CREATE : {
      m_file_create_cb = callback;
    } break;
    default : {
      ERROR("File operation callback type error.");
      ret = false;
    } break;
  }
  return ret;
}

bool AMTsFileWriter::file_finish_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_finish_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_name;
      if (file_name.size() <= 128) {
        file_info.stream_id = m_config->video_id;
        file_info.muxer_id = m_config->muxer_id;
        file_info.type = AM_RECORD_FILE_FINISH_INFO;
        memcpy(file_info.file_name, file_name.c_str(), file_name.size());
        memcpy(file_info.create_time_str, m_time_string, sizeof(m_time_string));
        memset(m_time_string, 0, sizeof(m_time_string));
        if (AM_UNLIKELY(!get_current_time_string(m_time_string,
                                                 sizeof(m_time_string)))) {
          ERROR("Failed to get current time string.");
        }
        memcpy(file_info.finish_time_str, m_time_string, sizeof(m_time_string));
        m_file_finish_cb(file_info);
      } else {
        ERROR("The file name : %s is too longer than 128 bytes.",
              file_name.c_str());
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMTsFileWriter::file_create_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_create_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_name;
      if (file_name.size() <= 128) {
        file_info.stream_id = m_config->video_id;
        file_info.muxer_id = m_config->muxer_id;
        file_info.type = AM_RECORD_FILE_CREATE_INFO;
        memcpy(file_info.file_name, file_name.c_str(), file_name.size());
        memcpy(file_info.create_time_str, m_time_string, sizeof(m_time_string));
        memset(file_info.finish_time_str, 0, sizeof(file_info.finish_time_str));
        m_file_create_cb(file_info);
      } else {
        ERROR("The file name : %s is too longer than 128 bytes.",
              file_name.c_str());
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMTsFileWriter::get_current_time_string(char *time_str, int32_t len)
{
  time_t current = time(NULL);
  if (AM_UNLIKELY(strftime(time_str, len, "%Y%m%d%H%M%S",
                           localtime(&current)) == 0)) {
    ERROR("Date string format error!");
    time_str[0] = '\0';
    return false;
  }

  return true;
}
