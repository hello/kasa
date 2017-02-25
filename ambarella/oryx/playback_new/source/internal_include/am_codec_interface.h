/**
 * am_codec_interface.h
 *
 * History:
 *  2013/11/18 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#ifndef __AM_CODEC_INTERFACE_H__
#define __AM_CODEC_INTERFACE_H__

enum {
  H264_FMT_INVALID = 0,
  H264_FMT_AVCC,      // nal delimit: 4 byte, which represents data length
  H264_FMT_ANNEXB,     // nal delimiter: 00 00 00 01
};

enum {
  EH264SliceType_P = 0,
  EH264SliceType_B = 1,
  EH264SliceType_I = 2,
  EH264SliceType_SP = 3,
  EH264SliceType_SI = 4,

  EH264SliceType_FieldOffset = 5,
  EH264SliceType_MaxValue = 9,
};

typedef struct {
#ifdef DCONFIG_BIG_ENDIAN
  TU8 syncword_11to4 : 8;
  TU8 syncword_3to0 : 4;
  TU8 id : 1;
  TU8 layer : 2;
  TU8 protection_absent : 1;
  TU8 profile : 2;
  TU8 sampling_frequency_index : 4;
  TU8 private_bit : 1;
  TU8 channel_configuration_2 : 1;
  TU8 channel_configuration_1to0 : 2;
  TU8 orignal_copy : 1;
  TU8 home : 1;
  TU8 reserved : 4;
#else
  TU8 syncword_11to4 : 8;
  TU8 protection_absent : 1;
  TU8 layer : 2;
  TU8 id : 1;
  TU8 syncword_3to0 : 4;
  TU8 channel_configuration_2 : 1;
  TU8 private_bit : 1;
  TU8 sampling_frequency_index : 4;
  TU8 profile : 2;
  TU8 reserved : 4;
  TU8 home : 1;
  TU8 orignal_copy : 1;
  TU8 channel_configuration_1to0 : 2;
#endif
} SADTSFixedHeader;

TU8 *gfNALUFindNextStartCode(TU8 *p, TU32 len);
TU8 *gfNALUFindIDR(TU8 *p, TU32 len);
TU8 *gfNALUFindSPSHeader(TU8 *p, TU32 len);
TU8 *gfNALUFindPPSEnd(TU8 *p, TU32 len);
void gfFindH264SpsPpsIdr(TU8 *data_base, TInt data_size, TU8 &has_sps, TU8 &has_pps, TU8 &has_idr, TU8 *&p_sps, TU8 *&p_pps, TU8 *&p_pps_end, TU8 *&p_idr);
void gfFindH265VpsSpsPpsIdr(TU8 *data_base, TInt data_size, TU8 &has_vps, TU8 &has_sps, TU8 &has_pps, TU8 &has_idr, TU8 *&p_vps, TU8 *&p_sps, TU8 *&p_pps, TU8 *&p_pps_end, TU8 *&p_idr);


typedef enum {
  EADTSID_MPEG2 = 0,
  EADTSID_MPEG4 = 1,
} EADTSID;

typedef enum {
  EADTSMPEG2Profile_Main = 0,
  EADTSMPEG2Profile_LowComplexity = 1,
  EADTSMPEG2Profile_ScalableSamplingRate = 2,
} EADTSMPEG2Profile;

typedef enum {
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
} EADTSSamplingFrequency;

//AAC
typedef struct {
  TU8 ID;
  TU8 layer;
  TU8 protection_absent;
  TU8 profile;
  TU8 sampling_frequency_index;
  TU8 private_bit;
  TU8 channel_configuration;
  TU8 original_copy;
  TU8 home;

  TU8 number_of_raw_data_blocks_in_frame;
  TU8 copyright_identification_bit;
  TU8 copyright_identification_start;
  TU16 aac_frame_length;
  TU16 adts_buffer_fullness;
} SADTSHeader;

TU32 gfGetADTSFrameLength(TU8 *p);
EECode gfParseADTSHeader(TU8 *p, SADTSHeader *header);
EECode gfBuildADTSHeader(TU8 *p,
  TU8 ID, TU8 layer, TU8 protection_absent, TU8 profile, TU8 sampling_frequency_index, TU8 private_bit, TU8 channel_configuration,
  TU8 original_copy, TU8 home, TU8 copyright_identification_bit, TU8 copyright_identification_start,
  TU32 frame_length, TU16 adts_buffer_fullness, TU8 number_of_raw_data_blocks_in_frame);
TU32 gfGetADTSSamplingFrequency(TU8 sampling_frequency_index);


//H264
#define DQP_MAX_NUM (51 + 6*6)
#define DMAX_SPS_COUNT          32
#define DMAX_PPS_COUNT         256
#define DMIN_LOG2_MAX_FRAME_NUM 4
#define DMAX_LOG2_MAX_FRAME_NUM 16
#define DMAX_PICTURE_COUNT 36
#define DEXTENDED_SAR          255
#define DMIN_CACHE_BITS 25
#define DARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

typedef enum {
  EColorPrimaries_BT709       = 1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
  EColorPrimaries_Unspecified = 2,
  EColorPrimaries_BT470M      = 4,
  EColorPrimaries_BT470BG     = 5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
  EColorPrimaries_SMPTE170M   = 6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
  EColorPrimaries_SMPTE240M   = 7, ///< functionally identical to above
  EColorPrimaries_FILM        = 8,
  EColorPrimaries_BT2020      = 9, ///< ITU-R BT2020
  EColorPrimaries_NB, ///< Not part of ABI
} EColorPrimaries;

typedef enum {
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
} EColorTransferCharacteristic;

typedef enum {
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
} EColorSpace;

typedef struct {
  TInt profile_idc;
  TInt level_idc;
  TInt chroma_format_idc;
  TInt transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
  TInt log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
  TInt poc_type;                      ///< pic_order_cnt_type
  TInt log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
  TInt delta_pic_order_always_zero_flag;
  TInt offset_for_non_ref_pic;
  TInt offset_for_top_to_bottom_field;
  TInt poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
  TInt ref_frame_count;               ///< num_ref_frames
  TInt gaps_in_frame_num_allowed_flag;
  TInt mb_width;                      ///< pic_width_in_mbs_minus1 + 1
  TInt mb_height;                     ///< pic_height_in_map_units_minus1 + 1
  TInt frame_mbs_only_flag;
  TInt mb_aff;                        ///< mb_adaptive_frame_field_flag
  TInt direct_8x8_inference_flag;
  TInt crop;                          ///< frame_cropping_flag
  TUint crop_left;            ///< frame_cropping_rect_left_offset
  TUint crop_right;           ///< frame_cropping_rect_right_offset
  TUint crop_top;             ///< frame_cropping_rect_top_offset
  TUint crop_bottom;          ///< frame_cropping_rect_bottom_offset
  TInt vui_parameters_present_flag;
  TU32 sar_num;
  TU32 sar_den;
  TInt video_signal_type_present_flag;
  TInt full_range;
  TInt colour_description_present_flag;
  TInt timing_info_present_flag;
  TU32 num_units_in_tick;
  TU32 time_scale;
  TInt fixed_frame_rate_flag;
  TS16 offset_for_ref_frame[256]; // FIXME dyn aloc?
  TInt bitstream_restriction_flag;
  TInt num_reorder_frames;
  TInt scaling_matrix_present;
  TU8 scaling_matrix4[6][16];
  TU8 scaling_matrix8[6][64];
  TInt nal_hrd_parameters_present_flag;
  TInt vcl_hrd_parameters_present_flag;
  TInt pic_struct_present_flag;
  TInt time_offset_length;
  TInt cpb_cnt;                          ///< See H.264 E.1.2
  TInt initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 + 1
  TInt cpb_removal_delay_length;         ///< cpb_removal_delay_length_minus1 + 1
  TInt dpb_output_delay_length;          ///< dpb_output_delay_length_minus1 + 1
  TInt bit_depth_luma;                   ///< bit_depth_luma_minus8 + 8
  TInt bit_depth_chroma;                 ///< bit_depth_chroma_minus8 + 8
  TInt residual_color_transform_flag;    ///< residual_colour_transform_flag
  TInt constraint_set_flags;             ///< constraint_set[0-3]_flag
  TInt isnew;                              ///< flag to keep track if the decoder context needs re-init due to changed SPS

  EColorPrimaries color_primaries;
  EColorTransferCharacteristic color_trc;
  EColorSpace colorspace;
} SCodecVideoH264SPS;

/**
 * Picture parameter set
 */
