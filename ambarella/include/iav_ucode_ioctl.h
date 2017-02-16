
/*
 * iav_ucode_ioctl.h
 *
 * History:
 *	2012/10/25 - [Cao Rongrong] Created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __UCODE_IOCTL_ARCH_H__
#define __UCODE_IOCTL_ARCH_H__

#include <linux/ioctl.h>

#define UCODE_LOAD_ITEM_MAX		4

typedef struct ucode_load_item_s {
	unsigned long		addr_offset;
	char			filename[20];
} ucode_load_item_t;


typedef struct ucode_load_info_s {
	unsigned long		map_size;
	int			nr_item;
	ucode_load_item_t	items[UCODE_LOAD_ITEM_MAX];
} ucode_load_info_t;


typedef struct ucode_version_s {
	u16	year;
	u8	month;
	u8	day;
	u32	edition_num;
	u32	edition_ver;
} ucode_version_t;


/*
 * Ucode IOCTL definitions
 */
#define UCODE_IOC_MAGIC		'u'

typedef enum {
	// Basic functions (0x00 ~ 0x2F)
	IOC_GET_UCODE_INFO = 0x00,
	IOC_GET_UCODE_VERSION = 0x01,
	IOC_UPDATE_UCODE = 0x02,

	// Reserved: 0x30 ~ 0xFF
} UCODE_IOC;

#define IAV_IOC_GET_UCODE_INFO		_IOR(UCODE_IOC_MAGIC, IOC_GET_UCODE_INFO, ucode_load_info_t)
#define IAV_IOC_GET_UCODE_VERSION	_IOR(UCODE_IOC_MAGIC, IOC_GET_UCODE_VERSION, ucode_version_t)
#define IAV_IOC_UPDATE_UCODE		_IOW(UCODE_IOC_MAGIC, IOC_UPDATE_UCODE, int)

#endif

