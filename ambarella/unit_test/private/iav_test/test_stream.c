
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
#include <sys/select.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>

#include <signal.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include "datatx_lib.h"

#include <pthread.h>

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
#define FAST_SEEK_PORT_OFFSET	(20)
#define STATISTIC_PORT_OFFSET	(1)
#define MAX_SVCT_LAYERS	(3)
#define FAST_SEEK_INTVL_MAX	(63)
#define MIN_SVCT_GOP_STRUCTURE	(2)
#define MAX_SVCT_GOP_STRUCTURE	(3)

//print and write file intervals
#define DEFAUT_FPS_STATISTICS_INTERVAL		(300) //Bold fps print.(AVG FPS = ...)
#define DEFAUT_PRINT_INTERVAL		(30) //(stream A:...)
#define DEFAUT_WRITE_STATISTICS_INTERVAL	(900) //how often we refresh the statistics file

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)

//total frames we need to capture
static int md5_idr_number = -1;

//debug options
#undef DEBUG_PRINT_FRAME_INFO

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
static int split_fast_seek_flag = 0;

static int fps_statistics_interval = DEFAUT_FPS_STATISTICS_INTERVAL;
static int print_interval = DEFAUT_PRINT_INTERVAL;
static int write_statistics_interval = DEFAUT_WRITE_STATISTICS_INTERVAL;

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

//statistics
static int statistics_mode = 0;

// options and usage
#define NO_ARG		0
#define HAS_ARG		1

#define TRANSFER_OPTIONS_BASE		0
#define INFO_OPTIONS_BASE			10
#define TEST_OPTIONS_BASE			20
#define MISC_OPTIONS_BASE			40
#define PRINT_CHARACTER_AMOUNT		1024

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
	SPLIT_FAST_SEEK,
	HOST_IP_ADDR,
};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"statistics",	HAS_ARG,	0,	's'},
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
	{"split-fast-seek", NO_ARG,	0,	SPLIT_FAST_SEEK},
	{"fps-intvl",	HAS_ARG,	0,	'i'},
	{"frame-intvl",	HAS_ARG,	0,	'n'},
	{"streamA",	NO_ARG,		0,	'A'},   // -A xxxxx	means all following configs will be applied to stream A
	{"streamB",	NO_ARG,		0,	'B'},
	{"streamC",	NO_ARG,		0,	'C'},
	{"streamD",	NO_ARG,		0,	'D'},
	{"host-ip-addr",	HAS_ARG,		0,	HOST_IP_ADDR},
	{"verbose",	NO_ARG,		0,	'v'},
	{0, 0, 0, 0}
};

static const char *short_options = "f:s:tuoi:n:ABCDv";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"filename", "specify filename"},
	{"", "\t\tspecify refresh statistics file interval (frames)"},
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
	{"", "\tsplit and write fast seek frames into a solo file"},
	{"", "\t\tset fps statistics interval"},
	{"", "\tset frame/field statistic interval"},
	{"", "\t\tstream A"},
	{"", "\t\tstream B"},
	{"", "\t\tstream C"},
	{"", "\t\tstream D"},
	{"", "\tset the host ip address"},
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
	printf("\nStatistics module usage: \n"
		"\tIt has to run test_stream before enabling encoding stream \n"
		"\tin order to get accurate statistics values. And when test \n"
		"\tdone, please end streaming first. \n"

		"\nExample: \n"
		"\tSave stream file with file name beginning with 'video' under \n"
		"\t/mnt folder and refresh statistics file every 900 frames.\n"
		"\t\ttest_stream -f /mnt/video -s 900\n"
		"\tSave no file and print statistics information every 900 frames.\n"
		"\t\ttest_stream -s 900 --nofile\n");
}

typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	u64 total_frames; // count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;  //count how many bytes encoded
	int pic_type;	  //picture type,  non changed during encoding, help to validate encoding state.
	u64 pts;
	u64 pts_reverse_count;	//self increase 1 when dsp pts reverse
	u64 monotonic_pts;
	u64 enc_done_ts;
	u64 enc_done_ts_diff_sum;
	u64 i_num;	//record i frame's number
	u64 total_frames_i;
	u64 total_frames_p;
	u64 total_frames_b;
	u64 total_bytes_i;
	u64 total_bytes_p;
	u64 total_bytes_b;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start
	struct timeval encoding_start_time;	//the time when encoding starts

	u64 total_frames2;	//for fps measured every fps_statistics_interval frames
	struct timeval time2;	//for fps measured every fps_statistics_interval frames

	u64 total_bytes_gop;	//for bitrate measured within the scope of one gop
	struct timeval time_gop;	//for bitrate measured within the scope of one gop

} stream_encoding_state_t;

typedef struct stream_files_s{
	int session_id;	//stream encoding session
	u64 total_bytes;  //count how many bytes encoded
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	int fd_svct[MAX_SVCT_LAYERS];	//file descriptor for svct streams
	int fd_fast_seek;	//file descriptor for fast seek streams
	int gop_structure;	//store gop structure
	int fast_seek_intvl;	//store fast seek intvl
} stream_files_t;

typedef struct stream_encoding_statistics_s{
	int statistics_fd;	//file descriptor for statistics file
	char write_file_name[1024];
	u32 gop;		//previous gop value
	u64 max_dsp_pts;
	u64 min_dsp_pts;
	u64 avg_dsp_pts;	//avg dsp pts diff from the beginning to the end
	u64 max_arm_pts;
	u64 min_arm_pts;
	u64 avg_arm_pts;	//avg arm pts diff from the beginning to the end
	u64 max_enc_done_pts;
	u64 min_enc_done_pts;
	u64 avg_enc_done_pts;	//avg enc done pts diff from the beginning to the end
	char *gop_record;	//record different gop value
	double max_fps;
	double min_fps;
	double rt_fps;		//real-time fps, calculated every print_interval frames
	double avg_fps;		//avg fps measured from the beginning to the end
	u32 rt_bitrate;		//real-time bitrate, measured every print_interval frames
	u32 max_bitrate;
	u32 min_bitrate;
	u32 avg_bitrate;	//avg bitrate from the beginning to the end
	u32 max_bitrate_gop;
	u32 min_bitrate_gop;
	u32 max_frame_size;
	u32 min_frame_size;
	u32 avg_frame_size;	//avg frame size from the beginning to the end
	u32 max_i_frame;
	u32 min_i_frame;
	u32 avg_i_frame;	//avg i frame size from the beginning to the end
	u32 max_p_frame;
	u32 min_p_frame;
	u32 avg_p_frame;	//avg p frame size from the beginning to the end
	u32 max_b_frame;
	u32 min_b_frame;
	u32 avg_b_frame;	//avg b frame size from the beginning to the end
} stream_encoding_statistics_t;

typedef struct pipe_mesg_s {
	struct iav_framedesc framedesc;
	int new_session;
	int stop;
} pipe_mesg_t;

