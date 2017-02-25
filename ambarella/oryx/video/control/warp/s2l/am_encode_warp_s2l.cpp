/*******************************************************************************
 * am_encode_warp_s2l.cpp
 *
 * History:
 *   Nov 6, 2015 - [zfgong] created file
 *   Sep 22, 2016 - [Huaiqing Wang] modified file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_encode_warp_s2l.h"
#include "am_video_types.h"
#include "am_vin.h"

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  AMPluginData *pdata = ((AMPluginData*)data);
  return AMEncodeWarpS2L::create(pdata->vin, pdata->stream);
}
#ifdef __cplusplus
}
#endif

AMEncodeWarpS2L *AMEncodeWarpS2L::create(AMVin *vin, AMEncodeStream *stream)
{
  AMEncodeWarpS2L *warp = new AMEncodeWarpS2L();
  do {
    if (!warp) {
      ERROR("Failed to create AMEncodeWarpS2L");
      break;
    }
    if (AM_RESULT_OK != warp->init(vin, stream)) {
      delete warp;
      warp = nullptr;
      break;
    }
    if (!(warp->m_config = AMWarpConfigS2L::get_instance())) {
      ERROR("Failed to create AMWarpConfig!");
      break;
    }
    if (AM_RESULT_OK != warp->load_config()) {
      break;
    }

  } while (0);
  return warp;
}

void AMEncodeWarpS2L::destroy()
{
  inherited::destroy();
}

void* AMEncodeWarpS2L::get_interface()
{
  return ((AMIEncodeWarp*)this);
}

AM_RESULT AMEncodeWarpS2L::set_ldc_mode(int region_id, AM_WARP_MODE mode)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((mode > AM_WARP_MODE_FULLFRAME) || (mode < AM_WARP_MODE_NO_TRANSFORM)) {
      ERROR("Invalid LDC mode: %d!", mode);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config_param.warp_mode.first = true;
    m_config_param.warp_mode.second = mode;
  }while(0);

  return ret;
}

AM_RESULT AMEncodeWarpS2L::get_ldc_mode(int region_id, AM_WARP_MODE &mode)
{
  mode = m_config_param.warp_mode.second;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::set_ldc_strength(int region_id, float strength)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((strength > 36.0) || (strength < 0.0)) {
      ERROR("Invalid LDC strength: %f!", strength);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config_param.ldc_strength.first = true;
    m_config_param.ldc_strength.second = strength;
  }while(0);

  return ret;
}

AM_RESULT AMEncodeWarpS2L::get_ldc_strength(int region_id, float &strength)
{
  strength = m_config_param.ldc_strength.second;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::set_max_radius(int region_id, int radius)
{
  m_config_param.max_radius.first = true;
  m_config_param.max_radius.second = radius;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::get_max_radius(int region_id, int &radius)
{
  radius = m_config_param.max_radius.second;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::set_pano_hfov_degree(int region_id, float degree)
{
  AM_RESULT ret = AM_RESULT_OK;

  do {
    if ((degree > 180.0) || (degree < 1.0)) {
      ERROR("Invalid Pano hfov degree: %f!", degree);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config_param.pano_hfov_degree.first = true;
    m_config_param.pano_hfov_degree.second = degree;
  }while(0);

  return ret;
}

AM_RESULT AMEncodeWarpS2L::get_pano_hfov_degree(int region_id, float &degree)
{
  degree = m_config_param.pano_hfov_degree.second;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::set_warp_region_yaw_pitch(int region_id,
                                                     int yaw, int pitch)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((yaw > 90) || (yaw < -90)) {
      ERROR("Invalid warp region yaw: %d", yaw);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    if ((pitch > 90) || (pitch < -90)) {
      ERROR("Invalid warp region pitch: %d", pitch);
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_config_param.lens.first = true;
    m_config_param.lens.second.yaw = yaw;
    m_config_param.lens.second.pitch = pitch;
  } while(0);

  return ret;
}

AM_RESULT AMEncodeWarpS2L::get_warp_region_yaw_pitch(int region_id,
                                                     int &yaw, int &pitch)
{
  yaw = m_config_param.lens.second.yaw;
  pitch = m_config_param.lens.second.pitch;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::set_hor_ver_zoom(int region_id,
                                            AMFrac hor, AMFrac ver)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if ((hor.num > 0) && (hor.denom > 0)) {
      m_config_param.lens.first = true;
      m_config_param.lens.second.hor_zoom = hor;
    }
    if ((ver.num > 0) && (ver.denom > 0)) {
      m_config_param.lens.first = true;
      m_config_param.lens.second.ver_zoom = ver;
    }
  } while(0);

  return ret;
}

AM_RESULT AMEncodeWarpS2L::get_hor_ver_zoom(int region_id,
                                            AMFrac &hor, AMFrac &ver)
{
  hor = m_config_param.lens.second.hor_zoom;
  ver = m_config_param.lens.second.ver_zoom;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeWarpS2L::init_lens_warp_param(dewarp_init_t &param,
                                                warp_region_t &region)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMPlatformBufferFormat buf_format;
    AMPlatformVinInfo vin_info;
    AMVinAAAInfo aaa_info = {0};

    if (!inherited::is_ldc_enable()) {
      ERROR("dewarp only valid when ldc enable");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    vin_info.id = AM_VIN_0;
    vin_info.mode = AM_VIN_MODE_CURRENT;
    if ((result = m_platform->vin_info_get(vin_info)) != AM_RESULT_OK) {
      ERROR("Failed to get VIN info!");
      break;
    }

    if (!m_pixel_width_um) {
      if ((result = m_platform->get_aaa_info(aaa_info)) != AM_RESULT_OK) {
        ERROR("Failed to get AAA info!\n");
        break;
      }
      m_pixel_width_um = (aaa_info.pixel_size >> 16) +
          (1.0 / 0x10000) * (aaa_info.pixel_size & 0xFFFF);
    }

    buf_format.id = AM_SOURCE_BUFFER_MAIN;
    if ((result = m_platform->buffer_format_get(buf_format)) != AM_RESULT_OK) {
      ERROR("Failed to get buffer config!");
      break;
    }
    region.output.upper_left.x = 0;
    region.output.upper_left.y = 0;
    region.output.width = buf_format.size.width;
    region.output.height = buf_format.size.height;

    param.projection_mode   = PROJECTION_MODE(m_config_param.proj_mode.second);
    param.max_fov           = m_config_param.ldc_strength.second > 0 ?
        m_config_param.ldc_strength.second * 10 : 180.0;
    param.max_input_width   = vin_info.size.width;
    param.max_input_height  = vin_info.size.height;
    param.max_radius        = m_config_param.max_radius.second > 0 ?
        m_config_param.max_radius.second : param.max_input_width / 2;
    param.lut_focal_length  = m_config_param.lens.second.efl_mm *
        1000 / m_pixel_width_um;
    param.lut_radius        = nullptr;
    param.main_buffer_width = buf_format.size.width;

    if (m_config_param.lens.second.lens_center_in_max_input.x > 0) {
      param.lens_center_in_max_input.x =
          m_config_param.lens.second.lens_center_in_max_input.x;
    } else {
      param.lens_center_in_max_input.x = param.max_input_width / 2;
    }

    if (m_config_param.lens.second.lens_center_in_max_input.y > 0) {
      param.lens_center_in_max_input.y =
          m_config_param.lens.second.lens_center_in_max_input.y;
    } else {
      param.lens_center_in_max_input.y = param.max_input_height / 2;
    }

    if ((m_config_param.proj_mode.second == AM_WARP_PROJECTION_LOOKUP_TABLE) &&
        (!m_config_param.lens.second.lut_file.empty())) {
      FILE *fp = fopen(m_config_param.lens.second.lut_file.c_str(), "r");
      char line[1024] = {0};
      uint32_t i = 0;
      if (!fp) {
        ERROR("Failed to open file: %s for reading!",
              m_config_param.lens.second.lut_file.c_str());
        result = AM_RESULT_ERR_IO;
        break;
      }
      while (fgets(line, sizeof(line), fp) != nullptr) {
        m_distortion_lut[i] = (int32_t)(atof(line) * param.max_input_width);
        ++ i;
      }
      fclose(fp);
      param.lut_radius = m_distortion_lut;
    }
    INFO("projection=%d, max fov=%f, max input=%dx%d, max radius=%d, main buf width=%d\n",
         param.projection_mode, param.max_fov, param.max_input_width,
         param.max_input_height, param.max_radius, param.main_buffer_width);
    INFO("lens center = %dx%d\n",param.lens_center_in_max_input.x,param.lens_center_in_max_input.y);
  } while (0);

  return result;
}

AM_RESULT AMEncodeWarpS2L::create_lens_warp_vector(warp_region_t &region,
                                                   warp_vector_t &vector)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    rect_in_main_t notrans = {0};

    region.pitch = m_config_param.lens.second.pitch;
    region.yaw = m_config_param.lens.second.yaw;
    /* Zoom factor */
    region.hor_zoom.num = m_config_param.lens.second.hor_zoom.num;
    region.hor_zoom.denom = m_config_param.lens.second.hor_zoom.denom;
    region.vert_zoom.num = m_config_param.lens.second.ver_zoom.num;
    region.vert_zoom.denom = m_config_param.lens.second.ver_zoom.denom;
    if (region.hor_zoom.num <= 0 || region.hor_zoom.denom <= 0) {
      NOTICE("Invalid horizontal zoom ratio: %d/%d, set to default value: 1/1",
             region.hor_zoom.num, region.hor_zoom.denom);
      region.hor_zoom.num = 1;
      region.hor_zoom.denom = 1;
    }
    if (region.vert_zoom.num <= 0 || region.vert_zoom.denom <= 0) {
      NOTICE("Invalid vertical zoom ratio: %d/%d, set to default value: 1/1",
             region.vert_zoom.num, region.vert_zoom.denom);
      region.vert_zoom.num = 1;
      region.vert_zoom.denom = 1;
    }

    vector.hor_map.addr = (data_t*)m_mem.addr;
    vector.ver_map.addr = (data_t*)(m_mem.addr +
        MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t));
    vector.me1_ver_map.addr = (data_t*)(m_mem.addr +
        MAX_WARP_TABLE_SIZE_LDC * sizeof(data_t) * 2);

    switch (m_config_param.warp_mode.second) {
      case AM_WARP_MODE_RECTLINEAR:
        /* create vector */
        if (lens_wall_normal(&region, &vector) <= 0) {
          ERROR("lens_wall_normal");
          result = AM_RESULT_ERR_INVALID;
        }
        break;
      case AM_WARP_MODE_PANORAMA:
        /* create vector */
        if (lens_wall_panorama(&region, m_config_param.pano_hfov_degree.second,
                               &vector) <= 0) {
          ERROR("lens_wall_panorama");
          result = AM_RESULT_ERR_INVALID;
        }
        break;
      case AM_WARP_MODE_NO_TRANSFORM:
        /* this mode can be used to do DPTZ_I */
      {
        AMPlatformVinInfo vin_info;
        vin_info.id = AM_VIN_0;
        vin_info.mode = AM_VIN_MODE_CURRENT;
        if ((result = m_platform->vin_info_get(vin_info)) != AM_RESULT_OK) {
          ERROR("Failed to get VIN info!");
          break;
        }
        notrans.upper_left.x = 0;
        notrans.upper_left.y = 0;
        notrans.width = vin_info.size.width;
        notrans.height = vin_info.size.height;
        if (lens_no_transform(&region, &notrans, &vector) <= 0) {
          ERROR("lens_wall_no_transform");
          result = AM_RESULT_ERR_INVALID;
        }
        break;
      }
      case AM_WARP_MODE_SUBREGION:
      default:
        ERROR("Warp mode%d not supported For LDC!\n",
              m_config_param.warp_mode.second);
        result = AM_RESULT_ERR_PLATFORM_SUPPORT;
        break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeWarpS2L::update_lens_warp_area(warp_vector_t &vector,
                                                 iav_warp_area &area)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_config_param.ldc_strength.second > 0) {
      area.enable = 1;
      area.input.width = vector.input.width;
      area.input.height = vector.input.height;
      area.input.x = vector.input.upper_left.x;
      area.input.y = vector.input.upper_left.y;
      area.output.width = vector.output.width;
      area.output.height = vector.output.height;
      area.output.x = vector.output.upper_left.x;
      area.output.y = vector.output.upper_left.y;
      area.rotate_flip = vector.rotate_flip;

      area.h_map.enable = ((vector.hor_map.rows) > 0 &&
          (vector.hor_map.cols) > 0);
      area.h_map.h_spacing = get_grid_spacing(vector.hor_map.grid_width);
      area.h_map.v_spacing = get_grid_spacing(vector.hor_map.grid_height);
      area.h_map.output_grid_row = vector.hor_map.rows;
      area.h_map.output_grid_col = vector.hor_map.cols;
      area.h_map.data_addr_offset = 0;

      area.v_map.enable = ((vector.ver_map.rows) > 0 &&
          (vector.ver_map.cols) > 0);
      area.v_map.h_spacing = get_grid_spacing(vector.ver_map.grid_width);
      area.v_map.v_spacing = get_grid_spacing(vector.ver_map.grid_height);
      area.v_map.output_grid_row = vector.ver_map.rows;
      area.v_map.output_grid_col = vector.ver_map.cols;
      area.v_map.data_addr_offset = MAX_WARP_TABLE_SIZE_LDC * sizeof(s16);

      area.me1_v_map.h_spacing =
          get_grid_spacing(vector.me1_ver_map.grid_width);
      area.me1_v_map.v_spacing =
          get_grid_spacing(vector.me1_ver_map.grid_height);
      area.me1_v_map.output_grid_row = vector.me1_ver_map.rows;
      area.me1_v_map.output_grid_col = vector.me1_ver_map.cols;
      area.me1_v_map.data_addr_offset =
          MAX_WARP_TABLE_SIZE_LDC * sizeof(s16) * 2;

      INFO("\tInput size %dx%d, offset %dx%d\n", area.input.width,
          area.input.height, area.input.x, area.input.y);
      INFO("\tOutput size %dx%d, offset %dx%d\n", area.output.width,
          area.output.height, area.output.x, area.output.y);
      INFO("\tHor Table %dx%d, grid %dx%d (%dx%d)\n",
          area.h_map.output_grid_col, area.h_map.output_grid_row,
          area.h_map.h_spacing,area.h_map.v_spacing,
          vector.hor_map.grid_width, vector.hor_map.grid_height);
      INFO("\tVer Table %dx%d, grid %dx%d (%dx%d)\n",
          area.v_map.output_grid_col, area.v_map.output_grid_row,
          area.v_map.h_spacing,area.v_map.v_spacing,
          vector.ver_map.grid_width, vector.ver_map.grid_height);
      INFO("\tME1 Ver Table %dx%d, grid %dx%d (%dx%d)\n",
          area.me1_v_map.output_grid_col, area.me1_v_map.output_grid_row,
          area.me1_v_map.h_spacing, area.me1_v_map.v_spacing,
          vector.me1_ver_map.grid_width, vector.me1_ver_map.grid_height);
    } else {
      //clear ldc
      area.enable = 0;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeWarpS2L::check_warp_control(iav_warp_area &area,
                                              dewarp_init_t &param)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!area.enable) {
      break;
    }

    if (!area.input.height || !area.input.width ||
        !area.output.width || !area.output.height) {
      ERROR("input size %dx%d, output size %dx%d cannot be 0\n",
            area.input.width, area.input.height,
            area.output.width, area.output.height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if ((area.input.x + area.input.width > uint32_t(param.max_input_width)) ||
        (area.input.y + area.input.height > uint32_t(param.max_input_height))) {
      ERROR("input size %dx%d + offset %dx%d is out of vin %dx%d.\n",
            area.input.width, area.input.height,
            area.input.x, area.input.y,
            param.max_input_width, param.max_input_height);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeWarpS2L::apply()
{
  AM_RESULT result = AM_RESULT_OK;
  bool deinit_flag = false;
  do {
    AM_IAV_STATE iav_state;
    warp_region_t lens_warp_region = {0};
    warp_vector_t lens_warp_vector = {0};
    dewarp_init_t param;
    memset(&param, 0, sizeof(param));

    if (AM_RESULT_OK != (result = m_platform->iav_state_get(iav_state))) {
      ERROR("AMWarp: iav_state_get failed\n");
      break;
    }
    if (AM_IAV_STATE_PREVIEW != iav_state &&
        AM_IAV_STATE_ENCODING != iav_state) {
      ERROR("AMWarp: iav state must be preview or encode\n");
      result = AM_RESULT_ERR_NO_ACCESS;
      break;
    }

    if ((result = init_lens_warp_param(param, lens_warp_region))
        != AM_RESULT_OK) {
      break;
    }
    if (dewarp_init(&param) < 0) {
      ERROR("dewarp_init failed");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    deinit_flag = true;

    if ((result = create_lens_warp_vector(lens_warp_region, lens_warp_vector))
        != AM_RESULT_OK) {
      break;
    }

    iav_warp_ctrl warp_ctrl = {IAV_WARP_CTRL_MAIN};
    iav_warp_area &warp_area = warp_ctrl.arg.main.area[0];
    u32 flags = (1 << IAV_WARP_CTRL_MAIN);

    if ((result = update_lens_warp_area(lens_warp_vector, warp_area))
        != AM_RESULT_OK) {
      break;
    }

    if ((result = check_warp_control(warp_area, param))
        != AM_RESULT_OK) {
      break;
    }

    if (ioctl(m_iav, IAV_IOC_CFG_WARP_CTRL, &warp_ctrl) < 0) {
      PERROR("IAV_IOC_CFG_WARP_CTRL");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    if (ioctl(m_iav, IAV_IOC_APPLY_WARP_CTRL, &flags) < 0) {
      PERROR("IAV_IOC_APPLY_WARP_CTRL");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  if (deinit_flag) {
    dewarp_deinit();
  }
  return result;
}

AM_RESULT AMEncodeWarpS2L::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->load_config()) != AM_RESULT_OK) {
      ERROR("Failed to get warp config!");
      break;
    }
    if ((result = m_config->get_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get warp config!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeWarpS2L::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->set_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get warp config!");
      break;
    }
    if ((result = m_config->save_config()) != AM_RESULT_OK) {
      ERROR("Failed to set warp config!");
      break;
    }
  } while (0);
  return result;
}

int AMEncodeWarpS2L::get_grid_spacing(const int spacing)
{
  int grid_space = GRID_SPACING_PIXEL_64;
  switch (spacing) {
    case 16:
      grid_space = GRID_SPACING_PIXEL_16;
      break;
    case 32:
      grid_space = GRID_SPACING_PIXEL_32;
      break;
    case 64:
      grid_space = GRID_SPACING_PIXEL_64;
      break;
    case 128:
      grid_space = GRID_SPACING_PIXEL_128;
      break;
    case 512:
      grid_space = GRID_SPACING_PIXEL_512;
      break;
    default:
      grid_space = GRID_SPACING_PIXEL_64;
      break;
  }
  return grid_space;
}

AMEncodeWarpS2L::AMEncodeWarpS2L() :
    inherited("WARP.S2L"),
    m_config(nullptr)
{
}

AMEncodeWarpS2L::~AMEncodeWarpS2L()
{
  m_config = nullptr;
}
