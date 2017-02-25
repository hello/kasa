/*******************************************************************************
 * md_motbuf.c
 *
 * History:
 *	2013/05/22 - [Jian Tang] created file
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
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <basetypes.h>

#include "iav_ioctl.h"
#include "md_motbuf.h"

typedef struct tagRGBQUAD {
	char rgbBlue;
	char rgbGreen;
	char rgbRed;
	char rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
	u32 biSize;
	long biWidth;
	long biHeight;
	u16 biPlanes;
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
} __attribute__((packed)) BITMAPINFO;

typedef struct tagBITMAPFILEHEADER {
	u16 bfType;
	u32 bfSize;
	u16 bfReserved1;
	u16 bfReserved2;
	u32 bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

u32 fd_iav;
u8 *dsp_mem;
u32 dsp_size;
u32 mem_mapped = 0;

static md_motbuf_roi_t roi_rect[MD_MAX_ROI_N];
static u32 buf_pitch, buf_width, buf_height;
u8 *me_y_buffer[2] = {NULL, NULL};
static int cur_buf_id = 0;
static int md_motbuf_exit_flag = 0;
static u32 total_event = 0;
static u32 done_flag = 0;

static md_motbuf_init_t md_me_buffer_init = {
	.me_buffer = 0,
	.frame_interval =1,
	.pixel_line_interval = 2,
	.motion_end_num = 30,
};

static md_motbuf_evt_t md_motbuf_event[MD_MAX_ROI_N], md_motbuf_evtbuf[MD_MAX_ROI_N];
static md_status roi_md_status[MD_MAX_ROI_N];

static pthread_t thread_id;
static pthread_mutex_t md_motbuf_roi_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t md_motbuf_evt_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t md_motbuf_data_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t md_motbuf_evt_cond = PTHREAD_COND_INITIALIZER;

static md_motbuf_callback_func md_motbuf_callback;
static md_motbuf_algo_func md_motbuf_algo;

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	} else {
		mem_mapped = 1;
	}

	return 0;
}

static int unmap_buffer(void)
{
	if (!mem_mapped) {
		return 0;
	}
	if (munmap(dsp_mem, dsp_size) < 0) {
		printf("munmap failed!\n");
		return -1;
	} else {
		mem_mapped = 0;
	}

	return 0;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_STATE");
		exit(2);
	}

	if ((state != IAV_STATE_PREVIEW ) && (state != IAV_STATE_ENCODING)) {
		printf("md_motbuf lib requires iav in PREVIEW or ENCODE status.\n");
		return -1;
	}
	return 0;
}

static int set_realtime_schedule(pthread_t tid)
{
	struct sched_param param;
	int policy = SCHED_FIFO;
	int priority = 80;
	if (!tid)
		return -1;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;
	if (pthread_setschedparam(tid, policy, &param) < 0)
		perror("pthread_setschedparam");
	pthread_getschedparam(tid, &policy, &param);
	if (param.sched_priority != priority)
		return -1;

	return 0;
}

static u32 algo_framediff_default(md_motbuf_roi_t *roi, u8* cur_data, u8* pre_data)
{
	u16 i, j;
	u32 diff;

	u32 sum_diff = 0, sum_num = 0, index;
	u16 roi_width = roi->roi_location.width;
	u16 roi_height = roi->roi_location.height/md_me_buffer_init.pixel_line_interval;
	for (i = 0; i < roi_height; i ++) {
		for (j = 0; j < roi_width; j ++) {
			index = buf_pitch * (roi->roi_location.y
			                        + i * md_me_buffer_init.pixel_line_interval)
			            + roi->roi_location.x + j;
			diff = abs(cur_data[index] - pre_data[index]);
			sum_num ++;
			sum_diff += diff;
		}
	}
	if (sum_diff > (u32)(MD_SENSITIVITY_LEVEL-roi->sensitivity)*sum_num/10) {
		roi->motion_flag = MOT_HAPPEN;
	}
	else {
		roi->motion_flag = MOT_NONE;
	}

	return sum_diff;
}

static int put_event(u8 *md_event)
{
	u32 error;

	if ((error = pthread_mutex_lock(&md_motbuf_evt_lock)) != 0)
		return error;

	memcpy(&md_motbuf_evtbuf, md_event, sizeof(md_motbuf_event));
	total_event = 1;

	if ((error = pthread_cond_signal(&md_motbuf_evt_cond)) != 0) {
		pthread_mutex_unlock(&md_motbuf_evt_lock);
		return error;
	}

	return pthread_mutex_unlock(&md_motbuf_evt_lock);
}

static int save_me1_luma_buffer(u8* output, struct iav_mebufdesc *me1_desc)
{
	int i;
	u8 *in;
	u8 *out;

	if (me1_desc->pitch < me1_desc->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	if (me1_desc->pitch == me1_desc->width) {
		memcpy(output, dsp_mem + me1_desc->data_addr_offset,
			me1_desc->width * me1_desc->height);
	} else {
		in = dsp_mem + me1_desc->data_addr_offset;
		out = output;
		for (i = 0; i < me1_desc->height; i++) { //row
			memcpy(out, in, me1_desc->width);
			in += me1_desc->pitch;
			out += me1_desc->width;
		}
	}

	return 0;
}

static void *md_motbuf_mainloop(void *arg)
{
	int error, total_valid = 0;
	struct iav_mebufdesc me1_desc;
	struct iav_querydesc query_desc;
	struct iav_srcbuf_format buf_format;
	u32	i;
	u8 *cur_data = NULL, *pre_data = NULL;
	int md_evt_flag = 0;
	int sil_cnt[MD_MAX_ROI_N];
	md_evt_type hist_evt[MD_MAX_ROI_N];
	memset(sil_cnt, 0, sizeof(sil_cnt));
	memset(hist_evt, MOT_NO_EVENT, sizeof(hist_evt));

	for (i=0; i<MD_MAX_ROI_N; i++) {
		total_valid = total_valid || roi_rect[i].valid;
	}
	if (total_valid == 0) {
		printf("Please set at least one valid ROI!\n");
	}

	buf_format.buf_id = md_me_buffer_init.me_buffer;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return NULL;
	}
	memset(&query_desc, 0, sizeof(query_desc));
	query_desc.qid = IAV_DESC_ME1;
	query_desc.arg.me1.buf_id = md_me_buffer_init.me_buffer;
	query_desc.arg.me1.flag &= ~IAV_BUFCAP_NONBLOCK;
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return NULL;
	}

	// Save the first frame for later diff computation
	if ((error = pthread_mutex_lock(&md_motbuf_data_lock)) != 0)
		return NULL;

	me1_desc = query_desc.arg.me1;
	if ((u8 *)me1_desc.data_addr_offset == NULL) {
		printf("ME1 buffer [%d] address is NULL!\n", me1_desc.data_addr_offset);
	}
	save_me1_luma_buffer(me_y_buffer[cur_buf_id], &me1_desc);
	cur_buf_id = (!cur_buf_id);
	if ((error = pthread_mutex_unlock(&md_motbuf_data_lock)) != 0)
		return NULL;

	while (md_motbuf_exit_flag == 0) {
		i=0;
		while (i++ <= md_me_buffer_init.frame_interval) {
			if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
				perror("IAV_IOC_QUERY_DESC");
				return NULL;
			}
		}
		me1_desc = query_desc.arg.me1;
		if ((u8 *)me1_desc.data_addr_offset == NULL) {
			printf("ME1 buffer [%d] address is NULL! Skip to next!\n",
			       me1_desc.data_addr_offset);
			continue;
		}
		save_me1_luma_buffer(me_y_buffer[cur_buf_id], &me1_desc);
		if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
			return NULL;

		for (i=0; i<MD_MAX_ROI_N; i++) {

			cur_data = me_y_buffer[cur_buf_id];
			pre_data = me_y_buffer[!cur_buf_id];
			if (roi_rect[i].valid == 1) {
				md_motbuf_event[i].diff_value =
							md_motbuf_algo(&roi_rect[i], cur_data, pre_data);
			}
			//printf("RIO[%d]: diff_value is %d\n", i, md_motbuf_event[i].diff_value);
		}

		cur_buf_id = (!cur_buf_id);
		for (i=0; i<MD_MAX_ROI_N; i++) {
			md_motbuf_event[i].motion_flag = hist_evt[i];
			md_motbuf_event[i].x = roi_rect[i].roi_location.x;
			md_motbuf_event[i].y = roi_rect[i].roi_location.y;
			md_motbuf_event[i].width = roi_rect[i].roi_location.width;
			md_motbuf_event[i].height = roi_rect[i].roi_location.height;
			md_motbuf_event[i].valid = 0;
			if (roi_rect[i].valid == 1) {
				md_motbuf_event[i].roi_idx = i;
				if (roi_md_status[i] == MOT_NONE) {
					if (roi_rect[i].motion_flag == 1) {
						roi_md_status[i] = MOT_HAPPEN;
						md_motbuf_event[i].motion_flag = MOT_START;
						md_motbuf_event[i].valid = 1;
						hist_evt[i] = MOT_HAS_STARTED;
						md_evt_flag = 1;
					}
				}
				else if (roi_md_status[i] == MOT_HAPPEN) {
					if (roi_rect[i].motion_flag == 0) {
						sil_cnt[i]++;
						if (sil_cnt[i] == md_me_buffer_init.motion_end_num) {
							roi_md_status[i] = MOT_NONE;
							md_motbuf_event[i].motion_flag = MOT_END;
							md_motbuf_event[i].valid = 1;
							hist_evt[i] = MOT_HAS_ENDED;
							md_evt_flag = 1;
							sil_cnt[i] = 0;
						}
					}
					else {
						sil_cnt[i] = 0;
					}
				}
			}

		}

		if (md_motbuf_callback != NULL)
			md_motbuf_callback((u8 *)&md_motbuf_event);

		// Update event
		if (md_evt_flag) {
			md_evt_flag = 0;
			if ((error = put_event((u8 *)&md_motbuf_event)) != 0) {
				printf("md_motbuf put event error %d\n", error);
				return NULL;
			}
		}

		if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
			return NULL;
	}

	return NULL;
}

/*
 * *****************************************************************
 *
 * This function is used to capture picture in yuv bitmap format.
 *
 ******************************************************************
 */

