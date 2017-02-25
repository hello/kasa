/*
 * test_digital_signature.cpp
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "digital_signature_if.h"

#define SU_MAX_FILE_NAME_LENGTH 512

typedef struct
{
    unsigned char mode;
    unsigned char reserved0, reserved1, reserved2;

    char rsa_private_key_file[SU_MAX_FILE_NAME_LENGTH];
    char rsa_public_key_file[SU_MAX_FILE_NAME_LENGTH];
    int rsa_bits;
    int rsa_exp;

    char file[SU_MAX_FILE_NAME_LENGTH];
    char signature_file[SU_MAX_FILE_NAME_LENGTH];
} su_context;

static void show_usage()
{
    printf("test_digital_signature usage:\n");
    printf("\t--genrsakey [key file name base]\n");
    printf("\t--sign [file name] [private key file name] [signature file]\n");
    printf("\t--verifysign [file name] [public key file name] [signature file]\n");

    printf("\t--rsabits [bits]\n");
    printf("\t--rsaexp [exp]\n");

    printf("\t--help: show usage\n");
}

static int init_params(int argc, char **argv, su_context* context)
{
    int i = 0;
    int val = 0;

    for (i = 1; i < argc; i ++) {
        if (!strcmp("--genrsakey", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->rsa_private_key_file, SU_MAX_FILE_NAME_LENGTH, "%s_private.pem", argv[i + 1]);
                snprintf(context->rsa_public_key_file, SU_MAX_FILE_NAME_LENGTH, "%s_public.pem", argv[i + 1]);
                context->mode = 1;
                printf("[input argument]: '--genrsakey': (private %s, public %s).\n", context->rsa_private_key_file, context->rsa_public_key_file);
            } else {
                printf("[input argument error]: '--genrsakey', should follow with file name base, argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--rsabits", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                context->rsa_bits = val;
                printf("[input argument]: '--rsabits': (%d).\n", context->rsa_bits);
            } else {
                printf("[input argument error]: '--rsabits', should follow with integer(bits), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        } else if (!strcmp("--rsaexp", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
                context->rsa_exp = val;
                printf("[input argument]: '--rsaexp': (%d).\n", context->rsa_bits);
            } else {
                printf("[input argument error]: '--rsaexp', should follow with integer(exp), argc %d, i %d.\n", argc, i);
                return (-3);
            }
            i ++;
        } else if (!strcmp("--sign", argv[i])) {
            if (((i + 3) < argc)) {
                snprintf(context->file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 1]);
                snprintf(context->rsa_private_key_file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 2]);
                snprintf(context->signature_file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 3]);
                context->mode = 2;
                printf("[input argument]: '--sign': file (%s), key file (%s), signature file (%s).\n", context->file, context->rsa_private_key_file, context->signature_file);
            } else {
                printf("[input argument error]: '--sign', should follow with three filenames: [file], [private key file], [signature file], argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i += 3;
        } else if (!strcmp("--verifysign", argv[i])) {
            if (((i + 3) < argc)) {
                snprintf(context->file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 1]);
                snprintf(context->rsa_public_key_file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 2]);
                snprintf(context->signature_file, SU_MAX_FILE_NAME_LENGTH, "%s", argv[i + 3]);
                context->mode = 3;
                printf("[input argument]: '--verifysign': file (%s), key file (%s), signature file (%s).\n", context->file, context->rsa_public_key_file, context->signature_file);
            } else {
                printf("[input argument error]: '--verifysign', should follow with three filenames: [file], [public key file], [signature file], argc %d, i %d.\n", argc, i);
                return (-5);
            }
            i += 3;
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
    su_context context;
    memset(&context, 0x0, sizeof(context));

    context.rsa_bits = 1024;
    context.rsa_exp = 65537;

    if (argc < 2) {
        show_usage();
        return 1;
    }

    int ret = init_params(argc, argv, &context);
    if (0 > ret) {
        return ret;
    }

    if (1 == context.mode) {
        ret = generate_rsa_key(context.rsa_private_key_file, context.rsa_bits, context.rsa_exp, KEY_FORMAT_PEM);
        if (SU_ECODE_OK != ret) {
            printf("[error]: generate_rsa_key fail, ret %d\n", ret);
            return ret;
        }
        ret = get_public_rsa_key(context.rsa_public_key_file, context.rsa_private_key_file);
        if (SU_ECODE_OK != ret) {
            printf("[error]: get_public_rsa_key fail, ret %d\n", ret);
            return ret;
        }
        printf("RSA key pair generate [%s, %s] done\n", context.rsa_private_key_file, context.rsa_public_key_file);
    } else if (2 == context.mode) {
        ret = signature_file(context.file, context.signature_file, context.rsa_private_key_file, SHA_TYPE_SHA256);
        if (SU_ECODE_OK != ret) {
            printf("[error]: signature_file fail, ret %d\n", ret);
            return ret;
        }
        printf("Signature [%s] done\n", context.signature_file);
    } else if (3 == context.mode) {
        ret = verify_signature(context.file, context.signature_file, context.rsa_public_key_file, SHA_TYPE_SHA256);
        if (SU_ECODE_OK != ret) {
            printf("[error]: verify_signature fail, ret %d\n", ret);
            return ret;
        }
        printf("Verify signature OK\n");
    } else {
        printf("[error]: should set mode, either generate rsa key, signature or verify signatrue\n");
        return (-10);
    }

    return 0;
}

