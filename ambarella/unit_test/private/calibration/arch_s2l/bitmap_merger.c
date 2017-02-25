/*
 * bitmap_merger.c
 *
 * Histroy:
 *   Dec 5, 2013 - [Shupeng Ren] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

#define MERGE_MAX_COUNT	3
#define MAX_STRING_LEN		512

static int merge_flag = 0;
static char *file_name[MERGE_MAX_COUNT];

#define NO_ARG			0
#define HAS_ARG			1

static struct option long_options[] = {
	{"or", HAS_ARG, 0, 'o'},
	{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "o: ";

static const struct hint_s hint[] = {
	{"", "\t\t3 params for merge bitmap. For example :\n\t\t bitmap_merger -o srcfile1,srcfile2, destfile"}
};

static void usage(void)
{
	int i;

	printf("\nbitmap_merger usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("%s\n", hint[i].str);
	}
}

static int get_multi_arg(char *optarg, char *argarr[], int argcnt)
{
	int i = 0;
	char *delim = ",:";
	char *ptr = NULL;

	ptr = strtok(optarg, delim);

	if (ptr){
		argarr[0] = malloc(strlen(ptr) + 1);
		if (!argarr[0]){
			printf("get_multi_arg: malloc failed!\n");
			return -1;
		}
		strncpy(argarr[0], ptr, strlen(ptr) + 1);
	}

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;

		argarr[i] = malloc(strlen(ptr) + 1);
		if (!argarr[i]){
			printf("get_multi_arg: malloc failed!\n");
			return -1;
		}

		strncpy(argarr[i], ptr, strlen(ptr) + 1);
	}

	if (i < argcnt)
		return -1;

	return 0;
}

static int init_param(int argc, char *argv[])
{
	int ch;
	int option_index = 0;
	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'o':
			merge_flag = 1;
			if (get_multi_arg(optarg, file_name, ARRAY_SIZE(file_name)) < 0) {
				printf("need %d args for opt %c!\n", ARRAY_SIZE(file_name), ch);
				return -1;
			}
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

//merge_bad_pixel and return bad pixel count
int merge_bad_pixel(const char *file_in_name1, const char *file_in_name2, const char *file_out_name)
{
	int	fd_out = -1;
	int	fd_in1 = -1;
	int	fd_in2 = -1;
	char	buf_in1[4096] = {0};
	char	buf_in2[4096] = {0};
	char	buf_out[4096] = {0};
	int	num_in1 = 0;
	int	num_in2 = 0;
	int	fsz_in1 = 0;
	int	fsz_in2 = 0;
	int	bcnt = 0;
	char	mask;
	int	bad_pixel_cnt = 0;
	int	i;
	int	j;

	// check parameters
	if (!file_in_name1 || strlen(file_in_name1) == 0 ||
		!file_in_name2 ||strlen(file_in_name2) == 0 ||
		!file_out_name || strlen(file_out_name) == 0){
		printf("%s: parameter is error!\n", __func__);
		return -1;
	}

	fd_in1 = open(file_in_name1, O_RDONLY, 0);
	if(fd_in1 < 0){
		printf("%s: fopen %s file fail\n", __func__, file_in_name1);
		return -1;
	}

	fd_in2 = open(file_in_name2, O_RDONLY, 0);
	if(fd_in2 < 0){
		printf( "%s: fopen %s file fail\n", __func__, file_in_name2);
		close(fd_in1);
		return -1;
	}

	fsz_in1 = lseek(fd_in1, 0, SEEK_END);
	lseek(fd_in1, 0, SEEK_SET);
	fsz_in2 = lseek(fd_in2, 0, SEEK_END);
	lseek(fd_in2,0, SEEK_SET);

	// check if two bitmap file size is the same
	if (fsz_in1 != fsz_in2){
		printf("%s file size != %s file size", file_in_name1, file_in_name2);
		close(fd_in1);
		close(fd_in2);
		return -1;
	}

	fd_out = open(file_out_name, O_CREAT | O_TRUNC | O_WRONLY, 0777);
	if (fd_out < 0){
		printf("%s: open %s file fail\n", __func__, file_out_name);
		close(fd_in1);
		close(fd_in2);
		return -1;
	}

	bad_pixel_cnt = 0;
	while (1) {
		memset(buf_in1, 0, sizeof(buf_in1));
		memset(buf_in2, 0, sizeof(buf_in2));
		memset(buf_out, 0, sizeof(buf_out));

		if ((num_in1 = read(fd_in1, buf_in1, sizeof(buf_in1))) < 0){
			printf("%s: read %s file fail\n", __func__, file_in_name1);
			return -1;
		}
		if ((num_in2 = read(fd_in2, buf_in2, sizeof(buf_in2))) < 0){
			printf("%s: read %s file fail\n",__func__,  file_in_name2);
			return -1;
		}
		if (num_in1 != num_in2){
			printf("%s: num_in1 != num_in2\n", __func__);
			return -1;
		}

		for (i=0; i<num_in1; i++) {
			buf_out[i] = buf_in1[i] | buf_in2[i];
			for (j=0; j<8; j++) {
			    mask = ((1<<j) & 0xff);
			    if (buf_out[i] & mask) {
			        bad_pixel_cnt++;
			    }
			}
		}

		if (num_in1 != write(fd_out, buf_out, num_in1)){
			printf("%s: write error!\n", __func__);
			return -1;
		}

		bcnt += num_in1;
		if (bcnt >= fsz_in1)
		    break;
	}

	close(fd_out);
	close(fd_in1);
	close(fd_in2);

	// error bad pixel
	if (bad_pixel_cnt == 0)
		return -1;

	return bad_pixel_cnt;
}

int main(int argc, char *argv[])
{
	int i =0;
	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0){
		return -1;
	}

	if (merge_flag){
		if (merge_bad_pixel(file_name[0], file_name[1], file_name[2]) < 0){
			return -1;
		}
	}

	// free memory
	for (i = 0; i < ARRAY_SIZE(file_name); i++){
		free(file_name[i]);
		file_name[i] = NULL;
	}

	return 0;
}

