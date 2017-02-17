/**
 * system/include/flash/slcnanddb.h
 *
 * History:
 *    2007/10/03 - [Charles Chiou] created file
 *    2007/12/18 - [Charles Chiou] changed the header to be compatible with
 *			AMBoot
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


#ifndef __FLASH__SLCNANDDB_H__
#define __FLASH__SLCNANDDB_H__

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef NULL
#define NULL 0x0
#endif

#ifndef NAND_ID3
#define NAND_ID3	0x00
#endif

#ifndef NAND_ID4
#define NAND_ID4	0x00
#endif

#ifndef NAND_ID5
#define NAND_ID5	0x00
#endif

#ifndef NAND_ECC_BIT
#define NAND_ECC_BIT	1
#endif

/* From AHB */

/* FLASH_CTR_REG (NAND mode) */
#if !defined(NAND_CTR_REG)
#define NAND_CTR_A(x)			((x) << 28)
#define NAND_CTR_SA			0x08000000
#define NAND_CTR_SE			0x04000000
#define NAND_CTR_C2			0x02000000
#define NAND_CTR_P3			0x01000000
#define NAND_CTR_I4			0x00800000
#define NAND_CTR_RC			0x00400000
#define NAND_CTR_CC			0x00200000
#define NAND_CTR_CE			0x00100000
#define NAND_CTR_EC_MAIN		0x00080000
#define NAND_CTR_EC_SPARE		0x00040000
#define NAND_CTR_EG_MAIN		0x00020000
#define NAND_CTR_EG_SPARE		0x00010000
#define NAND_CTR_WAS			0x00000400
#define NAND_CTR_WP			0x00000200
#define NAND_CTR_IE			0x00000100
#define NAND_CTR_XS			0x00000080
#define NAND_CTR_SZ_8G			0x00000070
#define NAND_CTR_SZ_4G			0x00000060
#define NAND_CTR_SZ_1G			0x00000040
#define NAND_CTR_SZ_2G			0x00000050
#define NAND_CTR_SZ_512M		0x00000030
#define NAND_CTR_SZ_256M		0x00000020
#define NAND_CTR_SZ_128M		0x00000010
#define NAND_CTR_SZ_64M			0x00000000
#define NAND_CTR_4BANK			0x00000008
#define NAND_CTR_2BANK			0x00000004
#define NAND_CTR_1BANK			0x00000000
#define NAND_CTR_WD_64BIT		0x00000003
#define NAND_CTR_WD_32BIT		0x00000002
#define NAND_CTR_WD_16BIT		0x00000001
#define NAND_CTR_WD_8BIT		0x00000000
#endif

/* Define for plane mapping */

/* Device does not support copyback command */
#ifndef NAND_PLANE_MAP_0
#define NAND_PLANE_MAP_0	0
#endif
/* plane address is according the lowest block address */
#ifndef NAND_PLANE_MAP_1
#define NAND_PLANE_MAP_1	1
#endif
/* plane address is according the highest block address */
#ifndef NAND_PLANE_MAP_2
#define NAND_PLANE_MAP_2	2
#endif
/* plane address is according the lowest and highest block address */
#ifndef NAND_PLANE_MAP_3
#define NAND_PLANE_MAP_3	3
#endif

#define NAND_ID_K9K8_SERIAL	0xecd35195
#define NAND_ID_K9K4_SERIAL	0xecdcc115

struct nand_db_s {
	const char *name;	/**< Name */
	int	devices;	/**< Number of devices(chips) */
	int	total_banks;	/**< Totals banks in system */

	int	control;	/**< Control register setting */
	int	manid;		/**< Manufacturer ID */
	int	devid;		/**< Device ID */
	int	id3;		/**< ID code from 3rd cycle */
	int	id4;		/**< ID code from 4th cycle */
	int	id5;		/**< ID code from 5th cycle */

