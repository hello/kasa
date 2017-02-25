/*******************************************************************************
 * am_api_cmd_network.h
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
