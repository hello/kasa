/**
 * fio/firmfl.h
 *
 * Embedded Flash and Firmware Programming Utilities
 *
 * History:
 *    2005/01/25 - [Charles Chiou] created file
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


#ifndef __FIRMFL_H__
#define __FIRMFL_H__

#include <fio/partition.h>

/**
 * @defgroup firmfl Embedded Flash and Firmware Programming Utilities
 *
 * This module exports a set of utilities for programming embedded flash
 * memory area. (This does not include managerment of a file system on
 * NAND flash memory beyond the device code + data area).
 *
 * Definitions:
 * <pre>
 *
 * Boot Strapper:
 *	Code that resides in the first 2K area that is loaded and executed
 *	by the hardware. This code should contain minimal hardware
 *	initialization routines and be responsible for loading a 2nd stage
 *	code bootloader that contain more utilities.
 * Boot Loader:
 *	Code that executes after it has been loaded and placed in DRAM by the
 *	first stage boot strapper. This code should contain:
 *		- Loading firmware code, ramdisk, etc. to DRAM and execute it
 *		- Partition management utilities
 *		- Firmware programming utilities
 *		- Terminal (RS-232) interactive shell
 *		- Serial or USB download server
 *		- Diagnostic tools
 * Partition Table:
 *	A fixed region in the the flash memory that contain meta data on the
 *	location, size, and other information of 'partitions' stored in other
 *	parts of the flash area.
 * Primary Firmware:
 *	The system software code that is loaded and executed by the boot
 *	loader at system startup.
 * Backup Firmware:
 *	A valid firmware code that serves as a backup copy to the primary
 *	firmware.
 * Ramdisk:
 *	A contiguous block of data to be copied from the flash memory to
 *	the RAM area, which may later be intepreted by the operating system
 *	to be a RAM-resident file-system.
 * ROMFS:
 *	A simple read-only 'file system'.
 * Embedded Media FS:
 *	This region is not managed and known by the boot loader. It should
 *	be managed by the file system and block device driver.
 *
 * Embedded Flash Memory Layout:
 *
 *           +----------------------------+     -----
 *           | Boot Strapper              |       ^
 *           +----------------------------+       |
 *           | Boot Loader                |       |
 *           +----------------------------+       |
 *           | Partition Table            |       |
 *           +----------------------------+       |
 *           | Splash                     |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Persistent BIOS App        |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Primary Firmware           |       |
 *           |                            |       |
 *           +----------------------------+       | Read-Only
 *           |                            |       | Simple Raw Flash
 *           | Secondary Firmware         |       | Format
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Backup Firmware            |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Ramdisk                    |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | ROMFS                      |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | DSP uCode                  |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | LINUX                  	  |       |
 *           |                            |       v
 *           +----------------------------+     -----
 *           |                            |       ^
 *           | Swap partition in linux 	  |       |
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       | General purpose partitions.
 *           | Android data partition     |       | (without pre-built image)
 *           |                            |       |
 *           +----------------------------+       |
 *           |                            |       |
 *           | Android cache partition    |       |
 *           |                            |       v
 *           +----------------------------+     -----
 *           |                            |       ^
 *           | Raw Data                   |       |
 *           |                            |       v
 *           +----------------------------+     -----
 *
 * </pre>
 * Note: The boot strapper is fixed in address and length
 * and cannot be changed.
 */