	/** Device info */
	struct {
		int	main_size;		/**< Main area size */
		int	spare_size;		/**< Spare area size */
		int	page_size;		/**< Page size */
		int	pages_per_block;	/**< Pages per block */
		int	blocks_per_plane;	/**< Blocks per plane */
		int	blocks_per_zone;	/**< Blocks per zone */
		int	blocks_per_bank;	/**< Blocks per bank */
		int	planes_per_bank;	/**< Planes per bank */
		int	total_blocks;		/**< Total blocks */
		int	total_zones;		/**< Total zones */
		int	total_planes;		/**< Total planes */
		int	plane_mask;		/**< Plane address mask */
		int	plane_map;		/**< Plane map */
		int	column_cycles;		/**< Column access cycles */
		int	page_cycles;		/**< Page access cycles */
		int	id_cycles;		/**< ID access cycles */
		int	chip_width;		/**< Chip data bus width */
		int	chip_size;		/**< Chip size (MB) */
		int	bus_width;		/**< Data bus width of ctrl. */
		int	banks;			/**< Banks in device */
		int	ecc_bits;		/**< ECC bits of device */
	} devinfo;

	/** Chip(s) timing parameters */
	struct {
		int	tcls;
		int	tals;
		int	tcs;
		int	tds;
		int	tclh;
		int	talh;
		int	tch;
		int	tdh;
		int	twp;
		int	twh;
		int	twb;
		int	trr;
		int	trp;
		int	treh;
  		int	trb;
		int	tceh;
		int	trdelay;
		int	tclr;
		int	twhr;
		int	tir;
		int	tww;
		int	trhz;
		int	tar;
	} timing;
};

extern const struct nand_db_s *G_nand_db[];

#if defined(__cplusplus)
}
#endif

#define IMPLEMENT_NAND_DB_DEV(x)		\
	const struct nand_db_s x = {		\
		NAND_NAME,			\
		NAND_DEVICES,			\
		NAND_TOTAL_BANKS,		\
		\
		__NAND_CONTROL,			\
		NAND_MANID,			\
		NAND_DEVID,			\
		NAND_ID3,			\
		NAND_ID4,			\
		NAND_ID5,			\
		\
		{	/* devinfo */		\
			NAND_MAIN_SIZE,		\
			NAND_SPARE_SIZE,	\
			NAND_PAGE_SIZE,		\
			NAND_PAGES_PER_BLOCK,	\
			NAND_BLOCKS_PER_PLANE,	\
			NAND_BLOCKS_PER_ZONE,	\
			NAND_BLOCKS_PER_BANK,	\
			NAND_PLANES_PER_BANK,	\
			NAND_TOTAL_BLOCKS,	\
			NAND_TOTAL_ZONES,	\
			NAND_TOTAL_PLANES,	\
			NAND_PLANE_ADDR_MASK,	\
			NAND_PLANE_MAP,		\
			NAND_COLUMN_CYCLES,	\
			NAND_PAGE_CYCLES,	\
			NAND_ID_CYCLES,		\
			NAND_CHIP_WIDTH,	\
			NAND_CHIP_SIZE_MB,	\
			NAND_BUS_WIDTH,		\
			NAND_BANKS_PER_DEVICE,	\
			NAND_ECC_BIT,		\
		},				\
		{	/* timing */		\
			NAND_TCLS,		\
			NAND_TALS,		\
			NAND_TCS,		\
			NAND_TDS,		\
			NAND_TCLH,		\
			NAND_TALH,		\
			NAND_TCH,		\
			NAND_TDH,		\
			NAND_TWP,		\
			NAND_TWH,		\
			NAND_TWB,		\
			NAND_TRR,		\
			NAND_TRP,		\
			NAND_TREH,		\
			NAND_TRB,		\
			NAND_TCEH,		\
			NAND_TRDELAY,		\
			NAND_TCLR,		\
			NAND_TWHR,		\
			NAND_TIR,		\
			NAND_TWW,		\
			NAND_TRHZ,		\
			NAND_TAR,		\
		},				\
	}

#endif
