/*******************************************************************************
 * am_api_cmd_image.h
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
/*! @file am_api_cmd_image.h
 *  @brief This file defines Image Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_IMAGE_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_IMAGE_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_IMAGE
 *  @brief Used for system IPC basic function
 */

enum AM_SYS_IPC_MW_CMD_IMAGE
{
  //! _AM_IPC_MW_CMD_IMAGE_TEST
  _AM_IPC_MW_CMD_IMAGE_TEST = IMAGE_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET
  _AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET,

  //! _AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET
  _AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET,

  //! _AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET
  _AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET,

  //! _AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET
  _AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET,

  //! _AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET
  _AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET,

  //! _AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET
  _AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET,

  //! _AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET
  _AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET,

  //! _AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET
  _AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET,

  //! _AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET
  _AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET,

  //! _AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET
  _AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET,

  //! _AM_IPC_MW_CMD_IMAGE_LUMA_INFO_GET
  _AM_IPC_MW_CMD_IMAGE_LUMA_INFO_GET,

  //! _AM_IPC_MW_CMD_IMAGE_IRLED_SET
  _AM_IPC_MW_CMD_IMAGE_IRLED_SET,

  //! _AM_IPC_MW_CMD_IMAGE_IRLED_GET
  _AM_IPC_MW_CMD_IMAGE_IRLED_GET,
};

/****************************Image Service CMDS********************************/
/*! @defgroup airapi-commandid-image Air API Command IDs - Image Service
 *  @ingroup airapi-commandid
 *  @brief Image Service Related command IDs,
 *         refer to @ref airapi-datastructure-image
 *         "Data Structure of Image Service" to see data structures
 *  @{
 */

/*! @brief This command is used for APIProxy test program,
 *  see @ref test_api_helper.cpp for more information
 */
#define AM_IPC_MW_CMD_IMAGE_TEST                                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_TEST,                          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @defgroup airapi-commandid-image-ae AE Commands
 *  @ingroup airapi-commandid-image
 *  @brief Image Service AE related command IDs,
 *         refer to @ref airapi-datastructure-image-ae
 *         "Data Structure of AE Settings" to see data structure
 *  @{
 */
/*! @brief Get AE setting
 *
 *  Use this command to get AE related parameters in current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET
 *  @sa am_ae_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @brief Set AE setting
 *
 *  Use this command to set AE related parameters into current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET
 *  @sa am_ae_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)
/*! @} */

/*! @defgroup airapi-commandid-image-awb AWB Commands
 *  @ingroup airapi-commandid-image
 *  @brief Image Service AWB related command IDs,
 *         refer to @ref airapi-datastructure-image-awb
 *         "Data Structure of AWB Settings" to see data structure
 *  @{
 */
/*! @brief Get AWB setting
 *
 *  Use this command to get AWB related parameters in current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET
 *  @sa am_awb_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @brief Set AWB setting
 *
 *  Use this command to set AWB related parameters into current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET
 *  @sa am_awb_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)
/*! @} */

/*! @defgroup airapi-commandid-image-af AF Commands
 *  @ingroup airapi-commandid-image
 *  @brief Image Service AF related command IDs,
 *         refer to @ref airapi-datastructure-image-af
 *         "Data Structure of AF Settings" to see data structure
 *  @{
 */
/*! @brief Get AF setting
 *
 *  Use this command to get AF related parameters in current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET
 *  @sa am_af_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @brief Set AF setting
 *
 *  Use this command to set AF related parameters into current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_AF_SETTING_GET
 *  @sa am_af_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_AF_SETTING_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)
/*! @} */

/*! @defgroup airapi-commandid-image-nr Noise Reduction Commands
 *  @ingroup airapi-commandid-image
 *  @brief Image Service Noise Reduction related command IDs,
 *         refer to @ref airapi-datastructure-image-nr
 *         "Data Structure of Noise Reduction Settings" to see data structure
 *  @{
 */
/*! @brief Get NR setting
 *
 *  Use this command to get Noise Filter related parameters in current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET
 *  @sa am_noise_filter_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @brief Set NR setting
 *
 *  Use this command to set Noise Filter related parameters into current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET
 *  @sa am_noise_filter_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)
/*! @} */

/*! @defgroup airapi-commandid-image-style Image Style Commands
 *  @ingroup airapi-commandid-image
 *  @brief Image Service Noise Reduction related command IDs,
 *         refer to @ref airapi-datastructure-image-style
 *         "Data Structure of Image Style Settings" to see data structure
 *  @{
 */
/*! @brief Get Image Style setting
 *
 *  Use this command to get Image Style related parameters in current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET
 *  @sa am_image_style_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)

/*! @brief Set Image Style setting
 *
 *  Use this command to set Image Style related parameters into current system.
 *  @sa AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET
 *  @sa am_image_style_config_s
 */
#define AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_IMAGE)
/*! @} */
/*! @} */
/******************************************************************************/

/*! @example test_image_service.cpp
 *  This is the example program of Image Service APIs.
 */

/*! @example test_api_helper.cpp
 *  This is the test program of APIHelper.
 */
#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_IMAGE_H_ */
