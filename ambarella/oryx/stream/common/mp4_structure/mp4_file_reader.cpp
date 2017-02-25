/*******************************************************************************
 * mp4_file_reader.cpp
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
#include "mp4_file_reader.h"

#define BOX_TAG_CREATE(X,Y,Z,N) \
          X | (Y << 8) | (Z << 16) | (N << 24)

Mp4FileReader* Mp4FileReader::create(const char* file_name)
{
  Mp4FileReader* result = new Mp4FileReader();
  if(AM_UNLIKELY(result && (!result->init(file_name)))) {
    ERROR("Failed to create mp4 file reader");
    delete result;
    result = NULL;
  }
  return result;
}

Mp4FileReader::Mp4FileReader() :
    m_file_reader(nullptr)
{
}

Mp4FileReader::~Mp4FileReader()
{
  m_file_reader->destroy();
}

bool Mp4FileReader::init(const char* file_name)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY((m_file_reader = AMIFileReader::create()) == NULL)) {
      ERROR("Failed to create file reader.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!m_file_reader->open_file(file_name))) {
      ERROR("Failed to open file %s", file_name);
      ret = false;
      break;
    }
    INFO("Mp4 file reader open %s success.", file_name);
  } while(0);
  return ret;
}

void Mp4FileReader::destroy()
{
  delete this;
}

bool Mp4FileReader::seek_file(uint32_t offset)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_file_reader->seek_file((int32_t)offset))) {
      ERROR("Failed to seek file.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool Mp4FileReader::advance_file(uint32_t offset)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_file_reader->advance_file(offset))) {
      ERROR("Failed to advance file.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

int Mp4FileReader::read_data(uint8_t *data, uint32_t size)
{
  return m_file_reader->read_file(data, size);
}

bool Mp4FileReader::read_le_u8(uint8_t& value)
{

  bool ret = true;
  do {
    if(AM_UNLIKELY(m_file_reader->read_file(&value, 1) != 1)) {
      ERROR("Failed to read little endian uint8.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_s8(int8_t& value)
{

  bool ret = true;
  do {
    if(AM_UNLIKELY(m_file_reader->read_file(&value, 1) != 1)) {
      ERROR("Failed to read little endian uint8.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_u16(uint16_t& value)
{
  bool ret = true;
  do {
    uint8_t bytes[2] { 0 };
    for(uint8_t i = 0; i < 2; ++ i) {
      if(AM_UNLIKELY(!read_le_u8((bytes[i])))) {
        ERROR("Failed to read little endian uint16");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uint16_t temp = 0xff;
    value = ((temp & bytes[0]) << 8) | (temp & bytes[1]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_s16(int16_t& value)
{
  bool ret = true;
  do {
    int8_t bytes[2] { 0 };
    for(uint8_t i = 0; i < 2; ++ i) {
      if(AM_UNLIKELY(!read_le_s8((bytes[i])))) {
        ERROR("Failed to read little endian uint16");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    int16_t temp = 0xffff;
    value = ((temp & bytes[0]) << 8) | (temp & bytes[1]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_u24(uint32_t& value)
{

  bool ret = true;
  do {
    uint8_t bytes[3] { 0 };
    for(uint8_t i = 0; i < 3; ++ i) {
      if(AM_UNLIKELY(!read_le_u8((bytes[i])))) {
        ERROR("Failed to read little endian uint24");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uint32_t temp = 0xffffffff;
    value = ((temp & bytes[0]) << 16) | ((temp & bytes[1]) << 8)
        | (temp & bytes[2]);
  }while(0);
  return ret;
}

bool Mp4FileReader::read_le_s24(int32_t& value)
{

  bool ret = true;
  do {
    int8_t bytes[3] { 0 };
    for(uint8_t i = 0; i < 3; ++ i) {
      if(AM_UNLIKELY(!read_le_s8((bytes[i])))) {
        ERROR("Failed to read little endian int24");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    int32_t temp = 0xffffffff;
    value = ((temp & bytes[0]) << 16) | ((temp & bytes[1]) << 8)
        | (temp & bytes[2]);
  }while(0);
  return ret;
}

bool Mp4FileReader::read_le_u32(uint32_t& value)
{

  bool ret = true;
  do {
    uint8_t bytes[4] { 0 };
    for(uint8_t i = 0; i < 4; ++ i) {
      if(AM_UNLIKELY(!read_le_u8((bytes[i])))) {
        ERROR("Failed to read little endian uint16");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uint32_t temp = 0xffffffff;
    value = ((temp & (uint32_t)bytes[0]) << 24)
        | ((temp & (uint32_t)bytes[1]) << 16)
        | ((temp & (uint32_t)bytes[2]) << 8)
        | (temp & (uint32_t)bytes[3]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_s32(int32_t& value)
{

  bool ret = true;
  do {
    int8_t bytes[4] { 0 };
    for(uint8_t i = 0; i < 4; ++ i) {
      if(AM_UNLIKELY(!read_le_s8((bytes[i])))) {
        ERROR("Failed to read little endian int32");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    int32_t temp = 0xffffffff;
    value = ((temp & bytes[0]) << 24) | ((temp & bytes[1]) << 16) |
        ((temp & bytes[2]) << 8) | (temp & bytes[3]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_u64(uint64_t& value)
{

  bool ret = true;
  do {
    uint8_t bytes[8] { 0 };
    for(uint8_t i = 0; i < 8; ++ i) {
      if(AM_UNLIKELY(!read_le_u8((bytes[i])))) {
        ERROR("Failed to read little endian uint16");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uint64_t temp = 0xffffffffffffffff;
    value = ((temp & (uint64_t)bytes[0]) << 56) |
        ((temp & (uint64_t)bytes[1] << 48)) |
        ((temp & (uint64_t)bytes[2]) << 40) |
        ((temp & (uint64_t)bytes[3]) << 32) |
        ((temp & (uint64_t)bytes[4]) << 24) |
        ((temp & (uint64_t)bytes[5]) << 16) |
        ((temp & (uint64_t)bytes[6]) << 8) |
        (temp & (uint64_t)bytes[7]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_le_s64(int64_t& value)
{

  bool ret = true;
  do {
    int8_t bytes[8] { 0 };
    for(uint8_t i = 0; i < 8; ++ i) {
      if(AM_UNLIKELY(!read_le_s8((bytes[i])))) {
        ERROR("Failed to read little endian int64");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    int64_t temp = 0xffffffffffffffff;
    value = ((temp & (uint64_t)bytes[0]) << 56) |
        ((temp & (uint64_t)bytes[1] << 48)) |
        ((temp & (uint64_t)bytes[2]) << 40) |
        ((temp & (uint64_t)bytes[3]) << 32) |
        ((temp & (uint64_t)bytes[4]) << 24) |
        ((temp & (uint64_t)bytes[5]) << 16) |
        ((temp & (uint64_t)bytes[6]) << 8) |
        (temp & (uint64_t)bytes[7]);
  } while(0);
  return ret;
}

bool Mp4FileReader::read_box_tag(uint32_t& value)
{
  bool ret = true;
  do {
    char tag[5] = { 0 };
    if(AM_UNLIKELY(m_file_reader->read_file(tag, 4) != 4)) {
      ERROR("Failed to read box tag.");
      ret = false;
      break;
    }
    INFO("box tag is %s", tag);
    value = BOX_TAG_CREATE(tag[0], tag[1], tag[2], tag[3]);
  } while(0);
  return ret;
}

bool Mp4FileReader::tell_file(uint64_t& offset)
{
  return m_file_reader->tell_file(offset);
}

uint32_t Mp4FileReader::get_box_type()
{
  uint32_t type = 0;
  do {
    uint64_t offset = 0;
    if(AM_UNLIKELY(!tell_file(offset))) {
      ERROR("Failed to tell file.");
      break;
    }
    if(AM_UNLIKELY(!m_file_reader->advance_file(4))) {
      ERROR("Failed to advance file.");
      break;
    }
    if(AM_UNLIKELY(!read_box_tag(type))) {
      ERROR("Failed to read box tag.");
      type = 0;
      break;
    }
    if(AM_UNLIKELY(!m_file_reader->seek_file(offset))) {
      ERROR("Failed to seek file.");
      break;
    }
  } while(0);
  return type;
}

uint64_t Mp4FileReader::get_file_size()
{
  return m_file_reader->get_file_size();
}

void Mp4FileReader::close_file()
{
  m_file_reader->close_file();
}

