/*
 * test_fastosd.c
 *
 * History:
 * 2014/11/24  - [Zhi He] created for test fast osd
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
#include <linux/rtc.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <iav_ioctl.h>

#include <signal.h>

#define MAX_ENCODE_STREAM_NUM               (4)
#define YEAR_BASE 			(1900)
#define MONTH_BASE 		(1)

static char gs_fastosd_font_index_file[256] = "/usr/local/bin/font_index.bin";
static char gs_fastosd_font_map_file[256] = "/usr/local/bin/font_map.bin";
static char gs_fastosd_clut_file[256] = "/usr/local/bin/clut.bin";

static int running = 1;

static struct iav_fastosd_insert fastosd_insert[MAX_ENCODE_STREAM_NUM];

int fd_fastosd_overlay;
int fastosd_overlay_size;
u8 *fastosd_overlay_addr;

static u32 fastosd_font_index_offset = 0x0000;
static u32 fastosd_font_map_offset = 0x1000;

//small font
//static u32 fastosd_font_pitch = 256;
//static u32 fastosd_font_height = 16;
//static u32 fastosd_font_width = 14;

//large font
static u32 fastosd_font_pitch = 16 * 32;
static u32 fastosd_font_height = 32;
static u32 fastosd_font_width = 24;

static u32 fastosd_stringosd_offset = 0;
static u32 fastosd_stringosd_string_size = 0x1000;
static u32 fastosd_stringosd_clut_size = 0x1000;
static u32 fastosd_stringosd_output_size = 0x2000;

static int auto_update_time_osd_flag = 0;
static int verbose_flag = 0;

static char fastosd_display_string[64] = "2014/12/8-12:01:01";

static int check_encode_status(void)
{
	int iav_state;

	/* IAV must be in ENOCDE or PREVIEW state */
	if (ioctl(fd_fastosd_overlay, IAV_IOC_GET_IAV_STATE, &iav_state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}

	if ((iav_state != IAV_STATE_PREVIEW) &&
			(iav_state != IAV_STATE_ENCODING)) {
		printf("IAV must be in PREVIEW or ENCODE for text OSD.\n");
		return -1;
	}

	return 0;
}

