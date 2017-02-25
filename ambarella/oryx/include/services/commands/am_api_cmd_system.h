/*******************************************************************************
 * am_api_cmd_system.h
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
/*! @file am_api_cmd_system.h
 *  @brief This file defines System Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_SYSTEM_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_SYSTEM_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_SYSTEM
 *  @brief Used for system IPC basic function
 */

enum AM_SYS_IPC_MW_CMD_SYSTEM
{
  //! _AM_IPC_MW_CMD_SYSTEM_TEST
  _AM_IPC_MW_CMD_SYSTEM_TEST = SYSTEM_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET
  _AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET
  _AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET
  _AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET
  _AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET
  _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET
  _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT
  _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT,

  //! _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL
  _AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL,

  //! _AM_IPC_MW_CMD_SYSTEM_UID_GET
  _AM_IPC_MW_CMD_SYSTEM_UID_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_UPGRADEMODE_GET
  _AM_IPC_MW_CMD_SYSTEM_UPGRADEMODE_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_UPGRADEMODE_SET
  _AM_IPC_MW_CMD_SYSTEM_UPGRADEMODE_SET,

  //!< _AM_IPC_MW_CMD_SYSTEM_RESET_FACTORY_SET
  _AM_IPC_MW_CMD_SYSTEM_RESET_FACTORY_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_REBOOT_SET
  _AM_IPC_MW_CMD_SYSTEM_REBOOT_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_SERIALNUM_GET
  _AM_IPC_MW_CMD_SYSTEM_SERIALNUM_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_SERIALNUM_SET
  _AM_IPC_MW_CMD_SYSTEM_SERIALNUM_SET,

  //! _AM_IPC_MW_CMD_SYSTEM_LASTUPDATE_GET
  _AM_IPC_MW_CMD_SYSTEM_LASTUPDATE_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_INFO_GET
  _AM_IPC_MW_CMD_SYSTEM_INFO_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_ENCRYPTION_KEY_GET
  _AM_IPC_MW_CMD_SYSTEM_ENCRYPTION_KEY_GET,

  //! _AM_IPC_MW_CMD_SYSTEM_ENCRYPTION_CONTROL_SET
  _AM_IPC_MW_CMD_SYSTEM_ENCRYPTION_CONTROL_SET,

  //! Start firmware upgrade operation
  _AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET,

  //! Check upgrade status
  _AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET,

  //! Get firmware version
  _AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET,
};

/*****************************System Service CMDS******************************/

/*! @defgroup airapi-commandid-system Air API Command IDs - System Service
 *  @ingroup airapi-commandid
 *  @brief System Service Related command IDs,
 *         refer to @ref airapi-datastructure-system
 *         "Data Structure of System Service" to see data structures
 *  @{
 */

/*! @defgroup airapi-commandid-system-firm-up Firmware Upgrade Commands
 *  @ingroup airapi-commandid-system
 *  @brief System time related commands,
 *  see @ref airapi-datastructure-system-firm-up "Firmware Upgrade Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @{
 */
/*! @brief Set firmware upgrade configure
 *
 * use this command to set firmware upgrade parameters
 * @sa AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET
 * @sa am_mw_fw_upgrade_args
 */
#define AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Get firmware upgrade status
 *
 * use this command to get firmware upgrade status
 * @sa AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET
 * @sa am_mw_upgrade_status
 */
#define AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET,  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)
/*! @} */

/*! @defgroup airapi-commandid-system-firm-ver Firmware Version Commands
 *  @ingroup airapi-commandid-system
 *  @brief System time related commands,
 *  see @ref airapi-datastructure-system-firm-ver "Firmware Version Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @{
 */
/*! @brief Get firmware version
 *
 * use this command to get current firmware version information
 * @sa am_mw_firmware_version_all
 */
#define AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)
/*! @} */

/*! @defgroup airapi-commandid-system-led LED Control Commands
 *  @ingroup airapi-commandid-system
 *  @brief System time related commands,
 *  see @ref airapi-datastructure-system-led "LED Config Parameters" for
 *  more information.
 *  @sa AMAPIHelper
 *  @{
 */
/*! @brief Get specified gpio LED status
 *
 * use this command to get specified gpio LED status
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL
 * @sa am_mw_led_config
 */
#define AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Set led status
 *
 * use this command to set specified gpio LED status
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL
 * @sa am_mw_led_config
 */
#define AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Release specified gpio LED
 *
 * use this command to release specified initialized gpio led
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL
 */
#define AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Release all gpio LEDs
 *
 * use this command to release all initialized gpio LEDs
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET
 * @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT
 */
#define AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)
/*! @} */

/*! @defgroup airapi-commandid-system-sys-time System Time Commands
 *  @ingroup airapi-commandid-system
 *  @brief System time related commands,
 *  see @ref airapi-datastructure-system-sys-time "System Time Parameters" for
 *  more information.
 *  @sa AMAPIHelper
 *  @{
 */
/*! @brief Get system time
 *
 * use this command to get system time
 * @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET
 * @sa am_mw_system_settings_datetime
 */
#define AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Set system time
 *
 * use this command to set system time
 * @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET
 * @sa am_mw_system_settings_datetime
 */
#define AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)
/*! @} */

/*! @defgroup airapi-commandid-system-ntp NTP Server Config Commands
 *  @ingroup airapi-commandid-system
 *  @brief System time related commands,
 *  see @ref airapi-datastructure-system-ntp "NTP Server Configuration" for
 *  more information.
 *  @sa AMAPIHelper
 *  @{
 */
/*! @brief Get ntp settings
 *
 * use this command to get ntp settings
 * @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET
 */
#define AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)

/*! @brief Get ntp settings
 *
 * use this command to get ntp settings
 * @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET
 */
#define AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_SYSTEM)
/*! @} */
/*! @} */ /* API of "System Service" */
/******************************************************************************/

/*! @example test_system_service.cpp
 *  This is the example program of System Service APIs.
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_SYSTEM_H_ */
