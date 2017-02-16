/**
 * boards/hawthorn/bsp/bsp.c
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

#include <bldfunc.h>
#include <bapi.h>
#include <eth/network.h>
#include <ambhw/gpio.h>
#include <ambhw/idc.h>
#include <ambhw/sd.h>
#include <sdmmc.h>
#include <dsp/dsp.h>

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

#if defined(CONFIG_AMBOOT_ENABLE_IDC)
	idc_bld_init(IDC_MASTER3, 100000);
#endif

	return retval;
}

/* ==========================================================================*/

#if defined(CONFIG_HAWTHORN_IAV_OV4689)
#include "iav/sensor_ov4689.c"
#endif
#if defined(CONFIG_HAWTHORN_IAV_CVBS)
#include "iav/iav_cvbs.c"
#elif defined(CONFIG_HAWTHORN_IAV_TD043)
#include "iav/iav_td043.c"
#endif
#if defined(CONFIG_HAWTHORN_IAV_FASTOSD)
#include "iav/iav_fastosd.c"
#endif

int amboot_bsp_entry(flpart_table_t *pptb)
{
	int retval = 0;
	flpart_table_t ptb;

#if defined(CONFIG_HAWTHORN_DSP_BOOT)
	u32 *p_iav_status = NULL;
	u32 iav_status = IAV_STATE_IDLE;
	/* Change the value after call dsp_boot() */
	p_iav_status = (u32*)(DSP_STATUS_STORE_START);
#endif

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	bld_bapi_set_fb_info(0, 0, 1920, 1080, 1920, 1080, 1, 8);
#endif

	/* setup sensor */
#if defined(CONFIG_HAWTHORN_IAV_OV4689)
	ov4689_init();
	vin_phy_init(SENSOR_MIPI);
#endif

	/* setup vout */
#if defined(CONFIG_HAWTHORN_IAV_CVBS)
	iav_boot_cvbs();
#elif defined(CONFIG_HAWTHORN_IAV_TD043)
	iav_boot_td043();
#endif

	/* setup encode */
#if defined(CONFIG_HAWTHORN_IAV_ENCODING)
	iav_boot_preview();
	iav_status = IAV_STATE_PREVIEW;
	iav_boot_encode();
	iav_status = IAV_STATE_ENCODING;
#if defined(CONFIG_HAWTHORN_IAV_FASTOSD)
	iav_fastosd(pptb);
#endif
#endif

#if defined(CONFIG_HAWTHORN_DSP_BOOT)
	dsp_boot();
	*p_iav_status = iav_status;
#endif

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

/* =========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_SD)
int amboot_bsp_sd_slot_init(int slot, int volt)
{
	if(volt == SDMMC_VOLTAGE_3V3) {
		int gpio_pwr = (slot == 0) ? 0 : 1;

		if (slot == 0) {
			gpio_config_sw_out(GPIO(0));
			gpio_set(GPIO(0));
		}

		/* power off, then power on */
		pca9539_set_gpio(gpio_pwr, 0);
		rct_timer_dly_ms(10);
		pca9539_set_gpio(gpio_pwr, 1);

	} else {
		if(slot == 0) {
			gpio_config_sw_out(GPIO(0));
			gpio_clr(GPIO(0));
		} else {
			putstr("Can not switch voltage to 1.8v\r\n");
			return -1;
		}
	}

	return 0;
}

int amboot_bsp_sd_phy_init(int slot, int mode)
{
	u32 phy0_val, latency_val;

	/* only slot 0 support sd PHY tunning */
	if (slot > 0)
		return 0;

	if (mode == SDMMC_MODE_AUTO) { /* "auto" also means in init stage */
		phy0_val = 0x04070000;
		latency_val = 0x00000000;
	} else if (mode == SDMMC_MODE_DDR50) {
		phy0_val = 0x00000003;
		latency_val = 0x00001111;
	} else {
		phy0_val = 0x00000001;
		latency_val = 0x00001111;
	}

	writel(SD_PHY_CTRL_0_REG, phy0_val | 0x02000000);
	rct_timer_dly_ms(1);
	writel(SD_PHY_CTRL_0_REG, phy0_val);
	writel(SD_BASE(slot) + SD_LAT_CTRL_OFFSET, latency_val);

	return 0;
}
#endif
