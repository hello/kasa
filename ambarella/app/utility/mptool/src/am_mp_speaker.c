/*******************************************************************************
 * am_mp_speaker.cpp
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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "am_mp_server.h"
#include "am_mp_speaker.h"

enum {
  MP_SPEAKER_HAND_SHAKE = 0,
  MP_SPEAKER_PLAY,
  MP_SAVE_RESULT
};

am_mp_err_t mptool_speaker_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  int32_t status;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case MP_SPEAKER_HAND_SHAKE:
      break;
    case MP_SPEAKER_PLAY:
      // playback audio data
      status = system("/usr/bin/aplay -f dat /usr/local/bin/test_speaker.wav &");
      if (WEXITSTATUS(status)) {
        printf("result verify: aplay failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_SAVE_RESULT:
      system("kill -9 `pidof aplay`");
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret,
                           SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_NOT_SUPPORT;
        printf("save failed!\n");
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
