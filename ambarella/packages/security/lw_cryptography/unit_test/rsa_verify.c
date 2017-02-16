/*
 *  RSA/SHA-256 signature verification program
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
#include <string.h>
#include <stdio.h>

#include "cryptography_if.h"

int main( int argc, char *argv[] )
{
    FILE *f;
    int ret, c;
    unsigned int i = 0;
    rsa_context_t rsa;
    unsigned char hash[32] = {
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
        0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
    };
    unsigned char buf[D_BIG_NUMBER_MAX_SIZE];

    ret = 1;

    if( ( f = fopen( "rsa_pub.txt", "rb" ) ) == NULL )
    {
        printf( " failed\n  ! Could not open rsa_pub.txt\n" \
                "  ! Please run rsa_genkey first\n\n" );
        goto exit;
    }

    rsa_init(&rsa, RSA_PKCS_V15, 0 );

    if( ( ret = big_number_read_file( &rsa.N, 16, f ) ) != 0 ||
        ( ret = big_number_read_file( &rsa.E, 16, f ) ) != 0 )
    {
        printf( " failed\n  ! big_number_read_file returned %d\n\n", ret );
        goto exit;
    }

    rsa.len = ( big_number_msb( &rsa.N ) + 7 ) >> 3;

    fclose( f );

    /*
     * Extract the RSA signature from the text file
     */
    ret = 1;

    if( ( f = fopen("test.sign", "rb" ) ) == NULL )
    {
        printf( "\n  ! Could not open %s\n\n", argv[1] );
        goto exit;
    }

    while( fscanf( f, "%02X", &c ) > 0 &&
           i < (int) sizeof( buf ) )
        buf[i++] = (unsigned char) c;

    fclose( f );

    if( i != rsa.len )
    {
        printf( "\n  ! Invalid RSA signature format\n\n" );
        goto exit;
    }

    /*
     * Compute the SHA-256 hash of the input file and compare
     * it with the hash decrypted from the RSA signature.
     */
    printf( "\n  . Verifying the RSA/SHA-256 signature" );
    fflush( stdout );

#if 0
    if( ( ret = sha1_file( argv[1], hash ) ) != 0 )
    {
        printf( " failed\n  ! Could not open or read %s\n\n", argv[1] );
        goto exit;
    }
#endif

    if ((ret = rsa_sha256_verify(&rsa, hash, buf)) != 0)
    {
        printf( " failed\n  ! rsa_pkcs1_verify returned -0x%0x\n\n", -ret );
        goto exit;
    }

    printf( "\n  . OK (the decrypted SHA-256 hash matches)\n\n" );

    ret = 0;

exit:

#if defined(_WIN32)
    printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( ret );
}

