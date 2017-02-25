/*******************************************************************************
 * am_motion_detect.h
 *
 * History:
 *   May 3, 2016 - [binwang] created file
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
#ifndef ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_H_
#define ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_H_

#include "am_motion_detect_types.h"
#include "am_encode_plugin_if.h"
#include "am_motion_detect_if.h"
#include "am_platform_if.h"
#include "am_video_camera_if.h"
#include "am_motion_detect_config.h"

class AMVin;
class AMMotionDetect: public AMIEncodePlugin, public AMIMotionDetect
{
  public:
    static AMMotionDetect* create(AMVin *vin);
    virtual void destroy();
    virtual bool start(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual bool stop(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual std::string& name();
    virtual void* get_interface();
  public:
    virtual bool set_config(AMMDConfig *pConfig);
    virtual bool get_config(AMMDConfig *pConfig);
    virtual bool set_md_callback(void *this_obj,
                                 AMRecvMD recv_md,
                                 AMMDMessage *msg);
    //for config files
    virtual AM_RESULT load_config();
    virtual AM_RESULT save_config();

  protected:
    AMMotionDetect(const char *name);
    virtual ~AMMotionDetect();

  private:
    bool construct();
    bool sync_config();
    void print_md_config();
    bool check_motion(const AMQueryFrameDesc &buf_frame);
    static void md_main(void *data);
    bool set_md_state(bool enable);
    bool get_md_state(bool *enable);
    bool set_md_buffer_id(AM_SOURCE_BUFFER_ID source_buf_id);
    bool get_md_buffer_id(AM_SOURCE_BUFFER_ID *source_buf_id);
    bool set_md_buffer_type(AM_DATA_FRAME_TYPE source_buf_type);
    bool get_md_buffer_type(AM_DATA_FRAME_TYPE *source_buf_type);
    bool check_md_roi_format(AMMDRoi *roi_info, uint32_t buf_width,
                             uint32_t buf_height, uint32_t buf_pitch);
    bool set_roi_info(AMMDRoi *roi_info);
    bool get_roi_info(AMMDRoi *roi_info);
    bool set_threshold_info(AMMDThreshold *threshold);
    bool get_threshold_info(AMMDThreshold *threshold);
    bool set_level_change_delay_info(
        AMMDLevelChangeDelay *level_change_delay);
    bool get_level_change_delay_info(
        AMMDLevelChangeDelay *level_change_delay);
    bool exec_callback_func(void);

  protected:
    std::string                     m_name;
  private:
    AMMDMessage                     m_msg;
    AMQueryFrameDesc                m_frame_desc;
    mdet_session_t                  mdet_session;
    mdet_cfg                        mdet_config;
    std::string                     m_conf_path;
    std::atomic<bool>               m_main_loop_exit;
    std::atomic<bool>               m_started;
    std::atomic<bool>               m_enable;
    AMMDBuf                         m_md_buffer;
    AMIPlatformPtr                  m_platform;
    AMIVideoReaderPtr               m_video_reader;
    AMIVideoAddressPtr              m_video_address;
    AMRecvMD                        m_callback;
    AMThread                       *m_md_thread;
    mdet_instance                  *m_inst;
    AMMotionDetectConfig           *m_config;
    MotionDetectParam               m_config_param;
    AMMDRoi                        *m_roi_info;
    AMMDThreshold                  *m_threshold;
    AMMDLevelChangeDelay           *m_level_change_delay;
    void                           *m_callback_owner_obj;
};

#endif /* ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_H_ */
