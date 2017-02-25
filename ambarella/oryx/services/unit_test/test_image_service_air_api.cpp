/*******************************************************************************
 * test_image_service.cpp
 *
 * History:
 *   Jan 6, 2015 - [binwang] created file
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
#include <getopt.h>
#include <signal.h>

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_api_helper.h"
#include "am_api_image.h"
#include "am_image_quality_if.h"

#define NO_ARG 0
#define HAS_ARG 1
#define FILE_NAME_LENGTH 64

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

struct setting_option
{
    union
    {
        bool v_bool;
        float v_float;
        int32_t v_int;
    } value;bool is_set;
};

static AMAPIHelperPtr g_api_helper = nullptr;
static bool load_aeb_adj_flag = false;
static bool show_ae_flag = false;
static bool show_awb_flag = false;
static bool show_af_flag = false;
static bool show_noise_filter_flag = false;
static bool show_image_style_flag = false;
static bool show_all_flag = false;

static void sigstop(int32_t arg)
{
  INFO("signal caught, force quit\n");
  exit(1);
}

struct ae_setting
{
    bool is_set;
    setting_option ae_metering_mode;
    setting_option day_night_mode;
    setting_option slow_shutter_mode;
    setting_option anti_flicker_mode;
    setting_option ae_target_ratio;
    setting_option back_light_comp;
    setting_option local_exposure;
    setting_option dc_iris_enable;
    setting_option sensor_gain_max;
    setting_option shutter_time_min;
    setting_option shutter_time_max;
    setting_option ir_led_mode;
    setting_option sensor_gain_manual;
    setting_option shutter_time_manual;
    setting_option ae_enable;
};

struct awb_setting
{
    bool is_set;
    setting_option wb_mode;
};

struct af_setting
{
    bool is_set;
    setting_option af_mode;
};

struct noise_filter_setting
{
    bool is_set;
    setting_option mctf_strength;
};

struct image_style_setting
{
    bool is_set;
    setting_option quick_style_code;
    setting_option hue;
    setting_option saturation;
    setting_option sharpness;
    setting_option brightness;
    setting_option contrast;
    setting_option auto_contrast_mode;
};

static ae_setting g_ae_setting;
static awb_setting g_awb_setting;
static af_setting g_af_setting;
static noise_filter_setting g_noise_filter_setting;
static image_style_setting g_image_style_setting;
static char aeb_adj_bin_file[64];

enum numeric_short_options
{
  NEMERIC_BEGIN = 'z',

  //AE settings
  AE_METERING_MODE,
  DAY_NIGHT_MODE,
  SLOW_SHUTTER_MODE,
  ANTI_FLICKER,
  AE_TARGET_RATIO,
  BACK_LIGHT_COMP,
  LOCAL_EXPOSURE,
  DC_IRIS_ENABLE,
  SENSOR_GAIN_MAX,
  SHUTTER_TIME_MIN,
  SHUTTER_TIME_MAX,
  IR_LED_MODE,
  SENSOR_GAIN_MANUAL,
  SHUTTER_TIME_MANUAL,
  AE_ENABLE,

  //AWB settings
  AWB_MODE,

  //AF_settings
  AF_MODE,

  //noise filter settings
  MCTF_STRENGTH,

  //image style settings
  QUICK_STYLE_CODE,
  HUE,
  SATURATION,
  SHARPNESS,
  BRIGHTNESS,
  CONTRAST,
  AUTO_CONTRAST_MODE,

  SHOW_AE_SETTING,
  SHOW_AWB_SETTING,
  SHOW_AF_SETTING,
  SHOW_NOISE_FILTER_SETTING,
  SHOW_IMAGE_STYLE_SETTING,
  SHOW_ALL_SETTING,
};

static struct option long_options[] =
{
{"help", NO_ARG, 0, 'h'},

{"load-adj-bin", HAS_ARG, 0, 'd'},
{"ae-metering-mode", HAS_ARG, 0, AE_METERING_MODE},
{"day-night-mode", HAS_ARG, 0, DAY_NIGHT_MODE},
{"slow-shutter-mode", HAS_ARG, 0, SLOW_SHUTTER_MODE},
{"anti-flicker", HAS_ARG, 0, ANTI_FLICKER},
{"ae-target-ratio", HAS_ARG, 0, AE_TARGET_RATIO},
{"back-light-comp", HAS_ARG, 0, BACK_LIGHT_COMP},
{"local-exposure", HAS_ARG, 0, LOCAL_EXPOSURE},
{"dc-iris-enable", HAS_ARG, 0, DC_IRIS_ENABLE},
{"sensor-gain-max", HAS_ARG, 0, SENSOR_GAIN_MAX},
{"shutter-time-min", HAS_ARG, 0, SHUTTER_TIME_MIN},
{"shutter-time-max", HAS_ARG, 0, SHUTTER_TIME_MAX},
{"ir-led-mode", HAS_ARG, 0, IR_LED_MODE},
{"sensor-gain-manual", HAS_ARG, 0, SENSOR_GAIN_MANUAL},
{"shutter-time-manual", HAS_ARG, 0, SHUTTER_TIME_MANUAL},
{"ae-enable", HAS_ARG, 0, AE_ENABLE},

{"awb-mode", HAS_ARG, 0, AWB_MODE},

{"af-mode", HAS_ARG, 0, AF_MODE},

{"mctf-strength", HAS_ARG, 0, MCTF_STRENGTH},

{"quick-style-code", HAS_ARG, 0, QUICK_STYLE_CODE},
{"hue", HAS_ARG, 0, HUE},
{"saturation", HAS_ARG, 0, SATURATION},
{"sharpness", HAS_ARG, 0, SHARPNESS},
{"brightness", HAS_ARG, 0, BRIGHTNESS},
{"contrast", HAS_ARG, 0, CONTRAST},
{"auto_contrast_mode", HAS_ARG, 0, AUTO_CONTRAST_MODE},

{"show-ae-setting", NO_ARG, 0, SHOW_AE_SETTING},
{"show-awb-setting", NO_ARG, 0, SHOW_AWB_SETTING},
{"show-af-setting", NO_ARG, 0, SHOW_AF_SETTING},
{"show-noise-filter-setting", NO_ARG, 0, SHOW_NOISE_FILTER_SETTING},
{"show-image-style-setting", NO_ARG, 0, SHOW_IMAGE_STYLE_SETTING},
{"show-all-setting", NO_ARG, 0, SHOW_ALL_SETTING},
{0, 0, 0, 0}};

static const char *short_options = "hd:";

struct hint32_t_s
{
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
{"", "\t\t\t" "Show usage. \n"},
{"file path", "\t" "Load adj bin file. \n"},
{"0-3", "\t" "AE metering mode. 0: spot 1: center 2: average 3: custom. "},
{"0|1",
  "\t" "Day/night mode. 0: day mode, colorful; 1: night mode, black&white. "},
{"0|1", "\t" "Slow shutter mode. 0: disable  1: enable. "},
{"0|1", "\t" "Anti flicker mode. 0: 50Hz, 1: 60Hz. "},
{"25-400", "\t" "AE target ratio. default 100. "},
{"0|1", "\t" "Back light composition. 0: disable 1: enable. "},
{"0-3", "\t" "Local exposure. 0: disable 3: strongest. "},
{"0|1", "\t" "DC iris enable. 0: disable 1: enable. "},
{"0-48", "\t" "Maximum sensor analog gain(dB). "},
{"1-", "\t" "Minimum sensor shutter time. 30 means 1/30 seconds. "},
{"1-", "\t" "Maximum sensor shutter time. 8000 means 1/8000 seconds. "},
{"0-2", "\t\t" "IR LED mode. 0: off 1: on 2: auto. "},
{"0-", "\t" "Manually sensor gain. Set when AE disabled. "},
{"1-", "\t" "Manually sensor shutter time. Set when AE disabled. "},
{"0|1", "\t\t" "AE enable. 0: disable 1: enable. \n"},

{"0-9",
  "\t\t" "WB mode. 0:MW_WB_AUTO, 1:INCANDESCENT(2800K), 2:D4000, 3:D5000, "
    "4:SUNNY(6500K), 5:CLOUDY(7500K), 6:FLASH, "
    "7:FLUORESCENT, 8:FLUORESCENT_H, 9:UNDERWATER. \n"},

{"0-5", "\t\t" "AF mode. 0:CAF, 1:SAF, 3:MANUAL, 4:CALIB, 5:DEBUG. \n"},

{"0-11", "\t" "MCTF strength. 0: off 1: weakest 11: strongest. \n"},

{"-2-2", "\t" "quick style code. "},
{"-2-2", "\t\t" "hue. "},
{"-2-2", "\t\t" "saturation. "},
{"-2-2", "\t\t" "sharpness. "},
{"-2-2", "\t\t" "brightness. "},
{"-2-2", "\t\t" "contrast. "},
{"0-128", "\t" "auto_contrast_mode. "},

{"", "\t\t" "Show AE setting. "},
{"", "\t\t" "Show AWB setting. "},
{"", "\t\t" "Show AF setting. "},
{"", "\t" "Show noise filter setting. "},
{"", "\t" "Show image style setting. "},
{"", "\t\t" "Show all setting. \n"}};

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1;
      ++ i) {
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
  opterr = 0;
  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        usage(argc, argv);
        return -1;
      case 'd':
        load_aeb_adj_flag = true;
        if (strlen(optarg) > 0 && strlen(optarg) < FILE_NAME_LENGTH) {
          memcpy(aeb_adj_bin_file, optarg, strlen(optarg));
        } else {
          ERROR("the length of file name must be [0~63]");
        }
        break;
      case AE_METERING_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_ae_setting.is_set = true;
        g_ae_setting.ae_metering_mode.is_set = true;
        g_ae_setting.ae_metering_mode.value.v_int = atoi(optarg);
        break;
      case DAY_NIGHT_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.day_night_mode.is_set = true;
        g_ae_setting.day_night_mode.value.v_int = atoi(optarg);
        break;
      case SLOW_SHUTTER_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.slow_shutter_mode.is_set = true;
        g_ae_setting.slow_shutter_mode.value.v_int = atoi(optarg);
        break;
      case ANTI_FLICKER:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.anti_flicker_mode.is_set = true;
        g_ae_setting.anti_flicker_mode.value.v_int = atoi(optarg);
        break;
      case AE_TARGET_RATIO:
        VERIFY_PARA_2(atoi(optarg), 25, 400);
        g_ae_setting.is_set = true;
        g_ae_setting.ae_target_ratio.is_set = true;
        g_ae_setting.ae_target_ratio.value.v_int = atoi(optarg);
        break;
      case BACK_LIGHT_COMP:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.back_light_comp.is_set = true;
        g_ae_setting.back_light_comp.value.v_int = atoi(optarg);
        break;
      case LOCAL_EXPOSURE:
        VERIFY_PARA_2(atoi(optarg), 0, 3);
        g_ae_setting.is_set = true;
        g_ae_setting.local_exposure.is_set = true;
        g_ae_setting.local_exposure.value.v_int = atoi(optarg);
        break;
      case DC_IRIS_ENABLE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.dc_iris_enable.is_set = true;
        g_ae_setting.dc_iris_enable.value.v_int = atoi(optarg);
        break;
      case SENSOR_GAIN_MAX:
        g_ae_setting.is_set = true;
        g_ae_setting.sensor_gain_max.is_set = true;
        g_ae_setting.sensor_gain_max.value.v_int = atoi(optarg);
        break;
      case SHUTTER_TIME_MIN:
        g_ae_setting.is_set = true;
        g_ae_setting.shutter_time_min.is_set = true;
        g_ae_setting.shutter_time_min.value.v_int = atoi(optarg);
        break;
      case SHUTTER_TIME_MAX:
        g_ae_setting.is_set = true;
        g_ae_setting.shutter_time_max.is_set = true;
        g_ae_setting.shutter_time_max.value.v_int = atoi(optarg);
        break;
      case IR_LED_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 2);
        g_ae_setting.is_set = true;
        g_ae_setting.ir_led_mode.is_set = true;
        g_ae_setting.ir_led_mode.value.v_int = atoi(optarg);
        break;
      case SENSOR_GAIN_MANUAL:
        g_ae_setting.is_set = true;
        g_ae_setting.sensor_gain_manual.is_set = true;
        g_ae_setting.sensor_gain_manual.value.v_int = atoi(optarg);
        break;
      case SHUTTER_TIME_MANUAL:
        g_ae_setting.is_set = true;
        g_ae_setting.shutter_time_manual.is_set = true;
        g_ae_setting.shutter_time_manual.value.v_int = atoi(optarg);
        break;
      case AE_ENABLE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_ae_setting.is_set = true;
        g_ae_setting.ae_enable.is_set = true;
        g_ae_setting.ae_enable.value.v_int = atoi(optarg);
        break;

        //AWB settings
      case AWB_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 9);
        g_awb_setting.is_set = true;
        g_awb_setting.wb_mode.is_set = true;
        g_awb_setting.wb_mode.value.v_int = atoi(optarg);
        break;

        //AF_settings
      case AF_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 5);
        g_af_setting.is_set = true;
        g_af_setting.af_mode.is_set = true;
        g_af_setting.af_mode.value.v_int = atoi(optarg);
        break;

        //noise filter settings
      case MCTF_STRENGTH:
        VERIFY_PARA_2(atoi(optarg), 0, 11);
        g_noise_filter_setting.is_set = true;
        g_noise_filter_setting.mctf_strength.is_set = true;
        g_noise_filter_setting.mctf_strength.value.v_int = atoi(optarg);
        break;

        //image style settings
      case QUICK_STYLE_CODE:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.quick_style_code.is_set = true;
        g_image_style_setting.quick_style_code.value.v_int = atoi(optarg);
        break;
      case HUE:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.hue.is_set = true;
        g_image_style_setting.hue.value.v_int = atoi(optarg);
        break;
      case SATURATION:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.saturation.is_set = true;
        g_image_style_setting.saturation.value.v_int = atoi(optarg);
        break;
      case SHARPNESS:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.sharpness.is_set = true;
        g_image_style_setting.sharpness.value.v_int = atoi(optarg);
        break;
      case BRIGHTNESS:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.brightness.is_set = true;
        g_image_style_setting.brightness.value.v_int = atoi(optarg);
        break;
      case CONTRAST:
        VERIFY_PARA_2(atoi(optarg), -2, 2);
        g_image_style_setting.is_set = true;
        g_image_style_setting.contrast.is_set = true;
        g_image_style_setting.contrast.value.v_int = atoi(optarg);
        break;
      case AUTO_CONTRAST_MODE:
        VERIFY_PARA_2(atoi(optarg), 0, 128);
        g_image_style_setting.is_set = true;
        g_image_style_setting.auto_contrast_mode.is_set = true;
        g_image_style_setting.auto_contrast_mode.value.v_int = atoi(optarg);
        break;

      case SHOW_AE_SETTING:
        show_ae_flag = true;
        break;
      case SHOW_AWB_SETTING:
        show_awb_flag = true;
        break;
      case SHOW_AF_SETTING:
        show_af_flag = true;
        break;
      case SHOW_NOISE_FILTER_SETTING:
        show_noise_filter_flag = true;
        break;
      case SHOW_IMAGE_STYLE_SETTING:
        show_image_style_flag = true;
        break;
      case SHOW_ALL_SETTING:
        show_all_flag = true;
        break;
      default:
        PRINTF("unknown option found: %d\n", ch);
        break;
    }
  }

  return 0;
}

static int32_t load_aeb_adj_bin()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  if (load_aeb_adj_flag) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_ADJ_LOAD,
                              aeb_adj_bin_file,
                              FILE_NAME_LENGTH,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to load bin file!\n");
    }
  }
  return ret;
}

static int32_t ae_setting(void)
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  bool has_setting = false;
  am_ae_config_s ae_setting_set =
  {0};
  if (!g_ae_setting.is_set) {
    return ret;
  }
  if (g_ae_setting.ae_metering_mode.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_AE_METERING_MODE);
    ae_setting_set.ae_metering_mode = g_ae_setting.ae_metering_mode.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.day_night_mode.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_DAY_NIGHT_MODE);
    ae_setting_set.day_night_mode = g_ae_setting.day_night_mode.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.slow_shutter_mode.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SLOW_SHUTTER_MODE);
    ae_setting_set.slow_shutter_mode =
        g_ae_setting.slow_shutter_mode.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.anti_flicker_mode.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_ANTI_FLICKER_MODE);
    ae_setting_set.anti_flicker_mode =
        g_ae_setting.anti_flicker_mode.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.ae_target_ratio.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_AE_TARGET_RATIO);
    ae_setting_set.ae_target_ratio = g_ae_setting.ae_target_ratio.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.back_light_comp.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_BACKLIGHT_COMP_ENABLE);
    ae_setting_set.backlight_comp_enable =
        g_ae_setting.back_light_comp.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.local_exposure.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_LOCAL_EXPOSURE);
    ae_setting_set.local_exposure = g_ae_setting.local_exposure.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.dc_iris_enable.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_DC_IRIS_ENABLE);
    ae_setting_set.dc_iris_enable = g_ae_setting.dc_iris_enable.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.sensor_gain_max.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MAX);
    ae_setting_set.sensor_gain_max = g_ae_setting.sensor_gain_max.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.shutter_time_min.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MIN);
    ae_setting_set.sensor_shutter_min =
        g_ae_setting.shutter_time_min.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.shutter_time_max.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MAX);
    ae_setting_set.sensor_shutter_max =
        g_ae_setting.shutter_time_max.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.ir_led_mode.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_IR_LED_MODE);
    ae_setting_set.ir_led_mode = g_ae_setting.ir_led_mode.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.sensor_gain_manual.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SENSOR_GAIN_MANUAL);
    ae_setting_set.sensor_gain = g_ae_setting.sensor_gain_manual.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.shutter_time_manual.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_SENSOR_SHUTTER_MANUAL);
    ae_setting_set.sensor_shutter =
        g_ae_setting.shutter_time_manual.value.v_int;
    has_setting = true;
  }
  if (g_ae_setting.ae_enable.is_set) {
    SET_BIT(ae_setting_set.enable_bits, AM_AE_CONFIG_AE_ENABLE);
    ae_setting_set.ae_enable = g_ae_setting.ae_enable.value.v_int;
    has_setting = true;
  }
  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET,
                              &ae_setting_set,
                              sizeof(ae_setting_set),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set AE config!\n");
    }
  }
  return ret;
}

static int32_t awb_setting(void)
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  bool has_setting = false;
  am_awb_config_s awb_setting_set =
  {0};
  if (!g_awb_setting.is_set) {
    return ret;
  }
  if (g_awb_setting.wb_mode.is_set) {
    SET_BIT(awb_setting_set.enable_bits, AM_AWB_CONFIG_WB_MODE);
    awb_setting_set.wb_mode = g_awb_setting.wb_mode.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET,
                              &awb_setting_set,
                              sizeof(awb_setting_set),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set AWB config!\n");
    }
  }
  return ret;
}

static int32_t af_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  bool has_setting = false;
  am_af_config_s af_setting_set =
  {0};
  if (!g_af_setting.is_set) {
    return ret;
  }
  if (g_af_setting.af_mode.is_set) {
    SET_BIT(af_setting_set.enable_bits, AM_AF_CONFIG_AF_MODE);
    af_setting_set.af_mode = g_af_setting.af_mode.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET,
                              &af_setting_set,
                              sizeof(af_setting_set),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set AF config!\n");
    }
  }
  return ret;
}

static int32_t noise_filter_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  bool has_setting = false;
  am_noise_filter_config_s noise_filter_setting_set =
  {0};
  if (!g_noise_filter_setting.is_set) {
    return ret;
  }
  if (g_noise_filter_setting.mctf_strength.is_set) {
    SET_BIT(noise_filter_setting_set.enable_bits,
            AM_NOISE_FILTER_CONFIG_MCTF_STRENGTH);
    noise_filter_setting_set.mctf_strength =
        g_noise_filter_setting.mctf_strength.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET,
                              &noise_filter_setting_set,
                              sizeof(noise_filter_setting_set),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set noise filter config!\n");
    }
  }
  return ret;
}

static int32_t image_style_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  bool has_setting = false;
  am_image_style_config_s image_style_setting_set =
  {0};
  if (!g_image_style_setting.is_set) {
    return ret;
  }
  if (g_image_style_setting.quick_style_code.is_set) {
    SET_BIT(image_style_setting_set.enable_bits,
            AM_IMAGE_STYLE_CONFIG_QUICK_STYLE_CODE);
    image_style_setting_set.quick_style_code =
        g_image_style_setting.quick_style_code.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.hue.is_set) {
    SET_BIT(image_style_setting_set.enable_bits, AM_IMAGE_STYLE_CONFIG_HUE);
    image_style_setting_set.hue = g_image_style_setting.hue.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.saturation.is_set) {
    SET_BIT(image_style_setting_set.enable_bits,
            AM_IMAGE_STYLE_CONFIG_SATURATION);
    image_style_setting_set.saturation =
        g_image_style_setting.saturation.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.sharpness.is_set) {
    SET_BIT(image_style_setting_set.enable_bits,
            AM_IMAGE_STYLE_CONFIG_SHARPNESS);
    image_style_setting_set.sharpness =
        g_image_style_setting.sharpness.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.brightness.is_set) {
    SET_BIT(image_style_setting_set.enable_bits,
            AM_IMAGE_STYLE_CONFIG_BRIGHTNESS);
    image_style_setting_set.brightness =
        g_image_style_setting.brightness.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.contrast.is_set) {
    SET_BIT(image_style_setting_set.enable_bits, AM_IMAGE_STYLE_CONFIG_CONTRAST);
    image_style_setting_set.contrast =
        g_image_style_setting.contrast.value.v_int;
    has_setting = true;
  }
  if (g_image_style_setting.auto_contrast_mode.is_set) {
    SET_BIT(image_style_setting_set.enable_bits, AM_IMAGE_STYLE_CONFIG_AUTO_CONTRAST_MODE);
    image_style_setting_set.auto_contrast_mode =
        g_image_style_setting.auto_contrast_mode.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET,
                              &image_style_setting_set,
                              sizeof(image_style_setting_set),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set image_style config!\n");
    }
  }
  return ret;
}

static int32_t show_ae_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET,
                              nullptr,
                              0,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get AE setting!\n");
      break;
    }
    am_ae_config_s *ae_setting_get = (am_ae_config_s *) service_result.data;

    PRINTF("ae_metering_mode=%d,\t\t" "day_night_mode=%d\n"
           "slow_shutter_mode=%d,\t\t" "anti_flicker_mode=%d\n"
           "ae_target_ratio=%d,\t\t" "backlight_comp_enable=%d\n"
           "local_exposure=%d,\t\t" "dc_iris_enable=%d\n"
           "ae_enable=%d,\t\t\t" "ir_led_mode=%d\n"
           "sensor_shutter=1/%dseconds,\t" "sensor_gain=%ddB\n"
           "luma_value: RGB_luma=%d,\t" "CFA_luma=%d\n"
           "shutter_time_min=1/%dseconds, " "shutter_time_max=1/%dseconds\n"
           "sensor_gain_max=%ddB\n\n",
           ae_setting_get->ae_metering_mode, ae_setting_get->day_night_mode,
           ae_setting_get->slow_shutter_mode, ae_setting_get->anti_flicker_mode,
           ae_setting_get->ae_target_ratio, ae_setting_get->backlight_comp_enable,
           ae_setting_get->local_exposure, ae_setting_get->dc_iris_enable,
           ae_setting_get->ae_enable, ae_setting_get->ir_led_mode,
           ae_setting_get->sensor_shutter, ae_setting_get->sensor_gain,
           ae_setting_get->luma_value[0], ae_setting_get->luma_value[1],
           ae_setting_get->sensor_shutter_min, ae_setting_get->sensor_shutter_max,
           ae_setting_get->sensor_gain_max);
  } while (0);

  return ret;
}

static int32_t show_awb_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET,
                              nullptr,
                              0,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get AWB setting!\n");
      break;
    }
    am_awb_config_s *awb_setting_get = (am_awb_config_s *) service_result.data;

    PRINTF("WB mode=%d.\n\n", awb_setting_get->wb_mode);
  } while (0);

  return ret;
}

static int32_t show_af_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET,
                              nullptr,
                              0,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get AF setting!\n");
      break;
    }
    am_af_config_s *af_setting_get = (am_af_config_s *) service_result.data;

    PRINTF("AF mode=%d.\n\n", af_setting_get->af_mode);
  } while (0);

  return ret;
}

static int32_t show_noise_filter_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET,
                              nullptr,
                              0,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get noise filter setting!\n");
      break;
    }
    am_noise_filter_config_s *noise_filter_setting_get =
        (am_noise_filter_config_s *) service_result.data;
    PRINTF("MCTF strength=%d.\n\n", noise_filter_setting_get->mctf_strength);
  } while (0);

  return ret;
}

static int32_t show_image_style_setting()
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET,
                              nullptr,
                              0,
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get image style setting!\n");
      break;
    }
    am_image_style_config_s *image_style_setting_get =
        (am_image_style_config_s *) service_result.data;
    PRINTF("quick style code=%d, hue=%d, saturation=%d, brightness=%d, sharpness=%d, contrast=%d, auto_contrast_mode=%d\n\n",
           image_style_setting_get->quick_style_code,
           image_style_setting_get->hue,
           image_style_setting_get->saturation,
           image_style_setting_get->brightness,
           image_style_setting_get->sharpness,
           image_style_setting_get->contrast,
           image_style_setting_get->auto_contrast_mode);
  } while (0);

  return ret;
}

static int32_t show_all_setting()
{
  int32_t ret = 0;
  do {
    if ((ret = show_ae_setting()) < 0) {
      break;
    }
    if ((ret = show_awb_setting()) < 0) {
      break;
    }
    if ((ret = show_af_setting()) < 0) {
      break;
    }
    if ((ret = show_noise_filter_setting()) < 0) {
      break;
    }
    if ((ret = show_image_style_setting()) < 0) {
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
    if (load_aeb_adj_flag) {
      load_aeb_adj_bin();
    }
    if (show_all_flag) {
      show_all_setting();
      break;
    } else {
      if (show_ae_flag) {
        show_ae_setting();
      }
      if (show_awb_flag) {
        show_awb_setting();
      }
      if (show_af_flag) {
        show_af_setting();
      }
      if (show_noise_filter_flag) {
        show_noise_filter_setting();
      }
      if (show_image_style_flag) {
        show_image_style_setting();
      }
      if (show_all_flag | show_ae_flag | show_awb_flag | show_af_flag
          | show_noise_filter_flag | show_image_style_flag) {
        break;
      }
    }

    if ((ret = ae_setting()) < 0) {
      break;
    }
    if ((ret = awb_setting()) < 0) {
      break;
    }
    if ((ret = af_setting()) < 0) {
      break;
    }
    if ((ret = noise_filter_setting()) < 0) {
      break;
    }
    if ((ret = image_style_setting()) < 0) {
      break;
    }
  } while (0);

  return ret;
}

