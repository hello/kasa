/**
 * prebuild/sys_data/preload_idsp/host_adcfw.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "config.h"
#include "adc.h"

#if 0
#define DSP_HOST_DBG(format, arg...)	fprintf(stdout, format , ## arg)
#else
#define DSP_HOST_DBG(format, arg...)
#endif
#define DSP_HOST_ERR(format, arg...)	fprintf(stderr, format , ## arg)

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define BIN_ALIGN(x)		(((x) + 3) & ~3);

#define DEFAULT_BINARY_IDSPCFG_OFFSET	(8208)
#define IDSPCFG_BINARY_HEAD_SIZE			(64)

typedef enum {
	PARAM1_OUTPUT = 1,
	PARAM1_IDSPCFG_PATH,
	PARAM1_IDSPCFG_LIST,
	PARAM1_END,
	PARAM1_FIRST = PARAM1_OUTPUT,
	PARAM1_LAST = PARAM1_IDSPCFG_LIST,
} PARAM1_TYPE;

/* ==========================================================================*/
static int parse_line(const char *line, const char *prefix,
		char *binfile, char *binname)
{
	const char *bofn = line;

	*binfile = '\0';
	*binname = '\0';

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Check for comment */
	if (*line == '#' || *line == '\0') {
		return 0;
	}

	/* Get 'binfile' */
	if (prefix) {
		binfile += sprintf(binfile, "%s/", prefix);
	}
	while (*line != '\0' && !isspace(*line) && *line != '#') {
		if (*line == '/')
			bofn = line;
		*binfile++ = *line++;
	}
	*binfile = '\0';

	if (*bofn == '/') {
		bofn++;
	}

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	/* Check for comment */
	if (*line == '#' || *line == '\0') {
		while (*bofn != '\0' && !isspace(*bofn) && *bofn != '#') {
			*binname++ = *bofn++;
		}
		*binname = '\0';
		return 1;
	}

	/* Get 'binname' */
	while (*line != '\0' && !isspace(*line) && *line != '#') {
		*binname++ = *line++;
	}
	*binname = '\0';

	/* Consume rest of line */
	while (*line != '\0' && isspace(*line)) {
		line++;
	}

	if (*line != '\0' && *line != '#') {
		return -1;	/* Line has more than allowed # of args */
	}

	return 2;
}

static int fill_bin_file(FILE *foutput, FILE *fbin,
		struct smart3a_file_info *pfile_info)
{
	int rval = 0;
	int bin_size = 0;
	size_t tmp_size = 0;
	unsigned char buf[1024];

	//snprintf(pfile_info->name, sizeof(pfile_info->name), "%s", binname);
	//pfile_info->size = 0;
	pfile_info->offset = ftell(foutput);
	pfile_info->offset = BIN_ALIGN(pfile_info->offset);

	/* default value */
	pfile_info->r_gain = 6400;
	pfile_info->b_gain = 10000;
	pfile_info->d_gain = 4096;
	pfile_info->shutter = 1500;
	pfile_info->agc = 60;
	pfile_info->para0 = 0x00;
	pfile_info->para1 = 0x60;
	pfile_info->para2 = 0x00;
	pfile_info->para3 = 0x00;
	pfile_info->para4 = 0x80;

	rval = fseek(foutput, pfile_info->offset, SEEK_SET);
	if (rval < 0) {
		goto fill_bin_file_exit;
	}

	do {
		tmp_size = fread(buf, 1, sizeof(buf), fbin);
		if (tmp_size <= 0) {
			rval = tmp_size;
			break;
		}
		rval = fwrite(buf, 1, tmp_size, foutput);
		if (rval == tmp_size)
			bin_size += tmp_size;
		else
			rval = -1;
	} while (rval > 0);

	DSP_HOST_DBG("[%d: 0x%08X]\n", bin_size, pfile_info->offset);

fill_bin_file_exit:
	return bin_size;
}

static int fill_idspcfg_file(FILE *foutput, FILE *fidspcfg,
		struct smart3a_file_info *pfile_info)
{
	int rval = 0;
	size_t tmp_size;
	unsigned char buf[1024];
	int idspcfg_header = IDSPCFG_BINARY_HEAD_SIZE;

	rval = fseek(foutput, pfile_info->offset + DEFAULT_BINARY_IDSPCFG_OFFSET, SEEK_SET);
	if (rval < 0) {
		goto fill_idspcfg_file_exit;
	}

