/**
 * @file system/include/flash/spi_nor/fl01gs.h
 *
 * History:
 *    2014/05/05 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FL01GS_H__
#define __FL01GS_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "Spansion SPI Nor FL01GSA flash"

/* fl01gs specific command */
#define FL01GS_BRRD			0x16
#define FL01GS_BRWR			0x17
#define FL01GS_RESET_ENABLE		0xF0

/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINOR_CHIP_SIZE		(64 * 1024 * 1024)
#define SPINOR_SECTOR_SIZE		(256 * 1024)
#define SPINOR_PAGE_SIZE		512

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		1

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		1
#endif

