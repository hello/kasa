/*******************************************************************************
 * am_mp_server.h
 *
 * History:
 *   Mar 18, 2015 - [longli] created file
 *
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
 */


#ifndef _AM_MP_SERVER_H_
#define _AM_MP_SERVER_H_

#define SYNC_FILE 1
#define NOT_SYNC_FILE 0
#define BUF_LEN 128
#define VBUF_LEN 4
#define EXTRA_MSG_FILE "/etc/mptool/mptool_extra_msg.txt"

typedef enum {
  MP_OK = 0,
  MP_WAIT,
  MP_ERROR,
  MP_CLOSED,
  MP_BUSY,
  MP_INTERNAL_ERROR,
  MP_NO_RUN,
  MP_NO_IMPL,
  MP_OS_ERROR,
  MP_HW_ERROR,
  MP_IO_ERROR,
  MP_TIMEOUT,
  MP_NO_MEMORY,
  MP_TOO_MANY,

  MP_NOT_EXIST,
  MP_NOT_SUPPORT,
  MP_NO_INTERFACE,
  MP_BAD_STATE,
  MP_BAD_PARAM,
  MP_BAD_COMMAND,

  MP_UNKNOWN_FMT,
} am_mp_err_t;

typedef enum {
  AM_MP_ETHERNET = 0,
  AM_MP_BUTTON,
  AM_MP_LED,
  AM_MP_IRLED,
  AM_MP_LIGHT_SENSOR,
  AM_MP_MAC_ADDR,
  AM_MP_WIFI_STATION,
  AM_MP_MIC,
  AM_MP_SPEAKER,
  AM_MP_SENSOR,
  AM_MP_LENS_FOCUS,
  AM_MP_LENS_SHADING_CALIBRATION,
  AM_MP_BAD_PIXEL_CALIBRATION,
  AM_MP_FW_INFO,
  AM_MP_SDCARD,
  AM_MP_IRCUT,
  AM_MP_MAX_ITEM_NUM, //all added item must before this item
  AM_MP_COMLETE_TEST,
  AM_MP_RESET_ALL,
  AM_MP_TYPE_INIT = 255,
} am_mp_msg_type_t;

typedef struct {
    am_mp_msg_type_t type;
    am_mp_err_t ret;
} am_mp_result_t;

typedef struct{
    int32_t stage;
    am_mp_result_t result;
    char msg[256];
} am_mp_msg_t;

typedef am_mp_err_t (*am_mptool_handler) (am_mp_msg_t*, am_mp_msg_t*);

am_mp_err_t mptool_save_data(am_mp_msg_type_t mp_type,
                             am_mp_err_t mp_result,
                             int32_t sync_flag);
#endif
