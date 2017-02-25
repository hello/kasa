/*******************************************************************************
 * am_mp4_fixer.h
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
#ifndef AM_MP4_FIXER_H_
#define AM_MP4_FIXER_H_

#include "am_audio_define.h"
#include "am_video_types.h"
#include "h265.h"
#include "h264.h"
#include "am_mp4_file_fixer_if.h"

#define VIDEO_FRAME_COUNT 20000
#define AUDIO_FRAME_COUNT 10000
#define GPS_FRAME_COUNT   1000
#define VIDEO_H264_COMPRESSORNAME "Ambarella Smart AVC"
#define VIDEO_H265_COMPRESSORNAME "Ambarella Smart HEVC"
#define AUDIO_AAC_COMPRESSORNAME  "Ambarella AAC"
#define AUDIO_OPUS_COMPRESSORNAME "Xiph.Org Opus"
#define GPS_COMPRESSORNAME        "Global Positioning System"

#define INDEX_TAG_CREATE(X,Y,Z,N) X | (Y << 8) | (Z << 16) | (N << 24)
enum IndexTag{
  VIDEO_INFO  = INDEX_TAG_CREATE('v', 'i', 'n', 'f'),
  AUDIO_INFO  = INDEX_TAG_CREATE('a', 'i', 'n', 'f'),
  VIDEO_VPS   = INDEX_TAG_CREATE('v', 'p', 's', ':'),
  VIDEO_SPS   = INDEX_TAG_CREATE('s', 'p', 's', ':'),
  VIDEO_PPS   = INDEX_TAG_CREATE('p', 'p', 's', ':'),
  VIDEO_INDEX = INDEX_TAG_CREATE('v', 'i', 'n', 'd'),
  AUDIO_INDEX = INDEX_TAG_CREATE('a', 'i', 'n', 'd'),
  GPS_INDEX   = INDEX_TAG_CREATE('g', 'i', 'n', 'd'),
};

class AMMp4Fixer : public AMIMp4FileFixer
{
  public :
    static AMMp4Fixer* create(const char* index_file_name = nullptr,
                              const char* damaged_mp4_name = nullptr,
                              const char* fixed_mp4_name = nullptr);
  public :
    virtual bool fix_mp4_file();
    virtual void destroy();
  private :
    virtual ~AMMp4Fixer();
    AMMp4Fixer();
    bool init(const char* index_file_name, const char* damaged_mp4_name,
              const char* fixed_mp4_name);
    bool parse_index_file();
    bool build_mp4_header();
    bool set_proper_damaged_mp4_name();
    bool set_proper_fixed_mp4_name();
    bool write_video_data_to_file();
    bool write_audio_data_to_file();
    bool write_gps_data_to_file();
  private :
    void fill_file_type_box();
    void fill_media_data_box();
    void fill_movie_header_box();
    void fill_object_desc_box();
    void fill_video_track_header_box();
    void fill_audio_track_header_box();
    void fill_gps_track_header_box();
    void fill_media_header_box_for_video_track();
    void fill_media_header_box_for_audio_track();
    void fill_media_header_box_for_gps_track();
    bool fill_video_handler_reference_box();
    bool fill_audio_handler_reference_box();
    bool fill_gps_handler_reference_box();
    void fill_video_chunk_offset_box();
    void fill_video_chunk_offset64_box();
    void fill_audio_chunk_offset_box();
    void fill_audio_chunk_offset64_box();
    void fill_gps_chunk_offset_box();
    void fill_gps_chunk_offset64_box();
    void fill_video_sync_sample_box();
    bool fill_video_sample_to_chunk_box();
    bool fill_audio_sample_to_chunk_box();
    bool fill_gps_sample_to_chunk_box();
    void fill_video_sample_size_box();
    void fill_audio_sample_size_box();
    void fill_gps_sample_size_box();
    bool fill_avc_decoder_configuration_box();
    bool fill_hevc_decoder_configuration_box();
    bool fill_visual_sample_description_box();
    void fill_aac_description_box();
    bool fill_opus_description_box();
    bool fill_audio_sample_description_box();
    bool fill_gps_sample_description_box();
    bool fill_video_decoding_time_to_sample_box();
    bool fill_video_composition_time_to_sample_box();
    bool fill_audio_decoding_time_to_sample_box();
    bool fill_gps_decoding_time_to_sample_box();
    bool fill_video_sample_table_box();
    bool fill_sound_sample_table_box();
    bool fill_gps_sample_table_box();
    void fill_video_data_reference_box();
    void fill_audio_data_reference_box();
    void fill_gps_data_reference_box();
    void fill_video_data_info_box();
    void fill_audio_data_info_box();
    void fill_gps_data_info_box();
    void fill_video_media_info_header_box();
    void fill_sound_media_info_header_box();
    void fill_gps_media_info_header_box();
    bool fill_video_media_info_box();
    bool fill_audio_media_info_box();
    bool fill_gps_media_info_box();
    bool fill_video_media_box();
    bool fill_audio_media_box();
    bool fill_gps_media_box();
    bool fill_video_track_box();
    bool fill_audio_track_box();
    bool fill_gps_track_box();
    bool fill_movie_box();
    bool insert_video_composition_time_to_sample_box(int64_t delta_pts);
    bool insert_video_decoding_time_to_sample_box(uint32_t delta_pts);
    bool insert_audio_decoding_time_to_sample_box(uint32_t delta_pts);
    bool insert_gps_decoding_time_to_sample_box(uint32_t delta_pts);
    bool insert_video_chunk_offset_box(uint64_t offset);
    bool insert_video_chunk_offset64_box(uint64_t offset);
    bool insert_audio_chunk_offset_box(uint64_t offset);
    bool insert_audio_chunk_offset64_box(uint64_t offset);
    bool insert_gps_chunk_offset_box(uint64_t offset);
    bool insert_gps_chunk_offset64_box(uint64_t offset);
    bool insert_video_sample_size_box(uint32_t size);
    bool insert_audio_sample_size_box(uint32_t size);
    bool insert_gps_sample_size_box(uint32_t size);
    bool insert_video_sync_sample_box(uint32_t video_frame_number);
  private :
    uint64_t       m_creation_time;
    uint64_t       m_modification_time;
    uint64_t       m_video_duration;
    uint64_t       m_audio_duration;
    uint64_t       m_gps_duration;
    Mp4FileWriter *m_file_writer;
    Mp4FileReader *m_file_reader;
    char          *m_index_file_name;
    char          *m_damaged_mp4_name;
    char          *m_fixed_mp4_name;
    uint8_t       *m_sps;
    uint8_t       *m_pps;
    uint8_t       *m_vps;
    uint32_t       m_rate;
    uint32_t       m_matrix[9];
    uint32_t       m_video_track_id;
    uint32_t       m_audio_track_id;
    uint32_t       m_gps_track_id;
    uint32_t       m_next_track_id;
    uint32_t       m_video_frame_number;
    uint32_t       m_audio_frame_number;
    uint32_t       m_gps_frame_number;
    int32_t        m_last_ctts;
    uint32_t       m_sps_length;
    uint32_t       m_pps_length;
    uint32_t       m_vps_length;
    uint32_t       m_media_data_length;
    uint32_t       m_audio_pkt_frame_number;
    uint32_t       m_original_movie_size;
    uint32_t       m_flags;
    uint16_t       m_volume;
    uint16_t       m_aac_spec_config;
    uint8_t        m_name_map;
    uint8_t        m_info_map;
    uint8_t        m_param_sets_map;
    uint8_t        m_used_version;
    bool           m_have_B_frame;
    FileTypeBox    m_file_type_box;
    MediaDataBox   m_media_data_box;
    MovieBox       m_movie_box;
    FreeBox        m_free_box;
    AM_VIDEO_INFO  m_video_info;
    AM_AUDIO_INFO  m_audio_info;
    HEVCVPS        m_hevc_vps_struct;
    HEVCSPS        m_hevc_sps_struct;
    HEVCPPS        m_hevc_pps_struct;
    AVCSPS         m_avc_sps_struct;
    AVCPPS         m_avc_pps_struct;
};

#endif /* AM_MP4_FIXER_H_ */
