/*******************************************************************************
 * am_muxer_mp4_helper.h
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

#ifndef AM_MUXER_MP4_HELPER_H_
#define AM_MUXER_MP4_HELPER_H_

#include "am_amf_types.h"
//ADTS
/*
enum EADTSSamplingFrequency
{
  EADTSSamplingFrequency_96000 = 0,
  EADTSSamplingFrequency_88200 = 1,
  EADTSSamplingFrequency_64000 = 2,
  EADTSSamplingFrequency_48000 = 3,
  EADTSSamplingFrequency_44100 = 4,
  EADTSSamplingFrequency_32000 = 5,
  EADTSSamplingFrequency_24000 = 6,
  EADTSSamplingFrequency_22050 = 7,
  EADTSSamplingFrequency_16000 = 8,
  EADTSSamplingFrequency_12000 = 9,
  EADTSSamplingFrequency_11025 = 10,
  EADTSSamplingFrequency_8000 = 11,
  EADTSSamplingFrequency_7350 = 12,
};

struct SADTSHeader
{
  uint8_t ID;
  uint8_t layer;
  uint8_t protection_absent;
  uint8_t profile;
  uint8_t sampling_frequency_index;
  uint8_t private_bit;
  uint8_t channel_configuration;
  uint8_t original_copy;
  uint8_t home;

  uint8_t number_of_raw_data_blocks_in_frame;
  uint8_t copyright_identification_bit;
  uint8_t copyright_identification_start;
  uint16_t aac_frame_length;
  uint16_t adts_buffer_fullness;
};

uint32_t _get_adts_frame_length(uint8_t* p);
AM_STATE _parse_adts_header(uint8_t* p, SADTSHeader* header);
uint32_t _get_adts_sampling_frequency(uint8_t sampling_frequency_index);
*/

//AVC
enum {
  ENalType_unspecified = 0,
  ENalType_IDR = 5,
  ENalType_SEI = 6,
  ENalType_SPS = 7,
  ENalType_PPS = 8,
};

#define DQP_MAX_NUM (51 + 6*6)
#define DMAX_SPS_COUNT          32
#define DMAX_PPS_COUNT         256
#define DMIN_LOG2_MAX_FRAME_NUM 4
#define DMAX_LOG2_MAX_FRAME_NUM 16
#define DMAX_PICTURE_COUNT 36
#define DEXTENDED_SAR          255
#define DMIN_CACHE_BITS 25
#define DARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

enum EColorPrimaries {
  EColorPrimaries_BT709       = 1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
  EColorPrimaries_Unspecified = 2,
  EColorPrimaries_BT470M      = 4,
  EColorPrimaries_BT470BG     = 5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
  EColorPrimaries_SMPTE170M   = 6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
  EColorPrimaries_SMPTE240M   = 7, ///< functionally identical to above
  EColorPrimaries_FILM        = 8,
  EColorPrimaries_BT2020      = 9, ///< ITU-R BT2020
  EColorPrimaries_NB, ///< Not part of ABI
};

enum EColorTransferCharacteristic {
  EColorTransferCharacteristic_BT709       = 1, ///< also ITU-R BT1361
  EColorTransferCharacteristic_UNSPECIFIED = 2,
  EColorTransferCharacteristic_GAMMA22     = 4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
  EColorTransferCharacteristic_GAMMA28     = 5, ///< also ITU-R BT470BG
  EColorTransferCharacteristic_SMPTE170M    =  6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
  EColorTransferCharacteristic_SMPTE240M   = 7,
  EColorTransferCharacteristic_LINEAR       =  8, ///< "Linear transfer characteristics"
  EColorTransferCharacteristic_LOG          =  9, ///< "Logarithmic transfer characteristic (100:1 range)"
  EColorTransferCharacteristic_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt( 10 ) : 1 range)"
  EColorTransferCharacteristic_IEC61966_2_4 = 11, ///< IEC 61966-2-4
  EColorTransferCharacteristic_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
  EColorTransferCharacteristic_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
  EColorTransferCharacteristic_BT2020_10    = 14, ///< ITU-R BT2020 for 10 bit system
  EColorTransferCharacteristic_BT2020_12    = 15, ///< ITU-R BT2020 for 12 bit system
  EColorTransferCharacteristic_NB, ///< Not part of ABI
};

enum EColorSpace {
  EColorSpace_RGB         = 0,
  EColorSpace_BT709       = 1, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
  EColorSpace_UNSPECIFIED = 2,
  EColorSpace_FCC         = 4,
  EColorSpace_BT470BG     = 5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
  EColorSpace_SMPTE170M   = 6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC / functionally identical to above
  EColorSpace_SMPTE240M   = 7,
  EColorSpace_YCOCG       = 8, ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
  EColorSpace_BT2020_NCL  =  9, ///< ITU-R BT2020 non-constant luminance system
  EColorSpace_BT2020_CL   = 10, ///< ITU-R BT2020 constant luminance system
  EColorSpace_NB             , ///< Not part of ABI
};

