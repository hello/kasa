/*******************************************************************************
 * am_api_cmd_playback.h
 *
 * History:
 *   2016-04-14 - [Zhi He] created file
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
/*! @file am_api_cmd_playback.h
 *  @brief This file defines Playback Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_PLAYBACK_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_PLAYBACK_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_PLAYBACK
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_PLAYBACK
{
  //! _AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE
  _AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE = PLAYBACK_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE
  _AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE,

  //! _AM_IPC_MW_CMD_PLAYBACK_START_PLAY
  _AM_IPC_MW_CMD_PLAYBACK_START_PLAY,

  //! _AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY
  _AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY,

  //! _AM_IPC_MW_CMD_PLAYBACK_PAUSE
  _AM_IPC_MW_CMD_PLAYBACK_PAUSE,

  //! _AM_IPC_MW_CMD_PLAYBACK_RESUME
  _AM_IPC_MW_CMD_PLAYBACK_RESUME,

  //! _AM_IPC_MW_CMD_PLAYBACK_STEP
  _AM_IPC_MW_CMD_PLAYBACK_STEP,

  //! _AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION
  _AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION,

  //! _AM_IPC_MW_CMD_PLAYBACK_SEEK
  _AM_IPC_MW_CMD_PLAYBACK_SEEK,

  //! _AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD
  _AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD,

  //! _AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD
  _AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD,

  //! _AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN
  _AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN,

  //! _AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END
  _AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END,

  //! _AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT
  _AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT,

  //! _AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT
  _AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT,
};

/*****************************Playback Service CMDs*******************************/
/*! @defgroup airapi-commandid-playback Air API Command IDs - Playback Service
 *  @ingroup airapi-commandid
 *  @brief Playback Service Related command IDs,
 *         refer to @ref airapi-datastructure-playback
 *         "Data Structure of Playback Service" to see data structures
 *  @{
 */

/*! @brief Check DSP Mode
 *
 *  Use this command to check if DSP is ready to playback.
 *  @sa AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE
 */
#define AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief Exit DSP Playback Mode
 *
 *  Use this command to exit DSP playback mode.
 *  @sa AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE
 */
#define AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief start play
 *
 *  Use this command to start playback, need to specify prefer modules, output device, and initial playback speed/scan mode/direction
 *  @sa AM_IPC_MW_CMD_PLAYBACK_START_PLAY
 *  @sa am_playback_start_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_START_PLAY                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_START_PLAY,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief stop play
 *
 *  Use this command to stop playback
 *  @sa AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY
 */
#define AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief pause play
 *
 *  Use this command to pause play
 *  @sa AM_IPC_MW_CMD_PLAYBACK_PAUSE
 */
#define AM_IPC_MW_CMD_PLAYBACK_PAUSE                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_PAUSE,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief resume play
 *
 *  Use this command to resume play
 *  @sa AM_IPC_MW_CMD_PLAYBACK_RESUME
 */
#define AM_IPC_MW_CMD_PLAYBACK_RESUME                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_RESUME,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief step play
 *
 *  Use this command to step play
 *  @sa AM_IPC_MW_CMD_PLAYBACK_STEP
 */
#define AM_IPC_MW_CMD_PLAYBACK_STEP                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_STEP,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief query playback position
 *
 *  Use this command to query playback position
 *  @sa AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION
 *  @sa am_playback_position_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief seek
 *
 *  Use this command to seek
 *  @sa AM_IPC_MW_CMD_PLAYBACK_SEEK
 *  @sa am_playback_seek_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_SEEK                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_SEEK,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief fast forward play
 *
 *  Use this command to fast forward play
 *  @sa AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD
 *  @sa am_playback_fast_forward_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief fast backward play
 *
 *  Use this command to fast backward play
 *  @sa AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD
 *  @sa am_playback_fast_backward_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief fast forward play from begin
 *
 *  Use this command to fast forward play from begin
 *  @sa AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN
 *  @sa am_playback_fast_forward_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief fast backward play from end
 *
 *  Use this command to fast backward play from end
 *  @sa AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END
 *  @sa am_playback_fast_backward_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief configure vout
 *
 *  Use this command to configure vout
 *  @sa AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT
 *  @sa am_playback_vout_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT                                  \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT,             \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_PLAYBACK)

/*! @brief halt vout
 *
 *  Use this command to halt vout
 *  @sa AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT
 *  @sa am_playback_vout_t
 */
#define AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT                                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT,                 \
                           AM_IPC_DIRECTION_DOWN,                             \
                           AM_IPC_NEED_RETURN,                                \
                           AM_SERVICE_TYPE_PLAYBACK)

/*!
 *  @}
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_PLAYBACK_H_ */
