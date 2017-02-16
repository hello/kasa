/*******************************************************************************
 * iso_base_defs.cpp
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
#include "am_define.h"
#include "am_log.h"

#include "mp4_file_writer.h"
#include "mp4_file_reader.h"
#include "iso_base_defs.h"

WriteInfo::WriteInfo() :
    m_video_duration(0),
    m_audio_duration(0),
    m_video_first_frame(0),
    m_video_last_frame(0),
    m_audio_first_frame(0),
    m_audio_last_frame(0)
{}

bool WriteInfo::set_video_duration(uint64_t video_duration)
{
  bool ret = true;
  if(video_duration > 0) {
    m_video_duration = video_duration;
  } else {
    ERROR("video duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool WriteInfo::set_audio_duration(uint64_t audio_duration)
{
  bool ret = true;
  if(audio_duration > 0) {
    m_audio_duration = audio_duration;
  } else {
    ERROR("audio duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool WriteInfo::set_video_first_frame(uint32_t video_first_frame)
{
  bool ret = true;
  if(video_first_frame > 0) {
    if((m_video_last_frame > 0) && (video_first_frame >= m_video_last_frame)) {
      ERROR("video first frame is bigger than video last frame");
      ret = false;
    } else {
      m_video_first_frame = video_first_frame;
    }
  } else {
    ERROR("video first frame should not be  zero.");
    ret = false;
  }
  return ret;
}

bool WriteInfo::set_video_last_frame(uint32_t video_last_frame)
{
  bool ret = true;
  if(video_last_frame > 0) {
    if((m_video_first_frame > 0) && (video_last_frame <= m_video_first_frame)) {
      ERROR("video last frame is smaller than video first frame");
      ret = false;
    } else {
      m_video_last_frame = video_last_frame;
    }
  } else {
    ERROR("video last frame should not be zero");
    ret = false;
  }
  return ret;
}

bool WriteInfo::set_audio_first_frame(uint32_t audio_first_frame)
{
  bool ret = true;
  if(audio_first_frame > 0) {
    if((m_audio_last_frame > 0) && (audio_first_frame >= m_audio_last_frame)) {
      ERROR("audio first frame is bigger than audio last frame");
      ret = false;
    } else {
      m_audio_first_frame = audio_first_frame;
    }
  } else {
    ERROR("audio first frame should not be zero.");
    ret = false;
  }
  return ret;
}

bool WriteInfo::set_audio_last_frame(uint32_t audio_last_frame)
{
  bool ret = true;
  if(audio_last_frame > 0) {
    if((m_audio_first_frame > 0) && (audio_last_frame <= m_audio_first_frame)) {
      ERROR("audio last frame is smaller than audio first frame");
      ret = false;
    } else {
      m_audio_last_frame = audio_last_frame;
    }
  } else {
    ERROR("audio last frame should not be zero");
    ret = false;
  }
  return ret;
}

uint64_t WriteInfo::get_video_duration()
{
  return m_video_duration;
}

uint64_t WriteInfo::get_audio_duration()
{
  return m_audio_duration;
}

uint32_t WriteInfo::get_video_first_frame()
{
  return m_video_first_frame;
}

uint32_t WriteInfo::get_video_last_frame()
{
  return m_video_last_frame;
}

uint32_t WriteInfo::get_audio_first_frame()
{
  return m_audio_first_frame;
}

uint32_t WriteInfo::get_audio_last_frame()
{
  return m_audio_last_frame;
}

BaseBox::BaseBox() :
    m_size(0),
    m_type(DISOM_BOX_TAG_NULL)
{
}

bool BaseBox::write_base_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_size))) {
      ERROR("Write base box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t*)&m_type,
                                             sizeof(m_type)))) {
      ERROR("Write base box error.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool BaseBox::read_base_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!file_reader->read_le_u32(m_size))) {
      ERROR("Failed to read size of base box.");
      ret = false;
      break;
    }
    INFO("read base box size success, size is %u", m_size);
    uint8_t type[4] = { 0 };
    if(AM_UNLIKELY(file_reader->read_data(type, 4) != 4)) {
      ERROR("Failed to read type of base box.");
      ret = false;
      break;
    }
    m_type = DISOM_BOX_TAG(DISO_BOX_TAG_CREATE(type[0], type[1],
                                               type[2], type[3]));
  } while(0);
  return ret;
}

uint32_t BaseBox::get_base_box_length()
{
  return ((uint32_t)(sizeof(uint32_t) + DISOM_TAG_SIZE));
}

FullBox::FullBox():
    m_flags(0),
    m_version(0)
{
}

bool FullBox::write_full_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write base box in the full box");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_writer->write_be_u32(((uint32_t) m_version << 24)
                                | ((uint32_t)m_flags)))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool FullBox::read_full_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box of full box.");
      ret = false;
      break;
    }
    uint32_t value = 0;
    if(AM_UNLIKELY(!file_reader->read_le_u32(value))) {
      ERROR("Failed to read full box.");
      ret = false;
      break;
    }
    m_version = (uint8_t)((value >> 24) & 0xff);
    m_flags = value & 0x00ffffff;
    INFO("read version and flag of full box success, version si %u, flag is %u",
            m_version, m_flags);
  } while(0);
  return ret;
}

uint32_t FullBox::get_full_box_length()
{
  //m_flags is 24 bits.
  return ((uint32_t)(m_base_box.get_base_box_length() + sizeof(uint32_t)));
}

bool FreeBox::write_free_box(Mp4FileWriter* file_writer)
{
  return m_base_box.write_base_box(file_writer);
}

bool FreeBox::read_free_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box of free box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

FileTypeBox::FileTypeBox():
    m_compatible_brands_number(0),
    m_minor_version(0),
    m_major_brand(DISOM_BRAND_NULL)
{
  for(int i = 0; i< DISOM_MAX_COMPATIBLE_BRANDS; i++) {
    m_compatible_brands[i] = DISOM_BRAND_NULL;
  }
}

bool FileTypeBox::write_file_type_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write file type box.");
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write base box in file type box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t*)&m_major_brand,
                                            DISOM_TAG_SIZE))) {
      ERROR("Failed to write major brand in file type box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_minor_version))) {
      ERROR("Failed to write minor version in file type box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_compatible_brands_number; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_data((uint8_t*)&m_compatible_brands[i],
                                              DISOM_TAG_SIZE))) {
        ERROR("Failed to write file type box");
        ret = false;
        break;
      }
    }
    INFO("write file type box success.");
  } while (0);
  return ret;
}

bool FileTypeBox::read_file_type_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read base box of file type box");
    if(AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read file type box.");
      ret = false;
      break;
    }
    INFO("File type box size is %u", m_base_box.m_size);
    uint8_t major_brand[4];
    INFO("begin to read major brand of file type box");
    if(AM_UNLIKELY(file_reader->read_data(major_brand, 4) != 4)) {
      ERROR("Failed to read major brand when read file type box.");
      ret = false;
      break;
    }
    m_major_brand = DISO_BRAND_TAG(DISO_BOX_TAG_CREATE(major_brand[0],
                          major_brand[1], major_brand[2], major_brand[3]));
    if(AM_UNLIKELY(!file_reader->read_le_u32(m_minor_version))) {
      ERROR("Failed to read minor version when read file type box.");
      ret = false;
      break;
    }
    INFO("Read minor version of file type box success, minor version is %u",
         m_minor_version);
    /*read file type box compatible brands*/
    uint32_t compatible_brands_size = m_base_box.m_size -
        sizeof(m_base_box.m_size) - sizeof(m_base_box.m_type) -
        sizeof(major_brand) - sizeof(m_minor_version);
    INFO("compatible brands size is %u", compatible_brands_size);
    if(AM_UNLIKELY((compatible_brands_size < 0) ||
                   (compatible_brands_size % 4 != 0))) {
      ERROR("compatible brands size is not correct, failed to read file type box.");
      ret = false;
      break;
    }
    m_compatible_brands_number = compatible_brands_size / 4;
    INFO("begin to read compatible brands of file type box, number is %u.",
         m_compatible_brands_number);
    for(uint32_t i = 0; i < m_compatible_brands_number; ++ i) {
      uint8_t compatible_brands[4] = { 0 };
      if(AM_UNLIKELY(file_reader->read_data(compatible_brands, 4) != 4)) {
        ERROR("Failed to read compatible brands of file type box");
        ret = false;
        break;
      }
      m_compatible_brands[i] =
          DISO_BRAND_TAG(DISO_BOX_TAG_CREATE(compatible_brands[0], compatible_brands[1],
                              compatible_brands[2], compatible_brands[3]));
      INFO("Set compatible brands success.");
    }
    if(AM_UNLIKELY(!ret)) {
      ERROR("Failed to read file type box.");
      break;
    }
    INFO("Read file type box success.");
  } while(0);
  return ret;
}

bool MediaDataBox::write_media_data_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  INFO("begin to write media data box.");
  if(AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
    ERROR("Failed to write media data box");
    ret = false;
  }
  if(ret) {
    INFO("write media data box success.");
  }
  return ret;
}
 bool MediaDataBox::read_media_data_box(Mp4FileReader *file_reader)
 {
   bool ret = true;
   do {
     INFO("begin to read media data box");
     if(AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
       ERROR("Failed to read base box when read media data box");
       ret = false;
       break;
     }
     INFO("read media data box success.");
   } while(0);
   return ret;
 }

MovieHeaderBox::MovieHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_timescale(0),
    m_rate(0),
    m_reserved_1(0),
    m_reserved_2(0),
    m_next_track_ID(0),
    m_volume(0),
    m_reserved(0)
{
  memset(m_matrix, 0, sizeof(m_matrix));
  memset(m_pre_defined, 0, sizeof(m_pre_defined));
}

bool MovieHeaderBox::write_movie_header_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("write movie header box, size is %u", m_base_full_box.m_base_box.m_size);
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write base full box in movie header box");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time))
            || (!file_writer->write_be_u32((uint32_t )m_modification_time))
            || (!file_writer->write_be_u32(m_timescale))
            || (!file_writer->write_be_u32((uint32_t )m_duration)))) {
        ERROR("Failed to write movie header box");
        ret = false;
        break;
      }
    } else {
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                      || (!file_writer->write_be_u64(m_modification_time))
                      || (!file_writer->write_be_u32(m_timescale))
                      || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write movie header box");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_rate))
                    || (!file_writer->write_be_u16(m_volume))
                    || (!file_writer->write_be_u16(m_reserved))
                    || (!file_writer->write_be_u32(m_reserved_1))
                    || (!file_writer->write_be_u32(m_reserved_2)))) {
      ERROR("Failed to write movie header box");
      ret = false;
      break;
    }

    for (uint32_t i = 0; i < 9; i ++) {
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_matrix[i]))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    for (uint32_t i = 0; i < 6; i ++) {
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_pre_defined[i]))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    if(AM_UNLIKELY(!file_writer->write_be_u32(m_next_track_ID))) {
      ERROR("Failed to write movie header box");
      ret = false;
      break;
    }
    INFO("write movie header box success.");
  } while (0);
  return ret;
}

