/*******************************************************************************
 * am_api_system.h
 *
 * History:
 *   2015-1-20 - [longli] created file
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
/*! @file am_api_system.h
 *  @brief This file defines Oryx System Service related data structures
 */
#ifndef AM_API_SYSTEM_H_
#define AM_API_SYSTEM_H_

#include "commands/am_api_cmd_system.h"

/*! @defgroup airapi-datastructure-system Data Structure of System Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx System Service related method call data structures
 *  @{
 */

/**************************System Time Parameters******************************/
/*! @defgroup airapi-datastructure-system-sys-time System Time Parameters
 *  @ingroup airapi-datastructure-system
 *  @brief System time related parameters,
 *         refer to @ref airapi-commandid-system-sys-time "System Time Commands"
 *         to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET
 *  @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET
 *  @sa am_mw_system_settings_datetime
 *  @{
 */ /* Start of System Time Section */

/*! @struct am_mw_date
 *  @brief For calendar date
 */
struct am_mw_date
{
    uint16_t year; //!< year
    uint8_t  month; //!< month
    uint8_t  day; //!< day
};

/*! @struct am_mw_time
 *  @brief For time of day
 */
struct am_mw_time
{
    uint8_t  hour;   //!< hour in 24-hour format
    uint8_t  minute; //!< minute
    uint8_t  second; //!< second
    /*!
     * - negative(-): west time zone
     * - positive(+): east time zone
     *
     * eg. +8 is for Beijing Time.
     */
    int8_t   timezone; //!< time zone
};

/*! @struct am_mw_system_settings_datetime
 *  @brief For system time GET and SET
 */
struct am_mw_system_settings_datetime
{
    /*!
     * @sa am_mw_date
     */
    am_mw_date date; //!< day to set
    /*!
     * @sa am_mw_time
     */
    am_mw_time time; //!< time to set
};
/*! @} */ /* End of System Time Section */
/******************************************************************************/

/**************************** firmware version ********************************/
/*! @defgroup airapi-datastructure-system-firm-ver Firmware Version Parameters
 *  @ingroup airapi-datastructure-system
 *  @brief Firmware Version related parameters,
 *         refer to @ref airapi-commandid-system-firm-ver
 *         "Firmware Version Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET
 *  @{
 */ /* Start of Firmware Version Section */

/*! Define the length of Linux SHA1 */
#define LINUX_SHA1_LEN 64

/*! @struct am_mw_firmware_version
 *  @brief For firmware version number
 */
struct am_mw_firmware_version
{
    uint8_t major; //!< major version number
    uint8_t minor; //!< minor version number
    uint8_t trivial; //!< revision number
};

/*! @struct am_mw_firmware_version_all
 *  @brief For firmware overall version
 */
struct am_mw_firmware_version_all
{
    uint32_t rc_number;  //!< release candidate number
    am_mw_date rel_date; //!< release firmware date
    /*!
     * @sa am_mw_firmware_version
     */
    am_mw_firmware_version sw_version;  //!< firmware overall version
    /*!
     * @sa am_mw_firmware_version
     */
    am_mw_firmware_version linux_kernel_version; //!< kernel version
    char linux_sha1[LINUX_SHA1_LEN]; //!< linux sha1
};
/*! @} */ /* End of Firmware Version Section */
/******************************************************************************/

/**************************** firmware upgrade ********************************/
/*! @defgroup airapi-datastructure-system-firm-up Firmware Upgrade Parameters
 *  @ingroup airapi-datastructure-system
 *  @brief Firmware Upgrade related parameters,
 *         refer to @ref airapi-commandid-system-firm-up
 *         "Firmware Upgrade Commands" to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET
 *  @sa AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET
 *  @{
 */ /* Start of Firmware Upgrade Section */

/*! @enum am_mw_upgrade_state
 *  @brief Firmware upgrade status
 */
enum am_mw_upgrade_state
{
  AM_MW_NOT_KNOWN                = 0,//!< AM_MW_NOT_KNOWN
  AM_MW_UPGRADE_PREPARE,             //!< AM_MW_UPGRADE_PREPARE
  AM_MW_UPGRADE_PREPARE_FAIL,        //!< AM_MW_UPGRADE_PREPARE_FAIL
  AM_MW_DOWNLOADING_FW,              //!< AM_MW_DOWNLOADING_FW
  AM_MW_DOWNLOAD_FW_COMPLETE,        //!< AM_MW_DOWNLOAD_FW_COMPLETE
  AM_MW_DOWNLOAD_FW_FAIL,            //!< AM_MW_DOWNLOAD_FW_FAIL
  AM_MW_INIT_PBA_SYS_DONE,           //!< AM_MW_INIT_PBA_SYS_DONE
  AM_MW_INIT_PBA_SYS_FAIL,           //!< AM_MW_INIT_PBA_SYS_FAIL
  AM_MW_UNPACKING_FW,                //!< AM_MW_UNPACKING_FW
  AM_MW_UNPACK_FW_DONE,              //!< AM_MW_UNPACK_FW_DONE
  AM_MW_UNPACK_FW_FAIL,              //!< AM_MW_UNPACK_FW_FAIL
  AM_MW_FW_INVALID,                  //!< AM_MW_FW_INVALID
  AM_MW_WRITEING_FW_TO_FLASH,        //!< AM_MW_WRITEING_FW_TO_FLASH
  AM_MW_WRITE_FW_TO_FLASH_DONE,      //!< AM_MW_WRITE_FW_TO_FLASH_DONE
  AM_MW_WRITE_FW_TO_FLASH_FAIL,      //!< AM_MW_WRITE_FW_TO_FLASH_FAIL
  AM_MW_INIT_MAIN_SYS,               //!< AM_MW_INIT_MAIN_SYS
  AM_MW_UPGRADE_SUCCESS,             //!< AM_MW_UPGRADE_SUCCESS
};

