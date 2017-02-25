/*******************************************************************************
 * am_time_elapse_mp4_builder.h
 *
 * History:
 *   2016-05-12  [ccjing] created file
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

#ifndef AM_MUXER_MP4_BUILDER_H_
#define AM_MUXER_MP4_BUILDER_H_
#include "am_muxer_codec_info.h"
#include "iso_base_defs.h"
#include "am_video_types.h"
#include "h264.h"
#include "h265.h"
#include <vector>
#include <deque>


#define DVIDEO_FRAME_COUNT 20000
#define DGPS_FRAME_COUNT   1000
#define DVIDEO_H264_COMPRESSORNAME "Ambarella Smart AVC"
#define DVIDEO_H265_COMPRESSORNAME "Ambarella Smart HEVC"
#define DGPS_COMPRESSORNAME        "Global Positioning System"

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

class AMTimeElapseMp4FileWriter;
struct TimeElapseMp4ConfigStruct;
class AMTimeElapseMp4Builder
{
    typedef std::vector<AM_H264_NALU> AMH264NaluList;
    typedef std::vector<AM_H265_NALU> AMH265NaluList;
    typedef std::deque<AMPacket*>     AMPacketDQ;
    typedef std::deque<AM_H265_NALU>  AMH265NaluQ;
  public:
    static AMTimeElapseMp4Builder* create(AMTimeElapseMp4FileWriter* file_writer,
                                          TimeElapseMp4ConfigStruct* config);

  private:
    AMTimeElapseMp4Builder();
    AM_STATE init(AMTimeElapseMp4FileWriter* file_writer,
                  TimeElapseMp4ConfigStruct* config);
    virtual ~AMTimeElapseMp4Builder();

  public:
    AM_STATE set_video_info(AM_VIDEO_INFO* video_info);
    AM_STATE begin_file();
    AM_STATE end_file();
    AM_STATE write_video_data(AMPacket *packet);
    AM_STATE write_SEI_data(AMPacket *usr_sei);
    AM_MUXER_CODEC_TYPE get_muxer_type();
    void clear_video_data();
    void destroy();

  private:
    AM_STATE update_media_box_size();
    AM_STATE write_h264_video_data(AMPacket *packet);
    AM_STATE write_h265_video_data(AMPacket *packet);

  private:
    void default_mp4_file_setting();
    bool find_h264_nalu(uint8_t *data, uint32_t len, H264_NALU_TYPE expect);
    bool parse_h264_sps(uint8_t *sps, uint32_t size);
    bool parse_h264_pps(uint8_t *pps, uint32_t size);
    bool find_h265_nalu(uint8_t *data, uint32_t len);
    bool parse_h265_vps(uint8_t *vps, uint32_t size);
    bool parse_h265_sps(uint8_t *sps, uint32_t size);
    bool parse_h265_pps(uint8_t *pps, uint32_t size);
    void filter_emulation_prevention_three_byte(uint8_t *input, uint8_t *output,
                                       uint32_t insize, uint32_t& outsize);

  private:
    void fill_file_type_box();
    void fill_free_box(uint32_t size);
    void fill_media_data_box();
    void fill_movie_header_box();
    void fill_object_desc_box();
    void fill_video_track_header_box();
    /*This media header box contained in the "mdia" type box.*/
    void fill_media_header_box_for_video_track();
    AM_STATE fill_video_handler_reference_box();
    void fill_video_chunk_offset32_box();
    void fill_video_chunk_offset64_box();
    void fill_video_sync_sample_box();
    AM_STATE fill_video_sample_to_chunk_box();
    void fill_video_sample_size_box();
    AM_STATE fill_avc_decoder_configuration_record_box();
    AM_STATE fill_hevc_decoder_configuration_record_box();
    AM_STATE fill_visual_sample_description_box();
    AM_STATE fill_video_decoding_time_to_sample_box();
    AM_STATE fill_video_composition_time_to_sample_box();
    AM_STATE fill_video_sample_table_box();
    void fill_video_data_reference_box();
    void fill_video_data_info_box();
    void fill_video_media_info_header_box();
    AM_STATE fill_video_media_info_box();
    AM_STATE fill_video_media_box();
    AM_STATE fill_video_track_box();
    AM_STATE fill_movie_box();

  private:
    AM_STATE insert_video_composition_time_to_sample_box(int64_t delta_pts);
    AM_STATE insert_video_decoding_time_to_sample_box(uint32_t delta_pts);
    AM_STATE insert_video_chunk_offset32_box(uint64_t offset);
    AM_STATE insert_video_chunk_offset64_box(uint64_t offset);
    AM_STATE insert_video_sample_size_box(uint32_t size);
    AM_STATE insert_video_sync_sample_box(uint32_t video_frame_number);
    AM_STATE write_video_frame_info(uint64_t offset,
                                    uint64_t size, uint8_t sync_sample);

  private:


  private:
    uint64_t                   m_overall_media_data_len   = 0;
    uint64_t                   m_video_duration           = 0;
    uint64_t                   m_creation_time            = 0;
    uint64_t                   m_modification_time        = 0;
    uint8_t                   *m_avc_sps                  = nullptr;
    uint8_t                   *m_avc_pps                  = nullptr;
    uint8_t                   *m_hevc_vps                 = nullptr;
    uint8_t                   *m_hevc_sps                 = nullptr;
    uint8_t                   *m_hevc_pps                 = nullptr;
    AMTimeElapseMp4FileWriter *m_file_writer              = nullptr;
    TimeElapseMp4ConfigStruct *m_config                   = nullptr;
    AMPacketDQ                *m_sei_queue                = nullptr;
    AMPacketDQ                *m_h265_frame_queue         = nullptr;
    int32_t                    m_last_ctts                = 0;
    uint32_t                   m_video_frame_number       = 0;
    uint32_t                   m_rate                     = 0; //default setting
    uint32_t                   m_matrix[9]                = { 0 };
    uint32_t                   m_flags                    = 0; //default setting
    uint32_t                   m_avc_sps_length           = 0;
    uint32_t                   m_avc_pps_length           = 0;
    uint32_t                   m_hevc_vps_length          = 0;
    uint32_t                   m_hevc_sps_length          = 0;
    uint32_t                   m_hevc_pps_length          = 0;
    uint32_t                   m_movie_box_pre_length     = 0;
    AM_H265_FRAME_STATE        m_h265_frame_state         = AM_H265_FRAME_NONE;
    uint16_t                   m_volume                   = 0;
    uint8_t                    m_used_version             = 0; //default setting
    uint8_t                    m_video_track_id           = 0; //default setting
    uint8_t                    m_next_track_id            = 1;  //default setting
    uint8_t                    m_write_media_data_started = 0;
    uint8_t                    m_hevc_tile_num            = 0;
    uint8_t                    m_hevc_slice_num           = 0;
    bool                       m_have_B_frame             = false;
    FileTypeBox                m_file_type_box;
    FreeBox                    m_free_box;
    MediaDataBox               m_media_data_box;
    MovieBox                   m_movie_box;
    AM_VIDEO_INFO              m_video_info;
    AMH264NaluList             m_h264_nalu_list;
    AMH265NaluList             m_h265_nalu_list;
    AMH265NaluQ                m_h265_nalu_frame_queue;
    HEVCVPS                    m_hevc_vps_struct;
    HEVCSPS                    m_hevc_sps_struct;
    HEVCPPS                    m_hevc_pps_struct;
    AVCSPS                     m_avc_sps_struct;
    AVCPPS                     m_avc_pps_struct;
};

#endif
