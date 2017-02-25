/*
 *
 * upgrade_adj_bin.c
 *
 * History:
 *	2015/09/10 - [Jingyang Qiu] Created this file
 *
 * Description :
 *	Generate new adj param binary based on input adj param file and tuned file.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <basetypes.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "img_abs_filter.h"
#include "img_adv_struct_arch.h"

typedef enum {
	ERROR_LEVEL = 0,
	INFO_LEVEL,
	DEBUG_LEVEL,
} LOG_LEVEL;

#define ITEM_EXIST		1

#define	PARSE_PRINT(mylog, LOG_LEVEL, format, args...)		do {		\
			if (mylog >= LOG_LEVEL) {			\
				printf(format, ##args);			\
			}									\
		} while (0)


static int G_log_level = INFO_LEVEL;

#define PARSE_ERROR(format, args...)		PARSE_PRINT(G_log_level, \
		ERROR_LEVEL, "!!! Error [%s: %d]" format "\n", \
		__FILE__, __LINE__, ##args)
#define PARSE_INFO(format, args...)		PARSE_PRINT(G_log_level, \
		INFO_LEVEL, "::: " format, ##args)
#define PARSE_DEBUG(format, args...)	PARSE_PRINT(G_log_level, \
		DEBUG_LEVEL, "=== " format, ##args)


typedef enum {
	FILE_INPUT_ADJ = 0,
	FILE_INPUT_PARAMS = 1,
	FILE_OUTPUT_ADJ = 2,
	FILE_TYPE_TOTAL_NUM,
	FILE_TYPE_FIRST = FILE_INPUT_ADJ,
	FILE_TYPE_LAST = FILE_TYPE_TOTAL_NUM,
} FILE_TYPE;

#define NO_ARG	0
#define HAS_ARG	1

#define	INPUT_ADJ_FILE_OPTIONS		\
	{"input", HAS_ARG, 	0, 	'i'},
#define	INPUT_ADJ_FILE_HINTS			\
	{"", "\tthe adj file which need update"},

#define	PARAMS_TXT_OPTIONS		\
	{"params", HAS_ARG, 	0, 	't'},
#define	PARAMS_TXT_HINTS			\
	{"", "\tthe params file which tuned by amagetool"},

#define	DB_ID_OPTIONS		\
	{"db", HAS_ARG, 	0, 	'd'},
#define	DB_ID_HINTS			\
	{"0~15", "\tThe DB ID of tuned params file, ID = 2 + db/6."},

#define	OUT_ADJ_FILE_OPTIONS		\
	{"output", HAS_ARG, 	0, 	'o'},
#define	OUT_ADJ_FILE_HINTS			\
	{"", "\tthe adj file which will generate"},

#define	LOG_LEVEL_OPTIONS		\
	{"log-level", HAS_ARG, 	0, 	'm'},
#define	LOG_LEVEL_HINTS			\
	{"0~2", "\tset log level"},

static const char* short_options = "i:t:o:d:m:";
static struct option long_options[] = {
	INPUT_ADJ_FILE_OPTIONS
	PARAMS_TXT_OPTIONS
	DB_ID_OPTIONS
	OUT_ADJ_FILE_OPTIONS
	LOG_LEVEL_OPTIONS
	{0, 0, 0, 0},
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	INPUT_ADJ_FILE_HINTS
	PARAMS_TXT_HINTS
	DB_ID_HINTS
	OUT_ADJ_FILE_HINTS
	LOG_LEVEL_HINTS
	{0, 0},
};

static void usage(char *appname)
{
	int i;
	printf("%s usage:\n", appname);
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("\nNotes:\n");
	printf("\t1. The input adj file must no same ADJ ID in it.\n");
	printf("\t   To confirm no the same ADJ_ID, could use test_image parse it.\n");
	printf("\t\t# tset_image -i 1 > a > x > input_adj.bin.\n");
	printf("\t2. The output adj can not instead of the default adj file"
		"(/etc/idsp/adj_params/xxx_xiso_adj_param.bin).\n");
	printf("\t   Because some ADJ ID has serval group params "
		"for different states,\n");
	printf("\t   if using the adj bin, it may cause incorrect params "
		"for adj\n");

	printf("\nExamples:\n");
	printf("\t# %s -i input_adj.bin -t 6db.txt -d 3 -o out_adj.bin\n", appname);

	printf("\nVerifications:\n");
	printf("\t1. test_image start 3A with it.\n");
	printf("\t\t# tset_image -i 1 -d out_adj.bin\n");
	printf("\t2. test_image reload it when 3A working.\n");
	printf("\t\t# tset_image -i 1 > a > L > out_adj.bin\n");
}

#include "adj_container_map.c"

typedef struct {
	u8 *buf[TOTAL_STRUCT_NUM];
	u32 size_list[TOTAL_STRUCT_NUM];
	u8 offset_num[ADJ_FILTER_NUM];
} filter_parser_param;

#define FILE_NAME_LENGTH	64
static int	load_file_flag[FILE_TYPE_TOTAL_NUM] = {0};
static char	G_file_name[FILE_TYPE_TOTAL_NUM][FILE_NAME_LENGTH] = {""};
static int G_db_id = 0;

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int i;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'i':
			strncpy(G_file_name[FILE_INPUT_ADJ], optarg,
				sizeof(G_file_name[FILE_INPUT_ADJ]));
			G_file_name[FILE_INPUT_ADJ][sizeof(G_file_name[FILE_INPUT_ADJ]) - 1] = '\0';
			load_file_flag[FILE_INPUT_ADJ] = 1;
			break;
		case 't':
			strncpy(G_file_name[FILE_INPUT_PARAMS], optarg,
				sizeof(G_file_name[FILE_INPUT_PARAMS]));
			G_file_name[FILE_INPUT_PARAMS][sizeof(G_file_name[FILE_INPUT_PARAMS]) - 1] = '\0';
			load_file_flag[FILE_INPUT_PARAMS] = 1;
			break;
		case 'o':
			strncpy(G_file_name[FILE_OUTPUT_ADJ], optarg,
				sizeof(G_file_name[FILE_OUTPUT_ADJ]));
			G_file_name[FILE_OUTPUT_ADJ][sizeof(G_file_name[FILE_OUTPUT_ADJ]) - 1] = '\0';
			load_file_flag[FILE_OUTPUT_ADJ] = 1;
			break;
		case 'd':
			i = atoi(optarg);
			if (i < 0 || i >= ADJ_MAX_ENTRY_NUM) {
				PARSE_ERROR("Error, the range is [0~%d]\n", ADJ_MAX_ENTRY_NUM);
				return -1;
			}
			G_db_id = i;
			break;
		case 'm':
			i = atoi(optarg);
			G_log_level = i;
			break;
		default:
			PARSE_ERROR("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}

static int read_adv_header(int fd, u32 *psize, int size)
{
	if (psize == NULL) {
		PARSE_ERROR("Error: The point is NULL.\n");
		return -1;
	}

	if (read(fd, psize, size) != size) {
		PARSE_ERROR("read");
		return -1;
	}

	return 0;
}

static inline void deinit_parse_params(filter_parser_param *parser)
{
	int id;

	for (id = 0; id < TOTAL_STRUCT_NUM; id++) {
		parser->size_list[id] = 0;
		if (parser->buf[id] != NULL) {
			free(parser->buf[id]);
			parser->buf[id] = NULL;
		}
	}
}

static int check_intput_adjfile_item(filter_parser_param *parser)
{
	int id_num = 0, ret= 0;
	u8 *buf = NULL;
	int item = 0;
	int filter_flag[ADJ_FILTER_NUM] = {0};

	while (id_num < TOTAL_STRUCT_NUM) {
		buf = parser->buf[id_num];
		if (buf == NULL) {
			id_num++;
			continue;
		}
		item = (int)(buf[0] + (buf[1] << 8) + (buf[2] << 16));

		if (item >= ADJ_FILTER_NUM) {
			PARSE_ERROR("The ID:%d is excess ADJ_FILTER_NUM:%d "
				"in intput adj file\n", item, ADJ_FILTER_NUM);
			ret = -1;
			break;
		}

		if (filter_flag[item] == ITEM_EXIST) {
			PARSE_ERROR("The ID:%d exist multiple in the input adj file, "
				"please refer to usage, run test_image parse it.\n", item);
			ret = -1;
			break;
		}

		filter_flag[item] = ITEM_EXIST;
		id_num++;
	}

	return ret;
}

static int init_parse_params(char *filename, filter_parser_param *parser)
{
	int fd = -1;
	int ret = 0;
	off_t read_addr;
	int id, size;

	do {
		PARSE_INFO("Load file:%s\n", filename);
		if ((fd  = open(filename, O_RDONLY, S_IREAD)) < 0) {
			ret = -1;
			PARSE_ERROR("Open file:%s error\n", filename);
			break;
		}

		size = TOTAL_STRUCT_NUM * sizeof(u32);
		memset(parser->size_list, 0, size);
		if ((read_adv_header(fd, parser->size_list, size)) < 0) {
			ret = -1;
			PARSE_ERROR("read_header");
			break;
		}

		lseek(fd, size, SEEK_SET);

		for(id = 0; id < TOTAL_STRUCT_NUM; id++) {
			parser->buf[id] = NULL;
			read_addr = lseek(fd, 0, SEEK_CUR);
			lseek(fd, READ_ALIGN(read_addr, 4), SEEK_SET);
			size = parser->size_list[id];

			PARSE_DEBUG("read data from Id[%d], size %d\n", id, size);
			if (size == 0) {
				continue;
			}
			if ((parser->buf[id] = (u8 *)malloc(size)) == NULL) {
				ret = -1;
				PARSE_ERROR("malloc");
				break;
			}
			memset(parser->buf[id], 0, size);
			if (read(fd, parser->buf[id], size) != size) {
				PARSE_ERROR("read data from Id[%d]\n", id);
				ret = -1;
				break;
			}
		}
		if (check_intput_adjfile_item(parser) < 0) {
			PARSE_ERROR("check_intput_adjfile_item");
			ret = -1;
			break;
		}
	} while (0);

	if (ret) {
		deinit_parse_params(parser);
	}
	if (fd > 0) {
		close(fd);
		fd = -1;
	}

	return ret;
}


static int set_iso_container(filter_parser_param *filter_parser, int Gain_ID)
{
	fc_collection_t fcc;
	int item;
	int ret = 0;
	u8 *buf = NULL;
	int *update_param = NULL;

	memset(&fcc, 0, sizeof(fcc));
	for (item = ADJ_AE_TARGET; item < ADJ_FILTER_NUM; item++) {
		if (filter_parser->size_list[item] == 0) {
			continue;
		}
		buf = filter_parser->buf[item];
		update_param = (int *)&g_parser[item][0].argu[0];
		switch(item) {
		case ADJ_AE_TARGET:
			fcc.fc_ae_target = (filter_container_p8_t *)buf;
			break;
		case ADJ_WB_RATIO:
			fcc.fc_wb_ratio = (filter_container_p8_t *)buf;
			break;
		case ADJ_BLC:
			fcc.fc_blc = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_blc->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_ANTIALIASING:
			fcc.fc_antialiasing = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_antialiasing->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_GRGB_MISMATCH:
			fcc.fc_grgbmismatch = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_grgbmismatch->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_DPC:
			fcc.fc_dpc = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_dpc->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_CFANF_LOW:
			fcc.fc_cfanf_low = (filter_container_p32_t *)buf;
			memcpy(&fcc.fc_cfanf_low->param[Gain_ID].argu[0], update_param, sizeof(argu32));
			break;
		case ADJ_CFANF_HIGH:
			fcc.fc_cfanf_high = (filter_container_p32_t *)buf;
			memcpy(&fcc.fc_cfanf_high->param[Gain_ID].argu[0], update_param, sizeof(argu32));
			break;
		case ADJ_LE:
			fcc.fc_le = (filter_container_p8_t1_t *)buf;
			memcpy(&fcc.fc_le->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			memcpy(&fcc.fc_le->tbl0[0], &g_parser[item][1], sizeof(table_u16x256));
			break;
		case ADJ_DEMOSAIC:
			fcc.fc_demosaic = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_demosaic->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_CC:
			fcc.fc_cc = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_cc->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_TONE:
			fcc.fc_tone = (filter_container_p0_t3_t *)buf;
			memcpy(&fcc.fc_tone->tbl0[0], &g_parser[item][1], sizeof(table_u16x256));
			memcpy(&fcc.fc_tone->tbl1[0], &g_parser[item][2], sizeof(table_u16x256));
			memcpy(&fcc.fc_tone->tbl2[0], &g_parser[item][3], sizeof(table_u16x256));
			break;
		case ADJ_RGB2YUV:
			fcc.fc_rgb2yuv = (filter_container_p4_t3_t *)buf;
			memcpy(&fcc.fc_rgb2yuv->tbl0[0], &g_parser[item][1], sizeof(table_s16x16));
			memcpy(&fcc.fc_rgb2yuv->tbl1[0], &g_parser[item][1], sizeof(table_s16x16));
			memcpy(&fcc.fc_rgb2yuv->tbl2[0], &g_parser[item][1], sizeof(table_s16x16));
			break;
		case ADJ_CHROMA_SCALE:
			fcc.fc_chroma_scale = (filter_container_p8_t1_t *)buf;
			memcpy(&fcc.fc_chroma_scale->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			memcpy(&fcc.fc_chroma_scale->tbl0[0], &g_parser[item][1], sizeof(table_u16x256));
			break;
		case ADJ_CHROMA_MEDIAN:
			fcc.fc_chroma_median =(filter_container_p8_t *)buf;
			memcpy(&fcc.fc_chroma_median->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_CDNR:
			fcc.fc_cdnr = (filter_container_p2_t *)buf;
			memcpy(&fcc.fc_cdnr->param[Gain_ID].argu[0], update_param, sizeof(argu2));
			break;
		case ADJ_1STMODE_SEL:
			fcc.fc_1stmode_sel = (filter_container_p2_t *)buf;
			memcpy(&fcc.fc_1stmode_sel->param[Gain_ID].argu[0], update_param, sizeof(argu2));
			break;
		case ADJ_ASF:
			fcc.fc_asf = (filter_container_p64_t4_t *)buf;
			memcpy(&fcc.fc_asf->param[Gain_ID].argu[0], update_param, sizeof(argu64));
			memcpy(&fcc.fc_asf->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x16));
			memcpy(&fcc.fc_asf->tbl1[Gain_ID], &g_parser[item][2], sizeof(table_u16x16));
			memcpy(&fcc.fc_asf->tbl2[Gain_ID], &g_parser[item][3], sizeof(table_u16x16));
			memcpy(&fcc.fc_asf->tbl3[Gain_ID], &g_parser[item][4], sizeof(table_s16x256));
			break;
		case ADJ_1ST_SHPBOTH:
			fcc.fc_1st_shpboth = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_1st_shpboth->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_1ST_SHPNOISE:
			fcc.fc_1st_shpnoise = (filter_container_p64_t4_t *)buf;
			memcpy(&fcc.fc_1st_shpnoise->param[Gain_ID].argu[0], update_param, sizeof(argu64));
			memcpy(&fcc.fc_1st_shpnoise->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpnoise->tbl1[Gain_ID], &g_parser[item][2], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpnoise->tbl2[Gain_ID], &g_parser[item][3], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpnoise->tbl3[Gain_ID], &g_parser[item][4], sizeof(table_s16x256));
			break;
		case ADJ_1ST_SHPFIR:
			fcc.fc_1st_shpfir = (filter_container_p16_t4_t *)buf;
			memcpy(&fcc.fc_1st_shpfir->param[Gain_ID].argu[0], update_param, sizeof(argu16));
			memcpy(&fcc.fc_1st_shpfir->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpfir->tbl1[Gain_ID], &g_parser[item][2], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpfir->tbl2[Gain_ID], &g_parser[item][3], sizeof(table_u16x16));
			memcpy(&fcc.fc_1st_shpfir->tbl3[Gain_ID], &g_parser[item][4], sizeof(table_s16x256));
			break;
		case ADJ_1ST_SHPCORING:
			fcc.fc_1st_shpcoring = (filter_container_p0_t1_t *)buf;
			memcpy(&fcc.fc_1st_shpcoring->param.argu[0], update_param, sizeof(argu2));
			memcpy(&fcc.fc_1st_shpcoring->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x256));

			break;
		case ADJ_1ST_SHPCORING_INDEX_SCALE:
			fcc.fc_1st_shpcoring_idx_scale = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_1st_shpcoring_idx_scale->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_1ST_SHPCORING_MIN_RESULT:
			fcc.fc_1st_shpcoring_min = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_1st_shpcoring_min->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_1ST_SHPCORING_SCALE_CORING:
			fcc.fc_1st_shpcoring_scale_coring = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_1st_shpcoring_scale_coring->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_VIDEO_MCTF:
			fcc.fc_video_mctf = (filter_container_p64_t *)buf;
			memcpy(&fcc.fc_video_mctf->param[Gain_ID].argu[0], update_param, sizeof(argu64));
			break;
		case ADJ_HDR_ALPHA:
			fcc.fc_hdr_alpha = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_hdr_alpha->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_HDR_THRESHOLD:
			fcc.fc_hdr_threshold = (filter_container_p4_t *)buf;
			memcpy(&fcc.fc_hdr_threshold->param[Gain_ID].argu[0], update_param, sizeof(argu4));
			break;
		case ADJ_CHROMANF:
			fcc.fc_chroma_nf = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_chroma_nf->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_FINAL_SHPBOTH:
			fcc.fc_final_shpboth = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_final_shpboth->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_FINAL_SHPNOISE:
			fcc.fc_final_shpnoise = (filter_container_p64_t4_t *)buf;
			memcpy(&fcc.fc_final_shpnoise->param[Gain_ID].argu[0], update_param, sizeof(argu64));
			memcpy(&fcc.fc_final_shpnoise->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpnoise->tbl1[Gain_ID], &g_parser[item][2], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpnoise->tbl2[Gain_ID], &g_parser[item][3], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpnoise->tbl3[Gain_ID], &g_parser[item][4], sizeof(table_s16x256));
			break;
		case ADJ_FINAL_SHPFIR:
			fcc.fc_final_shpfir = (filter_container_p16_t4_t *)buf;
			memcpy(&fcc.fc_final_shpfir->param[Gain_ID].argu[0], update_param, sizeof(argu16));
			memcpy(&fcc.fc_final_shpfir->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpfir->tbl1[Gain_ID], &g_parser[item][2], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpfir->tbl2[Gain_ID], &g_parser[item][3], sizeof(table_u16x16));
			memcpy(&fcc.fc_final_shpfir->tbl3[Gain_ID], &g_parser[item][4], sizeof(table_s16x256));
			break;
		case ADJ_FINAL_SHPCORING:
			fcc.fc_final_shpcoring = (filter_container_p0_t1_t *)buf;
			memcpy(&fcc.fc_final_shpcoring->param.argu[0], update_param, sizeof(argu2));
			memcpy(&fcc.fc_final_shpcoring->tbl0[Gain_ID], &g_parser[item][1], sizeof(table_u16x256));
			break;
		case ADJ_FINAL_SHPCORING_INDEX_SCALE:
			fcc.fc_final_shpcoring_idx_scale = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_final_shpcoring_idx_scale->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_FINAL_SHPCORING_MIN_RESULT:
			fcc.fc_final_shpcoring_min = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_final_shpcoring_min->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_FINAL_SHPCORING_SCALE_CORING:
			fcc.fc_final_shpcoring_scale_coring = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_final_shpcoring_scale_coring->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_WIDE_CHROMA_FILTER:
			fcc.fc_wide_chroma_filter = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_wide_chroma_filter->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		case ADJ_WIDE_CHROMA_FILTER_COMBINE:
			fcc.fc_wide_chroma_filter_combine = (filter_container_p16_t *)buf;
			memcpy(&fcc.fc_wide_chroma_filter_combine->param[Gain_ID].argu[0], update_param, sizeof(argu16));
			break;
		case ADJ_TEMPORAL_ADJUST:
			fcc.fc_video_mctf_temporal_adjust = (filter_container_p32_t *)buf;
			memcpy(&fcc.fc_video_mctf_temporal_adjust->param[Gain_ID].argu[0], update_param, sizeof(argu32));
			break;
		case ADJ_HDR_CE:
			fcc.fc_hdr_ce = (filter_container_p8_t *)buf;
			memcpy(&fcc.fc_hdr_ce->param[Gain_ID].argu[0], update_param, sizeof(argu8));
			break;
		default:
			PARSE_ERROR("Error: The type:%d is unknown\n", item);
			ret = -1;
			break;
		}
	}

	return ret;
}

static int save_to_file(char *filename, filter_parser_param *filter_parser)
{
	int ret = 0;
	u32 *header_buf = NULL;
	int item;
	int fd = -1, size = 0;
	off_t read_addr;

	do {
		if ((fd = open(filename, O_RDWR | O_CREAT, 0)) < 0) {
			PARSE_ERROR("Open file: %s error\n", filename);
			ret = -1;
			break;
		}

		/* write the header to file */
		size = sizeof(u32) * TOTAL_STRUCT_NUM;
		header_buf = malloc(size);
		if (header_buf == NULL) {
			PARSE_ERROR("Malloc error");
			ret = -1;
			break;
		}
		memset(header_buf, 0, size);
		memcpy(header_buf, &filter_parser->size_list[0], size);
		if (write(fd, header_buf, size) != size) {
			PARSE_ERROR("write header error");
			ret = -1;
			break;
		}
		lseek(fd, size, SEEK_SET);

		/* Write data to file */
		for (item = 0; item < ADJ_FILTER_NUM; item++) {
			read_addr = lseek(fd, 0, SEEK_CUR);
			lseek(fd, READ_ALIGN(read_addr, 4), SEEK_SET);
			size = header_buf[item];
			PARSE_DEBUG("Save ADJ: item:%4d,\tsize:%8d, offset;0x%x\n", item, size, (u32)read_addr);
			if (!size) {
				continue;
			}
			if (write(fd, filter_parser->buf[item], size) != size) {
				PARSE_ERROR("write item[%d] error", item);
				ret = -1;
				break;
			}
		}
	} while (0);

	if (header_buf != NULL) {
		free(header_buf);
		header_buf = NULL;
	}
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	return ret;
}

