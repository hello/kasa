/*******************************************************************************
 * h264_parser.cpp
 *
 * History:
 *  2014/12/18 - [Zhi He] create file
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

#include "media_mw_if.h"
#include "codec_misc.h"
#include "codec_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define MAX_SPATIAL_SEGMENTATION 4096 // max. value of u(12) field
#define MIN_CACHE_BITS 25
#define MAX_SUB_LAYERS 7
#define MAX_VPS_COUNT 16
#define MAX_SPS_COUNT 32
#define MAX_PPS_COUNT 256
#define MAX_SHORT_TERM_RPS_COUNT 64
#define MAX_CU_SIZE 128

typedef struct {
    const TU8 *buffer, *buffer_end;
    TUint index;
    TUint size_in_bits;
} GetbitsContext;

#define READ_LE_32(x)   \
    ((((const TU8*)(x))[3] << 24)   | \
     (((const TU8*)(x))[2] << 16)    |   \
     (((const TU8*)(x))[1] <<  8)    |  \
     ((const TU8*)(x))[0])

#define READ_BE_32(x)   \
    ((((const TU8*)(x))[0] << 24) | \
     (((const TU8*)(x))[1] << 16) |  \
     (((const TU8*)(x))[2] <<  8) |   \
     ((const TU8*)(x))[3])

#define BITS_OPEN_READER(name, gb)   \
    TUint name##_index = (gb)->index;   \
    TUint name##_cache  =   0

#define BITS_CLOSE_READER(name, gb) \
    (gb)->index = name##_index; \
    name##_cache  =   0

#define BITS_OPEN_READER_SKIP(name, gb)   \
    TUint name##_index = (gb)->index

#define BITS_CLOSE_READER_SKIP(name, gb) \
    (gb)->index = name##_index

#define BITS_UPDATE_CACHE_BE(name, gb) \
    name##_cache = READ_BE_32(((const TU8 *)(gb)->buffer)+(name##_index>>3)) << (name##_index&0x07)
#define BITS_UPDATE_CACHE_LE(name, gb) \
    name##_cache = READ_LE_32(((const TU8 *)(gb)->buffer)+(name##_index>>3)) >> (name##_index&0x07)

#define BITS_SKIP_CACHE(name, gb, num) name##_cache >>= (num)
#define BITS_SKIP_COUNTER(name, gb, num) name##_index += (num)

#define BITS_SKIP_BITS(name, gb, num) do {  \
        BITS_SKIP_CACHE(name, gb, num); \
        BITS_SKIP_COUNTER(name, gb, num);   \
    } while (0)

#define BITS_LAST_SKIP_BITS(name, gb, num) BITS_SKIP_COUNTER(name, gb, num)
#define BITS_LAST_SKIP_CACHE(name, gb, num)

#define NEG_USR32(a,s) (((TU32)(a))>>(32-(s)))
#define BITS_SHOW_UBITS(name, gb, num) NEG_USR32(name ## _cache, num)

#define BITS_GET_CACHE(name, gb) ((TU32)name##_cache)

const TU8 simple_log2_table[256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

const TU8 simple_golomb_vlc_len[512] = {
    19, 17, 15, 15, 13, 13, 13, 13, 11, 11, 11, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const TU8 simple_ue_golomb_vlc_code[512] = {
    32, 32, 32, 32, 32, 32, 32, 32, 31, 32, 32, 32, 32, 32, 32, 32, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 14,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const TS8 simple_se_golomb_vlc_code[512] = {
    17, 17, 17, 17, 17, 17, 17, 17, 16, 17, 17, 17, 17, 17, 17, 17,  8, -8,  9, -9, 10, -10, 11, -11, 12, -12, 13, -13, 14, -14, 15, -15,
    4,  4,  4,  4, -4, -4, -4, -4,  5,  5,  5,  5, -5, -5, -5, -5,  6,  6,  6,  6, -6, -6, -6, -6,  7,  7,  7,  7, -7, -7, -7, -7,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

static TInt simple_log2_c(TUint v)
{
    TInt n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += simple_log2_table[v];

    return n;
}

static inline TInt _get_se_golomb(GetbitsContext *gb)
{
    TUint buf;
    TInt log;

    BITS_OPEN_READER(re, gb);
    BITS_UPDATE_CACHE_BE(re, gb);
    buf = BITS_GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
        BITS_CLOSE_READER(re, gb);

        return simple_se_golomb_vlc_code[buf];
    } else {
        log = simple_log2_c(buf);
        BITS_LAST_SKIP_BITS(re, gb, 31 - log);
        BITS_UPDATE_CACHE_BE(re, gb);
        buf = BITS_GET_CACHE(re, gb);
        buf >>= log;

        BITS_LAST_SKIP_BITS(re, gb, 32 - log);
        BITS_CLOSE_READER(re, gb);

        if (buf & 1) { buf = -(buf >> 1); }
        else      { buf = (buf >> 1); }

        return buf;
    }
}

static inline TInt _get_ue_golomb(GetbitsContext *gb)
{
    TUint buf;
    TInt log;

    BITS_OPEN_READER(re, gb);
    BITS_UPDATE_CACHE_BE(re, gb);
    buf = BITS_GET_CACHE(re, gb);

    if (buf >= (1 << 27)) {
        buf >>= 32 - 9;
        BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
        BITS_CLOSE_READER(re, gb);
        return simple_ue_golomb_vlc_code[buf];
    } else {
        log = 2 * simple_log2_c(buf) - 31;
        buf >>= log;
        buf--;
        BITS_LAST_SKIP_BITS(re, gb, 32 - log);
        BITS_CLOSE_READER(re, gb);
        return buf;
    }
}

static inline TInt _get_ue_golomb_31(GetbitsContext *gb)
{
    TUint buf;

    BITS_OPEN_READER(re, gb);

    BITS_UPDATE_CACHE_BE(re, gb);
    buf = BITS_GET_CACHE(re, gb);

    buf >>= 32 - 9;

    BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
    BITS_CLOSE_READER(re, gb);

    return simple_ue_golomb_vlc_code[buf];
}

static inline TUint _show_bits(GetbitsContext *s, TInt n)
{
    register TInt tmp;
    BITS_OPEN_READER(re, s);
    BITS_UPDATE_CACHE_BE(re, s);
    tmp = BITS_SHOW_UBITS(re, s, n);
    return tmp;
}

static inline TUint _show_bits1(GetbitsContext *s)
{
    return _show_bits(s, 1);
}

static inline TUint _get_bits(GetbitsContext *s, TInt n)
{
    TInt tmp;
    BITS_OPEN_READER(re, s);
    BITS_UPDATE_CACHE_BE(re, s);
    tmp = BITS_SHOW_UBITS(re, s, n);
    BITS_LAST_SKIP_BITS(re, s, n);
    BITS_CLOSE_READER(re, s);
    return tmp;
}

static inline TUint _get_bits1(GetbitsContext *s)
{
    TUint index = s->index;
    TU8 result  = s->buffer[index >> 3];

    result <<= index & 7;
    result >>= 8 - 1;

    index++;
    s->index = index;

    return result;
}

static inline TU32 _get_bits_long(GetbitsContext *s, TInt n)
{
    if (!n) {
        return 0;
    } else if (n <= MIN_CACHE_BITS) {
        return _get_bits(s, n);
    } else {
        unsigned ret = _get_bits(s, 16) << (n - 16);
        return ret | _get_bits(s, n - 16);
    }
}

static TU64 _get_bits64(GetbitsContext *s, TInt n)
{
    if (n <= 32) {
        return _get_bits_long(s, n);
    } else {
        TU64 ret = (TU64) _get_bits_long(s, n - 32) << 32;
        return ret | _get_bits_long(s, 32);
    }
}

static inline void _skip_bits(GetbitsContext *s, TInt n)
{
    BITS_OPEN_READER_SKIP(re, s);
    BITS_LAST_SKIP_BITS(re, s, n);
    BITS_CLOSE_READER_SKIP(re, s);
}

static inline void _skip_bits1(GetbitsContext *s)
{
    _skip_bits(s, 1);
}

static inline TU32 _show_bits_long(GetbitsContext *s, TInt n)
{
    if (n <= MIN_CACHE_BITS) {
        return _show_bits(s, n);
    } else {
        GetbitsContext gb = *s;
        return _get_bits_long(&gb, n);
    }
}

static inline void _skip_bits_long(GetbitsContext *s, TInt n)
{
    s->index += n;
}

static inline unsigned _get_ue_golomb_long(GetbitsContext *gb)
{
    unsigned buf, log;

    buf = _show_bits_long(gb, 32);
    log = 31 - simple_log2_c(buf);
    _skip_bits_long(gb, log);

    return _get_bits_long(gb, log + 1) - 1;
}

static inline TInt _get_se_golomb_long(GetbitsContext *gb)
{
    TU32 buf = _get_ue_golomb_long(gb);

    if (buf & 1)
    { buf = (buf + 1) >> 1; }
    else
    { buf = -(buf >> 1); }

    return buf;
}

typedef struct SHVCCProfileTierLevel {
    TU8  profile_space;
    TU8  tier_flag;
    TU8  profile_idc;
    TU32 profile_compatibility_flags;
    TU64 constraint_indicator_flags;
    TU8  level_idc;
} SHVCCProfileTierLevel;

static TU8 *_rbsp_from_nalu(const TU8 *src, TU32 src_len, TU32 *dst_len)
{
    TU8 *dst;
    TU32 i, len;

    dst = (TU8 *) DDBG_MALLOC(src_len, "RBSP");
    if (!dst) {
        return NULL;
    }

    i = len = 0;
    while (i < 2 && i < src_len) {
        dst[len++] = src[i++];
    }

    while (i + 2 < src_len) {
        if (!src[i] && !src[i + 1] && src[i + 2] == 3) {
            dst[len++] = src[i++];
            dst[len++] = src[i++];
            i++;
        } else {
            dst[len++] = src[i++];
        }
    }

    while (i < src_len) {
        dst[len++] = src[i++];
    }

    *dst_len = len;
    return dst;
}

static void _skip_sub_layer_ordering_info(GetbitsContext *gb)
{
    _get_ue_golomb_long(gb);
    _get_ue_golomb_long(gb);
    _get_ue_golomb_long(gb);
}

static void _skip_scaling_list_data(GetbitsContext *gb)
{
    TInt i, j, k, num_coeffs;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < (i == 3 ? 2 : 6); j++) {
            if (!_get_bits1(gb)) {
                _get_ue_golomb_long(gb);
            } else {
                num_coeffs = DMIN(64, 1 << (4 + (i << 1)));

                if (i > 1) {
                    _get_se_golomb_long(gb);
                }

                for (k = 0; k < num_coeffs; k++) {
                    _get_se_golomb_long(gb);
                }
            }
        }
    }
}

static void _skip_sub_layer_hrd_parameters(GetbitsContext *gb, TU32 cpb_cnt_minus1, TU8 sub_pic_hrd_params_present_flag)
{
    TU32 i;

    for (i = 0; i <= cpb_cnt_minus1; i++) {
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);

        if (sub_pic_hrd_params_present_flag) {
            _get_ue_golomb_long(gb);
            _get_ue_golomb_long(gb);
        }

        _skip_bits1(gb);
    }
}

static void _skip_hrd_parameters(GetbitsContext *gb, TU8 cprms_present_flag, TU32 max_sub_layers_minus1)
{
    TU32 i;
    TU8 sub_pic_hrd_params_present_flag = 0;
    TU8 nal_hrd_parameters_present_flag = 0;
    TU8 vcl_hrd_parameters_present_flag = 0;

    if (cprms_present_flag) {
        nal_hrd_parameters_present_flag = _get_bits1(gb);
        vcl_hrd_parameters_present_flag = _get_bits1(gb);

        if (nal_hrd_parameters_present_flag ||
                vcl_hrd_parameters_present_flag) {
            sub_pic_hrd_params_present_flag = _get_bits1(gb);

            if (sub_pic_hrd_params_present_flag)
                /*
                 * tick_divisor_minus2                          u(8)
                 * du_cpb_removal_delay_increment_length_minus1 u(5)
                 * sub_pic_cpb_params_in_pic_timing_sei_flag    u(1)
                 * dpb_output_delay_du_length_minus1            u(5)
                 */
            { _skip_bits(gb, 19); }

            /*
             * bit_rate_scale u(4)
             * cpb_size_scale u(4)
             */
            _skip_bits(gb, 8);

            if (sub_pic_hrd_params_present_flag)
            { _skip_bits(gb, 4); } // cpb_size_du_scale

            /*
             * initial_cpb_removal_delay_length_minus1 u(5)
             * au_cpb_removal_delay_length_minus1      u(5)
             * dpb_output_delay_length_minus1          u(5)
             */
            _skip_bits(gb, 15);
        }
    }

    for (i = 0; i <= max_sub_layers_minus1; i++) {
        TU32 cpb_cnt_minus1            = 0;
        TU8 low_delay_hrd_flag             = 0;
        TU8 fixed_pic_rate_within_cvs_flag = 0;
        TU8 fixed_pic_rate_general_flag    = _get_bits1(gb);

        if (!fixed_pic_rate_general_flag)
        { fixed_pic_rate_within_cvs_flag = _get_bits1(gb); }

        if (fixed_pic_rate_within_cvs_flag)
        { _get_ue_golomb_long(gb); } // elemental_duration_in_tc_minus1
        else
        { low_delay_hrd_flag = _get_bits1(gb); }

        if (!low_delay_hrd_flag)
        { cpb_cnt_minus1 = _get_ue_golomb_long(gb); }

        if (nal_hrd_parameters_present_flag)
            _skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                           sub_pic_hrd_params_present_flag);

        if (vcl_hrd_parameters_present_flag)
            _skip_sub_layer_hrd_parameters(gb, cpb_cnt_minus1,
                                           sub_pic_hrd_params_present_flag);
    }
}

