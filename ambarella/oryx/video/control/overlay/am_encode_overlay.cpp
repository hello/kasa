/*******************************************************************************
 * am_encode_overlay.cpp
 *
 * History:
 *   Oct 20, 2015 - [Huaiqing Wang] created file
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

#include "am_event.h"
#include "am_thread.h"
#include "am_platform_if.h"

#include "am_encode_config.h"
#include "am_encode_stream.h"
#include "am_encode_overlay.h"
#include "am_encode_overlay_area.h"

#include "text_insert.h"

typedef map<AM_STREAM_ID, AMOverlayInsert> AMOverlayInsertMap;

#define  AUTO_LOCK_OVERLAY(mtx)  std::lock_guard<std::recursive_mutex> lck(mtx)
std::recursive_mutex AMEncodeOverlay::m_mtx;

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  return AMEncodeOverlay::create((AMEncodeStream*)data);
}
#ifdef __cplusplus
}
#endif

AMEncodeOverlay* AMEncodeOverlay::create(AMEncodeStream *stream)
{
  AMEncodeOverlay *result = new AMEncodeOverlay();
  if (result && (AM_RESULT_OK != result->init(stream))) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMEncodeOverlay::destroy()
{
  stop();
  if (m_mmap_flag) {
    m_platform->unmap_overlay();
  }
  delete this;
}

bool AMEncodeOverlay::start(AM_STREAM_ID id)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  return (AM_RESULT_OK == setup(id));
}

bool AMEncodeOverlay::stop(AM_STREAM_ID id)
{
  return (AM_RESULT_OK == delete_overlay(id));
}

std::string& AMEncodeOverlay::name()
{
  return m_name;
}

void* AMEncodeOverlay::get_interface()
{
  return ((AMIEncodeOverlay*)this);
}

int32_t AMEncodeOverlay::init_area(AM_STREAM_ID stream_id,
                                   AMOverlayAreaAttr &attr)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  int32_t area_id = -1;
  do {
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    AMStreamStateMap::const_iterator itr_s = stream_state.find(stream_id);
    if ((itr_s == stream_state.end()) ||
        ((itr_s->second != AM_STREAM_STATE_ENCODING) &&
            (itr_s->second != AM_STREAM_STATE_STARTING))) {
      WARN("Stream%d is not enable!",stream_id);
      break;
    }

    //find out a available area(not created) for the specify stream
    if ((area_id = get_available_area_id(stream_id)) < 0) {
      break;
    }

    int32_t overlay_obj_max = m_max_stream_num * m_max_area_num;
    int32_t data_offset_start = OVERLAY_CLUT_SIZE * overlay_obj_max;

    AMOverlayAreaInsert area_insert = {0};
    area_insert.enable = attr.enable;
    area_insert.area_size_max = (m_mmap_mem.length - data_offset_start)
        / overlay_obj_max;
    area_insert.clut_addr_offset = m_stream_buf_start[stream_id].first
        + area_id * OVERLAY_CLUT_SIZE;
    area_insert.data_addr_offset = m_stream_buf_start[stream_id].second
        + area_id * area_insert.area_size_max;
    area_insert.clut_addr = (uint8_t *) ((uint32_t) m_mmap_mem.addr
        + area_insert.clut_addr_offset);
    area_insert.data_addr = (uint8_t *) ((uint32_t) m_mmap_mem.addr
        + area_insert.data_addr_offset);

    memset(area_insert.clut_addr, 0, OVERLAY_CLUT_SIZE);
    INFO("clut offset = %d, data offset = %d\n",
         area_insert.clut_addr_offset,
         area_insert.data_addr_offset);

    if (check_area_param(stream_id, attr, area_insert) != AM_RESULT_OK) {
      break;
    }

    AMOverlayArea *overlay_area = new AMOverlayArea();
    if (!overlay_area
        || (overlay_area->init(attr, area_insert) != AM_RESULT_OK)) {
      ERROR("stream%d create area%d overlay failed", stream_id, area_id);
      delete overlay_area;
      area_id = -1;
      break;
    }
    m_area[stream_id][area_id] = overlay_area;

    if (render_area(stream_id, area_id, attr.enable) != AM_RESULT_OK) {
      delete overlay_area;
      m_area[stream_id][area_id] = nullptr;
      area_id = -1;
      break;
    }
  } while (0);

  return area_id;
}

int32_t AMEncodeOverlay::add_data_to_area(AM_STREAM_ID stream_id,
                                          int32_t area_id,
                                          AMOverlayAreaData &data)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  int32_t index = -1;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      WARN("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    if ((index = it->second[area_id]->add_data(data)) < 0) {
      break;
    }

    if (data.type == AM_OVERLAY_DATA_TYPE_ANIMATION
        || data.type == AM_OVERLAY_DATA_TYPE_TIME) {
      m_update_area[stream_id].push_back(std::make_pair(area_id, index));
      if (!m_update_run.load()) {
        m_update_run = true;
        m_event->signal();
      }
    }
  } while (0);

  return index;
}

AM_RESULT AMEncodeOverlay::destroy_overlay(AM_STREAM_ID id)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  return delete_overlay(id);
}

AM_RESULT AMEncodeOverlay::update_area_data(AM_STREAM_ID stream_id,
                                            int32_t area_id,
                                            int32_t index,
                                            AMOverlayAreaData &data)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      result = AM_RESULT_ERR_MODULE_STATE;
      INFO("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    if ((result = it->second[area_id]->update_data(index, &data))
        != AM_RESULT_OK) {
      ERROR("update stream:%d area:%d index:%d data failed",
            stream_id, area_id, index);
      break;
    }

    if ((result = render_area(stream_id, area_id,
                              it->second[area_id]->get_state()))
        != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::delete_area_data(AM_STREAM_ID stream_id,
                                            int32_t area_id,
                                            int32_t index)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      result = AM_RESULT_ERR_MODULE_STATE;
      INFO("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    AMOverlayDataVecMap::iterator itr = m_update_area.find(stream_id);
    if (itr != m_update_area.end()) {
      vector<pair<int32_t, int32_t>>::iterator it_b = itr->second.begin();
      for (; it_b != itr->second.end(); ++ it_b) {
        if (*it_b == std::make_pair(area_id, index)) {
          itr->second.erase(it_b);
          break;
        }
      }
      if (itr->second.empty()) {
        m_update_area.erase(itr);
      }
      if (m_update_area.empty()) {
        m_update_run = false;
      }
    }

    if ((result = it->second[area_id]->delete_data(index)) != AM_RESULT_OK) {
      break;
    }

    if ((result = render_area(stream_id, area_id,
                              it->second[area_id]->get_state()))
        != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::change_state(AM_STREAM_ID stream_id,
                                        int32_t area_id,
                                        AM_OVERLAY_STATE state)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;

  switch(state) {
    case AM_OVERLAY_ENABLE:
      result = render_area(stream_id, area_id, 1);
      break;
    case AM_OVERLAY_DISABLE:
      result = render_area(stream_id, area_id, 0);
      break;
    case AM_OVERLAY_DELETE:
      result = delete_area(stream_id, area_id);
      break;
    default:
      result = AM_RESULT_ERR_INVALID;
      break;
  }

  return result;
}

AM_RESULT AMEncodeOverlay::get_area_param(AM_STREAM_ID stream_id,
                                          int32_t area_id,
                                          AMOverlayAreaParam &param)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      param.attr.enable = -1;
      result = AM_RESULT_ERR_MODULE_STATE;
      WARN("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    param = it->second[area_id]->get_param();
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::get_area_data_param(AM_STREAM_ID stream_id,
                                               int32_t area_id,
                                               int32_t index,
                                               AMOverlayAreaData &param)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      result = AM_RESULT_ERR_MODULE_STATE;
      WARN("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    it->second[area_id]->get_data_param(index, param);
  } while (0);

  return result;
}

void AMEncodeOverlay::get_user_defined_limit_value(AMOverlayUserDefLimitVal
                                                   &val)
{
  val.s_num_max.second = m_max_stream_num;
  val.a_num_max.second = m_max_area_num;
}

AM_RESULT AMEncodeOverlay::save_param_to_config()
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config) {
      result = AM_RESULT_ERR_PERM;
      ERROR("AMOverlayConfigPtr is null!");
      break;
    }
    m_config_param.clear();
    for (auto &m : m_area) {
      AM_STREAM_ID stream_id = m.first;
      m_config_param[stream_id].limit.s_num_max.second = m_max_stream_num;
      m_config_param[stream_id].limit.s_num_max.first = true;
      m_config_param[stream_id].limit.a_num_max.second = m_max_area_num;
      m_config_param[stream_id].limit.a_num_max.first = true;
      int32_t num = 0;
      for (uint32_t i = 0; i < m.second.size(); ++ i) {
        if (m.second[i] != nullptr) {
          AMOverlayConfigAreaParam config;
          AMOverlayAreaParam param = m.second[i]->get_param();
          param.data.clear();
          int32_t n = param.num;
          for (int32_t j = 0; j < n; ++ j) {
            AMOverlayAreaData data;
            m.second[i]->get_data_param(j, data);
            if (AM_OVERLAY_DATA_TYPE_NONE == data.type) {
              ++ n;
              continue;
            }
            param.data.push_back(data);
          }
          area_param_to_config(param, config);
          ++ num;
          m_config_param[stream_id].area.push_back(config);
          m_config_param[stream_id].num.first = true;
        }
      }
      m_config_param[stream_id].num.second = num;
    }

    if ((result = m_config->overwrite_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to set overlay config!");
      break;
    }
  } while (0);

  return result;
}

uint32_t AMEncodeOverlay::get_area_max_num()
{
  return m_platform->overlay_max_area_num();
}

AM_RESULT AMEncodeOverlay::load_config()
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMOverlayConfigPtr is null!");
      break;
    }
    if (!m_config_param.empty()) {
      m_config_param.clear();
    }
    if ((result = m_config->get_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get overlay config!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::setup(AM_STREAM_ID id)
{
  AUTO_LOCK_OVERLAY(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_config_param.empty()) {
      WARN("Overlay config parameters is null, no overlay to insert");
      break;
    }

    for (auto &m : m_config_param) {
      if ((id != AM_STREAM_ID_MAX) && (m.first != id)) {
        continue;
      }
      const AMStreamStateMap &stream_state = m_stream->stream_state();
      AMStreamStateMap::const_iterator itr_s = stream_state.find(m.first);
      AMOverlayAreaVecMap::iterator itr_o = m_area.find(m.first);
      if ((itr_s == stream_state.end()) ||
          ((itr_s->second != AM_STREAM_STATE_ENCODING) &&
              (itr_s->second != AM_STREAM_STATE_STARTING)) ||
          (itr_o != m_area.end())) {
        continue;
      }

      int32_t overlay_obj_max = m_max_stream_num * m_max_area_num;
      int32_t area_offset_start = OVERLAY_CLUT_SIZE * overlay_obj_max;
      int32_t area_buf_size = (m_mmap_mem.length - area_offset_start)
            / overlay_obj_max;

      int32_t size = m_stream_buf_start.size();
      m_stream_buf_start[m.first] =
          std::make_pair<int32_t, int32_t>(size * m_max_area_num
                                           * OVERLAY_CLUT_SIZE,
                                           area_offset_start
                                           + size * m_max_area_num
                                           * area_buf_size);
      for (int32_t j = 0; j < m_max_area_num; ++ j) {
        m_area[m.first].push_back(nullptr);
      }

      AMOverlayParams param;
      param.stream_id = m.first;
      param.num =
          (m.second.num.second < int32_t(m.second.area.size())) ?
              m.second.num.second : m.second.area.size();

      for (int32_t i = 0; i < param.num; ++ i) {
        AMOverlayAreaParam area;
        const AMOverlayConfigAreaParam &config = m.second.area[i];
        INFO("config stream%d area%d w = %d h = %d\n",
             m.first,
             i,
             config.width.second,
             config.height.second);
        area_config_to_param(config, area);

        param.area.push_back(area);
      }

      if ((result = add(param)) != AM_RESULT_OK) {
        ERROR("Add overlay failed!");
        m_area.erase(m.first);
        m_stream_buf_start.erase(m.first);
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::add(AMOverlayParams &params)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_STREAM_ID stream_id = params.stream_id;
    INFO("stream%d", stream_id);
    for (auto &m : params.area) {
      if ((result = add_area(stream_id, m)) != AM_RESULT_OK) {
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::add_area(AM_STREAM_ID stream_id,
                                    AMOverlayAreaParam &param)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    int32_t area_id = -1;
    if ((area_id = init_area(stream_id, param.attr)) < 0) {
      ERROR("init area failed!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    INFO("init stream%d area%d success!", stream_id, area_id);

    for (auto &m : param.data) {
      int32_t index = -1;
      if ((index = add_data_to_area(stream_id, area_id, m)) < 0) {
        ERROR("add data to stream%d area%d failed!", stream_id, area_id);
        result = AM_RESULT_ERR_INVALID;
        break;
      }
      INFO("add data block%d to stream%d area%d success!",
           index,
           stream_id,
           area_id);
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::delete_overlay(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    for (auto &m : m_area) {
      if ((id != AM_STREAM_ID_MAX) && (m.first != id)) {
        continue;
      }
      if ((result = render_stream(m.first, 0)) != AM_RESULT_OK) {
        break;
      }

      AMOverlayDataVecMap::iterator itr = m_update_area.find(m.first);
      if (itr != m_update_area.end()) {
        vector<pair<int32_t, int32_t>>::iterator it_b = itr->second.begin();
        while(it_b != itr->second.end()) {
          it_b = itr->second.erase(it_b);
        }
        m_update_area.erase(itr);
        if (m_update_area.empty()) {
          m_update_run = false;
        }
      }

      for (auto &n : m.second) {
        delete n;
        n = nullptr;
      }
      m.second.clear();

      m_area.erase(m.first);
      m_stream_buf_start.erase(m.first);
    }

  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::render_area(AM_STREAM_ID stream_id,
                                       int32_t area_id,
                                       int32_t state)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      result = AM_RESULT_ERR_MODULE_STATE;
      INFO("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    AMOverlayInsert insert;
    insert.stream_id = stream_id;
    insert.enable = 1;
    bool  set_flag = false;
    for (auto &m : it->second) {
      if (m != nullptr) {
        const AMOverlayAreaInsert &area_insert = m->get_drv_param();
        insert.area.push_back(area_insert);
        set_flag = true;
      } else {
        AMOverlayAreaInsert area_insert = { 0 };
        insert.area.push_back(area_insert);
      }
    }
    insert.area[area_id].enable = state;

    if (!set_flag ||
        ((result = m_platform->overlay_set(insert)) != AM_RESULT_OK)) {
      break;
    }
    it->second[area_id]->set_state(state);
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::delete_area(AM_STREAM_ID stream_id, int32_t area_id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if ((it == m_area.end()) || (area_id >= m_max_area_num)
        || (it->second[area_id] == nullptr)) {
      result = AM_RESULT_ERR_MODULE_STATE;
      INFO("stream%d area%d is not created", stream_id, area_id);
      break;
    }

    AMOverlayDataVecMap::iterator itr = m_update_area.find(stream_id);
    if (itr != m_update_area.end()) {
      int32_t data_num = it->second[area_id]->get_param().num;

      vector<pair<int32_t, int32_t>>::iterator it_b = itr->second.begin();
      for (; it_b != itr->second.end();) {
        int32_t i = 0;
        for (; i < data_num; ++ i) {
          if (*it_b == std::make_pair(area_id, i)) {
            it_b == itr->second.erase(it_b);
            break;
          }
        }
        if (i == data_num) {
          ++ it_b;
        }
      }
      if (itr->second.empty()) {
        m_update_area.erase(itr);
      }
      if (m_update_area.empty()) {
        m_update_run = false;
      }
    }

    if ((result = render_area(stream_id, area_id, 0)) != AM_RESULT_OK) {
      break;
    }

    delete it->second[area_id];
    it->second[area_id] = nullptr;
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::render_stream(AM_STREAM_ID stream_id, int32_t state)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if (it == m_area.end()) {
      result = AM_RESULT_ERR_MODULE_STATE;
      INFO("stream%d have not create any overlay area", stream_id);
      break;
    }

    AMOverlayInsert insert;
    insert.stream_id = stream_id;
    insert.enable = state;
    bool  set_flag = false;
    for (auto &m : it->second) {
      if (m != nullptr) {
        const AMOverlayAreaInsert &area_insert = m->get_drv_param();
        insert.area.push_back(area_insert);
        set_flag = true;
      } else {
        AMOverlayAreaInsert area_insert = { 0 };
        insert.area.push_back(area_insert);
      }
    }
    if (set_flag &&
        ((result = m_platform->overlay_set(insert)) != AM_RESULT_OK)) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::check_area_param(AM_STREAM_ID stream_id,
                                            AMOverlayAreaAttr &attr,
                                            AMOverlayAreaInsert &overlay_area)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformStreamFormat stream_format;
    stream_format.id = stream_id;
    if ((result = m_platform->stream_format_get(stream_format))
        != AM_RESULT_OK) {
      ERROR("Overlay get stream format failed!");
      break;
    }

    if (attr.rotate) {
      overlay_area.rotate_mode = (stream_format.rotate ?
          AM_ROTATE_90 : AM_NO_ROTATE_FLIP);
      overlay_area.rotate_mode |= (
          (stream_format.flip & 0x02) ? AM_HORIZONTAL_FLIP : AM_NO_ROTATE_FLIP);
      overlay_area.rotate_mode |= (
          (stream_format.flip & 0x01) ? AM_VERTICAL_FLIP : AM_NO_ROTATE_FLIP);
    }

    uint32_t win_width = stream_format.enc_win.size.width;
    uint32_t win_height = stream_format.enc_win.size.height;

    INFO("Encode stream%d width = %d, height = %d, flip_rotate = %d.\n",
         stream_id, win_width, win_height, overlay_area.rotate_mode);

    int32_t source_buf_w, source_buf_h, tmp;
    int32_t &tmp_startx = attr.rect.offset.x;
    int32_t &tmp_starty = attr.rect.offset.y;
    int32_t &area_w = attr.rect.size.width;
    int32_t &area_h = attr.rect.size.height;

    if (overlay_area.rotate_mode & AM_ROTATE_90) {
      source_buf_w = win_height;
      source_buf_h = win_width;
    } else {
      source_buf_w = win_width;
      source_buf_h = win_height;
    }

    if (area_w > source_buf_w) {
      WARN("Overlay area width[%u] > source buffer width[%u]. "
           "Reset area width to[%u], offset x to 0.",
           area_w, source_buf_w, source_buf_w);
      area_w = source_buf_w;
      tmp_startx = 0;
    }

    if (area_h > source_buf_h) {
      WARN("Overlay area height[%u] > source buffer height[%u]. "
           "Reset area height to[%u], offset y to 0.",
           area_h, source_buf_h, source_buf_h);
      area_h = source_buf_h;
      tmp_starty = 0;
    }

    if (tmp_startx + area_w > source_buf_w) {
      tmp = source_buf_w - area_w;
      INFO("Overlay area width[%u] + offset x[%u] > source buffer width[%u]. "
           "Reset area offset x to[%u]",
           area_w, tmp_startx, source_buf_w, tmp);
      tmp_startx = (uint16_t) tmp;
    }

    if (tmp_starty + area_h > source_buf_h) {
      tmp = source_buf_h - area_h;
      INFO("Overlay area height[%u] + offset y[%u] > source buffer height[%u]. "
           "Reset area offset y to[%u]",
           area_h, tmp_starty, source_buf_h, tmp);
      tmp_starty = (uint16_t) tmp;
    }

    //overlay_area's start_x,start_y,width and height are
    //relative to mmap buffer, not encode stream that user set.
    //so we must transform it here
    if (overlay_area.rotate_mode & AM_ROTATE_90) {
      overlay_area.start_x = tmp_starty;
      overlay_area.start_y = (uint16_t) (win_height - area_w - tmp_startx);
      overlay_area.width = area_h;
      overlay_area.height = area_w;
    } else {
      overlay_area.start_x = tmp_startx;
      overlay_area.start_y = tmp_starty;
      overlay_area.width = area_w;
      overlay_area.height = area_h;
    }

    if (overlay_area.rotate_mode & AM_HORIZONTAL_FLIP) {
      overlay_area.start_x = win_width - overlay_area.width
          - overlay_area.start_x;
    }

    if (overlay_area.rotate_mode & AM_VERTICAL_FLIP) {
      overlay_area.start_y = win_height - overlay_area.height
          - overlay_area.start_y;
    }

    DEBUG("Area state in local memory:[%ux%u]", area_w, area_h);
    DEBUG("Area state in mmap buffer: [%ux%u] + offset [%ux%u].",
          overlay_area.width, overlay_area.height,
          overlay_area.start_x, overlay_area.start_y);
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::check_stream_id(AM_STREAM_ID stream_id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if (it != m_area.end()) {
      break;
    }

    if ((int32_t) m_stream_buf_start.size() >= m_max_stream_num) {
      ERROR("It is already insert %d stream overlay, "
            "can't add any more stream",
            m_area.size());
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (stream_id < 0) {
      result = AM_RESULT_ERR_STREAM_ID;
      ERROR("invalid overlay stream id: %d", stream_id);
    }

    INFO("Insert a new stream(%d) overlay", stream_id);
    if (!m_mmap_flag) {
      if ((result=m_platform->map_overlay(m_mmap_mem)) != AM_RESULT_OK) {
        ERROR("overlay call map_overlay failed");
        break;
      }
      m_mmap_flag = true;
    }

    int32_t overlay_obj_max = m_max_stream_num * m_max_area_num;
    int32_t data_offset_start = OVERLAY_CLUT_SIZE * overlay_obj_max;
    int32_t data_buf_size = (m_mmap_mem.length - data_offset_start)
        / overlay_obj_max;
    int32_t size = m_stream_buf_start.size();
    m_stream_buf_start[stream_id] =
        std::make_pair<int32_t, int32_t>(size * m_max_area_num *
                                         OVERLAY_CLUT_SIZE,
                                         data_offset_start +
                                         size * m_max_area_num * data_buf_size);

    for (int32_t j = 0; j < m_max_area_num; ++ j) {
      m_area[stream_id].push_back(nullptr);
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::check_area_id(int32_t area_id)
{
  AM_RESULT result = AM_RESULT_OK;
  if (area_id < 0 || area_id >= m_max_area_num) {
    INFO("Invalid area id: %d!!! \n", area_id);
    result = AM_RESULT_ERR_INVALID;
  }

  return result;
}

int32_t AMEncodeOverlay::get_available_area_id(AM_STREAM_ID stream_id)
{
  int32_t area_id = -1;
  do {
    if (check_stream_id(stream_id) != AM_RESULT_OK) {
      break;
    }
    AMOverlayAreaVecMap::iterator it = m_area.find(stream_id);
    if (it == m_area.end()) {
      area_id = 0;
      break;
    } else {
      int32_t i;
      for (i = 0; i < m_max_area_num; ++ i) {
        if (nullptr == it->second[i]) {
          area_id = i;
          break;
        }
      }
      if (i == m_max_area_num) {
        WARN("stream%d has no area to add overlay!", stream_id);
        break;
      }
    }
  } while (0);

  if (area_id != -1) {
    INFO("stream%d area[%d] is available", stream_id, area_id);
  }

  return area_id;
}

void AMEncodeOverlay::area_config_to_param(const AMOverlayConfigAreaParam &src,
                                           AMOverlayAreaParam &dst)
{
  AMOverlayAreaAttr &attr = dst.attr;
  attr.enable = src.state.second;
  attr.rotate = src.rotate.second;
  attr.rect.size.width = src.width.second;
  attr.rect.size.height = src.height.second;
  attr.rect.offset.x = src.offset_x.second;
  attr.rect.offset.y = src.offset_y.second;
  attr.bg_color.v = uint8_t((src.bg_color.second >> 24) & 0xff);
  attr.bg_color.u = uint8_t((src.bg_color.second >> 16) & 0xff);
  attr.bg_color.y = uint8_t((src.bg_color.second >> 8) & 0xff);
  attr.bg_color.a = uint8_t(src.bg_color.second & 0xff);
  attr.buf_num = src.buf_num.second;

  dst.num =
      (src.num.second < int32_t(src.data.size())) ? src.num.second :
                                                    src.data.size();
  for (int32_t i = 0; i < dst.num; ++ i) {
    AMOverlayAreaData data;
    const AMOverlayConfigDataParam &m = src.data[i];

    data.rect.size.width = m.width.second;
    data.rect.size.height = m.height.second;
    data.rect.offset.x = m.offset_x.second;
    data.rect.offset.y = m.offset_y.second;
    data.type = m.type.second;

    if ((AM_OVERLAY_DATA_TYPE_PICTURE == data.type)
        || (AM_OVERLAY_DATA_TYPE_ANIMATION == data.type)) {
      AMOverlayPicture pic;
      pic.filename = m.bmp.second;
      pic.colorkey.color.v = uint8_t((m.color_key.second >> 24) & 0xff);
      pic.colorkey.color.u = uint8_t((m.color_key.second >> 16) & 0xff);
      pic.colorkey.color.y = uint8_t((m.color_key.second >> 8) & 0xff);
      pic.colorkey.color.a = uint8_t(m.color_key.second & 0xff);
      pic.colorkey.range = m.color_range.second;
      if (AM_OVERLAY_DATA_TYPE_ANIMATION == data.type) {
        AMOverlayAnimation &anim = data.anim;
        anim.num = m.bmp_num.second;
        anim.interval = m.interval.second;
        anim.pic = pic;
      } else {
        data.pic = pic;
      }
    } else if ((AM_OVERLAY_DATA_TYPE_STRING == data.type)
        || (AM_OVERLAY_DATA_TYPE_TIME == data.type)) {
      AMOverlayTextBox text;
      text.str = m.str.second;
      text.spacing = m.spacing.second;
      text.font.ttf_name = m.font_name.second;
      text.font.width = m.font_size.second;
      text.font.outline_width = m.font_outwidth.second;
      text.font.hor_bold = m.font_horbold.second;
      text.font.ver_bold = m.font_verbold.second;
      text.font.italic = m.font_italic.second;

      uint32_t tmp = m.font_color.second;
      if (tmp < AM_OVERLAY_COLOR_NUM) {
        text.font_color.id = tmp;
      } else {
        text.font_color.id = AM_OVERLAY_COLOR_CUSTOM;
        text.font_color.color.v = (uint8_t) ((tmp >> 24) & 0xff);
        text.font_color.color.u = (uint8_t) ((tmp >> 16) & 0xff);
        text.font_color.color.y = (uint8_t) ((tmp >> 8) & 0xff);
        text.font_color.color.a = (uint8_t) (tmp & 0xff);
      }

      tmp = m.bg_color.second;
      text.background_color.v = (uint8_t) ((tmp >> 24) & 0xff);
      text.background_color.u = (uint8_t) ((tmp >> 16) & 0xff);
      text.background_color.y = (uint8_t) ((tmp >> 8) & 0xff);
      text.background_color.a = (uint8_t) (tmp & 0xff);

      tmp = m.ol_color.second;
      text.outline_color.v = (uint8_t) ((tmp >> 24) & 0xff);
      text.outline_color.u = (uint8_t) ((tmp >> 16) & 0xff);
      text.outline_color.y = (uint8_t) ((tmp >> 8) & 0xff);
      text.outline_color.a = (uint8_t) (tmp & 0xff);
      if (AM_OVERLAY_DATA_TYPE_TIME == data.type) {
        AMOverlayTime &otime = data.time;
        otime.pre_str = m.pre_str.second;
        otime.suf_str = m.suf_str.second;
        otime.en_msec = m.en_msec.second;
        otime.format = m.format.second;
        otime.is_12h = m.is_12h.second;
        otime.text = text;
      } else {
        data.text = text;
      }
    } else if (AM_OVERLAY_DATA_TYPE_LINE == data.type) {
      AMOverlayLine &line = data.line;
      uint32_t tmp = m.line_color.second;
      if (tmp < AM_OVERLAY_COLOR_NUM) {
        line.color.id = tmp;
      } else {
        line.color.id = AM_OVERLAY_COLOR_CUSTOM;
        line.color.color.v = (uint8_t) ((tmp >> 24) & 0xff);
        line.color.color.u = (uint8_t) ((tmp >> 16) & 0xff);
        line.color.color.y = (uint8_t) ((tmp >> 8) & 0xff);
        line.color.color.a = (uint8_t) (tmp & 0xff);
      }

      line.thickness = m.line_tn.second;
      line.point = m.line_p.second;
    }

    dst.data.push_back(data);
  }
}

void AMEncodeOverlay::area_param_to_config(const AMOverlayAreaParam &src,
                                           AMOverlayConfigAreaParam &dst)
{
  const AMOverlayAreaAttr &attr = src.attr;
  dst.state.second = attr.enable;
  dst.state.first = true;

  dst.rotate.second = attr.rotate;
  dst.rotate.first = true;

  dst.width.second = attr.rect.size.width;
  dst.width.first = true;

  dst.height.second = attr.rect.size.height;
  dst.height.first = true;

  dst.offset_x.second = attr.rect.offset.x;
  dst.offset_x.first = true;

  dst.offset_y.second = attr.rect.offset.y;
  dst.offset_y.first = true;

  dst.bg_color.second = (uint32_t) ((attr.bg_color.v << 24)
      | (attr.bg_color.u << 16) | (attr.bg_color.y << 8) | attr.bg_color.a);
  dst.bg_color.first = true;

  dst.buf_num.second = attr.buf_num;
  dst.buf_num.first = true;

  dst.num.second = src.num;
  dst.num.first = true;
  for (uint32_t i = 0; i < src.data.size(); ++ i) {
    const AMOverlayAreaData &data = src.data[i];
    AMOverlayConfigDataParam config;

    config.width.second = data.rect.size.width;
    config.width.first = true;

    config.height.second = data.rect.size.height;
    config.height.first = true;

    config.offset_x.second = data.rect.offset.x;
    config.offset_x.first = true;

    config.offset_y.second = data.rect.offset.y;
    config.offset_y.first = true;

    config.type.second = data.type;
    config.type.first = true;

    if ((AM_OVERLAY_DATA_TYPE_PICTURE == data.type)
        || (AM_OVERLAY_DATA_TYPE_ANIMATION == data.type)) {
      AMOverlayPicture pic;
      if (AM_OVERLAY_DATA_TYPE_ANIMATION == data.type) {
        config.bmp_num.second = data.anim.num;
        config.bmp_num.first = true;

        config.interval.second = data.anim.interval;
        config.interval.first = true;

        pic = data.anim.pic;
      } else {
        pic = data.pic;
      }
      config.bmp.second = pic.filename;
      config.bmp.first = true;

      config.color_key.second = (uint32_t) ((pic.colorkey.color.v << 24)
          | (pic.colorkey.color.u << 16) | (pic.colorkey.color.y << 8)
          | pic.colorkey.color.a);
      config.color_key.first = true;

      config.color_range.second = pic.colorkey.range;
      config.color_range.first = true;
    } else if ((AM_OVERLAY_DATA_TYPE_STRING == data.type)
        || (AM_OVERLAY_DATA_TYPE_TIME == data.type)) {
      AMOverlayTextBox text;
      if (AM_OVERLAY_DATA_TYPE_TIME == data.type) {
        config.pre_str.second = data.time.pre_str;
        config.pre_str.first = true;

        config.suf_str.second = data.time.suf_str;
        config.suf_str.first = true;

        config.en_msec.second = data.time.en_msec;
        config.en_msec.first = true;

        config.format.second = data.time.en_msec;
        config.format.first = true;

        config.is_12h.second = data.time.is_12h;
        config.is_12h.first = true;

        text = data.time.text;
      } else {
        text = data.text;
      }
      config.str.second = text.str;
      config.str.first = true;

      config.spacing.second = text.spacing;
      config.spacing.first = true;

      config.font_name.second = text.font.ttf_name;
      config.font_name.first = true;

      config.font_size.second = text.font.width;
      config.font_size.first = true;

      config.font_outwidth.second = text.font.outline_width;
      config.font_outwidth.first = true;

      config.font_horbold.second = text.font.hor_bold;
      config.font_horbold.first = true;

      config.font_verbold.second = text.font.ver_bold;
      config.font_verbold.first = true;

      config.font_italic.second = text.font.italic;
      config.font_italic.first = true;

      if (text.font_color.id < AM_OVERLAY_COLOR_NUM) {
        config.font_color.second = text.font_color.id;
      } else {
        config.font_color.second = (uint32_t) ((text.font_color.color.v << 24)
            | (text.font_color.color.u << 16) | (text.font_color.color.y << 8)
            | text.font_color.color.a);
      }
      config.font_color.first = true;

      config.bg_color.second = (uint32_t) ((text.background_color.v << 24)
          | (text.background_color.u << 16) | (text.background_color.y << 8)
          | text.background_color.a);
      config.bg_color.first = true;

      config.ol_color.second = (uint32_t) ((text.outline_color.v << 24)
          | (text.outline_color.u << 16) | (text.outline_color.y << 8)
          | text.outline_color.a);
      config.ol_color.first = true;
    } else if (AM_OVERLAY_DATA_TYPE_LINE == data.type) {
      const AMOverlayLine &line = data.line;

      if (line.color.id < AM_OVERLAY_COLOR_NUM) {
        config.line_color.second = line.color.id;
      } else {
        config.line_color.second = (uint32_t) ((line.color.color.v << 24)
            | (line.color.color.u << 16) | (line.color.color.y << 8)
            | line.color.color.a);
      }
      config.line_color.first = true;

      config.line_tn.second = line.thickness;
      config.line_tn.first = true;

      config.line_p.second = line.point;
      config.line_p.first = true;
    }

    dst.data.push_back(config);
  }
}

void AMEncodeOverlay::auto_update_overlay_data()
{
  while(!m_exit.load()) {
    if (m_event) {
      m_event->wait();
    }

    while (m_update_run.load()) {
      if (m_platform->vin_wait_next_frame() != AM_RESULT_OK) {
        m_update_run = false;
        ERROR("Overlay failed to wait vin next frame!");
        break;
      }

      for (auto &s : m_update_area) {
        AMOverlayAreaVecMap::iterator it = m_area.find(s.first);
        if (it == m_area.end()) {
          continue;
        }

        for (auto &v : s.second) {
          if ((v.first >= m_max_area_num)
              || (it->second[v.first] == nullptr)) {
            continue;
          }

          if (it->second[v.first]->update_data(v.second, nullptr)
              != AM_RESULT_OK) {
            break;
          }
        }

        if (render_stream(s.first, 1) != AM_RESULT_OK) {
          break;
        }
      }
    }
  }
}

void AMEncodeOverlay::static_update_thread(void* arg)
{
  ((AMEncodeOverlay*)arg)->auto_update_overlay_data();
}

AMEncodeOverlay::AMEncodeOverlay() :
  m_event(nullptr),
  m_thread_hand(nullptr),
  m_stream(nullptr),
  m_max_stream_num(0),
  m_max_area_num(0),
  m_name("Overlay"),
  m_update_run(false),
  m_exit(false),
  m_mmap_flag(false),
  m_platform(nullptr),
  m_config(nullptr)
{
}

AMEncodeOverlay::~AMEncodeOverlay()
{
  m_exit = true;
  m_update_run = false;
  m_event->signal();
  AM_DESTROY(m_thread_hand);
  AM_DESTROY(m_event);
  m_platform = nullptr;
  m_config = nullptr;
  m_config_param.clear();
  DEBUG("AMEncodeOverlay is destroyed!");
}

AM_RESULT AMEncodeOverlay::init(AMEncodeStream *stream)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_stream = stream;
    if (!m_stream) {
      ERROR("Invalid Encode Stream!");
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (!(m_config = AMOverlayConfig::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMOverlayConfig!");
      break;
    }
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform");
      break;
    }
    if (!(m_event = AMEvent::create())) {
      ERROR("Failed to create event!");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    if (!m_thread_hand) {
      if ((m_thread_hand = AMThread::create("overlay_update_thread",
                                            static_update_thread,
                                            (void*)this)))
        DEBUG("Overlay update task created!!!");
      else {
        result = AM_RESULT_ERR_MEM;
        ERROR("Create overlay update thread failed!!!");
        break;
      }
    }
    if (AM_RESULT_OK != (result = init_platform_param())) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeOverlay::init_platform_param()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_config->get_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get overlay config!");
      break;
    }
    for (auto &m : m_config_param) {
      m_max_stream_num = m.second.limit.s_num_max.second;
      m_max_area_num = m.second.limit.a_num_max.second;
      break;
    }

    int32_t platform_area_max = m_platform->overlay_max_area_num();
    if (m_max_area_num > platform_area_max) {
      WARN("platform just support max %d areas, so change max area number from "
           "%d to %d\n",
           platform_area_max, m_max_area_num, platform_area_max);
      m_max_area_num = platform_area_max;
    }

    if (!m_mmap_flag) {
      if (AM_RESULT_OK != (result = m_platform->map_overlay(m_mmap_mem))) {
        ERROR("overlay call map_overlay failed");
        break;
      }
      m_mmap_flag = true;
    }
  } while (0);

  return result;
}
