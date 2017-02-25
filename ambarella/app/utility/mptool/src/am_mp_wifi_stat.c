/*******************************************************************************
 * am_mp_wifi_stat.cpp
 *
 * History:
 *   Mar 24, 2015 - [longli] created file
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <net/if.h>
#include <ifaddrs.h>
#include "am_mp_server.h"
#include "am_mp_wifi_stat.h"

#define WIFI_CONFIG_FILE "/etc/mptool/mp_wifi.conf"
#define LINE_LENGHTH  256
#define SIGNAL_LEVEL_LEN  4
#define WIFI_ATTR_LEN 32

typedef struct wifi_area_info_t
{
    int32_t type; //0: judge by local file 1:judge by mac addr extra msg
    int32_t val; //0:world wide 1:USA and CANADA 2:Japan
} wifi_area_info;
/*
 * This routine try to fetch ssid and password if wifi
 * from client's message.
 *
 * Wifi configuration should like ssid;password
 *
 * @param  msg: message sent from client
 * @param ssid: store wifi's ssid
 * @param password: store wifi's password
 *
 * @return: 0 on success, -1 on error.
 */
static int32_t parse_wifi_attr(const char *msg, char *ssid, char *password)
{
  int32_t i, j;

  if (!msg || !ssid || !password) {
    printf("Invalid argument!\n");
    return -1;
  }

  if (strlen(msg) == 0) {
    printf("There is no ssid and password has been passed!\n");
    return -1;
  }

  /* Remove space characters at the begin of msg. */
  for (i = 0; msg[i] == ' '; i++);

  /* Try to parse ssid from wifi configuration. */
  for (j = 0; msg[i] != ';' && msg[i] != '\0'; i++) {
    if (msg[i] == '\\') {
      ssid[j++] = msg[++i];
    } else {
      ssid[j++] = msg[i];
    }
  }

  ssid[j] = '\0';
  if (msg[i] == '\0') {
    printf("No password needs to be parsed!\n");
    return 0;
  }

  /* Try to parse password from wifi configuraton. */
  for (j = 0, i++; msg[i] != '\0'; i++) {
    if (msg[i] == '\\') {
      password[j++] = msg[++i];
    } else {
      password[j++] = msg[i];
    }
  }

  password[j] = '\0';
  return 0;
}

/*
 * This routine try to connect specific AP.
 *
 * Firstly, we need to delete any connections under:
 * /etc/NetworkManager/system-connections
 *
 * Then, we using nmcli command to connect AP.
 *
 * @param ssid: AP's ssid, sent from client.
 * @param password: AP's password, sent from client.
 *
 * @return: 0 if connected, -1 on failure.
 */
static int32_t connect_to_ap(const char *ssid, const char *password)
{
  int32_t status, ret = -1;
  char cmd[BUF_LEN];

  if (strlen(password) > 0) {
    snprintf(cmd, sizeof (cmd),
             "nmcli dev wifi connect '%s' password '%s'",
             ssid, password);
  } else {
    snprintf(cmd, sizeof (cmd),
             "nmcli dev wifi connect '%s'", ssid);
  }

  system("rm /etc/NetworkManager/system-connections/*");
  status = system(cmd);
  if (WEXITSTATUS(status)) {
    printf ("Failed to connect %s\n", ssid);
  } else {
    struct ifaddrs *ifAddrList = NULL;

    if (getifaddrs(&ifAddrList) < 0) {
      perror("getifaddrs");
    } else {
      struct ifaddrs *ifa;
      for (ifa = ifAddrList; ifa; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_flags & IFF_LOOPBACK) && ifa->ifa_addr) {
          if ((ifa->ifa_addr->sa_family == AF_INET) ||
              (ifa->ifa_addr->sa_family == AF_INET6)) {
            if (!strcmp("wlan0", ifa->ifa_name)) {
              ret = 0;
            }
          }
        }
      }
    }
    freeifaddrs(ifAddrList);
  }

  return ret;
}

