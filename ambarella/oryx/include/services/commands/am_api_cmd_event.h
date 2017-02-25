/*******************************************************************************
 * am_api_cmd_event.h
 *
 * History:
 *   2015-4-15 - [ypchang] created file
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
/*! @file am_api_cmd_event.h
 *  @brief This file defines Event Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_EVENT_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_EVENT_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_EVENT
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_EVENT
{
  //! _AM_IPC_MW_CMD_EVENT_SERVICE_PAUSE
  _AM_IPC_MW_CMD_EVENT_SERVICE_PAUSE = EVENT_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_RESUME
  _AM_IPC_MW_CMD_EVENT_SERVICE_RESUME,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_GET
  _AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_GET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_GET
  _AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_GET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_SET
  _AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_SET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_SET
  _AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_SET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_REGISTER_MODULE_SET
  _AM_IPC_MW_CMD_EVENT_SERVICE_REGISTER_MODULE_SET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_CHECK_MODULE_REGISTER_GET
  _AM_IPC_MW_CMD_EVENT_SERVICE_CHECK_MODULE_REGISTER_GET,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_START_ALL_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_START_ALL_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_STOP_ALL_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_STOP_ALL_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_ALL_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_ALL_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_START_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_START_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_STOP_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_STOP_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_EVENT_MODULE
  _AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_EVENT_MODULE,
};

/*****************************Event Service CMDs*******************************/
/*! @defgroup airapi-commandid-event Air API Command IDs - Event Service
 *  @ingroup airapi-commandid
 *  @brief Event Service Related command IDs,
 *         refer to @ref airapi-datastructure-event
 *         "Data Structure of Event Service" to see data structures
 *  @{
 */

/*! @defgroup airapi-commandid-event-plugin Plug-in Control
 *  @ingroup airapi-commandid-event
 *  @brief Event Plug-in Control realted command IDs,
 *         refer to @ref airapi-datastructure-event-plugin
 *         "Event Plug-in Control" to see data structures
 *  @{
 */
/*! @brief Register Event Module
 *
 * Use this command to register an event module.
 * @sa AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET
 * @sa am_event_module_id
 * @sa am_event_module_register_state
 */
#define AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_REGISTER_MODULE_SET,   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Check Event Module Register
 *
 * Use this command to check whether an event module is registered or not.
 * @sa AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET
 * @sa am_event_module_id
 * @sa am_event_module_register_state
 */
#define AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET                          \
BUILD_IPC_MSG_ID_WITH_TYPE(                                                    \
                       _AM_IPC_MW_CMD_EVENT_SERVICE_CHECK_MODULE_REGISTER_GET, \
                       AM_IPC_DIRECTION_DOWN,                                  \
                       AM_IPC_NEED_RETURN,                                     \
                       AM_SERVICE_TYPE_EVENT)

/*! @brief Start All Event Modules
 *
 * Use this command to start all event modules which have been registered.
 * @sa AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_START_ALL_MODULE                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_START_ALL_EVENT_MODULE,\
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Stop All Event Modules
 *
 * Use this command to stop all event modules which have been registered.
 * @sa AM_IPC_MW_CMD_EVENT_START_ALL_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_STOP_ALL_EVENT_MODULE, \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Destroy All Event Modules
 *
 * Use this command to destroy all event modules which have been registered.
 * @sa AM_IPC_MW_CMD_EVENT_START_ALL_MODULE
 * @sa AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_DESTROY_ALL_MODULE                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(                                                    \
                       _AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_ALL_EVENT_MODULE,  \
                       AM_IPC_DIRECTION_DOWN,                                  \
                       AM_IPC_NEED_RETURN,                                     \
                       AM_SERVICE_TYPE_EVENT)

/*! @brief Start a Specified Event Module
 *
 * Use this command to start a specified event module which has been registered.
 * @sa AM_IPC_MW_CMD_EVENT_STOP_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_START_MODULE                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_START_EVENT_MODULE,    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Stop a Specified Event Module
 *
 * Use this command to stop a specified event module which has been registered.
 * @sa AM_IPC_MW_CMD_EVENT_START_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_STOP_MODULE                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_STOP_EVENT_MODULE,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Destroy a Specified Event Module
 *
 * Use this command to destroy a specified event module
 * which has been registered.
 * @sa AM_IPC_MW_CMD_EVENT_START_MODULE
 * @sa AM_IPC_MW_CMD_EVENT_STOP_MODULE
 * @sa am_event_module_id
 */
#define AM_IPC_MW_CMD_EVENT_DESTROY_MODULE                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_DESTROY_EVENT_MODULE,  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)
/*! @} */

/*! @defgroup airapi-commandid-event-audio-alert Audio Alert
 *  @ingroup airapi-commandid-event
 *  @brief Audio Alert realted command IDs,
 *         refer to @ref airapi-datastructure-event-audio-alert
 *         "Audio Alert" to see data structures
 *  @{
 */
/*! @brief Set Configure of Audio Alert Event Module
 *
 * Use this command to set a configure item of audio alert module
 * @sa AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET
 * @sa AM_EVENT_AUDIO_ALERT_CONFIG_BITS
 */
#define AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_SET,\
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Get Configure of Audio Alert Event Module
 *
 * Use this command to get a configure item of audio alert module
 * @sa AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET
 * @sa AM_EVENT_AUDIO_ALERT_CONFIG_BITS
 */
#define AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_AUDIO_ALERT_CONFIG_GET,\
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)
/*! @} */

/*! @defgroup airapi-commandid-event-input Key Input
 *  @ingroup airapi-commandid-event
 *  @brief Key Input realted command IDs,
 *         refer to @ref airapi-datastructure-event-input
 *         "Key Input" to see data structures
 *  @{
 */
/*! @brief Set Configure of Key Input Event Module
 *
 * Use this command to set a configure item of key input module
 * @sa AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET
 * @sa AM_EVENT_KEY_INPUT_CONFIG_BITS
 */
#define AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_SET,  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)

/*! @brief Get Configure of Key Input Event Module
 *
 * Use this command to get a configure item of key input module
 * @sa AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET
 * @sa AM_EVENT_KEY_INPUT_CONFIG_BITS
 */
#define AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_EVENT_SERVICE_KEY_INPUT_CONFIG_GET,  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EVENT)
/*! @} */

/*!
 * @}
 */
/******************************************************************************/

/*! @example test_event_service.cpp
 *  This is the example program of Event Service APIs.
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_EVENT_H_ */
