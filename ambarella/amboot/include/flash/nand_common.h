/**
 * @file system/include/flash/nand_common.h
 *
 * History:
 *	2005/03/31 - [Charles Chiou] created file
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


#ifndef __FLASH_NAND_COMMON_H__
#define __FLASH_NAND_COMMON_H__

/**
 * Define for xdhost and nand_host
 */
#define MAIN_ONLY		0
#define SPARE_ONLY		1
#define MAIN_ECC		2
#define SPARE_ECC		3
#define SPARE_ONLY_BURST	4
#define SPARE_ECC_BURST		5

/* ------------------------------------------------------------------------- */
/* NAND HOST                                                                 */
/* ------------------------------------------------------------------------- */

/* Define for plane mapping */
/* Device does not support copyback command */
#define NAND_PLANE_MAP_0	0
/* plane address is according the lowest block address */
#define NAND_PLANE_MAP_1	1
/* plane address is according the highest block address */
#define NAND_PLANE_MAP_2	2
/* plane address is according the lowest and highest block address */
#define NAND_PLANE_MAP_3	3

/* Minimun number of translation tables in trl_blocks */
#define NAND_TRL_TABLES		16
#define NAND_SS_BLK_PAGES	1
#define NAND_IMARK_PAGES	1
#define NAND_BBINFO_SIZE	(4 * 1024)

/* NAND FTL partition ID and max partition number. */
#define NAND_FTL_PART_ID_RAW		MP_RAW
#define NAND_FTL_PART_ID_STORAGE2	MP_STG2
#define NAND_FTL_PART_ID_STORAGE	MP_STG
#define NAND_FTL_PART_ID_PREF		MP_PRF
#define NAND_FTL_PART_ID_CALIB		MP_CAL
#define NAND_MAX_FTL_PARTITION		MP_MAX

#define PREF_ZONE_THRESHOLD		1000
#define PREF_RSV_BLOCKS_PER_ZONET	24
#define PREF_MIN_RSV_BLOCKS_PER_ZONE	5
#define PREF_TRL_TABLES			0

#define CALIB_ZONE_THRESHOLD		1000
#define CALIB_RSV_BLOCKS_PER_ZONET	24
#define CALIB_MIN_RSV_BLOCKS_PER_ZONE	5
#define CALIB_TRL_TABLES		0

#define RAW_ZONE_THRESHOLD		1000
#define RAW_RSV_BLOCKS_PER_ZONET	24
#define RAW_MIN_RSV_BLOCKS_PER_ZONE	24
#define RAW_TRL_TABLES			NAND_TRL_TABLES

#define STG_ZONE_THRESHOLD		1000
#define STG_RSV_BLOCKS_PER_ZONET	24
#define STG_MIN_RSV_BLOCKS_PER_ZONE	24
#define STG_TRL_TABLES			NAND_TRL_TABLES

#define STG2_ZONE_THRESHOLD		1000
#define STG2_RSV_BLOCKS_PER_ZONET	24
#define STG2_MIN_RSV_BLOCKS_PER_ZONE	24
#define STG2_TRL_TABLES			NAND_TRL_TABLES

#ifndef __ASM__

struct nand_oobfree {
	u32 offset;
	u32 length;
};

#endif /* __ASM__ */

#endif
