/*
 *
 * mw_md.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <basetypes.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "iav_common.h"

#include "mw_defines.h"
#include "mw_struct.h"
#include "mw_api.h"

#define MAX_ROW					8
#define MAX_COLUMN				12
#define MAX_TILE_NUM			(MAX_ROW * MAX_COLUMN)
#define MAX_MOTION_STAT_NUM		900	 // capable of motion statistics info based on @30fps * 30sec

#define MAX_MOTION_INDICATOR	1000000
#define LUMA_DIFF_THRESHOLD 	300

#define DEFAULT_KEY	 5678	// share memory key number, don't modify it

typedef struct aaa_ae_data_s {
	u32 lin_y;
	u32 non_lin_y;
} aaa_ae_data_t;

struct shmid_ds shmid_ds;

aaa_ae_data_t ae_luma_buf[2][MAX_TILE_NUM];
static int buf_id = 0;
static int lock_id;
#define SHM_DATA_SZ	 (sizeof(aaa_ae_data_t) * MAX_TILE_NUM)


typedef struct region_s {
	u32 x;
	u32 y;
	u32 width;
	u32 height;
} region_t;

typedef struct roi_s {
	region_t area;
	u16 tiles[MAX_ROW][MAX_COLUMN];
	u16 tiles_num;
	u16 cur_buf;
	u16 luma_diff_threshold;
	u16 sensitivity;
	u16 update_freq;
	u16 update_cnt;

	u32 luma_mse_threshold;
	u8 valid;
} roi_t;

typedef struct motion_statistics_s {
	u32 pts;
	u32 indicator[MW_MAX_ROI_NUM];
} motion_statistics_t;

typedef struct motion_stat_queue_s {
	motion_statistics_t motion_stat[MAX_MOTION_STAT_NUM];
	u32 front;
	u32 rear;
} motion_stat_queue_t;

static int motion_status[MW_MAX_ROI_NUM] = {
	NO_MOTION,
	NO_MOTION,
	NO_MOTION,
	NO_MOTION,
};

static int motion_event[MW_MAX_ROI_NUM] = {
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
};

static int motion_event_buffer[MW_MAX_ROI_NUM] = {
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
	EVENT_NO_MOTION,
};

static roi_t roi[MW_MAX_ROI_NUM];
motion_stat_queue_t motion_stat_queue;

static pthread_t thread_id;
static pthread_mutex_t roi_lock			 = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t motion_record_lock   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t motion_event_lock	= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t motion_event_cond	= PTHREAD_COND_INITIALIZER;

static alarm_handle_func alarm_handler;
static int total_event = 0;
static int done_flag = 0;
int mw_md_print_event_flag = 0;

static int shmid = 0;
static char *shm = NULL;

static int init_shm_get(void)
{
	/* Locate the segment.*/
	if ((shmid = shmget(DEFAULT_KEY, SHM_DATA_SZ, 0666)) < 0) {
		perror("3A is off!\n");
		perror("shmget");
		exit(1);
	}

	return 0;
}

static int get_shm_aedata(void)
{
	if ((shm = (char*)shmat(shmid, NULL, 0)) == (char*) - 1) {
		perror("shmat");
		exit(1);
	}
	return 0;
}

static int default_handler(const int *p_motion_event)
{
	int i;
	for (i = 0; i < MW_MAX_ROI_NUM; i++) {
		if (p_motion_event[i] == EVENT_MOTION_START)
			MW_MSG("ROI#%d Motion started!\n", i);
		else if (p_motion_event[i] == EVENT_MOTION_END)
			MW_MSG("ROI#%d Motion ended.\n", i);
	}
	return 0;
}

static void motion_stat_queue_init(void)
{
	motion_stat_queue.front = 0;
	motion_stat_queue.rear = 0;
}

static int put_motion_record(const motion_statistics_t *p_motion_record)
{
	int errno;
	if ((errno =pthread_mutex_lock(&motion_record_lock)) != 0)
		return errno;

	if (motion_stat_queue.front == (motion_stat_queue.rear + 1) % MAX_MOTION_STAT_NUM)	  // event queue is full
		motion_stat_queue.front = (motion_stat_queue.front + 1) % MAX_MOTION_STAT_NUM;	  // discard the oldest one element

	memcpy(&motion_stat_queue.motion_stat[motion_stat_queue.rear], p_motion_record, sizeof(motion_statistics_t));

	motion_stat_queue.rear = (motion_stat_queue.rear + 1) % MAX_MOTION_STAT_NUM;

	return pthread_mutex_unlock(&motion_record_lock);
}