int md_motbuf_yuv2graybmp(md_motbuf_roi_t roi, char *bmpname, int width, int height)
{
	u8 *pR, *pB, *pG, *pRGBBuf = NULL;
	u8 *pY = me_y_buffer[cur_buf_id];
	int i, j, index, error = -1;
	u32 rgb_size = width*height*3;

	FILE *fp = NULL;
	u16 count = 0;
	BITMAPFILEHEADER bmpHeader;
	BITMAPINFO bmpInfo;

	do {
		pRGBBuf = (u8 *)malloc(rgb_size);
		if (pRGBBuf == NULL ) {
			printf("yuv2graybmp malloc failed.\n");
			error = -1;
			break;
		}
		// Make sure width and height is in unit of 16 pixels
		if (width & 0x3) {
			printf("YUV2GrayBMP bitmap width no align 4\n");
			error = -1;
			break;
		}
		if ((error = pthread_mutex_lock(&md_motbuf_data_lock)) != 0) {
			break;
		}
		for (i = 0; i < height; i ++) {
			pB = pRGBBuf + rgb_size - 3 * width * (i + 1);
			pG = pRGBBuf + rgb_size - 3 * width * (i + 1) + 1;
			pR = pRGBBuf + rgb_size - 3 * width * (i + 1) + 2;

			for (j = 0; j < width; j += 2) {
				index = buf_pitch * (roi.roi_location.y + i) + roi.roi_location.x + j;
				*pR = *pG = *pB = *(pY + index);
				pR += 3;
				pB += 3;
				pG += 3;
				index ++;

				*pR = *pG = *pB = *(pY + index);
				pR += 3;
				pB += 3;
				pG += 3;
			}
		}

		if ((error = pthread_mutex_unlock(&md_motbuf_data_lock)) != 0) {
			break;
		}

		if ((fp = fopen(bmpname, "wb")) == NULL ) {
			printf("YUV2GrayBMP Open file %s failed\n", bmpname);
			error = -1;
			break;
		}

		bmpHeader.bfType = 0x4D42;
		bmpHeader.bfSize = rgb_size + sizeof(BITMAPINFOHEADER)
		            + sizeof(BITMAPFILEHEADER);
		bmpHeader.bfReserved1 = 0;
		bmpHeader.bfReserved2 = 0;
		bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER)
		            + sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = width;
		bmpInfo.bmiHeader.biHeight = height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = 24;
		bmpInfo.bmiHeader.biCompression = 0;
		bmpInfo.bmiHeader.biSizeImage = rgb_size;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biClrUsed = 0;
		bmpInfo.bmiHeader.biClrImportant = 0;

		if ((count = fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp))
		            != sizeof(BITMAPFILEHEADER))
			printf("YUV2GrayBMP write BMP file %s header failed: count = %d\n",
			       bmpname, count);
		if ((count = fwrite(&bmpInfo.bmiHeader, 1, sizeof(BITMAPINFOHEADER), fp))
		            != sizeof(BITMAPINFOHEADER))
			printf("YUV2GrayBMP write BMP file %s info failed: count = %d\n",
			       bmpname, count);

		if (!fwrite(pRGBBuf, 1, rgb_size, fp))
			printf("YUV2GrayBMP write BMP file %s data failed: count = %d\n",
			       bmpname, count);
		error = 0;
	} while (0);

	if (fp) {
		fclose(fp);
	}
	if (pRGBBuf) {
		free(pRGBBuf);
	}

	return error;
}

