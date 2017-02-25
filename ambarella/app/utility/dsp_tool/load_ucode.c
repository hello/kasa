/*
 * load_ucode.c
 *
 * History:
 *	2012/01/25 - [Jian Tang] created file
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


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <basetypes.h>

#include <config.h>
#if (defined(CONFIG_ARCH_S2L) || defined(CONFIG_ARCH_S3) || defined(CONFIG_ARCH_S3L) || defined(CONFIG_ARCH_S5) || defined(CONFIG_ARCH_S5L))
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
	u8 *ucode_mem = NULL, *addr = NULL;
	char filename[256];
	long file_length;

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

		printf("map_size = 0x%lx, nr_item = %lu\n", info.map_size, info.nr_item);
		for (i = 0; i < info.nr_item; i++) {
			printf("addr_offset = 0x%08x, ", (u32)info.items[i].addr_offset);
			printf("filename = %s\n", info.items[i].filename);
		}

		ucode_mem = (u8 *)mmap(NULL, info.map_size,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if ((intptr_t)ucode_mem == -1) {
			perror("mmap");
			retv = -1;
			break;
		}
		printf("mmap returns %p\n", ucode_mem);

		for (i = 0; i < info.nr_item; i++) {
			addr = ucode_mem + info.items[i].addr_offset;
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

			printf("addr = %p, size = 0x%lx\n", addr, file_length);

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

		printf("===============================================\n");
		printf("ucode ");
#if (defined(CONFIG_ARCH_S2) || defined(CONFIG_ARCH_S2E))
		if (version.chip_version & 0x1) {
			printf("(S2E) ");
		} else {
			printf("(S2) ");
		}
		if (version.chip_version & 0x2) {
			printf("(S2 test) ");
		}
#else
		switch (version.chip_arch) {
		case UCODE_ARCH_S2L:
			printf("(S2L) ");
			break;
		case UCODE_ARCH_S3L:
			printf("(S3L) ");
			break;
		case UCODE_ARCH_S5:
			printf("(S5) ");
			break;
		case UCODE_ARCH_S5L:
			printf("(S5L)");
			break;
		default:
			printf("(Unknown: %d) ", version.chip_arch);
			break;
		}
#endif
		printf("version = %d/%d/%d %d.%d\n",
			version.year, version.month, version.day,
			version.edition_num, version.edition_ver);
		printf("===============================================\n");
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

