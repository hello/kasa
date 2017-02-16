/**
 * bld/host_permutate.c
 *
 * For permuatating the strings in generating firmware combinations.
 *
 * History:
 *    2008/11/03 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void combinations_recursive(char **elems,
			    int elems_size,
			    unsigned long req_len,
			    int *pos,
			    int pos_size,
			    unsigned long depth,
			    unsigned long margin)
{
	unsigned long ii;

	if (depth >= req_len) {
		for (ii = 0; ii < pos_size; ++ii) {
			if (ii != 0)
				printf("_");
			printf("%s", elems[pos[ii]]);
		}
		printf(" ");
		return;
	}

	if ((elems_size - margin) < (req_len - depth))
		return;

	for (ii = margin; ii < elems_size; ++ii) {
		pos[depth] = ii;
		combinations_recursive(elems, elems_size, req_len,
				       pos, pos_size,
				       depth + 1, ii + 1);
	}

	return;
}

int main(int argc, char **argv)
{
	int n, k;
	int *pos;
	char **elems;
	int minimal = 0;

	n = argc - 1;
	elems = &argv[1];

	if (n == 0)
		return 0;

	if (strcmp(argv[1], "-m") == 0) {
		n--;
		if (n == 0)
			return 0;
		minimal = 1;
		elems = &argv[2];
	}

	pos = (int *) malloc(sizeof(int) * n);
	memset(pos, 0x0, sizeof(int) * n);
	if (minimal) {
		combinations_recursive(elems, n, 1, pos, 1, 0, 0);
		combinations_recursive(elems, n, n, pos, n, 0, 0);
	} else {
		for (k = 1; k <= n; k++) {
			combinations_recursive(elems, n, k, pos, k, 0, 0);
		}
	}

	free(pos);

	return 0;
}
