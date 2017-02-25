/*******************************************************************************
 * am_mp_led.cpp
 *
 * History:
 *   Mar 20, 2015 - [longli] created file
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
#include <fcntl.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_led.h"
#include "devices.h"

enum {
  LED_STATE_SET     = 0,
  LED_RESULT_SAVE   = 1
};

//======================== LED ============================================
typedef struct{
    int32_t gpio_id;
    int32_t led_state;
}mp_led_msg;

//static struct AMILEDHandlerPtr ledhandler_ptr = NULL;

static int32_t gpio_export(const int32_t gpio_id, const int32_t gpio_export)
{
  int32_t ret = 0;
  int32_t fd = -1;
  int32_t access_ret = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};


  if (gpio_id > -1) {
    sprintf(buf, "/sys/class/gpio/gpio%d", gpio_id);
    access_ret = access(buf, F_OK);
    if ((access_ret && gpio_export) || (!access_ret && !gpio_export)) {
      sprintf(buf, "/sys/class/gpio/%s", gpio_export ? "export" : "unexport");
      if ((fd = open(buf, O_WRONLY)) < 0) {
        ret = -1;
        printf("open %s fail\n", buf);
      } else {
        sprintf(vbuf, "%d", gpio_id);
        int32_t len = (int32_t)strlen(vbuf);
        if (len != write(fd, vbuf, len)) {
          ret = -1;
          printf("write %s to %s fail.\n", vbuf, buf);
        }
        close(fd);
      }
    }
  } else {
    printf("Invalid gpio id: %d\n", gpio_id);
    ret = -1;
  }

  return ret;
}

static int32_t set_gpio_direction_out(const int32_t gpio_id,
                                      const int32_t gpio_out)
{
  int32_t ret = 0;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
  if ((fd = open(buf, O_WRONLY)) < 0) {
    ret = -1;
    printf("Fail to open %s\n", buf);
  } else {
    int32_t len = 0;
    if (gpio_out) {
      sprintf(vbuf, "%s","out");
      len = 3;
    } else {
      sprintf(vbuf, "%s","in");
      len = 2;
    }

    if (len != write(fd, vbuf, len)) {
      ret = -1;
      printf("Fail to write %s to %s", vbuf, buf);
    }
    close(fd);
  }

  return ret;
}



static int32_t light_gpio_led(const int32_t gpio_id, const int32_t led_on)
{
  int32_t ret = 0;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
  fd = open(buf, O_RDWR);
  if (fd < 0) {
    printf("Open %s value failed\n", buf);
    ret = -1;
  } else {
    sprintf(vbuf, "%s",(led_on ? "0":"1"));
    if (write(fd, vbuf, 1) != 1) {
      printf("Write gpio%d value failed\n", gpio_id);
      ret = -1;
    }
    close(fd);
  }

  return ret;
}


am_mp_err_t mptool_led_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{

  mp_led_msg led_msg;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case LED_STATE_SET:
      memcpy(&led_msg, from_msg->msg, sizeof(led_msg));
      if (gpio_export(led_msg.gpio_id, led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (set_gpio_direction_out(led_msg.gpio_id, led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (light_gpio_led(led_msg.gpio_id, led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      //memcpy(to_msg->msg, &led_msg, sizeof(led_msg));
      break;
    case LED_RESULT_SAVE:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}

//======================== IR LED =========================================

typedef struct{
    int32_t gpio_id;
    int32_t led_state;
    int32_t pwm_channel;
    int32_t bl_brightness;
}mp_ir_led_msg;

#ifdef IR_LED_NO_GPIO_CONTROL
static int32_t power_on_pwm(int32_t pwm_ch_id, int32_t ir_on)
{
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};
  int32_t ret = 0;
  int32_t fd = -1;

  do {
    if (pwm_ch_id < 0) {
      printf("Invalid pwm channel id: %d", pwm_ch_id);
      ret = -1;
      break;
    }

    sprintf(buf, "/sys/class/backlight/%d.pwm_bl/brightness", pwm_ch_id);
    if ((fd = open(buf, O_RDWR)) < 0) {
      perror("power_on_pwm open");
      ret = -1;
      break;
    }

    sprintf(vbuf, "%d", ir_on ? 0 : 1);
    if (1 != write(fd, vbuf, strlen(vbuf))) {
      perror("write");
      ret = -1;
      break;
    }
  } while (0);

  if (fd > -1) {
    close(fd);
    fd = -1;
  }

  return ret;
}

static int32_t set_pwm_brightness(int32_t pwm_ch_id, int32_t value)
{
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};
  int32_t ret = 0;
  int32_t fd = -1;
  int32_t vbuf_strlen;

  do {
    if (pwm_ch_id < 0) {
      printf("Invalid pwm channel id: %d", pwm_ch_id);
      ret = -1;
      break;
    }

    if (value < 0 || value > 99) {
      printf("Invalid backlight brightness value: %d, range[0 ~ 99]", value);
      ret = -1;
      break;
    }

    sprintf(buf, "/sys/class/backlight/%d.pwm_bl/brightness", pwm_ch_id);
    if ((fd = open(buf, O_RDWR)) < 0) {
      perror("set pwm brightness open");
      ret = -1;
      break;
    }

    sprintf(vbuf, "%d", value);
    vbuf_strlen = strlen(vbuf);
    if (vbuf_strlen != write(fd, vbuf, strlen(vbuf))) {
      perror("write");
      ret = -1;
      break;
    }

  } while (0);

  if (fd > -1) {
    close(fd);
    fd = -1;
  }

  return ret;
}

static int32_t set_ir_brightness(int32_t pwm_ch_id, int32_t value)
{
  int32_t ret = 0;

  if (power_on_pwm(pwm_ch_id, 1)) {
    ret = -1;
  } else {
    if (set_pwm_brightness(pwm_ch_id, value)) {
      ret = -1;
    }
  }

  return ret;
}
#endif

am_mp_err_t mptool_ir_led_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  mp_ir_led_msg ir_led_msg;
  to_msg->result.ret = MP_OK;
  switch (from_msg->stage) {
    case LED_STATE_SET:
      memcpy(&ir_led_msg, from_msg->msg, sizeof(ir_led_msg));
#ifdef IR_LED_NO_GPIO_CONTROL
      if (set_ir_brightness(ir_led_msg.pwm_channel,
                            ir_led_msg.bl_brightness)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
#else
      if (gpio_export(ir_led_msg.gpio_id, ir_led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (set_gpio_direction_out(ir_led_msg.gpio_id, ir_led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (light_gpio_led(ir_led_msg.gpio_id, ir_led_msg.led_state)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
#endif
      break;
    case LED_RESULT_SAVE:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
