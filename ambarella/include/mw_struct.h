/*
 * mw_struct.h
 *
 * History:
 *	2013/03/12 - [Cao Rongrong] Created file
 *	2013/12/12 - [Jian Tang] Modified file
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

#ifndef __MW_STRUCT_H__
#define __MW_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "basetypes.h"

typedef u32			mw_id;
typedef u32			mw_stream;

#define	MW_MAX_STREAM_NUM		(4)
#define	MW_MAX_BUFFER_NUM		(4)
#define	MW_NUM_EXPOSURE_CURVE	(256)
#define	MW_MAX_ROI_NUM			(4)
#define	MW_MAX_MD_THRESHOLD		(200)

#define	MW_TRANSPARENT			(0)
#define	MW_NONTRANSPARENT		(255)
#define	MW_OSD_AREA_NUM			(3)
#define	MW_OSD_CLUT_NUM			(MW_MAX_STREAM_NUM * MW_OSD_AREA_NUM)
#define	MW_OSD_CLUT_SIZE			(1024)
#define	MW_OSD_CLUT_OFFSET		(0)
#define	MW_OSD_YUV_OFFSET		(MW_OSD_CLUT_NUM * MW_OSD_CLUT_SIZE)
#define	MW_ADJ_FILTER_NUM		64

#define SHUTTER_INVALID				0
#define SHUTTER_1_SEC					512000000		//(512000000 / 1)
#define SHUTTER_1BY2_SEC				256000000		//(512000000 / 2)
#define SHUTTER_1BY3_SEC				170666667		//(512000000 / 3)
#define SHUTTER_1BY3P125_SEC			163840000		//(512000000 / 25 * 8)
#define SHUTTER_1BY4_SEC				128000000		//(512000000 / 4)
#define SHUTTER_1BY5_SEC				102400000		//(512000000 / 5)
#define SHUTTER_1BY6_SEC				85333333		//(512000000 / 6)
#define SHUTTER_1BY6P25_SEC				81920000		//(512000000 / 25 * 4)
#define SHUTTER_1BY7P5_SEC				68266667		//(512000000 / 15 * 2)
#define SHUTTER_1BY10_SEC				51200000		//(512000000 / 10)
#define SHUTTER_1BY12P5_SEC				40960000		//(512000000 / 25 * 2)
#define SHUTTER_1BY15_SEC				34133333		//(512000000 / 15)
#define SHUTTER_1BY20_SEC				25600000		//(512000000 / 20)
#define SHUTTER_1BY24_SEC				21333333		//(512000000 / 24)
#define SHUTTER_1BY25_SEC				20480000		//(512000000 / 25)
#define SHUTTER_1BY30_SEC				17066667		//(512000000 / 30)
#define SHUTTER_1BY33_SEC				15360000		//(512000000 / 33.3)
#define SHUTTER_1BY40_SEC				12800000		//(512000000 / 40)
#define SHUTTER_1BY50_SEC				10240000		//(512000000 / 50)
#define SHUTTER_1BY60_SEC				8533333		//(512000000 / 60)
#define SHUTTER_1BY100_SEC				5120000		//(512000000 / 100)
#define SHUTTER_1BY120_SEC				4266667		//(512000000 / 120)
#define SHUTTER_1BY240_SEC				2133333		//(512000000 / 240)
#define SHUTTER_1BY480_SEC				1066667		//(512000000 / 480)
#define SHUTTER_1BY960_SEC				533333		//(512000000 / 960)
#define SHUTTER_1BY1024_SEC				500000		//(512000000 / 1024)
#define SHUTTER_1BY8000_SEC				64000		//(512000000 / 8000)
#define SHUTTER_1BY16000_SEC				32000		//(512000000 / 16000)
#define SHUTTER_1BY32000_SEC				16000		//(512000000 /32000)

#define LENS_FNO_UNIT		65536

#define		MW_ERROR_LEVEL      	0
#define 	MW_MSG_LEVEL          	1
#define 	MW_INFO_LEVEL         	2
#define 	MW_DEBUG_LEVEL      	3
#define 	MW_LOG_LEVEL_NUM  		4
typedef int mw_log_level;

typedef enum {
	MW_LE_STOP = 0,
	MW_LE_AUTO,
	MW_LE_2X,
	MW_LE_3X,
	MW_LE_4X,
	MW_LE_TOTAL_NUM,
} mw_local_exposure_mode;


typedef enum {
	MW_ANTI_FLICKER_50HZ			= 0,
	MW_ANTI_FLICKER_60HZ			= 1,
} mw_anti_flicker_mode;


typedef enum {
	MW_WB_NORMAL_METHOD = 0,
	MW_WB_CUSTOM_METHOD,
	MW_WB_GREY_WORLD_METHOD,
	MW_WB_METHOD_NUMBER,
} mw_white_balance_method;


typedef enum {
	MW_WB_AUTO 					= 0,
	MW_WB_INCANDESCENT,			// 2800K
	MW_WB_D4000,
	MW_WB_D5000,
	MW_WB_SUNNY,					// 6500K
	MW_WB_CLOUDY,				// 7500K
	MW_WB_FLASH,
	MW_WB_FLUORESCENT,
	MW_WB_FLUORESCENT_H,
	MW_WB_UNDERWATER,
	MW_WB_CUSTOM,				// custom
	MW_WB_MODE_NUMBER,
} mw_white_balance_mode;

typedef struct {
	int				arch;
	int				model;
	int				major;
	int				minor;
	int				patch;
	char				description[64];
} mw_driver_info;


typedef struct {
	u32 				major;
	u32 				minor;
	u32 				patch;
	u32				update_time;
} mw_version_info;

typedef struct  {
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
} mw_aaa_lib_version;

typedef struct {
	int					p_coef;
	int					i_coef;
	int					d_coef;
} mw_dc_iris_pid_coef;

typedef struct {
	int					open_threshold; //about 500 in 3lux ambient light
	int					close_threshold; //about 510 in 10lux ambient light
	int					open_delay;
	int					close_delay;
	int					threshold_1X; //about 490, depends on customization
	int					threshold_2X; //about 480
	int					threshold_3X; //about 470
	int					over_exp_threshold; //about 1000, depends on customization
	int					not_over_exp_threshold; //about 800
} mw_ir_led_control_param;

typedef enum {
	MW_IR_LED_MODE_OFF = 0,
	MW_IR_LED_MODE_ON = 1,
	MW_IR_LED_MODE_AUTO = 2
} mw_ir_led_mode;

typedef enum {
	MW_SHUTTER = 0,
	MW_DGAIN,
	MW_IRIS,
	MW_TOTAL_DIMENSION
} mw_ae_point_dimension;


typedef enum {
	MW_AE_START_POINT = 0,
	MW_AE_END_POINT,
} mw_ae_point_position;


typedef struct {
	s32			factor[3];	// 0:shutter, 1:gain, 2:iris
	u32			pos;		// 0:start, 1:end
} mw_ae_point;


typedef struct {
	mw_ae_point	start;
	mw_ae_point	end;
} mw_ae_line;


typedef enum {
	MW_AE_SPOT_METERING = 0,
	MW_AE_CENTER_METERING,
	MW_AE_AVERAGE_METERING,
	MW_AE_CUSTOM_METERING,
	MW_AE_TILE_METERING,
	MW_AE_Y_ORDER_METERING,
	MW_AE_EXTERN_DESIGN_METERING,
	MW_AE_METERING_TYPE_NUMBER,
} mw_ae_metering_mode;


typedef struct {
	int				metering_weight[96];
} mw_ae_metering_table;


typedef struct {
	s32				saturation;
	s32				brightness;
	s32				hue;
	s32				contrast;
	s32				sharpness;
} mw_image_param;


typedef struct {
	char				lect_filename[64];		// local exposure curve table filename
	u32				lect_reload;
	char				gct_filename[64];		// gamma curve table filename
	u32				gct_reload;
} mw_iq_param;

#define	MW_WB_MODE_HOLD			(100)

typedef struct {
	u32				wb_mode;
} mw_awb_param;


typedef struct {
	u32				lens_type;
	u16				af_mode;
	u16				af_tile_mode;
	u16				zm_dist;
	u16				fs_dist;
	u16				fs_near;
	u16				fs_far;
} mw_af_param;


typedef struct {
	u32			agc;
	u32			shutter;
} mw_image_stat_info;


typedef struct {
	u16			rgb_luma;
	u16			cfa_luma;
} mw_luma_value;


typedef struct {
	u32			r_gain;
	u32			g_gain;
	u32			b_gain;
	u16			d_gain;
	u16			reserved;
} mw_wb_gain;


typedef struct {
	u16			gain_curve_table[MW_NUM_EXPOSURE_CURVE];
} mw_local_exposure_curve;

typedef enum {
	NO_MOTION = 0,
	IN_MOTION,
} mw_motion_status_type;


typedef enum {
	EVENT_NO_MOTION = 0,
	EVENT_MOTION_START,
	EVENT_MOTION_END,
} mw_motion_event_type;


typedef int (*alarm_handle_func)(const int *p_motion_event);


typedef struct {
	u16	x;
	u16	y;
	u16	width;
	u16	height;
	u16	threshold;
	u16	sensitivity;
	u16	valid;
} mw_roi_info;


typedef struct {
	char bad_pixel[64];		// for bad pixel calibration data
	char wb[64];			// for white balance calibration data
	char shading[64];		// for lens shading calibration data
} mw_cali_file;


typedef struct {
	u8 hdr_mode;
	u8 hdr_expo_num;
	u8 hdr_shutter_mode;
	u8 isp_pipeline;
	int raw_pitch;
} mw_sys_res;

typedef struct {
	u16 expo_ratio; 	// one param for all frames. 4 ~ 128
	u16 boost_factor;	// one param for all frames. 0 ~ 256, 0 means no boost
} mw_hdr_blend_info;

typedef struct {
	u8 radius;
	u8 luma_weight_red;
	u8 luma_weight_green;
	u8 luma_weight_blue;
	u8 luma_weight_shift;
}mw_wdr_luma_info;

typedef struct {
	u32 step;
	u32 line_time;
} mw_sensor;

typedef struct {
	mw_sys_res res;
	mw_sensor sensor;
} mw_sys_info;

typedef struct {
	u32		FNO_min;
	u32		FNO_max;
	u32		aperture_min;
	u32		aperture_max;
} mw_aperture_param;

typedef struct {
	mw_anti_flicker_mode	anti_flicker_mode;
	u32					shutter_time_min;
	u32					shutter_time_max;
	u32					sensor_gain_max;
	u32					slow_shutter_enable;
	u32					ir_led_mode;
	u32					current_vin_fps;
	int					ae_level[4];
	mw_aperture_param	lens_aperture;
	mw_ae_metering_mode	ae_metering_mode;
	mw_ae_metering_table	ae_metering_table;
	u32					tone_curve_duration;
} mw_ae_param;

#define	FILE_NAME_LENGTH		64

typedef enum {
	FILE_TYPE_ADJ = 0,
	FILE_TYPE_AEB = 1,
	FILE_TYPE_CONFIG = 2,
	FILE_TYPE_PIRIS = 3,
	FILE_TYPE_TOTAL_NUM,
	FILE_TYPE_FIRST = FILE_TYPE_ADJ,
	FILE_TYPE_LAST = FILE_TYPE_TOTAL_NUM,
} MW_AAA_FILE_TYPE;

typedef enum {
	MW_AAA_SUCCESS = 0,
	MW_AAA_INIT_FAIL = -1,
	MW_AAA_DEINIT_FAIL = -2,
	MW_AAA_NL_INIT_FAIL = -3,
	MW_AAA_INTERRUPTED = -4,
	MW_AAA_SET_FAIL = -5,
	MW_AAA_GET_FAIL = -6,
} MW_AAA_RESULT;


/*******************************************************
 *
 *	Parse Configuration Parameters
 *
 *******************************************************/
