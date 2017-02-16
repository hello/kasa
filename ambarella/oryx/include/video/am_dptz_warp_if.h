/*******************************************************************************
 * am_dptz_warp_if.h
 *
 * History:
 *   2014-12-15 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_VIDEO_AM_DPTZ_WARP_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_DPTZ_WARP_IF_H_

#include "am_video_types.h"

#include "lib_dewarp_header.h"
#include "lib_dewarp.h"

#define AM_MAX_DZOOM_FACTOR_X 8
#define AM_MAX_DZOOM_FACTOR_Y 8
#define AM_MIN_DZOOM_INPUT_RECT_WIDTH  64
#define AM_MIN_DZOOM_INPUT_RECT_HEIGHT  64

#define MAX_DISTORTION_LOOKUP_TABLE_ENTRY_NUM  256
#define MAX_LUT_FILE_NAME  (256)

enum AM_DPTZ_CONTROL_METHOD
{
  //default, simple control method.
  AM_DPTZ_CONTROL_NO_CHANGE       = 0,
  //default, simple control method.
  AM_DPTZ_CONTROL_BY_DPTZ_RATIO   = 1,
  //accurate control, but needs app to calculate the ratio
  AM_DPTZ_CONTROL_BY_INPUT_WINDOW = 2,
};

/* Easy zoom mode is provided to use enum to fast and simple zoom,
 * easier to use for consumer, easy zoom sometimes is easier to use,
 * especially when the sensor aspect ratio is not strictly same as
 * encoding aspect ratio.
 */
enum AM_EASY_ZOOM_MODE
{
  AM_EASY_ZOOM_MODE_FULL_FOV       = 0,
  AM_EASY_ZOOM_MODE_PIXEL_TO_PIXEL = 1,
  AM_EASY_ZOOM_MODE_AMPLIFY        = 2,
};

enum AM_WARP_PROJECTION_MODE
{
  AM_WARP_PROJRECTION_EQUIDISTANT  = 0,
  AM_WARP_PROJECTION_STEREOGRAPHIC = 1,
  AM_WARP_PROJECTION_LOOKUP_TABLE  = 2
};

enum AM_WARP_MODE
{
  AM_WARP_MODE_NO_TRANSFORM = 0,
  //"normal" correciton, both H/V lines are corrected to straight
  AM_WARP_MODE_RECTLINEAR   = 1,
  //"V" lines are corrected to be straight, used in wall mount
  AM_WARP_MODE_PANORAMA     = 2,
  //reserved for more advanced mode
  AM_WARP_MODE_SUBREGION    = 3,
};

enum AM_WARP_ZOOM_SOURCE
{
  AM_WARP_ZOOM_DPTZ = 0,
  AM_WARP_ZOOM_LDC = 1,
};

struct AMLenParam
{
  float max_hfov_degree;
  float pano_hfov_degree;         // For Panorama only
  float image_circle_mm;          //fisheye lens image circle in mm
  float efl_mm;                   //Effective focal length in mm
  int32_t lut_entry_num;          //lookup table entry num
  frac_t zoom;
  point_t lens_center_in_max_input;
  char lut_file[MAX_LUT_FILE_NAME];
};

struct AMWarpParam
{
    AM_WARP_PROJECTION_MODE proj_mode;  //most lens use LUT
    AM_WARP_MODE warp_mode; //RECTLINEAR for LDC,  PANORAMA for "180 Wall-mount"
    AMLenParam lens_param;  //Lens param to be loaded from config
};

struct AMDPTZRatio
{
    /* usually, zoom_factor_x should be equal to zoom_factor_y,
     * otherwise, after digital zoom, the aspect ratio will change
     */
    float zoom_factor_x; /* like 1.0X,  4.0X,  8.0X */
    float zoom_factor_y; /* like 1.0X,  4.0X,  8.0X */

    /* if zoom_center_pos_x and zoom_center_pos_y are both 0,
     * then it's center zoomed */
    float zoom_center_pos_x; /* -1.0~ 1.0. -1.0: left most, 1.0: right most */
    float zoom_center_pos_y; /* -1.0~ 1.0. -1.0: top most, 1.0: bottom most */
};

struct AMDPTZParam
{
    //DPTZ related
    AM_DPTZ_CONTROL_METHOD dptz_method;
    //only effective if dptz_method uses Input Window
    AMRect dptz_input_window;
    //only effective if dptz_method uses RATIO
    AMDPTZRatio dptz_ratio;
};

class AMIDPTZWarp
{
  public:
    // DPTZ related APIs
    virtual AM_RESULT set_dptz_input_window(AM_ENCODE_SOURCE_BUFFER_ID id,
                                            AMRect *input_rect) = 0;
    virtual AM_RESULT apply_dptz_warp() = 0;
    virtual AM_RESULT set_dptz_ratio(AM_ENCODE_SOURCE_BUFFER_ID id,
                                     AMDPTZRatio *ratio) = 0;
    virtual AM_RESULT get_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID id,
                                     AMDPTZParam *dptz_param) = 0;
    virtual AM_RESULT set_dptz_easy(AM_ENCODE_SOURCE_BUFFER_ID id,
                                    AM_EASY_ZOOM_MODE zoom_mode) = 0;
    virtual AM_RESULT set_warp_param(AMWarpParam *warp_param) = 0;
    virtual AM_RESULT get_warp_param(AMWarpParam *warp_param) = 0;
    virtual AM_RESULT set_config(AMDPTZWarpConfig *dptz_warp_config) = 0;
    virtual AM_RESULT get_config(AMDPTZWarpConfig *dptz_warp_config) = 0;
    virtual ~AMIDPTZWarp(){}
};

#endif /* ORYX_INCLUDE_VIDEO_AM_DPTZ_WARP_IF_H_ */
