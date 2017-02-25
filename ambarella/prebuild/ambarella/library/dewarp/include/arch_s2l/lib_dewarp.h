/*******************************************************************************
 *  lib_dewarp.h
 *
 * History:
 * 	2012/08/22 - [Qian Shen] created file
 * 	2013/03/01 - [Qian Shen] Redesign APIs
 * 	2013/05/03 - [Jian Tang] modified file
 * 	2013/09/03 - [Qian Shen] added point mapping APIs
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

#ifndef _LIB_DEWARP_H_
#define _LIB_DEWARP_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_CPU_CORTEXA9_HF)
#  if !((( __GNUC__  == 4) && (  __GNUC_MINOR__ >= 7)) || (__GNUC__  > 4))
#    error  "Hard float needs Linaro Toolchain GCC4.8 to compile"
#  endif
#elif defined(CONFIG_CPU_CORTEXA9) && \
      (((( __GNUC__  == 4) && (  __GNUC_MINOR__ >= 7)) || (__GNUC__  > 4)))
#  error  "Soft float is not supported by GCC version > 4.7.4"
#endif

typedef signed short data_t;

typedef float degree_t;

typedef enum {
	HFLIP = (1 << 0),
	VFLIP = (1 << 1),
	ROTATE = (1 << 2),
} ROTATE_FLIP;

#define LDC_STITCH_WIDTH (1920)

typedef enum {
	PROJECTION_EQUIDISTANT = 0, // Linear scaled: r = f * theta (most common F theta)
	PROJECTION_STEREOGRAPHIC, // Stereographic: r = 2 * f * tan(theta / 2)
	PROJECTION_LOOKUPTABLE, // For other customized lens which the relationship between r and theta is given out in a look up table, such as tailor distortion lens.

	PROJECTION_MODE_TOTAL_NUM,
	PROJECTION_MODE_FIRST = PROJECTION_EQUIDISTANT,
	PROJECTION_MODE_LAST = PROJECTION_LOOKUPTABLE,
} PROJECTION_MODE;

typedef enum {
	CEILING_EAST = 0,
	CEILING_NORTH,
	CEILING_WEST,
	CEILING_SOUTH,
} ORIENTATION;

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

typedef struct {
	int wall_normal_max_area_num;
	int wall_panor_max_area_num;
	int ceiling_normal_max_area_num;
	int ceiling_panor_max_area_num;
	int ceiling_sub_max_area_num;
	int desktop_normal_max_area_num;
	int desktop_panor_max_area_num;
	int desktop_sub_max_area_num;
	int max_warp_input_width;
	int max_warp_input_height;
	int max_warp_output_width;
} fisheye_config_t;

typedef struct {
	int x;
	int y;
} point_t;

typedef struct {
	float x;
	float y;
} fpoint_t;

typedef struct {
	int width;
	int height;
} rect_t;

typedef struct {
	int width;
	int height;
	point_t upper_left;
} rect_in_main_t;

typedef struct {
	int num;
	int denom;
} frac_t;

typedef struct {
	frac_t left_ver_zoom;
	frac_t right_ver_zoom;
	frac_t top_hor_zoom; /* only support top_hor_zoom and bottom_hor_zoom */
	frac_t bottom_hor_zoom;
} warp_zoom_t;

typedef struct {
	rect_in_main_t output;
	rect_in_main_t inter_output;
	frac_t zoom;
	frac_t hor_zoom;
	frac_t vert_zoom;
	int pitch;
	int yaw;
	int circle_radius;
	int circle_max_fov;
	point_t circle_center_in_premain;
	int rotate;
} warp_region_t;

typedef struct {
	degree_t pan;
	degree_t tilt;
} pantilt_angle_t;

typedef struct {
	int cols;
	int rows;
	int grid_width;
	int grid_height;
	data_t* addr;
} vector_map_t;

typedef struct {
	rect_t size;
	point_t unwarp_center;
	fpoint_t upper_left;
	fpoint_t lower_right;
} input_limit_t;

typedef struct {
	PROJECTION_MODE projection_mode;
	degree_t max_fov;
	int max_radius;
	int max_input_width;              // In fisheye (multi region) dewarp, max_input = pre_main; In lens warp, max_input = vin
	int max_input_height;
	point_t lens_center_in_max_input;
	int lut_focal_length;             // only for PROJECTION_LOOKUPTABLE
	int* lut_radius;                  // only for PROJECTION_LOOKUPTABLE
	int main_buffer_width;	// we need this value to check stitching or not while init library.
	input_limit_t max_input;
} dewarp_init_t;

typedef struct {
	rect_in_main_t input;
	rect_in_main_t inter_output;
	rect_in_main_t output;
	int rotate_flip;
	vector_map_t ver_map;
	vector_map_t hor_map;
	vector_map_t me1_ver_map;
} warp_vector_t;

int dewarp_init(dewarp_init_t* setup);
int dewarp_deinit(void);
int dewarp_get_version(version_t* version);

// Lens Transforming APIs : lens to xxx
int lens_no_transform(warp_region_t* region, rect_in_main_t* roi_in_vin,
	warp_vector_t* vector);
int lens_wall_normal(warp_region_t* region, warp_vector_t* vector);
int lens_wall_panorama(warp_region_t* region, degree_t hor_range,
	warp_vector_t* vector);
int lens_wall_subregion_roi(warp_region_t* region, fpoint_t* roi_in_fisheye,
	warp_vector_t* vector, pantilt_angle_t* angle);
int lens_wall_subregion_angle(warp_region_t* region, pantilt_angle_t* angle,
	warp_vector_t* vector, fpoint_t* roi_in_fisheye);

