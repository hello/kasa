/*******************************************************************************
 * bsreader_s2l.c
 *
 * History:
 *	2012/05/23 - [Jian Tang] created file
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include "bsreader.h"
#include "fifo.h"

//#define BSRREADER_DEBUG

#ifdef BSRREADER_DEBUG
        #define DEBUG_PRINT(x...)   printf("BSREADER: " x)
#else
        #define DEBUG_PRINT(x...)    do{} while(0)
#endif

static version_t G_bsreader_version = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.mod_time = 0x20140702,
	.description = "Ambarella S2L Bsreader Library",
};

static bsreader_init_data_t G_bsreader_init_data;

static int G_ready = 0;
static pthread_mutex_t G_all_mutex;

static u8 *G_dsp_bsb_addr;
static int G_dsp_bsb_size;

static pthread_t G_main_thread_id = 0;

static int G_iav_fd = -1;

static int G_force_main_thread_quit = 0;

static fifo_t* myFIFO[MAX_BS_READER_SOURCE_STREAM];

/*****************************************************************************
 *
 *				private functions
 *
 *****************************************************************************/

static void inline lock_all(void)
{
	pthread_mutex_lock(&G_all_mutex);
}

static void inline unlock_all(void)
{
	pthread_mutex_unlock(&G_all_mutex);
}

/* Blocking call to fetch one frame from BSB ,
 * return -1 to indicate unknown error
 * return 1 to indicate try again
 * this function just get frame information and does not
 * do memcpy
 */
static int fetch_one_frame(bsreader_frame_info_t * bs_info)
{
	struct iav_querydesc query_desc;
	struct iav_framedesc *frame_desc;
	int i, stream_end_num, ret = 0;
	struct iav_stream_info stream_info;
	static u32 dsp_h264_frame_num = 0;
	static u32 dsp_mjpeg_frame_num = 0;
	static u32 frame_num[MAX_BS_READER_SOURCE_STREAM];
	static u32 end_of_stream[MAX_BS_READER_SOURCE_STREAM];
	static int init_flag = 0;

	if (init_flag == 0) {
		for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
			frame_num[i] = 0;
			end_of_stream[i] = 1;
		}
		init_flag = 1;
	}

	for (i = 0, stream_end_num = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		stream_info.id = i;
		if (ioctl(G_iav_fd, IAV_IOC_GET_STREAM_INFO, &stream_info) < 0) {
			perror("IAV_IOC_GET_STREAM_INFO");
			ret = -1;
			goto error_catch;
		}
		if (stream_info.state == IAV_STREAM_STATE_ENCODING) {
			end_of_stream[i] = 0;
		}
		stream_end_num += end_of_stream[i];
	}
	// There is no encoding stream, skip to next turn
	if (stream_end_num == MAX_BS_READER_SOURCE_STREAM) {
		dsp_h264_frame_num = 0;
		dsp_mjpeg_frame_num = 0;
		ret = -2;
		goto error_catch;
	}

	frame_desc = &query_desc.arg.frame;
	// Read frames from all stream by setting id = -1
	frame_desc->id = -1;

	query_desc.qid = IAV_DESC_FRAME;
	if (ioctl(G_iav_fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		ret = -1;
		goto error_catch;
	}

	memset(bs_info, 0, sizeof(*bs_info));	//clear zero
	bs_info->bs_info.stream_id= frame_desc->id;
	bs_info->bs_info.session_id = frame_desc->session_id;
	bs_info->bs_info.frame_num = frame_num[frame_desc->id];
	bs_info->bs_info.pic_type = frame_desc->pic_type;
	bs_info->bs_info.PTS = frame_desc->dsp_pts;
	bs_info->bs_info.mono_pts = frame_desc->arm_pts;
	bs_info->bs_info.stream_end = frame_desc->stream_end;
	bs_info->bs_info.jpeg_quality = frame_desc->jpeg_quality;

	/* If it's a stream end null frame indicator, update the frame counter,
	 * but skip filling frame_addr and frame_size
	 */
	if (frame_desc->stream_end) {
		end_of_stream[frame_desc->id] = 1;
		++stream_end_num;
		if (stream_end_num == MAX_BS_READER_SOURCE_STREAM) {
			dsp_h264_frame_num = 0;
			dsp_mjpeg_frame_num = 0;
		}
		goto error_catch;
	}
	bs_info->frame_addr = G_dsp_bsb_addr + frame_desc->data_addr_offset;
	bs_info->frame_size = frame_desc->size;

	// Check BSB overflow
	if (frame_desc->stream_type == IAV_STREAM_TYPE_MJPEG) {
		if ((frame_desc->frame_num - dsp_mjpeg_frame_num) != 1) {
			printf(" MJPEG [%d] - [%d]  BSB overflow!\n", frame_desc->frame_num, dsp_mjpeg_frame_num);
		}
		dsp_mjpeg_frame_num = frame_desc->frame_num;
	} else {
		if ((frame_desc->frame_num - dsp_h264_frame_num) != 1) {
			printf(" H.264 [%d] - [%d] BSB overflow!\n", frame_desc->frame_num, dsp_h264_frame_num);
		}
		dsp_h264_frame_num = frame_desc->frame_num;
	}

	frame_num[frame_desc->id] ++;

	//dump frame info
	DEBUG_PRINT(">>>>FETCH frm num %d, frm size %d , frm addr 0x%x, stream id %d\n",
		bs_info->bs_info.frame_num, bs_info->frame_size,
		(u32)bs_info->frame_addr, bs_info->bs_info.stream_id);

error_catch:
	return ret;
}


