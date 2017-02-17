/**
 * bld/nand.c
 *
 * Flash controller functions with NAND chips.
 *
 * History:
 *    2005/02/15 - [Charles Chiou] created file
 *    2006/07/26 - [Charles Chiou] converted to DMA descriptor-mode
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


#include <bldfunc.h>

#include <ambhw/fio.h>
#include <ambhw/nand.h>
#include <ambhw/dma.h>
#include <ambhw/vic.h>
#include <ambhw/cache.h>

#include <fio/ftl_const.h>
#include <flash/flpart.h>
#include <flash/nanddb.h>

/* ==========================================================================*/
#define NAND_CMD_ADDR(x)		((x) << 4)
#define NAND_CMD_NOP			0x0
#define NAND_CMD_DMA			0x1
#define NAND_CMD_RESET			0x2
#define NAND_CMD_NOP2			0x3
#define NAND_CMD_NOP3			0x4
#define NAND_CMD_NOP4			0x5
#define NAND_CMD_NOP5			0x6
#define NAND_CMD_COPYBACK		0x7
#define NAND_CMD_NOP6			0x8
#define NAND_CMD_ERASE			0x9
#define NAND_CMD_READID			0xa
#define NAND_CMD_NOP7			0xb
#define NAND_CMD_READSTATUS		0xc
#define NAND_CMD_NOP8			0xd
#define NAND_CMD_READ			0xe
#define NAND_CMD_PROGRAM		0xf

/* ==========================================================================*/
extern flnand_t flnand;

/**
 * DMA descriptor.
 */
struct fio_dmadesc_s
{
	u32	src_addr;	/**< Source address */
	u32	dst_addr;	/**< Destination address */
	u32	next;		/**< Next descriptor */
	u32	rpt_addr;	/**< Report address */
	u32	xfrcnt;		/**< Transfer count */
	u32	ctrl;		/**< Control */
	u32	rsv0;		/**< Reserved */
	u32	rsv1;		/**< Reserved */
	u32	rpt;		/**< Report */
	u32	rsv2;		/**< Reserved */
	u32	rsv3;		/**< Reserved */
	u32	rsv4;		/**< Reserved */
} __attribute__((packed));

static struct fio_dmadesc_s G_fio_dmadesc __attribute__((aligned(32)));
static struct fio_dmadesc_s G_fio_dma_spr_desc __attribute__((aligned(32)));

#define PAGE_SIZE_512		512
#define PAGE_SIZE_2K		2048
#define MAX_SPARE_SIZE_BLK	8192

#define read_status()						\
{								\
	/* Read Status */					\
	writel(NAND_CMD_REG, NAND_CMD_READSTATUS);		\
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);	\
								\
	writel(NAND_INT_REG, 0x0);				\
								\
	status = readl(NAND_STA_REG);				\
								\
	return (status & 0x1) ? -1 : 0;				\
}

#define ECC_STEPS			4
static u8 buffer[PAGE_SIZE_2K] __attribute__ ((aligned(32)));
static u8 dummy_buffer_bch[MAX_SPARE_SIZE_BLK] __attribute__ ((aligned(32)));

/* ==========================================================================*/
#define FLASH_TIMING_MIN(x) flash_timing(0, x)
#define FLASH_TIMING_MAX(x) flash_timing(1, x)
static int flash_timing(int minmax, int val)
{
	u32 clk, x;
	int n, r;

	/* to avoid overflow, divid clk by 1000000 first */
	clk = get_core_bus_freq_hz() / 1000000;
	x = val * clk;
#if (FIO_USE_2X_FREQ == 1)
	x *= 2;
#endif

	n = x / 1000;
	r = x % 1000;

	if (r != 0)
		n++;

	if (minmax)
		n--;

	return n < 1 ? 1 : n;
}

/**
 * Calculate address from the (block, page) pair.
 */
u32 addr_from_block_page(u32 block, u32 page)
{
	u32 rval = ((block * flnand.pages_per_block) + page);

	if (flnand.main_size == PAGE_SIZE_512)
		return (rval << 9);
	else if (flnand.main_size == PAGE_SIZE_2K)
		return (rval << 11);

	return -1;
}

int nand_probe_banks(u32 id, const struct nand_db_s *db)
{
	u64 addr64;
	u32 addrhi, addrlo;
	u32 probe;
	int i, banks = 0;;

	if (db == NULL)
		goto done;

	/* Probe for banks */
	//writel(HOST_ENABLE_REG, readl(HOST_ENABLE_REG) | 0x04);

	for (i = 0; i < 4; i++) {
		addr64 = (u64) (db->devinfo.blocks_per_bank *
				db->devinfo.pages_per_block *
				db->devinfo.main_size * i);
		addrhi = (u32) (addr64 >> 32);
		addrlo = (u32) addr64;

		writel(NAND_CTR_REG, (db->control & 0x0ffffff3) |
		       NAND_CTR_A(addrhi) | NAND_CTR_4BANK | NAND_CTR_I4);

		writel(NAND_CMD_REG, NAND_CMD_READID | addrlo);
		while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);

		writel(NAND_INT_REG, 0x0);

		probe = readl(NAND_ID_REG);
		if (probe == id) {
			banks++;
		} else {
			break;
		}
	}

done:
	return banks;
}

#define hweight8(w)		\
      (	(!!((w) & (0x01 << 0))) +	\
	(!!((w) & (0x01 << 1))) +	\
	(!!((w) & (0x01 << 2))) +	\
	(!!((w) & (0x01 << 3))) +	\
	(!!((w) & (0x01 << 4))) +	\
	(!!((w) & (0x01 << 5))) +	\
	(!!((w) & (0x01 << 6))) +	\
	(!!((w) & (0x01 << 7)))	)

static int count_zero_bits(u8 *buf, int size, int max_bits)
{
	int i, zero_bits = 0;

	for (i = 0; i < size; i++) {
		zero_bits += hweight8(~buf[i]);
		if (zero_bits > max_bits)
			break;
	}
	return zero_bits;
}

/*
	This func maybe not have effect when read (>1)pages data
	and we do not memset the erased page to 0xFF, if error bit < ecc_bits
*/
static int nand_bch_check_blank_pages(u32 pages, u8 *main, u8 *spare)
{
	u32 i, j;
	int zeroflip = 0;
	int oob_subset, main_subset;
	int zero_bits = 0;
	u8 *bsp;
	u8 *bufpos;

	bsp = spare;
	bufpos = main;
	main_subset = flnand.main_size / ECC_STEPS;
	oob_subset  = flnand.spare_size / ECC_STEPS;
	if (flnand.ecc_bits > 0x1) {
		for (i = 0; i < pages; i++) {
			zeroflip = 0;
			for (j = 0; j < ECC_STEPS; j++) {
				zero_bits = count_zero_bits(bufpos, main_subset,
								flnand.ecc_bits);
				if (zero_bits > flnand.ecc_bits)
					return -1;

				if (zero_bits)
					zeroflip = 1;

				zero_bits += count_zero_bits(bsp, oob_subset,
								flnand.ecc_bits);
				if (zero_bits > flnand.ecc_bits)
					return -1;

				bufpos += main_subset;
				bsp += oob_subset;
			}
			/* use zeroflip for declaring blank page status */
			if (zeroflip)
				putstr("Erased blank page has bitflip!\n");
		}
	}
	return 0;
}

