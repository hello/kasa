/*******************************************************************************
 * am_api_image.h
 *
 * History:
 *   Dec 29, 2014 - [binwang] created file
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
/*! @file am_api_image.h
 *  @brief This file defines Image Service related data structures
 */
#ifndef AM_API_IMAGE_H_
#define AM_API_IMAGE_H_

#include "commands/am_api_cmd_image.h"

/*! @defgroup airapi-datastructure-image Data Structure of Image Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Image Service related method call data structures
 *  @{
 */

/* this header file defines APIs for image service (3A & image quality )
 * configurations and control.    */

/*! @defgroup airapi-datastructure-image-ae Data Structure of AE Settings
 *  @ingroup airapi-datastructure-image
 *  @brief AE related parameters,
 *         refer to @ref airapi-commandid-image-ae "AE Commands" to see
 *         related commands.
 *  @{
 */
/*image auto exposure configs*/

/*! @enum AM_AE_CONFIG_BIT
 *  @brief Defines AE related config enable bits
 */
enum AM_AE_CONFIG_BIT
{
  AM_AE_CONFIG_AE_METERING_MODE = 0, //!< AE metering mode
  AM_AE_CONFIG_DAY_NIGHT_MODE = 1, //!< night mode
  AM_AE_CONFIG_SLOW_SHUTTER_MODE = 2, //!< slow shutter mode
  AM_AE_CONFIG_ANTI_FLICKER_MODE = 3, //!< anti-flicker mode
  AM_AE_CONFIG_AE_TARGET_RATIO = 4, //!< AE target ratio
  AM_AE_CONFIG_BACKLIGHT_COMP_ENABLE = 5, //!< back-light compensation
  AM_AE_CONFIG_LOCAL_EXPOSURE = 6, //!< local exposure
  AM_AE_CONFIG_DC_IRIS_ENABLE = 7, //!< DC-iris
  AM_AE_CONFIG_SENSOR_GAIN_MAX = 8, //!< sensor gain max
  AM_AE_CONFIG_SENSOR_SHUTTER_MIN = 9, //!< sensor shutter minimum
  AM_AE_CONFIG_SENSOR_SHUTTER_MAX = 10, //!< sensor shutter maximum
  AM_AE_CONFIG_SENSOR_GAIN_MANUAL = 11, //!< sensor gain manual
  AM_AE_CONFIG_SENSOR_SHUTTER_MANUAL = 12, //!< sensor shutter manual
  AM_AE_CONFIG_IR_LED_MODE = 13, //!< LED mode
  AM_AE_CONFIG_AE_ENABLE = 14, //!< AE enable
};

/*! @struct am_ae_config_s
 *  @brief Defines AE config parameters
 *  @sa AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET
 *  @sa AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET
 */
struct am_ae_config_s
{
    /*!
     * @sa AM_AE_CONFIG_BIT
     */
    uint32_t enable_bits;

    /*! percentage of ae target to use, [25,400], default value is 100
     * - < 100 (get darker picture)
     * - > 100 (get brighter picture)
     */
    uint32_t ae_target_ratio;

    /*! minimum sensor shutter time */
    uint32_t sensor_shutter_min;

    /*! maximum sensor shutter time */
    uint32_t sensor_shutter_max;

    /*!
     * image sensor analog gain, in dB unit;
     * this field is used to get current sensor gain,
     * or manually set sensor gain when AE is disabled.
     */
    uint32_t sensor_gain;

    /*!
     * image sensor shutter time, 1/x seconds unit;
     * this field is used to get current sensor shutter time,
     * or manually set sensor shutter time when AE is disabled.
     */
    uint32_t sensor_shutter;

    /*!
     *  - luma_value[0]RGB luma
     *  - lumavalue[1]CFA luma
     */
    uint32_t luma_value[2];

    /*!
     * - 0: spot
     * - 1: center(default)
     * - 2: average
     * - 3: custom
     */
    uint8_t ae_metering_mode;

    /*!
     * - 0: day mode, colorful(default)
     * - 1: night mode, black&white
     */
    uint8_t day_night_mode;

    /*!
     * - 0: off,
     * - 1: auto(default)
     */
    uint8_t slow_shutter_mode;

    /*!
     * - 0: 50Hz (China, EU)
     * - 1: 60Hz (US, TW)(default)
     */
    uint8_t anti_flicker_mode;

    /*!
     * - 0: off(default)
     * - 1: on
     */
    uint8_t backlight_comp_enable;

    /*!
     * - 0: off(default)
     * - 1: weak
     * - 2: normal
     * - 3: strong
     */
    uint8_t local_exposure;

    /*!
     * - 0: disabled(default)
     * - 1: enabled
     */
    uint8_t dc_iris_enable;

    /*!
     * maximum sensor analog gain, minimum sensor analog gain usually is 0 dB
     */
    uint8_t sensor_gain_max;

    /*!
     * - 0: off
     * - 1: on
     * - 2: auto(default)
     */
    uint8_t ir_led_mode;

    /*!
     * - 0: disable AE
     * - 1: enable AE(default)
     */
    uint8_t ae_enable;
};
/*! @} */

/*! @defgroup airapi-datastructure-image-awb Data Structure of AWB Settings
 *  @ingroup airapi-datastructure-image
 *  @brief AWB related parameters,
 *         refer to @ref airapi-commandid-image-awb "AWB Commands" to see
 *         related commands.
 *  @{
 */
/*! image auto white balance config enable bit number */
#define AM_AWB_CONFIG_WB_MODE 0

/*! @struct am_awb_config_s
 *  @brief Defines AWB related parameters
 *  @sa AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET
 *  @sa AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET
 */
