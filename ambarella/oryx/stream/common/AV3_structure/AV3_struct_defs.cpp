/*******************************************************************************
 *  av3_struct_defs.cpp
 *
 * History:
 *   2016-08-24 - [ccjing] created file
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
#include "am_define.h"
#include "am_log.h"
#include "AV3_file_writer.h"
#include "AV3_file_reader.h"
#include "AV3_struct_defs.h"

bool AV3MediaInfo::set_video_duration(uint64_t video_duration)
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

bool AV3MediaInfo::set_audio_duration(uint64_t audio_duration)
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

bool AV3MediaInfo::set_gps_duration(uint64_t gps_duration)
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

bool AV3MediaInfo::set_video_first_frame(uint32_t video_first_frame)
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

bool AV3MediaInfo::set_video_last_frame(uint32_t video_last_frame)
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

bool AV3MediaInfo::set_audio_first_frame(uint32_t audio_first_frame)
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

bool AV3MediaInfo::set_audio_last_frame(uint32_t audio_last_frame)
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

bool AV3MediaInfo::set_gps_first_frame(uint32_t gps_first_frame)
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

bool AV3MediaInfo::set_gps_last_frame(uint32_t gps_last_frame)
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

uint64_t AV3MediaInfo::get_video_duration()
{
  return m_video_duration;
}

uint64_t AV3MediaInfo::get_audio_duration()
{
  return m_audio_duration;
}

uint64_t AV3MediaInfo::get_gps_duration()
{
  return m_gps_duration;
}

uint32_t AV3MediaInfo::get_video_first_frame()
{
  return m_video_first_frame;
}

uint32_t AV3MediaInfo::get_video_last_frame()
{
  return m_video_last_frame;
}

uint32_t AV3MediaInfo::get_audio_first_frame()
{
  return m_audio_first_frame;
}

uint32_t AV3MediaInfo::get_audio_last_frame()
{
  return m_audio_last_frame;
}

uint32_t AV3MediaInfo::get_gps_first_frame()
{
  return m_gps_first_frame;
}

uint32_t AV3MediaInfo::get_gps_last_frame()
{
  return m_gps_last_frame;
}

bool AV3MediaInfo::get_video_sub_info(AV3SubMediaInfo& video_sub_info)
{
  bool ret = true;
  video_sub_info.set_duration(m_video_duration);
  video_sub_info.set_first_frame(m_video_first_frame);
  video_sub_info.set_last_frame(m_video_last_frame);
  return ret;
}

bool AV3MediaInfo::get_audio_sub_info(AV3SubMediaInfo& audio_sub_info)
{
  bool ret = true;
  audio_sub_info.set_duration(m_audio_duration);
  audio_sub_info.set_first_frame(m_audio_first_frame);
  audio_sub_info.set_last_frame(m_audio_last_frame);
  return ret;
}

bool AV3MediaInfo::get_gps_sub_info(AV3SubMediaInfo& gps_sub_info)
{
  bool ret = true;
  gps_sub_info.set_duration(m_gps_duration);
  gps_sub_info.set_first_frame(m_gps_first_frame);
  gps_sub_info.set_last_frame(m_gps_last_frame);
  return ret;
}

AV3MediaInfo::AV3MediaInfo()
{}

bool AV3SubMediaInfo::set_duration(uint64_t duration)
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

AV3SubMediaInfo::AV3SubMediaInfo()
{}

bool AV3SubMediaInfo::set_first_frame(uint32_t first_frame)
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

bool AV3SubMediaInfo::set_last_frame(uint32_t last_frame)
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

uint64_t AV3SubMediaInfo::get_duration()
{
  return m_duration;
}

uint32_t AV3SubMediaInfo::get_first_frame()
{
  return m_first_frame;
}

uint32_t AV3SubMediaInfo::get_last_frame()
{
  return m_last_frame;
}

AV3BaseBox::AV3BaseBox()
{
}

bool AV3BaseBox::write_base_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_size, copy_to_mem))) {
      ERROR("Write base box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )&m_type,
                                             sizeof(m_type), copy_to_mem))) {
      ERROR("Write base box error.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AV3BaseBox::read_base_box(AV3FileReader *file_reader)
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
    INFO("Read base box size success, size is %u, type is %s", m_size, type);
    m_type = AV3_BOX_TAG(AV3_BOX_TAG_CREATE(type[0], type[1],
                                               type[2], type[3]));
  } while (0);
  return ret;
}

void AV3BaseBox::copy_base_box(AV3BaseBox& box)
{
  box.m_enable = m_enable;
  box.m_size = m_size;
  box.m_type = m_type;
}

AV3FullBox::AV3FullBox()
{
}

bool AV3FullBox::write_full_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write base box in the full box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(((uint32_t ) m_version << 24) |
                                               ((uint32_t )m_flags),
                                               copy_to_mem))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AV3FullBox::read_full_box(AV3FileReader *file_reader)
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

void AV3FullBox::copy_full_box(AV3FullBox& box)
{
  m_base_box.copy_base_box(box.m_base_box);
  box.m_flags = m_flags;
  box.m_version = m_version;
}

AV3FileTypeBox::AV3FileTypeBox()
{
}

bool AV3FileTypeBox::write_file_type_box(AV3FileWriter* file_writer,
                                         bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The file type box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write base box in file type box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t*)&m_major_brand,
                                             AV3_TAG_SIZE, copy_to_mem))) {
      ERROR("Failed to write major brand in file type box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_minor_version, copy_to_mem))) {
      ERROR("Failed to write minor version in file type box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_compatible_brands_number; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_data(
          (uint8_t*)&m_compatible_brands[i], AV3_TAG_SIZE, copy_to_mem))) {
        ERROR("Failed to write file type box");
        ret = false;
        break;
      }
    }
    INFO("write file type box success.");
  } while (0);
  return ret;
}

bool AV3FileTypeBox::read_file_type_box(AV3FileReader *file_reader)
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
    m_major_brand = AV3_BRAND_TAG(AV3_BOX_TAG_CREATE(
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
         AV3_MAX_COMPATIBLE_BRANDS) {
      ERROR("compatible brands number is larger than %u, error.",
            AV3_MAX_COMPATIBLE_BRANDS);
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
      m_compatible_brands[i] = AV3_BRAND_TAG(
          AV3_BOX_TAG_CREATE(compatible_brands[0], compatible_brands[1],
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

void AV3FileTypeBox::copy_file_type_box(AV3FileTypeBox& box)
{
  m_base_box.copy_base_box(box.m_base_box);
  box.m_compatible_brands_number = m_compatible_brands_number;
  box.m_minor_version = m_minor_version;
  box.m_major_brand = m_major_brand;
  memcpy(box.m_compatible_brands, m_compatible_brands,
         AV3_MAX_COMPATIBLE_BRANDS * sizeof(m_compatible_brands[0]));
}

bool AV3MediaDataBox::write_media_data_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media data box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write media data box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_flags, copy_to_mem))) {
      ERROR("Failed to write m_falg in media data box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_data((uint8_t*)m_hash, sizeof(m_hash),
                                             copy_to_mem))) {
      ERROR("Failed to write hash data in media data box.");
      ret = false;
      break;
    }
    INFO("write media data box success.");
  } while (0);
  return ret;
}

bool AV3MediaDataBox::read_media_data_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box when read media data box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_flags))) {
      ERROR("Failed to read flags value in media data box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_data((uint8_t*)m_hash, sizeof(m_hash)))) {
      ERROR("Failed to read hash value in media data box.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read media data box success.");
  } while (0);
  return ret;
}

AV3MovieHeaderBox::AV3MovieHeaderBox()
{}

bool AV3MovieHeaderBox::write_movie_header_box(AV3FileWriter* file_writer,
                                               bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The movie header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write base full box in movie header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time,
                                                copy_to_mem))
                    || (!file_writer->write_be_u32(
                        (uint32_t )m_modification_time, copy_to_mem))
                    || (!file_writer->write_be_u32(m_timescale, copy_to_mem))
                    || (!file_writer->write_be_u32((uint32_t )m_duration,
                                                   copy_to_mem)))) {
      ERROR("Failed to write movie header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_rate, copy_to_mem))
                 || (!file_writer->write_be_u16(m_volume, copy_to_mem))
                 || (!file_writer->write_be_u16(m_reserved, copy_to_mem))
                 || (!file_writer->write_be_u32(m_reserved_1, copy_to_mem))
                 || (!file_writer->write_be_u32(m_reserved_2, copy_to_mem)))) {
      ERROR("Failed to write movie header box");
      ret = false;
      break;
    }

    for (uint32_t i = 0; i < 9; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_matrix[i], copy_to_mem))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    for (uint32_t i = 0; i < 6; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_pre_defined[i],
                                                 copy_to_mem))) {
        ERROR("Failed to write movie header box");
        ret = false;
        return ret;
      }
    }

    if (AM_UNLIKELY(!file_writer->write_be_u32(m_next_track_ID, copy_to_mem))) {
      ERROR("Failed to write movie header box");
      ret = false;
      break;
    }
    INFO("write movie header box success.");
  } while (0);
  return ret;
}

bool AV3MovieHeaderBox::read_movie_header_box(AV3FileReader* file_reader)
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
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_creation_time))
                    || (!file_reader->read_le_u32(m_modification_time))
                    || (!file_reader->read_le_u32(m_timescale))
                    || (!file_reader->read_le_u32(m_duration)))) {
      ERROR("Failed to read movie header box");
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

bool AV3MovieHeaderBox::get_proper_movie_header_box(
    AV3MovieHeaderBox& box, AV3MediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_creation_time = m_creation_time;
    box.m_modification_time = m_modification_time;
    uint64_t video_duration = media_info.get_video_duration();
    uint64_t audio_duration = media_info.get_audio_duration();
    box.m_duration =  (video_duration > audio_duration) ?
                       (uint32_t)video_duration : (uint32_t)audio_duration;
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

bool AV3MovieHeaderBox::copy_movie_header_box(AV3MovieHeaderBox& box)
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

bool AV3MovieHeaderBox::get_combined_movie_header_box(AV3MovieHeaderBox& source_box,
                                                      AV3MovieHeaderBox& combined_box)
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

AV3TrackHeaderBox::AV3TrackHeaderBox()
{
}

bool AV3TrackHeaderBox::write_movie_track_header_box(AV3FileWriter* file_writer,
                                                     bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The movie track header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32((uint32_t )m_creation_time,
                                                copy_to_mem))
                    || (!file_writer->write_be_u32((uint32_t )m_modification_time,
                                                   copy_to_mem))
                    || (!file_writer->write_be_u32(m_track_ID, copy_to_mem))
                    || (!file_writer->write_be_u32(m_reserved, copy_to_mem))
                    || (!file_writer->write_be_u32((uint32_t )m_duration,
                                                   copy_to_mem)))) {
      ERROR("Failed to write movie track header box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved_1[i], copy_to_mem))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_layer, copy_to_mem))
                 || (!file_writer->write_be_u16(m_alternate_group, copy_to_mem))
                 || (!file_writer->write_be_u16(m_volume, copy_to_mem))
                 || (!file_writer->write_be_u16(m_reserved_2, copy_to_mem)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 9; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_matrix[i], copy_to_mem))) {
        ERROR("Failed to write movie track header box");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_width_integer, copy_to_mem))
             || (!file_writer->write_be_u16(m_width_decimal, copy_to_mem)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_height_integer, copy_to_mem))
                    || (!file_writer->write_be_u16(m_height_decimal,
                                                   copy_to_mem)))) {
      ERROR("Failed to write movie track header box");
      ret = false;
      break;
    }
    INFO("write movie track header box success.");
  } while (0);
  return ret;
}

bool AV3TrackHeaderBox::read_movie_track_header_box(AV3FileReader* file_reader)
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
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_creation_time))
                    || (!file_reader->read_le_u32(m_modification_time))
                    || (!file_reader->read_le_u32(m_track_ID))
                    || (!file_reader->read_le_u32(m_reserved))
                    || (!file_reader->read_le_u32(m_duration)))) {
      ERROR("Failed to read movie track header box.");
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

bool AV3TrackHeaderBox::get_proper_track_header_box(
    AV3TrackHeaderBox& track_header_box, AV3SubMediaInfo& sub_media_info)
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

bool AV3TrackHeaderBox::copy_track_header_box(AV3TrackHeaderBox& track_header_box)
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

bool AV3TrackHeaderBox::get_combined_track_header_box(AV3TrackHeaderBox& source_box,
                                                      AV3TrackHeaderBox& combined_box)
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
AV3MediaHeaderBox::AV3MediaHeaderBox()
{}

bool AV3MediaHeaderBox::write_media_header_box(AV3FileWriter* file_writer,
                                               bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The media header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_creation_time, copy_to_mem))
                    || (!file_writer->write_be_u32(m_modification_time,
                                                   copy_to_mem))
                    || (!file_writer->write_be_u32(m_timescale, copy_to_mem))
                    || (!file_writer->write_be_u32(m_duration, copy_to_mem)))) {
      ERROR("Failed to write media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_language, copy_to_mem))) {
      ERROR("Failed to writer m_language in media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_pre_defined, copy_to_mem))) {
      ERROR("Failed to write pre define value in media header box.");
      ret = false;
      break;
    }
    INFO("write media header box success.");
  } while (0);
  return ret;
}

bool AV3MediaHeaderBox::read_media_header_box(AV3FileReader* file_reader)
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
    if (AM_UNLIKELY((!file_reader->read_le_u32(m_creation_time))
                    || (!file_reader->read_le_u32(m_modification_time))
                    || (!file_reader->read_le_u32(m_timescale))
                    || (!file_reader->read_le_u32(m_duration)))) {
      ERROR("Failed to read media header box.");
      ret = true;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_language))) {
      ERROR("Failed to read media header box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_pre_defined))) {
      ERROR("Failed to read media header box");
      ret = false;
      break;
    }
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

bool AV3MediaHeaderBox::get_proper_media_header_box(
    AV3MediaHeaderBox& media_header_box, AV3SubMediaInfo& sub_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(media_header_box.m_full_box);
    media_header_box.m_creation_time = m_creation_time;
    media_header_box.m_modification_time = m_modification_time;
    media_header_box.m_duration = sub_media_info.get_duration();
    media_header_box.m_timescale = m_timescale;
    media_header_box.m_pre_defined = m_pre_defined;
    media_header_box.m_language = m_language;
  } while (0);
  return ret;
}

bool AV3MediaHeaderBox::copy_media_header_box(AV3MediaHeaderBox& media_header_box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(media_header_box.m_full_box);
    media_header_box.m_creation_time = m_creation_time;
    media_header_box.m_modification_time = m_modification_time;
    media_header_box.m_duration = m_duration;
    media_header_box.m_timescale = m_timescale;
    media_header_box.m_pre_defined = m_pre_defined;
    media_header_box.m_language = m_language;
  } while (0);
  return ret;
}

bool AV3MediaHeaderBox::get_combined_media_header_box(AV3MediaHeaderBox& source_box,
                                                      AV3MediaHeaderBox& combined_box)
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
    combined_box.m_language = m_language;
  } while (0);
  return ret;
}

AV3HandlerReferenceBox::AV3HandlerReferenceBox()
{
}

AV3HandlerReferenceBox::~AV3HandlerReferenceBox()
{
  delete[] m_name;
}

bool AV3HandlerReferenceBox::write_handler_reference_box(
    AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The handler reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer, copy_to_mem)) ||
                    (!file_writer->write_be_u32(m_pre_defined, copy_to_mem)) ||
                    (!file_writer->write_data((uint8_t*)&m_handler_type,
                                              AV3_TAG_SIZE, copy_to_mem)))) {
      ERROR("Failed to write hander reference box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved[i], copy_to_mem))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (m_name && (m_name_size > 0)) {
      if (AM_UNLIKELY(!file_writer->write_data((uint8_t* )(m_name),
                                               m_name_size, copy_to_mem))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    }
    INFO("write handler reference box success.");
  } while (0);
  return ret;
}

bool AV3HandlerReferenceBox::read_handler_reference_box(AV3FileReader* file_reader)
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
    m_handler_type = AV3_HANDLER_TYPE(handler_type);
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
                       - AV3_HANDLER_REF_SIZE;
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

bool AV3HandlerReferenceBox::get_proper_handler_reference_box(
    AV3HandlerReferenceBox& handler_reference_box, AV3SubMediaInfo& sub_media_info)
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

bool AV3HandlerReferenceBox::copy_handler_reference_box(
    AV3HandlerReferenceBox& handler_reference_box)
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

bool AV3HandlerReferenceBox::get_combined_handler_reference_box(
    AV3HandlerReferenceBox& source_box, AV3HandlerReferenceBox& combined_box)
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

AV3VideoMediaInfoHeaderBox::AV3VideoMediaInfoHeaderBox()
{}

bool AV3VideoMediaInfoHeaderBox::write_video_media_info_header_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The video media information header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write video media header box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_graphicsmode, copy_to_mem))
                 || (!file_writer->write_be_u16(m_opcolor[0], copy_to_mem))
                 || (!file_writer->write_be_u16(m_opcolor[1], copy_to_mem))
                 || (!file_writer->write_be_u16(m_opcolor[2], copy_to_mem)))) {
      ERROR("Failed to write video media header box.");
      ret = false;
      break;
    }
    INFO("write video media header box success.");
  } while (0);
  return ret;
}

bool AV3VideoMediaInfoHeaderBox::read_video_media_info_header_box(
     AV3FileReader* file_reader)
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

bool AV3VideoMediaInfoHeaderBox::get_proper_video_media_info_header_box(
     AV3VideoMediaInfoHeaderBox& box, AV3SubMediaInfo& video_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*) box.m_opcolor, m_opcolor, 3 * sizeof(uint16_t));
  } while (0);
  return ret;
}

bool AV3VideoMediaInfoHeaderBox::copy_video_media_info_header_box(
     AV3VideoMediaInfoHeaderBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_graphicsmode = m_graphicsmode;
    memcpy((uint8_t*) box.m_opcolor, m_opcolor, 3 * sizeof(uint16_t));
  } while (0);
  return ret;
}

bool AV3VideoMediaInfoHeaderBox::get_combined_video_media_info_header_box(
     AV3VideoMediaInfoHeaderBox& source_box,
     AV3VideoMediaInfoHeaderBox& combined_box)
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

AV3SoundMediaInfoHeaderBox::AV3SoundMediaInfoHeaderBox()
{
}

bool AV3SoundMediaInfoHeaderBox::write_sound_media_info_header_box(
    AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sound media information header box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_balanse, copy_to_mem))
                 || (!file_writer->write_be_u16(m_reserved, copy_to_mem)))) {
      ERROR("Failed to write sound media header box.");
      ret = false;
      break;
    }
    INFO("Write sound media information header box success.");
  } while (0);
  return ret;
}

bool AV3SoundMediaInfoHeaderBox::read_sound_media_info_header_box(
    AV3FileReader* file_reader)
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

bool AV3SoundMediaInfoHeaderBox::get_proper_sound_media_info_header_box(
    AV3SoundMediaInfoHeaderBox& box, AV3SubMediaInfo& audio_media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_balanse = m_balanse;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool AV3SoundMediaInfoHeaderBox::copy_sound_media_info_header_box(
     AV3SoundMediaInfoHeaderBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_balanse = m_balanse;
    box.m_reserved = m_reserved;
  } while (0);
  return ret;
}

bool AV3SoundMediaInfoHeaderBox::get_combined_sound_media_info_header_box(
     AV3SoundMediaInfoHeaderBox& source_box,
     AV3SoundMediaInfoHeaderBox& combined_box)
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

bool AV3DataEntryBox::write_data_entry_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The data entry box of reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write full box when write data entry box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AV3DataEntryBox::read_data_entry_box(AV3FileReader *file_reader)
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

bool AV3DataEntryBox::get_proper_data_entry_box(AV3DataEntryBox& box,
                                                AV3SubMediaInfo& media_info)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool AV3DataEntryBox::copy_data_entry_box(AV3DataEntryBox& box)
{
  bool ret = true;
  m_full_box.copy_full_box(box.m_full_box);
  return ret;
}

bool AV3DataEntryBox::get_combined_data_entry_box(AV3DataEntryBox& source_box,
                                                  AV3DataEntryBox& combined_box)
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

AV3DataReferenceBox::AV3DataReferenceBox()
{}

bool AV3DataReferenceBox::write_data_reference_box(AV3FileWriter* file_writer,
                                                   bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The data reference box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer, copy_to_mem))
        || (!file_writer->write_be_u32(m_entry_count, copy_to_mem)))) {
      ERROR("Failed to write data reference box.");
      ret = true;
      break;
    }
    if (m_url.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_url.write_data_entry_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write data reference box.");
        ret = false;
        break;
      }
    }
    INFO("read reference box success.");
  } while (0);
  return ret;
}

bool AV3DataReferenceBox::read_data_reference_box(AV3FileReader* file_reader)
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

bool AV3DataReferenceBox::get_proper_data_reference_box(
    AV3DataReferenceBox& box, AV3SubMediaInfo& media_info)
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

bool AV3DataReferenceBox::copy_data_reference_box(AV3DataReferenceBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    m_url.copy_data_entry_box(box.m_url);
  } while (0);
  return ret;
}

bool AV3DataReferenceBox::get_combined_data_reference_box(
    AV3DataReferenceBox& source_box, AV3DataReferenceBox& combined_box)
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

bool AV3DataInformationBox::write_data_info_box(AV3FileWriter* file_writer,
                                                bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The data information box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write data infomation box.");
      ret = false;
      break;
    }
    if (m_data_ref.m_full_box.m_base_box.m_enable) {
      if (!m_data_ref.write_data_reference_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write data reference box");
        ret = false;
        break;
      }
    }
    INFO("write data information box success.");
  } while (0);
  return ret;
}

bool AV3DataInformationBox::read_data_info_box(AV3FileReader* file_reader)
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

bool AV3DataInformationBox::get_proper_data_info_box(
    AV3DataInformationBox& box, AV3SubMediaInfo& media_info)
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

bool AV3DataInformationBox::copy_data_info_box(AV3DataInformationBox& box)
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

bool AV3DataInformationBox::get_combined_data_info_box(
    AV3DataInformationBox& source_box, AV3DataInformationBox& combined_box)
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

AV3DecodingTimeToSampleBox::AV3DecodingTimeToSampleBox()
{}

AV3DecodingTimeToSampleBox::~AV3DecodingTimeToSampleBox()
{
  delete[] m_entry_ptr;
}

bool AV3DecodingTimeToSampleBox::write_decoding_time_to_sample_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The decoding time to sample box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write decoding time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count, copy_to_mem))) {
      ERROR("Failed to write decoding time to sample box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY((!file_writer->write_be_u32(m_entry_ptr[i].sample_count,
                                                  copy_to_mem))
            || (!file_writer->write_be_s32(m_entry_ptr[i].sample_delta,
                                           copy_to_mem)))) {
        ERROR("Failed to write decoding time to sample box.");
        ret = false;
        break;
      }
    }
    INFO("write decoding time to sample box  success.");
  } while (0);
  return ret;
}

bool AV3DecodingTimeToSampleBox::read_decoding_time_to_sample_box(
     AV3FileReader* file_reader)
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
      m_entry_ptr = new AV3TimeEntry[m_entry_count];
      if (AM_UNLIKELY(!m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_entry_ptr[i].sample_count))
            || (!file_reader->read_le_s32(m_entry_ptr[i].sample_delta)))) {
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

bool AV3DecodingTimeToSampleBox::optimize_time_entry()
{
  bool ret = true;
  do {
    AV3TimeEntry *tmp_entry = nullptr;
    tmp_entry = new AV3TimeEntry[m_entry_count + 1];
    if (!tmp_entry) {
      ERROR("Failed to malloc memory for decoding time to sample box.");
      ret = false;
      break;
    }
    m_entry_buf_count = m_entry_count + 1;
    uint32_t tmp_entry_count = 0;
    tmp_entry[0].sample_delta = m_entry_ptr[0].sample_delta;
    tmp_entry[0].sample_count = m_entry_ptr[0].sample_count;
    for (uint32_t i = 1; i < m_entry_count; ++ i) {
      if (m_entry_ptr[i].sample_delta == tmp_entry[tmp_entry_count].sample_delta) {
        tmp_entry[tmp_entry_count].sample_count += m_entry_ptr[i].sample_count;
      } else {
        ++ tmp_entry_count;
        tmp_entry[tmp_entry_count].sample_delta = m_entry_ptr[i].sample_delta;
        tmp_entry[tmp_entry_count].sample_count = m_entry_ptr[i].sample_count;
      }
    }
    delete[] m_entry_ptr;
    m_entry_ptr = tmp_entry;
    m_entry_count = tmp_entry_count + 1;
  } while(0);
  return ret;
}

void AV3DecodingTimeToSampleBox::update_decoding_time_to_sample_box_size()
{
  if (!optimize_time_entry()) {
    ERROR("Failed to optimize time entry for decoding time to sample box");
  }
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * AV3_TIME_ENTRY_SIZE;
}

bool AV3DecodingTimeToSampleBox::get_proper_decoding_time_to_sample_box(
    AV3DecodingTimeToSampleBox& box, AV3SubMediaInfo& sub_media_info)
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
      box.m_entry_ptr = new AV3TimeEntry[box.m_entry_count];
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

bool AV3DecodingTimeToSampleBox::copy_decoding_time_to_sample_box(
     AV3DecodingTimeToSampleBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new AV3TimeEntry[box.m_entry_count];
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

bool AV3DecodingTimeToSampleBox::get_combined_decoding_time_to_sample_box(
     AV3DecodingTimeToSampleBox& source_box,
     AV3DecodingTimeToSampleBox& combined_box)
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
      combined_box.m_entry_ptr = new AV3TimeEntry[combined_box.m_entry_buf_count];
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

AV3CompositionTimeToSampleBox::AV3CompositionTimeToSampleBox()
{}

AV3CompositionTimeToSampleBox::~AV3CompositionTimeToSampleBox()
{
  delete[] m_entry_ptr;
}

bool AV3CompositionTimeToSampleBox::write_composition_time_to_sample_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The composition time to sample box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count, copy_to_mem))) {
      ERROR("Failed to write composition time to sample box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_ptr[i].sample_count,
                                                 copy_to_mem)))
      {
        ERROR("Failed to write sample count of composition time to sample box");
        ret = false;
        break;
      }
      if (AM_UNLIKELY(!file_writer->write_be_s32(m_entry_ptr[i].sample_delta,
                                                 copy_to_mem)))
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

bool AV3CompositionTimeToSampleBox::read_composition_time_to_sample_box(
     AV3FileReader* file_reader)
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
      m_entry_ptr = new AV3TimeEntry[m_entry_count];
      if (AM_UNLIKELY(!m_entry_ptr)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      NOTICE("composition time to sample box entry count is %u", m_entry_count);
      for (uint32_t i = 0; i < m_entry_count; ++ i) {
        if (AM_UNLIKELY((!file_reader->read_le_u32(m_entry_ptr[i].sample_count))
            || (!file_reader->read_le_s32(m_entry_ptr[i].sample_delta)))) {
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

void AV3CompositionTimeToSampleBox::update_composition_time_to_sample_box_size()
{
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * AV3_TIME_ENTRY_SIZE;
}

bool AV3CompositionTimeToSampleBox::get_proper_composition_time_to_sample_box(
     AV3CompositionTimeToSampleBox& box, AV3SubMediaInfo& sub_media_info)
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
      box.m_entry_ptr = new AV3TimeEntry[box.m_entry_count];
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

bool AV3CompositionTimeToSampleBox::copy_composition_time_to_sample_box(
     AV3CompositionTimeToSampleBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_entry_count = m_entry_count;
    box.m_entry_buf_count = box.m_entry_count;
    if (box.m_entry_count > 0) {
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = new AV3TimeEntry[box.m_entry_count];
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

bool AV3CompositionTimeToSampleBox::get_combined_composition_time_to_sample_box(
     AV3CompositionTimeToSampleBox& source_box,
     AV3CompositionTimeToSampleBox& combined_box)
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
      combined_box.m_entry_ptr = new AV3TimeEntry[combined_box.m_entry_buf_count];
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

AV3AVCDecoderConfigurationRecord::AV3AVCDecoderConfigurationRecord()
{}

AV3AVCDecoderConfigurationRecord::~AV3AVCDecoderConfigurationRecord()
{
  delete[] m_pps;
  delete[] m_sps;
}

bool AV3AVCDecoderConfigurationRecord::write_avc_decoder_configuration_record_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The avc decoder configuration box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem))
                  || (!file_writer->write_be_u8(m_config_version, copy_to_mem)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_profile_indication, copy_to_mem))
                 || (!file_writer->write_be_u8(m_profile_compatibility, copy_to_mem))
                 || (!file_writer->write_be_u8(m_level_indication, copy_to_mem)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(0xfc | m_len_size_minus_one,
                                              copy_to_mem))) {
      ERROR("Failed to write len size minus one");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(0xe0 | m_sps_num, copy_to_mem))) {
      ERROR("Failed to write sps number");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_sps_len, copy_to_mem))
        || (!file_writer->write_data(m_sps, m_sps_len, copy_to_mem))
        || (!file_writer->write_be_u8(m_pps_num, copy_to_mem))
        || (!file_writer->write_be_u16(m_pps_len, copy_to_mem))
        || (!file_writer->write_data(m_pps, m_pps_len, copy_to_mem)))) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = false;
      break;
    }
    if (m_padding_count > 0) {
      for (uint32_t i = 0; i< m_padding_count; ++ i) {
        if (!file_writer->write_be_u8(0, copy_to_mem)) {
          ERROR("Failed to write padding in avc decoder configuration record box.");
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
    INFO("write avc decoder configuration record box.");
  } while (0);
  return ret;
}

bool AV3AVCDecoderConfigurationRecord::read_avc_decoder_configuration_record_box(
     AV3FileReader* file_reader)
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
    m_len_size_minus_one = value & 0x03;
    value = 0;
    if (AM_UNLIKELY(!file_reader->read_le_u8(value))) {
      ERROR("Failed to read data when read decoder configuration record box.");
      ret = false;
      break;
    }
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
    m_padding_count = m_base_box.m_size - (offset_end - offset_begin);
    if ((m_padding_count >= 0) && (m_padding_count < 4)) {
      if (!file_reader->advance_file(m_padding_count)) {
        ERROR("Failed to advance file when read avcC box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The padding count is error in avcC box.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
    INFO("read decoder configuration record box success.");
  } while (0);
  return ret;
}

bool AV3AVCDecoderConfigurationRecord::
   get_proper_avc_decoder_configuration_record_box(
   AV3AVCDecoderConfigurationRecord& box, AV3SubMediaInfo& sub_media_info)
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
      box.m_len_size_minus_one = m_len_size_minus_one;
      box.m_sps_num = m_sps_num;
      box.m_pps_num = m_pps_num;
      box.m_padding_count = m_padding_count;
    }
  } while (0);
  return ret;
}

bool AV3AVCDecoderConfigurationRecord::
     copy_avc_decoder_configuration_record_box(
         AV3AVCDecoderConfigurationRecord& box)
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
      box.m_len_size_minus_one = m_len_size_minus_one;
      box.m_sps_num = m_sps_num;
      box.m_pps_num = m_pps_num;
      box.m_padding_count = m_padding_count;
    }
  } while (0);
  return ret;
}

bool AV3AVCDecoderConfigurationRecord::
     get_combined_avc_decoder_configuration_record_box(
         AV3AVCDecoderConfigurationRecord& source_box,
         AV3AVCDecoderConfigurationRecord& combined_box)
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
    combined_box.m_len_size_minus_one = m_len_size_minus_one;
    combined_box.m_sps_num = m_sps_num;
    combined_box.m_pps_num = m_pps_num;
    combined_box.m_padding_count = m_padding_count;
  } while (0);
  return ret;
}

AV3VisualSampleEntry::AV3VisualSampleEntry()
{}

bool AV3VisualSampleEntry::write_visual_sample_entry(AV3FileWriter* file_writer,
                                                     bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The visual sample entry is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem))
                 || (!file_writer->write_data(m_reserved_0, 6 * sizeof(uint8_t),
                                              copy_to_mem))
                 || (!file_writer->write_be_u16(m_data_reference_index,
                                                copy_to_mem))
                 || (!file_writer->write_be_u16(m_pre_defined, copy_to_mem))
                 || (!file_writer->write_be_u16(m_reserved, copy_to_mem)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_pre_defined_1[i],
                                                 copy_to_mem))) {
        ERROR("Failed to write visual sample description box.");
        ret = false;
        return ret;
      }
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_width, copy_to_mem))
                 || (!file_writer->write_be_u16(m_height, copy_to_mem))
                 || (!file_writer->write_be_u32(m_horizresolution, copy_to_mem))
                 || (!file_writer->write_be_u32(m_vertresolution, copy_to_mem))
                 || (!file_writer->write_be_u32(m_reserved_1, copy_to_mem))
                 || (!file_writer->write_be_u16(m_frame_count, copy_to_mem))
                 || (!file_writer->write_data((uint8_t* )m_compressorname,
                                              32 * sizeof(char), copy_to_mem))
                 || (!file_writer->write_be_u16(m_depth, copy_to_mem))
                 || (!file_writer->write_be_s16(m_pre_defined_2, copy_to_mem)))) {
      ERROR("Failed to write visual sample description box.");
      ret = false;
      break;
    }
    if (m_avc_config.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_avc_config.write_avc_decoder_configuration_record_box(
          file_writer, copy_to_mem))) {
        ERROR("Failed to write visual sample description box.");
        ret = false;
        break;
      }
    }
    INFO("write visual sample entry success.");
  } while (0);
  return ret;
}

bool AV3VisualSampleEntry::read_visual_sample_entry(AV3FileReader* file_reader)
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
    AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
    switch (type) {
      case AV3_AVC_DECODER_CONFIGURATION_RECORD_TAG: {
        if (AM_UNLIKELY(!m_avc_config.read_avc_decoder_configuration_record_box(
            file_reader))) {
          ERROR("Failed to read visual sample description box.");
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

bool AV3VisualSampleEntry::get_proper_visual_sample_entry(
     AV3VisualSampleEntry& entry, AV3SubMediaInfo& media_info)
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

bool AV3VisualSampleEntry::copy_visual_sample_entry(AV3VisualSampleEntry& entry)
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

bool AV3VisualSampleEntry::get_combined_visual_sample_entry(
    AV3VisualSampleEntry& source_entry, AV3VisualSampleEntry& combined_entry)
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

void AV3VisualSampleEntry::update_visual_sample_entry_size()
{
  m_base_box.m_size = AV3_VISUAL_SAMPLE_ENTRY_SIZE;
  if (m_avc_config.m_base_box.m_enable) {
    m_base_box.m_size += m_avc_config.m_base_box.m_size;
  }
}

AV3AACDescriptorBox::AV3AACDescriptorBox()
{}

bool AV3AACDescriptorBox::write_aac_descriptor_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The aac elementary stream description box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
    /*ES descriptor takes 38 bytes*/
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_es_descriptor_tag, copy_to_mem))
        || (!file_writer->write_be_u8(m_es_descriptor_tag_size & 0x7f, copy_to_mem))
        || (!file_writer->write_be_u16(m_es_id, copy_to_mem)))) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = false;
      break;
    }
    uint8_t value = ((m_stream_dependency_flag & 0x01) << 7) |
                     ((m_URL_flag & 0x01) << 6) |
                     ((m_OCR_stream_flag & 0x01) << 5) |
                     (m_stream_priority & 0x1f);
    if (AM_UNLIKELY(!file_writer->write_be_u8(value, copy_to_mem))) {
      ERROR("Failed to write stream priority in aac descriptor box.");
      ret = false;
      break;
    }
    uint8_t stream_value = ((m_stream_type & 0x3f) << 2) |
                            ((m_up_stream & 0x01) << 1) |
                            (m_reserved & 0x01);
    if (AM_UNLIKELY((!file_writer->write_be_u8(
                      m_decoder_config_descriptor_tag, copy_to_mem))
        || (!file_writer->write_be_u8(m_decoder_config_descriptor_tag_size & 0x7f,
                                      copy_to_mem))
        || (!file_writer->write_be_u8(m_object_type_id, copy_to_mem))
        // stream type:6 upstream flag:1 reserved flag:1
        || (!file_writer->write_be_u8(stream_value, copy_to_mem))
        || (!file_writer->write_be_u24(m_buffer_size_DB, copy_to_mem))
        || (!file_writer->write_be_u32(m_max_bitrate, copy_to_mem))
        || (!file_writer->write_be_u32(m_avg_bitrate, copy_to_mem)))) {
      ERROR("Failed to write decoder descriptor.");
      ret = false;
      break;
    }
    uint16_t sample_value = ((m_audio_object_type & 0x1f) << 11) |
                             ((m_sampling_frequency_index & 0x0f) << 7) |
                             ((m_channel_configuration & 0x0f) << 3) |
                             ((m_frame_length_flag & 0x01) << 2) |
                             ((m_depends_on_core_coder & 0x01) << 1) |
                             (m_extension_flag & 0x01);
    if (AM_UNLIKELY((!file_writer->write_be_u8(
        m_decoder_specific_info_tag, copy_to_mem))
        || (!file_writer->write_be_u8(
            m_decoder_specific_info_tag_size & 0x7f, copy_to_mem))
        || (!file_writer->write_be_u16(sample_value, copy_to_mem)))) {
      ERROR("Failed to write decoder specific info descriptor");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u8(m_SL_config_descriptor_tag,
                                               copy_to_mem))
        || (!file_writer->write_be_u8(m_SL_config_descriptor_tag_size & 0x7f,
                                      copy_to_mem))
        || (!file_writer->write_be_u8(m_predefined, copy_to_mem)))) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u8(m_padding, copy_to_mem))) {
      ERROR("Failed to write padding in write aac description box.");
      ret = false;
      break;
    }
    INFO("write aac elementary stream description box.");
  } while (0);
  return ret;
}

