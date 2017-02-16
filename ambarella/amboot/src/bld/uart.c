/**
 * bld/uart.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <ambhw/uart.h>
#include <ambhw/gpio.h>

#if defined(SECURE_BOOT)
#include "secure/secure_boot.h"
#endif

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

#if defined(SECURE_BOOT)

static int uart_get_readable_string(const char* begin, int head_length, char* string, int number)
{
	int c;
	int i = 0;

uart_get_readable_string_resync_get:

	for (i = 0; i < head_length; i++) {
		while (uart_poll() == 0) {
			//rct_timer_delay_ticks(5);
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
			//rct_timer_delay_ticks(5);
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

static int uart_get_readable_string_variable_length(const char* begin, char* string, int number)
{
	int c;
	int i = 0;

uart_get_readable_string_variable_length_resync_get:

	for (i = 0; i < 4; i++) {
		while (uart_poll() == 0) {
			//rct_timer_delay_ticks(5);
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
			//rct_timer_delay_ticks(5);
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

#endif

int uart_get_onechar_blocked()
{
	while (uart_poll() == 0) {
		continue;
	}

	return uart_read();
}