bool MovieHeaderBox::read_movie_header_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("Begin to read movie header box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read base full box when read movie header box");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      uint32_t creation_time = 0;
      uint32_t modification_time = 0;
      uint32_t duration = 0;
      if (AM_UNLIKELY((!file_reader->read_le_u32(creation_time))
            || (!file_reader->read_le_u32(modification_time))
            || (!file_reader->read_le_u32(m_timescale))
            || (!file_reader->read_le_u32(duration)))) {
        ERROR("Failed to read movie header box");
        ret = false;
        break;
      }
      m_creation_time = creation_time;
      m_modification_time = modification_time;
      m_duration = duration;
    } else {
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                      || (!file_reader->read_le_u64(m_modification_time))
                      || (!file_reader->read_le_u32(m_timescale))
                      || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read movie header box");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_rate))
                    || (!file_reader->read_le_u16(m_volume))
                    || (!file_reader->read_le_u16(m_reserved))
                    || (!file_reader->read_le_u32(m_reserved_1))
                    || (!file_reader->read_le_u32(m_reserved_2)))) {
      ERROR("Failed to read movie header box");
      ret = false;
      break;
    }

    for (uint32_t i = 0; i < 9; i ++) {
      if(AM_UNLIKELY(!file_reader->read_le_u32(m_matrix[i]))) {
        ERROR("Failed to read movie header box");
        ret = false;
        return ret;
      }
    }

    for (uint32_t i = 0; i < 6; i ++) {
      if(AM_UNLIKELY(!file_reader->read_le_u32(m_pre_defined[i]))) {
        ERROR("Failed to read movie header box");
        ret = false;
        return ret;
      }
    }

    if(AM_UNLIKELY(!file_reader->read_le_u32(m_next_track_ID))) {
      ERROR("Failed to read movie header box");
      ret = false;
      break;
    }
    INFO("read movie header box success.");
  } while (0);
  return ret;
}

bool MovieHeaderBox::get_proper_movie_header_box(
    MovieHeaderBox& movie_header_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&movie_header_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    movie_header_box.m_creation_time = m_creation_time;
    movie_header_box.m_modification_time = m_modification_time;
    movie_header_box.m_duration = write_info.get_video_duration();
    movie_header_box.m_timescale = m_timescale;
    movie_header_box.m_rate = m_rate;
    movie_header_box.m_reserved_1 = m_reserved_1;
    movie_header_box.m_reserved_2 = m_reserved_2;
    memcpy((uint8_t*)movie_header_box.m_matrix,
           (uint8_t*)m_matrix, 9 * sizeof(uint32_t));
    memcpy((uint8_t*)movie_header_box.m_pre_defined,
           (uint8_t*)m_pre_defined, 6 * sizeof(uint32_t));
    movie_header_box.m_next_track_ID = m_next_track_ID;
    movie_header_box.m_volume = m_volume;
    movie_header_box.m_reserved = m_reserved;
  } while(0);
  return ret;
}

TrackHeaderBox::TrackHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_track_ID(0),
    m_reserved(0),
    m_width(0),
    m_height(0),
    m_layer(0),
    m_alternate_group(0),
    m_volume(0),
    m_reserved_2(0)
{
  memset(m_reserved_1, 0, sizeof(m_reserved_1));
  memset(m_matrix, 0, sizeof(m_matrix));
}

bool TrackHeaderBox::write_movie_track_header_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write movie track header box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time))
              || (!file_writer->write_be_u32((uint32_t )m_modification_time))
              || (!file_writer->write_be_u32(m_track_ID))
              || (!file_writer->write_be_u32(m_reserved))
              || (!file_writer->write_be_u32((uint32_t )m_duration)))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    } else {
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                      || (!file_writer->write_be_u64(m_modification_time))
                      || (!file_writer->write_be_u32(m_track_ID))
                      || (!file_writer->write_be_u32(m_reserved))
                      || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved_1[i]))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_layer))
                    || (!file_writer->write_be_u16(m_alternate_group))
                    || (!file_writer->write_be_u16(m_volume))
                    || (!file_writer->write_be_u16(m_reserved_2)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 9; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_matrix[i]))) {
        ERROR("Failed to write movie track header box");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_width))
                    || (!file_writer->write_be_u32(m_height)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    INFO("write movie track header box success.");
  } while (0);
  return ret;
}

bool TrackHeaderBox::read_movie_track_header_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read movie track header box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      uint32_t creation_time = 0;
      uint32_t modification_time = 0;
      uint32_t duration = 0;
      if (AM_UNLIKELY((!file_reader->read_le_u32(creation_time))
                      || (!file_reader->read_le_u32(modification_time))
                      || (!file_reader->read_le_u32(m_track_ID))
                      || (!file_reader->read_le_u32(m_reserved))
                      || (!file_reader->read_le_u32(duration)))) {
        ERROR("Failed to read movie track header box.");
        ret = false;
        break;
      }
      m_creation_time = creation_time;
      m_modification_time = modification_time;
      m_duration = duration;
    } else {
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                      || (!file_reader->read_le_u64(m_modification_time))
                      || (!file_reader->read_le_u32(m_track_ID))
                      || (!file_reader->read_le_u32(m_reserved))
                      || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read movie track header box.");
        ret = false;
        break;
      }
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved_1[i]))) {
        ERROR("Failed to read movie track header box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_layer))
                    || (!file_reader->read_le_u16(m_alternate_group))
                    || (!file_reader->read_le_u16(m_volume))
                    || (!file_reader->read_le_u16(m_reserved_2)))) {
      ERROR("Failed to read movie track header box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 9; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_matrix[i]))) {
        ERROR("Failed to read movie track header box");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_width))
                    || (!file_reader->read_le_u32(m_height)))) {
      ERROR("Failed to read movie track header box");
      ret = false;
      break;
    }
    INFO("read movie track header box success.");
  } while (0);
  return ret;
}

bool TrackHeaderBox::get_proper_track_header_box(
    TrackHeaderBox& track_header_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&track_header_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    track_header_box.m_creation_time = m_creation_time;
    track_header_box.m_modification_time = m_modification_time;
    track_header_box.m_duration = write_info.get_video_duration();
    track_header_box.m_track_ID = m_track_ID;
    track_header_box.m_reserved = m_reserved;
    memcpy((uint8_t*)track_header_box.m_reserved_1,
           (uint8_t*)m_reserved_1, 2 * sizeof(uint32_t));
    memcpy((uint8_t*)(track_header_box.m_matrix), (uint8_t*)(m_matrix),
           9 * sizeof(uint32_t));
    track_header_box.m_width = m_width;
    track_header_box.m_height = m_height;
    track_header_box.m_layer = m_layer;
    track_header_box.m_alternate_group = m_alternate_group;
    track_header_box.m_volume = m_volume;
    track_header_box.m_reserved_2 = m_reserved_2;
  } while(0);
  return ret;
}

MediaHeaderBox::MediaHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_timescale(0),
    m_pre_defined(0),
    m_pad(0)
{
  memset(m_language, 0, sizeof(m_language));
}

bool MediaHeaderBox::write_media_header_box(
                                      Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write media header box");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write media header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      uint32_t creation_time = m_creation_time;
      uint32_t modification_time = m_modification_time;
      uint32_t duration = m_duration;
      if (AM_UNLIKELY((!file_writer->write_be_u32(creation_time))
             || (!file_writer->write_be_u32(modification_time))
             || (!file_writer->write_be_u32(m_timescale))
             || (!file_writer->write_be_u32(duration)))) {
        ERROR("Failed to write media header box.");
        ret = true;
        break;
      }
    } else {
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                   || (!file_writer->write_be_u64(m_modification_time))
                   || (!file_writer->write_be_u32(m_timescale))
                   || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write media header box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(((uint32_t )m_pad << 31)
                                          | ((uint32_t )m_language[0] << 26)
                                          | ((uint32_t )m_language[1] << 21)
                                          | ((uint32_t )m_language[2] << 16)
                                          | ((uint32_t )m_pre_defined)))) {
      ERROR("Failed to write media header box");
      ret = false;
      break;
    }
    INFO("write media header box success.");
  } while (0);
  return ret;
}

bool MediaHeaderBox::read_media_header_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read media header box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read media header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      uint32_t creation_time = 0;
      uint32_t modification_time = 0;
      uint32_t duration = 0;
      if (AM_UNLIKELY((!file_reader->read_le_u32(creation_time))
            || (!file_reader->read_le_u32(modification_time))
            || (!file_reader->read_le_u32(m_timescale))
            || (!file_reader->read_le_u32(duration)))) {
        ERROR("Failed to read media header box.");
        ret = true;
        break;
      }
      m_creation_time = creation_time;
      m_modification_time = modification_time;
      m_duration = duration;
    } else {
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                      || (!file_reader->read_le_u64(m_modification_time))
                      || (!file_reader->read_le_u32(m_timescale))
                      || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read media header box.");
        ret = false;
        break;
      }
    }
    uint32_t value = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u32(value))) {
      ERROR("Failed to read media header box");
      ret = false;
      break;
    }
    m_pad = (value >> 31) & 0x01;
    m_language[0] = (value & 0x7C000000) >> 26;
    m_language[1] = (value & 0x03E00000) >> 21;
    m_language[2] = (value & 0x001F0000) >> 16;
    m_pre_defined = (value & 0x0000FFFF);
    INFO("read media header box success.");
  } while (0);
  return ret;
}

bool MediaHeaderBox::get_proper_media_header_box(
    MediaHeaderBox& media_header_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&media_header_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    media_header_box.m_creation_time = m_creation_time;
    media_header_box.m_modification_time = m_modification_time;
    media_header_box.m_duration = write_info.get_video_duration();
    media_header_box.m_timescale = m_timescale;
    media_header_box.m_pre_defined = m_pre_defined;
    media_header_box.m_pad = m_pad;
    memcpy(media_header_box.m_language, m_language, 3);
  } while(0);
  return ret;
}

HandlerReferenceBox::HandlerReferenceBox():
    m_handler_type(DISOM_BOX_TAG_NULL),
    m_pre_defined(0),
    m_name(NULL)
{
  memset(m_reserved, 0, sizeof(m_reserved));
}

HandlerReferenceBox::~HandlerReferenceBox()
{
  delete[] m_name;
}

bool HandlerReferenceBox::write_handler_reference_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write handler reference box.");
    if (AM_UNLIKELY((!m_base_full_box.write_full_box(file_writer))
                  || (!file_writer->write_be_u32(m_pre_defined))
                  || (!file_writer->write_data((uint8_t*)&m_handler_type,
                        DISOM_TAG_SIZE)))) {
      ERROR("Failed to write hander reference box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved[i]))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (m_name) {
      if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )(m_name),
            (uint32_t )strlen(m_name)))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(0))) {
      ERROR("Failed to write handler reference box.");
      ret = false;
      break;
    }
    INFO("write handler reference box success.");
  } while (0);
  return ret;
}

