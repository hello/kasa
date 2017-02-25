/*
 *
 * ir_led.c
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
 * Copyright (C) 2015 Ambarella, Inc.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>
#include <errno.h>

#include "devices.h"
#include "img_dev.h"

s32 do_gpio_export(s32 gpio_id, gpio_ex ex)
{
	s32 ret = -1;
	s32 fd = -1;
	char buf[128] = {0};
	char vbuf[4] = {0};

	sprintf(buf, "/sys/class/gpio/%s", ex == GPIO_EXPORT ? "export" : "unexport");
	if ((fd = open(buf, O_WRONLY)) < 0) {
		perror("do_gpio_export open");
	} else {
		sprintf(vbuf, "%d", gpio_id);
		if (strlen(vbuf) != write(fd, vbuf, strlen(vbuf))) {
			perror("write");
		} else {
			ret = 0;
		}
		close(fd);
	}

	return ret;
}

s32 set_gpio_direction(s32 gpio_id, gpio_direction direction)
{
	s32 ret = -1;
	s32 fd = -1;
	char buf[128] = {0};
	char *vbuf = NULL;

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	if ((fd = open(buf, O_WRONLY)) < 0) {
		perror("set_gpio_direction open");
	} else {
		switch(direction) {
			case GPIO_OUT:
				vbuf = "out";
				break;
			case GPIO_IN:
				vbuf = "in";
				break;
			default:
				fprintf(stderr, "Invalid direction!\n");
				break;
		}
		if (vbuf) {
			if (strlen(vbuf) != write(fd, vbuf, strlen(vbuf))) {
				perror("write");
			} else {
				ret = 0;
			}
		}
		close(fd);
	}

	return ret;
}

s32 get_gpio_direction(s32 gpio_id)  //return 1:out, 0:in, -1:error
{
	s32 ret = -1;
	s32 fd = -1;
	char buf[128] = {0};
	char vbuf[4] = {0};

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	if ((fd = open(buf, O_RDONLY)) < 0) {
		perror("get_gpio_direction open");
	} else {
		if (read(fd, vbuf, sizeof(vbuf)) < 0) {
			perror("read");
		} else {
			if (vbuf[0] == 'o') {
				ret = 1;
			} else if (vbuf[0] == 'i') {
				ret = 0;
			} else {
				fprintf(stderr, "Invalid direction!\n");
			}
		}
		close(fd);
	}

	return ret;
}

s32 get_gpio_state(s32 *fd, s32 gpio_id)
{
	char buf[128] = {0};
	char vbuf[4] = {0};

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
	if (*fd < 0) {
		if ((*fd = open(buf, O_RDWR)) < 0) {
			perror("get_gpio_state open");
			return -1;
		}
	}

	if (read(*fd, vbuf, sizeof(vbuf)) < 0) {
		perror("read");
		return -1;
	} else {
		lseek(*fd, 0, SEEK_SET);
	}

	return atoi(vbuf);
}

s32 set_gpio_state(s32 *fd, s32 gpio_id, u32 state)
{
	char buf[128] = {0};
	char *vbuf = NULL;

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
	if (*fd < 0) {
		if ((*fd = open(buf, O_RDWR)) < 0) {
			perror("set_gpio_state open");
			return -1;
		}
	}
	if (state == GPIO_LOW) {
		vbuf = "0";
	} else if (state == GPIO_HIGH) {
		vbuf = "1";
	} else {
		fprintf(stderr, "Invalid gpio state: %d\n", state);
		return -1;
	}
	if (vbuf) {
		if (strlen(vbuf) != write(*fd, vbuf, strlen(vbuf))) {
			perror("write");
			return -1;
		} else {
			lseek(*fd, 0, SEEK_SET);
		}
	}

	return 0;
}

s32 get_adc_value(s32 *fd, s32 adc_ch_id, u32 *value)
{
	char value_str[128] = {0};
	char vbuf[8] = {0};
	char adc_device[128] = "/sys/devices/e8000000.apb/e801d000.adc/adcsys";
	sprintf(vbuf, "adc%d=0x", adc_ch_id);

	if (!value) {
		fprintf(stderr, "Invalid pointer!");
		return -1;
	}
	if (*fd < 0) {
		if ((*fd = open(adc_device, O_RDONLY)) < 0) {
			fprintf(stderr, "get_adc_value open %s\n", strerror(errno));
			return -1;
		}
	}
	if (read(*fd, value_str, sizeof(value_str)) <= 0) {
		perror("read");
		return -1;
	} else {
		sscanf(strstr(value_str, vbuf), "%*[^x]x%x", value);
		lseek(*fd, 0, SEEK_SET);
	}

	return 0;
}

s32 get_pwm_duty(s32 *fd, s32 pwm_ch_id)
{
	char buf[128] = {0};
	char vbuf[8] = {0};

	sprintf(buf, "/sys/class/backlight/%d.pwm_bl/brightness", pwm_ch_id);
	if (*fd < 0) {
		if ((*fd = open(buf, O_RDWR)) < 0) {
			perror("pwm duty open");
			return -1;
		}
	}

	if (read(*fd, vbuf, sizeof(vbuf)) < 0) {
		perror("read");
		return -1;
	} else {
		lseek(*fd, 0, SEEK_SET);
	}

	return atoi(vbuf);
}

s32 set_pwm_duty(s32 *fd, s32 pwm_ch_id, s32 duty)
{
	char buf[128] = {0};
	char vbuf[8] = {0};

	if (duty < 0) {
		fprintf(stderr, "Invalid brightness value: %d", duty);
		return -1;
	}
	sprintf(buf, "/sys/class/backlight/%d.pwm_bl/brightness", pwm_ch_id);
	if (*fd < 0) {
		if ((*fd = open(buf, O_RDWR)) < 0) {
			perror("set_pwm_duty open");
			return -1;
		}
	}
	sprintf(vbuf, "%d", duty);
	if (strlen(vbuf) != write(*fd, vbuf, strlen(vbuf))) {
		perror("write");
		return -1;
	} else {
		lseek(*fd, 0, SEEK_SET);
	}

	return 0;
}

/* functions for IR LED control, adc light sensor, pwm brightness*/
s32 ir_led_gpio_fd = -1;
s32 ir_led_pwm_fd = -1;
s32 ir_led_adc_fd = -1;

