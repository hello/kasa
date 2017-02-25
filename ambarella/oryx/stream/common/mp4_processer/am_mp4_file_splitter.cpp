/*******************************************************************************
 * am_mp4_file_splitter.cpp
 *
 * History:
 *   2015-09-10 - [ccjing] created file
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
#include "am_mp4_file_splitter.h"

#include "am_mp4_file_parser_if.h"
#include "mp4_file_writer.h"

AMIMp4FileSplitter* AMIMp4FileSplitter::create(const char* source_mp4_file_path)
{
  return AMMp4FileSplitter::create(source_mp4_file_path);
}

AMMp4FileSplitter* AMMp4FileSplitter::create(const char* source_mp4_file_path)
{
  AMMp4FileSplitter *result = new AMMp4FileSplitter();
  if (AM_UNLIKELY(result && (!result->init(source_mp4_file_path)))) {
    ERROR("Failed to create mp4 file splitter.");
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMMp4FileSplitter::init(const char *source_mp4_file_path)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((m_mp4_parser = AMIMp4FileParser::create(source_mp4_file_path))
                    == NULL)) {
      ERROR("Failed to create mp4 parser.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

AMMp4FileSplitter::AMMp4FileSplitter() :
    m_mp4_parser(nullptr)
{
}

AMMp4FileSplitter::~AMMp4FileSplitter()
{
  m_mp4_parser->destroy();
}

void AMMp4FileSplitter::destroy()
{
  delete this;
}

bool AMMp4FileSplitter::find_split_frame_num(SplitInfo& split_info,
                                        uint32_t begin_time,uint32_t end_time)
{
  bool ret = true;
  do {
    MovieBox *movie_box = m_mp4_parser->get_movie_box();
    DecodingTimeToSampleBox& video_stts_box = movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    DecodingTimeToSampleBox& audio_stts_box = movie_box->m_audio_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    DecodingTimeToSampleBox& gps_stts_box = movie_box->m_gps_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    uint64_t video_begin_pts = 0;
    uint64_t video_end_pts = 0;
    if (movie_box->m_video_track.m_base_box.m_enable) {
      uint64_t pts = 0;
      uint32_t begin_frame_num = 1;
      uint32_t frame_num = 1;
      uint32_t stts_num = 1;
      uint32_t stts_frame_num = video_stts_box.m_entry_ptr[0].sample_count;
      for (; (frame_num <= m_mp4_parser->get_video_frame_number()) &&
      (stts_num <= video_stts_box.m_entry_count); ++ frame_num) {
        if (pts >= begin_time * 90000) {
          INFO("Find the video begin frame number : %u", frame_num);
          begin_frame_num = frame_num;
          break;
        }
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += video_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
        pts += video_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
      }
      if (AM_UNLIKELY((frame_num >= m_mp4_parser->get_video_frame_number()) &&
                      (begin_frame_num == 1))) {
        ERROR("Do not find the begin frame num.");
        ret = false;
        break;
      }
      /*To find the begin IDR frame*/
      bool is_find_IDR_frame = false;
      SyncSampleTableBox& sync_sample_box = movie_box->m_video_track.\
          m_media.m_media_info.m_sample_table.m_sync_sample;
      for (uint32_t i = 0; i < sync_sample_box.m_stss_count; ++ i) {
        if (begin_frame_num == 1) {
          is_find_IDR_frame = true;
          break;
        } else if ((begin_frame_num < sync_sample_box.m_sync_sample_table[i])) {
          begin_frame_num = sync_sample_box.m_sync_sample_table[i - 1];
          is_find_IDR_frame = true;
          break;
        }
      }
      if (AM_UNLIKELY(!is_find_IDR_frame)) {
        ERROR("Failed to find IDR frame.");
        ret = false;
        break;
      }
      split_info.video_begin_frame_num = begin_frame_num;
      video_begin_pts = 0;
      frame_num = 1;
      stts_num = 1;
      stts_frame_num = video_stts_box.m_entry_ptr[0].sample_count;
      for (; (frame_num < split_info.video_begin_frame_num) &&
      (stts_num <= video_stts_box.m_entry_count); ++ frame_num) {
        video_begin_pts += video_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += video_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
      }
      NOTICE("The video begin IDR frame number is %u, video begin pts is %llu",
             begin_frame_num, video_begin_pts);
      //To find the end video frame.
      TrackHeaderBox& video_track_header_box =
          movie_box->m_video_track.m_track_header;
      INFO("Source file duration is %llu", video_track_header_box.m_duration);
      if (AM_UNLIKELY(video_track_header_box.m_duration
                      <= (end_time * 90000))) {
        WARN("End point time is larger than file duration, "
            "set it file duration as default");
        end_time = video_track_header_box.m_duration / 90000;
        split_info.video_end_frame_num = m_mp4_parser->get_video_frame_number();
        video_end_pts = video_track_header_box.m_duration;
        split_info.duration = video_end_pts - video_begin_pts;
      } else {
        pts = 0;
        frame_num = 1;
        stts_num = 1;
        stts_frame_num = video_stts_box.m_entry_ptr[0].sample_count;
        for (; (frame_num <= m_mp4_parser->get_video_frame_number()) &&
        (stts_num <= video_stts_box.m_entry_count); ++ frame_num) {
          pts += video_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
          if (pts >= end_time * 90000) {
            INFO("Find the video end frame number : %u", frame_num);
            split_info.video_end_frame_num = frame_num;
            break;
          }
          if (stts_frame_num <= frame_num) {
            ++ stts_num;
            stts_frame_num += video_stts_box.m_entry_ptr[stts_num - 1].sample_count;
          }
        }
        video_end_pts = pts;
        split_info.duration = video_end_pts - video_begin_pts;
      }
      NOTICE("The end video frame number is %u, video end pts is %llu",
             split_info.video_end_frame_num, video_end_pts);
    } else {
      ERROR("The video track is not enabled.");
      ret = false;
      break;
    }
    //to Find audio split frame number
    if (movie_box->m_audio_track.m_base_box.m_enable) {
      uint64_t audio_pts = 0;
      uint32_t frame_num = 1;
      uint32_t stts_num = 1;
      uint32_t stts_frame_num = audio_stts_box.m_entry_ptr[0].sample_count;
      for (; (frame_num <= m_mp4_parser->get_audio_frame_number()) &&
      (stts_num <= audio_stts_box.m_entry_count); ++ frame_num) {
        if (audio_pts >= video_begin_pts) {
          INFO("Find the audio begin frame number : %u", frame_num);
          split_info.audio_begin_frame_num = frame_num;
          break;
        }
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += audio_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
        audio_pts += audio_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
      }
      INFO("Audio begin frame number is %u", split_info.audio_begin_frame_num);
      audio_pts = 0;
      frame_num = 1;
      stts_num = 1;
      stts_frame_num = audio_stts_box.m_entry_ptr[0].sample_count;
      INFO("audio frame number is %u", m_mp4_parser->get_audio_frame_number());
      for (; (frame_num <= m_mp4_parser->get_audio_frame_number()) &&
      (stts_num <= audio_stts_box.m_entry_count); ++ frame_num) {
        audio_pts += audio_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
        split_info.audio_end_frame_num = frame_num;
        if (audio_pts >= video_end_pts) {
          INFO("Find the audio end frame number : %u", frame_num);
          break;
        }
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += audio_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
      }
      INFO("audio end frame number is %u", split_info.audio_end_frame_num);
    } else {
      split_info.audio_begin_frame_num = 0;
      split_info.audio_end_frame_num = 0;
      NOTICE("There is not audio in this file");
    }
    //to Find gps split frame number
    if (movie_box->m_gps_track.m_base_box.m_enable) {
      uint64_t gps_pts = 0;
      uint32_t frame_num = 1;
      uint32_t stts_num = 1;
      uint32_t stts_frame_num = gps_stts_box.m_entry_ptr[0].sample_count;
      for (; (frame_num <= m_mp4_parser->get_gps_frame_number()) &&
      (stts_num <= gps_stts_box.m_entry_count); ++ frame_num) {
        gps_pts += gps_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
        if (gps_pts >= video_begin_pts) {
          INFO("Find the gps begin frame number : %u", frame_num);
          split_info.gps_begin_frame_num = frame_num;
          break;
        }
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += gps_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
      }
      INFO("GPS begin frame number is %u", split_info.gps_begin_frame_num);
      gps_pts = 0;
      frame_num = 1;
      stts_num = 1;
      stts_frame_num = gps_stts_box.m_entry_ptr[0].sample_count;
      for (; (frame_num <= m_mp4_parser->get_gps_frame_number()) &&
      (stts_num <= gps_stts_box.m_entry_count); ++ frame_num) {
        split_info.gps_end_frame_num = frame_num;
        if (gps_pts >= video_end_pts) {
          INFO("Find the gps end frame number : %u", frame_num);
          break;
        }
        if (stts_frame_num <= frame_num) {
          ++ stts_num;
          stts_frame_num += gps_stts_box.m_entry_ptr[stts_num - 1].sample_count;
        }
        gps_pts += gps_stts_box.m_entry_ptr[stts_num - 1].sample_delta;
      }
      INFO("GPS end frame number is %u", split_info.gps_end_frame_num);
    } else {
      split_info.gps_begin_frame_num = 0;
      split_info.gps_end_frame_num = 0;
      INFO("There is not gps in this file");
    }
  } while(0);
  return ret;
}

