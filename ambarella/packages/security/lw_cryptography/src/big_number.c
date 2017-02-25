/*******************************************************************************
 * big_number.c
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

#include "cryptography_if.h"

#ifndef DNOT_INCLUDE_C_HEADER
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#else
#include <bldfunc.h>
typedef unsigned int size_t;
#endif

#include "big_number.h"
#include "big_number_asm.h"

#define D_BIG_NUMBER_MAX_BITS_SCALE100          (100 * D_BIG_NUMBER_MAX_BITS)
#define D_LN_2_DIV_LN_10_SCALE100                 332
#define D_BIG_NUMBER_RW_BUFFER_SIZE             (((D_BIG_NUMBER_MAX_BITS_SCALE100 + D_LN_2_DIV_LN_10_SCALE100 - 1) / D_LN_2_DIV_LN_10_SCALE100) + 10 + 6)

#define DCHARS_IN_LIMB    (sizeof(TUINT))
#define DBITS_IN_LIMB    (DCHARS_IN_LIMB << 3)
#define DHALF_BITS_IN_LIMB    (DCHARS_IN_LIMB << 2)

#define MPI_SIZE_T_MAX  ( (size_t) -1 ) /* SIZE_T_MAX is not standard */

//#define DBITS_TO_LIMBS(i)  (((i) + DBITS_IN_LIMB - 1) / DBITS_IN_LIMB)
//#define DCHARS_TO_LIMBS(i) (((i) + DCHARS_IN_LIMB - 1) / DCHARS_IN_LIMB)
#define DBITS_TO_LIMBS(i)  ((i) / DBITS_IN_LIMB + ( (i) % DBITS_IN_LIMB != 0 ))
#define DCHARS_TO_LIMBS(i) ((i) / DCHARS_IN_LIMB + ( (i) % DCHARS_IN_LIMB != 0 ))

static void __zerosize(void *v, unsigned int n)
{
    volatile unsigned char *p = v;

    while ( n-- ) {
        *p++ = 0;
    }
}

void *__internal_malloc(int size)
{
#ifdef DNOT_INCLUDE_C_HEADER
    if (size < 0x200) {
        size = 0x200;
    }
#endif
   return malloc(size);
}

void big_number_init(big_number_t* X)
{
    if (X == NULL) {
        return;
    }

    X->s = 1;
    X->n = 0;
    X->p = NULL;
}

void big_number_free(big_number_t* X)
{
    if ( X == NULL ) {
        return;
    }

    if ( X->p != NULL ) {
        __zerosize(X->p, X->n * DCHARS_IN_LIMB);
        free(X->p);
    }

    X->s = 1;
    X->n = 0;
    X->p = NULL;
}

int big_number_grow(big_number_t* X, unsigned int nblimbs)
{
    TUINT *p;

    if (nblimbs > D_BIG_NUMBER_MAX_LIMBS) {
        return CRYPTO_ECODE_ERROR_NO_MEMORY;
    }

    if (X->n < nblimbs) {
        if ((p = __internal_malloc( nblimbs * DCHARS_IN_LIMB )) == NULL) {
            return CRYPTO_ECODE_ERROR_NO_MEMORY;
        }

        memset(p, 0, nblimbs * DCHARS_IN_LIMB);

        if (X->p != NULL) {
            memcpy(p, X->p, X->n * DCHARS_IN_LIMB);
            __zerosize(X->p, X->n * DCHARS_IN_LIMB);
            free(X->p);
        }

        X->n = nblimbs;
        X->p = p;
    }

    return CRYPTO_ECODE_OK;
}

int big_number_shrink(big_number_t* X, unsigned int nblimbs)
{
    TUINT *p;
    unsigned int i;

    if (X->n <= nblimbs) {
        return(big_number_grow(X, nblimbs));
    }

    for (i = X->n - 1; i > 0; i--)
        if (X->p[i] != 0) {
            break;
        }

    i++;

    if (i < nblimbs) {
        i = nblimbs;
    }

    if ((p = __internal_malloc( i * DCHARS_IN_LIMB )) == NULL) {
        return CRYPTO_ECODE_ERROR_NO_MEMORY;
    }

    memset(p, 0, i * DCHARS_IN_LIMB);

    if (X->p != NULL) {
        memcpy(p, X->p, i * DCHARS_IN_LIMB);
        __zerosize(X->p, X->n * DCHARS_IN_LIMB);
        free (X->p);
    }

    X->n = i;
    X->p = p;

    return CRYPTO_ECODE_OK;
}

int big_number_copy(big_number_t* X, const big_number_t* Y)
{
    int ret;
    unsigned int i;

    if (X == Y) {
        return CRYPTO_ECODE_OK;
    }

    if (Y->p == NULL) {
        big_number_free(X);
        return CRYPTO_ECODE_OK;
    }

    for (i = Y->n - 1; i > 0; i--)
        if (Y->p[i] != 0) {
            break;
        }

    i++;

    X->s = Y->s;

    D_CLEAN_IF_FAILED(big_number_grow(X, i));

    memset(X->p, 0, X->n * DCHARS_IN_LIMB);
    memcpy(X->p, Y->p, i * DCHARS_IN_LIMB);

cleanup:

    return ret;
}

void big_number_swap(big_number_t *X, big_number_t *Y)
{
    big_number_t T;

    memcpy(&T,  X, sizeof(big_number_t ));
    memcpy(X,  Y, sizeof(big_number_t ));
    memcpy(Y, &T, sizeof(big_number_t ));
}

int big_number_safe_cond_assign(big_number_t* X, const big_number_t* Y, unsigned char assign)
{
    int ret = 0;
    unsigned int i;

    assign = (assign | (unsigned char) - assign) >> 7;

    D_CLEAN_IF_FAILED(big_number_grow( X, Y->n));

    X->s = X->s * ( 1 - assign ) + Y->s * assign;

    for ( i = 0; i < Y->n; i++ ) {
        X->p[i] = X->p[i] * ( 1 - assign ) + Y->p[i] * assign;
    }

    for ( ; i < X->n; i++ ) {
        X->p[i] *= ( 1 - assign );
    }

cleanup:
    return ret;
}

int big_number_safe_cond_swap(big_number_t *X, big_number_t *Y, unsigned char swap)
{
    int ret, s;
    unsigned int i;
    TUINT tmp;

    if (X == Y) {
        return CRYPTO_ECODE_OK;
    }

    swap = (swap | (unsigned char) - swap) >> 7;

    D_CLEAN_IF_FAILED( big_number_grow(X, Y->n));
    D_CLEAN_IF_FAILED( big_number_grow(Y, X->n));

    s = X->s;
    X->s = X->s * ( 1 - swap ) + Y->s * swap;
    Y->s = Y->s * ( 1 - swap ) +    s * swap;


    for ( i = 0; i < X->n; i++ ) {
        tmp = X->p[i];
        X->p[i] = X->p[i] * ( 1 - swap ) + Y->p[i] * swap;
        Y->p[i] = Y->p[i] * ( 1 - swap ) +     tmp * swap;
    }

cleanup:
    return ret;
}

