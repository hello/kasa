/**
 * bld/gpio.S
 *
 * History:
 *    2005/09/03 - [Charles Chiou] created file
 *    2008/05/09 - [Charles Chiou] added macros specifying default DIR, MASK
 *    2009/10/15 - [Allen Wang] added macros for GPIO4.
 *    2014/02/13 - [Anthony Ginger] Amboot V2
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <ambhw/gpio.h>

#ifndef DEFAULT_GPIO0_AFSEL
#define DEFAULT_GPIO0_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO0_DIR
#define DEFAULT_GPIO0_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO0_MASK
#define DEFAULT_GPIO0_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO0_DATA
#define DEFAULT_GPIO0_DATA	0x00000000
#endif

#if (GPIO_INSTANCES >= 2)
#ifndef DEFAULT_GPIO1_AFSEL
#define DEFAULT_GPIO1_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO1_DIR
#define DEFAULT_GPIO1_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO1_MASK
#define DEFAULT_GPIO1_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO1_DATA
#define DEFAULT_GPIO1_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 3)
#ifndef DEFAULT_GPIO2_AFSEL
#define DEFAULT_GPIO2_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO2_DIR
#define DEFAULT_GPIO2_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO2_MASK
#define DEFAULT_GPIO2_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO2_DATA
#define DEFAULT_GPIO2_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 4)
#ifndef DEFAULT_GPIO3_AFSEL
#define DEFAULT_GPIO3_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO3_DIR
#define DEFAULT_GPIO3_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO3_MASK
#define DEFAULT_GPIO3_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO3_DATA
#define DEFAULT_GPIO3_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 5)
#ifndef DEFAULT_GPIO4_AFSEL
#define DEFAULT_GPIO4_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO4_DIR
#define DEFAULT_GPIO4_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO4_MASK
#define DEFAULT_GPIO4_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO4_DATA
#define DEFAULT_GPIO4_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 6)
#ifndef DEFAULT_GPIO5_AFSEL
#define DEFAULT_GPIO5_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO5_DIR
#define DEFAULT_GPIO5_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO5_MASK
#define DEFAULT_GPIO5_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO5_DATA
#define DEFAULT_GPIO5_DATA	0x00000000
#endif
#endif

#if (GPIO_INSTANCES >= 7)
#ifndef DEFAULT_GPIO6_AFSEL
#define DEFAULT_GPIO6_AFSEL    	0xffffffff
#endif
#ifndef DEFAULT_GPIO6_DIR
#define DEFAULT_GPIO6_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO6_MASK
#define DEFAULT_GPIO6_MASK	0xffffffff
#endif
#ifndef DEFAULT_GPIO6_DATA
#define DEFAULT_GPIO6_DATA	0x00000000
#endif
#endif

#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
#ifndef DEFAULT_GPIO0_CTRL_ENA
#define DEFAULT_GPIO0_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO0_CTRL_DIR
#define DEFAULT_GPIO0_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_ENA
#define DEFAULT_GPIO1_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO1_CTRL_DIR
#define DEFAULT_GPIO1_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_ENA
#define DEFAULT_GPIO2_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO2_CTRL_DIR
#define DEFAULT_GPIO2_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_ENA
#define DEFAULT_GPIO3_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO3_CTRL_DIR
#define DEFAULT_GPIO3_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_ENA
#define DEFAULT_GPIO4_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO4_CTRL_DIR
#define DEFAULT_GPIO4_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_ENA
#define DEFAULT_GPIO5_CTRL_ENA	0x00000000
#endif
#ifndef DEFAULT_GPIO5_CTRL_DIR
#define DEFAULT_GPIO5_CTRL_DIR	0x00000000
#endif
#ifndef DEFAULT_GPIO6_CTRL_DIR
#define DEFAULT_GPIO6_CTRL_DIR	0x00000000
#endif
#endif

#if (GPIO_PAD_DS_SUPPORT == 1)
#ifndef DEFAULT_GPIO_DS0_REG_0
#define DEFAULT_GPIO_DS0_REG_0	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_0
#define DEFAULT_GPIO_DS1_REG_0	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_1
#define DEFAULT_GPIO_DS0_REG_1	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_1
#define DEFAULT_GPIO_DS1_REG_1	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_2
#define DEFAULT_GPIO_DS0_REG_2	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_2
#define DEFAULT_GPIO_DS1_REG_2	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_3
#define DEFAULT_GPIO_DS0_REG_3	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_3
#define DEFAULT_GPIO_DS1_REG_3	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_4
#define DEFAULT_GPIO_DS0_REG_4	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_4
#define DEFAULT_GPIO_DS1_REG_4	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS0_REG_5
#define DEFAULT_GPIO_DS0_REG_5	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_5
#define DEFAULT_GPIO_DS1_REG_5	0xFFFFFFFF
#endif
#ifndef DEFAULT_GPIO_DS1_REG_6
#define DEFAULT_GPIO_DS1_REG_6	0xFFFFFFFF
#endif
#endif

.text

/**
 *
 * @param r0 - GPIO pin number
 * @returns r0 - BASE
 * @returns r1 - GPIO Group ID
 * @returns r3 - offset
 */
