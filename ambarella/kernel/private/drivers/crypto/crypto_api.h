/*
 * kernel/private/drivers/ambarella/crypto/crypto_api.h
 *
 * History:
 *	2011/05/05 - [Jian Tang] created file
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



#ifndef __CRYPTO_API_H__
#define __CRYPTO_API_H__


typedef struct amba_crypto_context_s {
	void 			* file;
	struct mutex		* mutex;
	u8				* buffer;
} amba_crypto_context_t;


// external function for driver
int amba_crypto_set_algo(amba_crypto_context_t *context, struct amba_crypto_algo_s __user *arg);
int amba_crypto_set_key(amba_crypto_context_t *context, struct amba_crypto_key_s __user *arg);
int amba_crypto_encrypt(amba_crypto_context_t *context, struct amba_crypto_info_s __user *arg);
int amba_crypto_decrypt(amba_crypto_context_t *context, struct amba_crypto_info_s __user *arg);
int amba_crypto_sha(amba_crypto_context_t *context, struct amba_crypto_sha_md5_s __user *arg);
int amba_crypto_md5(amba_crypto_context_t *context, struct amba_crypto_sha_md5_s __user *arg);

#endif	//  __CRYPTO_API_H__