bool AV3AACDescriptorBox::read_aac_descriptor_box(
     AV3FileReader* file_reader)
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
    uint8_t stream_value = 0;
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_es_descriptor_tag))
        || (!file_reader->read_le_u8(m_es_descriptor_tag_size))
        || (!file_reader->read_le_u16(m_es_id))
        || (!file_reader->read_le_u8(stream_value)))) {
      ERROR("Failed to read aac elementary stream description box.");
      ret = false;
      break;
    }
    m_stream_dependency_flag = (stream_value >> 7) & 0x01;
    m_URL_flag = (stream_value >> 6) & 0x01;
    m_OCR_stream_flag = (stream_value >> 5) & 0x01;
    m_stream_priority = stream_value & 0x1f;

    uint8_t decoder_value = 0;
    if (AM_UNLIKELY((!file_reader->read_le_u8(
        m_decoder_config_descriptor_tag))
        || (!file_reader->read_le_u8(m_decoder_config_descriptor_tag_size))
        || (!file_reader->read_le_u8(m_object_type_id))
        // stream type:6 upstream flag:1 reserved flag:1
        || (!file_reader->read_le_u8(decoder_value))
        || (!file_reader->read_le_u24(m_buffer_size_DB))
        || (!file_reader->read_le_u32(m_max_bitrate))
        || (!file_reader->read_le_u32(m_avg_bitrate)))) {
      ERROR("Failed to read decoder descriptor.");
      ret = false;
      break;
    }
    m_stream_type = (decoder_value >> 2) & 0x3f;
    m_up_stream = (decoder_value >> 1) & 0x01;
    m_reserved = decoder_value & 0x01;

    uint16_t info_value = 0;
    if (AM_UNLIKELY((!file_reader->read_le_u8(
        m_decoder_specific_info_tag))
        || (!file_reader->read_le_u8(m_decoder_specific_info_tag_size))
        || (!file_reader->read_le_u16(info_value)))) {
      ERROR("Failed to read decoder specific info descriptor");
      ret = false;
      break;
    }
    m_audio_object_type = (info_value >> 11) & 0x1f;
    m_sampling_frequency_index = (info_value >> 7) & 0x0f;
    m_channel_configuration = (info_value >> 3) & 0x0f;
    m_frame_length_flag = (info_value >> 2) & 0x01;
    m_depends_on_core_coder = (info_value >> 1) & 0x01;
    m_extension_flag = info_value & 0x01;
    if (AM_UNLIKELY((!file_reader->read_le_u8(m_SL_config_descriptor_tag))
        || (!file_reader->read_le_u8(m_SL_config_descriptor_tag_size))
        || (!file_reader->read_le_u8(m_predefined))
        || (!file_reader->read_le_u8(m_padding)))) {
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

bool AV3AACDescriptorBox::get_proper_aac_descriptor_box(
    AV3AACDescriptorBox& box, AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_buffer_size_DB = m_buffer_size_DB; // 24bits
    box.m_max_bitrate = m_max_bitrate;
    box.m_avg_bitrate = m_avg_bitrate;
    box.m_es_id = m_es_id;
    box.m_es_descriptor_tag = m_es_descriptor_tag;
    box.m_es_descriptor_tag_size = m_es_descriptor_tag_size; //7 bits
    box.m_stream_dependency_flag = m_stream_dependency_flag; //1 bit
    box.m_URL_flag = m_URL_flag; //1 bit
    box.m_OCR_stream_flag = m_OCR_stream_flag; //1 bit
    box.m_stream_priority = m_stream_priority; //5 bits
    box.m_decoder_config_descriptor_tag = m_decoder_config_descriptor_tag;
    box.m_decoder_config_descriptor_tag_size =
        m_decoder_config_descriptor_tag_size;
    box.m_object_type_id = m_object_type_id;
    box.m_stream_type = m_stream_type; //6 bits
    box.m_up_stream = m_up_stream; //1 bit
    box.m_reserved = m_reserved; //1 bit
    box.m_decoder_specific_info_tag = m_decoder_specific_info_tag;
    box.m_decoder_specific_info_tag_size = m_decoder_specific_info_tag_size;
    box.m_audio_object_type = m_audio_object_type; //5 bits
    box.m_sampling_frequency_index = m_sampling_frequency_index; //4 bits
    box.m_channel_configuration = m_channel_configuration; //4 bits
    box.m_frame_length_flag = m_frame_length_flag; //1 bit
    box.m_depends_on_core_coder = m_depends_on_core_coder; //1 bit
    box.m_extension_flag = m_extension_flag; //1 bit
    box.m_SL_config_descriptor_tag = m_SL_config_descriptor_tag;
    box.m_SL_config_descriptor_tag_size = m_SL_config_descriptor_tag_size;
    box.m_predefined = m_predefined;
    box.m_padding = m_padding;
  } while (0);
  return ret;
}