gpio_to_reg:
	mov	r3, r0
#if (GPIO_INSTANCES >= 7)
	cmp	r3, #191
	subhi	r3, r3, #192
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO6_OFFSET
	movhi	r1, #5
	movhi	pc, lr
#endif
#if (GPIO_INSTANCES >= 6)
	cmp	r3, #159
	subhi	r3, r3, #160
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO5_OFFSET
	movhi	r1, #5
	movhi	pc, lr
#endif
#if (GPIO_INSTANCES >= 5)
	cmp	r3, #127
	subhi	r3, r3, #128
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO4_OFFSET
	movhi	r1, #4
	movhi	pc, lr
#endif
#if (GPIO_INSTANCES >= 4)
	cmp	r3, #95
	subhi	r3, r3, #96
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO3_OFFSET
	movhi	r1, #3
	movhi	pc, lr
#endif
#if (GPIO_INSTANCES >= 3)
	cmp	r3, #63
	subhi	r3, r3, #64
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO2_OFFSET
	movhi	r1, #2
	movhi	pc, lr
#endif
#if (GPIO_INSTANCES >= 2)
	cmp	r3, #31
	subhi	r3, r3, #32
	movhi	r0, #APB_BASE
	orrhi	r0, r0, #GPIO1_OFFSET
	movhi	r1, #1
	movhi	pc, lr
#endif
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	mov	r1, #0
	mov	pc, lr

/**
 * Initiale the GPIO controller.
 */
.globl	gpio_init
gpio_init:
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO0_OFFSET
	ldr	r1, __gpio0_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio0_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio0_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio0_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#if (GPIO_INSTANCES >= 2)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO1_OFFSET
	ldr	r1, __gpio1_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio1_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio1_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio1_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 2 */
#if (GPIO_INSTANCES >= 3)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO2_OFFSET
	ldr	r1, __gpio2_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio2_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio2_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio2_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 3 */
#if (GPIO_INSTANCES >= 4)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO3_OFFSET
	ldr	r1, __gpio3_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio3_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio3_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio3_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 4 */
#if (GPIO_INSTANCES >= 5)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO4_OFFSET
	ldr	r1, __gpio4_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio4_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio4_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio4_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 5 */
#if (GPIO_INSTANCES >= 6)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO5_OFFSET
	ldr	r1, __gpio5_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio5_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio5_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio5_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 6 */
