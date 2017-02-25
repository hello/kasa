/*
 * test_bsreader.c
 * the program use "bsreader" library to read bit streams, instead of direct IAV IOCTL
 * all other parts are same as test_stream.c
 * the benefit of using "bsreader" is being able to use multi threading to handle each
 * stream independantly, and have 100% control of encoded frames by "bsreader"
 *
 * this program may stop encoding if the encoding was started to be "limited frames",
 * or stop all encoding when this program was forced to quit
 *
 * History:
 *	2010/5/17 - [Louis Sun] create to use "bsreader" to get encoded streams
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
#include <linux/spi/spidev.h>

#include <signal.h>
#include <pthread.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include "bsreader.h"
#include "datatx_lib.h"


//#define ENABLE_RT_SCHED

/* when buffered frame number exceeds MAX_BUFFERED_FRAME_NUM,
 * test_bsreader will over write the frames earlier buffered in the buffer
 */
#define MAX_BUFFERED_FRAME_NUM 64

#define BASE_PORT			(2000)

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


/* Check hawthon.dts file in boards/hawthon/bsp for the device node */
#define MAX7219_SPI_DEV_NODE		"/dev/spidev0.2"
// for led display
#define MAX7219_WRITE_REGISTER(addr, val)	\
	do {	\
		cmd = ((addr) << 8) | (val);	\
		write(fd_spi, &cmd, 2);			\
	} while (0)
#define DIGITAL_SEGMENT 8
#define OFFSET_SEGMENT 3

//debug options
#undef DEBUG_PRINT_FRAME_INFO

#define ACCURATE_FPS_CALC

// the device file handle
int fd_iav;
int fd_spi;
u16 cmd;

// the bitstream buffer
u8 *bsb_mem;
u32 bsb_size;

static int led_display_flag = 0;
static int nofile_flag = 0;
static int frame_info_flag = 0;
static int show_pts_flag = 0;
static int file_size_flag = 0;
static int file_size_mega_byte = 100;
static int remove_time_string_flag = 0;

static int fps_statistics_interval = 300;
static int print_interval = 30;

// bitstream filename base
const char *default_filename;
static char filename[256]= "media/default";
static int user_filename_flag = 0;

//create files for writing
static int transfer_method = TRANS_METHOD_NFS;
static int transfer_port = BASE_PORT;
const char *default_filename_nfs = "/mnt/media/test";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";

static int G_bsreader_opened = 0;

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

	DURATION_TEST = TEST_OPTIONS_BASE,
	REMOVE_TIME_STRING,
	LED_DISPLAY,
};

static struct option long_options[] = {
	{"filename",	HAS_ARG,	0,	'f'},
	{"tcp",		NO_ARG,		0,	't'},
	{"usb",		NO_ARG,		0,	'u'},
	{"stdout",	NO_ARG,		0,	'o'},
	{"nofile",		NO_ARG,		0,	NOFILE_TRANSER},
	{"frames",	HAS_ARG,	0,	TOTAL_FRAMES},
	{"size",		HAS_ARG,	0,	TOTAL_SIZE},
	{"file-size",	HAS_ARG,	0,	FILE_SIZE},
	{"frame-info",	NO_ARG,		0,	SAVE_FRAME_INFO},
	{"show-pts",	NO_ARG,		0,	SHOW_PTS_INFO},
	{"rm-time",	NO_ARG,		0,	REMOVE_TIME_STRING},
	{"fps-intvl",	HAS_ARG,	0,	'i'},
	{"frame-intvl",	HAS_ARG,	0,	'n'},
	{"led", NO_ARG, 0, LED_DISPLAY},
	{0, 0, 0, 0}
};

static const char *short_options = "f:tuoi:n:";

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
	{"frames", "\tspecify how many frames to encode"},
	{"size", "\tspecify how many bytes to encode"},
	{"size", "\tcreate new file to capture when size is over maximum (MB)"},
	{"", "\t\tgenerate frame info"},
	{"", "\t\tshow stream pts info"},
	{"", "\t\tremove time string from the file name"},
	{"", "\t\tset fps statistics interval"},
	{"", "\tset frame/field statistic interval"},
	{"", "\t\tDisplay stream A's frame number on LED driven by Maxim 7219 controller"},

};

