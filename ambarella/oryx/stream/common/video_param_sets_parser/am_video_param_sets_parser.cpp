/*******************************************************************************
 * am_video_param_sets_parser.cpp
 *
 * History:
 *   2015-11-11 - [ccjing] created file
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
#include "am_base_include.h"
#include "am_log.h"
#include "am_video_param_sets_parser.h"
#include "am_param_sets_parser_helper.h"

#ifndef MAX_VPS_COUNT
#define MAX_VPS_COUNT 16
#endif

#ifndef MAX_SUB_LAYERS
#define MAX_SUB_LAYERS 7
#endif

#ifndef MAX_DPB_SIZE
#define MAX_DPB_SIZE 16
#endif

#ifndef MAX_SPS_COUNT
#define MAX_SPS_COUNT 32
#endif

#ifndef MAX_PPS_COUNT
#define MAX_PPS_COUNT 256
#endif

#ifndef FFMIN
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef FF_ARRAY_ELEMS
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MAX_SHORT_TERM_RPS_COUNT
#define MAX_SHORT_TERM_RPS_COUNT   64
#endif

#ifndef MAX_REFS
#define MAX_REFS   16
#endif

#ifndef MAX_LOG2_CTB_SIZE
#define MAX_LOG2_CTB_SIZE   6
#endif

#ifndef MIN_LOG2_MAX_FRAME_NUM
#define MIN_LOG2_MAX_FRAME_NUM   4
#endif

#ifndef MAX_LOG2_MAX_FRAME_NUM
#define MAX_LOG2_MAX_FRAME_NUM   (12 + 4)
#endif

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif

#ifndef EXTENDED_SAR
#define EXTENDED_SAR   255
#endif

AMIVideoParamSetsParser* AMIVideoParamSetsParser::create()
{
  AMVideoParamSetsParser* result = new AMVideoParamSetsParser();
  if(!result) {
    ERROR("Failed to create AMVideoParamSetsParser.");
  }
  return (AMIVideoParamSetsParser*)result;
}

bool AMVideoParamSetsParser::parse_hevc_vps(uint8_t *p_data, uint32_t data_size,
                        HEVCVPS& vps)
{
  bool ret = true;
  do {
    int i = 0, j = 0;
    int vps_id = 0;
    GetBitContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = data_size * 8;
    /*begin to parse vps of hevc*/
    get_bits(&gb, 16);//nalu header
    vps_id = get_bits(&gb, 4);
    if(vps_id >= MAX_VPS_COUNT) {
      ERROR("vps id is out of range : %d", vps_id);
      ret = false;
      break;
    }
    if(get_bits(&gb, 2) != 3) {
      ERROR("vps_reservec_three_2bits is not three");
      ret = false;
      break;
    }
    vps.vps_max_layers = get_bits(&gb, 6) + 1;
    vps.vps_max_sub_layers = get_bits(&gb, 3) + 1;
    vps.vps_temporal_id_nesting_flag = get_bits1(&gb);
    if(get_bits(&gb, 16) != 0xffff) {
      ERROR("vps_reserved_ffff_16bits is not 0xffff.");
      ret = false;
      break;
    }
    if(vps.vps_max_sub_layers > MAX_SUB_LAYERS) {
      ERROR("vps_max_sub_layers out of range : %d", vps.vps_max_sub_layers);
      ret = false;
      break;
    }
    if(!decode_profile_tier_level(gb, vps.ptl, vps.vps_max_sub_layers)) {
      ERROR("Failed to decode profile tier level.");
      ret = false;
      break;
    }
    vps.vps_sub_layer_ordering_info_present_flag = get_bits1(&gb);
    i = vps.vps_sub_layer_ordering_info_present_flag ? 0 :
        vps.vps_max_sub_layers - 1;
    for (; i < vps.vps_max_sub_layers; ++ i) {
      vps.vps_max_dec_pic_buffering[i] = get_ue_golomb_long(&gb) + 1;
      vps.vps_num_reorder_pics[i]      = get_ue_golomb_long(&gb);
      vps.vps_max_latency_increase[i]  = get_ue_golomb_long(&gb) - 1;
      if ((vps.vps_max_dec_pic_buffering[i] > MAX_DPB_SIZE) ||
          !vps.vps_max_dec_pic_buffering[i]) {
        ERROR("vps_max_dec_pic_buffering_minus1 out of range: %d\n",
               vps.vps_max_dec_pic_buffering[i] - 1);
        ret = false;
        break;
      }
      if (vps.vps_num_reorder_pics[i] > vps.vps_max_dec_pic_buffering[i] - 1) {
        ERROR("vps_max_num_reorder_pics out of range: %d\n",
               vps.vps_num_reorder_pics[i]);
        ret = false;
        break;
      }
    }
    if(!ret) {
      break;
    }
    vps.vps_max_layer_id   = get_bits(&gb, 6);
    vps.vps_num_layer_sets = get_ue_golomb_long(&gb) + 1;
    for (i = 1; i < vps.vps_num_layer_sets; ++ i) {
      for (j = 0; j <= vps.vps_max_layer_id; ++ j) {
        skip_bits(&gb, 1); // layer_id_included_flag[i][j]
      }
    }
    vps.vps_timing_info_present_flag = get_bits1(&gb);
    if (vps.vps_timing_info_present_flag) {
      vps.vps_num_units_in_tick               = get_bits_long(&gb, 32);
      vps.vps_time_scale                      = get_bits_long(&gb, 32);
      vps.vps_poc_proportional_to_timing_flag = get_bits1(&gb);
      if (vps.vps_poc_proportional_to_timing_flag)
        vps.vps_num_ticks_poc_diff_one = get_ue_golomb_long(&gb) + 1;
      vps.vps_num_hrd_parameters = get_ue_golomb_long(&gb);
      for (i = 0; i < vps.vps_num_hrd_parameters; i++) {
        int common_inf_present = 1;
        get_ue_golomb_long(&gb); // hrd_layer_set_idx
        if (i) {
          common_inf_present = get_bits1(&gb);
        }
        decode_hrd(gb, common_inf_present, vps.vps_max_sub_layers);
      }
    }
    get_bits1(&gb); /* vps_extension_flag */
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::parse_hevc_sps(uint8_t *p_data,
                                            uint32_t data_size,
                                            HEVCSPS& sps)
{
  bool ret = true;
  do {
    GetBitContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = data_size * 8;
    int32_t sps_id = 0;
    int32_t log2_diff_max_min_transform_block_size;
    int32_t bit_depth_chroma, vui_present, sublayer_ordering_info;
    int32_t start = 0;
    int32_t i;
    //begin to parse hevc sps.
    get_bits(&gb, 16);//nalu header
    sps.vps_id = get_bits(&gb, 4);
    if (sps.vps_id >= MAX_VPS_COUNT) {
      ERROR("VPS id out of range: %d\n", sps.vps_id);
      ret = false;
      break;
    }
    sps.max_sub_layers = get_bits(&gb, 3) + 1;
    if (sps.max_sub_layers > MAX_SUB_LAYERS) {
      ERROR("sps_max_sub_layers out of range: %d\n",
             sps.max_sub_layers);
      ret = false;
      break;
    }
    skip_bits(&gb, 1); // temporal_id_nesting_flag
    if (!decode_profile_tier_level(gb, sps.ptl, sps.max_sub_layers)) {
      ERROR("Failed to decoding profile tier level\n");
      ret = false;
      break;
    }
    sps_id = get_ue_golomb_long(&gb);
    if (sps_id >= MAX_SPS_COUNT) {
      ERROR("SPS id out of range: %d\n", sps_id);
      ret = false;
      break;
    }
    sps.chroma_format_idc = get_ue_golomb_long(&gb);
    if (sps.chroma_format_idc != 1) {
      ERROR("chroma_format_idc != 1\n");
      ret = false;
      break;
    }
    if (sps.chroma_format_idc == 3) {
      sps.separate_colour_plane_flag = get_bits1(&gb);
    }
    sps.width  = get_ue_golomb_long(&gb);
    sps.height = get_ue_golomb_long(&gb);
    if (get_bits1(&gb)) { // pic_conformance_flag
      //TODO: * 2 is only valid for 420
      sps.pic_conf_win.left_offset   = get_ue_golomb_long(&gb) * 2;
      sps.pic_conf_win.right_offset  = get_ue_golomb_long(&gb) * 2;
      sps.pic_conf_win.top_offset    = get_ue_golomb_long(&gb) * 2;
      sps.pic_conf_win.bottom_offset = get_ue_golomb_long(&gb) * 2;
      sps.output_window = sps.pic_conf_win;
    }
    sps.bit_depth   = get_ue_golomb_long(&gb) + 8;
    bit_depth_chroma = get_ue_golomb_long(&gb) + 8;
    if (bit_depth_chroma != sps.bit_depth) {
      ERROR("Luma bit depth (%d) is different from chroma bit depth (%d), "
          "this is unsupported.\n", sps.bit_depth, bit_depth_chroma);
      ret = false;
      break;
    }
    if (sps.chroma_format_idc == 1) {
//      switch (sps.bit_depth) {
//        case 8:  sps.pix_fmt = AV_PIX_FMT_YUV420P;   break;
//        case 9:  sps.pix_fmt = AV_PIX_FMT_YUV420P9;  break;
//        case 10: sps.pix_fmt = AV_PIX_FMT_YUV420P10; break;
//        default: {
//          ERROR("Unsupported bit depth: %d\n", sps.bit_depth);
//          ret = false;
//        } break;
//      }
      if((sps.bit_depth != 8) &&
         (sps.bit_depth != 9) &&
         (sps.bit_depth != 10)) {
        ERROR("Unsupported bit depth: %d\n", sps.bit_depth);
        ret = false;
        break;
      }
    } else {
      ERROR("non-4:2:0 support is currently unspecified.\n");
      ret = false;
      break;
    }
//    desc = av_pix_fmt_desc_get(sps->pix_fmt);
//    if (!desc) {
//      ret = AVERROR(EINVAL);
//      goto err;
//    }
    sps.hshift[0] = sps.vshift[0] = 0;
    sps.hshift[2] = sps.hshift[1] = 0;
    sps.vshift[2] = sps.vshift[1] = 0;
//    sps->hshift[2] = sps->hshift[1] = desc->log2_chroma_w;
//    sps->vshift[2] = sps->vshift[1] = desc->log2_chroma_h;
    sps.pixel_shift = sps.bit_depth > 8;
    sps.log2_max_poc_lsb = get_ue_golomb_long(&gb) + 4;
    if (sps.log2_max_poc_lsb > 16) {
      ERROR("log2_max_pic_order_cnt_lsb_minus4 out range: %d\n",
             sps.log2_max_poc_lsb - 4);
      ret = false;
      break;
    }
    sublayer_ordering_info = get_bits1(&gb);
    sps.num_temporal_layers = sublayer_ordering_info ? sps.max_sub_layers : 1;
    start = sublayer_ordering_info ? 0 : (sps.max_sub_layers - 1);
    for (int32_t j = start; j < sps.max_sub_layers; ++ j) {
      sps.temporal_layer[j].max_dec_pic_buffering = get_ue_golomb_long(&gb) + 1;
      sps.temporal_layer[j].num_reorder_pics      = get_ue_golomb_long(&gb);
      sps.temporal_layer[j].max_latency_increase  = get_ue_golomb_long(&gb) - 1;
      if (sps.temporal_layer[j].max_dec_pic_buffering > MAX_DPB_SIZE) {
        ERROR("sps_max_dec_pic_buffering_minus1 out of range: %d\n",
               sps.temporal_layer[j].max_dec_pic_buffering - 1);
        ret = false;
        break;
      }
      if (sps.temporal_layer[j].num_reorder_pics >
      sps.temporal_layer[j].max_dec_pic_buffering - 1) {
        ERROR("sps_max_num_reorder_pics out of range: %d\n",
               sps.temporal_layer[j].num_reorder_pics);
        ret = false;
        break;
      }
    }
    int32_t max_dec_pic_buffering = sps.temporal_layer[start].max_dec_pic_buffering;
    int32_t num_reorder_pics = sps.temporal_layer[start].num_reorder_pics;
    int32_t max_latency_increase = sps.temporal_layer[start].max_latency_increase;
    if (!sublayer_ordering_info) {
      for (int32_t j = 0; j < start; ++ j){
        sps.temporal_layer[j].max_dec_pic_buffering = max_dec_pic_buffering;
        sps.temporal_layer[j].num_reorder_pics      = num_reorder_pics;
        sps.temporal_layer[j].max_latency_increase  = max_latency_increase;
      }
    }

    sps.log2_min_cb_size             = get_ue_golomb_long(&gb) + 3;
    sps.log2_diff_max_min_coding_block_size    = get_ue_golomb_long(&gb);
    sps.log2_min_tb_size                       = get_ue_golomb_long(&gb) + 2;
    log2_diff_max_min_transform_block_size      = get_ue_golomb_long(&gb);
    sps.log2_max_trafo_size  =
        log2_diff_max_min_transform_block_size + sps.log2_min_tb_size;
    if (sps.log2_min_tb_size >= sps.log2_min_cb_size) {
      ERROR("Invalid value for log2_min_tb_size");
      ret = false;
      break;
    }
    sps.max_transform_hierarchy_depth_inter = get_ue_golomb_long(&gb);
    sps.max_transform_hierarchy_depth_intra = get_ue_golomb_long(&gb);
    sps.scaling_list_enable_flag = get_bits1(&gb);
    if (sps.scaling_list_enable_flag) {
      set_default_scaling_list_data(&sps.scaling_list);
      if (get_bits1(&gb)) {
        if(!scaling_list_data(gb, &sps.scaling_list)) {
          ERROR("Failed to scaling list data.");
          ret = false;
          break;
        }
      }
    }
    sps.amp_enabled_flag = get_bits1(&gb);
    sps.sao_enabled      = get_bits1(&gb);
    sps.pcm_enabled_flag = get_bits1(&gb);
    if (sps.pcm_enabled_flag) {
      sps.pcm.bit_depth   = get_bits(&gb, 4) + 1;
      sps.pcm.bit_depth_chroma = get_bits(&gb, 4) + 1;
      sps.pcm.log2_min_pcm_cb_size = get_ue_golomb_long(&gb) + 3;
      sps.pcm.log2_max_pcm_cb_size = sps.pcm.log2_min_pcm_cb_size +
          get_ue_golomb_long(&gb);
      if (sps.pcm.bit_depth > sps.bit_depth) {
        ERROR("PCM bit depth (%d) is greater than normal bit depth (%d)\n",
               sps.pcm.bit_depth, sps.bit_depth);
        ret = false;
        break;
      }
      sps.pcm.loop_filter_disable_flag = get_bits1(&gb);
    }
    sps.nb_st_rps = get_ue_golomb_long(&gb);
    if (sps.nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
      ERROR("Too many short term RPS: %d.\n", sps.nb_st_rps);
      ret = false;
      break;
    }
    for (uint32_t j = 0; j < sps.nb_st_rps; ++ j) {
      if (!hevc_decode_short_term_rps(gb, &sps.st_rps[j],
                                               &sps, 0)) {
        ERROR("hevc_decode_short_term_rps error.");
        ret = false;
        break;
      }
    }
    sps.long_term_ref_pics_present_flag = get_bits1(&gb);
    if (sps.long_term_ref_pics_present_flag) {
      sps.num_long_term_ref_pics_sps = get_ue_golomb_long(&gb);
      for (i = 0; i < sps.num_long_term_ref_pics_sps; ++ i) {
        sps.lt_ref_pic_poc_lsb_sps[i] = get_bits(&gb, (int32_t)sps.log2_max_poc_lsb);
        sps.used_by_curr_pic_lt_sps_flag[i] = get_bits1(&gb);
      }
    }
    sps.sps_temporal_mvp_enabled_flag          = get_bits1(&gb);
    sps.sps_strong_intra_smoothing_enable_flag = get_bits1(&gb);
    sps.vui.sar_num = 0;
    sps.vui.sar_den = 1;
    vui_present = get_bits1(&gb);
    if (vui_present) {
      decode_vui(gb, &sps);
    }
    skip_bits(&gb, 1); // sps_extension_flag
//    if (s->strict_def_disp_win) {
//      sps->output_window.left_offset   += sps->vui.def_disp_win.left_offset;
//      sps->output_window.right_offset  += sps->vui.def_disp_win.right_offset;
//      sps->output_window.top_offset    += sps->vui.def_disp_win.top_offset;
//      sps->output_window.bottom_offset += sps->vui.def_disp_win.bottom_offset;
//    }
//    if (sps.output_window.left_offset & (0x1F >> (sps.pixel_shift)) &&
//        !(s->avctx->flags & CODEC_FLAG_UNALIGNED)) {
//      sps->output_window.left_offset &= ~(0x1F >> (sps->pixel_shift));
//      av_log(s->avctx, AV_LOG_WARNING, "Reducing left output window to %d "
//             "chroma samples to preserve alignment.\n",
//             sps->output_window.left_offset);
//    }
    sps.output_width  = sps.width -
        (sps.output_window.left_offset + sps.output_window.right_offset);
    sps.output_height = sps.height -
        (sps.output_window.top_offset + sps.output_window.bottom_offset);
    if (sps.output_width <= 0 || sps.output_height <= 0) {
      WARN("Invalid visible frame dimensions: %dx%d.\n",
             sps.output_width, sps.output_height);
      sps.pic_conf_win.left_offset   =
          sps.pic_conf_win.right_offset  =
              sps.pic_conf_win.top_offset    =
                  sps.pic_conf_win.bottom_offset = 0;
      sps.output_width  = sps.width;
      sps.output_height = sps.height;
    }

    // Inferred parameters
    sps.log2_ctb_size     = sps.log2_min_cb_size
        + sps.log2_diff_max_min_coding_block_size;
    sps.log2_min_pu_size  = sps.log2_min_cb_size - 1;
    sps.ctb_width         =
        (sps.width  + (1 << sps.log2_ctb_size) - 1) >> sps.log2_ctb_size;
    sps.ctb_height        =
        (sps.height + (1 << sps.log2_ctb_size) - 1) >> sps.log2_ctb_size;
    sps.ctb_size          = sps.ctb_width * sps.ctb_height;

    sps.min_cb_width      = sps.width  >> sps.log2_min_cb_size;
    sps.min_cb_height     = sps.height >> sps.log2_min_cb_size;
    sps.min_tb_width      = sps.width  >> sps.log2_min_tb_size;
    sps.min_tb_height     = sps.height >> sps.log2_min_tb_size;
    sps.min_pu_width      = sps.width  >> sps.log2_min_pu_size;
    sps.min_pu_height     = sps.height >> sps.log2_min_pu_size;

    sps.qp_bd_offset      = 6 * (sps.bit_depth - 8);

    if ((sps.width  & ((1 << sps.log2_min_cb_size) - 1)) ||
        (sps.height & ((1 << sps.log2_min_cb_size) - 1))) {
      ERROR("Invalid coded frame dimensions.\n");
      ret = false;
      break;
    }

    if (sps.log2_ctb_size > MAX_LOG2_CTB_SIZE) {
      ERROR("CTB size out of range: 2^%d\n", sps.log2_ctb_size);
      ret = false;
      break;
    }
    if (sps.max_transform_hierarchy_depth_inter >
        (int32_t)(sps.log2_ctb_size - sps.log2_min_tb_size)) {
      ERROR("max_transform_hierarchy_depth_inter out of range: %d\n",
             sps.max_transform_hierarchy_depth_inter);
      ret = false;
      break;
    }
    if (sps.max_transform_hierarchy_depth_intra >
    (int32_t)(sps.log2_ctb_size - sps.log2_min_tb_size)) {
      ERROR("max_transform_hierarchy_depth_intra out of range: %d\n",
             sps.max_transform_hierarchy_depth_intra);
      ret = false;
      break;
    }
    if (sps.log2_max_trafo_size > FFMIN(sps.log2_ctb_size, 5)) {
      ERROR("max transform block size out of range: %d\n",
             sps.log2_max_trafo_size);
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::parse_hevc_pps(uint8_t *p_data, uint32_t data_size,
                                HEVCPPS& pps, HEVCSPS& sps)
{
  bool ret = true;
  do {
    GetBitContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = data_size * 8;
    int pic_area_in_ctbs, pic_area_in_min_cbs, pic_area_in_min_tbs;
    int log2_diff_ctb_min_tb_size;
    int i, j, x, y, ctb_addr_rs, tile_id;
    int pps_id = 0;
    INFO("Decoding PPS\n");
    // Default values
    pps.loop_filter_across_tiles_enabled_flag = 1;
    pps.num_tile_columns                      = 1;
    pps.num_tile_rows                         = 1;
    pps.uniform_spacing_flag                  = 1;
    pps.disable_dbf                           = 0;
    pps.beta_offset                           = 0;
    pps.tc_offset                             = 0;
    get_bits(&gb, 16);//nalu header
    // Coded parameters
    pps_id = get_ue_golomb_long(&gb);
    if (pps_id >= MAX_PPS_COUNT) {
      ERROR("PPS id out of range: %d\n", pps_id);
      ret = false;
      break;
    }
    pps.sps_id = get_ue_golomb_long(&gb);
    if (pps.sps_id >= MAX_SPS_COUNT) {
      ERROR("SPS id out of range: %d\n", pps.sps_id);
      ret = false;
      break;
    }
    pps.dependent_slice_segments_enabled_flag = get_bits1(&gb);
    pps.output_flag_present_flag              = get_bits1(&gb);
    pps.num_extra_slice_header_bits           = get_bits(&gb, 3);
    pps.sign_data_hiding_flag = get_bits1(&gb);
    pps.cabac_init_present_flag = get_bits1(&gb);
    pps.num_ref_idx_l0_default_active = get_ue_golomb_long(&gb) + 1;
    pps.num_ref_idx_l1_default_active = get_ue_golomb_long(&gb) + 1;
    pps.pic_init_qp_minus26 = get_se_golomb(&gb);
    pps.constrained_intra_pred_flag = get_bits1(&gb);
    pps.transform_skip_enabled_flag = get_bits1(&gb);
    pps.cu_qp_delta_enabled_flag = get_bits1(&gb);
    pps.diff_cu_qp_delta_depth   = 0;
    if (pps.cu_qp_delta_enabled_flag) {
      pps.diff_cu_qp_delta_depth = get_ue_golomb_long(&gb);
    }
    pps.cb_qp_offset = get_se_golomb(&gb);
    if (pps.cb_qp_offset < -12 || pps.cb_qp_offset > 12) {
      ERROR("pps_cb_qp_offset out of range: %d\n", pps.cb_qp_offset);
      ret = false;
      break;
    }
    pps.cr_qp_offset = get_se_golomb(&gb);
    if (pps.cr_qp_offset < -12 || pps.cr_qp_offset > 12) {
      ERROR("pps_cr_qp_offset out of range: %d\n", pps.cr_qp_offset);
      ret = false;
      break;
    }
    pps.pic_slice_level_chroma_qp_offsets_present_flag = get_bits1(&gb);
    pps.weighted_pred_flag   = get_bits1(&gb);
    pps.weighted_bipred_flag = get_bits1(&gb);
    pps.transquant_bypass_enable_flag    = get_bits1(&gb);
    pps.tiles_enabled_flag               = get_bits1(&gb);
    pps.entropy_coding_sync_enabled_flag = get_bits1(&gb);
    if (pps.tiles_enabled_flag) {
      pps.num_tile_columns     = get_ue_golomb_long(&gb) + 1;
      pps.num_tile_rows        = get_ue_golomb_long(&gb) + 1;
      if ((pps.num_tile_columns == 0) ||
          (pps.num_tile_columns >= sps.width)) {
        ERROR("num_tile_columns_minus1 out of range: %d, sps.width: %d\n",
              pps.num_tile_columns - 1, sps.width);
        ret = false;
        break;
      }
      if (pps.num_tile_rows == 0 ||
          pps.num_tile_rows >= sps.height) {
        ERROR("num_tile_rows_minus1 out of range: %d\n",
               pps.num_tile_rows - 1);
        ret = false;
        break;
      }
      pps.column_width = new int32_t[pps.num_tile_columns];
      pps.row_height   = new int32_t[pps.num_tile_rows];
      if (!pps.column_width || !pps.row_height) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      pps.uniform_spacing_flag = get_bits1(&gb);
      if (!pps.uniform_spacing_flag) {
        int sum = 0;
        for (i = 0; i < pps.num_tile_columns - 1; ++ i) {
          pps.column_width[i] = get_ue_golomb_long(&gb) + 1;
          sum += pps.column_width[i];
        }
        if (sum >= sps.ctb_width) {
          ERROR("Invalid tile widths: %d.\n", sum);
          ret = false;
          break;
        }
        pps.column_width[pps.num_tile_columns - 1] = sps.ctb_width - sum;
        sum = 0;
        for (i = 0; i < pps.num_tile_rows - 1; ++ i) {
          pps.row_height[i] = get_ue_golomb_long(&gb) + 1;
          sum += pps.row_height[i];
        }
        if (sum >= sps.ctb_height) {
          ERROR("Invalid tile heights.\n");
          ret = false;
          break;
        }
        pps.row_height[pps.num_tile_rows - 1] = sps.ctb_height - sum;
      }
      pps.loop_filter_across_tiles_enabled_flag = get_bits1(&gb);
    }
    pps.seq_loop_filter_across_slices_enabled_flag = get_bits1(&gb);
    pps.deblocking_filter_control_present_flag = get_bits1(&gb);
    if (pps.deblocking_filter_control_present_flag) {
      pps.deblocking_filter_override_enabled_flag = get_bits1(&gb);
      pps.disable_dbf = get_bits1(&gb);
      if (!pps.disable_dbf) {
        pps.beta_offset = get_se_golomb(&gb) * 2;
        pps.tc_offset = get_se_golomb(&gb) * 2;
        if (pps.beta_offset/2 < -6 || pps.beta_offset/2 > 6) {
          ERROR("pps_beta_offset_div2 out of range: %d\n",
                 pps.beta_offset/2);
          ret = false;
          break;
        }
        if (pps.tc_offset/2 < -6 || pps.tc_offset/2 > 6) {
          ERROR("pps_tc_offset_div2 out of range: %d\n",
                 pps.tc_offset/2);
          ret = false;
          break;
        }
      }
    }
    pps.pps_scaling_list_data_present_flag = get_bits1(&gb);
    if (pps.pps_scaling_list_data_present_flag) {
      set_default_scaling_list_data(&pps.scaling_list);
      if(!scaling_list_data(gb, &pps.scaling_list)) {
        ERROR("Faield to scaling list data.");
        ret = false;
        break;
      }
    }
    pps.lists_modification_present_flag = get_bits1(&gb);
    pps.log2_parallel_merge_level       = get_ue_golomb_long(&gb) + 2;
    if (pps.log2_parallel_merge_level > (int32_t)sps.log2_ctb_size) {
      ERROR("log2_parallel_merge_level_minus2 out of range: %d\n",
             pps.log2_parallel_merge_level - 2);
      ret = false;
      break;
    }
    pps.slice_header_extension_present_flag = get_bits1(&gb);
    pps.pps_extension_flag                  = get_bits1(&gb);
    // Inferred parameters
    pps.col_bd   = new int32_t[pps.num_tile_columns + 1];
    pps.row_bd   = new int32_t[pps.num_tile_rows + 1];
    pps.col_idxX = new int32_t[sps.ctb_width];
    if (!pps.col_bd || !pps.row_bd || !pps.col_idxX) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    if (pps.uniform_spacing_flag) {
      if (!pps.column_width) {
        pps.column_width = new int32_t[pps.num_tile_columns];
        pps.row_height   = new int32_t[pps.num_tile_rows];
      }
      if (!pps.column_width || !pps.row_height) {
        ret = false;
        ERROR("Failed to malloc memory.");
        break;
      }
      for (i = 0; i < pps.num_tile_columns; ++ i) {
        pps.column_width[i] = ((i + 1) * sps.ctb_width) / pps.num_tile_columns -
            (i * sps.ctb_width) / pps.num_tile_columns;
      }
      for (i = 0; i < pps.num_tile_rows; ++ i) {
        pps.row_height[i] = ((i + 1) * sps.ctb_height) / pps.num_tile_rows -
            (i * sps.ctb_height) / pps.num_tile_rows;
      }
    }
    pps.col_bd[0] = 0;
    for (i = 0; i < pps.num_tile_columns; ++ i) {
      pps.col_bd[i + 1] = pps.col_bd[i] + pps.column_width[i];
    }
    pps.row_bd[0] = 0;
    for (i = 0; i < pps.num_tile_rows; ++ i) {
      pps.row_bd[i + 1] = pps.row_bd[i] + pps.row_height[i];
    }
    for (i = 0, j = 0; i < sps.ctb_width; ++ i) {
      if (i > pps.col_bd[j]) {
        ++ j;
      }
      pps.col_idxX[i] = j;
    }
     //6.5
    pic_area_in_ctbs     = sps.ctb_width    * sps.ctb_height;
    pic_area_in_min_cbs  = sps.min_cb_width * sps.min_cb_height;
    pic_area_in_min_tbs  = sps.min_tb_width * sps.min_tb_height;

    pps.ctb_addr_rs_to_ts = new int32_t[pic_area_in_ctbs];
    pps.ctb_addr_ts_to_rs = new int32_t[pic_area_in_ctbs];
    pps.tile_id           = new int32_t[pic_area_in_ctbs];
    pps.min_cb_addr_zs    = new int32_t[pic_area_in_min_cbs];
    pps.min_tb_addr_zs    = new int32_t[pic_area_in_min_tbs];
    if (!pps.ctb_addr_rs_to_ts || !pps.ctb_addr_ts_to_rs ||
        !pps.tile_id || !pps.min_cb_addr_zs || !pps.min_tb_addr_zs) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (ctb_addr_rs = 0; ctb_addr_rs < pic_area_in_ctbs; ctb_addr_rs++) {
      int tb_x   = ctb_addr_rs % sps.ctb_width;
      int tb_y   = ctb_addr_rs / sps.ctb_width;
      int tile_x = 0;
      int tile_y = 0;
      int val    = 0;
      for (i = 0; i < pps.num_tile_columns; ++ i) {
        if (tb_x < pps.col_bd[i + 1]) {
          tile_x = i;
          break;
        }
      }
      for (i = 0; i < pps.num_tile_rows; ++ i) {
        if (tb_y < pps.row_bd[i + 1]) {
          tile_y = i;
          break;
        }
      }
      for (i = 0; i < tile_x; ++ i )
        val += pps.row_height[tile_y] * pps.column_width[i];
      for (i = 0; i < tile_y; ++ i )
        val += sps.ctb_width * pps.row_height[i];

      val += (tb_y - pps.row_bd[tile_y]) * pps.column_width[tile_x] +
          tb_x - pps.col_bd[tile_x];

      pps.ctb_addr_rs_to_ts[ctb_addr_rs] = val;
      pps.ctb_addr_ts_to_rs[val] = ctb_addr_rs;
    }
    for (j = 0, tile_id = 0; j < pps.num_tile_rows; ++ j)
      for (i = 0; i < pps.num_tile_columns; ++ i, ++ tile_id)
        for (y = pps.row_bd[j]; y < pps.row_bd[j + 1]; ++ y)
          for (x = pps.col_bd[i]; x < pps.col_bd[i + 1]; ++ x)
            pps.tile_id[pps.ctb_addr_rs_to_ts[y * sps.ctb_width + x]] = tile_id;

    pps.tile_pos_rs = new int32_t[tile_id];
    if (!pps.tile_pos_rs) {
      ERROR("Failed to malloc memory.");
      ret = false;
      break;
    }
    for (j = 0; j < pps.num_tile_rows; ++ j) {
      for (i = 0; i < pps.num_tile_columns; ++ i) {
        pps.tile_pos_rs[j * pps.num_tile_columns + i] =
            pps.row_bd[j] * sps.ctb_width + pps.col_bd[i];
      }
    }
    for (y = 0; y < sps.min_cb_height; ++ y) {
      for (x = 0; x < sps.min_cb_width; ++ x) {
        int tb_x = x >> sps.log2_diff_max_min_coding_block_size;
        int tb_y = y >> sps.log2_diff_max_min_coding_block_size;
        int ctb_addr_rs = sps.ctb_width * tb_y + tb_x;
        int val = pps.ctb_addr_rs_to_ts[ctb_addr_rs] <<
            (sps.log2_diff_max_min_coding_block_size * 2);
        for (uint32_t kk = 0; kk < sps.log2_diff_max_min_coding_block_size; ++ kk) {
          int m = 1 << kk;
          val += (m & x ? m * m : 0) + (m & y ? 2 * m * m : 0);
        }
        pps.min_cb_addr_zs[y * sps.min_cb_width + x] = val;
      }
    }
    log2_diff_ctb_min_tb_size = sps.log2_ctb_size - sps.log2_min_tb_size;
    for (y = 0; y < sps.min_tb_height; ++ y) {
      for (x = 0; x < sps.min_tb_width; ++ x) {
        int tb_x = x >> log2_diff_ctb_min_tb_size;
        int tb_y = y >> log2_diff_ctb_min_tb_size;
        int ctb_addr_rs = sps.ctb_width * tb_y + tb_x;
        int val = pps.ctb_addr_rs_to_ts[ctb_addr_rs] <<
            (log2_diff_ctb_min_tb_size * 2);
        for (i = 0; i < log2_diff_ctb_min_tb_size; ++ i) {
          int m = 1 << i;
          val += (m & x ? m * m : 0) + (m & y ? 2 * m * m : 0);
        }
        pps.min_tb_addr_zs[y * sps.min_tb_width + x] = val;
      }
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::parse_avc_sps(uint8_t *p_data, uint32_t data_size,
                                           AVCSPS& sps)
{
  bool ret = true;
  do {
    int32_t profile_idc, level_idc, constraint_set_flags = 0;
    uint32_t sps_id;
    int32_t i, log2_max_frame_num_minus4;
    GetBitContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = data_size * 8;
    get_bits(&gb, 8);//sps header
    profile_idc           = get_bits(&gb, 8);
    constraint_set_flags |= get_bits1(&gb) << 0;   // constraint_set0_flag
    constraint_set_flags |= get_bits1(&gb) << 1;   // constraint_set1_flag
    constraint_set_flags |= get_bits1(&gb) << 2;   // constraint_set2_flag
    constraint_set_flags |= get_bits1(&gb) << 3;   // constraint_set3_flag
    constraint_set_flags |= get_bits1(&gb) << 4;   // constraint_set4_flag
    constraint_set_flags |= get_bits1(&gb) << 5;   // constraint_set5_flag
    get_bits(&gb, 2); // reserved
    level_idc = get_bits(&gb, 8);
    sps_id    = get_ue_golomb_31(&gb);

    if (sps_id >= MAX_SPS_COUNT) {
      ERROR("avc sps_id (%d) out of range\n", sps_id);
      ret = false;
      break;
    }
    sps.time_offset_length   = 24;
    sps.profile_idc          = profile_idc;
    sps.constraint_set_flags = constraint_set_flags;
    sps.level_idc            = level_idc;
    sps.full_range           = -1;
    memset(sps.scaling_matrix4, 16, sizeof(sps.scaling_matrix4));
    memset(sps.scaling_matrix8, 16, sizeof(sps.scaling_matrix8));
    sps.scaling_matrix_present = 0;
    sps.colorspace = EColorSpace_UNSPECIFIED;
    if (sps.profile_idc == 100 || sps.profile_idc == 110 ||
        sps.profile_idc == 122 || sps.profile_idc == 244 ||
        sps.profile_idc ==  44 || sps.profile_idc ==  83 ||
        sps.profile_idc ==  86 || sps.profile_idc == 118 ||
        sps.profile_idc == 128 || sps.profile_idc == 144) {
      sps.chroma_format_idc = get_ue_golomb_31(&gb);
      if (sps.chroma_format_idc > 3) {
        ERROR("chroma_format_idc %d is illegal\n", sps.chroma_format_idc);
        ret = false;
        break;
      } else if (sps.chroma_format_idc == 3) {
        sps.residual_color_transform_flag = get_bits1(&gb);
        if (sps.residual_color_transform_flag) {
          ERROR("separate color planes are not supported\n");
          ret = false;
          break;
        }
      }
      sps.bit_depth_luma   = get_ue_golomb(&gb) + 8;
      sps.bit_depth_chroma = get_ue_golomb(&gb) + 8;
      if (sps.bit_depth_luma > 14 || sps.bit_depth_chroma > 14 ||
          sps.bit_depth_luma != sps.bit_depth_chroma) {
        ERROR("illegal bit depth value (%d, %d)\n",
               sps.bit_depth_luma, sps.bit_depth_chroma);
        ret = false;
        break;
      }
      sps.transform_bypass = get_bits1(&gb);
      decode_scaling_matrices(gb, &sps, NULL, 1,
                              sps.scaling_matrix4, sps.scaling_matrix8);
    } else {
      sps.chroma_format_idc = 1;
      sps.bit_depth_luma    = 8;
      sps.bit_depth_chroma  = 8;
    }

    log2_max_frame_num_minus4 = get_ue_golomb(&gb);
    if (log2_max_frame_num_minus4 < MIN_LOG2_MAX_FRAME_NUM - 4 ||
        log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM - 4) {
      ERROR("log2_max_frame_num_minus4 out of range (0-12): %d\n",
             log2_max_frame_num_minus4);
      ret = false;
      break;
    }
    sps.log2_max_frame_num = log2_max_frame_num_minus4 + 4;

    sps.poc_type = get_ue_golomb_31(&gb);

    if (sps.poc_type == 0) { // FIXME #define
      uint32_t t = get_ue_golomb(&gb);
      if (t>12) {
        ERROR("log2_max_poc_lsb (%d) is out of range\n", t);
        ret = false;
        break;
      }
      sps.log2_max_poc_lsb = t + 4;
    } else if (sps.poc_type == 1) { // FIXME #define
      sps.delta_pic_order_always_zero_flag = get_bits1(&gb);
      sps.offset_for_non_ref_pic           = get_se_golomb(&gb);
      sps.offset_for_top_to_bottom_field   = get_se_golomb(&gb);
      sps.poc_cycle_length                 = get_ue_golomb(&gb);
      if ((uint32_t)sps.poc_cycle_length >=
          FF_ARRAY_ELEMS(sps.offset_for_ref_frame)) {
        ERROR("poc_cycle_length overflow %u\n", sps.poc_cycle_length);
        ret = false;
        break;
      }
      for (i = 0; i < sps.poc_cycle_length; ++ i)
        sps.offset_for_ref_frame[i] = get_se_golomb(&gb);
    } else if (sps.poc_type != 2) {
      ERROR("illegal POC type %d\n", sps.poc_type);
      ret = false;
      break;
    }
    sps.ref_frame_count = get_ue_golomb_31(&gb);
//    if (h->avctx->codec_tag == MKTAG('S', 'M', 'V', '2'))
//      sps->ref_frame_count = FFMAX(2, sps->ref_frame_count);
//    if (sps->ref_frame_count > MAX_PICTURE_COUNT - 2 ||
//        sps->ref_frame_count > 16U) {
//      av_log(h->avctx, AV_LOG_ERROR, "too many reference frames\n");
//      goto fail;
//    }
    sps.gaps_in_frame_num_allowed_flag = get_bits1(&gb);
    sps.mb_width                       = get_ue_golomb(&gb) + 1;
    sps.mb_height                      = get_ue_golomb(&gb) + 1;
    if ((uint32_t)sps.mb_width  >= INT_MAX / 16 ||
        (uint32_t)sps.mb_height >= INT_MAX / 16 ||
        !av_image_check_size(16 * sps.mb_width,
                            16 * sps.mb_height, 0)) {
      ERROR("mb_width/height overflow\n");
      ret = false;
      break;
    }
    sps.frame_mbs_only_flag = get_bits1(&gb);
    if (!sps.frame_mbs_only_flag) {
      sps.mb_aff = get_bits1(&gb);
    } else {
      sps.mb_aff = 0;
    }
    sps.direct_8x8_inference_flag = get_bits1(&gb);
    sps.crop = get_bits1(&gb);
    if (sps.crop) {
      int32_t crop_left   = get_ue_golomb(&gb);
      int32_t crop_right  = get_ue_golomb(&gb);
      int32_t crop_top    = get_ue_golomb(&gb);
      int32_t crop_bottom = get_ue_golomb(&gb);
      int32_t width  = 16 * sps.mb_width;
      int32_t height = 16 * sps.mb_height * (2 - sps.frame_mbs_only_flag);
        int32_t vsub   = (sps.chroma_format_idc == 1) ? 1 : 0;
        int32_t hsub   = (sps.chroma_format_idc == 1 ||
            sps.chroma_format_idc == 2) ? 1 : 0;
        int32_t step_x = 1 << hsub;
        int32_t step_y = (2 - sps.frame_mbs_only_flag) << vsub;
//        if (crop_left & (0x1F >> (sps.bit_depth_luma > 8)) &&
//            !(h->avctx->flags & CODEC_FLAG_UNALIGNED)) {
//          crop_left &= ~(0x1F >> (sps->bit_depth_luma > 8));
//          av_log(h->avctx, AV_LOG_WARNING,
//                 "Reducing left cropping to %d "
//                 "chroma samples to preserve alignment.\n",
//                 crop_left);
//        }
        if (crop_left  > INT_MAX / 4 / step_x ||
            crop_right > INT_MAX / 4 / step_x ||
            crop_top   > INT_MAX / 4 / step_y ||
            crop_bottom> INT_MAX / 4 / step_y ||
            (crop_left + crop_right ) * step_x >= width ||
            (crop_top  + crop_bottom) * step_y >= height
        ) {
          ERROR("crop values invalid crop_left :%d  crop_right : %d  "
              "crop_top : %d  crop_bottom : %d  width : %d  height : %d"
              "  step_x : %d   step_y : %d\n",
                crop_left, crop_right, crop_top, crop_bottom, width, height,
                step_x, step_y);
          ret = false;
          break;
        }

        sps.crop_left   = crop_left   * step_x;
        sps.crop_right  = crop_right  * step_x;
        sps.crop_top    = crop_top    * step_y;
        sps.crop_bottom = crop_bottom * step_y;
    } else {
      sps.crop_left = sps.crop_right = sps.crop_top = sps.crop_bottom =
      sps.crop = 0;
    }

    sps.vui_parameters_present_flag = get_bits1(&gb);
    if (sps.vui_parameters_present_flag) {
      if (!decode_h264_vui_parameters(gb, &sps)) {
        ERROR("Faield to decode h264_vui_parameters");
        ret = false;
        break;
      }
    }
    if (!sps.sar_den) {
      sps.sar_den = 1;
    }
    static const char csp[4][5] = { "Gray", "420", "422", "444" };
    NOTICE("\n                          sps: %u"
           "\n                  profile_icd: %d"
           "\n                    level_idc: %d"
           "\n                          poc: %d"
           "\n              ref_frame_count: %d"
           "\n         mb_width x mb_height: %d x %d"
           "\n          frame_mbs_only_flag: %s"
           "\n    direct_8x8_inference_flag: %s"
           "\n   crop_left/right/top/bottom: %u/%u/%u/%u"
           "\n  vui_parameters_present_flag: %s"
           "\n   csp[sps.chroma_format_idc]: %s"
           "\n num_units_in_tick/time_scale: %u/%u"
           "\n               bit_depth_luma: %d "
           "\n           num_reorder_frames: %d",
         sps_id, sps.profile_idc, sps.level_idc, sps.poc_type,
         sps.ref_frame_count,
         sps.mb_width, sps.mb_height,
         sps.frame_mbs_only_flag ? "FRM" : (sps.mb_aff ? "MB-AFF" : "PIC-AFF"),
         sps.direct_8x8_inference_flag ? "8B8" : "",
         sps.crop_left, sps.crop_right,
         sps.crop_top, sps.crop_bottom,
         sps.vui_parameters_present_flag ? "VUI" : "",
         csp[sps.chroma_format_idc],
         sps.timing_info_present_flag ? sps.num_units_in_tick : 0,
         sps.timing_info_present_flag ? sps.time_scale : 0,
         sps.bit_depth_luma,
         sps.bitstream_restriction_flag ? sps.num_reorder_frames : -1
      );
    sps.isnew = 1;
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::parse_avc_pps(uint8_t *p_data, uint32_t data_size,
                               AVCPPS& pps, AVCSPS& sps)
{
  bool ret = true;
  do {
    GetBitContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = data_size * 8;
    get_bits(&gb, 8);
    uint32_t pps_id = get_ue_golomb(&gb);
    int32_t qp_bd_offset;
    int32_t bits_left;
    if (pps_id >= MAX_PPS_COUNT) {
      ERROR("pps_id (%d) out of range\n", pps_id);
      ret = false;
      break;
    }
    pps.sps_id = get_ue_golomb_31(&gb);
    if ((uint32_t)pps.sps_id >= MAX_SPS_COUNT) {
      ERROR("sps_id out of range\n");
      ret = false;
      break;
    }
    qp_bd_offset = 6 * (sps.bit_depth_luma - 8);
    if (sps.bit_depth_luma > 14) {
      ERROR("Invalid luma bit depth=%d\n",sps.bit_depth_luma);
      ret = false;
      break;
    } else if (sps.bit_depth_luma == 11 || sps.bit_depth_luma == 13) {
      ERROR("Unimplemented luma bit depth=%d\n", sps.bit_depth_luma);
      ret = false;
      break;
    }
    pps.cabac             = get_bits1(&gb);
    pps.pic_order_present = get_bits1(&gb);
    pps.slice_group_count = get_ue_golomb(&gb) + 1;
    if (pps.slice_group_count > 1) {
      pps.mb_slice_group_map_type = get_ue_golomb(&gb);
      WARN("FMO not supported\n");
    }
    pps.ref_count[0] = get_ue_golomb(&gb) + 1;
    pps.ref_count[1] = get_ue_golomb(&gb) + 1;
    if (pps.ref_count[0] - 1 > 32 - 1 || pps.ref_count[1] - 1 > 32 - 1) {
      ERROR("reference overflow (pps)\n");
      ret = false;
      break;
    }
    pps.weighted_pred                 = get_bits1(&gb);
    pps.weighted_bipred_idc           = get_bits(&gb, 2);
    pps.init_qp                       = get_se_golomb(&gb) + 26 + qp_bd_offset;
    pps.init_qs                       = get_se_golomb(&gb) + 26 + qp_bd_offset;
    pps.chroma_qp_index_offset[0]     = get_se_golomb(&gb);
    pps.deblocking_filter_parameters_present = get_bits1(&gb);
    pps.constrained_intra_pred               = get_bits1(&gb);
    pps.redundant_pic_cnt_present            = get_bits1(&gb);

    pps.transform_8x8_mode = 0;
    // contents of sps/pps can change even if id doesn't, so reinit
    memcpy(pps.scaling_matrix4, sps.scaling_matrix4,
           sizeof(pps.scaling_matrix4));
    memcpy(pps.scaling_matrix8, sps.scaling_matrix8,
           sizeof(pps.scaling_matrix8));
    bits_left = gb.size_in_bits - get_bit_count(&gb);
    if (bits_left > 0 && more_rbsp_data_in_pps(gb, &sps, &pps)) {
      pps.transform_8x8_mode = get_bits1(&gb);
      decode_scaling_matrices(gb, &sps, &pps, 0,
                              pps.scaling_matrix4, pps.scaling_matrix8);
      // second_chroma_qp_index_offset
      pps.chroma_qp_index_offset[1] = get_se_golomb(&gb);
    } else {
      pps.chroma_qp_index_offset[1] = pps.chroma_qp_index_offset[0];
    }

    build_qp_table(&pps, 0, pps.chroma_qp_index_offset[0], sps.bit_depth_luma);
    build_qp_table(&pps, 1, pps.chroma_qp_index_offset[1], sps.bit_depth_luma);
    if (pps.chroma_qp_index_offset[0] != pps.chroma_qp_index_offset[1])
      pps.chroma_qp_diff = 1;
      NOTICE("\n          pps: %u"
             "\n          sps: %u %s"
             "\n slice_groups: %d"
             "\n          ref: %u/%u %s"
             "\n           qp: %d/%d/%d/%d %s %s %s %s",
             pps_id,
             pps.sps_id, pps.cabac ? "CABAC" : "CAVLC",
             pps.slice_group_count,
             pps.ref_count[0], pps.ref_count[1],
             pps.weighted_pred ? "weighted" : "",
             pps.init_qp, pps.init_qs, pps.chroma_qp_index_offset[0],
             pps.chroma_qp_index_offset[1],
             pps.deblocking_filter_parameters_present ? "LPAR" : "",
             pps.constrained_intra_pred ? "CONSTR" : "",
             pps.redundant_pic_cnt_present ? "REDU" : "",
             pps.transform_8x8_mode ? "8x8DCT" : "");
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::decode_profile_tier_level(
    GetBitContext& gb, PTL& ptl, int32_t max_num_sub_layers)
{
  bool ret = true;
  do {
    ptl.general_profile_space = get_bits(&gb, 2);
    ptl.general_tier_flag = get_bits1(&gb);
    ptl.general_profile_idc = get_bits(&gb, 5);
    for(int i = 0; i < 32; ++ i) {
      ptl.general_profile_compatibility_flag[i] = get_bits1(&gb);
    }
    ptl.general_progressive_source_flag = get_bits1(&gb);
    ptl.general_interlaced_source_flag = get_bits1(&gb);
    ptl.general_non_packed_constraint_flag = get_bits1(&gb);
    ptl.general_frame_only_constraint_flag = get_bits1(&gb);
    // XXX_reserved_zero_43bits[0..23]
    skip_bits(&gb, 16);
    skip_bits(&gb, 16);
    skip_bits(&gb, 12);
//    uint32_t tmp;
//    if((tmp = get_bits(&gb, 24)) != 0) {
//      ERROR("reserved zero 24 bits is not zero: %08X.", tmp);
//      ret = false;
//      break;
//    }
//    // XXX_reserved_zero_44bits[24..43]
//    if((tmp = get_bits(&gb, 19)) != 0) {
//      ERROR("reserved zero 19 bits is not zero: %08x.", tmp);
//      ret = false;
//      break;
//    }
//    get_bits(&gb, 1);
    ptl.general_level_idc = get_bits(&gb, 8);
    for (int i = 0; i < max_num_sub_layers - 1; ++ i) {
      ptl.sub_layer_profile_present_flag[i] = get_bits1(&gb);
      ptl.sub_layer_level_present_flag[i] = get_bits1(&gb);
    }
    if (max_num_sub_layers - 1 > 0) {
      for (int i = max_num_sub_layers - 1; i < 8; ++ i) {
        skip_bits(&gb, 2); // reserved_zero_2bits[i]
      }
    }
    for (int i = 0; i < max_num_sub_layers - 1; ++ i) {
      if (ptl.sub_layer_profile_present_flag[i]) {
        ptl.sub_layer_profile_space[i] = get_bits(&gb, 2);
        ptl.sub_layer_tier_flag[i] = get_bits(&gb, 1);
        ptl.sub_layer_profile_idc[i] = get_bits(&gb, 5);
        for (int j = 0; j < 32; ++ j) {
          ptl.sub_layer_profile_compatibility_flags[i][j] = get_bits1(&gb);
        }
        skip_bits(&gb, 1);// sub_layer_progressive_source_flag
        skip_bits(&gb, 1);// sub_layer_interlaced_source_flag
        skip_bits(&gb, 1);// sub_layer_non_packed_constraint_flag
        skip_bits(&gb, 1);// sub_layer_frame_only_constraint_flag
        skip_bits(&gb, 16);
        skip_bits(&gb, 16);
        skip_bits(&gb, 12);
//        if (get_bits(&gb, 16) != 0) {// sub_layer_reserved_zero_44bits[0..15]
//          ret = false;
//          break;
//        }
//        if (get_bits(&gb, 16) != 0) {// sub_layer_reserved_zero_44bits[16..31]
//          ret = false;
//          break;
//        }
//        if (get_bits(&gb, 12) != 0) {// sub_layer_reserved_zero_44bits[32..43]
//          ret = false;
//          break;
//        }
      }
      if (ptl.sub_layer_level_present_flag[i]) {
        ptl.sub_layer_level_idc[i] = get_bits(&gb, 8);
      }
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::decode_hrd(GetBitContext& gb,
                                      int32_t common_inf_present,
                                      int32_t max_sublayers)
{
  bool ret = false;
  do {
    int nal_params_present = 0, vcl_params_present = 0;
    int subpic_params_present = 0;
    int i;
    if (common_inf_present) {
      nal_params_present = get_bits1(&gb);
      vcl_params_present = get_bits1(&gb);
      if (nal_params_present || vcl_params_present) {
        subpic_params_present = get_bits1(&gb);
        if (subpic_params_present) {
          skip_bits(&gb, 8); // tick_divisor_minus2
          skip_bits(&gb, 5); // du_cpb_removal_delay_increment_length_minus1
          skip_bits(&gb, 1); // sub_pic_cpb_params_in_pic_timing_sei_flag
          skip_bits(&gb, 5); // dpb_output_delay_du_length_minus1
        }
        skip_bits(&gb, 4); // bit_rate_scale
        skip_bits(&gb, 4); // cpb_size_scale
        if (subpic_params_present) {
          skip_bits(&gb, 4); // cpb_size_du_scale
        }
        skip_bits(&gb, 5); // initial_cpb_removal_delay_length_minus1
        skip_bits(&gb, 5); // au_cpb_removal_delay_length_minus1
        skip_bits(&gb, 5); // dpb_output_delay_length_minus1
      }
    }
    for (i = 0; i < max_sublayers; i++) {
      int low_delay = 0;
      int nb_cpb = 1;
      int fixed_rate = get_bits1(&gb);
      if (!fixed_rate) {
        fixed_rate = get_bits1(&gb);
      }
      if (fixed_rate) {
        get_ue_golomb_long(&gb); // elemental_duration_in_tc_minus1
      } else {
          low_delay = get_bits1(&gb);
      }
      if (!low_delay) {
        nb_cpb = get_ue_golomb_long(&gb) + 1;
      }
      if (nal_params_present) {
        if(!decode_sublayer_hrd(gb, nb_cpb, subpic_params_present)) {
          ERROR("Failed to decode sublayer hrd.");
          ret = false;
          break;
        }
      }
      if (vcl_params_present) {
        if(!decode_sublayer_hrd(gb, nb_cpb, subpic_params_present)) {
          ERROR("Failed to decode sublayer hrd.");
          ret = false;
          break;
        }
      }
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::decode_sublayer_hrd(GetBitContext& gb,
                              int32_t nb_cpb, int32_t subpic_params_present)
{
  bool ret = true;
  do {
    int i;
    for (i = 0; i < nb_cpb; i++) {
      get_ue_golomb_long(&gb); // bit_rate_value_minus1
      get_ue_golomb_long(&gb); // cpb_size_value_minus1
      if (subpic_params_present) {
        get_ue_golomb_long(&gb); // cpb_size_du_value_minus1
        get_ue_golomb_long(&gb); // bit_rate_du_value_minus1
      }
      skip_bits(&gb, 1); // cbr_flag
    }
  } while(0);
  return ret;
}
void AMVideoParamSetsParser::set_default_scaling_list_data(ScalingList *sl)
{
  int matrixId;

  for (matrixId = 0; matrixId < 6; matrixId++) {
    // 4x4 default is 16
    memset(sl->sl[0][matrixId], 16, 16);
    sl->sl_dc[0][matrixId] = 16; // default for 16x16
    sl->sl_dc[1][matrixId] = 16; // default for 32x32
  }
  memcpy(sl->sl[1][0], default_scaling_list_intra, 64);
  memcpy(sl->sl[1][1], default_scaling_list_intra, 64);
  memcpy(sl->sl[1][2], default_scaling_list_intra, 64);
  memcpy(sl->sl[1][3], default_scaling_list_inter, 64);
  memcpy(sl->sl[1][4], default_scaling_list_inter, 64);
  memcpy(sl->sl[1][5], default_scaling_list_inter, 64);
  memcpy(sl->sl[2][0], default_scaling_list_intra, 64);
  memcpy(sl->sl[2][1], default_scaling_list_intra, 64);
  memcpy(sl->sl[2][2], default_scaling_list_intra, 64);
  memcpy(sl->sl[2][3], default_scaling_list_inter, 64);
  memcpy(sl->sl[2][4], default_scaling_list_inter, 64);
  memcpy(sl->sl[2][5], default_scaling_list_inter, 64);
  memcpy(sl->sl[3][0], default_scaling_list_intra, 64);
  memcpy(sl->sl[3][1], default_scaling_list_inter, 64);
}

bool AMVideoParamSetsParser::scaling_list_data(GetBitContext& gb, ScalingList *sl)
{
  bool ret = true;
  do {
    uint8_t scaling_list_pred_mode_flag[4][6];
    int32_t scaling_list_dc_coef[2][6];
    int size_id, matrix_id, i, pos, delta;
    for (size_id = 0; size_id < 4; size_id++) {
      for (matrix_id = 0; matrix_id < ((size_id == 3) ? 2 : 6); matrix_id++) {
        scaling_list_pred_mode_flag[size_id][matrix_id] = get_bits1(&gb);
        if (!scaling_list_pred_mode_flag[size_id][matrix_id]) {
          delta = get_ue_golomb_long(&gb);
          /* Only need to handle non-zero delta. Zero means default,
        which should already be in the arrays.*/
          if (delta) {
            // Copy from previous array.
            if (matrix_id - delta < 0) {
              ERROR("Invalid delta in scaling list data: %d.\n", delta);
              ret = false;
              break;
            }
            memcpy(sl->sl[size_id][matrix_id], sl->sl[size_id][matrix_id - delta],
                   size_id > 0 ? 64 : 16);
            if (size_id > 1) {
              sl->sl_dc[size_id - 2][matrix_id] =
                  sl->sl_dc[size_id - 2][matrix_id - delta];
            }
          }
        } else {
          int next_coef;
          int coef_num;
          int32_t scaling_list_delta_coef;
          next_coef = 8;
          coef_num = FFMIN(64, (1  <<  (4 + (size_id  <<  1))));
          if (size_id > 1) {
            scaling_list_dc_coef[size_id - 2][matrix_id] = get_se_golomb(&gb) + 8;
            next_coef = scaling_list_dc_coef[size_id - 2][matrix_id];
            sl->sl_dc[size_id - 2][matrix_id] = next_coef;
          }
          for (i = 0; i < coef_num; i++) {
            if (size_id == 0) {
              pos = 4 * ff_hevc_diag_scan4x4_y[i] + ff_hevc_diag_scan4x4_x[i];
            } else {
              pos = 8 * ff_hevc_diag_scan8x8_y[i] + ff_hevc_diag_scan8x8_x[i];
            }
            scaling_list_delta_coef = get_se_golomb(&gb);
            next_coef = (next_coef + scaling_list_delta_coef + 256 ) % 256;
            sl->sl[size_id][matrix_id][pos] = next_coef;
          }
        }
      }
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::hevc_decode_short_term_rps(GetBitContext& gb,
                 ShortTermRPS *rps, const HEVCSPS *sps, int is_slice_header)
{
  bool ret = true;
  do {
    uint8_t rps_predict = 0;
    int delta_poc;
    int k0 = 0;
    int k1 = 0;
    int k  = 0;
    int i;
    if (rps != sps->st_rps && sps->nb_st_rps) {
      rps_predict  = get_bits1(&gb);
    }
    if (rps_predict) {
      const ShortTermRPS *rps_ridx;
      int delta_rps, abs_delta_rps;
      uint8_t use_delta_flag = 0;
      uint8_t delta_rps_sign;
      if (is_slice_header) {
        int delta_idx = get_ue_golomb_long(&gb) + 1;
        if (delta_idx > (int32_t)sps->nb_st_rps) {
          ERROR("Invalid value of delta_idx "
              "in slice header RPS: %d > %d.\n", delta_idx, sps->nb_st_rps);
          ret = false;
          break;
        }
        rps_ridx = &sps->st_rps[sps->nb_st_rps - delta_idx];
      } else {
        rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];
      }
      delta_rps_sign = get_bits1(&gb);
      abs_delta_rps  = get_ue_golomb_long(&gb) + 1;
      delta_rps      = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
      for (i = 0; i <= rps_ridx->num_delta_pocs; ++ i) {
        int used = rps->used[k] = get_bits1(&gb);
        if (!used) {
          use_delta_flag = get_bits1(&gb);
        }
        if (used || use_delta_flag) {
          if (i < rps_ridx->num_delta_pocs) {
            delta_poc = delta_rps + rps_ridx->delta_poc[i];
          } else {
            delta_poc = delta_rps;
          }
          rps->delta_poc[k] = delta_poc;
          if (delta_poc < 0) {
            k0++;
          } else {
            k1++;
          }
          k++;
        }
      }
      rps->num_delta_pocs    = k;
      rps->num_negative_pics = k0;
      // sort in increasing order (smallest first)
      if (rps->num_delta_pocs != 0) {
        int used, tmp;
        for (i = 1; i < rps->num_delta_pocs; ++ i) {
          delta_poc = rps->delta_poc[i];
          used      = rps->used[i];
          for (k = i-1 ; k >= 0;  k--) {
            tmp = rps->delta_poc[k];
            if (delta_poc < tmp ) {
              rps->delta_poc[k+1] = tmp;
              rps->used[k+1]      = rps->used[k];
              rps->delta_poc[k]   = delta_poc;
              rps->used[k]        = used;
            }
          }
        }
      }
      if ((rps->num_negative_pics >> 1) != 0) {
        int used;
        k = rps->num_negative_pics - 1;
        // flip the negative values to largest first
        for (i = 0; i < rps->num_negative_pics>>1; ++ i) {
          delta_poc          = rps->delta_poc[i];
          used               = rps->used[i];
          rps->delta_poc[i]  = rps->delta_poc[k];
          rps->used[i]       = rps->used[k];
          rps->delta_poc[k]  = delta_poc;
          rps->used[k]       = used;
          k--;
        }
      }
    } else {
      uint32_t prev, nb_positive_pics;
      rps->num_negative_pics = get_ue_golomb_long(&gb);
      nb_positive_pics       = get_ue_golomb_long(&gb);

      if (rps->num_negative_pics >= MAX_REFS ||
          nb_positive_pics >= MAX_REFS) {
        ERROR("Too many refs in a short term RPS.\n");
        ret = false;
        break;
      }
      rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
      if (rps->num_delta_pocs) {
        prev = 0;
        for (i = 0; i < rps->num_negative_pics; ++ i) {
          delta_poc = get_ue_golomb_long(&gb) + 1;
          prev -= delta_poc;
          rps->delta_poc[i] = prev;
          rps->used[i] = get_bits1(&gb);
        }
        prev = 0;
        for (uint32_t j = 0; j < nb_positive_pics; ++ j) {
          delta_poc = get_ue_golomb_long(&gb) + 1;
          prev += delta_poc;
          rps->delta_poc[rps->num_negative_pics + j] = prev;
          rps->used[rps->num_negative_pics + j] = get_bits1(&gb);
        }
      }
    }
  } while(0);
  return ret;
}

void AMVideoParamSetsParser::decode_vui(GetBitContext& gb, HEVCSPS *sps)
{
  VUI *vui = &sps->vui;
  int sar_present;
  INFO("Decoding VUI\n");
  sar_present = get_bits1(&gb);
  if (sar_present) {
    uint8_t sar_idx = get_bits(&gb, 8);
    if (sar_idx < FF_ARRAY_ELEMS(vui_sar)) {
      vui->sar_num = vui_sar[sar_idx].num;
      vui->sar_den = vui_sar[sar_idx].den;
    } else if (sar_idx == 255) {
      vui->sar_num = get_bits(&gb, 16);
      vui->sar_den = get_bits(&gb, 16);
    } else {
      WARN("Unknown SAR index: %u.\n", sar_idx);
    }
  }
  vui->overscan_info_present_flag = get_bits1(&gb);
  if (vui->overscan_info_present_flag) {
    vui->overscan_appropriate_flag = get_bits1(&gb);
  }
  vui->video_signal_type_present_flag = get_bits1(&gb);
  if (vui->video_signal_type_present_flag) {
    vui->video_format                    = get_bits(&gb, 3);
    vui->video_full_range_flag           = get_bits1(&gb);
    vui->colour_description_present_flag = get_bits1(&gb);
    if (vui->colour_description_present_flag) {
      vui->colour_primaries        = get_bits(&gb, 8);
      vui->transfer_characteristic = get_bits(&gb, 8);
      vui->matrix_coeffs           = get_bits(&gb, 8);
    }
  }
  vui->chroma_loc_info_present_flag = get_bits1(&gb);
  if (vui->chroma_loc_info_present_flag) {
    vui->chroma_sample_loc_type_top_field    = get_ue_golomb_long(&gb);
    vui->chroma_sample_loc_type_bottom_field = get_ue_golomb_long(&gb);
  }
  vui->neutra_chroma_indication_flag = get_bits1(&gb);
  vui->field_seq_flag                = get_bits1(&gb);
  vui->frame_field_info_present_flag = get_bits1(&gb);
  vui->default_display_window_flag = get_bits1(&gb);
  if (vui->default_display_window_flag) {
    //TODO: * 2 is only valid for 420
    vui->def_disp_win.left_offset   = get_ue_golomb_long(&gb) * 2;
    vui->def_disp_win.right_offset  = get_ue_golomb_long(&gb) * 2;
    vui->def_disp_win.top_offset    = get_ue_golomb_long(&gb) * 2;
    vui->def_disp_win.bottom_offset = get_ue_golomb_long(&gb) * 2;
//    if (s->strict_def_disp_win &&
//        s->avctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {
//      av_log(s->avctx, AV_LOG_DEBUG,
//             "discarding vui default display window, "
//             "original values are l:%u r:%u t:%u b:%u\n",
//             vui->def_disp_win.left_offset,
//             vui->def_disp_win.right_offset,
//             vui->def_disp_win.top_offset,
//             vui->def_disp_win.bottom_offset);
//
//      vui->def_disp_win.left_offset   =
//          vui->def_disp_win.right_offset  =
//              vui->def_disp_win.top_offset    =
//                  vui->def_disp_win.bottom_offset = 0;
//    }
  }
  vui->vui_timing_info_present_flag = get_bits1(&gb);
  if (vui->vui_timing_info_present_flag) {
    vui->vui_num_units_in_tick               = get_bits(&gb, 32);
    vui->vui_time_scale                      = get_bits(&gb, 32);
    vui->vui_poc_proportional_to_timing_flag = get_bits1(&gb);
    if (vui->vui_poc_proportional_to_timing_flag) {
      vui->vui_num_ticks_poc_diff_one_minus1 = get_ue_golomb_long(&gb);
    }
    vui->vui_hrd_parameters_present_flag = get_bits1(&gb);
    if (vui->vui_hrd_parameters_present_flag)
      decode_hrd(gb, 1, sps->max_sub_layers);
  }
  vui->bitstream_restriction_flag = get_bits1(&gb);
  if (vui->bitstream_restriction_flag) {
    vui->tiles_fixed_structure_flag              = get_bits1(&gb);
    vui->motion_vectors_over_pic_boundaries_flag = get_bits1(&gb);
    vui->restricted_ref_pic_lists_flag           = get_bits1(&gb);
    vui->min_spatial_segmentation_idc            = get_ue_golomb_long(&gb);
    vui->max_bytes_per_pic_denom                 = get_ue_golomb_long(&gb);
    vui->max_bits_per_min_cu_denom               = get_ue_golomb_long(&gb);
    vui->log2_max_mv_length_horizontal           = get_ue_golomb_long(&gb);
    vui->log2_max_mv_length_vertical             = get_ue_golomb_long(&gb);
  }
}

void AMVideoParamSetsParser::decode_scaling_list(GetBitContext& gb,
                                uint8_t *factors, int size,
                                const uint8_t *jvt_list,
                                const uint8_t *fallback_list)
{
  int i, last = 8, next = 8;
  const uint8_t *scan = size == 16 ? zigzag_scan : ff_zigzag_direct;
  if (!get_bits1(&gb)) /* matrix not written, we use the predicted one */
    memcpy(factors, fallback_list, size * sizeof(uint8_t));
  else
    for (i = 0; i < size; i++) {
      if (next)
        next = (last + get_se_golomb(&gb)) & 0xff;
      if (!i && !next) { /* matrix not written, we use the preset one */
        memcpy(factors, jvt_list, size * sizeof(uint8_t));
        break;
      }
      last = factors[scan[i]] = next ? next : last;
    }
}

bool AMVideoParamSetsParser::av_image_check_size(unsigned int w,
                                                 unsigned int h, int log_offset)
{
  if((int)w>0 && (int)h>0 && (w+128)*(uint64_t)(h+128) < INT_MAX/8) {
    return true;
  } else {
    return false;
  }
}

bool AMVideoParamSetsParser::decode_h264_vui_parameters(GetBitContext& gb,
                                                        AVCSPS* sps)
{
  bool ret = true;
  do {
    int32_t aspect_ratio_info_present_flag;
    uint32_t aspect_ratio_idc;
    aspect_ratio_info_present_flag = get_bits1(&gb);
    if (aspect_ratio_info_present_flag) {
      aspect_ratio_idc = get_bits(&gb, 8);
      if (aspect_ratio_idc == EXTENDED_SAR) {
        sps->sar_num = get_bits(&gb, 16);
        sps->sar_den = get_bits(&gb, 16);
      } else if (aspect_ratio_idc < FF_ARRAY_ELEMS(pixel_aspect)) {
        sps->sar_num = pixel_aspect[aspect_ratio_idc].num;
        sps->sar_den = pixel_aspect[aspect_ratio_idc].den;
      } else {
        ERROR("illegal aspect ratio\n");
        ret = false;
        break;
      }
    } else {
      sps->sar_num = sps->sar_den = 0;
    }
    if (get_bits1(&gb))      /* overscan_info_present_flag */
      get_bits1(&gb);      /* overscan_appropriate_flag */
    sps->video_signal_type_present_flag = get_bits1(&gb);
    if (sps->video_signal_type_present_flag) {
      get_bits(&gb, 3);                 /* video_format */
      sps->full_range = get_bits1(&gb); /* video_full_range_flag */
      sps->colour_description_present_flag = get_bits1(&gb);
      if (sps->colour_description_present_flag) {
        /* colour_primaries */
        sps->color_primaries = EColorPrimaries(get_bits(&gb, 8));
        /* transfer_characteristics */
        sps->color_trc  = EColorTransferCharacteristic(get_bits(&gb, 8));
        sps->colorspace = EColorSpace(get_bits(&gb, 8)); /* matrix_coefficients */
        if (sps->color_primaries >= EColorPrimaries_NB)
          sps->color_primaries = EColorPrimaries_Unspecified;
        if (sps->color_trc >= EColorTransferCharacteristic_NB)
          sps->color_trc = EColorTransferCharacteristic_UNSPECIFIED;
        if (sps->colorspace >= EColorSpace_NB)
          sps->colorspace = EColorSpace_UNSPECIFIED;
      }
    }
    /* chroma_location_info_present_flag */
    if (get_bits1(&gb)) {
      /* chroma_sample_location_type_top_field */
      get_ue_golomb(&gb);
      get_ue_golomb(&gb);  /* chroma_sample_location_type_bottom_field */
    }
    if (show_bits(&gb, 1) && get_bits_left(&gb) < 10) {
      WARN("Truncated VUI\n");
    }
    sps->timing_info_present_flag = get_bits1(&gb);
    if (sps->timing_info_present_flag) {
      sps->num_units_in_tick = get_bits_long(&gb, 32);
      sps->time_scale        = get_bits_long(&gb, 32);
      if (!sps->num_units_in_tick || !sps->time_scale) {
        ERROR("time_scale/num_units_in_tick invalid or unsupported (%d/%d)\n",
               sps->time_scale, sps->num_units_in_tick);
        ret = false;
        break;
      }
      sps->fixed_frame_rate_flag = get_bits1(&gb);
    }
    sps->nal_hrd_parameters_present_flag = get_bits1(&gb);
    if (sps->nal_hrd_parameters_present_flag) {
      if (!decode_h264_hrd_parameters(gb, sps)) {
        ret = false;
        ERROR("Failed to decode h264 hrd parameters");
        break;
      }
    }
    sps->vcl_hrd_parameters_present_flag = get_bits1(&gb);
    if (sps->vcl_hrd_parameters_present_flag) {
      if (!decode_h264_hrd_parameters(gb, sps)) {
        ret = false;
        ERROR("Failed to decode h264 hrd parameters");
        break;
      }
    }
    if (sps->nal_hrd_parameters_present_flag ||
        sps->vcl_hrd_parameters_present_flag) {
      get_bits1(&gb);     /* low_delay_hrd_flag */
    }
    sps->pic_struct_present_flag = get_bits1(&gb);
    if (!get_bits_left(&gb)) {
      WARN("get bits left error.");
      break;
    }
    sps->bitstream_restriction_flag = get_bits1(&gb);
    if (sps->bitstream_restriction_flag) {
      get_bits1(&gb);     /* motion_vectors_over_pic_boundaries_flag */
      get_ue_golomb(&gb); /* max_bytes_per_pic_denom */
      get_ue_golomb(&gb); /* max_bits_per_mb_denom */
      get_ue_golomb(&gb); /* log2_max_mv_length_horizontal */
      get_ue_golomb(&gb); /* log2_max_mv_length_vertical */
      sps->num_reorder_frames = get_ue_golomb(&gb);
      get_ue_golomb(&gb); /*max_dec_frame_buffering*/
      if (get_bits_left(&gb) < 0) {
        sps->num_reorder_frames         = 0;
        sps->bitstream_restriction_flag = 0;
      }
      /* max_dec_frame_buffering || max_dec_frame_buffering > 16 */
      if (sps->num_reorder_frames > 16) {
        ERROR("illegal num_reorder_frames %d\n", sps->num_reorder_frames);
        ret = false;
        break;
      }
    }
    if (get_bits_left(&gb) < 0) {
      ERROR("Overread VUI by %d bits\n", -get_bits_left(&gb));
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::decode_h264_hrd_parameters(GetBitContext& gb,
                                                        AVCSPS *sps)
{
  bool ret = true;
  do {
    int32_t cpb_count, i;
    cpb_count = get_ue_golomb_31(&gb) + 1;
    if (cpb_count > 32) {
      ERROR("cpb_count %d invalid\n", cpb_count);
      ret = false;
      break;
    }
    get_bits(&gb, 4); /* bit_rate_scale */
    get_bits(&gb, 4); /* cpb_size_scale */
    for (i = 0; i < cpb_count; i++) {
      get_ue_golomb_long(&gb); /* bit_rate_value_minus1 */
      get_ue_golomb_long(&gb); /* cpb_size_value_minus1 */
      get_bits1(&gb);          /* cbr_flag */
    }
    sps->initial_cpb_removal_delay_length = get_bits(&gb, 5) + 1;
    sps->cpb_removal_delay_length         = get_bits(&gb, 5) + 1;
    sps->dpb_output_delay_length          = get_bits(&gb, 5) + 1;
    sps->time_offset_length               = get_bits(&gb, 5);
    sps->cpb_cnt                          = cpb_count;
  } while(0);
  return ret;
}

bool AMVideoParamSetsParser::more_rbsp_data_in_pps(GetBitContext& gb,
                                                   AVCSPS *sps, AVCPPS* pps)
{
  int32_t profile_idc = sps->profile_idc;
  if(((profile_idc == 66) || (profile_idc == 77) || (profile_idc == 88)) &&
      (sps->constraint_set_flags & 7)) {
    NOTICE("current profile does not provide more RBSP data in PPS, skipping.");
    return false;
  }
  return true;
}

void AMVideoParamSetsParser::decode_scaling_matrices(GetBitContext& gb,
                             AVCSPS *sps,AVCPPS *pps, int32_t is_sps,
                             uint8_t(*scaling_matrix4)[16],
                             uint8_t(*scaling_matrix8)[64])
{
  int fallback_sps = !is_sps && sps->scaling_matrix_present;
  const uint8_t *fallback[4] = {
               fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
               fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
               fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
               fallback_sps ? sps->scaling_matrix8[3] : default_scaling8[1]
  };
  if (get_bits1(&gb)) {
    sps->scaling_matrix_present |= is_sps;
    decode_scaling_list(gb, scaling_matrix4[0], 16,
                        default_scaling4[0], fallback[0]);        // Intra, Y
    decode_scaling_list(gb, scaling_matrix4[1], 16,
                        default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
    decode_scaling_list(gb, scaling_matrix4[2], 16,
                        default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
    decode_scaling_list(gb, scaling_matrix4[3], 16,
                        default_scaling4[1], fallback[1]);        // Inter, Y
    decode_scaling_list(gb, scaling_matrix4[4], 16,
                        default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
    decode_scaling_list(gb, scaling_matrix4[5], 16,
                        default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
    if (is_sps || pps->transform_8x8_mode) {
      decode_scaling_list(gb, scaling_matrix8[0], 64,
                          default_scaling8[0], fallback[2]); // Intra, Y
      decode_scaling_list(gb, scaling_matrix8[3], 64,
                          default_scaling8[1], fallback[3]); // Inter, Y
      if (sps->chroma_format_idc == 3) {
        decode_scaling_list(gb, scaling_matrix8[1], 64,
                          default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
        decode_scaling_list(gb, scaling_matrix8[4], 64,
                          default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
        decode_scaling_list(gb, scaling_matrix8[2], 64,
                          default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
        decode_scaling_list(gb, scaling_matrix8[5], 64,
                          default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
      }
    }
  }
}

void AMVideoParamSetsParser::build_qp_table(AVCPPS *pps, int t, int index,
                                            const int depth)
{
  int i;
  const int max_qp = 51 + 6 * (depth - 8);
  for (i = 0; i < max_qp + 1; i++)
    pps->chroma_qp_table[t][i] =
        ff_h264_chroma_qp[depth - 8][av_clip(i + index, 0, max_qp)];
}

int32_t AMVideoParamSetsParser::av_clip(int32_t a, int32_t amin, int32_t amax)
{
  if(amin > amax) {
    ERROR("amax is smaller than amin");
  }
  if      (a < amin) return amin;
  else if (a > amax) return amax;
  else               return a;
}
