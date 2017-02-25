/*
 * amba_fdet.h
 *
 * History:
 *	2013/03/12 - [Cao Rongrong] Created file
 *	2013/12/12 - [Jian Tang] Modified file
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

#ifndef __AMBA_FDET_H__
#define __AMBA_FDET_H__

enum fdet_ioctl_id
{
	FDET_IOCTL_START		= 0,
	FDET_IOCTL_STOP,
	FDET_IOCTL_GET_RESULT		= 3,
	FDET_IOCTL_SET_MMAP_TYPE,
	FDET_IOCTL_SET_CONFIGURATION,
	FDET_IOCTL_TRACK_FACE,
};

enum fdet_mmap_type {
	FDET_MMAP_TYPE_ORIG_BUFFER	= 0,
	FDET_MMAP_TYPE_TMP_BUFFER,
	FDET_MMAP_TYPE_CLASSIFIER_BINARY,
};

enum fdet_mode {
	FDET_MODE_VIDEO			= 0,
	FDET_MODE_STILL,
};

enum fdet_filtering_policy {
	FDET_POLICY_WAIT_FS_COMPLETE		= (1 << 0),
	FDET_POLICY_DISABLE_TS			= (1 << 1),
	FDET_POLICY_DEBUG			= (1 << 2),
	FDET_POLICY_MEASURE_TIME		= (1 << 3),
};

enum fdet_result_type {
	FDET_RESULT_TYPE_FS		= 0,
	FDET_RESULT_TYPE_TS,
};

struct fdet_configuration {
	int				input_width;
	int				input_height;
	int				input_pitch;
	int				input_source;
	unsigned int			input_offset;
	enum fdet_mode			input_mode;

	enum fdet_filtering_policy	policy;
};

struct fdet_face {
	unsigned short			x;
	unsigned short			y;
	unsigned short			size;
	enum fdet_result_type		type;
};

#endif
