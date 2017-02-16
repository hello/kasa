/*******************************************************************************
 * am_system_service_msg_action.cpp
 *
 * History:
 *   2014-9-16 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include <time.h>
#include <signal.h>
#include <thread>
#include <sys/time.h>
#include <sys/reboot.h>
#include <fstream>
#include "am_log.h"
#include "am_service_frame_if.h"
#include "am_api_system.h"
#include "am_system_service_priv.h"
#include "am_upgrade_if.h"
#include "am_led_handler_if.h"

#define RC_NUMBER 2     //RC2 (release candidate)
#define VIRSION_FILE "/etc/os-release"

using namespace std;

extern AMIServiceFrame   *g_service_frame;
extern AM_SERVICE_STATE g_service_state;
extern AMIFWUpgradePtr g_upgrade_fw;
extern AMILEDHandlerPtr g_ledhandler;
extern am_mw_ntp_cfg g_ntp_cfg;
extern thread *g_th_ntp;

void ON_SERVICE_INIT(void *msg_data,
                     int32_t msg_data_size,
                     void *result_addr,
                     int32_t result_max_size)
{
  //put init result to result_addr
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("System service Init Done...");
    } break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize System service...");
    } break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("System service is still initializing...");
    } break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                     int32_t msg_data_size,
                     void *result_addr,
                     int32_t result_max_size)
{
  INFO("system service destroy, cleanup\n");
  PRINTF("ON IMAGE SERVICE DESTROY");
  g_service_frame->quit(); /* make run() in main quit */
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_START(void *msg_data,
                      int32_t msg_data_size,
                      void *result_addr,
                      int32_t result_max_size)
{
  INFO("system service start\n");
  int ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP(void *msg_data,
                     int32_t msg_data_size,
                     void *result_addr,
                     int32_t result_max_size)
{
  INFO("system service stop\n");
  int ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int32_t msg_data_size,
                        void *result_addr,
                        int32_t result_max_size)
{
  INFO("system service restart\n");
  int ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STATUS(void *msg_data,
                       int32_t msg_data_size,
                       void *result_addr,
                       int32_t result_max_size)
{
  INFO("system service status\n");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_FIRMWARE_VERSION_GET(void *msg_data,
                     int32_t msg_data_size,
                     void *result_addr,
                     int32_t result_max_size)
{
  INFO("system service firmware version get\n");

  if (!result_addr) {
    ERROR("result_addr is NULL.\n");
    return;
  }

  if (result_max_size < (int32_t)sizeof(am_mw_firmware_version_all)) {
    ERROR("result_max_size is not correct.");
    return;
  }

  am_mw_firmware_version_all * pfirmware_version;
  ifstream in_file;
  string line_str;
  int32_t num1, num2, num3;

  pfirmware_version = (am_mw_firmware_version_all *) result_addr;
  memset(pfirmware_version, 0, sizeof(am_mw_firmware_version_all));

  in_file.open("/proc/version", ios::in);
  if (in_file.is_open()) {
    while (!in_file.eof()) {
      if(getline(in_file, line_str) > 0) {
        if (3 == sscanf(line_str.c_str(), "Linux version %d.%d.%d+\n",
                        &num1, &num2, &num3)) {
          pfirmware_version->linux_kernel_version.major = num1;
          pfirmware_version->linux_kernel_version.minor = num2;
          pfirmware_version->linux_kernel_version.trivial = num3;
        } else {
          ERROR("Fail to get linux version.");
        }
        break;
      }
    }
    in_file.close();
  } else {
    ERROR("Failed to open /proc/version");
  }

  pfirmware_version->rc_number = RC_NUMBER;

  in_file.open(VIRSION_FILE, ios::in);
  if (in_file.is_open()) {
    bool ver_f = false;
    bool date_f = false;
    while (!in_file.eof()) {
      if(getline(in_file, line_str) > 0) {
        if(!ver_f && (line_str.find("VERSION_ID") != string::npos)) {
          if (3 == sscanf(line_str.c_str(),
                          "VERSION_ID=%d.%d.%d",
                          &num1, &num2, &num3)) {
            pfirmware_version->sw_version.major = num1;
            pfirmware_version->sw_version.minor = num2;
            pfirmware_version->sw_version.trivial = num3;
            ver_f = true;
          }
        } else if (!date_f && (line_str.find("RELEASE_DATE") != string::npos)) {
          if (3 == sscanf(line_str.c_str(),
                          "RELEASE_DATE=%d-%d-%d",
                          &num1, &num2, &num3)) {
            pfirmware_version->rel_date.year = num1;
            pfirmware_version->rel_date.month = num2;
            pfirmware_version->rel_date.day = num3;
            date_f = true;
          }
        }

        if (ver_f && date_f)
          break;
      }
    }
    in_file.close();
  } else {
    ERROR("Failed to open %s", VIRSION_FILE);
  }

  /* generate version sha1, which is a unique firmware id,
   * just for unique purpose, even recompiling will make it different
   */
  system("/usr/bin/sha1sum /proc/version  > /tmp/version_sha1");
  in_file.open("/tmp/version_sha1", ios::in);
  if (in_file.is_open()) {
    if (!in_file.eof()) {
      getline(in_file, line_str);
      uint32_t pos = 0;
      pos = line_str.find_first_of(" ");
      if (pos == string::npos) {
        pos = line_str.find_first_of('\t');
      }

      if (pos != string::npos) {
        strncpy(pfirmware_version->linux_sha1,
                line_str.substr(0, pos).c_str(),
                LINUX_SHA1_LEN);
        pfirmware_version->linux_sha1[LINUX_SHA1_LEN - 1] = '\0';
      } else {
        ERROR("Failed to read kernel version SHA1 string "
            "from file /tmp/version_sha1.");
      }
    }
    in_file.close();
  } else {
    ERROR("Failed to open /tmp/version_sha1");
  }

}

void ON_SYSTEM_TEST(void *msg_data,
                     int32_t msg_data_size,
                     void *result_addr,
                     int32_t result_max_size)
{

  INFO("system service TEST FUNCTION OK\n");

}

void ON_SYSTEM_DATETIME_GET(void *msg_data,
                            int32_t msg_data_size,
                            void *result_addr,
                            int32_t result_max_size)
{
  INFO("system service: ON_SYSTEM_DATETIME_GET \n");

  if (!result_addr) {
    ERROR("result_addr is NULL.\n");
    return;
  }

  if (result_max_size < (int32_t)sizeof(am_mw_system_settings_datetime)) {
    ERROR("result_max_size is not correct.");
    return;
  }

  time_t now;
  struct tm *timenow;
  struct timezone tz;
  struct timeval tv;
  am_mw_system_settings_datetime mw_system_settings_datetime_get;

  time(&now);
  timenow = localtime(&now);
  gettimeofday(&tv, &tz);

  mw_system_settings_datetime_get.date.year = timenow->tm_year + 1900;
  mw_system_settings_datetime_get.date.month = timenow->tm_mon + 1;
  mw_system_settings_datetime_get.date.day = timenow->tm_mday;
  mw_system_settings_datetime_get.time.hour = timenow->tm_hour;
  mw_system_settings_datetime_get.time.minute = timenow->tm_min;
  mw_system_settings_datetime_get.time.second = timenow->tm_sec;
  //west is negtive , east is positive
  mw_system_settings_datetime_get.time.timezone = (-tz.tz_minuteswest) / 60;
  DEBUG("%d-%d-%d %d:%d:%d TZ:%d",
       mw_system_settings_datetime_get.date.year,
       mw_system_settings_datetime_get.date.month,
       mw_system_settings_datetime_get.date.day,
       mw_system_settings_datetime_get.time.hour,
       mw_system_settings_datetime_get.time.minute,
       mw_system_settings_datetime_get.time.second,
       mw_system_settings_datetime_get.time.timezone);

  memcpy(result_addr,
         &mw_system_settings_datetime_get,
         sizeof(mw_system_settings_datetime_get));
}

void ON_SYSTEM_DATETIME_SET(void *msg_data,
                            int32_t msg_data_size,
                            void *result_addr,
                            int32_t result_max_size)
{
  INFO("system service: ON_SYSTEM_DATETIME_SET \n");

  int32_t ret = 0;
  struct tm tmv;
  struct timeval tv, tv1;
  struct timezone tz;
  time_t timep;
  am_mw_system_settings_datetime *mw_system_settings_datetime_set;

  do {
    if (!msg_data) {
      ERROR("msg_data is NULL.\n");
      ret = 1;
      break;
    }

    if (msg_data_size != (int32_t)sizeof(am_mw_system_settings_datetime)) {
      ERROR("msg_data_size is not correct.");
      ret = 2;
      break;
    }

    mw_system_settings_datetime_set=(am_mw_system_settings_datetime *)msg_data;
    tmv.tm_sec = mw_system_settings_datetime_set->time.second;
    tmv.tm_min = mw_system_settings_datetime_set->time.minute;
    tmv.tm_hour = mw_system_settings_datetime_set->time.hour;
    tmv.tm_mday = mw_system_settings_datetime_set->date.day;
    tmv.tm_mon = mw_system_settings_datetime_set->date.month - 1;
    tmv.tm_year = mw_system_settings_datetime_set->date.year - 1900;

    timep = mktime(&tmv);
    tv.tv_sec = timep;
    tv.tv_usec = 0;

    gettimeofday(&tv1, &tz);
    tz.tz_minuteswest = -(mw_system_settings_datetime_set->time.timezone) * 60;

    if (settimeofday(&tv, &tz) < 0) {
      ERROR("settimeofday error!\n");
      ret = 3;
      break;
    }
  } while (0);

  if ((result_max_size >= (int32_t)sizeof(ret)) && result_addr) {
    memcpy(result_addr, &ret, sizeof(ret));
  }

  return;
}

void ON_LED_INDICATOR_GET(void *msg_data,
                          int32_t msg_data_size,
                          void *result_addr,
                          int32_t result_max_size)
{
  am_mw_led_config mw_led_cfg;

  INFO("system service: ON_LED_INDICATOR_GET \n");
  do {
    mw_led_cfg.gpio_id = -1;

    if (!msg_data) {
      ERROR("msg_data is NULL");
      break;
    }

    if (!result_addr) {
      ERROR("result_addr NULL.\n");
      break;
    }

    if (msg_data_size != (int32_t)sizeof(int32_t)) {
      ERROR("msg_data size error.");
      break;
    }

    if (result_max_size < (int32_t)sizeof(am_mw_led_config)) {
      ERROR("result_max_size is not correct.");
      break;
    }

    if (!g_ledhandler) {
      g_ledhandler = AMILEDHandler::get_instance();
      if (!g_ledhandler) {
        ERROR("AMILEDHandler::get_instance() failed.\n");
        break;
      }
    }

    AMLEDConfig led_cfg;
    led_cfg.gpio_id = *((int32_t *)msg_data);
    INFO("gpio_id=%d\n", led_cfg.gpio_id);
    if(!g_ledhandler->get_led_state(led_cfg)) {
      INFO("Failed to get gpio%d state.", led_cfg.gpio_id);
      break;
    }

    mw_led_cfg.blink_flag = led_cfg.blink_flag;
    mw_led_cfg.gpio_id = led_cfg.gpio_id;
    mw_led_cfg.led_on = led_cfg.led_on;
    mw_led_cfg.off_time = led_cfg.off_time;
    mw_led_cfg.on_time = led_cfg.on_time;

  } while (0);

  memcpy(result_addr, &mw_led_cfg, sizeof(mw_led_cfg));

}

void ON_LED_INDICATOR_SET(void *msg_data,
                          int32_t msg_data_size,
                          void *result_addr,
                          int32_t result_max_size)
{
  int32_t ret = 0;

  INFO("system service: ON_LED_INDICATOR_SET \n");
  do {
    if ((msg_data_size != (int32_t)sizeof(am_mw_led_config)) || (!msg_data)) {
      ERROR("Invalid led cfg parameters");
      ret = 1;
      break;
    }

    if (!g_ledhandler) {
      g_ledhandler = AMILEDHandler::get_instance();
      if (!g_ledhandler) {
        ERROR("AMILEDHandler::get_instance() failed.\n");
        ret = 2;
        break;
      }
    }

    am_mw_led_config *mw_led_cfg = (am_mw_led_config *)msg_data;
    AMLEDConfig led_cfg;
    led_cfg.blink_flag = mw_led_cfg->blink_flag;
    led_cfg.gpio_id = mw_led_cfg->gpio_id;
    led_cfg.led_on = mw_led_cfg->led_on;
    led_cfg.off_time = mw_led_cfg->off_time;
    led_cfg.on_time = mw_led_cfg->on_time;
    if (!g_ledhandler->set_led_state(led_cfg)) {
      ERROR("Failed to set led state");
      ret = 3;
      break;
    }
  } while (0);

  if (result_addr && result_max_size >= (int32_t)sizeof(ret)) {
    memcpy(result_addr, &ret, sizeof(ret));
  }
}

void ON_LED_INDICATOR_UNINIT(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size)
{
  int32_t ret = 0;

  do {
    if (!msg_data) {
      ERROR("msg_data is NULL");
      ret = 1;
      break;
    }

    if (msg_data_size != (int32_t)sizeof(int32_t)) {
      ERROR("msg_data size is not correct.");
      ret = 2;
      break;
    }

    if (!g_ledhandler) {
      g_ledhandler = AMILEDHandler::get_instance();
      if (!g_ledhandler) {
        ERROR("AMILEDHandler::get_instance() failed.");
        ret = 3;
        break;
      }
    }

    int32_t gpio_id = *((int32_t*)msg_data);
    if (gpio_id < 0) {
      INFO("Invalid gpio id.");
      ret = 4;
      break;
    }

    if (!g_ledhandler->deinit_led(gpio_id)) {
      INFO("Uninit gpio led fail.");
      ret = 5;
      break;
    }
  } while (0);

  if (result_addr && result_max_size >= (int32_t)sizeof(ret)) {
    memcpy(result_addr, &ret, sizeof(ret));
  }
}

void ON_LED_INDICATOR_UNINIT_ALL(void *msg_data,
                                 int msg_data_size,
                                 void *result_addr,
                                 int result_max_size)
{
  int32_t ret = 0;

  do {
    if (!g_ledhandler) {
      g_ledhandler = AMILEDHandler::get_instance();
      if (!g_ledhandler) {
        ERROR("AMILEDHandler::get_instance() failed.");
        ret = 1;
        break;
      }
    }

    g_ledhandler->deinit_all_led();
  } while (0);

  if (result_addr && result_max_size >= (int32_t)sizeof(ret)) {
    memcpy(result_addr, &ret, sizeof(ret));
  }
}

/* 0: success
 * 1: Invalid parameters
 * 2: Get instance fail
 * 3: Upgrade is still in process
 * */

void ON_FIRMWARE_UPGRADE_SET(void *msg_data,
                             int32_t msg_data_size,
                             void *result_addr,
                             int32_t result_max_size)
{
  int32_t ret = 0;

  INFO("system service: ON_FIRMWARE_UPGRADE_SET \n");
  do {
    if ((msg_data_size != (int32_t)sizeof(AMUpgradeArgs)) || !msg_data) {
      ERROR("msg_data is NULL or msg_data size error.");
      ret = 1;
      break;
    }

    if (!g_upgrade_fw) {
      g_upgrade_fw = AMIFWUpgrade::get_instance();
      if (!g_upgrade_fw) {
        ERROR("AMIFWUpgrade::get_instance() fail.");
        ret = 2;
        break;
      }
    }

    if (g_upgrade_fw->is_in_progress()) {
      PRINTF("Upgrade is still in process, please try again later.\n");
      ret = 3;
      break;
    }

    g_upgrade_fw->run_upgrade_thread(msg_data, msg_data_size);
  } while (0);

  if (result_addr && result_max_size >= (int32_t)sizeof(ret)) {
    memcpy(result_addr, &ret, sizeof(ret));
  }
}

void ON_UPGRADE_STATUS_GET(void *msg_data,
                           int32_t msg_data_size,
                           void *result_addr,
                           int32_t result_max_size)
{
  am_mw_upgrade_status upgrade_state;

  INFO("system service: ON_UPGRADE_STATUS_GET \n");
  do {
    if (!result_addr) {
      ERROR("result_addr NULL.\n");
      break;
    }

    if (result_max_size < (int32_t)sizeof(am_mw_upgrade_status)) {
      ERROR("result_max_size is not correct.");
      break;
    }

    if (!g_upgrade_fw) {
      g_upgrade_fw = AMIFWUpgrade::get_instance();
      if (!g_upgrade_fw) {
        ERROR("AMIFWUpgrade::get_instance() fail.");
        break;
      }
    }

    upgrade_state.in_progress = g_upgrade_fw->is_in_progress();
    upgrade_state.state=(am_mw_upgrade_state)g_upgrade_fw->get_upgrade_status();
    memcpy(result_addr, &upgrade_state, sizeof(upgrade_state));
  } while (0);
}

static void do_ntp()
{
  string cmd("");
  int32_t status, i;

  INFO("do ntpdate...");
  for (i = 0; i < SERVER_NUM; ++i) {
    if (g_ntp_cfg.server[i].empty()) {
      continue;
    }
    cmd = "ntpdate " + g_ntp_cfg.server[i];
    status = system(cmd.c_str());
    if (WEXITSTATUS(status)) {
      PRINTF("%s fail\n", cmd.c_str());
    } else {
      break;
    }
  }
}

static void ntp_thread()
{
  time_t s_time, e_time, time_inter;

  INFO("Enter ntp thread...");
  s_time = time(NULL);
  time_inter = (((g_ntp_cfg.sync_time_day * 24) + g_ntp_cfg.sync_time_hour) * 60
      + g_ntp_cfg.sync_time_minute) * 60 + g_ntp_cfg.sync_time_second;

//check network connection?
  do_ntp();
  while (g_ntp_cfg.enable) {
    e_time = time(NULL);
    if (((e_time - s_time) > time_inter) || (e_time < s_time)) {
      do_ntp();
      s_time = e_time;
    }
    sleep(1);
  }
  INFO("exit ntp thread...");
}

void ON_NTP_GET(void *msg_data,
                int32_t msg_data_size,
                void *result_addr,
                int32_t result_max_size)
{
  INFO("system service: ON_NTP_GET \n");
  do {
    if (!result_addr) {
      ERROR("result_addr NULL.\n");
      break;
    }

    int32_t i = 0, str_len;
    string ntp_set;
    char ntp_cfg[32] = {0};
    char *p_str = (char *)result_addr;
    snprintf(ntp_cfg,
             sizeof(ntp_cfg) - 1,
             "%d,%d/%d/%d/%d,",
             g_ntp_cfg.enable ? 1 : 0,
             g_ntp_cfg.sync_time_day,
             g_ntp_cfg.sync_time_hour,
             g_ntp_cfg.sync_time_minute,
             g_ntp_cfg.sync_time_second);

    ntp_set = ntp_cfg;
    for (i = 0; i < SERVER_NUM && (!g_ntp_cfg.server[i].empty()); ++i) {
      if (i != 0) {
        ntp_set =  ntp_set + " " + g_ntp_cfg.server[i];
      } else {
        ntp_set =  ntp_set + g_ntp_cfg.server[i];
      }
    }

    str_len = ntp_set.length();
    if (result_max_size < str_len + 1) {
      ERROR("result_max_size is not enough.");
      break;
    }

    memcpy(result_addr, ntp_set.c_str(), str_len);
    p_str[str_len] = '\0';
  } while (0);
}

void ON_NTP_SET(void *msg_data,
                int32_t msg_data_size,
                void *result_addr,
                int32_t result_max_size)
{
  int32_t ret = 0;

  INFO("system service: ON_NTP_SET \n");
  do {
    if (!msg_data) {
      ERROR("msg_data is NULL");
      ret = 1;
      break;
    }

    if (msg_data_size < 9) {
      ERROR("ntp settings format wrong.");
      ret = 2;
      break;
    }

    string ntp_set, tmp_str;
    int32_t ntp_enble, day, hour, minute, second;
    uint32_t pos = string::npos;

    ntp_set = (char *)msg_data;
    pos = ntp_set.find_last_of(",");
    if (pos == string::npos) {
      ERROR("ntp settings format wrong.");
      ret = 2;
      break;
    }
    tmp_str = ntp_set.substr(0, pos + 1);

    if (5 != sscanf(tmp_str.c_str(),
                    "%d,%d/%d/%d/%d,",
                    &ntp_enble,
                    &day,
                    &hour,
                    &minute,
                    &second)) {
      ERROR("ntp settings format wrong.");
      ret = 2;
      break;
    }

    tmp_str = ntp_set.substr(pos + 1);
    int32_t i = 0;
    for (i = 0; i < SERVER_NUM; ++i) {
      pos = tmp_str.find_first_of(" ");
      if (pos != string::npos) {
        g_ntp_cfg.server[i] = tmp_str.substr(0, pos);
        pos = tmp_str.find_first_not_of(" ", pos + 1);
        if (pos != string::npos) {
          tmp_str = tmp_str.substr(pos);
        } else {
          ++i;
          break;
        }
      } else {
        if (!tmp_str.empty()) {
          g_ntp_cfg.server[i] = tmp_str;
          ++i;
        }
        break;
      }
    }

    for (; i < SERVER_NUM; ++i) {
      g_ntp_cfg.server[i] = "";
    }

    if (g_th_ntp) {
      g_ntp_cfg.enable = false;
      g_th_ntp->join();
      delete g_th_ntp;
      g_th_ntp = NULL;
    }

    g_ntp_cfg.sync_time_day = day;
    g_ntp_cfg.sync_time_hour = hour;
    g_ntp_cfg.sync_time_minute = minute;
    g_ntp_cfg.sync_time_second = second;

    if (!ntp_enble) {
      g_ntp_cfg.enable = false;
      break;
    }

    g_ntp_cfg.enable = true;

    for (i = 0;
        i < SERVER_NUM && (g_ntp_cfg.server[i].empty()); ++i);

    if (i == SERVER_NUM) {
      ERROR("server addr is NULL");
      ret = 3;
      break;
    }

    g_th_ntp = new thread(ntp_thread);
    if (!g_th_ntp) {
      ERROR("Fail to create ntp thread.");
      ret = 4;
      break;
    }

  } while (0);

  if (result_addr && result_max_size >= (int32_t)sizeof(ret)) {
    memcpy(result_addr, &ret, sizeof(ret));
  }
}