typedef enum {
	MAP_TO_U32 = 0,
	MAP_TO_U16,
	MAP_TO_U8,
	MAP_TO_S32,
	MAP_TO_S16,
	MAP_TO_S8,
	MAP_TO_DOUBLE,
	MAP_TO_STRING,
} mapping_data_type;


typedef enum {
	NO_LIMIT = 0,
	MIN_MAX_LIMIT,
	MIN_LIMIT,
	MAX_LIMIT,
} mapping_data_constraint;


typedef struct {
	char * 					TokenName;		// name
	void * 					Address;		// address
	mapping_data_type		Type;			// type: 0 - u32, 1 - u16, 2 - u8, 3 - int(s32), 4 - double, 5 - char[]>
	double					Default;			// default value>
	mapping_data_constraint	ParamLimits;	// 0 - no limits, 1 - min and max limits, 2 - only min limit, 3 - only max limit
	double					MinLimit;		// Minimun value
	double					MaxLimit;		// Maximun value
	int						StringLengthLimit;	// Dimension of type char[]
} Mapping;

typedef struct {
	u32 apply : 1;		//input params
	u32 done : 1;		//output params
	u32 reserved : 30;
} mw_operate;

typedef struct {
	char filename[FILE_NAME_LENGTH];
	mw_operate flag[MW_ADJ_FILTER_NUM];
} mw_adj_file_param;

#ifdef __cplusplus
}
#endif

#endif //  __MW_STRUCT_H__

