/*******************************************************************************
 * am_result.h
 *
 * History:
 *   Jul 28, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_INCLUDE_UTILITY_AM_RESULT_H_
#define ORYX_INCLUDE_UTILITY_AM_RESULT_H_

enum AM_RESULT
{
  AM_RESULT_OK                   = 0,   //!< Success
  AM_RESULT_ERR_INVALID          = -1,  //!< Invalid argument
  AM_RESULT_ERR_PERM             = -2,  //!< Operation is not permitted
  AM_RESULT_ERR_BUSY             = -3,  //!< Too busy to handle
  AM_RESULT_ERR_AGAIN            = -4,  //!< Not ready now, need to try again
  AM_RESULT_ERR_NO_ACCESS        = -5,  //!< Not allowed to access
  AM_RESULT_ERR_DSP              = -6,  //!< DSP related error
  AM_RESULT_ERR_MEM              = -7,  //!< Memory related error
  AM_RESULT_ERR_IO               = -8,  //!< I/O related error
  AM_RESULT_ERR_DATA_POINTER     = -9,  //!< Invalid data pointer
  AM_RESULT_ERR_STREAM_ID        = -10, //!< Invalid stream ID
  AM_RESULT_ERR_BUFFER_ID        = -11, //!< Invalid source buffer ID
  AM_RESULT_ERR_FILE_EXIST       = -12, //!< Directory or file does not exist
  AM_RESULT_ERR_PLATFORM_SUPPORT = -13, //!< Not supported by platform
  AM_RESULT_ERR_PLUGIN_LOAD      = -14, //!< Plugin is not loaded
  AM_RESULT_ERR_MODULE_STATE     = -15, //!< Module is not create or is disabled
  AM_RESULT_ERR_ALREADY          = -17, //!< Operation already in progress
  AM_RESULT_ERR_CONN_REFUSED     = -18, //!< Connection refused by server
  AM_RESULT_ERR_NO_CONN          = -19, //!< No connection to server
  AM_RESULT_ERR_SERVER_DOWN      = -20, //!< Server is down
  AM_RESULT_ERR_TIMEOUT          = -21, //!< Operate is timeout

  AM_RESULT_ERR_UNKNOWN          = -99, //!< Undefined error
  AM_RESULT_ERR_USER_BEGIN       = -100,
  AM_RESULT_ERR_USER_END         = -255
};

#endif /* ORYX_INCLUDE_UTILITY_AM_RESULT_H_ */
