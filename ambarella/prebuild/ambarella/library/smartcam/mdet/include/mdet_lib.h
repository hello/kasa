/*******************************************************************************
 *  mdet_lib.h
 *
 * History:
 *    2013/12/18 - [Zhenwu Xue] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 ******************************************************************************/

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
	void			*data;
	u32			fg_pxls[MDET_MAX_ROIS];
	u32			pixels[MDET_MAX_ROIS];
} mdet_session_t;

typedef struct {
	mdet_dimension_t	fm_dim;
	mdet_roi_info_t		roi_info;
	int			threshold;
} mdet_cfg;

typedef enum {
	MDET_ALGO_DIFF		= 0,
	MDET_ALGO_MEDIAN	= 1,
	MDET_ALGO_MOG2		= 2,
	MDET_ALGO_DIFF_RGB	= 3,
	MDET_ALGO_AWS		= 4,
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

