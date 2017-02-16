/**
 * tiny_asn1.c
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
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


