/**
 * am_video_reader.cpp
 *
 *  History:
 *    Aug 10, 2015 - [Shupeng Ren] created file
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
#include "am_define.h"
#include "am_log.h"

#include "am_platform_if.h"
#include "am_video_reader_if.h"
#include "am_video_reader.h"

#define  AUTO_LOCK_VREADER(mtx)  std::lock_guard<std::recursive_mutex> lck(mtx)

AMVideoReader *AMVideoReader::m_instance = nullptr;
std::recursive_mutex AMVideoReader::m_mtx;

AMIVideoReaderPtr AMIVideoReader::get_instance()
{
  return AMVideoReader::get_instance();
}

AMVideoReader* AMVideoReader::get_instance()
{
  AUTO_LOCK_VREADER(m_mtx);
  if (!m_instance) {
    m_instance = AMVideoReader::create();
  }
  return m_instance;
}

void AMVideoReader::inc_ref()
{
  ++m_ref_cnt;
}

void AMVideoReader::release()
{
  AUTO_LOCK_VREADER(m_mtx);
  if ((m_ref_cnt > 0) && (--m_ref_cnt == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMVideoReader* AMVideoReader::create()
{
  AMVideoReader *result = new AMVideoReader();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMVideoReader::init()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }
  } while (0);

  return result;
}

AMVideoReader::AMVideoReader() :
    m_ref_cnt(0)
{

}

AMVideoReader::~AMVideoReader()
{
  m_platform = nullptr;
}

AM_RESULT AMVideoReader::query_video_frame(AMQueryFrameDesc &desc,
                                           uint32_t timeout)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameInfo frame;
    frame.type = AM_DATA_FRAME_TYPE_VIDEO;
    frame.timeout = timeout;
    if ((result = m_platform->query_frame(frame)) != AM_RESULT_OK) {
      switch(result) {
        case AM_RESULT_ERR_IO:
          WARN("Operation is interrupted!");
          break;
        case AM_RESULT_ERR_AGAIN:
          NOTICE("Data is not ready, try again!");
          break;
        case AM_RESULT_ERR_DSP:
          ERROR("IAV internal error!");
          break;
        default:
          ERROR("Failed to query video frame! %d", result);
          break;
      }
      break;
    }
    desc = frame.desc;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::query_yuv_frame(AMQueryFrameDesc &desc,
                                         AM_SOURCE_BUFFER_ID id,
                                         bool latest_snapshot)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameInfo frame;
    frame.type = AM_DATA_FRAME_TYPE_YUV;
    frame.buf_id = id;
    frame.latest = latest_snapshot;
    if ((result = m_platform->query_frame(frame)) != AM_RESULT_OK) {
      switch(result) {
        case AM_RESULT_ERR_IO:
          WARN("Operation is interrupted!");
          break;
        case AM_RESULT_ERR_AGAIN:
          NOTICE("Data is not ready, try again!");
          break;
        case AM_RESULT_ERR_DSP:
          ERROR("IAV internal error!");
          break;
        default:
          ERROR("Failed to query video frame! %d", result);
          break;
      }
      break;
    }
    desc = frame.desc;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::query_me0_frame(AMQueryFrameDesc &desc,
                                         AM_SOURCE_BUFFER_ID id,
                                         bool latest_snapshot)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameInfo frame;
    frame.type = AM_DATA_FRAME_TYPE_ME0;
    frame.buf_id = id;
    frame.latest = latest_snapshot;
    if ((result = m_platform->query_frame(frame)) != AM_RESULT_OK) {
      switch(result) {
        case AM_RESULT_ERR_IO:
          WARN("Operation is interrupted!");
          break;
        case AM_RESULT_ERR_AGAIN:
          NOTICE("Data is not ready, try again!");
          break;
        case AM_RESULT_ERR_DSP:
          ERROR("IAV internal error!");
          break;
        default:
          ERROR("Failed to query video frame! %d", result);
          break;
      }
      break;
    }
    desc = frame.desc;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::query_me1_frame(AMQueryFrameDesc &desc,
                                         AM_SOURCE_BUFFER_ID id,
                                         bool latest_snapshot)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameInfo frame;
    frame.type = AM_DATA_FRAME_TYPE_ME1;
    frame.buf_id = id;
    frame.latest = latest_snapshot;
    if ((result = m_platform->query_frame(frame)) != AM_RESULT_OK) {
      switch(result) {
        case AM_RESULT_ERR_IO:
          WARN("Operation is interrupted!");
          break;
        case AM_RESULT_ERR_AGAIN:
          NOTICE("Data is not ready, try again!");
          break;
        case AM_RESULT_ERR_DSP:
          ERROR("IAV internal error!");
          break;
        default:
          ERROR("Failed to query video frame! %d", result);
          break;
      }
      break;
    }
    desc = frame.desc;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::query_raw_frame(AMQueryFrameDesc &desc)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformFrameInfo frame;
    frame.type = AM_DATA_FRAME_TYPE_RAW;
    if ((result = m_platform->query_frame(frame)) != AM_RESULT_OK) {
      switch(result) {
        case AM_RESULT_ERR_IO:
          WARN("Operation is interrupted!");
          break;
        case AM_RESULT_ERR_AGAIN:
          NOTICE("Data is not ready, try again!");
          break;
        case AM_RESULT_ERR_DSP:
          ERROR("IAV internal error!");
          break;
        default:
          ERROR("Failed to query video frame! %d", result);
          break;
      }
      break;
    }
    desc = frame.desc;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::query_stream_info(AMStreamInfo &info)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    bool is_ok = true;
    AMPlatformStreamFormat stm_fmt;
    stm_fmt.id = info.stream_id;

    result = m_platform->stream_format_get(stm_fmt);
    if (AM_UNLIKELY(result != AM_RESULT_OK)) {
      ERROR("Failed to get stream format for stream%d", info.stream_id);
      break;
    }
    info.mul   = stm_fmt.fps.mul;
    info.div   = stm_fmt.fps.div;

    switch(stm_fmt.type) {
      case AM_STREAM_TYPE_H264: {
        AMPlatformH26xConfig h264_conf;
        h264_conf.id = info.stream_id;
        result = m_platform->stream_h264_config_get(h264_conf);
        if (AM_UNLIKELY(result != AM_RESULT_OK)) {
          ERROR("Failed to get H.264 config for stream%u", info.stream_id);
          is_ok = false;
        } else {
          info.m     = h264_conf.M;
          info.n     = h264_conf.N;
          info.rate  = h264_conf.rate;
          info.scale = h264_conf.scale;
        }
      }break;
      case AM_STREAM_TYPE_MJPEG: {
        info.m     = 0;
        info.n     = 0;
        info.rate  = 0;
        info.scale = 0;
      }break;
      case AM_STREAM_TYPE_H265: {
        AMPlatformH26xConfig h265_conf;
        h265_conf.id = info.stream_id;
        result = m_platform->stream_h265_config_get(h265_conf);
        if (AM_UNLIKELY(result != AM_RESULT_OK)) {
          ERROR("Failed to get H.265 config for stream%u", info.stream_id);
          is_ok = false;
        } else {
          info.m     = h265_conf.M;
          info.n     = h265_conf.N;
          info.rate  = h265_conf.rate;
          info.scale = h265_conf.scale;
        }
      }break;
      default: break;
    }

    if (AM_UNLIKELY(!is_ok)) {
      break;
    }
  } while (0);

  return result;
}

bool AMVideoReader::is_gdmacpy_support()
{
  bool result = true;
  AMMemMapInfo usr_addr;

  do {
    if (m_platform->map_usr(usr_addr) != AM_RESULT_OK) {
      ERROR("Failed to map usr!");
      result = false;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::gdmacpy(void *dst, const void *src,
                                 size_t width, size_t height, size_t pitch)
{
  AUTO_LOCK_VREADER(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_platform->gdmacpy(dst, src, width, height, pitch))
                != AM_RESULT_OK) {
      ERROR("Failed to gdmacpy!");
      break;
    }
  } while (0);

  return result;
}
