/*******************************************************************************
 * am_dptz.cpp
 *
 * History:
 *   Mar 28, 2016 - [zfgong] created file
 *   Sep 23, 2016 - [Huaiqing Wang] modified file
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

#include "am_video_types.h"
#include "am_dptz.h"

#define LOCK_GUARD(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  return AMDPTZ::create((AMVin*)data);
}
#ifdef __cplusplus
}
#endif

AMDPTZ* AMDPTZ::create(AMVin* vin)
{
  AMDPTZ *result = nullptr;
  do {
    result = new AMDPTZ();
    if (result && (AM_RESULT_OK != result->init(vin))) {
      delete result;
      result = nullptr;
      break;
    }
  } while (0);
  return result;
}

void AMDPTZ::destroy()
{
  delete this;
}

AMDPTZ::AMDPTZ() :
  m_mutex(nullptr),
  m_vin(nullptr),
  m_name("DPTZ"),
  m_platform(nullptr)
{
  m_plat_param_map.clear();
}

AMDPTZ::~AMDPTZ()
{
  delete m_mutex;
}

bool AMDPTZ::start(AM_STREAM_ID UNUSED(id))
{
  bool ret = true;
  do {
    INFO("No need to start!");
  } while (0);
  return ret;
}

bool AMDPTZ::stop(AM_STREAM_ID UNUSED(id))
{
  bool ret = true;
  do {
    INFO("No need to stop!");
  } while (0);
  return ret;
}

std::string& AMDPTZ::name()
{
  return m_name;
}

void* AMDPTZ::get_interface()
{
  return ((AMIDPTZ*)this);
}

AM_RESULT AMDPTZ::init(AMVin *vin)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_vin = vin;
    if (AM_UNLIKELY(!m_vin)) {
      ERROR("Invalid VIN device!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_mutex = new std::recursive_mutex();
    if (AM_UNLIKELY(!m_mutex)) {
      ERROR("Failed to allocate memory for mutex!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to get AMIPlatform!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMDPTZ::set_ratio(AM_SOURCE_BUFFER_ID id, AMDPTZRatio &ratio)
{
  AM_RESULT ret = AM_RESULT_OK;
  LOCK_GUARD(*m_mutex);
  do {
    AMRect window;
    AMPlatformDPTZParam param;
    if ((ret = get_plat_param(id, param)) != AM_RESULT_OK) {
      ERROR("get_plat_param failed!");
      break;
    }

    if (ratio.zoom.first) {
      if (ratio.zoom.second > 8 || ratio.zoom.second <= 0) {
        ERROR("Wrong zoom value %d",ratio.zoom.second);
        ret = AM_RESULT_ERR_INVALID;
        break;
      }
      param.ratio.zoom_factor_x = ratio.zoom.second;
      param.ratio.zoom_factor_y = ratio.zoom.second;
    }
    if (ratio.pan.first) {
      if (ratio.pan.second > 1 || ratio.pan.second < -1) {
        ERROR("Wrong pan value %d",ratio.pan.second);
        ret = AM_RESULT_ERR_INVALID;
        break;
      }
      param.ratio.zoom_center_pos_x = ratio.pan.second;
    }
    if (ratio.tilt.first) {
      if (ratio.tilt.second > 1 || ratio.tilt.second < -1) {
        ERROR("Wrong tilt value %d",ratio.tilt.second);
        ret = AM_RESULT_ERR_INVALID;
        break;
      }
      param.ratio.zoom_center_pos_y = ratio.tilt.second;
    }

    if ((ret = ratio_to_window(id, param.ratio, window)) != AM_RESULT_OK) {
      ERROR("Failed to ratio_to_window");
      break;
    }
    if ((ret = apply(id, window)) != AM_RESULT_OK) {
      ERROR("apply to iav failed!");
      break;
    }

    m_plat_param_map[id] = param;
  } while (0);
  return ret;
}

AM_RESULT AMDPTZ::get_ratio(AM_SOURCE_BUFFER_ID id, AMDPTZRatio &ratio)
{
  AM_RESULT ret = AM_RESULT_OK;
  LOCK_GUARD(*m_mutex);
  do {
    AMPlatformDPTZParam param;
    if ((ret = get_plat_param(id, param)) != AM_RESULT_OK) {
      ERROR("get_plat_param failed!");
      break;
    }
    ratio.pan.second = param.ratio.zoom_center_pos_x;
    ratio.tilt.second = param.ratio.zoom_center_pos_y;
    ratio.zoom.second = std::max(param.ratio.zoom_factor_x,
                                 param.ratio.zoom_factor_y);
    m_plat_param_map[id] = param;
  } while (0);
  return ret;
}

AM_RESULT AMDPTZ::set_size(AM_SOURCE_BUFFER_ID id, AMDPTZSize &size)
{
  AM_RESULT ret = AM_RESULT_OK;
  LOCK_GUARD(*m_mutex);
  do {
    AMPlatformDPTZParam param;
    if ((ret = get_plat_param(id, param)) != AM_RESULT_OK) {
      ERROR("get_plat_param failed!");
      break;
    }

    if (size.x.first) {
      param.window.offset.x = size.x.second;
    }
    if (size.y.first) {
      param.window.offset.y = size.y.second;
    }
    if (size.w.first) {
      param.window.size.width = size.w.second;
    }
    if (size.h.first) {
      param.window.size.height = size.h.second;
    }

    if ((ret = round_window(id, param.window)) != AM_RESULT_OK) {
      ERROR("Failed to round_window");
      break;
    }
    if ((ret = apply(id, param.window)) != AM_RESULT_OK) {
      ERROR("apply to iav failed!");
      break;
    }

    m_plat_param_map[id] = param;
  } while (0);
  return ret;
}

AM_RESULT AMDPTZ::get_size(AM_SOURCE_BUFFER_ID id, AMDPTZSize &size)
{
  AM_RESULT ret = AM_RESULT_OK;
  LOCK_GUARD(*m_mutex);
  do {
    AMPlatformDPTZParam param;
    if ((ret = get_plat_param(id, param)) != AM_RESULT_OK) {
      ERROR("get_plat_param failed!");
      break;
    }
    m_plat_param_map[id] = param;
    size.x.second = param.window.offset.x;
    size.y.second = param.window.offset.y;
    size.w.second = param.window.size.width;
    size.h.second = param.window.size.height;
  } while (0);
  return ret;
}

AM_RESULT AMDPTZ::get_current_buffer(AM_SOURCE_BUFFER_ID id,
                                     AMResolution &rsln)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_IAV_STATE iav_state;
    AMResolution vin_res;
    AMPlatformBufferFormat format;
    if (AM_RESULT_OK != (result = m_platform->iav_state_get(iav_state))) {
      ERROR("AMDPTZ: iav_state_get failed\n");
      break;
    }
    if (AM_IAV_STATE_PREVIEW != iav_state &&
        AM_IAV_STATE_ENCODING != iav_state) {
      ERROR("AMDPTZ: iav state must be preview or encode\n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    format.id = id;
    if ((result = m_platform->buffer_format_get(format)) != AM_RESULT_OK) {
      ERROR("Failed to get buffer%d format\n",id);
      break;
    }
    rsln = format.size;
    INFO("buffer[%d] resolution: %dx%d", id, rsln.width, rsln.height);
  } while (0);
  return result;
}

AM_RESULT AMDPTZ::get_input_buffer(AM_SOURCE_BUFFER_ID id,
                                   AMResolution &rsln)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_IAV_STATE iav_state;
    AMResolution vin_res;
    AMPlatformBufferFormat format;
    if (AM_RESULT_OK != (result = m_platform->iav_state_get(iav_state))) {
      ERROR("AMDPTZ: iav_state_get failed\n");
      break;
    }
    if (AM_IAV_STATE_PREVIEW != iav_state &&
        AM_IAV_STATE_ENCODING != iav_state) {
      ERROR("AMDPTZ: iav state must be preview or encode\n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if (id == AM_SOURCE_BUFFER_MAIN) {//main buffer's input is vin
      if ((result = m_vin->size_get(vin_res)) != AM_RESULT_OK) {
        ERROR("Failed to get vin size\n");
        break;
      }
      INFO("VIN resolution: %dx%d \n", vin_res.width, vin_res.height);
      rsln = vin_res;
    } else {//other buffer's input is main buffer
      format.id = AM_SOURCE_BUFFER_MAIN;
      if ((result = m_platform->buffer_format_get(format)) != AM_RESULT_OK) {
        ERROR("Failed to get bufferi%d format\n",format.id);
        break;
      }
      rsln = format.size;
    }
    INFO("buffer[%d] input resolution: %dx%d \n", id, rsln.width, rsln.height);
  } while (0);
  return result;
}

AM_RESULT AMDPTZ::get_plat_param(AM_SOURCE_BUFFER_ID id,
                                 AMPlatformDPTZParam &param)
{
  AMRect rect;
  AMResolution input, current;
  AM_RESULT result = AM_RESULT_OK;
  int offset_x, offset_y, delta_x, delta_y;
  float zoom_ratio_x, zoom_ratio_y, pan_ratio, tilt_ratio;
  float zoom_ratio_x_min, zoom_ratio_y_min;
  do {
    if (((int)id < (int)AM_SOURCE_BUFFER_MAIN)
        || ((int)id > (int)AM_SOURCE_BUFFER_EFM)) {
      ERROR("AMDPTZ: buffer id wrong\n");
      result = AM_RESULT_ERR_BUFFER_ID;
      break;
    }
    if ((result = m_platform->get_digital_zoom(id, rect)) != AM_RESULT_OK) {
      ERROR("Failed to get_digital_zoom!\n");
      break;
    }

    if ((result = get_input_buffer(id, input)) != AM_RESULT_OK) {
      ERROR("Failed to get_input_buffer!\n");
      break;
    }

    if ((result = get_current_buffer(id, current)) != AM_RESULT_OK) {
      ERROR("Failed to get_current_buffer!\n");
      break;
    }

    zoom_ratio_x_min = ((float)current.width)/input.width;
    zoom_ratio_y_min = ((float)current.height)/input.height;
    zoom_ratio_x = ((float)current.width)/rect.size.width;
    zoom_ratio_y = ((float)current.height)/rect.size.height;

    offset_x = FLOAT_TO_INT(input.width*0.5f - rect.size.width*0.5f);
    offset_y = FLOAT_TO_INT(input.height*0.5f - rect.size.height*0.5f);
    delta_x = rect.offset.x - offset_x;
    delta_y = rect.offset.y - offset_y;

    if (zoom_ratio_x > zoom_ratio_x_min) {
      pan_ratio = (float)delta_x/(input.width*0.5f - rect.size.width*0.5f);
    } else {
      pan_ratio = 0;
    }
    if (zoom_ratio_y > zoom_ratio_y_min) {
      tilt_ratio = (float)delta_y/(input.height*0.5f - rect.size.height*0.5f);
    } else {
      tilt_ratio = 0;
    }

    param.ratio.zoom_factor_x = zoom_ratio_x;
    param.ratio.zoom_factor_y = zoom_ratio_y;
    param.ratio.zoom_center_pos_x = pan_ratio;
    param.ratio.zoom_center_pos_y = tilt_ratio;
    param.window = rect;
  } while (0);
  return result;
}

AM_RESULT AMDPTZ::round_window(AM_SOURCE_BUFFER_ID id, AMRect &rect)
{
  AM_RESULT result = AM_RESULT_OK;
  AMResolution input, current;
  do {
    if ((result = get_input_buffer(id, input))!= AM_RESULT_OK) {
      ERROR("Failed to get_input_buffer!\n");
      break;
    }
    if ((result = get_current_buffer(id, current)) != AM_RESULT_OK) {
      ERROR("Failed to get_current_buffer!\n");
      break;
    }

    if (rect.offset.x < 0) {
      rect.offset.x = 0;
    }
    if (rect.offset.x > input.width) {
      rect.offset.x = input.width;
    }
    if (rect.offset.y < 0) {
      rect.offset.y = 0;
    }
    if (rect.offset.y > input.height) {
      rect.offset.y = input.height;
    }
    if (rect.size.width > input.width || rect.size.height > input.height) {
      ERROR("AMDPTZ: dptz window %dx%d is bigger than input %dx%d\n",
            rect.size.width, rect.size.height, input.width, input.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (((rect.size.width != 0) &&
        (float(current.width) / rect.size.width > 8.0)) ||
       ((rect.size.height != 0) &&
           (float(current.height) / rect.size.height > 8.0))) {
      ERROR("Source buffer%d upscaled from %dx%d to %dx%d, "
          "out of the max zoom in factor 8", id, rect.size.width,
          rect.size.height, current.width, current.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (rect.offset.x + rect.size.width > input.width) {
      rect.offset.x = input.width - rect.size.width;
    }
    if (rect.offset.y + rect.size.height > input.height) {
      rect.offset.y = input.height - rect.size.height;
    }

    rect.offset.x = ROUND_DOWN(rect.offset.x, 4);
    rect.offset.y = ROUND_DOWN(rect.offset.y, 4);
    rect.size.width = ROUND_UP(rect.size.width, 4);
    rect.size.height = ROUND_UP(rect.size.height, 4);
  } while (0);

  return result;
}

AM_RESULT AMDPTZ::ratio_to_window(AM_SOURCE_BUFFER_ID id,
                                  AMPlatformDPTZRatio &ratio, AMRect &wnd)
{
  int cwnd_x, cwnd_y, offset_x, offset_y, delta_x, delta_y;
  AMResolution input, current, crop;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = get_input_buffer(id, input)) != AM_RESULT_OK) {
      ERROR("Failed to get_input_buffer!\n");
      break;
    }
    if ((result = get_current_buffer(id, current)) != AM_RESULT_OK) {
      ERROR("Failed to get_current_buffer!\n");
      break;
    }
    //width and height must be round UP to multiple of 4
    crop.width = ROUND_UP(FLOAT_TO_INT(current.width/ratio.zoom_factor_x), 4);
    crop.height = ROUND_UP(FLOAT_TO_INT(current.height/ratio.zoom_factor_y), 4);

    //prefer to FOV resolution instead of invalid setting
    if (crop.width > input.width) {
      WARN("AMDPTZ: crop width %d is bigger than input width %d\n",
            crop.width, input.width);
      crop.width = input.width;
    }

    if (crop.height > input.height) {
      WARN("AMDPTZ: crop height %d is bigger than input height %d\n",
            crop.height, input.height);
      crop.height = input.height;
    }

    // convert float zoom center into actual zoom_center,
    // this could be half pixel
    // when pos_x = -1 means left most, when pos_x = 1 means right most
    // when pos_y = -1 means top most, when  pos_y = 1 means bottom most
    cwnd_x = FLOAT_TO_INT(input.width*0.5f - crop.width*0.5f);
    cwnd_y = FLOAT_TO_INT(input.height*0.5f - crop.height*0.5f);
    delta_x = FLOAT_TO_INT((input.width*0.5f-crop.width*0.5f)
                           *ratio.zoom_center_pos_x);
    delta_y = FLOAT_TO_INT((input.height*0.5f-crop.height*0.5f)
                           *ratio.zoom_center_pos_y);

    offset_x = cwnd_x + delta_x;
    offset_y = cwnd_y + delta_y;

    //data range clamp
    if (offset_x < 0)
      offset_x = 0;
    if (offset_x > input.width)
      offset_x = input.width;
    if (offset_y < 0)
      offset_y = 0;
    if (offset_y > input.height)
      offset_y = input.height;

    //round down to even number
    offset_x = ROUND_DOWN(offset_x, 4);
    offset_y = ROUND_DOWN(offset_y, 4);

    wnd.offset.x = offset_x;
    wnd.offset.y = offset_y;
    wnd.size.width = crop.width;
    wnd.size.height = crop.height;
  } while (0);
  return result;
}

AM_RESULT AMDPTZ::apply(AM_SOURCE_BUFFER_ID id, AMRect &window)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformBufferFormatMap buf_setup;
    if ((result = m_platform->buffer_setup_get(buf_setup)) != AM_RESULT_OK) {
      ERROR("Failed to get buffer%d format\n",id);
      break;
    }
    AMPlatformBufferFormatMap::iterator iter = buf_setup.find(id);
    if (iter == buf_setup.end()) {
      ERROR("wrong buffer id%d\n",id);
      result = AM_RESULT_ERR_BUFFER_ID;
      break;
    }

    if (iter->second.type == AM_SOURCE_BUFFER_TYPE_OFF) {
      result = AM_RESULT_ERR_MODULE_STATE;
      ERROR("source buffer type is \"off\"");
      break;
    }
    DEBUG("AMDPTZ: input window x=%d, y=%d, w=%d, h=%d\n",
         window.offset.x, window.offset.y,
         window.size.width, window.size.height);

    if ((result = m_platform->set_digital_zoom(id, window)) != AM_RESULT_OK) {
      ERROR("Failed to set_digital_zoom\n");
      break;
    }
  } while (0);

  return result;
}
