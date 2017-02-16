/*******************************************************************************
 * am_service_impl.h
 *
 * History:
 *   2014-9-15 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

/*! @file am_service_impl.h
 *  @brief This file defines service names and generic data structures
 */

/*! @defgroup airapi-commandid Air API Command IDs
 *  @ingroup airapi
 *  @brief All the Air API Command IDs, see @ref airapi-datastructure
 *  "Air API Data Structure" to find related data structures,
 *  see @ref AMAPIHelper to learn how to use these commands
 *  @sa AMAPIHelper
 */

#ifndef AM_SERVICE_IMPL_H_
#define AM_SERVICE_IMPL_H_

#include "am_ipc_sync_cmd.h"

/*! @enum AM_SERVICE_CMD_START_NUM
 *  @brief Defines service start number
 */
enum AM_SERVICE_CMD_START_NUM
{
  /*!
   * enumeration value of the first VIDEO service CMD
   */
  VIDEO_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START),

  /*!
   * enumeration value of the first SYSTEM service CMD
   */
  SYSTEM_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 100),

  /*!
   * enumeration value of the first IMAGE service CMD
   */
  IMAGE_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 200),

  /*!
   * enumeration value of the first AUDIO service CMD
   */
  AUDIO_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 300),

  /*!
   * enumeration value of the first EVENT service CMD
   */
  EVENT_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 400),

  /*!
   * enumeration value of the first MEDIA service CMD
   */
  MEDIA_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 500),

  /*!
   * enumeration value of the first NETWORK service CMD
   */
  NETWORK_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 600),

  /*!
   * enumeration value of the first SIP service CMD
   */
  SIP_SERVICE_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 700),

  /*!
   * enumeration value of the first service common CMD
   */

  SERVICE_COMMON_CMD_START = (AM_IPC_CMD_FOR_MW_API_START + 800),
};

/*! @enum AM_SERVICE_CMD_TYPE
 *  @brief Define service type.
 *  all of the services should be defined here as enumeration
 */
enum AM_SERVICE_CMD_TYPE
{
  /*!
   * generic will set to 0, so it's same as type not set
   */
  AM_SERVICE_TYPE_GENERIC             = 0,
  AM_SERVICE_TYPE_API_PROXY_SERVER    = 1, //!< API proxy server Type
  AM_SERVICE_TYPE_AUDIO               = 2, //!< Audio Service Type
  AM_SERVICE_TYPE_EVENT               = 3, //!< Event Service Type
  AM_SERVICE_TYPE_IMAGE               = 4, //!< Image Service Type
  AM_SERVICE_TYPE_MEDIA               = 5, //!< Media Service Type
  AM_SERVICE_TYPE_NETWORK             = 6, //!< Network Service Type
  AM_SERVICE_TYPE_SYSTEM              = 7, //!< System Service Type
  AM_SERVICE_TYPE_VIDEO               = 8, //!< Video Service Type
  AM_SERVICE_TYPE_RTSP                = 9, //!< RTSP Service Type
  AM_SERVICE_TYPE_SIP                 = 10, //!< SIP Service Type
  AM_SERVICE_TYPE_OTHERS              = 11, //!< Other Service Type
};

/*!
 * Service name maximum length
 */
#define AM_SERVICE_NAME_MAX_LENGTH   32

/*! API Proxy Process Name */
#define AM_SERVICE_API_PROXY_NAME "api_proxy"

/*! Audio Service Process Name */
#define AM_SERVICE_AUDIO_NAME     "audio_svc"

/*! Event Service Process Name */
#define AM_SERVICE_EVENT_NAME     "event_svc"

/*! Image Service Process Name */
#define AM_SERVICE_IMAGE_NAME     "img_svc"

/*! Network Service Process Name */
#define AM_SERVICE_NETWORK_NAME   "net_svc"

/*! Media Service Process Name */
#define AM_SERVICE_MEDIA_NAME     "media_svc"

/*! System Service Process Name */
#define AM_SERVICE_SYSTEM_NAME    "sys_svc"

/*! Video Service Process Name */
#define AM_SERVICE_VIDEO_NAME     "video_svc"

/*! RTSP Service Process Name */
#define AM_SERVICE_RTSP_NAME      "rtsp_svc"

/*! SIP Service Process Name */
#define AM_SERVICE_SIP_NAME       "sip_svc"

/*! Audio Service IPC Name */
#define AM_IPC_AUDIO_NAME          "/" AM_SERVICE_AUDIO_NAME

/*! Event Service IPC Name */
#define AM_IPC_EVENT_NAME          "/" AM_SERVICE_EVENT_NAME

/*! Image Service IPC Name */
#define AM_IPC_IMAGE_NAME          "/" AM_SERVICE_IMAGE_NAME

/*! Network Service IPC Name */
#define AM_IPC_NETWORK_NAME        "/" AM_SERVICE_NETWORK_NAME

/*! Media Service IPC Name */
#define AM_IPC_MEDIA_NAME          "/" AM_SERVICE_MEDIA_NAME

/*! System Service IPC Name */
#define AM_IPC_SYSTEM_NAME         "/" AM_SERVICE_SYSTEM_NAME

/*! Video Service IPC Name */
#define AM_IPC_VIDEO_NAME          "/" AM_SERVICE_VIDEO_NAME

/*! RTSP Service IPC Name */
#define AM_IPC_RTSP_NAME           "/" AM_SERVICE_RTSP_NAME

/*! SIP Service IPC Name */
#define AM_IPC_SIP_NAME            "/" AM_SERVICE_SIP_NAME

/*! @struct am_service_attribute
 *  @brief Defines service basic attributes
 */
