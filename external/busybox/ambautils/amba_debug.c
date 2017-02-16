/*
 * amba_debug.c
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *    2008/05/05 - [Oliver Li] change printf(...,errno) to perror(); remove warning
 *    2009/05/15 - [Zhenwu Xue] add peripheral debug
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <libbb.h>
#include <getopt.h>

#include "../../../ambarella/build/include/basetypes.h"
#include "../../../ambarella/build/include/amba_debug.h"
#include "../../../ambarella/kernel/linux/include/linux/spi/spidev.h"

#define I2C_SLAVE	0x0703
#define I2C_TENBIT	0x0704

//extern int getopt_long(int, char **, const char *, const struct option*, int *);

#define	AMBA_DEBUG_NULL	(0)
#define	AMBA_DEBUG_IAV	(1 << 0)
#define	AMBA_DEBUG_DSP	(1 << 1)
#define	AMBA_DEBUG_VIN	(1 << 2)
#define	AMBA_DEBUG_VOUT	(1 << 3)
#define	AMBA_DEBUG_AAA	(1 << 4)

#define NO_ARG		0
#define HAS_ARG		1

#define AMBA_DEBUG_VIN_START_ADDRESS	(0x00000000)
#define AMBA_DEBUG_VIN_SRC_SIZE		(0x00010000)

struct hint_s {
	const char *arg;
	const char *str;
};

struct ambarella_debug_level_mask_table {
	const char *name;
	u32 mask;
};

/* Debug Mode */
enum amba_debug_opmode {
	AMBA_DEBUG_IDLE,
	AMBA_DEBUG_READ,
	AMBA_DEBUG_WRITE,
	AMBA_DEBUG_CHANGE_FLAG,
	AMBA_DEBUG_GPIO,
	AMBA_DEBUG_PERIPHERAL,
} opmode = AMBA_DEBUG_IDLE;

/* Debug Variables */
struct op_s {
	u32 op_address;
	u32 op_size;
	int op_data;
	u32 op_byte_width;
	char op_filename[256];
	u32 op_set_mask;
	u32 op_clr_mask;
	u32 op_map;

	u32 op_data_valid;
	u32 op_verbose;

	int op_gpioid;
	char op_peripheral[256];
	char op_cfgname[256];
};

#define op_address	operands->op_address
#define op_size		operands->op_size
#define op_data		operands->op_data
#define op_byte_width	operands->op_byte_width
#define op_filename	operands->op_filename
#define op_set_mask	operands->op_set_mask
#define op_clr_mask	operands->op_clr_mask
#define op_map		operands->op_map
#define op_data_valid	operands->op_data_valid
#define op_verbose	operands->op_verbose
#define op_gpioid	operands->op_gpioid
#define op_peripheral	operands->op_peripheral
#define op_cfgname	operands->op_cfgname


/* Function Declaration */
static void set_debug_flag(struct op_s *operands,
	struct ambarella_debug_level_mask_table *mask_table_list,
	const char *mode);
static void clr_debug_flag(struct op_s *operands,
	struct ambarella_debug_level_mask_table *mask_table_list,
	const char *mode);
static void usage(struct option *long_options, int num_options,
	const struct hint_s *hint,
	struct ambarella_debug_level_mask_table *mask_table_list, int num_table,
	const char **peripheral_table_list, int num_list);
static int dump_data(struct op_s *operands, char *pmem, u32 start,
	u32 size_limit, u32 address, u32 size);
static int dump_vin(struct op_s *operands, int fd_debug,
	u32 address, u32 size, u32 map);
static int fill_data(struct op_s *operands, char *pmem, u32 start,
	u32 size_limit, u32 address, u32 size);
static int fill_vin(struct op_s *operands, int fd_debug,
	u32 address, u32 size, u32 map);
static int debug_spi(struct op_s *operands);
static int debug_i2c(struct op_s *operands);
int amba_debug_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;

static void set_debug_flag(struct op_s *operands,
	struct ambarella_debug_level_mask_table *mask_table_list,
	const char *mode)
{
	int i;

	for (i = 0; i < sizeof(mask_table_list)/sizeof(mask_table_list[0]); i++)
		if (strcmp(mask_table_list[i].name, mode) == 0)
			op_set_mask |= mask_table_list[i].mask;
}

static void clr_debug_flag(struct op_s *operands,
	struct ambarella_debug_level_mask_table *mask_table_list,
	const char *mode)
{
	int i;

	for (i = 0; i < sizeof(mask_table_list)/sizeof(mask_table_list[0]); i++)
		if (strcmp(mask_table_list[i].name, mode) == 0)
			op_clr_mask |= mask_table_list[i].mask;
}

