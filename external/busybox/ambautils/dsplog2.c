/*
 * dsplog2.c
 *
 * History:
 *	2008/5/5 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <libbb.h>

#include "../../../ambarella/build/include/basetypes.h"
#include "../../../ambarella/build/include/iav_drv.h"

#ifdef WIN32
#include <windows.h>
#endif

typedef struct idsp_printf_s {
	u32 seq_num;		/**< Sequence number */
	u8  dsp_core;
	u8  thread_id;
	u16 reserved;
	u32 format_addr;	/**< Address (offset) to find '%s' arg */
	u32 arg1;		/**< 1st var. arg */
	u32 arg2;		/**< 2nd var. arg */
	u32 arg3;		/**< 3rd var. arg */
	u32 arg4;		/**< 4th var. arg */
	u32 arg5;		/**< 5th var. arg */
} idsp_printf_t;

static u8 *read_firmware(const char *dir, char *name);
static char *get_real_format_addr(u32 format_addr, u32 offset, u8 *base_addr);
static void print_log(idsp_printf_t *record, u8 *pcode, u8 *pmemd);
int dsplog2_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;

static u8 *read_firmware(const char *dir, char *name)
{
	u8 *mem;
	FILE *fp;
	int fsize;
	char filename[256];

	sprintf(filename, "%s/%s", dir, name);
	if ((fp = fopen(filename, "r")) == NULL) {
		perror(filename);
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END) < 0) {
		perror("fseek");
		return NULL;
	}
	fsize = ftell(fp);

	mem = (u8 *)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
	if ((int)mem == -1) {
		perror("mmap");
		return NULL;
	}

	return mem;
}

static char *get_real_format_addr(u32 format_addr, u32 offset, u8 *base_addr)
{
	char *addr = (char *)base_addr;

	if (format_addr & 0xF0000000) {
		format_addr -= 0xF0000000;
	} else {
		format_addr -= offset;
	}

	addr += format_addr;

	return addr;
}

static void print_log(idsp_printf_t *record, u8 *pcode, u8 *pmemd)
{
	char *fmt;

	if (record->format_addr == 0)
		return;

	if (record->dsp_core == 0) {
		printf("[core:%d:%d] ", record->thread_id, record->seq_num);
		fmt = get_real_format_addr(record->format_addr, 0x00900000, pcode);
	} else
	if (record->dsp_core == 1) {
		printf("[memd:%d:%d] ", record->thread_id, record->seq_num);
		fmt = get_real_format_addr(record->format_addr, 0x00300000, pmemd);
	} else {
		printf("Unknown dsp_core %d", record->dsp_core);
		return;
	}
	printf(fmt, record->arg1, record->arg2, record->arg3, record->arg4, record->arg5);
}

int dsplog2_main(int argc, char **argv)
{
	FILE *input;
	int first = 0;
	int last = 0;
	int total = 0;
	idsp_printf_t record;
	char *code_name = NULL;
	char *memd_name = NULL;
	ucode_load_info_t info;
	int fd;
	u8 *pcode;
	u8 *pmemd;

	if (argc < 2) {
		fprintf(stderr, "Please specify input log file\n");
		fprintf(stderr, "For example, dsplog2 dsplog.dat firmware_dir > output_file\n");
		return -1;
	}

	if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		return -1;
	}

	if (info.nr_item < 2) {
		fprintf(stderr, "Please check nr_item!\n");
		return -1;
	}
	code_name = info.items[0].filename;
	memd_name = info.items[1].filename;

	if ((input = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "cannot open %s\n", argv[1]);
		return -1;
	}

	if (argc == 2) {
		if ((pcode = read_firmware("/lib/firmware", code_name)) == NULL)
			return -1;
		if ((pmemd = read_firmware("/lib/firmware", memd_name)) == NULL)
			return -1;
	} else {
		if ((pcode = read_firmware(argv[2], code_name)) == NULL)
			return -1;
		if ((pmemd = read_firmware(argv[2], memd_name)) == NULL)
			return -1;
	}

	for (;;) {
		int rval;
		if ((rval = fread(&record, 1, sizeof(record), input)) != sizeof(record)) {
			break;
		}
		if (first == 0) {
			first = record.seq_num;
			last = first - 1;
		}
		print_log(&record, pcode, pmemd);
		total++;
		++last;
//		if (++last!= record.seq_num)
//			fprintf(stderr, "\t%d records %d - %d lost\n",
//				record.seq_num - last,
//				last, record.seq_num - 1);
		last = record.seq_num;
		if ((total % 1000) == 0) {
			fprintf(stderr, "\r%d", total);
			fflush(stderr);
		}
	}
	fprintf(stderr, "\r");

	fprintf(stderr, "first record: %d\n", first);
	fprintf(stderr, "total record: %d\n", total);
	fprintf(stderr, " last record: %d\n", last);
	fprintf(stderr, "lost records: %d\n", (last - first + 1) - total);
	fclose(input);
	close(fd);
	return 0;
}

