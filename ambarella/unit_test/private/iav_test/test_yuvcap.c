/*
 * test_yuvcap.c
 *
 * History:
 *	2012/02/09 - [Jian Tang] created file
 *	2014/04/25 - [Zhaoyang Chen] modified file
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
#include "datatx_lib.h"
#include <signal.h>


#define	MAX_SOURCE_BUFFER_NUM	(4)

#ifndef VERIFY_BUFFERID
#define VERIFY_BUFFERID(x)	do {		\
			if ((x) < 0 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf("Invalid buffer id %d.\n", (x));	\
				return -1;	\
			}	\
		} while (0)
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif

typedef enum {
	CAPTURE_NONE = 255,
	CAPTURE_PREVIEW_BUFFER = 0,
	CAPTURE_ME1_BUFFER,
	CAPTURE_ME0_BUFFER,
	CAPTURE_RAW_BUFFER,
	CAPTURE_TYPE_NUM,
} CAPTURE_TYPE;

#define MAX_YUV_BUFFER_SIZE		(4096*3000)		// 4096x3000
#define MAX_ME_BUFFER_SIZE		(MAX_YUV_BUFFER_SIZE / 16)	// 1/16 of 4096x3000
#define MAX_FRAME_NUM				(120)

#define YUVCAP_PORT					(2024)

typedef enum {
	YUV420_IYUV = 0,	// Pattern: YYYYYYYYUUVV
	YUV420_YV12 = 1,	// Pattern: YYYYYYYYVVUU
	YUV420_NV12 = 2,	// Pattern: YYYYYYYYUVUV
	YUV422_YU16 = 3,	// Pattern: YYYYYYYYUUUUVVVV
	YUV422_YV16 = 4,	// Pattern: YYYYYYYYVVVVUUUU
	YUV_FORMAT_TOTAL_NUM,
	YUV_FORMAT_FIRST = YUV420_IYUV,
	YUV_FORMAT_LAST = YUV_FORMAT_TOTAL_NUM,
} YUV_FORMAT;

typedef struct {
	u8 *in;
	u8 *u;
	u8 *v;
	u32 row;
	u32 col;
	u32 pitch;
} yuv_neon_arg;

int fd_iav;

static int transfer_method = TRANS_METHOD_NFS;
static int port = YUVCAP_PORT;

static int current_buffer = -1;
static int capture_select = 0;
static int non_block_read = 0;

static int yuv_buffer_id = 0;
static int yuv_format = YUV420_IYUV;

static int me1_buffer_id = 0;
static int me0_buffer_id = 0;
static int frame_count = 1;
static int quit_capture = 0;
static int verbose = 0;
static int delay_frame_cap_data = 0;
static int G_multi_vin_num = 1;

const char *default_filename_nfs = "/mnt/media/test.yuv";
const char *default_filename_tcp = "media/test";
const char *default_host_ip_addr = "10.0.0.1";
const char *default_filename;
static char filename[256];
static int fd_yuv[MAX_SOURCE_BUFFER_NUM];
static int fd_me[MAX_SOURCE_BUFFER_NUM];
static int fd_raw = -1;

static u8 *dsp_mem = NULL;
static u32 dsp_size = 0;
static u32 dsp_partition_map = 0;
static u8* dsp_sub_mem[IAV_DSP_SUB_BUF_NUM];
static u32 dsp_sub_size[IAV_DSP_SUB_BUF_NUM];

static struct timeval pre = {0, 0}, curr = {0, 0};

extern void chrome_convert(yuv_neon_arg *);

static int map_sub_buffers(void)
{
	struct iav_query_subbuf query;
	int i;

	query.buf_id = IAV_BUFFER_DSP;
	query.sub_buf_map = dsp_partition_map;
	if (ioctl(fd_iav, IAV_IOC_QUERY_SUB_BUF, &query) < 0) {
		perror("IAV_IOC_QUERY_SUB_BUF");
		return -1;
	}

	memset(dsp_sub_mem, 0, sizeof(dsp_sub_mem));
	for (i = 0; i < IAV_DSP_SUB_BUF_NUM; ++i) {
		if (query.sub_buf_map & (1 << i)) {
			dsp_sub_size[i] = query.length[i];
			dsp_sub_mem[i] = mmap(NULL, dsp_sub_size[i], PROT_READ, MAP_SHARED,
				fd_iav, query.daddr[i]);
			if (dsp_sub_mem[i] == MAP_FAILED) {
				perror("mmap (%d) failed: %s\n");
				return -1;
			}
		}
	}

	return 0;
}

static int map_dsp_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ, MAP_SHARED, fd_iav,
		querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int map_buffer(void)
{
	struct iav_system_resource resource;
	int ret = 0;

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}

	dsp_partition_map = resource.dsp_partition_map;

	if (dsp_partition_map) {
		ret = map_sub_buffers();
	} else {
		ret = map_dsp_buffer();
	}

	return ret;
}

static u8* get_buffer_base(int buf_id, int me1_flag)
{
	u8* addr = NULL;

	if (dsp_partition_map) {
		switch (buf_id) {
		case IAV_SRCBUF_MN:
			if (me1_flag) {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_MAIN_ME1];
			} else {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_MAIN_YUV];
			}
			break;
		case IAV_SRCBUF_PA:
			if (me1_flag) {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_A_ME1];
			} else {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_A_YUV];
			}
			break;
		case IAV_SRCBUF_PB:
			if (me1_flag) {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_B_ME1];
			} else {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_B_YUV];
			}
			break;
		case IAV_SRCBUF_PC:
			if (me1_flag) {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_C_ME1];
			} else {
				addr = dsp_sub_mem[IAV_DSP_SUB_BUF_PREV_C_YUV];
			}
			break;
		default:
			printf("Unsupported buffer ID %d!\n", buf_id);
			break;
		}
	} else {
		addr = dsp_mem;
	}

	return addr;
}

static int save_yuv_luma_buffer(u8* output, struct iav_yuvbufdesc *yuv_desc)
{
	int i;
	u8 *in = NULL;
	u8 *out = NULL;
	u8 *base = NULL;

	if (yuv_desc->pitch < yuv_desc->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	base = get_buffer_base(yuv_desc->buf_id, 0);

	if (base == NULL) {
		printf("Invalid buffer address for buffer %d YUV!"
			" Map YUV buffer from DSP first.\n", yuv_desc->buf_id);
		return -1;
	}

	if (yuv_desc->pitch == yuv_desc->width) {
		memcpy(output, base + yuv_desc->y_addr_offset,
			yuv_desc->width * yuv_desc->height);
	} else {
		in = base + yuv_desc->y_addr_offset;
		out = output;
		for (i = 0; i < yuv_desc->height; i++) {		//row
			memcpy(out, in, yuv_desc->width);
			in += yuv_desc->pitch;
			out += yuv_desc->width;
		}
	}

	return 0;
}

static int save_yuv_chroma_buffer(u8* output, struct iav_yuvbufdesc *yuv_desc)
{
	int width, height, pitch;
	u8* input = NULL;
	u8 *base = NULL;
	int i;
	yuv_neon_arg yuv;
	int ret = 0;

	base = get_buffer_base(yuv_desc->buf_id, 0);

	if (base == NULL) {
		printf("Invalid buffer address for buffer %d YUV!"
			" Map YUV buffer from DSP first.\n", yuv_desc->buf_id);
		return -1;
	}

	// input yuv is uv interleaved with padding (uvuvuvuv.....)
	if (yuv_desc->format == IAV_YUV_FORMAT_YUV420) {
		width = yuv_desc->width / 2;
		height = yuv_desc->height / 2;
		pitch = yuv_desc->pitch / 2;
		yuv.in = base + yuv_desc->uv_addr_offset;
		yuv.row = height;
		yuv.col = yuv_desc->width;
		yuv.pitch = yuv_desc->pitch;
		if (yuv_format == YUV420_YV12) {
			// YV12 format (YYYYYYYYVVUU)
			yuv.u = output + width * height;
			yuv.v = output;
			chrome_convert(&yuv);
		} else if (yuv_format == YUV420_IYUV) {
			// IYUV (I420) format (YYYYYYYYUUVV)
			yuv.u = output;
			yuv.v = output + width * height;
			chrome_convert(&yuv);
		} else {
			if (yuv_format != YUV420_NV12) {
				printf("Change output format back to NV12 for encode!\n");
				yuv_format = YUV420_NV12;
			}
			// NV12 format (YYYYYYYYUVUV)
			input = base + yuv_desc->uv_addr_offset;
			for (i = 0; i < height; ++i) {
				memcpy(output + i * width * 2, input + i * pitch * 2, width * 2);
			}
		}
	} else if (yuv_desc->format == IAV_YUV_FORMAT_YUV422) {
		width = yuv_desc->width / 2;
		height = yuv_desc->height;
		pitch = yuv_desc->pitch / 2;
		yuv.in = base + yuv_desc->uv_addr_offset;
		yuv.row = height;
		yuv.col = yuv_desc->width;
		yuv.pitch = yuv_desc->pitch;
		if (yuv_format == YUV422_YU16) {
			yuv.u = output;
			yuv.v = output + width * height;
		} else {
			if (yuv_format != YUV422_YV16) {
				printf("Change output format back to YV16 for preview!\n");
				yuv_format = YUV422_YV16;
			}
			yuv.u = output + width * height;
			yuv.v = output;
		}
		chrome_convert(&yuv);
	} else {
		printf("Error: Unsupported YUV input format!\n");
		ret = -1;
	}

	return ret;
}

static int save_yuv_data(int fd, int buffer, struct iav_yuvbufdesc *yuv_desc,
	u8 * luma, u8 * chroma)
{
	static int pts_prev = 0, pts = 0;
	int luma_size, chroma_size;

	luma_size = yuv_desc->width * yuv_desc->height;
	if (yuv_desc->format == IAV_YUV_FORMAT_YUV420) {
		chroma_size = luma_size / 2;
	} else if (yuv_desc->format == IAV_YUV_FORMAT_YUV422) {
		chroma_size = luma_size;
	} else {
		printf("Error: Unrecognized yuv data format from DSP!\n");
		return -1;
	}

	/* Save YUV data into memory */
	if (verbose) {
		gettimeofday(&curr, NULL);
		pre = curr;
	}
	if (save_yuv_luma_buffer(luma, yuv_desc) < 0) {
		perror("Failed to save luma data into buffer !\n");
		return -1;
	}
	if (verbose) {
		gettimeofday(&curr, NULL);
		printf("22. Save Y [%06ld us].\n", (curr.tv_sec - pre.tv_sec) *
			1000000 + (curr.tv_usec - pre.tv_usec));
		pre = curr;
	}
	if (save_yuv_chroma_buffer(chroma, yuv_desc) < 0) {
		perror("Failed to save chroma data into buffer !\n");
		return -1;
	}
	if (verbose) {
		gettimeofday(&curr, NULL);
		printf("33. Save UV [%06ld us].\n", (curr.tv_sec - pre.tv_sec) *
			1000000 + (curr.tv_usec - pre.tv_usec));
	}

	/* Write YUV data from memory to file */
	if (amba_transfer_write(fd, luma, luma_size, transfer_method) < 0) {
		perror("Failed to save luma data into file !\n");
		return -1;
	}
	if (amba_transfer_write(fd, chroma,	chroma_size, transfer_method) < 0) {
		perror("Failed to save chroma data into file !\n");
		return -1;
	}

	if (verbose) {
		pts = yuv_desc->mono_pts;
		printf("BUF [%d] Y 0x%08x, UV 0x%08x, pitch %u, %ux%u = Seqnum[%u], PTS [%u-%u].\n",
			buffer, (u32)yuv_desc->y_addr_offset, (u32)yuv_desc->y_addr_offset,
			yuv_desc->pitch, yuv_desc->width, yuv_desc->height, yuv_desc->seq_num,
			pts, (pts - pts_prev));
		pts_prev = pts;
	}

	return 0;
}

