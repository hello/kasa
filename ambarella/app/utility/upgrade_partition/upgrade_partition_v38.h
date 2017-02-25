/**
 * upgrade_partition_v38.h
 *
 * History:
 *    2014/05/27 - [Ken He] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#ifndef __UPGRADE_PARTITION_H__
#define __UPGRADE_PARTITION_H__

/*========================== Header Files ====================================*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/string.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif
#include <ctype.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <basetypes.h>
#include <mtd/mtd-user.h>
#include <linux/jffs2.h>
/*===========================================================================*/
#define MAX_PAGE_SIZE	2048
#define MAX_OOB_SIZE	64

#define FLPART_MAGIC	0x8732dfe6
#define __ARMCC_PACK__
#define __ATTRIB_PACK__  __attribute__ ((packed))
#define ETH_INSTANCES		2
#define USE_WIFI		1
#define CMD_LINE_SIZE		512
#define MAC_SIZE		6
#define SN_SIZE			32

typedef __ARMCC_PACK__ struct flpart_s
{
	uint32_t	crc32;		/**< CRC32 checksum of image */
	uint32_t	ver_num;	/**< Version number */
	uint32_t	ver_date;	/**< Version date */
	uint32_t	img_len;	/**< Lengh of image in the partition */
	uint32_t	mem_addr;	/**< Starting address to copy to RAM */
	uint32_t	flag;		/**< Special properties of this partition */
	uint32_t	magic;		/**< Magic number */
} __ATTRIB_PACK__ flpart_t;

/**
 * Partitions on device.
 */
/* ------------------------------------------------------------------------- */
/* Below are firmware partitions. (with pre-built image) */
#define PART_BST		(0x00)
#define PART_PTB		(0x01)
#define PART_BLD		(0x02)
#define PART_HAL		(0x03)
#define PART_PBA		(0x04)
#define PART_PRI		(0x05)
#define PART_SEC		(0x06)
#define PART_BAK		(0x07)
#define PART_RMD		(0x08)
#define PART_ROM		(0x09)
#define PART_DSP		(0x0A)
#define PART_LNX		(0x0B)
/* ------------------------------------------------------------------------- */
/* Below are general purpose partitions. (without pre-built image) */
#define PART_SWP		(0x0C)
#define PART_ADD		(0x0D)
#define PART_ADC		(0x0E)
/* ------------------------------------------------------------------------- */
/* Below are media(nftl) partitions. (without pre-built image) */
#define PART_RAW		(0x0F)
#define PART_STG2		(0x10)
#define PART_STG		(0x11)
#define PART_PRF		(0x12)
#define PART_CAL		(0x13)

#define PART_MAX		20

#define FLDEV_CMD_LINE_SIZE	1024

/**
 * Properties of the network device
 */
typedef __ARMCC_PACK__ struct netdev_s
{
	/* This section contains networking related settings */
	uint8_t	mac[6];		/**< MAC address*/
	uint32_t	ip;		/**< Boot loader's LAN IP */
	uint32_t	mask;		/**< Boot loader's LAN mask */
	uint32_t	gw;		/**< Boot loader's LAN gateway */
} __ATTRIB_PACK__ netdev_t;

/**
 * Properties of the target device that is stored in the flash.
 */
typedef __ARMCC_PACK__ struct fldev_s
{
	char	sn[32];			/**< Serial number */
	uint8_t	usbdl_mode;		/**< USB download mode */
	uint8_t	auto_boot;		/**< Automatic boot */
	char	cmdline[FLDEV_CMD_LINE_SIZE];	/**< Boot command line options */
	uint8_t	rsv[2];
	uint32_t splash_id;
	/* This section contains networking related settings */
	netdev_t eth[2];
	netdev_t wifi[2];		/* Updating the fldev_t struct */
	netdev_t usb_eth[2];

	/* This section contains update by network  related settings */
	uint8_t	auto_dl;	/**< Automatic download? */
	uint32_t	tftpd;		/**< Boot loader's TFTP server */
	uint32_t	pri_addr;	/**< RTOS download address */
	char	pri_file[32];	/**< RTOS file name */
	uint8_t	pri_comp;	/**< RTOS compressed? */
	uint32_t	rmd_addr;	/**< Ramdisk download address */
	char	rmd_file[32];	/**< Ramdisk file name */
	uint8_t	rmd_comp;	/**< Ramdisk compressed? */
	uint32_t	dsp_addr;	/**< DSP download address */
	char	dsp_file[32];	/**< DSP file name */
	uint8_t	dsp_comp;	/**< DSP compressed? */
	uint8_t	rsv2[2];
	uint32_t	magic;		/**< Magic number */
} __ATTRIB_PACK__ fldev_t;

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
 *
 */