bool HandlerReferenceBox::read_handler_reference_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read handler reference box.");
    uint32_t handler_type = 0;
    if (AM_UNLIKELY((!m_base_full_box.read_full_box(file_reader))
                  || (!file_reader->read_le_u32(m_pre_defined))
                  || (!file_reader->read_box_tag(handler_type)))) {
      ERROR("Failed to read hander reference box");
      ret = false;
      break;
    }
    INFO("pre_defined of handler reference box is %u", m_pre_defined);
    m_handler_type = DISOM_BOX_TAG(handler_type);
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved[i]))) {
        ERROR("Failed to read handler reference box.");
        ret = false;
        break;
      }
      INFO("m_reserved of handler reference box is %u", m_reserved[i]);
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    uint32_t name_size = m_base_full_box.m_base_box.m_size -
        m_base_full_box.get_full_box_length() - sizeof(uint32_t) -
        sizeof(DISOM_BOX_TAG) - 3 * sizeof(uint32_t) - sizeof(uint8_t);
    if(name_size > 0) {
      INFO("name size of handler reference box is %u", name_size);
      m_name = new char[name_size + 1];
      if(AM_UNLIKELY(!m_name)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memset(m_name, 0, name_size + 1);
      if (AM_UNLIKELY(!file_reader->read_data((uint8_t* )(m_name),
                                              name_size))) {
        ERROR("Failed to read handler reference box.");
        ret = false;
        break;
      }
      INFO("name of handler reference box is %s", m_name);
      uint8_t unused_value = 0;
      if(AM_UNLIKELY(!file_reader->read_le_u8(unused_value))) {
        ERROR("Failed to read handler reference box.");
        ret = false;
        break;
      }
      INFO("unused value of handler reference box is %u", unused_value);
    }
    INFO("read handler reference box success.");
  } while (0);
  return ret;
}

bool HandlerReferenceBox::get_proper_handler_reference_box(
        HandlerReferenceBox& handler_reference_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&handler_reference_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    handler_reference_box.m_handler_type = m_handler_type;
    handler_reference_box.m_pre_defined = m_pre_defined;
    memcpy((uint8_t*)(handler_reference_box.m_reserved),
           (uint8_t*)m_reserved, 3 * sizeof(uint32_t));
    uint32_t name_size = m_base_full_box.m_base_box.m_size -
        m_base_full_box.get_full_box_length() - sizeof(m_pre_defined) -
        sizeof(m_handler_type) - 3 * sizeof(m_reserved[1]) -
        sizeof(uint8_t);
    if(name_size > 0) {
      handler_reference_box.m_name = new char[name_size];
      if(AM_UNLIKELY(!handler_reference_box.m_name)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(handler_reference_box.m_name, m_name, name_size);
    }
  } while(0);
  return ret;
}

VideoMediaInformationHeaderBox::VideoMediaInformationHeaderBox():
    m_graphicsmode(0)
{
  memset(m_opcolor, 0, sizeof(m_opcolor));
}

bool VideoMediaInformationHeaderBox::
    write_video_media_information_header_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write video media header box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write video media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_graphicsmode))
                || (!file_writer->write_be_u16(m_opcolor[0]))
                || (!file_writer->write_be_u16(m_opcolor[1]))
                || (!file_writer->write_be_u16(m_opcolor[2])))) {
      ERROR("Failed to write video media header box.");
      ret = false;
      break;
    }
    INFO("write video media header box success.");
  } while (0);
  return ret;
}

bool VideoMediaInformationHeaderBox::
     read_video_media_information_header_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read video media information header box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read video media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_graphicsmode))
                    || (!file_reader->read_le_u16(m_opcolor[0]))
                    || (!file_reader->read_le_u16(m_opcolor[1]))
                    || (!file_reader->read_le_u16(m_opcolor[2])))) {
      ERROR("Failed to read video media header box.");
      ret = false;
      break;
    }
    INFO("read video media information header box.");
  } while (0);
  return ret;
}

bool VideoMediaInformationHeaderBox::
  get_proper_video_media_information_header_box(
        VideoMediaInformationHeaderBox& video_media_information_header_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&video_media_information_header_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    video_media_information_header_box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*)m_opcolor, m_opcolor, 3 * sizeof(uint16_t));
  } while(0);
  return ret;
}

SoundMediaInformationHeaderBox::SoundMediaInformationHeaderBox():
    m_balanse(0),
    m_reserved(0)
{
}

bool SoundMediaInformationHeaderBox::
     write_sound_media_information_header_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to wrtie sound media information header box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_balanse))
                    || (!file_writer->write_be_u16(m_reserved)))) {
      ERROR("Failed to write sound media header box.");
      ret = false;
      break;
    }
    INFO("write sound media information header box success.");
  } while (0);
  return ret;
}

bool SoundMediaInformationHeaderBox::
     read_sound_media_information_header_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read sound media information header box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_balanse))
                    || (!file_reader->read_le_u16(m_reserved)))) {
      ERROR("Failed to read sound media header box.");
      ret = false;
      break;
    }
    INFO("read sound media infromation header box.");
  } while (0);
  return ret;
}

bool SoundMediaInformationHeaderBox::
   get_proper_sound_media_information_header_box(
        SoundMediaInformationHeaderBox& sound_media_information_header_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&sound_media_information_header_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    sound_media_information_header_box.m_balanse = m_balanse;
    sound_media_information_header_box.m_reserved = m_reserved;
  } while(0);
  return ret;
}

DecodingTimeToSampleBox::DecodingTimeToSampleBox():
    m_entry_buf_count(0),
    m_entry_count(0),
    m_p_entry(NULL)
{
}

DecodingTimeToSampleBox::~DecodingTimeToSampleBox()
{
  delete[] m_p_entry;
}

bool DecodingTimeToSampleBox::write_decoding_time_to_sample_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write decoding time to sample box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write decoding time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write decoding time to sample box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY((!file_writer->write_be_u32(m_p_entry[i].sample_count))
               || (!file_writer->write_be_u32(m_p_entry[i].sample_delta)))) {
        ERROR("Failed to write decoding time to sample box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    INFO("write decoding time to sample box  success.");
  } while (0);
  return ret;
}

bool DecodingTimeToSampleBox::read_decoding_time_to_sample_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read decoding time to sample box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read decoding time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read decoding time to sample box.");
      ret = false;
      break;
    }
    if(m_entry_count > 0) {
      m_p_entry = new _STimeEntry[m_entry_count];
      if(AM_UNLIKELY(!m_p_entry)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_p_entry[i].sample_count))
                        || (!file_reader->read_le_u32(m_p_entry[i].sample_delta)))) {
          ERROR("Failed to read decoding time to sample box.");
          ret = false;
          break;
        }
      }
      if(AM_UNLIKELY(!ret)) {
        break;
      }
    }
    INFO("read decoding time to sample box success.");
  } while (0);
  return ret;
}

void DecodingTimeToSampleBox::update_decoding_time_to_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        m_entry_count * sizeof(_STimeEntry);
}

bool DecodingTimeToSampleBox::get_proper_video_decoding_time_to_sample_box(
        DecodingTimeToSampleBox& decoding_time_to_sample_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(write_info.get_video_last_frame() > m_entry_count)) {
      ERROR("End frame is larger than frame number, error.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)(&decoding_time_to_sample_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    decoding_time_to_sample_box.m_entry_count =
        write_info.get_video_last_frame() -
        write_info.get_video_first_frame() + 1;
    decoding_time_to_sample_box.m_entry_buf_count =
        decoding_time_to_sample_box.m_entry_count;
    if(decoding_time_to_sample_box.m_entry_count > 0) {
      decoding_time_to_sample_box.m_p_entry =
          new _STimeEntry[decoding_time_to_sample_box.m_entry_count];
      if(AM_UNLIKELY(!decoding_time_to_sample_box.m_p_entry)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy((uint8_t*)(decoding_time_to_sample_box.m_p_entry),
             (uint8_t*)(&m_p_entry[write_info.get_video_first_frame() -1]),
             decoding_time_to_sample_box.m_entry_count * sizeof(_STimeEntry));
    }
  } while(0);
  return ret;
}

bool DecodingTimeToSampleBox::get_proper_audio_decoding_time_to_sample_box(
            DecodingTimeToSampleBox& decoding_time_to_sample_box,
            WriteInfo& write_info)
{
  bool ret = true;
  do {
    if(m_entry_count > 1) {
      ERROR("The entry count of audio decoding time to sample box is not 1");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(write_info.get_audio_last_frame() > m_p_entry[0].sample_count)) {
      ERROR("The audio last frame is bigger than sample count.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)(&decoding_time_to_sample_box.m_base_full_box),
               (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    decoding_time_to_sample_box.m_entry_count = m_entry_count;
    decoding_time_to_sample_box.m_p_entry = new _STimeEntry[1];
    if(AM_UNLIKELY(!decoding_time_to_sample_box.m_p_entry)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    decoding_time_to_sample_box.m_p_entry[0].sample_count =
        write_info.get_audio_last_frame() -
        write_info.get_audio_first_frame() + 1;
    decoding_time_to_sample_box.m_p_entry[0].sample_delta =
        m_p_entry[0].sample_delta;
  } while(0);
  return ret;
}

CompositionTimeToSampleBox::CompositionTimeToSampleBox():
    m_entry_count(0),
    m_entry_buf_count(0),
    m_p_entry(NULL)
{
}

CompositionTimeToSampleBox::~CompositionTimeToSampleBox()
{
  delete[] m_p_entry;
}

bool CompositionTimeToSampleBox::write_composition_time_to_sample_box(
                    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do{
    INFO("begin to write composition time to sample box.");
    if(AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    for(uint32_t i = 0; i< m_entry_count; ++ i) {
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_p_entry[i].sample_count))) {
        ERROR("Failed to write sample count of composition time to sample box");
        ret = false;
        break;
      }
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_p_entry[i].sample_delta))) {
        ERROR("Failed to write sample delta fo composition time to sample box.");
        ret = false;
        break;
      }
    }
    INFO("write composition time to sample box success.");
  }while(0);
  return ret;
}

bool CompositionTimeToSampleBox::read_composition_time_to_sample_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do{
    INFO("begin to read compositon time to sample box.");
    if(AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read composition time to sample box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read composition time to sample box.");
      ret = false;
      break;
    }
    if(m_entry_count > 0) {
      m_p_entry = new _STimeEntry[m_entry_count];
      if(AM_UNLIKELY(!m_p_entry)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      NOTICE("composition time to sample box entry count is %u", m_entry_count);
      for(uint32_t i = 0; i< m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_p_entry[i].sample_count))
                        || (!file_reader->read_le_u32(m_p_entry[i].sample_delta)))) {
          ERROR("Failed to read composition time to sample box.");
          ret = false;
          break;
        }
      }
    }
    INFO("read composition time to sample box.");
  }while(0);
  return ret;
}

void CompositionTimeToSampleBox::
                        update_composition_time_to_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        m_entry_count * sizeof(_STimeEntry);
}

