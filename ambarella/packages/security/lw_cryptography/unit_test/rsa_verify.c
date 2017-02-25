/*******************************************************************************
 * rsa_verify.c
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

#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "cryptography_if.h"

int main()
{
    FILE *f = NULL;
    int ret = 0, c;
    unsigned int i = 0;
    rsa_context_t rsa;
    unsigned char fake_digest[32] = {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
    };
    unsigned char signature[128];

    rsa_init(&rsa, RSA_PKCS_V15, 0 );

    if ((f = fopen( "rsa_pub.txt", "rb")) == NULL) {
        printf( "cannot open rsa_pub.txt\n");
        goto exit;
    }

    if ((ret = big_number_read_file(&rsa.N, 16, f)) != 0 ||
        (ret = big_number_read_file(&rsa.E, 16, f)) != 0) {
        printf("big_number_read_file fail, return %d\n", ret);
        goto exit;
    }

    fclose(f);
    f = NULL;

    rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;
    if (128 != rsa.len){
        printf("only support 1024 bit rsa, len %d\n", rsa.len);
        goto exit;
    }

    if ((f = fopen("test.sign", "rb")) == NULL) {
        printf("cannot open test.sign\n");
        goto exit;
    }

    fread(signature, 1, sizeof(signature), f);
    fclose(f);
    f = NULL;

    if ((ret = rsa_sha256_verify(&rsa, fake_digest, signature)) != 0) {
        printf( "rsa_sha256_verify fail, retruen %d\n", ret);
        goto exit;
    }

    printf("verify OK\n");

exit:

    if (f != NULL)
        fclose(f);

    rsa_free(&rsa);

#if defined(_WIN32)
    printf("press any key to exit.\n" );
    fflush(stdout); getchar();
#endif

    return ret;
}

