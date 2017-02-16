/**
 * app/ipcam/fastboot_smart3a/smart3a_adc.c
 *
 * Author: Roy Su <qiangsu@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <linux/rtc.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <getopt.h>
#include "config.h"
#include "../../utility/upgrade_partition/upgrade_partition.h"
#include "../../../amboot/include/adc.h"
#include "../../../include/arch_s2l/ambas_imgproc_arch.h"
#include "../../../include/arch_s2l/ambas_imgproc_ioctl_arch.h"
#include "iav_common.h"
#include "iav_vin_ioctl.h"

#define PRELOAD_AWB_FILE	"/tmp/awb"
#define PRELOAD_AE_FILE	"/tmp/ae"
#define PRELOAD_VIDEOPARAM_FILE	"/tmp/video_param.conf"

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
	const char *arg;
	const char *str;
};

enum idsp_cfg_select_policy {
	IDSP_CFG_SELECT_ONLY_ONE = 0,
	IDSP_CFG_SELECT_VIA_UTC_HOUR,
	IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS,
};

#define IDSPCFG_BINARY_HEAD_SIZE	(64)
#define UPDATED_ADC_IMAGE_PATH	"/tmp/updated_adc.bin"

static int select_read_3a = 1;
static int verbose = 0;

static const char *short_options = "rwv";

static struct option long_options[] = {
	{"read",	NO_ARG, 0, 'r'},
	{"write",	NO_ARG, 0, 'w'},
	{"verbose", NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"", "Read out the 3A config from ADC (At the begin of Linux)"},
	{"", "Write 3A config into ADC which from 3A process (In the end of Linux)"},
	{"",	"print debug info"},
};

static void usage(void)
{
	u32 i;
	char *itself = "smart_3a";
	printf("This program used to read/write 3A config from/into ADC partition\n");
	printf("\n");
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

	printf("Example:\n\tRead # %s -r\n", itself);
	printf("\tWrite # %s -w -v\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'r':
			select_read_3a = 1;
			break;
		case 'w':
			select_read_3a = 0;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

uint32_t idspcfg_dump_bin(int fd_iav, uint8_t *bin_buffer)
{
	idsp_config_info_t	dump_idsp_info;
	int rval = -1;

	dump_idsp_info.id_section = 0;
	dump_idsp_info.addr = bin_buffer;

	rval = ioctl(fd_iav, IAV_IOC_IMG_DUMP_IDSP_SEC, &dump_idsp_info);
	if (rval < 0) {
		perror("IAV_IOC_IMG_DUMP_IDSP_SEC");
		return 0;
	}
	printf("idspcfg size=%u\n", dump_idsp_info.addr_long);
	return dump_idsp_info.addr_long;
}

static void rtc_read_tm(struct tm *ptm, int fd)
{
	memset(ptm, 0, sizeof(*ptm));
	ioctl(fd, RTC_RD_TIME, ptm);
	ptm->tm_isdst = -1; /* "not known" */
}

static int select_idsp_cfg_via_rtc_hour()
{
	struct tm tm_time;
	time_t timer;
	struct tm s_tblock = {0};
	struct tm *tblock = NULL;
	int fd = 0;
	char *oldtz = NULL;

	//get RTC time
	fd = open("/dev/rtc", O_RDONLY);
	if (fd < 0) {
		fd = open("/dev/rtc0", O_RDONLY);
		if (fd < 0) {
			fd = open("/dev/misc/rtc", O_RDONLY);
			if (fd < 0) {
				printf("Open RTC failed\n");
				return -1;
			}
		}
	}
	rtc_read_tm(&tm_time, fd);
	close(fd);

	//save old local time zone
	oldtz = getenv("TZ");

	//set local time zone to UTC
	putenv((char*)"TZ=UTC0");
	tzset();

	//get local time(now is UTC)
	timer = mktime(&tm_time);

	//parse local time(now is UTC)
	localtime_r(&timer, &s_tblock);

	//restore old local time zone
	unsetenv("TZ");
	if (oldtz) {
		putenv(oldtz - 3);
	}
	tzset();

	tblock = &s_tblock;
	if (tblock->tm_hour < 0 || tblock->tm_hour >= 24) {
		printf("RTC hour is invalid\n");
		return -1;
	} else {
		return tblock->tm_hour;
	}
}