/*
 * ************************************************************ *
 *  Algorithm mse
 *
 **************************************************************
 */

u32 algo_framediff_mse(md_motbuf_roi_t *roi, u8 *cur_data, u8 *pre_data)
{
	u16 i, j;
	u32 luma_sum = 0, total_num = 0, index;
	u32 diff, diff_sum = 0, diff_avg = 0, diff_num = 0;
	u32 diff_mse = 0, mse_avg = 0;
	u16 roi_width = roi->roi_location.width;
	u16 roi_height = roi->roi_location.height/md_me_buffer_init.pixel_line_interval;

	for (i=0; i<roi_height; i++) {
		for (j=0; j<roi_width; j++) {
			index = buf_pitch * (roi->roi_location.y + i*md_me_buffer_init.pixel_line_interval) +
				roi->roi_location.x + j;
			diff = abs(cur_data[index] - pre_data[index]);
			luma_sum += cur_data[index];
			diff_sum += diff;
			total_num++;
		}
	}

	if (total_num != 0 ) {
		diff_avg = diff_sum / total_num;
		roi->luma_avg = luma_sum / total_num;
		for (i=0; i<roi_height; i++) {
			for (j=0; j<roi_width; j++) {
				index = buf_pitch * (roi->roi_location.y +
						i*md_me_buffer_init.pixel_line_interval) + roi->roi_location.x + j;
				diff = abs(cur_data[index] - pre_data[index]);
				if (diff > (pre_data[index] >>4) ) {
					diff_mse +=abs(diff-diff_avg);
					diff_num++;
				}
			}
		}
		if (diff_num > (total_num >> 4) *
				(MD_SENSITIVITY_LEVEL - roi->sensitivity) /
				MD_SENSITIVITY_LEVEL)
			mse_avg = diff_mse/diff_num;
	}

	/*printf("diff %d, cmp %d, avg %d, sen %d, num %d\n",
	diff_mse ,
	roi->luma_avg * roi->luma_diff_threshold / MD_MAX_THRESHOLD ,
	mse_avg,
	(total_num>>4)*(MD_SENSITIVITY_LEVEL-roi->sensitivity)/MD_SENSITIVITY_LEVEL,
	diff_num);*/

	if (mse_avg > roi->luma_avg*roi->luma_diff_threshold/MD_MAX_THRESHOLD) {
		roi->motion_flag = MOT_HAPPEN;
	}
	else {
		roi->motion_flag = MOT_NONE;
	}

	return diff_mse;
}

