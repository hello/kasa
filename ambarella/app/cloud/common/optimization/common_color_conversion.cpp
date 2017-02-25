/*******************************************************************************
 * common_color_conversion.cpp
 *
 * History:
 *  2014/10/23 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//bt.601 fullrange
// RGB in full range [0...255]:
//   0.257   0.504   0.098
//  -0.148  -0.291   0.439
//   0.439  -0.368  -0.071
// yuvoffset = [16; 128; 128]
// output range is Y=[16...235];Cb=[16...240];Cr=[16...240]

//bt.601 limited range 1
// RGB limited to [0...219], rgb2yuvT matrix:
//   0.299    0.587    0.114
//   -0.173   -0.339    0.511
//   0.511   -0.428   -0.083
// yuvoffset = [16; 128; 128]
// output range is Y=[16...235];Cb=[16...240];Cr=[16...240]

//bt.601 limited range 2
// RGB limited to [16...235], rgb2yuvT matrix:
//   0.299   0.587   0.114
//   -0.169  -0.331   0.500
//   0.500  -0.419  -0.081
// yuvoffset = [0; 128; 128]
// output range is Y=[16...235];Cb=[16...237.5];Cr=[16...237.5]

//bt709 full range
// RGB in full range [0...255], rgb2yuvT matrix:
//   0.1826   0.6142   0.0620
//   -0.1006  -0.3386   0.4392
//   0.4392  -0.3989  -0.0403
//  yuvoffset = [16; 128; 128]
//  output range is Y=[16...235];Cb=[16...240];Cr=[16...240])

// bt709 limited range
// RGB limited to [16...235], rgb2yuvT matrix:
//   0.2126   0.7152   0.0722
//   -0.1146  -0.3854   0.5000
//   0.5000  -0.4542  -0.0468
//  yuvoffset = [0; 128; 128]
//  output range is Y=[16...235];Cb=[18.5...237.5];Cr=[18.5...237.5])


//
// base function
//   y = 0.1826 * r + 0.6142 * g + 0.0620 * b + 16;
//   u = 128 - 0.1006 * r  -0.3386 * g  + 0.4392 * b;
//   v = 128 + 0.4392 * r  -0.3989 * g  -0.0403 * b;

#define DCSCApproximate 14
#define DCSCApproximateYOffset (16 << DCSCApproximate)
#define DCSCApproximateUVOffset (128 << DCSCApproximate)

//#define D709F00 (0.1826 * (1 << DCSCApproximate))
//#define D709F01 (0.6142 * (1 << DCSCApproximate))
//#define D709F02 (0.0620 * (1 << DCSCApproximate))
//#define D709F10 (-0.1006 * (1 << DCSCApproximate))
//#define D709F11 (-0.3386 * (1 << DCSCApproximate))
//#define D709F12 (0.4392 * (1 << DCSCApproximate))
//#define D709F20 (0.4392 * (1 << DCSCApproximate))
//#define D709F21 (-0.3989 * (1 << DCSCApproximate))
//#define D709F22 (-0.0403 * (1 << DCSCApproximate))

#define D709F00 (2992)
#define D709F01 (10063)
#define D709F02 (1016)
#define D709F10 (-1648)
#define D709F11 (-5548)
#define D709F12 (7196)
#define D709F20 (7196)
#define D709F21 (-6536)
#define D709F22 (-660)

void gfRGBA8888_2_YUV420p_BT709FullRange(TU8 *rgba, TU32 width, TU32 height, TU32 linesize, TU8 *y, TU8 *u, TU8 *v, TU32 linesize_y, TU32 linesize_u, TU32 linesize_v)
{
    TU32 i = 0, j = 0;
    TU8 *y1 = y + linesize_y;
    TU8 *rgba1 = rgba + linesize;
    TS32 value = 0;

    linesize = linesize << 1;
    linesize -= (width << 2);
    linesize_y = linesize_y << 1;
    linesize_y -= width;
    width = (width + 1) >> 1;
    height = (height + 1) >> 1;
    linesize_u -= width;
    linesize_v -= width;

    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            value = D709F00 * rgba[0] + D709F01 * rgba[1] + D709F02 * rgba[2] + DCSCApproximateYOffset;
            *y ++ = (value >> DCSCApproximate) & 0xff;
            value = D709F10 * rgba[0] + D709F11 * rgba[1] + D709F12 * rgba[2] + DCSCApproximateUVOffset;
            *u ++ = (value >> DCSCApproximate) & 0xff;
            value = D709F20 * rgba[0] + D709F21 * rgba[1] + D709F22 * rgba[2] + DCSCApproximateUVOffset;
            *v ++ = (value >> DCSCApproximate) & 0xff;
            rgba += 4;

            value = D709F00 * rgba[0] + D709F01 * rgba[1] + D709F02 * rgba[2] + DCSCApproximateYOffset;
            *y ++ = (value >> DCSCApproximate) & 0xff;
            rgba += 4;

            value = D709F00 * rgba1[0] + D709F01 * rgba1[1] + D709F02 * rgba1[2] + DCSCApproximateYOffset;
            *y1 ++ = (value >> DCSCApproximate) & 0xff;
            rgba1 += 4;

            value = D709F00 * rgba1[0] + D709F01 * rgba1[1] + D709F02 * rgba1[2] + DCSCApproximateYOffset;
            *y1 ++ = (value >> DCSCApproximate) & 0xff;
            rgba1 += 4;
        }

        rgba += linesize;
        rgba1 += linesize;
        y += linesize_y;
        y1 += linesize_y;
        u += linesize_u;
        v += linesize_v;
    }
}

void gfBGRA8888_2_YUV420p_BT709FullRange(TU8 *rgba, TU32 width, TU32 height, TU32 linesize, TU8 *y, TU8 *u, TU8 *v, TU32 linesize_y, TU32 linesize_u, TU32 linesize_v)
{
    TU32 i = 0, j = 0;
    TU8 *y1 = y + linesize_y;
    TU8 *rgba1 = rgba + linesize;
    TS32 value = 0;

    linesize = linesize << 1;
    linesize -= (width << 2);
    linesize_y = linesize_y << 1;
    linesize_y -= width;
    width = (width + 1) >> 1;
    height = (height + 1) >> 1;
    linesize_u -= width;
    linesize_v -= width;

    for (j = 0; j < height; j ++) {
        for (i = 0; i < width; i ++) {
            value = D709F00 * rgba[2] + D709F01 * rgba[1] + D709F02 * rgba[0] + DCSCApproximateYOffset;
            *y ++ = (value >> DCSCApproximate) & 0xff;
            value = D709F10 * rgba[2] + D709F11 * rgba[1] + D709F12 * rgba[0] + DCSCApproximateUVOffset;
            *u ++ = (value >> DCSCApproximate) & 0xff;
            value = D709F20 * rgba[2] + D709F21 * rgba[1] + D709F22 * rgba[0] + DCSCApproximateUVOffset;
            *v ++ = (value >> DCSCApproximate) & 0xff;
            rgba += 4;

            value = D709F00 * rgba[2] + D709F01 * rgba[1] + D709F02 * rgba[0] + DCSCApproximateYOffset;
            *y ++ = (value >> DCSCApproximate) & 0xff;
            rgba += 4;

            value = D709F00 * rgba1[2] + D709F01 * rgba1[1] + D709F02 * rgba1[0] + DCSCApproximateYOffset;
            *y1 ++ = (value >> DCSCApproximate) & 0xff;
            rgba1 += 4;

            value = D709F00 * rgba1[2] + D709F01 * rgba1[1] + D709F02 * rgba1[0] + DCSCApproximateYOffset;
            *y1 ++ = (value >> DCSCApproximate) & 0xff;
            rgba1 += 4;
        }

        rgba += linesize;
        rgba1 += linesize;
        y += linesize_y;
        y1 += linesize_y;
        u += linesize_u;
        v += linesize_v;
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

