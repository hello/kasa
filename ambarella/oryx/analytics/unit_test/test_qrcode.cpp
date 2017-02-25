/*******************************************************************************
 * test_qrcode.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
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
#include <iostream>
#include <string>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include "am_qrcode_if.h"
#include "am_log.h"

using namespace std;

#define DEFAULT_CONFIG_NAME "wifi_config"
#define DEFAULT_BUF_ID      0
#define DEFAULT_TIMEOUT     5
// options and usage
#define NO_ARG    0
#define HAS_ARG   1

static const char *short_options = "f:b:t:h";
static struct option long_options[]={
  {"filename", HAS_ARG, 0, 'f'},
  {"src-buf", HAS_ARG, 0, 'b'},
  {"timeout", HAS_ARG, 0, 't'},
  {0, 0, 0, 0}
};

static int32_t buf_index = DEFAULT_BUF_ID;
static uint32_t timeout = DEFAULT_TIMEOUT;
static string config_file(DEFAULT_CONFIG_NAME);

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
  {"file_name", "specify config_file name with path to save config, ""default: \"wifi_config\""},
  {"0~3", "\tfrom which encode source buffer to read QR code, default: 0"},
  {">=0", "\tspecify timeout interval, default: 5, value 0: blocked until read QR code successfully"},
};

static void usage()
{
  uint32_t i = 0;
  printf("Instruction:\n"
      "1. When using this test_encode, make sure that camera must be in preview"
      " or encoding mode.\n"
      "2. Ambarella QRcode is implemented by using zbar lib.\n"
      "3. Text format that qrcode_parse can recognize:\n"
      "   config_name1(not null):key_name1(not null):key_value1;"
      "key_name2(not null):key_value2;;config_name2...\n\n");

  printf("Usage: test_qrcode [options]\n");
  printf("options:\n");

  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val))
      printf("-%c ", long_options[i].val);
    else
      printf("   ");
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }
}

static void setting_menu()
{
  printf("\n============================================================================\n"
      "  command:\n"
      "    b --- [0~3] set from which encode source buffer to read QR code\n"
      "    f --- set config_file name with path to save config\n"
      "    t --- set timeout interval value\n"
      "    q --- exit to main menu\n"
      "============================================================================\n");
  printf(">");
  fflush(stdout);
}

static bool is_dir(const string &path)
{
  bool ret = false;
  struct stat sb;
  if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
    ret = true;
  } else {
    ret = false;
  }
  return ret;
}

static void settings(AMIQrcode *&qrcode)
{
  bool run = true;
  int32_t index;
  uint32_t time_out;
  string filename;
  string cmd;

  while (run) {
    setting_menu();
    cin >> cmd;
    switch(cmd[0]) {
      case 'b':
      case 'B':
        printf("Please input encode source buffer id [0~3](current value: %d):\n",
               buf_index);
        cin >> index;
        if (cin.fail()) {
          cin.clear();
          printf("Invalid input! Use previous buffer id: %d\n", buf_index);
        } else {
          if (index >= 0 && index < 4) {
            buf_index = index;
            printf("Source buffer id changed successfully.\n");
          } else {
            printf("Invalid id: %d! Use previous buffer id: %d\n",
                   index, buf_index);
          }
        }
        break;
      case 'f':
      case 'F':
        printf("Please input config_file name(current value: %s):\n",
               config_file.c_str());
        cin >> filename;
        if (is_dir(filename)) {
          printf("Invalid filename: %s, it is a directory. "
              "Use previous filename:%s\n",
              filename.c_str(), config_file.c_str());
        } else {
          config_file = filename;
          qrcode->set_config_path(config_file);
          printf("Config file changed successfully.\n");
        }
        break;
      case 't':
      case 'T':
        printf("Please input timeout value(current value: %us):\n",
               timeout);
        cin >> time_out;
        if (cin.fail()) {
          cin.clear();
          printf("Invalid input! Use previous value: %u\n", timeout);
        } else {
          timeout = time_out;
          printf("Timeout value changed successfully.\n");
        }
        break;
      case 'q':
      case 'Q':
        run = false;
        break;
      default:
        printf("Unkown cmd %s\n", cmd.c_str());
        break;
    }
  }
}

static void main_menu()
{
  printf("\n*****************************************************************************\n"
      "  command:\n"
      "    r --- read QR code from camera and store it to config_file\n"
      "    l --- load(parse) QR code config_string from config_file again\n"
      "    g --- get key_value (config_name->key_name:key_value;)\n"
      "    p --- print QR code config\n"
      "    s --- configure settings(buffer id, config file, timeout value)\n"
      "    q --- exit this test\n\n"
      "    Tips:\n"
      "      1.If not run cmd 'r' before, please run it first.\n"
      "      2.When modify config_file manually, please run cmd 'l' to reload config\n"
      "      3.For more information, please run cmd 'test_qrcode -h'.\n"
      "*****************************************************************************\n");
  printf(">");
  fflush(stdout);
}

static bool init_param(int argc, char **argv)
{
  bool ret = true;
  bool show_help = false;
  int32_t ch;
  int32_t opt_index;

  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &opt_index)) != -1) {
    switch (ch) {
      case 'f':
        if (optarg) {
          config_file = optarg;
          if (is_dir(config_file)) {
            printf("Invalid filename: %s, it is a directory. "
                "Use default filename:%s\n",
                config_file.c_str(), DEFAULT_CONFIG_NAME);
            config_file = DEFAULT_CONFIG_NAME;
          }
        } else {
          show_help = true;
        }
        break;
      case 'b':
        if (optarg) {
          if(isdigit(optarg[0])) {
            buf_index = atoi(optarg);
            if (buf_index < 0 || buf_index > 3) {
              printf("Invalid buffer id: %d, use default buffer id: %d\n",
                     buf_index, DEFAULT_BUF_ID);
            }
          } else {
            printf("Invalid buffer id: %s, use default buffer id: %d\n",
                   optarg, DEFAULT_BUF_ID);
          }
        } else {
          show_help = true;
        }
        break;
      case 't':
        if (optarg) {
          if(isdigit(optarg[0])) {
            timeout = atoi(optarg);
          } else {
            printf("Invalid timeout value: %s, use default timeout value: %d\n",
                   optarg, DEFAULT_TIMEOUT);
          }
        } else {
          show_help = true;
        }
        break;
      default:
        show_help = true;
        break;
    }
  }
  if (show_help) {
    usage();
    ret = false;
  }
  return ret;
}

static void get_key_value(AMIQrcode *&qrcode)
{
  string input_config;
  string input;
  string key_value("");
  while (1) {
    printf("In order to get key_value, please input config_name and key_name!\n");
    if(qrcode->print_config_name_list()) {
      printf("(Enter 'e' to exit to main menu)\n");
      printf("Please input config_name:");
      cin >> input;
      if (input == "e" || input == "E")
        return;
      else
        input_config = input;
      while (1) {
        if (qrcode->print_key_name_list(input_config)) {
          printf("(Enter'q' to upper menu, 'e' to main menu)\n");
          printf("Please input key_name:");
          cin >> input;
          if (input == "q" || input == "Q") {
            break;
          } else if (input == "e" || input == "E") {
            return;
          } else {
            key_value = qrcode->get_key_value(input_config, input);
            printf("\nThe key_value is: %s\n\n", key_value.c_str());
          }
        } else {
          printf("key name list is empty! Return to upper menu!\n\n");
          break;
        }
      }
    } else {
      printf("config name list is empty! Return to main menu!\n");
      break;
    }
  }
}
int main(int argc, char **argv)
{
  bool ret = true;
  bool run = true;
  bool show_cmd = true;
  AMIQrcode *qrcode = nullptr;
  string user_input;

  if (!init_param(argc, argv)) {
    return -1;
  }
  qrcode = AMIQrcode::create();
  if (!qrcode) {
    printf("Failed to create an instance of qrcode!\n");
  }

  if (!(qrcode->set_config_path(config_file))) {
    printf("Qrcode set config path fail\n");
    return -1;
  }

  while (run) {
    if (show_cmd) {
      main_menu();
    }
    show_cmd = true;
    cin >> user_input;
    switch (user_input[0]) {
      case 'r':
      case 'R':
        ret = qrcode->qrcode_read(buf_index, timeout);
        if (ret) {
          ret = qrcode->qrcode_parse();
          if (ret) {
            printf("Read QR code successfully from Encode source buffer %d\n",
                   buf_index);
          } else {
            printf("Read QR code successfully, but failed to parse QR code!\n");
          }
        } else {
          printf("Unable to detect QR code and Time out after %d second(s).\n"
              "Please try to check whether you put a QR code picture in front of lens.\n"
              "And whether lens is focused well at the picture.\n", timeout);
        }
        break;
      case 'g':
      case 'G':
        get_key_value(qrcode);
        break;
      case 'l':
      case 'L':
        ret = qrcode->qrcode_parse();
        if (ret) {
          printf("Reload config successfully from config_file!\n");
        } else {
          printf("Failed to reload config from config_file!\n");
        }
        break;
      case 'p':
      case 'P':
        qrcode->print_qrcode_config();
        break;
      case 's':
      case 'S':
        settings(qrcode);
        break;
      case 'q':
      case 'Q':
        run = false;
        break;
      default:
        show_cmd = false;
        printf(">");
        fflush(stdout);
        break;
    }
  }
  qrcode->destroy();

  return 0;
}
