/**
 * ambhw/memory.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBHW__MEMORY_H__
#define __AMBHW__MEMORY_H__

/*===========================================================================*/
#if (CHIP_REV == A8)
#define DRAM_START_ADDR		0x00000000
#define AHB_PHYS_BASE		0xE0000000
#define APB_PHYS_BASE		0xE8000000
#define DBGBUS_PHYS_BASE	APB_PHYS_BASE
#define PHY_BUS_MAP_TYPE	1
#elif (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define DRAM_START_ADDR		0x00000000
#define AHB_PHYS_BASE		0xE0000000
#define APB_PHYS_BASE		0xE8000000
#define DBGBUS_PHYS_BASE	0xEC000000
#define PHY_BUS_MAP_TYPE	1
#elif (CHIP_REV == S2E)
#define DRAM_START_ADDR		0x00000000
#define AHB_PHYS_BASE		0x60000000
#define APB_PHYS_BASE		0x70000000
#define DBGBUS_PHYS_BASE	APB_PHYS_BASE
#define PHY_BUS_MAP_TYPE	2
#else
#define DRAM_START_ADDR		0xC0000000
#define AHB_PHYS_BASE		0x60000000
#define APB_PHYS_BASE		0x70000000
#define DBGBUS_PHYS_BASE	APB_PHYS_BASE
#define PHY_BUS_MAP_TYPE	0
#endif

/* The boot-loader still deals with physical address */
#define AHB_BASE		AHB_PHYS_BASE
#define APB_BASE		APB_PHYS_BASE
#define DBGBUS_BASE		DBGBUS_PHYS_BASE

/*===========================================================================*/
#ifndef DRAM_SIZE
#error "DRAM_SIZE undefined!"
#endif

#define SIZE_1KB		(1 * 1024)
#define SIZE_1KB_MASK		(SIZE_1KB - 1)
#define SIZE_1MB		(1024 * 1024)
#define SIZE_1MB_MASK		(SIZE_1MB - 1)

#define DRAM_END_ADDR		(DRAM_START_ADDR + DRAM_SIZE - 1)

#ifndef AMBOOT_BLD_RAM_START
#define AMBOOT_BLD_RAM_START	DRAM_START_ADDR
#endif
#define AMBOOT_BLD_RAM_MAX_SIZE	(0x000f0000)
#define AMBOOT_BLD_RAM_MAX_END	(AMBOOT_BLD_RAM_START + AMBOOT_BLD_RAM_MAX_SIZE)

#ifndef MEMFWPROG_RAM_START
#define MEMFWPROG_RAM_START	(DRAM_START_ADDR + (1 * SIZE_1MB))
#endif
#define MEMFWPROG_HOOKCMD_SIZE	(0x00010000)

#define AMBOOT_DTB_MAX_SIZE	(0x00008000)
/* ptb buffer is used to store the DTB and PTB, and must be multiple of 2048 */
#define AMBOOT_PTB_BUF_SIZE	(AMBOOT_DTB_MAX_SIZE + 0x00002000)

/*===========================================================================*/
#if defined(AMBOOT_DEV_BOOT_CORTEX)
#define CORTEX_TO_ARM11(x)	((x) + 0xC0000000)
#define ARM11_TO_CORTEX(x)	((x) - 0xC0000000)
#else
#define CORTEX_TO_ARM11(x)	(x)
#define ARM11_TO_CORTEX(x)	(x)
#endif

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

extern unsigned char bld_buf_addr[];
extern unsigned char bld_buf_end[];
extern unsigned char splash_buf_addr[];
extern unsigned char splash_buf_end[];

extern unsigned char bld_hugebuf_addr[];

extern unsigned char __heap_start[];
extern unsigned char __heap_end[];

extern unsigned int ambausb_boot_from[];

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

