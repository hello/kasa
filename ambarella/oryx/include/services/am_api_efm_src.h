/*
 * am_api_efm_src.h
 *
 * @Author: Zhi He
 * @Email : zhe@ambarella.com
 * @Time  : 05/04/2016 [Created]
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
/*! @file am_api_efm_src.h
 *  @brief This file defines EFM Source Service related data structures
 */
#ifndef __AM_API_EFM_SRC_H__
#define __AM_API_EFM_SRC_H__

#include "commands/am_api_cmd_efm_src.h"

/*! @enum AM_EFM_SRC_SERVICE_RETCODE
 *  @brief Defines EFM Source service return code
 */
enum AM_EFM_SRC_SERVICE_RETCODE
{
  AM_EFM_SRC_SERVICE_RETCODE_OK = 0, //!< OK

  AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS = -1, //!< invalid parameters
  AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE = -2, //!< bad state
  AM_EFM_SRC_SERVICE_RETCODE_INSTANCE_BUSY = -3, //!< instance busy
  AM_EFM_SRC_SERVICE_RETCODE_BAD_INPUT = -4, //!< bad input
  AM_EFM_SRC_SERVICE_RETCODE_BAD_OUTPUT = -5, //!< bad output
  AM_EFM_SRC_SERVICE_RETCODE_NO_RESOURCE = -6, //!< no resource
  AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL = -7, //!< fail
  AM_EFM_SRC_SERVICE_RETCODE_NOT_AVIALABLE = -8,//!<not available
};

#define AM_EFM_SRC_MAX_URL_LENGTH 384

/*! @enum AM_EFM_SRC_SERVICE_INSTANCE_ID
 *  @brief Defines EFM Source service instance id
 */
enum AM_EFM_SRC_SERVICE_INSTANCE_ID
{
  AM_EFM_SRC_SERVICE_INSTANCE_ID_FOR_USB_CAMERA = 0, //!< id for usb camera
  AM_EFM_SRC_SERVICE_INSTANCE_ID_FOR_OFFLINE_EDIT = 1, //!< id for offline edit
  AM_EFM_SRC_SERVICE_INSTANCE_ID_FOR_YUV_CAPTURE = 2, //!< id for yuv capture

  AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER,
};

/*! @enum AM_EFM_SRC_FORMAT
 *  @brief Defines EFM Source format
 */
enum AM_EFM_SRC_FORMAT
{
  AM_EFM_SRC_FORMAT_AUTO = 0, //!< auto

  AM_EFM_SRC_FORMAT_MJPEG = 1, //!< mjpeg
  AM_EFM_SRC_FORMAT_YUYV = 2, //!< YUYV format, YUV422
};

/*! @defgroup airapi-datastructure-EFM source Data Structure of EFM Source Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx EFM Source Service related method call data structures
 *  @{
 */

/*!
 * @struct am_efm_src_context_t
 * @brief Define EFM Source context
 */
struct am_efm_src_context_t
{
  /*! EFM source instance id ... */
  uint8_t instance_id;

  /*! EFM stream index ... */
  uint8_t stream_index;

  /*! dump efm source flag, debug option for analyze efm ... */
  uint8_t b_dump_efm_src;

  /*! prefer format, see AM_EFM_SRC_FORMAT */
  uint8_t prefer_format;

  /*! prefer video width ... */
  uint32_t prefer_video_width;

  /*! prefer video height ... */
  uint32_t prefer_video_height;

  /*! prefer video fps ... */
  uint32_t prefer_video_fps;

  /*! input url, can be local file like .mjpeg, .h264, .h265 or
   *  USB camera device ...
   */
  char input_url[AM_EFM_SRC_MAX_URL_LENGTH];

  /*! dump filename
   *   debug option for analyze efm
   */
  char dump_efm_src_url[AM_EFM_SRC_MAX_URL_LENGTH];

  /*! EFM source YUV source buffer id ... */
  uint8_t yuv_source_buffer_id;

  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*!
 * @struct am_efm_src_instance_id_t
 * @brief Define video edit instance id
 */
struct am_efm_src_instance_id_t
{
  /*! EFM source instance id ... */
  uint8_t instance_id;

  uint8_t number_of_yuv_frames;
  uint8_t reserved1;
  uint8_t reserved2;
};

/*! @} */

#endif
