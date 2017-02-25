/*******************************************************************************
 * frame_buffer.h
 *
 * history:
 *   nov 30, 2015 - [Huaiqing Wang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#ifndef ORYX_APP_AUTO_INTELLIGENT_FRAME_BUFFER_H_
#define ORYX_APP_AUTO_INTELLIGENT_FRAME_BUFFER_H_

#include <linux/fb.h>
#include <atomic>

#include "am_pointer.h"
#include "am_video_types.h"
#include "common.h"

class AMFrameBuffer;
typedef AMPointer<AMFrameBuffer> AMFrameBufferPtr;
class AMFrameBuffer
{
    friend AMFrameBufferPtr;

  public:
    enum AM_FB_COLOR
    {
      FB_COLOR_TRANSPARENT = 0,
      FB_COLOR_SEMI_TRANSPARENT,
      FB_COLOR_RED,
      FB_COLOR_GREEN,
      FB_COLOR_BLUE,
      FB_COLOR_YELLOW,
      FB_COLOR_WHITE,
      FB_COLOR_BLACK,
    };

  public:
    static AMFrameBufferPtr get_instance();
    AM_RESULT draw_clear();

    AM_RESULT draw_dot(point_desc *p, uint32_t size, uint32_t color);
    AM_RESULT draw_line(point_desc p1, point_desc p2, uint32_t color);
    AM_RESULT draw_polyline(point_desc *p, uint32_t size, uint32_t color);
    AM_RESULT draw_rectangle(point_desc p1, point_desc p2, uint32_t color);

    //use specify equation to draw line
    /*equation: x = k*y + b,
     *osd->offset.x <= x <= osd->offset.x+osd->size.width or
     *osd->offset.y <= y <= osd->offset.x+osd->size.height*/
    AM_RESULT draw_straight_line(const vout_osd_desc *osd);
    /*equation: x = k * y2 + b,
     *osd->offset.x <= x <= osd->offset.x+osd->size.width,
     *osd->offset.y <= y <= osd->offset.x+osd->size.height*/
    AM_RESULT draw_curve(const vout_osd_desc *osd);

  private:
    AMFrameBuffer();
    virtual ~AMFrameBuffer();
    void release();
    void inc_ref();
    void check_point(point_desc *p);
    AM_RESULT draw_line_without_render(point_desc &p1,
                                       point_desc &p2,
                                       uint32_t color);
    AM_RESULT init_fb();
    AM_RESULT blank_fb();
    AM_RESULT render_frame();
    void close_fb();

    static AMFrameBuffer *m_instance;
    uint8_t *m_buf;
    int32_t m_fd;
    uint32_t m_buf_len;
    std::atomic_int m_ref_counter;
    fb_fix_screeninfo m_finfo;
    fb_var_screeninfo m_vinfo;
};

#endif /* ORYX_APP_AUTO_INTELLIGENT_FRAME_BUFFER_H_ */
