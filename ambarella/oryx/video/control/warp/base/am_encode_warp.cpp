/*******************************************************************************
 * am_encode_warp.cpp
 *
 * History:
 *   Oct 15, 2015 - [zfgong] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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

#include "am_encode_warp.h"
#include "am_encode_stream.h"
#include "am_video_utility.h"
#include "am_configure.h"
#include "am_vin.h"

void AMEncodeWarp::destroy()
{
  if (AM_LIKELY(m_platform != nullptr)) {
    m_platform->unmap_warp();
  }
  delete this;
}

bool AMEncodeWarp::start(AM_STREAM_ID id)
{
  if (AM_LIKELY(!m_is_running)) {
    m_is_running = (AM_RESULT_OK == apply());
  }

  return m_is_running;
}

bool AMEncodeWarp::stop(AM_STREAM_ID id)
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
  m_is_running = !need_stop;
  if (!need_stop) {
    INFO("No need to stop!");
  }

  return true;
}

std::string& AMEncodeWarp::name()
{
  return m_name;
}

AMEncodeWarp::AMEncodeWarp(const char *name) :
  m_pixel_width_um(0.0),
  m_vin(nullptr),
  m_stream(nullptr),
  m_iav(-1),
  m_ldc_enable(false),
  m_ldc_checked(false),
  m_is_running(false),
  m_name(name),
  m_platform(nullptr)
{
  memset(&m_distortion_lut, 0, sizeof(m_distortion_lut));
}

AMEncodeWarp::~AMEncodeWarp()
{
  if (m_iav >= 0) {
    close(m_iav);
  }
  m_platform = nullptr;
}

AM_RESULT AMEncodeWarp::init(AMVin *vin, AMEncodeStream *stream)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_vin = vin;
    if (AM_UNLIKELY(!m_vin)) {
      ERROR("Invalid VIN device!");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    m_stream = stream;
    if (AM_UNLIKELY(!m_stream)) {
      ERROR("Invalid Encode Stream device!");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (AM_UNLIKELY(!(m_platform = AMIPlatform::get_instance()))) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to get AMIPlatform!");
      break;
    }
    if (AM_UNLIKELY(m_platform->map_warp(m_mem)) < 0) {
      ERROR("Failed to map warp!\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    if ((m_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
      result = AM_RESULT_ERR_IO;
      PERROR("open /dev/iav");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeWarp::set_ldc_mode(int region_id, AM_WARP_MODE mode)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_ldc_mode(int region_id, AM_WARP_MODE &mode)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::set_ldc_strength(int region_id, float strength)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::set_max_radius(int region_id, int radius)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_max_radius(int region_id, int &radius)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_ldc_strength(int region_id, float &strength)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::set_pano_hfov_degree(int region_id, float degree)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_pano_hfov_degree(int region_id, float &degree)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::set_warp_region_yaw_pitch(int region_id,
                                                  int yaw, int pitch)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_warp_region_yaw_pitch(int region_id,
                                                  int &yaw, int &pitch)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::set_hor_ver_zoom(int region_id, AMFrac hor, AMFrac ver)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::get_hor_ver_zoom(int region_id, AMFrac &hor, AMFrac &ver)
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::apply()
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::load_config()
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

AM_RESULT AMEncodeWarp::save_config()
{
  ERROR("Dummy warp, should be implemented in sub-class!");
  return AM_RESULT_ERR_INVALID;
}

bool AMEncodeWarp::is_ldc_enable()
{
  do {
    if (!m_ldc_checked) {
      m_ldc_checked = true;
      if (AM_UNLIKELY(m_platform->check_ldc_enable(m_ldc_enable))) {
        ERROR("Failed to check_ldc_enable!\n");
        break;
      }
    }
  } while (0);
  return m_ldc_enable;
}