/*===========================================================================*/
/* Error codes	 							     */
/*===========================================================================*/
#define FLPROG_OK		 0
#define FLPROG_ERR_MAGIC	-1
#define FLPROG_ERR_LENGTH	-2
#define FLPROG_ERR_CRC		-3
#define FLPROG_ERR_VER_NUM	-4
#define FLPROG_ERR_VER_DATE	-5
#define FLPROG_ERR_PROG_IMG	-6
#define FLPROG_ERR_PTB_GET	-7
#define FLPROG_ERR_PTB_SET	-8
#define FLPROG_ERR_FIRM_FILE	-9
#define FLPROG_ERR_FIRM_FLAG	-10
#define FLPROG_ERR_NO_MEM	-11
#define FLPROG_ERR_FIFO_OPEN	-12
#define FLPROG_ERR_FIFO_READ	-13
#define FLPROG_ERR_PAYLOAD	-14
#define FLPROG_ERR_ILLEGAL_HDR	-15
#define FLPROG_ERR_EXTRAS_MAGIC	-16
#define FLPROG_ERR_PREPROCESS	-17
#define FLPROG_ERR_POSTPROCESS	-18
#define FLPROG_ERR_META_SET	-19
#define FLPROG_ERR_META_GET	-20

#define FLPROG_ERR_FIRM_HW_SIGN_VERIFY_FAIL	-21
#define FLPROG_ERR_FIRM_HW_SIGN_FAIL -22


/*===========================================================================*/
/* Partition flag 							     */
/*===========================================================================*/
#define PART_LOAD		0x00		/**< Load partition data */
#define PART_NO_LOAD		0x01		/**< Don't load part data */
#define PART_READONLY		0x02		/**< Data is RO */

/*===========================================================================*/
#define FLPART_MAGIC		0x8732DFE6
#define PARTHD_MAGIC		0xA324EB90

#define FIRMFL_FORCE		0x00	/**< No limit */
#define FIRMFL_VER_EQ		0x01	/**< Version is eqral */
#define FIRMFL_VER_GT		0x02	/**< Version is greater */
#define FIRMFL_DATE_EQ		0x04	/**< Date is equal */
#define FIRMFL_DATE_GT		0x08	/**< Date is greater */

#define DSPFW_HEADER_BYTES	0x50
#define FW_MODEL_NAME_SIZE	128

/*===========================================================================*/
#ifndef __ASM__

/**
 * The PTB partition is a region in flash where image info, meta data
 * and DTB are stored.
 *
 * PTB Layout:
 *
 *           +--------------------+
 *           | PTB Header         |
 *           +--------------------+
 *           | flpart_table_t     |
 *           +--------------------+
 *           | flpart_meta_t      |
 *           +--------------------+
 *           | DTB                |
 *           +--------------------+
 *
 */

#define PTB_HEADER_SIZE		64
#define PTB_HEADER_PAD_SIZE	(PTB_HEADER_SIZE - sizeof(u32) * 4)
#define PTB_HEADER_MAGIC	0x524f4e47
#define PTB_HEADER_VERSION	0x20140417

typedef struct ptb_header_s
{
	u32	magic;
	u32	crc32;
	u32	version;
	u32	dev;
	u8	rsv[PTB_HEADER_PAD_SIZE];
} __attribute__((packed)) ptb_header_t;


/**
 * Flash partition entry. This structure describes a partition allocated
 * on the embedded flash device for either:
 * <pre>
 *	- bootloader
 *	- primary firmware image
 *	- backup firmware image
 *	- ramdisk image
 *	- romfs image
 *	- dsp image
 * </pre>
 * The bootloader, as well as the online flash programming utilities
 * rely on these information to:
 * <pre>
 *	- Validate address boundary, range
 *	- Guard against version conflicts
 *	- Load image to RAM area
 *	- Compute checksum
 * </pre>
 */
typedef struct flpart_s
{
	u32	crc32;		/**< CRC32 checksum of image */
	u32	ver_num;	/**< Version number */
	u32	ver_date;	/**< Version date */
	u32	img_len;	/**< Lengh of image in the partition */
	u32	mem_addr;	/**< Starting address to copy to RAM */
	u32	flag;		/**< Special properties of this partition */
	u32	magic;		/**< Magic number */
} __attribute__((packed)) flpart_t;

#define FLDEV_CMD_LINE_SIZE	1024

/**
 * Properties of the network device
 */
