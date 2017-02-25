/*******************************************************************************
 * big_number.h
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


#ifndef __BIG_NUMBER_H__
#define __BIG_NUMBER_H__

#ifdef __cplusplus
extern "C" {
#endif

void big_number_init(big_number_t* X);
void big_number_free(big_number_t* X);
int big_number_grow(big_number_t* X, unsigned int nblimbs);
int big_number_shrink(big_number_t* X, unsigned int nblimbs);
int big_number_copy(big_number_t* X, const big_number_t* Y);
void big_number_swap(big_number_t* X, big_number_t* Y);
int big_number_safe_cond_assign(big_number_t* X, const big_number_t* Y, unsigned char assign);
int big_number_safe_cond_swap(big_number_t* X, big_number_t* Y, unsigned char assign);
int big_number_lset(big_number_t* X, TSINT z);
int big_number_get_bit(const big_number_t* X, unsigned int pos);
int big_number_set_bit(big_number_t* X, unsigned int pos, unsigned char val);
unsigned int big_number_lsb(const big_number_t* X);
unsigned int big_number_msb(const big_number_t* X);
unsigned int big_number_size(const big_number_t* X);

int big_number_read_binary(big_number_t* X, const unsigned char* buf, unsigned int buflen);
int big_number_write_binary(const big_number_t* X, unsigned char* buf, unsigned int buflen);
int big_number_shift_l(big_number_t* X, unsigned int count);
int big_number_shift_r(big_number_t* X, unsigned int count);
int big_number_cmp_abs(const big_number_t* X, const big_number_t* Y);
int big_number_cmp_big_number(const big_number_t* X, const big_number_t* Y);
int big_number_cmp_int(const big_number_t* X, TSINT z);
int big_number_add_abs(big_number_t* X, const big_number_t* A, const big_number_t* B);
int big_number_sub_abs(big_number_t* X, const big_number_t* A, const big_number_t* B);
int big_number_add_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B);
int big_number_sub_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B);
int big_number_add_int(big_number_t* X, const big_number_t* A, TSINT b);
int big_number_sub_int(big_number_t* X, const big_number_t* A, TSINT b);
int big_number_mul_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B);
int big_number_mul_int(big_number_t* X, const big_number_t* A, TSINT b);
int big_number_div_big_number(big_number_t* Q, big_number_t* R, const big_number_t* A, const big_number_t* B);
int big_number_div_int(big_number_t* Q, big_number_t* R, const big_number_t* A, TSINT b);
int big_number_mod_big_number(big_number_t* R, const big_number_t* A, const big_number_t* B);
int big_number_mod_int(TUINT *r, const big_number_t *A, TSINT b);
int big_number_exp_mod(big_number_t* X, const big_number_t* A, const big_number_t* E, const big_number_t* N, big_number_t* _RR);
int big_number_fill_random(big_number_t* X, unsigned int size, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng);
int big_number_gcd(big_number_t* G, const big_number_t* A, const big_number_t* B);
int big_number_inv_mod(big_number_t* X, const big_number_t* A, const big_number_t* N);
int big_number_is_prime(big_number_t* X, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng);
int big_number_gen_prime(big_number_t* X, unsigned int nbits, int dh_flag, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng);

#ifdef __cplusplus
}
#endif

#endif

