/*
 * test_textinsert.c
 *
 * History:
 *  2014/03/19 - [ywang] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <signal.h>
#include <getopt.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <basetypes.h>
#include <iav_ioctl.h>
#include <locale.h>
#include "text_insert.h"

#define MAX_ENCODE_STREAM_NUM	(4)
#define MAX_OVERLAY_AREA_NUM	(MAX_NUM_OVERLAY_AREA)
#define OVERLAY_CLUT_NUM		(16)
#define OVERLAY_CLUT_SIZE		(1024)
#define OVERLAY_CLUT_OFFSET		(0)
#define OVERLAY_YUV_OFFSET		(OVERLAY_CLUT_NUM * OVERLAY_CLUT_SIZE)
#define CLUT_OUTLINE_OFFSET		(OVERLAY_CLUT_NUM / 2)

#define STRING_SIZE				(255)
#define FIXED_DIGIT_WIDTH(x)	(2 * (x))
#define TOTAL_DIGIT_NUM			(10)
#define NOT_CONVERTED			(-1)

#ifndef LINE_SPACING
#define LINE_SPACING(x)		((x) * 3 / 2)
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#ifndef VERIFY_STREAMID
#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while(0)
#endif

#ifndef VERIFY_AREAID
#define VERIFY_AREAID(x)	do {		\
			if (((x) < 0) || ((x) >= MAX_OVERLAY_AREA_NUM)) {	\
				printf("area id wrong %d, not in range [0~3].\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#define	ROTATE_BIT		(0)
#define	HFLIP_BIT		(1)
#define	VFLIP_BIT		(2)
#define	SET_BIT(x)		(1 << (x))

typedef enum rotate_type_s {
	CLOCKWISE_0 = 0,
	CLOCKWISE_90 = SET_BIT(ROTATE_BIT),
	CLOCKWISE_180 = SET_BIT(HFLIP_BIT) | SET_BIT(VFLIP_BIT),
	CLOCKWISE_270 = SET_BIT(HFLIP_BIT) | SET_BIT(VFLIP_BIT) | SET_BIT(ROTATE_BIT),
	AUTO_ROTATE,
} rotate_type_t;

typedef struct time_display_s {
	int enable;
	char time_string[STRING_SIZE];
	int osd_x[STRING_SIZE];	/* Digit's offset x to the osd */
	int osd_y[STRING_SIZE];	/* Digit's offset y to the osd */
	int max_width;					/* Maximum width of 10 digits as bitmap
									 * width in time display string. */
	u8 *digits_addr;				/* Used to store 10 digits' bitmap data */
	int digit_width;				/* One digit's width in digits_addr */
	int digit_height;				/* One digit's height */
} time_display_t;

typedef struct textbox_s {
	int enable;
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	int color;
	int font_type;
	int font_size;
	int outline;
	int bold;
	int italic;
	rotate_type_t rotate;
	wchar_t content[STRING_SIZE];
	time_display_t time;
} textbox_t;

typedef struct stream_text_info_s {
	int enable;
	int win_width;
	int win_height;
	rotate_type_t rotate;
	textbox_t textbox[MAX_OVERLAY_AREA_NUM];
} stream_text_info_t;

typedef struct clut_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;
} clut_t;

typedef struct buffer_info_s {
	u16 buf_width;
	u16 buf_height;
	u16 sub_x;
	u16 sub_y;
	u16 sub_width;
	u16 sub_height;
	u8 *buf_base;
} buffer_info_t;

#define COLOR_NUM		8
static clut_t clut[] = {
	/* alpha 0 is full transparent */
	{ .y = 82, .u = 90, .v = 240, .alpha = 255 },	/* red */
	{ .y = 41, .u = 240, .v = 110, .alpha = 255 },	/* blue */
	{ .y = 12, .u = 128, .v = 128, .alpha = 255 },	/* black */
	{ .y = 235, .u = 128, .v = 128, .alpha = 255 },	/* white */
	{ .y = 145, .u = 54, .v = 34, .alpha = 255 },	/* green */
	{ .y = 210, .u = 16, .v = 146, .alpha = 255 },	/* yellow */
	{ .y = 170, .u = 166, .v = 16, .alpha = 255 },	/* cyan */
	{ .y = 107, .u = 202, .v = 222, .alpha = 255 }, /* magenta */

};