static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static stream_encoding_state_t old_encoding_states[MAX_ENCODE_STREAM_NUM];  //old states for statistics only
static stream_encoding_statistics_t encoding_statistics[MAX_ENCODE_STREAM_NUM]; //store statistics data
static stream_files_t stream_files[MAX_ENCODE_STREAM_NUM];

//pipe and thread for statistics module
static int pipefd[2] = {-1, -1};
static int statistics_run = 1;
static int write_video_file_run = 1;

static int init_param(int argc, char **argv)
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
		case 's':
			statistics_mode = 1;
			write_statistics_interval = atoi(optarg);
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
		case SPLIT_FAST_SEEK:
			split_fast_seek_flag = 1;
			break;
		case HOST_IP_ADDR:
			if (INADDR_NONE == inet_addr(optarg)) {
				printf("ip address error!\n");
				return -1;
			}
			setenv("HOST_IP_ADDR", optarg, 1);
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

static int init_encoding_states(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].total_bytes = 0;
		encoding_states[i].total_frames = 0;
		encoding_states[i].pic_type = 0;
		encoding_states[i].pts = 0;
		encoding_states[i].i_num = 0;
		encoding_states[i].enc_done_ts_diff_sum = 0;
		encoding_states[i].pts_reverse_count = 0;
	}
	return 0;
}

static int init_stream_files(void)
{
	int i,j;

	for(i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		stream_files[i].fd = -1;
		stream_files[i].fd_info = -1;
		stream_files[i].session_id = -1;
		stream_files[i].gop_structure = 0;
		stream_files[i].total_bytes = 0;
		for (j = 0; j < MAX_SVCT_LAYERS; ++j) {
			stream_files[i].fd_svct[j] = -1;
		}
		stream_files[i].fd_fast_seek = -1;
		stream_files[i].fast_seek_intvl = 0;
	}
	return 0;
}

static int init_encoding_statistics(void)
{
	int i;
	int err = 0;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		memset(&encoding_statistics[i], 0, sizeof(struct stream_encoding_statistics_s));
		encoding_statistics[i].statistics_fd = -1;
		encoding_statistics[i].gop_record = (char *)malloc(sizeof(char)*PRINT_CHARACTER_AMOUNT);
		if (NULL == encoding_statistics[i].gop_record) {
			printf("Can't malloc memory for gop_record!\n");
			err = -1;
			break;
		}
		memset(encoding_statistics[i].gop_record, 0, sizeof(char)*PRINT_CHARACTER_AMOUNT);
	}
	if (err < 0) {
		for (i = 0; i< MAX_ENCODE_STREAM_NUM; ++i) {
			if (encoding_statistics[i].gop_record) {
				free(encoding_statistics[i].gop_record);
				encoding_statistics[i].gop_record = NULL;
			}
		}
	}
	return err;
}

static int close_stream_files(int stream_id)
{
	int i;

	if (stream_files[stream_id].fd > 0) {
		amba_transfer_close(stream_files[stream_id].fd,
			stream_transfer[stream_id].method);
		stream_files[stream_id].fd = -1;
	}
	if (stream_files[stream_id].fd_info > 0) {
		amba_transfer_close(stream_files[stream_id].fd,
			stream_transfer[stream_id].method);
		stream_files[stream_id].fd_info = -1;
	}
	if (split_svct_layer_flag) {
		for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
			if (stream_files[stream_id].fd_svct[i] > 0) {
				amba_transfer_close(stream_files[stream_id].fd_svct[i],
					stream_transfer[stream_id].method);
				stream_files[stream_id].fd_svct[i] = -1;
			}
		}
		if (stream_files[stream_id].gop_structure > 0) {
			stream_files[stream_id].gop_structure = 0;
		}
	}
	if (split_fast_seek_flag) {
		if (stream_files[stream_id].fd_fast_seek > 0) {
			amba_transfer_close(stream_files[stream_id].fd_fast_seek,
				stream_transfer[stream_id].method);
			stream_files[stream_id].fd_fast_seek = -1;
		}
	}
	return 0;
}

static int deinit_stream_files(int stream_id)
{
	int i;

	if (stream_id == -1) {
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
			close_stream_files(i);
		}
	} else {
		close_stream_files(stream_id);
	}
	return 0;
}

static int init_data(void)
{
	int ret = 0;

	init_encoding_states();
	init_stream_files();
	if (init_encoding_statistics() < 0) {
		ret = -1;
	}
	return ret;
}

//return 0 if it's not new session,  return 1 if it is new session
static int is_new_session(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	int new_session = 0 ;
	if  (framedesc ->session_id != stream_files[stream_id].session_id) {
		//a new session
		new_session = 1;
	}
	if (file_size_flag) {
		if ((stream_files[stream_id].total_bytes / 1024) > (file_size_mega_byte * 1024))
			new_session = 1;
	}

	return new_session;
}

#include <time.h>

static int get_time_string( char * time_str,  int len)
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
static int write_frame_info_header(int stream_id)
{
	char dummy_config[36];
	int version = VERSION;
	u32 size = sizeof(dummy_config);
	int fd_info = stream_files[stream_id].fd_info;
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

static int write_frame_info(struct iav_framedesc *framedesc)
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
	int fd_info = stream_files[stream_id].fd_info;
	int method = stream_transfer[stream_id].method;

	if (amba_transfer_write(fd_info, &frame_info, sizeof(frame_info), method) < 0) {
		perror("write(5)");
		return -1;
	}
	return 0;
}

static int open_statistics_file(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	int method = stream_transfer[stream_id].method;
	int port = stream_transfer[stream_id].port;

	if (encoding_statistics[stream_id].statistics_fd > 0) {
		amba_transfer_close(encoding_statistics[stream_id].statistics_fd, method);
		encoding_statistics[stream_id].statistics_fd = -1;
	}
	if ((encoding_statistics[stream_id].statistics_fd =
		amba_transfer_open(encoding_statistics[stream_id].write_file_name,
			method, port + STATISTIC_PORT_OFFSET)) < 0) {
		printf("create file for statistics failed %s \n",
			encoding_statistics[stream_id].write_file_name);
		return -1;
	}

	return 0;
}

