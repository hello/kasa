/*
 * test_mainpp.c
 *
 * History:
 *  2015/04/02 - [Zhaoyang Chen] created file
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "iav_ioctl.h"
#include "utils.h"
#include "lib_vproc.h"

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#define MAX_MASK_ID       (65536)

enum {
	MENU_RUN = 0,
	AUTO_RUN,
	AUTO_RUN2
};

static int exit_flag = 0;
static int loglevel = AMBA_LOG_INFO;
static int run_mode = 0;
static int autorun_interval = 3;
static int autorun_size = 64;
static int verbose = 0;


static cawarp_param_t cawarp;

#define NO_ARG		0
#define HAS_ARG		1

static struct option long_options[] = {
	{"menu", NO_ARG, 0, 'm'},
	{"auto", HAS_ARG, 0, 'r'},
	{"auto-size", HAS_ARG, 0, 's' },
	{"random", NO_ARG, 0, 'R'},
	{"level", HAS_ARG, 0, 'l'},
	{"verbose", NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const char *short_options = "l:mr:Rs:v";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "\t\tInteractive menu"},
	{"3~30", "\tAuto run privacy mask every N frames"},
	{"8|16|32|...", "Auto run privacy mask size which must be multiple of 8."},
	{"", "\t\tAuto add and remove one privacy mask randomly."},
	{"0~2", "\tConfig log level. 0: error, 1: info (default), 2: debug"},
	{"", "\t\tPrint more message"},
};

static void usage(void)
{
	int i;

	printf("test_pm usage:\n");
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
	printf("\nExamples:");
	printf("\n  Menu:\n    test_mainpp -m\n");
	printf("\n  Auto run privacy masks every 10 frames:\n    test_mainpp -r 10\n");
	printf("\n  Auto add/remove one privacy mask randomly:\n    test_mainpp -R\n");
	printf("\n");
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'm':
			run_mode = MENU_RUN;
			break;
		case 'r':
			run_mode = AUTO_RUN;
			autorun_interval = atoi(optarg);
			break;
		case 'R':
			run_mode = AUTO_RUN2;
			break;
		case 's':
			autorun_size = atoi(optarg);
			break;
		case 'l':
			loglevel = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
			break;
		}
	}
	return 0;
}

static int load_cawarp_data_from_file(const char* filename)
{
#define LINE_BUF_LEN 128
	FILE* fp;
	int i = 0;
	char line[LINE_BUF_LEN];
	char* ptr;

	if (cawarp.data) {
		free(cawarp.data);
		cawarp.data = NULL;
	}
	if ((cawarp.data = malloc(cawarp.data_num * sizeof(float))) == NULL ) {
		perror("malloc");
		return -1;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("### Cannot open file [%s].\n", filename);
		return -1;
	}

	while ((ptr = fgets(line, LINE_BUF_LEN, fp)) != NULL) {
		cawarp.data[i++] = atof(ptr);
		if (i == cawarp.data_num)
			break;
	}

	fclose(fp);
	if (i < cawarp.data_num) {
		printf("Only %d are found in file. Expected is %d\n",
			i, cawarp.data_num);
	}

	return i < cawarp.data_num ? -1 : 0;
}

static int show_menu(void)
{
	printf("\n");
	printf("|-------------------------------------|\n");
	printf("| 1  - Privacy Mask                   |\n");
	printf("| 2  - Privacy Mask Color             |\n");
	printf("| 3  - DPTZ                           |\n");
	printf("| 4  - Circle Privacy Mask            |\n");
	printf("| 5  - Chromatic Aberration Correction|\n");
	printf("| q  - Quit                           |\n");
	printf("|-------------------------------------|\n");
	printf("Note: Option with * is supported in specfic mode.\n");
	printf("\n");
	return 0;
}

static int input_value(int min, int max)
{
	int retry = 1, input;
	char tmp[16];
	do {
		scanf("%s", tmp);
		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			continue;
		}
		retry = 0;
	} while (retry);
	return input;
}

static void quit()
{
	exit_flag = 1;
	vproc_exit();
	if (cawarp.data) {
		free(cawarp.data);
		cawarp.data = NULL;
	}

	exit(0);
}

// rectangle pm menu
static int pm_menu(void)
{
	pm_param_t pm;
	pm_cfg_t* pm_cfg;
	int pm_opt, i;

	printf("\nYour operation (1: Insert PM, 2: Remove PM, "
		"3: Draw PM, 4: Clear PM, 5: Show PM): ");
	pm_opt = input_value(1, 5);
	switch (pm_opt) {
	case 1:
	case 3:
		if (pm_opt == 1) {
			pm.op = VPROC_OP_INSERT;
			printf("\nInput mask num to be inserted: ");
		} else {
			pm.op = VPROC_OP_DRAW;
			printf("\nInput mask num to be drawed: ");
		}
		pm.pm_num= input_value(0, MAX_PM_NUM);
		pm_cfg = &pm.pm_list[0];
		for (i = 0; i < pm.pm_num; ++i) {
			printf("\nInput mask Id for PM %d: ", i);
			pm_cfg[i].id = input_value(0, MAX_MASK_ID);
			printf("Input mask x : ");
			pm_cfg[i].rect.x = input_value(0, 4096);
			printf("Input mask y : ");
			pm_cfg[i].rect.y = input_value(0, 4096);
			printf("Input mask width : ");
			pm_cfg[i].rect.width = input_value(0, 4096);
			printf("Input mask height : ");
			pm_cfg[i].rect.height = input_value(0, 4096);
		}
		vproc_pm(&pm);
		break;
	case 2:
		pm.op = VPROC_OP_REMOVE;
		printf("\nInput mask num to be removed: ");
		pm.pm_num= input_value(0, MAX_PM_NUM);
		pm_cfg = &pm.pm_list[0];
		for (i = 0; i < pm.pm_num; ++i) {
			printf("\nInput mask Id for PM %d: ", i);
			pm_cfg[i].id = input_value(0, MAX_MASK_ID);
		}
		vproc_pm(&pm);
		break;
	case 4:
		pm.op = VPROC_OP_CLEAR;
		vproc_pm(&pm);
		break;
	case 5:
		pm.op = VPROC_OP_SHOW;
		vproc_pm(&pm);
		printf("\nTotal mask num: %d.\n", pm.pm_num);
		pm_cfg = &pm.pm_list[0];
		for (i = 0; i < pm.pm_num; ++i) {
			printf("Mask ID: %d, ", pm_cfg[i].id);
			printf("offset: (%d, %d), ",
				pm_cfg[i].rect.x, pm_cfg[i].rect.y);
			printf("size: (%d, %d)\n",
				pm_cfg[i].rect.width, pm_cfg[i].rect.height);
		}
		break;
	default:
		printf("Unknown operation %d for PM.\n", pm_opt);
		break;
	}

	return 0;
}

// circle pm menu
static int pmc_menu(void)
{
	pmc_param_t pmc;
	pmc_cfg_t* pmc_cfg;
	int pm_opt, i;

	printf("\nYour operation (1: Insert PM, 2: Remove PM, "
		"3: Draw PM, 4: Clear PM, 5: Show PM): ");
	pm_opt = input_value(1, 5);
	switch (pm_opt) {
	case 1:
	case 3:
		if (pm_opt == 1) {
			pmc.op = VPROC_OP_INSERT;
			printf("\nInput circle mask num to be inserted: ");
		} else {
			pmc.op = VPROC_OP_DRAW;
			printf("\nInput circle mask num to be drawed: ");
		}
		pmc.pm_num= input_value(0, MAX_PMC_NUM);
		pmc_cfg = &pmc.pm_list[0];
		for (i = 0; i < pmc.pm_num; ++i) {
			printf("\nInput mask Id for Circle PM %d: ", i);
			pmc_cfg[i].id = input_value(0, MAX_MASK_ID);
			printf("Input circle mask type (1: Outside, 2: Inside): ");
			pmc_cfg[i].pmc_type = input_value(1, 2) - 1;
			printf("Input circle mask center x : ");
			pmc_cfg[i].area.center.x = input_value(0, 4096);
			printf("Input circle mask center y : ");
			pmc_cfg[i].area.center.y = input_value(0, 4096);
			printf("Input circle mask radius : ");
			pmc_cfg[i].area.radius = input_value(0, 4096);
		}
		vproc_pmc(&pmc);
		break;
	case 2:
		pmc.op = VPROC_OP_REMOVE;
		printf("\nInput mask num to be removed: ");
		pmc.pm_num= input_value(0, MAX_PMC_NUM);
		pmc_cfg = &pmc.pm_list[0];
		for (i = 0; i < pmc.pm_num; ++i) {
			printf("\nInput mask Id for PM %d: ", i);
			pmc_cfg[i].id = input_value(0, MAX_MASK_ID);
		}
		vproc_pmc(&pmc);
		break;
	case 4:
		pmc.op = VPROC_OP_CLEAR;
		vproc_pmc(&pmc);
		break;
	case 5:
		pmc.op = VPROC_OP_SHOW;
		vproc_pmc(&pmc);
		printf("\nTotal circle mask num: %d.\n", pmc.pm_num);
		pmc_cfg = &pmc.pm_list[0];
		for (i = 0; i < pmc.pm_num; ++i) {
			printf("Circle Mask ID: %d, ", pmc_cfg[i].id);
			printf("cernter offset: (%d, %d), ", pmc_cfg[i].area.center.x,
				pmc_cfg[i].area.center.y);
			printf("radius: %d, ", pmc_cfg[i].area.radius);
			printf("type: %s\n", (pmc_cfg[i].pmc_type == PMC_TYPE_INSIDE) ?
				"Inside" : "Outside");
		}
		break;
	default:
		printf("Unknown operation %d for Circle PM.\n", pm_opt);
		break;
	}

	return 0;
}

static int menu_run(void)
{
	char opt;
	char input[16], ca_file[128];
	yuv_color_t color;
	struct iav_digital_zoom dptz;

	memset(&cawarp, 0, sizeof(cawarp));

	while (!exit_flag) {
		show_menu();
		printf("Your choice: ");
		fflush(NULL);
		scanf("%s", input);
		opt = input[0];
		switch (opt) {
			case '1':
				pm_menu();
				break;
			case '2':
				printf("Input Y : ");
				color.y = input_value(0, 255);
				printf("Input U : ");
				color.u = input_value(0, 255);
				printf("Input V : ");
				color.v = input_value(0, 255);
				vproc_pm_color(&color);
				break;
			case '3':
				memset(&dptz, 0, sizeof(dptz));
				dptz.buf_id = IAV_SRCBUF_MN;
				printf("Input DPTZ input offset x : ");
				dptz.input.x = input_value(0, 10000);
				printf("Input DPTZ input offset y : ");
				dptz.input.y = input_value(0, 10000);
				printf("Input DPTZ input width : ");
				dptz.input.width = input_value(0, 10000);
				printf("Input DPTZ input height : ");
				dptz.input.height = input_value(0, 10000);
				vproc_dptz(&dptz);
				break;
			case '4':
				pmc_menu();
				break;
			case '5':
				memset(ca_file, 0, sizeof(ca_file));
				printf("Input CA data num (>= 2: enable CA, 0: disable CA) : ");
				cawarp.data_num = input_value(0, 10000);
				if (cawarp.data_num >= 2) {
					printf("Input CA data file: ");
					scanf("%s", ca_file);
					if (load_cawarp_data_from_file(ca_file) < 0) {
						break;
					}
					printf("Input Red scale factor: ");
					scanf("%f", &cawarp.red_factor);
					printf("Input Blue scale factor: ");
					scanf("%f", &cawarp.blue_factor);
					printf("Input Center Offset X: ");
					cawarp.center_offset_x = input_value(-10000, 10000);
					printf("Input Center Offset Y: ");
					cawarp.center_offset_y = input_value(-10000, 10000);
				}
				vproc_cawarp(&cawarp);
				break;
			case 'q':
				exit_flag = 1;
				break;
			default:
				printf("unknown option %c.\n", opt);
				break;
		}
	}
	return 0;
}

static int auto_run(void)
{
	int iav_fd = -1;
	struct iav_srcbuf_format main_buffer;
	int pic_width = 0, pic_height = 0;
	pm_param_t mask;
	pm_cfg_t* pm_cfg;
	int frame_count;
	int remove = 0;
	int ret = 0;
	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	main_buffer.buf_id = 0;
	if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_buffer) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		ret = -1;
		goto auto_run_exit;
	}
	pic_width = ROUND_DOWN(main_buffer.size.width, 16);
	pic_height = ROUND_DOWN(main_buffer.size.height, 16);

	mask.pm_num = 1;
	pm_cfg = &mask.pm_list[0];
	pm_cfg->rect.x = 0;
	pm_cfg->rect.y = 0;
	pm_cfg->rect.width = autorun_size;
	pm_cfg->rect.height = autorun_size;
	pm_cfg->id = 0;
	while (1) {
		if (!remove) {
			if (pm_cfg->rect.x + pm_cfg->rect.width > pic_width) {
				pm_cfg->rect.x = 0;
				pm_cfg->rect.y += pm_cfg->rect.height;
			}

			if (pm_cfg->rect.y + pm_cfg->rect.height > pic_height) {
				remove = 1;
				--pm_cfg->id;
			}
		} else {
			if (pm_cfg->id < 0) {
				if (pm_cfg->rect.width == pic_width &&
					pm_cfg->rect.height == pic_height) {
					pm_cfg->rect.width = autorun_size;
					pm_cfg->rect.height = autorun_size;
				} else {
					pm_cfg->rect.width *= 2;
					pm_cfg->rect.height *= 2;
					if (pm_cfg->rect.height > pic_height) {
						pm_cfg->rect.height = pic_height;
					}

					if (pm_cfg->rect.width > pic_width) {
						pm_cfg->rect.width = pic_width;
					}
				}
				pm_cfg->id = 0;
				pm_cfg->rect.x = 0;
				pm_cfg->rect.y = 0;
				remove = 0;
			}
		}

		if (verbose)
			start_timing();
		while (frame_count < autorun_interval) {
			ioctl(iav_fd, IAV_IOC_WAIT_NEXT_FRAME, 0);
			frame_count ++;
		}
		if (frame_count >= autorun_interval)
			frame_count = 0;

		if (remove) {
			mask.op = VPROC_OP_REMOVE;
			vproc_pm(&mask);
			INFO("remove mask [%d]\n", pm_cfg->id);
			--pm_cfg->id;
		} else {
			mask.op = VPROC_OP_INSERT;
			vproc_pm(&mask);
			INFO("add mask [%d] x %d, y %d, w %d, h %d\n", pm_cfg->id,
				pm_cfg->rect.x, pm_cfg->rect.y, pm_cfg->rect.width,
				pm_cfg->rect.height);
			++pm_cfg->id;
			pm_cfg->rect.x += pm_cfg->rect.width;
		}

		if (verbose) {
			stop_timing();
			perf_report();
		}

	}

auto_run_exit:
	if (iav_fd >= 0) {
		close(iav_fd);
		iav_fd = -1;
	}
	return ret;
}

int auto_run2(void)
{
	int iav_fd = -1;
	struct iav_srcbuf_format main_buffer;
	int pic_width = 0, pic_height = 0, align;
	pm_param_t mask = {0};
	pm_cfg_t* pm_cfg;
	int ret = 0;

	if ((iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	main_buffer.buf_id = 0;
	if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_buffer) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		ret = -1;
		goto auto_run2_exit;
	}
	pic_width = ROUND_DOWN(main_buffer.size.width, 16);
	pic_height = ROUND_DOWN(main_buffer.size.height, 16);
	align = 16;

	mask.pm_num = 1;
	pm_cfg = &mask.pm_list[0];
	while (1) {
		pm_cfg->rect.x = ROUND_DOWN(rand() % pic_width, align);
		pm_cfg->rect.y = ROUND_DOWN(rand() % pic_height, align);
		pm_cfg->rect.width =  ROUND_UP(8 * (10 + rand() % 50), align);
		pm_cfg->rect.height = ROUND_UP(8 * (10 + rand() % 50), align);
		if (pm_cfg->rect.x + pm_cfg->rect.width > pic_width)
			pm_cfg->rect.x -= (pm_cfg->rect.x + pm_cfg->rect.width) - pic_width;

		if (pm_cfg->rect.y + pm_cfg->rect.height > pic_height)
			pm_cfg->rect.y -= (pm_cfg->rect.y + pm_cfg->rect.height) - pic_height;

		INFO("adding mask x %d, y %d, w %d, h %d\n",
		    pm_cfg->rect.x, pm_cfg->rect.y, pm_cfg->rect.width, pm_cfg->rect.height);
		mask.op = VPROC_OP_INSERT;
		vproc_pm(&mask);
		usleep(1000 * 1000);
		INFO("removing ...\n");
		mask.op = VPROC_OP_REMOVE;
		vproc_pm(&mask);
		usleep(100 * 1000);
	}

auto_run2_exit:
	if (iav_fd >= 0) {
		close(iav_fd);
		iav_fd = -1;
	}
	return ret;
}

int main(int argc, char** argv)
{
	version_t version;
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGQUIT, quit);

	if (argc < 2) {
		usage();
		return 0;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	vproc_version(&version);
	printf("Privacy Mask Library Version: %s-%d.%d.%d (Last updated: %x)\n\n",
	    version.description, version.major, version.minor,
	    version.patch, version.mod_time);

	set_log(loglevel, NULL);

	if (run_mode == MENU_RUN) {
		menu_run();
	} else if (run_mode == AUTO_RUN){
		auto_run();
	} else if (run_mode == AUTO_RUN2) {
		auto_run2();
	}

	quit();
	return 0;
}

