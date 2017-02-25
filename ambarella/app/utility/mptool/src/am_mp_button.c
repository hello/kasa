/*******************************************************************************
 * am-mp_button.cpp
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
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>
#include <sys/select.h>
#include <sys/time.h>
#include "am_mp_server.h"
#include "am_mp_button.h"


#define INPUT_DEV "/dev/input/event0"

enum {
  BUTTON_STATE_GET    = 0,
  BUTTON_RESULT_SAVE  = 1
};

typedef enum{
  success   = 0,
  failed    = 1,
  not_test  = 2
}button_err;

typedef struct{
    int32_t id;
    button_err state;
}mp_button_msg;

static int32_t key_cmd_process(struct input_event *cmd, int32_t gpioid)
{
  int32_t ret = -1;

  do {
    if (cmd->type != EV_KEY) {
      ret = 0;
      break;
    }

    if (cmd->code != gpioid) {
      printf("Pressed the wrong button! event.code: %d\n", cmd->code);
      if (cmd->value == 0) {
        break;
      }
      ret = 0;
      break;
    }

    if (cmd->value) {
      printf(" == Key [%d] is pressed ==\n", gpioid);
      ret = 0;
      break;
    } else {
      printf(" == Key [%d] is released ==\n", gpioid);
      ret = 1;
      break;
    }
  } while (0);

  return ret;
}

static int32_t get_button_state(mp_button_msg *button_msg,
                                am_mp_msg_t *to_msg)
{
  int32_t ret = 0;
  struct timeval timeout;
  int32_t event_fd = -1;
  fd_set rfds;
  int32_t retv = -1;
  struct input_event event;

  do {
    timeout.tv_sec = 20;
    timeout.tv_usec = 0;

    event_fd = open(INPUT_DEV, O_RDONLY);
    if (event_fd < 0) {
      perror("Open input/event0 error\n");
      to_msg->result.ret = MP_ERROR;
      ret = -1;
      break;
    }

    FD_ZERO(&rfds);
    FD_SET(event_fd, &rfds);
    while (1) {
      retv = select(FD_SETSIZE, &rfds, NULL, NULL, &timeout);
      if (retv < 0) {
        perror("select");
        ret = -1;
        break;
      } else if (retv == 0) {
        printf("select timeout!\n");
        ret = -1;
        break;
      } else {
        if (FD_ISSET(event_fd, &rfds)) {
          if (read(event_fd, &event, sizeof(event)) < 0) {
            button_msg->state = failed;
            perror("Read event_fd error\n");
            ret = -1;
            break;
          }

          retv = key_cmd_process(&event, button_msg->id);
          if (retv < 0) {
            button_msg->state = failed;
            ret = -1;
            break;
          }
        } else {
          printf("FD is not in SET.\n");
          ret = -1;
          break;
        }
      }

      if (retv == 1) {
        button_msg->state = success;
        break;
      }
    }
  } while (0);

  if (event_fd > 0) {
    close(event_fd);
    event_fd = -1;
  }

  return ret;
}

am_mp_err_t mptool_button_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  mp_button_msg button_msg;

  memcpy(&button_msg, from_msg->msg, sizeof(button_msg));
  button_msg.state = not_test;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case BUTTON_STATE_GET:
      if (get_button_state(&button_msg, to_msg)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case BUTTON_RESULT_SAVE:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret,
                           SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  memcpy(to_msg->msg, &button_msg, sizeof(button_msg));

  return MP_OK;
}