static int check_setting(md_motbuf_init_t *md_buf_init)
{
	u32 me_buffer_size;
	int buffer_p,buffer_w, buffer_h;

	if (md_buf_init->me_buffer < ME_MAIN_BUFFER ||
		md_buf_init->me_buffer > ME_TOTAL_BUFFER -1) {
		printf(" the value must be [%d~%d]\n", ME_MAIN_BUFFER, ME_TOTAL_BUFFER -1);
		return -1;
	}
	if (md_buf_init->frame_interval < ME_FRAME_INTERVAL_MIN ||
		md_buf_init->frame_interval > ME_FRAME_INTERVAL_MAX) {
		printf(" the value must be [%d~%d]\n", ME_FRAME_INTERVAL_MIN,
			ME_FRAME_INTERVAL_MAX);
		return -1;
	}
	if (md_buf_init->pixel_line_interval < ME_PIXEL_LINE_INTERVAL_MIN ||
		md_buf_init->pixel_line_interval > ME_PIXEL_LINE_INTERVAL_MAX) {
		printf(" the value must be [%d~%d]\n", ME_PIXEL_LINE_INTERVAL_MIN,
			ME_PIXEL_LINE_INTERVAL_MAX);
		return -1;
	}

	if (md_buf_init->motion_end_num < ME_MOTION_END_NUM_MIN ||
		md_buf_init->motion_end_num > ME_MOTION_END_NUM_MAX) {
		printf(" the value must be [%d~%d]\n", ME_MOTION_END_NUM_MIN,
			ME_MOTION_END_NUM_MAX);
		return -1;
	}

	md_me_buffer_init.me_buffer = md_buf_init->me_buffer;

	if (md_motbuf_get_current_buffer_P_W_H(&buffer_p, &buffer_w, &buffer_h) < 0) {
		printf("Get Motion Buffer pitch, width & height failed.\n");
		return -1;
	}

	printf("Motion Buffer pitch %d, width %d, height %d\n",
			buf_pitch, buf_width, buf_height);

	/*if ((buffer_p & 0xf) != 0 || (buffer_h & 0x7) != 0) {
		printf("ME1 buffer width must be multiple of 16 and height must be multiple of 8\n");
		return -1;
	}*/

	me_buffer_size = buffer_p * buffer_h;

	me_y_buffer[0] = (u8 *)malloc(me_buffer_size * sizeof(u8));
	me_y_buffer[1] = (u8 *)malloc(me_buffer_size * sizeof(u8));

	if (me_y_buffer[0] == NULL || me_y_buffer[1] == NULL) {
		printf("md_motbuf_init not enough memory\n");
		return -1;
	}

	md_me_buffer_init.motion_end_num= md_buf_init->motion_end_num;
	md_me_buffer_init.frame_interval = md_buf_init->frame_interval;
	md_me_buffer_init.pixel_line_interval = md_buf_init->pixel_line_interval;

	return 0;
}