typedef struct netdev_s
{
	/* This section contains networking related settings */
	u8	mac[6];		/**< MAC address*/
	u32	ip;		/**< Boot loader's LAN IP */
	u32	mask;		/**< Boot loader's LAN mask */
	u32	gw;		/**< Boot loader's LAN gateway */
} __attribute__((packed)) netdev_t;

/**
 * Properties of the target device that is stored in the flash.
 */
typedef struct fldev_s
{
	u32	magic;		/**< Magic number */

	char	sn[32];		/**< Serial number */
	u8	usbdl_mode;	/**< USB download mode */
	u8	auto_boot;	/**< Automatic boot */
	u8	rsv[2];
	u32	splash_id;
	char	cmdline[FLDEV_CMD_LINE_SIZE];

	/* This section contains networking related settings */
	netdev_t eth[2];
	netdev_t wifi[2];
	netdev_t usb_eth[2];

	/* This section contains update by network  related settings */
	u32	auto_dl;	/**< Automatic download? */
	u32	tftpd;		/**< Boot loader's TFTP server */
	u32	pri_addr;	/**< RTOS download address */
	char	pri_file[32];	/**< RTOS file name */
	u32	pri_comp;	/**< RTOS compressed? */
	u32	rmd_addr;	/**< Ramdisk download address */
	char	rmd_file[32];	/**< Ramdisk file name */
	u32	rmd_comp;	/**< Ramdisk compressed? */
	u32	dsp_addr;	/**< DSP download address */
	char	dsp_file[32];	/**< DSP file name */
	u32	dsp_comp;	/**< DSP compressed? */

	/* This section contains secure boot related settings */
	u8	secure_boot_init;	/**< Secure boot initialized flag */
	u8	need_generate_firmware_hw_signature;	/**< Secure boot flag, indicate the firmware need generate hw signature*/
	u8	reserved0;
	u8	reserved1;

	u8	rsa_key_n[256 + 4];
	u8	rsa_key_e[16];
	//u8	cryptochip_sn[12];
	u8	sn_signature[128];
} __attribute__((packed)) fldev_t;

/**
 * The partition table is a region in flash where meta data about
 * different partitions are stored.
 */
#define PTB_TABLE_SIZE		4096
#define PTB_PAD_SIZE		(PTB_TABLE_SIZE - \
				 PART_MAX_WITH_RSV * sizeof(flpart_t) \
				 - sizeof(fldev_t))
typedef struct flpart_table_s
{
	flpart_t	part[PART_MAX_WITH_RSV];/** Partitions */
	/* ------------------------------------------ */
	fldev_t		dev;			/**< Device properties */
	u8		rsv[PTB_PAD_SIZE];	/**< Padding to 2048 bytes */
} __attribute__((packed)) flpart_table_t;


/**
 * The meta data table is a region in flash after partition table.
 * The data need by dual boot are stored.
 */
#define PTB_META_MAGIC		0x42405891
#define PART_NAME_LEN		8
#define PTB_META_SIZE		2048
#define PTB_META_PAD_SIZE	(PTB_META_SIZE - sizeof(u32) - \
				(sizeof(u32) * 3 + PART_NAME_LEN) * PART_MAX - \
				FW_MODEL_NAME_SIZE)
typedef struct flpart_meta_s
{
	u32		magic;
	struct {
		u32	sblk;
		u32	nblk;
		u32	dev;
		char	name[PART_NAME_LEN];
	} part_info[PART_MAX];
	u8		model_name[FW_MODEL_NAME_SIZE];
	u8 		rsv[PTB_META_PAD_SIZE];
} __attribute__((packed)) flpart_meta_t;


#define PTB_HEADER(ptb_buf)	((ptb_header_t *)ptb_buf)

#define PTB_TABLE(ptb_buf)	((flpart_table_t *) \
				((u8 *)ptb_buf + sizeof(ptb_header_t)))

