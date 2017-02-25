/*
 * am_api_cmd_common.h
 *
 *  History:
 *    May 13, 2015 - [Shupeng Ren] created file
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
/*! @file am_api_cmd_common.h
 *  @brief This file defines commands can used by all services
 */
#ifndef ORYX_INCLUDE_SERVICES_COMMANDS_AM_API_CMD_COMMON_H_
#define ORYX_INCLUDE_SERVICES_COMMANDS_AM_API_CMD_COMMON_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_COMMON
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_COMMON
{
  //! _AM_IPC_MW_CMD_COMMON_GET_EVENT
  _AM_IPC_MW_CMD_COMMON_GET_EVENT = SERVICE_COMMON_CMD_START,

  //! _AM_IPC_MW_CMD_VIDEO_EDIT_FINISH_TO_APP
  _AM_IPC_MW_CMD_VIDEO_EDIT_FINISH_TO_APP,

  //! _AM_IPC_MW_CMD_COMMON_START_EVENT_MODULE
  _AM_IPC_MW_CMD_COMMON_START_EVENT_MODULE,

  //! _AM_IPC_MW_CMD_COMMON_STOP_EVENT_MODULE
  _AM_IPC_MW_CMD_COMMON_STOP_EVENT_MODULE,
  //!_AM_IPC_MW_CMD_MEDIA_FILE_OPERATION_TO_APP
  _AM_IPC_MW_CMD_MEDIA_FILE_OPERATION_TO_APP,

};

/*********************************Common CMDS**********************************/
/*! @defgroup airapi-commandid-common Air API Command IDs - Common Commands
 *  @ingroup airapi-commandid
 *  @brief Commonly used command IDs
 *  @{
 */

/*! @defgroup airapi-commandid-common-evt-get Event Get Commands
 *  @ingroup airapi-commandid-common
 *  @brief Event related commands,
 *  see @ref airapi-commandid-event "Air API Command IDs - Event Service" for
 *  Event Service related command IDs
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Event Get*/

/*!
 * @brief Get event from event service [INTERNAL]
 *
 * Other services can use this command to get event notification from
 * event service. This command is used in service implementation internally.
 * Do NOT call this in client applications.
 */
#define AM_IPC_MW_CMD_COMMON_GET_EVENT                                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_COMMON_GET_EVENT,                    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Event Get Commands*/

/*! @defgroup airapi-commandid-common-switch-event-state Event State Commands
 *  @ingroup airapi-commandid-common
 *  @brief Event related commands,
 *  see @ref airapi-commandid-event "Air API Command IDs - Event Service" for
 *  Event Service related command IDs
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Event State*/

/*!
 * @brief Start event from other service [INTERNAL]
 *
 * Other services can use this command to start event module state.
 * This command is used in service implementation internally.
 * Do NOT call this in client applications.
 */
#define AM_IPC_MW_CMD_COMMON_START_EVENT_MODULE                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_COMMON_START_EVENT_MODULE,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_EVENT)

/*!
 * @brief Stop event from other service [INTERNAL]
 *
 * Other services can use this command to stop event module state.
 * This command is used in service implementation internally.
 * Do NOT call this in client applications.
 */
#define AM_IPC_MW_CMD_COMMON_STOP_EVENT_MODULE                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_COMMON_STOP_EVENT_MODULE,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_EVENT)
/*! @} */ /* End of Event State Commands*/

/*!
 * @brief Post video edit finish event to Application
 *
 * Application should receive this event, it means video edit (re-encode) is finished
 */
#define AM_IPC_MW_CMD_VIDEO_EDIT_FINISH_TO_APP                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_EDIT_FINISH_TO_APP,            \
                           AM_IPC_DIRECTION_UP,                                \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_GENERIC)

/*!
 * @brief Post media file operation msg to Application
 */
#define AM_IPC_MW_CMD_MEDIA_FILE_OPERATION_TO_APP                                 \
    BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_FILE_OPERATION_TO_APP,        \
                               AM_IPC_DIRECTION_UP,                            \
                               AM_IPC_NO_NEED_RETURN,                          \
                               AM_SERVICE_TYPE_GENERIC)

/*!
 *  @}
 */ /* End of Service Common Command IDs */

#endif /* ORYX_INCLUDE_SERVICES_COMMANDS_AM_API_CMD_COMMON_H_ */
