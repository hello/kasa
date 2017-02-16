/*
 * nandwrite and nanddump ported to busybox from mtd-utils
 *
 * Author: Baruch Siach <baruch@tkos.co.il>, Orex Computed Radiography
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 *
 * TODO: add support for large (>4GB) MTD devices
 */

//config:config NANDWRITE
//config:	bool "nandwrite"
//config:	default y
//config:	select PLATFORM_LINUX
//config:	help
//config:	  Write to the specified MTD device, with bad blocks awareness

//applet:IF_NANDWRITE(APPLET(nandwrite, BB_DIR_USR_SBIN, BB_SUID_DROP))
//kbuild:lib-$(CONFIG_NANDWRITE) += nandwrite.o

//usage:#define nandwrite_trivial_usage
//usage:	"[OPTIONS] MTD-device"
//usage:#define nandwrite_full_usage "\n\n"
//usage:	"Write to the specified MTD device\n"
//usage:     "\n	-j, --jffs2             force jffs2 oob layout (legacy support)"
//usage:     "\n	-y, --yaffs             force yaffs oob layout (legacy support)"
//usage:     "\n	-f, --forcelegacy       force legacy support on autoplacement enabled mtd device"
//usage:     "\n	-o, --oob               image contains oob data"
//usage:     "\n	-s addr, --start=addr   set start address (default is 0)"
//usage:     "\n	-p, --pad               pad to page size"
//usage:     "\n	-b, --blockalign=1|2|4  set multiple of eraseblocks to align to"
//usage:     "\n	-q, --quiet             don't display progress messages"
//usage:     "\n	    --help              display this help and exit"
//usage:     "\n	    --version           output version information and exit"
//usage:     "\n	-G, --bld               Update Ambarella AMBOOT BLD Partition"
//usage:     "\n	-H, --hal               Update Ambarella AMBOOT HAL Partition"
//usage:     "\n	-Q, --pba               Update Ambarella AMBOOT PBA Partition"
//usage:     "\n	-I, --sec               Update Ambarella AMBOOT SEC Partition"
//usage:     "\n	-U, --dsp               Update Ambarella AMBOOT DSP Partition"
//usage:     "\n	-W, --lnx               Update Ambarella AMBOOT LNX Partition"
//usage:     "\n	-X, --swp               Update Ambarella AMBOOT SWP Partition"
//usage:     "\n	-Y, --add               Update Ambarella AMBOOT ADD Partition"
//usage:     "\n	-Z, --adc               Update Ambarella AMBOOT ADC Partition"
//usage:     "\n	-K, --pri               Update Ambarella AMBOOT PRI Partition"
//usage:     "\n	-M, --rmd               Update Ambarella AMBOOT RMD Partition"
//usage:     "\n	-R, --rom               Update Ambarella AMBOOT ROM Partition"
//usage:     "\n	-B, --bak               Update Ambarella AMBOOT BAK Partition"
//usage:     "\n	-C, --cmd               Update Ambarella AMBOOT CMD line"
//usage:     "\n	-F hex, --flag=hex      Update Ambarella AMBOOT Partition load flag"
//usage:     "\n	-S hex, --safe=hex      force AMBOOT boot form PTB, safe recovery"
//usage:     "\n	-L hex, --load=hex      Update Ambarella AMBOOT Partition load address"
//usage:     "\n	-D hex, --date=hex      Update Ambarella AMBOOT Partition date"
//usage:     "\n	-V hex, --ver=hex       Update Ambarella AMBOOT Partition version"
//usage:     "\n	-E MAC, --ethmac=MAC    Update Ambarella AMBOOT MAC"
//usage:     "\n	-N sn, --sn=sn  Update Ambarella AMBOOT SN"

#include "libbb.h"
#include <mtd/mtd-user.h>
#include <linux/jffs2.h>
#include <getopt.h>

#define PROGRAM "nandwrite"
#define VERSION "$Revision: 1.32 $"
#define AMB_VERSION "Ambarella: 0-20110318 $"

#define MAX_PAGE_SIZE	2048
#define MAX_OOB_SIZE	64

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

struct globals {
	/* Buffer array used for writing data */
	unsigned char writebuf[MAX_PAGE_SIZE];
	unsigned char oobbuf[MAX_OOB_SIZE];
	unsigned char oobreadbuf[MAX_OOB_SIZE];
	/* oob layouts to pass into the kernel as default */
	struct nand_oobinfo none_oobinfo;
	struct nand_oobinfo jffs2_oobinfo;
	struct nand_oobinfo yaffs_oobinfo;
	struct nand_oobinfo autoplace_oobinfo;
};

#define G (*ptr_to_globals)
#define INIT_G() do { \
	SET_PTR_TO_GLOBALS(xzalloc(sizeof(G))); \
\
	G.none_oobinfo.useecc = MTD_NANDECC_OFF; \
	G.autoplace_oobinfo.useecc = MTD_NANDECC_AUTOPLACE; \
\
	G.jffs2_oobinfo.useecc = MTD_NANDECC_PLACE; \
	G.jffs2_oobinfo.eccbytes = 6; \
	G.jffs2_oobinfo.eccpos[0] = 0; \
	G.jffs2_oobinfo.eccpos[1] = 1; \
	G.jffs2_oobinfo.eccpos[2] = 2; \
	G.jffs2_oobinfo.eccpos[3] = 3; \
	G.jffs2_oobinfo.eccpos[4] = 6; \
	G.jffs2_oobinfo.eccpos[5] = 7; \
\
	G.yaffs_oobinfo.useecc = MTD_NANDECC_PLACE; \
	G.yaffs_oobinfo.eccbytes = 6; \
	G.yaffs_oobinfo.eccpos[0] = 8; \
	G.yaffs_oobinfo.eccpos[1] = 9; \
	G.yaffs_oobinfo.eccpos[2] = 10; \
	G.yaffs_oobinfo.eccpos[3] = 13; \
	G.yaffs_oobinfo.eccpos[4] = 14; \
	G.yaffs_oobinfo.eccpos[5] = 15; \
} while (0)

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
#define PART_BST	0
#define PART_PTB	1
#define PART_BLD	2
#define PART_HAL	3
#define PART_PBA	4
#define PART_PRI  	5
#define PART_SEC	6
#define PART_BAK	7
#define PART_RMD	8
#define PART_ROM	9
#define PART_DSP	10
#define PART_LNX	11
/* ------------------------------------------------------------------------- */
/* Below are general purpose partitions. (without pre-built image) */
#define PART_SWP	12
#define PART_ADD	13
#define PART_ADC	14
/* ------------------------------------------------------------------------- */
/* Below are media(nftl) partitions. (without pre-built image) */
#define PART_RAW	15
#define PART_STG2	16
#define PART_STG	17
#define PART_PRF	18
#define PART_CAL	19

const char *g_part_str[] = {"bst", "ptb", "bld", "hal", "pba",
"pri", "sec", "bak", "rmd", "rom",
"dsp", "lnx", "swp", "add", "adc",
"raw", "stg2", "stg", "prf", "cal",
"all"};

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
	char	sn[32];		/**< Serial number */
	uint8_t	usbdl_mode;	/**< USB download mode */
	uint8_t	auto_boot;	/**< Automatic boot */
	char	cmdline[FLDEV_CMD_LINE_SIZE];	/**< Boot command line options */
	uint8_t	pba_boot_flag[2];//uint8_t	rsv[2];
	                                        //NOTE: just modify this part for clear semantics, the structure size remains.
	                                        //pba_boot_flag[0] -> BIOS boot flag
	                                        //pba_boot_flag[1] -> cloud camera upgrade stage flag
	uint32_t	splash_id;

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

