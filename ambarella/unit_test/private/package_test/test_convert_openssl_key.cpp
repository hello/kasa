/*******************************************************************************
 * test_convert_openssl_key.cpp
 *
 * History:
 *  2016/12/01 - [Zhi He] created file
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

typedef struct
{
    unsigned char convert_public_key;
    unsigned char convert_private_key;
    unsigned char reserved1;
    unsigned char reserved2;

    char *input_file;
    char *output_file;
} tcok_context;

static char * __remove_heading_zerozero(char *p, int length)
{
    char * t = p;
    while (2 < length) {
        if ((t[0] == '0') && t[1] == '0') {
            t += 2;
            length -= 2;
            continue;
        }
        break;
    }
    return t;
}

static int __dump_bignumber(char *hex_text, int max_string_buffer_length, BIGNUM *big_num)
{
    const char hex_text_table[] = "0123456789ABCDEF";
    int i = big_num->dmax - 1;
    BN_ULONG value = 0;
    char *p_cur = hex_text;
    int string_length = 0;

#if __WORDSIZE == 64
    i = big_num->dmax - 1;
    while (0 <= i) {
        value = big_num->d[i];
        p_cur[0] = hex_text_table[(value >> 60) & 0x0f];
        p_cur[1] = hex_text_table[(value >> 56) & 0x0f];
        p_cur[2] = hex_text_table[(value >> 52) & 0x0f];
        p_cur[3] = hex_text_table[(value >> 48) & 0x0f];
        p_cur[4] = hex_text_table[(value >> 44) & 0x0f];
        p_cur[5] = hex_text_table[(value >> 40) & 0x0f];
        p_cur[6] = hex_text_table[(value >> 36) & 0x0f];
        p_cur[7] = hex_text_table[(value >> 32) & 0x0f];
        p_cur[8] = hex_text_table[(value >> 28) & 0x0f];
        p_cur[9] = hex_text_table[(value >> 24) & 0x0f];
        p_cur[10] = hex_text_table[(value >> 20) & 0x0f];
        p_cur[11] = hex_text_table[(value >> 16) & 0x0f];
        p_cur[12] = hex_text_table[(value >> 12) & 0x0f];
        p_cur[13] = hex_text_table[(value >> 8) & 0x0f];
        p_cur[14] = hex_text_table[(value >> 4) & 0x0f];
        p_cur[15] = hex_text_table[(value) & 0x0f];
        p_cur += 16;
        string_length += 16;
        i --;
        if ((string_length + 16) > max_string_buffer_length) {
            printf("must have errors, the length exceed!\n");
            break;
        }
    }
#else
    i = big_num->dmax - 1;
    while (0 <= i) {
        value = big_num->d[i];
        p_cur[0] = hex_text_table[(value >> 28) & 0x0f];
        p_cur[1] = hex_text_table[(value >> 24) & 0x0f];
        p_cur[2] = hex_text_table[(value >> 20) & 0x0f];
        p_cur[3] = hex_text_table[(value >> 16) & 0x0f];
        p_cur[4] = hex_text_table[(value >> 12) & 0x0f];
        p_cur[5] = hex_text_table[(value >> 8) & 0x0f];
        p_cur[6] = hex_text_table[(value >> 4) & 0x0f];
        p_cur[7] = hex_text_table[(value) & 0x0f];
        p_cur += 8;
        string_length += 8;
        i --;
        if ((string_length + 8) > max_string_buffer_length) {
            printf("must have errors, the length exceed!\n");
            break;
        }
    }
#endif

    return string_length;
}

static int dump_rsa_pem_privkey(const char *file, const char *output_file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if ((file == NULL) || (output_file == NULL)) {
        printf("[error]: no keyfile specified\n");
        return (-1);
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return (-2);
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return (-3);
    }

    pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey) {
        char max_string_buffer[2048 + 2] = {0};
        int string_length = 0;
        char *p_num = NULL;

        FILE *p_out_file = fopen(output_file, "wt+");
        if (p_out_file) {
            fprintf(p_out_file, "N = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->n);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "E = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->e);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "D = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->d);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "P = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->p);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "Q = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->q);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "DP = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->dmp1);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "DQ = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->dmq1);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "QP = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->iqmp);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fflush(p_out_file);
            fclose(p_out_file);
            p_out_file = NULL;
        }

        EVP_PKEY_free(pkey);
        pkey = NULL;
    } else {
        printf("[error]: PEM_read_bio_PrivateKey fail\n");
        return (-4);
    }

    return 0;
}

static int dump_rsa_pem_pubkey(const char *file, const char *output_file)
{
    int ret = 0;
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if ((file == NULL) || (output_file == NULL)) {
        printf("[error]: no keyfile specified\n");
        return (-1);
    }

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
        printf("[error]: no memory, BIO_new fail\n");
        return (-2);
    }

    ret = BIO_read_filename(key, file);
    if (ret <= 0) {
        printf("[error]: BIO_read_filename fail\n");
        return (-3);
    }

    pkey = PEM_read_bio_PUBKEY(key, NULL, NULL, NULL);

    BIO_free(key);

    if (pkey) {
        char max_string_buffer[2048 + 2] = {0};
        int string_length = 0;
        char *p_num = NULL;

        FILE *p_out_file = fopen(output_file, "wt+");
        if (p_out_file) {
            fprintf(p_out_file, "N = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->n);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fprintf(p_out_file, "E = ");
            string_length = __dump_bignumber((char *)max_string_buffer, 2048, pkey->pkey.rsa->e);
            max_string_buffer[string_length] = 0x0;
            p_num = __remove_heading_zerozero(max_string_buffer, string_length);
            fprintf(p_out_file, "%s", p_num);
            fprintf(p_out_file, "\r\n");

            fflush(p_out_file);
            fclose(p_out_file);
            p_out_file = NULL;
        }

        EVP_PKEY_free(pkey);
        pkey = NULL;
    } else {
        printf("[error]: PEM_read_bio_PUBKEY fail\n");
        return (-4);
    }

    return 0;
}

static void __tcok_show_usage()
{
    printf("test_convert_openssl_key usage:\n");
    printf("\t--convert-private convert private key\n");
    printf("\t--convert-public convert public key\n");
    printf("\t-i [file name] input file name, (pem)\n");
    printf("\t-o [file name] output file name, plaint text\n");

    printf("\t--help: show usage\n");
}

static int __tcok_init_params(int argc, char **argv, tcok_context* context)
{
    int i = 0;

    for (i = 1; i < argc; i ++) {
        if (!strcmp("-i", argv[i])) {
            if ((i + 1) < argc) {
                context->input_file = argv[i + 1];
                printf("[input file]: %s.\n", context->input_file);
            } else {
                printf("[input argument error]: '-i', should follow with input file name, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("-o", argv[i])) {
            if ((i + 1) < argc) {
                context->output_file = argv[i + 1];
                printf("[output file]: %s.\n", context->output_file);
            } else {
                printf("[input argument error]: '-o', should follow with output file name, argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--convert-private", argv[i])) {
            context->convert_private_key = 1;
        } else if (!strcmp("--convert-public", argv[i])) {
            context->convert_public_key = 1;
        } else if (!strcmp("--help", argv[i])) {
            __tcok_show_usage();
            return 1;
        } else {
            printf("[input argument error]: unknwon input params, [%d][%s]\n", i, argv[i]);
            return (-20);
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    tcok_context context;
    memset(&context, 0x0, sizeof(context));

    if (argc < 2) {
        __tcok_show_usage();
        return 1;
    }

    int ret = __tcok_init_params(argc, argv, &context);
    if (0 > ret) {
        return ret;
    }

    if ((!context.input_file) || (!context.output_file)) {
        printf("input or output file not specidied\n");
        return (-1);
    }

    if (1 == context.convert_private_key) {
        ret = dump_rsa_pem_privkey(context.input_file, context.output_file);
        if (ret) {
            printf("[error]: dump_rsa_pem_privkey fail, ret %d\n", ret);
            return ret;
        }
    } else if (1 == context.convert_public_key) {
        ret = dump_rsa_pem_pubkey(context.input_file, context.output_file);
        if (ret) {
            printf("[error]: dump_rsa_pem_pubkey fail, ret %d\n", ret);
            return ret;
        }
    } else {
        printf("[error]: should set mode, either convert private key, or convert public key\n");
        return (-10);
    }

    return 0;
}