bool CompositionTimeToSampleBox::get_proper_composition_time_to_sample_box(
        CompositionTimeToSampleBox& composition_time_to_sample_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(write_info.get_video_last_frame() > m_entry_count)) {
      ERROR("end frame is larger than frame number, error.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)(&composition_time_to_sample_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    composition_time_to_sample_box.m_entry_count =
        write_info.get_video_last_frame() -
        write_info.get_video_first_frame() + 1;
    composition_time_to_sample_box.m_entry_buf_count =
        composition_time_to_sample_box.m_entry_count;
    if(composition_time_to_sample_box.m_entry_count > 0) {
      composition_time_to_sample_box.m_p_entry =
          new _STimeEntry[composition_time_to_sample_box.m_entry_count];
      if(AM_UNLIKELY(!composition_time_to_sample_box.m_p_entry)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy((uint8_t*)(composition_time_to_sample_box.m_p_entry),
             (uint8_t*)(&m_p_entry[write_info.get_video_first_frame() -1]),
             composition_time_to_sample_box.m_entry_count * sizeof(_STimeEntry));
    }
  } while(0);
  return ret;
}

AVCDecoderConfigurationRecord::AVCDecoderConfigurationRecord():
    m_p_pps(NULL),
    m_p_sps(NULL),
    m_picture_parameters_set_length(0),
    m_sequence_parameters_set_length(0),
    m_configuration_version(0),
    m_AVC_profile_indication(0),
    m_profile_compatibility(0),
    m_AVC_level_indication(0),
    m_reserved(0),
    m_length_size_minus_one(0),
    m_reserved_1(0),
    m_num_of_sequence_parameter_sets(0),
    m_num_of_picture_parameter_sets(0)
{
}

AVCDecoderConfigurationRecord::~AVCDecoderConfigurationRecord()
{
  delete[] m_p_pps;
  delete[] m_p_sps;
}

bool AVCDecoderConfigurationRecord::
         write_avc_decoder_configuration_record_box(
             Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write avc decoder configuration record box.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                    || (!file_writer->write_be_u8(m_configuration_version)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((!file_writer->write_be_u8(m_AVC_profile_indication))
                   || (!file_writer->write_be_u8(m_profile_compatibility))
                   || (!file_writer->write_be_u8(m_AVC_level_indication)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((!file_writer->write_be_u16(((uint16_t )m_reserved << 10)
                | ((uint16_t )m_length_size_minus_one << 8)
                | ((uint16_t )m_reserved_1 << 5)
                | ((uint16_t )m_num_of_sequence_parameter_sets)))
           || (!file_writer->write_be_u16(m_sequence_parameters_set_length))
           || (!file_writer->write_data(m_p_sps, m_sequence_parameters_set_length))
           || (!file_writer->write_data(&m_num_of_picture_parameter_sets, 1))
           || (!file_writer->write_be_u16(m_picture_parameters_set_length))
           || (!file_writer->write_data(m_p_pps,
                                        m_picture_parameters_set_length)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    INFO("write avc decoder configuration record box.");
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::
         read_avc_decoder_configuration_record_box(
             Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read avc decoder configuration record box.");
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                    || (!file_reader->read_le_u8(m_configuration_version)))) {
      ERROR("Failed to read avc decoder configuration record box.");
      ret = false;
      break;
    }
    INFO("Read base box of avc decoder configuration record box success, "
        "size is %u, configuration version is %u", m_base_box.m_size,
        m_configuration_version);
    if(AM_UNLIKELY((!file_reader->read_le_u8(m_AVC_profile_indication))
                   || (!file_reader->read_le_u8(m_profile_compatibility))
                   || (!file_reader->read_le_u8(m_AVC_level_indication)))) {
      ERROR("Failed to read avc decoder configuration record box.");
      ret = false;
      break;
    }
    INFO("avc_profile_indication is %u, profile_compatibility is %u,"
        "avc_level_indication is %u", m_AVC_profile_indication,
        m_profile_compatibility, m_AVC_level_indication);
    uint8_t value = 0;
    if(AM_UNLIKELY(!file_reader->read_le_u8(value))) {
      ERROR("Failed to read data when read decoder configuration record box.");
      ret = false;
      break;
    }
    m_reserved = 0xff;
    m_length_size_minus_one = value & 0x03;
    INFO("length_size_minux_one is %u", m_length_size_minus_one);
    value = 0;
    if(AM_UNLIKELY(!file_reader->read_le_u8(value))) {
      ERROR("Failed to read data when read decoder configuration record box.");
      ret = false;
      break;
    }
    m_reserved_1 = 0xff;
    m_num_of_sequence_parameter_sets = value & 0x1f;
    INFO("num_of_sequence_parameter_sets is %u", m_num_of_sequence_parameter_sets);
    if(AM_UNLIKELY(!file_reader->read_le_u16(
        m_sequence_parameters_set_length))) {
      ERROR("Failed to read sequence parameter set length when read "
          "decoder configuration record box.");
      ret = false;
      break;
    }
    INFO("sequence_parameters_set_length is %u", m_sequence_parameters_set_length);
    if(m_sequence_parameters_set_length > 0) {
      delete[] m_p_sps;
      m_p_sps = new uint8_t[m_sequence_parameters_set_length];
      if(AM_UNLIKELY(!m_p_sps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if(AM_UNLIKELY(!file_reader->read_data(m_p_sps,
                                            m_sequence_parameters_set_length))) {
        ERROR("Failed to read sps when read decoder configuration record box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!file_reader->read_le_u8(m_num_of_picture_parameter_sets))) {
      ERROR("Failed to read num_of_picture_parameter_sets");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_reader->read_le_u16(
        m_picture_parameters_set_length))) {
      ERROR("Failed to read picture parameter set length");
      ret = false;
      break;
    }
    INFO("picture parameter set length is %u", m_picture_parameters_set_length);
    if(m_picture_parameters_set_length > 0) {
      delete[] m_p_pps;
      m_p_pps = new uint8_t[m_picture_parameters_set_length];
      if(AM_UNLIKELY(!m_p_pps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if(AM_UNLIKELY(!file_reader->read_data(m_p_pps,
                                        m_picture_parameters_set_length))) {
        ERROR("Failed to read data when read decoder configuration record box.");
        ret = false;
        break;
      }
    }
    INFO("read decoder configuration record box success.");
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::
   get_proper_avc_decoder_configuration_record_box(
       AVCDecoderConfigurationRecord& avc_decoder_configuration_record_box,
       WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&avc_decoder_configuration_record_box.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    if(m_picture_parameters_set_length > 0) {
      avc_decoder_configuration_record_box.m_picture_parameters_set_length =
          m_picture_parameters_set_length;
      avc_decoder_configuration_record_box.m_p_pps =
          new uint8_t[m_picture_parameters_set_length];
      if(AM_UNLIKELY(!avc_decoder_configuration_record_box.m_p_pps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(avc_decoder_configuration_record_box.m_p_pps, m_p_pps,
             m_picture_parameters_set_length);
      if(m_sequence_parameters_set_length > 0) {
        avc_decoder_configuration_record_box.m_sequence_parameters_set_length =
            m_sequence_parameters_set_length;
        avc_decoder_configuration_record_box.m_p_sps =
            new uint8_t[m_sequence_parameters_set_length];
        if(AM_UNLIKELY(!avc_decoder_configuration_record_box.m_p_sps)) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        memcpy(avc_decoder_configuration_record_box.m_p_sps, m_p_sps,
               m_sequence_parameters_set_length);
      }
      avc_decoder_configuration_record_box.m_configuration_version =
          m_configuration_version;
      avc_decoder_configuration_record_box.m_AVC_profile_indication =
          m_AVC_profile_indication;
      avc_decoder_configuration_record_box.m_profile_compatibility =
          m_profile_compatibility;
      avc_decoder_configuration_record_box.m_AVC_level_indication =
          m_AVC_level_indication;
      avc_decoder_configuration_record_box.m_reserved = m_reserved;
      avc_decoder_configuration_record_box.m_length_size_minus_one =
          m_length_size_minus_one;
      avc_decoder_configuration_record_box.m_reserved_1 =
          m_reserved_1;
      avc_decoder_configuration_record_box.m_num_of_sequence_parameter_sets =
          m_num_of_sequence_parameter_sets;
      avc_decoder_configuration_record_box.m_num_of_picture_parameter_sets =
          m_num_of_picture_parameter_sets;
    }
  } while(0);
  return ret;
}

_SVisualSampleEntry::_SVisualSampleEntry():
    m_horizresolution(0),
    m_vertresolution(0),
    m_reserved_1(0),
    m_data_reference_index(0),
    m_pre_defined(0),
    m_reserved(0),
    m_width(0),
    m_height(0),
    m_frame_count(0),
    m_depth(0),
    m_pre_defined_2(0)
{
  memset(m_pre_defined_1, 0, sizeof(m_pre_defined_1));
  memset(m_reserved_0, 0, sizeof(m_reserved_0));
  memset(m_compressorname, 0, sizeof(m_compressorname));
}

bool _SVisualSampleEntry::write_visual_sample_entry(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write visual sample entry.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                    || (!file_writer->write_data(m_reserved_0, 6))
                    || (!file_writer->write_be_u16(m_data_reference_index))
                    || (!file_writer->write_be_u16(m_pre_defined))
                    || (!file_writer->write_be_u16(m_reserved)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_pre_defined_1[i]))) {
        ERROR("Failed to write visual sample description box.");
        ret = false;
        return ret;
      }
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_width))
                 || (!file_writer->write_be_u16(m_height))
                 || (!file_writer->write_be_u32(m_horizresolution))
                 || (!file_writer->write_be_u32(m_vertresolution))
                 || (!file_writer->write_be_u32(m_reserved_1))
                 || (!file_writer->write_be_u16(m_frame_count))
                 || (!file_writer->write_data((uint8_t* )m_compressorname, 32))
                 || (!file_writer->write_be_u16(m_depth))
                 || (!file_writer->write_be_s16(m_pre_defined_2)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_avc_config.write_avc_decoder_configuration_record_box
                    (file_writer))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    INFO("write visual sample entry success.");
  } while (0);
  return ret;
}

bool _SVisualSampleEntry::read_visual_sample_entry(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read visual sample entry.");
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                    || (!file_reader->read_data(m_reserved_0, 6))
                    || (!file_reader->read_le_u16(m_data_reference_index))
                    || (!file_reader->read_le_u16(m_pre_defined))
                    || (!file_reader->read_le_u16(m_reserved)))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_pre_defined_1[i]))) {
        ERROR("Failed to read visual sample description box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_width))
                 || (!file_reader->read_le_u16(m_height))
                 || (!file_reader->read_le_u32(m_horizresolution))
                 || (!file_reader->read_le_u32(m_vertresolution))
                 || (!file_reader->read_le_u32(m_reserved_1))
                 || (!file_reader->read_le_u16(m_frame_count))
                 || (!file_reader->read_data((uint8_t* )m_compressorname, 32))
                 || (!file_reader->read_le_u16(m_depth))
                 || (!file_reader->read_le_s16(m_pre_defined_2)))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_avc_config.read_avc_decoder_configuration_record_box
                    (file_reader))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    INFO("read visual sample entry success.");
  } while (0);
  return ret;
}

bool _SVisualSampleEntry::get_proper_visual_sample_entry(
      _SVisualSampleEntry& visual_sample_entry, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&visual_sample_entry.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    m_avc_config.get_proper_avc_decoder_configuration_record_box(
        visual_sample_entry.m_avc_config, write_info);
    visual_sample_entry.m_horizresolution = m_horizresolution;
    visual_sample_entry.m_vertresolution = m_vertresolution;
    visual_sample_entry.m_reserved_1 = m_reserved_1;
    memcpy((uint8_t*)(visual_sample_entry.m_pre_defined_1),
           (uint8_t*)(m_pre_defined_1), 3 * sizeof(uint32_t));
    visual_sample_entry.m_data_reference_index = m_data_reference_index;
    visual_sample_entry.m_pre_defined = m_pre_defined;
    visual_sample_entry.m_reserved = m_reserved;
    visual_sample_entry.m_width = m_width;
    visual_sample_entry.m_height = m_height;
    visual_sample_entry.m_frame_count = m_frame_count;
    visual_sample_entry.m_depth = m_depth;
    visual_sample_entry.m_pre_defined_2 = m_pre_defined_2;
    memcpy(visual_sample_entry.m_reserved_0, m_reserved_0, 6);
    memcpy(visual_sample_entry.m_compressorname, m_compressorname, 32);
  } while(0);
  return ret;
}

AACElementaryStreamDescriptorBox::AACElementaryStreamDescriptorBox():
            m_es_id(0),
            m_es_descriptor_type_tag(0),
            m_es_descriptor_type_length(0),
            m_stream_priority(0),
            m_buffer_size(0),
            m_max_bitrate(0),
            m_avg_bitrate(0),
            m_decoder_config_descriptor_type_tag(0),
            m_decoder_descriptor_type_length(0),
            m_object_type(0),
            m_stream_flag(0),
            m_audio_spec_config(0),
            m_decoder_specific_descriptor_type_tag(0),
            m_decoder_specific_descriptor_type_length(0),
            m_SL_descriptor_type_tag(0),
            m_SL_descriptor_type_length(0),
            m_SL_value(0)
{
}

bool AACElementaryStreamDescriptorBox::
                          write_aac_elementary_stream_description_box(
        Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write aac elementary stream description box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
    /*ES descriptor takes 38 bytes*/
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_es_descriptor_type_tag))
                    || (!file_writer->write_be_u16(0x8080))
                    || (!file_writer->write_be_u8(m_es_descriptor_type_length))
                    || (!file_writer->write_be_u16(m_es_id))
                    || (!file_writer->write_be_u8(m_stream_priority)))) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = false;
      break;
    }
    /*decoder descriptor takes 26 bytes*/
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_decoder_config_descriptor_type_tag))
                   || (!file_writer->write_be_u16(0x8080))
                   || (!file_writer->write_be_u8(m_decoder_descriptor_type_length))
                   || (!file_writer->write_be_u8(m_object_type))
                   // stream type:6 upstream flag:1 reserved flag:1
                   || (!file_writer->write_be_u8(0x15))
                   || (!file_writer->write_be_u24(m_buffer_size))
                   || (!file_writer->write_be_u32(m_max_bitrate))
                   || (!file_writer->write_be_u32(m_avg_bitrate)))) {
      ERROR("Failed to write decoder descriptor.");
      ret = false;
      break;
    }
    /*decoder specific info descriptor takes 9 bytes*/
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_decoder_specific_descriptor_type_tag))
                  || (!file_writer->write_be_u16(0x8080))
                  || (!file_writer->write_be_u8(m_decoder_specific_descriptor_type_length))
                  || (!file_writer->write_be_u16(m_audio_spec_config))
                  || (!file_writer->write_be_u16(0))
                  || (!file_writer->write_be_u8(0)))) {
      ERROR("Failed to write decoder specific info descriptor");
      ret = false;
      break;
    }
    /*SL descriptor takes 5 bytes*/
    if(AM_UNLIKELY((!file_writer->write_be_u8(m_SL_descriptor_type_tag))
                 || (!file_writer->write_be_u16(0x8080))
                 || (!file_writer->write_be_u8(m_SL_descriptor_type_length))
                 || (!file_writer->write_be_u8(m_SL_value)))) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = false;
      break;
    }
    INFO("write aac elementary stream description box.");
  } while (0);
  return ret;
}