static const uint32_t *crc32_table;

/* Return a 32-bit CRC of the contents of the buffer. */
static inline uint32_t crc32(uint32_t val, const void *ss, int len)
{
	const unsigned char *s = ss;
	while (--len >= 0)
		val = crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);
	return val;
}

static void display_version (void)
{
	printf(PROGRAM " " VERSION "" AMB_VERSION "\n"
			"\n"
			"Copyright (C) 2003 Thomas Gleixner \n"
			"\n"
			PROGRAM " comes with NO WARRANTY\n"
			"to the extent permitted by law.\n"
			"\n"
			"You may redistribute copies of " PROGRAM "\n"
			"under the terms of the GNU General Public Licence.\n"
			"See the file `COPYING' for more information.\n");
	exit(0);
}

static void display_flpart (const char *name, flpart_t *p_flpart)
{
	printf("%s:\n", name);
	printf("crc32 \t= 0x%08x\n", p_flpart->crc32);
	printf("ver_num = 0x%08x\n", p_flpart->ver_num);
	printf("ver_date= 0x%08x\n", p_flpart->ver_date);
	printf("img_len = 0x%08x\n", p_flpart->img_len);
	printf("mem_addr= 0x%08x\n", p_flpart->mem_addr);
	printf("flag \t= 0x%08x\n", p_flpart->flag);
	printf("magic \t= 0x%08x\n", p_flpart->magic);
	printf("\n");
}

static void display_dev (fldev_t *p_dev)
{
	int i;
	printf("sn		= %s\n", p_dev->sn);
	printf("magic		= 0x%08x\n", p_dev->magic);
	printf("usbdl_mode	= 0x%08x\n", p_dev->usbdl_mode);
	printf("cmdline	= %s\n", p_dev->cmdline);
	printf("force_ptb	= 0x%08x\n", p_dev->pba_boot_flag[0]);
       printf("upgrade status = 0x%08x\n", p_dev->pba_boot_flag[1]);
	printf("auto_boot 	= %d\n", p_dev->auto_boot);
	printf("auto_dl 	= %d\n", p_dev->auto_dl);
	printf("pri_file	= %s\n", p_dev->pri_file);
 	printf("pri_addr	= 0x%08x\n", p_dev->pri_addr);

	for (i=0 ; i< ETH_INSTANCES ; i++)
	{
		printf("eth%d MAC	= %02x:%02x:%02x:%02x:%02x:%02x\n",i,
			p_dev->eth[i].mac[0], p_dev->eth[i].mac[1],
			p_dev->eth[i].mac[2], p_dev->eth[i].mac[3],
			p_dev->eth[i].mac[4], p_dev->eth[i].mac[5]);
		printf("Eth%d ip  	= %d.%d.%d.%d\n",i,
			(p_dev->eth[i].ip) &0xff, (p_dev->eth[i].ip>>8) &0xff,
			(p_dev->eth[i].ip>>16) &0xff, (p_dev->eth[i].ip>>24) &0xff);
		printf("Eth%d mask	= %d.%d.%d.%d\n",i,
			(p_dev->eth[i].mask) &0xff, (p_dev->eth[i].mask>>8) &0xff,
			(p_dev->eth[i].mask>>16) &0xff, (p_dev->eth[i].mask>>24) &0xff);
		printf("Eth%d gw  	= %d.%d.%d.%d\n",i,
			(p_dev->eth[i].gw) &0xff, (p_dev->eth[i].gw>>8) &0xff,
			(p_dev->eth[i].gw>>16) &0xff, (p_dev->eth[i].gw>>24) &0xff);
	}
	if (USE_WIFI)
	{
		for (i=0; i< USE_WIFI; i++)
		{
			printf("WIFI%d MAC	= %02x:%02x:%02x:%02x:%02x:%02x\n",i,
				p_dev->wifi[i].mac[0], p_dev->wifi[i].mac[1],
				p_dev->wifi[i].mac[2], p_dev->wifi[i].mac[3],
				p_dev->wifi[i].mac[4], p_dev->wifi[i].mac[5]);
			printf("WIFI%d ip		= %d.%d.%d.%d\n",i,
				(p_dev->wifi[i].ip) &0xff,(p_dev->wifi[i].ip>>8) &0xff,
				(p_dev->wifi[i].ip>>16) &0xff, (p_dev->wifi[i].ip>>24) &0xff);
			printf("WIFI%d mask	= %d.%d.%d.%d\n",i,
				(p_dev->wifi[i].mask) &0xff,(p_dev->wifi[i].mask>>8) &0xff,
				(p_dev->wifi[i].mask>>16) &0xff, (p_dev->wifi[i].mask>>24) &0xff);
			printf("WIFI%d gw		= %d.%d.%d.%d\n",i,
				(p_dev->wifi[i].gw) &0xff,(p_dev->wifi[i].gw>>8) &0xff,
				(p_dev->wifi[i].gw>>16) &0xff, (p_dev->wifi[i].gw>>24) &0xff);
		}
	}
	printf("tftpd		= %d.%d.%d.%d\n",
		( p_dev->tftpd) &0xff,(p_dev->tftpd>>8) &0xff,
		(p_dev->tftpd>>16) &0xff, (p_dev->tftpd>>24) &0xff);
	printf("\n");

}

#define NANDWRITE_OPTIONS_BASE		0
#define NETWORK_OPTION_BASE		20

enum numeric_short_options {
	HELP = NANDWRITE_OPTIONS_BASE,
	VERSION_INFO,
	AUTO_DOWNLOAD,
	AUTO_BOOT,
	PRI_ADDR,
	PRI_FILE,
	SHOW_INFO,

	ETH0_IP = NETWORK_OPTION_BASE,
	ETH0_MASK,
	ETH0_GW,
	TFTPD,
	ETH1_MAC,
	ETH1_IP,
	ETH1_MASK,
	ETH1_GW,
	WIFI0_MAC,  //20120331+ for updating with the fldev_t struct
	WIFI0_IP,
	WIFI0_MASK,
	WIFI0_GW,
#if (USE_WIFI >= 2)
	WIFI1_MAC,
	WIFI1_IP,
	WIFI1_MASK,
	WIFI1_GW,
#endif
	USB_ETH0_MAC,
	USB_ETH0_IP,
	USB_ETH0_MASK,
	USB_ETH0_GW,
	USB_ETH1_MAC,
	USB_ETH1_IP,
	USB_ETH1_MASK,
	USB_ETH1_GW,
};

static const char *short_options = "b:fjopqs:ytKMRBGHQIUWXYZC:F:L:S:D:V:E:N:B:P:";
static struct option long_options[] = {
	{"jffs2", no_argument, 0, 'j'},
	{"yaffs", no_argument, 0, 'y'},
	{"forcelegacy", no_argument, 0, 'f'},
	{"oob", no_argument, 0, 'o'},
	{"start", required_argument, 0, 's'},
	{"pad", no_argument, 0, 'p'},
	{"blockalign", required_argument, 0, 'b'},
	{"quiet", no_argument, 0, 'q'},
	{"skiphead", no_argument, 0, 't'},
	{"help", no_argument, 0, HELP},
	{"version", no_argument, 0, VERSION_INFO},

