/*******************************************************************************
 * am_encode_eis_config.h
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
#ifndef ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_CONFIG_H_
#define ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_CONFIG_H_

#include <map>
#include <mutex>
#include <atomic>

#include "lib_eis.h"
#include "am_pointer.h"
#include "am_encode_eis_if.h"

class AMEISConfig;
typedef AMPointer<AMEISConfig> AMEISConfigPtr;

struct AMEISGyroData
{
  int sample_id;    //incremental id of gyro data sample
  int16_t gyro_x;   //agular velocity on Axis-X (Gyro)
  int16_t gyro_y;   //agular velocity on Axis-Y (Gyro)
  int16_t gyro_z;   //agular velocity on Axis-Z (Gyro)
  int16_t accel_x;  //acceleration on Axis-X  (Accelerometer)
  int16_t accel_y;  //acceleration on Axis-Y  (Accelerometer)
  int16_t accel_z;  //acceleration on Axis-Z  (Accelerometer)
  AMEISGyroData() :
    sample_id(0),
    gyro_x(0),
    gyro_y(0),
    gyro_z(0),
    accel_x(0),
    accel_y(0),
    accel_z(0)
  {
  }
  AMEISGyroData(const AMEISGyroData &gyro_data) :
    sample_id(gyro_data.sample_id),
    gyro_x(gyro_data.gyro_x),
    gyro_y(gyro_data.gyro_y),
    gyro_z(gyro_data.gyro_z),
    accel_x(gyro_data.accel_x),
    accel_y(gyro_data.accel_y),
    accel_z(gyro_data.accel_z)
  {
  }
};

struct AMEISConfigParam
{
  std::pair<bool, double> accel_full_scale_range;
  std::pair<bool, double> accel_lsb;
  std::pair<bool, double> gyro_full_scale_range;
  std::pair<bool, double> gyro_lsb;
  std::pair<bool, double> threshhold;
  std::pair<bool, int> cali_num;
  std::pair<bool, int> gyro_sample_rate_in_hz;
  std::pair<bool, int> lens_focal_length_in_um;
  std::pair<bool, int> frame_buffer_num;
  std::pair<bool, EIS_FLAG> eis_mode;
  std::pair<bool, AXIS> gravity;
  std::pair<bool, EIS_AVG_MODE> avg_mode;
  std::pair<bool, std::string> file_name;
  std::pair<bool, AMEISGyroData> gyro_shift;

  AMEISConfigParam() :
    accel_full_scale_range(false, 0.0),
    accel_lsb(false, 0.0),
    gyro_full_scale_range(false, 0.0),
    gyro_lsb(false, 0.0),
    threshhold(false, 0.0),
    cali_num(false, 0),
    gyro_sample_rate_in_hz(false, 0),
    lens_focal_length_in_um(false, 0),
    frame_buffer_num(false, 0),
    eis_mode(false, EIS_DISABLE),
    gravity(false, AXIS_Z),
    avg_mode(false, EIS_AVG_MA)
  {
  }
  AMEISConfigParam(const AMEISConfigParam &conf) :
    accel_full_scale_range(conf.accel_full_scale_range),
    accel_lsb(conf.accel_lsb),
    gyro_full_scale_range(conf.gyro_full_scale_range),
    gyro_lsb(conf.gyro_lsb),
    threshhold(conf.threshhold),
    cali_num(conf.cali_num),
    gyro_sample_rate_in_hz(conf.gyro_sample_rate_in_hz),
    lens_focal_length_in_um(conf.lens_focal_length_in_um),
    frame_buffer_num(conf.frame_buffer_num),
    eis_mode(conf.eis_mode),
    gravity(conf.gravity),
    avg_mode(conf.avg_mode),
    file_name(conf.file_name),
    gyro_shift(conf.gyro_shift)
  {
  }
};

class AMEISConfig
{
    friend AMEISConfigPtr;
  public:
    static AMEISConfigPtr get_instance();
    AM_RESULT get_config(AMEISConfigParam &config);
    AM_RESULT set_config(const AMEISConfigParam &config);

    AM_RESULT load_config();
    AM_RESULT save_config();

    AMEISConfig();
    virtual ~AMEISConfig();
    void inc_ref();
    void release();

  private:
    static AMEISConfig          *m_instance;
    static std::recursive_mutex  m_mtx;
    std::atomic_int              m_ref_cnt;
    AMEISConfigParam             m_config;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define EIS_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video"))
#else
#define EIS_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#define EIS_CONFIG_FILE "eis.acs"


#endif /* ORYX_VIDEO_CONTROL_EIS_AM_ENCODE_EIS_CONFIG_H_ */
