/*******************************************************************************
 * hashes.cpp
 *
 * History:
 *  2014/07/03 - [Zhi He] create file
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

static TU32 fakeHashV3(TU8 *p)
{
    TU8 v1 = (p[2] ^ p[3]) + 101;
    TU8 v2 = (p[0] & p[1]) + 157;
    TU8 v3 = (p[6] + 181) ^ p[7];
    TU8 v4 = (p[4] | p[5]) + 37;

    v1 += (v2 ^ v3) + 3;
    v2 += (v3 ^ v4) + 7;
    v3 += (v4 ^ v1) + 11;
    v4 += (v1 ^ v2) + 13;

    TU32 ret = ((v1 << 24) | (v2 << 16) | (v3 << 8) | v4);

    return ret;
}

static TU32 fakeHashV6(TU8 *p)
{
    TU8 v1 = (p[2] ^ p[3]) + 83;
    TU8 v2 = (p[0] & p[1]) + 43;
    TU8 v3 = (p[6] + 167) ^ p[7];
    TU8 v4 = (p[4] | p[5]) + 131;

    v1 += (v2 ^ v3) + 19;
    v2 += (v3 ^ v4) + 29;
    v3 += (v4 ^ v1) + 53;
    v4 += (v1 ^ v2) + 3;

    TU32 ret = ((v1 << 24) | (v2 << 16) | (v3 << 8) | v4);

    return ret;
}

TU32 gfHashAlgorithmV3(TU8 *p, TU32 input_size)
{
    //shift and XOR params
    TU8 v1 = 'A', v2 = 'M', v3 = 'B', v4 = 'A';
    TU8 vv1 = 'B', vv2 = 'E', vv3 = 'S', vv4 = 'T';

    TU8 r1, r2, r3, r4;

    //check the input data's check sum, if fail, return a fake one
    r1 = (p[0] + p[2] + p[4]) % 257;
    r2 = (p[1] + p[3] + p[5]) % 263;

    if ((r1 != p[6]) || (r2 != p[7])) {
        return fakeHashV3(p);
    }

    //initial scramble data in buffer
    char scrambling_buffer[32] = {0x0};
    strcpy(scrambling_buffer, "anti12345678hack");
    TU32 size = 16;

    memcpy(scrambling_buffer + 4, p, 8);
    p = (TU8 *)scrambling_buffer;
    r1 = p[0], r2 = p[1], r3 = p[2], r4 = p[3];

    //initial scrambling
    v1 += p[4] + p[5] + p[8];
    v2 += (p[5] | p[6]) + p[9];
    v3 += (p[6] & p[7]) + p[10];
    v4 += (p[7] ^ p[8]) + p[11];

    vv1 += (p[4] ^ p[8]) + p[5];
    vv2 += (p[5] & p[9]) + p[6];
    vv3 += (p[6] | p[10]) + p[7];
    vv4 += (p[7] + p[11]) + p[8];

    do {
        //circular shift (v1 & 0x7)
        r1 = (r1 >> ((v1) & 0x7)) | (r1 << (8 - ((v1) & 0x7)));
        r2 = (r2 >> ((v2) & 0x7)) | (r2 << (8 - ((v2) & 0x7)));
        r3 = (r3 >> ((v3) & 0x7)) | (r3 << (8 - ((v3) & 0x7)));
        r4 = (r4 >> ((v4) & 0x7)) | (r4 << (8 - ((v4) & 0x7)));

        //XOR with vv1
        r1 = r1 ^ vv1;
        r2 = r2 ^ vv2;
        r3 = r3 ^ vv3;
        r4 = r4 ^ vv4;

        //update params, circular within 255
        v1 += 71;
        v2 += 7;
        v3 += 223;
        v4 += 109;
        vv1 += 67;
        vv2 += 199;
        vv3 += 41;
        vv4 += 179;

        size -= 4;
        p += 4;
        if (size < 4) {
            break;
        }

        //add input
        r1 += p[0];
        r2 += p[1];
        r3 += p[2];
        r4 += p[3];
    } while (1);

    TU32 ret = ((r1 << 24) | (r2 << 16) | (r3 << 8) | r4);

    return ret;
}

TU32 gfHashAlgorithmV4(TU8 *p_input, TU32 input_size)
{
    LOG_ERROR("gfHashAlgorithmV4 to do\n");
    return 0;
}

TU64 gfHashAlgorithmV5(TU8 *p, TU32 input_size)
{
    TU8 r1, r2;

    TU8 v1 = p[0], v2 = p[1], v3 = p[2], v4 = p[3];
    TU8 v5 = p[4], v6 = p[5], v7 = p[6], v8 = p[7];

    DASSERT(input_size >= 8);

    v1 = (((v1 + v2) & (v3 | v4)) ^ v5) + 17;
    v2 = (((v2 & v3) + (v4 ^ v5)) | v6) + 73;
    v3 = (((v3 | v4) ^ (v5 & v6)) ^ v7) + 157;
    v4 = (((v4 ^ v5) | (v6 + v7)) & v8) + 101;
    v5 = (((v5 + v6) & (v7 | v8)) ^ v1) + 211;
    v6 = (((v6 & v7) + (v8 ^ v1)) | v2) + 29;

    if (input_size > 8) {
        input_size -= 8;
        p += 8;
        while (input_size) {
            v1 += (v2 + 7) ^ p[0];
            v2 += (v3 + 47) ^ p[0];
            v3 ^= (v4 + 11) & p[0];
            v4 ^= (v5 + 23) & p[0];
            v5 += (v6 + 89) ^ p[0];
            v6 ^= (v1 + 71) | p[0];
            p ++;
            input_size --;
        }
    }

    r1 = ((v1 + (v3 ^ v5)) % 269) ^ (v2 + v4);
    r2 = ((v2 + (v4 ^ v6)) % 251) ^ (v3 + v5);

    return (((TU64)v1 << 56) | ((TU64)v2 << 48) | ((TU64)v3 << 40) | ((TU64)v4 << 32) | ((TU64)v5 << 24) | ((TU64)v6 << 16) | ((TU64)r1 << 8) | ((TU64)r2));
}

TU32 gfHashAlgorithmV6(TU8 *p, TU32 input_size)
{
    TU8 v1 = 0x34, v2 = 0xfc, v3 = 0x51, v4 = 0xe6;
    TU8 vv1 = 0xd7, vv2 = 0x4e, vv3 = 0x17, vv4 = 0x74;

    TU8 r1, r2, r3, r4;

    r1 = ((p[0] + (p[2] ^ p[4])) % 269) ^ (p[1] + p[3]);
    r2 = ((p[1] + (p[3] ^ p[5])) % 251) ^ (p[2] + p[4]);

    if ((r1 != p[6]) || (r2 != p[7])) {
        return fakeHashV6(p);
    }

    if (input_size < 16) {
        return fakeHashV6(p);
    }

    r1 = p[0], r2 = p[1], r3 = p[2], r4 = p[3];

    v1 += (p[4] ^ p[5]) & p[8];
    v2 += (p[5] | p[6]) ^ p[9];
    v3 += (p[6] & p[7]) | p[10];
    v4 += (p[7] ^ p[8]) + p[11];

    vv1 += (p[4] ^ p[8]) + p[5];
    vv2 += (p[5] & p[9]) | p[6];
    vv3 += (p[6] | p[10]) ^ p[7];
    vv4 += (p[7] + p[11]) & p[8];

    do {
        r1 = (r1 >> ((v1) & 0x7)) | (r1 << (8 - ((v1) & 0x7)));
        r2 = (r2 >> ((v2) & 0x7)) | (r2 << (8 - ((v2) & 0x7)));
        r3 = (r3 >> ((v3) & 0x7)) | (r3 << (8 - ((v3) & 0x7)));
        r4 = (r4 >> ((v4) & 0x7)) | (r4 << (8 - ((v4) & 0x7)));

        r1 = r1 ^ vv1;
        r2 = r2 ^ vv2;
        r3 = r3 ^ vv3;
        r4 = r4 ^ vv4;

        v1 += 113;
        v2 += 173;
        v3 += 199;
        v4 += 47;
        vv1 += 71;
        vv2 += 223;
        vv3 += 239;
        vv4 += 53;

        input_size -= 4;
        p += 4;
        if (input_size < 4) {
            break;
        }

        r1 += p[0];
        r2 += p[1];
        r3 += p[2];
        r4 += p[3];

    } while (1);

    TU32 ret = ((r1 << 24) | (r2 << 16) | (r3 << 8) | r4);

    return ret;
}

TU64 gfHashAlgorithmV7(TU8 *p, TU32 input_size)
{
    TU8 r1, r2;

    TU8 v1 = p[0], v2 = p[1], v3 = p[2], v4 = p[3];
    TU8 v5 = p[4], v6 = p[5], v7 = p[6], v8 = p[7];

    v1 = (((v1 ^ v2) & (v3 + v4)) | v5) + 113;
    v2 = (((v2 & v3) ^ (v4 | v5)) ^ v6) + 31;
    v3 = (((v3 | v4) + (v5 & v6)) & v7) + 17;
    v4 = (((v4 + v5) | (v6 ^ v7)) ^ v8) + 101;
    v5 = (((v5 & v6) ^ (v7 | v8)) | v1) + 43;
    v6 = (((v6 ^ v7) & (v8 + v1)) & v2) + 238;
    r1 = (((v7 + v8) | (v1 ^ v2)) + v3) + 29;
    r2 = (((v8 | v1) + (v2 & v3)) ^ v4) + 157;

    if (input_size > 8) {
        input_size -= 8;
        p += 8;
        while (input_size) {
            v1 ^= p[0] + 29;
            v2 ^= p[0] + 3;
            v3 ^= p[0] + 37;
            v4 ^= p[0] + 61;
            v5 ^= p[0] + 157;
            v6 ^= p[0] + 101;
            v7 ^= p[0] + 17;
            v8 ^= p[0] + 41;
            p ++;
            input_size --;
        }
    }

    return (((TU64)v1 << 56) | ((TU64)v2 << 48) | ((TU64)v3 << 40) | ((TU64)v4 << 32) | ((TU64)v5 << 24) | ((TU64)v6 << 16) | ((TU64)r1 << 8) | ((TU64)r2));
}

TU32 gfHashAlgorithmV8(TU8 *p, TU32 input_size)
{
    TU8 v1 = 0xdf, v2 = 0xa4, v3 = 0x67, v4 = 0x35;
    TU8 vv1 = 0xab, vv2 = 0x59, vv3 = 0xe6, vv4 = 0x9e;

    TU8 r1, r2, r3, r4;

    r1 = p[0], r2 = p[1], r3 = p[2], r4 = p[3];

    v1 += (p[4] & p[5]) ^ p[8];
    v2 += (p[5] | p[6]) & p[9];
    v3 += (p[6] ^ p[7]) | p[10];
    v4 += (p[7] ^ p[8]) ^ p[11];

    vv1 += (p[4] + p[8]) ^ p[5];
    vv2 += (p[5] | p[9]) & p[6];
    vv3 += (p[6] & p[10]) + p[7];
    vv4 += (p[7] + p[11]) ^ p[8];

    do {
        r1 = (r1 >> ((v1) & 0x7)) | (r1 << (8 - ((v1) & 0x7)));
        r2 = (r2 >> ((v2) & 0x7)) | (r2 << (8 - ((v2) & 0x7)));
        r3 = (r3 >> ((v3) & 0x7)) | (r3 << (8 - ((v3) & 0x7)));
        r4 = (r4 >> ((v4) & 0x7)) | (r4 << (8 - ((v4) & 0x7)));

        r1 = r1 ^ vv1;
        r2 = r2 ^ vv2;
        r3 = r3 ^ vv3;
        r4 = r4 ^ vv4;

        v1 += 101;
        v2 += 47;
        v3 += 71;
        v4 += 199;
        vv1 += 73;
        vv2 += 37;
        vv3 += 17;
        vv4 += 223;

        input_size -= 4;
        p += 4;
        if (input_size < 4) {
            break;
        }

        r1 += p[0];
        r2 += p[1];
        r3 += p[2];
        r4 += p[3];

    } while (1);

    TU32 ret = ((r1 << 24) | (r2 << 16) | (r3 << 8) | r4);

    return ret;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

