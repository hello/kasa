/**
 * system/src/bld/diag_ir.c
 *
 * History:
 *    2005/09/25 - [Charles Chiou] created file
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
#include <ambhw/ir.h>
#include <ambhw/gpio.h>
#include <ambhw/uart.h>

extern void rct_set_ir_pll(void);

/**
 * Run diagnostic on IR: continuously display the current value.
 */
void diag_ir(u32 ir_in_io)
{
	int i;
	u32 data;

	/* Set IR sampling frequency */
	rct_set_ir_pll();

	/* config IR GPIO */
	gpio_config_hw(GPIO(ir_in_io),1);

	uart_putstr("running IR diagnostics... \r\n");
	uart_putstr("press any key to quit! \r\n");

	writel(IR_CONTROL_REG, IR_CONTROL_RESET);
	writel(IR_CONTROL_REG, IR_CONTROL_ENB);

	for (i = 0; ; i++) {
		if (uart_poll())
			break;

		while (readl(IR_STATUS_REG) == 0x0);
		data = readl(IR_DATA_REG);
		uart_putdec(data);
		uart_putchar(' ');
		if (i == 8) {
			uart_putstr("\r\n");
			i = 0;
		}
	}

	writel(IR_CONTROL_REG, 0x0);

	uart_putstr("\r\ndone!\r\n");
}

static int cmd_ir(int argc, char *argv[])
{
	u32 gpio;
	if(argc < 2){
		uart_putstr("invalid argument!\r\n");
		return -1;
	}

	strtou32(argv[1], &gpio);

	diag_ir(gpio);

	return 0;
}

static char help_ir[] =
	"ir- Diag ir\r\n"
	;
__CMDLIST(cmd_ir, "ir", help_ir);