static int put_motion_event(const int *p_motion_event)
{
	int errno;
	if (( errno = pthread_mutex_lock(&motion_event_lock)) != 0)
		return errno;

	memcpy(&motion_event_buffer[0], p_motion_event, sizeof(motion_event_buffer));
	total_event = 1;		// keep only one latest event

	if ((errno = pthread_cond_signal(&motion_event_cond)) != 0) {
		pthread_mutex_unlock(&motion_event_lock);
		return errno;
	}

	return pthread_mutex_unlock(&motion_event_lock);
}

int mw_md_get_motion_event(int *p_motion_event)
{
	int errno;
	if ((errno = pthread_mutex_lock(&motion_event_lock)) != 0)
		return errno;

	while ((total_event <=0 ) && !errno && !done_flag) {
		errno = pthread_cond_wait(&motion_event_cond, &motion_event_lock);
	}

	if (errno) {
		pthread_mutex_unlock(&motion_event_lock);
		return errno;
	}

	if (done_flag && (total_event <=0)) {
		pthread_mutex_unlock(&motion_event_lock);
		return ECANCELED;
	}

	memcpy(p_motion_event, &motion_event_buffer[0], sizeof(motion_event_buffer));
	total_event--;

	return pthread_mutex_unlock(&motion_event_lock);
}

int mw_md_get_motion_event_cancel(void)
{
	int errno1, errno2;

	if ((errno1 = pthread_mutex_lock(&motion_event_lock)) != 0)
		return errno1;

	errno1 = pthread_cond_broadcast(&motion_event_cond);
	errno2 = pthread_mutex_unlock(&motion_event_lock);

	if (errno1)
		return errno1;
	if (errno2)
		return errno2;
	return 0;
}

void mw_md_print_event(int flag)
{
	mw_md_print_event_flag = 1;
}

static int check_trigger(void)
{
	s32 diff_tile;
	u32 index = 0;
	s32 diff_y_total;
	s32 diff_y_avg;
	u32 mse_diff_y;

	s32 diff_y[MAX_TILE_NUM];
	static u32 silent_cnt[MW_MAX_ROI_NUM] = {0};
	int motion_event_flag = 0;
	s16 inc_num = 0;
	int errno;

	motion_statistics_t motion_stat;

	int i, j, k;

	if ((errno = pthread_mutex_lock(&roi_lock)) != 0)
		return errno;

	memset(&motion_stat, 0, sizeof(motion_stat));
	motion_stat.pts = 0;		// reserved
	for (i = 0; i < MW_MAX_ROI_NUM; i++) {
		motion_event[i] = EVENT_NO_MOTION;
		if (roi[i].valid == 0) {
		 //   memcpy(roi[i].ptr_buf[], &ae_cur_y[0], sizeof(roi[i].pre_y));
			continue;
		}

		diff_y_total = 0;
		diff_y_avg = 0;
		mse_diff_y = 0;
		inc_num = 0;

		if (roi[i].update_cnt) {
			roi[i].update_cnt --;
			continue;
		} else {
			roi[i].update_cnt = roi[i].update_freq - 1;
		}

		/* Caculate MSE of luma diff in each ROI */
		for (j = 0; j < MAX_ROW; j++) {
			for (k = 0; k < MAX_COLUMN; k++) {
				if (roi[i].tiles[j][k] == 1) {
					index = j * MAX_COLUMN + k;
					diff_tile = ABS(ae_luma_buf[buf_id][index].lin_y - ae_luma_buf[1 - buf_id][index].lin_y);
					if (diff_tile >= (ae_luma_buf[1 - buf_id][index].lin_y /100 * roi[i].sensitivity)) {
						inc_num++;
					}
					diff_y[index] = diff_tile;
					diff_y_total += diff_tile;
				}
			}
			MW_INFO("\n");
		}

		inc_num = (inc_num > 0 ? inc_num : roi[i].tiles_num);
		diff_y_avg = diff_y_total / roi[i].tiles_num;
		for (j = 0; j < MAX_ROW; j++) {
			for (k = 0; k < MAX_COLUMN; k++) {
				if (roi[i].tiles[j][k] == 1) {
					index = j * MAX_COLUMN + k;
					MW_INFO("%d\t", diff_y[index]);
			   if (diff_y[index])
					mse_diff_y += ((diff_y[index] - diff_y_avg) *(diff_y[index] - diff_y_avg));
				}
			}
			MW_INFO("\n");
		}
		mse_diff_y /= inc_num;
		mse_diff_y /= inc_num;
		MW_INFO("ROI#%d: diff_y_avg = %d,\tinc = %d,\tmse_diff_y = %d,\tthreshold = %d\n",
				i, diff_y_avg, inc_num, mse_diff_y, roi[i].luma_mse_threshold);

		if (mse_diff_y > MAX_MOTION_INDICATOR)
			mse_diff_y = MAX_MOTION_INDICATOR;

		motion_stat.indicator[i] = mse_diff_y;

		if (motion_status[i] == NO_MOTION) {
			if (mse_diff_y > roi[i].luma_mse_threshold) {
				motion_status[i] = IN_MOTION;
				motion_event[i] = EVENT_MOTION_START;
				motion_event_flag = 1;
			}
		} else if (motion_status[i] == IN_MOTION) {
			if (mse_diff_y < roi[i].luma_mse_threshold) {
				silent_cnt[i]++;
				if (silent_cnt[i] == 30) {
					motion_status[i] = NO_MOTION;
					motion_event[i] = EVENT_MOTION_END;
					motion_event_flag = 1;
					silent_cnt[i] = 0;
				}
			} else {
				silent_cnt[i] = 0;
			}
		}
	}

	if ((errno = pthread_mutex_unlock(&roi_lock)) != 0)
		return errno;
	if ((errno = put_motion_record(&motion_stat)) != 0)
		return errno;

	if (motion_event_flag) {
		if (mw_md_print_event_flag)
			alarm_handler(motion_event);
		errno = put_motion_event(motion_event);
		return errno;
	}

	return 0;
}