/*
 * ************************************************************ *
 *  Motion Detection based on Motion Buffer APIs
 **************************************************************
 */

int md_motbuf_init(md_motbuf_init_t *md_buf_init)
{
	u32 i;
	md_motbuf_exit_flag = 0;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (map_buffer() < 0)
		return -1;

	if (check_state() < 0)
		return -1;

	if (md_buf_init == NULL) {
		printf("md_motbuf_init pointer is NULL!\n");
		return -1;
	}

	if (check_setting(md_buf_init) < 0)
		return -1;

	for (i=0; i<MD_MAX_ROI_N; i++) {
		memset(&roi_rect[i], 0, sizeof(md_motbuf_roi_t));
		roi_rect[i].valid = 0;
		roi_rect[i].sensitivity = MD_SENSITIVITY_LEVEL/2;
		memset(&md_motbuf_event[i], 0, sizeof(md_motbuf_evt_t));
		memset(&md_motbuf_evtbuf[i], 0, sizeof(md_motbuf_evt_t));
		roi_md_status[i] = MOT_NONE;
	}

	md_motbuf_algo = algo_framediff_default;
	return 0;
}

int md_motbuf_deinit(void)
{
	int i;
	for (i=0; i<2; i++) {
		if (me_y_buffer[i] != NULL) {
			free(me_y_buffer[i]);
			me_y_buffer[i] = NULL;
		}
	}
	unmap_buffer();
	close(fd_iav);
	return 0;
}