int big_number_lset(big_number_t *X, TSINT z)
{
    int ret;

    D_CLEAN_IF_FAILED( big_number_grow(X, 1));
    memset(X->p, 0, X->n * DCHARS_IN_LIMB);

    X->p[0] = (z < 0) ? -z : z;
    X->s    = (z < 0) ? -1 : 1;

cleanup:

    return( ret );
}

int big_number_get_bit(const big_number_t *X, unsigned int pos)
{
    if (X->n * DBITS_IN_LIMB <= pos) {
        return 0;
    }

    return ((X->p[pos / DBITS_IN_LIMB] >> (pos % DBITS_IN_LIMB)) & 0x01);
}

int big_number_set_bit( big_number_t *X, unsigned int pos, unsigned char val )
{
    int ret = 0;
    unsigned int off = pos / DBITS_IN_LIMB;
    unsigned int idx = pos % DBITS_IN_LIMB;

    if (val != 0 && val != 1) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    if (X->n * DBITS_IN_LIMB <= pos) {
        if (val == 0) {
            return CRYPTO_ECODE_OK;
        }

        D_CLEAN_IF_FAILED( big_number_grow(X, off + 1));
    }

    X->p[off] &= ~((TUINT) 0x01 << idx);
    X->p[off] |= (TUINT) val << idx;

cleanup:

    return ret;
}

unsigned int big_number_lsb(const big_number_t *X)
{
    unsigned int i, j, count = 0;

    for (i = 0; i < X->n; i++) {
        for (j = 0; j < DBITS_IN_LIMB; j++, count++) {
            if (((X->p[i] >> j) & 1) != 0) {
                return count ;
            }
        }
    }

    return 0;
}

unsigned int big_number_msb(const big_number_t *X)
{
    unsigned int i, j;

    if (X->n == 0) {
        return 0;
    }

    for (i = X->n - 1; i > 0; i--) {
        if (X->p[i] != 0) {
            break;
        }
    }

    for (j = DBITS_IN_LIMB; j > 0; j--) {
        if (((X->p[i] >> (j - 1)) & 1) != 0) {
            break;
        }
    }

    return ((i * DBITS_IN_LIMB) + j);
}

unsigned int big_number_size(const big_number_t *X)
{
    return ((big_number_msb(X) + 7) >> 3);
}

static int big_number_get_digit(TUINT *d, int radix, char c)
{
    *d = 255;

    if (c >= 0x30 && c <= 0x39) {
        *d = c - 0x30;
    }

    if (c >= 0x41 && c <= 0x46) {
        *d = c - 0x37;
    }

    if (c >= 0x61 && c <= 0x66) {
        *d = c - 0x57;
    }

    if ( *d >= (TUINT) radix ) {
        return CRYPTO_ECODE_INVALID_CHARACTER;
    }

    return CRYPTO_ECODE_OK;
}

int big_number_read_string(big_number_t *X, int radix, const char *s)
{
    int ret;
    unsigned int i, j, slen, n;
    TUINT d;
    big_number_t T;

    if (radix < 2 || radix > 16) {
        return( CRYPTO_ECODE_BAD_INPUT_DATA );
    }

    big_number_init(&T);

    slen = strlen(s);

    if (radix == 16) {
        if ( slen > MPI_SIZE_T_MAX >> 2 ) {
             return( CRYPTO_ECODE_BAD_INPUT_DATA );
        }
        n = DBITS_TO_LIMBS(slen << 2);

        D_CLEAN_IF_FAILED(big_number_grow( X, n));
        D_CLEAN_IF_FAILED(big_number_lset( X, 0));

        for (i = slen, j = 0; i > 0; i--, j++) {
            if (i == 1 && s[i - 1] == '-') {
                X->s = -1;
                break;
            }

            D_CLEAN_IF_FAILED( big_number_get_digit(&d, radix, s[i - 1]));
            X->p[j / ( 2 * DCHARS_IN_LIMB )] |= d << ((j % (2 * DCHARS_IN_LIMB)) << 2);
        }
    } else {
        D_CLEAN_IF_FAILED(big_number_lset( X, 0));

        for (i = 0; i < slen; i++) {
            if (i == 0 && s[i] == '-') {
                X->s = -1;
                continue;
            }

            D_CLEAN_IF_FAILED(big_number_get_digit( &d, radix, s[i]));
            D_CLEAN_IF_FAILED(big_number_mul_int(&T, X, radix));

            if (X->s == 1) {
                D_CLEAN_IF_FAILED(big_number_add_int(X, &T, d));
            } else {
                D_CLEAN_IF_FAILED(big_number_sub_int(X, &T, d));
            }
        }
    }

cleanup:

    big_number_free(&T);

    return ret ;
}

static int big_number_write_hlp(big_number_t *X, int radix, char **p)
{
    int ret;
    TUINT r;

    if (radix < 2 || radix > 16) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    D_CLEAN_IF_FAILED(big_number_mod_int(&r, X, radix));
    D_CLEAN_IF_FAILED(big_number_div_int(X, NULL, X, radix));

    if (big_number_cmp_int(X, 0) != 0) {
        D_CLEAN_IF_FAILED( big_number_write_hlp(X, radix, p));
    }

    if (r < 10) {
        *(*p)++ = (char)(r + 0x30);
    } else {
        *(*p)++ = (char)(r + 0x37);
    }

cleanup:

    return ret;
}

int big_number_write_string(const big_number_t* X, int radix, char* s, unsigned int* slen)
{
    int ret = 0;
    unsigned int n;
    char *p;
    big_number_t T;

    if (radix < 2 || radix > 16) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    n = big_number_msb(X);

    if (radix >=  4) {
        n >>= 1;
    }

    if (radix >= 16) {
        n >>= 1;
    }

    n += 3;

    if (*slen < n) {
        *slen = n;
        return CRYPTO_ECODE_BUFFER_TOO_SMALL;
    }

    p = s;
    big_number_init(&T);

    if (X->s == -1) {
        *p++ = '-';
    }

    if (radix == 16) {
        int c;
        unsigned int i, j, k;

        for (i = X->n, k = 0; i > 0; i--) {
            for (j = DCHARS_IN_LIMB; j > 0; j--) {
                c = (X->p[i - 1] >> ( ( j - 1 ) << 3)) & 0xFF;

                if (c == 0 && k == 0 && ( i + j ) != 2) {
                    continue;
                }

                *(p++) = "0123456789ABCDEF" [c / 16];
                *(p++) = "0123456789ABCDEF" [c % 16];
                k = 1;
            }
        }
    } else {
        D_CLEAN_IF_FAILED(big_number_copy(&T, X));

        if ( T.s == -1 ) {
            T.s = 1;
        }

        D_CLEAN_IF_FAILED(big_number_write_hlp(&T, radix, &p));
    }

    *p++ = '\0';
    *slen = p - s;

cleanup:

    big_number_free(&T);

    return ret;
}

