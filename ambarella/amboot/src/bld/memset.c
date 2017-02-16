/**
 * bld/memset.c
 *
 * History:
 *    2008/09/17 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

/*
 * Optimised C version of memset for the ARM.
 */
void *memset(void *s, int c, unsigned int n)
{
	unsigned char *cd = (unsigned char *) s;

	while (n) {
		*cd++ = (unsigned char) c;
		n--;
	}

	return cd;
}

