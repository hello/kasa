/*******************************************************************************
 * mp4_file_writer.cpp
 *
 * History:
 *   2015-09-07 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_file_sink_if.h"
#include "am_file.h"
#include "mp4_file_writer.h"

#include <libgen.h>
#include <arpa/inet.h>

Mp4FileWriter* Mp4FileWriter::create(const char *file_name)
{
  Mp4FileWriter* result = new Mp4FileWriter();
  if(AM_UNLIKELY(result && (!result->init(file_name)))) {
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

bool Mp4FileWriter::init(const char *file_name)
{
  bool ret = true;
  char *copy_name = NULL;
  char *dir_name = NULL;
  do {
    if(AM_UNLIKELY((m_file_writer = AMIFileWriter::create()) == NULL)) {
      ERROR("Failed to create file writer.");
      ret = false;
      break;
    }
    copy_name = amstrdup(file_name);
    dir_name = dirname(copy_name);
    if (!AMFile::exists(dir_name)) {
      if (!AMFile::create_path(dir_name)) {
        ERROR("Failed to create file path: %s!", dir_name);
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!m_file_writer->create_file(file_name))) {
      ERROR("Failed to create file %s", file_name);
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_file_writer->set_buf(IO_TRANSFER_BLOCK_SIZE))) {
      WARN("Failed to set IO buffer.\n");
    }
  } while(0);
  delete[] copy_name;
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
    if(AM_UNLIKELY(m_file_writer == NULL)) {
      ERROR("File writer was not initialized.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!m_file_writer->write_file(data, data_len))) {
      ERROR("Failed to write data to file");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool Mp4FileWriter::seek_data(uint32_t offset, uint32_t whence)
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
  return write_data((uint8_t*)w, 2);
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
  return write_data((uint8_t*)w, 3);
}

bool Mp4FileWriter::write_be_u32(uint32_t value)
{
  uint32_t re_value = htonl(value);
  return write_data((uint8_t*)(&re_value), 4);
}

bool Mp4FileWriter::write_be_s32(int32_t value)
{
  int8_t w[4];
  w[3] = value & 0x000000ff;
  w[2] = (value >> 8) & 0x000000ff;
  w[1] = (value >> 16) & 0x000000ff;
  w[0] = (value >> 24) & 0x000000ff;
  return write_data((uint8_t*)(w), 4);
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
  return write_data((uint8_t*)buf, 8);
}

void Mp4FileWriter::close_file()
{
  m_file_writer->close_file();
}
