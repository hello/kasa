/*******************************************************************************
 * iso_base_defs.cpp
 *
 * History:
 *   2016-01-04 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include "am_define.h"
#include "am_log.h"

#include "mp4_file_writer.h"
#include "mp4_file_reader.h"
#include "iso_base_defs.h"

MediaInfo::MediaInfo() :
    m_video_duration(0),
    m_audio_duration(0),
    m_video_first_frame(0),
    m_video_last_frame(0),
    m_audio_first_frame(0),
    m_audio_last_frame(0)
{
}

bool MediaInfo::set_video_duration(uint64_t video_duration)
{
  bool ret = true;
  if (video_duration > 0) {
    m_video_duration = video_duration;
  } else {
    ERROR("video duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool MediaInfo::set_audio_duration(uint64_t audio_duration)
{
  bool ret = true;
  if (audio_duration > 0) {
    m_audio_duration = audio_duration;
  } else {
    ERROR("audio duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool MediaInfo::set_gps_duration(uint64_t gps_duration)
{
  bool ret = true;
  if (gps_duration > 0) {
    m_gps_duration = gps_duration;
  } else {
    ERROR("gps duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool MediaInfo::set_video_first_frame(uint32_t video_first_frame)
{
  bool ret = true;
  if (video_first_frame > 0) {
    if ((m_video_last_frame > 0) && (video_first_frame > m_video_last_frame)) {
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

bool MediaInfo::set_video_last_frame(uint32_t video_last_frame)
{
  bool ret = true;
  if (video_last_frame > 0) {
    if ((m_video_first_frame > 0)
        && (video_last_frame < m_video_first_frame)) {
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

bool MediaInfo::set_audio_first_frame(uint32_t audio_first_frame)
{
  bool ret = true;
  if (audio_first_frame > 0) {
    if ((m_audio_last_frame > 0) && (audio_first_frame > m_audio_last_frame)) {
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

bool MediaInfo::set_audio_last_frame(uint32_t audio_last_frame)
{
  bool ret = true;
  if (audio_last_frame > 0) {
    if ((m_audio_first_frame > 0)
        && (audio_last_frame < m_audio_first_frame)) {
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

bool MediaInfo::set_gps_first_frame(uint32_t gps_first_frame)
{
  bool ret = true;
  if (gps_first_frame > 0) {
    if ((m_gps_last_frame > 0) && (gps_first_frame > m_gps_last_frame)) {
      ERROR("GPS first frame is bigger than gps last frame");
      ret = false;
    } else {
      m_gps_first_frame = gps_first_frame;
    }
  } else {
    ERROR("GPS first frame should not be zero.");
    ret = false;
  }
  return ret;
}

bool MediaInfo::set_gps_last_frame(uint32_t gps_last_frame)
{
  bool ret = true;
  if (gps_last_frame > 0) {
    if ((m_gps_first_frame > 0)
        && (gps_last_frame < m_gps_first_frame)) {
      ERROR("GPS last frame is smaller than gps first frame");
      ret = false;
    } else {
      m_gps_last_frame = gps_last_frame;
    }
  } else {
    ERROR("GPS last frame should not be zero");
    ret = false;
  }
  return ret;
}

uint64_t MediaInfo::get_video_duration()
{
  return m_video_duration;
}

uint64_t MediaInfo::get_audio_duration()
{
  return m_audio_duration;
}

uint64_t MediaInfo::get_gps_duration()
{
  return m_gps_duration;
}

uint32_t MediaInfo::get_video_first_frame()
{
  return m_video_first_frame;
}

uint32_t MediaInfo::get_video_last_frame()
{
  return m_video_last_frame;
}

uint32_t MediaInfo::get_audio_first_frame()
{
  return m_audio_first_frame;
}

uint32_t MediaInfo::get_audio_last_frame()
{
  return m_audio_last_frame;
}

uint32_t MediaInfo::get_gps_first_frame()
{
  return m_gps_first_frame;
}

uint32_t MediaInfo::get_gps_last_frame()
{
  return m_gps_last_frame;
}

bool MediaInfo::get_video_sub_info(SubMediaInfo& video_sub_info)
{
  bool ret = true;
  video_sub_info.set_duration(m_video_duration);
  video_sub_info.set_first_frame(m_video_first_frame);
  video_sub_info.set_last_frame(m_video_last_frame);
  return ret;
}

bool MediaInfo::get_audio_sub_info(SubMediaInfo& audio_sub_info)
{
  bool ret = true;
  audio_sub_info.set_duration(m_audio_duration);
  audio_sub_info.set_first_frame(m_audio_first_frame);
  audio_sub_info.set_last_frame(m_audio_last_frame);
  return ret;
}

bool MediaInfo::get_gps_sub_info(SubMediaInfo& gps_sub_info)
{
  bool ret = true;
  gps_sub_info.set_duration(m_gps_duration);
  gps_sub_info.set_first_frame(m_gps_first_frame);
  gps_sub_info.set_last_frame(m_gps_last_frame);
  return ret;
}

SubMediaInfo::SubMediaInfo() :
    m_duration(0),
    m_first_frame(0),
    m_last_frame(0)
{
}

bool SubMediaInfo::set_duration(uint64_t duration)
{
  bool ret = true;
  if (duration > 0) {
    m_duration = duration;
  } else {
    ERROR("Duration should not below zero.");
    ret = false;
  }
  return ret;
}

bool SubMediaInfo::set_first_frame(uint32_t first_frame)
{
  bool ret = true;
  if (first_frame > 0) {
    if ((m_last_frame > 0) && (first_frame >= m_last_frame)) {
      ERROR("First frame is bigger than audio last frame");
      ret = false;
    } else {
      m_first_frame = first_frame;
    }
  } else {
    ERROR("First frame should not be zero.");
    ret = false;
  }
  return ret;
}

bool SubMediaInfo::set_last_frame(uint32_t last_frame)
{
  bool ret = true;
  if (last_frame > 0) {
    if ((m_first_frame > 0) && (last_frame <= m_first_frame)) {
      ERROR("Last frame is smaller than first frame");
      ret = false;
    } else {
      m_last_frame = last_frame;
    }
  } else {
    ERROR("Last frame should not be zero");
    ret = false;
  }
  return ret;
}

uint64_t SubMediaInfo::get_duration()
{
  return m_duration;
}

uint32_t SubMediaInfo::get_first_frame()
{
  return m_first_frame;
}

uint32_t SubMediaInfo::get_last_frame()
{
  return m_last_frame;
}

BaseBox::BaseBox() :
    m_size(0),
    m_type(DISOM_BOX_TAG_NULL),
    m_enable(false)
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
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )&m_type,
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
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_size))) {
      ERROR("Failed to read size of base box.");
      ret = false;
      break;
    }
    uint8_t type[5] = { 0 };
    if (AM_UNLIKELY(file_reader->read_data(type, 4) != 4)) {
      ERROR("Failed to read type of base box.");
      ret = false;
      break;
    }
    INFO("read base box size success, size is %u, type is %s", m_size, type);
    m_type = DISOM_BOX_TAG(DISO_BOX_TAG_CREATE(type[0], type[1],
                                               type[2], type[3]));
  } while (0);
  return ret;
}

void BaseBox::copy_base_box(BaseBox& box)
{
  box.m_enable = m_enable;
  box.m_size = m_size;
  box.m_type = m_type;
}

FullBox::FullBox() :
    m_flags(0),
    m_version(0)
{
}

bool FullBox::write_full_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write base box in the full box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(((uint32_t ) m_version << 24) |
                                               ((uint32_t )m_flags)))) {
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
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box of full box.");
      ret = false;
      break;
    }
    uint32_t value = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u32(value))) {
      ERROR("Failed to read full box.");
      ret = false;
      break;
    }
    m_version = (uint8_t) ((value >> 24) & 0xff);
    m_flags = value & 0x00ffffff;
    INFO("read version and flag of full box success, version is %u, flag is %u",
         m_version, m_flags);
  } while (0);
  return ret;
}

void FullBox::copy_full_box(FullBox& box)
{
  m_base_box.copy_base_box(box.m_base_box);
  box.m_flags = m_flags;
  box.m_version = m_version;
}

bool FreeBox::write_free_box(Mp4FileWriter* file_writer)
{
  if (m_base_box.m_enable) {
    return m_base_box.write_base_box(file_writer);
  } else {
    ERROR("Free box is not enabled.");
    return false;
  }
}

bool FreeBox::read_free_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box of free box.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

FileTypeBox::FileTypeBox() :
    m_compatible_brands_number(0),
    m_minor_version(0),
    m_major_brand(DISOM_BRAND_NULL)
{
  for (int i = 0; i < DISOM_MAX_COMPATIBLE_BRANDS; i ++) {
    m_compatible_brands[i] = DISOM_BRAND_NULL;
  }
}

bool FileTypeBox::write_file_type_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The file type box is not enabled.");
      ret = false;
      break;
    }
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
      if (AM_UNLIKELY(!file_writer->write_data(
          (uint8_t*)&m_compatible_brands[i], DISOM_TAG_SIZE))) {
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
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read file type box.");
      ret = false;
      break;
    }
    uint8_t major_brand[4];
    if (AM_UNLIKELY(file_reader->read_data(major_brand, 4) != 4)) {
      ERROR("Failed to read major brand when read file type box.");
      ret = false;
      break;
    }
    m_major_brand = DISO_BRAND_TAG(DISO_BOX_TAG_CREATE(
        major_brand[0], major_brand[1], major_brand[2], major_brand[3]));
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_minor_version))) {
      ERROR("Failed to read minor version when read file type box.");
      ret = false;
      break;
    }
    /*read file type box compatible brands*/
    uint32_t compatible_brands_size = m_base_box.m_size
        - sizeof(m_base_box.m_size) - sizeof(m_base_box.m_type)
        - sizeof(major_brand) - sizeof(m_minor_version);
    if (AM_UNLIKELY((compatible_brands_size < 0)
                 || (compatible_brands_size % 4 != 0))) {
      ERROR("compatible brands size is not correct, failed to read file "
          "type box.");
      ret = false;
      break;
    }
    if ((m_compatible_brands_number = compatible_brands_size / 4) >
        DISOM_MAX_COMPATIBLE_BRANDS) {
      ERROR("compatible brands number is larger than "
            "DISOM_MAX_COMPATIBLE_BRANDS : %u, error.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_compatible_brands_number; ++ i) {
      uint8_t compatible_brands[4] =  { 0 };
      if (AM_UNLIKELY(file_reader->read_data(compatible_brands, 4) != 4)) {
        ERROR("Failed to read compatible brands of file type box");
        ret = false;
        break;
      }
      m_compatible_brands[i] = DISO_BRAND_TAG(
          DISO_BOX_TAG_CREATE(compatible_brands[0], compatible_brands[1],
                              compatible_brands[2], compatible_brands[3]));
    }
    if (AM_UNLIKELY(!ret)) {
      ERROR("Failed to read file type box.");
      break;
    }
    m_base_box.m_enable = true;
    INFO("Read file type box success.");
  } while (0);
  return ret;
}

void FileTypeBox::copy_file_type_box(FileTypeBox& box)
{
  m_base_box.copy_base_box(box.m_base_box);
  box.m_compatible_brands_number = m_compatible_brands_number;
  box.m_minor_version = m_minor_version;
  box.m_major_brand = m_major_brand;
  memcpy(box.m_compatible_brands, m_compatible_brands,
         DISOM_MAX_COMPATIBLE_BRANDS * sizeof(m_compatible_brands[0]));
}

bool MediaDataBox::write_media_data_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media data box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write media data box");
      ret = false;
      break;
    }
    INFO("write media data box success.");
  } while (0);
  return ret;
}
bool MediaDataBox::read_media_data_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read media data box");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read media data box success.");
  } while (0);
  return ret;
}

MovieHeaderBox::MovieHeaderBox() :
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
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The movie header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write base full box in movie header box");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
      if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time))
          || (!file_writer->write_be_u32((uint32_t )m_modification_time))
          || (!file_writer->write_be_u32(m_timescale))
          || (!file_writer->write_be_u32((uint32_t )m_duration)))) {
        ERROR("Failed to write movie header box");
        ret = false;
        break;
      }
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                   || (!file_writer->write_be_u64(m_modification_time))
                   || (!file_writer->write_be_u32(m_timescale))
                   || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write movie header box");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported now.", m_full_box.m_version);
      ret = false;
      break;
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
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_matrix[i]))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    for (uint32_t i = 0; i < 6; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_pre_defined[i]))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    if (AM_UNLIKELY(!file_writer->write_be_u32(m_next_track_ID))) {
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
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read base full box when read movie header box");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
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
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                   || (!file_reader->read_le_u64(m_modification_time))
                   || (!file_reader->read_le_u32(m_timescale))
                   || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read movie header box");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported now.", m_full_box.m_version);
      ret = false;
      break;
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
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_matrix[i]))) {
        ERROR("Failed to read movie header box");
        ret = false;
        return ret;
      }
    }
    for (uint32_t i = 0; i < 6; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_pre_defined[i]))) {
        ERROR("Failed to read movie header box");
        ret = false;
        return ret;
      }
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_next_track_ID))) {
      ERROR("Failed to read movie header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read movie header box size error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read movie header box success.");
  } while (0);
  return ret;
}

bool MovieHeaderBox::get_proper_movie_header_box(
    MovieHeaderBox& box, MediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_creation_time = m_creation_time;
    box.m_modification_time = m_modification_time;
    uint64_t video_duration = media_info.get_video_duration();
    uint64_t audio_duration = media_info.get_audio_duration();
    box.m_duration =  (video_duration > audio_duration) ?
                       video_duration : audio_duration;
    box.m_timescale = m_timescale;
    box.m_rate = m_rate;
    box.m_reserved_1 = m_reserved_1;
    box.m_reserved_2 = m_reserved_2;
    memcpy((uint8_t*) box.m_matrix, (uint8_t*) m_matrix,
           9 * sizeof(uint32_t));
    memcpy((uint8_t*) box.m_pre_defined, (uint8_t*) m_pre_defined,
           6 * sizeof(uint32_t));
    box.m_next_track_ID = m_next_track_ID;
    box.m_volume = m_volume;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool MovieHeaderBox::copy_movie_header_box(MovieHeaderBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_creation_time = m_creation_time;
    box.m_modification_time = m_modification_time;
    box.m_duration =  m_duration;
    box.m_timescale = m_timescale;
    box.m_rate = m_rate;
    box.m_reserved_1 = m_reserved_1;
    box.m_reserved_2 = m_reserved_2;
    memcpy((uint8_t*) box.m_matrix, (uint8_t*) m_matrix,
           9 * sizeof(uint32_t));
    memcpy((uint8_t*) box.m_pre_defined, (uint8_t*) m_pre_defined,
           6 * sizeof(uint32_t));
    box.m_next_track_ID = m_next_track_ID;
    box.m_volume = m_volume;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool MovieHeaderBox::get_combined_movie_header_box(MovieHeaderBox& source_box,
                                                   MovieHeaderBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined movie header box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_creation_time = m_creation_time;
    combined_box.m_modification_time = m_modification_time;
    combined_box.m_duration = source_box.m_duration + m_duration;
    if (source_box.m_timescale != m_timescale) {
      ERROR("The timescale of movie header box is not same.");
      ret = false;
      break;
    } else {
      combined_box.m_timescale = m_timescale;
    }
    if (source_box.m_rate != m_rate) {
      ERROR("The rate of movie header box is not same");
      ret = false;
      break;
    } else {
      combined_box.m_rate = m_rate;
    }
    combined_box.m_reserved_1 = m_reserved_1;
    combined_box.m_reserved_2 = m_reserved_2;
    memcpy((uint8_t*) (combined_box.m_matrix),
           (uint8_t*) m_matrix, sizeof(uint32_t) * 9);
    memcpy((uint8_t*) combined_box.m_pre_defined,
           (uint8_t*) m_pre_defined, sizeof(uint32_t) * 6);
    combined_box.m_next_track_ID = m_next_track_ID;
    combined_box.m_volume = m_volume;
    combined_box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

ObjectDescBox::ObjectDescBox() :
    m_iod_type_tag(0x10),
    m_desc_type_length(7),
    m_od_profile_level(0xff),
    m_scene_profile_level(0xff),
    m_audio_profile_level(0x02),
    m_video_profile_level(0x01),
    m_graphics_profile_level(0xff)
{
  m_extended_desc_type[0] = 0x80;
  m_extended_desc_type[1] = 0x80;
  m_extended_desc_type[2] = 0x80;
  m_od_id[0] = 0x00;
  m_od_id[1] = 0x4f;
}

bool ObjectDescBox::write_object_desc_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.write_full_box(file_writer)) {
      ERROR("Failed to write full box when write object desc box.");
      ret = false;
      break;
    }
    if (!file_writer->write_be_u8(m_iod_type_tag)) {
      ERROR("Failed to write iod type tag when write object desc box.");
      ret = false;
      break;
    }
    if (!file_writer->write_data(m_extended_desc_type,
                                 sizeof(m_extended_desc_type))) {
      ERROR("Failed to write m_extended_desc_type when write object desc box.");
      ret = false;
      break;
    }
    if ((!file_writer->write_be_u8(m_desc_type_length))
     || (!file_writer->write_data(m_od_id, sizeof(m_od_id)))
     || (!file_writer->write_be_u8(m_od_profile_level))
     || (!file_writer->write_be_u8(m_scene_profile_level))
     || (!file_writer->write_be_u8(m_audio_profile_level))
     || (!file_writer->write_be_u8(m_video_profile_level))
     || (!file_writer->write_be_u8(m_graphics_profile_level))) {
      ERROR("Failed write data to file when write object desc box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool ObjectDescBox::read_object_desc_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (!m_full_box.read_full_box(file_reader)) {
      ERROR("Failed to read full box when read object desc box.");
      ret = false;
      break;
    }
    if (!file_reader->read_le_u8(m_iod_type_tag)) {
      ERROR("Failed to read iod type tag when read object desc box.");
      ret = false;
      break;
    }
    if (!file_reader->read_data(m_extended_desc_type,
                                sizeof(m_extended_desc_type))) {
      ERROR("Failed to read m_extended_desc_type when read object desc box.");
      ret = false;
      break;
    }
    if ((!file_reader->read_le_u8(m_desc_type_length))
     || (!file_reader->read_data(m_od_id, sizeof(m_od_id)))
     || (!file_reader->read_le_u8(m_od_profile_level))
     || (!file_reader->read_le_u8(m_scene_profile_level))
     || (!file_reader->read_le_u8(m_audio_profile_level))
     || (!file_reader->read_le_u8(m_video_profile_level))
     || (!file_reader->read_le_u8(m_graphics_profile_level))) {
      ERROR("Failed read data when read object desc box.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool ObjectDescBox::get_proper_ojbect_desc_box(ObjectDescBox& box,
                                               MediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_iod_type_tag = m_iod_type_tag;
    memcpy(box.m_extended_desc_type, m_extended_desc_type,
           sizeof(m_extended_desc_type));
    box.m_desc_type_length = m_desc_type_length;
    memcpy(box.m_od_id, m_od_id, sizeof(m_od_id));
    box.m_od_profile_level = m_od_profile_level;
    box.m_scene_profile_level = m_scene_profile_level;
    box.m_audio_profile_level = m_audio_profile_level;
    box.m_video_profile_level = m_video_profile_level;
    box.m_graphics_profile_level = m_graphics_profile_level;
  } while (0);
  return ret;
}

bool ObjectDescBox::copy_object_desc_box(ObjectDescBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_iod_type_tag = m_iod_type_tag;
    memcpy(box.m_extended_desc_type, m_extended_desc_type,
           sizeof(m_extended_desc_type));
    box.m_desc_type_length = m_desc_type_length;
    memcpy(box.m_od_id, m_od_id, sizeof(m_od_id));
    box.m_od_profile_level = m_od_profile_level;
    box.m_scene_profile_level = m_scene_profile_level;
    box.m_audio_profile_level = m_audio_profile_level;
    box.m_video_profile_level = m_video_profile_level;
    box.m_graphics_profile_level = m_graphics_profile_level;
  } while (0);
  return ret;
}

bool ObjectDescBox::get_combined_object_desc_box(ObjectDescBox& source_box,
                                                 ObjectDescBox& combined_box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (source_box.m_iod_type_tag != m_iod_type_tag) {
      WARN("The iod_type_tag of first box is not same with second box");
    }
    combined_box.m_iod_type_tag = m_iod_type_tag;
    if (memcmp(source_box.m_extended_desc_type, m_extended_desc_type,
               sizeof(m_extended_desc_type)) != 0) {
      WARN("The extended_desc_type of first box is not same with second box.");
    }
    memcpy(combined_box.m_extended_desc_type, m_extended_desc_type,
           sizeof(m_extended_desc_type));
    if (source_box.m_desc_type_length != m_desc_type_length) {
      WARN("The m_desc_type_length of first box is not same with second box.");
    }
    combined_box.m_desc_type_length = m_desc_type_length;
    if (memcmp(source_box.m_od_id, m_od_id, sizeof(m_od_id)) != 0) {
      WARN("The m_od_id of first box is not same with second box.");
    }
    memcpy(combined_box.m_od_id, m_od_id, sizeof(m_od_id));
    if (source_box.m_od_profile_level != m_od_profile_level) {
      WARN("The m_od_profile_level of first box is not same with second box.");
    }
    combined_box.m_od_profile_level = m_od_profile_level;
    if (source_box.m_scene_profile_level != m_scene_profile_level) {
      WARN("The m_scene_profile_level of first box is not same with second box.");
    }
    combined_box.m_scene_profile_level = m_scene_profile_level;
    if (source_box.m_audio_profile_level != m_audio_profile_level) {
      WARN("The m_audio_profile_level of first box is not same with second box.");
    }
    combined_box.m_audio_profile_level = m_audio_profile_level;
    if (source_box.m_video_profile_level != m_video_profile_level) {
      WARN("The m_video_profile_level of first box is not same with second box.");
    }
    combined_box.m_video_profile_level = m_video_profile_level;
    if (source_box.m_graphics_profile_level != m_graphics_profile_level) {
      WARN("The m_graphics_profile_level of first box is not same "
           "with second box.");
    }
    combined_box.m_graphics_profile_level = m_graphics_profile_level;
  } while (0);
  return ret;
}

TrackHeaderBox::TrackHeaderBox() :
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_track_ID(0),
    m_reserved(0),
    m_width_integer(0),
    m_width_decimal(0),
    m_height_integer(0),
    m_height_decimal(0),
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
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The movie track header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
      if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time))
          || (!file_writer->write_be_u32((uint32_t )m_modification_time))
          || (!file_writer->write_be_u32(m_track_ID))
          || (!file_writer->write_be_u32(m_reserved))
          || (!file_writer->write_be_u32((uint32_t )m_duration)))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                   || (!file_writer->write_be_u64(m_modification_time))
                   || (!file_writer->write_be_u32(m_track_ID))
                   || (!file_writer->write_be_u32(m_reserved))
                   || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported currently.",
            m_full_box.m_version);
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved_1[i]))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_width_integer))
             || (!file_writer->write_be_u16(m_width_decimal)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_height_integer))
                    || (!file_writer->write_be_u16(m_height_decimal)))) {
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
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read movie track header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
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
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                   || (!file_reader->read_le_u64(m_modification_time))
                   || (!file_reader->read_le_u32(m_track_ID))
                   || (!file_reader->read_le_u32(m_reserved))
                   || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read movie track header box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported currently.",
            m_full_box.m_version);
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved_1[i]))) {
        ERROR("Failed to read movie track header box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_width_integer))
                 || (!file_reader->read_le_u16(m_width_decimal)))) {
      ERROR("Failed to read width of track header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_height_integer))
                    || (!file_reader->read_le_u16(m_height_decimal)))) {
      ERROR("Failed to read height of track header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read movie track header box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read movie track header box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read movie track header box success.");
  } while (0);
  return ret;
}