static int write_statistics(struct iav_framedesc *framedesc, char *print_buffer)
{
	int stream_id = framedesc->id;
	char stream_type_str[8];
	int statistics_fd = encoding_statistics[stream_id].statistics_fd;
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];
	int method = stream_transfer[stream_id].method;
	u64 total_msecond = (framedesc->arm_pts == 0) ?
		(encoding_states[stream_id].monotonic_pts - old_encoding_states[stream_id].monotonic_pts)/90
		: (framedesc->arm_pts - old_encoding_states[stream_id].monotonic_pts)/90;
	int print_msecond = (int)total_msecond % 1000;
	long int total_second = (long)total_msecond / 1000;
	int print_hour = (int)total_second /3600;
	int print_minute = (int)(total_second % 3600) / 60;
	int print_second = (int)(total_second % 3600) % 60;

	switch (framedesc->stream_type) {
	case IAV_STREAM_TYPE_H264:
		strcpy(stream_type_str, "H.264");
		break;
	case IAV_STREAM_TYPE_MJPEG:
		strcpy(stream_type_str, "MJPEG");
		break;
	default:
		strcpy(stream_type_str, "Unknown");
		break;
	}

	sprintf(print_buffer,
		"This statistics file is refreshed every %d frames.\n"
		"***************************************\n"
		"                             Stream ID:     stream %c\n"
		"                        Encoding Codec:        %s\n"
		"                    Total Frames Count:     %8llu\n"
		"                            Resolution:    %4ux%4u\n"
		"               Total Encoding Duration:  %d:%02d:%02d.%03d\n"
		"    I Frame Size (Byte)(MAX, AVG, MIN):     %8u, %8u, %8u\n"
		"    P Frame Size (Byte)(MAX, AVG, MIN):     %8u, %8u, %8u\n"
		"    B Frame Size (Byte)(MAX, AVG, MIN):     %8u, %8u, %8u\n"
		"    ARM PTS Diff       (MAX, AVG, MIN):     %8llu, %8llu, %8llu\n"
		"    DSP PTS Diff       (MAX, AVG, MIN):     %8llu, %8llu, %8llu\n"
		"    ENC PTS Diff       (MAX, AVG, MIN):     %8llu, %8llu, %8llu\n"
		"             FPS       (MAX, AVG, MIN):       %6.2f,   %6.2f,   %6.2f\n"
		"         Bitrate (Kbps)(MAX, AVG, MIN):     %8u, %8u, %8u\n"
		"     Bitrate GOP (Kbps)     (MAX, MIN):     %8u, %8u\n"
		"      Frame Size (Byte)(MAX, AVG, MIN):     %8u, %8u, %8u\n"
		"      GOP Record      (Old --> Recent): ", write_statistics_interval,
		'A' + framedesc->id, stream_type_str,
		encoding_states[stream_id].total_frames,
		framedesc->reso.width, framedesc->reso.height,
		print_hour, print_minute, print_second, print_msecond,
		statistics->max_i_frame, statistics->avg_i_frame, statistics->min_i_frame,
		statistics->max_p_frame, statistics->avg_p_frame, statistics->min_p_frame,
		statistics->max_b_frame, statistics->avg_b_frame, statistics->min_b_frame,
		statistics->max_arm_pts, statistics->avg_arm_pts, statistics->min_arm_pts,
		statistics->max_dsp_pts, statistics->avg_dsp_pts, statistics->min_dsp_pts,
		statistics->max_enc_done_pts, statistics->avg_enc_done_pts, statistics->min_enc_done_pts,
		statistics->max_fps, statistics->avg_fps, statistics->min_fps,
		statistics->max_bitrate, statistics->avg_bitrate, statistics->min_bitrate,
		statistics->max_bitrate_gop, statistics->min_bitrate_gop,
		statistics->max_frame_size, statistics->avg_frame_size, statistics->min_frame_size);
	sprintf(print_buffer+strlen(print_buffer), "%s\n", statistics->gop_record);
	if (method == TRANS_METHOD_NONE) {
		printf("%s\n", print_buffer);
	} else {
		if (amba_transfer_write(statistics_fd, print_buffer, strlen(print_buffer), method) < 0) {
			perror("write_data(4)");
			return (-1);
		}
	}

	return 0;
}

static int check_h264_info(int stream_id)
{
	struct iav_h264_cfg h264;
	int rval = 0;

	memset(&h264, 0, sizeof(h264));
	h264.id = stream_id;
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG, &h264);

	if (split_fast_seek_flag && !h264.fast_seek_intvl) {
		printf("Invalid fast seek intvl to split fast seek.\n");
		return -1;
	}
	stream_files[stream_id].fast_seek_intvl = h264.fast_seek_intvl;

	switch (h264.gop_structure) {
	case IAV_GOP_SVCT_2:
		stream_files[stream_id].gop_structure = 2;
		break;
	case IAV_GOP_SVCT_3:
		stream_files[stream_id].gop_structure = 3;
		break;
	case IAV_GOP_LT_REF_P:
		if (split_svct_layer_flag) {
			printf("SVCT is not enabled when gop = %d\n", h264.gop_structure);
			rval = -1;
			break;
		}
		stream_files[stream_id].gop_structure = 8;
		break;
	default:
		rval = -1;
		printf("SCVT is not enabled when gop = %d.", h264.gop_structure);
		break;
	}

	return rval;
}

