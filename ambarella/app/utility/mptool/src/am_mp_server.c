/*******************************************************************************
 * mpserver.cpp
 *
 * History:
 *   Mar 5, 2015 - [longli] created file
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include "am_mp_server.h"
#include "am_mp_bad_pixel_calibration.h"
#include "am_mp_button.h"
#include "am_mp_ircut.h"
#include "am_mp_led.h"
#include "am_mp_lens_focus.h"
#include "am_mp_lens_shad_cal.h"
#include "am_mp_light_sensor.h"
#include "am_mp_mac_addr.h"
#include "am_mp_mic.h"
#include "am_mp_sensor.h"
#include "am_mp_sdcard.h"
#include "am_mp_speaker.h"
#include "am_mp_wifi_stat.h"

#define SERV_PORT 5000
#define LISTEN_LENGTH 10
#define VIRSION_FILE "/etc/os-release"
#define MPTOOL_DIR  "/etc/mptool"
#define MPTOOL_DATA_PATH  "/etc/mptool/mptool.log"

typedef enum {
  MP_ETH_HAND_SHAKE = 0,
  MP_ETH_ECHO_MSG,
  MP_ETH_SAVE_RESULT,
} mp_eth_stage_t;

typedef enum {
  MP_RESET_HAND_SHAKE = 0,
  MP_RESET_ALL,
} mp_reset_all_stage_t;

typedef enum {
  MP_GET_FW_INFO = 0,
  MP_FW_INFO_SAVE_RESULT,
} mp_fw_info_stage_t;

typedef struct {
    char os_version[16];
    char build_time[64];
    char fw_version[16];
} am_mp_fw_info_t;

am_mp_result_t mptool_result[AM_MP_MAX_ITEM_NUM];
int32_t fd_iav = -1;
static am_mp_err_t mptool_eth_handler(am_mp_msg_t *from_msg,
                                      am_mp_msg_t *to_msg);
static am_mp_err_t mptool_get_fw_info_handler(am_mp_msg_t *from_msg,
                                              am_mp_msg_t *to_msg);
static am_mp_err_t mptool_reset_all_handler(am_mp_msg_t *from_msg,
                                            am_mp_msg_t *to_msg);
static am_mp_err_t mptool_complete_test_handler(am_mp_msg_t *from_msg,
                                                am_mp_msg_t *to_msg);

am_mptool_handler msg_handler[] = {
  mptool_eth_handler, //AM_MP_ETHERNET
  mptool_button_handler, //AM_MP_BUTTON
  mptool_led_handler,//AM_MP_LED
  mptool_ir_led_handler,//AM_MP_IRLED
  mptool_light_sensor_handler,//AM_MP_LIGHT_SENSOR
  mptool_mac_addr_handler,//AM_MP_MAC_ADDR
  mptool_wifi_stat_handler,//AM_MP_WIFI_STATION
  mptool_mic_handler,//AM_MP_MIC
  mptool_speaker_handler,//AM_MP_SPEAKER
  mptool_sensor_handler,//AM_MP_SENSOR
  mptool_lens_focus_handler,//AM_MP_LENS_FOCUS
  mptool_lens_shad_cal_handler,//AM_MP_LENS_SHADING_CALIBRATION
  mptool_bad_pixel_calibration,//AM_MP_BAD_PIXEL_CALIBRATION
  mptool_get_fw_info_handler,//AM_MP_FW_INFO
  mptool_sd_test_handler,//AM_MP_SDCARD
  mptool_ircut_handler,//AM_MP_IRCUT
  mptool_complete_test_handler,//AM_MP_COMLETE_TEST
  mptool_reset_all_handler,//AM_MP_RESET_ALL
};

static am_mp_err_t mptool_load_data()
{
  FILE *fp = NULL;
  int32_t i, val;
  am_mp_err_t ret = MP_OK;

  do {
    /*Init the cache*/
    for (i = 0; i < AM_MP_MAX_ITEM_NUM; i ++) {
      mptool_result[i].type = (am_mp_msg_type_t) i;
      mptool_result[i].ret = MP_NO_RUN;
    }

    if (access(MPTOOL_DATA_PATH, F_OK) != 0) {
      printf("No test data was saved before, start new test!\n");
      ret = MP_NOT_EXIST;
      break;
    }

    if (!(fp = fopen(MPTOOL_DATA_PATH, "r"))) {
        perror("fopen");
        ret = MP_IO_ERROR;
        break;
      }

    char buffer[BUF_LEN] = { 0 };
    for (i = 0; i < AM_MP_MAX_ITEM_NUM; i ++) {
      if (fgets(buffer, sizeof(buffer) - 1, fp) == NULL) {
        printf("there are not enough data, treat the rest as not-test\n");
        break;
      }
      sscanf(buffer, "%d", &val);
      if (val > 20) val = MP_NO_RUN;
      mptool_result[i].ret = (am_mp_err_t) val;
    }
  } while (0);

  if (fp) {
    fclose(fp);
    fp = NULL;
  }

  return ret;
}

