/*******************************************************************************
 * am_muxer_jpeg_exif.h
 *
 * History:
 *   2016-6-27 - [smdong] created file
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
#ifndef AM_MUXER_JPEG_EXIF_H_
#define AM_MUXER_JPEG_EXIF_H_

#include "am_base_include.h"
#include <libexif/exif-data.h>

#define JPEG_MARKER_SOI_LENGTH  2
#define FILE_BYTE_ORDER EXIF_BYTE_ORDER_INTEL

class AMMuxerJpegExif
{
  public:
    static AMMuxerJpegExif* create();

    ExifEntry* init_tag(ExifIfd ifd, ExifTag tag);
    ExifEntry* create_tag(ExifIfd ifd, ExifTag tag, size_t len);
    ExifEntry* is_exif_content_has_entry(ExifIfd ifd, ExifTag tag);
    void destroy();
    ExifData* get_exif_data();

  private:
    AMMuxerJpegExif();
    virtual ~AMMuxerJpegExif();
    AM_STATE init();

  private:
    ExifData  *m_exif_data = nullptr;
};


#endif /* AM_MUXER_JPEG_EXIF_H_ */
