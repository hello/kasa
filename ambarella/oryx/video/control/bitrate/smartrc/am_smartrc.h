/*******************************************************************************
 * am_smartrc.h
 *
 * History:
 *   Jul 4, 2016 - [binwang] created file
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
#ifndef ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_H_
#define ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_H_

#include "am_encode_plugin_if.h"
#include "am_smartrc_config.h"
#include "lib_smartrc.h"
#include <atomic>
#include <mutex>
#include <map>
#include "am_smartrc_if.h"

class AMVin;
class AMThread;
class AMEncodeStream;
class AMSmartRC: public AMIEncodePlugin, public AMISmartRC
{
  public:
    static AMSmartRC* create(AMVin* vin, AMEncodeStream *stream);

  public:
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();
  public:
    virtual bool set_enable(uint32_t stream_id, bool enable = true);
    virtual bool get_enable(uint32_t stream_id);
    virtual bool set_bitrate_ceiling(uint32_t stream_id, uint32_t ceiling);
    virtual bool get_bitrate_ceiling(uint32_t stream_id, uint32_t& ceiling);
    virtual bool set_drop_frame_enable(uint32_t stream_id, bool enable);
    virtual bool get_drop_frame_enable(uint32_t stream_id);
    virtual bool set_dynanic_gop_n(uint32_t stream_id, bool enable);
    virtual bool get_dynanic_gop_n(uint32_t stream_id);
    virtual bool set_mb_level_control(uint32_t stream_id, bool enable);
    virtual bool get_mb_level_control(uint32_t stream_id);
    virtual void process_motion_info(void *msg_data);

  private:
    static void static_smartrc_main(void *data);
    void smartrc_main();

  private:
    AMSmartRC();
    virtual ~AMSmartRC();
    bool init(AMVin *vin, AMEncodeStream *stream);
  private:
    AMSmartRCParam       m_smartrc_param;
    AMVin                *m_vin;
    AMEncodeStream       *m_stream;
    AMSmartRCConfig      *m_config;
    roi_session_t        *m_roi_session;
    AMThread             *m_smartrc_thread;
    std::recursive_mutex *m_mutex;
    int32_t              m_iav_fd;
    uint32_t             m_motion_value;
    std::string          m_name;
    std::atomic<bool>    m_run;
};

#endif /* ORYX_VIDEO_CONTROL_BITRATE_SMARTRC_AM_SMARTRC_H_ */