static am_mp_err_t mptool_write_file()
{
  FILE *fp = NULL;
  am_mp_err_t ret = MP_OK;

  if (!(fp = fopen(MPTOOL_DATA_PATH, "wb+"))) {
    perror("fopen");
    return MP_IO_ERROR;
  } else {
    int32_t i;
    for (i = 0; i < AM_MP_MAX_ITEM_NUM; i++) {
      fprintf(fp, "%d\n", mptool_result[i].ret);
    }
    fclose(fp);
    fp = NULL;
  }

  return ret;
}

am_mp_err_t mptool_save_data(am_mp_msg_type_t mp_type,
                             am_mp_err_t mp_result,
                             int32_t sync_flag)
{
  am_mp_err_t ret = MP_OK;
  /*save to the cache*/
  mptool_result[mp_type].type = mp_type;
  mptool_result[mp_type].ret = mp_result;

  if (sync_flag) {
    if (mptool_write_file() != MP_OK) {
      printf("mptool_write_file\n");
      ret = MP_IO_ERROR;
    }
  }

  return ret;
}

static am_mp_err_t mptool_eth_handler(am_mp_msg_t *from_msg,
                                      am_mp_msg_t *to_msg)
{
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case MP_ETH_HAND_SHAKE:
      break;
    case MP_ETH_ECHO_MSG:
      memcpy(to_msg->msg, from_msg->msg, sizeof(to_msg->msg));
      break;
    case MP_ETH_SAVE_RESULT:
      mptool_save_data(from_msg->result.type,
                       from_msg->result.ret,
                       SYNC_FILE);
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}

static int32_t get_fw_version(am_mp_fw_info_t *fw_info)
{
  FILE *fp = NULL;
  char buf[BUF_LEN];
  int32_t found = 0;

  do {
    if (access(VIRSION_FILE, F_OK) != 0) {
      printf("The file : %s  does not exist!\n", VIRSION_FILE);
      break;
    }
    if (!(fp = fopen(VIRSION_FILE, "r"))) {
      perror("open fw version file");
      break;
    }
    while (fgets(buf, sizeof(buf), fp)) {
      if(!strncmp("VERSION_ID=", buf, 11)) {
        memcpy(fw_info->fw_version,
               buf + 11,
               sizeof(fw_info->fw_version) - 1);
        found = 1;
        break;
      }
    }

    fclose(fp);
    fp = NULL;
  } while (0);

  return found;
}

