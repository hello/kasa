/*
 *
 * mw_get_aaa_params.c
 *
 * History:
 *	2012/12/10 - [Jingyang Qiu] Created this file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <basetypes.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "basetypes.h"
#include "mw_defines.h"
#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"

#include "iav_common.h"
#include "iav_vin_ioctl.h"

#include "mw_aaa_params.h"
#include "mw_pri_struct.h"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

#define	READ_ALIGN(value, align)		((value + align -1) / align * align)

#define	TOTAL_STRUCT_NUM		(TOTAL_FILTER_NUM)
#define	INVALID_OFFSET			(0xff)

enum {
	NON_USED = 0,
	USED = 1,
};

typedef enum {
	NORMAL_EXPO_LINES = 0,
	HDR_2X_EXPO_LINES,
	HDR_3X_EXPO_LINES,
	HDR_4X_EXPO_LINES,
	PIRIS_LINEAR1,
	PIRIS_LINEAR2,
	PIRIS_HDR_2X,
	PIRIS_HDR_3X,
	EXPO_LINES_TOTAL,
} ae_lines_id;

typedef struct {
	u8 *buf[TOTAL_STRUCT_NUM];
	u32 size_list[TOTAL_STRUCT_NUM];
	u8 offset_num[ADJ_FILTER_NUM];
} filter_parser_param;

static filter_parser_param filter_parser;

int load_cc_bin(char *sensor_name)
{
	int file, count;
	char filename[128];
	u8 *matrix[4];
	u8 i;

	for (i = 0; i < 4; i++) {
		matrix[i] = malloc(AMBA_DSP_IMG_CC_3D_SIZE);
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			MW_ERROR("%s cannot be opened\n", filename);
			return -1;
		}
		if ((count = read(file, matrix[i], AMBA_DSP_IMG_CC_3D_SIZE)) !=
			AMBA_DSP_IMG_CC_3D_SIZE) {
			MW_ERROR("read %s error\n",filename);
			return -1;
		}
		close(file);
	}
	cc_binary_addr_t cc_addr;
	cc_addr.cc_0 = matrix[0];
	cc_addr.cc_1 = matrix[1];
	cc_addr.cc_2 = matrix[2];
	cc_addr.cc_3 = matrix[3];
	img_adj_load_cc_binary(&cc_addr);

	for(i=0; i<4; i++) {
		free(matrix[i]);
	}
	return 0;
}

int enable_cc(int fd, char *sensor_name, amba_img_dsp_mode_cfg_t *pMode)
{
	amba_img_dsp_color_correction_reg_t cc_reg;
	amba_img_dsp_color_correction_t cc_3d;

	u8 *reg, *matrix;
	char filename[128];
	int file, count;

	reg = malloc(AMBA_DSP_IMG_CC_REG_SIZE);
	matrix = malloc(AMBA_DSP_IMG_CC_3D_SIZE);

	sprintf(filename,"%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		MW_ERROR("%s cannot be opened\n", filename);
		return -1;
	}
	if ((count = read(file, reg, AMBA_DSP_IMG_CC_REG_SIZE)) !=
		AMBA_DSP_IMG_CC_REG_SIZE) {
		MW_ERROR("read %s error\n", filename);
		return -1;
	}
	close(file);

	sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, 2);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		MW_ERROR("%s cannot be opened\n", filename);
		return -1;
	}
	if ((count = read(file, matrix, AMBA_DSP_IMG_CC_3D_SIZE)) !=
		AMBA_DSP_IMG_CC_3D_SIZE) {
		MW_ERROR("read %s error\n", filename);
		return -1;
	}
	close(file);

	cc_reg.RegSettingAddr = (u32)reg;
	amba_img_dsp_set_color_correction_reg(pMode, &cc_reg);

	cc_3d.MatrixThreeDTableAddr = (u32)matrix;
	amba_img_dsp_set_color_correction(fd, pMode,&cc_3d);

	free(reg);
	free(matrix);
	return 0;
}

int load_dsp_cc_table(int fd, char *sensor_name, amba_img_dsp_mode_cfg_t *pMode)
{
	if (pMode == NULL) {
		MW_ERROR("Error:%s, the point is NULL \n", __func__);
		return -1;
	}
	load_cc_bin(sensor_name);
	enable_cc(fd, sensor_name, pMode);
	return 0;
}

static int read_adv_header(int fd, u32 *psize, int size)
{
	if (psize == NULL) {
		MW_ERROR("Error: The point is NULL.\n");
		return -1;
	}

	if (read(fd, psize, size) != size) {
		MW_ERROR("read");
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

static int init_parse_params(char *filename, filter_parser_param *parser)
{
	int fd = -1;
	int error_flag = 0;
	off_t read_addr;
	int id, size;

	do {
		MW_DEBUG("Load file:%s\n", filename);
		if ((fd  = open(filename, O_RDONLY, S_IREAD)) < 0) {
			error_flag = -1;
			MW_ERROR("Open file:%s error\n", filename);
			break;
		}

		size = TOTAL_STRUCT_NUM * sizeof(u32);
		memset(parser->size_list, 0, size);
		if ((read_adv_header(fd, parser->size_list, size)) < 0) {
			error_flag = -1;
			MW_ERROR("read_header");
			break;
		}

		lseek(fd, size, SEEK_SET);

		for(id = 0; id < TOTAL_STRUCT_NUM; id++) {
			parser->buf[id] = NULL;
			read_addr = lseek(fd, 0, SEEK_CUR);
			lseek(fd, READ_ALIGN(read_addr, 4), SEEK_SET);
			size = parser->size_list[id];

			if (size == 0) {
				continue;
			}
			if ((parser->buf[id] = (u8 *)malloc(size)) == NULL) {
				error_flag = -1;
				MW_ERROR("malloc");
				break;
			}
			memset(parser->buf[id], 0, size);
			if (read(fd, parser->buf[id], size) != size) {
				MW_ERROR("read data from Id[%d]", id);
				error_flag = -1;
				break;
			}
		}
	} while (0);

	if (error_flag) {
		deinit_parse_params(parser);
	}
	if (fd > 0) {
		close(fd);
		fd = -1;
	}

	return error_flag;
}

static int get_filter_offset(_mw_global_config *pMw_info)
{
	int offset = 0;
	_mw_sensor_param *sensor = NULL;
	mw_sys_res *resource = NULL;

	resource = &(pMw_info->res);
	sensor = &(pMw_info->sensor);

	if (resource->hdr_mode == HDR_PIPELINE_OFF) {
		if ((!sensor->vin_aaa_info.dual_gain_mode) &&
			(sensor->vin_aaa_info.hdr_mode ==
			AMBA_VIDEO_INT_HDR_MODE)) {
			offset = FILTER_OFFSET_WDR >> 24;
		} else {
			offset = FILTER_OFFSET_NORAML >> 24;
		}
	} else {
		switch (resource->hdr_expo_num) {
			case HDR_2X:
				offset = FILTER_OFFSET_2X_HDR >> 24;
				break;
			case HDR_3X:
				offset = FILTER_OFFSET_3X_HDR >> 24;
				break;
			case HDR_4X:
				offset = FILTER_OFFSET_4X_HDR >> 24;
				break;
			default:
				offset = FILTER_OFFSET_NORAML >> 24;
				break;
		}
	}

	return offset;
}


static inline int ignore_adj_list_HDR_OFF(int item)
{
	if ((item == ADJ_HDR_ALPHA) || (item == ADJ_HDR_THRESHOLD) ||
		(item == ADJ_HDR_CE)) {
		return 1;
	}

	return 0;
}

static inline int ignore_nonmatch_item(int v1, int v2, int flag)
{
	int ret = 0;

	/* Ignore the non-match items except default offset(0). */
	if ((v1 != (FILTER_OFFSET_NORAML >> 24)) && (v1 != v2)) {
		ret = 1;
	}
	/* Ignore the item which has used when offset = 0. */
	if ((v1 == (FILTER_OFFSET_NORAML >> 24)) && (flag == USED)) {
		ret = 1;
	}

	return ret;
}

