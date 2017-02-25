/*
 * amba_debug.c
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *    2008/05/05 - [Oliver Li] change printf(...,errno) to perror(); remove warning
 *    2009/05/15 - [Zhenwu Xue] add peripheral debug
 *
 * Copyright (C) 2015 Ambarella, Inc.
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


/*========================== Header Files ====================================*/
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#endif

#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <basetypes.h>
#include <amba_debug.h>


/*============================== Constants ===================================*/
#define PAGE_SHIFT			12

#ifndef I2C_SLAVE
#define I2C_SLAVE			0x0703
#endif
#ifndef I2C_TENBIT
#define I2C_TENBIT			0x0704
#endif

#define	AMBA_DEBUG_NULL			(0)
#define	AMBA_DEBUG_IAV			(1 << 0)
#define	AMBA_DEBUG_DSP			(1 << 1)
#define	AMBA_DEBUG_VIN			(1 << 2)
#define	AMBA_DEBUG_VOUT			(1 << 3)
#define	AMBA_DEBUG_AAA			(1 << 4)

#define AMBA_DEBUG_VIN_START_ADDRESS	(0x00000000)
#define AMBA_DEBUG_VIN_SRC_SIZE		(0x00010000)

#define MAX_STRING_LEN			256

#define WRITE_THEN_READ			0
#define WRITE_ONLY			1
#define READ_ONLY			2

#define NO_ARG				0
#define HAS_ARG				1


/*========================== Structures and Enums ============================*/
struct hint_s {
	const char			*arg;
	const char			*str;
};

struct ambarella_debug_level_mask_table {
	const char			*name;
	u32				mask;
};

enum amba_debug_opmode {
	AMBA_DEBUG_IDLE,
	AMBA_DEBUG_READ,
	AMBA_DEBUG_WRITE,
	AMBA_DEBUG_CHANGE_FLAG,
	AMBA_DEBUG_GPIO,
};

/*============================= Global Variables =============================*/
static struct option long_options[] = {
	{"read",	HAS_ARG, 0, 'r'},
	{"write",	HAS_ARG, 0, 'w'},
	{"data",	HAS_ARG, 0, 'd'},
	{"size",	HAS_ARG, 0, 's'},
	{"file",	HAS_ARG, 0, 'f'},
	{"verbose",	HAS_ARG, 0, 'v'},
	{"EBANLE",	HAS_ARG, 0, 'E'},
	{"DISABLE",	HAS_ARG, 0, 'D'},
	{"gpioid",	HAS_ARG, 0, 'g'},
	{"regmap",	HAS_ARG, 0, 'm'},
	{"peripheral",	HAS_ARG, 0, 'p'},
	{"cfg",		HAS_ARG, 0, 'c'},
	{0, 0, 0, 0}
};

static const char *short_options = "r:w:b:d:s:f:v:E:D:g:m:p:c:";

static const struct hint_s hint[] = {
	{"address",		"read address, hex[0x00000000]"},
	{"address",		"write address, hex[0x00000000]"},
	{"data",		"data to written, hex[0x00000000]"},
	{"size",		"read/write size, hex[0x00000000]"},
	{"binfile",		"file to load or store binary data"},
	{"level",		"set verbose level, dec[0]"},
	{"module name",		"enable module debug"},
	{"module name",		"disbale module debug"},
	{"gpioid",		"read/write gpio"},
	{"regmap",		"set register map"},
	{"peripheral name",	"communicate with peripherals"},
	{"configuration file",	"input argument from .cfg file"},
};

static const struct ambarella_debug_level_mask_table mask_table_list[] = {
	{"iav",		AMBA_DEBUG_IAV},
	{"dsp",		AMBA_DEBUG_DSP},
	{"vin",		AMBA_DEBUG_VIN},
	{"vout",	AMBA_DEBUG_VOUT},
	{"aaa",		AMBA_DEBUG_AAA},
};

static const char * peripheral_table_list[] = {
	"spi",
	"i2c",
	"I2C",
};

static enum amba_debug_opmode opmode = AMBA_DEBUG_IDLE;
static int peripheral_opmode = 0;

