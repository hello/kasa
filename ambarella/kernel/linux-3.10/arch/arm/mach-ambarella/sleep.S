/*
 * arch/arm/mach-ambarella/sleep.S
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/cache.h>
#include <mach/hardware.h>
#include <plat/drctl.h>
#include <plat/rct.h>
#include <plat/uart.h>

	.text

ENTRY(ambarella_finish_suspend)
	/*
	 * Execute DSB and ISB instruction to ensure that all of the
	 * memory accesses have been completed.
	 */
	dsb
	isb
	nop

	/*
	 * Following codes have to run from cache since
	 * the DRAM is going to self refresh mode
	 */
	.align	L1_CACHE_SHIFT
	ldr	r0, =(DRAMC_BASE + DRAM_DDRC_OFFSET)
	ldr	r1, [r0, #DDRC_CTL_OFFSET]
	bic	r1, r1, #0x1
	str	r1, [r0, #DDRC_CTL_OFFSET]
	mov	r1, #0x80000000
	str	r1, [r0, #DDRC_SELF_REF_OFFSET]
1:
	ldr	r1, [r0, #DDRC_SELF_REF_OFFSET]
	tst	r1, #0x10000000
	beq	1b

	/* now turn off PWC */
	ldr	r0, =ANA_PWR_REG
	mov	r1, #ANA_PWR_POWER_DOWN
	str	r1, [r0]

	/* never get here */
2:	b	2b

ENDPROC(ambarella_finish_suspend)

#if (CHIP_REV == S3)
#define	IOMUX_UART0_PIN_CFG	0x00000030
#else
#define	IOMUX_UART0_PIN_CFG	0x00000180
#endif

ENTRY(ambarella_cpu_resume)
	/*
	 * init UART(115200, 8N1) to avoid deadloop in printk.
	 * MMU is not enabled yet, so we use physical address here.
	 */
	ldr	r0, =RCT_PHYS_BASE
	mov	r1, #0x1
	str	r1, [r0, #CG_UART_OFFSET]

	ldr	r0, =(APB_PHYS_BASE + UART_OFFSET)
	mov	r1, #0x1
	str	r1, [r0, #UART_SRR_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #UART_SRR_OFFSET]
	mov	r1, #0x80
	str	r1, [r0, #UART_LC_OFFSET]
	ldr	r1, =(REF_CLK_FREQ/16/115200)
	str	r1, [r0, #UART_DLL_OFFSET]
	mov	r1, #0x00
	str	r1, [r0, #UART_DLH_OFFSET]
	mov	r1, #0x03
	str	r1, [r0, #UART_LC_OFFSET]

	/* configure Tx/Rx pin as hw mode, it's chip specific. */
#if (IOMUX_SUPPORT > 0)
	/* gpio39/40 are used for UART0_rx/tx */
	ldr	r0, =(APB_PHYS_BASE + IOMUX_OFFSET)
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_0_OFFSET]
	orr	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_0_OFFSET]
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_1_OFFSET]
	bic	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_1_OFFSET]
	/* read-modify-write */
	ldr	r1, [r0, #IOMUX_REG1_2_OFFSET]
	bic	r1, r1, #IOMUX_UART0_PIN_CFG
	str	r1, [r0, #IOMUX_REG1_2_OFFSET]
	ldr	r1, =0x00000001
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	ldr	r1, =0x00000000
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#else
	/* gpio14/15 are used for UART0_tx/rx */
	ldr	r0, =(APB_PHYS_BASE + GPIO0_OFFSET)
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, #0x0000c000
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
#endif
	/* jump to generic resume */
	b	cpu_resume
ENDPROC(ambarella_cpu_resume)