int fd_overlay;
int overlay_yuv_size;
u8 *overlay_clut_addr;

static stream_text_info_t stream_text_info[MAX_ENCODE_STREAM_NUM];
static struct iav_overlay_insert overlay_insert[MAX_ENCODE_STREAM_NUM];
static int flag_time_display = 0;

wchar_t wText[][STRING_SIZE] = {
	L"AmbarellaS2LSDK",
	L"安霸中国(Chinese)"
};

enum font_type {
	FONT_TYPE_ENGLISH		= 0,
	FONT_TYPE_CHINESE		= 1,
	FONT_TYPE_NUM
};

char font_type[FONT_TYPE_NUM][MAX_BYTE_SIZE] = {
	"/usr/share/fonts/DroidSans.ttf",
	"/usr/share/fonts/DroidSansFallbackFull.ttf"
};

pixel_type_t pixel_type = {
	.pixel_background = 30,
	.pixel_outline = 1,
	.pixel_font = 255,
};

enum {
	SPECIFY_OUTLINE = 130,
	SPECIFY_BOLD,
	SPECIFY_ITALIC,
	SPECIFY_ROTATE,
	SPECIFY_STRING,
	TIME_DISPLAY,
};

#define NO_ARG	0
#define HAS_ARG	1
static struct option long_options[] = {
	{ "streamA", NO_ARG, 0, 'A' },
	{ "streamB", NO_ARG, 0, 'B' },
	{ "streamC", NO_ARG, 0, 'C' },
	{ "streamD", NO_ARG, 0, 'D' },
	{ "area", HAS_ARG, 0, 'a' },
	{ "xstart", HAS_ARG, 0, 'x' },
	{ "ystart", HAS_ARG, 0, 'y' },
	{ "width", HAS_ARG, 0, 'w' },
	{ "height", HAS_ARG, 0, 'h' },
	{ "fonttype", HAS_ARG, 0, 't' },
	{ "fontsize", HAS_ARG, 0, 's' },
	{ "color", HAS_ARG, 0, 'c' },
	{ "outline", HAS_ARG, 0, SPECIFY_OUTLINE },
	{ "bold", HAS_ARG, 0, SPECIFY_BOLD },
	{ "italic", HAS_ARG, 0, SPECIFY_ITALIC },
	{ "norotate", NO_ARG, 0, SPECIFY_ROTATE },
	{ "string", HAS_ARG, 0, SPECIFY_STRING },
	{ "time", NO_ARG, 0, TIME_DISPLAY },
	{ 0, 0, 0, 0 }
};

static const char *short_options = "ABCDa:x:y:w:h:t:s:c:";

typedef struct hint_s {
	const char *arg;
	const char *str;
} hint_t;

static const hint_t hint[] = {
	{ "", "\tconfig stream A" },
	{ "", "\tconfig stream B" },
	{ "", "\tconfig stream C" },
	{ "", "\tconfig stream D" },
	{ "0~2", "\tconfig text area number in one stream" },
	{ "", "\tset OSD horizontal offset" },
	{ "", "\tset OSD vertical offset" },
	{ "", "\tset OSD width" },
	{ "", "\tset OSD height" },
	{ "0~1", "set font type, 0 DroidSans, 1 DroidSansFallbackFull" },
	{ "", "\tset font size " },
	{ "0~7", "set color, 0 red, 1 blue, 2 black, 3 white, 4 green, 5 yellow, 6 cyan, 7 magenta" },
	{ ">=0", "set outline size, 0 none outline" },
	{ "", "\tn percentage bold of font size. Negative means thinner." },
	{ ">=0", "n percentage italic of font size." },
	{ "", "\tWhen stream rotate 90/180/270 degree, OSD tries to be consistent with stream orientation."},
	{ "string", "text to be inserted" },
	{ "","\tdisplay current time." },
 };

static void usage(void)
{
	int i;

	printf("test_textinsert usage:\n");
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
	printf("\nExample:\n"
		"  Display string:\n"
		"    test_textinsert -A -a0 -x10 -y80 -w400 -h100 -s48 -t1 --string \"Hello World\"\n"
		"  Display time:\n"
		"    test_textinsert -A -a0 -x40 -y40 -w360 -h36 -s24 -c1 --outline 2 --time\n"
		"   Keep OSD upright in 90 degree clockwise rotate stream:\n"
		"    test_encode -A --smaxsize 720x1280 -h720p --rotate 1 --hflip 0 --vflip 0 -e\n"
		"    test_textinsert -A -a0 -x40 -y40 -w320 -h36 -s24 -c2 --outline 2 --time --norotate\n");
	printf("\n");
}

