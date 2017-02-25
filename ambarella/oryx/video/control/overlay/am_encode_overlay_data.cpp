/*******************************************************************************
 * am_encode_overlay_data.cpp
 *
 * History:
 *   2016-3-15 - [ypchang] created file
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

#include "am_encode_overlay_data.h"
#include "am_encode_overlay_area.h"

/*
 * class AMOverlayData
 */

void AMOverlayData::destroy()
{
  delete this;
}

AMOverlayData::AMOverlayData(AMOverlayArea *area) :
    m_area(area),
    m_buffer(nullptr)
{
}

AMOverlayData::~AMOverlayData()
{
  delete[] m_buffer;
}

const AMOverlayAreaData& AMOverlayData::get_param()
{
  return m_param;
}

AM_RESULT AMOverlayData::blank()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //fill background index
    memset(m_buffer, CLUT_ENTRY_BACKGOURND,
           m_param.rect.size.width * m_param.rect.size.height);

    AMRect rect;
    rect.size = m_param.rect.size;
    rect.offset = {0};
    if ((result = m_area->update_drv_data(m_buffer, m_param.rect.size, rect,
                                          m_param.rect.offset, true))
        != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayData::adjust_clut_and_data(const AMOverlayCLUT &clut,
                                              uint8_t *buf,
                                              int32_t size,
                                              int32_t cur_index,
                                              index_set_map &backup)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!buf) {
      ERROR("invalid pointer!!!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    int32_t c;
    //index 0 is reserve for backgroud color
    int32_t &cur_color_num = m_area->get_clut_color_num();
    AMOverlayCLUT *local_clut = m_area->get_clut();
    for (c = 1; c < cur_color_num; ++ c) {
      if (local_clut[c].y == clut.y && local_clut[c].u == clut.u
          && local_clut[c].v == clut.v && local_clut[c].a == clut.a) {
        DEBUG("find a exist same clut[%d,%d,%d,%d]\n",
              local_clut[c].y, local_clut[c].u,
              local_clut[c].v, local_clut[c].a);

        //have find it, ajust picture data info's index if needed
        index_set_map::iterator it = backup.find(cur_index);
        for (int32_t d = 0; d < size; ++ d) {
          if (it != backup.end()) {
            //backup next index avoid it override by new index
            if (buf[d] == c) {
              DEBUG("tmp backup data:%d clut index: %d", d, c);
              backup[c].push_back(d);
            }
          } else {
            if (cur_index != c) {
              //backup next index avoid it override by new index
              if (buf[d] == c) {
                DEBUG("tmp backup data:%d clut index: %d", d, c);
                backup[c].push_back(d);
              } else if (buf[d] == cur_index) {
                buf[d] = c;
                DEBUG("change data:%d clut idex from %d to %d\n",
                      d, cur_index, c);
              }
            }
          }
        }
        if (it != backup.end()) {
          for (auto m : it->second) {
            buf[m] = c;
            DEBUG("change data:%d clut idex from %d to %d\n", m, cur_index, c);
          }
          backup.erase(it);
        }
        //find a same color in m_clut, then break the loop
        break;
      }
    }

    /* if not find a same color in m_clut filled before, then use
     * the color as a new color and fill it in the m_clut*/
    if (c == cur_color_num) {
      if (cur_color_num
          < int32_t(OVERLAY_CLUT_SIZE / (sizeof(AMOverlayCLUT)))) {
        index_set_map::iterator it = backup.find(cur_index);
        for (int32_t d = 0; d < size; ++ d) {
          if (it != backup.end()) {
            if (buf[d] == cur_color_num) {
              //backup next index avoid it override by new index
              backup[cur_color_num].push_back(d);
              DEBUG("tmp backup data:%d clut index: %d\n",
                    d, cur_color_num);
            }
          } else {
            if (cur_index != cur_color_num) {
              if (buf[d] == cur_index) {
                buf[d] = cur_color_num;
                DEBUG("change data:%d clut idex from %d to %d\n",
                      d, cur_index, cur_color_num);
              } else if (buf[d] == cur_color_num) {
                //backup next index avoid it override by new index
                DEBUG("tmp backup data:%d clut index: %d\n",
                      d, cur_color_num);
                backup[cur_color_num].push_back(d);
              }
            }
          }
        }

        if (it != backup.end()) {
          for (auto m : it->second) {
            buf[m] = cur_color_num;
            DEBUG("change backup data:%d clut idex from %d to %d\n",
                  m, cur_index, cur_color_num);
          }
          backup.erase(it);
        }

        local_clut[cur_color_num ++] = clut;
        INFO("current color number = %d\n", cur_color_num);
        INFO("fill a new clut[%d,%d,%d,%d]\n", clut.y, clut.u, clut.v, clut.a);
      } else {
        WARN("area total color number is %d now, can't add more!",
             OVERLAY_CLUT_SIZE / (sizeof(AMOverlayCLUT)));
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayData::check_block_rect_param(int32_t w, int32_t h,
                                                int32_t x, int32_t y)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    const AMRect &area = m_area->get_param().attr.rect;
    if (w > area.size.width || h > area.size.height) {
      ERROR("Data block width:%d or height:%d is large than area's width:%d or"
          " height:%d\n", w, h, area.size.width, area.size.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (w + x > area.size.width || h + y > area.size.height) {
      ERROR("Data block width:%d + offset_x:%d > area's width:%d, "
          "or picture height:%d + offset_y:%d > area's height:%d\n",
          w, x, area.size.width, h, y, area.size.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  return result;
}