int main(int argc, char ** argv)
{
	int i;
	filter_parser_param parser;
	int ret = 0;

	if (argc < 2) {
		usage(argv[0]);
		return -1;
	}

	if (init_param(argc, argv) < 0) {
		return -1;
	}

	for (i = FILE_TYPE_FIRST; i < FILE_TYPE_LAST; i++) {
		if (load_file_flag[i] == 0) {
			PARSE_ERROR("Error: params are not enough\n");
			usage(argv[0]);
			return -1;
		}
	}

	do {
		if (parse_adj_txt_file(G_file_name[FILE_INPUT_PARAMS]) < 0) {
			PARSE_ERROR("parse_adj_txt_file\n");
			ret = -1;
			break;
		}

		if (init_parse_params(G_file_name[FILE_INPUT_ADJ], &parser) < 0) {
			PARSE_ERROR("init_parse_params\n");
			ret = -1;
			break;
		}

		if (set_iso_container(&parser, G_db_id) < 0) {
			PARSE_ERROR("set_iso_container\n");
			ret = -1;
			break;
		}

		if (save_to_file(G_file_name[FILE_OUTPUT_ADJ], &parser) < 0) {
			PARSE_ERROR("save_to_file\n");
			ret = -1;
			break;
		}
	} while (0);

	deinit_parse_params(&parser);
	if (ret == 0) {
		PARSE_INFO("Generate %s sucessful\n", G_file_name[FILE_OUTPUT_ADJ]);
	}

	return ret;
}