/*
 * This routine try to get signal level of specifi AP
 * by using nmcli command. Because we will use nmcli
 * command and popen library function, signal level
 * will be expressed in string format and it can be
 * transfered to client directly, so we don't need to
 * make any transformation between string and integer.
 *
 * @param ssid: ssid of specific AP
 * @param signal_level: AP's signal level
 *
 * @return: 1 if success,
 *          0 if ap can't be scanned,
 *          -1 on error
 */
static int32_t fetch_signal_level(const char *ssid, char *signal_level)
{
  FILE *fp;
  char cmd[BUF_LEN];
  char buf[LINE_LENGHTH];
  int32_t ret = 1;

  snprintf(cmd, sizeof (cmd),
           "nmcli dev wifi | grep '%s  ' | awk -F '/' '{print $2}' |"
           "awk '{print $2}'", ssid);

  /*
   * Set up a pipe between current process
   * and shell program
   */
  if ((fp = popen(cmd, "r")) == NULL) {
    printf ("Failed to create a pipe with sh.\n");
    return -1;
  }

  if (fgets(buf, LINE_LENGHTH, fp) == NULL) {
    /* AP with specific ssid can't be scanned. */
    ret = 0;
  } else {
    /* Remove new line character at the end of signal_level. */
    strcpy(signal_level, buf);
    signal_level[strlen(signal_level) - 1] = '\0';
  }

  pclose (fp);
  return ret;
}

static int32_t read_config_file()
{
  FILE *wifi_file_handle = NULL;
  int32_t file_len = 0;

  if (access(WIFI_CONFIG_FILE,F_OK)) {
    printf("The file : %s  is not existed!\n", WIFI_CONFIG_FILE);
    return 0;
  }

  if ((wifi_file_handle= fopen(WIFI_CONFIG_FILE, "rb+")) < 0) {
    perror("fopen");
    return 0;
  }

  fseek(wifi_file_handle, 0L, SEEK_END);
  file_len = ftell(wifi_file_handle);
  rewind(wifi_file_handle);

  char file_content[256];

  if (256 <= file_len) {
    printf("The msg buffer is not enough, file(%s) is in wrong format!\n",
           WIFI_CONFIG_FILE);
    fclose(wifi_file_handle);
    return 0;
  }

  memset(file_content,0,sizeof(file_content));

  if (fread(file_content, file_len, 1, wifi_file_handle) < 0) {
    perror("fread");
    fclose(wifi_file_handle);
    return 0;
  }

  fclose(wifi_file_handle);
  wifi_file_handle= NULL;

  file_len=strlen(file_content);
  int32_t i;
  for(i=file_len-1;file_content[i]==10;--i);
  file_content[i+1]='\0';

  if(!strcmp(file_content,"options area=0"))
  {
    return 0;
  }
  else if(!strcmp(file_content,"options area=1"))
  {
    return 1;
  }
  else if(!strcmp(file_content,"options area=2"))
  {
    return 2;
  }
  else
  {
    return 3;
  }
}

static int32_t write_config_file(int32_t area)
{
  FILE *wifi_file_handle = NULL;

  if ((wifi_file_handle= fopen(WIFI_CONFIG_FILE, "w+")) < 0) {
    perror("fopen");
    return -1;
  }

  if(area==0||area==3)
  {
    fprintf(wifi_file_handle,"options area=0");
  }
  else if(area==1)
  {
    fprintf(wifi_file_handle,"options area=1");
  }
  else if(area==2)
  {
    fprintf(wifi_file_handle,"options area=2");
  }
  fclose(wifi_file_handle);
  return 0;
}

