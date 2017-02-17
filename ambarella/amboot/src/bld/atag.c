/**
 * system/src/bld/atag.c
 *
 * History:
 *    2006/12/31 - [Charles Chiou] created file
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
#include <atag.h>
#include <ambhw/cortex.h>

/* ==========================================================================*/
u8 *atag_data;
struct tag *params; /* used to point at the current tag */

/* ==========================================================================*/
static void setup_core_tag(void *address, long pagesize)
{
	params = (struct tag *) address;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 1;
	params->u.core.pagesize = pagesize;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}

static void setup_mem_tag(u32 hdrtag, u32 start, u32 len)
{
	params->hdr.tag = hdrtag;
	params->hdr.size = tag_size(tag_mem32);

	params->u.mem.start = start;
	params->u.mem.size = len;

	params = tag_next(params);
}

static void setup_initrd_tag(u32 start, u32 size)
{
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size(tag_initrd);

	params->u.initrd.start = start;
	params->u.initrd.size = size;

	params = tag_next(params);
}

#if 0
static void setup_serialnr_tag(u32 hdrtag, u32 low, u32 high)
{
	params->hdr.tag = hdrtag;
	params->hdr.size = tag_size(tag_serialnr);

	params->u.serialnr.low = low;
	params->u.serialnr.high = high;

	params = tag_next(params);
}
#endif

static void setup_tag_revision(u32 rev)
{
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size(tag_revision);

	params->u.revision.rev = rev;

	params = tag_next(params);
}

static void setup_cmdline_tag(const char *cmdline)
{
	int slen;
	char *cmdend;

	if ((cmdline == NULL) || (*cmdline == '\0'))
		return;

	slen = strlen(cmdline);
	if (slen > MAX_CMDLINE_LEN) {
		slen = MAX_CMDLINE_LEN;
	}
	cmdend = strncpy(params->u.cmdline.cmdline, cmdline, slen);
	slen = (cmdend - params->u.cmdline.cmdline) + 3;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = ((sizeof(struct tag_header) + slen) >> 2);
	params = tag_next(params);
}

void setup_cortex_bst_tag(int verbose)
{
#if defined(AMBOOT_DEV_BOOT_CORTEX)
	params->hdr.tag = ATAG_AMBARELLA_BST;
	params->hdr.size = tag_size(tag_mem32);
	params->u.mem.start = ARM11_TO_CORTEX((u32)cortex_bst_entry);
	params->u.mem.size = (u32)(cortex_bst_end - cortex_bst_entry);
	params = tag_next(params);
	if (verbose) {
		putstr("Cortex BST: 0x");
		puthex(params->u.mem.start);
		putstr(" size: 0x");
		puthex(params->u.mem.size);
		putstr("\r\n");
	}
#endif
}

static void setup_end_tag(void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

/* ==========================================================================*/
u32 setup_tags(void *jump_addr, const char *cmdline, u32 cpux_jump,
	u32 initrd2_start, u32 initrd2_size, int verbose)
{
	u32 mem_base;
	u32 mem_size;
	u32 kernelp;
	u32 kernels;
	u32 idspp;
	u32 idsps;
	u32 atag_addr;

	mem_base = (((u32)jump_addr) & (~SIZE_1MB_MASK));
	mem_size = (IDSP_RAM_START - mem_base);
	K_ASSERT(mem_base >= DRAM_START_ADDR);
	K_ASSERT(mem_size <= DRAM_SIZE);

	atag_addr = (mem_base + AMBOOT_TAG_OFFSET);
	atag_data = (void *)atag_addr;

	memzero(atag_data, ATAG_DATA_SIZE);
	setup_core_tag(atag_data, ATAG_DATA_SIZE);
	if (verbose) {
		putstr("atag_data: 0x");
		puthex((u32)atag_data);
		putstr("\r\n");
	}

	kernelp = ARM11_TO_CORTEX(mem_base);
	kernels = mem_size;
	idspp = ARM11_TO_CORTEX(IDSP_RAM_START);
	idsps = (DRAM_SIZE - (IDSP_RAM_START - DRAM_START_ADDR));
	if (verbose) {
		putstr("kernelp: 0x");
		puthex(kernelp);
		putstr(" kernels: 0x");
		puthex(kernels);
		putstr("\r\n");

		putstr("idspp: 0x");
		puthex(idspp);
		putstr(" idsps: 0x");
		puthex(idsps);
		putstr("\r\n");
	}
	setup_mem_tag(ATAG_MEM, kernelp, kernels);
	setup_mem_tag(ATAG_AMBARELLA_DSP, idspp, idsps);
	setup_tag_revision(AMBOOT_BOARD_ID);
	if (verbose) {
		putstr("AMBOOT_BOARD_ID: 0x");
		puthex(AMBOOT_BOARD_ID);
		putstr("\r\n");
	}

	if (initrd2_start != 0x0 && initrd2_size != 0x0) {
		setup_initrd_tag(ARM11_TO_CORTEX(initrd2_start), initrd2_size);
		if (verbose) {
			putstr("initrd: 0x");
			puthex(ARM11_TO_CORTEX(initrd2_start));
			putstr(" size: 0x");
			puthex(initrd2_size);
			putstr("\r\n");
		}
	}

#if defined(CONFIG_AMBOOT_BD_CMDLINE)
	if (cmdline == NULL) {
		cmdline = CONFIG_AMBOOT_BD_CMDLINE;
	}
#endif
	if ((cmdline != NULL) && (cmdline[0] != '\0')) {
		setup_cmdline_tag(cmdline);
		if (verbose) {
			putstr(cmdline);
			putstr("\r\n");
		}
	}

	setup_cortex_bst_tag(verbose);
	setup_end_tag();

	return ARM11_TO_CORTEX(atag_addr);
}

