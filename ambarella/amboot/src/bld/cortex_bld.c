/**
 * bld/cortex.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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
#include <ambhw/vic.h>
#include <ambhw/cache.h>
#include <ambhw/cortex.h>
#include <ambhw/fio.h>
#include <eth/network.h>

#if defined(AMBOOT_DEV_BOOT_CORTEX)

/*===========================================================================*/
#define BOOT_BACKUP_SIZE	(4)

/*===========================================================================*/
static u32 boot_backuped = 0;
static u32 boot_backup[BOOT_BACKUP_SIZE];
static u32 *pcortex_start_add = NULL;

/*===========================================================================*/
static int bld_cortex_load_bootstrap(u32 start_address, int verbose,
	u32 jump_addr, u32 r0)
{
	int ret_val = 0;
	int i;
	u32 *pcortex_bsp_add;
	u32 *psys_bsp_add;

	pcortex_start_add = (u32 *)CORTEX_TO_ARM11(start_address);
	pcortex_bsp_add = (u32 *)cortex_bst_entry;
	pcortex_bsp_add = (u32 *)ARM11_TO_CORTEX((u32)pcortex_bsp_add);
	psys_bsp_add = (u32 *)bld_start;

	if (psys_bsp_add == pcortex_start_add) {
	} else {
		if (boot_backuped == 0) {
			for (i = 0; i < BOOT_BACKUP_SIZE; i++) {
				boot_backup[i] = pcortex_start_add[i];
			}
			boot_backuped = 1;
		}
		pcortex_start_add[0] = 0xE59F0000;	//ldr	r0, [pc, #0]
		pcortex_start_add[1] = 0xE12FFF10;	//bx	r0
		pcortex_start_add[2] = (u32)pcortex_bsp_add;
		pcortex_start_add[3] = 0xEAFFFFFB;	//b	start
		clean_d_cache(pcortex_start_add,
			(BOOT_BACKUP_SIZE * sizeof(u32)));
	}

	cortex_processor_start[0] = ARM11_TO_CORTEX(jump_addr);
	cortex_processor_status[0] = CORTEX_BST_START_COUNTER;
	*cortex_machine_id = AMBARELLA_LINUX_MACHINE_ID;
	if (verbose) {
		putstr("loading jmp: 0x");
		puthex(start_address);
		putstr(" size: 0x");
		puthex(BOOT_BACKUP_SIZE);
		putstr("\r\n");
		putstr("BST: 0x");
		puthex((u32)pcortex_bsp_add);
		putstr(" size: 0x");
		puthex((u32)(cortex_bst_end - cortex_bst_entry));
		putstr("\r\n");
		putstr("HEAD: 0x");
		puthex(ARM11_TO_CORTEX((u32)cortex_bst_head));
		putstr(" size: 0x");
		puthex((u32)(cortex_bst_end - cortex_bst_head));
		putstr("\r\n");
	}

	if (r0) {
		*cortex_processor0_r0 = r0;
	}

	if (verbose) {
		putstr("PROCESSOR_START_0: 0x");
		puthex(cortex_processor_start[0]);
		putstr("\r\nMACHINE_ID: 0x");
		puthex(*cortex_machine_id);
		putstr("\r\nATAG_DATA: 0x");
		puthex(*cortex_atag_data);
		putstr("\r\nPROCESSOR0_R0: 0x");
		puthex(*cortex_processor0_r0);
		putstr("\r\n");
	}
	clean_d_cache(cortex_bst_head,
		(u32)(cortex_bst_end - cortex_bst_head));

	return ret_val;
}

static void bld_cortex_pre_init(int verbose,
	u32 ctrl, u32 ctrl2, u32 ctrl3, u32 frac)
{
	u32 cortex_ctl;

	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_RESET(0x3);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_CLOCK(0x3);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);
	if (ctrl) {
		ctrl &= ~(0x00200000);
		writel(PLL_CORTEX_FRAC_REG, frac);
		rct_timer_dly_ms(1);
		writel(PLL_CORTEX_CTRL_REG, ctrl);
		rct_timer_dly_ms(1);
		writel(PLL_CORTEX_CTRL2_REG, ctrl2);
		rct_timer_dly_ms(1);
		writel(PLL_CORTEX_CTRL3_REG, ctrl3);
		rct_timer_dly_ms(1);
		writel(PLL_CORTEX_CTRL_REG, (ctrl | 0x01));
		rct_timer_dly_ms(1);
		writel(PLL_CORTEX_CTRL_REG, ctrl);
		rct_timer_dly_ms(1);
	} else {
		ctrl = readl(PLL_CORTEX_CTRL_REG);
		if (ctrl & (0x00200000)) {
			ctrl &= ~(0x00200000);
			writel(PLL_CORTEX_CTRL_REG, ctrl);
			rct_timer_dly_ms(1);
			writel(PLL_CORTEX_CTRL_REG, (ctrl | 0x01));
			rct_timer_dly_ms(1);
			writel(PLL_CORTEX_CTRL_REG, ctrl);
			rct_timer_dly_ms(1);
		}
	}
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_RESET(0x3));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);
}

