/**
 * bld/strutil.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
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


/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * From: @(#)strtoul.c	8.1 (Berkeley) 6/4/93
 */

#include <bldfunc.h>

/* max value for an unsigned long */
#define	ULONG_MAX	0xffffffffUL

int strlen(const char *str)
{
	int len = -1;

	do {
		len++;
	} while (*str++ != 0);

	return len;
}

int strcmp(const char *s1, const char *s2)
{
	if (s1 == s2)
		return 0;

	while (*s1 != '\0' && *s1 == *s2) {
		s1++;
		s2++;
	}

	return (*(unsigned char *) s1) - (*(unsigned char *) s2);
}

char *strcat(register char *src, register const char *append)
{
	char *ret = src;

	for (; *src; ++src);

	while ((*src++ = *append++) != '\0');

	return ret;
}

char *strcpy(char *dst, const char *src)
{
	char *ret = dst;

	while ((*dst++ = *src++) != '\0');

	return ret;
}

int strncmp(const char *s1, const char *s2, unsigned int maxlen)
{
	int i;

	for (i = 0; i < maxlen; i++) {
		if (s1[i] != s2[i])
			return ((int) s1[i]) - ((int) s2[i]);
		if (s1[i] == 0)
			return 0;
	}

	return 0;
}

char *strncpy(char *dest, const char *src, unsigned int n)
{
	while (n > 0) {
		n--;

		if ((*dest++ = *src++) == '\0')
			break;
	}

	return dest;
}

char *strlcpy(char *dest, const char *src, unsigned int n)
{
	strncpy(dest, src, n);

	if (n > 0)
		dest[n-1] = '\0';

	return dest;
}

char *strchr(const char * s, int c)
{
	for(; *s != (char) c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *) s;
}

void *memchr(const void *s, int c, unsigned int n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
		if ((unsigned char)c == *p++) {
			return (void *)(p-1);
		}
	}
	return NULL;
}

int strtou32(const char *str, u32 *endptr)
{
	const char *s = str;
	unsigned long acc;
	unsigned char c;
	unsigned long cutoff;
	int neg = 0, any, cutlim, base;
	int rval = 0;

	if (strncmp(str, "0x", 2) == 0) {
                /* hexadecimal mode */
         	base = 16;
        } else {
                /* decimal mode */
		base = 10;
        }

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)base;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (c == '\0')
			break;

		if (!isascii(c)) {
			rval = -1;
			break;
		}
		if (isdigit_c(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else {
			rval = -1;
			break;
		}
		if (c >= base) {
			rval = -1;
			break;
		}
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = any ? acc : *str;

	return rval;
}

char *strfromargv(char *s, unsigned int slen, int argc, char **argv)
{
	int i;

	if (argc == 0) {
		*s = '\0';
		return s;
	}

	for (i = 0; i < slen; i++)
		s[i] = '\0';
	for (i = 0; i < argc && slen > 0; i++) {
		if (i != 0) {
			s--;
			*s = ' ';
			s++;
		}
		s = strncpy(s, argv[i], slen);
		slen -= strlen(argv[i]);
	}

	return s;
}

int str_to_hwaddr(const char *bufp, u8 *hwaddr)
{
	unsigned char *ptr;
	int i, j;
	unsigned char val;
	unsigned char c;

	ptr = hwaddr;
	i = 0;
	do {
		j = val = 0;

		/* We might get a semicolon here - not required. */
		if (i && (*bufp == ':')) {
			bufp++;
		}

		do {
			c = *bufp;
			if (((unsigned char)(c - '0')) <= 9) {
				c -= '0';
			} else if (((unsigned char)((c | 0x20) - 'a')) <= 5) {
				c = (c | 0x20) - ('a' - 10);
			} else if (j && (c == ':' || c == 0)) {
				break;
			} else {
				return -1;
			}
			++bufp;
			val <<= 4;
			val += c;
		} while (++j < 2);
		*ptr++ = val;
	} while (++i < 6);

	return (int) (*bufp);   /* Error if we don't end at end of string. */
}

int str_to_ipaddr(const char *src, u32 *addr)
{
	int saw_digit, octets, ch;
	unsigned char tmp[4], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {

		if (ch >= '0' && ch <= '9') {
			unsigned int new = *tp * 10 + (ch - '0');

			if (new > 255)
				return (0);
			*tp = new;
			if (! saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return -1;
	}
	if (octets < 4)
		return -2;
	memcpy(addr, tmp, 4);

	return 0;
}

char *strstr(const char *s1, const char *s2, int n)
{
	int len = strlen(s2);

	if (!len)
		return NULL;

	while (n--){
		if(*s1 == *s2)
			if(!strncmp(s1, s2, len))
				return (char *)s1;
		s1++;
	}
	return NULL;
}

