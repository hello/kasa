/*******************************************************************************
 * am_video_config.cpp
 *
 * History:
 *   2014-7-14 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_video_param.h"
#include "am_base_include.h"
#include "am_encode_device.h"
#include "am_configure.h"
#include "am_vin_trans.h"
#include "am_vout_trans.h"
#include "am_video_trans.h"
#include "am_video_camera.h"

#define VIN_CONFIG_FILE           "vin.acs"
#define VOUT_CONFIG_FILE          "vout.acs"
#define STREAM_FORMAT_FILE        "stream_fmt.acs"
#define STREAM_CONFIG_FILE        "stream_cfg.acs"
#define SOURCE_BUFFER_CONFIG_FILE "source_buffer.acs"
#define FEATURE_CONFIG_FILE       "features.acs"
#define LBR_CONFIG_FILE           "lbr.acs"
#define DPTZ_WARP_CONFIG_FILE     "dptz_warp.acs"
#define OSD_OVERLAY_CONFIG_FILE   "osd_overlay.acs"

#define DEFAULT_ORYX_CONF_DIR "/etc/oryx/video/"

/************  AM VIN Config Starts*******************************************/
AMVinConfig::AMVinConfig() :
    m_changed(false),
    m_need_restart(false)
{
  for(int32_t i = 0; i < AM_VIN_MAX_NUM; i++) {
    m_loaded.vin[i].type = AM_VIN_TYPE_NONE;
    m_loaded.vin[i].mode = AM_VIN_MODE_AUTO;
    m_loaded.vin[i].fps = AM_VIDEO_FPS_AUTO;
    m_loaded.vin[i].flip = AM_VIDEO_FLIP_NONE;
  }

  m_using = m_loaded;
}

AMVinConfig::~AMVinConfig()
{
}

AM_RESULT AMVinConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMVinParamAll vin_param;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  uint32_t i;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(VIN_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMVinConfig: fail to create vin AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& vin = *config;
    memset(&vin_param, 0, sizeof(vin_param));

    for (i = 0 ; i < AM_VIN_MAX_NUM ; i++)  {
      //VIN Type
      //tag = std::string("vin") + std::to_string(i);

      if (vin[i]["type"].exists()) {
        tmp = vin[i]["type"].get<std::string>("");
        vin_param.vin[i].type = vin_type_str_to_enum(tmp.c_str());
      } else{
        vin_param.vin[i].type = AM_VIN_TYPE_NONE; //set to none by default
      }

      //VIN Mode
      if (vin[i]["mode"].exists()) {
        tmp = vin[i]["mode"].get<std::string>("");
        vin_param.vin[i].mode = vin_mode_str_to_enum(tmp.c_str());
      } else{
        vin_param.vin[i].mode = AM_VIN_MODE_AUTO; //set to auto by default
      }

      //Vin Flip
      if (vin[i]["flip"].exists()) {
        tmp = vin[i]["flip"].get<std::string>("");
        vin_param.vin[i].flip = video_flip_str_to_enum(tmp.c_str());
      } else {
        vin_param.vin[i].flip = AM_VIDEO_FLIP_NONE; //set to linear by default
      }

      //Vin Fps
      if (vin[i]["fps"].exists()) {
        tmp = vin[i]["fps"].get<std::string>("");
        vin_param.vin[i].fps = video_fps_str_to_enum(tmp.c_str());
      } else {
        vin_param.vin[i].fps = AM_VIDEO_FPS_AUTO; //set to linear by default
      }
    }//end for
    //copy to m_loaded
    m_loaded = vin_param;
    m_changed = !!memcmp(&m_using, &m_loaded, sizeof(m_using));
  } while(0);

  //delete config class
  delete config;
  return result;
}

bool AMVinConfig::need_restart()
{
  return m_need_restart;
}

AM_RESULT AMVinConfig::sync()
{
  memcpy(&m_using, &m_loaded, sizeof(m_using));
  m_changed = false;
  return AM_RESULT_OK;
}

