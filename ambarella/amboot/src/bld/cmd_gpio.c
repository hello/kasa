/**
 * bld/cmd_gpio.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#include <bldfunc.h>
#include <ambhw/gpio.h>

/*===========================================================================*/
static void diag_gpio_diag_set(int gpio, int on)
{
	gpio_config_sw_out(gpio);
	if (on) {
		gpio_set(gpio);
	} else {
		gpio_clr(gpio);
	}
}

static void diag_gpio_diag_get(int gpio)
{
	u32 gpio_val;

	gpio_config_sw_in(gpio);
	gpio_val = gpio_get(gpio);

	putstr("gpio[");
	putdec(gpio);
	putstr("] = ");
	putdec(gpio_val);
	putstr("\r\n");
}

static void diag_gpio_diag_hw(int gpio, int alt_func)
{
	gpio_config_hw(gpio, alt_func);
}

static void diag_gpio_diag_pull(int gpio, int val)
{
	u32 gpio_pull_en_base = 0;
	u32 gpio_pull_dir_base = 0;
	u32 gpio_offset;
	u32 en_set = 0;
	u32 en_clr = 0;
	u32 dir_set = 0;
	u32 dir_clr = 0;
	u32 gpio_pull_en;
	u32 gpio_pull_dir;

#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
	if ((gpio >= 0) && (gpio < 32)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_0_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_0_OFFSET);
	} else if ((gpio >= 32) && (gpio < 64)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_1_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_1_OFFSET);
	} else if ((gpio >= 64) && (gpio < 96)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_2_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_2_OFFSET);
	} else if ((gpio >= 96) && (gpio < 128)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_3_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_3_OFFSET);
	} else if ((gpio >= 128) && (gpio < 160)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_4_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_4_OFFSET);
	} else if ((gpio >= 160) && (gpio < 192)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_5_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_5_OFFSET);
	} else if ((gpio >= 192) && (gpio < 224)) {
		gpio_pull_en_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_EN_6_OFFSET);
		gpio_pull_dir_base = GPIO_PAD_PULL_REG(GPIO_PAD_PULL_DIR_6_OFFSET);
	}
#endif
	gpio_offset = gpio % 32;
	switch(val & 0x02) {
	case 0x02:
		en_set = (0x01 << gpio_offset);
		break;
	default:
		en_clr = (0x01 << gpio_offset);
		break;
	}
	switch(val & 0x01) {
	case 0x01:
		dir_set = (0x01 << gpio_offset);
		break;
	default:
		dir_clr = (0x01 << gpio_offset);
		break;
	}

	if (gpio_pull_en_base && gpio_pull_dir_base) {
		gpio_pull_en = readl(gpio_pull_en_base);
		gpio_pull_en |= en_set;
		gpio_pull_en &= ~en_clr;

		gpio_pull_dir = readl(gpio_pull_dir_base);
		gpio_pull_dir |= dir_set;
		gpio_pull_dir &= ~dir_clr;

		writel(gpio_pull_dir_base, gpio_pull_dir);
		writel(gpio_pull_en_base, gpio_pull_en);
	}
}

/*===========================================================================*/
static int cmd_gpio(int argc, char *argv[])
{
	u32 gpio_id = 0xFFFFFFFF;

	if (argc < 3) {
		return -1;
	}

	strtou32(argv[2], &gpio_id);
	if (strcmp(argv[1], "get") == 0) {
		diag_gpio_diag_get(gpio_id);
	} else if (strcmp(argv[1], "set") == 0) {
		diag_gpio_diag_set(gpio_id, 1);
	} else if (strcmp(argv[1], "clr") == 0) {
		diag_gpio_diag_set(gpio_id, 0);
	} else if (strcmp(argv[1], "poff") == 0) {
		diag_gpio_diag_pull(gpio_id, 0);
	} else if (strcmp(argv[1], "pdown") == 0) {
		diag_gpio_diag_pull(gpio_id, 2);
	} else if (strcmp(argv[1], "pup") == 0) {
		diag_gpio_diag_pull(gpio_id, 3);
	} else if (strcmp(argv[1], "hw") == 0) {
		u32 alt_func = 1;
		if (argc >= 4) {
			strtou32(argv[3], &alt_func);
		}
		diag_gpio_diag_hw(gpio_id, alt_func);
	}

	return 0;
}

/*===========================================================================*/
static char help_gpio[] =
	"gpio [set|clr|get|pup|pdown|poff] id - Basic GPIO function\r\n"
	"gpio [hw] id func - Set HW mode\r\n"
	"Test GPIO.\r\n";
__CMDLIST(cmd_gpio, "gpio", help_gpio);

