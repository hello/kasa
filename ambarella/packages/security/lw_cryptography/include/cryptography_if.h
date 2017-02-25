/*******************************************************************************
 * cryptography_if.h
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

#ifndef __CRYPTOGRAPHY_IF_H__
#define __CRYPTOGRAPHY_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

//#define DNOT_INCLUDE_C_HEADER
#ifdef DNOT_INCLUDE_C_HEADER
#define NULL ((void *) 0x0)
#endif

#define DCRYPT_LOG printf

#define DAMBOOT_PUT_STRING putstr
#define DAMBOOT_PUT_BYTE putbyte
#define DAMBOOT_PUT_DEC putdec

enum {
    CRYPTO_ECODE_OK = 0x00,
    CRYPTO_ECODE_INVALID_ARGUMENT = -0x01,
    CRYPTO_ECODE_INVALID_INPUT = -0x02,
    CRYPTO_ECODE_INVALID_OUTPUT = -0x03,
    CRYPTO_ECODE_INVALID_KEY = -0x04,
    CRYPTO_ECODE_SIGNATURE_VERIFY_FAIL = -0x05,
    CRYPTO_ECODE_GENERATE_KEY_FAIL = -0x06,
    CRYPTO_ECODE_SIGNATURE_INVALID_PADDING = -0x07,

    CRYPTO_ECODE_INTERNAL_ERROR = -0x10,
    CRYPTO_ECODE_ERROR_NO_MEMORY = -0x11,

    CRYPTO_ECODE_FILE_IO_ERROR = -0x20,
    CRYPTO_ECODE_BAD_INPUT_DATA = -0x21,
    CRYPTO_ECODE_INVALID_CHARACTER = -0x22,
    CRYPTO_ECODE_BUFFER_TOO_SMALL = -0x23,
    CRYPTO_ECODE_NEGATIVE_VALUE = -0x24,
    CRYPTO_ECODE_DIVISION_BY_ZERO = -0x25,
    CRYPTO_ECODE_NOT_ACCEPTABLE = -0x26,
    CRYPTO_ECODE_PK_PUBLIC_FAILED = -0x27,
    CRYPTO_ECODE_PK_PRIVATE_FAILED = -0x28,
    CRYPTO_ECODE_RANDOM_NUMBER_GENERATION_FAILED = -0x29,

    CRYPTO_ECODE_ASN1_OUT_OF_DATA = -0x30,
    CRYPTO_ECODE_ASN1_INVALID_LENGTH = -0x31,
    CRYPTO_ECODE_ASN1_BUFFER_TOO_SMALL = -0x32,
    CRYPTO_ECODE_ASN1_UNEXPECTED_TAG = -0x33,

    CRYPTO_ECODE_OID_NOT_FOUND = -0x40,
};

typedef enum {
    CRYPTO_SHA_TYPE_NONE = 0x0,
    CRYPTO_SHA_TYPE_SHA1 = 0x1,
    CRYPTO_SHA_TYPE_SHA224 = 0x2,
    CRYPTO_SHA_TYPE_SHA256 = 0x3,
    CRYPTO_SHA_TYPE_SHA384 = 0x4,
    CRYPTO_SHA_TYPE_SHA512 = 0x5,
} sha_type_t;

#define DCRYPTO_SHA1_DIGEST_LENGTH 20
#define DCRYPTO_SHA256_DIGEST_LENGTH 32
#define DCRYPTO_SHA384_DIGEST_LENGTH 48
#define DCRYPTO_SHA512_DIGEST_LENGTH 64

enum {
    CRYPTO_PKEY_TYPE_RSA = 0x1,
};

#define D_CLEAN_IF_FAILED(f) do { if ((ret = f) != CRYPTO_ECODE_OK) goto cleanup; } while(0)

#define D_BIG_NUMBER_MAX_LIMBS                             10000
#define D_BIG_NUMBER_WINDOW_SIZE                           6
#define D_BIG_NUMBER_MAX_SIZE                              1024
#define D_BIG_NUMBER_MAX_BITS                              (8 * D_BIG_NUMBER_MAX_SIZE)

#define RSA_PUBLIC      0
#define RSA_PRIVATE     1
#define RSA_PKCS_V15    0
#define RSA_PKCS_V21    1

#if (defined(_MSC_VER) && defined(_M_AMD64))
    typedef  long long TSINT;
    typedef unsigned long long TUINT;
    #define DHAVE_INT_64
#else
    #define DHAVE_DOUBLE_LONG_INT
#if (defined(__GNUC__) && (defined(__amd64__) || defined(__x86_64__)  || defined(__ppc64__) || defined(__powerpc64__) || \
      defined(__ia64__)  || defined(__alpha__) || (defined(__sparc__) && defined(__arch64__))  ||defined(__s390x__) || defined(__mips64) ) )
    typedef  long long TSINT;
    typedef unsigned long long TUINT;
    typedef unsigned int TUDBL __attribute__((mode(TI)));
    #define DHAVE_INT_64
#else
    typedef int TSINT;
    typedef unsigned int TUINT;
    typedef unsigned long long TUDBL;
#endif
#endif

typedef struct {
    int s;
    unsigned int n;
    TUINT *p;
} big_number_t;

typedef struct {
    int ver;
    unsigned int len;

    big_number_t N;
    big_number_t E;

    big_number_t D;
    big_number_t P;
    big_number_t Q;
    big_number_t DP;
    big_number_t DQ;
    big_number_t QP;

    big_number_t RN;
    big_number_t RP;
    big_number_t RQ;

    big_number_t Vi;
    big_number_t Vf;

    int padding;
    int hash_id;
} rsa_context_t;

void pseudo_random_scamble_sequence(unsigned char* p, unsigned int len);

void digest_sha256(const unsigned char* message, unsigned int len, unsigned char* digest);
#ifndef DNOT_INCLUDE_C_HEADER
int digest_sha256_file(const char *file, unsigned char* digest);
void* digest_sha256_init();
void digest_sha256_update(void* ctx, const unsigned char* message, unsigned int len);
void digest_sha256_final(void* ctx, unsigned char* digest);
#endif

void digest_md5(const unsigned char* message, unsigned int len, unsigned char* digest);
void digest_md5_hmac(const unsigned char* key, unsigned int keylen, const unsigned char* input, unsigned int ilen, unsigned char* output);
#ifndef DNOT_INCLUDE_C_HEADER
int digest_md5_file(const char* file, unsigned char* digest);
void* digest_md5_init();
void digest_md5_update(void* ctx, const unsigned char* message, unsigned int len);
void digest_md5_final(void* ctx, unsigned char* digest);
#endif

void rsa_init(rsa_context_t* ctx, int padding, int hash_id);
void rsa_free(rsa_context_t* ctx);
int rsa_check_pubkey(const rsa_context_t* ctx);
int rsa_check_privkey(const rsa_context_t* ctx);
int rsa_check_key_pair(const rsa_context_t* pub, const rsa_context_t* prv);
int rsa_gen_key(rsa_context_t* ctx, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng, unsigned int nbits, int exponent);
int rsa_sha256_sign(rsa_context_t* ctx, const unsigned char* hash, unsigned char* sig);
int rsa_sha256_verify(rsa_context_t* ctx, const unsigned char* hash, const unsigned char* sig);

extern int big_number_read_string(big_number_t* X, int radix, const char* s);
extern int big_number_write_string(const big_number_t* X, int radix, char* s, unsigned int* slen);

extern int big_number_write_binary(const big_number_t* X, unsigned char* buf, unsigned int buflen);
extern int big_number_read_binary(big_number_t* X, const unsigned char* buf, unsigned int buflen);

#ifndef DNOT_INCLUDE_C_HEADER
extern int big_number_write_file(const char* p, const big_number_t* X, int radix, void* f);
extern int big_number_read_file(big_number_t* X, int radix, void* f);
#endif

extern unsigned int big_number_size(const big_number_t* X);
extern unsigned int big_number_msb(const big_number_t *X);


#ifdef __cplusplus
}
#endif

#endif

