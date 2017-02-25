/*
 *
 * History:
 *    2016/08/31 - [Zhenwu Xue] Create
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
#include <getopt.h>
#include "mdet_lib.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sys/time.h>

#define	MEASURE_TIME_VARIABLES()	\
	struct timeval			tv1, tv2;	\
	int						tms

#define MEASURE_TIME_START()		gettimeofday(&tv1, NULL)

#define MEASURE_TIME_END(s)	gettimeofday(&tv2, NULL);	\
	tms = 1000 * (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000;	\
	printf("%-24s: %6d ms\n", s, tms)

#define CDET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __FILE__, __LINE__);	\
		return ret;	\
	}


using namespace cv;

#include "../utils2/fb.cpp"
#include "../utils2/iav.cpp"
#include "../utils2/csc.cpp"

static int				a_video = 0;
static int				a_vw = 640;
static int				a_vh = 360;
static int				a_gui = 0;

static mdet_instance	*v_MI;
static mdet_cfg			v_cfg;
static mdet_roi_t		*v_roi;
static mdet_session_t	v_ms;


static second_buf_t	v_buf;

struct option long_options[] = {
	{"video",	1,	0,	'v'},
	{"gui",		0,	0,	'g'},
	{0,			0,	0,	 0 },
};

const char *short_options = "v:g";

int init_param(int argc, char **argv)
{
	int		c;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {

		case 'v':
			sscanf(optarg, "%dx%d", &a_vw, &a_vh);
			if (a_vw >= 0 && a_vh >= 0) {
				a_video = 1;
			}
			break;

		case 'g':
			a_gui = 1;
			break;

		default:
			printf("Unknown parameter %c!\n", c);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{

	int				ret;
	int				i, j;
	Mat				rgb1, rgb2, fg;

	MEASURE_TIME_VARIABLES();

	ret = init_param(argc, argv);
	CDET_CHECK_ERROR();

	if (a_gui) {
		ret = open_fb(&a_vw, &a_vh);
		CDET_CHECK_ERROR();

		ret = blank_fb();
		CDET_CHECK_ERROR();
	}

	ret = init_iav();
	CDET_CHECK_ERROR();

	ret = get_second_buf(&v_buf);
	CDET_CHECK_ERROR();

	v_MI = mdet_create_instance(MDET_ALGO_AWS);
	if (!v_MI) {
		ret = -1;
		CDET_CHECK_ERROR();
	}

	ret = (*v_MI->md_get_config)(&v_cfg);
	CDET_CHECK_ERROR();

	v_cfg.fm_dim.width		= a_vw;
	v_cfg.fm_dim.height		= a_vh;
	v_cfg.fm_dim.pitch		= 3 * a_vw;

	v_cfg.roi_info.num_roi	= 1;
	v_roi					= v_cfg.roi_info.roi;
	v_roi->num_points		= 4;
	v_roi->points[0].x		= 0;
	v_roi->points[0].y		= 0;
	v_roi->points[1].x		= a_vw - 1;
	v_roi->points[1].y		= 0;
	v_roi->points[2].x		= a_vw - 1;
	v_roi->points[2].y		= a_vh - 1;
	v_roi->points[3].x		= 0;
	v_roi->points[3].y		= a_vh - 1;
	v_roi->type				= MDET_REGION_POLYGON;

	ret = (*v_MI->md_set_config)(&v_cfg);
	CDET_CHECK_ERROR();

	memset(&v_ms, 0, sizeof(v_ms));
	ret = (*v_MI->md_start)(&v_ms);
	CDET_CHECK_ERROR();

	rgb1.create(v_buf.yuv_h, v_buf.yuv_w, CV_8UC3);
	rgb2.create(a_vh, a_vw, CV_8UC3);
	fg.create(a_vh, a_vw, CV_8UC3);

	i = 1;
	while (1) {
		ret = get_second_buf(&v_buf);
		CDET_CHECK_ERROR();

		printf("Processing Frame %5d ...\n", i);

		MEASURE_TIME_START();

		__yuv_2_rgb__(v_buf.yuv_w, v_buf.yuv_h, v_buf.yuv_p, rgb1.step, v_buf.yuv_y, v_buf.yuv_uv, rgb1.data);
		resize(rgb1, rgb2, rgb2.size(), CV_INTER_AREA);
		ret = (*v_MI->md_update_frame)(&v_ms, rgb2.data, 0);

		MEASURE_TIME_END("Change Detection");
		CDET_CHECK_ERROR();

		printf("\n");

		for (j = 0; j < a_vw * a_vh; j++) {
			fg.data[3 * j + 0] = v_ms.fg[j];
			fg.data[3 * j + 1] = v_ms.fg[j];
			fg.data[3 * j + 2] = v_ms.fg[j];
		}

		if (a_gui) {
			ret = render_fb(fg.data, fg.cols, fg.rows, fg.step, 0, 0);
			CDET_CHECK_ERROR();

			ret = refresh_fb();
			CDET_CHECK_ERROR();
		}

		i++;
	}

	ret = (*v_MI->md_stop)(&v_ms);
	CDET_CHECK_ERROR();

	mdet_destroy_instance(v_MI);

	return 0;
}
