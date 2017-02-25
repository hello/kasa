/*
 * test_decode.c
 *
 * History:
 *	2015/01/27 - [Zhi He] create file for s2l
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
#include <sched.h>

#include <pthread.h>
#include <semaphore.h>

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

#include "codec_parser.h"
#include "playback_helper.h"

#define u_printf printf
#define u_printf_error printf
#define u_printf_debug printf

static int test_decode_running = 1;
static int test_decode_feed_bitstream_done = 0;
static int test_decode_1xfw_playback = 0;
static unsigned long test_decode_last_timestamp = 0;

#define M_MSG_KILL	0

#define M_MSG_FW_FROM_BEGIN 1
#define M_MSG_BW_FROM_END 2
#define M_MSG_FW_FROM_CUR_GOP 3
#define M_MSG_BW_FROM_CUR_GOP 4

#define D_FILE_READ_BUF_SIZE (1 * 1024 * 1024)
#define D_READ_COPY_BUF_SIZE (1 * 1024 * 1024)

typedef struct {
	int	cmd;
	void	*ptr;
	u8 arg1, arg2, arg3, arg4;
} msg_t;

#define MAX_MSGS	8

typedef struct {
	msg_t	msgs[MAX_MSGS];
	int	read_index;
	int	write_index;
	int	num_msgs;
	int	num_readers;
	int	num_writers;
	pthread_mutex_t mutex;
	pthread_cond_t cond_read;
	pthread_cond_t cond_write;
	sem_t	echo_event;
} msg_queue_t;

int msg_queue_init(msg_queue_t *q)
{
	q->read_index = 0;
	q->write_index = 0;
	q->num_msgs = 0;
	q->num_readers = 0;
	q->num_writers = 0;
	pthread_mutex_init(&q->mutex, NULL);
	pthread_cond_init(&q->cond_read, NULL);
	pthread_cond_init(&q->cond_write, NULL);
	sem_init(&q->echo_event, 0, 0);
	return 0;
}

int msg_queue_deinit(msg_queue_t *q)
{
	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->cond_read);
	pthread_cond_destroy(&q->cond_write);
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

void msg_queue_ack(msg_queue_t *q)
{
	sem_post(&q->echo_event);
}

void msg_queue_wait(msg_queue_t *q)
{
	sem_wait(&q->echo_event);
}

#define DAMBA_GOP_HEADER_LENGTH 22
#define DAMBA_RESERVED_SPACE 32

void _fill_amba_gop_header(u8 *p_gop_header, u32 frame_tick, u32 time_scale, u32 pts, u8 gopsize, u8 m)
{
	u32 tick_high = frame_tick;
	u32 tick_low = tick_high & 0x0000ffff;
	u32 scale_high = time_scale;
	u32 scale_low = scale_high & 0x0000ffff;
	u32 pts_high = 0;
	u32 pts_low = 0;

	tick_high >>= 16;
	scale_high >>= 16;

	p_gop_header[0] = 0; // start code prefix
	p_gop_header[1] = 0;
	p_gop_header[2] = 0;
	p_gop_header[3] = 1;

	p_gop_header[4] = 0x7a; // nal type
	p_gop_header[5] = 0x01; // version main
	p_gop_header[6] = 0x01; // version sub

	p_gop_header[7] = tick_high >> 10;
	p_gop_header[8] = tick_high >> 2;
	p_gop_header[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
	p_gop_header[10] = tick_low >> 3;

	p_gop_header[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
	p_gop_header[12] = scale_high >> 4;
	p_gop_header[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
	p_gop_header[14] = scale_low >> 5;

	p_gop_header[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
	p_gop_header[16] = pts_high >> 6;

	p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
	p_gop_header[18] = pts_low >> 7;
	p_gop_header[19] = (pts_low << 1) | 1;

	p_gop_header[20] = gopsize;
	p_gop_header[21] = (m & 0xf) << 4;
}

void _update_amba_gop_header(u8 *p_gop_header, u32 pts)
{
	u32 pts_high = (pts >> 16) & 0x0000ffff;
	u32 pts_low = pts & 0x0000ffff;

	p_gop_header[15] = (p_gop_header[15]  & 0xFC) | (pts_high >> 14);
	p_gop_header[16] = pts_high >> 6;

	p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
	p_gop_header[18] = pts_low >> 7;
	p_gop_header[19] = (pts_low << 1) | 1;
}

#define MAX_NUM_DECODER 1

typedef struct {
	int sink_id;
	int sink_type;
	int source_id;

	int rotate;
	int flip;
	int offset_x;
	int offset_y;
	int width;
	int height;

	unsigned int vout_mode;
} vout_device_info;

enum {
	FAST_NAVI_MODE_ALL_FRAME_FORWARD = 0x00,
	FAST_NAVI_MODE_I_FRAME_FORWARD = 0x01,

	FAST_NAVI_MODE_ALL_FRAME_BACKWARD = 0x02,
	FAST_NAVI_MODE_I_FRAME_BACKWARD = 0x03,

	PLAYBACK_PENDDING = 0x08,
	PLAYBACK_ERROR = 0x09,
	PLAYBACK_EXIT = 0x0a,
};

#define DMAX_TEST_FILE_NAME_LENGTH 512
struct test_decode_context;

typedef struct {
	//thread and msg_queue
	int index;
	msg_queue_t cmd_queue;
	pthread_t thread_id;

	//iav info
	struct iav_decoder_info decoder_info;

	// file reader
	char filename[DMAX_TEST_FILE_NAME_LENGTH];
	FILE* file_fd;
	void* prealloc_file_buffer;
	u32 rd_buffer_size;

	//trickplay
	u8 current_trickplay;
	u8 cur_pb_mode; // 0: all frame, 1: I only
	u16 cur_speed;

	//bit-stream buffer
	u8* p_bsb_start;
	u8* p_bsb_end;

	struct test_decode_context* parent;

	//fast navi
	fast_navi_file fast_navi;
	int navi_mode;

	//dump
	FILE* p_dump_input_file;
	int trickplay_index;
}test_decoder_instance_context;

typedef struct {
	int iav_fd;
	struct iav_decode_mode_config mode_config;

	u8 enable_debug_log;
	u8 specify_fps;
	u8 enable_prefetch;
	u8 prefetch_count;

	u8 is_fastnavi_mode;
	u8 dump_input_bitstream;
	u8 debug_read_direct_to_bsb;
	u8 debug_frames_per_interrupt;

	u16 daemon_pause_time;
	u8 daemon;
	u8 enable_cvbs;

	u8 enable_digital;
	u8 enable_hdmi;
	u8 vout_num;
	u8 enter_idle_flag; //timer mode

	u8 debug_use_dproc;
	u8 debug_return_idle;
	u8 reserved1;
	u8 reserved2;

	u32 max_bitrate;
	u32 gop_size;
	u32 framerate_den; //fps = 90000/framerate_den

	u16 max_width;
	u16 max_height;

	u8 *bsb_base;
	u32 bsb_size;

	vout_device_info vout_dev_info[DIAV_MAX_DECODE_VOUT_NUMBER];

	test_decoder_instance_context decoder[MAX_NUM_DECODER];

} test_decode_context;

static void __print_vout_info(int iav_fd)
{
	int					num;
	int					i;
	struct amba_vout_sink_info		sink_info;

	num = 0;
	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return;
	}
	if (num < 1) {
		u_printf("Please load vout driver!\n");
		return;
	}

	for (i = 0; i < num; i++) {
		sink_info.id = i;
		if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return;
		}
		u_printf("\tvout id %d, source id %d, sink_type %d, mode %08x\n", i, sink_info.source_id, sink_info.sink_type, sink_info.sink_mode.mode);
		u_printf("\tw %d, h %d, enable %d, rotate %d, flip %d\n", sink_info.sink_mode.video_size.vout_width, sink_info.sink_mode.video_size.vout_height, sink_info.sink_mode.video_en, sink_info.sink_mode.video_rotate, sink_info.sink_mode.video_flip);
	}

	return;
}

static int get_single_vout_info(int chan, int sink_type, vout_device_info* voutinfo, int iav_fd)
{
	int	num;
	int	i;
	struct amba_vout_sink_info	sink_info;

	num = 0;
	if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return -1;
	}
	if (num < 1) {
		u_printf("Please load vout driver!\n");
		return -2;
	}

	for (i = 0; i < num; i++) {
		sink_info.id = i;
		if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return -3;
		}

		if ((sink_info.state == AMBA_VOUT_SINK_STATE_RUNNING) && (sink_info.sink_type == sink_type) && (sink_info.source_id == chan)) {
			u_printf("find sink_type %x, source id %d\n", sink_info.sink_type, sink_info.source_id);
			if (voutinfo) {
				voutinfo->sink_id = sink_info.id;
				voutinfo->source_id = sink_info.source_id;
				voutinfo->sink_type = sink_info.sink_type;

				voutinfo->width = sink_info.sink_mode.video_size.vout_width;
				voutinfo->height = sink_info.sink_mode.video_size.vout_height;
				voutinfo->offset_x = sink_info.sink_mode.video_offset.offset_x;
				voutinfo->offset_y = sink_info.sink_mode.video_offset.offset_y;
				voutinfo->rotate = sink_info.sink_mode.video_rotate;
				voutinfo->flip = sink_info.sink_mode.video_flip;

				voutinfo->vout_mode = sink_info.sink_mode.mode;
				return 0;
			}
		}
	}

	return (-4);
}

unsigned char* _find_h264_sps_header(unsigned char* p, unsigned int len)
{
	while (len) {
		if (!p[0] && !p[1]) {
			if ((p[2] == 0x00) && (p[3] == 0x01) && ((p[4] & 0x1F) == 0x07)) {
				return p + 5;
			} else if ((p[2] == 0x01) && ((p[3] & 0x1F) == 0x07)) {
				return p + 4;
			}
		}
		++ p;
		len --;
	}
	return NULL;
}

#if 0
static void _print_memory(u8* p, u32 size)
{
	while (size > 7) {
		printf("%02x %02x %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		size -= 8;
	}

	if (size) {
		while (size) {
			printf("%02x ", p[0]);
			p ++;
			size --;
		}
		printf("\n");
	}
}
#endif

static void __print_first_and_last_8bytes(u8* p, u8* p_end, u8* buf_start, u8* buf_end, u32 size)
{
	u32 size1 = 0, size2 = 0;
	u8* ptmp = NULL;

	if ((p + 7) < buf_end) {
		u_printf("begin(size %d): %02x %02x %02x %02x, %02x %02x %02x %02x\n", size, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	} else {
		size1 = buf_end - p;
		size2 = 8 - size1;

		ptmp = p;
		u_printf("begin(size %d):", size);
		while (size1) {
			u_printf(" %02x", ptmp[0]);
			ptmp ++;
			size1 --;
		}

		ptmp = buf_start;
		while (size2) {
			u_printf(" %02x", ptmp[0]);
			ptmp ++;
			size2 --;
		}
		u_printf("\n");
	}

	if (p_end > (buf_start + 7)) {
		u_printf("end: %02x %02x %02x %02x, %02x %02x %02x %02x\n", p_end[-8], p_end[-7], p_end[-6], p_end[-5], p_end[-4], p_end[-3], p_end[-2], p_end[-1]);
	} else {
		size1 = p_end - buf_start;
		size2 = 8 - size1;

		ptmp = buf_end - size2;
		u_printf("end:");
		while (size2) {
			u_printf(" %02x", ptmp[0]);
			ptmp ++;
			size2 --;
		}

		ptmp = buf_start;
		while (size1) {
			u_printf(" %02x", ptmp[0]);
			ptmp ++;
			size1 --;
		}
		u_printf("\n");
	}

}

static int enter_decode_mode(test_decode_context* context, u16 max_width, u16 max_height, u32 max_bitrate, u8 support_ff_fb_bw, u8 use_dproc)
{
	struct iav_decode_mode_config *decode_mode = &context->mode_config;

	decode_mode->num_decoder = 1;
	decode_mode->debug_max_frame_per_interrupt = context->debug_frames_per_interrupt;
	decode_mode->debug_use_dproc = use_dproc;
	decode_mode->b_support_ff_fb_bw = support_ff_fb_bw;
	decode_mode->max_frm_width = max_width;
	decode_mode->max_frm_height = max_height;

	u_printf("[flow]: before IAV_IOC_ENTER_DECODE_MODE, max_width %d, max_height %d, max_bitrate %d Mbps, support fast fwbw %d\n", max_width, max_height, max_bitrate, support_ff_fb_bw);
	if (ioctl(context->iav_fd, IAV_IOC_ENTER_DECODE_MODE, decode_mode) < 0) {
		perror("IAV_IOC_ENTER_DECODE_MODE");
		u_printf("[error]: enter decode mode fail\n");
		return (-1);
	}

	if (decode_mode->debug_max_frame_per_interrupt) {
		u_printf("[debug config]: max frames per interrupt %d\n", decode_mode->debug_max_frame_per_interrupt);
	}

	return 0;
}

static int leave_decode_mode(test_decode_context* context)
{
	u_printf("[flow]: before IAV_IOC_LEAVE_DECODE_MODE\n");
	if (ioctl(context->iav_fd, IAV_IOC_LEAVE_DECODE_MODE) < 0) {
		perror("IAV_IOC_LEAVE_DECODE_MODE");
		u_printf("[error]: leave decode mode fail\n");
		return (-1);
	}

	return 0;
}

static int enter_idle(int fd)
{
	u_printf("[flow]: before enter idle\n");

	if (ioctl(fd, IAV_IOC_ENTER_IDLE) < 0) {
		u_printf("[flow]: enter idle fail\n");
		perror("IAV_IOC_ENTER_IDLE");
		return (-1);
	}

	u_printf("[flow]: enter idle done\n");

	return 0;
}

static int ioctl_create_decoder(int iav_fd, struct iav_decoder_info *decode_info)
{
	u_printf("[flow]: before IAV_IOC_CREATE_DECODER\n");
	if (ioctl(iav_fd, IAV_IOC_CREATE_DECODER, decode_info) < 0) {
		perror("IAV_IOC_CREATE_DECODER");
		u_printf("[error]: create decoder fail\n");
		return (-1);
	}

	return 0;
}

static int ioctl_destroy_decoder(int iav_fd, u8 decoder_id)
{
	u_printf("[flow]: before IAV_IOC_DESTROY_DECODER\n");
	if (ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id) < 0) {
		perror("IAV_IOC_DESTROY_DECODER");
		u_printf("[error]: destroy decoder fail\n");
		return (-1);
	}

	return 0;
}

static void __open_dump_file(test_decoder_instance_context* instance_content, const char* file_tail)
{
	if (instance_content->p_dump_input_file) {
		fclose(instance_content->p_dump_input_file);
		instance_content->p_dump_input_file = NULL;
	}

	char dump_filename[DMAX_TEST_FILE_NAME_LENGTH] = {0};
	snprintf(dump_filename, DMAX_TEST_FILE_NAME_LENGTH, "%s_%04d.%s", instance_content->filename, instance_content->trickplay_index, file_tail);
	instance_content->trickplay_index ++;

	instance_content->p_dump_input_file = fopen(dump_filename, "wb+");
	if (!instance_content->p_dump_input_file) {
		u_printf("[error]: open dump file [%s] fail, please check if the filename is valid.\n", dump_filename);
		return;
	}
}

static int create_decoder(test_decode_context* context, u8 decoder_id, u8 decoder_type)
{
	struct iav_decoder_info *decode_info = &context->decoder[decoder_id].decoder_info;
	test_decoder_instance_context* decoder_context = &context->decoder[decoder_id];
	int vout_index = 0;
	unsigned char* p_sps = NULL;
	unsigned int clip_width = 0, clip_height = 0;

	decoder_context->current_trickplay = IAV_TRICK_PLAY_RESUME;

	msg_queue_init(&decoder_context->cmd_queue);
	decoder_context->file_fd = fopen(decoder_context->filename, "rb");
	if (!decoder_context->file_fd) {
		u_printf("[error]: open file [%s] fail, please check if the filename is valid.\n", decoder_context->filename);
		return (-1);
	}

	decoder_context->rd_buffer_size = D_FILE_READ_BUF_SIZE;
	decoder_context->prealloc_file_buffer = malloc(decoder_context->rd_buffer_size);
	if (!decoder_context->prealloc_file_buffer) {
		u_printf("[error]: malloc fail, request size %d.\n", decoder_context->rd_buffer_size);
		return (-2);
	}

	//parse sps to get bit-stream width and height
	fread(decoder_context->prealloc_file_buffer, 1, 8 * 1024, decoder_context->file_fd);
	p_sps = _find_h264_sps_header(decoder_context->prealloc_file_buffer, 8 * 1024);
	if (!p_sps) {
		u_printf("[error]: can not find sps in file, not valid h264 es file?.\n");
		return (-3);
	}
	if (get_h264_width_height_from_sps(p_sps, 256, &clip_width, &clip_height) < 0) {
		u_printf("[error]: parse sps fail? use default 1920x1080\n");
		clip_width = 1920;
		clip_height = 1080;
	} else {
		u_printf("parse h264 sps, clip width %d, height %d\n", clip_width, clip_height);
	}

	decode_info->decoder_id = decoder_id;
	decode_info->decoder_type = decoder_type;
	decode_info->num_vout = context->vout_num;
	decode_info->width = clip_width;
	decode_info->height = clip_height;
	for (vout_index = 0; vout_index < decode_info->num_vout; vout_index ++) {
		decode_info->vout_configs[vout_index].enable = 1;
		decode_info->vout_configs[vout_index].vout_id = context->vout_dev_info[vout_index].source_id;
		decode_info->vout_configs[vout_index].flip = context->vout_dev_info[vout_index].flip;
		decode_info->vout_configs[vout_index].rotate = context->vout_dev_info[vout_index].rotate;
		decode_info->vout_configs[vout_index].target_win_offset_x = context->vout_dev_info[vout_index].offset_x;
		decode_info->vout_configs[vout_index].target_win_offset_y = context->vout_dev_info[vout_index].offset_y;
		decode_info->vout_configs[vout_index].target_win_width = context->vout_dev_info[vout_index].width;
		decode_info->vout_configs[vout_index].target_win_height = context->vout_dev_info[vout_index].height;
		decode_info->vout_configs[vout_index].zoom_factor_x = (context->vout_dev_info[vout_index].width * 0x10000) / clip_width;
		decode_info->vout_configs[vout_index].zoom_factor_y = (context->vout_dev_info[vout_index].height * 0x10000) / clip_height;
		decode_info->vout_configs[vout_index].vout_mode = context->vout_dev_info[vout_index].vout_mode;
		vout_index ++;
	}

	if (ioctl_create_decoder(context->iav_fd, decode_info) < 0) {
		return (-4);
	}

	decoder_context->p_bsb_start = context->bsb_base + decode_info->bsb_start_offset;
	decoder_context->p_bsb_end = decoder_context->p_bsb_start + decode_info->bsb_size;

	u_printf("[debug], create_decoder done: p_bsb_start %p, p_bsb_end %p, bsb_size %d\n", decoder_context->p_bsb_start, decoder_context->p_bsb_end, decode_info->bsb_size);

	decoder_context->parent = (struct test_decode_context*) context;

	if (context->dump_input_bitstream) {
		__open_dump_file(decoder_context, "fw1x.dump");
	}

	return 0;
}

static int destroy_decoder(test_decode_context* context, u8 decoder_id)
{
	test_decoder_instance_context* decoder_context = &context->decoder[decoder_id];

	ioctl_destroy_decoder(context->iav_fd, decoder_id);

	if (decoder_context->prealloc_file_buffer) {
		free(decoder_context->prealloc_file_buffer);
		decoder_context->prealloc_file_buffer = NULL;
	}

	if (decoder_context->file_fd) {
		fclose(decoder_context->file_fd);
		decoder_context->file_fd = NULL;
	}

	if (decoder_context->p_dump_input_file) {
		fclose(decoder_context->p_dump_input_file);
		decoder_context->p_dump_input_file = NULL;
	}

	msg_queue_deinit(&decoder_context->cmd_queue);

	return 0;
}

static void _print_ut_options()
{
	u_printf("test_decode options:\n");
	u_printf("\t'-f [filename]': '-f' specify input file name\n");

	u_printf("\t'--help': print help\n\n");
	u_printf("\t'--idle': will enter idle state\n");
	//u_printf("\t'--daemon': will run as daemon, no user interact from stdin for this mode\n");
	u_printf("\t'--pausetime %%d': will pause for debug, after %%d seconds, for daemon mode only\n");

	u_printf("\t'--cvbs': will use cvbs output\n");
	u_printf("\t'--hdmi': will use HDMI output\n");
	u_printf("\t'--digital': will use digital (LCD) output\n");

	u_printf("\t'--returnidle': return idle mode for DSP after playback\n");
	u_printf("\t'--notreturnidle': not return idle mode for DSP after playback, keep playback mode\n");

	u_printf("\t'--fastnavi': will enable fast fw/bw and backwarrd playabck, with this mode, DSP will need more dram space\n");

	u_printf("\t'--maxwidth %%d': specify max video wdith\n");
	u_printf("\t'--maxheight %%d': specify max video height\n");
	u_printf("\t'--maxbitrate %%d': specify max bitrate\n");
	u_printf("\t'--gopsize %%d': specify gopsize\n");
	u_printf("\t'--frden %%d': specify framerate den, FPS = 90000/frden, for example, frden = 3003, FPS = 90000/3003 = 29.97\n");
	u_printf("\t'--prefetch %%d': prefetch %%d frames before start decode\n");
	u_printf("\t'--frameperinterrupt %%d': specify max frames per DSP interrupt\n");
}

static void _print_ut_cmds()
{
	u_printf("test_decode runtime cmds: press cmd + Enter\n");
	u_printf("\t'q': Quit unit test\n");
	u_printf("\t' ': pause/resume\n");
	u_printf("\t's': step play\n");
	u_printf("\t'f%%d': fast forward, %%d = 1 or 2, will choose all frame decoding, 4, 8 will choose I frame only\n");
	u_printf("\t'b%%d': fast backward, %%d = 1, will choose all frame decoding, 2, 4, 8 will choose I frame only\n");
}

static int init_test_decode_params(int argc, char **argv, test_decode_context* context)
{
	int i = 0;
	int ret = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp("--idle", argv[i])) {
			context->enter_idle_flag = 1;
			u_printf("[input argument] --idle, enter idle.\n");
		} else if (!strcmp("--daemon", argv[i])) {
			context->daemon = 1;
			u_printf("[input argument] --daemon, run as daemon.\n");
		} else if (!strcmp("--fastnavi", argv[i])) {
			context->is_fastnavi_mode = 1;
			u_printf("[input argument] --fastnavi, fast navi, build navigation tree first.\n");
		} else if (!strcmp("--dumpinput", argv[i])) {
			context->dump_input_bitstream = 1;
			u_printf("[input argument] --dumpinput, dump input bitsream.\n");
		} else if (!strcmp("--directread", argv[i])) {
			context->debug_read_direct_to_bsb = 1;
			u_printf("[input argument] --directread, direct read file data to bsb.\n");
		} else if (!strcmp("--cvbs", argv[i])) {
			context->enable_cvbs = 1;
			u_printf("[input argument] --cvbs, try cvbs output.\n");
		} else if (!strcmp("--digital", argv[i])) {
			context->enable_digital = 1;
			u_printf("[input argument] --digital, try digital output.\n");
		} else if (!strcmp("--hdmi", argv[i])) {
			context->enable_hdmi = 1;
			u_printf("[input argument] --hdmi, try hdmi output.\n");
		} else if (!strcmp("--returnidle", argv[i])) {
			context->debug_return_idle = 1;
			u_printf("[input argument] --returnidle, return idle after playabck.\n");
		} else if (!strcmp("--notreturnidle", argv[i])) {
			context->debug_return_idle = 0;
			u_printf("[input argument] --notreturnidle, keep playback mode after playabck.\n");
		} else if (!strcmp("--prefetch", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --prefetch, %d.\n", ret);
				context->prefetch_count = ret;
				context->enable_prefetch = 1;
				i ++;
			} else {
				u_printf("'error: [input argument] --prefetch should follow prefetch frame count (integer)'\n");
			}
		} else if (!strcmp("--pausetime", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --pausetime, %d.\n", ret);
				context->daemon_pause_time = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --pausetime should follow pause time (seconds), daemon mode only'\n");
			}
		} else if (!strcmp("--maxwidth", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --maxwidth, %d.\n", ret);
				context->max_width = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --maxwidth should follow width (integer)'\n");
			}
		} else if (!strcmp("--maxheight", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --maxheight, %d.\n", ret);
				context->max_height = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --maxheight should follow height (integer)'\n");
			}
		} else if (!strcmp("--maxbitrate", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --maxbirate, %d.\n", ret);
				context->max_bitrate = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --maxbitrate should follow bitrate (integer)'\n");
			}
		} else if (!strcmp("--gopsize", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --gopsize, %d.\n", ret);
				context->gop_size = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --maxgopsize should follow gopsize (integer)'\n");
			}
		} else if (!strcmp("--frden", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --frden, %d.\n", ret);
				context->framerate_den = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --frden should follow framerate den (integer)'\n");
			}
		} else if (!strcmp("--debuglog", argv[i])) {
			context->enable_debug_log = 1;
			u_printf("[input argument] --debuglog, enable debug log.\n");
		} else if (!strcmp("--frameperinterrupt", argv[i])) {
			if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
				u_printf("[input argument] --frameperinterrupt, %d.\n", ret);
				context->debug_frames_per_interrupt = ret;
				i ++;
			} else {
				u_printf("'error: [input argument] --frameperinterrupt should follow frames number (integer)'\n");
			}
		} else if (!strcmp("--help", argv[i])) {
			_print_ut_options();
			_print_ut_cmds();
		} else if(!strcmp("-f", argv[i])) {
			if ((i + 1) < argc) {
				snprintf(context->decoder[0].filename, DMAX_TEST_FILE_NAME_LENGTH, "%s", argv[i + 1]);
				i ++;
				u_printf("[input argument] -f: %s.\n", context->decoder[0].filename);
			} else {
				u_printf_error("[input argument] -f: should follow filename.\n");
			}
		} else if (!strcmp("--help", argv[i])) {
			_print_ut_options();
			_print_ut_cmds();
		} else {
			u_printf("error: NOT processed option(%s).\n", argv[i]);
			_print_ut_options();
			_print_ut_cmds();
			return (-1);
		}
	}

	return 0;
}

static int query_bsb_and_print(int iav_fd, u8 decoder_id)
{
	int ret;
	struct iav_decode_bsb bsb;

	memset(&bsb, 0x0, sizeof(bsb));
	bsb.decoder_id = decoder_id;

	ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
	if (ret < 0) {
		perror("IAV_IOC_QUERY_DECODE_BSB");
		return ret;
	}

	u_printf("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);

	return 0;
}

static int query_decode_status_and_print(int iav_fd, u8 decoder_id)
{
	int ret;
	struct iav_decode_status status;

	memset(&status, 0x0, sizeof(status));
	status.decoder_id = decoder_id;

	ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
	if (ret < 0) {
		perror("IAV_IOC_QUERY_DECODE_STATUS");
		return ret;
	}

	u_printf("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
	u_printf("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
	u_printf("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
	u_printf("[decode status, dram addr]: yuv422_y_addr 0x%08x, yuv422_uv_addr 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);

	return 0;
}

static int ioctl_decode_trick_play(int iav_fd, u8 decoder_id, u8 trick_play)
{
	int ret;
	struct iav_decode_trick_play trickplay;
	trickplay.decoder_id = decoder_id;
	trickplay.trick_play = trick_play;
	ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
	if (ret < 0) {
		perror("IAV_IOC_DECODE_TRICK_PLAY");
		return ret;
	}

	return 0;
}

static int ioctl_decode_start(int iav_fd, u8 decoder_id)
{
	int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);
	if (ret < 0) {
		perror("IAV_IOC_DECODE_START");
		return ret;
	}

	return 0;
}

static int ioctl_decode_stop(int iav_fd, u8 decoder_id, u8 stop_flag)
{
	int ret;
	struct iav_decode_stop stop;
	stop.decoder_id = decoder_id;
	stop.stop_flag = stop_flag;

	ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
	if (ret < 0) {
		perror("IAV_IOC_DECODE_STOP");
		return ret;
	}

	return 0;
}

static int ioctl_decode_speed(int iav_fd, u8 decoder_id, u16 speed, u8 scan_mode, u8 direction)
{
	int ret;
	struct iav_decode_speed spd;
	spd.decoder_id = decoder_id;
	spd.direction = direction;
	spd.speed = speed;
	spd.scan_mode = scan_mode;

	if ((ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd)) < 0) {
		perror("IAV_IOC_DECODE_SPEED");
		return -1;
	}

	return 0;
}

static unsigned char* copy_to_bsb(unsigned char *p_bsb_cur, unsigned char *buffer, unsigned int size, unsigned char* p_bsb_start, unsigned char* p_bsb_end)
{
	//printf("[debug], p_bsb_cur %p, buffer %p, size %d.    p_bsb_start %p, p_bsb_end %p\n", p_bsb_cur, buffer, size, p_bsb_start, p_bsb_end);
	if ((p_bsb_cur + size) <= p_bsb_end) {
		memcpy(p_bsb_cur, buffer, size);
		return p_bsb_cur + size;
	} else {
		int room = p_bsb_end - p_bsb_cur;
		unsigned char *ptr2;
		if (room) {
			memcpy(p_bsb_cur, buffer, room);
		}
		ptr2 = buffer + room;
		size -= room;
		if (size) {
			memcpy(p_bsb_start, ptr2, size);
		}
		return p_bsb_start + size;
	}
}

static unsigned char* copy_filedata_to_bsb(FILE* fd, unsigned long size, unsigned long file_offset, unsigned long* cur_file_offset, unsigned char *p_bsb_cur, unsigned char* p_bsb_start, unsigned char* p_bsb_end)
{
	//if (file_offset != (*cur_file_offset)) {
		//printf("[seek file]: ori offset %ld, current offset %ld\n", *cur_file_offset, file_offset);
		fseek(fd, file_offset, SEEK_SET);
	//}

	if ((p_bsb_cur + size) <= p_bsb_end) {
		fread(p_bsb_cur, 1, size, fd);
		*cur_file_offset = file_offset + size;
		return p_bsb_cur + size;
	} else {
		unsigned long room = p_bsb_end - p_bsb_cur;
		if (room) {
			fread(p_bsb_cur, 1, room, fd);
		}
		size -= room;
		if (size) {
			fread(p_bsb_start, 1, size, fd);
		}
		*cur_file_offset = file_offset + size + room;
		return p_bsb_start + size;
	}
}

static void write_bsb_data(FILE* fd, unsigned char *p_data_start, unsigned char *p_data_end, unsigned char* p_bsb_start, unsigned char* p_bsb_end)
{
	if (p_data_start < p_data_end) {
		fwrite(p_data_start, 1, p_data_end - p_data_start, fd);
	} else {
		if (p_bsb_end > p_data_start) {
			fwrite(p_data_start, 1, p_bsb_end - p_data_start, fd);
		}
		if (p_data_end > p_bsb_start) {
			fwrite(p_bsb_start, 1, p_data_end - p_bsb_start, fd);
		}
	}
	fflush(fd);
}

static int request_bits_fifo(int iav_fd, int decoder_id, u32 size, u32 cur_pos_offset)
{
	struct iav_decode_bsb wait;
	int ret;

	wait.decoder_id = decoder_id;
	wait.room = size;
	wait.start_offset = cur_pos_offset;

	if ((ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait)) < 0) {
		u_printf("[error]: IAV_IOC_WAIT_DECODE_BSB fail, ret %d.\n", ret);
		perror("IAV_IOC_WAIT_DECODE_BSB");
		return ret;
	}

	return 0;
}

static int map_bitstream_buffer(test_decode_context* context)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_BSB;
	if (ioctl(context->iav_fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	context->bsb_size = querybuf.length;
	context->bsb_base = (u8 *) mmap(NULL, (size_t) context->bsb_size, PROT_WRITE, MAP_SHARED, context->iav_fd, (size_t) querybuf.offset);
	if (context->bsb_base == MAP_FAILED) {
		perror("mmap failed\n");
		return -1;
	}

	u_printf("bsb_mem = %p, size = %d\n", context->bsb_base, context->bsb_size);
	return 0;
}

static void test_decode_sig_stop(int a)
{
	test_decode_running = 0;
}

static int __get_current_timestamp(int iav_fd, u8 id, unsigned long *timestamp)
{
	struct iav_decode_status status;
	int ret = 0;

	memset(&status, 0x0, sizeof(status));
	status.decoder_id = id;

	ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
	if (ret < 0) {
		u_printf_error("error: IAV_IOC_QUERY_DECODE_STATUS fail, ret %d\n", ret);
		perror("IAV_IOC_QUERY_DECODE_STATUS");
		return ret;
	}

	*timestamp = status.last_pts;
	return 0;
}

static void* decoder_instance_h264_es_file(void* param)
{
	test_decoder_instance_context* instance_content = (test_decoder_instance_context*) param;
	test_decode_context* context = (test_decode_context*) instance_content->parent;
	int ret = 0;
	u32 sendsize = 0;
	u8 *p_frame_start = NULL;
	u8 *p_bsb_cur = instance_content->p_bsb_start;
	u8 nal_type, slice_type, need_more_data, wait_next_key_frame = 0;
	u8 file_eos = 0;
	u32 target_prefetch_count = context->prefetch_count;
	u32 current_prefetch_count = 0;
	u32 in_prefetching = context->enable_prefetch;
	u8* p_prefetching = NULL;
	msg_t msg;
	u32 frame_index = 0;
	u32 last_pts = 0;

	u8 amba_gop_header[DAMBA_GOP_HEADER_LENGTH] = {0};

	_t_file_reader file_reader;
	memset(&file_reader, 0x0, sizeof(file_reader));
	ret = file_reader_init(&file_reader, instance_content->file_fd, instance_content->rd_buffer_size, instance_content->prealloc_file_buffer);
	if (ret < 0) {
		u_printf_error("file_reader_init fail, ret %d\n", ret);
		test_decode_running = 0;
		return NULL;
	}

	file_reader_reset(&file_reader);

	file_reader_read_trunk(&file_reader);
	p_bsb_cur = instance_content->p_bsb_start;

	u_printf("[flow]: decoder_instance_h264_es_file begin, gopsize = %d, framerate den %d\n", context->gop_size, context->framerate_den);

	_fill_amba_gop_header(amba_gop_header, context->framerate_den, 90000, 0, context->gop_size, 1);
	frame_index = 0;

	while (1) {

		if (msg_queue_peek(&instance_content->cmd_queue, &msg)) {
			if (M_MSG_KILL == msg.cmd) {
				u_printf("recieve quit cmd, return.\n");
				ret = 0;
				break;
			}
		}

		sendsize = get_next_frame(file_reader.p_read_buffer_cur_start, file_reader.p_read_buffer_cur_end, &nal_type, &slice_type, &need_more_data);

		if (!sendsize && need_more_data) {
			if (!file_reader.file_remainning_size) {
				sendsize = file_reader.p_read_buffer_cur_end - file_reader.p_read_buffer_cur_start;
				file_eos = 1;
				if (sendsize <= 6) {
					u_printf("warning, discard last incomplete frame, size %d\n", sendsize);
					break;
				} else {
					u_printf("[flow]: file end, feed last frame\n");
				}
			} else {
				file_reader_read_trunk(&file_reader);
				continue;
			}
		}

		if (wait_next_key_frame && (5 > nal_type)) {
			u_printf("(decoder_id %d) skip frame(nal_type %d, slice_type %d) till IDR\n", instance_content->index, nal_type, slice_type);
			continue;
		}

		if (context->enable_debug_log) {
			u_printf("frame index %d, nal type %d, slice type %d\n", frame_index, nal_type, slice_type);
			//u_printf("sendsize %d, need_more_data %d\n", sendsize, need_more_data);
		}

		wait_next_key_frame = 0;
		p_frame_start = p_bsb_cur;
		//wrap case
		if (p_frame_start == instance_content->p_bsb_end) {
			p_frame_start = instance_content->p_bsb_start;
		}

		if (5 == nal_type) {
			//printf("key frame + gop header size %d\n", sendsize + DAMBA_GOP_HEADER_LENGTH);
			ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_GOP_HEADER_LENGTH + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
			//printf("[loop flow]: after request_bits_fifo\n");

			if (ret < 0) {
				u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
				break;
			}

			//printf("[debug], before copy_to_bsb, sendsize %d\n", sendsize);
			_update_amba_gop_header(amba_gop_header, (u32) (3003 * frame_index));
			p_bsb_cur = copy_to_bsb(p_bsb_cur, amba_gop_header, DAMBA_GOP_HEADER_LENGTH, instance_content->p_bsb_start, instance_content->p_bsb_end);
		} else {
			//printf("non-key frame size %d\n", sendsize);
			ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
			//printf("[loop flow]: after request_bits_fifo\n");

			if (ret < 0) {
				u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
				break;
			}
		}

		last_pts = 3003 * frame_index;
		frame_index ++;

		p_bsb_cur = copy_to_bsb(p_bsb_cur, file_reader.p_read_buffer_cur_start, sendsize, instance_content->p_bsb_start, instance_content->p_bsb_end);
		//printf("[debug], after copy_to_bsb\n");
		file_reader.p_read_buffer_cur_start += sendsize;
		file_reader.data_remainning_size_in_buffer -= sendsize;

		if (in_prefetching && (target_prefetch_count > current_prefetch_count)) {
			if (!p_prefetching) {
				p_prefetching = p_frame_start;
				u_printf_debug("[prebuffering start], target_prefetch_count %d, p_prefetching %p\n", target_prefetch_count, p_prefetching);
			}

			current_prefetch_count ++;
			u_printf_debug("[prebuffering]: current_prefetch_count %d, target_prefetch_count %d\n", current_prefetch_count, target_prefetch_count);
			if (target_prefetch_count == current_prefetch_count) {
				u_printf_debug("[prebuffering done]\n");
				in_prefetching = 0;

				struct iav_decode_video decode_video;
				memset(&decode_video, 0, sizeof(decode_video));
				decode_video.decoder_id = instance_content->index;
				decode_video.num_frames = current_prefetch_count;

				decode_video.start_ptr_offset = p_prefetching - instance_content->p_bsb_start;
				decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;

				if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
					u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
					ret = -11;
					break;
				}
				if (instance_content->p_dump_input_file) {
					write_bsb_data(instance_content->p_dump_input_file, p_prefetching, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
				}
			}
		} else {
			struct iav_decode_video decode_video;
			memset(&decode_video, 0, sizeof(decode_video));
			decode_video.decoder_id = instance_content->index;
			decode_video.num_frames = 1;

			decode_video.start_ptr_offset = p_frame_start - instance_content->p_bsb_start;
			decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;

			if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
				u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
				ret = -11;
				break;
			}
			//printf("[loop flow]: after IAV_IOC_DECODE_VIDEO\n");

			if (context->enable_debug_log) {
				u32 data_size = 0;
				if (p_bsb_cur > p_frame_start) {
					data_size = (u32) (p_bsb_cur - p_frame_start);
				} else {
					data_size = (u32) (instance_content->p_bsb_end - p_frame_start) + (u32) (p_bsb_cur - instance_content->p_bsb_start);
				}
				__print_first_and_last_8bytes((u8*)p_frame_start, (u8*)p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end, data_size);
			}

			if (instance_content->p_dump_input_file) {
				write_bsb_data(instance_content->p_dump_input_file, p_frame_start, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
			}
		}

		if (file_eos) {
			u_printf("[feeding flow]: exit feeding loop\n");
			ret = 0;
			break;
		}

	}

	file_reader_deinit(&file_reader);
	test_decode_last_timestamp = last_pts;
	test_decode_feed_bitstream_done = 1;
	u_printf("[feeding flow]: decoder_instance_h264_es_file end, last timestamp %ld\n", test_decode_last_timestamp);

	return NULL;
}

//return 0: keep going, 1: change state, 2: exit
static int _process_cmd(test_decoder_instance_context* instance_content, test_decode_context* context, msg_t* msg)
{
	if (M_MSG_KILL == msg->cmd) {
		u_printf("recieve quit cmd, return.\n");
		instance_content->navi_mode = PLAYBACK_EXIT;
		return 2;
	} else if (M_MSG_FW_FROM_BEGIN == msg->cmd) {

		if ((FAST_NAVI_MODE_ALL_FRAME_BACKWARD == instance_content->navi_mode) || (FAST_NAVI_MODE_I_FRAME_BACKWARD == instance_content->navi_mode)) {
			u_printf("backward ---> forward\n");
			ioctl_destroy_decoder(context->iav_fd, instance_content->index);
			ioctl_create_decoder(context->iav_fd, &context->decoder[instance_content->index].decoder_info);
			ioctl_decode_start(context->iav_fd, instance_content->index);
		} else {
			u_printf("forward---> forward\n");
			ioctl_decode_stop(context->iav_fd, instance_content->index, 1);
			ioctl_decode_start(context->iav_fd, instance_content->index);
		}

		if ((1 == msg->arg1) || (2 == msg->arg1)) {
			if (1 == msg->arg1) {
				u_printf("receive 1x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x100, IAV_PB_SCAN_MODE_ALL_FRAMES, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw1x.dump");
				}
			} else {
				u_printf("receive 2x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x200, IAV_PB_SCAN_MODE_ALL_FRAMES, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw2x.dump");
				}
			}
			instance_content->navi_mode = FAST_NAVI_MODE_ALL_FRAME_FORWARD;
			return 1;
		} else {
			if (4 == msg->arg1) {
				u_printf("receive 4x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x400, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw4x.dump");
				}
			} else if (8 == msg->arg1) {
				u_printf("receive 8x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x800, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw8x.dump");
				}
			} else if (16 == msg->arg1) {
				u_printf("receive 16x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x1000, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw16x.dump");
				}
			} else if (32 == msg->arg1) {
				u_printf("receive 32x fw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x2000, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_FW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "fw32x.dump");
				}
			}
			instance_content->navi_mode = FAST_NAVI_MODE_I_FRAME_FORWARD;
			return 1;
		}
	} else if (M_MSG_BW_FROM_END == msg->cmd) {

		if ((FAST_NAVI_MODE_ALL_FRAME_FORWARD == instance_content->navi_mode) || (FAST_NAVI_MODE_I_FRAME_FORWARD == instance_content->navi_mode)) {
			u_printf("forward ---> backward\n");
			ioctl_destroy_decoder(context->iav_fd, instance_content->index);
			ioctl_create_decoder(context->iav_fd, &context->decoder[instance_content->index].decoder_info);
			ioctl_decode_start(context->iav_fd, instance_content->index);
		} else {
			u_printf("backward ---> backward\n");
			ioctl_decode_stop(context->iav_fd, instance_content->index, 1);
			ioctl_decode_start(context->iav_fd, instance_content->index);
		}

		if (1 == msg->arg1) {
			u_printf("receive 1x bw\n");
			ioctl_decode_speed(context->iav_fd, instance_content->index, 0x100, IAV_PB_SCAN_MODE_ALL_FRAMES, IAV_PB_DIRECTION_BW);
			instance_content->navi_mode = FAST_NAVI_MODE_ALL_FRAME_BACKWARD;
			if (context->dump_input_bitstream) {
				__open_dump_file(instance_content, "bw1x.dump");
			}
			return 1;
		} else {
			if (2 == msg->arg1) {
				u_printf("receive 2x bw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x200, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_BW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "bw2x.dump");
				}
			} else if (4 == msg->arg1) {
				u_printf("receive 4x bw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x400, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_BW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "bw4x.dump");
				}
			} else if (8 == msg->arg1) {
				u_printf("receive 8x bw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x800, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_BW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "bw8x.dump");
				}
			} else if (16 == msg->arg1) {
				u_printf("receive 16x bw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x1000, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_BW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "bw16x.dump");
				}
			} else if (32 == msg->arg1) {
				u_printf("receive 32x bw\n");
				ioctl_decode_speed(context->iav_fd, instance_content->index, 0x2000, IAV_PB_SCAN_MODE_I_ONLY, IAV_PB_DIRECTION_BW);
				if (context->dump_input_bitstream) {
					__open_dump_file(instance_content, "bw32x.dump");
				}
			}
			instance_content->navi_mode = FAST_NAVI_MODE_I_FRAME_BACKWARD;
			return 1;
		}
	} else {
		u_printf_error("not support cmd %d\n", msg->cmd);
	}

	return 0;
}

static void* decoder_instance_h264_es_file_fast_navi(void* param)
{
	test_decoder_instance_context* instance_content = (test_decoder_instance_context*) param;
	test_decode_context* context = (test_decode_context*) instance_content->parent;
	fast_navi_gop* p_gop;

	int ret = 0;
	unsigned long sendsize = 0;
	u32 file_read_buffer_size = D_READ_COPY_BUF_SIZE;
	u8* p_file_read_buffer = NULL;

	if (!context->debug_read_direct_to_bsb) {
		p_file_read_buffer = (u8*) malloc(file_read_buffer_size);
		if (!p_file_read_buffer) {
			u_printf("error: no memory\n");
			return NULL;
		}
	}

	u8 amba_gop_header[DAMBA_GOP_HEADER_LENGTH] = {0};

	u8 *p_frame_start = instance_content->p_bsb_start;
	u8 *p_bsb_cur = instance_content->p_bsb_start;
	u8 *p_bit_start = NULL, *p_bit_end = NULL;
	unsigned long cur_file_offset = 0;
	struct iav_decode_video decode_video;

	msg_t msg;
	int running = 1;
	u8 h264_eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};

	u32 gopsize = 15;
	if (instance_content->fast_navi.gop_list_header.p_next) {
		if (instance_content->fast_navi.gop_list_header.p_next->frame_count > 2) {
			gopsize = instance_content->fast_navi.gop_list_header.p_next->frame_count;
		}
	}
	u_printf("[flow]: decoder_instance_h264_es_file_fast_navi begin, gopsize = %d, framerate den %d\n", gopsize, context->framerate_den);

	_fill_amba_gop_header(amba_gop_header, context->framerate_den, 90000, 0, gopsize, 1);// hard code to 29.97 fps

	while (running) {

		switch (instance_content->navi_mode) {

			case FAST_NAVI_MODE_ALL_FRAME_FORWARD:
				p_gop = instance_content->fast_navi.gop_list_header.p_next;
				cur_file_offset = p_gop->gop_offset;
				fseek(instance_content->file_fd, cur_file_offset, SEEK_SET);
				p_frame_start = p_bsb_cur = instance_content->p_bsb_start;

				while (1) {
					if (msg_queue_peek(&instance_content->cmd_queue, &msg)) {
						ret = _process_cmd(instance_content, context, &msg);
						if (ret) {
							break;
						}
					}

					//printf("1, p_gop %p, next %p, header %p\n", p_gop, p_gop->p_next, &instance_content->fast_navi.gop_list_header);
					if (p_gop && p_gop != &instance_content->fast_navi.gop_list_header) {
						if (p_gop->p_next != &instance_content->fast_navi.gop_list_header) {
							sendsize = p_gop->p_next->gop_offset - p_gop->gop_offset;
							//printf("not end gop, size %ld, next offset %ld, current offset %ld\n", sendsize, p_gop->p_next->gop_offset, p_gop->gop_offset);
						} else {
							sendsize = instance_content->fast_navi.file_size - p_gop->gop_offset;
							//printf("end gop, size %ld, file size %ld, gop offset %ld\n", sendsize, instance_content->fast_navi.file_size, p_gop->gop_offset);
						}

						p_frame_start = p_bsb_cur;
						//wrap case
						if (p_frame_start == instance_content->p_bsb_end) {
							p_frame_start = instance_content->p_bsb_start;
						}
						//printf("wait bsb, start offset %08x, size %ld\n", p_frame_start - instance_content->p_bsb_start, sendsize);

						ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_GOP_HEADER_LENGTH + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
						if (ret < 0) {
							u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						_update_amba_gop_header(amba_gop_header, (u32) (3003 * p_gop->start_frame_index));
						p_bsb_cur = copy_to_bsb(p_bsb_cur, amba_gop_header, DAMBA_GOP_HEADER_LENGTH, instance_content->p_bsb_start, instance_content->p_bsb_end);
						p_bit_start = p_bsb_cur;
						if (context->debug_read_direct_to_bsb) {
							p_bit_end = p_bsb_cur = copy_filedata_to_bsb(instance_content->file_fd, sendsize, p_gop->gop_offset, &cur_file_offset, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
						} else {
							fseek(instance_content->file_fd, p_gop->gop_offset, SEEK_SET);
							fread(p_file_read_buffer, 1, sendsize, instance_content->file_fd);
							cur_file_offset = p_gop->gop_offset + sendsize;
							p_bit_end = p_bsb_cur = copy_to_bsb(p_bsb_cur, p_file_read_buffer, sendsize, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

						decode_video.decoder_id = instance_content->index;
						decode_video.num_frames = p_gop->frame_count;
						decode_video.start_ptr_offset = p_frame_start - instance_content->p_bsb_start;
						decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;
						decode_video.first_frame_display = (3003 * p_gop->start_frame_index);

						if (instance_content->p_dump_input_file) {
							write_bsb_data(instance_content->p_dump_input_file, p_bit_start, p_bit_end, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

#if 0
						u_printf("[fw, all frames]: IAV_IOC_DECODE_VIDEO, frame number %d, start offset %08x, end offset %08x, pts %d\n", decode_video.num_frames, decode_video.start_ptr_offset, decode_video.end_ptr_offset, decode_video.first_frame_display);
						if ((p_frame_start + DAMBA_GOP_HEADER_LENGTH + 256) <= instance_content->p_bsb_end) {
							_print_memory(p_frame_start, DAMBA_GOP_HEADER_LENGTH);
							_print_memory(p_frame_start + DAMBA_GOP_HEADER_LENGTH, 256);
						} else {
							_print_memory(p_frame_start, (instance_content->p_bsb_end - p_frame_start));
							_print_memory(instance_content->p_bsb_start, (DAMBA_GOP_HEADER_LENGTH + 256) - (instance_content->p_bsb_end - p_frame_start));
						}
#endif

						if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
							u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						p_gop = p_gop->p_next;
					} else {
						instance_content->navi_mode = PLAYBACK_PENDDING;
						u_printf("feed data end\n");
						p_gop = NULL;
						break;
					}
				}
				break;

			case FAST_NAVI_MODE_I_FRAME_FORWARD:
				p_gop = instance_content->fast_navi.gop_list_header.p_next;
				cur_file_offset = p_gop->gop_offset;
				fseek(instance_content->file_fd, cur_file_offset, SEEK_SET);
				p_frame_start = p_bsb_cur = instance_content->p_bsb_start;

				while (1) {

					if (msg_queue_peek(&instance_content->cmd_queue, &msg)) {
						ret = _process_cmd(instance_content, context, &msg);
						if (ret) {
							break;
						}
					}

					//printf("1, p_gop %p, next %p\n", p_gop, p_gop->p_next);
					if (p_gop && p_gop != &instance_content->fast_navi.gop_list_header) {
						sendsize = p_gop->frames[1].frame_offset - p_gop->frames[0].frame_offset;

						p_frame_start = p_bsb_cur;
						//wrap case
						if (p_frame_start == instance_content->p_bsb_end) {
							p_frame_start = instance_content->p_bsb_start;
						}

						ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_GOP_HEADER_LENGTH  + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
						if (ret < 0) {
							u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						_update_amba_gop_header(amba_gop_header, (u32) (3003 * p_gop->start_frame_index));
						p_bsb_cur = copy_to_bsb(p_bsb_cur, amba_gop_header, DAMBA_GOP_HEADER_LENGTH, instance_content->p_bsb_start, instance_content->p_bsb_end);
						p_bit_start = p_bsb_cur;
						if (context->debug_read_direct_to_bsb) {
							p_bit_end = p_bsb_cur = copy_filedata_to_bsb(instance_content->file_fd, sendsize, p_gop->gop_offset, &cur_file_offset, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
						} else {
							fseek(instance_content->file_fd, p_gop->gop_offset, SEEK_SET);
							fread(p_file_read_buffer, 1, sendsize, instance_content->file_fd);
							cur_file_offset = p_gop->gop_offset + sendsize;
							p_bit_end = p_bsb_cur = copy_to_bsb(p_bsb_cur, p_file_read_buffer, sendsize, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

						if (instance_content->p_dump_input_file) {
							write_bsb_data(instance_content->p_dump_input_file, p_bit_start, p_bit_end, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

						decode_video.decoder_id = instance_content->index;
						decode_video.num_frames = 1;
						decode_video.start_ptr_offset = p_frame_start - instance_content->p_bsb_start;
						decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;
						decode_video.first_frame_display = (3003 * p_gop->start_frame_index);

#if 0
						u_printf("[fw, I only]: IAV_IOC_DECODE_VIDEO, frame number %d, start offset %08x, end offset %08x, first pts %d\n", decode_video.num_frames, decode_video.start_ptr_offset, decode_video.end_ptr_offset, decode_video.first_frame_display);
						if ((p_frame_start + DAMBA_GOP_HEADER_LENGTH + 256) <= instance_content->p_bsb_end) {
							_print_memory(p_frame_start, DAMBA_GOP_HEADER_LENGTH);
							_print_memory(p_frame_start + DAMBA_GOP_HEADER_LENGTH, 256);
						} else {
							_print_memory(p_frame_start, (instance_content->p_bsb_end - p_frame_start));
							_print_memory(instance_content->p_bsb_start, (DAMBA_GOP_HEADER_LENGTH + 256) - (instance_content->p_bsb_end - p_frame_start));
						}
#endif

						if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
							u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						p_gop = p_gop->p_next;
					} else {
						instance_content->navi_mode = PLAYBACK_PENDDING;
						u_printf("feed data end\n");
						p_gop = NULL;
						break;
					}
				}
				break;

			case FAST_NAVI_MODE_ALL_FRAME_BACKWARD:
				p_gop = instance_content->fast_navi.gop_list_header.p_pre;
				cur_file_offset = p_gop->gop_offset;
				fseek(instance_content->file_fd, cur_file_offset, SEEK_SET);
				p_frame_start = p_bsb_cur = instance_content->p_bsb_start;

				while (1) {
					if (msg_queue_peek(&instance_content->cmd_queue, &msg)) {
						ret = _process_cmd(instance_content, context, &msg);
						if (ret) {
							break;
						}
					}

					//printf("1, p_gop %p, next %p, header %p\n", p_gop, p_gop->p_next, &instance_content->fast_navi.gop_list_header);
					if (p_gop && p_gop != &instance_content->fast_navi.gop_list_header) {
						if (p_gop->p_next != &instance_content->fast_navi.gop_list_header) {
							sendsize = p_gop->p_next->gop_offset - p_gop->gop_offset;
							//printf("not end gop, size %ld, next offset %ld, current offset %ld\n", sendsize, p_gop->p_next->gop_offset, p_gop->gop_offset);
						} else {
							sendsize = instance_content->fast_navi.file_size - p_gop->gop_offset;
							//printf("end gop, size %ld, file size %ld, gop offset %ld\n", sendsize, instance_content->fast_navi.file_size, p_gop->gop_offset);
						}

						p_frame_start = p_bsb_cur;
						//wrap case
						if (p_frame_start == instance_content->p_bsb_end) {
							p_frame_start = instance_content->p_bsb_start;
						}

						//printf("wait bsb, start offset %08x, size %ld\n", p_frame_start - instance_content->p_bsb_start, sendsize);

						ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_GOP_HEADER_LENGTH  + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
						if (ret < 0) {
							u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						_update_amba_gop_header(amba_gop_header, (u32) (3003 * p_gop->start_frame_index));
						p_bsb_cur = copy_to_bsb(p_bsb_cur, amba_gop_header, DAMBA_GOP_HEADER_LENGTH, instance_content->p_bsb_start, instance_content->p_bsb_end);
						p_bit_start = p_bsb_cur;
						if (context->debug_read_direct_to_bsb) {
							p_bit_end = p_bsb_cur = copy_filedata_to_bsb(instance_content->file_fd, sendsize, p_gop->gop_offset, &cur_file_offset, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
						} else {
							fseek(instance_content->file_fd, p_gop->gop_offset, SEEK_SET);
							fread(p_file_read_buffer, 1, sendsize, instance_content->file_fd);
							cur_file_offset = p_gop->gop_offset + sendsize;
							p_bit_end = p_bsb_cur = copy_to_bsb(p_bsb_cur, p_file_read_buffer, sendsize, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}
						p_bsb_cur = copy_to_bsb(p_bsb_cur, h264_eos, 5, instance_content->p_bsb_start, instance_content->p_bsb_end);

						decode_video.decoder_id = instance_content->index;
						decode_video.num_frames = p_gop->frame_count;
						decode_video.start_ptr_offset = p_frame_start - instance_content->p_bsb_start;
						decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;
						decode_video.first_frame_display = (u32) (3003 * (p_gop->start_frame_index + p_gop->frame_count));

						if (instance_content->p_dump_input_file) {
							write_bsb_data(instance_content->p_dump_input_file, p_bit_start, p_bit_end, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

#if 0
						u_printf("[bw, all frames]: IAV_IOC_DECODE_VIDEO, frame number %d, start offset %08x, end offset %08x, pts %d\n", decode_video.num_frames, decode_video.start_ptr_offset, decode_video.end_ptr_offset, decode_video.first_frame_display);
						if ((p_frame_start + DAMBA_GOP_HEADER_LENGTH + 256) <= instance_content->p_bsb_end) {
							_print_memory(p_frame_start, DAMBA_GOP_HEADER_LENGTH);
							_print_memory(p_frame_start + DAMBA_GOP_HEADER_LENGTH, 256);
						} else {
							_print_memory(p_frame_start, (instance_content->p_bsb_end - p_frame_start));
							_print_memory(instance_content->p_bsb_start, (DAMBA_GOP_HEADER_LENGTH + 256) - (instance_content->p_bsb_end - p_frame_start));
						}
#endif

						if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
							u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						p_gop = p_gop->p_pre;
					} else {
						instance_content->navi_mode = PLAYBACK_PENDDING;
						u_printf("feed data end\n");
						p_gop = NULL;
						break;
					}
				}
				break;

			case FAST_NAVI_MODE_I_FRAME_BACKWARD:
				p_gop = instance_content->fast_navi.gop_list_header.p_pre;
				cur_file_offset = p_gop->gop_offset;
				fseek(instance_content->file_fd, cur_file_offset, SEEK_SET);
				p_frame_start = p_bsb_cur = instance_content->p_bsb_start;

				while (1) {

					if (msg_queue_peek(&instance_content->cmd_queue, &msg)) {
						ret = _process_cmd(instance_content, context, &msg);
						if (ret) {
							break;
						}
					}

					//printf("1, p_gop %p, next %p\n", p_gop, p_gop->p_next);
					if (p_gop && p_gop != &instance_content->fast_navi.gop_list_header) {
						sendsize = p_gop->frames[1].frame_offset - p_gop->frames[0].frame_offset;

						p_frame_start = p_bsb_cur;
						//wrap case
						if (p_frame_start == instance_content->p_bsb_end) {
							p_frame_start = instance_content->p_bsb_start;
						}

						ret = request_bits_fifo(context->iav_fd, instance_content->index, sendsize + DAMBA_GOP_HEADER_LENGTH  + DAMBA_RESERVED_SPACE, p_frame_start - instance_content->p_bsb_start);
						if (ret < 0) {
							u_printf("[error]: request_bits_fifo() fail, ret %d\n", ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						_update_amba_gop_header(amba_gop_header, (u32) (3003 * p_gop->start_frame_index));
						p_bsb_cur = copy_to_bsb(p_bsb_cur, amba_gop_header, DAMBA_GOP_HEADER_LENGTH, instance_content->p_bsb_start, instance_content->p_bsb_end);
						p_bit_start = p_bsb_cur;
						if (context->debug_read_direct_to_bsb) {
							p_bit_end = p_bsb_cur = copy_filedata_to_bsb(instance_content->file_fd, sendsize, p_gop->gop_offset, &cur_file_offset, p_bsb_cur, instance_content->p_bsb_start, instance_content->p_bsb_end);
						} else {
							fseek(instance_content->file_fd, p_gop->gop_offset, SEEK_SET);
							fread(p_file_read_buffer, 1, sendsize, instance_content->file_fd);
							cur_file_offset = p_gop->gop_offset + sendsize;
							p_bit_end = p_bsb_cur = copy_to_bsb(p_bsb_cur, p_file_read_buffer, sendsize, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}
						p_bsb_cur = copy_to_bsb(p_bsb_cur, h264_eos, 5, instance_content->p_bsb_start, instance_content->p_bsb_end);

						decode_video.decoder_id = instance_content->index;
						decode_video.num_frames = 1;
						decode_video.start_ptr_offset = p_frame_start - instance_content->p_bsb_start;
						decode_video.end_ptr_offset = p_bsb_cur - instance_content->p_bsb_start;
						decode_video.first_frame_display = (u32) (3003 * p_gop->start_frame_index);

						if (instance_content->p_dump_input_file) {
							write_bsb_data(instance_content->p_dump_input_file, p_bit_start, p_bit_end, instance_content->p_bsb_start, instance_content->p_bsb_end);
						}

#if 0
						u_printf("[bw, I only]: IAV_IOC_DECODE_VIDEO, frame number %d, start offset %08x, end offset %08x, pts %d\n", decode_video.num_frames, decode_video.start_ptr_offset, decode_video.end_ptr_offset, decode_video.first_frame_display);
						if ((p_frame_start + DAMBA_GOP_HEADER_LENGTH + 256) <= instance_content->p_bsb_end) {
							_print_memory(p_frame_start, DAMBA_GOP_HEADER_LENGTH);
							_print_memory(p_frame_start + DAMBA_GOP_HEADER_LENGTH, 256);
						} else {
							_print_memory(p_frame_start, (instance_content->p_bsb_end - p_frame_start));
							_print_memory(instance_content->p_bsb_start, (DAMBA_GOP_HEADER_LENGTH + 256) - (instance_content->p_bsb_end - p_frame_start));
						}
#endif

						if ((ret = ioctl(context->iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video)) < 0) {
							u_printf_error("[ioctl error]: (decoder %d) IAV_IOC_DECODE_VIDEO ret %d, exit loop\n", instance_content->index, ret);
							instance_content->navi_mode = PLAYBACK_ERROR;
							running = 0;
							break;
						}

						p_gop = p_gop->p_pre;
					} else {
						instance_content->navi_mode = PLAYBACK_PENDDING;
						u_printf("feed data end\n");
						p_gop = NULL;
						break;
					}
				}
				break;

			case PLAYBACK_PENDDING:
				msg_queue_get(&instance_content->cmd_queue, &msg);
				printf("get cmd in pending\n");
				_process_cmd(instance_content, context, &msg);
				break;

			case PLAYBACK_ERROR:
				running = 0;
				break;

			case PLAYBACK_EXIT:
				running = 0;
				break;

			default:
				break;
		}

	}

	if (p_file_read_buffer) {
		free(p_file_read_buffer);
		p_file_read_buffer = NULL;
	}

	printf("[flow]: decoder_instance_h264_es_file_fast_navi end\n");

	return NULL;
}

static int test_decode(test_decode_context* context)
{
	unsigned long cur_timestamp = 0;
	unsigned long repeat_timestamp = 0;
	unsigned int repeat_times = 0;
	unsigned int i;
	msg_t msg;
	void* pv;
	int ret = 0;

	//disable fast fw, fast bw and backward playback
	u_printf("[config]: max width %d, max height %d, max bitrate %d, disable fast fw/bw and backward playabck\n", context->max_width, context->max_height, context->max_bitrate);
	ret = enter_decode_mode(context, context->max_width, context->max_height, context->max_bitrate, 0, 0);
	if (0 > ret) {
		return ret;
	}

	test_decode_1xfw_playback = 1;

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = create_decoder(context, i, IAV_DECODER_TYPE_H264);
		if (0 > ret) {
			return ret;
		}
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = ioctl_decode_start(context->iav_fd, i);
		if (0 > ret) {
			return ret;
		}
		ioctl_decode_speed(context->iav_fd, i, 0x100, IAV_PB_SCAN_MODE_ALL_FRAMES, IAV_PB_DIRECTION_FW);
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = pthread_create(&context->decoder[i].thread_id, NULL, decoder_instance_h264_es_file, (void*) &context->decoder[i]);
		if (ret) {
			perror("pthread_create");
			u_printf("[error]: pthread_create fail, ret %d\n", ret);
			if (0) {
				goto _test_decode_fail_;
			} else {
				decoder_instance_h264_es_file((void*) &context->decoder[i]);
				sleep(6);
				destroy_decoder(context, (u8) i);
				leave_decode_mode(context);
				return (-18);
			}
		}
	}


	if (context->daemon) {
		daemon(1, 0);
		u32 wait_times = 10 * context->daemon_pause_time;
		while (test_decode_running) {
			usleep(100000);
			if (context->daemon_pause_time) {
				if (0 == wait_times) {
					printf("pause at daemon mode\n");
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_PAUSE;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_PAUSE);
				} else {
					wait_times --;
				}
			}
		}
	} else {
		char buffer_old[128] = {0};
		char buffer[128];
		char* p_buffer = buffer;

		memset(buffer, 0x0, sizeof(buffer));
		memset(buffer_old, 0x0, sizeof(buffer_old));

		int flag_stdin = 0;
		flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
		if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1) {
			u_printf("[error]: stdin_fileno set error.\n");
		}

		while (test_decode_running) {
			usleep(100000);

			memset(buffer, 0x0, sizeof(buffer));
			if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {

				//check if last frame is displayed
				if (test_decode_feed_bitstream_done && test_decode_1xfw_playback) {
					ret = __get_current_timestamp(context->iav_fd, 0, &cur_timestamp);
					if (ret) {
						test_decode_running = 0;
						break;
					}
					if (((cur_timestamp + 6600) > test_decode_last_timestamp) && (cur_timestamp < (test_decode_last_timestamp + 6600))) {
						u_printf("receive last time stamp %ld, target %ld\n", cur_timestamp, test_decode_last_timestamp);
						test_decode_running = 0;
						break;
					}
					if (repeat_timestamp == cur_timestamp) {
						if (repeat_times > 1) {
							u_printf("receive repeat timestamp, last %ld, target %ld, gap %ld\n", cur_timestamp, test_decode_last_timestamp, test_decode_last_timestamp - cur_timestamp);
							test_decode_running = 0;
							break;
						}
						repeat_times ++;
					} else {
						repeat_times = 0;
					}
					repeat_timestamp = cur_timestamp;
				}

				continue;
			}

			if (buffer[0] == '\n') {
				p_buffer = buffer_old;
				u_printf("repeat last cmd:\n\t\t%s\n", buffer_old);
			} else if (buffer[0] == 'l') {
				u_printf("show last cmd:\n\t\t%s\n", buffer_old);
				continue;
			} else {
				p_buffer = buffer;
				//record last cmd
				strncpy(buffer_old, buffer, sizeof(buffer_old) -1);
				buffer_old[sizeof(buffer_old) -1] = 0x0;
			}

			if ('q' == p_buffer[0]) {
				u_printf("Quit, 'q'\n");
				test_decode_running = 0;
			} else if ('p' == p_buffer[0]) {
				if (0) {
					query_bsb_and_print(context->iav_fd, 0);
				} else {
					query_decode_status_and_print(context->iav_fd, 0);
				}
			} else if (' ' == p_buffer[0]) {
				if (IAV_TRICK_PLAY_RESUME == context->decoder[0].current_trickplay) {
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_PAUSE;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_PAUSE);
					test_decode_1xfw_playback = 0;
				} else {
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_RESUME;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_RESUME);
					test_decode_1xfw_playback = 1;
				}
			} else if ('s' == p_buffer[0]) {
				context->decoder[0].current_trickplay = IAV_TRICK_PLAY_STEP;
				ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_STEP);
				test_decode_1xfw_playback = 0;
			} else if ('f' == p_buffer[0]) {
				u_printf("not support fast navigation, you need add '--fastnavi'\n");
			} else if ('b' == p_buffer[0]) {
				u_printf("not support fast navigation, you need add '--fastnavi'\n");
			}
		}

		if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
			u_printf("[error]: stdin_fileno set error");
		}
	}

	u_printf("[flow]: start exit\n");

	//exit each threads
	msg.cmd = M_MSG_KILL;
	msg.ptr = NULL;
	for (i = 0; i < MAX_NUM_DECODER; i++) {
		msg_queue_put(&context->decoder[i].cmd_queue, &msg);
		ioctl_decode_stop(context->iav_fd, i, 1);
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		u_printf("[flow]: wait decoder_thread(%d) exit...\n", i);
		ret = pthread_join(context->decoder[i].thread_id, &pv);
		u_printf("[flow]: wait decoder_thread(%d) exit done, (ret %d).\n", i, ret);
	}

_test_decode_fail_:

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = destroy_decoder(context, (u8) i);
	}

	if (context->debug_return_idle) {
		leave_decode_mode(context);
		u_printf("[flow]: after leave decode mode\n");
	}

	return ret;
}

static int test_decode_fast_navi(test_decode_context* context)
{
	unsigned int i;
	msg_t msg;
	void* pv;
	int ret = 0;

	u_printf("[config]: max width %d, max height %d, max bitrate %d, enable fast fw/bw and backward playabck\n", context->max_width, context->max_height, context->max_bitrate);
	ret = enter_decode_mode(context, context->max_width, context->max_height, context->max_bitrate, 1, 0);
	if (0 > ret) {
		return ret;
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = create_decoder(context, i, IAV_DECODER_TYPE_H264);
		if (0 > ret) {
			return ret;
		}
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = ioctl_decode_start(context->iav_fd, i);
		if (0 > ret) {
			return ret;
		}
		// initial speed, playback mode
		ioctl_decode_speed(context->iav_fd, i, 0x100, IAV_PB_SCAN_MODE_ALL_FRAMES, IAV_PB_DIRECTION_FW);
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = build_fast_navi_tree(context->decoder[i].file_fd, &context->decoder[i].fast_navi);
		if (0 > ret) {
			return ret;
		}
		pthread_create(&context->decoder[i].thread_id, NULL, decoder_instance_h264_es_file_fast_navi, (void*) &context->decoder[i]);
	}

	if (context->daemon) {
		daemon(1, 0);
		u32 wait_times = 10 * context->daemon_pause_time;
		while (test_decode_running) {
			usleep(100000);
			if (context->daemon_pause_time) {
				if (0 == wait_times) {
					printf("pause at daemon mode\n");
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_PAUSE;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_PAUSE);
				} else {
					wait_times --;
				}
			}
		}
	} else {
		char buffer_old[128] = {0};
		char buffer[128];
		char* p_buffer = buffer;

		memset(buffer, 0x0, sizeof(buffer));
		memset(buffer_old, 0x0, sizeof(buffer_old));

		int flag_stdin = 0;
		flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
		if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
			u_printf("[error]: stdin_fileno set error.\n");
		}

		while (test_decode_running) {
			usleep(100000);
			memset(buffer, 0x0, sizeof(buffer));
			if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
				continue;
			}

			if (buffer[0] == '\n') {
				p_buffer = buffer_old;
				u_printf("repeat last cmd:\n\t\t%s\n", buffer_old);
			} else if (buffer[0] == 'l') {
				u_printf("show last cmd:\n\t\t%s\n", buffer_old);
				continue;
			} else {
				p_buffer = buffer;
				//record last cmd
				strncpy(buffer_old, buffer, sizeof(buffer_old) -1);
				buffer_old[sizeof(buffer_old) -1] = 0x0;
			}

			if ('q' == p_buffer[0]) {
				u_printf("Quit, 'q'\n");
				test_decode_running = 0;
			} else if ('p' == p_buffer[0]) {
				if (0) {
					query_bsb_and_print(context->iav_fd, 0);
				} else {
					query_decode_status_and_print(context->iav_fd, 0);
				}
			} else if (' ' == p_buffer[0]) {
				if (IAV_TRICK_PLAY_RESUME == context->decoder[0].current_trickplay) {
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_PAUSE;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_PAUSE);
				} else {
					context->decoder[0].current_trickplay = IAV_TRICK_PLAY_RESUME;
					ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_RESUME);
				}
			} else if ('s' == p_buffer[0]) {
				context->decoder[0].current_trickplay = IAV_TRICK_PLAY_STEP;
				ioctl_decode_trick_play(context->iav_fd, 0, IAV_TRICK_PLAY_STEP);
			} else if ('f' == p_buffer[0]) {
				if ('1' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_FW_FROM_BEGIN;
					msg.arg1 = 1;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('2' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_FW_FROM_BEGIN;
					msg.arg1 = 2;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('4' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_FW_FROM_BEGIN;
					msg.arg1 = 4;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('8' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_FW_FROM_BEGIN;
					msg.arg1 = 8;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				}
			} else if ('b' == p_buffer[0]) {
				if ('1' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_BW_FROM_END;
					msg.arg1 = 1;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('2' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_BW_FROM_END;
					msg.arg1 = 2;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('4' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_BW_FROM_END;
					msg.arg1 = 4;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				} else if ('8' == p_buffer[1]) {
					msg_t msg;
					msg.cmd = M_MSG_BW_FROM_END;
					msg.arg1 = 8;
					msg.arg2 = 0;
					msg_queue_put(&context->decoder[0].cmd_queue, &msg);
				}
			}
		}

		if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
			u_printf("[error]: stdin_fileno set error");
		}
	}

	u_printf("[flow]: start exit\n");

	//exit each threads
	msg.cmd = M_MSG_KILL;
	msg.ptr = NULL;
	for (i = 0; i < MAX_NUM_DECODER; i++) {
		msg_queue_put(&context->decoder[i].cmd_queue, &msg);
		ioctl_decode_stop(context->iav_fd, i, 1);
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		u_printf("[flow]: wait decoder_thread(%d) exit...\n", i);
		ret = pthread_join(context->decoder[i].thread_id, &pv);
		u_printf("[flow]: wait decoder_thread(%d) exit done, (ret %d).\n", i, ret);
	}

	for (i = 0; i < MAX_NUM_DECODER; i++) {
		ret = destroy_decoder(context, (u8) i);
	}

	if (context->debug_return_idle) {
		leave_decode_mode(context);
		u_printf("[flow]: after leave decode mode\n");
	}

	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	test_decode_context context;

	if (argc < 2) {
		_print_ut_options();
		_print_ut_cmds();
		return 1;
	}

	signal(SIGINT, test_decode_sig_stop);
	signal(SIGQUIT, test_decode_sig_stop);
	signal(SIGTERM, test_decode_sig_stop);

	u_printf("[flow]: test_decode start\n");

	memset(&context, 0x0, sizeof(context));
	context.iav_fd = -1;

	//default settings
	context.debug_read_direct_to_bsb = 1;
	context.max_width = 1920;
	context.max_height = 1088;
	context.gop_size = 30;
	context.framerate_den = 3003;

	if ((ret = init_test_decode_params(argc, argv, &context)) < 0) {
		u_printf_error("[error]: init_test_decode_params() fail.\n");
		return (-2);
	}

	if (!context.framerate_den) {
		u_printf_error("zero frame den, use 3003 as default\n");
		context.framerate_den = 3003;
	}

	context.iav_fd = open("/dev/iav", O_RDWR, 0);
	if (context.iav_fd < 0) {
		u_printf_error("[error]: open iav fail.\n");
		return (-3);
	}

	if (context.enter_idle_flag) {
		enter_idle(context.iav_fd);
		close(context.iav_fd);
		context.iav_fd = -1;
		return 0;
	}

	if (context.enable_debug_log) {
		__print_vout_info(context.iav_fd);
	}

	if (context.enable_cvbs) {
		ret = get_single_vout_info(1, AMBA_VOUT_SINK_TYPE_CVBS, &context.vout_dev_info[0], context.iav_fd);
		if (0 > ret) {
			u_printf("[error]: CVBS is not enabled\n");
			close(context.iav_fd);
			return (-4);
		} else {
			if (context.vout_dev_info[0].width && context.vout_dev_info[0].height) {
				u_printf("[vout info query]: CVBS width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
			} else {
				u_printf("[error]: CVBS is not configured\n");
				close(context.iav_fd);
				return (-4);
			}
		}
		context.vout_num = 1;
	} else if (context.enable_digital) {
		ret = get_single_vout_info(0, AMBA_VOUT_SINK_TYPE_DIGITAL, &context.vout_dev_info[0], context.iav_fd);
		if (0 > ret) {
			u_printf("[error]: Digital Vout is not enabled\n");
			close(context.iav_fd);
			return (-4);
		} else {
			if (context.vout_dev_info[0].width && context.vout_dev_info[0].height) {
				u_printf("[vout info query]: Digital Vout width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
			} else {
				u_printf("[error]: Digital Vout is not configured\n");
				close(context.iav_fd);
				return (-4);
			}
		}
		context.vout_num = 1;
	} else if (context.enable_hdmi) {
		ret = get_single_vout_info(1, AMBA_VOUT_SINK_TYPE_HDMI, &context.vout_dev_info[0], context.iav_fd);
		if (0 > ret) {
			u_printf("[error]: HDMI is not enabled\n");
			close(context.iav_fd);
			return (-4);
		} else {
			if (context.vout_dev_info[0].width && context.vout_dev_info[0].height) {
				u_printf("[vout info query]: HDMI width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
			} else {
				u_printf("[error]: HDMI is not configured\n");
				close(context.iav_fd);
				return (-4);
			}
		}
		context.vout_num = 1;
	} else {
		u_printf("try find default vout\n");
		ret = get_single_vout_info(1, AMBA_VOUT_SINK_TYPE_CVBS, &context.vout_dev_info[0], context.iav_fd);
		if ((!ret) && (context.vout_dev_info[0].width && context.vout_dev_info[0].height)) {
			u_printf("[vout info query]: CVBS width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
			context.enable_cvbs = 1;
		} else {
			ret = get_single_vout_info(0, AMBA_VOUT_SINK_TYPE_DIGITAL, &context.vout_dev_info[0], context.iav_fd);
			if ((!ret) && (context.vout_dev_info[0].width && context.vout_dev_info[0].height)) {
				u_printf("[vout info query]: Digital Vout width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
				context.enable_digital = 1;
			} else {
				ret = get_single_vout_info(1, AMBA_VOUT_SINK_TYPE_HDMI, &context.vout_dev_info[0], context.iav_fd);
				if ((!ret) && (context.vout_dev_info[0].width && context.vout_dev_info[0].height)) {
					u_printf("[vout info query]: HDMI width %d, height %d, offset_x %d, offset_y %d, rotate %d, flip %d\n", context.vout_dev_info[0].width, context.vout_dev_info[0].height, context.vout_dev_info[0].offset_x, context.vout_dev_info[0].offset_y, context.vout_dev_info[0].rotate, context.vout_dev_info[0].flip);
					context.enable_hdmi = 1;
				} else {
					u_printf("[error]: No Vout is configured\n");
					close(context.iav_fd);
					return (-4);
				}
			}
		}
		context.vout_num = 1;
	}

	ret = map_bitstream_buffer(&context);
	if (ret < 0) {
		u_printf_error("[error]: map_bitstream_buffer() fail.\n");
		return (-4);
	}

	if (!context.is_fastnavi_mode) {
		test_decode(&context);
	} else {
		test_decode_fast_navi(&context);
	}

	if (context.iav_fd > 0) {
		close(context.iav_fd);
		context.iav_fd = -1;
	}

	u_printf("[flow]: test_decode end\n");

	return 0;
}