bool AACElementaryStreamDescriptorBox::
    read_aac_elementary_stream_description_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read aac elementary stream description box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read full box.");
      ret = false;
      break;
    }
    /*ES descriptor takes 38 bytes*/
    uint16_t temp16 = 0;
    uint8_t temp8 = 0;
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_es_descriptor_type_tag))
                    || (!file_reader->read_le_u16(temp16))
                    || (!file_reader->read_le_u8(m_es_descriptor_type_length))
                    || (!file_reader->read_le_u16(m_es_id))
                    || (!file_reader->read_le_u8(m_stream_priority)))) {
      ERROR("Failed to read aac elementary stream description box.");
      ret = false;
      break;
    }
    /*decoder descriptor takes 26 bytes*/
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_decoder_config_descriptor_type_tag))
                    || (!file_reader->read_le_u16(temp16))
                    || (!file_reader->read_le_u8(m_decoder_descriptor_type_length))
                    || (!file_reader->read_le_u8(m_object_type))
                    // stream type:6 upstream flag:1 reserved flag:1
                    || (!file_reader->read_le_u8(temp8))
                    || (!file_reader->read_le_u24(m_buffer_size))
                    || (!file_reader->read_le_u32(m_max_bitrate))
                    || (!file_reader->read_le_u32(m_avg_bitrate)))) {
      ERROR("Failed to read decoder descriptor.");
      ret = false;
      break;
    }
    /*decoder specific info descriptor takes 9 bytes*/
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_decoder_specific_descriptor_type_tag))
                    || (!file_reader->read_le_u16(temp16))
                    || (!file_reader->read_le_u8(m_decoder_specific_descriptor_type_length))
                    || (!file_reader->read_le_u16(m_audio_spec_config))
                    || (!file_reader->read_le_u16(temp16))
                    || (!file_reader->read_le_u8(temp8)))) {
      ERROR("Failed to read decoder specific info descriptor");
      ret = false;
      break;
    }
    /*SL descriptor takes 5 bytes*/
    if(AM_UNLIKELY((!file_reader->read_le_u8(m_SL_descriptor_type_tag))
                   || (!file_reader->read_le_u16(temp16))
                   || (!file_reader->read_le_u8(m_SL_descriptor_type_length))
                   || (!file_reader->read_le_u8(m_SL_value)))) {
      ERROR("Failed to read aac elementary stream description box.");
      ret = false;
      break;
    }
    INFO("read aac elementary stream description box.");
  } while (0);
  return ret;
}

bool AACElementaryStreamDescriptorBox::
    get_proper_aac_elementary_stream_descriptor_box(
        AACElementaryStreamDescriptorBox& aac_elementary_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&aac_elementary_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    aac_elementary_box.m_es_id = m_es_id;
    aac_elementary_box.m_es_descriptor_type_tag = m_es_descriptor_type_tag;
    aac_elementary_box.m_es_descriptor_type_length = m_es_descriptor_type_length;
    aac_elementary_box.m_stream_priority = m_stream_priority;
    aac_elementary_box.m_buffer_size = m_buffer_size;
    aac_elementary_box.m_max_bitrate = m_max_bitrate;
    aac_elementary_box.m_avg_bitrate = m_avg_bitrate;
    aac_elementary_box.m_decoder_config_descriptor_type_tag =
        m_decoder_config_descriptor_type_tag;
    aac_elementary_box.m_decoder_descriptor_type_length =
        m_decoder_descriptor_type_length;
    aac_elementary_box.m_object_type = m_object_type;
    aac_elementary_box.m_stream_flag = m_stream_flag;
    aac_elementary_box.m_audio_spec_config = m_audio_spec_config;
    aac_elementary_box.m_decoder_specific_descriptor_type_tag =
        m_decoder_specific_descriptor_type_tag;
    aac_elementary_box.m_decoder_specific_descriptor_type_length =
        m_decoder_specific_descriptor_type_length;
    aac_elementary_box.m_SL_descriptor_type_tag = m_SL_descriptor_type_tag;
    aac_elementary_box.m_SL_descriptor_type_length =
        m_SL_descriptor_type_length;
    aac_elementary_box.m_SL_value = m_SL_value;
  } while(0);
  return ret;
}

_SAudioSampleEntry::_SAudioSampleEntry():
          m_samplerate(0),
          m_data_reference_index(0),
          m_channelcount(0),
          m_samplesize(0),
          m_pre_defined(0),
          m_reserved_1(0)
{
  memset(m_reserved, 0, sizeof(m_reserved));
  memset(m_reserved_0, 0, sizeof(m_reserved_0));
}

bool _SAudioSampleEntry::write_audio_sample_entry(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write audio sample entry.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                    || (!file_writer->write_data(m_reserved_0, 6))
                    || (!file_writer->write_be_u16(m_data_reference_index)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved[i]))) {
        ERROR("Failed to write audio sample description box.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_channelcount))
                    || (!file_writer->write_be_u16(m_samplesize))
                    || (!file_writer->write_be_u16(m_pre_defined))
                    || (!file_writer->write_be_u16(m_reserved_1))
                    || (!file_writer->write_be_u32(m_samplerate)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_esd.write_aac_elementary_stream_description_box(
        file_writer))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    INFO("write audio sample entry success.");
  } while (0);
  return ret;
}

bool _SAudioSampleEntry::read_audio_sample_entry(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read audio sample entry.");
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                    || (!file_reader->read_data(m_reserved_0, 6))
                    || (!file_reader->read_le_u16(m_data_reference_index)))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved[i]))) {
        ERROR("Failed to read audio sample description box.");
        ret = false;
        break;;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_channelcount))
                    || (!file_reader->read_le_u16(m_samplesize))
                    || (!file_reader->read_le_u16(m_pre_defined))
                    || (!file_reader->read_le_u16(m_reserved_1))
                    || (!file_reader->read_le_u32(m_samplerate)))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_esd.read_aac_elementary_stream_description_box(
        file_reader))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    INFO("read audio sample entry success.");
  } while (0);
  return ret;
}

bool _SAudioSampleEntry::get_proper_audio_sample_entry(
    _SAudioSampleEntry& audio_sample_entry, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&audio_sample_entry.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    m_esd.get_proper_aac_elementary_stream_descriptor_box(
        audio_sample_entry.m_esd, write_info);
    memcpy((uint8_t*)(audio_sample_entry.m_reserved), (uint8_t*)m_reserved,
           sizeof(uint32_t) * 2);
    audio_sample_entry.m_samplerate = m_samplerate;
    audio_sample_entry.m_data_reference_index = m_data_reference_index;
    audio_sample_entry.m_channelcount = m_channelcount;
    audio_sample_entry.m_samplesize = m_samplesize;
    audio_sample_entry.m_pre_defined = m_pre_defined;
    audio_sample_entry.m_reserved_1 = m_reserved_1;
    memcpy(audio_sample_entry.m_reserved_0, m_reserved_0, 6);
  } while(0);
  return ret;
}

VisualSampleDescriptionBox::VisualSampleDescriptionBox():
    m_entry_count(0)
{
}

