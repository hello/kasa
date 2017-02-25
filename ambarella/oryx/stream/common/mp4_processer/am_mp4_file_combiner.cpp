/*******************************************************************************
 * am_mp4_file_combiner.cpp
 *
 * History:
 *   2015-10-10 - [ccjing] created file
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

#include "mp4_file_writer.h"
#include "am_mp4_file_combiner.h"
#include "am_mp4_file_parser_if.h"

AMIMp4FileCombiner* AMIMp4FileCombiner::create(const char *first_mp4_file_path,
                                               const char *second_mp4_file_path)
{
  return (AMIMp4FileCombiner*) (AMMp4FileCombiner::create(first_mp4_file_path,
                                                          second_mp4_file_path));
}

AMMp4FileCombiner*AMMp4FileCombiner::create(const char *first_mp4_file_path,
                                            const char *second_mp4_file_path)
{
  AMMp4FileCombiner* result = NULL;
  result = new AMMp4FileCombiner();
  if (result && (!result->init(first_mp4_file_path, second_mp4_file_path))) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMMp4FileCombiner::destroy()
{
  delete this;
}

AMMp4FileCombiner::AMMp4FileCombiner() :
    m_first_mp4_parser(nullptr),
    m_second_mp4_parser(nullptr)
{
}

AMMp4FileCombiner::~AMMp4FileCombiner()
{
  m_first_mp4_parser->destroy();
  m_second_mp4_parser->destroy();
}

bool AMMp4FileCombiner::init(const char *first_mp4_file_path,
                             const char* second_mp4_file_path)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((m_first_mp4_parser =
        AMIMp4FileParser::create(first_mp4_file_path)) == NULL)) {
      ERROR("Failed to create first mp4 parser.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY((m_second_mp4_parser =
        AMIMp4FileParser::create(second_mp4_file_path)) == NULL)) {
      ERROR("Failed to create second mp4 parser.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMp4FileCombiner::get_combined_mp4_file(const char* file_path)
{
  bool ret = true;
  MovieBox combined_movie_box;
  Mp4FileWriter* mp4_file_writer = NULL;
  do {
    mp4_file_writer = Mp4FileWriter::create();
    if (AM_UNLIKELY(!mp4_file_writer)) {
      ERROR("Failed to create mp4 file writer.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_first_mp4_parser->parse_file())) {
      ERROR("Failed to parse first mp4 file.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_second_mp4_parser->parse_file())) {
      ERROR("Failed to parse second mp4 file.");
      ret = false;
      break;
    }
    MovieBox *first_movie_box = m_first_mp4_parser->get_movie_box();
    MovieBox *second_movie_box = m_second_mp4_parser->get_movie_box();
    NOTICE("Begin to get combined movie box.");
    if (!first_movie_box->get_combined_movie_box(*second_movie_box,
                                                 combined_movie_box)) {
      ERROR("Failed to get combined movie box.");
      ret = false;
      break;
    }
    ChunkOffsetBox& video_combined_offset_box = combined_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& video_combined_offset64_box = combined_movie_box.m_video_track.\
            m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (video_combined_offset_box.m_full_box.m_base_box.m_enable) {
      video_combined_offset_box.m_entry_count =
                m_first_mp4_parser->get_video_frame_number() +
                m_second_mp4_parser->get_video_frame_number();
      video_combined_offset_box.m_buf_count =
          video_combined_offset_box.m_entry_count;
      delete[] video_combined_offset_box.m_chunk_offset;
      video_combined_offset_box.m_chunk_offset =
          new uint32_t[video_combined_offset_box.m_entry_count];
      if (!video_combined_offset_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else if (video_combined_offset64_box.m_full_box.m_base_box.m_enable) {
      video_combined_offset64_box.m_entry_count =
          m_first_mp4_parser->get_video_frame_number() +
          m_second_mp4_parser->get_video_frame_number();
      video_combined_offset64_box.m_buf_count =
          video_combined_offset64_box.m_entry_count;
      delete[] video_combined_offset64_box.m_chunk_offset;
      video_combined_offset64_box.m_chunk_offset =
          new uint64_t[video_combined_offset64_box.m_entry_count];
      if (!video_combined_offset64_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else {
      ERROR("Video chunk offset box is not enabled.");
      ret = false;
      break;
    }
    ChunkOffsetBox& audio_combined_offset_box = combined_movie_box.m_audio_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& audio_combined_offset64_box = combined_movie_box.m_audio_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (audio_combined_offset_box.m_full_box.m_base_box.m_enable) {
      audio_combined_offset_box.m_entry_count =
          m_first_mp4_parser->get_audio_frame_number() +
          m_second_mp4_parser->get_audio_frame_number();
      audio_combined_offset_box.m_buf_count =
          audio_combined_offset_box.m_entry_count;
      delete[] audio_combined_offset_box.m_chunk_offset;
      audio_combined_offset_box.m_chunk_offset =
          new uint32_t[audio_combined_offset_box.m_entry_count];
      if (!audio_combined_offset_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else if (audio_combined_offset64_box.m_full_box.m_base_box.m_enable) {
      audio_combined_offset64_box.m_entry_count =
          m_first_mp4_parser->get_audio_frame_number() +
          m_second_mp4_parser->get_audio_frame_number();
      audio_combined_offset64_box.m_buf_count =
          audio_combined_offset64_box.m_entry_count;
      delete[] audio_combined_offset64_box.m_chunk_offset;
      audio_combined_offset64_box.m_chunk_offset =
          new uint64_t[audio_combined_offset64_box.m_entry_count];
      if (!audio_combined_offset64_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else {
      NOTICE("Audio chunk offset box is not enabled.");
    }
    ChunkOffsetBox& gps_combined_offset_box = combined_movie_box.m_gps_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& gps_combined_offset64_box = combined_movie_box.m_gps_track.\
        m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (gps_combined_offset_box.m_full_box.m_base_box.m_enable) {
      gps_combined_offset_box.m_entry_count =
          m_first_mp4_parser->get_gps_frame_number() +
          m_second_mp4_parser->get_gps_frame_number();
      gps_combined_offset_box.m_buf_count =
          gps_combined_offset_box.m_entry_count;
      delete[] gps_combined_offset_box.m_chunk_offset;
      gps_combined_offset_box.m_chunk_offset =
          new uint32_t[gps_combined_offset_box.m_entry_count];
      if (!gps_combined_offset_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else if (gps_combined_offset64_box.m_full_box.m_base_box.m_enable) {
      gps_combined_offset64_box.m_entry_count =
          m_first_mp4_parser->get_gps_frame_number() +
          m_second_mp4_parser->get_gps_frame_number();
      gps_combined_offset64_box.m_buf_count =
          gps_combined_offset64_box.m_entry_count;
      delete[] gps_combined_offset64_box.m_chunk_offset;
      gps_combined_offset64_box.m_chunk_offset =
          new uint64_t[gps_combined_offset64_box.m_entry_count];
      if (!gps_combined_offset64_box.m_chunk_offset) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
    } else {
      INFO("Gps chunk offset box is not enabled.");
    }
    combined_movie_box.update_movie_box_size();
    /*combined sync sample table box*/
    NOTICE("Begin to combine sync sample table box");
    uint32_t first_file_video_frame_number =
        m_first_mp4_parser->get_video_frame_number();
    SyncSampleTableBox& first_sync_box = first_movie_box->m_video_track.\
     m_media.m_media_info.m_sample_table.m_sync_sample;
    SyncSampleTableBox& second_sync_box = second_movie_box->m_video_track.\
     m_media.m_media_info.m_sample_table.m_sync_sample;
    SyncSampleTableBox& combined_sync_box = combined_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_sync_sample;
    for (uint32_t i = 0; i < first_sync_box.m_stss_count; ++ i) {
      combined_sync_box.m_sync_sample_table[i] =
          first_sync_box.m_sync_sample_table[i];
    }
    INFO("First file video frame number is %u, first stts count is %u,"
         "second sync sample table[0] is %u",
         first_file_video_frame_number, first_sync_box.m_stss_count,
         second_sync_box.m_sync_sample_table[0]);
    for (uint32_t i = 0; i < second_sync_box.m_stss_count; ++ i) {
      combined_sync_box.m_sync_sample_table[first_sync_box.m_stss_count + i] =
        second_sync_box.m_sync_sample_table[i] + first_file_video_frame_number;
    }
    NOTICE("Combine sync sample table box success.");
    /*begin to write combined mp4 file*/
    FileTypeBox* file_type_box = m_first_mp4_parser->get_file_type_box();
    MediaDataBox* first_media_data_box =
        m_first_mp4_parser->get_media_data_box();
    MediaDataBox* second_media_data_box =
        m_second_mp4_parser->get_media_data_box();
    MediaDataBox combined_media_data_box;
    first_media_data_box->m_base_box.copy_base_box(
        combined_media_data_box.m_base_box);
    combined_media_data_box.m_base_box.m_size =
        first_media_data_box->m_base_box.m_size
      + second_media_data_box->m_base_box.m_size - DISOM_BASE_BOX_SIZE;
    if (!mp4_file_writer->create_file(file_path)) {
      ERROR("Failed to create file %s", file_path);
      ret = false;
      break;
    }
    file_type_box->write_file_type_box(mp4_file_writer);
    mp4_file_writer->seek_file(combined_movie_box.m_base_box.m_size, SEEK_CUR);
    uint64_t file_offset = 0;
    if (AM_UNLIKELY(!mp4_file_writer->tell_file(file_offset))) {
      ERROR("Failed to get offset of mp4 file");
      ret = false;
      break;
    }
    NOTICE("Begin to write media data box.");
    combined_media_data_box.write_media_data_box(mp4_file_writer);
    file_offset += DISOM_BASE_BOX_SIZE;
    uint32_t video_size = 0;
    uint32_t audio_size = 0;
    uint32_t gps_size = 0;
    //begin to write media data.
    FrameInfo frame_info;
    if (combined_movie_box.m_video_track.m_base_box.m_enable) {
      for (uint32_t i = 1; i <= m_first_mp4_parser->get_video_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_first_mp4_parser->get_video_frame(frame_info)) {
          ERROR("Failed to get video frame.");
          ret = false;
          break;
        }
        video_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (video_combined_offset_box.m_full_box.m_base_box.m_enable) {
          video_combined_offset_box.m_chunk_offset[i - 1] =
                      (uint32_t) file_offset;
        } else if (video_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          video_combined_offset64_box.m_chunk_offset[i - 1] = file_offset;
        } else {
          ERROR("The video combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write video data to new mp4 file");
        break;
      }
      for (uint32_t i = 1; i <= m_second_mp4_parser->get_video_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_second_mp4_parser->get_video_frame(frame_info)) {
          ERROR("Failed to get video frame.");
          ret = false;
          break;
        }
        video_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (video_combined_offset_box.m_full_box.m_base_box.m_enable) {
          video_combined_offset_box.m_chunk_offset[i +
                 m_first_mp4_parser->get_video_frame_number() - 1] =
              (uint32_t) file_offset;
        } else if (video_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          video_combined_offset64_box.m_chunk_offset[i +
              m_first_mp4_parser->get_video_frame_number() - 1] = file_offset;
        } else {
          ERROR("The video combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write video data to new mp4 file");
        break;
      }
    } else {
      ERROR("The video track is not enabled.");
      ret = false;
      break;
    }
    if (combined_movie_box.m_audio_track.m_base_box.m_enable) {
      for (uint32_t i = 1; i <= m_first_mp4_parser->get_audio_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_first_mp4_parser->get_audio_frame(frame_info)) {
          ERROR("Failed to get audio frame.");
          ret = false;
          break;
        }
        audio_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (audio_combined_offset_box.m_full_box.m_base_box.m_enable) {
          audio_combined_offset_box.m_chunk_offset[i - 1] =
              (uint32_t) file_offset;
        } else if (audio_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          audio_combined_offset64_box.m_chunk_offset[i - 1] = file_offset;
        } else {
          ERROR("The audio combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write audio data to new mp4 file.");
        break;
      }
      for (uint32_t i = 1; i <= m_second_mp4_parser->get_audio_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_second_mp4_parser->get_audio_frame(frame_info)) {
          ERROR("Failed to get audio frame.");
          ret = false;
          break;
        }
        audio_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (audio_combined_offset_box.m_full_box.m_base_box.m_enable) {
          audio_combined_offset_box.m_chunk_offset[i
                    + m_first_mp4_parser->get_audio_frame_number() - 1] =
              (uint32_t) file_offset;
        } else if (audio_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          audio_combined_offset64_box.m_chunk_offset[i
            + m_first_mp4_parser->get_audio_frame_number() - 1] = file_offset;
        } else {
          ERROR("The audio combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write audio data to new mp4 file.");
        break;
      }
    } else {
      NOTICE("The audio track is not enabled.");
    }
    if (combined_movie_box.m_gps_track.m_base_box.m_enable) {
      for (uint32_t i = 1; i <= m_first_mp4_parser->get_gps_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_first_mp4_parser->get_gps_frame(frame_info)) {
          ERROR("Failed to get gps frame.");
          ret = false;
          break;
        }
        gps_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (gps_combined_offset_box.m_full_box.m_base_box.m_enable) {
          gps_combined_offset_box.m_chunk_offset[i - 1] =
              (uint32_t) file_offset;
        } else if (gps_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          gps_combined_offset64_box.m_chunk_offset[i - 1] = file_offset;
        } else {
          ERROR("The gps combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write gps data to new mp4 file.");
        break;
      }
      for (uint32_t i = 1; i <= m_second_mp4_parser->get_gps_frame_number();
          ++ i) {
        frame_info.frame_number = i;
        if (!m_second_mp4_parser->get_gps_frame(frame_info)) {
          ERROR("Failed to get gps frame.");
          ret = false;
          break;
        }
        gps_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write data to new file.");
          ret = false;
          break;
        }
        if (gps_combined_offset_box.m_full_box.m_base_box.m_enable) {
          gps_combined_offset_box.m_chunk_offset[i
                  + m_first_mp4_parser->get_audio_frame_number() - 1] =
                                                       (uint32_t) file_offset;
        } else if (gps_combined_offset64_box.m_full_box.m_base_box.m_enable) {
          gps_combined_offset64_box.m_chunk_offset[i
            + m_first_mp4_parser->get_audio_frame_number() - 1] = file_offset;
        } else {
          ERROR("The gps combined offset and offset64 are not enabled.");
          ret = false;
          break;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write gps data to new mp4 file.");
        break;
      }
    } else {
      NOTICE("The gps track is not enabled.");
    }
    combined_media_data_box.m_base_box.m_size =
        DISOM_BASE_BOX_SIZE + video_size + audio_size + gps_size;
    INFO("Video size is %u, audio size is %u, gps size is %u, "
        "media data size is %u",
         video_size, audio_size, gps_size,
         combined_media_data_box.m_base_box.m_size);
    if (!mp4_file_writer->seek_file(file_type_box->m_base_box.m_size
                     + combined_movie_box.m_base_box.m_size, SEEK_SET)) {
      ERROR("Failed to seek data.");
      ret = false;
      break;
    }
    NOTICE("Begin to write the size of combined media data box.");
    if (!mp4_file_writer->write_be_u32(
        combined_media_data_box.m_base_box.m_size)) {
      ERROR("Failed to write media data box size.");
      ret = false;
      break;
    }
    if (!mp4_file_writer->seek_file(file_type_box->m_base_box.m_size,
          SEEK_SET)) {
      ERROR("Failed to seek data.");
      ret = false;
      break;
    }
    if (!combined_movie_box.write_movie_box(mp4_file_writer)) {
      ERROR("Failed to write movie box to mp4 file.");
      ret = false;
      break;
    }
    mp4_file_writer->close_file(true);
    INFO("Write new mp4 file success.");
  } while (0);
  if (mp4_file_writer) {
    mp4_file_writer->destroy();
  }
  return ret;
}
