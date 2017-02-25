/*
 * cali_lens_focus.c
 *
 * History:
 *	2013/08/13 - [Jingyang qiu] created file
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


#include <sys/signal.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/shm.h>
#include "stdio.h"
#include "fb_image.h"

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"

#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"

#include "AmbaDSP_Img3aStatistics.h"

static int log_level = MSG_LEVEL;

typedef enum {
	RGB_STATIS_AF_HIST = 0,
	CFA_STATIS_AF_HIST,
} STATISTICS_AF_TYPE;

typedef enum {
	AF_FY = 0,
	AF_FV1,
	AF_FV2,
} STATISTICS_AF_HISTOGRAM_TYPE;

typedef struct {
	enum amba_video_mode mode;
	uint32_t width;
	uint32_t height;
} __amba_video_mode_info_t;

static __amba_video_mode_info_t video_modes_info[] =
{
	{ AMBA_VIDEO_MODE_AUTO       ,   0,    0 },
	{ AMBA_VIDEO_MODE_HDMI_NATIVE,   0,    0 },
	{ AMBA_VIDEO_MODE_480I       , 720,  480 },
	{ AMBA_VIDEO_MODE_576I       , 720,  576 },
	{ AMBA_VIDEO_MODE_D1_NTSC    , 720,  480 },
	{ AMBA_VIDEO_MODE_D1_PAL     , 720,  480 },
	{ AMBA_VIDEO_MODE_720P       ,1280,  720 },
	{ AMBA_VIDEO_MODE_720P_PAL   ,1280,  720 },
	{ AMBA_VIDEO_MODE_720P30     ,1280,  720 },
	{ AMBA_VIDEO_MODE_720P25     ,1280,  720 },
	{ AMBA_VIDEO_MODE_720P24     ,1280,  720 },
	{ AMBA_VIDEO_MODE_1080I      ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080I_PAL  ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080P      ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080P_PAL  ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080P30    ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080P25    ,1920, 1080 },
	{ AMBA_VIDEO_MODE_1080P24    ,1920, 1080 },
	{ AMBA_VIDEO_MODE_2160P30    ,3840, 2160 },
	{ AMBA_VIDEO_MODE_2160P25    ,3840, 2160 },
	{ AMBA_VIDEO_MODE_2160P24    ,3840, 2160 },
	{ AMBA_VIDEO_MODE_2160P24_SE ,4096, 2160 },
	{ AMBA_VIDEO_MODE_D1_NTSC    , 720,  480 },
	{ AMBA_VIDEO_MODE_D1_PAL     , 720,  480 },
	{ AMBA_VIDEO_MODE_320_240    , 320,  240 },
	{ AMBA_VIDEO_MODE_HVGA       , 320,  480 },
	{ AMBA_VIDEO_MODE_VGA        , 640,  480 },
	{ AMBA_VIDEO_MODE_WVGA       , 800,  480 },
	{ AMBA_VIDEO_MODE_WSVGA      ,1024,  600 },
	{ AMBA_VIDEO_MODE_QXGA       ,2048, 1536 },
	{ AMBA_VIDEO_MODE_XGA        , 800,  600 },
	{ AMBA_VIDEO_MODE_WXGA       ,1280,  800 },
	{ AMBA_VIDEO_MODE_UXGA       ,1600, 1200 },
	{ AMBA_VIDEO_MODE_QSXGA      ,2592, 1944 },
	{ AMBA_VIDEO_MODE_QXGA       ,2048, 1536 },
	{ AMBA_VIDEO_MODE_QSXGA      ,2592, 1944 },
	{ AMBA_VIDEO_MODE_240_400    , 240,  400 },
	{ AMBA_VIDEO_MODE_320_240    , 320,  240 },
	{ AMBA_VIDEO_MODE_320_288    , 320,  288 },
	{ AMBA_VIDEO_MODE_360_240    , 360,  240 },
	{ AMBA_VIDEO_MODE_360_288    , 360,  288 },
	{ AMBA_VIDEO_MODE_480_640    , 480,  640 },
	{ AMBA_VIDEO_MODE_480_800    , 480,  800 },
	{ AMBA_VIDEO_MODE_960_240    , 960,  240 },
	{ AMBA_VIDEO_MODE_960_540    , 960,  540 },
	{ AMBA_VIDEO_MODE_XGA        ,1024,  768 },
	{ AMBA_VIDEO_MODE_720P       ,1280,  720 },
	{ AMBA_VIDEO_MODE_1280_960   ,1280,  960 },
	{ AMBA_VIDEO_MODE_SXGA       ,1280, 1024 },
	{ AMBA_VIDEO_MODE_UXGA       ,1600, 1200 },
	{ AMBA_VIDEO_MODE_1920_1440  ,1920, 1440 },
	{ AMBA_VIDEO_MODE_1080P      ,1920, 1080 },
	{ AMBA_VIDEO_MODE_2048_1152  ,2048, 1152 },
	{ AMBA_VIDEO_MODE_QXGA       ,2048, 1536 },
	{ AMBA_VIDEO_MODE_QSXGA      ,2592, 1944 },
};

int fd_iav = -1;

static pthread_t show_histogram_task_id;
static int show_histogram_task_exit_flag = 0;
fb_show_histogram_t hist_data;
static int get_data_method_flag = 0;
static int enable_fb_show_histogram = 0;

static int get_fb_config_size(int fd, fb_vin_vout_size_t *pFb_size)
{
	struct iav_srcbuf_setup buffer_setup;
	struct iav_system_resource resource_setup;

	if (ioctl(fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &buffer_setup) < 0) {
		perror("ioctl IAV_IOC_GET_SOURCE_BUFFER_SETUP error");
		return -1;
	}

	resource_setup.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd, IAV_IOC_GET_SYSTEM_RESOURCE, &resource_setup) < 0) {
		perror("ioctl IAV_IOC_GET_SYSTEM_RESOURCE error");
		return -1;
	}

	if (buffer_setup.type[IAV_SRCBUF_MN] ==
		IAV_SRCBUF_TYPE_ENCODE) {
		pFb_size->vin_width =   buffer_setup.size[IAV_SRCBUF_MN].width;
		pFb_size->vin_height =  buffer_setup.size[IAV_SRCBUF_MN].height;
	}

	if (resource_setup.encode_mode == DSP_SINGLE_REGION_WARP_MODE) {
		pFb_size->vin_width =  buffer_setup.size[IAV_SRCBUF_PMN].width;
		pFb_size->vin_height =  buffer_setup.size[IAV_SRCBUF_PMN].height;
	}

	int ret = 0;
	if (buffer_setup.type[IAV_SRCBUF_PB] ==
		IAV_SRCBUF_TYPE_PREVIEW) {
		int num = 0;
		struct amba_vout_sink_info vout_info;
		enum amba_video_mode mode;
		int i = 0;
		memset(&vout_info, 0, sizeof(vout_info));
		if (ioctl(fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
			perror("ioctl IAV_IOC_VOUT_GET_SINK_NUM error");
			return -1;
		}
		for (i = 0; i < num; i++) {
			vout_info.id = i;
			if (ioctl(fd, IAV_IOC_VOUT_GET_SINK_INFO, &vout_info) < 0) {
				perror("ioctl IAV_IOC_VOUT_GET_SINK_INFO error");
				ret = -1;
				break;
			}
			if (vout_info.sink_type == AMBA_VOUT_SINK_TYPE_HDMI) {
				if (vout_info.hdmi_plug) {
					mode = vout_info.hdmi_native_mode;
					int j = 0;
					for (j = 0;
					     j < (sizeof(video_modes_info)/sizeof(video_modes_info[0]));
					     j++) {
						if (mode == video_modes_info[j].mode) {
							pFb_size->vout_width = video_modes_info[j].width;
							pFb_size->vout_height = video_modes_info[j].height;
							ret = 0;
							break;
						}
					}
				} else {
					PRINT_ERROR("HDMI not plugged, check your TV status\n");
					ret = -1;
					break;
				}
			}
		}
	} else {
		PRINT_ERROR("no preview for frame buffer\n");
		ret = -1;
	}

	if (ret != 0) {
		PRINT_ERROR("could not determine vout size\n");
	}
	return ret;
}

static int config_fb_vout(int fd)
{
	iav_vout_fb_sel_t fb_sel;
	fb_sel.vout_id = 1;
	fb_sel.fb_id = 0;
	if (ioctl(fd, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
		PRINT_ERROR("Change osd of vout1");
		return -1;
	}
	return 0;
}

static int show_af_hist_data(af_stat_t * af_data, int af_col, int af_row)
{
	int bin_num;
	int i, j, max_value = 0;

	hist_data.row_num = af_row;
	hist_data.col_num = af_col;
	hist_data.total_num = af_col * af_row;
	for (bin_num = 0; bin_num < hist_data.total_num; ++bin_num) {
		switch(hist_data.hist_type) {
		case AF_FY:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fy;
			break;
		case AF_FV1:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv1;
			break;
		case AF_FV2:
		default:
			hist_data.histogram_value[bin_num] = af_data[bin_num].sum_fv2;
			break;
		}
	}

	for (i = 0; i < af_row; ++i) {
		for (j = 0; j< af_col; ++j) {
			PRINT_INFO("%6d ", hist_data.histogram_value[i * af_col + j]);
			if (max_value < hist_data.histogram_value[i * af_col + j]) {
				max_value = hist_data.histogram_value[i * af_col + j];
			}
		}
		PRINT_INFO("\n");
	}
	PRINT_INFO("\n--------------\n");

	hist_data.the_max = max_value;

	if (enable_fb_show_histogram) {
		draw_histogram_plane_figure(&hist_data);
	}

	return 0;
}

static int curr_expo_num = 1;
static int curr_expo_idx = 0;
#define SHM_DATA_SZ (curr_expo_num*sizeof(img_aaa_stat_t))
#define DEFAULT_KEY 5678
static int shmid = -1;
static char *shm;
static struct shmid_ds shmid_ds;
static inline int get_aaa_shm()
{
	if (shmid >= 0) {
		return 0;
	}

  shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666);
	if (shmid < 0) {
		perror("shmget error");
		return -1;
	}

	shm = (char *) shmat(shmid, NULL, 0);
	if (shm == (char *) -1) {
		perror("shmat error");
		return -1;
	}
	return 0;
}
static inline int release_aaa_shm()
{
	if (shmdt(shm) < 0) {
		perror("shmdt error");
		return -1;
	}
	shmid = -1;
	shm = NULL;
	return 0;
}
static int get_aaa_stats(img_aaa_stat_t* p, int curr_idx)
{
	if (p == NULL) {
		printf("Gotcha, NULL pointer at %s:%d\n",
					 __func__, __LINE__);
		return -1;
	}
	if (shmctl(shmid, IPC_STAT, &shmid_ds) == -1) {
		perror("shmctl IPC_STAT");
		return -1;
	}
	if (shmid_ds.shm_perm.mode & SHM_DEST) {
		perror("3A is off");
		return -1;
	}

	do {
		if (shmid_ds.shm_perm.mode & SHM_LOCKED) {
			printf("shared memory is locked, retry in 100ms\n");
			usleep(100*1000);
			continue;
		}
		break;
	} while (1);

	if (shmctl(shmid, SHM_LOCK, (struct shmid_ds *)NULL) == -1) {
		perror("shmctl SHM_LOCK");
		return -1;
	}
	memcpy(p, (shm + curr_idx * sizeof(img_aaa_stat_t)),
				 sizeof(img_aaa_stat_t));

	if (shmctl(shmid, SHM_UNLOCK, (struct shmid_ds *)NULL) == -1) {
    perror("shmctl SHM_UNLOCK");
    return -1;
	}

	return 0;
}
void show_histogram_task(void *arg)
{
	int retv = 0;
	img_aaa_stat_t img_aaa_stats;
	u16 af_tile_col, af_tile_row;
	af_stat_t *const p_af_stat = img_aaa_stats.af_info;

	if (get_aaa_shm() < 0) {
		return;
	}

	do {
		if (get_aaa_stats(&img_aaa_stats, curr_expo_idx) < 0) {
			break;
		}
		af_tile_col = img_aaa_stats.tile_info.af_tile_num_col;
		af_tile_row = img_aaa_stats.tile_info.af_tile_num_row;
		PRINT_INFO("\nAF tils [row =%d, colume = %d ] \n",
							 af_tile_col, af_tile_row);

		retv = show_af_hist_data(p_af_stat, af_tile_col, af_tile_row);

		if (retv) {
			PRINT_ERROR("display error\n");
			break;
		}
		usleep (500000);
	} while (!show_histogram_task_exit_flag);

	release_aaa_shm();

	return;
}

void show_histogram_once(void)
{
	int retv = 0;
	u16 af_tile_col, af_tile_row;
	img_aaa_stat_t img_aaa_stats;
	af_stat_t *const p_af_stat = img_aaa_stats.af_info;
	char ch = 1;

	if (get_aaa_shm() < 0) {
		return;
	}

	while (ch) {
		if (get_aaa_stats(&img_aaa_stats, curr_expo_idx) < 0) {
			break;
		}

		af_tile_col = img_aaa_stats.tile_info.af_tile_num_col;
		af_tile_row = img_aaa_stats.tile_info.af_tile_num_row;
		PRINT_INFO("\nAF tils [row =%d, colume = %d ] \n",
								af_tile_col, af_tile_row);

		retv = show_af_hist_data(p_af_stat, af_tile_col, af_tile_row);

		if (retv) {
			PRINT_ERROR("display error\n");
			break;
		}
		printf("next time('q' = exit & return to upper level)>> \n ");
		ch = getchar();
		if (ch == 'q') {
			break;
		}
	}

	release_aaa_shm();

	return;
}

static int setting_param(fb_show_histogram_t *pHistogram)
{
	if (enable_fb_show_histogram) {
		fb_vin_vout_size_t fbsize;

		if (get_fb_config_size(fd_iav, &fbsize) < 0) {
			PRINT_MSG("Error:get_fb_config_size\n");
			return -1;
		}
		if (init_fb(&fbsize) < 0) {
			PRINT_MSG("frame buffer doesn't work successfully, please check the steps\n");
			PRINT_MSG("Example:\n");
			PRINT_MSG("\tcheck whether frame buffer is malloced\n");
			PRINT_MSG("\t\tnandwrite --show_info | grep cmdline\n");
			PRINT_MSG("\tconfirm ambarella_fb module has installed\n");
			PRINT_MSG("\t\tmodprobe ambarella_fb\n");
			return -1;
		}
		if(config_fb_vout(fd_iav) < 0) {
			printf("Error:config_fb_vout\n");
			return -1;
		}
	}
	hist_data.statist_type = pHistogram->statist_type;
	hist_data.hist_type = pHistogram ->hist_type;

	return 0;
}

static int create_show_histogram_task(fb_show_histogram_t *pHistogram)
{
	show_histogram_task_exit_flag = 0;

	if (setting_param(pHistogram) < 0) {
		PRINT_ERROR("Setting_param error!\n");
		return -1;
	}

	if (show_histogram_task_id == 0) {
		if (pthread_create(&show_histogram_task_id, NULL, (void *)show_histogram_task, NULL)
			!= 0) {
			PRINT_ERROR("Failed. Cant create thread <show_histogram_task>!\n");
			return -1;
		}
	}
	return 0;
}

static int destroy_show_histogram_task(void)
{
	show_histogram_task_exit_flag = 1;
	if (show_histogram_task_id != 0) {
		if (pthread_join(show_histogram_task_id, NULL) != 0) {
			PRINT_ERROR("Failed. Cant destroy thread <show_histogram_task>!\n");
		}
		PRINT_MSG("Destroy thread <show_histogram_task> successful !\n");
	}
	show_histogram_task_exit_flag = 0;
	show_histogram_task_id = 0;
	deinit_fb();

	return 0;
}

static int active_histogram(fb_show_histogram_t *pHistogram)
{

	 if (show_histogram_task_id) {
		if (destroy_show_histogram_task() < 0)  {
			printf("Error: destroy_show_histogram_task\n");
			return -1;
		}
	}

	if (get_data_method_flag) {
		if (setting_param(pHistogram) < 0) {
			PRINT_ERROR("Setting_param error!\n");
			return -1;
		}
		show_histogram_once();
	} else if (create_show_histogram_task(pHistogram) < 0) {
		printf("Error:create_show_histogram_task\n> ");
		show_histogram_task_id = 0;
		return -1;
	}

	return 0;

}

static int show_histogram_menu()
{
	printf("\n================ Histogram Settings ================\n");
	printf("  m -- Get statistics data method\n");
	printf("  f -- AF histogram setting\n");
	printf("  e -- Show histogram with fb\n");
	printf("  q -- Return to upper level\n");
	printf("\n================================================\n\n");
	printf("d > ");
	return 0;
}

static int histogram_setting(void)
{
	int i, exit_flag, error_opt;
	char ch;
	fb_show_histogram_t hist;

	show_histogram_menu();
	ch = getchar();
	while (ch) {
		exit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'e':
			printf("Show Histogram with fb: 0 - disable  1 - enable\n> ");
			scanf("%d", &i);
			if ( i > 1 || i < 0) {
				printf("the value must be 0|1\n> ");
				break;
			}
			if (show_histogram_task_id > 0) {
				destroy_show_histogram_task();
			}
			enable_fb_show_histogram = i;
			break;
		case 'm':
			PRINT_MSG("get statistics dat method:(0-auto; 1-press key)\n> ");
			scanf("%d", &i);
			if ( i < 0 || i> 1) {
				PRINT_ERROR("The value must be 0|1\n ");
			}
			get_data_method_flag = i;
			break;
		case 'f':
			printf("Select af histogram type: 0 - CFA:fy, 1 - CFA:fv1, 2 - CFA:fv2,\n");
			printf("\t\t\t  3 - RGB:fy, 4 - RGB:fv1, 5 - RGB:fv2\n> ");
			scanf("%d", &i);
			if (i < 0 || i > 5) {
				printf("Error:The value must be 0~5\n> ");
				break;
			}
			hist.statist_type = i / 3;
			hist.hist_type = i % 3;
			active_histogram(&hist);
			break;
		case 'q':
			exit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}
		if (exit_flag)
			break;
		if (error_opt == 0) {
			show_histogram_menu();
		}
		ch = getchar();
	}
	return 0;
}


int set_log_level(int log)
{
	if (log < ERROR_LEVEL || log >= (LOG_LEVEL_NUM - 1)) {
		PRINT_ERROR("Invalid log level, please set it in the range of (0~%d).\n",
			(LOG_LEVEL_NUM - 1));
		return -1;
	}
	log_level = log + 1;
	return 0;
}

static void sigstop(int signo)
{
	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
	}
	close(fd_iav);
	exit(1);
}

static int show_menu(void)
{
	PRINT_MSG("\n================================================\n");
	PRINT_MSG("  h -- Histogram menu \n");
	PRINT_MSG("  v -- Set log level\n");
	PRINT_MSG("  q -- Quit");
	PRINT_MSG("\n================================================\n\n");
	PRINT_MSG("> ");
	return 0;
}

int main(int argc, char ** argv)
{
	char ch, error_opt, quit_flag;
	int i;

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		PRINT_ERROR("open /dev/iav");
		return -1;
	}

	show_menu();
	ch = getchar();
	while (ch) {
		quit_flag = 0;
		error_opt = 0;
		switch (ch) {
		case 'h':
			histogram_setting();
			break;
		case 'v':
			PRINT_MSG("Input log level: (0~%d)\n> ", LOG_LEVEL_NUM - 2);
			scanf("%d", &i);
			set_log_level(i);
			break;
		case 'q':
			quit_flag = 1;
			break;
		default:
			error_opt = 1;
			break;
		}

		if (quit_flag)
			break;
		if (error_opt == 0) {
			show_menu();
		}
		ch = getchar();
	}

	if (show_histogram_task_id > 0) {
		destroy_show_histogram_task();
	}
	close(fd_iav);

	return 0;
}

