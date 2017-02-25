/*
 * cryptochip_ioctl.h
 *
 * History:
 *	2015/06/04 - [Zhi He] Created file
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

#ifndef __CRYPTOCHIP_IOCTL_H__
#define __CRYPTOCHIP_IOCTL_H__

#include <linux/ioctl.h>

#define CRYPTOCHIP_MAGIC	'C'

#define CRYPTOCHIP_IO(nr)			_IO(CRYPTOCHIP_MAGIC, nr)
#define CRYPTOCHIP_IOR(nr, size)		_IOR(CRYPTOCHIP_MAGIC, nr, size)
#define CRYPTOCHIP_IOW(nr, size)		_IOW(CRYPTOCHIP_MAGIC, nr, size)
#define CRYPTOCHIP_IOWR(nr, size)		_IOWR(CRYPTOCHIP_MAGIC, nr, size)

#define DCC_INIT_DEFAULT_I2C_ADDRESS 0xc8
#define DCC_INIT_OTP_MODE_READ_ONLY 0xaa
#define DCC_INIT_OTP_MODE_CONSUMPTION 0x55
#define DCC_INIT_OTP_MODE_LEGACY 0x00
#define DCC_INIT_DEFAULT_EXPERIMENTAL_CHECK_MAC_CONFIG 0x81
#define DCC_INIT_DEFAULT_CHECK_MAC_CONFIG 0x00
#define DCC_INIT_DEFAULT_SELECTOR_MODE 0x00

#define DCC_INIT_SLOT_CONFIG1_READ_SLOT_MASK 0x0f
#define DCC_INIT_SLOT_CONFIG1_CHECK_ONLY_BIT 0x10
#define DCC_INIT_SLOT_CONFIG1_SINGLE_USE_BIT 0x20
#define DCC_INIT_SLOT_CONFIG1_ENCRYPT_READ_BIT 0x40
#define DCC_INIT_SLOT_CONFIG1_IS_SECRET_BIT 0x80

#define DCC_INIT_SLOT_CONFIG2_WRITE_SLOT_MASK 0x0f
#define DCC_INIT_SLOT_CONFIG2_ENCRYPT_WRITE_BIT 0x40
#define DCC_INIT_SLOT_CONFIG2_NON_WRITE_FLAG 0x80

#define DCC_DATA_SLOT_LENGTH 32
#define DCC_CONFIG_ZONE_PROGRAM_OFFSET 16

#define DCC_MODE_FALG_SN_23_47 (1 << 6)
#define DCC_MODE_FALG_OPT_64 (1 << 5)
#define DCC_MODE_FALG_OPT_88 (1 << 4)
#define DCC_MODE_FALG_TEMPKEY_SOURCE_FLAG (1 << 2)
#define DCC_MODE_FALG_TEMPKEY_OR_DATA_SLOT (1 << 1)
#define DCC_MODE_FALG_TEMPKEY_OR_CHALLENGE (1 << 0)

#define DCC_RANDOM_MODE_UPDATE_SEED 0x00
#define DCC_RANDOM_MODE_NOT_UPDATE_SEED 0x01
#define DCC_NOUCE_MODE_PASS_THROUGH 0x03

#define DCC_CRYPT_BIT (1 << 6)
#define DCC_ACCESS_DATA_SLOT (1 << 7)

typedef enum {
	CRYPTOCHIP_ZONE_CONFIG = 0x00,
	CRYPTOCHIP_ZONE_OTP = 0x01,
	CRYPTOCHIP_ZONE_DATA = 0x02,
} CRYPTOCHIP_ZONE;

typedef enum {
	//atomic transaction
	IOC_TRANSACTION_GENERATE_SIGNATURE = 0x01,
	IOC_TRANSACTION_VERIFY_SIGNATURE = 0x02,
	IOC_TRANSACTION_PREPARE_CRYPT_ACCESS = 0x03,
	IOC_TRANSACTION_GET_SERIAL_NUMBER = 0x04,
	IOC_TRANSACTION_ENCRYPT_WRITE = 0x05,
	IOC_TRANSACTION_DECRYPT_READ = 0x06,

	IOC_TRANSACTION_SHOW_CONFIG_ZONE = 0x07,
	IOC_TRANSACTION_SHOW_OTP_ZONE = 0x08,
	IOC_TRANSACTION_SHOW_DATA_ZONE = 0x09,

	IOC_TRANSACTION_PLAINTEXT_WRITE = 0x0a,
	IOC_TRANSACTION_PLAINTEXT_READ = 0x0b,

	//for factory inistialize only
	IOC_TRANSACTION_WRITE_CONFIG_ZONE = 0x20,
	IOC_TRANSACTION_WRITE_OTP_ZONE = 0x21,
	IOC_TRANSACTION_WRITE_DATA_ZONE = 0x22,

	IOC_TRANSACTION_INITIALIZE_CONFIG_ZONE = 0x23,
	IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE = 0x24,

	//debug use only
	IOC_TRANSACTION_LOCK_CONFIG_ZONE = 0x30,
	IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE_FORCE_LOCK = 0x31,
	IOC_TRANSACTION_PREPARE_CRYPT_ACCESS_PASS_THROUGH = 0x32,

	//operation can be used to build up customized transaction
	IOC_CHECK_MAC = 0x41,
	IOC_DERIVE_KEY = 0x42,
	IOC_DEVICE_REVISION = 0x43,
	IOC_GEN_DIGEST = 0x44,
	IOC_HMAC = 0x45,
	IOC_MAC = 0x46,
	IOC_NONCE = 0x47,
	IOC_RANDOM = 0x49,
	IOC_READ = 0x4a,
	IOC_SHA = 0x4b,
	IOC_UPDATE_EXTRA = 0x4c,
	IOC_WRITE = 0x4d,

	IOC_LOCK = 0x51,
	IOC_PAUSE = 0x52,

	IOC_BEGIN_TRANSACTION = 0x54,
	IOC_END_TRANSACTION = 0x55,
} CRYPTOCHIP_IOC;

typedef enum {
	EATSHA_CMD_TAG_CHECK_MAC = 0x28,
	EATSHA_CMD_TAG_DERIVE_KEY = 0x1c,
	EATSHA_CMD_TAG_DEVICE_REVISION = 0x30,
	EATSHA_CMD_TAG_GEN_DIGEST = 0x15,
	EATSHA_CMD_TAG_HMAC = 0x11,
	EATSHA_CMD_TAG_LOCK = 0x17,
	EATSHA_CMD_TAG_MAC = 0x08,
	EATSHA_CMD_TAG_NONCE = 0x16,
	EATSHA_CMD_TAG_PAUSE = 0x01,
	EATSHA_CMD_TAG_RANDOM = 0x1b,
	EATSHA_CMD_TAG_READ = 0x02,
	EATSHA_CMD_TAG_SHA = 0x47,
	EATSHA_CMD_TAG_UPDATE_EXTRA = 0x20,
	EATSHA_CMD_TAG_WRITE = 0x12,
} EATSHA_CMD_TAG;

typedef enum {
	EATSHA_RESPONCE_TAG_SUCCESS = 0x00,
	EATSHA_RESPONCE_TAG_CHECKMAC_FAIL = 0x01,
	EATSHA_RESPONCE_TAG_CMD_PARSE_ERROR = 0x03,
	EATSHA_RESPONCE_TAG_EXECUTION_ERROR = 0x0f,
	EATSHA_RESPONCE_TAG_AWAKE_NOTIFY = 0x11,
	EATSHA_RESPONCE_TAG_CRC_TRANS_ERROR = 0xff,
} EATSHA_RESPONCE_TAG;

struct cryptochip_transaction_signature {
	unsigned char	private_slot;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;

	unsigned char	digest[DCC_DATA_SLOT_LENGTH];
	unsigned char	sig[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_transaction_prepare_crypt_access {
	//nonce
	unsigned char	random[DCC_DATA_SLOT_LENGTH];
	unsigned char input[32]; //in out

	//gendig
	unsigned char zone;
	unsigned char slot_id;
	unsigned char b_pass_through;
	unsigned char reserved1;

	unsigned char otherdata[4];

	unsigned char digest_input_prefix[32];
};

struct cryptochip_transaction_crypt_access {
	unsigned char	data_slot;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;

	unsigned char	data[DCC_DATA_SLOT_LENGTH];
	unsigned char	mac[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_transaction_plaintext_access {
	unsigned char	is_4bytes;
	unsigned char	zone;
	unsigned char	slot;// for 32 bytes access
	unsigned char	address; //for 4 bytes access

	unsigned char	data[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_transaction_get_serial_number {
	unsigned char	serial_number[16];
};

struct cryptochip_transaction_config_zone {
	unsigned char	config_zone[88];
};

struct cryptochip_transaction_otp_zone {
	unsigned char	otp_zone[64];
};

struct cryptochip_transaction_data_zone {
	unsigned char	data_zone[512];
};

struct cryptochip_transaction_program_otp_and_data_zone {
	unsigned char	data_zone[512];
	unsigned char	otp_zone[64];
};

struct cryptochip_mac {
	unsigned char	mode;
	unsigned char	reserved0;
	unsigned char	slotid[2];

	unsigned char	challenge[DCC_DATA_SLOT_LENGTH];
	unsigned char	digest[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_check_mac {
	unsigned char	mode;
	unsigned char	reserved0;
	unsigned char	slotid[2];

	unsigned char	challenge[DCC_DATA_SLOT_LENGTH];
	unsigned char	digest[DCC_DATA_SLOT_LENGTH];

	unsigned char	otherdata[16]; // 13 bytes ared used
};

struct cryptochip_derive_key {
	unsigned char	mode;
	unsigned char	use_optional_data;
	unsigned char	target_slotid[2];

	unsigned char	optional_data[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_device_revision {
	unsigned char	revision[4];
};

struct cryptochip_gen_digest {
	unsigned char	zone;
	unsigned char	use_other_data;
	unsigned char	slotid[2];

	unsigned char	other_data[4];
};

struct cryptochip_hmac {
	unsigned char	mode;
	unsigned char	reserved_0;
	unsigned char	slotid[2];

	unsigned char	hmac_digest[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_nonce {
	unsigned char	random_mode;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;

	unsigned char	in_out[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_random {
	unsigned char	random_mode;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;

	unsigned char	random_out[DCC_DATA_SLOT_LENGTH];
};

#define DCC_SHA_MODE_INIT 0x00
#define DCC_SHA_MODE_COMPUTE 0x01
struct cryptochip_sha {
	unsigned char	sha_mode;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;

	unsigned char	data_input[64];
	unsigned char	sha_digest[DCC_DATA_SLOT_LENGTH];
};

#define DCC_UE_MODE_UPDATE_84 0x00
#define DCC_UE_MODE_UPDATE_85 0x01
#define DCC_UE_MODE_DECREASE 0x02
struct cryptochip_update_extra {
	unsigned char	ue_mode;
	unsigned char	lsb;
	unsigned char	reserved_1;
	unsigned char	reserved_2;
};

struct cryptochip_read {
	unsigned char	zone;
	unsigned char	dec;
	unsigned char	address[2];

	unsigned char	data[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_write {
	unsigned char	zone;
	unsigned char	enc;
	unsigned char	address[2];

	unsigned char	data[DCC_DATA_SLOT_LENGTH];
	unsigned char	mac[DCC_DATA_SLOT_LENGTH];
};

struct cryptochip_lock {
	unsigned char	zone;
	unsigned char	reserved_0;
	unsigned char	summary[2];
};

struct cryptochip_pause {
	unsigned char	selector;
	unsigned char	reserved_0;
	unsigned char	reserved_1;
	unsigned char	reserved_2;
};

#define CRYPTOCHIP_IOC_TRANSACTION_GENERATE_SIGNATURE    CRYPTOCHIP_IOWR(IOC_TRANSACTION_GENERATE_SIGNATURE, struct cryptochip_transaction_signature *)
#define CRYPTOCHIP_IOC_TRANSACTION_VERIFY_SIGNATURE    CRYPTOCHIP_IOWR(IOC_TRANSACTION_VERIFY_SIGNATURE, struct cryptochip_transaction_signature *)
#define CRYPTOCHIP_IOC_TRANSACTION_PREPARE_CRYPT_ACCESS    CRYPTOCHIP_IOWR(IOC_TRANSACTION_PREPARE_CRYPT_ACCESS, struct cryptochip_transaction_prepare_crypt_access *)
#define CRYPTOCHIP_IOC_TRANSACTION_GET_SERIAL_NUMBER CRYPTOCHIP_IOR(IOC_TRANSACTION_GET_SERIAL_NUMBER, struct cryptochip_transaction_get_serial_number *)
#define CRYPTOCHIP_IOC_TRANSACTION_ENCRYPT_WRITE    CRYPTOCHIP_IOW(IOC_TRANSACTION_ENCRYPT_WRITE, struct cryptochip_transaction_crypt_access *)
#define CRYPTOCHIP_IOC_TRANSACTION_DECRYPT_READ    CRYPTOCHIP_IOWR(IOC_TRANSACTION_DECRYPT_READ, struct cryptochip_transaction_crypt_access *)

#define CRYPTOCHIP_IOC_TRANSACTION_SHOW_CONFIG_ZONE CRYPTOCHIP_IOR(IOC_TRANSACTION_SHOW_CONFIG_ZONE, struct cryptochip_transaction_config_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_SHOW_OTP_ZONE CRYPTOCHIP_IOR(IOC_TRANSACTION_SHOW_OTP_ZONE, struct cryptochip_transaction_otp_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_SHOW_DATA_ZONE CRYPTOCHIP_IOR(IOC_TRANSACTION_SHOW_DATA_ZONE, struct cryptochip_transaction_data_zone *)

#define CRYPTOCHIP_IOC_TRANSACTION_PLAINTEXT_WRITE    CRYPTOCHIP_IOW(IOC_TRANSACTION_PLAINTEXT_WRITE, struct cryptochip_transaction_plaintext_access *)
#define CRYPTOCHIP_IOC_TRANSACTION_PLAINTEXT_READ    CRYPTOCHIP_IOWR(IOC_TRANSACTION_PLAINTEXT_READ, struct cryptochip_transaction_plaintext_access *)

#define CRYPTOCHIP_IOC_TRANSACTION_WRITE_CONFIG_ZONE CRYPTOCHIP_IOW(IOC_TRANSACTION_WRITE_CONFIG_ZONE, struct cryptochip_transaction_config_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_WRITE_OTP_ZONE CRYPTOCHIP_IOW(IOC_TRANSACTION_WRITE_OTP_ZONE, struct cryptochip_transaction_otp_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_WRITE_DATA_ZONE CRYPTOCHIP_IOW(IOC_TRANSACTION_WRITE_DATA_ZONE, struct cryptochip_transaction_data_zone *)

#define CRYPTOCHIP_IOC_TRANSACTION_INITIALIZE_CONFIG_ZONE CRYPTOCHIP_IOWR(IOC_TRANSACTION_INITIALIZE_CONFIG_ZONE, struct cryptochip_transaction_config_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE CRYPTOCHIP_IOW(IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE, struct cryptochip_transaction_program_otp_and_data_zone *)

#define CRYPTOCHIP_IOC_TRANSACTION_LOCK_CONFIG_ZONE CRYPTOCHIP_IO(IOC_TRANSACTION_LOCK_CONFIG_ZONE)
#define CRYPTOCHIP_IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE_FORCE_LOCK CRYPTOCHIP_IOW(IOC_TRANSACTION_INITIALIZE_OTP_AND_DATA_ZONE_FORCE_LOCK, struct cryptochip_transaction_program_otp_and_data_zone *)
#define CRYPTOCHIP_IOC_TRANSACTION_PREPARE_CRYPT_ACCESS_PASS_THROUGH    CRYPTOCHIP_IOWR(IOC_TRANSACTION_PREPARE_CRYPT_ACCESS_PASS_THROUGH, struct cryptochip_transaction_prepare_crypt_access *)

#define CRYPTOCHIP_IOC_MAC    CRYPTOCHIP_IOWR(IOC_MAC, struct cryptochip_mac *)
#define CRYPTOCHIP_IOC_CHECK_MAC    CRYPTOCHIP_IOWR(IOC_CHECK_MAC, struct cryptochip_check_mac *)
#define CRYPTOCHIP_IOC_DERIVE_KEY    CRYPTOCHIP_IOWR(IOC_DERIVE_KEY, struct cryptochip_derive_key *)
#define CRYPTOCHIP_IOC_DEVICE_REVISION    CRYPTOCHIP_IOR(IOC_DEVICE_REVISION, struct cryptochip_device_revision *)
#define CRYPTOCHIP_IOC_IOC_GEN_DIGEST    CRYPTOCHIP_IOW(IOC_GEN_DIGEST, struct cryptochip_gen_digest *)
#define CRYPTOCHIP_IOC_HMAC    CRYPTOCHIP_IOWR(IOC_HMAC, struct cryptochip_hmac *)
#define CRYPTOCHIP_IOC_NONCE    CRYPTOCHIP_IOWR(IOC_NONCE, struct cryptochip_nonce *)
#define CRYPTOCHIP_IOC_RANDOM    CRYPTOCHIP_IOWR(IOC_RANDOM, struct cryptochip_random *)
#define CRYPTOCHIP_IOC_SHA    CRYPTOCHIP_IOWR(IOC_SHA, struct cryptochip_sha *)
#define CRYPTOCHIP_IOC_UPDATE_EXTRA    CRYPTOCHIP_IOWR(IOC_UPDATE_EXTRA, struct cryptochip_update_extra *)
#define CRYPTOCHIP_IOC_READ    CRYPTOCHIP_IOWR(IOC_READ, struct cryptochip_read *)
#define CRYPTOCHIP_IOC_WRITE    CRYPTOCHIP_IOWR(IOC_WRITE, struct cryptochip_write *)

#define CRYPTOCHIP_IOC_LOCK    CRYPTOCHIP_IOW(IOC_LOCK, struct cryptochip_lock *)
#define CRYPTOCHIP_IOC_PAUSE    CRYPTOCHIP_IOW(IOC_PAUSE, struct cryptochip_pause *)

#define CRYPTOCHIP_IOC_BEGIN_TRANSACTION    CRYPTOCHIP_IO(IOC_BEGIN_TRANSACTION)
#define CRYPTOCHIP_IOC_END_TRANSACTION    CRYPTOCHIP_IO(IOC_END_TRANSACTION)

#endif

