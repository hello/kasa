/**
 * bld/idc_bld.c
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
#include <ambhw/idc.h>
#include <ambhw/gpio.h>

/*===========================================================================*/
static u32 idc_bld_get_addr(u8 idc_id, u32 reg_offset)
{
	u32 reg_adds = 0xFFFFFFFF;

	K_ASSERT(idc_id < IDC_INSTANCES);
	switch (idc_id) {
	case IDC_MASTER1:
		reg_adds = IDC_REG(reg_offset);
		break;
#if (IDC_INSTANCES >= 2)
	case IDC_MASTER2:
		reg_adds = IDC2_REG(reg_offset);
		break;
#endif
#if (IDC_INSTANCES >= 3)
	case IDC_MASTER3:
		reg_adds = IDC3_REG(reg_offset);
		break;
#endif
	}

	return reg_adds;
}

static void idc_bld_set_pin_mux(u8 idc_id)
{
#if (CHIP_REV == A5S)
	switch (idc_id) {
	case IDC_MASTER1:
		/* GPIO1.AFSEL[4] == 0 (IDC1)*/
		writel(GPIO1_REG(GPIO_AFSEL_OFFSET),
			(readl(GPIO1_REG(GPIO_AFSEL_OFFSET)) &
			(~(0x1 << 4))));
		break;
	case IDC_MASTER3:
		/* GPIO1.AFSEL[4] == 1 (IDC3)*/
		writel(GPIO1_REG(GPIO_AFSEL_OFFSET),
			(readl(GPIO1_REG(GPIO_AFSEL_OFFSET)) |
			(0x1 << 4)));
		break;
	}
#elif (CHIP_REV == S2) || (CHIP_REV == S2E)
	switch (idc_id) {
	case IDC_MASTER1:
		/* GPIO0.AFSEL[17] == 0 (IDC1)*/
		writel(GPIO0_REG(GPIO_AFSEL_OFFSET),
			(readl(GPIO0_REG(GPIO_AFSEL_OFFSET)) &
			(~(0x1 << 17))));
		break;
	case IDC_MASTER3:
		/* GPIO0.AFSEL[17] == 1 (IDC3)*/
		writel(GPIO0_REG(GPIO_AFSEL_OFFSET),
			(readl(GPIO0_REG(GPIO_AFSEL_OFFSET)) |
			(0x1 << 17)));
		break;
	}
#endif
}

/*===========================================================================*/
static u32 idc_bld_readl(u8 idc_id, u32 reg_offset)
{
	u32 reg_adds;
	u32 reg_val = 0;

	reg_adds = idc_bld_get_addr(idc_id, reg_offset);
	if (reg_adds != 0xFFFFFFFF) {
		reg_val = readl(reg_adds);
	}

	return reg_val;
}

static inline void idc_bld_writel(u8 idc_id, u32 reg_offset, u32 reg_val)
{
	u32 reg_add;

	reg_add = idc_bld_get_addr(idc_id, reg_offset);
	if (reg_add != 0xFFFFFFFF) {
		reg_val = writel(reg_add, reg_val);
	}
}

/*===========================================================================*/
static void idc_bld_dclk_config(u8 idc_id, u32 freq_hz)
{
	unsigned int apb_clk;
	u32 idc_prescale;

	if (!freq_hz) {
		return;
	}
	apb_clk = get_apb_bus_freq_hz();
	idc_prescale = (apb_clk / freq_hz) >> 2;
	idc_prescale--;

	idc_bld_writel(idc_id, IDC_PSLL_OFFSET, (idc_prescale & 0xFF));
	idc_bld_writel(idc_id, IDC_PSLH_OFFSET, ((idc_prescale & 0xFF00) >> 8));
}

/*===========================================================================*/
void idc_bld_init(u8 idc_id, u32 freq_hz)
{
	idc_bld_writel(idc_id, IDC_ENR_OFFSET, 0);
	idc_bld_dclk_config(idc_id, freq_hz);
	idc_bld_writel(idc_id, IDC_ENR_OFFSET, 1);

	idc_bld_set_pin_mux(idc_id);
}

/*===========================================================================*/
static void idc_bld_wait_interrupt(u8 idc_id, u32 ctrl_start)
{
	u32 reg_ctrl;
	int wait_loop = IDC_INT_MAX_WAIT_LOOP;

	idc_bld_writel(idc_id, IDC_CTRL_OFFSET, ctrl_start);
	do {
		reg_ctrl = idc_bld_readl(idc_id, IDC_CTRL_OFFSET);
		wait_loop--;
	} while (((reg_ctrl & 0x00000002) == 0) && (wait_loop > 0));
}

/*===========================================================================*/
static void idc_bld_set_8bit_addr(u8 idc_id, u8 addr, u8 rw)
{
	idc_bld_writel(idc_id, IDC_DATA_OFFSET, (addr | rw));
	idc_bld_wait_interrupt(idc_id, 0x04);
}

static void idc_bld_stop(u8 idc_id)
{
	idc_bld_writel(idc_id, IDC_CTRL_OFFSET, 0x08);
}

/*===========================================================================*/
static u8 idc_bld_readb(u8 idc_id, u32 nack)
{
	idc_bld_wait_interrupt(idc_id, nack);
	return (u8)idc_bld_readl(idc_id, IDC_DATA_OFFSET);
}

static void idc_bld_writeb(u8 idc_id, u8 data)
{
	idc_bld_writel(idc_id, IDC_DATA_OFFSET, data);
	idc_bld_wait_interrupt(idc_id, 0x00);
}

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_ISL12022M)
#define ISL12022M_I2C_ADDR			(0xDE)