bool VisualSampleDescriptionBox::write_visual_sample_description_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write visual sample description box.");
    if (AM_UNLIKELY((!m_base_full_box.write_full_box(file_writer))
                    || (!file_writer->write_be_u32(m_entry_count))
                    || (!m_visual_entry.write_visual_sample_entry(
                        file_writer)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    INFO("write visual sample description box success.");
  } while (0);
  return ret;
}

bool VisualSampleDescriptionBox::read_visual_sample_description_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read visual sample description box.");
    if (AM_UNLIKELY((!m_base_full_box.read_full_box(file_reader))
                    || (!file_reader->read_le_u32(m_entry_count))
                    || (!m_visual_entry.read_visual_sample_entry(
                        file_reader)))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    INFO("read visual sample description box success.");
  } while (0);
  return ret;
}

bool VisualSampleDescriptionBox::
   get_proper_visual_sample_description_box(
        VisualSampleDescriptionBox& visual_sample_description_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&visual_sample_description_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    if(!m_visual_entry.get_proper_visual_sample_entry(
        visual_sample_description_box.m_visual_entry, write_info)) {
      ERROR("Failed to get proper visual sample entry.");
      ret = false;
      break;
    }
    visual_sample_description_box.m_entry_count = m_entry_count;
  } while(0);
  return ret;
}

AudioSampleDescriptionBox::AudioSampleDescriptionBox():
    m_entry_count(0)
{
}

bool AudioSampleDescriptionBox::write_audio_sample_description_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write audio sample description box.");
    if (AM_UNLIKELY((!m_base_full_box.write_full_box(file_writer))
                    || (!file_writer->write_be_u32(m_entry_count)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!m_audio_entry.write_audio_sample_entry(file_writer))) {
      ERROR("Failed to write audio sample entry.");
      ret = false;
      break;
    }
    INFO("write audio sample description box success.");
  } while (0);
  return ret;
}

bool AudioSampleDescriptionBox::read_audio_sample_description_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read audio sample description box.");
    if (AM_UNLIKELY((!m_base_full_box.read_full_box(file_reader))
                    || (!file_reader->read_le_u32(m_entry_count)))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!m_audio_entry.read_audio_sample_entry(file_reader))) {
      ERROR("Failed to read audio sample entry.");
      ret = false;
      break;
    }
    INFO("read audio sample description box success.");
  } while (0);
  return ret;
}

bool AudioSampleDescriptionBox::
   get_proper_audio_sample_description_box(
        AudioSampleDescriptionBox& audio_sample_description_box,
        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&audio_sample_description_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    m_audio_entry.get_proper_audio_sample_entry(
        audio_sample_description_box.m_audio_entry, write_info);
    audio_sample_description_box.m_entry_count = m_entry_count;
  } while(0);
  return ret;
}

SampleSizeBox::SampleSizeBox():
    m_sample_size(0),
    m_sample_count(0),
    m_entry_size(NULL),
    m_entry_buf_count(0)
{
}

SampleSizeBox::~SampleSizeBox()
{
  delete[] m_entry_size;
}

bool SampleSizeBox::write_sample_size_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write sample size box");
    if(AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((!file_writer->write_be_u32(m_sample_size))
                   || (!file_writer->write_be_u32(m_sample_count)))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_sample_count; i ++) {
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_entry_size[i]))) {
        ERROR("Failed to write sample size box");
        ret = false;
        break;
      }
    }
    if(ret != true) {
      break;
    }
    INFO("write sample size box success.");
  } while (0);
  return ret;
}

bool SampleSizeBox::read_sample_size_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read sample size box.");
    if(AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read sample size box");
      ret = false;
      break;
    }
    if(AM_UNLIKELY((!file_reader->read_le_u32(m_sample_size))
                   || (!file_reader->read_le_u32(m_sample_count)))) {
      ERROR("Failed to read sample size box");
      ret = false;
      break;
    }
    if(m_sample_count > 0) {
      m_entry_size = new uint32_t[m_sample_count];
      if(AM_UNLIKELY(!m_entry_size)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_sample_count; i ++) {
        if(AM_UNLIKELY(!file_reader->read_le_u32(m_entry_size[i]))) {
          ERROR("Failed to read sample size box");
          ret = false;
          break;
        }
      }
      if(ret != true) {
        break;
      }
    }
    INFO("read sample size box success.");
  } while (0);
  return ret;
}

void SampleSizeBox::update_sample_size_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        sizeof(uint32_t) + m_sample_count * sizeof(uint32_t);
}

bool SampleSizeBox::get_proper_video_sample_size_box(SampleSizeBox& sample_size_box,
                                  WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&sample_size_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    sample_size_box.m_sample_size = m_sample_size;
    if(write_info.get_video_last_frame() > m_sample_count) {
      ERROR("end frame is bigger than sample count.");
      ret = false;
      break;
    }
    uint32_t sample_count = write_info.get_video_last_frame() -
        write_info.get_video_first_frame() + 1;
    sample_size_box.m_entry_size = new uint32_t[sample_count];
    if(AM_UNLIKELY(!sample_size_box.m_entry_size)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)sample_size_box.m_entry_size,
           (uint8_t*)(&m_entry_size[write_info.get_video_first_frame() -1]),
           sample_count * sizeof(uint32_t));
    sample_size_box.m_sample_count = sample_count;
  } while(0);
  return ret;
}

bool SampleSizeBox::get_proper_audio_sample_size_box(SampleSizeBox& sample_size_box,
                                        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&sample_size_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    sample_size_box.m_sample_size = m_sample_size;
    if(write_info.get_audio_last_frame() > m_sample_count) {
      ERROR("end frame is bigger than sample count.");
      ret = false;
      break;
    }
    uint32_t sample_count = write_info.get_audio_last_frame() -
        write_info.get_audio_first_frame() + 1;
    sample_size_box.m_entry_size = new uint32_t[sample_count];
    if(AM_UNLIKELY(!sample_size_box.m_entry_size)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)sample_size_box.m_entry_size,
           (uint8_t*)(&m_entry_size[write_info.get_audio_first_frame() -1]),
           sample_count * sizeof(uint32_t));
    sample_size_box.m_sample_count = sample_count;
  } while(0);
  return ret;
}

_SSampleToChunkEntry::_SSampleToChunkEntry():
    m_first_entry(0),
    m_sample_per_chunk(0),
    m_sample_description_index(0)
{
}

bool _SSampleToChunkEntry::write_sample_chunk_entry(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do{
    INFO("begin to write sample chunk entry.");
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_first_entry))
                    || (!file_writer->write_be_u32(m_sample_per_chunk))
                    || (!file_writer->write_be_u32(
                        m_sample_description_index)))) {
      ERROR("Failed to write sample to chunk box.");
      ret = false;
      break;
    }
    INFO("write sample chunk entry success.");
  }while(0);
  return ret;
}

bool _SSampleToChunkEntry::read_sample_chunk_entry(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do{
    INFO("begin to read sample chunk entry.");
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_first_entry))
                    || (!file_reader->read_le_u32(m_sample_per_chunk))
                    || (!file_reader->read_le_u32(
                        m_sample_description_index)))) {
      ERROR("Failed to read sample to chunk box.");
      ret = false;
      break;
    }
    INFO("read sample chunk entry success.");
  }while(0);
  return ret;
}

SampleToChunkBox::SampleToChunkBox():
    m_entrys(NULL),
    m_entry_count(0)
{
}

SampleToChunkBox::~SampleToChunkBox()
{
  delete[] m_entrys;
}

bool SampleToChunkBox::write_sample_to_chunk_box(
                                         Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write sample to chunk box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write sample to chunk box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write sample to chunk box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(!m_entrys[i].write_sample_chunk_entry(
          file_writer))) {
        ERROR("Failed to write sample to chunk box.");
        ret = false;
        break;
      }
    }
    INFO("write sample to chunk box success.");
  } while (0);
  return ret;
}

bool SampleToChunkBox::read_sample_to_chunk_box(
                                         Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read sample to chunk box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read sample to chunk box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read sample to chunk box");
      ret = false;
      break;
    }
    if(AM_LIKELY(m_entry_count > 0)) {
      m_entrys = new _SSampleToChunkEntry[m_entry_count];
      if(AM_UNLIKELY(!m_entrys)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY(!m_entrys[i].read_sample_chunk_entry(
            file_reader))) {
          ERROR("Failed to read sample to chunk box.");
          ret = false;
          break;
        }
      }
    }
    INFO("read sample to chunk box success.");
  } while (0);
  return ret;
}

void SampleToChunkBox::update_sample_to_chunk_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
                              m_entry_count * sizeof(_SSampleToChunkEntry);
}

bool SampleToChunkBox::get_proper_sample_to_chunk_box(
    SampleToChunkBox& sample_to_chunk_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&sample_to_chunk_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    sample_to_chunk_box.m_entry_count = m_entry_count;
    sample_to_chunk_box.m_entrys = new _SSampleToChunkEntry[m_entry_count];
    if(AM_UNLIKELY(!sample_to_chunk_box.m_entrys)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    memcpy((uint8_t*)(sample_to_chunk_box.m_entrys),
           (uint8_t*)m_entrys, sizeof(_SSampleToChunkEntry) * m_entry_count);
  } while(0);
  return ret;
}

ChunkOffsetBox::ChunkOffsetBox() :
    m_entry_count(0),
    m_chunk_offset(NULL),
    m_entry_buf_count(0)
{
}

ChunkOffsetBox::~ChunkOffsetBox()
{
  delete[] m_chunk_offset;
}

bool ChunkOffsetBox::write_chunk_offset_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write chunk offset box.");
    if(AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write chunk offset box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write chunk offset box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; i ++) {
      if(AM_UNLIKELY(!file_writer->write_be_u32(m_chunk_offset[i]))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    INFO("write chunk offset box success.");
  } while (0);
  return ret;
}

bool ChunkOffsetBox::read_chunk_offset_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read chunk offset box.");
    if(AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read chunk offset box.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read chunk offset box");
      ret = false;
      break;
    }
    if(AM_LIKELY(m_entry_count > 0)) {
      m_chunk_offset = new uint32_t[m_entry_count];
      if(AM_UNLIKELY(!m_chunk_offset)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; i ++) {
        if(AM_UNLIKELY(!file_reader->read_le_u32(m_chunk_offset[i]))) {
          ERROR("Failed to read chunk offset box.");
          ret = false;
          break;
        }
      }
    }
    INFO("read chunk offset box success.");
  } while (0);
  return ret;
}

void ChunkOffsetBox::update_chunk_offset_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
      m_entry_count * sizeof(uint32_t);
}

bool ChunkOffsetBox::get_proper_video_chunk_offset_box(
    ChunkOffsetBox& chunk_offset_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&chunk_offset_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    uint32_t entry_count = write_info.get_video_last_frame()
        - write_info.get_video_first_frame() + 1;
     chunk_offset_box.m_entry_count = entry_count;
     chunk_offset_box.m_chunk_offset = new uint32_t[entry_count];
     if(AM_UNLIKELY(!chunk_offset_box.m_chunk_offset)) {
       ERROR("Failed to malloc memory.");
       ret = false;
       break;
     }
  } while(0);
  return ret;
}