//check session and update file handle for write when needed
static int check_session_file_handle(struct iav_framedesc *framedesc, int new_session)
{
	char write_file_name[1024];
	char time_str[256];
	char file_type[8];
	int i, is_h264;
	int stream_id = framedesc->id;
	char stream_name;
	int method = stream_transfer[stream_id].method;
	int port = stream_transfer[stream_id].port;
	enum iav_stream_type stream_type = framedesc->stream_type;

	if (new_session) {
		is_h264 = (stream_type != IAV_STREAM_TYPE_MJPEG);
		sprintf(file_type, "%s", is_h264 ? "h264" : "mjpeg");
		//close old session if needed
		if (stream_files[stream_id].fd > 0) {
			amba_transfer_close(stream_files[stream_id].fd, method);
			stream_files[stream_id].fd = -1;
		}
		//character based stream name
		if (split_svct_layer_flag) {
			for (i = 0; i < MAX_SVCT_LAYERS; ++i) {
				if (stream_files[stream_id].fd_svct[i] > 0) {
					amba_transfer_close(stream_files[stream_id].fd_svct[i], method);
					stream_files[stream_id].fd_svct[i] = -1;
				}
			}
			stream_files[stream_id].gop_structure = 0;
		}
		if (split_fast_seek_flag) {
			if (stream_files[stream_id].fd_fast_seek > 0) {
				amba_transfer_close(stream_files[stream_id].fd_fast_seek, method);
				stream_files[stream_id].fd_fast_seek = -1;
			}
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
				((stream_type == IAV_STREAM_TYPE_MJPEG) ? "mjpeg" : "h264"));
		}
		if ((stream_files[stream_id].fd =
			amba_transfer_open(write_file_name, method, port)) < 0) {
			printf("create file for write failed %s \n", write_file_name);
			return -1;
		} else {
			if (!nofile_flag) {
				printf("\nnew session file name [%s], fd [%d] \n", write_file_name,
					stream_files[stream_id].fd);
			}
		}

		if (split_svct_layer_flag || split_fast_seek_flag) {
			if (check_h264_info(stream_id) < 0) {
				printf("check h264 info failed for split svct or fast seek!\n");
				return -1;
			}
		}

		if (split_svct_layer_flag && is_h264) {
			if ((stream_files[stream_id].gop_structure >= MIN_SVCT_GOP_STRUCTURE) &&
				(stream_files[stream_id].gop_structure <= MAX_SVCT_GOP_STRUCTURE)) {
				for (i = 0; i < stream_files[stream_id].gop_structure; ++i) {
					sprintf(filename, "%s.svct_%d.%s", write_file_name, i, file_type);
					stream_files[stream_id].fd_svct[i] = amba_transfer_open(
						filename, method, (port + i + SVCT_PORT_OFFSET));
					if (stream_files[stream_id].fd_svct[i] < 0) {
						printf("create file for write SVCT layers failed %s.\n",
							filename);
						return -1;
					}
				}
			}
		}

		if (split_fast_seek_flag && is_h264) {
			sprintf(filename, "%s.fast_seek.%s", write_file_name, file_type);
			stream_files[stream_id].fd_fast_seek = amba_transfer_open(
				filename, method, (port + FAST_SEEK_PORT_OFFSET));
			if (stream_files[stream_id].fd_fast_seek < 0) {
				printf("create file for write fast seek failed %s.\n",
					filename);
				return -1;
			}
		}

		if (frame_info_flag) {
			sprintf(write_file_name, "%s.info", write_file_name);
			if ((stream_files[stream_id].fd_info =
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

static int update_session_data(struct iav_framedesc *framedesc, int new_session)
{
	int stream_id = framedesc->id;
	char *gop_record_buffer = NULL;

	//update pic type, session id on new session
	if (new_session) {
		encoding_states[stream_id].session_id = framedesc->session_id;
		encoding_states[stream_id].total_bytes = 0;
		encoding_states[stream_id].total_frames = 0;
		encoding_states[stream_id].total_frames_i = 0;
		encoding_states[stream_id].total_bytes_i = 0;
		encoding_states[stream_id].total_frames_p = 0;
		encoding_states[stream_id].total_bytes_p = 0;
		encoding_states[stream_id].total_frames_b = 0;
		encoding_states[stream_id].total_bytes_b = 0;
		encoding_states[stream_id].i_num = 0;
		encoding_states[stream_id].enc_done_ts_diff_sum = 0;
		encoding_states[stream_id].pts_reverse_count = 0;

		encoding_states[stream_id].monotonic_pts = 0;
		encoding_states[stream_id].pts = 0;
		encoding_states[stream_id].encoding_start_time = encoding_states[stream_id].capture_start_time;

		//for statistics data only
		old_encoding_states[stream_id].total_frames = 0;
		old_encoding_states[stream_id].capture_start_time = encoding_states[stream_id].capture_start_time;
		old_encoding_states[stream_id].total_bytes = 0;
		old_encoding_states[stream_id].total_frames2 = 0;
		old_encoding_states[stream_id].time2 = encoding_states[stream_id].capture_start_time;
		old_encoding_states[stream_id].monotonic_pts = framedesc->arm_pts;
		old_encoding_states[stream_id].pts = framedesc->dsp_pts;

		//for biterate within the scope of gop
		memset(&encoding_states[stream_id].time_gop, 0, sizeof(struct timeval));
		encoding_states[stream_id].total_bytes_gop = 0;

		//backup allocated memory pointer and reset statistic values
		gop_record_buffer = encoding_statistics[stream_id].gop_record;
		memset(&encoding_statistics[stream_id], 0, sizeof(struct stream_encoding_statistics_s));
		encoding_statistics[stream_id].gop_record = gop_record_buffer;
		gop_record_buffer = NULL;
		memset(encoding_statistics[stream_id].gop_record, 0, sizeof(char)*PRINT_CHARACTER_AMOUNT);
	}

	//update statistics on all frame
	encoding_states[stream_id].pic_type = framedesc->pic_type;
	encoding_states[stream_id].total_bytes += framedesc->size;
	encoding_states[stream_id].total_frames++;
	encoding_states[stream_id].enc_done_ts_diff_sum += framedesc->enc_done_ts - framedesc->arm_pts;

	//update i p b frame count
	if (framedesc->pic_type == IAV_PIC_TYPE_IDR_FRAME || framedesc->pic_type == IAV_PIC_TYPE_I_FRAME) {
		encoding_states[stream_id].total_frames_i ++;
		encoding_states[stream_id].total_bytes_i += framedesc->size;
	}
	if (framedesc->pic_type == IAV_PIC_TYPE_P_FRAME) {
		encoding_states[stream_id].total_frames_p ++;
		encoding_states[stream_id].total_bytes_p += framedesc->size;
	}
	if (framedesc->pic_type == IAV_PIC_TYPE_B_FRAME) {
		encoding_states[stream_id].total_frames_b ++;
		encoding_states[stream_id].total_bytes_b += framedesc->size;
	}

	return 0;
}

static int update_files_data(struct iav_framedesc *framedesc, int new_session)
{
	int stream_id = framedesc->id;

	if (new_session) {
		stream_files[stream_id].session_id = framedesc->session_id;
		stream_files[stream_id].total_bytes = 0;
	}

	stream_files[stream_id].total_bytes += framedesc->size;

	return 0;
}

static int statistics_gop(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	stream_encoding_state_t *state = &encoding_states[stream_id];
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];

	if (framedesc->pic_type == IAV_PIC_TYPE_IDR_FRAME ||
		framedesc->pic_type == IAV_PIC_TYPE_I_FRAME) {
		if (state->i_num != 0) {
			if (statistics->gop != (state->total_frames- state->i_num)) {
				statistics->gop = state->total_frames- state->i_num;
				if (strlen(statistics->gop_record) > (sizeof(char)*(PRINT_CHARACTER_AMOUNT - 5))) {
					printf("Gop_record doesn't have enough space for new gop record!\n");
					return -1;
				}
				sprintf(statistics->gop_record, "%s%4u ",
					statistics->gop_record, statistics->gop);
			}
		}
		state->i_num = state->total_frames;
	}
	return 0;
}

static void statistics_ipb_frame_size(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];

	if (framedesc->pic_type == IAV_PIC_TYPE_IDR_FRAME ||
		framedesc->pic_type == IAV_PIC_TYPE_I_FRAME) {
		if (statistics->max_i_frame == 0 || statistics->min_i_frame == 0) {
			statistics->max_i_frame = framedesc->size;
			statistics->min_i_frame = framedesc->size;
		} else if (framedesc->size > statistics->max_i_frame) {
			statistics->max_i_frame = framedesc->size;
		} else if (framedesc->size < statistics->min_i_frame) {
			statistics->min_i_frame = framedesc->size;
		}
	}
	if (framedesc->pic_type == IAV_PIC_TYPE_P_FRAME) {
		if (statistics->max_p_frame == 0 || statistics->min_p_frame == 0) {
			statistics->max_p_frame = framedesc->size;
			statistics->min_p_frame = framedesc->size;
		} else if (framedesc->size > statistics->max_p_frame) {
			statistics->max_p_frame = framedesc->size;
		} else if (framedesc->size < statistics->min_p_frame) {
			statistics->min_p_frame = framedesc->size;
		}
	}
	if (framedesc->pic_type == IAV_PIC_TYPE_B_FRAME) {
		if (statistics->max_b_frame == 0 || statistics->min_b_frame == 0) {
			statistics->max_b_frame = framedesc->size;
			statistics->min_b_frame = framedesc->size;
		} else if (framedesc->size > statistics->max_b_frame) {
			statistics->max_b_frame = framedesc->size;
		} else if (framedesc->size < statistics->min_b_frame) {
			statistics->min_b_frame = framedesc->size;
		}
	}
}

static void statistics_pts(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	stream_encoding_state_t *state = &encoding_states[stream_id];
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];
	u64 enc_done_pts_diff;
	u64 dsp_pts_diff;

	if (state->monotonic_pts != 0) {
		if ((framedesc->arm_pts - state->monotonic_pts) > statistics->max_arm_pts)
			statistics->max_arm_pts = framedesc->arm_pts - state->monotonic_pts;
		if ((framedesc->arm_pts - state->monotonic_pts) < statistics->min_arm_pts)
			statistics->min_arm_pts = framedesc->arm_pts - state->monotonic_pts;
	} else {
		statistics->max_arm_pts = 0;
		statistics->min_arm_pts = framedesc->arm_pts - state->monotonic_pts;
	}
	state->monotonic_pts = framedesc->arm_pts;

	if (state->pts != 0) {
		if (framedesc->dsp_pts < state->pts) {
			dsp_pts_diff = (1 << 30) + framedesc->dsp_pts - state->pts;
			state->pts_reverse_count ++;
		} else {
			dsp_pts_diff = framedesc->dsp_pts - state->pts;
		}
		if (dsp_pts_diff > statistics->max_dsp_pts) {
			statistics->max_dsp_pts = dsp_pts_diff;
		} else if (dsp_pts_diff < statistics->min_dsp_pts) {
			statistics->min_dsp_pts = dsp_pts_diff;
		}
	} else {
		statistics->max_dsp_pts = 0;
		statistics->min_dsp_pts = PTS_IN_ONE_SECOND;
	}
	state->pts = framedesc->dsp_pts;

	enc_done_pts_diff = framedesc->enc_done_ts - framedesc->arm_pts;
	if (statistics->max_enc_done_pts == 0 || statistics->min_enc_done_pts == 0) {
		statistics->max_enc_done_pts = enc_done_pts_diff;
		statistics->min_enc_done_pts = enc_done_pts_diff;
	} else if (enc_done_pts_diff > statistics->max_enc_done_pts) {
		statistics->max_enc_done_pts = enc_done_pts_diff;
	} else if (enc_done_pts_diff < statistics->min_enc_done_pts) {
		statistics->min_enc_done_pts = enc_done_pts_diff;
	}
}

static void statistics_fps_bitrate(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	int curr_frames, pre_frames, pre_frames2;
	struct timeval curr_time, pre_time, pre_time2;
	u64 time_interval_us, time_interval_us2;
	u64 time_inter_i_frame;
	u64 pre_bytes, curr_bytes;
	u32 bitrate = 0;
	double fps_average;
	stream_encoding_state_t *state = &encoding_states[stream_id];
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];

	pre_frames2 = old_encoding_states[stream_id].total_frames2;
	pre_time2 = old_encoding_states[stream_id].time2;
	pre_time = old_encoding_states[stream_id].capture_start_time;
	curr_time = encoding_states[stream_id].capture_start_time;
	curr_frames = encoding_states[stream_id].total_frames;
	pre_frames = old_encoding_states[stream_id].total_frames;
	pre_bytes = old_encoding_states[stream_id].total_bytes;
	curr_bytes = encoding_states[stream_id].total_bytes;

	if (curr_frames % print_interval == 0) {
		time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000ull +
						curr_time.tv_usec - pre_time.tv_usec;
		statistics->rt_fps = (curr_frames - pre_frames)* 1000000.0/(double)time_interval_us;

		if (statistics->max_fps == 0 || statistics->min_fps  == 0) {
			statistics->max_fps = statistics->rt_fps;
			statistics->min_fps = statistics->rt_fps;
		} else if (statistics->rt_fps > statistics->max_fps) {
			statistics->max_fps = statistics->rt_fps;
		} else if (statistics->rt_fps < statistics->min_fps) {
			statistics->min_fps = statistics->rt_fps;
		}

		statistics->rt_bitrate= (int)((curr_bytes - pre_bytes) * 8000000LL /time_interval_us /1024);
		if (statistics->max_bitrate == 0 || statistics->min_bitrate == 0) {
			statistics->max_bitrate = statistics->rt_bitrate;
			statistics->min_bitrate = statistics->rt_bitrate;
		} else if (statistics->rt_bitrate > statistics->max_bitrate) {
			statistics->max_bitrate = statistics->rt_bitrate;
		} else if (statistics->rt_bitrate < statistics->min_bitrate) {
			statistics->min_bitrate = statistics->rt_bitrate;
		}

		old_encoding_states[stream_id].total_frames = state->total_frames;
		old_encoding_states[stream_id].total_bytes = state->total_bytes;
		old_encoding_states[stream_id].capture_start_time = state->capture_start_time;
	}

	if (framedesc->pic_type == IAV_PIC_TYPE_IDR_FRAME || framedesc->pic_type == IAV_PIC_TYPE_I_FRAME) {
		time_inter_i_frame = (state->capture_start_time.tv_sec - state->time_gop.tv_sec) * 1000000ull +
						state->capture_start_time.tv_usec - state->time_gop.tv_usec;
		bitrate = state->capture_start_time.tv_sec ?
			(int)((state->total_bytes - state->total_bytes_gop) * 8000000LL / time_inter_i_frame/1024) : 0;
		state->time_gop = state->capture_start_time;
		state->total_bytes_gop = state->total_bytes;

		if (state->total_frames_i > 1) {
			if (statistics->max_bitrate_gop == 0 || statistics->min_bitrate_gop  == 0) {
				statistics->max_bitrate_gop = bitrate;
				statistics->min_bitrate_gop = bitrate;
			} else if (bitrate > statistics->max_bitrate_gop) {
				statistics->max_bitrate_gop = bitrate;
			} else if (bitrate < statistics->min_bitrate_gop) {
				statistics->min_bitrate_gop = bitrate;
			}
		}
	}

	if (curr_frames % fps_statistics_interval == 0) {
		time_interval_us2 = (curr_time.tv_sec - pre_time2.tv_sec) * 1000000ull +
					curr_time.tv_usec - pre_time2.tv_usec;
		fps_average = (curr_frames - pre_frames2)* 1000000.0/(double)time_interval_us2;

		BOLD_PRINT("AVG FPS = %4.2f\n", fps_average);

		old_encoding_states[stream_id].total_frames2 = state->total_frames;
		old_encoding_states[stream_id].time2 = state->capture_start_time;
	}
}

