/*
 * secure_boot.h
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

int secure_boot_init(unsigned char i2c_index, unsigned int i2c_freq);

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