void usage(void)
{
	int i;
	printf("test_bsreader usage:\n");
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


#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)

typedef struct stream_encoding_state_s{
	int session_id;	//stream encoding session
	int fd;		//stream write file handle
	int fd_info;	//info write file handle
	int total_frames; // count how many frames encoded, help to divide for session before session id is implemented
	u64 total_bytes;  //count how many bytes encoded
	int pic_type;	  //picture type,  non changed during encoding, help to validate encoding state.
	u32 pts;
	u64 mono_pts;

	struct timeval capture_start_time;	//for statistics only,  this "start" is captured start, may be later than actual start

#ifdef ACCURATE_FPS_CALC
	int 	 total_frames2;	//for statistics only
	struct timeval time2;	//for statistics only
#endif

} stream_encoding_state_t;

static stream_encoding_state_t encoding_states[MAX_ENCODE_STREAM_NUM];
static stream_encoding_state_t old_encoding_states[MAX_ENCODE_STREAM_NUM];  //old states for statistics only

static int int2char(u32 source, u8 *target)
{
	const u16 radix = 10;
	int len = 0;
	do {
		target[len] = source % radix;
		source /= radix;
		len++;
	} while ((source > 0) && (len <= (DIGITAL_SEGMENT  - OFFSET_SEGMENT)));
	return len;
}

static int led_display()
{
	int i, mode = 0, bits = 16, max_speed = 1200000;
	if ((fd_spi = open(MAX7219_SPI_DEV_NODE, O_RDWR)) < 0) {
		printf("Can't open MAX7219_SPI_DEV_NODE to write \n");
		return -1;
	}
	if (ioctl(fd_spi, SPI_IOC_WR_MODE, &mode) < 0) {
		perror("SPI_IOC_WR_MODE");
		return -1;
	}
	if (ioctl(fd_spi, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
		perror("SPI_IOC_WR_BITS_PER_WORD");
		return -1;
	}

	if (ioctl(fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed) < 0) {
		perror("SPI_IOC_WR_MAX_SPEED_HZ");
		return -1;
	}

	MAX7219_WRITE_REGISTER(0xa, 0xa);
	MAX7219_WRITE_REGISTER(0x9, 0xff);
	MAX7219_WRITE_REGISTER(0xb, 0x7);
	MAX7219_WRITE_REGISTER(0xc, 0x1);
	MAX7219_WRITE_REGISTER(0xf, 0x0);
	for (i = 1; i <= DIGITAL_SEGMENT; i++) {
		MAX7219_WRITE_REGISTER(i, 0x7f);
	}
	return 0;
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'f':
			strcpy(filename, optarg);
			user_filename_flag = 1;
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'o':
			transfer_method = TRANS_METHOD_STDOUT;
			break;
		case 'i':
			fps_statistics_interval = atoi(optarg);
			break;
		case 'n':
			print_interval = atoi(optarg);
			break;
		case NOFILE_TRANSER:
			nofile_flag = 1;
			break;
		case TOTAL_FRAMES:
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
		case REMOVE_TIME_STRING:
			remove_time_string_flag = 1;
			break;
		case LED_DISPLAY:
			led_display_flag = 1;
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	if (transfer_method == TRANS_METHOD_TCP || transfer_method == TRANS_METHOD_USB)
		default_filename = default_filename_tcp;
	else
		default_filename = default_filename_nfs;

	if (nofile_flag)
		transfer_method = TRANS_METHOD_NONE;

	if (!user_filename_flag) {
		strcpy(filename, default_filename);
	}

	return 0;
}



int init_encoding_states(void)
{
	int i;
	//init all file hander and session id to invalid at start
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		encoding_states[i].fd = -1;
		encoding_states[i].fd_info = -1;
		encoding_states[i].session_id = -1;
		encoding_states[i].total_bytes = 0;
		encoding_states[i].total_frames = 0;
		encoding_states[i].pic_type = 0;
		encoding_states[i].pts = 0;
	}
	return 0;
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
int write_frame_info_header(int stream_id)
{
	struct iav_h264_cfg config;
	int version = VERSION;
	u32 size = sizeof(config);
	int fd_info = encoding_states[stream_id].fd_info;

	config.id = (1 << stream_id);
	if (ioctl(fd_iav, IAV_IOC_GET_H264_CONFIG, &config) < 0) {
		perror("IAV_IOC_GET_H264_CONFIG");
		return -1;
	}
	if (amba_transfer_write(fd_info, &version, sizeof(int), transfer_method) < 0 ||
		amba_transfer_write(fd_info, &size, sizeof(u32), transfer_method) < 0 ||
		amba_transfer_write(fd_info, &config, sizeof(config), transfer_method) < 0) {
		perror("write_data(4)");
		return -1;
	}

	return 0;
}

//return 0 if it's not new session,  return 1 if it is new session
int is_new_session(bs_info_t * bs_info)
{
	int stream_id = bs_info->stream_id;
	int new_session = 0 ;
	if  (bs_info ->session_id != encoding_states[stream_id].session_id) {
		//a new session
		new_session = 1;
	}
	if (file_size_flag) {
		if ((encoding_states[stream_id].total_bytes / 1024) > (file_size_mega_byte * 1024))
			new_session = 1;
	}

	return new_session;
}

//check session and update file handle for write when needed
int check_session_file_handle(bs_info_t *bs_info,  int new_session)
{
	char write_file_name[1024];
	char time_str[256];
	int stream_id = bs_info->stream_id;
	char stream_name;

   	if (new_session) {
		//close old session if needed
		if (encoding_states[stream_id].fd > 0) {
			close(encoding_states[stream_id].fd);
			encoding_states[stream_id].fd = -1;
		}
		//character based stream name
		stream_name = 'A' + stream_id;

		get_time_string(time_str, sizeof(time_str));
		if (remove_time_string_flag) {
			memset(time_str, 0, sizeof(time_str));
		}
		sprintf(write_file_name, "%s_%c_%s_%x.%s", filename, stream_name, time_str,
			bs_info->session_id, (bs_info->pic_type == IAV_PIC_TYPE_MJPEG_FRAME)? "mjpeg":"h264");
		if ((encoding_states[stream_id].fd =
			amba_transfer_open(write_file_name, transfer_method, (transfer_port+stream_id*2))) < 0) {
			printf("create file for write failed %s \n", write_file_name);
			return -1;
		} else {
			if (!nofile_flag)
				printf("\nnew session file name [%s], fd [%d].\n", write_file_name,
					encoding_states[stream_id].fd);
		}

		if (frame_info_flag) {
			sprintf(write_file_name, "%s.info", write_file_name);
			if ((encoding_states[stream_id].fd_info =
				amba_transfer_open(write_file_name, transfer_method, (transfer_port+stream_id*2+1))) < 0) {
				printf("create file for frame info failed %s \n", write_file_name);
				return -1;
			}
			if (write_frame_info_header(stream_id) < 0) {
				printf("write h264 header info file [%s] failed.\n", write_file_name);
				return -1;
			}
		}
	}
	return 0;
}

int update_session_data(bsreader_frame_info_t * bs_info, int new_session)
{
	int stream_id = bs_info->bs_info.stream_id;
	//update pic type, session id on new session
	if (new_session) {
		encoding_states[stream_id].pic_type = bs_info->bs_info.pic_type;
		encoding_states[stream_id].session_id = bs_info->bs_info.session_id;
		encoding_states[stream_id].total_bytes = 0;
		encoding_states[stream_id].total_frames = 0;
		old_encoding_states[stream_id] = encoding_states[stream_id];	//for statistics data only

#ifdef ACCURATE_FPS_CALC
		old_encoding_states[stream_id].total_frames2 = 0;
		old_encoding_states[stream_id].time2 = old_encoding_states[stream_id].capture_start_time;	//reset old counter
#endif
	}

	//update statistics on all frame
	encoding_states[stream_id].total_bytes += bs_info->frame_size;
	encoding_states[stream_id].total_frames ++;
	encoding_states[stream_id].pts = bs_info->bs_info.mono_pts;

	return 0;
}

int write_frame_info(bsreader_frame_info_t * video_frame_info)
{
	typedef struct video_frame_s {
		u32     pic_type;
		u32     pts;
		u32     size;
		u32     frame_num;
	} video_frame_t;
	video_frame_t frame_info;
	frame_info.pic_type = video_frame_info->bs_info.pic_type;
	frame_info.pts = video_frame_info->bs_info.PTS;
	frame_info.size = video_frame_info->frame_size;
	frame_info.frame_num = video_frame_info->bs_info.frame_num;

	int fd_info = encoding_states[video_frame_info->bs_info.stream_id].fd_info;

	if (amba_transfer_write(fd_info, &frame_info, sizeof(frame_info), transfer_method) < 0) {
		perror("write(5)");
		return -1;
	}
	return 0;
}


int write_video_file2(bsreader_frame_info_t * video_frame_info)
{
	int fd = encoding_states[video_frame_info->bs_info.stream_id].fd;
	//each frame data from bsreader must be continuous
	if (amba_transfer_write(fd, (void*) video_frame_info->frame_addr,
		video_frame_info->frame_size, transfer_method) < 0) {
		perror("write(1)");
		return -1;
	}

	return 0;
}


int write_stream2(bsreader_frame_info_t* bs_info, int * total_frames, u64 *total_bytes)
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
	u32 pre_pts, curr_pts;
	int mono_diff;
	char stream_name[128];
	static int pre_frame_num[MAX_ENCODE_STREAM_NUM];
	//update current frame encoding time
	stream_id = bs_info->bs_info.stream_id;
//	printf("stream_id is %d \n", stream_id);
	int display_len, i;
	u8 display[DIGITAL_SEGMENT - OFFSET_SEGMENT];
	if (led_display_flag) {
		if (stream_id == 0) {
			display_len = int2char(bs_info->bs_info.frame_num, display);
			for (i = 1; i <= display_len; i++) {
				MAX7219_WRITE_REGISTER(i+OFFSET_SEGMENT, display[i-1]);
			}
		}
	}

	gettimeofday(&encoding_states[stream_id].capture_start_time, NULL);

#ifdef DEBUG_PRINT_FRAME_INFO
	printf("frame type %d,frame number %d, pts %d, pic_size %d,"
		" start_addr %p, stream_id %d, session_id %d \n",
		bs_info->bs_info.pic_type, bs_info->bs_info.frame_num,
		bs_info->bs_info.PTS, bs_info->frame_size,
		bs_info->frame_addr, bs_info->bs_info.stream_id,
		bs_info->bs_info.session_id);
#endif

	//check if it's a stream end null frame indicator
	if (bs_info->bs_info.stream_end) {
		//printf("close file of stream %d at end, session id %d \n", stream_id,  bs_info->bs_info.session_id);
		if (encoding_states[stream_id].fd > 0) {
			amba_transfer_close(encoding_states[stream_id].fd, transfer_method);
			encoding_states[stream_id].fd = -1;
		}
		if (encoding_states[stream_id].fd_info > 0) {
			amba_transfer_close(encoding_states[stream_id].fd, transfer_method);
			encoding_states[stream_id].fd_info = -1;
		}
		return 0;
	}

	// check if it's new record session, since file name and recording control are
	// based on session,  session id and change are important data
	new_session = is_new_session(&bs_info->bs_info);

	if (new_session)
		pre_frame_num[stream_id] = 0;

	if (((int)bs_info->bs_info.frame_num > 0) &&
		((int)bs_info->bs_info.frame_num <= pre_frame_num[bs_info->bs_info.stream_id])) {
		printf("incorrect frame num %d -> %d !\n", bs_info->bs_info.frame_num,
			pre_frame_num[bs_info->bs_info.stream_id]);
	}
//	printf("[%d] frame num %d\n", bs_info->bs_info.stream_id, bs_info->bs_info.frame_num);
	pre_frame_num[bs_info->bs_info.stream_id] = bs_info->bs_info.frame_num;

	//update session data
	if (update_session_data(bs_info, new_session) < 0) {
		printf("update session data failed \n");
		return -2;
	}

	//check and update session file handle
	if (check_session_file_handle(&bs_info->bs_info, new_session) < 0) {
		printf("check session file handle failed \n");
		return -3;
	}

	if (frame_info_flag) {
		if (write_frame_info(bs_info) < 0) {
			printf("write video frame info failed for stream %d, session id = %d.\n",
				stream_id, bs_info->bs_info.session_id);
			return -5;
		}
	}

	//write file if file is still opened
	if (write_video_file2(bs_info) < 0) {
		printf("write video file failed for stream %d, session id = %d \n",
			stream_id, bs_info->bs_info.session_id);
		return -4;
	}

	//update global statistics
	if (total_frames) {
		*total_frames = (*total_frames) + 1;
	}

	if (total_bytes) {
		*total_bytes  = (*total_bytes) + bs_info->frame_size;
	}

	//print statistics
	pre_time = old_encoding_states[stream_id].capture_start_time;
	curr_time = encoding_states[stream_id].capture_start_time;
	pre_frames = old_encoding_states[stream_id]. total_frames;
	curr_frames = encoding_states[stream_id]. total_frames;
	pre_bytes = old_encoding_states[stream_id].total_bytes;
	curr_bytes = encoding_states[stream_id].total_bytes;
	pre_pts = old_encoding_states[stream_id].pts;
	curr_pts = encoding_states[stream_id].pts;

	if (show_pts_flag) {
		sprintf(stream_name, "stream %c", 'A' + stream_id);
		mono_diff = bs_info->bs_info.mono_pts - old_encoding_states[stream_id].mono_pts;
		printf("%s:\tFrame NO: %d, PTS: %u, diff: %d, mono_pts=%llu,"
			" mono_diff=%d, frame size: %d\n", stream_name, curr_frames,
			curr_pts, (curr_pts - pre_pts), bs_info->bs_info.mono_pts,
			mono_diff, bs_info->frame_size);
		old_encoding_states[stream_id].pts = encoding_states[stream_id].pts;
		old_encoding_states[stream_id].mono_pts = bs_info->bs_info.mono_pts;
	}
	if ((curr_frames % print_interval == 0) && (print_frame ) ) {
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
		int pre_frames2;
		struct timeval pre_time2;
		pre_frames2 = old_encoding_states[stream_id].total_frames2;
		pre_time2 = old_encoding_states[stream_id].time2;
		if ((curr_frames % fps_statistics_interval == 0) &&(print_frame)) {
			time_interval_us2 = (curr_time.tv_sec - pre_time2.tv_sec) * 1000000 +
						curr_time.tv_usec - pre_time2.tv_usec;
			double fps = (curr_frames - pre_frames2) * 1000000.0/(double)time_interval_us2;
			BOLD_PRINT("AVG FPS = %4.2f\n",fps);
			old_encoding_states[stream_id].total_frames2 = encoding_states[stream_id].total_frames;
			old_encoding_states[stream_id].time2 = encoding_states[stream_id].capture_start_time;
		}
	}
	#endif

	return 0;
}


int stop_all_encode(void)
{
	if (ioctl(fd_iav, IAV_IOC_STOP_ENCODE, 0) < 0) {
		perror("IAV_IOC_STOP_ENCODE");
		return -1;
	}

	printf("\nstop_encode\n");

	return 0;
}


//return 1 is IAV is in encoding,  else, return 0
int is_video_encoding(void)
{
	u32 info;
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		exit(1);
	}
	return (info == IAV_STATE_ENCODING);
}

