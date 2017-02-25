/*******************************************************************************
 * am_muxer_AV3_file_writer.cpp
 *
 * History:
 *   2016-08-30 - [ccjing] created file
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
#include "am_log.h"
#include "am_define.h"
#include "am_muxer_codec_info.h"
#include "AV3_file_writer.h"
#include "am_file_sink_if.h"
#include "am_file.h"
#include "AV3_struct_defs.h"
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include "am_utils.h"
#include "am_muxer_av3_config.h"
#include "am_muxer_av3_file_writer.h"

#ifndef AV3_EXTRA_NAME_LEN
#define AV3_EXTRA_NAME_LEN 128
#endif

AMAv3FileWriter* AMAv3FileWriter::create (
    AMMuxerCodecAv3Config *muxer_AV3_config, AM_VIDEO_INFO *video_info)
{
  AMAv3FileWriter* result = new AMAv3FileWriter();
  if(AM_UNLIKELY(result && (result->init(muxer_AV3_config, video_info)
      != AM_STATE_OK))) {
    delete result;
    result = NULL;
  }
  return result;
}

AMAv3FileWriter::AMAv3FileWriter()
{}

AMAv3FileWriter::~AMAv3FileWriter()
{
  std::ofstream file;
  if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
    file.open("/etc/oryx/stream/muxer/muxer_file.conf");
    file << "dir_number:" << m_dir_counter << std::endl;
    file << "file_number:" << m_file_counter << std::endl;
    file.close();
  }
  AM_DESTROY(m_file_writer);
  delete[] m_file_name;
  delete[] m_tmp_file_name;
  delete[] m_path_name;
  delete[] m_base_name;
}

void AMAv3FileWriter::destroy()
{
  delete this;
}

AM_STATE AMAv3FileWriter::init(AMMuxerCodecAv3Config *muxer_AV3_config,
                               AM_VIDEO_INFO *video_info)
{
  AM_STATE ret = AM_STATE_OK;
  std::ifstream file;
  std::string read_line;
  do {
    if (AM_UNLIKELY((m_file_writer = AV3FileWriter::create()) == NULL)) {
      ERROR("Failed to create file writer");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config = muxer_AV3_config;
    if (video_info) {
      memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
    } else {
      ERROR("The video info is null.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(muxer_AV3_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
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

bool AMAv3FileWriter::set_video_info(AM_VIDEO_INFO *vinfo)
{
  bool ret = true;
  if (vinfo) {
    memcpy(&m_video_info, vinfo, sizeof(AM_VIDEO_INFO));
  } else {
    ERROR("The video info is null.");
    ret = false;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::set_media_sink(const char* file_name)
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
    delete[] m_tmp_file_name;
    if (AM_UNLIKELY((m_tmp_file_name = new char [strlen (file_name) +
                                                 AV3_EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for tmp name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_tmp_file_name, 0, strlen(file_name) + AV3_EXTRA_NAME_LEN);

    /* Allocate memory for copyName */
    if (AM_UNLIKELY((copy_name = new char [strlen (file_name) +
                                           AV3_EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for copy name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    }

    /* Allocate memory for mpPathName */
    delete[] m_path_name;
    if (AM_UNLIKELY((m_path_name = new char [strlen (file_name) +
                                             AV3_EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for m_path_name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch path name from file_name. */
      strcpy(copy_name, file_name);
      str = dirname(copy_name);
      strcpy(m_path_name, str);
      INFO("Directory's path: %s", m_path_name);
    }

    /* Allocate memory for mpBaseName */
    delete[] m_base_name;
    if (AM_UNLIKELY((m_base_name = new char [strlen (file_name)]) == NULL)) {
      ERROR("Failed to allocate memory for m_base_name.\n");
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch base name from file_name. */
      strcpy(copy_name, file_name);
      str = basename(copy_name);
      strcpy(m_base_name, str);
      INFO("Base name: %s", m_base_name);
    }
  } while (0);

  if (AM_LIKELY(copy_name)) {
    delete[] copy_name;
    copy_name = NULL;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::create_next_file()
{
  AM_STATE ret = AM_STATE_OK;
  char* copy_name = NULL;
  char* dir_name = NULL;
  do {
    if((m_file_counter % m_config->max_file_num_per_folder) == 0) {
      ++ m_dir_counter;
    }
    if(m_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      snprintf(m_tmp_file_name,
               strlen(m_file_name) + AV3_EXTRA_NAME_LEN,
               "%s/%d/%s_%u_%u.av3",
               m_path_name, m_dir_counter, m_base_name,
               m_dir_counter,
               (m_file_counter ++) % m_config->max_file_num_per_folder);
    } else if(m_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      snprintf(m_tmp_file_name,
               strlen(m_file_name) + AV3_EXTRA_NAME_LEN,
               "%s/%s_%u.av3",
               m_path_name, m_base_name,
               m_file_counter ++);
    } else {
      ERROR("muxer attr error.");
      ret = AM_STATE_ERROR;
      break;
    }
    copy_name = amstrdup(m_tmp_file_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s!", dir_name);
        ret = AM_STATE_IO_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(!m_file_writer->create_file(m_tmp_file_name))) {
      ERROR("Failed to create %s: %s\n", m_tmp_file_name, strerror (errno));
      ret = AM_STATE_IO_ERROR;
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
    m_cur_file_offset = 0;
    if (AM_UNLIKELY(!m_file_writer->set_buf(IO_TRANSFER_BLOCK_SIZE))) {
      NOTICE("Failed to set IO buffer.\n");
    }

    INFO("File create: %s success.\n", m_tmp_file_name);
  } while (0);
  delete[] copy_name;

  return ret;
}

char* AMAv3FileWriter::get_current_file_name()
{
  return m_tmp_file_name;
}

AM_STATE AMAv3FileWriter::write_data(const uint8_t* data, uint32_t data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(data == NULL || data_len == 0)) {
      ERROR("Invalid parameter: %u bytes data at %p.", data_len, data);
      break;
    }
    if(AM_UNLIKELY(m_file_writer == NULL)) {
      ERROR("File writer was not initialized.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_file_writer->write_data(data, data_len) != true)) {
      ERROR("Failed to write data to file");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    m_cur_file_offset += data_len;
  } while (0);
  return ret;
}

AM_STATE AMAv3FileWriter::write_data_direct(uint8_t *data, uint32_t data_len)
{
  AM_STATE state = AM_STATE_IO_ERROR;
  if (AM_LIKELY(AM_STATE_OK == write_data(data, data_len))) {
    m_cur_file_offset += data_len;
    state = m_file_writer->flush_file() ? AM_STATE_OK : AM_STATE_IO_ERROR;
  }

  return state;
}

AM_STATE AMAv3FileWriter::seek_data(uint32_t offset, uint32_t whence)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_file_writer->seek_file(offset, whence) != true)) {
      ERROR("Failed to seek file");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    if (!m_file_writer->tell_file(m_cur_file_offset)) {
      ERROR("Failed to tell file");
      ret = AM_STATE_IO_ERROR;
      break;
    }
  } while (0);
  return ret;
}

bool AMAv3FileWriter::is_file_open()
{
  return m_file_writer->is_file_open();
}

AM_STATE AMAv3FileWriter::write_file_type_box(AV3FileTypeBox& box)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!box.write_file_type_box(m_file_writer)) {
      ERROR("Failed to write file type box.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    m_cur_file_offset += box.m_base_box.m_size;
  } while(0);
  return ret;
}

AM_STATE AMAv3FileWriter::write_media_data_box(AV3MediaDataBox& box)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!box.write_media_data_box(m_file_writer)) {
      ERROR("Failed to write media data box.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    m_cur_file_offset += box.m_base_box.m_size;
  } while(0);
  return ret;
}

AM_STATE AMAv3FileWriter::write_movie_box(AV3MovieBox& box, bool copy_to_mem)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!box.write_movie_box(m_file_writer, copy_to_mem)) {
      ERROR("Failed to write movie box.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    m_cur_file_offset += box.m_base_box.m_size;
  } while(0);
  return ret;
}

AM_STATE AMAv3FileWriter::write_uuid_box(AV3UUIDBox& box, bool copy_to_mem)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!box.write_uuid_box(m_file_writer, copy_to_mem)) {
      ERROR("Failed to write uuid box.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    m_cur_file_offset += box.m_base_box.m_size;
  } while(0);
  return ret;
}

