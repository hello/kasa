/*
 * am_api_audio.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 02/12/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
/*! @file am_api_audio.h
 *  @brief This file defines Audio Service related data structures
 */
#ifndef __AM_API_AUDIO_H__
#define __AM_API_AUDIO_H__

#include "commands/am_api_cmd_audio.h"

/*! @defgroup airapi-datastructure-audio Data Structure of Audio Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Audio Service related method call data structures
 *  @{
 */

/*! @defgroup airapi-datastructure-audio-format Data Structure of Audio Format
 *  @ingroup airapi-datastructure-audio
 *  @brief Audio Format related data structures,
 *         refer to @ref airapi-commandid-audio-format "Audio Format Commands"
 *         to see related commands.
 *  @{
 */

/*!
 * @struct am_audio_format_t
 * @brief Define audio information
 * @sa AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET
 * @sa AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET
 * @sa AM_IPC_MW_CMD_AUDIO_CHANNEL_GET
 * @sa AM_IPC_MW_CMD_AUDIO_CHANNEL_SET
 */
struct am_audio_format_t
{
  /*! sample rate: 48K, 44.1K ... */
  uint32_t sample_rate;

  /*! sample bit count: 8, 16, 24, 32. */
  uint8_t sample_bit_count;

  /*!
   * sample format: refer to PulseAudio library.
   *
   * - 0  : Unsigned 8 bit PCM
   * - 1  : 8 Bit a-Law
   * - 2  : 8 Bit mu-Law
   * - 3  : Signed 16 Bit PCM, little endian (PC)
   * - 4  : Signed 16 Bit PCM, big endian
   * - 5  : 32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0
   * - 6  : 32 Bit IEEE floating point, big endian, range -1.0 to 1.0
   * - 7  : Signed 32 Bit PCM, little endian (PC)
   * - 8  : Signed 32 Bit PCM, big endian
   * - 9  : Signed 24 Bit PCM packed, little endian (PC)
   * - 10 : Signed 24 Bit PCM packed, big endian
   * - 11 : Signed 24 Bit PCM in LSB of 32 Bit words, little endian
   * - 12 : Signed 24 Bit PCM in LSB of 32 Bit words, big endian
   */
  uint8_t sample_format;

  /*! - 1: mono
   *  - 2: stereo
   */
  uint8_t channels;

  /*! reserved: just for alignment */
  uint8_t reserved;
};
/*! @} */

/*! @defgroup airapi-datastructure-audio-device Data Structure of Audio Device
 *  @ingroup airapi-datastructure-audio
 *  @brief Audio Device related data structures,
 *         refer to @ref airapi-commandid-audio-device "Audio Device Commands"
 *         to see related commands.
 *  @{
 */
#ifndef AM_AUDIO_DEVICE_NAME_MAX_LEN
/*!
 * Audio device name maximum length
 */
#define AM_AUDIO_DEVICE_NAME_MAX_LEN (128)
#endif

#ifndef AM_AUDIO_DEVICE_NUM_MAX
/*!
 * Audio device maximum number
 */
#define AM_AUDIO_DEVICE_NUM_MAX (16)
#endif
/*!
 * @struct am_audio_device_t
 * @brief Define audio device information
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET
 * @sa AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX
 */
struct am_audio_device_t
{
  /*! device's index */
  uint32_t index;

  /*!
   * device's type
   *
   * - 0: mic
   * - 1: speaker
   * - 2: playback stream
   * - 3: record stream
   */
  uint8_t type;

  /*!
   * volume: mic's volume
   *
   * - minimum volume: 0
   * - maximum volume: 100
   */
  uint8_t volume;

  /*!
   * This property is just used by mic.
   *
   * - 0: alc is not enabled, use static gain setting;
   * - 1: use alc setting (default)
   */
  uint8_t alc_enable;

  /*!
   * - 0: unmute
   * - 1: mute
   */
  uint8_t mute;

  /*!
   * device's name
   *
   * maximum length is defined by the macro AUDIO_DEVICE_NAME_MAX_LEN
   */
  char name[AM_AUDIO_DEVICE_NAME_MAX_LEN];
};
/*! @} */
/*! @} */
#endif