static void _skip_timing_info(GetbitsContext *gb)
{
    _skip_bits_long(gb, 32);
    _skip_bits_long(gb, 32);

    if (_get_bits1(gb)) {
        _get_ue_golomb_long(gb);
    }
}

static void _hvcc_parse_vui(GetbitsContext *gb, SHEVCDecoderConfigurationRecord *record, TU32 max_sub_layers_minus1)
{
    TU32 min_spatial_segmentation_idc;

    if (_get_bits1(gb)) {
        if (_get_bits(gb, 8) == 255) {
            _skip_bits_long(gb, 32);
        }
    }

    if (_get_bits1(gb)) {
        _skip_bits1(gb);
    }

    if (_get_bits1(gb)) {
        _skip_bits(gb, 4);

        if (_get_bits1(gb)) {
            _skip_bits(gb, 24);
        }
    }

    if (_get_bits1(gb)) {
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
    }

    _skip_bits(gb, 3);

    if (_get_bits1(gb)) {
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
    }

    if (_get_bits1(gb)) {
        _skip_timing_info(gb);
        if (_get_bits1(gb)) {
            _skip_hrd_parameters(gb, 1, max_sub_layers_minus1);
        }
    }

    if (_get_bits1(gb)) {
        _skip_bits(gb, 3);
        min_spatial_segmentation_idc = _get_ue_golomb_long(gb);
        record->min_spatial_segmentation_idc = DMIN(record->min_spatial_segmentation_idc, min_spatial_segmentation_idc);

        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
        _get_ue_golomb_long(gb);
    }
}

