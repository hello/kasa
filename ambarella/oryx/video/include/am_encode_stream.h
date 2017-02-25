/**
 * am_encode_stream.h
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_STREAM_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_STREAM_H_

#include "am_platform_if.h"
#include <mutex>

class AMEncodeStream
{
  public:
    static AMEncodeStream* create();
    void destroy();

    AM_RESULT load_config();
    AM_RESULT get_param(AMStreamParamMap &param);
    AM_RESULT set_param(const AMStreamParamMap &param);
    AM_RESULT save_config();
    const AMStreamFormatConfig& stream_config(AM_STREAM_ID id);

    AM_RESULT setup(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    AM_RESULT start_encode(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    AM_RESULT stop_encode(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    const AMStreamStateMap& stream_state();



  protected:
    void check_stream_state(AM_STREAM_ID id = AM_STREAM_ID_MAX);

  private:
    AMEncodeStream();
    virtual ~AMEncodeStream();
    AM_RESULT init();
    uint32_t  adjust_i_frame_max_size_with_res(AMResolution res);

  private:
    AMStreamConfigPtr   m_config;
    AMIPlatformPtr      m_platform;
    AMStreamParamMap    m_param;
    AMStreamStateMap    m_stream_state;
    std::mutex          m_conf_mutex;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_STREAM_H_ */
