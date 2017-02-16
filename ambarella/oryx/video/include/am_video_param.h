/*******************************************************************************
 * am_video_param.h
 *
 * Histroy:
 * 2014-6-24 - [Louis] created this file for video param parsing
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_PARAM_H_
#define AM_VIDEO_PARAM_H_

#include "am_video_dsp.h"
#include "am_vin_config_if.h"
#include "am_encode_device_config_if.h"

#define MAX_VIDEO_TYPE_STRING_LENGTH  12
#define MAX_VIDEO_MODE_STRING_LENGTH  64
#define MAX_VIDEO_FPS_STRING_LENGTH   12

/* AMVinConfig is for AMVin class data conversion with strings */

struct AMVinParam
{
    AM_VIN_TYPE type;
    AM_VIN_MODE mode;
    AM_VIDEO_FLIP flip;
    AM_VIDEO_FPS  fps;
    AM_VIN_BAYER_PATTERN bayer_pattern;
};

struct AMVinParamAll
{
    AMVinParam  vin[AM_VIN_MAX_NUM];
};

struct AMVoutParam
{
    AM_VOUT_TYPE  type;
    AM_VOUT_MODE  mode;
    AM_VIDEO_FLIP flip;
    AM_VIDEO_ROTATE rotate;
    AM_VIDEO_FPS  fps;
};

struct AMVoutParamAll
{
    AMVoutParam vout[AM_VOUT_MAX_NUM];
};

struct AMResourceLimitParam
{
    uint32_t max_num_encode;
    uint32_t max_num_cap_sources;