#ifndef DNOT_INCLUDE_C_HEADER
int big_number_read_file(big_number_t* X, int radix, void* f)
{
    TUINT d;
    unsigned int slen;
    char *p;
    FILE* fin = (FILE*) f;

    char s[D_BIG_NUMBER_RW_BUFFER_SIZE];

    memset(s, 0, sizeof(s));

    if (fgets(s, sizeof(s) - 1, fin) == NULL) {
        return CRYPTO_ECODE_FILE_IO_ERROR;
    }

    slen = strlen(s);

    if (slen == sizeof( s ) - 2) {
        return CRYPTO_ECODE_BUFFER_TOO_SMALL;
    }

    if (s[slen - 1] == '\n') {
        slen--; s[slen] = '\0';
    }

    if (s[slen - 1] == '\r') {
        slen--; s[slen] = '\0';
    }

    p = s + slen;

    while (--p >= s) {
        if (big_number_get_digit( &d, radix, *p) != CRYPTO_ECODE_OK) {
            break;
        }
    }

    return big_number_read_string(X, radix, p + 1);
}

int big_number_write_file(const char* p, const big_number_t* X, int radix, void* f)
{
    int ret;
    unsigned int n, slen, plen;
    FILE* fout = (FILE*) f;

    char s[D_BIG_NUMBER_RW_BUFFER_SIZE];

    n = sizeof(s);
    memset(s, 0, n);
    n -= 2;

    D_CLEAN_IF_FAILED(big_number_write_string(X, radix, s, (unsigned int*) &n));

    if (p == NULL) {
        p = "";
    }

    plen = strlen(p);
    slen = strlen(s);
    s[slen++] = '\r';
    s[slen++] = '\n';

    if (fout != NULL) {
        if (fwrite( p, 1, plen, fout ) != plen || fwrite( s, 1, slen, fout ) != slen) {
            return CRYPTO_ECODE_FILE_IO_ERROR;
        }
    } else {
        DCRYPT_LOG("%s%s", p, s);
    }

cleanup:

    return ret;
}
#endif

int big_number_read_binary(big_number_t* X, const unsigned char* buf, unsigned int buflen)
{
    int ret;
    unsigned int i, j, n;

    for (n = 0; n < buflen; n++) {
        if (buf[n] != 0) {
            break;
        }
    }

    D_CLEAN_IF_FAILED(big_number_grow(X, DCHARS_TO_LIMBS(buflen - n)));
    D_CLEAN_IF_FAILED(big_number_lset(X, 0));

    for (i = buflen, j = 0; i > n; i--, j++) {
        X->p[j / DCHARS_IN_LIMB] |= ((TUINT) buf[i - 1]) << ((j % DCHARS_IN_LIMB) << 3);
    }

cleanup:

    return ret;
}

int big_number_write_binary(const big_number_t *X, unsigned char *buf, unsigned int buflen)
{
    unsigned int i, j, n;

    n = big_number_size(X);

    if (buflen < n) {
        return CRYPTO_ECODE_BUFFER_TOO_SMALL;
    }

    memset(buf, 0, buflen);

    for (i = buflen - 1, j = 0; n > 0; i--, j++, n--) {
        buf[i] = (unsigned char)(X->p[j / DCHARS_IN_LIMB] >> ((j % DCHARS_IN_LIMB) << 3));
    }

    return CRYPTO_ECODE_OK;
}

int big_number_shift_l(big_number_t* X, unsigned int count)
{
    int ret;
    unsigned int i, v0, t1;
    TUINT r0 = 0, r1;

    v0 = count / (DBITS_IN_LIMB);
    t1 = count & (DBITS_IN_LIMB - 1);

    i = big_number_msb(X) + count;

    if (X->n * DBITS_IN_LIMB < i) {
        D_CLEAN_IF_FAILED(big_number_grow(X, DBITS_TO_LIMBS(i)));
    }

    ret = 0;

    if (v0 > 0) {
        for (i = X->n; i > v0; i--) {
            X->p[i - 1] = X->p[i - v0 - 1];
        }

        for (; i > 0; i--) {
            X->p[i - 1] = 0;
        }
    }

    if (t1 > 0) {
        for (i = v0; i < X->n; i++) {
            r1 = X->p[i] >> (DBITS_IN_LIMB - t1);
            X->p[i] <<= t1;
            X->p[i] |= r0;
            r0 = r1;
        }
    }

cleanup:

    return ret;
}

int big_number_shift_r(big_number_t* X, unsigned int count)
{
    unsigned int i, v0, v1;
    TUINT r0 = 0, r1;

    v0 = count /  DBITS_IN_LIMB;
    v1 = count & (DBITS_IN_LIMB - 1);

    if (v0 > X->n || ( v0 == X->n && v1 > 0)) {
        return big_number_lset(X, 0);
    }

    if (v0 > 0) {
        for (i = 0; i < X->n - v0; i++) {
            X->p[i] = X->p[i + v0];
        }

        for (; i < X->n; i++) {
            X->p[i] = 0;
        }
    }

    if (v1 > 0) {
        for (i = X->n; i > 0; i--) {
            r1 = X->p[i - 1] << (DBITS_IN_LIMB - v1);
            X->p[i - 1] >>= v1;
            X->p[i - 1] |= r0;
            r0 = r1;
        }
    }

    return CRYPTO_ECODE_OK;
}

int big_number_cmp_abs(const big_number_t* X, const big_number_t* Y)
{
    unsigned int i, j;

    for (i = X->n; i > 0; i--) {
        if (X->p[i - 1] != 0) {
            break;
        }
    }

    for (j = Y->n; j > 0; j--) {
        if (Y->p[j - 1] != 0) {
            break;
        }
    }

    if (i == 0 && j == 0) {
        return 0;
    }

    if (i > j) {
        return (1);
    }

    if (j > i) {
        return (-1);
    }

    for (; i > 0; i--) {
        if (X->p[i - 1] > Y->p[i - 1]) {
            return (1);
        }

        if (X->p[i - 1] < Y->p[i - 1]) {
            return (-1);
        }
    }

    return 0;
}

int big_number_cmp_big_number(const big_number_t* X, const big_number_t* Y)
{
    unsigned int i, j;

    for (i = X->n; i > 0; i--) {
        if (X->p[i - 1] != 0) {
            break;
        }
    }

    for (j = Y->n; j > 0; j--) {
        if (Y->p[j - 1] != 0) {
            break;
        }
    }

    if (i == 0 && j == 0) {
        return 0;
    }

    if (i > j) {
        return (X->s);
    }

    if (j > i) {
        return (-Y->s);
    }

    if (X->s > 0 && Y->s < 0) {
        return (1);
    }

    if (Y->s > 0 && X->s < 0) {
        return (-1);
    }

    for (; i > 0; i--) {
        if (X->p[i - 1] > Y->p[i - 1]) {
            return (X->s);
        }

        if (X->p[i - 1] < Y->p[i - 1]) {
            return (-X->s);
        }
    }

    return 0;
}

int big_number_cmp_int(const big_number_t *X, TSINT z)
{
    big_number_t Y;
    TUINT p[1];

    *p  = (z < 0) ? -z : z;
    Y.s = (z < 0) ? -1 : 1;
    Y.n = 1;
    Y.p = p;

    return (big_number_cmp_big_number(X, &Y));
}