static u32	op_address = 0;
static u32	op_size = 1;
static int	op_data = -1;
static u32	op_byte_width = 4;
static char	op_filename[MAX_STRING_LEN] = "";
static u32	op_set_mask = 0;
static u32	op_clr_mask = 0;
static u32	op_map = 0;
static u32	op_data_valid = 0;
static u32	op_verbose = 0;
static int	op_gpioid = -1;
static char	op_peripheral[MAX_STRING_LEN] = "";
static char	op_cfgname[MAX_STRING_LEN] = "";


/*========================== Functions =======================================*/
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))

static void set_debug_flag(const char *mode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mask_table_list); i++)
		if (strcmp(mask_table_list[i].name, mode) == 0)
			op_set_mask |= mask_table_list[i].mask;
}

static void clr_debug_flag(const char *mode)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mask_table_list); i++)
		if (strcmp(mask_table_list[i].name, mode) == 0)
			op_clr_mask |= mask_table_list[i].mask;
}

static void usage(void)
{
	int i;

	printf("amba_debug usage:\n");
	for (i = 0; i < ARRAY_SIZE(long_options) - 1; i++) {
		printf("--%s", long_options[i].name);
		if (long_options[i].val != 0)
			printf("/-%c", long_options[i].val);
		if (hint[i].arg[0] != 0)
			printf(" <%s>", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("debug level:\n");
	for (i = 0; i < ARRAY_SIZE(mask_table_list); i++)
		printf("\t%s\n", mask_table_list[i].name);

	printf("peripheral name:\n");
	for (i = 0; i < ARRAY_SIZE(peripheral_table_list); i++)
		printf("\t%s\n", peripheral_table_list[i]);
}

static int init_param(int argc, char **argv)
{
	int rval = 0, errorCode = 0;
	int ch, option_index = 0;

	opterr = 0;

	while (1) {
		ch = getopt_long(argc, argv,
				short_options, long_options, &option_index);
		if (ch < 0)
			break;

		switch (ch) {
		case 'r':
			opmode = AMBA_DEBUG_READ;
			op_address = strtoul(optarg, NULL, 0);
			break;

		case 'w':
			opmode = AMBA_DEBUG_WRITE;
			op_address = strtoul(optarg, NULL, 0);
			break;

		case 'b':
			op_byte_width = strtoul(optarg, NULL, 0);
			break;

		case 'd':
			op_data = strtoul(optarg, NULL, 0);
			op_data_valid = 1;
			break;

		case 's':
			op_size = strtoul(optarg, NULL, 0);
			break;

		case 'f':
			strcpy(op_filename, optarg);
			break;

		case 'v':
			op_verbose = strtoul(optarg, NULL, 0);
			break;

		case 'E':
			opmode = AMBA_DEBUG_CHANGE_FLAG;
			set_debug_flag(optarg);
			break;

		case 'D':
			opmode = AMBA_DEBUG_CHANGE_FLAG;
			clr_debug_flag(optarg);
			break;

		case 'g':
			opmode = AMBA_DEBUG_GPIO;
			op_gpioid = strtoul(optarg, NULL, 0);
			break;

		case 'm':
			op_map = strtoul(optarg, NULL, 0);
			break;

		case 'p':
			peripheral_opmode = 1;
			strcpy(op_peripheral, optarg);
			break;

		case 'c':
			strcpy(op_cfgname, optarg);
			break;

		default:
			printf("unknown option found: %c\n", ch);
			errorCode = -1;
			break;
		}
	}

	if (op_verbose) {
		printf("opmode = %d\n",			opmode);
		printf("op_address = 0x%08x\n",		op_address);
		printf("op_map = 0x%02x\n",		op_map);
		printf("op_size = 0x%08x\n",		op_size);
		printf("op_data = 0x%08x\n",		op_data);
		printf("op_byte_width = 0x%08x\n",	op_byte_width);
		printf("op_filename: %s\n",		op_filename);
		printf("op_set_mask = 0x%08x\n",	op_set_mask);
		printf("op_clr_mask = 0x%08x\n",	op_clr_mask);
		printf("op_data_valid = %d\n",		op_data_valid);
		printf("op_gpioid = %d\n",		op_gpioid);
		printf("op_peripheral: %s\n",		op_peripheral);
		printf("rval = %d\n",			rval);
	}

	return errorCode;
}

static int dump_data(char *pmem, u32 start, u32 size_limit,
							u32 address, u32 size)
{
	u32			offset, data, i;
	int			fd;

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
		printf("size too big! 0x%x < 0x%x\n",
						size_limit, (offset + size));
		size = size_limit - offset;
	}
	pmem += offset;

	if (strlen(op_filename)) {
		if (strcmp(op_filename, "stdout") == 0) {
			fd = STDOUT_FILENO;
		}
		else if ((fd = open(op_filename,
				O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
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

			default:
				break;
			}
		}
		printf("\n");
	}

	return 0;
}

static int dump_vin(u32 address, u32 size, u32 map)
{
	int fd_debug;
	u32 i;
	int fd;
	struct amba_vin_test_reg_data reg_data;

	if ((fd_debug = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

	if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_SRC_ID,
		(address / AMBA_DEBUG_VIN_SRC_SIZE)) < 0) {
		perror("AMBA_DEBUG_IOC_VIN_SET_SRC_ID");
		return -1;
	}

	if (strlen(op_filename)) {
		if ((fd = open(op_filename,
				O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
			perror(op_filename);
			return -1;
		}

		for (i = address; i < (address + size); i++) {
			reg_data.reg = i;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_GET_REG_DATA,
							&reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_GET_REG_DATA");
				return -1;
			}
			if (write(fd, (void *)&reg_data.data,
						sizeof(reg_data.data)) < 0) {
				perror("write");
				return -1;
			}
		}

		close(fd);
	} else {
		for (i = address; i < (address + size); i++) {
			reg_data.reg = i;
			reg_data.regmap = map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_GET_REG_DATA,
							&reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_GET_REG_DATA");
				return -1;
			}

			printf("0x%x=0x%x\n", reg_data.reg, reg_data.data);
		}
	}

	close(fd_debug);

	return 0;
}

static int fill_data(char *pmem, u32 start, u32 size_limit,
							u32 address, u32 size)
{
	u32			offset, i;
	int			fd;

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
		printf("size too big! 0x%x < 0x%x\n",
						size_limit, (offset + size));
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

			default:
				break;
			}
		}
	}

	return 0;
}

