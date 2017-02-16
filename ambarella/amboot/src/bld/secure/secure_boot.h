/*
 * secure_boot.h
 *
 * History:
 *	2015/07/01 - [Zhi He] create file for secure boot logic
 *
 * Copyright (C) 2015 -2025, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __SECURE_BOOT_H__
#define __SECURE_BOOT_H__

#define D_RSA_KEY_LENGTH 1024
#define D_RSA_KEY_BN_TXT_LENGTH 256

typedef struct {
	char n[D_RSA_KEY_BN_TXT_LENGTH + 4];
	char e[16];
	char d[D_RSA_KEY_BN_TXT_LENGTH + 4];
	char p[(D_RSA_KEY_BN_TXT_LENGTH/2) + 8];
	char q[(D_RSA_KEY_BN_TXT_LENGTH/2) + 8];
	char dp[(D_RSA_KEY_BN_TXT_LENGTH/2) + 8];
	char dq[(D_RSA_KEY_BN_TXT_LENGTH/2) + 8];
	char qp[(D_RSA_KEY_BN_TXT_LENGTH/2) + 8];
} rsa_key_t;

int secure_boot_init();

int generate_firmware_hw_signature(unsigned char* p_firmware, unsigned int firmware_length);
int verify_firmware_hw_signature(unsigned char* p_firmware, unsigned int firmware_length);
int generate_rsapubkey_hw_signature(unsigned char* p_pubkey, unsigned int pubkey_length);
int verify_rsapubkey_hw_signature(unsigned char* p_pubkey, unsigned int pubkey_length);
int generate_serialnumber_hw_signature();
int verify_serialnumber_hw_signature();

int verify_pubkey(void* fldev);
int verify_and_fill_pubkey(void* fldev, void* prsa_content);
int verify_key(rsa_key_t* key);
int verify_and_fill_key(rsa_key_t* key, void* prsa_content);
int generate_sn_signature(unsigned char* sign, void* prsa_content);
int verify_sn_signature(unsigned char* sign, void* prsa_content);

#endif