int big_number_add_abs(big_number_t *X, const big_number_t *A, const big_number_t *B)
{
    int ret;
    unsigned int i, j;
    TUINT *o, *p, c;

    if (X == B) {
        const big_number_t *T = A; A = X; B = T;
    }

    if (X != A) {
        D_CLEAN_IF_FAILED(big_number_copy(X, A));
    }

    X->s = 1;

    for (j = B->n; j > 0; j--) {
        if (B->p[j - 1] != 0) {
            break;
        }
    }

    D_CLEAN_IF_FAILED(big_number_grow(X, j));

    o = B->p; p = X->p; c = 0;

    for (i = 0; i < j; i++, o++, p++) {
        *p +=  c; c  = (*p <  c);
        *p += *o; c += (*p < *o);
    }

    while (c != 0) {
        if (i >= X->n) {
            D_CLEAN_IF_FAILED(big_number_grow(X, i + 1));
            p = X->p + i;
        }

        *p += c; c = ( *p < c ); i++; p++;
    }

cleanup:

    return ret;
}

static void big_number_sub_hlp(unsigned int n, TUINT *s, TUINT *d)
{
    unsigned int i;
    TUINT c, z;

    for (i = c = 0; i < n; i++, s++, d++) {
        z = (*d <  c);     *d -=  c;
        c = (*d < *s) + z; *d -= *s;
    }

    while (c != 0) {
        z = (*d < c); *d -= c;
        c = z; i++; d++;
    }
}

int big_number_sub_abs(big_number_t* X, const big_number_t* A, const big_number_t* B)
{
    big_number_t TB;
    int ret;
    unsigned int n;

    if (big_number_cmp_abs(A, B) < 0) {
        return (CRYPTO_ECODE_NEGATIVE_VALUE);
    }

    big_number_init(&TB);

    if (X == B) {
        D_CLEAN_IF_FAILED(big_number_copy(&TB, B));
        B = &TB;
    }

    if (X != A) {
        D_CLEAN_IF_FAILED(big_number_copy(X, A));
    }

    X->s = 1;

    ret = 0;

    for (n = B->n; n > 0; n--) {
        if (B->p[n - 1] != 0) {
            break;
        }
    }

    big_number_sub_hlp(n, B->p, X->p);

cleanup:

    big_number_free(&TB);

    return ret;
}

int big_number_add_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B)
{
    int ret, s = A->s;

    if (A->s * B->s < 0) {
        if (big_number_cmp_abs(A, B) >= 0) {
            D_CLEAN_IF_FAILED(big_number_sub_abs(X, A, B));
            X->s =  s;
        } else {
            D_CLEAN_IF_FAILED(big_number_sub_abs(X, B, A));
            X->s = -s;
        }
    } else {
        D_CLEAN_IF_FAILED(big_number_add_abs(X, A, B));
        X->s = s;
    }

cleanup:

    return ret;
}

int big_number_sub_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B)
{
    int ret, s = A->s;

    if (A->s * B->s > 0) {
        if (big_number_cmp_abs(A, B) >= 0) {
            D_CLEAN_IF_FAILED(big_number_sub_abs(X, A, B));
            X->s =  s;
        } else {
            D_CLEAN_IF_FAILED(big_number_sub_abs(X, B, A));
            X->s = -s;
        }
    } else {
        D_CLEAN_IF_FAILED(big_number_add_abs(X, A, B));
        X->s = s;
    }

cleanup:

    return ret;
}

int big_number_add_int(big_number_t* X, const big_number_t* A, TSINT b)
{
    big_number_t _B;
    TUINT p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return (big_number_add_big_number(X, A, &_B));
}

int big_number_sub_int(big_number_t* X, const big_number_t* A, TSINT b)
{
    big_number_t _B;
    TUINT p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return (big_number_sub_big_number(X, A, &_B));
}

