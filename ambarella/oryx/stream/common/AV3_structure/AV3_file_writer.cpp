/*******************************************************************************
 * AV3_file_writer.cpp
 *
 * History:
 *   2016-08-26 - [ccjing] created file
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
#include "am_log.h"
#include "am_define.h"
#include "am_file.h"
#include "am_file_sink_if.h"
#include "AV3_file_writer.h"
#include <libgen.h>
#include <arpa/inet.h>

#define BUFFER_INCREMENT (512 * 1024)
AV3FileWriter* AV3FileWriter::create()
{
  AV3FileWriter* result = new AV3FileWriter();
  if (AM_UNLIKELY(result && (!result->init()))) {
    ERROR("Failed to create AV3 file writer.");
    delete result;
    result = NULL;
  }
  return result;
}

AV3FileWriter::AV3FileWriter()
{
}

AV3FileWriter::~AV3FileWriter()
{
  m_file_writer->destroy();
  delete[] m_buffer;
}

bool AV3FileWriter::init()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((m_file_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create file writer.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AV3FileWriter::create_file(const char* file_name)
{
  bool ret = true;
  char *copy_name = NULL;
  char *dir_name = NULL;
  do {
    copy_name = amstrdup(file_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s!", dir_name);
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!m_file_writer->create_file(file_name))) {
      ERROR("Failed to create file %s", file_name);
      ret = false;
      break;
    }
  } while (0);
  delete[] copy_name;
  m_buf_pos = 0;
  return ret;
}

bool AV3FileWriter::set_buf(uint32_t size)
{
  bool ret = true;
  if (AM_UNLIKELY(!m_file_writer->set_buf(IO_TRANSFER_BLOCK_SIZE))) {
    ERROR("Failed to set IO buffer.\n");
    ret = false;
  }
  return ret;
}

void AV3FileWriter::destroy()
{
  delete this;
}

bool AV3FileWriter::write_data(const uint8_t *data, uint32_t data_len,
                               bool copy_to_mem)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(data == nullptr || data_len == 0)) {
      WARN("Invalid parameter: %u bytes data at %p.", data_len, data);
      break;
    }
    if (AM_UNLIKELY(m_file_writer == nullptr)) {
      ERROR("File writer was not initialized.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_file_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file");
      ret = false;
      break;
    }
    if (copy_to_mem) {
      if (!check_buf(data_len)) {
        ERROR("Failed to check buffer.");
        ret = false;
        break;
      }
      memcpy(m_buffer + m_buf_pos, data, data_len);
      m_buf_pos += data_len;
    }
  } while (0);
  return ret;
}

bool AV3FileWriter::seek_file(uint32_t offset, uint32_t whence)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_file_writer->seek_file(offset, whence))) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AV3FileWriter::tell_file(uint64_t& offset)
{
  return m_file_writer->tell_file(offset);
}

bool AV3FileWriter::write_be_u8(uint8_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!write_data(&value, sizeof(uint8_t), copy_to_mem)) {
      ERROR("Write data error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_s8(int8_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!write_data((uint8_t*)(&value), sizeof(int8_t), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_u16(uint16_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    uint16_t re_value = htons(value);
    if (!write_data((uint8_t*)(&re_value), sizeof(uint16_t), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;

}

bool AV3FileWriter::write_be_s16(int16_t value, bool copy_to_mem)
{
  bool ret = true;
  int8_t w[2];
  do {
    w[1] = (value & 0x00ff);
    w[0] = (value >> 8) & 0x00ff;
    if (!write_data((uint8_t*) w, sizeof(w), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_u24(uint32_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    uint8_t w[3];
    w[2] = value;     //(data&0x0000FF);
    w[1] = value >> 8;  //(data&0x00FF00)>>8;
    w[0] = value >> 16; //(data&0xFF0000)>>16;
    if (!write_data(w, sizeof(w), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_s24(int32_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    int8_t w[3];
    w[2] = value & 0x000000ff;     //(data&0x0000FF);
    w[1] = (value >> 8) & 0x000000ff;  //(data&0x00FF00)>>8;
    w[0] = (value >> 16) & 0x000000ff; //(data&0xFF0000)>>16;
    if (!write_data((uint8_t*) w, sizeof(w), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_u32(uint32_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    uint32_t re_value = htonl(value);
    if (!write_data((uint8_t*) (&re_value), sizeof(uint32_t), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_s32(int32_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    int8_t w[4];
    w[3] = value & 0x000000ff;
    w[2] = (value >> 8) & 0x000000ff;
    w[1] = (value >> 16) & 0x000000ff;
    w[0] = (value >> 24) & 0x000000ff;
    if (!write_data((uint8_t*) (w), sizeof(w), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_u64(uint64_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    uint8_t buf[8] = { 0 };
    buf[0] = (value >> 56) & 0xff;
    buf[1] = (value >> 48) & 0xff;
    buf[2] = (value >> 40) & 0xff;
    buf[3] = (value >> 32) & 0xff;
    buf[4] = (value >> 24) & 0xff;
    buf[5] = (value >> 16) & 0xff;
    buf[6] = (value >> 8) & 0xff;
    buf[7] = value & 0xff;
    if (!write_data(buf, sizeof(buf), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::write_be_s64(int64_t value, bool copy_to_mem)
{
  bool ret = true;
  do {
    int8_t buf[8] = { 0 };
    buf[0] = (value >> 56) & 0xff;
    buf[1] = (value >> 48) & 0xff;
    buf[2] = (value >> 40) & 0xff;
    buf[3] = (value >> 32) & 0xff;
    buf[4] = (value >> 24) & 0xff;
    buf[5] = (value >> 16) & 0xff;
    buf[6] = (value >> 8) & 0xff;
    buf[7] = value & 0xff;
    if (!write_data((uint8_t*) buf, sizeof(buf), copy_to_mem)) {
      ERROR("Failed to write data.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3FileWriter::flush_file()
{
  return m_file_writer->flush_file();
}

bool AV3FileWriter::is_file_open()
{
  return m_file_writer->is_file_open();
}

void AV3FileWriter::close_file(bool need_sync)
{
  m_file_writer->close_file(need_sync);
}

uint64_t AV3FileWriter::get_file_size()
{
  return m_file_writer->get_file_size();
}

bool AV3FileWriter::check_buf(uint32_t data_len)
{
  bool ret = true;
  do {
    if (data_len <= 0) {
      WARN("The data len is zero.");
      break;
    }
    if (data_len >= (m_buffer_len - m_buf_pos)) {

      uint32_t multiple = (data_len / BUFFER_INCREMENT) + 1;
      uint8_t *tmp = new uint8_t[m_buffer_len + BUFFER_INCREMENT * multiple];
      if (!tmp) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (m_buffer) {
        memcpy(tmp, m_buffer, m_buf_pos);
      }
      delete[] m_buffer;
      m_buffer = tmp;
      m_buffer_len += BUFFER_INCREMENT * multiple;
    }
  } while(0);
  return ret;
}

const uint8_t* AV3FileWriter::get_data_buffer()
{
  return m_buffer;
}

uint32_t AV3FileWriter::get_data_size()
{
  return m_buf_pos;
}

void AV3FileWriter::clear_buffer(bool free_mem)
{
  m_buf_pos = 0;
  if (free_mem) {
    delete[] m_buffer;
    m_buffer = nullptr;
    m_buffer_len = 0;
  }
}

