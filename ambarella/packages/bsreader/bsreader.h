/*
 * bsreader.h
 *
 * History:
 *	2010/05/13 - [Louis Sun] created file
 *	2013/03/04 - [Jian Tang] modified file
 * Copyright (C) 2010-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __BSREADER_LIB_H__
#define __BSREADER_LIB_H__


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
	u8	reserved[7];
} bs_info_t;


/* any frame stored in bsreader must has continuous memory */
typedef struct bsreader_frame_info_s {
	bs_info_t bs_info;	/* orignal information got from dsp */
	u8	*frame_addr; /* the actual address in stream buffer */
	u32	frame_size;	 /* the actual output frame size , including CAVLC */
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


