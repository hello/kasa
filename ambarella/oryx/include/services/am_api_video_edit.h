/*
 * am_api_video_edit.h
 *
 * @Author: Zhi He
 * @Email : zhe@ambarella.com
 * @Time  : 04/28/2016 [Created]
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
/*! @file am_api_video_edit.h
 *  @brief This file defines Video Edit Service related data structures
 */
#ifndef __AM_API_VIDEO_EDIT_H__
#define __AM_API_VIDEO_EDIT_H__

#include "commands/am_api_cmd_video_edit.h"

/*! @enum AM_VIDEO_EDIT_SERVICE_RETCODE
 *  @brief Defines video edit service return code
 */
enum AM_VIDEO_EDIT_SERVICE_RETCODE
{
  AM_VIDEO_EDIT_SERVICE_RETCODE_OK = 0, //!< OK

  AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS = -1, //!< invalid parameters
  AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_STATE = -2, //!< bad state
  AM_VIDEO_EDIT_SERVICE_RETCODE_INSTANCE_BUSY = -3, //!< instance busy
  AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_INPUT = -4, //!< bad input
  AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_OUTPUT = -5, //!< bad output
  AM_VIDEO_EDIT_SERVICE_RETCODE_NO_RESOURCE = -6, //!< no resource
  AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL = -7, //!< fail
  AM_VIDEO_EDIT_SERVICE_RETCODE_NOT_AVIALABLE = -8,//!<not available
};

#define AM_VIDEO_EDIT_MAX_URL_LENGTH 384
#define AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER 1

/*! @defgroup airapi-datastructure-videoedit Data Structure of Video Edit Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Video Edit Service related method call data structures
 *  @{
 */

/*!
 * @struct am_video_edit_context_t
 * @brief Define video edit context
 */
struct am_video_edit_context_t
{
  /*! video edit instance id ... */
  uint8_t instance_id;

  /*! EFM stream index ... */
  uint8_t stream_index;

  uint8_t reserved1;
  uint8_t reserved2;

  /*! input url, can be .mjpeg, .h264, .h265 ... */
  char input_url[AM_VIDEO_EDIT_MAX_URL_LENGTH];

  /*! output url, can be .mp4, .h264 ... */
  char output_url[AM_VIDEO_EDIT_MAX_URL_LENGTH];
};

/*!
 * @struct am_video_edit_instance_id_t
 * @brief Define video edit instance id
 */
struct am_video_edit_instance_id_t
{
  /*! video edit instance id ... */
  uint8_t instance_id;

  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*! @enum VE_APP_MSG
 * Defines all video edit APP msg
 */
enum VE_APP_MSG
{
  VE_APP_MSG_FINISH = 0,    //!< Video edit finish msg
};

/*!
 * @struct am_video_edit_msg_t
 * @brief Define video edit msg
 */
struct am_video_edit_msg_t
{
  uint8_t instance_id;

  /*!
   * msg type.
   * @sa VE_APP_MSG
   */
  uint8_t msg_type;

  uint8_t reserved0;
  uint8_t reserved1;
};

/*! @} */

#endif
