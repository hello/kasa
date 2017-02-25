/*******************************************************************************
 * am_encode_overlay_if.h
 *
 * History:
 *   2016年3月15日 - [ypchang] created file
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
 *
 ******************************************************************************/
#ifndef AM_ENCODE_OVERLAY_IF_H_
#define AM_ENCODE_OVERLAY_IF_H_

#include "am_encode_overlay_types.h"

class AMIEncodeOverlay
{
  public:
    virtual int32_t init_area(AM_STREAM_ID stream_id,
                              AMOverlayAreaAttr &attr)                      = 0;
    //return value is the unique index in the area or error(<0)
    virtual int32_t add_data_to_area(AM_STREAM_ID stream_id,
                                     int32_t area_id,
                                     AMOverlayAreaData &data)               = 0;
    virtual AM_RESULT destroy_overlay(AM_STREAM_ID id = AM_STREAM_ID_MAX)   = 0;
    virtual AM_RESULT update_area_data(AM_STREAM_ID stream_id,
                                       int32_t area_id,
                                       int32_t index,
                                       AMOverlayAreaData &data)             = 0;
    virtual AM_RESULT delete_area_data(AM_STREAM_ID stream_id,
                                       int32_t area_id,
                                       int32_t index)                       = 0;
    virtual AM_RESULT change_state(AM_STREAM_ID stream_id,
                                   int32_t area_id,
                                   AM_OVERLAY_STATE state)                  = 0;
    virtual AM_RESULT get_area_param(AM_STREAM_ID stream_id,
                                     int32_t area_id,
                                     AMOverlayAreaParam &param)             = 0;
    virtual AM_RESULT get_area_data_param(AM_STREAM_ID stream_id,
                                          int32_t area_id,
                                          int32_t index,
                                          AMOverlayAreaData &param)         = 0;
    virtual void get_user_defined_limit_value(AMOverlayUserDefLimitVal &val)= 0;
    virtual AM_RESULT save_param_to_config()                                = 0;
    virtual uint32_t get_area_max_num()                                     = 0;

  protected:
    virtual ~AMIEncodeOverlay(){}
};

#define VIDEO_PLUGIN_OVERLAY    ("overlay")
#define VIDEO_PLUGIN_OVERLAY_SO ("video-overlay.so")

#endif /* AM_ENCODE_OVERLAY_IF_H_ */
