/**
 * bld/uart.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
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


#include <amboot.h>
#include <ambhw/uart.h>
#include <ambhw/gpio.h>

/* ==========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_UART0)
#define CONFIG_BUILD_WITH_UART

#define uart_writeb(p, v)		__raw_writeb(v, UART0_REG(p))
#define uart_readb(p)			__raw_readb(UART0_REG(p))

#else
#undef CONFIG_BUILD_WITH_UART

#define uart_writeb(p, v)
#define uart_readb(p)

#endif

#if defined(AMBOOT_UART_19200)
static const int uart_baud = 19200;
#elif defined(AMBOOT_UART_38400)
static const int uart_baud = 38400;
#elif defined(AMBOOT_UART_57600)
static const int uart_baud = 57600;
#else
static const int uart_baud = 115200;
#endif

/* ==========================================================================*/
void uart_init(void)
{
#if defined(CONFIG_BUILD_WITH_UART)
	u32 clk;
	u16 dl;

	rct_set_uart_pll();
	uart_writeb(UART_SRR_OFFSET, 0x00);
	clk = get_uart_freq_hz();
	dl = clk * 10 / uart_baud / 16;
	if (dl % 10 >= 5) {
		dl = (dl / 10) + 1;
	} else {
		dl = (dl / 10);
	}
	uart_writeb(UART_LC_OFFSET, UART_LC_DLAB);
	uart_writeb(UART_DLL_OFFSET, dl & 0xff);
	uart_writeb(UART_DLH_OFFSET, dl >> 8);
	uart_writeb(UART_LC_OFFSET, UART_LC_8N1);
#endif
}

void uart_putchar(char c)
{
#if defined(CONFIG_BUILD_WITH_UART)
	while (!(uart_readb(UART_LS_OFFSET) & UART_LS_TEMT));
	uart_writeb(UART_TH_OFFSET, c);
#endif
}

void uart_getchar(char *c)
{
#if defined(CONFIG_BUILD_WITH_UART)
	while (!(uart_readb(UART_LS_OFFSET) & UART_LS_DR));
	*c = uart_readb(UART_RB_OFFSET);
#else
	*c = 0;
#endif
}

void uart_puthex(u32 h)
{
	int i;
	char c;

	for (i = 7; i >= 0; i--) {
		c = (char) ((h >> (i * 4)) & 0xf);

		if (c > 9)
			c += ('A' - 10);
		else
			c += '0';
		uart_putchar(c);
	}
}

void uart_putbin(u32 h, int bits, int show_0)
{
	u32 i;

	if ((bits <= 0) || (bits >= 32)) {
		return;
	}

	for (i = (0x1 << (bits - 1)); i > 0; i = (i >> 1)) {
		if (h & i) {
			uart_putchar('1');
			show_0 = 1;
		} else {
			if (show_0) {
				uart_putchar('0');
			}
		}
	}
}

void uart_putbyte(u32 h)
{
	int i;
	char c;

	for (i = 1; i >= 0; i--) {
		c = (char) ((h >> (i * 4)) & 0xf);

		if (c > 9)
			c += ('A' - 10);
		else
			c += '0';
		uart_putchar(c);
	}
}

void uart_putdec(u32 d)
{
	int leading_zero = 1;
	u32 divisor, result, remainder;

	if (d > 1000000000) {
		uart_putchar('?');  /* FIXME! */
		return;
	}

	remainder = d;
	for (divisor = 1000000000; divisor > 0; divisor /= 10) {
		result = 0;
		if (divisor <= remainder) {
			for (;(result * divisor) <= remainder; result++);
			result--;
			remainder -= (result * divisor);
		}

		if (result != 0 || divisor == 1)
			leading_zero = 0;
		if (leading_zero == 0)
			uart_putchar((char) (result + '0'));
	}
}

int uart_putstr(const char *str)
{
	int rval = 0;

	if (str == ((void *) 0x0))
		return rval;

	while (*str != '\0') {
		uart_putchar(*str);
		str++;
		rval++;
	}

	return rval;
}

int uart_poll(void)
{
#if defined(CONFIG_BUILD_WITH_UART)
	return (uart_readb(UART_LS_OFFSET) & UART_LS_DR);
#else
	return 0;
#endif
}

int uart_read(void)
{
#if defined(CONFIG_BUILD_WITH_UART)
	if (uart_readb(UART_LS_OFFSET) & UART_LS_DR)
		return uart_readb(UART_RB_OFFSET);
	else
#endif
	return -1;
}

void uart_flush_input(void)
{
#if defined(CONFIG_BUILD_WITH_UART)
	unsigned char dump;
	while (uart_readb(UART_LS_OFFSET) & UART_LS_DR) {
		dump = uart_readb(UART_RB_OFFSET);
	}
#endif
}

int uart_wait_escape(int time)
{
	int c;
	u32 t_now;

	rct_timer_reset_count();
	for (;;) {
		t_now = rct_timer_get_count();
		if (t_now >= time)
			break;

		if (uart_poll()) {
			c = uart_read();
			if (c == 0x0a || c == 0x0d || c == 0x1b) {
				return 1;
			}
		}
	}

	return 0;
}

int uart_getstr(char *str, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;
	u32 t_now;
	int skipnl = 1;

	rct_timer_reset_count();
	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = rct_timer_get_count();
			if ((timeout > 0) && (t_now > timeout * 10)) {
				str[i++] = '\0';
				return nread;
			}
		}

		c = uart_read();
		if (c < 0) {
			str[i++] = '\0';
			return c;
		}

		if ((skipnl == 1) && (c != '\r') && (c != '\n'))
			skipnl = 0;

		if (skipnl == 0) {
			if ((c == '\r') || (c == '\n')) {
				str[i++] = '\0';
				return nread;
			} else {
				str[i++] = (char) c;
				nread++;
			}
		}
	}

	return nread;
}

int uart_getcmd(char *cmd, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;
	u32 t_now;

	rct_timer_reset_count();
	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = rct_timer_get_count();
			if ((timeout > 0) && (t_now > timeout * 10)) {
				cmd[i++] = '\0';
				return nread;
			}
		}

		c = uart_read();
		if (c < 0) {
			cmd[i++] = '\0';
			uart_putchar('\n');
			return c;
		}

		if ((c == '\r') || (c == '\n')) {
			cmd[i++] = '\0';

			uart_putstr("\r\n");
			return nread;
		} else if (c == 0x08) {
			if(i > 0) {
				i--;
				nread--;
				uart_putstr("\b \b");
			}
		} else {
			cmd[i++] = c;
			nread++;

			uart_putchar(c);
		}
	}

	return nread;
}

int uart_getblock(char *buf, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n;
	u32 t_now;

	rct_timer_reset_count();
	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = rct_timer_get_count();
			if ((timeout > 0) && (t_now > timeout * 10)) {
				return nread;
			}
		}

		c = uart_read();
		if (c < 0)
			return c;

		buf[i++] = (char) c;
		nread++;
	}

	return nread;
}

