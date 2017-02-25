/*******************************************************************************
 * h264.h
 *
 * History:
 *   2014-12-24 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_

/* Generic Nal header
 *
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */

struct AMH264HeaderOctet
{
#ifdef BIGENDIAN
    uint8_t f    :1; /* Bit 7     */
    uint8_t nri  :2; /* Bit 6 ~ 5 */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t nri  :2; /* Bit 5 ~ 6 */
    uint8_t f    :1; /* Bit 7     */
#endif
};

typedef struct AMH264HeaderOctet H264_NAL_HEADER;
typedef struct AMH264HeaderOctet H264_FU_INDICATOR;
typedef struct AMH264HeaderOctet H264_STAP_HEADER;
typedef struct AMH264HeaderOctet H264_MTAP_HEADER;

/*  FU header
 *  +---------------+
 *  |7|6|5|4|3|2|1|0|
 *  +-+-+-+-+-+-+-+-+
 *  |S|E|R|  Type   |
 *  +---------------+
 */

struct AMH264FUHeader
{
#ifdef BIGENDIAN
    uint8_t s    :1; /* Bit 7     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t r    :1; /* Bit 5     */
    uint8_t type :5; /* Bit 4 ~ 0 */
#else
    uint8_t type :5; /* Bit 0 ~ 4 */
    uint8_t r    :1; /* Bit 5     */
    uint8_t e    :1; /* Bit 6     */
    uint8_t s    :1; /* Bit 7     */
#endif
};

typedef struct AMH264FUHeader H264_FU_HEADER;

enum H264_NALU_TYPE
{
  H264_IBP_HEAD = 0x01,
  H264_IDR_HEAD = 0x05,
  H264_SEI_HEAD = 0x06,
  H264_SPS_HEAD = 0x07,
  H264_PPS_HEAD = 0x08,
  H264_AUD_HEAD = 0x09,
};

struct AM_H264_NALU
{
    uint8_t        *addr;    /* Start address of this NALU in the bit stream */
    H264_NALU_TYPE  type;
    uint32_t        size;    /* Size of this NALU */
    uint32_t       pkt_num; /* How many RTP packets are needed for this NALU */
    AM_H264_NALU() :
      addr(nullptr),
      type(H264_IBP_HEAD),
      size(0),
      pkt_num(0)
    {}
    AM_H264_NALU(const AM_H264_NALU &n) :
      addr(n.addr),
      type(n.type),
      size(n.size),
      pkt_num(n.pkt_num)
    {}
};

//AVC
enum
{
  DMIN_LOG2_MAX_FRAME_NUM  = 4,
  DMAX_LOG2_MAX_FRAME_NUM  = 16,
  DMIN_CACHE_BITS          = 25,
  DMAX_SPS_COUNT           = 32,
  DMAX_PICTURE_COUNT       = 36,
  DQP_MAX_NUM              = (51 + 6*6),
  DEXTENDED_SAR            =  255,
  DMAX_PPS_COUNT           = 256,
};

#ifndef DARRAY_ELEMS
#define DARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#endif

enum EColorPrimaries {
  //also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
  EColorPrimaries_BT709       = 1,
  EColorPrimaries_Unspecified = 2,
  EColorPrimaries_BT470M      = 4,
  //also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
  EColorPrimaries_BT470BG     = 5,
  //also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
  EColorPrimaries_SMPTE170M   = 6,
  EColorPrimaries_SMPTE240M   = 7, //functionally identical to above
  EColorPrimaries_FILM        = 8,
  EColorPrimaries_BT2020      = 9, // ITU-R BT2020
  EColorPrimaries_NB, //Not part of ABI
};

enum EColorTransferCharacteristic {
  EColorTransferCharacteristic_BT709        = 1, //also ITU-R BT1361
  EColorTransferCharacteristic_UNSPECIFIED  = 2,
  //also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
  EColorTransferCharacteristic_GAMMA22      = 4,
  EColorTransferCharacteristic_GAMMA28      = 5, //also ITU-R BT470BG
  //also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
  EColorTransferCharacteristic_SMPTE170M    =  6,
  EColorTransferCharacteristic_SMPTE240M    = 7,
  //"Linear transfer characteristics"
  EColorTransferCharacteristic_LINEAR       =  8,
  //"Logarithmic transfer characteristic (100:1 range)"
  EColorTransferCharacteristic_LOG          =  9,
  //"Logarithmic transfer characteristic (100 * Sqrt( 10 ) : 1 range)"
  EColorTransferCharacteristic_LOG_SQRT     = 10,
  EColorTransferCharacteristic_IEC61966_2_4 = 11, //IEC 61966-2-4
  //ITU-R BT1361 Extended Colour Gamut
  EColorTransferCharacteristic_BT1361_ECG   = 12,
  EColorTransferCharacteristic_IEC61966_2_1 = 13, //IEC 61966-2-1 (sRGB or sYCC)
  EColorTransferCharacteristic_BT2020_10    = 14, //ITU-R BT2020 for 10 bit system
  EColorTransferCharacteristic_BT2020_12    = 15, //ITU-R BT2020 for 12 bit system
  EColorTransferCharacteristic_NB, //Not part of ABI
};

