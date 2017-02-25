/*******************************************************************************
 * mjpeg.h
 *
 * History:
 *   2014-12-24 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_
#define ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_

#include <vector>

struct AMJpegHeader
{
    uint8_t typeSpecific;
    uint8_t fregOffset3;
    uint8_t fregOffset2;
    uint8_t fregOffset1;
    uint8_t type;
    uint8_t qfactor;
    uint8_t width;
    uint8_t height;
};

struct AMJpegQuantizationHeader
{
    uint8_t mbz;
    uint8_t precision;
    uint8_t length2;
    uint8_t length1;
};

struct AMJpegQtable
{
    uint8_t *data;
    uint32_t len;
    AMJpegQtable() :
      data(nullptr),
      len(0)
    {}
    AMJpegQtable(const AMJpegQtable &qtable) :
      data(qtable.data),
      len(qtable.len)
    {}
};

typedef std::vector<AMJpegQtable> AMJpegQtableList;
struct AMJpegParams
{
    uint8_t         *data;
    uint32_t         len;
    uint32_t         offset;
    uint32_t         q_table_total_len;
    uint16_t         width;
    uint16_t         height;
    uint16_t         precision;
    uint16_t         type;
    AMJpegQtableList q_table_list;
    void clear()
    {
      q_table_list.clear();
      data = nullptr;
      len  = 0;
      offset = 0;
      q_table_total_len = 0;
      width = 0;
      height = 0;
      precision = 0;
      type = 0;
    }
};

#endif /* ORYX_STREAM_INCLUDE_COMMON_MEDIA_MJPEG_H_ */