bool ChunkOffsetBox::get_proper_audio_chunk_offset_box(
    ChunkOffsetBox& chunk_offset_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&chunk_offset_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    uint32_t entry_count = write_info.get_audio_last_frame()
            - write_info.get_audio_first_frame() + 1;
    chunk_offset_box.m_entry_count = entry_count;
    chunk_offset_box.m_chunk_offset = new uint32_t[entry_count];
    if(AM_UNLIKELY(!chunk_offset_box.m_chunk_offset)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

SyncSampleTableBox::SyncSampleTableBox():
    m_sync_sample_table(NULL),
    m_stss_count(0),
    m_stss_buf_count(0)
{
}

SyncSampleTableBox::~SyncSampleTableBox()
{
  delete[] m_sync_sample_table;
}

bool SyncSampleTableBox::
                 write_sync_sample_table_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write sync sample table box.");
    if (AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_stss_count))) {
      ERROR("Failed to write stss count.");
      ret = false;
      break;
    }
    for(uint32_t i = 0; i< m_stss_count; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_sync_sample_table[i]))) {
        ERROR("Failed to write stss to file.");
        ret = false;
        break;
      }
    }
    INFO("write sync sample table box success.");
  } while (0);
  return ret;
}

bool SyncSampleTableBox::
                 read_sync_sample_table_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read sync sample table box.");
    if (AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read full box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_stss_count))) {
      ERROR("Failed to read stss count.");
      ret = false;
      break;
    }
    if(AM_LIKELY(m_stss_count > 0)) {
      m_sync_sample_table = new uint32_t[m_stss_count];
      if(AM_UNLIKELY(!m_sync_sample_table)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      for(uint32_t i = 0; i< m_stss_count; ++ i) {
        if(AM_UNLIKELY(!file_reader->read_le_u32(m_sync_sample_table[i]))) {
          ERROR("Failed to read sync sample table box.");
          ret = false;
          break;
        }
      }
    }
    INFO("read sync sample table box success.");
  } while (0);
  return ret;
}

void SyncSampleTableBox::update_sync_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
      m_stss_count * sizeof(uint32_t);
}

bool SyncSampleTableBox::get_proper_sync_sample_table_box(
        SyncSampleTableBox& sync_sample_table_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&sync_sample_table_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    uint32_t i = 0;
    for(; i < m_stss_count; ++ i) {
      if(m_sync_sample_table[i] == write_info.get_video_first_frame()) {
        break;
      }
    }
    if(AM_UNLIKELY(i == m_stss_count)) {
      ERROR("The start frame is not the IDR frame");
      ret = false;
      break;
    }
    sync_sample_table_box.m_sync_sample_table = new uint32_t[m_stss_count];
    if(AM_UNLIKELY(!sync_sample_table_box.m_sync_sample_table)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for(uint32_t j = 0; i < m_stss_count; ++ i, ++ j) {
      if(write_info.get_video_last_frame() >= m_sync_sample_table[i]) {
        sync_sample_table_box.m_sync_sample_table[j] =
            m_sync_sample_table[i] - write_info.get_video_first_frame() + 1;
      } else {
        sync_sample_table_box.m_stss_count = j;
        break;
      }
    }
  } while(0);
  return ret;
}

bool DataEntryBox::write_data_entry_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_base_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write full box when write data entry box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool DataEntryBox::read_data_entry_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if(AM_UNLIKELY(!m_base_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read full box when read data entry box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool DataEntryBox::get_proper_data_entry_box(DataEntryBox& data_entry_box,
                                   WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&data_entry_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
  } while(0);
  return ret;
}

DataReferenceBox::DataReferenceBox():
    m_entry_count(0)
{}

bool DataReferenceBox::write_data_reference_box(
                                 Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to read data reference box.");
    if (AM_UNLIKELY((!m_base_full_box.write_full_box(file_writer))
                  || (!file_writer->write_be_u32(m_entry_count)))) {
      ERROR("Failed to write data reference box.");
      ret = true;
      break;
    }
    if (AM_UNLIKELY(!m_url.write_data_entry_box(file_writer))) {
      ERROR("Failed to write data reference box.");
      ret = false;
      break;
    }
    INFO("read reference box success.");
  } while (0);
  return ret;
}

bool DataReferenceBox::read_data_reference_box(
                                 Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read data reference box.");
    if (AM_UNLIKELY((!m_base_full_box.read_full_box(file_reader))
                  || (!file_reader->read_le_u32(m_entry_count)))) {
      ERROR("Failed to read data reference box.");
      ret = true;
      break;
    }
    if(m_entry_count != 1) {
      ERROR("entry count is not correct, m_entry_count is %u", m_entry_count);
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_url.read_data_entry_box(file_reader))) {
      ERROR("Failed to read data reference box.");
      ret = false;
      break;
    }
    INFO("read data reference box success.");
  } while (0);
  return ret;
}

bool DataReferenceBox::get_proper_data_reference_box(
    DataReferenceBox& data_reference_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&data_reference_box.m_base_full_box),
           (uint8_t*)(&m_base_full_box), m_base_full_box.get_full_box_length());
    data_reference_box.m_entry_count = m_entry_count;
    memcpy((uint8_t*)(&data_reference_box.m_url), (uint8_t*)(&m_url),
           sizeof(DataEntryBox));
  } while(0);
  return ret;
}

bool DataInformationBox::write_data_information_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write data information box.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                    || (!m_data_ref.write_data_reference_box(file_writer)))) {
      ERROR("Failed to write data infomation box.");
      ret = false;
      break;
    }
    INFO("write data information box success.");
  } while (0);
  return ret;
}

bool DataInformationBox::read_data_information_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read data information box.");
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                    || (!m_data_ref.read_data_reference_box(file_reader)))) {
      ERROR("Failed to read data infomation box.");
      ret = false;
      break;
    }
    INFO("read data information box success.");
  } while (0);
  return ret;
}

bool DataInformationBox::get_proper_data_information_box(
        DataInformationBox& data_information_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&data_information_box.m_base_box),
           (uint8_t*)(& m_base_box), sizeof(BaseBox));
    m_data_ref.get_proper_data_reference_box(
        data_information_box.m_data_ref, write_info);
  } while(0);
  return ret;
}

bool SampleTableBox::write_video_sample_table_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write video sample table box.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
             || (!m_visual_sample_description.\
                      write_visual_sample_description_box(file_writer))
             || (!m_stts.write_decoding_time_to_sample_box(file_writer))
             || (!m_ctts.write_composition_time_to_sample_box(file_writer))
             || (!m_sample_size.write_sample_size_box(file_writer))
             || (!m_sample_to_chunk.write_sample_to_chunk_box(file_writer))
             || (!m_chunk_offset.write_chunk_offset_box(file_writer)))) {
      ERROR("Failed to write video sample table box.");
      ret = false;
      break;
    }
    INFO("begin to write sync_sample table box");
    if(!m_sync_sample.write_sync_sample_table_box(file_writer)) {
      ERROR("Failed to write sync sample table box.");
      ret = false;
      break;
    }
    INFO("write video sample table box.");
  }while(0);
  return ret;
}

bool SampleTableBox::read_video_sample_table_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  INFO("begin to read video sample table box.");
  if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
           || (!m_visual_sample_description.\
                  read_visual_sample_description_box(file_reader))
           || (!m_stts.read_decoding_time_to_sample_box(file_reader))
           || (!m_ctts.read_composition_time_to_sample_box(file_reader))
           || (!m_sample_size.read_sample_size_box(file_reader))
           || (!m_sample_to_chunk.read_sample_to_chunk_box(file_reader))
           || (!m_chunk_offset.read_chunk_offset_box(file_reader))
           || (!m_sync_sample.read_sync_sample_table_box(file_reader)))) {
    ERROR("Failed to read video sample table box.");
    ret = false;
  }
  if(ret) {
    INFO("read video sample table box.");
  }
  return ret;
}

bool SampleTableBox::get_proper_video_sample_table_box(
        SampleTableBox& video_sample_table_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&video_sample_table_box.m_base_box),
           (uint8_t*)&m_base_box, sizeof(BaseBox));
    if (AM_UNLIKELY((!m_visual_sample_description.\
                        get_proper_visual_sample_description_box(
                            video_sample_table_box.m_visual_sample_description,
                            write_info))
                        || (!m_stts.get_proper_video_decoding_time_to_sample_box(
                            video_sample_table_box.m_stts, write_info))
                        || (!m_ctts.get_proper_composition_time_to_sample_box(
                            video_sample_table_box.m_ctts, write_info))
                        || (!m_sample_size.get_proper_video_sample_size_box(
                            video_sample_table_box.m_sample_size, write_info))
                        || (!m_sample_to_chunk.get_proper_sample_to_chunk_box(
                            video_sample_table_box.m_sample_to_chunk, write_info))
                        || (!m_chunk_offset.get_proper_video_chunk_offset_box(
                            video_sample_table_box.m_chunk_offset, write_info))
                        || (!m_sync_sample.get_proper_sync_sample_table_box(
                            video_sample_table_box.m_sync_sample, write_info)))) {
      ERROR("Failed to get proper video sample table box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool SampleTableBox::write_audio_sample_table_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
               || (!m_audio_sample_description.\
                    write_audio_sample_description_box(file_writer))
               || (!m_stts.write_decoding_time_to_sample_box(file_writer))
               || (!m_sample_size.write_sample_size_box(file_writer))
               || (!m_sample_to_chunk.write_sample_to_chunk_box(file_writer))
               || (!m_chunk_offset.write_chunk_offset_box(file_writer)))) {
      ERROR("Failed to write sound sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool SampleTableBox::read_audio_sample_table_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
               || (!m_audio_sample_description.\
                    read_audio_sample_description_box(file_reader))
               || (!m_stts.read_decoding_time_to_sample_box(file_reader))
               || (!m_sample_size.read_sample_size_box(file_reader))
               || (!m_sample_to_chunk.read_sample_to_chunk_box(file_reader))
               || (!m_chunk_offset.read_chunk_offset_box(file_reader)))) {
      ERROR("Failed to read sound sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool SampleTableBox::get_proper_audio_sample_table_box(
        SampleTableBox& audio_sample_table_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&audio_sample_table_box.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    if (AM_UNLIKELY((!m_audio_sample_description.get_proper_audio_sample_description_box(
        audio_sample_table_box.m_audio_sample_description, write_info))
          || (!m_stts.get_proper_audio_decoding_time_to_sample_box(
              audio_sample_table_box.m_stts, write_info))
          || (!m_sample_size.get_proper_audio_sample_size_box(
              audio_sample_table_box.m_sample_size, write_info))
          || (!m_sample_to_chunk.get_proper_sample_to_chunk_box(
              audio_sample_table_box.m_sample_to_chunk, write_info))
          || (!m_chunk_offset.get_proper_audio_chunk_offset_box(
              audio_sample_table_box.m_chunk_offset, write_info)))) {
      ERROR("Failed to get proper audio sample table box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void SampleTableBox::update_video_sample_table_box_size()
{
  m_stts.update_decoding_time_to_sample_box_size();
  m_ctts.update_composition_time_to_sample_box_size();
  m_sample_size.update_sample_size_box_size();
  m_sample_to_chunk.update_sample_to_chunk_box_size();
  m_chunk_offset.update_chunk_offset_box_size();
  m_sync_sample.update_sync_sample_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_stts.m_base_full_box.m_base_box.m_size + \
      m_ctts.m_base_full_box.m_base_box.m_size + \
      m_visual_sample_description.m_base_full_box.m_base_box.m_size + \
      m_sample_size.m_base_full_box.m_base_box.m_size + \
      m_sample_to_chunk.m_base_full_box.m_base_box.m_size + \
      m_chunk_offset.m_base_full_box.m_base_box.m_size + \
      m_sync_sample.m_base_full_box.m_base_box.m_size;
}

void SampleTableBox::update_audio_sample_table_box_size()
{
  m_stts.update_decoding_time_to_sample_box_size();
  m_sample_size.update_sample_size_box_size();
  m_sample_to_chunk.update_sample_to_chunk_box_size();
  m_chunk_offset.update_chunk_offset_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_stts.m_base_full_box.m_base_box.m_size + \
      m_audio_sample_description.m_base_full_box.m_base_box.m_size + \
      m_sample_size.m_base_full_box.m_base_box.m_size + \
      m_sample_to_chunk.m_base_full_box.m_base_box.m_size + \
      m_chunk_offset.m_base_full_box.m_base_box.m_size;
}

bool MediaInformationBox::write_video_media_information_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
             || (!m_video_information_header.\
                    write_video_media_information_header_box(file_writer))
             || (!m_data_info.write_data_information_box(file_writer))
             || (!m_sample_table.write_video_sample_table_box(file_writer)))) {
      ERROR("Failed to write video media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::read_video_media_information_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
             || (!m_video_information_header.\
                    read_video_media_information_header_box(file_reader))
             || (!m_data_info.read_data_information_box(file_reader))
             || (!m_sample_table.read_video_sample_table_box(file_reader)))) {
      ERROR("Failed to read video media information box.");
      ret = false;
      break;
    }
    NOTICE("the size of video media information box is %u", m_base_box.m_size);
  } while (0);
  return ret;
}

