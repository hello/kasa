
/*
 * test_stream.c
 * the program can read bitstream from BSB buffer and dump to different files
 * in different recording session, if there is no stream being recorded, then this
 * program will be in idle loop waiting and do not exit until explicitly closed.
 *
 * this program may stop encoding if the encoding was started to be "limited frames",
 * or stop all encoding when this program was forced to quit
 *
 * History:
 *	2009/12/31 - [Louis Sun] modify to use EX functions
 *	2012/01/11 - [Jian Tang] add different transfer method for streams
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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


#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

//#define ENABLE_RT_SCHED

#define MAX_ENCODE_STREAM_NUM	(4)
#define BASE_PORT			(2000)
#define SVCT_PORT_OFFSET	(16)
#define MAX_SVCT_LAYERS	(3)

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)

//total frames we need to capture
static int md5_idr_number = -1;

//debug options
#undef DEBUG_PRINT_FRAME_INFO

#define ACCURATE_FPS_CALC

#define GET_STREAMID(x)	((((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) ? -1: (x))

typedef struct transfer_method {
	trans_method_t method;
	int port;
	char filename[256];
} transfer_method;

// the device file handle
int fd_iav;

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

static int nofile_flag = 0;
static int frame_info_flag = 0;
static int show_pts_flag = 0;
static int check_pts_flag = 0;
static int file_size_flag = 0;
static int file_size_mega_byte = 100;
static int remove_time_string_flag = 0;
static int split_svct_layer_flag = 0;

static int fps_statistics_interval = 300;
static int print_interval = 30;

//create files for writing
static int init_none, init_nfs, init_tcp, init_usb, init_stdout;
static transfer_method stream_transfer[MAX_ENCODE_STREAM_NUM];
static int default_transfer_method = TRANS_METHOD_NFS;
static int transfer_port = BASE_PORT;

// bitstream filename base
static char filename[256];
const char *default_filename;
const char *default_filename_nfs = "/mnt/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";

//verbose
static int verbose_mode = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

#define TRANSFER_OPTIONS_BASE		0
#define INFO_OPTIONS_BASE			10
#define TEST_OPTIONS_BASE			20
#define MISC_OPTIONS_BASE			40

enum numeric_short_options {
	FILENAME = TRANSFER_OPTIONS_BASE,
	NOFILE_TRANSER,
	TCP_TRANSFER,
	USB_TRANSFER,
	STDOUT_TRANSFER,

	TOTAL_FRAMES = INFO_OPTIONS_BASE,
	TOTAL_SIZE,
	FILE_SIZE,
	SAVE_FRAME_INFO,
	SHOW_PTS_INFO,
	CHECK_PTS_INFO,

	DURATION_TEST = TEST_OPTIONS_BASE,
	REMOVE_TIME_STRING,
	SPLIT_SVCT_LAYER,
};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"tcp",		NO_ARG,		0,	't'},
	{"usb",		NO_ARG,		0,	'u'},
	{"stdout",	NO_ARG,		0,	'o'},
	{"nofile",		NO_ARG,		0,	NOFILE_TRANSER},
	{"frame-test-only",	HAS_ARG,	0,	TOTAL_FRAMES},
	{"size",		HAS_ARG,	0,	TOTAL_SIZE},
	{"file-size",	HAS_ARG,	0,	FILE_SIZE},
	{"frame-info",	NO_ARG,		0,	SAVE_FRAME_INFO},
	{"show-pts",	NO_ARG,		0,	SHOW_PTS_INFO},
	{"check-pts",	NO_ARG,		0,	CHECK_PTS_INFO},
	{"rm-time",	NO_ARG,		0,	REMOVE_TIME_STRING},
	{"split-svct",	NO_ARG,		0,	SPLIT_SVCT_LAYER},
	{"fps-intvl",	HAS_ARG,	0,	'i'},
	{"frame-intvl",	HAS_ARG,	0,	'n'},
	{"streamA",	NO_ARG,		0,	'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB",	NO_ARG,		0,	'B'},
	{"streamC",	NO_ARG,		0,	'C'},
	{"streamD",	NO_ARG,		0,	'D'},
	{"verbose",	NO_ARG,		0,	'v'},
	{0, 0, 0, 0}
};

static const char *short_options = "f:tuoi:n:ABCDv";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename", "specify filename"},
	{"", "\t\tuse tcp (ps) to receive bitstream"},
	{"", "\t\tuse usb (us_pc2) to receive bitstream"},
	{"", "\t\treceive data from BSB buffer, and write to stdout"},
	{"", "\t\tjust receive data from BSB buffer, but do not write file"},
        {"frame-test-only", "\tspecify how many frames to encode"},
	{"size", "\tspecify how many bytes to encode"},
	{"size", "\tcreate new file to capture when size is over maximum (MB)"},
	{"", "\t\tgenerate frame info"},
	{"", "\t\tshow stream pts info"},
	{"", "\t\tcheck stream pts info"},
	{"", "\t\tremove time string from the file name"},
	{"", "\t\tsplit SVCT layers into different streams"},
	{"", "\t\tset fps statistics interval"},
	{"", "\tset frame/field statistic interval"},
	{"", "\t\tstream A"},
	{"", "\t\tstream B"},
	{"", "\t\tstream C"},
	{"", "\t\tstream D"},
	{"", "\t\tprint more information"},

};

void usage(void)
{
	int i;
	printf("test_stream usage:\n");
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


typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	u32 total_frames; // count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;  //count how many bytes encoded
	int pic_type;	  //picture type,  non changed during encoding, help to validate encoding state.
	u32 pts;
	u64 monotonic_pts;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start

#ifdef ACCURATE_FPS_CALC
	int 	 total_frames2;	//for statistics only
	struct timeval time2;	//for statistics only
#endif

	int fd_svct[MAX_SVCT_LAYERS];	//file descriptor for svct streams
	int svct_layers;	//count how many svct layers
} stream_encoding_state_t;

static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static stream_encoding_state_t old_encoding_states[MAX_ENCODE_STREAM_NUM];  //old states for statistics only

int init_param(int argc, char **argv)
{
	int i, ch;
	int option_index = 0;
	int current_stream = -1;
	transfer_method *trans;
	char str[][16] = { "NONE", "NFS", "TCP", "USB", "STDOUT"};

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		stream_transfer[i].method = TRANS_METHOD_UNDEFINED;
	}

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'A':
			current_stream = 0;
			break;
		case 'B':
			current_stream = 1;
			break;
		case 'C':
			current_stream = 2;
			break;
		case 'D':
			current_stream = 3;
			break;
		case 'f':
			current_stream = GET_STREAMID(current_stream);
			if (current_stream < 0) {
				strcpy(filename, optarg);
			} else {
				strcpy(stream_transfer[current_stream].filename, optarg);
			}
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
		case 'i':
			fps_statistics_interval = atoi(optarg);
			break;
		case 'n':
			print_interval = atoi(optarg);
			break;
		case NOFILE_TRANSER:
			current_stream = GET_STREAMID(current_stream);
			if (current_stream >= 0) {
				stream_transfer[current_stream].method = TRANS_METHOD_NONE;
			}
			default_transfer_method = TRANS_METHOD_NONE;
			break;
		case TOTAL_FRAMES:
			md5_idr_number = atoi(optarg);
			printf("md5_idr_number %d \n", md5_idr_number);
			break;
		case TOTAL_SIZE:
			break;
		case FILE_SIZE:
			file_size_flag = 1;
			file_size_mega_byte = atoi(optarg);
			break;
		case SAVE_FRAME_INFO:
			frame_info_flag = 1;
			break;
		case SHOW_PTS_INFO:
			show_pts_flag = 1;
			break;
		case CHECK_PTS_INFO:
			check_pts_flag = 1;
			break;
		case REMOVE_TIME_STRING:
			remove_time_string_flag = 1;
			break;
		case SPLIT_SVCT_LAYER:
			split_svct_layer_flag = 1;
			break;
		case 'v':
			verbose_mode = 1;
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		trans = &stream_transfer[i];
		trans->port = transfer_port + i * 4;
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
				if (strlen(filename) == 0)
					default_filename = default_filename_nfs;
				else
					default_filename = filename;
				break;
			case TRANS_METHOD_TCP:
			case TRANS_METHOD_USB:
				if (strlen(filename) == 0)
					default_filename = default_filename_tcp;
				else
					default_filename = filename;
				break;
			default:
				default_filename = NULL;
				break;
			}
			if (default_filename != NULL)
				strcpy(trans->filename, default_filename);
		}
		printf("Stream %c %s: %s\n", 'A' + i, str[trans->method], trans->filename);
	}

	return 0;
}



int init_encoding_states(void)
{
	int i,j;
	//init all file hander and session id to invalid at start
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].fd = -1;
		encoding_states[i].fd_info = -1;
		encoding_states[i].session_id = -1;
		encoding_states[i].total_bytes = 0;
		encoding_states[i].total_frames = 0;
		encoding_states[i].pic_type = 0;
		encoding_states[i].pts = 0;
		encoding_states[i].svct_layers = 0;
		for (j = 0; j < MAX_SVCT_LAYERS; ++j) {
			encoding_states[i].fd_svct[j] = -1;
		}
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
	if (file_size_flag) {
		if ((encoding_states[stream_id].total_bytes / 1024) > (file_size_mega_byte * 1024))
			new_session = 1;
	}

	return new_session;
}

#include <time.h>


int get_time_string( char * time_str,  int len)
{
	time_t  t;
	struct tm * tmp;

	t= time(NULL);
	tmp = gmtime(&t);
	if (strftime(time_str, len, "%m%d%H%M%S", tmp) == 0) {
		printf("date string format error \n");
		return -1;
	}

	return 0;
}

#define VERSION	0x00000005
#define PTS_IN_ONE_SECOND		(90000)
int write_frame_info_header(int stream_id)
{
	char dummy_config[36];
	int version = VERSION;
	u32 size = sizeof(dummy_config);
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	sprintf(dummy_config, "Here should contain H264 config\n");

	if (amba_transfer_write(fd_info, &version, sizeof(int), method) < 0 ||
		amba_transfer_write(fd_info, &size, sizeof(u32), method) < 0 ||
		amba_transfer_write(fd_info, &dummy_config, sizeof(dummy_config), method) < 0) {
		perror("write_data(4)");
		return -1;
	}

	return 0;
}

int write_frame_info(struct iav_framedesc *framedesc)
{
	typedef struct video_frame_s {
		u32     size;
		u32     pts;
		u32     pic_type;
		u32     reserved;
	} video_frame_t;
	video_frame_t frame_info;
	frame_info.pic_type = framedesc->pic_type;
	frame_info.pts = (u32)framedesc->dsp_pts;
	frame_info.size = framedesc->size;
	int stream_id = framedesc->id;
	int fd_info = encoding_states[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	if (amba_transfer_write(fd_info, &frame_info, sizeof(frame_info), method) < 0) {
		perror("write(5)");
		return -1;
	}
	return 0;
}

static int calc_svct_layers(int stream_id)
{
	struct iav_h264_cfg h264;

	memset(&h264, 0, sizeof(h264));
	h264.id = stream_id;
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG, &h264);

	switch (h264.gop_structure) {
	case IAV_GOP_SVCT_2:
		encoding_states[stream_id].svct_layers = 2;
		break;
	case IAV_GOP_SVCT_3:
		encoding_states[stream_id].svct_layers = 3;
		break;
	default:
		printf("SCVT is not enabled when gop = %d.", h264.gop_structure);
		break;
	}

	return 0;
}

//check session and update file handle for write when needed
int check_session_file_handle(struct iav_framedesc *framedesc, int new_session)
{
	char write_file_name[1024];
	char time_str[256];
	char file_type[8];
	int i, is_h264;
	int stream_id = framedesc->id;
	char stream_name;
	int method = stream_transfer[stream_id].method;
	int port = stream_transfer[stream_id].port;

   	if (new_session) {
		is_h264 = (framedesc->stream_type != IAV_STREAM_TYPE_MJPEG);
		sprintf(file_type, "%s", is_h264 ? "h264" : "mjpeg");
		//close old session if needed
		if (encoding_states[stream_id].fd > 0) {
			close(encoding_states[stream_id].fd);
			encoding_states[stream_id].fd = -1;
		}
		//character based stream name
		if (split_svct_layer_flag) {
			for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
				if (encoding_states[stream_id].fd_svct[i] > 0) {
					close(encoding_states[stream_id].fd_svct[i]);
					encoding_states[stream_id].fd_svct[i] = -1;
				}
			}
			encoding_states[stream_id].svct_layers = 0;
		}

		stream_name = 'A' + stream_id;

		get_time_string(time_str, sizeof(time_str));
		if (remove_time_string_flag) {
			memset(time_str, 0, sizeof(time_str));
		}
		if(md5_idr_number > 0) {
			sprintf(write_file_name, "%s", stream_transfer[stream_id].filename);
		} else {
			sprintf(write_file_name, "%s_%c_%s_%x.%s",
				stream_transfer[stream_id].filename, stream_name,
				time_str, framedesc->session_id,
				(framedesc->stream_type == IAV_STREAM_TYPE_MJPEG) ? "mjpeg" : "h264");
		}
		if ((encoding_states[stream_id].fd =
			amba_transfer_open(write_file_name, method, port)) < 0) {
			printf("create file for write failed %s \n", write_file_name);
			return -1;
		} else {
			if (!nofile_flag) {
				printf("\nnew session file name [%s], fd [%d] \n", write_file_name,
					encoding_states[stream_id].fd);
			}
		}

		if (split_svct_layer_flag && is_h264) {
			calc_svct_layers(stream_id);
			if (encoding_states[stream_id].svct_layers > 1) {
				for (i = 0; i < encoding_states[stream_id].svct_layers; ++i) {
					sprintf(filename, "%s.svct_%d.%s", write_file_name, i,
						file_type);
					encoding_states[stream_id].fd_svct[i] = amba_transfer_open(
						filename, method, (port + i + SVCT_PORT_OFFSET));
					if (encoding_states[stream_id].fd_svct[i] < 0) {
						printf("create file for write SVCT layers failed %s.\n",
							filename);
						return -1;
					}
				}
			}
		}
		if (frame_info_flag) {
			sprintf(write_file_name, "%s.info", write_file_name);
			if ((encoding_states[stream_id].fd_info =
				amba_transfer_open(write_file_name, method, port)) < 0) {
				printf("create file for frame info  failed %s \n", write_file_name);
				return -1;
			}
			if (write_frame_info_header(stream_id) < 0) {
				printf("write h264 header info failed %s \n", write_file_name);
				return -1;
			}
		}
	}
	return 0;
}

int update_session_data(struct iav_framedesc *framedesc, int new_session)
{
	int stream_id = framedesc->id;
	//update pic type, session id on new session
	if (new_session) {
		encoding_states[stream_id].pic_type = framedesc->pic_type;
		encoding_states[stream_id].session_id = framedesc->session_id;
		encoding_states[stream_id].total_bytes = 0;
		encoding_states[stream_id].total_frames = 0;
		old_encoding_states[stream_id] = encoding_states[stream_id];	//for statistics data only

#ifdef ACCURATE_FPS_CALC
		old_encoding_states[stream_id].total_frames2 = 0;
		old_encoding_states[stream_id].time2 = old_encoding_states[stream_id].capture_start_time;	//reset old counter
#endif
	}

	//update statistics on all frame
	encoding_states[stream_id].total_bytes += framedesc->size;
	encoding_states[stream_id].total_frames++;
	encoding_states[stream_id].pts = (u32)framedesc->dsp_pts;

	return 0;
}

static int write_svct_file(int method, unsigned char *in, int len, int fd)
{
	if (amba_transfer_write(fd, in, len, method) < 0) {
		perror("Failed to write stream into SVCT file.\n");
		return -1;
	}
	return 0;
}

static int find_svct_layer(unsigned char *in, int in_len)
{
	const int header_magic_num = 0x00000001;
	unsigned int header_mn = 0;
	unsigned char nalu, layer = -1;
	int i = 0;

	do {
		header_mn = (in[i] << 24 | in[i+1] << 16 | in[i+2] << 8 | in[i+3]);
		if (header_mn == header_magic_num) {
			nalu = in[i+4] & 0x1F;
			if ((nalu == NT_IDR) || (nalu == NT_NON_IDR)) {
				layer = (in[i+4] >> 5) & 0x3;
				break;
			}
		}
	} while (++i < in_len);
	return layer;
}

static int write_svct_files(int transfer_method, int stream_id,
	unsigned char *in, int in_len)
{
	int layer = find_svct_layer(in, in_len);
	int rval = 0;

	switch (encoding_states[stream_id].svct_layers) {
	case 3:
		switch (layer) {
		case 3:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[2]);
			/* Fall through to write this frame into other layers */
		case 2:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 0:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	case 2:
		switch (layer) {
		case 3:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 0:
			write_svct_file(transfer_method, in, in_len,
				encoding_states[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	default:
		rval = -1;
		printf("Invalid SVCT layers. Cannot be larger than 4.\n");
		break;
	}

	if ((rval >= 0) && verbose_mode) {
		printf("Save SVCT layer [%d] into file.\n", layer);
	}

	return rval;
}

int write_video_file(struct iav_framedesc *framedesc)
{
	static unsigned int whole_pic_size=0;
	u32 pic_size = framedesc->size;
	int fd = encoding_states[framedesc->id].fd;
	int stream_id = framedesc->id;

	//remove align
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
	if(md5_idr_number > 0) {
		if(framedesc->frame_num > 900) {
			//printf("write frame %d\n",framedesc->frame_num);
			if (amba_transfer_write(fd, bsb_mem + framedesc->data_addr_offset, pic_size,
				stream_transfer[stream_id].method) < 0) {
				perror("Failed to write specify streams into file!\n");
				return -1;
			}
			md5_idr_number = md5_idr_number - 1;
		}
		return 0;
	}
	if (amba_transfer_write(fd, bsb_mem + framedesc->data_addr_offset, pic_size,
		stream_transfer[stream_id].method) < 0) {
		perror("Failed to write streams into file!\n");
		return -1;
	}
	if (split_svct_layer_flag && (encoding_states[stream_id].svct_layers > 1)) {
		if (write_svct_files(stream_transfer[stream_id].method, stream_id,
			(unsigned char *)bsb_mem + framedesc->data_addr_offset, pic_size) < 0) {
			perror("Failed to split and write SVCT layers into files!\n");
			return -1;
		}
	}

	return 0;
}

int write_stream(int *total_frames, u64 *total_bytes)
{
	int new_session; //0:  old session  1: new session
	int print_frame = 1;
	u32 time_interval_us;
#ifdef ACCURATE_FPS_CALC
	u32 time_interval_us2;
#endif
	int stream_id;
	struct timeval pre_time, curr_time;
	int pre_frames ,curr_frames;
	u64 pre_bytes, curr_bytes;
	u32 pre_pts, curr_pts, curr_vin_fps;
	char stream_name[128];
	static int init_flag = 0;
	static int end_of_stream[MAX_ENCODE_STREAM_NUM];
	struct iav_stream_info stream_info;
	struct iav_querydesc query_desc;
	struct iav_framedesc *frame_desc;
	int i, stream_end_num;

	if (init_flag == 0) {
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
			end_of_stream[i] = 1;
		}
		init_flag = 1;
	}

	for (i = 0, stream_end_num = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info);
		if (stream_info.state == IAV_STREAM_STATE_ENCODING) {
			end_of_stream[i] = 0;
		}
		stream_end_num += end_of_stream[i];
	}

	// There is no encoding stream, skip to next turn
