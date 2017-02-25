/*#############################################################################
 * Copyright 2014-2015 Sony Corporation.
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Sony Corporation.
 * No part of this file may be copied, modified, sold, and distributed in any
 * form or by any means without prior explicit permission in writing from
 * Sony Corporation.
 * @file sonysd_kernel.c
 */
/*###########################################################################*/

#include <linux/types.h>
#include <linux/major.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/ioctl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "sonysd.h"
#include "sonysd_local.h"


/* from core.h */
#define MMC_RSP_SPI_S1 (1 << 7)
#define MMC_RSP_SPI_R1	(MMC_RSP_SPI_S1)

#define MMC_RSP_PRESENT	(1 << 0)
#define MMC_RSP_CRC	(1 << 2)
#define MMC_RSP_OPCODE	(1 << 4)
#define MMC_RSP_R1	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMC_CMD_ADTC	(1 << 5)


int sonysd_card_lock(int fd){
#ifdef MMC_OP_MULTI_BIT
	struct mmc_ioc_cmd cmd;
	char buf[SECTORSZ];

	memset(&cmd, 0, sizeof(cmd));
	memset(buf, 0, SECTORSZ);

	cmd.write_flag = 0;
	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.opcode |= MMC_OP_MULTI_FIRST;

	cmd.arg = 0; /* 0th sector */
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.blksz = SECTORSZ;
	cmd.blocks = 1;
	mmc_ioc_cmd_set_data(cmd, buf);

	return ioctl(fd, MMC_IOC_CMD, &cmd);
#else
	fd = fd;
	return 0;
#endif
}


int sonysd_card_unlock(int fd){
	struct mmc_ioc_cmd cmd;
	char buf[SECTORSZ];

	memset(&cmd, 0, sizeof(cmd));
	memset(buf, 0, SECTORSZ);

	cmd.write_flag = 0;
	cmd.opcode = MMC_READ_SINGLE_BLOCK;
#ifdef MMC_OP_MULTI_BIT
	cmd.opcode |= MMC_OP_MULTI_LAST;
#endif

	cmd.arg = 0; /* 0th sector */
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.blksz = SECTORSZ;
	cmd.blocks = 1;
	mmc_ioc_cmd_set_data(cmd, buf);

	return ioctl(fd, MMC_IOC_CMD, &cmd);
}


int sonysd_readsector(int fd, unsigned char* buf, unsigned long addr){
	struct mmc_ioc_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	memset(buf, 0, SECTORSZ);

	cmd.write_flag = 0;
	cmd.opcode = MMC_READ_SINGLE_BLOCK;
#ifdef MMC_OP_MULTI_BIT
	cmd.opcode |= MMC_OP_MULTI_MIDDLE;
#endif

	cmd.arg = addr;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.blksz = SECTORSZ;
	cmd.blocks = 1;
	mmc_ioc_cmd_set_data(cmd, buf);

	return ioctl(fd, MMC_IOC_CMD, &cmd);
}


int sonysd_vscmd(int fd){
	struct mmc_ioc_cmd cmd;
	char buf[SECTORSZ];

	memset(&cmd, 0, sizeof(cmd));
	memset(buf, 0, SECTORSZ);

	cmd.write_flag = 0;
	cmd.opcode = 56;
#ifdef MMC_OP_MULTI_BIT
	cmd.opcode |= MMC_OP_MULTI_MIDDLE;
#endif

	cmd.arg = 0x534E5901;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	cmd.blksz = SECTORSZ;
	cmd.blocks = 1;
	mmc_ioc_cmd_set_data(cmd, buf);

	return ioctl(fd, MMC_IOC_CMD, &cmd);
}


