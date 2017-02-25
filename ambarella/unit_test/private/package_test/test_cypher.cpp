/*
 * test_cypher.cpp
 *
 * History:
 *	2015/04/17 - [Zhi He] create file
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "openssl_wrapper.h"

#define TCYPHER_MAX_FILE_NAME_LENGTH 512

typedef struct
{
    unsigned char cypher_type;
    unsigned char cypher_mode;
    unsigned char key_len_in_bytes;
    unsigned char enc;

    char passphrase[32];
    char salt[32];

    char key[64];
    char iv[64];

    char infile[TCYPHER_MAX_FILE_NAME_LENGTH];
    char outfile[TCYPHER_MAX_FILE_NAME_LENGTH];
} tcypher_context;

static void show_usage()
{
    printf("test_cypher usage:\n");
    printf("\t--enc: encryption\n");
    printf("\t--dec: decryption\n");
    printf("\t--in [input file name]\n");
    printf("\t--out [output file name]\n");
    printf("\t--key [key]\n");
    printf("\t--iv [iv]\n");
    printf("\t--passphrase [passphrase]\n");
    printf("\t--salt [salt]\n");
    printf("\t--mode-cbc: CBC mode\n");
    printf("\t--mode-ctr: CTR mode\n");

    printf("\t--help: show usage\n");
}

static int init_params(int argc, char **argv, tcypher_context* context)
{
    int i = 0;

    for (i = 1; i < argc; i ++) {
        if (!strcmp("--enc", argv[i])) {
            context->enc = 1;
            printf("[input argument]: '--enc': encryption mode.\n");
        } else if (!strcmp("--dec", argv[i])) {
            context->enc = 0;
            printf("[input argument]: '--dec': decryption mode.\n");
        } else if (!strcmp("--in", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->infile, TCYPHER_MAX_FILE_NAME_LENGTH, "%s", argv[i + 1]);
                printf("[input argument]: '--in': (filename %s).\n", context->infile);
            } else {
                printf("[input argument error]: '--in', should follow with file name base, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--out", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->outfile, TCYPHER_MAX_FILE_NAME_LENGTH, "%s", argv[i + 1]);
                printf("[input argument]: '--out': (filename %s).\n", context->outfile);
            } else {
                printf("[input argument error]: '--out', should follow with file name base, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--key", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->key, 64, "%s", argv[i + 1]);
                printf("[input argument]: '--key': (%s).\n", context->key);
            } else {
                printf("[input argument error]: '--key', should follow with key, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--iv", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->iv, 64, "%s", argv[i + 1]);
                printf("[input argument]: '--iv': (%s).\n", context->iv);
            } else {
                printf("[input argument error]: '--iv', should follow with iv, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--passphrase", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->passphrase, 32, "%s", argv[i + 1]);
                printf("[input argument]: '--passphrase': (%s).\n", context->passphrase);
            } else {
                printf("[input argument error]: '--passphrase', should follow with passphrase, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--salt", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->salt, 32, "%s", argv[i + 1]);
                printf("[input argument]: '--salt': (%s).\n", context->salt);
            } else {
                printf("[input argument error]: '--salt', should follow with salt, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--mode-cbc", argv[i])) {
            context->cypher_mode = BLOCK_CYPHER_MODE_CBC;
            printf("[input argument]: '--mode-cbc': CBC mode.\n");
        } else if (!strcmp("--mode-ctr", argv[i])) {
            context->cypher_mode = BLOCK_CYPHER_MODE_CTR;
            printf("[input argument]: '--mode-ctr': CTR mode.\n");
        } else if (!strcmp("--help", argv[i])) {
            show_usage();
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
    FILE* p_infile = NULL;
    FILE* p_outfile = NULL;
    int ret = 0;
    s_symmetric_cypher* p_cypher = NULL;

    unsigned char inbuf[1024] = {0};
    unsigned char outbuf[1024] = {0};
    int insize = 0;
    int outsize = 0;

    tcypher_context context;
    memset(&context, 0x0, sizeof(context));

    context.cypher_type = CYPHER_TYPE_AES128;
    if (BLOCK_CYPHER_MODE_NONE == context.cypher_type) {
        context.cypher_mode = BLOCK_CYPHER_MODE_CTR;
        printf("use CTR mode as default.\n");
    }
    context.key_len_in_bytes = 16;

    if (argc < 2) {
        show_usage();
        ret = 1;
        goto fail_exit;
    }

    ret = init_params(argc, argv, &context);
    if (0 > ret) {
        printf("[error]: init_params fail\n");
        ret = -1;
        goto fail_exit;
    }

    p_infile = fopen(context.infile, "rb");
    if (!p_infile) {
        printf("[error]: open input file(%s) fail\n", context.infile);
        ret = -2;
        goto fail_exit;
    }

    p_outfile = fopen(context.outfile, "wb");
    if (!p_outfile) {
        printf("[error]: open output file(%s) fail\n", context.outfile);
        ret = -3;
        goto fail_exit;
    }

    p_cypher = create_symmetric_cypher(context.cypher_type, context.cypher_mode);
    if (!p_cypher) {
        printf("[error]: create_symmetric_cypher(%d, %d) fail\n", context.cypher_type, context.cypher_mode);
        ret = -4;
        goto fail_exit;
    }

    ret = set_symmetric_key_iv(p_cypher, context.key, context.iv);
    if (0 > ret) {
        printf("[error]: set_symmetric_key_iv fail\n");
        ret = -5;
        goto fail_exit;
    }

    ret = begin_symmetric_cryption(p_cypher, context.enc);
    if (0 > ret) {
        printf("[error]: begin_symmetric_cryption fail\n");
        ret = -6;
        goto fail_exit;
    }

    while (1) {
        insize = fread((void*)inbuf, 1, 1024, p_infile);
        if (0 >= insize) {
            printf("[eof]:\n");
            break;
        }

        if (1024 > insize) {
            fwrite((void*)inbuf, 1, insize, p_outfile);
            printf("[tail]: %d, not processed\n", insize);
            break;
        } else {
            outsize = symmetric_cryption(p_cypher, inbuf, insize, outbuf, context.enc);
            if (outsize != insize) {
                printf("why in out size not match\n");
                break;
            }
            fwrite((void*)outbuf, 1, outsize, p_outfile);
        }
    }

    ret = end_symmetric_cryption(p_cypher);
    if (0 > ret) {
        printf("[error]: end_symmetric_cryption fail\n");
        ret = -8;
        goto fail_exit;
    }

fail_exit:
    if (p_infile) {
        fclose(p_infile);
        p_infile = NULL;
    }

    if (p_outfile) {
        fclose(p_outfile);
        p_outfile = NULL;
    }

    if (p_cypher) {
        destroy_symmetric_cypher(p_cypher);
        p_cypher = NULL;
    }

    return ret;
}