int show_waiting(void)
{
	#define DOT_MAX_COUNT 10
	static int dot_count = DOT_MAX_COUNT;
	int i;

	if (dot_count < DOT_MAX_COUNT) {
		fprintf(stderr, ".");	//print a dot to indicate it's alive
		dot_count++;
	} else {
		fprintf(stderr, "\r");
		for ( i = 0; i < 80 ; i++)
			fprintf(stderr, " ");
		fprintf(stderr, "\r");
		dot_count = 0;
	}

	fflush(stderr);
	return 0;
}

typedef struct thread_local_data_s {
	int stream;
} thread_local_data_t;


static pthread_t G_thread_id[MAX_ENCODE_STREAM_NUM];
static int G_thread_force_quit[MAX_ENCODE_STREAM_NUM];
static thread_local_data_t G_thread_local_data[MAX_ENCODE_STREAM_NUM];
static void * reading_thread_func(void * arg)
{
	thread_local_data_t * pthread_data = (thread_local_data_t * )arg;
	bsreader_frame_info_t frame_info;
	int retv = 0;

	int stream = pthread_data->stream;
	while (!G_thread_force_quit[stream]) {
		retv = bsreader_get_one_frame(stream, &frame_info);
		if (retv < 0) {
			if (retv != -2) {
				printf("bs reader gets frame error\n");
				return NULL;
			}
			usleep(1*1000);
			continue;
		}

		//dump frame info
//		printf("*****READ  [%d] frame_num %d, frame_size %d \n",
//				stream, frame_info.bs_info.frame_num, frame_info.frame_size);

		if (write_stream2(&frame_info, NULL, NULL) < 0) {
			printf("write stream 2 error \n");
			return NULL;
		}
	}

	return NULL;
}

