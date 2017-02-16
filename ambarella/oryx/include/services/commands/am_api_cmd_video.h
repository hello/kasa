/*******************************************************************************
 * am_api_cmd_video.h
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
  //! _AM_IPC_MW_CMD_VIDEO_VIN_GET
  _AM_IPC_MW_CMD_VIDEO_VIN_GET = VIDEO_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_VIDEO_VIN_SET
  _AM_IPC_MW_CMD_VIDEO_VIN_SET,

  //! _AM_IPC_MW_CMD_VIDEO_VOUT_GET
  _AM_IPC_MW_CMD_VIDEO_VOUT_GET,

  //! _AM_IPC_MW_CMD_VIDEO_VOUT_SET
  _AM_IPC_MW_CMD_VIDEO_VOUT_SET,

  //! _AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET
  _AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET,

  //! _AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET
  _AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET,

  //! _AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET
  _AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET,

  //! _AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET
  _AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET,

  //! _AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET
  _AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET,

  //! _AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET
  _AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET,

  //! _AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET
  _AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET,

  //! _AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET
  _AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET,

  //! _AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET
  _AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_DYNAMIC_SET
  _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_DYNAMIC_SET,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_DYNAMIC_GET
  _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_DYNAMIC_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET
  _AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET
  _AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET
  _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET
  _AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_STOP
  _AM_IPC_MW_CMD_VIDEO_ENCODE_STOP,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_START
  _AM_IPC_MW_CMD_VIDEO_ENCODE_START,

  //! _AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR
  _AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR,

  //! _AM_IPC_MW_CMD_VIDEO_OSD_OVERLAY_SET
  _AM_IPC_MW_CMD_VIDEO_OSD_OVERLAY_SET,

  //! _AM_IPC_MW_CMD_VIDEO_DPTZ_GET
  _AM_IPC_MW_CMD_VIDEO_DPTZ_GET,

  //! _AM_IPC_MW_CMD_VIDEO_DPTZ_SET
  _AM_IPC_MW_CMD_VIDEO_DPTZ_SET,

  //! _AM_IPC_MW_CMD_VIDEO_LDC_GET
  _AM_IPC_MW_CMD_VIDEO_LDC_GET,

  //! _AM_IPC_MW_CMD_VIDEO_LDC_SET
  _AM_IPC_MW_CMD_VIDEO_LDC_SET,

  //! _AM_IPC_MW_CMD_VIDEO_FORCE_IDR
  _AM_IPC_MW_CMD_VIDEO_FORCE_IDR,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_SET,

  //! _AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
  _AM_IPC_MW_CMD_VIDEO_OVERLAY_GET,
};

/******************************Video Service CMDS******************************/
/*! @defgroup airapi-commandid-video Air API Command IDs - Video Service
 *  @ingroup airapi-commandid
 *  @brief Video Service Related command IDs,
 *         refer to @ref airapi-datastructure-video
 *         "Data Structure of Video Service" to see data structures.
 *  @{
 */

/*! @defgroup airapi-commandid-video-vin VIN Commands
 *  @ingroup airapi-commandid-video
 *  @brief VIN related Commands,
 *  see @ref airapi-datastructure-video-vin "VIN Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_vin_config_s
 *  @{
 */ /* Start of VIN Commands Section */

/*! @brief Get video VIN information
 *
 * use this command to get video VIN device information
 * @sa AM_IPC_MW_CMD_VIDEO_VIN_SET
 * @sa am_vin_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_VIN_GET                                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_VIN_GET,                       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set video VIN parameters
 *
 * use this command to set video VIN device parameters
 * @sa AM_IPC_MW_CMD_VIDEO_VIN_GET
 * @sa am_vin_config_s
 */
#define AM_IPC_MW_CMD_VIDEO_VIN_SET                                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_VIN_SET,                       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of VIN Commands */

/*! @defgroup airapi-commandid-video-stream-fmt Stream Format Commands
 *  @ingroup airapi-commandid-video
 *  @brief Stream format related Commands,
 *  see @ref airapi-datastructure-video-stream-fmt
 *  "Stream Formate Parameters" for more information.
 *  @sa AMAPIHelper
 *  @sa am_stream_fmt_s
 *  @{
 */ /* Start of Stream Format Parameters */

/*! @brief Get stream format
 *
 * use this command to get stream format
 * @sa AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET
 * @sa am_stream_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream format
 *
 * use this command to set stream format
 * @sa AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET
 * @sa am_stream_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET,                \
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

/*! @brief Get stream configuration
 *
 * use this command to get stream configuration
 * @sa AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET
 * @sa am_stream_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set stream configuration
 *
 * use this command to set stream configuration
 * @sa AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET
 * @sa am_stream_cfg_s
 */
#define AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Stream Config Commands */

/*! @defgroup airapi-commandid-video-src-buf-fmt Source Buffer Format Commands
 *  @ingroup airapi-commandid-video
 *  @brief Source Buffer format related commands,
 *  see @ref airapi-datastructure-video-src-buf-fmt
 *  "Source Buffer Format" for more information.
 *  @sa AMAPIHelper
 *  @sa am_buffer_fmt_s
 *  @{
 */ /* Start of Source Buffer Format */

/*! @brief Get Buffer Format
 *
 * use this command to get buffer format
 * @sa AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set buffer format
 *
 * use this command to set buffer format
 * @sa AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET
 * @sa am_buffer_fmt_s
 */
