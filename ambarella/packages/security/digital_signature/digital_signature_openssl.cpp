/*******************************************************************************
 * digital_signature_openssl.cpp
 *
 * History:
 *	2015/04/03 - [Zhi He] created file
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/engine.h>

#include "digital_signature_if.h"

#define BUFSIZE (1024 * 8)

int __pkey_ctrl_string_rsa_bits(EVP_PKEY_CTX *ctx, int bits)
{
    char bits_string[16] = {0};
    snprintf(bits_string, 16, "%d", bits);

    return EVP_PKEY_CTX_ctrl_str(ctx, "rsa_keygen_bits", bits_string);
}

int __pkey_ctrl_string_rsa_pubexp(EVP_PKEY_CTX *ctx, int pubexp)
{
    char bits_string[16] = {0};
    snprintf(bits_string, 16, "%d", pubexp);

    return EVP_PKEY_CTX_ctrl_str(ctx, "rsa_keygen_pubexp", bits_string);
}

EVP_PKEY_CTX* __pkey_get_ctx(const char *algname, ENGINE *e)
{
    EVP_PKEY_CTX *ctx = NULL;
    const EVP_PKEY_ASN1_METHOD *ameth;
    ENGINE *tmpeng = NULL;
    int pkey_id;

    ameth = EVP_PKEY_asn1_find_str(&tmpeng, algname, -1);

#ifndef OPENSSL_NO_ENGINE
    if (!ameth && e) {
        ameth = ENGINE_get_pkey_asn1_meth_str(e, algname, -1);
    }
#endif

    if (!ameth) {
        printf("[error]: Algorithm %s not found\n", algname);
        return NULL;
    }

    EVP_PKEY_asn1_get0_info(&pkey_id, NULL, NULL, NULL, NULL, ameth);
#ifndef OPENSSL_NO_ENGINE
    if (tmpeng) {
        ENGINE_finish(tmpeng);
    }
#endif
    ctx = EVP_PKEY_CTX_new_id(pkey_id, e);

    if (!ctx) {
        printf("[error]: EVP_PKEY_CTX_new_id fail\n");
        return NULL;
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        printf("[error]: EVP_PKEY_keygen_init fail\n");
        EVP_PKEY_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

static EVP_PKEY *__rsa_load_key(const char *file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if (file == NULL) {
        printf("[error]: no keyfile specified\n");
        return NULL;
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return NULL;
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return NULL;
    }

    pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey == NULL) {
        printf("[error]: PEM_read_bio_PrivateKey fail\n");
    }

    return (pkey);
}

static EVP_PKEY *__rsa_load_pubkey(const char *file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if (file == NULL) {
        printf("[error]: no keyfile specified\n");
        return NULL;
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return NULL;
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return NULL;
    }

    pkey = PEM_read_bio_PUBKEY(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey == NULL) {
        printf("[error]: PEM_read_bio_PUBKEY fail\n");
    }

    return (pkey);
}

int __dgst_do_fp(BIO *out, unsigned char *buf, BIO *bp, int sep, int binout,
          EVP_PKEY *key, unsigned char *sigin, int siglen,
          const char *sig_name, const char *md_name,
          const char *file, BIO *bmd)
{
    size_t len;
    int i;

    for (;;) {
        i = BIO_read(bp, (char *)buf, BUFSIZE);
        if (i < 0) {
            printf("[error]: Read Error in %s\n", file);
            return 1;
        }
        if (i == 0)
            break;
    }
    if (sigin) {
        EVP_MD_CTX *ctx;
        BIO_get_md_ctx(bp, &ctx);
        i = EVP_DigestVerifyFinal(ctx, sigin, (unsigned int)siglen);
        if (i > 0)
            printf("Verified OK\n");
        else if (i == 0) {
            printf("Verification Failure\n");
            return 1;
        } else {
            printf("[error]: Error Verifying Data\n");
            return 1;
        }
        return 0;
    }
    if (key) {
        EVP_MD_CTX *ctx;
        BIO_get_md_ctx(bp, &ctx);
        len = BUFSIZE;
        if (!EVP_DigestSignFinal(ctx, buf, &len)) {
            printf("[error]: Error Signing Data\n");
            return 1;
        }
    } else {
        len = BIO_gets(bp, (char *)buf, BUFSIZE);
        if ((int)len < 0) {
            printf("[error]: BIO_gets fail\n");
            return 1;
        }
    }

    if (binout)
        BIO_write(out, buf, len);
    else if (sep == 2) {
        for (i = 0; i < (int)len; i++)
            BIO_printf(out, "%02x", buf[i]);
        BIO_printf(out, " *%s\n", file);
    } else {
        if (sig_name) {
            BIO_puts(out, sig_name);
            if (md_name)
                BIO_printf(out, "-%s", md_name);
            BIO_printf(out, "(%s)= ", file);
        } else if (md_name)
            BIO_printf(out, "%s(%s)= ", md_name, file);
        else
            BIO_printf(out, "(%s)= ", file);
        for (i = 0; i < (int)len; i++) {
            if (sep && (i != 0))
                BIO_printf(out, ":");
            BIO_printf(out, "%02x", buf[i]);
        }
        BIO_printf(out, "\n");
    }
    return 0;
}

int generate_rsa_key(char* output_file, int bits, int pubexp, int key_format)
{
    int ret = 0;
    BIO *out = NULL;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX* ctx = NULL;

    if (!output_file) {
        printf("[error]: NULL output_file\n");
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto generate_rsa_key_exit;
    }

    ctx = __pkey_get_ctx("RSA", NULL);
    if (!ctx) {
        printf("[error]: __pkey_get_ctx fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    out = BIO_new_file(output_file, "wb");
    if (!out) {
        printf("[error]: BIO_new_file(%s) fail\n", output_file);
        ret = SU_ECODE_INVALID_OUTPUT;
        goto generate_rsa_key_exit;
    }

    ret = __pkey_ctrl_string_rsa_bits(ctx, bits);
    ret = __pkey_ctrl_string_rsa_pubexp(ctx, pubexp);

    ret = EVP_PKEY_keygen(ctx, &pkey);
    if (ret <= 0) {
        printf("[error]: EVP_PKEY_keygen(%d) fail\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    ret = PEM_write_bio_PrivateKey(out, pkey, NULL, NULL, 0, NULL, NULL);
    if (ret <= 0) {
        printf("[error]: PEM_write_bio_PrivateKey(%d) fail\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto generate_rsa_key_exit;
    }

    ret = SU_ECODE_OK;

generate_rsa_key_exit:
    if (pkey) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
    }
    if (ctx) {
        EVP_PKEY_CTX_free(ctx);
        ctx = NULL;
    }
    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    return ret;
}

int get_public_rsa_key(char* output_file, char* input_file)
{
    EVP_PKEY * pkey = NULL;
    RSA *rsa = NULL;
    BIO *out = NULL;
    int ret = 0;

    if (!output_file || !input_file) {
        printf("[error]: NULL input filename or NULL output filename\n");
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto get_public_rsa_key_exit;
    }

    pkey = __rsa_load_key(input_file);
    if (!pkey) {
        printf("[error]: __rsa_load_key(%s) fail\n", input_file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    pkey = NULL;

    out = BIO_new(BIO_s_file());
    if (!out) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = BIO_write_filename(out, output_file);
    if (0 >= ret) {
        printf("[error]: BIO_write_filename fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = PEM_write_bio_RSA_PUBKEY(out, rsa);
    if (0 >= ret) {
        printf("[error]: PEM_write_bio_RSA_PUBKEY fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto get_public_rsa_key_exit;
    }

    ret = SU_ECODE_OK;

get_public_rsa_key_exit:
    if (pkey) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
    }
    if (rsa) {
        RSA_free(rsa);
        rsa = NULL;
    }
    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    return ret;
}

int signature_file(char* file, char* digital_signature, char* keyfile, int sha_type)
{
    const EVP_MD *md = NULL;
    BIO *in = NULL, *inp = NULL;
    BIO *out = NULL;
    EVP_PKEY *sigkey = NULL;
    BIO* bmd = NULL;
    EVP_MD_CTX *mctx = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    unsigned char* buf = NULL;
    int ret = 0;

    char sha_name[32] = {0};

    OpenSSL_add_all_algorithms();

    if (SHA_TYPE_SHA256 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha256");
    } else if (SHA_TYPE_SHA1 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha1");
    } else if (SHA_TYPE_SHA384 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha384");
    } else if (SHA_TYPE_SHA512 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha512");
    } else {
        printf("[error]: bad sha_type %d\n", sha_type);
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto signature_file_exit;
    }

    md = EVP_get_digestbyname(sha_name);
    if (!md) {
        printf("[error]: EVP_get_digestbyname(%s) fail\n", sha_name);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    in = BIO_new(BIO_s_file());
    if (!in) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = BIO_read_filename(in, file);
    if (0 >= ret) {
        printf("[error]: BIO_read_filename(%s) fail\n", file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    out = BIO_new_file(digital_signature, "wb");
    if (!out) {
        printf("[error]: BIO_new_file(%s)\n", digital_signature);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    sigkey = __rsa_load_key(keyfile);
    if (!sigkey) {
        printf("[error]: __rsa_load_key(%s)\n", keyfile);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    bmd = BIO_new(BIO_f_md());
    if (!bmd) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = BIO_get_md_ctx(bmd, &mctx);
    if (0 >= ret) {
        printf("[error]: BIO_get_md_ctx() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = EVP_DigestSignInit(mctx, &pctx, md, NULL, sigkey);
    if (0 >= ret) {
        printf("[error]: EVP_DigestSignInit() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    inp = BIO_push(bmd, in);

    if ((buf = (unsigned char *)OPENSSL_malloc(BUFSIZE)) == NULL) {
        printf("[error]: no memory() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = __dgst_do_fp(out, buf, inp, 0, 1, sigkey, NULL, 0, NULL, NULL, file, bmd);
    if (ret) {
        printf("[error]: __dgst_do_fp() fail, ret %d\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto signature_file_exit;
    }

    ret = SU_ECODE_OK;

signature_file_exit:

    if (buf != NULL) {
        OPENSSL_cleanse(buf, BUFSIZE);
        OPENSSL_free(buf);
        buf = NULL;
    }

    if (in != NULL) {
        BIO_free(in);
        in = NULL;
    }

    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    if (sigkey) {
        EVP_PKEY_free(sigkey);
        sigkey = NULL;
    }

    if (bmd != NULL) {
        BIO_free(bmd);
    }

    return ret;
}

int verify_signature(char* file, char* digital_signature, char* keyfile, int sha_type)
{
    int ret = 0;
    unsigned char *sigbuf = NULL;
    int siglen = 0;
    unsigned char *buf = NULL;
    BIO *out = NULL;

    const EVP_MD *md = NULL;

    EVP_PKEY *sigkey = NULL;
    EVP_MD_CTX *mctx = NULL;
    EVP_PKEY_CTX *pctx = NULL;
    BIO* bmd = NULL;
    BIO *in = NULL, *inp;
    BIO *sigbio = NULL;

    char sha_name[32] = {0};

    OpenSSL_add_all_algorithms();

    if (SHA_TYPE_SHA256 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha256");
    } else if (SHA_TYPE_SHA1 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha1");
    } else if (SHA_TYPE_SHA384 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha384");
    } else if (SHA_TYPE_SHA512 == sha_type) {
        snprintf(sha_name, 32, "%s", "sha512");
    } else {
        printf("[error]: bad sha_type %d\n", sha_type);
        ret = SU_ECODE_INVALID_ARGUMENT;
        goto verify_signature_exit;
    }

    md = EVP_get_digestbyname(sha_name);
    if (!md) {
        printf("[error]: EVP_get_digestbyname(%s) fail\n", sha_name);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    in = BIO_new(BIO_s_file());
    if (!in) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = BIO_read_filename(in, file);
    if (0 >= ret) {
        printf("[error]: BIO_read_filename(%s) fail\n", file);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    out = BIO_new_fp(stdout, BIO_NOCLOSE);

    sigkey = __rsa_load_pubkey(keyfile);
    if (!sigkey) {
        printf("[error]: __rsa_load_pubkey(%s)\n", keyfile);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    bmd = BIO_new(BIO_f_md());
    if (!bmd) {
        printf("[error]: no memory, BIO_new fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = BIO_get_md_ctx(bmd, &mctx);
    if (0 >= ret) {
        printf("[error]: BIO_get_md_ctx() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = EVP_DigestVerifyInit(mctx, &pctx, md, NULL, sigkey);
    if (0 >= ret) {
        printf("[error]: EVP_DigestVerifyInit() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    sigbio = BIO_new_file(digital_signature, "rb");
    if (!sigbio) {
        printf("[error]: no memory, BIO_new_file() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    siglen = EVP_PKEY_size(sigkey);
    sigbuf = (unsigned char*) OPENSSL_malloc(siglen);
    if (!sigbuf) {
        printf("[error]: Out of memory\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    siglen = BIO_read(sigbio, sigbuf, siglen);
    if (siglen <= 0) {
        printf("[error]: Error reading signature file %s\n", digital_signature);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    inp = BIO_push(bmd, in);

    if ((buf = (unsigned char *)OPENSSL_malloc(BUFSIZE)) == NULL) {
        printf("[error]: no memory() fail\n");
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    ret = __dgst_do_fp(out, buf, inp, 0, 1, sigkey, sigbuf, siglen, NULL, NULL, file, bmd);
    if (ret) {
        printf("[error]: __dgst_do_fp() fail, ret %d\n", ret);
        ret = SU_ECODE_ERROR_FROM_LIB;
        goto verify_signature_exit;
    }

    if (ret) {
        printf("verify fail\n");
        ret = SU_ECODE_SIGNATURE_VERIFY_FAIL;
        goto verify_signature_exit;
    }

    printf("verify OK\n");
    ret = SU_ECODE_OK;

verify_signature_exit:

    if (buf != NULL) {
        OPENSSL_cleanse(buf, BUFSIZE);
        OPENSSL_free(buf);
        buf = NULL;
    }

    if (in != NULL) {
        BIO_free(in);
        in = NULL;
    }

    if (out) {
        BIO_free_all(out);
        out = NULL;
    }

    if (sigkey) {
        EVP_PKEY_free(sigkey);
        sigkey = NULL;
    }

    if (bmd != NULL) {
        BIO_free(bmd);
    }

    if (sigbio) {
        BIO_free(sigbio);
        sigbio = NULL;
    }

    return ret;
}