bool AV3AACDescriptorBox::copy_aac_descriptor_box(
     AV3AACDescriptorBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    box.m_buffer_size_DB = m_buffer_size_DB; // 24bits
    box.m_max_bitrate = m_max_bitrate;
    box.m_avg_bitrate = m_avg_bitrate;
    box.m_es_id = m_es_id;
    box.m_es_descriptor_tag = m_es_descriptor_tag;
    box.m_es_descriptor_tag_size = m_es_descriptor_tag_size; //7 bits
    box.m_stream_dependency_flag = m_stream_dependency_flag; //1 bit
    box.m_URL_flag = m_URL_flag; //1 bit
    box.m_OCR_stream_flag = m_OCR_stream_flag; //1 bit
    box.m_stream_priority = m_stream_priority; //5 bits
    box.m_decoder_config_descriptor_tag = m_decoder_config_descriptor_tag;
    box.m_decoder_config_descriptor_tag_size =
        m_decoder_config_descriptor_tag_size;
    box.m_object_type_id = m_object_type_id;
    box.m_stream_type = m_stream_type; //6 bits
    box.m_up_stream = m_up_stream; //1 bit
    box.m_reserved = m_reserved; //1 bit
    box.m_decoder_specific_info_tag = m_decoder_specific_info_tag;
    box.m_decoder_specific_info_tag_size = m_decoder_specific_info_tag_size;
    box.m_audio_object_type = m_audio_object_type; //5 bits
    box.m_sampling_frequency_index = m_sampling_frequency_index; //4 bits
    box.m_channel_configuration = m_channel_configuration; //4 bits
    box.m_frame_length_flag = m_frame_length_flag; //1 bit
    box.m_depends_on_core_coder = m_depends_on_core_coder; //1 bit
    box.m_extension_flag = m_extension_flag; //1 bit
    box.m_SL_config_descriptor_tag = m_SL_config_descriptor_tag;
    box.m_SL_config_descriptor_tag_size = m_SL_config_descriptor_tag_size;
    box.m_predefined = m_predefined;
    box.m_padding = m_padding;
  } while (0);
  return ret;
}

