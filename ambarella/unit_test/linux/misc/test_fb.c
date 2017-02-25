/*
 * test_fb.c
 *
 * History:
 *	2009/10/13 - [Jian Tang] created file
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
#include <unistd.h>
#include <malloc.h>
#include <getopt.h>
#include <ctype.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <basetypes.h>

#define	NO_ARG		0
#define	HAS_ARG		1


static struct option long_options[] = {
	{"rgb",	HAS_ARG,	0,	'c'},
	{"height",	HAS_ARG,	0,	'h'},
	{"width",		HAS_ARG,	0,	'w'},
	{"id",		HAS_ARG,	0,	'b'},

	{0, 0, 0, 0}
};

static const char *short_options = "c:h:w:b:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"",			"\t\tset color(Format:R, G, B, alpha), max value is 255"},
	{"",			"\t\tset frame buffer height"},
	{"",			"\t\tset frame buffer width"},
	{"",			"\t\tset frame buffer id"},
};

struct fb_cmap_user {
	unsigned int start;			/* First entry	*/
	unsigned int len;			/* Number of entries */
	u16 *r;		/* Red values	*/
	u16 *g;
	u16 *b;
	u16 *transp;		/* transparency, can be NULL */
};

typedef struct fb_color_s {
	u16 r;
	u16 g;
	u16 b;
	u16 alpha;
} fb_color_t;

static int fb_width = 0;
static int fb_width_flag = 0;
static int fb_height = 0;
static int fb_height_flag = 0;
static int fb_id = 0;
static int fb_id_flag = 0;

#define MAX_COLOR_VALUE	255
enum {
	COLOR_R = 0,
	COLOR_G,
	COLOR_B,
	COLOR_ALPHA,
	COLOR_RGB_TOTAL_TYPE,
} COLOR_TYPE;

static fb_color_t fb_color = {255, 255, 255, 64};

void usage(void)
{
	int i;

	printf("test_fb usage:\n");
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
	printf("Example:\n Red:\t test_fb -c 255,0,0,128 \n\n");
}

/***************************************************************************************
	function:	int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
	input:	input_str: input string line for analysis
			argarr: buffer array accommodating the parsed arguments
			argcnt: number of the arguments parsed
	return value:	0: successful, -1: failed
	remarks:
***************************************************************************************/
static int get_multi_arg(char *input_str, u16 *argarr, int *argcnt)
{
	int i = 0;
	char *delim = " ,";
	char *ptr;
	*argcnt = 0;

	ptr = strtok(input_str, delim);
	if (ptr == NULL)
		return 0;
	argarr[i++] = atoi(ptr);

	while (1) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i++] = atoi(ptr);
	}

	*argcnt = i;
	return 0;
}

static int input_color(char *input)
{
	int num;
	u16 color[COLOR_RGB_TOTAL_TYPE];

	if (input == NULL) {
		return -1;
	}

	get_multi_arg(input, color, &num);


	if (num != COLOR_RGB_TOTAL_TYPE) {
		return -1;
	}

	for (num = 0; num < COLOR_RGB_TOTAL_TYPE; num++) {
		if (color[num] > MAX_COLOR_VALUE) {
			return -1;
		}
	}

	memcpy(&fb_color, &color, sizeof(fb_color_t));
	return 0;
}

int init_param(int argc, char ** argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
		&option_index)) != -1) {
		switch (ch) {
		case 'c':
			if (input_color(optarg) < 0) {
				printf("Input error\n");
				return -1;
			}
			break;
		case 'h':
			fb_height = atoi(optarg);
			fb_height_flag = 1;
			break;
		case 'w':
			fb_width = atoi(optarg);
			fb_width_flag = 1;
			break;
		case 'b':
			fb_id = atoi(optarg);
			if ((2 > fb_id) && (0 <= fb_id)) {
				fb_id_flag = 1;
			} else {
				printf("frame buffer id should in range [0, 1]\n");
				fb_id = 0;
				fb_id_flag = 0;
			}
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int				fb;
	struct fb_var_screeninfo	var;
	struct fb_fix_screeninfo	fix;
	unsigned char			*buf;
	struct fb_cmap_user	cmap;
	int color, width, height;
	unsigned short r_table[256];
	unsigned short g_table[256];
	unsigned short b_table[256];
	unsigned short trans_table[256];
	int i;

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (!fb_id_flag) {
		fb = open("/dev/fb0", O_RDWR);
		if (fb < 0) {
			printf("open /dev/fb0 fail\n");
			perror("Open fb error");
			return -1;
		}
		printf("open default /dev/fb0 done\n");
	} else {
		char fb_dev_string[32] = {0};
		snprintf(fb_dev_string, sizeof(fb_dev_string), "/dev/fb%d", fb_id);
		fb = open(fb_dev_string, O_RDWR);
		if (fb < 0) {
			printf("open /dev/fb%d fail\n", fb_id);
			perror("Open fb error");
			return -1;
		}
		printf("open %s done\n", fb_dev_string);
	}

	if(ioctl(fb, FBIOGET_VSCREENINFO, &var) >= 0
		&& ioctl(fb, FBIOGET_FSCREENINFO, &fix) >= 0) {
		buf = (unsigned char *)mmap(NULL, fix.smem_len, PROT_WRITE,  MAP_SHARED, fb, 0);
		printf("FB: %d x %d @ %dbpp\n", var.xres, var.yres, var.bits_per_pixel);
	} else {
		printf("Unable to get var and fix info!\n");
		return -1;
	}

	printf("Set color map ...\n");
	cmap.start = 0;
	cmap.len = 256;

	for (i = 0; i < cmap.len - cmap.start; i++) {
		r_table[i] = fb_color.r;
		g_table[i] = fb_color.g;
		b_table[i] = fb_color.b;
		trans_table[i] = i;
	}

	cmap.r = r_table;
	cmap.g = g_table;
	cmap.b = b_table;
	cmap.transp = trans_table;
	if (ioctl(fb, FBIOPUTCMAP, &cmap) < 0) {
		printf("Unable to put cmap!\n");
		return -1;
	}

	printf("Initializing frame buffer ...\n");
	ioctl(fb, FBIOPAN_DISPLAY, &var);

	printf("Setting frame buffer size ...\n");
	width =  fb_width_flag ? fb_width : var.xres;
	height = fb_height_flag ? fb_height : var.yres;
	var.xres = width;
	var.yres = height;
	printf("xres = %d, yres = %d, line = %d\n", var.xres, var.yres, fix.line_length);
	ioctl(fb, FBIOPUT_VSCREENINFO, &var);

	printf("Filling ...\n");
	color = fb_color.alpha;
	memset(buf, color, var.yres * fix.line_length);
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	sleep(5);

	printf("Blanking ...\n");
	memset(buf, 0, var.yres * fix.line_length);
	ioctl(fb, FBIOBLANK, FB_BLANK_NORMAL);
	ioctl(fb, FBIOPAN_DISPLAY, &var);

	munmap(buf, fb);
	close(fb);

	return 0;
}

