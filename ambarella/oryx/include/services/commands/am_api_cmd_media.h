/*******************************************************************************
 * am_api_cmd_media.h
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

  //! _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START
  _AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,

  //! _AM_IPC_MW_CMD_MEDIA_NOTIF_ENGINE_MSG
  _AM_IPC_MW_CMD_MEDIA_NOTIF_ENGINE_MSG,

  //! _AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE
  _AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,

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
};

/*****************************Media Service CMDs*******************************/
/*! @defgroup airapi-commandid-media Air API Command IDs - Media Service
 *  @ingroup airapi-commandid
 *  @brief Media Service Related command IDs,
 *         refer to @ref airapi-datastructure-media
 *         "Data Structure of Media Service" to see data structures
 *  @{
 */

/*! @brief Start event recording.
 *
 * Use this command to start event recording,
 * and it will record a event file in specified path
 * when receives AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START command.
 */
#define AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Add audio file to media service.
 *
 * Use this command to add audio file to media service. The media service
 * will play these audio files when receives
 * AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE command.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 * @sa am_api_playback_audio_file_list_t
 */
#define AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE                                     \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,                \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Start to play audio file.
 *
 * Use this command to tell media service to start to play audio files which
 * added by AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE command.
 * @sa AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief Pause audio playback.
 *
 * Use this command to tell media service to pause the audio playback.
 * If the media service is in a pause mode, it will continue playing
 * the audio when receives AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 * command.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop playing audio file.
 *
 * Use this command to tell media service to stop playing audio file.
 * @sa AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief start media recording.
 *
 * Use this command to tell media service to start media recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_START_RECORDING                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_RECORDING,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop media recording.
 *
 * Use this command to tell media service to stop media recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_RECORDING                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief start file recording.
 *
 * Use this command to tell media service to start file recording.
 * @sa AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*! @brief stop file recording.
 *
 * Use this command to tell media service to stop file recording.
 * @sa AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING
 */
#define AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_MEDIA)

/*!
 * @}
 */
/******************************************************************************/

/*! @example test_media_service.cpp
 *  This is the example program of Media Service APIs.
 */
#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_MEDIA_H_ */
