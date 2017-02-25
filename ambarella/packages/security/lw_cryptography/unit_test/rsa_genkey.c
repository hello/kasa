/*******************************************************************************
 * rsa_genkey.c
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
#include <time.h>
#include <windows.h>

#include "cryptography_if.h"

#include <stdio.h>
#include <string.h>

#define KEY_SIZE 1024
#define EXPONENT 65537

static int __random_generator(void* context, unsigned char* output, unsigned int output_len)
{
    static unsigned char seed1 = 29, seed2 = 43, seed3 = 17, seed4 = 97;

    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    memcpy(output, &wtm, sizeof(wtm));

    output[0] = output[1] ^ seed1;
    output[1] = output[2] ^ seed2;
    output[2] = output[3] ^ seed3;
    output[3] = output[0] ^ seed4;
    pseudo_random_scamble_sequence(output, output_len);

    seed1 += 163;
    seed2 += 59;
    seed3 += 101;
    seed4 += 23;

    return 0;
}

int main()
{
    int ret;
    rsa_context_t rsa;
    FILE *fpub  = NULL;
    FILE *fpriv = NULL;

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    if ((ret = rsa_gen_key( &rsa, __random_generator, NULL, KEY_SIZE, EXPONENT)) != 0) {
        printf("rsa_gen_key fail, ret %d\n", ret);
        goto exit;
    }

    if ((fpub = fopen( "rsa_pub.txt", "wb+")) == NULL) {
        printf("cannot open rsa_pub.txt\n");
        ret = 1;
        goto exit;
    }

    if ((ret = big_number_write_file("N = ", &rsa.N, 16, fpub)) != 0 ||
        (ret = big_number_write_file("E = ", &rsa.E, 16, fpub )) != 0) {
        printf("big_number_write_file fail, return %d\n", ret );
        goto exit;
    }

    if ((fpriv = fopen("rsa_priv.txt", "wb+")) == NULL) {
        printf("cannot open rsa_priv.txt\n");
        ret = 1;
        goto exit;
    }

    if ((ret = big_number_write_file("N = ", &rsa.N , 16, fpriv)) != 0 ||
        (ret = big_number_write_file("E = ", &rsa.E , 16, fpriv)) != 0 ||
        (ret = big_number_write_file("D = ", &rsa.D , 16, fpriv)) != 0 ||
        (ret = big_number_write_file("P = " , &rsa.P , 16, fpriv)) != 0 ||
        (ret = big_number_write_file("Q = " , &rsa.Q , 16, fpriv)) != 0 ||
        (ret = big_number_write_file("DP = ", &rsa.DP, 16, fpriv)) != 0 ||
        (ret = big_number_write_file("DQ = ", &rsa.DQ, 16, fpriv)) != 0 ||
        (ret = big_number_write_file("QP = ", &rsa.QP, 16, fpriv)) != 0) {
        printf("big_number_write_file fail, ret %d\n", ret);
        goto exit;
    }

exit:

    if (fpub != NULL)
        fclose(fpub);

    if (fpriv != NULL)
        fclose(fpriv);

    rsa_free(&rsa);

#if defined(_WIN32)
    printf("press any key to exit\n" );
    fflush(stdout); getchar();
#endif

    return ret;
}