	//{"ptb", required_argument, 0, 'P'},
	{"bld", no_argument, 0, 'G'},
	{"hal", no_argument, 0, 'H'},
	{"pba", no_argument, 0, 'Q'},
	{"sec", no_argument, 0, 'I'},
	{"dsp", no_argument, 0, 'U'},
	{"lnx", no_argument, 0, 'W'},
	{"swp", no_argument, 0, 'X'},
	{"add", no_argument, 0, 'Y'},
	{"adc", no_argument, 0, 'Z'},
	{"pri", no_argument, 0, 'K'},
	{"rmd", no_argument, 0, 'M'},
	{"rom", no_argument, 0, 'R'},
	{"bak", no_argument, 0, 'B'},

	{"cmd", required_argument, 0, 'C'},
	{"flag", required_argument, 0, 'F'},
	{"safe", required_argument, 0, 'S'},
	{"upgrade", required_argument, 0, 'P'},
	{"load", required_argument, 0, 'L'},

	{"date", required_argument, 0, 'D'},
	{"ver", required_argument, 0, 'V'},
	{"ethmac", required_argument, 0, 'E'},
	{"sn   ", required_argument, 0, 'N'},

	{"auto_boot", required_argument, 0, AUTO_BOOT},
	{"auto_dl", required_argument, 0, AUTO_DOWNLOAD},

	{"pri_addr", required_argument, 0, PRI_ADDR},
	{"pri_file", required_argument, 0, PRI_FILE},

	{"lan_ip", required_argument, 0, ETH0_IP},
	{"lan_mask", required_argument, 0, ETH0_MASK},
	{"lan_gw", required_argument, 0, ETH0_GW},
	{"tftpd  ", required_argument, 0, TFTPD},

	{"eth1_mac", required_argument, 0, ETH1_MAC},
	{"eth1_ip", required_argument, 0, ETH1_IP},
	{"eth1_mask", required_argument, 0, ETH1_MASK},
	{"eth1_gw", required_argument, 0, ETH1_GW},

	{"wifi0_mac", required_argument, 0, WIFI0_MAC},
	{"wifi0_ip", required_argument, 0, WIFI0_IP},
	{"wifi0_mask", required_argument, 0, WIFI0_MASK},
	{"wifi0_gw", required_argument, 0, WIFI0_GW},
#if (USE_WIFI >= 2)
	{"wifi1_mac", required_argument, 0, WIFI1_MAC},
	{"wifi1_ip", required_argument, 0, WIFI1_IP},
	{"wifi1_mask", required_argument, 0, WIFI1_MASK},
	{"wifi1_gw", required_argument, 0, WIFI1_GW},
#endif
	{"show_info", no_argument, 0, SHOW_INFO},
	{0, 0, 0, 0},
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"", "force jffs2 oob layout (legacy support)"},
	{"", "force yaffs oob layout (legacy support)"},
	{"", "force legacy support on autoplacement enabled mtd device"},
	{"", "image contains oob data"},
	{"", "set start address (default is 0)"},
	{"", "pad to page size"},
	{"blockalign=1|2|4", "set multiple of eraseblocks to align to"},
	{"", "don't display progress messages"},
	{"", "skip head when update if image have"},
	{"", "display this help and exit"},
	{"", "output version information and exit"},
	{"", "Update Ambarella AMBOOT BLD Partition"},
	{"", "Update Ambarella AMBOOT HAL Partition"},
	{"", "Update Ambarella AMBOOT PBA Partition"},
	{"", "Update Ambarella AMBOOT SEC Partition "},
	{"", "Update Ambarella AMBOOT DSP Partition "},
	{"", "Update Ambarella AMBOOT LNX Partition "},
	{"", "Update Ambarella AMBOOT SWP Partition "},
	{"", "Update Ambarella AMBOOT ADD Partition "},
	{"", "Update Ambarella AMBOOT ADC Partition "},
	{"", "Update Ambarella AMBOOT PRI Partition "},
	{"", "Update Ambarella AMBOOT RMD Partition "},
	{"", "Update Ambarella AMBOOT ROM Partition "},
	{"", "Update Ambarella AMBOOT BAK Partition "},
	{"", "Update Ambarella AMBOOT CMD line "},
	{"hex", "Update Ambarella AMBOOT Partition load flag "},
	{"hex", "force AMBOOT boot form PTB, safe recovery "},
	{"hex", "cloud camera upgrade stage flag "},
	{"hex", "Update Ambarella AMBOOT Partition load address "},
	{"hex", "Update Ambarella AMBOOT Partition date "},
	{"hex", "Update Ambarella AMBOOT Partition version "},
	{"MAC", "Update Ambarella AMBOOT eth0 MAC "},
	{"sn", "Update Ambarella AMBOOT SN "},
	{"hex", "Update Ambarella AMBOOT load linux from nandflash or no "},
	{"hex", "Update Ambarella AMBOOT load linux from network or no "},
	{"str", "Update Ambarella updated file name(32bytes) "},
	{"hex", "Update Ambarella AMBOOT pri_addr "},
	{"ip", "Update Ambarella AMBOOT eth0 lan ip "},
	{"ip", "Update Ambarella AMBOOT eth0 lan mask "},
	{"ip", "Update Ambarella AMBOOT eth0 lan gw "},
	{"ip", "Update Ambarella AMBOOT eth0 tftpd ip "},
	{"MAC", "Update Ambarella AMBOOT eth1 MAC "},
	{"ip", "Update Ambarella AMBOOT eth1 lan ip "},
	{"ip", "Update Ambarella AMBOOT eth1 lan mask "},
	{"ip", "Update Ambarella AMBOOT eth1 lan gw "},
	{"MAC", "Update Ambarella AMBOOT wifi0 MAC "},
	{"ip", "Update Ambarella AMBOOT wifi0 ip "},
	{"ip", "Update Ambarella AMBOOT wifi0 mask"},
	{"ip", "Update Ambarella AMBOOT wifi0 gw "},
#if (USE_WIFI >= 2)
	{"MAC", "Update Ambarella AMBOOT wifi1 MAC "},
	{"ip", "Update Ambarella AMBOOT wifi1 ip "},
	{"ip", "Update Ambarella AMBOOT wifi1 mask"},
	{"ip", "Update Ambarella AMBOOT wifi1 gw "},
#endif
	{"",  "Show Ambarella AMBOOT information "},

};

