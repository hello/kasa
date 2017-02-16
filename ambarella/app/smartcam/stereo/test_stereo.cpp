/*
 *
 * History:
 *    2015/07/21 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "cv.h"
#include "highgui.h"
#include <getopt.h>
#include "stereo_lib.h"
#include "fb.h"

using namespace cv;

#define STEREO_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}

struct option long_options[] = {
	{"debug",		0,	0,	'd'},
	{"xml",			1,	0,	'x'},
	{"mind",		1,	0,	'm'},
	{"maxd",		1,	0,	'M'},
	{0,				0,	0,	 0 },
};

const char *short_options = "dx:m:M:";

int						g_debug		= 0;
char					g_xml[128]	= "/usr/local/bin/map.xml.gz";
int						g_mind		= 1;
int						g_maxd		= 32;

int parse_parameters(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'd':
			g_debug = 1;
			break;

		case 'x':
			strcpy(g_xml, optarg);
			break;

		case 'm':
			g_mind = atoi(optarg);
			break;

		case 'M':
			g_maxd = atoi(optarg);
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			break;

		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int						ret;
	stereo_config_t			cfg;
	stereo_addr_t			addr;

	int						i;
	struct timeval			tv_begin, tv_end;
	int						seconds;
	Mat						both, gb, left, right, disp;
	VideoCapture			cap;
	u8						*p, *b;

	cap.open(0);
	if (!cap.isOpened()) {
		printf("ERROR: Unable to open camera 0!\n");
		return -1;
	}

	left.create(240, 320, CV_8U);
	right.create(240, 320, CV_8U);
	disp.create(240, 320, CV_8U);

	ret = parse_parameters(argc, argv);
	STEREO_CHECK_ERROR();

	cfg.width	= 320;
	cfg.height	= 240;
	cfg.mind	= g_mind;
	cfg.maxd	= g_maxd;
	strcpy(cfg.xml, g_xml);
	cfg.debug	= g_debug;
	ret = stereo_start(&cfg);

	if (g_debug) {
		ret = open_fb(320, 240);
		STEREO_CHECK_ERROR();

		ret = blank_fb();
		STEREO_CHECK_ERROR();
	}

	while (1) {
		if (g_debug) {
			gettimeofday(&tv_begin, NULL);
		}

		cap >> both;
		if (both.empty()) {
			break;
		}
		cvtColor(both, gb, COLOR_BGR2GRAY);
		resize(gb, gb, cvSize(640, 240));
		p = gb.data;
		b = left.data;
		for (i = 0; i < 240; i++) {
			memcpy(b, p, 320);
			p += 640;
			b += 320;
		}
		p = gb.data + 320;
		b = right.data;
		for (i = 0; i < 240; i++) {
			memcpy(b, p, 320);
			p += 640;
			b += 320;
		}

		if (g_debug) {
			gettimeofday(&tv_end, NULL);
			seconds = 1000 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000;
			printf("%-24s: %6d ms\n", "Read Frame", seconds);
		}

		addr.L = left.data;
		addr.R = right.data;
		addr.D = disp.data;
		ret = stereo_frame(&addr);
		STEREO_CHECK_ERROR();

		if (g_debug) {
			ret = render_frame((char *)disp.data);
			STEREO_CHECK_ERROR();
		}
	}

	ret = stereo_stop();
	STEREO_CHECK_ERROR();

	if (g_debug) {
		ret = close_fb();
		STEREO_CHECK_ERROR();
	}

	return ret;
}
