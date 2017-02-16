/*******************************************************************************
 * am_muxer_mp4_builder.cpp
 *
 * History:
 *   2014-12-02 - [Zhi He] created file
 *
 * Copyright (C) 2014, Ambarella Co, Ltd.
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

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_utils.h"
#include "am_muxer_codec_if.h"
#include "am_muxer_codec_info.h"
#include "am_muxer_mp4_helper.h"
#include "am_file_sink_if.h"
#include "am_muxer_mp4_file_writer.h"
#include "am_muxer_mp4_builder.h"
#include "iso_base_media_file_defs.h"

#include <arpa/inet.h>


#define MP4_MOOV_BOX_PRE_LENGTH (1350 * 2 + \
        (28 * m_video_info.scale/m_video_info.rate + \
        28 * 90000 / m_audio_info.pkt_pts_increment) * m_file_time_length)

AMMuxerMp4Builder* AMMuxerMp4Builder::create(AMMp4FileWriter* file_writer,
                                             uint64_t file_time_length,
                                             uint32_t hls_enable)
{
  AMMuxerMp4Builder *result = new AMMuxerMp4Builder();
  if (result && result->init(file_writer, file_time_length, hls_enable)
      != AM_STATE_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AMMuxerMp4Builder::AMMuxerMp4Builder():
    m_overall_media_data_len(0),
    m_audio_pkt_frame_number(1),
    m_video_duration(0),
    m_audio_duration(0),
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0xffffffffffffffff),
    m_last_pts(0),
    m_last_ctts(0),
    m_video_frame_number(0),
    m_audio_frame_number(0),
    m_audio_frame_count_per_packet(0),
    m_file_time_length(0),
    m_hls_enable(0),
    m_rate(0),
    m_volume(0),
    m_used_version(0),
    m_video_track_id(1),
    m_audio_track_id(2),
    m_next_track_id(3),
    m_flags(0),
    m_curr_adts_index(0)
{
  memcpy(m_video_codec_name, "avc1", 4);
  memcpy(m_audio_codec_name, "mp4a", 4);

  m_avc_sps = NULL;
  m_avc_sps_length = 0;

  m_avc_pps = NULL;
  m_avc_pps_length = 0;

  m_avc_profile_indicator = 0;
  m_avc_level_indicator = 0;

  m_write_media_data_started = 0;

  m_audio_max_bitrate = 64000;
  m_audio_avg_bitrate = 64000;

  m_audio_extradata = NULL;
  m_audio_extradata_length = 0;
  m_audio_extradata_buffer_length = 0;
  m_aac_spec_config = 0xffff;

  m_file_writer = NULL;
  m_adts.clear();

  memset(&m_file_type_box, 0x0, sizeof(m_file_type_box));
  memset(&m_media_data_box, 0x0, sizeof(m_media_data_box));
  memset(&m_movie_box, 0x0, sizeof(m_movie_box));
}

AM_STATE AMMuxerMp4Builder::init(AMMp4FileWriter* file_writer,
                                 uint64_t file_time_length, uint32_t hls_enable)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    memset(m_video_compressor_name, 0x0, sizeof(m_video_compressor_name));
    memset(m_audio_compressor_name, 0x0, sizeof(m_audio_compressor_name));
    default_mp4_file_setting();
    if (AM_UNLIKELY(!file_writer)) {
      ERROR("Invalid parameter.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_file_writer = file_writer;
    m_file_time_length = file_time_length;
    m_hls_enable = hls_enable;
    m_nalu_list.clear();
  } while (0);
  return ret;
}

AMMuxerMp4Builder::~AMMuxerMp4Builder()
{
  delete[] m_avc_sps;
  delete[] m_avc_pps;
  delete[] m_audio_extradata;

  m_file_writer = NULL;
  m_nalu_list.clear();
}

AM_STATE AMMuxerMp4Builder::set_video_info(AM_VIDEO_INFO* video_info)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(!video_info)) {
    ERROR("Invalid parameter of set video info");
    ret = AM_STATE_ERROR;
  } else {
    memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::set_audio_info(AM_AUDIO_INFO* audio_info)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(!audio_info)) {
    ERROR("Invalid parameter of set audio info");
    ret = AM_STATE_ERROR;
  } else {
    memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::write_video_data(AMPacket *packet,
                                             AMPacket* usr_sei)
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t chunk_offset = m_file_writer->get_file_offset();
  H264_NALU_TYPE expect_type = H264_IBP_HEAD;
  uint8_t sync_sample = 0;
  if(AM_UNLIKELY(m_write_media_data_started == 0)) {
    m_write_media_data_started = 1;
  }
  switch(packet->get_frame_type()) {
    case AM_VIDEO_FRAME_TYPE_H264_I:
    case AM_VIDEO_FRAME_TYPE_H264_B:
    case AM_VIDEO_FRAME_TYPE_H264_P:
      expect_type = H264_IBP_HEAD;
      break;
    case AM_VIDEO_FRAME_TYPE_H264_IDR:
      expect_type = H264_IDR_HEAD;
      sync_sample = 1;
      break;
    default:
      break;
  }
  if(AM_LIKELY(find_nalu(packet->get_data_ptr(), packet->get_data_size(),
                         expect_type))) {
    for(uint32_t i = 0;
        (i < m_nalu_list.size()) && (ret == AM_STATE_OK); ++ i) {
      AM_H264_NALU &nalu = m_nalu_list[i];
      switch (nalu.type) {
        case H264_SPS_HEAD: {
          if (m_avc_sps == NULL) {
            m_avc_sps = new uint8_t[nalu.size];
            m_avc_sps_length = nalu.size;
            INFO("sps length is %u", m_avc_sps_length);
            if (AM_UNLIKELY(!m_avc_sps)) {
              ERROR("Failed to alloc memory for sps info.");
              ret = AM_STATE_ERROR;
              break;
            }
            memcpy(m_avc_sps, nalu.addr, nalu.size);
          }
        }break;
        case H264_PPS_HEAD: {
          if (m_avc_pps == NULL) {
            m_avc_pps = new uint8_t[nalu.size];
            if (AM_UNLIKELY(!m_avc_pps)) {
              ERROR("Failed to alloc memory for pps info.");
              ret = AM_STATE_ERROR;
              break;
            }
            m_avc_pps_length = nalu.size;
            INFO("pps length is %u", m_avc_pps_length);
            memcpy(m_avc_pps, nalu.addr, nalu.size);
          }
        }break;
        case H264_AUD_HEAD: {
          if (usr_sei) {
            uint32_t length = usr_sei->get_data_size() - 4;
            if (AM_UNLIKELY(m_file_writer->write_be_u32(length) !=
                AM_STATE_OK)) {
              ERROR("Failed to write sei data length.");
              ret = AM_STATE_ERROR;
              break;
            }
            if (AM_UNLIKELY(m_file_writer->
                            write_data(usr_sei->get_data_ptr() + 4, length) !=
                                AM_STATE_OK)) {
              ERROR("Failed to write sei data to file.");
              ret = AM_STATE_ERROR;
              break;
            }
            INFO("Get usr SEI %d", length);
          }
        }break;
        case H264_IBP_HEAD:
        case H264_IDR_HEAD: {
          if (AM_UNLIKELY(m_file_writer->write_be_u32(nalu.size) !=
              AM_STATE_OK)) {
            ERROR("Write nalu length error.");
            ret = AM_STATE_ERROR;
            break;
          }
          if (AM_UNLIKELY(m_file_writer->write_data(nalu.addr, nalu.size) !=
              AM_STATE_OK)) {
            ERROR("Failed to nalu data to file.");
            ret = AM_STATE_ERROR;
            break;
          }
        }break;
        default: {
          NOTICE("nalu type is not needed, drop this nalu");
        }break;
      }
    }
  } else {
    ERROR("Find nalu error.");
    ret = AM_STATE_ERROR;
  }

  do {
    int64_t delta_pts = 0;
    if(AM_UNLIKELY(ret != AM_STATE_OK)) {
      break;
    }
    m_overall_media_data_len += m_file_writer->get_file_offset() - chunk_offset;
    if(m_video_frame_number == 0) {
      delta_pts = 90000 * m_video_info.rate / m_video_info.scale;
    } else {
      delta_pts = packet->get_pts() - m_last_pts;
    }
    m_last_pts = packet->get_pts();
    m_video_duration += delta_pts < 0 ? 0 : delta_pts;
    ++ m_video_frame_number;
    if (AM_UNLIKELY(write_video_frame_info(delta_pts, chunk_offset,
                 m_file_writer->get_file_offset() - chunk_offset, sync_sample)
        != AM_STATE_OK)) {
      ERROR("Failed to write video frame info.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::write_audio_data(uint8_t *pdata, uint32_t len,
                                             uint32_t frame_count)
{
  AM_STATE ret = AM_STATE_OK;
  uint8_t *frame_start;
  uint32_t frame_size;
  if (AM_UNLIKELY(m_write_media_data_started == 0)) {
    m_write_media_data_started = 1;
  }
  feed_stream_data(pdata, len, frame_count);
  m_audio_pkt_frame_number = frame_count;
  while (get_one_frame(&frame_start, &frame_size)) {
    uint64_t chunk_offset = m_file_writer->get_file_offset();
    AdtsHeader *adts_header = (AdtsHeader*) frame_start;
    uint32_t header_length = sizeof(AdtsHeader);
    if (AM_UNLIKELY(frame_size < 7)) {
      ERROR("Audio frame size is too short.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(false == adts_header->is_sync_word_ok())) {
      ERROR("Adts is sync word ok error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_LIKELY(adts_header->aac_frame_number() == 0)) {
      if (adts_header->protection_absent() == 0) {
        header_length += 2;
      }
      if (AM_UNLIKELY(0xffff == m_aac_spec_config)) {
        m_aac_spec_config = (((adts_header->aac_audio_object_type() << 11) |
            (adts_header->aac_frequency_index() << 7)   |
            (adts_header->aac_channel_conf() << 3)) & 0xFFF8);
      }
      if (AM_UNLIKELY(m_file_writer->write_data(frame_start + header_length,
                                 frame_size - header_length) != AM_STATE_OK)) {
        ERROR("Failed to write data to file.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      NOTICE("Adts header aac frame number is not zero, Do not write "
             "this frame to file.");
    }
    m_overall_media_data_len += frame_size - header_length;

    ++ m_audio_frame_number;
    if (AM_UNLIKELY(write_audio_frame_info(chunk_offset,
       (uint64_t)(m_file_writer->get_file_offset() - chunk_offset))
                    != AM_STATE_OK)) {
      ERROR("Failed to write audio frame info.");
      ret = AM_STATE_ERROR;
      break;
    }
  }
  m_audio_duration += (
      m_audio_frame_number != 0 ? m_audio_info.pkt_pts_increment :
                                  m_audio_info.pkt_pts_increment * 2);
  return ret;
}


void AMMuxerMp4Builder::destroy()
{
  delete this;
}

AM_MUXER_CODEC_TYPE AMMuxerMp4Builder::get_muxer_type()
{
  return AM_MUXER_CODEC_MP4;
}

/*AM_STATE AMMuxerMp4Builder::set_specified_info(AMMuxerCodecSpecifiedInfo *info)
{
  if (info->video_compressorname[0]) {
    snprintf(m_video_compressor_name, 32, "%s", info->video_compressorname);
  }

  if (info->audio_compressorname[0]) {
    snprintf(m_audio_compressor_name, 32, "%s", info->audio_compressorname);
  }

  m_const_framerate = info->b_video_const_framerate;
  m_have_bframe = info->b_video_have_b_frame;

  m_creation_time = __gmtime(&info->create_time);
  m_modification_time = __gmtime(&info->modification_time);

  m_time_scale = info->timescale;
  m_video_frame_tick = info->video_frametick;
  m_audio_frame_tick = info->audio_frametick;

  m_video_enabled = info->enable_video;
  m_audio_enabled = info->enable_audio;

  return AM_STATE_OK;
}*/

