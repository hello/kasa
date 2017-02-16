/*
 * load_ucode.c
 *
 * History:
 *	2008/1/25 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <libbb.h>
#include <mtd/mtd-user.h>

#include "../../../ambarella/build/include/basetypes.h"
#include "../../../ambarella/build/include/iav_drv.h"


/* DSP fw header definition, copied from amboot/include/fio/firmfl.h,
   may have bugs, further verification needed.
*/
typedef struct dspfw_header_s {
	/** CODE segment */
	struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} code;

	/** MEMD segment */
	struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} memd;

	/** Data segment */
	struct {
		u32	offset;
		u32	len;
		u32	crc32;
		u32	rsv;
	} data;

	u32	rsv[8];	/**< Padding */
} dspfw_header_t;

static int load_ucode_to_mem(int fd, int *bbtable, unsigned int blocks,
	unsigned int bs, void *dst, unsigned int offset, unsigned int length);
static int load_ucode_from_ramfs(const char *ucode_path);
static int load_ucode_from_dspmem(void);
static int load_ucode_from_flash(const char *mtd_device);
int load_ucode_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;


static int load_ucode_to_mem(int fd, int *bbtable, unsigned int blocks,
	unsigned int bs, void *dst, unsigned int offset, unsigned int length)
{
	int rval = 0;
	unsigned int block;
	unsigned int startblock, goodblock;
	unsigned int addr, off, len;

	startblock = (offset / bs) + 1;
	off = offset % bs;


	fprintf(stderr, "loading 0x%x of length 0x%x to 0x%.8x\n",
		offset, length, (unsigned int) dst);

	/* Goto offset */
	for (block = 0, addr = 0, goodblock = 0; block < blocks;
		block++, addr += bs) {
		if (bbtable[block] == 0) {
			goodblock++;
			if (goodblock >= startblock)
				break;
		}
	}
	if (block >= blocks) {
		printf("%s: Can't goto offset 0x%x!\n", __func__, offset);
		rval = -1;
		goto done;
	}

	/* Read data */
	while (length > 0) {
		if (block >= blocks) {
			printf("%s: read error!\n", __func__);
			rval = -1;
			goto done;
		}

		if (bbtable[block]) {
			block++;
			addr += bs;
			continue;
		}

		if (off == 0) {
			len = (length > bs) ? bs : length;
		} else {
			len = (length > (bs - off)) ? (bs - off) : length;
		}

		rval = pread(fd, dst, len, addr + off);
		if (rval != len) {
			perror("pread fd");
			goto done;
		}

		length -= len;
		dst = (void *) (((unsigned int) dst) + len);
		block++;
		addr += bs;
		off = 0;
	}

	rval = 0;

done:
	return rval;
}

static int load_ucode_from_ramfs(const char *ucode_path)
{
	int i;
	int fd;
	FILE *fp;
	ucode_load_info_t info;
	ucode_version_t version;
	u8 *ucode_mem;
	char filename[256];
	int file_length;

	if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		return -1;
	}

	printf("map_size = 0x%x\n", (u32)info.map_size);
	printf("nr_item = %d\n", info.nr_item);
	for (i = 0; i < info.nr_item; i++) {
		printf("addr_offset = 0x%x ", (u32)info.items[i].addr_offset);
		printf("filename = %s\n", info.items[i].filename);
	}

	ucode_mem = (u8 *)mmap(NULL, info.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((int)ucode_mem == -1) {
		perror("mmap");
		return -1;
	}
	printf("mmap returns 0x%x\n", (unsigned)ucode_mem);

	for (i = 0; i < info.nr_item; i++) {
		u8 *addr = ucode_mem + info.items[i].addr_offset;
		sprintf(filename, "%s/%s", ucode_path, info.items[i].filename);

		printf("loading %s...", filename);
		if ((fp = fopen(filename, "rb")) == NULL) {
			perror(filename);
			return -1;
		}

		if (fseek(fp, 0, SEEK_END) < 0) {
			perror("SEEK_END");
			return -1;
		}
		file_length = ftell(fp);
		if (fseek(fp, 0, SEEK_SET) < 0) {
			perror("SEEK_SET");
			return -1;
		}

		printf("addr = 0x%x, size = 0x%x\n", (u32)addr, file_length);

		if (fread(addr, 1, file_length, fp) != file_length) {
			perror("fread");
			return -1;
		}

		fclose(fp);
	}

	if (ioctl(fd, IAV_IOC_UPDATE_UCODE, 0) < 0) {
		perror("IAV_IOC_UPDATE_UCODE");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_VERSION, &version) < 0) {
		perror("IAV_IOC_GET_UCODE_VERSION");
		return -1;
	}

	printf("===============================\n");
	printf("u_code version = %d/%d/%d %d.%d\n",
		version.year, version.month, version.day,
		version.edition_num, version.edition_ver);
	printf("===============================\n");

