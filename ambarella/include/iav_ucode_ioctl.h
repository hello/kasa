
/*
 * iav_ucode_ioctl.h
 *
 * History:
 *	2012/10/25 - [Cao Rongrong] Created file
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

#ifndef __UCODE_IOCTL_ARCH_H__
#define __UCODE_IOCTL_ARCH_H__

#include <linux/ioctl.h>

#define UCODE_LOAD_ITEM_MAX		4

typedef struct ucode_load_item_s {
	unsigned long		addr_offset;
	char			filename[24];
} ucode_load_item_t;


typedef struct ucode_load_info_s {
	unsigned long		map_size;
	unsigned long		nr_item;
	ucode_load_item_t	items[UCODE_LOAD_ITEM_MAX];
} ucode_load_info_t;


typedef struct ucode_version_s {
	u16	year;
	u8	month;
	u8	day;
	u32	edition_num;
	u32	edition_ver;
	u32	chip_arch;
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

enum ucode_arch {
	UCODE_ARCH_UNKNOWN = 0xFFFF,
	UCODE_ARCH_A5S = 0,
	UCODE_ARCH_S2 = 1,
	UCODE_ARCH_S2L = 2,
	UCODE_ARCH_S2E = 3,
	UCODE_ARCH_S3L = 4,
	UCODE_ARCH_S5 = 5,
	UCODE_ARCH_S5L = 6,

	UCODE_ARCH_NUM,
	UCODE_ARCH_FIRST = 0,
	UCODE_ARCH_LAST = UCODE_ARCH_NUM,
};

#define IAV_IOC_GET_UCODE_INFO		_IOR(UCODE_IOC_MAGIC, IOC_GET_UCODE_INFO, ucode_load_info_t)
#define IAV_IOC_GET_UCODE_VERSION	_IOR(UCODE_IOC_MAGIC, IOC_GET_UCODE_VERSION, ucode_version_t)
#define IAV_IOC_UPDATE_UCODE		_IOW(UCODE_IOC_MAGIC, IOC_UPDATE_UCODE, int)

#endif