static int capture_yuv(int buff_id, int count)
{
	int i, buf, save[MAX_SOURCE_BUFFER_NUM];
	int write_flag[MAX_SOURCE_BUFFER_NUM];
	char yuv_file[256];
	int non_stop = 0;
	u8 * luma = NULL;
	u8 * chroma = NULL;
	char format[32];
	struct iav_yuvbufdesc yuv_desc, yuv_desc_cache;
	struct iav_querydesc query_desc;
	struct iav_srcbuf_format buf_format;
	struct iav_yuv_cap *yuv_cap;

	luma = malloc(MAX_YUV_BUFFER_SIZE);
	if (luma == NULL) {
		printf("Not enough memory for preview capture !\n");
		goto yuv_error_exit;
	}
	chroma = malloc(MAX_YUV_BUFFER_SIZE);
	if (chroma == NULL) {
		printf("Not enough memory for preivew capture !\n");
		goto yuv_error_exit;
	}
	memset(save, 0, sizeof(save));
	memset(write_flag, 0, sizeof(write_flag));
	memset(luma, 1, MAX_YUV_BUFFER_SIZE);
	memset(chroma, 1, MAX_YUV_BUFFER_SIZE);

	if (count == 0) {
		non_stop = 1;
	}

	for (i = 0; ((i < count || non_stop) && !quit_capture); ++i) {
		for (buf = IAV_SRCBUF_FIRST;
			buf < IAV_SRCBUF_LAST; ++buf) {
			if (buff_id & (1 << buf)) {
				if (fd_yuv[buf] < 0) {
					memset(&buf_format, 0, sizeof(buf_format));
					buf_format.buf_id = buf;
					if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
						perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
						continue;
					}
					memset(yuv_file, 0, sizeof(yuv_file));
					sprintf(yuv_file, "%s_prev_%c_%dx%d.yuv", filename,
						(buf == IAV_SRCBUF_MN) ? 'M' :
						('a' + IAV_SRCBUF_PA - buf),
						buf_format.size.width, buf_format.size.height);
					if (fd_yuv[buf] < 0) {
						fd_yuv[buf] = amba_transfer_open(yuv_file,
							transfer_method, port++);
					}
					if (fd_yuv[buf] < 0) {
						printf("Cannot open file [%s].\n", yuv_file);
						continue;
					}
				}

				memset(&query_desc, 0, sizeof(query_desc));
				if (!non_block_read) {
					query_desc.qid = IAV_DESC_YUV;
					query_desc.arg.yuv.buf_id = buf;
					query_desc.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
				} else {
					/* It's also supported to use 'IAV_DESC_YUV' here. */
					query_desc.qid = IAV_DESC_BUFCAP;
					query_desc.arg.bufcap.flag |= IAV_BUFCAP_NONBLOCK;
				}

				if (verbose) {
					gettimeofday(&curr, NULL);
					pre = curr;
				}
				if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
					if (errno == EINTR) {
						continue;		/* back to for() */
					} else {
						perror("IAV_IOC_QUERY_DESC");
						goto yuv_error_exit;
					}
				}
				if (verbose) {
					gettimeofday(&curr, NULL);
					printf("11. Query DESC [%06ld us].\n", 1000000 *
						(curr.tv_sec - pre.tv_sec)  + (curr.tv_usec - pre.tv_usec));
				}
				memset(&yuv_desc, 0, sizeof(yuv_desc));
				if (!non_block_read) {
					yuv_desc = query_desc.arg.yuv;
				} else {
					yuv_cap = &query_desc.arg.bufcap.yuv[buf];
					yuv_desc.y_addr_offset = yuv_cap->y_addr_offset;
					yuv_desc.uv_addr_offset = yuv_cap->uv_addr_offset;
					yuv_desc.width = yuv_cap->width;
					yuv_desc.height = yuv_cap->height;
					yuv_desc.pitch = yuv_cap->pitch;
					yuv_desc.seq_num = yuv_cap->seq_num;
					yuv_desc.format = yuv_cap->format;
					yuv_desc.mono_pts = yuv_cap->mono_pts;
				}

				if (((u8 *)yuv_desc.y_addr_offset == NULL) || ((u8 *)yuv_desc.uv_addr_offset == NULL)) {
					printf("YUV buffer [%d] address is NULL! Skip to next!\n", buf);
					continue;
				}
				if (delay_frame_cap_data) {
					if (write_flag[buf] == 0) {
						write_flag[buf] = 1;
						yuv_desc_cache = yuv_desc;
					} else {
						write_flag[buf] = 0;
						if (save_yuv_data(fd_yuv[buf], buf, &yuv_desc_cache, luma, chroma) < 0) {
							printf("Failed to save YUV data of buf [%d].\n", buf);
							goto yuv_error_exit;
						}
					}
				} else {
					if (save_yuv_data(fd_yuv[buf], buf, &yuv_desc, luma, chroma) < 0) {
						printf("Failed to save YUV data of buf [%d].\n", buf);
						goto yuv_error_exit;
					}
				}

				if (save[buf] == 0) {
					save[buf] = 1;
					if (yuv_desc.format == IAV_YUV_FORMAT_YUV422) {
						if (yuv_format == YUV422_YU16) {
							sprintf(format,"YU16");
						} else if (yuv_format == YUV422_YV16) {
							sprintf(format, "YV16");
						} else {
							printf("Error: Unsupported YUV 422 format!\n");
							return -1;
						}
					} else if (yuv_desc.format == IAV_YUV_FORMAT_YUV420) {
						switch (yuv_format) {
						case YUV420_YV12:
							sprintf(format, "YV12");
							break;
						case YUV420_NV12:
							sprintf(format, "NV12");
							break;
						case YUV420_IYUV:
							sprintf(format, "IYUV");
							break;
						default:
							sprintf(format, "IYUV");
							break;
						}
					} else {
						sprintf(format, "Unknown [%d]", yuv_desc.format);
					}
					printf("Delay %d frame capture yuv data.\n", delay_frame_cap_data);
					printf("Capture_yuv_buffer: resolution %dx%d in %s format\n",
						yuv_desc.width, yuv_desc.height, format);
				}
			}
		}
	}

	free(luma);
	free(chroma);
	return 0;