/*AM_STATE AMMuxerMp4Builder::set_video_extradata(uint8_t *p_data, uint32_t data_size)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!p_data || !data_size) {
      ERROR("bad parameters p_data %p, data_size %d\n", p_data, data_size);
      ret = AM_STATE_BAD_PARAM;
      break;
    }

    if (AM_UNLIKELY(get_avc_sps_pps(p_data, data_size) != AM_STATE_OK)) {
      ERROR("Failed to get avc sps and pps.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_get_video_extradata = 1;
  } while (0);
  return ret;
}*/

/*AM_STATE AMMuxerMp4Builder::set_audio_extradata(uint8_t *p_data, uint32_t data_size)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (!p_data || !data_size) {
      ERROR("bad parameters p_data %p, data_size %d\n", p_data, data_size);
      ret = AM_STATE_BAD_PARAM;
      break;
    }

    if (m_audio_extradata_buffer_length < data_size) {
      if (m_audio_extradata) {
        free(m_audio_extradata);
      }
      m_audio_extradata_buffer_length = data_size;
      m_audio_extradata = (uint8_t*) malloc(m_audio_extradata_buffer_length);
      if (AM_UNLIKELY(!m_audio_extradata)) {
        ERROR("not enough memory, request size %d\n",
              m_audio_extradata_buffer_length);
        ret = AM_STATE_NO_MEMORY;
        break;
      }
    }
    m_audio_extradata_length = data_size;
    memcpy(m_audio_extradata, p_data, m_audio_extradata_length);
    m_get_audio_extradata = 1;
  } while (0);
  return ret;
}
*/
void AMMuxerMp4Builder::default_mp4_file_setting()
{
  m_used_version = 0;
  m_flags = 0;

  m_rate = (1 << 16);
  m_volume = (1 << 8);

  m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[5] = m_matrix[6] =
      m_matrix[7] = 0;
  m_matrix[0] = m_matrix[4] = 0x00010000;
  m_matrix[8] = 0x40000000;

  m_avc_profile_indicator = 100;
  m_avc_level_indicator = 40;

  m_audio_info.channels = 2;
  m_audio_info.sample_size = 16;
  m_audio_info.sample_rate = 48000;

  snprintf(m_video_compressor_name, 32, "%s", DVIDEO_COMPRESSORNAME);
  snprintf(m_audio_compressor_name, 32, "%s", DAUDIO_COMPRESSORNAME);
}

