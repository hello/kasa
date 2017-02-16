/*******************************************************************************
 * am_api_cmd_audio.h
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
/*! @file am_api_cmd_audio.h
 *  @brief This file defines Audio Service related commands
 */
#ifndef ORYX_INCLUDE_SERVICES_AM_API_CMD_AUDIO_H_
#define ORYX_INCLUDE_SERVICES_AM_API_CMD_AUDIO_H_

#include "commands/am_service_impl.h"

/*! @enum AM_SYS_IPC_MW_CMD_AUDIO
 *  @brief Used for system IPC basic function
 */
enum AM_SYS_IPC_MW_CMD_AUDIO
{
  //! _AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET
  _AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET = AUDIO_SERVICE_CMD_START,

  //! HW sample rate from audio codec
  _AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET,

  //! Get channel info mono(1) or stereo(2)
  _AM_IPC_MW_CMD_AUDIO_CHANNEL_GET,

  //! _AM_IPC_MW_CMD_AUDIO_CHANNEL_SET
  _AM_IPC_MW_CMD_AUDIO_CHANNEL_SET,

  //! capture volume of audio device by index
  _AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX,

  //! change  volume of audio device by name
  _AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX,

  //! capture volume of audio device by name
  _AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME,

  //! change  volume of audio device by index
  _AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME,

  //! test audio device is silent or not by index
  _AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX,

  //! let audio device be silent or normal by index
  _AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX,

  //! test audio device is silent or not by name
  _AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME,

  //! let audio device be silent or normal by name
  _AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME,

  //! get name of auido device or name by it's index
  _AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX,

  //! list indexes of audio device or stream with same type
  _AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET,

#if 0
  //! select between MIC and line-in
  _AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_GET,

  //! _AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_SET
  _AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_SET,

  //! acoustic echo cancellation
  _AM_IPC_MW_CMD_AUDIO_AEC_GET,

  //! _AM_IPC_MW_CMD_AUDIO_AEC_SET
  _AM_IPC_MW_CMD_AUDIO_AEC_SET,

  //! HPF, LPF and 3D effect and etc
  _AM_IPC_MW_CMD_AUDIO_EFFECT_GET,

  //! _AM_IPC_MW_CMD_AUDIO_EFFECT_SET
  _AM_IPC_MW_CMD_AUDIO_EFFECT_SET,
#endif
};

/*****************************Audio Service CMDs*******************************/
/*! @defgroup airapi-commandid-audio Air API Command IDs - Audio Service
 *  @ingroup airapi-commandid
 *  @brief Audio Service Related command IDs,
 *         refer to @ref airapi-datastructure-audio
 *         "Data Structure of Audio Service" to see data structures
 *  @{
 */

/*! @defgroup airapi-commandid-audio-format Audio Format Commands
 *  @ingroup airapi-commandid-audio
 *  @brief Audio Format related commands,
 *  see @ref airapi-datastructure-audio-format "Data Structure of Audio Format"
 *  for more information.
 *  @{
 */ /* Start of Audio Format Commands */

/*! @brief Get audio sample rate
 *
 *  Use this command to get current audio client audio sample rate.
 *  @sa AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET
 *  @sa am_audio_format_t
 */
#define AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio sample rate
 *
 *  Use this command to set current audio client audio sample rate.
 *  @sa AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET
 *  @sa am_audio_format_t
 */
#define AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET                                    \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET,               \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get audio channel number
 *
 *  Use this command to get current audio client audio channel number.
 *  @sa AM_IPC_MW_CMD_AUDIO_CHANNEL_SET
 *  @sa am_audio_format_t
 */
#define AM_IPC_MW_CMD_AUDIO_CHANNEL_GET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_CHANNEL_GET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio channel number
 *
 *  Use this command to set current audio client audio channel number.
 *  @sa AM_IPC_MW_CMD_AUDIO_CHANNEL_GET
 *  @sa am_audio_format_t
 */
#define AM_IPC_MW_CMD_AUDIO_CHANNEL_SET                                        \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_CHANNEL_SET,                   \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)
/*! @} */ /* End of Audio Format Commands */

/*! @defgroup airapi-commandid-audio-device Audio Device Commands
 *  @ingroup airapi-commandid-audio
 *  @brief Audio Device related commands,
 *  see @ref airapi-datastructure-audio-device "Data Structure of Audio Device"
 *  for more information.
 *  @{
 */  /* Start of Audio Device Commands */

/*! @brief Get audio device volume information.
 *
 *  Use this command to get audio device volume information
 *  by audio device index number, to list all the audio device,
 *  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX,    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio device volume information.
 *
 *  Use this command to set audio device volume information
 *  by audio device index number, to list all the audio device,
 *  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX,    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get audio device volume information.
 *
 *  Use this command to get audio device volume information
 *  by audio device name string, to list all the audio device,
 *  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio device volume information.
 *
 *  Use this command to set audio device volume information
 *  by audio device name string, to list all the audio device,
 *  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME                          \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME,     \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get audio device mute status.
 *
 * Use this command to get audio device mute status by device index,
 * to list all the audio device,
 * refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME
 * @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio device mute status.
 *
 * Use this command to set audio device mute status by device index,
 * to list all the audio device,
 * refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME
 * @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get audio device mute status.
 *
 * Use this command to get audio device mute status by device name string,
 * to list all the audio device,
 * refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET,
 * to find related device name string, refer to
 * AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 * @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME,       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Set audio device mute status.
 *
 * Use this command to set audio device mute status by device name string,
 * to list all the audio device,
 * refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET,
 * to find related device name string, refer to
 * @ref AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 * @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME,       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get Audio device index list
 *
 *  Use this command to list all the audio devices including\n
 *  - Sink: Audio output device
 *  - Source: Audio input device
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET                              \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET,         \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

/*! @brief Get Audio device name by device index
 *
 *  Use this command to get the audio device name string by device index.
 *  to list all the audio devices,
 *  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 *  @sa am_audio_device_t
 */
#define AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX                           \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX,      \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)
#if 0
#define AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_GET                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_GET,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

#define AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_SET                                   \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_SET,              \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)
#endif
/*! @} */ /* End of Audio Device Commands */

#if 0
/*! @defgroup airapi-commandid-audio-feature Audio Feature Commands
 *  @ingroup airapi-commandid-audio
 *  @brief Audio feature related commands, currently not implemented.
 *  @{
 */ /* Start of Audio Feature Commands */
#define AM_IPC_MW_CMD_AUDIO_AEC_GET                                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_AEC_GET,                       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

#define AM_IPC_MW_CMD_AUDIO_AEC_SET                                            \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_AEC_SET,                       \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

#define AM_IPC_MW_CMD_AUDIO_EFFECT_GET                                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_EFFECT_GET,                    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)

#define AM_IPC_MW_CMD_AUDIO_EFFECT_SET                                         \
BUILD_IPC_MSG_ID_WITH_TYPE(_AM_IPC_MW_CMD_AUDIO_EFFECT_SET,                    \
                           AM_IPC_DIRECTION_DOWN,                              \
                           AM_IPC_NEED_RETURN,                                 \
                           AM_SERVICE_TYPE_AUDIO)
/*! @} */ /* End of Audio Feature Commands */
#endif

/*!
 * @}
 */
/******************************************************************************/

/*! @example test_audio_service.cpp
 *  This is the example program of Audio Service APIs.
 */

#endif /* ORYX_INCLUDE_SERVICES_AM_API_CMD_AUDIO_H_ */
