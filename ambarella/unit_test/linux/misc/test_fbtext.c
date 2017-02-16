/*
 * test_fbtext.c
 *
 * History:
 *	2009/10/13 - [Jian Tang] created file.
 *	2011/06/10 - [Qian Shen] modified file.
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>
#include <wchar.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <basetypes.h>
#include "text_insert.h"

#ifndef LINE_SPACING
#define LINE_SPACING(x)		((x) * 3 / 2)
#endif

#define FIXED_DIGIT_WIDTH	(2)

typedef struct fb_clut_s {
	u16 r;
	u16 g;
	u16 b;
	u16 alpha;
} fb_clut_t;

struct fb_cmap_user {
	u32 start; /* First entry	*/
	u32 len; /* Number of entries */
	u16 *r;
	u16 *g;
	u16 *b;
	u16 *transparent;		/* transparency, can be NULL */
};

pixel_type_t pixel_type = {
		.pixel_background = 0,
		.pixel_outline = 1,
		.pixel_font = 255,
};

static fb_clut_t default_clut_data[6] = { /* alpha 0 is full transparent */
		{ .r = 255, .g = 0, .b = 0, .alpha = 255 }, /* red */
		{ .r = 255, .g = 255, .b = 255, .alpha = 255 }, /* white */
		{ .r = 0, .g = 0, .b = 255, .alpha = 255 }, /* blue */
		{ .r = 0, .g = 255, .b = 0, .alpha = 255 }, /* green */
		{ .r = 255, .g = 255, .b = 0, .alpha = 255 }, /* yellow */
		{ .r = 255, .g = 0, .b = 255, .alpha = 255 },/* cyan */
};

struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;

static int fb;
static font_attribute_t font;
static wchar_t wstring[MAX_BYTE_SIZE];
static int fb_color_id = 0;
static int fb_x = 0;
static int fb_y = 0;
static int rect_width = 0;
static int rect_height = 0;
static int flag_string = 0;
u8 *fb_mem = NULL;

/* for time display */
static int flag_time_display = 0;
u8 *digit_buf = NULL;
static int digit_width = 0;
static int time_offset[32][2];
char last_time[MAX_BYTE_SIZE];

static u16 r_table[256];
static u16 g_table[256];
static u16 b_table[256];
static u16 trans_table[256];

wchar_t default_wtext[2][MAX_BYTE_SIZE] = {
	L"AmbarellaA5sSDK",
	L"安霸中国(Chinese)",
};

wchar_t *digits = L"0123456789";
char font_type[2][MAX_BYTE_SIZE] = {
	"/usr/share/fonts/DroidSans.ttf",
	"/usr/share/fonts/DroidSansFallbackFull.ttf",
};


#define	NO_ARG		0
#define	HAS_ARG		1

enum {
	SPECIFY_BOLD,
	SPECIFY_ITALIC,
	SPECIFY_STRING,
	TIME_DISPLAY,
};

static const char *short_options = "c:x:y:w:h:t:s:";
static struct option long_options[] = {
		{ "color", HAS_ARG, 0, 'c'},
		{ "offset_x", HAS_ARG, 0, 'x'},
		{ "offset_y", HAS_ARG, 0, 'y'},
		{ "string", HAS_ARG, 0, SPECIFY_STRING},
		{ "fonttype", HAS_ARG, 0, 't' },
		{ "fontsize", HAS_ARG, 0, 's' },
		{ "bold", HAS_ARG, 0, SPECIFY_BOLD },
		{ "italic", HAS_ARG, 0, SPECIFY_ITALIC },
		{ "rect_w", HAS_ARG, 0, 'w'},
		{ "rect_h", HAS_ARG, 0, 'h'},
		{ "time", NO_ARG, 0, TIME_DISPLAY },

