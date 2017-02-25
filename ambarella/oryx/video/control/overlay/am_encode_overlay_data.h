/*******************************************************************************
 * am_encode_overlay_data.h
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
#ifndef AM_ENCODE_OVERLAY_DATA_H_
#define AM_ENCODE_OVERLAY_DATA_H_
#include "am_encode_overlay_config.h"

typedef std::map<int32_t, std::vector<uint32_t>> index_set_map;
typedef std::map<int32_t, bool> clut_used_map;

class AMOverlayArea;
class AMOverlayData
{
    friend class AMOverlayArea;
  public:
    virtual void destroy();

  protected:
    AMOverlayData(AMOverlayArea *area);
    virtual ~AMOverlayData();
    const AMOverlayAreaData &get_param();

    virtual AM_RESULT add(AMOverlayAreaData *data) = 0;
    virtual AM_RESULT update(AMOverlayAreaData *data) = 0;
    virtual AM_RESULT blank();

  protected:
    AM_RESULT adjust_clut_and_data(const AMOverlayCLUT &clut, uint8_t *buf,
                                   int32_t size, int32_t cur_index,
                                   index_set_map &backup);
    AM_RESULT check_block_rect_param(int32_t w, int32_t h,
                                     int32_t x, int32_t y);

  protected:
    AMOverlayArea    *m_area;
    uint8_t          *m_buffer;
    AMOverlayAreaData m_param;
};

#endif /* AM_ENCODE_OVERLAY_DATA_H_ */