enum EColorSpace {
  EColorSpace_RGB         = 0,
  //also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / SMPTE RP177 Annex B
  EColorSpace_BT709       = 1,
  EColorSpace_UNSPECIFIED = 2,
  EColorSpace_FCC         = 4,
  //ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM /
  //IEC 61966-2-4 xvYCC601
  EColorSpace_BT470BG     = 5,
  //also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC /
  //functionally identical to above
  EColorSpace_SMPTE170M   = 6,
  EColorSpace_SMPTE240M   = 7,
  //Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
  EColorSpace_YCOCG       = 8,
  EColorSpace_BT2020_NCL  =  9, //ITU-R BT2020 non-constant luminance system
  EColorSpace_BT2020_CL   = 10, //ITU-R BT2020 constant luminance system
  EColorSpace_NB             , //Not part of ABI
};

struct AVCSPS {
  int32_t profile_idc;
  int32_t level_idc;
  int32_t chroma_format_idc;
  int32_t transform_bypass;              //qpprime_y_zero_transform_bypass_flag
  int32_t log2_max_frame_num;            //log2_max_frame_num_minus4 + 4
  int32_t poc_type;                      //pic_order_cnt_type
  int32_t log2_max_poc_lsb;              //log2_max_pic_order_cnt_lsb_minus4
  int32_t delta_pic_order_always_zero_flag;
  int32_t offset_for_non_ref_pic;
  int32_t offset_for_top_to_bottom_field;
  int32_t poc_cycle_length;              //num_ref_frames_in_pic_order_cnt_cycle
  int32_t ref_frame_count;               //num_ref_frames
  int32_t gaps_in_frame_num_allowed_flag;
  int32_t mb_width;                      //pic_width_in_mbs_minus1 + 1
  int32_t mb_height;                     //pic_height_in_map_units_minus1 + 1
  int32_t frame_mbs_only_flag;
  int32_t mb_aff;                        //mb_adaptive_frame_field_flag
  int32_t direct_8x8_inference_flag;
  int32_t crop;                          //frame_cropping_flag
  uint32_t crop_left;            //frame_cropping_rect_left_offset
  uint32_t crop_right;           //frame_cropping_rect_right_offset
  uint32_t crop_top;             //frame_cropping_rect_top_offset
  uint32_t crop_bottom;          //frame_cropping_rect_bottom_offset
  int32_t vui_parameters_present_flag;
  int32_t sar_num;//numerator
  int32_t sar_den;//denominator
  int32_t video_signal_type_present_flag;
  int32_t full_range;
  int32_t colour_description_present_flag;
  int32_t timing_info_present_flag;
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  int32_t fixed_frame_rate_flag;
  int32_t bitstream_restriction_flag;
  int32_t num_reorder_frames;
  int32_t scaling_matrix_present;
  int32_t nal_hrd_parameters_present_flag;
  int32_t vcl_hrd_parameters_present_flag;
  int32_t pic_struct_present_flag;
  int32_t time_offset_length;
  int32_t cpb_cnt;                          //See H.264 E.1.2
  //initial_cpb_removal_delay_length_minus1 + 1
  int32_t initial_cpb_removal_delay_length;
  int32_t cpb_removal_delay_length;         //cpb_removal_delay_length_minus1 + 1
  int32_t dpb_output_delay_length;          //dpb_output_delay_length_minus1 + 1
  int32_t bit_depth_luma;                   //bit_depth_luma_minus8 + 8
  int32_t bit_depth_chroma;                 //bit_depth_chroma_minus8 + 8
  int32_t residual_color_transform_flag;    //residual_colour_transform_flag
  int32_t constraint_set_flags;             //constraint_set[0-3]_flag
  //flag to keep track if the decoder context needs re-init due to changed SPS
  int32_t isnew;
  EColorPrimaries color_primaries;
  EColorTransferCharacteristic color_trc;
  EColorSpace colorspace;
  int16_t offset_for_ref_frame[256]; // FIXME dyn aloc?
  uint8_t scaling_matrix4[6][16];
  uint8_t scaling_matrix8[6][64];
};

/**
 * Picture parameter set
 */
struct AVCPPS {
  uint32_t sps_id;
  int32_t  cabac;                  //entropy_coding_mode_flag
  int32_t  pic_order_present;      //pic_order_present_flag
  int32_t  slice_group_count;      //num_slice_groups_minus1 + 1
  int32_t  mb_slice_group_map_type;
  uint32_t ref_count[2];  //num_ref_idx_l0/1_active_minus1 + 1
  int32_t  weighted_pred;          //weighted_pred_flag
  int32_t  weighted_bipred_idc;
  int32_t  init_qp;                //pic_init_qp_minus26 + 26
  int32_t  init_qs;                //pic_init_qs_minus26 + 26
  int32_t  chroma_qp_index_offset[2];
  //deblocking_filter_parameters_present_flag
  int32_t  deblocking_filter_parameters_present;
  int32_t  constrained_intra_pred;     //constrained_intra_pred_flag
  int32_t  redundant_pic_cnt_present;  //redundant_pic_cnt_present_flag
  int32_t  transform_8x8_mode;         //transform_8x8_mode_flag
  int32_t  chroma_qp_diff;
  uint8_t  scaling_matrix4[6][16];
  uint8_t  scaling_matrix8[6][64];
  //pre-scaled (with chroma_qp_index_offset) version of qp_table
  uint8_t  chroma_qp_table[2][DQP_MAX_NUM + 1];
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_H264_H_ */
