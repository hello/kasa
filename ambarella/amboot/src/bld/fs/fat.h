/**
 * fs/fat.h
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

#ifndef __AMBA_FAT__
#define __AMBA_FAT__

#define VFAT_MAXLEN_BYTES	256 /* Maximum LFN buffer in bytes */
#define VFAT_MAXSEQ		9   /* Up to 9 of 13 2-byte UTF-16 entries */
#define PREFETCH_BLOCKS		2

#define DOS_PART_DISKSIG_OFFSET	0x1b8
#define DOS_PART_TBL_OFFSET		0x1be
#define DOS_PART_MAGIC_OFFSET	0x1fe

#define DOS_BOOT_MAGIC_OFFSET	0x1fe
#define DOS_FS_TYPE_OFFSET		0x36
#define DOS_FS32_TYPE_OFFSET	0x52

#define DEFAULT_BUF_START	bld_hugebuf_addr

#define DIRENTSPERBLOCK	(mydisk.sect_size / sizeof(dir_entry))
#define DIRENTSPERCLUST	((mydisk.clust_size * mydisk.sect_size) / \
			 sizeof(dir_entry))

#define CLUST2SECTOR(v)  (mydisk.data_sector + (v - 2) * mydisk.clust_size)

#define CLUSTSIZE   (mydisk.clust_size * mydisk.sect_size)

#define FATBUFBLOCKS	6
#define FATBUFSIZE	(mydata->sect_size * FATBUFBLOCKS)
#define FAT12BUFSIZE	((FATBUFSIZE*2)/3)
#define FAT16BUFSIZE	(FATBUFSIZE/2)
#define FAT32BUFSIZE	(FATBUFSIZE/4)

/* Filesystem identifiers */
#define FAT12_SIGN	"FAT12   "
#define FAT16_SIGN	"FAT16   "
#define FAT32_SIGN	"FAT32   "
#define SIGNLEN		8

/* File attributes */
#define ATTR_RO		(1 << 0)
#define ATTR_HIDDEN	(1 << 1)
#define ATTR_SYS	(1 << 2)
#define ATTR_VOLUME	(1 << 3)
#define ATTR_DIR	(1 << 4) /* 0x10 */
#define ATTR_ARCH	(1 << 5) /* 0x20 */

#define ATTR_VFAT	(ATTR_RO | ATTR_HIDDEN | ATTR_SYS | ATTR_VOLUME)

#define DELETED_FLAG	((char)0xe5) /* Marks deleted files when in name[0] */
#define aRING		0x05	     /* Used as special character in name[0] */

/*
 * Indicates that the entry is the last long entry in a set of long
 * dir entries
 */
#define LAST_LONG_ENTRY_MASK	0x40

/* Flags telling whether we should read a file or list a directory */
#define LS_NO		0
#define LS_YES		1
#define LS_DIR		1
#define LS_ROOT		2

#define ISDIRDELIM(c)	((c) == '/' || (c) == '\\')

#define FSTYPE_NONE	(-1)

#define FATTBL_END    0x0fffffff
#define FATTBL_BAD    0xfffffff7
#define FATTBL_MAGIC  0x0ffffff8

typedef struct dos_partition {
	unsigned char boot_ind;		/* 0x80 - active			*/
	unsigned char head;			/* starting head			*/
	unsigned char sector;		/* starting sector			*/
	unsigned char cyl;			/* starting cylinder			*/
	unsigned char sys_ind;		/* What partition type			*/
	unsigned char end_head;		/* end head				*/
	unsigned char end_sector;	/* end sector				*/
	unsigned char end_cyl;		/* end cylinder				*/
	unsigned char start4[4];	/* starting sector counting from 0	*/
	unsigned char size4[4];		/* nr of sectors in partition		*/
} dos_partition_t;


struct disk_info{
	int boot_sector;
	int total_sector;
	int data_sector;

	u32	tbl_addr;	/* FAT TBL add in sectors */
	u32	fatlength;	/* Length of FAT in sectors */
	u16	fat_sect;	/* Starting sector of the FAT */
	u16	sect_size;	/* Size of sectors in bytes */
	u16	clust_size;	/* Size of clusters in sectors */
};