//	if (stream_end_num == MAX_ENCODE_STREAM_NUM)
//		return -1;

	memset(&query_desc, 0, sizeof(query_desc));
	frame_desc = &query_desc.arg.frame;
	query_desc.qid = IAV_DESC_FRAME;
	frame_desc->id = -1;
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}

	//update current frame encoding time
	stream_id = frame_desc->id;
	gettimeofday(&encoding_states[stream_id].capture_start_time, NULL);

	if (verbose_mode) {
		printf("type=%d, dspPTS=%lld, size=%d, addr=0x%x, strm_id=%d,"
			" sesn_id=%u, monotonic_pts=%lld, mono_diff=%lld, reso=%dx%d\n",
		frame_desc->pic_type, frame_desc->dsp_pts, frame_desc->size,
			frame_desc->data_addr_offset, frame_desc->id,
			frame_desc->session_id, frame_desc->arm_pts,
			(frame_desc->arm_pts - old_encoding_states[stream_id].monotonic_pts),
			frame_desc->reso.width, frame_desc->reso.height);
		old_encoding_states[stream_id].monotonic_pts = frame_desc->arm_pts;
	}

	//check if it's a stream end null frame indicator
	if (frame_desc->stream_end) {
		end_of_stream[stream_id] = 1;
		if (encoding_states[stream_id].fd > 0) {
			amba_transfer_close(encoding_states[stream_id].fd,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd = -1;
		}
		if (encoding_states[stream_id].fd_info > 0) {
			amba_transfer_close(encoding_states[stream_id].fd,
				stream_transfer[stream_id].method);
			encoding_states[stream_id].fd_info = -1;
		}
		if (split_svct_layer_flag) {
			for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
				if (encoding_states[stream_id].fd_svct[i] > 0) {
					amba_transfer_close(encoding_states[stream_id].fd_svct[i],
						stream_transfer[stream_id].method);
					encoding_states[stream_id].fd_svct[i] = -1;
				}
			}
			if (encoding_states[stream_id].svct_layers > 0) {
				encoding_states[stream_id].svct_layers = 0;
			}
		}

		goto write_stream_exit;
	}

	//check if it's new record session, since file name and recording control are based on session,
	//session id and change are important data
	new_session = is_new_session(frame_desc);
	//update session data
	if (update_session_data(frame_desc, new_session) < 0) {
		printf("update session data failed \n");
		return -2;
	}

	//check and update session file handle
	if (check_session_file_handle(frame_desc, new_session) < 0) {
		printf("check session file handle failed \n");
		return -3;
	}

	if (frame_info_flag) {
		if (write_frame_info(frame_desc) < 0) {
			printf("write video frame info failed for stream %d, session id = %d.\n",
				stream_id, frame_desc->session_id);
			return -5;
		}
	}

	//write file if file is still opened
	if (write_video_file(frame_desc) < 0) {
		printf("write video file failed for stream %d, session id = %d \n",
			stream_id, frame_desc->session_id);
		return -4;
	}

	//update global statistics
	if (total_frames)
		*total_frames = (*total_frames) + 1;
	if (total_bytes)
		*total_bytes = (*total_bytes) + frame_desc->size;

	//print statistics
	pre_time = old_encoding_states[stream_id].capture_start_time;
	curr_time = encoding_states[stream_id].capture_start_time;
	pre_frames = old_encoding_states[stream_id].total_frames;
	curr_frames = encoding_states[stream_id].total_frames;
	pre_bytes = old_encoding_states[stream_id].total_bytes;
	curr_bytes = encoding_states[stream_id].total_bytes;
	pre_pts = old_encoding_states[stream_id].pts;
	curr_pts = encoding_states[stream_id].pts;
	if (show_pts_flag) {
		struct vindev_fps vsrc_fps;
		vsrc_fps.vsrc_id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps);
		curr_vin_fps = vsrc_fps.fps;
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
			curr_time.tv_usec - pre_time.tv_usec;
		sprintf(stream_name, "stream %c", 'A' + stream_id);
		printf("%s: [%d]\tVIN: [%d], PTS: %d, diff: %d, frames NO: %d, size: %d\n",
			stream_name, time_interval_us, curr_vin_fps,
			curr_pts, (curr_pts - pre_pts), curr_frames, frame_desc->size);
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
	}
	if ((curr_frames % print_interval == 0) && (print_frame)) {
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time.tv_usec;

		sprintf(stream_name, "stream %c",  'A'+ stream_id);
		printf("%s:\t%4d %s, %2d fps, %18lld\tbytes, %5d kbps\n", stream_name,
			curr_frames, nofile_flag ? "discard" : "frames",
			DIV_ROUND((curr_frames - pre_frames) * 1000000, time_interval_us), curr_bytes,
			pre_time.tv_sec ? (int)((curr_bytes - pre_bytes) * 8000000LL /time_interval_us /1024) : 0);
		//backup time and states
		old_encoding_states[stream_id].session_id = encoding_states[stream_id].session_id;
		old_encoding_states[stream_id].fd = encoding_states[stream_id].fd;
		old_encoding_states[stream_id].total_frames = encoding_states[stream_id].total_frames;
		old_encoding_states[stream_id].total_bytes = encoding_states[stream_id].total_bytes;
		old_encoding_states[stream_id].pic_type = encoding_states[stream_id].pic_type;
		old_encoding_states[stream_id].capture_start_time = encoding_states[stream_id].capture_start_time;
	}
	#ifdef ACCURATE_FPS_CALC
	{
		const int fps_statistics_interval = 900;
		int pre_frames2;
		struct timeval pre_time2;
		pre_frames2 = old_encoding_states[stream_id].total_frames2;
		pre_time2 = old_encoding_states[stream_id].time2;
		if ((curr_frames % fps_statistics_interval ==0) &&(print_frame)) {
			time_interval_us2 = (curr_time.tv_sec - pre_time2.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time2.tv_usec;
			double fps = (curr_frames - pre_frames2)* 1000000.0/(double)time_interval_us2;
			BOLD_PRINT("AVG FPS = %4.2f\n",fps);
			old_encoding_states[stream_id].total_frames2 = encoding_states[stream_id].total_frames;
			old_encoding_states[stream_id].time2 = encoding_states[stream_id].capture_start_time;
		}
	}
	#endif

write_stream_exit:
	return 0;
}