struct SCodecAVCSPS {
  int32_t profile_idc;
  int32_t level_idc;
  int32_t chroma_format_idc;
  int32_t transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
  int32_t log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
  int32_t poc_type;                      ///< pic_order_cnt_type
  int32_t log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
  int32_t delta_pic_order_always_zero_flag;
  int32_t offset_for_non_ref_pic;
  int32_t offset_for_top_to_bottom_field;
  int32_t poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
  int32_t ref_frame_count;               ///< num_ref_frames
  int32_t gaps_in_frame_num_allowed_flag;
  int32_t mb_width;                      ///< pic_width_in_mbs_minus1 + 1
  int32_t mb_height;                     ///< pic_height_in_map_units_minus1 + 1
  int32_t frame_mbs_only_flag;
  int32_t mb_aff;                        ///< mb_adaptive_frame_field_flag
  int32_t direct_8x8_inference_flag;
  int32_t crop;                          ///< frame_cropping_flag
  uint32_t crop_left;            ///< frame_cropping_rect_left_offset
  uint32_t crop_right;           ///< frame_cropping_rect_right_offset
  uint32_t crop_top;             ///< frame_cropping_rect_top_offset
  uint32_t crop_bottom;          ///< frame_cropping_rect_bottom_offset
  int32_t vui_parameters_present_flag;
  uint32_t sar_num;
  uint32_t sar_den;
  int32_t video_signal_type_present_flag;
  int32_t full_range;
  int32_t colour_description_present_flag;
  int32_t timing_info_present_flag;
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  int32_t fixed_frame_rate_flag;
  int16_t offset_for_ref_frame[256]; // FIXME dyn aloc?
  int32_t bitstream_restriction_flag;
  int32_t num_reorder_frames;
  int32_t scaling_matrix_present;
  uint8_t scaling_matrix4[6][16];
  uint8_t scaling_matrix8[6][64];
  int32_t nal_hrd_parameters_present_flag;
  int32_t vcl_hrd_parameters_present_flag;
  int32_t pic_struct_present_flag;
  int32_t time_offset_length;
  int32_t cpb_cnt;                          ///< See H.264 E.1.2
  int32_t initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
  int32_t cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
  int32_t dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
  int32_t bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
  int32_t bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
  int32_t residual_color_transform_flag;    ///< residual_colour_transform_flag
  int32_t constraint_set_flags;             ///< constraint_set[0-3]_flag
  int32_t isnew;                              ///< flag to keep track if the decoder context needs re-init due to changed SPS

  EColorPrimaries color_primaries;
  EColorTransferCharacteristic color_trc;
  EColorSpace colorspace;
};

/**
 * Picture parameter set
 */
struct SCodecAVCPPS {
  uint32_t sps_id;
  int32_t cabac;                  ///< entropy_coding_mode_flag
  int32_t pic_order_present;      ///< pic_order_present_flag
  int32_t slice_group_count;      ///< num_slice_groups_minus1 + 1
  int32_t mb_slice_group_map_type;
  uint32_t ref_count[2];  ///< num_ref_idx_l0/1_active_minus1 + 1
  int32_t weighted_pred;          ///< weighted_pred_flag
  int32_t weighted_bipred_idc;
  int32_t init_qp;                ///< pic_init_qp_minus26 + 26
  int32_t init_qs;                ///< pic_init_qs_minus26 + 26
  int32_t chroma_qp_index_offset[2];
  int32_t deblocking_filter_parameters_present; ///< deblocking_filter_parameters_present_flag
  int32_t constrained_intra_pred;     ///< constrained_intra_pred_flag
  int32_t redundant_pic_cnt_present;  ///< redundant_pic_cnt_present_flag
  int32_t transform_8x8_mode;         ///< transform_8x8_mode_flag
  uint8_t scaling_matrix4[6][16];
  uint8_t scaling_matrix8[6][64];
  uint8_t chroma_qp_table[2][DQP_MAX_NUM + 1];  ///< pre-scaled (with chroma_qp_index_offset) version of qp_table
  int32_t chroma_qp_diff;
};

struct SCodecAVCExtraData {
  SCodecAVCSPS sps;
  SCodecAVCPPS pps;
};

AM_STATE _get_avc_sps_pps(uint8_t* data_base, uint32_t data_size,
                          uint8_t*& p_sps, uint32_t& sps_size,
                          uint8_t*& p_pps, uint32_t& pps_size);
AM_STATE _try_parse_avc_sps_pps(uint8_t* p_data, uint32_t data_size,
                                SCodecAVCExtraData* p_header);

#endif


