/*******************************************************************************
 * am_lbr_control.h
 *
 * History:
 *   Nov 11, 2015 - [ypchang] created file
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
#ifndef AM_LBR_CONTROL_H_
#define AM_LBR_CONTROL_H_

#include "am_encode_plugin_if.h"
#include "am_lbr_control_config.h"

#include "lbr_api.h"
#include <atomic>
#include <mutex>
#include <map>
#include "am_low_bitrate_control_if.h"

class AMVin;
class AMThread;
class AMEncodeStream;
class AMLbrControl: public AMIEncodePlugin, public AMILBRControl
{
    typedef std::map<AM_STREAM_ID, std::pair<bool,LBR_STYLE>> AMLbrStyleMap;
  public:
    static AMLbrControl* create(AMVin *vin, AMEncodeStream *stream);

  public:
    /* Implement interface of AMIEncodePlugin */
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();

  public:
    /* Implement interface of AMIBitrateControl */
    virtual bool set_enable(uint32_t stream_id, bool enable = true);
    virtual bool get_enable(uint32_t stream_id);
    virtual bool set_bitrate_ceiling(uint32_t stream_id,
                                     uint32_t ceiling,
                                     bool is_auto = false);
    virtual bool get_bitrate_ceiling(uint32_t stream_id,
                                     uint32_t& ceiling,
                                     bool &is_auto);
    virtual bool set_drop_frame_enable(uint32_t stream_id, bool enable);
    virtual bool get_drop_frame_enable(uint32_t stream_id);
    virtual void process_motion_info(void *msg_data);
    virtual bool save_current_config();

  private:
    inline bool apply_lbr_style(AM_STREAM_ID id);

  private:
    static void static_lbr_main(void *data);
    void lbr_main();

  private:
    AMLbrControl();
    virtual ~AMLbrControl();
    bool init(AMVin *vin, AMEncodeStream *stream);

  private:
    AMLbrConfig          *m_lbr_config; /* No need to delete */
    AMVin                *m_vin;        /* No need to delete */
    AMEncodeStream       *m_stream;     /* No need to delete */
    AMLbrControlConfig   *m_config;
    AMThread             *m_lbr_thread;
    std::recursive_mutex *m_mutex;
    int                   m_iav_fd;
    uint32_t              m_motion_value;
    std::string           m_name;
    std::atomic_bool      m_run;
    AMLbrStyleMap         m_lbr_style;
};

#endif /* AM_LBR_CONTROL_H_ */