static void statistics_frame_size(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];

	if (statistics->max_frame_size == 0 || statistics->min_frame_size  == 0) {
		statistics->max_frame_size = framedesc->size;
		statistics->min_frame_size = framedesc->size;
	} else if (framedesc->size > statistics->max_frame_size) {
		statistics->max_frame_size = framedesc->size;
	} else if (framedesc->size < statistics->min_frame_size) {
		statistics->min_frame_size = framedesc->size;
	}
}

static void statistics_average(struct iav_framedesc *framedesc)
{
	int stream_id = framedesc->id;
	struct timeval curr_time, start_time;
	u64 time_interval_us;
	stream_encoding_state_t *state = &encoding_states[stream_id];
	stream_encoding_statistics_t *statistics = &encoding_statistics[stream_id];

	curr_time = state->capture_start_time;
	start_time = state->encoding_start_time;

	time_interval_us = (curr_time.tv_sec - start_time.tv_sec) * 1000000ull + curr_time.tv_usec - start_time.tv_usec;

	//avrage i p b frame size
	if (state->total_bytes_i && state->total_frames_i)
		statistics->avg_i_frame =
			(u32)(state->total_bytes_i/state->total_frames_i);
	else
		statistics->avg_i_frame = 0;
	if (state->total_bytes_p && state->total_frames_p)
		statistics->avg_p_frame =
			(u32)(state->total_bytes_p/state->total_frames_p);
	else
		statistics->avg_p_frame = 0;
	if (state->total_bytes_b && state->total_frames_b)
		statistics->avg_b_frame =
			(u32)(state->total_bytes_b/state->total_frames_b);
	else
		statistics->avg_b_frame = 0;

	//average pts diff
	statistics->avg_arm_pts =
		(state->monotonic_pts - old_encoding_states[stream_id].monotonic_pts)/
		(state->total_frames - 1);
	statistics->avg_dsp_pts =
		((state->pts_reverse_count << 30) + state->pts - old_encoding_states[stream_id].pts)/(state->total_frames -1);
	statistics->avg_enc_done_pts =
		state->enc_done_ts_diff_sum/state->total_frames;

	//average fps
	statistics->avg_fps = ((state->total_frames - 1) * 1000000.0)/(double)time_interval_us;

	//average bitrate
	statistics->avg_bitrate = (int)(state->total_bytes * 8000000LL /time_interval_us /1024);

	//average frame size
	statistics->avg_frame_size = (u32)(state->total_bytes/state->total_frames);
}

