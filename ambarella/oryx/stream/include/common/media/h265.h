/*******************************************************************************
 * h265.h
 *
 * History:
 *   2015-3-3 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_
/* Generic NAL Unit Header
 * +---------------+---------------+
 * |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |F|   Type    |  LayerId  | TID |
 * +-------------+-----------------+
 */
enum
{
  H265_NAL_HEADER_SIZE         = 2,
  H265_FU_INDICATOR_SIZE       = 2,
  H265_PROFILE_TIER_LEVEL_SIZE = 12,
  MAX_SUB_LAYERS               = 7,
  MAX_SHORT_TERM_RPS_COUNT     = 64,
};

/*
 * FU Header
 * +---------------+
 * |7|6|5|4|3|2|1|0|
 * +-+-+-+-+-+-+-+-+
 * |S|E|  FuType   |
 * +---------------+
 */

struct AMH265FUHeader
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 7     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t type :6; /* Bit 5 ~ 0 */
#else
    uint8_t type :6; /* Bit 0 ~ 5 */
    uint8_t e    :1; /* Bit 6     */
    uint8_t s    :1; /* Bit 7     */
#endif
};
typedef struct AMH265FUHeader H265_FU_HEADER;

enum H265_NALU_TYPE
{
  H265_TRAIL_N    = 0,
  H265_TRAIL_R    = 1,
  H265_TSA_N      = 2,
  H265_TSA_R      = 3,
  H265_STSA_N     = 4,
  H265_STSA_R     = 5,
  H265_RADL_N     = 6,
  H265_RADL_R     = 7,
  H265_BLA_W_LP   = 16,
  H265_BLA_W_RADL = 17,
  H265_BLA_N_LP   = 18,
  H265_IDR_W_RADL = 19,
  H265_IDR_N_LP   = 20,
  H265_CRA_NUT    = 21,
  H265_VPS        = 32,
  H265_SPS        = 33,
  H265_PPS        = 34,
  H265_AUD        = 35,
  H265_EOS_NUT    = 36,
  H265_EOB_NUT    = 37,
  H265_FD_NUT     = 38,
  H265_SEI_PREFIX = 39,
  H265_SEI_SUFFIX = 40,
};

struct AM_H265_NALU
{
    uint8_t  *addr;    /* start address of this NALU in the bit stream */
    int32_t   type;
    uint32_t  size;    /* Size of this NALU */
    uint32_t  pkt_num; /* How many RTP packets are needed for this NALU */
    AM_H265_NALU() :
      addr(nullptr),
      type(0),
      size(0),
      pkt_num(0)
    {}
    AM_H265_NALU(const AM_H265_NALU &n) :
      addr(n.addr),
      type(n.type),
      size(n.size),
      pkt_num(n.pkt_num)
    {}
};

struct ShortTermRPS
{
    int32_t num_negative_pics;
    int32_t num_delta_pocs;
    int32_t delta_poc[32];
    uint8_t used[32];
};

struct HEVCWindow
{
    int32_t left_offset;
    int32_t right_offset;
    int32_t top_offset;
    int32_t bottom_offset;
};