static int map_overlay(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_OVERLAY;
	if (ioctl(fd_fastosd_overlay, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	fastosd_overlay_size = querybuf.length;
	fastosd_overlay_addr = mmap(NULL, fastosd_overlay_size, PROT_WRITE, MAP_SHARED,
		fd_fastosd_overlay, querybuf.offset);
	printf("overlay: map_overlay, start = 0x%p, total size = 0x%x\n",
	       fastosd_overlay_addr, fastosd_overlay_size);
	if (fastosd_overlay_addr == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int ioctl_set_fastosd(int stream_id)
{
	if (ioctl(fd_fastosd_overlay, IAV_IOC_SET_FASTOSD_INSERT, &fastosd_insert[stream_id]) < 0) {
		perror("IAV_IOC_SET_FASTOSD_INSERT");
		return -1;
	}

	return 0;
}

static int disable_fastosd(int stream_id)
{
	fastosd_insert[stream_id].enable = 0;
	return ioctl_set_fastosd(stream_id);
}

#define NO_ARG	  0
#define HAS_ARG	 1
static struct option long_options[] = {
	{"font_width", HAS_ARG, 0, 'w'},
	{"font_height", HAS_ARG, 0, 'h'},
	{"font_pitch", HAS_ARG, 0, 'p'},

	{"xstart", HAS_ARG, 0, 'x'},
	{"ystart", HAS_ARG, 0, 'y'},

	{"clut_file", HAS_ARG, 0, 'c'},
	{"fontmap_index", HAS_ARG, 0, 'i'},
	{"fontmap_file", HAS_ARG, 0, 'm'},
	{"string", HAS_ARG, 0, 't'},
	{"auto", HAS_ARG, 0, 'a'},
	{"verbose", HAS_ARG, 0, 'v'},

	{"disable", NO_ARG, 0, 's'},
	{0, 0, 0, 0}
};

static const char *short_options = "w:h:p:x:y:c:i:m:t:avs";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"integer", "each char font's width"},
	{"integer", "fontmap's height"},
	{"integer", "fontmap's pitch"},
	{"", "\t\tset x"},
	{"", "\t\tset y"},
	{"filename", "read clut from file"},
	{"filename", "read font index from file"},
	{"filename", "read font map from file"},
	{"string", "display string"},
	{"", "auto get hw timer and update it on OSD"},
	{"", "Show verbose info"},

	{"", "\t\tturn off fastosd"},
};

void usage(void)
{
	unsigned int i;

	printf("test_fastosd usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val)) {
			printf("-%c ", long_options[i].val);
		} else {
			printf("   ");
		}
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0) {
			printf(" [%s]", hint[i].arg);
		}
		printf("\t%s\n", hint[i].str);
	}
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'w':
			fastosd_font_width = atoi(optarg);
			break;
		case 'h':
			fastosd_font_height = atoi(optarg);
			break;
		case 'p':
			fastosd_font_pitch = atoi(optarg);
			break;
		case 'x':
			break;
		case 'y':
			break;
		case 'c':
			break;
		case 'i':
			break;
		case 'm':
			break;
		case 't':
			strncpy(fastosd_display_string, optarg, sizeof(fastosd_display_string));
			break;
		case 'a':
			auto_update_time_osd_flag = 1;
			break;
		case 'v':
			verbose_flag = 1;
			break;
		case 's':
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static void sigstop()
{
	running = 0;
}

static char* load_binary_to_mem(const char* filename, int* size)
{
	char *p = NULL;
	long filesize = 0;

	if (!filename) {
		printf("error: NULL params\n");
		return NULL;
	}

	FILE* file = fopen(filename, "rb");
	if (!file) {
		printf("error: open(%s) fail\n", filename);
		return NULL;
	}

	do{
		fseek(file, 0x0, SEEK_END);
		filesize = ftell(file);
		p = (char*)malloc(filesize);
		if (!p) {
			printf("error: not enough memory, request size %ld\n", filesize);
			break;
		}
	}while(0);

	fseek(file, 0x0, SEEK_SET);
	if (p) {
		fread(p, 1, filesize, file);
	}
	*size = (int)filesize;
	fclose(file);

	return p;
}

static int load_font_clut(int i, u32 font_index_offset, u32 font_map_offset, u32 clut_offset)
{
	int font_index_size = 0;
	int font_map_size = 0;
	int clut_size = 0;

	char* p_font_index = load_binary_to_mem((const char*)gs_fastosd_font_index_file, &font_index_size);
	char* p_font_map = load_binary_to_mem((const char*)gs_fastosd_font_map_file, &font_map_size);
	char* p_clut = load_binary_to_mem((const char*)gs_fastosd_clut_file, &clut_size);
	if ((!p_font_index) || (!p_font_map) || (!p_clut)) {
		printf("[error]: load binary file fail, %p, %p, %p\n", p_font_index, p_font_map, p_clut);
		return (-1);
	}

	memcpy(fastosd_overlay_addr + font_index_offset, p_font_index, font_index_size);
	memcpy(fastosd_overlay_addr + font_map_offset, p_font_map, font_map_size);
	memcpy(fastosd_overlay_addr + clut_offset, p_clut, clut_size);

	return 0;
}

static void preset_fontmap_memory_layout(int i)
{
	fastosd_font_index_offset = 0x0000;
	fastosd_font_map_offset = 0x1000;
	fastosd_stringosd_offset = fastosd_font_map_offset + (fastosd_font_pitch * fastosd_font_height);

	fastosd_stringosd_clut_size = 0x1000;
	fastosd_stringosd_string_size = 0x1000;
	fastosd_stringosd_output_size = 0x2000;

	fastosd_insert[i].enable = 1;
	fastosd_insert[i].id = i;
	fastosd_insert[i].string_num_region = 1;
	fastosd_insert[i].osd_num_region = 0;
	fastosd_insert[i].font_index_offset = fastosd_font_index_offset;
	fastosd_insert[i].font_map_offset = fastosd_font_map_offset;
	fastosd_insert[i].font_map_pitch = fastosd_font_pitch;
	fastosd_insert[i].font_map_height = fastosd_font_height;
}

static void preset_stringosd_memory_layout(int i, int num_string_area)
{
	int index = 0;
	u32 offset = fastosd_stringosd_offset;
	for (index = 0; index < num_string_area; index ++) {
		fastosd_insert[i].string_area[index].clut_offset = offset;
		offset += fastosd_stringosd_clut_size;
		fastosd_insert[i].string_area[index].string_offset = offset;
		offset += fastosd_stringosd_string_size;
		fastosd_insert[i].string_area[index].string_output_offset = offset;
		offset += fastosd_stringosd_output_size;
		fastosd_insert[i].string_area[index].string_output_pitch = 32 * 32;//max for 32 char * 32 (max font with)
	}
}

static int load_fastosd_mem(int stream_id)
{
	int i = 0;

	memset(fastosd_insert, 0x0, sizeof(fastosd_insert));

	fastosd_insert[0].enable = 1;
	fastosd_insert[0].id = stream_id;
	fastosd_insert[0].string_num_region = 1;
	fastosd_insert[0].osd_num_region = 0;

	preset_fontmap_memory_layout(stream_id);
	preset_stringosd_memory_layout(stream_id, 1);
	fastosd_insert[stream_id].string_area[0].enable = 1;
	fastosd_insert[stream_id].string_area[0].offset_x = 16;
	fastosd_insert[stream_id].string_area[0].offset_y = 16;

	i = load_font_clut(stream_id, fastosd_insert[stream_id].font_index_offset,
		fastosd_insert[stream_id].font_map_offset,
		fastosd_insert[stream_id].string_area[stream_id].clut_offset);
	if (0 > i) {
		printf("load_font_clut() fail\n");
		return (-3);
	}

	return i;
}

static int  rtc_xopen(const char **default_rtc, int flags)
{
	int rtc = 0;

	if (!*default_rtc) {
		*default_rtc = "/dev/rtc";
		rtc = open(*default_rtc, flags);
		if (rtc >= 0)
			return rtc;
		*default_rtc = "/dev/rtc0";
		rtc = open(*default_rtc, flags);
		if (rtc >= 0)
			return rtc;
		*default_rtc = "/dev/misc/rtc";
	}

	return open(*default_rtc, flags);
}

static void rtc_read_tm(struct tm *ptm, int fd)
{
	memset(ptm, 0, sizeof(*ptm));
	ioctl(fd, RTC_RD_TIME, ptm);
	ptm->tm_isdst = -1; /* "not known" */
}

static time_t rtc_tm2time(struct tm *ptm, int utc)
{
	char *oldtz = NULL; /* for compiler */
	time_t t;

	if (utc) {
		oldtz = getenv("TZ");
		putenv((char*)"TZ=UTC0");
		tzset();
	}

	t = mktime(ptm);

	if (utc) {
		unsetenv("TZ");
		if (oldtz)
			putenv(oldtz - 3);
		tzset();
	}

	return t;
}

static time_t read_rtc(const char **pp_rtcname, int utc)
{
	struct tm tm_time;
	int fd;

	fd = rtc_xopen(pp_rtcname, O_RDONLY);
	rtc_read_tm(&tm_time, fd);
	close(fd);

	return rtc_tm2time(&tm_time, utc);
}

static void show_clock(const char **pp_rtcname, int utc)
{
	time_t t = read_rtc(pp_rtcname, utc);

	/* Standard hwclock uses locale-specific output format */
	char cp[64];
	struct tm *ptm = localtime(&t);

	snprintf(fastosd_display_string, sizeof(fastosd_display_string), "%04d/%02d/%02d-%02d:%02d:%02d",
			(ptm->tm_year + YEAR_BASE),
			(ptm->tm_mon + MONTH_BASE),
			ptm->tm_mday,
			ptm->tm_hour,
			ptm->tm_min,
			ptm->tm_sec);

	if (verbose_flag) {
		printf ("fastosd_display_string[%d] = %s.\n", strlen(fastosd_display_string), fastosd_display_string);
		strftime(cp, sizeof(cp), "%c", ptm);
		printf("%s\n", cp);
	}
}

static void normal_loop(int stream_id)
{
	char buffer[128];
	char* p = NULL;
	char *p_start = NULL;
	int stringlen = 0;
	int maxlen = 32;
	int flag_stdin = 0;

	stringlen = strlen(fastosd_display_string);
	memcpy(fastosd_overlay_addr + fastosd_insert[stream_id].string_area[0].string_offset, fastosd_display_string, stringlen);
	fastosd_insert[stream_id].string_area[0].string_length = stringlen;
	fastosd_insert[stream_id].string_area[0].overall_string_width = stringlen * fastosd_font_width;
	ioctl_set_fastosd(stream_id);
	printf("width=%d, height=%d, pitch=%d.\n",
		fastosd_font_width, fastosd_font_height, fastosd_font_pitch);

	flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
	if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
		printf("[error]: stdin_fileno set error.\n");
	}

	while (running) {
		memset(buffer, 0x0, sizeof(buffer));

		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
			usleep(100000);
			continue;
		}
		switch (buffer[0]) {
			case 'q':
				printf("press 'q': quit\n");
				break;
			case 'p':
				{
					p = buffer + 1;
					if (p[0] == ' ') {
						p ++;
					}
					p_start = p;
					stringlen = 0;
					while (stringlen < maxlen) {
						if (p[stringlen] != 0x0) {
							if ((p[stringlen] < '0') || (p[stringlen] > ':')) {
								break;
							}
							stringlen ++;
						} else {
							break;
						}
					}
				}
				if (stringlen > 0) {
					memcpy(fastosd_overlay_addr + fastosd_insert[stream_id].string_area[0].string_offset, p_start, stringlen);
					fastosd_insert[stream_id].string_area[0].string_length = stringlen;
					fastosd_insert[stream_id].string_area[0].overall_string_width = stringlen * fastosd_font_width;
					ioctl_set_fastosd(stream_id);
				}
				continue;
		}
	}

	if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
		printf("[error]: stdin_fileno set error");
	}
}

