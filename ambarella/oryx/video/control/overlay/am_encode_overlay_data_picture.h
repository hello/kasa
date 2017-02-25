/*******************************************************************************
 * am_encode_overlay_data_picture.h
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
#ifndef AM_ENCODE_OVERLAY_DATA_PICTURE_H_
#define AM_ENCODE_OVERLAY_DATA_PICTURE_H_
#include "am_encode_overlay_data.h"

struct AMOverlayRGB
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t reserved;
};

struct AMOverlayBmpFileHeader
{
    uint8_t  bfType[2];     // file type
    uint8_t  bfSize[4];     //file size
    uint8_t  bfReserved1[2];
    uint8_t  bfReserved2[2];
    uint8_t  bfOffBits[4];
};

struct AMOverlayBmpInfoHeader
{
    uint32_t biSize;
    uint32_t biWidth;     //bmp width
    uint32_t biHeight;    //bmp height
    uint16_t biPlanes;
    uint16_t biBitCount;  // 1,4,8,16,24 ,32 color attribute
    uint32_t biCompression;
    uint32_t biSizeImage;   //Image size
    uint32_t biXPelsPerMerer;
    uint32_t biYPelsPerMerer;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

class AMOverlayArea;
class AMOverlayPicData: public AMOverlayData
{
  public:
    static AMOverlayData* create(AMOverlayArea *area,
                                 AMOverlayAreaData *data);
    virtual void destroy();

  protected:
    AMOverlayPicData(AMOverlayArea *area);
    virtual ~AMOverlayPicData();

    virtual AM_RESULT add(AMOverlayAreaData *data);
    virtual AM_RESULT update(AMOverlayAreaData *data);

  protected:
    AM_RESULT init_bitmap_info(FILE *fp, int32_t &cn,
                               int32_t &w, int32_t &h);
    AM_RESULT make_bmp_clut(FILE *fp, uint32_t cn, uint32_t size,
                            const AMOverlayColorKey &ck, uint8_t *buf);
    uint32_t  get_proper_value(float value);
    void      rgb_to_yuv(const AMOverlayRGB &src, AMOverlayCLUT &dst);
};

static const int32_t OVERLAY_BMP_MAGIC = 0x4D42;
static const int32_t OVERLAY_BMP_BIT = 8; //just support 8 bit bitmap
static const int32_t OVERLAY_NONE_TRANSPARENT = 0xff;

#endif /* AM_ENCODE_OVERLAY_DATA_PICTURE_H_ */
