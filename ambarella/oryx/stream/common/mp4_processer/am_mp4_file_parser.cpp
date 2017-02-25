/*******************************************************************************
 * am_mp4_file_parser.cpp
 *
 * History:
 *   2015-09-06 - [ccjing] created file
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
#include "am_mp4_file_parser.h"

#include "mp4_file_reader.h"

AMIMp4FileParser* AMIMp4FileParser::create(const char* file_path)
{
  return (AMIMp4FileParser*)(AMMp4FileParser::create(file_path));
}

AMMp4FileParser* AMMp4FileParser::create(const char* file_path)
{
  AMMp4FileParser* result = new AMMp4FileParser();
  if (AM_UNLIKELY(result && (!result->init(file_path)))) {
    delete result;
    result = nullptr;
  }
  return result;
}

AMMp4FileParser::AMMp4FileParser() :
    m_file_type_box(nullptr),
    m_free_box(nullptr),
    m_media_data_box(nullptr),
    m_movie_box(nullptr),
    m_file_reader(nullptr),
    m_video_chunk_list(nullptr),
    m_audio_chunk_list(nullptr),
    m_gps_chunk_list(nullptr),
    m_file_offset(0)
{
}

bool AMMp4FileParser::init(const char* file_path)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((m_file_reader = Mp4FileReader::create(file_path))
                    == nullptr)) {
      ERROR("Failed to create mp4 file reader.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_file_type_box = new FileTypeBox()) == nullptr)) {
      ERROR("Failed to create file type box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_free_box = new FreeBox()) == nullptr)) {
      ERROR("Failed to create free box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_media_data_box = new MediaDataBox()) == nullptr)) {
      ERROR("Failed to create media data box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_movie_box = new MovieBox()) == nullptr)) {
      ERROR("Failed to create movie box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_video_chunk_list = new ChunkList()) == nullptr)) {
      ERROR("Failed to create video chunk list.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_audio_chunk_list = new ChunkList()) == nullptr)) {
      ERROR("Failed to create audio chunk list.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_gps_chunk_list = new ChunkList()) == nullptr)) {
      ERROR("Failed to create gps chunk list.");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

AMMp4FileParser::~AMMp4FileParser()
{
  m_file_reader->destroy();
  delete m_file_type_box;
  delete m_free_box;
  delete m_media_data_box;
  delete m_movie_box;
  while (!m_video_chunk_list->empty()) {
    delete m_video_chunk_list->back();
    m_video_chunk_list->pop_back();
  }
  while (!m_audio_chunk_list->empty()) {
    delete m_audio_chunk_list->back();
    m_audio_chunk_list->pop_back();
  }
  while (!m_gps_chunk_list->empty()) {
    delete m_gps_chunk_list->back();
    m_gps_chunk_list->pop_back();
  }
  delete m_video_chunk_list;
  delete m_audio_chunk_list;
  delete m_gps_chunk_list;
}

void AMMp4FileParser::destroy()
{
  delete this;
}

DISOM_BOX_TAG AMMp4FileParser::get_box_type()
{
  DISOM_BOX_TAG box_type = DISOM_BOX_TAG_NULL;
  uint8_t buffer[4] = { 0 };
  do {
    if (AM_UNLIKELY(!m_file_reader->advance_file(4))) {
      ERROR("Failed to advance file.");
      break;
    }
    if (AM_UNLIKELY(m_file_reader->read_data(buffer, 4) != 4)) {
      ERROR("Failed to read file");
      break;
    }
    box_type = DISOM_BOX_TAG(DISO_BOX_TAG_CREATE(buffer[0], buffer[1],
                                                 buffer[2], buffer[3]));
    if (AM_UNLIKELY(!m_file_reader->seek_file(m_file_offset))) {
      ERROR("Failed to seek file.");
      box_type = DISOM_BOX_TAG_NULL;
      break;
    }
  } while (0);
  return box_type;
}

bool AMMp4FileParser::parse_file()
{
  bool ret = true;
  INFO("begin to parse mp4 file");
  do {
    switch (get_box_type()) {
      case DISOM_FILE_TYPE_BOX_TAG : {
        INFO("begin to parse file type box");
        ret = parse_file_type_box();
        m_file_offset += m_file_type_box->m_base_box.m_size;
        if (ret) {
          INFO("parse file type box success.");
        }
      }break;
      case DISOM_MEDIA_DATA_BOX_TAG : {
        INFO("begin to parse media data box.");
        ret = parse_media_data_box();
        m_file_offset += m_media_data_box->m_base_box.m_size;
        if (ret) {
          INFO("parse media data box success.");
        }
      } break;
      case DISOM_MOVIE_BOX_TAG : {
        INFO("begin to parse movie box");
        ret = parse_movie_box();
        m_file_offset += m_movie_box->m_base_box.m_size;
        if (ret) {
          INFO("parse movie box success.");
        }
      } break;
      case DISOM_FREE_BOX_TAG : {
        INFO("begin to parse free box.");
        ret = parse_free_box();
        m_file_offset += m_free_box->m_base_box.m_size;
        if (ret) {
          INFO("parse free box success.");
        }
      } break;
      default : {
        NOTICE("Get unknown box type, skip it.");
        uint32_t box_size = 0;
        if (!m_file_reader->read_le_u32(box_size)) {
          ERROR("Failed to read le_u32");
          ret = false;
          break;
        }
        if (box_size - sizeof(uint32_t) > 0) {
          if (AM_UNLIKELY(!m_file_reader->advance_file(box_size
              - sizeof(uint32_t)))) {
            ERROR("Failed to advance file.");
            break;
          }
        }
      } break;
    }
    if (!ret) {
      ERROR("Parse mp4 file error.");
      break;
    }
  } while (((uint64_t) m_file_offset) < m_file_reader->get_file_size());
  return ret;
}

bool AMMp4FileParser::parse_file_type_box()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_file_type_box->read_file_type_box(m_file_reader))) {
      ERROR("Parse file type box error.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::parse_media_data_box()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_media_data_box->read_media_data_box(m_file_reader))) {
      ERROR("parse media data box error.");
      ret = false;
      break;
    }
    /*skip audio and video data*/
    if (m_media_data_box->m_base_box.m_size > DISOM_BASE_BOX_SIZE) {
      if (AM_UNLIKELY(!m_file_reader->advance_file(
          m_media_data_box->m_base_box.m_size - DISOM_BASE_BOX_SIZE))) {
        ERROR("Failed to advance file.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::parse_movie_box()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_movie_box->read_movie_box(m_file_reader))) {
      ERROR("Failed to read movie box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!create_chunk_sample_table())) {
      ERROR("Failed to create chunk sample table.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::parse_free_box()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!m_free_box->read_free_box(m_file_reader))) {
      ERROR("Failed to read free box.");
      ret = false;
      break;
    }
    //skip empty bytes
    if (m_free_box->m_base_box.m_size > DISOM_BASE_BOX_SIZE) {
      if (AM_UNLIKELY(!m_file_reader->advance_file(
          m_free_box->m_base_box.m_size - DISOM_BASE_BOX_SIZE))) {
        ERROR("Failed to advance file in parse free box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::create_chunk_sample_table()
{
  bool ret = true;
  ChunkOffsetBox& video_chunk_offset = m_movie_box->m_video_track.\
           m_media.m_media_info.m_sample_table.m_chunk_offset;
  ChunkOffsetBox& audio_chunk_offset = m_movie_box->m_audio_track.\
           m_media.m_media_info.m_sample_table.m_chunk_offset;
  ChunkOffsetBox& gps_chunk_offset = m_movie_box->m_gps_track.\
           m_media.m_media_info.m_sample_table.m_chunk_offset;
  ChunkOffset64Box& video_chunk_offset64 = m_movie_box->m_video_track.\
      m_media.m_media_info.m_sample_table.m_chunk_offset64;
  ChunkOffset64Box& audio_chunk_offset64 = m_movie_box->m_audio_track.\
      m_media.m_media_info.m_sample_table.m_chunk_offset64;
  ChunkOffset64Box& gps_chunk_offset64 = m_movie_box->m_gps_track.\
      m_media.m_media_info.m_sample_table.m_chunk_offset64;
  SampleToChunkBox& video_sample_to_chunk = m_movie_box->m_video_track.\
      m_media.m_media_info.m_sample_table.m_sample_to_chunk;
  SampleToChunkBox& audio_sample_to_chunk = m_movie_box->m_audio_track.\
      m_media.m_media_info.m_sample_table.m_sample_to_chunk;
  SampleToChunkBox& gps_sample_to_chunk = m_movie_box->m_gps_track.\
      m_media.m_media_info.m_sample_table.m_sample_to_chunk;
  do {
    //parse video chunk offset
    if (video_chunk_offset.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < video_chunk_offset.m_entry_count; ++ i) {
        SampleChunk* video_chunk = nullptr;
        if ((video_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        video_chunk->chunk_num = i + 1;
        video_chunk->sample_count = 0;
        m_video_chunk_list->push_back(video_chunk);
      }
      if (!ret) {
        break;
      }
    } else if (video_chunk_offset64.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < video_chunk_offset64.m_entry_count; ++ i) {
        SampleChunk* video_chunk = nullptr;
        if ((video_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        video_chunk->chunk_num = i + 1;
        video_chunk->sample_count = 0;
        m_video_chunk_list->push_back(video_chunk);
      }
      if (!ret) {
        break;
      }
    } else {
      ERROR("video chunk offset is not enabled.");
      ret = false;
      break;
    }
    //parse video chunk to sample box
    if (video_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      uint32_t video_chunk_list_count = m_video_chunk_list->size();
      uint32_t video_sample_to_chunk_count = video_sample_to_chunk.m_entry_count;
      for (; video_sample_to_chunk_count >= 1; -- video_sample_to_chunk_count) {
        while((video_chunk_list_count >= video_sample_to_chunk.\
            m_entrys[video_sample_to_chunk_count -1].m_first_chunk)) {
          (*m_video_chunk_list)[video_chunk_list_count -1]->sample_count =
              video_sample_to_chunk.m_entrys[video_sample_to_chunk_count -1].\
              m_sample_per_chunk;
          -- video_chunk_list_count;
        }
      }
      if ((video_sample_to_chunk_count == 0) && (video_chunk_list_count == 0)) {
        INFO("Create video chunk list success.");
      } else {
        ERROR("Failded to create video chunk list.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video sample to chunk box is not enabled.");
      ret = false;
      break;
    }
    //parse audio chunk offset
    if (audio_chunk_offset.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < audio_chunk_offset.m_entry_count; ++ i) {
        SampleChunk* audio_chunk = nullptr;
        if ((audio_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        audio_chunk->chunk_num = i + 1;
        audio_chunk->sample_count = 0;
        m_audio_chunk_list->push_back(audio_chunk);
      }
      if (!ret) {
        break;
      }
    } else if (audio_chunk_offset64.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < audio_chunk_offset64.m_entry_count; ++ i) {
        SampleChunk* audio_chunk = nullptr;
        if ((audio_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        audio_chunk->chunk_num = i + 1;
        audio_chunk->sample_count = 0;
        m_audio_chunk_list->push_back(audio_chunk);
      }
      if (!ret) {
        break;
      }
    } else {
      ERROR("audio chunk offset is not enabled.");
      ret = false;
      break;
    }
    //parse audio chunk to sample box
    if (audio_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      uint32_t audio_chunk_list_count = m_audio_chunk_list->size();
      uint32_t audio_sample_to_chunk_count = audio_sample_to_chunk.m_entry_count;
      for (; audio_sample_to_chunk_count >= 1; -- audio_sample_to_chunk_count) {
        while((audio_chunk_list_count >= audio_sample_to_chunk.\
            m_entrys[audio_sample_to_chunk_count -1].m_first_chunk)) {
          (*m_audio_chunk_list)[audio_chunk_list_count -1]->sample_count =
              audio_sample_to_chunk.m_entrys[audio_sample_to_chunk_count -1].\
              m_sample_per_chunk;
          -- audio_chunk_list_count;
        }
      }
      if ((audio_sample_to_chunk_count == 0) && (audio_chunk_list_count == 0)) {
        INFO("Create audio chunk list success.");
      } else {
        ERROR("Failded to create audio chunk list.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio sample to chunk box is not enalbed.");
    }
    //parse gps chunk offset
    if (gps_chunk_offset.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < gps_chunk_offset.m_entry_count; ++ i) {
        SampleChunk* gps_chunk = nullptr;
        if ((gps_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        gps_chunk->chunk_num = i + 1;
        gps_chunk->sample_count = 0;
        m_gps_chunk_list->push_back(gps_chunk);
      }
      if (!ret) {
        break;
      }
    } else if (gps_chunk_offset64.m_full_box.m_base_box.m_enable) {
      for (uint32_t i = 0; i < gps_chunk_offset64.m_entry_count; ++ i) {
        SampleChunk* gps_chunk = nullptr;
        if ((gps_chunk = new SampleChunk()) == nullptr) {
          ERROR("Failed to create sample chunk.");
          ret = false;
          break;
        }
        gps_chunk->chunk_num = i + 1;
        gps_chunk->sample_count = 0;
        m_gps_chunk_list->push_back(gps_chunk);
      }
      if (!ret) {
        break;
      }
    } else {
      INFO("gps chunk offset is not enabled.");
    }
    //parse gps chunk to sample box
    if (gps_sample_to_chunk.m_full_box.m_base_box.m_enable) {
      uint32_t gps_chunk_list_count = m_gps_chunk_list->size();
      uint32_t gps_sample_to_chunk_count = gps_sample_to_chunk.m_entry_count;
      for (; gps_sample_to_chunk_count >= 1; -- gps_sample_to_chunk_count) {
        while((gps_chunk_list_count >= gps_sample_to_chunk.\
            m_entrys[gps_sample_to_chunk_count -1].m_first_chunk)) {
          (*m_gps_chunk_list)[gps_chunk_list_count -1]->sample_count =
              gps_sample_to_chunk.m_entrys[gps_sample_to_chunk_count -1].\
              m_sample_per_chunk;
          -- gps_chunk_list_count;
        }
      }
      if ((gps_sample_to_chunk_count == 0) && (gps_chunk_list_count == 0)) {
        INFO("Create gps chunk list success.");
      } else {
        ERROR("Failded to create gps chunk list.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The gps sample to chunk box is not enabled.");
    }
  } while(0);
  return ret;
}

bool AMMp4FileParser::get_video_frame(FrameInfo& frame_info)
{
  bool ret = true;
  do {
    if (frame_info.frame_number <= 0) {
      ERROR("frame number is zero.");
      ret = false;
      break;
    }
    if (!m_movie_box->m_video_track.m_base_box.m_enable) {
      ERROR("There is no video frame in this file");
      ret = false;
      break;
    }
    SampleSizeBox& sample_size_box = m_movie_box->m_video_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    ChunkOffsetBox& chunk_offset_box = m_movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& chunk_offset64_box = m_movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (frame_info.frame_number <= sample_size_box.m_sample_count) {
      frame_info.frame_size =
          sample_size_box.m_size_array[frame_info.frame_number - 1];
    } else {
      ERROR("The frame number of frame_info is bigger than frame count of "
          "sample size box.");
      ret = false;
      break;
    }
    if ((frame_info.frame_data == NULL)
        || (frame_info.buffer_size < frame_info.frame_size)) {
      delete[] frame_info.frame_data;
      frame_info.frame_data = new uint8_t[frame_info.frame_size + 1];
      if (AM_UNLIKELY(!frame_info.frame_data)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      frame_info.buffer_size = frame_info.frame_size + 1;
    }
    //To find the offset of video frame
    uint32_t frame_count = 0;
    uint32_t chunk_num = 1;
    for (uint32_t i = 0; i < m_video_chunk_list->size(); ++ i) {
      if ((frame_info.frame_number > frame_count) &&
          (frame_info.frame_number <=
           (frame_count + (*m_video_chunk_list)[i]->sample_count))) {
        chunk_num = i + 1;
        break;
      } else {
        frame_count += (*m_video_chunk_list)[i]->sample_count;
      }
    }
    uint32_t offset = 0;
    if (chunk_offset_box.m_full_box.m_base_box.m_enable) {
      offset = chunk_offset_box.m_chunk_offset[chunk_num -1];
    } else if (chunk_offset64_box.m_full_box.m_base_box.m_enable) {
      offset = (uint32_t) chunk_offset64_box.m_chunk_offset[chunk_num -1];
    } else {
      ERROR("The video chunk offset is not enabled.");
      ret = false;
      break;
    }
    for (uint32_t i = frame_count + 1; i < frame_info.frame_number; ++ i) {
      offset += sample_size_box.m_size_array[i - 1];
    }
    if (!m_file_reader->seek_file(offset)) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
    if (m_file_reader->read_data(frame_info.frame_data, frame_info.frame_size)
        != (int) frame_info.frame_size) {
      ERROR("Failed to read video data from mp4 file.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::get_audio_frame(FrameInfo& frame_info)
{
  bool ret = true;
  do {
    if (frame_info.frame_number <= 0) {
      ERROR("frame number is zero.");
      ret = false;
      break;
    }
    if (!m_movie_box->m_audio_track.m_base_box.m_enable) {
      ERROR("There is no audio frame in this file");
      ret = false;
      break;
    }
    SampleSizeBox& sample_size_box = m_movie_box->m_audio_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    ChunkOffsetBox& chunk_offset_box = m_movie_box->m_audio_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& chunk_offset64_box = m_movie_box->m_audio_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (frame_info.frame_number <= sample_size_box.m_sample_count) {
      frame_info.frame_size =
          sample_size_box.m_size_array[frame_info.frame_number - 1];
    } else {
      ERROR("The frame number of frame_info is bigger than frame count of "
          "sample size box.");
      ret = false;
      break;
    }
    if ((frame_info.frame_data == NULL)
        || (frame_info.buffer_size < frame_info.frame_size)) {
      delete[] frame_info.frame_data;
      frame_info.frame_data = new uint8_t[frame_info.frame_size + 1];
      if (AM_UNLIKELY(!frame_info.frame_data)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      frame_info.buffer_size = frame_info.frame_size + 1;
    }
    //To find the offset of audio frame
    uint32_t frame_count = 0;
    uint32_t chunk_num = 1;
    for (uint32_t i = 0; i < m_audio_chunk_list->size(); ++ i) {
      if ((frame_info.frame_number > frame_count) &&
          (frame_info.frame_number <=
              (frame_count + (*m_audio_chunk_list)[i]->sample_count))) {
        chunk_num = i + 1;
        break;
      } else {
        frame_count += (*m_audio_chunk_list)[i]->sample_count;
      }
    }
    uint32_t offset = 0;
    if (chunk_offset_box.m_full_box.m_base_box.m_enable) {
      offset = chunk_offset_box.m_chunk_offset[chunk_num -1];
    } else if (chunk_offset64_box.m_full_box.m_base_box.m_enable) {
      offset = (uint32_t) chunk_offset64_box.m_chunk_offset[chunk_num -1];
    } else {
      ERROR("The audio chunk offset is not enabled.");
      ret = false;
      break;
    }
    for (uint32_t i = frame_count + 1; i < frame_info.frame_number; ++ i) {
      offset += sample_size_box.m_size_array[i - 1];
    }
    if (!m_file_reader->seek_file(offset)) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
    if (m_file_reader->read_data(frame_info.frame_data, frame_info.frame_size)
        != (int) frame_info.frame_size) {
      ERROR("Failed to read audio data from mp4 file.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4FileParser::get_gps_frame(FrameInfo& frame_info)
{
  bool ret = true;
  do {
    if (frame_info.frame_number <= 0) {
      ERROR("frame number is zero.");
      ret = false;
      break;
    }
    if (!m_movie_box->m_gps_track.m_base_box.m_enable) {
      ERROR("There is no audio frame in this file");
      ret = false;
      break;
    }
    SampleSizeBox& sample_size_box = m_movie_box->m_gps_track.m_media.\
        m_media_info.m_sample_table.m_sample_size;
    ChunkOffsetBox& chunk_offset_box = m_movie_box->m_gps_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& chunk_offset64_box = m_movie_box->m_gps_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (frame_info.frame_number <= sample_size_box.m_sample_count) {
      frame_info.frame_size =
          sample_size_box.m_size_array[frame_info.frame_number - 1];
    } else {
      ERROR("The frame number of frame_info is bigger than frame count of "
          "sample size box.");
      ret = false;
      break;
    }
    if ((frame_info.frame_data == NULL)
        || (frame_info.buffer_size < frame_info.frame_size)) {
      delete[] frame_info.frame_data;
      frame_info.frame_data = new uint8_t[frame_info.frame_size + 1];
      if (AM_UNLIKELY(!frame_info.frame_data)) {
        ERROR("Failed to malloc memory");
        ret = false;
        break;
      }
      frame_info.buffer_size = frame_info.frame_size + 1;
    }
    //To find the offset of gps frame
    uint32_t frame_count = 0;
    uint32_t chunk_num = 1;
    for (uint32_t i = 0; i < m_gps_chunk_list->size(); ++ i) {
      if ((frame_info.frame_number > frame_count) &&
          (frame_info.frame_number <=
              (frame_count + (*m_gps_chunk_list)[i]->sample_count))) {
        chunk_num = i + 1;
        break;
      } else {
        frame_count += (*m_gps_chunk_list)[i]->sample_count;
      }
    }
    uint32_t offset = 0;
    if (chunk_offset_box.m_full_box.m_base_box.m_enable) {
      offset = chunk_offset_box.m_chunk_offset[chunk_num -1];
    } else if (chunk_offset64_box.m_full_box.m_base_box.m_enable) {
      offset = (uint32_t) chunk_offset64_box.m_chunk_offset[chunk_num -1];
    } else {
      ERROR("The gps chunk offset is not enabled.");
      ret = false;
      break;
    }
    for (uint32_t i = frame_count + 1; i < frame_info.frame_number; ++ i) {
      offset += sample_size_box.m_size_array[i - 1];
    }
    if (!m_file_reader->seek_file(offset)) {
      ERROR("Failed to seek file");
      ret = false;
      break;
    }
    if (m_file_reader->read_data(frame_info.frame_data, frame_info.frame_size)
        != (int) frame_info.frame_size) {
      ERROR("Failed to read gps data from mp4 file.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

uint32_t AMMp4FileParser::get_video_frame_number()
{
  if (m_movie_box->m_video_track.m_base_box.m_enable) {
    return m_movie_box->m_video_track.m_media.m_media_info.\
           m_sample_table.m_sample_size.m_sample_count;
  } else {
    return 0;
  }
}

uint32_t AMMp4FileParser::get_audio_frame_number()
{
  if (m_movie_box->m_audio_track.m_base_box.m_enable) {
    return m_movie_box->m_audio_track.m_media.m_media_info.\
           m_sample_table.m_sample_size.m_sample_count;
  } else {
    return 0;
  }
}

uint32_t AMMp4FileParser::get_gps_frame_number()
{
  if (m_movie_box->m_gps_track.m_base_box.m_enable) {
    return m_movie_box->m_gps_track.m_media.m_media_info.\
        m_sample_table.m_sample_size.m_sample_count;
  } else {
    return 0;
  }
}

FileTypeBox* AMMp4FileParser::get_file_type_box()
{
  return m_file_type_box;
}

MediaDataBox* AMMp4FileParser::get_media_data_box()
{
  return m_media_data_box;
}

MovieBox* AMMp4FileParser::get_movie_box()
{
  return m_movie_box;
}