//	if (munmap(ucode_mem, info.map_size) < 0)
//		perror("munmap");

//	close(fd);

	return 0;
}

static int load_ucode_from_dspmem(void)
{
	int i;
	int fd;
	ucode_load_info_t info;
	ucode_version_t version;
	u8 *ucode_mem;
	dspfw_header_t dsphdr;

	if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		return -1;
	}

	printf("map_size = 0x%x\n", (u32)info.map_size);
	printf("nr_item = %d\n", info.nr_item);
	for (i = 0; i < info.nr_item; i++) {
		printf("addr_offset = 0x%x ", (u32)info.items[i].addr_offset);
		printf("filename = %s\n", info.items[i].filename);
	}

	ucode_mem = (u8 *)mmap(NULL, info.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((int)ucode_mem == -1) {
		perror("mmap");
		return -1;
	}
	memcpy(&dsphdr, ucode_mem, sizeof(dsphdr));
	printf("mmap returns 0x%x\n", (unsigned)ucode_mem);

	for (i = info.nr_item - 1; i >= 0; i--) {
		u8 *src, *dst = ucode_mem + info.items[i].addr_offset;
		u32 len;

		if (strcmp(info.items[i].filename, "orccode.bin") == 0 ||
			strcmp(info.items[i].filename, "code.bin") == 0) {

			src = ucode_mem + dsphdr.code.offset;
			len = dsphdr.code.len;
			memcpy(dst, src, len);
		} else if (strcmp(info.items[i].filename, "orcme.bin") == 0 ||
			strcmp(info.items[i].filename, "memd.bin") == 0) {

			src = ucode_mem + dsphdr.memd.offset;
			len = dsphdr.memd.len;
			memcpy(dst, src, len);
		} else if (strcmp(info.items[i].filename, "default_binary.bin") == 0) {

			src = ucode_mem + dsphdr.data.offset;
			len = dsphdr.data.len;

			if (dst > src && dst < src + len) {
				u8 *tmp = (u8 *)malloc(len);
				if (!tmp) {
					printf("%s: Not enought memory!\n", __func__);
					return -1;
				}
				memcpy(tmp, src, len);
				memcpy(dst, tmp, len);
				free(tmp);
			} else {
				memcpy(dst, src, len);
			}
		}
	}

	if (ioctl(fd, IAV_IOC_UPDATE_UCODE, 0) < 0) {
		perror("IAV_IOC_UPDATE_UCODE");
		return -1;
	}

	if (ioctl(fd, IAV_IOC_GET_UCODE_VERSION, &version) < 0) {
		perror("IAV_IOC_GET_UCODE_VERSION");
		return -1;
	}

	printf("===============================\n");
	printf("u_code version = %d/%d/%d %d.%d\n",
		version.year, version.month, version.day,
		version.edition_num, version.edition_ver);
	printf("===============================\n");

//	if (munmap(ucode_mem, info.map_size) < 0)
//		perror("munmap");

//	close(fd);

	return 0;
}

