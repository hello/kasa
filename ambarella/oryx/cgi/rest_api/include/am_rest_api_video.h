/**
 * am_rest_api_video.h
 *
 *  History:
 *		2015年8月18日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_

#include "am_base_include.h"
#include "am_api_video.h"
#include "am_rest_api_handle.h"

//video service handle class
class AMRestAPIVideo: public AMRestAPIHandle
{
  public:
    virtual AM_REST_RESULT rest_api_handle();

  private:
    AM_REST_RESULT  video_overlay_handle();
    AM_REST_RESULT  video_dptz_handle();
    AM_REST_RESULT  video_dewarp_handle();
    AM_REST_RESULT  video_enc_ctrl_handle();

  private:
    //overlay group manipulate
    int32_t         overlay_get_max_num();
    AM_REST_RESULT  overlay_add_handle();
    AM_REST_RESULT  overlay_get_handle();
    AM_REST_RESULT  overlay_manipulate_handle();
    AM_REST_RESULT  overlay_save_handle();
    AM_REST_RESULT  overlay_parameters_init(am_overlay_s *add_cfg);
    AM_REST_RESULT  overlay_manipulate_by_stream(int32_t stream_id, int32_t area_id);
    AM_REST_RESULT  overlay_get_config();
    AM_REST_RESULT  overlay_get_available_font();
    AM_REST_RESULT  overlay_get_available_bmp();

    //dptz and dewarp group manipulate
    AM_REST_RESULT  dptz_set_handle();
    AM_REST_RESULT  dptz_get_handle();
    AM_REST_RESULT  dewarp_set_handle();
    AM_REST_RESULT  dewarp_get_handle();

    //encode control group manipulate
    AM_REST_RESULT  enc_ctrl_start_stream();
    AM_REST_RESULT  enc_ctrl_stop_stream();
    AM_REST_RESULT  enc_ctrl_get_stream_status();
    AM_REST_RESULT  enc_ctrl_force_idr();

    AM_REST_RESULT  get_srteam_id(int32_t &stream_id, const std::string &arg_name);
    AM_REST_RESULT  get_area_id(int32_t &area_id);
    AM_REST_RESULT  get_buffer_id(int32_t &buffer_id);
};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_VIDEO_H_ */