/*! @struct am_mw_upgrade_status
 *  @brief For firmware upgrade status GET
 */
struct am_mw_upgrade_status
{
    bool in_progress; //!< whether upgrade is in progress
    /*!
     * @sa am_mw_upgrade_state
     */
    am_mw_upgrade_state state; //!< firmware upgrade status
};

/*! @enum am_mw_firmware_upgrade_mode
 *  @brief Firmware upgrade mode
 */
enum am_mw_firmware_upgrade_mode
{
  /*!
   * Not initialize upgrade mode
   */
  AM_FW_MODE_NOT_SET         = -1, //!< AM_FW_MODE_NOT_SET
  /*!
   * Only download upgrading file from URL
   */
  AM_FW_DOWNLOAD_ONLY        = 0, //!< AM_FW_DOWNLOAD_ONLY
  /*!
   * Only update firmware with the local existing upgrading file
   */
  AM_FW_UPGRADE_ONLY         = 1, //!< AM_FW_UPGRADE_ONLY
  /*!
   * Do whole process of upgrading, download upgrading file from URL
   *  and update firmware with the file
   */
  AM_FW_DOWNLOAD_AND_UPGRADE = 2, //!< AM_FW_DOWNLOAD_AND_UPGRADE
};

/*! @struct am_mw_fw_upgrade_args
 *  @brief For firmware upgrade SET
 */
struct am_mw_fw_upgrade_args {
    char path_to_upgrade_file[128]; //!< the location of the upgrade firmware
    char user_name[32]; //!< authentication user name when needed
    char passwd[32]; //!< authentication password when needed
    uint32_t use_sdcard; //!< set whether upgrade from sdcard or not
    uint32_t timeout; //!< break out after timeout if can not connect to server
    /*!
     * @sa am_mw_firmware_upgrade_mode
     */
    am_mw_firmware_upgrade_mode upgrade_mode;//!< Firmware upgrade mode
};
/*! @} */ /* End of Firmware Upgrade Section */
/******************************************************************************/

/****************************LED Config Parameters*****************************/
/*! @defgroup airapi-datastructure-system-led LED Config Parameters
 *  @ingroup airapi-datastructure-system
 *  @brief LED config related parameters,
 *         refer to @ref airapi-commandid-system-led "LED Control Commands" to
 *         see related commands.
 *
 *  @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET
 *  @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET
 *  @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT
 *  @sa AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL
 *  @{
 */ /* Start of LED Config Section */
/*! @struct am_mw_led_config
 *  @brief For LED config GET and SET
 */
struct am_mw_led_config
{
    int32_t gpio_id; //!< gpio index
    uint32_t on_time; //!< Used for led blink, Unit:100ms, 1 = 100ms
    uint32_t off_time; //!< Used for led blink, Unit:100ms, 1 = 100ms
    bool blink_flag; //!< Whether blink
    bool led_on; //!< led status if blink not set
};
/*! @} */ /* End of LED Config Section */
/******************************************************************************/

/******************************NTP Server Config*******************************/
/*! @defgroup airapi-datastructure-system-ntp NTP Server Configuration
 *  @ingroup airapi-datastructure-system
 *  @brief NTP Server configruation related parameters,
 *         refer to @ref airapi-commandid-system-sys-time "System Time Commands"
 *         to see related commands.
 *
 *  @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET
 *  @sa AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET
 *  @{
 */ /* Start of NTP Server Configuration */

/*! Define the total number of NTP server */
#define SERVER_NUM 2

/*! @struct am_mw_ntp_cfg
 *  @brief For system service to save NTP config
 */
struct am_mw_ntp_cfg
{
    std::string server[SERVER_NUM]; //!< The NTP server URL
    uint8_t sync_time_day; //!< days of sync time, range: 00-255
    uint8_t sync_time_hour; //!< hours of sync time, range: 00 - 23
    uint8_t sync_time_minute; //!< minutes of sync time, range:00-59
    uint8_t sync_time_second; //!< seconds of sync time, range:00-59
    bool enable; //!< whether enble ntp upgrade date
};
/*! @} */ /* end of NTP Server Configuration */
/******************************************************************************/

/*!
 * @}
 */
#endif /* AM_API_SYSTEM_H_ */
