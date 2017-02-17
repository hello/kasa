/**
 * @file system/include/flash/slcnand/parts.h
 *
 * History:
 *    2009/08/17 - [Chien-Yang Chen] created file
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


#ifndef __SLCNAND_PARTS_H__
#define __SLCNAND_PARTS_H__

#if   defined(CONFIG_NAND_K9F1208)
#include <flash/slcnand/k9f1208.h>
#elif defined(CONFIG_NAND_K9F1208X0C)
#include <flash/slcnand/k9f1208x0c.h>
#elif defined(CONFIG_NAND_K9W8G08)
#include <flash/slcnand/k9w8g08.h>
#elif defined(CONFIG_NAND_K9K4G08)
#include <flash/slcnand/k9k4g08.h>
#elif defined(CONFIG_NAND_K9F5608)
#include <flash/slcnand/k9f5608.h>
#elif defined(CONFIG_NAND_K9F2808)
#include <flash/slcnand/k9f2808.h>
#elif defined(CONFIG_NAND_HY27US08281A)
#include <flash/slcnand/hy27us08281a.h>
#elif defined(CONFIG_NAND_HY27US08561A)
#include <flash/slcnand/hy27us08561a.h>
#elif defined(CONFIG_NAND_HY27US08561M)
#include <flash/slcnand/hy27us08561m.h>
#elif defined(CONFIG_NAND_HY27US08121M)
#include <flash/slcnand/hy27us08121m.h>
#elif defined(CONFIG_NAND_HY27US08121A)
#include <flash/slcnand/hy27us08121a.h>
#elif defined(CONFIG_NAND_HY27UF081G2A)
#include <flash/slcnand/hy27uf081g2a.h>
#elif defined(CONFIG_NAND_HY27UF082G2A)
#include <flash/slcnand/hy27uf082g2a.h>
#elif defined(CONFIG_NAND_HY27UF082G2B)
#include <flash/slcnand/hy27uf082g2b.h>
#elif defined(CONFIG_NAND_HY27UF084G2B)
#include <flash/slcnand/hy27uf084g2b.h>
#elif defined(CONFIG_NAND_HY27UF084G2M)
#include <flash/slcnand/hy27uf084g2m.h>
#elif defined(CONFIG_NAND_HY27U1G8F2B)
#include <flash/slcnand/hy27u1g8f2b.h>
#elif defined(CONFIG_NAND_H27U518S2c)
#include <flash/slcnand/h27u518s2c.h>
#elif defined(CONFIG_NAND_HY27US4G86F2D)
#include <flash/slcnand/hy27us4g86f2d.h>
#elif defined(CONFIG_NAND_H27U2G8F2D)
#include <flash/slcnand/h27u2g8f2d.h>
#elif defined(CONFIG_NAND_H27U2G8F2C)
#include <flash/slcnand/h27u2g8f2c.h>
#elif defined(CONFIG_NAND_K9K8G08)
#include <flash/slcnand/k9k8g08.h>
#elif defined(CONFIG_NAND_K9WAG08)
#include <flash/slcnand/k9wag08.h>
#elif defined(CONFIG_NAND_K9NBG08)
#include <flash/slcnand/k9nbg08.h>
#elif defined(CONFIG_NAND_K9F1G08)
#include <flash/slcnand/k9f1g08.h>
#elif defined(CONFIG_NAND_K9F1G08U0B)
#include <flash/slcnand/k9f1g08u0b.h>
#elif defined(CONFIG_NAND_K9F1G08U0E)
#include <flash/slcnand/k9f1g08u0e.h>
#elif defined(CONFIG_NAND_K9F4G08U0E)
#include <flash/slcnand/k9f4g08u0e.h>
#elif defined(CONFIG_NAND_K9F2G08)
#include <flash/slcnand/k9f2g08.h>
#elif defined(CONFIG_NAND_ST128W3A)
#include <flash/slcnand/st128w3a.h>
#elif defined(CONFIG_NAND_ST256W3A)
#include <flash/slcnand/st256w3a.h>
#elif defined(CONFIG_NAND_ST512W3A)
#include <flash/slcnand/st512w3a.h>
#elif defined(CONFIG_NAND_ST01GW3A)
#include <flash/slcnand/st01gw3a.h>
#elif defined(CONFIG_NAND_ST01GW3B)
#include <flash/slcnand/st01gw3b.h>
#elif defined(CONFIG_NAND_ST02GW3B)
#include <flash/slcnand/st02gw3b.h>
#elif defined(CONFIG_NAND_TC58BVG0S3H)
#include <flash/slcnand/tc58bvg0s3h.h>
#elif defined(CONFIG_NAND_TC58BVG1S3H)
#include <flash/slcnand/tc58bvg1s3h.h>
#elif defined(CONFIG_NAND_TC58DVM72A)
#include <flash/slcnand/tc58dvm72a.h>
#elif defined(CONFIG_NAND_TC58DVM82A)
#include <flash/slcnand/tc58dvm82a.h>
#elif defined(CONFIG_NAND_TC58DVM92A)
#include <flash/slcnand/tc58dvm92a.h>
#elif defined(CONFIG_NAND_TC58NVG0S3C)
#include <flash/slcnand/tc58nvg0s3c.h>
#elif defined(CONFIG_NAND_TC58NVG0S3E)
#include <flash/slcnand/tc58nvg0s3e.h>
#elif defined(CONFIG_NAND_TC58NVG0S3H)
#include <flash/slcnand/tc58nvg0s3h.h>
#elif defined(CONFIG_NAND_TC58NVG1S3H)
#include <flash/slcnand/tc58nvg1s3h.h>
#elif defined(CONFIG_NAND_TC58NVG1S3E)
#include <flash/slcnand/tc58nvg1s3e.h>
#elif defined(CONFIG_NAND_TC58NVG2S3E)
#include <flash/slcnand/tc58nvg2s3e.h>
#elif defined(CONFIG_NAND_TC58NVM9S3C)
#include <flash/slcnand/tc58nvm9s3c.h>
#elif defined(CONFIG_NAND_MT29F2G08AAC)
#include <flash/slcnand/mt29f2g08aac.h>
#elif defined(CONFIG_NAND_MT29F1G08ABAEA)
#include <flash/slcnand/mt29f1g08abaea.h>
#elif defined(CONFIG_NAND_MT29F2G08ABA)
#include <flash/slcnand/mt29f2g08aba.h>
#elif defined(CONFIG_NAND_MT29F8G08DAA)
#include <flash/slcnand/mt29f8g08daa.h>
#elif defined(CONFIG_NAND_MT29F4G08ABADA)
#include <flash/slcnand/mt29f4g08abada.h>
#elif defined(CONFIG_NAND_MT29F4G08ABBDA)
#include <flash/slcnand/mt29f4g08abbda.h>
#elif defined(CONFIG_NAND_MT29F2G08ABAFA)
#include <flash/slcnand/mt29f2g08abafa.h>
#elif defined(CONFIG_NAND_MT29F2G08ABBEA)
#include <flash/slcnand/mt29f2g08abbea.h>
#elif defined(CONFIG_NAND_NUMONYX02GW3B2D)
#include <flash/slcnand/numonyx02gw3b2d.h>
#elif defined(CONFIG_NAND_CT48248NS486G1)
#include <flash/slcnand/ct48248ns486g1.h>
#elif defined(CONFIG_NAND_K9F2G08U0C)
#include <flash/slcnand/k9f2g08u0c.h>
#elif defined(CONFIG_NAND_ASU1GA30HT)
#include <flash/slcnand/asu1ga30ht.h>
#elif defined(CONFIG_NAND_K9F4G08U0A)
#include <flash/slcnand/k9f4g08u0a.h>
#elif defined(CONFIG_NAND_F59L1G81A)
#include <flash/slcnand/f59l1g81a.h>
#elif defined(CONFIG_NAND_F59L1G81LA)
#include <flash/slcnand/f59l1g81la.h>
#elif defined(CONFIG_NAND_F59L1G81MA)
#include <flash/slcnand/f59l1g81ma.h>
#elif defined(CONFIG_NAND_F59L2G81A)
#include <flash/slcnand/f59l2g81a.h>
#elif defined(CONFIG_NAND_F59L4G81A)
#include <flash/slcnand/f59l4g81a.h>
#elif defined(CONFIG_NAND_S34ML01G1)
#include <flash/slcnand/s34ml01g1.h>
#elif defined(CONFIG_NAND_S34ML02G1)
#include <flash/slcnand/s34ml02g1.h>
#elif defined(CONFIG_NAND_S34ML04G1)
#include <flash/slcnand/s34ml04g1.h>
#elif defined(CONFIG_NAND_S34ML01G2)
#include <flash/slcnand/s34ml01g2.h>
#elif defined(CONFIG_NAND_S34ML02G2)
#include <flash/slcnand/s34ml02g2.h>
#elif defined(CONFIG_NAND_S34ML04G2)
#include <flash/slcnand/s34ml04g2.h>
#elif defined(CONFIG_NAND_MX30LF1G08AA)
#include <flash/slcnand/mx30lf1g08aa.h>
#elif defined(CONFIG_NAND_MX30LF1GE8AB)
#include <flash/slcnand/mx30lf1ge8ab.h>
#elif defined(CONFIG_NAND_MX30LF2GE8AB)
#include <flash/slcnand/mx30lf2ge8ab.h>
#elif defined(CONFIG_NAND_W29N01GVSCAA)
#include <flash/slcnand/w29n01gvscaa.h>
#elif defined(CONFIG_NAND_W29N02GVSIAA)
#include <flash/slcnand/w29n02gvsiaa.h>
#endif

#if !defined(CONFIG_AMBOOT_ENABLE_NAND)
/******************************************/
/* Physical NAND flash device information */
/******************************************/

