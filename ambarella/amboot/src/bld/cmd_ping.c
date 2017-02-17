/**
 * bld/cmd_ping.c
 *
 * History:
 *    2006/10/16 - [Charles Chiou] created file
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
#include <bldfunc.h>
#include <eth/network.h>

/*===========================================================================*/
static int cmd_ping(int argc, char *argv[])
{
	int rval = 0;
	bld_ip_t ip;

	if (argc != 2) {
		return -1;
	}

	if (str_to_ipaddr(argv[1], &ip) < 0) {
		putstr("IP addr error!\r\n");
		return 0;
	}

	rval = bld_net_wait_ready(DEFAULT_NET_WAIT_TMO);
	if (rval < 0) {
		putstr("link down ...\r\n");
		return 0;
	}

	rval = bld_ping(ip, DEFAULT_NET_WAIT_TMO);
	if (rval >= 0) {
		putstr("alive!\r\n");
	} else {
		putstr("no echo reply! ...\r\n");
	}

	return 0;
}

/*===========================================================================*/
static char help_ping[] =
	"ping [addr]\r\n"
	"PING a host\r\n";
__CMDLIST(cmd_ping, "ping", help_ping);