#define PTB_META(ptb_buf)	((flpart_meta_t *) \
					((u8 *)ptb_buf + \
					 sizeof(ptb_header_t) + \
					 sizeof(flpart_table_t)))

#define PTB_DTB(ptb_buf)	((void *) \
					((u8 *)ptb_buf + \
					sizeof(ptb_header_t) + \
					sizeof(flpart_table_t)) + \
					sizeof(flpart_meta_t))

/**
 * This is the header for a flash image partition.
 */
typedef struct partimg_header_s
{
	u32	crc32;		/**< CRC32 Checksum */
	u32	ver_num;	/**< Version number */
	u32	ver_date;	/**< Version date */
	u32	img_len;	/**< Image length */
	u32	mem_addr;	/**< Location to be loaded into memory */
	u32	flag;		/**< Flag of partition. */
	u32	magic;		/**< The magic number */
	u32	reserved[57];
} __attribute__((packed)) partimg_header_t;


#if defined(CONFIG_NAND_USE_FLASH_BBT) || defined(CONFIG_SPINAND_USE_FLASH_BBT)
/**
 * struct nand_bbt_descr - bad block table descriptor
 *
 * Descriptor for the bad block table marker and the descriptor for the
 * pattern which identifies good and bad blocks. The assumption is made
 * that the pattern and the version count are always located in the oob area
 * of the first block.
 */
typedef struct nand_bbt_descr_s {
	u32 options;
	int block;
	u8 offs;
	u8 veroffs;
	u8 version;
	u8 len;
	u8 maxblocks;
	u8 *pattern;
} nand_bbt_descr_t;

#define NAND_BBT_LASTBLOCK		0x00000010
#define NAND_BBT_VERSION		0x00000100
#define NAND_BBT_NO_OOB			0x00040000

#endif

#ifdef __BUILD_AMBOOT__

/*
 * The following data structure is used by the memfwprog program to output
 * the flash programming results to a memory area.
 */

#define FWPROG_RESULT_FLAG_LEN_MASK	0x00ffffff
#define FWPROG_RESULT_FLAG_CODE_MASK	0xff000000

#define FWPROG_RESULT_MAKE(code, len) \
	((code) << 24) | ((len) & FWPROG_RESULT_FLAG_LEN_MASK)

typedef struct fwprog_result_s
{
	u32	magic;
#define FWPROG_RESULT_MAGIC	0xb0329ac3

	u32	bad_blk_info;
#define BST_BAD_BLK_OVER	0x00000001
#define BLD_BAD_BLK_OVER	0x00000002
#define SPL_BAD_BLK_OVER	0x00000004
#define PBA_BAD_BLK_OVER	0x00000008
#define PRI_BAD_BLK_OVER	0x00000010
#define SEC_BAD_BLK_OVER	0x00000020
#define BAK_BAD_BLK_OVER	0x00000040
#define RMD_BAD_BLK_OVER	0x00000080
#define ROM_BAD_BLK_OVER	0x00000100
#define DSP_BAD_BLK_OVER	0x00000200

	u32	flag[HAS_IMG_PARTS];
} fwprog_result_t;

#define MEMFWPROG_CMD_VERIFY 0x00007E57
typedef struct fwprog_cmd_s
{
#define FWPROG_MAX_CMD		8
	u32	cmd[FWPROG_MAX_CMD];
	u32	error;
	u32	result_data;
	u32	result_str_len;
	u32	result_str_addr;
	u8	data[MEMFWPROG_HOOKCMD_SIZE - 12 * sizeof(u32)];
} fwprog_cmd_t;

#endif /* __BUILD_AMBOOT__ */

/*===========================================================================*/
/* Private functions for implementors of NAND/NOR host controller            */
/*===========================================================================*/

/**
 * Compile-time info. of NAND.
 */
