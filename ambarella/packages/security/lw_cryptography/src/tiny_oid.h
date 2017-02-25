/*******************************************************************************
 * tiny_oid.h
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

#ifndef __TINY_OID_H__
#define __TINY_OID_H__

/*
 * Top level OID tuples
 */
#define OID_ISO_MEMBER_BODIES           "\x2a"          /* {iso(1) member-body(2)} */
#define OID_ISO_IDENTIFIED_ORG          "\x2b"          /* {iso(1) identified-organization(3)} */
#define OID_ISO_CCITT_DS                "\x55"          /* {joint-iso-ccitt(2) ds(5)} */
#define OID_ISO_ITU_COUNTRY             "\x60"          /* {joint-iso-itu-t(2) country(16)} */

/*
 * ISO Identified organization OID parts
 */
#define OID_ORG_DOD                     "\x06"          /* {dod(6)} */
#define OID_ORG_OIW                     "\x0e"
#define OID_OIW_SECSIG                  OID_ORG_OIW "\x03"
#define OID_OIW_SECSIG_ALG              OID_OIW_SECSIG "\x02"
#define OID_OIW_SECSIG_SHA1             OID_OIW_SECSIG_ALG "\x1a"
#define OID_ORG_CERTICOM                "\x81\x04"  /* certicom(132) */
#define OID_CERTICOM                    OID_ISO_IDENTIFIED_ORG OID_ORG_CERTICOM
#define OID_ORG_TELETRUST               "\x24" /* teletrust(36) */
#define OID_TELETRUST                   OID_ISO_IDENTIFIED_ORG OID_ORG_TELETRUST

/*
 * ISO ITU OID parts
 */
#define OID_ORGANIZATION                "\x01"          /* {organization(1)} */
#define OID_ISO_ITU_US_ORG              OID_ISO_ITU_COUNTRY OID_COUNTRY_US OID_ORGANIZATION /* {joint-iso-itu-t(2) country(16) us(840) organization(1)} */

#define OID_ORG_GOV                     "\x65"          /* {gov(101)} */
#define OID_GOV                         OID_ISO_ITU_US_ORG OID_ORG_GOV /* {joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101)} */

#define OID_ORG_NETSCAPE                "\x86\xF8\x42"  /* {netscape(113730)} */
#define OID_NETSCAPE                    OID_ISO_ITU_US_ORG OID_ORG_NETSCAPE /* Netscape OID {joint-iso-itu-t(2) country(16) us(840) organization(1) netscape(113730)} */

/*
 * ISO Member bodies OID parts
 */
#define OID_COUNTRY_US                  "\x86\x48"      /* {us(840)} */
#define OID_ORG_RSA_DATA_SECURITY       "\x86\xf7\x0d"  /* {rsadsi(113549)} */
#define OID_RSA_COMPANY                 OID_ISO_MEMBER_BODIES OID_COUNTRY_US \
    OID_ORG_RSA_DATA_SECURITY /* {iso(1) member-body(2) us(840) rsadsi(113549)} */
#define OID_ORG_ANSI_X9_62              "\xce\x3d" /* ansi-X9-62(10045) */
#define OID_ANSI_X9_62                  OID_ISO_MEMBER_BODIES OID_COUNTRY_US \
    OID_ORG_ANSI_X9_62

/*
 * PKCS definition OIDs
 */

#define OID_PKCS                OID_RSA_COMPANY "\x01" /**< pkcs OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) 1 } */
#define OID_PKCS1               OID_PKCS "\x01" /**< pkcs-1 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 1 } */
#define OID_PKCS5               OID_PKCS "\x05" /**< pkcs-5 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 5 } */
#define OID_PKCS9               OID_PKCS "\x09" /**< pkcs-9 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 9 } */
#define OID_PKCS12              OID_PKCS "\x0c" /**< pkcs-12 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 12 } */

/*
 * PKCS#1 OIDs
 */
#define OID_PKCS1_RSA           OID_PKCS1 "\x01" /**< rsaEncryption OBJECT IDENTIFIER ::= { pkcs-1 1 } */
#define OID_PKCS1_SHA1          OID_PKCS1 "\x05" /**< sha1WithRSAEncryption ::= { pkcs-1 5 } */
#define OID_PKCS1_SHA224        OID_PKCS1 "\x0e" /**< sha224WithRSAEncryption ::= { pkcs-1 14 } */
#define OID_PKCS1_SHA256        OID_PKCS1 "\x0b" /**< sha256WithRSAEncryption ::= { pkcs-1 11 } */
#define OID_PKCS1_SHA384        OID_PKCS1 "\x0c" /**< sha384WithRSAEncryption ::= { pkcs-1 12 } */
#define OID_PKCS1_SHA512        OID_PKCS1 "\x0d" /**< sha512WithRSAEncryption ::= { pkcs-1 13 } */

#define OID_RSA_SHA_OBS         "\x2B\x0E\x03\x02\x1D"

/*
 * Digest algorithms
 */
#define OID_DIGEST_ALG_SHA1             OID_ISO_IDENTIFIED_ORG OID_OIW_SECSIG_SHA1 /**< id-sha1 OBJECT IDENTIFIER ::= { iso(1) identified-organization(3) oiw(14) secsig(3) algorithms(2) 26 } */
#define OID_DIGEST_ALG_SHA224           OID_GOV "\x03\x04\x02\x04" /**< id-sha224 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 4 } */
#define OID_DIGEST_ALG_SHA256           OID_GOV "\x03\x04\x02\x01" /**< id-sha256 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 1 } */

#define OID_DIGEST_ALG_SHA384           OID_GOV "\x03\x04\x02\x02" /**< id-sha384 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 2 } */

#define OID_DIGEST_ALG_SHA512           OID_GOV "\x03\x04\x02\x03" /**< id-sha512 OBJECT IDENTIFIER ::= { joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101) csor(3) nistalgorithm(4) hashalgs(2) 3 } */

#define OID_HMAC_SHA1                   OID_RSA_COMPANY "\x02\x07" /**< id-hmacWithSHA1 OBJECT IDENTIFIER ::= { iso(1) member-body(2) us(840) rsadsi(113549) digestAlgorithm(2) 7 } */

/*
 * SEC1 C.1
 *
 * prime-field OBJECT IDENTIFIER ::= { id-fieldType 1 }
 * id-fieldType OBJECT IDENTIFIER ::= { ansi-X9-62 fieldType(1)}
 */
#define OID_ANSI_X9_62_FIELD_TYPE   OID_ANSI_X9_62 "\x01"
#define OID_ANSI_X9_62_PRIME_FIELD  OID_ANSI_X9_62_FIELD_TYPE "\x01"

/*
 * ECDSA signature identifiers, from RFC 5480
 */
#define OID_ANSI_X9_62_SIG          OID_ANSI_X9_62 "\x04" /* signatures(4) */
#define OID_ANSI_X9_62_SIG_SHA2     OID_ANSI_X9_62_SIG "\x03" /* ecdsa-with-SHA2(3) */


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *asn1;
    unsigned int asn1_len;
    const char *name;
    const char *description;
} oid_descriptor_t;

int oid_get_oid_by_sha(sha_type_t sha_type, const char** oid, unsigned int* olen);
int oid_get_sha_type(const asn1_buf* oid, sha_type_t* sha_type);

#ifdef __cplusplus
}
#endif

#endif