u32 ir_led_is_supported(void)
{
	return (u32)SUPPORT_IR_LED;
}

s32 ir_led_set_state(u32 value)
{
#ifndef IR_LED_NO_GPIO_CONTROL
	s32 ret = -1;
	ret = set_gpio_state(&ir_led_gpio_fd, GPIO_ID_LED_IR_ENABLE, value);
	if (ret < 0) {
		fprintf(stderr, "ir led set state to %d failed!\n", value);
	}

	return ret;
#else
	return 0;
#endif
}

s32 ir_led_get_state(void)
{
#ifndef IR_LED_NO_GPIO_CONTROL
	s32 ret = -1;
	ret = get_gpio_state(&ir_led_gpio_fd, GPIO_ID_LED_IR_ENABLE);
	if (ret < 0) {
		fprintf(stderr, "ir led get state failed!\n");
	}

	return ret;
#else
	return 0;
#endif
}

s32 ir_led_get_brightness(void)
{
	s32 ret = -1;
	ret = get_pwm_duty(&ir_led_pwm_fd, PWM_CHANNEL_IR_LED);
	if (ret < 0) {
		fprintf(stderr, "ir led get brightness failed!\n");
	}

	return ret;
}

s32 ir_led_set_brightness(s32 brightness)
{
	s32 ret = -1;
	ret = set_pwm_duty(&ir_led_pwm_fd, PWM_CHANNEL_IR_LED, brightness);
	if (ret < 0) {
		fprintf(stderr, "ir led set brightness to %d failed!\n", brightness);
	}

	return ret;
}

s32 ir_led_get_adc_value(u32 *value)
{
	s32 ret = -1;
	ret = get_adc_value(&ir_led_adc_fd, IR_LED_ADC_CHANNEL, value);
	if (ret < 0) {
		fprintf(stderr, "ir led get adc value failed!\n");
	}

	return ret;
}

