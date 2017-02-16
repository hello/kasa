/**
 * boards/s2lmkiwi/bsp/bsp.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <bapi.h>
#include <ambhw/drctl.h>
#include <ambhw/idc.h>
#include <eth/network.h>
#include <dsp/dsp.h>
#include <sdmmc.h>

#if defined(CONFIG_S2LMKIWI_SENSOR_OV4689)
#include "iav/sensor_ov4689.c"
#elif defined(CONFIG_S2LMKIWI_SENSOR_OV9710)
#include "iav/sensor_ov9710.c"
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230)
#include "iav/sensor_ar0230.c"
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230_HDR_2X)
#include "iav/sensor_ar0230_hdr_2x.c"
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230_WDR_IN)
#include "iav/sensor_ar0230_wdr_in.c"
#endif

#if defined(CONFIG_S2LMKIWI_DSP_VOUT_CVBS)
#include "iav/iav_cvbs.c"
#elif defined(CONFIG_S2LMKIWI_DSP_VOUT_TD043)
#include "iav/iav_td043.c"
#endif

#if defined(CONFIG_S2LMKIWI_DSP_FASTOSD)
#include "iav/iav_fastosd.c"
#endif

#if defined(CONFIG_S2LMKIWI_AUDIO_AK4954)
#include "iav/codec_ak4954.c"
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
#include "iav/smart3a.c"
#endif

/* ==========================================================================*/
static void pca9539_set_gpio(u32 id, u32 set)
{
#if defined(CONFIG_AMBOOT_ENABLE_PCA953X)
	u8 pca9539_adds = 0xE8;
	u8 pca9539_cfg_adds;
	u8 pca9539_out_adds;
	u8 pca9539_shift;
	u8 reg_val;

	if (id < 8) {
		pca9539_cfg_adds = 0x06;
		pca9539_out_adds = 0x02;
		pca9539_shift = id;
	} else if (id < 15) {
		pca9539_cfg_adds = 0x07;
		pca9539_out_adds = 0x03;
		pca9539_shift = (id - 8);
	} else {
		return;
	}

	reg_val = idc_bld_pca953x_read(IDC_MASTER3,
		pca9539_adds, pca9539_cfg_adds);
	reg_val &= ~(0x1 << pca9539_shift);
	idc_bld_pca953x_write(IDC_MASTER3,
		pca9539_adds, pca9539_cfg_adds, reg_val);

	reg_val = idc_bld_pca953x_read(IDC_MASTER3,
		pca9539_adds, pca9539_out_adds);
	if (set) {
		reg_val |= (0x1 << pca9539_shift);
	} else {
		reg_val &= ~(0x1 << pca9539_shift);
	}
	idc_bld_pca953x_write(IDC_MASTER3,
		pca9539_adds, pca9539_out_adds, reg_val);
#endif
}

/* ==========================================================================*/
int amboot_bsp_hw_init(void)
{
	int retval = 0;
	u32 tmp_reg;

	//Invert Ethernet Output Clock
	tmp_reg = readl(0xE001B00C);
	tmp_reg |= 0x80000000;
	writel(0xE001B00C, tmp_reg);

	// Change VD0 Driving Strength
	writel(0xec170324, 0x1fffffff);
	writel(0xec17032c, 0xfffe2000);
	writel(0xec170330, 0xffffdfff);

#if defined(CONFIG_AMBOOT_ENABLE_IDC)
	idc_bld_init(IDC_MASTER3, 100000);
#endif

#if defined(CONFIG_S2LMKIWI_AUDIO_AK4954)
	ak4954_init();
#endif

	return retval;
}

/* ==========================================================================*/
void amboot_fast_boot(flpart_table_t *pptb)
{
	/* store dsp status in amboot, restore it after enter Linux IAV */
#if defined(CONFIG_S2LMKIWI_DSP_BOOT)
	u32 *p_iav_status = NULL;
	u32 iav_status = IAV_STATE_IDLE;
	/* Change the value after call dsp_boot() */
	p_iav_status = (u32*)(DSP_STATUS_STORE_START);
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	int smart3a_ret = -1;
	int idsp_cfg_index = -1;
	struct adcfw_header *hdr = NULL;
#endif
#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	smart3a_ret = find_idsp_cfg(pptb, &hdr, &idsp_cfg_index);
#endif

	/* setup sensor */
#if defined(CONFIG_S2LMKIWI_SENSOR_OV4689)
	ov4689_init();
	vin_phy_init(SENSOR_MIPI);
#elif defined(CONFIG_S2LMKIWI_SENSOR_OV9710)
	ov9710_init();
	vin_phy_init(SENSOR_PARALLEL_LVCMOS);
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230)
	ar0230_init();
	vin_phy_init(SENSOR_SERIAL_LVDS);
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230_HDR_2X)
	ar0230_init_hdr_2x();
	vin_phy_init(SENSOR_SERIAL_LVDS);
