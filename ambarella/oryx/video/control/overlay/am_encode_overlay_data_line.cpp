/*******************************************************************************
 * am_encode_overlay_data_line.cpp
 *
 * History:
 *   2016-12-6 - [Huaiqing Wang] created file
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
#include "am_encode_overlay_data_line.h"

static const AMOverlayCLUT color[AM_OVERLAY_COLOR_NUM] =
{                         { 128, 128, 235, 255 }, /* white */
                          { 128, 128, 12, 255 }, /* black */
                          { 240, 90, 82, 255 }, /* red */
                          { 110, 240, 41, 255 }, /* blue */
                          { 34, 54, 145, 255 }, /* green */
                          { 146, 16, 210, 255 }, /* yellow */
                          { 16, 166, 170, 255 }, /* cyan */
                          { 222, 202, 107, 255 }, /* magenta */
};

AMOverlayData* AMOverlayLineData::create(AMOverlayArea *area,
                                         AMOverlayAreaData *data)
{
  AMOverlayLineData *result = new AMOverlayLineData(area);
  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayLineData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayLineData::AMOverlayLineData(AMOverlayArea *area) :
    AMOverlayData(area)
{
}

AMOverlayLineData::~AMOverlayLineData()
{
}

AM_RESULT AMOverlayLineData::add(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    index_set_map tmp;
    uint8_t index = 0;

    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }

    AMRect &rect = data->rect;
    if ((result=check_block_rect_param(rect.size.width, rect.size.height,
                                       rect.offset.x, rect.offset.y))
        != AM_RESULT_OK) {
      break;
    }

    int32_t buf_size = rect.size.width * rect.size.height;
    if (!m_buffer) {
      m_buffer = (uint8_t *) new uint8_t[buf_size];
      if (!m_buffer) {
        ERROR("Can't allot memory[%u].", buf_size);
        break;
      }
    }
    memset(m_buffer, CLUT_ENTRY_BACKGOURND, buf_size);

    AMOverlayLine &line = data->line;
    if (line.thickness <= 0) {
      line.thickness = 1;
    }
    if (line.color.id != AM_OVERLAY_COLOR_CUSTOM) {
      memcpy(&(line.color.color),
             &color[line.color.id], sizeof(AMOverlayCLUT));
    }

    if ((result = adjust_clut_and_data(line.color.color, &index, 1, 0, tmp))
        != AM_RESULT_OK) {
      break;
    }

    for (uint32_t i = 0; i < line.point.size() - 1; ++ i) {
      AMPoint &start = line.point[i];
      AMPoint &end = line.point[i+1];
      if ((result=check_point_param(start, rect.size)) != AM_RESULT_OK ||
          (result=check_point_param(end, rect.size)) != AM_RESULT_OK) {
        break;
      }
      draw_straight_line(start, end, index, line.thickness, rect.size);
    }
    if (result != AM_RESULT_OK) {
      break;
    }

    AMRect data_rect;
    data_rect.size = data->rect.size;
    data_rect.offset = {0};
    if ((result = m_area->update_drv_data(m_buffer, data->rect.size, data_rect,
                                          data->rect.offset, true))
        != AM_RESULT_OK) {
      break;
    }

    m_param = *data;
  } while (0);

  return result;
}

AM_RESULT AMOverlayLineData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (memcmp(&data->line, &m_param.line, sizeof(data->text)) == 0) {
      break;
    }
    if (data->type != m_param.type) {
      DEBUG("Don't change data type with update");
      data->type = m_param.type;
    }
    if (memcmp(&(data->rect), &(m_param.rect), sizeof(AMRect)) != 0) {
      DEBUG("Don't change area data block size and offset with update!\n");
      data->rect = m_param.rect;
    }

    if ((result = add(data)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayLineData::blank()
{
  return AMOverlayData::blank();
}

AM_RESULT AMOverlayLineData::check_point_param(AMPoint &p, AMResolution &size)
{
  AM_RESULT result = AM_RESULT_OK;
  if (p.x >= size.width || p.y >= size.height) {
    ERROR("point(%d,%d) exceed block's rect(%d,%d)\n",
        p.x, p.y, size.width-1, size.height-1);
    result = AM_RESULT_ERR_INVALID;
  }

  return result;
}

void AMOverlayLineData::draw_straight_line(AMPoint &p1, AMPoint &p2,
                                           uint8_t color, int32_t thickness,
                                           const AMResolution &size)
{
  do {
    uint32_t location = 0;
    int32_t xl, xr, yl, yh;
    int32_t dx, dy;

    if (p1.x > p2.x) {
      xl = p2.x;
      xr = p1.x;
    } else {
      xl = p1.x;
      xr = p2.x;
    }
    if (p1.y > p2.y) {
      yl = p2.y;
      yh = p1.y;
    } else {
      yl = p1.y;
      yh = p2.y;
    }

    if (p1.x == p2.x) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        if (p1.x + tn >= size.width) {
          break;
        }
        for (int32_t y = yl; y <= yh; ++ y) {
          location = y * size.width + p1.x + tn;
          m_buffer[location] = color;
        }
      }
      break;
    }

    if (p1.y == p2.y) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        if (p1.y + tn >= size.height) {
          break;
        }
        for (int32_t x = xl; x <= xr; ++ x) {
          location = (p1.y + tn) * size.width + x;
          m_buffer[location] = color;
        }
      }
      break;
    }

    dx = 2 * (p2.x - p1.x);
    dy = 2 * (p2.y - p1.y);
    for (int32_t x = xl; x <= xr; ++ x) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        int32_t y = (p1.y * dx - p1.x * dy + x * dy + dx / 2) / dx + tn;
        if (y >= size.height) {
          break;
        }
        location = y * size.width + x;
        m_buffer[location] = color;
      }
    }

    for (int32_t y = yl; y <= yh; ++ y) {
      for (int32_t tn = 0; tn < thickness; ++tn) {
        int32_t x = (p1.x * dy - p1.y * dx + y * dx + dy / 2) / dy + tn;
        if (x >= size.width) {
          break;
        }
        location = y * size.width + x;
        m_buffer[location] = color;
      }
    }
  } while (0);
}