static int select_idsp_cfg(enum idsp_cfg_select_policy policy)
{
	switch (policy) {
	case IDSP_CFG_SELECT_ONLY_ONE:
		return 0;
		break;

	case IDSP_CFG_SELECT_VIA_UTC_HOUR:
		return select_idsp_cfg_via_rtc_hour();
		break;

	case IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS:
		printf("Policy BRIGHTNESS not supported yet\n");
		return -1;
		break;

	default:
		printf("Invalid policy %d\n", policy);
		return -1;
		break;
	}
}

static int find_mtd_device_path(const char *dev_name, char *dev_info_buf,
	int dev_info_bufsize)
{
	FILE   *stream = 0;
	int device_index = -1;

	if (!dev_name || !dev_info_buf || 0 == dev_info_bufsize) {
		printf("Find mtd device path, NULL input\n");
		return -1;
	}
	memset(dev_info_buf, 0, dev_info_bufsize);

	//get mtd device index and path
	sprintf(dev_info_buf, "cat /proc/mtd | grep %s | cut -d':' -f1 | cut -d'd' -f2", dev_name);
	stream = popen(dev_info_buf , "r" );
	if (NULL == stream) {
		printf("Open /proc/mtd  %s  failed.\n", dev_name);
		return -1;
	}
	fscanf(stream,"%d", &device_index);
	pclose(stream);

	if (device_index < 0) {
		printf("Not found %s partition on /proc/mtd\n", dev_name);
		return -1;
	}
	memset(dev_info_buf, 0, dev_info_bufsize);
	sprintf(dev_info_buf, "/dev/mtd%d", device_index);

	return 0;
}

static int get_reg_data(const uint16_t reg_addr, int fd_iav)
{
	struct vindev_reg reg;
	reg.vsrc_id = 0;
	reg.addr = reg_addr;
	int rval = ioctl(fd_iav, IAV_IOC_VIN_GET_REG, &reg);
	if (rval < 0) {
	    printf("addr 0x%04x, read fail\n", reg_addr);
	    return -1;
	}

	return reg.data;
}

static int load_adc_to_mem (uint32_t *adc_img_len,
	uint32_t *adc_img_aligned_len, void **pp_adc_aligned_mem)
{
	char dev_info_buf[128];
	int ret = 0, count = 0;
	int ptb_fd = 0, ptb_offset;
	uint8_t *ptb_buf = NULL;
	struct mtd_info_user ptb_meminfo;
	loff_t ptb_bad_offset;
	flpart_table_t *table;
	unsigned long long blockstart = 1;
	int fd = 0, bs, badblock = 0;
	struct mtd_info_user meminfo;
	unsigned long ofs;
	void *p_adc_mem_cur = NULL;

	if (!adc_img_len || !adc_img_aligned_len || !pp_adc_aligned_mem) {
		printf("Load ADC to mem, NULL input\n");
		ret = -1;
		goto closeall;
	}

	ret = find_mtd_device_path("ptb", dev_info_buf, sizeof(dev_info_buf));
	if (ret < 0) {
		printf("Find ptb partition failed\n");
		ret = -1;
		goto closeall;
	}

	/* Open the PTB device */
	if ((ptb_fd = open(dev_info_buf, O_RDONLY)) == -1) {
		perror("open PTB");
		ret = -1;
		goto closeall;
	}

	/* Fill in MTD device capability structure */
	if ((ret = ioctl(ptb_fd, MEMGETINFO, &ptb_meminfo)) != 0) {
		perror("PTB MEMGETINFO");
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
			break;
		}
	}
	if (ptb_offset >= ptb_meminfo.size) {
		printf("Can not find good block in PTB.\n");
		ret = -1;
		goto closeall;
	}

	ptb_buf = malloc(ptb_meminfo.erasesize);
	memset(ptb_buf, 0, ptb_meminfo.erasesize);

	/* Read partition table.
	* Note: we need to read and save the entire block data, because the
	* entire block will be erased when write partition table back to flash.
	* BTW, flpart_meta_t is located in the same block as flpart_table_t
	*/
	count = ptb_meminfo.erasesize;
	if (pread(ptb_fd, ptb_buf, count, ptb_offset) != count) {
		perror("pread PTB");
		ret = -1;
		goto closeall;
	}

	table = PTB_TABLE(ptb_buf);
	*adc_img_len = table->part[PART_ADC].img_len;

	ret = find_mtd_device_path("adc", dev_info_buf, sizeof(dev_info_buf));
	if (ret < 0) {
		printf("Find ADC partition failed\n");
		ret = -1;
		goto closeall;
	}

	/* Open ADC device */
	if ((fd = open(dev_info_buf, O_RDONLY)) == -1) {
		perror("open mtd");
		ret = -1;
		goto closeall;
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		ret = -1;
		goto closeall;
	}

	bs = meminfo.writesize;
	*adc_img_aligned_len = ((*adc_img_len)+(bs-1))&(~(bs-1));

	if (verbose) {
		printf("ADC img addr=%u, img len=%u, img aligned_len=%u\n",
			table->part[PART_ADC].mem_addr, *adc_img_len, *adc_img_aligned_len);
	}

	//will be freed when process return
	*pp_adc_aligned_mem = (void*)malloc(*adc_img_aligned_len);
	if (!(*pp_adc_aligned_mem)) {
		printf("Can not malloc memory for load ADC partiton!\n");
		ret = -1;
		goto closeall;
	}
	p_adc_mem_cur = *pp_adc_aligned_mem;

	if (verbose) {
		/* Print informative message */
		printf("Total size %u, Block size %u, page size %u, OOB size %u\n",
			meminfo.size,meminfo.erasesize, meminfo.writesize, meminfo.oobsize);
		printf("Loading data starting at 0x%08x and ending at 0x%08x\n",
			table->part[PART_ADC].mem_addr,
			table->part[PART_ADC].mem_addr+(*adc_img_aligned_len));
	}

	/* Load the flash contents */
	for (ofs = 0; ofs < *adc_img_aligned_len ; ofs+=bs) {
		// new eraseblock , check for bad block
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);
			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				perror("ioctl(MEMGETBADBLOCK)");
				ret = -1;
				goto closeall;
			}
		}

		if (badblock) {
				//memset (p_adc_mem_cur, 0xff, bs);
				continue;
		} else {
			/* Read page data and exit on failure */
			if (pread(fd, p_adc_mem_cur, bs, ofs) != bs) {
				perror("pread");
				ret = -1;
				goto closeall;
			}
		}
		p_adc_mem_cur+=bs;
	}

	/* Exit happy */
	ret = 0;

	closeall:
	if (ptb_buf) {
		free(ptb_buf);
		ptb_buf  = NULL;
	}

	if (ptb_fd) {
		close(ptb_fd);
		ptb_fd = 0;
	}
	if (fd) {
		close(fd);
		fd = 0;
	}

	return ret;
}

