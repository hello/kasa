/*******************************************************************************
 * am_muxer_mjpeg_file_writer.cpp
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
#include "am_log.h"
#include "am_define.h"

#include "am_muxer_codec_info.h"
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

#include "am_muxer_periodic_mjpeg_writer.h"
#include "am_muxer_periodic_mjpeg_config.h"

#ifndef EXTRA_NAME_LEN
#define EXTRA_NAME_LEN 128
#endif

AMPeriodicMjpegWriter* AMPeriodicMjpegWriter::create (
    AMMuxerCodecPeriodicMjpegConfig *config)
{
  AMPeriodicMjpegWriter* result = new AMPeriodicMjpegWriter();
  if(AM_UNLIKELY(result && (result->init(config)
      != AM_STATE_OK))) {
    delete result;
    result = NULL;
  }
  return result;
}

AMPeriodicMjpegWriter::AMPeriodicMjpegWriter()
{
}

AMPeriodicMjpegWriter::~AMPeriodicMjpegWriter()
{
  AM_DESTROY(m_jpeg_writer);
  AM_DESTROY(m_text_writer);
  delete[] m_file_name;
  delete[] m_tmp_jpeg_name;
  delete[] m_tmp_text_name;
  delete[] m_path_name;
  delete[] m_base_name;
}

void AMPeriodicMjpegWriter::destroy()
{
  delete this;
}

AM_STATE AMPeriodicMjpegWriter::init(AMMuxerCodecPeriodicMjpegConfig *config)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY((m_jpeg_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create jpeg writer");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_text_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create text writer");
      ret = AM_STATE_ERROR;
      break;
    }
    m_config = config;
    m_name = "PeriodicJpegWriter";
  } while (0);
  return ret;
}

AM_STATE AMPeriodicMjpegWriter::set_file_name(const char* file_name)
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
    delete[] m_tmp_jpeg_name;
    if (AM_UNLIKELY((m_tmp_jpeg_name = new char [strlen (file_name) +
                                            EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for tmp jpeg name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_tmp_jpeg_name, 0, strlen(file_name) + EXTRA_NAME_LEN);

    delete[] m_tmp_text_name;
    if (AM_UNLIKELY((m_tmp_text_name = new char [strlen (file_name) +
                                                 EXTRA_NAME_LEN]) == NULL)) {
      ERROR("Failed to allocate memory for tmp text name in %s", m_name.c_str());
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_tmp_text_name, 0, strlen(file_name) + EXTRA_NAME_LEN);

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
  } while (0);

  if (AM_LIKELY(copy_name)) {
    delete[] copy_name;
    copy_name = NULL;
  }
  return ret;
}

AM_STATE AMPeriodicMjpegWriter::write_jpeg_data(uint8_t* data, uint32_t data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(data == NULL || data_len == 0)) {
      ERROR("Invalid parameter: %u bytes data at %p in %s.",
            data_len, data, m_name.c_str());
      break;
    }
    if(AM_UNLIKELY(m_jpeg_writer == NULL)) {
      ERROR("Jpeg writer was not initialized in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_jpeg_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMPeriodicMjpegWriter::write_text_data(uint8_t* data, uint32_t data_len)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(data == NULL || data_len == 0)) {
      ERROR("Invalid parameter: %u bytes data at %p in %s.",
            data_len, data, m_name.c_str());
      break;
    }
    if(AM_UNLIKELY(m_text_writer == NULL)) {
      ERROR("Text writer was not initialized in %s.", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!m_text_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file in %s", m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

bool AMPeriodicMjpegWriter::file_finish_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_finish_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_jpeg_name;
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

bool AMPeriodicMjpegWriter::file_create_cb()
{
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_callback_lock);
    if (m_file_create_cb) {
      AMRecordFileInfo file_info;
      std::string file_name = m_tmp_jpeg_name;
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

void AMPeriodicMjpegWriter::close_files()
{
  if (m_jpeg_writer && m_jpeg_writer->is_file_open()) {
    m_jpeg_writer->close_file(true);
    if (!file_finish_cb()) {
      ERROR("File finish callback error.");
    }
  } else {
    NOTICE("The periodic jpeg file has been closed already.");
  }
  if (m_text_writer->is_file_open()) {
    m_text_writer->close_file(true);
  }
}

bool AMPeriodicMjpegWriter::is_file_open()
{
  return (m_jpeg_writer->is_file_open() && m_text_writer->is_file_open());
}

AM_STATE AMPeriodicMjpegWriter::create_next_files()
{
  AM_STATE ret = AM_STATE_OK;
  char* copy_name = NULL;
  char* dir_name = NULL;
  do {
    if(m_file_counter >= 65535) {
      m_file_counter = 0;
    }
    snprintf(m_tmp_jpeg_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.mjpeg",
             m_path_name, m_base_name,
             m_file_counter);
    snprintf(m_tmp_text_name,
             strlen(m_file_name) + EXTRA_NAME_LEN,
             "%s/%s_%u.txt",
             m_path_name, m_base_name,
             m_file_counter);
    copy_name = amstrdup(m_tmp_jpeg_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s in %s!", dir_name, m_name.c_str());
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(!m_jpeg_writer->create_file(m_tmp_jpeg_name))) {
      ERROR("Failed to create %s: %s in %s",
            m_tmp_jpeg_name, strerror (errno), m_name.c_str());
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
    if (AM_UNLIKELY(!m_text_writer->create_file(m_tmp_text_name))) {
      ERROR("Failed to create %s: %s in %s",
            m_tmp_text_name, strerror (errno), m_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    ++ m_file_counter;
  } while (0);
  delete[] copy_name;
  return ret;
}

bool AMPeriodicMjpegWriter::set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
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

bool AMPeriodicMjpegWriter::get_current_time_string(char *time_str, int32_t len)
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
