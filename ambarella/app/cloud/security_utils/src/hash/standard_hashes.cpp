/*******************************************************************************
 * standard_hashes.cpp
 *
 * History:
 *  2014/09/25 - [Zhi He] create file
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

#include "common_log.h"

#include "security_utils_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

// Constants for the transform:
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

// Basic MD5 functions:
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// Rotate "x" left "n" bits:
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// Other transforms:
#define FF(a, b, c, d, x, s, ac) { \
        (a) += F((b), (c), (d)) + (x) + (TU32)(ac); \
        (a) = ROTATE_LEFT((a), (s)); \
        (a) += (b); \
    }
#define GG(a, b, c, d, x, s, ac) { \
        (a) += G((b), (c), (d)) + (x) + (TU32)(ac); \
        (a) = ROTATE_LEFT((a), (s)); \
        (a) += (b); \
    }
#define HH(a, b, c, d, x, s, ac) { \
        (a) += H((b), (c), (d)) + (x) + (TU32)(ac); \
        (a) = ROTATE_LEFT((a), (s)); \
        (a) += (b); \
    }
#define II(a, b, c, d, x, s, ac) { \
        (a) += I((b), (c), (d)) + (x) + (TU32)(ac); \
        (a) = ROTATE_LEFT((a), (s)); \
        (a) += (b); \
    }

static void _trans64(const TU8 *input, TU32 &a0, TU32 &b0, TU32 &c0, TU32 &d0)
{
    TU32 a = a0, b = b0, c = c0, d = d0;

    TU32 x[16];
    for (TU32 i = 0; i < 16; i ++, input += 4) {
        DLER32(x[i], input);
    }

    FF(a, b, c, d, x[0], S11, 0xd76aa478);  // 1
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);  // 2
    FF(c, d, a, b, x[2], S13, 0x242070db);  // 3
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);  // 4
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);  // 5
    FF(d, a, b, c, x[5], S12, 0x4787c62a);  // 6
    FF(c, d, a, b, x[6], S13, 0xa8304613);  // 7
    FF(b, c, d, a, x[7], S14, 0xfd469501);  // 8
    FF(a, b, c, d, x[8], S11, 0x698098d8);  // 9
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);  // 10
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); // 11
    FF(b, c, d, a, x[11], S14, 0x895cd7be); // 12
    FF(a, b, c, d, x[12], S11, 0x6b901122); // 13
    FF(d, a, b, c, x[13], S12, 0xfd987193); // 14
    FF(c, d, a, b, x[14], S13, 0xa679438e); // 15
    FF(b, c, d, a, x[15], S14, 0x49b40821); // 16

    GG(a, b, c, d, x[1], S21, 0xf61e2562);  // 17
    GG(d, a, b, c, x[6], S22, 0xc040b340);  // 18
    GG(c, d, a, b, x[11], S23, 0x265e5a51); // 19
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);  // 20
    GG(a, b, c, d, x[5], S21, 0xd62f105d);  // 21
    GG(d, a, b, c, x[10], S22, 0x2441453);  // 22
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); // 23
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);  // 24
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);  // 25
    GG(d, a, b, c, x[14], S22, 0xc33707d6); // 26
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);  // 27
    GG(b, c, d, a, x[8], S24, 0x455a14ed);  // 28
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); // 29
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);  // 30
    GG(c, d, a, b, x[7], S23, 0x676f02d9);  // 31
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32

    HH(a, b, c, d, x[5], S31, 0xfffa3942);  // 33
    HH(d, a, b, c, x[8], S32, 0x8771f681);  // 34
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); // 35
    HH(b, c, d, a, x[14], S34, 0xfde5380c); // 36
    HH(a, b, c, d, x[1], S31, 0xa4beea44);  // 37
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);  // 38
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);  // 39
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); // 40
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); // 41
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);  // 42
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);  // 43
    HH(b, c, d, a, x[6], S34, 0x4881d05);   // 44
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);  // 45
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); // 46
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); // 47
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);  // 48

    II(a, b, c, d, x[0], S41, 0xf4292244);  // 49
    II(d, a, b, c, x[7], S42, 0x432aff97);  // 50
    II(c, d, a, b, x[14], S43, 0xab9423a7); // 51
    II(b, c, d, a, x[5], S44, 0xfc93a039);  // 52
    II(a, b, c, d, x[12], S41, 0x655b59c3); // 53
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);  // 54
    II(c, d, a, b, x[10], S43, 0xffeff47d); // 55
    II(b, c, d, a, x[1], S44, 0x85845dd1);  // 56
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);  // 57
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58
    II(c, d, a, b, x[6], S43, 0xa3014314);  // 59
    II(b, c, d, a, x[13], S44, 0x4e0811a1); // 60
    II(a, b, c, d, x[4], S41, 0xf7537e82);  // 61
    II(d, a, b, c, x[11], S42, 0xbd3af235); // 62
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);  // 63
    II(b, c, d, a, x[9], S44, 0xeb86d391);  // 64

    a0 += a;
    b0 += b;
    c0 += c;
    d0 += d;
}

void gfStandardHashMD5Oneshot(TU8 *input, TU32 size, TChar *&output)
{
    if (DUnlikely(!input || !size)) {
        LOG_FATAL("NULL params\n");
        return;
    }

    if (!output) {
        output = (TChar *) DDBG_MALLOC(32 + 4, "HSOT");
        if (DUnlikely(!output)) {
            LOG_FATAL("No memory\n");
            return;
        }
    }

    TU32 round = size >> 6, i = 0;
    TU32 remaing_size = size & 0x3f;
    TU8 last_round[128] = {0x0};

    TU32 a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;

    for (i = 0; i < round; i ++, input += 64) {
        _trans64((const TU8 *)input, a0, b0, c0, d0);
    }

    TU64 v = (TU64)size << 3;
    memcpy(last_round, input, remaing_size);
    last_round[remaing_size] = 0x80;

    if (56 > remaing_size) {
        last_round[56] = v & 0xff;
        last_round[57] = (v >> 8) & 0xff;
        last_round[58] = (v >> 16) & 0xff;
        last_round[59] = (v >> 24) & 0xff;
        last_round[60] = (v >> 32) & 0xff;
        last_round[61] = (v >> 40) & 0xff;
        last_round[62] = (v >> 48) & 0xff;
        last_round[63] = (v >> 56) & 0xff;
        input = last_round;
        _trans64((const TU8 *)input, a0, b0, c0, d0);
    } else {
        last_round[120] = v & 0xff;
        last_round[121] = (v >> 8) & 0xff;
        last_round[122] = (v >> 16) & 0xff;
        last_round[123] = (v >> 24) & 0xff;
        last_round[124] = (v >> 32) & 0xff;
        last_round[125] = (v >> 40) & 0xff;
        last_round[126] = (v >> 48) & 0xff;
        last_round[127] = (v >> 56) & 0xff;
        input = last_round;
        _trans64((const TU8 *)input, a0, b0, c0, d0);
        input += 64;
        _trans64((const TU8 *)input, a0, b0, c0, d0);
    }

    static const TChar hex[] = "0123456789abcdef";
    output[0] = hex[(a0 >> 4) & 0xf];
    output[1] = hex[(a0) & 0xf];
    output[2] = hex[(a0 >> 12) & 0xf];
    output[3] = hex[(a0 >> 8) & 0xf];
    output[4] = hex[(a0 >> 20) & 0xf];
    output[5] = hex[(a0 >> 16) & 0xf];
    output[6] = hex[(a0 >> 28) & 0xf];
    output[7] = hex[(a0 >> 24) & 0xf];

    output[8] = hex[(b0 >> 4) & 0xf];
    output[9] = hex[(b0) & 0xf];
    output[10] = hex[(b0 >> 12) & 0xf];
    output[11] = hex[(b0 >> 8) & 0xf];
    output[12] = hex[(b0 >> 20) & 0xf];
    output[13] = hex[(b0 >> 16) & 0xf];
    output[14] = hex[(b0 >> 28) & 0xf];
    output[15] = hex[(b0 >> 24) & 0xf];

    output[16] = hex[(c0 >> 4) & 0xf];
    output[17] = hex[(c0) & 0xf];
    output[18] = hex[(c0 >> 12) & 0xf];
    output[19] = hex[(c0 >> 8) & 0xf];
    output[20] = hex[(c0 >> 20) & 0xf];
    output[21] = hex[(c0 >> 16) & 0xf];
    output[22] = hex[(c0 >> 28) & 0xf];
    output[23] = hex[(c0 >> 24) & 0xf];

    output[24] = hex[(d0 >> 4) & 0xf];
    output[25] = hex[(d0) & 0xf];
    output[26] = hex[(d0 >> 12) & 0xf];
    output[27] = hex[(d0 >> 8) & 0xf];
    output[28] = hex[(d0 >> 20) & 0xf];
    output[29] = hex[(d0 >> 16) & 0xf];
    output[30] = hex[(d0 >> 28) & 0xf];
    output[31] = hex[(d0 >> 24) & 0xf];

    output[31] = 0x0;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