		{0, 0, 0, 0}
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{ "0~6", "\tset color. 0 red, 1 white, 2 blue, 3 green, 4 yellow, 5 cyan"},
	{ "", "\t\tset display horizontal offset"},
	{ "", "\t\tset display vertical offset"},
	{ "", "\t\ttext to be displayed" },
	{ "0~1", "\tset font type, 0 DroidSans, 1 DroidSansFallbackFull" },
	{ "", "\t\tset font size " },
	{ "0~100", "\tn percentage bold of font size. Negative means thinner." },
	{ "0~100", "\tn percentage italic of font size." },
	{ "", "\t\tdraw a rectangle. Set width."},
	{ "", "\t\tdraw a rectangle. Set height."},
	{ "","\t\tdisplay current time." },
};

static void usage(void)
{
	int i;

	printf("test_fbtext usage:\n");
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
	printf("Example:\n"
		"  To display text\n"
		"    test_fbtext -x100 -y100 -c2 -t1 -s50 --string \"Hello World\"\n"
		"  To draw rectangle\n"
		"    test_fbtext -x200 -y50 -w128 -h128 -c4\n"
		"  To display time\n"
		"    test_fbtext -x450 -y50 -t1 -c1 -s20 --time\n");
}

static void get_time_string(char *str)
{
	time_t t;
	struct tm *p;
	time(&t);
	p = gmtime(&t);
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1
	        + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

static int char_to_wchar(const char* orig_str, wchar_t *wtext)
{
	int str_length = strlen(orig_str);
	if (str_length != mbstowcs(wtext, orig_str, str_length + 1))
		return -1;

	if (wcscat(wtext, L"") == NULL)
		return -1;

	return 0;
}

static int init_param(int argc, char ** argv)
{
	int ch, tmp, type = 0;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
			case 'c':
				fb_color_id = atoi(optarg);
				break;
			case 'x':
				tmp = atoi(optarg);
				fb_x = tmp > 0 ? tmp : 0;
				break;
			case 'y':
				tmp = atoi(optarg);
				fb_y = tmp > 0 ? tmp : 0;
				break;
			case 'w':
				rect_width = atoi(optarg);
				flag_string = 0;
				break;
			case 'h':
				rect_height = atoi(optarg);
				flag_string = 0;
				break;
			case SPECIFY_STRING:
				if (!flag_time_display && char_to_wchar(optarg, &wstring[0]) < 0) {
					memset(&wstring[0], 0, MAX_BYTE_SIZE);
				}
				flag_string = 1;
				break;
			case 't':
				type = atoi(optarg);
				strcpy(font.type, font_type[type]);
				flag_string = 1;
				break;
			case 's':
				font.size = atoi(optarg);
				flag_string = 1;
				break;
				break;
			case SPECIFY_BOLD:
				font.hori_bold = font.vert_bold = atoi(optarg);
				flag_string = 1;
				break;
			case SPECIFY_ITALIC:
				font.italic = atoi(optarg);
				flag_string = 1;
				break;
			case TIME_DISPLAY:
				get_time_string(&last_time[0]);
				char_to_wchar(&last_time[0], &wstring[0]);
				flag_time_display = 1;
				flag_string = 1;
				break;
			default:
				printf("unknown option found: %c\n", ch);
				return -1;
		}
	}

	if (flag_string && !font.size) {
		font.size = 32;
		printf("Use default font size %d\n", font.size);
	}
	if (flag_string && (wcslen(&wstring[0]) == 0))
		wcscpy(&wstring[0],  &default_wtext[type][0]);

	return 0;
}
static int fb_put_cmap(int color)
{
	u16 i;
	struct fb_cmap_user cmap;
	cmap.start = 0;
	cmap.len = 256;

	for (i = 0; i < cmap.len - cmap.start; i++) {
		r_table[i] = default_clut_data[color].r;
		g_table[i] = default_clut_data[color].g;
		b_table[i] = default_clut_data[color].b;
		trans_table[i] = i;
	}

	cmap.r = r_table;
	cmap.g = g_table;
	cmap.b = b_table;
	cmap.transparent = trans_table;
	if (ioctl(fb, FBIOPUTCMAP, &cmap) < 0) {
		printf("Unable to put cmap!\n");
		return -1;
	}
	return 0;
}