static int update_adc_partition()
{
	char cmd[256];
	FILE   *stream = 0;
	int device_index = -1;

	memset(cmd, 0, sizeof(cmd));

	stream = popen( "cat /proc/mtd | grep adc | cut -d':' -f1 | cut -d'd' -f2", "r" );
	if (NULL == stream) {
		printf("Open /proc/mtd failed\n");
		return -1;
	}
	fscanf(stream, "%d", &device_index);
	pclose(stream);
	if (device_index < 0) {
		printf("Not found ADC partition on /proc/mtd\n");
		return -1;
	}
	//erase nand flash before re-write
	sprintf(cmd, "flash_eraseall /dev/mtd%d", device_index);

	if (verbose) {
		printf("CMD: %s\n", cmd);
	}
	system(cmd);

	sprintf(cmd, "upgrade_partition /dev/mtd%d %s", device_index, UPDATED_ADC_IMAGE_PATH);

	if (verbose) {
		printf("CMD: %s\n", cmd);
	}
	system(cmd);

	return 0;
}

static int save_content_file(char *filename, char *content)
{
	FILE *fp;
	int ret = -1;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		perror("fopen");
		return ret;
	}

	ret = fwrite(content, strlen(content), 1, fp);
	fclose(fp);
	fp = NULL;

	return ret;
}

static void read_awb_ae_config(struct smart3a_file_info *info)
{
	FILE *fp_ae;
	FILE *fp_awb;
	char content[64];
	char *content_c = NULL;

	fp_awb = fopen(PRELOAD_AWB_FILE, "r");
	if (fp_awb != NULL) {
		fseek(fp_awb, 0, SEEK_SET);
		memset(content, 0, sizeof(content));
		fread(content, sizeof(content) + 1, 1, fp_awb);
		fclose(fp_awb);
		fp_awb = NULL;

		content_c = strtok(content, ",");
		if (content_c) {
			info->r_gain = atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			info->b_gain = atoi(content_c);
		}
	}

	fp_ae = fopen(PRELOAD_AE_FILE, "r");
	if (fp_ae != NULL) {
		fseek(fp_ae, 0, SEEK_SET);
		memset(content, 0, sizeof(content));
		fread(content, sizeof(content) + 1, 1, fp_ae);
		fclose(fp_ae);
		fp_ae = NULL;

		content_c = strtok(content, ",");
		if (content_c) {
			info->d_gain= atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			info->shutter = atoi(content_c);
		}
		content_c = strtok(NULL, ",");
		if (content_c) {
			info->agc = atoi(content_c);
		}
	}

}

