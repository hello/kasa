/*******************************************************************************
 * lib_eis.h
 *
 * History:
 *  Nov 6, 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef LIB_EIS_H_
#define LIB_EIS_H_

#include <basetypes.h>
#include <amba_eis.h>
#include "iav_ioctl.h"

#define EIS_GYRO_STAT_MAX_NUM    (GYRO_DATA_ENTRY_MAX_NUM)
#define EIS_WARP_ROW_MAX_NUM     (48)
#define EIS_WARP_COL_MAX_NUM     (32)
#define EIS_WARP_COL_STITCH_MAX_NUM     (EIS_WARP_COL_MAX_NUM * 2)

typedef double radian_t;
typedef double degree_t;

typedef enum {
	AXIS_Z = 0,
	AXIS_Y,
	AXIS_X,
	AXIS_TOTAL_NUM,
} AXIS;

typedef enum {
	EIS_DISABLE = 0,
	EIS_PITCH,
	EIS_ROTATE,
} EIS_FLAG;

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

typedef enum {
	EIS_AVG_MA = 0, // Moving Average
	EIS_AVG_ABSOLUTE = 1,
	EIS_AVG_NUM,
	EIS_AVG_FIRST = EIS_AVG_MA,
} EIS_AVG_MODE;

typedef int (*eis_apply_warp_func_t)(const struct iav_warp_main*);
typedef int (*eis_get_stat_func_t)(amba_eis_stat_t*);

typedef struct eis_setup_s {
	// gyro
	double accel_full_scale_range;
	double accel_lsb;
	double gyro_full_scale_range;
	double gyro_lsb;
	int gyro_sample_rate_in_hz;   // hz
	gyro_data_t gyro_shift;
	AXIS gravity_axis;

	// vin
	int vin_width;
	int vin_height;
	int vin_col_bin;
	int vin_row_bin;
	double vin_cell_width_in_um;          // um
	double vin_cell_height_in_um;         // um
	double vin_frame_rate_in_hz;          // hz
	double vin_vblank_in_ms;              // ms

	// premain
	int premain_input_width;
	int premain_input_height;
	int premain_input_offset_x;
	int premain_input_offset_y;

	int premain_width;
	int premain_height;

	// main
	int output_width;
	int output_height;

	// lens
	int lens_focal_length_in_um;         // um

	double threshhold;
	int frame_buffer_num;

	// Warp table address info
	s16 *h_table_addr;
	s16 *v_table_addr;
	s16 *me1_v_table_addr;

	// Average Mode
	EIS_AVG_MODE avg_mode;
} eis_setup_t;

int eis_setup(const eis_setup_t* setup, eis_apply_warp_func_t apply_warp_func, eis_get_stat_func_t get_stat_func);
int eis_open(void);
int eis_close(void);
int eis_enable(const EIS_FLAG flag);

int eis_version(version_t* version);

#endif /* LIB_EIS_H_ */