typedef struct {
  TUint sps_id;
  TInt cabac;                  ///< entropy_coding_mode_flag
  TInt pic_order_present;      ///< pic_order_present_flag
  TInt slice_group_count;      ///< num_slice_groups_minus1 + 1
  TInt mb_slice_group_map_type;
  TUint ref_count[2];  ///< num_ref_idx_l0/1_active_minus1 + 1
  TInt weighted_pred;          ///< weighted_pred_flag
  TInt weighted_bipred_idc;
  TInt init_qp;                ///< pic_init_qp_minus26 + 26
  TInt init_qs;                ///< pic_init_qs_minus26 + 26
  TInt chroma_qp_index_offset[2];
  TInt deblocking_filter_parameters_present; ///< deblocking_filter_parameters_present_flag
  TInt constrained_intra_pred;     ///< constrained_intra_pred_flag
  TInt redundant_pic_cnt_present;  ///< redundant_pic_cnt_present_flag
  TInt transform_8x8_mode;         ///< transform_8x8_mode_flag
  TU8 scaling_matrix4[6][16];
  TU8 scaling_matrix8[6][64];
  TU8 chroma_qp_table[2][DQP_MAX_NUM + 1];  ///< pre-scaled (with chroma_qp_index_offset) version of qp_table
  TInt chroma_qp_diff;
} SCodecVideoH264PPS;