bool TrackHeaderBox::get_proper_track_header_box(
           TrackHeaderBox& track_header_box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(track_header_box.m_full_box);
    track_header_box.m_creation_time = m_creation_time;
    track_header_box.m_modification_time = m_modification_time;
    track_header_box.m_duration = sub_media_info.get_duration();
    track_header_box.m_track_ID = m_track_ID;
    track_header_box.m_reserved = m_reserved;
    memcpy((uint8_t*) track_header_box.m_reserved_1, (uint8_t*) m_reserved_1,
           2 * sizeof(uint32_t));
    memcpy((uint8_t*) (track_header_box.m_matrix), (uint8_t*) (m_matrix),
           9 * sizeof(uint32_t));
    track_header_box.m_width_integer = m_width_integer;
    track_header_box.m_width_decimal = m_width_decimal;
    track_header_box.m_height_integer = m_height_integer;
    track_header_box.m_height_decimal = m_height_decimal;
    track_header_box.m_layer = m_layer;
    track_header_box.m_alternate_group = m_alternate_group;
    track_header_box.m_volume = m_volume;
    track_header_box.m_reserved_2 = m_reserved_2;
  } while (0);
  return ret;
}

bool TrackHeaderBox::copy_track_header_box(TrackHeaderBox& track_header_box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(track_header_box.m_full_box);
    track_header_box.m_creation_time = m_creation_time;
    track_header_box.m_modification_time = m_modification_time;
    track_header_box.m_duration = m_duration;
    track_header_box.m_track_ID = m_track_ID;
    track_header_box.m_reserved = m_reserved;
    memcpy((uint8_t*) track_header_box.m_reserved_1, (uint8_t*) m_reserved_1,
           2 * sizeof(uint32_t));
    memcpy((uint8_t*) (track_header_box.m_matrix), (uint8_t*) (m_matrix),
           9 * sizeof(uint32_t));
    track_header_box.m_width_integer = m_width_integer;
    track_header_box.m_width_decimal = m_width_decimal;
    track_header_box.m_height_integer = m_height_integer;
    track_header_box.m_height_decimal = m_height_decimal;
    track_header_box.m_layer = m_layer;
    track_header_box.m_alternate_group = m_alternate_group;
    track_header_box.m_volume = m_volume;
    track_header_box.m_reserved_2 = m_reserved_2;
  } while (0);
  return ret;
}

bool TrackHeaderBox::get_combined_track_header_box(TrackHeaderBox& source_box,
                                                   TrackHeaderBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined track header box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_creation_time = m_creation_time;
    combined_box.m_modification_time = m_modification_time;
    combined_box.m_duration = m_duration + source_box.m_duration;
    combined_box.m_track_ID = m_track_ID;
    combined_box.m_reserved = m_reserved;
    memcpy((uint8_t*) combined_box.m_reserved_1, (uint8_t*) m_reserved_1,
           sizeof(uint32_t) * 2);
    memcpy((uint8_t*) combined_box.m_matrix, (uint8_t*) m_matrix,
           sizeof(uint32_t) * 9);
    if ((source_box.m_width_integer != m_width_integer)
     || (source_box.m_height_integer != m_height_integer)) {
      ERROR("The width or height of track header box is not same.");
      ret = false;
      break;
    } else {
      combined_box.m_height_integer = m_height_integer;
      combined_box.m_height_decimal = m_height_decimal;
      combined_box.m_width_integer = m_width_integer;
      combined_box.m_width_decimal = m_width_decimal;
    }
    combined_box.m_layer = m_layer;
    combined_box.m_alternate_group = m_alternate_group;
    combined_box.m_volume = m_volume;
    combined_box.m_reserved_2 = m_reserved_2;
  } while (0);
  return ret;
}
MediaHeaderBox::MediaHeaderBox() :
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_timescale(0),
    m_pre_defined(0),
    m_pad(0)
{
  memset(m_language, 0, sizeof(m_language));
}

bool MediaHeaderBox::write_media_header_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The media header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write media header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
      uint32_t creation_time = m_creation_time;
      uint32_t modification_time = m_modification_time;
      uint32_t duration = m_duration;
      if (AM_UNLIKELY((!file_writer->write_be_u32(creation_time))
                   || (!file_writer->write_be_u32(modification_time))
                   || (!file_writer->write_be_u32(m_timescale))
                   || (!file_writer->write_be_u32(duration)))) {
        ERROR("Failed to write media header box.");
        ret = false;
        break;
      }
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_writer->write_be_u64(m_creation_time))
                   || (!file_writer->write_be_u64(m_modification_time))
                   || (!file_writer->write_be_u32(m_timescale))
                   || (!file_writer->write_be_u64(m_duration)))) {
        ERROR("Failed to write media header box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported currently",
            m_full_box.m_version);
      ret = false;
      break;
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

bool MediaHeaderBox::read_media_header_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read media header box.");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_full_box.m_version == 0)) {
      uint32_t creation_time = 0;
      uint32_t modification_time = 0;
      uint32_t duration = 0;
      if (AM_UNLIKELY((!file_reader->read_le_u32(creation_time))
                   || (!file_reader->read_le_u32(modification_time))
                   || (!file_reader->read_le_u32(m_timescale))
                   || (!file_reader->read_le_u32(duration)))) {
        ERROR("Failed to read media header box.");
        ret = false;
        break;
      }
      m_creation_time = creation_time;
      m_modification_time = modification_time;
      m_duration = duration;
    } else if (m_full_box.m_version == 1){
      if (AM_UNLIKELY((!file_reader->read_le_u64(m_creation_time))
                   || (!file_reader->read_le_u64(m_modification_time))
                   || (!file_reader->read_le_u32(m_timescale))
                   || (!file_reader->read_le_u64(m_duration)))) {
        ERROR("Failed to read media header box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The version %u is not supported currently",
            m_full_box.m_version);
      ret = false;
      break;
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
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read media header box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read media header box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read media header box success.");
  } while (0);
  return ret;
}

bool MediaHeaderBox::get_proper_media_header_box(
    MediaHeaderBox& media_header_box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(media_header_box.m_full_box);
    media_header_box.m_creation_time = m_creation_time;
    media_header_box.m_modification_time = m_modification_time;
    media_header_box.m_duration = sub_media_info.get_duration();
    media_header_box.m_timescale = m_timescale;
    media_header_box.m_pre_defined = m_pre_defined;
    media_header_box.m_pad = m_pad;
    memcpy(media_header_box.m_language, m_language, 3 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool MediaHeaderBox::copy_media_header_box(MediaHeaderBox& media_header_box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(media_header_box.m_full_box);
    media_header_box.m_creation_time = m_creation_time;
    media_header_box.m_modification_time = m_modification_time;
    media_header_box.m_duration = m_duration;
    media_header_box.m_timescale = m_timescale;
    media_header_box.m_pre_defined = m_pre_defined;
    media_header_box.m_pad = m_pad;
    memcpy(media_header_box.m_language, m_language, 3 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool MediaHeaderBox::get_combined_media_header_box(MediaHeaderBox& source_box,
                                                   MediaHeaderBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined media header box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_creation_time = m_creation_time;
    combined_box.m_modification_time = m_modification_time;
    combined_box.m_duration = m_duration + source_box.m_duration;
    if (source_box.m_timescale != m_timescale) {
      ERROR("The timescale of media header box is not same.");
      ret = false;
      break;
    } else {
      combined_box.m_timescale = m_timescale;
    }
    combined_box.m_pre_defined = m_pre_defined;
    combined_box.m_pad = m_pad;
    memcpy(combined_box.m_language, m_language, 3 * sizeof(uint8_t));
  } while (0);
  return ret;
}

HandlerReferenceBox::HandlerReferenceBox() :
    m_name(nullptr),
    m_handler_type(NULL_TRACK),
    m_pre_defined(0),
    m_name_size(0)
{
  memset(m_reserved, 0, sizeof(m_reserved));
}

HandlerReferenceBox::~HandlerReferenceBox()
{
  delete[] m_name;
}

bool HandlerReferenceBox::write_handler_reference_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The handler reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer)) ||
                    (!file_writer->write_be_u32(m_pre_defined)) ||
                    (!file_writer->write_data((uint8_t*)&m_handler_type,
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (m_name) {
      if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )(m_name),
                                               m_name_size))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    }
    INFO("write handler reference box success.");
  } while (0);
  return ret;
}

bool HandlerReferenceBox::read_handler_reference_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint32_t handler_type = 0;
    if (AM_UNLIKELY((!m_full_box.read_full_box(file_reader))
                 || (!file_reader->read_le_u32(m_pre_defined))
                 || (!file_reader->read_box_tag(handler_type)))) {
      ERROR("Failed to read hander reference box");
      ret = false;
      break;
    }
    INFO("pre_defined of handler reference box is %u", m_pre_defined);
    m_handler_type = DISO_HANDLER_TYPE(handler_type);
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved[i]))) {
        ERROR("Failed to read handler reference box.");
        ret = false;
        break;
      }
      INFO("m_reserved of handler reference box is %u", m_reserved[i]);
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    m_name_size = m_full_box.m_base_box.m_size
                       - DISOM_HANDLER_REF_SIZE;
    if (m_name_size > 0) {
      INFO("Name size of handler reference box is %u", m_name_size);
      delete[] m_name;
      m_name = new char[m_name_size + 1];
      memset(m_name, 0, m_name_size + 1);
      if (AM_UNLIKELY(!m_name)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_reader->read_data((uint8_t* )(m_name),
                                              m_name_size))) {
        ERROR("Failed to read handler reference box.");
        ret = false;
        break;
      }
      INFO("name of handler reference box is %s", m_name);
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read handler reference box success.");
  } while (0);
  return ret;
}

bool HandlerReferenceBox::get_proper_handler_reference_box(
    HandlerReferenceBox& handler_reference_box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(handler_reference_box.m_full_box);
    handler_reference_box.m_handler_type = m_handler_type;
    handler_reference_box.m_pre_defined = m_pre_defined;
    memcpy((uint8_t*) (handler_reference_box.m_reserved),
           (uint8_t*) m_reserved, 3 * sizeof(uint32_t));
    handler_reference_box.m_name_size = m_name_size;
    if (m_name_size > 0) {
      delete[] handler_reference_box.m_name;
      handler_reference_box.m_name = new char[m_name_size];
      if (AM_UNLIKELY(!handler_reference_box.m_name)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(handler_reference_box.m_name, m_name, m_name_size);
    }
  } while (0);
  return ret;
}

bool HandlerReferenceBox::copy_handler_reference_box(
    HandlerReferenceBox& handler_reference_box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(handler_reference_box.m_full_box);
    handler_reference_box.m_handler_type = m_handler_type;
    handler_reference_box.m_pre_defined = m_pre_defined;
    memcpy((uint8_t*) (handler_reference_box.m_reserved),
           (uint8_t*) m_reserved, 3 * sizeof(uint32_t));
    handler_reference_box.m_name_size = m_name_size;
    if (m_name_size > 0) {
      delete[] handler_reference_box.m_name;
      handler_reference_box.m_name = new char[m_name_size];
      if (AM_UNLIKELY(!handler_reference_box.m_name)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(handler_reference_box.m_name, m_name, m_name_size);
    }
  } while (0);
  return ret;
}

bool HandlerReferenceBox::get_combined_handler_reference_box(
    HandlerReferenceBox& source_box, HandlerReferenceBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One fo combined handler reference box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (source_box.m_handler_type != m_handler_type) {
      ERROR("The handler type of handler reference box is not same.");
      ret = false;
      break;
    } else {
      combined_box.m_handler_type = m_handler_type;
      combined_box.m_pre_defined = m_pre_defined;
      memcpy((uint8_t*) combined_box.m_reserved,
             (uint8_t*) m_reserved, sizeof(uint32_t) * 3);
      combined_box.m_name_size = m_name_size;
      if (m_name_size > 0) {
        delete[] combined_box.m_name;
        combined_box.m_name = new char[m_name_size];
        if (!combined_box.m_name) {
          ERROR("Failed to malloc memory for name of handler reference box.");
          ret = false;
          break;
        }
        memcpy(combined_box.m_name, m_name, m_name_size);
      }
    }
  } while (0);
  return ret;
}

VideoMediaInfoHeaderBox::VideoMediaInfoHeaderBox() :
    m_graphicsmode(0)
{
  memset(m_opcolor, 0, sizeof(m_opcolor));
}

bool VideoMediaInfoHeaderBox::write_video_media_info_header_box(
                                            Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The video media information header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
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

bool VideoMediaInfoHeaderBox::read_video_media_info_header_box(
                                          Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read video media information header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
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
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read video media information header box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read video media information header box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read video media information header box.");
  } while (0);
  return ret;
}

bool VideoMediaInfoHeaderBox::get_proper_video_media_info_header_box(
    VideoMediaInfoHeaderBox& box, SubMediaInfo& video_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*) box.m_opcolor, m_opcolor, 3 * sizeof(uint16_t));
  } while (0);
  return ret;
}

bool VideoMediaInfoHeaderBox::copy_video_media_info_header_box(
    VideoMediaInfoHeaderBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*) box.m_opcolor, m_opcolor, 3 * sizeof(uint16_t));
  } while (0);
  return ret;
}

bool VideoMediaInfoHeaderBox::get_combined_video_media_info_header_box(
    VideoMediaInfoHeaderBox& source_box,VideoMediaInfoHeaderBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined video media information header box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*) combined_box.m_opcolor, (uint8_t*) m_opcolor,
           sizeof(uint16_t) * 3);
  } while (0);
  return ret;
}

SoundMediaInfoHeaderBox::SoundMediaInfoHeaderBox() :
    m_balanse(0),
    m_reserved(0)
{
}

bool SoundMediaInfoHeaderBox::write_sound_media_info_header_box(
                                          Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sound media information header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
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

bool SoundMediaInfoHeaderBox::read_sound_media_info_header_box(
                                       Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read sound media information header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_balanse))
                 || (!file_reader->read_le_u16(m_reserved)))) {
      ERROR("Failed to read sound media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read sound media information header box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read sound media information header box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read sound media infromation header box.");
  } while (0);
  return ret;
}

bool SoundMediaInfoHeaderBox::get_proper_sound_media_info_header_box(
    SoundMediaInfoHeaderBox& box, SubMediaInfo& audio_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_balanse = m_balanse;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool SoundMediaInfoHeaderBox::copy_sound_media_info_header_box(
    SoundMediaInfoHeaderBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_balanse = m_balanse;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool SoundMediaInfoHeaderBox::get_combined_sound_media_info_header_box(
    SoundMediaInfoHeaderBox& source_box, SoundMediaInfoHeaderBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined sound media information header box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_balanse = m_balanse;
    combined_box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool NullMediaInfoHeaderBox::write_null_media_info_header_box(
                                        Mp4FileWriter* file_writer)
{
  bool ret = true;
  if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
    ret = false;
  }
  return ret;
}

bool NullMediaInfoHeaderBox::read_null_media_info_header_box(
                                      Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (!m_full_box.read_full_box(file_reader)) {
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool NullMediaInfoHeaderBox::get_proper_null_media_info_header_box(
    NullMediaInfoHeaderBox& box, SubMediaInfo& null_media_info)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool NullMediaInfoHeaderBox::copy_null_media_info_header_box(
    NullMediaInfoHeaderBox& box)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool NullMediaInfoHeaderBox::get_combined_null_media_info_header_box(
    NullMediaInfoHeaderBox& source_box, NullMediaInfoHeaderBox& combined_box)
{
  bool ret = true;
  m_full_box.copy_full_box(combined_box.m_full_box);
  return ret;
}

DecodingTimeToSampleBox::DecodingTimeToSampleBox() :
    m_entry_ptr(nullptr),
    m_entry_buf_count(0),
    m_entry_count(0)
{
}

DecodingTimeToSampleBox::~DecodingTimeToSampleBox()
{
  delete[] m_entry_ptr;
}

bool DecodingTimeToSampleBox::write_decoding_time_to_sample_box(
                                          Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The decoding time to sample box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
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
      if (AM_UNLIKELY((!file_writer->write_be_u32(m_entry_ptr[i].sample_count))
            || (!file_writer->write_be_u32(m_entry_ptr[i].sample_delta)))) {
        ERROR("Failed to write decoding time to sample box.");
        ret = false;
        break;
      }
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
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read decoding time to sample box.");
      ret = false;
      break;
    }
    INFO("begin to read decoding time to sample box.");
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read decoding time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read decoding time to sample box.");
      ret = false;
      break;
    }
    INFO("Read decoding time to sample box, entry count is %u", m_entry_count);
    if (m_entry_count > 0) {
      delete[] m_entry_ptr;
      m_entry_ptr = new TimeEntry[m_entry_count];
      if (AM_UNLIKELY(!m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_entry_ptr[i].sample_count))
            || (!file_reader->read_le_u32(m_entry_ptr[i].sample_delta)))) {
          ERROR("Failed to read decoding time to sample box.");
          ret = false;
          break;
        }
      }
      if (AM_UNLIKELY(!ret)) {
        break;
      }
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read decoding time to sample box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read decoding time to sample box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read decoding time to sample box success.");
  } while (0);
  return ret;
}

void DecodingTimeToSampleBox::update_decoding_time_to_sample_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * TIME_ENTRY_SIZE;
}

bool DecodingTimeToSampleBox::get_proper_decoding_time_to_sample_box(
    DecodingTimeToSampleBox& box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    uint32_t frame_count = 0;
    bool get_begin_entry_count = false;
    bool get_end_entry_count = false;
    uint32_t count_in_begin_entry = 0;
    uint32_t count_in_end_entry = 0;
    uint32_t begin_entry_count = 0;
    uint32_t end_entry_count = 0;
    uint32_t last_frame_count = 0;
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      last_frame_count = frame_count;
      frame_count += m_entry_ptr[i].sample_count;
      if ((!get_begin_entry_count) &&
          (sub_media_info.get_first_frame() <= frame_count)) {
        begin_entry_count = i;
        get_begin_entry_count = true;
        count_in_begin_entry = frame_count -
            sub_media_info.get_first_frame() + 1;
      }
      if ((!get_end_entry_count) &&
          (sub_media_info.get_last_frame() <= frame_count)) {
        end_entry_count = i;
        get_end_entry_count = true;
        count_in_end_entry = sub_media_info.get_last_frame() - last_frame_count;
      }
    }
    if (AM_UNLIKELY(sub_media_info.get_last_frame() > frame_count)) {
      ERROR("Last frame %u is larger than frame count %u, error.",
            sub_media_info.get_last_frame(), frame_count);
      ret = false;
      break;
    }
    if ((!get_begin_entry_count) || (!get_end_entry_count)) {
      ERROR("Failed to find proper entry count.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = end_entry_count - begin_entry_count + 1;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new TimeEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      uint32_t j = 0;
      for (uint32_t i = begin_entry_count; i <= end_entry_count; ++ i, ++ j) {
        if ((i == begin_entry_count) || (i == end_entry_count)) {
          if (i == begin_entry_count) {
            box.m_entry_ptr[j].sample_count = count_in_begin_entry;
            box.m_entry_ptr[j].sample_delta =
                m_entry_ptr[begin_entry_count].sample_delta;
          }
          if (i == end_entry_count) {
            box.m_entry_ptr[j].sample_count += count_in_end_entry;
            box.m_entry_ptr[j].sample_delta =
                m_entry_ptr[end_entry_count].sample_delta;
          }
          if (begin_entry_count == end_entry_count) {
            box.m_entry_ptr[j].sample_count = sub_media_info.get_last_frame()
                - sub_media_info.get_first_frame() + 1;
          }
        } else {
          box.m_entry_ptr[j].sample_count = m_entry_ptr[i].sample_count;
          box.m_entry_ptr[j].sample_delta = m_entry_ptr[i].sample_delta;
        }
      }
    }
  } while (0);
  return ret;
}

bool DecodingTimeToSampleBox::copy_decoding_time_to_sample_box(
    DecodingTimeToSampleBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new TimeEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        box.m_entry_ptr[i].sample_count = m_entry_ptr[i].sample_count;
        box.m_entry_ptr[i].sample_delta = m_entry_ptr[i].sample_delta;
      }
    }
  } while (0);
  return ret;
}

