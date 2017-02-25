/*******************************************************************************
 * am_encode_overlay_area.h
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
#ifndef AM_ENCODE_OVERLAY_AREA_H_
#define AM_ENCODE_OVERLAY_AREA_H_
#include "am_encode_overlay_config.h"

class AMOverlayData;
typedef vector<AMOverlayData*> AMOverlayDataVec;
class AMOverlayArea
{
  public:
    AMOverlayArea();
    virtual ~AMOverlayArea();

    AM_RESULT init(const AMOverlayAreaAttr &attr,
                   const AMOverlayAreaInsert &overlay_area);
    int32_t   add_data(AMOverlayAreaData &data);
    AM_RESULT update_data(int32_t index, AMOverlayAreaData *data);
    AM_RESULT delete_data(int32_t index);

    const AMOverlayAreaParam& get_param();
    const AMOverlayAreaInsert& get_drv_param();
    void set_state(uint16_t state);
    void get_data_param(int32_t index, AMOverlayAreaData &param);
    int32_t get_state();

    //below apis used for AMOverlayData class to call
    AMOverlayCLUT *get_clut();
    int32_t &get_clut_color_num();
    AM_RESULT update_drv_data(uint8_t *buf, AMResolution buf_size, AMRect data,
                              AMOffset dst_offset, bool all_flag);

  private:
    void      switch_buffer();
    int32_t   get_available_index();
    AM_RESULT adjust_area_param();
    AM_RESULT alloc_area_mem();
    AM_RESULT rotate_fill(uint8_t *dst,
                          uint32_t dst_pitch,
                          uint32_t dst_width,
                          uint32_t dst_height,
                          uint32_t dst_x,
                          uint32_t dst_y,
                          uint8_t *src,
                          uint32_t src_pitch,
                          uint32_t src_height,
                          uint32_t src_x,
                          uint32_t src_y,
                          uint32_t data_width,
                          uint32_t data_height);

  private:
    static std::recursive_mutex  m_mtx;
    AMOverlayCLUT               *m_clut;
    uint8_t                     *m_buffer; //local buffer for manipulate data
    int32_t                      m_color_num;
    int32_t                      m_cur_buf;
    int32_t                      m_flip_rotate; //area flip and rotate flag
    AMOverlayAreaParam           m_param;
    AMOverlayAreaInsert          m_drv_param;
    AMOverlayDataVec             m_data;
};

#endif /* AM_ENCODE_OVERLAY_AREA_H_ */