// Fisheye Configuration APIs
int fisheye_get_config(fisheye_config_t* config);
int fisheye_set_config(fisheye_config_t* config);

// Fisheye Transforming APIs
int fisheye_no_transform(warp_region_t* region, rect_in_main_t* roi_in_premain,
	warp_vector_t* vector);

int fisheye_wall_normal(warp_region_t* region, warp_vector_t* vector);
int fisheye_wall_panorama(warp_region_t* region, degree_t hor_range,
	warp_vector_t* vector);
int fisheye_wall_subregion_roi(warp_region_t* region, fpoint_t* roi_in_fisheye,
	warp_vector_t* vector, pantilt_angle_t* angle);
int fisheye_wall_subregion_angle(warp_region_t* region, pantilt_angle_t* angle,
	warp_vector_t* vector, fpoint_t* roi_in_fisheye);

int fisheye_ceiling_normal(warp_region_t* region, degree_t top_angle,
	ORIENTATION orient, warp_vector_t* vector);
int fisheye_ceiling_panorama(warp_region_t* region, degree_t top_angle,
	degree_t hor_range, ORIENTATION orient, warp_vector_t* vector);
int fisheye_ceiling_subregion_roi(warp_region_t* region,
	fpoint_t* froi_in_fisheye,
	warp_vector_t* vector, pantilt_angle_t* angle);
int fisheye_ceiling_subregion_angle(warp_region_t* region,
	pantilt_angle_t* angle, warp_vector_t* vector, fpoint_t* roi_in_fisheye);

int fisheye_desktop_normal(warp_region_t* region, degree_t bottom_angle,
	ORIENTATION orient, warp_vector_t* vector);
int fisheye_desktop_panorama(warp_region_t* region, degree_t bottom_angle,
	degree_t hor_range, ORIENTATION orient, warp_vector_t* vector);
int fisheye_desktop_subregion_roi(warp_region_t* region,
	fpoint_t* roi_in_fisheye,
	warp_vector_t* vector, pantilt_angle_t* angle);
int fisheye_desktop_subregion_angle(warp_region_t* region,
	pantilt_angle_t* angle, warp_vector_t* vector, fpoint_t* roi_in_fisheye);

// Fisheye Point Mapping APIs : xxx to fisheye
int point_mapping_wall_normal_to_fisheye(warp_region_t* region,
	fpoint_t* point_in_region, fpoint_t* point_in_fisheye);
int point_mapping_wall_panorama_to_fisheye(warp_region_t* region,
	degree_t hor_range, fpoint_t* point_in_region, fpoint_t* point_in_fisheye);
int point_mapping_wall_subregion_to_fisheye(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_region,
	fpoint_t* point_in_fisheye, pantilt_angle_t* angle);

int point_mapping_ceiling_normal_to_fisheye(warp_region_t* region,
	degree_t top_angle, ORIENTATION orient, fpoint_t* point_in_region,
	fpoint_t* point_in_fisheye);
int point_mapping_ceiling_panorama_to_fisheye(warp_region_t* region,
	degree_t top_angle, degree_t hor_range, ORIENTATION orient,
	fpoint_t* point_in_region, fpoint_t* fpoint_in_fisheye);
int point_mapping_ceiling_subregion_to_fisheye(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_region,
	fpoint_t* point_in_fisheye, pantilt_angle_t* angle);

int point_mapping_desktop_normal_to_fisheye(warp_region_t* region,
	degree_t bottom_angle, ORIENTATION orient, fpoint_t* point_in_region,
	fpoint_t* point_in_fisheye);
int point_mapping_desktop_panorama_to_fisheye(warp_region_t* region,
	degree_t bottom_angle, degree_t hor_range, ORIENTATION orient,
	fpoint_t* point_in_region, fpoint_t* point_in_fisheye);
int point_mapping_desktop_subregion_to_fisheye(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_region,
	fpoint_t* point_in_fisheye, pantilt_angle_t* angle);

// Fisheye Point Mapping APIs : fisheye to xxx
int point_mapping_fisheye_to_wall_normal(warp_region_t* region,
	fpoint_t* point_in_fisheye, fpoint_t* point_in_region);
int point_mapping_fisheye_to_wall_panorama(warp_region_t* region,
	degree_t hor_range, fpoint_t* point_in_fisheye, fpoint_t* point_in_region);
int point_mapping_fisheye_to_wall_subregion(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_fisheye,
	fpoint_t* point_in_region);

int point_mapping_fisheye_to_ceiling_normal(warp_region_t* region,
	degree_t top_angle, ORIENTATION orient, fpoint_t* point_in_fisheye,
	fpoint_t* point_in_region);
int point_mapping_fisheye_to_ceiling_panorama(warp_region_t* region,
	degree_t top_angle, degree_t hor_range, ORIENTATION orient,
	fpoint_t* point_in_fisheye, fpoint_t* point_in_region);
int point_mapping_fisheye_to_ceiling_subregion(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_fisheye,
	fpoint_t* point_in_region);

int point_mapping_fisheye_to_desktop_normal(warp_region_t* region,
	degree_t bottom_angle, ORIENTATION orient, fpoint_t* point_in_fisheye,
	fpoint_t* point_in_region);
int point_mapping_fisheye_to_desktop_panorama(warp_region_t* region,
	degree_t bottom_angle, degree_t hor_range, ORIENTATION orient,
	fpoint_t* point_in_fisheye, fpoint_t* point_in_region);
int point_mapping_fisheye_to_desktop_subregion(warp_region_t* region,
	fpoint_t* roi_in_fisheye, fpoint_t* point_in_fisheye,
	fpoint_t* point_in_region);

#ifdef __cplusplus
}
#endif

#endif	/* _LIB_DEWARP_H_ */