/* ==========================================================================*/
int main(int argc, char **argv)
{
	int rval = 0;
	struct adcfw_header *hdr;
	uint32_t addr = 0, size = 0;
	enum idsp_cfg_select_policy policy = IDSP_CFG_SELECT_ONLY_ONE;
	int idsp_cfg_index = -1;
	int fd_iav = 0;
	uint8_t *bin_buffer = NULL;
	uint32_t idsp_bin_size = 0;
	FILE *f_updated_adc = NULL;
	uint32_t adc_img_len = 0;	/**< Lengh of image in the partition */
	uint32_t adc_img_aligned_len = 0;//aligned to page size of adc parttiton
	void *p_adc_aligned_mem = NULL;//aligned to page size of adc parttiton

	char aaa_content[32];

	struct timeval time1 = { 0, 0 };
	struct timeval time2 = { 0, 0 };
	struct timeval time3 = { 0, 0 };
	struct timeval time4 = { 0, 0 };
	struct timeval time5 = { 0, 0 };
	struct timeval time6 = { 0, 0 };
	struct timeval time7 = { 0, 0 };
	struct timeval time8 = { 0, 0 };
	signed long long  time1_US = 0;
	signed long long  time2_US = 0;
	signed long long  time3_US = 0;
	signed long long  time4_US = 0;
	signed long long  time5_US = 0;
	signed long long  time6_US = 0;
	signed long long  time7_US = 0;
	signed long long  time8_US = 0;

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if (verbose) {
		gettimeofday(&time1, NULL);
	}

	//step 1: load whole adc partition to mem
	rval = load_adc_to_mem (&adc_img_len, &adc_img_aligned_len, &p_adc_aligned_mem);
	if (rval < 0) {
		printf("Load ADC to memory failed\n");
		goto main_exit;
	}

	if (verbose) {
		gettimeofday(&time2, NULL);
	}

	//step 2: find the mem address of the selected section among adc image
	hdr = (struct adcfw_header *)p_adc_aligned_mem;
	if (hdr->magic != ADCFW_IMG_MAGIC || hdr->fw_size != adc_img_len) {
		printf("Invalid ADC partition, magic=%u(should be %u), fw_size=%u(should be %u)\n",
		hdr->magic, ADCFW_IMG_MAGIC, hdr->fw_size, adc_img_len);
		goto main_exit;
	}
	idsp_cfg_index = select_idsp_cfg(policy);
	if (idsp_cfg_index < 0 || idsp_cfg_index >= hdr->smart3a_num) {
		printf("Wrong idspcfg section via policy 0x%x, idspcfg index=%d, should in [0, %u)\n",
		policy, idsp_cfg_index, hdr->smart3a_num);
		goto main_exit;
	}
	addr = (uint32_t)hdr + hdr->smart3a[idsp_cfg_index].offset;
	size = hdr->smart3a_size;

	if (size <= IDSPCFG_BINARY_HEAD_SIZE || 0 == addr) {
		printf("Invalid idspcfg section %d, addr=%u, size=%u!\n", idsp_cfg_index, addr, size);
		goto main_exit;
	}

	if (select_read_3a) {
		printf("ADC 3A info: r_gain=%d, b_gain=%d. d_gain=%d, shutter=%d, agc=%d.\n",
			hdr->smart3a[idsp_cfg_index].r_gain, hdr->smart3a[idsp_cfg_index].b_gain,
			hdr->smart3a[idsp_cfg_index].d_gain, hdr->smart3a[idsp_cfg_index].shutter,
			hdr->smart3a[idsp_cfg_index].agc);

		memset(aaa_content, 0, sizeof(aaa_content));
		snprintf(aaa_content, sizeof(aaa_content), "%d,%d\n",
			hdr->smart3a[idsp_cfg_index].r_gain,
			hdr->smart3a[idsp_cfg_index].b_gain);
		save_content_file(PRELOAD_AWB_FILE, aaa_content);

		memset(aaa_content, 0, sizeof(aaa_content));
		snprintf(aaa_content, sizeof(aaa_content), "%d,%d,%d\n",
			hdr->smart3a[idsp_cfg_index].d_gain,
			hdr->smart3a[idsp_cfg_index].shutter,
			hdr->smart3a[idsp_cfg_index].agc);
		save_content_file(PRELOAD_AE_FILE, aaa_content);

              do{
                  FILE *fp = fopen(PRELOAD_VIDEOPARAM_FILE, "wb");
                  if (fp) {
                      fprintf(fp,"enable_video_param=%d\n", hdr->video_param.enable_video_param);
                      fprintf(fp,"mode=%d\n", hdr->video_param.video_mode);
                      fprintf(fp,"bitrate_quality=%d\n",hdr->video_param.bitrate_quality);
                      fprintf(fp,"enable_smartavc=%d\n",hdr->video_param.res);
                      fclose(fp);
                  }
              }while(0);

		goto main_exit;
	}

	if (verbose) {
		gettimeofday(&time3, NULL);
	}

	//step 3: dump 3a date to mem
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		goto main_exit;
	}

	bin_buffer = (uint8_t*)malloc(MAX_DUMP_BUFFER_SIZE);
	if (bin_buffer == NULL) {
		printf("Can not malloc idspcfg buffer\n");
		goto main_exit;
	}

	memset(bin_buffer, 0, MAX_DUMP_BUFFER_SIZE);
	idsp_bin_size = idspcfg_dump_bin(fd_iav, bin_buffer);
	if (size != idsp_bin_size) {
		printf("idspcfg dump bin size[%u] invalid, should be %u as saved in adcfw_header\n",
			idsp_bin_size, size);
		goto main_exit;
	}

	if (verbose) {
		gettimeofday(&time4, NULL);
	}

	//step 4: save 3a data to selected section of adc image
	memcpy((void *)addr, (void *)bin_buffer, size);//TODO:  how to check success
	//printf("Update idsp cfg section %d done!\n", idsp_cfg_index);

	if (verbose) {
		gettimeofday(&time5, NULL);
	}

	//step 5: dump and save shutter and gain params to selected section of adc image
	read_awb_ae_config(&(hdr->smart3a[idsp_cfg_index]));
