/**
 * bld/minipin.S
 *
 * History:
 *    2014/12/29 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <ambhw/gpio.h>

.text

#if (IOMUX_SUPPORT > 0)
.macro	altfunc, bank, alt
	ldr	r1, [r0, #IOMUX_REG_OFFSET(\bank, 0)]
	ands	r4, \alt, #0x1
	orrne	r1, r1, r2
	biceq	r1, r1, r2
	str	r1, [r0, #IOMUX_REG_OFFSET(\bank, 0)]
	ldr	r1, [r0, #IOMUX_REG_OFFSET(\bank, 1)]
	ands	r4, \alt, #0x2
	orrne	r1, r1, r2
	biceq	r1, r1, r2
	str	r1, [r0, #IOMUX_REG_OFFSET(\bank, 1)]
	ldr	r1, [r0, #IOMUX_REG_OFFSET(\bank, 2)]
	ands	r4, \alt, #0x4
	orrne	r1, r1, r2
	biceq	r1, r1, r2
	str	r1, [r0, #IOMUX_REG_OFFSET(\bank, 2)]
.endm
#endif


/**
 * Initiate minimal pins for UART, NAND, eMMC and SPINOR.
 */
.globl	minipin_init
minipin_init:
#if (CHIP_REV == S2L)
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	/* UART */
	ldr	r2, =0x00000180
	mov	r3, #0x1
	altfunc	1, r3

	/* PINs for boot media */
	ldr	r2, =ambausb_boot_from
	ldr	r2, [r2]
	cmp	r2, #RCT_BOOT_FROM_SPINOR
	beq	minipin_spinor
	cmp	r2, #RCT_BOOT_FROM_EMMC
	beq	minipin_emmc

	/* NAND */
	ldr	r2, =0xe1c00000
	mov	r3, #0x2
	altfunc	1, r3
	ldr	r2, =0x000001ff
	mov	r3, #0x2
	altfunc	2, r3
	b	minipin_iomux_done

	/* SPINOR */
minipin_spinor:
	ldr	r2, =0xe1800000
	mov	r3, #0x3
	altfunc	1, r3
	ldr	r2, =0x000001ff
	mov	r3, #0x3
	altfunc	2, r3
	b	minipin_iomux_done

	/* eMMC */
minipin_emmc:
	ldr	r2, =0x1e000000
	mov	r3, #0x2
	altfunc	1, r3
	ldr	r2, =0x0001fe00
	mov	r3, #0x2
	altfunc	2, r3

minipin_iomux_done:
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]

#elif (CHIP_REV == S3)
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	/* UART */
	ldr	r2, =0x00000030
	mov	r3, #0x1
	altfunc	1, r3

	/* PINs for boot media */
	ldr	r2, =ambausb_boot_from
	ldr	r2, [r2]
	cmp	r2, #RCT_BOOT_FROM_SPINOR
	beq	minipin_spinor
	cmp	r2, #RCT_BOOT_FROM_EMMC
	beq	minipin_emmc

	/* NAND */
	ldr	r2, =0xfc380000
	mov	r3, #0x2
	altfunc	3, r3
	ldr	r2, =0x0000003f
	mov	r3, #0x2
	altfunc	4, r3
	b	minipin_iomux_done

	/* SPINOR */
minipin_spinor:
	ldr	r2, =0xec300000
	mov	r3, #0x3
	altfunc	3, r3
	ldr	r2, =0x0000001f
	mov	r3, #0x3
	altfunc	4, r3
	b	minipin_iomux_done

	/* eMMC */
minipin_emmc:
	ldr	r2, =0x03c00000
	mov	r3, #0x2
	altfunc	3, r3
	ldr	r2, =0x04003fc0
	mov	r3, #0x2
	altfunc	4, r3

minipin_iomux_done:
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]

#elif (CHIP_REV == S3L)
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	/* UART */
	ldr	r2, =0x00000300
	mov	r3, #0x1
	altfunc	1, r3

	/* PINs for boot media */
	ldr	r2, =ambausb_boot_from
	ldr	r2, [r2]
	cmp	r2, #RCT_BOOT_FROM_SPINOR
	beq	minipin_spinor
	cmp	r2, #RCT_BOOT_FROM_EMMC
	beq	minipin_emmc

	/* NAND */
	ldr	r2, =0xc3800000
	mov	r3, #0x2
	altfunc	1, r3
	ldr	r2, =0x000003ff
	mov	r3, #0x2
	altfunc	2, r3
	b	minipin_iomux_done

	/* SPINOR */
minipin_spinor:
	ldr	r2, =0xc3000000
	mov	r3, #0x3
	altfunc	1, r3
	ldr	r2, =0x000001ff
	mov	r3, #0x3
	altfunc	2, r3
	b	minipin_iomux_done

	/* eMMC */
minipin_emmc:
	ldr	r2, =0x3c000000
	mov	r3, #0x2
	altfunc	1, r3
	ldr	r2, =0x0003fc00
	mov	r3, #0x2
	altfunc	2, r3

minipin_iomux_done:
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]

#else	/* S2/A5S */

	/* UART */
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, #0x0000c000
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, #0x0000c000
	str	r1, [r0, #GPIO_MASK_OFFSET]
	mov	r1, #0x1
	str	r1, [r0, #GPIO_ENABLE_OFFSET]

	/* eMMC */
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, #0x0000001e
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	mov	r1, #0x1
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif

	mov	pc, lr