static void nand_corrected_recovery(void)
{
	u32 fio_ctr_reg;

	/* FIO reset will just reset FIO registers, but will not affect
	 * Nand controller. */
	fio_ctr_reg = readl(FIO_CTR_REG);

	writel(FIO_RESET_REG, FIO_RESET_FIO_RST);
	rct_timer_dly_ms(1);
	writel(FIO_RESET_REG, 0x0);
	rct_timer_dly_ms(1);

	writel(FIO_CTR_REG, fio_ctr_reg);
}

/*
 * Set Flash_IO_dsm_control Register
 */
static void nand_en_bch()
{
	u32 fio_dsm_ctr = 0, fio_ctr_reg = 0, dma_dsm_ctr = 0;

	fio_ctr_reg = readl(FIO_CTR_REG);
	/* Setup FIO Dual Space Mode Control Register */
	if (flnand.ecc_bits > 0x1) {
		/* Using BCH */
		fio_dsm_ctr |= (FIO_DSM_EN | FIO_DSM_MAJP_2KB);
		dma_dsm_ctr |= (DMA_DSM_EN | DMA_DSM_MAJP_2KB);
		fio_ctr_reg |= (FIO_CTR_RS | FIO_CTR_CO);

		if (flnand.ecc_bits == 0x6) {
			fio_dsm_ctr |= FIO_DSM_SPJP_64B;
			dma_dsm_ctr |= DMA_DSM_SPJP_64B;
			fio_ctr_reg |=	FIO_CTR_ECC_6BIT;
		} else {
			fio_dsm_ctr |= FIO_DSM_SPJP_128B;
			dma_dsm_ctr |= DMA_DSM_SPJP_128B;
			fio_ctr_reg |=	FIO_CTR_ECC_8BIT;
			writel(NAND_EXT_CTR_REG, readl(NAND_EXT_CTR_REG) | NAND_EXT_CTR_SP_2X);
		}
	} else {
		/* Should not be here! */
		putstr("ECC bit is 0 or 1,Do not need to enable BCH,so it can not be here!\n\r");
	}

	writel(FIO_DSM_CTR_REG, fio_dsm_ctr);
	writel(FIO_CTR_REG, fio_ctr_reg);
	writel(FDMA_CHAN_DSM_CTR_REG(FIO_DMA_CHAN), dma_dsm_ctr);
}

/*
 * Disable Flash_IO_dsm_control and Flash_IO_control Register
 */
static void nand_dis_bch()
{
	u32 fio_ctr_reg = 0;

	fio_ctr_reg = readl(FIO_CTR_REG);
	/* Setup FIO Dual Space Mode Control Register */
	fio_ctr_reg |= FIO_CTR_RS;
	fio_ctr_reg &= ~(FIO_CTR_CO |
			 FIO_CTR_ECC_6BIT |
			 FIO_CTR_ECC_8BIT);

	writel(FIO_CTR_REG, fio_ctr_reg);
}

/**
 * Check for bad block.
 */
int nand_is_bad_block(u32 block)
{
	int ret_val = -1, i;
	u8 sbuf[1024], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((u32)sbuf + 31) & (~31));

#if defined(CONFIG_NAND_USE_FLASH_BBT)
	if(nand_has_bbt())
		return nand_isbad_bbt(block);
#endif

	ret_val = nand_read_spare(block, 0, BAD_BLOCK_PAGES, sbuf_ptr);
	if (ret_val < 0) {
		putstr("check bad block failed >> "
				"read spare data error.\r\n");
		/* Treat as factory bad block */
		return NAND_INITIAL_BAD_BLOCK;
	}

	for (i = 0; i < INIT_BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + i * (1 << NAND_SPARE_16_SHF) + 5);
		else
			bi = *(sbuf_ptr + i * (1 << NAND_SPARE_64_SHF));

		if (bi != 0xff)
			break;
	}


	/* Good block */
	if (i == INIT_BAD_BLOCK_PAGES)
		return NAND_GOOD_BLOCK;

	for (i = INIT_BAD_BLOCK_PAGES; i < BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + i * (1 << NAND_SPARE_16_SHF) + 5);
		else
			bi = *(sbuf_ptr + i * (1 << NAND_SPARE_64_SHF));

		if (bi != 0xff)
			break;
	}

	if (i < BAD_BLOCK_PAGES) {
		/* Late developed bad blocks. */
		return NAND_LATE_DEVEL_BAD_BLOCK;
	} else {
		/* Initial invalid blocks. */
		return NAND_INITIAL_BAD_BLOCK;
	}
}

void nand_output_bad_block(u32 block, int bb_type)
{
	if (bb_type & NAND_INITIAL_BAD_BLOCK) {
		putstr("initial bad block. <block ");
	} else if (bb_type & NAND_LATE_DEVEL_BAD_BLOCK) {
		putstr("late developed bad block. <block ");
	} else {
		putstr("other bad block. <block ");
	}
	putdec(block);
	putstr(">\r\n");
	putstr("Try next block...\r\n");
}

/**
 * Mark a bad block.
 */
int nand_mark_bad_block(u32 block)
{
	int ret_val = -1, i;
	u8 sbuf[256], *sbuf_ptr;
	u8 bi;

	/* make sure 32 bytes aligned */
	sbuf_ptr = (u8 *)(((u32)sbuf + 31) & (~31));

#if defined(CONFIG_NAND_USE_FLASH_BBT)
	nand_update_bbt(block, 0);
#endif

	for (i = AMB_BB_START_PAGE; i < BAD_BLOCK_PAGES; i++) {
		memset(sbuf_ptr, 0xff, flnand.spare_size);
		if (flnand.main_size == 512) {
			*(sbuf_ptr + 5) = AMB_BAD_BLOCK_MARKER;
		} else {
			*sbuf_ptr = AMB_BAD_BLOCK_MARKER;
		}

		ret_val = nand_prog_spare(block, i, 1, sbuf_ptr);
		if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"write spare data error.\r\n");
			return ret_val;
		}

		ret_val = nand_read_spare(block, i, 1, sbuf_ptr);
		if (ret_val < 0) {
			putstr("mark bad block failed >> "
				"read spare data error.\r\n");
			return ret_val;
		}

		if (flnand.main_size == 512)
			bi = *(sbuf_ptr + 5);
		else
			bi = *sbuf_ptr;

		if (bi == 0xff) {
			putstr("mark bad block failed >> "
				"verify failed at block ");
			putdec(block);
			putstr("\r\n");
			return -1;
		}
	}

	return 0;
}

/**
 * Initialize NAND parameters.
 */