#if defined(CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L55M)
	int value = get_reg_data(0x3500, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para0 = value;
	}
	value = get_reg_data(0x3501, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para1 = value;
	}
	value = get_reg_data(0x3502, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para2 = value;
	}
	value = get_reg_data(0x3508, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para3 = value;
	}
	value = get_reg_data(0x3509, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para4 = value;
	}
	printf("Read sensor OV4689 register: shutter3500=0x%04x, shutter3501=0x%04x, shutter3502=0x%04x," \
	    "  gain3508=0x%04x, gain3509=0x%04x\n",
	    hdr->smart3a[idsp_cfg_index].para0,
	    hdr->smart3a[idsp_cfg_index].para1,
	    hdr->smart3a[idsp_cfg_index].para2,
	    hdr->smart3a[idsp_cfg_index].para3,
	    hdr->smart3a[idsp_cfg_index].para4);
#elif defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L55M)
	int value = get_reg_data(0x0202, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para0 = value;
	}
	value = get_reg_data(0x0203, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para1 = value;
	}
	value = get_reg_data(0x301E, fd_iav);
	if (value >= 0) {
	    hdr->smart3a[idsp_cfg_index].para2 = value;
	}
	printf("Read sensor IMX322 register: shutter0202=0x%04x, shutter0203=0x%04x, gain301E=0x%04x\n",
	    hdr->smart3a[idsp_cfg_index].para0,
	    hdr->smart3a[idsp_cfg_index].para1,
	    hdr->smart3a[idsp_cfg_index].para2);
#else
	int value = get_reg_data(0x3500, fd_iav);
	if (value >= 0) {
		hdr->smart3a[idsp_cfg_index].para0 = value;
	}
	value = get_reg_data(0x3501, fd_iav);
	if (value >= 0) {
		hdr->smart3a[idsp_cfg_index].para1 = value;
	}
	value = get_reg_data(0x3502, fd_iav);
	if (value >= 0) {
		hdr->smart3a[idsp_cfg_index].para2 = value;
	}
	value = get_reg_data(0x3508, fd_iav);
	if (value >= 0) {
		hdr->smart3a[idsp_cfg_index].para3 = value;
	}
	value = get_reg_data(0x3509, fd_iav);
	if (value >= 0) {
		hdr->smart3a[idsp_cfg_index].para4 = value;
	}
	printf("Read sensor OV4689 register: shutter3500=0x%04x, shutter3501=0x%04x, shutter3502=0x%04x," \
	"  gain3508=0x%04x, gain3509=0x%04x\n",
		hdr->smart3a[idsp_cfg_index].para0,
		hdr->smart3a[idsp_cfg_index].para1,
		hdr->smart3a[idsp_cfg_index].para2,
		hdr->smart3a[idsp_cfg_index].para3,
		hdr->smart3a[idsp_cfg_index].para4);
