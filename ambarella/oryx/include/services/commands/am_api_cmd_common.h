/*
 * am_api_cmd_common.h
 *
 *  History:
 *    May 13, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

};

/******************************Common CMDS******************************/
/*! @defgroup airapi-commandid-common Air API Command IDs
 *  @ingroup airapi-commandid
 *  @brief Common used command IDs,
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
 * @brief Get event
 *
 * use this command to get event
 */
#define AM_IPC_MW_CMD_COMMON_GET_EVENT                                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_COMMON_GET_EVENT,                    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Event Get Commands*/


/*!
 *  @}
 */ /* End of Service Common Command IDs */

#endif /* ORYX_INCLUDE_SERVICES_COMMANDS_AM_API_CMD_COMMON_H_ */