#define AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @} */ /* End of Source Buffer Format Commands */

/*! @defgroup airapi-commandid-video-buf-style Buffer Allocation Style Commands
 *  @ingroup airapi-commandid-video
 *  @brief Buffer allocation style related commands,
 *  see @ref airapi-datastructure-video-src-buf-style
 *  "Buffer Allocation Style" for more information.
 *  @sa AMAPIHelper
 *  @sa am_buffer_alloc_style_s
 *  @{
 */ /* Start of Buffer Allocation Style */

/*! @brief Get buffer allocation style
 *
 * use this command to get buffer allocation style
 * @sa AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET
 * @sa am_buffer_alloc_style_s
 */
#define AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set buffer allocation style
 *
 * use this command to set buffer allocation style
 * @sa AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET
 * @sa am_buffer_alloc_style_s
 */
#define AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET,        \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Buffer Allocation Style Commands*/

/*! @defgroup airapi-commandid-video-stream-status Stream Status Commands
 *  @ingroup airapi-commandid-video
 *  @brief Stream status related commands
 *  @sa AMAPIHelper
 *  @sa am_stream_status_s
 *  @{
 */ /* Start of Stream Status Commands */

/*! @brief Get stream status
 *
 * use this command to get stream status
 * @sa am_stream_status_s
 */
#define AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Stream Status Commands */

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
/*! @} */ /* End of Encode Start and Stop Commands */

/*! @defgroup airapi-commandid-video-enc-dynamic-control Encode Dynamic Contorl Commands
 *  @ingroup airapi-commandid-video
 *  @brief Encode dynamic control related commands
 *  @sa AMAPIHelper
 *  @{
 */ /* Start of Encode Dynamic Contorl Commands */

/*! @brief Force encode to idr
 *
 * use this command to force encode to idr
 */
#define AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of Encode Dynamic Contorl Commands */

/*! @defgroup airapi-commandid-video-dptz_warp DPTZ Warp Commands
 *  @ingroup aorapi-commandid-video
 *  @brief DPTZ Warp related commands used
 *  see @ref airapi-datastructure-video-dptz_warp "DPTZ Warp" for more
 *  information.
 *  @sa AMAPIHelper
 *  @sa am_dptz_warp_s
 *  @{
 */ /* Start of DPTZ Warp */

/*! @brief Set DPTZ Warp setting
 *
 * use this command to set DPTZ Warp setting
 * @sa AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET
 * @sa am_dptz_warp_s
 */
#define AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET                                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET,                 \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get DPTZ Warp setting
 *
 * use this command to get DPTZ Warp setting
 * @sa AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET
 * @sa am_dptz_warp_s
 */
#define AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET                                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET,                 \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of DPTZ Warp Commands*/

/*! @defgroup airapi-commandid-video-lbr H.264 LBR Control Commands
 *  @ingroup airapi-commandid-video
 *  @brief H264 lbr control related commands used,
 *  see @ref airapi-datastructure-video-lbr "H264 LBR Control" for more
 *  information.
 *  @sa AMAPIHelper
 *  @sa am_encode_h264_lbr_ctrl_s
 *  @{
 */ /* Start of H264 LBR Control */

 /*! @brief Set H264 LBR setting
  *
  * use this command to set h264 lbr setting
  * @sa AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET
  * @sa am_encode_h264_lbr_ctrl_s
  */
#define AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

 /*! @brief Get H264 LBR setting
  *
  * use this command to get h264 lbr setting
  * @sa AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET
  * @sa am_encode_h264_lbr_ctrl_s
  */
#define AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of H264 LBR Control Commands*/

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

/*! @defgroup airapi-commandid-video-overlay OVERLAY Commands
 *  @ingroup airapi-commandid-video
 *  @brief OVERLAY related commands,
 *  see @ref airapi-datastructure-video-overlay "OVERLAY Parameters"
 *  for more information.
 *  @sa AMAPIHelper
 *  @sa am_overlay_set_s
 *  @sa am_overlay_area_s
 *  @sa am_overlay_s
 *  @{
 */ /* Start of OVERLAY Commands */

/*! @brief Get overlay max area number
 *
 * use this command to get overlay max area number
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM,           \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Destroy all overlay
 *
 * use this command to destroy all overlay
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Save all overlay parameter to configure file
 *
 * use this command to save all user parameters to configure file
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE,                  \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Add overlay
 *
 * use this command to add one overlay
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 * @sa am_overlay_s
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD                                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD,                  \
                           AM_IPC_DIRECTION_DOWN,                             \
                           AM_IPC_NEED_RETURN,                                \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Set overlay
 *
 * use this command to remove/enable/disable overlay
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET
 * @sa am_overlay_set_s
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_SET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_SET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)

/*! @brief Get overlay configure
 *
 * use this command to get all overlay parameters
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE
 * @sa AM_IPC_MW_CMD_VIDEO_OVERLAY_SET
 * @sa am_overlay_area_s
 */
#define AM_IPC_MW_CMD_VIDEO_OVERLAY_GET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_VIDEO_OVERLAY_GET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_VIDEO)
/*! @} */ /* End of OVERLAY Commands */

/*! * @} */ /* End of Video Service Command IDs */
/******************************************************************************/

/*! @example test_video_service.cpp
 *  This is the example program of Video Service APIs.
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_VIDEO_H_ */
