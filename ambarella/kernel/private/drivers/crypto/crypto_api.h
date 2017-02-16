/*
 * kernel/private/drivers/ambarella/crypto/crypto_api.h
 *
 * History:
 *	2011/05/05 - [Jian Tang] created file
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