bool DecodingTimeToSampleBox::get_combined_decoding_time_to_sample_box(
    DecodingTimeToSampleBox& source_box, DecodingTimeToSampleBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One fo combined decoding time to sample box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (m_entry_ptr[m_entry_count -1].sample_delta ==
        source_box.m_entry_ptr[0].sample_delta) {
      combined_box.m_entry_count = source_box.m_entry_count + m_entry_count - 1;
      combined_box.m_entry_buf_count = combined_box.m_entry_count;
    } else {
      combined_box.m_entry_count = source_box.m_entry_count + m_entry_count;
      combined_box.m_entry_buf_count = combined_box.m_entry_count;
    }
    if (combined_box.m_entry_count > 0) {
      delete[] combined_box.m_entry_ptr;
      combined_box.m_entry_ptr = new TimeEntry[combined_box.m_entry_buf_count];
      if (!combined_box.m_entry_ptr) {
        ERROR("Failed to malloc memory for decoding time to sample box.");
        ret = false;
        break;
      }
      INFO("entry count is %u, source entry count is %u, combined entry count"
          "is %u", m_entry_count, source_box.m_entry_count,
          combined_box.m_entry_count);
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        combined_box.m_entry_ptr[i].sample_count = m_entry_ptr[i].sample_count;
        combined_box.m_entry_ptr[i].sample_delta = m_entry_ptr[i].sample_delta;
      }
      uint32_t k = 0;
      for (uint32_t j = 0; j < source_box.m_entry_count; ++ j) {
        if ((j == 0) && (combined_box.m_entry_ptr[m_entry_count - 1].sample_delta
            == source_box.m_entry_ptr[0].sample_delta)) {
          combined_box.m_entry_ptr[m_entry_count - 1].sample_count +=
              source_box.m_entry_ptr[0].sample_count;
          ++ k;
        } else {
          combined_box.m_entry_ptr[j + m_entry_count - k].sample_count =
              source_box.m_entry_ptr[j].sample_count;
          combined_box.m_entry_ptr[j + m_entry_count - k].sample_delta =
              source_box.m_entry_ptr[j].sample_delta;
        }
      }
    }
    combined_box.update_decoding_time_to_sample_box_size();
  } while (0);
  return ret;
}

CompositionTimeToSampleBox::CompositionTimeToSampleBox() :
    m_entry_ptr(nullptr),
    m_entry_count(0),
    m_entry_buf_count(0)
{
}

CompositionTimeToSampleBox::~CompositionTimeToSampleBox()
{
  delete[] m_entry_ptr;
}

bool CompositionTimeToSampleBox::write_composition_time_to_sample_box(
                                                Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The composition time to sample box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_ptr[i].sample_count)))
      {
        ERROR("Failed to write sample count of composition time to sample box");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_ptr[i].sample_delta)))
      {
        ERROR("Failed to write sample delta fo composition time to sample box.");
        ret = false;
        break;
      }
    }
    INFO("write composition time to sample box success.");
  } while (0);
  return ret;
}

bool CompositionTimeToSampleBox::read_composition_time_to_sample_box(
                                               Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read composition time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read composition time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read composition time to sample box.");
      ret = false;
      break;
    }
    if (m_entry_count > 0) {
      delete[] m_entry_ptr;
      m_entry_ptr = new TimeEntry[m_entry_count];
      if (AM_UNLIKELY(!m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      NOTICE("composition time to sample box entry count is %u", m_entry_count);
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_entry_ptr[i].sample_count))
            || (!file_reader->read_le_u32(m_entry_ptr[i].sample_delta)))) {
          ERROR("Failed to read composition time to sample box.");
          ret = false;
          break;
        }
      }
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read composition time to sample box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read composition time to sample box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read composition time to sample box success.");
  } while (0);
  return ret;
}

void CompositionTimeToSampleBox::update_composition_time_to_sample_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * TIME_ENTRY_SIZE;
}

bool CompositionTimeToSampleBox::get_proper_composition_time_to_sample_box(
    CompositionTimeToSampleBox& box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    uint32_t frame_count = 0;
    bool get_begin_entry_count = false;
    bool get_end_entry_count = false;
    uint32_t count_in_begin_entry = 0;
    uint32_t count_in_end_entry = 0;
    uint32_t begin_entry_count = 0;
    uint32_t end_entry_count = 0;
    uint32_t last_frame_count = 0;
    frame_count = 0;
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      last_frame_count = frame_count;
      frame_count += m_entry_ptr[i].sample_count;
      if ((!get_begin_entry_count) &&
          (sub_media_info.get_first_frame() <= frame_count)) {
        begin_entry_count = i;
        get_begin_entry_count = true;
        count_in_begin_entry = frame_count -
               sub_media_info.get_first_frame() + 1;
      }
      if ((!get_end_entry_count) &&
          (sub_media_info.get_last_frame() <= frame_count)) {
        end_entry_count = i;
        get_end_entry_count = true;
        count_in_end_entry = sub_media_info.get_last_frame() - last_frame_count;
      }
    }
    if (AM_UNLIKELY(sub_media_info.get_last_frame() > frame_count)) {
      ERROR("Last frame is larger than frame count, error.");
      ret = false;
      break;
    }
    if ((!get_begin_entry_count) || (!get_end_entry_count)) {
      ERROR("Failed to find proper entry count.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = end_entry_count - begin_entry_count + 1;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new TimeEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      uint32_t j = 0;
      for (uint32_t i = begin_entry_count; i <= end_entry_count; ++ i, ++ j) {
        if ((i == begin_entry_count) || (i == end_entry_count)) {
          if (i == begin_entry_count) {
            box.m_entry_ptr[j].sample_count = count_in_begin_entry;
            box.m_entry_ptr[j].sample_delta =
                m_entry_ptr[begin_entry_count].sample_delta;
          }
          if (i == end_entry_count) {
            box.m_entry_ptr[j].sample_count += count_in_end_entry;
            box.m_entry_ptr[j].sample_delta =
                m_entry_ptr[end_entry_count].sample_delta;
          }
        } else {
          box.m_entry_ptr[j].sample_count = m_entry_ptr[i].sample_count;
          box.m_entry_ptr[j].sample_delta = m_entry_ptr[i].sample_delta;
        }
      }
    }
  } while (0);
  return ret;
}

bool CompositionTimeToSampleBox::copy_composition_time_to_sample_box(
    CompositionTimeToSampleBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new TimeEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i <= m_entry_count; ++ i) {
        box.m_entry_ptr[i].sample_count = m_entry_ptr[i].sample_count;
        box.m_entry_ptr[i].sample_delta = m_entry_ptr[i].sample_delta;
      }
    }
  } while (0);
  return ret;
}

bool CompositionTimeToSampleBox::get_combined_composition_time_to_sample_box(
    CompositionTimeToSampleBox& source_box,
    CompositionTimeToSampleBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One fo combined decoding time to sample box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (m_entry_ptr[m_entry_count -1].sample_delta ==
        source_box.m_entry_ptr[0].sample_delta) {
      combined_box.m_entry_count = source_box.m_entry_count + m_entry_count - 1;
      combined_box.m_entry_buf_count = combined_box.m_entry_count;
    } else {
      combined_box.m_entry_count = source_box.m_entry_count + m_entry_count;
      combined_box.m_entry_buf_count = combined_box.m_entry_count;
    }
    if (combined_box.m_entry_count > 0) {
      delete[] combined_box.m_entry_ptr;
      combined_box.m_entry_ptr = new TimeEntry[combined_box.m_entry_buf_count];
      if (!combined_box.m_entry_ptr) {
        ERROR("Failed to malloc memory for decoding time to sample box.");
        ret = false;
        break;
      }
      INFO("entry count is %u, source entry count is %u, combined entry count"
          "is %u", m_entry_count, source_box.m_entry_count,
          combined_box.m_entry_count);
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        combined_box.m_entry_ptr[i].sample_count = m_entry_ptr[i].sample_count;
        combined_box.m_entry_ptr[i].sample_delta = m_entry_ptr[i].sample_delta;
      }
      uint32_t k = 0;
      for (uint32_t j = 0; j < source_box.m_entry_count; ++ j) {
        if ((j == 0) && (combined_box.m_entry_ptr[m_entry_count - 1].sample_delta
            == source_box.m_entry_ptr[0].sample_delta)) {
          combined_box.m_entry_ptr[m_entry_count - 1].sample_count +=
              source_box.m_entry_ptr[0].sample_count;
          ++ k;
        } else {
          combined_box.m_entry_ptr[j + m_entry_count - k].sample_count =
              source_box.m_entry_ptr[j].sample_count;
          combined_box.m_entry_ptr[j + m_entry_count - k].sample_delta =
              source_box.m_entry_ptr[j].sample_delta;
        }
      }
    }
  } while (0);
  return ret;
}

AVCDecoderConfigurationRecord::AVCDecoderConfigurationRecord() :
    m_pps(nullptr),
    m_sps(nullptr),
    m_pps_len(0),
    m_sps_len(0),
    m_config_version(0),
    m_profile_indication(0),
    m_profile_compatibility(0),
    m_level_indication(0),
    m_reserved(0),
    m_len_size_minus_one(0),
    m_reserved_1(0),
    m_sps_num(0),
    m_pps_num(0)
{
}

AVCDecoderConfigurationRecord::~AVCDecoderConfigurationRecord()
{
  delete[] m_pps;
  delete[] m_sps;
}

