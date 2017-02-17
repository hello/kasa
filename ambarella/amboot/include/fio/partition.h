/**
 * fio/partition.h
 *
 * Firmware partition information.
 *
 * History:
 *    2009/10/06 - [Chien-Yang Chen] created file
 *    2014/02/13 - [Anthony Ginger] Amboot V2
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


#ifndef __FIRMWARE_PARTITION_H__
#define __FIRMWARE_PARTITION_H__

#include <config.h>

/*===========================================================================*/
#define PART_BST		(0x00)
#define PART_BLD		(0x01)
#define PART_PTB		(0x02)
#define PART_SPL		(0x03)
#define PART_PBA		(0x04)
#define PART_PRI		(0x05)
#define PART_SEC		(0x06)
#define PART_BAK		(0x07)
#define PART_RMD		(0x08)
#define PART_ROM		(0x09)
#define PART_DSP		(0x0A)
#define PART_LNX		(0x0B)
#define PART_SWP		(0x0C)
#define PART_ADD		(0x0D)
#define PART_ADC		(0x0E)
#define PART_RAW		(0x0F)

#define HAS_IMG_PARTS		15	/* [PART_BST - PART_ADC] */
#define HAS_NO_IMG_PARTS	1	/* [PART_RAW] */
#define TOTAL_FW_PARTS		(HAS_IMG_PARTS + HAS_NO_IMG_PARTS)

#define PART_DEV_AUTO		(0x00)
#define PART_DEV_NAND		(0x01)
#define PART_DEV_EMMC		(0x02)
#define PART_DEV_SPINOR		(0x04)
#define PART_DEV_SPINAND	(0x08)

#define PART_MAX		20
#define PART_MAX_WITH_RSV	32

#define MP_MAX			5
#define MP_MAX_WITH_RSV		10

/*===========================================================================*/
/* Partitions size 							     */
/*===========================================================================*/
#ifdef CONFIG_4K_BOOT_IMAGE
#define AMBOOT_BST_FIXED_SIZE	(4096)
#else
#define AMBOOT_BST_FIXED_SIZE	(2048)
#endif
#define AMBOOT_MIN_PART_SIZE	(0x00020000)

#ifdef AMBOOT_BST_FIXED_SIZE
#undef AMBOOT_BST_SIZE
#define AMBOOT_BST_SIZE		AMBOOT_BST_FIXED_SIZE
#endif
#ifndef AMBOOT_PTB_SIZE
#define AMBOOT_PTB_SIZE		0
#endif
#ifndef AMBOOT_BLD_SIZE
#define AMBOOT_BLD_SIZE		0
#endif
#ifndef AMBOOT_SPL_SIZE
#define AMBOOT_SPL_SIZE		0
#endif
#ifndef AMBOOT_PBA_SIZE
#define AMBOOT_PBA_SIZE		0
#endif
#ifndef AMBOOT_PRI_SIZE
#define AMBOOT_PRI_SIZE		0
#endif
#ifndef AMBOOT_SEC_SIZE
#define AMBOOT_SEC_SIZE		0
#endif
#ifndef AMBOOT_BAK_SIZE
#define AMBOOT_BAK_SIZE		0
#endif
#ifndef AMBOOT_RMD_SIZE
#define AMBOOT_RMD_SIZE		0
#endif
#ifndef AMBOOT_ROM_SIZE
#define AMBOOT_ROM_SIZE		0
#endif
#ifndef AMBOOT_DSP_SIZE
#define AMBOOT_DSP_SIZE		0
#endif
#ifndef AMBOOT_LNX_SIZE
#define AMBOOT_LNX_SIZE		0
#endif
#ifndef AMBOOT_SWP_SIZE
#define AMBOOT_SWP_SIZE		0
#endif
#ifndef AMBOOT_ADD_SIZE
#define AMBOOT_ADD_SIZE		0
#endif
#ifndef AMBOOT_ADC_SIZE
#define AMBOOT_ADC_SIZE		0
#endif
#ifndef AMBOOT_RAW_SIZE
#define AMBOOT_RAW_SIZE		0
#endif

/*===========================================================================*/
/* Partitions device 							     */
/*===========================================================================*/
#ifndef PART_BST_DEV
#define PART_BST_DEV		PART_DEV_AUTO
#endif
#ifndef PART_PTB_DEV
#define PART_PTB_DEV		PART_DEV_AUTO
#endif
#ifndef PART_BLD_DEV
#define PART_BLD_DEV		PART_DEV_AUTO
#endif
#ifndef PART_SPL_DEV
#define PART_SPL_DEV		PART_DEV_AUTO
#endif
#ifndef PART_PBA_DEV
#define PART_PBA_DEV		PART_DEV_AUTO
#endif
#ifndef PART_PRI_DEV
#define PART_PRI_DEV		PART_DEV_AUTO
#endif
#ifndef PART_SEC_DEV
#define PART_SEC_DEV		PART_DEV_AUTO
#endif
#ifndef PART_BAK_DEV
#define PART_BAK_DEV		PART_DEV_AUTO
#endif
#ifndef PART_RMD_DEV
#define PART_RMD_DEV		PART_DEV_AUTO
#endif
#ifndef PART_ROM_DEV
#define PART_ROM_DEV		PART_DEV_AUTO
#endif
#ifndef PART_DSP_DEV
#define PART_DSP_DEV		PART_DEV_AUTO
#endif
#ifndef PART_LNX_DEV
#define PART_LNX_DEV		PART_DEV_AUTO
#endif
#ifndef PART_SWP_DEV
#define PART_SWP_DEV		PART_DEV_AUTO
#endif
#ifndef PART_ADD_DEV
#define PART_ADD_DEV		PART_DEV_AUTO
#endif
#ifndef PART_ADC_DEV
#define PART_ADC_DEV		PART_DEV_AUTO
#endif
#ifndef PART_RAW_DEV
#define PART_RAW_DEV		PART_DEV_AUTO
#endif

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

extern void get_part_size(int * part_size);
extern const char *get_part_str(int part_id);
extern unsigned int set_part_dev(unsigned int boot_from);
extern unsigned int get_part_dev(unsigned int part_id);

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

