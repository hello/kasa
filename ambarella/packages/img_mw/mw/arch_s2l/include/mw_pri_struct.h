/*
 *
 * mw_pri_struct.h
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

#ifndef _MW_PRI_STRUCT_H_
#define _MW_PRI_STRUCT_H_

#include "iav_common.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "mw_defines.h"


typedef enum {
	MW_IDLE				= 0,
	MW_PREVIEW,
	MW_ENCODING,
	MW_STILL_CAPTURE,
	MW_DECODING,
} MW_STATE;

typedef enum {
	OPERATE_NONE = 0,
	OPERATE_LOAD = 1,
	OPERATE_SAVE = 2,
} OPERATE_METHOD;

typedef struct {
	u32				lens_type;
	u16				af_mode;
	u16				af_tile_mode;
	u16				zm_idx;
	u16				fs_idx;
	u16				fs_near;
	u16				fs_far;
} _mw_af_info;

typedef struct {
	int line_num;
	u16 belt_def;
	u16 belt_cur;
} line_info;

typedef struct {
	line_info ae_belt;
} _mw_aaa_cur_status;

//init
typedef struct {
	u32 mw_enable : 1;
	u32 nl_enable : 1;
	u32 reload_flag : 1;
	u32 is_rgb_sensor : 1;
	u32 aaa_active : 1;
	u32 slow_shutter_active : 1;
	u32 load_cfg_file : 1;
	u32 wait_3a : 1;
	u32 operate_adj : 2;
	u32 reserved : 22;
	int mode;
} _mw_init_param;

//ADJ
typedef struct {
	int sharpen_strength;
	int mctf_strength;
	int dn_mode;
	int chroma_noise_str;
	int bl_compensation;
	int local_expo_mode;
	int auto_contrast;
	int auto_wdr_str;
	mw_local_exposure_curve le_off_curve;
	int auto_knee_strength;
	int auto_dump_cfg;
} _mw_enhance_param;

//iris
enum iris_type_enum {
	DC_IRIS = 0,
	P_IRIS = 1,
};

typedef struct {
	int iris_enable;
	int iris_type;
	int aperture_min;
	int aperture_max;
} _mw_iris_param;

//AWB
typedef struct {
	int enable;
	int wb_mode;
	int wb_method;
} _mw_awb_param;

typedef struct {
	u16 main_width;
	u16 main_height;
	u16 max_cr : 2;
	u16 max_wcr : 2;
	u16 wcr_enable : 1;
	u16 reserved1 : 11;
	u16 reserved2;
} _mw_pri_iav;

typedef struct {
	u16 raw_width;
	u16 raw_height;
	int raw_resolution;
	int raw_bayer;
	u16 vin_width;
	u16 vin_height;
	char name[32];
	u32 default_fps;
	u32 current_fps;
	u32 vin_mode;
	u32 sensor_op_mode;
	u32 offset_num[LIST_TOTAL_NUM];
	struct vindev_aaa_info vin_aaa_info;
	aaa_files_t load_files;
	u32 lens_id;
	u32 sensor_slow_shutter : 1;
	u32 is_rgb_sensor : 1;
	u32 reserved : 30;
} _mw_sensor_param;

typedef struct {
	int fd;
	mw_sys_res res;
	mw_ae_param ae_params;
	mw_image_param image_params;
	mw_cali_file cali_file;
	mw_af_param af_info;
	_mw_init_param init_params;
	_mw_sensor_param sensor;
	_mw_awb_param awb_params;
	_mw_enhance_param enh_params;
	_mw_iris_param iris_params;
	_mw_aaa_cur_status cur_status;
	_mw_pri_iav mw_iav;
	pthread_mutex_t slow_shutter_lock;
	mw_adj_file_param adj;
} _mw_global_config;

#endif //  _MW_PRI_STRUCT_H_