bool AVCDecoderConfigurationRecord::write_avc_decoder_configuration_record_box(
                                                    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The avc decoder configuration box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                  || (!file_writer->write_be_u8(m_config_version)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_profile_indication))
                 || (!file_writer->write_be_u8(m_profile_compatibility))
                 || (!file_writer->write_be_u8(m_level_indication)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(((uint16_t )m_reserved << 10)
        | ((uint16_t )m_len_size_minus_one << 8)
        | ((uint16_t )m_reserved_1 << 5)
        | ((uint16_t )m_sps_num)))
        || (!file_writer->write_be_u16(m_sps_len))
        || (!file_writer->write_data(m_sps, m_sps_len))
        || (!file_writer->write_be_u8(m_pps_num))
        || (!file_writer->write_be_u16(m_pps_len))
        || (!file_writer->write_data(m_pps, m_pps_len)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    INFO("write avc decoder configuration record box.");
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::read_avc_decoder_configuration_record_box(
                                      Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!file_reader->read_le_u8(m_config_version)))) {
      ERROR("Failed to read avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_profile_indication))
                 || (!file_reader->read_le_u8(m_profile_compatibility))
                 || (!file_reader->read_le_u8(m_level_indication)))) {
      ERROR("Failed to read avc decoder configuration record box.");
      ret = false;
      break;
    }
    uint8_t value = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u8(value))) {
      ERROR("Failed to read data when read decoder configuration record box.");
      ret = false;
      break;
    }
    m_reserved = 0xff;
    m_len_size_minus_one = value & 0x03;
    value = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u8(value))) {
      ERROR("Failed to read data when read decoder configuration record box.");
      ret = false;
      break;
    }
    m_reserved_1 = 0xff;
    m_sps_num = value & 0x1f;
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_sps_len))) {
      ERROR("Failed to read sequence parameter set length when read "
            "decoder configuration record box.");
      ret = false;
      break;
    }
    if (m_sps_len > 0) {
      delete[] m_sps;
      m_sps = new uint8_t[m_sps_len];
      if (AM_UNLIKELY(!m_sps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_reader->read_data(m_sps,
                                              m_sps_len))) {
        ERROR("Failed to read sps when read decoder configuration record box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_pps_num))) {
      ERROR("Failed to read num_of_picture_parameter_sets");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_pps_len))) {
      ERROR("Failed to read picture parameter set length");
      ret = false;
      break;
    }
    if (m_pps_len > 0) {
      delete[] m_pps;
      m_pps = new uint8_t[m_pps_len];
      if (AM_UNLIKELY(!m_pps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_reader->read_data(m_pps, m_pps_len))) {
        ERROR("Failed to read data when read decoder configuration record box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read avc decoder configuration record box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read avc decoder configuration record box.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read decoder configuration record box success.");
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::
   get_proper_avc_decoder_configuration_record_box(
       AVCDecoderConfigurationRecord& box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_pps_len > 0) {
      box.m_pps_len = m_pps_len;
      delete[] box.m_pps;
      box.m_pps = new uint8_t[m_pps_len];
      if (AM_UNLIKELY(!box.m_pps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(box.m_pps, m_pps, m_pps_len);
      if (m_sps_len > 0) {
        box.m_sps_len = m_sps_len;
        delete[] box.m_sps;
        box.m_sps = new uint8_t[m_sps_len];
        if (AM_UNLIKELY(!box.m_sps)) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        memcpy(box.m_sps, m_sps, m_sps_len);
      }
      box.m_config_version = m_config_version;
      box.m_profile_indication = m_profile_indication;
      box.m_profile_compatibility = m_profile_compatibility;
      box.m_level_indication = m_level_indication;
      box.m_reserved = m_reserved;
      box.m_len_size_minus_one = m_len_size_minus_one;
      box.m_reserved_1 = m_reserved_1;
      box.m_sps_num = m_sps_num;
      box.m_pps_num = m_pps_num;
    }
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::
   copy_avc_decoder_configuration_record_box(AVCDecoderConfigurationRecord& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_pps_len > 0) {
      box.m_pps_len = m_pps_len;
      delete[] box.m_pps;
      box.m_pps = new uint8_t[m_pps_len];
      if (AM_UNLIKELY(!box.m_pps)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy(box.m_pps, m_pps, m_pps_len);
      if (m_sps_len > 0) {
        box.m_sps_len = m_sps_len;
        delete[] box.m_sps;
        box.m_sps = new uint8_t[m_sps_len];
        if (AM_UNLIKELY(!box.m_sps)) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        memcpy(box.m_sps, m_sps, m_sps_len);
      }
      box.m_config_version = m_config_version;
      box.m_profile_indication = m_profile_indication;
      box.m_profile_compatibility = m_profile_compatibility;
      box.m_level_indication = m_level_indication;
      box.m_reserved = m_reserved;
      box.m_len_size_minus_one = m_len_size_minus_one;
      box.m_reserved_1 = m_reserved_1;
      box.m_sps_num = m_sps_num;
      box.m_pps_num = m_pps_num;
    }
  } while (0);
  return ret;
}

bool AVCDecoderConfigurationRecord::
     get_combined_avc_decoder_configuration_record_box(
         AVCDecoderConfigurationRecord& source_box,
         AVCDecoderConfigurationRecord& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined avc decoder configuration record box is not enabled.");
      ret = false;
      break;
    }
    if ((source_box.m_pps_len != m_pps_len)
     || (source_box.m_sps_len != m_sps_len)) {
      ERROR("The picture parameter set length or sequence parameter set length "
            "is not same.");
      ret = false;
      break;
    }
    if (strncmp((const char*) m_pps, (const char*) source_box.m_pps,
                (size_t) m_pps_len) != 0) {
      ERROR("The pps is not same.");
      ret = false;
      break;
    }
    if (strncmp((const char*) m_sps, (const char*) source_box.m_sps,
                (size_t) m_sps_len) != 0) {
      ERROR("The sps is not same.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    delete[] combined_box.m_pps;
    combined_box.m_pps = new uint8_t[m_pps_len];
    if (!combined_box.m_pps) {
      ERROR("Failed to malloc memory for pps in avc decoder "
            "configuration record box");
      ret = false;
      break;
    }
    memcpy(combined_box.m_pps, m_pps, m_pps_len);
    delete[] combined_box.m_sps;
    combined_box.m_sps = new uint8_t[m_sps_len];
    if (!combined_box.m_sps) {
      ERROR("Failed to malloc memory for sps in avc decoder "
            "configuration record box.");
      ret = false;
      break;
    }
    memcpy(combined_box.m_sps, m_sps, m_sps_len);
    combined_box.m_pps_len = m_pps_len;
    combined_box.m_sps_len = m_sps_len;
    combined_box.m_config_version = m_config_version;
    combined_box.m_profile_indication = m_profile_indication;
    combined_box.m_profile_compatibility = m_profile_compatibility;
    combined_box.m_level_indication = m_level_indication;
    combined_box.m_reserved = m_reserved;
    combined_box.m_len_size_minus_one = m_len_size_minus_one;
    combined_box.m_reserved_1 = m_reserved_1;
    combined_box.m_sps_num = m_sps_num;
    combined_box.m_pps_num = m_pps_num;
  } while (0);
  return ret;
}

HEVCConArrayNal::HEVCConArrayNal() :
    m_nal_unit(nullptr),
    m_nal_unit_length(0)
{
}

HEVCConArrayNal::~HEVCConArrayNal()
{
  delete[] m_nal_unit;
}

bool HEVCConArrayNal::write_HEVC_con_array_nal(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (m_nal_unit_length == 0) {
      ERROR("The nal unit length is zero.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_nal_unit_length))) {
      ERROR("Failed to write nal unit length when write HEVC con array nal");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data(m_nal_unit, m_nal_unit_length))) {
      ERROR("Failed to write nal unit when write HEVC con array nal.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool HEVCConArrayNal::read_HEVC_con_array_nal(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_nal_unit_length))) {
      ERROR("Failed to read nal unit length when read HEVC con array nal.");
      ret = false;
      break;
    }
    if (m_nal_unit_length > 0) {
      delete[] m_nal_unit;
      m_nal_unit = new uint8_t[m_nal_unit_length];
      if (AM_UNLIKELY(!m_nal_unit)) {
        ERROR("Failed to malloc memory when read HEVC con array nal.");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_reader->read_data(m_nal_unit, m_nal_unit_length))) {
        ERROR("Failed to read data when read HEVC con array nal.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool HEVCConArrayNal::get_proper_HEVC_con_array_nal(
    HEVCConArrayNal& HEVC_con_array_nal, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    if (m_nal_unit_length == 0) {
      ERROR("The nal unit length is zero");
      ret = false;
      break;
    }
    HEVC_con_array_nal.m_nal_unit_length = m_nal_unit_length;
    if (HEVC_con_array_nal.m_nal_unit_length > 0) {
      delete[] HEVC_con_array_nal.m_nal_unit;
      HEVC_con_array_nal.m_nal_unit = new uint8_t[m_nal_unit_length];
      if (AM_UNLIKELY(!HEVC_con_array_nal.m_nal_unit)) {
        ERROR("Failed to malloc memory when get proper HEVC con array nal.");
        ret = false;
        break;
      }
      memcpy(HEVC_con_array_nal.m_nal_unit, m_nal_unit, m_nal_unit_length);
    }
  } while (0);
  return ret;
}

bool HEVCConArrayNal::copy_HEVC_con_array_nal(
    HEVCConArrayNal& HEVC_con_array_nal)
{
  bool ret = true;
  do {
    if (m_nal_unit_length == 0) {
      ERROR("The nal unit length is zero");
      ret = false;
      break;
    }
    HEVC_con_array_nal.m_nal_unit_length = m_nal_unit_length;
    if (HEVC_con_array_nal.m_nal_unit_length > 0) {
      delete[] HEVC_con_array_nal.m_nal_unit;
      HEVC_con_array_nal.m_nal_unit = new uint8_t[m_nal_unit_length];
      if (AM_UNLIKELY(!HEVC_con_array_nal.m_nal_unit)) {
        ERROR("Failed to malloc memory when get proper HEVC con array nal.");
        ret = false;
        break;
      }
      memcpy(HEVC_con_array_nal.m_nal_unit, m_nal_unit, m_nal_unit_length);
    }
  } while (0);
  return ret;
}

bool HEVCConArrayNal::get_combined_HEVC_con_array_nal(
    HEVCConArrayNal& source_nal, HEVCConArrayNal& combined_nal)
{
  bool ret = true;
  do {
    if (source_nal.m_nal_unit_length != m_nal_unit_length) {
      ERROR("The nal unit length is not same when get combined "
          "HEVC con array nal");
      ret = false;
      break;
    }
    if (memcmp(m_nal_unit, source_nal.m_nal_unit, m_nal_unit_length) != 0) {
      ERROR("The nal unit is not same when get combined HEVC con array nal.");
      ret = false;
      break;
    }
    combined_nal.m_nal_unit_length = m_nal_unit_length;
    if (combined_nal.m_nal_unit_length > 0) {
      delete[] combined_nal.m_nal_unit;
      combined_nal.m_nal_unit = new uint8_t[combined_nal.m_nal_unit_length];
      if (!combined_nal.m_nal_unit) {
        ERROR("Failed to malloc memory for nal unit.");
        ret = false;
        break;
      }
      memcpy(combined_nal.m_nal_unit, m_nal_unit, m_nal_unit_length);
    }
  } while (0);
  return ret;
}

uint32_t HEVCConArrayNal::get_HEVC_array_nal_size()
{
  return (2 + m_nal_unit_length);
}

HEVCConArray::HEVCConArray() :
    m_array_units(nullptr),
    m_num_nalus(0),
    m_array_completeness(0),
    m_reserved(0),
    m_NAL_unit_type(0)
{
}

HEVCConArray::~HEVCConArray()
{
  delete[] m_array_units;
}

bool HEVCConArray::write_HEVC_con_array(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_writer->write_be_u8((m_array_completeness << 7)
        | (m_reserved << 6) | (m_NAL_unit_type & 0x3f)))) {
      ERROR("Failed to write data when write HEVC con array.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_num_nalus))) {
      ERROR("Failed to write m_num_nalus when write HEVC con array.");
      ret = false;
      break;
    }
    if (m_num_nalus > 0) {
      for (uint16_t i = 0; i < m_num_nalus; ++ i) {
        if (AM_UNLIKELY(!m_array_units[i].write_HEVC_con_array_nal(
            file_writer))) {
          ERROR("Failed to write HEVC con array nal when write HEVC con array.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCConArray::read_HEVC_con_array(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint8_t data = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u8(data))) {
      ERROR("Failed to read data when read HEVC con array.");
      ret = false;
      break;
    }
    m_array_completeness = (data >> 7) & 0x01;
    m_reserved = 0;
    m_NAL_unit_type = data & 0x3f;
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_num_nalus))) {
      ERROR("Failed to read m_num_nalus when read HEVC con array.");
      ret = false;
      break;
    }
    if (m_num_nalus > 0) {
      delete[] m_array_units;
      m_array_units = new HEVCConArrayNal[m_num_nalus];
      if (AM_UNLIKELY(!m_array_units)) {
        ERROR("Failed to malloc memory when read HEVC con array.");
        ret = false;
        break;
      }
      for (uint16_t i = 0; i < m_num_nalus; ++ i) {
        if (AM_UNLIKELY(!m_array_units[i].read_HEVC_con_array_nal(file_reader))) {
          ERROR("Failed to read HEVC con array nal.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCConArray::get_proper_HEVC_con_array(HEVCConArray& HEVC_con_array,
                                             SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    HEVC_con_array.m_array_completeness = m_array_completeness;
    HEVC_con_array.m_NAL_unit_type = m_NAL_unit_type;
    HEVC_con_array.m_num_nalus = m_num_nalus;
    if (m_num_nalus > 0) {
      delete[] HEVC_con_array.m_array_units;
      HEVC_con_array.m_array_units = new HEVCConArrayNal[m_num_nalus];
      if (AM_UNLIKELY(!HEVC_con_array.m_array_units)) {
        ERROR("Failed to malloc memory when get proper HEVC con array.");
        ret = false;
        break;
      }
      for (uint16_t i = 0; i < m_num_nalus; ++ i) {
        if (AM_UNLIKELY(!m_array_units[i].get_proper_HEVC_con_array_nal(
            HEVC_con_array.m_array_units[i], sub_media_info))) {
          ERROR("Failed to get proper HEVC con array nal.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCConArray::copy_HEVC_con_array(HEVCConArray& HEVC_con_array)
{
  bool ret = true;
  do {
    HEVC_con_array.m_array_completeness = m_array_completeness;
    HEVC_con_array.m_NAL_unit_type = m_NAL_unit_type;
    HEVC_con_array.m_num_nalus = m_num_nalus;
    if (m_num_nalus > 0) {
      delete[] HEVC_con_array.m_array_units;
      HEVC_con_array.m_array_units = new HEVCConArrayNal[m_num_nalus];
      if (AM_UNLIKELY(!HEVC_con_array.m_array_units)) {
        ERROR("Failed to malloc memory when get proper HEVC con array.");
        ret = false;
        break;
      }
      for (uint16_t i = 0; i < m_num_nalus; ++ i) {
        if (AM_UNLIKELY(!m_array_units[i].copy_HEVC_con_array_nal(
            HEVC_con_array.m_array_units[i]))) {
          ERROR("Failed to get proper HEVC con array nal.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCConArray::get_combined_HEVC_con_array(HEVCConArray& source_array,
                                               HEVCConArray& combined_array)
{
  bool ret = true;
  do {
    if ((m_array_completeness != source_array.m_array_completeness)
     || (m_NAL_unit_type != source_array.m_NAL_unit_type)
     || (m_num_nalus != source_array.m_num_nalus)) {
      ERROR("The array completeness or nalu type or nalus number is not same "
            "when get combined HEVC con array.");
      ret = false;
      break;
    }
    combined_array.m_array_completeness = m_array_completeness;
    combined_array.m_reserved = 0;
    combined_array.m_NAL_unit_type = m_NAL_unit_type;
    combined_array.m_num_nalus = m_num_nalus;
    if (m_num_nalus > 0) {
      delete[] combined_array.m_array_units;
      combined_array.m_array_units = new HEVCConArrayNal[m_num_nalus];
      if (AM_UNLIKELY(!combined_array.m_array_units)) {
        ERROR("Failed to malloc memory when get combined HEVC con array.");
        ret = false;
        break;
      }
      for (uint16_t i = 0; i < m_num_nalus; ++ i) {
        if (AM_UNLIKELY(!m_array_units[i].get_combined_HEVC_con_array_nal(
            source_array.m_array_units[i], combined_array.m_array_units[i]))) {
          ERROR("Failed to get combined HEVC con array nal");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

uint32_t HEVCConArray::get_HEVC_con_array_size()
{
  uint32_t size = 0;
  size = 1 + 2;
  for (uint16_t i = 0; i < m_num_nalus; ++ i) {
    size += m_array_units[i].get_HEVC_array_nal_size();
  }
  return size;
}

HEVCDecoderConfigurationRecord::HEVCDecoderConfigurationRecord() :
    m_HEVC_config_array(nullptr),
    m_general_profile_compatibility_flags(0),
    m_general_constraint_indicator_flags_low(0),
    m_general_constraint_indicator_flags_high(0),
    m_min_spatial_segmentation_idc(0),
    m_avg_frame_rate(0),
    m_configuration_version(0),
    m_general_profile_space(0),
    m_general_tier_flag(0),
    m_general_profile_idc(0),
    m_general_level_idc(0),
    m_parallelism_type(0),
    m_chroma_format(0),
    m_bit_depth_luma_minus8(0),
    m_bit_depth_chroma_minus8(0),
    m_constant_frame_rate(0),
    m_num_temporal_layers(0),
    m_temporal_id_nested(0),
    m_length_size_minus_one(0),
    m_num_of_arrays(0)
{
}

HEVCDecoderConfigurationRecord::~HEVCDecoderConfigurationRecord()
{
  delete[] m_HEVC_config_array;
}

bool HEVCDecoderConfigurationRecord::write_HEVC_decoder_configuration_record(
                                                   Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The HEVC decoder configuration record is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write base box when write HEVC decoder configuration "
          "record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_configuration_version))) {
      ERROR("Failed to write configuration version when write HEVC "
          "decoder record");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(
        (((m_general_profile_space << 6)) & 0xc0) |
        ((m_general_tier_flag << 5) & 0x20) |
        (m_general_profile_idc & 0x1f)))) {
      ERROR("Failed to write data when write HEVC decoder configuration record");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(
        m_general_profile_compatibility_flags))) {
      ERROR("Failed to write m_general_profile_compatibility_flags "
            "when write HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(
        m_general_constraint_indicator_flags_high))) {
      ERROR("Failed to write m_general_constraint_indicator_flags_high"
            "when write HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(
        m_general_constraint_indicator_flags_low))) {
      ERROR("Failed to write m_general_constraint_indicator_flags_low"
            "when write HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_general_level_idc))) {
      ERROR("Failed to write general level idc when write HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_min_spatial_segmentation_idc
        | 0xf000))) {
      ERROR("Failed to write m_min_spatial_segmentation_idc when write"
            "HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_parallelism_type | 0xfc))) {
      ERROR("Failed to write m_parallelism_type when write HEVC decoder"
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_chroma_format | 0xfc))) {
      ERROR("Failed to write chroma_foramt when write HEVC decoder"
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_bit_depth_luma_minus8
        | 0xf8))) {
      ERROR("Failed to write m_bit_depth_luma_minus8 when write HEVC"
            "decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_bit_depth_chroma_minus8
        | 0xf8))) {
      ERROR("Failed to write m_bit_depth_chroma_minus8 when write HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_avg_frame_rate))) {
      ERROR("Failed to write m_avg_frame_rate when write HEVC decoder"
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(
        ((m_constant_frame_rate << 6) & 0xc0) |
        ((m_num_temporal_layers << 3) & 0x38) |
        ((m_temporal_id_nested << 2) & 0x04) |
        (m_length_size_minus_one & 0x03)))) {
      ERROR("Failed to write data when write HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_num_of_arrays))) {
      ERROR("Failed to write num of arrays when write HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    if (m_num_of_arrays > 0) {
      for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
        if (AM_UNLIKELY(!m_HEVC_config_array[i].write_HEVC_con_array(
            file_writer))) {
          ERROR("Failed to write HEVC config array.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCDecoderConfigurationRecord::read_HEVC_decoder_configuration_record(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read visual sample entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read HEVC decoder "
          "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_configuration_version))) {
      ERROR("Failed to read m_configuration_version when read HEVC decoder "
            "configuration  record.");
      ret = false;
      break;
    }
    uint8_t data_u8 = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u8(data_u8))) {
      ERROR("Failed to read data when read HEVC decoder configuration record.");
      ret = false;
      break;
    }
    m_general_profile_space = (data_u8 >> 6) & 0x03;
    m_general_tier_flag = (data_u8 >> 5) & 0x01;
    m_general_profile_idc = data_u8 & 0x1f;
    if (AM_UNLIKELY(!file_reader->read_le_u32(
        m_general_profile_compatibility_flags))) {
      ERROR("Failed to read m_general_profile_compatibility_flags when read "
            "HEVC decoder configuration  record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(
        m_general_constraint_indicator_flags_high))) {
      ERROR("Failed to read m_general_constraint_indicator_flags_high"
            "when read HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(
        m_general_constraint_indicator_flags_low))) {
      ERROR("Failed to read m_general_constraint_indicator_flags_low when read"
            "HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_general_level_idc))) {
      ERROR("Failed to read m_general_level_idc when read HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_min_spatial_segmentation_idc))) {
      ERROR("Failed to read m_min_spatial_segmentation_idc when read HEVC "
          "decoder configuration record.");
      ret = false;
      break;
    }
    m_min_spatial_segmentation_idc = m_min_spatial_segmentation_idc & 0x0fff;
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_parallelism_type))) {
      ERROR("Failed to read m_parallelism_type when read HEVC decoder"
            "configuration record.");
      ret = false;
      break;
    }
    m_parallelism_type = m_parallelism_type & 0x03;
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_chroma_format))) {
      ERROR("Failed to read m_chroma_format when read HEVC decoder configuration"
            "record.");
      ret = false;
      break;
    }
    m_chroma_format = m_chroma_format & 0x03;
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_bit_depth_luma_minus8))) {
      ERROR("Failed to read m_bit_depth_luma_minus8 when read HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    m_bit_depth_luma_minus8 = m_bit_depth_luma_minus8 & 0x07;
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_bit_depth_chroma_minus8))) {
      ERROR("Failed to read m_bit_depth_chroma_minus8 when read HEVC"
            "decoder configuration record.");
      ret = false;
      break;
    }
    m_bit_depth_chroma_minus8 = m_bit_depth_chroma_minus8 & 0x07;
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_avg_frame_rate))) {
      ERROR("Failed to read m_avg_frame_rate when read HEVC decoder "
            "configuration record.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u8(data_u8))) {
      ERROR("Failed to read data when read HEVC decoder configuration record.");
      ret = false;
      break;
    }
    m_constant_frame_rate = (data_u8 >> 6) & 0x03;
    m_num_temporal_layers = (data_u8 >> 3) & 0x07;
    m_temporal_id_nested = (data_u8 >> 2) & 0x01;
    m_length_size_minus_one = (data_u8) & 0x03;
    if (AM_UNLIKELY(!file_reader->read_le_u8(m_num_of_arrays))) {
      ERROR("Failed to read num of arrays when read HEVC decoder configuration"
          " record");
      ret = false;
      break;
    }
    if (m_num_of_arrays > 0) {
      delete[] m_HEVC_config_array;
      m_HEVC_config_array = new HEVCConArray[m_num_of_arrays];
      if (!m_HEVC_config_array) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
        if (AM_UNLIKELY(!m_HEVC_config_array[i].read_HEVC_con_array(
            file_reader))) {
          ERROR("Failed to read HEVC con array when read HEVC decoder "
              "configuration record.");
          ret = false;
          break;
        }
      }
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read HEVC decoder configuration record.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read HEVC decoder configuration record error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool HEVCDecoderConfigurationRecord::get_proper_HEVC_decoder_configuration_record(
    HEVCDecoderConfigurationRecord& box, SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    box.m_configuration_version = 1;
    box.m_general_profile_space = m_general_profile_space;
    box.m_general_tier_flag = m_general_tier_flag;
    box.m_general_profile_idc = m_general_profile_idc;
    box.m_general_profile_compatibility_flags =
        m_general_profile_compatibility_flags;
    box.m_general_constraint_indicator_flags_high =
        m_general_constraint_indicator_flags_high;
    box.m_general_constraint_indicator_flags_low =
        m_general_constraint_indicator_flags_low;
    box.m_general_level_idc = m_general_level_idc;
    box.m_min_spatial_segmentation_idc =
        m_min_spatial_segmentation_idc;
    box.m_parallelism_type = m_parallelism_type;
    box.m_chroma_format = m_chroma_format;
    box.m_bit_depth_luma_minus8 = m_bit_depth_luma_minus8;
    box.m_bit_depth_chroma_minus8 = m_bit_depth_chroma_minus8;
    box.m_avg_frame_rate = m_avg_frame_rate;
    box.m_constant_frame_rate = m_constant_frame_rate;
    box.m_num_temporal_layers = m_num_temporal_layers;
    box.m_temporal_id_nested = m_temporal_id_nested;
    box.m_length_size_minus_one = m_length_size_minus_one;
    box.m_num_of_arrays = m_num_of_arrays;
    if (m_num_of_arrays > 0) {
      delete[] box.m_HEVC_config_array;
      box.m_HEVC_config_array = new HEVCConArray[m_num_of_arrays];
      if (AM_UNLIKELY(!box.m_HEVC_config_array)) {
        ERROR("Failed to malloc memory when get proper HEVC decoder configuration record.");
        ret = false;
        break;
      }
      for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
        if (AM_UNLIKELY(!m_HEVC_config_array[i].get_proper_HEVC_con_array(
            box.m_HEVC_config_array[i], sub_media_info))) {
          ERROR("Failed to get proper HEVC conf array.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCDecoderConfigurationRecord::copy_HEVC_decoder_configuration_record(
    HEVCDecoderConfigurationRecord& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    box.m_configuration_version = 1;
    box.m_general_profile_space = m_general_profile_space;
    box.m_general_tier_flag = m_general_tier_flag;
    box.m_general_profile_idc = m_general_profile_idc;
    box.m_general_profile_compatibility_flags =
        m_general_profile_compatibility_flags;
    box.m_general_constraint_indicator_flags_high =
        m_general_constraint_indicator_flags_high;
    box.m_general_constraint_indicator_flags_low =
        m_general_constraint_indicator_flags_low;
    box.m_general_level_idc = m_general_level_idc;
    box.m_min_spatial_segmentation_idc =
        m_min_spatial_segmentation_idc;
    box.m_parallelism_type = m_parallelism_type;
    box.m_chroma_format = m_chroma_format;
    box.m_bit_depth_luma_minus8 = m_bit_depth_luma_minus8;
    box.m_bit_depth_chroma_minus8 = m_bit_depth_chroma_minus8;
    box.m_avg_frame_rate = m_avg_frame_rate;
    box.m_constant_frame_rate = m_constant_frame_rate;
    box.m_num_temporal_layers = m_num_temporal_layers;
    box.m_temporal_id_nested = m_temporal_id_nested;
    box.m_length_size_minus_one = m_length_size_minus_one;
    box.m_num_of_arrays = m_num_of_arrays;
    if (m_num_of_arrays > 0) {
      delete[] box.m_HEVC_config_array;
      box.m_HEVC_config_array = new HEVCConArray[m_num_of_arrays];
      if (AM_UNLIKELY(!box.m_HEVC_config_array)) {
        ERROR("Failed to malloc memory when copy HEVC decoder "
            "configuration record.");
        ret = false;
        break;
      }
      for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
        if (AM_UNLIKELY(!m_HEVC_config_array[i].copy_HEVC_con_array(
            box.m_HEVC_config_array[i]))) {
          ERROR("Failed to copy HEVC conf array.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool HEVCDecoderConfigurationRecord::
    get_combined_HEVC_decoder_configuration_record(
    HEVCDecoderConfigurationRecord& source_box,
    HEVCDecoderConfigurationRecord& combined_box)
{
  bool ret = true;
  do {
    if ((!source_box.m_base_box.m_enable) || (!m_base_box.m_enable)) {
      ERROR("One of HEVC decoder configuration record is not enabled.");
      ret = false;
      break;
    }
    if ((m_base_box.m_size != source_box.m_base_box.m_size)
        || (m_general_profile_space != source_box.m_general_profile_space)
        || (m_general_tier_flag != source_box.m_general_tier_flag)
        || (m_general_profile_idc != source_box.m_general_profile_idc)
        || (m_general_profile_compatibility_flags
            != source_box.m_general_profile_compatibility_flags)
        || (m_general_constraint_indicator_flags_high
            != source_box.m_general_constraint_indicator_flags_high)
        || (m_general_constraint_indicator_flags_low
            != source_box.m_general_constraint_indicator_flags_low)
        || (m_general_level_idc != source_box.m_general_level_idc)
        || (m_min_spatial_segmentation_idc
            != source_box.m_min_spatial_segmentation_idc)
        || (m_parallelism_type != source_box.m_parallelism_type)
        || (m_chroma_format != source_box.m_chroma_format)
        || (m_bit_depth_luma_minus8 != source_box.m_bit_depth_luma_minus8)
        || (m_bit_depth_chroma_minus8 != source_box.m_bit_depth_chroma_minus8)
        || (m_avg_frame_rate != source_box.m_avg_frame_rate)
        || (m_constant_frame_rate != source_box.m_constant_frame_rate)
        || (m_num_temporal_layers != source_box.m_num_temporal_layers)
        || (m_temporal_id_nested != source_box.m_temporal_id_nested)
        || (m_length_size_minus_one != source_box.m_length_size_minus_one)
        || (m_num_of_arrays != source_box.m_num_of_arrays)) {
      ERROR("The parameter is not same.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    combined_box.m_configuration_version = 1;
    combined_box.m_general_profile_space = m_general_profile_space;
    combined_box.m_general_tier_flag = m_general_tier_flag;
    combined_box.m_general_profile_idc = m_general_profile_idc;
    combined_box.m_general_profile_compatibility_flags =
        m_general_profile_compatibility_flags;
    combined_box.m_general_constraint_indicator_flags_high =
        m_general_constraint_indicator_flags_high;
    combined_box.m_general_constraint_indicator_flags_low =
        m_general_constraint_indicator_flags_low;
    combined_box.m_general_level_idc = m_general_level_idc;
    combined_box.m_min_spatial_segmentation_idc =
        m_min_spatial_segmentation_idc;
    combined_box.m_parallelism_type = m_parallelism_type;
    combined_box.m_chroma_format = m_chroma_format;
    combined_box.m_bit_depth_luma_minus8 = m_bit_depth_luma_minus8;
    combined_box.m_bit_depth_chroma_minus8 = m_bit_depth_chroma_minus8;
    combined_box.m_avg_frame_rate = m_avg_frame_rate;
    combined_box.m_constant_frame_rate = m_constant_frame_rate;
    combined_box.m_num_temporal_layers = m_num_temporal_layers;
    combined_box.m_temporal_id_nested = m_temporal_id_nested;
    combined_box.m_length_size_minus_one = m_length_size_minus_one;
    combined_box.m_num_of_arrays = m_num_of_arrays;
    if (m_num_of_arrays > 0) {
      delete[] combined_box.m_HEVC_config_array;
      combined_box.m_HEVC_config_array = new HEVCConArray[m_num_of_arrays];
      if (!combined_box.m_HEVC_config_array) {
        ERROR("Failed to malloc memory when get combined HEVC decoder "
              "configuration record.");
        ret = false;
        break;
      }
      for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
        if (AM_UNLIKELY(!m_HEVC_config_array[i].get_combined_HEVC_con_array(
            source_box.m_HEVC_config_array[i],
            combined_box.m_HEVC_config_array[i]))) {
          ERROR("Failed to get combined HEVC con array.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

void HEVCDecoderConfigurationRecord::update_HEVC_decoder_configuration_record_size()
{
  m_base_box.m_size = DISOM_HEVC_DECODER_CONFIG_RECORD_SIZE;
  for (uint8_t i = 0; i < m_num_of_arrays; ++ i) {
    m_base_box.m_size += m_HEVC_config_array[i].get_HEVC_con_array_size();
  }
}

VisualSampleEntry::VisualSampleEntry() :
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

bool VisualSampleEntry::write_visual_sample_entry(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The visual sample entry is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
                 || (!file_writer->write_data(m_reserved_0, 6 * sizeof(uint8_t)))
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
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_width))
                 || (!file_writer->write_be_u16(m_height))
                 || (!file_writer->write_be_u32(m_horizresolution))
                 || (!file_writer->write_be_u32(m_vertresolution))
                 || (!file_writer->write_be_u32(m_reserved_1))
                 || (!file_writer->write_be_u16(m_frame_count))
                 || (!file_writer->write_data((uint8_t* )m_compressorname,
                                              32 * sizeof(char)))
                 || (!file_writer->write_be_u16(m_depth))
                 || (!file_writer->write_be_s16(m_pre_defined_2)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    if (m_avc_config.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_avc_config.write_avc_decoder_configuration_record_box(
          file_writer))) {
        ERROR("Failed to write visual sample description box.");
        ret = false;
        break;
      }
    }
    if (m_HEVC_config.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_HEVC_config.write_HEVC_decoder_configuration_record(
          file_writer))) {
        ERROR("Failed to write HEVC decoder configuration record.");
        ret = false;
        break;
      }
    }
    INFO("write visual sample entry success.");
  } while (0);
  return ret;
}

bool VisualSampleEntry::read_visual_sample_entry(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read visual sample entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
                 || (!file_reader->read_data(m_reserved_0, sizeof(uint8_t) * 6))
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_width))
        || (!file_reader->read_le_u16(m_height))
        || (!file_reader->read_le_u32(m_horizresolution))
        || (!file_reader->read_le_u32(m_vertresolution))
        || (!file_reader->read_le_u32(m_reserved_1))
        || (!file_reader->read_le_u16(m_frame_count))
        || (!file_reader->read_data((uint8_t* )m_compressorname,
                                    32 * sizeof(char)))
        || (!file_reader->read_le_u16(m_depth))
        || (!file_reader->read_le_s16(m_pre_defined_2)))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
    switch (type) {
      case DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG: {
        if (AM_UNLIKELY(!m_avc_config.read_avc_decoder_configuration_record_box(
            file_reader))) {
          ERROR("Failed to read visual sample description box.");
          ret = false;
        }
      } break;
      case DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG: {
        if (AM_UNLIKELY(!m_HEVC_config.read_HEVC_decoder_configuration_record(
            file_reader))) {
          ERROR("Failed to read HEVC decoder configuration record.");
          ret = false;
        }
      } break;
      default: {
        ERROR("Get invalid type when read avc decoder configuration record box.");
        ret = false;
      } break;
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read visual sample entry.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read visual sample entry error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read visual sample entry success.");
  } while (0);
  return ret;
}

bool VisualSampleEntry::get_proper_visual_sample_entry(
    VisualSampleEntry& entry, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    if (m_avc_config.m_base_box.m_enable) {
      if (!m_avc_config.get_proper_avc_decoder_configuration_record_box(
          entry.m_avc_config, media_info)) {
        ERROR("Failed to get proper avc decoder configuration record box.");
        ret = false;
        break;
      }
    }
    if (m_HEVC_config.m_base_box.m_enable) {
      if (!m_HEVC_config.get_proper_HEVC_decoder_configuration_record(
          entry.m_HEVC_config, media_info)) {
        ERROR("Failed to get proper HEVC decoder configuration record.");
        ret = false;
        break;
      }
    }
    entry.m_horizresolution = m_horizresolution;
    entry.m_vertresolution = m_vertresolution;
    entry.m_reserved_1 = m_reserved_1;
    memcpy((uint8_t*) (entry.m_pre_defined_1), (uint8_t*) (m_pre_defined_1),
           3 * sizeof(uint32_t));
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_pre_defined = m_pre_defined;
    entry.m_reserved = m_reserved;
    entry.m_width = m_width;
    entry.m_height = m_height;
    entry.m_frame_count = m_frame_count;
    entry.m_depth = m_depth;
    entry.m_pre_defined_2 = m_pre_defined_2;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
    memcpy(entry.m_compressorname, m_compressorname, 32 * sizeof(char));
  } while (0);
  return ret;
}

bool VisualSampleEntry::copy_visual_sample_entry(VisualSampleEntry& entry)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    if (m_avc_config.m_base_box.m_enable) {
      if (!m_avc_config.copy_avc_decoder_configuration_record_box(
          entry.m_avc_config)) {
        ERROR("Failed to copy avc decoder configuration record box.");
        ret = false;
        break;
      }
    }
    if (m_HEVC_config.m_base_box.m_enable) {
      if (!m_HEVC_config.copy_HEVC_decoder_configuration_record(
          entry.m_HEVC_config)) {
        ERROR("Failed to copy HEVC decoder configuration record.");
        ret = false;
        break;
      }
    }
    entry.m_horizresolution = m_horizresolution;
    entry.m_vertresolution = m_vertresolution;
    entry.m_reserved_1 = m_reserved_1;
    memcpy((uint8_t*) (entry.m_pre_defined_1), (uint8_t*) (m_pre_defined_1),
           3 * sizeof(uint32_t));
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_pre_defined = m_pre_defined;
    entry.m_reserved = m_reserved;
    entry.m_width = m_width;
    entry.m_height = m_height;
    entry.m_frame_count = m_frame_count;
    entry.m_depth = m_depth;
    entry.m_pre_defined_2 = m_pre_defined_2;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
    memcpy(entry.m_compressorname, m_compressorname, 32 * sizeof(char));
  } while (0);
  return ret;
}

bool VisualSampleEntry::get_combined_visual_sample_entry(
    VisualSampleEntry& source_entry, VisualSampleEntry& combined_entry)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_entry.m_base_box.m_enable)) {
      ERROR("One of combined visual sample entry is not enabled.");
      ret = false;
      break;
    }
    if (m_avc_config.m_base_box.m_enable) {
      if (!m_avc_config.get_combined_avc_decoder_configuration_record_box(
          source_entry.m_avc_config, combined_entry.m_avc_config)) {
        ERROR("Get combined avc decoder configuration record box error.");
        ret = false;
        break;
      }
    } else if (m_HEVC_config.m_base_box.m_enable) {
      if (!m_HEVC_config.get_combined_HEVC_decoder_configuration_record(
          source_entry.m_HEVC_config, combined_entry.m_HEVC_config)) {
        ERROR("Failed to get combined HEVC decoder configuration record.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video type is not same");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_entry.m_base_box);
    if ((source_entry.m_width != m_width)
     || (source_entry.m_height != m_height)) {
      ERROR("The width or height is not same.");
      ret = false;
      break;
    }
    combined_entry.m_horizresolution = m_horizresolution;
    combined_entry.m_vertresolution = m_vertresolution;
    combined_entry.m_reserved_1 = m_reserved_1;
    memcpy((uint8_t*) combined_entry.m_pre_defined_1, (uint8_t*) m_pre_defined_1,
           sizeof(uint32_t) * 3);
    combined_entry.m_data_reference_index = m_data_reference_index;
    combined_entry.m_pre_defined = m_pre_defined;
    combined_entry.m_reserved = m_reserved;
    combined_entry.m_width = m_width;
    combined_entry.m_height = m_height;
    combined_entry.m_frame_count = m_frame_count;
    combined_entry.m_depth = m_depth;
    combined_entry.m_pre_defined_2 = m_pre_defined_2;
    memcpy(combined_entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
    memcpy(combined_entry.m_compressorname, m_compressorname, 32 * sizeof(char));
  } while (0);
  return ret;
}

void VisualSampleEntry::update_visual_sample_entry_size()
{
  m_base_box.m_size = VISUAL_SAMPLE_ENTRY_SIZE;
  if (m_avc_config.m_base_box.m_enable) {
    m_base_box.m_size += m_avc_config.m_base_box.m_size;
  }
  if (m_HEVC_config.m_base_box.m_enable) {
    m_HEVC_config.update_HEVC_decoder_configuration_record_size();
    m_base_box.m_size += m_HEVC_config.m_base_box.m_size;
  }
}

AACDescriptorBox::AACDescriptorBox() :
    m_buffer_size(0),
    m_max_bitrate(0),
    m_avg_bitrate(0),
    m_es_id(0),
    m_audio_spec_config(0),
    m_es_descriptor_type_tag(0),
    m_es_descriptor_type_length(0),
    m_stream_priority(0),
    m_decoder_config_descriptor_type_tag(0),
    m_decoder_descriptor_type_length(0),
    m_object_type(0),
    m_stream_flag(0),
    m_decoder_specific_descriptor_type_tag(0),
    m_decoder_specific_descriptor_type_length(0),
    m_SL_descriptor_type_tag(0),
    m_SL_descriptor_type_length(0),
    m_SL_value(0)
{
}

bool AACDescriptorBox::write_aac_descriptor_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The aac elementary stream description box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
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
    if (AM_UNLIKELY((!file_writer->write_be_u8(
                      m_decoder_config_descriptor_type_tag))
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
    if (AM_UNLIKELY((!file_writer->write_be_u8(
        m_decoder_specific_descriptor_type_tag))
        || (!file_writer->write_be_u16(0x8080))
        || (!file_writer->write_be_u8(m_decoder_specific_descriptor_type_length))
        || (!file_writer->write_be_u16(m_audio_spec_config))
        || (!file_writer->write_be_u16(0)) || (!file_writer->write_be_u8(0)))) {
      ERROR("Failed to write decoder specific info descriptor");
      ret = false;
      break;
    }
    /*SL descriptor takes 5 bytes*/
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_SL_descriptor_type_tag))
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

bool AACDescriptorBox::read_aac_descriptor_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read aac elementary stream description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
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
    if (AM_UNLIKELY((!file_reader->read_le_u8(
        m_decoder_config_descriptor_type_tag))
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
    if (AM_UNLIKELY((!file_reader->read_le_u8(
        m_decoder_specific_descriptor_type_tag))
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
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_SL_descriptor_type_tag))
        || (!file_reader->read_le_u16(temp16))
        || (!file_reader->read_le_u8(m_SL_descriptor_type_length))
        || (!file_reader->read_le_u8(m_SL_value)))) {
      ERROR("Failed to read aac elementary stream description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read aac elementary "
          "stream description box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read aac elementary stream description box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read aac elementary stream description box.");
  } while (0);
  return ret;
}

bool AACDescriptorBox::get_proper_aac_descriptor_box(
    AACDescriptorBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_es_id = m_es_id;
    box.m_es_descriptor_type_tag = m_es_descriptor_type_tag;
    box.m_es_descriptor_type_length = m_es_descriptor_type_length;
    box.m_stream_priority = m_stream_priority;
    box.m_buffer_size = m_buffer_size;
    box.m_max_bitrate = m_max_bitrate;
    box.m_avg_bitrate = m_avg_bitrate;
    box.m_decoder_config_descriptor_type_tag =
        m_decoder_config_descriptor_type_tag;
    box.m_decoder_descriptor_type_length =
        m_decoder_descriptor_type_length;
    box.m_object_type = m_object_type;
    box.m_stream_flag = m_stream_flag;
    box.m_audio_spec_config = m_audio_spec_config;
    box.m_decoder_specific_descriptor_type_tag =
        m_decoder_specific_descriptor_type_tag;
    box.m_decoder_specific_descriptor_type_length =
        m_decoder_specific_descriptor_type_length;
    box.m_SL_descriptor_type_tag = m_SL_descriptor_type_tag;
    box.m_SL_descriptor_type_length =
        m_SL_descriptor_type_length;
    box.m_SL_value = m_SL_value;
  } while (0);
  return ret;
}

bool AACDescriptorBox::copy_aac_descriptor_box(
    AACDescriptorBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_es_id = m_es_id;
    box.m_es_descriptor_type_tag = m_es_descriptor_type_tag;
    box.m_es_descriptor_type_length = m_es_descriptor_type_length;
    box.m_stream_priority = m_stream_priority;
    box.m_buffer_size = m_buffer_size;
    box.m_max_bitrate = m_max_bitrate;
    box.m_avg_bitrate = m_avg_bitrate;
    box.m_decoder_config_descriptor_type_tag =
        m_decoder_config_descriptor_type_tag;
    box.m_decoder_descriptor_type_length =
        m_decoder_descriptor_type_length;
    box.m_object_type = m_object_type;
    box.m_stream_flag = m_stream_flag;
    box.m_audio_spec_config = m_audio_spec_config;
    box.m_decoder_specific_descriptor_type_tag =
        m_decoder_specific_descriptor_type_tag;
    box.m_decoder_specific_descriptor_type_length =
        m_decoder_specific_descriptor_type_length;
    box.m_SL_descriptor_type_tag = m_SL_descriptor_type_tag;
    box.m_SL_descriptor_type_length =
        m_SL_descriptor_type_length;
    box.m_SL_value = m_SL_value;
  } while (0);
  return ret;
}

bool AACDescriptorBox::get_combined_aac_descriptor_box(
    AACDescriptorBox& source_box,
    AACDescriptorBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined aac elementary stream description box is"
          " not enabled.");
      ret = false;
      break;
    }
    if (source_box.m_audio_spec_config != m_audio_spec_config) {
      ERROR("The aac spec config is not same");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_es_id = m_es_id;
    combined_box.m_es_descriptor_type_tag = m_es_descriptor_type_tag;
    combined_box.m_es_descriptor_type_length = m_es_descriptor_type_length;
    combined_box.m_stream_priority = m_stream_priority;
    combined_box.m_buffer_size = m_buffer_size;
    combined_box.m_max_bitrate = m_max_bitrate;
    combined_box.m_avg_bitrate = m_avg_bitrate;
    combined_box.m_decoder_config_descriptor_type_tag =
        m_decoder_config_descriptor_type_tag;
    combined_box.m_decoder_descriptor_type_length =
        m_decoder_descriptor_type_length;
    combined_box.m_object_type = m_object_type;
    combined_box.m_stream_flag = m_stream_flag;
    combined_box.m_audio_spec_config = m_audio_spec_config;
    combined_box.m_decoder_specific_descriptor_type_tag =
        m_decoder_specific_descriptor_type_tag;
    combined_box.m_decoder_specific_descriptor_type_length =
        m_decoder_specific_descriptor_type_length;
    combined_box.m_SL_descriptor_type_tag = m_SL_descriptor_type_tag;
    combined_box.m_SL_descriptor_type_length = m_SL_descriptor_type_length;
    combined_box.m_SL_value = m_SL_value;
  } while (0);
  return ret;
}

OpusSpecificBox::OpusSpecificBox() :
    m_channel_mapping(nullptr),
    m_input_sample_rate(0),
    m_pre_skip(0),
    m_output_gain(0),
    m_version(0),
    m_output_channel_count(0),
    m_channel_mapping_family(0),
    m_stream_count(0),
    m_couple_count(0)
{

}

OpusSpecificBox::~OpusSpecificBox()
{
  delete[] m_channel_mapping;
}

bool OpusSpecificBox::write_opus_specific_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The opus specific box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer))) {
      ERROR("Failed to write full box when write opus specific box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_output_channel_count))) {
      ERROR("Failed to write output channel count when write opus specific box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_pre_skip))) {
      ERROR("Failed to write pre skip when write opus specific box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_input_sample_rate))) {
      ERROR("Failed to write input sample rate when write opus specific box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_s16(m_output_gain))) {
      ERROR("Failed to write output gain.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_channel_mapping_family))) {
      ERROR("Failed to write channel mapping family when write opus specific box.");
      ret = false;
      break;
    }
    if (m_channel_mapping_family != 0) {
      if (AM_UNLIKELY(!file_writer->write_be_u8(m_stream_count))) {
        ERROR("Failed to write stream count when write opus specific box.");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_writer->write_be_u8(m_couple_count))) {
        ERROR("Failed to write couple count when write opus specific box.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_output_channel_count; ++ i) {
        if (AM_UNLIKELY(!file_writer->write_be_u8(m_channel_mapping[i]))) {
          ERROR("Failed to write channel mapping when write opus specific box.");
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
  } while (0);
  return ret;
}

bool OpusSpecificBox::read_opus_specific_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read opus specific box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read opus specific box.");
      ret = false;
      break;
    }
    if ((!file_reader->read_le_u8(m_output_channel_count))
     || (!file_reader->read_le_u16(m_pre_skip))
     || (!file_reader->read_le_u32(m_input_sample_rate))
     || (!file_reader->read_le_s16(m_output_gain))
     || (!file_reader->read_le_u8(m_channel_mapping_family))) {
      ERROR("Failed to read data when read opus specific box");
      ret = false;
      break;
    }
    if (m_channel_mapping_family != 0) {
      if ((!file_reader->read_le_u8(m_stream_count))
       || (!file_reader->read_le_u8(m_couple_count))) {
        ERROR("Failed to read data when read opus specific box.");
        ret = false;
        break;
      }
      if (m_output_channel_count > 0) {
        delete[] m_channel_mapping;
        m_channel_mapping = new uint8_t[m_output_channel_count];
        if (!m_channel_mapping) {
          ERROR("Failed to malloc memory when read opus specific box.");
          ret = false;
          break;
        }
        for (uint8_t i = 0; i < m_output_channel_count; ++ i) {
          if (!file_reader->read_le_u8(m_channel_mapping[i])) {
            ERROR("Failed to read data when read opus specific box.");
            ret = false;
            break;
          }
        }
        if (!ret) {
          break;
        }
      }
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read opus specific box");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read opus specific box error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool OpusSpecificBox::get_proper_opus_specific_box(OpusSpecificBox& box,
                                                   SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    box.m_output_channel_count = m_output_channel_count;
    box.m_pre_skip = m_pre_skip;
    box.m_input_sample_rate = m_input_sample_rate;
    box.m_output_gain = m_output_gain;
    box.m_channel_mapping_family = m_channel_mapping_family;
    box.m_stream_count = m_stream_count;
    box.m_couple_count = m_couple_count;
    if ((m_channel_mapping_family != 0) && (m_output_channel_count > 0)) {
      delete[] box.m_channel_mapping;
      box.m_channel_mapping = new uint8_t[m_output_channel_count];
      if (!box.m_channel_mapping) {
        ERROR("Failed to malloc memory when get proper opus specific box.");
        ret = false;
        break;
      }
      memcpy(box.m_channel_mapping, m_channel_mapping,
             m_output_channel_count * sizeof(uint8_t));
    }
  } while (0);
  return ret;
}

bool OpusSpecificBox::copy_opus_specific_box(OpusSpecificBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    box.m_output_channel_count = m_output_channel_count;
    box.m_pre_skip = m_pre_skip;
    box.m_input_sample_rate = m_input_sample_rate;
    box.m_output_gain = m_output_gain;
    box.m_channel_mapping_family = m_channel_mapping_family;
    box.m_stream_count = m_stream_count;
    box.m_couple_count = m_couple_count;
    if ((m_channel_mapping_family != 0) && (m_output_channel_count > 0)) {
      delete[] box.m_channel_mapping;
      box.m_channel_mapping = new uint8_t[m_output_channel_count];
      if (!box.m_channel_mapping) {
        ERROR("Failed to malloc memory when get proper opus specific box.");
        ret = false;
        break;
      }
      memcpy(box.m_channel_mapping, m_channel_mapping,
             m_output_channel_count * sizeof(uint8_t));
    }
  } while (0);
  return ret;
}

bool OpusSpecificBox::get_combined_opus_specific_box(
    OpusSpecificBox& source_box, OpusSpecificBox& combined_box)
{
  bool ret = true;
  do {
    if (m_base_box.m_size != source_box.m_base_box.m_size) {
      ERROR("The size of opus specific box is not same.");
      ret = false;
      break;
    }
    if ((m_output_channel_count != source_box.m_output_channel_count)
     || (m_pre_skip != source_box.m_pre_skip)
     || (m_input_sample_rate != source_box.m_input_sample_rate)
     || (m_output_gain != source_box.m_output_gain)
     || (m_channel_mapping_family != source_box.m_channel_mapping_family)) {
      ERROR("The parameter of opus specific box is not same with source box.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    combined_box.m_output_channel_count = m_output_channel_count;
    combined_box.m_pre_skip = m_pre_skip;
    combined_box.m_input_sample_rate = m_input_sample_rate;
    combined_box.m_output_gain = m_output_gain;
    combined_box.m_channel_mapping_family = m_channel_mapping_family;
    if (m_channel_mapping_family != 0) {
      if ((m_stream_count != source_box.m_stream_count)
       || (m_couple_count != source_box.m_couple_count)) {
        ERROR("The parameter of opus specific box is not same with source box.");
        ret = false;
        break;
      }
      combined_box.m_stream_count = m_stream_count;
      combined_box.m_couple_count = m_couple_count;
      if (m_output_channel_count > 0) {
        if (memcmp(m_channel_mapping,
                   source_box.m_channel_mapping,
                   m_output_channel_count) != 0) {
          ERROR("The channel mapping is not same with source opus specific box");
          ret = false;
          break;
        }
        delete[] combined_box.m_channel_mapping;
        combined_box.m_channel_mapping = new uint8_t[m_output_channel_count];
        if (!combined_box.m_channel_mapping) {
          ERROR("Failed to malloc memory when get combined opus specific box");
          ret = false;
          break;
        }
        memcpy(combined_box.m_channel_mapping, source_box.m_channel_mapping,
               m_output_channel_count);
      }
    }
  } while (0);
  return ret;
}
void OpusSpecificBox::update_opus_specific_box_size()
{
  m_base_box.m_size = OPUS_SPECIFIC_BOX_SIZE;
  if (m_channel_mapping_family) {
    m_base_box.m_size += (2 * sizeof(uint8_t) +
        sizeof(uint8_t) * m_output_channel_count);
  }
}

AudioSampleEntry::AudioSampleEntry() :
    m_sample_rate_integer(0),
    m_sample_rate_decimal(0),
    m_data_reference_index(0),
    m_channelcount(0),
    m_sample_size(0),
    m_pre_defined(0),
    m_reserved_1(0)
{
  memset(m_reserved, 0, sizeof(m_reserved));
  memset(m_reserved_0, 0, sizeof(m_reserved_0));
}

bool AudioSampleEntry::write_audio_sample_entry(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The audio sample entry is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
        || (!file_writer->write_data(m_reserved_0, 6 * sizeof(uint8_t)))
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_channelcount))
        || (!file_writer->write_be_u16(m_sample_size))
        || (!file_writer->write_be_u16(m_pre_defined))
        || (!file_writer->write_be_u16(m_reserved_1))
        || (!file_writer->write_be_u16(m_sample_rate_integer))
        || (!file_writer->write_be_u16(m_sample_rate_decimal)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    if (m_aac.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_aac.write_aac_descriptor_box(file_writer))) {
        ERROR("Failed to write audio sample description box.");
        ret = false;
        break;
      }
    }
    if (m_opus.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_opus.write_opus_specific_box(file_writer))) {
        ERROR("Failed to write opus specific box");
        ret = false;
        break;
      }
    }
    INFO("write audio sample entry success, size is %u.", m_base_box.m_size);
  } while (0);
  return ret;
}