static void auto_update_time_osd_loop(int stream_id)
{
	int utc = 0;
	int stringlen = 0;
	const char *rtcname = NULL;
	stringlen = strlen(fastosd_display_string);

	while (running) {
		show_clock(&rtcname, utc);
		stringlen = strlen(fastosd_display_string);
		memcpy(fastosd_overlay_addr + fastosd_insert[stream_id].string_area[0].string_offset, fastosd_display_string, stringlen);
		fastosd_insert[stream_id].string_area[0].string_length = stringlen;
		fastosd_insert[stream_id].string_area[0].overall_string_width =
			stringlen * fastosd_font_width;
		ioctl_set_fastosd(stream_id);
		sleep(1);
	}

}

int main(int argc, char **argv)
{
	int stream_id = 0;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_fastosd_overlay = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (argc < 2) {
		usage();
	} else {
		if (init_param(argc, argv) < 0) {
			return -1;
		}
	}

	if (check_encode_status() < 0) {
		return -1;
	}

	if (map_overlay() < 0) {
		return -1;
	}

	if (load_fastosd_mem(stream_id) < 0) {
		printf("load fastosd mem failed\n");
		return -1;
	}

	if (auto_update_time_osd_flag) {
		auto_update_time_osd_loop(stream_id);
	} else {
		normal_loop(stream_id);
	}

	if (fastosd_insert[stream_id].enable) {
		disable_fastosd(stream_id);
	}

	return 0;
}

