/*******************************************************************************
 * am_image_quality_test.cpp
 *
 * History:
 *   Jan 4, 2015 - [binwang] created file
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
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

//C++ 11 thread
#include <thread>
#include <mw_struct.h>
#include "am_log.h"
#include "am_define.h"
#include "am_image_quality_if.h"

AMIImageQualityPtr g_image_quality;
AM_IQ_CONFIG iq_config;

static void sigstop(int arg)
{
}

static void show_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  r -- Run Image Quality\n");
  PRINTF("  s -- Stop Image Quality\n");
  PRINTF("  l -- Load settings from config file and print out\n");
  PRINTF("  w -- Write current config into config file\n");
  PRINTF("  c -- Change settings on the fly\n");
  PRINTF("  q -- Quit Image Quality\n");
  PRINTF("================================================\n\n");
  PRINTF("> ");
}

static void show_config_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  e -- AE settings\n");
  PRINTF("  w -- AWB settings\n");
  PRINTF("  f -- AF settings \n");
  PRINTF("  n -- Noise filter settings\n");
  PRINTF("  i -- Image style settings \n");
  PRINTF("  q -- Quit \n");
  PRINTF("================================================\n\n");
  PRINTF("> ");
}

static void show_ae_menu()
{
  PRINTF("\n==========================================================\n");
  PRINTF("m-----------------ae_metering_mode (0: spot 1: center 2: average 3: custom 4: type number\n");
  PRINTF("d-----------------day_night_mode (0: colorful    1: black&white)\n");
  PRINTF("o-----------------slow_shutter_mode (0: off    1: auto)\n");
  PRINTF("a-----------------anti_flicker (0: 50Hz    1: 60Hz)\n");
  PRINTF("t-----------------ae_target_ratio (100: default (25 ~ 400))\n");
  PRINTF("b-----------------back_light_comp (0: disable    1: enable)\n");
  PRINTF("l-----------------local_exposure ( 0|1|2|3, 3 is the strongest)\n");
  PRINTF("i-----------------dc_iris_enable (0: disable    1: enable)\n");
  PRINTF("g-----------------sensor_gain_max (0 ~ sensor's max analog gain dB)\n");
  PRINTF("s-----------------shutter_time_min\n");
  PRINTF("S-----------------shutter_time_max\n");
  PRINTF("I-----------------ir_led_mode (0:off 1:on 2:auto)\n");
  PRINTF("G-----------------sensor_gain_manual (0 ~ sensor's max analog gain dB)\n");
  PRINTF("T-----------------shutter_time_manual(30, 60, 120...)\n");
  PRINTF("p-----------------show ae info (current shutter time, sensor gain, luma)\n");
  PRINTF("e-----------------AE enable or disable (0: disable, 1: enable)\n");
  PRINTF("q-----------------quit\n");
  PRINTF("==========================================================\n");
  PRINTF("> ");
}

static void show_awb_menu()
{
  PRINTF("\n===========================\n");
  PRINTF("m-----------------wb_mode (0:MW_WB_AUTO, 1:INCANDESCENT(2800K), 2:D4000, 3:D5000, 4:SUNNY(6500K), 5:CLOUDY(7500K), 6:FLASH, 7:FLUORESCENT, 8:FLUORESCENT_H, 9:UNDERWATER)\n");
  PRINTF("q-----------------quit\n");
  PRINTF("===========================\n");
  PRINTF("> ");
}

static void show_af_menu()
{
  PRINTF("\n==========================================\n");
  PRINTF("m-----------------af_mode (0:CAF, 1:SAF, 3:MANUAL, 4:CALIB, 5:DEBUG)\n");
  PRINTF("q-----------------quit\n");
  PRINTF("==========================================\n");
  PRINTF("> ");
}

static void show_noise_filter_menu()
{
  PRINTF("\n==========================================\n");
  PRINTF("m----------------mctf_strength (0: off) 1~255  different levels of mctf strength 255 is strongest\n");
  PRINTF("f----------------FIR_param \n");
  PRINTF("s----------------spatial_nr\n");
  PRINTF("c----------------chroma_nr\n");
  PRINTF("q----------------quit\n");
  PRINTF("==========================================\n");
  PRINTF("> ");
}

static void show_image_style_menu()
{
  PRINTF("\n==========================================\n");
  PRINTF("h----------------hue(0~255)\n");
  PRINTF("s----------------saturation(0~255)\n");
  PRINTF("p----------------sharpness(0~128)\n");
  PRINTF("b----------------brightness(-255~255)\n");
  PRINTF("o----------------contrast(0~128)\n");
  PRINTF("q----------------quit\n");
  PRINTF("==========================================\n");
  PRINTF("> ");
}

static bool ae_setting()
{
  bool ret = true;
  int i;
  int exit_flag, error_opt;
  char ch;
  show_ae_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'm':
        iq_config.key = AM_IQ_AE_METERING_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_METERING_MODE config error\n");
          break;
        }
        PRINTF("current ae_metering_mode = %d\n"
               "input ae_metering_mode: \n>",
               *(AM_AE_METERING_MODE* )(iq_config.value));
        scanf("%d", &i);
        iq_config.value = (void*) &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_METERING_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_METERING_MODE config after set error\n");
          break;
        }
        PRINTF("ae_meter_mode is %d\n",
               *(AM_AE_METERING_MODE* )(iq_config.value));
        break;
      case 'd':
        iq_config.key = AM_IQ_AE_DAY_NIGHT_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_DAY_NIGHT_MODE config error\n");
          break;
        }
        PRINTF("current day_night_mode = %d\n"
               "input day_night_mode: \n>",
               *(AM_DAY_NIGHT_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_DAY_NIGHT_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_DAY_NIGHT_MODE config after set error\n");
          break;
        }
        PRINTF("day_night_mode is %d\n", *(AM_DAY_NIGHT_MODE* )iq_config.value);
        break;
      case 'o':
        iq_config.key = AM_IQ_AE_SLOW_SHUTTER_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_SLOW_SHUTTER_MODE config error\n");
          break;
        }
        PRINTF("current slow_shutter_mode = %d\n"
               "input slow_shutter_mode: \n>",
               *(AM_SLOW_SHUTTER_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_SLOW_SHUTTER_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_SLOW_SHUTTER_MODE config after set error\n");
          break;
        }
        PRINTF("slow_shutter_mode is %d\n",
               *(AM_SLOW_SHUTTER_MODE* )iq_config.value);
        break;
      case 'a':
        iq_config.key = AM_IQ_AE_ANTI_FLICKER_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_ANTI_FLICKER_MODE config error\n");
          break;
        }
        PRINTF("current anti_flick_mode = %d\n"
               "input anti_flick_mode: \n>",
               *(AM_ANTI_FLICK_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_ANTI_FLICKER_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_ANTI_FLICKER_MODE config after set error\n");
          break;
        }
        PRINTF("anti_flick_mode is %d\n",
               *(AM_ANTI_FLICK_MODE* )iq_config.value);
        break;
      case 't':
        iq_config.key = AM_IQ_AE_TARGET_RATIO;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_TARGET_RATIO config error\n");
          break;
        }
        PRINTF("current ae_target_ratio = %d\n"
               "input ae_target_ratio: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_TARGET_RATIO config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_TARGET_RATIO config after set error\n");
          break;
        }
        PRINTF("ae_target_ratio is %d\n", *(int* )iq_config.value);
        break;
      case 'b':
        iq_config.key = AM_IQ_AE_BACKLIGHT_COMP_ENABLE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_BACKLIGHT_COMP_ENABLE config error\n");
          break;
        }
        PRINTF("current backlight_comp_enable = %d\n"
               "input backlight_comp_enable: \n>",
               *(AM_BACKLIGHT_COMP_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_BACKLIGHT_COMP_ENABLE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_BACKLIGHT_COMP_ENABLE config after set error\n");
          break;
        }
        PRINTF("backlight_comp_enable is %d\n",
               *(AM_BACKLIGHT_COMP_MODE* )iq_config.value);
        break;
      case 'l':
        iq_config.key = AM_IQ_AE_LOCAL_EXPOSURE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_LOCAL_EXPOSURE config error\n");
          break;
        }
        PRINTF("current local_exposure_mode = %d\n"
               "input local_exposure_mode: \n>",
               *(uint32_t* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_LOCAL_EXPOSURE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_LOCAL_EXPOSURE config after set error\n");
          break;
        }
        PRINTF("local_exposure_mode is %d\n",
               *(uint32_t* )iq_config.value);
        break;
      case 'i':
        iq_config.key = AM_IQ_AE_DC_IRIS_ENABLE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_DC_IRIS_ENABLE config error\n");
          break;
        }
        PRINTF("current dc_iris_enable = %d\n"
               "input dc_iris_enable: \n>",
               *(AM_DC_IRIS_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_DC_IRIS_ENABLE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_DC_IRIS_ENABLE config after set error\n");
          break;
        }
        PRINTF("dc_iris_enable is %d\n", *(AM_DC_IRIS_MODE* )iq_config.value);
        break;
      case 'g':
        iq_config.key = AM_IQ_AE_SENSOR_GAIN_MAX;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_SENSOR_GAIN_MAX config error\n");
          break;
        }
        PRINTF("current sensor_gain_max = %d\n"
               "input sensor_gain_max: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_SENSOR_GAIN_MAX config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_SENSOR_GAIN_MAX config after set error\n");
          break;
        }
        PRINTF("sensor_gain_max is %d\n", *(int* )iq_config.value);
        break;
      case 's':
        iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MIN;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MIN config error\n");
          break;
        }
        PRINTF("current sensor_shutter_min = %d\n"
               "input sensor_shutter_min: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_SENSOR_SHUTTER_MIN config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MIN config after set error\n");
          break;
        }
        PRINTF("sensor_shutter_min is %d\n", *(int* )iq_config.value);
        break;
      case 'S':
        iq_config.key = AM_IQ_AE_SENSOR_SHUTTER_MAX;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MAX config error\n");
          break;
        }
        PRINTF("current sensor_shutter_max = %d\n"
               "input sensor_shutter_max: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_SENSOR_SHUTTER_MAX config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SENSOR_SHUTTER_MAX config after set error\n");
          break;
        }
        PRINTF("sensor_shutter_max is %d\n", *(int* )iq_config.value);
        break;
      case 'I':
        iq_config.key = AM_IQ_AE_IR_LED_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_IR_LED_MODE config error\n");
          break;
        }
        PRINTF("current ir_led_mode = %d\n"
               "input ir_led_mode: \n>",
               *(AM_IR_LED_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_IR_LED_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_IR_LED_MODE config after set error\n");
          break;
        }
        PRINTF("ir_led_mode is %d\n", *(AM_IR_LED_MODE* )iq_config.value);
        break;
      case 'G':
        iq_config.key = AM_IQ_AE_AE_ENABLE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_AE_ENABLE config error\n");
          break;
        }
        if (*(AM_AE_MODE *) iq_config.value != AM_AE_DISABLE) {
          ERROR("Disable AE first!\n");
          ret = false;
          break;
        }
        iq_config.key = AM_IQ_AE_SENSOR_GAIN;
        ret = g_image_quality->get_config(&iq_config);
        PRINTF("current sensor_gain = %d\n"
               "input sensor_gain: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_SENSOR_GAIN config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SENSOR_GAIN config after set error\n");
          break;
        }
        PRINTF("sensor_gain is %d\n", *(int* )iq_config.value);
        break;
      case 'T':
        iq_config.key = AM_IQ_AE_AE_ENABLE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_AE_ENABLE config error\n");
          break;
        }
        if (*(AM_AE_MODE *) iq_config.value != AM_AE_DISABLE) {
          ERROR("Disable AE first!\n");
          ret = false;
          break;
        }
        iq_config.key = AM_IQ_AE_SHUTTER_TIME;
        ret = g_image_quality->get_config(&iq_config);
        PRINTF("current shutter_time = %d\n"
               "input shutter_time: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_SHUTTER_TIME config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_SHUTTER_TIME config after set error\n");
          break;
        }
        PRINTF("shutter_time is %d\n", *(int* )iq_config.value);
        break;
      case 'p':
        iq_config.key = AM_IQ_AE_SHUTTER_TIME;
        ret = g_image_quality->get_config(&iq_config);
        PRINTF("current shutter time = %d, ", *(int* )iq_config.value);
        iq_config.key = AM_IQ_AE_SENSOR_GAIN;
        ret = g_image_quality->get_config(&iq_config);
        PRINTF("current sensor gain = %d, ", *(int* )iq_config.value);
        iq_config.key = AM_IQ_AE_LUMA;
        ret = g_image_quality->get_config(&iq_config);
        PRINTF("current RGB luma = %d, CFA luma = %d\n",
               ((AMAELuma * ) iq_config.value)->rgb_luma,
               ((AMAELuma * ) iq_config.value)->cfa_luma);
        break;
      case 'e':
        iq_config.key = AM_IQ_AE_AE_ENABLE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_AE_ENABLE config error\n");
          break;
        }
        PRINTF("current ae_enable = %d\n"
               "input ae_enable: \n>",
               *(AM_AE_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AE_AE_ENABLE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AE_AE_ENABLE config after set error\n");
          break;
        }
        PRINTF("ae_enable is %d\n", *(AM_AE_MODE* )iq_config.value);
        break;
      case 'q':
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_ae_menu();
    }
    ch = getchar();
  }
  return ret;
}

static bool awb_setting(void)
{
  bool ret = true;
  int i;
  int exit_flag, error_opt;
  char ch;
  show_awb_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'm':
        iq_config.key = AM_IQ_AWB_WB_MODE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AWB_WB_MODE config error\n");
          break;
        }
        PRINTF("current wb_mode = %d\n"
               "input wb_mode: \n>",
               *(AM_WB_MODE* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_AWB_WB_MODE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_AWB_WB_MODE config after set error\n");
          break;
        }
        PRINTF("wb_mode is %d\n", *(AM_WB_MODE* )iq_config.value);
        break;
      case 'q':
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_awb_menu();
    }
    ch = getchar();
  }
  return ret;
}

static bool af_setting()
{
  bool ret = true;
  int exit_flag, error_opt;
  char ch;
  show_af_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'm':
        PRINTF("Empty API, do nothing!\n");
        break;
      case 'q':
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_af_menu();
    }
    ch = getchar();
  }
  return ret;
}

static bool noise_filter_setting()
{
  bool ret = true;
  int i;
  int exit_flag, error_opt;
  char ch;
  show_noise_filter_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'm':
        iq_config.key = AM_IQ_NF_MCTF_STRENGTH;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_NF_MCTF_STRENGTH config error\n");
          break;
        }
        PRINTF("current mctf_strength = %d\n"
               "input mctf_strength: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_NF_MCTF_STRENGTH config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_NF_MCTF_STRENGTH config after set error\n");
          break;
        }
        PRINTF("mctf_strength is %d\n", *(int* )iq_config.value);
        break;
      case 'f':
        PRINTF("Empty API, do nothing!\n");
        break;
      case 's':
        PRINTF("Empty API, do nothing!\n");
        break;
      case 'c':
        PRINTF("Empty API, do nothing!\n");
        break;
      case 'q':
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_noise_filter_menu();
    }
    ch = getchar();
  }
  return ret;
}

static bool image_style_setting()
{
  bool ret = true;
  int i;
  int exit_flag, error_opt;
  char ch;
  show_image_style_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'c':
        PRINTF("Empty API, do nothing!\n");
        break;
      case 'h':
        iq_config.key = AM_IQ_STYLE_HUE;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_HUE config error\n");
          break;
        }
        PRINTF("current hue = %d\n"
               "input hue: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_STYLE_HUE config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_HUE config after set error\n");
          break;
        }
        PRINTF("hue is %d\n", *(int* )iq_config.value);
        break;
      case 's':
        iq_config.key = AM_IQ_STYLE_SATURATION;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_SATURATION config error\n");
          break;
        }
        PRINTF("current saturation = %d\n"
               "input saturation: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_STYLE_SATURATION config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_SATURATION config after set error\n");
          break;
        }
        PRINTF("saturation is %d\n", *(int* )iq_config.value);
        break;
      case 'p':
        iq_config.key = AM_IQ_STYLE_SHARPNESS;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_SHARPNESS config error\n");
          break;
        }
        PRINTF("current sharpness = %d\n"
               "input sharpness: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_STYLE_SHARPNESS config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_SHARPNESS config after set error\n");
          break;
        }
        PRINTF("sharpness is %d\n", *(int* )iq_config.value);
        break;
      case 'b':
        iq_config.key = AM_IQ_STYLE_BRIGHTNESS;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_BRIGHTNESS config error\n");
          break;
        }
        PRINTF("current brightness = %d\n"
               "input brightness: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_STYLE_BRIGHTNESS config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_BRIGHTNESS config after set error\n");
          break;
        }
        PRINTF("brightness is %d\n", *(int* )iq_config.value);
        break;
      case 'o':
        iq_config.key = AM_IQ_STYLE_CONTRAST;
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_CONTRAST config error\n");
          break;
        }
        PRINTF("current contrast = %d\n"
               "input contrast: \n>",
               *(int* )iq_config.value);
        scanf("%d", &i);
        iq_config.value = &i;
        ret = g_image_quality->set_config(&iq_config);
        if (!ret) {
          ERROR("set AM_IQ_STYLE_CONTRAST config error\n");
          break;
        }
        ret = g_image_quality->get_config(&iq_config);
        if (!ret) {
          ERROR("get AM_IQ_STYLE_CONTRAST config after set error\n");
          break;
        }
        PRINTF("contrast is %d\n", *(int* )iq_config.value);
        break;
      case 'q':
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_image_style_menu();
    }
    ch = getchar();
  }
  return ret;
}

static void config_setting()
{
  int exit_flag, error_opt;
  char ch;
  show_config_menu();
  ch = getchar();
  while (ch) {
    exit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'e':
        ae_setting();
        break;
      case 'w':
        awb_setting();
        break;
      case 'f':
        af_setting();
        break;
      case 'n':
        noise_filter_setting();
        break;
      case 'i':
        image_style_setting();
        break;
      case 'q':
        INFO("Quit config setting\n");
        exit_flag = 1;
        break;
      default:
        error_opt = 1;
        break;
    }
    if (exit_flag) {
      break;
    }
    if (error_opt == 0) {
      show_config_menu();
    }
    ch = getchar();
  }
}

int main()
{
  bool ret = true;
  char ch;
  int quit_flag;
  int error_opt;
  int config_value = 0;

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  iq_config.value = &config_value;

  g_image_quality = AMIImageQuality::get_instance();
  do {
    if (!g_image_quality) {
      ERROR("ImageQuality get instance failed\n");
      ret = false;
      break;
    }

    INFO("ImageQuality is running\n");
    show_menu();

    ch = getchar();
    while (ch) {
      quit_flag = 0;
      error_opt = 0;
      switch (ch) {
        case 'r':
          INFO("ImageQuality runs\n");
          ret = g_image_quality->start();
          if (!ret) {
            ERROR("ImageQuality runs failed\n");
          }
          break;
        case 's':
          INFO("ImageQuality stops\n");
          ret = g_image_quality->stop();
          if (!ret) {
            ERROR("ImageQuality stops failed\n");
          }
          break;
        case 'l':
          //load config from file
          ret = g_image_quality->load_config();
          if (!ret) {
            ERROR("ImageQuality load config failed \n");
          }
          break;
        case 'c':
          config_setting();
          break;
        case 'w':
          //write config to file
          ret = g_image_quality->save_config();
          if (!ret) {
            ERROR("ImageQuality save config failed \n");
          }
          break;
        case 'q':
          INFO("ImageQuality quit \n");
          g_image_quality = nullptr;
          quit_flag = 1;
          break;
        case 'e':
        case 'd':{
          AM_IQ_CONFIG tem_config;
          char tem_str[FILE_NAME_LENGTH] = {0};
          printf("Please input the file path\n");
          std::cin >> tem_str;
          tem_config.value = tem_str;
          tem_config.key = AM_IQ_AEB_ADJ_BIN_LOAD;
          g_image_quality->set_config(&tem_config);
        } break;
        default:
          error_opt = 1;
          break;
      }

      //quit too if function has error
      if (!ret) {
        quit_flag = 1;
      }
      if (quit_flag) {
        break;
      }
      if (error_opt == 0) {
        show_menu();
      }

      ch = getchar();
    }
  } while (0);

  return 0;
}
