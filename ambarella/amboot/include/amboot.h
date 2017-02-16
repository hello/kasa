/**
 * amboot.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBOOT_H__
#define __AMBOOT_H__

/*===========================================================================*/
#include <config.h>
#include <basedef.h>

/*===========================================================================*/
#define AMBARELLA_BOARD_TYPE_AUTO		(0)
#define AMBARELLA_BOARD_TYPE_BUB		(1)
#define AMBARELLA_BOARD_TYPE_EVK		(2)
#define AMBARELLA_BOARD_TYPE_IPCAM		(3)
#define AMBARELLA_BOARD_TYPE_VENDOR		(4)
#define AMBARELLA_BOARD_TYPE_ATB		(5)

#define AMBARELLA_BOARD_CHIP_AUTO		(0)

#define AMBARELLA_BOARD_REV_AUTO		(0)

#define AMBARELLA_BOARD_VERSION(c,t,r)		(((c) << 16) + ((t) << 12) + (r))
#define AMBARELLA_BOARD_CHIP(v)			(((v) >> 16) & 0xFFFF)
#define AMBARELLA_BOARD_TYPE(v)			(((v) >> 12) & 0xF)
#define AMBARELLA_BOARD_REV(v)			(((v) >> 0) & 0xFFF)

#include <bsp.h>

/*===========================================================================*/
#include <ambhw/chip.h>
#include <ambhw/rct.h>
#include <ambhw/memory.h>

/* ==========================================================================*/
#ifndef __ASM__

#endif
/* ==========================================================================*/

#endif

