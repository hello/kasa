/*
 * load_ucode.c
 *
 * History:
 *	2012/01/25 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <basetypes.h>

#include <config.h>
#if (defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3) || defined(CONFIG_ARCH_S3L))
#include "iav_ioctl.h"
#include "iav_ucode_ioctl.h"
#else
#include "iav_drv.h"
#endif

int load_ucode(const char *ucode_path)
{
	int i, retv = 0;
	int fd = -1;
	FILE *fp = NULL;
	ucode_load_info_t info;
	ucode_version_t version;
	u8 *ucode_mem;
	char filename[256];
	int file_length;

	do {
		if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
			perror("/dev/ucode");
			retv = -1;
			break;
		}

		if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
			perror("IAV_IOC_GET_UCODE_INFO");
			retv = -1;
			break;
		}

		printf("map_size = 0x%x, nr_item = %d\n", (u32)info.map_size, info.nr_item);
		for (i = 0; i < info.nr_item; i++) {
			printf("addr_offset = 0x%08x, ", (u32)info.items[i].addr_offset);
			printf("filename = %s\n", info.items[i].filename);
		}

		ucode_mem = (u8 *)mmap(NULL, info.map_size,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if ((int)ucode_mem == -1) {
			perror("mmap");
			retv = -1;
			break;
		}
		printf("mmap returns 0x%x\n", (unsigned)ucode_mem);

		for (i = 0; i < info.nr_item; i++) {
			u8 *addr = ucode_mem + info.items[i].addr_offset;
			snprintf(filename, sizeof(filename), "%s/%s",
				ucode_path, info.items[i].filename);

			printf("loading %s...", filename);
			if ((fp = fopen(filename, "rb")) == NULL) {
				perror(filename);
				retv = -1;
				break;
			}

			if (fseek(fp, 0, SEEK_END) < 0) {
				perror("SEEK_END");
				retv = -1;
				break;
			}
			file_length = ftell(fp);
			if (fseek(fp, 0, SEEK_SET) < 0) {
				perror("SEEK_SET");
				retv = -1;
				break;
			}

			printf("addr = 0x%x, size = 0x%x\n", (u32)addr, file_length);

			if (fread(addr, 1, file_length, fp) != file_length) {
				perror("fread");
				retv = -1;
				break;
			}

			fclose(fp);
		}

		if (ioctl(fd, IAV_IOC_UPDATE_UCODE, 0) < 0) {
			perror("IAV_IOC_UPDATE_UCODE");
			retv = -1;
			break;
		}

		if (ioctl(fd, IAV_IOC_GET_UCODE_VERSION, &version) < 0) {
			perror("IAV_IOC_GET_UCODE_VERSION");
			retv = -1;
			break;
		}

		printf("===============================\n");
		printf("u_code version = %d/%d/%d %d.%d\n",
			version.year, version.month, version.day,
			version.edition_num, version.edition_ver);
		printf("===============================\n");

		if (munmap(ucode_mem, info.map_size) < 0) {
			perror("munmap");
			retv = -1;
			break;
		}
	} while (0);

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	return retv;
}


int main(int argc, char **argv)
{
	const char *ucode_path;

	if (argc > 1) {
		// ucode path specified
		ucode_path = argv[1];
	} else {
		// use default path
		ucode_path = "/mnt/firmware";
	}

	return load_ucode(ucode_path);
}

