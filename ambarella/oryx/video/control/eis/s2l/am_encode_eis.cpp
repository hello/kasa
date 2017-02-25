/*******************************************************************************
 * am_encode_eis.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_thread.h"

#include <sys/ioctl.h>
#include "lib_eis.h"
#include "am_encode_eis.h"
#include "am_video_utility.h"
#include "am_configure.h"
#include "am_vin.h"
#include "am_encode_stream.h"

#define DIV_ROUND(divident, divider) (((divident)+((divider)>>1)) / (divider))

int AMEncodeEIS::m_fd_iav = -1;
int AMEncodeEIS::m_fd_eis = -1;
int AMEncodeEIS::m_fd_hwtimer = -1;

bool  AMEncodeEIS::m_is_calibrate_done = false;
bool  AMEncodeEIS::m_save_file_flag = false;
FILE *AMEncodeEIS::m_fd_save_file = nullptr;

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  AMPluginData *pdata = ((AMPluginData*)data);
  return AMEncodeEIS::create(pdata->vin, pdata->stream);
}
#ifdef __cplusplus
}
#endif

AMEncodeEIS* AMEncodeEIS::create(AMVin *vin, AMEncodeStream *stream)
{
  AMEncodeEIS *eis = new AMEncodeEIS("EIS");
  do {
    if (AM_UNLIKELY(!eis)) {
      ERROR("Failed to create AMEncodeEIS");
      break;
    }
    if (AM_UNLIKELY(AM_RESULT_OK != eis->init(vin, stream))) {
      delete eis;
      eis = nullptr;
      break;
    }
    if (AM_UNLIKELY(!(eis->m_config = AMEISConfig::get_instance()))) {
      ERROR("Failed to create AMEISConfig!");
      break;
    }
    if (AM_UNLIKELY(AM_RESULT_OK != eis->load_config())) {
      ERROR("EIS load config error");
      break;
    }

  } while(0);
  return eis;
}

void AMEncodeEIS::destroy()
{
  if (m_is_running) {
    eis_enable(EIS_DISABLE);
    eis_close();
    m_is_running = false;
  }
  m_is_setup = false;
  AM_DESTROY(m_eis_thread);
  m_eis_thread = nullptr;

  if (AM_LIKELY(m_fd_hwtimer > 0)) {
    close(m_fd_hwtimer);
  }
  if (AM_LIKELY(m_fd_iav > 0)) {
    close(m_fd_iav);
  }
  if (AM_LIKELY(m_fd_eis > 0)) {
    close(m_fd_eis);
  }
  if (AM_LIKELY(m_fd_save_file)) {
    fclose(m_fd_save_file);
    m_fd_save_file = nullptr;
  }
  if (AM_LIKELY(m_platform != nullptr)) {
    m_platform->unmap_eis_warp();
  }
  delete this;
}

bool AMEncodeEIS::start(AM_STREAM_ID UNUSED(id))
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(m_eis_thread)) {
      NOTICE("EIS is already running!");
      break;
    }
    m_eis_thread = AMThread::create("EIS.ctrl", static_eis_main, this);
    if (AM_UNLIKELY(!m_eis_thread)) {
      ret = false;
      ERROR("Failed to start EIS Controlling thread!");
      break;
    }
  } while(0);

  return ret;
}

bool AMEncodeEIS::stop(AM_STREAM_ID id)
{
  bool need_stop = true;
  if (AM_LIKELY(AM_STREAM_ID_MAX != id)) {
    /* When only one stream is disabled,
     * we need to check if this stream is the only one left to be disabled,
     * if it is, we need to stop this plug-in
     */
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    for (auto &m : stream_state) {
      if (AM_LIKELY(m.first != id)) {
        if (AM_LIKELY(m.second != AM_STREAM_STATE_IDLE)) {
          need_stop = false;
          break;
        }
      } else {
        if (AM_LIKELY((m.second != AM_STREAM_STATE_ENCODING) &&
                      (m.second != AM_STREAM_STATE_IDLE))) {
          need_stop = false;
          break;
        }
      }
    }
  }

  if (AM_LIKELY(need_stop)) {
    if (m_is_running) {
      eis_enable(EIS_DISABLE);
      eis_close();
      m_is_running = false;
    }
    m_is_setup = false;
    m_is_calibrate_done = false;
    AM_DESTROY(m_eis_thread);
    m_eis_thread = nullptr;
  } else {
    INFO("No need to stop!");
  }

  return true;
}

std::string& AMEncodeEIS::name()
{
  return m_name;
}

void* AMEncodeEIS::get_interface()
{
  return ((AMIEncodeEIS*)this);
}

AMEncodeEIS::AMEncodeEIS(const char *name) :
    m_vin(nullptr),
    m_stream(nullptr),
    m_platform(nullptr),
    m_name(name),
    m_eis_thread(nullptr),
    m_current_fps(0),
    m_cali_num(3000),
    m_is_setup(false),
    m_is_running(false),
    m_config(nullptr)
{

}