int md_motbuf_callback_setup(md_motbuf_callback_func handler)
{
	md_motbuf_callback = handler;
	return 0;
}

int md_motbuf_algo_setup(md_motbuf_algo_func handler)
{
	md_motbuf_algo = handler;
	return 0;
}

int md_motbuf_get_current_buffer_P_W_H(int *buffer_pitch, int *buffer_width, int *buffer_height)
{
	struct iav_srcbuf_format buf_format;
	struct iav_querydesc query_desc;
	struct iav_me_cap *me1_cap;

	memset(&buf_format, 0, sizeof(buf_format));
	buf_format.buf_id = md_me_buffer_init.me_buffer;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return -1;
	}
	memset(&query_desc, 0, sizeof(query_desc));
	query_desc.qid = IAV_DESC_BUFCAP;
	query_desc.arg.bufcap.flag |= IAV_BUFCAP_NONBLOCK;
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}
	me1_cap = &query_desc.arg.bufcap.me1[md_me_buffer_init.me_buffer];

	buf_pitch = *buffer_pitch = me1_cap->pitch;
	buf_width = *buffer_width = me1_cap->width;
	buf_height = *buffer_height = me1_cap->height;

	return 0;
}

int md_motbuf_set_roi_location(u32 roi_idx,
		const md_motbuf_roi_location_t *roi_location)
{
	int error;

	if (roi_idx > MD_MAX_ROI_N-1 || roi_idx < 0) {
		printf("ROI index %d out of range [0~%d]\n",
				roi_idx, MD_MAX_ROI_N-1);
		return -1;
	}

	if (roi_location->x > buf_width-1 || roi_location->x < 0) {
		printf("md_motbuf_set_roi_location ROI#%d x %u is out of range 0~%u.\n",
				roi_idx, roi_location->x, buf_width-1);
		return -1;
	}

	if (roi_location->y > buf_height-1 || roi_location->y < 0) {
		printf("md_motbuf_set_roi_location ROI#%d y %u is out of range 0~%u.\n",
				roi_idx, roi_location->y, buf_height-1);
		return -1;
	}

	if ((roi_location->width+roi_location->x) > buf_width-1) {
		printf("md_motbuf_set_roi_location ROI#%d width %u is out of range 0~%u.\n",
				roi_idx, roi_location->width, buf_width-roi_location->x-1);
		return -1;
	}

	if ((roi_location->height+roi_location->y) > buf_height-1) {
		printf("md_motbuf_set_roi_location ROI#%d height %u is out of range 0~%u.\n",
				roi_idx, roi_location->height, buf_height-roi_location->y-1);
		return -1;
	}

	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;


	memcpy(&roi_rect[roi_idx].roi_location,roi_location,
			sizeof(md_motbuf_roi_location_t));
	roi_rect[roi_idx].roi_update_flag = 1;


	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_get_roi_info(u32 roi_idx, md_motbuf_roi_t *roi_info)
{
	int error;
	if (roi_idx > MD_MAX_ROI_N-1 || roi_idx < 0) {
		printf("ROI index %d out of range [0~%d]\n",
				roi_idx, MD_MAX_ROI_N-1);
		return -1;
	}
	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;
	memcpy(roi_info, &roi_rect[roi_idx], sizeof(md_motbuf_roi_t));
	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_set_roi_sensitivity(u32 roi_idx, u16 sensitivity)
{
	int error;

	if (roi_idx > MD_MAX_ROI_N-1 || roi_idx < 0) {
		printf("ROI index %d out of range [0~%d]\n",
				roi_idx, MD_MAX_ROI_N-1);
		return -1;
	}

	if (sensitivity < 0 || sensitivity > MD_SENSITIVITY_LEVEL-1) {
		printf("md_motbuf_set_roi_sensitivity ROI#%d sensitivity %d is out of range 0~%d\n",
				roi_idx, sensitivity, MD_SENSITIVITY_LEVEL-1);
		return -1;
	}

	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;

	roi_rect[roi_idx].sensitivity = sensitivity;

	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_set_roi_threshold(u32 roi_idx, u32 threshold)
{
	int error;

	if (roi_idx > MD_MAX_ROI_N-1 || roi_idx < 0) {
		printf("ROI index %d out of range [0~%d]\n", roi_idx, MD_MAX_ROI_N-1);
		return -1;
	}

	if (threshold < 0) {
		printf("md_motbuf_set_roi_threshold ROI#%d threshold %d is out of range 0~%d\n",
				roi_idx, threshold, MD_MAX_THRESHOLD-1);
		return -1;
	}

	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;

	roi_rect[roi_idx].luma_diff_threshold = threshold;

	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_set_roi_validity(u32 roi_idx, u16 valid_flag)
{
	int error;

	if (roi_idx > MD_MAX_ROI_N-1 || roi_idx < 0) {
		printf("ROI index %d out of range [0~%d]\n", roi_idx, MD_MAX_ROI_N-1);
		return -1;
	}

	if (!(valid_flag == 0 || valid_flag == 1)) {
		printf("md_motbuf_set_roi_sensitivity ROI#%d valid %d is neither 0 (disable) nor 1 (enable)\n",
				roi_idx, valid_flag);
		return -1;
	}

	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;

	roi_rect[roi_idx].valid = valid_flag;

	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_status_display(void)
{
	int i, error;

	if ((error = pthread_mutex_lock(&md_motbuf_roi_lock)) != 0)
		return error;

	for (i=0; i<MD_MAX_ROI_N; i++) {
		printf("\tROI#%d, valid %d, x %3d, y %3d, width %3d, height %3d, sensitivity %d, threshold %d\n",
				i, roi_rect[i].valid,
				roi_rect[i].roi_location.x,
				roi_rect[i].roi_location.y,
				roi_rect[i].roi_location.width,
				roi_rect[i].roi_location.height,
				roi_rect[i].sensitivity,
				roi_rect[i].luma_diff_threshold);
	}

	if ((error = pthread_mutex_unlock(&md_motbuf_roi_lock)) != 0)
		return error;

	return 0;
}

int md_motbuf_get_event(u8 *md_event)
{
	u32 error;
	if ((error = pthread_mutex_lock(&md_motbuf_evt_lock)) != 0)
		return error;

	while ((total_event <= 0) && !error && !done_flag) {
		error = pthread_cond_wait(&md_motbuf_evt_cond, &md_motbuf_evt_lock);
	}

	if (error) {
		pthread_mutex_unlock(&md_motbuf_evt_lock);
		return error;
	}

	if (done_flag && total_event <= 0) {
		pthread_mutex_unlock(&md_motbuf_evt_lock);
		return 0;
	}
	else if (total_event > 0)
		memcpy(md_event, &md_motbuf_evtbuf, sizeof(md_motbuf_evtbuf));
	total_event--;

	return pthread_mutex_unlock(&md_motbuf_evt_lock);
}

int md_motbuf_thread_start(void)
{
	int ret;
	md_motbuf_exit_flag = 0;
	if (thread_id == 0) {
		ret = pthread_create(&thread_id, NULL, md_motbuf_mainloop, NULL);
		if (ret != 0) {
			perror("md_motbuf pthread create failed.");
			return -1;
		}
	}

	if (set_realtime_schedule(thread_id) < 0) {
		printf("Set Realtime schedule failed.\n");
	}

	return 0;
}

int md_motbuf_thread_stop(void)
{
	int ret, i;
	if (thread_id == 0) {
		ret = 0;
	} else {
		md_motbuf_exit_flag = 1;
		ret = pthread_join(thread_id, NULL);
		thread_id = 0;
		for (i=0; i<MD_MAX_ROI_N; i++) {
			memset(&md_motbuf_event[i], 0, sizeof(md_motbuf_evt_t));
			memset(&md_motbuf_evtbuf[i], 0, sizeof(md_motbuf_evt_t));
			roi_md_status[i] = MOT_NONE;
		}
	}

	return ret;
}