static void usage(void)
{
	int i;
	printf("Usage:nandwrite [OPTION] MTD_DEVICE INPUTFILE\n"
			"Writes to the specified MTD device.\n"
			"\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

/**
 * Converts a string to a ethernet HW address. (xx:xx:xx:xx:xx:xx).
 */
static int str_to_hwaddr(const char *bufp, uint8_t *hwaddr)
{
	unsigned char *ptr;
	int i, j;
	unsigned char val;
	unsigned char c;

	ptr = hwaddr;
 	i = 0;
	do {
                j = val = 0;

		/* We might get a semicolon here - not required. */
		if (i && (*bufp == ':')) {
			bufp++;
		}

		do {
			c = *bufp;
			if (((unsigned char)(c - '0')) <= 9) {
				c -= '0';
			} else if (((unsigned char)((c | 0x20) - 'a')) <= 5) {
				c = (c | 0x20) - ('a' - 10);
			} else if (j && (c == ':' || c == 0)) {
				break;
			} else {
				return -1;
			}
			++bufp;
			val <<= 4;
			val += c;
		} while (++j < 2);
		*ptr++ = val;
	} while (++i < MAC_SIZE);

	return (int) (*bufp);   /* Error if we don't end at end of string. */
}

static int str_to_ipaddr(const char *src, uint32_t *addr)
{
	int saw_digit, octets, ch;
	unsigned char tmp[4], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {

		if (ch >= '0' && ch <= '9') {
			unsigned int new = *tp * 10 + (ch - '0');

			if (new > 255)
				return (0);
			*tp = new;
			if (! saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
                                return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return -1;
        }
	if (octets < 4)
		return -2;
	memcpy(addr, tmp, 4);

	return 0;
}

static char     *mtd_device, *img;
static int      mtdoffset = 0;
static int      quiet = 0;
static int      writeoob = 0;
static int      skip_head = 0;
static int      forcejffs2 = 0;
static int      forceyaffs = 0;
static int      forcelegacy = 0;
static int      pad = 0;
static int      blockalign = 1; /*default to using 16K block size */
static char     ptb_device[1024] = "/dev/mtd1";
static int      ambarella_ptb = 0;
static int      ambarella_bld = 0;
static int      ambarella_hal = 0;
static int      ambarella_pba = 0;
static int      ambarella_sec = 0;
static int      ambarella_dsp = 0;
static int      ambarella_lnx = 0;
static int      ambarella_swp = 0;
static int      ambarella_add = 0;
static int      ambarella_adc = 0;
static int      ambarella_pri = 0;
static int      ambarella_rmd = 0;
static int      ambarella_rom = 0;
static int      ambarella_bak = 0;
static int      ambarella_cmd = 0;
static uint32_t ambarella_flpart_flag = 0;
static uint32_t ambarella_flpart_mem_addr = 0;
static uint32_t ambarella_flpart_version = 0;
static uint32_t ambarella_flpart_date = 0;
static int      ambarella_force_ptb = 0;
static int      ambarella_ptb_data = 0;
static char     ambarella_cmdline[CMD_LINE_SIZE];
static int      ambarella_skip_img = 0;

static int      ambarella_sn = 0;
static char     ambarella_sn_data[SN_SIZE];

static int      ambarella_auto_dl = 0;
static uint8_t  ambarella_auto_dl_data=1;

static int      ambarella_auto_boot = 0;
static uint8_t  ambarella_auto_boot_data=1;

static int      ambarella_eth = 0;
static uint8_t  ambarella_eth_mac[MAC_SIZE];

static int      ambarella_lan_ip = 0;
static uint32_t ambarella_lan_ip_data;

static int      ambarella_lan_mask = 0;
static uint32_t ambarella_lan_mask_data;

static int      ambarella_lan_gw = 0;
static uint32_t ambarella_lan_gw_data;

static int      ambarella_tftpd = 0;
static uint32_t ambarella_tftpd_data;

#if (ETH_INSTANCES >= 2)
static int      ambarella_eth1 = 0;
static uint8_t  ambarella_eth1_mac[MAC_SIZE];

static int      ambarella_eth1_ip = 0;
static uint32_t ambarella_eth1_ip_data;

static int      ambarella_eth1_mask = 0;
static uint32_t ambarella_eth1_mask_data;

static int      ambarella_eth1_gw = 0;
static uint32_t ambarella_eth1_gw_data;

#endif

#if ( USE_WIFI >=1 )
static int      ambarella_wifi0 = 0;
static uint8_t  ambarella_wifi0_mac[MAC_SIZE];

static int      ambarella_wifi0_ip = 0;
static uint32_t ambarella_wifi0_ip_data;

static int      ambarella_wifi0_mask = 0;
static uint32_t ambarella_wifi0_mask_data;

static int      ambarella_wifi0_gw = 0;
static uint32_t ambarella_wifi0_gw_data;

#endif

/* 20120331+ for updating with the fldev_t struct */
#if (USE_WIFI >=2)
static int      ambarella_wifi1 = 0;
static uint8_t  ambarella_wifi1_mac[MAC_SIZE];

static int      ambarella_wifi1_ip = 0;
static uint32_t ambarella_wifi1_ip_data;

static int      ambarella_wifi1_mask = 0;
static uint32_t ambarella_wifi1_mask_data;

static int      ambarella_wifi1_gw = 0;
static uint32_t ambarella_wifi1_gw_data;

#endif

static int      ambarella_show_info_flag = 0;

static int      ambarella_pri_addr = 0;
static uint32_t ambarella_pri_addr_data;

static int      ambarella_pri_file = 0;
static char     ambarella_pri_file_data[32];

static int      ambarella_set_upgrade_status = 0;
static int      ambarella_upgrade_status_data = 0;

static void process_options (int argc, char *argv[])
{
	int error = 0;
	int ch;
	int option_index = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
			case HELP:
				usage();
				break;
			case VERSION_INFO:
				display_version();
				break;
			case AUTO_DOWNLOAD:
				ambarella_auto_dl = 1;
				ambarella_auto_dl_data=strtoul (optarg, NULL, 0);
				ambarella_skip_img = 1;
				break;
			case AUTO_BOOT:
				ambarella_auto_boot = 1;
				ambarella_auto_boot_data=strtoul (optarg, NULL, 0);
				ambarella_skip_img = 1;
				break;
			case PRI_ADDR:
				ambarella_pri_addr = 1;
				ambarella_pri_addr_data = strtoul (optarg, NULL, 0);
				ambarella_skip_img = 1;
				break;
			case PRI_FILE:
				ambarella_pri_file = 1;
				strncpy(ambarella_pri_file_data, optarg, SN_SIZE);
				ambarella_skip_img = 1;
				break;
			case ETH0_IP:
				ambarella_lan_ip=1;
				if (str_to_ipaddr(optarg,&ambarella_lan_ip_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case ETH0_MASK:
				ambarella_lan_mask = 1;
				if (str_to_ipaddr(optarg,&ambarella_lan_mask_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case ETH0_GW:
				ambarella_lan_gw = 1;
				if (str_to_ipaddr(optarg,&ambarella_lan_gw_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case TFTPD:
				ambarella_tftpd = 1;
				if (str_to_ipaddr(optarg,&ambarella_tftpd_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;

			case ETH1_MAC:
				if (str_to_hwaddr(optarg, ambarella_eth1_mac) != 0) {
					perror ("hwaddr error!\n");
					error = 1;
				}
				ambarella_eth1 = 1;
				ambarella_skip_img = 1;
				break;
			case ETH1_IP:
				ambarella_eth1_ip=1;
				if (str_to_ipaddr(optarg,&ambarella_eth1_ip_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case ETH1_MASK:
				ambarella_eth1_mask = 1;
				if (str_to_ipaddr(optarg,&ambarella_eth1_mask_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case ETH1_GW:
				ambarella_eth1_gw=1;
				if (str_to_ipaddr(optarg,&ambarella_eth1_gw_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;

			case WIFI0_MAC:
				if (str_to_hwaddr(optarg, ambarella_wifi0_mac) != 0) {
					perror ("hwaddr error!\n");
					error = 1;
				}
				ambarella_wifi0 = 1;
				ambarella_skip_img = 1;
				break;
			case WIFI0_IP:
				ambarella_wifi0_ip = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi0_ip_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case WIFI0_MASK:
				ambarella_wifi0_mask = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi0_mask_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case WIFI0_GW:
				ambarella_wifi0_gw = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi0_gw_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			#if (USE_WIFI >= 2)
			case WIFI1_MAC:
				if (str_to_hwaddr(optarg, ambarella_wifi1_mac) != 0) {
					perror ("hwaddr error!\n");
					error = 1;
				}
				ambarella_wifi1 = 1;
				ambarella_skip_img = 1;
				break;
			case WIFI1_IP:
				ambarella_wifi1_ip = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi1_ip_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case WIFI1_MASK:
				ambarella_wifi1_mask = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi1_mask_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			case WIFI1_GW:
				ambarella_wifi1_gw = 1;
				if (str_to_ipaddr(optarg,&ambarella_wifi1_gw_data))
				{
					printf ("IP error \n");
					return;
				}
				ambarella_skip_img = 1;
				break;
			#endif

			case SHOW_INFO:
				ambarella_show_info_flag = 1;
				ambarella_skip_img = 1;
				break;
			case 'q':
				quiet = 1;
				break;
			case 'j':
				forcejffs2 = 1;
				break;
			case 'y':
				forceyaffs = 1;
				break;
			case 'f':
				forcelegacy = 1;
				break;
			case 'o':
				writeoob = 1;
				break;
			case 'p':
				pad = 1;
				break;
			case 's':
				mtdoffset = strtol (optarg, NULL, 0);
				break;
			case 'b':
				blockalign = atoi (optarg);
				break;
			case 't':
				skip_head = 1;
				break;
			case 'T':
				strncpy(ptb_device, optarg, sizeof(ptb_device));
				break;
			case 'K':
				ambarella_pri = 1;
				break;
			case 'M':
				ambarella_rmd = 1;
				break;
			case 'R':
				ambarella_rom = 1;
				break;
			case 'B':
				ambarella_bak = 1;
				break;
			case 'A':
				ambarella_ptb = 1;
				break;
			case 'G':
				ambarella_bld = 1;
				break;
			case 'H':
				ambarella_hal = 1;
				break;
			case 'Q':
				ambarella_pba = 1;
				break;
			case 'I':
				ambarella_sec = 1;
				break;
			case 'U':
				ambarella_dsp = 1;
				break;
			case 'W':
				ambarella_lnx = 1;
				break;
			case 'X':
				ambarella_swp = 1;
				break;
			case 'Y':
				ambarella_add = 1;
				break;
			case 'Z':
				ambarella_adc = 1;
				break;
			case 'C':
				ambarella_cmd = 1;
				strncpy(ambarella_cmdline, optarg, CMD_LINE_SIZE);
				ambarella_skip_img = 1;
				break;
			case 'F':
				ambarella_flpart_flag = strtoul (optarg, NULL, 0);
				break;
			case 'L':
				ambarella_flpart_mem_addr = strtoul (optarg, NULL, 0);
				break;
			case 'V':
				ambarella_flpart_version = strtoul (optarg, NULL, 0);
				break;
			case 'D':
				ambarella_flpart_date = strtoul (optarg, NULL, 0);
				break;
			case 'S':
				ambarella_force_ptb = 1;
				ambarella_ptb_data = atoi (optarg);
				ambarella_skip_img = 1;
				break;
			case 'E':
				if (str_to_hwaddr(optarg, ambarella_eth_mac) != 0) {
					perror ("hwaddr error!\n");
					error = 1;
				}
				ambarella_eth = 1;
				ambarella_skip_img = 1;
				break;
			case 'N':
				strncpy(ambarella_sn_data, optarg, SN_SIZE);
				ambarella_sn = 1;
				ambarella_skip_img = 1;
				break;
                     case 'P':
                            ambarella_set_upgrade_status = 1;
			      ambarella_upgrade_status_data = atoi (optarg);
                            ambarella_skip_img = 1;
                            break;
			case '?':
				error = 1;
				break;
		}
	}

	if ((((argc - optind) != 2) && (ambarella_skip_img != 1)) || error)
		usage ();

	if ((argc - optind) == 2) {
		mtd_device = argv[optind++];
		img = argv[optind];
		ambarella_skip_img = 0;
	}
}

/**
 * Get the ptb.dev parameters which are original settings of firmware.
 */
#if 0
static void flprog_get_dev_param(flpart_table_t *table)
{
	table->dev.usbdl_mode = 0;
	table->dev.auto_boot = 1;
	memset(table->dev.cmdline, 0x0, sizeof(table->dev.cmdline));
}
#endif

/**
 * Get the content of the partition table.
 */
static int flprog_get_part_table (uint8_t **ptb_buf)
{
	int ret, i, count;
	int ptb_fd, ptb_offset;
	struct mtd_info_user ptb_meminfo;
	loff_t ptb_bad_offset;
	flpart_table_t *table;

	/* Open the PTB device */
	if ((ptb_fd = open(ptb_device, O_RDONLY)) == -1) {
		perror("open PTB");
		exit(1);
	}

	/* Fill in MTD device capability structure */
	if ((ret = ioctl(ptb_fd, MEMGETINFO, &ptb_meminfo)) != 0) {
		perror("PTB MEMGETINFO");
		goto closeall;
	}

	for (ptb_offset = 0; ptb_offset < ptb_meminfo.size; ptb_offset += ptb_meminfo.erasesize) {
		ptb_bad_offset = ptb_offset;
		if ((ret = ioctl(ptb_fd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
			perror("ioctl(MEMGETBADBLOCK)");
			goto closeall;
		}

		if (ret == 0)
			break;

		if (!quiet)
			fprintf (stderr,
			"Bad block at %x, from %x will be skipped\n",
			(int)ptb_bad_offset, ptb_offset);
	}
	if (ptb_offset >= ptb_meminfo.size) {
		fprintf(stderr, "Can't find good block in PTB.\n");
		ret = -1;
		goto closeall;
	}

	/* ptb_buf will be freed in flprog_set_part_table() */
	*ptb_buf = xmalloc(ptb_meminfo.erasesize);

	/* Read partition table.
	 * Note: we need to read and save the entire block data, because the
	 * entire block will be erased when write partition table back to flash.
	 * BTW, flpart_meta_t is located in the same block as flpart_table_t
	 */
	count = ptb_meminfo.erasesize;
	if (pread(ptb_fd, *ptb_buf, count, ptb_offset) != count) {
		perror("pread PTB");
		ret = -1;
		free(*ptb_buf);
		goto closeall;
	}

	table = (flpart_table_t *)(*ptb_buf);
	if (!quiet) {
		 for(i = 0; i < PART_MAX_WITH_RSV; i++){
			if( table->part[i].img_len!=0)
				display_flpart(g_part_str[i], &(table->part[i]));
		 }
		 display_dev(&(table->dev));
	}

closeall:
	close(ptb_fd);

	return ret;
}

/**
 * Program the PTB entry.
 */
static int flprog_set_part_table(uint8_t **ptb_buf)
{
	int ret, i, count, ptb_fd, ptb_offset;
	struct mtd_info_user ptb_meminfo;
	loff_t ptb_bad_offset;
	flpart_table_t *table = (flpart_table_t *)(*ptb_buf);

	/* Open the PTB device */
	if ((ptb_fd = open(ptb_device, O_RDWR)) == -1) {
		perror("open PTB");
		exit(1);
	}

	/* Fill in MTD device capability structure */
	if ((ret = ioctl(ptb_fd, MEMGETINFO, &ptb_meminfo)) != 0) {
		perror("PTB MEMGETINFO");
		goto closeall;
	}

	if (PTB_SIZE > ptb_meminfo.erasesize) {
		fprintf(stderr, "PTB can't fit into erasesize.\n");
		ret = -1;
		goto closeall;
	}

	for (ptb_offset = 0; ptb_offset < ptb_meminfo.size; ptb_offset += ptb_meminfo.erasesize) {
		ptb_bad_offset = ptb_offset;
		if ((ret = ioctl(ptb_fd, MEMGETBADBLOCK, &ptb_bad_offset)) < 0) {
			perror("ioctl(MEMGETBADBLOCK)");
			goto closeall;
		}

		if (ret == 0) {
			/* This isn't a bad block, so erase it first */
			erase_info_t erase;
			erase.start = ptb_offset;
			erase.length = ptb_meminfo.erasesize;
			if ((ret = ioctl(ptb_fd, MEMERASE, &erase)) != 0) {
				perror("PTB MEMERASE");
				continue;
			}
			break;
		}

		if (!quiet)
			fprintf (stderr,
				"Bad block at %x, from %x will be skipped\n",
				(int)ptb_bad_offset, ptb_offset);
	}

	if (ptb_offset >= ptb_meminfo.size) {
		fprintf(stderr, "Can't find good block in PTB.\n");
		ret = -1;
		goto closeall;
	}

#if 0
	if (table->dev.magic != FLPART_MAGIC) {
		memset(&table->dev, 0x0, sizeof(table->dev));
		flprog_get_dev_param(table);
		table->dev.magic = FLPART_MAGIC;
	}

	for(i = 0; i < PART_MAX_WITH_RSV; i++){
		if (table->part[i].img_len != 0 && table->part[i].magic != FLPART_MAGIC) {
			memset(&table->part[i], 0x0, sizeof(table->part[i]));
			table->part[i].magic = FLPART_MAGIC;
		}
	}
#else
	if (table->dev.magic != FLPART_MAGIC) {
		fprintf(stderr, "Invalid dev magic: 0x%08x(0x%08x)\n",
			table->dev.magic, FLPART_MAGIC);
		ret = -1;
		goto closeall;
	}

	for(i = 0; i < PART_MAX_WITH_RSV; i++){
		if (table->part[i].img_len != 0 && table->part[i].magic != FLPART_MAGIC) {
			fprintf(stderr,
				"Invalid partition table magic(%d): 0x%08x(0x%08x)\n",
				i, table->part[i].magic, FLPART_MAGIC);
			ret = -1;
			goto closeall;
		}
	}
#endif

	count = ptb_meminfo.erasesize;
	if (pwrite(ptb_fd, *ptb_buf, count, ptb_offset) != count) {
		perror ("pwrite PTB");
		ret = -1;
		goto closeall;
	}

	if (!quiet) {
		for(i = 0;i < PART_MAX_WITH_RSV; i++){
			if( table->part[i].img_len!=0)
				display_flpart(g_part_str[i], &table->part[i]);
		}
		display_dev(&table->dev);
	}
closeall:
	free(*ptb_buf);
	close(ptb_fd);

	return ret;
}


/*
 * Main program
 */
int nandwrite_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int nandwrite_main(int argc, char **argv)
{
	int cnt, fd, ifd, pagelen, baderaseblock, blockstart = -1;
	int ret, readlen, oobinfochanged = 0;
	int image_crc = ~0U, image_length = 0, imglen = 0;
	uint8_t *ptb_buf = NULL;
	flpart_table_t *ptb_table;
	struct nand_oobinfo old_oobinfo;
	struct mtd_info_user meminfo;
	struct mtd_oob_buf oob;
	loff_t offs;
	unsigned char readbuf[MAX_PAGE_SIZE];
	int file_offset = mtdoffset;
	int buf_num = 0;
	nand_update_file_header_t image_head;

	INIT_G();

	if (argc < 2) {
		usage();
		return -1;
	}

	process_options(argc, argv);

	if (ambarella_skip_img == 1)
		goto ambarella_process;

	memset(G.oobbuf, 0xff, sizeof(G.oobbuf));

	if (pad && writeoob) {
		fprintf(stderr, "Can't pad when oob data is present.\n");
		exit(1);
	}

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	/* Set erasesize to specified number of blocks - to match jffs2
	 * (virtual) block size */
	meminfo.erasesize *= blockalign;

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
			!(meminfo.oobsize == 8 && meminfo.writesize == 256) &&
			!(meminfo.oobsize == 64 && meminfo.writesize == 2048)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}

	/*
	 * force oob layout for jffs2 or yaffs ?
	 * Legacy support
	 */
	if (forcejffs2 || forceyaffs) {
		if (meminfo.oobsize == 8) {
			if (forceyaffs) {
				fprintf (stderr, "YAFSS cannot operate on 256 Byte page size");
				goto restoreoob;
			}
			/* Adjust number of ecc bytes */
			G.jffs2_oobinfo.eccbytes = 3;
		}
	}

	oob.length = meminfo.oobsize;
	oob.ptr = G.oobbuf;

	/* Open the input file */
	if ((ifd = open(img, O_RDONLY)) == -1) {
		perror("open input file");
		goto restoreoob;
	}

	// get image length
	imglen = lseek(ifd, 0, SEEK_END);
	lseek (ifd, 0, SEEK_SET);
	if(skip_head)
	{
		if(read(ifd, &image_head, sizeof(nand_update_file_header_t)) != sizeof(nand_update_file_header_t))
		{
			perror ("File I/O error on input file");
				goto closeall;
		}
		lseek(ifd, image_head.header_size, SEEK_SET);
		imglen =image_head.payload_size;
		file_offset +=image_head.header_size;
	}
	image_length = imglen;

	pagelen = meminfo.writesize + ((writeoob == 1) ? meminfo.oobsize : 0);

	// Check, if file is pagealigned
	if ((!pad) && ((imglen % pagelen) != 0)) {
		fprintf (stderr, "Input file is not page aligned\n");
		goto closeall;
	}

	// Check, if length fits into device
	if ( ((imglen / pagelen) * meminfo.writesize) > (meminfo.size - mtdoffset)) {
		fprintf (stderr, "Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
				imglen, pagelen, meminfo.writesize, meminfo.size);
		perror ("Input file does not fit into device");
		goto closeall;
	}

	crc32_table = crc32_filltable(NULL, 0);

	/* Get data from input and write to the device */
	while (imglen && (mtdoffset < meminfo.size)) {
		// new eraseblock , check for bad block(s)
		// Stay in the loop to be sure if the mtdoffset changes because
		// of a bad block, that the next block that will be written to
		// is also checked. Thus avoiding errors if the block(s) after the
		// skipped block(s) is also bad (number of blocks depending on
		// the blockalign
		while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) {
			blockstart = mtdoffset & (~meminfo.erasesize + 1);
			offs = blockstart;
			baderaseblock = 0;
			if (!quiet)
				fprintf (stdout, "Writing data to block %x\n", blockstart);

			/* Check all the blocks in an erase block for bad blocks */
			do {
				if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
					perror("ioctl(MEMGETBADBLOCK)");
					goto closeall;
				}
				if (ret == 1) {
					baderaseblock = 1;
					if (!quiet)
						fprintf (stderr, "Bad block at %x, %u block(s) "
								"from %x will be skipped\n",
								(int) offs, blockalign, blockstart);
				}

				if (baderaseblock) {
					mtdoffset = blockstart + meminfo.erasesize;
				}
				offs +=  meminfo.erasesize / blockalign ;
			} while ( offs < blockstart + meminfo.erasesize );

		}

		readlen = meminfo.writesize;
		if (pad && (imglen < readlen))
		{
			readlen = imglen;
			memset(G.writebuf + readlen, 0xff, meminfo.writesize - readlen);
		}

		/* Read Page Data from input file */
		if ((cnt = pread(ifd, G.writebuf, readlen,file_offset)) != readlen) {
			if (cnt == 0)	// EOF
				break;
			perror ("File I/O error on input file");
			goto closeall;
		}

		image_crc = crc32(image_crc, G.writebuf, cnt);

		if (writeoob) {
			int i, start, len, filled;
			/* Read OOB data from input file, exit on failure */
			if ((cnt = pread(ifd, G.oobreadbuf, meminfo.oobsize,file_offset)) != meminfo.oobsize) {
				perror ("File I/O error on input file");
				goto closeall;
			}
			/*
			 *  We use autoplacement and have the oobinfo with the autoplacement
			 * information from the kernel available
			 *
			 * Modified to support out of order oobfree segments,
			 * such as the layout used by diskonchip.c
			 */
			if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE)) {
				for (filled = 0, i = 0; old_oobinfo.oobfree[i][1] && (i < MTD_MAX_OOBFREE_ENTRIES); i++) {
					/* Set the reserved bytes to 0xff */
					start = old_oobinfo.oobfree[i][0];
					len = old_oobinfo.oobfree[i][1];
					memcpy(G.oobbuf + start,
							G.oobreadbuf + filled,
							len);
					filled += len;
				}
			} else {
				/* Set at least the ecc byte positions to 0xff */
				start = old_oobinfo.eccbytes;
				len = meminfo.oobsize - start;
				memcpy(G.oobbuf + start,
						G.oobreadbuf + start,
						len);
			}
			/* Write OOB data first, as ecc will be placed in there*/
			oob.start = mtdoffset;
			if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
				perror ("ioctl(MEMWRITEOOB)");
				goto closeall;
			}
			imglen -= meminfo.oobsize;
		}

		/* Write out the Page data */
		if (pwrite(fd, G.writebuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
			perror ("pwrite");
			goto closeall;
		}

		/* read out the Page data */
		if (pread(fd, readbuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
			perror ("pread");
			goto closeall;
		}

		buf_num=0;
		while ((G.writebuf[buf_num]==readbuf[buf_num]) && (buf_num < readlen)) buf_num++;

		//if (((blockstart/MAX_PAGE_SIZE/64) % 10) == 9)
		if (buf_num < readlen)
		{
			//printf("offs[%x ],blockstart[%x],mtdoffset[%x],writesize[0x%x], buf_num[0x%x]\n",(int)offs,blockstart,mtdoffset,meminfo.writesize, buf_num);

			/* set bad blocks */
			offs = (loff_t) blockstart;
			if ((ret = ioctl(fd, MEMSETBADBLOCK, &offs)) < 0) {
				perror("ioctl(MEMSETBADBLOCK)");
				goto closeall;
			}
			if ((ret == 0) && (!quiet)) {
				fprintf (stdout, "set Bad block at %x !\n",blockstart);
				file_offset = file_offset - (mtdoffset-blockstart);
				imglen = imglen+ (mtdoffset-blockstart);;
				mtdoffset = blockstart + meminfo.erasesize;
			}
		}
		else {
			imglen -= readlen;
			mtdoffset += meminfo.writesize;
			file_offset+= meminfo.writesize;
		}
	}

closeall:
	close(ifd);

restoreoob:

	close(fd);

	if (imglen > 0) {
		perror ("Data was only partially written due to error\n");
		exit (1);
	}

	image_crc ^= ~0U;
	printf ("image_length = 0x%08x\n", image_length);
	printf ("image_crc = 0x%08x\n", image_crc);
ambarella_process:
	if (ambarella_bld ||ambarella_force_ptb|| ambarella_hal ||
		ambarella_pba || ambarella_sec || ambarella_dsp ||
		ambarella_lnx || ambarella_swp || ambarella_add ||
		ambarella_adc || ambarella_pri || ambarella_rmd ||
		ambarella_rom || ambarella_cmd || ambarella_eth ||
		ambarella_sn || ambarella_bak || ambarella_auto_dl ||
		ambarella_auto_boot || ambarella_lan_ip || ambarella_lan_mask ||
		ambarella_lan_gw || ambarella_tftpd || ambarella_pri_addr ||
		ambarella_pri_file || ambarella_show_info_flag||
		ambarella_eth1 || ambarella_eth1_ip || ambarella_eth1_mask ||
		ambarella_eth1_gw ||
		ambarella_wifi0 || ambarella_wifi0_ip || ambarella_wifi0_mask ||
		ambarella_wifi0_gw || ambarella_set_upgrade_status
#if (USE_WIFI >= 2)
		|| ambarella_wifi1 || ambarella_wifi1_ip || ambarella_wifi1_mask ||
		ambarella_wifi1_gw
#endif
		) {
		if (flprog_get_part_table(&ptb_buf) < 0)
			exit (1);
	}

	ptb_table = (flpart_table_t *)ptb_buf;

	if (ambarella_pri) {
		ptb_table->part[PART_PRI].img_len = image_length;
		ptb_table->part[PART_PRI].crc32 = image_crc;
		ptb_table->part[PART_PRI].flag = ambarella_flpart_flag;
		ptb_table->part[PART_PRI].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PRI].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PRI].ver_date = ambarella_flpart_date;
	}
	if (ambarella_rmd) {
		ptb_table->part[PART_RMD].img_len = image_length;
		ptb_table->part[PART_RMD].crc32 = image_crc;
		ptb_table->part[PART_RMD].flag = ambarella_flpart_flag;
		ptb_table->part[PART_RMD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_RMD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_RMD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_rom) {
		ptb_table->part[PART_ROM].img_len = image_length;
		ptb_table->part[PART_ROM].crc32 = image_crc;
		ptb_table->part[PART_ROM].flag = ambarella_flpart_flag;
		ptb_table->part[PART_ROM].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ROM].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ROM].ver_date = ambarella_flpart_date;
	}
	if (ambarella_bak) {
		ptb_table->part[PART_BAK].img_len = image_length;
		ptb_table->part[PART_BAK].crc32 = image_crc;
		ptb_table->part[PART_BAK].flag = ambarella_flpart_flag;
		ptb_table->part[PART_BAK].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_BAK].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_BAK].ver_date = ambarella_flpart_date;
	}

	if (ambarella_ptb) {
		ptb_table->part[PART_PTB].img_len = image_length;
		ptb_table->part[PART_PTB].crc32 = image_crc;
		ptb_table->part[PART_PTB].flag = ambarella_flpart_flag;
		ptb_table->part[PART_PTB].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PTB].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PTB].ver_date = ambarella_flpart_date;
	}
	if (ambarella_bld) {
		ptb_table->part[PART_BLD].img_len = image_length;
		ptb_table->part[PART_BLD].crc32 = image_crc;
		ptb_table->part[PART_BLD].flag = ambarella_flpart_flag;
		ptb_table->part[PART_BLD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_BLD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_BLD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_hal) {
		ptb_table->part[PART_HAL].img_len = image_length;
		ptb_table->part[PART_HAL].crc32 = image_crc;
		ptb_table->part[PART_HAL].flag = ambarella_flpart_flag;
		ptb_table->part[PART_HAL].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_HAL].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_HAL].ver_date = ambarella_flpart_date;
	}
	if (ambarella_pba) {
		ptb_table->part[PART_PBA].img_len = image_length;
		ptb_table->part[PART_PBA].crc32 = image_crc;
		ptb_table->part[PART_PBA].flag = ambarella_flpart_flag;
		ptb_table->part[PART_PBA].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PBA].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PBA].ver_date = ambarella_flpart_date;
	}
	if (ambarella_sec) {
		ptb_table->part[PART_SEC].img_len = image_length;
		ptb_table->part[PART_SEC].crc32 = image_crc;
		ptb_table->part[PART_SEC].flag = ambarella_flpart_flag;
		ptb_table->part[PART_SEC].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_SEC].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_SEC].ver_date = ambarella_flpart_date;
	}
	if (ambarella_dsp) {
		ptb_table->part[PART_DSP].img_len = image_length;
		ptb_table->part[PART_DSP].crc32 = image_crc;
		ptb_table->part[PART_DSP].flag = ambarella_flpart_flag;
		ptb_table->part[PART_DSP].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_DSP].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_DSP].ver_date = ambarella_flpart_date;
	}
	if (ambarella_lnx) {
		ptb_table->part[PART_LNX].img_len = image_length;
		ptb_table->part[PART_LNX].crc32 = image_crc;
		ptb_table->part[PART_LNX].flag = ambarella_flpart_flag;
		ptb_table->part[PART_LNX].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_LNX].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_LNX].ver_date = ambarella_flpart_date;
	}
	if (ambarella_swp) {
		ptb_table->part[PART_SWP].img_len = image_length;
		ptb_table->part[PART_SWP].crc32 = image_crc;
		ptb_table->part[PART_SWP].flag = ambarella_flpart_flag;
		ptb_table->part[PART_SWP].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_SWP].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_SWP].ver_date = ambarella_flpart_date;
	}
	if (ambarella_add) {
		ptb_table->part[PART_ADD].img_len = image_length;
		ptb_table->part[PART_ADD].crc32 = image_crc;
		ptb_table->part[PART_ADD].flag = ambarella_flpart_flag;
		ptb_table->part[PART_ADD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ADD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ADD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_adc) {
		ptb_table->part[PART_ADC].img_len = image_length;
		ptb_table->part[PART_ADC].crc32 = image_crc;
		ptb_table->part[PART_ADC].flag = ambarella_flpart_flag;
		ptb_table->part[PART_ADC].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ADC].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ADC].ver_date = ambarella_flpart_date;
	}

	if (ambarella_force_ptb) {
		ptb_table->dev.pba_boot_flag[0] = ambarella_ptb_data;
	}

	if (ambarella_cmd) {
		strncpy(ptb_table->dev.cmdline, ambarella_cmdline, CMD_LINE_SIZE);
	}


	if (ambarella_sn) {
		strncpy(ptb_table->dev.sn, ambarella_sn_data, SN_SIZE);
	}


	if (ambarella_auto_dl) {
		ptb_table->dev.auto_dl = ambarella_auto_dl_data;
	}

	if (ambarella_auto_boot) {
		ptb_table->dev.auto_boot = ambarella_auto_boot_data;
	}

	if (ambarella_eth) {
		memcpy(ptb_table->dev.eth[0].mac, ambarella_eth_mac, MAC_SIZE);
	}
	if (ambarella_lan_ip) {
		ptb_table->dev.eth[0].ip = ambarella_lan_ip_data;
	}
	if (ambarella_lan_mask) {
		ptb_table->dev.eth[0].mask = ambarella_lan_mask_data;
	}
	if (ambarella_lan_gw) {
		ptb_table->dev.eth[0].gw = ambarella_lan_gw_data;
	}
	if (ambarella_tftpd) {
		ptb_table->dev.tftpd = ambarella_tftpd_data;
	}

	if (ambarella_eth1) {
		memcpy(ptb_table->dev.eth[1].mac, ambarella_eth1_mac, MAC_SIZE);
	}
	if (ambarella_eth1_ip) {
		ptb_table->dev.eth[1].ip = ambarella_eth1_ip_data;
	}
	if (ambarella_eth1_mask) {
		ptb_table->dev.eth[1].mask = ambarella_eth1_mask_data;
	}
	if (ambarella_eth1_gw) {
		ptb_table->dev.eth[1].gw = ambarella_eth1_gw_data;
	}

	if (ambarella_wifi0) {
		memcpy(ptb_table->dev.wifi[0].mac, ambarella_wifi0_mac, MAC_SIZE);
	}
	if (ambarella_wifi0_ip) {
		ptb_table->dev.wifi[0].ip = ambarella_wifi0_ip_data;
	}
	if (ambarella_wifi0_mask) {
		ptb_table->dev.wifi[0].mask = ambarella_wifi0_mask_data;
	}
	if (ambarella_wifi0_gw) {
		ptb_table->dev.wifi[0].gw = ambarella_wifi0_gw_data;
	}