#define NAND_CONTROL		0
#define NAND_MANID              0
#define NAND_DEVID              0
#define NAND_ID3                0
#define NAND_ID4                0
#define NAND_ID			0

#define NAND_MAIN_SIZE		0
#define NAND_SPARE_SIZE		0
#define NAND_PAGE_SIZE		0
#define NAND_PAGES_PER_BLOCK	0
#define NAND_BLOCKS_PER_PLANE	0
#define NAND_BLOCKS_PER_ZONE	0
#define NAND_BLOCKS_PER_BANK	0
#define NAND_PLANES_PER_BANK	0
#define NAND_BANKS_PER_DEVICE	0
#define NAND_TOTAL_BLOCKS	0
#define NAND_TOTAL_ZONES	0
#define NAND_TOTAL_PLANES	0
#define NAND_BLOCK_ADDR_BIT	0
#define NAND_PLANE_ADDR_BIT	0
#define NAND_PLANE_MASK		0
#define NAND_PLANE_ADDR_MASK	0
#define NAND_PLANE_MAP		0
#define NAND_COLUMN_CYCLES	0
#define NAND_PAGE_CYCLES	0
#define NAND_ID_CYCLES		0
#define NAND_CHIP_WIDTH		0
#define NAND_CHIP_SIZE_MB	0
#define NAND_BUS_WIDTH		0
#define NAND_DEVICES		0
#define NAND_NAME		"NAND_NONE"

