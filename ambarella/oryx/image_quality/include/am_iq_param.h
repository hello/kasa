/*******************************************************************************
 * am_iq_param.h
 *
 * History:
 *   Dec 29, 2014 - [binwang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_IQ_PARAM_H_
#define AM_IQ_PARAM_H_

#include "am_base_include.h"

#define AE_METERING_TABLE_LEN 96

enum AM_IQ_LOG_LEVEL
{
  AM_ERROR_LEVEL = 0,
  AW_MSG_LEVEL,
  AW_INFO_LEVEL,
  AW_DEBUG_LEVEL,
  AW_LOG_LEVEL_NUM
};

enum AM_AAA_FILE_TYPE
{
  AM_FILE_TYPE_ADJ = 0,
  AM_FILE_TYPE_AEB = 1,
  AM_FILE_TYPE_CFG = 2,
  AM_FILE_TYPE_TOTAL_NUM
};

enum AM_AE_METERING_MODE
{
  AM_AE_SPOT_METERING = 0,
  AM_AE_CENTER_METERING,
  AM_AE_AVERAGE_METERING,
  AM_AE_CUSTOM_METERING,
  AM_AE_METERING_TYPE_NUMBER,
};

enum AM_DAY_NIGHT_MODE
{
  AM_DAY_MODE = 0,
  AM_NIGHT_MODE = 1
};

enum AM_SLOW_SHUTTER_MODE
{
  AM_SLOW_SHUTTER_OFF = 0,
  AM_SLOW_SHUTTER_ON = 1
};

enum AM_ANTI_FLICK_MODE
{
  AM_ANTI_FLICKER_50HZ = 0,
  AM_ANTI_FLICKER_60HZ = 1
};

enum AM_BACKLIGHT_COMP_MODE
{
  AM_BACKLIGHT_COMP_OFF = 0,
  AM_BACKLIGHT_COMP_ON = 1
};

enum AM_LOCAL_EXPOSURE_MODE
{
  AM_LE_STOP = 0,
  AM_LE_AUTO,
  AM_LE_2X,
  AM_LE_3X,
  AM_LE_4X,
  AM_LE_TOTAL_NUM,
};

enum AM_DC_IRIS_MODE
{
  AM_DC_IRIS_DISABLE = 0,
  AM_DC_IRIS_ENABLE = 1
};

enum AM_IR_LED_MODE
{
  AM_IR_LED_OFF = 0,
  AM_IR_LED_ON = 1,
  AM_IRLED_AUTO = 2
};

enum AM_AE_MODE
{
  AM_AE_DISABLE = 0,
  AM_AE_ENABLE = 1
};

enum AM_WB_MODE
{
  AM_WB_AUTO = 0,
  AM_WB_INCANDESCENT, // 2800
  AM_WB_D4000,
  AM_WB_D5000,
  AM_WB_SUNNY, // 6500K
  AM_WB_CLOUDY, // 7500K
  AM_WB_FLASH,
  AM_WB_FLUORESCENT,
  AM_WB_FLUORESCENT_H,
  AM_WB_UNDERWATER,
  AM_WB_CUSTOM, // custom
  AM_WB_MODE_NUM
};

struct AMAELuma
{
    uint16_t rgb_luma;
    uint16_t cfa_luma;
};

struct AMAEParam
{
    AM_AE_METERING_MODE ae_metering_mode;
    uint8_t ae_metering_table[AE_METERING_TABLE_LEN];
    AM_DAY_NIGHT_MODE day_night_mode;
    AM_SLOW_SHUTTER_MODE slow_shutter_mode;
    AM_ANTI_FLICK_MODE anti_flicker_mode;
    int32_t ae_target_ratio;
    AM_BACKLIGHT_COMP_MODE backlight_comp_enable;
    AM_LOCAL_EXPOSURE_MODE local_exposure;
    AM_DC_IRIS_MODE dc_iris_enable;
    uint32_t sensor_gain_max;
    uint32_t sensor_shutter_min;
    uint32_t sensor_shutter_max;
    AM_IR_LED_MODE ir_led_mode;
    int32_t shutter_time;
    int32_t sensor_gain;
    AMAELuma luma;
    AM_AE_MODE ae_enable;
};

struct AMAWBParam
{
    AM_WB_MODE wb_mode;
};

struct AMNFParam
{
    uint32_t mctf_strength;
};

struct AMStyleParam
{
    int32_t saturation;
    int32_t brightness;
    int32_t hue;
    int32_t contrast;
    int32_t sharpness;
};

struct AMIQParam
{
    AM_IQ_LOG_LEVEL log_level;
    AMAEParam ae;
    AMAWBParam awb;
    AMNFParam nf;
    AMStyleParam style;
};

struct AM_IQ_CONFIG
{
    uint32_t key;
    void *value;
};

enum AM_IQ_CONFIG_KEY
{
  /*log level config*/
  AM_IQ_LOGLEVEL = 0,
  /*AE config*/
  AM_IQ_AE_METERING_MODE,
  AM_IQ_AE_DAY_NIGHT_MODE,
  AM_IQ_AE_SLOW_SHUTTER_MODE,
  AM_IQ_AE_ANTI_FLICKER_MODE,
  AM_IQ_AE_TARGET_RATIO,
  AM_IQ_AE_BACKLIGHT_COMP_ENABLE,
  AM_IQ_AE_LOCAL_EXPOSURE,
  AM_IQ_AE_DC_IRIS_ENABLE,
  AM_IQ_AE_SENSOR_GAIN_MAX,
  AM_IQ_AE_SENSOR_SHUTTER_MIN,
  AM_IQ_AE_SENSOR_SHUTTER_MAX,
  AM_IQ_AE_IR_LED_MODE,
  AM_IQ_AE_SHUTTER_TIME,
  AM_IQ_AE_SENSOR_GAIN,
  AM_IQ_AE_LUMA,
  AM_IQ_AE_AE_ENABLE,
  /*AWB config*/
  AM_IQ_AWB_WB_MODE,
  /*NF config*/
  AM_IQ_NF_MCTF_STRENGTH,
  /*IQ style config*/
  AM_IQ_STYLE_SATURATION,
  AM_IQ_STYLE_BRIGHTNESS,
  AM_IQ_STYLE_HUE,
  AM_IQ_STYLE_CONTRAST,
  AM_IQ_STYLE_SHARPNESS,
  /*total config key number*/
  AM_MD_KEY_NUM
};

#endif /* AM_IQ_PARAM_H_ */
