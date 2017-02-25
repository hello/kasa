/*******************************************************************************
 * am_video_param_sets_parser.h
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
#ifndef AM_VIDEO_PARAM_SETS_PARSER_H_
#define AM_VIDEO_PARAM_SETS_PARSER_H_
#include "am_video_param_sets_parser_if.h"
struct GetBitContext;
struct PTL;
struct ScalingList;
struct ShortTermRPS;
class AMVideoParamSetsParser : public AMIVideoParamSetsParser
{
  public :
    AMVideoParamSetsParser(){};
    virtual ~AMVideoParamSetsParser(){};
    virtual bool parse_hevc_vps(uint8_t *p_data, uint32_t data_size,
                                HEVCVPS& vps);
    virtual bool parse_hevc_sps(uint8_t *p_data, uint32_t data_size,
                                HEVCSPS& sps);
    /*must parse sps first*/
    virtual bool parse_hevc_pps(uint8_t *p_data, uint32_t data_size,
                                HEVCPPS& pps, HEVCSPS& sps);
    virtual bool parse_avc_sps(uint8_t *p_data, uint32_t data_size,
                               AVCSPS& sps);
    virtual bool parse_avc_pps(uint8_t *p_data, uint32_t data_size,
                               AVCPPS& pps, AVCSPS& sps);
  private :
    bool hevc_decode_short_term_rps(GetBitContext& gb, ShortTermRPS *rps,
                                       const HEVCSPS *sps, int is_slice_header);
    bool decode_profile_tier_level(GetBitContext& gb, PTL& ptl,
                                   int32_t max_sub_layers);
    bool decode_hrd(GetBitContext& gb, int32_t common_inf_present,
                    int32_t max_sublayers);
    bool decode_sublayer_hrd(GetBitContext& gb, int32_t nb_cpb,
                             int32_t subpic_params_present);
    void set_default_scaling_list_data(ScalingList *sl);
    bool scaling_list_data(GetBitContext& gb, ScalingList *sl);
    void decode_vui(GetBitContext& gb, HEVCSPS *sps);
    void decode_scaling_list(GetBitContext& gb, uint8_t *factors, int size,
                                    const uint8_t *jvt_list,
                                    const uint8_t *fallback_list);
    bool av_image_check_size(unsigned int w, unsigned int h, int log_offset);
    bool decode_h264_vui_parameters(GetBitContext& gb, AVCSPS* sps);
    bool decode_h264_hrd_parameters(GetBitContext& gb, AVCSPS *sps);
    bool more_rbsp_data_in_pps(GetBitContext& gb, AVCSPS *sps, AVCPPS* pps);
    void decode_scaling_matrices(GetBitContext& gb, AVCSPS *sps,
                                 AVCPPS *pps, int32_t is_sps,
                                 uint8_t(*scaling_matrix4)[16],
                                 uint8_t(*scaling_matrix8)[64]);
    void build_qp_table(AVCPPS *pps, int t, int index, const int depth);
    int32_t av_clip(int32_t a, int32_t amin, int32_t amax);
};

#endif /* AM_VIDEO_PARAM_SETS_PARSER_H_ */