#endif
	printf("Read AAA param: r_gain=%d, b_gain=%d, dgain=%d, shutter=%d, agc=%d\n",
		hdr->smart3a[idsp_cfg_index].r_gain,
		hdr->smart3a[idsp_cfg_index].b_gain,
		hdr->smart3a[idsp_cfg_index].d_gain,
		hdr->smart3a[idsp_cfg_index].shutter,
		hdr->smart3a[idsp_cfg_index].agc);

	if (verbose) {
		gettimeofday(&time6, NULL);
	}

	//step 6: save the whole updated adc image to file
	f_updated_adc = fopen(UPDATED_ADC_IMAGE_PATH, "wb");
	if (f_updated_adc == NULL) {
		printf("Open /tmp/updated_adc.bin failed\n");
		rval = -EINVAL;
		goto main_exit;
	}
	rval = fwrite(p_adc_aligned_mem, 1, adc_img_aligned_len, f_updated_adc);
	if (rval != adc_img_aligned_len) {
		printf("/tmp/updated_adc.bin size [%d] is wrong, should be %u\n", rval, adc_img_aligned_len);
		goto main_exit;
	}

	if (verbose) {
		gettimeofday(&time7, NULL);
	}

	//step 7: update adc partition with adc image file
	rval = update_adc_partition();
	if (0 != rval) {
		printf("Update adc partition failed\n");
		goto main_exit;
	}

	if (verbose) {
		//printf("[IMPORT3A]: export Updated idsp cfg image to /tmp/updated_adc.bin done!\n");
		gettimeofday(&time8, NULL);
		time1_US = ((signed long long)(time1.tv_sec) * 1000000 + (signed long long)(time1.tv_usec));
		time2_US = ((signed long long)(time2.tv_sec) * 1000000 + (signed long long)(time2.tv_usec));
		time3_US = ((signed long long)(time3.tv_sec) * 1000000 + (signed long long)(time3.tv_usec));
		time4_US = ((signed long long)(time4.tv_sec) * 1000000 + (signed long long)(time4.tv_usec));
		time5_US = ((signed long long)(time5.tv_sec) * 1000000 + (signed long long)(time5.tv_usec));
		time6_US = ((signed long long)(time6.tv_sec) * 1000000 + (signed long long)(time6.tv_usec));
		time7_US = ((signed long long)(time7.tv_sec) * 1000000 + (signed long long)(time7.tv_usec));
		time8_US = ((signed long long)(time8.tv_sec) * 1000000 + (signed long long)(time8.tv_usec));
		printf("[TIME]: <Load ADC partition to image> cost %lld US!\n", time2_US-time1_US);
		printf("[TIME]: <Find selected section of ADC image> cost %lld US!\n", time3_US-time2_US);
		printf("[TIME]: <Dump 3A date to mem> cost %lld US!\n", time4_US-time3_US);
		printf("[TIME]: <Copy 3A mem to selected section of ADC image> cost %lld US!\n", time5_US-time4_US);
		printf("[TIME]: <Dump&save shutter and gain params to selected section of ADC image> cost %lld US!\n", time6_US-time5_US);
		printf("[TIME]: <Save the updated ADC image to file> cost %lld US!\n", time7_US-time6_US);
		printf("[TIME]: <Update ADC partition from file> cost %lld US!\n", time8_US-time7_US);
	}

main_exit:
	if (bin_buffer != NULL) {
		free(bin_buffer);
		bin_buffer = NULL;
	}
	if (p_adc_aligned_mem) {
		free(p_adc_aligned_mem);
		p_adc_aligned_mem  = NULL;
	}
	if (fd_iav != 0) {
		close(fd_iav);
		fd_iav = 0;
	}
	if (f_updated_adc) {
		fclose(f_updated_adc);
		f_updated_adc = NULL;
	}

	return rval;
}