static int fill_vin(u32 address, u32 size, u32 map)
{
	int fd_debug;
	u32 i;
	int fd;
	struct amba_vin_test_reg_data reg_data;

	if ((fd_debug = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

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
			if (read(fd, (void *)&reg_data.data,
						sizeof(reg_data.data)) < 0) {
				perror("read");
				return -1;
			}
			reg_data.reg	= i;
			reg_data.regmap	= map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_REG_DATA,
							&reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_SET_REG_DATA");
				return -1;
			}
		}

		close(fd);
	} else {
		for (i = address; i < (address + size); i++) {
			reg_data.reg	= i;
			reg_data.data	= op_data;
			reg_data.regmap	= map;
			if (ioctl(fd_debug, AMBA_DEBUG_IOC_VIN_SET_REG_DATA,
							&reg_data) < 0) {
				perror("AMBA_DEBUG_IOC_VIN_SET_REG_DATA");
				return -1;
			}
		}
	}

	close(fd_debug);

	return 0;
}

static int debug_spi()
{
	char				spi_dev[MAX_STRING_LEN];
	int				mode, bpw, baudrate;
	int				rw, size, spi_fd;
	u8				*w8 = NULL, *r8 = NULL;
	u16				*w16 = NULL, *r16 = NULL;
	int				i, confirm, ret = 0;
	u32				tmp;
	struct spi_ioc_transfer		tr;
	int				input_from_file;
	FILE				*spi_cfg = NULL;

	input_from_file = strlen(op_cfgname);

	if (!input_from_file) {
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
			printf("Input read/write mode(0-write & read,"
					"1-write only, 2-read only): ");
			scanf("%d", &rw);
		} while (rw < WRITE_THEN_READ || rw > READ_ONLY);

		/*size*/
		do {
			printf("Input transfer size(>0): ");
			scanf("%d", &size);
		} while (size <= 0);
	} else {
		spi_cfg = fopen(op_cfgname, "r");
		if (spi_cfg == NULL) {
			perror("Can't open op_filename");
			ret = -1;
			goto debug_spi_exit;
		}

		/*device*/
		fscanf(spi_cfg, "node: ");
		fscanf(spi_cfg, "%s\n", spi_dev);

		spi_fd = open(spi_dev, O_RDWR, 0);
		if (spi_fd < 0) {
			perror("Incorrect device node!\n");
			ret = -1;
			goto debug_spi_exit;
		}

		/*mode*/
		fscanf(spi_cfg, "mode: ");
		fscanf(spi_cfg, "%d\n", &mode);
		if (mode < 0 || mode > 3) {
			printf("Incorrect spi mode!\n");
			ret = -1;
			goto debug_spi_exit;
		}

		/*bpw*/
		fscanf(spi_cfg, "bpw: ");
		fscanf(spi_cfg, "%d\n", &bpw);
		if (bpw < 4 || bpw > 16) {
			printf("Incorrect bits per word!\n");
			ret = -1;
			goto debug_spi_exit;
		}

		/*baudrate*/
		fscanf(spi_cfg, "baudrate: ");
		fscanf(spi_cfg, "%d\n", &baudrate);
		if (baudrate < 10000 || baudrate > 10000000) {
			printf("Incorrect baudrate!\n");
			ret = -1;
			goto debug_spi_exit;
		}

		/*rw*/
		fscanf(spi_cfg, "rw: ");
		fscanf(spi_cfg, "%d\n", &rw);
		if (rw < WRITE_THEN_READ || rw > READ_ONLY) {
			printf("Incorrect rw mode!\n");
			ret = -1;
			goto debug_spi_exit;
		}

		/*size*/
		fscanf(spi_cfg, "size: ");
		fscanf(spi_cfg, "%d\n", &size);
		if (size <= 0) {
			printf("Incorrect transfer size!\n");
			ret = -1;
			goto debug_spi_exit;
		}
	}

	/* Memory Allocation */
	switch (rw) {
	case WRITE_THEN_READ:
		if (bpw <= 8) {
			w8 = (u8 *)malloc(size * sizeof(u8));
			r8 = (u8 *)malloc(size * sizeof(u8));
			if (w8 == NULL || r8 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		} else {
			w16 = (u16 *)malloc(size * sizeof(u16));
			r16 = (u16 *)malloc(size * sizeof(u16));
			if (w16 == NULL || r16 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		}
		break;

	case WRITE_ONLY:
		if (bpw <= 8) {
			w8 = (u8 *)malloc(size * sizeof(u8));
			if (w8 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		} else {
			w16 = (u16 *)malloc(size * sizeof(u16));
			if (w16 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		}
		break;

	case READ_ONLY:
		if (bpw <= 8) {
			r8 = (u8 *)malloc(size * sizeof(u8));
			if (r8 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		} else {
			r16 = (u16 *)malloc(size * sizeof(u16));
			if (r16 == NULL) {
				printf("Error: Not enough memory!\n");
				ret = -1;
				goto debug_spi_exit;
			}
		}
		break;

	default:
		break;
	}

	/* Data to write */
	if (!input_from_file) {
		if (rw == WRITE_THEN_READ || rw == WRITE_ONLY) {
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
						printf("data %d = 0x%02x\n",
								i + 1, w8[i]);
					else
						printf("data %d = 0x%04x\n",
								i + 1, w16[i]);
				}

				printf("Input data correct(1/0): ");
				scanf("%d", &confirm);
			} while (confirm == 0);
		}
	} else {
		if (rw == WRITE_THEN_READ || rw == WRITE_ONLY) {
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
					printf("data %d = 0x%02x\n",
								i + 1, w8[i]);
				else
					printf("data %d = 0x%04x\n",
								i + 1, w16[i]);
			}
		}
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("can't set spi mode!\n");
		goto debug_spi_exit;
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bpw);
	if (ret < 0) {
		perror("can't set bits per word!\n");
		goto debug_spi_exit;
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate);
	if (ret < 0) {
		perror("can't set max speed hz!\n");
		goto debug_spi_exit;
	}

	switch (rw) {
	case WRITE_THEN_READ:
		if (bpw <= 8) {
			tr.tx_buf	= (unsigned long)w8;
			tr.rx_buf	= (unsigned long)r8;
			tr.len		= size;
		} else {
			tr.tx_buf	= (unsigned long)w16;
			tr.rx_buf	= (unsigned long)r16;
			tr.len		= size << 1;
		}
		tr.delay_usecs		= 0;
		tr.speed_hz		= baudrate;
		tr.bits_per_word	= bpw;

		ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
		if (ret != tr.len) {
			perror("can't send spi message!\n");
			ret = -1;
			goto debug_spi_exit;
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

	case WRITE_ONLY:
		if (bpw <= 8) {
			ret = write(spi_fd, w8, size);
			if (ret != size) {
				perror("write error!\n");
				ret = -1;
				goto debug_spi_exit;
			}

			free(w8);
		} else {
			ret = write(spi_fd, w16, size << 1);
			if (ret != (size << 1)) {
				perror("write error!\n");
				ret = -1;
				goto debug_spi_exit;
			}

			free(w16);
		}
		break;

	case READ_ONLY:
		if (bpw <= 8) {
			ret = read(spi_fd, r8, size);
			if (ret != size) {
				perror("read error!\n");
				ret = -1;
				goto debug_spi_exit;
			}

			printf("Data received:\n");
			for (i = 0; i < size; i++)
				printf("read data %d = 0x%02x\n", i + 1, r8[i]);

			free(r8);
		} else {
			ret = read(spi_fd, r16, size << 1);
			if (ret != (size << 1)) {
				perror("read error!\n");
				ret = -1;
				goto debug_spi_exit;
			}

			printf("Data received:\n");
			for (i = 0; i < size; i++)
				printf("read data %d = 0x%04x\n", i + 1, r16[i]);

			free(r16);
		}
		break;

	default:
		break;
	}

	close(spi_fd);

debug_spi_exit:
	if (spi_cfg)
		fclose(spi_cfg);
	return ret;
}

static int debug_i2c(void)
{
	char				i2c_dev[MAX_STRING_LEN];
	int				mode, addr, r_size, w_size;
	int				i2c_fd, rw;
	u8				*w8 = NULL, *r8 = NULL;
	int				i, confirm, ret = 0, fd_opened = 0;
	u32				tmp;
	int				input_from_file;
	FILE				*i2c_cfg = NULL;

	input_from_file = strlen(op_cfgname);

	if (!input_from_file) {
		/*device*/
		do {
			printf("Input I2C device node: ");
			scanf("%s", i2c_dev);
			i2c_fd = open(i2c_dev, O_RDWR, 0);
		} while (i2c_fd < 0);
		fd_opened = 1;

		/*mode*/
		do {
			printf("Input I2C address bits(7/10): ");
			scanf("%d", &mode);
		} while (mode != 7 && mode != 10);

		if (mode == 7) {
			ret = ioctl(i2c_fd, I2C_TENBIT, 0);
			if (ret < 0) {
				perror("Error: Unable to set"
						"7-bit address mode!\n");
				goto debug_i2c_exit;
			}
		} else {
			ret = ioctl(i2c_fd, I2C_TENBIT, 1);
			if (ret < 0) {
				perror("Error: Unable to set "
						"10-bit address mode!\n");
				goto debug_i2c_exit;
			}
		}

		/*addr*/
		do {
			printf("Input I2C address(>=0): ");
			scanf("%d", &addr);
		} while (addr < 0);

		ret = ioctl(i2c_fd, I2C_SLAVE, addr >> 1);
		if (ret < 0) {
			perror("Error: Unable to set slave address!\n");
			goto debug_i2c_exit;
		}

		/*rw*/
		do {
			printf("Input read/write mode(0-write then read, "
						"1-write only, 2-read only): ");
			scanf("%d", &rw);
		} while (rw < WRITE_THEN_READ || rw > READ_ONLY);

		/*size*/
		switch (rw) {
		case WRITE_THEN_READ:
			do {
				printf("Input write size(>0) "
							"and read size(>0): ");
				scanf("%d %d", &w_size, &r_size);
			} while (w_size <= 0 || r_size <= 0);
			break;

		case WRITE_ONLY:
			do {
				printf("Input write size(>0): ");
				scanf("%d", &w_size);
			} while (w_size <= 0);
			break;

		case READ_ONLY:
			do {
				printf("Input read size(>0): ");
				scanf("%d", &r_size);
			} while (r_size <= 0);
			break;

		default:
			break;
		}
	} else {
		i2c_cfg = fopen(op_cfgname, "r");
		if (i2c_cfg == NULL) {
			perror("Can't open op_filename");
			ret = -1;
			goto debug_i2c_exit;
		}

		/*device*/
		fscanf(i2c_cfg, "node: ");
		fscanf(i2c_cfg, "%s\n", i2c_dev);
		i2c_fd = open(i2c_dev, O_RDWR, 0);
		if (i2c_fd < 0) {
			perror("Incorrect i2c_dev!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		fd_opened = 1;

		/*mode*/
		fscanf(i2c_cfg, "mode: ");
		fscanf(i2c_cfg, "%d\n", &mode);
		if (mode != 7 && mode != 10) {
			printf("Incorrect i2c address mode!\n");
			ret = -1;
			goto debug_i2c_exit;
		}

		if (mode == 7) {
			ret = ioctl(i2c_fd, I2C_TENBIT, 0);
			if (ret < 0) {
				perror("Error: Unable to set"
						" 7-bit address mode!\n");
				goto debug_i2c_exit;
			}
		} else {
			ret = ioctl(i2c_fd, I2C_TENBIT, 1);
			if (ret < 0) {
				perror("Error: Unable to set"
						" 10-bit address mode!\n");
				goto debug_i2c_exit;
			}
		}

		/*addr*/
		fscanf(i2c_cfg, "addr: ");
		fscanf(i2c_cfg, "%d\n", &addr);
		if (addr < 0) {
			printf("Incorrect i2c address!\n");
			ret = -1;
			goto debug_i2c_exit;
		}

		ret = ioctl(i2c_fd, I2C_SLAVE, addr >> 1);
		if (ret < 0) {
			perror("Error: Unable to set slave address!\n");
			goto debug_i2c_exit;
		}

		/*rw*/
		fscanf(i2c_cfg, "rw: ");
		fscanf(i2c_cfg, "%d\n", &rw);
		if (rw < WRITE_THEN_READ || rw > READ_ONLY) {
			printf("Incorrect i2c rw!\n");
			ret = -1;
			goto debug_i2c_exit;
		}

		/*size*/
		fscanf(i2c_cfg, "size: ");
		switch (rw) {
		case WRITE_THEN_READ:
			fscanf(i2c_cfg, "%d %d\n", &w_size, &r_size);
			if (w_size <= 0 || r_size <= 0) {
				printf("Incorrect size!\n");
				ret = -1;
				goto debug_i2c_exit;
			}
			break;

		case WRITE_ONLY:
			fscanf(i2c_cfg, "%d\n", &w_size);
			if (w_size <= 0) {
				printf("Incorrect size!\n");
				ret = -1;
				goto debug_i2c_exit;
			}
			break;

		case READ_ONLY:
			fscanf(i2c_cfg, "%d\n", &r_size);
			if (r_size <= 0) {
				printf("Incorrect size!\n");
				ret = -1;
				goto debug_i2c_exit;
			}
			break;

		default:
			break;
		}
	}

	/*memory*/
	switch (rw) {
	case WRITE_THEN_READ:
		w8 = (u8 *)malloc(w_size * sizeof(u8));
		r8 = (u8 *)malloc(r_size * sizeof(u8));
		if (w8 == NULL || r8 == NULL) {
			printf("Error: Not enough memory!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		break;

	case WRITE_ONLY:
		w8 = (u8 *)malloc(w_size * sizeof(u8));
		if (w8 == NULL) {
			printf("Error: Not enough memory!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		break;

	case READ_ONLY:
		r8 = (u8 *)malloc(r_size * sizeof(u8));
		if (r8 == NULL) {
			printf("Error: Not enough memory!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		break;

	default:
		break;
	}

	/* Data to write */
	if (!input_from_file) {
		if (rw == WRITE_THEN_READ || rw == WRITE_ONLY) {
			do {
				for (i = 0; i < w_size; i++) {
					printf("data %d: ", i + 1);
					scanf("%u", &tmp);
					w8[i] = (u8)tmp;
				}

				printf("You've input:\n");
				for (i = 0; i < w_size; i++) {
					printf("data %d = 0x%02x\n",
								i + 1, w8[i]);
				}

				printf("Input data correct(1/0): ");
				scanf("%d", &confirm);
			} while (confirm == 0);
		}
	} else {
		if (rw == WRITE_THEN_READ || rw == WRITE_ONLY) {
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
	}

	switch (rw) {
	case WRITE_THEN_READ:
		ret = write(i2c_fd, w8, w_size);
		if (ret != w_size) {
			perror("Error: Unable to write i2c!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		free(w8);

		ret = read(i2c_fd, r8, r_size);
		if (ret != r_size) {
			perror("Error: Unable to read i2c!\n");
			ret = -1;
			goto debug_i2c_exit;
		}

		printf("Data received:\n");
		for (i = 0; i < r_size; i++)
			printf("read data %d = 0x%02x\n", i + 1, r8[i]);

		free(r8);
		break;

	case WRITE_ONLY:
		ret = write(i2c_fd, w8, w_size);
		if (ret != w_size) {
			perror("Error: Unable to write i2c!\n");
			ret = -1;
			goto debug_i2c_exit;
		}
		free(w8);
		break;

	case READ_ONLY:
		ret = read(i2c_fd, r8, r_size);
		if (ret != r_size) {
			perror("Error: Unable to read i2c!\n");
			ret = -1;
			goto debug_i2c_exit;
		}

		printf("Data received:\n");
		for (i = 0; i < r_size; i++)
			printf("read data %d = 0x%02x\n", i + 1, r8[i]);

		free(r8);
		break;

	default:
		break;
	}

debug_i2c_exit:
	if (fd_opened)
		close(i2c_fd);
	if (i2c_cfg)
		fclose(i2c_cfg);
	return ret;
}

static int debug_amba(void)
{
	int				fd, debug_flag;
	char				*debug_mem = NULL;
	struct amba_vin_test_gpio	gpio;
	u32				map_start = 0;

	if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
		perror("/dev/ambad");
		return -1;
	}

	if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
		perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
		return -1;
	}
	if (op_verbose)
		printf("debug_flag 0x%08x\n", debug_flag);

	if (opmode == AMBA_DEBUG_READ || opmode == AMBA_DEBUG_WRITE) {
		map_start = op_address;
		map_start >>= PAGE_SHIFT;
		map_start <<= PAGE_SHIFT;
		debug_mem = (char *)mmap(NULL, op_address - map_start + op_size,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_start);
		if ((int)debug_mem < 0) {
			perror("mmap");
			return -1;
		}

		if (op_verbose)
			printf("mmap returns 0x%08x\n", (unsigned)debug_mem);
	}

	switch (opmode) {
	case AMBA_DEBUG_READ:
		if (op_verbose) {
			printf("debug read: op_address = 0x%08x map_start = "
				"0x%08x op_size = %d\n",
				op_address, map_start, op_size);
		}
		dump_data(debug_mem, map_start, 0xffffffff, op_address, op_size);
		break;

	case AMBA_DEBUG_WRITE:
		if (op_verbose) {
			printf("debug write: op_address = 0x%08x map_start = "
				"0x%08x op_size = %d\n",
				op_address, map_start, op_size);
		}
		fill_data(debug_mem, map_start, 0xffffffff, op_address, op_size);
		break;

	case AMBA_DEBUG_CHANGE_FLAG:
		if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
			return -1;
		}
		debug_flag &= (~op_clr_mask);
		debug_flag |= op_set_mask;
		if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
			perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
			return -1;
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
				return -1;
			}
			if (op_verbose) {
				printf("id 0x%08x\n", gpio.id);
				printf("data 0x%08x\n", gpio.data);
			}
		} else {
			if (ioctl(fd, AMBA_DEBUG_IOC_GET_GPIO, &gpio) < 0) {
				perror("AMBA_DEBUG_IOC_SET_GPIO");
				return -1;
			}
			printf("id 0x%08x\n", gpio.id);
			printf("data 0x%08x\n", gpio.data);
		}
		break;

	default:
		printf("Wrong opmode %d\n", opmode);
		break;
	}

	if (debug_mem)
		munmap(debug_mem, op_size);

	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
	/* Init Parameters */
	if (argc < 2 || init_param(argc, argv)) {
		usage();
		return -1;
	}

	if (peripheral_opmode) {
		if (strcmp(op_peripheral, "spi") == 0) {
			debug_spi();
		} else if (strcmp(op_peripheral, "i2c") == 0) {
			debug_i2c();
		} else if (strcmp(op_peripheral, "vin") == 0) {
			if (opmode == AMBA_DEBUG_READ) {
				dump_vin(op_address, op_size, op_map);
			} else if (opmode == AMBA_DEBUG_WRITE) {
				fill_vin(op_address, op_size, op_map);
			}
		}
	} else {
		debug_amba();
	}

	return 0;
}