struct am_awb_config_s
{
    /*!
     * @sa AM_AWB_CONFIG_WB_MODE
     */
    uint32_t enable_bits;

    /*!
     * - 0: auto(default)
     * - 1: 2800K
     * - 2: D4000
     * - 3: D5000
     * - 4: SUNNY(6500K)
     * - 5: CLOUD(7500K)
     * - 6: FLASH
     * - 7: FLUORESCENT
     * - 8: FLUORESCENT_H
     * - 9: UNDERWATER
     * - 10: CUSTOM
     */
    uint32_t wb_mode;
};
/*! @} */

/*! @defgroup airapi-datastructure-image-af Data Structure of AF Settings
 *  @ingroup airapi-datastructure-image
 *  @brief AF related parameters,
 *         refer to @ref airapi-commandid-image-af "AF Commands" to see
 *         related commands.
 *  @{
 */
/*! image auto focus config enable bit number */
#define AM_AF_CONFIG_AF_MODE 0

/*! @struct am_af_config_s
 *  @brief Defines AF related parameters
 *  @sa AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET
 *  @sa AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET
 */
struct am_af_config_s
{
    /*!
     * @sa AM_AF_CONFIG_AF_MODE
     */
    uint32_t enable_bits;

    /*! - 0: default
     */
    uint32_t af_mode;
};
/*! @} */

/*! @defgroup airapi-datastructure-image-nr
 *            Data Structure of Noise Reduction Settings
 *  @ingroup airapi-datastructure-image
 *  @brief Noise Reduction related parameters,
 *         refer to @ref airapi-commandid-image-nr "Noise Reduction Commands"
 *         to see related commands.
 *  @{
 */
/*! @enum AM_NOISE_FILTER_CONFIG_BIT
 *  @brief image noise filter configs
 */
enum AM_NOISE_FILTER_CONFIG_BIT
{
  AM_NOISE_FILTER_CONFIG_MCTF_STRENGTH = 0, //!< MCTF Strength
  AM_NOISE_FILTER_CONFIG_FIR_PARAM = 1, //!< FIR parameter
  AM_NOISE_FILTER_CONFIG_SPATIAL_NR = 2, //!< SPATIAL
  AM_NOISE_FILTER_CONFIG_CHROMA_NR = 3, //!< CHROMA
};

/*! @struct am_noise_filter_config_s
 *  @brief Defines noise filter related parameters
 *  @sa AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET
 *  @sa AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET
 */
struct am_noise_filter_config_s
{
    /*!
     * @sa AM_NOISE_FILTER_CONFIG_BIT
     */
    uint32_t enable_bits;

    /*!
     * - 0: off (or zero strength)
     * - 1~255  different levels of mctf strength
     * - default is 64
     * - 255 is strongest
     */
    uint32_t mctf_strength;

    /*! image FIR config */
    uint8_t FIR_param;

    /*! spatial noise reduction */
    uint8_t spatial_nr;

    /*! chroma noise reduction */
    uint8_t chroma_nr;

    /*! defined to keep alignment */
    uint8_t reserved;
};
/*! @} */

/*! @defgroup airapi-datastructure-image-style
 *            Data Structure of Image Style Settings
 *  @ingroup airapi-datastructure-image
 *  @brief Image Style related parameters,
 *         refer to @ref airapi-commandid-image-style "Image Style Commands"
 *         to see related commands.
 *  @{
 */
/*! @enum AM_IMAGE_STYLE_CONFIG_BIT
 *  @brief Defines image style config enable bits
 */
enum AM_IMAGE_STYLE_CONFIG_BIT
{
  AM_IMAGE_STYLE_CONFIG_QUICK_STYLE_CODE = 0,
  AM_IMAGE_STYLE_CONFIG_HUE = 1,
  AM_IMAGE_STYLE_CONFIG_SATURATION = 2,
  AM_IMAGE_STYLE_CONFIG_SHARPNESS = 3,
  AM_IMAGE_STYLE_CONFIG_BRIGHTNESS = 4,
  AM_IMAGE_STYLE_CONFIG_CONTRAST = 5,
  AM_IMAGE_STYLE_CONFIG_BLACK_LEVEL = 6,
  AM_IMAGE_STYLE_CONFIG_AUTO_CONTRAST_MODE = 7,
};

/*! @struct am_image_style_config_s
 *  @brief Defines image style related parameters
 *
 *  image style is "high level" user preference
 *  @sa AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET
 *  @sa AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET
 */
struct am_image_style_config_s
{
    /*!
     * @sa AM_IMAGE_STYLE_CONFIG_BIT
     */
    uint32_t enable_bits;

    /*!
     * - 0:(default, disable) range[0,128]
     * - 1: weakest, 128: strongest
     */
    int32_t auto_contrast_mode;

    /*!
     * - 0: default style(lab style)
     * - 1: China IPCAM style
     * - 2: World wide IPCAM style
     * - 3: Internet style ( low bitrate optimize)
     */
    uint8_t quick_style_code;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t hue;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t saturation;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t sharpness;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t brightness;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t contrast;

    /*!
     * - 0: (default) range[-2, 2]
     * - -2, -1: less
     * - +1, +2: more
     */
    int8_t black_level;

    /*!
     * defined to keep alignment
     */
    uint8_t reserved;
};
/*! @} */

struct am_image_3A_info_s
{
    am_ae_config_s ae;
    am_af_config_s af;
    am_awb_config_s awb;
};

/*! @} */
#endif /* AM_API_IMAGE_H_ */