static void _hvcc_update_ptl(SHEVCDecoderConfigurationRecord *record, SHVCCProfileTierLevel *ptl)
{
    record->general_profile_space = ptl->profile_space;
    if (record->general_tier_flag < ptl->tier_flag) {
        record->general_level_idc = ptl->level_idc;
    } else {
        record->general_level_idc = DMAX(record->general_level_idc, ptl->level_idc);
    }

    record->general_tier_flag = DMAX(record->general_tier_flag, ptl->tier_flag);
    record->general_profile_idc = DMAX(record->general_profile_idc, ptl->profile_idc);
    record->general_profile_compatibility_flags &= ptl->profile_compatibility_flags;
    record->general_constraint_indicator_flags &= ptl->constraint_indicator_flags;
}

static void _hvcc_parse_ptl(GetbitsContext *gb, SHEVCDecoderConfigurationRecord *record, TU32 max_sub_layers_minus1)
{
    TU32 i;
    SHVCCProfileTierLevel general_ptl;
    TU8 sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    TU8 sub_layer_level_present_flag[MAX_SUB_LAYERS];

    general_ptl.profile_space               = _get_bits(gb, 2);
    general_ptl.tier_flag                   = _get_bits1(gb);
    general_ptl.profile_idc                 = _get_bits(gb, 5);
    general_ptl.profile_compatibility_flags = _get_bits_long(gb, 32);
    general_ptl.constraint_indicator_flags  = _get_bits64(gb, 48);
    general_ptl.level_idc                   = _get_bits(gb, 8);
    _hvcc_update_ptl(record, &general_ptl);

    for (i = 0; i < max_sub_layers_minus1; i++) {
        sub_layer_profile_present_flag[i] = _get_bits1(gb);
        sub_layer_level_present_flag[i]   = _get_bits1(gb);
    }

    if (max_sub_layers_minus1 > 0) {
        for (i = max_sub_layers_minus1; i < 8; i++) {
            _skip_bits(gb, 2);
        }
    }

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            _skip_bits_long(gb, 32);
            _skip_bits_long(gb, 32);
            _skip_bits(gb, 24);
        }

        if (sub_layer_level_present_flag[i])
        { _skip_bits(gb, 8); }
    }
}

