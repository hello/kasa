/*
 *
 * History:
 *    2013/12/18 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __MDET_H__
#define __MDET_H__

#include "basetypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#define	MDET_MAX_ROIS		4
#define MDET_MAX_POINTS_PER_ROI	8

typedef enum {
	MDET_REGION_POLYGON	= 0,
	MDET_REGION_LINE,
} mdet_region_t;

typedef struct {
	u32			pitch;
	u32			width;
	u32			height;
} mdet_dimension_t;

typedef struct {
	int			x;
	int			y;
} mdet_point_t;

typedef struct {
	mdet_region_t		type;
	u32			num_points;
	mdet_point_t		points[MDET_MAX_POINTS_PER_ROI];
} mdet_roi_t;

typedef struct {
	u32			num_roi;
	mdet_roi_t		roi[MDET_MAX_ROIS];
} mdet_roi_info_t;

typedef struct {
	/* Output Result to App */
	float			motion[MDET_MAX_ROIS];
	int			*fg;
	/* Internal Use for Library Only */
	int			fn;
	int			len;
	void		*data;
	u32			fg_pxls[MDET_MAX_ROIS];
	u32			pixels[MDET_MAX_ROIS];
} mdet_session_t;

typedef struct {
	mdet_dimension_t	fm_dim;
	mdet_roi_info_t		roi_info;
	int	threshold;
} mdet_cfg;

typedef enum {
	MDET_ALGO_DIFF = 0,
	MDET_ALGO_MEDIAN = 1,
	MDET_ALGO_MOG2 = 2,
	MDET_ALGO_NUM
} mdet_algo_t;

typedef struct {
	mdet_algo_t algo_type;
	int (*md_start)(mdet_session_t*);
	int (*md_update_frame)(mdet_session_t*, const u8*, u32);
	int (*md_stop)(mdet_session_t*);
	int (*md_set_config)(mdet_cfg*);
	int (*md_get_config)(mdet_cfg*);
} mdet_instance;

typedef struct {
	u16	major;
	u8	minor;
	u8	patch;
	u16	year;
	u8	month;
	u8	day;
} mdet_version;

#ifdef __cplusplus
extern "C" {
#endif

mdet_instance *mdet_create_instance(mdet_algo_t algo_type);
int mdet_destroy_instance(mdet_instance *instance);
int mdet_is_roi(mdet_point_t pt, mdet_roi_t *r);

void mdet_get_version(mdet_version *version);

#ifdef __cplusplus
}
#endif

#endif