#define NAND_TOTAL_BANKS	0
#define NAND_BB_MARKER_OFFSET	0
#define NAND_RSV_BLKS_PER_ZONE  0

#define NAND_TCLS		0
#define NAND_TALS		0
#define NAND_TCS		0
#define NAND_TDS		0
#define NAND_TCLH		0
#define NAND_TALH		0
#define NAND_TCH		0
#define NAND_TDH		0
#define NAND_TWP		0
#define NAND_TWH		0
#define NAND_TWB		0
#define NAND_TRR		0
#define NAND_TRP		0
#define NAND_TREH		0
#define NAND_TRB		0
#define NAND_TCEH		0
#define NAND_TRDELAY		0
#define NAND_TCLR		0
#define NAND_TWHR		0
#define NAND_TIR		0
#define NAND_TWW		0
#define NAND_TRHZ		0
#define NAND_TAR		0

#define NAND_RSV_BLKS_PER_ZONE	0
#define NAND_CAPABILITY		0
/*****************************************/
/* Logical NAND flash device information */
/*****************************************/

#undef NAND_NUM_INTLVE
#define NAND_NUM_INTLVE		0

#define NAND_LMAIN_SIZE		0
#define NAND_LSPARE_SIZE	0
#define NAND_LPAGE_SIZE		0
#define NAND_LPAGES_PER_BLOCK	0
#define NAND_LBLOCKS_PER_PLANE	0
#define NAND_LBLOCKS_PER_ZONE	0
#define NAND_LBLOCKS_PER_BANK	0
#define NAND_LPLANES_PER_BANK	0
#define NAND_LBANKS_PER_DEVICE	0
#define NAND_LTOTAL_BLOCKS	0
#define NAND_LTOTAL_ZONES	0
#define NAND_LTOTAL_PLANES	0
#define NAND_LTOTAL_BANKS	0
#define NAND_LDEVICES		0