bool AudioSampleEntry::read_audio_sample_entry(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read audio sample entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
        || (!file_reader->read_data(m_reserved_0, 6 * sizeof(uint8_t)))
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
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_channelcount))
        || (!file_reader->read_le_u16(m_sample_size))
        || (!file_reader->read_le_u16(m_pre_defined))
        || (!file_reader->read_le_u16(m_reserved_1))
        || (!file_reader->read_le_u16(m_sample_rate_integer))
        || (!file_reader->read_le_u16(m_sample_rate_decimal)))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    switch ((uint32_t) m_base_box.m_type) {
      case DISOM_OPUS_ENTRY_TAG : {
        if (AM_UNLIKELY(!m_opus.read_opus_specific_box(file_reader))) {
          ERROR("Failed to read opus specific box.");
          ret = false;
        }
      } break;
      case DISOM_AAC_ENTRY_TAG : {
        if (AM_UNLIKELY(!m_aac.read_aac_descriptor_box(file_reader))) {
          ERROR("Failed to read audio sample description box.");
          ret = false;
        }
      } break;
      default: {
        ERROR("invalid audio sample entry type.");
        ret = false;
      } break;
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read audio sample description box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read audio sample description box error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read audio sample entry success.");
  } while (0);
  return ret;
}

bool AudioSampleEntry::get_proper_audio_sample_entry(AudioSampleEntry& entry,
                                                     SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    if (m_aac.m_full_box.m_base_box.m_enable) {
      if (!m_aac.get_proper_aac_descriptor_box(entry.m_aac, media_info)) {
        ERROR("Failed to get proper aac elementary stream description box.");
        ret = false;
        break;
      }
    }
    if (m_opus.m_base_box.m_enable) {
      if (!m_opus.get_proper_opus_specific_box(entry.m_opus, media_info)) {
        ERROR("Failed to get proper opus specific box.");
        ret = false;
        break;
      }
    }
    memcpy((uint8_t*) (entry.m_reserved), (uint8_t*) m_reserved,
           sizeof(uint32_t) * 2);
    entry.m_sample_rate_integer = m_sample_rate_integer;
    entry.m_sample_rate_decimal = m_sample_rate_decimal;
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_channelcount = m_channelcount;
    entry.m_sample_size = m_sample_size;
    entry.m_pre_defined = m_pre_defined;
    entry.m_reserved_1 = m_reserved_1;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool AudioSampleEntry::copy_audio_sample_entry(AudioSampleEntry& entry)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    if (m_aac.m_full_box.m_base_box.m_enable) {
      if (!m_aac.copy_aac_descriptor_box(entry.m_aac)) {
        ERROR("Failed to copy aac elementary stream description box.");
        ret = false;
        break;
      }
    }
    if (m_opus.m_base_box.m_enable) {
      if (!m_opus.copy_opus_specific_box(entry.m_opus)) {
        ERROR("Failed to copy opus specific box.");
        ret = false;
        break;
      }
    }
    memcpy((uint8_t*) (entry.m_reserved), (uint8_t*) m_reserved,
           sizeof(uint32_t) * 2);
    entry.m_sample_rate_integer = m_sample_rate_integer;
    entry.m_sample_rate_decimal = m_sample_rate_decimal;
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_channelcount = m_channelcount;
    entry.m_sample_size = m_sample_size;
    entry.m_pre_defined = m_pre_defined;
    entry.m_reserved_1 = m_reserved_1;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool AudioSampleEntry::get_combined_audio_sample_entry(
    AudioSampleEntry& source_entry, AudioSampleEntry& combined_entry)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_entry.m_base_box.m_enable)) {
      ERROR("One of combined audio sample entry is not enabled.");
      ret = false;
      break;
    }
    if ((source_entry.m_sample_rate_integer != m_sample_rate_integer)
     || (source_entry.m_channelcount != m_channelcount)
     || (source_entry.m_sample_size != m_sample_size)) {
      ERROR("audio info is not same.");
      ret = false;
      break;
    }
    if ((m_aac.m_full_box.m_base_box.m_enable)
        && (source_entry.m_aac.m_full_box.m_base_box.m_enable)) {
      if (!m_aac.get_combined_aac_descriptor_box(source_entry.m_aac,
                                                      combined_entry.m_aac)) {
        ERROR("Get combined aac elementary stream description box error.");
        ret = false;
        break;
      }
    } else if ((m_opus.m_base_box.m_enable)
        && (source_entry.m_opus.m_base_box.m_enable)) {
      if (!m_opus.get_combined_opus_specific_box(source_entry.m_opus,
                                                 combined_entry.m_opus)) {
        ERROR("Failed to get combined opus specific box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The audio type is not same.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_entry.m_base_box);
    memcpy((uint8_t*) combined_entry.m_reserved,
           (uint8_t*) m_reserved, sizeof(uint32_t) * 2);
    combined_entry.m_sample_rate_integer = m_sample_rate_integer;
    combined_entry.m_sample_rate_decimal = m_sample_rate_decimal;
    combined_entry.m_data_reference_index = m_data_reference_index;
    combined_entry.m_channelcount = m_channelcount;
    combined_entry.m_sample_size = m_sample_size;
    combined_entry.m_pre_defined = m_pre_defined;
    combined_entry.m_reserved_1 = m_reserved_1;
    memcpy(combined_entry.m_reserved_0, m_reserved_0, sizeof(uint8_t) * 6);
  } while (0);
  return ret;
}

void AudioSampleEntry::update_audio_sample_entry_size()
{
  m_base_box.m_size = DISOM_AUDIO_SAMPLE_ENTRY_SIZE;
  if (m_aac.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_aac.m_full_box.m_base_box.m_size;
    INFO("aac entry size is %u", m_aac.m_full_box.m_base_box.m_size);
  }
  if (m_opus.m_base_box.m_enable) {
    m_opus.update_opus_specific_box_size();
    m_base_box.m_size += m_opus.m_base_box.m_size;
    INFO("opus entry size is %u", m_opus.m_base_box.m_size);
  }
}

TextMetadataSampleEntry::TextMetadataSampleEntry() :
    m_data_reference_index(0)
{
  memset(m_reserved, 0, 6 * sizeof(uint8_t));
  m_mime_format.clear();
}

bool TextMetadataSampleEntry::write_text_metadata_sample_entry(
                                              Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer))
        || (!file_writer->write_data(m_reserved, 6 * sizeof(uint8_t)))
        || (!file_writer->write_be_u16(m_data_reference_index)))) {
      ERROR("Failed to write text metadata sample entry");
      ret = false;
      break;
    }
    if (m_mime_format.size() > 0) {
      if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )m_mime_format.c_str(),
                                               m_mime_format.size()))) {
        ERROR("Failed to write mime format when write text metadata sample entry.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool TextMetadataSampleEntry::read_text_metadata_sample_entry(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  char* mime_format = nullptr;
  do {
    if (!m_base_box.read_base_box(file_reader)) {
      ERROR("Failed to read base box when read text metadata sample entry.");
      ret = false;
      break;
    }
    if (!file_reader->read_data(m_reserved, 6 * sizeof(uint8_t))) {
      ERROR("Failed to read m_reserved when read text metadata sample entry.");
      ret = false;
      break;
    }
    if (!file_reader->read_le_u16(m_data_reference_index)) {
      ERROR("Failed to read data reference index when read text metadata "
          "sample entry.");
      ret = false;
      break;
    }
    uint32_t mime_format_size = m_base_box.m_size -
        TEXT_METADATA_SAMPLE_ENTRY_SIZE;
    if (mime_format_size > 0) {
      mime_format = new char[mime_format_size];
      if (!mime_format) {
        ERROR("Failed to malloc memory for mime format.");
        ret = false;
        break;
      }
      if (!file_reader->read_data((uint8_t*)mime_format, mime_format_size)) {
        ERROR("Failed to read mime format when read text metadata sample entry.");
        ret = false;
        break;
      }
      m_mime_format = mime_format;
      INFO("Read mime format is %s", m_mime_format.c_str());
    }
    m_base_box.m_enable = true;
  } while(0);
  delete[] mime_format;
  return ret;
}

bool TextMetadataSampleEntry::get_proper_text_medadata_sample_entry(
    TextMetadataSampleEntry& entry, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    memcpy(entry.m_reserved, m_reserved, 6 * sizeof(uint8_t));
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_mime_format = m_mime_format;
  } while (0);
  return ret;
}

