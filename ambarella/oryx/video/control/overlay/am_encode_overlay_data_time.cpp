/*******************************************************************************
 * am_encode_overlay_data_time.cpp
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
#include "am_encode_overlay_data_time.h"

#include <sys/time.h>
#include "text_insert.h"

static string g_format_notation = "ap";
AMOverlayData* AMOverlayTimeData::create(AMOverlayArea *area,
                                         AMOverlayAreaData *data)
{
  AMOverlayTimeData *result = new AMOverlayTimeData(area);

  if (AM_UNLIKELY(result && (AM_RESULT_OK != result->add(data)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMOverlayTimeData::destroy()
{
  AMOverlayData::destroy();
}

AMOverlayTimeData::AMOverlayTimeData(AMOverlayArea *area) :
    AMOverlayStrData(area),
    m_digit_size_max(0),
    m_notation_size_max(0)
{
  m_offset.clear();
  for (uint32_t i = 0; i < OVERLAY_DIGIT_NUM; ++ i) {
    m_digit.push_back(nullptr);
  }
  for (uint32_t i = 0; i < g_format_notation.length(); ++ i) {
    m_format_notation.push_back(nullptr);
  }
}

AMOverlayTimeData::~AMOverlayTimeData()
{
  m_offset.clear();
  for (uint32_t i = 0; i < OVERLAY_DIGIT_NUM; ++ i) {
    delete m_digit[i];
    m_digit[i] = nullptr;
  }
  m_digit.clear();
  for (uint32_t i = 0; i < g_format_notation.length(); ++ i) {
    delete m_format_notation[i];
    m_format_notation[i] = nullptr;
  }
  m_format_notation.clear();
}

AM_RESULT AMOverlayTimeData::add(AMOverlayAreaData *data)
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

    AMOverlayTime    &otime = data->time;
    AMOverlayTextBox &text = otime.text;
    if ((result = init_timestamp_info(text, m_clut)) != AM_RESULT_OK) {
      break;
    }

    int32_t buf_size = data->rect.size.width * data->rect.size.height;
    if (!buf_size) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid parameter!");
      break;
    }
    if (!m_buffer) {
      m_buffer = (uint8_t *) new uint8_t[buf_size];
      if (!m_buffer) {
        ERROR("Can't allot memory[%u].", buf_size);
        break;
      }
    }
    memset(m_buffer, CLUT_ENTRY_BACKGOURND, buf_size);

    get_time_string(text.str, otime);
    if ((result = convert_timestamp_to_bmp(text, rect)) != AM_RESULT_OK) {
      break;
    }

    if ((result = make_timestamp_clut(text, buf_size, m_buffer, m_clut))
        != AM_RESULT_OK) {
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

AM_RESULT AMOverlayTimeData::update(AMOverlayAreaData *data)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!data) {
      string time_str;
      uint32_t n = 0;
      uint8_t *src = nullptr;
      AMOverlayTime    &otime = m_param.time;
      AMOverlayTextBox &text = otime.text;
      get_time_string(time_str, otime);
      for (uint32_t i = 0; i < time_str.size(); ++ i) {
        if (time_str[i] != text.str[i]) {
          if (::isdigit(time_str[i])) {
            n = time_str[i] - '0';
            src = (m_digit[n] ? m_digit[n] : nullptr);
          } else if (g_format_notation.find(time_str[i]) != string::npos) {
            n = g_format_notation.find(time_str[i]);
            src = (m_format_notation[n] ? m_format_notation[n] : nullptr);
          } else {
            src = nullptr;
          }
          if (src) {
            AMResolution buf_size;
            AMRect data_rect;
            AMOffset dst_offset;
            buf_size.width = text.font.width;
            buf_size.height = text.font.height;
            data_rect.size.width = m_digit_size_max;
            data_rect.size.height = text.font.height;
            data_rect.offset.x = 0;
            data_rect.offset.y = 0;
            dst_offset.x = m_offset[i].first;
            dst_offset.y = m_offset[i].second;
            result = m_area->update_drv_data(src, buf_size, data_rect,
                                             dst_offset, true);
          } else {
            ERROR("Unknown character [%c] in time string [%s].",
                  time_str[i], time_str.c_str());
            result = AM_RESULT_ERR_INVALID;
            break;
          }
        }
      }
      text.str = time_str;
    } else {
      if (memcmp(&data->time, &m_param.time, sizeof(data->time)) == 0) {
        break;
      }
      data->rect = m_param.rect;
      if ((result = blank()) != AM_RESULT_OK ||
          (result = add(data)) != AM_RESULT_OK) {
        ERROR("Update time overlay failed!\n");
        break;
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMOverlayTimeData::blank()
{
  return AMOverlayData::blank();
}

void AMOverlayTimeData::get_time_string(string &result_str,
                                        const AMOverlayTime &otime)
{
  timeval t;
  if (0 == gettimeofday(&t, nullptr)) {
    tm *p = localtime(&(t.tv_sec));
    string time_form = "";
    string form = "";
    char tmp[128] = {0};
    int32_t hours = p->tm_hour;

    time_form = "%02d:%02d:%02d";
    if (otime.en_msec) {
      char msec_tmp[4] = {0};
      snprintf(msec_tmp, 4, ".%02d", int32_t(t.tv_usec / 1000 / 10));
      time_form.append(msec_tmp);
    }
    if (otime.is_12h) {
      if (hours < 12) {
        time_form.append(" am");
        if (hours == 0) {
          hours = 12;
        }
      } else {
        time_form.append(" pm");
        if (hours > 12) {
          hours -= 12;
        }
      }
    }
    time_form.append(" %s");

    switch (otime.format) {
      case 2:
        form = "%s%02d/%02d/%04d ";
        form.append(time_form.c_str());
        snprintf(tmp, 128, form.c_str(),
                 otime.pre_str.c_str(),
                 p->tm_mday,
                 (1 + p->tm_mon),
                 (1900 + p->tm_year),
                 hours,
                 p->tm_min,
                 p->tm_sec,
                 otime.suf_str.c_str());
        break;
      case 1:
        form = "%s%02d/%02d/%04d ";
        form.append(time_form.c_str());
        snprintf(tmp, 128, form.c_str(),
                 otime.pre_str.c_str(),
                 (1 + p->tm_mon),
                 p->tm_mday,
                 (1900 + p->tm_year),
                 hours,
                 p->tm_min,
                 p->tm_sec,
                 otime.suf_str.c_str());
        break;
      case 0:
      default://all other value set to 0
        form = "%s%04d-%02d-%02d ";
        form.append(time_form.c_str());
        snprintf(tmp, 128, form.c_str(),
                 otime.pre_str.c_str(),
                 (1900 + p->tm_year),
                 (1 + p->tm_mon),
                 p->tm_mday,
                 hours,
                 p->tm_min,
                 p->tm_sec,
                 otime.suf_str.c_str());
       break;
    }

    result_str = tmp;
    DEBUG("current time is [%s] \n", result_str.c_str());
  }
}

AM_RESULT AMOverlayTimeData::init_timestamp_info(AMOverlayTextBox &text,
                                                 AMOverlayCLUT *clut)
{
  AM_RESULT result = AM_RESULT_OK;
  wchar_t *wide_str = nullptr;
  do {
    if (!clut) {
      ERROR("invalid pointer!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if ((result = init_text_info(text, clut)) != AM_RESULT_OK) {
      break;
    }

    int32_t font_pitch = text.font.width;
    int32_t font_height = text.font.height;
    if (!font_pitch || !font_height) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Font pitch [%u] or height [%u] is zero.", font_pitch, font_height);
      break;
    }

    uint32_t &font_width = text.font.width;
    bitmap_info_t bmp_info = { 0 };
    uint32_t digit_size = font_pitch * font_height;
    //transform digit
    for (uint32_t i = 0; i < OVERLAY_DIGIT_NUM; ++ i) {
      delete[] m_digit[i];
      m_digit[i] = (uint8_t *) new uint8_t[digit_size];
      if (!m_digit[i]) {
        result = AM_RESULT_ERR_MEM;
        ERROR("Cannot allot memory for digit %u (size %ux%u).",
              i, font_pitch, font_height);
        break;
      }
      memset(m_digit[i], 0, digit_size);
      if (text2bitmap_convert_character(i + L'0',
                                        m_digit[i],
                                        font_height,
                                        font_pitch,
                                        0,
                                        &bmp_info) < 0) {
        result = AM_RESULT_ERR_IO;
        ERROR("Failed to convert digit %u in text insert library.", i);
        break;
      }
      if (bmp_info.width > m_digit_size_max) {
        m_digit_size_max = bmp_info.width;
      }
    }

    //transform am/pm notation char
    uint32_t len = g_format_notation.length();
    delete[] wide_str;
    wide_str = new wchar_t[len];
    if ((result = char_to_wchar(wide_str, g_format_notation, len)) != AM_RESULT_OK) {
      break;
    }
    for (uint32_t i = 0; i < len; ++ i) {
      delete[] m_format_notation[i];
      m_format_notation[i] = (uint8_t *) new uint8_t[digit_size];
      if (!m_format_notation[i]) {
        result = AM_RESULT_ERR_MEM;
        ERROR("Cannot allot memory for format char %u (size %ux%u).",
              g_format_notation[i], font_pitch, font_height);
        break;
      }
      memset(m_format_notation[i], 0, digit_size);
      if (text2bitmap_convert_character(wide_str[i],
                                        m_format_notation[i],
                                        font_height,
                                        font_pitch,
                                        0,
                                        &bmp_info) < 0) {
        result = AM_RESULT_ERR_IO;
        ERROR("Failed to convert format char [%c] in text insert library.",
              g_format_notation[i]);
        break;
      }
      if (bmp_info.width > m_notation_size_max) {
        m_notation_size_max = bmp_info.width;
      }
    }

    if ((text.spacing + m_digit_size_max < 0) ||
        (text.spacing + m_notation_size_max < 0)) {
      INFO("spacing value:%d is too small, set to %d\n",
             text.spacing, m_digit_size_max > m_notation_size_max ?
                 -m_notation_size_max : -m_digit_size_max);
      text.spacing = m_digit_size_max > m_notation_size_max ?
                 -m_notation_size_max : -m_digit_size_max;
    }

    DEBUG("Font pitch %u, height %u, width %u.",
          font_pitch, font_height, font_width);
  } while (0);

  delete[] wide_str;
  return result;
}

AM_RESULT AMOverlayTimeData::convert_timestamp_to_bmp(AMOverlayTextBox &text,
                                                      AMRect &rect)
{
  AM_RESULT result = AM_RESULT_OK;
  wchar_t *wide_str = nullptr;
  do {
    uint32_t len = text.str.size();
    if (len > OVERLAY_STRING_MAX_NUM) {
      INFO("The length [%d] of time string exceeds the max length [%d]."
           " Display %d at most.",
           len, OVERLAY_STRING_MAX_NUM, OVERLAY_STRING_MAX_NUM);
      len = OVERLAY_STRING_MAX_NUM;
      text.str[len - 1] = '\0';
    }
    wide_str = new wchar_t[len];
    if ((result = char_to_wchar(wide_str, text.str, len)) != AM_RESULT_OK) {
      break;
    }

    int32_t offset_x = 0;
    int32_t offset_y = 0;
    int32_t display_len = 0;
    bitmap_info_t bmp_info = { 0 };
    uint8_t *line_head = m_buffer;
    AMOverlayFont &font = text.font;
    int32_t font_width = font.width;
    int32_t font_pitch = font.width;
    int32_t font_height = font.height;
    for (uint32_t i = 0; i < len; ++ i) {
      if ((offset_x + font_pitch) > rect.size.width) {
        // Add a new line
        DEBUG("Add a new line.");
        offset_y += font_height;
        offset_x = 0;
        line_head += rect.size.width * font_height;
      }
      if ((font_height + offset_y) > rect.size.height) {
        result = AM_RESULT_ERR_MEM;
        // No more new line
        ERROR("No more space for a new line. %u + %u > %u",
              font_height, offset_y, rect.size.height);
        break;
      }
      // Remember the charactor's offset in the overlay data memory
      m_offset.push_back(std::make_pair<uint32_t, uint32_t>(0, 0));
      m_offset[i].first = rect.offset.x + offset_x;
      m_offset[i].second = rect.offset.y + offset_y;

      // Digit and letter in time string
      if (::isdigit(text.str[i])) {
        uint8_t *dst = line_head + offset_x;
        uint8_t *src = m_digit[text.str[i] - '0'];
        if (src) {
          for (int32_t row = 0; row < font_height; ++ row) {
            memcpy(dst, src, font_width);
            dst += rect.size.width;
            src += font_pitch;
          }
          offset_x += (m_digit_size_max + text.spacing) >= 0 ?
              (m_digit_size_max + text.spacing) : 0;
        }
      } else if (g_format_notation.find(text.str[i]) != string::npos) {
        uint8_t *dst = line_head + offset_x;
        uint8_t *src = m_format_notation[g_format_notation.find(text.str[i])];
        if (src) {
          for (int32_t row = 0; row < font_height; ++ row) {
            memcpy(dst, src, font_width);
            dst += rect.size.width;
            src += font_pitch;
          }
          offset_x += (m_notation_size_max + text.spacing) >= 0 ?
              (m_notation_size_max + text.spacing) : 0;
        }
      } else {
        // Neither a digit nor a converted letter in time string
        if (text2bitmap_convert_character(wide_str[i],
                                          line_head,
                                          font_height,
                                          rect.size.width,
                                          offset_x,
                                          &bmp_info) < 0) {
          result = AM_RESULT_ERR_IO;
          ERROR("text2bitmap library： Failed to convert the charactor[%c].",
                text.str[i]);
          break;
        }
        int32_t stride = (bmp_info.width + text.spacing) >= 0 ?
            (bmp_info.width + text.spacing) : 0;
        offset_x += stride;
        if (text.spacing < 0) {
          for (int32_t i = 0; i < font_height; ++ i) {
            int32_t offset = i * rect.size.width + offset_x + stride;
            memset(line_head + offset, 0, -text.spacing);
          }
        }
      }
      display_len ++;
    }
    DEBUG("Time display length %u, time string: [%s] \n",
          display_len, text.str.c_str());
  } while (0);

  delete[] wide_str;
  return result;
}

AM_RESULT AMOverlayTimeData::make_timestamp_clut(const AMOverlayTextBox &text,
                                                 int32_t size,
                                                 uint8_t *buf,
                                                 AMOverlayCLUT *clut)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = make_text_clut(size, buf, clut)) != AM_RESULT_OK) {
      break;
    }

    //adjust local buffer clut index
    int32_t size = text.font.width * text.font.height;
    for (auto &m : m_digit) {
      for (int32_t n = 0; n < size; ++ n) {
        if (m[n] > TEXT_CLUT_ENTRY_FONT) {
          m[n] = TEXT_CLUT_ENTRY_FONT;
        }
      }
      index_set_map data;
      //if text background color is full transparent,use area background color
      int32_t i = (clut[TEXT_CLUT_ENTRY_BACKGOURND].a == 0) ? 1 : 0;
      for (; i < 3; ++ i) {
        if ((result = adjust_clut_and_data(clut[i], m, size, i, data))
            != AM_RESULT_OK) {
          break;
        }
      }
      data.clear();
    }
    for (auto &m : m_format_notation) {
      for (int32_t n = 0; n < size; ++ n) {
        if (m[n] > TEXT_CLUT_ENTRY_FONT) {
          m[n] = TEXT_CLUT_ENTRY_FONT;
        }
      }
      index_set_map data;
      //if text background color is full transparent,use area background color
      int32_t i = (clut[TEXT_CLUT_ENTRY_BACKGOURND].a == 0) ? 1 : 0;
      for (; i < 3; ++ i) {
        if ((result = adjust_clut_and_data(clut[i], m, size, i, data))
            != AM_RESULT_OK) {
          break;
        }
      }
      data.clear();
    }
  } while (0);

  return result;
}