#if (GPIO_INSTANCES >= 7)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO6_OFFSET
	ldr	r1, __gpio6_afsel_dval
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	ldr	r1, __gpio6_dir_dval
	str	r1, [r0, #GPIO_DIR_OFFSET]
	ldr	r1, __gpio6_mask_dval
	str	r1, [r0, #GPIO_MASK_OFFSET]
	ldr	r1, __gpio6_data_dval
	str	r1, [r0, #GPIO_DATA_OFFSET]
	mov	r1, #0xffffffff
	str	r1, [r0, #GPIO_ENABLE_OFFSET]
#endif	/* GPIO_INSTANCES >= 6 */

#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
	mov	r0, #APB_BASE
	orr	r0, r0, #GPIO_PAD_PULL_OFFSET
	ldr	r1, __gpio0_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_0_OFFSET]
	ldr	r1, __gpio0_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_0_OFFSET]
#if (GPIO_INSTANCES >= 2)
	ldr	r1, __gpio1_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_1_OFFSET]
	ldr	r1, __gpio1_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_1_OFFSET]
#endif
#if (GPIO_INSTANCES >= 3)
	ldr	r1, __gpio2_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_2_OFFSET]
	ldr	r1, __gpio2_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_2_OFFSET]
#endif
#if (GPIO_INSTANCES >= 4)
	ldr	r1, __gpio3_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_3_OFFSET]
	ldr	r1, __gpio3_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_3_OFFSET]
#endif
#if (GPIO_INSTANCES >= 5)
	ldr	r1, __gpio4_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_4_OFFSET]
	ldr	r1, __gpio4_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_4_OFFSET]
#endif
#if (GPIO_INSTANCES >= 6)
	ldr	r1, __gpio5_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_5_OFFSET]
	ldr	r1, __gpio5_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_5_OFFSET]