static TInt _parse_rps(GetbitsContext *gb, TU32 rps_idx, TU32 num_rps, TU32 num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT])
{
    TU32 i;

    if (rps_idx && _get_bits1(gb)) {
        if (rps_idx >= num_rps) {
            LOG_ERROR("_parse_rps, rps_idx > num_rps\n");
            return (-1);
        }

        _skip_bits1(gb);
        _get_ue_golomb_long(gb);

        num_delta_pocs[rps_idx] = 0;

        for (i = 0; i < num_delta_pocs[rps_idx - 1]; i++) {
            TU8 use_delta_flag = 0;
            TU8 used_by_curr_pic_flag = _get_bits1(gb);
            if (!used_by_curr_pic_flag) {
                use_delta_flag = _get_bits1(gb);
            }

            if (used_by_curr_pic_flag || use_delta_flag) {
                num_delta_pocs[rps_idx]++;
            }
        }
    } else {
        TU32 num_negative_pics = _get_ue_golomb_long(gb);
        TU32 num_positive_pics = _get_ue_golomb_long(gb);

        num_delta_pocs[rps_idx] = num_negative_pics + num_positive_pics;

        for (i = 0; i < num_negative_pics; i++) {
            _get_ue_golomb_long(gb);
            _skip_bits1(gb);
        }

        for (i = 0; i < num_positive_pics; i++) {
            _get_ue_golomb_long(gb);
            _skip_bits1(gb);
        }
    }

    return 0;
}

