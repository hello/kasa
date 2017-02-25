/*******************************************************************************
 * crypt_utils.c
 *
 * History:
 *  2015/06/16 - [Zhi He] create utils file for cryptography library
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#include "cryptography_if.h"

#ifndef DNOT_INCLUDE_C_HEADER
#include <stdio.h>
#include <stdlib.h>
#endif

#define DCIRCULAR_SHIFT_8(x, s) ((x >> s) | (x << (8 - s)))

void pseudo_random_scamble_sequence(unsigned char* p, unsigned int len)
{
    unsigned int i = 0;
    unsigned char r[8] = {31, 47, 5, 19, 149, 71, 101, 163};
    unsigned char v[8] = {211, 7, 59, 127, 79, 73, 11, 23};
    unsigned char* pp = NULL;

    unsigned char t1, t2;

    if (7 < len) {
        for (i = 0; i < 8; i ++) {
            t1 = (v[i] ^ p[i]);
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }
    } else {
        for (i = 0; i < len; i ++) {
            t1 = (v[i] ^ p[i]);
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }

        for (; i < 8; i ++) {
            t1 = v[i];
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }
    }

    t2 = (v[7] & 0x7);
    i = 0;
    pp = p;

    while (len > 8) {
        pp[0] = (((v[0] ^ v[1]) & (r[4] + r[5])) | DCIRCULAR_SHIFT_8(pp[0], t2)) + 113;
        t2 = pp[0] & 0x7;
        pp[1] = (((v[1] & v[2]) ^ (r[5] | r[6])) ^ DCIRCULAR_SHIFT_8(pp[1], t2)) + 31;
        t2 = pp[1] & 0x7;
        pp[2] = (((v[2] | v[3]) + (r[6] & r[7])) ^ DCIRCULAR_SHIFT_8(pp[2], t2)) + 17;
        t2 = pp[2] & 0x7;
        pp[3] = (((v[3] + v[4]) | (r[7] ^ r[0])) ^ DCIRCULAR_SHIFT_8(pp[3], t2)) + 101;
        t2 = pp[3] & 0x7;
        pp[4] = (((v[4] & v[5]) ^ (r[0] | r[1])) | DCIRCULAR_SHIFT_8(pp[4], t2)) + 43;
        t2 = pp[4] & 0x7;
        pp[5] = (((v[5] ^ v[6]) | (r[1] + r[2])) ^ DCIRCULAR_SHIFT_8(pp[5], t2)) + 238;
        t2 = pp[5] & 0x7;
        pp[6] = (((v[6] + v[7]) | (r[2] ^ r[3])) + DCIRCULAR_SHIFT_8(pp[6], t2)) + 29;
        t2 = pp[6] & 0x7;
        pp[7] = (((v[7] | v[0]) + (r[3] & r[4])) ^ DCIRCULAR_SHIFT_8(pp[7], t2)) + 157;
        t2 = pp[7] & 0x7;

        v[0] = (v[1] ^ r[6]) + 83 + i + pp[7];
        r[0] = (v[6] & r[1]) + 109 + i + pp[6];
        v[1] = ((v[2] | r[5]) ^ 149) + i + pp[5];
        r[1] = ((v[5] + r[2]) & 197) + i + pp[4];

        v[2] = (v[3] ^ r[0]) + 59 + i + pp[3];
        r[2] = (v[0] & r[3]) + 61 + i + pp[2];
        v[3] = ((v[4] | r[7]) ^ 67) + i + pp[1];
        r[3] = ((v[7] + r[4]) & 37) + i + pp[0];

        v[4] = (v[2] ^ r[6]) + 179 + i + pp[7];
        r[4] = (v[6] & r[2]) + 131 + i + pp[5];
        v[5] = ((v[1] | r[7]) ^ 71) + i + pp[3];
        r[5] = ((v[7] + r[1]) & 29) + i + pp[1];

        v[6] = (v[5] ^ r[3]) + 179 + i + pp[0];
        r[6] = (v[3] & r[5]) + 131 + i + pp[2];
        v[7] = ((v[4] | r[2]) ^ 71) + i + pp[4];
        r[7] = ((v[2] + r[4]) & 29) + i + pp[6];

        len -= 8;
        pp += 8;
        i ++;
    }

    i = 0;

    for (i = 0; i < len; i ++) {
        pp[i] = pp[i] ^ (((v[i] + r[(8 - i) & 0x7]) & (v[(8 - i) & 0x7] | r[i])) + 181);
    }

    return;
}

