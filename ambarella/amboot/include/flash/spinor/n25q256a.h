/**
 * @file system/include/flash/spi_nor/n25q256a.h
 *
 * History:
 *    2014/08/15 - [Ken He] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __N25Q256A_H__
#define __N25Q256A_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "Micron SPI Nor N25Q256A flash"

/* N25Q256A specific command */
#define N25Q256A_RESET_ENABLE		0x66
#define N25Q256A_RESET_MEMORY		0x99
#define N25Q256A_ENTER_4BYTES		0xb7
#define N25Q256A_EXIT_4BYTES		0xe9

/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINOR_CHIP_SIZE		(32 * 1024 * 1024)
#define SPINOR_SECTOR_SIZE		(64 * 1024)
#define SPINOR_PAGE_SIZE		256

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		1

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		2
#endif