s32 ir_led_init(u32 init) /* init=1:initiate IR LED, init=0:de-initiate IR LED*/
{
	s32 ret = 0;
	do {
		if (!init) {
			if (ir_led_pwm_fd != -1) {
				if ((ret = close(ir_led_pwm_fd)) < 0) {
					perror("close");
					break;
				}
				ir_led_pwm_fd = -1;
			}
			if (ir_led_adc_fd != -1) {
				if ((ret = close(ir_led_adc_fd)) < 0) {
					perror("close");
					break;
				}
				ir_led_adc_fd = -1;
			}
		}
#ifndef IR_LED_NO_GPIO_CONTROL
		char gpio_addr[128] = { 0 };
		sprintf(gpio_addr, "/sys/class/gpio/gpio%d",
		GPIO_ID_LED_IR_ENABLE);
		if (init) {
			//set IR CUT control GPIO to default value 0, make sure it is covered
			if (0 != access(gpio_addr, F_OK)) {
				if ((ret = do_gpio_export(GPIO_ID_LED_IR_ENABLE, GPIO_EXPORT)) < 0) {
					fprintf(stderr, "export gpio%d failed!\n", GPIO_ID_LED_IR_ENABLE);
					break;
				}
				if ((ret = set_gpio_direction(GPIO_ID_LED_IR_ENABLE, GPIO_OUT)) < 0) {
					fprintf(stderr,
									"set gpio%d direction failed!\n",
									GPIO_ID_LED_IR_ENABLE);
					break;
				}
			}
		} else {
			if (ir_led_gpio_fd != -1) {
				if ((ret = close(ir_led_gpio_fd)) < 0) {
					perror("close");
					break;
				}
				ir_led_gpio_fd = -1;
			}
		}
#endif
	} while (0);

	if (ret < 0) {
		if (init) {
			fprintf(stderr, "ir led init failed!\n");
		} else {
			fprintf(stderr, "ir led de-init failed!\n");
		}
	}

	return ret;
}

/* functions for IR cut switch */
s32 ir_cut_gpio_fd = -1;

u32 ir_cut_is_supported(void)
{
	return (u32)SUPPORT_IR_CUT;
}

s32 ir_cut_set_state(u32 value)
{
	s32 ret = -1;
	ret = set_gpio_state(&ir_cut_gpio_fd, GPIO_ID_IR_CUT_CTRL, value);
	if (ret < 0) {
		fprintf(stderr, "ir cut set state to %d failed!\n", value);
	}

	return ret;
}

s32 ir_cut_get_state(void)
{
	s32 ret =- 1;
	ret = get_gpio_state(&ir_cut_gpio_fd, GPIO_ID_IR_CUT_CTRL);
	if (ret < 0) {
		fprintf(stderr, "ir cut get state failed!\n");
	}

	return ret;
}

s32 ir_cut_init(u32 init)/* init=1:initiate IR cut, init=0:de-initiate IR cut*/
{
	s32 ret = 0;
	char gpio_addr[128] = { 0 };

	sprintf(gpio_addr, "/sys/class/gpio/gpio%d", GPIO_ID_IR_CUT_CTRL);
	do {
		if (init) {
			//set IR CUT control GPIO to default value 0, make sure it is covered
			if (0 != access(gpio_addr, F_OK)) {
				if ((ret = do_gpio_export(GPIO_ID_IR_CUT_CTRL, GPIO_EXPORT)) < 0) {
					fprintf(stderr, "export gpio%d failed!\n", GPIO_ID_IR_CUT_CTRL);
					break;
				}
				if ((ret = set_gpio_direction(GPIO_ID_IR_CUT_CTRL, GPIO_OUT)) < 0) {
					fprintf(stderr,
									"set gpio%d direction failed!\n",
									GPIO_ID_IR_CUT_CTRL);
					break;
				}
			}
		} else {
			if (ir_cut_gpio_fd != -1) {
				if ((ret = close(ir_cut_gpio_fd)) < 0) {
					perror("close");
					break;
				}
				ir_cut_gpio_fd = -1;
			}
		}
	} while (0);

	if (ret < 0) {
		if (init) {
			fprintf(stderr, "ir cut init failed!\n");
		} else {
			fprintf(stderr, "ir cut de-init failed!\n");
		}
	}

	return ret;
}
