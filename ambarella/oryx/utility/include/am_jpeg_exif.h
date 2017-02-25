/*******************************************************************************
 * am_jpeg_exif.h
 *
 * History:
 *   2016-07-25 - [smdong] created file
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
#ifndef AM_JPEG_EXIF_H_
#define AM_JPEG_EXIF_H_

#include <atomic>
#include "am_mutex.h"
#include "am_jpeg_exif_if.h"

#define MAX_FILE_NAME_LENGTH 128
#define DEFAULT_THUMBNAIL_WIDTH 160
#define DEFAULT_THUMBNAIL_HEIGHT 120
#define MAX_SCALE_NUMBER 8
#define COLOR_COMPONENTS 3

class AMJpegExif: public AMIJpegExif
{
    friend class AMIJpegExif;

  public:
    static AMIJpegExif* get_instance();
    virtual bool combine_jpeg_exif(const char *jpeg_file,
                                   const char *exif_file,
                                   const char *output_file,
                                   uint32_t thumbnail_jpeg_qual);

  private:
    bool read_file(const char *file_name, uint8_t *&file_data,
                   uint32_t &file_len);
    bool write_file(const char *file_name, uint8_t *file_data,
                    uint32_t file_len);

  private:
    AMJpegExif();
    virtual ~AMJpegExif();
    virtual void inc_ref();
    virtual void release();

  private:
    std::atomic_int    m_ref_count;
    static AMJpegExif *m_instance;
};


#endif  /* AM_JPEG_EXIF_H_ */