struct file
{
	char name[256];
	u8 	 attr;
	u32  cluster;     /* start sector */
	u32  size;
};

typedef struct boot_sector {
	u8	ignored[3];	/* Bootstrap code */
	char	system_id[8];	/* Name of fs */
	u8	sector_size[2];	/* Bytes/sector */
	u8	cluster_size;	/* Sectors/cluster */
	u16	reserved;	/* Number of reserved sectors */
	u8	fats;		/* Number of FATs */
	u8	dir_entries[2];	/* Number of root directory entries */
	u8	sectors[2];	/* Number of sectors */
	u8	media;		/* Media code */
	u16	fat_length;	/* Sectors/FAT */
	u16	secs_track;	/* Sectors/track */
	u16	heads;		/* Number of heads */
	u32	hidden;		/* Number of hidden sectors */
	u32	total_sect;	/* Number of sectors (if sectors == 0) */

	/* FAT32 only */
	u32	fat32_length;	/* Sectors/FAT */
	u16	flags;		/* Bit 8: fat mirroring, low 4: active fat */
	u8	version[2];	/* Filesystem version */
	u32	root_cluster;	/* First cluster in root directory */
	u16	info_sector;	/* Filesystem info sector */
	u16	backup_boot;	/* Backup boot sector */
	u16	reserved2[6];	/* Unused */
} boot_sector;

typedef struct volume_info
{
	u8 drive_number;	/* BIOS drive number */
	u8 reserved;		/* Unused */
	u8 ext_boot_sign;	/* 0x29 if fields below exist (DOS 3.3+) */
	u8 volume_id[4];	/* Volume ID number */
	char volume_label[11];	/* Volume label */
	char fs_type[8];	/* Typically FAT12, FAT16, or FAT32 */
	/* Boot code comes next, all but 2 bytes to fill up sector */
	/* Boot sign comes last, 2 bytes */
} volume_info;

#define DEFAULT_LCASE      0x10
#define DEFAULT_CTIME_MS   0xA3
#define DEFAULT_CTIME      0x5F0A
#define DEFAULT_CDATE      0x4698
#define DEFAULT_ADATE      0x4698
#define DEFAULT_TIME       0x5F0B
#define DEFAULT_DATE       0x4698

typedef struct dir_entry {
	char	name[8],ext[3];	/* Name and extension */
	u8	attr;		/* Attribute bits */
	u8	lcase;		/* Case for base and extension */
	u8	ctime_ms;	/* Creation time, milliseconds */
	u16	ctime;		/* Creation time */
	u16	cdate;		/* Creation date */
	u16	adate;		/* Last access date */
	u16	starthi;	/* High 16 bits of cluster in FAT32 */
	u16	time,date,start;/* Time, date and first cluster */
	u32	size;		/* File size in bytes */
} dir_entry;

typedef struct {
	u8	*fatbuf;	/* Current FAT buffer */
	int	fatsize;	/* Size of FAT in bits */
	u32	fatlength;	/* Length of FAT in sectors */
	u16	fat_sect;	/* Starting sector of the FAT */
	u32	rootdir_sect;	/* Start sector of root directory */
	u16	sect_size;	/* Size of sectors in bytes */
	u16	clust_size;	/* Size of clusters in sectors */
	int	data_begin;	/* The sector of the first cluster, can be negative */
	int	fatbufnum;	/* Used by get_fatent, init to -1 */
} fsdata;

typedef struct dir_slot {
	u8	id;		/* Sequence number for slot */
	u8	name0_4[10];	/* First 5 characters in name */
	u8	attr;		/* Attribute byte */
	u8	reserved;	/* Unused */
	u8	alias_checksum;/* Checksum for 8.3 alias */
	u8	name5_10[12];	/* 6 more characters in name */
	u16	start;		/* Unused */
	u8	name11_12[4];	/* Last 2 characters in name */
} dir_slot;


int file_fat_ls(const char *dir);
int file_fat_chdir(const char *dir);
int file_fat_info(void);
int file_fat_read(const char *filename, u32 addr, int exec);
int file_fat_write(const char *filename, u32 addr, u32 size);
#endif  /*__AMBA_FAT__*/
