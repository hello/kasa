/**
 * am_rest_api_video.h
 *
 *  History:
 *		2015/08/18 - [Huaiqing Wang] created file
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
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_

#include "am_base_include.h"
#include "am_video_types.h"
#include "am_api_video.h"
#include "am_rest_api_handle.h"

//video service handle class
class AMRestAPIVideo: public AMRestAPIHandle
{
  public:
    virtual AM_REST_RESULT rest_api_handle();

  private:
    AM_REST_RESULT  video_load_all_cfg();
    AM_REST_RESULT  video_overlay_handle();
    AM_REST_RESULT  video_dptz_handle();
    AM_REST_RESULT  video_dewarp_handle();
    AM_REST_RESULT  video_enc_ctrl_handle();

  private:
    //overlay group manipulate
    AM_REST_RESULT  overlay_get_max_num(int32_t &value);
    AM_REST_RESULT  overlay_set_handle();
    AM_REST_RESULT  overlay_init_handle();
    AM_REST_RESULT  overlay_data_manipulate_handle();
    AM_REST_RESULT  overlay_data_update_handle(int32_t data_id);
    AM_REST_RESULT  overlay_data_add_handle();
    AM_REST_RESULT  overlay_get_handle();
    AM_REST_RESULT  overlay_data_get_handle();
    AM_REST_RESULT  overlay_manipulate_handle(const std::string &action);
    AM_REST_RESULT  overlay_save_handle();
    AM_REST_RESULT  overlay_parameters_init(am_overlay_data_t &data);
    AM_REST_RESULT  overlay_manipulate_by_area(int32_t stream_id,
                                               int32_t area_id,
                                               const std::string &action);
    AM_REST_RESULT  overlay_delete_all_areas();
    AM_REST_RESULT  overlay_get_area_params();
    AM_REST_RESULT  overlay_get_data_params(int32_t stream_id,
                                            int32_t area_id,
                                            int32_t &data_id);
    AM_REST_RESULT  overlay_get_available_font();
    AM_REST_RESULT  overlay_get_available_bmp();

    //dptz and dewarp group manipulate
    AM_REST_RESULT  dptz_set_handle();
    AM_REST_RESULT  dptz_get_handle();
    AM_REST_RESULT  dewarp_set_handle();
    AM_REST_RESULT  dewarp_get_handle();

    //encode control group manipulate
    AM_REST_RESULT  enc_ctrl_encode();
    AM_REST_RESULT  enc_ctrl_stream(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_start_encode();
    AM_REST_RESULT  enc_ctrl_stop_encode();
    AM_REST_RESULT  enc_ctrl_get_encode_status();
    AM_REST_RESULT  enc_ctrl_start_streaming(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_stop_streaming(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_force_idr(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_stream_format(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_stream_format_dyn(int32_t stream_id);
    AM_REST_RESULT  enc_ctrl_stream_info_get(int32_t stream_id);

    AM_REST_RESULT  get_stream_max_num(int32_t &value);
    AM_REST_RESULT  get_buffer_max_num(int32_t &value);
    AM_REST_RESULT  get_srteam_id(int32_t &stream_id,
                                  const std::string &arg_name);
    AM_REST_RESULT  get_area_id(int32_t &area_id);
    AM_REST_RESULT  get_buffer_id(int32_t &buffer_id);
};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_ */
