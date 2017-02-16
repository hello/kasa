/*
 * dsplog_cap.h
 *
 * History:
 *	2014/09/05 - [Jian Tang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __DSPLOG_CAP_H__
#define __DSPLOG_CAP_H__

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <basetypes.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sched.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "amba_debug.h"

/* include platform related config file */
#include <config.h>
#if defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3)
#include "iav_ioctl.h"
#include "iav_ucode_ioctl.h"
#else
#include "iav_drv.h"
#endif
#include "dsplog_drv.h"
#include "datatx_lib.h"  //use datax lib to transfer data

#define	DSP_MODULE_TOTAL_NUM	(8)
#define	STR_LEN				(512)
#define	DSP_CODE_NAME			(16)
#define	NO_ARG					(0)
#define	HAS_ARG				(1)

#define	DSP_LOG_PORT			(2017)        /* use port 2017 for tcp transfer */
#define	AMBA_DEBUG_DSP		(1 << 1)
#define	MAX_LOG_READ_SIZE	(1 << 20)
#define	DEFAULT_PINPONG_FILE_SIZE    (100 << 20)

//#define	ENABLE_RT_SCHED

#define	DSPLOG_VERSION_STRING  "1.5"

typedef enum {
	DSPLOG_CAPTURE_MODE = 0,
	DSPLOG_PARSE_MODE = 1,
} DSPLOG_WORK_MODE;

typedef enum {
	DSPLOG_OFF = 0,
	DSPLOG_FORCE = 1,
	DSPLOG_NORMAL = 2,
	DSPLOG_VERBOSE = 3,
	DSPLOG_DEBUG = 4,
	DSPLOG_LEVEL_NUM,
	DSPLOG_LEVEL_FIRST = 0,
	DSPLOG_LEVEL_LAST = DSPLOG_LEVEL_NUM,
} DSPLOG_LEVEL;

struct hint_s {
	const char *arg;
	const char *str;
};

/* struct idsp_printf_s is compatible for all ARCH */
typedef struct idsp_printf_s {
	u32	seq_num;		/**< Sequence number */
	u8	dsp_core;
	u8	thread_id;
	u16	reserved;
	u32	format_addr;	/**< Address (offset) to find '%s' arg */
	u32	arg1;		/**< 1st var. arg */
	u32	arg2;		/**< 2nd var. arg */
	u32	arg3;		/**< 3rd var. arg */
	u32	arg4;		/**< 4th var. arg */
	u32	arg5;		/**< 5th var. arg */
} idsp_printf_t;

typedef struct dsplog_cap_s {
	int		logdrv_fd;
	char *	log_buffer;
	int		datax_method;
	char *	output_filename;
	int		log_port;
} dsplog_cap_t;

typedef struct {
	u32	mem_phys_addr;
	u32	mem_size;

	char	core_file[DSP_CODE_NAME];
	char	mdxf_file[DSP_CODE_NAME];
	char	memd_file[DSP_CODE_NAME];
	u32	core_offset;
	u32	mdxf_offset;
	u32	memd_offset;
} dsplog_memory_block;

typedef struct {
	char	output[STR_LEN];
	char	parse_in[STR_LEN];
	char	parse_out[STR_LEN];
	char	verify[STR_LEN];
	char	ucode[STR_LEN];
	u8	input_file_flag;
	u8	output_file_flag;
	u8	verify_flag;
	u8	capture_current_flag;
	u32	work_mode;

	/* debug_level is supported on A5s / A7L / S2L */
	u8	debug_level_flag;
	u8	debug_level;
	u8	module_id;
	u8	module_id_last;
	int	modules[DSP_MODULE_TOTAL_NUM];

	u32	thread_bitmask_flag;
	u32	thread_bitmask;

	/* debug_bitmask is only supported on iOne / S2 */
	u32	debug_bitmask[DSP_MODULE_TOTAL_NUM];
	u32	operation_add[DSP_MODULE_TOTAL_NUM];

	u8	show_version_flag;
	u8	show_ucode_flag;
	u8	pinpong_flag;
	u8	transfer_tcp_flag;

	/* file transfer method */
	u32	transfer_method;
	int	transfer_fd;

	u32	pinpong_state;
	u32	pinpong_filesize_limit;
	u32	pinpong_filesize_now;
} dsplog_debug_obj;

int preload_dsplog_mem(dsplog_memory_block *block);
int get_dsp_module_id(char *module_name);
int get_dsp_bitmask(char *bitmask_str);
int get_thread_bitmask(char *bitmask_str);
int dsplog_setup(int fd_iav, dsplog_debug_obj *obj);
void extra_usage(char *itself);

#endif	// __DSPLOG_CAP_H__