bool AV3AACDescriptorBox::get_combined_aac_descriptor_box(
     AV3AACDescriptorBox& source_box,
     AV3AACDescriptorBox& combined_box)
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
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_buffer_size_DB = m_buffer_size_DB; // 24bits
    combined_box.m_max_bitrate = m_max_bitrate;
    combined_box.m_avg_bitrate = m_avg_bitrate;
    combined_box.m_es_id = m_es_id;
    combined_box.m_es_descriptor_tag = m_es_descriptor_tag;
    combined_box.m_es_descriptor_tag_size = m_es_descriptor_tag_size; //7 bits
    combined_box.m_stream_dependency_flag = m_stream_dependency_flag; //1 bit
    combined_box.m_URL_flag = m_URL_flag; //1 bit
    combined_box.m_OCR_stream_flag = m_OCR_stream_flag; //1 bit
    combined_box.m_stream_priority = m_stream_priority; //5 bits
    combined_box.m_decoder_config_descriptor_tag = m_decoder_config_descriptor_tag;
    combined_box.m_decoder_config_descriptor_tag_size =
        m_decoder_config_descriptor_tag_size;
    combined_box.m_object_type_id = m_object_type_id;
    combined_box.m_stream_type = m_stream_type; //6 bits
    combined_box.m_up_stream = m_up_stream; //1 bit
    combined_box.m_reserved = m_reserved; //1 bit
    combined_box.m_decoder_specific_info_tag = m_decoder_specific_info_tag;
    combined_box.m_decoder_specific_info_tag_size = m_decoder_specific_info_tag_size;
    combined_box.m_audio_object_type = m_audio_object_type; //5 bits
    combined_box.m_sampling_frequency_index = m_sampling_frequency_index; //4 bits
    combined_box.m_channel_configuration = m_channel_configuration; //4 bits
    combined_box.m_frame_length_flag = m_frame_length_flag; //1 bit
    combined_box.m_depends_on_core_coder = m_depends_on_core_coder; //1 bit
    combined_box.m_extension_flag = m_extension_flag; //1 bit
    combined_box.m_SL_config_descriptor_tag = m_SL_config_descriptor_tag;
    combined_box.m_SL_config_descriptor_tag_size = m_SL_config_descriptor_tag_size;
    combined_box.m_predefined = m_predefined;
    combined_box.m_padding = m_padding;
  } while (0);
  return ret;
}

AV3AudioSampleEntry::AV3AudioSampleEntry()
{}

bool AV3AudioSampleEntry::write_audio_sample_entry(AV3FileWriter* file_writer,
                                                   bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The audio sample entry is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem))
        || (!file_writer->write_data(m_reserved_0, 6 * sizeof(uint8_t), copy_to_mem))
        || (!file_writer->write_be_u16(m_data_reference_index, copy_to_mem)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_sound_version, copy_to_mem))) {
      ERROR("Failed to write sound version");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 6; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u8(m_reserved_0[i], copy_to_mem))) {
        ERROR("Failed to write audio sample description box.");
        ret = false;
        break;
      }
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u16(m_channels, copy_to_mem))
        || (!file_writer->write_be_u16(m_sample_size, copy_to_mem))
        || (!file_writer->write_be_u16(m_packet_size, copy_to_mem))
        || (!file_writer->write_be_u32(m_time_scale, copy_to_mem))
        || (!file_writer->write_be_u16(m_reserved_1, copy_to_mem)))) {
      ERROR("Failed to write audio sample description box.");
      ret = false;
      break;
    }
    if (m_aac.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_aac.write_aac_descriptor_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write audio sample description box.");
        ret = false;
        break;
      }
    }
    INFO("write audio sample entry success, size is %u.", m_base_box.m_size);
  } while (0);
  return ret;
}

bool AV3AudioSampleEntry::read_audio_sample_entry(AV3FileReader* file_reader)
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
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_sound_version))) {
      ERROR("Failed to read sound version");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 6; i ++) {
      if (AM_UNLIKELY(!file_reader->read_le_u8(m_reserved_0[i]))) {
        ERROR("Failed to read audio sample description box.");
        ret = false;
        break;;
      }
    }
    if (AM_UNLIKELY(!ret)) {
      break;
    }
    if (AM_UNLIKELY((!file_reader->read_le_u16(m_channels))
        || (!file_reader->read_le_u16(m_sample_size))
        || (!file_reader->read_le_u16(m_packet_size))
        || (!file_reader->read_le_u32(m_time_scale))
        || (!file_reader->read_le_u16(m_reserved_1)))) {
      ERROR("Failed to read audio sample description box.");
      ret = false;
      break;
    }
    switch ((uint32_t) m_base_box.m_type) {
      case AV3_AAC_ENTRY_TAG : {
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

bool AV3AudioSampleEntry::get_proper_audio_sample_entry(
    AV3AudioSampleEntry& entry, AV3SubMediaInfo& media_info)
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
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_sound_version = m_sound_version;
    entry.m_channels = m_channels;
    entry.m_sample_size = m_sample_size;
    entry.m_packet_size = m_packet_size;
    entry.m_time_scale = m_time_scale;
    entry.m_reserved_1 = m_reserved_1;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool AV3AudioSampleEntry::copy_audio_sample_entry(AV3AudioSampleEntry& entry)
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
    entry.m_time_scale = m_time_scale;
    entry.m_data_reference_index = m_data_reference_index;
    entry.m_sound_version = m_sound_version;
    entry.m_channels = m_channels;
    entry.m_sample_size = m_sample_size;
    entry.m_packet_size = m_packet_size;
    entry.m_reserved_1 = m_reserved_1;
    memcpy(entry.m_reserved_0, m_reserved_0, 6 * sizeof(uint8_t));
  } while (0);
  return ret;
}