int nand_init(void)
{
	u32 id;
	u8 manid, devid, id3, id4, id5 = 0;
	int i;
	int match = 0;
	flnand_t *fn = &flnand;
	struct nand_db_s db;
	u32 t0_reg, t1_reg, t2_reg, t3_reg, t4_reg, t5_reg;
	u8 tcls, tals, tcs, tds;
	u8 tclh, talh, tch, tdh;
	u8 twp, twh, twb, trr;
	u8 trp, treh, trb, tceh;
	u8 trdelay, tclr, twhr, tir;
	u8 tww, trhz, tar;
	int sblk;
	int nblk;
	int part_size[HAS_IMG_PARTS];
#ifdef ARCH_SUPPORT_BCH_ID5
	u32 sys_config_val;
#endif
	u32 total_blocks;

	vic_set_type(DMA_FIOS_IRQ, VIRQ_LEVEL_HIGH);

	writel(FIO_DMACTR_REG,
	       (readl(FIO_DMACTR_REG) & 0xcfffffff) | FIO_DMACTR_FL);

	/* Force ReadID with 4-cycles */
	writel(NAND_CTR_REG, readl(NAND_CTR_REG) | NAND_CTR_I4);

	/* Reset chip */
	writel(NAND_CMD_REG, NAND_CMD_RESET);
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);
	writel(NAND_INT_REG, 0x0);

	/* Read ID with maximun 5 times if id is 0. */
	for (i = 0; i < 5; i++) {
		writel(NAND_CMD_REG, NAND_CMD_READID);
		while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);
		writel(NAND_INT_REG, 0x0);
		id = readl(NAND_ID_REG);
		if (id)
			break;
	}

#ifdef ARCH_SUPPORT_BCH_ID5
	{
		u32 id_bak = 0;
		/* Read ID5 with maximun 5 times if id is 0. */
		/* Disable NAND_CTR_I4 */
		writel(NAND_CTR_REG, readl(NAND_CTR_REG) & ~(NAND_CTR_I4));
		writel(NAND_EXT_CTR_REG, readl(NAND_EXT_CTR_REG) | NAND_EXT_CTR_I5);
		for (i = 0; i < 5; i++) {
			writel(NAND_CMD_REG, NAND_CMD_READID);
			while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);
			writel(NAND_INT_REG, 0x0);
			id_bak = readl(NAND_ID_REG);
			id5 = readl(NAND_EXT_ID5_REG) & 0xff;
			if (id5)
				break;
		}
	}
#endif

	manid = (id >> 24);
	devid = (id >> 16);
	id3 = (id >> 8);
	id4 = id & 0xff;

	if (id == NAND_ID && id5 == NAND_ID5) {
		match = 1;
		goto matched;
	}

	/* Search the NAND-DB for an exact match */
	for (i = 0; G_nand_db[i] != NULL; i++) {
		if (G_nand_db[i]->manid == manid &&
		    G_nand_db[i]->devid == devid &&
		    G_nand_db[i]->id3 == id3 &&
		    G_nand_db[i]->id4 == id4 &&
			G_nand_db[i]->id5 == id5) {
			match = 3;
			if (id == NAND_ID_K9K8_SERIAL ||
		    	    id == NAND_ID_K9K4_SERIAL) {
		    	    	/* These kinds of NAND have the same ID and */
				/* need to probe the banks to match exactly. */
				goto probe_banks;
			} else {
				goto matched;
			}
		}
	}
	/* Step to search the same ID nand without id5
	 (some platform do not support ID5) */
	/*
	In this situation, we make sure the nand is match the right nand data,
	we do not check only id with NAND_ID, but search the id in the NAND_DB,
	For example the SPANSION S34ML0xGx have the same NAND_ID in different
	ecc support nand.
	In the NAND_DB, we should make sure the ecc-1 bit nand db is ahead of
	the ecc-x bit nand db which has the same id[1-4].
	*/

	/* Search the NAND-DB for an exact match */
	for (i = 0; G_nand_db[i] != NULL; i++) {
		if (G_nand_db[i]->manid == manid &&
		    G_nand_db[i]->devid == devid &&
		    G_nand_db[i]->id3 == id3 &&
		    G_nand_db[i]->id4 == id4) {
			match = 3;
			if (id == NAND_ID_K9K8_SERIAL ||
			id == NAND_ID_K9K4_SERIAL) {
				/* These kinds of NAND have the same ID and */
				/* need to probe the banks to match exactly. */
				goto probe_banks;
			} else {
				goto matched;
			}
		}
	}
	if ((id >> 16) == (NAND_ID >> 16)) {
		match = 2;
		goto matched;
	}

	/* Search the NAND-DB for a match on manfid & devid */
	for (i = 0; G_nand_db[i] != NULL; i++) {
		if (G_nand_db[i]->manid == manid &&
		    G_nand_db[i]->devid == devid) {
			match = 4;
			goto matched;
		}
	}

probe_banks:
	if (match == 3) {
		/* Re-match K9K8 and K9K4 serial NAND. */
		match = 0;
	    	fn->banks = nand_probe_banks(id, G_nand_db[i]);
		/* Search the NAND-DB for an exact match with banks. */
		for (i = 0; G_nand_db[i] != NULL; i++) {
			if (G_nand_db[i]->manid == manid &&
			    G_nand_db[i]->devid == devid &&
			    G_nand_db[i]->id3 == id3 &&
			    G_nand_db[i]->id4 == id4 &&
			    G_nand_db[i]->total_banks == fn->banks) {
				match = 5;
				goto matched;
			}
		}

		/* Search the NAND-DB for an exact match with one bank. */
		for (i = 0; G_nand_db[i] != NULL; i++) {
			if (G_nand_db[i]->manid == manid &&
			    G_nand_db[i]->devid == devid &&
			    G_nand_db[i]->id3 == id3 &&
			    G_nand_db[i]->id4 == id4 &&
			    G_nand_db[i]->total_banks == 1) {
				match = 6;
				goto matched;
			}
		}
	}