AM_RESULT AMVinConfig::set_mode_config(AM_VIN_ID id, AM_VIN_MODE mode)
{
  if (m_using.vin[id].mode != mode) {
    m_using.vin[id].mode = mode;
    m_changed = true;
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVinConfig::set_flip_config(AM_VIN_ID id, AM_VIDEO_FLIP flip)
{
  if (m_using.vin[id].flip != flip) {
    m_using.vin[id].flip = flip;
    m_changed = true;
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVinConfig::set_fps_config(AM_VIN_ID id, AM_VIDEO_FPS fps)
{
  if (m_using.vin[id].fps != fps) {
    m_using.vin[id].fps = fps;
    m_changed = true;
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVinConfig::set_bayer_config(AM_VIN_ID id,
                                        AM_VIN_BAYER_PATTERN bayer)
{
  if (m_using.vin[id].bayer_pattern != bayer) {
    m_using.vin[id].bayer_pattern = bayer;
    m_changed = true;
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVinConfig::save_config()
{
  uint32_t i;
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    INFO("AMVinConfig: save config(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(VIN_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMVinConfig: fail to open vin AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& vin = *config;
    AMVinParamAll *vin_param = &m_using;

    for (i = 0; i < AM_VIN_MAX_NUM; i ++) {
      //VIN Type
      vin[i]["type"] = vin_type_enum_to_str(vin_param->vin[i].type);
      vin[i]["mode"] = vin_mode_enum_to_str(vin_param->vin[i].mode);
      vin[i]["flip"] = video_flip_enum_to_str(vin_param->vin[i].flip);
      vin[i]["fps"] = video_fps_enum_to_str(vin_param->vin[i].fps);
    }
    if (!vin.save()){
      ERROR("AMVinConfig:  failed to save_config\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
    if (m_changed) {
      m_need_restart = true;
    }
  } while(0);

  //delete config class
  delete config;
  return result;
}

/************  AM VIN Config Ends**********************************************/


/************  AM VOUT Config Starts*******************************************/

AMVoutConfig::AMVoutConfig():
            m_changed(false),
            m_need_restart(false)
{
  int i;
  for( i= 0 ; i < AM_VOUT_MAX_NUM; i++) {
    m_loaded.vout[i].type = AM_VOUT_TYPE_NONE;
    m_loaded.vout[i].mode = AM_VOUT_MODE_AUTO;
    m_loaded.vout[i].fps = AM_VIDEO_FPS_AUTO;
    m_loaded.vout[i].flip = AM_VIDEO_FLIP_NONE;
  }
  m_using = m_loaded;
}

AMVoutConfig::~AMVoutConfig()
{
}

AM_RESULT AMVoutConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMVoutParamAll vout_param;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  uint32_t i;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(VOUT_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMVoutConfig: fail to create vout AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& vout = *config;
    memset(&vout_param, 0, sizeof(vout_param));

    for (i = 0; i < AM_VOUT_MAX_NUM; i ++) {
      //VOUT Type
      if (vout[i]["type"].exists()) {
        tmp = vout[i]["type"].get<std::string>("");
        vout_param.vout[i].type = vout_type_str_to_enum(tmp.c_str());
      } else {
        vout_param.vout[i].type = AM_VOUT_TYPE_NONE; //set to none by default
      }

      //mode
      if (vout[i]["mode"].exists()) {
        tmp = vout[i]["mode"].get<std::string>("");
        vout_param.vout[i].mode = vout_mode_str_to_enum(tmp.c_str());
      } else {
        vout_param.vout[i].mode = AM_VOUT_MODE_AUTO; //set to none by default
      }

      //flip
      if (vout[i]["flip"].exists()) {
        tmp = vout[i]["flip"].get<std::string>("");
        vout_param.vout[i].flip = video_flip_str_to_enum(tmp.c_str());
      } else {
        vout_param.vout[i].flip = AM_VIDEO_FLIP_NONE; //set to none by default
      }

      //rotate
      if (vout[i]["rotate"].exists()) {
        tmp = vout[i]["rotate"].get<std::string>("");
        vout_param.vout[i].rotate = video_rotate_str_to_enum(tmp.c_str());
      } else {
        //set to none by default
        vout_param.vout[i].rotate = AM_VIDEO_ROTATE_NONE;
      }

      //fps
      if (vout[i]["fps"].exists()) {
        tmp = vout[i]["fps"].get<std::string>("");
        vout_param.vout[i].fps = video_fps_str_to_enum(tmp.c_str());
      } else {
        vout_param.vout[i].fps = AM_VIDEO_FPS_AUTO; //set to none by default
      }
    }

    //copy to m_loaded
    m_loaded = vout_param;
    m_changed = !!memcmp(&m_using, &m_loaded, sizeof(m_using));
  } while (0);

  //delete config class
  delete config;
  return result;
}

bool AMVoutConfig::need_restart()
{
  return m_need_restart;
}

AM_RESULT AMVoutConfig::sync()
{
  memcpy(&m_using, &m_loaded, sizeof(m_using));
  m_changed = false;
  return AM_RESULT_OK;
}

AM_RESULT AMVoutConfig::save_config()
{
  INFO("AMVoutConfig: save config (m_using) to file \n");
  uint32_t i;
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    INFO("AMVoutConfig: save config(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(VOUT_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMVoutConfig: fail to open vout AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& vout = *config;
    AMVoutParamAll *vout_param = &m_using;

    for (i = 0; i < AM_VOUT_MAX_NUM; i ++) {
      vout[i]["type"] = vout_type_enum_to_str(vout_param->vout[i].type);
      vout[i]["mode"] = vout_mode_enum_to_str(vout_param->vout[i].mode);
      vout[i]["flip"] = video_flip_enum_to_str(vout_param->vout[i].flip);
      vout[i]["fps"] = video_fps_enum_to_str(vout_param->vout[i].fps);
    }
    if (!vout.save()){
      ERROR("AMVoutConfig:  failed to save_config\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
    if (m_changed) {
      m_need_restart = true;
    }
  } while(0);

  //delete config class
  delete config;
  return result;
}

/************  AM VOUT Config Ends*********************************************/


/************  AM Resource Limit Config Starts*********************************/
AMResourceLimitConfig::AMResourceLimitConfig(const std::string& file_name):
    cfg_file_name("")
{
  cfg_file_name = file_name;
}

AMResourceLimitConfig::~AMResourceLimitConfig()
{
}

AM_RESULT AMResourceLimitConfig::load_config(AMResourceLimitParam *resource_limit)
{
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir) {
      oryx_conf_dir = default_dir;
    }
    tmp = std::string(oryx_conf_dir) + std::string(cfg_file_name);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMResourceLimitConfig: fail to create resource limit AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& rsrc_lmt = *config;
    if (rsrc_lmt["max_streams"].exists()) {
      resource_limit->max_num_encode = rsrc_lmt["max_streams"].get<int>(4);
    } else {
      resource_limit->max_num_encode = 4;
    }
    if (rsrc_lmt["max_buffers"].exists()) {
      resource_limit->max_num_cap_sources = rsrc_lmt["max_buffers"].get<int>(4);
    } else {
      resource_limit->max_num_cap_sources = 4;
    }
    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; i ++) {
      if (rsrc_lmt["source_buffer_max_size"][i]["width"].exists()) {
        resource_limit->buf_max_width[i] = rsrc_lmt["source_buffer_max_size"][i]["width"].get<int>(0);
      } else {
        resource_limit->buf_max_width[i] = 0;
      }
      if (rsrc_lmt["source_buffer_max_size"][i]["height"].exists()) {
        resource_limit->buf_max_height[i] = rsrc_lmt["source_buffer_max_size"][i]["height"].get<int>(0);
      } else {
        resource_limit->buf_max_height[i] = 0;
      }
    }

    for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      if (rsrc_lmt["stream_max_size"][i]["width"].exists()) {
        resource_limit->stream_max_width[i] = rsrc_lmt["stream_max_size"][i]["width"].get<int>(0);
      } else {
        resource_limit->stream_max_width[i] = 0;
      }
      if (rsrc_lmt["stream_max_size"][i]["height"].exists()) {
        resource_limit->stream_max_height[i] = rsrc_lmt["stream_max_size"][i]["height"].get<int>(0);
      } else {
        resource_limit->stream_max_height[i] = 0;
      }
      if (rsrc_lmt["stream_max_gop_M"][i].exists()) {
        resource_limit->stream_max_M[i] = rsrc_lmt["stream_max_gop_M"][i].get<int>(0);
      } else {
        resource_limit->stream_max_M[i] = 0;
      }
      if (rsrc_lmt["stream_max_gop_N"][i].exists()) {
        resource_limit->stream_max_N[i] = rsrc_lmt["stream_max_gop_N"][i].get<int>(0);
      } else {
        resource_limit->stream_max_N[i] = 0;
      }
      if (rsrc_lmt["stream_long_ref_possible"][i].exists()) {
        resource_limit->stream_long_ref_possible[i] = rsrc_lmt["stream_long_ref_possible"][i].get<bool>(false);
      } else {
        resource_limit->stream_long_ref_possible[i] = false;
      }
    }
    if (rsrc_lmt["rotate_possible"].exists()) {
      resource_limit->rotate_possible = rsrc_lmt["rotate_possible"].get<bool>(false);
    } else {
      resource_limit->rotate_possible = false;
    }
    if (rsrc_lmt["raw_capture_possible"].exists()) {
      resource_limit->raw_capture_possible = rsrc_lmt["raw_capture_possible"].get<bool>(false);
    } else {
      resource_limit->raw_capture_possible = false;
    }
    if (rsrc_lmt["vout_swap_possible"].exists()) {
      resource_limit->vout_swap_possible = rsrc_lmt["vout_swap_possible"].get<bool>(false);
    } else {
      resource_limit->vout_swap_possible = false;
    }
    if (rsrc_lmt["lens_warp_possible"].exists()) {
      resource_limit->lens_warp_possible = rsrc_lmt["lens_warp_possible"].get<bool>(false);
    } else {
      resource_limit->lens_warp_possible = false;
    }
    if (rsrc_lmt["enc_from_raw_possible"].exists()) {
      resource_limit->enc_from_raw_possible = rsrc_lmt["enc_from_raw_possible"].get<bool>(false);
    } else {
      resource_limit->enc_from_raw_possible = false;
    }
    if (rsrc_lmt["mixer_a_possible"].exists()) {
      resource_limit->mixer_a_possible = rsrc_lmt["mixer_a_possible"].get<bool>(false);
    } else {
      resource_limit->mixer_a_possible = false;
    }
    if (rsrc_lmt["mixer_b_possible"].exists()) {
      resource_limit->mixer_b_possible = rsrc_lmt["mixer_b_possible"].get<bool>(false);
    } else {
      resource_limit->mixer_b_possible = false;
    }
    if (rsrc_lmt["raw_max_width"].exists()) {
      resource_limit->raw_max_width = rsrc_lmt["raw_max_width"].get<int>(0);
    } else {
      resource_limit->raw_max_width = 0;
    }
    if (rsrc_lmt["raw_max_height"].exists()) {
      resource_limit->raw_max_height = rsrc_lmt["raw_max_height"].get<int>(0);
    } else {
      resource_limit->raw_max_height = 0;
    }
    if (rsrc_lmt["max_warp_input_width"].exists()) {
      resource_limit->max_warp_input_width = rsrc_lmt["max_warp_input_width"].get<int>(0);
    } else {
      resource_limit->max_warp_input_width = 0;
    }
    if (rsrc_lmt["max_warp_output_width"].exists()) {
      resource_limit->max_warp_output_width = rsrc_lmt["max_warp_output_width"].get<int>(0);
    } else {
      resource_limit->max_warp_output_width = 0;
    }
    if (rsrc_lmt["max_padding_width"].exists()) {
      resource_limit->max_padding_width = rsrc_lmt["max_padding_width"].get<int>(0);
    } else {
      resource_limit->max_padding_width = 0;
    }
    if (rsrc_lmt["v_warped_main_max_width"].exists()) {
      resource_limit->v_warped_main_max_width = rsrc_lmt["v_warped_main_max_width"].get<int>(0);
    } else {
      resource_limit->v_warped_main_max_width = 0;
    }
    if (rsrc_lmt["v_warped_main_max_height"].exists()) {
      resource_limit->v_warped_main_max_height = rsrc_lmt["v_warped_main_max_height"].get<int>(0);
    } else {
      resource_limit->v_warped_main_max_height = 0;
    }
    if (rsrc_lmt["enc_dummy_latency"].exists()) {
      resource_limit->enc_dummy_latency = rsrc_lmt["enc_dummy_latency"].get<int>(0);
    } else {
      resource_limit->enc_dummy_latency = 0;
    }
    if (rsrc_lmt["idsp_upsample_type"].exists()) {
      resource_limit->idsp_upsample_type = rsrc_lmt["idsp_upsample_type"].get<int>(0);
    } else {
      resource_limit->idsp_upsample_type = 0;
    }
    if (rsrc_lmt["debug_iso_type"].exists()) {
      resource_limit->debug_iso_type = rsrc_lmt["debug_iso_type"].get<int>(1);
    } else {
      resource_limit->debug_iso_type = -1;
    }
    if (rsrc_lmt["debug_stitched"].exists()) {
      resource_limit->debug_stitched = rsrc_lmt["debug_stitched"].get<int>(1);
    } else {
      resource_limit->debug_stitched = -1;
    }
    if (rsrc_lmt["debug_chip_id"].exists()) {
      resource_limit->debug_chip_id = rsrc_lmt["debug_chip_id"].get<int>(1);
    } else {
      resource_limit->debug_chip_id = -1;
    }
  } while (0);

  delete config;
  return result;
}
/************  AM Resource Limit Config Ends***********************************/


/************  AM Encode Device Config starts*********************************/

AMEncodeDeviceConfig::AMEncodeDeviceConfig(AMEncodeDevice *device) :
            m_buffer_alloc_style(AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_ONE_TO_ONE),
            m_buffer_changed(false),
            m_stream_format_changed(false),
            m_stream_config_changed(false),
            m_lbr_config_changed(false),
            m_osd_overlay_config_changed(false),
            m_is_idle_cycle_required(false),
            m_encode_device(device)
{
  memset(&m_loaded, 0, sizeof(m_loaded));
  memset(&m_using, 0, sizeof(m_using));
}

AMEncodeDeviceConfig::~AMEncodeDeviceConfig()
{
}

static AM_ENCODE_STREAM_TYPE stream_type_str_to_enum(const char *type_str)
{
  AM_ENCODE_STREAM_TYPE type = AM_ENCODE_STREAM_TYPE_NONE;
  if (type_str) {
    if (is_str_equal(type_str, "h264")) {
      type = AM_ENCODE_STREAM_TYPE_H264;
    } else if (is_str_equal(type_str, "mjpeg")) {
      type = AM_ENCODE_STREAM_TYPE_MJPEG;
    } else {
      type = AM_ENCODE_STREAM_TYPE_NONE;
    }
  }
  return type;
}

AM_RESULT AMEncodeDeviceConfig::load_stream_format()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeStreamFormatAll stream_format;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  uint32_t i;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(STREAM_FORMAT_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create stream format AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& format = *config;
    memset(&stream_format, 0, sizeof(stream_format));

    for (i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      //stream format
      if (format[i]["type"].exists()) {
        tmp = format[i]["type"].get<std::string>("");
        stream_format.encode_format[i].type =
            stream_type_str_to_enum(tmp.c_str());
      } else {
        //set to none by default
        stream_format.encode_format[i].type = AM_ENCODE_STREAM_TYPE_NONE;
      }

      //stream format
      if (format[i]["source"].exists()) {
        stream_format.encode_format[i].source =
            AM_ENCODE_SOURCE_BUFFER_ID(format[i]["source"].get<int>(0));
      } else {
        stream_format.encode_format[i].source = AM_ENCODE_SOURCE_BUFFER_MAIN;
      }

      //stream frame_factor (fps)
      if (format[i]["frame_factor"].exists()) {
        tmp = format[i]["frame_factor"].get<std::string>("");
        stream_format.encode_format[i].fps =
            stream_frame_factor_str_to_fps(tmp.c_str());
      } else {
        stream_format.encode_format[i].fps = {1, 1};
      }

      //stream width
      if (format[i]["width"].exists()) {
        stream_format.encode_format[i].width = format[i]["width"].get<int>(0);
      } else {
        stream_format.encode_format[i].width = 0;
      }

      //stream height
      if (format[i]["height"].exists()) {
        stream_format.encode_format[i].height = format[i]["height"].get<int>(0);
      } else {
        stream_format.encode_format[i].height = 0;
      }

      //stream offset_x
      if (format[i]["offset_x"].exists()) {
        stream_format.encode_format[i].offset_x =
            format[i]["offset_x"].get<int>(0);
      } else {
        stream_format.encode_format[i].offset_x = 0;
      }

      //stream offset_y
      if (format[i]["offset_y"].exists()) {
        stream_format.encode_format[i].offset_y =
            format[i]["offset_y"].get<int>(0);
      } else {
        stream_format.encode_format[i].offset_y = 0;
      }

      //stream hflip
      if (format[i]["hflip"].exists()) {
        stream_format.encode_format[i].hflip =
            format[i]["hflip"].get<bool>(false);
      } else {
        stream_format.encode_format[i].hflip = false;
      }

      //stream vflip
      if (format[i]["vflip"].exists()) {
        stream_format.encode_format[i].vflip =
            format[i]["vflip"].get<bool>(false);
      } else {
        stream_format.encode_format[i].vflip = false;
      }

      //stream rotate_90_ccw
      if (format[i]["rotate_90_ccw"].exists()) {
        stream_format.encode_format[i].rotate_90_ccw =
            format[i]["rotate_90_ccw"].get<bool>(false);
      } else {
        stream_format.encode_format[i].rotate_90_ccw = false;
      }

      //stream enable
      if (format[i]["enable"].exists()) {
        stream_format.encode_format[i].enable =
            format[i]["enable"].get<bool>(false);
      } else {
        stream_format.encode_format[i].enable = false;
      }
    } //end for

    memcpy(&m_loaded.stream_format, &stream_format, sizeof(m_loaded.stream_format));
  } while (0);

  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_stream_format()
{
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    INFO("AMEncodeDeviceConfig: save config(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(STREAM_FORMAT_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open stream format AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& format = *config;
    AMEncodeStreamFormatAll *stream_format = &m_using.stream_format;

    for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      format[i]["type"] =
          stream_format_enum_to_str(stream_format->encode_format[i].type);
      format[i]["source"] =
          (int)stream_format->encode_format[i].source;
      format[i]["frame_factor"] =
          stream_fps_to_str(&(stream_format->encode_format[i].fps));
      format[i]["width"] =  stream_format->encode_format[i].width;
      format[i]["height"] =  stream_format->encode_format[i].height;
      format[i]["offset_x"] =  stream_format->encode_format[i].offset_x;
      format[i]["offset_y"] =  stream_format->encode_format[i].offset_y;
      format[i]["hflip"] =  stream_format->encode_format[i].hflip;
      format[i]["vflip"] =  stream_format->encode_format[i].vflip;
      format[i]["rotate_90_ccw"] =
          stream_format->encode_format[i].rotate_90_ccw;
      format[i]["enable"] =  stream_format->encode_format[i].enable;
    }
    if (!format.save()){
      ERROR("AMEncodeDeviceConfig:  failed to save stream format\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while(0);
  //delete config class
  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::load_stream_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeStreamConfigAll stream_config;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  uint32_t i;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(STREAM_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create stream config AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& cfg = *config;
    memset(&stream_config, 0, sizeof(stream_config));

    for (i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      //h264_config.gop_model
      if (cfg[i]["h264_config"]["gop_model"].exists()) {
        stream_config.encode_config[i].h264_config.gop_model =
            AM_ENCODE_H264_GOP_MODEL(cfg[i]["h264_config"]["gop_model"].
                                     get<int>(0));
      } else {
        stream_config.encode_config[i].h264_config.gop_model =
            AM_ENCODE_H264_GOP_MODEL_SIMPLE;
      }

      //h264_config.bitrate_control
      if (cfg[i]["h264_config"]["bitrate_control"].exists()) {
        tmp = cfg[i]["h264_config"]["bitrate_control"].get<std::string>("");
        stream_config.encode_config[i].h264_config.bitrate_control =
            stream_bitrate_control_str_to_enum(tmp.c_str());
      } else {
        stream_config.encode_config[i].h264_config.bitrate_control =
            AM_ENCODE_H264_RC_CBR;
      }

      //h264_config.profile_level
      if (cfg[i]["h264_config"]["profile_level"].exists()) {
        tmp = cfg[i]["h264_config"]["profile_level"].get<std::string>("");
        stream_config.encode_config[i].h264_config.profile_level =
            stream_profile_str_to_enum(tmp.c_str());
      } else {
        stream_config.encode_config[i].h264_config.profile_level =
            AM_ENCODE_H264_PROFILE_MAIN;
      }

      //au_type
      if (cfg[i]["h264_config"]["au_type"].exists()) {
        stream_config.encode_config[i].h264_config.au_type =
            AM_ENCODE_H264_AU_TYPE(cfg[i]["h264_config"]["au_type"].
                                   get<int>(0));
      } else {
        stream_config.encode_config[i].h264_config.au_type =
            AM_ENCODE_H264_AU_TYPE_NO_AUD_NO_SEI;
      }

      //H.264 chroma_format
      if (cfg[i]["h264_config"]["chroma_format"].exists()) {

        tmp = cfg[i]["h264_config"]["chroma_format"].get<std::string>("");
        stream_config.encode_config[i].h264_config.chroma_format =
            stream_chroma_str_to_enum(tmp.c_str());

      } else {
        stream_config.encode_config[i].h264_config.chroma_format =
            AM_ENCODE_CHROMA_FORMAT_YUV420;
      }

      //M
      if (cfg[i]["h264_config"]["M"].exists()) {
        stream_config.encode_config[i].h264_config.M =
            cfg[i]["h264_config"]["M"].get<int>(1);
      } else {
        stream_config.encode_config[i].h264_config.M = 1;
      }

      //N
      if (cfg[i]["h264_config"]["N"].exists()) {
        stream_config.encode_config[i].h264_config.N =
            cfg[i]["h264_config"]["N"].get<int>(30);
      } else {
        stream_config.encode_config[i].h264_config.N = 30;
      }

      //idr_interval
      if (cfg[i]["h264_config"]["idr_interval"].exists()) {
        stream_config.encode_config[i].h264_config.idr_interval =
            cfg[i]["h264_config"]["idr_interval"].get<int>(4);
      } else {
        stream_config.encode_config[i].h264_config.idr_interval = 4;
      }

      //target_bitrate
      if (cfg[i]["h264_config"]["target_bitrate"].exists()) {
        stream_config.encode_config[i].h264_config.target_bitrate =
            cfg[i]["h264_config"]["target_bitrate"].get<int>(1000000);
      } else {
        stream_config.encode_config[i].h264_config.target_bitrate = 1000000;
      }

      //mv threshold
      if (cfg[i]["h264_config"]["mv_threshold"].exists()) {
        stream_config.encode_config[i].h264_config.mv_threshold =
            cfg[i]["h264_config"]["mv_threshold"].get<int>(0);
      } else {
        stream_config.encode_config[i].h264_config.mv_threshold = 0;
      }

      if (cfg[i]["h264_config"]["encode_improve"].exists()) {
        stream_config.encode_config[i].h264_config.encode_improve =
            cfg[i]["h264_config"]["encode_improve"].get<unsigned>(0);
      } else {
        stream_config.encode_config[i].h264_config.encode_improve = 0;
      }

      if (cfg[i]["h264_config"]["multi_ref_p"].exists()) {
        stream_config.encode_config[i].h264_config.multi_ref_p =
            cfg[i]["h264_config"]["multi_ref_p"].get<unsigned>(0);
      } else {
        stream_config.encode_config[i].h264_config.multi_ref_p = 0;
      }

      if (cfg[i]["h264_config"]["long_term_intvl"].exists()) {
        stream_config.encode_config[i].h264_config.long_term_intvl =
            cfg[i]["h264_config"]["long_term_intvl"].get<unsigned>(4);
      } else {
        stream_config.encode_config[i].h264_config.long_term_intvl = 4;
      }

      //mjpeg quality
      if (cfg[i]["mjpeg_config"]["quality"].exists()) {
        stream_config.encode_config[i].mjpeg_config.quality =
            cfg[i]["mjpeg_config"]["quality"].get<int>(60);
      } else {
        stream_config.encode_config[i].mjpeg_config.quality = 60;
      }

      //MJPEG chroma_format
      if (cfg[i]["mjpeg_config"]["chroma_format"].exists()) {

        tmp = cfg[i]["mjpeg_config"]["chroma_format"].get<std::string>("");
        stream_config.encode_config[i].mjpeg_config.chroma_format =
            stream_chroma_str_to_enum(tmp.c_str());
      } else {
        stream_config.encode_config[i].mjpeg_config.chroma_format =
            AM_ENCODE_CHROMA_FORMAT_YUV420;
      }

      //apply
      if (cfg[i]["apply"].exists()) {
        stream_config.encode_config[i].apply =
            cfg[i]["apply"].get<bool>(true);
      } else {
        stream_config.encode_config[i].apply = true;
      }
    } //end for

    memcpy(&m_loaded.stream_config, &stream_config, sizeof(m_loaded.stream_config));
  } while (0);

  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_stream_config()
{
  uint32_t i;
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    INFO("AMEncodeDeviceConfig: save config(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(STREAM_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open stream format AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& cfg = *config;
    AMEncodeStreamConfigAll *stream_cfg = &m_using.stream_config;

    for (i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      cfg[i]["h264_config"]["gop_model"] =
          (int) stream_cfg->encode_config[i].h264_config.gop_model;
      cfg[i]["h264_config"]["bitrate_control"] =
          stream_bitrate_control_enum_to_str(stream_cfg->encode_config[i].\
                                             h264_config.bitrate_control);
      cfg[i]["h264_config"]["profile_level"] =
          stream_profile_level_enum_to_str(stream_cfg->encode_config[i].\
                                           h264_config.profile_level);
      cfg[i]["h264_config"]["au_type"] =
          (int) stream_cfg->encode_config[i].h264_config.au_type;
      cfg[i]["h264_config"]["M"] = stream_cfg->encode_config[i].h264_config.M;
      cfg[i]["h264_config"]["N"] = stream_cfg->encode_config[i].h264_config.N;
      cfg[i]["h264_config"]["idr_interval"] =
          stream_cfg->encode_config[i].h264_config.idr_interval;
      cfg[i]["h264_config"]["target_bitrate"] =
          stream_cfg->encode_config[i].h264_config.target_bitrate;
      cfg[i]["h264_config"]["mv_threshold"] =
          stream_cfg->encode_config[i].h264_config.mv_threshold;
      cfg[i]["h264_config"]["encode_improve"] =
          stream_cfg->encode_config[i].h264_config.encode_improve;
      cfg[i]["h264_config"]["multi_ref_p"] =
          stream_cfg->encode_config[i].h264_config.multi_ref_p;
      cfg[i]["h264_config"]["long_term_intvl"] =
          stream_cfg->encode_config[i].h264_config.long_term_intvl;
      cfg[i]["h264_config"]["chroma_format"] =
          stream_chroma_to_str(stream_cfg->encode_config[i].\
                               h264_config.chroma_format);
      cfg[i]["mjpeg_config"]["quality"] =
          stream_cfg->encode_config[i].mjpeg_config.quality;
      cfg[i]["mjpeg_config"]["chroma_format"] =
          stream_chroma_to_str(stream_cfg->encode_config[i].\
                               mjpeg_config.chroma_format);
    }
    if (!cfg.save()) {
      ERROR("AMEncodeDeviceConfig:  failed to save stream config\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);

  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::load_lbr_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeLBRConfig lbr_param;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;

  do {
    std::string tmp;
    uint32_t i;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(LBR_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create LBR AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& lbr_config = *config;
    memset(&lbr_param, 0, sizeof(lbr_param));
    if (lbr_config["LogLevel"].exists()) {
      lbr_param.log_level = lbr_config["LogLevel"].get<uint32_t>(1);
    }
    if (lbr_config["NoiseStatistics"]["NoiseLowThreshold"].exists()) {
      lbr_param.noise_low_threshold = lbr_config
            ["NoiseStatistics"]["NoiseLowThreshold"].get<uint32_t>(1);
    }
    if (lbr_config["NoiseStatistics"]["NoiseHighThreshold"].exists()) {
      lbr_param.noise_high_threshold = lbr_config
            ["NoiseStatistics"]["NoiseHighThreshold"].get<uint32_t>(6);
    }
    if (lbr_config["ProfileBTScaleFactor"]["Static"].exists()) {
      lbr_param.profile_bt_sf[0].numerator = lbr_config
            ["ProfileBTScaleFactor"]["Static"][0].get<uint32_t>(1);
      lbr_param.profile_bt_sf[0].denominator = lbr_config
            ["ProfileBTScaleFactor"]["Static"][1].get<uint32_t>(1);
    }
    if (lbr_config["ProfileBTScaleFactor"]["SmallMotion"].exists()) {
      lbr_param.profile_bt_sf[1].numerator = lbr_config
            ["ProfileBTScaleFactor"]["SmallMotion"][0].get<uint32_t>(1);
      lbr_param.profile_bt_sf[1].denominator = lbr_config
            ["ProfileBTScaleFactor"]["SmallMotion"][1].get<uint32_t>(1);
    }
    if (lbr_config["ProfileBTScaleFactor"]["BigMotion"].exists()) {
      lbr_param.profile_bt_sf[2].numerator = lbr_config
            ["ProfileBTScaleFactor"]["BigMotion"][0].get<uint32_t>(1);
      lbr_param.profile_bt_sf[2].denominator = lbr_config
            ["ProfileBTScaleFactor"]["BigMotion"][1].get<uint32_t>(1);
    }
    if (lbr_config["ProfileBTScaleFactor"]["LowLight"].exists()) {
      lbr_param.profile_bt_sf[3].numerator = lbr_config
            ["ProfileBTScaleFactor"]["LowLight"][0].get<uint32_t>(1);
      lbr_param.profile_bt_sf[3].denominator = lbr_config
            ["ProfileBTScaleFactor"]["LowLight"][1].get<uint32_t>(1);
    }
    if (lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"].exists()) {
      lbr_param.profile_bt_sf[4].numerator = lbr_config
            ["ProfileBTScaleFactor"]["BigMotionWithFD"][0].get<uint32_t>(1);
      lbr_param.profile_bt_sf[4].denominator = lbr_config
            ["ProfileBTScaleFactor"]["BigMotionWithFD"][1].get<uint32_t>(1);
    }
    if (lbr_config["StreamConfig"].exists()) {
      for (i = 0; i < AM_STREAM_MAX_NUM; i++) {
        if (lbr_config["StreamConfig"][i]["EnableLBR"].exists()) {
          lbr_param.stream_params[i].enable_lbr = lbr_config
                ["StreamConfig"][i]["EnableLBR"].get<bool>(true);
        }
        if (lbr_config["StreamConfig"][i]["MotionControl"].exists()) {
          lbr_param.stream_params[i].motion_control = lbr_config
                ["StreamConfig"][i]["MotionControl"].get<bool>(true);
        }
        if (lbr_config["StreamConfig"][i]["NoiseControl"].exists()) {
          lbr_param.stream_params[i].noise_control = lbr_config
                ["StreamConfig"][i]["NoiseControl"].get<bool>(true);
        }
        if (lbr_config["StreamConfig"][i]["FrameDrop"].exists()) {
          lbr_param.stream_params[i].frame_drop = lbr_config
                ["StreamConfig"][i]["FrameDrop"].get<bool>(true);
        }
        if (lbr_config["StreamConfig"][i]["AutoBitrateTarget"].exists()) {
          lbr_param.stream_params[i].auto_target = lbr_config
                ["StreamConfig"][i]["AutoBitrateTarget"].get<bool>(false);
        }
        if (lbr_config["StreamConfig"][i]["BitrateCeiling"].exists()) {
          lbr_param.stream_params[i].bitrate_ceiling = lbr_config
                ["StreamConfig"][i]["BitrateCeiling"].get<uint32_t>(142);
        }
      }
    }
    memcpy(&m_loaded.lbr_config, &lbr_param, sizeof(m_loaded.lbr_config));

  } while (0);

  //delete config class
  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_lbr_config()
{
  INFO("AMEncodeDeviceConfig: save lbr config (m_using) to file \n");
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  uint32_t i;
  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(LBR_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open LBR AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& lbr_config = *config;
    AMEncodeLBRConfig *lbr_param = &m_using.lbr_config;
    lbr_config["LogLevel"] = lbr_param->log_level;
    lbr_config["NoiseStatistics"]["NoiseLowThreshold"] =
          lbr_param->noise_low_threshold;
    lbr_config["NoiseStatistics"]["NoiseHighThreshold"] =
          lbr_param->noise_high_threshold;
    lbr_config["ProfileBTScaleFactor"]["Static"][0] =
          lbr_param->profile_bt_sf[0].numerator;
    lbr_config["ProfileBTScaleFactor"]["Static"][1] =
          lbr_param->profile_bt_sf[0].denominator;
    lbr_config["ProfileBTScaleFactor"]["SmallMotion"][0] =
          lbr_param->profile_bt_sf[1].numerator;
    lbr_config["ProfileBTScaleFactor"]["SmallMotion"][1] =
          lbr_param->profile_bt_sf[1].denominator;
    lbr_config["ProfileBTScaleFactor"]["BigMotion"][0] =
          lbr_param->profile_bt_sf[2].numerator;
    lbr_config["ProfileBTScaleFactor"]["BigMotion"][1] =
          lbr_param->profile_bt_sf[2].denominator;
    lbr_config["ProfileBTScaleFactor"]["LowLight"][0] =
          lbr_param->profile_bt_sf[3].numerator;
    lbr_config["ProfileBTScaleFactor"]["LowLight"][1] =
          lbr_param->profile_bt_sf[3].denominator;
    lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"][0] =
          lbr_param->profile_bt_sf[4].numerator;
    lbr_config["ProfileBTScaleFactor"]["BigMotionWithFD"][1] =
          lbr_param->profile_bt_sf[4].denominator;
    for (i = 0; i < AM_STREAM_MAX_NUM; i++) {
      lbr_config["StreamConfig"][i]["EnableLBR"] =
            lbr_param->stream_params[i].enable_lbr;
      lbr_config["StreamConfig"][i]["MotionControl"] =
            lbr_param->stream_params[i].motion_control;
      lbr_config["StreamConfig"][i]["NoiseControl"] =
            lbr_param->stream_params[i].noise_control;
      lbr_config["StreamConfig"][i]["FrameDrop"] =
            lbr_param->stream_params[i].frame_drop;
      lbr_config["StreamConfig"][i]["AutoBitrateTarget"] =
            lbr_param->stream_params[i].auto_target;
      lbr_config["StreamConfig"][i]["BitrateCeiling"] =
            lbr_param->stream_params[i].bitrate_ceiling;
    }

    if (!lbr_config.save()) {
      ERROR("AMEncodeDeviceConfig: failed to save_config\n");
      result = AM_RESULT_ERR_IO;
      break;
    }

  } while(0);

  //delete config class
  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::load_dptz_warp_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMDPTZWarpConfig dptz_warp_param;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;

  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(DPTZ_WARP_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create DPTZ Warp AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& dptz_warp_config = *config;
    memset(&dptz_warp_param, 0, sizeof(dptz_warp_param));
    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
      if (dptz_warp_config["ptz"][i]["pan_ratio"].exists()) {
        dptz_warp_param.dptz_cfg[i].pan_ratio = dptz_warp_config["ptz"][i]["pan_ratio"]
            .get<float>(0);
      }
      if (dptz_warp_config["ptz"][i]["tilt_ratio"].exists()) {
        dptz_warp_param.dptz_cfg[i].tilt_ratio =
            dptz_warp_config["ptz"][i]["tilt_ratio"].get<float>(0);
      }
      if (dptz_warp_config["ptz"][i]["zoom_ratio"].exists()) {
        dptz_warp_param.dptz_cfg[i].zoom_ratio =
            dptz_warp_config["ptz"][i]["zoom_ratio"].get<float>(1.0);
      }
    }
    if (dptz_warp_config["warp"]["proj_mode"].exists()) {
      dptz_warp_param.warp_cfg.proj_mode = dptz_warp_config["warp"]["proj_mode"]
          .get<uint32_t>(0);
    }
    if (dptz_warp_config["warp"]["warp_mode"].exists()) {
      dptz_warp_param.warp_cfg.warp_mode = dptz_warp_config["warp"]["warp_mode"]
          .get<uint32_t>(1);
    }
    if (dptz_warp_config["warp"]["ldc_strength"].exists()) {
      dptz_warp_param.warp_cfg.ldc_strength =
          dptz_warp_config["warp"]["ldc_strength"].get<float>(0.0);
    }
    if (dptz_warp_config["warp"]["pano_hfov_degree"].exists()) {
      dptz_warp_param.warp_cfg.pano_hfov_degree =
          dptz_warp_config["warp"]["pano_hfov_degree"].get<float>(180.0);
    }

    memcpy(&m_loaded.dptz_warp_config, &dptz_warp_param, sizeof(m_loaded.dptz_warp_config));
  } while (0);

  //delete config class
  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_dptz_warp_config()
{
  INFO("AMEncodeDeviceConfig: save dptz warp config (m_using) to file \n");
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(DPTZ_WARP_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open DPTZ Warp AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& dptz_warp_config = *config;
    AMDPTZWarpConfig *dptz_warp_param = &m_using.dptz_warp_config;
    dptz_warp_config["warp"]["proj_mode"] = (uint32_t) dptz_warp_param->warp_cfg
        .proj_mode;
    dptz_warp_config["warp"]["warp_mode"] = (uint32_t) dptz_warp_param->warp_cfg
        .warp_mode;
    dptz_warp_config["warp"]["ldc_strength"] = dptz_warp_param->warp_cfg
        .ldc_strength;
    dptz_warp_config["warp"]["pano_hfov_degree"] = dptz_warp_param->warp_cfg
        .pano_hfov_degree;

    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
      dptz_warp_config["ptz"][i]["pan_ratio"] = dptz_warp_param->dptz_cfg[i].pan_ratio;
      dptz_warp_config["ptz"][i]["tilt_ratio"] =
          dptz_warp_param->dptz_cfg[i].tilt_ratio;
      dptz_warp_config["ptz"][i]["zoom_ratio"] =
          dptz_warp_param->dptz_cfg[i].zoom_ratio;
    }

    if (!dptz_warp_config.save()) {
      ERROR("AMEncodeDeviceConfig: failed to save_config\n");
      result = AM_RESULT_ERR_IO;
      break;
    }

  } while (0);

  //delete config class
  delete config;
  return result;
}


AM_RESULT AMEncodeDeviceConfig::load_osd_overlay_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMOSDOverlayConfig osd = {0};
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;

  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(OSD_OVERLAY_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create OSD AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& osd_config = *config;

    for (uint32_t i=0; i<AM_STREAM_MAX_NUM; ++i) {
      uint32_t area_num = osd_config[i]["n"].get<uint32_t>(0);
      osd.osd_cfg[i].area_num = area_num;

      for (uint32_t j=0; j<area_num; ++j) {
        AMOSDAttribute *attr = &(osd.osd_cfg[i].attribute[j]);
        attr->type = (AMOSDType)osd_config[i][j]["type"].get<uint32_t>(0);
        attr->enable_rotate = osd_config[i][j]["rotate"].get<uint32_t>(1);

        if ((AM_OSD_TYPE_GENERIC_STRING == attr->type) ||
            (AM_OSD_TYPE_TIME_STRING == attr->type)) {
          if (osd_config[i][j]["str"].exists()) {
            tmp = osd_config[i][j]["str"].get<std::string>("");
            memcpy(attr->osd_text_box.osd_string, tmp.c_str(), tmp.size());
            INFO("str = %s", attr->osd_text_box.osd_string);
          }
          if (osd_config[i][j]["font_name"].exists()) {
            tmp = osd_config[i][j]["font_name"].get<std::string>("");
            memcpy(attr->osd_text_box.font.ttf_name, tmp.c_str(), tmp.size());
          }
          if (osd_config[i][j]["font_size"].exists()) {
            attr->osd_text_box.font.size =
                osd_config[i][j]["font_size"].get<uint32_t>(0);
          }
          if (osd_config[i][j]["font_color"].exists()) {
            uint32_t color = osd_config[i][j]["font_color"].get<uint32_t>(0);
            if (color < 8) {
              attr->osd_text_box.font_color.id = color;
            } else {
              attr->osd_text_box.font_color.id = 8;
              attr->osd_text_box.font_color.color.v =
                  (uint8_t)((color >> 24) & 0xff);
              attr->osd_text_box.font_color.color.u =
                  (uint8_t)((color >> 16) & 0xff);
              attr->osd_text_box.font_color.color.y =
                  (uint8_t)((color >> 8) & 0xff);
              attr->osd_text_box.font_color.color.a =
                  (uint8_t)(color & 0xff);
            }
          }
          if (osd_config[i][j]["font_outline_w"].exists()) {
            attr->osd_text_box.font.outline_width =
                osd_config[i][j]["font_outline_w"].get<uint32_t>(0);
          }
          if (osd_config[i][j]["font_ver_bold"].exists()) {
            attr->osd_text_box.font.ver_bold =
                osd_config[i][j]["font_ver_bold"].get<uint32_t>(0);
          }
          if (osd_config[i][j]["font_hor_bold"].exists()) {
            attr->osd_text_box.font.hor_bold =
                osd_config[i][j]["font_hor_bold"].get<uint32_t>(0);
          }
          if (osd_config[i][j]["font_italic"].exists()) {
            attr->osd_text_box.font.italic =
                osd_config[i][j]["font_italic"].get<uint32_t>(0);
          }
        } else if (AM_OSD_TYPE_PICTURE == attr->type) {
          if (osd_config[i][j]["ckey"].exists()) {
            uint32_t color = osd_config[i][j]["ckey"].get<uint32_t>(0);
            attr->osd_pic.colorkey[0].color.v =
                (uint8_t)((color >> 24) & 0xff);
            attr->osd_pic.colorkey[0].color.u =
                (uint8_t)((color >> 16) & 0xff);
            attr->osd_pic.colorkey[0].color.y =
                (uint8_t)((color >> 8) & 0xff);
            attr->osd_pic.colorkey[0].color.a =
                (uint8_t)(color & 0xff);
          }
          if (osd_config[i][j]["crange"].exists()) {
            attr->osd_pic.colorkey[0].range = osd_config[i][j]["crange"].get<uint32_t>(0);
          }
          if (osd_config[i][j]["pic"].exists()) {
            tmp = osd_config[i][j]["pic"].get<std::string>("");
            memcpy(attr->osd_pic.filename, tmp.c_str(), tmp.size());
          }
        }
        AMOSDArea *area = &(osd.osd_cfg[i].overlay_area[j]);
        area->enable = osd_config[i][j]["enable"].get<uint32_t>(0);
        area->area_position.style = (AMOSDLayoutStyle)
            osd_config[i][j]["layout"].get<uint32_t>(0);
        if (osd_config[i][j]["w"].exists()) {
          area->width = osd_config[i][j]["w"].get<uint32_t>(0);
        }
        if (osd_config[i][j]["h"].exists()) {
          area->height = osd_config[i][j]["h"].get<uint32_t>(0);
        }
        if (osd_config[i][j]["x"].exists()) {
          area->area_position.offset_x =
              osd_config[i][j]["x"].get<uint32_t>(0);
        }
        if (osd_config[i][j]["y"].exists()) {
          area->area_position.offset_y =
              osd_config[i][j]["y"].get<uint32_t>(0);
        }
      }
    }

    memcpy(&m_loaded.osd_overlay_config, &osd, sizeof(m_loaded.osd_overlay_config));
  } while (0);

  //delete config class
  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_osd_overlay_config()
{
  INFO("AMEncodeDeviceConfig: save overlay config (m_using) to file \n");
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(OSD_OVERLAY_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open overlay AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& osd_config = *config;
    AMOSDOverlayConfig  *osd = &m_using.osd_overlay_config;

    for (uint32_t i=0; i<AM_STREAM_MAX_NUM; ++i) {
      uint32_t area_num = osd->osd_cfg[i].area_num;
      osd_config[i]["n"] = area_num;

      for (uint32_t j=0; j<OSD_AREA_MAX_NUM; ++j) {
        AMOSDAttribute *attr = &osd->osd_cfg[i].attribute[j];
        INFO("stream: %d area :%d \n",i, j);
        osd_config[i][j]["type"] = (uint32_t)attr->type;
        osd_config[i][j]["rotate"] = (uint32_t)attr->enable_rotate;
        if ((AM_OSD_TYPE_GENERIC_STRING == attr->type) ||
            (AM_OSD_TYPE_TIME_STRING == attr->type)) {
          tmp = attr->osd_text_box.osd_string;
          osd_config[i][j]["str"] = tmp;
          tmp = attr->osd_text_box.font.ttf_name;
          osd_config[i][j]["font_name"] = tmp;
          osd_config[i][j]["font_size"] = attr->osd_text_box.font.size;
          uint32_t id = attr->osd_text_box.font_color.id;
          if (id < 8) {
            osd_config[i][j]["font_color"] = id;
          } else {
            AMOSDCLUT color = attr->osd_text_box.font_color.color;
            osd_config[i][j]["font_color"] = (uint32_t)((color.v << 24) |
                (color.u << 16) | (color.y << 8) | color.a);
          }
          osd_config[i][j]["font_outline_w"] = attr->osd_text_box.font.outline_width;
          osd_config[i][j]["font_ver_bold"] = attr->osd_text_box.font.ver_bold;
          osd_config[i][j]["font_hor_bold"] = attr->osd_text_box.font.hor_bold;
          osd_config[i][j]["font_italic"] = attr->osd_text_box.font.italic;
        } else if (AM_OSD_TYPE_PICTURE == attr->type) {
          AMOSDCLUT color = attr->osd_pic.colorkey[0].color;
          osd_config[i][j]["ckey"] = (uint32_t)((color.v << 24) |
              (color.u << 16) | (color.y << 8) | color.a);
          osd_config[i][j]["crange"] = (uint32_t)attr->osd_pic.colorkey[0].range;
          tmp = attr->osd_pic.filename;
          osd_config[i][j]["pic"] = tmp;
        }

        AMOSDArea *area = &(osd->osd_cfg[i].overlay_area[j]);
        osd_config[i][j]["enable"] = (uint32_t)area->enable;
        osd_config[i][j]["layout"] = (uint32_t)area->area_position.style;
        if (4 == area->area_position.style) {
          osd_config[i][j]["x"] = (uint32_t)area->area_position.offset_x;
          osd_config[i][j]["y"] = (uint32_t)area->area_position.offset_y;
        }
        osd_config[i][j]["w"] = (uint32_t)area->width;
        osd_config[i][j]["h"] = (uint32_t)area->height;
      }
    }

    if (!osd_config.save()) {
      ERROR("AMEncodeDeviceConfig:  failed to save overlay config\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);

  //delete config class
  delete config;
  return result;
}

AMRect AMEncodeDeviceConfig::get_default_input_window_rect(AM_ENCODE_SOURCE_BUFFER_ID buf_id)
{
  AMRect rect;
  AMResolution resolution = {0};
  switch(buf_id)
  {
    case AM_ENCODE_SOURCE_BUFFER_MAIN:
      break;
    case AM_ENCODE_SOURCE_BUFFER_2ND:
    case AM_ENCODE_SOURCE_BUFFER_3RD:
    case AM_ENCODE_SOURCE_BUFFER_4TH:
    default:
      resolution = m_loaded.buffer_format.source_buffer_format[0].size;
      break;
  }
  rect.x = 0;
  rect.y = 0;
  rect.width = resolution.width;
  rect.height = resolution.height;
  return rect;
}

AMRect AMEncodeDeviceConfig::buffer_input_str_to_rect(
    const char *input_str,
    AM_ENCODE_SOURCE_BUFFER_ID buf_id)
{
  AMRect rect = {0, 0, 0, 0};
  if (input_str) {
    if (is_str_equal(input_str, "auto")) {
      return get_default_input_window_rect(buf_id);
    } else {
      ERROR("buffer input str not recognized %s\n", input_str);
    }
  }
  return rect;
}

static AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE alloc_stype_str_to_enum(
    const char *str)
{
  AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE alloc_style =
      AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_ONE_TO_ONE;

  if (str) {
    if (is_str_equal(str, "auto")) {
      alloc_style = AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_ONE_TO_ONE;
    } else if (is_str_equal(str, "manual")) {
      alloc_style = AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL;
    }
  }

  return alloc_style;
}

static std::string alloc_stype_enum_to_str(
    AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE style)
{
  std::string ret;
  switch (style) {
    case AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL:
      ret = "manual";
      break;
    default:
      ret = "auto";
      break;
  }
  return ret;
}

AM_RESULT AMEncodeDeviceConfig::load_buffer_format()
{
  AM_RESULT result = AM_RESULT_OK;
  AMEncodeSourceBufferFormatAll buffer_format;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  uint32_t i;
  AMConfig *config = NULL;

  do {
    std::string tmp;

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(SOURCE_BUFFER_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to create buffer format AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& buffer = *config;
    memset(&buffer_format, 0, sizeof(buffer_format));

    if (buffer["alloc_style"].exists()) {
      tmp = buffer["alloc_style"].get<std::string>("auto");
      set_buffer_alloc_style(alloc_stype_str_to_enum(tmp.c_str()));
    }

    for (i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; i ++) {
      //buffer.type
      if (buffer[i]["type"].exists()) {
        tmp = buffer[i]["type"].get<std::string>("");
        buffer_format.source_buffer_format[i].type =
            buffer_type_str_to_enum(tmp.c_str());
      } else {
        buffer_format.source_buffer_format[i].type =
            AM_ENCODE_SOURCE_BUFFER_TYPE_OFF;
      }

      //buffer.input_crop
      if (buffer[i]["input_crop"].exists()) {
        buffer_format.source_buffer_format[i].input_crop =
            buffer[i]["input_crop"].get<bool>(false);
      } else {
        buffer_format.source_buffer_format[i].input_crop = false;
      }

      //buffer.input_window
      if (buffer[i]["input_window"].exists()) {
        //it is array
        if (buffer_format.source_buffer_format[i].input_crop == true) {
          buffer_format.source_buffer_format[i].input.width =
              buffer[i]["input_window"][0].get<int>(0);
          buffer_format.source_buffer_format[i].input.height =
              buffer[i]["input_window"][1].get<int>(0);
          buffer_format.source_buffer_format[i].input.x =
              buffer[i]["input_window"][2].get<int>(0);
          buffer_format.source_buffer_format[i].input.y =
              buffer[i]["input_window"][3].get<int>(0);
        }
      }

      if (buffer_format.source_buffer_format[i].input_crop == false ) {
        buffer_format.source_buffer_format[i].input =
            get_default_input_window_rect(AM_ENCODE_SOURCE_BUFFER_ID(i));
      }

      //buffer.size
      if (buffer[i]["size"].exists()) {
        buffer_format.source_buffer_format[i].size.width =
            buffer[i]["size"][0].get<int>(0);
        buffer_format.source_buffer_format[i].size.height =
            buffer[i]["size"][1].get<int>(0);
      } else {
        buffer_format.source_buffer_format[i].size = {0, 0};
        WARN("buf %d size is configured to be 0x0 \n", i);
      }

      //buffer.prewarp
      if (buffer[i]["prewarp"].exists()) {
        buffer_format.source_buffer_format[i].prewarp =
            buffer[i]["prewarp"].get<bool>(false);
      } else {
        buffer_format.source_buffer_format[i].prewarp = false;
      }

    } //end for

    memcpy(&m_loaded.buffer_format, &buffer_format, sizeof(m_loaded.buffer_format));
  } while(0);

  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::save_buffer_format()
{
  AM_RESULT result = AM_RESULT_OK;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    INFO("AMEncodeDeviceConfig: save buffer format(m_using) to file \n");
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(SOURCE_BUFFER_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMEncodeDeviceConfig: fail to open stream format AMConfig\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& buf = *config;
    AMEncodeSourceBufferFormatAll *buf_fmt = &m_using.buffer_format;

    buf["alloc_style"] = alloc_stype_enum_to_str(m_buffer_alloc_style);
    for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++ i) {
      buf[i]["type"] =
          source_buffer_type_to_str(buf_fmt->source_buffer_format[i].type);
      buf[i]["input_crop"] = buf_fmt->source_buffer_format[i].input_crop;
      buf[i]["size"][0] = buf_fmt->source_buffer_format[i].size.width;
      buf[i]["size"][1] = buf_fmt->source_buffer_format[i].size.height;
      buf[i]["prewarp"] = buf_fmt->source_buffer_format[i].prewarp;

      if (buf_fmt->source_buffer_format[i].input_crop) {
        buf[i]["input_window"][0] =
            buf_fmt->source_buffer_format[i].input.width;
        buf[i]["input_window"][1] =
            buf_fmt->source_buffer_format[i].input.height;
        buf[i]["input_window"][2] =
            buf_fmt->source_buffer_format[i].input.x;
        buf[i]["input_window"][3] =
            buf_fmt->source_buffer_format[i].input.y;
      }
    } //end for

    if (!buf.save()) {
      ERROR("AMEncodeDeviceConfig:  failed to save buffer format\n ");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);

  delete config;
  return result;
}

AM_RESULT AMEncodeDeviceConfig::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    result = load_buffer_format();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  load_buffer_format failed \n");
      break;
    }

    result = load_stream_format();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  load_stream_format failed \n");
      break;
    }

    result = load_stream_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  load_stream_config failed \n");
      break;
    }

    result = load_dptz_warp_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  load_dptz_warp_config failed \n");
      break;
    }

    result = load_lbr_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  load_lbr_config failed \n");
      break;
    }

    INFO("begin to load osd config file!!!");
    result = load_osd_overlay_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDevideConfig:  load_osd_overlay_config failed \n");
      break;
    }
    INFO("load osd config file end!!!");

    m_stream_format_changed = !!memcmp(&m_using.stream_format,
                                       &m_loaded.stream_format,
                                       sizeof(m_using.stream_format));

    m_stream_config_changed = !!memcmp(&m_using.stream_config,
                                       &m_loaded.stream_config,
                                       sizeof(m_using.stream_config));

    m_lbr_config_changed = !!memcmp(&m_using.lbr_config,
                                       &m_loaded.lbr_config,
                                       sizeof(m_using.lbr_config));

    m_buffer_changed = !!memcmp(&m_using.buffer_format,
                                &m_loaded.buffer_format,
                                sizeof(m_using.buffer_format));

    m_dptz_warp_config_changed = !!memcmp(&m_using.dptz_warp_config,
                                &m_loaded.dptz_warp_config,
                                sizeof(m_using.dptz_warp_config));

    m_osd_overlay_config_changed = !!memcmp(&m_using.osd_overlay_config,
                                            &m_loaded.osd_overlay_config,
                                            sizeof(m_using.osd_overlay_config));

    if (m_buffer_changed) {
      m_is_idle_cycle_required = true;
    }
    //result = source_buffer_auto_update();
  } while (0);

  return result;
}

//write m_using configuration into files.
AM_RESULT AMEncodeDeviceConfig::save_config()
{
  INFO("AMEncodeDeviceConfig::save_config\n");
  AM_RESULT result = AM_RESULT_OK;
  do {
    result = save_buffer_format();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  save_buffer_format failed \n");
      break;
    }

    result = save_stream_format();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  save_stream_format failed \n");
      break;
    }

    result = save_stream_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  save_stream_config failed \n");
      break;
    }

    result = save_dptz_warp_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  save_dptz_warp_config failed \n");
      break;
    }

    result = save_lbr_config();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeDeviceConfig:  save_lbr_config failed \n");
      break;
    }

    if (m_loaded.osd_overlay_config.save_flag) {
      result = save_osd_overlay_config();
      if (result != AM_RESULT_OK) {
        ERROR("AMEncodeDeviceConfig:  save_osd_overlay_config failed \n");
        break;
      }
      m_loaded.osd_overlay_config.save_flag = false;
      m_using.osd_overlay_config.save_flag = false;
    }

  } while (0);
  return result;
}

//sync is called after config has been applied, and just to sync current
//using configuration, only at this condition, we can clear flag like
//m_is_idle_cycle_required
AM_RESULT AMEncodeDeviceConfig::sync()
{
  memcpy(&m_using, &m_loaded, sizeof(m_using));
  m_is_idle_cycle_required = false;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeDeviceConfig::set_stream_format(
    AM_STREAM_ID id,
    AMEncodeStreamFormat *stream_format)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

    if (!stream_format) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (((int) id < 0) || ((int) id > AM_STREAM_ID_LAST)) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_loaded.stream_format.encode_format[(int) id] = *stream_format;
  } while (0);

  return result;
}

AM_RESULT AMEncodeDeviceConfig::set_stream_config(
    AM_STREAM_ID id,
    AMEncodeStreamConfig *stream_config)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!stream_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (((int) id < 0) || ((int) id > AM_STREAM_ID_LAST)) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_loaded.stream_config.encode_config[(int) id] = *stream_config;
  } while (0);

  return result;
}

int AMEncodeDeviceConfig::get_enabled_stream_num()
{
  int i;
  int count = 0;
  for (i = 0; i < AM_STREAM_ID_LAST; i ++) {
    if (m_loaded.stream_format.encode_format[i].enable) {
      count ++;
    }
  }

  return count;
}

AMEncodeParamAll* AMEncodeDeviceConfig::get_encode_param()
{
  return &m_loaded;
}


AMResolution g_source_buffer_max_size_default[AM_ENCODE_SOURCE_BUFFER_MAX_NUM] =
{
 {1920,1080}, //main buffer max size
 {720, 576},  //2nd buffer max size
 {1920,1080}, //3rd buffer max size
 {1280,720},  //4th buffer max size
 //{0, 0},    do not consider premain buffer now
};

//Let's assume stream size max has no need to change now,
//and can be supported by IAV.


AM_RESULT AMEncodeDeviceConfig::set_buffer_alloc_style(
    AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE style)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (((int) style < 0)
        || ((int) style > AM_ENCODE_SOURCE_BUFFER_ALLOC_LAST)) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    m_buffer_alloc_style = style;
  } while (0);

  return result;
}

AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE
AMEncodeDeviceConfig::get_buffer_alloc_style()
{
  return m_buffer_alloc_style;
}

bool AMEncodeDeviceConfig::is_idle_cycle_required()
{
  //TODO: calculate variable idle cycle required.
  //calculate condition is base on source buffer change and etc.

  return m_is_idle_cycle_required;
}

/************  AM Encode Device Config ends **********************************/



/************  AM Camera Config starts ***************************************/
AMCameraConfig::AMCameraConfig():
    m_changed(false)
{
  memset(&m_loaded, 0, sizeof(m_loaded));
  memset(&m_using, 0, sizeof(m_using));
}

AMCameraConfig::~AMCameraConfig()
{

}


AM_RESULT AMCameraConfig::load_config(AMEncodeDSPCapability *dsp_cap)
{

  AM_RESULT result = AM_RESULT_OK;

  AMCameraParam param;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {

    std::string tmp= "/etc/oryx/video/features.acs";

    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(FEATURE_CONFIG_FILE);


    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMCameraConfig: fail to load feature config file\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }


    AMConfig& feature = *config;
    memset(&param, 0, sizeof(param));
    if (feature["mode"].exists()) {
      tmp = feature["mode"].get<std::string>("auto");
      param.mode = camera_param_mode_str_to_enum(tmp.c_str());
    } else {
      param.mode = AM_VIDEO_ENCODE_NORMAL_MODE;
    }

    if (feature["hdr"].exists()) {
      tmp = feature["hdr"].get<std::string>("none");
      param.hdr = camera_param_hdr_str_to_enum(tmp.c_str());
    } else {
      param.hdr = AM_HDR_SINGLE_EXPOSURE;
    }

    if (feature["iso"].exists()) {
      tmp = feature["iso"].get<std::string>("normal");
      param.iso = camera_param_iso_str_to_enum(tmp.c_str());
    } else {
      param.iso = AM_IMAGE_PIPELINE_NORMAL_ISO;
    }

    if (feature["dewarp"].exists()) {
      tmp = feature["dewarp"].get<std::string>("none");
      param.dewarp = camera_param_dewarp_str_to_enum(tmp.c_str());
    } else {
      param.dewarp = AM_DEWARP_NONE;
    }

    m_loaded = param;
    m_changed = !!memcmp(&m_using, &m_loaded, sizeof(m_using));

    //convert to DSP cap
    if (dsp_cap) {
      result = camera_param_to_dsp_cap(&m_loaded, dsp_cap);
      if (result!= AM_RESULT_OK)
        break;
    }


  } while (0);

  delete config;


  return result;
}

