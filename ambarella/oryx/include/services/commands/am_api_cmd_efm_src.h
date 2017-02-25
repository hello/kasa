/*******************************************************************************
 * am_api_cmd_efm_src.h
 *
 * History:
 *   2016-05-04 - [Zhi He] created file
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
/*! @file am_api_cmd_efm_src.h
 *  @brief This file defines EFM source Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_EFM_SRC_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_EFM_SRC_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_EFM_SOURCE
 *  @brief Used for EFM source service
 */
enum AM_SYS_IPC_MW_CMD_EFM_SOURCE
{
  //! _AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE
  _AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE = EFM_SOURCE_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_FEED_EFM_FROM_USB_CAMERA
  _AM_IPC_MW_CMD_FEED_EFM_FROM_USB_CAMERA,

  //! _AM_IPC_MW_CMD_END_FEED_EFM
  _AM_IPC_MW_CMD_END_FEED_EFM,

  //!_AM_IPC_MW_CMD_SETUP_YUV_CAPTURE_PIPELINE,
  _AM_IPC_MW_CMD_SETUP_YUV_CAPTURE_PIPELINE,

  //!_AM_IPC_MW_CMD_DESTROY_YUV_CAPTURE_PIPELINE,
  _AM_IPC_MW_CMD_DESTROY_YUV_CAPTURE_PIPELINE,

  //!_AM_IPC_MW_CMD_YUV_CAPTURE_FOR_EFM,
  _AM_IPC_MW_CMD_YUV_CAPTURE_FOR_EFM,
};

/*****************************EFM Source Service CMDs**************************/
/*! @defgroup airapi-commandid-EFM source Air API Command IDs - EFM Source Service
 *  @ingroup airapi-commandid
 *  @brief EFM Source Service Related command IDs,
 *         refer to @ref airapi-datastructure-EFM source
 *         "Data Structure of EFM Source" to see data structures
 *  @{
 */

/*! @brief parse es file, decode and feed EFM
 *
 *  Use this command to parse local file, decode and feed as EFM source.
 *  @sa AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE
 */
#define AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)

/*! @brief read from USB camera (via v4l), decode and feed EFM
 *
 *  Use this command read from USB Camera, decode and feed EFM.
 *  @sa AM_IPC_MW_CMD_FEED_EFM_FROM_LOCAL_ES_FILE
 */
#define AM_IPC_MW_CMD_FEED_EFM_FROM_USB_CAMERA                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_FEED_EFM_FROM_USB_CAMERA,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)

/*! @brief end feed EFM
 *
 *  Use this command to end feed EFM
 *  @sa AM_IPC_MW_CMD_END_FEED_EFM
 */
#define AM_IPC_MW_CMD_END_FEED_EFM                                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_END_FEED_EFM,                        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)

/*! @brief setup yuv capture and feed efm pipeline, pre-setup pipeline is used to capture YUV frame in short delay
 *
 *  Use this command to setup yuv capture and feed efm pipeline
 *  @sa AM_IPC_MW_CMD_SETUP_YUV_CAPTURE_PIPELINE
 */
#define AM_IPC_MW_CMD_SETUP_YUV_CAPTURE_PIPELINE                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_SETUP_YUV_CAPTURE_PIPELINE,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)


/*! @brief destroy yuv capture and feed efm pipeline
 *
 *  Use this command to destroy yuv capture and feed efm pipeline
 *  @sa AM_IPC_MW_CMD_DESTROY_YUV_CAPTURE_PIPELINE
 */
#define AM_IPC_MW_CMD_DESTROY_YUV_CAPTURE_PIPELINE                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_DESTROY_YUV_CAPTURE_PIPELINE,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)


/*! @brief capture yuv frames and feed them to EFM
 *
 *  Use this command to capture yuv frames and feed efm pipeline
 *  @sa AM_IPC_MW_CMD_YUV_CAPTURE_FOR_EFM
 */
#define AM_IPC_MW_CMD_YUV_CAPTURE_FOR_EFM                                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_YUV_CAPTURE_FOR_EFM,                 \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_EFM_SRC)


/*!
 *  @}
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_EFM_SRC_H_ */
