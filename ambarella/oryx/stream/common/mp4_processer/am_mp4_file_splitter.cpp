/*******************************************************************************
 * am_mp4_file_splitter.cpp
 *
 * History:
 *   2015-09-10 - [ccjing] created file
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
#include "am_mp4_file_splitter.h"

#include "mp4_parser.h"
#include "mp4_file_writer.h"

AMIMp4FileSplitter* AMIMp4FileSplitter::create(const char* source_mp4_file_path)
{
  return AMMp4FileSplitter::create(source_mp4_file_path);
}

AMMp4FileSplitter* AMMp4FileSplitter::create(const char* source_mp4_file_path)
{
  AMMp4FileSplitter *result = new AMMp4FileSplitter();
  if(AM_UNLIKELY(result && (!result->init(source_mp4_file_path)))) {
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
    if(AM_UNLIKELY((m_mp4_parser = AMMp4Parser::create(source_mp4_file_path))
                   == NULL)) {
      ERROR("Failed to create mp4 parser.");
      ret = false;
      break;
    }
  } while(0);
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

bool AMMp4FileSplitter::get_proper_mp4_file(const char* file_path,
                                            uint32_t begin_point_time,
                                            uint32_t end_point_time)
{
  bool ret = true;
  MovieBox* movie_box_for_new_file = NULL;
  Mp4FileWriter* mp4_file_writer = NULL;
  do{
    FrameInfo frame_info;
    INFO("get proper mp4 file info: file path %s, begin point time %u,"
        "end point time %u", file_path, begin_point_time,
        end_point_time);
    if(end_point_time <= begin_point_time) {
      ERROR("end point time is smaller than begin point time");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!m_mp4_parser->parse_file())) {
      ERROR("Failed to parse mp4 file.");
      ret = false;
      break;
    }
    INFO("parse mp4 file success.");
    /*find the frame of split point*/
    MovieBox *movie_box = m_mp4_parser->get_movie_box();
    DecodingTimeToSampleBox& video_stts_box = movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
    DecodingTimeToSampleBox& audio_stts_box = movie_box->m_audio_track.\
            m_media.m_media_info.m_sample_table.m_stts;
    uint64_t pts = 0;
    uint32_t video_begin_frame_number = 1;
    uint32_t audio_begin_frame_number = 1;
    uint32_t video_end_frame_number = 1;
    uint32_t audio_end_frame_number = 1;
    while(pts < begin_point_time * 90000) {
      if(video_begin_frame_number <= video_stts_box.m_entry_count) {
        pts += video_stts_box.m_p_entry[video_begin_frame_number - 1].sample_delta;
        ++ video_begin_frame_number;
      } else {
        ERROR("The split point time is larger than file length.");
        ret = false;
        break;
      }
    }
    if(AM_UNLIKELY(!ret)) {
      break;
    }
    INFO("begin video frame number is %u", video_begin_frame_number);
    /*To find the begin IDR frame*/
    bool is_find_IDR_frame = false;
    SyncSampleTableBox& sync_sample_box = movie_box->m_video_track.\
        m_media.m_media_info.m_sample_table.m_sync_sample;
    for(uint32_t i = 0; i < sync_sample_box.m_stss_count; ++ i) {
      if(video_begin_frame_number == 1) {
        is_find_IDR_frame = true;
        break;
      } else if((video_begin_frame_number < sync_sample_box.m_sync_sample_table[i])){
        video_begin_frame_number = sync_sample_box.m_sync_sample_table[i - 1];
        is_find_IDR_frame = true;
        break;
      }
    }
    if(AM_UNLIKELY(!is_find_IDR_frame)) {
      ERROR("Failed to find IDR frame.");
      ret = false;
      break;
    }
    INFO("The begin IDR frame number is %u", video_begin_frame_number);
    TrackHeaderBox& video_track_header_box = movie_box->m_video_track.m_track_header;
    INFO("source file duration is %llu", video_track_header_box.m_duration);
    if(AM_UNLIKELY(video_track_header_box.m_duration < (end_point_time * 90000))) {
      ERROR("End point time is longer than file duration.");
      ret = false;
      break;
    }
    pts = 0;
    video_end_frame_number = 0;
    for(uint32_t i = 0; i < video_stts_box.m_entry_count; ++ i) {
      pts += video_stts_box.m_p_entry[i].sample_delta;
      ++ video_end_frame_number;
      if(pts >= end_point_time * 90000) {
        break;
      }
    }
    INFO("video end frame number is %u", video_end_frame_number);
    pts = 0;
    for(uint32_t i = 0; i < video_begin_frame_number; ++ i) {
      pts += video_stts_box.m_p_entry[i].sample_delta;
    }
    audio_begin_frame_number = pts / audio_stts_box.m_p_entry[0].sample_delta;
    INFO("audio begin frame number is %u", audio_begin_frame_number);
    pts = 0;
    for(uint32_t i = 0; i < video_end_frame_number; ++ i) {
      pts += video_stts_box.m_p_entry[i].sample_delta;
    }
    audio_end_frame_number = pts / audio_stts_box.m_p_entry[0].sample_delta;
    INFO("audio end frame number is %u", audio_end_frame_number);
    /*create the new movie box*/
    if(AM_UNLIKELY((movie_box_for_new_file = new MovieBox()) == NULL)) {
      ERROR("Failed to create movie box.");
      ret = false;
      break;
    }
    WriteInfo write_info;
    if(!write_info.set_video_first_frame(video_begin_frame_number)) {
      ERROR("Failed to set video first frame.");
      ret = false;
      break;
    }
    if(!write_info.set_video_last_frame(video_end_frame_number)) {
      ERROR("Failed to set video last frame");
      ret = false;
      break;
    }
    if(!write_info.set_video_duration((end_point_time - begin_point_time) * 90000)) {
      ERROR("Failed to set video duration.");
      ret = false;
      break;
    }
    if(!write_info.set_audio_duration((end_point_time - begin_point_time) * 90000)) {
      ERROR("Failed to set audio duration.");
      ret = false;
      break;
    }
    if(!write_info.set_audio_first_frame(audio_begin_frame_number)) {
      ERROR("Failed to set audio first frame");
      ret = false;
      break;
    }
    if(!write_info.set_audio_last_frame(audio_end_frame_number)) {
      ERROR("Failed to set audio last frame");
      ret = false;
      break;
    }

    if(!movie_box->get_proper_movie_box(*movie_box_for_new_file,
                                        write_info)) {
      ERROR("Failed to get proper movie box.");
      ret = false;
      break;
    }
    movie_box_for_new_file->update_movie_box_size();
    mp4_file_writer = Mp4FileWriter::create(file_path);
    if(AM_UNLIKELY(!mp4_file_writer)) {
      ERROR("Failed to create mp4 file writer.");
      ret = false;
      break;
    }
    INFO("movie box size is %u", movie_box_for_new_file->m_base_box.m_size);
    FileTypeBox* file_type_box = m_mp4_parser->get_file_type_box();
    MediaDataBox* media_data_box = m_mp4_parser->get_media_data_box();
    file_type_box->write_file_type_box(mp4_file_writer);
    mp4_file_writer->seek_data(movie_box_for_new_file->m_base_box.m_size,
                               SEEK_CUR);
    uint64_t file_offset = 0;
    if(AM_UNLIKELY(!mp4_file_writer->tell_file(file_offset))) {
      ERROR("Failed to get offset of mp4 file");
      ret = false;
      break;
    }
    media_data_box->write_media_data_box(mp4_file_writer);
    file_offset += sizeof(MediaDataBox);
    uint32_t video_size = 0;
    uint32_t audio_size = 0;
    ChunkOffsetBox& video_chunk_offset_box = movie_box_for_new_file->\
        m_video_track.m_media.m_media_info.m_sample_table.m_chunk_offset;
    ChunkOffsetBox& audio_chunk_offset_box = movie_box_for_new_file->\
        m_audio_track.m_media.m_media_info.m_sample_table.m_chunk_offset;
    for(uint32_t i = video_begin_frame_number; i < video_end_frame_number; ++ i) {
      frame_info.frame_number = i;
      if (!m_mp4_parser->get_video_frame(frame_info)) {
        ERROR("Failed to get video frame.");
        ret = false;
        break;
      }
      video_size += frame_info.frame_size;
      if(!mp4_file_writer->write_data(frame_info.frame_data,
                                      frame_info.frame_size)) {
        ERROR("Failed to write data to new file.");
        ret = false;
        break;
      }
      video_chunk_offset_box.m_chunk_offset[i - video_begin_frame_number]
                                            = (uint32_t)file_offset;

      file_offset += frame_info.frame_size;
    }
    if(!ret) {
      ERROR("Failed to write video data to new mp4 file");
      break;
    }
    for(uint32_t i = audio_begin_frame_number; i < audio_end_frame_number; ++ i) {
      frame_info.frame_number = i;
      if (!m_mp4_parser->get_audio_frame(frame_info)) {
        ERROR("Failed to get audio frame.");
        ret = false;
        break;
      }
      audio_size += frame_info.frame_size;
      if(!mp4_file_writer->write_data(frame_info.frame_data,
                                      frame_info.frame_size)) {
        ERROR("Failed to write data to new file.");
        ret = false;
        break;
      }
      audio_chunk_offset_box.m_chunk_offset[i - audio_begin_frame_number]
                                            = (uint32_t)file_offset;
      file_offset += frame_info.frame_size;
    }
    if(!ret) {
      ERROR("Failed to write audio data to new mp4 file.");
      break;
    }
    media_data_box->m_base_box.m_size = sizeof(BaseBox) + video_size + audio_size;
    INFO("video size is %u, audio size is %u, media data size is %u",
           video_size, audio_size, media_data_box->m_base_box.m_size);
    if(!mp4_file_writer->seek_data(file_type_box->m_base_box.m_size +
                     movie_box_for_new_file->m_base_box.m_size, SEEK_SET)) {
      ERROR("Failed to seek data.");
      ret = false;
      break;
    }
    if(!mp4_file_writer->write_be_u32(media_data_box->m_base_box.m_size)) {
      ERROR("failed to write u32.");
      ret = false;
      break;
    }
    if(!mp4_file_writer->seek_data(file_type_box->m_base_box.m_size,
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
    mp4_file_writer->close_file();
    INFO("Write new mp4 file success.");
  }while(0);
  delete movie_box_for_new_file;
  if(mp4_file_writer) {
    mp4_file_writer->destroy();
  }
  return ret;
}

