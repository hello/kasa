/*******************************************************************************
 * mp4_file_writer.cpp
 *
 * History:
 *   2016-01-05 - [ccjing] created file
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
#include "mp4_file_writer.h"
#include <libgen.h>
#include <arpa/inet.h>

Mp4FileWriter* Mp4FileWriter::create()
{
  Mp4FileWriter* result = new Mp4FileWriter();
  if (AM_UNLIKELY(result && (!result->init()))) {
    ERROR("Failed to create mp4 file writer.");
    delete result;
    result = NULL;
  }
  return result;
}

Mp4FileWriter::Mp4FileWriter() :
    m_file_writer(nullptr)
{
}

Mp4FileWriter::~Mp4FileWriter()
{
  m_file_writer->destroy();
}

bool Mp4FileWriter::init()
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

bool Mp4FileWriter::create_file(const char* file_name)
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
  return ret;
}

bool Mp4FileWriter::set_buf(uint32_t size)
{
  bool ret = true;
  if (AM_UNLIKELY(!m_file_writer->set_buf(IO_TRANSFER_BLOCK_SIZE))) {
    ERROR("Failed to set IO buffer.\n");
    ret = false;
  }
  return ret;
}

void Mp4FileWriter::destroy()
{
  delete this;
}

bool Mp4FileWriter::write_data(uint8_t *data, uint32_t data_len)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(data == NULL || data_len == 0)) {
      WARN("Invalid parameter: %u bytes data at %p.", data_len, data);
      break;
    }
    if (AM_UNLIKELY(m_file_writer == NULL)) {
      ERROR("File writer was not initialized.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_file_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool Mp4FileWriter::seek_file(uint32_t offset, uint32_t whence)
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

bool Mp4FileWriter::tell_file(uint64_t& offset)
{
  return m_file_writer->tell_file(offset);
}

bool Mp4FileWriter::write_be_u8(uint8_t value)
{
  bool ret = true;
  ret = write_data(&value, sizeof(uint8_t));
  return ret;
}

bool Mp4FileWriter::write_be_s8(int8_t value)
{
  return write_data((uint8_t*)(&value), sizeof(int8_t));
}

bool Mp4FileWriter::write_be_u16(uint16_t value)
{
  uint16_t re_value = htons(value);
  return write_data((uint8_t*)(&re_value), 2);
}

bool Mp4FileWriter::write_be_s16(int16_t value)
{
  int8_t w[2];
  w[1] = (value & 0x00ff);
  w[0] = (value >> 8) & 0x00ff;
  return write_data((uint8_t*) w, 2);
}

bool Mp4FileWriter::write_be_u24(uint32_t value)
{
  uint8_t w[3];
  w[2] = value;     //(data&0x0000FF);
  w[1] = value >> 8;  //(data&0x00FF00)>>8;
  w[0] = value >> 16; //(data&0xFF0000)>>16;
  return write_data(w, 3);
}

bool Mp4FileWriter::write_be_s24(int32_t value)
{
  int8_t w[3];
  w[2] = value & 0x000000ff;     //(data&0x0000FF);
  w[1] = (value >> 8) & 0x000000ff;  //(data&0x00FF00)>>8;
  w[0] = (value >> 16) & 0x000000ff; //(data&0xFF0000)>>16;
  return write_data((uint8_t*) w, 3);
}

bool Mp4FileWriter::write_be_u32(uint32_t value)
{
  uint32_t re_value = htonl(value);
  return write_data((uint8_t*) (&re_value), 4);
}

bool Mp4FileWriter::write_be_s32(int32_t value)
{
  int8_t w[4];
  w[3] = value & 0x000000ff;
  w[2] = (value >> 8) & 0x000000ff;
  w[1] = (value >> 16) & 0x000000ff;
  w[0] = (value >> 24) & 0x000000ff;
  return write_data((uint8_t*) (w), 4);
}

bool Mp4FileWriter::write_be_u64(uint64_t value)
{
  uint8_t buf[8] = { 0 };
  buf[0] = (value >> 56) & 0xff;
  buf[1] = (value >> 48) & 0xff;
  buf[2] = (value >> 40) & 0xff;
  buf[3] = (value >> 32) & 0xff;
  buf[4] = (value >> 24) & 0xff;
  buf[5] = (value >> 16) & 0xff;
  buf[6] = (value >> 8) & 0xff;
  buf[7] = value & 0xff;
  return write_data(buf, 8);
}

bool Mp4FileWriter::write_be_s64(int64_t value)
{
  int8_t buf[8] = { 0 };
  buf[0] = (value >> 56) & 0xff;
  buf[1] = (value >> 48) & 0xff;
  buf[2] = (value >> 40) & 0xff;
  buf[3] = (value >> 32) & 0xff;
  buf[4] = (value >> 24) & 0xff;
  buf[5] = (value >> 16) & 0xff;
  buf[6] = (value >> 8) & 0xff;
  buf[7] = value & 0xff;
  return write_data((uint8_t*) buf, 8);
}

bool Mp4FileWriter::flush_file()
{
  return m_file_writer->flush_file();
}

bool Mp4FileWriter::is_file_open()
{
  return m_file_writer->is_file_open();
}

void Mp4FileWriter::close_file(bool need_sync)
{
  m_file_writer->close_file(need_sync);
}

uint64_t Mp4FileWriter::get_file_size()
{
  return m_file_writer->get_file_size();
}
