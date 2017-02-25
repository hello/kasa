/*******************************************************************************
 * am_encode_overlay_data_time.h
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
#ifndef AM_ENCODE_OVERLAY_DATA_TIME_H_
#define AM_ENCODE_OVERLAY_DATA_TIME_H_

#include "am_encode_overlay_data_string.h"

class AMOverlayTimeData: public AMOverlayStrData
{
  public:
    static AMOverlayData* create(AMOverlayArea *area, AMOverlayAreaData *data);
    virtual void destroy();

  protected:
    AMOverlayTimeData(AMOverlayArea *area);
    virtual ~AMOverlayTimeData();

    virtual AM_RESULT add(AMOverlayAreaData *data);
    virtual AM_RESULT update(AMOverlayAreaData *data);
    virtual AM_RESULT blank();

  private:
    AM_RESULT init_timestamp_info(AMOverlayTextBox &text, AMOverlayCLUT *clut);
    AM_RESULT convert_timestamp_to_bmp(AMOverlayTextBox &text, AMRect &rect);
    AM_RESULT make_timestamp_clut(const AMOverlayTextBox &text, int32_t size,
                                  uint8_t *buf, AMOverlayCLUT *clut);
    void get_time_string(string &result_str, const AMOverlayTime &otime);

  private:
    int32_t          m_digit_size_max;
    int32_t          m_notation_size_max;
    vector<pair<uint32_t, uint32_t>> m_offset; //first is x, second is y
    vector<uint8_t*> m_digit;
    vector<uint8_t*> m_format_notation;
};


static const int32_t OVERLAY_DIGIT_NUM = 10;

#endif /* AM_ENCODE_OVERLAY_DATA_TIME_H_ */
