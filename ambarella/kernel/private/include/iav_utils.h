/*
 * iav_util.h
 *
 * History:
 *	2008/1/25 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 */

#ifndef __IAV_UTILS_H__
#define __IAV_UTILS_H__

#include <iav_config.h>
#include "msg_print.h"
#define KB			(1024)
#define MB			(1024 * 1024)
#define KB_ALIGN(addr)		ALIGN(addr, KB)
#define MB_ALIGN(addr)		ALIGN(addr, MB)

#define clean_d_cache		ambcache_clean_range
#define invalidate_d_cache	ambcache_inv_range

/* ARM physical address to DSP address */
#if defined(CONFIG_PLAT_AMBARELLA_MEM_START_LOW)
#define PHYS_TO_DSP(addr)	(u32)(addr)
#else
#define PHYS_TO_DSP(addr)	(u32)((u32)(addr) & 0x3FFFFFFF)
#endif
#define VIRT_TO_DSP(addr)	PHYS_TO_DSP(virt_to_phys(addr))			// kernel virtual address to DSP address
#define AMBVIRT_TO_DSP(addr)	PHYS_TO_DSP(ambarella_virt_to_phys((u32)addr))	// ambarella virtual address to DSP address

/* DSP address to ARM physical address */
#if defined(CONFIG_PLAT_AMBARELLA_MEM_START_LOW)
#define DSP_TO_PHYS(addr)	(u32)(addr)
#else
#define DSP_TO_PHYS(addr)	(u32)((u32)(addr) | 0xC0000000)
#endif
#define DSP_TO_VIRT(addr)	phys_to_virt(DSP_TO_PHYS(addr))			// DSP address to kernel virtual address
#define DSP_TO_AMBVIRT(addr)	ambarella_phys_to_virt(DSP_TO_PHYS((u32)addr))	// DSP address to ambarella virtual address

#ifndef DRV_PRINT
#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif
#endif

#define iav_debug(str, arg...)	DRV_PRINT(KERN_DEBUG "%s(%d): "str, __func__, __LINE__, ##arg)
#define iav_printk(str, arg...)	DRV_PRINT(KERN_INFO "%s(%d): "str, __func__, __LINE__, ##arg)
#define iav_error(str, arg...)	DRV_PRINT(KERN_ERR "%s(%d): "str, __func__, __LINE__, ##arg)
#define iav_warn(str, arg...)	DRV_PRINT(KERN_WARNING "%s(%d): "str, __func__, __LINE__, ##arg)
#define iav_info(str...)	DRV_PRINT(KERN_INFO str)

#ifndef CONFIG_AMBARELLA_VIN_DEBUG
#define vin_debug(format, arg...)
#else
#define vin_debug(format, arg...)		\
	if (ambarella_debug_level & AMBA_DEBUG_VIN)	\
		DRV_PRINT(KERN_DEBUG "%s(%d): "str, __func__, __LINE__, ##arg)
#endif
#define vin_printk(str, arg...)	iav_printk(str, ##arg)
#define vin_error(str, arg...)	iav_error("VIN: "str, ##arg)
#define vin_warn(str, arg...)	iav_warn("VIN: "str, ##arg)
#define vin_info(str, arg...)	iav_info("VIN: "str, ##arg)


#endif	// UTIL_H

