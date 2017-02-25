/*
 * test_video_service_cfg_air_api.cpp
 *
 *  History:
 *		Nov 27, 2015 - [Shupeng Ren] created file
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
      if (((x) < 0) || ((x) >= STREAM_MAX_NUM)) { \
        printf ("Stream id wrong: %d \n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_BUFFER_ID(x) \
    do { \
      if (((x) < 0) || ((x) >= SOURCE_BUFFER_MAX_NUM)) { \
        printf ("Buffer id wrong: %d \n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_VOUT_ID(x) \
    do { \
      if (((x) < 0) || ((x) >= AM_VOUT_MAX_NUM)) { \
        printf ("Vout id wrong: %d \n", (x)); \
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

static AMAPIHelperPtr g_api_helper = nullptr;
static bool show_info_flag = false;
static bool show_buf_flag = false;
static bool show_fmt_flag = false;
static bool show_feature_flag = false;
static bool show_cfg_flag = false;

static bool apply_config_flag = false;

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
    setting_option flip;
    setting_option fps;
};

struct vout_setting {
    bool is_set;
    setting_option type;
    setting_option video_type;
    setting_option flip;
    setting_option rotate;
    setting_option fps;
    bool is_mode_set;
    char mode[VOUT_MAX_CHAR_NUM];
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

struct stream_fmt_setting {
    bool is_set;
    setting_option enable;
    setting_option type;
    setting_option source;
    setting_option framerate;
    setting_option width;
    setting_option height;
    setting_option offset_x;
    setting_option offset_y;
    setting_option hflip;
    setting_option vflip;
    setting_option rotate;
};

struct stream_h26x_setting {
    bool is_set;
    setting_option h26x_bitrate_ctrl;
    setting_option h26x_profile;
    setting_option h26x_au_type;
    setting_option h26x_chroma;
    setting_option h26x_m;
    setting_option h26x_n;
    setting_option h26x_idr_interval;
    setting_option h26x_bitrate;
    setting_option h26x_mv_threshold;
    setting_option h26x_flat_area_improve;
    setting_option h26x_multi_ref_p;
    setting_option h26x_fast_seek_intvl;
};

struct stream_mjpeg_setting {
    bool is_set;
    setting_option mjpeg_quality;
    setting_option mjpeg_chroma;
};

struct lbr_cfg_setting {
    bool is_set;
    setting_option lbr_enable;
    setting_option lbr_auto_bitrate_target;
    setting_option lbr_bitrate_ceiling;
    setting_option lbr_frame_drop;
};

struct feature_setting {
    bool is_set;
    setting_option mode;
    setting_option hdr;
    setting_option iso;
    setting_option dewarp_func;
    setting_option dptz;
    setting_option bitrate_ctrl;
    setting_option overlay;
    setting_option hevc;
    setting_option iav_ver;
};

#define SOURCE_BUFFER_MAX_NUM 4
#define STREAM_MAX_NUM 4

static vin_setting g_vin_setting[AM_VIN_MAX_NUM];
static vout_setting g_vout_setting[AM_VOUT_MAX_NUM];
static buffer_fmt_setting g_buffer_fmt[SOURCE_BUFFER_MAX_NUM];
static stream_fmt_setting g_stream_fmt[STREAM_MAX_NUM];
static stream_h26x_setting g_stream_h26x[STREAM_MAX_NUM];
static stream_mjpeg_setting g_stream_mjpeg[STREAM_MAX_NUM];
static feature_setting g_feature_setting;

enum numeric_short_options {
  NONE = 0,
  H264 = 1,
  H265 = 2,
  MJPEG = 3,

  OFF = 0,
  ENCODE = 1,
  PREVIEW = 2,

  CHROMA_420 = 0,
  CHROMA_422 = 1,
  CHROMA_MONO = 2,

  NEMERIC_BEGIN = 'z',

  VIN_FLIP,
  VIN_FPS,

  //vout
  VOUT_TYPE,
  VOUT_VIDEO_TYPE,
  VOUT_MODE,
  VOUT_FLIP,
  VOUT_ROTATE,
  VOUT_FPS,

  //buffer
  INPUT_CROP,
  INPUT_WIDTH,
  INPUT_HEIGHT,
  INPUT_OFFSET_X,
  INPUT_OFFSET_Y,

  //stream format
  STREAM_ENABLE,
  STREAM_SOURCE,
  STREAM_FRAMERATE,
  STREAM_OFFSET_X,
  STREAM_OFFSET_Y,
  STREAM_HFLIP,
  STREAM_VFLIP,
  STREAM_ROTATE,

  //stream config
  BITRATE_CTRL,
  PROFILE,
  AU_TYPE,
  H26x_CHROMA,
  IDR_INTERVAL,
  BITRATE,
  MV_THRESHOLD,
  FLAT_AREA_IMPROVE,
  MULTI_REF_P,
  FAST_SEEK_INTVL,
  MJPEG_QUALITY,
  MJPEG_CHROMA,

  //feature config
  FEATURE_MODE,
  FEATURE_HDR,
  FEATURE_ISO,
  FEATURE_DEWARP_FUNC,
  FEATURE_DPTZ,
  FEATURE_BITRATE_CTRL,
  FEATURE_OVERLAY,
  FEATURE_HEVC,
  FEATURE_IAV_VER,

  APPLY,

  SHOW_BUFFER_FMT,
  SHOW_STREAM_FMT,
  SHOW_STREAM_CFG,
  SHOW_FEATURE_CFG,

  SHOW_ALL_INFO
};

static struct option long_options[] =
{
 {"help",               NO_ARG,   0,  'h'},

 {"vin-flip",           HAS_ARG,  0,  VIN_FLIP},
 {"vin-fps",            HAS_ARG,  0,  VIN_FPS},

 {"vout",               HAS_ARG,  0,  'V'},
 {"vout-type",          HAS_ARG,  0,  VOUT_TYPE},
 {"vout-video-type",    HAS_ARG,  0,  VOUT_VIDEO_TYPE},
 {"vout-mode",          HAS_ARG,  0,  VOUT_MODE},
 {"vout-flip",          HAS_ARG,  0,  VOUT_FLIP},
 {"vout-rotate",        HAS_ARG,  0,  VOUT_ROTATE},
 {"vout-fps",           HAS_ARG,  0,  VOUT_FPS},

 {"buffer",             HAS_ARG,  0,  'b'},

 {"type",               HAS_ARG,  0,  't'},
 {"input-crop",         HAS_ARG,  0,  INPUT_CROP},
 {"input-w",            HAS_ARG,  0,  INPUT_WIDTH},
 {"input-h",            HAS_ARG,  0,  INPUT_HEIGHT},
 {"input-x",            HAS_ARG,  0,  INPUT_OFFSET_X},
 {"input-y",            HAS_ARG,  0,  INPUT_OFFSET_Y},
 {"width",              HAS_ARG,  0,  'W'},
 {"height",             HAS_ARG,  0,  'H'},

 {"stream",             HAS_ARG,  0,  's'},
 {"enable",             HAS_ARG,  0,  STREAM_ENABLE},
 {"type",               HAS_ARG,  0,  't'},
 {"source",             HAS_ARG,  0,  STREAM_SOURCE},
 {"framerate",          HAS_ARG,  0,  STREAM_FRAMERATE},
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
 {"h-chroma",           HAS_ARG,  0,  H26x_CHROMA},
 {"gop-m",              HAS_ARG,  0,  'm'},
 {"gop-n",              HAS_ARG,  0,  'n'},
 {"idr",                HAS_ARG,  0,  IDR_INTERVAL},
 {"bitrate",            HAS_ARG,  0,  BITRATE},
 {"mv",                 HAS_ARG,  0,  MV_THRESHOLD},
 {"flat-area-improve",  HAS_ARG,  0,  FLAT_AREA_IMPROVE},
 {"multi-ref-p",        HAS_ARG,  0,  MULTI_REF_P},
 {"fast-seek-intvl",    HAS_ARG,  0,  FAST_SEEK_INTVL},
 {"m-quality",          HAS_ARG,  0,  MJPEG_QUALITY},
 {"m-chroma",           HAS_ARG,  0,  MJPEG_CHROMA},

 {"feature-mode",       HAS_ARG,  0,  FEATURE_MODE},
 {"feature-hdr",        HAS_ARG,  0,  FEATURE_HDR},
 {"feature-iso",        HAS_ARG,  0,  FEATURE_ISO},
 {"feature-dewarp",     HAS_ARG,  0,  FEATURE_DEWARP_FUNC},
 {"feature-dptz",       HAS_ARG,  0,  FEATURE_DPTZ},
 {"feature-bitrate",    HAS_ARG,  0,  FEATURE_BITRATE_CTRL},
 {"feature-overlay",    HAS_ARG,  0,  FEATURE_OVERLAY},
 {"feature-hevc",       HAS_ARG,  0,  FEATURE_HEVC},
 {"feature-iav-ver",    HAS_ARG,  0,  FEATURE_IAV_VER},

 {"apply",              NO_ARG,  0,   APPLY},

 {"show-buffer-fmt",    NO_ARG,   0,  SHOW_BUFFER_FMT},
 {"show-stream-fmt",    NO_ARG,   0,  SHOW_STREAM_FMT},
 {"show-stream-cfg",    NO_ARG,   0,  SHOW_STREAM_CFG},
 {"show-feature-cfg",   NO_ARG,   0,  SHOW_FEATURE_CFG},

 {"show-all-info",      NO_ARG,   0,  SHOW_ALL_INFO},
 {0, 0, 0, 0}
};

static const char *short_options = "hb:t:W:H:s:m:n:V:";

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
 {"",     "\t\t\t"  "VIN framerate. 0: auto; Set to config file\n"},

 {"0-1",  "\t\t"    "VOUT ID. 0: VOUTB; 1:VOUTA"},
 {"0-4",  "\t\t"    "VOUT physically type. 0: none; 1: LCD; 2: HDMI; 3: CVBS; 4:YPbPr"},
 {"0-6",  "\t"      "VOUT video type. 0: none; 1: YUV BT601; 2: YUV BT656; "
                    "3: YUV BT1120; 4:RGB BT601; 5: RGB BT656; 6:RGB BT1120"},
 {"",     "\t\t\t"  "VOUT mode: 480i/576i/480p/576p/720p/1080p..."},
 {"0-3",  "\t\t"    "VOUT flip. 0: none; 1:v-flip; 2: h-flip; 3:both-flip"},
 {"0-1",  "\t\t"    "VOUT rotate. 0: none; 1:90 degree rotate"},
 {"",     "\t\t\t"  "VOUT framerate. 0: auto\n"},

 {"0-3",  "\t\t"    "Source buffer ID. 0: Main buffer; 1: 2nd buffer; "
                    "2: 3rd buffer; 3: 4th buffer. Set to config file.\n"},

 {"OFF|ENCODE|PREVIEW", "\t" "Source buffer type."
                    " Set to config file."},
 {"0|1",  "\t\t"    "Crop input. 0: use default input window; "
                    "1: use input window specify in next options. "
                    "Set to confif file."},
 {"",     "\t\t\t"  "Input window width. Set to config file."},
 {"",     "\t\t\t"  "Input window height. Set to config file."},
 {"",     "\t\t\t"  "Input window offset X. Set to config file."},
 {"",     "\t\t\t"  "Input window offset Y. Set to config file."},
 {"",     "\t\t\t"  "Source buffer width. Set to config file."},
 {"",     "\t\t\t"  "Source buffer height. Set to config file.\n"},

 {"0-3",  "\t\t"    "Stream ID"},
 {"0|1",  "\t\t"    "Enable or disable stream."},
 {"H264|H265|MJPEG", "\t"    "Stream type."
                    "Set to config file."},
 {"0-3",  "\t\t"    "Stream source. "
                    "0: Main buffer; 1: 2nd buffer; "
                    "2: 3rd buffer, 3: 4th buffer. "
                    "Set to config file."},
 {"",     "\t\t\t"  "Stream frame rate. Set to config file."},
 {"",     "\t\t\t"  "Stream width. Set to config file."},
 {"",     "\t\t\t"  "Stream height. Set to config file."},
 {"",     "\t\t\t"  "Stream offset x. Set to config file."},
 {"",     "\t\t\t"  "Stream offset y. Set to config file."},
 {"0|1",  "\t\t"    "Stream horizontal flip. Set to config file."},
 {"0|1",  "\t\t"    "Stream vertical flip. Set to config file."},
 {"0|1",  "\t\t"    "Stream counter-clock-wise rotation 90 degrees. "
                    "Set to config file.\n"},

 {"0-1",  "\t\t"    "Bitrate control method. 0: CBR; 1: VBR. Set to config file."},
 {"0-2",  "\t\t"    "H264 or H265 Profile. 0: Baseline; 1: Main; 2: High. "
                    "Set to config file."},
 {"0-3",  "\t\t"    "H264 or H265 AU type. \n"
                    "\t\t\t\t\t"
                    "  0: NO_AUD_NO_SEI; 1: AUD_BEFORE_SPS_WITH_SEI; "
                    "2: AUD_AFTER_SPS_WITH_SEI; 3: NO_AUD_WITH_SEI\n"
                    "\t\t\t\t\t"
                    "  Set to config file."},
 {"420|mono",   "\t"  "H264 or H265 Chroma format. Set to config file."},
 {"1-3",  "\t\t"    "H264 or H265 GOP M. Set to config file."},
 {"15-1800", "\t\t" "H264 or H265 GOP N. Set to config file."},
 {"1-4",  "\t\t\t"  "H264 or H265 IDR interval. Set to config file."},
 {"",     "\t\t\t"  "H264 or H265 Target bitrate. Set to config file."},
 {"0-64", "\t\t\t"  "H264 or H265 MV threshold. Set to config file."},
 {"0|1", "\t"       "H264 or H265 encode improve. Set to config file."},
 {"0|1", "\t\t"     "H264 or H265 multi ref P. Set to config file."},
 {"0-63", "\t"      "H264 or H265 fast seek interval. Set to config file."},
 {"1-100","\t\t"    "MJPEG quality. Set to config file."},
 {"420|mono",   "\t"  "MJPEG Chroma format. Set to config file.\n"},

 {"mode0|mode1|...|mode10",   ""  "Mode type in feature. Set to config file."},
 {"none|2x|3x|4x|sensor",   ""  "HDR type in feature. Set to config file."},
 {"normal|plus|advanced",   ""  "ISO type in feature. Set to config file."},
 {"none|ldc|full|eis",   ""  "Dewarp type in feature. Set to config file."
                             "(Not supported on S2E platform)"},
 {"disable|enable",   ""  "DPTZ in feature. Set to config file."},
 {"none|lbr",     ""  "Bitrate control type in feature. Set to config file."},
 {"disable|enable",     ""  "Overlay type in feature. Set to config file."},
 {"disable|enable",     ""  "HEVC type in feature. Set to config file."},
 {"1|2|3",   "\t"  "IAV version in feature. Set to config file.\n"},

 {"",     "\t\t\t"  "restart encoding and reload all configs\n"},

 {"",     "\t\t"    "Show all buffers' formats"},
 {"",     "\t\t"    "Show all streams' formats"},
 {"",     "\t\t"    "Show all streams' configs"},
 {"",     "\t\t"    "Show feature configs"},

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
  int32_t current_vout_id = -1;
  int32_t current_stream_id = -1;
  int32_t current_buffer_id = -1;
  int32_t ret = 0;
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
        g_vin_setting[current_vin_id].flip.is_set = true;
        g_vin_setting[current_vin_id].flip.value.v_int = atoi(optarg);
        break;
      case VIN_FPS:
        VERIFY_PARA_2_FLOAT(atof(optarg), 0, 120);
        g_vin_setting[current_vin_id].is_set = true;
        g_vin_setting[current_vin_id].fps.is_set = true;
        g_vin_setting[current_vin_id].fps.value.v_float = atof(optarg);
        break;

      case 'V':
        VERIFY_VOUT_ID(atoi(optarg));
        current_vout_id = atoi(optarg);
        g_vout_setting[current_vout_id].is_set = true;
        break;

      case VOUT_TYPE:
        VERIFY_VOUT_ID(current_vout_id);
        g_vout_setting[current_vout_id].type.is_set = true;
        g_vout_setting[current_vout_id].type.value.v_int = atoi(optarg);
        break;

      case VOUT_VIDEO_TYPE:
        VERIFY_VOUT_ID(current_vout_id);
        g_vout_setting[current_vout_id].video_type.is_set = true;
        g_vout_setting[current_vout_id].video_type.value.v_int = atoi(optarg);
        break;

      case VOUT_MODE:
        VERIFY_VOUT_ID(current_vout_id);
          g_vout_setting[current_vout_id].is_mode_set = true;
          strcpy(g_vout_setting[current_vout_id].mode, optarg);
          g_vout_setting[current_vout_id].mode[VOUT_MAX_CHAR_NUM-1] = '\0';
        break;

      case VOUT_FLIP:
        VERIFY_VOUT_ID(current_vout_id);
        g_vout_setting[current_vout_id].flip.is_set = true;
        g_vout_setting[current_vout_id].flip.value.v_int = atoi(optarg);
        break;

      case VOUT_ROTATE:
        VERIFY_VOUT_ID(current_vout_id);
        g_vout_setting[current_vout_id].rotate.is_set = true;
        g_vout_setting[current_vout_id].rotate.value.v_int = atoi(optarg);
        break;

      case VOUT_FPS:
        VERIFY_VOUT_ID(current_vout_id);
        g_vout_setting[current_vout_id].fps.is_set = true;
        g_vout_setting[current_vout_id].fps.value.v_float = atoi(optarg);
        break;

      case 's':
        current_stream_id = atoi(optarg);
        current_buffer_id = -1;
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].is_set = true;
        g_stream_h26x[current_stream_id].is_set = true;
        g_stream_mjpeg[current_stream_id].is_set = true;
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
      case STREAM_FRAMERATE:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_fmt[current_stream_id].framerate.is_set = true;
        g_stream_fmt[current_stream_id].framerate.value.v_int = atoi(optarg);
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
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_h26x[current_stream_id].h26x_bitrate_ctrl.is_set = true;
        g_stream_h26x[current_stream_id].h26x_bitrate_ctrl.value.v_int = atoi(optarg);
        break;
      case PROFILE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 2);
        g_stream_h26x[current_stream_id].h26x_profile.is_set = true;
        g_stream_h26x[current_stream_id].h26x_profile.value.v_int = atoi(optarg);
        break;
      case AU_TYPE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_stream_h26x[current_stream_id].h26x_au_type.is_set = true;
        g_stream_h26x[current_stream_id].h26x_au_type.value.v_int = atoi(optarg);
        break;
      case H26x_CHROMA:
        VERIFY_STREAM_ID(current_stream_id);
        if (!strcasecmp(optarg, "420")) {
          g_stream_h26x[current_stream_id].h26x_chroma.value.v_int = CHROMA_420;
        } else if (!strcasecmp(optarg, "mono")) {
          g_stream_h26x[current_stream_id].h26x_chroma.value.v_int = CHROMA_MONO;
        } else {
          ERROR("Wrong H264 or H265 Chroma Format: %s", optarg);
        }
        g_stream_h26x[current_stream_id].h26x_chroma.is_set = true;
        break;
      case 'm':
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 3);
        g_stream_h26x[current_stream_id].h26x_m.is_set = true;
        g_stream_h26x[current_stream_id].h26x_m.value.v_int = atoi(optarg);
        break;
      case 'n':
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 4095);
        g_stream_h26x[current_stream_id].h26x_n.is_set = true;
        g_stream_h26x[current_stream_id].h26x_n.value.v_int = atoi(optarg);
        break;
      case IDR_INTERVAL:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 4);
        g_stream_h26x[current_stream_id].h26x_idr_interval.is_set = true;
        g_stream_h26x[current_stream_id].h26x_idr_interval.value.v_int = atoi(optarg);
        break;
      case BITRATE:
        VERIFY_STREAM_ID(current_stream_id);
        g_stream_h26x[current_stream_id].h26x_bitrate.is_set = true;
        g_stream_h26x[current_stream_id].h26x_bitrate.value.v_int = atoi(optarg);
        break;
      case MV_THRESHOLD:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 64);
        g_stream_h26x[current_stream_id].h26x_mv_threshold.is_set = true;
        g_stream_h26x[current_stream_id].h26x_mv_threshold.value.v_int = atoi(optarg);
        break;
      case FLAT_AREA_IMPROVE:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_h26x[current_stream_id].h26x_flat_area_improve.is_set = true;
        g_stream_h26x[current_stream_id].h26x_flat_area_improve.value.v_int = atoi(optarg);
        break;
      case MULTI_REF_P:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_stream_h26x[current_stream_id].h26x_multi_ref_p.is_set = true;
        g_stream_h26x[current_stream_id].h26x_multi_ref_p.value.v_int = atoi(optarg);
        break;
      case FAST_SEEK_INTVL:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 0, 63);
        g_stream_h26x[current_stream_id].h26x_fast_seek_intvl.is_set = true;
        g_stream_h26x[current_stream_id].h26x_fast_seek_intvl.value.v_int = atoi(optarg);
        break;
      case MJPEG_QUALITY:
        VERIFY_STREAM_ID(current_stream_id);
        VERIFY_PARA_2(atoi(optarg), 1, 100);
        g_stream_mjpeg[current_stream_id].mjpeg_quality.is_set = true;
        g_stream_mjpeg[current_stream_id].mjpeg_quality.value.v_int = atoi(optarg);
        break;
      case MJPEG_CHROMA:
        VERIFY_STREAM_ID(current_stream_id);
        if (!strcasecmp(optarg, "420")) {
          g_stream_mjpeg[current_stream_id].mjpeg_chroma.value.v_int = CHROMA_420;
        } else if (!strcasecmp(optarg, "mono")) {
          g_stream_mjpeg[current_stream_id].mjpeg_chroma.value.v_int = CHROMA_MONO;
        } else {
          ERROR("Wrong Mjpeg Chroma Format: %s", optarg);
        }
        g_stream_mjpeg[current_stream_id].mjpeg_chroma.is_set = true;
        break;
      case FEATURE_MODE:
        if (!strcasecmp(optarg, "mode0")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_0;
        } else if (!strcasecmp(optarg, "mode1")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_1;
        } else if (!strcasecmp(optarg, "mode2")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_2;
        } else if (!strcasecmp(optarg, "mode3")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_3;
        } else if (!strcasecmp(optarg, "mode4")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_4;
        } else if (!strcasecmp(optarg, "mode5")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_5;
        } else if (!strcasecmp(optarg, "mode6")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_6;
        } else if (!strcasecmp(optarg, "mode7")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_7;
        } else if (!strcasecmp(optarg, "mode8")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_8;
        } else if (!strcasecmp(optarg, "mode9")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_9;
        } else if (!strcasecmp(optarg, "mode10")) {
          g_feature_setting.mode.value.v_int = AM_ENCODE_MODE_10;
        } else {
          ERROR("Wrong mode type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.mode.is_set = true;
        break;

      case FEATURE_HDR:
        if (!strcasecmp(optarg, "none") || !strcasecmp(optarg, "linear")) {
          g_feature_setting.hdr.value.v_int = AM_HDR_SINGLE_EXPOSURE;
        } else if (!strcasecmp(optarg, "2x")) {
          g_feature_setting.hdr.value.v_int = AM_HDR_2_EXPOSURE;
        } else if (!strcasecmp(optarg, "3x")) {
          g_feature_setting.hdr.value.v_int = AM_HDR_3_EXPOSURE;
        } else if (!strcasecmp(optarg, "4x")) {
          g_feature_setting.hdr.value.v_int = AM_HDR_4_EXPOSURE;
        } else if (!strcasecmp(optarg, "sensor")) {
          g_feature_setting.hdr.value.v_int = AM_HDR_SENSOR_INTERNAL;
        } else {
          ERROR("Wrong HDR type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.hdr.is_set = true;
        break;
      case FEATURE_ISO:
        if (!strcasecmp(optarg, "normal")) {
          g_feature_setting.iso.value.v_int = AM_IMAGE_NORMAL_ISO;
        } else if (!strcasecmp(optarg, "plus")) {
          g_feature_setting.iso.value.v_int = AM_IMAGE_ISO_PLUS;
        } else if (!strcasecmp(optarg, "advanced")) {
          g_feature_setting.iso.value.v_int = AM_IMAGE_ADVANCED_ISO;
        } else {
          ERROR("Wrong ISO type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.iso.is_set = true;
        break;

      case FEATURE_DEWARP_FUNC:
        if (!strcasecmp(optarg, "none")) {
          g_feature_setting.dewarp_func.value.v_int = AM_DEWARP_NONE;
        } else if (!strcasecmp(optarg, "ldc")) {
          g_feature_setting.dewarp_func.value.v_int = AM_DEWARP_LDC;
        } else if (!strcasecmp(optarg, "full")) {
          g_feature_setting.dewarp_func.value.v_int = AM_DEWARP_FULL;
        } else if (!strcasecmp(optarg, "eis")) {
          g_feature_setting.dewarp_func.value.v_int = AM_DEWARP_EIS;
        } else {
          ERROR("Wrong Dewarp func type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.dewarp_func.is_set = true;
        break;
      case FEATURE_DPTZ:
        if (!strcasecmp(optarg, "enable")) {
          g_feature_setting.dptz.value.v_int = AM_DPTZ_ENABLE;
        } else if (!strcasecmp(optarg, "disable")) {
          g_feature_setting.dptz.value.v_int = AM_DPTZ_DISABLE;
        } else {
          ERROR("Wrong Dewarp func type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.dptz.is_set = true;
        break;
      case FEATURE_BITRATE_CTRL:
        if (!strcasecmp(optarg, "none")) {
          g_feature_setting.bitrate_ctrl.value.v_int = AM_BITRATE_CTRL_NONE;
        } else if (!strcasecmp(optarg, "lbr")) {
          g_feature_setting.bitrate_ctrl.value.v_int = AM_BITRATE_CTRL_LBR;
        } else {
          ERROR("Wrong Bitrate control type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.bitrate_ctrl.is_set = true;
        break;
      case FEATURE_OVERLAY:
        if (!strcasecmp(optarg, "enable")) {
          g_feature_setting.overlay.value.v_int = AM_OVERLAY_PLUGIN_ENABLE;
        } else if (!strcasecmp(optarg, "disable")) {
          g_feature_setting.overlay.value.v_int = AM_OVERLAY_PLUGIN_DISABLE;
        } else {
          ERROR("Wrong Overlay type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.overlay.is_set = true;
        break;
      case FEATURE_HEVC:
        if (!strcasecmp(optarg, "enable")) {
          g_feature_setting.hevc.value.v_int = AM_HEVC_CLOCK_ENABLE;
        } else if (!strcasecmp(optarg, "disable")) {
          g_feature_setting.hevc.value.v_int = AM_HEVC_CLOCK_DISABLE;
        } else {
          ERROR("Wrong hevc type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.hevc.is_set = true;
        break;
      case FEATURE_IAV_VER:
        g_feature_setting.iav_ver.value.v_int = atoi(optarg);
        if (g_feature_setting.iav_ver.value.v_int > 3 ||
            g_feature_setting.iav_ver.value.v_int < 1) {
          ERROR("Wrong IAV version type: %s", optarg);
          break;
        }
        g_feature_setting.is_set = true;
        g_feature_setting.iav_ver.is_set = true;
        break;

      case 'b':
        current_buffer_id = atoi(optarg);
        current_stream_id = -1;
        VERIFY_BUFFER_ID(current_buffer_id);
        g_buffer_fmt[current_buffer_id].is_set = true;
        set_state = set_buffer;
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
          } else if (!strcasecmp(optarg, "h265")) {
            g_stream_fmt[current_stream_id].type.value.v_int = H265;
          } else if (!strcasecmp(optarg, "mjpeg")) {
            g_stream_fmt[current_stream_id].type.value.v_int = MJPEG;
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

      case APPLY:
        apply_config_flag = true;
        break;

      case SHOW_STREAM_FMT:
        show_fmt_flag = true;
        break;
      case SHOW_STREAM_CFG:
        show_cfg_flag = true;
        break;

      case SHOW_BUFFER_FMT:
        show_buf_flag = true;
        break;
      case SHOW_FEATURE_CFG:
        show_feature_flag = true;
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

    if (g_vin_setting[i].flip.is_set) {
      config.flip = g_vin_setting[i].flip.value.v_int;
      SET_BIT(config.enable_bits, AM_VIN_CONFIG_FLIP_EN_BIT);
      has_setting = true;
    }

    if (g_vin_setting[i].fps.is_set) {
      float fps = g_vin_setting[i].fps.value.v_float;
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
    if (!has_setting) {
      continue;
    }
    config.vin_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET,
                              &config, sizeof(config),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set VIN config!\n");
      break;
    }
  }
  return ret;
}

static int32_t set_vout_config()
{
  int32_t ret  = 0;

  for (int32_t i = 0; i < AM_VOUT_MAX_NUM; ++i) {
    if (!g_vout_setting[i].is_set) {
      continue;
    }

    bool has_setting = false;
    am_vout_config_t config;
    memset(&config, 0, sizeof(config));
    am_service_result_t service_result;

    if (g_vout_setting[i].type.is_set) {
      config.type = g_vout_setting[i].type.value.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_TYPE_EN_BIT);
      has_setting = true;
    }

    if (g_vout_setting[i].video_type.is_set) {
      config.video_type = g_vout_setting[i].video_type.value.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_VIDEO_TYPE_EN_BIT);
      has_setting = true;
    }

    if (g_vout_setting[i].is_mode_set) {
      snprintf(config.mode, VOUT_MAX_CHAR_NUM, "%s", g_vout_setting[i].mode);
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_MODE_EN_BIT);
      has_setting = true;
    }

    if (g_vout_setting[i].flip.is_set) {
      config.flip = g_vout_setting[i].flip.value.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_FLIP_EN_BIT);
      has_setting = true;
    }

    if (g_vout_setting[i].rotate.is_set) {
      config.rotate = g_vout_setting[i].rotate.value.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_ROTATE_EN_BIT);
      has_setting = true;
    }

    if (g_vout_setting[i].fps.is_set) {
      float fps = g_vout_setting[i].fps.value.v_float;
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
        config.fps = static_cast<int32_t>(fps);
      }
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_FPS_EN_BIT);
      has_setting = true;
    }
    if (has_setting) {
      config.vout_id = i;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET,
                                &config, sizeof(config),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set VOUT config!\n");
        break;
      }
    }
  }
  return ret;
}

static int32_t show_vout_cfg()
{
  int32_t ret = 0;
  uint32_t vout_id;
  am_vout_config_t *cfg = nullptr;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < AM_VOUT_MAX_NUM; ++i) {
    vout_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET,
                              &vout_id, sizeof(vout_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get VOUT config!");
      break;
    }

    cfg = (am_vout_config_t*)service_result.data;

    printf("\n");
    PRINTF("[Vout%d Config]", vout_id);
    PRINTF("  type:\t\t%d", cfg->type);
    PRINTF("  video type:\t%d", cfg->video_type);
    PRINTF("  mode:\t\t%s", cfg->mode);
    PRINTF("  flip:\t\t%d", cfg->flip);
    PRINTF("  rotate:\t\t%d", cfg->rotate);
    PRINTF("  fps:\t\t%d", cfg->fps);
  }
  return ret;
}

static int32_t set_buffer_fmt_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_buffer_fmt_t buffer_fmt = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < SOURCE_BUFFER_MAX_NUM; ++i) {
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
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
      buffer_fmt.height = g_buffer_fmt[i].height.value.v_int;
      has_setting = true;
    }
    if (g_buffer_fmt[i].prewarp.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_PREWARP_EN_BIT);
      buffer_fmt.prewarp = g_buffer_fmt[i].prewarp.value.v_bool;
      has_setting = true;
    }

    if (has_setting) {
      INFO("Buffer[%d]", i);
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,
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

static int32_t set_stream_fmt_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_stream_fmt_t stream_fmt = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < STREAM_MAX_NUM; ++i) {
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
    if (g_stream_fmt[i].framerate.is_set) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_FRAME_RATE_EN_BIT);
      stream_fmt.frame_rate = g_stream_fmt[i].framerate.value.v_int;
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
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET,
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

static int32_t set_stream_h26x_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_h26x_cfg_t stream_h26x = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < STREAM_MAX_NUM; ++i) {
    if (!g_stream_h26x[i].is_set) {
      continue;
    }
    stream_h26x.stream_id = i;
    if (g_stream_h26x[i].h26x_bitrate_ctrl.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_BITRATE_CTRL_EN_BIT);
      stream_h26x.bitrate_ctrl = g_stream_h26x[i].h26x_bitrate_ctrl.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_profile.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_PROFILE_EN_BIT);
      stream_h26x.profile = g_stream_h26x[i].h26x_profile.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_au_type.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_AU_TYPE_EN_BIT);
      stream_h26x.au_type = g_stream_h26x[i].h26x_au_type.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_chroma.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_CHROMA_EN_BIT);
      stream_h26x.chroma = g_stream_h26x[i].h26x_chroma.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_m.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_M_EN_BIT);
      stream_h26x.M = g_stream_h26x[i].h26x_m.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_n.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_N_EN_BIT);
      stream_h26x.N = g_stream_h26x[i].h26x_n.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_idr_interval.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_IDR_EN_BIT);
      stream_h26x.idr_interval = g_stream_h26x[i].h26x_idr_interval.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_bitrate.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_BITRATE_EN_BIT);
      stream_h26x.target_bitrate = g_stream_h26x[i].h26x_bitrate.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_mv_threshold.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_MV_THRESHOLD_EN_BIT);
      stream_h26x.mv_threshold = g_stream_h26x[i].h26x_mv_threshold.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_flat_area_improve.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_FLAT_AREA_IMPROVE_EN_BIT);
      stream_h26x.flat_area_improve = g_stream_h26x[i].h26x_flat_area_improve.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_multi_ref_p.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_MULTI_REF_P_EN_BIT);
      stream_h26x.multi_ref_p = g_stream_h26x[i].h26x_multi_ref_p.value.v_int;
      has_setting = true;
    }
    if (g_stream_h26x[i].h26x_fast_seek_intvl.is_set) {
      SET_BIT(stream_h26x.enable_bits, AM_H26x_CFG_FAST_SEEK_INTVL_EN_BIT);
      stream_h26x.fast_seek_intvl = g_stream_h26x[i].h26x_fast_seek_intvl.value.v_int;
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET,
                                &stream_h26x, sizeof(stream_h26x),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream h264 or h265 config!\n");
        break;
      }
    }
  }
  return ret;
}

static int32_t set_stream_mjpeg_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_mjpeg_cfg_t stream_mjpeg = {0};
  bool has_setting = false;

  for (uint32_t i = 0; i < STREAM_MAX_NUM; ++i) {
    if (!g_stream_mjpeg[i].is_set) {
      continue;
    }
    stream_mjpeg.stream_id = i;

    if (g_stream_mjpeg[i].mjpeg_quality.is_set) {
      SET_BIT(stream_mjpeg.enable_bits, AM_MJPEG_CFG_QUALITY_EN_BIT);
      stream_mjpeg.quality = g_stream_mjpeg[i].mjpeg_quality.value.v_int;
      has_setting = true;
    }
    if (g_stream_mjpeg[i].mjpeg_chroma.is_set) {
      SET_BIT(stream_mjpeg.enable_bits, AM_MJPEG_CFG_CHROMA_EN_BIT);
      stream_mjpeg.chroma = g_stream_mjpeg[i].mjpeg_chroma.value.v_int;
      has_setting = true;
    }

    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET,
                                &stream_mjpeg, sizeof(stream_mjpeg),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream mjpeg config!\n");
        break;
      }
    }
  }
  return ret;
}

static int32_t set_feature_config()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_feature_config_t feature = {0};
  bool has_setting = false;

  do {
    if (!g_feature_setting.is_set) {
      break;
    }

    if (g_feature_setting.mode.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_MODE_EN_BIT);
      feature.mode = g_feature_setting.mode.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.hdr.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_HDR_EN_BIT);
      feature.hdr = g_feature_setting.hdr.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.iso.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_ISO_EN_BIT);
      feature.iso = g_feature_setting.iso.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.dewarp_func.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_DEWARP_EN_BIT);
      feature.dewarp_func = g_feature_setting.dewarp_func.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.dptz.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_DPTZ_EN_BIT);
      feature.dptz = g_feature_setting.dptz.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.bitrate_ctrl.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_BITRATECTRL_EN_BIT);
      feature.bitrate_ctrl = g_feature_setting.bitrate_ctrl.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.overlay.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_OVERLAY_EN_BIT);
      feature.overlay = g_feature_setting.overlay.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.hevc.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_HEVC_EN_BIT);
      feature.hevc = g_feature_setting.hevc.value.v_int;
      has_setting = true;
    }

    if (g_feature_setting.iav_ver.is_set) {
      SET_BIT(feature.enable_bits, AM_FEATURE_CONFIG_IAV_VERSION_EN_BIT);
      feature.version = g_feature_setting.iav_ver.value.v_int;
      has_setting = true;
    }


    if (has_setting) {
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET,
                                &feature, sizeof(feature),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set feature config!\n");
        break;
      }
    }
  } while (0);
  return ret;
}

static int32_t show_feature_cfg()
{
  int ret = 0;
  do {
    am_feature_config_t *feature = nullptr;
    am_service_result_t service_result;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set feature config!\n");
      break;
    }
    feature = (am_feature_config_t *) service_result.data;
    PRINTF("[video feature config]");
    PRINTF("  version:\t\t%d", feature->version);
    if (feature->mode == AM_ENCODE_MODE_AUTO) {
      PRINTF("  mode:\t\tauto");
    } else {
      PRINTF("  mode:\t\tmode%d", feature->mode);
    }
    switch (feature->hdr) {
      case AM_HDR_SINGLE_EXPOSURE: PRINTF("  hdr:\t\tlinear"); break;
      case AM_HDR_2_EXPOSURE: PRINTF("  hdr:\t\t2X"); break;
      case AM_HDR_3_EXPOSURE: PRINTF("  hdr:\t\t3X"); break;
      case AM_HDR_4_EXPOSURE: PRINTF("  hdr:\t\t4X"); break;
      case AM_HDR_SENSOR_INTERNAL: PRINTF("  hdr:\t\tsensor internal"); break;
      default: PRINTF("  hdr:\t\tunknown"); break;
    }
    switch (feature->iso) {
      case AM_IMAGE_NORMAL_ISO: PRINTF("  iso:\t\tnormal"); break;
      case AM_IMAGE_ISO_PLUS: PRINTF("  iso:\t\tplus"); break;
      case AM_IMAGE_ADVANCED_ISO: PRINTF("  iso:\t\tadvanced"); break;
      default: PRINTF("  iso:\t\tunknown"); break;
    }
    switch (feature->dewarp_func) {
      case AM_DEWARP_NONE: PRINTF("  dewarp_func:\tnone"); break;
      case AM_DEWARP_LDC: PRINTF("  dewarp_func:\tLDC"); break;
      case AM_DEWARP_FULL: PRINTF("  dewarp_func:\tfull"); break;
      case AM_DEWARP_EIS: PRINTF("  dewarp_func:\tEIS"); break;
      default: PRINTF("  dewarp_func:\tunknown"); break;
    }
    switch (feature->dptz) {
      case AM_DPTZ_DISABLE: PRINTF("  dptz:\t\tdisable"); break;
      case AM_DPTZ_ENABLE: PRINTF("  dptz:\t\tenable"); break;
      default: PRINTF("  dptz:\t\t\tunknown"); break;
    }

    switch (feature->bitrate_ctrl) {
      case AM_BITRATE_CTRL_NONE: PRINTF("  bitrate_ctrl:\tnone"); break;
      case AM_BITRATE_CTRL_LBR: PRINTF("  bitrate_ctrl:\tlbr"); break;
      default: PRINTF("  bitrate_ctrl:\t\tunknown"); break;
    }
    switch (feature->overlay) {
      case AM_OVERLAY_PLUGIN_DISABLE: PRINTF("  overlay:\t\tdisable"); break;
      case AM_OVERLAY_PLUGIN_ENABLE: PRINTF("  overlay:\t\tenable"); break;
      default: PRINTF("  overlay:\t\tunknown"); break;
    }
    switch (feature->hevc) {
      case AM_HEVC_CLOCK_UNSUPPORTED: PRINTF("  HEVC:\t\tunsupported"); break;
      case AM_HEVC_CLOCK_DISABLE: PRINTF("  HEVC:\t\tdisable"); break;
      case AM_HEVC_CLOCK_ENABLE: PRINTF("  HEVC:\t\tenable"); break;
      default: PRINTF("  HEVC:\t\tunknown"); break;
    }
  } while (0);
  return ret;
}

static int32_t show_stream_fmt()
{
  int32_t ret = 0;
  uint32_t stream_id;
  am_stream_fmt_t *fmt = nullptr;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < STREAM_MAX_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,
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
        PRINTF("  Type:\t\t\tH265");
        break;
      case 3:
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

    PRINTF("  Frame rate:\t\t%d", fmt->frame_rate);
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
  am_service_result_t service_result;

  for (uint32_t i = 0; i < STREAM_MAX_NUM; ++i) {
    stream_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Stream config!");
      break;
    }

    am_h26x_cfg_t *h26x_cfg = (am_h26x_cfg_t*)service_result.data;

    printf("\n");
    PRINTF("[Stream%d Config]", stream_id);
    PRINTF("  Bitrate:\t\t%d", h26x_cfg->target_bitrate);
    switch (h26x_cfg->bitrate_ctrl) {
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
        PRINTF("  Bitrate Control:\t%d, error!", h26x_cfg->bitrate_ctrl);
        break;
    }

    switch (h26x_cfg->profile) {
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
        PRINTF("  Profile:\t\t%d, error!", h26x_cfg->profile);
        break;
    }

    switch (h26x_cfg->au_type) {
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
        PRINTF("  AU type:\t\t%d, error!", h26x_cfg->au_type);
        break;
    }

    switch (h26x_cfg->chroma) {
      case 0:
        PRINTF("  Chroma format:\t\t420");
        break;
      case 1:
        PRINTF("  Chroma format:\t\t422");
        break;
      case 2:
        PRINTF("  Chroma format:\t\tMono");
        break;
      default:
        PRINTF("  Chroma format:\t\t%d, error!", h26x_cfg->chroma);
        break;
    }
    PRINTF("  M:\t\t\t%d", h26x_cfg->M);
    PRINTF("  N:\t\t\t%d", h26x_cfg->N);
    PRINTF("  IDR interval:\t\t%d", h26x_cfg->idr_interval);
    PRINTF("  MV threshold:\t\t%d", h26x_cfg->mv_threshold);
    PRINTF("  Flat area improve:\t%d", h26x_cfg->flat_area_improve);
    PRINTF("  Multi ref P:\t\t%d", h26x_cfg->multi_ref_p);
    PRINTF("  Fast seek interval:\t%d", h26x_cfg->fast_seek_intvl);

    // MJPEG
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET,
                              &stream_id, sizeof(stream_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Stream config!");
      break;
    }

    am_mjpeg_cfg_t *mjpeg_cfg = (am_mjpeg_cfg_t*)service_result.data;

    PRINTF("[MJPEG]");
    PRINTF("  Quality:\t\t%d", mjpeg_cfg->quality);
    switch (mjpeg_cfg->chroma) {
      case 0:
        PRINTF("  Chroma Format:\t\t420");
        break;
      case 1:
        PRINTF("  Chroma Format:\t\t422");
        break;
      case 2:
        PRINTF("  Chroma Format:\t\tMono");
        break;
      default:
        PRINTF("  Chroma Format:\t\t%d, error!", mjpeg_cfg->chroma);
        break;
    }
  }
  return ret;
}

static int32_t show_buffer_fmt()
{
  int32_t ret = 0;
  uint32_t buffer_id;
  am_service_result_t service_result;

  for (uint32_t i = 0; i < SOURCE_BUFFER_MAX_NUM; ++i) {
    buffer_id = i;
    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET,
                              &buffer_id, sizeof(buffer_id),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to Get Buffer format!");
      break;
    }
    am_buffer_fmt_t *fmt = (am_buffer_fmt_t*)service_result.data;

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

static int32_t show_all_info()
{
  int32_t ret = 0;
  do {
    if ((ret = show_feature_cfg()) < 0) {
      break;
    }
    if ((ret = show_stream_fmt()) < 0) {
      break;
    }
    if ((ret = show_stream_cfg()) < 0) {
      break;
    }
    if ((ret = show_buffer_fmt()) < 0) {
      break;
    }
    if ((ret = show_vout_cfg()) < 0) {
      break;
    }
  } while (0);
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

static int32_t load_all_cfg()
{
  int32_t ret = 0;
  INFO("Load all configures!!!\n");
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to load all configs!\n");
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

    if ((ret = load_all_cfg()) < 0) {
      break;
    }

    if ((ret = start_encoding()) < 0) {
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
      if (show_fmt_flag) {
        show_stream_fmt();
      }
      if (show_cfg_flag) {
        show_stream_cfg();
      }
      if (show_buf_flag) {
        show_buffer_fmt();
      }
      if (show_feature_flag) {
        show_feature_cfg();
      }
      if (show_fmt_flag ||
          show_cfg_flag ||
          show_feature_flag ||
          show_buf_flag) {
        break;
      }
    }

    if ((ret = set_vin_config()) < 0) {
      break;
    }

    if ((ret = set_vout_config()) < 0) {
      break;
    }

    if ((ret = set_stream_fmt_config()) < 0) {
      break;
    }
    if ((ret = set_stream_h26x_config()) < 0) {
      break;
    }
    if ((ret = set_stream_mjpeg_config()) < 0) {
      break;
    }
    if ((ret = set_buffer_fmt_config()) < 0) {
      break;
    }
    if ((ret = set_feature_config()) < 0) {
      break;
    }

    if (apply_config_flag) {
      if ((ret = apply_config()) < 0) {
        break;
      }
    }
  } while (0);

  return ret;
}
