/*******************************************************************************
 * am_mp4_fixer.cpp
 *
 * History:
 *   2015-12-11 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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

#include "iso_base_defs.h"
#include "mp4_file_writer.h"
#include "mp4_file_reader.h"
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include "am_mp4_file_fixer.h"

/* seconds from 1904-01-01 00:00:00 UTC to 1969-12-31 24:00:00 UTC */
#define UTC_TIME_OFFSET    2082844800ULL
#define FIX_CHUNK_OFFSET64_ENABLE           0
#define FIX_OBJECT_DESC_BOX_ENABLE          0

AMIMp4FileFixer* AMIMp4FileFixer::create(const char* index_file_name,
                                         const char* damaged_mp4_name,
                                         const char* fixed_mp4_name)
{
  return AMMp4Fixer::create(index_file_name, damaged_mp4_name, fixed_mp4_name);
}

AMMp4Fixer* AMMp4Fixer::create(const char* index_file_name,
                               const char* damaged_mp4_name,
                               const char* fixed_mp4_name)
{
  AMMp4Fixer* result = new AMMp4Fixer();
  if (result
      && (!result->init(index_file_name, damaged_mp4_name, fixed_mp4_name))) {
    ERROR("Failed to init mp4 fixer.");
    delete result;
    result = nullptr;
  }
  return result;
}

AMMp4Fixer::AMMp4Fixer() :
    m_creation_time(0),
    m_modification_time(0),
    m_video_duration(0),
    m_audio_duration(0),
    m_gps_duration(0),
    m_file_writer(nullptr),
    m_file_reader(nullptr),
    m_index_file_name(nullptr),
    m_damaged_mp4_name(nullptr),
    m_fixed_mp4_name(nullptr),
    m_sps(nullptr),
    m_pps(nullptr),
    m_vps(nullptr),
    m_rate(1 << 16),
    m_video_track_id(1),
    m_audio_track_id(2),
    m_gps_track_id(3),
    m_next_track_id(4),
    m_video_frame_number(0),
    m_audio_frame_number(0),
    m_gps_frame_number(0),
    m_last_ctts(0),
    m_sps_length(0),
    m_pps_length(0),
    m_vps_length(0),
    m_media_data_length(0),
    m_audio_pkt_frame_number(1),
    m_original_movie_size(0),
    m_flags(0),
    m_volume(1 << 8),
    m_aac_spec_config(0),
    m_name_map(0),
    m_info_map(0),
    m_param_sets_map(0),
    m_used_version(0),
    m_have_B_frame(false)
{
  m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[5] = m_matrix[6] =
      m_matrix[7] = 0;
  m_matrix[0] = m_matrix[4] = 0x00010000;
  m_matrix[8] = 0x40000000;
}

void AMMp4Fixer::destroy()
{
  delete this;
}

AMMp4Fixer::~AMMp4Fixer()
{
  AM_DESTROY(m_file_writer);
  AM_DESTROY(m_file_reader);
  delete[] m_sps;
  delete[] m_pps;
  delete[] m_vps;
  delete[] m_index_file_name;
  delete[] m_damaged_mp4_name;
  delete[] m_fixed_mp4_name;
}

