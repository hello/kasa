/*******************************************************************************
 * common_customized_codec.cpp
 *
 * History:
 *  2014/02/19 - [Zhi He] create file
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
#include "common_base.h"

#include "common_customized_codec.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#if 0

static const TU8 gCustomized8BitsQuantTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
    8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static const TU8 gCustomized8BitsDeQuantTable[16] = {
    16, 48, 72, 88, 104, 116, 122, 126, 130, 134, 140, 152, 168, 184, 208, 240
};
#define DROUND_VALUE 0x1000
#else

static const TU8 gCustomized8BitsQuantTable[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7,
    8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

static const TU8 gCustomized8BitsDeQuantTable[16] = {
    24, 65, 90, 106, 117, 122, 125, 127, 128, 130, 133, 138, 149, 165, 190, 231
};
#define DSIGNED_OFFSET_VALUE 0x1000
#define DROUND_VALUE 0x0080

static const TS16 gCustomized8BitsDeQuantTable_Shift[16] = {
    ((24 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((65 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((90 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((106 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((117 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((122 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((125 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((127 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((128 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((130 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((133 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((138 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((149 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((165 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((190 << 8) >> 3) - DSIGNED_OFFSET_VALUE,
    ((231 << 8) >> 3) - DSIGNED_OFFSET_VALUE
};

#endif

static void _adpcm_quant_merge_2_samples(TU16 &ref_value, TU16 value1, TU16 value2, TU8 &ret)
{
    TU8 quant_value1 = 0, quant_value2 = 0;

    //LOG_PRINTF("gCustomized8BitsQuantTable %02x, %02x, %02x, %02x\n", gCustomized8BitsQuantTable[0], gCustomized8BitsQuantTable[32], gCustomized8BitsQuantTable[64], gCustomized8BitsQuantTable[240]);
    //LOG_PRINTF("gCustomized8BitsDeQuantTable %02x, %02x, %02x, %02x\n", gCustomized8BitsDeQuantTable[0], gCustomized8BitsDeQuantTable[1], gCustomized8BitsDeQuantTable[2], gCustomized8BitsDeQuantTable[3]);

    //LOG_PRINTF("[stage 1]: value1 %04x, ref_value %04x\n", value1, ref_value);
    value1 = value1 - ref_value + DSIGNED_OFFSET_VALUE + DROUND_VALUE;
    //LOG_PRINTF("[stage 2]: value1 %04x, ref_value %04x\n", value1, ref_value);

    // -0x0800 ~ 0x0800 --> 0x0 ~ 0x1000
    if (DUnlikely(value1 > 0x7FFF)) {
        value1 = 0;
    } else if (DUnlikely(value1 > 0x1FFF)) {
        value1 = 0x1FFF;
    }
    //LOG_PRINTF("[stage 3]: value1 %04x, ref_value %04x\n", value1, ref_value);

    quant_value1 = gCustomized8BitsQuantTable[value1 >> 5];
    //LOG_PRINTF("[stage 4]: quant_value1 %02x\n", quant_value1);

    ref_value += gCustomized8BitsDeQuantTable[quant_value1] << 5;
    //LOG_PRINTF("[stage 4.5]: ref_value %04x\n", ref_value);
    ref_value -= DSIGNED_OFFSET_VALUE;
    //LOG_PRINTF("[stage 5]: ref_value %04x\n", ref_value);


    //LOG_PRINTF("[stage 6]: value2 %04x, ref_value %04x\n", value2, ref_value);
    value2 = value2 - ref_value + DSIGNED_OFFSET_VALUE + DROUND_VALUE;
    //LOG_PRINTF("[stage 7]: value2 %04x, ref_value %04x\n", value2, ref_value);

    // -0x0800 ~ 0x0800 --> 0x0 ~ 0x1000
    if (DUnlikely(value2 > 0x7FFF)) {
        value2 = 0;
    } else if (DUnlikely(value2 > 0x1FFF)) {
        value2 = 0x1FFF;
    }
    //LOG_PRINTF("[stage 8]: value2 %04x, ref_value %04x\n", value2, ref_value);

    quant_value2 = gCustomized8BitsQuantTable[value2 >> 5];
    //LOG_PRINTF("[stage 9]: quant_value1 %02x, quant_value2 %02x\n",quant_value1, quant_value2);

    ret = (quant_value1 | (quant_value2 << 4));
    //LOG_PRINTF("[stage 10]: ret %02x\n", ret);

    ref_value += gCustomized8BitsDeQuantTable[quant_value2] << 5;
    //LOG_PRINTF("[stage 10.5]: ref_value %04x\n", ref_value);
    ref_value -= DSIGNED_OFFSET_VALUE;
    //LOG_PRINTF("[stage 11]: ref_value %04x\n", ref_value);

}

#if 0
static void _adpcm_dequant_2_samples(TU16 &ref_value, TU16 &rec_value1, TU16 &rec_value2, TU8 input)
{
    //LOG_PRINTF("[decode input]: ref_value 0x%04x, input %02x\n", ref_value, input);

    rec_value1 = (input & 0x0F);
    rec_value1 = ((((TS16)gCustomized8BitsDeQuantTable[rec_value1]) << 8) >> 3);
    rec_value1 += ref_value;
    rec_value1 -= DROUND_VALUE;
    //LOG_PRINTF("[output 1]: rec_value1 %04x\n", rec_value1);

    rec_value2 = input >> 4;
    rec_value2 = ((((TS16)gCustomized8BitsDeQuantTable[rec_value2]) << 8) >> 3);
    rec_value2 += rec_value1;
    rec_value2 -= DROUND_VALUE;

    ref_value = rec_value2;
    //LOG_PRINTF("[output 2]: rec_value2 %04x\n", rec_value2);
}
#else
static void _adpcm_dequant_2_samples(TU16 &ref_value, TU16 &rec_value1, TU16 &rec_value2, TU8 input)
{
    //LOG_PRINTF("[decode input]: ref_value 0x%04x, input %02x\n", ref_value, input);

    rec_value1 = gCustomized8BitsDeQuantTable_Shift[input & 0x0F] + ref_value;
    ref_value = rec_value2 = gCustomized8BitsDeQuantTable_Shift[input >> 4] + rec_value1;
    //LOG_PRINTF("[output 1]: rec_value1 %04x\n", rec_value1);
    //LOG_PRINTF("[output 2]: rec_value2 %04x\n", rec_value2);
}
#endif

#if 0
static void _5_tag_loop_filter_1(TS16 *p, TMemSize size)
{
    TS32 v0, v1, v2, v3, v4;

    if ((!p) || (size < 3)) {
        LOG_FATAL("BAD param %p, %ld\n", p, size);
        return;
    }

    v0 = v1 = v2 = p[0];
    v3 = p[1];
    v4 = p[2];

    size -= 2;

    while (1) {
        p[0] = ((v0 + v4) + ((v1 + v3) << 1) + (v2 * 26)) >> 5;

        v0 = v1;
        v1 = v2;
        v2 = v3;
        v3 = v4;
        size --;

        if (DUnlikely(size == 0)) {
            break;
        }
        v4 = p[3];
        p ++;
    }

    p[0] = ((v0 + v4) + ((v1 + v3) << 1) + (v2 * 26)) >> 5;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;

    p[0] = ((v0 + v4) + ((v1 + v3) << 1) + (v2 * 26)) >> 5;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;
}

static void _5_tag_loop_filter_2(TS16 *p, TMemSize size)
{
    TS32 v0, v1, v2, v3, v4;

    if ((!p) || (size < 3)) {
        LOG_FATAL("BAD param %p, %ld\n", p, size);
        return;
    }

    v0 = v1 = v2 = p[0];
    v3 = p[1];
    v4 = p[2];

    size -= 2;

    while (1) {
        p[0] = (((v1 + v3) << 1) + (v2 * 6) - (v0 + v4)) >> 3;

        v0 = v1;
        v1 = v2;
        v2 = v3;
        v3 = v4;
        size --;

        if (DUnlikely(size == 0)) {
            break;
        }
        v4 = p[3];
        p ++;
    }

    p[0] = (((v1 + v3) << 1) + (v2 * 6) - (v0 + v4)) >> 3;


    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;

    p[0] = (((v1 + v3) << 1) + (v2 * 6) - (v0 + v4)) >> 3;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;
}

static void _5_tag_loop_filter_3(TS16 *p, TMemSize size)
{
    TS32 v0, v1, v2, v3, v4;

    if ((!p) || (size < 3)) {
        LOG_FATAL("BAD param %p, %ld\n", p, size);
        return;
    }

    v0 = v1 = v2 = p[0];
    v3 = p[1];
    v4 = p[2];

    size -= 2;

    while (1) {
        p[0] = (((v1 + v3 + v2) << 1) + (v0 + v4)) >> 3;

        v0 = v1;
        v1 = v2;
        v2 = v3;
        v3 = v4;
        size --;

        if (DUnlikely(size == 0)) {
            break;
        }
        v4 = p[3];
        p ++;
    }

    p[0] = (((v1 + v3 + v2) << 1) + (v0 + v4)) >> 3;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;

    p[0] = (((v1 + v3 + v2) << 1) + (v0 + v4)) >> 3;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;
}

static void _5_tag_loop_filter_4(TS16 *p, TMemSize size)
{
    TS32 v0, v1, v2, v3, v4;

    if ((!p) || (size < 3)) {
        LOG_FATAL("BAD param %p, %ld\n", p, size);
        return;
    }

    v0 = v1 = v2 = p[0];
    v3 = p[1];
    v4 = p[2];

    size -= 2;

    while (1) {
        p[0] = (((v1 + v3) << 2) + (v0 + v4) + (v2 * 6)) >> 4;

        v0 = v1;
        v1 = v2;
        v2 = v3;
        v3 = v4;
        size --;

        if (DUnlikely(size == 0)) {
            break;
        }
        v4 = p[3];
        p ++;
    }

    p[0] = (((v1 + v3) << 2) + (v0 + v4) + (v2 * 6)) >> 4;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;

    p[0] = (((v1 + v3) << 2) + (v0 + v4) + (v2 * 6)) >> 4;

    v0 = v1;
    v1 = v2;
    v2 = v3;
    v3 = v4;
    p ++;
}
#endif

static const TU8 gCustomized8BitsQuantTable_v1[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 6, 6,
    6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 9, 9, 9,
    9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13,
    14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19,
    19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24,
    24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 30, 31,
    32, 33, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 38, 39,
    39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 42, 43, 43, 43, 44, 44,
    44, 45, 45, 45, 46, 46, 46, 47, 47, 47, 48, 48, 48, 49, 49, 49,
    50, 50, 50, 51, 51, 51, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54,
    54, 54, 54, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 57, 57, 57,
    57, 57, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 59,
    60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 62, 62,
    62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63, 63
};

static const TU8 gCustomized8BitsDeQuantTable_v1[64] = {
    4, 13, 21, 28, 35, 42, 48, 53, 58, 63, 68, 72, 75, 78, 81, 84,
    87, 90, 93, 96, 99, 102, 105, 108, 111, 114, 117, 120, 123, 125, 126, 127,
    128, 129, 130, 132, 135, 138, 141, 144, 147, 150, 153, 156, 159, 162, 165, 168,
    171, 174, 177, 180, 183, 187, 192, 197, 202, 207, 213, 220, 227, 234, 242, 251
};
#define DROUND_VALUE_v1 0x0008
#define DSIGNED_OFFSET_v1 0x0800

static const TS16 gCustomized8BitsOffsetTable_v1[4] = {
    0, 4, -4, 8
};

static void _adpcm_quant_merge_2_samples_v1(TU16 &ref_value, TU16 value1, TU16 value2, TU8 &ret)
{
    TU8 quant_value1 = 0;
    TS16 offset = value2 - value1;

    //LOG_PRINTF("gCustomized8BitsQuantTable %02x, %02x, %02x, %02x\n", gCustomized8BitsQuantTable[0], gCustomized8BitsQuantTable[32], gCustomized8BitsQuantTable[64], gCustomized8BitsQuantTable[240]);
    //LOG_PRINTF("gCustomized8BitsDeQuantTable %02x, %02x, %02x, %02x\n", gCustomized8BitsDeQuantTable[0], gCustomized8BitsDeQuantTable[1], gCustomized8BitsDeQuantTable[2], gCustomized8BitsDeQuantTable[3]);

    //LOG_PRINTF("[stage 1]: value1 %04x, ref_value %04x\n", value1, ref_value);
    value1 = value1 - ref_value + DSIGNED_OFFSET_v1 + DROUND_VALUE_v1;
    //LOG_PRINTF("[stage 2]: value1 %04x, ref_value %04x\n", value1, ref_value);

    // -0x0800 ~ 0x0800 --> 0x0 ~ 0x1000
    if (DUnlikely(value1 > 0xFFF)) {
        value1 = 0;
    } else if (DUnlikely(value1 > 0xFFF)) {
        value1 = 0xFFF;
    }
    //LOG_PRINTF("[stage 3]: value1 %04x, ref_value %04x\n", value1, ref_value);

    quant_value1 = gCustomized8BitsQuantTable_v1[value1 >> 4];
    //LOG_PRINTF("[stage 4]: quant_value1 %02x\n", quant_value1);

    ref_value += gCustomized8BitsDeQuantTable_v1[quant_value1] << 4;
    //LOG_PRINTF("[stage 4.5]: ref_value %04x\n", ref_value);
    ref_value -= DSIGNED_OFFSET_v1;
    //LOG_PRINTF("[stage 5]: ref_value %04x\n", ref_value);

    if ((-10) > offset) {
        ret = quant_value1 | 0xc0; // -8
    } else if ((6) < offset) {
        ret = quant_value1 | 0x80; // 4
    } else if ((-2) < offset) {
        ret = quant_value1; // 0
    } else {
        ret = quant_value1 | 0x40; // -4
    }

}

static const TS16 gCustomized8BitsDeQuantTable_Shift_v1[64] = {
    ((4 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((13 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((21 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((28 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((35 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((42 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((48 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((53 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((58 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((63 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((68 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((72 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((75 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((78 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((81 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((84 << 8) >> 4) - DSIGNED_OFFSET_v1,

    ((87 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((90 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((93 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((96 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((99 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((102 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((105 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((108 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((111 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((114 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((117 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((120 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((123 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((125 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((126 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((127 << 8) >> 4) - DSIGNED_OFFSET_v1,

    ((128 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((129 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((130 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((132 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((135 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((138 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((141 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((144 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((147 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((150 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((153 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((156 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((159 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((162 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((165 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((168 << 8) >> 4) - DSIGNED_OFFSET_v1,

    ((171 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((174 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((177 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((180 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((183 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((187 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((192 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((197 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((202 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((207 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((213 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((220 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((227 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((234 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((242 << 8) >> 4) - DSIGNED_OFFSET_v1,
    ((251 << 8) >> 4) - DSIGNED_OFFSET_v1
};

#if 0
static void _adpcm_dequant_2_samples_v1(TU16 &ref_value, TU16 &rec_value1, TU16 &rec_value2, TU8 input)
{
    //LOG_PRINTF("[decode input]: ref_value 0x%04x, input %02x\n", ref_value, input);

    rec_value1 = (input & 0x3F);
    rec_value1 = ((((TS16)gCustomized8BitsDeQuantTable_v1[rec_value1]) << 8) >> 4);
    rec_value1 += ref_value;
    rec_value1 -= DSIGNED_OFFSET_v1;
    //LOG_PRINTF("[output 1]: rec_value1 %04x\n", rec_value1);

    rec_value2 = rec_value1 + gCustomized8BitsOffsetTable_v1[input >> 6];

    ref_value = rec_value1;
    //LOG_PRINTF("[output 2]: rec_value2 %04x\n", rec_value2);
}
#else
static void _adpcm_dequant_2_samples_v1(TU16 &ref_value, TU16 &rec_value1, TU16 &rec_value2, TU8 input)
{
    //LOG_PRINTF("[decode input]: ref_value 0x%04x, input %02x\n", ref_value, input);

    ref_value = rec_value1 = gCustomized8BitsDeQuantTable_Shift_v1[input & 0x3F] + ref_value;
    rec_value2 = rec_value1 + gCustomized8BitsOffsetTable_v1[input >> 6];
    //LOG_PRINTF("[output 1]: rec_value1 %04x\n", rec_value1);
    //LOG_PRINTF("[output 2]: rec_value2 %04x\n", rec_value2);
}
#endif

ICustomizedCodec *gfCreateCustomizedADPCMCodec(TUint index)
{
    return CCustomizedADPCMCodec::Create(index);
}

ICustomizedCodec *CCustomizedADPCMCodec::Create(TUint index)
{
    CCustomizedADPCMCodec *result = new CCustomizedADPCMCodec(index);
    if ((result) && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }

    return result;
}

CCustomizedADPCMCodec::CCustomizedADPCMCodec(TUint index)
    : mEncoderInputBufferSize(2 * 1024)
    , mEncoderOutputBufferSize(516)
    , mAdBitDepth(4)
    , mSkipBits(2)
    , mMergeFactor(0)
    , mQuantParam(3)
    , mQuantPattern(0)
    , mIndex(index)
    , mbModeVersion(1)
{
    mHeader.ad_bit_depth = mAdBitDepth;
    mHeader.skip_bits = mSkipBits;
    mHeader.merge_factor = mMergeFactor;
    mHeader.quant_param = mQuantParam;
    mHeader.quant_pattern = mQuantPattern;
}

CCustomizedADPCMCodec::~CCustomizedADPCMCodec()
{
}

EECode CCustomizedADPCMCodec::Construct()
{
    return EECode_OK;
}

EECode CCustomizedADPCMCodec::ConfigCodec(TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5)
{
    mAdBitDepth = param1;
    mSkipBits = param2;
    mMergeFactor = param3;
    mQuantParam = param4;
    mQuantPattern = param5;

    mHeader.ad_bit_depth = mAdBitDepth;
    mHeader.skip_bits = mSkipBits;
    mHeader.merge_factor = mMergeFactor;
    mHeader.quant_param = mQuantParam;
    mHeader.quant_pattern = mQuantPattern;

    return EECode_OK;
}

EECode CCustomizedADPCMCodec::QueryInOutBufferSize(TMemSize &encoder_input_buffer_size, TMemSize &encoder_output_buffer_size) const
{
    encoder_input_buffer_size = mEncoderInputBufferSize;
    encoder_output_buffer_size = mEncoderOutputBufferSize;

    return EECode_OK;
}

EECode CCustomizedADPCMCodec::Encoding(void *in_buffer, void *out_buffer, TMemSize in_data_size, TMemSize &out_data_size)
{
    TU16 *p_in = (TU16 *) in_buffer;
    TU8 *p_out = (TU8 *) out_buffer;
    TU16 ref_value = 0;

    if (DUnlikely(!in_buffer || !out_buffer || (!in_data_size))) {
        LOG_FATAL("NULL in_buffer %p or out_buffer %p, or in_data_size %ld\n", in_buffer, out_buffer, in_data_size);
        return EECode_BadParam;
    }

    if (DUnlikely(mEncoderInputBufferSize != in_data_size)) {
        LOG_WARN("mEncoderInputBufferSize %ld not same as in_data_size %ld\n", mEncoderInputBufferSize, in_data_size);
    }

    out_data_size = 0;

    if (DLikely(in_data_size > 31)) {
        encodeFirstGroupSample(ref_value, p_in, p_out);
        p_out += 12;
        p_in += 16;
        in_data_size -= 32;
        out_data_size += 12;
    }

    while (in_data_size > 31) {
        encodeGroupSample(ref_value, p_in, p_out);
        p_out += 8;
        p_in += 16;
        in_data_size -= 32;
        out_data_size += 8;
    }

    return EECode_OK;
}

EECode CCustomizedADPCMCodec::Decoding(void *in_buffer, void *out_buffer, TMemSize in_data_size, TMemSize &out_data_size)
{
    TU8 *p_in = (TU8 *) in_buffer;
    TU16 *p_out = (TU16 *) out_buffer;
    TU16 ref_value = 0;

    if (DUnlikely(!in_buffer || !out_buffer || (!in_data_size))) {
        LOG_FATAL("NULL in_buffer %p or out_buffer %p, or in_data_size %ld\n", in_buffer, out_buffer, in_data_size);
        return EECode_BadParam;
    }

    out_data_size = 0;

    if (DLikely(in_data_size > 11)) {
        decodeFirstGroupSample(ref_value, p_in, p_out);
        p_out += 16;
        p_in += 12;
        in_data_size -= 12;
        out_data_size += 32;
    }

    while (in_data_size > 7) {
        decodeGroupSample(ref_value, p_in, p_out);
        p_out += 16;
        p_in += 8;
        in_data_size -= 8;
        out_data_size += 32;
    }

    //_5_tag_loop_filter_4((TS16*)out_buffer, out_data_size);
    //_5_tag_loop_filter_1((TS16*)out_buffer, out_data_size);

    return EECode_OK;
}

void CCustomizedADPCMCodec::Destroy()
{
    delete this;
}

void CCustomizedADPCMCodec::encodeGroupSample(TU16 &ref_value, TU16 *p_in, TU8 *p_out)
{
    if (DUnlikely(0 == mbModeVersion)) {
        _adpcm_quant_merge_2_samples(ref_value, p_in[0], p_in[1], p_out[0]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[0], p_in[1], p_out[0]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[2], p_in[3], p_out[1]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[2], p_in[3], p_out[1]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[4], p_in[5], p_out[2]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[4], p_in[5], p_out[2]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[6], p_in[7], p_out[3]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[6], p_in[7], p_out[3]);

        _adpcm_quant_merge_2_samples(ref_value, p_in[8], p_in[9], p_out[4]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[8], p_in[9], p_out[4]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[10], p_in[11], p_out[5]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[10], p_in[11], p_out[5]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[12], p_in[13], p_out[6]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[12], p_in[13], p_out[6]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[14], p_in[15], p_out[7]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[14], p_in[15], p_out[7]);
    } else {
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[0], p_in[1], p_out[0]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[0], p_in[1], p_out[0]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[2], p_in[3], p_out[1]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[2], p_in[3], p_out[1]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[4], p_in[5], p_out[2]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[4], p_in[5], p_out[2]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[6], p_in[7], p_out[3]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[6], p_in[7], p_out[3]);

        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[8], p_in[9], p_out[4]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[8], p_in[9], p_out[4]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[10], p_in[11], p_out[5]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[10], p_in[11], p_out[5]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[12], p_in[13], p_out[6]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[12], p_in[13], p_out[6]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[14], p_in[15], p_out[7]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[14], p_in[15], p_out[7]);
    }
}

void CCustomizedADPCMCodec::encodeFirstGroupSample(TU16 &ref_value, TU16 *p_in, TU8 *p_out)
{
    TU16 *p_header = (TU16 *)p_out;
    memcpy(p_header, &mHeader, 2);
    p_header[1] = p_in[0];

    //LOG_PRINTF("encodeFirstGroupSample: out header %02x %02x %02x %02x, ref %04x\n", p_out[0], p_out[1], p_out[2], p_out[3], p_in[0]);
    p_out += 4;
    ref_value = p_in[0];

    if (DUnlikely(0 == mbModeVersion)) {
        _adpcm_quant_merge_2_samples(ref_value, p_in[0], p_in[1], p_out[0]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[0], p_in[1], p_out[0]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[2], p_in[3], p_out[1]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[2], p_in[3], p_out[1]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[4], p_in[5], p_out[2]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[4], p_in[5], p_out[2]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[6], p_in[7], p_out[3]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[6], p_in[7], p_out[3]);

        _adpcm_quant_merge_2_samples(ref_value, p_in[8], p_in[9], p_out[4]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[8], p_in[9], p_out[4]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[10], p_in[11], p_out[5]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[10], p_in[11], p_out[5]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[12], p_in[13], p_out[6]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[12], p_in[13], p_out[6]);
        _adpcm_quant_merge_2_samples(ref_value, p_in[14], p_in[15], p_out[7]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[14], p_in[15], p_out[7]);
    } else {
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[0], p_in[1], p_out[0]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[0], p_in[1], p_out[0]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[2], p_in[3], p_out[1]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[2], p_in[3], p_out[1]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[4], p_in[5], p_out[2]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[4], p_in[5], p_out[2]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[6], p_in[7], p_out[3]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[6], p_in[7], p_out[3]);

        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[8], p_in[9], p_out[4]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[8], p_in[9], p_out[4]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[10], p_in[11], p_out[5]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[10], p_in[11], p_out[5]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[12], p_in[13], p_out[6]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[12], p_in[13], p_out[6]);
        _adpcm_quant_merge_2_samples_v1(ref_value, p_in[14], p_in[15], p_out[7]);
        //LOG_PRINTF("encode: ref %04x, in0 %04x, in1 %04x, out %02x\n", ref_value, p_in[14], p_in[15], p_out[7]);
    }
}

void CCustomizedADPCMCodec::decodeGroupSample(TU16 &ref_value, TU8 *p_in, TU16 *p_out)
{
    if (DUnlikely(0 == mbModeVersion)) {
        _adpcm_dequant_2_samples(ref_value, p_out[0], p_out[1], p_in[0]);
        _adpcm_dequant_2_samples(ref_value, p_out[2], p_out[3], p_in[1]);
        _adpcm_dequant_2_samples(ref_value, p_out[4], p_out[5], p_in[2]);
        _adpcm_dequant_2_samples(ref_value, p_out[6], p_out[7], p_in[3]);

        _adpcm_dequant_2_samples(ref_value, p_out[8], p_out[9], p_in[4]);
        _adpcm_dequant_2_samples(ref_value, p_out[10], p_out[11], p_in[5]);
        _adpcm_dequant_2_samples(ref_value, p_out[12], p_out[13], p_in[6]);
        _adpcm_dequant_2_samples(ref_value, p_out[14], p_out[15], p_in[7]);
    } else {
        _adpcm_dequant_2_samples_v1(ref_value, p_out[0], p_out[1], p_in[0]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[2], p_out[3], p_in[1]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[4], p_out[5], p_in[2]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[6], p_out[7], p_in[3]);

        _adpcm_dequant_2_samples_v1(ref_value, p_out[8], p_out[9], p_in[4]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[10], p_out[11], p_in[5]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[12], p_out[13], p_in[6]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[14], p_out[15], p_in[7]);
    }
}

void CCustomizedADPCMCodec::decodeFirstGroupSample(TU16 &ref_value, TU8 *p_in, TU16 *p_out)
{
    ref_value = ((TU16 *)p_in)[1];
    p_in += 4;
    if (DUnlikely(0 == mbModeVersion)) {
        _adpcm_dequant_2_samples(ref_value, p_out[0], p_out[1], p_in[0]);
        _adpcm_dequant_2_samples(ref_value, p_out[2], p_out[3], p_in[1]);
        _adpcm_dequant_2_samples(ref_value, p_out[4], p_out[5], p_in[2]);
        _adpcm_dequant_2_samples(ref_value, p_out[6], p_out[7], p_in[3]);

        _adpcm_dequant_2_samples(ref_value, p_out[8], p_out[9], p_in[4]);
        _adpcm_dequant_2_samples(ref_value, p_out[10], p_out[11], p_in[5]);
        _adpcm_dequant_2_samples(ref_value, p_out[12], p_out[13], p_in[6]);
        _adpcm_dequant_2_samples(ref_value, p_out[14], p_out[15], p_in[7]);
    } else {
        _adpcm_dequant_2_samples_v1(ref_value, p_out[0], p_out[1], p_in[0]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[2], p_out[3], p_in[1]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[4], p_out[5], p_in[2]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[6], p_out[7], p_in[3]);

        _adpcm_dequant_2_samples_v1(ref_value, p_out[8], p_out[9], p_in[4]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[10], p_out[11], p_in[5]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[12], p_out[13], p_in[6]);
        _adpcm_dequant_2_samples_v1(ref_value, p_out[14], p_out[15], p_in[7]);
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