static void big_number_mul_hlp(unsigned int i, TUINT *s, TUINT *d, TUINT b)
{
    TUINT c = 0, t = 0;

#if defined(MULADDC_HUIT)

    for ( ; i >= 8; i -= 8 ) {
        MULADDC_INIT
        MULADDC_HUIT
        MULADDC_STOP
    }

    for ( ; i > 0; i-- ) {
        MULADDC_INIT
        MULADDC_CORE
        MULADDC_STOP
    }

#else /* MULADDC_HUIT */

    for ( ; i >= 16; i -= 16 ) {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for ( ; i >= 8; i -= 8 ) {
        MULADDC_INIT
        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE

        MULADDC_CORE   MULADDC_CORE
        MULADDC_CORE   MULADDC_CORE
        MULADDC_STOP
    }

    for ( ; i > 0; i-- ) {
        MULADDC_INIT
        MULADDC_CORE
        MULADDC_STOP
    }

#endif /* MULADDC_HUIT */

    t++;

    do {
        *d += c; c = ( *d < c ); d++;
    } while ( c != 0 );
}

int big_number_mul_big_number(big_number_t* X, const big_number_t* A, const big_number_t* B)
{
    int ret;
    unsigned int i, j;
    big_number_t TA, TB;

    big_number_init(&TA); big_number_init(&TB);

    if (X == A) {
        D_CLEAN_IF_FAILED(big_number_copy(&TA, A));
        A = &TA;
    }

    if (X == B) {
        D_CLEAN_IF_FAILED(big_number_copy(&TB, B));
        B = &TB;
    }

    for (i = A->n; i > 0; i--) {
        if (A->p[i - 1] != 0) {
            break;
        }
    }

    for (j = B->n; j > 0; j--) {
        if (B->p[j - 1] != 0) {
            break;
        }
    }

    D_CLEAN_IF_FAILED(big_number_grow(X, i + j));
    D_CLEAN_IF_FAILED(big_number_lset(X, 0));

    for (i++; j > 0; j--) {
        big_number_mul_hlp(i - 1, A->p, X->p + j - 1, B->p[j - 1]);
    }

    X->s = A->s * B->s;

cleanup:

    big_number_free(&TB);
    big_number_free(&TA);

    return( ret );
}

int big_number_mul_int(big_number_t* X, const big_number_t* A, TSINT b)
{
    big_number_t _B;
    TUINT p[1];

    _B.s = 1;
    _B.n = 1;
    _B.p = p;
    p[0] = b;

    return (big_number_mul_big_number(X, A, &_B));
}

int big_number_div_big_number(big_number_t* Q, big_number_t* R, const big_number_t* A, const big_number_t* B)
{
    int ret;
    unsigned int i, n, t, k;
    big_number_t X, Y, Z, T1, T2;

    if (big_number_cmp_int(B, 0) == 0) {
        return (CRYPTO_ECODE_DIVISION_BY_ZERO);
    }

    big_number_init(&X);
    big_number_init(&Y);
    big_number_init(&Z);
    big_number_init(&T1);
    big_number_init(&T2);

    if (big_number_cmp_abs(A, B) < 0) {
        if (Q != NULL) {
            D_CLEAN_IF_FAILED( big_number_lset(Q, 0));
         }

        if (R != NULL) {
            D_CLEAN_IF_FAILED( big_number_copy(R, A));
        }

        return CRYPTO_ECODE_OK;
    }

    D_CLEAN_IF_FAILED(big_number_copy(&X, A));
    D_CLEAN_IF_FAILED(big_number_copy(&Y, B));
    X.s = Y.s = 1;

    D_CLEAN_IF_FAILED(big_number_grow(&Z, A->n + 2));
    D_CLEAN_IF_FAILED(big_number_lset(&Z,  0));
    D_CLEAN_IF_FAILED(big_number_grow(&T1, 2));
    D_CLEAN_IF_FAILED(big_number_grow(&T2, 3));

    k = big_number_msb(&Y) % DBITS_IN_LIMB;

    if (k < DBITS_IN_LIMB - 1) {
        k = DBITS_IN_LIMB - 1 - k;
        D_CLEAN_IF_FAILED(big_number_shift_l(&X, k));
        D_CLEAN_IF_FAILED(big_number_shift_l(&Y, k));
    } else {
        k = 0;
    }

    n = X.n - 1;
    t = Y.n - 1;
    D_CLEAN_IF_FAILED(big_number_shift_l(&Y, DBITS_IN_LIMB * (n - t)));

    while (big_number_cmp_big_number(&X, &Y) >= 0) {
        Z.p[n - t]++;
        D_CLEAN_IF_FAILED(big_number_sub_big_number(&X, &X, &Y));
    }

    D_CLEAN_IF_FAILED(big_number_shift_r(&Y, DBITS_IN_LIMB * (n - t)));

    for (i = n; i > t ; i--) {
        if (X.p[i] >= Y.p[t]) {
            Z.p[i - t - 1] = ~0;
        } else {
#ifdef DHAVE_DOUBLE_LONG_INT
            TUDBL r;

            r  = (TUDBL) X.p[i] << DBITS_IN_LIMB;
            r |= (TUDBL) X.p[i - 1];
            r /= Y.p[t];
            if (r > ((TUDBL) 1 << DBITS_IN_LIMB) - 1) {
                r = ((TUDBL) 1 << DBITS_IN_LIMB) - 1;
            }

            Z.p[i - t - 1] = (TUINT) r;
#else
            TUINT q0, q1, r0, r1;
            TUINT d0, d1, d, m;

            d  = Y.p[t];
            d0 = ( d << DHALF_BITS_IN_LIMB ) >> DHALF_BITS_IN_LIMB;
            d1 = ( d >> DHALF_BITS_IN_LIMB );

            q1 = X.p[i] / d1;
            r1 = X.p[i] - d1 * q1;
            r1 <<= DHALF_BITS_IN_LIMB;
            r1 |= ( X.p[i - 1] >> DHALF_BITS_IN_LIMB );

            m = q1 * d0;
            if( r1 < m )
            {
                q1--, r1 += d;
                while( r1 >= d && r1 < m )
                    q1--, r1 += d;
            }
            r1 -= m;

            q0 = r1 / d1;
            r0 = r1 - d1 * q0;
            r0 <<= DHALF_BITS_IN_LIMB;
            r0 |= ( X.p[i - 1] << DHALF_BITS_IN_LIMB ) >> DHALF_BITS_IN_LIMB;

            m = q0 * d0;
            if( r0 < m )
            {
                q0--, r0 += d;
                while( r0 >= d && r0 < m )
                    q0--, r0 += d;
            }
            r0 -= m;

            Z.p[i - t - 1] = ( q1 << DHALF_BITS_IN_LIMB ) | q0;
#endif
        }

        Z.p[i - t - 1]++;

        do {
            Z.p[i - t - 1]--;

            D_CLEAN_IF_FAILED(big_number_lset(&T1, 0));
            T1.p[0] = ( t < 1 ) ? 0 : Y.p[t - 1];
            T1.p[1] = Y.p[t];
            D_CLEAN_IF_FAILED(big_number_mul_int(&T1, &T1, Z.p[i - t - 1]));

            D_CLEAN_IF_FAILED(big_number_lset(&T2, 0));
            T2.p[0] = ( i < 2 ) ? 0 : X.p[i - 2];
            T2.p[1] = ( i < 1 ) ? 0 : X.p[i - 1];
            T2.p[2] = X.p[i];
        } while (big_number_cmp_big_number(&T1, &T2) > 0);

        D_CLEAN_IF_FAILED(big_number_mul_int(&T1, &Y, Z.p[i - t - 1]));
        D_CLEAN_IF_FAILED(big_number_shift_l(&T1,  DBITS_IN_LIMB * (i - t - 1)));
        D_CLEAN_IF_FAILED(big_number_sub_big_number(&X, &X, &T1));

        if (big_number_cmp_int(&X, 0) < 0) {
            D_CLEAN_IF_FAILED(big_number_copy(&T1, &Y));
            D_CLEAN_IF_FAILED(big_number_shift_l(&T1, DBITS_IN_LIMB * (i - t - 1)));
            D_CLEAN_IF_FAILED(big_number_add_big_number(&X, &X, &T1));
            Z.p[i - t - 1]--;
        }
    }

    if (Q != NULL) {
        D_CLEAN_IF_FAILED(big_number_copy(Q, &Z));
        Q->s = A->s * B->s;
    }

    if (R != NULL) {
        D_CLEAN_IF_FAILED(big_number_shift_r(&X, k));
        X.s = A->s;
        D_CLEAN_IF_FAILED(big_number_copy(R, &X));

        if (big_number_cmp_int(R, 0) == 0) {
            R->s = 1;
        }
    }

cleanup:

    big_number_free(&X);
    big_number_free(&Y);
    big_number_free(&Z);
    big_number_free(&T1);
    big_number_free(&T2);

    return ret;
}

int big_number_div_int(big_number_t *Q, big_number_t *R, const big_number_t *A, TSINT b)
{
    big_number_t _B;
    TUINT p[1];

    p[0] = ( b < 0 ) ? -b : b;
    _B.s = ( b < 0 ) ? -1 : 1;
    _B.n = 1;
    _B.p = p;

    return (big_number_div_big_number(Q, R, A, &_B));
}

int big_number_mod_big_number(big_number_t *R, const big_number_t *A, const big_number_t *B)
{
    int ret;

    if (big_number_cmp_int(B, 0) < 0) {
        return CRYPTO_ECODE_NEGATIVE_VALUE;
    }

    D_CLEAN_IF_FAILED(big_number_div_big_number(NULL, R, A, B));

    while (big_number_cmp_int(R, 0) < 0) {
        D_CLEAN_IF_FAILED( big_number_add_big_number(R, R, B));
    }

    while (big_number_cmp_big_number(R, B) >= 0) {
        D_CLEAN_IF_FAILED(big_number_sub_big_number(R, R, B));
    }

cleanup:

    return ret;
}

int big_number_mod_int(TUINT *r, const big_number_t *A, TSINT b)
{
    unsigned int i;
    TUINT x, y, z;

    if (b == 0) {
        return CRYPTO_ECODE_DIVISION_BY_ZERO;
    }

    if (b < 0) {
        return CRYPTO_ECODE_NEGATIVE_VALUE;
    }

    if (b == 1) {
        *r = 0;
        return CRYPTO_ECODE_OK;
    }

    if (b == 2) {
        *r = A->p[0] & 1;
        return CRYPTO_ECODE_OK;
    }

    for (i = A->n, y = 0; i > 0; i--) {
        x  = A->p[i - 1];
        y  = ( y << DHALF_BITS_IN_LIMB ) | ( x >> DHALF_BITS_IN_LIMB );
        z  = y / b;
        y -= z * b;

        x <<= DHALF_BITS_IN_LIMB;
        y  = ( y << DHALF_BITS_IN_LIMB ) | ( x >> DHALF_BITS_IN_LIMB );
        z  = y / b;
        y -= z * b;
    }

    if (A->s < 0 && y != 0) {
        y = b - y;
    }

    *r = y;

    return CRYPTO_ECODE_OK;
}

static void big_number_montg_init(TUINT *mm, const big_number_t* N)
{
    TUINT x, m0 = N->p[0];
    unsigned int i;

    x  = m0;
    x += ((m0 + 2) & 4) << 1;

    for (i = DBITS_IN_LIMB; i >= 8; i /= 2) {
        x *= (2 - (m0 * x));
    }

    *mm = ~x + 1;
}

static void big_number_montmul(big_number_t* A, const big_number_t* B, const big_number_t* N, TUINT mm, const big_number_t* T)
{
    unsigned int i, n, m;
    TUINT u0, u1, *d;

    memset(T->p, 0, T->n * DCHARS_IN_LIMB);

    d = T->p;
    n = N->n;
    m = ( B->n < n ) ? B->n : n;

    for (i = 0; i < n; i++) {
        u0 = A->p[i];
        u1 = ( d[0] + u0 * B->p[0] ) * mm;

        big_number_mul_hlp(m, B->p, d, u0);
        big_number_mul_hlp(n, N->p, d, u1);

        *d++ = u0; d[n + 1] = 0;
    }

    memcpy(A->p, d, ( n + 1 ) * DCHARS_IN_LIMB);

    if (big_number_cmp_abs( A, N ) >= 0) {
        big_number_sub_hlp(n, N->p, A->p);
    } else {
        big_number_sub_hlp(n, A->p, T->p);
    }
}

static void big_number_montred(big_number_t* A, const big_number_t* N, TUINT mm, const big_number_t* T)
{
    TUINT z = 1;
    big_number_t U;

    U.n = U.s = (int) z;
    U.p = &z;

    big_number_montmul(A, &U, N, mm, T);
}

int big_number_exp_mod(big_number_t* X, const big_number_t* A, const big_number_t* E, const big_number_t* N, big_number_t*_RR)
{
    int ret;
    unsigned int wbits, wsize, one = 1;
    unsigned int i, j, nblimbs;
    unsigned int bufsize, nbits;
    TUINT ei, mm, state;
    big_number_t RR, T, W[2 << D_BIG_NUMBER_WINDOW_SIZE], Apos;
    int neg;

    if (big_number_cmp_int( N, 0) < 0 || (N->p[0] & 1) == 0) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    if (big_number_cmp_int(E, 0) < 0) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    big_number_montg_init(&mm, N);
    big_number_init(&RR);
    big_number_init(&T);
    big_number_init(&Apos);
    memset(W, 0, sizeof(W));

    i = big_number_msb(E);

    wsize = ( i > 671 ) ? 6 : ( i > 239 ) ? 5 :
            ( i >  79 ) ? 4 : ( i >  23 ) ? 3 : 1;

    if (wsize > D_BIG_NUMBER_WINDOW_SIZE) {
        wsize = D_BIG_NUMBER_WINDOW_SIZE;
    }

    j = N->n + 1;
    D_CLEAN_IF_FAILED(big_number_grow(X, j ));
    D_CLEAN_IF_FAILED(big_number_grow(&W[1],  j));
    D_CLEAN_IF_FAILED(big_number_grow(&T, j * 2));

    neg = (A->s == -1);

    if (neg) {
        D_CLEAN_IF_FAILED(big_number_copy(&Apos, A));
        Apos.s = 1;
        A = &Apos;
    }

    if (_RR == NULL || _RR->p == NULL) {
        D_CLEAN_IF_FAILED(big_number_lset(&RR, 1));
        D_CLEAN_IF_FAILED(big_number_shift_l(&RR, N->n * 2 * DBITS_IN_LIMB));
        D_CLEAN_IF_FAILED(big_number_mod_big_number(&RR, &RR, N));

        if (_RR != NULL) {
            memcpy(_RR, &RR, sizeof(big_number_t));
        }
    } else {
        memcpy(&RR, _RR, sizeof(big_number_t));
    }

    if (big_number_cmp_big_number(A, N) >= 0) {
        D_CLEAN_IF_FAILED(big_number_mod_big_number(&W[1], A, N));
    } else {
        D_CLEAN_IF_FAILED(big_number_copy(&W[1], A));
    }

    big_number_montmul(&W[1], &RR, N, mm, &T);

    D_CLEAN_IF_FAILED(big_number_copy(X, &RR));
    big_number_montred(X, N, mm, &T);

    if (wsize > 1) {
        j =  one << ( wsize - 1 );

        D_CLEAN_IF_FAILED(big_number_grow(&W[j], N->n + 1));
        D_CLEAN_IF_FAILED(big_number_copy(&W[j], &W[1]));

        for (i = 0; i < wsize - 1; i++) {
            big_number_montmul(&W[j], &W[j], N, mm, &T);
        }

        for (i = j + 1; i < ( one << wsize ); i++) {
            D_CLEAN_IF_FAILED(big_number_grow(&W[i], N->n + 1));
            D_CLEAN_IF_FAILED(big_number_copy(&W[i], &W[i - 1]));

            big_number_montmul(&W[i], &W[1], N, mm, &T);
        }
    }

    nblimbs = E->n;
    bufsize = 0;
    nbits   = 0;
    wbits   = 0;
    state   = 0;

    while (1) {
        if (bufsize == 0) {
            if (nblimbs == 0) {
                break;
            }

            nblimbs--;

            bufsize = sizeof(TUINT) << 3;
        }

        bufsize--;

        ei = (E->p[nblimbs] >> bufsize) & 1;

        if (ei == 0 && state == 0) {
            continue;
        }

        if (ei == 0 && state == 1) {
            big_number_montmul(X, X, N, mm, &T);
            continue;
        }

        state = 2;

        nbits++;
        wbits |= ( ei << ( wsize - nbits ) );

        if (nbits == wsize) {
            for (i = 0; i < wsize; i++) {
                big_number_montmul(X, X, N, mm, &T);
            }

            big_number_montmul(X, &W[wbits], N, mm, &T);

            state--;
            nbits = 0;
            wbits = 0;
        }
    }

    for (i = 0; i < nbits; i++) {
        big_number_montmul(X, X, N, mm, &T);

        wbits <<= 1;

        if ((wbits & (one << wsize)) != 0) {
            big_number_montmul(X, &W[1], N, mm, &T);
        }
    }

    big_number_montred(X, N, mm, &T);

    if (neg) {
        X->s = -1;
        D_CLEAN_IF_FAILED(big_number_add_big_number(X, N, X));
    }

cleanup:

    for (i = (one << (wsize - 1)); i < (one << wsize); i++) {
        big_number_free(&W[i]);
    }

    big_number_free(&W[1]);
    big_number_free(&T);
    big_number_free(&Apos);

    if (_RR == NULL || _RR->p == NULL) {
        big_number_free(&RR);
    }

    return ret;
}

int big_number_gcd(big_number_t *G, const big_number_t *A, const big_number_t *B)
{
    int ret;
    unsigned int lz, lzt;
    big_number_t TG, TA, TB;

    big_number_init(&TG);
    big_number_init(&TA);
    big_number_init(&TB);

    D_CLEAN_IF_FAILED(big_number_copy(&TA, A));
    D_CLEAN_IF_FAILED(big_number_copy(&TB, B));

    lz = big_number_lsb(&TA);
    lzt = big_number_lsb(&TB);

    if (lzt < lz) {
        lz = lzt;
    }

    D_CLEAN_IF_FAILED(big_number_shift_r(&TA, lz));
    D_CLEAN_IF_FAILED(big_number_shift_r(&TB, lz));

    TA.s = TB.s = 1;

    while (big_number_cmp_int(&TA, 0) != 0) {
        D_CLEAN_IF_FAILED(big_number_shift_r(&TA, big_number_lsb(&TA)));
        D_CLEAN_IF_FAILED(big_number_shift_r(&TB, big_number_lsb(&TB)));

        if (big_number_cmp_big_number(&TA, &TB) >= 0) {
            D_CLEAN_IF_FAILED(big_number_sub_abs(&TA, &TA, &TB));
            D_CLEAN_IF_FAILED(big_number_shift_r(&TA, 1));
        } else {
            D_CLEAN_IF_FAILED(big_number_sub_abs(&TB, &TB, &TA));
            D_CLEAN_IF_FAILED(big_number_shift_r(&TB, 1));
        }
    }

    D_CLEAN_IF_FAILED(big_number_shift_l(&TB, lz));
    D_CLEAN_IF_FAILED(big_number_copy(G, &TB));

cleanup:

    big_number_free(&TG);
    big_number_free(&TA);
    big_number_free(&TB);

    return ret;
}

int big_number_fill_random(big_number_t* X, unsigned int size, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng)
{
    int ret;
    unsigned char buf[D_BIG_NUMBER_MAX_SIZE];

    if (size > D_BIG_NUMBER_MAX_SIZE) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    D_CLEAN_IF_FAILED(f_rng(p_rng, buf, size));
    D_CLEAN_IF_FAILED(big_number_read_binary(X, buf, size));

cleanup:
    return ret;
}

int big_number_inv_mod(big_number_t* X, const big_number_t* A, const big_number_t*N )
{
    int ret;
    big_number_t G, TA, TU, U1, U2, TB, TV, V1, V2;

    if (big_number_cmp_int(N, 0) <= 0) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    big_number_init(&TA);
    big_number_init(&TU);
    big_number_init(&U1);
    big_number_init(&U2);
    big_number_init(&G);
    big_number_init(&TB);
    big_number_init(&TV);
    big_number_init(&V1);
    big_number_init(&V2);

    D_CLEAN_IF_FAILED(big_number_gcd(&G, A, N));

    if (big_number_cmp_int(&G, 1) != 0) {
        ret = CRYPTO_ECODE_NOT_ACCEPTABLE;
        goto cleanup;
    }

    D_CLEAN_IF_FAILED(big_number_mod_big_number(&TA, A, N));
    D_CLEAN_IF_FAILED(big_number_copy(&TU, &TA));
    D_CLEAN_IF_FAILED(big_number_copy(&TB, N));
    D_CLEAN_IF_FAILED(big_number_copy(&TV, N));

    D_CLEAN_IF_FAILED(big_number_lset(&U1, 1));
    D_CLEAN_IF_FAILED(big_number_lset(&U2, 0));
    D_CLEAN_IF_FAILED(big_number_lset(&V1, 0));
    D_CLEAN_IF_FAILED(big_number_lset(&V2, 1));

    do {
        while ((TU.p[0] & 1) == 0) {
            D_CLEAN_IF_FAILED(big_number_shift_r(&TU, 1));

            if ((U1.p[0] & 1 ) != 0 || ( U2.p[0] & 1 ) != 0) {
                D_CLEAN_IF_FAILED(big_number_add_big_number(&U1, &U1, &TB));
                D_CLEAN_IF_FAILED(big_number_sub_big_number(&U2, &U2, &TA));
            }

            D_CLEAN_IF_FAILED(big_number_shift_r(&U1, 1));
            D_CLEAN_IF_FAILED(big_number_shift_r(&U2, 1));
        }

        while ((TV.p[0] & 1 ) == 0) {
            D_CLEAN_IF_FAILED(big_number_shift_r(&TV, 1));

            if ((V1.p[0] & 1 ) != 0 || ( V2.p[0] & 1 ) != 0) {
                D_CLEAN_IF_FAILED(big_number_add_big_number(&V1, &V1, &TB));
                D_CLEAN_IF_FAILED(big_number_sub_big_number(&V2, &V2, &TA));
            }

            D_CLEAN_IF_FAILED(big_number_shift_r(&V1, 1));
            D_CLEAN_IF_FAILED(big_number_shift_r(&V2, 1));
        }

        if (big_number_cmp_big_number(&TU, &TV) >= 0) {
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&TU, &TU, &TV));
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&U1, &U1, &V1));
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&U2, &U2, &V2));
        } else {
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&TV, &TV, &TU));
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&V1, &V1, &U1));
            D_CLEAN_IF_FAILED(big_number_sub_big_number(&V2, &V2, &U2));
        }
    } while (big_number_cmp_int(&TU, 0) != 0);

    while (big_number_cmp_int( &V1, 0 ) < 0) {
        D_CLEAN_IF_FAILED(big_number_add_big_number(&V1, &V1, N));
    }

    while (big_number_cmp_big_number( &V1, N ) >= 0) {
        D_CLEAN_IF_FAILED(big_number_sub_big_number(&V1, &V1, N));
    }

    D_CLEAN_IF_FAILED(big_number_copy(X, &V1));

