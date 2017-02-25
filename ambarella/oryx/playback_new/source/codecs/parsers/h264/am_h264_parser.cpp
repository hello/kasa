/**
 * am_h264_parser.cpp
 *
 * History:
 *  2013/11/18 - [Zhi He] create file
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
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_codec_interface.h"

typedef struct {
  const TU8 *buffer, *buffer_end;
  TUint index;
  TUint size_in_bits;
} GetbitsContext;

typedef struct {
  TInt num; ///< numerator
  TInt den; ///< denominator
} _AVRational;

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

#define BIT_OPEN_READER(name, gb)   \
  TUint name##_index = (gb)->index;   \
  TUint name##_cache  =   0

#define BITS_CLOSE_READER(name, gb) (gb)->index = name##_index
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

//log2
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

static const _AVRational pixel_aspect[17] = {
  {0, 1},
  {1, 1},
  {12, 11},
  {10, 11},
  {16, 11},
  {40, 33},
  {24, 11},
  {20, 11},
  {32, 11},
  {80, 33},
  {18, 11},
  {15, 11},
  {64, 33},
  {160, 99},
  {4, 3},
  {3, 2},
  {2, 1},
};


static const TU8 default_scaling4[2][16] = {
  {
    6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42
  }, {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34
  }
};

static const TU8 default_scaling8[2][64] = {
  {
    6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42
  }, {
    9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35
  }
};

static const TU8 simple_zigzag_scan[16] = {
  0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
  1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
  1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
  3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};

const TU8 simple_zigzag_direct[64] = {
  0,   1,  8, 16,  9,  2,  3, 10,
  17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
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
  BIT_OPEN_READER(re, gb);
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
  BIT_OPEN_READER(re, gb);
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
  BIT_OPEN_READER(re, gb);
  BITS_UPDATE_CACHE_BE(re, gb);
  buf = BITS_GET_CACHE(re, gb);
  buf >>= 32 - 9;
  BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
  BITS_CLOSE_READER(re, gb);
  return simple_ue_golomb_vlc_code[buf];
}

static inline TUint _show_bits(GetbitsContext *s, int n)
{
  register int tmp;
  BIT_OPEN_READER(re, s);
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
  BIT_OPEN_READER(re, s);
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
#if 0
static inline TUint _get_bits_long(GetbitsContext *s, TInt n)
{
  if (n <= DMIN_CACHE_BITS) {
    return _get_bits(s, n);
  } else {
    TInt ret = _get_bits(s, 16) << (n - 16);
    return ret | _get_bits(s, n - 16);
  }
}

static inline TInt _get_bits_count(const GetbitsContext *s)
{
  return s->index;
}

static inline TInt _get_bits_left(GetbitsContext *gb)
{
  return gb->size_in_bits - _get_bits_count(gb);
}

static inline void _skip_bits_long(GetbitsContext *s, int n)
{
  s->index += n;
}

static inline TUint _show_bits_long(GetbitsContext *s, TInt n)
{
  if (n <= DMIN_CACHE_BITS) {
    return _show_bits(s, n);
  } else {
    GetbitsContext gb = *s;
    return _get_bits_long(&gb, n);
  }
}

static inline TUint _get_ue_golomb_long(GetbitsContext *gb)
{
  TUint buf, log;
  buf = _show_bits_long(gb, 32);
  log = 31 - simple_log2_c(buf);
  _skip_bits_long(gb, log);
  return _get_bits_long(gb, log + 1) - 1;
}

static inline TInt _decode_hrd_parameters(GetbitsContext *gb, SCodecVideoH264SPS *sps)
{
  TUint cpb_count, i;
  cpb_count = _get_ue_golomb_31(gb) + 1;
  if (cpb_count > 32U) {
    LOG_ERROR("cpb_count %d invalid\n", cpb_count);
    return -1;
  }
  _get_bits(gb, 4); /* bit_rate_scale */
  _get_bits(gb, 4); /* cpb_size_scale */
  for (i = 0; i < cpb_count; i++) {
    _get_ue_golomb_long(gb); /* bit_rate_value_minus1 */
    _get_ue_golomb_long(gb); /* cpb_size_value_minus1 */
    _get_bits1(gb);     /* cbr_flag */
  }
  sps->initial_cpb_removal_delay_length = _get_bits(gb, 5) + 1;
  sps->cpb_removal_delay_length = _get_bits(gb, 5) + 1;
  sps->dpb_output_delay_length = _get_bits(gb, 5) + 1;
  sps->time_offset_length = _get_bits(gb, 5);
  sps->cpb_cnt = cpb_count;
  return 0;
}

