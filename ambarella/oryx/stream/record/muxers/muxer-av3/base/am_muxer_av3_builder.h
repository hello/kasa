/*******************************************************************************
 * am_muxer_av3_builder.h
 *
 * History:
 *   2016-08-30  [ccjing] created file
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

#ifndef AM_MUXER_AV3_BUILDER_H_
#define AM_MUXER_AV3_BUILDER_H_
#include "am_muxer_codec_info.h"
#include "AV3_struct_defs.h"
#include "am_video_types.h"
#include "am_audio_define.h"
#include "adts.h"
#include "h264.h"
#include "h265.h"
#include <vector>
#include <deque>
#include <time.h>

#define AV3_VIDEO_FRAME_COUNT 20000
#define AV3_AUDIO_FRAME_COUNT 10000
#define AV3_VIDEO_H264_COMPRESSORNAME "Ambarella Smart AVC"
#define AV3_VIDEO_H265_COMPRESSORNAME "Ambarella Smart HEVC"
#define AV3_AUDIO_AAC_COMPRESSORNAME  "Ambarella AAC"

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

enum AM_H265_FRAME_STATE
{
  AM_H265_FRAME_NONE,
  AM_H265_FRAME_START,
  AM_H265_FRAME_SLICE,
  AM_H265_FRAME_END,
};

class AMAv3FileWriter;
class AMAv3IndexWriter;
struct AMMuxerCodecAv3Config;
class AMIDataSignature;
class AMMuxerAv3Builder
{
    typedef std::vector<AM_H264_NALU> AMH264NaluList;
    typedef std::vector<AM_H265_NALU> AMH265NaluList;
    typedef std::vector<ADTS>         AMAdtsList;
    typedef std::deque<AMPacket*>     AMPacketDQ;
    typedef std::deque<AM_H265_NALU>  AMH265NaluQ;
  public:
    static AMMuxerAv3Builder* create(AMAv3FileWriter* file_writer,
                                     AMMuxerCodecAv3Config* config);

  private:
    AMMuxerAv3Builder();
    AM_STATE init(AMAv3FileWriter* file_writer, AMMuxerCodecAv3Config* config);
    virtual ~AMMuxerAv3Builder();

  public:
    AM_STATE set_video_info(AM_VIDEO_INFO* video_info);
    AM_STATE set_audio_info(AM_AUDIO_INFO* audio_info);
    AM_STATE begin_file();
    AM_STATE end_file();
    AM_STATE write_video_data(AMPacket *packet);
    AM_STATE write_audio_data(AMPacket *packet, bool new_chunk,
                              uint32_t frame_number = 0);
    AM_STATE write_meta_data(AMPacket *packet, bool new_chunk);
    AM_MUXER_CODEC_TYPE get_muxer_type();
    void clear_video_data();
    void destroy();
    void reset_video_params();

  private:
    AM_STATE update_media_box_size();
    AM_STATE write_h264_video_data(AMPacket *packet);
    AM_STATE write_h265_video_data(AMPacket *packet);
    AM_STATE write_audio_aac_data(AMPacket *packet, bool new_chunk,
                                  uint32_t frame_number = 0);

  private:
    void default_av3_file_setting();
    bool find_h264_nalu(uint8_t *data, uint32_t len, H264_NALU_TYPE expect);
    bool parse_h264_sps(uint8_t *sps, uint32_t size);
    bool parse_h264_pps(uint8_t *pps, uint32_t size);
    bool find_h265_nalu(uint8_t *data, uint32_t len);
    bool parse_h265_vps(uint8_t *vps, uint32_t size);
    bool parse_h265_sps(uint8_t *sps, uint32_t size);
    bool parse_h265_pps(uint8_t *pps, uint32_t size);
    void find_adts(uint8_t *bs);
    void filter_emulation_prevention_three_byte(uint8_t *input, uint8_t *output,
                                       uint32_t insize, uint32_t& outsize);
    bool feed_stream_data(uint8_t* input_buf, uint32_t input_size,
                          uint32_t frame_count);
    bool get_one_frame(uint8_t **frame_start, uint32_t* frame_size);
    int FCScrambleProc2(unsigned char *data, int timestamp, int extended, int len);

  private:
    void fill_file_type_box();
    void fill_media_data_box();
    void fill_movie_header_box();
    void fill_video_track_header_box();
    void fill_audio_track_header_box();
    /*This media header box contained in the "mdia" type box.*/
    void fill_media_header_box_for_video_track();
    void fill_media_header_box_for_audio_track();
    bool fill_video_handler_reference_box();
    bool fill_audio_handler_reference_box();
    bool fill_video_media_info_box();
    bool fill_audio_media_info_box();
    void fill_video_chunk_offset32_box();
    void fill_audio_chunk_offset32_box();
    void fill_video_sync_sample_box();
    bool fill_video_sample_to_chunk_box();
    bool fill_audio_sample_to_chunk_box();
    void fill_video_sample_size_box();
    void fill_audio_sample_size_box();
    bool fill_avc_decoder_configuration_record_box();
    bool fill_visual_sample_description_box();
    void fill_aac_description_box();
    bool fill_audio_sample_description_box();
    bool fill_video_decoding_time_to_sample_box();
    bool fill_audio_decoding_time_to_sample_box();
    bool fill_video_sample_table_box();
    bool fill_sound_sample_table_box();
    void fill_video_data_reference_box();
    void fill_audio_data_reference_box();
    void fill_video_data_info_box();
    void fill_audio_data_info_box();
    void fill_video_media_info_header_box();
    void fill_sound_media_info_header_box();
    bool fill_video_track_uuid_box();
    bool fill_video_psmt_box();
    bool fill_video_psmh_box();

    bool fill_meta_sample_table_box(); //stbl for meta data
    bool fill_meta_sample_to_chunk_box();//stsc for meta data
    void fill_meta_sample_size_box();//stsz for meta data
    void fill_meta_chunk_offset_box(); //stco for meta data


    bool fill_audio_track_uuid_box();
    bool fill_audio_psmt_box();
    bool fill_audio_psmh_box();
    bool fill_video_media_box();
    bool fill_audio_media_box();
    bool fill_video_track_box();
    bool fill_audio_track_box();
    bool fill_movie_box();
    bool fill_uuid_box();
    bool fill_psfm_box();
    bool fill_xml_box();
  private:
    bool insert_video_decoding_time_to_sample_box(int64_t delta_pts);
    bool insert_audio_decoding_time_to_sample_box(int64_t delta_pts);
    bool insert_video_chunk_offset32_box(uint64_t offset);
    bool insert_audio_chunk_offset32_box(uint64_t offset);
    bool insert_meta_chunk_offset_box(uint64_t offset);
    bool insert_video_sample_size_box(uint32_t size);
    bool insert_audio_sample_size_box(uint32_t size);
    bool insert_meta_sample_size_box(uint32_t size);
    bool insert_video_sync_sample_box(uint32_t video_frame_number);
    bool insert_video_sample_to_chunk_box(bool new_chunk);
    bool insert_audio_sample_to_chunk_box(bool new_chunk);
    bool insert_meta_sample_to_chunk_box(bool new_chunk);
    bool insert_video_psmh_box(uint32_t sample_num, int32_t sec,
                               int16_t msec, int64_t time_stamp);
    bool insert_audio_psmh_box(uint32_t sample_num, int32_t sec,
                               int16_t msec, int64_t time_stamp);
  private:
    bool insert_video_frame_info(int64_t delta_pts, uint64_t offset,
                                    uint64_t size, uint8_t sync_sample);
    bool insert_audio_frame_info(int64_t delta_pts, uint64_t offset,
                                    uint64_t size, bool new_chunk);
    bool insert_meta_frame_info(uint64_t offset, uint64_t size, bool new_chunk);
  private:
    uint64_t               m_overall_media_data_len       = 0;
    uint64_t               m_video_duration               = 0;
    uint64_t               m_audio_duration               = 0;
    uint64_t               m_creation_time                = 0;
    uint64_t               m_modification_time            = 0;
    int64_t                m_last_video_pts               = 0;
    int64_t                m_last_audio_pts               = 0;
    int64_t                m_first_video_pts              = 0;
    AMAv3FileWriter       *m_file_writer                  = nullptr;
    AMAv3IndexWriter      *m_index_writer                 = nullptr;
    AMMuxerCodecAv3Config *m_config                       = nullptr;
    AMPacketDQ            *m_h265_frame_queue             = nullptr;
    uint8_t               *m_avc_sps                      = nullptr;
    uint8_t               *m_avc_pps                      = nullptr;
    uint8_t               *m_hevc_vps                     = nullptr;
    uint8_t               *m_hevc_sps                     = nullptr;
    uint8_t               *m_hevc_pps                     = nullptr;
    AMIDataSignature      *m_hash_sig                     = nullptr;
    uint32_t               m_video_frame_number           = 0;
    uint32_t               m_audio_frame_number           = 0;
    uint32_t               m_meta_frame_number            = 0;
    uint32_t               m_audio_frame_count_per_packet = 0;
    int64_t                m_video_delta_pts_sum          = 0;
    int64_t                m_audio_delta_pts_sum          = 0;
    uint32_t               m_rate                         = 0; //default setting
    uint32_t               m_matrix[9];
    uint32_t               m_flags                        = 0; //default setting
    uint32_t               m_avc_sps_length               = 0;
    uint32_t               m_avc_pps_length               = 0;
    uint32_t               m_hevc_vps_length              = 0;
    uint32_t               m_hevc_sps_length              = 0;
    uint32_t               m_hevc_pps_length              = 0;
    uint32_t               m_audio_max_bitrate            = 64000; //default setting
    uint32_t               m_audio_avg_bitrate            = 64000; //default setting
    uint32_t               m_curr_adts_index              = 0;
    AM_H265_FRAME_STATE    m_h265_frame_state             = AM_H265_FRAME_NONE;
    uint16_t               m_volume                       = 0; //default setting
    uint16_t               m_aac_spec_config              = 0xffff;
    uint8_t                m_used_version                 = 0; //default setting
    uint8_t                m_video_track_id               = 0;
    uint8_t                m_audio_track_id               = 0;
    uint8_t                m_next_track_id                = 0;
    uint8_t                m_index_info_map               = 0;
    uint8_t                m_write_media_data_started     = 0;
    uint8_t                m_hevc_tile_num                = 0;
    uint8_t                m_hevc_slice_num               = 0;
    bool                   m_have_B_frame                 = false;
    bool                   m_rotate                       = false;
    AV3FileTypeBox         m_file_type_box;
    AV3MediaDataBox        m_media_data_box;
    AV3MovieBox            m_movie_box;
    AV3UUIDBox             m_uuid_box;
    AM_AUDIO_INFO          m_audio_info;
    AM_VIDEO_INFO          m_video_info;
    AMAdtsList             m_adts;
    AMH264NaluList         m_h264_nalu_list;
    AMH265NaluList         m_h265_nalu_list;
    AMH265NaluQ            m_h265_nalu_frame_queue;
    HEVCVPS                m_hevc_vps_struct;
    HEVCSPS                m_hevc_sps_struct;
    HEVCPPS                m_hevc_pps_struct;
    AVCSPS                 m_avc_sps_struct;
    AVCPPS                 m_avc_pps_struct;
    timeval                m_time_val;
};

#endif