/* dispatch frame , with ring buffer full handling  */
static int dispatch_and_copy_one_frame(bsreader_frame_info_t * bs_info)
{
	int ret;
	DEBUG_PRINT("dispatch_and_copy_one_frame \n");
	if (bs_info->bs_info.stream_end == 1) {
		bs_info->frame_size = 0;
	}
	ret = fifo_write_one_packet(myFIFO[bs_info->bs_info.stream_id],
		(u8 *)&bs_info->bs_info, bs_info->frame_addr, bs_info->frame_size);

	return ret;
}


static int set_realtime_schedule(pthread_t thread_id)
{
	struct sched_param param;
	int policy = SCHED_RR;
	int priority = 90;
	if (!thread_id)
		return -1;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;
	if (pthread_setschedparam(thread_id, policy, &param) < 0)
		perror("pthread_setschedparam");
	pthread_getschedparam(thread_id, &policy, &param);
	if (param.sched_priority != priority)
		return -1;
	return 0;
}


/* this thread main func will be a loop to fetch frames from BSB and fill
 * into max to four stream ring buffers
 */
static void * bsreader_dispatcher_thread_func(void * arg)
{
	bsreader_frame_info_t frame_info;
	int retv = 0;
	//printf("->enter bsreader dispatcher main loop\n");

	while (!G_force_main_thread_quit) {
		if ((retv = fetch_one_frame(&frame_info)) < 0) {
			if ((retv == -1) && (errno != EAGAIN)) {
				perror("fetch one frame failed");
			}
			usleep(10*1000);
			continue;
		}

		//dispatch frame (including ring buffer full handling)
		if (dispatch_and_copy_one_frame(&frame_info) < 0) {
			printf("dispatch frame failed \n");
			continue;
		}
	}
//	printf("->quit bsreader dispatcher main loop\n");

	return 0;
}


/* create a main thread reading data and four threads for four buffer */
static int create_working_thread(void)
{
	pthread_t tid;
	int ret;
	G_force_main_thread_quit = 0;

	ret = pthread_create(&tid, NULL, bsreader_dispatcher_thread_func, NULL);
	if (ret != 0) {
		perror("main thread create failed");
		return -1;
	}
	G_main_thread_id = tid;
	if (set_realtime_schedule(G_main_thread_id) < 0) {
		printf("set realtime schedule error \n");
		return -1;
	}
	return 0;
}

static int cancel_working_thread (void)
{
	int ret;
	if (!G_main_thread_id)
		return 0;

	G_force_main_thread_quit = 1;
	ret = pthread_join(G_main_thread_id, NULL);
	if (ret < 0) {
		perror("pthread_cancel bsreader main");
		return -1;
	}
	G_main_thread_id = 0;
	return 0;
}

