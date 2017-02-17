/**
 * ambhw/memory.h
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
#define AMBOOT_BLD_RAM_MAX_SIZE	(0x00100000)
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

