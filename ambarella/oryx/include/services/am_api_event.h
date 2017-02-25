/**
 * am_api_event.h
 *
 *  History:
 *    Nov 18, 2014 - [Dongge Wu] created file
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

/*! @file am_api_event.h
 *  @brief This file defines Event Service related data structures
 */
#ifndef _AM_API_EVENT_H_
#define _AM_API_EVENT_H_

#include "commands/am_api_cmd_event.h"

/*! @defgroup airapi-datastructure-event Data Structure of Event Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Event Service related method call data structures
 *  @{
 */

/*! @defgroup airapi-datastructure-event-plugin Event Plug-in Control
 *  @ingroup airapi-datastructure
 *  @brief Event Plug-in Control related data structures,
 *         refer to @ref airapi-commandid-event-plugin "Plug-in Control" to see
 *         related commands
 *  @{
 */

/*! @enum am_event_module_id
 *  @brief Defines event plugin ID
 *  @sa AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET
 *  @sa AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET
 */
enum am_event_module_id
{
  AUDIO_ALERT_DECT = 0,//!< Plugin ID of audio alert
  AUDIO_ANALYSIS_DECT, //!< Plugin ID of audio analysis
  FACE_DECT,           //!< Plugin ID of face detection
  KEY_INPUT_DECT,      //!< Plugin ID of key press detection
  ALL_MODULE_NUM,      //!< Total number of Plugins
};

/*! @typedef am_event_callback
 *  @brief Event callback function type
 *  @param event_msg event message data
 *  @return int32_t
 */
typedef int32_t (*am_event_callback)(void *event_msg);

/*! @struct am_event_module_path
 *  This structure describes event plugin's path
 *
 *  @sa AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET
 *  @sa AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET
 */
struct am_event_module_path
{
    /*!
     * Event plugin's full path
     */
    char *path_name;

    /*!
     * Module ID
     * @sa am_event_module_id
     */
    uint32_t id;
};

/*! @struct am_event_module_register_state
 *  This structure describes event plugin register status
 *
 *  @sa AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET
 */
struct am_event_module_register_state
{
    /*!
     * Module ID
     * @sa am_event_module_id
     */
    uint32_t id;

    /*!
     * - true: registered
     * - false: not registered
     */
    bool state;
};
/*! @} */

/*! @defgroup airapi-datastructure-event-input Key Input
 *  @ingroup airapi-datastructure-event
 *  @brief Key Input related data structure,
 *         refer to @ref airapi-commandid-event-input "Key Input" to see
 *         related commands.
 *  @{
 */
/*! @enum AM_EVENT_KEY_INPUT_CONFIG
 *
 */
enum AM_EVENT_KEY_INPUT_CONFIG_BITS
{
  /*!
   * Config key input plug-in long_press_time
   */
  AM_EVENT_KEY_INPUT_CONFIG_LPT = 0,
};

/*! @struct am_event_key_input_config_s
 *  Key input plug-in config data structure
 *
 *  @sa AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET
 *  @sa AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET
 */
struct am_event_key_input_config_s
{
    /*!
     * @sa AM_EVENT_KEY_INPUT_CONFIG_BITS
     */
    uint32_t enable_bits;

    /*!
     * Defines key long-press event interval.
     * To make a "key long-press" event,
     * key must be pressed at least the time length defined here.
     */
    uint32_t long_press_time;
};
/*! @} */

/*! @defgroup airapi-datastructure-event-audio-alert Audio Alert
 *  @ingroup airapi-datastructure-event
 *  @brief Audio Alert related data structure,
 *         refer to @ref airapi-commandid-event-audio-alert "Audio Alert" to
 *         see related commands.
 *  @{
 */
/*! @enum AM_EVENT_AUDIO_ALERT_CONFIG_BITS
 */
enum AM_EVENT_AUDIO_ALERT_CONFIG_BITS
{
  /*!
   * Config channel number
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_CHANNEL_NUM = 0,

  /*!
   * Config sample rate
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_RATE = 1,

  /*!
   * Config chunk bytes
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_CHUNK_BYTES = 2,

  /*!
   * Config sample format
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_FORMAT = 3,

  /*!
   * Config enable
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_ENABLE = 4,

  /*!
   * Config sensitivity
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_SENSITIVITY = 5,

  /*!
   * Config threshold
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_THRESHOLD = 6,

  /*!
   * Config direction
   */
  AM_EVENT_AUDIO_ALERT_CONFIG_DIRECTION = 7,
};

/*! @struct am_event_audio_alert_config_s
 *  Defines audio alert plug-in config
 *
 *  @sa AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET
 *  @sa AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET
 */
struct am_event_audio_alert_config_s
{
    /*!
     * @sa AM_EVENT_AUDIO_ALERT_CONFIG_BITS
     */
    uint32_t enable_bits;

    /*!
     * Audio channel number
     * - 1: mono
     * - 2: strereo
     */
    uint32_t channel_num;

    /*!
     * Audio sample rate
     * - 48000: default
     * - 32000
     * - 16000
     * - 8000
     */
    uint32_t sample_rate;

    /*!
     * Audio chunk bytes
     */
    uint32_t chunk_bytes;

    /*!
     * Audio sample format
     * @sa AM_AUDIO_SAMPLE_FORMAT
     */
    uint32_t sample_format;

    /*!
     * Audio alert sensitivity
     */
    uint32_t sensitivity;

    /*!
     * Audio alert threshold
     * - 0: minimum
     * - 100: maximum
     */
    uint32_t threshold;

    /*!
     * Audio alert direction
     * - 0: Alert when audio becomes louder
     * - 1: Alert when audio becomes quite
     */
    uint32_t direction;

    /*!
     * - true: enable audio alert
     * - false: disable audio alert
     */
    bool enable;
};
/*! @} */

/*!
 * @}
 */

#endif /*_AM_API_EVENT_H_*/