matched:
	if (match <= 2) {
		db.name = NAND_NAME;
		db.devices = NAND_DEVICES;
		db.total_banks = NAND_TOTAL_BANKS;
		db.control = NAND_CONTROL;
		db.manid = NAND_MANID;
		db.id3 = NAND_ID3;
		db.id4 = NAND_ID4;

		db.devinfo.main_size = NAND_MAIN_SIZE;
		db.devinfo.spare_size = NAND_SPARE_SIZE;
		db.devinfo.page_size = NAND_PAGE_SIZE;
		db.devinfo.pages_per_block = NAND_PAGES_PER_BLOCK;
		db.devinfo.blocks_per_plane = NAND_BLOCKS_PER_PLANE;
		db.devinfo.blocks_per_bank = NAND_BLOCKS_PER_BANK;
		db.devinfo.planes_per_bank = NAND_PLANES_PER_BANK;
		db.devinfo.total_blocks = NAND_TOTAL_BLOCKS;
		db.devinfo.total_zones = NAND_TOTAL_ZONES;
		db.devinfo.total_planes = NAND_TOTAL_PLANES;
		db.devinfo.plane_mask = NAND_PLANE_MASK;
		db.devinfo.plane_map = NAND_PLANE_MAP;
		db.devinfo.column_cycles = NAND_COLUMN_CYCLES;
		db.devinfo.page_cycles = NAND_PAGE_CYCLES;
		db.devinfo.id_cycles = NAND_ID_CYCLES;
		db.devinfo.chip_width = NAND_CHIP_WIDTH;
		db.devinfo.chip_size = NAND_CHIP_SIZE_MB;
		db.devinfo.bus_width = NAND_BUS_WIDTH;
		db.devinfo.banks = NAND_BANKS_PER_DEVICE;
		db.devinfo.ecc_bits = NAND_ECC_BIT;

		db.timing.tcls = NAND_TCLS;
		db.timing.tals = NAND_TALS;
		db.timing.tcs = NAND_TCS;
		db.timing.tds = NAND_TDS;
		db.timing.tclh = NAND_TCLH;
		db.timing.talh = NAND_TALH;
		db.timing.tch = NAND_TCH;
		db.timing.tdh = NAND_TDH;
		db.timing.twp = NAND_TWP;
		db.timing.twh = NAND_TWH;
		db.timing.twb = NAND_TWB;
		db.timing.trr = NAND_TRR;
		db.timing.trp = NAND_TRP;
		db.timing.treh = NAND_TREH;
		db.timing.trb = NAND_TRB;
		db.timing.tceh = NAND_TCEH;
		db.timing.trdelay = NAND_TRDELAY;
		db.timing.tclr = NAND_TCLR;
		db.timing.twhr = NAND_TWHR;
		db.timing.tir = NAND_TIR;
		db.timing.tww = NAND_TWW;
		db.timing.trhz = NAND_TRHZ;
		db.timing.tar = NAND_TAR;
	} else {
		memcpy(&db, G_nand_db[i], sizeof(db));
		if (db.total_banks == 0)
			db.total_banks = nand_probe_banks(id, &db);
	}

	fn->nandtiming0 = NAND_TIM0_TCLS(db.timing.tcls) |
		 NAND_TIM0_TALS(db.timing.tals) |
		 NAND_TIM0_TCS(db.timing.tcs)   |
		 NAND_TIM0_TDS(db.timing.tds);

	fn->nandtiming1 = NAND_TIM1_TCLH(db.timing.tclh) |
		 NAND_TIM1_TALH(db.timing.talh) |
		 NAND_TIM1_TCH(db.timing.tch)   |
		 NAND_TIM1_TDH(db.timing.tdh);

	fn->nandtiming2 = NAND_TIM2_TWP(db.timing.twp) |
		 NAND_TIM2_TWH(db.timing.twh) |
		 NAND_TIM2_TWB(db.timing.twb) |
		 NAND_TIM2_TRR(db.timing.trr);

	fn->nandtiming3 = NAND_TIM3_TRP(db.timing.trp)	|
		 NAND_TIM3_TREH(db.timing.treh) |
		 NAND_TIM3_TRB(db.timing.trb)   |
		 NAND_TIM3_TCEH(db.timing.tceh);

	fn->nandtiming4 = NAND_TIM4_TRDELAY(db.timing.trdelay) |
		 NAND_TIM4_TCLR(db.timing.tclr) |
		 NAND_TIM4_TWHR(db.timing.twhr) |
		 NAND_TIM4_TIR(db.timing.tir);

	fn->nandtiming5 = NAND_TIM5_TWW(db.timing.tww) |
		 NAND_TIM5_TRHZ(db.timing.trhz) |
		 NAND_TIM5_TAR(db.timing.tar);

	tcls	= FLASH_TIMING_MIN(db.timing.tcls);
	tals	= FLASH_TIMING_MIN(db.timing.tals);
	tcs	= FLASH_TIMING_MIN(db.timing.tcs);
	tds	= FLASH_TIMING_MIN(db.timing.tds);
	tclh	= FLASH_TIMING_MIN(db.timing.tclh);
	talh	= FLASH_TIMING_MIN(db.timing.talh);
	tch	= FLASH_TIMING_MIN(db.timing.tch);
	tdh	= FLASH_TIMING_MIN(db.timing.tdh);
	twp	= FLASH_TIMING_MIN(db.timing.twp);
	twh	= FLASH_TIMING_MIN(db.timing.twh);
	twb	= FLASH_TIMING_MAX(db.timing.twb);
	trr	= FLASH_TIMING_MIN(db.timing.trr);
	trp	= FLASH_TIMING_MIN(db.timing.trp);
	treh	= FLASH_TIMING_MIN(db.timing.treh);
	trb	= FLASH_TIMING_MAX(db.timing.trb);
	tceh	= FLASH_TIMING_MAX(db.timing.tceh);
	trdelay = trp + treh;
	tclr	= FLASH_TIMING_MIN(db.timing.tclr);
	twhr	= FLASH_TIMING_MIN(db.timing.twhr);
	tir	= FLASH_TIMING_MIN(db.timing.tir);
	tww	= FLASH_TIMING_MIN(db.timing.tww);
	trhz	= FLASH_TIMING_MAX(db.timing.trhz);
	tar	= FLASH_TIMING_MIN(db.timing.tar);

	t0_reg = NAND_TIM0_TCLS(tcls) |
		 NAND_TIM0_TALS(tals) |
		 NAND_TIM0_TCS(tcs)   |
		 NAND_TIM0_TDS(tds);

	t1_reg = NAND_TIM1_TCLH(tclh) |
		 NAND_TIM1_TALH(talh) |
		 NAND_TIM1_TCH(tch)   |
		 NAND_TIM1_TDH(tdh);

	t2_reg = NAND_TIM2_TWP(twp) |
		 NAND_TIM2_TWH(twh) |
		 NAND_TIM2_TWB(twb) |
		 NAND_TIM2_TRR(trr);

	t3_reg = NAND_TIM3_TRP(trp)	|
		 NAND_TIM3_TREH(treh) |
		 NAND_TIM3_TRB(trb)   |
		 NAND_TIM3_TCEH(tceh);

	t4_reg = NAND_TIM4_TRDELAY(trdelay) |
		 NAND_TIM4_TCLR(tclr) |
		 NAND_TIM4_TWHR(twhr) |
		 NAND_TIM4_TIR(tir);

	t5_reg = NAND_TIM5_TWW(tww) |
		 NAND_TIM5_TRHZ(trhz) |
		 NAND_TIM5_TAR(tar);

	fn->control = db.control;
	fn->timing0 = t0_reg;
	fn->timing1 = t1_reg;
	fn->timing2 = t2_reg;
	fn->timing3 = t3_reg;
	fn->timing4 = t4_reg;
	fn->timing5 = t5_reg;
	fn->main_size = db.devinfo.main_size;
	fn->spare_size = db.devinfo.spare_size;
	fn->blocks_per_bank = db.devinfo.blocks_per_bank;
	fn->pages_per_block = db.devinfo.pages_per_block;
	fn->block_size = fn->main_size * fn->pages_per_block;
	fn->banks = db.total_banks;

	fn->ecc_bits = 1;
#ifdef ARCH_SUPPORT_BCH_ID5
	sys_config_val = rct_get_nand_poc();
	if ((sys_config_val & RCT_BOOT_NAND_ECC_BCH_EN) ==
		RCT_BOOT_NAND_ECC_BCH_EN) {
		if ((sys_config_val & RCT_BOOT_NAND_ECC_SPARE_2X) ==
			RCT_BOOT_NAND_ECC_SPARE_2X) {
			fn->ecc_bits = 8;
		} else {
			fn->ecc_bits = 6;
		}

		if (fn->main_size != 2048) {
			putstr("nand_init failed!\n\r");
			putstr("Small page does not support multi-bits ECC!\n\r");
			while(1);
		}

		if (fn->ecc_bits == 8 ) {
			if (fn->spare_size != 128) {
				putstr("nand_init failed!\n\r");
				putstr("Spare size must be 2x for 8-bits ECC!\n\r");
				while(1);
			}
		}
	}