bool AV3AudioSampleEntry::get_combined_audio_sample_entry(
    AV3AudioSampleEntry& source_entry, AV3AudioSampleEntry& combined_entry)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_entry.m_base_box.m_enable)) {
      ERROR("One of combined audio sample entry is not enabled.");
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
    } else {
      ERROR("The audio type is not same.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_entry.m_base_box);
    combined_entry.m_time_scale = m_time_scale;
    combined_entry.m_data_reference_index = m_data_reference_index;
    combined_entry.m_sound_version = m_sound_version;
    combined_entry.m_channels = m_channels;
    combined_entry.m_sample_size = m_sample_size;
    combined_entry.m_packet_size = m_packet_size;
    combined_entry.m_reserved_1 = m_reserved_1;
    memcpy(combined_entry.m_reserved_0, m_reserved_0, sizeof(uint8_t) * 6);
  } while (0);
  return ret;
}

void AV3AudioSampleEntry::update_audio_sample_entry_size()
{
  m_base_box.m_size = AV3_AUDIO_SAMPLE_ENTRY_SIZE;
  if (m_aac.m_full_box.m_base_box.m_enable) {
    m_base_box.m_size += m_aac.m_full_box.m_base_box.m_size;
    INFO("aac entry size is %u", m_aac.m_full_box.m_base_box.m_size);
  }
}

AV3SampleDescriptionBox::AV3SampleDescriptionBox()
{}

bool AV3SampleDescriptionBox::write_sample_description_box(
     AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample description box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_full_box.write_full_box(file_writer, copy_to_mem))
                    || (!file_writer->write_be_u32(m_entry_count, copy_to_mem)))) {
      ERROR("Failed to write sample description box.");
      ret = false;
      break;
    }
    if (m_visual_entry.m_base_box.m_enable) {
      if (!m_visual_entry.write_visual_sample_entry(file_writer, copy_to_mem)) {
        ERROR("Failed to write visual sample entry.");
        ret = false;
        break;
      }
    }
    if (m_audio_entry.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_audio_entry.write_audio_sample_entry(file_writer,
                                                              copy_to_mem))) {
        ERROR("Failed to write audio sample entry.");
        ret = false;
        break;
      }
    }
    INFO("sample description box size is %u when write it.",
         m_full_box.m_base_box.m_size);
  } while (0);
  return ret;
}

bool AV3SampleDescriptionBox::read_sample_description_box(
     AV3FileReader* file_reader)
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
      case AV3_H264_ENTRY_TAG :
      case AV3_H265_ENTRY_TAG : {
        if (!m_visual_entry.read_visual_sample_entry(file_reader)) {
          ERROR("Failed to read visual sample entry.");
          ret = false;
        }
      }break;
      case AV3_AAC_ENTRY_TAG : {
        if (!m_audio_entry.read_audio_sample_entry(file_reader)) {
          ERROR("Failed to read audio sample entry.");
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

bool AV3SampleDescriptionBox::get_proper_sample_description_box(
     AV3SampleDescriptionBox& box, AV3SubMediaInfo& media_info)
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
    box.m_entry_count = m_entry_count;
  } while (0);
  return ret;
}

bool AV3SampleDescriptionBox::copy_sample_description_box(
     AV3SampleDescriptionBox& box)
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
    box.m_entry_count = m_entry_count;
  } while (0);
  return ret;
}

bool AV3SampleDescriptionBox::get_combined_sample_description_box(
     AV3SampleDescriptionBox& source_box,
     AV3SampleDescriptionBox& combined_box)
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
  } while (0);
  return ret;
}

void AV3SampleDescriptionBox::update_sample_description_box()
{
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t);
  if (m_visual_entry.m_base_box.m_enable) {
    m_visual_entry.update_visual_sample_entry_size();
    m_full_box.m_base_box.m_size += m_visual_entry.m_base_box.m_size;
  }
  if (m_audio_entry.m_base_box.m_enable) {
    m_audio_entry.update_audio_sample_entry_size();
    m_full_box.m_base_box.m_size += m_audio_entry.m_base_box.m_size;
  }
}

AV3SampleSizeBox::AV3SampleSizeBox()
{}

AV3SampleSizeBox::~AV3SampleSizeBox()
{
  delete[] m_size_array;
}

bool AV3SampleSizeBox::write_sample_size_box(AV3FileWriter* file_writer,
                                             bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample size box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_sample_size, copy_to_mem))
        || (!file_writer->write_be_u32(m_sample_count, copy_to_mem)))) {
      ERROR("Failed to write sample size box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_sample_count; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_size_array[i], copy_to_mem))) {
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

bool AV3SampleSizeBox::read_sample_size_box(AV3FileReader* file_reader)
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

void AV3SampleSizeBox::update_sample_size_box_size()
{
  m_full_box.m_base_box.m_size = AV3_SAMPLE_SIZE_BOX_SIZE;
  if (m_sample_size == 0) {
    m_full_box.m_base_box.m_size += m_sample_count * sizeof(uint32_t);
  }
}

bool AV3SampleSizeBox::get_proper_sample_size_box(AV3SampleSizeBox& box,
                                                  AV3SubMediaInfo& media_info)
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

bool AV3SampleSizeBox::copy_sample_size_box(AV3SampleSizeBox& box)
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

bool AV3SampleSizeBox::get_combined_sample_size_box(AV3SampleSizeBox& source_box,
                                                    AV3SampleSizeBox& combined_box)
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

AV3SampleToChunkEntry::AV3SampleToChunkEntry()
{}

bool AV3SampleToChunkEntry::write_sample_chunk_entry(AV3FileWriter* file_writer,
                                                     bool copy_to_mem)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((!file_writer->write_be_u32(m_first_chunk, copy_to_mem))
        || (!file_writer->write_be_u32(m_sample_per_chunk, copy_to_mem))
        || (!file_writer->write_be_u32(m_sample_description_index,
                                       copy_to_mem)))) {
      ERROR("Failed to write sample to chunk box.");
      ret = false;
      break;
    }
    INFO("write sample chunk entry success.");
  } while (0);
  return ret;
}

bool AV3SampleToChunkEntry::read_sample_chunk_entry(AV3FileReader* file_reader)
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

AV3SampleToChunkBox::AV3SampleToChunkBox()
{}

AV3SampleToChunkBox::~AV3SampleToChunkBox()
{
  delete[] m_entrys;
}

bool AV3SampleToChunkBox::write_sample_to_chunk_box(AV3FileWriter* file_writer,
                                                    bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sample to chunk box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write sample to chunk box");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count, copy_to_mem))) {
      ERROR("Failed to write sample to chunk box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(!m_entrys[i].write_sample_chunk_entry(file_writer,
                                                            copy_to_mem))) {
        ERROR("Failed to write sample to chunk box.");
        ret = false;
        break;
      }
    }
    INFO("write sample to chunk box success.");
  } while (0);
  return ret;
}

bool AV3SampleToChunkBox::read_sample_to_chunk_box(AV3FileReader* file_reader)
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
      m_entrys = new AV3SampleToChunkEntry[m_entry_count];
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

bool AV3SampleToChunkBox::optimize_sample_to_chunk_entry()
{
  bool ret = true;
  do {
    if (m_entry_count == 0) {
      ERROR("The entry count of sample to chunk box is zero.");
      ret = false;
      break;
    }
    AV3SampleToChunkEntry *tmp_entry = new AV3SampleToChunkEntry[m_entry_count];
    if (!tmp_entry) {
      ERROR("Failed to malloc memory for sample to chunk box.");
      ret = false;
      break;
    }
    uint32_t tmp_chunk_cnt = 0;
    tmp_entry[0].m_first_chunk = 1;
    tmp_entry[0].m_sample_description_index = 1;
    tmp_entry[0].m_sample_per_chunk = m_entrys[0].m_sample_per_chunk;
    for (uint i = 1; i < m_entry_count; ++ i) {
      if (m_entrys[i].m_sample_per_chunk ==
          tmp_entry[tmp_chunk_cnt].m_sample_per_chunk) {
        continue;
      }
      ++ tmp_chunk_cnt;
      tmp_entry[tmp_chunk_cnt].m_first_chunk = i + 1;
      tmp_entry[tmp_chunk_cnt].m_sample_description_index = 1;
      tmp_entry[tmp_chunk_cnt].m_sample_per_chunk = m_entrys[i].m_sample_per_chunk;
    }
    delete[] m_entrys;
    m_entrys = tmp_entry;
    m_entry_buf_count = m_entry_count;
    m_entry_count = tmp_chunk_cnt + 1;
  } while(0);
  return ret;
}

void AV3SampleToChunkBox::update_sample_to_chunk_box_size()
{
  if (!optimize_sample_to_chunk_entry()) {
    ERROR("Failed to optimize sample to chunk entry.");
  }
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * AV3_SAMPLE_TO_CHUNK_ENTRY_SIZE;
}

bool AV3SampleToChunkBox::get_proper_sample_to_chunk_box(
     AV3SampleToChunkBox& box, AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_entry_count > 0) {
      box.m_entry_count = 1;
      delete[] box.m_entrys;
      box.m_entrys = new AV3SampleToChunkEntry[1];
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

bool AV3SampleToChunkBox::copy_sample_to_chunk_box(
     AV3SampleToChunkBox& box)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    if (m_entry_count > 0) {
      box.m_entry_count = 1;
      delete[] box.m_entrys;
      box.m_entrys = new AV3SampleToChunkEntry[1];
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

bool AV3SampleToChunkBox::get_combined_sample_to_chunk_box(
     AV3SampleToChunkBox& source_box, AV3SampleToChunkBox& combined_box)
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
      combined_box.m_entrys = new AV3SampleToChunkEntry[1];
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

AV3ChunkOffsetBox::AV3ChunkOffsetBox()
{}

AV3ChunkOffsetBox::~AV3ChunkOffsetBox()
{
  delete[] m_chunk_offset;
}

bool AV3ChunkOffsetBox::write_chunk_offset_box(AV3FileWriter* file_writer,
                                               bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The chunk offset box is not enabled.");
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write chunk offset box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count, copy_to_mem))) {
      ERROR("Failed to write chunk offset box");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; i ++) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_chunk_offset[i],
                                                 copy_to_mem))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    INFO("write chunk offset box success.");
  } while (0);
  return ret;
}

bool AV3ChunkOffsetBox::read_chunk_offset_box(AV3FileReader* file_reader)
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

void AV3ChunkOffsetBox::update_chunk_offset_box_size()
{
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_entry_count * sizeof(uint32_t);
}

bool AV3ChunkOffsetBox::get_proper_chunk_offset_box(AV3ChunkOffsetBox& box,
                                                    AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_full_box.copy_full_box(box.m_full_box);
    //create m_chunk_offset and fill it on the outsize.
  } while (0);
  return ret;
}

bool AV3ChunkOffsetBox::copy_chunk_offset_box(AV3ChunkOffsetBox& box)
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

bool AV3ChunkOffsetBox::get_combined_chunk_offset_box(
     AV3ChunkOffsetBox& source_box,
     AV3ChunkOffsetBox& combined_box)
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

AV3SyncSampleTableBox::AV3SyncSampleTableBox()
{}

AV3SyncSampleTableBox::~AV3SyncSampleTableBox()
{
  delete[] m_sync_sample_table;
}

bool AV3SyncSampleTableBox::write_sync_sample_table_box(
    AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The sync sample table box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_full_box.write_full_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write full box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_stss_count, copy_to_mem))) {
      ERROR("Failed to write stss count.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_stss_count; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_sync_sample_table[i],
                                                 copy_to_mem))) {
        ERROR("Failed to write stss to file.");
        ret = false;
        break;
      }
    }
    INFO("write sync sample table box success.");
  } while (0);
  return ret;
}

bool AV3SyncSampleTableBox::read_sync_sample_table_box(AV3FileReader* file_reader)
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

void AV3SyncSampleTableBox::update_sync_sample_box_size()
{
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + sizeof(uint32_t)
      + m_stss_count * sizeof(uint32_t);
}

bool AV3SyncSampleTableBox::get_proper_sync_sample_table_box(
     AV3SyncSampleTableBox& box, AV3SubMediaInfo& media_info)
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

bool AV3SyncSampleTableBox::copy_sync_sample_table_box(AV3SyncSampleTableBox& box)
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


bool AV3SyncSampleTableBox::get_combined_sync_sample_table_box(
     AV3SyncSampleTableBox& source_box, AV3SyncSampleTableBox& combined_box)
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

