/*******************************************************************************
 * am_low_bitrate_contrl_if.h
 *
 * History:
 *   Nov 9, 2015 - [ypchang] created file
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
#ifndef AM_LOW_BITRATE_CONTROL_IF_H_
#define AM_LOW_BITRATE_CONTROL_IF_H_

class AMILBRControl
{
  public:
    virtual bool set_enable(uint32_t stream_id, bool enable = true)         = 0;
    virtual bool get_enable(uint32_t stream_id)                             = 0;
    virtual bool set_bitrate_ceiling(uint32_t stream_id,
                                     uint32_t ceiling,
                                     bool is_auto = false)                  = 0;
    virtual bool get_bitrate_ceiling(uint32_t stream_id,
                                     uint32_t& ceiling,
                                     bool &is_auto)                         = 0;
    virtual bool set_drop_frame_enable(uint32_t stream_id, bool enable)     = 0;
    virtual bool get_drop_frame_enable(uint32_t stream_id)                  = 0;
    virtual void process_motion_info(void *msg_data)                        = 0;
    virtual bool save_current_config()                                      = 0;

  protected:
    virtual ~AMILBRControl(){}
};

#define VIDEO_PLUGIN_LBR    ("lbr")
#define VIDEO_PLUGIN_LBR_SO ("video-lbr.so")

#endif /* AM_LOW_BITRATE_CONTROL_IF_H_ */