struct VUI
{
    int32_t sar_num;//numerator
    int32_t sar_den;//denominator
    int32_t overscan_info_present_flag;
    int32_t overscan_appropriate_flag;
    int32_t video_signal_type_present_flag;
    int32_t video_format;
    int32_t video_full_range_flag;
    int32_t colour_description_present_flag;
    int32_t chroma_loc_info_present_flag;
    int32_t chroma_sample_loc_type_top_field;
    int32_t chroma_sample_loc_type_bottom_field;
    int32_t neutra_chroma_indication_flag;
    int32_t field_seq_flag;
    int32_t frame_field_info_present_flag;
    int32_t default_display_window_flag;
    int32_t vui_timing_info_present_flag;
    uint32_t vui_num_units_in_tick;
    uint32_t vui_time_scale;
    int32_t vui_poc_proportional_to_timing_flag;
    int32_t vui_num_ticks_poc_diff_one_minus1;
    int32_t vui_hrd_parameters_present_flag;
    int32_t bitstream_restriction_flag;
    int32_t tiles_fixed_structure_flag;
    int32_t motion_vectors_over_pic_boundaries_flag;
    int32_t restricted_ref_pic_lists_flag;
    int32_t min_spatial_segmentation_idc;
    int32_t max_bytes_per_pic_denom;
    int32_t max_bits_per_min_cu_denom;
    int32_t log2_max_mv_length_horizontal;
    int32_t log2_max_mv_length_vertical;
    uint8_t colour_primaries;
    uint8_t transfer_characteristic;
    uint8_t matrix_coeffs;
    HEVCWindow def_disp_win;
};
/* profile tier level for hevc*/
struct PTL
{
    int32_t general_profile_space;
    int32_t general_profile_idc;
    int32_t general_profile_compatibility_flag[32];
    int32_t general_level_idc;
    int32_t sub_layer_profile_space[MAX_SUB_LAYERS];
    int32_t sub_layer_profile_idc[MAX_SUB_LAYERS];
    int32_t sub_layer_level_idc[MAX_SUB_LAYERS];
    uint8_t general_tier_flag;
    uint8_t general_progressive_source_flag; // 1 bit
    uint8_t general_interlaced_source_flag; // 1 bit
    uint8_t general_non_packed_constraint_flag; // 1 bit
    uint8_t general_frame_only_constraint_flag; // 1 bit
    uint8_t sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[MAX_SUB_LAYERS];
    uint8_t sub_layer_tier_flag[MAX_SUB_LAYERS];
    uint8_t sub_layer_profile_compatibility_flags[MAX_SUB_LAYERS][32];
};

struct ScalingList
{
    /* This is a little wasteful, since sizeID 0 only needs 8 coeffs,
    and size ID 3 only has 2 arrays, not 6.*/
    uint8_t sl[4][6][64];
    uint8_t sl_dc[2][6];
};

/*
 * HEVC vps
 */
struct HEVCVPS
{

    int32_t vps_max_layers;
    int32_t vps_max_sub_layers; ///< vps_max_temporal_layers_minus1 + 1
    int32_t vps_sub_layer_ordering_info_present_flag;
    uint32_t vps_max_dec_pic_buffering[MAX_SUB_LAYERS];
    uint32_t vps_num_reorder_pics[MAX_SUB_LAYERS];
    uint32_t vps_max_latency_increase[MAX_SUB_LAYERS];
    int32_t vps_max_layer_id;
    int32_t vps_num_layer_sets; ///< vps_num_layer_sets_minus1 + 1
    uint32_t vps_num_units_in_tick;
    uint32_t vps_time_scale;
    int32_t vps_num_ticks_poc_diff_one; ///< vps_num_ticks_poc_diff_one_minus1 + 1
    int32_t vps_num_hrd_parameters;
    uint8_t vps_temporal_id_nesting_flag;
    uint8_t vps_timing_info_present_flag;
    uint8_t vps_poc_proportional_to_timing_flag;
    PTL ptl;
};

/*
 * HEVC sps
 */
struct HEVCSPS
{
    int32_t vps_id;
    int32_t chroma_format_idc;
    int32_t output_width;
    int32_t output_height;
    int32_t bit_depth;
    int32_t pixel_shift;
    uint32_t pix_fmt;
    uint32_t log2_max_poc_lsb;
    int32_t pcm_enabled_flag;
    int32_t max_sub_layers;
    uint32_t num_temporal_layers;
    uint32_t nb_st_rps;
    uint32_t log2_min_cb_size;
    uint32_t log2_diff_max_min_coding_block_size;
    uint32_t log2_min_tb_size;
    uint32_t log2_max_trafo_size;
    uint32_t log2_ctb_size;
    uint32_t log2_min_pu_size;
    int32_t max_transform_hierarchy_depth_inter;
    int32_t max_transform_hierarchy_depth_intra;
    ///< coded frame dimension in various units
    int32_t width;
    int32_t height;
    int32_t ctb_width;
    int32_t ctb_height;
    int32_t ctb_size;
    int32_t min_cb_width;
    int32_t min_cb_height;
    int32_t min_tb_width;
    int32_t min_tb_height;
    int32_t min_pu_width;
    int32_t min_pu_height;
    int32_t hshift[3];
    int32_t vshift[3];
    int32_t qp_bd_offset;
    uint16_t lt_ref_pic_poc_lsb_sps[32];
    uint8_t separate_colour_plane_flag;
    uint8_t scaling_list_enable_flag;
    uint8_t amp_enabled_flag;
    uint8_t sao_enabled;

