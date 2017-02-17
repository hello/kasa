/*
 * secure_boot.c
 *
 * History:
 *	2015/07/01 - [Zhi He] create file for secure boot logic
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#include <bldfunc.h>
#include <ambhw/idc.h>

#include "cryptography_if.h"
#include "peripheral/crypto_chip.h"
#include "peripheral/library_atsha204.h"

#include "secure_boot.h"

static void* gp_atsha204 = 0;

#define D_DIGEST_LENGTH DCRYPTO_SHA256_DIGEST_LENGTH

#if 0
static void __print_memory(unsigned char* p, unsigned int size)
{
	while (size > 7) {
		putbyte(p[0]);
		putbyte(p[1]);
		putbyte(p[2]);
		putbyte(p[3]);
		putbyte(p[4]);
		putbyte(p[5]);
		putbyte(p[6]);
		putbyte(p[7]);
		putstr("\r\n");
		p += 8;
		size -= 8;
	}

	while (size) {
		putbyte(p[0]);
		p ++;
		size --;
	}

	putstr("\r\n");
}
#endif

int secure_boot_init(unsigned char i2c_index, unsigned int i2c_freq)
{
	//putstr("secure_boot_init\r\n");
	if (!gp_atsha204) {
		idc_bld_init(i2c_index, i2c_freq);
		gp_atsha204 = libatsha204_open(i2c_index);
		if (!gp_atsha204) {
			putstr("[error]: libatsha204_open fail\r\n");
			return (-1);
		}

		if (0 == libatsha204_is_config_zone_locked(gp_atsha204)) {
			int ret = libatsha204_initialize_config_zone(gp_atsha204);
			if (ret) {
				putstr("[error]: libatsha204_initialize_config_zone fail\r\n");
				return (-4);
			} else {
				putstr("atsha204 init config zone done\r\n");
			}

			ret = libatsha204_initialize_otp_and_data_zone(gp_atsha204);
			if (ret) {
				putstr("[error]: libatsha204_initialize_otp_and_data_zone fail\r\n");
				return (-5);
			} else {
				putstr("atsha204 init otp and data zone done\r\n");
			}
			return 1;
		} else if (0 == libatsha204_is_otp_data_zone_locked(gp_atsha204)) {
			int ret = libatsha204_initialize_otp_and_data_zone(gp_atsha204);
			if (ret) {
				putstr("[error]: libatsha204_initialize_otp_and_data_zone fail\r\n");
				return (-6);
			} else {
				putstr("atsha204 init otp and data zone done\r\n");
			}
			return 1;
		}
	}

	return 0;
}

int generate_firmware_hw_signature(unsigned char* p_firmware, unsigned int firmware_length)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		int ret = 0;

		digest_sha256(p_firmware, firmware_length, digest);

		ret = libatsha204_generate_hw_signature(gp_atsha204, digest, DSECURE_FIRMWARE_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: gen fw sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int verify_firmware_hw_signature(unsigned char* p_firmware, unsigned int firmware_length)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		int ret = 0;

		digest_sha256(p_firmware, firmware_length, digest);

		ret = libatsha204_verify_hw_signature(gp_atsha204, digest, DSECURE_FIRMWARE_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: verify fw sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int generate_rsapubkey_hw_signature(unsigned char* p_pubkey, unsigned int pubkey_length)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		int ret = 0;

		digest_sha256(p_pubkey, pubkey_length, digest);

		ret = libatsha204_generate_hw_signature(gp_atsha204, digest, DSECURE_PUBKEY_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: gen pubkey sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int verify_rsapubkey_hw_signature(unsigned char* p_pubkey, unsigned int pubkey_length)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		int ret = 0;

		digest_sha256(p_pubkey, pubkey_length, digest);

		ret = libatsha204_verify_hw_signature(gp_atsha204, digest, DSECURE_PUBKEY_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: verify pubkey sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int generate_serialnumber_hw_signature()
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		unsigned char serial_number[16] = {0};
		int ret = 0;

		libatsha204_get_serial_number(gp_atsha204, serial_number);
		digest_sha256(serial_number, 9, digest);

		ret = libatsha204_generate_hw_signature(gp_atsha204, digest, DSECURE_SERIAL_NUMBER_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: gen sn sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int verify_serialnumber_hw_signature()
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		unsigned char serial_number[16] = {0};
		int ret = 0;

		libatsha204_get_serial_number(gp_atsha204, serial_number);
		digest_sha256(serial_number, 9, digest);

		ret = libatsha204_verify_hw_signature(gp_atsha204, digest, DSECURE_SERIAL_NUMBER_VERIFY_SECRET_SLOT);
		if (EStatusCode_OK != ret) {
			putstr("[error]: verify sn sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int verify_pubkey(void* fldev)
{
	fldev_t* fl = (fldev_t*) fldev;
	int ret;
	rsa_context_t rsa;

	rsa_init(&rsa, RSA_PKCS_V15, 0);

	ret = big_number_read_string(&rsa.N, 16, (const char*) fl->rsa_key_n);
	ret = big_number_read_string(&rsa.E, 16, (const char*) fl->rsa_key_e);
	rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;

	ret = rsa_check_pubkey(&rsa);
	rsa_free(&rsa);
	if (CRYPTO_ECODE_OK != ret) {
		return (-1);
	}

	return 0;
}

int verify_and_fill_pubkey(void* fldev, void* prsa_content)
{
	fldev_t* fl = (fldev_t*) fldev;
	rsa_context_t* prsa = (rsa_context_t*) prsa_content;
	int ret;

	rsa_init(prsa, RSA_PKCS_V15, 0);

	ret = big_number_read_string(&prsa->N, 16, (const char*) fl->rsa_key_n);
	ret = big_number_read_string(&prsa->E, 16, (const char*) fl->rsa_key_e);
	prsa->len = (big_number_msb(&prsa->N) + 7) >> 3;

	ret = rsa_check_pubkey(prsa);
	if (CRYPTO_ECODE_OK != ret) {
		rsa_free(prsa);
		return (-1);
	}

	return 0;
}

int verify_key(rsa_key_t* key)
{
	int ret;
	rsa_context_t rsa;

	rsa_init(&rsa, RSA_PKCS_V15, 0);

	ret = big_number_read_string(&rsa.N, 16, (const char*) key->n);
	ret = big_number_read_string(&rsa.E, 16, (const char*) key->e);
	ret = big_number_read_string(&rsa.D, 16, (const char*) key->d);
	ret = big_number_read_string(&rsa.P, 16, (const char*) key->p);
	ret = big_number_read_string(&rsa.Q, 16, (const char*) key->q);
	ret = big_number_read_string(&rsa.DP, 16, (const char*) key->dp);
	ret = big_number_read_string(&rsa.DQ, 16, (const char*) key->dq);
	ret = big_number_read_string(&rsa.QP, 16, (const char*) key->qp);

	rsa.len = (big_number_msb(&rsa.N) + 7) >> 3;

	ret = rsa_check_privkey(&rsa);
	rsa_free(&rsa);
	if (CRYPTO_ECODE_OK != ret) {
		return (-1);
	}

	return 0;
}

int verify_and_fill_key(rsa_key_t* key, void* prsa_content)
{
	int ret;
	rsa_context_t* prsa = (rsa_context_t*) prsa_content;

	rsa_init(prsa, RSA_PKCS_V15, 0);

	ret = big_number_read_string(&prsa->N, 16, (const char*) key->n);
	ret = big_number_read_string(&prsa->E, 16, (const char*) key->e);
	ret = big_number_read_string(&prsa->D, 16, (const char*) key->d);
	ret = big_number_read_string(&prsa->P, 16, (const char*) key->p);
	ret = big_number_read_string(&prsa->Q, 16, (const char*) key->q);
	ret = big_number_read_string(&prsa->DP, 16, (const char*) key->dp);
	ret = big_number_read_string(&prsa->DQ, 16, (const char*) key->dq);
	ret = big_number_read_string(&prsa->QP, 16, (const char*) key->qp);

	prsa->len = (big_number_msb(&prsa->N) + 7) >> 3;

	ret = rsa_check_privkey(prsa);
	if (CRYPTO_ECODE_OK != ret) {
		rsa_free(prsa);
		return (-1);
	}

	return 0;
}

int generate_sn_signature(unsigned char* sign, void* prsa_content)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		unsigned char serial_number[16] = {0};
		int ret = 0;
		rsa_context_t* prsa = (rsa_context_t*) prsa_content;

		libatsha204_get_serial_number(gp_atsha204, serial_number);
		digest_sha256(serial_number, 9, digest);

		//putstr("digest:\r\n");
		//__print_memory(digest, D_DIGEST_LENGTH);

		//putstr("serialnumber:\r\n");
		//__print_memory(serial_number, 9);

		ret = rsa_sha256_sign(prsa, digest, sign);

		//putstr("sign:\r\n");
		//__print_memory(sign, 128);

		if (CRYPTO_ECODE_OK != ret) {
			putstr("[error]: sn sign fail\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}

int verify_sn_signature(unsigned char* sign, void* prsa_content)
{
	if (gp_atsha204) {
		unsigned char digest[D_DIGEST_LENGTH] = {0};
		unsigned char serial_number[16] = {0};
		int ret = 0;
		rsa_context_t* prsa = (rsa_context_t*) prsa_content;

		libatsha204_get_serial_number(gp_atsha204, serial_number);
		digest_sha256(serial_number, 9, digest);

		//putstr("digest:\r\n");
		//__print_memory(digest, D_DIGEST_LENGTH);

		//putstr("serialnumber:\r\n");
		//__print_memory(serial_number, 9);

		//putstr("sign:\r\n");
		//__print_memory(sign, 128);

		ret = rsa_sha256_verify(prsa, digest, sign);

		if (CRYPTO_ECODE_OK != ret) {
			putstr("[error]: sn sign verify fail\r\n");
			putbyte((unsigned char) (0 - ret));
			putstr("\r\n");
			return (-1);
		}

	} else {
		putstr("[error]: device not inited\r\n");
		return (-2);
	}

	return 0;
}


