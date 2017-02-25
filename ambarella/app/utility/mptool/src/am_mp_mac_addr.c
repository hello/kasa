/*******************************************************************************
 * am_mp_mac_addr.cpp
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_mac_addr.h"

enum {
  MP_MAC_HAND_SHAKE = 0,
  MP_MAC_SET,
  MP_SAVE_EXTRA_MSG,
  MP_SAVE_RESULT
};

static int32_t write_extra_msg(char* extra_msg)
{
  int32_t ret = 0;

  do {
    if(!extra_msg) {
      printf("extra_msg is NULL\n");
      ret = -1;
      break;
    }

    FILE *fd = NULL;

    if(!(fd = fopen(EXTRA_MSG_FILE, "w+"))) {
      perror("fopen");
      ret = -1;;
      break;
    }

    fprintf(fd,"%s",extra_msg);
    fclose(fd);

  } while (0);

  return ret;
}

static int32_t fetch_mac_addr(const char *client_msg, char *mac_addr)
{
  int32_t ret = 0;
  int32_t i, valid_c;
  char c;

  do {
    if (!client_msg || !mac_addr) {
      printf("Invalid parameter[%s: %d]\n",
              __PRETTY_FUNCTION__, __LINE__);
      ret = -1;
      break;
    }

    for (i = 0, valid_c = 0; client_msg[i] != '\0'; i++) {
        c = client_msg[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')) {
          mac_addr[valid_c++] = c;
        } else if (c >= 'A' && c <= 'F') {
          mac_addr[valid_c++] = 'a' + c - 'A';
        } else if (c == ':' || c == ' ') {
          continue;
        } else {
          printf("Invalid character %c has been found [%s: %d]\n",
                  c, __PRETTY_FUNCTION__, __LINE__);
          ret = -1;
          break;
        }
      }

    if (valid_c != 12) {
      printf("Invalid mac address [%s: %d]\n",
                  __PRETTY_FUNCTION__, __LINE__);
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

am_mp_err_t mptool_mac_addr_handler(am_mp_msg_t *from, am_mp_msg_t *to)
{
  int32_t status;
  char cmd[BUF_LEN];
  char mac_addr[13];

  to->result.ret = MP_OK;
  switch (from->stage) {
    case MP_MAC_HAND_SHAKE:
      break;

    case MP_MAC_SET:
      if (fetch_mac_addr (from->msg, mac_addr) < 0) {
        snprintf(cmd, sizeof(cmd) - 1, "Invalid mac address: %s" ,mac_addr);
        strcpy (to->msg, cmd);
        to->result.ret = MP_ERROR;
      } else {
        snprintf(cmd, sizeof(cmd) - 1,
                 "upgrade_partition --wifi0_mac %s", mac_addr);
        status = system(cmd);
        if (WEXITSTATUS(status)) {
          printf("Failed to use upgrade_partition"
              " to modify mac address of wifi!\n");
          strcpy (to->msg, "Failed to modify mac address of wifi");
          to->result.ret = MP_ERROR;
        } else {
          strcpy (to->msg, mac_addr);
        }
      }
      break;

    case MP_SAVE_EXTRA_MSG:
      if(write_extra_msg(from->msg) < 0) {
        to->result.ret = MP_ERROR;
      }
      break;

    case MP_SAVE_RESULT:
      if (mptool_save_data(from->result.type, from->result.ret, SYNC_FILE) != MP_OK) {
        printf("save failed !\n");
        to->result.ret = MP_ERROR;
      }
      break;

    default:
      to->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
