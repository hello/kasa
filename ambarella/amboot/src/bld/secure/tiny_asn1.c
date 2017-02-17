/**
 * tiny_asn1.c
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
 *
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
 */


#include "cryptography_if.h"

#ifndef DNOT_INCLUDE_C_HEADER
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <bldfunc.h>
#endif

#include "tiny_asn1.h"

int asn1_get_len(unsigned char** p, const unsigned char* end, unsigned int* len)
{
    if ((end - *p) < 1) {
        return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
    }

    if ((**p & 0x80) == 0) {
        *len = *(*p)++;
    } else {
        switch (**p & 0x7F) {
            case 1:
                if ((end - *p) < 2) {
                    return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
                }

                *len = (*p)[1];
                (*p) += 2;
                break;

            case 2:
                if ((end - *p) < 3) {
                    return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
                }

                *len = ((unsigned int)(*p)[1] << 8 ) | (*p)[2];
                (*p) += 3;
                break;

            case 3:
                if ((end - *p) < 4) {
                    return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
                }

                *len = ((unsigned int)(*p)[1] << 16 ) |( (unsigned int)(*p)[2] << 8  ) | (*p)[3];
                (*p) += 4;
                break;

            case 4:
                if ((end - *p) < 5) {
                    return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
                }

                *len = ((unsigned int)(*p)[1] << 24) | ((unsigned int)(*p)[2] << 16) | ((unsigned int)(*p)[3] << 8) | (*p)[4];
                (*p) += 5;
                break;

            default:
                return (-0x64);
        }
    }

    if (*len > (unsigned int) (end - *p)) {
        return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
    }

    return CRYPTO_ECODE_OK;
}

int asn1_get_tag(unsigned char** p, const unsigned char* end, unsigned int* len, int tag)
{
    if ((end - *p) < 1) {
        return CRYPTO_ECODE_ASN1_OUT_OF_DATA;
    }

    if (**p != tag) {
        return CRYPTO_ECODE_ASN1_UNEXPECTED_TAG;
    }

    (*p)++;

    return asn1_get_len(p, end, len);
}

int asn1_write_len(unsigned char** p, unsigned char* start, unsigned int len)
{
    if (len < 0x80) {
        if (*p - start < 1) {
            return CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL;
        }

        *--(*p) = (unsigned char) len;
        return (1);
    }

    if (len <= 0xFF) {
        if (*p - start < 2) {
            return CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL;
        }

        *--(*p) = (unsigned char) len;
        *--(*p) = 0x81;
        return (2);
    }

    if (*p - start < 3) {
        return CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL;
    }

    *--(*p) = len % 256;
    *--(*p) = (len / 256) % 256;
    *--(*p) = 0x82;

    return (3);
}

int asn1_write_tag(unsigned char **p, unsigned char *start, unsigned char tag)
{
    if (*p - start < 1) {
        return CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL;
    }

    *--(*p) = tag;

    return 1;
}