AMEncodeEIS::~AMEncodeEIS()
{
  m_platform = nullptr;
  m_config = nullptr;
}

AM_RESULT AMEncodeEIS::init(AMVin *vin, AMEncodeStream *stream)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_vin = vin;
    m_stream = stream;
    if (AM_UNLIKELY(!m_vin)) {
      ERROR("Invalid VIN device");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (AM_UNLIKELY(!m_stream)) {
      ERROR("Invalid Encode Stream device");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (AM_UNLIKELY(!(m_platform = AMIPlatform::get_instance()))) {
      ERROR("Failed to get AMIPlatform!");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    if (AM_UNLIKELY(m_platform->map_eis_warp(m_mem)) < 0) {
      ERROR("Failed to map EIS!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    if ((m_fd_iav < 0) && ((m_fd_iav = open("/dev/iav", O_RDWR, 0)) < 0)) {
      PERROR("open /dev/iav");
      result = AM_RESULT_ERR_IO;
      break;
    }
    if ((m_fd_eis < 0) && ((m_fd_eis = open("/dev/eis", O_RDWR, 0)) < 0)) {
      PERROR("open /dev/eis");
      result = AM_RESULT_ERR_IO;
      break;
    }
    if ((m_fd_hwtimer < 0) && ((m_fd_hwtimer =
           open("/proc/ambarella/ambarella_hwtimer", O_RDWR, 0)) < 0)) {
      PERROR("open /proc/ambarella/ambarella_hwtimer");
      result = AM_RESULT_ERR_IO;
      break;
    }

  } while(0);
  return result;
}

void AMEncodeEIS::static_eis_main(void *data)
{
  ((AMEncodeEIS*)data)->eis_main();
}

void AMEncodeEIS::eis_main()
{
  do {
    if (AM_RESULT_OK != setup()) {
      ERROR("Failed to setup EIS");
      break;
    }

    if (AM_RESULT_OK != apply()) {
      ERROR("Failed to apply EIS");
      break;
    }

  } while(0);

}

AM_RESULT AMEncodeEIS::setup()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    version_t version;
    eis_version(&version);
    INFO("\nLibrary Version: %s-%d.%d.%d (Last updated: %x)\n\n",
        version.description, version.major, version.minor,
        version.patch, version.mod_time);

    m_cali_num = m_config_param.cali_num.second;

    m_eis_setup.accel_full_scale_range =
        m_config_param.accel_full_scale_range.second;
    m_eis_setup.accel_lsb = m_config_param.accel_lsb.second;
    m_eis_setup.gyro_full_scale_range =
        m_config_param.gyro_full_scale_range.second;
    m_eis_setup.gyro_lsb = m_config_param.gyro_lsb.second;
    m_eis_setup.gyro_sample_rate_in_hz =
        m_config_param.gyro_sample_rate_in_hz.second;
    m_eis_setup.gyro_shift.xg = m_config_param.gyro_shift.second.gyro_x;
    m_eis_setup.gyro_shift.yg = m_config_param.gyro_shift.second.gyro_y;
    m_eis_setup.gyro_shift.zg = m_config_param.gyro_shift.second.gyro_z;
    m_eis_setup.gyro_shift.xa = m_config_param.gyro_shift.second.accel_x;
    m_eis_setup.gyro_shift.ya = m_config_param.gyro_shift.second.accel_y;
    m_eis_setup.gyro_shift.za = m_config_param.gyro_shift.second.accel_z;
    m_eis_setup.gravity_axis = m_config_param.gravity.second;

    m_save_file_flag = strlen(m_config_param.file_name.second.c_str()) > 0;
    if (m_save_file_flag && !m_fd_save_file) {
      if (AM_UNLIKELY((m_fd_save_file = fopen(m_config_param.file_name.second.
                                              c_str(), "w+")) == nullptr)) {
        PERROR(m_config_param.file_name.second.c_str());
        result = AM_RESULT_ERR_IO;
        break;
      } else {
        INFO("Ready to save gyro data to %s",
            m_config_param.file_name.second.c_str());
      }
    }

    m_eis_setup.lens_focal_length_in_um =
        m_config_param.lens_focal_length_in_um.second;
    m_eis_setup.threshhold = m_config_param.threshhold.second;
    m_eis_setup.frame_buffer_num = m_config_param.frame_buffer_num.second;
    m_eis_setup.avg_mode = m_config_param.avg_mode.second;

    iav_srcbuf_setup buffer_setup;
    vindev_video_info vin_info;
    vindev_eisinfo vin_eis_info;
    vindev_fps vsrc_fps;

    memset(&buffer_setup, 0, sizeof(buffer_setup));
    memset(&vin_info, 0, sizeof(vin_info));
    memset(&vin_eis_info, 0, sizeof(vin_eis_info));

    vin_info.vsrc_id = 0;
    vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
    if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_VIN_GET_VIDEOINFO,
                          &vin_info) < 0)) {
      PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP,
                          &buffer_setup) < 0)) {
      PERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_VIN_GET_EISINFO,
                          &vin_eis_info) < 0)) {
      PERROR("IAV_IOC_VIN_GET_EISINFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    vsrc_fps.vsrc_id = 0;
    if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0)) {
      PERROR("IAV_IOC_VIN_GET_FPS");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    m_eis_setup.vin_width = vin_info.info.width;
    m_eis_setup.vin_height = vin_info.info.height;
    m_eis_setup.vin_col_bin = vin_eis_info.column_bin;
    m_eis_setup.vin_row_bin = vin_eis_info.row_bin;
    m_eis_setup.vin_cell_width_in_um =
        (float)vin_eis_info.sensor_cell_width / 100.0;
    m_eis_setup.vin_cell_height_in_um =
        (float)vin_eis_info.sensor_cell_height / 100.0;
    m_eis_setup.vin_frame_rate_in_hz = change_fps_to_hz(vsrc_fps.fps);
    m_eis_setup.vin_vblank_in_ms = vin_eis_info.vb_time / 1000000.0;

    m_eis_setup.premain_input_width = vin_info.info.width;
    m_eis_setup.premain_input_height = vin_info.info.height;
    m_eis_setup.premain_input_offset_x = 0;
    m_eis_setup.premain_input_offset_y = 0;

    m_eis_setup.premain_width = vin_info.info.width;
    m_eis_setup.premain_height = vin_info.info.height;

    m_eis_setup.output_width = buffer_setup.size[0].width;
    m_eis_setup.output_height = buffer_setup.size[0].height;

    m_current_fps = vsrc_fps.fps;

    //map_eis_warp
    m_eis_setup.h_table_addr = (s16*)m_mem.addr;
    m_eis_setup.v_table_addr = (s16*)(m_mem.addr + MAX_WARP_TABLE_SIZE_LDC *
                                       sizeof(s16));
    m_eis_setup.me1_v_table_addr = (s16*)(m_mem.addr + MAX_WARP_TABLE_SIZE_LDC
                                           * sizeof(s16) * 2);

    if(AM_LIKELY(m_cali_num > 0)) {
      calibrate();
      m_is_calibrate_done = true;
    }

    if (AM_UNLIKELY(eis_setup(&m_eis_setup, set_eis_warp, get_eis_stat) < 0)) {
      ERROR("Failed to setup EIS");
      result = AM_RESULT_ERR_INVALID;
      break;
    } else {
      m_is_setup = true;
    }
  } while(0);

  return result;
}

