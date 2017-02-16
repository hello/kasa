/*******************************************************************************
 * am_event_types.h
 *
 * Histroy:
 *  2014-11-19  [Dongge Wu] created file
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

/*! @file am_event_types.h
 *  @brief This file defines Event Service related data structure and enumeration
 */
#ifndef AM_EVENT_TYPES_H_
#define AM_EVENT_TYPES_H_

#include <mutex>

#ifndef AUTO_LOCK
#define AUTO_LOCK(mtx) std::lock_guard<std::mutex> lck (mtx)
#endif

/*! @enum EVENT_MODULE_ID
 * Defines all event modules ID
 */
enum EVENT_MODULE_ID
{
  EV_MOTION_DECT = 0,    //!< Plugin ID of motion detect
  EV_AUDIO_ALERT_DECT,   //!< Plugin ID of motion detect
  EV_AUDIO_ANALYSIS_DECT,//!< Plugin ID of audio analysis
  EV_FACE_DECT,          //!< Plugin ID of face detection
  EV_KEY_INPUT_DECT,     //!< Plugin ID of key press detection
  EV_ALL_MODULE_NUM,     //!< Total number of Plugins
};

/*! @struct EVENT_MODULE_CONFIG
 *  Defines event module configure.
 */
struct EVENT_MODULE_CONFIG
{
    uint32_t key;//!< specifying which configure item it is.
    void *value; //!< specifying the value of configure item.
};

/*! Audio Alert max threshold  */
#define AM_AUDIO_ALERT_MAX_THRESHOLD   (100)

/*! Audio Alert max sensitivity */
#define AM_AUDIO_ALERT_MAX_SENSITIVITY (100)

/*! @enum AM_AUDIO_ALERT_KEY
 * Defines configure item of audio alert event module.
 */
enum AM_AUDIO_ALERT_KEY
{
  //! Channel Number. 1:mono, 2:stereo
  AM_CHANNEL_NUM = 0,

  //! Sample Rate. 8K, 16K, 32K, 48K
  AM_SAMPLE_RATE,

  //! Chunk Bytes. 1024, 2048
  AM_CHUNK_BYTES,

  //! Sample Format. PCM audio format
  AM_SAMPLE_FORMAT,

  //! Audio Alert Enable. 0: disable, 1: enable
  AM_ALERT_ENABLE,

  //! Audio Alert Callback function register.
  AM_ALERT_CALLBACK,

  //! Audio Alert Sensitivity. 0-100
  AM_ALERT_SENSITIVITY,

  //! Audio Alert Threshold. 0-100
  AM_ALERT_THRESHOLD,

  //! Audio Alert Direction. 0: rise edge, 1: down edge
  AM_ALERT_DIRECTION,

  //! Audio Alert sync up configure item to configure file.
  AM_ALERT_SYNC_CONFIG,

  //! Audio Alert Key number.
  AM_AUDIO_ALERT_KEY_NUM,
};

/*! @enum AM_KEY_STATE
 *  Defines key input states
 */
enum AM_KEY_STATE
{
  AM_KEY_UP = 0X01,   //!< key up.
  AM_KEY_DOWN,        //!< key down.
  AM_KEY_CLICKED,     //!< key clicked.
  AM_KEY_LONG_PREESED,//!< key long time press.
  AM_KEY_STATE_NUM,   //!< total number of key states.
};

/*! @typedef AM_KEY_CODE
 *  @brief Defines key ID
 */
typedef uint32_t AM_KEY_CODE;

/*! @struct AM_KEY_INPUT_EVENT
 */
struct AM_KEY_INPUT_EVENT
{
    AM_KEY_CODE key_value; //!< key ID
    AM_KEY_STATE key_state;//!< key state
};

/*! @enum AM_KEY_INPUT_KEY
 *  Defines key input event module configure item
 */
enum AM_KEY_INPUT_KEY
{
  AM_GET_KEY_STATE = 0, //!< Just for get, value : AM_KEY_INPUT_EVENT
  AM_WAIT_KEY_PRESSED,  //!< Just for get, value: AM_KEY_INPUT_EVENT
  AM_KEY_CALLBACK,      //!< Just for set, value: AM_KEY_INPUT_CALLBACK
  AM_LONG_PRESSED_TIME, //!< set/get, long press time(milliseconds)
  AM_KEY_INPUT_SYNC_CONFIG,//!< sync configure item and save to configure file
  AM_KEY_INPUT_KEY_NUM,    //!< total number of key input configure items
};

/*! @enum AM_MOTION_DETECT_KEY
 *  Defines motion detect event module configure item
 */
