/*******************************************************************************
 * am_encode_eis.h
 *
 * History:
 *   Feb 26, 2016 - [smdong] created file
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
#ifndef ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_H_
#define ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_H_

#include "am_encode_plugin_if.h"
#include "am_encode_eis_if.h"
#include "am_platform_if.h"
#include "am_video_camera_if.h"
#include "am_encode_eis_config.h"
#include "am_encode_config.h"

class AMVin;
class AMEncodeEIS: public AMIEncodePlugin, public AMIEncodeEIS
{
  public:
    static AMEncodeEIS* create(AMVin *vin, AMEncodeStream *stream);

    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();

  public:
    virtual AM_RESULT set_eis_mode(int32_t mode);
    virtual AM_RESULT get_eis_mode(int32_t &mode);
    virtual AM_RESULT apply();
    //for config files
    virtual AM_RESULT load_config();
    virtual AM_RESULT save_config();

    float change_fps_to_hz(uint32_t fps_q9);
    void calibrate();
    static int get_eis_stat(amba_eis_stat_t *eis_stat);
    static int set_eis_warp(const struct iav_warp_main* warp_main);


  protected:
    AMEncodeEIS(const char *name);
    virtual ~AMEncodeEIS();
    AM_RESULT init(AMVin *vin, AMEncodeStream *stream);

  private:
    static void static_eis_main(void *data);
    void eis_main();
    AM_RESULT setup();

  protected:
    AMVin                *m_vin;
    AMEncodeStream       *m_stream;
    AMIPlatformPtr        m_platform;
    std::string           m_name;
    AMMemMapInfo          m_mem;

  private:
    AMThread             *m_eis_thread;
    uint32_t              m_current_fps;
    int32_t               m_cali_num;
    bool                  m_is_setup;
    bool                  m_is_running;
    AMEISConfigPtr        m_config;
    AMEISConfigParam      m_config_param;
    eis_setup_t           m_eis_setup;

    static int            m_fd_iav;
    static int            m_fd_eis;
    static int            m_fd_hwtimer;

    static bool           m_is_calibrate_done;
    static bool           m_save_file_flag;
    static FILE          *m_fd_save_file;
};


#endif /* ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_H_ */
