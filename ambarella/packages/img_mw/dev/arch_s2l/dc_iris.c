/*
 *
 * dc_iris.c
 *
 * History:
 *	2014/09/17 - [Bin Wang] Created this file
 *
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

#include "devices.h"
#include "img_dev.h"

static u32 support_dc_iris = (u32)SUPPORT_DC_IRIS;
static u32 dc_iris_inited = 0;
static u32 dc_iris_enabled = 0;

static int pwm_proc = -1;
static int pwm_channel = (int)PWM_CHANNEL_DC_IRIS;

static int my_itoa(int num,  char *str,  int base)
{
	char index[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	u32 unum;
	int i = 0, j, k;

	if (base == 10 && num < 0) {
		unum = (u32)-num;
		str[i++] = '-';
	} else {
		unum = (u32)num;
	}

	do {
		str[i++] = index[unum % (u32)base];
		unum /= base;
	} while (unum);
	str[i] = '\0';

	if (str[0] == '-') {
		k = 1;
	} else {
		k = 0;
	}
	for (j = k; j <= (i - 1) / 2 + k; ++j) {
		num = str[j];
		str[j] = str[i-j-1+k];
		str[i-j-1+k] = num;
	}

	return 0;
}

int dc_iris_update_duty(int duty_cycle)
{
	u32 n = 0;
	char value[16];

	if (dc_iris_enabled != 1) {
		fprintf(stderr, "dc iris not enabled for update duty!\n");
		return -1;
	}

	my_itoa(duty_cycle, value, 10);
	if (pwm_proc > 0) {
		n = write(pwm_proc, value, strlen(value));
		if (n != strlen(value)) {
			fprintf(stderr, "pwm duty [%s] set error [%d].\n", value, n);
			return -1;
		}
	} else {
		fprintf(stderr, "pwm proc should be opened when dc iris is on!\n");
	}

	return 0;
}

static int open_pwm_proc(void)
{
	char pwm_device[64];

	if (pwm_proc < 0) {
		sprintf(pwm_device, "/sys/class/backlight/%d.pwm_bl/brightness",
			pwm_channel);
		if ((pwm_proc = open(pwm_device, O_RDWR | O_TRUNC)) < 0) {
			printf("open %s failed!\n",  pwm_device);
			return -1;
		}
	}

	return 0;
}

int dc_iris_init(void)
{
	if (dc_iris_inited == 0) {
		open_pwm_proc();
	} else {
		fprintf(stderr, "dc iris already inited!\n");
	}
	dc_iris_inited = 1;

	return 0;
}

static int close_pwm_proc(void)
{
	if (pwm_proc >= 0) {
		close(pwm_proc);
		pwm_proc = -1;
	}

	return 0;
}

int dc_iris_deinit(void)
{
	if (dc_iris_inited) {
		close_pwm_proc();
	} else {
		fprintf(stderr, "dc iris already deinited!\n");
	}
	dc_iris_inited = 0;

	return 0;
}

int dc_iris_enable(void)
{
	if (dc_iris_inited == 0) {
		dc_iris_init();
	}

	dc_iris_enabled = 1;

	return 0;
}

int dc_iris_disable(void)
{
	if (dc_iris_inited == 0) {
		dc_iris_init();
	}

	if (dc_iris_enabled == 1) {
		dc_iris_enabled = 0;
	}

	return 0;
}

u32 dc_iris_is_supported(void)
{
	 return support_dc_iris;
}