bool MediaInformationBox::get_proper_video_media_information_box(
      MediaInformationBox& media_information_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&media_information_box.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    if (AM_UNLIKELY((!m_video_information_header.\
        get_proper_video_media_information_header_box(
            media_information_box.m_video_information_header, write_info))
    || (!m_data_info.get_proper_data_information_box(
        media_information_box.m_data_info, write_info))
    || (!m_sample_table.get_proper_video_sample_table_box(
        media_information_box.m_sample_table, write_info)))) {
      ERROR("Failed to get proper video media information box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool MediaInformationBox::write_audio_media_information_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                 || (!m_sound_information_header.\
                    write_sound_media_information_header_box(file_writer))
                 || (!m_data_info.write_data_information_box(file_writer))
                 || (!m_sample_table.write_audio_sample_table_box(file_writer)))) {
      ERROR("Failed to write audio media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::read_audio_media_information_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!m_sound_information_header.\
                    read_sound_media_information_header_box(file_reader))
                 || (!m_data_info.read_data_information_box(file_reader))
                 || (!m_sample_table.read_audio_sample_table_box(
                     file_reader)))) {
      ERROR("Failed to read audio media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::get_proper_audio_media_information_box(
        MediaInformationBox& media_information_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&media_information_box.m_base_box),
           (uint8_t*)(&m_base_box), sizeof(BaseBox));
    if (AM_UNLIKELY((!m_sound_information_header.\
                        get_proper_sound_media_information_header_box(
                            media_information_box.m_sound_information_header,
                            write_info))
                 || (!m_data_info.get_proper_data_information_box(
                      media_information_box.m_data_info, write_info))
                 || (!m_sample_table.get_proper_audio_sample_table_box(
                      media_information_box.m_sample_table, write_info)))) {
      ERROR("Failed to get proper audio media information box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void MediaInformationBox::update_video_media_information_box_size()
{
  m_sample_table.update_video_sample_table_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + m_sample_table.m_base_box.m_size
      + m_video_information_header.m_base_full_box.m_base_box.m_size
      + m_data_info.m_base_box.m_size;
}

void MediaInformationBox::update_audio_media_information_box_size()
{
  m_sample_table.update_audio_sample_table_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + m_sample_table.m_base_box.m_size +
      m_sound_information_header.m_base_full_box.m_base_box.m_size +
      m_data_info.m_base_box.m_size;
}

bool MediaBox::write_video_media_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    INFO("begin to write video media box.");
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                 || (!m_media_header.write_media_header_box(file_writer))
                 || (!m_media_handler.write_handler_reference_box(file_writer))
                 || (!m_media_info.write_video_media_information_box(
                     file_writer)))) {
      ERROR("Failed to write video media box.");
      ret = false;
      break;
    }
    INFO("write video media box success.");
  } while (0);
  return ret;
}

bool MediaBox::read_video_media_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!m_media_header.read_media_header_box(file_reader))
                 || (!m_media_handler.read_handler_reference_box(file_reader))
                 || (!m_media_info.read_video_media_information_box(
                     file_reader)))) {
      ERROR("Failed to read video media box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaBox::get_proper_video_media_box(MediaBox& media_box,
                                  WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&media_box.m_base_box), (uint8_t*)&m_base_box,
           sizeof(BaseBox));
    if (AM_UNLIKELY((!m_media_header.get_proper_media_header_box(
        media_box.m_media_header, write_info))
            || (!m_media_handler.get_proper_handler_reference_box(
                media_box.m_media_handler, write_info))
            || (!m_media_info.get_proper_video_media_information_box(
                media_box.m_media_info, write_info)))) {
      ERROR("Failed to get proper video media box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool MediaBox::write_audio_media_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                 || (!m_media_header.write_media_header_box(file_writer))
                 || (!m_media_handler.write_handler_reference_box(file_writer))
                 || (!m_media_info.write_audio_media_information_box(
                     file_writer)))) {
      ERROR("Failed to write audio media box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaBox::read_audio_media_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!m_media_header.read_media_header_box(file_reader))
                 || (!m_media_handler.read_handler_reference_box(file_reader))
                 || (!m_media_info.read_audio_media_information_box(
                     file_reader)))) {
      ERROR("Failed to read audio media box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MediaBox::get_proper_audio_media_box(MediaBox& media_box,
                                  WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&media_box.m_base_box), (uint8_t*)&m_base_box,
           sizeof(BaseBox));
    if (AM_UNLIKELY((!m_media_header.get_proper_media_header_box(
        media_box.m_media_header, write_info))
             || (!m_media_handler.get_proper_handler_reference_box(
         media_box.m_media_handler, write_info))
             || (!m_media_info.get_proper_audio_media_information_box(
         media_box.m_media_info, write_info)))) {
      ERROR("Failed to get proper audio media box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void MediaBox::update_video_media_box_size()
{
  m_media_info.update_video_media_information_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_media_header.m_base_full_box.m_base_box.m_size + \
      m_media_handler.m_base_full_box.m_base_box.m_size + \
      m_media_info.m_base_box.m_size;
}

void MediaBox::update_audio_media_box_size()
{
  m_media_info.update_audio_media_information_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_media_header.m_base_full_box.m_base_box.m_size + \
      m_media_handler.m_base_full_box.m_base_box.m_size + \
      m_media_info.m_base_box.m_size;
}

bool TrackBox::write_video_movie_track_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                 || (!m_track_header.write_movie_track_header_box(file_writer))
                 || (!m_media.write_video_media_box(file_writer)))) {
      ERROR("Failed to write movie video track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool TrackBox::read_video_movie_track_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!m_track_header.read_movie_track_header_box(file_reader))
                 || (!m_media.read_video_media_box(file_reader)))) {
      ERROR("Failed to read movie video track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool TrackBox::get_proper_video_movie_track_box(TrackBox& track_box,
                                       WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&track_box.m_base_box), (uint8_t*)&m_base_box,
           sizeof(BaseBox));
    if (AM_UNLIKELY((!m_track_header.get_proper_track_header_box(
        track_box.m_track_header, write_info))
                    || (!m_media.get_proper_video_media_box(
                        track_box.m_media, write_info)))) {
      ERROR("Failed to get proper video movie track box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool TrackBox::write_audio_movie_track_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                  || (!m_track_header.write_movie_track_header_box(file_writer))
                  || (!m_media.write_audio_media_box(file_writer)))) {
      ERROR("Failed to write movie audio track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool TrackBox::read_audio_movie_track_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                  || (!m_track_header.read_movie_track_header_box(file_reader))
                  || (!m_media.read_audio_media_box(file_reader)))) {
      ERROR("Failed to read movie audio track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool TrackBox::get_proper_audio_movie_track_box(TrackBox& track_box,
                                        WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&track_box.m_base_box), (uint8_t*)&m_base_box,
           sizeof(BaseBox));
    if (AM_UNLIKELY((!m_track_header.get_proper_track_header_box(
        track_box.m_track_header, write_info))
          || (!m_media.get_proper_audio_media_box(
              track_box.m_media, write_info)))) {
      ERROR("Failed to get proper audio movie track box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

void TrackBox::update_video_movie_track_box_size()
{
  m_media.update_video_media_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_track_header.m_base_full_box.m_base_box.m_size + \
      m_media.m_base_box.m_size;
}

void TrackBox::update_audio_movie_track_box_size()
{
  m_media.update_audio_media_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE
      + m_track_header.m_base_full_box.m_base_box.m_size
      + m_media.m_base_box.m_size;
}

bool MovieBox::write_movie_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                  || (!m_movie_header_box.write_movie_header_box(
                      file_writer)))) {
      ERROR("Failed to write movie box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_video_track.write_video_movie_track_box(
        file_writer))) {
      ERROR("Failed to write video movie track box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_audio_track.write_audio_movie_track_box(
        file_writer))) {
      ERROR("Failed to write audio movie track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MovieBox::read_movie_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                  || (!m_movie_header_box.read_movie_header_box(
                      file_reader)))) {
      ERROR("Failed to read movie box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_video_track.read_video_movie_track_box(
        file_reader))) {
      ERROR("Failed to read video movie track box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_audio_track.read_audio_movie_track_box(
        file_reader))) {
      ERROR("Failed to read audio movie track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool MovieBox::get_proper_movie_box(MovieBox& movie_box, WriteInfo& write_info)
{
  bool ret = true;
  do {
    memcpy((uint8_t*)(&movie_box.m_base_box), (uint8_t*)(&m_base_box),
           sizeof(BaseBox));
    if(AM_UNLIKELY(!m_movie_header_box.get_proper_movie_header_box(
        movie_box.m_movie_header_box, write_info))) {
      ERROR("Failed to get proper movie header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_video_track.get_proper_video_movie_track_box(
        movie_box.m_video_track, write_info))) {
      ERROR("Failed to get proper video movie track box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_audio_track.get_proper_audio_movie_track_box(
        movie_box.m_audio_track, write_info))) {
      ERROR("Failed to get proper audio movie track box.");
      ret = false;
      break;
    }
  }while(0);
  return ret;
}

void MovieBox::update_movie_box_size()
{
  uint32_t size = DISOM_BOX_SIZE
        + m_movie_header_box.m_base_full_box.m_base_box.m_size;
    m_video_track.update_video_movie_track_box_size();
    size += m_video_track.m_base_box.m_size;
    m_audio_track.update_audio_movie_track_box_size();
    size += m_audio_track.m_base_box.m_size;
    m_base_box.m_size = size;
}
