/*******************************************************************************
 * iav.c
 *
 * History:
 *   Jun 24, 2015 - [binwang] created file
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include "iav.h"

static iav_buf_id_t g_buf_id;
static iav_buf_type_t g_buf_type;
static int fd;
static u8 *dsp_mem;
static u32 dsp_size;
static u32 mem_mapped = 0;
struct iav_querydesc query_desc;

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = (u8 *)mmap(NULL, dsp_size, PROT_READ, MAP_SHARED, fd, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	} else {
		mem_mapped = 1;
	}

	memset(&query_desc, 0, sizeof(query_desc));
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

int init_iav(iav_buf_id_t buf_id, iav_buf_type_t buf_type)
{
	fd = open("/dev/iav", O_RDWR);
	if (fd <= 0) {
		perror("open /dev/iav failed\n");
		return -1;
	}

	if (buf_id >= IAV_BUF_ID_MAX || buf_type >= IAV_BUF_TYPE_MAX) {
		printf("Buffer ID/Type is wrong\n");
		return -1;
	} else {
		g_buf_id = buf_id;
		g_buf_type = buf_type;
	}

	return map_buffer();
}

int get_iav_buf_size(unsigned int *p, unsigned int *w, unsigned int *h)
{
	switch (g_buf_type) {
		case IAV_BUF_YUV:
			query_desc.qid = IAV_DESC_YUV;
			query_desc.arg.yuv.buf_id = (enum iav_srcbuf_id)g_buf_id;
			query_desc.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
			break;
		case IAV_BUF_ME0:
			query_desc.qid = IAV_DESC_ME0;
			query_desc.arg.me0.buf_id = (enum iav_srcbuf_id)g_buf_id;
			query_desc.arg.me0.flag &= ~IAV_BUFCAP_NONBLOCK;
			break;
		case IAV_BUF_ME1:
			query_desc.qid = IAV_DESC_ME1;
			query_desc.arg.me1.buf_id = (enum iav_srcbuf_id)g_buf_id;
			query_desc.arg.me1.flag &= ~IAV_BUFCAP_NONBLOCK;
			break;
		default:
			printf("Buffer type %d is unknown or not supported\n", g_buf_type);
			return -1;
	}

	if (ioctl(fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return -1;
	}
	switch (g_buf_type) {
		case IAV_BUF_YUV:
			*p = query_desc.arg.yuv.pitch;
			*h = query_desc.arg.yuv.height;
			*w = query_desc.arg.yuv.width;
			break;
		case IAV_BUF_ME0:
			*p = query_desc.arg.me0.pitch;
			*h = query_desc.arg.me0.height;
			*w = query_desc.arg.me0.width;
			break;
		case IAV_BUF_ME1:
			*p = query_desc.arg.me1.pitch;
			*h = query_desc.arg.me1.height;
			*w = query_desc.arg.me1.width;
			break;
		default:
			break;
	}

	return 0;
}

char *get_iav_buf(void)
{
	char *data_addr = NULL;
	if (ioctl(fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
		perror("IAV_IOC_QUERY_DESC");
		return NULL;
	}
	switch (g_buf_type) {
		case IAV_BUF_YUV:
			data_addr = (char *)(dsp_mem + query_desc.arg.yuv.y_addr_offset);
			break;
		case IAV_BUF_ME0:
			data_addr = (char *)(dsp_mem + query_desc.arg.me0.data_addr_offset);
			break;
		case IAV_BUF_ME1:
			data_addr = (char *)(dsp_mem + query_desc.arg.me1.data_addr_offset);
			break;
		default:
			break;
	}

	return data_addr;
}

int exit_iav(void)
{
	if (fd >= 0) {
		close(fd);
	}

	return unmap_buffer();
}
