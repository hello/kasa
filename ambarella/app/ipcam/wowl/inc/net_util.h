/*******************************************************************************
 * net_util.h
 *
 * History:
 *    2015/8/8 - [Tao Wu] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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

#ifndef __NET_UTIL_H_
#define __NET_UTIL_H_

#include <basetypes.h>
#include <unistd.h>

#define SET_TCP_FIX		(0xBADBEEF) /* Fix TCP Session Ipid, Seq, Ack */

#define TCP_PL_OFFSET 	(66)	 /* 66 is the offset of TCP payload */
#define UDP_PL_OFFSET 	(42)	 /* 42 is the offset of UDP payload */

#define IPV4_ADDR_STR_LEN (32)
#define IPV4_ADDR_LEN		(4)
#define IPV4_PORT_LEN			(2)

typedef enum {
	WIFI_BCM43340 = 0,
	WIFI_BCM43438 = 1,

	WIFI_SD8977 = 10,
	WIFI_SD8801 = 11,

	WIFI_CHIP_TOTAL,
	WIFI_CHIP_FIRST = WIFI_BCM43340,
	WIFI_CHIP_LAST = WIFI_SD8801,
} wifi_chip_t;

typedef struct ipv4_addr {
        u8   addr[IPV4_ADDR_LEN];
} ipv4_addr_t;

typedef struct ipv4_port {
        u8   data[IPV4_PORT_LEN];
} ipv4_port_t;

char *iptoa(const struct ipv4_addr *n)
{
	static char iptoa_buf[IPV4_ADDR_LEN * 4];
	sprintf(iptoa_buf, "%u.%u.%u.%u", n->addr[0], n->addr[1], n->addr[2], n->addr[3]);

	return iptoa_buf;
}

int atoip(const char *a, struct ipv4_addr *n)
{
	char *c = NULL;
	int i = 0;

	for (;;) {
		n->addr[i++] = (u8)strtoul(a, &c, 0);
		if (*c++ != '.' || i == IPV4_ADDR_LEN)
			break;
		a = c;
	}

	return (i == IPV4_ADDR_LEN);
}

void in_addr_to_ipv4(struct ipv4_addr *ipa, u32 ip)
{
	ipa->addr[0] = (u8)(ip >> 0) & 0xFF;
	ipa->addr[1] = (u8)(ip >> 8) & 0xFF;
	ipa->addr[2] = (u8)(ip >> 16) & 0xFF;
	ipa->addr[3] = (u8)(ip >> 24) & 0xFF;
}

ssize_t readn( int inSock, void *outBuf, size_t inLen )
{
	size_t  nleft = 0;
	ssize_t nread = 0;
	char *ptr;

	assert( inSock >= 0 );
	assert( outBuf != NULL );
	assert( inLen > 0 );

	ptr   = (char*) outBuf;
	nleft = inLen;

	while ( nleft > 0 ) {
		nread = read( inSock, ptr, nleft );
		if ( nread < 0 ) {
			if ( errno == EINTR ) {
				nread = 0;  /* interupted, call read again */
			}
			else{
				return -1;  /* error */
			}
		} else if ( nread == 0 ) {
			break;        /* EOF */
		}
		nleft -= nread;
		ptr   += nread;
	}

	return(inLen - nleft);
} /* end readn */

ssize_t writen( int inSock, const void *inBuf, size_t inLen )
{
	size_t  nleft = 0;
	ssize_t nwritten = 0;
	const char *ptr;

	assert( inSock >= 0 );
	assert( inBuf != NULL );
	assert( inLen > 0 );

	ptr   = (char*) inBuf;
	nleft = inLen;

	while ( nleft > 0 ) {
		nwritten = write( inSock, ptr, nleft );
		if ( nwritten <= 0 ) {
			if ( errno == EINTR ) {
				nwritten = 0; /* interupted, call write again */
			} else {
				return -1;    /* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return inLen;
} /* end writen */

#endif
