/*******************************************************************************
 * am_muxer_mp4_builder.h
 *
 * History:
 *   2014-12-02  [Zhi He] created file
 *
 * Copyright (C) 2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#ifndef AM_MUXER_MP4_BUILDER_H_
#define AM_MUXER_MP4_BUILDER_H_
#include "am_muxer_codec_info.h"
#include "iso_base_media_file_defs.h"
#include "am_video_types.h"
#include "am_audio_define.h"
#include "adts.h"
#include "h264.h"
#include <vector>

#define DVIDEO_FRAME_COUNT 108000
#define DAUDIO_FRAME_COUNT 169000
#define DVIDEO_COMPRESSORNAME "Ambarella Smart AVC"
#define DAUDIO_COMPRESSORNAME "Ambarella AAC"

enum
{
  NAL_UNSPECIFIED = 0,
  NAL_NON_IDR,
  NAL_IDR = 5,
  NAL_SEI,
  NAL_SPS,
  NAL_PPS,
  NAL_AUD,
};

class AMMp4FileWriter;
class AMMuxerMp4Builder
{
    typedef std::vector<AM_H264_NALU> AMNaluList;
    typedef std::vector<ADTS>         AMAdtsList;
  public:
    static AMMuxerMp4Builder* create(AMMp4FileWriter* file_writer,
                                     uint64_t file_time_length,
                                     uint32_t hls_enable);

  private:
    AMMuxerMp4Builder();
    AM_STATE init(AMMp4FileWriter* file_writer, uint64_t file_time_length,
                  uint32_t hls_enable);
    virtual ~AMMuxerMp4Builder();

  public:
    AM_STATE set_video_info(AM_VIDEO_INFO* video_info);
    AM_STATE set_audio_info(AM_AUDIO_INFO* audio_info);
    AM_STATE begin_file();
    AM_STATE end_file();
    virtual AM_STATE write_video_data(AMPacket *packet, AMPacket *usr_sei);
    virtual AM_STATE write_audio_data(uint8_t *pdata, uint32_t len,
                                      uint32_t frame_count);
    virtual void destroy();
    virtual AM_MUXER_CODEC_TYPE get_muxer_type();


  private:
    AM_STATE update_media_box_size();
    //virtual AM_STATE set_specified_info(AMMuxerCodecSpecifiedInfo *info);

    //virtual AM_STATE set_video_extradata(uint8_t *p_data, uint32_t data_size);
    //virtual AM_STATE set_audio_extradata(uint8_t *p_data, uint32_t data_size);

  private:
    void default_mp4_file_setting();
    bool find_nalu(uint8_t *data, uint32_t len, H264_NALU_TYPE expect);
    void find_adts(uint8_t *bs);
    void feed_stream_data(uint8_t* input_buf, uint32_t input_size,
                          uint32_t frame_count);
    bool get_one_frame(uint8_t **frame_start, uint32_t* frame_size);

  private:
    void fill_file_type_box();
    void fill_free_box(uint32_t size);
    void fill_media_data_box();
    void fill_movie_header_box();
    void fill_movie_video_track_header_box();
    void fill_movie_audio_track_header_box();
    /*This media header box contained in the "mdia" type box.*/
    void fill_media_header_box_for_video_track();
    void fill_media_header_box_for_audio_track();
    void fill_video_hander_reference_box();
    void fill_audio_hander_reference_box();
    void fill_video_chunk_offset_box();
    void fill_audio_chunk_offset_box();
    void fill_video_sync_sample_box();
    AM_STATE fill_video_sample_to_chunk_box();
    AM_STATE fill_audio_sample_to_chunk_box();
    void fill_video_sample_size_box();
    void fill_audio_sample_size_box();
    void fill_avc_decoder_configuration_record_box();
    void fill_visual_sample_description_box();
    void fill_aac_elementary_stream_description_box();
    void fill_audio_sample_description_box();
    AM_STATE fill_video_decoding_time_to_sample_box();
    AM_STATE fill_video_composition_time_to_sample_box();
    AM_STATE fill_audio_decoding_time_to_sample_box();
    AM_STATE fill_video_sample_table_box();
    AM_STATE fill_sound_sample_table_box();
    void fill_video_data_reference_box();
    void fill_audio_data_reference_box();
    void fill_video_data_information_box();
    void fill_audio_data_information_box();
    void fill_video_media_information_header_box();
    void fill_sound_media_information_header_box();
    AM_STATE fill_video_media_information_box();
    AM_STATE fill_audio_media_information_box();
    AM_STATE fill_video_media_box();
    AM_STATE fill_audio_media_box();
    AM_STATE fill_video_movie_track_box();
    AM_STATE fill_audio_movie_track_box();
    AM_STATE fill_movie_box();

  private:
    AM_STATE insert_video_composition_time_to_sample_box(int64_t delta_pts);
    AM_STATE insert_video_decoding_time_to_sample_box(uint32_t delta_pts);
    AM_STATE insert_video_chunk_offset_box(uint64_t offset);
    AM_STATE insert_audio_chunk_offset_box(uint64_t offset);
    AM_STATE insert_video_sample_size_box(uint32_t size);
    AM_STATE insert_audio_sample_size_box(uint32_t size);
    AM_STATE insert_video_sync_sample_box(uint32_t video_frame_number);
    AM_STATE write_video_frame_info(int64_t delta_pts, uint64_t offset,
                                    uint64_t size, uint8_t sync_sample);
    AM_STATE write_audio_frame_info(uint64_t offset, uint64_t size);

  private:
    //AM_STATE get_avc_sps_pps(uint8_t *pdata, uint32_t len);

  private:
    SISOMFileTypeBox  m_file_type_box;
    SISOMFreeBox      m_free_box;
    SISOMMediaDataBox m_media_data_box;
    SISOMMovieBox     m_movie_box;

  private:
    uint64_t m_overall_media_data_len;
    uint32_t m_audio_pkt_frame_number;

    uint64_t m_video_duration;
    uint64_t m_audio_duration;

    uint64_t m_creation_time;
    uint64_t m_modification_time;
    uint64_t m_duration;
    int64_t  m_last_pts;
    int32_t  m_last_ctts;

    uint32_t m_video_frame_number;
    uint32_t m_audio_frame_number;
    uint32_t m_audio_frame_count_per_packet;
    uint64_t m_file_time_length;
    uint32_t m_hls_enable;

    uint32_t m_rate; //default setting
    uint16_t m_volume; //default setting

    uint32_t m_matrix[9];

    uint8_t m_used_version; //default setting

    uint8_t m_video_track_id; //default setting
    uint8_t m_audio_track_id; //default setting
    uint8_t m_next_track_id;  //default setting

    uint32_t m_flags; //default setting

    char m_video_codec_name[4];
    char m_audio_codec_name[4];

    uint8_t *m_avc_sps;
    uint32_t m_avc_sps_length;

    uint8_t *m_avc_pps;
    uint32_t m_avc_pps_length;

    uint8_t m_avc_profile_indicator;//default setting
    uint8_t m_avc_level_indicator;  //default setting
    uint8_t m_write_media_data_started;

    uint32_t m_audio_max_bitrate; //default setting
    uint32_t m_audio_avg_bitrate; //default setting

    uint8_t *m_audio_extradata;
    uint32_t m_audio_extradata_length;
    uint32_t m_audio_extradata_buffer_length;
    uint16_t m_aac_spec_config;

    char m_video_compressor_name[32];
    char m_audio_compressor_name[32];

    AMMp4FileWriter* m_file_writer;
    AM_AUDIO_INFO m_audio_info;
    AM_VIDEO_INFO m_video_info;
    AMAdtsList m_adts;
    uint32_t m_curr_adts_index;
    AMNaluList m_nalu_list;
};

#endif