static void get_time_string(char *str)
{
	time_t t;
	struct tm *p;
	time(&t);
	p = gmtime(&t);
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year),
	        (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

static int char_to_wchar(int font_type, const char* orig_str, wchar_t *wtext, int max_length)
{
	switch (font_type) {
	case FONT_TYPE_ENGLISH:
		setlocale(LC_ALL,"");
		break;
	case FONT_TYPE_CHINESE:
		setlocale(LC_ALL,"zh_CN.utf8");
		break;
	default:
		setlocale(LC_ALL,"");
		break;
	}

	if (-1 == mbstowcs(wtext, orig_str, max_length - 1))
		return -1;
	if (wcscat(wtext, L"") == NULL)
		return -1;
	return 0;
}

static inline int is_valid_rotate(rotate_type_t rotate)
{
	switch (rotate) {
	case CLOCKWISE_0:
	case CLOCKWISE_90:
	case CLOCKWISE_180:
	case CLOCKWISE_270:
	case AUTO_ROTATE:
		return 1;
	default:
		return 0;
	}
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int stream_id = -1, area_id = -1; /* for initialization only */

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index))
	        != -1) {
		switch (ch) {
		case 'A':
			stream_id = 0;
			stream_text_info[stream_id].enable = 1;
			break;
		case 'B':
			stream_id = 1;
			stream_text_info[stream_id].enable = 1;
			break;
		case 'C':
			stream_id = 2;
			stream_text_info[stream_id].enable = 1;
			break;
		case 'D':
			stream_id = 3;
			stream_text_info[stream_id].enable = 1;
			break;
		case 'a':
			area_id = atoi(optarg);
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].enable = 1;
			break;
		case 'x':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].x = atoi(optarg);
			break;
		case 'y':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].y = atoi(optarg);
			break;
		case 'w':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].width = atoi(optarg);
			break;
		case 'h':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].height = atoi(optarg);
			break;
		case 't':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].font_type =
					atoi(optarg);
			break;
		case 's':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].font_size =
					atoi(optarg);
			break;
		case 'c':
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].color = atoi(optarg);
			break;
		case SPECIFY_OUTLINE:
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].outline = atoi(optarg);
			break;
		case SPECIFY_BOLD:
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].bold = atoi(optarg);
			break;
		case SPECIFY_ITALIC:
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].textbox[area_id].italic = atoi(optarg);
			break;
		case SPECIFY_ROTATE:
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			stream_text_info[stream_id].rotate = AUTO_ROTATE;
			break;
		case SPECIFY_STRING:
			VERIFY_STREAMID(stream_id);
			VERIFY_AREAID(area_id);
			if (char_to_wchar(stream_text_info[stream_id].textbox[area_id].font_type, optarg,
				stream_text_info[stream_id].textbox[area_id].content,
				sizeof(stream_text_info[stream_id].textbox[area_id].content)) < 0) {
				printf("Convert to wchar failed. Use default string.\n");
				memset(stream_text_info[stream_id].textbox[area_id].content, 0,
				       sizeof(stream_text_info[stream_id].textbox[area_id].content));
			}
			break;
		case TIME_DISPLAY:
			VERIFY_STREAMID(stream_id);
			VERIFY_STREAMID(area_id);
			stream_text_info[stream_id].textbox[area_id].time.enable = 1;
			flag_time_display = 1;
			break;
		default:
			printf("unknow option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

static int check_encode_status(void)
{
	int i;
	enum iav_state status;
	struct iav_stream_format format;
	u8 rotate_clockwise, vflip, hflip;

	/* IAV must be in ENOCDE or PREVIEW state */
	if (ioctl(fd_overlay, IAV_IOC_GET_IAV_STATE, &status) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}

	if ((status != IAV_STATE_PREVIEW) &&
			(status != IAV_STATE_ENCODING)) {
		printf("IAV must be in PREVIEW or ENCODE for text OSD.\n");
		return -1;
	}

	/* Check rotate status */
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_text_info[i].enable) {
			format.id = i;
			if (ioctl(fd_overlay, IAV_IOC_GET_STREAM_FORMAT, &format) < 0) {
				perror("IAV_IOC_GET_STREAM_FORMAT");
				return -1;
			}

			if (stream_text_info[i].rotate == AUTO_ROTATE) {
				rotate_clockwise = format.rotate_cw;
				hflip = format.hflip;
				vflip = format.vflip;

				stream_text_info[i].rotate = CLOCKWISE_0;
				stream_text_info[i].rotate |= (rotate_clockwise ?
						SET_BIT(ROTATE_BIT) : 0);
				stream_text_info[i].rotate |= (hflip ? SET_BIT(HFLIP_BIT) : 0);
				stream_text_info[i].rotate |= (vflip ? SET_BIT(VFLIP_BIT) : 0);
				if (!is_valid_rotate(stream_text_info[i].rotate)) {
					printf("!Warning: Stream %c Unknown rotate type. OSD is "
							"consistent with VIN orientation.\n", 'A' + i);
					stream_text_info[i].rotate = CLOCKWISE_0;
				}
			}

			if (stream_text_info[i].rotate & SET_BIT(ROTATE_BIT)) {
				stream_text_info[i].win_width = format.enc_win.height;
				stream_text_info[i].win_height = format.enc_win.width;
			} else {
				stream_text_info[i].win_width = format.enc_win.width;
				stream_text_info[i].win_height = format.enc_win.height;
			}
		}
	}
	return 0;
}