static int bld_cortex_init(int verbose)
{
	u32 cortex_ctl;
	int i, j;

	vic_init();
	disable_interrupts();

	if (amboot_bsp_cortex_init_pre != NULL) {
		amboot_bsp_cortex_init_pre();
	} else {
		bld_cortex_pre_init(verbose, 0x00000000,
			0x00000000, 0x00000000, 0x00000000);
	}
	_drain_write_buffer();
	_clean_flush_all_cache();

	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x3));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);

	i = 0;
	while (1) {
		if (i > CORTEX_BST_WAIT_LIMIT) {
			i = 0;
			cortex_ctl = readl(AHB_SECURE_REG(0x04));
			if (verbose) {
				putstr("Timeout: Reset Core0 [0x");
				puthex(cortex_ctl);
				putstr("]...\r\n");
			}
			cortex_ctl |= AXI_CORTEX_RESET(0x3);
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl |= AXI_CORTEX_CLOCK(0x3);
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl &= ~(AXI_CORTEX_RESET(0x3));
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x3));
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
		}
		flush_d_cache(cortex_bst_head,
			(u32)(cortex_bst_end - cortex_bst_head));
		if (cortex_processor_status[0] == 0)
			break;
		i++;
	}

	__asm__ __volatile__ ("bkpt");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");

	if (boot_backuped == 1) {
		for (i = 0; i < BOOT_BACKUP_SIZE; i++) {
			pcortex_start_add[i] = boot_backup[i];
		}
		clean_d_cache(pcortex_start_add,
			(BOOT_BACKUP_SIZE * sizeof(u32)));
		_flush_i_cache();
	}

	enable_interrupts();

	return 0;
}

/*===========================================================================*/
int bld_cortex_boot(int verbose, u32 jump_addr)
{
	int ret_val;

	ret_val = bld_cortex_load_bootstrap(CORTEX_BOOT_ADDRESS, verbose,
		jump_addr, 0);
	if (ret_val) {
		goto bld_cortex_boot_exit;
	}

	ret_val = bld_cortex_init(verbose);

bld_cortex_boot_exit:
	return ret_val;
}

#elif defined(AMBOOT_BOOT_SECONDARY_CORTEX)

int bld_boot_secondary_cortex(void)
{
	u32 cortex_ctl;
#if defined(CHIP_FIX_2NDCORTEX_BOOT)
	u32 fio_ctr;
	u32 sysconfig;
	u32 nand_boot;

	/* Enter random read mode */
	fio_ctr = readl(FIO_CTR_REG);
	writel(FIO_CTR_REG, (fio_ctr | FIO_CTR_RR));

	sysconfig = readl(SYS_CONFIG_REG);
	nand_boot = ((sysconfig & ~POC_BOOT_FROM_MASK) | 0x00000010);
	writel(SYS_CONFIG_REG, nand_boot);

	writel(FIO_4K_REG(0x00), 0xE59F0000);	//ldr	r0, [pc, #0]
	writel(FIO_4K_REG(0x04), 0xE12FFF10);	//bx	r0
	writel(FIO_4K_REG(0x08), 0x00000000);	//secondary cortex start from 0x00000000
	writel(FIO_4K_REG(0x0C), 0xEAFFFFFB);	//b	start
	rct_timer_dly_ms(1);
#endif
	/* FIXME: maybe the delay functions can be removed. */

	/* reset secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_RESET(0x2);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate on clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate off clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_CLOCK(0x2);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* secondary cortex core exit reset */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_RESET(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

	/* gate on clock to secondary cortex core */
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x2));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	rct_timer_dly_ms(1);

#if defined(CHIP_FIX_2NDCORTEX_BOOT)
	writel(SYS_CONFIG_REG, sysconfig);
	/* Exit random read mode */
	writel(FIO_CTR_REG, fio_ctr);
	rct_timer_dly_ms(1);
#endif

	return 0;
}

#endif

