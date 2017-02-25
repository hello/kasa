/*******************************************************************************
 * test_system_service.cpp
 *
 * History:
 *   2015-1-27 - [longli] created file
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

#include "am_base_include.h"
#include "am_log.h"
#include <signal.h>
#include <fstream>
#include <iostream>

#include "am_api_system.h"
#include "am_api_helper.h"

using namespace std;

static AMAPIHelperPtr g_api_helper = nullptr;
static am_mw_fw_upgrade_args up_args = {0};
static bool first_set = true;

static void sigstop (int32_t arg)
{
  INFO("Signal has been captured, test_system_service quits!");
  exit (1);
}

static void show_upgrade_menu()
{
  printf("\n------------------ upgrade settings --------------------\n");
  printf("Tips: firmware path must be set when select mode 0 or mode 2\n\n");
  printf(" m -- set upgrade mode(default mode: 2)\n");
  printf(" p -- set firmware path\n");
  printf(" c -- set use sdcard to do upgrade\n");
  printf(" t -- set connect timeout value (optional)\n");
  printf(" a -- set download authentication (optional)\n");
  printf(" s -- start to do upgrade\n");
  printf(" q -- exit to main menu\n");
  printf("\n--------------------------------------------------------\n");
  printf(">\n");
}

void show_upgrade_settings(am_mw_fw_upgrade_args &up_args)
{
  printf("\nUpgrade mode : %d\n", up_args.upgrade_mode);
  if (up_args.path_to_upgrade_file[0]) {
    printf("Firmware path : %s\n", up_args.path_to_upgrade_file);
  } else {
    printf("Firmware path :  (If not set in mode 1, use the path set before or"
        " default path)\n");
  }
  printf("use sdcard : %s\n", up_args.use_sdcard ? "YES":"NO");
  printf("connect timeout value : %d\n", up_args.timeout);
  if (!up_args.user_name[0]) {
    printf("connect download authentication :\n");
  } else{
    printf("connect download authentication : %s",
           up_args.user_name);
    if (up_args.passwd[0]) {
      printf(":%s\n", up_args.passwd);
    }
  }
}

static void upgrade_settings()
{
  string in_str;
  int32_t in_num = -1;
  bool is_run = true;

  if (first_set) {
    up_args.upgrade_mode = AM_FW_DOWNLOAD_AND_UPGRADE;
    up_args.use_sdcard = 0;
    first_set = false;
  }

  while (is_run) {
    show_upgrade_settings(up_args);
    show_upgrade_menu();
    cin >> in_str;

    switch (in_str[0]) {
      case 'q':
      case 'Q': {
        is_run = false;
      } break;

      case 'm':
      case 'M': {
        printf("Please input upgrade mode (0:download firmware only 1:upgrade only "
            "2: download and upgrade only):\n");
        cin >> in_str;
        if (!isdigit((in_str.c_str())[0])) {
          PRINTF("Input is not digit.\n");
          continue;
        }

        in_num = atoi(in_str.c_str());
        if (in_num < 0 || in_num > 2) {
          PRINTF("Invalid upgrade mode, default mode is 2.\n");
        } else {
          up_args.upgrade_mode = (am_mw_firmware_upgrade_mode)in_num;
          printf("Set upgrade mode successfully.\n");
        }
      } break;

      case 'C':
      case 'c':
        printf("Please set whether upgrade from sdcard (0:use adc partition "
            "1:use sdcard):\n");
        cin >> in_str;
        if (!isdigit((in_str.c_str())[0])) {
          PRINTF("Input is not digit.\n\n");
          continue;
        }
        in_num = atoi(in_str.c_str());
        up_args.use_sdcard = in_num;
        printf("Set whether upgrade from sdcard successfully.\n");
        break;

      case 'p':
      case 'P': {
        int32_t len = sizeof(up_args.path_to_upgrade_file);
        printf("Please input firmware path(e.g: "
            "http://10.0.0.1:8080/s2lm_ironman.7z):\n");
        cin >> in_str;
        strncpy(up_args.path_to_upgrade_file, in_str.c_str(), len - 1);
        up_args.path_to_upgrade_file[len - 1] = '\0';
        printf("Set upgrade firmware path successfully.\n");
      } break;

      case 't':
      case 'T': {
        printf("Please input connect timeout value(e.g: 5):\n");
        cin >> in_str;
        if (!isdigit((in_str.c_str())[0])) {
          PRINTF("Input is not digit.\n\n");
          continue;
        }
        in_num = atoi(in_str.c_str());
        up_args.timeout = in_num;
        printf("Set download connect timeout value successfully.\n");
      } break;

      case 'a':
      case 'A': {
        uint32_t pos = string::npos;
        uint32_t len = 0;
        len = sizeof(up_args.user_name);
        printf("Please input download authentication(e.g:username:passwd):\n");
        cin >> in_str;
        pos = in_str.find_first_of(":");
        if (pos != string::npos) {
          strncpy(up_args.user_name, in_str.substr(0, pos).c_str(), len - 1);
          strncpy(up_args.passwd, in_str.substr(pos + 1).c_str(), len - 1);
          up_args.user_name[len - 1] = '\0';
          up_args.passwd[len - 1] = '\0';
        } else {
          strncpy(up_args.user_name, in_str.c_str(), len - 1);
          up_args.user_name[len - 1] = '\0';
          up_args.passwd[0] = '\0';
        }
        printf("Set download authentication successfully.\n");
      } break;

      case 's':
      case 'S': {
        if (up_args.upgrade_mode < 0 || up_args.upgrade_mode > 2) {
          PRINTF("Invalid mode %d, please set mode, and retry.\n",
                 up_args.upgrade_mode);
        } else if (up_args.upgrade_mode != AM_FW_UPGRADE_ONLY &&
            !up_args.path_to_upgrade_file[0]) {
          PRINTF("Firmware path not set, please set it and try again,"
              " your selected mode is %d.\n", up_args.upgrade_mode);
        } else {
          printf("Begin to do upgrade...\n");
          am_service_result_t m_ret = {0};
          g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET,
                                    &up_args,
                                    sizeof(up_args),
                                    &m_ret,
                                    sizeof(m_ret));
          if (m_ret.ret < 0) {
            PRINTF("Method_call error!");
          } else if (m_ret.ret == 1) {
            PRINTF("Invalid upgrade parameters!");
          } else if (m_ret.ret == 2) {
            PRINTF("Failed to get upgrade instance!");
          } else if (m_ret.ret == 3) {
            PRINTF("Upgrade is still in process, please try again later.");
          } else {
            printf("Start upgrade thread and back to main menu...\n");
            is_run = false;
          }
        }
      } break;

      default: {
        printf("No such option, please try again.\n");
      } break;
    }
  }
}

static void get_system_time()
{
  am_service_result_t result = {0};
  am_mw_system_settings_datetime now_time = {0};

  INFO("get_system_time is called.");
  memset(result.data, 0, sizeof(result.data));
  memset(&now_time, 0, sizeof(am_mw_system_settings_datetime));
  g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET,
                            nullptr,
                            0,
                            &result,
                            sizeof(result));
  printf("\n");
  memcpy(&now_time, result.data, sizeof(am_mw_system_settings_datetime));
  if (now_time.date.year > 0) {
    printf("Now time is : %d-%d-%d %d:%d:%d , Time zone is: %d\n",
           now_time.date.year,
           now_time.date.month,
           now_time.date.day,
           now_time.time.hour,
           now_time.time.minute,
           now_time.time.second,
           now_time.time.timezone);
  } else {
    PRINTF("Get system time fail.");
  }
}

static bool set_system_time()
{
  bool ret = true;
  string time_str;
  am_service_result_t retv = {0};
  am_mw_system_settings_datetime new_time = {0};

  INFO("set_system_time is called.");
  do {
    get_system_time();
    printf("\nTips:\n");
    printf("1.Time format is: "
        "Year/Month/Day/Hour/Minute/Second/Timezone "
        "(e.g: 2013/5/16/13/15/8/8)\n");
    printf("2.Time zone: (e.g: +8 is for Beijing Time)\n\n");
    printf("Please input new time:\n");
    cin >> time_str;
    int32_t year, month, day, hour, minute, second, timezone;
    int32_t arg_num = sscanf(time_str.c_str(),
                             "%d/%d/%d/%d/%d/%d/%d",
                             &year,
                             &month,
                             &day,
                             &hour,
                             &minute,
                             &second,
                             &timezone);
    if (arg_num != 7) {
      printf("format is not correct.\n");
      ret = false;
      break;
    }
    new_time.date.year = year;
    new_time.date.month = month;
    new_time.date.day = day;
    new_time.time.hour = hour;
    new_time.time.minute = minute;
    new_time.time.second = second;
    new_time.time.timezone = timezone;
    g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET,
                              &new_time,
                              sizeof(new_time),
                              &retv,
                              sizeof(retv));
    switch (retv.ret) {
      case 0:
        printf("set system time successfully.\n");
        break;
      case 1:
        PRINTF("msg_data is NULL");
        break;
      case 2:
        PRINTF("msg_data_size is not correct.");
        break;
      case 3:
        PRINTF("settimeofday() return error.");
        break;
      default:
        PRINTF("Unknown return value.");
        break;
    }
  } while (0);

  return ret;
}

static void get_upgrade_status()
{
  am_service_result_t get_state = {0};
  am_mw_upgrade_status state = {0};

  INFO("get_upgrade_status is called.");
  memset(get_state.data, 0, sizeof(get_state.data));
  g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET,
                            nullptr,
                            0,
                            &get_state,
                            sizeof(get_state));

  memcpy(&state, get_state.data, sizeof(state));
  if (state.in_progress) {
    printf("Upgrade firmware is in progress, current state is: ");
  } else {
    printf("Upgrade firmware not in progress, last upgrade state is: ");
  }

  switch (state.state) {
    case AM_MW_NOT_KNOWN:
      printf("\n\tUpgrade firmware status unknown.\n");
      break;
    case AM_MW_UPGRADE_PREPARE:
      printf("\n\tPreparing upgrade firmware resources...\n");
      break;
    case AM_MW_UPGRADE_PREPARE_FAIL:
      printf("\n\tFailed to prepare upgrade firmware resources.\n");
      break;
    case AM_MW_DOWNLOADING_FW:
      printf("\n\tDownloading firmware...\n");
      break;
    case AM_MW_DOWNLOAD_FW_COMPLETE:
      printf("\n\tDownload firmware complete.\n");
      break;
    case AM_MW_DOWNLOAD_FW_FAIL:
      printf("\n\tFailed to download firmware.\n");
      break;
    case AM_MW_INIT_PBA_SYS_DONE:
      printf("\n\tEntering PBA system and prepare resources...\n");
      break;
    case AM_MW_INIT_PBA_SYS_FAIL:
      printf("\n\tFailed to entering PBA system\n");
      break;
    case AM_MW_UNPACKING_FW:
      printf("\n\tUnpacking upgrade firmware...\n");
      break;
    case AM_MW_UNPACK_FW_DONE:
      printf("\n\tUnpack upgrade firmware done.\n");
      break;
    case AM_MW_UNPACK_FW_FAIL:
      printf("\n\tFailed to unpack upgrade firmware.\n");
      break;
    case AM_MW_FW_INVALID:
      printf("\n\tFirmware is invalid.\n");
      break;
    case AM_MW_WRITEING_FW_TO_FLASH:
      printf("\n\tUpdate flash with unpacked upgrade firmware...\n");
      break;
    case AM_MW_WRITE_FW_TO_FLASH_DONE:
      printf("\n\tUpgrade firmware write to flash done.\n");
      break;
    case AM_MW_WRITE_FW_TO_FLASH_FAIL:
      printf("\n\tFailed to write upgrade firmware to flash.\n");
      break;
    case AM_MW_INIT_MAIN_SYS:
      printf("\n\tPreparing to back to main system...\n");
      break;
    case AM_MW_UPGRADE_SUCCESS:
      printf("\n\tUpgrade firmware success.\n");
      break;
    default:
      printf("\n\tCan not resolve this status, got unknown status\n");
      break;
  }
}

static void set_led_state()
{
  string user_input;
  int32_t gpio_id, led_on, blink, on_time, off_time;
  am_service_result_t retv = {0};
  am_mw_led_config mw_led_cfg = {0};

  printf("Please input settings(format: gpio_id,led_on,blink,"
      "on_time,off_time  e.g. 1,0,1,1,2):\n");
  cin >> user_input;
  if (sscanf(user_input.c_str(),
             "%d,%d,%d,%d,%d",
             &gpio_id,
             &led_on,
             &blink,
             &on_time,
             &off_time) == 5) {

    if (gpio_id > -1) {
      mw_led_cfg.gpio_id = gpio_id;
      mw_led_cfg.led_on = (bool)led_on;
      mw_led_cfg.blink_flag = (bool)blink;
      mw_led_cfg.on_time = on_time;
      mw_led_cfg.off_time = off_time;
      g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET,
                                &mw_led_cfg,
                                sizeof(mw_led_cfg),
                                &retv,
                                sizeof(retv));
      if (retv.ret == 1) {
        PRINTF("Invalid led cfg parameters");
      } else if (retv.ret == 2) {
        PRINTF("Failed to get upgrade instance!");
      } else if (retv.ret == 3) {
        PRINTF("Failed to set led state");
      } else {
        printf("Set gpio%d led state successfully\n", gpio_id);
      }
    } else {
      printf("Invalid gpio id, it must greater than 0\n");
    }
  } else {
    printf("Invalid led cfg format!!!\n");
  }
}

static void get_led_state()
{
  string user_input;
  int32_t gpio_id = -1;
  am_service_result_t led_cfg = {0};
  am_mw_led_config mw_led_cfg = {0};

  printf("Please input gpio id:\n");
  cin >> user_input;
  if (sscanf(user_input.c_str(), "%d", &gpio_id) == 1 && gpio_id > -1) {
    memset(led_cfg.data, 0, sizeof(led_cfg.data));
    g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET,
                              &gpio_id,
                              sizeof(gpio_id),
                              &led_cfg,
                              sizeof(led_cfg));
    if (led_cfg.ret == 0) {
      memcpy(&mw_led_cfg ,led_cfg.data, sizeof(led_cfg));
      printf("gpio id: %d\n", mw_led_cfg.gpio_id);
      printf("gpio led_on: %s\n", mw_led_cfg.led_on ? "on" : "off");
      printf("gpio blink: %s\n", mw_led_cfg.blink_flag ? "on":"off");
      printf("gpio on_time: %dms\n", mw_led_cfg.on_time * 100);
      printf("gpio off_time: %dms\n", mw_led_cfg.off_time * 100);
    } else {
      printf("Failed to get led state, maybe gpio%d not initialized before\n",
             gpio_id);
    }
  } else {
    printf("Invalid gpio id!\n");
  }
}

static void uninit_led()
{
  string user_input;
  int32_t gpio_id = -1;
  am_service_result_t retv = {0};

  printf("Please input gpio id:\n");
  cin >> user_input;
  if (sscanf(user_input.c_str(), "%d", &gpio_id) == 1 && gpio_id > -1) {
    g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT,
                              &gpio_id,
                              sizeof(gpio_id),
                              &retv,
                              sizeof(retv));
    switch (retv.ret) {
      case 0:
        printf("Uninit gpio%d led done.\n", gpio_id);
        break;
      case 1:
        PRINTF("gpio_id ptr is NULL");
        break;
      case 2:
        PRINTF("gpio_id format error.");
        break;
      case 3:
        PRINTF("ledhandler get instance fail.");
        break;
      case 4:
        PRINTF("Invalid gpio id.");
        break;
      case 5:
        PRINTF("Fail to uninit gpio led , it might not be initialized before.");
        break;
      default:
        PRINTF("Unknown return value.");
        break;
    }
  } else {
    printf("Invalid gpio id!\n");
  }
}

static void uninit_all_led()
{
  am_service_result_t retv = {0};

  g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL,
                            nullptr,
                            0,
                            &retv,
                            sizeof(retv));
  if (retv.ret) {
    PRINTF("ledhandler get instance fail");
  } else {
    printf("Uninit all leds done.\n");
  }
}

static void show_led_menu()
{
  printf("\n------------------ led settings ------------------------\n");
  printf(" s -- set led state\n");
  printf(" g -- get led state\n");
  printf(" u -- uninit led\n");
  printf(" d -- uninit all leds\n");
  printf(" q -- exit to main menu\n");
  printf("\n--------------------------------------------------------\n");
  printf(">\n");
}

static void led_settings()
{
  string in_str;
  bool is_run = true;

  while (is_run) {
    show_led_menu();
    cin >> in_str;
    switch (in_str[0]) {
      case 's':
      case 'S': {
        set_led_state();
      } break;
      case 'g':
      case 'G': {
        get_led_state();
      } break;
      case 'u':
      case 'U': {
        uninit_led();
      } break;
      case 'd':
      case 'D': {
        uninit_all_led();
      } break;
      case 'q':
      case 'Q': {
        is_run = false;
      } break;
      default: {
        printf("No such option, please try again.\n");
      } break;
    }
  }
}

static void get_ntp()
{
  int32_t enable, day, hour, minute, second;
  char ntp_cfg[256] = {0};
  am_service_result_t ntp_settings = {0};
  memset(ntp_settings.data, 0, sizeof(ntp_settings.data));
  g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET,
                            nullptr,
                            0,
                            &ntp_settings,
                            sizeof(ntp_settings));
  memcpy(ntp_cfg, ntp_settings.data, sizeof(ntp_cfg) - 1);
  printf("current ntp setting is: %s.\n", ntp_cfg);
  if (5 != sscanf(ntp_cfg,
                  "%d,%d/%d/%d/%d,",
                  &enable,
                  &day,
                  &hour,
                  &minute,
                  &second)) {
    PRINTF("Failed to get ntp config.");
  } else {
    char *ntp_server = strrchr(ntp_cfg, ',') + 1;
    printf("ntp enable: %s\n"
           "ntp update interval: %dday(s) %dhour(s) %dminute(s) %dsecond(s)\n"
           "ntp server list:\n%s\n",
           enable ? "true":"false",
           day,
           hour,
           minute,
           second,
           ntp_server);
  }
}

static void set_ntp()
{
  string usr_input;
  am_service_result_t retv = {0};

  do {
    get_ntp();
    printf("\nTips:\n");
    printf("  NTP config format is:\n"
        "  enable,update_interval(Day/Hour/Minute/Second),server1 server2\n"
        "  (e.g: 1,0/1/0/0,hk.pool.ntp.org time.nist.gov)\n");
    printf("Please input new NTP settings:\n");

    do {
      getline(cin, usr_input);
    } while (usr_input.empty());

    g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET,
                              (void *)usr_input.c_str(),
                              usr_input.length(),
                              &retv,
                              sizeof(retv));
    switch (retv.ret) {
      case 0:
        printf("set NTP successfully.\n");
        break;
      case 1:
        PRINTF("ntp config format is wrong.");
        break;
      case 2:
        PRINTF("msg_data_size is not correct.");
        break;
      case 3:
        PRINTF("server_addr error.");
        break;
      case 4:
        PRINTF("Fail to create ntp thread.");
        break;
      default:
        PRINTF("Unknown return value.");
        break;
    }
  } while (0);
}

static void get_firmware_version()
{
  INFO("get_firmware_version is called.");
  am_service_result_t retv = {0};
  am_mw_firmware_version_all firmware_version = {0};

  memset(retv.data, 0, sizeof(retv.data));
  memset(&firmware_version, 0, sizeof(firmware_version));

  g_api_helper->method_call(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET,
                            nullptr,
                            0,
                            &retv,
                            sizeof(retv));
  memcpy(&firmware_version, retv.data, sizeof(firmware_version));
  printf("\nrelease date: %u/%u/%u\n",
         firmware_version.rel_date.year,
         firmware_version.rel_date.month,
         firmware_version.rel_date.day);
  printf("software version: %u.%u.%u\n",
         firmware_version.sw_version.major,
         firmware_version.sw_version.minor,
         firmware_version.sw_version.trivial);
  printf("release candidate: RC%u\n", firmware_version.rc_number);
  printf("linux kernel version: %u.%u.%u\n",
         firmware_version.linux_kernel_version.major,
         firmware_version.linux_kernel_version.minor,
         firmware_version.linux_kernel_version.trivial);
  printf("linux_sha1: %s\n\n", firmware_version.linux_sha1);

}

static void show_main_menu ()
{
  printf("\n=============== oryx system service test =================\n");
  printf(" 1 -- set system time\n");
  printf(" 2 -- get system time\n");
  printf(" 3 -- set ntp\n");
  printf(" 4 -- get ntp\n");
  printf(" 5 -- led settings\n");
  printf(" 6 -- upgrade settings\n");
  printf(" 7 -- get upgrade status\n");
  printf(" 8 -- get firmware version\n");
  printf(" 0 -- quit\n");
  printf("==========================================================\n");
  printf(">\n");
}

int main (int argc, char **argv)
{
  string input_str;
  int32_t input_num = 0;
  bool run = true;

  signal (SIGINT , sigstop);
  signal (SIGTERM, sigstop);
  signal (SIGQUIT, sigstop);

  if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    ERROR ("Failed to get an instance of AMAPIHelper!");
    return -1;
  }

  while (run) {
    show_main_menu();
    cin >> input_str;
    if (!isdigit((input_str.c_str())[0])) {
      printf("Input is not digit.\n\n");
      continue;
    }

    input_num = atoi(input_str.c_str());
    switch (input_num) {
      case 0: {
        run = false;
      } break;
      case 1: {
        set_system_time();
      } break;
      case 2: {
        get_system_time();
      } break;
      case 3: {
        set_ntp();
      } break;
      case 4: {
        get_ntp();
      } break;
      case 5: {
        led_settings();
      } break;
      case 6: {
        upgrade_settings();
      } break;
      case 7: {
        get_upgrade_status();
      } break;
      case 8: {
        get_firmware_version();
      } break;

      default: {
        printf("No such option, try again!");
      } break;
    }
  }

  return 0;
}
