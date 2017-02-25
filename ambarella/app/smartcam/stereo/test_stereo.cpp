/*
 *
 * History:
 *    2015/07/21 - [Zhenwu Xue] Create
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <string.h>
#include <sys/time.h>
#include "cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"
#include <getopt.h>
#include "stereo_lib.h"
#include "iav.h"
#include "fb.h"

using namespace cv;

#define	STEREO_WIDTH		480
#define	STEREO_HEIGHT		270

//#define	STEREO_VERTICALLY

#define	DIFF(x, y)			(((x) > (y)) ? ((x) - (y)) : ((y) - (x)))

#define STEREO_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}

struct option long_options[] = {
	{"debug",		0,	0,	'd'},
	{"distance",	0,	0,	'D'},
	{"calib",		1,	0,	'c'},
	{"mind",		1,	0,	'm'},
	{"maxd",		1,	0,	'M'},
	{"save",		1,	0,	's'},
	{0,				0,	0,	 0 },
};

const char *short_options = "dDc:m:M:s:";

int						g_debug		= 0;
int						g_distance	= 0;
char					g_calib[128]= "/usr/local/bin/calib.dat";
int						g_mind		= 1;
int						g_maxd		= 96;
char					g_save[64]	= "/sdcard/stereo/snapshot";
int						g_snapshot	= 0;

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

		case 'D':
			g_distance = 1;
			break;

		case 'c':
			strcpy(g_calib, optarg);
			break;

		case 'm':
			g_mind = atoi(optarg);
			break;

		case 'M':
			g_maxd = atoi(optarg);
			break;

		case 's':
			strcpy(g_save, optarg);
			g_snapshot = 1;
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
	void					*stereo;
	stereo_config_t			cfg;
	stereo_addr_t			addr;

	int						i, fid;
	int						x, y;
	unsigned int			W, H, P;
	struct timeval			tv_begin, tv_end;
	int						seconds;
	Mat						left, right, disp, fb;
	char					*f;
	char					text[64];
	u8						*p, *q;

	ret = init_iav(IAV_BUF_MAIN, IAV_BUF_ME1);
	STEREO_CHECK_ERROR();

	ret = get_iav_buf_size(&P, &W, &H);
	STEREO_CHECK_ERROR();

#ifndef STEREO_VERTICALLY
	if (W != 2 * STEREO_WIDTH || H != STEREO_HEIGHT) {
#else
	if (W != STEREO_WIDTH || H != 2 * STEREO_HEIGHT) {
#endif
		printf("W = %d, H = %d\n", W, H);
		ret = -1;
	}
	STEREO_CHECK_ERROR();

	left.create(STEREO_HEIGHT, STEREO_WIDTH, CV_8U);
	right.create(STEREO_HEIGHT, STEREO_WIDTH, CV_8U);
	disp.create(STEREO_HEIGHT, STEREO_WIDTH, CV_8U);
	fb.create(STEREO_HEIGHT, STEREO_WIDTH, CV_8U);

	ret = parse_parameters(argc, argv);
	STEREO_CHECK_ERROR();

	cfg.width	= STEREO_WIDTH;
	cfg.height	= STEREO_HEIGHT;
	cfg.mind	= g_mind;
	cfg.maxd	= g_maxd;
	cfg.calib	= g_calib;
	cfg.debug	= g_debug;
	stereo = stereo_start(&cfg);
	ret = stereo ? 0 : -1;
	STEREO_CHECK_ERROR();

	if (g_debug) {
		ret = open_fb(STEREO_WIDTH, STEREO_HEIGHT);
		STEREO_CHECK_ERROR();

		ret = blank_fb();
		STEREO_CHECK_ERROR();
	}

	fid = 1;
	while (1) {
		if (g_debug) {
			printf("=========== Frame %4d =============\n", fid);
			gettimeofday(&tv_begin, NULL);
		}

		f = get_iav_buf();
		ret = f ? 0 : -1;

		p = left.data;
		q = right.data;
#ifndef STEREO_VERTICALLY
		for (i = 0; i < STEREO_HEIGHT; i++) {
			memcpy(p, f, STEREO_WIDTH);
			memcpy(q, f + STEREO_WIDTH, STEREO_WIDTH);

			f += P;
			p += STEREO_WIDTH;
			q += STEREO_WIDTH;
		}
#else
		for (i = 0; i < STEREO_HEIGHT; i++) {
			memcpy(p, f, STEREO_WIDTH);

			f += P;
			p += STEREO_WIDTH;
		}

		for (i = 0; i < STEREO_HEIGHT; i++) {
			memcpy(q, f, STEREO_WIDTH);

			f += P;
			q += STEREO_WIDTH;
		}
#endif

		if (g_debug) {
			gettimeofday(&tv_end, NULL);
			seconds = 1000 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000;
			printf("%-24s: %6d ms\n", "Read Frame", seconds);
		}

		addr.L = left.data;
		addr.R = right.data;
		addr.D = disp.data;
		ret = stereo_frame(stereo, &addr);
		STEREO_CHECK_ERROR();

		if (g_debug) {
			p = disp.data;
			q = fb.data;
			for (i = 0; i < STEREO_WIDTH * STEREO_HEIGHT; i++) {
				if (*p * 4 > 255) {
					*q = 255;
				} else {
					*q = *p * 4;
				}

				p++;
				q++;
			}
			ret = render_frame((char *)fb.data);
			STEREO_CHECK_ERROR();

			if (g_distance) {
				while (1) {
					printf("Input X Y:\n");
					ret = scanf("%d %d", &x, &y);
					if (ret != 2) {
						break;
					}

					if (x < 0 || x >= STEREO_WIDTH || y < 0 || y >= STEREO_HEIGHT) {
						break;
					}

					p = disp.data + y * STEREO_WIDTH + x;
					if (*p) {
						printf("Distance at (%3d, %3d) is: %4.1fm (d = %d)\n", x, y, (double)cfg.ft / (*p * 100), *p);
					} else {
						printf("Distance at (%3d, %3d) is: N/A\n", x, y);
					}

					if (! *p) {
						continue;
					}

					sprintf(text, "%.1fm", (double)cfg.ft / (*p * 100));
					if (*p < 16) {
						putText(fb, text, Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 255, 255));
					} else {
						putText(fb, text, Point(x, y), FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(1, 1, 1));
					}

					ret = render_frame((char *)fb.data);
					STEREO_CHECK_ERROR();
				}
			}
		}

		if (g_snapshot) {
			sprintf(text, "%s/left.jpg", g_save);
			imwrite(text, left);
			sprintf(text, "%s/right.jpg", g_save);
			imwrite(text, right);
			sprintf(text, "%s/disp.jpg", g_save);
			imwrite(text, disp);
			break;
		}

		fid++;
	}

	ret = stereo_stop(stereo);
	STEREO_CHECK_ERROR();

	if (g_debug) {
		ret = close_fb();
		STEREO_CHECK_ERROR();
	}

	exit_iav();

	return ret;
}