/**
 * The partition table is a region in flash where meta data about
 * different partitions are stored.
 */

#define PART_MAX_WITH_RSV	32
#define PTB_SIZE		4096
#define PTB_PAD_SIZE		\
	(PTB_SIZE - PART_MAX_WITH_RSV * sizeof(flpart_t) - sizeof(fldev_t))
typedef __ARMCC_PACK__ struct flpart_table_s
{
	flpart_t	part[PART_MAX_WITH_RSV];/** Partitions */
	/* ------------------------------------------ */
	fldev_t		dev;			/**< Device properties */
	uint8_t		rsv[PTB_PAD_SIZE];	/**< Padding to 2048 bytes */
} __ATTRIB_PACK__ flpart_table_t;

/**
 * The meta data table is a region in flash after partition table.
 * The data need by dual boot are stored.
 */
#define PTB_META_MAGIC		0x4432a0ce
#define PART_NAME_LEN		8
#define PTB_META_SIZE		2048
#define FW_MODEL_NAME_SIZE	128

#define PTB_META_PAD_SIZE	(PTB_META_SIZE - sizeof(uint32_t)*6 - \
				(sizeof(uint32_t) * 3 + PART_NAME_LEN) * PART_MAX - \
				FW_MODEL_NAME_SIZE)

typedef __ARMCC_PACK__ struct flpart_meta_s
{
	struct {
		u32	sblk;
		u32	nblk;
		char	name[PART_NAME_LEN];
	} part_info[PART_MAX];
	u32	magic;				/**< Magic number */
	u32	part_dev[PART_MAX];
	u8	model_name[FW_MODEL_NAME_SIZE];
	struct {
		u32	sblk;
		u32	nblk;
	} sm_stg[2];
	u8 	rsv[PTB_META_PAD_SIZE];
	/* This meta crc32 doesn't include itself. */
	/* It's only calc data before this field.  */
	u32 	crc32;
} __ATTRIB_PACK__ flpart_meta_t;

/* conversion between little-endian and big-endian */
#define uswap_32(x) \
	((((x) & 0xff000000) >> 24) | \
	 (((x) & 0x00ff0000) >> 8) | \
	 (((x) & 0x0000ff00) << 8) | \
	 (((x) & 0x000000ff) << 24))

#define be32_to_cpu(x)		uswap_32(x)

/* mtd struct for image */
typedef struct nand_update_file_header_s {
	unsigned char	magic_number[8];           /*   AMBUPGD'\0'     8 chars including '\0' */
	unsigned short	header_ver_major;
	unsigned short	header_ver_minor;
	unsigned int	header_size;               /* payload starts at header_start_address + header_size */
	unsigned int  payload_type;                /* NAND_UPGRADE_FILE_TYPE_xxx */
	unsigned char	payload_description[256];  /* payload description string, end with '\0' */
	unsigned int  payload_size;                /* payload of upgrade file, after header */
	unsigned int  payload_crc32;               /* payload crc32 checksum, crc calculation from
	                                              header_start_address  + header_size,
	                                              crc calculation size is payload_size */
}nand_update_file_header_t;

typedef struct nand_update_global_s {
	/* Buffer array used for writing data */
	unsigned char writebuf[MAX_PAGE_SIZE];
	unsigned char oobbuf[MAX_OOB_SIZE];
	unsigned char oobreadbuf[MAX_OOB_SIZE];
	/* oob layouts to pass into the kernel as default */
	struct nand_oobinfo none_oobinfo;
	struct nand_oobinfo jffs2_oobinfo;
	struct nand_oobinfo yaffs_oobinfo;
	struct nand_oobinfo autoplace_oobinfo;
} nand_update_global_t;
#endif
