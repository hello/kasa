/*******************************************************************************
 * am_video_dsp.h
 *
 * Histroy:
 *  2014-6-22 - [Louis] created file
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_DSP_H_
#define AM_VIDEO_DSP_H_

#include "am_video_types.h"
#include "iav_ioctl.h"

/* this file is the place where Oryx video uses DSP  to access DSP functions,
 * and these structures stores DSP data structure.
 * The original data structure comes from test_encode.c
 */

enum AM_VIN_STATE
{
  AM_VIN_STATE_NOT_INITIALIZED = 0,
  AM_VIN_STATE_RUNNING = 1,
  AM_VIN_STATE_STOPPED = 2,
};

enum AM_VOUT_STATE
{
  AM_VOUT_STATE_NOT_INITIALIZED = 0,
  AM_VOUT_STATE_RUNNING = 1,
  AM_VOUT_STATE_STOPPED = 2,
};

enum AM_VOUT_ID
{
  AM_VOUT_ID_INVALID = -1,    //invalid ID
  AM_VOUT_ID_HDMI = 0,//VOUT-B, which is usually associated with Preview-B
  AM_VOUT_ID_LCD = 1, //VOUT-A, which is usually associated with Preview-A
  AM_VOUT_A = AM_VOUT_ID_LCD, //duplicate name, by another way of reference
  AM_VOUT_B = AM_VOUT_ID_HDMI,//duplicate name, by another way of reference
  AM_VOUT_MAX_NUM = 2
};

enum AM_VOUT_MODE
{
  AM_VOUT_MODE_AUTO = 0, //Auto Adaptive to the vout device

  AM_VOUT_MODE_480P = 1, //480p60
  AM_VOUT_MODE_576P = 2, //576p50

  AM_VOUT_MODE_480I = 3,
  AM_VOUT_MODE_576I = 4,

  AM_VOUT_MODE_NTSC = 3,
  AM_VOUT_MODE_PAL = 4,

  AM_VOUT_MODE_720P60 = 10,
  AM_VOUT_MODE_720P50 = 11,

  AM_VOUT_MODE_720P30 = 12,
  AM_VOUT_MODE_720P25 = 13,

  AM_VOUT_MODE_1080P60 = 20,
  AM_VOUT_MODE_1080P50 = 21,
  AM_VOUT_MODE_1080P30 = 22,
  AM_VOUT_MODE_1080P25 = 23,
  AM_VOUT_MODE_1080P24 = 24,
  AM_VOUT_MODE_1080I60 = 25,
  AM_VOUT_MODE_1080I50 = 26,

  AM_VOUT_MODE_QFHD30 = 40, // 3840x2160p30
  AM_VOUT_MODE_QFHD24 = 41, // 3840x2160p24
};

enum AM_VIN_TYPE
{
  AM_VIN_TYPE_NONE = 0,
  //rgb type sensor has full control for DSP
  AM_VIN_TYPE_RGB_SENSOR = 1,
  //yuv sensor can still control fps and resolution,
  //can adjust params on yuv output
  AM_VIN_TYPE_YUV_SENSOR = 2,
  //yuv generic type is pure passive, unable to control fps and resolution
  AM_VIN_TYPE_YUV_GENERIC = 3
};

enum AM_VIDEO_ROTATE
{
  AM_VIDEO_ROTATE_NONE = 0,
  AM_VIDEO_ROTATE_90 = 1
};

enum AM_VOUT_TYPE
{
  AM_VOUT_TYPE_NONE = 0,
  AM_VOUT_TYPE_LCD = 1,       //LCD interface
  AM_VOUT_TYPE_HDMI = 2,      //HDMI
  AM_VOUT_TYPE_CVBS = 3,      //composite video
  AM_VOUT_TYPE_DIGITAL = 4    //BT656,601,1120
};

enum AM_VOUT_COLOR_CONVERSION
{
  AM_VOUT_COLOR_CONVERSION_NONE = 0,
};

enum AM_LCD_PANEL_TYPE
{
  AM_LCD_PANEL_NONE = 0,
  AM_LCD_PANEL_DIGITAL = 1,
  AM_LCD_PANEL_OTHERS
};