static int create_working_thread(int stream)
{
	pthread_t tid;
	int ret;

	G_thread_local_data[stream].stream  = stream;
	ret = pthread_create(&tid, NULL, reading_thread_func,
		&G_thread_local_data[stream]);
	if (ret != 0) {
		perror("main thread create failed");
		return -1;
	}

	G_thread_id[stream] = tid;
	return 0;
}


int capture_encoded_video()
{
	int i, total_size;
	//init bsreader

	bsreader_init_data_t  init_data;
	memset(&init_data, 0, sizeof(init_data));
	init_data.fd_iav = fd_iav;
	init_data.max_stream_num = MAX_ENCODE_STREAM_NUM;

	init_data.max_buffered_frame_num = MAX_BUFFERED_FRAME_NUM;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		switch (i) {
		case 0:
			total_size = 8 << 20;
			break;
		case 1:
			total_size = 2 << 20;
			break;
		default:
			total_size = 1 << 20;
			break;
		}
		init_data.ring_buf_size[i] = total_size;
	}
	/* Enable cavlc encoding when configured.
	 * Enable this option will use more memory in bsreader.
	 */
	init_data.cavlc_possible = 1;

	if (bsreader_init(&init_data) < 0) {
		printf("bsreader init failed \n");
		return -1;
	}

	if (bsreader_open() < 0) {
		printf("bsreader open failed \n");
		return -1;
	}

	//create four threads, each read its related streams
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (create_working_thread(i) < 0) {
			perror("create thread");
			return -1;
		}
	}

	//by now, bsreader init completes and thread created
	G_bsreader_opened = 1;

	//sleep for a relatively long time

	while (1) {
		if (!is_video_encoding()) {
			usleep(100 * 1000);
			show_waiting();
		} else {
			usleep(100 * 1000);
		}
	}

	return 0;
}


static void sigstop()
{
	int i;
	if (G_bsreader_opened) {
//		printf("cancel all threads \n");
		for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
			if (pthread_cancel(G_thread_id[i]) < 0) {
				perror("cancel thread");
				printf("%d \n", i);
			} else {
			pthread_join(G_thread_id[i], NULL);
//			printf("thread %d cancelled\n", i);
			}
		}
		if (bsreader_close() < 0) {
			printf("bsreader_close() failed\n");
		}
	}
	amba_transfer_deinit(transfer_method);
	if (led_display_flag) {
		MAX7219_WRITE_REGISTER(0xc, 0x0);
		close(fd_spi);
	}

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
	version_t version;
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

	bsreader_get_version(&version);
	printf("Bsreader Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
	    version.description, version.major, version.minor,
	    version.patch, version.mod_time);


	if (led_display_flag) {
		if (led_display() < 0)
			return -1;
	}

	if (map_bsb() < 0) {
		printf("map bsb failed\n");
		return -1;
	}

	init_encoding_states();

	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}

	if (capture_encoded_video() < 0) {
		printf("capture encoded video failed \n");
		return -1;
	}

	if (amba_transfer_deinit(transfer_method) < 0) {
		return -1;
	}

	close(fd_iav);
	return 0;
}