#endif

	if (fn->ecc_bits > 1) {
#ifdef ARCH_SUPPORT_BCH_ID5
		nand_en_bch();
#else
		/* Should not be here! */
		putstr("Not support multi-bits ECC w/o DSM!\n\r");
#endif
	}

	get_part_size(part_size);

	sblk = nblk = 0;
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if ((get_part_dev(i) & PART_DEV_NAND) != PART_DEV_NAND) {
			continue;
		}

		sblk += nblk;
		nblk = part_size[i] / fn->block_size;
		if ((part_size[i] % fn->block_size) != 0x0)
			nblk++;
		fn->sblk[i] = (nblk == 0) ? 0 : sblk;
		fn->nblk[i] = nblk;
	}
	for (; i < PART_MAX; i++) {
		fn->sblk[i] = 0;
		fn->nblk[i] = 0;
	}
	total_blocks = (fn->blocks_per_bank * fn->banks);
	nblk = (total_blocks > sblk) ? (total_blocks - sblk) : 0;
	//Raw part include BBT, take care!
	fn->sblk[PART_RAW] = (nblk == 0) ? 0 : sblk;
	fn->nblk[PART_RAW] = nblk;
	if (fn->sblk[PART_RAW] < 2) {
		putstr("No Space for BBT!\n\r");
	}

	if (fn->banks == 0)
		return -1;

	return 0;
}

int nand_is_init(void)
{
	if (flnand.banks == 0 || flnand.banks > 4)
		return 0;
	else
		return 1;
}

/**
 * Reset NAND flash chip(s).
 */
int nand_reset(void)
{
	u32 ctr;
	//u32 tmp;

	/* Setup Flash IO Control Register (exit random mode) */
	ctr = readl(FIO_CTR_REG);
	if (ctr & FIO_CTR_RR) {
		ctr &= ~FIO_CTR_RR;
		ctr |= FIO_CTR_XD;
		writel(FIO_CTR_REG, ctr);
	}

	/* Clear the FIO DMA Status Register */
	writel(FIO_DMASTA_REG, 0x0);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Enable NAND flash bank(s) */
	if (flnand.banks == 1) {
		flnand.control = flnand.control & (~0xc);
		//tmp = readl(HOST_ENABLE_REG);
		//writel(HOST_ENABLE_REG, tmp & ~0x04);
	} else if (flnand.banks == 2) {
		flnand.control = (flnand.control & ~0xc) | NAND_CTR_2BANK;
		//tmp = readl(HOST_ENABLE_REG);
		//writel(HOST_ENABLE_REG, tmp | 0x04);
	} else if (flnand.banks == 3) {
		flnand.control = (flnand.control & ~0xc) | NAND_CTR_4BANK;
		//tmp = readl(HOST_ENABLE_REG);
		//writel(HOST_ENABLE_REG, tmp | 0x04);
	} else if (flnand.banks == 4) {
		flnand.control = (flnand.control & ~0xc) | NAND_CTR_4BANK;
		//tmp = readl(HOST_ENABLE_REG);
		//writel(HOST_ENABLE_REG, tmp | 0x04);
	}

	/* Setup NAND Flash Control Register */
	writel(NAND_CTR_REG, flnand.control);
	writel(NAND_INT_REG, 0x0);

	/* Setup flash timing register */
	writel(NAND_TIM0_REG, flnand.timing0);
	writel(NAND_TIM1_REG, flnand.timing1);
	writel(NAND_TIM2_REG, flnand.timing2);
	writel(NAND_TIM3_REG, flnand.timing3);
	writel(NAND_TIM4_REG, flnand.timing4);
	writel(NAND_TIM5_REG, flnand.timing5);

	return 0;
}


/**
 * Read multiple pages from NAND flash with ecc check.
 */
int nand_read_pages(u32 block, u32 page, u32 pages,
		const u8 *main_buf, const u8 *spare_buf, u32 enable_ecc)
{
	int i;
	u32 status;
	u32 addr;
	u32 mlen = 0, slen = 0;
	u32 nand_ctr_reg = 0;
	u32 spare_buf_addr = 0;
	u32 fio_ctr_reg = 0;

#if defined(DEBUG)
	putstr("nand_read_pages( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)main_buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (main_buf == NULL)) {
		putstr("ERR: parameter error in nand_read_pages()");
		return -1;
	}

	for (i = 0; i < pages; i++)
		mlen += flnand.main_size;

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	clean_flush_d_cache((void *) main_buf, mlen);

	/* Setup Flash Control Register */
#if (NAND_XD_SUPPORT_WAS == 0) && (NAND_SUPPORT_INTLVE == 0)
	nand_ctr_reg = flnand.control;
#elif (NAND_XD_SUPPORT_WAS >= 1) && (NAND_SUPPORT_INTLVE == 0)
	nand_ctr_reg = flnand.control | NAND_CTR_WAS;
#elif (NAND_XD_SUPPORT_WAS >= 1) && (NAND_SUPPORT_INTLVE == 1)
	nand_ctr_reg = flnand.control | NAND_CTR_WAS;

#if defined(NAND_K9K8_INTLVE)
	nand_ctr_reg = flnand.control | NAND_CTR_K9;
#endif
#endif

	/* Setup Flash IO Control Register */
	if (enable_ecc) {
		if (flnand.ecc_bits > 1) {
			slen = pages * flnand.spare_size;
			if (spare_buf == NULL ) {
				spare_buf_addr = (u32) dummy_buffer_bch;
				clean_flush_d_cache((void *) dummy_buffer_bch, slen);
			} else {
				spare_buf_addr = (u32) spare_buf;
				clean_flush_d_cache((void *) spare_buf, slen);
			}
			nand_en_bch();
			/* Setup Flash Control Register*/
			/* Don't set NAND_CTR_EC_MAIN, because we use BCH */
			writel(NAND_CTR_REG, nand_ctr_reg | NAND_CTR_SE);
			/* Clean Flash_IO_ecc_rpt_status Register */
			writel(FIO_ECC_RPT_STA_REG, 0x0);
		} else {
			/* Setup Flash IO Control Register */
			writel(FIO_CTR_REG, FIO_CTR_XD | FIO_CTR_RS | FIO_CTR_CO);

			/* Setup Flash Control Register*/
			writel(NAND_CTR_REG, nand_ctr_reg | NAND_CTR_SE | NAND_CTR_EC_MAIN);
		}
	} else {
		if (flnand.ecc_bits > 1) {
			slen = pages * flnand.spare_size;
			if (spare_buf == NULL)
				spare_buf_addr = (u32) dummy_buffer_bch;
			else
				spare_buf_addr = (u32) spare_buf;

			clean_flush_d_cache((void *)spare_buf_addr, slen);
			nand_dis_bch();
			fio_ctr_reg = FIO_CTR_XD | FIO_CTR_RS;
		} else {
			slen = 0;
			fio_ctr_reg = FIO_CTR_XD;
		}

		/* NO ECC */
		writel(FIO_CTR_REG, fio_ctr_reg);
		writel(NAND_CTR_REG, nand_ctr_reg);
	}

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(FDMA_CHAN_SRC_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
	writel(FDMA_CHAN_DST_REG(FIO_DMA_CHAN), (u32) main_buf);

	if (flnand.ecc_bits > 1) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG(FIO_DMA_CHAN), 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_DST_REG(FIO_DMA_CHAN), (u32) spare_buf_addr);
		writel(FDMA_CHAN_SPR_CNT_REG(FIO_DMA_CHAN), slen);
	}

	writel(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_WM		|
	       DMA_CHANX_CTR_NI		|
	       DMA_NODC_MN_BURST_SIZE	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr = addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_FL		|
	       FIO_MN_BURST_SIZE	|
	       (mlen + slen));

	/* Wait for interrupt for DMA done */
	while (((readl(FDMA_REG(DMA_INT_OFFSET)) &
		DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0));
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);	/* clear */
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0);

	status = readl(FIO_DMASTA_REG);

	writel(FIO_DMASTA_REG, 0x0);	/* clear */

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Wait for interrupt for NAND operation done */
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);
	writel(NAND_INT_REG, 0x0);

	if (flnand.ecc_bits > 1 && enable_ecc) {
		status = readl(FIO_ECC_RPT_STA_REG);
		if (status & FIO_ECC_RPT_FAIL) {
			/* Workaround for page never used, BCH will failed */
			i = nand_bch_check_blank_pages(pages, (u8 *)main_buf,
				(u8 *)(uintptr_t)spare_buf_addr);

			if (i < 0) {
				putstr("BCH corrected failed (0x");
				puthex(status);
				putstr(")!\n\r");
			}
		} else if (status & FIO_ECC_RPT_ERR) {
#if 0
			putstr("BCH code corrected (0x");
			puthex(status);
			putstr(")!\n\r");
#endif
			/* once bitflip and data corrected happened, BCH will keep
			 * on to report bitflip in next read operation, even though
			 * there is no bitflip happened really. So this is a workaround
			 * to get it back. */
			nand_corrected_recovery();
			return 1;
		}
	}
	return 0;
}

