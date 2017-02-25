/*******************************************************************************
 * am_encode_overlay_data_animate.cpp
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
#include "am_encode_overlay_data_animate.h"

AMOverlayData* AMOverlayAnimData::create(AMOverlayArea *area,
                                         AMOverlayAreaData *data)
{
  AMOverlayAnimData *result = new AMOverlayAnimData(area);
  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayAnimData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayAnimData::AMOverlayAnimData(AMOverlayArea *area) :
    AMOverlayPicData(area),
    m_update_count(0),
    m_cur_idx(0)
{
}

AMOverlayAnimData::~AMOverlayAnimData()
{
  m_data.clear();
}

AM_RESULT AMOverlayAnimData::add(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  FILE * fp = nullptr;
  uint8_t *buffer = nullptr;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    int32_t cn = 0;
    int32_t &width = data->rect.size.width;
    int32_t &height = data->rect.size.height;
    const AMOverlayAnimation &anim = data->anim;
    const AMOverlayPicture &pic = anim.pic;

    if (!(fp = fopen(pic.filename.c_str(), "r"))) {
      ERROR("failed to open bitmap file [%s].", pic.filename.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }

    if ((result = init_bitmap_info(fp, cn, width, height)) != AM_RESULT_OK) {
      break;
    }

    if (anim.num <= 0) {
      ERROR("Sub-picture number: %d is invalid!!!", anim.num);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if ((result=check_block_rect_param(width, height / anim.num,
                                       data->rect.offset.x, data->rect.offset.y))
        != AM_RESULT_OK) {
      break;
    }

    int32_t bmp_total_size = width * height;
    buffer = (uint8_t *) new uint8_t[bmp_total_size];
    if (!buffer) {
      ERROR("alloc memory failed!!!");
      result = AM_RESULT_ERR_MEM;
      break;
    }
    if ((!m_buffer)
        || ((width != m_param.rect.size.width)
            || height != m_param.rect.size.height)) {
      delete[] m_buffer;
      m_buffer = (uint8_t*)new uint8_t[bmp_total_size];
    }
    if (!m_buffer) {
      ERROR("alloc memory failed!!!");
      delete[] buffer;
      result = AM_RESULT_ERR_MEM;
      break;
    }

    //make a copy of bmp data info
    fseek(fp, cn * sizeof(AMOverlayRGB), SEEK_CUR);
    fread(buffer, bmp_total_size, 1, fp);

    fseek(fp, -(cn * sizeof(AMOverlayRGB) + bmp_total_size), SEEK_CUR);
    if ((result = make_bmp_clut(fp, cn, bmp_total_size, pic.colorkey, buffer))
        != AM_RESULT_OK) {
      break;
    }

    //because bmp data store order is from bottom to top, so adjust it here
    for (int32_t h = 0; h < height; ++ h) {
      memcpy(m_buffer + h * width, buffer + ((height - 1) - h) * width, width);
    }

    height /= anim.num;
    for (int32_t i = 0; i < anim.num; ++ i) {
      uint8_t *buf = m_buffer + i * width * height;
      m_data.push_back(buf);
    }

    if (anim.interval < 0) {
      WARN("Invalid interval parameter:%d, set to default value:1\n",
           data->anim.interval);
      data->anim.interval = 1;
    }
    m_param = *data;
  } while (0);

  if (fp) {
    fclose(fp);
  }
  delete[] buffer;

  return result;
}

AM_RESULT AMOverlayAnimData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      if (((++ m_update_count) % (m_param.anim.interval + 1)) != 0) {
        break;
      }
      if ((++ m_cur_idx) >= m_param.anim.num) {
        m_cur_idx = 0;
      }

      AMRect data;
      data.size = m_param.rect.size;
      data.offset = {0};
      result = m_area->update_drv_data(m_data[m_cur_idx], m_param.rect.size,
                                       data, m_param.rect.offset, false);
    } else {
      if (memcmp(&(data->anim), &(m_param.anim),
                 sizeof(AMOverlayAnimation)) == 0) {
        break;
      }
      data->rect = m_param.rect;
      m_update_count = 0;
      m_cur_idx = 0;
      m_data.clear();
      if ((result = blank()) != AM_RESULT_OK ||
          (result = add(data)) != AM_RESULT_OK) {
        ERROR("Update animation overlay failed!\n");
        break;
      }
    }
  } while (0);

  return result;
}