static void _hvcc_parse_vps(SHEVCDecoderConfigurationRecord *record, TU8 *p_data, TU32 data_size)
{
    GetbitsContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = 8 * data_size;

    TU32 vps_max_sub_layers_minus1;
    _skip_bits(&gb, 12);
    vps_max_sub_layers_minus1 = _get_bits(&gb, 3);
    record->numTemporalLayers = DMAX(record->numTemporalLayers, vps_max_sub_layers_minus1 + 1);
    _skip_bits(&gb, 17);

    _hvcc_parse_ptl(&gb, record, vps_max_sub_layers_minus1);
}

static void _hvcc_parse_sps(SHEVCDecoderConfigurationRecord *record, TU8 *p_data, TU32 data_size, TU32 &pic_width, TU32 &pic_height)
{
    GetbitsContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = 8 * data_size;

    TU32 i, sps_max_sub_layers_minus1, log2_max_pic_order_cnt_lsb_minus4;
    TU32 num_short_term_ref_pic_sets, num_delta_pocs[MAX_SHORT_TERM_RPS_COUNT];

    _skip_bits(&gb, 4);
    sps_max_sub_layers_minus1 = _get_bits(&gb, 3);
    record->numTemporalLayers = DMAX(record->numTemporalLayers, sps_max_sub_layers_minus1 + 1);
    record->temporalIdNested = _get_bits1(&gb);
    _hvcc_parse_ptl(&gb, record, sps_max_sub_layers_minus1);
    _get_ue_golomb_long(&gb);
    record->chromaFormat = _get_ue_golomb_long(&gb);

    if (record->chromaFormat == 3) {
        _skip_bits1(&gb);
    }

    pic_width = _get_ue_golomb_long(&gb);
    pic_height = _get_ue_golomb_long(&gb);

    if (_get_bits1(&gb)) {
        _get_ue_golomb_long(&gb);
        _get_ue_golomb_long(&gb);
        _get_ue_golomb_long(&gb);
        _get_ue_golomb_long(&gb);
    }

    record->bitDepthLumaMinus8 = _get_ue_golomb_long(&gb);
    record->bitDepthChromaMinus8 = _get_ue_golomb_long(&gb);
    log2_max_pic_order_cnt_lsb_minus4 = _get_ue_golomb_long(&gb);

    i = _get_bits1(&gb) ? 0 : sps_max_sub_layers_minus1;
    for (; i <= sps_max_sub_layers_minus1; i++)
    { _skip_sub_layer_ordering_info(&gb); }

    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);

    if (_get_bits1(&gb) && _get_bits1(&gb))
    { _skip_scaling_list_data(&gb); }

    _skip_bits1(&gb);
    _skip_bits1(&gb);

    if (_get_bits1(&gb)) {
        _skip_bits(&gb, 4);
        _skip_bits(&gb, 4);
        _get_ue_golomb_long(&gb);
        _get_ue_golomb_long(&gb);
        _skip_bits1(&gb);
    }

    num_short_term_ref_pic_sets = _get_ue_golomb_long(&gb);
    if (DUnlikely(num_short_term_ref_pic_sets > MAX_SHORT_TERM_RPS_COUNT)) {
        LOG_FATAL("_hvcc_parse_sps fail, num_short_term_ref_pic_sets(%d) exceed max value\n", num_short_term_ref_pic_sets);
        return;
    }

    for (i = 0; i < num_short_term_ref_pic_sets; i++) {
        TInt ret = _parse_rps(&gb, i, num_short_term_ref_pic_sets, num_delta_pocs);
        if (DUnlikely(ret < 0)) {
            LOG_FATAL("_parse_rps fail, ret %d\n", ret);
            return;
        }
    }

    if (_get_bits1(&gb)) {
        for (i = 0; i < _get_ue_golomb_long(&gb); i++) {
            TInt len = DMIN(log2_max_pic_order_cnt_lsb_minus4 + 4, 16);
            _skip_bits(&gb, len);
            _skip_bits1(&gb);
        }
    }

    _skip_bits1(&gb);
    _skip_bits1(&gb);

    if (_get_bits1(&gb)) {
        _hvcc_parse_vui(&gb, record, sps_max_sub_layers_minus1);
    }
}

