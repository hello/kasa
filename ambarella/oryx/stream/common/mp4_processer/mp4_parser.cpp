/*******************************************************************************
 * mp4_parser.cpp
 *
 * History:
 *   2015-09-06 - [ccjing] created file
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

#include "mp4_parser.h"
#include "mp4_file_reader.h"

FrameInfo::FrameInfo():
    frame_number(0),
    frame_size(0),
    buffer_size(0),
    frame_data(NULL)
{};

FrameInfo::~FrameInfo()
{
  delete[] frame_data;
}

AMMp4Parser* AMMp4Parser::create(const char* file_path)
{
  AMMp4Parser* result = new AMMp4Parser();
  if(AM_UNLIKELY(result && (!result->init(file_path)))) {
    delete result;
    result = nullptr;
  }
  return result;
}

AMMp4Parser::AMMp4Parser() :
    m_file_offset(0),
    m_file_type_box(nullptr),
    m_free_box(nullptr),
    m_media_data_box(nullptr),
    m_movie_box(nullptr),
    m_file_reader(nullptr)
{
}

bool AMMp4Parser::init(const char* file_path)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY((m_file_reader = Mp4FileReader::create(file_path))
                   == NULL)) {
      ERROR("Failed to create mp4 file reader.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((m_file_type_box = new FileTypeBox()) == NULL)) {
      ERROR("Failed to create file type box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((m_free_box = new FreeBox()) == NULL)) {
      ERROR("Failed to create free box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((m_media_data_box = new MediaDataBox()) == NULL)) {
      ERROR("Failed to create media data box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((m_movie_box = new MovieBox()) == NULL)) {
      ERROR("Failed to create movie box.");
      ret = false;
      break;
    }
  }while(0);

  return  ret;
}

AMMp4Parser::~AMMp4Parser()
{
  m_file_reader->destroy();
  delete m_file_type_box;
  delete m_free_box;
  delete m_media_data_box;
  delete m_movie_box;
}

void AMMp4Parser::destroy()
{
  delete this;
}

DISOM_BOX_TAG AMMp4Parser::get_box_type()
{
  DISOM_BOX_TAG box_type = DISOM_BOX_TAG_NULL;
  uint8_t buffer[4] = { 0 };
  do {
    if(AM_UNLIKELY(!m_file_reader->advance_file(4))) {
      ERROR("Failed to advance file.");
      break;
    }
    if(AM_UNLIKELY(m_file_reader->read_data(buffer, 4) != 4)) {
      ERROR("Failed to read file");
      break;
    }
    box_type = DISOM_BOX_TAG(DISO_BOX_TAG_CREATE(buffer[0], buffer[1],
                                                 buffer[2], buffer[3]));
    if(AM_UNLIKELY(!m_file_reader->seek_file(m_file_offset))) {
      ERROR("Failed to seek file.");
      box_type = DISOM_BOX_TAG_NULL;
      break;
    }
  } while(0);
  return box_type;
}

bool AMMp4Parser::parse_file()
{
  bool ret = true;
  INFO("begin to parse mp4 file");
  do {
    switch (get_box_type()) {
      case DISOM_FILE_TYPE_BOX_TAG :
        INFO("begin to parse file type box");
        ret = parse_file_type_box();
        m_file_offset += m_file_type_box->m_base_box.m_size;
        if(ret) {
          INFO("parse file type box success.");
        }
        break;
      case DISOM_MEDIA_DATA_BOX_TAG :
        INFO("begin to parse media data box.");
        ret = parse_media_data_box();
        m_file_offset += m_media_data_box->m_base_box.m_size;
        if(ret) {
          INFO("parse media data box success.");
        }
        break;
      case DISOM_MOVIE_BOX_TAG :
        INFO("begin to parse movie box");
        ret = parse_movie_box();
        m_file_offset += m_movie_box->m_base_box.m_size;
        if(ret) {
          INFO("parse movie box success.");
        }
        break;
      case DISOM_FREE_BOX_TAG :
        INFO("begin to parse free box.");
        ret = parse_free_box();
        m_file_offset += m_free_box->m_base_box.m_size;
        if(ret) {
          INFO("parse free box success.");
        }
        break;
      default :
        ERROR("box type error.");
        ret = false;
        break;
    }
    if(!ret) {
      ERROR("Parse mp4 file error.");
      break;
    }
  } while(((uint64_t)m_file_offset) < m_file_reader->get_file_size());
  return ret;
}

bool AMMp4Parser::parse_file_type_box()
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_file_type_box->read_file_type_box(m_file_reader))) {
      ERROR("Parse file type box error.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMp4Parser::parse_media_data_box()
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_media_data_box->read_media_data_box(m_file_reader))) {
      ERROR("parse media data box error.");
      ret = false;
      break;
    }
    /*skip audio and video data*/
    if(m_media_data_box->m_base_box.m_size > sizeof(MediaDataBox)) {
      if(AM_UNLIKELY(!m_file_reader->advance_file(
          m_media_data_box->m_base_box.m_size - sizeof(MediaDataBox)))) {
        ERROR("Failed to advance file.");
        ret = false;
        break;
      }
    }
  }while(0);
  return ret;
}