static void usage(struct option *long_options, int num_options,
	const struct hint_s *hint,
	struct ambarella_debug_level_mask_table *mask_table_list, int num_table,
	const char **peripheral_table_list, int num_list)
{
	int i;

	printf("/*\n\
 * The followings are memory partitions for amba_debug.\n\
 *       +----------------------------+ FFFF.FFFF\n\
 *       |                            |\n\
 *       | Reserved                   |\n\
 *       |                            |\n\
 *       +----------------------------+ D000.0000\n\
 *       |                            |\n\
 *       | DDR SDRAM                  |\n\
 *       |                            |\n\
 *       +----------------------------+ C000.0000\n\
 *       |                            |\n\
 *       | Reserved                   |\n\
 *       |                            |\n\
 *       +----------------------------+ 8000.0000\n\
 *       |                            |\n\
 *       | APB Mapped Registers       |\n\
 *       |                            |\n\
 *       +----------------------------+ 7000.0000\n\
 *       |                            |\n\
 *       | AHB Mapped Registers       |\n\
 *       |                            |\n\
 *       +----------------------------+ 6000.0000\n\
 *       |                            |\n\
 *       | Reserved                   |\n\
 *       |                            |\n\
 *       +----------------------------+ 1000.0000\n\
 *       |                            |\n\
 *       | VIN Source test registers  |\n\
 *       |                            |\n\
 *       +----------------------------+ 0000.0000\n\
 *\n\
 */ \n");

	printf("amba_debug usage:\n");
	for (i = 0; i < num_options; i++) {
		printf("--%s", long_options[i].name);
		if (long_options[i].val != 0)
			printf("/-%c", long_options[i].val);
		if (hint[i].arg[0] != 0)
			printf(" <%s>", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("module name:\n");
	for (i = 0; i < num_table; i++)
		printf("\t%s\n", mask_table_list[i].name);

	printf("peripheral name:\n");
	for (i = 0; i < num_list; i++)
		printf("\t%s\n", peripheral_table_list[i]);
}

static int dump_data(struct op_s *operands, char *pmem, u32 start,
	u32 size_limit, u32 address, u32 size)
{
	u32 offset;
	u32 data;
	u32 i;
	int fd;

	if (address & 0x03) {
		printf("address[0x%08x] will be alined to 0x%08x!\n",
			address, (address & 0xfffffffc));
		address &= 0xfffffffc;
	}

	offset = address - start;
	if (offset >= size_limit) {
		printf("offset too big! 0x%x < 0x%x\n", size_limit, offset);
		return -1;
	}
	if ((offset + size) >= size_limit) {
		printf("size too big! 0x%x < 0x%x\n", size_limit, (offset + size));
		size = size_limit - offset;
	}
	pmem += offset;

	if (strlen(op_filename)) {
		if (strcmp(op_filename, "stdout") == 0) {
			fd = STDOUT_FILENO;
		}
		else if ((fd = open(op_filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			perror(op_filename);
			return -1;
		}

		if (write(fd, (void *)pmem, size) < 0) {
			perror("write");
			return -1;
		}

		close(fd);
	} else {
		for (i = 0; i < size; i += 4) {
			if (i % 16 == 0)
				printf("\n0x%08x:", address + i);

			data = *(u32 *)(pmem + i);

			switch (op_byte_width) {
			case 1:
			case 2:
			case 4:
				printf("\t0x%08x", data);
				break;
			}
		}
		printf("\n");
	}

	return 0;
}

static int dump_vin(struct op_s *operands, int fd_debug,
	u32 address, u32 size, u32 map)
{
	u32 i;
	int fd;
	struct amba_vin_test_reg_data reg_data;

	if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_SRC_ID,
		(address / AMBA_DEBUG_VIN_SRC_SIZE)) < 0) {
		perror("AMBA_DEBUG_IOC_VIN_SET_SRC_ID");
		return -1;
	}

	if (strlen(op_filename)) {
		if ((fd = open(op_filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			perror(op_filename);
			return -1;
		}

		for (i = address; i < (address + size); i++) {
			reg_data.reg = i;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_GET_REG_DATA, &reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_GET_REG_DATA");
				return -1;
			}
			if (write(fd, (void *)&reg_data.data, sizeof(reg_data.data)) < 0) {
				perror("write");
				return -1;
			}
		}

		close(fd);
	} else {
		for (i = address; i < (address + size); i++) {
			reg_data.reg = i;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_GET_REG_DATA, &reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_GET_REG_DATA");
				return -1;
			}

			printf("0x%x=0x%x\n", reg_data.reg, reg_data.data);
		}
	}

	return 0;
}

static int fill_data(struct op_s *operands, char *pmem, u32 start,
	u32 size_limit, u32 address, u32 size)
{
	u32 offset;
	u32 i;
	int fd;

	if (address & 0x03) {
		printf("address[0x%08x] will be alined to 0x%08x!\n",
			address, (address & 0xfffffffc));
		address &= 0xfffffffc;
	}

	offset = address - start;
	if (offset >= size_limit) {
		printf("offset too big! 0x%x < 0x%x\n", size_limit, offset);
		return -1;
	}
	if ((offset + size) >= size_limit) {
		printf("size too big! 0x%x < 0x%x\n", size_limit, (offset + size));
		size = size_limit - offset;
	}
	pmem += offset;

	if (strlen(op_filename)) {
		if ((fd = open(op_filename, O_RDONLY, 0777)) < 0) {
			perror(op_filename);
			return -1;
		}

		if (read(fd, (void *)pmem, size) < 0) {
			perror("read");
			return -1;
		}

		close(fd);
	} else if (op_data_valid) {
		for (i = 0; i < size; i += 4) {
			switch (op_byte_width) {
			case 1:
			case 2:
			case 4:
				*(u32 *)(pmem + i) = op_data;
				break;
			}
		}
	}

	return 0;
}

static int fill_vin(struct op_s *operands, int fd_debug,
	u32 address, u32 size, u32 map)
{
	u32 i;
	int fd;
	struct amba_vin_test_reg_data reg_data;

	if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_SRC_ID,
		(address / AMBA_DEBUG_VIN_SRC_SIZE)) < 0) {
		perror("AMBA_DEBUG_IOC_VIN_SET_SRC_ID");
		return -1;
	}

	if (strlen(op_filename)) {
		if ((fd = open(op_filename, O_RDONLY, 0777)) < 0) {
			perror(op_filename);
			return -1;
		}

		for (i = address; i < (address + size); i++) {
			if (read(fd, (void *)&reg_data.data, sizeof(reg_data.data)) < 0) {
				perror("read");
				return -1;
			}
			reg_data.reg = i;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_REG_DATA, &reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_SET_REG_DATA");
				return -1;
			}
		}

		close(fd);
	} else {
		for (i = address; i < (address + size); i++) {
			reg_data.reg = i;
			reg_data.data = op_data;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_REG_DATA, &reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_SET_REG_DATA");
				return -1;
			}
		}
	}

	return 0;
}