bool AMMp4FileSplitter::get_proper_mp4_file(const char* file_path,
                                            uint32_t begin_point_time,
                                            uint32_t end_point_time)
{
  bool ret = true;
  MovieBox* movie_box_for_new_file = nullptr;
  Mp4FileWriter* mp4_file_writer = nullptr;
  MovieBox *source_file_movie_box = m_mp4_parser->get_movie_box();
  do {
    FrameInfo frame_info;
    INFO("Get proper mp4 file info: file path %s, begin point time %u,"
         "end point time %u",
         file_path, begin_point_time, end_point_time);
    if (end_point_time <= begin_point_time) {
      ERROR("end point time is smaller than begin point time");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!m_mp4_parser->parse_file())) {
      ERROR("Failed to parse mp4 file.");
      ret = false;
      break;
    }
    INFO("parse mp4 file success.");
    TrackHeaderBox& video_track_header_box =
              source_file_movie_box->m_video_track.m_track_header;
    if (begin_point_time * 90000 >= video_track_header_box.m_duration) {
      ERROR("The begin point time is larger than file duration.");
      ret = false;
      break;
    }
    SplitInfo split_info;
    if (!find_split_frame_num(split_info, begin_point_time, end_point_time)) {
      ERROR("Failed to find split frame number.");
      ret = false;
      break;
    }
    /*create the new movie box*/
    if (AM_UNLIKELY((movie_box_for_new_file = new MovieBox()) == NULL)) {
      ERROR("Failed to create movie box.");
      ret = false;
      break;
    }
    MediaInfo media_info;
    if (source_file_movie_box->m_video_track.m_base_box.m_enable) {
      if (!media_info.set_video_first_frame(split_info.video_begin_frame_num)) {
        ERROR("Failed to set video first frame.");
        ret = false;
        break;
      }
      if (!media_info.set_video_last_frame(split_info.video_end_frame_num)) {
        ERROR("Failed to set video last frame");
        ret = false;
        break;
      }
      if (!media_info.set_video_duration(split_info.duration)) {
        ERROR("Failed to set video duration.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video track is not enabled.");
      ret = false;
      break;
    }
    if (source_file_movie_box->m_audio_track.m_base_box.m_enable) {
      if (!media_info.set_audio_duration(split_info.duration)) {
        ERROR("Failed to set audio duration.");
        ret = false;
        break;
      }
      if (!media_info.set_audio_first_frame(split_info.audio_begin_frame_num)) {
        ERROR("Failed to set audio first frame");
        ret = false;
        break;
      }
      if (!media_info.set_audio_last_frame(split_info.audio_end_frame_num)) {
        ERROR("Failed to set audio last frame");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio track is not enabled.");
    }
    if (source_file_movie_box->m_gps_track.m_base_box.m_enable) {
      if (!media_info.set_gps_duration(split_info.duration)) {
        ERROR("Failed to set gps duration.");
        ret = false;
        break;
      }
      if (!media_info.set_gps_first_frame(split_info.gps_begin_frame_num)) {
        ERROR("Failed to set gps first frame");
        ret = false;
        break;
      }
      if (!media_info.set_gps_last_frame(split_info.gps_end_frame_num)) {
        ERROR("Failed to set gps last frame");
        ret = false;
        break;
      }
    } else {
      INFO("The gps track is not enabled.");
    }
    INFO("video start frame is %u, end frame is %u, duration is %llu\n"
         "audio start frame is %u, end frame is %u\n"
         "  gps start frame is %u, end frame is %u.",
         split_info.video_begin_frame_num, split_info.video_end_frame_num,
         split_info.duration,
         split_info.audio_begin_frame_num, split_info.audio_end_frame_num,
         split_info.gps_begin_frame_num, split_info.gps_end_frame_num);
    if (!source_file_movie_box->get_proper_movie_box((*movie_box_for_new_file),
                                                     media_info)) {
      ERROR("Failed to get proper movie box.");
      ret = false;
      break;
    }
    ChunkOffsetBox& video_chunk_offset = movie_box_for_new_file->\
        m_video_track.m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& video_chunk_offset64 = movie_box_for_new_file->\
        m_video_track.m_media.m_media_info.m_sample_table.m_chunk_offset64;
    ChunkOffsetBox& audio_chunk_offset = movie_box_for_new_file->\
        m_audio_track.m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& audio_chunk_offset64 = movie_box_for_new_file->\
        m_audio_track.m_media.m_media_info.m_sample_table.m_chunk_offset64;
    ChunkOffsetBox& gps_chunk_offset = movie_box_for_new_file->\
        m_gps_track.m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffset64Box& gps_chunk_offset64 = movie_box_for_new_file->\
        m_gps_track.m_media.m_media_info.m_sample_table.m_chunk_offset64;
    if (source_file_movie_box->m_video_track.m_base_box.m_enable) {
      uint32_t chunk_count = split_info.video_end_frame_num -
          split_info.video_begin_frame_num + 1;
      if (video_chunk_offset.m_full_box.m_base_box.m_enable) {
        delete[] video_chunk_offset.m_chunk_offset;
        video_chunk_offset.m_chunk_offset = new uint32_t[chunk_count];
        if (!video_chunk_offset.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        video_chunk_offset.m_buf_count = chunk_count;
        video_chunk_offset.m_entry_count = chunk_count;
      } else if (video_chunk_offset64.m_full_box.m_base_box.m_enable) {
        delete[] video_chunk_offset64.m_chunk_offset;
        video_chunk_offset64.m_chunk_offset = new uint64_t[chunk_count];
        if (!video_chunk_offset64.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        video_chunk_offset64.m_buf_count = chunk_count;
        video_chunk_offset64.m_entry_count = chunk_count;
      } else {
        ERROR("The video chunk offset box is not enabled.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video track is not enabled.");
      ret = false;
      break;
    }
    if (source_file_movie_box->m_audio_track.m_base_box.m_enable) {
      uint32_t chunk_count = split_info.audio_end_frame_num -
          split_info.audio_begin_frame_num + 1;
      if (audio_chunk_offset.m_full_box.m_base_box.m_enable) {
        delete[] audio_chunk_offset.m_chunk_offset;
        audio_chunk_offset.m_chunk_offset = new uint32_t[chunk_count];
        if (!audio_chunk_offset.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        audio_chunk_offset.m_buf_count = chunk_count;
        audio_chunk_offset.m_entry_count = chunk_count;
      } else if (audio_chunk_offset64.m_full_box.m_base_box.m_enable) {
        delete[] audio_chunk_offset64.m_chunk_offset;
        audio_chunk_offset64.m_chunk_offset = new uint64_t[chunk_count];
        if (!audio_chunk_offset64.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        audio_chunk_offset64.m_buf_count = chunk_count;
        audio_chunk_offset64.m_entry_count = chunk_count;
      } else {
        ERROR("The audio chunk offset box is not enabled.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio track is not enabled.");
    }
    if (source_file_movie_box->m_gps_track.m_base_box.m_enable) {
      uint32_t chunk_count = split_info.gps_end_frame_num -
          split_info.gps_begin_frame_num + 1;
      if (gps_chunk_offset.m_full_box.m_base_box.m_enable) {
        delete[] gps_chunk_offset.m_chunk_offset;
        gps_chunk_offset.m_chunk_offset = new uint32_t[chunk_count];
        if (!gps_chunk_offset.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        gps_chunk_offset.m_buf_count = chunk_count;
        gps_chunk_offset.m_entry_count = chunk_count;
      } else if (gps_chunk_offset64.m_full_box.m_base_box.m_enable) {
        delete[] gps_chunk_offset64.m_chunk_offset;
        gps_chunk_offset64.m_chunk_offset = new uint64_t[chunk_count];
        if (!gps_chunk_offset64.m_chunk_offset) {
          ERROR("Failed to malloc memory.");
          ret = false;
          break;
        }
        gps_chunk_offset64.m_buf_count = chunk_count;
        gps_chunk_offset64.m_entry_count = chunk_count;
      } else {
        ERROR("The gps chunk offset box is not enabled.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The gps track is not enabled.");
    }
    movie_box_for_new_file->update_movie_box_size();
    mp4_file_writer = Mp4FileWriter::create();
    if (AM_UNLIKELY(!mp4_file_writer)) {
      ERROR("Failed to create mp4 file writer.");
      ret = false;
      break;
    }
    if (!mp4_file_writer->create_file(file_path)) {
      ERROR("Failed to create file %s", file_path);
      ret = false;
      break;
    }
    INFO("movie box size is %u", movie_box_for_new_file->m_base_box.m_size);
    FileTypeBox* file_type_box = m_mp4_parser->get_file_type_box();
    MediaDataBox* media_data_box = m_mp4_parser->get_media_data_box();
    file_type_box->write_file_type_box(mp4_file_writer);
    mp4_file_writer->seek_file(movie_box_for_new_file->m_base_box.m_size,
    SEEK_CUR);
    uint64_t file_offset = 0;
    if (AM_UNLIKELY(!mp4_file_writer->tell_file(file_offset))) {
      ERROR("Failed to get offset of mp4 file");
      ret = false;
      break;
    }
    media_data_box->write_media_data_box(mp4_file_writer);
    file_offset += DISOM_BASE_BOX_SIZE;
    uint32_t video_size = 0;
    uint32_t audio_size = 0;
    uint32_t gps_size = 0;
    INFO("Begin to write video data.");
    if (source_file_movie_box->m_video_track.m_base_box.m_enable) {
      for (uint32_t i = split_info.video_begin_frame_num;
          i <= split_info.video_end_frame_num; ++ i) {
        frame_info.frame_number = i;
        if (!m_mp4_parser->get_video_frame(frame_info)) {
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
        if (video_chunk_offset.m_full_box.m_base_box.m_enable) {
          video_chunk_offset.m_chunk_offset[i - split_info.video_begin_frame_num]
                                            = (uint32_t) file_offset;
        } else if (video_chunk_offset64.m_full_box.m_base_box.m_enable) {
          video_chunk_offset64.m_chunk_offset[i - split_info.video_begin_frame_num]
                                              = file_offset;
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
    INFO("Write video data success.");
    INFO("Begin to write audio data.");
    if (source_file_movie_box->m_audio_track.m_base_box.m_enable) {
      for (uint32_t i = split_info.audio_begin_frame_num;
          i <= split_info.audio_end_frame_num; ++ i) {
        frame_info.frame_number = i;
        if (!m_mp4_parser->get_audio_frame(frame_info)) {
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
        if (audio_chunk_offset.m_full_box.m_base_box.m_enable) {
          audio_chunk_offset.m_chunk_offset[i - split_info.audio_begin_frame_num]
                                            = (uint32_t) file_offset;
        } else if (audio_chunk_offset64.m_full_box.m_base_box.m_enable) {
          audio_chunk_offset64.m_chunk_offset[i - split_info.audio_begin_frame_num]
                                              = file_offset;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write audio data to new mp4 file.");
        break;
      }
      INFO("Write audio data success.");
    } else {
      NOTICE("The audio track is not enabled.");
    }
    INFO("Begin to write gps data.");
    if (source_file_movie_box->m_gps_track.m_base_box.m_enable) {
      for (uint32_t i = split_info.gps_begin_frame_num;
          i <= split_info.gps_end_frame_num; ++ i) {
        frame_info.frame_number = i;
        if (!m_mp4_parser->get_gps_frame(frame_info)) {
          ERROR("Failed to get gps frame.");
          ret = false;
          break;
        }
        gps_size += frame_info.frame_size;
        if (!mp4_file_writer->write_data(frame_info.frame_data,
                                         frame_info.frame_size)) {
          ERROR("Failed to write gps data to new file.");
          ret = false;
          break;
        }
        if (gps_chunk_offset.m_full_box.m_base_box.m_enable) {
          gps_chunk_offset.m_chunk_offset[i - split_info.gps_begin_frame_num]
                                            = (uint32_t) file_offset;
        } else if (gps_chunk_offset64.m_full_box.m_base_box.m_enable) {
          gps_chunk_offset64.m_chunk_offset[i - split_info.gps_begin_frame_num]
                                              = file_offset;
        }
        file_offset += frame_info.frame_size;
      }
      if (!ret) {
        ERROR("Failed to write gps data to new mp4 file.");
        break;
      }
      INFO("Write gps data success.");
    } else {
      NOTICE("The gps track is not enabled.");
    }
    media_data_box->m_base_box.m_size = DISOM_BASE_BOX_SIZE + video_size
            + audio_size + gps_size;
    INFO("video size is %u, audio size is %u, gps size is %u, "
        "media data size is %u",
         video_size, audio_size, gps_size, media_data_box->m_base_box.m_size);
    if (!mp4_file_writer->seek_file(file_type_box->m_base_box.m_size
                + movie_box_for_new_file->m_base_box.m_size, SEEK_SET)) {
      ERROR("Failed to seek data.");
      ret = false;
      break;
    }
    if (!mp4_file_writer->write_be_u32(media_data_box->m_base_box.m_size)) {
      ERROR("failed to write u32.");
      ret = false;
      break;
    }
    if (!mp4_file_writer->seek_file(file_type_box->m_base_box.m_size,
                                    SEEK_SET)) {
      ERROR("Failed to seek data.");
      ret = false;
      break;
    }
    if (!movie_box_for_new_file->write_movie_box(mp4_file_writer)) {
      ERROR("Failed to write movie box to mp4 file.");
      ret = false;
      break;
    }
    mp4_file_writer->close_file(true);
    INFO("Write new mp4 file success.");
  } while (0);
  delete movie_box_for_new_file;
  if (mp4_file_writer) {
    mp4_file_writer->destroy();
  }
  return ret;
}

