/*******************************************************************************
 * am_encode_overlay_data_string.h
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
#ifndef AM_ENCODE_OVERLAY_DATA_STRING_H_
#define AM_ENCODE_OVERLAY_DATA_STRING_H_

#include "am_encode_overlay_data.h"

class AMOverlayStrData: public AMOverlayData
{
  public:
    static AMOverlayData* create(AMOverlayArea *area, AMOverlayAreaData *data);
    virtual void destroy();

  protected:
    AMOverlayStrData(AMOverlayArea *area);
    virtual ~AMOverlayStrData();

    virtual AM_RESULT add(AMOverlayAreaData *data);
    virtual AM_RESULT update(AMOverlayAreaData *data);
    virtual AM_RESULT blank();

  protected:
    AM_RESULT init_text_info(AMOverlayTextBox &text, AMOverlayCLUT *clut);
    AM_RESULT convert_text_to_bmp(AMOverlayTextBox &text, AMRect &rect);
    AM_RESULT init_textinsert_lib(const AMOverlayFont &font);
    AM_RESULT make_text_clut(int32_t size, uint8_t *buf, AMOverlayCLUT *clut);
    AM_RESULT char_to_wchar(wchar_t *wide_str, const string &str, int32_t len);
    AM_RESULT find_available_font(string &font);
    static AM_RESULT open_textinsert_lib();
    static AM_RESULT close_textinsert_lib();

  protected:
    static bool  m_font_lib_init;
    AMOverlayCLUT  *m_clut;
};

static const int32_t DEFAULT_OVERLAY_FONT_SIZE          = 24;
static const int32_t DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH = 2;

static const int32_t DEFAULT_OVERLAY_OUTLINE_Y     = 12;
static const int32_t DEFAULT_OVERLAY_OUTLINE_U     = 128;
static const int32_t DEFAULT_OVERLAY_OUTLINE_V     = 128;
static const int32_t DEFAULT_OVERLAY_OUTLINE_ALPHA = 0xff;

static const int32_t TEXT_CLUT_ENTRY_BACKGOURND = CLUT_ENTRY_BACKGOURND;
static const int32_t TEXT_CLUT_ENTRY_OUTLINE    = 1;
static const int32_t TEXT_CLUT_ENTRY_FONT       = 2;

#endif /* AM_ENCODE_OVERLAY_DATA_STRING_H_ */
