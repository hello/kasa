/*******************************************************************************
 * am_api_cmd_video.h
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
/*! @file am_api_cmd_video.h
 *  @brief This file defines Video Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_VIDEO
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_VIDEO
{
  //!_AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET = VIDEO_SERVICE_CMD_START,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET
  _AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET
  _AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET,

  //!_AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD
  _AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD,

  //!_AM_IPC_MW_CMD_VIDEO_ENCODE_START
  _AM_IPC_MW_CMD_VIDEO_ENCODE_START,

  //!_AM_IPC_MW_CMD_VIDEO_ENCODE_STOP
  _AM_IPC_MW_CMD_VIDEO_ENCODE_STOP,

  //!_AM_IPC_MW_CMD_VIDEO_POWER_MODE_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_SET,

  //!_AM_IPC_MW_CMD_VIDEO_POWER_MODE_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET,
  _AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET,
  _AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET,

  //!_AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET,
  _AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT
  _AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET
  _AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET
  _AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET,

  //! _AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET
  _AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET,

  //! _AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET
  _AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP
  _AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP,

  //! _AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START
  _AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START,
};


/******************************Video Service CMDS******************************/
/*! @defgroup airapi-commandid-video Air API Command IDs - Video Service
 *  @ingroup airapi-commandid
 *  @brief Video Service Related command IDs,
 *         refer to @ref airapi-datastructure-video
 *         "Data Structure of Video Service" to see data structures.
 *  @{
 */

/*! @defgroup airapi-commandid-video-feature Feature Commands
 *  @ingroup airapi-commandid-video
 *  @brief feature related Commands,
 *  see @ref airapi-datastructure-video-feature "Feature Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_feature_config_s
 *  @{
 */ /* Start of feature Commands Section */

/*! @brief Get video feature information
 *
 * use this command to get video feature information
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET
 * @sa am_feature_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set video feature parameters
 *
 * use this command to set video feature parameters
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET
 * @sa am_feature_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Feature Commands */



/*! @defgroup airapi-commandid-video-vin VIN Commands
 *  @ingroup airapi-commandid-video
 *  @brief VIN related Commands,
 *  see @ref airapi-datastructure-video-vin "VIN Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_vin_config_s
 *  @{
 */ /* Start of VIN Commands Section */

/*! @brief Set video VIN parameters
 *
 * use this command to get video VIN device parameters from config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET
 * @sa AM_IPC_MW_CMD_VIDEO_VIN_STOP
 * @sa am_vin_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set video VIN parameters
 *
 * use this command to set video VIN device parameters to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET
 * @sa AM_IPC_MW_CMD_VIDEO_VIN_STOP
 * @sa am_vin_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get current VIN parameters dynamically
 * use this command to get video current VIN parameters
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of VIN Commands */

/*! @defgroup airapi-commandid-video-vout VOUT Commands
 *  @ingroup airapi-commandid-video
 *  @brief VOUT related commands,
 *  see @ref airapi-datastructure-video-vout "VOUT Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of VOUT Commands Section */

/*! @brief Shutdown a video VOUT
 *
 * use this command to halt a video VOUT device
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT                                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT,                 \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set video VOUT parameters
 *
 * use this command to set video VOUT device parameters to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT
 * @sa am_vout_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get video VOUT parameters
 *
 * use this command to get video VOUT device parameters from config file
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET
 * @sa am_vout_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of VOUT Commands Section*/

/*! @defgroup airapi-commandid-video-src-buf-fmt Source Buffer Format Commands
 *  @ingroup airapi-commandid-video
 *  @brief Source Buffer format related commands,
 *  see @ref airapi-datastructure-video-src-buf-fmt
 *  "Source Buffer Format" for more information.
 *  @sa AMAPIHelper
 *  @sa am_buffer_fmt_s
 *  @{
 */ /* Start of Source Buffer Format */

/*! @brief Get buffer format
 *
 * use this command to get buffer format from config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set buffer format
 *
 * use this command to set buffer format to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)


/*! @brief Get buffer state
 *
 * use this command to get buffer state dynamically
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get buffer format
 *
 * use this command to get buffer format dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set buffer format
 *
 * use this command to set buffer format dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get buffer max number
 *
 * use this command to get buffer max number
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Source Buffer Format Commands */

/*! @defgroup airapi-commandid-video-stream-fmt Stream Format Commands
 *  @ingroup airapi-commandid-video
 *  @brief Stream format related Commands,
 *  see @ref airapi-datastructure-video-stream-fmt
 *  "Stream Formate Parameters" for more information.
 *  @sa AMAPIHelper
 *  @sa am_stream_fmt_s
 *  @{
 */ /* Start of Stream Format Parameters */

/*! @brief Set stream format
 *
 * use this command to get stream format from config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET
 * @sa am_stream_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream format
 *
 * use this command to set stream format to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET
 * @sa am_stream_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Stream Format Commands */