struct am_service_attribute
{
    char name[AM_SERVICE_NAME_MAX_LENGTH]; //!< Service Name
    AM_SERVICE_CMD_TYPE type;              //!< Service Type
    bool enable;                           //!< Is Enabled

    /*! Indicates if this service handle button event */
    bool handle_event_button;

    /*! Indicates if this service handle motion event */
    bool handle_event_motion;

    /*! Indicates if this service handle audio event */
    bool handle_event_audio;
};

/*! @enum AM_SERVICE_STATE
 *  @brief Defines service status
 */
enum AM_SERVICE_STATE
{
  //!Instance created but not init
  AM_SERVICE_STATE_NOT_INIT = -1,

  //!Service process created
  AM_SERVICE_STATE_INIT_PROCESS_CREATED = 0,

  //!IPC setup for service management
  AM_SERVICE_STATE_INIT_IPC_CONNECTED = 1,

  //!Init function done
  AM_SERVICE_STATE_INIT_DONE = 2,

  //!Started and running
  AM_SERVICE_STATE_STARTED = 3,

  //!Stopped running
  AM_SERVICE_STATE_STOPPED = 4,

  //!Transition state, during this state, does not accept more cmd
  AM_SERVICE_STATE_BUSY = 5,

  //!Error state, used for debugging
  AM_SERVICE_STATE_ERROR = 6,
};

/*! @enum AM_SYS_IPC_USER_CMD
 *  @brief use for system IPC basic function, not product related
 */
enum AM_SYS_IPC_USER_CMD
{
  _AM_IPC_SERVICE_INIT = AM_IPC_CMD_FOR_USER_START, //!< Service Init
  _AM_IPC_SERVICE_DESTROY,         //!< Service Destroy
  _AM_IPC_SERVICE_START,           //!< Service Start
  _AM_IPC_SERVICE_STOP,            //!< Service Stop
  _AM_IPC_SERVICE_RESTART,         //!< Service Restart
  _AM_IPC_SERVICE_STATUS,          //!< Service Status
  _AM_IPC_SERVICE_NOTIF,           //!< UP notification of actual service
  _AM_IPC_MW_CMD_AIR_API_CONTAINER, //!< forward the CMD as API proxy
};

#ifndef SERVICE_RESULT_DATA_MAX_LEN
/*!
 * Service result data max size
 */
#define SERVICE_RESULT_DATA_MAX_LEN 256
#endif

/*! @struct am_service_result_t
 *  @brief generic service result
 */
struct am_service_result_t
{
    /*!
     * method_call ret value, success or failure
     */
    int32_t ret;
    /*!
     * report service current state
     */
    int32_t state;
    /*!
     * data if used
     */
    uint8_t data[SERVICE_RESULT_DATA_MAX_LEN];
};

//! Service Init Message ID
#define AM_IPC_SERVICE_INIT                \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_INIT,     \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NEED_RETURN)

//! Service Destroy Message ID
#define AM_IPC_SERVICE_DESTROY             \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_DESTROY,  \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NO_NEED_RETURN)

//! Service Start Message ID
#define AM_IPC_SERVICE_START               \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_START,    \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NEED_RETURN)

//! Service Stop Message ID
#define AM_IPC_SERVICE_STOP                \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_STOP,     \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NEED_RETURN)

//! Service Restart Message ID
#define AM_IPC_SERVICE_RESTART             \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_RESTART,  \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NEED_RETURN)

//! Service Status Message ID
#define AM_IPC_SERVICE_STATUS              \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_STATUS,   \
                 AM_IPC_DIRECTION_DOWN,    \
                 AM_IPC_NEED_RETURN)

//! Service Notify Message ID
#define AM_IPC_SERVICE_NOTIF               \
BUILD_IPC_MSG_ID(_AM_IPC_SERVICE_NOTIF,    \
                 AM_IPC_DIRECTION_UP,      \
                 AM_IPC_NO_NEED_RETURN)

/*! @enum AM_SERVICE_NOTIFY_RESULT
 *  @brief This enumerate defines service notify type.
 */
enum AM_SERVICE_NOTIFY_RESULT
{
  /*!
   * Indicate a failure on some reason
   */
  AM_SERVICE_NOTIFY_FAILED =  -1,
  /*!
   * notify the service state has changed,
   * the app may call API to know what state it is
   */
  AM_SERVICE_NOTIFY_STATE_CHANGED = 1,
};

/*!
 * @struct am_service_notify_payload
 * @brief Service notify payload data structure.
 */
struct am_service_notify_payload
{
    AM_SERVICE_NOTIFY_RESULT result; //!< The service notify result
    /*! Destination service bitmap
     *
     * The bit offset is defined by @ref AM_SERVICE_CMD_TYPE,
     * 0 means broadcast
     */
    uint32_t dest_bits;

    /*!
     * IPC command id,
     * defined in @ref AM_SYS_IPC_MW_CMD_COMMON
     */
    uint32_t msg_id;
    uint32_t data_size; //!< Message data size
    uint8_t data[AM_MAX_IPC_MESSAGE_SIZE]; //!< Message data buffer

    /*!
     * Get struct header size, except message data size
     * @return header size
     */
    uint32_t header_size() {
      return offsetof(am_service_notify_payload, data);
    }
};

//! Service CMD forwarding message ID
#define AM_IPC_MW_CMD_AIR_API_CONTAINER            \
BUILD_IPC_MSG_ID(_AM_IPC_MW_CMD_AIR_API_CONTAINER, \
                 AM_IPC_DIRECTION_DOWN,            \
                 AM_IPC_NEED_RETURN )

#endif /* AM_SERVICE_IMPL_H_ */
