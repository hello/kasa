/**
 * boards/s2lmironman/bsp/bsp.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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


#include <bldfunc.h>
#include <ambhw/drctl.h>
#include <ambhw/idc.h>
#include <eth/network.h>
#include <ambhw/gpio.h>
#include <dsp/dsp.h>
#include "adc.h"

#if defined(CONFIG_S2LMIRONMAN_IAV_OV4689)
#include "iav/sensor_ov4689.c"
#endif

#if defined(CONFIG_S2LMIRONMAN_IAV_CVBS)
#include "iav/iav_cvbs.c"
#elif defined(CONFIG_S2LMIRONMAN_IAV_TD043)
#include "iav/iav_td043.c"
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
#include "iav/smart3a.c"
#endif

#if defined(CONFIG_S2LMIRONMAN_AK4954)
#include "iav/codec_ak4954.c"
#endif

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

#if defined(CONFIG_S2LMIRONMAN_AUDIO_BOOT)
	ak4954_init();
#endif

	return retval;
}

/* ==========================================================================*/
void amboot_fast_boot(flpart_table_t *pptb)
{
#if defined(CONFIG_S2LMIRONMAN_DSP_BOOT)
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

#if defined(CONFIG_S2LMIRONMAN_TURN_ON_LED_IN_BOOTLOADER)
	//fix me, turn on led, only for ces demo
	gpio_config_sw_out(113);
	gpio_set(113);
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	smart3a_ret = find_idsp_cfg(pptb, &hdr, &idsp_cfg_index);
#endif

#if defined(CONFIG_S2LMIRONMAN_IAV_OV4689)
	gpio_config_sw_out(100);//power
	gpio_config_sw_out(98);//reset
	gpio_set(100);//power on
	gpio_clr(98);
	rct_timer_dly_ms(10);//wait 10ms
#endif

	/* setup for IAV */
#if defined(CONFIG_S2LMIRONMAN_IAV_OV4689)
	gpio_set(98);//reset done
	ov4689_init();
#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	if (0 == smart3a_ret) {
		load_shutter_gain_param(hdr, idsp_cfg_index);
	}
#endif
	vin_phy_init(SENSOR_MIPI);
#endif

#if defined(CONFIG_S2LMIRONMAN_IAV_CVBS)
	iav_boot_cvbs();
#elif defined(CONFIG_S2LMIRONMAN_IAV_TD043)
	iav_boot_td043();
#endif

#if defined(BUILD_AMBARELLA_APP_FASTBOOT_SMART3A)
	if (0 == smart3a_ret) {
		load_3A_param(hdr, idsp_cfg_index);
	}
#endif

#if defined(CONFIG_S2LMIRONMAN_DSP_BOOT)
	iav_boot_preview();
	iav_status = IAV_STATE_PREVIEW;
	iav_boot_encode();
	iav_status = IAV_STATE_ENCODING;
	/* now boot up dsp */
	dsp_boot();
	*p_iav_status = iav_status;
#endif
}

int amboot_bsp_entry(flpart_table_t *pptb)
{
	int retval = 0;
	flpart_table_t ptb;

	amboot_fast_boot(pptb);

#if defined(CONFIG_S2LMIRONMAN_AUDIO_BOOT)
	audio_start();
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

