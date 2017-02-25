/*
 *
 * mw_aaa_params.h
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

#ifndef __SENSOR_ADJ_AEB_FILE_H__
#define __SENSOR_ADJ_AEB_FILE_H__

#include "mw_struct.h"
typedef enum  {
	LIST_TILE_CONFIG,
	LIST_LINES,
	LIST_AWB_PARAM,
	LIST_SENSOR_CONFIG,
	LIST_SHT_NL_TABLE,
	LIST_GAIN_TABLE,
	LIST_TOTAL_NUM,
} LIST_ID;

/// 32 bytes header
typedef struct ambarella_adj_aeb_bin_header_s {
	u32	magic_number;
	u16	header_ver_major;
	u16	header_ver_minor;
	u32	header_size;
	u32	payload_size;
	u32	sensor_id;
	u16	reserved[6];
} ambarella_adj_aeb_bin_header_t;

///////count data size
typedef struct ambarella_data_struct_header_s {
	u32	struct_size;
	u16	struct_total_num;
	u16	struct_align;
} ambarella_data_struct_header_t;

typedef struct ambarella_data_header_s {
	u8	file_name[FILE_NAME_LENGTH];
	ambarella_data_struct_header_t	struct_type[LIST_TOTAL_NUM];
} ambarella_data_header_t;


typedef struct ambarella_adj_aeb_bin_parse_s {
	ambarella_adj_aeb_bin_header_t	bin_header;
	ambarella_data_header_t	data_header;
} ambarella_adj_aeb_bin_parse_t;

typedef struct aaa_files_s {
	char	adj_file[FILE_NAME_LENGTH];
	char	aeb_file[FILE_NAME_LENGTH];
	char	lens_file[FILE_NAME_LENGTH];
} aaa_files_t;

ambarella_adj_aeb_bin_header_t G_adj_bin_header;

#endif

