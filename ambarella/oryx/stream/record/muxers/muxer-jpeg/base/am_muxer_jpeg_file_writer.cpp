/*******************************************************************************
 * am_muxer_jpeg_file_writer.cpp
 *
 * History:
 *   2015-10-8 - [ccjing] created file
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
#include "am_muxer_jpeg_file_writer.h"
#include "am_muxer_jpeg_config.h"
#include "am_file_sink_if.h"
#include "am_file.h"

#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>

#ifndef EXTRA_NAME_LEN
#define EXTRA_NAME_LEN 128
#endif

AMJpegFileWriter* AMJpegFileWriter::create (
    AMMuxerCodecJpegConfig *muxer_jpeg_config)
{
  AMJpegFileWriter* result = new AMJpegFileWriter();
  if(AM_UNLIKELY(result && (result->init(muxer_jpeg_config)
      != AM_STATE_OK))) {
    delete result;
    result = NULL;
  }
  return result;
}

AMJpegFileWriter::AMJpegFileWriter()
{}

AMJpegFileWriter::~AMJpegFileWriter()
{
  AM_DESTROY(m_file_writer);
  AM_DESTROY(m_exif_file_writer);
  delete[] m_file_name;
  delete[] m_tmp_file_name;
  delete[] m_exif_tmp_file_name;
  delete[] m_path_name;
  delete[] m_base_name;
}

void AMJpegFileWriter::destroy()
{
  delete this;
}

AM_STATE AMJpegFileWriter::init(AMMuxerCodecJpegConfig *muxer_jpeg_config)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY((m_file_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create file writer");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_exif_file_writer = AMIFileWriter::create()) ==
        nullptr)) {
      ERROR("Failed to create exif file writer");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config = muxer_jpeg_config;
    if (muxer_jpeg_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      m_name = "JpegNormalFileWriter";
    } else if (muxer_jpeg_config->muxer_attr == AM_MUXER_FILE_EVENT) {
      m_name = "JpegEventFileWriter";
    } else {
      ERROR("Muxer attr error in Jpeg file writer.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);

  return ret;
}

AM_STATE AMJpegFileWriter::set_media_sink(const char* file_name)
{
  char *copy_name = NULL;
  char *str = NULL;
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(file_name == NULL)) {
      ERROR("File's name should not be empty in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    INFO("File_name is %s in %s", file_name, m_name.c_str());
    delete[] m_file_name;
    if (AM_UNLIKELY(NULL == (m_file_name = amstrdup(file_name)))) {
      ERROR("Failed to duplicate file name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    delete[] m_tmp_file_name;
    if (AM_UNLIKELY((m_tmp_file_name = new char [strlen (file_name) +
                                            EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for tmp name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_tmp_file_name, 0, strlen(file_name) + EXTRA_NAME_LEN);

    delete[] m_exif_tmp_file_name;
    if (AM_UNLIKELY((m_exif_tmp_file_name = new char [strlen (file_name) +
                                            EXTRA_NAME_LEN]) == nullptr)) {
      ERROR("Failed to allocate memory for exif name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_exif_tmp_file_name, 0, strlen(file_name) + EXTRA_NAME_LEN);

    /* Allocate memory for copyName */
    if (AM_UNLIKELY((copy_name = new char [strlen (file_name) +
                                           EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for copy name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }

    /* Allocate memory for mpPathName */
    delete[] m_path_name;
    if (AM_UNLIKELY((m_path_name = new char [strlen (file_name) +
                                             EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for m_path_name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch path name from file_name. */
      strcpy(copy_name, file_name);
      str = dirname(copy_name);
      strcpy(m_path_name, str);
      INFO("Directory's path: %s in %s", m_path_name, m_name.c_str());
    }

    /* Allocate memory for mpBaseName */
    delete[] m_base_name;
    if (AM_UNLIKELY((m_base_name = new char [strlen (file_name)]) == NULL)) {
      ERROR("Failed to allocate memory for m_base_name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    } else {
      /* Fetch base name from file_name. */
      strcpy(copy_name, file_name);
      str = basename(copy_name);
      strcpy(m_base_name, str);
      INFO("Base name: %s in %s", m_base_name, m_name.c_str());
    }
    m_file_counter = 0;
    m_exif_file_counter = 0;
  } while (0);

  if (AM_LIKELY(copy_name)) {
    delete[] copy_name;
    copy_name = NULL;
  }
  return ret;
}

AM_STATE AMJpegFileWriter::write_data(uint8_t* data, uint32_t data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(create_next_file() != AM_STATE_OK)) {
      ERROR("Failed to create next jpeg file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(data == NULL || data_len == 0)) {
      ERROR("Invalid parameter: %u bytes data at %p in %s.",
            data_len, data, m_name.c_str());
      break;
    }
    if(AM_UNLIKELY(m_file_writer == NULL)) {
      ERROR("File writer was not initialized in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_file_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_file_writer->flush_file())) {
      ERROR("Failed to flush file in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    m_file_writer->close_file(true);
    if (!file_operation_callback()) {
      ERROR("File operation callback error.");
    }
    if (m_config->max_file_number > 0) {
      if(m_file_counter >= m_config->max_file_number) {
        if(AM_UNLIKELY(delete_oldest_file() != AM_STATE_OK)) {
          ERROR("Failed to delete oldest file in %s.", m_name.c_str());
        }
      }
    }
    ++ m_file_counter;
  } while (0);
  return ret;
}

AM_STATE AMJpegFileWriter::write_exif_data(uint8_t* data, uint32_t data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(create_exif_next_file() != AM_STATE_OK)) {
      ERROR("Failed to create next exif file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(data == nullptr || data_len == 0)) {
      ERROR("Invalid parameter: %u bytes data at %p in %s.",
            data_len, data, m_name.c_str());
      break;
    }
    if(AM_UNLIKELY(m_exif_file_writer == nullptr)) {
      ERROR("File writer was not initialized in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_exif_file_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_exif_file_writer->flush_file())) {
      ERROR("Failed to flush file in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    m_exif_file_writer->close_file(true);
    if (m_config->max_file_number > 0) {
      if(m_exif_file_counter >= m_config->max_file_number) {
        if(AM_UNLIKELY(delete_exif_oldest_file() != AM_STATE_OK)) {
          ERROR("Failed to delete oldest exif file in %s.", m_name.c_str());
        }
      }
    }
    ++ m_exif_file_counter;

  } while(0);

  return ret;
}

bool AMJpegFileWriter::set_file_operation_callback(AMFileOperationCB callback)
{
  AUTO_MEM_LOCK(m_callback_lock);
  m_file_operation_cb = callback;
  return true;
}

std::string AMJpegFileWriter::get_current_file_name()
{
  return std::string(m_tmp_file_name);
}

AM_STATE AMJpegFileWriter::create_next_file()
{
  AM_STATE ret = AM_STATE_OK;
  char* copy_name = NULL;
  char* dir_name = NULL;
  do {
    if(m_file_counter >= 65535) {
      m_file_counter = 0;
    }
    snprintf(m_tmp_file_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.jpg",
             m_path_name, m_base_name,
             m_file_counter);
    copy_name = amstrdup(m_tmp_file_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s in %s!", dir_name, m_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(!m_file_writer->create_file(m_tmp_file_name))) {
      ERROR("Failed to create %s: %s in %s",
            m_tmp_file_name, strerror (errno), m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  delete[] copy_name;
  return ret;
}

AM_STATE AMJpegFileWriter::delete_oldest_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    snprintf(m_tmp_file_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.jpg",
             m_path_name, m_base_name,
             m_file_counter - m_config->max_file_number);
    if(AM_UNLIKELY(unlink(m_tmp_file_name) < 0)) {
      PERROR("unlink");
      ret = AM_STATE_ERROR;
      break;
    }
  } while(0);
  return ret;
}

AM_STATE AMJpegFileWriter::create_exif_next_file()
{
  AM_STATE ret = AM_STATE_OK;
  char* copy_name = nullptr;
  char* dir_name = nullptr;
  do {
    if(m_exif_file_counter >= 65535) {
      m_exif_file_counter = 0;
    }
    snprintf(m_exif_tmp_file_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.jpg.exif",
             m_path_name, m_base_name,
             m_exif_file_counter);
    copy_name = amstrdup(m_exif_tmp_file_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s in %s!", dir_name, m_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(!m_exif_file_writer->create_file(m_exif_tmp_file_name))) {
      ERROR("Failed to create %s: %s in %s",
            m_exif_tmp_file_name, strerror (errno), m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  delete[] copy_name;
  return ret;
}

AM_STATE AMJpegFileWriter::delete_exif_oldest_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    snprintf(m_exif_tmp_file_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.jpg.exif",
             m_path_name, m_base_name,
             m_exif_file_counter - m_config->max_file_number);
    if(AM_UNLIKELY(unlink(m_exif_tmp_file_name) < 0)) {
      PERROR("unlink");
      ret = AM_STATE_ERROR;
      break;
    }
  } while(0);
  return ret;
}

bool AMJpegFileWriter::file_operation_callback()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_operation_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_file_name;
      if (file_name.size() <= 128) {
        file_info.stream_id = m_config->video_id;
        file_info.muxer_id = m_config->muxer_id;
        file_info.type = AM_RECORD_FILE_FINISH_INFO;
        memcpy(file_info.file_name, file_name.c_str(), file_name.size());
        if (AM_UNLIKELY(!(get_current_time_string(m_time_string,
                                                  sizeof(m_time_string),
                                                  "%Y%m%d%H%M%S")))) {
          ERROR("Get current time string error in %s.", m_name.c_str());
          ret = false;
          break;
        }
        memcpy(file_info.create_time_str, m_time_string, sizeof(m_time_string));
        memcpy(file_info.finish_time_str, m_time_string, sizeof(m_time_string));
        m_file_operation_cb(file_info);
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

bool AMJpegFileWriter::get_current_time_string(char *time_str, int32_t len,
                                               const char *format)
{
  time_t current = time(NULL);
  if (AM_UNLIKELY(strftime(time_str, len, format, localtime(&current))
                  == 0)) {
    ERROR("Date string format error!");
    time_str[0] = '\0';
    return false;
  }
  return true;
}