static inline TInt _decode_vui_parameters(SCodecVideoH264SPS *sps, GetbitsContext *gb)
{
  TInt aspect_ratio_info_present_flag;
  TUint aspect_ratio_idc;
  aspect_ratio_info_present_flag = _get_bits1(gb);
  if (aspect_ratio_info_present_flag) {
    aspect_ratio_idc = _get_bits(gb, 8);
    if (aspect_ratio_idc == DEXTENDED_SAR) {
      sps->sar_num = _get_bits(gb, 16);
      sps->sar_den = _get_bits(gb, 16);
    } else if (aspect_ratio_idc < DARRAY_ELEMS(pixel_aspect)) {
      sps->sar_num =  pixel_aspect[aspect_ratio_idc].num;
      sps->sar_den =  pixel_aspect[aspect_ratio_idc].den;
    } else {
      LOG_ERROR("illegal aspect ratio\n");
      return -1;
    }
  } else {
    sps->sar_num =
      sps->sar_den = 0;
  }
  if (_get_bits1(gb)) {    /* overscan_info_present_flag */
    _get_bits1(gb);      /* overscan_appropriate_flag */
  }
  sps->video_signal_type_present_flag = _get_bits1(gb);
  if (sps->video_signal_type_present_flag) {
    _get_bits(gb, 3);    /* video_format */
    sps->full_range = _get_bits1(gb); /* video_full_range_flag */
    sps->colour_description_present_flag = _get_bits1(gb);
    if (sps->colour_description_present_flag) {
      sps->color_primaries = (EColorPrimaries)_get_bits(gb, 8); /* colour_primaries */
      sps->color_trc       = (EColorTransferCharacteristic)_get_bits(gb, 8); /* transfer_characteristics */
      sps->colorspace      = (EColorSpace)_get_bits(gb, 8); /* matrix_coefficients */
      if (sps->color_primaries >= EColorPrimaries_NB) {
        sps->color_primaries  = EColorPrimaries_Unspecified;
      }
      if (sps->color_trc >= EColorTransferCharacteristic_NB) {
        sps->color_trc  = EColorTransferCharacteristic_UNSPECIFIED;
      }
      if (sps->colorspace >= EColorSpace_NB) {
        sps->colorspace  = EColorSpace_UNSPECIFIED;
      }
    }
  }
  if (_get_bits1(gb)) {    /* chroma_location_info_present_flag */
    /*s->avctx->chroma_sample_location = */_get_ue_golomb(gb) + 1; /* chroma_sample_location_type_top_field */
    _get_ue_golomb(gb);  /* chroma_sample_location_type_bottom_field */
  }
  if (_show_bits1(&gb) && _get_bits_left(&gb) < 10) {
    LOG_WARN("Truncated VUI\n");
    return 0;
  }
  sps->timing_info_present_flag = _get_bits1(gb);
  if (sps->timing_info_present_flag) {
    sps->num_units_in_tick = _get_bits_long(gb, 32);
    sps->time_scale = _get_bits_long(gb, 32);
    if (!sps->num_units_in_tick || !sps->time_scale) {
      LOG_ERROR("time_scale/num_units_in_tick invalid or unsupported (%d/%d)\n", sps->time_scale, sps->num_units_in_tick);
      return -1;
    }
    sps->fixed_frame_rate_flag = _get_bits1(gb);
  }
  sps->nal_hrd_parameters_present_flag = _get_bits1(gb);
  if (sps->nal_hrd_parameters_present_flag) {
    if (_decode_hrd_parameters(gb, sps) < 0) {
      return -1;
    }
  }
  sps->vcl_hrd_parameters_present_flag = _get_bits1(gb);
  if (sps->vcl_hrd_parameters_present_flag) {
    if (_decode_hrd_parameters(gb, sps) < 0) {
      return -1;
    }
  }
  if (sps->nal_hrd_parameters_present_flag || sps->vcl_hrd_parameters_present_flag) {
    _get_bits1(gb);     /* low_delay_hrd_flag */
  }
  sps->pic_struct_present_flag = _get_bits1(gb);
  if (!_get_bits_left(gb)) {
    return 0;
  }
  sps->bitstream_restriction_flag = _get_bits1(gb);
  if (sps->bitstream_restriction_flag) {
    _get_bits1(gb);     /* motion_vectors_over_pic_boundaries_flag */
    _get_ue_golomb(gb); /* max_bytes_per_pic_denom */
    _get_ue_golomb(gb); /* max_bits_per_mb_denom */
    _get_ue_golomb(gb); /* log2_max_mv_length_horizontal */
    _get_ue_golomb(gb); /* log2_max_mv_length_vertical */
    sps->num_reorder_frames = _get_ue_golomb(gb);
    _get_ue_golomb(gb); /*max_dec_frame_buffering*/
    if (_get_bits_left(gb) < 0) {
      sps->num_reorder_frames = 0;
      sps->bitstream_restriction_flag = 0;
    }
    if (sps->num_reorder_frames > 16U /*max_dec_frame_buffering || max_dec_frame_buffering > 16*/) {
      LOG_ERROR("illegal num_reorder_frames %d\n", sps->num_reorder_frames);
      sps->num_reorder_frames = 16;
      return -1;
    }
  }
  if (_get_bits_left(gb) < 0) {
    LOG_ERROR("Overread VUI by %d bits\n", -_get_bits_left(gb));
    return -1;
  }
  return 0;
}
#endif
static void _decode_scaling_list(GetbitsContext *gb, TU8 *factors, TInt size,
                                 const TU8 *jvt_list, const TU8 *fallback_list)
{
  TInt i, last = 8, next = 8;
  const TU8 *scan = (size == 16) ? simple_zigzag_scan : simple_zigzag_direct;
  if (!_get_bits1(gb)) { /* matrix not written, we use the predicted one */
    memcpy(factors, fallback_list, size * sizeof(TU8));
    //LOG_CONFIGURATION("[decode scaling list]: case 0\n");
  } else {
    for (i = 0; i < size; i++) {
      if (next) {
        next = (last + _get_se_golomb(gb)) & 0xff;
      }
      //LOG_CONFIGURATION("[decode scaling list]: case 1, i %d, next %d\n", i, next);
      if (!i && !next) { /* matrix not written, we use the preset one */
        memcpy(factors, jvt_list, size * sizeof(TU8));
        break;
      }
      last = factors[scan[i]] = next ? next : last;
      //LOG_CONFIGURATION("[decode scaling list]: case 1, i %d, last %d\n", i, last);
    }
  }
}