static int get_wifi_area_from_mac_addr_extra_file()
{
  FILE *handle = NULL;
  int file_len = 0;

  if ((handle= fopen(EXTRA_MSG_FILE, "rb+")) < 0) {
    perror("fopen");
    return -1;
  }

  fseek(handle, 0L, SEEK_END);
  file_len = ftell(handle);
  rewind(handle);

  char file_content[256];

  if (256 <= file_len) {
    printf("The msg buffer is not enough!\n");
    fclose(handle);
    return -1;
  }

  memset(file_content,0,sizeof(file_content));

  if (fread(file_content, file_len, 1, handle) < 0) {
    perror("fread");
    fclose(handle);
    return -1;
  }

  fclose(handle);
  handle= NULL;

  int len = strlen(file_content);
  int i=0;
  for(;i < len && file_content[i] != '/'; ++i);
  if (i == len) {
    printf("can't find wifi area code\n");
    return -1;
  }
  file_content[i+3]='\0';
  if (!strcmp(&file_content[i+1], "00")) {
    return 0;
  }
  else if (!strcmp(&file_content[i+1], "37")) {
    return 1;
  }
  else if(!strcmp(&file_content[i+1], "11")) {
    return 2;
  } else {
    printf("unrecognized area code:%s", &file_content[i]);
    return -1;
  }
}

/*
 * This routine will serve as a handler to process
 * client's requests related to wifi.
 *
 * @param from: message sent from client.
 * @param to  : message sent to client.
 *
 * @return: MP_OK on success.
 */
am_mp_err_t mptool_wifi_stat_handler(am_mp_msg_t *from, am_mp_msg_t *to)
{
  char sl[SIGNAL_LEVEL_LEN] = {'\0'};
  char ssid[WIFI_ATTR_LEN] = {'\0'};
  char password[WIFI_ATTR_LEN] = {'\0'};
  wifi_area_info wifi_area_val;

  to->result.ret = MP_OK;
  switch (from->stage) {
    case 0:
      break;

    case 1:
      wifi_area_val.val = 0;
      wifi_area_val.type = 0;
      if (!access(EXTRA_MSG_FILE,F_OK)) {
        wifi_area_val.val = get_wifi_area_from_mac_addr_extra_file();
        if (wifi_area_val.val >= 0) {
          if (write_config_file(wifi_area_val.val) < 0) {
            to->result.ret=MP_ERROR;
            break;
          }
          wifi_area_val.type=1;
        }
      }
      if (!wifi_area_val.type) {
        wifi_area_val.val = read_config_file();
        if (wifi_area_val.val == -1) {
          to->result.ret=MP_ERROR;
          break;
        }
      }
      memset(to->msg,0,sizeof(to->msg));
      memcpy(to->msg, &wifi_area_val, sizeof(wifi_area_val));
      break;

    case 2:
      memset(&wifi_area_val.val,0,sizeof(int32_t));
      memcpy(&wifi_area_val.val, from->msg, sizeof(int32_t));
      if (write_config_file(wifi_area_val.val) < 0) {
        to->result.ret=MP_ERROR;
        break;
      }
      break;

    case 3:
      if (parse_wifi_attr(from->msg, ssid, password) < 0) {
        strncpy(to->msg, "Invalid wifi configuration!", sizeof(to->msg));
        to->result.ret = MP_ERROR;
      } else {
        if (connect_to_ap(ssid, password) < 0) {
          strncpy(to->msg, "Can't connect to AP!", sizeof(to->msg));
          to->result.ret = MP_ERROR;
        } else {
          if (fetch_signal_level(ssid, sl) <= 0) {
            strncpy(to->msg, "Can't get signal level of AP", sizeof(to->msg));
            to->result.ret = MP_ERROR;
          } else {
            strncpy(to->msg, sl, sizeof(to->msg));
          }
        }
      }

      if (mptool_save_data(from->result.type,
                           to->result.ret, SYNC_FILE) != MP_OK) {
        printf("save failed\n");
        to->result.ret = MP_IO_ERROR;
      }
      break;

    default:
      to->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
