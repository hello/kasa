/**
 * am_encode_buffer.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_BUFFER_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_BUFFER_H_

#include "am_platform_if.h"

class AMEncodeBuffer
{
  public:
    static AMEncodeBuffer* create();
    virtual void destroy();

    AM_RESULT setup();
    AM_RESULT load_config();
    AM_RESULT get_param(AMBufferParamMap &param);
    AM_RESULT set_param(const AMBufferParamMap &param);
    AM_RESULT save_config();

    AM_RESULT get_buffer_state(AM_SOURCE_BUFFER_ID id,
                               AM_SRCBUF_STATE &state);
    AM_RESULT get_buffer_format(AMBufferConfigParam &param);
    AM_RESULT set_buffer_format(const AMBufferConfigParam &param);

  private:
    AMEncodeBuffer();
    virtual ~AMEncodeBuffer();
    AM_RESULT init();

  private:
    AMIPlatformPtr      m_platform;
    AMBufferConfigPtr   m_config;
    AMBufferParamMap    m_param;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_BUFFER_H_ */
