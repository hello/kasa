/*
 * dsplog_utils.c (for S2L)
 *
 * History:
 *	2014/09/10 - [Jian Tang] created file
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


#include "../dsplog_cap.h"

#define DSPLOG_MEM_PHY_ADDR  0x80000
#define DSPLOG_MEM_SIZE            0x20000

//three ucode binaries for this ARCH

#define UCODE_CORE_FILE "orccode.bin"
#define UCODE_MEMD_FILE "orcme.bin"
#define UCODE_MDXF_FILE  "default_binary.bin"

#define UCODE_CORE_OFFSET   0x900000
#define UCODE_MDXF_OFFSET   0x600000
#define UCODE_MEMD_OFFSET   0x300000

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

static int do_dsp_setup_log(int iav_fd, u8 module, u8 level, u8 thread)
{
	if (ioctl(iav_fd, IAV_IOC_SET_DSP_LOG,
			(module | (level << 8) | (thread << 16))) < 0) {
		perror("IAV_IOC_SET_DSP_LOG");
		return -1;
	}
	return 0;
}

int preload_dsplog_mem(dsplog_memory_block *block)
{
	block->mem_phys_addr = DSPLOG_MEM_PHY_ADDR;
	block->mem_size = DSPLOG_MEM_SIZE;

	snprintf(block->core_file, sizeof(block->core_file), UCODE_CORE_FILE);
	block->core_offset = UCODE_CORE_OFFSET;
	snprintf(block->mdxf_file, sizeof(block->mdxf_file), UCODE_MEMD_FILE);
	block->mdxf_offset = UCODE_MDXF_OFFSET;
	snprintf(block->memd_file, sizeof(block->memd_file), UCODE_MEMD_FILE);
	block->memd_offset = UCODE_MEMD_OFFSET;

	return 0;
}

int get_dsp_module_id(char * input_name)
{
	int module_id;

	if (!strcasecmp(input_name, "all")) {
		module_id = 0;
	} else if  (!strcasecmp(input_name, "encoder")) {
		module_id = 1;
	} else if  (!strcasecmp(input_name, "decoder")) {
		module_id = 2;
	} else if  (!strcasecmp(input_name, "idsp")) {
		module_id = 3;
	} else if  (!strcasecmp(input_name, "vout")) {
		module_id = 4;
	} else if  (!strcasecmp(input_name, "api")) {
		module_id = 5;
	} else if  (!strcasecmp(input_name, "memd")) {
		module_id = 6;
	} else {
		printf("DSP module %s not supported to debug for this ARCH \n", input_name);
		return -1;
	}
	return module_id;
}

int get_dsp_bitmask(char * bitmask_str)
{
	printf("dsp debug bit mask not supported by this ARCH \n");
	return -1;
}

int get_thread_bitmask(char * bitmask_str)
{
	int bitmask;

	if (strlen(bitmask_str) < 3) {
		//should only be a decimal value
		bitmask = atoi(bitmask_str);
	} else {
		//check whether this is decimal or heximal
		if ((bitmask_str[0]=='0') && ((bitmask_str[1]=='x') ||(bitmask_str[1]=='X'))) {
			//it's a hex
			bitmask_str+=2;
			if (EOF == sscanf(bitmask_str, "%x", &bitmask)) {
				printf("get_thread_bitmask: error to get hex value from string %s\n", bitmask_str);
				return -1;
			}
		} else {
			bitmask = atoi(bitmask_str);
		}
	}

	/* S2L only uses 4-bit of thread bitmask */
	if ((bitmask < 0 ) || (bitmask > 0xF)) {
		printf("unsupported bit mask value in decimal %d for this ARCH\n", bitmask);
		bitmask = -1;
	}

	return bitmask;
}

int dsplog_setup(int iav_fd, dsplog_debug_obj *debug)
{
	int i;
	for (i = 0 ; i < debug->module_id; i++) {
		if (do_dsp_setup_log(iav_fd, debug->modules[i], debug->debug_level,
			debug->thread_bitmask) < 0) {
			printf("dsplog setup failed \n");
			return -1;
		}
	}
	return 0;
}

void extra_usage(char *itself)
{
	printf("  Enable all debug information :\n");
	printf("  #%s -m all -l 3 -r 0 -o /tmp/dsplog1.bin \n", itself);

	printf("  Use Pinpong files to store DSP log for vcap module with limited size\n");
	printf("  #%s -m all -l 3 -r 0 -o /sdcard/dsplog1.bin -p 100000000 \n\n", itself);

	printf("  Parse captured DSP log into txt file.\n");
	printf("  #%s -i /tmp/dsplog1.bin -f /tmp/dsplog.txt \n\n", itself);
}

int print_log(idsp_printf_base_t *arg, u8 * pcode,
	u8 * pmdxf, u8 * pmemd,  FILE *  write_file,
	dsplog_memory_block *dsplog_mem)
{
	char *fmt;
	u8 *ptr;
	u32 offset;
	idsp_printf_t *record = (idsp_printf_t *)arg;

	if (record->format_addr == 0)
		return -1;
	switch (record->dsp_core) {
	case 0: ptr = pcode; offset = dsplog_mem->core_offset; break;
	case 1: ptr = pmdxf; offset = dsplog_mem->mdxf_offset; break;
	case 2: ptr = pmemd; offset = dsplog_mem->memd_offset; break;
	default:
		fprintf(stderr, "dsp_core = %d\n", record->dsp_core);
		return -1;
	}
	fmt = (char*)(ptr + (record->format_addr - offset));
	fprintf(write_file, "[core:%d:%d] ", record->thread_id, record->seq_num);
	fprintf(write_file, fmt, record->arg1, record->arg2,
		record->arg3, record->arg4, record->arg5);
	return 0;
}