static void* do_statistics(void *arg)
{
	int i = 0;
	u32 time_interval_us;
	int stream_id;
	struct timeval pre_time, curr_time;
	u64 curr_frames;
	u64 curr_bytes, curr_pts;
	u32 curr_vin_fps;
	char stream_name[128];
	pipe_mesg_t pipe_mesg;
	struct iav_framedesc *framedesc;
	int new_session;
	char *print_buffer;
	char stream_num;
	char time_str[256];
	fd_set all_set;
	fd_set set;

	FD_ZERO(&all_set);
	FD_SET(pipefd[0], &all_set);

	print_buffer = (char *)malloc(sizeof(char)*PRINT_CHARACTER_AMOUNT*2);
	if (NULL == print_buffer) {
		printf("Malloc print buffer failed!\n");
		statistics_run = 0;
	}
	if (print_buffer) {
		memset(print_buffer, 0, sizeof(char)*PRINT_CHARACTER_AMOUNT*2);
	}

	while (statistics_run) {
		set = all_set;
		if(select(pipefd[0] + 1, &set, NULL, NULL, NULL) < 0) {
			perror("Do_statistics select");
		} else {
			if (FD_ISSET(pipefd[0], &set)) {
				u8 *buf = (u8*)&pipe_mesg;
				int read_size = 0;
				int count = 0;
				do {
					int read_ret = read(pipefd[0], buf + read_size,
									sizeof(pipe_mesg) - read_size);
					if (read_ret > 0) {
						read_size += read_ret;
					} else if (read_ret < 0) {
						perror("read statistics pipe");
					}
				} while((read_size < sizeof(pipe_mesg)) && (++ count < 5));
				if (read_size != sizeof(pipe_mesg)) {
					printf("Failed to read message from main loop:\n"
					  	  "expected message size: %u, received message size: %d\n",
					  	  sizeof(pipe_mesg), read_size);
					continue;
				}
			}
		}

		if (pipe_mesg.stop) {
			printf("Received stop message, stop statistics!\n");
			statistics_run = 0;
			continue;
		}
		framedesc = &pipe_mesg.framedesc;
		new_session = pipe_mesg.new_session;
		stream_id = framedesc->id;

		if (verbose_mode) {
			printf("type=%d, dspPTS=%llu, size=%d, addr=0x%x, strm_id=%d,"
				" sesn_id=%u, monotonic_pts=%llu, mono_diff=%llu,"
				" enc_done_pts = %llu, enc_pipe_time = %llu,"
				" reso=%dx%d\n",
				framedesc->pic_type, framedesc->dsp_pts, framedesc->size,
				framedesc->data_addr_offset, framedesc->id,
				framedesc->session_id, framedesc->arm_pts,
				framedesc->arm_pts - encoding_states[stream_id].monotonic_pts,
				framedesc->enc_done_ts,
				framedesc->enc_done_ts - framedesc->arm_pts,
				framedesc->reso.width, framedesc->reso.height);
		}

		if (framedesc->stream_end) {
			statistics_average(framedesc);
			if (statistics_mode) {
				if (encoding_statistics[stream_id].statistics_fd < 0) {
					if (open_statistics_file(framedesc) < 0) {
						printf("Open statistics file %s failed.\n",
							encoding_statistics[stream_id].write_file_name);
						break;
					}
				}
				if (write_statistics(framedesc, print_buffer) < 0) {
					printf("write statistics file %s failed.\n",
						encoding_statistics[stream_id].write_file_name);
					break;
				}
				amba_transfer_close(encoding_statistics[stream_id].statistics_fd,
									stream_transfer[stream_id].method);
				encoding_statistics[stream_id].statistics_fd = -1;
			}
			continue;
		}

		gettimeofday(&encoding_states[stream_id].capture_start_time, NULL);

		if (update_session_data(framedesc, new_session) < 0) {
			printf("update sessiondata failed \n");
			break;
		}

		pre_time = old_encoding_states[stream_id].capture_start_time;
		curr_time = encoding_states[stream_id].capture_start_time;
		curr_frames = encoding_states[stream_id].total_frames;
		curr_bytes = encoding_states[stream_id].total_bytes;
		curr_pts = encoding_states[stream_id].pts;

		if (show_pts_flag) {
			struct vindev_fps vsrc_fps;
			vsrc_fps.vsrc_id = 0;
			if (ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
				perror("IAV_IOC_VIN_GET_FPS\n");
				break;
			}
			curr_vin_fps = vsrc_fps.fps;
			time_interval_us = (curr_time.tv_sec - pre_time.tv_sec) * 1000000ull +
				curr_time.tv_usec - pre_time.tv_usec;
			sprintf(stream_name, "stream %c", 'A' + stream_id);
			printf("%s: [%d]\tVIN: [%d], PTS: %llu, diff: %llu, frames NO: %llu, size: %d\n",
				stream_name, time_interval_us, curr_vin_fps,
				curr_pts, framedesc->dsp_pts  - encoding_states[stream_id].pts,
				curr_frames, framedesc->size);
		}

		if (statistics_gop(framedesc) < 0) {
			printf("Do statistics_gop failed!\n");
			break;
		}

		statistics_ipb_frame_size(framedesc);

		statistics_pts(framedesc);

		statistics_fps_bitrate(framedesc);

		statistics_frame_size(framedesc);

		if (curr_frames % write_statistics_interval == 0) {
			statistics_average(framedesc);
		}

		//print statistics
		if (statistics_mode) {
			if (new_session) {
				stream_num = 'A' + stream_id;
				get_time_string(time_str, sizeof(time_str));
				sprintf(encoding_statistics[stream_id].write_file_name,
					"%s_%c_%s_%x.stat", stream_transfer[stream_id].filename,
					stream_num, time_str, framedesc->session_id);
				//open statistcs file
				if (open_statistics_file(framedesc) < 0) {
					printf("Open statistics file %s failed.\n",
						encoding_statistics[stream_id].write_file_name);
					break;
				}
			}

			//write file
			if (curr_frames % write_statistics_interval == 0) {
				if (encoding_statistics[stream_id].statistics_fd < 0) {
					if (open_statistics_file(framedesc) < 0) {
						printf("Open statistics file %s failed.\n",
							encoding_statistics[stream_id].write_file_name);
						break;
					}
				}
				if (write_statistics(framedesc, print_buffer) < 0) {
					printf("write statistics file %s failed.\n",
						encoding_statistics[stream_id].write_file_name);
					break;
				}
				amba_transfer_close(encoding_statistics[stream_id].statistics_fd,
									stream_transfer[stream_id].method);
				encoding_statistics[stream_id].statistics_fd = -1;
			}
		}

		if (curr_frames %print_interval == 0) {
			sprintf(stream_name, "stream %c",  'A'+ stream_id);
			printf("%s:\t%llu %s, %4.2f fps, %18llu\tbytes, %5d kbps\n",
				stream_name, curr_frames, nofile_flag ? "discard" : "frames",
				encoding_statistics[stream_id].rt_fps, curr_bytes,
				encoding_statistics[stream_id].rt_bitrate);
		}
	}
	if (statistics_run) {
		printf("Do statistics exited abnormally!\n");
	}
	if (pipefd[0] >= 0) {
		close(pipefd[0]);
		pipefd[0] = -1;
	}
	if (print_buffer) {
		free(print_buffer);
		print_buffer = NULL;
	}
	for(i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (encoding_statistics[i].gop_record) {
			free(encoding_statistics[i].gop_record);
			encoding_statistics[i].gop_record = NULL;
		}
		if (encoding_statistics[i].statistics_fd) {
			amba_transfer_close(encoding_statistics[i].statistics_fd,
								stream_transfer[i].method);
			encoding_statistics[i].statistics_fd = -1;
		}
	}

	return ((void *)0);
}

