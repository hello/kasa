/*******************************************************************************
 * test_led.cpp
 *
 * History:
 *   2015-2-6 - [longli] created file
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
#include <iostream>
#include "devices.h"
#include "am_led_handler_if.h"

using namespace std;

struct AMGpioLedList {
    const int32_t gpio_id;
    const char *des;
};

/* used by class AMLEDHandler */
struct AMGpioLedList am_led_list[] = {
  {GPIO_ID_LED_POWER, "power_led"},
  {GPIO_ID_LED_NETWORK, "network_led"},
  {GPIO_ID_LED_SD_CARD, "sd_card_led"},
  {-1, 0}
};

static void list_supported_led()
{
  int32_t count = 0;
  printf("---------------------------------------------\n");
  printf("id\tdescription\n");
  for (uint32_t i = 0;
      i < (sizeof(am_led_list)/sizeof(am_led_list[0]) - 1); ++i) {
    if (am_led_list[i].gpio_id > -1) {
      printf(" %d\t%s\n", am_led_list[i].gpio_id, am_led_list[i].des);
      ++count;
    }
  }
  if (count) {
    printf("---------------------------------------------\n");
  } else {
    printf("\nNo supported gpio led found.\n");
  }
}

void main_menu()
{
  printf("\n*****************************************************************************\n"
      "  command:\n"
      "    s --- set led state\n"
      "    g --- get led state\n"
      "    p --- print leds that have been set\n"
      "    l --- supported led list\n"
      "    u --- unset led\n"
      "    d --- unset all led\n"
      "    q --- exit this test\n\n"
      "*****************************************************************************\n");
  printf(">");
  fflush(stdout);
}

int32_t main(int argc, char **argv)
{
  AMILEDHandlerPtr ledhandler_ptr = NULL;
  AMLEDConfig led_cfg;
  bool run = true;
  bool show_menu = true;
  string user_input;
  int32_t gpio_id;

  ledhandler_ptr = AMILEDHandler::get_instance();
  if (!ledhandler_ptr) {
    printf("Get instance failed.\n");
    return -1;
  }

  while (run) {
    if (show_menu) {
      main_menu();
    }
    show_menu = true;
    cin >> user_input;
    switch (user_input[0]) {
      case 'p':
      case 'P':
        ledhandler_ptr->print_led_list();
        break;
      case 'l':
      case 'L':
        list_supported_led();
        break;
      case 's':
      case 'S': {
        int32_t led_on, blink, on_time, off_time;
        printf("Please input settings(format: gpio_id,led_on,blink,"
            "on_time,off_time  e.g. 1,0,1,1,2):\n");
        cin >> user_input;
        if (sscanf(user_input.c_str(), "%d,%d,%d,%d,%d",
                   &gpio_id, &led_on, &blink, &on_time, &off_time) == 5) {

          if (gpio_id > -1) {
            led_cfg.gpio_id = gpio_id;
            led_cfg.led_on = (bool)led_on;
            led_cfg.blink_flag = (bool)blink;
            led_cfg.on_time = on_time;
            led_cfg.off_time = off_time;
            ledhandler_ptr->set_led_state(led_cfg);
          } else {
            printf("Invalid gpio id, it must greater than 0\n");
          }
        } else {
          printf("Invalid led cfg format!!!\n");
        }
      } break;
      case 'g':
      case 'G': {
        printf("Please input gpio id:\n");
        cin >> user_input;
        if (sscanf(user_input.c_str(), "%d", &gpio_id) == 1 && gpio_id > -1) {
          led_cfg.gpio_id = gpio_id;
          if (ledhandler_ptr->get_led_state(led_cfg)) {
            printf("gpio id: %d\n", led_cfg.gpio_id);
            printf("gpio led_on: %s\n", led_cfg.led_on ? "on" : "off");
            printf("gpio blink: %s\n", led_cfg.blink_flag ? "on":"off");
            printf("gpio on_time: %dms\n", led_cfg.on_time * 100);
            printf("gpio off_time: %dms\n", led_cfg.off_time * 100);
          }
        } else {
          printf("Invalid gpio id!\n");
        }
      } break;
      case 'u':
      case 'U': {
        printf("Please input gpio id:\n");
        cin >> user_input;
        if (sscanf(user_input.c_str(), "%d", &gpio_id) == 1 && gpio_id > -1) {
          if(ledhandler_ptr->deinit_led(gpio_id)) {
            printf("Unset gpio id %d done.\n", gpio_id);
          }
        } else {
          printf("Invalid gpio id!\n");
        }
      } break;
      case 'd':
      case 'D':
        ledhandler_ptr->deinit_all_led();
        printf("Unset all gpio done.\n");
        break;
      case 'q':
      case 'Q':
        run = false;
        break;
      default:
        show_menu = false;
        printf(">");
        fflush(stdout);
        break;
    }
  }
}
