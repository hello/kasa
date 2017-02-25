/*
 * kernel/private/drivers/ambarella/crypto/crypto_api.c
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



#include <crypto/hash.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/scatterlist.h>
#include <linux/timex.h>
#include <linux/types.h>
#include <linux/crypto.h>

#include <amba_crypto.h>
#include "crypto_api.h"

typedef enum {
	AMBA_CRYPTO_ENCRYPT = 0,
	AMBA_CRYPTO_DECRYPT,
} amba_crypto_cipher;

#define SHA1_UPDATE    (256)
#define MD5_UPDATE    (256)
#define CIPHER_BLOCK			(1 << 10)
#define CIPHER_PADDING			(0xAB)

#define ALGO_NAME_LENGTH		(32)

typedef struct amba_crypto_obj_s {
	u32		cipher_alloced;
	u8		cipher_algo[ALGO_NAME_LENGTH];
	u8	 	default_algo[ALGO_NAME_LENGTH];
	struct crypto_blkcipher * tfm;
	amba_crypto_key_t key;
	amba_crypto_info_t info;
} amba_ctypto_obj_t;

static struct amba_crypto_obj_s G_amba_crypto = {
	.cipher_alloced		= 0,
	.default_algo	= "aes",
	.tfm			= NULL,
};

static int amba_crypto_alloc_blkcipher(void)
{
	const char * algo = G_amba_crypto.cipher_algo;
	if (G_amba_crypto.cipher_alloced == 0) {
		G_amba_crypto.tfm = crypto_alloc_blkcipher(algo, 0, CRYPTO_ALG_ASYNC);

		// failed to load cipher, use default cipher algo
		if (IS_ERR(G_amba_crypto.tfm)) {
			printk(KERN_DEBUG "Failed to load transform for [%s : %ld]. Using default transform [%s].\n",
				algo, PTR_ERR(G_amba_crypto.tfm), G_amba_crypto.default_algo);
			algo = G_amba_crypto.default_algo;
			G_amba_crypto.tfm = crypto_alloc_blkcipher(algo, 0, CRYPTO_ALG_ASYNC);
			if (IS_ERR(G_amba_crypto.tfm)) {
				printk(KERN_DEBUG "Failed to load transform for %s: %ld.\n",
					algo, PTR_ERR(G_amba_crypto.tfm));
				return -1;
			}
		}
	}
	G_amba_crypto.cipher_alloced = 1;
	return 0;
}

static int amba_crypto_free_blkcipher(void)
{
	if (G_amba_crypto.tfm)
		crypto_free_blkcipher(G_amba_crypto.tfm);
	G_amba_crypto.tfm = NULL;
	G_amba_crypto.cipher_alloced = 0;
	return 0;
}

static int amba_crypto_blkcipher(struct blkcipher_desc *desc,
	struct scatterlist *sg, amba_crypto_cipher enc, u32 b_size)
{
	u32 retv;
	if (enc == AMBA_CRYPTO_ENCRYPT) {
		retv = crypto_blkcipher_encrypt(desc, sg, sg, b_size);
		if (retv) {
			printk(KERN_DEBUG "Encryption failed !\n");
			amba_crypto_free_blkcipher();
			return -EFAULT;
		}
	} else {
		retv = crypto_blkcipher_decrypt(desc, sg, sg, b_size);
		if (retv) {
			printk(KERN_DEBUG "Decryption failed !\n");
			amba_crypto_free_blkcipher();
			return -EFAULT;
		}
	}
	return 0;
}

static int amba_crypto_do_cipher(u8 *cipher_buffer, amba_crypto_info_t *info,
	amba_crypto_cipher enc)
{
	struct crypto_blkcipher * tfm;
	struct blkcipher_desc desc;
	struct scatterlist sg;
	const char iv[128];
	u8 *text_addr = NULL;
	u32 iv_len, total_bytes;

	printk(KERN_DEBUG "amba_crypto_do_cipher start !\n");

	if (amba_crypto_alloc_blkcipher() < 0)
		return -EFAULT;

	tfm = G_amba_crypto.tfm;

	// set plain text, cipher text and IV
	desc.tfm = tfm;
	desc.flags = 0;
	iv_len = crypto_blkcipher_ivsize(tfm);
	if (iv_len) {
		memset(&iv, 0xFF, iv_len);
		crypto_blkcipher_set_iv(tfm, iv, iv_len);
	}
	text_addr = info->text;
	total_bytes = info->total_bytes;

	sg_init_table(&sg, 1);
	while (total_bytes > CIPHER_BLOCK) {
		memcpy(cipher_buffer, text_addr, CIPHER_BLOCK);
		sg_set_buf(&sg, cipher_buffer, CIPHER_BLOCK);
		printk(KERN_DEBUG "start cryption (%d bit key, %d byte blocks).",
			G_amba_crypto.key.key_len * 8, CIPHER_BLOCK);
		amba_crypto_blkcipher(&desc, &sg, enc, CIPHER_BLOCK);
		memcpy(text_addr, cipher_buffer, CIPHER_BLOCK);
		total_bytes -= CIPHER_BLOCK;
		text_addr += CIPHER_BLOCK;
	}
	memset(cipher_buffer, CIPHER_PADDING, CIPHER_BLOCK);
	memcpy(cipher_buffer, text_addr, total_bytes);
	sg_set_buf(&sg, cipher_buffer, CIPHER_BLOCK);
	printk(KERN_DEBUG "start cryption (%d bit key, %d byte blocks).",
		G_amba_crypto.key.key_len * 8, CIPHER_BLOCK);
	amba_crypto_blkcipher(&desc, &sg, enc, CIPHER_BLOCK);
	memcpy(text_addr, cipher_buffer, total_bytes);

	printk(KERN_DEBUG "amba_crypto_do_cipher done !\n");
	return 0;
}

int amba_crypto_set_algo(amba_crypto_context_t *context, struct amba_crypto_algo_s __user *arg)
{
	amba_crypto_algo_t algo;

	printk(KERN_DEBUG "amba_crypto_set_algo start\n");
	if (copy_from_user(&algo, arg, sizeof(algo)))
		return -EFAULT;

	if (algo.cipher_algo != NULL) {
		printk(KERN_DEBUG "cipher_algo name : [%s].\n", algo.cipher_algo);
		strncpy(G_amba_crypto.cipher_algo, algo.cipher_algo, ALGO_NAME_LENGTH);
		amba_crypto_free_blkcipher();
	}
	if (algo.default_algo != NULL) {
		printk(KERN_DEBUG "default_algo name : [%s].\n", algo.default_algo);
		strncpy(G_amba_crypto.default_algo, algo.default_algo, ALGO_NAME_LENGTH);
		amba_crypto_free_blkcipher();
	}

	printk(KERN_DEBUG "amba_crypto_set_algo done\n");
	return 0;
}


int amba_crypto_set_key(amba_crypto_context_t *context, struct amba_crypto_key_s __user *arg)
{
	amba_crypto_key_t *crypto_key;
	u32 retv;

	printk(KERN_DEBUG "amba_crypto_set_key start\n");
	crypto_key = &G_amba_crypto.key;
	if (copy_from_user(crypto_key, arg, sizeof(*crypto_key)))
		return -EFAULT;

	if (amba_crypto_alloc_blkcipher() < 0)
		return -EFAULT;

	// set cipher key
	retv = crypto_blkcipher_setkey(G_amba_crypto.tfm, crypto_key->key, crypto_key->key_len);
	if (retv) {
		printk(KERN_DEBUG "Failed to set crypto key [%s], flags = %x.\n",
			G_amba_crypto.key.key,
			crypto_blkcipher_get_flags(G_amba_crypto.tfm));
		amba_crypto_free_blkcipher();
		return -EFAULT;
	}

	printk(KERN_DEBUG "amba_crypto_set_key done\n");
	return 0;
}


int amba_crypto_encrypt(amba_crypto_context_t *context, struct amba_crypto_info_s __user *arg)
{
	amba_crypto_info_t *info;

	printk(KERN_DEBUG "amba_crypto_encrypt start !\n");
	info = &G_amba_crypto.info;
	if (copy_from_user(info, arg, sizeof(*info)))
		return -EFAULT;

	if (amba_crypto_do_cipher(context->buffer, info, AMBA_CRYPTO_ENCRYPT) < 0)
		return -EFAULT;

	printk(KERN_DEBUG "amba_crypto_encrypt done !\n");
	return 0;
}


int amba_crypto_decrypt(amba_crypto_context_t *context, struct amba_crypto_info_s __user *arg)
{
	amba_crypto_info_t *info;

	printk(KERN_DEBUG "amba_crypto_decrypt start !\n");
	info = &G_amba_crypto.info;
	if (copy_from_user(info, arg, sizeof(*info)))
		return -EFAULT;

	if (amba_crypto_do_cipher(context->buffer, info, AMBA_CRYPTO_DECRYPT) < 0)
		return -EFAULT;

	printk(KERN_DEBUG "amba_crypto_decrypt done !\n");
	return 0;
}

int amba_crypto_do_sha(u8 *cipher_buffer, amba_crypto_sha_md5_t *info)
{
	struct scatterlist sg;
	struct crypto_hash *tfm;
	struct hash_desc desc;
	u8 *text_addr = NULL;
	u8 *out= NULL;
	u32 total_bytes;
	int ret;

	tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "failed to load transform for sha: %ld\n",  PTR_ERR(tfm));
		return -1;
	}

	desc.tfm = tfm;
	desc.flags = 0;

	text_addr = info->in;
	total_bytes = info->total_bytes;
	out = info->out;

	sg_init_table(&sg, 1);
	memcpy(cipher_buffer,text_addr,SHA1_UPDATE);
	sg_set_buf(&sg, cipher_buffer, SHA1_UPDATE);
	ret = crypto_hash_init(&desc);
	if(ret)
		printk("error crypto_hash_init\r\n");
	while(total_bytes > SHA1_UPDATE) {
		memcpy(cipher_buffer,text_addr,SHA1_UPDATE);
		sg_set_buf(&sg, cipher_buffer, SHA1_UPDATE);
		ret = crypto_hash_update(&desc, &sg, SHA1_UPDATE);
		if (ret)
			printk("error in crypto_hash_update\r\n");

		total_bytes -= SHA1_UPDATE;
		text_addr += SHA1_UPDATE;
	}

	memcpy(cipher_buffer,text_addr,total_bytes);
	sg_set_buf(&sg,cipher_buffer,total_bytes);
	ret = crypto_hash_update(&desc, &sg, total_bytes);
	if (ret)
		printk("error in crypto_hash_update\r\n");
	ret = crypto_hash_final(&desc, out);
	return ret;
}

int amba_crypto_sha(amba_crypto_context_t *context, struct amba_crypto_sha_md5_s __user *arg)
{
	amba_crypto_sha_md5_t info;

	if (copy_from_user(&info, arg, sizeof(amba_crypto_sha_md5_t)))
		return -EFAULT;

	if (amba_crypto_do_sha(context->buffer, &info) < 0)
		return -EFAULT;

	if (copy_to_user(arg, &info, sizeof(amba_crypto_sha_md5_t)))
		return -EFAULT;

	printk(KERN_DEBUG "amba_crypto_decrypt done !\n");
	return 0;
}

int amba_crypto_do_md5(u8 *cipher_buffer, amba_crypto_sha_md5_t *info)
{
	struct scatterlist sg;
	struct crypto_hash *tfm;
	struct hash_desc desc;
	u8 *text_addr = NULL;
	u8 *out= NULL;
	u32 total_bytes;
	int ret;

	tfm = crypto_alloc_hash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "failed to load transform for sha: %ld\n",  PTR_ERR(tfm));
		return -1;
	}

	desc.tfm = tfm;
	desc.flags = 0;

	text_addr = info->in;
	total_bytes = info->total_bytes;
	out = info->out;

	sg_init_table(&sg, 1);
	memcpy(cipher_buffer,text_addr,MD5_UPDATE);
	sg_set_buf(&sg, cipher_buffer, MD5_UPDATE);
	ret = crypto_hash_init(&desc);
	if(ret)
		printk("error crypto_hash_init\r\n");
	while(total_bytes > MD5_UPDATE) {
		memcpy(cipher_buffer,text_addr,MD5_UPDATE);
		sg_set_buf(&sg, cipher_buffer, MD5_UPDATE);
		ret = crypto_hash_update(&desc, &sg, MD5_UPDATE);
		if (ret)
			printk("error in crypto_hash_update\r\n");

		total_bytes -= MD5_UPDATE;
		text_addr += MD5_UPDATE;
	}

	memcpy(cipher_buffer,text_addr,total_bytes);
	sg_set_buf(&sg,cipher_buffer,total_bytes);
	ret = crypto_hash_update(&desc, &sg, total_bytes);
	if (ret)
		printk("error in crypto_hash_update\r\n");
	ret = crypto_hash_final(&desc, out);
	return ret;
}


int amba_crypto_md5(amba_crypto_context_t *context, struct amba_crypto_sha_md5_s __user *arg)
{
	amba_crypto_sha_md5_t info;

	if (copy_from_user(&info, arg, sizeof(amba_crypto_sha_md5_t)))
		return -EFAULT;

	if (amba_crypto_do_md5(context->buffer, &info) < 0)
		return -EFAULT;

	if (copy_to_user(arg, &info, sizeof(amba_crypto_sha_md5_t)))
		return -EFAULT;

	printk(KERN_DEBUG "amba_crypto_decrypt done !\n");
	return 0;
}

