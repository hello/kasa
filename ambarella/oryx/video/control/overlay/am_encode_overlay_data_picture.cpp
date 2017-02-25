/*******************************************************************************
 * am_encode_overlay_data_picture.cpp
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
#include "am_encode_overlay_data_picture.h"

AMOverlayData* AMOverlayPicData::create(AMOverlayArea *area,
                                        AMOverlayAreaData *data)
{
  AMOverlayPicData *result = new AMOverlayPicData(area);
  if (AM_LIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayPicData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayPicData::AMOverlayPicData(AMOverlayArea *area) :
    AMOverlayData(area)
{
}

AMOverlayPicData::~AMOverlayPicData()
{
}

AM_RESULT AMOverlayPicData::add(AMOverlayAreaData *data)
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
    const AMOverlayPicture &pic = data->pic;

    if (!(fp = fopen(pic.filename.c_str(), "r"))) {
      ERROR("failed to open bitmap file [%s].", pic.filename.c_str());
      result = AM_RESULT_ERR_IO;
      break;
    }

    if ((result = init_bitmap_info(fp, cn, width, height)) != AM_RESULT_OK) {
      break;
    }

    if ((result=check_block_rect_param(width, height,
                                       data->rect.offset.x, data->rect.offset.y))
        != AM_RESULT_OK) {
      break;
    }

    int32_t bmp_size = width * height;
    buffer = (uint8_t*)new uint8_t[bmp_size];
    if (!buffer) {
      ERROR("alloc memory failed!!!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if ((!m_buffer)
        || ((width != m_param.rect.size.width)
            || height != m_param.rect.size.height)) {
      delete[] m_buffer;
      m_buffer = (uint8_t*)new uint8_t[bmp_size];
    }
    if (!m_buffer) {
      ERROR("alloc memory failed!!!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    //make a copy of bmp data info
    fseek(fp, cn * sizeof(AMOverlayRGB), SEEK_CUR);
    fread(buffer, bmp_size, 1, fp);

    fseek(fp, -(cn * sizeof(AMOverlayRGB) + bmp_size), SEEK_CUR);
    if ((result = make_bmp_clut(fp, cn, bmp_size, pic.colorkey, buffer))
        != AM_RESULT_OK) {
      break;
    }

    //because bmp data store order is from bottom to top, so adjust it here
    for (int32_t h = 0; h < height; ++ h) {
      memcpy(m_buffer + h * width, buffer + ((height - 1) - h) * width, width);
    }

    AMRect rect;
    rect.size = data->rect.size;
    rect.offset = {0};
    if ((result = m_area->update_drv_data(m_buffer, data->rect.size, rect,
                                          data->rect.offset, true))
        != AM_RESULT_OK) {
      break;
    }

    m_param = *data;
  } while (0);

  delete[] buffer;
  if (fp) {
    fclose(fp);
  }
  return result;
}

AM_RESULT AMOverlayPicData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (data->type != m_param.type) {
      DEBUG("Don't change data type with update");
      data->type = m_param.type;
    }
    if (memcmp(&data->pic, &m_param.pic, sizeof(data->pic)) == 0) {
      break;
    }
    if (memcmp(&(data->rect.offset), &(m_param.rect.offset), sizeof(AMOffset))
        != 0) {
      DEBUG("Don't change area data offset with update!\n");
      data->rect.offset = m_param.rect.offset;
    }

    if ((result = add(data)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayPicData::init_bitmap_info(FILE *fp,
                                             int32_t &cn,
                                             int32_t &w,
                                             int32_t &h)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!fp) {
      ERROR("invalid pointer!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    AMOverlayBmpFileHeader fileHeader = { 0 };
    AMOverlayBmpInfoHeader infoHeader = { 0 };

    fread(&fileHeader.bfType, sizeof(fileHeader.bfType), 1, fp);
    fread(&fileHeader.bfSize, sizeof(fileHeader.bfSize), 1, fp);
    fread(&fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1), 1, fp);
    fread(&fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2), 1, fp);
    fread(&fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits), 1, fp);

    uint32_t type = fileHeader.bfType[1] << 8 | fileHeader.bfType[0];
    if (type != OVERLAY_BMP_MAGIC) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid type [%d]. Not a bitmap.", type);
      break;
    }

    fread(&infoHeader, sizeof(AMOverlayBmpInfoHeader), 1, fp);
    if (infoHeader.biBitCount != OVERLAY_BMP_BIT) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid [%u]bit. Only support %u bit bitmap.",
            infoHeader.biBitCount,
            OVERLAY_BMP_BIT);
      break;
    }
    if (infoHeader.biSizeImage != (infoHeader.biWidth * infoHeader.biHeight)) {
      WARN("Invalid image size [%u]. Not equal to width[%u] x height[%u].",
           infoHeader.biSizeImage,
           infoHeader.biWidth,
           infoHeader.biHeight);
    }

    if (infoHeader.biWidth & 0x3) {
      ERROR("Picture width:%d is not align with 4!!!", infoHeader.biWidth);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    uint32_t offbit = fileHeader.bfOffBits[3] << 24
        | fileHeader.bfOffBits[2] << 16 | fileHeader.bfOffBits[1] << 8
        | fileHeader.bfOffBits[0];
    cn = (offbit - sizeof(AMOverlayBmpFileHeader)
        - sizeof(AMOverlayBmpInfoHeader)) / sizeof(AMOverlayRGB);

    if (cn > int32_t(OVERLAY_CLUT_SIZE / sizeof(AMOverlayCLUT))) {
      WARN("OffsetBits [%u], color number = %u (>[%u]). Reset to max.",
           offbit,
           cn,
           OVERLAY_CLUT_SIZE / sizeof(AMOverlayCLUT));
      cn = OVERLAY_CLUT_SIZE / sizeof(AMOverlayCLUT);
    }
    INFO("color number = %d!!!\n", cn);

    w = infoHeader.biWidth;
    h = infoHeader.biHeight;
  } while (0);

  return result;
}

/* because two float values do subtraction may occur
 * indivisible problem, so we will do a round for it*/