int mw_md_callback_setup(alarm_handle_func handler)
{
	alarm_handler = handler;
	return 0;
}

int mw_md_get_roi_info(int roi_idx, mw_roi_info *roi_info)
{
	int errno;

	if (roi_idx > MW_MAX_ROI_NUM - 1) {
		MW_MSG("ROI index %d out of range(0~3)\n", roi_idx);
		return -1;
	}

	if ((errno = pthread_mutex_lock(&roi_lock)) != 0)
		return errno;

	roi_info->x = roi[roi_idx].area.x;
	roi_info->y = roi[roi_idx].area.y;
	roi_info->width = roi[roi_idx].area.width;
	roi_info->height = roi[roi_idx].area.height;
	roi_info->sensitivity = roi[roi_idx].sensitivity;
	roi_info->threshold = roi[roi_idx].luma_diff_threshold;
	roi_info->valid = roi[roi_idx].valid;

	if ((errno = pthread_mutex_unlock(&roi_lock)) != 0)
		return errno;

	MW_INFO("ROI#%d(x %d, y %d, width %d, height %d) threshold %d, sensitivity %d, valid %d\n",
			roi_idx, roi_info->x, roi_info->y, roi_info->width, roi_info->height,
			roi_info->threshold, roi_info->sensitivity, roi_info->valid);

	return 0;
}

int mw_md_set_roi_info(int roi_idx, const mw_roi_info *roi_info)
{
	int i, j;
	int threshold;
	int errno;

	MW_INFO("Original ROI#%d\t%d, %d, %d, %d, %d, %d, %d\n",
			roi_idx, roi_info->x, roi_info->y, roi_info->width, roi_info->height,
			roi_info->threshold, roi_info->sensitivity, roi_info->valid);

	if (roi_idx > MW_MAX_ROI_NUM - 1) {
		MW_MSG("ROI index %d out of range(0~3)\n", roi_idx);
		return -1;
	}

	if (roi_info->x + roi_info->width > MAX_COLUMN ||
			roi_info->y + roi_info->height > MAX_ROW) {
		MW_MSG("The ROI's area exceeds the 12*8 tiles.");
		return -1;
	}

	if ((errno = pthread_mutex_lock(&roi_lock)) != 0)
		return errno;

	roi[roi_idx].valid = roi_info->valid;
	roi[roi_idx].area.x = roi_info->x;
	roi[roi_idx].area.y = roi_info->y;
	roi[roi_idx].area.width = roi_info->width;
	roi[roi_idx].area.height = roi_info->height;
	roi[roi_idx].sensitivity = roi_info->sensitivity;
	roi[roi_idx].update_freq = 1;
	roi[roi_idx].update_cnt = roi[roi_idx].update_freq - 1;
	roi[roi_idx].tiles_num=0;

	/* Fill the tiles to be detected */
	for (i = 0; i < roi_info->width; i++) {
		for (j = 0; j < roi_info->height; j++) {
			roi[roi_idx].tiles[j + roi_info->y][i + roi_info->x] = 1;
			roi[roi_idx].tiles_num++;
		}
	}

	threshold = (roi_info->threshold > MW_MAX_MD_THRESHOLD ? MW_MAX_MD_THRESHOLD : roi_info->threshold);
	roi[roi_idx].luma_diff_threshold = threshold;
	roi[roi_idx].luma_mse_threshold = threshold * threshold;
	MW_INFO("ROI#%d valid = %d\n", roi_idx, roi[roi_idx].valid);
	return pthread_mutex_unlock(&roi_lock);
}

