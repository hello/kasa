
/*
 * amba_crypto.h
 *
 * History:
 *	2011/05/03 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_CRYPTO_H__
#define __AMBA_CRYPTO_H__

#include <linux/ioctl.h>

#define AMBA_CRYPTO_MAGIC			'c'

/*
 * ambarella crypto APIs	- 'c'
 */
enum {
	IOC_CRYPTO_BASE_NUM = 0,
	IOC_CRYPTO_SET_ALGO,
	IOC_CRYPTO_SET_KEY,
	IOC_CRYPTO_ENCRYPTE,
	IOC_CRYPTO_DECRYPTE,
	IOC_CRYPTO_SHA,
	IOC_CRYPTO_MD5,
};

#define AMBA_IOC_CRYPTO_SET_ALGO	_IOW(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_SET_ALGO, struct amba_crypto_algo_s *)
#define AMBA_IOC_CRYPTO_SET_KEY		_IOW(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_SET_KEY, struct amba_crypto_key_s *)
#define AMBA_IOC_CRYPTO_ENCRYPTE		_IOWR(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_ENCRYPTE, struct amba_crypto_info_s *)
#define AMBA_IOC_CRYPTO_DECRYPTE		_IOWR(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_DECRYPTE, struct amba_crypto_info_s *)
#define AMBA_IOC_CRYPTO_SHA            _IOWR(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_SHA, struct amba_crypto_sha_md5_s *)
#define AMBA_IOC_CRYPTO_MD5            _IOWR(AMBA_CRYPTO_MAGIC, IOC_CRYPTO_MD5, struct amba_crypto_sha_md5_s *)

#define AMBA_CRYPTO_ALGO_NAME_LEN	(32)

typedef struct amba_crypto_algo_s {
	char		cipher_algo[AMBA_CRYPTO_ALGO_NAME_LEN];
	char		default_algo[AMBA_CRYPTO_ALGO_NAME_LEN];
} amba_crypto_algo_t;

typedef struct amba_crypto_key_s {
	u8 *		key;
	u32		key_len;
} amba_crypto_key_t;

typedef struct amba_crypto_info_s {
	u8 *		text;
	u32		total_bytes;
} amba_crypto_info_t;

typedef struct amba_crypto_sha_md5_s {
	u8 *		in;
	u32		total_bytes;
	u8 *		out;
} amba_crypto_sha_md5_t;

#endif	//  __AMBA_CRYPTO_H__