enum AM_ROTATE_MODE
{
  AM_NO_ROTATE_FLIP = 0,
  AM_HORIZONTAL_FLIP = (1 << 0),
  AM_VERTICAL_FLIP = (1 << 1),
  AM_ROTATE_90 = (1 << 2),

  AM_CW_ROTATE_90 = AM_ROTATE_90,
  AM_CW_ROTATE_180 = AM_HORIZONTAL_FLIP | AM_VERTICAL_FLIP,
  AM_CW_ROTATE_270 = AM_CW_ROTATE_90 | AM_CW_ROTATE_180,
};

enum AM_DSP_STATE
{
  AM_DSP_STATE_NOT_INIT = -1,
  AM_DSP_STATE_IDLE = 0,
  AM_DSP_STATE_READY_FOR_ENCODE = 1,
  AM_DSP_STATE_ENCODING = 2,
  AM_DSP_STATE_EXITING_ENCODING = 3,
  AM_DSP_STATE_PLAYING = 4,
  AM_DSP_STATE_ERROR = 5,    //state unknown, for error
};

enum AM_OSD_BLEND_MIXER
{
  AM_OSD_BLEND_MIXER_OFF = 0,
  AM_OSD_BLEND_MIXER_A = 1,
  AM_OSD_BLEND_MIXER_B = 2,
};

enum AM_IMAGE_ISO_TYPE
{
   AM_IMAGE_PIPELINE_NORMAL_ISO = 0,
   AM_IMAGE_PIPELINE_ISO_PLUS = 1,
   AM_IMAGE_PIPELINE_ADVANCED_ISO = 2,
};

enum AM_DEWARP_TYPE
{
    AM_DEWARP_NONE = 0,
    AM_DEWARP_LDC = 1,
    AM_DEWARP_FULL = 2,
};

enum AM_ENCODE_SOURCE_BUFFER_TYPE
{
  AM_ENCODE_SOURCE_BUFFER_TYPE_OFF = 0,
  AM_ENCODE_SOURCE_BUFFER_TYPE_ENCODE = 1,
  AM_ENCODE_SOURCE_BUFFER_TYPE_PREVIEW = 2,
};

enum AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE
{
  //No auto allocate buffer, stream config must specify which buffer to use
  AM_ENCODE_SOURCE_BUFFER_ALLOC_MANUAL = 0,

  // Auto allocate, stream and buffer are 1 to 1 relation
  AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_ONE_TO_ONE = 1,

  // Auto allocate, high resolution streams share same buffer
  AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_HIGHRES_SHARE = 2,

  AM_ENCODE_SOURCE_BUFFER_ALLOC_LAST =
      AM_ENCODE_SOURCE_BUFFER_ALLOC_AUTO_HIGHRES_SHARE,
};

enum AM_ENCODE_SOURCE_BUFFER_STATE
{
  AM_ENCODE_SOURCE_BUFFER_STATE_OFF = 0, //not used
  AM_ENCODE_SOURCE_BUFFER_STATE_ON = 1, //used
};

enum AM_ENCODE_STREAM_TYPE
{
  AM_ENCODE_STREAM_TYPE_NONE = 0, //none means not configured
  AM_ENCODE_STREAM_TYPE_H264 = 1,
  AM_ENCODE_STREAM_TYPE_MJPEG = 2,
};

enum AM_ENCODE_STREAM_STATE
{
  AM_ENCODE_STREAM_STATE_IDLE = 0,     //not encoding
  AM_ENCODE_STREAM_STATE_STARTING = 1, //starting
  AM_ENCODE_STREAM_STATE_ENCODING = 2, //encoding
  AM_ENCODE_STREAM_STATE_STOPPING = 3, //encoding
  AM_ENCODE_STREAM_STATE_ERROR = 4,   // error, should not happen
};

//H.264 only support M 1~3.  0 is used for special config for non H.264
enum AM_ENCODE_H264_GOP_MODEL
{
  AM_ENCODE_H264_GOP_MODEL_SIMPLE = 0, //simpe GOP
  AM_ENCODE_H264_GOP_MODEL_ADVANCED = 1, //hierachical
  AM_ENCODE_H264_GOP_MODEL_SVCT_FIRST = 2, //please don't change this value
  AM_ENCODE_H264_GOP_MODEL_SVCT_LAST = 5, //please don't change this value
};

