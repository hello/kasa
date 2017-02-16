/**
 * bld/memmove.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

/*-------------------------------------------------------------------------
 void *memmove(void* dst, const void* src, unsigned int n)

 Copy block of memory, it's safe than memcpy

 Input	: void* dst		: destination address
	  const void* src	: source address
 	  unsigned int n	: memory size to be copied
 Output	: none
 Return	: destination address
 Note	: none
-------------------------------------------------------------------------*/
void *memmove(void *dst,const void *src, unsigned int n)
{
	char *tmp, *s;

	if (src == dst)
		return dst;

	if (dst <= src) {
		tmp = (char *) dst;
		s = (char *) src;
		while (n--)
			*tmp++ = *s++;
		}
	else {
		tmp = (char *) dst + n;
		s = (char *) src + n;
		while (n--)
			*--tmp = *--s;
		}

	return dst;
}