#endif
#if (GPIO_INSTANCES >= 7)
	ldr	r1, __gpio6_ctrl_ena_dval
	str	r1, [r0, #GPIO_PAD_PULL_EN_6_OFFSET]
	ldr	r1, __gpio6_ctrl_dir_dval
	str	r1, [r0, #GPIO_PAD_PULL_DIR_6_OFFSET]
#endif
#endif /* IO PAD PULL CONTROL */

#if (GPIO_PAD_DS_SUPPORT == 1)
	mov	r0, #DBGBUS_BASE
	orr	r0, r0, #RCT_OFFSET
	ldr	r1, __gpio0_ds0_dval
	str	r1, [r0, #GPIO_DS0_0_OFFSET]
	ldr	r1, __gpio0_ds1_dval
	str	r1, [r0, #GPIO_DS1_0_OFFSET]
#if (GPIO_INSTANCES >= 2)
	ldr	r1, __gpio1_ds0_dval
	str	r1, [r0, #GPIO_DS0_1_OFFSET]
	ldr	r1, __gpio1_ds1_dval
	str	r1, [r0, #GPIO_DS1_1_OFFSET]
#endif
#if (GPIO_INSTANCES >= 3)
	ldr	r1, __gpio2_ds0_dval
	str	r1, [r0, #GPIO_DS0_2_OFFSET]
	ldr	r1, __gpio2_ds1_dval
	str	r1, [r0, #GPIO_DS1_2_OFFSET]
#endif
#if (GPIO_INSTANCES >= 4)
	ldr	r1, __gpio3_ds0_dval
	str	r1, [r0, #GPIO_DS0_3_OFFSET]
	ldr	r1, __gpio3_ds1_dval
	str	r1, [r0, #GPIO_DS1_3_OFFSET]
#endif
#if (GPIO_INSTANCES >= 5)
	ldr	r1, __gpio4_ds0_dval
	str	r1, [r0, #GPIO_DS0_4_OFFSET]
	ldr	r1, __gpio4_ds1_dval
	str	r1, [r0, #GPIO_DS1_4_OFFSET]
#endif
#if (GPIO_INSTANCES >= 6)
	ldr	r1, __gpio5_ds0_dval
	str	r1, [r0, #GPIO_DS0_5_OFFSET]
	ldr	r1, __gpio5_ds1_dval
	str	r1, [r0, #GPIO_DS1_5_OFFSET]
#endif
#if (GPIO_INSTANCES >= 7)
	ldr	r1, __gpio6_ds0_dval
	str	r1, [r0, #GPIO_DS0_6_OFFSET]
	ldr	r1, __gpio6_ds1_dval
	str	r1, [r0, #GPIO_DS1_6_OFFSET]
#endif
#endif /* IO PAD Drive Strength */

#if (IOMUX_SUPPORT > 0)
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
#if defined(DEFAULT_IOMUX_REG0_0)
	ldr	r1, = DEFAULT_IOMUX_REG0_0
	str	r1, [r0, #IOMUX_REG_OFFSET(0, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG0_1)
	ldr	r1, = DEFAULT_IOMUX_REG0_1
	str	r1, [r0, #IOMUX_REG_OFFSET(0, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG0_2)
	ldr	r1, = DEFAULT_IOMUX_REG0_2
	str	r1, [r0, #IOMUX_REG_OFFSET(0, 2)]
#endif
#if defined(DEFAULT_IOMUX_REG1_0)
	ldr	r1, = DEFAULT_IOMUX_REG1_0
	str	r1, [r0, #IOMUX_REG_OFFSET(1, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG1_1)
	ldr	r1, = DEFAULT_IOMUX_REG1_1
	str	r1, [r0, #IOMUX_REG_OFFSET(1, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG1_2)
	ldr	r1, = DEFAULT_IOMUX_REG1_2
	str	r1, [r0, #IOMUX_REG_OFFSET(1, 2)]
#endif
#if defined(DEFAULT_IOMUX_REG2_0)
	ldr	r1, = DEFAULT_IOMUX_REG2_0
	str	r1, [r0, #IOMUX_REG_OFFSET(2, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG2_1)
	ldr	r1, = DEFAULT_IOMUX_REG2_1
	str	r1, [r0, #IOMUX_REG_OFFSET(2, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG2_2)
	ldr	r1, = DEFAULT_IOMUX_REG2_2
	str	r1, [r0, #IOMUX_REG_OFFSET(2, 2)]
#endif
#if defined(DEFAULT_IOMUX_REG3_0)
	ldr	r1, = DEFAULT_IOMUX_REG3_0
	str	r1, [r0, #IOMUX_REG_OFFSET(3, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG3_1)
	ldr	r1, = DEFAULT_IOMUX_REG3_1
	str	r1, [r0, #IOMUX_REG_OFFSET(3, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG3_2)
	ldr	r1, = DEFAULT_IOMUX_REG3_2
	str	r1, [r0, #IOMUX_REG_OFFSET(3, 2)]
#endif

#if (GPIO_INSTANCES >= 7)
#if defined(DEFAULT_IOMUX_REG4_0)
	ldr	r1, = DEFAULT_IOMUX_REG4_0
	str	r1, [r0, #IOMUX_REG_OFFSET(4, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG4_1)
	ldr	r1, = DEFAULT_IOMUX_REG4_1
	str	r1, [r0, #IOMUX_REG_OFFSET(4, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG4_2)
	ldr	r1, = DEFAULT_IOMUX_REG4_2
	str	r1, [r0, #IOMUX_REG_OFFSET(4, 2)]
#endif
#if defined(DEFAULT_IOMUX_REG5_0)
	ldr	r1, = DEFAULT_IOMUX_REG5_0
	str	r1, [r0, #IOMUX_REG_OFFSET(5, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG5_1)
	ldr	r1, = DEFAULT_IOMUX_REG5_1
	str	r1, [r0, #IOMUX_REG_OFFSET(5, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG5_2)
	ldr	r1, = DEFAULT_IOMUX_REG5_2
	str	r1, [r0, #IOMUX_REG_OFFSET(5, 2)]
#endif
#if defined(DEFAULT_IOMUX_REG6_0)
	ldr	r1, = DEFAULT_IOMUX_REG6_0
	str	r1, [r0, #IOMUX_REG_OFFSET(6, 0)]
#endif
#if defined(DEFAULT_IOMUX_REG6_1)
	ldr	r1, = DEFAULT_IOMUX_REG6_1
	str	r1, [r0, #IOMUX_REG_OFFSET(6, 1)]
#endif
#if defined(DEFAULT_IOMUX_REG6_2)
	ldr	r1, = DEFAULT_IOMUX_REG6_2
	str	r1, [r0, #IOMUX_REG_OFFSET(6, 2)]
#endif
#endif
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#endif

	mov	pc, lr

/**
 * Configure a GPIO pin to be a HW function pin.
 *
 * @param r0 - GPIO pin number
 * @param r1 - alt function number
 * @returns - None
 */
.globl gpio_config_hw
gpio_config_hw:
	stmfd	sp!, {r11, r12, lr}
	mov	r12, r1
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_config_hw_exit
	@ Valid GPIO pin number
	mov	r11, r1
	mov	r2, #1
	@ read-modify-write GPIO_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
#if (IOMUX_SUPPORT > 0)
	mov	r0, #0x0C
	mul	r0, r11, r0
	orr	r0, r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	@ read-modify-write IOMUX_REGx_0
	ldr	r1, [r0, #0x0]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	and	r2, r12, #0x1
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #0x0]
	@ read-modify-write IOMUX_REG0_1
	ldr	r1, [r0, #0x4]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	and	r2, r12, #0x2
	lsr	r2, r2, #1
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #0x4]
	@ read-modify-write IOMUX_REG0_2
	ldr	r1, [r0, #0x8]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	and	r2, r12, #0x4
	lsr	r2, r2, #2
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #0x8]
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#endif
gpio_config_hw_exit:
	ldmfd	sp!, {r11, r12, pc}

/**
 * Configure a GPIO pin to be a SW input pin.
 *
 * @param r0 - GPIO pin number
 * @returns - None
 */
.globl gpio_config_sw_in
gpio_config_sw_in:
	stmfd	sp!, {r11, lr}
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_config_sw_in_exit
	@ Valid GPIO pin number
	mov	r11, r1
	mov	r2, #0x1
	@ read-modify-write GPIO_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
#if (IOMUX_SUPPORT > 0)
	mov	r0, #0x0C
	mul	r0, r11, r0
	orr	r0, r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	@ read-modify-write IOMUX_REGx_0
	ldr	r1, [r0, #0x0]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x0]
	@ read-modify-write IOMUX_REG0_1
	ldr	r1, [r0, #0x4]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x4]
	@ read-modify-write IOMUX_REG0_2
	ldr	r1, [r0, #0x8]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x8]
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#endif
gpio_config_sw_in_exit:
	ldmfd	sp!, {r11, pc}

/**
 * Configure a GPIO pin to be a SW output pin
 *
 * @param r0 - GPIO pin number
 * @returns - None
 */
.globl gpio_config_sw_out
gpio_config_sw_out:
	stmfd	sp!, {r11, lr}
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_config_sw_out_exit
	@ Valid GPIO pin number
	mov	r11, r1
	mov	r2, #0x1
	@ read-modify-write GPIO_DIR_REG
	ldr	r1, [r0, #GPIO_DIR_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DIR_OFFSET]
	@ read-modify-write GPIO_AFSEL_REG
	ldr	r1, [r0, #GPIO_AFSEL_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_AFSEL_OFFSET]
	@ read-modify-write GPIO_MASK_REG
	ldr	r1, [r0, #GPIO_MASK_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_MASK_OFFSET]
#if (IOMUX_SUPPORT > 0)
	mov	r0, #0x0C
	mul	r0, r11, r0
	orr	r0, r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	@ read-modify-write IOMUX_REGx_0
	ldr	r1, [r0, #0x0]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x0]
	@ read-modify-write IOMUX_REG0_1
	ldr	r1, [r0, #0x4]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x4]
	@ read-modify-write IOMUX_REG0_2
	ldr	r1, [r0, #0x8]
	mov	r2, #1
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #0x8]
	mov	r0, #APB_BASE
	orr	r0, r0, #IOMUX_OFFSET
	mov	r1, #0x1
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
	mov	r1, #0x0
	str	r1, [r0, #IOMUX_CTRL_SET_OFFSET]
#endif
gpio_config_sw_out_exit:
	ldmfd	sp!, {r11, pc}

/**
 * Set GPIO.
 */
.globl gpio_set
gpio_set:
	stmfd	sp!, {r11, lr}
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_set_exit
	@ Valid GPIO pin number
	mov	r2, #0x1
	@ read-modify-write GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	orr	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
gpio_set_exit:
	ldmfd	sp!, {r11, pc}


/**
 * Clear GPIO.
 */
.globl gpio_clr
gpio_clr:
	stmfd	sp!, {r11, lr}
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_clr_exit
	@ Valid GPIO pin number
	mov	r2, #0x1
	@ read-modify-write GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	bic	r1, r1, r2, lsl r3
	str	r1, [r0, #GPIO_DATA_OFFSET]
gpio_clr_exit:
	ldmfd	sp!, {r11, pc}

/**
 * Get GPIO value.
 */
.globl gpio_get
gpio_get:
	stmfd	sp!, {r11, lr}
	bl	gpio_to_reg
	cmp	r3, #31
	@ Wrong GPIO pin number
	bhi	gpio_get_exit_0
	@ Valid GPIO pin number
	mov	r2, #0x1
	mov	r2, r2, lsl r3
	@ read-return-value GPIO0_DATA_REG
	ldr	r1, [r0, #GPIO_DATA_OFFSET]
	tst	r1, r2
	beq	gpio_get_exit_0
	mov	r0, #0x1
	ldmfd	sp!, {r11, pc}
gpio_get_exit_0:
	mov	r0, #0x0
	ldmfd	sp!, {r11, pc}

__gpio0_afsel_dval:	.word	DEFAULT_GPIO0_AFSEL
__gpio0_dir_dval:	.word	DEFAULT_GPIO0_DIR
__gpio0_mask_dval:	.word	DEFAULT_GPIO0_MASK
__gpio0_data_dval:	.word	DEFAULT_GPIO0_DATA
#if (GPIO_INSTANCES >= 2)
__gpio1_afsel_dval:	.word	DEFAULT_GPIO1_AFSEL
__gpio1_dir_dval:	.word	DEFAULT_GPIO1_DIR
__gpio1_mask_dval:	.word	DEFAULT_GPIO1_MASK
__gpio1_data_dval:	.word	DEFAULT_GPIO1_DATA
#endif
#if (GPIO_INSTANCES >= 3)
__gpio2_afsel_dval:	.word	DEFAULT_GPIO2_AFSEL
__gpio2_dir_dval:	.word	DEFAULT_GPIO2_DIR
__gpio2_mask_dval:	.word	DEFAULT_GPIO2_MASK
__gpio2_data_dval:	.word	DEFAULT_GPIO2_DATA
#endif
#if (GPIO_INSTANCES >= 4)
__gpio3_afsel_dval:	.word	DEFAULT_GPIO3_AFSEL
__gpio3_dir_dval:	.word	DEFAULT_GPIO3_DIR
__gpio3_mask_dval:	.word	DEFAULT_GPIO3_MASK
__gpio3_data_dval:	.word	DEFAULT_GPIO3_DATA
#endif
#if (GPIO_INSTANCES >= 5)
__gpio4_afsel_dval:	.word	DEFAULT_GPIO4_AFSEL
__gpio4_dir_dval:	.word	DEFAULT_GPIO4_DIR
__gpio4_mask_dval:	.word	DEFAULT_GPIO4_MASK
__gpio4_data_dval:	.word	DEFAULT_GPIO4_DATA
#endif
#if (GPIO_INSTANCES >= 6)
__gpio5_afsel_dval:	.word	DEFAULT_GPIO5_AFSEL
__gpio5_dir_dval:	.word	DEFAULT_GPIO5_DIR
__gpio5_mask_dval:	.word	DEFAULT_GPIO5_MASK
__gpio5_data_dval:	.word	DEFAULT_GPIO5_DATA
#endif
#if (GPIO_INSTANCES >= 7)
__gpio6_afsel_dval:	.word	DEFAULT_GPIO6_AFSEL
__gpio6_dir_dval:	.word	DEFAULT_GPIO6_DIR
__gpio6_mask_dval:	.word	DEFAULT_GPIO6_MASK
__gpio6_data_dval:	.word	DEFAULT_GPIO6_DATA
#endif

#if (GPIO_PAD_PULL_CTRL_SUPPORT == 1)
__gpio0_ctrl_ena_dval:	.word	DEFAULT_GPIO0_CTRL_ENA
__gpio0_ctrl_dir_dval:	.word	DEFAULT_GPIO0_CTRL_DIR
#if (GPIO_INSTANCES >= 2)
__gpio1_ctrl_ena_dval:	.word	DEFAULT_GPIO1_CTRL_ENA
__gpio1_ctrl_dir_dval:	.word	DEFAULT_GPIO1_CTRL_DIR
#endif
#if (GPIO_INSTANCES >= 3)
__gpio2_ctrl_ena_dval:	.word	DEFAULT_GPIO2_CTRL_ENA
__gpio2_ctrl_dir_dval:	.word	DEFAULT_GPIO2_CTRL_DIR
#endif
#if (GPIO_INSTANCES >= 4)
__gpio3_ctrl_ena_dval:	.word	DEFAULT_GPIO3_CTRL_ENA
__gpio3_ctrl_dir_dval:	.word	DEFAULT_GPIO3_CTRL_DIR
#endif
#if (GPIO_INSTANCES >= 5)
__gpio4_ctrl_ena_dval:	.word	DEFAULT_GPIO4_CTRL_ENA
__gpio4_ctrl_dir_dval:	.word	DEFAULT_GPIO4_CTRL_DIR
#endif
#if (GPIO_INSTANCES >= 6)
__gpio5_ctrl_ena_dval:	.word	DEFAULT_GPIO5_CTRL_ENA
__gpio5_ctrl_dir_dval:	.word	DEFAULT_GPIO5_CTRL_DIR
#endif
#if (GPIO_INSTANCES >= 7)
__gpio6_ctrl_ena_dval:	.word	DEFAULT_GPIO6_CTRL_ENA
__gpio6_ctrl_dir_dval:	.word	DEFAULT_GPIO6_CTRL_DIR
#endif
#endif

#if (GPIO_PAD_DS_SUPPORT == 1)
__gpio0_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_0
__gpio0_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_0
#if (GPIO_INSTANCES >= 2)
__gpio1_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_1
__gpio1_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_1
#endif
#if (GPIO_INSTANCES >= 3)
__gpio2_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_2
__gpio2_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_2
#endif
#if (GPIO_INSTANCES >= 4)
__gpio3_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_3
__gpio3_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_3
#endif
#if (GPIO_INSTANCES >= 5)
__gpio4_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_4
__gpio4_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_4
#endif
#if (GPIO_INSTANCES >= 6)
__gpio5_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_5
__gpio5_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_5
#endif
#if (GPIO_INSTANCES >= 7)
__gpio6_ds0_dval:	.word	DEFAULT_GPIO_DS0_REG_6
__gpio6_ds1_dval:	.word	DEFAULT_GPIO_DS1_REG_6
#endif
#endif