static am_mp_err_t mptool_get_fw_info_handler(am_mp_msg_t *from_msg,
                                              am_mp_msg_t *to_msg)
{
  am_mp_fw_info_t fw_info;
  struct utsname uts_buf;

  memset(&fw_info, 0, sizeof(fw_info));
  memset(&uts_buf, 0, sizeof(uts_buf));
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case MP_GET_FW_INFO:
      if (uname(&uts_buf) < 0) {
        perror("uname");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      memcpy(fw_info.os_version, uts_buf.release, sizeof(fw_info.os_version));
      memcpy(fw_info.build_time, uts_buf.version, sizeof(fw_info.build_time));
      if (!get_fw_version(&fw_info)) {
        memcpy(fw_info.fw_version, "undefined", sizeof(fw_info.fw_version));
      }
      memcpy(to_msg->msg, &fw_info, sizeof(fw_info));
      break;
    case MP_FW_INFO_SAVE_RESULT:
      mptool_save_data(from_msg->result.type,
                       from_msg->result.ret,
                       SYNC_FILE);
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}

static am_mp_err_t mptool_reset_all_handler(am_mp_msg_t *from_msg,
                                            am_mp_msg_t *to_msg)
{
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case MP_RESET_HAND_SHAKE:
      break;
    case MP_RESET_ALL:
    {
      int32_t i = 0;
      for (i = 0; i < AM_MP_MAX_ITEM_NUM; i++) {
        mptool_result[i].type = (am_mp_msg_type_t) i;
        mptool_result[i].ret = MP_NO_RUN;
      }

      if (mptool_write_file() != MP_OK) {
        printf("reset : save file failed!\n");
        to_msg->result.ret = MP_IO_ERROR;
      }
      break;
    }
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}

static am_mp_err_t mptool_complete_test_handler(am_mp_msg_t *from_msg,
                                                am_mp_msg_t *to_msg)
{
  to_msg->result.ret = MP_OK;

  return MP_OK;
}

static am_mp_err_t mptool_msg_handler(int client_fd, am_mp_msg_t *client_msg)
{
  am_mp_err_t ret = MP_OK;
  am_mp_msg_t mp_server_msg;
  am_mptool_handler mptool_handler = NULL;

  memset(&mp_server_msg, 0, sizeof(mp_server_msg));

  if (client_msg->result.type <= AM_MP_RESET_ALL) {
    if (client_msg->result.type < AM_MP_MAX_ITEM_NUM) {
      mptool_handler = msg_handler[client_msg->result.type];
    } else {
      mptool_handler = msg_handler[client_msg->result.type - 1];
    }
  } else {
    mp_server_msg.result.ret = MP_NOT_EXIST;
  }

  if (mptool_handler) {
    mptool_handler(client_msg, &mp_server_msg);
  }

  mp_server_msg.result.type = client_msg->result.type;
  mp_server_msg.stage = client_msg->stage;
  if (write(client_fd, &mp_server_msg, sizeof(mp_server_msg)) < 0) {
    perror("write");
    ret = MP_ERROR;
  }

  return ret;
}

static void main_loop(int32_t fd)
{
  int32_t ret = MP_OK;
  int32_t i = 0;
  am_mp_msg_t mptool_msg;

  do {
    ret = mptool_load_data();
    if (ret != MP_OK && ret != MP_NOT_EXIST) {
      printf("mptool_load_data failed\n");
      break;
    }

    for (i = 0; i < AM_MP_MAX_ITEM_NUM; i++) {
      memset(&mptool_msg, 0, sizeof(am_mp_msg_t));
      mptool_msg.result.type = mptool_result[i].type;
      mptool_msg.result.ret = mptool_result[i].ret;
      if (write(fd, &mptool_msg, sizeof(am_mp_msg_t)) < 0) {
        perror("write");
        break;
      }
    }

    while (1) {
      memset(&mptool_msg, 0, sizeof(am_mp_msg_t));
      mptool_msg.result.type = AM_MP_TYPE_INIT;

      ret = read(fd, &mptool_msg, sizeof(am_mp_msg_t));
      if (ret > 0) {
        if (mptool_msg.result.type == AM_MP_TYPE_INIT) {
          continue;
        }

        if (mptool_msg_handler(fd, &mptool_msg) != MP_OK) {
          printf("mptool_msg_handler failed\n");
          break;
        }
      } else if (ret == 0) {
        printf("client has closed connection\n");
        break;
      } else {
        perror("read");
        break;
      }
    }

    mptool_write_file();
  } while (0);
}

int32_t main()
{
  int32_t sockfd = -1;
  int32_t connfd = -1;
  int32_t ret = 0;
  int32_t on = 1;
  struct sockaddr_in servr_addr, client_addr;
  socklen_t length = sizeof(client_addr);

  do {
    if (access(MPTOOL_DIR, F_OK)) {
      int32_t res;
      res = mkdir(MPTOOL_DIR, 0777) == 0 ? 0 : errno;
      if (res && res != EEXIST) {
        printf("am_mpserver: mkdir %s error\n", MPTOOL_DIR);
        ret = -1;
        break;
      }
    }

    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
      perror("open /dev/iav");
      ret = -1;
      break;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("socket");
      ret = -1;
      break;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
      perror("setsockopt");
      ret = -1;
      break;
    }

    memset(&servr_addr, 0, sizeof(servr_addr));
    servr_addr.sin_family = AF_INET;
    servr_addr.sin_port = htons(SERV_PORT);
    servr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *) &servr_addr, sizeof(servr_addr)) < 0) {
      perror("bind");
      ret = -1;
      break;
    }

    if (listen(sockfd, LISTEN_LENGTH) < 0) {
      perror("listen");
      ret = -1;
      break;
    }

    // to do kill apps_launcher kill -2 pid

    while (1) {
      if ((connfd = accept(sockfd, (struct sockaddr *)&client_addr, &length)) < 0) {
        perror("accept");
        continue;
      }
      main_loop(connfd);
      close(connfd);
    }

  } while (0);

  if (sockfd >= 0) {
    close(sockfd);
  }

  if (fd_iav >= 0) {
    close(fd_iav);
  }

  return ret;
}