static int fb_init(void)
{
	if (ioctl(fb, FBIOGET_VSCREENINFO, &var) >= 0) {
		var.xres = var.xres_virtual;
		var.yres = var.yres_virtual;
		ioctl(fb, FBIOPUT_VSCREENINFO, &var);
		printf("\tFB: %d x %d @ %dbpp\n", var.xres, var.yres, var.bits_per_pixel);
	} else {
		printf("\tUnable to get var and fix info!\n");
		return -1;
	}

	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix) >= 0)
		fb_mem = (u8 *) mmap(NULL, fix.smem_len, PROT_WRITE, MAP_SHARED, fb, 0);
	else {
		printf("Unable to get screen_fix_info\n");
		return -1;
	}

	fb_x = fb_x < var.xres ? fb_x : 0;
	fb_y = fb_y < var.yres ? fb_y : 0;

	return 0;
}

static int fb_rectangle_display(void)
{
	int i;
	rect_width = (rect_width <= 0 || rect_width > var.xres - fb_x ?
			var.xres - fb_x : rect_width);
	rect_height = (rect_height <= 0 || rect_height > var.yres - fb_y ?
			var.yres - fb_y : rect_height);
	printf("\tRectangle [x %d, y %d, width %d, height %d]\n",
	       fb_x, fb_y, rect_width, rect_height);
	for (i = 0; i < rect_height; i++)
		memset(fb_mem + (i + fb_y) * fix.line_length + fb_x,
				pixel_type.pixel_font, rect_width);
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	return 0;
}

static int fb_text_display(void)
{
	int i, remain_y, offset_x, offset_y;
	u8 *line_addr = fb_mem + fb_y * fix.line_length;
	u16 line_height = LINE_SPACING(font.size);
	bitmap_info_t bmp = {0, 0};
	wchar_t *p = &wstring[0];
	font.disable_anti_alias = 1;

	if (text2bitmap_lib_init(&pixel_type) < 0)
		return -1;

	if (text2bitmap_set_font_attribute(&font) < 0)
		return -1;

	memset(fb_mem, pixel_type.pixel_background, fix.smem_len);
	offset_x = fb_x;
	offset_y = fb_y;
	for (i = 0; i < wcslen(&wstring[0]); i++) {
		printf("%c", *p);
		if (offset_x + font.size > var.xres) {
			offset_y += line_height;
			remain_y = var.yres - offset_y;
			if (remain_y < LINE_SPACING(font.size))
				return 0;
			line_addr += fix.line_length * line_height;
			offset_x = fb_x;
		}
		if (text2bitmap_convert_character(*p, line_addr, line_height,
				fix.line_length, offset_x, &bmp) < 0)
			return -1;
		offset_x += bmp.width;
		p++;
	}
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	printf("\n");
	return 0;
}

static int fb_time_display()
{
	int i, remain_y, offset_x, offset_y;
	u8 *line_addr = fb_mem + fb_y * fix.line_length;
	u16 total_width = 10 * FIXED_DIGIT_WIDTH * font.size;
	u16 line_height = LINE_SPACING(font.size);
	bitmap_info_t bmp = { 0, 0 };
	wchar_t digits[32] = L"0123456789";
	wchar_t *p;
	font.disable_anti_alias = 1;

	digit_buf = (u8 *) malloc(total_width * line_height);
	if (digit_buf == NULL) {
		printf("malloc error!\n");
		return -1;
	}

	if (text2bitmap_lib_init(&pixel_type) < 0)
		return -1;

	if (text2bitmap_set_font_attribute(&font) < 0)
		return -1;

	/* Store 10 digits' bitmap data in (u8 *)digit_buf for later reuse. */
	p = &digits[0];
	offset_x = 0;
	memset(digit_buf, pixel_type.pixel_background, total_width * line_height);
	for (i = 0; i < wcslen(&digits[0]); i++) {
		if (text2bitmap_convert_character(*p, digit_buf, line_height,
				total_width, offset_x, &bmp) < 0)
			return -1;

		offset_x += FIXED_DIGIT_WIDTH * font.size;
		if (bmp.width > digit_width)
			digit_width = bmp.width;
		p++;
	}

	/* Write the time in the frame buffer */
	memset(fb_mem, pixel_type.pixel_background, fix.smem_len);
	offset_x = fb_x;
	offset_y = fb_y;
	p = &wstring[0];
	for (i = 0; i < wcslen(&wstring[0]); i++) {
		printf("%c", *p);
		if (offset_x + font.size > var.xres) {
			remain_y = var.yres - offset_y;
			if (remain_y < LINE_SPACING(font.size)) {
				time_offset[i][0] = -1;
				printf("\n");
				return 0;
			}
			offset_y += line_height;
			line_addr += fix.line_length * line_height;
			offset_x = fb_x;
		}
		time_offset[i][0] = offset_x;
		time_offset[i][1] = offset_y;
		if (text2bitmap_convert_character(*p, line_addr, line_height,
				fix.line_length, offset_x, &bmp)
		        < 0)
			return -1;
		if (isdigit(*p))
			offset_x += digit_width;
		else
			offset_x += bmp.width;
		p++;
	}
	ioctl(fb, FBIOPAN_DISPLAY, &var);

	time_offset[i][0] = -1;
	printf("\n");
	return 0;
}