static int write_svct_file(int method, unsigned char *in, int len, int fd)
{
	if (amba_transfer_write(fd, in, len, method) < 0) {
		perror("Failed to write stream into SVCT file.\n");
		return -1;
	}
	return 0;
}

static int identify_nal_ref_idc(unsigned char *in, int in_len)
{
	const int header_magic_num = 0x00000001;
	unsigned int header_mn = 0;
	unsigned char nalu, nal_ref_idc = -1;
	int i = 0;
	do {
		header_mn = (in[i] << 24 | in[i+1] << 16 | in[i+2] << 8 | in[i+3]);
		if (header_mn == header_magic_num) {
			i += 4;
			nalu = in[i] & 0x1F;
			if ((nalu == NT_IDR) || (nalu == NT_NON_IDR)) {
				nal_ref_idc = (in[i] >> 5) & 0x3;
				break;
			}
		}
		++i;
	} while (i < in_len);
	return nal_ref_idc;
}

static int get_svct_layer(int stream_id, unsigned char *in,
	int in_len, int *ret_layer)
{
	int gop = stream_files[stream_id].gop_structure;
	int rval = 0;
	int layer, nal_ref_idc;

	if (!ret_layer) {
		printf("Invalid return layer pointer!\n");
		return -1;
	}
	nal_ref_idc = identify_nal_ref_idc(in, in_len);

	switch (gop) {
	case IAV_GOP_SVCT_3:
		switch (nal_ref_idc) {
		case 3:
			layer = 0;
			break;
		case 2:
			layer = 1;
			break;
		case 0:
			layer = 2;
			break;
		default:
			rval = -1;
			printf("Invalid nal ref idc %d\n", nal_ref_idc);
			break;
		}
		break;
	case IAV_GOP_SVCT_2:
		switch (nal_ref_idc) {
		case 3:
			layer = 0;
			break;
		case 0:
			layer = 1;
			break;
		default:
			rval = -1;
			printf("Invalid nal ref idc %d\n", nal_ref_idc);
			break;
		}
		break;
	default:
		rval = -1;
		printf("Invalid SVCT gop structure %d, cannot be larger than 3.\n", gop);
		break;
	}

	*ret_layer = layer;
	return rval;
}

