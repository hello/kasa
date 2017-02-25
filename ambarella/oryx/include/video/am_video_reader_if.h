/**
 * am_video_reader_if.h
 *
 *  History:
 *    Aug 10, 2015 - [Shupeng Ren] created file
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
 */
#ifndef ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

class AMIVideoReader;

typedef AMPointer<AMIVideoReader> AMIVideoReaderPtr;
class AMIVideoReader
{
    friend AMIVideoReaderPtr;

  public:
    static AMIVideoReaderPtr get_instance();

  public:
    virtual AM_RESULT query_video_frame(AMQueryFrameDesc &desc,
                                        uint32_t timeout = -1) = 0;
    virtual AM_RESULT query_yuv_frame(AMQueryFrameDesc &desc,
                                      AM_SOURCE_BUFFER_ID id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_me0_frame(AMQueryFrameDesc &desc,
                                      AM_SOURCE_BUFFER_ID id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_me1_frame(AMQueryFrameDesc &desc,
                                      AM_SOURCE_BUFFER_ID id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_raw_frame(AMQueryFrameDesc &desc) = 0;

    virtual AM_RESULT query_stream_info(AMStreamInfo &info) = 0;

    virtual bool is_gdmacpy_support() = 0;
    virtual AM_RESULT gdmacpy(void *dst, const void *src,
                              size_t width, size_t height, size_t pitch) = 0;
  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoReader(){};
};

#endif /* ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_ */
