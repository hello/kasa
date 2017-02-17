/**
 * bld/host_permutate.c
 *
 * For permuatating the strings in generating firmware combinations.
 *
 * History:
 *    2008/11/03 - [Charles Chiou] created file
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