static int map_overlay(void)
{
	struct iav_querybuf querybuf;
	u8* addr = NULL;

	querybuf.buf = IAV_BUFFER_OVERLAY;
	if (ioctl(fd_overlay, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		printf("IAV_IOC_QUERY_BUF: OVERLAY\n");
		return -1;
	}

	addr = (u8*)mmap(NULL, querybuf.length,
							PROT_WRITE, MAP_SHARED,
							fd_overlay, querybuf.offset);
	if (addr == MAP_FAILED) {
		printf("mmap OVERLAY failed\n");
		return -1;
	}

	printf("\noverlay: start = %p, total size = 0x%x ( bytes)\n",addr, querybuf.length);
	overlay_clut_addr = addr + OVERLAY_CLUT_OFFSET;
	overlay_yuv_size = (querybuf.length - OVERLAY_YUV_OFFSET)/MAX_ENCODE_STREAM_NUM;

	return 0;
}

static int check_param(void)
{
	int i, j, win_width, win_height;
	textbox_t *box;
	char str[STRING_SIZE];
	static int default_fontsize = 16;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++){
		if (stream_text_info[i].enable) {
			win_width = stream_text_info[i].win_width;
			win_height = stream_text_info[i].win_height;
			for (j = 0; j < MAX_OVERLAY_AREA_NUM; j++) {
				box = &stream_text_info[i].textbox[j];
				if (box->enable) {
					sprintf(str, "Stream %c Textbox %d", 'A' + i, j);
					if (box->font_type < 0 || box->font_type >= FONT_TYPE_NUM)
						box->font_type = 0;
					if (box->font_size <= 0)
						box->font_size = default_fontsize;
					if (box->color < 0 || box->color >= COLOR_NUM)
						box->color = 0;
					if (box->italic < 0)
						box->italic = 0;
					if (box->outline < 0)
						box->outline = 0;
					if (box->x < 0)
						box->x = 0;
					if (box->y < 0)
						box->y = 0;

					/* Text box's width and height cannot be ZERO */
					if (box->width <= 0|| box->height <= 0) {
						printf("!!!Error: %s width[%d] or height[%d] should be "
								"positive.\n", str, box->width, box->height);
						return -1;
					}
					/* Text box's height should be big enough for alignment.*/
					if (box->height < LINE_SPACING(box->font_size)){
						printf("!!!Error: %s height [%d] is too small for font"
								" size [%d].\nOSD height >= line_space * font "
								"size. The default line_space is 1.5x.\n", str,
								box->height, box->font_size);
						return -1;
					}
					/* Text box cannot exceed the encode size */
					if (box->x + box->width > win_width ||
							box->y + box->height > win_height) {
						printf("!!!Error: %s [x %d, y %d, w %d, h %d ] is out of "
								"stream [width %d, height %d]\n", str, box->x,
								box->y, box->width, box->height, win_width,
								win_height);
						return -1;
					}

					if (box->time.enable) {
						if (box->italic != 0) {
							printf("!Warning: Italic font display uncorrect time."
									"Use non italic font to display time.\n");
							box->italic = 0;
						}
						get_time_string(box->time.time_string);
						char_to_wchar(box->font_type, box->time.time_string, box->content,
						              sizeof(box->content));
						memset(box->time.osd_x, NOT_CONVERTED, STRING_SIZE);
						memset(box->time.osd_y, NOT_CONVERTED, STRING_SIZE);
						box->time.digit_height = LINE_SPACING(box->font_size);
						box->time.digit_width = FIXED_DIGIT_WIDTH(box->font_size);
					}

					if (wcslen(box->content) == 0)
						wcscpy(box->content, wText[box->font_type]);

					printf("%s [x %d, y %d, w %d, h %d], font size %d, type %d,"
							" color %d, bold %d, italic %d, outline %d\n", str,
							box->x, box->y, box->width, box->height,
							box->font_size, box->font_type, box->color,
							box->bold, box->italic, box->outline);
				}
			}
		}
	}
	return 0;
}