bool AMMuxerMp4Builder::find_nalu(uint8_t *data,
                                  uint32_t len,
                                  H264_NALU_TYPE expect)
{
  bool ret = false;
  m_nalu_list.clear();
  if (AM_LIKELY(data && (len > 4))) {
    int32_t index = -1;
    uint8_t *bs = data;
    uint8_t *last_header = bs + len - 4;

    while (bs <= last_header) {
      if (AM_UNLIKELY(0x00000001
          == ((bs[0] << 24) | (bs[1] << 16) | (bs[2] << 8) | bs[3]))) {
        AM_H264_NALU nalu;
        ++ index;
        bs += 4;
        nalu.type = H264_NALU_TYPE((int32_t) (0x1F & bs[0]));
        nalu.addr = bs;
        m_nalu_list.push_back(nalu);
        if (AM_LIKELY(index > 0)) {
          AM_H264_NALU &prev_nalu = m_nalu_list[index - 1];
          AM_H264_NALU &curr_nalu = m_nalu_list[index];
          /* calculate the previous NALU's size */
          prev_nalu.size = curr_nalu.addr - prev_nalu.addr - 4;
        }
        if (AM_LIKELY(nalu.type == expect)) {
          /* The last NALU has found */
          break;
        }
      } else if (bs[3] != 0) {
        bs += 4;
      } else if (bs[2] != 0) {
        bs += 3;
      } else if (bs[1] != 0) {
        bs += 2;
      } else {
        bs += 1;
      }
    }
    if (AM_LIKELY(index >= 0)) {
      /* calculate the last NALU's size */
      AM_H264_NALU &last_nalu = m_nalu_list[index];
      last_nalu.size = data + len - last_nalu.addr;
      ret = true;
    }
  } else {
    if (AM_LIKELY(!data)) {
      ERROR("Invalid bit stream!");
    }
    if (AM_LIKELY(len <= 4)) {
      ERROR("Bit stream is less equal than 4 bytes!");
    }
  }

  return ret;
}

void AMMuxerMp4Builder::find_adts (uint8_t *bs)
{
  m_adts.clear();
  for (uint32_t i = 0; i < m_audio_frame_count_per_packet; ++ i) {
    ADTS adts;
    adts.addr = bs;
    adts.size = ((AdtsHeader*)bs)->frame_length();
    bs += adts.size;
    m_adts.push_back(adts);
  }
}

void AMMuxerMp4Builder::feed_stream_data(uint8_t *input_buf,
                                 uint32_t input_size,  uint32_t frame_count)
{
  m_audio_frame_count_per_packet =  frame_count;
  find_adts(input_buf);
  m_curr_adts_index = 0;
}

bool AMMuxerMp4Builder::get_one_frame(uint8_t **frame_start,
                                 uint32_t *frame_size)
{
  if (m_curr_adts_index >= m_audio_frame_count_per_packet) {
    return false;
  }
  *frame_start  = m_adts[m_curr_adts_index].addr;
  *frame_size    = m_adts[m_curr_adts_index].size;
  ++ m_curr_adts_index;

  return true;
}

void AMMuxerMp4Builder::fill_file_type_box()
{
  SISOMFileTypeBox& box = m_file_type_box;
  box.m_base_box.m_type = DISOM_FILE_TYPE_BOX_TAG;
  box.m_major_brand = DISOM_BRAND_V0_TAG;
  box.m_minor_version = 0;
  box.m_compatible_brands[0] = DISOM_BRAND_V0_TAG;
  box.m_compatible_brands[1] = DISOM_BRAND_V1_TAG;
  box.m_compatible_brands[2] = DISOM_BRAND_AVC_TAG;
  box.m_compatible_brands[3] = DISOM_BRAND_MPEG4_TAG;
  box.m_compatible_brands_number = 4;
  box.m_base_box.m_size = sizeof(m_file_type_box.m_base_box) +
                          sizeof(m_file_type_box.m_major_brand) +
                          sizeof(m_file_type_box.m_minor_version) +
                (m_file_type_box.m_compatible_brands_number * DISOM_TAG_SIZE);
}

