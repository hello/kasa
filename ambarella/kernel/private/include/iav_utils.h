/*
 * iav_util.h
 *
 * History:
 *	2008/1/25 - [Oliver Li] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#ifndef __IAV_UTILS_H__
#define __IAV_UTILS_H__

#include <iav_config.h>
#include "msg_print.h"
#define KB			(1024)
#define MB			(1024 * 1024)
#define KB_ALIGN(addr)		ALIGN(addr, KB)
#define MB_ALIGN(addr)		ALIGN(addr, MB)

#include <plat/iav_helper.h>
#define clean_d_cache		ambcache_clean_range
#define invalidate_d_cache	ambcache_inv_range

/* ARM physical address to DSP address */
#define PHYS_TO_DSP(addr)	(unsigned long)(addr)
#define VIRT_TO_DSP(addr)	PHYS_TO_DSP(virt_to_phys(addr))			// kernel virtual address to DSP address

/* DSP address to ARM physical address */

#define DSP_TO_PHYS(addr)	(unsigned long)(addr)
#define DSP_TO_VIRT(addr)	phys_to_virt(DSP_TO_PHYS(addr))			// DSP address to kernel virtual address

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
#define vin_debug(str, arg...)	iav_debug("VIN: "str, ##arg)
#endif
#define vin_printk(str, arg...)	iav_printk(str, ##arg)
#define vin_error(str, arg...)	iav_error("VIN: "str, ##arg)
#define vin_warn(str, arg...)	iav_warn("VIN: "str, ##arg)
#define vin_info(str, arg...)	iav_info("VIN: "str, ##arg)

#include <plat/iav_helper.h>
static inline u32 ambarella_clk_get_rate(char *name)
{
	struct ambsvc_pll pll_svc;

	pll_svc.svc_id = AMBSVC_PLL_GET_RATE;
	pll_svc.name = name;
	ambarella_request_service(AMBARELLA_SERVICE_PLL, &pll_svc, NULL);

	return pll_svc.rate;
}

#endif	// UTIL_H