	do {
		tmp_size = fread(buf, 1, sizeof(buf), fidspcfg);
		if (tmp_size <= 0) {
			rval = tmp_size;
			break;
		}

		tmp_size -= idspcfg_header;

		rval = fwrite(&buf[idspcfg_header], 1, tmp_size , foutput);
		if (rval == tmp_size) {
			idspcfg_header = 0;
		} else {
			rval = -1;
		}
	} while (rval > 0);

	DSP_HOST_DBG("Add idspcfg[%d]: %s[%d @ 0x%08X + 0x%08X]\n",
		IDSPCFG_BINARY_HEAD_SIZE, pfile_info->name,
		pfile_info->size, pfile_info->offset, DEFAULT_BINARY_IDSPCFG_OFFSET);

fill_idspcfg_file_exit:
	return rval;
}

/* ==========================================================================*/
int main(int argc, char **argv)
{
	int rval = 0;
	const char *output;
	const char *idspcfg_path;
	const char *idspcfg_list;

	FILE *fidspcfg_list = NULL;
	FILE *foutput = NULL;
	int lineno;
	char buf[512];
	char binfile[512];
	char binname[512];
	FILE *fbin = NULL;
	int entries;
	struct adcfw_header hdr;

	if ((argc != PARAM1_LAST) && (argc != PARAM1_END)) {
		DSP_HOST_ERR("Usage: %s [adc.fw] [idspcfg_path] [idspcfg_list]\n", argv[0]);
		rval = -EINVAL;
		goto main_exit;
	}

	output = argv[PARAM1_OUTPUT];

	idspcfg_path = argv[PARAM1_IDSPCFG_PATH];
	idspcfg_list = argv[PARAM1_IDSPCFG_LIST];

	fidspcfg_list = fopen(idspcfg_list, "r");
	if (fidspcfg_list == NULL) {
		DSP_HOST_ERR("Unable to open '%s' for input!\n", idspcfg_list);
		rval = -EINVAL;
		goto main_exit;
	}

       /* Smart 3A */
        foutput = fopen(output, "wb");
	if (foutput == NULL) {
        	DSP_HOST_ERR("Unable to open '%s' for output!\n", output);
        	rval = -EINVAL;
        	goto main_exit;
	}

	rval = fseek(foutput, sizeof(struct adcfw_header), SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("fseek(%s) = %d\n", output, rval);
		goto main_exit;
	}

        memset(&hdr, 0, sizeof(struct adcfw_header));
        entries = 0;
        /*IDSP cfg */
	for (lineno = 1; fgets(buf, sizeof(buf), fidspcfg_list) != NULL; lineno++) {
		rval = parse_line(buf, idspcfg_path, binfile, binname);
		if (rval < 0) {
			DSP_HOST_ERR("line %d looks suspicious! ignored...\n",
				lineno);
			continue;
		}
		if (rval == 0) {
			/* Line is pure comment */
			continue;
		}
		if (entries >= ARRAY_SIZE(hdr.smart3a)) {
			DSP_HOST_ERR("Too many fidspcfg files! ignore[%s]\n", binfile);
			continue;
		}

		fbin = fopen(binfile, "rb");
		if (fbin == NULL) {
			DSP_HOST_ERR("Error: unable to open '%s'", binfile);
			rval = -EINVAL;
			goto main_exit;
		}

		DSP_HOST_DBG("IDSPCfg: process[%s]\n", binfile);
		rval = fill_bin_file(foutput, fbin, &hdr.smart3a[entries]);
		if (rval < 0) {
			DSP_HOST_ERR("Error: fill_bin_file(%s) = %d", binfile, rval);
			goto main_exit;
		}

		fclose(fbin);
		fbin = NULL;
		entries++;
	}

	hdr.smart3a_num = entries;
	hdr.smart3a_size = rval;

	hdr.magic = ADCFW_IMG_MAGIC;
	hdr.fw_size = ftell(foutput);

	rval = fseek(foutput, 0, SEEK_SET);
	if (rval < 0) {
		DSP_HOST_ERR("Error: HEAD fseek(%s) = %d\n", output, rval);
		goto main_exit;
	}
	rval = fwrite(&hdr, 1, sizeof(struct adcfw_header), foutput);
	if (rval != sizeof(struct adcfw_header)) {
		DSP_HOST_ERR("Error: HEAD fwrite(%s) = %d\n", output, rval);
		goto main_exit;
	}
	rval = 0;

main_exit:
	if (fidspcfg_list != NULL) {
		fclose(fidspcfg_list);
		fidspcfg_list = NULL;
	}
	if (foutput != NULL) {
		fclose(foutput);
		foutput = NULL;
	}
	if (fbin != NULL) {
		fclose(fbin);
		fbin = NULL;
	}

	return rval;
}