bool AV3SampleTableBox::write_sample_table_box(AV3FileWriter* file_writer,
                                               bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The sample table box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write sample table box.");
      ret = false;
      break;
    }
    if (m_sample_description.m_full_box.m_base_box.m_enable) {
      if (!m_sample_description.write_sample_description_box(
          file_writer, copy_to_mem)) {
        ERROR("Failed to write sample description box.");
        ret = false;
        break;
      }
    }
    if (m_stts.m_full_box.m_base_box.m_enable) {
      if ((!m_stts.write_decoding_time_to_sample_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write decoding time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      if ((!m_sample_to_chunk.write_sample_to_chunk_box(file_writer,
                                                        copy_to_mem))) {
        ERROR("Failed to write sample to chunk box.");
        ret = false;
        break;
      }
    }
    if (m_sample_size.m_full_box.m_base_box.m_enable) {
      if ((!m_sample_size.write_sample_size_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write sample size box.");
        ret = false;
        break;
      }
    }
    if (m_ctts.m_full_box.m_base_box.m_enable) {
      if ((!m_ctts.write_composition_time_to_sample_box(file_writer,
                                                        copy_to_mem))) {
        ERROR("Failed to write composition time to sample box.");
        ret = false;
        break;
      }
    }
    if (m_chunk_offset.m_full_box.m_base_box.m_enable) {
      if ((!m_chunk_offset.write_chunk_offset_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write chunk offset box.");
        ret = false;
        break;
      }
    }
    if (m_sync_sample.m_full_box.m_base_box.m_enable) {
      INFO("begin to write sync_sample table box");
      if (!m_sync_sample.write_sync_sample_table_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write sync sample table box.");
        ret = false;
        break;
      }
    }
    INFO("write sample table box success.");
  } while (0);
  return ret;
}

bool AV3SampleTableBox::read_sample_table_box(AV3FileReader* file_reader)
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
      AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case AV3_SAMPLE_DESCRIPTION_BOX_TAG: {
          if (!m_sample_description.read_sample_description_box(file_reader)) {
            ERROR("Failed to read visual sample description box.");
            ret = false;
          }
        } break;
        case AV3_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG: {
          if (!m_stts.read_decoding_time_to_sample_box(file_reader)) {
            ERROR("Failed to read decoding time to sample box.");
            ret = false;
          }
        } break;
        case AV3_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG: {
          if (!m_ctts.read_composition_time_to_sample_box(file_reader)) {
            ERROR("Failed to read composition time to sample box.");
            ret = false;
          }
        } break;
        case AV3_SAMPLE_SIZE_BOX_TAG: {
          if (!m_sample_size.read_sample_size_box(file_reader)) {
            ERROR("Failed to read sample size box.");
            ret = false;
          }
        } break;
        case AV3_SAMPLE_TO_CHUNK_BOX_TAG: {
          if (!m_sample_to_chunk.read_sample_to_chunk_box(file_reader)) {
            ERROR("Failed to read sample to chunk box.");
            ret = false;
          }
        } break;
        case AV3_CHUNK_OFFSET_BOX_TAG: {
          if (!m_chunk_offset.read_chunk_offset_box(file_reader)) {
            ERROR("Failed to read chunk offset box.");
            ret = false;
          }
        } break;
        case AV3_SYNC_SAMPLE_TABLE_BOX_TAG: {
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

bool AV3SampleTableBox::get_proper_sample_table_box(
     AV3SampleTableBox& box, AV3SubMediaInfo& media_info)
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

bool AV3SampleTableBox::copy_sample_table_box(AV3SampleTableBox& box)
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

bool AV3SampleTableBox::get_combined_sample_table_box(
     AV3SampleTableBox& source_box, AV3SampleTableBox& combined_box)
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

void AV3SampleTableBox::update_sample_table_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE;
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
  if (m_sync_sample.m_full_box.m_base_box.m_enable) {
    m_sync_sample.update_sync_sample_box_size();
    m_base_box.m_size += m_sync_sample.m_full_box.m_base_box.m_size;
    INFO("Sync box size is %u",
         m_sync_sample.m_full_box.m_base_box.m_size);
  }
}

bool AV3MediaInformationBox::write_media_info_box(AV3FileWriter* file_writer,
                                                  bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media information box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write media information box.");
      ret = false;
      break;
    }
    if (m_video_info_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_video_info_header.write_video_media_info_header_box(
          file_writer, copy_to_mem))) {
        ERROR("Failed to write video media information header box.");
        ret = false;
        break;
      }
    }
    if (m_sound_info_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sound_info_header.write_sound_media_info_header_box(
          file_writer, copy_to_mem))) {
        ERROR("Failed to write sound media information header box.");
        ret = false;
        break;
      }
    }
    if (m_data_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_data_info.write_data_info_box(file_writer,
                                                       copy_to_mem))) {
        ERROR("Failed to write data information box.");
        ret = false;
        break;
      }
    }
    if (m_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_sample_table.write_sample_table_box(
          file_writer, copy_to_mem))) {
        ERROR("Failed to write sample table box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AV3MediaInformationBox::read_media_info_box(AV3FileReader* file_reader)
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
      AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case AV3_VIDEO_MEDIA_HEADER_BOX_TAG: {
          if (!m_video_info_header.read_video_media_info_header_box(
              file_reader)) {
            ERROR("Failed to read video media information header box");
            ret = false;
          }
        } break;
        case AV3_SOUND_MEDIA_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_sound_info_header.read_sound_media_info_header_box(
              file_reader))) {
            ERROR("Failed to read sound media information header box.");
            ret = false;
          }
        } break;
        case AV3_DATA_INFORMATION_BOX_TAG: {
          if (!m_data_info.read_data_info_box(file_reader)) {
            ERROR("Failed to read data information box.");
            ret = false;
          }
        } break;
        case AV3_SAMPLE_TABLE_BOX_TAG: {
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

bool AV3MediaInformationBox::get_proper_media_info_box(
     AV3MediaInformationBox& box, AV3SubMediaInfo& media_info)
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

bool AV3MediaInformationBox::copy_media_info_box(
     AV3MediaInformationBox& box)
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

bool AV3MediaInformationBox::get_combined_media_info_box(
     AV3MediaInformationBox& source_box, AV3MediaInformationBox& combined_box)
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

void AV3MediaInformationBox::update_media_info_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE;
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

bool AV3MediaBox::write_media_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The media box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write media box.");
      ret = false;
      break;
    }
    if (m_media_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_header.write_media_header_box(file_writer,
                                                             copy_to_mem))) {
        ERROR("Failed to write media header box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media header box is not enabled.");
    }
    if (m_media_handler.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_handler.write_handler_reference_box(file_writer,
                                                               copy_to_mem))) {
        ERROR("Failed to write handler reference box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The media handler box is not enabled.");
    }
    if (m_media_info.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media_info.write_media_info_box(file_writer,
                                                         copy_to_mem))) {
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

bool AV3MediaBox::read_media_box(AV3FileReader* file_reader)
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
      AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case AV3_MEDIA_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_media_header.read_media_header_box(file_reader))) {
            ERROR("Failed to read media header box.");
            ret = false;
          }
        } break;
        case AV3_HANDLER_REFERENCE_BOX_TAG: {
          if (AM_UNLIKELY(!m_media_handler.read_handler_reference_box(file_reader))) {
            ERROR("Failed to read handler reference box.");
            ret = false;
          }
        } break;
        case AV3_MEDIA_INFORMATION_BOX_TAG: {
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

bool AV3MediaBox::get_proper_media_box(AV3MediaBox& box,
                                       AV3SubMediaInfo& media_info)
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

bool AV3MediaBox::copy_media_box(AV3MediaBox& box)
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

bool AV3MediaBox::get_combined_media_box(AV3MediaBox& source_box,
                                         AV3MediaBox& combined_box)
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

void AV3MediaBox::update_media_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE;
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

bool AV3MetaDataEntry::write_meta_data_entry(AV3FileWriter* file_writer,
                                             bool copy_to_mem)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_sample_number, copy_to_mem))) {
      ERROR("Failed to write sample number in meta data entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_s32(m_sec, copy_to_mem))) {
      ERROR("Failed to write sec in meta data entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_s16(m_msec, copy_to_mem))) {
      ERROR("Failed to write msec in meta data entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_reserved, copy_to_mem))) {
      ERROR("Failed to write served data in meta data entry.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_s32(m_time_stamp, copy_to_mem))) {
      ERROR("Failed to write time stamp in meta data entry.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3MetaDataEntry::read_meta_data_entry(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_sample_number))) {
      ERROR("Failed to read sample number in meta data entry");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_s32(m_sec))) {
      ERROR("Failed to read sec value in meta data entry");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_s16(m_msec))) {
      ERROR("Failed to read msec value in meta data entry");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_reserved))) {
      ERROR("Failed to read reserved data in meta data entry");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_s32(m_time_stamp))) {
      ERROR("Failed to read time stamp in meta data entry");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

AV3PSMHBox::~AV3PSMHBox()
{
  delete m_entry;
  m_entry = nullptr;
}

bool AV3PSMHBox::write_psmh_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psmh box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write base box in psmh box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < 2; ++ i) {
      if (AM_UNLIKELY(!file_writer->write_be_u32(m_reserved[i], copy_to_mem))) {
        ERROR("Failed to write reserved value in psmh box.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u16(m_meta_id, copy_to_mem))) {
      ERROR("Failed to write meta id in psmh box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_meta_length, copy_to_mem))) {
      ERROR("Failed to write meta length value in psmh box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_writer->write_be_u32(m_entry_count, copy_to_mem))) {
      ERROR("Failed to write entry count in psmh box.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (!m_entry[i].write_meta_data_entry(file_writer, copy_to_mem)) {
        ERROR("Failed to write meta data entry.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
  } while(0);
  return ret;
}

bool AV3PSMHBox::read_psmh_box(AV3FileReader *file_reader)
{
  bool ret = true;
  uint64_t offset_begin = 0;
  uint64_t offset_end = 0;
  do {
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
    for (uint32_t i = 0; i < 2; ++ i) {
      if (AM_UNLIKELY(!file_reader->read_le_u32(m_reserved[i]))) {
        ERROR("Failed to read reserved value in psmh box.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u16(m_meta_id))) {
      ERROR("Failed to read meta id in psmh box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_meta_length))) {
      ERROR("Failed to read meta length in psmh box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!file_reader->read_le_u32(m_entry_count))) {
      ERROR("Failed to read entry count in psmh box.");
      ret = false;
      break;
    }
    if (m_entry_count > 0) {
      m_entry = new AV3MetaDataEntry[m_entry_count];
      if (!m_entry) {
        ERROR("Failed to malloc AV3MetaDataEntry in psmh box.");
        ret = false;
        break;
      }
    } else {
      ERROR("entry count is zero.");
      ret = false;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(!m_entry[i].read_meta_data_entry(file_reader))) {
        ERROR("Failed to read meta data entry.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
    if (AM_UNLIKELY(!file_reader->tell_file(offset_end))) {
      ERROR("Failed to tell file when read media information box.");
      ret = false;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("read psmh box error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while(0);
  return ret;
}

void AV3PSMHBox::update_psmh_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE + 4 * sizeof(uint32_t) +
      sizeof(uint16_t) + m_entry_count * sizeof(AV3MetaDataEntry);
  m_meta_length = sizeof(uint32_t) + m_entry_count * sizeof(AV3MetaDataEntry);
}

bool AV3PSMHBox::get_proper_psmh_box(AV3PSMHBox& psmh_box, AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    m_base_box.copy_base_box(psmh_box.m_base_box);
    psmh_box.m_meta_length = m_meta_length;
    psmh_box.m_meta_id = m_meta_id;
    psmh_box.m_entry_count = media_info.get_last_frame() -
        media_info.get_first_frame() + 1;
    if (media_info.get_last_frame() > m_entry_count) {
      ERROR("end frame is bigger than sample count.");
      ret = false;
      break;
    }
    uint32_t entry_count = psmh_box.m_entry_count;
    delete[] psmh_box.m_entry;
    psmh_box.m_entry = new AV3MetaDataEntry[entry_count];
    if (AM_UNLIKELY(!psmh_box.m_entry)) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    uint32_t first_frame_num = media_info.get_first_frame();
    for (uint32_t i = 0; i < entry_count; ++ i) {
      psmh_box.m_entry[i].m_sample_number = i;
      psmh_box.m_entry[i].m_sec = m_entry[first_frame_num - 1 + i].m_sec;
      psmh_box.m_entry[i].m_msec = m_entry[first_frame_num - 1 + i].m_msec;
      psmh_box.m_entry[i].m_time_stamp =
          m_entry[first_frame_num - 1 + i].m_time_stamp;
    }
    psmh_box.update_psmh_box_size();
  } while(0);
  return ret;
}

bool AV3PSMHBox::get_combined_psmh_box(AV3PSMHBox& source_box,
                                       AV3PSMHBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("One of combined pamh box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    combined_box.m_meta_id = m_meta_id;
    combined_box.m_meta_length = sizeof(m_entry_count) +
        (m_entry_count + source_box.m_entry_count) * sizeof(AV3MetaDataEntry);
    combined_box.m_entry_count = m_entry_count + source_box.m_entry_count;
    combined_box.m_entry = new AV3MetaDataEntry[combined_box.m_entry_count];
    if (!combined_box.m_entry) {
      ERROR("Failed to malloc memory for psmh box.");
      ret = false;
      break;
    }
    uint32_t i = 0;
    for (; i < m_entry_count; ++ i) {
      combined_box.m_entry[i].m_sample_number = i;
      combined_box.m_entry[i].m_sec = m_entry[i].m_sec;
      combined_box.m_entry[i].m_time_stamp = m_entry[i].m_time_stamp;
      combined_box.m_entry[i].m_msec = m_entry[i].m_msec;
    }
    for (uint32_t j = 0; j < source_box.m_entry_count; ++ j) {
      combined_box.m_entry[i + j].m_sample_number = i + j;
      combined_box.m_entry[i + j].m_sec = source_box.m_entry[j].m_sec;
      combined_box.m_entry[i + j].m_time_stamp = source_box.m_entry[j].m_time_stamp;
      combined_box.m_entry[i + j].m_msec = source_box.m_entry[j].m_msec;
    }
    combined_box.update_psmh_box_size();
  } while(0);
  return ret;
}

bool AV3PSMHBox::copy_psmh_box(AV3PSMHBox& psmh_box)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psmh box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(psmh_box.m_base_box);
    psmh_box.m_meta_length = m_meta_length;
    psmh_box.m_entry_count = m_entry_count;
    psmh_box.m_meta_id = m_meta_id;
    psmh_box.m_entry = new AV3MetaDataEntry[m_entry_count];
    if (!psmh_box.m_entry) {
      ERROR("Failed to malloc memory for psmh box.");
      ret = false;
      break;
    }
    memcpy((char*)psmh_box.m_entry, (char*)m_entry,
           m_entry_count * sizeof(AV3MetaDataEntry));
  } while(0);
  return ret;
}

bool AV3PSMTBox::write_psmt_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psmt box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_base_box.write_base_box(file_writer, copy_to_mem))) {
      ERROR("Failed to write base box in psmt box.");
      ret = false;
      break;
    }
    if (m_psmh_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_psmh_box.write_psmh_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write psmb box in psmt box.");
        ret = false;
        break;
      }
    }
    if (m_meta_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_meta_sample_table.write_sample_table_box(file_writer,
                                                               copy_to_mem))) {
        ERROR("Failed to write meta sample table box in psmt box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3PSMTBox::read_psmt_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_base_box.read_base_box(file_reader))) {
      ERROR("Failed to read base box in psmt box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_psmh_box.read_psmh_box(file_reader))) {
      ERROR("Failed to read psmh box in psmt box.");
      ret = false;
      break;
    }
    if (file_reader->get_box_type() == AV3_SAMPLE_TABLE_BOX_TAG) {
      if (AM_UNLIKELY(!m_meta_sample_table.read_sample_table_box(file_reader))) {
        ERROR("Failed to read sample table box in psmt box.");
        ret = false;
        break;
      }
    }
    m_base_box.m_enable = true;
  } while(0);
  return ret;
}

void AV3PSMTBox::update_psmt_box_size()
{
  m_psmh_box.update_psmh_box_size();
  m_base_box.m_size = AV3_BASE_BOX_SIZE + m_psmh_box.m_base_box.m_size;
  if (m_meta_sample_table.m_base_box.m_enable) {
    m_meta_sample_table.update_sample_table_box_size();
    m_base_box.m_size += m_meta_sample_table.m_base_box.m_size;
  }
}

bool AV3PSMTBox::get_proper_psmt_box(AV3PSMTBox& psmt_box,
                                     AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    if (m_base_box.m_enable) {
      m_base_box.copy_base_box(psmt_box.m_base_box);
      if (m_psmh_box.m_base_box.m_enable) {
        if (AM_UNLIKELY(!m_psmh_box.get_proper_psmh_box(psmt_box.m_psmh_box,
                                                        media_info))) {
          ERROR("Failed to get proper psmh box");
          ret = false;
          break;
        }
      }
      if (m_meta_sample_table.m_base_box.m_enable) {
        if (AM_UNLIKELY(!m_meta_sample_table.get_proper_sample_table_box(
            psmt_box.m_meta_sample_table, media_info))) {
          ERROR("Failed to get proper meta sample table box in psmt box.");
          ret = false;
          break;
        }
      }
    } else {
      ERROR("The psmt box is not enabled.");
      ret = false;
      break;
    }
    psmt_box.update_psmt_box_size();
  } while(0);
  return ret;
}

bool AV3PSMTBox::get_combined_psmt_box(AV3PSMTBox& source_box,
                                       AV3PSMTBox& combined_box)
{
  bool ret = true;
  do {
    if ((!m_base_box.m_enable) || (!source_box.m_base_box.m_enable)) {
      ERROR("psmt box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    if (m_psmh_box.m_base_box.m_enable && source_box.m_psmh_box.m_base_box.m_enable) {
      if (!m_psmh_box.get_combined_psmh_box(source_box.m_psmh_box,
                                            combined_box.m_psmh_box)) {
        ERROR("Failed to get combined psmh box");
        ret = false;
        break;
      }
    } else {
      ERROR("The psmh box is not enabled.");
      ret = false;
      break;
    }
    if (m_meta_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_meta_sample_table.get_combined_sample_table_box(
          source_box.m_meta_sample_table, combined_box.m_meta_sample_table))) {
        ERROR("Failed to get combined meta sample table box in psmt box.");
        ret = false;
        break;
      }
    }
    combined_box.update_psmt_box_size();
  } while(0);
  return ret;
}

bool AV3PSMTBox::copy_psmt_box(AV3PSMTBox& psmt_box)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psmt box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(psmt_box.m_base_box);
    if (!m_psmh_box.m_base_box.m_enable) {
      ERROR("The psmh box is not enabled.");
      ret = false;
      break;
    }
    if (!m_psmh_box.copy_psmh_box(psmt_box.m_psmh_box)) {
      ERROR("Failed to copy psmh box");
      ret = false;
      break;
    }
    if (m_meta_sample_table.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_meta_sample_table.copy_sample_table_box(
          psmt_box.m_meta_sample_table))) {
        ERROR("Failed to copy meta sample table box in psmt box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3TrackUUIDBox::write_track_uuid_box(AV3FileWriter* file_writer,
                                           bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The track uuid is not enabled.");
      ret = false;
      break;
    }
    if (!m_base_box.write_base_box(file_writer, copy_to_mem)) {
      ERROR("Failed to write base box in track uuid  box.");
      ret = false;
      break;
    }
    for (uint8_t i = 0; i < 16; ++ i) {
      if (!file_writer->write_be_u8(m_uuid[i], copy_to_mem)) {
        ERROR("Failed to write uuid data in track uuid box.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
    if (m_psmt_box.m_base_box.m_enable) {
      if (!m_psmt_box.write_psmt_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write psmt box in track uuid box.");
        ret = false;
        break;
      }
    }
    if (m_padding_count > 0) {
      for (uint8_t i = 0; i < m_padding_count; ++ i) {
        if (!file_writer->write_be_u8(0, copy_to_mem)) {
          ERROR("Failed to write padding value in track uuid box.");
          ret = false;
          break;
        }
      }
      if (!ret) {
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3TrackUUIDBox::read_track_uuid_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file in track uuid box.");
      ret = false;
      break;
    }
    if (!m_base_box.read_base_box(file_reader)) {
      ERROR("Failed to read base box in track uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->read_data(m_uuid, 16)) {
      ERROR("Failed to read uuid data in track uuid box.");
      ret = false;
      break;
    }
    if (!m_psmt_box.read_psmt_box(file_reader)) {
      ERROR("Failed to read psmt box in track uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read track uuid box.");
      ret = false;
      break;
    }
    m_padding_count = m_base_box.m_size - (offset_end - offset_begin);
    if ((m_padding_count >= 0) && (m_padding_count < 4)) {
      uint8_t data[4] = {0};
      NOTICE("track uuid box padding count is %u", m_padding_count);
      if (!file_reader->read_data(data, m_padding_count)) {
        ERROR("Failed to read padding in track uuid box.");
        ret = false;
        break;
      }
    } else {
      ERROR("Padding count error.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while(0);
  return ret;
}

void AV3TrackUUIDBox::update_track_uuid_box_size()
{
  m_psmt_box.update_psmt_box_size();
  m_base_box.m_size = AV3_BASE_BOX_SIZE + 16 + m_psmt_box.m_base_box.m_size +
      m_padding_count;
}

bool AV3TrackUUIDBox::get_proper_track_uuid_box(AV3TrackUUIDBox& uuid_box,
                               AV3SubMediaInfo& media_info)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The track uuid box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(uuid_box.m_base_box);
    memcpy(uuid_box.m_uuid, m_uuid, 16);
    if (m_psmt_box.m_base_box.m_enable) {
      if (!m_psmt_box.get_proper_psmt_box(uuid_box.m_psmt_box, media_info)) {
        ERROR("Failed to get proper psmt box.");
        ret = false;
        break;
      }
    }
    uuid_box.update_track_uuid_box_size();
    if ((uuid_box.m_base_box.m_size % 4) != 0) {
      uuid_box.m_padding_count = 4 - (uuid_box.m_base_box.m_size % 4);
      uuid_box.update_track_uuid_box_size();
    }
  } while(0);
  return ret;
}

bool AV3TrackUUIDBox::get_combined_track_uuid_box(AV3TrackUUIDBox& source_box,
                                 AV3TrackUUIDBox& combined_box)
{
  bool ret = true;
  do {
    if (!(m_base_box.m_enable && source_box.m_base_box.m_enable)) {
      ERROR("The track uuid box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    memcpy(m_uuid, combined_box.m_uuid, sizeof(m_uuid));
    if (!(m_psmt_box.m_base_box.m_enable &&
        source_box.m_psmt_box.m_base_box.m_enable)) {
      ERROR("The psmt box is not enabled.");
      ret = false;
      break;
    }
    if (!m_psmt_box.get_combined_psmt_box(source_box.m_psmt_box,
                                          combined_box.m_psmt_box)) {
      ERROR("Failed to get combined psmt box.");
      ret = false;
      break;
    }
    combined_box.update_track_uuid_box_size();
    if ((combined_box.m_base_box.m_size % 4) != 0) {
      combined_box.m_padding_count = 4 - (combined_box.m_base_box.m_size % 4);
      combined_box.update_track_uuid_box_size();
    }
  } while(0);
  return ret;
}

bool AV3TrackUUIDBox::copy_track_uuid_box(AV3TrackUUIDBox& box)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The track uuid  box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(box.m_base_box);
    memcpy(box.m_uuid, m_uuid, sizeof(m_uuid));
    box.m_padding_count = m_padding_count;
    if (m_psmt_box.m_base_box.m_enable) {
      if (!m_psmt_box.copy_psmt_box(box.m_psmt_box)) {
        ERROR("Failed to copy psmt box");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3TrackBox::write_track_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The movie track box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write movie track box.");
      ret = false;
      break;
    }
    if (m_track_header.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_track_header.write_movie_track_header_box(
          file_writer, copy_to_mem))) {
        ERROR("Failed to write movie track header box.");
        ret = false;
        break;
      }
    }
    if (m_media.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_media.write_media_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write media box.");
        ret = false;
        break;
      }
    }
    if (m_uuid.m_base_box.m_enable) {
      if (!m_uuid.write_track_uuid_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write track uuid box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AV3TrackBox::read_track_box(AV3FileReader* file_reader)
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
      AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case AV3_TRACK_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_track_header.read_movie_track_header_box(
              file_reader))) {
            ERROR("Failed to read movie track header box.");
            ret = false;
          }
        } break;
        case AV3_MEDIA_BOX_TAG: {
          if (AM_UNLIKELY(!m_media.read_media_box(file_reader))) {
            ERROR("Failed to read media box.");
            ret = false;
          }
        } break;
        case AV3_UUID_BOX_TAG : {
          if (!m_uuid.read_track_uuid_box(file_reader)) {
            ERROR("Failed to read track uuid box.");
            ret = false;
            break;
          }
        }
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

bool AV3TrackBox::get_proper_track_box(AV3TrackBox& box,
                                       AV3SubMediaInfo& media_info)
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
    if (m_uuid.m_base_box.m_enable) {
      if (!m_uuid.get_proper_track_uuid_box(box.m_uuid, media_info)) {
        ERROR("Failed to get proper track uuid box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AV3TrackBox::copy_track_box(AV3TrackBox& box)
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
    if (m_uuid.m_base_box.m_enable) {
      if (!m_uuid.copy_track_uuid_box(box.m_uuid)) {
        ERROR("Failed to copy uuid box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AV3TrackBox::get_combined_track_box(AV3TrackBox& source_box,
                                         AV3TrackBox& combined_box)
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
    if (m_uuid.m_base_box.m_enable) {
      if (!m_uuid.get_combined_track_uuid_box(source_box.m_uuid,
                                              combined_box.m_uuid)) {
        ERROR("Failed to get combined track uuid box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

void AV3TrackBox::update_track_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE;
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
  if (m_uuid.m_base_box.m_enable) {
    m_uuid.update_track_uuid_box_size();
    m_base_box.m_size += m_uuid.m_base_box.m_size;
  }
}

bool AV3MovieBox::write_movie_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The movie box is not enabled.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((!m_base_box.write_base_box(file_writer, copy_to_mem)))) {
      ERROR("Failed to write movie box.");
      ret = false;
      break;
    }
    if (m_movie_header_box.m_full_box.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_movie_header_box.write_movie_header_box(file_writer,
                                                                 copy_to_mem))) {
        ERROR("Failed to write movie header box.");
        ret = false;
        break;
      }
    }
    if (m_video_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_video_track.write_track_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write video track box.");
        ret = false;
        break;
      }
    } else {
      WARN("There is no video in this file");
    }
    if (m_audio_track.m_base_box.m_enable) {
      if (AM_UNLIKELY(!m_audio_track.write_track_box(file_writer, copy_to_mem))) {
        ERROR("Failed to write audio track box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("There is not audio in this file");
    }
  } while (0);
  return ret;
}

bool AV3MovieBox::read_movie_box(AV3FileReader* file_reader)
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
      AV3_BOX_TAG type = AV3_BOX_TAG(file_reader->get_box_type());
      switch (type) {
        case AV3_MOVIE_HEADER_BOX_TAG: {
          if (AM_UNLIKELY(!m_movie_header_box.read_movie_header_box(
              file_reader))) {
            ERROR("Failed to read movie header box.");
            ret = false;
          }
        } break;
        case AV3_TRACK_BOX_TAG: {
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

bool AV3MovieBox::get_proper_movie_box(AV3MovieBox& box,
                                       AV3MediaInfo& media_info)
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
      AV3SubMediaInfo sub_media_info;
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
      AV3SubMediaInfo sub_media_info;
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
  } while (0);
  return ret;
}

bool AV3MovieBox::copy_movie_box(AV3MovieBox& box)
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
  } while (0);
  return ret;
}

bool AV3MovieBox::get_combined_movie_box(AV3MovieBox& source_box,
                                         AV3MovieBox& combined_box)
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
  } while (0);
  return ret;
}

void AV3MovieBox::update_movie_box_size()
{
  m_base_box.m_size = AV3_BASE_BOX_SIZE;
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
}

bool AV3MovieBox::read_track_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    AV3TrackBox tmp_track;
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

bool AV3PSFMBox::write_psfm_box(AV3FileWriter* file_writer, bool copy_to_mem) {
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psfm box is not enabled.");
      ret = false;
      break;
    }
    if (!m_base_box.write_base_box(file_writer, copy_to_mem)) {
      ERROR("Failed to write base box in psfm box.");
      ret = false;
      break;
    }
    if (!file_writer->write_be_u32(m_reserved, copy_to_mem)) {
      ERROR("Failed to write reserved value in psfm box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3PSFMBox::read_psfm_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read psfm box.");
      ret = false;
      break;
    }
    if (!m_base_box.read_base_box(file_reader)) {
      ERROR("Failed to read base box in psfm box.");
      ret = false;
      break;
    }
    if (!file_reader->read_le_u32(m_reserved)) {
      ERROR("Failed to read reserved value in psfm box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read psfm box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("The read size is not equal to box size.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while(0);
  return ret;
}

bool AV3PSFMBox::get_proper_psfm_box(AV3PSFMBox& psfm_box, AV3MediaInfo& media_info)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psfm box is not enable.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(psfm_box.m_base_box);
    psfm_box.m_reserved = m_reserved;
  } while(0);
  return ret;
}

bool AV3PSFMBox::get_combined_psfm_box(AV3PSFMBox& source_box,
                                       AV3PSFMBox& combined_box)
{
  bool ret = true;
  do {
    if (!(m_base_box.m_enable && source_box.m_base_box.m_enable)) {
      ERROR("The psfm box is not enabled when get combined psfm box.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    combined_box.m_reserved = m_reserved;
  } while(0);
  return ret;
}

bool AV3PSFMBox::copy_psfm_box(AV3PSFMBox& box)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The psfm box is not enabled when copy psfm box.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(box.m_base_box);
    box.m_reserved = m_reserved;
  } while(0);
  return ret;
}

bool AV3XMLBox::write_xml_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The xml box is not enabled.");
      ret = false;
      break;
    }
    if (!m_full_box.write_full_box(file_writer, copy_to_mem)) {
      ERROR("Failed to write full box in xml box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AV3XMLBox::read_xml_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read xml box.");
      ret = false;
      break;
    }
    if (!m_full_box.read_full_box(file_reader)) {
      ERROR("Failed to read full box. when read xml box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read xml box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_full_box.m_base_box.m_size) {
      ERROR("The read size is not equal to box size.");
      ret = false;
      break;
    }
    m_full_box.m_base_box.m_enable = true;
  } while(0);
  return ret;
}

void AV3XMLBox::update_xml_box_size()
{
  m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE + m_data_length;
}

bool AV3XMLBox::get_proper_xml_box(AV3XMLBox& xml_box, AV3MediaInfo& media_info)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The xml box is not enabled.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(xml_box.m_full_box);
    xml_box.m_data_length = m_data_length;
  } while(0);
  return ret;
}

bool AV3XMLBox::get_combined_xml_box(AV3XMLBox& source_box, AV3XMLBox& combined_box)
{
  bool ret = true;
  do {
    if (!(m_full_box.m_base_box.m_enable &&
        source_box.m_full_box.m_base_box.m_enable)) {
      ERROR("The xml box is not enabled when get combined xml box.");
      ret = false;
      break;
    }
    m_full_box.copy_full_box(combined_box.m_full_box);
    combined_box.m_data_length = m_data_length;
  } while(0);
  return ret;
}

bool AV3XMLBox::copy_xml_box(AV3XMLBox& box)
{
  bool ret = true;
  do {
    if (!m_full_box.m_base_box.m_enable) {
      ERROR("The xml box is not enabled.");
      ret  = false;
      break;
    }
    m_full_box.copy_full_box(box.m_full_box);
    box.m_data_length = m_data_length;
  } while(0);
  return ret;
}

bool AV3UUIDBox::write_uuid_box(AV3FileWriter* file_writer, bool copy_to_mem)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The uuid box is not enabled.");
      ret = false;
      break;
    }
    if (!m_base_box.write_base_box(file_writer, copy_to_mem)) {
      ERROR("Failed to write base box when write uuid box.");
      ret = false;
      break;
    }
    if (!file_writer->write_data(m_uuid, sizeof(m_uuid), copy_to_mem)) {
      ERROR("Failed to write uuid data when write uuid box.");
      ret = false;
      break;
    }
    if (!file_writer->write_data(m_product_num, sizeof(m_product_num),
                                 copy_to_mem)) {
      ERROR("Failed to write product num when write uuid box.");
      ret = false;
      break;
    }
    if (!file_writer->write_be_u32(m_ip_address, copy_to_mem)) {
      ERROR("Failed to write ip address when write uuid box.");
      ret = false;
      break;
    }
    if (!file_writer->write_be_u32(m_num_event_log, copy_to_mem)) {
      ERROR("Failed to write num event log when write uuid box.");
      ret = false;
      break;
    }
    if (m_psfm_box.m_base_box.m_enable) {
      if (!m_psfm_box.write_psfm_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write psfm box.");
        ret = false;
        break;
      }
    }
    if (m_xml_box.m_full_box.m_base_box.m_enable) {
      if (!m_xml_box.write_xml_box(file_writer, copy_to_mem)) {
        ERROR("Failed to write xml box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3UUIDBox::read_uuid_box(AV3FileReader *file_reader)
{
  bool ret = true;
  do {
    uint64_t offset_begin = 0;
    uint64_t offset_end = 0;
    if (!file_reader->tell_file(offset_begin)) {
      ERROR("Failed to tell file when read uuid box.");
      ret = false;
      break;
    }
    if (!m_base_box.read_base_box(file_reader)) {
      ERROR("Failed to read base box when read uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->read_data(m_uuid, sizeof(m_uuid))) {
      ERROR("Failed to read uuid data when read uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->read_data(m_product_num, sizeof(m_product_num))) {
      ERROR("Failed to read product num when read uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->read_le_u32(m_ip_address)) {
      ERROR("Failed to read ip address when read uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->read_le_u32(m_num_event_log)) {
      ERROR("Failed to read num event log when read uuid box.");
      ret = false;
      break;
    }
    if (!m_psfm_box.read_psfm_box(file_reader)) {
      ERROR("Failed to read psfm box when read uuid box.");
      ret = false;
      break;
    }
    if (!m_xml_box.read_xml_box(file_reader)) {
      ERROR("Failed to read xml box when read uuid box.");
      ret = false;
      break;
    }
    if (!file_reader->tell_file(offset_end)) {
      ERROR("Failed to tell file when read uuid box.");
      ret = false;
      break;
    }
    if ((offset_end - offset_begin) != m_base_box.m_size) {
      ERROR("The read size is not equal to box size.");
      ret = false;
      break;
    }
    m_base_box.m_enable = true;
  } while(0);
  return ret;
}

void AV3UUIDBox::update_uuid_box_size()
{
  m_xml_box.update_xml_box_size();
  m_base_box.m_size = 2 * sizeof(uint32_t) + sizeof(m_uuid) +
      sizeof(m_product_num) + AV3_BASE_BOX_SIZE + m_psfm_box.m_base_box.m_size +
      m_xml_box.m_full_box.m_base_box.m_size;
}

bool AV3UUIDBox::get_proper_uuid_box(AV3UUIDBox& uuid_box, AV3MediaInfo& media_info)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The uuid box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(uuid_box.m_base_box);
    memcpy(uuid_box.m_uuid, m_uuid, sizeof(m_uuid));
    memcpy(uuid_box.m_product_num, m_product_num, sizeof(m_product_num));
    uuid_box.m_ip_address = m_ip_address;
    uuid_box.m_num_event_log = m_num_event_log;
    if (m_psfm_box.m_base_box.m_enable) {
      if (!m_psfm_box.get_proper_psfm_box(uuid_box.m_psfm_box, media_info)) {
        ERROR("Failed to get proper psfm box.");
        ret = false;
        break;
      }
    }
    if (m_xml_box.m_full_box.m_base_box.m_enable) {
      if (!m_xml_box.get_proper_xml_box(uuid_box.m_xml_box, media_info)) {
        ERROR("Failed to get proper xml box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3UUIDBox::get_combined_uuid_box(AV3UUIDBox& source_box,
                                       AV3UUIDBox& combined_box)
{
  bool ret = true;
  do {
    if (!(m_base_box.m_enable && source_box.m_base_box.m_enable)) {
      ERROR("The uuid box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(combined_box.m_base_box);
    memcpy(combined_box.m_uuid, m_uuid, sizeof(m_uuid));
    memcpy(combined_box.m_product_num, m_product_num, sizeof(m_product_num));
    combined_box.m_ip_address = m_ip_address;
    combined_box.m_num_event_log = m_num_event_log;
    if (m_psfm_box.m_base_box.m_enable &&
        source_box.m_psfm_box.m_base_box.m_enable) {
      if (!m_psfm_box.get_combined_psfm_box(source_box.m_psfm_box,
                                            combined_box.m_psfm_box)) {
        ERROR("Failed to get combined psfm box.");
        ret = false;
        break;
      }
    }
    if (m_xml_box.m_full_box.m_base_box.m_enable &&
        source_box.m_xml_box.m_full_box.m_base_box.m_enable) {
      if (!m_xml_box.get_combined_xml_box(source_box.m_xml_box,
                                          combined_box.m_xml_box)) {
        ERROR("Failed to get combined xml box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AV3UUIDBox::copy_uuid_box(AV3UUIDBox& box)
{
  bool ret = true;
  do {
    if (!m_base_box.m_enable) {
      ERROR("The uuid box is not enabled.");
      ret = false;
      break;
    }
    m_base_box.copy_base_box(box.m_base_box);
    memcpy(box.m_uuid, m_uuid, sizeof(m_uuid));
    memcpy(box.m_product_num, m_product_num, sizeof(m_product_num));
    box.m_ip_address = m_ip_address;
    box.m_num_event_log = m_num_event_log;
    if (m_psfm_box.m_base_box.m_enable) {
      if (!m_psfm_box.copy_psfm_box(box.m_psfm_box)) {
        ERROR("Failed to copy psfm box.");
        ret = false;
        break;
      }
    }
    if (m_xml_box.m_full_box.m_base_box.m_enable) {
      if (!m_xml_box.copy_xml_box(box.m_xml_box)) {
        ERROR("Failed to copy xml box.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}
