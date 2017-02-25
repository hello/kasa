/*******************************************************************************
 * am_api_cmd_video_edit.h
 *
 * History:
 *   2016-04-28 - [Zhi He] created file
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
/*! @file am_api_cmd_video_edit.h
 *  @brief This file defines Video Edit Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_EDIT_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_EDIT_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_VIDEO_EDIT
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_VIDEO_EDIT
{
  //! _AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES
  _AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES = VIDEO_EDIT_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4
  _AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4,

  //! _AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES
  _AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES,

  //! _AM_IPC_MW_CMD_VIDEO_EDIT_END_PROCESSING
  _AM_IPC_MW_CMD_VIDEO_EDIT_END_PROCESSING,
};

/*****************************Video Edit Service CMDs*******************************/
/*! @defgroup airapi-commandid-video edit Air API Command IDs - Video Edit Service
 *  @ingroup airapi-commandid
 *  @brief Video Edit Service Related command IDs,
 *         refer to @ref airapi-datastructure-video edit
 *         "Data Structure of Video Edit" to see data structures
 *  @{
 */

/*! @brief parse es file, and re-encode by EFM
 *
 *  Use this command to parse and re-encode by EFM.
 *  @sa AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES
 */
#define AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO_EDIT)

/*! @brief parse es file and re-encode by EFM, then mux to mp4
 *
 *  Use this command to parse es file and re-encode by EFM, then mux to mp4
 *  @sa AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4
 */
#define AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO_EDIT)

/*! @brief parse es file and re-encode by EFM, do not read re-encode bitstream
 *
 *  Use this command to parse es file and re-encode by EFM, do not read re-encode bitstream
 *  @sa AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES
 */
#define AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO_EDIT)

/*! @brief end processing of video edit's instance
 *
 *  Use this command to end processing of video edit's instance
 *  @sa AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES
 */
#define AM_IPC_MW_CMD_VIDEO_EDIT_END_PROCESSING                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_EDIT_END_PROCESSING,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO_EDIT)

/*!
 *  @}
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_EDIT_H_ */
