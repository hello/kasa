/*
 *  Example RSA key generation program
 *
 *  Copyright (C) 2006-2011, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

int main( void )
{
    int ret;
    rsa_context_t rsa;
    FILE *fpub  = NULL;
    FILE *fpriv = NULL;
    const char *pers = "rsa_genkey";

    printf( ". Generating the RSA key [ %d-bit ]...", KEY_SIZE );
    fflush( stdout );

    rsa_init( &rsa, RSA_PKCS_V15, 0 );

    if( ( ret = rsa_gen_key( &rsa, __random_generator, NULL, KEY_SIZE,
                             EXPONENT ) ) != 0 )
    {
        printf( " failed\n  ! rsa_gen_key returned %d\n\n", ret );
        goto exit;
    }

    printf( " ok\n  . Exporting the public  key in rsa_pub.txt...." );
    fflush( stdout );

    if( ( fpub = fopen( "rsa_pub.txt", "wb+" ) ) == NULL )
    {
        printf( " failed\n  ! could not open rsa_pub.txt for writing\n\n" );
        ret = 1;
        goto exit;
    }

    if( ( ret = big_number_write_file( "N = ", &rsa.N, 16, fpub ) ) != 0 ||
        ( ret = big_number_write_file( "E = ", &rsa.E, 16, fpub ) ) != 0 )
    {
        printf( " failed\n  ! big_number_write_file returned %d\n\n", ret );
        goto exit;
    }

    printf( " ok\n  . Exporting the private key in rsa_priv.txt..." );
    fflush( stdout );

    if( ( fpriv = fopen( "rsa_priv.txt", "wb+" ) ) == NULL )
    {
        printf( " failed\n  ! could not open rsa_priv.txt for writing\n" );
        ret = 1;
        goto exit;
    }

    if( ( ret = big_number_write_file( "N = " , &rsa.N , 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "E = " , &rsa.E , 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "D = " , &rsa.D , 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "P = " , &rsa.P , 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "Q = " , &rsa.Q , 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "DP = ", &rsa.DP, 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "DQ = ", &rsa.DQ, 16, fpriv ) ) != 0 ||
        ( ret = big_number_write_file( "QP = ", &rsa.QP, 16, fpriv ) ) != 0 )
    {
        printf( " failed\n  ! big_number_write_file returned %d\n\n", ret );
        goto exit;
    }

    printf( " ok\n\n" );

exit:

    if( fpub  != NULL )
        fclose( fpub );

    if( fpriv != NULL )
        fclose( fpriv );

    rsa_free( &rsa );

#if defined(_WIN32)
    printf( "  Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( ret );
}