static void _decode_scaling_matrices(GetbitsContext *gb, SCodecVideoH264SPS *sps, SCodecVideoH264PPS *pps, TInt is_sps,
                                     TU8(*scaling_matrix4)[16], TU8(*scaling_matrix8)[64])
{
  TInt fallback_sps = !is_sps && sps->scaling_matrix_present;
  const TU8 *fallback[4] = {
    fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
    fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
    fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
    fallback_sps ? sps->scaling_matrix8[3] : default_scaling8[1]
  };
  if (_get_bits1(gb)) {
    //LOG_CONFIGURATION("[decode scaling matrics]: start\n");
    sps->scaling_matrix_present |= is_sps;
    _decode_scaling_list(gb, scaling_matrix4[0], 16, default_scaling4[0], fallback[0]); // Intra, Y
    _decode_scaling_list(gb, scaling_matrix4[1], 16, default_scaling4[0], scaling_matrix4[0]); // Intra, Cr
    _decode_scaling_list(gb, scaling_matrix4[2], 16, default_scaling4[0], scaling_matrix4[1]); // Intra, Cb
    _decode_scaling_list(gb, scaling_matrix4[3], 16, default_scaling4[1], fallback[1]); // Inter, Y
    _decode_scaling_list(gb, scaling_matrix4[4], 16, default_scaling4[1], scaling_matrix4[3]); // Inter, Cr
    _decode_scaling_list(gb, scaling_matrix4[5], 16, default_scaling4[1], scaling_matrix4[4]); // Inter, Cb
    if (is_sps || pps->transform_8x8_mode) {
      _decode_scaling_list(gb, scaling_matrix8[0], 64, default_scaling8[0], fallback[2]); // Intra, Y
      _decode_scaling_list(gb, scaling_matrix8[3], 64, default_scaling8[1], fallback[3]); // Inter, Y
      if (sps->chroma_format_idc == 3) {
        _decode_scaling_list(gb, scaling_matrix8[1], 64, default_scaling8[0], scaling_matrix8[0]); // Intra, Cr
        _decode_scaling_list(gb, scaling_matrix8[4], 64, default_scaling8[1], scaling_matrix8[3]); // Inter, Cr
        _decode_scaling_list(gb, scaling_matrix8[2], 64, default_scaling8[0], scaling_matrix8[1]); // Intra, Cb
        _decode_scaling_list(gb, scaling_matrix8[5], 64, default_scaling8[1], scaling_matrix8[4]); // Inter, Cb
      }
    }
  }
}