cleanup:

    big_number_free(&TA);
    big_number_free(&TU);
    big_number_free(&U1);
    big_number_free(&U2);
    big_number_free(&G);
    big_number_free(&TB);
    big_number_free(&TV);
    big_number_free(&V1);
    big_number_free(&V2);

    return( ret );
}

static const int g_small_prime[] = {
    3,    5,    7,   11,   13,   17,   19,   23,
    29,   31,   37,   41,   43,   47,   53,   59,
    61,   67,   71,   73,   79,   83,   89,   97,
    101,  103,  107,  109,  113,  127,  131,  137,
    139,  149,  151,  157,  163,  167,  173,  179,
    181,  191,  193,  197,  199,  211,  223,  227,
    229,  233,  239,  241,  251,  257,  263,  269,
    271,  277,  281,  283,  293,  307,  311,  313,
    317,  331,  337,  347,  349,  353,  359,  367,
    373,  379,  383,  389,  397,  401,  409,  419,
    421,  431,  433,  439,  443,  449,  457,  461,
    463,  467,  479,  487,  491,  499,  503,  509,
    521,  523,  541,  547,  557,  563,  569,  571,
    577,  587,  593,  599,  601,  607,  613,  617,
    619,  631,  641,  643,  647,  653,  659,  661,
    673,  677,  683,  691,  701,  709,  719,  727,
    733,  739,  743,  751,  757,  761,  769,  773,
    787,  797,  809,  811,  821,  823,  827,  829,
    839,  853,  857,  859,  863,  877,  881,  883,
    887,  907,  911,  919,  929,  937,  941,  947,
    953,  967,  971,  977,  983,  991,  997, -103
};

