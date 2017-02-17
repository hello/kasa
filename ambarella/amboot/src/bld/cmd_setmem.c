/**
 * system/src/bld/cmd_setmem.c
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

typedef unsigned long ul;
typedef unsigned long long ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

static int check_address(u32 start, u32 end)
{
        int rval = -1;

        if ((start < DRAM_START_ADDR) ||
            (end > DRAM_END_ADDR)) {
        	uart_putstr("address out of range\r\n");
                uart_putstr("DRAM start from 0x");
                uart_puthex(DRAM_START_ADDR);
                uart_putstr(" to 0x");
                uart_puthex(DRAM_END_ADDR);
                uart_putstr("\r\n");
        	return rval;
        }else if (((start >= AMBOOT_BLD_RAM_START) &&
                  (end <= AMBOOT_BLD_RAM_MAX_END)) ||
                  ((start <= AMBOOT_BLD_RAM_START) &&
                  (end >= AMBOOT_BLD_RAM_MAX_END))){
                uart_putstr("test DRAM include bld \r\n");
                uart_putstr("bld start from 0x");
                uart_puthex(AMBOOT_BLD_RAM_START);
                uart_putstr(" to 0x");
                uart_puthex(AMBOOT_BLD_RAM_MAX_END);
                uart_putstr("\r\n");
                return rval;
        }

        rval = 0;
        return rval;
}

static int memtest_setmem(u32 saddr, u32 eaddr, u32 val)
{
	u32	caddr;
	u32	verify_data;

	/* Perform the set operation */
	for (caddr = saddr; caddr <= eaddr; ) {
		writel(caddr, val);
                verify_data = readl(caddr);
                if(verify_data != val){
                        uart_putstr("0x");
                        uart_puthex(verify_data);
                        uart_putstr(" != 0x");
                        uart_puthex(val);
                        uart_putstr(" at 0x");
                        uart_puthex(caddr);
                        uart_putstr("\r\n");
                        return -1;
                }
                else{
                        uart_putstr("address: 0x");
                        uart_puthex(caddr);
                        uart_putstr("= 0x");
                        uart_puthex(verify_data);
                        uart_putstr(", R/W test pass\r\n");
                }
                caddr += 4;
	}
        return 0;
}

static int compare_region(ulv *bufa, ulv *bufb, ul count) {
    int r = 0;
    ul i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;

    for (i = 0; i < count; i++, p1++, p2++) {
        if (*p1 != *p2) {
                uart_putstr("FAILURE: 0x");
                uart_puthex((ul)*p1);
                uart_putstr(" != ");
                uart_putstr(" 0x");
                uart_puthex((ul)*p2);
                uart_putstr(" at offset ");
                uart_puthex((ul)(i * sizeof(ul)));
                uart_putstr("\r\n");
                    /* printf("Skipping to next test..."); */
                r = -1;
        }
    }
    return r;
}

int test_solidbits(ulv *bufa, ulv *bufb, ul count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j;
    ul q;
    ul i;

    for (j = 0; j < 64; j++) {
        uart_putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        q = (j % 2) == 0 ? 0xffffffff : 0;
        uart_putstr("setting ");
        uart_putdec(j);
        p1 = (ulv *) bufa;
        p2 = (ulv *) bufb;

        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (((i % 2) == 0) ? q : ~q);
        }
        uart_putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        uart_putstr("testing ");
        uart_putdec(j);

        if (compare_region(bufa, bufb, count)) {
            return -1;
        }
    }
    uart_putstr("\r\n");

    return 0;
}

int test_bitflip(ulv *bufa, ulv *bufb, ul count) {
    ulv *p1 = bufa;
    ulv *p2 = bufb;
    unsigned int j, k;
    ul q;
    ul i;

    for (k = 0; k < 32; k++) {
        q = 1 << k;
        for (j = 0; j < 8; j++) {
                uart_putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                q = ~q;
                uart_putstr("setting ");
                uart_putdec(k * 8 + j);
                p1 = (ulv *) bufa;
                p2 = (ulv *) bufb;
                for (i = 0; i < count; i++) {
                        *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
                }
                uart_putstr("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
                uart_putstr("testing ");
                uart_putdec(k * 8 + j);
                if (compare_region(bufa, bufb, count)) {
                        return -1;
                }
        }
    }
    uart_putstr("\r\n");
    return 0;
}

static int cmd_memtest(int argc, char *argv[])
{
	u32	val;
	u32	_saddr;
	u32	_eaddr;
        u32     _size;
        int     rval = -1;

	if (argc > 5){
		uart_putstr("Type 'help setmem' for help\r\n");
		return rval;
        }

	/* Get starting address */
	if (strtou32(argv[1], &_saddr) < 0) {
		uart_putstr("invalid start address!\r\n");
		return rval;
	}

	/* Get end address */
	if (strtou32(argv[2], &_size) < 0) {
		uart_putstr("incorrect ending address\r\n");
		return rval;
	}
        _eaddr = _saddr + _size;

	if (_eaddr < _saddr) {
		uart_putstr("end address must be larger than ");
		uart_putstr("start address\r\n");
		return rval;
	}

	/* Check address range */
        if (check_address(_saddr,_eaddr))
                return rval;

        if(!strcmp(argv[3],"set")){
                /* Get data */
                if (strtou32(argv[4], &val) < 0) {
                        uart_putstr("invalid data!\r\n");
                        return rval;
                }else{
                        rval = memtest_setmem(_saddr,_eaddr,val);
                        if(rval)
                                return rval;
                }
        }

        if(!strcmp(argv[3],"bitsolid")){
                uart_putstr("solid test!\r\n");
                test_solidbits((ulv *)_saddr,(ulv *)((_eaddr + _saddr)/2), _size/8);
        }

        if(!strcmp(argv[3],"bitflip")){
                uart_putstr("flip test!\r\n");
                test_bitflip((ulv *)_saddr,(ulv *)((_eaddr + _saddr)/2), _size/8);
        }
        rval = 0;
	return rval;
}

static char help_memtest[] =
	"memtest [start address] [size] [test_mode] [data]\r\n"
	"test_mode : set, bitsolid, bitflip"
	"Write and verify memory content from specified starting and ending addresses\r\n";

__CMDLIST(cmd_memtest, "memtest", help_memtest);