    uint32_t buf_max_width[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
    uint32_t buf_max_height[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
    uint32_t stream_max_width[AM_STREAM_MAX_NUM];
    uint32_t stream_max_height[AM_STREAM_MAX_NUM];
    uint32_t stream_max_M[AM_STREAM_MAX_NUM];
    uint32_t stream_max_N[AM_STREAM_MAX_NUM];
    uint32_t stream_max_advanced_quality_model[AM_STREAM_MAX_NUM];
    bool stream_long_ref_possible[AM_STREAM_MAX_NUM];

    /* Writable for different configuration */
    bool rotate_possible;
    bool raw_capture_possible;
    bool vout_swap_possible;
    bool lens_warp_possible;
    bool enc_from_raw_possible;
    bool mixer_a_possible;
    bool mixer_b_possible;

    uint32_t raw_max_width; /* Only for encode from raw feature */
    uint32_t raw_max_height;
    uint32_t max_warp_input_width;
    uint32_t max_warp_output_width;
    uint32_t max_padding_width; // For LDC stitching
    uint32_t v_warped_main_max_width;
    uint32_t v_warped_main_max_height;
    uint32_t enc_dummy_latency;
    uint32_t idsp_upsample_type;

    int32_t  debug_iso_type;
    int32_t  debug_stitched;
    int32_t  debug_chip_id;
};

struct AMEncodeParamAll
{
    //buffer format has contained "buffer enable flag" for each buffer
    AMEncodeSourceBufferFormatAll buffer_format;

    //stream format has contained "stream enable flag" for each stream
    AMEncodeStreamFormatAll       stream_format;

    AMEncodeStreamConfigAll       stream_config;
    AMEncodeLBRConfig             lbr_config;
    AMDPTZWarpConfig              dptz_warp_config;
    AMOSDOverlayConfig            osd_overlay_config;
};

class AMVinConfig: public AMIVinConfig
{
  public:
    AMVinConfig();
    ~AMVinConfig();

    //load from config file to m_loaded
    AM_RESULT load_config();

    //if changes are too big, VIN needs restart with new config
    bool need_restart();

    //save m_loaded to m_using
    AM_RESULT sync();

    AM_RESULT set_mode_config(AM_VIN_ID id, AM_VIN_MODE mode);
    AM_RESULT set_flip_config(AM_VIN_ID id, AM_VIDEO_FLIP flip);
    AM_RESULT set_fps_config(AM_VIN_ID id, AM_VIDEO_FPS fps);
    AM_RESULT set_bayer_config(AM_VIN_ID id, AM_VIN_BAYER_PATTERN bayer);

    //save m_using to file
    AM_RESULT save_config();

    //VIN attributes
    AMVinParamAll m_loaded;   //settings just loaded

    bool m_changed;

  protected:
    char m_type_string[MAX_VIDEO_TYPE_STRING_LENGTH]; //string like RGB_SENSOR
    //howerver, type_name is optional, because VIN_TYPE usually can be
    //detected.
    char m_mode_string[MAX_VIDEO_MODE_STRING_LENGTH]; //string like "720p"
    char m_fps_string[MAX_VIDEO_FPS_STRING_LENGTH]; //like "29.97",  "30"

    bool m_need_restart;
    AMVinParamAll m_using;    //setting saved
};


class AMVoutConfig
{
  public:
    AMVoutConfig();
    ~AMVoutConfig();

    //load from config file to m_loaded
    AM_RESULT load_config();

    //if changes are too big, VOUT needs restart with new config
    bool need_restart();

    //save m_loaded to m_using
    AM_RESULT sync();

    //save m_using to file
    AM_RESULT save_config();

    //VOUT attributes
    AMVoutParamAll m_loaded;   //settings just loaded

    bool m_changed;

  protected:
    char m_type_string[MAX_VIDEO_TYPE_STRING_LENGTH]; //string like RGB_SENSOR
    //howerver, type_name is optional, because VIN_TYPE usually can be
    //detected.
    char m_mode_string[MAX_VIDEO_MODE_STRING_LENGTH]; //string like "720p"
    char m_fps_string[MAX_VIDEO_FPS_STRING_LENGTH]; //like "29.97",  "30"
    bool m_need_restart;

    AMVoutParamAll m_using;    //setting saved
};


class AMResourceLimitConfig
{
  public:
    AMResourceLimitConfig(const std::string& file_name);
    virtual ~AMResourceLimitConfig();
    AM_RESULT load_config(AMResourceLimitParam *resource_limit);
  private:
    std::string cfg_file_name;
};


//this class is designed to load AMEncodeDeviceParameters from string
class AMEncodeDevice;
class AMEncodeDeviceConfig : public AMIEncodeDeviceConfig
{
  public:
    AMEncodeDeviceConfig(AMEncodeDevice *device);
    virtual ~AMEncodeDeviceConfig();

    //load config from file to m_loaded
    virtual AM_RESULT load_config();
    //save config m_using to file
    virtual AM_RESULT save_config();
    //compare "saved" copy to find difference
    //copy m_loaded to m_using
    virtual AM_RESULT sync();

    //other methods to change configs, which is equivalent to load_config

    virtual AM_RESULT set_stream_format(AM_STREAM_ID id,
                                        AMEncodeStreamFormat *stream_format);
    virtual AM_RESULT set_stream_config(AM_STREAM_ID id,
                                        AMEncodeStreamConfig *stream_config);
    virtual AMEncodeParamAll* get_encode_param();

    virtual AM_RESULT set_buffer_alloc_style(AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE style);
    virtual AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE get_buffer_alloc_style();

    virtual bool is_idle_cycle_required();

    AMEncodeParamAll m_loaded;
    AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE m_buffer_alloc_style;

  protected:
    int get_enabled_stream_num();

    AM_RESULT load_stream_format();
    AM_RESULT save_stream_format();
    AM_RESULT load_stream_config();
    AM_RESULT save_stream_config();
    AM_RESULT load_buffer_format();
    AM_RESULT save_buffer_format();
    AM_RESULT load_lbr_config();
    AM_RESULT save_lbr_config();
    AM_RESULT load_dptz_warp_config();
    AM_RESULT save_dptz_warp_config();
    AM_RESULT load_osd_overlay_config();
    AM_RESULT save_osd_overlay_config();

    AMRect get_default_input_window_rect(AM_ENCODE_SOURCE_BUFFER_ID buf_id);
    AMRect buffer_input_str_to_rect(const char *input_str, AM_ENCODE_SOURCE_BUFFER_ID buf_id);

    bool m_buffer_changed;
    bool m_stream_format_changed;
    bool m_stream_config_changed;
    bool m_lbr_config_changed;
    bool m_dptz_warp_config_changed;
    bool m_osd_overlay_config_changed;
    bool m_is_idle_cycle_required; //must go to idle cycle
    AMEncodeParamAll m_using;
    AMEncodeDevice *m_encode_device;
};

struct AMVinModeTable
{
    const char name[MAX_VIDEO_MODE_STRING_LENGTH];
    //the video strings will be loaded and processed hash value first,
    //then later, it will not do strcmp but compare hash values
    uint32_t hash_value;
    AM_VIN_MODE mode;
    uint32_t width;
    uint32_t height;
};

struct AMVoutModeTable
{
    const char name[MAX_VIDEO_MODE_STRING_LENGTH];
    //the video strings will be loaded and processed hash value first,
    //hen later, it will not do strcmp but compare hash values
    uint32_t hash_value;
    AM_VOUT_MODE mode;
    AM_VIDEO_FPS video_fps;
    uint32_t width;
    uint32_t height;
};

struct AMVideoFPSTable
{
    const char name[MAX_VIDEO_FPS_STRING_LENGTH];
    uint32_t hash_value;
    AM_VIDEO_FPS  video_fps;
    uint32_t video_fps_q9_format;
};

#endif /* AM_VIDEO_PARAM_H_ */
