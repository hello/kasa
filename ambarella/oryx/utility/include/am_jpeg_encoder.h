/*******************************************************************************
 * am_jpeg_encoder.h
 *
 * History:
 *   2015-07-01 - [Zhifeng Gong] created file
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

#ifndef AM_JPEG_ENCODER_H_
#define AM_JPEG_ENCODER_H_

#include <atomic>
#include <jpeglib.h>
#include "am_thread.h"
#include "am_mutex.h"
#include "am_event.h"
#include "am_jpeg_encoder_if.h"

class AMJpegEncoder: public AMIJpegEncoder
{
    friend class AMIJpegEncoder;

  public:
    virtual int create();
    virtual void destroy();
    virtual AMJpegData *new_jpeg_data(int width, int height);
    virtual void free_jpeg_data(AMJpegData *jpeg);
    virtual int encode_yuv(AMYUVData *input, AMJpegData *output);
    virtual int gen_thumbnail(const char *outfile, const char *infile, int scale);
    virtual int set_quality(int quality);
    virtual int get_quality();

  protected:
    std::atomic_int m_ref_counter;
    static AMJpegEncoder *get_instance();
    virtual void inc_ref();
    virtual void release();

  private:
    AMJpegEncoder();
    virtual ~AMJpegEncoder();
    AMJpegEncoder(AMJpegEncoder const &copy) = delete;
    AMJpegEncoder& operator=(AMJpegEncoder const &copy) = delete;

  private:
    int m_quality;
    struct jpeg_compress_struct m_info;
    struct jpeg_error_mgr m_err;
    static AMJpegEncoder *m_instance;
};

#endif  /*AM_JPEG_ENCODER_H_ */
