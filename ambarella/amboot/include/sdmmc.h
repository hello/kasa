/**
 * include/sdmmc.h
 *
 * SD/MMC/SDIO protocol stack from Ambarella
 *
 * History:
 *    2004/11/04 - [Charles Chiou] created file
 *    2007/01/24 - [Charles Chiou] merged from multiple headers into one
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


#ifndef __SDMMC_H__
#define __SDMMC_H__

#include <fio/partition.h>

/*===========================================================================*/

#define SDMMC_TYPE_SD		0x1
#define SDMMC_TYPE_MMC		0x2

#define SDMMC_VOLTAGE_3V3	0x1
#define SDMMC_VOLTAGE_1V8	0x2

#define SDMMC_MODE_AUTO		0x00000000
#define SDMMC_MODE_DS		0x00000001
#define SDMMC_MODE_HS		0x00000002
#define SDMMC_MODE_HS200	0x00000004 /* valid only for MMC */
#define SDMMC_MODE_SDR12	0x00000008
#define SDMMC_MODE_SDR25	0x00000010
#define SDMMC_MODE_SDR50	0x00000020
#define SDMMC_MODE_SDR104	0x00000040
#define SDMMC_MODE_DDR50	0x00000080

#define SDMMC_SEC_SIZE		512	/* sector size */
#define SDMMC_SEC_CNT		1024	/* sector count */

#define SD_CMD_TIMEOUT		1000
#define SD_TRAN_TIMEOUT		5000

/*===========================================================================*/
/*
 * cmd_x:  normal command,
 * acmd_x: app command,
 * scmd_x: sd command,
 * mcmd_x: mmc command
 */
#define cmd_0	0x0000	/* GO_IDLE_STATE	no response */
#define cmd_1	0x0102	/* SEND_OP_COND		expect 48 bit response (R3) */
#define cmd_2	0x0209	/* ALL_SEND_CID		expect 136 bit response (R2) */
#define cmd_3	0x031a	/* SET_RELATIVE_ADDR	expect 48 bit response (R6) */
#define acmd_6	0x061a	/* SET_BUS_WIDTH	expect 48 bit response (R1) */
#define acmd_23	0x171a	/* SET_WR_BLK_ERASE_CNT	expect 48 bit response (R1) */
#define cmd_6	0x061b	/* SWITCH_BUSWIDTH	expect 48 bit response (R1B) */
#define scmd_6	0x063a	/* SWITCH_BUSWIDTH	expect 48 bit response (R1) */
#define cmd_7	0x071b	/* SELECT_CARD		expect 48 bit response (R1B) */
#define cmd_8	0x081a	/* SEND_IF_COND		expect 48 bit response (R7) */
#define mcmd_8	0x083a	/* SEND_EXT_CSD		expect 48 bit response (R1) */
#define cmd_9	0x0909	/* GET_THE_CSD		expect 136 bit response (R2) */
#define cmd_11	0x0b1a	/* SWITCH VOLTAGE	expect 48 bit response (R1) */
#define cmd_13	0x0d1a	/* SEND_STATUS		expect 48 bit response (R1) */
#define acmd_13	0x0d3a	/* SEND_STATUS		expect 48 bit response (R1) */
#define cmd_16	0x101a	/* SET_BLOCKLEN		expect 48 bit response (R1) */
#define cmd_17	0x113a	/* READ_SINGLE_BLOCK	expect 48 bit response (R1) */
#define cmd_18	0x123a	/* READ_MULTIPLE_BLOCK	expect 48 bit response (R1) */
#define cmd_24	0x183a	/* WRITE_BLOCK		expect 48 bit response (R1) */
#define cmd_25	0x193a	/* WRITE_MULTIPLE_BLOCK	expect 48 bit response (R1) */
#define cmd_32	0x201a	/* ERASE_WR_BLK_START	expect 48 bit response (R1) */
#define cmd_33	0x211a	/* ERASE_WR_BLK_END	expect 48 bit response (R1) */
#define cmd_35	0x231a	/* ERASE_GROUP_START	expect 48 bit response (R1) */
#define cmd_36	0x241a	/* ERASE_GROUP_END	expect 48 bit response (R1) */
#define cmd_38	0x261b	/* ERASE		expect 48 bit response (R1B) */
#define acmd_41	0x2902	/* SD_SEND_OP_COND	expect 48 bit response (R3) */
#define acmd_42	0x2a1b	/* LOCK_UNLOCK		expect 48 bit response (R1B) */
#define acmd_51	0x333a	/* SEND_SCR		expect 48 bit response (R1) */
#define cmd_55	0x371a	/* APP_CMD		expect 48 bit response (R1) */

/*===========================================================================*/

#define SDMMC_HOST_AVAIL_OCR	0x00ff0000

#define SDMMC_CARD_BUSY		0x80000000	/**< Card Power up status bit */

/* HCS & CCS bit shift position in OCR */
#define HCS_SHIFT_BIT		30
#define CCS_SHIFT_BIT		HCS_SHIFT_BIT
#define XPC_SHIFT_BIT		28
#define S18R_SHIFT_BIT		24
#define S18A_SHIFT_BIT		S18R_SHIFT_BIT

#define ACCESS_COMMAND_SET	0x0
#define ACCESS_SET_BITS		0x1
#define ACCESS_CLEAR_BITS	0x2
#define ACCESS_WRITE_BYTE	0x3

/*===========================================================================*/

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*===========================================================================*/

extern int sdmmc_init_sd(int slot, int mode, int clock, int verbose);
extern int sdmmc_init_mmc(int slot, int mode, int clock, int verbose);
extern int sdmmc_show_card_info(void);
extern int sdmmc_read_sector(int sector, int sectors, u8 *target);
extern int sdmmc_write_sector(int sector, int sectors, u8 *image);
extern int sdmmc_erase_sector(int sector, int sectors);
extern u32 sdmmc_get_total_sectors(void);

extern int sdmmc_set_emmc_boot_info(void);
extern int sdmmc_set_emmc_normal(void);

/*===========================================================================*/

#endif