static void time_callback()
{
	int i, j;
	char current_time[255], c;
	u8 *src = NULL, *des = NULL;
	int line_height = LINE_SPACING(font.size);

	get_time_string(&current_time[0]);
	if (!flag_time_display)
		return;
	i = 0;
	while(current_time[i] != '\0') {
		if (time_offset[i][0] == -1)
			break;
		if (current_time[i] == last_time[i]) {
			i++;
			continue;
		}
		/* Update the changed digits */
		c = current_time[i] - '0';
		src = digit_buf + c * FIXED_DIGIT_WIDTH * font.size;
		des = fb_mem + time_offset[i][1] * fix.line_length + time_offset[i][0];
		for (j = 0; j < line_height; j++)
			memcpy(des + j * fix.line_length, src + j * FIXED_DIGIT_WIDTH
				* font.size * 10, digit_width);
		i++;
	}
	strcpy(last_time, current_time);
	ioctl(fb, FBIOPAN_DISPLAY, &var);

}

static void fb_exit()
{
	if (flag_time_display) {
		struct itimerval st_new = { { 0, 0 }, { 1, 0 } };
		struct itimerval st_old;
		setitimer(ITIMER_REAL, &st_new, &st_old);
		if (digit_buf != NULL) {
			free(digit_buf);
			digit_buf = NULL;
		}
	}
	if (flag_string)
		text2bitmap_lib_deinit();

	if (fb_mem != NULL) {
		memset(fb_mem, 0, fix.smem_len);
		ioctl(fb, FBIOBLANK, FB_BLANK_NORMAL);
		ioctl(fb, FBIOPAN_DISPLAY, &var);
		munmap(fb_mem, fb);
		fb_mem = NULL;
	}
	close(fb);
	exit(0);
}

int main(int argc, char **argv)
{
	signal(SIGINT, fb_exit);
	signal(SIGQUIT, fb_exit);
	signal(SIGTERM, fb_exit);

	if (argc < 2) {
		usage();
		return 0;
	}

	fb = open("/dev/fb0", O_RDWR);
	if (fb < 0) {
		perror("Open fb error");
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	printf("Set color map ...\n");
	if (fb_put_cmap(fb_color_id) < 0)
		return -1;

	printf("Initialize frame buffer ...\n");
	if (fb_init() < 0)
		return -1;

	if (!flag_time_display) {
		if (flag_string) {
			printf("Display text ...\n\t");
			if (fb_text_display() < 0)
				goto EXIT;
		} else {
			printf("Display rectangle ...\n");
			fb_rectangle_display();
		}
	}

	if (flag_time_display) {
		struct itimerval st_new = { { 1, 0 }, { 1, 0 } };
		struct itimerval st_old;
		printf("Display time ...\n\t");
		if (fb_time_display() < 0)
			goto EXIT;

		signal(SIGALRM, time_callback);
		setitimer(ITIMER_REAL, &st_new, &st_old);
	}

	while (1)
		sleep(10);

EXIT:
	fb_exit();
	return 0;
}