/*! @defgroup airapi-commandid-video-stream-cfg Stream Config Commands
 *  @ingroup airapi-commandid-video
 *  @brief Stream config related commands,
 *  see @ref airapi-datastructure-video-stream-cfg "Stream Config Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_stream_cfg_s
 *  @{
 */ /* Start of Stream Config Commands */

/*! @brief Set stream configuration
 *
 * use this command to get stream h264 or h265 parameters from config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET
 * @sa am_h26x_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream configuration
 *
 * use this command to set stream h264 or h265 parameters to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET
 * @sa am_h26x_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream configuration
 *
 * use this command to get stream mjpeg parameters from config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET
 * @sa am_mjpeg_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream configuration
 *
 * use this command to set stream mjpeg parameters to config file
 * @sa AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET
 * @sa am_mjpeg_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Stream Config Commands */

/*! @defgroup airapi-commandid-video-enc-on-off Encode Start and Stop Commands
 *  @ingroup airapi-commandid-video
 *  @brief Encode start and stop related commands
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Encode Start and Stop Commands */

/*! @brief Start to encode
 *
 * use this command to start encoding
 * @sa AM_IPC_MW_CMD_VIDEO_ENCODE_STOP
 */
#define AM_IPC_MW_CMD_VIDEO_ENCODE_START                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_ENCODE_START,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Stop to encode
 *
 * use this command to stop encoding
 * @sa AM_IPC_MW_CMD_VIDEO_ENCODE_START
 */
#define AM_IPC_MW_CMD_VIDEO_ENCODE_STOP                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_ENCODE_STOP,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set to dsp power mode
 *
 * use this command to set dsp power mode
 * @sa AM_IPC_MW_CMD_VIDEO_POWER_MODE_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_SET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_SET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get to dsp power mode
 *
 * use this command to get dsp power mode
 * @sa AM_IPC_MW_CMD_VIDEO_POWER_MODE_SET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_GET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_POWER_MODE_GET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Encode Start and Stop Commands */

/*! @defgroup airapi-commandid-video-stream-dynamic-control Stream Dynamic Control Commands
 *  @ingroup airapi-commandid-video
 *  @brief Stream dynamic control related commands
 *  see @ref airapi-datastructure-video-stream-dynamic-control
 *  "Stream Dynamic Control Parameters" for more information.
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Stream Dynamic Control Commands */

/*! @brief Get stream max number
 *
 * use this command to get stream max number
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get streams status
 *
 * use this command to get all streams status
 * @sa am_stream_status_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Check streams lock state
 *
 * use this command to check streams lock state
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Test if stream is occupied and lock it
 *
 * use this command to test if stream is occupied and lock it
 * @sa am_stream_lock_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Start a stream dynamically
 *
 * use this command to start a stream dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Stop a stream dynamically
 *
 * use this command to stop a stream dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Force a stream to idr
 *
 * use this command to force a stream to idr
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set a stream parameter dynamically
 *
 * use this command to set a stream parameter dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET
 * @sa am_stream_parameter_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get a stream parameters dynamically
 *
 * use this command to get a stream parameters dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET
 * @sa am_stream_parameter_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get available CPU frequencies
 *
 * use this command to get available CPU frequencies
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get current CPU frequency
 *
 * use this command to get current CPU frequency
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_CUR_CPU_CLK_GET,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)


/*! @brief Set current CPU frequency
 *
 * use this command to set current CPU frequency
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET
 * @sa AM_IPC_MW_CMD_VIDEO_ENCODE_STOP
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)


/*! @} */ /* End of Stream Dynamic Control Commands */

/*! @defgroup airapi-commandid-video-dptz DPTZ Commands
 *  @ingroup aorapi-commandid-video
 *  @brief DPTZ related commands used
 *  see @ref airapi-datastructure-video-dptz "DPTZ" for more
 *  information.
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of DPTZ */

/*! @brief Get DPTZ Ratio setting dynamically
 *
 * use this command to get DPTZ setting dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SET
 * @sa am_dptz_ratio_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get DPTZ Size setting dynamically
 *
 * use this command to get DPTZ Size setting dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET
 * @sa am_dptz_size_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set DPTZ setting dynamically
 *
 * use this command to set DPTZ setting dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET
 * @sa am_dptz_ratio_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET,            \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set DPTZ Size setting dynamically
 *
 * use this command to set DPTZ Size setting dynamically
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET
 * @sa am_dptz_size_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of DPTZ Commands */

/*! @defgroup airapi-commandid-video-warp Warp Commands
 *  @ingroup aorapi-commandid-video
 *  @brief Warp related commands used
 *  see @ref airapi-datastructure-video-warp "Warp" for more
 *  information.
 *  @sa AMAPIHelper
 *  @sa am_warp_s
 *  @{
 */ /* Start of Warp */

