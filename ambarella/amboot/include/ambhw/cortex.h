/**
 * ambhw/cortex.h
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

#ifndef __AMBHW__CORTEX_H__
#define __AMBHW__CORTEX_H__

/*===========================================================================*/
#define CORTEX_BST_MAGIC			(0xffaa5500)
#define CORTEX_BST_INVALID			(0xdeadbeaf)
#define CORTEX_BST_START_COUNTER		(0xffffffff)
#define CORTEX_BST_WAIT_LIMIT			(0x00010000)
#define AXI_CORTEX_RESET(x)			((x) << 5)
#define AXI_CORTEX_CLOCK(x)			((x) << 8)

#define CORTEX_BOOT_ADDRESS			(0x00000000)
#define CORTEX_BOOT_STRAP_ALIGN			(32)

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

extern void bld_start();
extern int bld_cortex_boot(int verbose, u32 jump_addr);

/* used for ARM11 boot Cortex */
extern u32 cortex_processor_start[];
extern u32 cortex_atag_data[];
extern u32 cortex_machine_id[];
extern u32 cortex_processor0_r0[];
extern u32 cortex_processor_status[];
extern void cortex_bst_entry();
extern void cortex_bst_head();
extern void cortex_bst_end();

/* used for Cortex boot Cortex */
extern u32 secondary_cortex_jump[];
extern int bld_boot_secondary_cortex(void);

extern int amboot_bsp_cortex_init_pre(void)
	__attribute__ ((weak));
extern int amboot_bsp_cortex_init_post(void)
	__attribute__ ((weak));

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