static int init_iav(void)
{
	struct iav_querybuf querybuf;
	int state;

	if (G_iav_fd != -1) {
		printf("iav already initialized \n");
		return -1;
	}

	if (G_bsreader_init_data.fd_iav < 0) {
		printf("iav handler invalid %d for bsreader\n", G_bsreader_init_data.fd_iav);
		return -1;
	}
	G_iav_fd = G_bsreader_init_data.fd_iav;

	if (ioctl(G_iav_fd, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAVIOC_G_IAVSTATE");
		return -1;
	}

	//check iav state, must start bsreader before start encoding to ensure no frame missing
	if ((state != IAV_STATE_IDLE) && (state != IAV_STATE_PREVIEW)) {
		printf("iav state must be in either idle or preview before open bsreader\n");
		return -1;
	}

	querybuf.buf = IAV_BUFFER_BSB;
	if (ioctl(G_iav_fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	G_dsp_bsb_size = querybuf.length;
	G_dsp_bsb_addr = mmap(NULL, G_dsp_bsb_size * 2,
				PROT_READ, MAP_SHARED, G_iav_fd, querybuf.offset);
	if (G_dsp_bsb_addr == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

/*****************************************************************************
 *
 *				export functions
 *
 *****************************************************************************/
/* set init parameters */
int bsreader_init(bsreader_init_data_t * bsinit)
{
	/* data check*/
	if (bsinit == NULL)
		return -1;

	if ((bsinit->max_stream_num > MAX_BS_READER_SOURCE_STREAM) ||
		(bsinit->max_stream_num  == 0))
		return -1;

	if (G_ready) {
		printf("bsreader not closed, cannot reinit\n");
		return -1;
	}

	G_bsreader_init_data = *bsinit;
	if (G_bsreader_init_data.max_buffered_frame_num == 0)
		G_bsreader_init_data.max_buffered_frame_num = 8;
	//init mux
	pthread_mutex_init(&G_all_mutex, NULL);
	G_ready = 0;
	return 0;
}


/* allocate memory and do data initialization, threads initialization,
 * also call reset write/read pointers
 */
int bsreader_open(void)
{
	int ret = 0, i = 0;
	u32 max_entry_num;

	lock_all();

	if (G_ready) {
		printf("already opened\n");
		return -1;
	}

	/* Init iav and test iav state, it's expected iav should not be
	 * in encoding, before open bsreader
	 */
	if (init_iav() < 0) {
		ret = -3;
		goto error_catch;
	}
	max_entry_num = G_bsreader_init_data.max_buffered_frame_num;

	for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		myFIFO[i] = fifo_create(G_bsreader_init_data.ring_buf_size[i],
			sizeof(bs_info_t), max_entry_num);
	}

	// Create thread after all other initialization is done
	if (create_working_thread() < 0) {
		printf("create working thread error \n");
		ret = -1;
		goto error_catch;
	}
	G_ready = 1;

	unlock_all();
	return 0;

error_catch:
	cancel_working_thread();
	unlock_all();
	DEBUG_PRINT("bsreader opened.\n");
	return ret;
}


/* free all resources, close all threads */
int bsreader_close(void)
{
	int ret = 0, i = 0;

	lock_all();
	//TODO, possible release
	if (!G_ready) {
		printf("has not opened, cannot close\n");
		return -1;
	}

	if (cancel_working_thread() < 0) {
		perror("pthread_cancel bsreader main");
		return -1;
	} else {
		DEBUG_PRINT("main working thread canceled \n");
	}

	for (i = 0; i < MAX_BS_READER_SOURCE_STREAM; ++i) {
		fifo_close(myFIFO[i]);
	}
	G_ready = 0;
	unlock_all();

	pthread_mutex_destroy(&G_all_mutex);

	return ret;
}


int bsreader_reset(int stream)
{
	if (!G_ready) {
		return -1;
	}
//	printf("bsreader_reset\n");
	fifo_flush(myFIFO[stream]);

	return 0;
}


int bsreader_get_one_frame(int stream, bsreader_frame_info_t * info)
{
	if (!G_ready) {
		return -1;
	}

	return fifo_read_one_packet(myFIFO[stream], (u8 *)&info->bs_info,
		&info->frame_addr, &info->frame_size);
}

int bsreader_get_version(version_t* version)
{
	memcpy(version, &G_bsreader_version, sizeof(version_t));
	return 0;
}


