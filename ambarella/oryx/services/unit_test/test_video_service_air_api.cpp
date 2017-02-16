/*
 * test_video_service.cpp
 *
 *  History:
 *		Nov 11, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <math.h>
#include <vector>

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_api_helper.h"
#include "am_api_video.h"
#include "am_video_types.h"

#define NO_ARG 0
#define HAS_ARG 1

#define VERIFY_STREAM_ID(x) \
    do { \
      if (((x) < 0) || ((x) >= AM_STREAM_MAX_NUM)) { \
        printf ("Stream id wrong: %d \n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_BUFFER_ID(x) \
    do { \
      if (((x) < 0) || ((x) >= AM_ENCODE_SOURCE_BUFFER_MAX_NUM)) { \
        printf ("Buffer id wrong: %d \n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_1(x, min) \
    do { \
      if ((x) < min) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_1_FLOAT(x, min) \
    do { \
      if ((x) < min) { \
        printf("Parameter wrong: %f\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2_FLOAT(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        printf("Parameter wrong: %f\n", (x)); \
        return -1; \
      } \
    } while (0)

static AMAPIHelperPtr g_api_helper = NULL;
static bool show_status_flag = false;
static bool show_info_flag = false;
static bool show_fmt_flag = false;
static bool show_cfg_flag = false;
static bool show_dptz_warp_flag = false;
static bool show_lbr_flag = false;
static bool show_style_flag = false;
static bool show_buf_flag = false;
static bool show_overlay_flag = false;
static bool pause_flag = false;
static bool resume_flag = false;
static bool apply_flag = false;
static bool destroy_overlay_flag = false;
static bool save_overlay_flag = false;
static bool get_overlay_max_flag = false;
static uint32_t force_idr_id = 0;

static void sigstop(int32_t arg)
{
  INFO("signal comes!\n");
  exit(1);
}

struct setting_option {
    union {
        bool     v_bool;
        float    v_float;
        int32_t  v_int;
        uint32_t v_uint;
    } value;
    bool is_set;
};

struct vin_setting {
    bool is_set;
    setting_option vin_flip;
    setting_option vin_fps;
    setting_option vin_hdr_mode;
};

struct stream_fmt_setting {
    bool is_set;
    setting_option enable;
    setting_option type;
    setting_option source;
    setting_option frame_facc_num;
    setting_option frame_fact_den;
    setting_option width;
    setting_option height;
    setting_option offset_x;
    setting_option offset_y;
    setting_option hflip;
    setting_option vflip;
    setting_option rotate;
};

struct stream_cfg_setting {
    bool is_set;
    setting_option h264_bitrate_ctrl;
    setting_option h264_profile;
    setting_option h264_au_type;
    setting_option h264_chroma;
    setting_option h264_m;
    setting_option h264_n;
    setting_option h264_idr_interval;
    setting_option h264_bitrate;
    setting_option h264_mv_threshold;
    setting_option h264_enc_improve;
    setting_option h264_multi_ref_p;
    setting_option h264_long_term_intvl;
    setting_option mjpeg_quality;
    setting_option mjpeg_chroma;
};

struct dptz_cfg_setting {
    bool is_set;
    setting_option pan_ratio;
    setting_option tilt_ratio;
    setting_option zoom_ratio;
};

struct warp_cfg_setting {
    bool is_set;
    setting_option ldc_strength;
    setting_option pano_hfov_degree;
};

struct dptz_warp_cfg_setting {
    setting_option buf_id;
    struct dptz_cfg_setting dptz[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
    struct warp_cfg_setting warp;
};

struct h264_lbr_cfg_setting {
    bool is_set;
    setting_option lbr_enable;
    setting_option lbr_auto_bitrate_target;
    setting_option lbr_bitrate_ceiling;
    setting_option lbr_frame_drop;
};

struct buffer_alloc_style {
    bool is_set;
    bool value;
};

struct buffer_fmt_setting {
    bool is_set;
    setting_option type;
    setting_option input_crop;
    setting_option input_width;
    setting_option input_height;
    setting_option input_offset_x;
    setting_option input_offset_y;
    setting_option width;
    setting_option height;
    setting_option prewarp;
};

struct area_setting {
    setting_option enable;
    setting_option width;
    setting_option height;
    setting_option layout;
    setting_option offset_x;
    setting_option offset_y;
    setting_option type;
    setting_option rotate;
    setting_option font_size;
    setting_option font_color;
    setting_option font_outline_w;
    setting_option font_hor_bold;
    setting_option font_ver_bold;
    setting_option font_italic;
    setting_option color_key;
    setting_option color_range;
    bool is_font_name_set;
    char font_name[OSD_MAX_FILENAME];
    bool is_string_set;
    char str[OSD_MAX_STRING];
    bool is_bmp_set;
    char bmp[OSD_MAX_FILENAME];
};

struct overlay_setting {
    bool                      is_set;
    uint32_t                  num;
    std::vector<area_setting> add;
    setting_option            remove;
    setting_option            enable;
    setting_option            disable;
};
static uint32_t g_overlay_max_num;

static vin_setting g_vin_setting[AM_VIN_MAX_NUM];
static stream_fmt_setting g_stream_fmt[AM_STREAM_MAX_NUM];
static stream_cfg_setting g_stream_cfg[AM_STREAM_MAX_NUM];
static dptz_warp_cfg_setting g_dptz_warp_cfg;
static h264_lbr_cfg_setting g_lbr_cfg[AM_STREAM_MAX_NUM];
static buffer_alloc_style g_alloc_style;
static buffer_fmt_setting g_buffer_fmt[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
static overlay_setting g_overlay_cfg[AM_STREAM_MAX_NUM];

enum numeric_short_options {
  NONE = 0,
  H264 = 1,
  MJPEG = 2,

  OFF = 0,
  ENCODE = 1,
  PREVIEW = 2,

  CHROMA_420 = 0,
  CHROMA_422 = 1,
  CHROMA_MONO = 2,

  NEMERIC_BEGIN = 'z',

  VIN_FLIP,
  VIN_FPS,
  VIN_HDR_MODE,

  //stream format
  STREAM_ENABLE,
  STREAM_SOURCE,
  STREAM_FRAME_NUM,
  STREAM_FRAME_DEN,
  STREAM_OFFSET_X,
  STREAM_OFFSET_Y,
  STREAM_HFLIP,
  STREAM_VFLIP,
  STREAM_ROTATE,

  //stream config
  BITRATE_CTRL,
  PROFILE,
  AU_TYPE,
  H264_CHROMA,
  IDR_INTERVAL,
  BITRATE,
  MV_THRESHOLD,
  ENC_IMPROVE,
  MULTI_REF_P,
  LONG_TERM_INTVL,
  MJPEG_QUALITY,
  MJPEG_CHROMA,

  //stream dynamic control
  FORCE_IDR,

  //DPTZ Warp config
  LDC_STRENGTH,
  PANO_HFOV_DEGREE,
  PAN_RATIO,
  TILT_RATIO,
  ZOOM_RATIO,

  //LBR config
  LBR_ENABLE,
  LBR_AUTO_BITRATE_TARGET,
  LBR_BITRATE_CEILING,
  LBR_FRAME_DROP,

  ALLOC_STYLE,
  INPUT_CROP,
  INPUT_WIDTH,
  INPUT_HEIGHT,
  INPUT_OFFSET_X,
  INPUT_OFFSET_Y,

  //OSD config
  OSD_MAX,
  OSD_ADD,
  OSD_REMOVE,
  OSD_ENABLE,
  OSD_DISABLE,
  OSD_DESTROY,
  OSD_WIDTH,
  OSD_HEIGHT,
  OSD_LAYOUT,
  OSD_OFFSET_X,
  OSD_OFFSET_Y,
  OSD_TYPE,
  OSD_ROTATE,
  OSD_FONT_TYPE,
  OSD_FONT_SIZE,
  OSD_FONT_COLOR,
  OSD_FONT_OUTLINE_WIDTH,
  OSD_FONT_HOR_BOLD,
  OSD_FONT_VER_BOLD,
  OSD_FONT_ITALIC,
  OSD_COLOR_KEY,
  OSD_COLOR_RANGE,
  OSD_STRING,
  OSD_BMP,
  OSD_SAVE,


  STOP_ENCODING,
  START_ENCODING,

  APPLY,

  SHOW_STREAM_STATUS,
  SHOW_STREAM_FMT,
  SHOW_STREAM_CFG,
  SHOW_DPTZ_WARP_CFG,
  SHOW_LBR_CFG,
  SHOW_ALLOC_STYLE,
  SHOW_BUFFER_FMT,
  SHOW_OSD_CFG,
  SHOW_ALL_INFO
};

static struct option long_options[] =
{
 {"help",               NO_ARG,   0,  'h'},

 {"vin-flip",           HAS_ARG,  0,  VIN_FLIP},
 {"vin-fps",            HAS_ARG,  0,  VIN_FPS},
 {"vin-hdr",            HAS_ARG,  0,  VIN_HDR_MODE},

 {"stream",             HAS_ARG,  0,  's'},
 {"enable",             HAS_ARG,  0,  STREAM_ENABLE},
 {"type",               HAS_ARG,  0,  't'},
 {"source",             HAS_ARG,  0,  STREAM_SOURCE},
 {"frame-num",          HAS_ARG,  0,  STREAM_FRAME_NUM},
 {"frame-den",          HAS_ARG,  0,  STREAM_FRAME_DEN},
 {"width",              HAS_ARG,  0,  'W'},
 {"height",             HAS_ARG,  0,  'H'},
 {"off-x",              HAS_ARG,  0,  STREAM_OFFSET_X},
 {"off-y",              HAS_ARG,  0,  STREAM_OFFSET_Y},
 {"hflip",              HAS_ARG,  0,  STREAM_HFLIP},
 {"vflip",              HAS_ARG,  0,  STREAM_VFLIP},
 {"rotate",             HAS_ARG,  0,  STREAM_ROTATE},

 {"b-ctrl",             HAS_ARG,  0,  BITRATE_CTRL},
 {"profile",            HAS_ARG,  0,  PROFILE},
 {"au-type",            HAS_ARG,  0,  AU_TYPE},
 {"h-chroma",           HAS_ARG,  0,  H264_CHROMA},
 {"gop-m",              HAS_ARG,  0,  'm'},
 {"gop-n",              HAS_ARG,  0,  'n'},
 {"idr",                HAS_ARG,  0,  IDR_INTERVAL},
 {"bitrate",            HAS_ARG,  0,  BITRATE},
 {"mv",                 HAS_ARG,  0,  MV_THRESHOLD},
 {"enc-improve",        HAS_ARG,  0,  ENC_IMPROVE},
 {"multi-ref-p",        HAS_ARG,  0,  MULTI_REF_P},
 {"long-term-intvl",    HAS_ARG,  0,  LONG_TERM_INTVL},
 {"m-quality",          HAS_ARG,  0,  MJPEG_QUALITY},
 {"m-chroma",           HAS_ARG,  0,  MJPEG_CHROMA},

 {"force-idr",          NO_ARG,   0,  FORCE_IDR},

 {"ldc-strength",       HAS_ARG,  0,  LDC_STRENGTH},
 {"pano-hfov-degree",   HAS_ARG,  0,  PANO_HFOV_DEGREE},
 {"pan-ratio",          HAS_ARG,  0,  PAN_RATIO},
 {"tilt-ratio",         HAS_ARG,  0,  TILT_RATIO},
 {"zoom-ratio",         HAS_ARG,  0,  ZOOM_RATIO},

 {"lbr-en",           HAS_ARG,  0,  LBR_ENABLE},
 {"lbr-abt",          HAS_ARG,  0,  LBR_AUTO_BITRATE_TARGET},
 {"lbr-bc",           HAS_ARG,  0,  LBR_BITRATE_CEILING},
 {"lbr-fd",           HAS_ARG,  0,  LBR_FRAME_DROP},

 {"alloc-style",        HAS_ARG,  0,  ALLOC_STYLE},

 {"buffer",             HAS_ARG,  0,  'b'},

 {"type",               HAS_ARG,  0,  't'},
 {"input-crop",         HAS_ARG,  0,  INPUT_CROP},
 {"input-width",        HAS_ARG,  0,  INPUT_WIDTH},
 {"input-height",       HAS_ARG,  0,  INPUT_HEIGHT},
 {"input-x",            HAS_ARG,  0,  INPUT_OFFSET_X},
 {"input-y",            HAS_ARG,  0,  INPUT_OFFSET_Y},
 {"width",              HAS_ARG,  0,  'W'},
 {"height",             HAS_ARG,  0,  'H'},

 //below is overlay option
 {"osd-max",            NO_ARG,  0, OSD_MAX}, //add osd
 {"osd-add",            HAS_ARG, 0, OSD_ADD}, //add osd
 {"osd-remove",         HAS_ARG, 0, OSD_REMOVE}, //remove osd
 {"osd-enable",         HAS_ARG, 0, OSD_ENABLE}, //enable
 {"osd-disable",        HAS_ARG, 0, OSD_DISABLE}, //disable
 {"osd-destroy",        NO_ARG,  0, OSD_DESTROY}, //destory all osd
 {"osd-layout",         HAS_ARG, 0, OSD_LAYOUT}, //area layout
 {"osd-w",              HAS_ARG, 0, OSD_WIDTH}, //area width
 {"osd-h",              HAS_ARG, 0, OSD_HEIGHT}, //area height
 {"osd-x",              HAS_ARG, 0, OSD_OFFSET_X}, //area offset x
 {"osd-y",              HAS_ARG, 0, OSD_OFFSET_Y}, //area offset y
 {"osd-type",           HAS_ARG, 0, OSD_TYPE}, //osd type
 {"osd-rotate",         HAS_ARG, 0, OSD_ROTATE}, //rotate
 {"osd-font_t",         HAS_ARG, 0, OSD_FONT_TYPE}, //font type name
 {"osd-font_s",         HAS_ARG, 0, OSD_FONT_SIZE}, //font size
 {"osd-font_c",         HAS_ARG, 0, OSD_FONT_COLOR}, //font color
 {"osd-font_ow",        HAS_ARG, 0, OSD_FONT_OUTLINE_WIDTH}, //font outline width
 {"osd-font_hb",        HAS_ARG, 0, OSD_FONT_HOR_BOLD}, //font hor_bold
 {"osd-font_vb",        HAS_ARG, 0, OSD_FONT_VER_BOLD}, //font ver_bold
 {"osd-font_i",         HAS_ARG, 0, OSD_FONT_ITALIC}, //font italic
 {"osd-color_k",        HAS_ARG, 0, OSD_COLOR_KEY}, //color to transparent
 {"osd-color_r",        HAS_ARG, 0, OSD_COLOR_RANGE}, //color range to transparent
 {"osd-str",            HAS_ARG, 0, OSD_STRING}, //string
 {"osd-bmp",            HAS_ARG, 0, OSD_BMP}, //bmp file path
 {"osd-save",           NO_ARG, 0, OSD_SAVE}, //save user setting to configure file


 {"pause",              NO_ARG,   0,  STOP_ENCODING},
 {"resume",             NO_ARG,   0,  START_ENCODING},

 {"apply",              NO_ARG,   0,  APPLY},

 {"show-stream-status", NO_ARG,   0,  SHOW_STREAM_STATUS},
 {"show-stream-fmt",    NO_ARG,   0,  SHOW_STREAM_FMT},
 {"show-stream-cfg",    NO_ARG,   0,  SHOW_STREAM_CFG},
 {"show-dptz-warp-cfg", NO_ARG,   0,  SHOW_DPTZ_WARP_CFG},
 {"show-lbr-cfg",       NO_ARG,   0,  SHOW_LBR_CFG},
 {"show-alloc-style",   NO_ARG,   0,  SHOW_ALLOC_STYLE},
 {"show-buffer-fmt",    NO_ARG,   0,  SHOW_BUFFER_FMT},
 {"show-osd-cfg",       NO_ARG,   0,  SHOW_OSD_CFG},
 {"show-all-info",      NO_ARG,   0,  SHOW_ALL_INFO},
 {0, 0, 0, 0}
};

static const char *short_options = "hs:t:W:H:b:m:n:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
 {"",     "\t\t\t"  "Show usage\n"},

 {"0-3",  "\t\t"    "VIN horizontal flip. "
                    "0: not flip, 1: flipv; 2: fliph; 3: both. "
                    "Set to config file."},
 {"",     "\t\t\t"  "VIN framerate. 0: auto; Set to config file"},
 {"0-3",  "\t\t"    "VIN HDR mode. 0: linear, 1: 2x; 2: 3x; 3: 4x. "
                    "Set to config file.\n"},

 {"0-3",  "\t\t"    "Stream ID"},
 {"0|1",  "\t\t"    "Enable or disable stream."},
 {"NONE|H264|MJPEG", "\t"    "Stream type."
                    "Set to config file."},
 {"0-3",  "\t\t"    "Stream source. "
                    "0: Main buffer; 1: 2nd buffer; "
                    "2: 3rd buffer, 3: 4th buffer. "
                    "Set to config file."},
 {"",     "\t\t\t"  "Frame factor numerator. Set to config file."},
 {"",     "\t\t\t"  "Frame factor denominator. Set to config file."},
 {"",     "\t\t\t"  "Stream width. Set to config file."},
 {"",     "\t\t\t"  "Stream height. Set to config file."},
 {"",     "\t\t\t"  "Stream offset x. Set to config file."},
 {"",     "\t\t\t"  "Stream offset y. Set to config file."},
 {"0|1",  "\t\t"    "Stream horizontal flip. Set to config file."},
 {"0|1",  "\t\t"    "Stream vertical flip. Set to config file."},
 {"0|1",  "\t\t"    "Stream counter-clock-wise rotation 90 degrees. "
                    "Set to config file.\n"},

 {"0-6",  "\t\t"    "Bitrate control method.\n"
                    "\t\t\t\t\t"
                    "  0: CBR; 1: VBR; 2: CBR Quality; 3: VBR Quality; 4: CBR2;"
                    "5: VBR2; 6: LBR.\n"
                    "\t\t\t\t\t"
                    "  Set to config file."},
 {"0-2",  "\t\t"    "H264 Profile. 0: Baseline; 1: Main; 2: High. "
                    "Set to config file."},
 {"0-3",  "\t\t"    "H264 AU type. \n"
                    "\t\t\t\t\t"
                    "  0: NO_AUD_NO_SEI; 1: AUD_BEFORE_SPS_WITH_SEI; "
                    "2: AUD_AFTER_SPS_WITH_SEI; 3: NO_AUD_WITH_SEI\n"
                    "\t\t\t\t\t"
                    "  Set to config file."},
 {"420|422|mono",   "\t"  "H264 Chroma format. Set to config file."},
 {"1-3",  "\t\t"    "H264 GOP M. Set to config file."},
 {"15-1800", "\t\t" "H264 GOP N. Set to config file."},
 {"1-4",  "\t\t\t"  "H264 IDR interval. Set to config file."},
 {"",     "\t\t\t"  "H264 Target bitrate. Set to config file."},
 {"0-64", "\t\t\t"  "H264 MV threshold. Set to config file."},
 {"0|1", "\t\t"     "H264 encode improve. Set to config file."},
 {"0|1", "\t\t"     "H264 multi ref P. Set to config file."},
 {"0-63", "\t"      "H264 long term interval. Set to config file."},
 {"1-99", "\t\t"    "MJPEG quality. Set to config file."},
 {"420|422|mono",   "\t"  "MJPEG Chroma format. Set to config file.\n"},

 {"",     "\t\t\t"  "force IDR at once for current stream"},

 {"0.0-20.0", "\t" "LDC strength"},
 {"1.0-180.0", "\t" "Panorama HFOV degree"},
 {"-1.0-1.0", "\t" "pan ratio"},
 {"-1.0-1.0", "\t" "tilt ratio"},
 {"0.25-8.0", "\t" "zoom ratio\n"
                   "\t\t\t\t\tDPTZ-I  1st buffer scalability 1/4X  - 8X\n"
                   "\t\t\t\t\tDPTZ-II 2nd buffer scalability 1/16X - 1X\n"
                   "\t\t\t\t\tDPTZ-II 3rd buffer scalability 1/8X  - 8X\n"
                   "\t\t\t\t\tDPTZ-II 4th buffer scalability 1/16X - 8X\n"},

 {"0|1", "\t\t" "low bitrate enable"},
 {"0|1", "\t\t" "low bitrate auto bitrate target"},
 {"", "\t\t\t"  "low bitrate bitrate ceiling(bps/MB)"},
 {"0|1", "\t\t" "low bitrate frame drop\n"},

 {"0|1",  "\t\t"    "Buffer allocate method. 0: Manual; 1: Auto. "
                    "Set to config file.\n"},
 {"0-3",  "\t\t"    "Source buffer ID. 0: Main buffer; 1: 2nd buffer; "
                    "2: 3rd buffer; 3: 4th buffer. Set to config file."},
 {"OFF|ENCODE|PREVIEW", "\t" "Source buffer type."
                    " Set to config file."},
 {"0|1",  "\t\t"    "Crop input. 0: use default input window; "
                    "1: use input window specify in next options. "
                    "Set to confif file."},
 {"",     "\t\t"    "Input window width. Set to config file."},
 {"",     "\t\t"    "Input window height. Set to config file."},
 {"",     "\t\t\t"  "Input window offset X. Set to config file."},
 {"",     "\t\t\t"  "Input window offset Y. Set to config file."},
 {"",     "\t\t\t"  "Source buffer width. Set to config file."},
 {"",     "\t\t\t"  "Source buffer height. Set to config file.\n"},

 //osd
 {"",     "\t\t\t"     "get the platform support max overlay area number"},
 {"0|1",  "\t\t"     "add a overlay area,0:disable,1:enable"},
 {"0~max","\t"       "remove overlay area, 0~max-1 means the area id for the stream, "
                     "max means remove all overlay area for the stream"},
 {"0~max","\t"       "enable overlay area, 0~max-1 means the area id for the stream, "
                     "max means enable all overlay area for the stream"},
 {"0~max","\t"       "disable overlay area, 0~max-1 means the area id for the stream, "
                     "max means disable all overlay area for the stream"},
 {"",     "\t\t"     "destroy all overlay for all stream"},
 {"",     "\t\t"     "area layout, 0:left top; 1:right top; 2:left bottom; 3:right bottom;"
                     " 4:manual set by osd-x and osd-y"},
 {"",     "\t\t\t"   "area width"},
 {"",     "\t\t\t"   "area height"},
 {"",     "\t\t\t"   "manual area offset x"},
 {"",     "\t\t\t"   "manual area offset y"},
 {"",     "\t\t\t"   "overlay type, 0:generic string; 1:string time; 2:bmp picture;"
                     "3:test pattern"},
 {"",     "\t\t"     "When rotate clockwise degree is 90/180/270, OSD is consistent "
                     "with stream orientation."},
 {"",     "\t\t"     "font type name"},
 {"",     "\t\t"     "font size"},
 {"",     "\t\t"     "font color,0-7 is predefine color: 0,white;1,black;2,red;3,blue;4,green;5,yellow;\n"
                     "\t\t\t\t\t6,cyan;7,magenta; >7,user custom(v:24-31,u:16-23,y:8-15,0-7:alpha)"},
 {"",     "\t\t"     "outline size, 0 none outline"},
 {"",     "\t\t"     "n percentage HOR bold of font size"},
 {"",     "\t\t"     "n percentage VER bold of font size"},
 {"",     "\t\t"     "n percentage italic of font size"},
 {"",     "\t\t"     "color used to transparent when osd type is picture,v:24-31,u:16-23,y:8-15,a:0-7"},
 {"",     "\t\t"     "color range used to transparent with color key"},
 {"string", "\t\t"   "text to be inserted"},
 {"bmp",  "\t\t"     "bmp file to be inserted"},
 {"",     "\t\t\t"     "save all setting to configure file(/etc/oryx/video/osd_overlay.acs)\n"},
 //osd

 {"",     "\t\t\t"  "Pause streaming"},
 {"",     "\t\t\t"  "Resume streaming"},

 {"",     "\t\t\t"  "Apply config files' changes immediately\n"},

 {"",     "\t"      "Show all streams' status"},
 {"",     "\t\t"    "Show all streams' formats"},
 {"",     "\t\t"    "Show all streams' configs"},
 {"",     "\t\t"    "Show DPTZ Warp configs"},
 {"",     "\t\t"    "Show lbr configs"},
 {"",     "\t\t"    "Show buffer allocate style"},
 {"",     "\t\t"    "Show all buffers' formats"},
 {"",     "\t\t"    "Show all overlay area information"},
 {"",     "\t\t"    "Show all information"}
};

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options)/sizeof(long_options[0])-1; ++i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c, ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
  printf("\n");
}

static int32_t init_param(int32_t argc, char **argv)
{
  int32_t ch;
  int32_t option_index = 0;
  int32_t current_vin_id = 0;
  int32_t current_stream_id = -1;
  int32_t current_buffer_id = -1;
  int32_t area_id = 0;
  int32_t ret = 0;
  area_setting tmp = {0};
  opterr = 0;
  enum {
    set_none = 0,
    set_stream,
    set_buffer
  } set_state = set_none;

  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        usage(argc, argv);
        return -1;

      case VIN_FLIP:
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_vin_setting[current_vin_id].is_set = true;
        g_vin_setting[current_vin_id].vin_flip.is_set = true;
        g_vin_setting[current_vin_id].vin_flip.value.v_int = atoi(optarg);
        break;
      case VIN_FPS:
        VERIFY_PARA_2_FLOAT(atof(optarg), 0, 120);
        g_vin_setting[current_vin_id].is_set = true;
        g_vin_setting[current_vin_id].vin_fps.is_set = true;
        g_vin_setting[current_vin_id].vin_fps.value.v_float = atof(optarg);
        break;
      case VIN_HDR_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_vin_setting[current_vin_id].is_set = true;
        g_vin_setting[current_vin_id].vin_hdr_mode.is_set = true;
        g_vin_setting[current_vin_id].vin_hdr_mode.value.v_int = atoi(optarg);
        break;

      case 's':
        current_stream_id = atoi(optarg);
        current_buffer_id = -1;
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].is_set = true;
        g_stream_cfg[current_stream_id].is_set = true;
        set_state = set_stream;
        break;
      case STREAM_ENABLE:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].enable.is_set = true;
        g_stream_fmt[current_stream_id].enable.value.v_bool = atoi(optarg);
        break;
      case STREAM_SOURCE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_stream_fmt[current_stream_id].source.is_set = true;
        g_stream_fmt[current_stream_id].source.value.v_int = atoi(optarg);
        break;
      case STREAM_FRAME_NUM:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].frame_facc_num.is_set = true;
        g_stream_fmt[current_stream_id].frame_facc_num.value.v_int = atoi(optarg);
        break;
      case STREAM_FRAME_DEN:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].frame_fact_den.is_set = true;
        g_stream_fmt[current_stream_id].frame_fact_den.value.v_int = atoi(optarg);
        break;
      case 'W':
        if (set_state == set_stream) {
          VERIFY_STREAM_ID(current_stream_id);
          g_stream_fmt[current_stream_id].width.is_set = true;
          g_stream_fmt[current_stream_id].width.value.v_int = atoi(optarg);
        } else if (set_state == set_buffer){
          VERIFY_BUFFER_ID(current_buffer_id);
          g_buffer_fmt[current_buffer_id].width.is_set = true;
          g_buffer_fmt[current_buffer_id].width.value.v_int = atoi(optarg);
        }
        break;
      case 'H':
        if (set_state == set_stream) {
          VERIFY_STREAM_ID(current_stream_id);
          g_stream_fmt[current_stream_id].height.is_set = true;
          g_stream_fmt[current_stream_id].height.value.v_int = atoi(optarg);
        } else if (set_state == set_buffer) {
          VERIFY_BUFFER_ID(current_buffer_id);
          g_buffer_fmt[current_buffer_id].height.is_set = true;
          g_buffer_fmt[current_buffer_id].height.value.v_int = atoi(optarg);
        }
        break;
      case STREAM_OFFSET_X:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].offset_x.is_set = true;
        g_stream_fmt[current_stream_id].offset_x.value.v_int = atoi(optarg);
        break;
      case STREAM_OFFSET_Y:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].offset_y.is_set = true;
        g_stream_fmt[current_stream_id].offset_y.value.v_int = atoi(optarg);
        break;
      case STREAM_HFLIP:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].hflip.is_set = true;
        g_stream_fmt[current_stream_id].hflip.value.v_bool = atoi(optarg);
        break;
      case STREAM_VFLIP:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].vflip.is_set = true;
        g_stream_fmt[current_stream_id].vflip.value.v_bool = atoi(optarg);
        break;
      case STREAM_ROTATE:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].rotate.is_set = true;
        g_stream_fmt[current_stream_id].rotate.value.v_bool = atoi(optarg);
        break;

      case BITRATE_CTRL:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 6);
        g_stream_cfg[current_stream_id].h264_bitrate_ctrl.is_set = true;
        g_stream_cfg[current_stream_id].h264_bitrate_ctrl.value.v_int = atoi(optarg);
        break;
      case PROFILE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 2);
        g_stream_cfg[current_stream_id].h264_profile.is_set = true;
        g_stream_cfg[current_stream_id].h264_profile.value.v_int = atoi(optarg);
        break;
      case AU_TYPE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_stream_cfg[current_stream_id].h264_au_type.is_set = true;
        g_stream_cfg[current_stream_id].h264_au_type.value.v_int = atoi(optarg);
        break;
      case H264_CHROMA:
        VERIFY_STREAM_ID(current_stream_id);
        if (!strcasecmp(optarg, "420")) {
          g_stream_cfg[current_stream_id].h264_chroma.value.v_int = CHROMA_420;
        } else if (!strcasecmp(optarg, "422")) {
          g_stream_cfg[current_stream_id].h264_chroma.value.v_int = CHROMA_422;
        } else if (!strcasecmp(optarg, "mono")) {
          g_stream_cfg[current_stream_id].h264_chroma.value.v_int = CHROMA_MONO;
        } else {
          ERROR("Wrong H264 Chroma Format: %s", optarg);
        }
        g_stream_cfg[current_stream_id].h264_chroma.is_set = true;
        break;
      case 'm':
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 3);
        g_stream_cfg[current_stream_id].h264_m.is_set = true;
        g_stream_cfg[current_stream_id].h264_m.value.v_int = atoi(optarg);
        break;
      case 'n':
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 15, 1800);
        g_stream_cfg[current_stream_id].h264_n.is_set = true;
        g_stream_cfg[current_stream_id].h264_n.value.v_int = atoi(optarg);
        break;
      case IDR_INTERVAL:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 4);
        g_stream_cfg[current_stream_id].h264_idr_interval.is_set = true;
        g_stream_cfg[current_stream_id].h264_idr_interval.value.v_int = atoi(optarg);
        break;
      case BITRATE:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_cfg[current_stream_id].h264_bitrate.is_set = true;
        g_stream_cfg[current_stream_id].h264_bitrate.value.v_int = atoi(optarg);
        break;
      case MV_THRESHOLD:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 64);
        g_stream_cfg[current_stream_id].h264_mv_threshold.is_set = true;
        g_stream_cfg[current_stream_id].h264_mv_threshold.value.v_int = atoi(optarg);
        break;
      case ENC_IMPROVE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_cfg[current_stream_id].h264_enc_improve.is_set = true;
        g_stream_cfg[current_stream_id].h264_enc_improve.value.v_int = atoi(optarg);
        break;
      case MULTI_REF_P:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_cfg[current_stream_id].h264_multi_ref_p.is_set = true;
        g_stream_cfg[current_stream_id].h264_multi_ref_p.value.v_int = atoi(optarg);
        break;
      case LONG_TERM_INTVL:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 63);
        g_stream_cfg[current_stream_id].h264_long_term_intvl.is_set = true;
        g_stream_cfg[current_stream_id].h264_long_term_intvl.value.v_int = atoi(optarg);
        break;
      case MJPEG_QUALITY:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 99);
        g_stream_cfg[current_stream_id].mjpeg_quality.is_set = true;
        g_stream_cfg[current_stream_id].mjpeg_quality.value.v_int = atoi(optarg);
        break;
      case MJPEG_CHROMA:
        VERIFY_STREAM_ID(current_stream_id);
        if (!strcasecmp(optarg, "420")) {
          g_stream_cfg[current_stream_id].mjpeg_chroma.value.v_int = CHROMA_420;
        } if (!strcasecmp(optarg, "422")) {
          g_stream_cfg[current_stream_id].mjpeg_chroma.value.v_int = CHROMA_422;
        } if (!strcasecmp(optarg, "mono")) {
          g_stream_cfg[current_stream_id].mjpeg_chroma.value.v_int = CHROMA_MONO;
        } else {
          ERROR("Wrong Mjpeg Chroma Format: %s", optarg);
        }
        g_stream_cfg[current_stream_id].mjpeg_chroma.is_set = true;
        break;

      case FORCE_IDR:
        VERIFY_STREAM_ID(current_stream_id);
        force_idr_id |= (1 << current_stream_id);
        break;
      case LDC_STRENGTH:
        current_buffer_id = AM_ENCODE_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2_FLOAT(atof(optarg), 0.0, 20.0);
        g_dptz_warp_cfg.warp.ldc_strength.value.v_float = atof(optarg);
        g_dptz_warp_cfg.warp.ldc_strength.is_set = true;
        g_dptz_warp_cfg.warp.is_set = true;
        break;
      case PANO_HFOV_DEGREE:
        current_buffer_id = AM_ENCODE_SOURCE_BUFFER_MAIN;
        VERIFY_PARA_2_FLOAT(atof(optarg), 1.0, 180.0);
        g_dptz_warp_cfg.warp.pano_hfov_degree.value.v_float = atof(optarg);
        g_dptz_warp_cfg.warp.pano_hfov_degree.is_set = true;
        g_dptz_warp_cfg.warp.is_set = true;
        break;
      case PAN_RATIO:
        VERIFY_BUFFER_ID(current_buffer_id);
        VERIFY_PARA_2_FLOAT(atof(optarg), -1.0, 1.0);
        g_dptz_warp_cfg.dptz[current_buffer_id].pan_ratio.value.v_float = atof(optarg);
        g_dptz_warp_cfg.dptz[current_buffer_id].pan_ratio.is_set = true;
        g_dptz_warp_cfg.dptz[current_buffer_id].is_set = true;
        break;
      case TILT_RATIO:
        VERIFY_BUFFER_ID(current_buffer_id);
        VERIFY_PARA_2_FLOAT(atof(optarg), -1.0, 1.0);
        g_dptz_warp_cfg.dptz[current_buffer_id].tilt_ratio.value.v_float = atof(optarg);
        g_dptz_warp_cfg.dptz[current_buffer_id].tilt_ratio.is_set = true;
        g_dptz_warp_cfg.dptz[current_buffer_id].is_set = true;
        break;
      case ZOOM_RATIO:
        VERIFY_BUFFER_ID(current_buffer_id);
        VERIFY_PARA_1_FLOAT(atof(optarg), 0.0);
        g_dptz_warp_cfg.dptz[current_buffer_id].zoom_ratio.value.v_float = atof(optarg);
        g_dptz_warp_cfg.dptz[current_buffer_id].zoom_ratio.is_set = true;
        g_dptz_warp_cfg.dptz[current_buffer_id].is_set = true;
        break;

      case LBR_ENABLE:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].lbr_enable.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_enable.is_set = true;
      break;
      case LBR_AUTO_BITRATE_TARGET:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].lbr_auto_bitrate_target.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_auto_bitrate_target.is_set = true;
        break;
      case LBR_BITRATE_CEILING:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].lbr_bitrate_ceiling.value.v_int = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_bitrate_ceiling.is_set = true;
      break;
      case LBR_FRAME_DROP:
        VERIFY_PARA_1(atoi(optarg), 0);
        g_lbr_cfg[current_stream_id].lbr_frame_drop.value.v_bool = atoi(optarg);
        g_lbr_cfg[current_stream_id].lbr_frame_drop.is_set = true;
      break;

      case ALLOC_STYLE:
        g_alloc_style.is_set = true;
        g_alloc_style.value = atoi(optarg);
        break;
      case 'b':
        current_buffer_id = atoi(optarg);
        current_stream_id = -1;
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].is_set = true;
        set_state = set_buffer;
        g_dptz_warp_cfg.buf_id.value.v_int = current_buffer_id;
        g_dptz_warp_cfg.buf_id.is_set = true;
        break;
      case INPUT_CROP:
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].input_crop.is_set = true;
        g_buffer_fmt[current_buffer_id].input_crop.value.v_bool = atoi(optarg);
        break;
      case INPUT_WIDTH:
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].input_width.is_set = true;
        g_buffer_fmt[current_buffer_id].input_width.value.v_int = atoi(optarg);
        break;
      case INPUT_HEIGHT:
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].input_height.is_set = true;
        g_buffer_fmt[current_buffer_id].input_height.value.v_int = atoi(optarg);
        break;
      case INPUT_OFFSET_X:
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].input_offset_x.is_set = true;
        g_buffer_fmt[current_buffer_id].input_offset_x.value.v_int = atoi(optarg);
        break;
      case INPUT_OFFSET_Y:
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].input_offset_y.is_set = true;
        g_buffer_fmt[current_buffer_id].input_offset_y.value.v_int = atoi(optarg);
        break;

      case 't':
        if (set_state == set_stream) {
          VERIFY_STREAM_ID(current_stream_id);
          if (!strcasecmp(optarg, "h264")) {
            g_stream_fmt[current_stream_id].type.value.v_int = H264;
          } else if (!strcasecmp(optarg, "mjpeg")) {
            g_stream_fmt[current_stream_id].type.value.v_int = MJPEG;
          } else if (!strcasecmp(optarg, "none")) {
            g_stream_fmt[current_stream_id].type.value.v_int = NONE;
          } else {
            ERROR("Wrong stream type: %s", optarg);
            return -1;
          }
          g_stream_fmt[current_stream_id].type.is_set = true;
        } else if (set_state == set_buffer) {
          VERIFY_BUFFER_ID(current_buffer_id);
          if (!strcasecmp(optarg, "off")) {
            g_buffer_fmt[current_buffer_id].type.value.v_int = OFF;
          } else if (!strcasecmp(optarg, "encode")) {
            g_buffer_fmt[current_buffer_id].type.value.v_int = ENCODE;
          } else if (!strcasecmp(optarg, "preview")) {
            g_buffer_fmt[current_buffer_id].type.value.v_int = PREVIEW;
          } else {
            ERROR("Wrong buffer type: %s", optarg);
          }
          g_buffer_fmt[current_buffer_id].type.is_set = true;
        }
        break;

      //overlay parameter
      case OSD_ADD:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        ++g_overlay_cfg[current_stream_id].num;
        area_id = g_overlay_cfg[current_stream_id].num - 1;
        g_overlay_cfg[current_stream_id].add.push_back(tmp);
        g_overlay_cfg[current_stream_id].add[area_id].enable.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].enable.value.v_int = atoi(optarg);
        INFO("enable = %d\n",g_overlay_cfg[current_stream_id].add[area_id].enable.value.v_int);
        break;
      case OSD_REMOVE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].remove.is_set = true;
        g_overlay_cfg[current_stream_id].remove.value.v_int = atoi(optarg);
        break;
      case OSD_ENABLE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].enable.is_set = true;
        g_overlay_cfg[current_stream_id].enable.value.v_int = atoi(optarg);
        break;
      case OSD_DISABLE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].disable.is_set = true;
        g_overlay_cfg[current_stream_id].disable.value.v_int = atoi(optarg);
        break;
      case OSD_DESTROY:
        destroy_overlay_flag = true;
        break;
      case OSD_WIDTH:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].width.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].width.value.v_int = atoi(optarg);
        break;
      case OSD_HEIGHT:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].height.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].height.value.v_int = atoi(optarg);
        break;
      case OSD_LAYOUT:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 4);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].layout.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].layout.value.v_int = atoi(optarg);
        break;
      case OSD_OFFSET_X:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].offset_x.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].offset_x.value.v_int = atoi(optarg);
        break;
      case OSD_OFFSET_Y:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].offset_y.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].offset_y.value.v_int = atoi(optarg);
        break;
      case OSD_TYPE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].type.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].type.value.v_int = atoi(optarg);
        INFO("type = %d\n",g_overlay_cfg[current_stream_id].add[area_id].type.value.v_int);
        break;
      case OSD_ROTATE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].rotate.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].rotate.value.v_int = atoi(optarg);
        break;
      case OSD_FONT_TYPE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].is_font_name_set = true;
        if (strlen(optarg) >= OSD_MAX_STRING) {
          printf("string size(%d) is large than max size(%d)!!!\n",
                 strlen(optarg) + 1, OSD_MAX_FILENAME);
          g_overlay_cfg[current_stream_id].add[area_id].font_name[0] = '\0';
        } else {
          strcpy(g_overlay_cfg[current_stream_id].add[area_id].font_name, optarg);
          g_overlay_cfg[current_stream_id].add[area_id].font_name[OSD_MAX_FILENAME-1] = '\0';
        }
        INFO("font = %s\n",g_overlay_cfg[current_stream_id].add[area_id].font_name);
        break;
      case OSD_FONT_SIZE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_size.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_size.value.v_int = atoi(optarg);
        break;
      case OSD_FONT_COLOR:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_color.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_color.value.v_uint = (uint32_t)atoll(optarg);
        break;
      case OSD_FONT_OUTLINE_WIDTH:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_outline_w.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_outline_w.value.v_int = atoi(optarg);
        break;
      case OSD_FONT_HOR_BOLD:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_hor_bold.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_hor_bold.value.v_int = atoi(optarg);
        break;
      case OSD_FONT_VER_BOLD:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_ver_bold.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_ver_bold.value.v_int = atoi(optarg);
        break;
      case OSD_FONT_ITALIC:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_italic.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].font_italic.value.v_int = atoi(optarg);
        break;
      case OSD_COLOR_KEY:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].color_key.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].color_key.value.v_uint = (uint32_t)atoll(optarg);
        break;
      case OSD_COLOR_RANGE:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].color_range.is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].color_range.value.v_int = atoi(optarg);
        break;
      case OSD_STRING:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].is_string_set = true;
        strcpy(g_overlay_cfg[current_stream_id].add[area_id].str, optarg);
        if (strlen(optarg) >= OSD_MAX_STRING) {
          printf("string size(%d) is large than max size(%d)!!!\n",
                 strlen(optarg) + 1, OSD_MAX_STRING);
        }
        INFO("string len = %d\n",strlen(optarg));
        g_overlay_cfg[current_stream_id].add[area_id].str[OSD_MAX_STRING-1] = '\0';
        INFO("string = %s\n",g_overlay_cfg[current_stream_id].add[area_id].str);
        break;
      case OSD_BMP:
        VERIFY_STREAM_ID(current_stream_id);
        g_overlay_cfg[current_stream_id].is_set = true;
        g_overlay_cfg[current_stream_id].add[area_id].is_bmp_set = true;
        if (strlen(optarg) >= OSD_MAX_FILENAME) {
          printf("BMP filename size(%d) is large than max size(%d)!!!\n",
                 strlen(optarg) + 1, OSD_MAX_FILENAME);
          ret = -1;
        } else {
          strcpy(g_overlay_cfg[current_stream_id].add[area_id].bmp, optarg);
          g_overlay_cfg[current_stream_id].add[area_id].bmp[OSD_MAX_FILENAME-1] = '\0';
        }
        break;
      case OSD_MAX:
        get_overlay_max_flag = true;
        break;
      case OSD_SAVE:
        save_overlay_flag = true;
        break;

      case STOP_ENCODING:
        pause_flag = true;
        break;
      case START_ENCODING:
        resume_flag = true;
        break;

      case APPLY:
        apply_flag = true;
        break;

      case SHOW_STREAM_STATUS:
        show_status_flag = true;
        break;
      case SHOW_STREAM_FMT:
        show_fmt_flag = true;
        break;
      case SHOW_STREAM_CFG:
        show_cfg_flag = true;
        break;
      case SHOW_DPTZ_WARP_CFG:
        show_dptz_warp_flag = true;
        break;
      case SHOW_LBR_CFG:
        show_lbr_flag = true;
        break;
      case SHOW_ALLOC_STYLE:
        show_style_flag = true;
        break;
      case SHOW_BUFFER_FMT:
        show_buf_flag = true;
        break;
      case SHOW_OSD_CFG:
        show_overlay_flag = true;
        break;
      case SHOW_ALL_INFO:
        show_info_flag = true;
        break;
      default:
        ret = -1;
        printf("unknown option found: %d\n", ch);
        break;
    }
  }

  return ret;
}

static bool float_equal(float a, float b)
{
  if (fabsf(a - b) < 0.00001) {
    return true;
  } else {
    return false;
  }
}

static int32_t set_vin_config()
{
  int32_t ret  = 0;

  for (int32_t i = 0; i < AM_VIN_MAX_NUM; ++i) {
    if (!g_vin_setting[i].is_set) {
      continue;
    }

    bool has_setting = false;
    am_vin_config_t config;
    memset(&config, 0, sizeof(config));
    am_service_result_t service_result;

    if (g_vin_setting[i].vin_flip.is_set) {
      config.flip = g_vin_setting[i].vin_flip.value.v_int;
      SET_BIT(config.enable_bits, AM_VIN_CONFIG_FLIP_EN_BIT);
      has_setting = true;
    }

    if (g_vin_setting[i].vin_fps.is_set) {
      float fps = g_vin_setting[i].vin_fps.value.v_float;
      if (float_equal(fps, 29.97)) {
        config.fps = 1000;
      } else if (float_equal(fps, 59.94)) {
        config.fps = 1001;
      } else if (float_equal(fps, 23.976)) {
        config.fps = 1002;
      } else if (float_equal(fps, 12.5)) {
        config.fps = 1003;
      } else if (float_equal(fps, 6.25)) {
        config.fps = 1004;
      } else if (float_equal(fps, 3.125)) {
        config.fps = 1005;
      } else if (float_equal(fps, 7.5)) {
        config.fps = 1006;
      } else if (float_equal(fps, 3.75)) {
        config.fps = 1007;
      } else {
        config.fps = static_cast<int16_t>(fps);
      }
      SET_BIT(config.enable_bits, AM_VIN_CONFIG_FPS_EN_BIT);
      has_setting = true;
    }

    if (g_vin_setting[i].vin_hdr_mode.is_set) {
      config.hdr_mode = g_vin_setting[i].vin_hdr_mode.value.v_int;
      SET_BIT(config.enable_bits, AM_VIN_CONFIG_HDR_MODE_EN_BIT);
      has_setting = true;
    }
    if (!has_setting) {
      continue;
    }
    config.vin_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_VIN_SET,
                              &config, sizeof(config),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set VIN config!\n");
      break;
    }
  }
  return ret;
}

static int32_t set_buffer_alloc_style()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_buffer_alloc_style_t style = {0};
  do {
    if (!g_alloc_style.is_set) {
      break;
    }
    style.alloc_style = g_alloc_style.value;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET,
                              &style, sizeof(style),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set VIN config!\n");
      break;
    }
  } while (0);

  return ret;
}

static int32_t set_stream_fmt_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_stream_fmt_t stream_fmt = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (!g_stream_fmt[i].is_set) {
      continue;
    }
    stream_fmt.stream_id = i;

    if (g_stream_fmt[i].enable.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_ENABLE_EN_BIT);
      stream_fmt.enable = g_stream_fmt[i].enable.value.v_bool;
      has_setting = true;
    }
    if (g_stream_fmt[i].type.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_TYPE_EN_BIT);
      stream_fmt.type = g_stream_fmt[i].type.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].source.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_SOURCE_EN_BIT);
      stream_fmt.source = g_stream_fmt[i].source.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].frame_facc_num.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_FRAME_NUM_EN_BIT);
      stream_fmt.frame_fact_num = g_stream_fmt[i].frame_facc_num.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].frame_fact_den.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_FRAME_DEN_EN_BIT);
      stream_fmt.frame_fact_den = g_stream_fmt[i].frame_fact_den.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].width.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_WIDTH_EN_BIT);
      stream_fmt.width = g_stream_fmt[i].width.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].height.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_HEIGHT_EN_BIT);
      stream_fmt.height = g_stream_fmt[i].height.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].offset_x.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_OFFSET_X_EN_BIT);
      stream_fmt.offset_x = g_stream_fmt[i].offset_x.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].offset_y.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_OFFSET_Y_EN_BIT);
      stream_fmt.offset_y = g_stream_fmt[i].offset_y.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].hflip.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_HFLIP_EN_BIT);
      stream_fmt.hflip = g_stream_fmt[i].hflip.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].vflip.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_VFLIP_EN_BIT);
      stream_fmt.vflip = g_stream_fmt[i].vflip.value.v_int;
      has_setting = true;
    }
    if (g_stream_fmt[i].rotate.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_ROTATE_EN_BIT);
      stream_fmt.rotate = g_stream_fmt[i].rotate.value.v_int;
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET,
                                &stream_fmt, sizeof(stream_fmt),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream format config!\n");
        break;
      }
    }
  }
  return ret;
}

static int32_t set_stream_cfg_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_stream_cfg_t stream_cfg = {0};
  bool has_h264_setting = false;
  bool has_mjpeg_setting = false;

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (!g_stream_cfg[i].is_set) {
      continue;
    }
    stream_cfg.stream_id = i;
    if (g_stream_cfg[i].h264_bitrate_ctrl.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_BITRATE_CTRL_EN_BIT);
      stream_cfg.h264.bitrate_ctrl = g_stream_cfg[i].h264_bitrate_ctrl.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_profile.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_PROFILE_EN_BIT);
      stream_cfg.h264.profile = g_stream_cfg[i].h264_profile.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_au_type.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_AU_TYPE_EN_BIT);
      stream_cfg.h264.au_type = g_stream_cfg[i].h264_au_type.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_chroma.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_BITRATE_CTRL_EN_BIT);
      stream_cfg.h264.chroma = g_stream_cfg[i].h264_chroma.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_m.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_M_EN_BIT);
      stream_cfg.h264.M = g_stream_cfg[i].h264_chroma.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_n.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_N_EN_BIT);
      stream_cfg.h264.N = g_stream_cfg[i].h264_n.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_idr_interval.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_IDR_EN_BIT);
      stream_cfg.h264.idr_interval = g_stream_cfg[i].h264_idr_interval.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_bitrate.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_BITRATE_EN_BIT);
      stream_cfg.h264.target_bitrate = g_stream_cfg[i].h264_bitrate.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_mv_threshold.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_MV_THRESHOLD_EN_BIT);
      stream_cfg.h264.mv_threshold = g_stream_cfg[i].h264_mv_threshold.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_enc_improve.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_ENC_IMPROVE_EN_BIT);
      stream_cfg.h264.encode_improve = g_stream_cfg[i].h264_enc_improve.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_multi_ref_p.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_MULTI_REF_P_EN_BIT);
      stream_cfg.h264.multi_ref_p = g_stream_cfg[i].h264_multi_ref_p.value.v_int;
      has_h264_setting = true;
    }
    if (g_stream_cfg[i].h264_long_term_intvl.is_set) {
      SET_BIT(stream_cfg.h264.enable_bits, AM_H264_CFG_LONG_TERM_INTVL_EN_BIT);
      stream_cfg.h264.long_term_intvl = g_stream_cfg[i].h264_long_term_intvl.value.v_int;
      has_h264_setting = true;
    }

    if (has_h264_setting) {
      SET_BIT(stream_cfg.enable_bits, AM_STREAM_CFG_H264_EN_BIT);
    }

    if (g_stream_cfg[i].mjpeg_quality.is_set) {
      SET_BIT(stream_cfg.mjpeg.enable_bits, AM_MJPEG_CFG_QUALITY_EN_BIT);
      stream_cfg.mjpeg.quality = g_stream_cfg[i].mjpeg_quality.value.v_int;
      has_mjpeg_setting = true;
    }
    if (g_stream_cfg[i].mjpeg_chroma.is_set) {
      SET_BIT(stream_cfg.mjpeg.enable_bits, AM_MJPEG_CFG_CHROMA_EN_BIT);
      stream_cfg.mjpeg.chroma = g_stream_cfg[i].mjpeg_chroma.value.v_int;
      has_mjpeg_setting = true;
    }

    if (has_mjpeg_setting) {
      SET_BIT(stream_cfg.enable_bits, AM_STREAM_CFG_MJPEG_EN_BIT);
    }

    if (has_h264_setting || has_mjpeg_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET,
                                &stream_cfg, sizeof(stream_cfg),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream cfg config!\n");
        break;
      }
    }
  }
  return ret;
}

static int32_t set_dptz_warp_config()
{
  uint32_t ret = 0;
  am_service_result_t service_result;
  am_dptz_warp_t dptz_warp_cfg = {0};
  uint32_t i = g_dptz_warp_cfg.buf_id.value.v_int;
  bool has_dptz_warp_setting = false;

  if (g_dptz_warp_cfg.warp.ldc_strength.is_set) {
    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_LDC_STRENGTH_EN_BIT);
    dptz_warp_cfg.ldc_strenght = g_dptz_warp_cfg.warp.ldc_strength.value.v_float;
    has_dptz_warp_setting = true;
  }
  if (g_dptz_warp_cfg.warp.pano_hfov_degree.is_set) {
    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_PANO_HFOV_DEGREE_EN_BIT);
    dptz_warp_cfg.pano_hfov_degree = g_dptz_warp_cfg.warp.pano_hfov_degree.value.v_float;
    has_dptz_warp_setting = true;
  }
  if (g_dptz_warp_cfg.buf_id.is_set) {
    dptz_warp_cfg.buf_id = g_dptz_warp_cfg.buf_id.value.v_int;
  }
  if (g_dptz_warp_cfg.dptz[i].pan_ratio.is_set) {
    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_PAN_RATIO_EN_BIT);
    dptz_warp_cfg.pan_ratio = g_dptz_warp_cfg.dptz[i].pan_ratio.value.v_float;
    has_dptz_warp_setting = true;
  }
  if (g_dptz_warp_cfg.dptz[i].tilt_ratio.is_set) {
    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_TILT_RATIO_EN_BIT);
    dptz_warp_cfg.tilt_ratio = g_dptz_warp_cfg.dptz[i].tilt_ratio.value.v_float;
    has_dptz_warp_setting = true;
  }
  if (g_dptz_warp_cfg.dptz[i].zoom_ratio.is_set) {
    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_ZOOM_RATIO_EN_BIT);
    dptz_warp_cfg.zoom_ratio = g_dptz_warp_cfg.dptz[i].zoom_ratio.value.v_float;
    has_dptz_warp_setting = true;
  }
  if (has_dptz_warp_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET,
                              &dptz_warp_cfg, sizeof(dptz_warp_cfg),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set dptz_warp config!\n");
    }
  }

  return ret;
}

static int32_t set_lbr_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_encode_h264_lbr_ctrl_t lbr_cfg = {0};
  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (!g_stream_cfg[i].is_set) {
      continue;
    }
    lbr_cfg.stream_id = i;
    if (g_lbr_cfg[i].lbr_enable.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_H264_LBR_ENABLE_LBR_EN_BIT);
      lbr_cfg.enable_lbr = g_lbr_cfg[i].lbr_enable.value.v_bool;
    }

    if (g_lbr_cfg[i].lbr_auto_bitrate_target.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_H264_LBR_AUTO_BITRATE_CEILING_EN_BIT);
      lbr_cfg.auto_bitrate_ceiling = g_lbr_cfg[i].lbr_auto_bitrate_target.value.v_bool;
    }

    if (g_lbr_cfg[i].lbr_bitrate_ceiling.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_H264_LBR_BITRATE_CEILING_EN_BIT);
      lbr_cfg.bitrate_ceiling = g_lbr_cfg[i].lbr_bitrate_ceiling.value.v_int;
    }

    if (g_lbr_cfg[i].lbr_frame_drop.is_set) {
      SET_BIT(lbr_cfg.enable_bits, AM_ENCODE_H264_LBR_DROP_FRAME_EN_BIT);
      lbr_cfg.drop_frame = g_lbr_cfg[i].lbr_frame_drop.value.v_bool;
    }
  }

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET,
                            &lbr_cfg, sizeof(lbr_cfg),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to set lbr cfg config!\n");
  }

  return ret;
}

static int32_t set_buffer_fmt_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_buffer_fmt_t buffer_fmt = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
    if (!g_buffer_fmt[i].is_set) {
      continue;
    }
    buffer_fmt.buffer_id = i;

    if (g_buffer_fmt[i].type.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_TYPE_EN_BIT);
      buffer_fmt.type = g_buffer_fmt[i].type.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].input_crop.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_CROP_EN_BIT);
      buffer_fmt.input_crop = g_buffer_fmt[i].input_crop.value.v_bool;
      has_setting = true;
    }
    if (g_buffer_fmt[i].input_width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT);
      buffer_fmt.input_width = g_buffer_fmt[i].input_width.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].input_height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
      buffer_fmt.input_height = g_buffer_fmt[i].input_height.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].input_offset_x.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT);
      buffer_fmt.input_offset_x = g_buffer_fmt[i].input_offset_x.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].input_offset_y.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT);
      buffer_fmt.input_offset_y = g_buffer_fmt[i].input_offset_y.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
      buffer_fmt.width = g_buffer_fmt[i].width.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
      buffer_fmt.input_height = g_buffer_fmt[i].input_height.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].prewarp.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_PREWARP_EN_BIT);
      buffer_fmt.prewarp = g_buffer_fmt[i].prewarp.value.v_bool;
      has_setting = true;
    }

    if (has_setting) {
      INFO("Buffer[%d]", i);
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET,
                                &buffer_fmt, sizeof(buffer_fmt),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream buffer format!\n");
        break;
      }
    }
  }

  return ret;
}

static int32_t start_encoding()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to start encoding!\n");
  }
  return ret;
}

static int32_t stop_encoding()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to stop encoding!\n");
  }
  return ret;
}

static int32_t apply_config()
{
  int32_t ret = 0;

  do {
    if ((ret = stop_encoding()) < 0) {
      break;
    }
    if ((ret = start_encoding()) < 0) {
      break;
    }
  } while (0);
  return ret;
}

static int32_t show_stream_status()
{
  int32_t ret = 0;
  am_stream_status_t *status = nullptr;
  am_service_result_t service_result;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("Failed to Get Stream format!");
    return -1;
  }
  status = (am_stream_status_t*)service_result.data;

  printf("\n");
  uint32_t encode_count = 0;
  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (TEST_BIT(status->status, i)) {
      ++encode_count;
      PRINTF("Stream[%d] is encoding...", i);
    }
  }
  if (encode_count == 0) {
    PRINTF("No stream in encoding!");
  }

  return ret;
}

static int32_t show_stream_fmt()
{
  int32_t ret = 0;
  uint32_t stream_id;
  am_stream_fmt_t *fmt = nullptr;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Stream format!");
      break;
    }
    fmt = (am_stream_fmt_t*)service_result.data;

    printf("\n");
    if (fmt->enable) {
      PRINTF("[Stream%d Format](enable)", stream_id);
    } else {
      PRINTF("[Stream%d Format](disable)", stream_id);
    }
    switch (fmt->type) {
      case 0:
        PRINTF("  Type:\t\t\tNone");
        break;
      case 1:
        PRINTF("  Type:\t\t\tH264");
        break;
      case 2:
        PRINTF("  Type:\t\t\tMJPEG");
        break;
      default:
        PRINTF("  Type:\t\t\t%d, error!", fmt->type);
        break;
    }
    switch (fmt->source) {
      case 0:
        PRINTF("  Source:\t\tMain buffer");
        break;
      case 1:
        PRINTF("  Source:\t\t2nd buffer");
        break;
      case 2:
        PRINTF("  Source:\t\t3rd buffer");
        break;
      case 3:
        PRINTF("  Source:\t\t4th buffer");
        break;
      default:
        PRINTF(" Source:\t\t%d, error!", fmt->source);
        break;
    }

    PRINTF("  Frame factor:\t\t%d/%d",
           fmt->frame_fact_num, fmt->frame_fact_den);
    if ((int32_t)fmt->width < 0 || (int32_t)fmt->height < 0) {
      PRINTF("  Size:\t\t\tAuto");
    } else {
      PRINTF("  Size:\t\t\t%dx%d", fmt->width, fmt->height);
    }
    PRINTF("  Offset:\t\t(%d, %d)", fmt->offset_x, fmt->offset_y);
    if (fmt->hflip) {
      PRINTF("  Hflip:\t\tEnable");
    } else {
      PRINTF("  Hflip:\t\tDisable");
    }

    if (fmt->vflip) {
      PRINTF("  Vflip:\t\tEnable");
    } else {
      PRINTF("  Vflip:\t\tDisable");
    }

    if (fmt->rotate) {
      PRINTF("  Rotate:\t\tEnable");
    } else {
      PRINTF("  Rotate:\t\tDisable");
    }
  }
  return ret;
}

static int32_t show_stream_cfg()
{
  int32_t ret = 0;
  uint32_t stream_id;
  am_stream_cfg_t *cfg = NULL;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Stream config!");
      break;
    }

    cfg = (am_stream_cfg_t*)service_result.data;

    printf("\n");
    PRINTF("[Stream%d Config]", stream_id);
    PRINTF("[H264]");
    PRINTF("  Bitrate:\t\t%d", cfg->h264.target_bitrate);
    switch (cfg->h264.bitrate_ctrl) {
      case 0:
        PRINTF("  Bitrate Control:\tCBR");
        break;
      case 1:
        PRINTF("  Bitrate Control:\tVBR");
        break;
      case 2:
        PRINTF("  Bitrate Control:\tCBR Quality");
        break;
      case 3:
        PRINTF("  Bitrate Control:\tVBR Quality");
        break;
      case 4:
        PRINTF("  Bitrate Control:\tCBR2");
        break;
      case 5:
        PRINTF("  Bitrate Control:\tVBR2");
        break;
      case 6:
        PRINTF("  Bitrate Control:\tLBR");
        break;
      default:
        PRINTF("  Bitrate Control:\t%d, error!", cfg->h264.bitrate_ctrl);
        break;
    }

    switch (cfg->h264.profile) {
      case 0:
        PRINTF("  Profile:\t\tBaseline");
        break;
      case 1:
        PRINTF("  Profile:\t\tMain");
        break;
      case 2:
        PRINTF("  Profile:\t\tHigh");
        break;
      default:
        PRINTF("  Profile:\t\t%d, error!", cfg->h264.profile);
        break;
    }

    switch (cfg->h264.au_type) {
      case 0:
        PRINTF("  AU type:\t\tNO AUD NO SEI");
        break;
      case 1:
        PRINTF("  AU type:\t\tAUD BEFORE SPS WITH SEI");
        break;
      case 2:
        PRINTF("  AU type:\t\tAUD AFTER SPS WITH SEI");
        break;
      case 3:
        PRINTF("  AU type:\t\tNO AUD WITH SEI");
        break;
      default:
        PRINTF("  AU type:\t\t%d, error!", cfg->h264.au_type);
        break;
    }

    switch (cfg->h264.chroma) {
      case 0:
        PRINTF("  Chroma format:\t420");
        break;
      case 1:
        PRINTF("  Chroma format:\t422");
        break;
      case 2:
        PRINTF("  Chroma format:\tMono");
        break;
      default:
        PRINTF("  Chroma format:\t%d, error!", cfg->h264.chroma);
        break;
    }
    PRINTF("  M:\t\t\t%d", cfg->h264.M);
    PRINTF("  N:\t\t\t%d", cfg->h264.N);
    PRINTF("  IDR interval:\t\t%d", cfg->h264.idr_interval);
    PRINTF("  MV threshold:\t\t%d", cfg->h264.mv_threshold);
    PRINTF("  Encode improve:\t%d", cfg->h264.encode_improve);
    PRINTF("  Multi ref P:\t\t%d", cfg->h264.multi_ref_p);
    PRINTF("  Long term interval:\t%d", cfg->h264.long_term_intvl);
    // MJPEG
    PRINTF("[MJPEG]");
    PRINTF("  Quality:\t\t%d", cfg->mjpeg.quality);
    switch (cfg->mjpeg.chroma) {
      case 0:
        PRINTF("  Chroma Format:\t420");
        break;
      case 1:
        PRINTF("  Chroma Format:\t422");
        break;
      case 2:
        PRINTF("  Chroma Format:\tMono");
        break;
      default:
        PRINTF("  Chroma Format:\t%d, error!", cfg->mjpeg.chroma);
        break;
    }
  }
  return ret;
}

static int32_t show_dptz_warp_cfg()
{
  int32_t ret = 0;
  uint32_t buffer_id;
  am_dptz_warp_t *cfg = NULL;
  am_service_result_t service_result;
  for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
    buffer_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET,
                              &buffer_id, sizeof(buffer_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get dptz warp config!");
      break;
    }
    cfg = (am_dptz_warp_t*)service_result.data;
    PRINTF("[Buffer%d dptz config]", buffer_id);
    PRINTF("pan_ratio:\t\t%.02f", cfg->pan_ratio);
    PRINTF("tilt_ratio:\t\t%.02f", cfg->tilt_ratio);
    PRINTF("zoom_ratio:\t\t%.02f", cfg->zoom_ratio);
    PRINTF("\n");
  }
  PRINTF("[warp config]");
  PRINTF("ldc_strength:\t\t%.02f", cfg->ldc_strenght);
  PRINTF("pano_hfov_degree:\t%.02f", cfg->pano_hfov_degree);

  return ret;
}

static int32_t show_lbr_cfg()
{
  int32_t ret = 0;
  uint32_t stream_id;
  am_encode_h264_lbr_ctrl_t *cfg = NULL;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get lbr config!");
      break;
    }

    cfg = (am_encode_h264_lbr_ctrl_t*)service_result.data;

    PRINTF("\n");
    PRINTF("[Stream%d LBR Config]", stream_id);
    PRINTF("enable_lbr:\t\t%d", cfg->enable_lbr);
    PRINTF("auto_bitrate_ceiling:\t%d", cfg->auto_bitrate_ceiling);
    PRINTF("bitrate_ceiling:\t%d", cfg->bitrate_ceiling);
    PRINTF("drop_frame:\t\t%d", cfg->drop_frame);
  }
  return ret;
}

static int32_t show_buffer_alloc_style()
{
  int32_t ret = 0;
  am_buffer_alloc_style_t *style;
  am_service_result_t service_result;
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET,
                              NULL, 0,
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Buffer allocate style!");
      break;
    }

    style = (am_buffer_alloc_style_t*)service_result.data;

    printf("\n");
    if (style->alloc_style) {
      PRINTF("Buffer alloc style: \tAuto");
    } else {
      PRINTF("Buffer alloc style: \tManual");
    }
  } while (0);

  return ret;
}

static int32_t show_buffer_fmt()
{
  int32_t ret = 0;
  uint32_t buffer_id;
  am_buffer_fmt_t *fmt = NULL;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < AM_ENCODE_SOURCE_BUFFER_MAX_NUM; ++i) {
    buffer_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET,
                              &buffer_id, sizeof(buffer_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Buffer format!");
      break;
    }

    fmt = (am_buffer_fmt_t*)service_result.data;

    printf("\n");
    PRINTF("[Buffer%d Format]", buffer_id);
    switch (fmt->type) {
      case 0:
        PRINTF("  Type:\t\t\tOFF");
        break;
      case 1:
        PRINTF("  Type:\t\t\tENCODE");
        break;
      case 2:
        PRINTF("  Type:\t\t\tPREVIEW");
        break;
      default:
        PRINTF("  Type:\t\t\t%d, error!", fmt->type);
        break;
    }
    PRINTF("  Width:\t\t%d", fmt->width);
    PRINTF("  Height:\t\t%d", fmt->height);
    PRINTF("  Input crop:\t\t%d", fmt->input_crop);
    if (fmt->input_crop) {
      PRINTF("  Input width:\t\t%d", fmt->input_width);
      PRINTF("  Input height:\t\t%d", fmt->input_height);
      PRINTF("  Input offset x:\t%d", fmt->input_offset_x);
      PRINTF("  Input offset y:\t%d", fmt->input_offset_y);
    }
    PRINTF("  Prewarp:\t\t%d", fmt->prewarp);
  }
  return ret;
}

static int32_t set_force_idr()
{
  int32_t ret = 0;
  am_service_result_t service_result;

  for (int32_t i = 0; i < AM_STREAM_MAX_NUM; i++) {
    if (force_idr_id & (1 << i)) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR,
                                &i, sizeof(int32_t),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("Failed to set stream%d to force idr!");
        break;
      }
    }
  }

  return ret;
}

static void get_overlay_max_num()
{
  int32_t ret = 0;
  if (!g_overlay_max_num) {
    am_service_result_t service_result;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM, nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to get overlay max number!\n");
    } else {
      g_overlay_max_num = (uint32_t)*service_result.data;
    }
  }
}

static void show_overlay_max()
{
  get_overlay_max_num();
  PRINTF("The platform overlay support max %d areas for a stream!\n",g_overlay_max_num);
}

static int32_t show_overlay_cfg()
{
  int32_t ret = 0;
  am_overlay_id_t input_para = {0};
  am_service_result_t service_result;

  get_overlay_max_num();
  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    input_para.stream_id = i;
    PRINTF("\n");
    PRINTF("[Stream %d overlay Config]", i);
    for (uint32_t j=0; j<g_overlay_max_num; ++j) {
      input_para.area_id = j;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET,
                                &input_para, sizeof(input_para),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to get overlay config!\n");
        break;
      }
      am_overlay_area_t *area = (am_overlay_area_t*)service_result.data;

      PRINTF("  Area:%d (%s): \n",j, area->enable == 1 ? "enable" :
          area->enable == 0 ? "disable" : "not created");
      PRINTF("    rotate:\t\t\t%d\n", area->rotate);
      PRINTF("    size:\t\t\t%d x %d\n", area->width, area->height);
      if(4 == area->layout) {
        PRINTF("    offset:\t\t\t%d x %d\n", area->offset_x, area->offset_y);
      } else {
        PRINTF("    position:\t\t\t%s\n", area->layout == 0 ? "Left Top" :
            area->layout == 1 ? "Right Top":
                area->layout == 2 ? "Left Bottom": "Right Bottom");
      }
      PRINTF("    osd type:\t\t\t%s\n",area->type == 0 ? "string text" :
          area->type == 1 ? "time text" :
              area->type == 2 ? "bmp" : "pattern test");
      if (0 == area->type || 1 == area->type) {
        PRINTF("    font size:\t\t\t%d\n", area->font_size);
        if (area->font_color < 8){
          PRINTF("    font color:\t\t\t%d\n", area->font_color);
        } else {
          PRINTF("    font color:\t\t\t0x%x\n", area->font_color);
        }
        PRINTF("    font outline_width:\t\t%d\n", area->font_outline_w);
        PRINTF("    font hor_bold:\t\t%d\n", area->font_hor_bold);
        PRINTF("    font ver_bold:\t\t%d\n", area->font_ver_bold);
        PRINTF("    font italic:\t\t%d\n", area->font_italic);
      }
      if (2 == area->type) {
        PRINTF("    color key:\t\t\t0x%x\n", area->color_key);
        PRINTF("    color range:\t\t%d\n", area->color_range);
      }
    }
  }
  return ret;
}

static int32_t set_overlay_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  bool has_setting = false;

  get_overlay_max_num();
  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
    if (!g_overlay_cfg[i].is_set) {
      continue;
    }
    //ADD
    for (uint32_t j=0; j<g_overlay_cfg[i].num; ++j) {
      if (i >= g_overlay_max_num) {
        WARN("The overlay max number is %d \n",g_overlay_max_num);
        continue; //free left memory
      }
      am_overlay_s add_cfg = {0};
      if (g_overlay_cfg[i].add[j].enable.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_ADD_EN_BIT);
        add_cfg.stream_id = i;
        add_cfg.area.enable = g_overlay_cfg[i].add[j].enable.value.v_int;
        INFO("Add stream:%d area:%d osd\n",i, j);
        INFO("enable = %d\n",add_cfg.area.enable);
      } else {
        break;
      }
      if (g_overlay_cfg[i].add[j].width.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_WIDTH_EN_BIT);
        add_cfg.area.width = g_overlay_cfg[i].add[j].width.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].height.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_HEIGHT_EN_BIT);
        add_cfg.area.height = g_overlay_cfg[i].add[j].height.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].layout.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_LAYOUT_EN_BIT);
        add_cfg.area.layout = g_overlay_cfg[i].add[j].layout.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].offset_x.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_OFFSETX_EN_BIT);
        add_cfg.area.offset_x = g_overlay_cfg[i].add[j].offset_x.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].offset_y.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_OFFSETY_EN_BIT);
        add_cfg.area.offset_y = g_overlay_cfg[i].add[j].offset_y.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].type.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_TYPE_EN_BIT);
        add_cfg.area.type = g_overlay_cfg[i].add[j].type.value.v_int;
        has_setting = true;
        INFO("type = %d\n",add_cfg.area.type);
      }
      if (g_overlay_cfg[i].add[j].rotate.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_ROTATE_EN_BIT);
        add_cfg.area.rotate = g_overlay_cfg[i].add[j].rotate.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].is_font_name_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_TYPE_EN_BIT);
        uint32_t len = strlen(g_overlay_cfg[i].add[j].font_name);
        if (len >= OSD_MAX_FILENAME) {
          WARN("BMP filename size is too long(%d bytes)! \n", len);
          add_cfg.area.font_type[0] = '\0';
        } else {
          strncpy(add_cfg.area.font_type, g_overlay_cfg[i].add[j].font_name, len);
          add_cfg.area.font_type[OSD_MAX_FILENAME-1] = '\0';
        }
        has_setting = true;
      } else if (add_cfg.area.type == 0 || add_cfg.area.type == 1) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_TYPE_EN_BIT);
        strcpy(add_cfg.area.font_type, "/usr/share/fonts/DroidSans.ttf");
      }
      if (g_overlay_cfg[i].add[j].font_size.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_SIZE_EN_BIT);
        add_cfg.area.font_size = g_overlay_cfg[i].add[j].font_size.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].font_color.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_COLOR_EN_BIT);
        add_cfg.area.font_color = g_overlay_cfg[i].add[j].font_color.value.v_uint;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].font_outline_w.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_OUTLINE_W_EN_BIT);
        add_cfg.area.font_outline_w = g_overlay_cfg[i].add[j].font_outline_w.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].font_hor_bold.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_HOR_BOLD_EN_BIT);
        add_cfg.area.font_hor_bold = g_overlay_cfg[i].add[j].font_hor_bold.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].font_ver_bold.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_VER_BOLD_EN_BIT);
        add_cfg.area.font_ver_bold = g_overlay_cfg[i].add[j].font_ver_bold.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].font_italic.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_FONT_ITALIC_EN_BIT);
        add_cfg.area.font_italic = g_overlay_cfg[i].add[j].font_italic.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].is_string_set) {
        uint32_t len = strlen(g_overlay_cfg[i].add[j].str);
        if (len >= OSD_MAX_STRING) {
          WARN("String is too long(%d bytes) to insert! \n", len);
        }
        SET_BIT(add_cfg.enable_bits, AM_STRING_EN_BIT);
        strncpy(add_cfg.area.str, g_overlay_cfg[i].add[j].str, len);
        add_cfg.area.str[len] = '\0';
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].color_key.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_COLOR_KEY_EN_BIT);
        add_cfg.area.color_key = g_overlay_cfg[i].add[j].color_key.value.v_uint;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].color_range.is_set) {
        SET_BIT(add_cfg.enable_bits, AM_COLOR_RANGE_EN_BIT);
        add_cfg.area.color_range = g_overlay_cfg[i].add[j].color_range.value.v_int;
        has_setting = true;
      }
      if (g_overlay_cfg[i].add[j].is_bmp_set) {
        uint32_t len = strlen(g_overlay_cfg[i].add[j].bmp);
        if (len >= OSD_MAX_FILENAME) {
          WARN("BMP filename size is too long(%d bytes)! \n", len);
          continue;
        }
        SET_BIT(add_cfg.enable_bits, AM_BMP_EN_BIT);
        strncpy(add_cfg.area.bmp, g_overlay_cfg[i].add[j].bmp, len);
        add_cfg.area.bmp[len] = '\0';
        has_setting = true;
      } else if (add_cfg.area.type == 2) {
        SET_BIT(add_cfg.enable_bits, AM_BMP_EN_BIT);
        strcpy(add_cfg.area.bmp, "/usr/share/oryx/video/overlay/Ambarella-256x128-8bit.bmp");
      }

      if (has_setting) {
        g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD,
                                  &add_cfg, sizeof(add_cfg),
                                  &service_result, sizeof(service_result));
        if ((ret = service_result.ret) != 0) {
          ERROR("failed to set overlay config!\n");
          break;
        }
      }
    }//end of ADD

    //SET
    has_setting = false;
    am_overlay_set_t set_cfg = {0};
    set_cfg.osd_id.stream_id = i;
    if (g_overlay_cfg[i].remove.is_set) {
      SET_BIT(set_cfg.enable_bits, AM_REMOVE_EN_BIT);
      set_cfg.osd_id.area_id = g_overlay_cfg[i].remove.value.v_int;
      INFO("Remove stream:%d area:%d osd\n",i, set_cfg.osd_id.area_id);
      has_setting = true;
    } //remove

    if (g_overlay_cfg[i].enable.is_set) {
      if (g_overlay_cfg[i].remove.is_set) {
        WARN("Don't do enable and remove action in a "
            "same stream at the same time! \n");
        continue;
      }
      SET_BIT(set_cfg.enable_bits, AM_ENABLE_EN_BIT);
      set_cfg.osd_id.area_id = g_overlay_cfg[i].enable.value.v_int;
      INFO("Enable stream:%d area:%d osd\n",i, set_cfg.osd_id.area_id);
      has_setting = true;
    } //enable

    if (g_overlay_cfg[i].disable.is_set) {
      if (g_overlay_cfg[i].remove.is_set) {
        WARN("Don't do disable and remove action in a "
            "same stream at the same time! \n");
        continue;
      } else if (g_overlay_cfg[i].enable.is_set) {
        WARN("Don't do disable and enable action in a "
            "same stream at the same time! \n");
        continue;
      }
      SET_BIT(set_cfg.enable_bits, AM_DISABLE_EN_BIT);
      set_cfg.osd_id.area_id = g_overlay_cfg[i].disable.value.v_int;
      INFO("Disable stream:%d area:%d osd\n",i, set_cfg.osd_id.area_id);
      has_setting = true;
    } //disable

    //if area_id is g_overlay_max_num, it means manipulate all overlay area at one time
    if (set_cfg.osd_id.area_id > g_overlay_max_num) {
      WARN("The overlay max number is %d \n",g_overlay_max_num);
      continue;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_SET,
                                &set_cfg, sizeof(set_cfg),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set overlay config!\n");
        break;
      }
    }
  } //stream loop
  return ret;
}

static int32_t destroy_overlay()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to destroy overlay!\n");
  }
  return ret;
}

static int32_t save_overlay_config()
{
  int32_t ret = 0;
  INFO("save overlay configure!!!\n");
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to destroy overlay!\n");
  }
  return ret;
}

static int32_t show_all_info()
{
  int32_t ret = 0;
  do {
    if ((ret = show_stream_status()) < 0) {
      break;
    }
    if ((ret = show_stream_fmt()) < 0) {
      break;
    }
    if ((ret = show_stream_cfg()) < 0) {
      break;
    }
    if ((ret = show_dptz_warp_cfg()) < 0) {
      break;
    }
    if ((ret = show_lbr_cfg()) < 0) {
      break;
    }
    if ((ret = show_buffer_alloc_style()) < 0) {
      break;
    }
    if ((ret = show_buffer_fmt()) < 0) {
      break;
    }
    if ((ret = show_overlay_cfg()) < 0) {
      break;
    }
  } while (0);
  return ret;
}

int32_t main(int32_t argc, char **argv)
{
  if (argc < 2) {
    usage(argc, argv);
    return -1;
  }

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (init_param(argc, argv) < 0) {
    return -1;
  }

  int32_t ret = 0;
  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }

  do {
    if (show_info_flag) {
      show_all_info();
      break;
    } else {
      if (show_status_flag) {
        show_stream_status();
      }
      if (show_fmt_flag) {
        show_stream_fmt();
      }
      if (show_cfg_flag) {
        show_stream_cfg();
      }
      if (show_dptz_warp_flag) {
        show_dptz_warp_cfg();
      }
      if (show_lbr_flag) {
        show_lbr_cfg();
      }
      if (show_style_flag) {
        show_buffer_alloc_style();
      }
      if (show_buf_flag) {
        show_buffer_fmt();
      }
      if (show_overlay_flag) {
        show_overlay_cfg();
      }
      if (show_status_flag ||
          show_fmt_flag ||
          show_cfg_flag ||
          show_dptz_warp_flag ||
          show_lbr_flag ||
          show_style_flag ||
          show_buf_flag ||
          show_overlay_flag) {
        break;
      }
    }

    if (apply_flag) {
      if ((ret = apply_config()) < 0) {
        break;
      }
    } else if (resume_flag) {
      if ((ret = start_encoding()) < 0) {
        break;
      }
    } else if (pause_flag) {
      if ((ret = stop_encoding()) < 0) {
        break;
      }
    }

    if ((ret = set_vin_config()) < 0) {
      break;
    }
    if ((ret = set_buffer_alloc_style()) < 0) {
      break;
    }
    if ((ret = set_stream_fmt_config()) < 0) {
      break;
    }
    if ((ret = set_stream_cfg_config()) < 0) {
      break;
    }
    if ((ret = set_dptz_warp_config()) < 0) {
      break;
    }
    if ((ret = set_lbr_config()) < 0) {
      break;
    }
    if ((ret = set_buffer_fmt_config()) < 0) {
      break;
    }

    if(force_idr_id) {
      if ((ret = set_force_idr()) < 0) {
        break;
      }
    }

    if (destroy_overlay_flag) {
      if ((ret = destroy_overlay()) < 0) {
        break;
      }
    }
    if (get_overlay_max_flag) {
      show_overlay_max();
    }
    if (save_overlay_flag) {
      if ((ret = save_overlay_config()) < 0) {
        break;
      }
    }
    if ((ret = set_overlay_config()) < 0) {
      break;
    }

  } while (0);

  return ret;
}