int mw_md_clear_roi(int roi_idx)
{
	int errno;
	if ((errno = pthread_mutex_lock(&roi_lock)) != 0)
		return errno;

	if (roi_idx > MW_MAX_ROI_NUM - 1) {
		MW_MSG("ROI index %d out of range [0, 3]\n", roi_idx);
		pthread_mutex_unlock(&roi_lock);
		return -1;
	}

	memset(&roi[roi_idx], 0, sizeof(roi[roi_idx]));
	return pthread_mutex_unlock(&roi_lock);
}

int mw_md_roi_region_display(void)
{
	int i;
	MW_MSG("ROI\tRegion\tDisplay %d * %d", MAX_COLUMN, MAX_ROW);

	for (i = 0; i < MW_MAX_ROI_NUM; i++) {
		if (roi[i].valid) {
			MW_MSG("ROI#%d\t(x %d, y %d, width %d, height %d)\ttiles_num %d\n",
				i, roi[i].area.x, roi[i].area.y, roi[i].area.width,
				roi[i].area.height, roi[i].tiles_num);
		}
	}
	return 0;
}

static int md_init(void)
{
	if (alarm_handler == 0)
		alarm_handler = default_handler;

	motion_stat_queue_init();
	done_flag = 0;
	return 0;
}

static int md_clear(void)
{
	done_flag = 1;
	return pthread_cond_broadcast(&motion_event_cond);
}

static int md_exit(void)
{
	int rtrn;
	md_clear();
	if (lock_id == 1) {
		if ((rtrn = shmctl(shmid , SHM_UNLOCK, (struct shmid_ds *)NULL)) == -1) {
			MW_MSG(" md_exit: shmctl(SHM_UNLOCK) failed!\n");
			return -1;
		} else {
			MW_MSG(" md_exit: shmctl(SHM_UNLOCK) success!\n");
		}
	}

	return 0;
}
static void* md_mainloop()
{
	struct shmid_ds *buf;
	buf = &shmid_ds;

	while (!done_flag) {
		int rtrn;
		usleep(90*1000);

		if ((rtrn = shmctl(shmid, IPC_STAT, buf)) == -1) {
			MW_MSG("shmctl get IPC_STAT for write failed.\n");
			continue;
		}

		if (buf->shm_perm.mode & SHM_DEST) {
			perror("\n\n3A is off.\n");
			exit(1);
		}

		if (buf->shm_perm.mode & SHM_LOCKED) {
			usleep(10*1000);
			continue;
		}
		if ((rtrn = shmctl(shmid, SHM_LOCK, (struct shmid_ds *)NULL)) == -1) {
			MW_MSG("\nThe shmctl call LOCK for read failed.\n");
		}

		lock_id = 1;
		buf_id = (buf_id ? 0 : 1);
		memcpy(&ae_luma_buf[buf_id][0], shm, SHM_DATA_SZ);

		if ((rtrn = shmctl(shmid, SHM_UNLOCK, (struct shmid_ds *)NULL)) == -1) {
			MW_MSG("\nThe shmctl UNLOCK for read failed.\n");
		}
		lock_id = 0;
		check_trigger();
	}
	return NULL;
}

int mw_md_thread_start(void)
{
	int rval;
	fflush(0);
	init_shm_get();
	get_shm_aedata();
	md_init();
	if (thread_id == 0) {
		rval = pthread_create(&thread_id, NULL, md_mainloop, NULL);

		if (rval != 0) {
			perror("Create md pthread error!\n");
			return -1;
		}
	}
	fflush(0);
	return 0;
}

int mw_md_thread_stop(void)
{
	md_exit();
	return 0;
}


#define __END_OF_FILE__