AM_STATE AMAv3FileWriter::write_u8(uint8_t value)
{
  AM_STATE ret = AM_STATE_OK;
  ++ m_cur_file_offset;
  if (!m_file_writer->write_be_u8(value)) {
    ERROR("Failed to write u8");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_s8(int8_t value)
{
  AM_STATE ret = AM_STATE_OK;
  ++ m_cur_file_offset;
  if (!m_file_writer->write_be_s8(value)) {
    ERROR("Failed to write be s8");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_u16(uint16_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 2;
  if (!m_file_writer->write_be_u16(value)) {
    ERROR("Failed to write be u16");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_s16(int16_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 2;
  if (!m_file_writer->write_be_s16(value)) {
    ERROR("Failed to write be s16");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_u24(uint32_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 3;
  if (!m_file_writer->write_be_u24(value)) {
    ERROR("Failed to write be u24");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_s24(int32_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 3;
  if (!m_file_writer->write_be_s24(value)) {
    ERROR("Failed to write be s24");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_u32(uint32_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 4;
  if (!m_file_writer->write_be_u32(value)) {
    ERROR("Failed to write be u32");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_s32(int32_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 4;
  if (!m_file_writer->write_be_s32(value)) {
    ERROR("Failed to write be s32");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_u64(uint64_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 8;
  if (!m_file_writer->write_be_u64(value)) {
    ERROR("Failed to write be u64");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

AM_STATE AMAv3FileWriter::write_be_s64(int64_t value)
{
  AM_STATE ret = AM_STATE_OK;
  m_cur_file_offset += 8;
  if (!m_file_writer->write_be_s64(value)) {
    ERROR("Failed to write be s64");
    ret = AM_STATE_IO_ERROR;
  }
  return ret;
}

uint64_t AMAv3FileWriter::get_file_offset()
{
  return m_cur_file_offset;
}

bool AMAv3FileWriter::get_current_time_string(char *time_str, int32_t len)
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

bool AMAv3FileWriter::file_finish_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_finish_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_file_name;
      if (file_name.size() <= 128) {
        file_info.stream_id = m_config->video_id;
        file_info.muxer_id = m_config->muxer_id;
        file_info.type = AM_RECORD_FILE_FINISH_INFO;
        memcpy(file_info.file_name, file_name.c_str(), file_name.size());
        memcpy(file_info.create_time_str, m_time_string, sizeof(m_time_string));
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

bool AMAv3FileWriter::file_create_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_create_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_file_name;
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

AM_STATE AMAv3FileWriter::close_file(bool need_sync)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_LIKELY(m_file_writer && m_file_writer->is_file_open())) {
    m_file_writer->close_file(need_sync);
    if (AM_UNLIKELY(!file_finish_cb())) {
      ERROR("File finish callback function error.");
    }
  } else {
    NOTICE("The AV3 file has been closed already.");
  }
  return ret;
}

void AMAv3FileWriter::set_begin_packet_pts(int64_t pts)
{
  m_begin_packet_pts = pts;
}

void AMAv3FileWriter::set_end_packet_pts(int64_t pts)
{
  m_end_packet_pts = pts;
}

AV3FileWriter* AMAv3FileWriter::get_file_writer()
{
  return m_file_writer;
}

bool AMAv3FileWriter::set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
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

const uint8_t* AMAv3FileWriter::get_data_buffer()
{
  return m_file_writer->get_data_buffer();
}

uint32_t AMAv3FileWriter::get_data_size()
{
  return m_file_writer->get_data_size();
}

void AMAv3FileWriter::clear_buffer(bool free_mem)
{
  m_file_writer->clear_buffer(free_mem);
}