static int load_ucode_from_flash(const char *mtd_device)
{
	int rval = 0;
	int fd = -1;
	int ud = -1;
	int *bbtable = NULL;
	mtd_info_t meminfo;
	unsigned char oobbuf[64];
	struct mtd_oob_buf oob = { 0, 16, oobbuf };
	struct mtd_ecc_stats __stat;
	unsigned int bs;
	ucode_load_info_t info;
	ucode_version_t version;
	unsigned int i, blocks, badblocks;
	unsigned long long addr;
	dspfw_header_t dsphdr;
	char *ucode_mem = NULL;

	/* Open MTD device */
	if ((fd = open(mtd_device, O_RDONLY)) == -1) {
		perror("open flash");
		rval = fd;
		goto done;
	}

	/* Fill in MTD device capability structure */
	rval = ioctl(fd, MEMGETINFO, &meminfo);
	if (rval < 0) {
		perror("MEMGETINFO");
		goto done;
	}

	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 64 && meminfo.writesize == 2048) &&
	    !(meminfo.oobsize == 32 && meminfo.writesize == 1024) &&
	    !(meminfo.oobsize == 16 && meminfo.writesize == 512) &&
	    !(meminfo.oobsize == 8 && meminfo.writesize == 256)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}

	/* Read the real oob length */
	oob.length = meminfo.oobsize;

	/* Check if we can read ecc stats */
	rval = ioctl(fd, ECCGETSTATS, &__stat);
	if (rval < 0) {
		perror("No ECC status information available");
		goto done;
	}

	/* Build BB Table */
	bs = meminfo.erasesize;
	blocks = meminfo.size / bs;
	bbtable = (int *) malloc(sizeof(int) * blocks);
	badblocks = 0;
	for (addr = i = 0; i < blocks; i++, addr += bs) {
		rval = ioctl(fd, MEMGETBADBLOCK, &addr);
		if (rval < 0) {
			perror("MEMGETBADBLOCK");
			goto done;
		}

		if (rval == 0) {
			bbtable[i] = 0;
		} else {
			bbtable[i] = 1;
			badblocks++;
		}
	}

	if (badblocks != __stat.badblocks) {
		fprintf(stderr, "Bad Block sanity check failed!\n");
		rval = -1;
		goto done;
	}

	/* Find the first good block where the header is stored */
	for (addr = i = 0; i < blocks; i++, addr += bs)
		if (bbtable[i] == 0)
			break;

	if (i >= blocks) {
		fprintf(stderr, "no good block found!\n");
		goto done;
	}

	rval = pread(fd, &dsphdr, sizeof(dsphdr), addr);
	if (rval < 0) {
		perror("pread");
		goto done;
	}

	ud = open("/dev/ucode", O_RDWR, 0);
	if (ud < 0) {
		perror("open ucode");
		goto done;
	}

	rval = ioctl(ud, IAV_IOC_GET_UCODE_INFO, &info);
	if (rval < 0) {
		perror("IAV_IOC_GET_UCODE_INFO");
		goto done;
	}

	ucode_mem = (char *) mmap(NULL, info.map_size,
				  PROT_READ | PROT_WRITE, MAP_SHARED, ud, 0);
	if (((int) ucode_mem) == -1) {
		perror("mmap");
		rval = -1;
		goto done;
	}

	for (i = 0; i < info.nr_item; i++) {
		if (strcmp(info.items[i].filename, "orccode.bin") == 0 ||
			strcmp(info.items[i].filename, "code.bin") == 0) {
			rval = load_ucode_to_mem(fd, bbtable, blocks, bs,
						 ucode_mem +
						 info.items[i].addr_offset,
						 dsphdr.code.offset,
						 dsphdr.code.len);
			if (rval < 0) {
				fprintf(stderr, "load %s failed!\n", info.items[i].filename);
			}
		} else if (strcmp(info.items[i].filename, "orcme.bin") == 0 ||
			strcmp(info.items[i].filename, "memd.bin") == 0) {
			rval = load_ucode_to_mem(fd, bbtable, blocks, bs,
						 ucode_mem +
						 info.items[i].addr_offset,
						 dsphdr.memd.offset,
						 dsphdr.memd.len);
			if (rval < 0) {
				fprintf(stderr, "load %s failed!\n", info.items[i].filename);
			}
		} else if (strcmp(info.items[i].filename, "default_binary.bin") == 0) {
			rval = load_ucode_to_mem(fd, bbtable, blocks, bs,
						 ucode_mem +
						 info.items[i].addr_offset,
						 dsphdr.data.offset,
						 dsphdr.data.len);
			if (rval < 0) {
				fprintf(stderr, "load %s failed!\n", info.items[i].filename);
			}
		}
	}

	rval = ioctl(ud, IAV_IOC_UPDATE_UCODE, 0);
	if (rval < 0) {
		perror("IAV_IOC_UPDATE_UCODE");
		goto done;
	}

	rval = ioctl(ud, IAV_IOC_GET_UCODE_VERSION, &version);
	if (rval < 0) {
		perror("IAV_IOC_GET_UCODE_VERSION");
		goto done;
	}

	fprintf(stderr, "u_code version = %d/%d/%d %d.%d\n",
		version.year, version.month, version.day,
		version.edition_num, version.edition_ver);

	rval = 0;

done:

	if (fd >= 0)
		close(fd);
	if (ud >= 0)
		close(ud);
	if (ucode_mem)
		munmap(ucode_mem, info.map_size);
	if (bbtable)
		free(bbtable);

	return rval;
}

int load_ucode_main(int argc, char **argv)
{
	const char *ucode_path;

	if (argc > 1) {
		// ucode path specified
		ucode_path = argv[1];
	} else {
		// use default path
		ucode_path = "/lib/firmware";
	}

	if (strncmp(ucode_path, "/dev/mtd", 8) == 0)
		return load_ucode_from_flash(ucode_path);

	if (strncmp(ucode_path, "dspmem", 6) == 0)
		return load_ucode_from_dspmem();

	return load_ucode_from_ramfs(ucode_path);
}