void AMMuxerMp4Builder::fill_free_box(uint32_t size)
{
  m_free_box.m_base_box.m_type = DISOM_FREE_BOX_TAG;
  m_free_box.m_base_box.m_size = size;
}

void AMMuxerMp4Builder::fill_media_data_box()
{
  SISOMMediaDataBox& box = m_media_data_box;
  box.m_base_box.m_type = DISOM_MEDIA_DATA_BOX_TAG;
  box.m_base_box.m_size = sizeof(SISOMMediaDataBox);
}

void AMMuxerMp4Builder::fill_movie_header_box()
{
  SISOMMovieHeaderBox& box = m_movie_box.m_movie_header_box;
  box.m_base_full_box.m_base_box.m_type = DISOM_MOVIE_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = m_duration;
  box.m_rate = m_rate;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_next_track_ID = m_next_track_id;
  if (!m_used_version) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMovieHeaderSize;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMovieHeader64Size;
  }
}

void AMMuxerMp4Builder::fill_movie_video_track_header_box()
{
  SISOMTrackHeaderBox& box = m_movie_box.m_video_track.m_track_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = 0x07;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_video_track_id;
  box.m_duration = m_video_duration;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_width = m_video_info.width;
  box.m_height = m_video_info.height;

  if (!m_used_version) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstTrackHeaderSize;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstTrackHeader64Size;
  }
}

void AMMuxerMp4Builder::fill_movie_audio_track_header_box()
{
  SISOMTrackHeaderBox& box = m_movie_box.m_audio_track.m_track_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = 0x07;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_audio_track_id;
  box.m_duration = m_audio_duration;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  if (!m_used_version) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstTrackHeaderSize;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstTrackHeader64Size;
  }
}

void AMMuxerMp4Builder::fill_media_header_box_for_video_track()
{
  SISOMMediaHeaderBox& box = m_movie_box.m_video_track.m_media.m_media_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;//the number of time units that pass in one second.
  box.m_duration = m_duration;
  if (!m_used_version) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMediaHeaderSize;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMediaHeader64Size;
  }
}

void AMMuxerMp4Builder::fill_media_header_box_for_audio_track()
{
  SISOMMediaHeaderBox& box = m_movie_box.m_audio_track.m_media.m_media_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = m_duration;
  if (!m_used_version) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMediaHeaderSize;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstMediaHeader64Size;
  }
}

void AMMuxerMp4Builder::fill_video_hander_reference_box()
{
  SISOMHandlerReferenceBox& box =
      m_movie_box.m_video_track.m_media.m_media_handler;
  box.m_base_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_handler_type = DISOM_VIDEO_HANDLER_REFERENCE_TAG;
  box.m_name = m_video_compressor_name;
  if (box.m_name) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstHandlerReferenceSize +
        strlen(box.m_name) + 1;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstHandlerReferenceSize + 1;
  }
}

void AMMuxerMp4Builder::fill_audio_hander_reference_box()
{
  SISOMHandlerReferenceBox& box =
      m_movie_box.m_audio_track.m_media.m_media_handler;
  box.m_base_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_handler_type = DISOM_AUDIO_HANDLER_REFERENCE_TAG;
  box.m_name = m_audio_compressor_name;
  if (box.m_name) {
    box.m_base_full_box.m_base_box.m_size = DISOMConstHandlerReferenceSize +
        strlen(box.m_name) + 1;
  } else {
    box.m_base_full_box.m_base_box.m_size = DISOMConstHandlerReferenceSize + 1;
  }
}

void AMMuxerMp4Builder::fill_video_chunk_offset_box()
{
  SISOMChunkOffsetBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_base_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
}

void AMMuxerMp4Builder::fill_audio_chunk_offset_box()
{
  SISOMChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_base_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
}

void AMMuxerMp4Builder::fill_video_sync_sample_box()
{
  SISOMSyncSampleTableBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_sync_sample;
  box.m_base_full_box.m_base_box.m_type = DISOM_SYNC_SAMPLE_TABLE_BOX_TAG;
  box.m_base_full_box.m_flags = m_flags;
  box.m_base_full_box.m_version = m_used_version;
}

AM_STATE AMMuxerMp4Builder::fill_video_sample_to_chunk_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMSampleToChunkBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  do {
    box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_base_full_box.m_version = m_used_version;
    box.m_base_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new _SSampleToChunkEntry[box.m_entry_count];
    }
    if (AM_UNLIKELY(!box.m_entrys)) {
      ERROR("fill_sample_to_chunk_box: no memory\n");
      ret = AM_STATE_ERROR;
      break;
    }
    box.m_entrys[0].m_first_entry = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_audio_sample_to_chunk_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMSampleToChunkBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  do {
    box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_base_full_box.m_version = m_used_version;
    box.m_base_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new _SSampleToChunkEntry[box.m_entry_count];
    }
    if (AM_UNLIKELY(!box.m_entrys)) {
      ERROR("fill_sample_to_chunk_box: no memory\n");
      ret = AM_STATE_ERROR;
      break;
    }
    box.m_entrys[0].m_first_entry = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

void AMMuxerMp4Builder::fill_video_sample_size_box()
{
  SISOMSampleSizeBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
}

void AMMuxerMp4Builder::fill_audio_sample_size_box()
{
  SISOMSampleSizeBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
}

void AMMuxerMp4Builder::fill_avc_decoder_configuration_record_box()
{
  SISOMAVCDecoderConfigurationRecord& box =
      m_movie_box.m_video_track.m_media.m_media_info.
      m_sample_table.m_visual_sample_description.m_visual_entry.m_avc_config;
  box.m_base_box.m_type = DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG;
  box.m_configurationVersion = 1;
  box.m_AVCProfileIndication = m_avc_profile_indicator;
  box.m_profile_compatibility = 0;
  box.m_AVCLevelIndication = m_avc_level_indicator;
  box.m_reserved = 0xff;
  box.m_lengthSizeMinusOne = 3;
  box.m_reserved_1 = 0xff;
  box.m_numOfSequenceParameterSets = 1;
  box.m_sequenceParametersSetLength = m_avc_sps_length;
  box.m_p_sps = m_avc_sps;
  box.m_numOfPictureParameterSets = 1;
  box.m_pictureParametersSetLength = m_avc_pps_length;
  box.m_p_pps = m_avc_pps;
  box.m_base_box.m_size = DISOMConstAVCDecoderConfiurationRecordSize_FixPart +
      box.m_sequenceParametersSetLength + box.m_pictureParametersSetLength;
}

