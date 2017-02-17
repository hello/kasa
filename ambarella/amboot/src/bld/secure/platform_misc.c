/*******************************************************************************
 * platform_misc.c
 *
 * History:
 *  2015/11/24 - [Zhi He] create file for secure boot logic
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

#include <bldfunc.h>
#include <ambhw/idc.h>
#include <ambhw/gpio.h>
#include <ambhw/dma.h>
#include <ambhw/fio.h>
#include <ambhw/i2s.h>
#include <ambhw/timer.h>
#include <sdmmc.h>

#include "secure_boot.h"

void rct_timer_delay_ticks(u32 ticks)
{
	u32 begin_time = rct_timer_get_tick();
	u32 end_time = begin_time + ticks;
	u32 cur_time = 0;

	if (end_time > begin_time) {
		while (1) {
			cur_time = rct_timer_get_tick();
			if (cur_time > end_time)
				break;
		}
	} else {
		while (1) {
			cur_time = rct_timer_get_tick();
			if ((cur_time > end_time) && (cur_time < begin_time))
				break;
		}
	}
}

u32 rct_timer_get_frequency_div_1000()
{
#if (RCT_TIMER_INSTANCES >= 1)
	return (REF_CLK_FREQ / 1000);
#else
	return (get_apb_bus_freq_hz() / 1000);
#endif
}

int uart_get_readable_string(const char* begin, int head_length, char* string, int number)
{
	int c;
	int i = 0;

uart_get_readable_string_resync_get:

	for (i = 0; i < head_length; i++) {
		while (uart_poll() == 0) {
			continue;
		}

		c = uart_read();

		if ((c == '\r') || (c == '\n')) {
			goto uart_get_readable_string_resync_get;
		}

		if (c != begin[i]) {
			putchar((char) c);
			putstr(":read header fail\r\n");
			return (-1);
		}
	}

	for (i = 0; i < number; i++) {
		while (uart_poll() == 0) {
			continue;
		}

		c = uart_read();
		if ((c == '\r') || (c == '\n')) {
			putstr("read content fail\r\n");
			return (-3);
		}
		string[i] = c;
	}

	return 0;
}

int uart_get_readable_string_variable_length(const char* begin, char* string, int number)
{
	int c;
	int i = 0;

uart_get_readable_string_variable_length_resync_get:

	for (i = 0; i < 4; i++) {
		while (uart_poll() == 0) {
			continue;
		}

		c = uart_read();

		if ((c == '\r') || (c == '\n')) {
			goto uart_get_readable_string_variable_length_resync_get;
		}

		if (c != begin[i]) {
			putchar((char) c);
			putstr(":read header fail\r\n");
			return (-1);
		}
	}

	for (i = 0; i < number; i++) {
		while (uart_poll() == 0) {
			continue;
		}

		c = uart_read();
		if ((c == '\r') || (c == '\n')) {
			break;
		}
		string[i] = c;
	}

	if (i == number) {
		putstr("read string exceed length\r\n");
		return (-2);
	}

	return 0;
}

int uart_get_rsakey_1024(void* p_key)
{
	int ret = 0;
	rsa_key_t* p_rsa_key = (rsa_key_t*) p_key;

	ret = uart_get_readable_string("N = ", 4, p_rsa_key->n, 256);
	if (ret) {
		putstr("read n fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string_variable_length("E = ", p_rsa_key->e, 16);
	if (ret) {
		putstr("read e fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("D = ", 4, p_rsa_key->d, 256);
	if (ret) {
		putstr("read d fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("P = ", 4, p_rsa_key->p, 128);
	if (ret) {
		putstr("read p fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("Q = ", 4, p_rsa_key->q, 128);
	if (ret) {
		putstr("read q fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("DP = ", 5, p_rsa_key->dp, 128);
	if (ret) {
		putstr("read dp fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("DQ = ", 5, p_rsa_key->dq, 128);
	if (ret) {
		putstr("read dq fail\r\n");
		return ret;
	}

	ret = uart_get_readable_string("QP = ", 5, p_rsa_key->qp, 128);
	if (ret) {
		putstr("read qp fail\r\n");
		return ret;
	}

	return 0;
}

void uart_print_rsakey_1024(void* p_key)
{
	rsa_key_t* p_rsa_key = (rsa_key_t*) p_key;

	uart_putstr("print rsa key for debug:\r\n");

	uart_putstr("N = ");
	uart_putstr((const char*) p_rsa_key->n);
	uart_putstr("\r\n");

	uart_putstr("E = ");
	uart_putstr((const char*) p_rsa_key->e);
	uart_putstr("\r\n");

	uart_putstr("D = ");
	uart_putstr((const char*) p_rsa_key->d);
	uart_putstr("\r\n");

	uart_putstr("P = ");
	uart_putstr((const char*) p_rsa_key->p);
	uart_putstr("\r\n");

	uart_putstr("Q = ");
	uart_putstr((const char*) p_rsa_key->q);
	uart_putstr("\r\n");

	uart_putstr("DP = ");
	uart_putstr((const char*) p_rsa_key->dp);
	uart_putstr("\r\n");

	uart_putstr("DQ = ");
	uart_putstr((const char*) p_rsa_key->dq);
	uart_putstr("\r\n");

	uart_putstr("QP = ");
	uart_putstr((const char*) p_rsa_key->qp);
	uart_putstr("\r\n");

	return;
}

int uart_get_onechar_blocked()
{
	while (uart_poll() == 0) {
		continue;
	}

	return uart_read();
}