yuv_error_exit:
	if (luma)
		free(luma);
	if (chroma)
		free(chroma);
	return -1;
}

static int save_me_luma_buffer(u8* output, struct iav_mebufdesc *me_desc)
{
	int i;
	u8 *in = NULL;
	u8 *out = NULL;
	u8 *base = NULL;

	if (me_desc->pitch < me_desc->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	base = get_buffer_base(me_desc->buf_id, 1);

	if (base == NULL) {
		printf("Invalid buffer address for buffer %d ME!"
			" Map ME buffer from DSP first.\n", me_desc->buf_id);
		return -1;
	}

	if (me_desc->pitch == me_desc->width) {
		memcpy(output, base + me_desc->data_addr_offset,
			me_desc->width * me_desc->height);
	} else {
		in = base + me_desc->data_addr_offset;
		out = output;
		for (i = 0; i < me_desc->height; i++) {		//row
			memcpy(out, in, me_desc->width);
			in += me_desc->pitch;
			out += me_desc->width;
		}
	}

	return 0;
}


static int save_me_data(int fd, int buffer, struct iav_mebufdesc *me_desc,
	u8 * y_buf, u8 * uv_buf)
{
	static u32 pts_prev = 0, pts = 0;

	me_desc->height =  me_desc->height * G_multi_vin_num;

	save_me_luma_buffer(y_buf, me_desc);

	if (amba_transfer_write(fd, y_buf,
		me_desc->width * me_desc->height, transfer_method) < 0) {
		perror("Failed to save ME data into file !\n");
		return -1;
	}

	if (amba_transfer_write(fd, uv_buf,
		me_desc->width * me_desc->height / 2, transfer_method) < 0) {
		perror("Failed to save ME data into file !\n");
		return -1;
	}

	if (verbose) {
		pts = me_desc->mono_pts;
		printf("BUF [%d] 0x%08x, pitch %d, %dx%d, seqnum [%d], PTS [%d-%d].\n",
			buffer, (u32)me_desc->data_addr_offset, me_desc->pitch,
			me_desc->width, me_desc->height, me_desc->seq_num,
			pts, (pts - pts_prev));
		pts_prev = pts;
	}

	return 0;
}

static void get_me_size(struct iav_srcbuf_format *buf_format, int is_me1,
	u32 *width, u32 *height)
{
	struct iav_system_resource resource;

	if (is_me1) {
		*width = buf_format->size.width >> 2;
		*height = buf_format->size.height >> 2;
	} else {
		memset(&resource, 0, sizeof(resource));
		resource.encode_mode = DSP_CURRENT_MODE;
		if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
			perror("IAV_IOC_GET_SYSTEM_RESOURCE");
			*width = 0;
			*height = 0;
		} else {
			switch (resource.me0_scale) {
			case ME0_SCALE_8X:
				*width = ROUND_UP(buf_format->size.width, 16) >> 3;
				*height = ROUND_UP(buf_format->size.height, 16) >> 3;
				break;
			case ME0_SCALE_16X:
				*width = ROUND_UP(buf_format->size.width, 16) >> 4;
				*height = ROUND_UP(buf_format->size.height, 16) >> 4;
				break;
			case ME0_SCALE_OFF:
			/* fall through*/
			default:
				*width = 0;
				*height = 0;
				break;
			}
		}
	}

	return ;
}

