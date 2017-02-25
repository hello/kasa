/*
 * cawarp.c
 *
 * History:
 *  2015/04/02 - [Zhaoyang Chen] created file
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basetypes.h>
#include <math.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

#define STITCH_PADDING             128

static u8* G_cawarp_mem_addr;
static int G_cawarp_mem_size;
static float* G_cawarp_data;
static int G_cawarp_data_num;
static int G_cawarp_center_x, G_cawarp_center_y;

static int G_cawarp_inited;

const static int HWARP_HOR_GRID_SPACING[] = { 0, 1, 2, 3, 4, 5 };
const static int HWARP_VER_GRID_SPACING[] = { 0, 1, 2, 3, 4, 5 };
const static int VWARP_HOR_GRID_SPACING[] = { 1, 2, 3, 4, 5 };
const static int VWARP_VER_GRID_SPACING[] = { 0, 1, 2, 3, 4, 5 };

static int is_stitch_output(int* output_width)
{
	int is_stitch;

	if (ioctl_get_system_resource() < 0) {
		return -1;
	}

	*output_width = MIN(vproc.dptz[0]->input.width, 1920);
	is_stitch = 0;

	DEBUG("Encode mode [%d], %s, DPTZ input width %d, CA output width %d\n",
	    vproc.resource.encode_mode, is_stitch ? "Stitching" : "Non-stitching",
	    vproc.dptz[0]->input.width, *output_width);

	return is_stitch;
}

static int get_spacing(const int expo)
{
	return (1 << (expo + 4));
}

static int find_grid_spacing(int* grid_expo, const int len,
    const int max_grid_num, const int* spacing_array, const int spacing_num)
{
	int i, min_grid_spacing, spacing;

	min_grid_spacing = len / (max_grid_num - 1);
	min_grid_spacing += ((len % (max_grid_num - 1) ? 1 : 0));
	i = 0;
	spacing = 0;
	do {
		spacing = get_spacing(spacing_array[i]);

		if (spacing >= min_grid_spacing) {
			break;
		}
	} while (++i < spacing_num);
	if (i >= spacing_num) {
		ERROR("grid is too large.\n"); // rarely
		return -1;
	}
	*grid_expo = spacing_array[i];
	return 0;
}

static int set_hor_grid(const int output_width, const int output_height,
    const int is_stitch)
{
	int spacing, grid_expo, spacing_num;
	int nonstitch_width = (
	    is_stitch ? (output_width / 2 + STITCH_PADDING) : output_width);

	spacing_num = sizeof(HWARP_HOR_GRID_SPACING)
	    / sizeof(HWARP_HOR_GRID_SPACING[0]);
	if (find_grid_spacing(&grid_expo, nonstitch_width, CA_TABLE_COL_NUM,
	    HWARP_HOR_GRID_SPACING, spacing_num) < 0) {
		ERROR("Failed to set hor grid spacing to hor map for %d\n",
		    nonstitch_width);
		return -1;
	}
	spacing = get_spacing(grid_expo);
	vproc.cawarp->hor_map.h_spacing = grid_expo;
	vproc.cawarp->hor_map.output_grid_col = ROUND_UP(output_width, spacing)
	    / spacing + 1;

	spacing_num = sizeof(HWARP_VER_GRID_SPACING)
	    / sizeof(HWARP_VER_GRID_SPACING[0]);
	if (find_grid_spacing(&grid_expo, output_height, CA_TABLE_ROW_NUM,
		HWARP_VER_GRID_SPACING, spacing_num) < 0) {
		ERROR("Failed to set ver grid spacing to hor map for %d\n",
		    output_height);
		return -1;
	}
	spacing = get_spacing(grid_expo);
	vproc.cawarp->hor_map.v_spacing = grid_expo;
	vproc.cawarp->hor_map.output_grid_row = ROUND_UP(output_height, spacing)
	    / spacing + 1;

	DEBUG("CA warp: hor map %dx%d, grid %dx%d\n",
	    vproc.cawarp->hor_map.output_grid_col,
	    vproc.cawarp->hor_map.output_grid_row,
	    vproc.cawarp->hor_map.h_spacing,
	    vproc.cawarp->hor_map.v_spacing);
	return 0;
}

static int set_ver_grid(const int output_width, const int output_height,
    const int is_stitch)
{
	int spacing, grid_expo, spacing_num;
	int nonstitch_width = (
	    is_stitch ? (output_width / 2 + STITCH_PADDING) : output_width);

	spacing_num = sizeof(VWARP_HOR_GRID_SPACING)
	    / sizeof(VWARP_HOR_GRID_SPACING[0]);
	if (find_grid_spacing(&grid_expo, nonstitch_width, CA_TABLE_COL_NUM,
		VWARP_HOR_GRID_SPACING, spacing_num) < 0) {
		ERROR("Failed to set hor grid spacing to ver map for %d\n",
		    nonstitch_width);
		return -1;
	}
	spacing = get_spacing(grid_expo);
	vproc.cawarp->ver_map.h_spacing = grid_expo;
	vproc.cawarp->ver_map.output_grid_col = ROUND_UP(output_width, spacing)
	    / spacing + 1;

	spacing_num = sizeof(VWARP_VER_GRID_SPACING)
	    / sizeof(VWARP_VER_GRID_SPACING[0]);
	if (find_grid_spacing(&grid_expo, output_height, CA_TABLE_ROW_NUM,
		VWARP_VER_GRID_SPACING, spacing_num) < 0) {
		ERROR("Failed to set ver grid spacing to ver map for %d\n",
		    output_height);
		return -1;
	}
	spacing = get_spacing(grid_expo);
	vproc.cawarp->ver_map.v_spacing = grid_expo;
	vproc.cawarp->ver_map.output_grid_row = ROUND_UP(output_height, spacing)
	    / spacing + 1;
	DEBUG("CA warp: ver map %dx%d, grid %dx%d\n",
	    vproc.cawarp->ver_map.output_grid_col,
	    vproc.cawarp->ver_map.output_grid_row,
	    vproc.cawarp->ver_map.h_spacing,
	    vproc.cawarp->ver_map.v_spacing);
	return 0;
}

static inline float get_x_in_vin(const float input_x, const float input_width,
    const float output_width, const float output_x)
{
	return input_x + output_x * input_width / output_width;
}

static inline float get_y_in_vin(const float input_y, const float output_y)
{
	return input_y + output_y;
}

static inline int cmp_float(const float x,const  float y)
{
#define EPSLON (1e-6)
	float large, small;
	large = y + EPSLON;
	small = y - EPSLON;
	if (x > large)
		return 1;
	else if (x < small)
		return -1;
	else return 0;
}

static inline int round_int(const float a)
{
	if (cmp_float(a, (int) a) < 0) {
		return ((int) (a - 1));
	}
	if (cmp_float(a, (int) a) > 0) {
		return ((int) (a + 1));
	}
	if (cmp_float(a, (int) a) == 0) {
		return ((int) a);
	}

	return -1;
}

inline float interpolate(const float index, const float start_index,
    const float end_index, const float start_value, const float end_value)
{
	return
	    (end_index == start_index) ? start_value :
	        ((index - start_index) * (end_value - start_value)
	            / (end_index - start_index) + start_value);
}

static inline float lookup_cawarp_map(float radius)
{
	int start_index, end_index;
	float result = 0;
	float* last;
	if (cmp_float(radius, G_cawarp_data_num - 1) <= 0) {
		start_index = (int) radius;
		end_index = round_int(radius);
		result = interpolate(radius, start_index, end_index,
		    G_cawarp_data[start_index], G_cawarp_data[end_index]);
	} else {
		last = &G_cawarp_data[G_cawarp_data_num - 1];
		result = *last
		    + (*last - *(last - 1)) * (radius - G_cawarp_data_num + 1);
	}

	return result;
}

static inline float get_hor_input(float x, float y)
{
	float r;
	x -= G_cawarp_center_x;
	y -= G_cawarp_center_y;
	r = sqrtf(x * x + y * y);

	return (cmp_float(r, 0) ? lookup_cawarp_map(r) * x / r : 0.0f)
	    + (float) G_cawarp_center_x;
}

static inline float get_ver_input(float x, float y)
{
	float r;
	x -= G_cawarp_center_x;
	y -= G_cawarp_center_y;
	r = sqrtf(x * x + y * y);
	return (cmp_float(r, 0) ? lookup_cawarp_map(r) * y / r : 0.0f)
	    + (float) G_cawarp_center_y;
}

static void fill_hor_grid(const struct iav_rect* dptz, const int output_width)
{
	int i, j;
	float out_x, out_y, in_x;
	struct iav_warp_map* map = &vproc.cawarp->hor_map;
	s16* addr;
	int grid_width = get_spacing(map->h_spacing);
	int grid_height = get_spacing(map->v_spacing);
	int pitch = map->output_grid_col;
	map->enable = 1;

	memset((void *) map->data_addr_offset, 0, G_cawarp_mem_size / 2);

	for (i = 0; i < map->output_grid_row; i++) {
		out_y = get_y_in_vin(dptz->y, grid_height * i);
		addr = (s16 *) map->data_addr_offset + i * pitch;
		for (j = 0; j < map->output_grid_col; j++) {
			out_x = get_x_in_vin(dptz->x, dptz->width, output_width,
			    grid_width * j);
			in_x = get_hor_input(out_x, out_y);
			addr[j] = (s16) (16 * (out_x - in_x));
//			fprintf(stderr, "%f\t", out_x - in_x);
		}
//		fprintf(stderr, "\n");
	}
//	fprintf(stderr, "\n\n\n");
}

static void fill_ver_grid(const struct iav_rect* dptz, const int output_width)
{
	int i, j;
	float out_x, out_y, in_y;
	struct iav_warp_map* map = &vproc.cawarp->ver_map;
	s16* addr;
	int grid_width = get_spacing(map->h_spacing);
	int grid_height = get_spacing(map->v_spacing);
	int pitch = map->output_grid_col;
	map->enable = 1;
	memset((void *) map->data_addr_offset, 0, G_cawarp_mem_size / 2);

	for (i = 0; i < map->output_grid_row; i++) {
		out_y = get_y_in_vin(dptz->y, grid_height * i);
		addr = (s16*) map->data_addr_offset + i * pitch;
		for (j = 0; j < map->output_grid_col; j++) {
			out_x = get_x_in_vin(dptz->x, dptz->width, output_width,
			    grid_width * j);
			in_y = get_ver_input(out_x, out_y);
			addr[j] = (s16) (16 * (out_y - in_y));
//			fprintf(stderr, "%f\t", out_y - in_y);
		}
//		fprintf(stderr, "\n");
	}
}

static int check_cawarp(const cawarp_param_t* param)
{
	if (param->data_num < 0 || param->data_num == 1) {
		ERROR("CA data num [%d] must be greater than 1.\n", param->data_num);
		return -1;
	}
	if (param->data_num) {
		if (!param->data) {
			ERROR("Invalid CA data addr.\n");
			return -1;
		}
	}
	return 0;
}

int init_cawarp(void)
{
	if (unlikely(!G_cawarp_inited)) {
		G_cawarp_mem_size = sizeof(s16) * CA_TABLE_MAX_SIZE * 2 * 2; // 2 areas for stitching and 2 tables per area
		G_cawarp_mem_addr = malloc(G_cawarp_mem_size);
		if (G_cawarp_mem_addr) {
			memset(G_cawarp_mem_addr, 0, G_cawarp_mem_size);
			vproc.cawarp->hor_map.data_addr_offset = (u32) G_cawarp_mem_addr;
			vproc.cawarp->ver_map.data_addr_offset = (u32) G_cawarp_mem_addr
			    + G_cawarp_mem_size / 2;
			G_cawarp_inited = 1;
			DEBUG("init cawarp\n");
		} else {
			ERROR("Failed to malloc memory for CA table.\n");
		}
	}
	return G_cawarp_inited ? 0 : -1;

}

int deinit_cawarp(void)
{
	if (G_cawarp_inited) {
		free(G_cawarp_mem_addr);
		G_cawarp_mem_addr = NULL;
		G_cawarp_mem_size = 0;
		G_cawarp_data = NULL;
		G_cawarp_data_num = 0;
		G_cawarp_inited = 0;
		DEBUG("done\n");
	}
	return G_cawarp_inited ? -1 : 0;
}

int operate_cawarp(cawarp_param_t* param)
{
	int output_width, is_stitch;
	struct iav_rect* main_input = &vproc.dptz[0]->input;

	if (init_dptz() < 0 || init_cawarp() < 0)
		return -1;

	if (param) {
		if (check_cawarp(param) < 0)
			return -1;
		G_cawarp_data = param->data;
		G_cawarp_data_num = param->data_num;
		vproc.cawarp->red_scale_factor = (s16) (param->red_factor * 256);
		vproc.cawarp->blue_scale_factor = (s16) (param->blue_factor * 256);
		G_cawarp_center_x = vproc.vin.info.width / 2 + param->center_offset_x;
		G_cawarp_center_y = vproc.vin.info.height / 2 + param->center_offset_y;
		DEBUG("ca warp center (%d, %d)\n", G_cawarp_center_x,
		    G_cawarp_center_y);
	}

	if (G_cawarp_data_num) {
		if ((is_stitch = is_stitch_output(&output_width)) < 0) {
			return -1;
		}

		if (set_hor_grid(output_width, main_input->height, is_stitch) < 0) {
			return -1;
		}

		if (set_ver_grid(output_width, main_input->height, is_stitch) < 0) {
			return -1;
		}

		fill_hor_grid(main_input, output_width);
		fill_ver_grid(main_input, output_width);

		return ioctl_cfg_cawarp();
	}

	return disable_cawarp();
}


int disable_cawarp(void)
{
	vproc.cawarp->blue_scale_factor = 0;
	vproc.cawarp->red_scale_factor = 0;
	vproc.cawarp->hor_map.enable = 0;
	vproc.cawarp->ver_map.enable = 0;
	return ioctl_cfg_cawarp();
}