typedef struct flnand_s
{
	u32	control;		/**< Control parameter */
	u32	timing0;		/**< Timing param. */
	u32	timing1;		/**< Timing param. */
	u32	timing2;		/**< Timing param. */
	u32	timing3;		/**< Timing param. */
	u32	timing4;		/**< Timing param. */
	u32	timing5;		/**< Timing param. */
	u32	nandtiming0;		/**<nand Timing param. */
	u32	nandtiming1;		/**<nand Timing param. */
	u32	nandtiming2;		/**<nand Timing param. */
	u32	nandtiming3;		/**<nand Timing param. */
	u32	nandtiming4;		/**<nand Timing param. */
	u32	nandtiming5;		/**<nand Timing param. */
	u32	banks;			/**< Total banks */
	u32	blocks_per_bank;	/**< blocks_per_bank */
	u32	main_size;		/**< Main size */
	u32	spare_size;		/**< Spare size */
	u32	pages_per_block;	/**< Pages per block */
	u32	block_size;		/**< Block size */
	u32	sblk[PART_MAX]; 	/**< Starting block */
	u32	nblk[PART_MAX]; 	/**< Number of blocks */
	u32	ecc_bits;		/**< ECC bits for new controller. */
} flnand_t;

extern flnand_t flnand;

/**
 * Compile info. of SPI NOR
 */
typedef struct flspinor_s
{
	u32	chip_sel;
	u32	page_size;
	u32	sector_size;
	u32	chip_size;
	u32	sectors_per_chip;
	u32	ssec[TOTAL_FW_PARTS]; 	/**< Starting sectors */
	u32	nsec[TOTAL_FW_PARTS]; 	/**< Number of sectors */
} flspinor_t;

extern flspinor_t flspinor;

/**
 * Compile info. of SPI NAND
 */
typedef struct flspinand_s
{
	u32	chip_sel;
	u32	main_size;		/**< Main size */
	u32	spare_size;		/**< Spare size */
	u32	pages_per_block;	/**< Pages per block */
	u32	block_size;		/**< Block size */
	u32	chip_size;
	u32	sblk[PART_MAX]; 	/**< Starting block */
	u32	nblk[PART_MAX]; 	/**< Number of blocks */
} flspinand_t;

extern flspinand_t flspinand;

typedef struct sdmmc_s {
	u8 verbose;
	u8 slot;
	u8 type;			/* card type: MMC or SD */
	u8 no_1v8;			/* no 1v8 support, i.e., no UHS-I support */
	u8 sector_mode;
	u32 sector_size;
	u32 clk_hz;			/* controller clock divider */
	u32 spec_vers;			/* Card spec version */
	u32 ccc;			/* Card Command Class */
	u32 rca;			/* Relative Card Address */
	u32 ocr;			/* Operation condition Register */
	u32 raw_cid[4];			/* Card CID register */
	u32 raw_csd[4];			/* Card CSD register */
	u32 raw_scr[2];			/* Card SCR register */
	u32 supported_mode;		/* Card supported mode */
	u32 current_mode;
	u32 bus_width;			/* Card supported bus width */
	u64 capacity;			/* Card capacity, unit is bytes: (capacity = sectors * sector_size) */
	u32 ssec[PART_MAX]; 		/**< Starting sectors */
	u32 nsec[PART_MAX];	 	/**< Number of sectors */
} sdmmc_t;

extern sdmmc_t sdmmc;

/*===========================================================================*/
extern int flprog_get_part_table(flpart_table_t *pptb);
extern int flprog_set_part_table(flpart_table_t *pptb);
extern int flprog_get_dtb(u8 *dtb);
extern int flprog_set_dtb(u8 *dtb, u32 len, u32 write);
extern int flprog_write_partition(int pid, u8 *image, unsigned int len);
extern int flprog_erase_partition(int pid);

extern void output_header(partimg_header_t *header, u32 len);
extern void output_failure(int errcode);
extern void output_report(const char *name, u32 flag);
extern void flprog_show_meta(void);

#endif  /* !__ASM__ */

/*@}*/

#endif