static int capture_me(int buff_id, int count, int is_me1)
{
	int i, buf, save[MAX_SOURCE_BUFFER_NUM];
	int write_flag[MAX_SOURCE_BUFFER_NUM];
	char me_file[256];
	int non_stop = 0;
	u32 width, height;
	u8 * luma = NULL;
	u8 * chroma = NULL;
	struct iav_mebufdesc me_desc, me_desc_cache;
	struct iav_querydesc query_desc;
	struct iav_srcbuf_format buf_format;
	struct iav_me_cap *me_cap;
	struct iav_mebufdesc *me_desc_ptr;
	struct iav_system_resource resource;

	luma = malloc(MAX_ME_BUFFER_SIZE);
	if (luma == NULL) {
		printf("Not enough memory for ME buffer capture !\n");
		goto me_error_exit;
	}

	//clear UV to be B/W mode, UV data is not useful for motion detection,
	//just fill UV data to make YUV to be YV12 format, so that it can play in YUV player
	chroma = malloc(MAX_ME_BUFFER_SIZE);
	if (chroma == NULL) {
		printf("Not enough memory for ME buffer capture !\n");
		goto me_error_exit;
	}
	memset(chroma, 0x80, MAX_ME_BUFFER_SIZE);
	memset(save, 0, sizeof(save));
	memset(write_flag, 0, sizeof(write_flag));
	memset(luma, 1, MAX_ME_BUFFER_SIZE);

	if (count == 0) {
		non_stop = 1;
	}

	if (is_me1) {
		me_desc_ptr = &query_desc.arg.me1;
	} else {
		memset(&resource, 0, sizeof(resource));
		resource.encode_mode = DSP_CURRENT_MODE;
		ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
		if (resource.me0_scale == 0) {
			printf("Please turn me0 scale on first!\n");
			goto me_error_exit;
		}

		me_desc_ptr = &query_desc.arg.me0;
	}

	for (i = 0; ((i < count || non_stop) && !quit_capture); ++i) {
		for (buf = IAV_SRCBUF_FIRST;
			buf < IAV_SRCBUF_LAST; ++buf) {
			if (buff_id & (1 << buf)) {
				if (fd_me[buf] < 0) {
					memset(&buf_format, 0, sizeof(buf_format));
					buf_format.buf_id = buf;
					if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT,
							&buf_format) < 0) {
						perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
						continue;
					}
					memset(me_file, 0, sizeof(me_file));
					get_me_size(&buf_format, is_me1, &width, &height);
					sprintf(me_file, "%s_me%d_%c_%dx%d.yuv", filename, is_me1,
						(buf == IAV_SRCBUF_MN) ? 'm' :
						('a' + IAV_SRCBUF_PA - buf),
						width, (height * G_multi_vin_num));
					if (fd_me[buf] < 0) {
						fd_me[buf] = amba_transfer_open(me_file,
							transfer_method, port++);
					}
					if (fd_me[buf] < 0) {
						printf("Cannot open file [%s].\n", me_file);
						continue;
					}
				}
				memset(&query_desc, 0, sizeof(query_desc));
				if (!non_block_read) {
					if (is_me1) {
						query_desc.qid = IAV_DESC_ME1;
					} else {
						query_desc.qid = IAV_DESC_ME0;
					}
					me_desc_ptr->buf_id = buf;
					me_desc_ptr->flag &= ~IAV_BUFCAP_NONBLOCK;
				} else {
					/* It's also supported to use 'IAV_DESC_MEx' here. */
					query_desc.qid = IAV_DESC_BUFCAP;
					query_desc.arg.bufcap.flag |= IAV_BUFCAP_NONBLOCK;
				}
				if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
					if (errno == EINTR) {
						continue;		/* back to for() */
					} else {
						perror("IAV_IOC_QUERY_DESC");
						goto me_error_exit;
					}
				}
				memset(&me_desc, 0, sizeof(me_desc));
				if (!non_block_read) {
					me_desc = *me_desc_ptr;
				} else {
					if (is_me1) {
						me_cap = &query_desc.arg.bufcap.me1[buf];
					} else {
						me_cap = &query_desc.arg.bufcap.me0[buf];
					}
					me_desc.data_addr_offset = me_cap->data_addr_offset;
					me_desc.width = me_cap->width;
					me_desc.height = me_cap->height;
					me_desc.pitch = me_cap->pitch;
					me_desc.seq_num = me_cap->seq_num;
					me_desc.mono_pts = me_cap->mono_pts;
				}

				if ((u8 *)me_desc.data_addr_offset == NULL) {
					printf("ME buffer [%d] address is NULL! Skip to next!\n", buf);
					continue;
				}
				if (delay_frame_cap_data) {
					if (write_flag[buf] == 0) {
						write_flag[buf] = 1;
						me_desc_cache = me_desc;
					} else {
						write_flag[buf] = 0;
						if (save_me_data(fd_me[buf], buf, &me_desc_cache, luma, chroma) < 0) {
							printf("Failed to save ME data of buf [%d].\n", buf);
							goto me_error_exit;
						}
					}
				} else {
					if (save_me_data(fd_me[buf], buf, &me_desc, luma, chroma) < 0) {
						printf("Failed to save ME data of buf [%d].\n", buf);
						goto me_error_exit;
					}
				}

				if (save[buf] == 0) {
					save[buf] = 1;
					printf("Delay %d frame capture me data.\n", delay_frame_cap_data);
					printf("Save_me_buffer: resolution %dx%d with Luma only.\n",
						me_desc.width, me_desc.height);
				}
			}
		}
	}

	free(luma);
	free(chroma);
	return 0;

