/**
 * system/src/bld/cmd_r32.c
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


#include <bldfunc.h>
#include <ambhw/drctl.h>

static int cmd_r32(int argc, char *argv[])
{
	u32	_taddr;
	u32	val;
	u32	addr;

	if (argc == 2){
		/* Check address format */
		if (strtou32(argv[1], &addr) == -1) {
			uart_putstr("invalid address!\r\n");
			return -1;
		}

		/* Check memory range */
		_taddr = addr & 0xf0000000;
		if (_taddr != AHB_BASE &&
		    _taddr != APB_BASE &&
		    _taddr != (DRAM_PHYS_BASE & 0xf0000000) &&
		    _taddr != DRAM_START_ADDR &&
		    (addr < DRAM_START_ADDR || addr > DRAM_START_ADDR + DRAM_SIZE-1)) {
			uart_putstr("address out of range!\r\n");
			return -2;
		}

		val = readl(addr);

		uart_putstr("0x");
		uart_puthex(addr);
		uart_putstr("  :  ");
		uart_putstr("0x");
		uart_putbyte(val >> 24);
		uart_putbyte(val >> 16);
		uart_putbyte(val >> 8);
		uart_putbyte(val & 0xff);
		uart_putstr("\r\n");
	} else {
		uart_putstr("Type 'help r32' for help\r\n");
		return -3;
	}

	return 0;
}

static char help_r32[] =
	"r32 [address]\r\n"
	"Read a 32-bit data from an address\r\n";

__CMDLIST(cmd_r32, "r32", help_r32);