static int set_iso_container(int reload_flag)
{
	fc_collection_t fcc;
	int offset = 0, item;
	int ret = 0;
	u8 *buf = NULL;

	memset(&fcc, 0, sizeof(fcc));
	for (item = ADJ_AE_TARGET; item < ADJ_FILTER_NUM; item++) {
		offset = filter_parser.offset_num[item];
		if (offset == INVALID_OFFSET) {
			continue;
		}
		buf = filter_parser.buf[offset];
		switch(item) {
		case ADJ_AE_TARGET:
			fcc.fc_ae_target = (filter_container_p8_t *)buf;
			break;
		case ADJ_WB_RATIO:
			fcc.fc_wb_ratio = (filter_container_p8_t *)buf;
			break;
		case ADJ_BLC:
			fcc.fc_blc = (filter_container_p8_t *)buf;
			break;
		case ADJ_ANTIALIASING:
			fcc.fc_antialiasing = (filter_container_p4_t *)buf;
			break;
		case ADJ_GRGB_MISMATCH:
			fcc.fc_grgbmismatch = (filter_container_p4_t *)buf;
			break;
		case ADJ_DPC:
			fcc.fc_dpc = (filter_container_p4_t *)buf;
			break;
		case ADJ_CFANF_LOW:
			fcc.fc_cfanf_low = (filter_container_p32_t *)buf;
			break;
		case ADJ_CFANF_HIGH:
			fcc.fc_cfanf_high = (filter_container_p32_t *)buf;
			break;
		case ADJ_LE:
			fcc.fc_le = (filter_container_p8_t1_t *)buf;
			break;
		case ADJ_DEMOSAIC:
			fcc.fc_demosaic = (filter_container_p8_t *)buf;
			break;
		case ADJ_CC:
			fcc.fc_cc = (filter_container_p4_t *)buf;
			break;
		case ADJ_TONE:
			fcc.fc_tone = (filter_container_p0_t3_t *)buf;
			break;
		case ADJ_RGB2YUV:
			fcc.fc_rgb2yuv = (filter_container_p4_t3_t *)buf;
			break;
		case ADJ_CHROMA_SCALE:
			fcc.fc_chroma_scale = (filter_container_p8_t1_t *)buf;
			break;
		case ADJ_CHROMA_MEDIAN:
			fcc.fc_chroma_median =(filter_container_p8_t *)buf;
			break;
		case ADJ_CDNR:
			fcc.fc_cdnr = (filter_container_p2_t *)buf;
			break;
		case ADJ_1STMODE_SEL:
			fcc.fc_1stmode_sel = (filter_container_p2_t *)buf;
			break;
		case ADJ_ASF:
			fcc.fc_asf = (filter_container_p64_t4_t *)buf;
			break;
		case ADJ_1ST_SHPBOTH:
			fcc.fc_1st_shpboth = (filter_container_p8_t *)buf;
			break;
		case ADJ_1ST_SHPNOISE:
			fcc.fc_1st_shpnoise = (filter_container_p64_t4_t *)buf;
			break;
		case ADJ_1ST_SHPFIR:
			fcc.fc_1st_shpfir = (filter_container_p16_t4_t *)buf;
			break;
		case ADJ_1ST_SHPCORING:
			fcc.fc_1st_shpcoring = (filter_container_p0_t1_t *)buf;
			break;
		case ADJ_1ST_SHPCORING_INDEX_SCALE:
			fcc.fc_1st_shpcoring_idx_scale = (filter_container_p8_t *)buf;
			break;
		case ADJ_1ST_SHPCORING_MIN_RESULT:
			fcc.fc_1st_shpcoring_min = (filter_container_p8_t *)buf;
			break;
		case ADJ_1ST_SHPCORING_SCALE_CORING:
			fcc.fc_1st_shpcoring_scale_coring = (filter_container_p8_t *)buf;
			break;
		case ADJ_VIDEO_MCTF:
			fcc.fc_video_mctf = (filter_container_p64_t *)buf;
			break;
		case ADJ_HDR_ALPHA:
			fcc.fc_hdr_alpha = (filter_container_p4_t *)buf;
			break;
		case ADJ_HDR_THRESHOLD:
			fcc.fc_hdr_threshold = (filter_container_p4_t *)buf;
			break;
		case ADJ_CHROMANF:
			fcc.fc_chroma_nf = (filter_container_p8_t *)buf;
			break;
		case ADJ_FINAL_SHPBOTH:
			fcc.fc_final_shpboth = (filter_container_p8_t *)buf;
			break;
		case ADJ_FINAL_SHPNOISE:
			fcc.fc_final_shpnoise = (filter_container_p64_t4_t *)buf;
			break;
		case ADJ_FINAL_SHPFIR:
			fcc.fc_final_shpfir = (filter_container_p16_t4_t *)buf;
			break;
		case ADJ_FINAL_SHPCORING:
			fcc.fc_final_shpcoring = (filter_container_p0_t1_t *)buf;
			break;
		case ADJ_FINAL_SHPCORING_INDEX_SCALE:
			fcc.fc_final_shpcoring_idx_scale = (filter_container_p8_t *)buf;
			break;
		case ADJ_FINAL_SHPCORING_MIN_RESULT:
			fcc.fc_final_shpcoring_min = (filter_container_p8_t *)buf;
			break;
		case ADJ_FINAL_SHPCORING_SCALE_CORING:
			fcc.fc_final_shpcoring_scale_coring = (filter_container_p8_t *)buf;
			break;
		case ADJ_WIDE_CHROMA_FILTER:
			fcc.fc_wide_chroma_filter = (filter_container_p8_t *)buf;
			break;
		case ADJ_WIDE_CHROMA_FILTER_COMBINE:
			fcc.fc_wide_chroma_filter_combine = (filter_container_p16_t *)buf;
			break;
		case ADJ_TEMPORAL_ADJUST:
			fcc.fc_video_mctf_temporal_adjust = (filter_container_p32_t *)buf;
			break;
		case ADJ_HDR_CE:
			fcc.fc_hdr_ce = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_ANTIALIASING:
			fcc.fc_mo_antialiasing = (filter_container_p4_t *)buf;
			break;
		case ADJ_MO_GRGB_MISMATCH:
			fcc.fc_mo_grgbmismatch = (filter_container_p4_t *)buf;
			break;
		case ADJ_MO_DPC:
			fcc.fc_mo_dpc = (filter_container_p4_t *)buf;
			break;
		case ADJ_MO_CFANF:
			fcc.fc_mo_cfanf = (filter_container_p32_t *)buf;
			break;
		case ADJ_MO_DEMOSAIC:
			fcc.fc_mo_demosaic = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_CHROMA_MEDIAN:
			fcc.fc_mo_chroma_median = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_CNF:
			fcc.fc_mo_chroma_nf = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_1STMODE_SEL:
			fcc.fc_mo_1stmode_sel = (filter_container_p2_t *)buf;
			break;
		case ADJ_MO_ASF:
			fcc.fc_mo_asf = (filter_container_p64_t4_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_BOTH:
			fcc.fc_mo_1st_shpboth = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_NOISE:
			fcc.fc_mo_1st_shpnoise = (filter_container_p64_t4_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_FIR:
			fcc.fc_mo_1st_shpfir = (filter_container_p16_t4_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_CORING:
			fcc.fc_mo_1st_shpcoring = (filter_container_p0_t1_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_CORING_INDEX_SCALE:
			fcc.fc_mo_1st_shpcoring_idx_scale = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_CORING_MIN_RESULT:
			fcc.fc_mo_1st_shpcoring_min = (filter_container_p8_t *)buf;
			break;
		case ADJ_MO_1ST_SHARPEN_CORING_SCALE_CORING:
			fcc.fc_mo_1st_shpcoring_scale_coring = (filter_container_p8_t *)buf;
			break;
		default:
			MW_ERROR("Error: The type:%d is unknown\n", item);
			ret = -1;
			break;
		}
	}

	if (ret == 0) {
		if (reload_flag) {
			img_dynamic_load_containers(&fcc);
		} else {
			img_adj_init_containers_liso(&fcc);
		}
	}

	return ret;
}

static int update_adj_buf_contents(void)
{
	/* To do, update buf for save*/
	return 0;
}

static int save_to_file(char *filename)
{
	int ret = 0;
	u32 *header_buf = NULL;
	int item;
	int fd = -1, size = 0;
	off_t read_addr;

	do {
		if ((fd = open(filename, O_RDWR | O_CREAT, 0)) < 0) {
			MW_ERROR("Open file: %s error\n", filename);
			ret = -1;
			break;
		}

		/* write the header to file */
		size = sizeof(u32) * TOTAL_STRUCT_NUM;
		header_buf = malloc(size);
		if (header_buf == NULL) {
			MW_ERROR("Malloc error");
			ret = -1;
			break;
		}
		memset(header_buf, 0, size);
		for (item = 0; item < ADJ_FILTER_NUM; item++) {
			if (filter_parser.offset_num[item] >= TOTAL_STRUCT_NUM) {
				header_buf[item] = 0;
			} else {
				header_buf[item] =
					filter_parser.size_list[filter_parser.offset_num[item]];
			}
		}
		if (write(fd, header_buf, size) != size) {
			MW_ERROR("write header error");
			ret = -1;
			break;
		}
		lseek(fd, size, SEEK_SET);

		/* Write data to file */
		for (item = 0; item < ADJ_FILTER_NUM; item++) {
			read_addr = lseek(fd, 0, SEEK_CUR);
			lseek(fd, READ_ALIGN(read_addr, 4), SEEK_SET);
			size = header_buf[item];
			MW_DEBUG("Save ADJ: item:%4d,\tsize:%8d\n", item, size);
			if (!size) {
				continue;
			}
			if (write(fd, filter_parser.buf[filter_parser.offset_num[item]],
				size) != size) {
				MW_ERROR("write item[%d] error", item);
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

static int parse_adj_params(_mw_global_config *pMw_info)
{
	int ret = 0;
	int id_num;
	int item = 0;
	int filter_flag[ADJ_FILTER_NUM];
	int hdr_offset;
	int operate_adj = pMw_info->init_params.operate_adj;
	u8 *buf = NULL;

	_mw_sensor_param *sensor = NULL;
	mw_sys_res *resource = NULL;
	mw_adj_file_param *adj = NULL;
	sensor = &(pMw_info->sensor);
	resource = &(pMw_info->res);
	adj = &(pMw_info->adj);
	char *filename = NULL;

	memset(filter_flag, NON_USED, ADJ_FILTER_NUM * sizeof(int));

	if (MW_ADJ_FILTER_NUM < ADJ_FILTER_NUM) {
		MW_ERROR("MW definition is not enough, "
			"can't maintain the whole adj filters\n");
		return -1;
	}

	if (TOTAL_STRUCT_NUM >= INVALID_OFFSET) {
		MW_ERROR("Definition error\n");
		return -1;
	}

	hdr_offset = get_filter_offset(pMw_info);

	do {
		/* parse adj binary file */
		if (operate_adj == OPERATE_LOAD) {
			filename = adj->filename;
		} else {
			filename = sensor->load_files.adj_file;
		}
		if (init_parse_params(filename, &filter_parser)) {
			ret = -1;
			break;
		}

		id_num = 0;
		memset(filter_parser.offset_num, INVALID_OFFSET, ADJ_FILTER_NUM);

		while (id_num < TOTAL_STRUCT_NUM) {
			buf = filter_parser.buf[id_num];
			if (buf == NULL) {
				id_num++;
				continue;
			}
			item = (int)(buf[0] + (buf[1] << 8) + (buf[2] << 16));

			if (item >= ADJ_FILTER_NUM) {
				MW_ERROR("The ID:%d is excess ADJ_FILTER_NUM:%d.\n",
					item, ADJ_FILTER_NUM);
				ret = -1;
				break;
			}

			if (ignore_nonmatch_item(buf[3], hdr_offset,
				filter_flag[item])) {
				id_num++;
				continue;
			}

			if (resource->hdr_mode == HDR_PIPELINE_OFF) {
				if(ignore_adj_list_HDR_OFF(item)) {
					id_num++;
					continue;
				}
			}

			filter_flag[item] = USED;
			if (operate_adj == OPERATE_LOAD) {
				if (adj->flag[item].apply) {
					adj->flag[item].done = 1;
					MW_DEBUG("Reload ADJ: item:%4d,\tsize:%8d,\toffset:%2d, %2d\n",
						item, filter_parser.size_list[id_num],
						hdr_offset, buf[3]);
				} else {
					id_num++;
					continue;
				}
			} else {
				MW_DEBUG("Load ADJ: item:%4d,\tsize:%8d,\toffset:%2d, %2d\n",
					item, filter_parser.size_list[id_num],
					hdr_offset, buf[3]);
			}

			filter_parser.offset_num[item] = id_num;
			id_num++;
		}
	} while (0);

	if (ret == 0) {
		if (operate_adj == OPERATE_SAVE) {
			filename = adj->filename;
			if (update_adj_buf_contents() < 0) {
				ret = -1;
			}
			if (save_to_file(filename) < 0) {
				ret = -1;
			}
		} else {
			if (set_iso_container(operate_adj) < 0) {
				ret = -1;
			}
		}
	}

	deinit_parse_params(&filter_parser);

	return ret;
}

int load_adj_file(_mw_global_config *pMw_info)
{
	pMw_info->init_params.operate_adj = OPERATE_LOAD;
	if (parse_adj_params(pMw_info) < 0) {
		MW_ERROR("%s", __func__);
		return -1;
	}

	return 0;
}

int save_adj_file(_mw_global_config *pMw_info)
{
	pMw_info->init_params.operate_adj = OPERATE_SAVE;
	if (parse_adj_params(pMw_info) < 0) {
		MW_ERROR("%s", __func__);
		return -1;
	}

	return 0;
}

static int parse_aeb_params(_mw_global_config *pMw_info)
{
	int error_flag = 0;
	int id_num;
	int i = 0;
	u8 *buf = NULL;

	int item = 0;
	img_aeb_tile_config_t *ptile_config = NULL;
	img_aeb_expo_lines_t *pexpo_line[EXPO_LINES_TOTAL] = {NULL};
	img_aeb_wb_param_t *pwb_params = NULL;
	img_aeb_sensor_config_t *paeb_sensor_config = NULL;
	img_aeb_sht_nl_table_t *psht_table = NULL;
	img_aeb_gain_table_t *pgain_table = NULL;
	img_aeb_expo_lines_t *p_tmp_lines = NULL;
	img_aeb_auto_knee_param_t *p_auto_knee = NULL;
	img_aeb_digit_wdr_param_t *p_digit_wdr[EXPO_LINES_TOTAL] = {NULL};
	img_aeb_digit_wdr_param_t *p_curr_digit_wdr = NULL;
	ae_cfg_tbl_t ae_tbl[EXPO_LINES_TOTAL];
	amba_img_dsp_mode_cfg_t ik_mode;
	amba_img_dsp_variable_range_t dsp_variable_range;
	int filter_flag[ADJ_FILTER_NUM];
	int hdr_offset;

	int fd_aaa = pMw_info->fd;
	_mw_sensor_param *sensor = NULL;
	mw_sys_res *resource = NULL;
	_mw_pri_iav *mw_iav = NULL;
	sensor = &(pMw_info->sensor);
	resource = &(pMw_info->res);
	mw_iav = &(pMw_info->mw_iav);
	memset(filter_flag, NON_USED, ADJ_FILTER_NUM * sizeof(int));

	hdr_offset = get_filter_offset(pMw_info);

	do {
		/* parse aeb binary file */
		if (init_parse_params(sensor->load_files.aeb_file, &filter_parser)) {
			error_flag = -1;
			break;
		}

		id_num = 0;
		while (id_num < TOTAL_STRUCT_NUM) {
			buf = filter_parser.buf[id_num];
			if (buf == NULL) {
				id_num++;
				continue;
			}
			item = (int)(buf[0] + (buf[1] << 8) + (buf[2] << 16));

			if (ignore_nonmatch_item(buf[3], hdr_offset,
				filter_flag[item])) {
				id_num++;
				continue;
			}

			filter_flag[item] = USED;

			MW_DEBUG("Load AEB: item:%4d,\tsize:%8d,\toffset:%2d, %2d\n",
				item, filter_parser.size_list[id_num],
				hdr_offset, buf[3]);

			switch(item) {
			case AEB_TILE_CONFIG:
				ptile_config = (img_aeb_tile_config_t *)buf;
				break;
			case AEB_EXPO_LINES:
				pexpo_line[NORMAL_EXPO_LINES] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_2X_HDR:		// TBD
				pexpo_line[HDR_2X_EXPO_LINES] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_3X_HDR:
				pexpo_line[HDR_3X_EXPO_LINES] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_4X_HDR:
				pexpo_line[HDR_4X_EXPO_LINES] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_WB_PARAM:
				pwb_params = (img_aeb_wb_param_t *)buf;
				break;
			case AEB_SENSOR_CONFIG:
				paeb_sensor_config = (img_aeb_sensor_config_t *)buf;
				break;
			case AEB_SHT_NL_TABLE:
				psht_table = (img_aeb_sht_nl_table_t *)buf;
				break;
			case AEB_GAIN_TABLE:
				pgain_table = (img_aeb_gain_table_t *)buf;
				break;
			case AEB_EXPO_LINES_PIRIS_LINEAR1:
				pexpo_line[PIRIS_LINEAR1] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_PIRIS_LINEAR2:
				pexpo_line[PIRIS_LINEAR2] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_PIRIS_HDR1:
				pexpo_line[PIRIS_HDR_2X] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_EXPO_LINES_PIRIS_HDR2:
				pexpo_line[PIRIS_HDR_3X] = (img_aeb_expo_lines_t *)buf;
				break;
			case AEB_AUTO_KNEE:
				p_auto_knee = (img_aeb_auto_knee_param_t *)buf;
				break;
			case AEB_DIGIT_WDR:
				p_digit_wdr[NORMAL_EXPO_LINES] = (img_aeb_digit_wdr_param_t *)buf;
				break;
			case AEB_DIGIT_WDR_2X_HDR:
				p_digit_wdr[HDR_2X_EXPO_LINES] = (img_aeb_digit_wdr_param_t *)buf;
				break;
			case AEB_DIGIT_WDR_3X_HDR:
				p_digit_wdr[HDR_3X_EXPO_LINES] = (img_aeb_digit_wdr_param_t *)buf;
				break;
			default:
				MW_ERROR("Error: The type:%d is unknown\n", item);
				error_flag = -1;
				break;
			}

			if (error_flag) {
				break;
			}
			id_num++;
		}

		if (error_flag) {
			break;
		}

		memset(&ik_mode, 0, sizeof(ik_mode));

		ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
		if (resource->hdr_mode != HDR_PIPELINE_OFF) {
			ik_mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_HDR;
		}

		if (resource->isp_pipeline == ISP_PIPELINE_LISO) {
			ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
		} else if (resource->isp_pipeline == ISP_PIPELINE_ADV_LISO ||
			resource->isp_pipeline == ISP_PIPELINE_MID_LISO ||
			resource->isp_pipeline == ISP_PIPELINE_B_LISO) {
			ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
			ik_mode.BatchId = 0xff;
		}

		dsp_variable_range.max_chroma_radius = 1 << (5 + mw_iav->max_cr);
		dsp_variable_range.max_wide_chroma_radius =
			1 << (5 + mw_iav->max_wcr);
		dsp_variable_range.inside_fpn_flag = 0;
		dsp_variable_range.wide_chroma_noise_filter_enable = mw_iav->wcr_enable;
		amba_img_dsp_set_variable_range(&ik_mode, &dsp_variable_range);

		load_dsp_cc_table(fd_aaa, sensor->name, &ik_mode);

		paeb_sensor_config->sensor_config.p_vindev_aaa_info =
			&(sensor->vin_aaa_info);
		if (img_config_sensor_info(&(paeb_sensor_config->sensor_config)) < 0) {
			MW_ERROR("Img_config_sensor_info error!\n");
			error_flag = -1;
			break;
		}

		img_config_stat_tiles(&(ptile_config->tile_config));

		if (resource->hdr_mode == HDR_PIPELINE_OFF) {
			switch (sensor->lens_id) {
			case LENS_M13VP288IR_ID:
				if (pexpo_line[PIRIS_LINEAR1] == NULL) {
					p_tmp_lines = pexpo_line[NORMAL_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_LINEAR1];
				}
				break;
			case LENS_MZ128BP2810ICR_ID:
				if (pexpo_line[PIRIS_LINEAR2] == NULL) {
					p_tmp_lines = pexpo_line[NORMAL_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_LINEAR2];
				}
				break;
			default:
				p_tmp_lines = pexpo_line[NORMAL_EXPO_LINES];
				break;
			}
			p_curr_digit_wdr = p_digit_wdr[NORMAL_EXPO_LINES];
		} else if (resource->hdr_expo_num == HDR_2X) {
			switch (sensor->lens_id) {
			case LENS_M13VP288IR_ID:
				if (pexpo_line[PIRIS_HDR_2X] == NULL) {
					p_tmp_lines = pexpo_line[HDR_2X_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_HDR_2X];
				}
				break;
			case LENS_MZ128BP2810ICR_ID:
				if (pexpo_line[PIRIS_HDR_2X] == NULL) {
					p_tmp_lines = pexpo_line[HDR_2X_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_HDR_2X];
				}
				break;
			default:
				p_tmp_lines = pexpo_line[HDR_2X_EXPO_LINES];
				break;
			}
			p_curr_digit_wdr = p_digit_wdr[HDR_2X_EXPO_LINES];
		}  else if (resource->hdr_expo_num == HDR_3X) {
			switch (sensor->lens_id) {
			case LENS_M13VP288IR_ID:
				if (pexpo_line[PIRIS_HDR_3X] == NULL) {
					p_tmp_lines = pexpo_line[HDR_3X_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_HDR_3X];
				}
				break;
			case LENS_MZ128BP2810ICR_ID:
				if (pexpo_line[PIRIS_HDR_3X] == NULL) {
					p_tmp_lines = pexpo_line[HDR_3X_EXPO_LINES];
				} else {
					p_tmp_lines = pexpo_line[PIRIS_HDR_3X];
				}
				break;
			default:
				p_tmp_lines = pexpo_line[HDR_3X_EXPO_LINES];
				break;
			}
			p_curr_digit_wdr = p_digit_wdr[HDR_3X_EXPO_LINES];
		} else {
			MW_ERROR("Error:can't find the proper ae lines for mode %d, expo num %d\n", \
				resource->hdr_mode, resource->hdr_expo_num);
			error_flag = -1;
			break;
		}

		for(i= 0; i < p_tmp_lines->header.total_tbl_num; i++) {
			ae_tbl[i].p_expo_lines = p_tmp_lines->expo_tables[i].expo_lines;
			ae_tbl[i].line_num = p_tmp_lines->header.item_per_tbl[i];
			ae_tbl[i].belt = p_tmp_lines->header.item_per_tbl[i];
			ae_tbl[i].db_step = sensor->vin_aaa_info.agc_step;
			ae_tbl[i].p_gain_tbl = pgain_table->gain_table;
			ae_tbl[i].gain_tbl_size = pgain_table->header.item_per_tbl[0];
			ae_tbl[i].p_sht_nl_tbl = psht_table->sht_nl_table;
			ae_tbl[i].sht_nl_tbl_size = psht_table->header.item_per_tbl[0];
		}

		img_config_ae_tables(ae_tbl, resource->hdr_expo_num);
		img_config_awb_param(&(pwb_params->wb_param));
		if (p_auto_knee) {
			if (img_config_auto_knee_info(p_auto_knee) < 0) {
				MW_ERROR("img_config_auto_knee_info error!\n");
				error_flag = -1;
				break;
			}
		}
		if (p_curr_digit_wdr) {
			if (img_config_digit_wdr_info(p_curr_digit_wdr) < 0) {
				MW_ERROR("img_config_digit_wdr_info error!\n");
				error_flag = -1;
				break;
			}
		}

	} while (0);

	deinit_parse_params(&filter_parser);

	return error_flag;
}


static int parse_lens_params(_mw_global_config *pMw_info)
{
	int ret = 0;
	int id_num;
	u8 *buf_lens = NULL;
	int item = 0;

	lens_piris_step_table_t *tmp_piris_step = NULL;
	lens_piris_fno_scope_t *tmp_piris_scope = NULL;
	_mw_sensor_param *sensor = &pMw_info->sensor;
	lens_param_t lens_param_info = {
		{NULL, NULL, 0, NULL},
	};
	lens_cali_t lens_cali_info = {
		NORMAL_RUN,
		{92},
	};

	do {
		/* parse lens binary file */
		if (init_parse_params(sensor->load_files.lens_file, &filter_parser)) {
			ret = -1;
			break;
		}

		id_num = PIRIS_DGAIN_TABLE;
		while (id_num < PIRIS_ID_NUM) {
			buf_lens = filter_parser.buf[id_num];
			if (buf_lens == NULL) {
				id_num++;
				continue;
			}
			item = (int)(buf_lens[0] + (buf_lens[1] << 8) +
				(buf_lens[2] << 16));
			MW_DEBUG("Load Lens: item:%4d,\tsize:%8d,\toffset: %2d\n",
				item, filter_parser.size_list[id_num],
				buf_lens[3]);
			switch(item) {
				case PIRIS_DGAIN_TABLE:
					lens_param_info.piris_std.dgain =
						(lens_piris_dgain_table_t *)buf_lens;
					break;
				case PIRIS_FNO_SCOPE:
					tmp_piris_scope = (lens_piris_fno_scope_t *)buf_lens;
					lens_param_info.piris_std.scope = tmp_piris_scope->table;
					break;
				case PIRIS_STEP_TABLE:
					tmp_piris_step = (lens_piris_step_table_t *)buf_lens;
					lens_param_info.piris_std.table = tmp_piris_step->table;
					lens_param_info.piris_std.tbl_size = tmp_piris_step->header.array_size;
					break;
				default:
					printf("Error: The type:%d is unknown\n", item);
					ret = -1;
					break;
			}

			if (ret < 0) {
				break;
			}
			id_num++;
		}
	} while (0);

	do {
		if (ret < 0) {
			break;
		}
		if(img_config_lens_cali(&lens_cali_info) < 0) {
			MW_ERROR("img_config_lens_cali error!\n");
			ret = -1;
			break;
		}

		if (img_config_lens_info(sensor->lens_id) < 0) {
			MW_ERROR("img_config_lens_info error!\n");
			ret = -1;
			break;
		}

		if (img_load_lens_param(&lens_param_info) < 0) {
			MW_ERROR("img_load_lens_param error!\n");
			ret = -1;
			break;
		}

		if (img_lens_init() < 0) {
			MW_ERROR("img_lens_init error!\n");
			ret = -1;
			break;
		}

		if (lens_param_info.piris_std.scope != NULL) {
			pMw_info->ae_params.lens_aperture.FNO_min =
				(u32)lens_param_info.piris_std.scope[0];
			pMw_info->ae_params.lens_aperture.FNO_max =
				(u32)lens_param_info.piris_std.scope[1];
		} else {
			pMw_info->ae_params.lens_aperture.FNO_min = 0;
			pMw_info->ae_params.lens_aperture.FNO_max = 0;
		}
		MW_DEBUG("MW lens FNO:0x%x, 0x%x\n",
			pMw_info->ae_params.lens_aperture.FNO_min,
			pMw_info->ae_params.lens_aperture.FNO_max);
	} while (0);

	deinit_parse_params(&filter_parser);

	return ret;
}

int get_sensor_aaa_params_from_bin(_mw_global_config *pMw_info)
{
	int ret = 0;
	if (pMw_info == NULL) {
		MW_ERROR("The point is NULL\n");
		return -1;
	}
	MW_DEBUG("MW IAV resource:%d, %d, %d, %d\n",
		pMw_info->res.isp_pipeline, pMw_info->res.hdr_mode,
		pMw_info->res.hdr_expo_num, pMw_info->res.raw_pitch);
	/* clear dynamic state */
	memset(&pMw_info->adj, 0, sizeof(mw_adj_file_param));
	do {
		if ((ret = parse_aeb_params(pMw_info)) < 0) {
			MW_ERROR("error: parse_aeb_params\n");
			break;
		}
		if ((pMw_info->sensor.lens_id != LENS_CMOUNT_ID) &&
			(strcmp(pMw_info->sensor.load_files.lens_file, "") != 0)) {
			if ((ret = parse_lens_params(pMw_info)) < 0) {
				MW_ERROR("error: parse_lens_params\n");
				break;
			}
		}
		pMw_info->init_params.operate_adj = OPERATE_NONE;
		if ((ret = parse_adj_params(pMw_info)) < 0) {
			MW_ERROR("error: parse_adj_params\n");
			break;
		}
	} while (0);

	return ret;
}

