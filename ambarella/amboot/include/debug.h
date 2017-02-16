/**
 * debug.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

/*===========================================================================*/
#ifndef __ASM__

#ifdef __DEBUG_BUILD__
#define K_ASSERT(x) {							\
		if (!(x)) {						\
			putstr("Assertion failed!(");			\
			putstr(__FILE__);				\
			putstr(":");					\
			putdec(__LINE__);				\
			putstr(")");					\
			for (;;);					\
		}							\
	}

#define BUG_ON(x) {							\
		if (x) {						\
			putstr("BUG_ON(");				\
			putstr(__FILE__);				\
			putstr(":");					\
			putdec(__LINE__);				\
			putstr(")");					\
			for (;;);					\
		}							\
	}

#else
#define K_ASSERT(x) {							\
		if (!(x)) {						\
			for (;;);					\
		}							\
	}

#define BUG_ON(x) {							\
		if (x) {						\
			for (;;);					\
		}							\
	}
#endif

/* ==========================================================================*/
#ifdef __RELEASE_NOPUT_BUILD__
#define putchar(out)
#define puthex(out)
#define putbin(h, bits, show_0)
#define putbyte(out)
#define putdec(out)
#define putstr(out)
#else
#include <ambhw/uart.h>
#define putchar		uart_putchar
#define puthex		uart_puthex
#define putbin		uart_putbin
#define putbyte		uart_putbyte
#define putdec		uart_putdec
#define putstr		uart_putstr
#endif

#define putstrchar(str, c) {putstr(str); putchar(c); putstr("\r\n");}
#define putstrdec(str, d) {putstr(str); putdec(d); putstr("\r\n");}
#define putstrhex(str, h) {putstr(str); puthex(h); putstr("\r\n");}
#define putstrstr(str, s) {putstr(str); putstr(s); putstr("\r\n");}

#define AMBOOT_IAV_STR_DEBUG
#undef AMBOOT_IAV_STR_DEBUG

#ifdef AMBOOT_IAV_STR_DEBUG
#define putstr_debug(msg)	\
	do {						\
		putstr(msg);		\
		putstr("\r\n");		\
	} while (0)
#else
#define putstr_debug(debug_msg)
#endif

#endif /* __ASM__ */

/*===========================================================================*/

#endif

