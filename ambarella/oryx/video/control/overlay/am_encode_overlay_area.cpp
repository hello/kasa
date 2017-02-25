/*******************************************************************************
 * am_encode_overlay_area.cpp
 *
 * History:
 *   2016-3-28 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_encode_overlay_area.h"

#include "am_encode_overlay_data_time.h"
#include "am_encode_overlay_data_animate.h"
#include "am_encode_overlay_data_line.h"

static const char* overlay_type_to_str(AM_OVERLAY_DATA_TYPE type)
{
  const char *str = "Unknown";
  switch(type) {
    case AM_OVERLAY_DATA_TYPE_PICTURE:   str = "Picture";   break;
    case AM_OVERLAY_DATA_TYPE_STRING:    str = "String";    break;
    case AM_OVERLAY_DATA_TYPE_TIME:      str = "Time";      break;
    case AM_OVERLAY_DATA_TYPE_ANIMATION: str = "Animation"; break;
    case AM_OVERLAY_DATA_TYPE_LINE:      str = "Line";      break;
    default: break;
  }

  return str;
}

std::recursive_mutex AMOverlayArea::m_mtx;
AMOverlayArea::AMOverlayArea() :
    m_clut(nullptr),
    m_buffer(nullptr),
    m_color_num(0),
    m_cur_buf(0),
    m_flip_rotate(0)
{
  memset(&m_drv_param, 0, sizeof(m_drv_param));
  m_data.clear();
}

AMOverlayArea::~AMOverlayArea()
{
  delete[] m_buffer;
  m_buffer = nullptr;
  for (auto &m : m_data) {
    AM_DESTROY(m);
  }
  m_data.clear();
}

AM_RESULT AMOverlayArea::init(const AMOverlayAreaAttr &attr,
                              const AMOverlayAreaInsert &overlay_area)
{
  OL_AUTO_LOCK(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    m_param.attr = attr;
    if (m_param.attr.buf_num <= 0) {
      m_param.attr.buf_num = 1;
    }
    m_clut = (AMOverlayCLUT*) overlay_area.clut_addr;
    m_clut[CLUT_ENTRY_BACKGOURND] = attr.bg_color;
    m_color_num = 1;

    m_drv_param = overlay_area;
    if ((result = adjust_area_param()) != AM_RESULT_OK || (result =
        alloc_area_mem()) != AM_RESULT_OK) {
      break;
    }

    AMRect rect;
    rect.size = m_param.attr.rect.size;
    rect.offset = {0};
    if ((result = update_drv_data(m_buffer, m_param.attr.rect.size, rect,
                                  rect.offset, true)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

int32_t AMOverlayArea::add_data(AMOverlayAreaData &data)
{
  OL_AUTO_LOCK(m_mtx);
  int32_t index = -1;

  do {
    if (data.rect.offset.x >= m_param.attr.rect.size.width
        || data.rect.offset.y >= m_param.attr.rect.size.height) {
      WARN("offset(%d, %d) is exceed the area size(%d x %d), "
           "no need to do deep process",
           data.rect.offset.x, data.rect.offset.y,
           m_param.attr.rect.size.width, m_param.attr.rect.size.height);
      break;
    }

    AMOverlayData *instance = nullptr;
    switch(data.type) {
      case AM_OVERLAY_DATA_TYPE_PICTURE:
        instance = AMOverlayPicData::create(this, &data);
        break;
      case AM_OVERLAY_DATA_TYPE_STRING:
        instance = AMOverlayStrData::create(this, &data);
        break;
      case AM_OVERLAY_DATA_TYPE_TIME:
        instance = AMOverlayTimeData::create(this, &data);
        break;
      case AM_OVERLAY_DATA_TYPE_ANIMATION:
        instance = AMOverlayAnimData::create(this, &data);
        break;
      case AM_OVERLAY_DATA_TYPE_LINE:
        instance = AMOverlayLineData::create(this, &data);
        break;
      default:
        ERROR("Invalid data type: %d", data.type);
        break;
    }
    if (!instance) {
      ERROR("Failed to create Overlay data for %s!",
            overlay_type_to_str(data.type));
      break;
    }

    index = get_available_index();
    m_data[index] = instance;
    ++ m_param.num;
  } while (0);

  return index;
}

AM_RESULT AMOverlayArea::update_data(int32_t index, AMOverlayAreaData *data)
{
  OL_AUTO_LOCK(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (index >= int32_t(m_data.size()) || (nullptr == m_data[index])) {
      WARN("invalid data index: %d\n", index);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_data[index]->update(data);
  } while (0);

  return result;
}

AM_RESULT AMOverlayArea::delete_data(int32_t index)
{
  OL_AUTO_LOCK(m_mtx);
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (index >= int32_t(m_data.size()) || (nullptr == m_data[index])) {
      WARN("invalid data index: %d\n", index);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if ((result = m_data[index]->blank()) != AM_RESULT_OK) {
      break;
    }

    AM_DESTROY(m_data[index]);
    -- m_param.num;
  } while (0);

  return result;
}

const AMOverlayAreaParam& AMOverlayArea::get_param()
{
  OL_AUTO_LOCK(m_mtx);
  return m_param;
}

const AMOverlayAreaInsert& AMOverlayArea::get_drv_param()
{
  OL_AUTO_LOCK(m_mtx);
  return m_drv_param;
}

int32_t AMOverlayArea::get_state()
{
  OL_AUTO_LOCK(m_mtx);
  return (m_param.attr.enable & m_drv_param.enable);
}

void AMOverlayArea::set_state(uint16_t state)
{
  OL_AUTO_LOCK(m_mtx);
  m_param.attr.enable = state;
  m_drv_param.enable = state;
}

void AMOverlayArea::get_data_param(int32_t index, AMOverlayAreaData &param)
{
  OL_AUTO_LOCK(m_mtx);
  if ((index >= int32_t(m_data.size())) || m_data[index] == nullptr) {
    param.type = AM_OVERLAY_DATA_TYPE_NONE;
  } else {
    param = m_data[index]->get_param();
  }
}

AMOverlayCLUT* AMOverlayArea::get_clut()
{
  return m_clut;
}

int32_t& AMOverlayArea::get_clut_color_num()
{
  return m_color_num;
}

AM_RESULT AMOverlayArea::update_drv_data(uint8_t *buf,
                                         AMResolution buf_size,
                                         AMRect data,
                                         AMOffset dst_offset,
                                         bool all_flag)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!buf) {
      ERROR("invalid pointer!");
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }

    int32_t buffer_num = 1;
    if (all_flag) {
      buffer_num = m_param.attr.buf_num;
    } else {
      switch_buffer();
    }

    for (int32_t i = 0; i < buffer_num; ++ i) {
      uint8_t *drv_buf = nullptr;
      if (1 == buffer_num) {
        drv_buf = m_drv_param.data_addr;
      } else {
        drv_buf = m_drv_param.data_addr +
            (i - m_cur_buf) * m_drv_param.total_size;
      }
      result = rotate_fill(drv_buf,
                           m_drv_param.pitch,
                           m_drv_param.width,
                           m_drv_param.height,
                           dst_offset.x,
                           dst_offset.y,
                           buf,
                           buf_size.width,
                           buf_size.height,
                           data.offset.x,
                           data.offset.y,
                           data.size.width,
                           data.size.height);
    }
  } while(0);

  return result;
}

AM_RESULT AMOverlayArea::rotate_fill(uint8_t *dst,
                                     uint32_t dst_pitch,
                                     uint32_t dst_width,
                                     uint32_t dst_height,
                                     uint32_t dst_x,
                                     uint32_t dst_y,
                                     uint8_t *src,
                                     uint32_t src_pitch,
                                     uint32_t src_height,
                                     uint32_t src_x,
                                     uint32_t src_y,
                                     uint32_t data_width,
                                     uint32_t data_height)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!src || !dst) {
      ERROR("src or dst is NULL.");
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }

    uint32_t flip_rotate = m_drv_param.rotate_mode;
    if (src_x >= src_pitch || src_y >= src_height) {
      WARN("No need to fill\n");
      break;
    }
    if (data_width + src_x > src_pitch) {
      WARN("Failed to fill overlay data. Rotate flag[%u]. Src width:%u must"
           " >= data width:%u + offset x:%u\n",
           flip_rotate, src_pitch, data_width, src_x);
      data_width = src_pitch - src_x;
    }
    if (data_height + src_y > src_height) {
      WARN("Failed to fill overlay data. Rotate flag[%u]. Src height:%u must"
           " >= data height:%u + offset y:%u\n",
           flip_rotate, src_height, data_height, src_y);
      data_height = src_height - src_y;
    }

    uint8_t *sp = src + src_y * src_pitch + src_x;
    uint8_t *dp = nullptr;
    uint32_t row = 0, col = 0;
    switch (flip_rotate) {
      case AM_NO_ROTATE_FLIP:
        dp = dst + dst_y * dst_pitch + dst_x;
        for (row = 0; row < data_height; ++ row) {
          memcpy(dp, sp, data_width);
          sp += src_pitch;
          dp += dst_pitch;
        }
        break;
      case AM_CW_ROTATE_90:
        dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_y;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp - col * dst_pitch) = *(sp + col);
          }
          sp += src_pitch;
          dp ++;
        }
        break;
      case AM_HORIZONTAL_FLIP:
        dp = dst + dst_y * dst_pitch + dst_width - dst_x - 1;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp - col) = *(sp + col);
          }
          sp += src_pitch;
          dp += dst_pitch;
        }
        break;
      case AM_VERTICAL_FLIP:
        dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_x;
        for (row = 0; row < data_height; ++ row) {
          memcpy(dp, sp, data_width);
          sp += src_pitch;
          dp -= dst_pitch;
        }
        break;
      case AM_CW_ROTATE_180:
        dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_width - dst_x - 1;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp - col) = *(sp + col);
          }
          sp += src_pitch;
          dp -= dst_pitch;
        }
        break;
      case AM_CW_ROTATE_270:
        dp = dst + dst_x * dst_pitch + dst_width - dst_y - 1;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp + col * dst_pitch) = *(sp + col);
          }
          sp += src_pitch;
          dp --;
        }
        break;
      case (AM_HORIZONTAL_FLIP | AM_ROTATE_90):
        dp = dst + dst_x * dst_pitch + dst_y;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp + col * dst_pitch) = *(sp + col);
          }
          sp += src_pitch;
          dp ++;
        }
        break;
      case (AM_VERTICAL_FLIP | AM_ROTATE_90):
        dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_width - dst_y - 1;
        for (row = 0; row < data_height; ++ row) {
          for (col = 0; col < data_width; ++ col) {
            *(dp - col * dst_pitch) = *(sp + col);
          }
          sp += src_pitch;
          dp --;
        }
        break;
      default:
        break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayArea::adjust_area_param()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    uint16_t width = 0, height = 0, offset_x = 0, offset_y = 0;
    AMOverlayAreaInsert &overlay_area = m_drv_param;

    if (overlay_area.width < OVERLAY_WIDTH_ALIGN
        || overlay_area.height < OVERLAY_HEIGHT_ALIGN) {
      ERROR("Area width[%u] x height[%u] cannot be smaller than [%ux%u]",
            overlay_area.width, overlay_area.height,
            OVERLAY_WIDTH_ALIGN, OVERLAY_HEIGHT_ALIGN);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (overlay_area.width & (OVERLAY_WIDTH_ALIGN - 1)) {
      width = ROUND_DOWN(overlay_area.width, OVERLAY_WIDTH_ALIGN);
      INFO("Area width[%u] is not aligned to %u. Round down to [%u].",
           overlay_area.width, OVERLAY_WIDTH_ALIGN, width);
      overlay_area.width = width;
    }

    if (overlay_area.start_x & (OVERLAY_OFFSET_X_ALIGN - 1)) {
      offset_x = ROUND_DOWN(overlay_area.start_x, OVERLAY_OFFSET_X_ALIGN);
      INFO("Area offset x[%u] is not aligned to %u. Round down to [%u].",
           overlay_area.start_x, OVERLAY_OFFSET_X_ALIGN, offset_x);
      overlay_area.start_x = offset_x;
    }

    if (overlay_area.height & (OVERLAY_HEIGHT_ALIGN - 1)) {
      height = ROUND_DOWN(overlay_area.height, OVERLAY_HEIGHT_ALIGN);
      INFO("Area height[%u] is not aligned to %u. Round down to [%u].",
           overlay_area.height, OVERLAY_HEIGHT_ALIGN, height);
      overlay_area.height = height;
    }

    if (overlay_area.start_y & (OVERLAY_OFFSET_Y_ALIGN - 1)) {
      offset_y = ROUND_DOWN(overlay_area.start_y, OVERLAY_OFFSET_Y_ALIGN);
      INFO("Area offset_y [%u] is not aligned to %u. Round down to [%u].",
           overlay_area.start_y, OVERLAY_OFFSET_Y_ALIGN, offset_y);
      overlay_area.start_y = offset_y;
    }

    overlay_area.pitch = ROUND_UP(overlay_area.width, OVERLAY_PITCH_ALIGN);
    overlay_area.total_size = overlay_area.pitch * overlay_area.height;

    if (overlay_area.total_size
        > (overlay_area.area_size_max / m_param.attr.buf_num)) {
      ERROR("Area size[%u] = [%u x %u] exceeds the max area size[%u] / "
            "buffer number[%d].",
            overlay_area.total_size, overlay_area.pitch, overlay_area.height,
            overlay_area.area_size_max, m_param.attr.buf_num);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (m_drv_param.rotate_mode & AM_ROTATE_90) {
      m_param.attr.rect.size.width = overlay_area.height;
      m_param.attr.rect.size.height = overlay_area.width;
    } else {
      m_param.attr.rect.size.width = overlay_area.width;
      m_param.attr.rect.size.height = overlay_area.height;
    }
    DEBUG("After aligned: local memory[%ux%u], mmap buffer[%ux%u]"
          "(pitch[%u]) + offset[%ux%u].",
          m_param.attr.rect.size.width, m_param.attr.rect.size.height,
          overlay_area.height, overlay_area.width, overlay_area.pitch,
          overlay_area.start_x, overlay_area.start_y);
  } while (0);

  return result;
}

AM_RESULT AMOverlayArea::alloc_area_mem()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    uint32_t display_size = m_param.attr.rect.size.width
        * m_param.attr.rect.size.height;
    if (display_size) {
      delete[] m_buffer;
      m_buffer = (uint8_t *) new uint8_t[display_size];

      if (!m_buffer) {
        ERROR("Can't allot memory[%u] for local buffer.", display_size);
        result = AM_RESULT_ERR_MEM;
        break;
      }
      memset(m_buffer, CLUT_ENTRY_BACKGOURND, display_size);
    } else {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Display size [%ux%u] cannot be zero.",
            m_param.attr.rect.size.width, m_param.attr.rect.size.height);
      break;
    }
  } while (0);

  return result;
}

int32_t AMOverlayArea::get_available_index()
{
  int32_t index = -1;
  do {
    for (uint32_t i = 0; i < m_data.size(); ++ i) {
      if (nullptr == m_data[i]) {
        index = i;
        break;
      }
    }
    if (-1 == index) {
      index = m_data.size();
      m_data.push_back(nullptr);
      AMOverlayAreaData data;
      m_param.data.push_back(data);
    }
  } while (0);

  return index;
}

void AMOverlayArea::switch_buffer()
{
  if (m_param.attr.buf_num > 1) {
    m_cur_buf = (m_cur_buf + 1 >= m_param.attr.buf_num) ? 0 : m_cur_buf + 1;
    if (m_cur_buf == 0) {
      m_drv_param.data_addr_offset -=
          (m_param.attr.buf_num - 1) * m_drv_param.total_size;
      m_drv_param.data_addr -=
          (m_param.attr.buf_num - 1) * m_drv_param.total_size;
    } else {
      m_drv_param.data_addr_offset += m_drv_param.total_size;
      m_drv_param.data_addr += m_drv_param.total_size;
    }
  }
}