AM_RESULT AMCameraConfig::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  AMCameraParam* param = &m_using;
  char default_dir[] = DEFAULT_ORYX_CONF_DIR;
  AMConfig *config = NULL;
  do {
    std::string tmp;
    char *oryx_conf_dir = getenv("AMBARELLA_ORYX_CONF_DIR");
    if (!oryx_conf_dir)
      oryx_conf_dir = default_dir;
    tmp = std::string(oryx_conf_dir) + std::string(FEATURE_CONFIG_FILE);
    config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AMCameraConfig: fail to load feature config file\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    AMConfig& feature = *config;
    feature["mode"] = camera_param_mode_enum_to_str(param->mode);
    feature["hdr"] = camera_param_hdr_enum_to_str(param->hdr);
    feature["iso"] = camera_param_iso_enum_to_str(param->iso);
    feature["dewarp"] = camera_param_dewarp_enum_to_str(param->dewarp);

    if (!feature.save()) {
      ERROR("AMCameraConfig : failed to save config \n");
      result = AM_RESULT_ERR_IO;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMCameraConfig::sync()
{
  memcpy(&m_using, &m_loaded, sizeof(m_using));
  m_changed = false;
  return AM_RESULT_OK;
}

AM_RESULT AMCameraConfig::camera_param_to_dsp_cap(
    AMCameraParam *cam_param,
    AMEncodeDSPCapability *dsp_cap)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!dsp_cap || !cam_param) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    dsp_cap->basic_hdr = (cam_param->hdr == AM_HDR_2_EXPOSURE);
    dsp_cap->advanced_hdr = (cam_param->hdr == AM_HDR_3_EXPOSURE);
    dsp_cap->normal_iso = (cam_param->iso == AM_IMAGE_PIPELINE_NORMAL_ISO);
    dsp_cap->normal_plus_iso = (cam_param->iso == AM_IMAGE_PIPELINE_ISO_PLUS);
    dsp_cap->advanced_iso = (cam_param->iso == AM_IMAGE_PIPELINE_ADVANCED_ISO);
    dsp_cap->single_dewarp = (cam_param->dewarp == AM_DEWARP_LDC);
    dsp_cap->multi_dewarp = (cam_param->dewarp == AM_DEWARP_FULL);

  } while (0);
  return result;
}


/************  AM Camera Config ends ***************************************/