void AMMuxerMp4Builder::fill_visual_sample_description_box()
{
  SISOMVisualSampleDescriptionBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_visual_sample_description;
  box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  if (m_video_codec_name) {
    memcpy(&box.m_visual_entry.m_base_box.m_type, m_video_codec_name,
           DISOM_TAG_SIZE);
  } else {
    NOTICE("not specify video codec name,use avc1 as default.\n");
    memcpy(&box.m_visual_entry.m_base_box.m_type, "avc1", DISOM_TAG_SIZE);
  }
  box.m_visual_entry.m_data_reference_index = 1;
  box.m_visual_entry.m_width = m_video_info.width;
  box.m_visual_entry.m_height = m_video_info.height;
  box.m_visual_entry.m_horizresolution = 0x00480000;
  box.m_visual_entry.m_vertresolution = 0x00480000;
  box.m_visual_entry.m_frame_count = 1;
  strcpy(box.m_visual_entry.m_compressorname, m_video_compressor_name);
  box.m_visual_entry.m_depth = 0x0018;
  box.m_visual_entry.m_pre_defined_2 = -1;
  fill_avc_decoder_configuration_record_box();
  box.m_visual_entry.m_base_box.m_size = DISOMConstVisualSampleEntrySize_FixPart
      + box.m_visual_entry.m_avc_config.m_base_box.m_size;
  box.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t)
      + box.m_visual_entry.m_base_box.m_size;
}

void AMMuxerMp4Builder::fill_aac_elementary_stream_description_box()
{
  SISOMAACElementaryStreamDescriptorBox& box = m_movie_box.m_audio_track.\
      m_media.m_media_info.m_sample_table.m_audio_sample_description.\
      m_audio_entry.m_esd;
  box.m_base_full_box.m_base_box.m_type =
      DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  /*ES descriptor*/
  box.m_es_descriptor_type_tag = 3;
  box.m_es_descriptor_type_length = 34;
  box.m_es_id = 0;
  box.m_stream_priority = 0;
  /*decoder config descriptor*/
  box.m_decoder_config_descriptor_type_tag = 4;
  box.m_decoder_descriptor_type_length = 22;
  box.m_object_type = 0x40;//MPEG-4 AAC = 64;
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
  box.m_base_full_box.m_base_box.m_size =
      DISOMConstAACElementaryStreamDescriptorSize_FixPart +
      m_audio_extradata_length;
}

void AMMuxerMp4Builder::fill_audio_sample_description_box()
{
  SISOMAudioSampleDescriptionBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_audio_sample_description;
  box.m_base_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  if (m_audio_codec_name) {
    memcpy(&box.m_audio_entry.m_base_box.m_type, m_audio_codec_name,
           DISOM_TAG_SIZE);
  } else {
    NOTICE("not specify audio codec name, use mp4a as default.\n");
    memcpy(&box.m_audio_entry.m_base_box.m_type, "mp4a", DISOM_TAG_SIZE);
  }
  box.m_audio_entry.m_data_reference_index = 1;
  box.m_audio_entry.m_channelcount = m_audio_info.channels;
  box.m_audio_entry.m_samplesize = m_audio_info.sample_size * 8;//in bits
  box.m_audio_entry.m_samplerate = m_audio_info.sample_rate << 16;
  box.m_audio_entry.m_pre_defined = 0xfffe;
  fill_aac_elementary_stream_description_box();
  box.m_audio_entry.m_base_box.m_size = DISOMConstAudioSampleEntrySize +
      box.m_audio_entry.m_esd.m_base_full_box.m_base_box.m_size;
  box.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
      sizeof(uint32_t) + box.m_audio_entry.m_base_box.m_size;
}

