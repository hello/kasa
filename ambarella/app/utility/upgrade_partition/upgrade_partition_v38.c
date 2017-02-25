/**
 * upgrade_partition_v38.c
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


//usage:#define nandwrite_trivial_usage
//usage:	"[OPTIONS] MTD-device"
//usage:#define nandwrite_full_usage "\n\n"
//usage:	"Write to the specified MTD device\n"
//usage:     "\n	-j, --jffs2             force jffs2 oob layout (legacy support)"
//usage:     "\n	-y, --yaffs             force yaffs oob layout (legacy support)"
//usage:     "\n	-f, --forcelegacy       force legacy support on autoplacement enabled mtd device"
//usage:     "\n	-s addr, --start=addr   set start address (default is 0)"
//usage:     "\n	-p, --pad               pad to page size"
//usage:     "\n	-b, --blockalign=1|2|4  set multiple of eraseblocks to align to"
//usage:     "\n	-q, --quiet             don't display progress messages"
//usage:     "\n	    --help              display this help and exit"
//usage:     "\n	    --version           output version information and exit"
//usage:     "\n	-G, --bld               Update Ambarella AMBOOT BLD Partition"
//usage:     "\n	-H, --spl               Update Ambarella AMBOOT HAL Partition"
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
//usage:     "\n	-d, --fdt               Update Ambarella AMBOOT FDT"
//usage:     "\n	-F hex, --flag=hex      Update Ambarella AMBOOT Partition load flag"
//usage:     "\n	-S hex, --safe=hex      force AMBOOT boot form PTB, safe recovery"
//usage:     "\n	-L hex, --load=hex      Update Ambarella AMBOOT Partition load address"
//usage:     "\n	-D hex, --date=hex      Update Ambarella AMBOOT Partition date"
//usage:     "\n	-V hex, --ver=hex       Update Ambarella AMBOOT Partition version"
//usage:     "\n	-E MAC, --ethmac=MAC    Update Ambarella AMBOOT MAC"
//usage:     "\n	-N sn, --sn=sn  Update Ambarella AMBOOT SN"


#include "upgrade_partition_v38.h"
#define VERSION "$Revision: 1.308 $ "

#define PROGRAM "upgrade_partition"
#define AMB_VERSION "Ambarella: 0-20140527 $"

const char *g_part_str[] = {"bst", "ptb", "bld", "hal", "pba",
"pri", "sec", "bak", "rmd", "rom",
"dsp", "lnx", "swp", "add", "adc",
"raw", "stg2", "stg", "prf", "cal",
"all"};

const uint32_t crc32_tab[] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

//static const uint32_t *crc32_table;

/* Return a 32-bit CRC of the contents of the buffer. */
static inline uint32_t crc32(uint32_t val, const void *ss, int len)
{
	const unsigned char *s = ss;
	while (--len >= 0)
		val = crc32_tab[(val ^ *s++) & 0xff] ^ (val >> 8);
	return val;
}
#if 0
static uint32_t* crc32_filltable(uint32_t *crc_table, int endian)
{
	uint32_t polynomial = endian ? 0x04c11db7 : 0xedb88320;
	uint32_t c;
	int i, j;

	for (i = 0; i < 256; i++) {
		c = endian ? (i << 24) : i;
		for (j = 8; j; j--) {
			if (endian)
				c = (c&0x80000000) ? ((c << 1) ^ polynomial) : (c << 1);
			else
				c = (c&1) ? ((c >> 1) ^ polynomial) : (c >> 1);
		}
		*crc_table++ = c;
		printf("crc value is [%x] = [0x%08x] \n", i,c);
	}

	return crc_table - 256;
}
#endif

