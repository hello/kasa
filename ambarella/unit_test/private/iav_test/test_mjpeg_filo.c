/*
 * test_mjpeg_filo.c
 *
 * History:
 *    2010/9/6 - [Louis Sun] create it to test memcpy by GDMA
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <signal.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include "datatx_lib.h"

#define GET_STREAM_FILO_ENABLE		1
#define MAX_ENCODE_STREAM_NUM	4
#define BASE_PORT			(2000)
#define NO_ARG		0
#define HAS_ARG		1
#define MAX_LENGTH			(256)
#define GET_STREAMID(x)		((((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) ? -1: (x))
// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

typedef struct transfer_method {
	trans_method_t method;
	int port;
	char filename[256];
	int filo_flag;
} transfer_method;

enum {
	NO_STREAM_INPUT = 0,
	STREAM_INPUT = 1,
};

typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
} stream_encoding_state_t;


static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static transfer_method stream_transfer[MAX_ENCODE_STREAM_NUM];
static int default_transfer_method = TRANS_METHOD_NFS;
static int transfer_port = BASE_PORT;
static int init_none, init_nfs, init_tcp, init_usb, init_stdout;
static int fetch_no = 1;
static int current_stream = -1;
static int current_stream_id;
static int k;

const char *default_filename;
const char *default_filename_nfs = "/mnt/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";

static int fd_iav;

static int deinit_transfer(void)
{
	if (init_none > 0)
		init_none = amba_transfer_deinit(TRANS_METHOD_NONE);
	if (init_nfs > 0)
		init_nfs = amba_transfer_deinit(TRANS_METHOD_NFS);
	if (init_tcp > 0)
		init_tcp = amba_transfer_deinit(TRANS_METHOD_TCP);
	if (init_usb)
		init_usb = amba_transfer_deinit(TRANS_METHOD_USB);
	if (init_stdout)
		init_stdout = amba_transfer_deinit(TRANS_METHOD_STDOUT);
	if (init_none < 0 || init_nfs < 0 || init_tcp < 0 || init_usb < 0 || init_stdout < 0)
		return -1;
	return 0;
}

static void sigstop()
{
	deinit_transfer();
	exit(1);
}

int map_bsb(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_BSB;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	bsb_size = querybuf.length;
	bsb_mem = mmap(NULL, bsb_size * 2, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (bsb_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int write_video_file(struct iav_framedesc *framedesc)
{
	static unsigned int whole_pic_size = 0;
	u32 pic_size = framedesc->size;
	int fd = encoding_states[framedesc->id].fd;
	whole_pic_size  += (pic_size & (~(1<<23)));

	if (pic_size>>23) {
		//end of frame
		pic_size = pic_size & (~(1<<23));
		//padding some data to make whole picture to be 32 byte aligned
		pic_size += (((whole_pic_size + 31) & ~31)- whole_pic_size);
		//rewind whole pic size counter
		// printf("whole %d, pad %d \n", whole_pic_size, (((whole_pic_size + 31) & ~31)- whole_pic_size));
		whole_pic_size = 0;
	}

	if (amba_transfer_write(fd, bsb_mem + framedesc->data_addr_offset, pic_size,
		stream_transfer[current_stream_id].method) < 0) {
		perror("Failed to write streams into file!\n");
		return -1;
	}
	return 0;
}

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename", "specify filename"},
	{"", "\t\tuse tcp (ps) to receive bitstream"},
	{"", "\t\tuse usb (us_pc2) to receive bitstream"},
	{"", "\t\treceive data from BSB buffer, and write to stdout"},
	{"", "\t\tstream A"},
	{"", "\t\tstream B"},
	{"", "\t\tstream C"},
	{"", "\t\tstream D"},
	{"","\t\tspecify the number of the frames to fetch"},
};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"tcp",		NO_ARG,		0,	't'},
	{"usb",		NO_ARG,		0,	'u'},
	{"stdout",	NO_ARG,		0,	'o'},
	{"streamA",	NO_ARG,		0,	'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB",	NO_ARG,		0,	'B'},
	{"streamC",	NO_ARG,		0,	'C'},
	{"streamD",	NO_ARG,		0,	'D'},
	{"frame_no",	HAS_ARG,	0,	'n'},
	{0, 0, 0, 0}
};

static const char *short_options = "ABCDf:tuoen:";

void usage(void)
{
	int i;
	printf("test_mjpeg_filo usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

int config_transfer(void)
{
	int i;
	transfer_method *trans;
	char str[][16] = { "NONE", "NFS", "TCP", "USB", "STDOUT"};

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		trans = &stream_transfer[i];
		trans->port = transfer_port + i * 2;
		if (strlen(trans->filename) > 0) {
			if (trans->method == TRANS_METHOD_UNDEFINED)
				trans->method = TRANS_METHOD_NFS;
		} else {
			if (trans->method == TRANS_METHOD_UNDEFINED) {
				trans->method = default_transfer_method;
			}
			switch (trans->method) {
			case TRANS_METHOD_NFS:
			case TRANS_METHOD_STDOUT:
				if (strlen(trans->filename) == 0) {
					default_filename = default_filename_nfs;
				}
				break;
			case TRANS_METHOD_TCP:
			case TRANS_METHOD_USB:
				if (strlen(trans->filename) == 0) {
					default_filename = default_filename_tcp;
				}
				break;
			default:
				default_filename = NULL;
				break;
			}
			if (default_filename != NULL)
				strcpy(trans->filename, default_filename);
		}
		if (trans->filo_flag) {
			printf("Stream %c %s: %s\n", 'A' + i, str[trans->method], trans->filename);
		}
	}

	return 0;
}

static int init_param(int argc, char **argv)
{
	int i, ch;
	int option_index = 0;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		stream_transfer[i].method = TRANS_METHOD_UNDEFINED;
	}

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			stream_transfer[current_stream].filo_flag = GET_STREAM_FILO_ENABLE;
			break;
		case 'B':
			current_stream = 1;
			stream_transfer[current_stream].filo_flag = GET_STREAM_FILO_ENABLE;
			break;
		case 'C':
			current_stream = 2;
			stream_transfer[current_stream].filo_flag = GET_STREAM_FILO_ENABLE;
			break;
		case 'D':
			current_stream = 3;
			stream_transfer[current_stream].filo_flag = GET_STREAM_FILO_ENABLE;
			break;
		case 'f':
			strcpy(stream_transfer[current_stream].filename, optarg);
			break;
		case 't':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_TCP;
			}
			default_transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_USB;
			}
			default_transfer_method = TRANS_METHOD_USB;
			break;
		case 'o':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_STDOUT;
			}
			default_transfer_method = TRANS_METHOD_STDOUT;
			break;
		case 'n':
			fetch_no = atoi(optarg);
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	if (config_transfer() < 0) {
		printf("config_transfer error\n");
		return -1;
	}
	return 0;
}

//return 0 if it's not new session,  return 1 if it is new session
int is_new_session(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	int new_session = 0 ;
	if  (framedesc ->session_id != encoding_states[stream_id].session_id) {
		//a new session
		new_session = 1;
	}
	return new_session;
}

int update_session_data(struct iav_framedesc *framedesc, int new_session)
{
	int stream_id = framedesc->id;
	if (new_session) {
		encoding_states[stream_id].session_id = framedesc->session_id;
	}
	return 0;
}

static int  write_stream(void)
{
	int	new_session ;
	struct iav_querydesc query_desc;
	struct iav_framedesc *frame_desc;
	char write_file_name[MAX_LENGTH], filename[MAX_LENGTH];
	char file_type[8];
	char *f_type = "filo";
	int stream_id;
	int j;

	for (j = 0; j < MAX_ENCODE_STREAM_NUM; j++) {
		if (stream_transfer[j].filo_flag == GET_STREAM_FILO_ENABLE) {
			memset(&query_desc,0,sizeof(query_desc));
			frame_desc = &query_desc.arg.frame;
			query_desc.qid = IAV_DESC_FRAME;
			frame_desc->id = j;
			frame_desc->time_ms = 0; // Non-Blocking way
			frame_desc->instant_fetch = 1;
			current_stream_id = j;
		} else {
			continue;
		}

		if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) <0) {
			perror("IAV_IOC_QUERY_DESC");
			return -1;
		}

		stream_id = frame_desc->id;
		new_session = is_new_session(frame_desc);
		if (update_session_data(frame_desc, new_session) < 0) {
			printf("update session data failed \n");
			return -5;
		}

		if (new_session) {
			sprintf(file_type, "%s", "mjpeg");
			sprintf(write_file_name, "%s_%c_%d_%s",
					stream_transfer[current_stream_id].filename,
					'A' + current_stream_id,fetch_no,f_type);
			sprintf(filename, "%s.%s", write_file_name, file_type);
			if (amba_transfer_init(stream_transfer[current_stream_id].method ) < 0) {
				printf("init failed\n");
				return -2;
			}
			if ((encoding_states[stream_id].fd = amba_transfer_open(filename,
				stream_transfer[current_stream_id].method,
				stream_transfer[current_stream_id].port)) < 0) {
				printf("create file for write failed %s \n", filename);
				return -3;
			}
		}
		if (write_video_file(frame_desc) < 0 ) {
			return -4;
		}

		if (frame_desc->stream_end) {
			if (encoding_states[stream_id].fd > 0) {
				amba_transfer_close(encoding_states[stream_id].fd,
					stream_transfer[stream_id].method);
				encoding_states[stream_id].fd = -1;
			}
		}
	}
	return 0;
}

static int check_stream(void)
{
	int i;
	int count_flag = 0;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_transfer[i].filo_flag == GET_STREAM_FILO_ENABLE) {
			count_flag = STREAM_INPUT;
		}
	}
	if (count_flag == NO_STREAM_INPUT) {
		printf("the stream should be input!\n");
		return -1;
	}
	return 0;
}

int init_encoding_states(void)
{
	int i;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].fd = -1;
		encoding_states[i].session_id = -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}
	if (check_stream() < 0) {
		return -1;
	}
	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}
	init_encoding_states();
	for (k = 0; k < fetch_no; k++) {
		if (write_stream() < 0) {
			printf("FILO Write in error!\n");
			break;
		}
	}
	if (deinit_transfer() < 0) {
		return -1;
	}
	close(fd_iav);
	return 0;
}