static int nand_read(u32 block, u32 page, u32 pages, u8 *buf)
{
	int rval = 0;
	u32 first_blk_pages, blocks, last_blk_pages;
	u32 bad_blks = 0;

#if defined(DEBUG)
	putstr("nand_read( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif

	first_blk_pages = flnand.pages_per_block - page;
	if (pages > first_blk_pages) {
		pages -= first_blk_pages;
		blocks = pages / flnand.pages_per_block;
		last_blk_pages = pages % flnand.pages_per_block;
	} else {
		first_blk_pages = pages;
		blocks = 0;
		last_blk_pages = 0;
	}

	if (first_blk_pages) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, page, first_blk_pages, buf, NULL, 1);
		if (rval < 0)
			return -1;
		block++;
		buf += first_blk_pages * flnand.main_size;
	}

	while (blocks > 0) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, 0, flnand.pages_per_block, buf, NULL, 1);
		if (rval < 0)
			return -1;
		block++;
		blocks--;
		buf += flnand.block_size;
	}

	if (last_blk_pages) {
		while (nand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = nand_read_pages(block, 0, last_blk_pages, buf, NULL, 1);
		if (rval < 0)
			return -1;
	}

	return bad_blks;
}

static void nand_get_offset_adr(u32 *block, u32 *page, u32 pages, u32 bad_blks)
{
	u32 blocks;

	blocks = pages / flnand.pages_per_block;
	pages  = pages % flnand.pages_per_block;

	*block =  *block + blocks;
	*page += pages;

	if (*page >= flnand.pages_per_block) {
		*page -= flnand.pages_per_block;
		*block += 1;
	}

	*block += bad_blks;
}

/**
 * Read data from NAND flash to memory.
 * dst - address in dram.
 * src - address in nand device.
 * len - length to be read from nand.
 * return - length of read data.
 */
int nand_read_data(u8 *dst, u8 *src, int len)
{
	u32 block, page, pages, pos;
	u32 first_ppage_size, last_ppage_size;
	int val, rval = -1;

#if defined(DEBUG)
	putstr("nand_read_data( 0x");
	puthex((u32)dst);
	putstr(", 0x");
	puthex((u32)src);
	putstr(", ");
	putdec(len);
	putstr(" )\r\n");
#endif

	/* translate address to block, page, address */
	val = (int) src;
	block = val / flnand.block_size;
	val  -= block * flnand.block_size;
	page  = val / flnand.main_size;
	pos   = val % flnand.main_size;
	pages = len / flnand.main_size;

	if (pos == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = flnand.main_size - pos;

	if (len >= first_ppage_size) {
		pages = (len - first_ppage_size) / flnand.main_size;

		last_ppage_size = (len - first_ppage_size) % flnand.main_size;
	} else {
		first_ppage_size = len;
		pages = 0;
		last_ppage_size = 0;
	}

	if (len !=
	    (first_ppage_size + pages * flnand.main_size + last_ppage_size)) {
		return -1;
	}

	len = 0;
	if (first_ppage_size) {
		rval = nand_read(block, page, 1, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) (buffer + pos), first_ppage_size);
		dst += first_ppage_size;
		len += first_ppage_size;
		nand_get_offset_adr(&block, &page, 1, rval);
	}

	if (pages > 0) {
		rval = nand_read(block, page, pages, dst);
		if (rval < 0)
			return len;

		dst += pages * flnand.main_size;
		len += pages * flnand.main_size;
		nand_get_offset_adr(&block, &page, pages, rval);
	}

	if (last_ppage_size > 0) {
		rval = nand_read(block, page, 1, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) buffer, last_ppage_size);
		len += last_ppage_size;
	}

	return len;
}

/**
 * Program a page to NAND flash.
 */