bool TextMetadataSampleEntry::copy_text_medadata_sample_entry(
    TextMetadataSampleEntry& entry)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(entry.m_base_box);
    memcpy(entry.m_reserved, m_reserved, 6 * sizeof(uint8_t));
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_mime_format = m_mime_format;
  } while (0);
  return ret;
}

bool TextMetadataSampleEntry::get_combined_text_medadata_sample_entry(
  TextMetadataSampleEntry&source_entry, TextMetadataSampleEntry& combined_entry)
{
  bool ret = true;
  do {
    if (m_data_reference_index != source_entry.m_data_reference_index) {
      ERROR("The data reference index is not same.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_entry.m_base_box);
    memcpy(combined_entry.m_reserved, m_reserved, sizeof(uint8_t) * 6);
    combined_entry.m_data_reference_index = m_data_reference_index;
    combined_entry.m_mime_format = m_mime_format;
  } while(0);
  return ret;
}

void TextMetadataSampleEntry::update_text_metadata_sample_entry_size()
{
  m_base_box.m_size = TEXT_METADATA_SAMPLE_ENTRY_SIZE + m_mime_format.size();
}

SampleDescriptionBox::SampleDescriptionBox() :
    m_entry_count(0)
{
}
bool SampleDescriptionBox::write_sample_description_box(
    Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample description box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer))
                    || (!file_writer->write_be_u32(m_entry_count)))) {
      ERROR("Failed to write sample description box.");
      ret = false;
      break;
    }
    if (m_visual_entry.m_base_box.m_enable) {
      if (!m_visual_entry.write_visual_sample_entry(file_writer)) {
        ERROR("Failed to write visual sample entry.");
        ret = false;
        break;
      }
    }
    if (m_audio_entry.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_audio_entry.write_audio_sample_entry(file_writer))) {
        ERROR("Failed to write audio sample entry.");
        ret = false;
        break;
      }
    }
    if (m_metadata_entry.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_metadata_entry.write_text_metadata_sample_entry(
          file_writer))) {
        ERROR("Failed to write metadata sample entry.");
        ret = false;
        break;
      }
    }
    INFO("sample description box size is %u when write it.",
         m_full_box.m_base_box.m_size);
  } while (0);
  return ret;
}

bool SampleDescriptionBox::read_sample_description_box(
    Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read visual sample description box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.read_full_box(file_reader))
                    || (!file_reader->read_le_u32(m_entry_count)))) {
      ERROR("Failed to read visual sample description box.");
      ret = false;
      break;
    }
    uint32_t type = file_reader->get_box_type();
    switch (type) {
      case DISOM_H264_ENTRY_TAG :
      case DISOM_H265_ENTRY_TAG : {
        if (!m_visual_entry.read_visual_sample_entry(file_reader)) {
          ERROR("Failed to read visual sample entry.");
          ret = false;
        }
      }break;
      case DISOM_OPUS_ENTRY_TAG :
      case DISOM_AAC_ENTRY_TAG : {
        if (!m_audio_entry.read_audio_sample_entry(file_reader)) {
          ERROR("Failed to read audio sample entry.");
          ret = false;
          break;
        }
      } break;
      case DISOM_META_ENTRY_TAG : {
        if (!m_metadata_entry.read_text_metadata_sample_entry(file_reader)) {
          ERROR("Failed to read metadata sample entry.");
          ret = false;
          break;
        }
      } break;
      default: {
        ERROR("Get invalid type when read visual sample description box.");
        ret = false;
      } break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read visual sample description box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read visaul sample description box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read visual sample description box success.");
  } while (0);
  return ret;
}

bool SampleDescriptionBox::get_proper_sample_description_box(
    SampleDescriptionBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_visual_entry.m_base_box.m_enable) {
      if (!m_visual_entry.get_proper_visual_sample_entry(
          box.m_visual_entry, media_info)) {
        ERROR("Failed to get proper visual sample entry.");
        ret = false;
        break;
      }
    }
    if (m_audio_entry.m_base_box.m_enable) {
      if (!m_audio_entry.get_proper_audio_sample_entry(box.m_audio_entry,
                                                       media_info)) {
        ERROR("Failed to get proper audio sample entry.");
        ret = false;
        break;
      }
    }
    if (m_metadata_entry.m_base_box.m_enable) {
      if (!m_metadata_entry.get_proper_text_medadata_sample_entry(
          box.m_metadata_entry,media_info)) {
        ERROR("Failed to get proper text metadata sample entry.");
        ret = false;
        break;
      }
    }
    box.m_entry_count = m_entry_count;
  } while (0);
  return ret;
}

bool SampleDescriptionBox::copy_sample_description_box(
    SampleDescriptionBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_visual_entry.m_base_box.m_enable) {
      if (!m_visual_entry.copy_visual_sample_entry(
          box.m_visual_entry)) {
        ERROR("Failed to copy visual sample entry.");
        ret = false;
        break;
      }
    }
    if (m_audio_entry.m_base_box.m_enable) {
      if (!m_audio_entry.copy_audio_sample_entry(box.m_audio_entry)) {
        ERROR("Failed to copy audio sample entry.");
        ret = false;
        break;
      }
    }
    if (m_metadata_entry.m_base_box.m_enable) {
      if (!m_metadata_entry.copy_text_medadata_sample_entry(
          box.m_metadata_entry)) {
        ERROR("Failed to copy text metadata sample entry.");
        ret = false;
        break;
      }
    }
    box.m_entry_count = m_entry_count;
  } while (0);
  return ret;
}

bool SampleDescriptionBox::get_combined_sample_description_box(
    SampleDescriptionBox& source_box,
    SampleDescriptionBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined visual sample description box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_entry_count = m_entry_count;
    if (m_visual_entry.m_base_box.m_enable) {
      if (!m_visual_entry.get_combined_visual_sample_entry(
          source_box.m_visual_entry, combined_box.m_visual_entry)) {
        ERROR("Get combined visual sample entry error.");
        ret = false;
        break;
      }
    }
    if (m_audio_entry.m_base_box.m_enable) {
      if (!m_audio_entry.get_combined_audio_sample_entry(
          source_box.m_audio_entry, combined_box.m_audio_entry)) {
        ERROR("Failed to get combined audio sample entry.");
        ret = false;
        break;
      }
    }
    if (m_metadata_entry.m_base_box.m_enable) {
      if (!m_metadata_entry.get_combined_text_medadata_sample_entry(
          source_box.m_metadata_entry, combined_box.m_metadata_entry)) {
        ERROR("Failed to get combined text metadata sample entry.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

void SampleDescriptionBox::update_sample_description_box()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t);
  if (m_visual_entry.m_base_box.m_enable) {
    m_visual_entry.update_visual_sample_entry_size();
    m_full_box.m_base_box.m_size += m_visual_entry.m_base_box.m_size;
  }
  if (m_audio_entry.m_base_box.m_enable) {
    m_audio_entry.update_audio_sample_entry_size();
    m_full_box.m_base_box.m_size += m_audio_entry.m_base_box.m_size;
  }
  if (m_metadata_entry.m_base_box.m_enable) {
    m_metadata_entry.update_text_metadata_sample_entry_size();
    m_full_box.m_base_box.m_size += m_metadata_entry.m_base_box.m_size;
  }
}

SampleSizeBox::SampleSizeBox() :
    m_size_array(nullptr),
    m_sample_size(0),
    m_sample_count(0),
    m_buf_count(0)
{
}

SampleSizeBox::~SampleSizeBox()
{
  delete[] m_size_array;
}

bool SampleSizeBox::write_sample_size_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample size box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_sample_size))
        || (!file_writer->write_be_u32(m_sample_count)))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_sample_count; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_size_array[i]))) {
        ERROR("Failed to write sample size box");
        ret = false;
        break;
      }
    }
    if (ret != true) {
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
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read sample size box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read sample size box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_sample_size))
        || (!file_reader->read_le_u32(m_sample_count)))) {
      ERROR("Failed to read sample size box");
      ret = false;
      break;
    }
    if ((m_sample_count > 0) && (m_sample_size == 0)) {
      delete[] m_size_array;
      m_size_array = new uint32_t[m_sample_count];
      if (AM_UNLIKELY(!m_size_array)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_sample_count; i ++) {
        if (AM_UNLIKELY(!file_reader->read_le_u32(m_size_array[i]))) {
          ERROR("Failed to read sample size box");
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read sample size box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read sample size box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read sample size box success.");
  } while (0);
  return ret;
}

void SampleSizeBox::update_sample_size_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_SAMPLE_SIZE_BOX_SIZE;
  if (m_sample_size == 0) {
    m_full_box.m_base_box.m_size += m_sample_count * sizeof(uint32_t);
  }
}

bool SampleSizeBox::get_proper_sample_size_box(SampleSizeBox& box,
                                               SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_sample_size = m_sample_size;
    box.m_sample_count = media_info.get_last_frame() -
        media_info.get_first_frame() + 1;
    if (media_info.get_last_frame() > m_sample_count) {
      ERROR("end frame is bigger than sample count.");
      ret = false;
      break;
    }
    if (m_sample_size == 0) {
      uint32_t sample_count = media_info.get_last_frame()
            - media_info.get_first_frame() + 1;
      delete[] box.m_size_array;
      box.m_size_array = new uint32_t[sample_count];
      if (AM_UNLIKELY(!box.m_size_array)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      memcpy((uint8_t*) box.m_size_array,
             (uint8_t*) (&m_size_array[media_info.get_first_frame() - 1]),
             sample_count * sizeof(uint32_t));
    }
    box.update_sample_size_box_size();
  } while (0);
  return ret;
}

bool SampleSizeBox::copy_sample_size_box(SampleSizeBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_sample_size = m_sample_size;
    box.m_sample_count = m_sample_count;
    box.m_buf_count = m_buf_count;
    delete[] box.m_size_array;
    box.m_size_array = new uint32_t[m_sample_count];
    if (AM_UNLIKELY(!box.m_size_array)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    memcpy((uint8_t*) box.m_size_array, (uint8_t*) m_size_array,
           m_sample_count * sizeof(uint32_t));
  } while (0);
  return ret;
}

bool SampleSizeBox::get_combined_sample_size_box(SampleSizeBox& source_box,
                                                 SampleSizeBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined sample size box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_sample_size = m_sample_size;
    combined_box.m_sample_count = source_box.m_sample_count + m_sample_count;
    if (m_sample_size == 0) {
      combined_box.m_buf_count = source_box.m_sample_count +
          m_sample_count + 1;
      delete[] combined_box.m_size_array;
      combined_box.m_size_array = new uint32_t[combined_box.m_buf_count];
      if (!combined_box.m_size_array) {
        ERROR("Failed to malloc memory for sample size.");
        ret = false;
        break;
      }
      INFO("entry count is %u, source entry count is %u, combined entry count "
          "is %u",
          m_sample_count, source_box.m_sample_count, combined_box.m_sample_count);
      memcpy((uint8_t*) combined_box.m_size_array,
             (uint8_t*) m_size_array,
             m_sample_count * sizeof(uint32_t));
      memcpy((uint8_t*) (&combined_box.m_size_array[m_sample_count]),
             (uint8_t*) source_box.m_size_array,
             source_box.m_sample_count * sizeof(uint32_t));
    }
  } while (0);
  return ret;
}

SampleToChunkEntry::SampleToChunkEntry() :
    m_first_chunk(0),
    m_sample_per_chunk(0),
    m_sample_description_index(0)
{
}

bool SampleToChunkEntry::write_sample_chunk_entry(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_first_chunk))
        || (!file_writer->write_be_u32(m_sample_per_chunk))
        || (!file_writer->write_be_u32(m_sample_description_index)))) {
      ERROR("Failed to write sample to chunk box.");
      ret = false;
      break;
    }
    INFO("write sample chunk entry success.");
  } while (0);
  return ret;
}

bool SampleToChunkEntry::read_sample_chunk_entry(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    INFO("begin to read sample chunk entry.");
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_first_chunk))
        || (!file_reader->read_le_u32(m_sample_per_chunk))
        || (!file_reader->read_le_u32(m_sample_description_index)))) {
      ERROR("Failed to read sample to chunk box.");
      ret = false;
      break;
    }
    INFO("read sample chunk entry success.");
  } while (0);
  return ret;
}

SampleToChunkBox::SampleToChunkBox() :
    m_entrys(nullptr),
    m_entry_count(0)
{
}

SampleToChunkBox::~SampleToChunkBox()
{
  delete[] m_entrys;
}

bool SampleToChunkBox::write_sample_to_chunk_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample to chunk box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
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
      if (AM_UNLIKELY(!m_entrys[i].write_sample_chunk_entry(file_writer))) {
        ERROR("Failed to write sample to chunk box.");
        ret = false;
        break;
      }
    }
    INFO("write sample to chunk box success.");
  } while (0);
  return ret;
}

bool SampleToChunkBox::read_sample_to_chunk_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read sample to chunk box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read sample to chunk box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read sample to chunk box");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_entry_count > 0)) {
      delete[] m_entrys;
      m_entrys = new SampleToChunkEntry[m_entry_count];
      if (AM_UNLIKELY(!m_entrys)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY(!m_entrys[i].read_sample_chunk_entry(file_reader))) {
          ERROR("Failed to read sample to chunk box.");
          ret = false;
          break;
        }
      }
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read sample to chunk box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read sample to chunk box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read sample to chunk box success.");
  } while (0);
  return ret;
}

void SampleToChunkBox::update_sample_to_chunk_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * SAMPLE_TO_CHUNK_ENTRY_SIZE;
}

bool SampleToChunkBox::get_proper_sample_to_chunk_box(
    SampleToChunkBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_entry_count > 0) {
      box.m_entry_count = 1;
      delete[] box.m_entrys;
      box.m_entrys = new SampleToChunkEntry[1];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      box.m_entrys[0].m_first_chunk = 1;
      box.m_entrys[0].m_sample_per_chunk = 1;
      box.m_entrys[0].m_sample_description_index = 1;
    }
    box.update_sample_to_chunk_box_size();
  } while (0);
  return ret;
}

bool SampleToChunkBox::copy_sample_to_chunk_box(
    SampleToChunkBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_entry_count > 0) {
      box.m_entry_count = 1;
      delete[] box.m_entrys;
      box.m_entrys = new SampleToChunkEntry[1];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      box.m_entrys[0].m_first_chunk = 1;
      box.m_entrys[0].m_sample_per_chunk = 1;
      box.m_entrys[0].m_sample_description_index = 1;
    }
  } while (0);
  return ret;
}

bool SampleToChunkBox::get_combined_sample_to_chunk_box(
    SampleToChunkBox& source_box, SampleToChunkBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined sample to chunk box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (m_entry_count > 0) {
      combined_box.m_entry_count = 1;
      delete[] combined_box.m_entrys;
      combined_box.m_entrys = new SampleToChunkEntry[1];
      if (!combined_box.m_entrys) {
        ERROR("Failed to malloc memory for sample to chunk box.");
        ret = false;
        break;
      }
      INFO("entry count is %u, source entry count is %u, combined entry count"
          "is %u",
          m_entry_count, source_box.m_entry_count, combined_box.m_entry_count);
      combined_box.m_entrys[0].m_first_chunk = 1;
      combined_box.m_entrys[0].m_sample_description_index = 1;
      combined_box.m_entrys[0].m_sample_per_chunk = 1;
    }
  } while (0);
  return ret;
}

ChunkOffsetBox::ChunkOffsetBox() :
    m_chunk_offset(nullptr),
    m_entry_count(0),
    m_buf_count(0)
{
}

ChunkOffsetBox::~ChunkOffsetBox()
{
  delete[] m_chunk_offset;
}

bool ChunkOffsetBox::write_chunk_offset_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The chunk offset box is not enabled.");
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write chunk offset box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_chunk_offset[i]))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    INFO("write chunk offset box success.");
  } while (0);
  return ret;
}

bool ChunkOffsetBox::read_chunk_offset_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read chunk offset box");
      ret = false;
      break;
    }
    INFO("Chunk offset entry count is %u", m_entry_count);
    if (AM_LIKELY(m_entry_count > 0)) {
      delete[] m_chunk_offset;
      m_chunk_offset = new uint32_t[m_entry_count];
      if (AM_UNLIKELY(!m_chunk_offset)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; i ++) {
        if (AM_UNLIKELY(!file_reader->read_le_u32(m_chunk_offset[i]))) {
          ERROR("Failed to read chunk offset box.");
          ret = false;
          break;
        }
      }
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read chunk offset box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read chunk offset box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read chunk offset box success.");
  } while (0);
  return ret;
}

void ChunkOffsetBox::update_chunk_offset_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * sizeof(uint32_t);
}

bool ChunkOffsetBox::get_proper_chunk_offset_box(ChunkOffsetBox& box,
                                                 SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    //create m_chunk_offset and fill it on the outsize.
  } while (0);
  return ret;
}

