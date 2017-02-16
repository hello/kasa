/**
 * @file system/include/flash/spi_nor/w25q64fv.h
 *
 * History:
 *    2014/07/25 - [Ken He] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __W25Q64FV_H__
#define __W25Q64FV_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "WINBOND SPI Nor W25Q64FV flash"

/* w25q64fv specific command */
#define W25Q64FV_RESET_ENABLE		0x66
#define W25Q64FV_RESET				0x99

/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINOR_CHIP_SIZE		(8 * 1024 * 1024)
/* W25Q64FV has two erase cmd,for 4KB and 64KB, we use 64K*/
#define SPINOR_SECTOR_SIZE		(64 * 1024)
#define SPINOR_PAGE_SIZE		256

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		0

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		0

#endif