static void _hvcc_parse_pps(SHEVCDecoderConfigurationRecord *record, TU8 *p_data, TU32 data_size)
{
    GetbitsContext gb;
    gb.buffer = p_data;
    gb.buffer_end = p_data + data_size;
    gb.index = 0;
    gb.size_in_bits = 8 * data_size;

    TU8 tiles_enabled_flag, entropy_coding_sync_enabled_flag;

    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _skip_bits(&gb, 7);

    _get_ue_golomb_long(&gb);
    _get_ue_golomb_long(&gb);
    _get_se_golomb_long(&gb);
    _skip_bits(&gb, 2);

    if (_get_bits1(&gb)) {
        _get_ue_golomb_long(&gb);
    }

    _get_se_golomb_long(&gb);
    _get_se_golomb_long(&gb);
    _skip_bits(&gb, 3);

    tiles_enabled_flag = _get_bits1(&gb);
    entropy_coding_sync_enabled_flag = _get_bits1(&gb);

    if (entropy_coding_sync_enabled_flag && tiles_enabled_flag) {
        record->parallelismType = 0;
    } else if (entropy_coding_sync_enabled_flag) {
        record->parallelismType = 3;
    } else if (tiles_enabled_flag) {
        record->parallelismType = 2;
    } else {
        record->parallelismType = 1;
    }
}

static void _parse_ptl(GetbitsContext *gb, TU32 max_sub_layers_minus1, SHVCCProfileTierLevel *general_ptl)
{
    TU32 i = 0;
    TU8 sub_layer_profile_present_flag[MAX_SUB_LAYERS];
    TU8 sub_layer_level_present_flag[MAX_SUB_LAYERS];

    general_ptl->profile_space               = _get_bits(gb, 2);
    general_ptl->tier_flag                   = _get_bits1(gb);
    general_ptl->profile_idc                 = _get_bits(gb, 5);
    general_ptl->profile_compatibility_flags = _get_bits_long(gb, 32);
    general_ptl->constraint_indicator_flags  = _get_bits64(gb, 48);
    general_ptl->level_idc                   = _get_bits(gb, 8);

    for (i = 0; i < max_sub_layers_minus1; i++) {
        sub_layer_profile_present_flag[i] = _get_bits1(gb);
        sub_layer_level_present_flag[i]   = _get_bits1(gb);
    }

    if (max_sub_layers_minus1 > 0) {
        for (i = max_sub_layers_minus1; i < 8; i++) {
            _skip_bits(gb, 2);
        }
    }

    for (i = 0; i < max_sub_layers_minus1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            _skip_bits_long(gb, 32);
            _skip_bits_long(gb, 32);
            _skip_bits(gb, 24);
        }

        if (sub_layer_level_present_flag[i])
        { _skip_bits(gb, 8); }
    }
}