static int fill_overlay_clut(void)
{
	clut_t * clut_data = (clut_t *)overlay_clut_addr;
	int i, j, outline_clut;

	for (i = 0; i < CLUT_OUTLINE_OFFSET && i < sizeof(clut); i++) {
		/* CLUT (0 ~ CLUT_OUTLINE_OFFSET) is for non outline font.*/
		for (j = 0; j < 256; j++) {
			clut_data[(i << 8) + j].y = clut[i].y;
			clut_data[(i << 8) + j].u = clut[i].u;
			clut_data[(i << 8) + j].v = clut[i].v;
			clut_data[(i << 8) + j].alpha = j;
		}

		/* CLUT (CLUT_OUTLINE_OFFSET ~ OVERLAY_CLUT_NUM) is outline font.*/
		outline_clut = i + CLUT_OUTLINE_OFFSET;
		for (j = 0; j < 256; j++) {
			clut_data[(outline_clut << 8) + j].y = clut[i].y;
			clut_data[(outline_clut << 8) + j].u = clut[i].u;
			clut_data[(outline_clut << 8) + j].v = clut[i].v;
			clut_data[(outline_clut << 8) + j].alpha = j;
		}
		clut_data[(outline_clut << 8) + pixel_type.pixel_outline].y = (
				clut[i].y > 128 ? 16 : 235);
		clut_data[(outline_clut << 8) + pixel_type.pixel_outline].u = 128;
		clut_data[(outline_clut << 8) + pixel_type.pixel_outline].v = 128;
		clut_data[(outline_clut << 8) + pixel_type.pixel_outline].alpha = 255;

	}

	return 0;
}

