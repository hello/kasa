/*******************************************************************************
 * am_motion_detect_config.h
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
#ifndef ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_CONFIG_H_
#define ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_CONFIG_H_

#include <mutex>
#include <atomic>

#include "am_pointer.h"
#include "am_motion_detect_types.h"
#include "am_motion_detect_if.h"

using std::string;

struct MotionDetectParam
{
    AMMDRoi roi_info[MAX_ROI_NUM];
    AMMDThreshold th[MAX_ROI_NUM];
    AMMDLevelChangeDelay lc_delay[MAX_ROI_NUM];
    AMMDBuf buf;
    bool enable;
    MotionDetectParam():
      enable(false)
    {}
};

class AMConfig;
class AMMotionDetectConfig
{
  public:
    static AMMotionDetectConfig *get_instance();
    AM_RESULT get_config(MotionDetectParam &config);
    AM_RESULT set_config(const MotionDetectParam &config);

    AM_RESULT load_config();
    AM_RESULT save_config();

    void inc_ref();
    void release();

  protected:
    AMMotionDetectConfig();
    virtual ~AMMotionDetectConfig();

  private:
    static AMMotionDetectConfig *m_instance;
    static std::recursive_mutex m_mtx;
    MotionDetectParam           m_md_config;
    std::atomic_int             m_ref_cnt;

    string buf_type_enum_to_str(AM_DATA_FRAME_TYPE buf_type);
    AM_DATA_FRAME_TYPE buf_type_str_to_enum (string buf_str);
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define MD_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video"))
#else
#define MD_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#define MD_CONFIG_FILE "motion-detect.acs"

#endif /* ORYX_VIDEO_CONTROL_MOTION_DETECT_AM_MOTION_DETECT_CONFIG_H_ */
