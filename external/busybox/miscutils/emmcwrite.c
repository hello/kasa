//config:config EMMCWRITE
//config:	bool "emmcwrite"
//config:	default y
//config:	select PLATFORM_LINUX
//config:	help
//config:	  Write to the specified emmc device

//applet:IF_EMMCWRITE(APPLET(emmcwrite, BB_DIR_USR_SBIN, BB_SUID_DROP))
//kbuild:lib-$(CONFIG_EMMCWRITE) += emmcwrite.o

//usage:#define emmcwrite_trivial_usage
//usage:	"[OPTIONS] EMMC-device"
//usage:#define emmcwrite_full_usage "\n\n"
//usage:	"Write to the specified emmc device\n"

#include "libbb.h"
#include <getopt.h>

#include "../../../ambarella/include/basetypes.h"
#include "../../../ambarella/kernel/linux/arch/arm/plat-ambarella/include/plat/ptb.h"

typedef enum {
	AMB_PTN_BST	= 1,
	AMB_PTN_PTB,
	AMB_PTN_BLD,
	AMB_PTN_HAL,
	AMB_PTN_PBA,
	AMB_PTN_PRI,
	AMB_PTN_SEC,
	AMB_PTN_BAK,
	AMB_PTN_RMD,
	AMB_PTN_ROM,
	AMB_PTN_DSP,
	AMB_PTN_LNX,
	AMB_PTN_SWP,
	AMB_PTN_ADD,
	AMB_PTN_ADC,
	AMB_PTN_STG2,

	AMB_PTN_MAX,
} amb_ptn_t;

#define	MAX_STR_LEN	128

#define COPY_BLOCK_SIZE	(512 * 1024)

typedef struct {
	char		node[MAX_STR_LEN];
	char		image[MAX_STR_LEN];
} amb_ptn_info_t;

amb_ptn_info_t		g_ptn_info[AMB_PTN_MAX] = {0};
char			*g_buf;
flpart_table_t		g_ptb;

static const char *short_options = "A:B:C:D:E:F:G:H:I:J:K:L:M:N:O:P:";

static struct option long_options[] = {
	{"bst",		1,	0,	'A'},
	{"ptb",		1,	0,	'B'},
	{"bld",		1,	0,	'C'},
	{"hal",		1,	0,	'D'},
	{"pba",		1,	0,	'E'},
	{"pri",		1,	0,	'F'},
	{"sec",		1,	0,	'G'},
	{"bak",		1,	0,	'H'},
	{"rmd",		1,	0,	'I'},
	{"rom",		1,	0,	'J'},
	{"dsp",		1,	0,	'K'},
	{"lnx",		1,	0,	'L'},
	{"swp",		1,	0,	'M'},
	{"add",		1,	0,	'N'},
	{"adc",		1,	0,	'O'},
	{"stg2",	1,	0,	'P'},

	{0,		0,	0,	 0 },
};

void usage()
{
	printf("Usage: emmcwrite [-A bst.img] [-B ptb.img] [-C bld.img]\r\n"
		"		 [-D hal.img] [-E pba.img] [-F pri.img]\r\n"
		"		 [-G sec.img] [-H bak.img] [-I rmd.img]\r\n"
		"		 [-J rom.img] [-K dsp.img] [-L lnx.img]\r\n"
		"		 [-M swp.img] [-N add.img] [-O adc.img]\r\n"
		"		 [-P stg2.img]\r\n");
}

int init_param(int argc, char **argv)
{
	int	c, id;

	while (1) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c < 0) {
			break;
		}

		switch (c) {
		case 'A':
			strcpy(g_ptn_info[AMB_PTN_BST].node, "/dev/mmcblk0boot0");
			strcpy(g_ptn_info[AMB_PTN_BST].image, optarg);
			break;

		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
			id = c - 'A' + AMB_PTN_BST;
			sprintf(g_ptn_info[id].node, "/dev/mmcblk0p%d", id);
			strcpy(g_ptn_info[id].image, optarg);
			break;

		default:
			printf("Unknown parameter: %c\n", c);
			return -1;

		}
	}

	return 0;
}

int sys_write(const char *fn, const char *v)
{
	int		ret = 0;
	FILE		*d;

	d = fopen(fn, "w");
	if (!d) {
		printf("Unable to fopen %s\n", fn);
		return -1;
	}

	fprintf(d, "%s", v);

	fclose(d);

	return ret;
}