#else /* !CONFIG_AMBOOT_ENABLE_NAND */
#define NAND_ID		((unsigned long)			   \
			 (NAND_MANID << 24) | (NAND_DEVID << 16) | \
			 (NAND_ID3 << 8) | NAND_ID4)

/*****************************************/
/* Logical NAND flash device information */
/*****************************************/
#ifndef NAND_CAPABILITY
#define NAND_CAPABILITY		0
#endif

#ifndef NAND_NUM_INTLVE
#undef NAND_NUM_INTLVE
#define NAND_NUM_INTLVE		1
#endif

#if (NAND_NUM_INTLVE == 1) || (NAND_NUM_INTLVE == 2) || (NAND_NUM_INTLVE == 4)
/* Logical device info(logical bank info) */
/* Page characteristic is "NAND_NUM_INTLVE" times of original */
#define NAND_LMAIN_SIZE		(NAND_MAIN_SIZE * NAND_NUM_INTLVE)
#define NAND_LSPARE_SIZE	(NAND_SPARE_SIZE * NAND_NUM_INTLVE)
#define NAND_LPAGE_SIZE		(NAND_PAGE_SIZE * NAND_NUM_INTLVE)

#define NAND_LPAGES_PER_BLOCK	NAND_PAGES_PER_BLOCK
#define NAND_LBLOCKS_PER_PLANE	NAND_BLOCKS_PER_PLANE
#define NAND_LBLOCKS_PER_ZONE	NAND_BLOCKS_PER_ZONE
#define NAND_LBLOCKS_PER_BANK	NAND_BLOCKS_PER_BANK
#define NAND_LPLANES_PER_BANK	NAND_PLANES_PER_BANK

#if (NAND_NUM_INTLVE == 1)
#define NAND_LBANKS_PER_DEVICE	NAND_BANKS_PER_DEVICE
#else
#define NAND_LBANKS_PER_DEVICE	1
#endif

#define NAND_LTOTAL_BLOCKS	(NAND_LBLOCKS_PER_BANK * NAND_LBANKS_PER_DEVICE)
#define NAND_LTOTAL_ZONES	(NAND_LTOTAL_BLOCKS / NAND_LBLOCKS_PER_ZONE)
#define NAND_LTOTAL_PLANES	(NAND_LTOTAL_BLOCKS / NAND_LBLOCKS_PER_PLANE)

/* Information for all devices considerd*/
#define NAND_LTOTAL_BANKS	(NAND_TOTAL_BANKS / NAND_NUM_INTLVE)

#if (NAND_NUM_INTLVE == 1)
#define NAND_LDEVICES		NAND_DEVICES
#else
#define NAND_LDEVICES		NAND_LTOTAL_BANKS
#endif

#if defined(CONFIG_AMBOOT_ENABLE_NAND)
#if (NAND_LTOTAL_BANKS == 0) || (NAND_TOTAL_BANKS % NAND_NUM_INTLVE)
#error Unsupport nand logical device information
#endif
#endif

#else
#error Unsupport nand flash interleave banks
#endif

#endif /* CONFIG_AMBOOT_ENABLE_NAND */

#endif
