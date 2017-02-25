/*******************************************************************************
 * tiny_oid.c
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <bldfunc.h>
#endif

#include "tiny_asn1.h"
#include "tiny_oid.h"

#define ADD_LEN(s)      s, OID_SIZE(s)

#define FN_OID_TYPED_FROM_ASN1( TYPE_T, NAME, LIST )                        \
    static const TYPE_T * oid_ ## NAME ## _from_asn1( const asn1_buf *oid )     \
    {                                                                           \
        const TYPE_T *p = LIST;                                                 \
        const oid_descriptor_t *cur = (const oid_descriptor_t *) p;             \
        if( p == NULL || oid == NULL ) return( NULL );                          \
        while( cur->asn1 != NULL ) {                                            \
            if( cur->asn1_len == oid->len &&                                    \
                memcmp( cur->asn1, oid->p, oid->len ) == 0 ) {                  \
                return( p );                                                    \
            }                                                                   \
            p++;                                                                \
            cur = (const oid_descriptor_t *) p;                                 \
        }                                                                       \
        return( NULL );                                                         \
    }

#define FN_OID_GET_DESCRIPTOR_ATTR1(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1) \
    int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1 )                  \
    {                                                                       \
        const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );        \
        if( data == NULL ) return( CRYPTO_ECODE_OID_NOT_FOUND );            \
        *ATTR1 = data->descriptor.ATTR1;                                    \
        return( 0 );                                                        \
    }

#define FN_OID_GET_ATTR1(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1) \
    int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1 )                  \
    {                                                                       \
        const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );        \
        if( data == NULL ) return( CRYPTO_ECODE_OID_NOT_FOUND );            \
        *ATTR1 = data->ATTR1;                                               \
        return( 0 );                                                        \
    }

#define FN_OID_GET_ATTR2(FN_NAME, TYPE_T, TYPE_NAME, ATTR1_TYPE, ATTR1,     \
                         ATTR2_TYPE, ATTR2)                                 \
int FN_NAME( const asn1_buf *oid, ATTR1_TYPE * ATTR1, ATTR2_TYPE * ATTR2 )  \
{                                                                           \
    const TYPE_T *data = oid_ ## TYPE_NAME ## _from_asn1( oid );            \
    if( data == NULL ) return( CRYPTO_ECODE_OID_NOT_FOUND );                \
    *ATTR1 = data->ATTR1;                                                   \
    *ATTR2 = data->ATTR2;                                                   \
    return( 0 );                                                            \
}

#define FN_OID_GET_OID_BY_ATTR1(FN_NAME, TYPE_T, LIST, ATTR1_TYPE, ATTR1)   \
    int FN_NAME( ATTR1_TYPE ATTR1, const char **oid, unsigned int *olen )             \
    {                                                                           \
        const TYPE_T *cur = LIST;                                               \
        while( cur->descriptor.asn1 != NULL ) {                                 \
            if( cur->ATTR1 == ATTR1 ) {                                         \
                *oid = cur->descriptor.asn1;                                    \
                *olen = cur->descriptor.asn1_len;                               \
                return( 0 );                                                    \
            }                                                                   \
            cur++;                                                              \
        }                                                                       \
        return( CRYPTO_ECODE_OID_NOT_FOUND );                                   \
    }

#define FN_OID_GET_OID_BY_ATTR2(FN_NAME, TYPE_T, LIST, ATTR1_TYPE, ATTR1,   \
                                ATTR2_TYPE, ATTR2)                          \
int FN_NAME( ATTR1_TYPE ATTR1, ATTR2_TYPE ATTR2, const char **oid ,         \
             unsigned int *olen )                                                 \
{                                                                           \
    const TYPE_T *cur = LIST;                                               \
    while( cur->descriptor.asn1 != NULL ) {                                 \
        if( cur->ATTR1 == ATTR1 && cur->ATTR2 == ATTR2 ) {                  \
            *oid = cur->descriptor.asn1;                                    \
            *olen = cur->descriptor.asn1_len;                               \
            return( 0 );                                                    \
        }                                                                   \
        cur++;                                                              \
    }                                                                       \
    return( CRYPTO_ECODE_OID_NOT_FOUND );                                   \
}

typedef struct {
    oid_descriptor_t    descriptor;
    sha_type_t           sha_type;
} oid_sha_type_t;

static const oid_sha_type_t oid_sha_type[] = {
    {
        { ADD_LEN( OID_DIGEST_ALG_SHA1 ),      "id-sha1",      "SHA-1" },
        CRYPTO_SHA_TYPE_SHA1,
    },
    {
        { ADD_LEN( OID_DIGEST_ALG_SHA224 ),    "id-sha224",    "SHA-224" },
        CRYPTO_SHA_TYPE_SHA224,
    },
    {
        { ADD_LEN( OID_DIGEST_ALG_SHA256 ),    "id-sha256",    "SHA-256" },
        CRYPTO_SHA_TYPE_SHA256,
    },
    {
        { ADD_LEN( OID_DIGEST_ALG_SHA384 ),    "id-sha384",    "SHA-384" },
        CRYPTO_SHA_TYPE_SHA384,
    },
    {
        { ADD_LEN( OID_DIGEST_ALG_SHA512 ),    "id-sha512",    "SHA-512" },
        CRYPTO_SHA_TYPE_SHA512,
    },
    {
        { NULL, 0, NULL, NULL },
        CRYPTO_SHA_TYPE_NONE,
    },
};

FN_OID_TYPED_FROM_ASN1(oid_sha_type_t, sha_type, oid_sha_type);
FN_OID_GET_ATTR1(oid_get_sha_type, oid_sha_type_t, sha_type, sha_type_t, sha_type);
FN_OID_GET_OID_BY_ATTR1(oid_get_oid_by_sha, oid_sha_type_t, oid_sha_type, sha_type_t, sha_type);


