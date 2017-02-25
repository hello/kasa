/*******************************************************************************
 * am_video_param_sets_parser_if.h
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
#ifndef AM_VIDEO_PARAM_SETS_PARSER_IF_H_
#define AM_VIDEO_PARAM_SETS_PARSER_IF_H_

#include "h264.h"
#include "h265.h"

class AMIVideoParamSetsParser
{
  public :
    static AMIVideoParamSetsParser* create();
    virtual ~AMIVideoParamSetsParser() {};
    virtual bool parse_hevc_vps(uint8_t *p_data,
                                uint32_t data_size,
                                HEVCVPS& vps) = 0;
    virtual bool parse_hevc_sps(uint8_t *p_data,
                                uint32_t data_size,
                                HEVCSPS& sps) = 0;
    /*should parse sps first.*/
    virtual bool parse_hevc_pps(uint8_t *p_data,
                                uint32_t data_size,
                                HEVCPPS& pps,
                                HEVCSPS& sps) = 0;
    virtual bool parse_avc_sps(uint8_t *p_data,
                               uint32_t data_size,
                               AVCSPS& sps) = 0;
    virtual bool parse_avc_pps(uint8_t *p_data,
                               uint32_t data_size,
                               AVCPPS& pps,
                               AVCSPS& sps) = 0;
};

#endif /* AM_VIDEO_PARAM_SETS_PARSER_IF_H_ */
