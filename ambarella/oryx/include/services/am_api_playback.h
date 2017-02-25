/*
 * am_api_playback.h
 *
 * @Author: Zhi He
 * @Email : zhe@ambarella.com
 * @Time  : 04/14/2016 [Created]
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
 */
/*! @file am_api_playback.h
 *  @brief This file defines Playback Service related data structures
 */
#ifndef __AM_API_PLAYBACK_H__
#define __AM_API_PLAYBACK_H__

#include "commands/am_api_cmd_playback.h"

/*! @enum AM_PLAYBACK_SERVICE_RETCODE
 *  @brief Defines playback service return code
 */
enum AM_PLAYBACK_SERVICE_RETCODE
{
  AM_PLAYBACK_SERVICE_RETCODE_OK = 0, //!< OK

  AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS = -1, //!< invalid parameters
  AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE = -2, //!< bad state
  AM_PLAYBACK_SERVICE_RETCODE_INSTANCE_BUSY = -3, //!< instance busy
  AM_PLAYBACK_SERVICE_RETCODE_NO_RESOURCE = -4, //!< no resource
  AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL = -5, //!< fail
  AM_PLAYBACK_SERVICE_RETCODE_NOT_SUPPORT = -6, //!< not support
  AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE = -7, //!< not support
};

#define AM_PLAYBACK_MAX_URL_LENGTH 512
#define AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER 1

/*! @defgroup airapi-datastructure-playback Data Structure of Playback Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Playback Service related method call data structures
 *  @{
 */

/*!
 * @struct am_playback_context_t
 * @brief Define playback context
 */
struct am_playback_context_t
{
  /*!
   * playback speed, use 8.8 fix point format
   *
   */
  uint16_t playback_speed;

  /*! playback instance id ... */
  uint8_t instance_id;

  /*! use HDMI output ... */
  uint8_t use_hdmi;

  /*! use Digital output ... */
  uint8_t use_digital;

  /*! use CVBS output ... */
  uint8_t use_cvbs;

  /*!
   * prefer demuxer module for playback.
   *
   * - 'PRMP4'  : native mp4 demuxer
   * - 'RTSP'  : native rtsp demuxer
   * - 'FFMpeg'  : use FFMpeg library for demux
   * - 'AUTO'  : auto select
   */
  char prefer_demuxer[8];

  /*!
   * prefer video decoder module for playback.
   *
   * - 'AmbaDSP'  : use Ambarella DSP for video decoding
   * - 'FFMpeg'  : use FFMpeg library for video decoding
   * - 'AUTO'  : auto select
   */
  char prefer_video_decoder[8];

  /*!
   * prefer video output module for playback.
   *
   * - 'AmbaDSP'  : use Ambarella DSP for video output
   * - 'LinuxFB'  : use Linux Frame Buffer for video output
   * - 'AUTO'  : auto select
   */
  char prefer_video_output[8];

  /*!
   * prefer audio decoder module for playback.
   *
   * - 'LibAAC'  : native Libaac for decoding
   * - 'FFMpeg'  : use FFMpeg library for decoding
   * - 'AUTO'  : auto select
   */
  char prefer_audio_decoder[8];

  /*!
   * prefer audio output module for playback.
   *
   * - 'ALSA'  : use ALSA for sound output
   * - 'AUTO'  : auto select
   */
  char prefer_audio_output[8];

  /*!
   * playback scan mode.
   *
   * - 0  : all frames
   * - 1  : I frame only
   */
  uint8_t scan_mode;

  /*!
   * playback direction.
   *
   * - 0  : forward direction
   * - 1  : backward direction
   */
  uint8_t playback_direction;

  /*! playback url, can be local file or streaming url ... */
  char url[AM_PLAYBACK_MAX_URL_LENGTH];
};

/*!
 * @struct am_playback_instance_id_t
 * @brief Define playback instance id
 */
struct am_playback_instance_id_t
{
  /*! playback instance id ... */
  uint8_t instance_id;

  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*!
 * @struct am_playback_position_t
 * @brief Define playback position
 */
struct am_playback_position_t
{
  /*! playback length ... */
  uint64_t length;

  /*! playback current position ... */
  uint64_t cur_position;

  /*! playback instance id ... */
  uint8_t instance_id;

  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*!
 * @struct am_playback_seek_t
 * @brief Define playback seek's parameter
 */
struct am_playback_seek_t
{
  /*! seek target ... */
  uint64_t target;

  /*! playback instance id ... */
  uint8_t instance_id;

  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*!
 * @struct am_playback_fast_forward_t
 * @brief Define playback fast forward's parameter
 */
struct am_playback_fast_forward_t
{
  /*!
   * playback speed, use 8.8 fix point format
   *
   */
  uint16_t playback_speed;

  /*! playback instance id ... */
  uint8_t instance_id;

  /*!
   * playback scan mode.
   *
   * - 0  : all frames
   * - 1  : I frame only
   */
  uint8_t scan_mode;
};

/*!
 * @struct am_playback_fast_backward_t
 * @brief Define playback fast backward's parameter
 */
struct am_playback_fast_backward_t
{
  /*!
   * playback speed, use 8.8 fix point format
   *
   */
  uint16_t playback_speed;

  /*! playback instance id ... */
  uint8_t instance_id;

  /*!
   * playback scan mode.
   *
   * - 0  : all frames
   * - 1  : I frame only
   */
  uint8_t scan_mode;
};

/*!
 * @struct am_playback_vout_t
 * @brief Define playback vout's parameter
 */
struct am_playback_vout_t
{
  /*! playback instance id ... */
  uint8_t instance_id;

  /*!
   * VOUT 0 would be digital (LCD), VOUT 1 can be HDMI or CVBS
   *
   */
  uint8_t vout_id;

  uint8_t reserved0;
  uint8_t reserved1;

  /*!
   * vout mode string, like "1080p", "720p", etc.
   *
   */
  char vout_mode[32];

  /*!
   * vout sink type string, like "hdmi", "cvbs", "digital", etc.
   *
   */
  char vout_sinktype[32];

  /*!
   * vout device string.
   *
   */
  char vout_device[32];
};

/*! @} */

#endif
