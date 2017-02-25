/*******************************************************************************
 * am_dptz.h
 *
 * History:
 *   Mar 28, 2016 - [zfgong] created file
 *   Sep 23, 2016 - [Huaiqing Wang] modified file
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
#ifndef AM_DPTZ_H_
#define AM_DPTZ_H_

#include "am_platform_if.h"
#include "am_encode_plugin_if.h"
#include "am_encode_config.h"
#include "am_vin.h"
#include "am_dptz_if.h"
#include <mutex>
#include <map>
#include <vector>


class AMVin;

class AMDPTZ: public AMIEncodePlugin, public AMIDPTZ
{

  public:
    /* Implement interface of AMIEncodePlugin */
    static AMDPTZ* create(AMVin *vin);

  public:
    /* Implement interface of AMIDPTZ */
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();

  public:
    virtual AM_RESULT set_ratio(AM_SOURCE_BUFFER_ID id, AMDPTZRatio &ratio);
    virtual AM_RESULT get_ratio(AM_SOURCE_BUFFER_ID id, AMDPTZRatio &ratio);
    virtual AM_RESULT set_size(AM_SOURCE_BUFFER_ID id, AMDPTZSize &wnd);
    virtual AM_RESULT get_size(AM_SOURCE_BUFFER_ID id, AMDPTZSize &wnd);

  private:
    AMDPTZ();
    virtual ~AMDPTZ();
    AM_RESULT init(AMVin *vin);

  private:
    AM_RESULT get_plat_param(AM_SOURCE_BUFFER_ID id, AMPlatformDPTZParam &param);
    AM_RESULT get_input_buffer(AM_SOURCE_BUFFER_ID id, AMResolution &rsln);
    AM_RESULT get_current_buffer(AM_SOURCE_BUFFER_ID id, AMResolution &rsln);

    AM_RESULT round_window(AM_SOURCE_BUFFER_ID id, AMRect &rect);
    AM_RESULT ratio_to_window(AM_SOURCE_BUFFER_ID id,
                              AMPlatformDPTZRatio &ratio,
                              AMRect &window);

    AM_RESULT apply(AM_SOURCE_BUFFER_ID id, AMRect &rect);

  private:
    std::recursive_mutex  *m_mutex;
    AMVin                 *m_vin;
    std::string            m_name;
    AMIPlatformPtr         m_platform;
    AMPlatformDPTZParamMap m_plat_param_map;
};

#endif /* AM_DPTZ_H_ */
