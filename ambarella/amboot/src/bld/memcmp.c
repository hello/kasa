/**
 * bld/memcmp.c
 *
 * History:
 *    2005/03/06 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

int memcmp(const void *dst, const void *src, unsigned int n)
{
	const unsigned char *_dst = (unsigned char *) dst;
	const unsigned char *_src = (unsigned char *) src;

	while (n--) {
		if (*_dst != *_src) {
			return -n;
		}
		_dst++;
		_src++;
	}

	return 0;
}