EECode gfGetH265SizeFromSPS(TU8 *p_data, TU32 data_size, TU32 &pic_width, TU32 &pic_height)
{
    if (DUnlikely(!p_data || !data_size)) {
        LOG_ERROR("NULL params in gfGetH265SizeFromSPS()\n");
        return EECode_BadParam;
    }

    TU32 rbsp_len = 0;
    TU8 *p_rbsp = _rbsp_from_nalu(p_data, data_size, &rbsp_len);
    if (DUnlikely(!p_data || !data_size)) {
        LOG_ERROR("no memory when parse sps\n");
        return EECode_NoMemory;
    }

    GetbitsContext gb;
    gb.buffer = p_rbsp;
    gb.buffer_end = p_rbsp + rbsp_len;
    gb.index = 0;
    gb.size_in_bits = 8 * rbsp_len;

    TU32 sps_max_sub_layers_minus1;
    SHVCCProfileTierLevel general_ptl;

    _skip_bits(&gb, 4);
    sps_max_sub_layers_minus1 = _get_bits(&gb, 3);
    _get_bits1(&gb);
    _parse_ptl(&gb, sps_max_sub_layers_minus1, &general_ptl);
    _get_ue_golomb_long(&gb);
    TU8 chomaformat = _get_ue_golomb_long(&gb);

    if (chomaformat == 3) {
        _skip_bits1(&gb);
    }

    pic_width = _get_ue_golomb_long(&gb);
    pic_height = _get_ue_golomb_long(&gb);

    DDBG_FREE(p_rbsp, "RBSP");

    return EECode_OK;
}

EECode gfGenerateHEVCDecoderConfigurationRecord(SHEVCDecoderConfigurationRecord *record, TU8 *vps, TU32 vps_length, TU8 *sps, TU32 sps_length, TU8 *pps, TU32 pps_length, TU32 &pic_width, TU32 &pic_height)
{
    if (!record || !vps || !vps_length || !sps || !sps_length || !pps || !pps_length) {
        LOG_ERROR("NULL params in gfGenerateHEVCDecoderConfigurationRecord()\n");
        return EECode_BadParam;
    }

    memset(record, 0, sizeof(SHEVCDecoderConfigurationRecord));
    record->configurationVersion = 1;
    record->lengthSizeMinusOne   = 3;
    record->general_profile_compatibility_flags = 0xffffffff;
    record->general_constraint_indicator_flags  = (TU64) 0xffffffffffffLL;
    record->min_spatial_segmentation_idc = MAX_SPATIAL_SEGMENTATION + 1;

    TU32 rbsp_len = 0;
    TU8 *p_rbsp = _rbsp_from_nalu(vps + 2, vps_length - 2, &rbsp_len);

    if (p_rbsp) {
        _hvcc_parse_vps(record, p_rbsp, rbsp_len);
        DDBG_FREE(p_rbsp, "RBSP");
    } else {
        LOG_FATAL("no memory when parse vps?\n");
    }

    p_rbsp = _rbsp_from_nalu(sps + 2, sps_length - 2, &rbsp_len);
    if (p_rbsp) {
        _hvcc_parse_sps(record, p_rbsp, rbsp_len, pic_width, pic_height);
        DDBG_FREE(p_rbsp, "RBSP");
    } else {
        LOG_FATAL("no memory when parse sps?\n");
    }

    p_rbsp = _rbsp_from_nalu(pps + 2, pps_length - 2, &rbsp_len);
    if (p_rbsp) {
        _hvcc_parse_pps(record, p_rbsp, rbsp_len);
        DDBG_FREE(p_rbsp, "RBSP");
    } else {
        LOG_FATAL("no memory when parse sps?\n");
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