bool AMMp4Fixer::init(const char* index_file_name,
                      const char* damaged_mp4_name,
                      const char* fixed_mp4_name)
{
  bool ret = true;
  do {
    if ((m_index_file_name = new char[256]) == nullptr) {
      ERROR("Failed to malloc memory for index file name.");
      ret = false;
      break;
    }
    memset(m_index_file_name, 0, 256);
    if (!index_file_name) {
      memcpy(m_index_file_name, "/.mp4_index", 12);
    } else {
      m_name_map |= 0x01;
      memcpy(m_index_file_name, index_file_name, strlen(index_file_name));
    }
    if ((m_damaged_mp4_name = new char[256]) == nullptr) {
      ERROR("Failed to malloc memory for damaged mp4 file name.");
      ret = false;
      break;
    }
    memset(m_damaged_mp4_name, 0, 256);
    if (damaged_mp4_name) {
      m_name_map |= 0x02;
      memcpy(m_damaged_mp4_name, damaged_mp4_name, strlen(damaged_mp4_name));
    }
    if ((m_fixed_mp4_name = new char[256]) == nullptr) {
      ERROR("Failed to malloc memory for fixed mp4 name.");
      ret = false;
      break;
    }
    memset(m_fixed_mp4_name, 0, 256);
    if (fixed_mp4_name) {
      m_name_map |= 0x04;
      memcpy(m_fixed_mp4_name, fixed_mp4_name, strlen(fixed_mp4_name));
    }
    struct timeval val;
    if (gettimeofday(&val, nullptr) != 0) {
      PERROR("Get time of day :");
    } else {
      m_creation_time = val.tv_sec + UTC_TIME_OFFSET;
      m_modification_time = val.tv_sec + UTC_TIME_OFFSET;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::set_proper_damaged_mp4_name()
{
  bool ret = true;
  std::ifstream file;
  do {
    if (((m_name_map >> 1) & 0x01) != 0x01) {
      std::string read_line;
      std::string storage_str = "/storage/s";
      std::string sdcard_str = "/sdcard";
      size_t find_str_position = 0;
      std::string file_location_suffix;
      std::string::size_type pos = 0;
      std::string damaged_mp4_name = m_damaged_mp4_name;
      if (damaged_mp4_name.find(sdcard_str) != std::string::npos) {
        INFO("damaged mp4 file is on the sdcard.");
        break;
      }
      if (damaged_mp4_name.find(storage_str) == std::string::npos) {
        ERROR("damaged mp4 file is neither on the sdcard nor "
            "on the storage, error");
        ret = false;
        break;
      }
      file.open("/proc/self/mounts");
      INFO("mount information :");
      std::string temp_location;
      temp_location.clear();
      while (getline(file, read_line)) {
        INFO("%s", read_line.c_str());
        if ((find_str_position = read_line.find(storage_str))
            != std::string::npos) {
          for (uint32_t i = find_str_position; i < std::string::npos; ++ i) {
            if (read_line.substr(i, 1) != " ") {
              temp_location += read_line.substr(i, 1);
            } else {
              break;
            }
          }
          break;
        }
      }
      pos = storage_str.size() + 1;
      if ((pos = damaged_mp4_name.find('/', pos)) != std::string::npos) {
        file_location_suffix = damaged_mp4_name.substr(pos);
      } else {
        ERROR("find location suffix error.");
        ret = false;
        break;
      }
      damaged_mp4_name = temp_location + file_location_suffix;
      memset(m_damaged_mp4_name, 0, 256);
      memcpy(m_damaged_mp4_name, damaged_mp4_name.c_str(),
             damaged_mp4_name.size());
    }
  } while (0);
  if (file.is_open()) {
    file.close();
  }
  return ret;
}

bool AMMp4Fixer::set_proper_fixed_mp4_name()
{
  bool ret = true;
  do {
    if (((m_name_map >> 2) & 0x01) != 0x01) {
      std::string base_name;
      std::string damaged_mp4_name = m_damaged_mp4_name;
      std::string fixed_mp4_name;
      std::string::size_type pos = 0;
      base_name = damaged_mp4_name.substr(pos, damaged_mp4_name.size() - 4);
      fixed_mp4_name = base_name + "_fixed.mp4";
      memcpy(m_fixed_mp4_name, fixed_mp4_name.c_str(), fixed_mp4_name.size());
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::parse_index_file()
{
  bool ret = true;
  std::ifstream file;
  std::string read_line;
  do {
    file.open(m_index_file_name);
    if (getline(file, read_line)) {
      if (((m_name_map >> 1) & 0x01) != 0x01) {
        memcpy(m_damaged_mp4_name, read_line.c_str(), read_line.size());
      }
    } else {
      ERROR("The mp4 index file is empty.");
      ret = false;
      break;
    }
    while (getline(file, read_line)) {
      switch (INDEX_TAG_CREATE(read_line[0], read_line[1],
                               read_line[2], read_line[3])) {
        case VIDEO_INFO: {
          memcpy(&m_video_info, read_line.c_str() + 4, sizeof(AM_VIDEO_INFO));
          m_info_map |= 0x01;
        } break;
        case AUDIO_INFO: {
          memcpy(&m_audio_info, read_line.c_str() + 4, sizeof(AM_AUDIO_INFO));
          m_info_map |= 0x02;
          size_t find_str_position = 0;
          std::string find_str = "pkt_frame_count";
          char* end_ptr = nullptr;
          if ((find_str_position = read_line.find(find_str))
              != std::string::npos) {
            m_audio_pkt_frame_number = strtol(&(read_line.c_str()[
               find_str_position + find_str.size()]), &end_ptr, 10);
            INFO("audio packet frame counter is %d", m_audio_pkt_frame_number);
          }
          find_str_position = 0;
          find_str = "aac_spec_config";
          end_ptr = nullptr;
          if ((find_str_position = read_line.find(find_str))
              != std::string::npos) {
            m_aac_spec_config = strtol(&(read_line.c_str()[find_str_position
                               + find_str.size()]), &end_ptr, 10);
            INFO("aac spec config is %d", m_aac_spec_config);
          }
        } break;
        case VIDEO_VPS: {
          m_vps_length = read_line.size() - 4;
          if (m_vps_length > 0) {
            delete[] m_vps;
            if ((m_vps = new uint8_t[m_vps_length]) == nullptr) {
              ERROR("Failed to malloc memory for vps");
              ret = false;
              break;
            } else {
              memcpy(m_vps, read_line.c_str() + 4, m_vps_length);
              m_param_sets_map |= 0x01;
            }
          } else {
            ERROR("vps length is below zero, error.");
            ret = false;
            break;
          }
        } break;
        case VIDEO_SPS: {
          m_sps_length = read_line.size() - 4;
          if (m_sps_length > 0) {
            delete[] m_sps;
            if ((m_sps = new uint8_t[m_sps_length]) == nullptr) {
              ERROR("Failed to malloc memory for sps");
              ret = false;
              break;
            } else {
              memcpy(m_sps, read_line.c_str() + 4, m_sps_length);
              m_param_sets_map |= 0x02;
            }
          } else {
            ERROR("vps length is below zero, error.");
            ret = false;
            break;
          }
        } break;
        case VIDEO_PPS: {
          m_pps_length = read_line.size() - 4;
          if (m_pps_length > 0) {
            delete[] m_pps;
            if ((m_pps = new uint8_t[m_pps_length]) == nullptr) {
              ERROR("Failed to malloc memory for pps");
              ret = false;
              break;
            } else {
              memcpy(m_pps, read_line.c_str() + 4, m_pps_length);
              m_param_sets_map |= 0x04;
            }
          } else {
            ERROR("vps length is below zero, error.");
            ret = false;
            break;
          }
        } break;
        case VIDEO_INDEX: {
          int64_t delta_pts = 0;
          uint64_t offset = 0;
          uint32_t size = 0;
          uint8_t sync = 0;
          const char* beg_ptr = read_line.c_str() + 4;
          char* end_ptr = nullptr;
          delta_pts = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          offset = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          size = strtol(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          sync = strtol(beg_ptr, &end_ptr, 10);
          ++ m_video_frame_number;
          if ((!m_have_B_frame) && (delta_pts < 0)) {
            m_have_B_frame = true;
          }
//        INFO("video info delta pts is %llu, offset is %llu, frame size is %u,"
//           " sync is %u", delta_pts, offset, size, sync);
          if (!insert_video_composition_time_to_sample_box(delta_pts)) {
            ERROR("Failed to insert video composition time to sample box.");
            ret = false;
            break;
          }
          if (!insert_video_decoding_time_to_sample_box(delta_pts)) {
            ERROR("Failed to insert video decoding time to sample box.");
            ret = false;
            break;
          }
          if (!insert_video_chunk_offset_box(offset)) {
            ERROR("Failed to insert video chunk offset box.");
            ret = false;
            break;
          }
          if (!insert_video_sample_size_box(size)) {
            ERROR("Failed to insert video sample size box.");
            ret = false;
            break;
          }
          if (sync) {
            if (!insert_video_sync_sample_box(m_video_frame_number)) {
              ERROR("Failed to insert video sync_sample box.");
              ret = false;
              break;
            }
          }
          m_video_duration += delta_pts < 0 ? 0 : delta_pts;
        } break;
        case AUDIO_INDEX: {
          int64_t delta_pts = 0;
          uint64_t offset = 0;
          uint32_t size = 0;
          const char* beg_ptr = read_line.c_str() + 4;
          char* end_ptr = nullptr;
          delta_pts = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          offset = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          size = strtol(beg_ptr, &end_ptr, 10);
          INFO("audio info offset is %llu, frame size is %u", offset, size);
          ++ m_audio_frame_number;
          if (!insert_audio_decoding_time_to_sample_box(delta_pts)) {
            ERROR("Failed to insert audio decoding time to sample box.");
            ret = false;
            break;
          }
          if (!insert_audio_chunk_offset_box(offset)) {
            ERROR("Failed to insert audio chunk offset box.");
            ret = false;
            break;
          }
          if (!insert_audio_sample_size_box(size)) {
            ERROR("Failed to insert audio sample size box.");
            ret = false;
            break;
          }
          m_audio_duration += delta_pts;
        } break;
        case GPS_INDEX: {
          int64_t delta_pts = 0;
          uint64_t offset = 0;
          uint32_t size = 0;
          const char* beg_ptr = read_line.c_str() + 4;
          char* end_ptr = nullptr;
          delta_pts = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          offset = strtoll(beg_ptr, &end_ptr, 10);
          beg_ptr = end_ptr + 1;
          end_ptr = nullptr;
          size = strtol(beg_ptr, &end_ptr, 10);
          INFO("gps info delta pts is %llu, offset is %llu, frame size is %u",
               delta_pts, offset, size);
          if (!insert_gps_decoding_time_to_sample_box(delta_pts)) {
            ERROR("Failed to insert gps decoding time to sample box.");
            ret = false;
            break;
          }
          if (!insert_gps_chunk_offset_box(offset)) {
            ERROR("Failed to insert gps chunk offset box.");
            ret = false;
            break;
          }
          if (!insert_gps_sample_size_box(size)) {
            ERROR("Failed to insert gps sample size box.");
            ret = false;
            break;
          }
          m_gps_duration += delta_pts < 0 ? 0 : delta_pts;
          ++ m_gps_frame_number;
        } break;
        default: {
          ERROR("mp4 index tag error.");
          ret = false;
        } break;
      }
    }
  } while (0);
  NOTICE("video frame number is %u, audio frame number is %u",
         m_video_frame_number, m_audio_frame_number);
  file.close();
  return ret;
}

bool AMMp4Fixer::build_mp4_header()
{
  bool ret = true;
  do {
    fill_file_type_box();
    fill_media_data_box();
    if (!fill_movie_box()) {
      ERROR("Failed to fill movie box.");
      ret = false;
      break;
    }
    m_movie_box.update_movie_box_size();
  } while (0);
  return ret;
}

bool AMMp4Fixer::fix_mp4_file()
{
  bool ret = true;
  do {
    if (!parse_index_file()) {
      ERROR("Failed to parse index file.");
      ret = false;
      break;
    }
    if (!set_proper_damaged_mp4_name()) {
      ERROR("Failed to set proper damaged mp4 name.");
      ret = false;
      break;
    }
    if (!set_proper_fixed_mp4_name()) {
      ERROR("Failed to set proper fixed mp4 name.");
      ret = false;
      break;
    }
    if (!build_mp4_header()) {
      ERROR("Failed to build mp4_header.");
      ret = false;
      break;
    }
    if ((m_file_reader = Mp4FileReader::create(m_damaged_mp4_name))
        == nullptr) {
      ERROR("Failed to create file reader.");
      ret = false;
      break;
    }
    if ((m_file_writer = Mp4FileWriter::create()) == nullptr) {
      ERROR("Failed to create file writer.");
      ret = false;
      break;
    }
    if (!m_file_writer->create_file(m_fixed_mp4_name)) {
      ERROR("Failed to create file %s", m_fixed_mp4_name);
      ret = false;
      break;
    }
    if (!m_file_type_box.write_file_type_box(m_file_writer)) {
      ERROR("Failed to write file type box.");
      ret = false;
      break;
    }
    m_original_movie_size = m_movie_box.m_base_box.m_size;
    NOTICE("Original movie box size is %u", m_original_movie_size);
    if (!m_file_writer->seek_file(m_original_movie_size, SEEK_CUR)) {
      ERROR("Failed to seek data when fix mp4 file.");
      ret = false;
      break;
    }
    uint64_t offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu before write media data box", offset);
    if (!m_media_data_box.write_media_data_box(m_file_writer)) {
      ERROR("Failed to write media data box.");
      ret = false;
      break;
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu after write media data box", offset);
    if (m_movie_box.m_video_track.m_base_box.m_enable) {
      if (!write_video_data_to_file()) {
        ERROR("Failed to write video data to file.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video track is not enabled.");
      ret = false;
      break;
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu after write video data", offset);
    if (m_movie_box.m_audio_track.m_base_box.m_enable) {
      if (!write_audio_data_to_file()) {
        ERROR("Failed to write audio data to file.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio track is not enabled.");
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu after write audio data", offset);
    if (m_movie_box.m_gps_track.m_base_box.m_enable) {
      if (!write_gps_data_to_file()) {
        ERROR("Failed to write gps data to file.");
        ret = false;
        break;
      }
    } else {
      INFO("The gps track is not enabled.");
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu after write gps data", offset);
    if (!m_file_writer->seek_file(m_file_type_box.m_base_box.m_size,
                                  SEEK_SET)) {
      ERROR("Failed to seek data when fix mp4 file.");
      ret = false;
      break;
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    m_movie_box.update_movie_box_size();
    NOTICE("file offset is %llu before write movie box", offset);
    NOTICE("movie box size is %u", m_movie_box.m_base_box.m_size);
    if (!m_movie_box.write_movie_box(m_file_writer)) {
      ERROR("Failed to write movie box.");
      ret = false;
      break;
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu before write free box", offset);
    uint32_t free_box_size = m_original_movie_size
        - m_movie_box.m_base_box.m_size;
    NOTICE("free box size is %u", free_box_size);
    if (free_box_size > 0) {
      m_free_box.m_base_box.m_enable = true;
      m_free_box.m_base_box.m_size = free_box_size;
      m_free_box.m_base_box.m_type = DISOM_FREE_BOX_TAG;
      if (!m_free_box.write_free_box(m_file_writer)) {
        ERROR("Failed to write free box.");
        ret = false;
        break;
      }
      if (!m_file_writer->seek_file(free_box_size - DISOM_BASE_BOX_SIZE,
                                    SEEK_CUR)) {
        ERROR("Failed to seek file.");
        ret = false;
        break;
      }
    }
    offset = 0;
    m_file_writer->tell_file(offset);
    NOTICE("file offset is %llu after write free box", offset);
    m_media_data_box.m_base_box.m_size += m_media_data_length;
    NOTICE("media data box size is %u", m_media_data_box.m_base_box.m_size);
    if (!m_file_writer->write_be_u32(m_media_data_box.m_base_box.m_size)) {
      ERROR("Failed to write big endian 32 bits value.");
      ret = false;
      break;
    }
  } while (0);
  if (m_file_reader) {
    m_file_reader->close_file();
  }
  if (m_file_writer) {
    m_file_writer->close_file(true);
  }
  return ret;
}

bool AMMp4Fixer::write_video_data_to_file()
{
  bool ret = true;
  bool is_file_end = false;
  uint8_t* data_buf = nullptr;
  uint32_t data_buf_size = 0;
  do {
    ChunkOffsetBox& video_offset_box =
        m_movie_box.m_video_track.m_media.\
        m_media_info.m_sample_table.m_chunk_offset;
    SampleSizeBox& video_size_box =
        m_movie_box.m_video_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    DecodingTimeToSampleBox& video_decoding_box =
        m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    CompositionTimeToSampleBox& video_composition_box =
        m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_ctts;
    SyncSampleTableBox& sync_box =
        m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_sync_sample;
    uint32_t i = 0;
    for (; i < video_offset_box.m_entry_count; ++ i) {
      uint32_t offset = video_offset_box.m_chunk_offset[i];
      if (!m_file_reader->seek_file(offset)) {
        ERROR("Failed to seek file.");
        ret = false;
        break;
      }
      uint32_t size = video_size_box.m_size_array[i];
      if (data_buf_size < size) {
        delete[] data_buf;
        if ((data_buf = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for data buf.");
          ret = false;
          break;
        }
        data_buf_size = size;
      }
      if (m_file_reader->read_data(data_buf, size) != (int32_t) size) {
        uint64_t offset = 0;
        if (!m_file_reader->tell_file(offset)) {
          ERROR("Failed to tell file");
          ret = false;
          break;
        }
        if (offset >= m_file_reader->get_file_size()) {
          NOTICE("Read end of  the file.");
          is_file_end = true;
          break;
        } else {
          ERROR("Read data error.");
          ret = false;
          break;
        }
      }
      uint64_t write_offset = 0;
      if (!m_file_writer->tell_file(write_offset)) {
        ERROR("Failed to tell file.");
        ret = false;
        break;
      }
      if (!m_file_writer->write_data(data_buf, size)) {
        ERROR("Failed to write data to fixed mp4 file.");
        ret = false;
        break;
      }
      video_offset_box.m_chunk_offset[i] = write_offset;
      m_media_data_length += size;
    }
    if (is_file_end) {
      video_offset_box.m_entry_count = i;
      video_size_box.m_sample_count = i;
      video_decoding_box.m_entry_count = i;
      video_composition_box.m_entry_count = i;
      if (sync_box.m_sync_sample_table[sync_box.m_stss_count - 1] > (i + 1)) {
        sync_box.m_stss_count -= 1;
      }
      m_movie_box.update_movie_box_size();
      m_video_duration = 0;
      for (uint32_t j = 0; j < video_decoding_box.m_entry_count; ++ j) {
        m_video_duration += video_decoding_box.m_entry_ptr[i].sample_delta;
      }
      m_movie_box.m_movie_header_box.m_duration = m_video_duration;
      m_movie_box.m_video_track.m_track_header.m_duration = m_video_duration;
      m_movie_box.m_video_track.m_media.m_media_header.m_duration =
          m_video_duration;
    }
  } while (0);
  delete[] data_buf;
  return ret;
}

bool AMMp4Fixer::write_audio_data_to_file()
{
  bool ret = true;
  bool is_file_end = false;
  uint8_t* data_buf = nullptr;
  uint32_t data_buf_size = 0;
  do {
    ChunkOffsetBox& audio_offset_box =
        m_movie_box.m_audio_track.m_media.\
m_media_info.m_sample_table.m_chunk_offset;
    SampleSizeBox& audio_size_box =
        m_movie_box.m_audio_track.m_media.\
m_media_info.m_sample_table.m_sample_size;
    DecodingTimeToSampleBox& audio_decoding_box =
        m_movie_box.m_audio_track.\
m_media.m_media_info.m_sample_table.m_stts;
    uint32_t i = 0;
    for (; i < audio_offset_box.m_entry_count; ++ i) {
      uint32_t offset = audio_offset_box.m_chunk_offset[i];
      if (!m_file_reader->seek_file(offset)) {
        ERROR("Failed to seek file.");
        ret = false;
        break;
      }
      uint32_t size = audio_size_box.m_size_array[i];
      if (data_buf_size < size) {
        delete[] data_buf;
        if ((data_buf = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for data buf.");
          ret = false;
          break;
        }
        data_buf_size = size;
      }
      if (m_file_reader->read_data(data_buf, size) != (int32_t) size) {
        uint64_t read_offset = 0;
        if (!m_file_reader->tell_file(read_offset)) {
          ERROR("Failed to tell file");
          ret = false;
          break;
        }
        if (read_offset >= m_file_reader->get_file_size()) {
          NOTICE("Read end of  the file.");
          is_file_end = true;
          break;
        } else {
          ERROR("Read data error.");
          ret = false;
          break;
        }
      }
      uint64_t write_offset = 0;
      if (!m_file_writer->tell_file(write_offset)) {
        ERROR("Failed to tell file.");
        ret = false;
        break;
      }
      if (!m_file_writer->write_data(data_buf, size)) {
        ERROR("Failed to write data to fixed mp4 file.");
        ret = false;
        break;
      }
      audio_offset_box.m_chunk_offset[i] = write_offset;
      m_media_data_length += size;
    }
    if (is_file_end) {
      audio_offset_box.m_entry_count = i;
      audio_size_box.m_sample_count = i;
      audio_decoding_box.m_entry_count = i;
      m_movie_box.update_movie_box_size();
      m_movie_box.m_audio_track.m_track_header.m_duration = m_audio_duration;
      m_movie_box.m_audio_track.m_media.m_media_header.m_duration =
          m_audio_duration;
    }
  } while (0);
  delete[] data_buf;
  return ret;
}

bool AMMp4Fixer::write_gps_data_to_file()
{
  bool ret = true;
  bool is_file_end = false;
  uint8_t* data_buf = nullptr;
  uint32_t data_buf_size = 0;
  do {
    ChunkOffsetBox& gps_offset_box =
        m_movie_box.m_gps_track.m_media.\
        m_media_info.m_sample_table.m_chunk_offset;
    SampleSizeBox& gps_size_box =
        m_movie_box.m_gps_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    DecodingTimeToSampleBox& gps_decoding_box =
        m_movie_box.m_gps_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    uint32_t i = 0;
    for (; i < gps_offset_box.m_entry_count; ++ i) {
      uint32_t offset = gps_offset_box.m_chunk_offset[i];
      if (!m_file_reader->seek_file(offset)) {
        ERROR("Failed to seek file.");
        ret = false;
        break;
      }
      uint32_t size = gps_size_box.m_size_array[i];
      if (data_buf_size < size) {
        delete[] data_buf;
        if ((data_buf = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for data buf.");
          ret = false;
          break;
        }
        data_buf_size = size;
      }
      if (m_file_reader->read_data(data_buf, size) != (int32_t) size) {
        uint64_t read_offset = 0;
        if (!m_file_reader->tell_file(read_offset)) {
          ERROR("Failed to tell file");
          ret = false;
          break;
        }
        if (read_offset >= m_file_reader->get_file_size()) {
          NOTICE("Read end of  the file.");
          is_file_end = true;
          break;
        } else {
          ERROR("Read data error.");
          ret = false;
          break;
        }
      }
      uint64_t write_offset = 0;
      if (!m_file_writer->tell_file(write_offset)) {
        ERROR("Failed to tell file.");
        ret = false;
        break;
      }
      if (!m_file_writer->write_data(data_buf, size)) {
        ERROR("Failed to write data to fixed mp4 file.");
        ret = false;
        break;
      }
      gps_offset_box.m_chunk_offset[i] = write_offset;
      m_media_data_length += size;
    }
    if (is_file_end) {
      gps_offset_box.m_entry_count = i;
      gps_size_box.m_sample_count = i;
      gps_decoding_box.m_entry_count = i;
      m_movie_box.update_movie_box_size();
      m_movie_box.m_gps_track.m_track_header.m_duration = m_gps_duration;
      m_movie_box.m_gps_track.m_media.m_media_header.m_duration =
          m_gps_duration;
    }
  } while (0);
  delete[] data_buf;
  return ret;
}

void AMMp4Fixer::fill_file_type_box()
{
  FileTypeBox& box = m_file_type_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_FILE_TYPE_BOX_TAG;
  box.m_major_brand = DISOM_BRAND_V0_TAG;
  box.m_minor_version = 0;
  box.m_compatible_brands[0] = DISOM_BRAND_V0_TAG;
  box.m_compatible_brands[1] = DISOM_BRAND_V1_TAG;
  box.m_compatible_brands[2] = DISOM_BRAND_AVC_TAG;
  box.m_compatible_brands[3] = DISOM_BRAND_MPEG4_TAG;
  box.m_compatible_brands_number = 4;
  box.m_base_box.m_size = DISOM_BASE_BOX_SIZE
      + sizeof(m_file_type_box.m_major_brand)
      + sizeof(m_file_type_box.m_minor_version)
      + (m_file_type_box.m_compatible_brands_number * DISOM_TAG_SIZE);
}

void AMMp4Fixer::fill_media_data_box()
{
  MediaDataBox& box = m_media_data_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_MEDIA_DATA_BOX_TAG;
  box.m_base_box.m_size = DISOM_BASE_BOX_SIZE;
}

void AMMp4Fixer::fill_movie_header_box()
{
  MovieHeaderBox& box = m_movie_box.m_movie_header_box;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MOVIE_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = m_video_duration;
  box.m_rate = m_rate;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_next_track_ID = m_next_track_id;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MOVIE_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MOVIE_HEADER_64_SIZE;
  }
}

void AMMp4Fixer::fill_object_desc_box()
{
  ObjectDescBox& box = m_movie_box.m_iods_box;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_size = 24;
  box.m_full_box.m_base_box.m_type = DIDOM_OBJECT_DESC_BOX_TAG;
  box.m_iod_type_tag = 0x10;
  box.m_extended_desc_type[0] = box.m_extended_desc_type[1] =
      box.m_extended_desc_type[2] = 0x80;
  box.m_desc_type_length = 7;
  box.m_od_id[0] = 0;
  box.m_od_id[1] = 0x4f;
  box.m_od_profile_level = 0xff;
  box.m_scene_profile_level = 0xff;
  box.m_audio_profile_level = 0x02;
  box.m_video_profile_level = 0x01;
  box.m_graphics_profile_level = 0xff;
}

void AMMp4Fixer::fill_video_track_header_box()
{
  TrackHeaderBox& box = m_movie_box.m_video_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x01;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_video_track_id;
  box.m_duration = m_video_duration;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_width_integer = m_video_info.width;
  box.m_width_decimal = 0;
  box.m_height_integer = m_video_info.height;
  box.m_height_decimal = 0;

  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_64_SIZE;
  }
}

void AMMp4Fixer::fill_audio_track_header_box()
{
  TrackHeaderBox& box = m_movie_box.m_audio_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x07;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_audio_track_id;
  box.m_duration = m_audio_duration;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_64_SIZE;
  }
}

void AMMp4Fixer::fill_gps_track_header_box()
{
  TrackHeaderBox& box = m_movie_box.m_gps_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x07;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_gps_track_id;
  box.m_duration = m_gps_duration;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_64_SIZE;
  }
}
void AMMp4Fixer::fill_media_header_box_for_video_track()
{
  MediaHeaderBox& box = m_movie_box.m_video_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000; //the number of time units that pass in one second.
  box.m_duration = m_video_duration;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_64_SIZE;
  }
}

void AMMp4Fixer::fill_media_header_box_for_audio_track()
{
  MediaHeaderBox& box = m_movie_box.m_audio_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = m_audio_duration;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_64_SIZE;
  }
}

void AMMp4Fixer::fill_media_header_box_for_gps_track()
{
  MediaHeaderBox& box = m_movie_box.m_gps_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = m_gps_duration;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_64_SIZE;
  }
}

bool AMMp4Fixer::fill_video_handler_reference_box()
{
  bool ret = true;
  do {
    HandlerReferenceBox& box =
        m_movie_box.m_video_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_handler_type = VIDEO_TRACK;
    switch (m_video_info.type) {
      case AM_VIDEO_H264 : {
        if (!box.m_name) {
          box.m_name_size = strlen(VIDEO_H264_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = false;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, VIDEO_H264_COMPRESSORNAME, box.m_name_size);
        }
      } break;
      case AM_VIDEO_H265 : {
        if (!box.m_name) {
          box.m_name_size = strlen(VIDEO_H265_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = false;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, VIDEO_H265_COMPRESSORNAME, box.m_name_size);
        }
      }break;
      default : {
        ERROR("The video type %u is not supported currently.");
        ret = false;
      }break;
    }
    if(!ret) {
      break;
    }
    box.m_full_box.m_base_box.m_size = DISOM_HANDLER_REF_SIZE + box.m_name_size;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_handler_reference_box()
{
  bool ret = true;
  do {
    HandlerReferenceBox& box =
        m_movie_box.m_audio_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = 0;
    box.m_handler_type = AUDIO_TRACK;
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC : {
        if (!box.m_name) {
          box.m_name_size = strlen(AUDIO_AAC_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = false;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, AUDIO_AAC_COMPRESSORNAME, box.m_name_size);
        }
      } break;
      case AM_AUDIO_OPUS : {
        if (!box.m_name) {
          box.m_name_size = strlen(AUDIO_OPUS_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = false;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, AUDIO_OPUS_COMPRESSORNAME, box.m_name_size);
        }
      }break;
      default : {
        ERROR("The audio type %u is not supported currently.");
        ret = false;
      }break;
    }
    if(!ret) {
      break;
    }
    box.m_full_box.m_base_box.m_size = DISOM_HANDLER_REF_SIZE + box.m_name_size;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_handler_reference_box()
{
  bool ret = true;
  do {
    HandlerReferenceBox& box =
        m_movie_box.m_gps_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = 0;
    box.m_handler_type = TIMED_METADATA_TRACK;
    if (!box.m_name) {
      box.m_name_size = strlen(GPS_COMPRESSORNAME);
      box.m_name = new char[box.m_name_size];
      if (!box.m_name) {
        ERROR("Failed to malloc memory in fill video hander reference box.");
        ret = false;
        break;
      } else {
        memset(box.m_name, 0, box.m_name_size);
      }
    }
    memcpy(box.m_name, GPS_COMPRESSORNAME,  box.m_name_size);
    box.m_full_box.m_base_box.m_size = DISOM_HANDLER_REF_SIZE + box.m_name_size;
  } while (0);
  return ret;
}

void AMMp4Fixer::fill_video_chunk_offset_box()
{
  ChunkOffsetBox& box = m_movie_box.m_video_track.m_media.\
       m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_video_chunk_offset64_box()
{
  ChunkOffset64Box& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset64;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET64_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_audio_chunk_offset_box()
{
  ChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.\
               m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_audio_chunk_offset64_box()
{
  ChunkOffset64Box& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset64;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET64_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_gps_chunk_offset_box()
{
  ChunkOffsetBox& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_gps_chunk_offset64_box()
{
  ChunkOffset64Box& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset64;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET64_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMp4Fixer::fill_video_sync_sample_box()
{
  SyncSampleTableBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_sync_sample;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SYNC_SAMPLE_TABLE_BOX_TAG;
  box.m_full_box.m_flags = m_flags;
  box.m_full_box.m_version = m_used_version;
}

bool AMMp4Fixer::fill_video_sample_to_chunk_box()
{
  bool ret = true;
  SampleToChunkBox& box = m_movie_box.m_video_track.m_media.\
       m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new SampleToChunkEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("fill_sample_to_chunk_box: no memory\n");
        ret = false;
        break;
      }
    }
    box.m_entrys[0].m_first_chunk = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_sample_to_chunk_box()
{
  bool ret = true;
  SampleToChunkBox& box = m_movie_box.m_audio_track.m_media.\
             m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new SampleToChunkEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("fill_sample_to_chunk_box: no memory\n");
        ret = false;
        break;
      }
    }
    box.m_entrys[0].m_first_chunk = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_sample_to_chunk_box()
{
  bool ret = true;
  SampleToChunkBox& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new SampleToChunkEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("fill_sample_to_chunk_box: no memory\n");
        ret = false;
        break;
      }
    }
    box.m_entrys[0].m_first_chunk = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

void AMMp4Fixer::fill_video_sample_size_box()
{
  SampleSizeBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0;
}

void AMMp4Fixer::fill_audio_sample_size_box()
{
  SampleSizeBox& box = m_movie_box.m_audio_track.m_media.\
       m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0;
}

void AMMp4Fixer::fill_gps_sample_size_box()
{
  SampleSizeBox& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

bool AMMp4Fixer::fill_avc_decoder_configuration_box()
{
  bool ret = true;
  do {
    AVCDecoderConfigurationRecord& box = m_movie_box.m_video_track.m_media.\
   m_media_info.m_sample_table.m_sample_description.m_visual_entry.m_avc_config;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG;
    box.m_config_version = 1;
    box.m_profile_indication = m_avc_sps_struct.profile_idc;
    box.m_profile_compatibility = 0;
    box.m_level_indication = m_avc_sps_struct.level_idc;
    box.m_reserved = 0xff;
    box.m_len_size_minus_one = 3;
    box.m_reserved_1 = 0xff;
    box.m_sps_num = 1;
    box.m_sps_len = m_sps_length;
    if (box.m_sps == nullptr) {
      box.m_sps = new uint8_t[m_sps_length];
      if (!box.m_sps) {
        ERROR("Failed to malloc memory when fill avc_decoder configuration "
              "record box");
        ret = false;
        break;
      }
      if (m_sps == nullptr) {
        ERROR("m_avc_sps is nullptr");
        ret = false;
        break;
      }
      memcpy(box.m_sps, m_sps, m_sps_length);
    }
    box.m_pps_num = 1;
    box.m_pps_len = m_pps_length;
    if (box.m_pps == nullptr) {
      box.m_pps = new uint8_t[m_pps_length];
      if (!box.m_pps) {
        ERROR("Failed to malloc memory when fill avc decoder configuration box.");
        ret = false;
        break;
      }
      if (m_pps == nullptr) {
        ERROR("m_avc_pps is nullptr");
        ret = false;
        break;
      }
      memcpy(box.m_pps, m_pps, m_pps_length);
    }
    box.m_base_box.m_size = DISOM_AVC_DECODER_CONFIG_RECORD_SIZE
        + box.m_sps_len + box.m_pps_len;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_hevc_decoder_configuration_box()
{
  bool ret = true;
  do {
    HEVCDecoderConfigurationRecord& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_sample_description.\
       m_visual_entry.m_HEVC_config;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG;
    box.m_configuration_version = 1;
    box.m_general_profile_space = m_hevc_vps_struct.ptl.general_profile_space;
    box.m_general_tier_flag = m_hevc_vps_struct.ptl.general_tier_flag;
    box.m_general_profile_idc = m_hevc_vps_struct.ptl.general_profile_idc;
    for (uint32_t i = 0; i < 32; ++ i) {
      box.m_general_profile_compatibility_flags |=
       m_hevc_vps_struct.ptl.general_profile_compatibility_flag[i] << (31 - i);
    }
    box.m_general_constraint_indicator_flags_high =
     (((uint16_t) m_hevc_vps_struct.ptl.general_progressive_source_flag) << 15)
   | (((uint16_t) m_hevc_vps_struct.ptl.general_interlaced_source_flag) << 14)
   | (((uint16_t) m_hevc_vps_struct.ptl.general_non_packed_constraint_flag)
       << 13)
   | (((uint16_t) m_hevc_vps_struct.ptl.general_frame_only_constraint_flag)
       << 12);
    box.m_general_constraint_indicator_flags_low = 0;
    box.m_general_level_idc = m_hevc_vps_struct.ptl.general_level_idc;
    box.m_min_spatial_segmentation_idc =
        (uint16_t) m_hevc_sps_struct.vui.min_spatial_segmentation_idc;
    box.m_parallelism_type = 0;
    box.m_chroma_format = (uint8_t) m_hevc_sps_struct.chroma_format_idc;
    box.m_bit_depth_luma_minus8 = m_hevc_sps_struct.pcm.bit_depth_chroma - 8;
    box.m_bit_depth_chroma_minus8 = m_hevc_sps_struct.pcm.bit_depth_chroma - 8;
    box.m_avg_frame_rate = 0;
    box.m_constant_frame_rate = 0;
    box.m_num_temporal_layers = m_hevc_sps_struct.num_temporal_layers;
    box.m_temporal_id_nested = m_hevc_vps_struct.vps_temporal_id_nesting_flag;
    box.m_length_size_minus_one = 3;
    box.m_num_of_arrays = 3; //vps,sps, pps.if have SEI, this value should be 4.
    if (!box.m_HEVC_config_array) {
      box.m_HEVC_config_array = new HEVCConArray[3];
      if (AM_UNLIKELY(!box.m_HEVC_config_array)) {
        ERROR("Failed to malloc memory when fill hevc decoder configuration "
              "record box.");
        ret = false;
        break;
      }
    }
    box.m_HEVC_config_array[0].m_array_completeness = 1;
    box.m_HEVC_config_array[0].m_reserved = 0;
    box.m_HEVC_config_array[0].m_NAL_unit_type = H265_VPS;
    box.m_HEVC_config_array[0].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[0].m_array_units) {
      box.m_HEVC_config_array[0].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[0].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = false;
        break;
      }
      box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit_length =
          m_vps_length;
      delete[] box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit =
          new uint8_t[m_vps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for vps");
        ret = false;
        break;
      }
      memcpy(box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit,
             m_vps, m_vps_length);
    }
    box.m_HEVC_config_array[1].m_array_completeness = 1;
    box.m_HEVC_config_array[1].m_reserved = 0;
    box.m_HEVC_config_array[1].m_NAL_unit_type = H265_SPS;
    box.m_HEVC_config_array[1].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[1].m_array_units) {
      box.m_HEVC_config_array[1].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[1].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = false;
        break;
      }
      box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit_length =
          m_sps_length;
      delete[] box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit =
          new uint8_t[m_sps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for sps");
        ret = false;
        break;
      }
      memcpy(box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit,
             m_sps, m_sps_length);
    }
    box.m_HEVC_config_array[2].m_array_completeness = 1;
    box.m_HEVC_config_array[2].m_reserved = 0;
    box.m_HEVC_config_array[2].m_NAL_unit_type = H265_PPS;
    box.m_HEVC_config_array[2].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[2].m_array_units) {
      box.m_HEVC_config_array[2].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[2].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = false;
        break;
      }
      box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit_length =
          m_pps_length;
      delete[] box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit =
          new uint8_t[m_pps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for pps");
        ret = false;
        break;
      }
      memcpy(box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit,
             m_pps, m_pps_length);
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_visual_sample_description_box()
{
  bool ret = true;
  do {
    SampleDescriptionBox& box = m_movie_box.m_video_track.\
       m_media.m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    switch (m_video_info.type) {
      case AM_VIDEO_H264: {
        box.m_visual_entry.m_base_box.m_type = DISOM_H264_ENTRY_TAG;
        box.m_visual_entry.m_base_box.m_enable = true;
        memset(box.m_visual_entry.m_compressorname, 0,
              sizeof(box.m_visual_entry.m_compressorname));
        strncpy(box.m_visual_entry.m_compressorname,
                  VIDEO_H264_COMPRESSORNAME,
                 (sizeof(box.m_visual_entry.m_compressorname) - 1));
        if (AM_UNLIKELY(fill_avc_decoder_configuration_box() != true)) {
          ERROR("Failed to fill avc decoder configuration record box.");
          ret = false;
          break;
        }
      } break;
      case AM_VIDEO_H265: {
        box.m_visual_entry.m_base_box.m_type = DISOM_H265_ENTRY_TAG;
        box.m_visual_entry.m_base_box.m_enable = true;
        memset(box.m_visual_entry.m_compressorname, 0,
              sizeof(box.m_visual_entry.m_compressorname));
        strncpy(box.m_visual_entry.m_compressorname,
                VIDEO_H265_COMPRESSORNAME,
               (sizeof(box.m_visual_entry.m_compressorname) - 1));
        if (AM_UNLIKELY(fill_hevc_decoder_configuration_box() != true)) {
          ERROR("Failed to fill hevc decoder configuration record box.");
          ret = false;
          break;
        }
      } break;
      default: {
        ERROR("The video type %u is not supported currently.");
        ret = false;
        break;
      }
    }
    if (!ret) {
      break;
    }
    box.m_visual_entry.m_data_reference_index = 1;
    box.m_visual_entry.m_width = m_video_info.width;
    box.m_visual_entry.m_height = m_video_info.height;
    box.m_visual_entry.m_horizresolution = 0x00480000;
    box.m_visual_entry.m_vertresolution = 0x00480000;
    box.m_visual_entry.m_frame_count = 1;
    box.m_visual_entry.m_depth = 0x0018;
    box.m_visual_entry.m_pre_defined_2 = -1;
    box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE
        + sizeof(uint32_t) + box.m_visual_entry.m_base_box.m_size;
  } while (0);
  return ret;
}

void AMMp4Fixer::fill_aac_description_box()
{
  AACDescriptorBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_description.m_audio_entry.m_aac;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_AAC_DESCRIPTOR_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  /*ES descriptor*/
  box.m_es_descriptor_type_tag = 3;
  box.m_es_descriptor_type_length = 34;
  box.m_es_id = 0;
  box.m_stream_priority = 0;
  /*decoder config descriptor*/
  box.m_decoder_config_descriptor_type_tag = 4;
  box.m_decoder_descriptor_type_length = 22;
  box.m_object_type = 0x40; //MPEG-4 AAC = 64;
  box.m_buffer_size = 8192;
  box.m_max_bitrate = 128000;
  box.m_avg_bitrate = 128000;
  /*decoder specific info descriptor*/
  box.m_decoder_specific_descriptor_type_tag = 5;
  box.m_decoder_specific_descriptor_type_length = 5;
  box.m_audio_spec_config = m_aac_spec_config;
  /*SL descriptor*/
  box.m_SL_descriptor_type_tag = 6;
  box.m_SL_value = 0x02;
  box.m_SL_descriptor_type_length = 1;
  box.m_full_box.m_base_box.m_size = DISOM_AAC_DESCRIPTOR_SIZE;
}

bool AMMp4Fixer::fill_opus_description_box()
{
  bool ret = true;
  do {
    OpusSpecificBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_description.m_audio_entry.m_opus;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_OPUS_ENTRY_TAG;
    box.m_version = 0;
    box.m_output_channel_count = m_audio_info.channels;
    box.m_pre_skip = 0;
    box.m_input_sample_rate = m_audio_info.sample_rate;
    box.m_output_gain = 0;
    box.m_channel_mapping_family = 0;
    box.m_base_box.m_size = OPUS_SPECIFIC_BOX_SIZE;
    if (box.m_channel_mapping_family) {
      box.m_base_box.m_size += 2 + box.m_output_channel_count;
      if (!box.m_channel_mapping) {
        box.m_channel_mapping = new uint8_t[box.m_output_channel_count];
        if (!box.m_channel_mapping) {
          ERROR("Failed to malloc memory when fill opus description box.");
          ret = false;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_sample_description_box()
{
  bool ret = true;
  do {
    SampleDescriptionBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC: {
        box.m_audio_entry.m_base_box.m_type = DISOM_AAC_ENTRY_TAG;
      } break;
      case AM_AUDIO_OPUS: {
        box.m_audio_entry.m_base_box.m_type = DISOM_OPUS_ENTRY_TAG;
      } break;
      default: {
        ERROR("The audio type %u is not supported currently.");
        ret = false;
      } break;
    }
    if (!ret) {
      break;
    }
    box.m_audio_entry.m_base_box.m_enable = true;
    box.m_audio_entry.m_data_reference_index = 1;
    box.m_audio_entry.m_channelcount = m_audio_info.channels;
    box.m_audio_entry.m_sample_size = m_audio_info.sample_size * 8; //in bits
    box.m_audio_entry.m_sample_rate_integer = m_audio_info.sample_rate;
    box.m_audio_entry.m_sample_rate_decimal = 0;
    box.m_audio_entry.m_pre_defined = 0xfffe;
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC: {
        fill_aac_description_box();
        box.m_audio_entry.m_base_box.m_enable = true;
        box.m_audio_entry.m_base_box.m_size = DISOM_AUDIO_SAMPLE_ENTRY_SIZE
            + box.m_audio_entry.m_aac.m_full_box.m_base_box.m_size;
      } break;
      case AM_AUDIO_OPUS: {
        if (!fill_opus_description_box()) {
          ERROR("Fill opus description box error.");
          ret = false;
          break;
        }
        box.m_audio_entry.m_base_box.m_enable = true;
        box.m_audio_entry.m_base_box.m_size = DISOM_AUDIO_SAMPLE_ENTRY_SIZE
            + box.m_audio_entry.m_opus.m_base_box.m_size;
      } break;
      default:
        ERROR("Audio type %u is not supported currently.", m_audio_info.type);
        ret = false;
        break;
    }
    if (!ret) {
      break;
    }
    box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE
        + sizeof(uint32_t) + box.m_audio_entry.m_base_box.m_size;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_sample_description_box()
{
  bool ret = true;
  do {
    SampleDescriptionBox& box = m_movie_box.m_gps_track.m_media.\
        m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = 0;
    box.m_entry_count = 1;
    box.m_metadata_entry.m_base_box.m_enable = true;
    box.m_metadata_entry.m_base_box.m_type = DISOM_META_ENTRY_TAG;
    box.m_metadata_entry.m_data_reference_index = 1;
    box.m_metadata_entry.m_mime_format = "text/GPS";
  } while(0);
  return ret;
}

bool AMMp4Fixer::fill_video_decoding_time_to_sample_box()
{
  bool ret = true;
  DecodingTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
        DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_video_composition_time_to_sample_box()
{
  bool ret = true;
  CompositionTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_ctts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
        DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_decoding_time_to_sample_box()
{
  bool ret = true;
    DecodingTimeToSampleBox& box =
        m_movie_box.m_audio_track.m_media.m_media_info.m_sample_table.m_stts;
    do {
      box.m_full_box.m_base_box.m_enable = true;
      box.m_full_box.m_base_box.m_type =
          DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
      box.m_full_box.m_flags = m_flags;
      box.m_full_box.m_version = m_used_version;
    } while (0);
    return ret;
}

bool AMMp4Fixer::fill_gps_decoding_time_to_sample_box()
{
  bool ret = true;
  DecodingTimeToSampleBox& box =
      m_movie_box.m_gps_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
        DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_video_sample_table_box()
{
  bool ret = true;
  SampleTableBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    if (!fill_visual_sample_description_box()) {
      ERROR("Failed to fill visual sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_video_decoding_time_to_sample_box())) {
      ERROR("Failed to fill video decoding time to sample box.");
      ret = false;
      break;
    }
    if (m_have_B_frame) {
      if (AM_UNLIKELY(!fill_video_composition_time_to_sample_box())) {
        ERROR("Failed to fill video composition time to sample box.");
        ret = false;
        break;
      }
    }
    fill_video_sample_size_box();
    if (AM_UNLIKELY(!fill_video_sample_to_chunk_box())) {
      ERROR("Failed to fill sample to chunk box.");
      ret = false;
      break;
    }
    if (FIX_CHUNK_OFFSET64_ENABLE) {
      fill_video_chunk_offset64_box();
    } else {
      fill_video_chunk_offset_box();
    }
    fill_video_sync_sample_box();
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_sound_sample_table_box()
{
  bool ret = true;
  SampleTableBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    if (!fill_audio_sample_description_box()) {
      ERROR("Fill audio sample description box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_audio_decoding_time_to_sample_box())) {
      ERROR("Failed to fill audio decoding time to sample box.");
      ret = false;
      break;
    }
    fill_audio_sample_size_box();
    if (AM_UNLIKELY(!fill_audio_sample_to_chunk_box())) {
      ERROR("Failed to fill sample to chunk box.");
      ret = false;
      break;
    }
    if (FIX_CHUNK_OFFSET64_ENABLE) {
      fill_audio_chunk_offset64_box();
    } else {
      fill_audio_chunk_offset_box();
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_sample_table_box()
{
  bool ret = true;
  SampleTableBox& box =
      m_movie_box.m_gps_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    if (!fill_gps_sample_description_box()) {
      ERROR("Fill gps sample description box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_gps_decoding_time_to_sample_box())) {
      ERROR("Failed to fill gps decoding time to sample box.");
      ret = false;
      break;
    }
    fill_gps_sample_size_box();
    if (AM_UNLIKELY(!fill_gps_sample_to_chunk_box())) {
      ERROR("Failed to fill gps sample to chunk box.");
      ret = false;
      break;
    }
    if (FIX_CHUNK_OFFSET64_ENABLE) {
      fill_gps_chunk_offset64_box();
    } else {
      fill_gps_chunk_offset_box();
    }
  } while (0);
  return ret;
}

void AMMp4Fixer::fill_video_data_reference_box()
{
  DataReferenceBox& box = m_movie_box.m_video_track.m_media.\
     m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE
      + box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMp4Fixer::fill_audio_data_reference_box()
{
  DataReferenceBox& box = m_movie_box.m_audio_track.m_media.\
    m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE
      + box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMp4Fixer::fill_gps_data_reference_box()
{
  DataReferenceBox& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMp4Fixer::fill_video_data_info_box()
{
  DataInformationBox& box = m_movie_box.m_video_track.m_media.\
    m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_video_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size
      + DISOM_BASE_BOX_SIZE;
}

void AMMp4Fixer::fill_audio_data_info_box()
{
  DataInformationBox& box = m_movie_box.m_audio_track.m_media.\
    m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_audio_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size
      + DISOM_BASE_BOX_SIZE;
}

void AMMp4Fixer::fill_gps_data_info_box()
{
  DataInformationBox& box = m_movie_box.m_gps_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_gps_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size +
      DISOM_BASE_BOX_SIZE;
}

void AMMp4Fixer::fill_video_media_info_header_box()
{
  VideoMediaInfoHeaderBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_video_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_VIDEO_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_graphicsmode = 0;
  box.m_opcolor[0] = box.m_opcolor[1] = box.m_opcolor[2] = 0;
  box.m_full_box.m_base_box.m_size = DISOM_VIDEO_MEDIA_INFO_HEADER_SIZE;
}

void AMMp4Fixer::fill_sound_media_info_header_box()
{
  SoundMediaInfoHeaderBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sound_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SOUND_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_balanse = 0;
  box.m_full_box.m_base_box.m_size = DISOM_SOUND_MEDIA_HEADER_SIZE;
}

void AMMp4Fixer::fill_gps_media_info_header_box()
{
  NullMediaInfoHeaderBox& box =
      m_movie_box.m_gps_track.m_media.m_media_info.m_null_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_NULL_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
}

bool AMMp4Fixer::fill_video_media_info_box()
{
  bool ret = true;
  MediaInformationBox& box = m_movie_box.m_video_track.m_media.m_media_info;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    fill_video_media_info_header_box();
    fill_video_data_info_box();
    fill_video_sample_table_box();
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_media_info_box()
{
  bool ret = true;
  MediaInformationBox& box = m_movie_box.m_audio_track.m_media.m_media_info;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    box.m_base_box.m_enable = true;
    fill_sound_media_info_header_box();
    fill_audio_data_info_box();
    if (AM_UNLIKELY(!fill_sound_sample_table_box())) {
      ERROR("Failed to fill sound sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_media_info_box()
{
  bool ret = true;
  MediaInformationBox& box = m_movie_box.m_gps_track.m_media.m_media_info;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    box.m_base_box.m_enable = true;
    fill_gps_media_info_header_box();
    fill_gps_data_info_box();
    if (AM_UNLIKELY(!fill_gps_sample_table_box())) {
      ERROR("Failed to fill gps sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_video_media_box()
{
  bool ret = true;
  MediaBox& box = m_movie_box.m_video_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_video_track();
    if (!fill_video_handler_reference_box()) {
      ERROR("Fill video hander reference box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_video_media_info_box())) {
      ERROR("Failed to fill video media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_media_box()
{
  bool ret = true;
  MediaBox& box = m_movie_box.m_audio_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_audio_track();
    if (!fill_audio_handler_reference_box()) {
      ERROR("Fill audio hander reference box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_audio_media_info_box())) {
      ERROR("Failed to fill audio media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_media_box()
{
  bool ret = true;
  MediaBox& box = m_movie_box.m_gps_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_gps_track();
    if (!fill_gps_handler_reference_box()) {
      ERROR("Fill gps hander reference box error.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_gps_media_info_box())) {
      ERROR("Failed to fill gps media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_video_track_box()
{
  bool ret = true;
  TrackBox& box = m_movie_box.m_video_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_video_track_header_box();
    if (AM_UNLIKELY(!fill_video_media_box())) {
      ERROR("Failed to fill movie video track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_audio_track_box()
{
  bool ret = true;
  TrackBox& box = m_movie_box.m_audio_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_audio_track_header_box();
    if (AM_UNLIKELY(fill_audio_media_box() != true)) {
      ERROR("Failed to fill movie audio track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_gps_track_box()
{
  bool ret = true;
  TrackBox& box = m_movie_box.m_gps_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_gps_track_header_box();
    if(AM_UNLIKELY(!fill_gps_media_box())) {
      ERROR("Failed to fill movie gps track box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::fill_movie_box()
{
  bool ret = true;
  MovieBox& box = m_movie_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MOVIE_BOX_TAG;
    fill_movie_header_box();
    if (FIX_OBJECT_DESC_BOX_ENABLE) {
      fill_object_desc_box();
    }
    if (m_video_frame_number > 0) {
      if (AM_UNLIKELY(!fill_video_track_box())) {
        ERROR("Failed to fill video track box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video frame number is zero, there are no video in this file");
    }
    if (m_audio_frame_number > 0) {
      if (AM_UNLIKELY(!fill_audio_track_box())) {
        ERROR("Failed to fill audio track box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio frame number is zero, there are no audio in this file.");
    }
    if(m_gps_frame_number > 0) {
      if (AM_UNLIKELY(!fill_gps_track_box())) {
        ERROR("Failed to fill gps track box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AMMp4Fixer::insert_video_composition_time_to_sample_box(int64_t delta_pts)
{
  bool ret = true;
  CompositionTimeToSampleBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_ctts;
  do {
    int32_t ctts = 0;
    if (AM_UNLIKELY(m_video_frame_number == 1)) {
      ctts = 0;
      box.m_entry_count = 0;
    } else {
      ctts = m_last_ctts
          + (delta_pts - (90000 * m_video_info.rate / m_video_info.scale));
      if (AM_UNLIKELY(ctts < 0)) {
        for (int32_t i = m_video_frame_number - 2; i >= 0; -- i) {
          box.m_entry_ptr[i].sample_delta =
              box.m_entry_ptr[i].sample_delta - ctts;
        }
        ctts = 0;
      }
    }
    if (m_video_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count + VIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += VIDEO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = ctts;
    ++ box.m_entry_count;
    m_last_ctts = ctts;
  } while (0);
  return ret;
}

bool AMMp4Fixer::insert_video_decoding_time_to_sample_box(uint32_t delta_pts)
{
  bool ret = true;
  DecodingTimeToSampleBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_stts;
  do {
    if (m_video_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (m_video_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count + VIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += VIDEO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = delta_pts;
    ++ box.m_entry_count;
  } while (0);
  return ret;
}

bool AMMp4Fixer::insert_audio_decoding_time_to_sample_box(uint32_t delta_pts)
{
  bool ret = true;
  DecodingTimeToSampleBox& box = m_movie_box.m_audio_track.\
      m_media.m_media_info.m_sample_table.m_stts;
  do {
    if (m_audio_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (m_audio_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count + AUDIO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += AUDIO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = delta_pts;
    ++ box.m_entry_count;
  } while (0);
  return ret;
}

bool AMMp4Fixer::insert_gps_decoding_time_to_sample_box(
    uint32_t delta_pts)
{
  bool ret = true;
  DecodingTimeToSampleBox& box = m_movie_box.m_gps_track.\
      m_media.m_media_info.m_sample_table.m_stts;
  do {
    if(m_gps_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (m_gps_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count + GPS_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += GPS_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = delta_pts;
    ++ box.m_entry_count;
  } while (0);
  return ret;
}

bool AMMp4Fixer::insert_video_chunk_offset64_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffset64Box& box = m_movie_box.m_video_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset64;
  DEBUG("Insert video chunk offset box, offset = %llu", offset);
  if(m_video_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint64_t* ptmp = new uint64_t[box.m_buf_count + VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint64_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += VIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_video_chunk_offset_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffsetBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset;
  DEBUG("Insert video chunk offset box, offset = %llu", offset);
  if (m_video_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
    box.m_entry_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count + VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += VIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_audio_chunk_offset64_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffset64Box& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset64;
  DEBUG("Insert audio chunk offset box, offset = %llu, frame count = %u",
        offset, box.m_entry_count);
  if(m_audio_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint64_t *ptmp = new uint64_t[box.m_buf_count + AUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint64_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += AUDIO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_audio_chunk_offset_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
     m_sample_table.m_chunk_offset;
  DEBUG("Insert audio chunk offset box, offset = %llu, frame count = %u",
        offset, box.m_entry_count);
  if (m_audio_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
    box.m_entry_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count + AUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += AUDIO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_gps_chunk_offset_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffsetBox& box = m_movie_box.m_gps_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset;
  DEBUG("Insert gps chunk offset box, offset = %llu, frame count = %u",
        offset, box.m_entry_count);
  if(m_gps_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
    box.m_entry_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count + GPS_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += GPS_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_gps_chunk_offset64_box(uint64_t offset)
{
  bool ret = true;
  ChunkOffset64Box& box = m_movie_box.m_gps_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset64;
  DEBUG("Insert gps chunk offset box, offset = %llu, frame count = %u",
        offset, box.m_entry_count);
  if(m_gps_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint64_t *ptmp = new uint64_t[box.m_buf_count + GPS_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint64_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += GPS_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_video_sample_size_box(uint32_t size)
{
  bool ret = true;
  SampleSizeBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
    m_sample_table.m_sample_size;
  DEBUG("Insert video sample size box, size = %u", size);
  if (m_video_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count + VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += VIDEO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      ++ box.m_sample_count;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_audio_sample_size_box(uint32_t size)
{
  bool ret = true;
  SampleSizeBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
     m_sample_table.m_sample_size;
  DEBUG("Insert audio sample size box, size = %u", size);
  if (m_audio_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count + AUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += AUDIO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      box.m_sample_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_gps_sample_size_box(uint32_t size)
{
  bool ret = true;
  SampleSizeBox& box = m_movie_box.m_gps_track.m_media.m_media_info.\
      m_sample_table.m_sample_size;
  DEBUG("Insert gps sample size box, size = %u", size);
  if(m_gps_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count +
                                  GPS_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += GPS_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      ++ box.m_sample_count;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMp4Fixer::insert_video_sync_sample_box(uint32_t video_frame_number)
{
  bool ret = true;
  SyncSampleTableBox& box = m_movie_box.m_video_track.m_media.\
     m_media_info.m_sample_table.m_sync_sample;
  do {
    if (m_video_frame_number == 1) {
      box.m_stss_count = 0;
    }
    if (box.m_stss_count >= box.m_buf_count) {
      uint32_t *tmp = new uint32_t[box.m_buf_count + VIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("malloc memory error.");
        ret = false;
        break;
      }
      if (box.m_stss_count > 0) {
        memcpy(tmp, box.m_sync_sample_table,
               box.m_stss_count * sizeof(uint32_t));
      }
      delete[] box.m_sync_sample_table;
      box.m_sync_sample_table = tmp;
      box.m_buf_count += VIDEO_FRAME_COUNT;
    }
    box.m_sync_sample_table[box.m_stss_count] = video_frame_number;
    ++ box.m_stss_count;
  } while (0);
  return ret;
}