static int write_svct_files(int transfer_method, int stream_id,
	unsigned char *in, int in_len)
{
	int gop = stream_files[stream_id].gop_structure;
	int layer = -1, rval = 0;

	if (get_svct_layer(stream_id, in, in_len, &layer) < 0) {
		printf("get svct layer failed!\n");
		return -1;
	}

	switch (gop) {
	case IAV_GOP_SVCT_3:
		switch (layer) {
		case 0:
			write_svct_file(transfer_method, in, in_len,
				stream_files[stream_id].fd_svct[2]);
			/* Fall through to write this frame into other layers */
		case 1:
			write_svct_file(transfer_method, in, in_len,
				stream_files[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 2:
			write_svct_file(transfer_method, in, in_len,
				stream_files[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	case IAV_GOP_SVCT_2:
		switch (layer) {
		case 0:
			write_svct_file(transfer_method, in, in_len,
				stream_files[stream_id].fd_svct[1]);
			/* Fall through to write this frame into other layers */
		case 1:
			write_svct_file(transfer_method, in, in_len,
				stream_files[stream_id].fd_svct[0]);
			break;
		default:
			rval = -1;
			printf("Incorrect SVCT layer [%d] from bitstream!\n", layer);
			break;
		}
		break;
	default:
		rval = -1;
		printf("Invalid gop structure %d, cannot be larger than 3.\n", gop);
		break;
	}

	if ((rval >= 0) && verbose_mode) {
		printf("Save SVCT layer [%d] into file.\n", layer);
	}

	return rval;
}

static int write_fast_seek_file(int transfer_method, int stream_id,
	unsigned char *in, int in_len)
{
	int nal_ref_idc = identify_nal_ref_idc(in, in_len);
	int fd = stream_files[stream_id].fd_fast_seek;
	int gop = stream_files[stream_id].gop_structure;
	int rval = 0;

	switch (gop) {
	case IAV_GOP_SVCT_2:
	case IAV_GOP_SVCT_3:
	case IAV_GOP_LT_REF_P:
		if (nal_ref_idc == 3) {
			if (amba_transfer_write(fd, in, in_len, transfer_method) < 0) {
				perror("Failed to write fast seek frames into file.\n");
				rval = -1;
			}
		}
		break;
	default:
		rval = -1;
		printf("Invalid gop structure %d for fast seek\n", gop);
		break;
	}

	if ((rval >= 0) && verbose_mode) {
		printf("Save fast seek frames into file.\n");
	}

	return rval;
}

static int write_video_file(struct iav_framedesc *framedesc)
{
	static unsigned int whole_pic_size=0;
	u32 pic_size = framedesc->size;
	int fd = stream_files[framedesc->id].fd;
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
	if (split_svct_layer_flag &&
		(stream_files[stream_id].gop_structure >= MIN_SVCT_GOP_STRUCTURE) &&
		(stream_files[stream_id].gop_structure <= MAX_SVCT_GOP_STRUCTURE)) {
		if (write_svct_files(stream_transfer[stream_id].method, stream_id,
			(unsigned char *)bsb_mem + framedesc->data_addr_offset, pic_size) < 0) {
			perror("Failed to split and write SVCT layers into files!\n");
			return -1;
		}
	}
	if (split_fast_seek_flag) {
		if (write_fast_seek_file(stream_transfer[stream_id].method, stream_id,
			(unsigned char *)bsb_mem + framedesc->data_addr_offset, pic_size) < 0) {
			perror("Failed to split and write fast seek frames into files!\n");
			return -1;
		}
	}

	return 0;
}

static int write_stream(u64 *total_frames, u64 *total_bytes)
{
	int new_session; //0:  old session  1: new session
	int stream_id;
	static int init_flag = 0;
	static int end_of_stream[MAX_ENCODE_STREAM_NUM];
	struct iav_stream_info stream_info;
	struct iav_querydesc query_desc;
	struct iav_framedesc *frame_desc;
	int i, stream_end_num;
	pipe_mesg_t pipe_mesg;

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
		if (errno != EAGAIN) {
			perror("IAV_IOC_QUERY_DESC");
		}
		return -1;
	}

	//check if it's new record session, since file name and recording control are based on session,
	//session id and change are important data
	stream_id = frame_desc->id;
	new_session = is_new_session(frame_desc);

	memcpy(&pipe_mesg.framedesc, frame_desc, sizeof(struct iav_framedesc));
	pipe_mesg.new_session = new_session;
	pipe_mesg.stop = 0;

	u8 *buf = (u8*)&pipe_mesg;
	int write_size = 0;
	int count = 0;
	do {
		int write_ret = write(pipefd[1], buf + write_size,
						sizeof(pipe_mesg) - write_size);
		if (write_ret > 0) {
			write_size += write_ret;
		} else if (write_ret < 0) {
			perror("write statistics pipe");
		}
	} while((write_size < sizeof(pipe_mesg)) && (++ count < 5));
	if (write_size != sizeof(pipe_mesg)) {
		printf("Failed to write message to statistics thread:\n"
			  "expected message size: %u, writen message size: %d\n",
			  sizeof(pipe_mesg), write_size);
		return -1;
	}

	//check if it's a stream end null frame indicator
	if (frame_desc->stream_end) {
		end_of_stream[stream_id] = 1;
		deinit_stream_files(stream_id);
		goto write_stream_exit;
	}

	if (update_files_data(frame_desc, new_session) < 0) {
		printf("update files data failed \n");
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

write_stream_exit:
	return 0;
}

#if 0
//return 1 is IAV is in encoding,  else, return 0
static int is_video_encoding(void)
{
	int state;

	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0 ) {
		perror("IAV_IOC_GET_IAV_STATE");
		exit(1);
	}

	return (state == IAV_STATE_ENCODING);
}
#endif

static int show_waiting(void)
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

static int capture_encoded_video()
{
	int rval;
	//open file handles to write to
	u64 total_frames;
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

	while (write_video_file_run) {
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
			statistics_run = 0;
			break;
		}
	}

	printf("stop encoded stream capture\n");

	printf("total_frames = %lld\n", total_frames);
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
	statistics_run = 0;
	write_video_file_run = 0;
}

static void stop_statistics()
{
	pipe_mesg_t msg;

	memset(&msg, 0, sizeof(msg));
	msg.stop = 1;
	printf("Sending stop statistics message!\n");
	if (write(pipefd[1], &msg, sizeof(msg)) != sizeof(msg)) {
		printf("Failed to send stop message to statistics thread: %s!\n", strerror(errno));
	}
	if (pipefd[1] >= 0) {
		close(pipefd[1]);
		pipefd[1] = -1;
	}
}

static int map_bsb(void)
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
	int ret = 0;
	int err = 0;
	pthread_t tid;

	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	do {
		if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
			perror("/dev/iav");
			ret = -1;
			break;
		}

		if (argc < 2) {
			usage();
			ret = -1;
			break;
		}

		if (init_param(argc, argv) < 0) {
			printf("init param failed \n");
			ret = -1;
			break;
		}

		if (map_bsb() < 0) {
			printf("map bsb failed\n");
			ret = -1;
			break;
		}

		if (init_data() < 0) {
			printf("data initiation failed!\n");
			ret = -1;
			break;
		}

		if (init_transfer() < 0) {
			ret = -1;
			break;
		}

		if (pipe(pipefd) < 0) {
			printf("Pipe create error!\n");
			ret = -1;
			break;
		}

		fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

		err = pthread_create(&tid, NULL, do_statistics, NULL);
		if (err != 0) {
			printf("can't create do statistics thread: %s\n", strerror(err));
			ret = -1;
			break;
		}
	}while (0);

	if (ret == 0) {
		if (capture_encoded_video() < 0) {
			printf("capture encoded video failed \n");
			ret = -1;
		}
		stop_statistics();
		pthread_join(tid, NULL);
	}

	if (deinit_transfer() < 0) {
		printf("deinit_transfer failed.\n");
		ret = -1;
	}

	deinit_stream_files(-1);

	if (fd_iav) {
		close(fd_iav);
		fd_iav = -1;
	}

	return ret;
}