static int set_overlay_config(void)
{
	int i, j, win_width, win_height, total_size;
	u32 overlay_data_offset;
	struct iav_overlay_area *area;
	textbox_t *box;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (stream_text_info[i].enable) {
			overlay_data_offset = OVERLAY_YUV_OFFSET + overlay_yuv_size * i;
			win_width = stream_text_info[i].win_width;
			win_height = stream_text_info[i].win_height;
			for (j = 0, total_size = 0; j < MAX_OVERLAY_AREA_NUM; j++) {
				area = &overlay_insert[i].area[j];
				box = &stream_text_info[i].textbox[j];

				if (box->enable) {
					if (stream_text_info[i].rotate &
							SET_BIT(ROTATE_BIT)) {
						area->width = box->height = ROUND_UP(box->height, 32);
						area->height = box->width = ROUND_UP(box->width, 4);
					} else {
						area->width = box->width = ROUND_UP(box->width, 32);
						area->height = box->height = ROUND_UP(box->height, 4);
					}
					switch (stream_text_info[i].rotate) {
					case CLOCKWISE_0:
						area->start_x = box->x = ROUND_DOWN(box->x, 2);
						area->start_y = box->y = ROUND_DOWN(box->y, 4);
						break;
					case CLOCKWISE_90:
						area->start_x = box->y = ROUND_DOWN(box->y, 2);
						area->start_y = ROUND_DOWN(
								win_width - box->x - box->width, 4);
						box->x = win_width - box->width - area->start_y;
						break;
					case CLOCKWISE_180:
						area->start_x = ROUND_DOWN(
								win_width - box->x - box->width, 2);
						area->start_y = ROUND_DOWN(
								win_height - box->y - box->height, 4);
						box->x = win_width - box->width - area->start_x;
						box->y = win_height - box->height -  area->start_y;
						break;
					case CLOCKWISE_270:
						area->start_x = ROUND_DOWN(
								win_height - box->y - box->height, 2);
						area->start_y = box->x = ROUND_DOWN(box->x, 4);
						box->y = win_height - box->height -  area->start_x;
						break;
					default:
						printf("unknown rotate type\n");
						return -1;
					}

					area->pitch = area->width;
					area->enable = 1;
					area->total_size = area->pitch * area->height;
					area->clut_addr_offset = (box->outline == 0 ? box->color :
							box->color + CLUT_OUTLINE_OFFSET) * OVERLAY_CLUT_SIZE;
					area->data_addr_offset = overlay_data_offset + total_size;

					printf("Stream %c Area %d [x %d, y %d, w %d, h %d,"
							" size %d],clut_addr_offset:%d,data_addr_offset:%d\n", 'A' + i, j, area->start_x,
							area->start_y, area->width, area->height,
							area->total_size,area->clut_addr_offset,area->data_addr_offset);

					total_size += area->total_size;
					if (total_size > overlay_yuv_size) {
						printf("The total OSD size is %d (should be <= %d).\n",
								total_size, overlay_yuv_size);
						return -1;
					}
				}
			}
		}
	}
	return 0;
}

static int text_set_font(int stream_id, int area_id)
{
	font_attribute_t font;
	textbox_t *box = &stream_text_info[stream_id].textbox[area_id];

	strcpy(font.type, font_type[box->font_type]);
	font.size = box->font_size;
	font.outline_width = box->outline;
	font.hori_bold = font.vert_bold = box->bold;
	font.italic = box->italic;
	font.disable_anti_alias = 0;

	if (text2bitmap_set_font_attribute(&font) < 0)
		return -1;

	return 0;
}

/*
 * Fill the overlay data based on rotate type.
 */

static void fill_overlay_data(const buffer_info_t *src, buffer_info_t *dst,
                              rotate_type_t rotate)
{
	u8 *sp, *dp, *src_base, *dst_base;
	int col, row;
	const u16 width = src->sub_width, height = src->buf_height;
	const u16 src_width = src->buf_width, src_height = src->buf_height;
	const u16 dst_width = dst->buf_width, dst_height = dst->buf_height;
	const u16 sx = src->sub_x, sy = src->sub_y;
	const u16 dx = dst->sub_x, dy = dst->sub_y;
	src_base = src->buf_base;
	dst_base = dst->buf_base;

	if (sx + width > src_width || sy + height > src_height) {
		printf("fill_overlay_data error.\n");
		return;
	}

	sp = src_base + sy * src_width + sx;

	switch (rotate) {
	case CLOCKWISE_0:
		dp = dst_base + dy * dst_width + dx;
		for (row = 0; row < height; row++) {
			memcpy(dp, sp, width);
			sp += src_width;
			dp += dst_width;
		}
		break;
	case CLOCKWISE_90:
		dp = dst_base + (dst_height - dx - 1) * dst_width + dy;
		for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
				*(dp - col * dst_width) = *(sp + col);
			}
			sp += src_width;
			dp++;
		}
		break;
	case CLOCKWISE_180:
		dp = dst_base + (dst_height - dy - 1) * dst_width + dst_width - dx - 1;
		for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
				*(dp - col) = *(sp + col);
			}
			sp += src_width;
			dp -= dst_width;
		}
		break;
	case CLOCKWISE_270:
		dp = dst_base + dx * dst_width + dst_width - dy - 1;
		for (row = 0; row < height; row++) {
			for (col = 0; col < width; col++) {
				*(dp + col * dst_width) = *(sp + col);
			}
			sp += src_width;
			dp--;
		}
		break;
	default:
		printf("Unknown rotate type.\n");
		break;
	}

	if (rotate & SET_BIT(ROTATE_BIT)) {
		dst->sub_width = src->sub_height;
		dst->sub_height = src->sub_width;
	} else {
		dst->sub_width = src->sub_width;
		dst->sub_height = src->sub_height;
	}
}