AM_RESULT AMEncodeEIS::apply()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (AM_UNLIKELY(!m_is_setup)) {
      WARN("EIS is not setup, can not apply!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    int eis_mode = m_config_param.eis_mode.second;
    INFO("EIS mode is %d", eis_mode);

    if (eis_mode) {
      if (AM_UNLIKELY(eis_open() < 0)) {
        ERROR("Failed to open EIS");
        result = AM_RESULT_ERR_INVALID;
        break;
      }
      eis_enable(EIS_FLAG(eis_mode));
      m_is_running = true;
    } else {
      eis_close();
    }

  } while(0);

  return result;
}

AM_RESULT AMEncodeEIS::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->load_config()) != AM_RESULT_OK) {
      ERROR("Failed to get eis config!");
      break;
    }
    if ((result = m_config->get_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get eis config!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeEIS::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->set_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get eis config!");
      break;
    }
    if ((result = m_config->save_config()) != AM_RESULT_OK) {
      ERROR("Failed to set eis config!");
      break;
    }
  } while (0);
  return result;
}

float AMEncodeEIS::change_fps_to_hz(uint32_t fps_q9)
{
  switch(fps_q9) {
  case AMBA_VIDEO_FPS_29_97:
    return 29.97f;
  case AMBA_VIDEO_FPS_59_94:
    return 59.94f;
  default:
    return DIV_ROUND(512000000, fps_q9);
  }

  return 0;
}