typedef struct {
  TU32 max_width, max_height;
  TU32 framerate_num;
  TU32 framerate_den;

  StreamFormat format;
  VideoFrameRate framerate;

  TU32 profile_indicator;
  TU32 level_indicator;
} SCodecVideoCommon;

typedef struct {
  SCodecVideoCommon common;

  SCodecVideoH264SPS sps;
  SCodecVideoH264PPS pps;
} SCodecVideoH264;

extern SCodecVideoCommon *gfGetVideoCodecParser(TU8 *p_data, TMemSize data_size, StreamFormat format, EECode &ret);
extern void gfReleaseVideoCodecParser(SCodecVideoCommon *parser);
extern TU8 gfGetH264SilceType(TU8 *pdata);

typedef struct {
  TU8  configurationVersion;
  TU8  general_profile_space; // 2bits
  TU8  general_tier_flag; // 1bit
  TU8  general_profile_idc; //5bits
  TU32 general_profile_compatibility_flags;
  TU64 general_constraint_indicator_flags; // 48 bits
  TU8  general_level_idc;
  TU16 min_spatial_segmentation_idc; //'1111' + 12bits
  TU8  parallelismType; // '111111' + 2bits
  TU8  chromaFormat; // '111111' + 2bits
  TU8  bitDepthLumaMinus8; // '11111' + 3bits
  TU8  bitDepthChromaMinus8; // '11111' + 3bits
  TU16 avgFrameRate;
  TU8  constantFrameRate; // 2bits
  TU8  numTemporalLayers; // 3bits
  TU8  temporalIdNested; // 1 bit
  TU8  lengthSizeMinusOne; // 2bits
  TU8  numOfArrays;
} SHEVCDecoderConfigurationRecord;

enum {
  eAudioObjectType_AAC_MAIN = 1,
  eAudioObjectType_AAC_LC = 2,
  eAudioObjectType_AAC_SSR = 3,
  eAudioObjectType_AAC_LTP = 4,
  eAudioObjectType_AAC_scalable = 6,
  //add others, todo

