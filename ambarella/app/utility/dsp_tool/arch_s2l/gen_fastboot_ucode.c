/**
 * gen_fastboot_ucode.c (for S2L)
 *
 * History:
 *	2014/10/22 - [Tao Wu] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char 		u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned int       u32; /**< UNSIGNED 32-bit data type */

int get_file_size(const char *path)
{
	int filesize = -1;
	struct stat statbuff;
	if(stat(path, &statbuff) < 0) {
		perror("stat");
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

int gen_default_binary_out (char * f_idspcfg, char * f_default_binary,
	char * f_default_binary_out, int head_idspcfg, int head_default_binary)
{
	int len = -1;
	int fd_idspcfg = -1;
	int fd_default_binary = -1;
	int fd_default_binary_out = -1;

	int size_idspcfg = -1;
	int size_default_binary = -1;

	u8 *buf_idspcfg = NULL;
	u8 *buf_default_binary = NULL;

	size_idspcfg = get_file_size(f_idspcfg);
	size_default_binary = get_file_size(f_default_binary);
	if (size_idspcfg < 0) {
		printf("Input file %s size is wrong\n", f_idspcfg);
		return -1;
	}
	if (size_default_binary <0) {
		printf("Input file %s size is wrong\n", f_default_binary);
		return -1;
	}

	printf("[%s] size is [%u].\n", f_idspcfg, size_idspcfg);
	printf("[%s] size is [%u].\n", f_default_binary, size_default_binary);
	printf("==================\n", f_default_binary, size_default_binary);

	do {
		if ((fd_idspcfg = open(f_idspcfg, O_RDONLY )) < 0) {
			printf("Failed to open file [%s].\n", f_idspcfg);
			break;
		}
		if ((fd_default_binary = open(f_default_binary, O_RDONLY )) < 0) {
			printf("Failed to open file [%s].\n", f_default_binary);
			break;
		}
		if ((fd_default_binary_out = open(f_default_binary_out,
				O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
			printf("Failed to open file [%s].\n", f_default_binary_out);
			break;
		}

		buf_idspcfg = (u8 *) malloc (size_idspcfg);
		if (!buf_idspcfg) {
			printf("Not enough memory for IDSP config binary.\n");
			break;
		}

		buf_default_binary = (u8 *) malloc (size_default_binary);
		if (!buf_default_binary) {
			printf("Not enough memory for default_binary.\n");
			break;
		}

		memset(buf_idspcfg, 0, size_idspcfg);
		memset(buf_default_binary, 0, size_default_binary);

		len = read(fd_idspcfg, buf_idspcfg, size_idspcfg);
		if (len != size_idspcfg) {
			printf("Read %s error\n", f_idspcfg);
			break;
		}

		len = read(fd_default_binary, buf_default_binary, size_default_binary);
		if (len != size_default_binary) {
			printf("Read %s  error\n", f_default_binary);
			break;
		}

		memcpy (buf_default_binary + head_default_binary, buf_idspcfg + head_idspcfg,
			size_idspcfg - head_idspcfg);

		printf("[%s] START: [%u]%x, %x, %x, %x.\n", f_idspcfg,
			head_idspcfg,
			buf_idspcfg[head_idspcfg],buf_idspcfg[head_idspcfg+1],
			buf_idspcfg[head_idspcfg+2], buf_idspcfg[head_idspcfg+3] );
		printf("[%s] END: [%u]%x, %x, %x, %x.\n", f_idspcfg,
			(size_idspcfg-4),
			buf_idspcfg[size_idspcfg-4],buf_idspcfg[size_idspcfg-3],
			buf_idspcfg[size_idspcfg-2], buf_idspcfg[size_idspcfg-1] );

		printf("[%s] START: [%u]%x, %x, %x, %x.\n", f_default_binary_out,
			head_default_binary,
			buf_default_binary[head_default_binary],
			buf_default_binary[head_default_binary+1],
			buf_default_binary[head_default_binary+2],
			buf_default_binary[head_default_binary+3]);

		printf("[%s] END: [%u]%x, %x, %x, %x.\n", f_default_binary_out,
			(head_default_binary + size_idspcfg -head_idspcfg-4),
			buf_default_binary[head_default_binary + size_idspcfg -head_idspcfg-4],
			buf_default_binary[head_default_binary + size_idspcfg -head_idspcfg-3],
			buf_default_binary[head_default_binary + size_idspcfg -head_idspcfg-2],
			buf_default_binary[head_default_binary + size_idspcfg -head_idspcfg-1]);

		len = write(fd_default_binary_out, buf_default_binary, size_default_binary);
		if (len != size_default_binary) {
			printf("Read %s  error\n", f_default_binary_out);
			break;
		}
	}while (0);

	if (fd_idspcfg > 0) {
		close(fd_idspcfg);
	}
	if (fd_default_binary > 0)  {
		close(fd_default_binary);
	}
	if (fd_default_binary_out > 0) {
		close(fd_default_binary_out);
	}
	if (buf_idspcfg) {
		free(buf_idspcfg);
	}
	if (buf_default_binary) {
		free(buf_default_binary);
	}

	return 0;
}

int gen_default_mctf(char * f_mctf, char * f_default_mctf,
						int head_mctf)
{
	int len = -1;
	int fd_mctf = -1;
	int fd_default_mctf = -1;
	int size_mctf = -1;
	u8 *buf_mctf = NULL;

	size_mctf = get_file_size(f_mctf);

	if (size_mctf < 0) {
		printf("Input file %s size is wrong\n", f_mctf);
		return -1;
	}
	printf("[%s] size is [%u].\n", f_mctf, size_mctf);


	do {
		if ((fd_mctf = open(f_mctf, O_RDONLY )) < 0) {
			printf("Failed to open file [%s].\n", f_mctf);
			break;
		}

		if ((fd_default_mctf = open(f_default_mctf, O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
			printf("Failed to open file [%s].\n", f_default_mctf);
			break;
		}

		buf_mctf = (u8 *) malloc (size_mctf);
		if (!buf_mctf) {
			printf("Not enough memory for MCTF binary.\n");
			break;
		}

		memset(buf_mctf, 0, size_mctf);

		len = read(fd_mctf, buf_mctf, size_mctf);
		if (len != size_mctf) {
			printf("Read %s error\n", f_mctf);
			break;
		}

		len = write(fd_default_mctf, buf_mctf + head_mctf, size_mctf - head_mctf);
		if (len != (size_mctf - head_mctf)) {
			printf("Write %s  error\n", f_default_mctf);
			break;
		}
	} while(0);

	if (fd_mctf > 0) {
		close(fd_mctf);
	}
	if (fd_default_mctf > 0) {
		close(fd_default_mctf);
	}
	if (buf_mctf) {
		free(buf_mctf);
	}

	return 0;
}

void usage(char **argv)
{
	printf("Usage: \n");
	printf("1. default_binary_out.bin:\n"\
		"CMD: $%s idsp_dump_bin_0.bin default_binary.bin default_binary_out.bin 64 8208\n"
		"Input:\t idsp_dump_bin_0.bin(idspcfg.bin) is generate by \"test_ituner -D 0\"; Offset [64]\n"\
		"\t default_binary.bin is from ucode release; Offset [8208]\n"\
		"Output:\t default_binary_out.bin is generated binary which contain idspcfg\n", argv[0]);
	printf("\n");
	printf("2. default_mctf.bin:\n"\
		"CMD: $%s idsp_dump_bin_100.bin default_mctf.bin 64\n"\
		"Input:\t idsp_dump_bin_100.bin is generate by \"test_ituner -D 100\"; Offset [64]\n"\
		"Output:\t default_mctf.bin is generated mctf binary\n", argv[0]);
}

int main(int argc, char **argv)
{
	int ret = -1;

	if (argc == 6) {
		ret = gen_default_binary_out(argv[1], argv[2], argv[3], atoi(argv[4]), atoi(argv[5]));
	} else if (argc == 4) {
		ret = gen_default_mctf(argv[1], argv[2], atoi(argv[3]));
	} else {
		usage(argv);
		ret = 0;
	}

	if (ret < 0 ) {
		printf("Failed.\n");
	}

	return ret;
}