#if (USE_WIFI >= 2)
	if (ambarella_wifi1) {
		memcpy(ptb_table->dev.wifi[1].mac, ambarella_wifi1_mac, MAC_SIZE);
	}
	if (ambarella_wifi1_ip) {
		ptb_table->dev.wifi[1].ip = ambarella_wifi1_ip_data;
	}
	if (ambarella_wifi1_mask) {
		ptb_table->dev.wifi[1].mask = ambarella_wifi1_mask_data;
	}
	if (ambarella_wifi1_gw) {
		ptb_table->dev.wifi[1].gw = ambarella_wifi1_gw_data;
	}
#endif
	if (ambarella_pri_addr) {
		ptb_table->dev.pri_addr = ambarella_pri_addr_data;
	}

	if (ambarella_pri_file) {
		strncpy(ptb_table->dev.pri_file, ambarella_pri_file_data, SN_SIZE);
	}

        if(ambarella_set_upgrade_status){
		ptb_table->dev.pba_boot_flag[1] = ambarella_upgrade_status_data;
	}

	if (ambarella_bld ||ambarella_force_ptb|| ambarella_hal ||
		ambarella_pba || ambarella_sec || ambarella_dsp ||
		ambarella_lnx || ambarella_swp || ambarella_add ||
		ambarella_adc || ambarella_pri || ambarella_rmd ||
		ambarella_rom || ambarella_cmd || ambarella_eth ||
		ambarella_sn || ambarella_bak ||ambarella_auto_dl||
		ambarella_auto_boot || ambarella_lan_ip || ambarella_lan_mask ||
		ambarella_lan_gw || ambarella_tftpd || ambarella_pri_addr ||
		ambarella_pri_file ||
		ambarella_eth1 || ambarella_eth1_ip || ambarella_eth1_mask ||
		ambarella_eth1_gw ||
		ambarella_wifi0 || ambarella_wifi0_ip || ambarella_wifi0_mask ||
		ambarella_wifi0_gw || ambarella_set_upgrade_status
#if (USE_WIFI >= 2)
		|| ambarella_wifi1 || ambarella_wifi1_ip || ambarella_wifi1_mask ||
		ambarella_wifi1_gw
#endif
		) {
		if (flprog_set_part_table(&ptb_buf) < 0)
			exit (1);
	}

	sync();
	/* Return happy */
	return 0;
}