  eSamplingFrequencyIndex_96000 = 0,
  eSamplingFrequencyIndex_88200 = 1,
  eSamplingFrequencyIndex_64000 = 2,
  eSamplingFrequencyIndex_48000 = 3,
  eSamplingFrequencyIndex_44100 = 4,
  eSamplingFrequencyIndex_32000 = 5,
  eSamplingFrequencyIndex_24000 = 6,
  eSamplingFrequencyIndex_22050 = 7,
  eSamplingFrequencyIndex_16000 = 8,
  eSamplingFrequencyIndex_12000 = 9,
  eSamplingFrequencyIndex_11025 = 0xa,
  eSamplingFrequencyIndex_8000 = 0xb,
  eSamplingFrequencyIndex_7350 = 0xc,
  eSamplingFrequencyIndex_escape = 0xf,//should not be this value
};

//refer to iso14496-3
#ifdef BUILD_OS_WINDOWS
#pragma pack(push,1)
typedef struct {
  TU8 samplingFrequencyIndex_high : 3;
  TU8 audioObjectType : 5;
  TU8 bitLeft : 3;
  TU8 channelConfiguration : 4;
  TU8 samplingFrequencyIndex_low : 1;
} SSimpleAudioSpecificConfig;
#pragma pack(pop)
#else
typedef struct {
  TU8 samplingFrequencyIndex_high : 3;
  TU8 audioObjectType : 5;
  TU8 bitLeft : 3;
  TU8 channelConfiguration : 4;
  TU8 samplingFrequencyIndex_low : 1;
} __attribute__((packed))SSimpleAudioSpecificConfig;
#endif

#define DMAX_JPEG_QT_TABLE_NUMBER 16

enum {
  EJpegTypeInFrameHeader_YUV422 = 0,
  EJpegTypeInFrameHeader_YUV420 = 1,
  EJpegTypeInFrameHeader_GREY8 = 2,
};

typedef struct {
  TU8 *p_table;
  TU32 length;
} SJPEGQtTable;

typedef struct {
  TU32 width, height;

  TU8 type; //0: 422, 1: 420
  TU8 number_qt_tables;
  TU16 precision;

  SJPEGQtTable qt_tables[DMAX_JPEG_QT_TABLE_NUMBER];

  TU32 total_tables_length;

  TU8 *p_jpeg_content;
  TU32 jpeg_content_length;//except eoi
} SJPEGInfo;

extern TU8 *gfGenerateAACExtraData(TU32 samplerate, TU32 channel_number, TU32 &size);
extern TU8 gfGetAACSamplingFrequencyIndex(TU32 samplerate);
extern EECode gfGetH264Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size);
extern EECode gfGetH264SPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size);
extern EECode gfGetH265Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size);
extern EECode gfGetH265VPSSPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_vps, TU32 &vps_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size);

extern TU8 *gfNALUFindFirstAVCSliceHeader(TU8 *p, TU32 len);
extern TU8 *gfNALUFindFirstAVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type);
extern TU8 *gfNALUFindFirstHEVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type, TU8 &is_first_slice);
extern TU8 *gfNALUFindFirstHEVCVPSSPSPPSAndSliceNalType(TU8 *p, TU32 len, TU8 &nal_type);
extern TU8 *gfNALUFindFirstAVCNalType(TU8 *p, TU32 len, TU8 &nal_type);

extern EECode gfGetH265SizeFromSPS(TU8 *p_data, TU32 data_size, TU32 &pic_width, TU32 &pic_height);
extern EECode gfGetH265SizeFromExtradata(TU8 *p_data, TU32 data_size, TU32 &pic_width, TU32 &pic_height);
extern EECode gfGenerateHEVCDecoderConfigurationRecord(SHEVCDecoderConfigurationRecord *record, TU8 *vps, TU32 vps_length, TU8 *sps, TU32 sps_length, TU8 *pps, TU32 pps_length, TU32 &pic_width, TU32 &pic_height);
extern EECode gfParseHEVCSPS(SHEVCDecoderConfigurationRecord *record, TU8 *sps, TU32 sps_length, TU32 &pic_width, TU32 &pic_height);

extern EECode gfParseJPEG(TU8 *p, TU32 size, SJPEGInfo *info);

extern void gfAmendVideoResolution(TU32 &width, TU32 &height);
extern EECode gfGetH264SizeFromSPS(TU8 *p_sps, TU32 &width, TU32 &height);

#endif

