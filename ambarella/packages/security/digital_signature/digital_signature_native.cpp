/*******************************************************************************
 * digital_signature_native.cpp
 *
 * History:
 *	2015/09/10 - [Zhi He] created file
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

#ifdef BUILD_OS_WINDOWS
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "digital_signature_if.h"

#include "cryptography_if.h"

#define DRSA_KEY_SIZE 1024

#if 0
static int __sw_random_generator(void* context, unsigned char* output, unsigned int output_len)
{
    static unsigned char seed1 = 29, seed2 = 43, seed3 = 17, seed4 = 97;

#ifdef BUILD_OS_WINDOWS
    SYSTEMTIME wtm1, wtm2;
    GetLocalTime(&wtm1);
    memcpy(output, &wtm1, sizeof(wtm1));
    Sleep(wtm1.wMilliseconds & 128);
    memcpy(output + sizeof(wtm1), &wtm2, sizeof(wtm2));
    GetLocalTime(&wtm2);
    seed1 += ((unsigned int)(wtm2.wMilliseconds - wtm1.wMilliseconds)) & 0xf;
    seed2 += (((unsigned int)(wtm2.wMilliseconds - wtm1.wMilliseconds)) >> 2) & 0xf;
    seed3 += (((unsigned int)(wtm2.wMilliseconds - wtm1.wMilliseconds)) >> 4) & 0xf;
    seed4 += (((unsigned int)(wtm2.wMilliseconds - wtm1.wMilliseconds)) >> 8) & 0xf;
#else
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    memcpy(output, &tv1, sizeof(tv1));
    usleep(10 * tv1.tv_usec);
    memcpy(output + sizeof(tv1), &tv2, sizeof(tv2));
    gettimeofday(&tv2, NULL);
    seed1 += ((unsigned int)(tv2.tv_usec - tv1.tv_usec )) & 0xff;
    seed2 += (((unsigned int)(tv2.tv_usec - tv1.tv_usec )) >> 4) & 0xff;
    seed3 += (((unsigned int)(tv2.tv_usec - tv1.tv_usec )) >> 8) & 0xff;
    seed4 += (((unsigned int)(tv2.tv_usec - tv1.tv_usec )) >> 12) & 0xff;
#endif

    digest_sha256(output, 32, output + 32);
    output[0] += output[32] ^ seed1;
    output[1] += output[33] ^ seed2;
    output[2] += output[34] ^ seed3;
    output[3] += output[35] ^ seed4;
    output[4] += output[36] ^ seed1;
    output[5] += output[37] ^ seed2;
    output[6] += output[38] ^ seed3;
    output[7] += output[39] ^ seed4;

    pseudo_random_scamble_sequence(output, output_len);

    seed1 += 163;
    seed2 += 59;
    seed3 += 101;
    seed4 += 23;

    return 0;
}
#endif

static int __sw_random_generator(void* context, unsigned char* output, unsigned int output_len)
{
    static unsigned char seed1 = 29, seed2 = 43, seed3 = 17, seed4 = 97;

#ifdef BUILD_OS_WINDOWS
    SYSTEMTIME wtm1;
    GetLocalTime(&wtm1);
    memcpy(output, &wtm1, sizeof(wtm1));
#else
    struct timeval tv1;
    gettimeofday(&tv1, NULL);
    memcpy(output, &tv1, sizeof(tv1));
#endif

    output[0] += output[1] ^ seed1;
    output[1] += output[2] ^ seed2;
    output[2] += output[3] ^ seed3;
    output[3] += output[4] ^ seed4;

    pseudo_random_scamble_sequence(output, output_len);

    seed1 += 163;
    seed2 += 59;
    seed3 += 101;
    seed4 += 23;

    return 0;
}

int generate_rsa_key(char* output_file, int bits, int pubexp, int key_format)
{
    int ret = 0;
    FILE *f = NULL;
    rsa_context_t rsa;

    if ((!output_file) || (!bits) || (!pubexp)) {
        printf("bad params, output_file %p, bits %d, pubexp %d\n", output_file, bits, pubexp);
        return (-1);
    }
    bits = DRSA_KEY_SIZE;

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    ret = rsa_gen_key(&rsa, __sw_random_generator, NULL, (unsigned int) bits, pubexp);

    if (ret) {
        printf("generate_rsa_key fail, ret %d\n", ret);
        return ret;
    }

    f = fopen(output_file, "wb+");
    if (!f) {
        printf("open file (%s) fail\n", output_file);
        return (-2);
    }

    ret = big_number_write_file("N = " , &rsa.N , 16, f);
    ret = big_number_write_file("E = " , &rsa.E , 16, f);
    ret = big_number_write_file("D = " , &rsa.D , 16, f);
    ret = big_number_write_file("P = " , &rsa.P , 16, f);
    ret = big_number_write_file("Q = " , &rsa.Q , 16, f);
    ret = big_number_write_file("DP = ", &rsa.DP, 16, f);
    ret = big_number_write_file("DQ = ", &rsa.DQ, 16, f);
    ret = big_number_write_file("QP = ", &rsa.QP, 16, f);

    fclose(f);

    rsa_free(&rsa);

    return 0;
}

int get_public_rsa_key(char* output_file, char* input_file)
{
    int ret = 0;
    FILE *f = NULL;
    rsa_context_t rsa;

    if ((!output_file) || (!input_file)) {
        printf("bad params, output_file %p, input_file %p\n", output_file, input_file);
        return (-1);
    }

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    f = fopen(input_file, "rb");

    if (!f) {
        printf("open key file(%s) fail\n", input_file);
        return (-2);
    }

    ret = big_number_read_file(&rsa.N , 16, f);
    ret = big_number_read_file(&rsa.E , 16, f);
    if (ret) {
        printf("read big number fail\n");
        return (-2);
    }

    fclose(f);
    f = NULL;

    f = fopen(output_file, "wb+");
    if (!f) {
        printf("open file (%s) fail\n", output_file);
        return (-3);
    }

    ret = big_number_write_file("N = ", &rsa.N, 16, f);
    ret = big_number_write_file("E = ", &rsa.E, 16, f);

    fclose(f);

    return 0;
}

int signature_file(char* file, char* digital_signature, char* keyfile, int sha_type)
{
    int ret = 0;
    FILE *f = NULL;
    rsa_context_t rsa;
    unsigned char digest[32] = {0};
    unsigned char buf[DRSA_KEY_SIZE] = {0};

    if ((!file) || (!digital_signature) || (!keyfile) || (SHA_TYPE_SHA256 != sha_type)) {
        printf("bad params, file %p, digital_signature %p, keyfile %p, sha_type %d\n", file, digital_signature, keyfile, sha_type);
        return (-1);
    }

    rsa_init(&rsa, RSA_PKCS_V15, 0);
    f = fopen(keyfile, "rb");

    if (!f) {
        printf("open key file(%s) fail\n", keyfile);
        return (-2);
    }

    ret = big_number_read_file(&rsa.N , 16, f);
    ret = big_number_read_file(&rsa.E , 16, f);
    ret = big_number_read_file(&rsa.D , 16, f);
    ret = big_number_read_file(&rsa.P , 16, f);
    ret = big_number_read_file(&rsa.Q , 16, f);
    ret = big_number_read_file(&rsa.DP, 16, f);
    ret = big_number_read_file(&rsa.DQ, 16, f);
    ret = big_number_read_file(&rsa.QP, 16, f);

    rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;
    fclose(f);
    f = NULL;
    ret = rsa_check_privkey(&rsa);

    if (ret) {
        printf("not valid key\n");
        return (-3);
    }

    ret = digest_sha256_file(file, digest);
    if (ret) {
        printf("digest_sha256_file(%s) fail\n", file);
        return (-4);
    }

    ret = rsa_sha256_sign(&rsa, digest, buf);
    if (ret) {
        printf("rsa_sha256_sign fail, ret %d\n", ret);
        return (-5);
    }

    f = fopen(digital_signature, "wb+");
    if (!f) {
        printf("open signature file(%s) fail\n", digital_signature);
        return (-6);
    }
    fwrite(buf, 1, rsa.len, f);
    fclose(f);

    return 0;
}

int verify_signature(char* file, char* digital_signature, char* keyfile, int sha_type)
{
    int ret = 0;
    FILE *f = NULL;
    unsigned char digest[32] = {0};
    unsigned char buf[DRSA_KEY_SIZE];
    rsa_context_t rsa;

    if ((!file) || (!digital_signature) || (!keyfile) || (SHA_TYPE_SHA256 != sha_type)) {
        printf("bad params, file %p, digital_signature %p, keyfile %p, sha_type %d\n", file, digital_signature, keyfile, sha_type);
        return (-1);
    }

    f = fopen(keyfile, "rb");

    if (!f) {
        printf("open key file(%s) fail\n", keyfile);
        return (-2);
    }

    rsa_init(&rsa, RSA_PKCS_V15, 0);

    ret = big_number_read_file(&rsa.N , 16, f);
    ret = big_number_read_file(&rsa.E , 16, f);

    rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;
    fclose(f);
    f = NULL;

    ret = digest_sha256_file(file, digest);
    if (ret) {
        printf("digest_sha256_file(%s) fail\n", file);
        return (-3);
    }

    f = fopen(digital_signature, "rb");
    if (!f) {
        printf("open signature file(%s) fail\n", digital_signature);
        return (-4);
    }
    ret = fread(buf, 1, DRSA_KEY_SIZE, f);
    fclose(f);

    ret = rsa_sha256_verify(&rsa, digest, buf);

    return ret;
}