AM_STATE AMMuxerMp4Builder::fill_video_decoding_time_to_sample_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMDecodingTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_base_full_box.m_base_box.m_type =
    DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_base_full_box.m_flags = m_flags;
    box.m_base_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_video_composition_time_to_sample_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMCompositionTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_ctts;
  do{
    box.m_base_full_box.m_base_box.m_type =
           DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_base_full_box.m_flags = m_flags;
    box.m_base_full_box.m_version = m_used_version;
  }while(0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_audio_decoding_time_to_sample_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMDecodingTimeToSampleBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_base_full_box.m_base_box.m_type =
        DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    if (box.m_p_entry && box.m_entry_buf_count >= 1) {
      box.m_entry_count = 1;
      box.m_p_entry[0].sample_count = m_audio_frame_number;
      box.m_p_entry[0].sample_delta = m_audio_info.pkt_pts_increment /
          m_audio_pkt_frame_number;
    } else {
      delete[] box.m_p_entry;
      box.m_entry_buf_count = 1;
      box.m_p_entry = new _STimeEntry[box.m_entry_buf_count];
      if (box.m_p_entry) {
        box.m_entry_count = 1;
        box.m_p_entry[0].sample_count = m_audio_frame_number;
        box.m_p_entry[0].sample_delta = m_audio_info.pkt_pts_increment /
            m_audio_pkt_frame_number;
      } else {
        ERROR("no memory\n");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_video_sample_table_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMSampleTableBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table;
  do {
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    fill_visual_sample_description_box();
    if(AM_UNLIKELY(fill_video_decoding_time_to_sample_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill video decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(fill_video_composition_time_to_sample_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill video composition time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_video_sample_size_box();
    if(AM_UNLIKELY(fill_video_sample_to_chunk_box()
                    != AM_STATE_OK)) {
      ERROR("Failed to fill sample to chunk box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_video_chunk_offset_box();
    fill_video_sync_sample_box();
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_sound_sample_table_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMSampleTableBox& box = m_movie_box.m_audio_track.m_media.\
        m_media_info.m_sample_table;
  do {
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    fill_audio_sample_description_box();
    if(AM_UNLIKELY(fill_audio_decoding_time_to_sample_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill audio decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_audio_sample_size_box();
    if(AM_UNLIKELY(fill_audio_sample_to_chunk_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill sample to chunk box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_audio_chunk_offset_box();
  } while (0);
  return ret;
}

void AMMuxerMp4Builder::fill_video_data_reference_box()
{
  SISOMDataReferenceBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_base_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_base_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_base_full_box.m_version = m_used_version;
  box.m_url.m_base_full_box.m_flags = 1;
  box.m_url.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_base_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMuxerMp4Builder::fill_audio_data_reference_box()
{
  SISOMDataReferenceBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_base_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_base_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_base_full_box.m_version = m_used_version;
  box.m_url.m_base_full_box.m_flags = 1;
  box.m_url.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_base_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMuxerMp4Builder::fill_video_data_information_box()
{
  SISOMDataInformationBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_video_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_base_full_box.m_base_box.m_size +
      DISOM_BOX_SIZE;
}

void AMMuxerMp4Builder::fill_audio_data_information_box()
{
  SISOMDataInformationBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_audio_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_base_full_box.m_base_box.m_size +
      DISOM_BOX_SIZE;
}

void AMMuxerMp4Builder::fill_video_media_information_header_box()
{
  SISOMVideoMediaInformationHeaderBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_video_information_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_VIDEO_MEDIA_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_graphicsmode = 0;
  box.m_opcolor[0] = box.m_opcolor[1] = box.m_opcolor[2] = 0;
  box.m_base_full_box.m_base_box.m_size = DISOMConstVideoMediaHeaderSize;
}

void AMMuxerMp4Builder::fill_sound_media_information_header_box()
{
  SISOMSoundMediaInformationHeaderBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sound_information_header;
  box.m_base_full_box.m_base_box.m_type = DISOM_SOUND_MEDIA_HEADER_BOX_TAG;
  box.m_base_full_box.m_version = m_used_version;
  box.m_base_full_box.m_flags = m_flags;
  box.m_balanse = 0;
  box.m_base_full_box.m_base_box.m_size = DISOMConstSoundMediaHeaderSize;
}

AM_STATE AMMuxerMp4Builder::fill_video_media_information_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMMediaInformationBox& box = m_movie_box.m_video_track.m_media.m_media_info;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    fill_video_media_information_header_box();
    fill_video_data_information_box();
    fill_video_sample_table_box();
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_audio_media_information_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMMediaInformationBox& box = m_movie_box.m_audio_track.m_media.m_media_info;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    fill_sound_media_information_header_box();
    fill_audio_data_information_box();
    if(AM_UNLIKELY(fill_sound_sample_table_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill sound sample table box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_video_media_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMMediaBox& box = m_movie_box.m_video_track.m_media;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_video_track();
    box.m_media_header.m_duration = m_duration;
    fill_video_hander_reference_box();
    if(AM_UNLIKELY(fill_video_media_information_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill video media information box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_audio_media_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMMediaBox& box = m_movie_box.m_audio_track.m_media;
  do {
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_audio_track();
    box.m_media_header.m_duration = m_audio_duration;
    fill_audio_hander_reference_box();
    if(AM_UNLIKELY(fill_audio_media_information_box()
                   != AM_STATE_OK)) {
      ERROR("Failed to fill audio media information box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_video_movie_track_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMTrackBox& box = m_movie_box.m_video_track;
  do {
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_movie_video_track_header_box();
    if(AM_UNLIKELY(fill_video_media_box() != AM_STATE_OK)) {
      ERROR("Failed to fill movie video track box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_audio_movie_track_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMTrackBox& box = m_movie_box.m_audio_track;
  do {
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_movie_audio_track_header_box();
    if(AM_UNLIKELY(fill_audio_media_box() != AM_STATE_OK)) {
      ERROR("Failed to fill movie audio track box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::fill_movie_box()
{
  AM_STATE ret = AM_STATE_OK;
  SISOMMovieBox& box = m_movie_box;
  do {
    box.m_base_box.m_type = DISOM_MOVIE_BOX_TAG;
    fill_movie_header_box();
    box.m_base_box.m_size = sizeof(SISOMBaseBox)
        + box.m_movie_header_box.m_base_full_box.m_base_box.m_size;
    if (AM_UNLIKELY(fill_video_movie_track_box()
        != AM_STATE_OK)) {
      ERROR("Failed to fill movie video track box.");
      ret = AM_STATE_ERROR;
      break;
    }
    box.m_base_box.m_size += box.m_video_track.m_base_box.m_size;
    if (AM_UNLIKELY(fill_audio_movie_track_box()
        != AM_STATE_OK)) {
      ERROR("Failed to fill movie audio track box.");
      ret = AM_STATE_ERROR;
      break;
    }
    box.m_base_box.m_size += box.m_audio_track.m_base_box.m_size;
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_video_composition_time_to_sample_box(int64_t
                                                          delta_pts)
{
  AM_STATE ret = AM_STATE_OK;
  SISOMCompositionTimeToSampleBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_ctts;
  do{
    int32_t ctts = 0;
    if(AM_UNLIKELY(m_video_frame_number == 1)) {
      ctts = 0;
    } else {
      ctts = m_last_ctts + (delta_pts - (90000 * m_video_info.rate /
          m_video_info.scale));
      if(AM_UNLIKELY(ctts < 0)) {
        for(int32_t i = m_video_frame_number - 2; i >= 0; --i) {
          box.m_p_entry[i].sample_delta =
              box.m_p_entry[i].sample_delta - ctts;
        }
        ctts = 0;
      }
    }
    if(m_video_frame_number >= box.m_entry_buf_count) {
      _STimeEntry* tmp = new _STimeEntry[box.m_entry_buf_count +
                                         DVIDEO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (box.m_entry_buf_count > 0) {
        memcpy(tmp, box.m_p_entry, box.m_entry_count * sizeof(_STimeEntry));
        delete[] box.m_p_entry;
      }
      box.m_p_entry = tmp;
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
    }
    if(m_video_frame_number > 0) {
      box.m_p_entry[m_video_frame_number - 1].sample_count = 1;
      box.m_p_entry[m_video_frame_number - 1].sample_delta = ctts;
    }
    ++ box.m_entry_count;
    m_last_ctts = ctts;
  }while(0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_video_decoding_time_to_sample_box(uint32_t delta_pts)
{
  AM_STATE ret = AM_STATE_OK;
  SISOMDecodingTimeToSampleBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
  do{
    if(m_video_frame_number >= box.m_entry_buf_count) {
      _STimeEntry* tmp = new _STimeEntry[box.m_entry_buf_count +
                                         DVIDEO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (box.m_entry_buf_count > 0) {
        memcpy(tmp, box.m_p_entry, box.m_entry_count * sizeof(_STimeEntry));
        delete[] box.m_p_entry;
      }
      box.m_p_entry = tmp;
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
    }
    if(m_video_frame_number > 0) {
      box.m_p_entry[m_video_frame_number - 1].sample_count = 1;
      box.m_p_entry[m_video_frame_number - 1].sample_delta = delta_pts;
    }
    ++ box.m_entry_count;
  }while(0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_video_chunk_offset_box(uint64_t offset)
{
  AM_STATE ret = AM_STATE_OK;
  SISOMChunkOffsetBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
             m_sample_table.m_chunk_offset;
  DEBUG("insert video chunk offset box, offset = %llu", offset);
  if (box.m_entry_count < box.m_entry_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
    box.m_entry_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_entry_buf_count +
                                 DVIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_buf_count && box.m_chunk_offset) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_buf_count *
               sizeof(uint32_t));
        delete[] box.m_chunk_offset;
      }
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_audio_chunk_offset_box(uint64_t offset)
{
  AM_STATE ret = AM_STATE_OK;
  SISOMChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_chunk_offset;
  DEBUG("insert audio chunk offset box, offset = %llu, frame count = %u",
         offset, box.m_entry_count);
  if (box.m_entry_count < box.m_entry_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
    box.m_entry_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_entry_buf_count +
                                  DAUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_buf_count && box.m_chunk_offset) {
        memcpy(ptmp, box.m_chunk_offset,
               box.m_entry_buf_count * sizeof(uint32_t));
        delete[] box.m_chunk_offset;
      }
      box.m_entry_buf_count += DAUDIO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = (uint32_t) offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_video_sample_size_box(uint32_t size)
{
  SISOMSampleSizeBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
      m_sample_table.m_sample_size;
  DEBUG("insert video sample size box, size = %u", size);
  AM_STATE ret = AM_STATE_OK;
  if (box.m_sample_count < box.m_entry_buf_count) {
    box.m_entry_size[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_entry_buf_count +
                                    DVIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_buf_count && box.m_entry_size) {
        memcpy(ptmp, box.m_entry_size,
               box.m_entry_buf_count * sizeof(uint32_t));
        delete[] box.m_entry_size;
      }
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
      box.m_entry_size = ptmp;
      box.m_entry_size[box.m_sample_count] = (uint32_t) size;
      box.m_sample_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_audio_sample_size_box(uint32_t size)
{
  SISOMSampleSizeBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_size;
  DEBUG("insert audio sample size box, size = %u", size);
  AM_STATE ret = AM_STATE_OK;
  if (box.m_sample_count < box.m_entry_buf_count) {
    box.m_entry_size[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_entry_buf_count +
                                  DAUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_buf_count && box.m_entry_size) {
        memcpy(ptmp, box.m_entry_size, box.m_entry_buf_count *
               sizeof(uint32_t));
        delete[] box.m_entry_size;
      }
      box.m_entry_buf_count += DAUDIO_FRAME_COUNT;
      box.m_entry_size = ptmp;
      box.m_entry_size[box.m_sample_count] = (uint32_t) size;
      box.m_sample_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMMuxerMp4Builder::insert_video_sync_sample_box(
    uint32_t video_frame_number)
{
  AM_STATE ret = AM_STATE_OK;
  SISOMSyncSampleTableBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sync_sample;
  do {
    if (box.m_stss_count + 1 >= box.m_stss_buf_count) {
      uint32_t *tmp = new uint32_t[box.m_stss_buf_count + DVIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("malloc memory error.");
        ret = AM_STATE_NO_MEMORY;
        break;
      }
      if(box.m_stss_buf_count > 0) {
        memcpy(tmp, box.m_sync_sample_table,
               box.m_stss_count * sizeof(uint32_t));
        delete[] box.m_sync_sample_table;
      }
      box.m_sync_sample_table = tmp;
      box.m_stss_buf_count += DVIDEO_FRAME_COUNT;
    }
    box.m_sync_sample_table[box.m_stss_count] = video_frame_number;
    ++ box.m_stss_count;
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::write_video_frame_info(int64_t delta_pts,
                          uint64_t offset, uint64_t size, uint8_t sync_sample)
{
  AM_STATE ret = AM_STATE_OK;
  DEBUG("write video frame info, offset = %llu, size = %llu", offset, size);
  do {
    if(AM_UNLIKELY(insert_video_composition_time_to_sample_box(delta_pts)
                   != AM_STATE_OK)) {
      ERROR("Failed to insert video composition time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(insert_video_decoding_time_to_sample_box((uint32_t)delta_pts)
                   != AM_STATE_OK)) {
      ERROR("Failed to insert video decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(insert_video_chunk_offset_box(offset) != AM_STATE_OK)) {
      ERROR("Failed to insert video chunk offset box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(insert_video_sample_size_box(size) != AM_STATE_OK)) {
      ERROR("Failed to insert video sample size box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(sync_sample) {
      if(AM_UNLIKELY(insert_video_sync_sample_box(m_video_frame_number)
                     != AM_STATE_OK)) {
        ERROR("Failed to insert video sync sample box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::write_audio_frame_info(uint64_t offset,
                                                   uint64_t size)
{
  AM_STATE ret = AM_STATE_OK;
  DEBUG("write audio frame info, offset = %llu, size = %llu", offset, size);
  do {
    if (AM_UNLIKELY(insert_audio_chunk_offset_box(offset) != AM_STATE_OK)) {
      ERROR("Failed to insert audio chunk offset box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(insert_audio_sample_size_box(size) != AM_STATE_OK)) {
      ERROR("Failed to insert audio sample size box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::update_media_box_size()
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t offset = m_file_type_box.m_base_box.m_size;
  uint32_t size = 0;
  do {
    if(m_hls_enable) {
      offset += MP4_MOOV_BOX_PRE_LENGTH;
    }
    m_media_data_box.m_base_box.m_size += (uint32_t) m_overall_media_data_len;
    if (AM_UNLIKELY(m_file_writer->seek_data(offset, SEEK_SET)
                    != AM_STATE_OK)) {
      ERROR("Failed to seek data.");
      ret = AM_STATE_ERROR;
      break;
    }
    size = htonl(m_media_data_box.m_base_box.m_size);
    if (AM_UNLIKELY(m_file_writer->write_data_direct((uint8_t*)(&size), 4)
                    != AM_STATE_OK)) {
      ERROR("Failed to write data to file");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::begin_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
//    m_creation_time = (uint64_t)(time(NULL) + ((1970 - 1904) * 365 + 17)
//                                                 * 24 * 60 * 60);
    fill_file_type_box();
    fill_media_data_box();
    if (AM_UNLIKELY(m_file_type_box.write_file_type_box(m_file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write file type box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(m_hls_enable) {
      uint8_t buf[512] = { 0 };
      for(uint32_t i = 0; i < (MP4_MOOV_BOX_PRE_LENGTH / 512); ++ i) {
        if(m_file_writer->write_data(buf, 512) != AM_STATE_OK) {
          ERROR("Failed to write data to mp4 file.");
          ret = AM_STATE_ERROR;
          break;
        }
      }
      uint32_t size = MP4_MOOV_BOX_PRE_LENGTH % 512;
      if(m_file_writer->write_data(buf, size) != AM_STATE_OK) {
        ERROR("Failed to write data to mp4 file.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(m_media_data_box.write_media_data_box(m_file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write media data box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerMp4Builder::end_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    m_duration = m_video_duration;
    if (AM_LIKELY(m_write_media_data_started)) {
      m_write_media_data_started = 0;
      if (AM_UNLIKELY(fill_movie_box() != AM_STATE_OK)) {
        ERROR("Failed to fill movie box.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_movie_box.update_movie_box_size();
      if(m_hls_enable) {
        if(m_file_writer->seek_data(m_file_type_box.m_base_box.m_size, SEEK_SET)
            != AM_STATE_OK) {
          ERROR("Failed to seek data");
          ret = AM_STATE_ERROR;
          break;
        }
        if(!m_file_writer->set_file_offset(m_file_type_box.m_base_box.m_size,
                                           SEEK_SET)) {
          ERROR("Failed to set file offset.");
          ret = AM_STATE_ERROR;
          break;
        }
        uint32_t free_box_length = MP4_MOOV_BOX_PRE_LENGTH -
            m_movie_box.m_base_box.m_size;
        NOTICE("MP4_MOOV_BOX_PRE_LENGTH is %u", MP4_MOOV_BOX_PRE_LENGTH);
        NOTICE("m_movie box length is %u", m_movie_box.m_base_box.m_size);
        NOTICE("free box length is %u", free_box_length);
        fill_free_box(free_box_length);
        m_free_box.write_free_box(m_file_writer);
        if(m_file_writer->seek_data(m_free_box.m_base_box.m_size - 8, SEEK_CUR)
            != AM_STATE_OK) {
          ERROR("Failed to seek data.");
          ret = AM_STATE_ERROR;
          break;
        }
        if(!m_file_writer->set_file_offset(m_free_box.m_base_box.m_size - 8,
                                           SEEK_CUR)) {
          ERROR("Failed to set file offset.");
          break;
        }
      }
      if (AM_UNLIKELY(m_movie_box.write_movie_box(m_file_writer)
                      != AM_STATE_OK)) {
        ERROR("Failed to write tail.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(update_media_box_size() != AM_STATE_OK)) {
        ERROR("Failed to end file");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  if (AM_UNLIKELY(m_file_writer->deinit() != AM_STATE_OK)) {
    ERROR("Failed to deinit file.");
    ret = AM_STATE_ERROR;
  }
  m_write_media_data_started = 0;
  m_overall_media_data_len = 0;
  m_audio_pkt_frame_number = 1;
  m_video_duration = 0;
  m_audio_duration = 0;
  m_duration = 0xffffffffffffffff;
  m_last_pts = 0;
  m_last_ctts = 0;
  m_video_frame_number = 0;
  m_audio_frame_number = 0;
  m_audio_frame_count_per_packet = 0;
  return ret;
}

/*AM_STATE AMMuxerMp4Builder::get_avc_sps_pps(uint8_t *pdata, uint32_t len)
{
  AM_STATE ret = AM_STATE_OK;
  uint8_t* p_sps = NULL, *p_pps = NULL;
  uint32_t sps_size = 0, pps_size = 0;
  do {
    if (AM_UNLIKELY(_get_avc_sps_pps(pdata, len, p_sps, sps_size,
                                     p_pps, pps_size) != AM_STATE_OK)) {
      ERROR("Failed to get avc sps pps.");
      ret = AM_STATE_ERROR;
      break;
    }
    SCodecAVCExtraData extradata;
    if (AM_UNLIKELY(_try_parse_avc_sps_pps(p_sps + 5, sps_size - 5, &extradata)
        != AM_STATE_OK)) {
      ERROR("Failed to parse avc sps pps.");
      ret = AM_STATE_ERROR;
      break;
    }
    m_video_width = extradata.sps.mb_width * 16;
    m_video_height = extradata.sps.mb_height * 16;

    m_avc_sps_length = sps_size - 4;
    if (m_avc_sps) {
      free(m_avc_sps);
      m_avc_sps = NULL;
    }
    m_avc_sps = (uint8_t*) malloc(m_avc_sps_length);
    if (m_avc_sps) {
      memcpy(m_avc_sps, p_sps + 4, m_avc_sps_length);
    }

    m_avc_pps_length = pps_size - 4;
    if (m_avc_pps) {
      free(m_avc_pps);
      m_avc_pps = NULL;
    }
    m_avc_pps = (uint8_t*) malloc(m_avc_pps_length);
    if (m_avc_pps) {
      memcpy(m_avc_pps, p_pps + 4, m_avc_pps_length);
    }
  } while (0);
  return ret;
}
*/