static void display_version (void)
{
	printf(PROGRAM " " VERSION "" AMB_VERSION "\n"
			"\n"
			"Copyright (C) 2014 Ambarella \n"
			"\n");
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
//	printf("force_ptb	= 0x%08x\n", p_dev->pba_boot_flag[0]);
//	printf("upgrade status = 0x%08x\n", p_dev->pba_boot_flag[1]);
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
			printf("WIFI%d ip	= %d.%d.%d.%d\n",i,
				(p_dev->wifi[i].ip) &0xff,(p_dev->wifi[i].ip>>8) &0xff,
				(p_dev->wifi[i].ip>>16) &0xff, (p_dev->wifi[i].ip>>24) &0xff);
			printf("WIFI%d mask	= %d.%d.%d.%d\n",i,
				(p_dev->wifi[i].mask) &0xff,(p_dev->wifi[i].mask>>8) &0xff,
				(p_dev->wifi[i].mask>>16) &0xff, (p_dev->wifi[i].mask>>24) &0xff);
			printf("WIFI%d gw	= %d.%d.%d.%d\n",i,
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
	WIFI0_MAC,
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

//static const char *short_options = "b:fjopqs:ytdKMRBGHQIUWXYZC:F:L:S:D:V:E:N:B:P:";
static const char *short_options = "b:fjopqs:ytKMRBGA:QIUWXYZC:F:L:S:D:V:E:N:B:P:";
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

	{"bld", no_argument, 0, 'G'},
	{"ptb", required_argument, 0, 'A'},
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
//	{"load", required_argument, 0, 'L'},

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
	{"", "Update Ambarella AMBOOT PTB Partition"},
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
//	{"hex", "Update Ambarella AMBOOT Partition load address "},
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
	printf("Usage:upgrade_partition [OPTION] MTD_DEVICE INPUTFILE\n"
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
static char     ptb_device[1024] = "/dev/mtd1";   /*kernel v3.8 amboot ptb is the mtd1 */
static int      ambarella_ptb = 0;
static int      ambarella_bld = 0;
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
//static uint32_t ambarella_flpart_mem_addr = 0;
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

static nand_update_global_t *ptr_update_G;

static void init_nand_update_global(nand_update_global_t *pG)
{
	pG->none_oobinfo.useecc = MTD_NANDECC_OFF;
	pG->autoplace_oobinfo.useecc = MTD_NANDECC_AUTOPLACE;

	pG->jffs2_oobinfo.useecc = MTD_NANDECC_PLACE;
	pG->jffs2_oobinfo.eccbytes = 6;
	pG->jffs2_oobinfo.eccpos[0] = 0;
	pG->jffs2_oobinfo.eccpos[1] = 1;
	pG->jffs2_oobinfo.eccpos[2] = 2;
	pG->jffs2_oobinfo.eccpos[3] = 3;
	pG->jffs2_oobinfo.eccpos[4] = 6;
	pG->jffs2_oobinfo.eccpos[5] = 7;

	pG->yaffs_oobinfo.useecc = MTD_NANDECC_PLACE;
	pG->yaffs_oobinfo.eccbytes = 6;
	pG->yaffs_oobinfo.eccpos[0] = 8;
	pG->yaffs_oobinfo.eccpos[1] = 9;
	pG->yaffs_oobinfo.eccpos[2] = 10;
	pG->yaffs_oobinfo.eccpos[3] = 13;
	pG->yaffs_oobinfo.eccpos[4] = 14;
	pG->yaffs_oobinfo.eccpos[5] = 15;

	memset(pG->oobbuf, 0xff, sizeof(pG->oobbuf));

}

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
			#if 0
			case 'L':
				ambarella_flpart_mem_addr = strtoul (optarg, NULL, 0);
				break;
			#endif
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
	*ptb_buf = malloc(ptb_meminfo.erasesize);
	//memset(*ptb_buf, 0x0, ptb_meminfo.erasesize);

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
int main(int argc, char **argv)
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

	if (argc < 2) {
		usage();
		return -1;
	}

	process_options(argc, argv);

	if (ambarella_skip_img == 1)
		goto ambarella_process;

	/* Update the partition */
	{
		ptr_update_G = malloc(sizeof(nand_update_global_t));
		if (!ptr_update_G) {
			perror("can not malloc buffer for update global.\n");
			exit(1);
		}
		init_nand_update_global(ptr_update_G);

		if (pad && writeoob) {
			fprintf(stderr, "Can't pad when oob data is present.\n");
			exit(1);
		}
		//update_partition_with_img();
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
				ptr_update_G->jffs2_oobinfo.eccbytes = 3;
			}
		}

		oob.length = meminfo.oobsize;
		oob.ptr = ptr_update_G->oobbuf;

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

//		crc32_table = crc32_filltable(NULL, 0);

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
				memset(ptr_update_G->writebuf + readlen, 0xff, meminfo.writesize - readlen);
			}

			/* Read Page Data from input file */
			if ((cnt = pread(ifd, ptr_update_G->writebuf, readlen,file_offset)) != readlen) {
				if (cnt == 0)	// EOF
					break;
				perror ("File I/O error on input file");
				goto closeall;
			}

			image_crc = crc32(image_crc, ptr_update_G->writebuf, cnt);

			if (writeoob) {
				int i, start, len, filled;
				/* Read OOB data from input file, exit on failure */
				if ((cnt = pread(ifd, ptr_update_G->oobreadbuf, meminfo.oobsize,file_offset)) != meminfo.oobsize) {
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
						memcpy(ptr_update_G->oobbuf + start,
								ptr_update_G->oobreadbuf + filled,
								len);
						filled += len;
					}
				} else {
					/* Set at least the ecc byte positions to 0xff */
					start = old_oobinfo.eccbytes;
					len = meminfo.oobsize - start;
					memcpy(ptr_update_G->oobbuf + start,
							ptr_update_G->oobreadbuf + start,
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
			if (pwrite(fd, ptr_update_G->writebuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
				perror ("pwrite");
				goto closeall;
			}

			/* read out the Page data */
			if (pread(fd, readbuf, meminfo.writesize, mtdoffset) != meminfo.writesize) {
				perror ("pread");
				goto closeall;
			}

			buf_num=0;
			while ((ptr_update_G->writebuf[buf_num]==readbuf[buf_num]) && (buf_num < readlen)) buf_num++;

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
		free(ptr_update_G);

	}
	/* end Update the partition */

ambarella_process:
	if (ambarella_bld ||ambarella_force_ptb ||
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
//		ptb_table->part[PART_PRI].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PRI].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PRI].ver_date = ambarella_flpart_date;
	}
	if (ambarella_rmd) {
		ptb_table->part[PART_RMD].img_len = image_length;
		ptb_table->part[PART_RMD].crc32 = image_crc;
		ptb_table->part[PART_RMD].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_RMD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_RMD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_RMD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_rom) {
		ptb_table->part[PART_ROM].img_len = image_length;
		ptb_table->part[PART_ROM].crc32 = image_crc;
		ptb_table->part[PART_ROM].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_ROM].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ROM].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ROM].ver_date = ambarella_flpart_date;
	}
	if (ambarella_bak) {
		ptb_table->part[PART_BAK].img_len = image_length;
		ptb_table->part[PART_BAK].crc32 = image_crc;
		ptb_table->part[PART_BAK].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_BAK].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_BAK].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_BAK].ver_date = ambarella_flpart_date;
	}

	if (ambarella_ptb) {
		ptb_table->part[PART_PTB].img_len = image_length;
		ptb_table->part[PART_PTB].crc32 = image_crc;
		ptb_table->part[PART_PTB].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_PTB].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PTB].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PTB].ver_date = ambarella_flpart_date;
	}
	if (ambarella_bld) {
		ptb_table->part[PART_BLD].img_len = image_length;
		ptb_table->part[PART_BLD].crc32 = image_crc;
		ptb_table->part[PART_BLD].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_BLD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_BLD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_BLD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_pba) {
		ptb_table->part[PART_PBA].img_len = image_length;
		ptb_table->part[PART_PBA].crc32 = image_crc;
		ptb_table->part[PART_PBA].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_PBA].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_PBA].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_PBA].ver_date = ambarella_flpart_date;
	}
	if (ambarella_sec) {
		ptb_table->part[PART_SEC].img_len = image_length;
		ptb_table->part[PART_SEC].crc32 = image_crc;
		ptb_table->part[PART_SEC].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_SEC].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_SEC].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_SEC].ver_date = ambarella_flpart_date;
	}
	if (ambarella_dsp) {
		ptb_table->part[PART_DSP].img_len = image_length;
		ptb_table->part[PART_DSP].crc32 = image_crc;
		ptb_table->part[PART_DSP].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_DSP].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_DSP].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_DSP].ver_date = ambarella_flpart_date;
	}
	if (ambarella_lnx) {
		ptb_table->part[PART_LNX].img_len = image_length;
		ptb_table->part[PART_LNX].crc32 = image_crc;
		ptb_table->part[PART_LNX].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_LNX].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_LNX].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_LNX].ver_date = ambarella_flpart_date;
	}
	if (ambarella_swp) {
		ptb_table->part[PART_SWP].img_len = image_length;
		ptb_table->part[PART_SWP].crc32 = image_crc;
		ptb_table->part[PART_SWP].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_SWP].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_SWP].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_SWP].ver_date = ambarella_flpart_date;
	}
	if (ambarella_add) {
		ptb_table->part[PART_ADD].img_len = image_length;
		ptb_table->part[PART_ADD].crc32 = image_crc;
		ptb_table->part[PART_ADD].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_ADD].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ADD].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ADD].ver_date = ambarella_flpart_date;
	}
	if (ambarella_adc) {
		ptb_table->part[PART_ADC].img_len = image_length;
		ptb_table->part[PART_ADC].crc32 = image_crc;
		ptb_table->part[PART_ADC].flag = ambarella_flpart_flag;
//		ptb_table->part[PART_ADC].mem_addr = ambarella_flpart_mem_addr;
		ptb_table->part[PART_ADC].ver_num = ambarella_flpart_version;
		ptb_table->part[PART_ADC].ver_date = ambarella_flpart_date;
	}
#if 0
	if (ambarella_force_ptb) {
		ptb_table->dev.pba_boot_flag[0] = ambarella_ptb_data;
	}
#endif
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
#if 0
    if(ambarella_set_upgrade_status){
		ptb_table->dev.pba_boot_flag[1] = ambarella_upgrade_status_data;
	}
#endif
	if (ambarella_bld ||ambarella_force_ptb||
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

