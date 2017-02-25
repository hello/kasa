/**
 * am_encode_stream.cpp
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
#include "am_video_utility.h"
#include "am_encode_config.h"
#include "am_encode_stream.h"

AMEncodeStream* AMEncodeStream::create()
{
  AMEncodeStream *result = new AMEncodeStream();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMEncodeStream::destroy()
{
  delete this;
}

AM_RESULT AMEncodeStream::init()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!(m_config = AMStreamConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMStreamConfig!");
      break;
    }

    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform");
      break;
    }
  } while (0);

  return result;
}

AMEncodeStream::AMEncodeStream() :
    m_config(nullptr),
    m_platform(nullptr)
{
  DEBUG("AMEncodeStream is created!");
}

AMEncodeStream::~AMEncodeStream()
{
  m_platform = nullptr;
  m_config = nullptr;
  DEBUG("AMEncodeStream is destroyed!");
}

AM_RESULT AMEncodeStream::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMStreamConfigPtr is null!");
      break;
    }
    if ((result = m_config->get_config(m_param)) != AM_RESULT_OK) {
      ERROR("Failed to get stream config!");
      break;
    }
    /* Initialize all the stream's status */
    check_stream_state(AM_STREAM_ID_MAX);
  } while (0);

  return result;
}

AM_RESULT AMEncodeStream::get_param(AMStreamParamMap &param)
{
  std::lock_guard<std::mutex> lock(m_conf_mutex);
  param = m_param;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeStream::set_param(const AMStreamParamMap &param)
{
  std::lock_guard<std::mutex> lock(m_conf_mutex);
  m_param = param;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeStream::save_config()
{
  return m_config->set_config(m_param);
}

const AMStreamFormatConfig& AMEncodeStream::stream_config(AM_STREAM_ID id)
{
  return m_param[id].stream_format.second;
}

AM_RESULT AMEncodeStream::setup(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (id == AM_STREAM_ID_INVALID) {
      result = AM_RESULT_ERR_PERM;
      ERROR("Invalid stream id %d)!", id);
      break;
    }
    for (auto &v : m_param) {
      AMPlatformStreamFormat format;
      AMPlatformH26xConfig h264;
      AMPlatformH26xConfig h265;
      AMPlatformMJPEGConfig mjpeg;
      AMStreamFormatConfig &stream_format_conf = v.second.stream_format.second;

      if ((AM_STREAM_ID_MAX != id) && (v.first != id)) {
        continue;
      }
      if (!v.second.stream_format.second.enable.second) {
        PRINTF("Stream[%d] is not enabled!", v.first);
        continue;
      }

      format.id      = v.first;
      format.type    = stream_format_conf.type.second;
      format.source  = stream_format_conf.source.second;
      format.fps.fps = stream_format_conf.fps.second;
      format.enc_win = stream_format_conf.enc_win.second;
      format.flip    = stream_format_conf.flip.second;
      format.rotate  = stream_format_conf.rotate_90_cw.second;
      if ((result = m_platform->stream_format_set(format)) != AM_RESULT_OK) {
        ERROR("Failed to set stream format!");
        break;
      }

      PRINTF("Stream[%d] type: %s, size: %dx%d, offset: %dx%d.",
             v.first,
             AMVideoTrans::stream_type_enum_to_str(format.type).c_str(),
             stream_format_conf.enc_win.second.size.width,
             stream_format_conf.enc_win.second.size.height,
             stream_format_conf.enc_win.second.offset.x,
             stream_format_conf.enc_win.second.offset.y);
      switch (format.type) {
        case AM_STREAM_TYPE_H264: {
          AMStreamH26xConfig &h264_config = v.second.h26x_config.second;
          h264.id = v.first;
          h264.profile         = h264_config.profile_level.second;
          h264.au_type         = h264_config.au_type.second;
          h264.gop_model       = h264_config.gop_model.second;
          h264.chroma_format   = h264_config.chroma_format.second;
          h264.M               = h264_config.M.second;
          h264.N               = h264_config.N.second;
          h264.target_bitrate  = h264_config.target_bitrate.second;
          h264.idr_interval    = h264_config.idr_interval.second;
          h264.mv_threshold    = h264_config.mv_threshold.second;
          h264.fast_seek_intvl = h264_config.fast_seek_intvl.second;
          h264.flat_area_improve  = h264_config.flat_area_improve.second;
          h264.multi_ref_p     = h264_config.multi_ref_p.second;
          h264.sar_width       = h264_config.sar.second.width;
          h264.sar_height      = h264_config.sar.second.height;
          h264.i_frame_max_size= h264_config.i_frame_max_size.second >= 0 ?
              h264_config.i_frame_max_size.second :
              adjust_i_frame_max_size_with_res(
                  stream_format_conf.enc_win.second.size);
          if ((result = m_platform->stream_h264_config_set(h264)) !=
              AM_RESULT_OK) {
            ERROR("Failed to set H.264 config!");
          }
        }break;
        case AM_STREAM_TYPE_H265: {
          AMStreamH26xConfig &h265_config = v.second.h26x_config.second;
          h265.id = v.first;
          h265.profile = h265_config.profile_level.second;
          if (AM_PROFILE_MAIN != h265.profile) {
            WARN("Only H.265 \"MAIN\" profile is support! Reset to \"main\"!");
            h265.profile = AM_PROFILE_MAIN;
          }
          h265.au_type         = h265_config.au_type.second;
          h265.gop_model       = h265_config.gop_model.second;
          h265.chroma_format   = h265_config.chroma_format.second;
          h265.M               = h265_config.M.second;
          h265.N               = h265_config.N.second;
          h265.target_bitrate  = h265_config.target_bitrate.second;
          h265.idr_interval    = h265_config.idr_interval.second;
          h265.mv_threshold    = h265_config.mv_threshold.second;
          h265.fast_seek_intvl = h265_config.fast_seek_intvl.second;
          h265.multi_ref_p     = h265_config.multi_ref_p.second;
          h265.sar_width       = h265_config.sar.second.width;
          h265.sar_height      = h265_config.sar.second.height;
          h265.i_frame_max_size= h265_config.i_frame_max_size.second >= 0 ?
              h265_config.i_frame_max_size.second :
              adjust_i_frame_max_size_with_res(
                  stream_format_conf.enc_win.second.size);
          if ((result = m_platform->stream_h265_config_set(h265)) !=
              AM_RESULT_OK) {
            ERROR("Failed to set H.265 config!");
          }
        }break;
        case AM_STREAM_TYPE_MJPEG: {
          AMStreamMJPEGConfig &mjpeg_config = v.second.mjpeg_config.second;
          mjpeg.id = v.first;
          mjpeg.quality = mjpeg_config.quality.second;
          mjpeg.chroma_format = mjpeg_config.chroma_format.second;
          if ((result = m_platform->stream_mjpeg_config_set(mjpeg)) !=
              AM_RESULT_OK) {
            ERROR("Failed to set mjpeg config");
          }
        }break;
        default:
          result = AM_RESULT_ERR_INVALID;
          ERROR("Invalid Video type: %d", format.type);
          break;
      }
      if (AM_UNLIKELY(result != AM_RESULT_OK)) {
        break;
      }
    }
    if (result != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeStream::start_encode(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  uint32_t stream_bits = 0;

  if (AM_STREAM_ID_MAX == id) {
    check_stream_state(id);
    for (auto &m : m_param) {
      if (m.second.stream_format.second.enable.second &&
          (AM_STREAM_STATE_IDLE == m_stream_state[m.first])) {
        stream_bits |= 1 << m.first;
        PRINTF(">>>Starting to encode stream[%d]...", m.first);
      }
    }
  } else if (AM_STREAM_ID_INVALID != id) {
    stream_bits |= 1 << id;
  }

  do {
    if (stream_bits == 0) {
      WARN("No streams are enabled!");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if ((result = m_platform->encode_start(stream_bits)) != AM_RESULT_OK) {
      ERROR("Failed to start encoding!");
      break;
    }
    check_stream_state(id);
    for (auto &m : m_param) {
      if ((AM_STREAM_ID_MAX != id) && (m.first != id)) {
        continue;
      }
      if (m.second.stream_format.second.enable.second) {
        if (m_stream_state[m.first] == AM_STREAM_STATE_STARTING) {
          PRINTF(">>>Stream[%d] is starting...", m.first);
        }
        if (m_stream_state[m.first] == AM_STREAM_STATE_ENCODING) {
          PRINTF(">>>Stream[%d] is encoding...", m.first);
        }
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeStream::stop_encode(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  uint32_t stream_bits = 0;

  if (AM_STREAM_ID_MAX == id) {
    check_stream_state(id);
    for (auto &m : m_param) {
      if ((m.second.stream_format.second.enable.second) &&
          (AM_STREAM_STATE_ENCODING == m_stream_state[m.first])) {
        stream_bits |= 1 << m.first;
        PRINTF(">>>Starting to stop stream[%d]...", m.first);
      }
    }
  } else if (AM_STREAM_ID_INVALID != id) {
    stream_bits |= 1 << id;
  }

  do {
    if (stream_bits == 0) {
      WARN("No streams are enabled!");
      break;
    }
    if ((result = m_platform->encode_stop(stream_bits)) != AM_RESULT_OK) {
      ERROR("Failed to stop encoding!");
      break;
    }
    check_stream_state(id);
    for (auto &m : m_param) {
      if ((AM_STREAM_ID_MAX != id) && (m.first != id)) {
        continue;
      }
      if (m.second.stream_format.second.enable.second &&
          (m_stream_state[m.first] == AM_STREAM_STATE_IDLE)) {
        PRINTF(">>>Stream[%d] is stopped.", m.first);
      }
    }
  } while (0);
  return result;
}

void AMEncodeStream::check_stream_state(AM_STREAM_ID id)
{

  do {
    if (AM_STREAM_ID_INVALID == id) {
      ERROR("Invalid stream id %d", id);
      break;
    }

    for (auto &m : m_param) {
      if ((AM_STREAM_ID_MAX != id) && (AM_STREAM_ID(m.first) != id)) {
        continue;
      }
      if (m.second.stream_format.second.enable.second) {
        AM_STREAM_STATE state;
        AM_RESULT result = m_platform->stream_state_get(AM_STREAM_ID(m.first),
                                                        state);
        if (AM_RESULT_OK == result) {
          m_stream_state[AM_STREAM_ID(m.first)] = state;
        } else {
          ERROR("Failed to query stream[%d] status!", m.first);
        }
      } else {
        if (AM_LIKELY(id != AM_STREAM_ID_MAX)) {
          NOTICE("Stream%d is not enabled!", id);
        }
        m_stream_state[AM_STREAM_ID(m.first)] = AM_STREAM_STATE_IDLE;
      }
    }
  }while(0);
}

const AMStreamStateMap& AMEncodeStream::stream_state()
{
  return m_stream_state;
}

uint32_t  AMEncodeStream::adjust_i_frame_max_size_with_res(AMResolution res)
{
  uint32_t size = 0;
  do {
    uint32_t stream_size = res.width * res.height;
    if (stream_size >= 8000000) {
      size = 550;
    } else if (stream_size >= 6000000) {
      size = 500;
    } else if (stream_size >= 4000000) {
      size = 450;
    } else if (stream_size >= 2000000) {
      size = 400;
    } else if (stream_size >= 1000000) {
      size = 300;
    } else {
      size = 200;
    }
  } while(0);

  return size;
}
