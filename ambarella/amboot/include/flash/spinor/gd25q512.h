/**
 * @file system/include/flash/spi_nor/gd25q512.h
 *
 * History:
 *    2015/06/25 - [Ken He] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __GD25Q512_H__
#define __GD25Q512_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "GigaDevice Nor GD25Q512 flash"

/* MX25L25645G specific command */
#define FLASH_RESET_ENABLE		0x66
#define FLASH_RESET_MEMORY		0x99
#define FLASH_ENTER_4BYTES		0xB7
#define FLASH_EXIT_4BYTES		0xE9

/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINOR_CHIP_SIZE		(64 * 1024 * 1024)
#define SPINOR_SECTOR_SIZE		(64 * 1024)
#define SPINOR_PAGE_SIZE		256

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		0

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		0
#endif