enum AM_ENCODE_H264_RATE_CONTROL
{
  AM_ENCODE_H264_RC_CBR = 0, //SCBR version
  AM_ENCODE_H264_RC_VBR = 1, //SCBR version
  AM_ENCODE_H264_RC_CBR_QUALITY = 2, //SCBR version
  AM_ENCODE_H264_RC_VBR_QUALITY = 3, //SCBR version
  AM_ENCODE_H264_RC_CBR2 = 4, //old style CBR
  AM_ENCODE_H264_RC_VBR2 = 5, //old style VBR
  AM_ENCODE_H264_RC_LBR = 6, //CBR + ZMV
};

enum AM_ENCODE_H264_PROFILE
{
  AM_ENCODE_H264_PROFILE_BASELINE = 0,
  AM_ENCODE_H264_PROFILE_MAIN = 1,
  AM_ENCODE_H264_PROFILE_HIGH = 2,
};

enum AM_ENCODE_H264_AU_TYPE
{
  AM_ENCODE_H264_AU_TYPE_NO_AUD_NO_SEI = 0,
  AM_ENCODE_H264_AU_TYPE_AUD_BEFORE_SPS_WITH_SEI = 1,
  AM_ENCODE_H264_AU_TYPE_AUD_AFTER_SPS_WITH_SEI = 2,
  AM_ENCODE_H264_AU_TYPE_NO_AUD_WITH_SEI = 3,
};

struct AMWarpResource
{
    bool lens_warp;
    uint32_t max_warp_input_width;
    uint32_t max_warp_output_width;
};

//this structure configures max buffers number, max buffer sizes and etc.
struct AMEncodeResourceLimit
{
    AMResolution source_buffer_max_size[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
    AMResolution stream_max_size[AM_STREAM_MAX_NUM];
    uint8_t max_gop_model[AM_STREAM_MAX_NUM];
    uint8_t max_M[AM_STREAM_MAX_NUM];
    uint16_t max_N[AM_STREAM_MAX_NUM];
    uint8_t max_num_streams;
    uint8_t max_num_source_buffers;
    AM_HDR_EXPOSURE_TYPE hdr_exposure_type;
    AMWarpResource warp_resource;
    bool rotate_possible;
    bool raw_capture_possible;
    bool adv_iso_possible;
};

//source buffer format
struct AMEncodeSourceBufferFormat
{
    AM_ENCODE_SOURCE_BUFFER_TYPE type;
    AMRect input; //input window of source buffer
    AMResolution size;
    bool input_crop;  //true means digital PTZ enabled, false means no DPTZ
    bool prewarp; //only useful in dewarp mode
};

//this structure configures for the streams and stream to buffer relations
struct AMEncodeStreamFormat
{
    AM_ENCODE_STREAM_TYPE type;
    AM_ENCODE_SOURCE_BUFFER_ID source; //source buffer ID
    AMStreamFps fps; //frame factor
    uint32_t width;
    uint32_t height;
    uint32_t offset_x; //offset in source buffer
    uint32_t offset_y;
    bool hflip;
    bool vflip;
    bool rotate_90_ccw; //rotate 90 degrees counter-clock-wise
    bool enable;
};

struct AMEncodeH264Config
{
    AM_ENCODE_H264_GOP_MODEL gop_model;
    AM_ENCODE_H264_RATE_CONTROL bitrate_control;
    AM_ENCODE_H264_PROFILE profile_level;
    AM_ENCODE_H264_AU_TYPE au_type;
    AM_ENCODE_CHROMA_FORMAT chroma_format;    //common property
    uint32_t M; // 0 ~ 3
    uint32_t N; // 1 ~255
    uint32_t idr_interval; //  1~n
    uint32_t target_bitrate;
    uint32_t mv_threshold; //used in lbr mode
    uint32_t encode_improve; //encode improve 1: enable, 0: disable
    uint32_t multi_ref_p; //enable multi-reference P frame 1: enable, 0: disable
    uint32_t long_term_intvl; //[0,63]specify long term reference P frame interval
};

struct AMEncodeMJPEGConfig
{
    AM_ENCODE_CHROMA_FORMAT chroma_format;    //common property
    uint32_t quality; // 1 ~99
};

struct AMEncodeStreamConfig
{
    AMEncodeH264Config h264_config;
    AMEncodeMJPEGConfig mjpeg_config;
    bool apply;
};

//Q9 format fps is (512000000L / fps)
typedef uint32_t AMVideoFpsFormatQ9;

struct AMEncodeStaticConfig
{
    AMEncodeH264Config h264_config;
    AMEncodeMJPEGConfig mjpeg_config;
    //some dynamic change param
};

struct AMEncodeBitrate
{
    uint32_t average_bitrate;
    uint32_t vbr_setting :8;
    uint32_t qp_min_on_I :8;
    uint32_t qp_max_on_I :8;
    uint32_t qp_min_on_P :8;
    uint32_t qp_max_on_P :8;
    uint32_t qp_min_on_B :8;
    uint32_t qp_max_on_B :8;
    uint32_t i_qp_reduce :8;
    uint32_t p_qp_reduce :8;
    uint32_t adapt_qp :8;
    uint32_t skip_flag :8;
    uint32_t reserved1 :8;
};

//the dynamic configurations
//Not allowd for MJPEG, LBR not counted in
struct AMEncodeStreamDynamicConfig
{
    bool frame_fps_change;
    AMStreamFps frame_fps;