static int digits_convert(int stream_id, int area_id)
{
	int i, offset_x, digits_size;
	wchar_t digits[TOTAL_DIGIT_NUM + 1] = L"0123456789";
	bitmap_info_t bitmap_info;
	u8* line_addr;
	wchar_t *p;
	textbox_t *box = &stream_text_info[stream_id].textbox[area_id];
	const u16 line_width = box->time.digit_width * TOTAL_DIGIT_NUM;
	const u16 line_height = box->time.digit_height;
	digits_size = line_width * line_height;

	if (NULL == (box->time.digits_addr = (u8 *)malloc(digits_size))) {
		perror("digits_convert: malloc\n");
		return -1;
	}

	line_addr = box->time.digits_addr;
	memset(line_addr, pixel_type.pixel_background, digits_size);

	offset_x = 0;
	for (i = 0, p = digits; i < TOTAL_DIGIT_NUM + 1; i++, p++) {
		if (text2bitmap_convert_character(*p, line_addr, line_height,
				line_width, offset_x, &bitmap_info) < 0)
			return -1;
		offset_x += box->time.digit_width;
		if (bitmap_info.width > box->time.max_width)
			box->time.max_width = bitmap_info.width;
	}
	return 0;
}

static int text_string_convert(int stream_id, int area_id)
{
	int i, remain_height, offset_x, offset_y;
	buffer_info_t boxbuf, overlay;
	u8 *line_addr, *boxbuf_addr;
	bitmap_info_t bitmap_info;
	wchar_t *p;
	struct iav_overlay_area * area = &overlay_insert[stream_id].area[area_id];
	textbox_t *box = &stream_text_info[stream_id].textbox[area_id];
	const u16 line_width = box->width;
	const u16 line_height = LINE_SPACING(box->font_size);

	if (NULL == (boxbuf_addr = (u8 *)malloc(area->total_size))) {
		perror("text_string_convert: malloc\n");
		return -1;
	}
	memset(boxbuf_addr, pixel_type.pixel_background, area->total_size);

	offset_x = offset_y = 0;
	line_addr = boxbuf_addr;

	for (i = 0, p = box->content;i < wcslen(box->content); i++, p++) {
		if (offset_x + box->font_size > line_width) {
			/*
			 * If there is enough memory for more lines, try to write on a new
			 * line. Otherwise, stop converting.
			 */
			offset_y += line_height;
			remain_height = box->height - offset_y;
			if (remain_height < line_height)
				break;
			line_addr += line_width * line_height;
			offset_x = 0;
		}

		/* Remember the offset of letters in time string */
		if (box->time.enable) {
			box->time.osd_x[i] = offset_x;
			box->time.osd_y[i] = offset_y;
		}
		if (text2bitmap_convert_character(*p, line_addr, line_height,
				line_width, offset_x, &bitmap_info) < 0) {
			free(boxbuf_addr);
			return -1;
		}
		if (box->time.enable && isdigit(*p)) {
			/*
			 * If the character is a digit in time string, use the max digit
			 * bitmap width as horizontal offset.
			 */
			offset_x += box->time.max_width;
		} else {
			offset_x += bitmap_info.width;
		}
	}

	overlay.buf_base = overlay_clut_addr + area->data_addr_offset;
	overlay.buf_height = area->height;
	overlay.buf_width = area->width;
	overlay.sub_x = overlay.sub_y = 0;
	boxbuf.buf_base = boxbuf_addr;
	boxbuf.buf_height = boxbuf.sub_height = box->height;
	boxbuf.buf_width = boxbuf.sub_width = box->width;
	boxbuf.sub_x = boxbuf.sub_y = 0;
	fill_overlay_data(&boxbuf, &overlay, stream_text_info[stream_id].rotate);
	free(boxbuf_addr);
	return 0;
}