void AMEncodeEIS::calibrate()
{
  int i, n = 0;
  int sum_xg = 0, sum_yg = 0, sum_zg = 0, sum_xa = 0, sum_ya = 0, sum_za = 0;
  amba_eis_stat_t eis_stat;

  while (n < m_cali_num) {
    if (get_eis_stat(&eis_stat) == 0) {
      for (i = 0; i < eis_stat.gyro_data_count; i++) {
        sum_xg += eis_stat.gyro_data[i].xg;
        sum_yg += eis_stat.gyro_data[i].yg;
        sum_zg += eis_stat.gyro_data[i].zg;
        sum_xa += eis_stat.gyro_data[i].xa;
        sum_ya += eis_stat.gyro_data[i].ya;
        sum_za += eis_stat.gyro_data[i].za;
      }
      n += eis_stat.gyro_data_count;
    }
  }
  m_eis_setup.gyro_shift.xg = sum_xg / n;
  m_eis_setup.gyro_shift.yg = sum_yg / n;
  m_eis_setup.gyro_shift.zg = sum_zg / n;
  m_eis_setup.gyro_shift.xa = sum_xa / n;
  m_eis_setup.gyro_shift.ya = sum_ya / n;
  m_eis_setup.gyro_shift.za = sum_za / n;

  INFO("Calibration: accel ( %d, %d, %d ), gyro ( %d, %d, %d )\n",
      m_eis_setup.gyro_shift.xa, m_eis_setup.gyro_shift.ya,
      m_eis_setup.gyro_shift.za, m_eis_setup.gyro_shift.xg,
      m_eis_setup.gyro_shift.yg, m_eis_setup.gyro_shift.zg);
}

int AMEncodeEIS::get_eis_stat(amba_eis_stat_t *eis_stat)
{
  int32_t i = 0;
  uint64_t hw_pts = 0;
  char pts_buf[32];
  if (AM_UNLIKELY(ioctl(m_fd_eis, AMBA_IOC_EIS_GET_STAT, eis_stat) < 0)) {
    PERROR("AMBA_IOC_EIS_GET_STAT\n");
    return -1;
  }
  if (AM_UNLIKELY(eis_stat->discard_flag || eis_stat->gyro_data_count == 0)) {
    DEBUG("stat discard\n");
    return -1;
  }

  if (m_save_file_flag && m_is_calibrate_done) {
    read(m_fd_hwtimer, pts_buf, sizeof(pts_buf));
    hw_pts = strtoull(pts_buf,(char **)NULL, 10);
    fprintf(m_fd_save_file, "===pts:%llu\n", hw_pts);
    lseek(m_fd_hwtimer, 0L, SEEK_SET);

    for (i = 0; i < eis_stat->gyro_data_count; i++) {
      fprintf(m_fd_save_file, "%d\t%d\t%d\t%d\t%d\t%d\n",
              eis_stat->gyro_data[i].xg, eis_stat->gyro_data[i].yg,
              eis_stat->gyro_data[i].zg, eis_stat->gyro_data[i].xa,
              eis_stat->gyro_data[i].ya, eis_stat->gyro_data[i].za);
    }
  }
  DEBUG("AMBA_IOC_EIS_GET_STAT: count %d\n", eis_stat->gyro_data_count);
  return 0;
}

int AMEncodeEIS::set_eis_warp(const struct iav_warp_main* warp_main)
{
  iav_warp_ctrl warp_control;
  uint32_t flags = (1 << IAV_WARP_CTRL_MAIN);

  memset(&warp_control, 0, sizeof(struct iav_warp_ctrl));
  warp_control.cid = IAV_WARP_CTRL_MAIN;
  memcpy(&warp_control.arg.main, warp_main, sizeof(struct iav_warp_main));

  DEBUG("warp:  input %dx%d+%dx%d, output %dx%d, hor enable %d (%dx%d),"
        " ver enable %d (%dx%d)\n",
        warp_main->area[0].input.width,
        warp_main->area[0].input.height,
        warp_main->area[0].input.x,
        warp_main->area[0].input.y,
        warp_main->area[0].output.width,
        warp_main->area[0].output.height,
        warp_main->area[0].h_map.enable,
        warp_main->area[0].h_map.output_grid_row,
        warp_main->area[0].h_map.output_grid_col,
        warp_main->area[0].v_map.enable,
        warp_main->area[0].v_map.output_grid_row,
        warp_main->area[0].v_map.output_grid_col);

  if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_CFG_WARP_CTRL, &warp_control) < 0)) {
    PERROR("IAV_IOC_CFG_WARP_CTRL");
    return -1;
  }

  if (AM_UNLIKELY(ioctl(m_fd_iav, IAV_IOC_APPLY_WARP_CTRL, &flags) < 0)) {
    PERROR("IAV_IOC_APPLY_WARP_CTRL");
    return -1;
  }

  return 0;
}

AM_RESULT AMEncodeEIS::set_eis_mode(int32_t mode)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (AM_UNLIKELY(mode < 0)) {
      ERROR("Invalid EIS mode %d", mode);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config_param.eis_mode.first = true;
    m_config_param.eis_mode.second = EIS_FLAG(mode);
  } while(0);

  return ret;
}

AM_RESULT AMEncodeEIS::get_eis_mode(int32_t &mode)
{
  mode = (int32_t)m_config_param.eis_mode.second;
  return AM_RESULT_OK;
}