int read_ptb(void)
{
	int		ret = 0;
	FILE		*d;

	d = fopen("/dev/mmcblk0p2", "rb");
	if (!d) {
		printf("Unable to fopen ptb\n");
		return -1;
	}

	fread(&g_ptb, 1, sizeof(g_ptb), d);

	fclose(d);

	return ret;
}

int write_ptb(void)
{
	int		ret = 0;
	FILE		*d;

	d = fopen("/dev/mmcblk0p2", "wb");
	if (!d) {
		printf("Unable to fopen ptb\n");
		return -1;
	}

	fwrite(&g_ptb, 1, sizeof(g_ptb), d);

	fclose(d);

	return ret;
}

int update_partitions(void)
{
	int		ret = 0, len, l;
	amb_ptn_t	p;
	FILE		*d, *i;
	int		crc;
	const u32	*table;

	g_buf = malloc(COPY_BLOCK_SIZE);
	if (!g_buf) {
		printf("Unable to allocate memory!\n");
	}

	table = crc32_filltable(NULL, 0);

	if (strlen(g_ptn_info[AMB_PTN_BST].node) &&
		strlen(g_ptn_info[AMB_PTN_BST].image)) {

		ret = sys_write("/sys/block/mmcblk0boot0/force_ro", "0");
		if (ret < 0) {
			goto update_partitions_exit;
		}

		d = fopen(g_ptn_info[AMB_PTN_BST].node, "wb");
		if (!d) {
			printf("Unable to fopen %s\n", g_ptn_info[AMB_PTN_BST].node);
			ret = -1;
			goto update_partitions_exit;
		}

		i = fopen(g_ptn_info[AMB_PTN_BST].image, "rb");
		if (!i) {
			printf("Unable to fopen %s\n", g_ptn_info[AMB_PTN_BST].image);
			ret = -1;
			goto update_partitions_exit;
		}

		len	= 0;
		crc	= ~0U;

		while (!feof(i)) {
			l = fread(g_buf, 1, COPY_BLOCK_SIZE, i);
			if (l > 0) {
				fwrite(g_buf, 1, l, d);
				crc = crc32_block_endian0(crc, g_buf, l, table);
				len += l;
			}
		}

		g_ptb.part[AMB_PTN_BST - 1].crc32	= crc;
		g_ptb.part[AMB_PTN_BST - 1].img_len	= len;

		fclose(d);
		fclose(i);

		ret = sys_write("/sys/block/mmcblk0boot0/force_ro", "1");
		if (ret < 0) {
			goto update_partitions_exit;
		}
	}

	for (p = AMB_PTN_PTB; p < AMB_PTN_MAX; p++) {

		if (!strlen(g_ptn_info[p].node)) {
			continue;
		}
		if (!strlen(g_ptn_info[p].image)) {
			continue;
		}

		d = fopen(g_ptn_info[p].node, "wb");
		if (!d) {
			printf("Unable to fopen %s\n", g_ptn_info[p].node);
			ret = -1;
			break;
		}

		i = fopen(g_ptn_info[p].image, "rb");
		if (!i) {
			printf("Unable to fopen %s\n", g_ptn_info[p].image);
			ret = -1;
			break;
		}

		len	= 0;
		crc	= ~0U;

		while (!feof(i)) {
			l = fread(g_buf, 1, COPY_BLOCK_SIZE, i);
			if (l > 0) {
				fwrite(g_buf, 1, l, d);
				crc = crc32_block_endian0(crc, g_buf, l, table);
				len += l;
			}
		}

		g_ptb.part[p - 1].crc32		= crc;
		g_ptb.part[p - 1].img_len	= len;

		fclose(d);
		fclose(i);
	}

update_partitions_exit:

	if (g_buf) {
		free(g_buf);
	}

	return ret;
}

int emmcwrite_main(int argc, char **argv)
{
	int		ret = 0;

	if (argc < 2) {
		usage();
	}

	ret = init_param(argc, argv);
	if (ret < 0) {
		usage();
		return ret;
	}

	ret = read_ptb();
	if (ret < 0) {
		return ret;
	}

	ret = update_partitions();
	if (ret < 0) {
		return ret;
	}

	ret = write_ptb();
	if (ret < 0) {
		return ret;
	}

	return ret;
}

