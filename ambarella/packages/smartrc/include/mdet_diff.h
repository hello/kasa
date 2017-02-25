/*******************************************************************************
 * mdet_diff.h
 *
 * History:
 *   2016/5/16 - ypxu created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/
#ifndef _MDET_DIFF_H_
#define _MDET_DIFF_H_

#include "basetypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#define	MDET_MAX_ROIS			(4)
#define MDET_MAX_POINTS_PER_ROI	(8)

typedef enum {
	MDET_REGION_POLYGON = 0,
	MDET_REGION_LINE,
} mdet_region_t;

typedef struct {
	u32 pitch;
	u32 width;
	u32 height;
} mdet_dimension_t;

typedef struct {
	int x;
	int y;
} mdet_point_t;

typedef struct {
	mdet_region_t type;
	u32 num_points;
	mdet_point_t points[MDET_MAX_POINTS_PER_ROI];
} mdet_roi_t;

typedef struct {
	u32 num_roi;
	mdet_roi_t roi[MDET_MAX_ROIS];
} mdet_roi_info_t;

typedef struct {
	int mdet_inited;
	/* Output Result to App */
	float motion[MDET_MAX_ROIS];
	u8 *fg;
	/* Internal Use for Library Only */
	int fn;
	int len;
	void *data;
	u32 fg_pxls[MDET_MAX_ROIS];
	u32 pixels[MDET_MAX_ROIS];
} mdet_session_t;

typedef struct {
	mdet_dimension_t fm_dim;
	mdet_roi_info_t roi_info;
	int threshold;
} mdet_cfg;

typedef struct {
	int num_rois;
	int is_roi[MDET_MAX_ROIS];
	u8 bg;
} mdet_per_pixel_diff_t;

#endif /* _MDET_DIFF_H_ */