    bool encode_offset_change;
    AMOffset encode_offset;

    bool chroma_format_change;
    AM_ENCODE_CHROMA_FORMAT chroma_format;

    bool h264_gop_change;
    AMGOP h264_gop;

    bool h264_target_bitrate_change;
    uint32_t  h264_target_bitrate;

    bool h264_force_idr_immediately;

    bool jpeg_quality_change;
    uint32_t  jpeg_quality;
};

//Format may be changed without forcing DSP to idle, however,
//if format change has to force source buffer change,
//then DSP will have to go to idle
struct AMEncodeStreamFormatAll
{
    AMEncodeStreamFormat encode_format[AM_STREAM_MAX_NUM];
};

//Param can be changed without forcing DSP to idle, and can be independent
struct AMEncodeStreamConfigAll
{
    AMEncodeStreamConfig encode_config[AM_STREAM_MAX_NUM];
};

//usually source buffer change can only happen during DSP idle,
//both type and format must be setup at same time
struct AMEncodeSourceBufferFormatAll
{
    AMEncodeSourceBufferFormat \
    source_buffer_format[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
};

struct AMEncodePerformance
{
    uint32_t total_macroblock_per_sec;
    uint32_t total_bitrate;
    uint32_t num_streams;
};

enum AM_LBR_PROFILE_TYPE {
  AM_LBR_PROFILE_STATIC = 0,
  AM_LBR_PROFILE_SMALL_MOTION = 1,
  AM_LBR_PROFILE_BIG_MOTION = 2,
  AM_LBR_PROFILE_LOW_LIGHT = 3,
  AM_LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP = 4,
  AM_LBR_PROFILE_FIRST = AM_LBR_PROFILE_STATIC,
  AM_LBR_PROFILE_LAST = AM_LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP,
  AM_LBR_MAX_PROFILE_NUM = (AM_LBR_PROFILE_LAST + 1),
};

struct AMLBRStream {
    bool enable_lbr;
    bool motion_control;
    bool noise_control;
    bool frame_drop;
    bool auto_target;
    uint32_t bitrate_ceiling;
};

struct AMEncodeLBRConfig {
    bool config_changed;
    uint32_t log_level;
    uint32_t noise_low_threshold;
    uint32_t noise_high_threshold;
    AMScaleFactor profile_bt_sf[AM_LBR_MAX_PROFILE_NUM];
    AMLBRStream stream_params[AM_STREAM_MAX_NUM];
};

struct AMWarpConfig {
    uint32_t proj_mode;
    uint32_t warp_mode;
    float ldc_strength;
    float pano_hfov_degree;
};

struct AMDPTZConfig {
    float pan_ratio;
    float tilt_ratio;
    float zoom_ratio;
};

struct AMDPTZWarpConfig {
    uint32_t buf_cfg;
    AMWarpConfig warp_cfg;
    AMDPTZConfig dptz_cfg[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
};

//both OSD manual and auto layout will take care video horizontal flip/vertical flip
//or both vflip/hflip( 180 degree rotation).
//for manual layout case, the offset is counted from final offset from left-top corner
enum AMOSDLayoutStyle
{
  AM_OSD_LAYOUT_AUTO_LEFT_TOP = 0, //position will auto fit stream size
  AM_OSD_LAYOUT_AUTO_RIGHT_TOP = 1, //position will auto fit stream size
  AM_OSD_LAYOUT_AUTO_LEFT_BOTTOM = 2, //position will auto fit stream size
  AM_OSD_LAYOUT_AUTO_RIGHT_BOTTOM = 3, //position will auto fit stream size
  AM_OSD_LAYOUT_MANUAL = 4, // by user input offset
};

struct AMOSDLayout
{
    AMOSDLayoutStyle style;
    //only when style is AM_OSD_LAYOUT_MANUAL, offset_x/offset_y are used
    uint16_t offset_x;
    uint16_t offset_y;
};

//osd area information provide by user
struct AMOSDArea
{
    uint16_t    enable;
    uint16_t    width;
    uint16_t    pitch;
    uint16_t    height;
    uint32_t    total_size;
    uint32_t    clut_addr_offset;
    uint32_t    data_addr_offset;
    AMOSDLayout area_position;
};

enum AMOSDType
{
  AM_OSD_TYPE_GENERIC_STRING = 0, //most simple form of OSD
  AM_OSD_TYPE_TIME_STRING = 1, //most simple form of OSD
  AM_OSD_TYPE_PICTURE = 2, //small picture type, load from file
  AM_OSD_TYPE_TEST_PATTERN = 3, //color block, used for test
};

#define OSD_COLOR_KEY_MAX_NUM (3)
#define OSD_STRING_MAX_NUM    (512)
#define OSD_FILENAME_MAX_NUM  (256)
struct AMOSDFont
{
    //Truetype Font Name like "Lucida.ttf", system will auto look for it
    char      ttf_name[OSD_FILENAME_MAX_NUM];
    uint32_t  size;
    uint32_t  outline_width;
    int32_t   ver_bold;
    int32_t   hor_bold;
    int32_t   italic;
    uint32_t  disable_anti_alias;
};

//yuv color look up table
struct AMOSDCLUT
{
    uint8_t v;
    uint8_t u;
    uint8_t y;
    uint8_t a;
};

//color used to set font color when osd type is text
struct AMOSDColor
{
    //0~7: predefine color: 0(white), 1(black), 2(red),
    //3(blue), 4(green), 5(yellow), 6(cyan), 7(magenta);
    //8: custom color set by color value
    uint32_t  id;
    AMOSDCLUT color; //user custom color
};

//color used to transparent when osd type is picture
struct AMOSDColorKey
{
    //when color value is in [color-range,
    //color+range], it will do transparent
    uint8_t   range;
    AMOSDCLUT color;
};

//Text type(string and time) parameter
struct AMOSDTextBox
{
    char       osd_string[OSD_STRING_MAX_NUM];
    AMOSDFont  font;
    AMOSDColor font_color;
    AMOSDCLUT  *outline_color; //can be null
    AMOSDCLUT  *background_color;  //can be null
};

//Picture type parameter
struct AMOSDPicture
{
    //color which user want to transparent in osd
    AMOSDColorKey colorkey[OSD_COLOR_KEY_MAX_NUM];
    char          filename[OSD_FILENAME_MAX_NUM];
};

//osd attribute provide by user
struct AMOSDAttribute
{
    AMOSDType    type;
    //if set to 0, osd area will not auto flip or rotate
    //when encode stream is flip or rotate state
    uint16_t     enable_rotate;
    union {
        AMOSDTextBox osd_text_box;
        AMOSDPicture osd_pic;
    };
};

struct AMOSDInfo
{
    //used for user to save configure file, it indicate
    //the area is created, no matter enable or disable
    //only add and ON_VIDEO_OVERLAY_SET(remove action)
    //function can modify this value
    uint32_t        active_area;
    uint32_t        area_num;
    AMOSDAttribute  attribute[MAX_NUM_OVERLAY_AREA];
    AMOSDArea       overlay_area[MAX_NUM_OVERLAY_AREA];
};

struct AMOSDOverlayConfig
{
    bool      save_flag; //save config to file flag
    AMOSDInfo osd_cfg[AM_STREAM_MAX_NUM];
};

#endif  //AM_VIDEO_DSP_H_