#if 0
static EECode _av_image_check_size(TUint w, TUint h, TInt log_offset)
{
  if ((TInt)w > 0 && (TInt)h > 0 && (w + 128) * (TU64)(h + 128) < INT_MAX / 8) {
    return EECode_OK;
  }
  LOG_ERROR("Picture size %ux%u is invalid\n", w, h);
  return EECode_BadParam;
}
#endif

SCodecVideoH264 *gfGetVideoCodecH264Parser(TU8 *p_data, TMemSize data_size, EECode &ret)
{
  GetbitsContext gb;
  gb.buffer = p_data;
  gb.buffer_end = p_data + data_size;
  gb.index = 0;
  gb.size_in_bits = 8 * data_size;
  SCodecVideoH264 *p_header = (SCodecVideoH264 *)DDBG_MALLOC(sizeof(SCodecVideoH264), "G264");
  if (DUnlikely(!p_header)) {
    LOG_FATAL("No memory\n");
    ret = EECode_NoMemory;
    return NULL;
  }
  memset(p_header, 0x0, sizeof(SCodecVideoH264));
  //TInt profile_idc=0, level_idc=0, constraint_set_flags = 0;
  TUint sps_id;
  TInt i, log2_max_frame_num_minus4;
  p_header->sps.profile_idc = _get_bits(&gb, 8);
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 0;   //constraint_set0_flag
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 1;   //constraint_set1_flag
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 2;   //constraint_set2_flag
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 3;   //constraint_set3_flag
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 4;   //constraint_set4_flag
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 5;   //constraint_set5_flag
  _get_bits(&gb, 2); // reserved
  p_header->sps.level_idc = _get_bits(&gb, 8);
  sps_id = _get_ue_golomb_31(&gb);
  p_header->common.profile_indicator = p_header->sps.profile_idc;
  p_header->common.level_indicator = p_header->sps.level_idc;
  if (sps_id >= DMAX_SPS_COUNT) {
    LOG_ERROR("sps_id (%d) out of range\n", sps_id);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->sps.time_offset_length = 24;
  //p_header->sps.profile_idc = profile_idc;
  //p_header->sps.constraint_set_flags = constraint_set_flags;
  //p_header->sps.level_idc= level_idc;
  p_header->sps.full_range = -1;
  memset(p_header->sps.scaling_matrix4, 16, sizeof(p_header->sps.scaling_matrix4));
  memset(p_header->sps.scaling_matrix8, 16, sizeof(p_header->sps.scaling_matrix8));
  p_header->sps.scaling_matrix_present = 0;
  p_header->sps.colorspace = EColorSpace_UNSPECIFIED;//  2; //AVCOL_SPC_UNSPECIFIED
  if (p_header->sps.profile_idc == 100 || p_header->sps.profile_idc == 110 ||
      p_header->sps.profile_idc == 122 || p_header->sps.profile_idc == 244 ||
      p_header->sps.profile_idc ==  44 || p_header->sps.profile_idc ==  83 ||
      p_header->sps.profile_idc ==  86 || p_header->sps.profile_idc == 118 ||
      p_header->sps.profile_idc == 128 || p_header->sps.profile_idc == 144) {
    p_header->sps.chroma_format_idc = _get_ue_golomb_31(&gb);
    LOG_CONFIGURATION("[parse sps]: chroma_format_idc %d\n", p_header->sps.chroma_format_idc);
    if ((TUint)p_header->sps.chroma_format_idc > 3U) {
      LOG_ERROR("chroma_format_idc %d is illegal\n", p_header->sps.chroma_format_idc);
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    } else if (p_header->sps.chroma_format_idc == 3) {
      p_header->sps.residual_color_transform_flag = _get_bits1(&gb);
      LOG_CONFIGURATION("[parse sps]: residual_color_transform_flag %d\n", p_header->sps.residual_color_transform_flag);
      if (p_header->sps.residual_color_transform_flag) {
        LOG_ERROR("separate color planes are not supported\n");
        ret = EECode_BadParam;
        goto gfGetVideoCodecH264Parser_failexit;
      }
    }
    p_header->sps.bit_depth_luma   = _get_ue_golomb(&gb) + 8;
    p_header->sps.bit_depth_chroma = _get_ue_golomb(&gb) + 8;
    LOG_CONFIGURATION("[parse sps]: bit_depth_luma %d, bit_depth_chroma %d\n", p_header->sps.bit_depth_luma, p_header->sps.bit_depth_chroma);
    if ((TUint)p_header->sps.bit_depth_luma > 14U || (TUint)p_header->sps.bit_depth_chroma > 14U || p_header->sps.bit_depth_luma != p_header->sps.bit_depth_chroma) {
      LOG_ERROR("illegal bit depth value (%d, %d)\n", p_header->sps.bit_depth_luma, p_header->sps.bit_depth_chroma);
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    }
    p_header->sps.transform_bypass = _get_bits1(&gb);
    LOG_CONFIGURATION("[parse sps]: transform_bypass %d\n", p_header->sps.transform_bypass);
    _decode_scaling_matrices(&gb, &p_header->sps, NULL, 1, p_header->sps.scaling_matrix4, p_header->sps.scaling_matrix8);
  } else {
    p_header->sps.chroma_format_idc = 1;
    p_header->sps.bit_depth_luma   = 8;
    p_header->sps.bit_depth_chroma = 8;
  }
  log2_max_frame_num_minus4 = _get_ue_golomb(&gb);
  if (log2_max_frame_num_minus4 < (DMIN_LOG2_MAX_FRAME_NUM - 4) ||
      log2_max_frame_num_minus4 > (DMAX_LOG2_MAX_FRAME_NUM - 4)) {
    LOG_ERROR("log2_max_frame_num_minus4 out of range (0-12): %d\n",
              log2_max_frame_num_minus4);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->sps.log2_max_frame_num = log2_max_frame_num_minus4 + 4;
  p_header->sps.poc_type = _get_ue_golomb_31(&gb);
  LOG_CONFIGURATION("[parse sps]: poc_type %d, log2_max_frame_num %d\n", p_header->sps.poc_type, p_header->sps.log2_max_frame_num);
  if (p_header->sps.poc_type == 0) { //FIXME #define
    TUint t = _get_ue_golomb(&gb);
    if (t > 12) {
      LOG_ERROR("log2_max_poc_lsb (%d) is out of range\n", t);
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    }
    p_header->sps.log2_max_poc_lsb = t + 4;
  } else if (p_header->sps.poc_type == 1) {//FIXME #define
    p_header->sps.delta_pic_order_always_zero_flag = _get_bits1(&gb);
    p_header->sps.offset_for_non_ref_pic = _get_se_golomb(&gb);
    p_header->sps.offset_for_top_to_bottom_field = _get_se_golomb(&gb);
    p_header->sps.poc_cycle_length                = _get_ue_golomb(&gb);
    if ((TUint)p_header->sps.poc_cycle_length >= DARRAY_ELEMS(p_header->sps.offset_for_ref_frame)) {
      LOG_ERROR("poc_cycle_length overflow %u\n", p_header->sps.poc_cycle_length);
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    }
    for (i = 0; i < p_header->sps.poc_cycle_length; i++) {
      p_header->sps.offset_for_ref_frame[i] = _get_se_golomb(&gb);
    }
  } else if (p_header->sps.poc_type != 2) {
    LOG_ERROR("illegal POC type %d\n", p_header->sps.poc_type);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->sps.ref_frame_count = _get_ue_golomb_31(&gb);
  if ((p_header->sps.ref_frame_count > (DMAX_PICTURE_COUNT - 2)) || ((TUint)p_header->sps.ref_frame_count > 16U)) {
    LOG_ERROR("too many reference frames, %d\n", p_header->sps.ref_frame_count);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  LOG_CONFIGURATION("[parse sps]: ref_frame_count %d\n", p_header->sps.ref_frame_count);
  p_header->sps.gaps_in_frame_num_allowed_flag = _get_bits1(&gb);
  p_header->sps.mb_width = _get_ue_golomb(&gb) + 1;
  p_header->sps.mb_height = _get_ue_golomb(&gb) + 1;
#if 0
  if ((TUint)p_header->sps.mb_width >= INT_MAX / 16 || (TUint)p_header->sps.mb_height >= INT_MAX / 16 ||
      _av_image_check_size(16 * p_header->sps.mb_width, 16 * p_header->sps.mb_height, 0)) {
    LOG_ERROR("mb_width/height overflow\n");
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
#endif
  p_header->common.max_width = 16 * p_header->sps.mb_width;
  p_header->common.max_height = 16 * p_header->sps.mb_height;
  LOG_CONFIGURATION("[parse sps]: picture width %d, height %d\n", p_header->common.max_width, p_header->common.max_height);
#if 0//TODO
  p_header->sps.frame_mbs_only_flag = _get_bits1(&gb);
  if (!p_header->sps.frame_mbs_only_flag) {
    p_header->sps.mb_aff = _get_bits1(&gb);
  } else {
    p_header->sps.mb_aff = 0;
  }
  p_header->sps.direct_8x8_inference_flag = _get_bits1(&gb);
  p_header->sps.crop = _get_bits1(&gb);
  if (p_header->sps.crop) {
    TInt crop_left   = _get_ue_golomb(&gb);
    TInt crop_right  = _get_ue_golomb(&gb);
    TInt crop_top    = _get_ue_golomb(&gb);
    TInt crop_bottom = _get_ue_golomb(&gb);
    TInt width  = 16 * p_header->sps.mb_width;
    TInt height = 16 * p_header->sps.mb_height * (2 - p_header->sps.frame_mbs_only_flag);
    TInt vsub   = (p_header->sps.chroma_format_idc == 1) ? 1 : 0;
    TInt hsub   = (p_header->sps.chroma_format_idc == 1 ||
                   p_header->sps.chroma_format_idc == 2) ? 1 : 0;
    TInt step_x = 1 << hsub;
    TInt step_y = (2 - p_header->sps.frame_mbs_only_flag) << vsub;
    /*if (crop_left & (0x1F >> (p_header->sps.bit_depth_luma > 8)) &&
        !(h->avctx->flags & CODEC_FLAG_UNALIGNED)) {
        crop_left &= ~(0x1F >> (p_header->sps.bit_depth_luma > 8));
        av_log(h->avctx, AV_LOG_WARNING,
               "Reducing left cropping to %d "
               "chroma samples to preserve alignment.\n",
               crop_left);
    }*/
    if (crop_left  > (TUint)INT_MAX / 4 / step_x ||
        crop_right > (TUint)INT_MAX / 4 / step_x ||
        crop_top   > (TUint)INT_MAX / 4 / step_y ||
        crop_bottom > (TUint)INT_MAX / 4 / step_y ||
        (crop_left + crop_right) * step_x >= width ||
        (crop_top  + crop_bottom) * step_y >= height
       ) {
      LOG_ERROR("crop values invalid %d %d %d %d / %d %d\n", crop_left, crop_right, crop_top, crop_bottom, width, height);
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    }
    p_header->sps.crop_left   = crop_left   * step_x;
    p_header->sps.crop_right  = crop_right  * step_x;
    p_header->sps.crop_top    = crop_top    * step_y;
    p_header->sps.crop_bottom = crop_bottom * step_y;
  } else {
    p_header->sps.crop_left   =
      p_header->sps.crop_right  =
        p_header->sps.crop_top    =
          p_header->sps.crop_bottom =
            p_header->sps.crop        = 0;
  }
  p_header->sps.vui_parameters_present_flag = _get_bits1(&gb);
  if (p_header->sps.vui_parameters_present_flag) {
    if (_decode_vui_parameters(&p_header->sps, &gb) < 0) {
      ret = EECode_BadParam;
      goto gfGetVideoCodecH264Parser_failexit;
    }
  }
  if (!p_header->sps.sar.den) {
    p_header->sps.sar.den = 1;
  }
  /*if(h->avctx->debug&FF_DEBUG_PICT_INFO){
      static const char csp[4][5] = { "Gray", "420", "422", "444" };
      av_log(h->avctx, AV_LOG_DEBUG, "sps:%u profile:%d/%d poc:%d ref:%d %dx%d %s %s crop:%d/%d/%d/%d %s %s %d/%d b%d reo:%d\n",
             sps_id, sps->profile_idc, sps->level_idc,
             sps->poc_type,
             sps->ref_frame_count,
             sps->mb_width, sps->mb_height,
             sps->frame_mbs_only_flag ? "FRM" : (sps->mb_aff ? "MB-AFF" : "PIC-AFF"),
             sps->direct_8x8_inference_flag ? "8B8" : "",
             sps->crop_left, sps->crop_right,
             sps->crop_top, sps->crop_bottom,
             sps->vui_parameters_present_flag ? "VUI" : "",
             csp[sps->chroma_format_idc],
             sps->timing_info_present_flag ? sps->num_units_in_tick : 0,
             sps->timing_info_present_flag ? sps->time_scale : 0,
             sps->bit_depth_luma,
             h->sps.bitstream_restriction_flag ? sps->num_reorder_frames : -1
             );
  }*/
  p_header->sps.isnew = 1;
  //PPS part
  TUint pps_id = _get_ue_golomb(&gb);
  TInt qp_bd_offset;
  TInt bits_left;
  if (pps_id >= DMAX_PPS_COUNT) {
    LOG_ERROR("pps_id %u out of range\n", pps_id);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->pps.sps_id = _get_ue_golomb_31(&gb);
  if ((unsigned)p_header->pps.sps_id >= DMAX_SPS_COUNT) {
    LOG_ERROR("sps_id %u out of range\n", p_header->pps.sps_id);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  qp_bd_offset = 6 * (p_header->sps.bit_depth_luma - 8);
  if (p_header->sps.bit_depth_luma > 14) {
    LOG_ERROR("Invalid luma bit depth=%d\n", p_header->sps.bit_depth_luma);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  } else if (p_header->sps.bit_depth_luma == 11 || p_header->sps.bit_depth_luma == 13) {
    LOG_ERROR("Unimplemented luma bit depth=%d\n", p_header->sps.bit_depth_luma);
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->pps.cabac             = _get_bits1(&gb);
  p_header->pps.pic_order_present = _get_bits1(&gb);
  p_header->pps.slice_group_count = _get_ue_golomb(&gb) + 1;
  if (p_header->pps.slice_group_count > 1) {
    p_header->pps.mb_slice_group_map_type = _get_ue_golomb(&gb);
    LOG_ERROR("FMO not supported\n");
    switch (p_header->pps.mb_slice_group_map_type) {
      case 0:
        break;
      case 2:
        break;
      case 3:
      case 4:
      case 5:
        break;
      case 6:
        break;
    }
  }
  p_header->pps.ref_count[0] = _get_ue_golomb(&gb) + 1;
  p_header->pps.ref_count[1] = _get_ue_golomb(&gb) + 1;
  if (p_header->pps.ref_count[0] - 1 > 32 - 1 || p_header->pps.ref_count[1] - 1 > 32 - 1) {
    LOG_ERROR("reference overflow (pps)\n");
    ret = EECode_BadParam;
    goto gfGetVideoCodecH264Parser_failexit;
  }
  p_header->pps.weighted_pred                        = _get_bits1(&gb);
  p_header->pps.weighted_bipred_idc                  = _get_bits(&gb, 2);
  p_header->pps.init_qp                              = _get_se_golomb(&gb) + 26 + qp_bd_offset;
  p_header->pps.init_qs                              = _get_se_golomb(&gb) + 26 + qp_bd_offset;
  p_header->pps.chroma_qp_index_offset[0]            = _get_se_golomb(&gb);
  p_header->pps.deblocking_filter_parameters_present = _get_bits1(&gb);
  p_header->pps.constrained_intra_pred               = _get_bits1(&gb);
  p_header->pps.redundant_pic_cnt_present            = _get_bits1(&gb);
  p_header->pps.transform_8x8_mode = 0;
  // contents of sps/pps can change even if id doesn't, so reinit
  //h->dequant_coeff_pps = -1;
  memcpy(p_header->pps.scaling_matrix4, p_header->sps.scaling_matrix4,
         sizeof(p_header->pps.scaling_matrix4));
  memcpy(p_header->pps.scaling_matrix8, p_header->sps.scaling_matrix8,
         sizeof(p_header->pps.scaling_matrix8));
:
  ibits_left = bit_length - _get_bits_count(&gb);
  if (bits_left > 0 && more_rbsp_data_in_pps(h, pps)) {
    p_header->pps.transform_8x8_mode = _get_bits1(&h->gb);
    _decode_scaling_matrices(&gb, &p_header->sps, &p_header->pps, 0,
                             p_header->pps.scaling_matrix4, p_header->pps.scaling_matrix8);
    // second_chroma_qp_index_offset
    p_header->pps.chroma_qp_index_offset[1] = _get_se_golomb(&gb);
  } else {
    p_header->pps.chroma_qp_index_offset[1] = p_header->pps.chroma_qp_index_offset[0];
  }
  build_qp_table(&p_header->pps, 0, p_header->pps.chroma_qp_index_offset[0], p_header->sps.bit_depth_luma);
  build_qp_table(&p_header->pps, 1, p_header->pps.chroma_qp_index_offset[1], p_header->sps.bit_depth_luma);
  if (p_header->pps.chroma_qp_index_offset[0] != p_header->pps.chroma_qp_index_offset[1])
  { p_header->pps.chroma_qp_diff = 1; }
  /*f (h->avctx->debug & FF_DEBUG_PICT_INFO) {
      av_log(h->avctx, AV_LOG_DEBUG,
             "pps:%u sps:%u %s slice_groups:%d ref:%d/%d %s qp:%d/%d/%d/%d %s %s %s %s\n",
             pps_id, p_header->pps.sps_id,
             p_header->pps.cabac ? "CABAC" : "CAVLC",
             p_header->pps.slice_group_count,
             p_header->pps.ref_count[0], p_header->pps.ref_count[1],
             p_header->pps.weighted_pred ? "weighted" : "",
             p_header->pps.init_qp, p_header->pps.init_qs, p_header->pps.chroma_qp_index_offset[0], p_header->pps.chroma_qp_index_offset[1],
             p_header->pps.deblocking_filter_parameters_present ? "LPAR" : "",
             p_header->pps.constrained_intra_pred ? "CONSTR" : "",
             p_header->pps.redundant_pic_cnt_present ? "REDU" : "",
             p_header->pps.transform_8x8_mode ? "8x8DCT" : "");
  }*/
#endif
  return p_header;
gfGetVideoCodecH264Parser_failexit:
  if (DLikely(p_header)) {
    DDBG_FREE(p_header, "G264");
  }
  return NULL;
}

TU8 gfGetH264SilceType(TU8 *pdata)
{
  TU8 slice_type = 0, first_mb_in_slice = 0;
  GetbitsContext gb;
  gb.buffer = pdata;
  gb.buffer_end = pdata + 16;
  gb.index = 0;
  gb.size_in_bits = 16 * 8;
  first_mb_in_slice = _get_ue_golomb(&gb);
  slice_type = _get_ue_golomb_31(&gb);
  if (DUnlikely(EH264SliceType_MaxValue < slice_type)) {
    LOG_FATAL("BAD slice_type %d, first_mb_in_slice %d\n", slice_type, first_mb_in_slice);
    return 0;
  }
  if (slice_type >= EH264SliceType_FieldOffset) {
    slice_type -= EH264SliceType_FieldOffset;
  }
  return slice_type;
}