bool ChunkOffsetBox::copy_chunk_offset_box(ChunkOffsetBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_buf_count = m_buf_count;
    delete[] box.m_chunk_offset;
    box.m_chunk_offset = new uint32_t[m_entry_count];
    if (!box.m_chunk_offset) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      box.m_chunk_offset[i] = m_chunk_offset[i];
    }
  } while (0);
  return ret;
}

bool ChunkOffsetBox::get_combined_chunk_offset_box(ChunkOffsetBox& source_box,
                                                   ChunkOffsetBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined chunk offset box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    //create m_chunk_offset and fill it on the outsize.
  } while (0);
  return ret;
}

ChunkOffset64Box::ChunkOffset64Box() :
    m_chunk_offset(nullptr),
    m_entry_count(0),
    m_buf_count(0)
{

}

ChunkOffset64Box::~ChunkOffset64Box()
{
  delete[] m_chunk_offset;
}

bool ChunkOffset64Box::write_chunk_offset64_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The chunk offset box is not enabled.");
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count))) {
      ERROR("Failed to write chunk offset box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u64(m_chunk_offset[i]))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    INFO("write chunk offset box success.");
  } while (0);
  return ret;
}

bool ChunkOffset64Box::read_chunk_offset64_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read chunk offset box");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_entry_count > 0)) {
      delete[] m_chunk_offset;
      m_chunk_offset = new uint64_t[m_entry_count];
      if (AM_UNLIKELY(!m_chunk_offset)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; i ++) {
        if (AM_UNLIKELY(!file_reader->read_le_u64(m_chunk_offset[i]))) {
          ERROR("Failed to read chunk offset box.");
          ret = false;
          break;
        }
      }
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read chunk offset box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read chunk offset box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read chunk offset box success.");
  } while (0);
  return ret;
}

void ChunkOffset64Box::update_chunk_offset64_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
            + m_entry_count * sizeof(uint64_t);
}

bool ChunkOffset64Box::get_proper_chunk_offset64_box(
    ChunkOffset64Box& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    //create and fill m_chunk_offset on the outsize.
  } while (0);
  return ret;
}

bool ChunkOffset64Box::copy_chunk_offset64_box(ChunkOffset64Box& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_buf_count = m_buf_count;
    delete[] box.m_chunk_offset;
    box.m_chunk_offset = new uint64_t[m_entry_count];
    if (!box.m_chunk_offset) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      box.m_chunk_offset[i] = m_chunk_offset[i];
    }
  } while (0);
  return ret;
}

bool ChunkOffset64Box::get_combined_chunk_offset64_box(
    ChunkOffset64Box& source_box, ChunkOffset64Box& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined chunk offset64 box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    //create and fill m_chunk_offset on the outsize.
  } while (0);
  return ret;
}

SyncSampleTableBox::SyncSampleTableBox() :
    m_sync_sample_table(nullptr),
    m_stss_count(0),
    m_buf_count(0)
{
}

SyncSampleTableBox::~SyncSampleTableBox()
{
  delete[] m_sync_sample_table;
}

bool SyncSampleTableBox::write_sync_sample_table_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sync sample table box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_stss_count))) {
      ERROR("Failed to write stss count.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_stss_count; ++ i) {
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

bool SyncSampleTableBox::read_sync_sample_table_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read sync sample table box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read full box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_stss_count))) {
      ERROR("Failed to read stss count.");
      ret = false;
      break;
    }
    if (AM_LIKELY(m_stss_count > 0)) {
      m_sync_sample_table = new uint32_t[m_stss_count];
      if (AM_UNLIKELY(!m_sync_sample_table)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_stss_count; ++ i) {
        if (AM_UNLIKELY(!file_reader->read_le_u32(m_sync_sample_table[i]))) {
          ERROR("Failed to read sync sample table box.");
          ret = false;
          break;
        }
      }
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read sync sample table box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read sync sample table box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read sync sample table box success.");
  } while (0);
  return ret;
}

void SyncSampleTableBox::update_sync_sample_box_size()
{
  m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_stss_count * sizeof(uint32_t);
}

bool SyncSampleTableBox::get_proper_sync_sample_table_box(
    SyncSampleTableBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    uint32_t i = 0;
    for (; i < m_stss_count; ++ i) {
      if (m_sync_sample_table[i] == media_info.get_first_frame()) {
        break;
      }
    }
    if (AM_UNLIKELY(i == m_stss_count)) {
      ERROR("The start frame is not the IDR frame");
      ret = false;
      break;
    }
    delete[] box.m_sync_sample_table;
    box.m_sync_sample_table = new uint32_t[m_stss_count];
    if (AM_UNLIKELY(!box.m_sync_sample_table)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (uint32_t j = 0; i < m_stss_count; ++ i, ++ j) {
      if (media_info.get_last_frame() >= m_sync_sample_table[i]) {
        box.m_sync_sample_table[j] = m_sync_sample_table[i]
            - media_info.get_first_frame() + 1;
        if (i == (m_stss_count - 1)) {
          box.m_stss_count = j + 1;
          break;
        }
      } else {
        box.m_stss_count = j;
        break;
      }
    }
  } while (0);
  return ret;
}

bool SyncSampleTableBox::copy_sync_sample_table_box(SyncSampleTableBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_stss_count = m_stss_count;
    box.m_buf_count = m_buf_count;
    delete[] box.m_sync_sample_table;
    box.m_sync_sample_table = new uint32_t[m_stss_count];
    if (AM_UNLIKELY(!box.m_sync_sample_table)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_stss_count; ++ i) {
      box.m_sync_sample_table[i] = m_sync_sample_table[i];
    }
  } while (0);
  return ret;
}


bool SyncSampleTableBox::get_combined_sync_sample_table_box(
    SyncSampleTableBox& source_box, SyncSampleTableBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of sync sample table box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_buf_count = source_box.m_stss_count + m_stss_count;
    combined_box.m_stss_count = source_box.m_stss_count + m_stss_count;
    if (combined_box.m_stss_count > 0) {
      delete[] combined_box.m_sync_sample_table;
      combined_box.m_sync_sample_table = new uint32_t[combined_box.m_stss_count];
      if (!combined_box.m_sync_sample_table) {
        ERROR("Failed to malloc memory for sync sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool DataEntryBox::write_data_entry_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The data entry box of reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer))) {
      ERROR("Failed to write full box when write data entry box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool DataEntryBox::read_data_entry_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read data entry box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.read_full_box(file_reader))) {
      ERROR("Failed to read full box when read data entry box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read data entry box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read data entry box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool DataEntryBox::get_proper_data_entry_box(DataEntryBox& box,
                                             SubMediaInfo& media_info)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool DataEntryBox::copy_data_entry_box(DataEntryBox& box)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool DataEntryBox::get_combined_data_entry_box(DataEntryBox& source_box,
                                               DataEntryBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
        || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined data entry box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
  } while (0);
  return ret;
}

DataReferenceBox::DataReferenceBox() :
    m_entry_count(0)
{
}

bool DataReferenceBox::write_data_reference_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The data reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer))
        || (!file_writer->write_be_u32(m_entry_count)))) {
      ERROR("Failed to write data reference box.");
      ret = true;
      break;
    }
    if (m_url.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_url.write_data_entry_box(file_writer))) {
        ERROR("Failed to write data reference box.");
        ret = false;
        break;
      }
    }
    INFO("Write reference box success.");
  } while (0);
  return ret;
}

bool DataReferenceBox::read_data_reference_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read data reference box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.read_full_box(file_reader))
        || (!file_reader->read_le_u32(m_entry_count)))) {
      ERROR("Failed to read data reference box.");
      ret = true;
      break;
    }
    if (m_entry_count != 1) {
      ERROR("entry count is not correct, m_entry_count is %u", m_entry_count);
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_url.read_data_entry_box(file_reader))) {
      ERROR("Failed to read data reference box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read data reference box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("Read data reference box error.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
    INFO("read data reference box success.");
  } while (0);
  return ret;
}

bool DataReferenceBox::get_proper_data_reference_box(
    DataReferenceBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    if (!m_url.get_proper_data_entry_box(box.m_url, media_info)) {
      ERROR("Failed to get proper data entry box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool DataReferenceBox::copy_data_reference_box(DataReferenceBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    m_url.copy_data_entry_box(box.m_url);
  } while (0);
  return ret;
}

bool DataReferenceBox::get_combined_data_reference_box(
    DataReferenceBox& source_box, DataReferenceBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_full_box.m_base_box.m_enable)
     || (!source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("One of combined data reference box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    if (m_url.m_full_box.m_base_box.m_enable) {
      if (!m_url.get_combined_data_entry_box(source_box.m_url,
                                             combined_box.m_url)) {
        ERROR("Get combined data entry box error.");
        ret = false;
        break;
      }
    }
    combined_box.m_entry_count = m_entry_count;
  } while (0);
  return ret;
}

bool DataInformationBox::write_data_info_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The data information box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write data infomation box.");
      ret = false;
      break;
    }
    if (m_data_ref.m_full_box.m_base_box.m_enable) {
      if (!m_data_ref.write_data_reference_box(file_writer)) {
        ERROR("Failed to write data reference box");
        ret = false;
        break;
      }
    }
    INFO("write data information box success.");
  } while (0);
  return ret;
}

bool DataInformationBox::read_data_info_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read data information box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader))
        || (!m_data_ref.read_data_reference_box(file_reader)))) {
      ERROR("Failed to read data infomation box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read data information box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("Read data information box error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read data information box success.");
  } while (0);
  return ret;
}

bool DataInformationBox::get_proper_data_info_box(
    DataInformationBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_data_ref.m_full_box.m_base_box.m_enable) {
      if (!m_data_ref.get_proper_data_reference_box(box.m_data_ref,media_info)) {
        ERROR("Failed to get proper data reference box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool DataInformationBox::copy_data_info_box(DataInformationBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_data_ref.m_full_box.m_base_box.m_enable) {
      if (!m_data_ref.copy_data_reference_box(box.m_data_ref)) {
        ERROR("Failed to copy data reference box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool DataInformationBox::get_combined_data_info_box(
    DataInformationBox& source_box, DataInformationBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined data information box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_data_ref.m_full_box.m_base_box.m_enable) {
      if (!m_data_ref.get_combined_data_reference_box(source_box.m_data_ref,
                                                      combined_box.m_data_ref)) {
        ERROR("Get combined data reference box error.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool SampleTableBox::write_sample_table_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The sample table box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write sample table box.");
      ret = false;
      break;
    }
    if (m_sample_description.m_full_box.m_base_box.m_enable) {
      if (!m_sample_description.write_sample_description_box(
          file_writer)) {
        ERROR("Failed to write sample description box.");
        ret = false;
        break;
      }
    }
    if (m_stts.m_full_box.m_base_box.m_enable) {
      if ((!m_stts.write_decoding_time_to_sample_box(file_writer))) {
        ERROR("Failed to write decoding time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_ctts.m_full_box.m_base_box.m_enable) {
      if ((!m_ctts.write_composition_time_to_sample_box(file_writer))) {
        ERROR("Failed to write composition time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_sample_size.m_full_box.m_base_box.m_enable) {
      if ((!m_sample_size.write_sample_size_box(file_writer))) {
        ERROR("Failed to write sample size box.");
        ret = false;
        break;
      }
    }
    if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      if ((!m_sample_to_chunk.write_sample_to_chunk_box(file_writer))) {
        ERROR("Failed to write sample to chunk box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
      if ((!m_chunk_offset.write_chunk_offset_box(file_writer))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset64.m_full_box.m_base_box.m_enable) {
      if ((!m_chunk_offset64.write_chunk_offset64_box(file_writer))) {
        ERROR("Failed to write chunk offset64 box.");
        ret = false;
        break;
      }
    }
    if (m_sync_sample.m_full_box.m_base_box.m_enable) {
      INFO("begin to write sync_sample table box");
      if (!m_sync_sample.write_sync_sample_table_box(file_writer)) {
        ERROR("Failed to write sync sample table box.");
        ret = false;
        break;
      }
    }
    INFO("write sample table box success.");
  } while (0);
  return ret;
}

bool SampleTableBox::read_sample_table_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read sample table box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader)))) {
      ERROR("Failed to read sample table box.");
      ret = false;
    }
    do {
      DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case DISOM_SAMPLE_DESCRIPTION_BOX_TAG: {
          if (!m_sample_description.read_sample_description_box(file_reader)) {
            ERROR("Failed to read visual sample description box.");
            ret = false;
          }
        } break;
        case DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG: {
          if (!m_stts.read_decoding_time_to_sample_box(file_reader)) {
            ERROR("Failed to read decoding time to sample box.");
            ret = false;
          }
        } break;
        case DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG: {
          if (!m_ctts.read_composition_time_to_sample_box(file_reader)) {
            ERROR("Failed to read composition time to sample box.");
            ret = false;
          }
        } break;
        case DISOM_SAMPLE_SIZE_BOX_TAG: {
          if (!m_sample_size.read_sample_size_box(file_reader)) {
            ERROR("Failed to read sample size box.");
            ret = false;
          }
        } break;
        case DISOM_SAMPLE_TO_CHUNK_BOX_TAG: {
          if (!m_sample_to_chunk.read_sample_to_chunk_box(file_reader)) {
            ERROR("Failed to read sample to chunk box.");
            ret = false;
          }
        } break;
        case DISOM_CHUNK_OFFSET_BOX_TAG: {
          if (!m_chunk_offset.read_chunk_offset_box(file_reader)) {
            ERROR("Failed to read chunk offset box.");
            ret = false;
          }
        } break;
        case DISOM_CHUNK_OFFSET64_BOX_TAG : {
          if (!m_chunk_offset64.read_chunk_offset64_box(file_reader)) {
            ERROR("Failed to read chunk offset64 box.");
            ret = false;
          }
        } break;
        case DISOM_SYNC_SAMPLE_TABLE_BOX_TAG: {
          if (!m_sync_sample.read_sync_sample_table_box(file_reader)) {
            ERROR("Failed to read sync sample table box.");
            ret = false;
          }
        } break;
        default: {
          ERROR("Invalid box type tag.");
          ret = false;
          break;
        } break;
      }
      if (!file_reader->tell_file(offset_end)) {
        ERROR("Failed to tell file when read sample table box.");
        ret = false;
        break;
      }
    } while (((offset_end - offset_begin) < (m_base_box.m_size)) && ret);
    if (ret) {
    m_base_box.m_enable = true;
    } else {
      break;
    }
    INFO("Read sample table box success.");
  } while (0);
  return ret;
}

bool SampleTableBox::get_proper_sample_table_box(
    SampleTableBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_sample_description.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_description.get_proper_sample_description_box(
          box.m_sample_description, media_info))) {
        ERROR("Failed to get proper sample description box.");
        ret = false;
        break;
      }
    }
    if (m_stts.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_stts.get_proper_decoding_time_to_sample_box(
          box.m_stts, media_info))) {
        ERROR("Failed to get proper decoding time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_ctts.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_ctts.get_proper_composition_time_to_sample_box(
          box.m_ctts, media_info))) {
        ERROR("Failed to get proper composition time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_sample_size.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_size.get_proper_sample_size_box(
          box.m_sample_size, media_info))) {
        ERROR("Failed to get proper sample size box.");
        ret = false;
        break;
      }
    }
    if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_to_chunk.get_proper_sample_to_chunk_box(
          box.m_sample_to_chunk, media_info))) {
        ERROR("Failed to get porper sample to chunk box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_chunk_offset.get_proper_chunk_offset_box(
          box.m_chunk_offset, media_info))) {
        ERROR("Failed to get proper chunk offset box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset64.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_chunk_offset64.get_proper_chunk_offset64_box(
          box.m_chunk_offset64, media_info))) {
        ERROR("Failed to get proper chunk offset64 box.");
        ret = false;
        break;
      }
    }
    if (m_sync_sample.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sync_sample.get_proper_sync_sample_table_box(
          box.m_sync_sample, media_info))) {
        ERROR("Failed to get proper sync sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool SampleTableBox::copy_sample_table_box(SampleTableBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_sample_description.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_description.copy_sample_description_box(
          box.m_sample_description))) {
        ERROR("Failed to copy sample description box.");
        ret = false;
        break;
      }
    }
    if (m_stts.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_stts.copy_decoding_time_to_sample_box(box.m_stts))) {
        ERROR("Failed to copy decoding time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_ctts.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_ctts.copy_composition_time_to_sample_box(
          box.m_ctts))) {
        ERROR("Failed to copy composition time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_sample_size.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_size.copy_sample_size_box(
          box.m_sample_size))) {
        ERROR("Failed to copy sample size box.");
        ret = false;
        break;
      }
    }
    if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_to_chunk.copy_sample_to_chunk_box(
          box.m_sample_to_chunk))) {
        ERROR("Failed to copy sample to chunk box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_chunk_offset.copy_chunk_offset_box(
          box.m_chunk_offset))) {
        ERROR("Failed to copy chunk offset box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset64.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_chunk_offset64.copy_chunk_offset64_box(
          box.m_chunk_offset64))) {
        ERROR("Failed to copy chunk offset64 box.");
        ret = false;
        break;
      }
    }
    if (m_sync_sample.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sync_sample.copy_sync_sample_table_box(
          box.m_sync_sample))) {
        ERROR("Failed to copy sync sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool SampleTableBox::get_combined_sample_table_box(SampleTableBox& source_box,
                                                   SampleTableBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined sample table box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    INFO("begin to get combined decoding time to sample box.");
    if (m_stts.m_full_box.m_base_box.m_enable) {
      if (!m_stts.get_combined_decoding_time_to_sample_box(
          source_box.m_stts, combined_box.m_stts)) {
        ERROR("Get combined decoding time to sample box error.");
        ret = false;
        break;
      }
    }
    INFO("begin to get combined composition time to sample box.");
    if (m_ctts.m_full_box.m_base_box.m_enable) {
      if (!m_ctts.get_combined_composition_time_to_sample_box(
          source_box.m_ctts, combined_box.m_ctts)) {
        ERROR("Get combined composition time to sample box error.");
        ret = false;
        break;
      }
    }
    if (m_sample_description.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined sample description box");
      if (!m_sample_description.get_combined_sample_description_box(
          source_box.m_sample_description,combined_box.m_sample_description)) {
        ERROR("Get combined sample description box.");
        ret = false;
        break;
      }
    }
    if (m_sample_size.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined sample size box.");
      if (!m_sample_size.get_combined_sample_size_box(
          source_box.m_sample_size, combined_box.m_sample_size)) {
        ERROR("Get combined sample size box error.");
        ret = false;
        break;
      }
    }
    if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined sample to chunk box.");
      if (!m_sample_to_chunk.get_combined_sample_to_chunk_box(
          source_box.m_sample_to_chunk, combined_box.m_sample_to_chunk)) {
        ERROR("Get combined sample to chunk box error.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined chunk offset box.");
      if (!m_chunk_offset.get_combined_chunk_offset_box(
          source_box.m_chunk_offset, combined_box.m_chunk_offset)) {
        ERROR("Get combined chunk offset box error");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset64.m_full_box.m_base_box.m_enable) {
      INFO("Begin to get combined chunk offset64 box.");
      if (!m_chunk_offset64.get_combined_chunk_offset64_box(
          source_box.m_chunk_offset64, combined_box.m_chunk_offset64)) {
        ERROR("Failed to get combined chunk offset64 box.");
        ret = false;
        break;
      }
    }
    if (m_sync_sample.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined sync sample table box.");
      if (!m_sync_sample.get_combined_sync_sample_table_box(
          source_box.m_sync_sample, combined_box.m_sync_sample)) {
        ERROR("Get combined sync sample table box error.");
        ret = false;
        break;
      }
    }
    INFO("Get combined sample table box success.");
  } while (0);
  return ret;
}

void SampleTableBox::update_sample_table_box_size()
{
  m_base_box.m_size = DISOM_BASE_BOX_SIZE;
  if (m_sample_description.m_full_box.m_base_box.m_enable) {
    m_sample_description.update_sample_description_box();
    m_base_box.m_size += m_sample_description.m_full_box.m_base_box.m_size;
    INFO("Sample description size is %u",
         m_sample_description.m_full_box.m_base_box.m_size);
  }
  if (m_stts.m_full_box.m_base_box.m_enable) {
    m_stts.update_decoding_time_to_sample_box_size();
    m_base_box.m_size += m_stts.m_full_box.m_base_box.m_size;
    INFO("Decoding time box size is %u",
         m_stts.m_full_box.m_base_box.m_size);
  }
  if (m_ctts.m_full_box.m_base_box.m_enable) {
    m_ctts.update_composition_time_to_sample_box_size();
    m_base_box.m_size += m_ctts.m_full_box.m_base_box.m_size;
    INFO("Composition box size is %u",
         m_ctts.m_full_box.m_base_box.m_size);
  }
  if (m_sample_size.m_full_box.m_base_box.m_enable) {
    m_sample_size.update_sample_size_box_size();
    m_base_box.m_size += m_sample_size.m_full_box.m_base_box.m_size;
    INFO("Sample size box size is %u",
         m_sample_size.m_full_box.m_base_box.m_size);
  }
  if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
    m_sample_to_chunk.update_sample_to_chunk_box_size();
    m_base_box.m_size += m_sample_to_chunk.m_full_box.m_base_box.m_size;
    INFO("Sample to chunk box size is %u",
         m_sample_to_chunk.m_full_box.m_base_box.m_size);
  }
  if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
    m_chunk_offset.update_chunk_offset_box_size();
    m_base_box.m_size += m_chunk_offset.m_full_box.m_base_box.m_size;
    INFO("Chunk offset box size is %u",
         m_chunk_offset.m_full_box.m_base_box.m_size);
  }
  if (m_chunk_offset64.m_full_box.m_base_box.m_enable) {
    m_chunk_offset64.update_chunk_offset64_box_size();
    m_base_box.m_size += m_chunk_offset64.m_full_box.m_base_box.m_size;
    INFO("Chunk offset64 box size is %u",
         m_chunk_offset64.m_full_box.m_base_box.m_size);
  }
  if (m_sync_sample.m_full_box.m_base_box.m_enable) {
    m_sync_sample.update_sync_sample_box_size();
    m_base_box.m_size += m_sync_sample.m_full_box.m_base_box.m_size;
    INFO("Sync box size is %u",
         m_sync_sample.m_full_box.m_base_box.m_size);
  }
}

