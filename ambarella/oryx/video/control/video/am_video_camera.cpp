/**
 * am_video_camera.cpp
 *
 *  History:
 *    Jul 22, 2015 - [Shupeng Ren] created file
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

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_encode_config.h"
#include "am_encode_device.h"
#include "am_video_camera.h"

#define  AUTO_LOCK_CAM(mtx)  std::lock_guard<std::recursive_mutex> lck(mtx)

AMVideoCamera *AMVideoCamera::m_instance = nullptr;
std::recursive_mutex AMVideoCamera::m_mtx;

AMIVideoCameraPtr AMIVideoCamera::get_instance()
{
  return AMVideoCamera::get_instance();
}

AMVideoCamera* AMVideoCamera::get_instance()
{
  AUTO_LOCK_CAM(m_mtx);
  if (!m_instance) {
    m_instance = AMVideoCamera::create();
  }
  return m_instance;
}

void AMVideoCamera::inc_ref()
{
  ++m_ref_cnt;
}

void AMVideoCamera::release()
{
  AUTO_LOCK_CAM(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMVideoCamera* AMVideoCamera::create()
{
  AMVideoCamera *result = new AMVideoCamera();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMVideoCamera::init()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }

    if (!(m_device = AMEncodeDevice::create())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMEncodeDevice!");
      break;
    }

    if (!(m_vin_config = AMVinConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMVinConfig!");
      break;
    }

    if (!(m_vout_config = AMVoutConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMVoutConfig!");
      break;
    }

    if (!(m_buffer_config = AMBufferConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMBufferConfig!");
      break;
    }

    if (!(m_stream_config = AMStreamConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMStreamConfig!");
      break;
    }
  } while (0);

  return result;
}

AMVideoCamera::AMVideoCamera() :
    m_device(nullptr),
    m_ref_cnt(0),
    m_platform(nullptr),
    m_vin_config(nullptr),
    m_buffer_config(nullptr),
    m_stream_config(nullptr)
{
  DEBUG("AMVideoCamera is created!");
}

AMVideoCamera::~AMVideoCamera()
{
  AM_DESTROY(m_device);
  m_vin_config = nullptr;
  m_buffer_config = nullptr;
  m_stream_config = nullptr;
  DEBUG("AMVideoCamera is destroyed!");
}

AM_RESULT AMVideoCamera::start()
{
  return m_device->start();
}

AM_RESULT AMVideoCamera::start_stream(AM_STREAM_ID id)
{
  return m_device->start_stream(id);
}

AM_RESULT AMVideoCamera::stop()
{
  return m_device->stop();
}

AM_RESULT AMVideoCamera::stop_stream(AM_STREAM_ID id)
{
  return m_device->stop_stream(id);
}

AM_RESULT AMVideoCamera::idle()
{
  return m_device->idle();
}

AM_RESULT AMVideoCamera::preview()
{
  return m_device->preview();
}

AM_RESULT AMVideoCamera::encode()
{
  return m_device->encode();
}

AM_RESULT AMVideoCamera::decode()
{
  return m_device->decode();
}

AM_RESULT AMVideoCamera::get_vin_status(AMVinInfo &vin)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((ret = m_platform->vin_params_get(vin)) != AM_RESULT_OK) {
      break;
    }
  } while(0);
  return ret;
}

AM_RESULT AMVideoCamera::get_stream_status(AM_STREAM_ID id,
                                           AM_STREAM_STATE &state)
{
  return m_device->get_stream_status(id, state);
}

AM_RESULT AMVideoCamera::get_buffer_state(AM_SOURCE_BUFFER_ID id,
                                          AM_SRCBUF_STATE &state)
{
  return m_device->get_buffer_state(id, state);
}

AM_RESULT AMVideoCamera::get_buffer_format(AMBufferConfigParam &param)
{
  return m_device->get_buffer_format(param);
}

AM_RESULT AMVideoCamera::set_buffer_format(const AMBufferConfigParam &param)
{
  return m_device->set_buffer_format(param);
}

AM_RESULT AMVideoCamera::save_buffer_config()
{
  return m_device->save_buffer_config();
}

uint32_t  AMVideoCamera::get_encode_stream_max_num()
{
  return m_platform->system_max_stream_num_get();
}

uint32_t  AMVideoCamera::get_source_buffer_max_num()
{
  return m_platform->system_max_buffer_num_get();
}

AM_RESULT AMVideoCamera::get_bitrate(AMBitrate &bitrate)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformBitrate pbitrate;
  memset(&pbitrate, 0, sizeof(pbitrate));

  do {
    pbitrate.id = bitrate.stream_id;
    result = m_platform->stream_bitrate_get(pbitrate);
    if (result != AM_RESULT_OK) {
      break;
    }
    bitrate.rate_control_mode = pbitrate.rate_control_mode;
    bitrate.target_bitrate = pbitrate.target_bitrate;
    bitrate.vbr_min_bitrate = pbitrate.vbr_min_bitrate;
    bitrate.vbr_max_bitrate = pbitrate.vbr_max_bitrate;
    bitrate.i_frame_max_size = pbitrate.i_frame_max_size;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_bitrate(const AMBitrate &bitrate)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = bitrate.stream_id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    if ((AM_STREAM_TYPE_H264 != format.type) &&
        (AM_STREAM_TYPE_H265 != format.type)) {
      WARN("Encode type is not H26x!\n");
      break;
    }
    AMPlatformBitrate pbitrate;
    pbitrate.id = bitrate.stream_id;
    pbitrate.rate_control_mode = bitrate.rate_control_mode;
    pbitrate.target_bitrate = bitrate.target_bitrate;
    pbitrate.vbr_min_bitrate = bitrate.vbr_min_bitrate;
    pbitrate.vbr_max_bitrate = bitrate.vbr_max_bitrate;
    pbitrate.i_frame_max_size = bitrate.i_frame_max_size;
    AMStreamConfigParam br_param;
    br_param.h26x_config.first = true;
    br_param.h26x_config.second.bitrate_control.first = true;
    br_param.h26x_config.second.bitrate_control.second = bitrate.rate_control_mode;
    br_param.h26x_config.second.target_bitrate.first = true;
    br_param.h26x_config.second.target_bitrate.second = bitrate.target_bitrate;
    do {
      if ((result = m_device->set_stream_param(bitrate.stream_id, br_param)) !=
          AM_RESULT_OK) {
        ERROR("Set config failed");
        break;
      }

      if ((result = m_platform->stream_bitrate_set(pbitrate)) != AM_RESULT_OK) {
        ERROR("bitrate set failed");
        break;
      }

    } while (0);
    return result;
  } while(0);

  return result;
}

AM_RESULT AMVideoCamera::get_framerate(AMFramerate &rate)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformFrameRate pframerate;

  do {
    pframerate.id = rate.stream_id;
    result = m_platform->stream_framerate_get(pframerate);
    if (result != AM_RESULT_OK) {
      break;
    }
    rate.fps = pframerate.fps;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_framerate(const AMFramerate &rate)
{
  AM_RESULT ret;
  AMPlatformFrameRate pframerate;
  pframerate.id = rate.stream_id;
  pframerate.fps = rate.fps;
  AMStreamConfigParam fr_param;
  fr_param.stream_format.first = true;
  fr_param.stream_format.second.fps.first = true;
  fr_param.stream_format.second.fps.second = rate.fps;
  do {
      if ((ret = m_device->set_stream_param(rate.stream_id, fr_param)) !=
          AM_RESULT_OK) {
        ERROR("Set config failed");
        break;
      }

      if ((ret = m_platform->stream_framerate_set(pframerate)) != AM_RESULT_OK) {
        ERROR("framerate set failed");
        break;
      }

    } while (0);
    return ret;
}

AM_RESULT AMVideoCamera::get_mjpeg_info(AMMJpegInfo &mjpeg)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformMJPEGConfig pmjpeg;

  do {
    pmjpeg.id = mjpeg.stream_id;
    result = m_platform->stream_mjpeg_quality_get(pmjpeg);
    if (result != AM_RESULT_OK) {
      break;
    }
    mjpeg.quality = pmjpeg.quality;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_mjpeg_info(const AMMJpegInfo &mjpeg)
{
  AM_RESULT ret = AM_RESULT_OK;
  AMPlatformMJPEGConfig pmjpeg;
  pmjpeg.id = mjpeg.stream_id;
  pmjpeg.quality = mjpeg.quality;
  AMStreamConfigParam mj_param;
  mj_param.mjpeg_config.first = true;
  mj_param.mjpeg_config.second.quality.first = true;
  mj_param.mjpeg_config.second.quality.second = mjpeg.quality;
  do {
    if ((ret = m_device->set_stream_param(mjpeg.stream_id, mj_param)) !=
        AM_RESULT_OK) {
      ERROR("Set config failed");
      break;
    }

    if ((ret = m_platform->stream_mjpeg_quality_set(pmjpeg)) != AM_RESULT_OK) {
      ERROR("mjpeg quality set failed");
      break;
    }

  } while (0);
  return ret;
}

AM_RESULT AMVideoCamera::get_h26x_gop(AMGOP &h26x)
{
  AM_RESULT result = AM_RESULT_OK;
  AMPlatformH26xConfig ph26x;

  do {
    ph26x.id = h26x.stream_id;
    result = m_platform->stream_h26x_gop_get(ph26x);
    if (result != AM_RESULT_OK) {
      break;
    }
    h26x.N = ph26x.N;
    h26x.idr_interval = ph26x.idr_interval;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_h26x_gop(const AMGOP &h26x)
{
  AM_RESULT ret;
  AMPlatformH26xConfig ph264;
  ph264.id = h26x.stream_id;
  ph264.N = h26x.N;
  ph264.idr_interval = h26x.idr_interval;
  AMStreamConfigParam h26x_param;
  h26x_param.h26x_config.first = true;
  h26x_param.h26x_config.second.N.first = true;
  h26x_param.h26x_config.second.idr_interval.first = true;
  h26x_param.h26x_config.second.N.second = h26x.N;
  h26x_param.h26x_config.second.idr_interval.second = h26x.idr_interval;
  do {
    if ((ret = m_device->set_stream_param(h26x.stream_id, h26x_param)) !=
        AM_RESULT_OK) {
      ERROR("Set config failed");
      break;
    }

    if ((ret = m_platform->stream_h26x_gop_set(ph264)) != AM_RESULT_OK) {
      ERROR("mjpeg quality set failed");
      break;
    }

  } while (0);
  return ret;
}

AM_RESULT AMVideoCamera::get_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    type = format.type;
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::set_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.type.first = true;
    param.stream_format.second.type.second = type;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_size(AM_STREAM_ID id, AMResolution &res)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    res = format.enc_win.size;
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::set_stream_size(AM_STREAM_ID id, AMResolution &res)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.enc_win.first = true;
    param.stream_format.second.enc_win.second.size = res;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_offset(AM_STREAM_ID id, AMOffset &offset)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    result = m_platform->stream_offset_get(id, offset);
    if (result != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_stream_offset(AM_STREAM_ID id, const AMOffset &offset)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.enc_win.first = true;
    param.stream_format.second.enc_win.second.offset = offset;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_source(AM_STREAM_ID id,
                                           AM_SOURCE_BUFFER_ID &source)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    source = format.source;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_stream_source(AM_STREAM_ID id,
                                           AM_SOURCE_BUFFER_ID &source)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.source.first = true;
    param.stream_format.second.source.second = source;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    flip = format.flip;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.flip.first = true;
    param.stream_format.second.flip.second = flip;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_rotate(AM_STREAM_ID id, AM_VIDEO_ROTATE &rot)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    rot = AM_VIDEO_ROTATE(format.rotate);
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_stream_rotate(AM_STREAM_ID id, AM_VIDEO_ROTATE &rot)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.stream_format.first = true;
    param.stream_format.second.rotate_90_cw.first = true;
    param.stream_format.second.rotate_90_cw.second = rot;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_stream_profile(AM_STREAM_ID id, AM_PROFILE &profile)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat format;
    format.id = id;
    if ((result = m_platform->stream_format_get(format)) != AM_RESULT_OK) {
      break;
    }
    AMPlatformH26xConfig conf;
    conf.id = id;
    if (format.type == AM_STREAM_TYPE_H264) {
      if ((result = m_platform->stream_h264_config_get(conf)) != AM_RESULT_OK) {
        break;
      }
    } else if (format.type == AM_STREAM_TYPE_H265) {
      if ((result = m_platform->stream_h265_config_get(conf)) != AM_RESULT_OK) {
        break;
      }
    }
    profile = conf.profile;
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::set_stream_profile(AM_STREAM_ID id, AM_PROFILE &profile)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamConfigParam param;
    param.h26x_config.first = true;
    param.h26x_config.second.profile_level.first = true;
    param.h26x_config.second.profile_level.second = profile;
    if ((result = m_device->set_stream_param(id, param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::save_stream_config()
{
  return m_device->save_stream_config();
}

AM_RESULT AMVideoCamera::set_power_mode(AM_POWER_MODE mode)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_POWER_MODE cur_mode = AM_POWER_MODE_HIGH;
    if ((result = m_platform->get_power_mode(cur_mode)) != AM_RESULT_OK) {
      break;
    }
    if (cur_mode == mode) {
      break;
    }

    if (mode == AM_POWER_MODE_LOW) {
      if ((result = m_device->goto_low_power_mode())
          != AM_RESULT_OK) {
        break;
      }
    } else {
      if ((result = m_device->encode()) != AM_RESULT_OK) {
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_power_mode(AM_POWER_MODE &mode)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_platform->get_power_mode(mode)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_avail_cpu_clks(std::map<int32_t, int32_t> &clks)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    vector<int32_t> tem_clks;
    if ((result = m_platform->get_avail_cpu_clks(tem_clks)) != AM_RESULT_OK) {
      break;
    }
    int clks_len = tem_clks.size();
    for (int i = 0; i < clks_len; i++) {
      clks[i] = tem_clks[i];
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::get_cur_cpu_clk(int32_t &clk)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_platform->get_cur_cpu_clk(clk)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::set_cpu_clk(int32_t index)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_platform->set_cpu_clk(index)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoCamera::stop_vin()
{
  return m_device->idle();
}

AM_RESULT AMVideoCamera::halt_vout(AM_VOUT_ID id)
{
  return m_device->halt_vout(id);
}

AM_RESULT AMVideoCamera::force_idr(AM_STREAM_ID stream_id)
{
  return m_device->force_idr(stream_id);
}

AM_RESULT  AMVideoCamera::get_ldc_state(bool &state)
{
  return m_platform->check_ldc_enable(state);
}

AM_RESULT AMVideoCamera::load_config_all()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_device->load_config_all()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVideoCamera::get_feature_config(AMFeatureParam &config)
{
  return m_platform->feature_config_get(config);
}

AM_RESULT AMVideoCamera::set_feature_config(const AMFeatureParam &config)
{
  return m_platform->feature_config_set(config);
}

AM_RESULT AMVideoCamera::get_vin_config(AMVinParamMap &config)
{
  return m_vin_config->get_config(config);
}

AM_RESULT AMVideoCamera::set_vin_config(const AMVinParamMap &config)
{
  return m_vin_config->set_config(config);
}

AM_RESULT AMVideoCamera::get_vout_config(AMVoutParamMap &config)
{
  return m_vout_config->get_config(config);
}

AM_RESULT AMVideoCamera::set_vout_config(const AMVoutParamMap &config)
{
  return m_vout_config->set_config(config);
}

AM_RESULT AMVideoCamera::get_buffer_config(AMBufferParamMap &config)
{
  return m_buffer_config->get_config(config);
}

AM_RESULT AMVideoCamera::set_buffer_config(const AMBufferParamMap &config)
{
  return m_buffer_config->set_config(config);
}

AM_RESULT AMVideoCamera::get_stream_config(AMStreamParamMap &config)
{
  return m_stream_config->get_config(config);
}

AM_RESULT AMVideoCamera::set_stream_config(const AMStreamParamMap &config)
{
  return m_stream_config->set_config(config);
}

void* AMVideoCamera::get_video_plugin(const std::string& plugin_name)
{
  return m_device->get_video_plugin_interface(plugin_name);
}
