/*******************************************************************************
 * am_video_address_if.h
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
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
#ifndef AM_VIDEO_ADDRESS_IF_H_
#define AM_VIDEO_ADDRESS_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

class AMIVideoAddress;

typedef AMPointer<AMIVideoAddress> AMIVideoAddressPtr;
class AMIVideoAddress
{
    friend AMIVideoAddressPtr;

  public:
    static AMIVideoAddressPtr get_instance();

  public:
    virtual AM_RESULT addr_get(AM_DATA_FRAME_TYPE type,
                               uint32_t offset,
                               AMAddress &addr)                             = 0;

    virtual uint8_t* pic_addr_get(uint32_t pic_data_offset)                 = 0;
    virtual uint8_t* video_addr_get(uint32_t video_data_offset)             = 0;

    virtual AM_RESULT video_addr_get(const AMQueryFrameDesc &desc,
                                     AMAddress &addr)                       = 0;
    virtual AM_RESULT yuv_y_addr_get(const AMQueryFrameDesc &desc,
                                     AMAddress &addr)                       = 0;
    virtual AM_RESULT yuv_uv_addr_get(const AMQueryFrameDesc &desc,
                                      AMAddress &addr)                      = 0;
    virtual AM_RESULT raw_addr_get(const AMQueryFrameDesc &desc,
                                   AMAddress &addr)                         = 0;
    virtual AM_RESULT me0_addr_get(const AMQueryFrameDesc &desc,
                                   AMAddress &addr)                         = 0;
    virtual AM_RESULT me1_addr_get(const AMQueryFrameDesc &desc,
                                   AMAddress &addr)                         = 0;
    virtual AM_RESULT usr_addr_get(AMAddress &addr)                         = 0;

    virtual bool is_new_video_session(uint32_t session_id,
                                      AM_STREAM_ID stream_id)               = 0;

  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoAddress(){};
};

#endif /* AM_VIDEO_ADDRESS_IF_H_ */