bool MediaInformationBox::write_media_info_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media information box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write media information box.");
      ret = false;
      break;
    }
    if (m_video_info_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_video_info_header.write_video_media_info_header_box(
          file_writer))) {
        ERROR("Failed to write video media information header box.");
        ret = false;
        break;
      }
    }
    if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sound_info_header.write_sound_media_info_header_box(
          file_writer))) {
        ERROR("Failed to write sound media information header box.");
        ret = false;
        break;
      }
    }
    if (m_null_info_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_null_info_header.write_null_media_info_header_box(
          file_writer))) {
        ERROR("Failed to write null media information header box.");
        ret = false;
        break;
      }
    }
    if (m_data_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_data_info.write_data_info_box(file_writer))) {
        ERROR("Failed to write data information box.");
        ret = false;
        break;
      }
    }
    if (m_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_table.write_sample_table_box(
          file_writer))) {
        ERROR("Failed to write sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::read_media_info_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read media information box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box.");
      ret = false;
      break;
    }
    do {
      DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case DISOM_VIDEO_MEDIA_HEADER_BOX_TAG: {
          if (!m_video_info_header.read_video_media_info_header_box(
              file_reader)) {
            ERROR("Failed to read video media information header box");
            ret = false;
          }
        } break;
        case DISOM_SOUND_MEDIA_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_sound_info_header.read_sound_media_info_header_box(
              file_reader))) {
            ERROR("Failed to read sound media information header box.");
            ret = false;
          }
        } break;
        case DISOM_NULL_MEDIA_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_null_info_header.read_null_media_info_header_box(
              file_reader))) {
            ERROR("Failed to read null media information header box.");
            ret = false;
          }
        } break;
        case DISOM_DATA_INFORMATION_BOX_TAG: {
          if (!m_data_info.read_data_info_box(file_reader)) {
            ERROR("Failed to read data information box.");
            ret = false;
          }
        } break;
        case DISOM_SAMPLE_TABLE_BOX_TAG: {
          if (!m_sample_table.read_sample_table_box(file_reader)) {
            ERROR("Failed to read sample table box.");
            ret = false;
          }
        } break;
        default: {
          ERROR("Get invalid type when read media information box.");
          ret = false;
        } break;
      }
      if (!file_reader->tell_file(offset_end)) {
        ERROR("Failed to tell file when read media information box.");
        ret = false;
        break;
      }
    } while (((offset_end - offset_begin) < m_base_box.m_size) && ret);
    if (!ret) {
      break;
    }
    m_base_box.m_enable = true;
    INFO("Read media information box success.");
  } while (0);
  return ret;
}

bool MediaInformationBox::get_proper_media_info_box(
    MediaInformationBox& box, SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_video_info_header.m_full_box.m_base_box.m_enable) {
      if (!m_video_info_header.get_proper_video_media_info_header_box(
          box.m_video_info_header, media_info)) {
        ERROR("Failed to get proper video media information header box.");
        ret = false;
        break;
      }
    }
    if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
      if (!m_sound_info_header.get_proper_sound_media_info_header_box(
          box.m_sound_info_header, media_info)) {
        ERROR("Failed to get proper sound media information header box.");
        ret = false;
        break;
      }
    }
    if (m_data_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_data_info.get_proper_data_info_box(
          box.m_data_info, media_info))) {
        ERROR("Failed to get proper data information box.");
        ret = false;
        break;
      }
    }
    if (m_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_table.get_proper_sample_table_box(
          box.m_sample_table, media_info))) {
        ERROR("Failed to get proper sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::copy_media_info_box(
    MediaInformationBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_video_info_header.m_full_box.m_base_box.m_enable) {
      if (!m_video_info_header.copy_video_media_info_header_box(
          box.m_video_info_header)) {
        ERROR("Failed to copy video media information header box.");
        ret = false;
        break;
      }
    }
    if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
      if (!m_sound_info_header.copy_sound_media_info_header_box(
          box.m_sound_info_header)) {
        ERROR("Failed to copy sound media information header box.");
        ret = false;
        break;
      }
    }
    if (m_data_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_data_info.copy_data_info_box(
          box.m_data_info))) {
        ERROR("Failed to copy data information box.");
        ret = false;
        break;
      }
    }
    if (m_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_table.copy_sample_table_box(
          box.m_sample_table))) {
        ERROR("Failed to copy sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool MediaInformationBox::get_combined_media_info_box(
    MediaInformationBox& source_box, MediaInformationBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined media information box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_video_info_header.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined video media information header box.");
      if (!m_video_info_header.get_combined_video_media_info_header_box(
          source_box.m_video_info_header, combined_box.m_video_info_header)) {
        ERROR("Get combined video media information header error.");
        ret = false;
        break;
      }
    }
    if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
      if (!m_sound_info_header.get_combined_sound_media_info_header_box(
          source_box.m_sound_info_header, combined_box.m_sound_info_header)) {
        ERROR("Get combined sound media information header error.");
        ret = false;
        break;
      }
    }
    if (m_data_info.m_base_box.m_enable) {
      INFO("begin to get combined data information box.");
      if (!m_data_info.get_combined_data_info_box(source_box.m_data_info,
                                                 combined_box.m_data_info)) {
        ERROR("Get combined data information box error.");
        ret = false;
        break;
      }
    }
    if (m_sample_table.m_base_box.m_enable) {
      INFO("begin to get combined sample table box.");
      if (!m_sample_table.get_combined_sample_table_box(
          source_box.m_sample_table, combined_box.m_sample_table)) {
        ERROR("Get combined sample box error.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

void MediaInformationBox::update_media_info_box_size()
{
  m_base_box.m_size = DISOM_BASE_BOX_SIZE;
  if (m_video_info_header.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_video_info_header.m_full_box.m_base_box.m_size;
    INFO("video information header size is %u",
         m_video_info_header.m_full_box.m_base_box.m_size);
  }
  if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_sound_info_header.m_full_box.m_base_box.m_size;
    INFO("sound information header size is %u",
         m_sound_info_header.m_full_box.m_base_box.m_size);
  }
  if (m_null_info_header.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_null_info_header.m_full_box.m_base_box.m_size;
  }
  if (m_data_info.m_base_box.m_enable) {
    m_base_box.m_size += m_data_info.m_base_box.m_size;
    INFO("Data info size is %u", m_data_info.m_base_box.m_size);
  }
  if (m_sample_table.m_base_box.m_enable) {
    m_sample_table.update_sample_table_box_size();
    m_base_box.m_size += m_sample_table.m_base_box.m_size;
    INFO("Sample table size is %u", m_sample_table.m_base_box.m_size);
  }
}

bool MediaBox::write_media_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write media box.");
      ret = false;
      break;
    }
    if (m_media_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_header.write_media_header_box(file_writer))) {
        ERROR("Failed to write media header box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media header box is not enabled.");
    }
    if (m_media_handler.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_handler.write_handler_reference_box(file_writer))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media handler box is not enabled.");
    }
    if (m_media_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_info.write_media_info_box(file_writer))) {
        ERROR("Failed to write media information box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media info box is not enabled.");
    }
    INFO("Write media box success.");
  } while (0);
  return ret;
}

bool MediaBox::read_media_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read media box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader)))) {
      ERROR("Failed to read media box.");
      ret = false;
      break;
    }
    do {
      DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case DISOM_MEDIA_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_media_header.read_media_header_box(file_reader))) {
            ERROR("Failed to read media header box.");
            ret = false;
          }
        } break;
        case DISOM_HANDLER_REFERENCE_BOX_TAG: {
          if (AM_UNLIKELY(!m_media_handler.read_handler_reference_box(file_reader))) {
            ERROR("Failed to read handler reference box.");
            ret = false;
          }
        } break;
        case DISOM_MEDIA_INFORMATION_BOX_TAG: {
          if (AM_UNLIKELY(!m_media_info.read_media_info_box(file_reader))) {
            ERROR("Failed to read media information box.");
            ret = false;
          }
        } break;
        default: {
          ERROR("Get invalid type when read media information box.");
          ret = false;
        } break;
      }
      if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
        ERROR("Failed to tell file when read media information box.");
        ret = false;
      }
    } while (((offset_end - offset_begin) < m_base_box.m_size) && ret);
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool MediaBox::get_proper_media_box(MediaBox& box,
                                    SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_media_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY((!m_media_header.get_proper_media_header_box(
          box.m_media_header, media_info)))) {
        ERROR("Failed to get proper media header box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media header is not enabed when get proper media box.");
    }
    if (m_media_handler.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_handler.get_proper_handler_reference_box(
          box.m_media_handler, media_info))) {
        ERROR("Failed to get proper handler reference box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media handler box is not enabled when get proper media box");
    }
    if (m_media_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_info.get_proper_media_info_box(
          box.m_media_info, media_info))) {
        ERROR("Failed to get proper media information box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media info box is not enabled when get proper media box.");
    }
  } while (0);
  return ret;
}

bool MediaBox::copy_media_box(MediaBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_media_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY((!m_media_header.copy_media_header_box(
          box.m_media_header)))) {
        ERROR("Failed to copy media header box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media header is not enabed when get proper media box.");
    }
    if (m_media_handler.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_handler.copy_handler_reference_box(
          box.m_media_handler))) {
        ERROR("Failed to copy handler reference box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media handler box is not enabled when get proper media box");
    }
    if (m_media_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_info.copy_media_info_box(
          box.m_media_info))) {
        ERROR("Failed to copy media information box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media info box is not enabled when get proper media box.");
    }
  } while (0);
  return ret;
}

bool MediaBox::get_combined_media_box(MediaBox& source_box,
                                      MediaBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined media box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_media_header.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined media header box.");
      if (AM_UNLIKELY(!m_media_header.get_combined_media_header_box(
          source_box.m_media_header, combined_box.m_media_header))) {
        ERROR("Get combined media header box error.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media header box is not enabled when get combined media"
          " box");
    }
    if (m_media_handler.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined handler reference box.");
      if (AM_UNLIKELY(!m_media_handler.get_combined_handler_reference_box(
          source_box.m_media_handler, combined_box.m_media_handler))) {
        ERROR("Get combined handler reference box error.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media handler box is not enabled when get combined "
          "media box.");
    }
    if (m_media_info.m_base_box.m_enable) {
      INFO("begin to get combined media information box.");
      if (AM_UNLIKELY(!m_media_info.get_combined_media_info_box(
          source_box.m_media_info, combined_box.m_media_info))) {
        ERROR("Get combined media information box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media info box is not enabled when get combined media box.");
    }
  } while (0);
  return ret;
}

void MediaBox::update_media_box_size()
{
  m_base_box.m_size = DISOM_BASE_BOX_SIZE;
  if (m_media_header.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_media_header.m_full_box.m_base_box.m_size;
    INFO("Media header size is %u",
         m_media_header.m_full_box.m_base_box.m_size);
  }
  if (m_media_handler.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_media_handler.m_full_box.m_base_box.m_size;
    INFO("Media handler size is %u",
         m_media_handler.m_full_box.m_base_box.m_size);
  }
  if (m_media_info.m_base_box.m_enable) {
    m_media_info.update_media_info_box_size();
    m_base_box.m_size += m_media_info.m_base_box.m_size;
    INFO("Media info size is %u", m_media_info.m_base_box.m_size);
  }
}

bool TrackBox::write_track_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The movie track box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write movie track box.");
      ret = false;
      break;
    }
    if (m_track_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_track_header.write_movie_track_header_box(
          file_writer))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    if (m_media.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media.write_media_box(file_writer))) {
        ERROR("Failed to write media box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool TrackBox::read_track_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read movie track box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader)))) {
      ERROR("Failed to read movie track box.");
      ret = false;
      break;
    }
    do {
      DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case DISOM_TRACK_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_track_header.read_movie_track_header_box(
              file_reader))) {
            ERROR("Failed to read movie track header box.");
            ret = false;
          }
        } break;
        case DISOM_MEDIA_BOX_TAG: {
          if (AM_UNLIKELY(!m_media.read_media_box(file_reader))) {
            ERROR("Failed to read media box.");
            ret = false;
          }
        } break;
        default: {
          ERROR("Get invalid type tag when read movie track box.");
          ret = false;
        } break;
      }
      if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
        ERROR("Failed to tell file when read movie track box.");
        ret = false;
        break;
      }
    } while (((offset_end - offset_begin) < m_base_box.m_size) && ret);
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool TrackBox::get_proper_track_box(TrackBox& box,
                                          SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_track_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_track_header.get_proper_track_header_box(
          box.m_track_header, media_info))) {
        ERROR("Failed to get proper track header box.");
        ret = false;
        break;
      }
    }
    if (m_media.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media.get_proper_media_box(box.m_media,
                                                    media_info))) {
        ERROR("Failed to get proper media box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool TrackBox::copy_track_box(TrackBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_track_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_track_header.copy_track_header_box(
          box.m_track_header))) {
        ERROR("Failed to copy track header box.");
        ret = false;
        break;
      }
    }
    if (m_media.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media.copy_media_box(box.m_media))) {
        ERROR("Failed to copy media box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool TrackBox::get_combined_track_box(TrackBox& source_box,
                                            TrackBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined track box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_track_header.m_full_box.m_base_box.m_enable) {
      INFO("begin to get combined track header box.");
      if (AM_UNLIKELY(!m_track_header.get_combined_track_header_box(
          source_box.m_track_header, combined_box.m_track_header))) {
        ERROR("Get combined track header box error.");
        ret = false;
        break;
      }
    }
    if (m_media.m_base_box.m_enable) {
      INFO("begin to get combined media box.");
      if (AM_UNLIKELY(!m_media.get_combined_media_box(source_box.m_media,
                                                      combined_box.m_media))) {
        ERROR("Failed to get combined movie track box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

void TrackBox::update_track_box_size()
{
  m_base_box.m_size = DISOM_BASE_BOX_SIZE;
  if (m_track_header.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_track_header.m_full_box.m_base_box.m_size;
    INFO("Track header size is %u",
         m_track_header.m_full_box.m_base_box.m_size);
  }
  if (m_media.m_base_box.m_enable) {
    m_media.update_media_box_size();
    m_base_box.m_size += m_media.m_base_box.m_size;
    INFO("Media box size is %u", m_media.m_base_box.m_size);
  }
}

bool MovieBox::write_movie_box(Mp4FileWriter* file_writer)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The movie box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer)))) {
      ERROR("Failed to write movie box.");
      ret = false;
      break;
    }
    if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_movie_header_box.write_movie_header_box(file_writer))) {
        ERROR("Failed to write movie header box.");
        ret = false;
        break;
      }
    }
    if (m_iods_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_iods_box.write_object_desc_box(file_writer))) {
        ERROR("Failed to write object desc box.");
        ret = false;
        break;
      }
    }
    if (m_video_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_video_track.write_track_box(file_writer))) {
        ERROR("Failed to write video track box.");
        ret = false;
        break;
      }
    } else {
      WARN("There is no video in this file");
    }
    if (m_audio_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_audio_track.write_track_box(file_writer))) {
        ERROR("Failed to write audio track box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("There is not audio in this file");
    }
    if (m_gps_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_gps_track.write_track_box(file_writer))) {
        ERROR("Failed to write gps track box.");
        ret = false;
        break;
      }
    } else {
      INFO("There is no gps in this file.");
    }
  } while (0);
  return ret;
}

bool MovieBox::read_movie_box(Mp4FileReader* file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (AM_UNLIKELY(!file_reader->tell_file(offset_begin))) {
      ERROR("Failed to tell file when read movie box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.read_base_box(file_reader)))) {
      ERROR("Failed to read movie box.");
      ret = false;
      break;
    }
    do {
      DISOM_BOX_TAG type = DISOM_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case DISOM_MOVIE_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_movie_header_box.read_movie_header_box(
              file_reader))) {
            ERROR("Failed to read movie header box.");
            ret = false;
          }
        } break;
        case DIDOM_OBJECT_DESC_BOX_TAG: {
          if (AM_UNLIKELY(!m_iods_box.read_object_desc_box(file_reader))) {
            ERROR("Failed to read object desc box.");
            ret = false;
          }
        } break;
        case DISOM_TRACK_BOX_TAG: {
          if (!read_track_box(file_reader)) {
            ERROR("Failed to read track box.");
            ret = false;
          }
        } break;
        default: {
          ERROR("Get invalid type tag when read movie box");
          ret = false;
        } break;
      }
      if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
        ERROR("Failed to tell file when read movie box.");
        ret = false;
        break;
      }
    } while (((offset_end - offset_begin) < m_base_box.m_size) && ret);
    m_base_box.m_enable = true;
  } while (0);
  return ret;
}

bool MovieBox::get_proper_movie_box(MovieBox& box,
                                    MediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_movie_header_box.get_proper_movie_header_box(
          box.m_movie_header_box, media_info))) {
        ERROR("Failed to get proper movie header box.");
        ret = false;
        break;
      }
    }
    if (m_video_track.m_base_box.m_enable) {
      SubMediaInfo sub_media_info;
      if (!media_info.get_video_sub_info(sub_media_info)) {
        ERROR("Failed to get video sub info");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!m_video_track.get_proper_track_box(
          box.m_video_track, sub_media_info))) {
        ERROR("Failed to get proper video track box.");
        ret = false;
        break;
      }
    }
    if (m_audio_track.m_base_box.m_enable) {
      SubMediaInfo sub_media_info;
      if (!media_info.get_audio_sub_info(sub_media_info)) {
        ERROR("Failed to get audio sub info");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!m_audio_track.get_proper_track_box(
          box.m_audio_track, sub_media_info))) {
        ERROR("Failed to get proper audio track box.");
        ret = false;
        break;
      }
    }
    if (m_gps_track.m_base_box.m_enable) {
      SubMediaInfo sub_media_info;
      if (!media_info.get_gps_sub_info(sub_media_info)) {
        ERROR("Failed to get gps sub info");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!m_gps_track.get_proper_track_box(
          box.m_gps_track, sub_media_info))) {
        ERROR("Failed to get proper gps track box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool MovieBox::copy_movie_box(MovieBox& box)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(box.m_base_box);
    if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_movie_header_box.copy_movie_header_box(
          box.m_movie_header_box))) {
        ERROR("Failed to copy movie header box.");
        ret = false;
        break;
      }
    }
    if (m_video_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_video_track.copy_track_box(
          box.m_video_track))) {
        ERROR("Failed to copy video track box.");
        ret = false;
        break;
      }
    }
    if (m_audio_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_audio_track.copy_track_box(
          box.m_audio_track))) {
        ERROR("Failed to copy audio track box.");
        ret = false;
        break;
      }
    }
    if (m_gps_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_gps_track.copy_track_box(
          box.m_gps_track))) {
        ERROR("Failed to copy gps track box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool MovieBox::get_combined_movie_box(MovieBox& source_box,
                                      MovieBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined movie box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_movie_header_box.get_combined_movie_header_box(
          source_box.m_movie_header_box, combined_box.m_movie_header_box))) {
        ERROR("Failed to get combined movie header box.");
        ret = false;
        break;
      }
    }
    if (m_video_track.m_base_box.m_enable) {
      INFO("begin to get combined video movie track box.");
      if (AM_UNLIKELY(!m_video_track.get_combined_track_box(
          source_box.m_video_track, combined_box.m_video_track))) {
        ERROR("Failed to get combined video movie track box.");
        ret = false;
        break;
      }
    }
    if (m_audio_track.m_base_box.m_enable) {
      INFO("begin to get combined audio movie track box.");
      if (AM_UNLIKELY(!m_audio_track.get_combined_track_box(
          source_box.m_audio_track, combined_box.m_audio_track))) {
        ERROR("Failed to get combined audio movie track box.");
        ret = false;
        break;
      }
    }
    if (m_gps_track.m_base_box.m_enable) {
      INFO("begin to get combined gps movie track box.");
      if (AM_UNLIKELY(!m_gps_track.get_combined_track_box(
          source_box.m_gps_track, combined_box.m_gps_track))) {
        ERROR("Failed to get combined gps movie track box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

void MovieBox::update_movie_box_size()
{
  m_base_box.m_size = DISOM_BASE_BOX_SIZE;
  if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_movie_header_box.m_full_box.m_base_box.m_size;
    INFO("movie header box is %u",
         m_movie_header_box.m_full_box.m_base_box.m_size);
  }
  if (m_video_track.m_base_box.m_enable) {
    m_video_track.update_track_box_size();
    m_base_box.m_size += m_video_track.m_base_box.m_size;
    INFO("video track size is %u", m_video_track.m_base_box.m_size);
  }
  if (m_audio_track.m_base_box.m_enable) {
    m_audio_track.update_track_box_size();
    m_base_box.m_size += m_audio_track.m_base_box.m_size;
    INFO("audio track size is %u", m_audio_track.m_base_box.m_size);
  }
  if (m_gps_track.m_base_box.m_enable) {
    m_gps_track.update_track_box_size();
    m_base_box.m_size += m_gps_track.m_base_box.m_size;
    INFO("audio track size is %u", m_gps_track.m_base_box.m_size);
  }
}

bool MovieBox::read_track_box(Mp4FileReader *file_reader)
{
  bool ret = true;
  do {
    TrackBox tmp_track;
    if (AM_UNLIKELY(!tmp_track.read_track_box(file_reader))) {
      ERROR("Failed to read movie track box.");
      ret = false;
      break;
    }
    switch (tmp_track.m_media.m_media_handler.m_handler_type) {
      case VIDEO_TRACK: {
        if (!tmp_track.copy_track_box(m_video_track)) {
          ERROR("Failed to copy video track box.");
          ret = false;
          break;
        }
      } break;
      case AUDIO_TRACK: {
        if (!tmp_track.copy_track_box(m_audio_track)) {
          ERROR("Failed to copy audio track box.");
          ret = false;
          break;
        }
      } break;
      case HINT_TRACK: {
        WARN("The hint track is not supported currently.");
      } break;
      case TIMED_METADATA_TRACK: {
        if (!tmp_track.copy_track_box(m_gps_track)) {
          ERROR("Failed to copy movie track box.");
          ret = false;
          break;
        }
      } break;
      case AUXILIARY_VIDEO_TRACK: {
        WARN("The auxiliary track is not supported currently.");
      } break;
      default: {
        ERROR("Get invalid track tag when read movie box.");
        ret = false;
      } break;
    }
  } while (0);
  return ret;
}