int nand_prog_pages(u32 block, u32 page, u32 pages, const u8 *main_buf, const u8 *spare_buf)
{
	int i;
	u32 status;
	u32 addr;
	u32 mlen = 0, slen = 0;
	u32 nand_ctr_reg = 0;
	u32 spare_buf_addr = 0;
	u32 dma_burst_ctrl, fio_burst_ctrl;

#if defined(DEBUG)
	putstr("nand_prog_pages( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)main_buf);
	putstr(" )\r\n");
#endif

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (main_buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_pages()");
		return -1;
	}

	for (i = 0; i < pages; i++)
		mlen += flnand.main_size;

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	clean_d_cache((void *) main_buf, mlen);

	/* diable write protect */
	//gpio_set(FL_WP);

	/* Always enable ECC */
	if (flnand.ecc_bits > 1) {
		/* Don't set NAND_CTR_EC_MAIN, because we use BCH */
		nand_ctr_reg = flnand.control | NAND_CTR_SE;

		slen = pages * flnand.spare_size;

		if (spare_buf == NULL) {
			memset(dummy_buffer_bch, 0xff, slen);
			spare_buf_addr = (u32) dummy_buffer_bch;
			clean_d_cache((void *) dummy_buffer_bch, slen);
		} else {
			spare_buf_addr = (u32) spare_buf;
			clean_d_cache((void *) spare_buf, slen);
		}
		nand_en_bch();

		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, nand_ctr_reg);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);

		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE | NAND_CTR_EG_MAIN);
	}

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(FDMA_CHAN_SRC_REG(FIO_DMA_CHAN), (u32) main_buf);
	writel(FDMA_CHAN_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);

	dma_burst_ctrl = DMA_NODC_MN_BURST_SIZE;
	fio_burst_ctrl = FIO_MN_BURST_SIZE;

	if ((flnand.ecc_bits > 1)) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG(FIO_DMA_CHAN), 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG(FIO_DMA_CHAN), spare_buf_addr);
		writel(FDMA_CHAN_SPR_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_CNT_REG(FIO_DMA_CHAN), slen);

		dma_burst_ctrl = DMA_NODC_MN_BURST_SIZE8;
		fio_burst_ctrl = FIO_MN_BURST_SIZE8;
	}

	writel(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_RM		|
	       DMA_CHANX_CTR_NI		|
	       dma_burst_ctrl	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl	|
			(mlen + slen));

	/* Wait for interrupt for NAND operation done and DMA done */
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0 ||
		((readl(FDMA_REG(DMA_INT_OFFSET)) &
			DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0) ||
		(readl(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN)) &
			DMA_CHANX_CTR_EN) ||
		(readl(FIO_DMASTA_REG) & FIO_DMASTA_DN) == 0x0 ||
		(readl(FIO_DMACTR_REG) & FIO_DMACTR_EN));
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	if (flnand.ecc_bits > 1) {
		status = readl(FIO_ECC_RPT_STA_REG);
		if (status & FIO_ECC_RPT_FAIL) {
				putstr("BCH corrected failed (0x");
				puthex(status);
				putstr(")!\n\r");
		} else if (status & FIO_ECC_RPT_ERR) {
			putstr("BCH code corrected (0x");
			puthex(status);
			putstr(")!\n\r");
		}
	}
	read_status();
}

int nand_prog_pages_noecc(u32 block, u32 page, u32 pages, const u8 *buf)
{
	int i;
	u32 status;
	u32 addr;
	u32 mlen = 0, slen = 0;
	u32 spare_buf_addr = 0;
	u32 dma_burst_ctrl, fio_burst_ctrl;

	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_pages()");
		return -1;
	}

	for (i = 0; i < pages; i++)
		mlen += flnand.main_size;

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	clean_d_cache((void *)buf, mlen);

	if (flnand.ecc_bits > 1) {
		slen = pages * flnand.spare_size;

		memset(dummy_buffer_bch, 0xff, slen);
		spare_buf_addr = (u32)dummy_buffer_bch;
		clean_d_cache((void *)dummy_buffer_bch, slen);

		nand_dis_bch();

		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		putstr("ERR: Not implemented for 1-bit ECC yet!\r\n");
		return -1;
	}

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
	/* Setup Flash Control Register */
	writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);

	/* Setup main external DMA engine transfer */
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(FDMA_CHAN_SRC_REG(FIO_DMA_CHAN), (u32)buf);
	writel(FDMA_CHAN_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);

	dma_burst_ctrl = DMA_NODC_MN_BURST_SIZE;
	fio_burst_ctrl = FIO_MN_BURST_SIZE;

	if ((flnand.ecc_bits > 1)) {
		/* Setup spare external DMA engine transfer */
		writel(FDMA_CHAN_SPR_STA_REG(FIO_DMA_CHAN), 0x0);
		writel(FDMA_CHAN_SPR_SRC_REG(FIO_DMA_CHAN), spare_buf_addr);
		writel(FDMA_CHAN_SPR_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
		writel(FDMA_CHAN_SPR_CNT_REG(FIO_DMA_CHAN), slen);

		dma_burst_ctrl = DMA_NODC_MN_BURST_SIZE8;
		fio_burst_ctrl = FIO_MN_BURST_SIZE8;
	}

	writel(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_RM		|
	       DMA_CHANX_CTR_NI		|
	       dma_burst_ctrl	|
	       mlen);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl		|
	       (mlen + slen));

	/* Wait for interrupt for NAND operation done and DMA done */
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0 ||
		((readl(FDMA_REG(DMA_INT_OFFSET)) &
			DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0) ||
		(readl(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN)) &
			DMA_CHANX_CTR_EN) ||
		(readl(FIO_DMASTA_REG) & FIO_DMASTA_DN) == 0x0 ||
		(readl(FIO_DMACTR_REG) & FIO_DMACTR_EN));
	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	read_status();
}

/**
 * Read spare area from NAND flash.
 * Always disable ECC.
 */
int nand_read_spare(u32 block, u32 page, u32 pages, const u8 *buf)
{
	int i;
	u32 status;
	u32 addr, size = 0, mlen = 0;
	u32 desc_burst_ctrl = 0, fio_burst_ctrl;

#if defined(DEBUG)
	putstr("nand_read_spare( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_read_spare()");
		return -1;
	}

	for (i = 0; i < pages; i++) {
		mlen += flnand.main_size;
		size += flnand.spare_size;
	}

	if (flnand.ecc_bits > 1) {
		if (mlen > MAX_SPARE_SIZE_BLK) {
			putstr("ERR: too many pages at one time\n");
		}
		/* Always disable ECC */
		/* Setup DMA main descriptor */
		G_fio_dmadesc.src_addr = FIO_FIFO_BASE;
		G_fio_dmadesc.dst_addr = (u32) dummy_buffer_bch;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (u32) &G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = mlen;

		/* Setup DMA spare descriptor */
		G_fio_dma_spr_desc.src_addr = FIO_FIFO_BASE;
		G_fio_dma_spr_desc.dst_addr = (u32) buf;
		G_fio_dma_spr_desc.next= 0x0;
		G_fio_dma_spr_desc.rpt_addr = (u32) &G_fio_dma_spr_desc.rpt;
		G_fio_dma_spr_desc.xfrcnt = size;
		G_fio_dma_spr_desc.rpt = 0x0;

		desc_burst_ctrl = DMA_DESC_MN_BURST_SIZE;
		fio_burst_ctrl 	= FIO_MN_BURST_SIZE;
	} else {
		/* Setup DMA descriptor */
		G_fio_dmadesc.src_addr = FIO_FIFO_BASE;
		G_fio_dmadesc.dst_addr = (u32) buf;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (u32) &G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = size;

		desc_burst_ctrl = DMA_DESC_SP_BURST_SIZE;
		fio_burst_ctrl 	= FIO_SP_BURST_SIZE;
		mlen = 0;
	}

	G_fio_dmadesc.ctrl =
		DMA_DESC_WM |
		DMA_DESC_EOC |
		DMA_DESC_NI |
		DMA_DESC_IE |
		DMA_DESC_ST |
		desc_burst_ctrl;
	G_fio_dmadesc.rpt = 0x0;

	_clean_flush_d_cache();

	if (flnand.ecc_bits > 1) {
		nand_dis_bch();

		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS | FIO_CTR_XD);
		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE  | NAND_CTR_SA);
	}

	/* Setup external DMA engine transfer */
	writel(FDMA_CHAN_DA_REG(FIO_DMA_CHAN), (u32) &G_fio_dmadesc);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);

	if (flnand.ecc_bits > 1) {
		/* Setup spare external DMA engine descriptor address */
		writel(FDMA_CHAN_SPR_DA_REG(FIO_DMA_CHAN),
							(u32) &G_fio_dma_spr_desc);
		writel(FDMA_CHAN_SPR_STA_REG(FIO_DMA_CHAN), 0);
	}

	writel(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_D |
	       DMA_CHANX_CTR_EN);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr = addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_FL		|
	       fio_burst_ctrl		|
	       (mlen + size));

	/* Wait for interrupt for NAND operation done and DMA done */
	do {
		_clean_flush_d_cache();
	} while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0	||
		 readl(FIO_DMASTA_REG) == 0x0			||
		 (G_fio_dmadesc.rpt & DMA_CHANX_STA_DN) == 0x0	||
		 ((readl(FDMA_REG(DMA_INT_OFFSET)) &
		 	DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0));

	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	read_status();
}

