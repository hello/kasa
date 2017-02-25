/*
 * test_md_motbuf.cpp
 *
 * History:
 *	2015/07/22 - [Zhi He] create file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <ctype.h>

#include <basetypes.h>
#include "md_motbuf.h"

#define ROUND_DOWNTO_4(x)   ((x) & (-4))

static md_motbuf_init_t init_mdbuf = {
	.me_buffer = 0,
	.frame_interval =1,
	.pixel_line_interval = 2,
	.motion_end_num = 30,
};

static int buffer_p, buffer_w, buffer_h;

static void sigstop()
{
	md_motbuf_thread_stop();
	md_motbuf_deinit();
	printf("\nDeinit........Yes!\n");
	exit(1);
}

static void my_md_alarm(const u8 *md_event)
{
	int i;
	md_motbuf_evt_t *event = (md_motbuf_evt_t *)md_event;
	for (i=0; i<MD_MAX_ROI_N; i++) {
		if (event[i].motion_flag == MOT_START)
			printf("\tROI#%d Motion Start!\n", i);
		else if (event[i].motion_flag == MOT_END)
			printf("\tROI#%d Motion End!\n", i);
	}
}

void show_menu(void)
{
	printf("\n");
	printf("Motion Detection based on Motion Buffer\n");
	printf("1 - md_motbuf_set_roi_validity\n");
	printf("2 - md_motbuf_set_roi_location\n");
	printf("3 - md_motbuf_set_roi_sensitivity\n");
	printf("4 - md_motbuf_set_roi_threshold\n");
	printf("5 - md_motbuf_status_display\n");
	printf("6 - md_motbuf_get_event\n");
	printf("7 - md_motbuf_algo_setup\n");
	printf("8 - md_motbuf_callback_setup\n");
	printf("9 - md_motbuf_yuv2graybmp\n");
	printf("0 - Quit\n");
	printf("\nYour choice: ");
}

void md_motbuf_menuloop(void)
{
	int c, i, j, roi_index;
	char bmpname[128], input[16];
	md_motbuf_roi_t roi_info;
	md_motbuf_roi_location_t roi_loc;
	static md_motbuf_evt_t md_evt[MD_MAX_ROI_N];
	memset(&md_evt[0], 0, sizeof(md_motbuf_evt_t) * MD_MAX_ROI_N);
	int evt_flag[MD_MAX_ROI_N] = {0, 0, 0, 0};

	while (1) {
		show_menu();
		scanf("%s", input);
		c = atoi(input);
		switch(c) {
			case 1:
				do {
					printf("\nset_roi_validity choose a ROI. Index = (0~%d)", MD_MAX_ROI_N-1);
					scanf("%s", input);
					roi_index = atoi(input);
				} while (roi_index < 0 || roi_index > MD_MAX_ROI_N-1);
				do {
					printf("\nset_roi_validity validity = (0:invalid, 1:valid)");
					scanf("%s", input);
					i = atoi(input);
				} while (!(i==0 || i==1));

				if (md_motbuf_set_roi_validity(roi_index,i) != 0)
					printf("set_roi_validity failed.\n");
				break;
			case 2:
				do {
					printf("\nget_roi_info choose a ROI. Index = (0~%d)", MD_MAX_ROI_N-1);
					scanf("%s", input);
					roi_index = atoi(input);
				} while (roi_index < 0 || roi_index > MD_MAX_ROI_N-1);
				do {
					printf("\nset_roi_location x = (0~%d)", buffer_w-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > buffer_w-1);
				roi_loc.x = i;
				do {
					printf("\nset_roi_location y = (0~%d)", buffer_h-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > buffer_h-1);
				roi_loc.y = i;
				do {
					printf("\nset_roi_location width = (0~%d)", buffer_w-roi_loc.x-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > buffer_w-roi_loc.x-1);
				roi_loc.width = i;
				do {
					printf("\nset_roi_location height = (0~%d)", buffer_h-roi_loc.y-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > buffer_h-roi_loc.y-1);
				roi_loc.height = i;

				if (md_motbuf_set_roi_location(roi_index, &roi_loc) != 0)
					printf("\nset_roi_location failed\n");
				break;
			case 3:
				do {
					printf("\nset_roi_sensitivity choose a ROI. Index = (0~%d)", MD_MAX_ROI_N-1);
					scanf("%s", input);
					roi_index = atoi(input);
				} while (roi_index < 0 || roi_index > MD_MAX_ROI_N-1);
				do {
					printf("\nset_roi_sensitivity sensitivity = (0 ~ %d)", MD_SENSITIVITY_LEVEL-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > MD_SENSITIVITY_LEVEL-1);

				if (md_motbuf_set_roi_sensitivity(roi_index,i) != 0)
					printf("set_roi_sensitivity failed.\n");
				break;
			case 4:
				do {
					printf("\nset_roi_threshold choose a ROI. Index = (0~%d)", MD_MAX_ROI_N-1);
					scanf("%s", input);
					roi_index = atoi(input);
				} while (roi_index < 0 || roi_index > MD_MAX_ROI_N-1);
				do {
					printf("\nset_roi_threshold threshold = (0 ~ %d)", MD_MAX_THRESHOLD-1);
					scanf("%s", input);
					i = atoi(input);
				} while (i < 0 || i > MD_MAX_THRESHOLD-1);

				if (md_motbuf_set_roi_threshold(roi_index,i) != 0)
					printf("set_roi_threshold failed.\n");
				break;
			case 5:
				if (md_motbuf_status_display() != 0)
					printf("status_display failed\n");
				break;
			case 6:
				while (1) {
					md_motbuf_get_event((u8 *)&md_evt);

					for (j=0; j<MD_MAX_ROI_N; j++) {
						if (md_evt[j].valid == 1) {
							if (evt_flag[j] == MOT_NONE &&md_evt[j].motion_flag == MOT_HAS_STARTED) {
								evt_flag[j] = MOT_HAPPEN;
								printf("get_event roi %d motion start\n", j);
							}

							switch (md_evt[j].motion_flag) {
								case MOT_START:
									evt_flag[j] = MOT_HAPPEN;
									printf("get_event roi %d motion start\n", j);
									break;
								case MOT_END:
									evt_flag[j] = MOT_HAPPEN;
									printf("get_event roi %d motion end\n", j);
									break;
								default:
									break;
							}
					   }
					}
					printf("\n");
				}
				break;
			case 7:
				do {
					printf("\nalgo_setup frame diff algorithm 0(default) or 1(mse): ");
					scanf("%s", input);
					i = atoi(input);
				} while (!(i == 0 || i == 1));
				if (i==1)
					md_motbuf_algo_setup(algo_framediff_mse);
				break;
			case 8:
				md_motbuf_callback_setup(my_md_alarm);
				while (1) {
					sleep(10);
				}
				break;
			case 9:
				do {
					printf("\nset_roi_sensitivity choose a ROI. Index = (0~%d)", MD_MAX_ROI_N-1);
					scanf("%s", input);
					roi_index = atoi(input);
				} while (roi_index < 0 || roi_index > MD_MAX_ROI_N-1);
				if (md_motbuf_get_roi_info(roi_index, &roi_info) != 0)
					printf("\nget_roi_info failed\n");
				sprintf(bmpname, "roi_%d.bmp",roi_index);
				if (roi_info.roi_location.width & 0x3)
					printf("bmp width must be align to 4\n");
				else
					md_motbuf_yuv2graybmp(roi_info, bmpname, roi_info.roi_location.width, roi_info.roi_location.height);
				break;
			case 0:
				return;
			default:
				printf("unknown option \n");
				return;
		}
	}
}

// options and usage
#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"me-buffer", HAS_ARG, 0, 'b'},
	{"frame-interval", HAS_ARG, 0, 'f'},
	{"pixel-lines", HAS_ARG, 0, 'p'},
	{"motion-end", HAS_ARG, 0, 'e'},

	{0, 0, 0, 0}
};

static const char *short_options = "b:f:p:e:";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"0|1", "ME1 buffer from Main or Second source buffer, default is 0:Main"},
	{"1~10", "Frame interval, default it 1"},
	{"1~10", "For each frame, pixels in the setting lines are taken into account, default is 2"},
	{"1~100", "Frame count for checking motion end, default is 30"},
};

void usage(void)
{
	int i;
	printf("\ntest_md_motbuf usage:\n");
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
	printf("Examples:\n");
	printf("\tME1 buffer from Main source buffer:\n\t\ttest_md_motbuf -b 0 -f 1 -p 2 -e 30\n");

	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'b':
			init_mdbuf.me_buffer = atoi (optarg);
			break;
		case 'f':
			init_mdbuf.frame_interval = atoi (optarg);
			break;
		case 'p':
			init_mdbuf.pixel_line_interval = atoi (optarg);
			break;
		case 'e':
			init_mdbuf.motion_end_num= atoi (optarg);
			break;
		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}
	return 0;
}


int main(int argc, char **argv)
{
	int i;
	signal(SIGINT, sigstop);
	signal(SIGTERM, sigstop);
	signal(SIGQUIT, sigstop);
	md_motbuf_roi_location_t roi_location[MD_MAX_ROI_N];

	if (argc < 2) {
		usage();
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

	if (md_motbuf_init(&init_mdbuf) < 0) {
		printf("Init motion detect based on motion buffer failed.\n");
		goto failed;
	}

	printf("Init........Yes!\n");

	if (md_motbuf_get_current_buffer_P_W_H(&buffer_p, &buffer_w, &buffer_h) < 0) {
		printf("Get Motion Buffer pitch, width & height failed.\n");
		goto failed;
	}

	// Config ROIs
	roi_location[0].x = 0;
	roi_location[0].y = 0;
	roi_location[0].width = ROUND_DOWNTO_4(buffer_w/2);
	roi_location[0].height = buffer_h/2;

	roi_location[1].x = 0;
	roi_location[1].y = buffer_h/2 - 1;
	roi_location[1].width = ROUND_DOWNTO_4(buffer_w/2);
	roi_location[1].height = buffer_h/2;

	roi_location[2].x = ROUND_DOWNTO_4(buffer_w/2) - 1;
	roi_location[2].y = 0;
	roi_location[2].width = ROUND_DOWNTO_4(buffer_w/2);
	roi_location[2].height = buffer_h/2;

	roi_location[3].x = ROUND_DOWNTO_4(buffer_w/2) - 1;
	roi_location[3].y = buffer_h/2 - 1;
	roi_location[3].width = ROUND_DOWNTO_4(buffer_w/2);
	roi_location[3].height = buffer_h/2;

	// Set ROIs
	for (i=0; i<MD_MAX_ROI_N; i++)
		md_motbuf_set_roi_location(i, &roi_location[i]);

	// Set threshold
	md_motbuf_set_roi_threshold(0, 10);
	md_motbuf_set_roi_threshold(1, 5);
	md_motbuf_set_roi_threshold(2, 20);
	md_motbuf_set_roi_threshold(3, 10);

	// Set sensitivity
	md_motbuf_set_roi_sensitivity(0, 90);
	md_motbuf_set_roi_sensitivity(1, 90);
	md_motbuf_set_roi_sensitivity(2, 90);
	md_motbuf_set_roi_sensitivity(3, 10);

	// Set valid
	for (i=0; i<MD_MAX_ROI_N; i++)
		md_motbuf_set_roi_validity(i, 1);

	md_motbuf_thread_start();
	md_motbuf_menuloop();

	md_motbuf_thread_stop();

failed:
	md_motbuf_deinit();
	printf("\nDeinit........Yes!\n");

	return 0;
}