static int text_osd_insert(int stream_id)
{
	int area_id;

	for (area_id = 0; area_id  < MAX_OVERLAY_AREA_NUM; area_id ++) {
		if (stream_text_info[stream_id].textbox[area_id].enable) {
			// Set font attribute
			if (text_set_font(stream_id, area_id) < 0)
				return -1;
			// Convert 10 digits for later reuse.
			if (stream_text_info[stream_id].textbox[area_id].time.enable) {
				if (digits_convert(stream_id, area_id) < 0)
					return -1;
			}
			// Convert string and fill the overlay data.
			if (text_string_convert(stream_id, area_id) < 0)
				return -1;
		}
	}

	overlay_insert[stream_id].enable = 1;
	overlay_insert[stream_id].id = stream_id;

	if (ioctl(fd_overlay, IAV_IOC_SET_OVERLAY_INSERT,
			&overlay_insert[stream_id]) < 0) {
		perror("IAV_IOC_SET_OVERLAY_INSERT");
		return -1;
	}

	return 0;
}

static void time_callback()
{
	int i, j, c, k;
	buffer_info_t digits, overlay;
	char current_time[STRING_SIZE];
	time_display_t *osd_time;
	struct iav_overlay_area *area;


	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (!stream_text_info[i].enable)
			continue;
		for (j = 0; j < MAX_OVERLAY_AREA_NUM; j++) {
			area = &overlay_insert[i].area[j];
			osd_time = &stream_text_info[i].textbox[j].time;
			if (area->enable && osd_time->enable) {
				digits.buf_width = osd_time->digit_width * TOTAL_DIGIT_NUM;
				digits.buf_height = digits.sub_height = osd_time->digit_height;
				digits.sub_width = osd_time->max_width;
				digits.buf_base = osd_time->digits_addr;
				digits.sub_y = 0;

				overlay.buf_width = area->width;
				overlay.buf_height = area->height;
				overlay.buf_base = overlay_clut_addr + area->data_addr_offset;
				get_time_string(current_time);
				k = 0;
				while (current_time[k] != '\0' &&
						osd_time->osd_x[k] != NOT_CONVERTED &&
						osd_time->osd_x[k] != NOT_CONVERTED) {
					if (current_time[k] != osd_time->time_string[k]) {
						c = current_time[k] - '0';
						digits.sub_x = c * osd_time->digit_width;
						overlay.sub_x = osd_time->osd_x[k];
						overlay.sub_y = osd_time->osd_y[k];
						fill_overlay_data(&digits, &overlay,
						                  stream_text_info[i].rotate);
					}
					k++;
				}
				strcpy(osd_time->time_string, current_time);
			}
		}
	}
}

static void exit_textinsert()
{
	int i, j;
	if (flag_time_display) {
		struct itimerval st_new = { {0, 0}, {1, 0} };
		struct itimerval st_old;
		setitimer(ITIMER_REAL, &st_new, &st_old);
	}
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (overlay_insert[i].enable) {
			overlay_insert[i].enable = 0;
			overlay_insert[i].id = i;
			if (ioctl(fd_overlay, IAV_IOC_SET_OVERLAY_INSERT,
					&overlay_insert[i]) < 0) {
				perror("IAV_IOC_SET_OVERLAY_INSERT");
			}
			for (j = 0; j < MAX_OVERLAY_AREA_NUM; j++) {
				if (stream_text_info[i].textbox[j].time.digits_addr != NULL)
					free(stream_text_info[i].textbox[j].time.digits_addr);
			}
		}
	}

	text2bitmap_lib_deinit();
	exit(0);
}

int main(int argc, char **argv)
{
	int i;
	struct itimerval st_new = { {1, 0}, {1, 0} }, st_old;

	signal(SIGINT, exit_textinsert);
	signal(SIGQUIT, exit_textinsert);
	signal(SIGTERM, exit_textinsert);

	if (argc < 2) {
		usage();
		return 0;
	}

	if ((fd_overlay = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (check_encode_status() < 0)
		return -1;

	if (check_param() < 0)
		return -1;

	if (map_overlay() < 0)
		return -1;

	if (set_overlay_config() < 0)
		return -1;

	if (fill_overlay_clut() < 0)
		return -1;

	if (text2bitmap_lib_init(&pixel_type) < 0)
		return -1;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (stream_text_info[i].enable) {
			if (text_osd_insert(i) < 0) {
				printf("Text insert for stream %d error!\n", i);
				goto TEXTINSERT_EXIT;
			}
		}
	}

	if (flag_time_display) {
		signal(SIGALRM, time_callback);
		setitimer(ITIMER_REAL, &st_new, &st_old);
	}

	while (1)
		sleep(10);

TEXTINSERT_EXIT:
	exit_textinsert();
	return 0;
}
