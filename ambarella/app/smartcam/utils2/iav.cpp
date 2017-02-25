/*
 *
 * History:
 *    2016/01/28 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
#include "iav_ioctl.h"

static int		v_iav = -1;
static unsigned char	*v_dspmem = NULL;
static int		v_dspsize = 0;

typedef struct {
	int		yuv_w;
	int		yuv_h;
	int		yuv_p;
	unsigned char	*yuv_y;
	unsigned char	*yuv_uv;
} second_buf_t;

int init_iav(void)
{
	int			ret;
	struct iav_querybuf	qb;

	v_iav = open("/dev/iav", O_RDWR);
	if (v_iav <= 0) {
		return -1;
	}

	qb.buf	= IAV_BUFFER_DSP;
	ret	= ioctl(v_iav, IAV_IOC_QUERY_BUF, &qb);
	if (ret < 0) {
		return -1;
	}

	v_dspsize = qb.length;
	v_dspmem = (unsigned char *)mmap(NULL, v_dspsize, PROT_READ, MAP_SHARED, v_iav, qb.offset);
	if (v_dspmem == MAP_FAILED) {
		return -1;
	}

	return 0;
}

int get_main_buf(second_buf_t *buf)
{
	int				ret;
	struct iav_querydesc		qd;

	qd.qid = IAV_DESC_YUV;
	qd.arg.yuv.buf_id = (enum iav_srcbuf_id)0;
//	qd.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
	ret = ioctl(v_iav, IAV_IOC_QUERY_DESC, &qd);
	if (ret < 0) {
		return -1;
	}
	buf->yuv_w = qd.arg.yuv.width;
	buf->yuv_h = qd.arg.yuv.height;
	buf->yuv_p = qd.arg.yuv.pitch;
	buf->yuv_y = v_dspmem + qd.arg.yuv.y_addr_offset;
	buf->yuv_uv = v_dspmem + qd.arg.yuv.uv_addr_offset;

	return 0;
}

int get_second_buf(second_buf_t *buf)
{
	int				ret;
	struct iav_querydesc		qd;

	qd.qid = IAV_DESC_YUV;
	qd.arg.yuv.buf_id = (enum iav_srcbuf_id)1;
//	qd.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;
	ret = ioctl(v_iav, IAV_IOC_QUERY_DESC, &qd);
	if (ret < 0) {
		return -1;
	}
	buf->yuv_w = qd.arg.yuv.width;
	buf->yuv_h = qd.arg.yuv.height;
	buf->yuv_p = qd.arg.yuv.pitch;
	buf->yuv_y = v_dspmem + qd.arg.yuv.y_addr_offset;
	buf->yuv_uv = v_dspmem + qd.arg.yuv.uv_addr_offset;

	return 0;
}

int exit_iav(void)
{
	int		ret;

	if (v_dspmem && v_dspsize > 0) {
		ret = munmap(v_dspmem, v_dspsize);
		if (ret < 0) {
			return -1;
		}
		v_dspmem = NULL;
		v_dspsize = 0;
	}

	close(v_iav);

	return 0;
}
