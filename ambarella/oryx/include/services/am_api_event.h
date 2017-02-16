/**
 * am_api_event.h
 *
 *  History:
 *    Nov 18, 2014 - [Dongge Wu] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
  MOTION_DECT = 0,    //!< Plugin ID of motion detect
  AUDIO_ALERT_DECT,   //!< Plugin ID of audio alert
  AUDIO_ANALYSIS_DECT,//!< Plugin ID of audio analysis
  FACE_DECT,          //!< Plugin ID of face detection
  KEY_INPUT_DECT,     //!< Plugin ID of key press detection
  ALL_MODULE_NUM,     //!< Total number of Plugins
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
     * Module ID
     * @sa am_event_module_id
     */
    uint32_t id;

    /*!
     * Event plugin's full path
     */
    char *path_name;
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

/*! @defgroup airapi-datastructure-event-md Motion Detection
 *  @ingroup airapi-datastructure-event
 *  @brief Motion Detection related data structure,
 *         refer to @ref airapi-commandid-event-md "Motion Detection" to see
 *         related commands.
 *  @{
 */

/*! @enum Defines motion detection ROI maximum number
 *
 *  ROI: Region Of Interest\n
 *  Only motions happend in ROI will trigger motion event.
 */
enum AM_MD_ROI_ID
{
  MD_ROI_ID_0 = 0, //!< ROI region 0
  MD_ROI_ID_1 = 1, //!< ROI region 1
  MD_ROI_ID_2 = 2, //!< ROI region 2
  MD_ROI_ID_3 = 3, //!< ROI region 3
  MD_MAX_ROI,      //!< ROI region number
};

/*! @enum Defines motion level
 *  Currently there are 3 levels of motion:\n
 *  - 0: Means no motion
 *  - 1: Small motion
 *  - 2: Big motion
 */
enum AM_MOTION_LEVEL_NUMBER
{
  MOTION_LEVEL_0 = 0, //!< No motion
  MOTION_LEVEL_1 = 1, //!< Small motion
  MOTION_LEVEL_2 = 2, //!< Big motion
  MOTION_LEVEL_NUM    //!< Motion level number
};

/*! @struct am_event_md_threshold_s
 *  This structure describes motion detection related info
 */
struct am_event_md_threshold_s
{
    /*!
     * @sa AM_MD_ROI_ID
     */
    uint32_t roi_id;

    /*!
     * Threshold of motion events
     */
    uint32_t threshold[MOTION_LEVEL_NUM - 1];
};

/*! @struct am_event_md_level_change_delay_s
 *  This structure describes motion level change number
 *
 *  We must make sure that the motion change is surely happened, if a motion
 *  level change happened a number of time described in this data structure,
 *  we can say that the motion level has changed.
 */
struct am_event_md_level_change_delay_s
{
    /*!
     * @sa AM_MD_ROI_ID
     */
    uint32_t roi_id;

    /*!
     *  Motion level change times
     */
    uint32_t lc_delay[MOTION_LEVEL_NUM - 1];
};

/*! @enum AM_EVENT_MD_CONFIG_BITS
 *
 */
enum AM_EVENT_MD_CONFIG_BITS
{
  /*! Config enabled */
  AM_EVENT_MD_CONFIG_ENABLE = 0,

  /*! Config @ref am_event_md_threshold_s */
  AM_EVENT_MD_CONFIG_THRESHOLD0 = 1,

  /*! Config @ref am_event_md_threshold_s */
  AM_EVENT_MD_CONFIG_THRESHOLD1 = 2,

  /*! Config @ref am_event_md_level_change_delay_s */
  AM_EVENT_MD_CONFIG_LEVEL0_CHANGE_DELAY = 3,

  /*! Config @ref am_event_md_level_change_delay_s */
  AM_EVENT_MD_CONFIG_LEVEL1_CHANGE_DELAY = 4,
};

/*! @struct am_event_md_config_s
 *  This data structure defines motion detection related config
 *
 *  @sa AM_IPC_MW_CMD_EVENT_MOTION_DETECT_CONFIG_SET
 *  @sa AM_IPC_MW_CMD_EVENT_MOTION_DETECT_CONFIG_GET
 */
struct am_event_md_config_s
{
    /*!
     * @sa AM_EVENT_MD_CONFIG_BITS
     */
    uint32_t enable_bits;

    /*!
     * - true: enable motion detection
     * - false: disable motion detection
     */
    bool enable;

    /*!
     * @sa am_event_md_threshold_s
     */
    am_event_md_threshold_s threshold;

    /*!
     * @sa am_event_md_level_change_delay_s
     */
    am_event_md_level_change_delay_s level_change_delay;
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
     * - true: enable audio alert
     * - false: disable audio alert
     */
    bool enable;

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
};
/*! @} */

/*!
 * @}
 */

#endif /*_AM_API_EVENT_H_*/
