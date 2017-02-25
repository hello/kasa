/*******************************************************************************
 * am_encode_overlay_config.h
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
#ifndef AM_ENCODE_OVERLAY_CONFIG_H_
#define AM_ENCODE_OVERLAY_CONFIG_H_

#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_encode_overlay_types.h"
#include "am_pointer.h"

#include <string>
#include <atomic>
#include <vector>
#include <mutex>
#include <map>

using std::string;
using std::vector;
using std::pair;
using std::map;

#define OVERLAY_STRING_MAX_NUM  (256)

struct AMOverlayParams
{
    AM_STREAM_ID                    stream_id;
    int32_t                         num;
    //user must call overlay_max_area_num to get the max area number
    std::vector<AMOverlayAreaParam> area;

    AMOverlayParams():
      stream_id(AM_STREAM_ID_INVALID),
      num(0)
    {}
};

struct AMOverlayConfigDataParam
{
    pair<bool, AM_OVERLAY_DATA_TYPE> type;
    pair<bool, int32_t>  width;
    pair<bool, int32_t>  height;
    pair<bool, int32_t>  offset_x;
    pair<bool, int32_t>  offset_y;
    pair<bool, int32_t>  spacing;
    pair<bool, int32_t>  en_msec;
    pair<bool, int32_t>  format;
    pair<bool, int32_t>  is_12h;
    pair<bool, string>   str;
    pair<bool, string>   pre_str;
    pair<bool, string>   suf_str;
    pair<bool, string>   font_name;
    pair<bool, int32_t>  font_size;
    pair<bool, int32_t>  font_outwidth;
    pair<bool, int32_t>  font_horbold;
    pair<bool, int32_t>  font_verbold;
    pair<bool, int32_t>  font_italic;
    pair<bool, uint32_t> font_color;
    pair<bool, uint32_t> bg_color;
    pair<bool, uint32_t> ol_color;
    pair<bool, string>   bmp;
    pair<bool, int32_t>  bmp_num;
    pair<bool, int32_t>  interval;
    pair<bool, uint32_t> color_key;
    pair<bool, int32_t>  color_range;
    pair<bool, uint32_t> line_color;
    pair<bool, int32_t>  line_tn;
    pair<bool, vector<AMPoint>> line_p;
    AMOverlayConfigDataParam():
      type(false, AM_OVERLAY_DATA_TYPE_NONE),
      width(false, 0),
      height(false, 0),
      offset_x(false, 0),
      offset_y(false, 0),
      spacing(false,0),
      en_msec(false,0),
      format(false,0),
      is_12h(false,0),
      str(false, ""),
      pre_str(false, ""),
      suf_str(false, ""),
      font_name(false, ""),
      font_size(false, 0),
      font_outwidth(false, 0),
      font_horbold(false, 0),
      font_verbold(false, 0),
      font_italic(false, 0),
      font_color(false, 0),
      bg_color(false, 0),
      ol_color(false, 0),
      bmp(false, ""),
      bmp_num(false, 0),
      interval(false, 0),
      color_key(false, 0),
      color_range(false, 0),
      line_color(false, 0),
      line_tn(false, 0)
    {line_p.first = false;}
};

struct AMOverlayConfigAreaParam
{
    pair<bool, int32_t>     state;
    pair<bool, int32_t>     rotate;
    pair<bool, int32_t>     width;
    pair<bool, int32_t>     height;
    pair<bool, int32_t>     offset_x;
    pair<bool, int32_t>     offset_y;
    pair<bool, uint32_t>    bg_color;
    pair<bool, int32_t>     buf_num;

    pair<bool, int32_t>     num;
    vector<AMOverlayConfigDataParam> data;

    AMOverlayConfigAreaParam():
      state(false, 0),
      rotate(false, 0),
      width(false, 0),
      height(false, 0),
      offset_x(false, 0),
      offset_y(false, 0),
      bg_color(false, 0),
      buf_num(false, 0),
      num(false, 0)
    {}
};

struct AMOverlayConfigParam
{
    AMOverlayUserDefLimitVal     limit;
    pair<bool, int32_t>              num;
    vector<AMOverlayConfigAreaParam> area;

    AMOverlayConfigParam():
      num(false, 0)
    {}
};
typedef map<AM_STREAM_ID, AMOverlayConfigParam> AMOverlayParamMap;

class AMOverlayConfig;
typedef AMPointer<AMOverlayConfig> AMOverlayConfigPtr;
class AMOverlayConfig
{
    friend  AMOverlayConfigPtr;

  public:
    static AMOverlayConfigPtr get_instance();

    AM_RESULT get_config(AMOverlayParamMap &config);
    AM_RESULT set_config(const AMOverlayParamMap &config);
    AM_RESULT overwrite_config(const AMOverlayParamMap &config);

  private:
    AM_RESULT load_config();
    AM_RESULT save_config();

    AMOverlayConfig();
    virtual ~AMOverlayConfig();
    void inc_ref();
    void release();

  private:
    static AMOverlayConfig     *m_instance;
    static std::recursive_mutex m_mtx;
    AMOverlayParamMap           m_config;
    std::atomic_int             m_ref_cnt;
};

//256 entries * 4 bytes each entry
static const int32_t OVERLAY_CLUT_SIZE      = 1024;
static const int32_t CLUT_ENTRY_BACKGOURND  = 0;
static const int32_t OVERLAY_WIDTH_ALIGN    = 0x10;
static const int32_t OVERLAY_HEIGHT_ALIGN   = 0x4;
static const int32_t OVERLAY_OFFSET_X_ALIGN = 0x2;
static const int32_t OVERLAY_OFFSET_Y_ALIGN = 0x4;
static const int32_t OVERLAY_PITCH_ALIGN    = 0x20;

static const int32_t DEFAULT_OVERLAY_BACKGROUND_Y = 235;
static const int32_t DEFAULT_OVERLAY_BACKGROUND_U = 128;
static const int32_t DEFAULT_OVERLAY_BACKGROUND_V = 128;
static const int32_t OVERLAY_FULL_TRANSPARENT     = 0x0;
static const int32_t DEFAULT_OVERLAY_BACKGROUND_ALPHA = \
                     OVERLAY_FULL_TRANSPARENT;

#define OL_AUTO_LOCK(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

#endif /* AM_ENCODE_OVERLAY_CONFIG_H_ */
