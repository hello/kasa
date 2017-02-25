/*
 * test_sw_encoding.c
 *
 * History:
 *	2014/12/16 - [Zhi He] created file
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include <iav_ioctl.h>
#include <signal.h>

#include <pthread.h>
#include <semaphore.h>

#include "x265.h"

static int fd_iav = -1;
static u8 *dsp_mem = NULL;
static u32 dsp_size = 0;
static int quit_test_encoding = 0;

enum
{
	EPIXEL_FORMAT_NV12 = 0,
	EPIXEL_FORMAT_YV12 = 1,
	EPIXEL_FORMAT_IYUV = 2,
} ;
static int current_pix_format = EPIXEL_FORMAT_NV12;

typedef struct msg_s {
	int cmd;
	void *ptr;
	u8 arg1, arg2, arg3, arg4;
} msg_t;

#define MAX_MSGS	64

typedef struct msg_queue_s {
	msg_t msgs[MAX_MSGS];
	int read_index;
	int write_index;
	int num_msgs;
	int num_readers;
	int num_writers;
	pthread_mutex_t mutex;
	pthread_cond_t cond_read;
	pthread_cond_t cond_write;
} msg_queue_t;

static int msg_queue_init(msg_queue_t *q)
{
	q->read_index = 0;
	q->write_index = 0;
	q->num_msgs = 0;
	q->num_readers = 0;
	q->num_writers = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond_read, NULL);
	pthread_cond_init(&q->cond_write, NULL);
	return 0;
}

void msg_queue_get(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs > 0) {
			*msg = q->msgs[q->read_index];

			if (++q->read_index == MAX_MSGS)
				q->read_index = 0;

			q->num_msgs--;

			if (q->num_writers > 0) {
				q->num_writers--;
				pthread_cond_signal(&q->cond_write);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_readers++;
		pthread_cond_wait(&q->cond_read, &q->mutex);
	}
}

int msg_queue_peek(msg_queue_t *q, msg_t *msg)
{
	int ret = 0;
	pthread_mutex_lock(&q->mutex);

	if (q->num_msgs > 0) {
		*msg = q->msgs[q->read_index];

		if (++q->read_index == MAX_MSGS)
			q->read_index = 0;

		q->num_msgs--;

		if (q->num_writers > 0) {
			q->num_writers--;
			pthread_cond_signal(&q->cond_write);
		}

		ret = 1;
	}

	pthread_mutex_unlock(&q->mutex);
	return ret;
}

void msg_queue_put(msg_queue_t *q, msg_t *msg)
{
	pthread_mutex_lock(&q->mutex);
	while (1) {
		if (q->num_msgs < MAX_MSGS) {
			q->msgs[q->write_index] = *msg;

			if (++q->write_index == MAX_MSGS)
				q->write_index = 0;

			q->num_msgs++;

			if (q->num_readers > 0) {
				q->num_readers--;
				pthread_cond_signal(&q->cond_read);
			}

			pthread_mutex_unlock(&q->mutex);
			return;
		}

		q->num_writers++;
		pthread_cond_wait(&q->cond_write, &q->mutex);
	}
}

typedef struct
{
	u8 *p_data[3];
	u32 linesize[3];

	int debug_id;
} _yuv_frame;

typedef struct
{
	u8 buffer_id;
	u8 b_non_block_mode;
	u8 b_dump_input_yuv;
	u8 flag_running;

	u32 tot_frames;
	u32 width;
	u32 height;

	const char *output_filenamebase;

	msg_queue_t* output_data_queue;

	msg_queue_t* simple_buffer_pool;
} _capture_thread_context;

typedef struct
{
	u8 bframes;
	u8 b_silence;
	u8 reserved0;
	u8 flag_running;

	u32 bitrate;

	u32 width;
	u32 height;

	u32 framerate_num;
	u32 framerate_den;

	u32 keyframe_min;
	u32 keyframe_max;

	const char *output_filenamebase;

	msg_queue_t* input_data_queue;

	msg_queue_t* simple_buffer_pool;
} _encoding_thread_context;

typedef struct
{
	u8 *p_data;
	u32 datasize;

	u8 nal_type;
	u8 reserved0;
	u8 reserved1;
	u8 reserved2;
} _s_video_data_packet;

static int pre_alloc_yuv420p_frames(u32 width, u32 height, u32 number_of_frames, msg_queue_t *simple_buffer_pool)
{
	u32 y_size = width * height;
	u32 c_size = (width * height) >> 2;
	u32 i = 0;
	msg_t msg;
	memset(&msg, 0x0, sizeof(msg));

	for (i = 0; i < number_of_frames; i ++) {
		_yuv_frame *video_frame = (_yuv_frame *) malloc(sizeof(_yuv_frame));
		if (video_frame) {
			video_frame->p_data[0] = (u8 *) malloc(y_size);
			video_frame->p_data[1] = (u8 *) malloc(c_size);
			video_frame->p_data[2] = (u8 *) malloc(c_size);
			if (video_frame->p_data[0] && video_frame->p_data[1] && video_frame->p_data[2]) {
				video_frame->linesize[0] = width;
				video_frame->linesize[1] = video_frame->linesize[2] = width >> 1;
				video_frame->debug_id = i;
				msg.ptr = (void *) video_frame;
				msg_queue_put(simple_buffer_pool, &msg);
			} else {
				printf("[error]: no memory in pre_alloc_yuv420p_frames\n");
				return (-1);
			}
		}
	}

	return 0;
}

typedef struct
{
	u8 buffer_id;
	u8 bframes;
	u8 b_silence;
	u8 b_non_block_mode;

	u8 b_dump_input_yuv;
	u8 reserved0;
	u8 reserved1;
	u8 reserved2;

	u32 tot_frames;
	u32 bitrate;

	u32 framerate_num;
	u32 framerate_den;

	u32 keyframe_min;
	u32 keyframe_max;

	char output_filenamebase[512];
} _sw_encoding_params;

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
	}

	return 0;
}

typedef int (* _f_video_encode) (void* context, _yuv_frame* input_buffer, _s_video_data_packet output_packet[], u32* output_packet_number);
typedef void (* _f_destroy_encoder) (void* context);

typedef struct
{
	_f_video_encode f_encode;
	_f_destroy_encoder f_destroy;
} _s_encoder_context;

typedef struct
{
	_s_encoder_context base;

	x265_param m_params;
	x265_encoder *m_encoder;
	x265_nal *m_nal;
	u32 m_nal_count;
	u32 m_nal_index;
	x265_picture m_input_picture;
	x265_picture m_output_picture;
} _s_x265_encoder_context;

static int __x265_encode (void* context, _yuv_frame* input_buffer, _s_video_data_packet output_packet[], u32* output_packet_number)
{
	if (!context || !input_buffer || !output_packet || !output_packet_number) {
		printf("[fatal error]: NULL params in x265_encode(), please check code!\n");
		return (-20);
	}

	_s_x265_encoder_context *thiz = (_s_x265_encoder_context*) context;

	thiz->m_input_picture.planes[0] = input_buffer->p_data[0];
	thiz->m_input_picture.planes[1] = input_buffer->p_data[1];
	thiz->m_input_picture.planes[2] = input_buffer->p_data[2];

	thiz->m_input_picture.stride[0] = input_buffer->linesize[0];
	thiz->m_input_picture.stride[1] = input_buffer->linesize[1];
	thiz->m_input_picture.stride[2] = input_buffer->linesize[2];

	int ret = x265_encoder_encode(thiz->m_encoder, &thiz->m_nal, &thiz->m_nal_count, &thiz->m_input_picture, &thiz->m_output_picture);
	if (0 <= ret) {
		if (output_packet_number[0] < thiz->m_nal_count) {
			printf("[error]: too small nal array size %d, current %d\n", output_packet_number[0], thiz->m_nal_count);
			return (-21);
		}
		int i = 0;
		for (i = 0; i < thiz->m_nal_count; i ++) {
			output_packet[i].p_data = thiz->m_nal[i].payload;
			output_packet[i].datasize = thiz->m_nal[i].sizeBytes;
			output_packet[i].nal_type = thiz->m_nal[i].type;
		}
		output_packet_number[0] = thiz->m_nal_count;
		return 0;
	}

	printf("[error]: x265_encoder_encode() ret %d\n", ret);
	return ret;
}

void __destroy_x265_encoder(void* context)
{
	if (!context) {
		printf("[fatal error]: NULL params in destroy_x265_encoder(), please check code!\n");
		return;
	}

	_s_x265_encoder_context *thiz = (_s_x265_encoder_context*) context;

	if (thiz->m_encoder) {
		x265_encoder_close(thiz->m_encoder);
	}

	free(context);
}

static _s_encoder_context* create_x265_encoder(u32 width, u32 height, u32 framerate_num, u32 framerate_den, u32 keyframe_min, u32 keyframe_max, u32 bitrate, u32 bframes)
{
	_s_x265_encoder_context *thiz = (_s_x265_encoder_context*) malloc(sizeof(_s_x265_encoder_context));
	if (!thiz) {
		printf("[fatal error]: not enough memory in create_x265_encoder(), request size %d\n", sizeof(_s_x265_encoder_context));
		return NULL;
	}

	memset(thiz, 0x0, sizeof(_s_x265_encoder_context));

	x265_param_default(&thiz->m_params);
	x265_param_default_preset(&thiz->m_params, "faster", "stillimage");

	thiz->m_params.sourceWidth = width;
	thiz->m_params.sourceHeight = height;
	thiz->m_params.fpsNum = framerate_num;
	thiz->m_params.fpsDenom = framerate_den;

	thiz->m_params.maxNumReferences = bframes + 1;
	thiz->m_params.keyframeMax = keyframe_max;
	thiz->m_params.keyframeMin = keyframe_min;

	thiz->m_params.rc.bitrate = bitrate >> 10;
	thiz->m_params.bRepeatHeaders = 1;

	thiz->m_params.bFrameAdaptive = 0;
	thiz->m_params.bframes = bframes;
	thiz->m_params.lookaheadDepth = 0;
	thiz->m_params.scenecutThreshold = 0;
	thiz->m_params.rc.cuTree = 0;

	thiz->m_encoder = x265_encoder_open(&thiz->m_params);
	if (NULL == thiz->m_encoder) {
		printf("[error]: x265_encoder_open() fail\n");
		free(thiz);
		return NULL;
	}

	x265_picture_init(&thiz->m_params, &thiz->m_input_picture);

	thiz->base.f_encode = __x265_encode;
	thiz->base.f_destroy = __destroy_x265_encoder;

	return (_s_encoder_context*) thiz;
}

static void convert_nv12_to_i420(_yuv_frame *src, _yuv_frame *dst, u32 width, u32 height)
{
	u8 *psrc, *pdst, *pdst1;
	u32 stride_src, stride_dst;
	u32 i = 0, j = 0;

	if ((src->p_data[0] == dst->p_data[0]) && (src->linesize[0] == dst->linesize[0])) {
		// skip y
	} else {
		printf("[debug check fail]: should not process y\n");
		psrc = src->p_data[0];
		pdst = dst->p_data[0];
		stride_src = src->linesize[0];
		stride_dst = dst->linesize[0];
		for (i = 0; i < height; i ++) {
			memcpy(pdst, psrc, width);
			pdst += stride_dst;
			psrc += stride_src;
		}
	}

	width = width >> 1;
	height = height >> 1;
	stride_src = src->linesize[1];
	for (i = 0; i < height; i ++) {
		psrc = src->p_data[1] + i * stride_src;
		pdst = dst->p_data[1] + i * dst->linesize[1];
		pdst1 = dst->p_data[2] + i * dst->linesize[2];
		for (j = 0; j < width; j ++) {
			*pdst ++ = *psrc ++;
			*pdst1 ++ = *psrc ++;
		}
	}

}

static void copy_nv12_to_i420(_yuv_frame *src, _yuv_frame *dst, u32 width, u32 height)
{
	u8 *psrc, *pdst, *pdst1;
	u32 stride_src, stride_dst;
	u32 i = 0, j = 0;

	if (src->linesize[0] == dst->linesize[0]) {
		memcpy(dst->p_data[0], src->p_data[0], height * src->linesize[0]);
	} else {
		psrc = src->p_data[0];
		pdst = dst->p_data[0];
		stride_src = src->linesize[0];
		stride_dst = dst->linesize[0];
		for (i = 0; i < height; i ++) {
			memcpy(pdst, psrc, width);
			pdst += stride_dst;
			psrc += stride_src;
		}
	}

	width = width >> 1;
	height = height >> 1;
	stride_src = src->linesize[1];
	for (i = 0; i < height; i ++) {
		psrc = src->p_data[1] + i * stride_src;
		pdst = dst->p_data[1] + i * dst->linesize[1];
		pdst1 = dst->p_data[2] + i * dst->linesize[2];
		for (j = 0; j < width; j ++) {
			*pdst ++ = *psrc ++;
			*pdst1 ++ = *psrc ++;
		}
	}

}

static void copy_i420(_yuv_frame *src, _yuv_frame *dst, u32 width, u32 height)
{
	u8 *psrc, *pdst;
	u32 stride_src, stride_dst;
	u32 i = 0;

	if (src->linesize[0] == dst->linesize[0]) {
		memcpy(dst->p_data[0], src->p_data[0], height * src->linesize[0]);
	} else {
		psrc = src->p_data[0];
		pdst = dst->p_data[0];
		stride_src = src->linesize[0];
		stride_dst = dst->linesize[0];
		for (i = 0; i < height; i ++) {
			memcpy(pdst, psrc, width);
			pdst += stride_dst;
			psrc += stride_src;
		}
	}

	width = width >> 1;
	height = height >> 1;

	if (src->linesize[1] == dst->linesize[1]) {
		memcpy(dst->p_data[1], src->p_data[1], height * src->linesize[1]);
	} else {
		stride_src = src->linesize[1];
		stride_dst = dst->linesize[1];
		psrc = src->p_data[1];
		pdst = dst->p_data[1];
		for (i = 0; i < height; i ++) {
			memcpy(pdst, psrc, width);
			pdst += stride_dst;
			psrc += stride_src;
		}
	}

	if (src->linesize[2] == dst->linesize[2]) {
		memcpy(dst->p_data[2], src->p_data[2], height * src->linesize[2]);
	} else {
		stride_src = src->linesize[2];
		stride_dst = dst->linesize[2];
		psrc = src->p_data[2];
		pdst = dst->p_data[2];
		for (i = 0; i < height; i ++) {
			memcpy(pdst, psrc, width);
			pdst += stride_dst;
			psrc += stride_src;
		}
	}
}

static void write_i420(_yuv_frame *src, FILE * p_file, u32 width, u32 height)
{
	u8 *psrc;
	u32 stride_src;
	u32 i = 0;

	if (src->linesize[0] == width) {
		fwrite(src->p_data[0], 1, height * src->linesize[0], p_file);
	} else {
		psrc = src->p_data[0];
		stride_src = src->linesize[0];
		for (i = 0; i < height; i ++) {
			fwrite(psrc, 1, width, p_file);
			psrc += stride_src;
		}
	}

	width = width >> 1;
	height = height >> 1;

	if (src->linesize[1] == width) {
		fwrite(src->p_data[1], 1, height * src->linesize[1], p_file);
	} else {
		stride_src = src->linesize[1];
		psrc = src->p_data[1];
		for (i = 0; i < height; i ++) {
			fwrite(psrc, 1, width, p_file);
			psrc += stride_src;
		}
	}

	if (src->linesize[2] == width) {
		fwrite(src->p_data[2], 1, height * src->linesize[2], p_file);
	} else {
		stride_src = src->linesize[2];
		psrc = src->p_data[2];
		for (i = 0; i < height; i ++) {
			fwrite(psrc, 1, width, p_file);
			psrc += stride_src;
		}
	}
}

static int get_source_buffer_format(int buf, u32 *width, u32 *height)
{
	struct iav_srcbuf_format buf_format;
	memset(&buf_format, 0, sizeof(buf_format));
	buf_format.buf_id = buf;
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return (-1);
	}
	*width = buf_format.size.width;
	*height = buf_format.size.height;

	return 0;
}

static int test_sw_encoding_single_thread(_sw_encoding_params* params)
{
	u32 i = 0, j = 0, tot_frames = 0;
	u32 width = 0, height = 0;
	int buf = params->buffer_id;

	if ((buf < IAV_SRCBUF_FIRST) || (buf > IAV_SRCBUF_LAST)) {
		printf("[error]: wrong buf id %d\n", buf);
		return (-1);
	}

	if (0 > get_source_buffer_format(buf, &width, &height)) {
		printf("[error]: get_source_buffer_format() fail, buf id %d\n", buf);
		return (-2);
	} else {
		printf("[config]: get (buf id %d) info: width %d, height %d\n", buf, width, height);
	}

	struct iav_yuvbufdesc yuv_desc;
	struct iav_querydesc query_desc;
	_s_encoder_context* p_x265_encoder = NULL;
	FILE* p_output_es = NULL;
	u8* puv = NULL;

	_yuv_frame input_frame;
	_yuv_frame cap_frame;
	_s_video_data_packet output_packet[16];
	u32 output_packet_number = 16;

	u32 voffset =  (width * height) >> 2;
	int ret = 0;

	p_x265_encoder = create_x265_encoder(width, height, params->framerate_num, params->framerate_den, params->keyframe_min, params->keyframe_max, params->bitrate, params->bframes);
	if (!p_x265_encoder) {
		printf("[error]: create_x265_encoder() fail\n");
		goto exit_sw_encoding_single_thread;
	}

	if (1) {
		char output_filename[512] = {0};
		snprintf(output_filename, 512, "%s_%dx%d.h265", params->output_filenamebase, width, height);
		p_output_es = fopen(output_filename, "wb");
		if (!p_output_es) {
			printf("[error]: open output file() fail\n");
			goto exit_sw_encoding_single_thread;
		}
	}

	if (EPIXEL_FORMAT_IYUV != current_pix_format) {
		puv= (u8*) malloc(width * height * 2);
		if (!puv) {
			printf("[error]: malloc buffer fail\n");
			goto exit_sw_encoding_single_thread;
		}
	}

	if (params->tot_frames) {
		tot_frames = params->tot_frames;
	} else {
		tot_frames = 8 * 1024 * 1024;//max enough for unit test
	}

	for (i = 0; ((i < tot_frames) && !quit_test_encoding); ++i) {

		memset(&query_desc, 0, sizeof(query_desc));
		query_desc.qid = IAV_DESC_YUV;
		query_desc.arg.yuv.buf_id = buf;
		query_desc.arg.yuv.flag = 0;

		if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				perror("IAV_IOC_QUERY_DESC");
				goto exit_sw_encoding_single_thread;
			}
		}

		yuv_desc = query_desc.arg.yuv;

		if (((u8 *)yuv_desc.y_addr_offset == NULL) || ((u8 *)yuv_desc.uv_addr_offset == NULL)) {
			printf("YUV buffer [%d] address is NULL! Skip to next!\n", buf);
			continue;
		}

		output_packet_number = 16;

		if (EPIXEL_FORMAT_NV12 == current_pix_format) {
			cap_frame.p_data[0] = yuv_desc.y_addr_offset + dsp_mem;
			cap_frame.p_data[1] = yuv_desc.uv_addr_offset + dsp_mem;
			cap_frame.linesize[0] = yuv_desc.pitch;
			cap_frame.linesize[1] = yuv_desc.pitch;

			input_frame.p_data[0] = cap_frame.p_data[0];
			input_frame.p_data[1] = puv;
			input_frame.p_data[2] = puv + voffset;
			input_frame.linesize[0] = cap_frame.linesize[0];
			input_frame.linesize[1] = input_frame.linesize[2] = width >> 1;

			convert_nv12_to_i420(&cap_frame, &input_frame, width, height);
		} else if (EPIXEL_FORMAT_IYUV == current_pix_format) {
			input_frame.p_data[0] = yuv_desc.y_addr_offset + dsp_mem;
			input_frame.p_data[1] = yuv_desc.uv_addr_offset + dsp_mem;
			input_frame.p_data[2] = yuv_desc.uv_addr_offset + dsp_mem + voffset;
			input_frame.linesize[0] = yuv_desc.pitch;
			input_frame.linesize[1] = input_frame.linesize[2] = yuv_desc.pitch >> 1;
		} else {
			printf("[error]: to do! need support pix format\n");
			goto exit_sw_encoding_single_thread;
		}

		ret = p_x265_encoder->f_encode(p_x265_encoder, &input_frame, output_packet, &output_packet_number);

		if (0 <= ret) {
			if (!params->b_silence) {
				printf("\toutput frame[%d]:\n", i);
			}
			for (j = 0; j < output_packet_number; j ++) {
				if (!params->b_silence) {
					printf("\t\toutput packet[%d]: size %d, nal_type %d\n", j, output_packet[j].datasize, output_packet[j].nal_type);
				}
				fwrite(output_packet[j].p_data, 1, output_packet[j].datasize, p_output_es);
			}
		} else {
			break;
		}

	}

	if (p_output_es) {
		fclose(p_output_es);
		p_output_es = NULL;
	}

	if (p_x265_encoder) {
		p_x265_encoder->f_destroy(p_x265_encoder);
		p_x265_encoder = NULL;
	}

	if (puv) {
		free(puv);
		puv = NULL;
	}

	return 0;

exit_sw_encoding_single_thread:

	if (p_output_es) {
		fclose(p_output_es);
		p_output_es = NULL;
	}

	if (p_x265_encoder) {
		p_x265_encoder->f_destroy(p_x265_encoder);
		p_x265_encoder = NULL;
	}

	if (puv) {
		free(puv);
		puv = NULL;
	}

	return -1;
}

static void* thread_capture(void* param)
{
	_capture_thread_context * thiz = (_capture_thread_context *) param;
	if (!thiz) {
		printf("[error]: capture thread's context is NULL\n");
		return NULL;
	}

	struct iav_yuvbufdesc yuv_desc;
	struct iav_querydesc query_desc;
	u8 *puv = NULL;
	u32 i = 0, tot_frames = thiz->tot_frames;
	_yuv_frame* p_frame = NULL;
	_yuv_frame cap_frame;
	msg_t msg;

	if (EPIXEL_FORMAT_IYUV != current_pix_format) {
		puv= (u8*) malloc(thiz->width * thiz->height * 2);
		if (!puv) {
			printf("[error]: malloc buffer in capture thread fail\n");
			return NULL;
		}
	}

	if (!tot_frames) {
		tot_frames = 8 * 1024 * 1024;//max enough for unit test
	}

	FILE *p_out_yuv = NULL;
	if (thiz->b_dump_input_yuv) {
		char output_filename[512] = {0};
		snprintf(output_filename, 512, "%s_%dx%d.yuv", thiz->output_filenamebase, thiz->width, thiz->height);
		p_out_yuv = fopen(output_filename, "wb");
		if (!p_out_yuv) {
			printf("[error]: open yuv output file() fail\n");
		}
	}

	printf("[flow]: thread_capture() begin ... tot_frames = %d\n", tot_frames);

	for (i = 0; ((i < tot_frames) && thiz->flag_running); ) {

		if (!p_frame) {
			msg_queue_get(thiz->simple_buffer_pool, &msg);
			p_frame = (_yuv_frame *) msg.ptr;
			if (!p_frame) {
				printf("[flow]: receive exit msg\n");
				break;
			}
			//printf("[debug data flow]: cap get buffer id = %d\n", p_frame->debug_id);
		}

		memset(&query_desc, 0, sizeof(query_desc));
		query_desc.qid = IAV_DESC_YUV;
		query_desc.arg.yuv.buf_id = thiz->buffer_id;
		query_desc.arg.yuv.flag = 0;

		if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
			if (errno == EINTR) {
				printf("[warn]: EINTR in IAV_IOC_QUERY_DESC\n");
				continue;
			} else {
				perror("IAV_IOC_QUERY_DESC");
				goto exit_capture_thread;
			}
		} else {
			++ i;
		}

		yuv_desc = query_desc.arg.yuv;

		if (((u8 *)yuv_desc.y_addr_offset == NULL) || ((u8 *)yuv_desc.uv_addr_offset == NULL)) {
			printf("[error]: YUV buffer [%d] address is NULL! Skip to next!\n", thiz->buffer_id);
			continue;
		}

		if (EPIXEL_FORMAT_NV12 == current_pix_format) {
			cap_frame.p_data[0] = yuv_desc.y_addr_offset + dsp_mem;
			cap_frame.p_data[1] = yuv_desc.uv_addr_offset + dsp_mem;
			cap_frame.p_data[2] = NULL;
			cap_frame.linesize[0] = yuv_desc.pitch;
			cap_frame.linesize[1] = yuv_desc.pitch;
			cap_frame.linesize[2] = 0;
			copy_nv12_to_i420(&cap_frame, p_frame, thiz->width, thiz->height);
		} else if (EPIXEL_FORMAT_IYUV == current_pix_format) {
			cap_frame.p_data[0] = yuv_desc.y_addr_offset + dsp_mem;
			cap_frame.p_data[1] = yuv_desc.uv_addr_offset + dsp_mem;
			cap_frame.p_data[2] = yuv_desc.uv_addr_offset + dsp_mem + (yuv_desc.pitch >> 1) * (thiz->height >> 1);
			cap_frame.linesize[0] = yuv_desc.pitch;
			cap_frame.linesize[1] = cap_frame.linesize[2] = yuv_desc.pitch >> 1;
			copy_i420(&cap_frame, p_frame, thiz->width, thiz->height);
		} else {
			printf("[error]: to do! need support pix format\n");
			goto exit_capture_thread;
		}

		if (p_out_yuv) {
			write_i420(p_frame, p_out_yuv, thiz->width, thiz->height);
		}

		msg.ptr = (void *) p_frame;
		//printf("[debug data flow]: cap seq_num %d, send buffer id = %d\n", yuv_desc.seq_num, p_frame->debug_id);
		msg_queue_put(thiz->output_data_queue, &msg);
		p_frame = NULL;

	}

	if (puv) {
		free(puv);
		puv = NULL;
	}

	if (p_out_yuv) {
		fclose(p_out_yuv);
		p_out_yuv = NULL;
	}

	printf("[flow]: thread_capture() end process frames %d\n", i);
	msg.ptr = NULL;
	msg_queue_put(thiz->output_data_queue, &msg);

	return NULL;

exit_capture_thread:

	if (puv) {
		free(puv);
		puv = NULL;
	}

	if (p_out_yuv) {
		fclose(p_out_yuv);
		p_out_yuv = NULL;
	}

	printf("[flow]: thread_capture() error, exit, process frame %d\n", i);
	msg.ptr = NULL;
	msg_queue_put(thiz->output_data_queue, &msg);

	return NULL;
}

static void* thread_capture_nonblock(void* param)
{
	//to do
	return NULL;
}

static void* thread_encoding(void* param)
{
	_encoding_thread_context * thiz = (_encoding_thread_context *) param;
	if (!thiz) {
		printf("[error]: encoding thread's context is NULL\n");
		return NULL;
	}

	_s_encoder_context *p_x265_encoder = NULL;
	FILE *p_output_es = NULL;
	_yuv_frame *input_frame = NULL;
	_s_video_data_packet output_packet[16];
	u32 i = 0, output_packet_number = 16;
	msg_t msg;
	int ret = 0;
	u32 debug_frame_count = 0;

	p_x265_encoder = create_x265_encoder(thiz->width, thiz->height, thiz->framerate_num, thiz->framerate_den, thiz->keyframe_min, thiz->keyframe_max, thiz->bitrate, thiz->bframes);
	if (!p_x265_encoder) {
		printf("[error]: create_x265_encoder() fail\n");
		goto exit_encoding_thread;
	}

	if (1) {
		char output_filename[512] = {0};
		snprintf(output_filename, 512, "%s_%dx%d.h265", thiz->output_filenamebase, thiz->width, thiz->height);
		p_output_es = fopen(output_filename, "wb");
		if (!p_output_es) {
			printf("[error]: open output file() fail\n");
			goto exit_encoding_thread;
		}
	}

	while (thiz->flag_running) {

		msg_queue_get(thiz->input_data_queue, &msg);
		if (!msg.ptr) {
			printf("[flow]: recieve exit msg\n");
			break;
		}
		input_frame = (_yuv_frame *) msg.ptr;
		//printf("[debug data flow]: encoder get buffer id = %d\n", input_frame->debug_id);

		output_packet_number = 16;
		ret = p_x265_encoder->f_encode(p_x265_encoder, input_frame, output_packet, &output_packet_number);

		if (0 <= ret) {
			if (!thiz->b_silence) {
				printf("\toutput frame[%d]:\n", debug_frame_count ++);
			}
			for (i = 0; i < output_packet_number; i ++) {
				if (!thiz->b_silence) {
					printf("\t\toutput packet[%d]: size %d, nal_type %d\n", i, output_packet[i].datasize, output_packet[i].nal_type);
				}
				fwrite(output_packet[i].p_data, 1, output_packet[i].datasize, p_output_es);
			}
		} else {
			printf("[flow]: thread_encoding, encode fail, ret %d\n", ret);
			break;
		}

		//printf("[debug data flow]: encoder return buffer id = %d\n", input_frame->debug_id);
		msg_queue_put(thiz->simple_buffer_pool, &msg);
	}

	if (p_output_es) {
		fclose(p_output_es);
		p_output_es = NULL;
	}

	if (p_x265_encoder) {
		p_x265_encoder->f_destroy(p_x265_encoder);
		p_x265_encoder = NULL;
	}

	printf("[flow]: thread_encoding() end\n");
	quit_test_encoding = 1;
	return NULL;

exit_encoding_thread:

	if (p_output_es) {
		fclose(p_output_es);
		p_output_es = NULL;
	}

	if (p_x265_encoder) {
		p_x265_encoder->f_destroy(p_x265_encoder);
		p_x265_encoder = NULL;
	}

	printf("[flow]: thread_capture() error, exit, process frame %d\n", i);
	quit_test_encoding = 1;
	return NULL;
}

static int test_sw_encoding(_sw_encoding_params* params)
{
	u32 width = 0, height = 0;
	int buf = params->buffer_id;

	pthread_t capture_thread;
	pthread_t encoding_thread;

	_capture_thread_context capture_context;
	_encoding_thread_context encoding_context;

	if ((buf < IAV_SRCBUF_FIRST) || (buf > IAV_SRCBUF_LAST)) {
		printf("[error]: wrong buf id %d\n", buf);
		return (-1);
	}

	if (0 > get_source_buffer_format(buf, &width, &height)) {
		printf("[error]: get_source_buffer_format() fail, buf id %d\n", buf);
		return (-2);
	} else {
		printf("[config]: get (buf id %d) info: width %d, height %d\n", buf, width, height);
	}

	msg_queue_t *p_data_queue = (msg_queue_t *)malloc(sizeof(msg_queue_t));
	msg_queue_t *p_simple_buffer_pool = (msg_queue_t *)malloc(sizeof(msg_queue_t));
	if (!p_data_queue || !p_simple_buffer_pool) {
		printf("[error]: create queue fail, no memory?\n");
		return (-2);
	}

	msg_queue_init(p_data_queue);
	msg_queue_init(p_simple_buffer_pool);

	pre_alloc_yuv420p_frames(width, height, 32, p_simple_buffer_pool);

	capture_context.buffer_id = params->buffer_id;
	capture_context.b_non_block_mode = params->b_non_block_mode;
	capture_context.b_dump_input_yuv = params->b_dump_input_yuv;
	capture_context.flag_running = 1;
	capture_context.tot_frames = params->tot_frames;
	capture_context.width = width;
	capture_context.height = height;
	capture_context.output_filenamebase = params->output_filenamebase;
	capture_context.output_data_queue = p_data_queue;
	capture_context.simple_buffer_pool = p_simple_buffer_pool;

	encoding_context.bframes = params->bframes;
	encoding_context.b_silence = params->b_silence;
	encoding_context.flag_running = 1;
	encoding_context.bitrate = params->bitrate;
	encoding_context.width = width;
	encoding_context.height = height;
	encoding_context.framerate_num = params->framerate_num;
	encoding_context.framerate_den = params->framerate_den;
	encoding_context.keyframe_min = params->keyframe_min;
	encoding_context.keyframe_max = params->keyframe_max;
	encoding_context.output_filenamebase = params->output_filenamebase;
	encoding_context.input_data_queue = p_data_queue;
	encoding_context.simple_buffer_pool = p_simple_buffer_pool;

	pthread_attr_t attr;
	struct sched_param param;
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = 50;
	pthread_attr_setschedparam(&attr, &param);
	int err = 0;

	if (1 || (!params->b_non_block_mode)) {
		if (params->b_non_block_mode) {
			printf("[to do]: non-block mode is not ready\n");
		}
		err = pthread_create(&capture_thread, &attr, thread_capture, &capture_context);
		pthread_attr_destroy(&attr);
		if (0 > err) {
			printf("[error]: create thread_capture fail, ret %d.\n", err);
			return (-3);
		}
	} else {
		err = pthread_create(&capture_thread, &attr, thread_capture_nonblock, &capture_context);
		pthread_attr_destroy(&attr);
		if (0 > err) {
			printf("[error]: create thread_capture fail, ret %d.\n", err);
			return (-3);
		}
	}

	pthread_create(&encoding_thread, NULL, thread_encoding, &encoding_context);
	if (0 > err) {
		printf("[error]: create thread_encoding fail, ret %d.\n", err);
		return (-4);
	}

	char buffer[128];
	int flag_stdin = 0;

	flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
	if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
		printf("[error]: stdin_fileno set error.\n");
	}

	while (!quit_test_encoding) {
		memset(buffer, 0x0, sizeof(buffer));

		if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
			usleep(100000);
			continue;
		}

		switch (buffer[0]) {
			case 'q':
				printf("press 'q': quit\n");
				quit_test_encoding = 1;
				break;

			default:
				break;
		}
	}

	if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
		printf("[error]: stdin_fileno set error\n");
	}

	printf("[flow]: start exit..\n");
	capture_context.flag_running = 0;
	encoding_context.flag_running = 0;
	void* pv;
	msg_t msg;
	memset(&msg, 0x0, sizeof(msg));
	msg_queue_put(p_data_queue, &msg);
	msg_queue_put(p_simple_buffer_pool, &msg);

	pthread_join(capture_thread, &pv);
	printf("[flow]: wait capture thread exit done\n");
	pthread_join(encoding_thread, &pv);
	printf("[error]: wait encoding thread exit done\n");

	return 0;
}

static void sigstop(int a)
{
	quit_test_encoding = 1;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		exit(2);
	}

	if ((state != IAV_STATE_PREVIEW) && state != IAV_STATE_ENCODING) {
		printf("IAV is not in preview / encoding state, cannot get yuv buf!\n");
		return -1;
	}

	return 0;
}

static void default_params(_sw_encoding_params* params)
{
	params->buffer_id = 1;
	params->bframes = 0;
	params->b_non_block_mode = 0;
	params->b_silence = 0;

	params->tot_frames = 512;
	params->bitrate = 128000;

	params->framerate_num = 90000;
	params->framerate_den = 3003;

	params->keyframe_min = 256;
	params->keyframe_max = 512;

	strcpy(params->output_filenamebase, "output");
}

static void show_usage()
{
	printf("test_sw_encoding usage:\n");
	printf("\t-b [%%d]: specify capture buffer id, default is 1, same with '--buffer' \n");
	printf("\t-f [%%s]: specify output filename base, final file name will be 'filename_%%dx%%d.h265', same with '--filename'\n");
	printf("\n");
	printf("\t--buffer [%%d]: specify capture buffer id, default is 1, same with '-b'\n");
	printf("\t--filename [%%s]: specify output filename base, final file name will be 'filename_%%dx%%d.h265', same with '-f'\n");
	printf("\t--framecount [%%d]: specify total encoding frame count, default is 512 frames\n");
	printf("\t--bitrate [%%d]: specify encoding bitrate, default is 128000\n");
	printf("\t--bfarmes [%%d]: specify bfarmes, default is 0\n");
	printf("\t--frnum [%%d]: specify framerate num, default is 90000\n");
	printf("\t--frden [%%d]: specify framerate den, default is 3003\n");
	printf("\t--keymin [%%d]: specify keyframe min (I), default is 256\n");
	printf("\t--keymax [%%d]: specify keyframe max (IDR), default is 512\n");
	printf("\t--nonblock: use non-block mode for yuv capture\n");
	printf("\t--dumpinputyuv: dump input yuv data\n");
	printf("\t--silence: do not show each frame's info\n");
	printf("\t--help: show usage\n");
}

static int init_params(int argc, char **argv, _sw_encoding_params* params)
{
	int i = 0;
	int val = 0;

	for (i = 1; i < argc; i ++) {
		if (!strcmp("--buffer", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->buffer_id = val;
				printf("[input argument (%d)]: '--buffer': (%d).\n", i, params->buffer_id);
			} else {
				printf("[input argument error]: '--buffer', should follow with integer(buffer id), argc %d, i %d.\n", argc, i);
				return (-6);
			}
			i ++;
		} else if (!strncmp("-b", argv[i], strlen("-b"))) {
			if (0x0 == argv[i][2]) {
				if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
					params->buffer_id = val;
					printf("[input argument]: '-b': (%d).\n", params->buffer_id);
				} else {
					printf("[input argument error]: '-b', should follow with integer(buffer id), argc %d, i %d.\n", argc, i);
					return (-7);
				}
				i ++;
			} else {
				if (1 == sscanf(argv[i], "-b%d", &val)) {
					params->buffer_id = val;
					printf("[input argument]: '-b%%d': (%d).\n", params->buffer_id);
				} else {
					printf("[input argument error]: '-b%%d', should follow with integer(buffer id), argc %d, i %d.\n", argc, i);
					return (-7);
				}
			}
		} else if (!strcmp("--filename", argv[i])) {
			if (((i + 1) < argc)) {
				snprintf(params->output_filenamebase, 512, "%s", argv[i + 1]);
				printf("[input argument]: '--filename': (%s).\n", params->output_filenamebase);
			} else {
				printf("[input argument error]: '--filename', should follow with output filename base, argc %d, i %d.\n", argc, i);
				return (-8);
			}
			i ++;
		} else if (!strcmp("-f", argv[i])) {
			if (((i + 1) < argc)) {
				snprintf(params->output_filenamebase, 512, "%s", argv[i + 1]);
				printf("[input argument]: '-f': (%s).\n", params->output_filenamebase);
			} else {
				printf("[input argument error]: '-f', should follow with output filename base, argc %d, i %d.\n", argc, i);
				return (-9);
			}
			i ++;
		} else if (!strcmp("--framecount", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->tot_frames = val;
				printf("[input argument]: '--framecount': (%d).\n", params->tot_frames);
			} else {
				printf("[input argument error]: '--framecount', should follow with integer(total frame count), argc %d, i %d.\n", argc, i);
				return (-10);
			}
			i ++;
		} else if (!strcmp("--block", argv[i])) {
			params->b_non_block_mode = 0;
			printf("[input argument]: '--block'\n");
		} else if (!strcmp("--nonblock", argv[i])) {
			params->b_non_block_mode = 1;
			printf("[input argument]: '--nonblock'\n");
		} else if (!strcmp("--bitrate", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->bitrate = val;
				printf("[input argument]: '--bitrate': (%d).\n", params->bitrate);
			} else {
				printf("[input argument error]: '--bitrate', should follow with integer(bitrate), argc %d, i %d.\n", argc, i);
				return (-11);
			}
			i ++;
		} else if (!strcmp("--bframes", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->bitrate = val;
				printf("[input argument]: '--bframes': (%d).\n", params->bframes);
			} else {
				printf("[input argument error]: '--bframes', should follow with integer(bframes), argc %d, i %d.\n", argc, i);
				return (-11);
			}
			i ++;
		} else if (!strcmp("--frnum", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->framerate_num = val;
				printf("[input argument]: '--frnum': (%d).\n", params->framerate_num);
			} else {
				printf("[input argument error]: '--frnum', should follow with integer(framerate num), argc %d, i %d.\n", argc, i);
				return (-12);
			}
			i ++;
		} else if (!strcmp("--frden", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->framerate_den = val;
				printf("[input argument]: '--frden': (%d).\n", params->framerate_den);
			} else {
				printf("[input argument error]: '--frden', should follow with integer(framerate den), argc %d, i %d.\n", argc, i);
				return (-13);
			}
			i ++;
		} else if (!strcmp("--keymin", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->keyframe_min = val;
				printf("[input argument]: '--keymin': (%d).\n", params->keyframe_min);
			} else {
				printf("[input argument error]: '--keymin', should follow with integer(key min), argc %d, i %d.\n", argc, i);
				return (-14);
			}
			i ++;
		} else if (!strcmp("--keymax", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &val))) {
				params->keyframe_max = val;
				printf("[input argument]: '--keymax': (%d).\n", params->keyframe_max);
			} else {
				printf("[input argument error]: '--keymax', should follow with integer(key max), argc %d, i %d.\n", argc, i);
				return (-15);
			}
			i ++;
		} else if (!strcmp("--dumpinputyuv", argv[i])) {
			params->b_dump_input_yuv = 1;
		} else if (!strcmp("--help", argv[i])) {
			show_usage();
			return (-21);
		} else if (!strcmp("--silence", argv[i])) {
			params->b_silence = 1;
			printf("[input argument]: '--silence'\n");
		} else {
			printf("[input argument error]: unknwon input params, [%d][%s]\n", i, argv[i]);
			return (-20);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;
	_sw_encoding_params params;

	printf("[flow]: begin\n");

	default_params(&params);

	if (argc > 1) {
		if (0 > init_params(argc, argv, &params)) {
			printf("[error]: bad input params, exit\n");
			show_usage();
			return (-1);
		}
	} else {
		printf("[error]: should specify options (like buffer id, output filename, etc), typical usage: 'test_sw_encoding -b 1', 'test_sw_encoding -b 1 -f testhevc'.\n");
		show_usage();
		return (-1);
	}

	signal(SIGINT, sigstop);
	signal(SIGQUIT, sigstop);
	signal(SIGTERM, sigstop);

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		printf("[flow]: open iav fail, exit\n");
		return (-2);
	}

	if (map_buffer() < 0) {
		printf("[flow]: map_buffer() fail, exit\n");
		return (-3);
	}

	if (check_state() < 0) {
		printf("[flow]: check_state() fail, exit\n");
		return (-4);
	}

	if (1) {
		ret = test_sw_encoding(&params);
	} else {
		ret = test_sw_encoding_single_thread(&params);
	}
	printf("[flow]: end, test_sw_encoding() ret %d\n", ret);

	return ret;
}