bool AMMp4Parser::parse_movie_box()
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_movie_box->read_movie_box(m_file_reader))) {
      ERROR("Failed to read movie box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMp4Parser::parse_free_box()
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_free_box->read_free_box(m_file_reader))) {
      ERROR("Failed to read free box.");
      ret = false;
      break;
    }
    //skip empty bytes
    if(m_free_box->m_base_box.m_size > sizeof(FreeBox)) {
      if(AM_UNLIKELY(!m_file_reader->advance_file(
          m_free_box->m_base_box.m_size - sizeof(FreeBox)))) {
        ERROR("Failed to advance file in parse free box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMMp4Parser::get_video_frame(FrameInfo& frame_info)
{
  bool ret = true;
  do {
    if(frame_info.frame_number <= 0) {
      ERROR("frame number is zero.");
      ret = false;
      break;
    }
    SampleSizeBox& sample_size_box = m_movie_box->m_video_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    frame_info.frame_size = sample_size_box.m_entry_size[frame_info.frame_number -1];
    if((frame_info.frame_data == NULL) ||
        (frame_info.buffer_size < frame_info.frame_size)) {
      delete[] frame_info.frame_data;
      frame_info.frame_data = new uint8_t[frame_info.frame_size + 1];
      if(AM_UNLIKELY(!frame_info.frame_data)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      frame_info.buffer_size = frame_info.frame_size + 1;
    }
    ChunkOffsetBox& chunk_offset_box = m_movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    uint32_t offset =
        chunk_offset_box.m_chunk_offset[frame_info.frame_number -1];
    if(!m_file_reader->seek_file(offset)) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
    if(m_file_reader->read_data(frame_info.frame_data,
                        frame_info.frame_size) != (int)frame_info.frame_size) {
      ERROR("Failed to read data from mp4 file.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMp4Parser::get_audio_frame(FrameInfo& frame_info)
{
  bool ret = true;
  do {
    if(frame_info.frame_number <= 0) {
      ERROR("frame number is zero.");
      ret = false;
      break;
    }
    SampleSizeBox& sample_size_box = m_movie_box->m_audio_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    frame_info.frame_size = sample_size_box.m_entry_size[frame_info.frame_number -1];
    if((frame_info.frame_data == NULL) ||
        (frame_info.buffer_size < frame_info.frame_size)) {
      delete[] frame_info.frame_data;
      frame_info.frame_data = new uint8_t[frame_info.frame_size + 1];
      if(AM_UNLIKELY(!frame_info.frame_data)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      frame_info.buffer_size = frame_info.frame_size + 1;
    }
    ChunkOffsetBox& chunk_offset_box = m_movie_box->m_audio_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    uint32_t offset =
        chunk_offset_box.m_chunk_offset[frame_info.frame_number -1];
    if(!m_file_reader->seek_file(offset)) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
    if(m_file_reader->read_data(frame_info.frame_data,
                                frame_info.frame_size) != (int)frame_info.frame_size) {
      ERROR("Failed to read data from mp4 file.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

FileTypeBox* AMMp4Parser::get_file_type_box()
{
  return m_file_type_box;
}

MediaDataBox* AMMp4Parser::get_media_data_box()
{
  return m_media_data_box;
}

MovieBox* AMMp4Parser::get_movie_box()
{
  return m_movie_box;
}