#elif defined(CONFIG_S2LMKIWI_SENSOR_AR0230_WDR_IN)
	ar0230_init_wdr_in();
	vin_phy_init(SENSOR_SERIAL_LVDS);
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	if (0 == smart3a_ret) {
		load_shutter_gain_param(hdr, idsp_cfg_index);
	}
#endif

	/* setup vout */
#if defined(CONFIG_S2LMKIWI_DSP_VOUT_CVBS)
	iav_boot_cvbs();
#elif defined(CONFIG_S2LMKIWI_DSP_VOUT_TD043)
	iav_boot_td043();
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	if (0 == smart3a_ret) {
		load_3A_param(hdr, idsp_cfg_index);
	}
#endif

	/* setup preview */
#if defined(CONFIG_S2LMKIWI_DSP_PREVIEW)
	iav_boot_preview();
	iav_status = IAV_STATE_PREVIEW;
#endif

	/* setup encode */
#if defined(CONFIG_S2LMKIWI_DSP_ENCODING)
	iav_boot_encode();
	iav_status = IAV_STATE_ENCODING;
#endif

	/* show fastosd */
#if defined(CONFIG_S2LMKIWI_DSP_FASTOSD)
	iav_fastosd(pptb);
#endif

	/* boot up dsp */
#if defined(CONFIG_S2LMKIWI_DSP_BOOT)
	dsp_boot();
	*p_iav_status = iav_status;
#endif

	/* boot up audio */
#if defined(CONFIG_S2LMKIWI_AUDIO_AK4954)
	audio_start();
#endif

}

int amboot_bsp_entry(flpart_table_t *pptb)
{
	int retval = 0;
	flpart_table_t ptb;

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	bld_bapi_set_fb_info(0, 0, 320, 240, 320, 480, 16, 32);
#endif

	amboot_fast_boot(pptb);
	/* Read the partition table */
	retval = flprog_get_part_table(&ptb);
	if (retval < 0) {
		return retval;
	}

	/* BIOS boot */
	if (ptb.dev.rsv[0] > 0) {
		putstr("Find BIOS boot flag\r\n");
		retval = ptb.dev.rsv[0];
	}

	return retval;
}

/* ==========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_ETH)
#define MII_KSZ80X1R_CTRL			0x1F
#define KSZ80X1R_CTRL_INT_ACTIVE_HIGH		(1 << 9)
#define KSZ80X1R_RMII_50MHZ_CLK			(1 << 7)

extern u16 eth_mii_read(struct bld_eth_dev_s *dev, u8 addr, u8 reg);
extern void eth_mii_write(struct bld_eth_dev_s *dev, u8 addr, u8 reg, u16 data);
extern u8 eth_scan_phy_addr(struct bld_eth_dev_s *dev);

int amboot_bsp_eth_init_post(void *dev)
{
	u8 phy_addr;
	u16 phy_reg;
	u32 phy_id;

	phy_addr = eth_scan_phy_addr(dev);
	phy_reg = eth_mii_read(dev, phy_addr, 0x02);	//PHYSID1
	phy_id = (phy_reg & 0xffff) << 16;
	phy_reg = eth_mii_read(dev, phy_addr, 0x03);	//PHYSID2
	phy_id |= (phy_reg & 0xffff);

	if (phy_id == 0x00221560) {
		phy_reg = eth_mii_read(dev, phy_addr, MII_KSZ80X1R_CTRL);
		phy_reg |= KSZ80X1R_RMII_50MHZ_CLK;
		eth_mii_write(dev, phy_addr, MII_KSZ80X1R_CTRL, phy_reg);
	}

	return 0;
}
#endif

#if defined(CONFIG_AMBOOT_ENABLE_SD)
int amboot_bsp_sd_slot_init(int slot, int volt)
{
	if (slot > 0 || volt != SDMMC_VOLTAGE_3V3) {
		putstr("Not support slot or volt\r\n");
		return -1;
	}

	/* power off, then power on */
	pca9539_set_gpio(14, 0);
	rct_timer_dly_ms(10);
	pca9539_set_gpio(14, 1);

	sdmmc.no_1v8 = 1;

	return 0;
}
#endif
