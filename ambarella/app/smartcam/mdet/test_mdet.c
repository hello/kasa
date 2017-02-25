/*
 *
 * History:
 *    2013/12/18 - [Zhenwu Xue] Create
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
#include <sys/time.h>
#include <getopt.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include "iav.h"
#include "fb.h"
#include "mdet_lib.h"

#define MDET_LINE_WIDTH		2

#define MDET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}

#define NO_ARG		0
#define HAS_ARG		1

struct option long_options[] = {
	{"help",	NO_ARG,	0,	'h'},
	{"verbose",	NO_ARG,	0,	'v'},
	{"gui",	NO_ARG,	0,	'g'},
	{"motion",	NO_ARG,	0,	'm'},
	{"intrude",	NO_ARG,	0,	'i'},
	{"cross",	NO_ARG,	0,	'c'},
	{"buf_id",	HAS_ARG,	0,	'b'},
	{"buf_type",	HAS_ARG,	0,	't'},
	{"algo_type",	HAS_ARG,	0,	'a'},
	{"alphaT",	HAS_ARG,	0,	'T'},
	{"threshold",	HAS_ARG,	0,	'S'},
	{"polygon",	HAS_ARG,	0,	'P'},
	{"line",	HAS_ARG,	0,	'L'},
	{0,	0,	0,	0},
};

const char *short_options = "hvgmicb:t:a:S:T:P:L:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] =
{
	{"", "Show usage\n"},
	{"", "Show motion detection detail info\n"},
	{"", "Show detection result through frame buffer. Must enable and configure frame buffer first"},
	{"", "Show front ground pixels through frame buffer"},
	{"", "Show intrude info of the ROI"},
	{"", "Show cross info of the line"},
	{"0-3", "Source buffer ID, 0:main buffer, 1:second buffer, 2:third buffer, 3:fourth buffer"},
	{"0-2", "Source buffer type, 0:YUV, 1:ME0, 2:ME1"},
	{"0-2", "Algorithm type, 0:frame diff, 1:median, 2:MOG2"},
	{"0-", "AlphaT, motion diff value smaller than alphaT will be ignored"},
	{"0-", "Threshold for ROI, motion value bigger than threshold will trigger frame buffer showing"},
	{"points coordinate", "Specify a polygon ROI, maximum edges are 8. Points input should be placed clockwise. For 180x120 whole buffer, parameters should be like 0,0,179,0,179,119,0,119"},
	{"points coordinate", "specify a line. Maximum points are 8. Points input should be placed clockwise."},
};
static void usage(void)
{
	u32 i;
	printf("This program is used to test motion detection algorithm, options are:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i ++) {
		if (isalpha(long_options[i].val)) {
			printf("-%c ", long_options[i].val);
		} else {
			printf("  ");
		}
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0) {
			printf(" [%s]", hint[i].arg);
		}
		printf("\t%s\n", hint[i].str);
	}
	printf("\n[Example commands]:\n");
	printf("\nUse ME1 data(180x120, only Y) from second buffer(720x480) to do motion detection. "
			"ROI#0 is the whole ME1 buffer, ROI#1 is left top 1/4 of the buffer.\n");
	printf("#test_mdet -b 1 -a 0 -t 2 -P 0,0,179,0,179,119,0,119 -P 0,0,89,0,89,59,0,59 -v\n");

	printf("\nUse YUV data's Y component from second buffer(720x480) to do motion detection. "
			"ROI#0 is the whole buffer, ROI#1 is right 1/2 of the buffer.\n");
	printf("#test_mdet -b 1 -a 0 -t 0 -P 0,0,719,0,719,479,0,479 -P 359,0,719,0,719,479,359,479 -v\n");

	printf("\nUse ME0 data(120x68, only Y) from main buffer(1920x1080) to do motion detection. "
			"ROI#0 is the whole buffer. Open ROI intruding warning.\n");
	printf("#test_mdet -b 0 -a 0 -t 1 -P 0,0,119,0,119,67,0,67 -i -v\n");

	printf("\nUse ME1 data(180x120, only Y) from second buffer(720x480) to do motion detection. "
			"ROI#0 is a line. Open line crossing warning. Algorithm is MOG2\n");
	printf("#test_mdet -b 1 -a 2 -t 2 -L 0,59,119,59 -c -v\n");
}

static int			g_verbose	= 0;
static int			g_gui		= 0;
static int			g_motion	= 0;
static int			g_intrude	= 0;
static int			g_cross	= 0;
static iav_buf_id_t		g_buf_id	= IAV_BUF_SECOND;
static iav_buf_type_t		g_buf_type	= IAV_BUF_ME1;
static mdet_algo_t		g_algo	= MDET_ALGO_DIFF;
static float			g_threshold	= 0.05;
static mdet_session_t		g_ms;
static mdet_cfg g_mdet_cfg;
static int is_th_set = 0;
static char			*g_fbmem	= NULL;

mdet_point_t get_points1(mdet_point_t p1, mdet_point_t p2)
{
	mdet_point_t	p3;
	int		dx, dy;
	float		tmp;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;

	if (dx == 0 && dy == 0) {
		return p2;
	}

	if (dx == 0 && dy != 0) {
		p3.x = p2.x + MDET_LINE_WIDTH;
		p3.y = p2.y;
		return p3;
	}

	if (dx != 0 && dy == 0) {
		p3.x = p2.x;
		p3.y = p2.y + MDET_LINE_WIDTH;
		return p3;
	}

	tmp = (float)dy / dx;
	tmp = 1.0f + tmp * tmp;
	tmp = sqrt(tmp);
	tmp = MDET_LINE_WIDTH / tmp;
	p3.y = p2.y + (int)tmp;

	tmp = (float)dx / dy;
	tmp = 1.0f + tmp * tmp;
	tmp = sqrt(tmp);
	tmp = MDET_LINE_WIDTH / tmp;
	if (dx * dy < 0) {
		p3.x = p2.x + (int)tmp;
	} else {
		p3.x = p2.x - (int)tmp;
	}

	return p3;
}

mdet_point_t get_points2(mdet_point_t p1, mdet_point_t p2)
{
	mdet_point_t	p3;
	int		dx, dy;
	float		tmp;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;

	if (dx == 0 && dy == 0) {
		return p2;
	}

	if (dx == 0 && dy != 0) {
		p3.x = p2.x - MDET_LINE_WIDTH;
		p3.y = p2.y;
		return p3;
	}

	if (dx != 0 && dy == 0) {
		p3.x = p2.x;
		p3.y = p2.y - MDET_LINE_WIDTH;
		return p3;
	}

	tmp = (float)dy / dx;
	tmp = 1.0f + tmp * tmp;
	tmp = sqrt(tmp);
	tmp = MDET_LINE_WIDTH / tmp;
	p3.y = p2.y - (int)tmp;

	tmp = (float)dx / dy;
	tmp = 1.0f + tmp * tmp;
	tmp = sqrt(tmp);
	tmp = MDET_LINE_WIDTH / tmp;
	if (dx * dy < 0) {
		p3.x = p2.x - (int)tmp;
	} else {
		p3.x = p2.x + (int)tmp;
	}

	return p3;
}

int line2polygon(mdet_roi_t *r)
{
	int		i, j = 0;
	mdet_roi_t	R;

	memcpy(&R, r, sizeof(R));

	r->points[j++] = get_points1(R.points[1], R.points[0]);
	for (i = 1; i < r->num_points; i++) {
		r->points[j++] = get_points1(R.points[i - 1], R.points[i]);
	}
	for (i = r->num_points - 1; i >= 1 ; i--) {
		r->points[j++] = get_points2(R.points[i - 1], R.points[i]);
	}
	r->points[j++] = get_points2(R.points[1], R.points[0]);

	r->num_points	= j;

	return 0;
}

int parse_parameters(int argc, char **argv)
{
	int		ret;
	int		c;
	mdet_roi_t	*r;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'v':
			g_verbose = 1;
			break;

		case 'g':
			g_gui = 1;
			break;

		case 'm':
			g_motion = 1;
			break;

		case 'i':
			g_intrude = 1;
			break;

		case 'c':
			g_cross = 1;
			break;

		case 'b':
			ret = sscanf(optarg, "%u", &g_buf_id);
			if (ret != 1 || g_buf_id < 0) {
				printf("Buffer ID is wrong!\n");
				return -1;
			} else {
				printf("Buffer ID = %u\n", g_buf_id);
			}
			break;

		case 't':
			ret = sscanf(optarg, "%u", &g_buf_type);
			if (ret != 1 || g_buf_type < 0) {
				printf("Buffer type is wrong!\n");
				return -1;
			} else {
				printf("Buffer type = %u\n", g_buf_type);
			}
			break;

		case 'a':
			ret = sscanf(optarg, "%u", &g_algo);
			if (ret != 1 || g_algo < 0 || g_algo > 2) {
				printf("Algorithm type must be between 0 and 2!\n");
				return -1;
			}
			break;

		case 'S':
			ret = sscanf(optarg, "%f", &g_threshold);
			if (ret != 1 || g_threshold < 0 || g_threshold > 1.0) {
				printf("Threshold must be between 0 and 1!\n");
				return -1;
			}
			break;

		case 'T':
			ret = sscanf(optarg, "%u", &g_mdet_cfg.threshold);
			if (!g_mdet_cfg.threshold) {
				printf("T must be larger than 0!\n");
				return -1;
			} else {
				is_th_set = 1;
			}
			break;

		case 'P':
			if (g_mdet_cfg.roi_info.num_roi >= MDET_MAX_ROIS) {
				printf("WARNING: ROI number exceeds maximum %u!\n", MDET_MAX_ROIS);
				return -1;
			}
			r = &g_mdet_cfg.roi_info.roi[g_mdet_cfg.roi_info.num_roi];
			ret = sscanf(optarg,
				"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
				&r->points[0].x, &r->points[0].y,
				&r->points[1].x, &r->points[1].y,
				&r->points[2].x, &r->points[2].y,
				&r->points[3].x, &r->points[3].y,
				&r->points[4].x, &r->points[4].y,
				&r->points[5].x, &r->points[5].y,
				&r->points[6].x, &r->points[6].y,
				&r->points[7].x, &r->points[7].y);
			if (ret >= 6) {
				r->type		= MDET_REGION_POLYGON;
				r->num_points	= ret / 2;
				g_mdet_cfg.roi_info.num_roi++;
			} else {
				printf("Polygon must have at least three points!\n");
				return -1;
			}
			break;

		case 'L':
			if (g_mdet_cfg.roi_info.num_roi >= MDET_MAX_ROIS) {
				printf("WARNING: ROI number exceeds maximum %u!\n", MDET_MAX_ROIS);
				return -1;
			}
			r = &g_mdet_cfg.roi_info.roi[g_mdet_cfg.roi_info.num_roi];
			ret = sscanf(optarg,
				"%u,%u,%u,%u,%u,%u,%u,%u",
				&r->points[0].x, &r->points[0].y,
				&r->points[1].x, &r->points[1].y,
				&r->points[2].x, &r->points[2].y,
				&r->points[3].x, &r->points[3].y);
			if (ret >= 4) {
				r->type		= MDET_REGION_LINE;
				r->num_points	= ret / 2;

				ret = line2polygon(r);
				if (!ret) {
					g_mdet_cfg.roi_info.num_roi++;
				}
			} else {
				printf("Line must have ae least two points!\n");
				return -1;
			}
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			break;

		}
	}

	return 0;
}

int draw_line(mdet_point_t p1, mdet_point_t p2, char color)
{
	int	xl, xr, yl, yh;
	int	x, y;
	int	dx, dy;

	if (p1.x > p2.x) {
		xl = p2.x;
		xr = p1.x;
	} else {
		xl = p1.x;
		xr = p2.x;
	}

	if (p1.y > p2.y) {
		yl = p2.y;
		yh = p1.y;
	} else {
		yl = p1.y;
		yh = p2.y;
	}

	if (p1.x == p2.x) {
		for (y = yl; y <= yh; y++) {
			g_fbmem[y * g_mdet_cfg.fm_dim.width + p1.x] = color;
		}

		return 0;
	}

	if (p1.y == p2.y) {
		for (x = xl; x <= xr; x++) {
			g_fbmem[p1.y * g_mdet_cfg.fm_dim.width + x] = color;
		}

		return 0;
	}

	dx = 2 * (p2.x - p1.x);
	dy = 2 * (p2.y - p1.y);

	for (x = xl; x <= xr; x++) {
		y = (p1.y * dx - p1.x * dy + x * dy + dx / 2) / dx;
		g_fbmem[y * g_mdet_cfg.fm_dim.width + x] = color;
	}

	for (y = yl; y <= yh; y++) {
		x = (p1.x * dy - p1.y * dx + y * dx + dy / 2) / dy;
		g_fbmem[y * g_mdet_cfg.fm_dim.width + x] = color;
	}

	return 0;
}

int draw_polygon(mdet_roi_t *p, char color)
{
	int		i;
	int		ret = 0;

	for (i = 0; i < p->num_points - 1; i++) {
		ret = draw_line(p->points[i], p->points[i + 1], color);
		MDET_CHECK_ERROR();
	}

	ret = draw_line(p->points[0], p->points[p->num_points - 1], color);
	MDET_CHECK_ERROR();

	return ret;
}

static void sigstop()
{
	if (g_gui) {
		blank_fb();
		printf("\nClear frame buffer!\n");
	}
	exit(1);
}

int main(int argc, char **argv)
{
	int				ret;
	u32				i, j;
	unsigned int			w, h, p;
	char				*buf = NULL;
	mdet_instance *inst = NULL;
	mdet_roi_t			*pr = NULL;
	mdet_point_t			pt, *ppt;
	struct timeval			tv_begin, tv_end;
	mdet_cfg mdet_cfg_get;

	memset(&g_ms, 0, sizeof(g_ms));

	if (argc < 2) {
		usage();
		return -1;
	}

	ret = parse_parameters(argc, argv);
	MDET_CHECK_ERROR();

	inst = mdet_create_instance(g_algo);
	if (!inst) {
		printf("mdet_create_instance failed\n");
		return -1;
	}

	(*inst->md_get_config)(&mdet_cfg_get);//get default configure

	if (is_th_set == 0) {//If do not specify threshold, default threshold will be used
		g_mdet_cfg.threshold = mdet_cfg_get.threshold;
	}

	ret = init_iav(g_buf_id, g_buf_type);
	MDET_CHECK_ERROR();

	ret = get_iav_buf_size(&p, &w, &h);
	MDET_CHECK_ERROR();
	if (g_verbose) {
		printf("Buffer Size: %d x %d, pitch = %d\n", w, h, p);
	}

	g_mdet_cfg.fm_dim.pitch	= p;
	g_mdet_cfg.fm_dim.width	= w;
	g_mdet_cfg.fm_dim.height	= h;

	if (!g_mdet_cfg.roi_info.num_roi) {
		pr			= g_mdet_cfg.roi_info.roi;

		if (g_intrude) {
			pr->type		= MDET_REGION_POLYGON;
			pr->num_points		= 5;

			pt.x	= w / 4;
			pt.y	= h / 4;
			pr->points[0]		= pt;

			pt.x	= 3 * w / 8;
			pt.y	= h / 4;
			pr->points[1]		= pt;

			pt.x	= w / 2;
			pt.y	= h / 2;
			pr->points[2]		= pt;

			pt.x	= 3 * w / 8;
			pt.y	= 3 * h / 4;
			pr->points[3]		= pt;

			pt.x	= w / 4;
			pt.y	= 3 * h / 4;
			pr->points[4]		= pt;

			pr++;

			pr->type		= MDET_REGION_POLYGON;
			pr->num_points		= 3;

			pt.x	= 3 * w / 4;
			pt.y	= h / 3;
			pr->points[0]		= pt;

			pt.x	= 7 * w / 8;
			pt.y	= 2 * h / 3;
			pr->points[1]		= pt;

			pt.x	= 5 * w / 8;
			pt.y	= 2 * h / 3;
			pr->points[2]		= pt;

			g_mdet_cfg.roi_info.num_roi		= 2;

			pr++;
		}

		if (g_cross) {
			pr->type		= MDET_REGION_LINE;
			pr->num_points		= 4;

			pt.x	= 19 * w / 32;
			pt.y	= h / 8;
			pr->points[0]		= pt;

			pt.x	= 9 * w / 16;
			pt.y	= h / 2;
			pr->points[1]		= pt;

			pt.x	= 5 * w / 8;
			pt.y	= 7 * h / 8;
			pr->points[2]		= pt;

			pt.x	= 19 * w / 32;
			pt.y	= 15 * h / 16;
			pr->points[3]		= pt;

			line2polygon(pr);

			pr++;
			g_mdet_cfg.roi_info.num_roi++;
		}
	}

	pr = g_mdet_cfg.roi_info.roi;
	for (i = 0; i < g_mdet_cfg.roi_info.num_roi; i++) {
		for (j = 0, ppt = pr[i].points; j < pr[i].num_points; j++, ppt++) {
			if (ppt->x < 0) {
				ppt->x = 0;
			}
			if (ppt->x >= w) {
				ppt->x = w - 1;
			}
			if (ppt->y < 0) {
				ppt->y = 0;
			}
			if (ppt->y >= h) {
				ppt->y = h - 1;
			}
		}
	}

	if (g_gui) {
		ret = open_fb(w, h);
		MDET_CHECK_ERROR();

		ret = blank_fb();
		MDET_CHECK_ERROR();

		g_fbmem = malloc(w * h);
		if (!g_fbmem) {
			printf("%s %d: Error!\n", __func__, __LINE__);
			return -1;
		}
	}

	ret = (*inst->md_set_config)(&g_mdet_cfg);
	MDET_CHECK_ERROR();

	ret = (*inst->md_start)(&g_ms);
	MDET_CHECK_ERROR();

	signal(SIGINT,  sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	while (1) {
		if (g_verbose) {
			gettimeofday(&tv_begin, NULL);
		}

		buf = get_iav_buf();
		if (!buf) {
			ret = -1;
		}
		MDET_CHECK_ERROR();
		if (g_verbose) {
			gettimeofday(&tv_end, NULL);
			printf("Get Buf: %ld ms\n", 1000 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000);
		}

		if (g_verbose) {
			gettimeofday(&tv_begin, NULL);
		}
		(*inst->md_update_frame)(&g_ms, (u8 *)buf, g_mdet_cfg.threshold);
		if (g_verbose) {
			gettimeofday(&tv_end, NULL);
			printf("Mdet: %ld ms\n", 1000 * (tv_end.tv_sec - tv_begin.tv_sec) + (tv_end.tv_usec - tv_begin.tv_usec) / 1000);
			for (i = 0; i < g_mdet_cfg.roi_info.num_roi; i++) {
				printf("ROI#%d: motion value = %f\n", i, g_ms.motion[i]);
			}
		}

		if (g_gui) {
			memset(g_fbmem, 0, w * h);
		}

		if (g_gui && g_motion) {
			for (i = 0; i < g_mdet_cfg.fm_dim.width * g_mdet_cfg.fm_dim.height; i++) {
				if (g_ms.fg[i]) {
					g_fbmem[i] = FB_COLOR_BLUE;
				}
			}
		}

		if (g_intrude) {
			for (i = 0; i < g_mdet_cfg.roi_info.num_roi; i++) {
				if (g_mdet_cfg.roi_info.roi[i].type != MDET_REGION_POLYGON) {
					continue;
				}

				if (g_ms.motion[i] >= g_threshold) {
					if (g_gui) {
						ret = draw_polygon(&g_mdet_cfg.roi_info.roi[i], FB_COLOR_RED);
						MDET_CHECK_ERROR();
					}
					if (g_verbose) {
						printf("Warning: ROI #%d is being intruded!\n", i);
					}
				} else {
					if (g_gui) {
						ret = draw_polygon(&g_mdet_cfg.roi_info.roi[i], FB_COLOR_WHITE);
						MDET_CHECK_ERROR();
					}
				}
			}
		}

		if (g_cross) {
			for (i = 0; i < g_mdet_cfg.roi_info.num_roi; i++) {
				if (g_mdet_cfg.roi_info.roi[i].type != MDET_REGION_LINE) {
					continue;
				}

				if (g_ms.motion[i] >= g_threshold) {
					if (g_gui) {
						ret = draw_polygon(&g_mdet_cfg.roi_info.roi[i], FB_COLOR_RED);
						MDET_CHECK_ERROR();
					}
					if (g_verbose) {
						printf("Warning: ROI #%d is being crossed!\n", i);
					}
				} else {
					if (g_gui) {
						ret = draw_polygon(&g_mdet_cfg.roi_info.roi[i], FB_COLOR_WHITE);
						MDET_CHECK_ERROR();
					}
				}
			}
		}

		if (g_gui) {
			ret = render_frame(g_fbmem);
			MDET_CHECK_ERROR();
		}
	}

	exit_iav();
	(*inst->md_stop)(&g_ms);
	mdet_destroy_instance(inst);
	inst = NULL;
	if (g_gui) {
		ret = close_fb();
		MDET_CHECK_ERROR();
	}

	return ret;
}
