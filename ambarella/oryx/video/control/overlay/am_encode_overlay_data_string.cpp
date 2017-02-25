/*******************************************************************************
 * am_encode_overlay_data_string.cpp
 *
 * History:
 *   Mar 28, 2016 - [ypchang] created file
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
#include "am_encode_overlay_data_string.h"

#include "text_insert.h"

#include <sys/types.h>
#include <dirent.h>
#include <locale.h>
#include <wchar.h>

static const AMOverlayCLUT color[] = { { 128, 128, 235, 255 }, /* white */
                                       { 128, 128, 12, 255 }, /* black */
                                       { 240, 90, 82, 255 }, /* red */
                                       { 110, 240, 41, 255 }, /* blue */
                                       { 34, 54, 145, 255 }, /* green */
                                       { 146, 16, 210, 255 }, /* yellow */
                                       { 16, 166, 170, 255 }, /* cyan */
                                       { 222, 202, 107, 255 }, /* magenta */
};

bool AMOverlayStrData::m_font_lib_init = false;

AMOverlayData* AMOverlayStrData::create(AMOverlayArea *area,
                                        AMOverlayAreaData *data)
{
  AMOverlayStrData *result = new AMOverlayStrData(area);
  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayStrData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayStrData::AMOverlayStrData(AMOverlayArea *area) :
    AMOverlayData(area),
    m_clut(nullptr)
{
}

AMOverlayStrData::~AMOverlayStrData()
{
  delete[] m_clut;
  m_clut = nullptr;
  close_textinsert_lib();
}

AM_RESULT AMOverlayStrData::add(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (!m_clut) {
      m_clut = (AMOverlayCLUT*) new uint8_t[OVERLAY_CLUT_SIZE];
    }
    if (!m_clut) {
      ERROR("allot memory for clut failed!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMRect &rect = data->rect;
    if ((result=check_block_rect_param(rect.size.width, rect.size.height,
                                       rect.offset.x, rect.offset.y))
        != AM_RESULT_OK) {
      break;
    }

    AMOverlayTextBox &text = data->text;
    if ((result = init_text_info(text, m_clut)) != AM_RESULT_OK) {
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

    if ((result = convert_text_to_bmp(text, rect)) != AM_RESULT_OK) {
      ERROR("convert text to bmp failed!!!");
      break;
    }

    if ((result = make_text_clut(buf_size, m_buffer, m_clut)) != AM_RESULT_OK) {
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

AM_RESULT AMOverlayStrData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      result = AM_RESULT_ERR_DATA_POINTER;
      break;
    }
    if (memcmp(&data->text, &m_param.text, sizeof(data->text)) == 0) {
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

AM_RESULT AMOverlayStrData::blank()
{
  return AMOverlayData::blank();
}

AM_RESULT AMOverlayStrData::find_available_font(string &font)
{
  AM_RESULT result = AM_RESULT_ERR_UNKNOWN;
  do {
    DIR *dir;
    struct dirent *ptr;
    string fonts_dir = "/usr/share/fonts/";
    char *font_extension = nullptr;

    if ((dir = opendir(fonts_dir.c_str())) == nullptr) {
      result = AM_RESULT_ERR_IO;
      perror("Open fonts dir error...");
      break;
    }
    while ((ptr = readdir(dir)) != nullptr) {
      if (0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name, ".."))
        continue;
      else if ((font_extension = strchr(ptr->d_name, '.')) != nullptr) {
        if (0 == strcmp(font_extension, ".ttf")
            || 0 == strcmp(font_extension, ".ttc")) {
          font = fonts_dir + ptr->d_name;
          DEBUG("Using a available font: \"%s\"\n", font.c_str());
          result = AM_RESULT_OK;
          break;
        }
      }
    }
    closedir(dir);
    if (result != AM_RESULT_OK) {
      ERROR("No available fonts in fonts dir: %s\n", fonts_dir.c_str());
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayStrData::init_text_info(AMOverlayTextBox &text,
                                           AMOverlayCLUT *clut)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!clut) {
      ERROR("invalid pointer!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    AMOverlayFont &font = text.font;

    font.width = (font.width > 0) ? font.width : DEFAULT_OVERLAY_FONT_SIZE;
    font.height =
        (font.height > font.width) ? font.height : (font.width * 3 / 2);
    font.outline_width =
        (font.outline_width > 0) ? font.outline_width :
            DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH;
    font.hor_bold = (font.hor_bold < 100) ?
        (font.hor_bold > -100 ? font.hor_bold : -100) : 100;
    font.ver_bold = (font.ver_bold < 100) ?
        (font.ver_bold > -100 ? font.ver_bold : -100) : 100;
    DEBUG("font width = %d, height = %d \n", font.width, font.height);

    if (font.ttf_name.size()) {
      if (access(font.ttf_name.c_str(), F_OK) == -1) {
        WARN("Font \"%s\" is not exist!\n", font.ttf_name.c_str());
        if ((result = find_available_font(font.ttf_name)) != AM_RESULT_OK) {
          break;
        }
      }
    } else {
      DEBUG("No font type parameter!\n");
      if ((result = find_available_font(font.ttf_name)) != AM_RESULT_OK) {
        break;
      }
    }

    font.italic = (font.italic <= 0) ? 0 :
        ((font.italic <= 100) ? font.italic : 100);
    font.disable_anti_alias = 0;

    uint32_t color_id = text.font_color.id;
    if (color_id >= 0 && color_id < 8) {
      memcpy(&(text.font_color.color), &color[color_id], sizeof(AMOverlayCLUT));
    }

    AMOverlayCLUT &bc = text.background_color;
    if ((bc.a == 0) && (bc.u == 0) && (bc.v == 0) && (bc.y == 0)) {
      bc.y = DEFAULT_OVERLAY_BACKGROUND_Y;
      bc.u = DEFAULT_OVERLAY_BACKGROUND_U;
      bc.v = DEFAULT_OVERLAY_BACKGROUND_V;
      bc.a = DEFAULT_OVERLAY_BACKGROUND_ALPHA;
      DEBUG("Use default backgroud color!!! \n");
    }

    if (font.outline_width) {
      AMOverlayCLUT &oc = text.outline_color;
      if ((oc.a == 0) && (oc.u == 0) && (oc.v == 0) && (oc.y == 0)) {
        oc.y = DEFAULT_OVERLAY_OUTLINE_Y;
        oc.u = DEFAULT_OVERLAY_OUTLINE_U;
        oc.v = DEFAULT_OVERLAY_OUTLINE_V;
        oc.a = DEFAULT_OVERLAY_OUTLINE_ALPHA;
        DEBUG("Use default outline color!!! \n");
      }
    }

    // Fill font color
    for (uint32_t i = 0; i < (OVERLAY_CLUT_SIZE / sizeof(AMOverlayCLUT));
        ++ i) {
      clut[i].y = text.font_color.color.y;
      clut[i].u = text.font_color.color.u;
      clut[i].v = text.font_color.color.v;
      clut[i].a = ((i < text.font_color.color.a) ? i : text.font_color.color.a);
    }
    // Fill Background color
    memcpy(&clut[TEXT_CLUT_ENTRY_BACKGOURND],
           &text.background_color,
           sizeof(AMOverlayCLUT));
    // Fill Outline color
    memcpy(&clut[TEXT_CLUT_ENTRY_OUTLINE],
           &text.outline_color,
           sizeof(AMOverlayCLUT));

    if ((result = init_textinsert_lib(text.font)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayStrData::init_textinsert_lib(const AMOverlayFont &font)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (open_textinsert_lib() < 0) {
      result = AM_RESULT_ERR_IO;
      ERROR("Failed to init text insert library.");
      break;
    }

    font_attribute_t font_attr;
    memset(&font_attr, 0, sizeof(font_attribute_t));
    font_attr.size = font.width;
    font_attr.outline_width = font.outline_width;
    font_attr.hori_bold = font.hor_bold;
    font_attr.vert_bold = font.ver_bold;
    font_attr.italic = font.italic;
    snprintf(font_attr.type, sizeof(font_attr.type), "%s",
             font.ttf_name.c_str());
    DEBUG("font type = %s \n", font_attr.type);

    if (text2bitmap_set_font_attribute(&font_attr) < 0) {
      result = AM_RESULT_ERR_IO;
      ERROR("Failed to set up font attribute in text insert library.");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayStrData::open_textinsert_lib()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_font_lib_init) {
      if (text2bitmap_lib_init(nullptr) < 0) {
        result = AM_RESULT_ERR_IO;
        ERROR("Failed to init text insert library.");
        break;
      }
      m_font_lib_init = true;
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayStrData::close_textinsert_lib()
{
  AM_RESULT result = AM_RESULT_OK;
  if (m_font_lib_init) {
    if (text2bitmap_lib_deinit() < 0) {
      result = AM_RESULT_ERR_IO;
    }
    m_font_lib_init = false;
  }
  return result;
}

AM_RESULT AMOverlayStrData::convert_text_to_bmp(AMOverlayTextBox &text,
                                                AMRect &rect)
{
  AM_RESULT result = AM_RESULT_OK;
  wchar_t *wide_str = nullptr;
  do {
    string &str = text.str;
    if (0 == str.size()) {
      str = "Hello, Ambarella!";
    }
    int32_t len = str.size();
    if (len > OVERLAY_STRING_MAX_NUM) {
      WARN("The length [%d] of time string exceeds the max length [%d]."
           " Display %d at most.",
           len, OVERLAY_STRING_MAX_NUM, OVERLAY_STRING_MAX_NUM);
      len = OVERLAY_STRING_MAX_NUM;
      str[len - 1] = '\0';
    }
    DEBUG("String is [%s] \n", str.c_str());
    DEBUG("len = %d \n", len);

    wide_str = (wchar_t *) new wchar_t[len];
    if ((result = char_to_wchar(wide_str, str, len)) != AM_RESULT_OK) {
      break;
    }

    uint8_t *buffer = m_buffer;
    int32_t buf_w_max = rect.size.width;
    int32_t offset_x = 0;
    int32_t offset_y = 0;
    bitmap_info_t bmp_info = { 0 };
    uint8_t *line_head = buffer;
    const AMOverlayFont &font = text.font;
    int32_t font_pitch = font.width;
    int32_t font_height = font.height;
    for (int32_t i = 0; i < len; ++ i) {
      if ((offset_x + font_pitch) > buf_w_max) {
        // Add a new line
        DEBUG("%d: offset_x = %d, font pitch = %d, font area width = %d \n",
              i, offset_x, font_pitch, buf_w_max);
        DEBUG("Add a new line.");
        offset_y += font_height;
        offset_x = 0;
        line_head += buf_w_max * font_height;
      }
      if ((font_height + offset_y) > rect.size.height) {
        // No more new line
        ERROR("No more space for a new line. font height:%u + offset_y:%u > "
            "area height:%u", font_height, offset_y, rect.size.height);
        result = AM_RESULT_ERR_INVALID;
        break;
      }

      // Neither a digit nor a converted letter in time string
      if (text2bitmap_convert_character(wide_str[i],
                                        line_head,
                                        font_height,
                                        buf_w_max,
                                        offset_x,
                                        &bmp_info) < 0) {
        ERROR("text2bitmap library： Failed to convert the charactor[%c].",
              str[i]);
        result = AM_RESULT_ERR_IO;
        break;
      }
      int32_t stride =
          (bmp_info.width + text.spacing) >= 0 ?
              (bmp_info.width + text.spacing) : 0;
      offset_x += stride;
      if (text.spacing < 0) {
        for (int32_t i = 0; i < font_height; ++ i) {
          int32_t offset = i * rect.size.width + offset_x + stride;
          memset(line_head + offset, 0, -text.spacing);
        }
      }
    }
  } while (0);

  delete[] wide_str;
  return result;
}

AM_RESULT AMOverlayStrData::char_to_wchar(wchar_t *wide_str,
                                          const string &str,
                                          int32_t len)
{
  AM_RESULT result = AM_RESULT_OK;
  setlocale(LC_ALL, "");
  if (mbstowcs(wide_str, str.c_str(), len) < 0) {
    result = AM_RESULT_ERR_IO;
    ERROR("Failed to convert char to wchar");
  }

  return result;
}
AM_RESULT AMOverlayStrData::make_text_clut(int32_t size,
                                           uint8_t *buf,
                                           AMOverlayCLUT *clut)
{
  AM_RESULT result = AM_RESULT_OK;
  clut_used_map clut_used;
  index_set_map data;
  do {
    if (!buf || !clut) {
      ERROR("invalid pointer!!!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    /* index 0 is backgroud color, index 1 is outline color, and all
     * others index are font color when returned by convert function,
     * but we just use one color for font, so change all index value
     * which greater than 2 to 2*/
    clut[TEXT_CLUT_ENTRY_FONT].a = 255;
    for (int32_t d = 0; d < size; ++ d) {
      if (buf[d] > TEXT_CLUT_ENTRY_FONT) {
        buf[d] = TEXT_CLUT_ENTRY_FONT;
      }
    }

    //if text background color is full transparent,use area background color
    int32_t i = (clut[TEXT_CLUT_ENTRY_BACKGOURND].a == 0) ? 1 : 0;
    for (; i < 3; ++ i) {
      if ((result = adjust_clut_and_data(clut[i], buf, size, i, data))
          != AM_RESULT_OK) {
        break;
      }
    }
  } while (0);

  data.clear();
  clut_used.clear();
  return result;
}