static int big_number_check_small_factors(const big_number_t* X)
{
    int ret = 0;
    unsigned int i;
    TUINT r;

    if ((X->p[0] & 1) == 0) {
        return CRYPTO_ECODE_NOT_ACCEPTABLE;
    }

    for (i = 0; g_small_prime[i] > 0; i++) {
        if (big_number_cmp_int(X, g_small_prime[i]) <= 0) {
            return( 1 );
        }

        D_CLEAN_IF_FAILED(big_number_mod_int(&r, X, g_small_prime[i]));

        if (r == 0) {
            return CRYPTO_ECODE_NOT_ACCEPTABLE;
        }
    }

cleanup:
    return ret;
}

static int big_number_miller_rabin(const big_number_t* X, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng)
{
    int ret, count;
    unsigned int i, j, k, n, s;
    big_number_t W, R, T, A, RR;

    big_number_init(&W);
    big_number_init(&R);
    big_number_init(&T);
    big_number_init(&A);
    big_number_init(&RR);

    D_CLEAN_IF_FAILED(big_number_sub_int(&W, X, 1));
    s = big_number_lsb(&W);
    D_CLEAN_IF_FAILED(big_number_copy(&R, &W));
    D_CLEAN_IF_FAILED(big_number_shift_r(&R, s));

    i = big_number_msb(X);

    n = ( ( i >= 1300 ) ?  2 : ( i >=  850 ) ?  3 :
          ( i >=  650 ) ?  4 : ( i >=  350 ) ?  8 :
          ( i >=  250 ) ? 12 : ( i >=  150 ) ? 18 : 27 );

    for (i = 0; i < n; i++) {
        count = 0;

        do {
            D_CLEAN_IF_FAILED(big_number_fill_random(&A, X->n * DCHARS_IN_LIMB, f_rng, p_rng));

            j = big_number_msb(&A);
            k = big_number_msb(&W);

            if (j > k) {
                D_CLEAN_IF_FAILED(big_number_shift_r(&A, j - k));
            }

            if (count++ > 30) {
                return CRYPTO_ECODE_NOT_ACCEPTABLE;
            }

        } while ((big_number_cmp_big_number(&A, &W) >= 0) || (big_number_cmp_int(&A, 1)  <= 0));

        D_CLEAN_IF_FAILED(big_number_exp_mod(&A, &A, &R, X, &RR));

        if (big_number_cmp_big_number(&A, &W) == 0 || big_number_cmp_int(&A,  1) == 0) {
            continue;
        }

        j = 1;

        while (j < s && big_number_cmp_big_number(&A, &W) != 0) {
            D_CLEAN_IF_FAILED(big_number_mul_big_number(&T, &A, &A));
            D_CLEAN_IF_FAILED(big_number_mod_big_number(&A, &T, X));

            if (big_number_cmp_int(&A, 1) == 0) {
                break;
            }

            j++;
        }

        if (big_number_cmp_big_number(&A, &W) != 0 ||
             big_number_cmp_int(&A,  1) == 0) {
            ret = CRYPTO_ECODE_NOT_ACCEPTABLE;
            break;
        }
    }

cleanup:
    big_number_free(&W);
    big_number_free(&R);
    big_number_free(&T);
    big_number_free(&A);
    big_number_free(&RR);

    return ret;
}

