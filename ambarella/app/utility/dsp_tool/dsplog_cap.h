/*
 * dsplog_cap.h
 *
 * History:
 *	2014/09/05 - [Jian Tang] created file
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
#include <stdint.h>

/* include platform related config file */
#include <config.h>
#if defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3) || defined(CONFIG_ARCH_S3L) || defined(CONFIG_ARCH_S5) || defined(CONFIG_ARCH_S5L)
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
#define	DEFAULT_PINPONG_FILE_SIZE    (10 << 20)

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

typedef struct idsp_printf_base_s {
	u32 seq_num; 		/**< Sequence number */
	u32 reserved[7];
} idsp_printf_base_t;

#define IDSP_LOG_SIZE	(sizeof(idsp_printf_base_t))

typedef struct dsplog_cap_s {
	char *	log_buffer;
	char *	output_filename;
	int	logdrv_fd;
	int	datax_method;
	int	log_port;
	int	reserved;
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
	u32	reserved;
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

	int	modules[DSP_MODULE_TOTAL_NUM];

	u32	thread_bitmask_flag;
	u32	thread_bitmask;

	/* debug_bitmask is only supported on iOne / S2 */
	u32	debug_bitmask[DSP_MODULE_TOTAL_NUM];
	u32	operation_add[DSP_MODULE_TOTAL_NUM];
	/* debug_level is supported on A5s / A7L / S2L */
	u8	debug_level_flag;
	u8	debug_level;
	u8	module_id;
	u8	module_id_last;

	u8	show_version_flag;
	u8	show_ucode_flag;
	u8	pinpong_flag;
	u8	transfer_tcp_flag;

	u8	capture_only_flag;
	u8	reserved0;
	u8	reserved1;
	u8	reserved2;

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
int print_log(idsp_printf_base_t *record, u8 * pcode,
	u8 * pmdxf, u8 * pmemd,  FILE *  write_file,
	dsplog_memory_block *dsplog_mem);

#endif	// __DSPLOG_CAP_H__