//return 1 is IAV is in encoding,  else, return 0
int is_video_encoding(void)
{
	int state;

	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0 ) {
		perror("IAV_IOC_GET_IAV_STATE");
		exit(1);
	}

	return (state == IAV_STATE_ENCODING);
}

int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stdout, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else{
		fprintf(stdout, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stdout, " ");
		fprintf(stdout, "\r");
		dot_count = 0;
	}

	fflush(stdout);
	return 0;
}


int capture_encoded_video()
{
	int rval;
	//open file handles to write to
	int total_frames;
	u64 total_bytes;
	total_frames = 0;
	total_bytes =  0;

#ifdef ENABLE_RT_SCHED
	{
	    struct sched_param param;
	    param.sched_priority = 99;
	    if (sched_setscheduler(0, SCHED_FIFO, &param) < 0)
	        perror("sched_setscheduler");
	}
#endif

	while (1) {
		if ((rval = write_stream(&total_frames, &total_bytes)) < 0) {
			if (rval == -1) {
				usleep(100 * 1000);
				show_waiting();
			} else {
				printf("write_stream err code %d \n", rval);
			}
			continue;
                }
                if(md5_idr_number == 0) {
                        md5_idr_number = -1;
                        break;
                }
	}

	printf("stop encoded stream capture\n");

	printf("total_frames = %d\n", total_frames);
	printf("total_bytes = %lld\n", total_bytes);

	return 0;
}

static int init_transfer(void)
{
	int i, do_init, rtn = 0;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		do_init = 0;
		switch (stream_transfer[i].method) {
		case TRANS_METHOD_NONE:
			if (init_none == 0)
				do_init = init_none = 1;
			break;
		case TRANS_METHOD_NFS:
			if (init_nfs == 0)
				do_init = init_nfs = 1;
			break;
		case TRANS_METHOD_TCP:
			if (init_tcp == 0)
				do_init = init_tcp = 1;
			break;
		case TRANS_METHOD_USB:
			if (init_usb == 0)
				do_init = init_usb = 1;
			break;
		case TRANS_METHOD_STDOUT:
			if (init_stdout == 0)
				do_init = init_stdout = 1;
			break;
		default:
			return -1;
		}
		if (do_init)
			rtn = amba_transfer_init(stream_transfer[i].method);
		if (rtn < 0)
			return -1;
	}
	return 0;
}

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

	printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
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

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	init_encoding_states();

	if (init_transfer() < 0) {
		return -1;
	}

	if (capture_encoded_video() < 0) {
		printf("capture encoded video failed \n");
		return -1;
	}

	if (deinit_transfer() < 0) {
		return -1;
	}

	close(fd_iav);
	return 0;
}