static int debug_spi(struct op_s *operands)
{
	char spi_dev[256];
	int mode;
	int bpw;
	int baudrate;
	int rw;
	int size;
	int spi_fd;
	u8 *w8 = NULL;
	u8 *r8 = NULL;
	u16 *w16 = NULL;
	u16 *r16 = NULL;
	int i;
	int confirm;
	int ret;
	u32 tmp;
	struct spi_ioc_transfer tr;

	if (strlen(op_cfgname) == 0) {
		/*device*/
		do {
			printf("Input SPI device node: ");
			scanf("%s", spi_dev);
			spi_fd = open(spi_dev, O_RDWR, 0);
		} while (spi_fd < 0);

		/*mode*/
		do {
			printf("Input SPI mode(0-3): ");
			scanf("%d", &mode);
		} while (mode < 0 || mode > 3);

		/*bpw*/
		do {
			printf("Input bits per word(4-16): ");
			scanf("%d", &bpw);
		} while (bpw < 4 || mode > 16);

		/*baudrate*/
		do {
			printf("Input baudrate(10,000-10,000,000): ");
			scanf("%d", &baudrate);
		} while (baudrate < 10000 || baudrate > 10000000);

		/*rw*/
		do {
			printf("Input read/write mode(0-write & read, 1-write only, 2-read only): ");
			scanf("%d", &rw);
		} while (rw < 0 || rw > 2);

		/*size*/
		do {
			printf("Input transfer size(>0): ");
			scanf("%d", &size);
		} while (size <= 0);

		/*memory*/
		switch (rw) {
		case 0:
			if (bpw <= 8) {
				w8 = (u8 *)malloc(size * sizeof(u8));
				r8 = (u8 *)malloc(size * sizeof(u8));
				if (w8 == NULL || r8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				w16 = (u16 *)malloc(size * sizeof(u16));
				r16 = (u16 *)malloc(size * sizeof(u16));
				if (w16 == NULL || r16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		case 1:
			if (bpw <= 8) {
				w8 = (u8 *)malloc(size * sizeof(u8));
				if (w8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				w16 = (u16 *)malloc(size * sizeof(u16));
				if (w16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		case 2:
			if (bpw <= 8) {
				r8 = (u8 *)malloc(size * sizeof(u8));
				if (r8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				r16 = (u16 *)malloc(size * sizeof(u16));
				if (r16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		default:
			break;
		}

		/*data*/
		if (rw == 0 || rw == 1) {
			do {
				for (i = 0; i < size; i++) {
					printf("data %d: ", i + 1);
					if (bpw <= 8) {
						scanf("%u", &tmp);
						w8[i] = (u8)tmp;
						w8[i] &= (0xff >> (8 - bpw));
					} else {
						scanf("%hu", &w16[i]);
						w16[i] &= (0xffff >> (16 - bpw));
					}
				}

				printf("You've input:\n");
				for (i = 0; i < size; i++) {
					if (bpw <= 8)
						printf("data %d = 0x%02x\n", i + 1, w8[i]);
					else
						printf("data %d = 0x%04x\n", i + 1, w16[i]);
				}

				printf("Input data correct(1/0): ");
				scanf("%d", &confirm);
			} while (confirm == 0);
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
		if (ret == -1) {
			printf("can't set spi mode!\n");
			return -1;
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bpw);
		if (ret == -1) {
			printf("can't set bits per word!\n");
			return -1;
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate);
		if (ret == -1) {
			printf("can't set max speed hz!\n");
			return -1;
		}

		switch (rw) {
		case 0:
			if (bpw <= 8) {
				tr.tx_buf = (unsigned long)w8;
				tr.rx_buf = (unsigned long)r8;
				tr.len = size;
			} else {
				tr.tx_buf = (unsigned long)w16;
				tr.rx_buf = (unsigned long)r16;
				tr.len = size << 1;
			}
			tr.delay_usecs = 0;
			tr.speed_hz = baudrate;
			tr.bits_per_word = bpw;

			ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
			if (ret == 1) {
				printf("can't send spi message!\n");
				return -1;
			}

			printf("Data received:\n");
			if (bpw <= 8) {
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%02x\n", i + 1, r8[i]);

				free(w8);
				free(r8);
			} else {
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%04x\n", i + 1, r16[i]);

				free(w16);
				free(r16);
			}

			break;
		case 1:
			if (bpw <= 8) {
				ret = write(spi_fd, w8, size);
				if (ret != size) {
					printf("write error!\n");
					return -1;
				}

				free(w8);
			} else {
				ret = write(spi_fd, w16, size << 1);
				if (ret != (size << 1)) {
					printf("write error!\n");
					return -1;
				}

				free(w16);
			}
			break;
		case 2:
			if (bpw <= 8) {
				ret = read(spi_fd, r8, size);
				if (ret != size) {
					printf("read error!\n");
					return -1;
				}

				printf("Data received:\n");
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%02x\n", i + 1, r8[i]);

				free(r8);
			} else {
				ret = read(spi_fd, r16, size << 1);
				if (ret != (size << 1)) {
					printf("read error!\n");
					return -1;
				}

				printf("Data received:\n");
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%04x\n", i + 1, r16[i]);

				free(r16);
			}
			break;
		}

		close(spi_fd);
	} else {
		FILE * spi_cfg = NULL;

		spi_cfg = fopen(op_cfgname, "r");
		if (spi_cfg == NULL) {
			perror("Can't open op_filename");
			return -1;
		}

		/*device*/
		fscanf(spi_cfg, "node: ");
		fscanf(spi_cfg, "%s\n", spi_dev);

		spi_fd = open(spi_dev, O_RDWR, 0);
		if (spi_fd < 0) {
			printf("Incorrect device node!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*mode*/
		fscanf(spi_cfg, "mode: ");
		fscanf(spi_cfg, "%d\n", &mode);
		if (mode < 0 || mode > 3) {
			printf("Incorrect spi mode!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*bpw*/
		fscanf(spi_cfg, "bpw: ");
		fscanf(spi_cfg, "%d\n", &bpw);
		if (bpw < 4 || bpw > 16) {
			printf("Incorrect bits per word!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*baudrate*/
		fscanf(spi_cfg, "baudrate: ");
		fscanf(spi_cfg, "%d\n", &baudrate);
		if (baudrate < 10000 || baudrate > 10000000) {
			printf("Incorrect baudrate!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*rw*/
		fscanf(spi_cfg, "rw: ");
		fscanf(spi_cfg, "%d\n", &rw);
		if (rw < 0 || rw > 2) {
			printf("Incorrect rw mode!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*size*/
		fscanf(spi_cfg, "size: ");
		fscanf(spi_cfg, "%d\n", &size);
		if (size <= 0) {
			printf("Incorrect transfer size!\n");
			fclose(spi_cfg);
			return -1;
		}

		/*memory*/
		switch (rw) {
		case 0:
			if (bpw <= 8) {
				w8 = (u8 *)malloc(size * sizeof(u8));
				r8 = (u8 *)malloc(size * sizeof(u8));
				if (w8 == NULL || r8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				w16 = (u16 *)malloc(size * sizeof(u16));
				r16 = (u16 *)malloc(size * sizeof(u16));
				if (w16 == NULL || r16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		case 1:
			if (bpw <= 8) {
				w8 = (u8 *)malloc(size * sizeof(u8));
				if (w8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				w16 = (u16 *)malloc(size * sizeof(u16));
				if (w16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		case 2:
			if (bpw <= 8) {
				r8 = (u8 *)malloc(size * sizeof(u8));
				if (r8 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			} else {
				r16 = (u16 *)malloc(size * sizeof(u16));
				if (r16 == NULL) {
					printf("Error: Not enough memory!\n");
					return -1;
				}
			}
			break;
		default:
			break;
		}

		/*data*/
		if (rw == 0 || rw == 1) {
			fscanf(spi_cfg, "data: ");
			for (i = 0; i < size; i++) {
				if (bpw <= 8) {
					fscanf(spi_cfg, "%u", &tmp);
					w8[i] = (u8)tmp;
					w8[i] &= (0xff >> (8 - bpw));
				} else {
					fscanf(spi_cfg, "%hu", &w16[i]);
					w16[i] &= (0xffff >> (16 - bpw));
				}
			}

			printf("You've input:\n");
			for (i = 0; i < size; i++) {
				if (bpw <= 8)
					printf("data %d = 0x%02x\n", i + 1, w8[i]);
				else
					printf("data %d = 0x%04x\n", i + 1, w16[i]);
			}
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
		if (ret == -1) {
			printf("can't set spi mode!\n");
			return -1;
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bpw);
		if (ret == -1) {
			printf("can't set bits per word!\n");
			return -1;
		}

		ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate);
		if (ret == -1) {
			printf("can't set max speed hz!\n");
			return -1;
		}

		switch (rw) {
		case 0:
			if (bpw <= 8) {
				tr.tx_buf = (unsigned long)w8;
				tr.rx_buf = (unsigned long)r8;
				tr.len = size;
			} else {
				tr.tx_buf = (unsigned long)w16;
				tr.rx_buf = (unsigned long)r16;
				tr.len = size << 1;
			}
			tr.delay_usecs = 0;
			tr.speed_hz = baudrate;
			tr.bits_per_word = bpw;

			ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
			if (ret == 1) {
				printf("can't send spi message!\n");
				return -1;
			}

			printf("Data received:\n");
			if (bpw <= 8) {
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%02x\n", i + 1, r8[i]);

				free(w8);
				free(r8);
			} else {
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%04x\n", i + 1, r16[i]);

				free(w16);
				free(r16);
			}

			break;
		case 1:
			if (bpw <= 8) {
				ret = write(spi_fd, w8, size);
				if (ret != size) {
					printf("write error!\n");
					return -1;
				}

				free(w8);
			} else {
				ret = write(spi_fd, w16, size << 1);
				if (ret != (size << 1)) {
					printf("write error!\n");
					return -1;
				}

				free(w16);
			}
			break;
		case 2:
			if (bpw <= 8) {
				ret = read(spi_fd, r8, size);
				if (ret != size) {
					printf("read error!\n");
					return -1;
				}

				printf("Data received:\n");
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%02x\n", i + 1, r8[i]);

				free(r8);
			} else {
				ret = read(spi_fd, r16, size << 1);
				if (ret != (size << 1)) {
					printf("read error!\n");
					return -1;
				}

				printf("Data received:\n");
				for (i = 0; i < size; i++)
					printf("read data %d = 0x%04x\n", i + 1, r16[i]);

				free(r16);
			}
			break;
		}

		fclose(spi_cfg);
		close(spi_fd);
	}

	return 0;
}

static int debug_i2c(struct op_s *operands)
{
	char i2c_dev[256];
	int mode;
	int addr;
	int r_size;
	int w_size;
	int i2c_fd;
	int rw;
	u8 *w8 = NULL;
	u8 *r8 = NULL;
	int i;
	int confirm;
	int ret;
	u32 tmp;

	if (strlen(op_cfgname) == 0) {
		/*device*/
		do {
			printf("Input I2C device node: ");
			scanf("%s", i2c_dev);
			i2c_fd = open(i2c_dev, O_RDWR, 0);
		} while (i2c_fd < 0);

		/*mode*/
		do {
			printf("Input I2C address bits(7/10): ");
			scanf("%d", &mode);
		} while (mode != 7 && mode != 10);

		if (mode == 7) {
			ret = ioctl(i2c_fd, I2C_TENBIT, 0);
			if (ret < 0) {
				printf("Error: Unable to set 7-bit address mode!\n");
				close(i2c_fd);
				return -1;
			}
		} else {
			ret = ioctl(i2c_fd, I2C_TENBIT, 1);
			if (ret < 0) {
				printf("Error: Unable to set 10-bit address mode!\n");
				close(i2c_fd);
				return -1;
			}
		}

		/*addr*/
		do {
			printf("Input I2C address(>=0): ");
			scanf("%d", &addr);
		} while (addr < 0);

		ret = ioctl(i2c_fd, I2C_SLAVE, addr >> 1);
		if (ret < 0) {
			printf("Error: Unable to set slave address!\n");
			close(i2c_fd);
			return -1;
		}

		/*rw*/
		do {
			printf("Input read/write mode(0-write then read, 1-write only, 2-read only): ");
			scanf("%d", &rw);
		} while (rw < 0 || rw > 2);

		/*size*/
		switch (rw) {
		case 0:
			do {
				printf("Input write size(>0) and read size(>0): ");
				scanf("%d %d", &w_size, &r_size);
			} while (w_size <= 0 || r_size <= 0);
			break;
		case 1:
			do {
				printf("Input write size(>0): ");
				scanf("%d", &w_size);
			} while (w_size <= 0);
			break;
		case 2:
			do {
				printf("Input read size(>0): ");
				scanf("%d", &r_size);
			} while (r_size <= 0);
			break;
		default:
			break;
		}

		/*memory*/
		switch (rw) {
		case 0:
			w8 = (u8 *)malloc(w_size * sizeof(u8));
			r8 = (u8 *)malloc(r_size * sizeof(u8));
			if (w8 == NULL || r8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		case 1:
			w8 = (u8 *)malloc(w_size * sizeof(u8));
			if (w8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		case 2:
			r8 = (u8 *)malloc(r_size * sizeof(u8));
			if (r8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		default:
			break;
		}

		/*data*/
		if (rw == 0 || rw == 1) {
			do {
				for (i = 0; i < w_size; i++) {
					printf("data %d: ", i + 1);
					scanf("%u", &tmp);
					w8[i] = (u8)tmp;
				}

				printf("You've input:\n");
				for (i = 0; i < w_size; i++) {
					printf("data %d = 0x%02x\n", i + 1, w8[i]);
				}

				printf("Input data correct(1/0): ");
				scanf("%d", &confirm);
			} while (confirm == 0);
		}

		switch (rw) {
		case 0:
			ret = write(i2c_fd, w8, w_size);
			if (ret != w_size) {
				printf("Error: Unable to write i2c!\n");
				close(i2c_fd);
				return -1;
			}
			free(w8);

			ret = read(i2c_fd, r8, r_size);
			if (ret != r_size) {
				printf("Error: Unable to read i2c!\n");
				close(i2c_fd);
				return -1;
			}

			printf("Data received:\n");
			for (i = 0; i < r_size; i++)
				printf("read data %d = 0x%02x\n", i + 1, r8[i]);

			free(r8);

			break;
		case 1:
			ret = write(i2c_fd, w8, w_size);
			if (ret != w_size) {
				printf("Error: Unable to write i2c!\n");
				close(i2c_fd);
				return -1;
			}
			free(w8);
			break;
		case 2:
			ret = read(i2c_fd, r8, r_size);
			if (ret != r_size) {
				printf("Error: Unable to read i2c!\n");
				close(i2c_fd);
				return -1;
			}

			printf("Data received:\n");
			for (i = 0; i < r_size; i++)
				printf("read data %d = 0x%02x\n", i + 1, r8[i]);

			free(r8);
			break;
		}

		close(i2c_fd);
	} else {
		FILE * i2c_cfg = NULL;

		i2c_cfg = fopen(op_cfgname, "r");
		if (i2c_cfg == NULL) {
			perror("Can't open op_filename");
			return -1;
		}

		/*device*/
		fscanf(i2c_cfg, "node: ");
		fscanf(i2c_cfg, "%s\n", i2c_dev);
		i2c_fd = open(i2c_dev, O_RDWR, 0);
		if (i2c_fd < 0) {
			printf("Incorrect i2c_dev!\n");
			fclose(i2c_cfg);
			return -1;
		}

		/*mode*/
		fscanf(i2c_cfg, "mode: ");
		fscanf(i2c_cfg, "%d\n", &mode);
		if (mode != 7 && mode != 10) {
			printf("Incorrect i2c address mode!\n");
			fclose(i2c_cfg);
			return -1;
		}

		if (mode == 7) {
			ret = ioctl(i2c_fd, I2C_TENBIT, 0);
			if (ret < 0) {
				printf("Error: Unable to set 7-bit address mode!\n");
				close(i2c_fd);
				return -1;
			}
		} else {
			ret = ioctl(i2c_fd, I2C_TENBIT, 1);
			if (ret < 0) {
				printf("Error: Unable to set 10-bit address mode!\n");
				close(i2c_fd);
				return -1;
			}
		}

		/*addr*/
		fscanf(i2c_cfg, "addr: ");
		fscanf(i2c_cfg, "%d\n", &addr);
		if (addr < 0) {
			printf("Incorrect i2c address!\n");
			fclose(i2c_cfg);
			return -1;
		}

		ret = ioctl(i2c_fd, I2C_SLAVE, addr >> 1);
		if (ret < 0) {
			printf("Error: Unable to set slave address!\n");
			close(i2c_fd);
			return -1;
		}

		/*rw*/
		fscanf(i2c_cfg, "rw: ");
		fscanf(i2c_cfg, "%d\n", &rw);
		if (rw < 0 || rw > 2) {
			printf("Incorrect i2c rw!\n");
			fclose(i2c_cfg);
			return -1;
		}

		/*size*/
		fscanf(i2c_cfg, "size: ");
		switch (rw) {
		case 0:
			fscanf(i2c_cfg, "%d %d\n", &w_size, &r_size);
			if (w_size <= 0 || r_size <= 0) {
				printf("Incorrect size!\n");
				fclose(i2c_cfg);
				return -1;
			}
			break;
		case 1:
			fscanf(i2c_cfg, "%d\n", &w_size);
			if (w_size <= 0) {
				printf("Incorrect size!\n");
				fclose(i2c_cfg);
				return -1;
			}
			break;
		case 2:
			fscanf(i2c_cfg, "%d\n", &r_size);
			if (r_size <= 0) {
				printf("Incorrect size!\n");
				fclose(i2c_cfg);
				return -1;
			}
			break;
		default:
			break;
		}

		/*memory*/
		switch (rw) {
		case 0:
			w8 = (u8 *)malloc(w_size * sizeof(u8));
			r8 = (u8 *)malloc(r_size * sizeof(u8));
			if (w8 == NULL || r8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		case 1:
			w8 = (u8 *)malloc(w_size * sizeof(u8));
			if (w8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		case 2:
			r8 = (u8 *)malloc(r_size * sizeof(u8));
			if (r8 == NULL) {
				printf("Error: Not enough memory!\n");
				return -1;
			}
			break;
		default:
			break;
		}

		/*data*/
		if (rw == 0 || rw == 1) {
			fscanf(i2c_cfg, "data: ");
			for (i = 0; i < w_size; i++) {
				fscanf(i2c_cfg, "%u", &tmp);
				w8[i] = (u8)tmp;
			}

			printf("You've input:\n");
			for (i = 0; i < w_size; i++) {
				printf("data %d = 0x%02x\n", i + 1, w8[i]);
			}
		}

		switch (rw) {
		case 0:
			ret = write(i2c_fd, w8, w_size);
			if (ret != w_size) {
				printf("Error: Unable to write i2c!\n");
				close(i2c_fd);
				return -1;
			}
			free(w8);

			ret = read(i2c_fd, r8, r_size);
			if (ret != r_size) {
				printf("Error: Unable to read i2c!\n");
				close(i2c_fd);
				return -1;
			}

			printf("Data received:\n");
			for (i = 0; i < r_size; i++)
				printf("read data %d = 0x%02x\n", i + 1, r8[i]);

			free(r8);

			break;
		case 1:
			ret = write(i2c_fd, w8, w_size);
			if (ret != w_size) {
				printf("Error: Unable to write i2c!\n");
				close(i2c_fd);
				return -1;
			}
			free(w8);
			break;
		case 2:
			ret = read(i2c_fd, r8, r_size);
			if (ret != r_size) {
				printf("Error: Unable to read i2c!\n");
				close(i2c_fd);
				return -1;
			}

			printf("Data received:\n");
			for (i = 0; i < r_size; i++)
				printf("read data %d = 0x%02x\n", i + 1, r8[i]);

			free(r8);
			break;
		}

		fclose(i2c_cfg);
		close(i2c_fd);
	}

	return 0;
}

int amba_debug_main(int argc, char **argv)
{
	/* Command Line Options */
	struct option long_options[] = {
		{"read", HAS_ARG, 0, 'r'},
		{"write", HAS_ARG, 0, 'w'},
	//	{"bytes", HAS_ARG, 0, 'b'},
		{"data", HAS_ARG, 0, 'd'},
		{"size", HAS_ARG, 0, 's'},
		{"file", HAS_ARG, 0, 'f'},
		{"verbose", HAS_ARG, 0, 'v'},
		{"EBANLE", HAS_ARG, 0, 'E'},
		{"DISABLE", HAS_ARG, 0, 'D'},
		{"gpioid", HAS_ARG, 0, 'g'},
		{"regmap", HAS_ARG, 0, 'm'},
		{"peripheral", HAS_ARG, 0, 'p'},
		{"cfg", HAS_ARG, 0, 'c'},
	};

	const char *short_options = "r:w:b:d:s:f:v:E:D:g:m:p:c:";

	const struct hint_s hint[] = {
		{"address", "read address, hex[0x00000000]"},
		{"address", "write address, hex[0x00000000]"},
	//	{"bytes", "read/write byte size, dec[4]"},
		{"data", "data to written, hex[0x00000000]"},
		{"size", "read/write size, hex[0x00000000]"},
		{"binfile", "file to load or store binary data"},
		{"level", "set verbose level, dec[0]"},
		{"module name", "enable module debug"},
		{"module name", "disbale module debug"},
		{"gpioid", "read/write gpio"},
		{"regmap", "set register map"},
		{"peripheral name", "communicate with peripherals"},
		{"configuration file", "input argument from .cfg file"},
	};

	struct ambarella_debug_level_mask_table	mask_table_list[] = {
		{"iav",		AMBA_DEBUG_IAV},
		{"dsp",		AMBA_DEBUG_DSP},
		{"vin",		AMBA_DEBUG_VIN},
		{"vout",	AMBA_DEBUG_VOUT},
		{"aaa",		AMBA_DEBUG_AAA},
	};

	const char *peripheral_table_list[] = {
		"spi",
		"i2c",
	};

	int rval = 0;
	int errorCode = 0;
	int ch;
	int option_index = 0;
	struct op_s *operands = NULL;

	int fd;
	int debug_flag;
	struct amba_debug_mem_info mem_info;
	char *debug_mem;
	struct amba_vin_test_gpio gpio;

	opterr = 0;

	if (argc < 2) {
		usage(long_options, ARRAY_SIZE(long_options), hint,
			mask_table_list, ARRAY_SIZE(mask_table_list),
			peripheral_table_list, ARRAY_SIZE(peripheral_table_list));
		errorCode = -1;
		goto amba_debug_main_exit;
	}

	operands = (struct op_s *)malloc(sizeof(struct op_s));
	if (!operands) {
		errorCode = -1;
		goto amba_debug_main_exit;
	}

	op_address = 0;
	op_size = 1;
	op_data = -1;
	op_byte_width = 4;
	op_filename[0] = '\0';
	op_set_mask = 0;
	op_clr_mask = 0;
	op_map = 0;

	op_data_valid = 0;
	op_verbose = 0;

	op_gpioid = -1;
	op_peripheral[0] = '\0';
	op_cfgname[0] = '\0';

	/* init-param */
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'r':
			opmode = AMBA_DEBUG_READ;
			rval = sscanf(optarg, "0x%08x", &op_address);
			if (rval != 1)
				errorCode = -1;
			break;

		case 'w':
			opmode = AMBA_DEBUG_WRITE;
			rval = sscanf(optarg, "0x%08x", &op_address);
			if (rval != 1)
				errorCode = -1;
			break;

		case 'b':
			rval = sscanf(optarg, "%d", &op_byte_width);
			if (rval != 1)
				errorCode = -1;

		case 'd':
			rval = sscanf(optarg, "0x%08x", &op_data);
			if (rval != 1)
				errorCode = -1;
			else
				op_data_valid = 1;
			break;

		case 's':
			rval = sscanf(optarg, "0x%08x", &op_size);
			if (rval != 1)
				errorCode = -1;
			break;

		case 'f':
			strcpy(op_filename, optarg);
			break;

		case 'v':
			rval = sscanf(optarg, "%d", &op_verbose);
			if (rval != 1)
				errorCode = -1;
			break;

		case 'E':
			opmode = AMBA_DEBUG_CHANGE_FLAG;
			set_debug_flag(operands, mask_table_list, optarg);
			break;

		case 'D':
			opmode = AMBA_DEBUG_CHANGE_FLAG;
			clr_debug_flag(operands, mask_table_list, optarg);
			break;

		case 'g':
			opmode = AMBA_DEBUG_GPIO;
			rval = sscanf(optarg, "%d", &op_gpioid);
			if (rval != 1)
				errorCode = -1;
			break;

		case 'm':
			rval = sscanf(optarg, "0x%02x", &op_map);
			if (rval != 1) {
				errorCode = -1;
				printf("Got error!\n");
			}
			break;
		case 'p':
			opmode = AMBA_DEBUG_PERIPHERAL;
			strcpy(op_peripheral, optarg);
			break;
		case 'c':
			strcpy(op_cfgname, optarg);
			break;
		default:
			printf("unknown option found: %c\n", ch);
			errorCode = -1;
		}
	}

	if (op_verbose) {
		printf("opmode = %d\n", opmode);
		printf("op_address = 0x%08x\n", op_address);
		printf("op_map = 0x%02x\n", op_map);
		printf("op_size = 0x%08x\n", op_size);
		printf("op_data = 0x%08x\n", op_data);
		printf("op_byte_width = 0x%08x\n", op_byte_width);
		printf("op_filename: %s\n", op_filename);
		printf("op_set_mask = 0x%08x\n", op_set_mask);
		printf("op_clr_mask = 0x%08x\n", op_clr_mask);
		printf("op_data_valid = %d\n", op_data_valid);
		printf("op_gpioid = %d\n", op_gpioid);
		printf("op_peripheral: %s\n", op_peripheral);
		printf("rval = %d\n", rval);
	}

	if (errorCode) {
		usage(long_options, ARRAY_SIZE(long_options), hint,
			mask_table_list, ARRAY_SIZE(mask_table_list),
			peripheral_table_list, ARRAY_SIZE(peripheral_table_list));
		errorCode = -1;
		goto amba_debug_main_exit;
	}

	/* debug_amba */
	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		errorCode = -1;
		goto amba_debug_main_exit;;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
		errorCode = -1;
		goto amba_debug_main_exit;
	}
	if (op_verbose)
		printf("debug_flag 0x%08x\n", debug_flag);

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_MEM_INFO, &mem_info) < 0) {
		perror("AMBA_DEBUG_IOC_GET_MEM_INFO");
		errorCode = -1;
		goto amba_debug_main_exit;
	}

	if (op_verbose) {
		printf("ahb_start 0x%08x\n", mem_info.ahb_start);
		printf("ahb_size 0x%08x\n", mem_info.ahb_size);
		printf("apb_start 0x%08x\n", mem_info.apb_start);
		printf("apb_size 0x%08x\n", mem_info.apb_size);
		printf("ddr_start 0x%08x\n", mem_info.ddr_start);
		printf("ddr_size 0x%08x\n", mem_info.ddr_size);
	}

	debug_mem = (char *)mmap(NULL, (mem_info.ahb_size +
					mem_info.apb_size +
					mem_info.ddr_size),
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ((int)debug_mem < 0) {
		perror("mmap");
		errorCode = -1;
		goto amba_debug_main_exit;
	}
	if (op_verbose)
		printf("mmap returns 0x%08x\n", (unsigned)debug_mem);

	switch (opmode) {
	case AMBA_DEBUG_READ:
		if (op_address >= mem_info.ddr_start) {
			dump_data(operands, debug_mem + mem_info.ahb_size + mem_info.apb_size,
				mem_info.ddr_start,
				mem_info.ddr_size,
				op_address,
				op_size);
		} else if (op_address >= mem_info.apb_start) {
			dump_data(operands, debug_mem + mem_info.ahb_size,
				mem_info.apb_start,
				mem_info.apb_size,
				op_address,
				op_size);
		} else if (op_address >= mem_info.ahb_start) {
			dump_data(operands, debug_mem,
				mem_info.ahb_start,
				mem_info.ahb_size,
				op_address,
				op_size);
		} else if (op_address >= AMBA_DEBUG_VIN_START_ADDRESS) {
			dump_vin(operands, fd, op_address, op_size, op_map);
		}
		break;
	case AMBA_DEBUG_WRITE:
		if (op_address >= mem_info.ddr_start) {
			fill_data(operands, debug_mem + mem_info.ahb_size + mem_info.apb_size,
				mem_info.ddr_start,
				mem_info.ddr_size,
				op_address,
				op_size);
		} else if (op_address >= mem_info.apb_start) {
			fill_data(operands, debug_mem + mem_info.ahb_size,
				mem_info.apb_start,
				mem_info.apb_size,
				op_address,
				op_size);
		} else if (op_address >= mem_info.ahb_start) {
			fill_data(operands, debug_mem,
				mem_info.ahb_start,
				mem_info.ahb_size,
				op_address,
				op_size);
		} else if (op_address >= AMBA_DEBUG_VIN_START_ADDRESS) {
			fill_vin(operands, fd, op_address, op_size, op_map);
		}
		break;

	case AMBA_DEBUG_CHANGE_FLAG:
		if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
			errorCode = -1;
			goto amba_debug_main_exit;
		}
		debug_flag &= (~op_clr_mask);
		debug_flag |= op_set_mask;
		if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
			errorCode = -1;
			goto amba_debug_main_exit;
		}
		if (op_verbose)
			printf("debug_flag 0x%08x\n", debug_flag);
		break;

	case AMBA_DEBUG_GPIO:
		gpio.id = op_gpioid;
		gpio.data = op_data;

		if (op_data >= 0) {
			if (ioctl(fd, AMBA_DEBUG_IOC_SET_GPIO, &gpio) < 0) {
				perror("AMBA_DEBUG_IOC_SET_GPIO");
				errorCode = -1;
				goto amba_debug_main_exit;
			}
			if (op_verbose) {
				printf("id 0x%08x\n", gpio.id);
				printf("data 0x%08x\n", gpio.data);
			}
		} else {
			if (ioctl(fd, AMBA_DEBUG_IOC_GET_GPIO, &gpio) < 0) {
				perror("AMBA_DEBUG_IOC_SET_GPIO");
				errorCode = -1;
				goto amba_debug_main_exit;
			}
			printf("id 0x%08x\n", gpio.id);
			printf("data 0x%08x\n", gpio.data);
		}
		break;

	case AMBA_DEBUG_PERIPHERAL:
		if (strcmp(op_peripheral, "spi") == 0) {
			debug_spi(operands);
			break;
		}

		if (strcmp(op_peripheral, "i2c") == 0) {
			debug_i2c(operands);
			break;
		}

		usage(long_options, ARRAY_SIZE(long_options), hint,
			mask_table_list, ARRAY_SIZE(mask_table_list),
			peripheral_table_list, ARRAY_SIZE(peripheral_table_list));
		break;

	default:
		printf("Wrong opmode %d\n", opmode);
		break;
	}

	close(fd);

amba_debug_main_exit:
	if (operands)
		free(operands);

	return errorCode;
}