/*! @brief Get Warp setting dynamically
 *
 * use this command to get Warp setting dynamically
 * @sa _AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET
 * @sa am_warp_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set Warp setting dynamically
 *
 * use this command to set Warp setting dynamically
 * @sa _AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET
 * @sa am_warp_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Warp Commands*/

/*! @defgroup airapi-commandid-video-lbr LBR Control Commands
 *  @ingroup airapi-commandid-video
 *  @brief lbr control related commands used,
 *  see @ref airapi-datastructure-video-lbr "LBR Control" for more
 *  information.
 *  @sa AMAPIHelper
 *  @sa am_encode_lbr_ctrl_s
 *  @{
 */ /* Start of LBR Control */

/*! @brief Set LBR setting
 *
 * use this command to set lbr setting
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET
 * @sa am_encode_lbr_ctrl_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get LBR setting
 *
 * use this command to get lbr setting
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET
 * @sa am_encode_lbr_ctrl_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of LBR Control Commands*/

/*! @defgroup airapi-commandid-video-evt-get Event Get Commands
 *  @ingroup airapi-commandid-video
 *  @brief Event related commands,
 *  see @ref airapi-commandid-event "Air API Command IDs - Event Service" for
 *  Event Service related command IDs
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Event Get*/

/*! @brief Get event
 *
 * use this command to get event
 */
#define AM_IPC_MW_CMD_VIDEO_GET_EVENT                                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_GET_EVENT,                     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NO_NEED_RETURN,                              \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */   /* End of Event Get Commands*/

/*! @defgroup airapi-commandid-video-overlay Overlay Commands
 *  @ingroup airapi-commandid-video
 *  @brief Overlay related commands,
 *  see @ref airapi-datastructure-video-overlay "Overlay Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_overlay_area_s
 *  @sa am_overlay_data_s
 *  @sa am_overlay_id_s
 *  @sa am_overlay_limit_val_s
 *  @{
 */ /* Start of Overlay Commands */

/*! @brief Get overlay platform and user defined limit value
 *
 * use this command to get overlay max area number
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET,       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Destroy all overlay
 *
 * use this command to destroy all overlay areas
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Save all overlay parameters to configure file
 *
 * use this command to save all user parameters to configure file
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Init a overlay area
 *
 * use this command to init a overlay area
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 * @sa am_overlay_area_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Add a data block to area
 *
 * use this command to add a data block to area
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 * @sa am_overlay_data_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Update a area data block
 *
 * use this command to add one overlay
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 * @sa am_overlay_id_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE,       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set overlay
 *
 * use this command to remove/enable/disable a area or remove a data block
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 * @sa am_overlay_id_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get area parameter
 *
 * use this command to get a area attribute parameter
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET
 * @sa am_overlay_id_s
 * @sa am_overlay_area_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get data block parameter
 *
 * use this command to get a data block parameter
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET
 * @sa am_overlay_id_s
 * @sa am_overlay_data_s
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET,          \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Overlay Commands */

/*! @defgroup airapi-commandid-video-eis EIS Config Commands
 *  @ingroup airapi-commandid-video
 *  @brief EIS Control related commands
 *  @sa AMAPIHelper
 *  @sa am_encode_eis_ctrl_s
 *  @{
 */ /* Start of EIS Config Commands */

/*! @brief Set EIS setting
 *
 * use this command to set EIS setting
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get EIS setting
 *
 * use this command to get EIS setting
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of EIS Config Commands */

/*! @defgroup airapi-commandid-video-config-load Load Config Commands
 *  @ingroup airapi-commandid-video
 *  @brief  Load module config related commands
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of CONFIG LOAD Commands Section */

/*! @brief Load all modules config
 *
 * use this command to load all modules config file
 */
#define AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)


/*! @} */ /* End of CONFIG LOAD Commands Section*/

/*! @defgroup airapi-commandid-video-motion Motion Detection
 *  @ingroup airapi-commandid-video
 *  @brief Motion Detection related command IDs,
 *         refer to @ref airapi-datastructure-event-md
 *         "Motion Detection" to see data structures
 *  @{
 */
/*! @brief Set configure of Motion Detect Module
 *
 * Use this command to set a configure item of motion detect module
 * @sa AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET
 * @sa AM_VIDEO_MD_CONFIG_BITS
 */

#define AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get configure of Motion Detect Module
 *
 * Use this command to get a configure item of motion detect module
 * @sa AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET
 * @sa AM_VIDEO_MD_CONFIG_BITS
 */
#define AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Stop Motion Detect Module
 *
 * Use this command to dynamic stop motion detect module
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Start Motion Detect Module
 *
 * Use this command to dynamic start motion detect module
 * @sa AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP
 */
#define AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START,       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Motion Detect Commands Section*/

/*! * @} */ /* End of Video Service Command IDs */
/******************************************************************************/

/*! @example test_video_service.cpp
 *  This is the example program of Video Service APIs.
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_H_ */