int big_number_is_prime(big_number_t* X, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng)
{
    int ret;
    big_number_t XX;

    XX.s = 1;
    XX.n = X->n;
    XX.p = X->p;

    if (big_number_cmp_int(&XX, 0) == 0 ||
         big_number_cmp_int(&XX, 1) == 0) {
        return CRYPTO_ECODE_NOT_ACCEPTABLE;
    }

    if (big_number_cmp_int(&XX, 2) == 0) {
        return 0;
    }

    if ((ret = big_number_check_small_factors(&XX)) != 0) {
        if (ret == 1) {
            return 0;
        }

        return (ret);
    }

    return (big_number_miller_rabin(&XX, f_rng, p_rng));
}

int big_number_gen_prime(big_number_t* X, unsigned int nbits, int dh_flag, int (*f_rng)(void*, unsigned char*, unsigned int), void* p_rng)
{
    int ret;
    unsigned int k, n;
    TUINT r;
    big_number_t Y;

    if (nbits < 3 || nbits > D_BIG_NUMBER_MAX_BITS) {
        return CRYPTO_ECODE_BAD_INPUT_DATA;
    }

    big_number_init(&Y);

    n = DBITS_TO_LIMBS(nbits);

    D_CLEAN_IF_FAILED(big_number_fill_random(X, n * DCHARS_IN_LIMB, f_rng, p_rng));

    k = big_number_msb(X);

    if (k > nbits) {
        D_CLEAN_IF_FAILED(big_number_shift_r(X, k - nbits + 1));
    }

    big_number_set_bit(X, nbits - 1, 1);

    X->p[0] |= 1;

    if (dh_flag == 0) {
        while ((ret = big_number_is_prime( X, f_rng, p_rng)) != 0) {
            if (ret != CRYPTO_ECODE_NOT_ACCEPTABLE) {
                goto cleanup;
            }

            D_CLEAN_IF_FAILED(big_number_add_int(X, X, 2));
        }
    } else {
        X->p[0] |= 2;

        D_CLEAN_IF_FAILED(big_number_mod_int(&r, X, 3));

        if (r == 0) {
            D_CLEAN_IF_FAILED(big_number_add_int(X, X, 8));
        } else if ( r == 1 ) {
            D_CLEAN_IF_FAILED(big_number_add_int(X, X, 4));
        }

        D_CLEAN_IF_FAILED(big_number_copy(&Y, X));
        D_CLEAN_IF_FAILED(big_number_shift_r(&Y, 1));

        while (1) {
            if ((ret = big_number_check_small_factors(X)) == 0 &&
                 (ret = big_number_check_small_factors(&Y)) == 0 &&
                 (ret = big_number_miller_rabin(X, f_rng, p_rng)) == 0 &&
                 (ret = big_number_miller_rabin(&Y, f_rng, p_rng)) == 0) {
                    break;
            }

            if (ret != CRYPTO_ECODE_NOT_ACCEPTABLE) {
                goto cleanup;
            }

            D_CLEAN_IF_FAILED(big_number_add_int(X,  X, 12));
            D_CLEAN_IF_FAILED(big_number_add_int(&Y, &Y, 6));
        }
    }

cleanup:

    big_number_free(&Y);

    return ret;
}