u8 idc_bld_isl12022m_read(u8 idc_id, u8 sub_adds)
{
	u8 isl12022m_adds = ISL12022M_I2C_ADDR;
	u8 reg_val = 0;

	idc_bld_set_8bit_addr(idc_id, isl12022m_adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_set_8bit_addr(idc_id, isl12022m_adds, 1);
	reg_val = idc_bld_readb(idc_id, 0x0);
	idc_bld_stop(idc_id);

	return reg_val;
}

void idc_bld_isl12022m_write(u8 idc_id, u8 sub_adds, u8 reg_val)
{
	u8 isl12022m_adds = ISL12022M_I2C_ADDR;

	idc_bld_set_8bit_addr(idc_id, isl12022m_adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_writeb(idc_id, reg_val);
	idc_bld_stop(idc_id);
}
#endif

/*===========================================================================*/
#if defined(CONFIG_AMBOOT_ENABLE_PCA953X)
u8 idc_bld_pca953x_read(u8 idc_id, u8 adds, u8 sub_adds)
{
	u8 reg_val = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_set_8bit_addr(idc_id, adds, 1);
	reg_val = idc_bld_readb(idc_id, 0x0);
	idc_bld_stop(idc_id);

	return reg_val;
}

void idc_bld_pca953x_write(u8 idc_id, u8 adds, u8 sub_adds, u8 reg_val)
{
	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_writeb(idc_id, reg_val);
	idc_bld_stop(idc_id);
}
#endif

int idc_bld_send_buf_without_ack(unsigned char idc_id, unsigned char adds, unsigned char* buf, int count)
{
	int i = 0;
	idc_bld_writel(idc_id, IDC_DATA_OFFSET, adds);
	idc_bld_writel(idc_id, IDC_CTRL_OFFSET, 0x04);

	for (i = 0; i < count; i++) {
		idc_bld_writel(idc_id, IDC_DATA_OFFSET, buf[i]);
	}
	idc_bld_stop(idc_id);

	return count;
}

int idc_bld_send_buf(unsigned char idc_id, unsigned char adds, unsigned char* send_buf, int send_len)
{
	int i = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	for (i = 0; i < send_len; i++) {
		idc_bld_writeb(idc_id, send_buf[i]);
	}
	idc_bld_stop(idc_id);

	return send_len;
}

int idc_bld_recv_buf(unsigned char idc_id, unsigned char adds, unsigned char* recv_buf, int recv_len)
{
	int i = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 1);

	for (i = 0; i < recv_len; i++) {
		recv_buf[i] = idc_bld_readb(idc_id, 0x0);
	}

	idc_bld_stop(idc_id);

	return recv_len;
}

int idc_bld_read_buf(unsigned char idc_id, unsigned char adds, unsigned char* read_buf, int read_len)
{
	int i = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, 0x03);
	idc_bld_set_8bit_addr(idc_id, adds, 1);

	for (i = 0; i < read_len; i++) {
		read_buf[i]= idc_bld_readb(idc_id, 0x0);
	}

	idc_bld_stop(idc_id);

	return read_len;
}

/*===========================================================================*/
u16 idc_bld_read_16_16(u8 idc_id, u8 adds, u16 sub_adds)
{
	u16 reg_val = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, (sub_adds >> 8) & 0xff);
	idc_bld_writeb(idc_id, sub_adds & 0xff);
	idc_bld_set_8bit_addr(idc_id, adds, 1);
	reg_val = idc_bld_readb(idc_id, 0x1);
	reg_val = reg_val << 8;
	reg_val |= idc_bld_readb(idc_id, 0x1);
	idc_bld_stop(idc_id);

	return reg_val;
}

void idc_bld_write_16_16(u8 idc_id, u8 adds, u16 sub_adds, u16 reg_val)
{
	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, (sub_adds >> 8) & 0xff);
	idc_bld_writeb(idc_id, sub_adds & 0xff);
	idc_bld_writeb(idc_id, (reg_val >> 8) & 0xff);
	idc_bld_writeb(idc_id, reg_val & 0xff);
	idc_bld_stop(idc_id);
}

u8 idc_bld_read_16_8(u8 idc_id, u8 adds, u16 sub_adds)
{
	u8 reg_val = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, (sub_adds >> 8) & 0xff);
	idc_bld_writeb(idc_id, sub_adds & 0xff);
	idc_bld_set_8bit_addr(idc_id, adds, 1);
	reg_val = idc_bld_readb(idc_id, 0x1);
	idc_bld_stop(idc_id);

	return reg_val;
}

void idc_bld_write_16_8(u8 idc_id, u8 adds, u16 sub_adds, u8 reg_val)
{
	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, (sub_adds >> 8) & 0xff);
	idc_bld_writeb(idc_id, sub_adds & 0xff);
	idc_bld_writeb(idc_id, reg_val);
	idc_bld_stop(idc_id);
}

u8 idc_bld_read_8_8(u8 idc_id, u8 adds, u8 sub_adds)
{
	u8 reg_val = 0;

	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_set_8bit_addr(idc_id, adds, 1);
	reg_val = idc_bld_readb(idc_id, 0x1);
	idc_bld_stop(idc_id);

	return reg_val;
}

void idc_bld_write_8_8(u8 idc_id, u8 adds, u8 sub_adds, u8 reg_val)
{
	idc_bld_set_8bit_addr(idc_id, adds, 0);
	idc_bld_writeb(idc_id, sub_adds);
	idc_bld_writeb(idc_id, reg_val);
	idc_bld_stop(idc_id);
}