me_error_exit:
	if (luma)
		free(luma);
	if (chroma)
		free(chroma);
	return -1;
}

static int capture_raw(void)
{
	struct iav_rawbufdesc *raw_desc;
	struct iav_querydesc query_desc;
	u8 *raw_buffer = NULL;
	u8 *base = NULL;
	struct iav_system_resource resource;
	u32 buffer_size;
	char raw_file[256];

	if (dsp_partition_map) {
		base = dsp_sub_mem[IAV_DSP_SUB_BUF_RAW];
	} else {
		base = dsp_mem;
	}

	if (base == NULL) {
		printf("Invalid buffer address for RAW! Map RAW buffer from DSP first.\n");
		goto raw_error_exit;
	}

	memset(raw_file, 0, sizeof(raw_file));
	sprintf(raw_file, "%s_raw.raw", filename);
	if (fd_raw < 0) {
		fd_raw = amba_transfer_open(raw_file,
			transfer_method, port++);
	}
	if (fd_raw < 0) {
		printf("Cannot open file [%s].\n", raw_file);
		goto raw_error_exit;
	}
	memset(&query_desc, 0, sizeof(query_desc));
	query_desc.qid = IAV_DESC_RAW;
	raw_desc = &query_desc.arg.raw;
	if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		if (errno == EINTR) {
			// skip to do nothing
		} else {
			perror("IAV_IOC_QUERY_DESC");
			goto raw_error_exit;
		}
	}

	buffer_size = raw_desc->pitch * raw_desc->height;
	raw_buffer = (u8 *)malloc(buffer_size);
	if (raw_buffer == NULL) {
		printf("Not enough memory for read out raw buffer!\n");
		goto raw_error_exit;
	}
	memset(raw_buffer, 0, buffer_size);

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		goto raw_error_exit;
	}
	if (resource.raw_capture_enable &&
		(!raw_desc->pitch || !raw_desc->height || !raw_desc->width)) {
		printf("Raw data resolution %ux%u with pitch %u is incorrect!\n",
			raw_desc->width, raw_desc->height, raw_desc->pitch);
		goto raw_error_exit;
	}
	memcpy(raw_buffer, (u8 *)dsp_mem + raw_desc->raw_addr_offset, buffer_size);
	if (amba_transfer_write(fd_raw, raw_buffer, buffer_size,
		transfer_method) < 0) {
		perror("Failed to save RAW data into file !\n");
		goto raw_error_exit;
	}

	amba_transfer_close(fd_raw, transfer_method);
	free(raw_buffer);
	printf("save raw buffer done!\n");
	printf("Raw data resolution %u x %u with pitch %u.\n",
		raw_desc->width, raw_desc->height, raw_desc->pitch);

	return 0;

