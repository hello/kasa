/*******************************************************************************
 * am_api_cmd_sip.h
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
/*! @file am_api_cmd_sip.h
 *  @brief This file defines SIP Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_SIP_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_SIP_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_SIP
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_SIP
{
  //! _AM_IPC_MW_CMD_SET_SIP_REGISTRATION
  _AM_IPC_MW_CMD_SET_SIP_REGISTRATION = SIP_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_SET_SIP_CONFIGURATION
  _AM_IPC_MW_CMD_SET_SIP_CONFIGURATION,

  //! _AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY
  _AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY,

  //!_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS
  _AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS,

  //!_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME
  _AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME,
};

/****************************SIP Service CMDs**********************************/
/*! @defgroup airapi-commandid-sip Air API Command IDs - SIP Service
 *  @ingroup airapi-commandid
 *  @brief SIP Service Related command IDs,
 *         refer to @ref airapi-datastructure-sip
 *         "Data Structure of SIP Service" to see data structures
 *  @{
 */
//SIP Service CMDS
/*!
 * @brief Set SIP registration parameters
 *
 * Use this command to set sip registration parameters
 * @sa AM_IPC_MW_CMD_SET_SIP_REGISTRATION
 * @sa AMSipRegisterParameter
 */
#define AM_IPC_MW_CMD_SET_SIP_REGISTRATION                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SET_SIP_REGISTRATION,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SIP)
/*!
 * @brief Set SIP configuration parameters
 *
 * Use this command to set sip registration parameters
 * @sa AM_IPC_MW_CMD_SET_SIP_CONFIGURATION
 * @sa AMSipConfigParameter
 */
#define AM_IPC_MW_CMD_SET_SIP_CONFIGURATION                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SET_SIP_CONFIGURATION,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SIP)
/*!
 * @brief Set SIP media priority list
 *
 * Use this command to set sip registration parameters
 * @sa AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY
 * @sa AMMediaPriorityList
 */
#define AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SIP)
/*!
 * @brief Set SIP callee address
 *
 * Use this command to set sip callee address parameter
 * @sa AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS
 * @sa AMSipCalleeAddress
 */
#define AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SIP)

/*!
 * @brief Set SIP hangup username
 *
 * Use this command to hangup specific calls
 * @sa AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME
 * @sa AMSipHangupUsername
 */
#define AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SIP)

/*!
 * @}
 */
/******************************************************************************/

/*! @example test_sip_service.cpp
 *  This is the example program of SIP Service APIs.
 */
#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_SIP_H_ */
