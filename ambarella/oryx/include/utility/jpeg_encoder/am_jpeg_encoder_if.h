/*******************************************************************************
 * am_jpeg_encoder_if.h
 *
 * History:
 *   2015-09-15 - [zfgong] created file
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
#ifndef ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_
#define ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_

#include "am_pointer.h"

typedef struct AMYUVData {
  uint8_t *buffer;
  int format;
  int width;
  int height;
  int pitch;
  struct iovec y;
  struct iovec uv;
  uint8_t *y_addr;
  uint8_t *uv_addr;
} AMYUVData;

typedef struct AMJpegData {
  struct iovec data;
  int color_componenets;
  int width;
  int height;
} AMJpegData;

class AMIJpegEncoder;
typedef AMPointer<AMIJpegEncoder> AMIJpegEncoderPtr;

class AMIJpegEncoder
{
    friend class AMPointer<AMIJpegEncoder>;

  public:
    static AMIJpegEncoderPtr get_instance();
    virtual int create() = 0;
    virtual void destroy() = 0;

    virtual AMJpegData *new_jpeg_data(int width, int height) = 0;
    virtual void free_jpeg_data(AMJpegData *jpeg) = 0;
    virtual int encode_yuv(AMYUVData *input, AMJpegData *output) = 0;
    virtual int gen_thumbnail(const char *outfile, const char *infile, int scale) = 0;
    virtual int set_quality(int quality) = 0;
    virtual int get_quality() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIJpegEncoder(){}
};


#endif /* ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_ */