raw_error_exit:
	if (raw_buffer)
		free(raw_buffer);
	if (fd_raw >= 0) {
		amba_transfer_close(fd_raw, transfer_method);
		fd_raw = -1;
	}
	return -1;
}

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "b:B:f:F:mMr:RtudvY";

static struct option long_options[] = {
	{"buffer",		HAS_ARG, 0, 'b'},
	{"non-block",	HAS_ARG, 0, 'B'},
	{"yuv",		NO_ARG, 0, 'Y'},
	{"me1",		NO_ARG, 0, 'm'},		/*capture me1 buffer */
	{"me0",		NO_ARG, 0, 'M'},		/*capture me0 buffer */
	{"raw",		NO_ARG, 0, 'R'},
	{"filename",	HAS_ARG, 0, 'f'},		/* specify file name*/
	{"format",	HAS_ARG, 0, 'F'},
	{"tcp", 		NO_ARG, 0, 't'},
	{"usb",		NO_ARG, 0,'u'},
	{"frames",	HAS_ARG,0, 'r'},
	{"delay-frame-cap-data",	NO_ARG, 0, 'd'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0~3", "select source buffer id, 0 for main, 1 for 2nd buffer, 2 for 3rd buffer, 3 for 4th buffer"},
	{"0|1", "select the read method, 0 is block call, 1 is non-block call. Default is block call."},
	{"",	"\tcapture YUV data from source buffer"},
	{"",	"\tcapture me1 data from source buffer"},
	{"",	"\tcapture me0 data from source buffer"},
	{"",	"\tcapture raw data"},
	{"?.yuv",	"filename to store output yuv"},
	{"0|1|2|3|4",	"YUV420 data format for encode buffer, 0: IYUV(I420), 1: YV12, 2: NV12, 3:YU16, 4:YV16. Default is IYUV format"},
	{"",	"\tuse tcp to send data to PC"},
	{"",	"\tuse usb to send data to PC"},
	{"1~n",	"frame counts to capture"},
	{"",	"delay 1 frame to capture previous frame, discard current frame. Real captured frame numbers are half of the frame counts specified by -r xxx"},
	{"",	"\tprint more messages"},
};

static void usage(void)
{
	u32 i;
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
	printf("\n\nThis program captures YUV or ME1 buffer in YUV420 format for encode buffer, and save as IYUV, YV12 or NV12.\n");
	printf("  IYUV format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVV\n");
	printf("  YV12 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVV\n"
		 "\t\tUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUU\n");
	printf("  NV12 format (U and V are interleaved buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUVUVUVUVUVUVUV\n"
		 "\t\tUVUVUVUVUVUVUV\n"
		 "\t\tUVUVUVUVUVUVUV\n"
		 "\t\tUVUVUVUVUVUVUV\n");

	printf("\n\nThis program captures YUV buffer in YUV422 format for preview buffer, and save as YU16 or YV16.\n");
	printf("  YU16 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n");
	printf("  YV16 format (U and V are planar buffers) is like :\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tYYYYYYYYYYYYYYYYYYYYYYYYYYYY\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tVVVVVVVVVVVVVVVVVVVVVVVVVVVV\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n"
		 "\t\tUUUUUUUUUUUUUUUUUUUUUUUUUUUU\n");


	printf("\neg: To get one single preview frame of IYUV format\n");
	printf("    > test_yuvcap -b 1 -Y -f 1.yuv -F 0 -r 1 --tcp\n\n");
	printf("    To get 30 frames of yuv data as .yuv file of YV12 format\n");
	printf("    > test_yuvcap -b 1 -Y -f 1.yuv -F 1 -r 30 --tcp\n\n");
	printf("    To get continuous frames of yuv data as .yuv file of YV12 format and with delay 1 frame capture yuv\n");
	printf("    > test_yuvcap -b 1 -Y -f 1.yuv -F 1 -r 0 --tcp -d \n\n");
	printf("    To get 30 frames of me1 data as .yuv file\n");
	printf("    > test_yuvcap -b 0 -m -b 1 -m -f 2.me1 -r 30 --tcp\n\n");
	printf("    To get continuous frames of me1 data as .yuv file\n");
	printf("    > test_yuvcap -b 0 -m -b 1 -m -f 2.me1 -r 0 --tcp\n\n");
	printf("    To get raw data from RGB sensor input, please enter \n");
	printf("    > test_yuvcap -R -f cap_raw --tcp\n");
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'b':
			value = atoi(optarg);
			if ((value < IAV_SRCBUF_FIRST) ||
				(value >= IAV_SRCBUF_LAST)) {
				printf("Invalid preview buffer id : %d.\n", value);
				return -1;
			}
			current_buffer = value;
			break;
		case 'B':
			value = atoi(optarg);
			if (value < 0 || value > 1) {
				printf("Read flag [%d] must be [0|1].\n", value);
				return -1;
			}
			non_block_read = value;
			break;
		case 'Y':
			VERIFY_BUFFERID(current_buffer);
			capture_select = CAPTURE_PREVIEW_BUFFER;
			yuv_buffer_id |= (1 << current_buffer);
			break;
		case 'm':
			VERIFY_BUFFERID(current_buffer);
			capture_select = CAPTURE_ME1_BUFFER;
			me1_buffer_id |= (1 << current_buffer);
			break;
		case 'M':
			VERIFY_BUFFERID(current_buffer);
			capture_select = CAPTURE_ME0_BUFFER;
			me0_buffer_id |= (1 << current_buffer);
			break;
		case 'R':
			capture_select = CAPTURE_RAW_BUFFER;
			break;
		case 'f':
			strcpy(filename, optarg);
			break;
		case 'F':
			value = atoi(optarg);
			if (value < YUV_FORMAT_FIRST || value >= YUV_FORMAT_LAST) {
				printf("Invalid output yuv format : %d.\n", value);
				return -1;
			}
			yuv_format = value;
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'r':
			value = atoi(optarg);
			if (value < 0 || value > MAX_FRAME_NUM) {
				printf("Cannot capture YUV or ME1 data over %d frames [%d].\n",
					MAX_FRAME_NUM, value);
				return -1;
			}
			frame_count = value;
			break;
		case 'd':
			delay_frame_cap_data = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	if (transfer_method == TRANS_METHOD_TCP ||
		transfer_method == TRANS_METHOD_USB)
		default_filename = default_filename_tcp;
	else
		default_filename = default_filename_nfs;

	return 0;
}

static void sigstop(int a)
{
	quit_capture = 1;
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

static int get_multi_vin_num(void)
{
	struct iav_system_resource resource_setup;
	memset(&resource_setup, 0, sizeof(resource_setup));
	resource_setup.encode_mode = DSP_CURRENT_MODE;

	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource_setup) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}
	G_multi_vin_num = 1;
	return 0;
}