enum AM_MOTION_DETECT_KEY
{
  AM_MD_ENABLE = 0,        //!< motion detect enable.
  AM_MD_BUFFER_ID,         //!< ME1 data buffer ID.
  AM_MD_ROI,               //!< region of interest.
  AM_MD_THRESHOLD,         //!< threshold.
  AM_MD_LEVEL_CHANGE_DELAY,//!< motion level change delay.
  AM_MD_CALLBACK,          //!< callback function.
  AM_MD_SYNC_CONFIG,       //!< sync configure item and save to configure file.
  AM_MD_KEY_NUM            //!< total number of configure items
};

/*! @enum AM_MOTION_LEVEL
 *  Defines motion level
 */
enum AM_MOTION_LEVEL
{
  AM_MOTION_L0 = 0,//!< motion level 0, smallest motion
  AM_MOTION_L1,    //!< motion level 1
  AM_MOTION_L2,    //!< motion level 2, biggest motion
  AM_MOTION_L_NUM  //!< total number of motion level
};

/*! @enum AM_MOTION_TYPE
 *  Defines motion event types
 */
enum AM_MOTION_TYPE
{
  AM_MD_MOTION_NONE = 0,     //!< No motion
  AM_MD_MOTION_START,        //!< motion start
  AM_MD_MOTION_LEVEL_CHANGED,//!< motion level changed
  AM_MD_MOTION_END,          //!< motion end
  AM_MD_MOTION_TYPE_NUM,     //!< total number of motion event type
};

/*! @struct AM_EVENT_MD_ROI
 *  Defines motion detect event module ROI
 */
struct AM_EVENT_MD_ROI
{
    uint32_t roi_id;//!< ROI ID
    uint32_t left;  //!< X offset of left edge
    uint32_t right; //!< X offset of right edge
    uint32_t top;   //!< Y offset of top edge
    uint32_t bottom;//!< Y offset of bottom edge

    /*!
     * - true: Valid ROI
     * - false: Invalid ROI
     */
    bool valid;
};

/*! @struct AM_EVENT_MD_THRESHOLD
 *  Defines ROI's threshold of motion detect event module
 */
struct AM_EVENT_MD_THRESHOLD
{
    uint32_t roi_id;                        //!< ROI ID
    uint32_t threshold[AM_MOTION_L_NUM - 1];//!< threshold of this ROI
};

/*! @struct AM_EVENT_MD_LEVEL_CHANGE_DELAY
 *  Defines how many counts to confirm motion level changed
 */
struct AM_EVENT_MD_LEVEL_CHANGE_DELAY
{
    uint32_t roi_id;                                    //!< ROI ID
    uint32_t mt_level_change_delay[AM_MOTION_L_NUM - 1];//!< motion level change delay
};

/*! @struct AM_MD_MESSAGE
 *  Defines motion detect event message
 */
struct AM_MD_MESSAGE
{
    uint32_t roi_id;         //!< ROI ID
    AM_MOTION_TYPE mt_type;  //!< Motion type. @sa AM_MOTION_TYPE
    AM_MOTION_LEVEL mt_level;//!< motion level. @sa AM_MOTION_LEVEL
    uint32_t mt_value;       //!< motion value
};

/*! @struct AM_EVENT_MESSAGE
 *  Defines event message for all event modules
 */
struct AM_EVENT_MESSAGE
{
    /*!
     * event type.
     * @sa EVENT_MODULE_ID
     */
    EVENT_MODULE_ID event_type;

    /*!
     * sequence number of events
     */
    uint64_t seq_num;

    /*!
     * PTS of this event
     */
    uint64_t pts;

    union
    {
        /*!
         * event message of motion detect event module.
         * @sa AM_MD_MESSAGE
         */
        AM_MD_MESSAGE md_msg;
        /*!
         * event message of key input event module.
         * @sa AM_KEY_INPUT_EVENT
         */
        AM_KEY_INPUT_EVENT key_event;
    };
};

/*! @typedef AM_EVENT_CALLBACK
 *  @brief Event callback function type
 *  @param event_msg @ref AM_EVENT_MESSAGE "event message data"
 *  @return int32_t
 */
typedef int32_t (*AM_EVENT_CALLBACK)(AM_EVENT_MESSAGE *event_msg);

/*! @struct AM_KEY_INPUT_CALLBACK
 *  Defines key input callback
 */
struct AM_KEY_INPUT_CALLBACK
{
    /*!
     * key ID.
     * @sa AM_KEY_CODE
     */
    AM_KEY_CODE key_value;

    /*!
     * callback.
     * @sa AM_EVENT_CALLBACK
     */
    AM_EVENT_CALLBACK callback;
};

#endif