    uint8_t long_term_ref_pics_present_flag;
    uint8_t used_by_curr_pic_lt_sps_flag[32];
    uint8_t num_long_term_ref_pics_sps;
    uint8_t sps_temporal_mvp_enabled_flag;
    uint8_t sps_strong_intra_smoothing_enable_flag;
    HEVCWindow output_window;
    HEVCWindow pic_conf_win;
    VUI vui;
    PTL ptl;
    ScalingList scaling_list;
    ShortTermRPS st_rps[MAX_SHORT_TERM_RPS_COUNT];
    struct
    {
        int32_t max_dec_pic_buffering;
        int32_t num_reorder_pics;
        int32_t max_latency_increase;
    } temporal_layer[MAX_SUB_LAYERS];
    struct
    {
        uint8_t bit_depth;
        uint8_t bit_depth_chroma;
        unsigned int log2_min_pcm_cb_size;
        unsigned int log2_max_pcm_cb_size;
        uint8_t loop_filter_disable_flag;
    } pcm;
};

/*
 * HEVC pps
 */
struct HEVCPPS
{
    // Inferred parameters
    int32_t *column_width; ///< ColumnWidth
    int32_t *row_height; ///< RowHeight
    int32_t *col_bd; ///< ColBd
    int32_t *row_bd; ///< RowBd
    int32_t *col_idxX;
    int32_t *ctb_addr_rs_to_ts; ///< CtbAddrRSToTS
    int32_t *ctb_addr_ts_to_rs; ///< CtbAddrTSToRS
    int32_t *tile_id; ///< TileId
    int32_t *tile_pos_rs; ///< TilePosRS
    int32_t *min_cb_addr_zs; ///< MinCbAddrZS
    int32_t *min_tb_addr_zs; ///< MinTbAddrZS
    int32_t sps_id; ///< seq_parameter_set_id
    //num_ref_idx_l0_default_active_minus1 + 1
    int32_t num_ref_idx_l0_default_active;
    //num_ref_idx_l1_default_active_minus1 + 1
    int32_t num_ref_idx_l1_default_active;
    int32_t pic_init_qp_minus26;
    int32_t diff_cu_qp_delta_depth;
    int32_t cb_qp_offset;
    int32_t cr_qp_offset;
    int32_t num_tile_columns; ///< num_tile_columns_minus1 + 1
    int32_t num_tile_rows; ///< num_tile_rows_minus1 + 1
    int32_t beta_offset; ///< beta_offset_div2 * 2
    int32_t tc_offset; ///< tc_offset_div2 * 2
    int32_t pps_scaling_list_data_present_flag;
    int32_t log2_parallel_merge_level; ///< log2_parallel_merge_level_minus2 + 2
    int32_t num_extra_slice_header_bits;
    uint8_t slice_header_extension_present_flag;
    uint8_t pps_extension_flag;
    uint8_t pps_extension_data_flag;
    uint8_t sign_data_hiding_flag;
    uint8_t cabac_init_present_flag;
    uint8_t constrained_intra_pred_flag;
    uint8_t transform_skip_enabled_flag;
    uint8_t cu_qp_delta_enabled_flag;
    uint8_t pic_slice_level_chroma_qp_offsets_present_flag;
    uint8_t weighted_pred_flag;
    uint8_t weighted_bipred_flag;
    uint8_t output_flag_present_flag;
    uint8_t transquant_bypass_enable_flag;
    uint8_t dependent_slice_segments_enabled_flag;
    uint8_t tiles_enabled_flag;
    uint8_t entropy_coding_sync_enabled_flag;
    uint8_t uniform_spacing_flag;
    uint8_t loop_filter_across_tiles_enabled_flag;
    uint8_t seq_loop_filter_across_slices_enabled_flag;
    uint8_t deblocking_filter_control_present_flag;
    uint8_t deblocking_filter_override_enabled_flag;
    uint8_t disable_dbf;
    uint8_t lists_modification_present_flag;
    ScalingList scaling_list;
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_H265_H_ */