/**
 * Program spare area to NAND flash.
 * Only for mark bad block, disable ECC.
 */
int nand_prog_spare(u32 block, u32 page, u32 pages, const u8 *buf)
{
	int i;
	u32 status;
	u32 addr, size = 0, mlen = 0;

#if defined(DEBUG)
	putstr("nand_prog_spare( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_spare()");
		return -1;
	}

	for (i = 0; i < pages; i++) {
		mlen += flnand.main_size;
		size += flnand.spare_size;
	}

	if (flnand.ecc_bits > 1) {
		if (mlen > MAX_SPARE_SIZE_BLK) {
			putstr("ERR: too many pages at one time\n");
		} else {
			memset(dummy_buffer_bch, 0xff, mlen);
		}

		/* Always disable ECC */
		/* Setup DMA main descriptor */
		G_fio_dmadesc.src_addr = (u32) dummy_buffer_bch;
		G_fio_dmadesc.dst_addr = FIO_FIFO_BASE;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (u32) &G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = mlen;

		/* Setup DMA spare descriptor */
		G_fio_dma_spr_desc.src_addr = (u32) buf;
		G_fio_dma_spr_desc.dst_addr = FIO_FIFO_BASE;
		G_fio_dma_spr_desc.next= 0x0;
		G_fio_dma_spr_desc.rpt_addr = (u32) &G_fio_dma_spr_desc.rpt;
		G_fio_dma_spr_desc.xfrcnt = size;
		G_fio_dma_spr_desc.rpt = 0x0;
	} else {
		/* Setup DMA descriptor */
		G_fio_dmadesc.src_addr = (u32) buf;
		G_fio_dmadesc.dst_addr = FIO_FIFO_BASE;
		G_fio_dmadesc.next= 0x0;
		G_fio_dmadesc.rpt_addr = (u32) &G_fio_dmadesc.rpt;
		G_fio_dmadesc.xfrcnt = size;
		mlen = 0;
	}

	G_fio_dmadesc.ctrl =
		DMA_DESC_RM	|
		DMA_DESC_EOC	|
		DMA_DESC_NI	|
		DMA_DESC_IE	|
		DMA_DESC_ST	|
		DMA_DESC_SP_BURST_SIZE;
	G_fio_dmadesc.rpt = 0x0;

	_clean_d_cache();

	/* Diable write protect */
	//gpio_set(FL_WP);

	if (flnand.ecc_bits > 1) {
		nand_dis_bch();

		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS);
		/* Setup Flash Control Register*/
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE);
		/* Clean Flash_IO_ecc_rpt_status Register */
		writel(FIO_ECC_RPT_STA_REG, 0x0);
	} else {
		/* Setup Flash IO Control Register */
		writel(FIO_CTR_REG, FIO_CTR_RS);
		/* Setup Flash Control Register */
		writel(NAND_CTR_REG, flnand.control | NAND_CTR_SE  | NAND_CTR_SA);
	}

	/* Setup main external DMA engine descriptor address */
	writel(FDMA_CHAN_DA_REG(FIO_DMA_CHAN), (u32) &G_fio_dmadesc);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0);

	if (flnand.ecc_bits > 1) {
		writel(FDMA_CHAN_SPR_STA_REG(FIO_DMA_CHAN), 0);
		/* Setup spare external DMA engine descriptor address */
		writel(FDMA_CHAN_SPR_DA_REG(FIO_DMA_CHAN),
							(u32) &G_fio_dma_spr_desc);
	}
	writel(FDMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_D |
	       DMA_CHANX_CTR_EN);

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	addr =  addr_from_block_page(block, page);
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       FIO_SP_BURST_SIZE	|
	       (mlen + size));

	/* Wait for interrupt for NAND operation done and DMA done */
	do {
		_clean_flush_d_cache();
	} while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0	||
		 readl(FIO_DMASTA_REG) == 0x0			||
		 (G_fio_dmadesc.rpt & DMA_CHANX_STA_DN) == 0x0	||
		 ((readl(FDMA_REG(DMA_INT_OFFSET)) &
		 	DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0));

	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);
	writel(FDMA_REG(DMA_INT_OFFSET), 0x0);
	writel(FDMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	read_status();
}

/**
 * Erase a NAND flash block.
 */
int nand_erase_block(u32 block)
{
	u32 status;
	u32 addr;

#if defined(DEBUG)
	putstr("nand_erase_block( ");
	putdec(block);
	putstr(" )\r\n");
#endif

	/* Disable write protect */
	//gpio_set(FL_WP);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NAND_CTR_REG, flnand.control);

	/* Erase block */
	addr = addr_from_block_page(block, 0);

#ifdef ARCH_SUPPORT_BCH_ID5
	/* Workround for DSM bug!*/
	writel(FIO_DMAADR_REG, addr);
#endif

	writel(NAND_INT_REG, 0x0);
	writel(NAND_CMD_REG, NAND_CMD_ERASE | addr);
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);

	status = readl(FIO_DMASTA_REG);

	writel(NAND_INT_REG, 0x0);
	writel(FIO_DMASTA_REG, 0x0);

	/* Enable write protect */
	//gpio_clr(FL_WP);

	if (status & (FIO_DMASTA_RE | FIO_DMASTA_AE))
		return -1;

	/* Read Status */
	writel(NAND_CMD_REG, NAND_CMD_READSTATUS);
	while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);

	writel(NAND_INT_REG, 0x0);
	status = readl(NAND_STA_REG);

	if ((status & 0x1)) {
		/* Reset chip */
		writel(NAND_CMD_REG, NAND_CMD_RESET);
		while ((readl(NAND_INT_REG) & NAND_INT_DI) == 0x0);
		writel(NAND_INT_REG, 0x0);
		return -1;
	} else {
#if defined(CONFIG_NAND_USE_FLASH_BBT)
		/* If erase success and block is marked as bad in BBT */
		/* then update BBT. */
		if (nand_is_bad_block(block))
			nand_update_bbt(0, block);
#endif
		return 0;
	}
}

