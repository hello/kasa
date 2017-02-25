/*******************************************************************************
 * am_api_cmd_media.h
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
/*! @file am_api_cmd_media.h
 *  @brief This file defines Media Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_MEDIA_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_MEDIA_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_MEDIA
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_MEDIA
{
  //! _AM_IPC_MW_CMD_MEDIA_PARAMETER_GET
  _AM_IPC_MW_CMD_MEDIA_PARAMETER_GET = MEDIA_SERVICE_CMD_START,

  //! _AM_IPC_MW_CMD_MEDIA_PARAMETER_SET
  _AM_IPC_MW_CMD_MEDIA_PARAMETER_SET,

  //! _AM_IPC_MW_CMD_MEDIA_START_FILE_MUXER
  _AM_IPC_MW_CMD_MEDIA_START_FILE_MUXER,

  //! _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START
  _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,

  //! _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP
  _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP,

  //! _AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING
  _AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING,

  //! _AM_IPC_MW_CMD_MEDIA_NOTIF_ENGINE_MSG
  _AM_IPC_MW_CMD_MEDIA_NOTIF_ENGINE_MSG,

  //! _AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE
  _AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,

  //! _AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID
  _AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID,

  //! _AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID
   _AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID,

  //! _AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN
  _AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN,

  //! _AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
  _AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,

  //! _AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE
  _AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,

  //! _AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE
  _AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,

  //!_AM_IPC_MW_CMD_MEDIA_START_RECORDING
  _AM_IPC_MW_CMD_MEDIA_START_RECORDING,

  //!_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING
  _AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,

  //!_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING
  _AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,

  //!_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING
  _AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,

  //!_AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM
  _AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM,

  //!_AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION
  _AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION,

  //!_AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION
  _AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION,

  //!_AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM
  _AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM,

  //!_AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC
  _AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC,

  //!_AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
  _AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,

  //!_AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO,
  _AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO,

  //!_AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE,
  _AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE,
};

/*****************************Media Service CMDs*******************************/
/*! @defgroup airapi-commandid-media Air API Command IDs - Media Service
 *  @ingroup airapi-commandid
 *  @brief Media Service Related command IDs,
 *         refer to @ref airapi-datastructure-media
 *         "Data Structure of Media Service" to see data structures
 *  @{
 */

/*! @brief Start file muxer.
 *
 * Use this command to start file muxer,
 */
#define AM_IPC_MW_CMD_MEDIA_START_FILE_MUXER                               \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_FILE_MUXER,          \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Start event recording.
 *
 * Use this command to start event recording,
 * and it will record a event file in specified path
 * when receives AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START command.
 */
#define AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,     \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Stop event recording.
 *
 * Use this command to stop event recording.
 */
#define AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP,      \
                               AM_IPC_DIRECTION_DOWN,                      \
                               AM_IPC_NEED_RETURN,                         \
                               AM_SERVICE_TYPE_MEDIA)
/*! @brief Set periodic jpeg recording.
 *
 * Use this command to set periodic jpeg recording,
 * and it will record a sequence of jpeg in a specified file
 * according to the parameter.
 */
#define AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING,   \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Get playback instance id.
 * Use this command to get playback instance id.
 */
#define AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID, \
                           AM_IPC_DIRECTION_DOWN,                         \
                           AM_IPC_NEED_RETURN,                            \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Release playback instance id.
 *
 * Use this command to tell media service to release playback instance id which
 * got  by AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID command.
 * @sa AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID
 */
#define AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID, \
                           AM_IPC_DIRECTION_DOWN,                             \
                           AM_IPC_NEED_RETURN,                                \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Add audio file to media service.
 *
 * Use this command to add audio file to media service. The media service
 * will play these audio files when receives
 * AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE command.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 * @sa am_api_playback_audio_file_list_t
 */
#define AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,            \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Add audio playback unix domain info to media service.
 *
 * Use this command to add unix domain info to media service. The media service
 * will playback the audio data which is received from unix domain.
 */
#define AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN                          \
    BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN, \
                               AM_IPC_DIRECTION_DOWN,                         \
                               AM_IPC_NEED_RETURN,                            \
                               AM_SERVICE_TYPE_MEDIA)

/*! @brief Start to play audio file.
 *
 * Use this command to tell media service to start to play audio files which
 * added by AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE command.
 * @sa AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE, \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Pause audio playback.
 *
 * Use this command to tell media service to pause the audio playback.
 * If the media service is in a pause mode, it will continue playing
 * the audio when receives AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 * command.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE                      \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE, \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop playing audio file.
 *
 * Use this command to tell media service to stop playing audio file.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE                       \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,  \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief start media recording.
 *
 * Use this command to tell media service to start media recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_START_RECORDING                                \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_RECORDING,           \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop media recording.
 *
 * Use this command to tell media service to stop media recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_RECORDING                                 \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,            \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief start file recording.
 *
 * Use this command to tell media service to start file recording.
 * @sa AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,      \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop file recording.
 *
 * Use this command to tell media service to stop file recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,       \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief set recording file num.
 *
 * Use this command to set recording file num.
 */
#define AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM,    \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)
/*! @brief set recording duration.
 *
 * Use this command to set recording duration.
 */
#define AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION,    \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief set file duration.
 *
 * Use this command to set file duration.
 */
#define AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION,         \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief set muxer parameters.
 *
 * Use this command to set muxer parameters.
 */
#define AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM,         \
                           AM_IPC_DIRECTION_DOWN,                        \
                           AM_IPC_NEED_RETURN,                           \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief enable audio codec.
 *
 * Use this command to enable or disable audio codec.
 */
#define AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC                             \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC,        \
                           AM_IPC_DIRECTION_DOWN,                          \
                           AM_IPC_NEED_RETURN,                             \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Set file operation callback function.
 *
 * Use this command to set file operation callback function.
 */
#define AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK                           \
    BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,  \
                               AM_IPC_DIRECTION_DOWN,                             \
                               AM_IPC_NEED_RETURN,                                \
                               AM_SERVICE_TYPE_MEDIA)

/*! @brief Update 3A info to media service
 *
 * Use this command to update 3A info to media service.
 */
#define AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO                                 \
    BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO,        \
                               AM_IPC_DIRECTION_DOWN,                      \
                               AM_IPC_NEED_RETURN,                         \
                               AM_SERVICE_TYPE_MEDIA)

/*! @brief Reload record engine
 *
 * Use this command to reload record engine.
 */
#define AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE                           \
    BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE,  \
                               AM_IPC_DIRECTION_DOWN,                      \
                               AM_IPC_NEED_RETURN,                         \
                               AM_SERVICE_TYPE_MEDIA)


/*!
 * @}
 */
/******************************************************************************/

/*! @example test_media_service.cpp
 *  This is the example program of Media Service APIs.
 */
#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_MEDIA_H_ */
