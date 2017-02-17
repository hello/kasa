/**
 * ambhw/cortex.h
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

