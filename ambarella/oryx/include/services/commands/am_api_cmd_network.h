/*******************************************************************************
 * am_api_cmd_network.h
 *
 * History:
 *   2015-4-15 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
/*! @file am_api_cmd_network.h
 *  @brief This file defines Network Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_NETWORK_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_NETWORK_H_

#include "../commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_NETWORK
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_NETWORK
{
  //! _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_GET
  _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_GET = NETWORK_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_SET
  _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_SET,

  //! _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_CLEAR
  _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_CONFIG_CLEAR,

  //! _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_RESET_DEVICE
  _AM_IPC_MW_CMD_NETWORK_CONTROL_WIFI_RESET_DEVICE,
};

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_NETWORK_H_ */