uint32_t AMOverlayPicData::get_proper_value(float value)
{
  uint32_t tmp = 0;
  if (value - (int32_t) value > 0.5f
      || (value < 0 && value - (int32_t) value > -0.5f)) {
    tmp = (uint32_t) value + 1;
  } else {
    tmp = (uint32_t) value;
  }

  return tmp;
}

void AMOverlayPicData::rgb_to_yuv(const AMOverlayRGB &src, AMOverlayCLUT &dst)
{
  float tmp;
  dst.y = (uint8_t) (0.257f * src.r + 0.504f * src.g + 0.098f * src.b + 16);

  tmp = 0.439f * src.b - 0.291f * src.g - 0.148f * src.r + 128;
  dst.u = (uint8_t) get_proper_value(tmp);

  tmp = 0.439f * src.r - 0.368f * src.g - 0.071f * src.b + 128;
  dst.v = (uint8_t) get_proper_value(tmp);
  dst.a = OVERLAY_NONE_TRANSPARENT;
}

AM_RESULT AMOverlayPicData::make_bmp_clut(FILE *fp,
                                          uint32_t cn,
                                          uint32_t size,
                                          const AMOverlayColorKey &ck,
                                          uint8_t *buf)
{
  AM_RESULT result = AM_RESULT_OK;
  clut_used_map clut_used;
  index_set_map data;
  do {
    if (!fp || !buf) {
      ERROR("invalid pointer!!!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    /* some bmp's clut have many color,but bmp data just
     * use some of them,so pick out the real used color*/
    for (uint32_t i = 0; i < cn; ++ i) {
      clut_used[i] = false;
      for (uint32_t d = 0; d < size; ++ d) {
        if (buf[d] == i) {
          clut_used[i] = true;
          break;
        }
      }
    }

    AMOverlayCLUT clut;
    AMOverlayRGB rgb = { 0 };
    for (uint32_t i = 0; i < cn; ++ i) {
      fread(&rgb, sizeof(AMOverlayRGB), 1, fp);
      if (false == clut_used[i]) {
        continue;
      }
      rgb_to_yuv(rgb, clut);
      if ((result = adjust_clut_and_data(clut, buf, size, i, data))
          != AM_RESULT_OK) {
        break;
      }
    }

    //do transparent
    AMOverlayCLUT ckey = ck.color;
    uint8_t cr = ck.range;
    int32_t &cur_color_num = m_area->get_clut_color_num();
    AMOverlayCLUT *local_clut = m_area->get_clut();
    for (int32_t c = 1; c < cur_color_num; ++ c) {
      if ((local_clut[c].y <= ckey.y + cr
          && local_clut[c].y + cr >= ckey.y)
          && (local_clut[c].u <= ckey.u + cr
              && local_clut[c].u + cr >= ckey.u)
          && (local_clut[c].v <= ckey.v + cr
              && local_clut[c].v + cr >= ckey.v)) {
        DEBUG("User transparent color(%d,%d,%d), Clut index = %d!!!",
              local_clut[c].y, local_clut[c].u, local_clut[c].v, c);
        for (uint32_t d = 0; d < size; ++ d) {
          if (buf[d] == c) {
            buf[d] = CLUT_ENTRY_BACKGOURND;
          }
        }
      }
    }
  } while (0);

  data.clear();
  clut_used.clear();
  return result;
}