int main(int argc, char **argv)
{
	int retv = 0, i;

	//register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	//check iav state
	if (check_state() < 0)
		return -1;

	if (map_buffer() < 0)
		return -1;

	if (amba_transfer_init(transfer_method) < 0) {
		return -1;
	}

	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	for (i = IAV_SRCBUF_FIRST;
		i < IAV_SRCBUF_LAST; ++i) {
		fd_yuv[i] = -1;
		fd_me[i] = -1;
	}

	if (get_multi_vin_num() < 0) {
		perror("get_multi_vin_num");
		return -1;
	}
	switch (capture_select) {
	case CAPTURE_PREVIEW_BUFFER:
		capture_yuv(yuv_buffer_id, frame_count);
		break;
	case CAPTURE_ME1_BUFFER:
		capture_me(me1_buffer_id, frame_count, 1);
		break;
	case CAPTURE_ME0_BUFFER:
		capture_me(me0_buffer_id, frame_count, 0);
		break;
	case CAPTURE_RAW_BUFFER:
		capture_raw();
		break;
	default:
		printf("Invalid capture mode [%d] !\n", capture_select);
		return -1;
		break;
	}

	for (i = IAV_SRCBUF_FIRST;
		i < IAV_SRCBUF_LAST; ++i) {
		if (fd_yuv[i] >= 0) {
			amba_transfer_close(fd_yuv[i], transfer_method);
			fd_yuv[i] = -1;
		}
		if (fd_me[i] >= 0) {
			amba_transfer_close(fd_me[i], transfer_method);
			fd_me[i] = -1;
		}
	}

	if (amba_transfer_deinit(transfer_method) < 0) {
		retv = -1;
	}

	return retv;
}

