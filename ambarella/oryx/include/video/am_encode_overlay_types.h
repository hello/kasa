/*******************************************************************************
 * am_encode_overlay_types.h
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
#ifndef AM_ENCODE_OVERLAY_TYPES_H_
#define AM_ENCODE_OVERLAY_TYPES_H_

#include "am_video_types.h"
#include <map>
#include <vector>

using std::string;
using std::vector;
using std::pair;

enum AM_OVERLAY_STATE
{
  AM_OVERLAY_ENABLE   = 0,
  AM_OVERLAY_DISABLE,
  AM_OVERLAY_DELETE,
};

enum AM_OVERLAY_DATA_TYPE
{
  AM_OVERLAY_DATA_TYPE_NONE = -1,
  AM_OVERLAY_DATA_TYPE_STRING = 0, //string type
  AM_OVERLAY_DATA_TYPE_PICTURE, //bmp type
  AM_OVERLAY_DATA_TYPE_TIME, //timestamp type
  AM_OVERLAY_DATA_TYPE_ANIMATION, //animation type
  AM_OVERLAY_DATA_TYPE_LINE, //line type
};

enum AM_OVERLAY_COLOR
{
  AM_OVERLAY_COLOR_WHITE  = 0,
  AM_OVERLAY_COLOR_BLACK,
  AM_OVERLAY_COLOR_RED,
  AM_OVERLAY_COLOR_BLUE,
  AM_OVERLAY_COLOR_GREEN,
  AM_OVERLAY_COLOR_YELLOW,
  AM_OVERLAY_COLOR_CYAN,
  AM_OVERLAY_COLOR_MAGENTA,
  AM_OVERLAY_COLOR_CUSTOM,

  AM_OVERLAY_COLOR_NUM = AM_OVERLAY_COLOR_CUSTOM,
};

//yuv color look up table
struct AMOverlayCLUT
{
    uint8_t v;
    uint8_t u;
    uint8_t y;
    uint8_t a;

    AMOverlayCLUT():
      v(0),
      u(0),
      y(0),
      a(0)
    {}

    AMOverlayCLUT(uint8_t v, uint8_t u, uint8_t y, uint8_t a):
      v(v),
      u(u),
      y(y),
      a(a)
    {}
};

struct AMOverlayFont
{
    uint32_t      width;
    uint32_t      height;
    uint32_t      outline_width;
    int32_t       ver_bold;
    int32_t       hor_bold;
    uint32_t      italic;
    uint32_t      disable_anti_alias;
    string        ttf_name;

    AMOverlayFont(string str=""):
      width(0),
      height(0),
      outline_width(0),
      ver_bold(0),
      hor_bold(0),
      italic(0),
      disable_anti_alias(0),
      ttf_name(str)
    {}
};

//color used to set font color when osd type is text
struct AMOverlayColor
{
    //0~7: predefine color: 0(white), 1(black), 2(red),
    //3(blue), 4(green), 5(yellow), 6(cyan), 7(magenta);
    //8: custom color set by color value
    uint32_t  id;
    AMOverlayCLUT color; //user custom color

    AMOverlayColor():
      id(0)
    {}
};

//Text type parameter
struct AMOverlayTextBox
{
    int32_t         spacing;
    string          str;
    AMOverlayFont   font;
    AMOverlayColor  font_color;
    AMOverlayCLUT   outline_color;
    AMOverlayCLUT   background_color;

    AMOverlayTextBox():
      spacing(0)
    {}
};

//Time type parameter
struct AMOverlayTime
{
    AMOverlayTextBox text;
    //prefix string add for timestamp
    string           pre_str;
    //suffix string add for timestamp
    string           suf_str;
    //whether enable msec display for time type
    int32_t          en_msec;
    int32_t          format;
    int32_t          is_12h;

    AMOverlayTime():
      en_msec(0),
      format(0),
      is_12h(0)
    {}
};

//overlay area attribute parameter
struct AMOverlayAreaAttr
{
    int16_t                   enable;
    /* if set to 0, osd area will not auto flip or rotate
      when encode stream is flip or rotate state*/
    int32_t                   rotate;
    /* buffer number for each area, annimation or
      frequently update manipulate may use double buffer*/
    int32_t                   buf_num;
    //area size and offset in stream
    AMRect                    rect;
    //color in full area as background
    AMOverlayCLUT             bg_color;

    AMOverlayAreaAttr():
      enable(0),
      rotate(1),
      buf_num(1)
    {}
};

//color used to transparent when osd type is picture
struct AMOverlayColorKey
{
    //when color value is in [color-range,
    //color+range], it will do transparent
    uint8_t   range;
    AMOverlayCLUT color;

    AMOverlayColorKey():
      range(0)
    {}
};

//Picture type parameter
struct AMOverlayPicture
{
    string            filename;
    //color which user want to transparent in osd
    AMOverlayColorKey colorkey;
};

//Animation type parameter
struct AMOverlayAnimation
{
    AMOverlayPicture  pic;
    //sub-picture number in this picture
    int32_t           num;
    //picture update frame interval
    int32_t           interval;

    AMOverlayAnimation() :
      num(0),
      interval(0)
    {}
};

struct AMOverlayLine
{
    vector<AMPoint>   point;
    AMOverlayColor    color;
    int32_t           thickness;

    AMOverlayLine() :
      thickness(0)
    {}
};

//overlay area data parameter
struct AMOverlayAreaData
{
    AM_OVERLAY_DATA_TYPE type;
    AMOverlayTextBox     text;//used for string type
    AMOverlayTime        time;//used for time type
    AMOverlayPicture     pic; //used for picture type
    AMOverlayAnimation   anim;//used for animation type
    AMOverlayLine        line;//used for line
    //data block size and offset in area
    AMRect               rect;

    AMOverlayAreaData():
      type(AM_OVERLAY_DATA_TYPE_NONE)
    {}
};

struct AMOverlayAreaParam
{
    int32_t                   num;
    AMOverlayAreaAttr         attr;
    vector<AMOverlayAreaData> data;

    AMOverlayAreaParam():
      num(0)
    {}
};

struct AMOverlayUserDefLimitVal
{
    pair<bool, int32_t> s_num_max;
    pair<bool, int32_t> a_num_max;

    AMOverlayUserDefLimitVal():
      s_num_max(false, 0),
      a_num_max(false, 0)
    {}
};

#endif /* AM_ENCODE_OVERLAY_TYPES_H_ */
