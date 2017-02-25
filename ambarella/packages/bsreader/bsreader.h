/*******************************************************************************
 * bsreader.h
 *
 * History:
 *	2010/05/13 - [Louis Sun] created file
 *	2013/03/04 - [Jian Tang] modified file
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
#ifndef __BSREADER_LIB_H__
#define __BSREADER_LIB_H__

#include "config.h"

#if defined(CONFIG_ARCH_S3L) || defined(CONFIG_ARCH_S5L)
#include <iav_ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_BS_READER_SOURCE_STREAM		(IAV_STREAM_MAX_NUM_IMPL)

#define MAX_BS_READER_FRAMES_OF_BITS_INFO		(IAV_STREAM_MAX_NUM_IMPL)

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

/* current implementation use IAV's bits_info_ex_t to be frame info*/

typedef enum {
	/* invalid frames, info is useless */
	BS_READER_FRAME_STATUS_INVALID = 0,
	/* fresh meat */
	BS_READER_FRAME_STATUS_FRESH = 1,
	/* the one being used */
	BS_READER_FRAME_STATUS_DIRTY = 2,
	BS_READER_FRAME_STATUS_TOTAL_NUM,
	BS_READER_FRAME_STATUS_FIRST = 0,
	BS_READER_FRAME_STATUS_LAST = BS_READER_FRAME_STATUS_TOTAL_NUM,
} BS_READER_FRAME_STATUS;

typedef struct {
	u32	stream_id;
	u32	session_id;	//use 32-bit session id
	u32	frame_num;
	u32	pic_type;
	u32	PTS;
	u32	audio_pts;
	u64	mono_pts;
	u32	stream_end;  // 0: normal stream frames,  1: stream end null frame
	u32	frame_index;
	u8	jpeg_quality;	// quality value for jpeg stream
	u8	tile_id:	2;
	u8	tile_num:	2;
	u8	is_pbg_frame:	1;
	u8	reserved_1:	3;
	u8	reserved[6];
} bs_info_t;


/* any frame stored in bsreader must has continuous memory */
typedef struct bsreader_frame_info_s {
	bs_info_t bs_info;	/* orignal information got from dsp */
	u8	*frame_addr; /* the actual address in stream buffer */
	u32	frame_size;	 /* the actual output frame size , including CAVLC */
#if defined(CONFIG_ARCH_S3L) || defined(CONFIG_ARCH_S5L)
	u8	*data_addr[IAV_HEVC_TILE_NUM];
	u32	slice_size[IAV_HEVC_TILE_NUM];
#endif
} bsreader_frame_info_t;

typedef struct bsreader_init_data_s {
	int	fd_iav;

	/* should be no more than MAX_BS_READER_SOURCE_STREAM */
	u32	max_stream_num;
	u32	ring_buf_size[MAX_BS_READER_SOURCE_STREAM];
	u32	max_buffered_frame_num;

	/* 0:default drop old when ring buffer full,
	 * 1: drop new when ring buffer full
	 */
	u8	policy;

	/* the dispatcher thread RT priority */
	u8	dispatcher_thread_priority;

	/* the reader threads' thread RT priority */
	u8	reader_threads_priority;

	/* if it is 1, then it's possible to use CAVLC encoding,
	 * set to 0 if don't need CAVLC
	 */
	u8	cavlc_possible;
#if defined(CONFIG_ARCH_S3L) || defined(CONFIG_ARCH_S5L)
	u8	slice_flag;
	u8	reserved[3];
#endif

}bsreader_init_data_t;

/* set init parameters, do nothing real*/
int bsreader_init(bsreader_init_data_t * bsinit);

/* create mutex and cond, allocate memory and do data initialization,
 * threads initialization, also call reset write/read pointers
 */
int bsreader_open(void);

/* free all resources, close all threads, destroy all mutex and cond */
int bsreader_close(void);

/* get one frame out of the stream after the frame is read out, the
 * frame still stays in ring buffer, until next bsreader_get_one_frame
 * will invalidate it.
 */
int bsreader_get_one_frame(int stream, bsreader_frame_info_t * info);
/* flush (discard) and reset all data in ring buf (can flush selected ring
 * buf), do not allocate or free ring buffer memory.
 */
int bsreader_reset(int stream);
int bsreader_unblock_reader(int stream);

int bsreader_get_version(version_t* version);

#ifdef __cplusplus
}
#endif


#endif // __BSREADER_LIB_H__


