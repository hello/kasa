/**
 * system/src/bld/spi_nor.c
 *
 * Flash controller functions with spi nor  chips.
 *
 * History:
 *    2013/10/12 - [cddiao] creat
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
#include <fio/firmfl.h>
#include <flash/spinor/spinor_flash.h>
#include <ambhw/dma.h>
#include <ambhw/cache.h>
#include <ambhw/rct.h>
#include <ambhw/spinor.h>

struct spinor_ctrl {
	u32 len;
	u8 *buf;
	u8 lane;
	u8 is_dtr : 1;
	u8 is_read : 1; /* following are only avaiable for data */
	u8 is_io : 1;
	u8 is_dma : 1;
};

int (*spinor_read_data)(u32 address, void *buf, int len);
static u32 spi_nor_addr_mode = 4;
static u32 spi_nor_id = 0;
#define JEDEC_MFR(_jedec_id)	((_jedec_id) & 0x000000FF)

static void spinor_setup_dma_devmem(struct spinor_ctrl *data)
{
	u32 reg;

	clean_flush_d_cache(data->buf, data->len);

	reg = DMA_CHAN_STA_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, 0x0);

	reg = DMA_CHAN_SRC_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, SPINOR_RXDATA_REG);

	reg = DMA_CHAN_DST_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, (u32)data->buf);

	reg = DMA_CHAN_CTR_REG(NOR_SPI_RX_DMA_CHAN);
	writel(reg, DMA_CHANX_CTR_EN | DMA_CHANX_CTR_WM |
		    DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_32B |
		    DMA_CHANX_CTR_TS_4B | (data->len & (~0x1f)));

	writel(SPINOR_DMACTRL_REG, SPINOR_DMACTRL_RXEN);
}

static void spinor_setup_dma_memdev(struct spinor_ctrl *data)
{
	u32 reg;

	clean_flush_d_cache(data->buf, data->len);

	reg = DMA_CHAN_STA_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, 0x0);

	reg = DMA_CHAN_SRC_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, (u32)data->buf);

	reg = DMA_CHAN_DST_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, SPINOR_TXDATA_REG);

	reg = DMA_CHAN_CTR_REG(NOR_SPI_TX_DMA_CHAN);
	writel(reg, DMA_CHANX_CTR_EN | DMA_CHANX_CTR_RM |
		    DMA_CHANX_CTR_NI | DMA_CHANX_CTR_BLK_32B |
		    DMA_CHANX_CTR_TS_4B | data->len);

	writel(SPINOR_DMACTRL_REG, SPINOR_DMACTRL_TXEN);
}

static int spinor_send_cmd(struct spinor_ctrl *cmd, struct spinor_ctrl *addr,
		struct spinor_ctrl *data, struct spinor_ctrl *dummy)
{
	u32 reg_length = 0, reg_ctrl = 0;
	u32 val, i;

	/* setup basic info */
	if (cmd != NULL) {
		reg_length |= SPINOR_LENGTH_CMD(cmd->len);

		reg_ctrl |= cmd->is_dtr ? SPINOR_CTRL_CMDDTR : 0;
		switch(cmd->lane) {
		case 8:
			reg_ctrl |= SPINOR_CTRL_CMD8LANE;
			break;
		case 4:
			reg_ctrl |= SPINOR_CTRL_CMD4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_CMD2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_CMD1LANE;
			break;
		}
	} else {
		return -1;
	}

	if (addr != NULL) {
		reg_length |= SPINOR_LENGTH_ADDR(addr->len);

		reg_ctrl |= addr->is_dtr ? SPINOR_CTRL_ADDRDTR : 0;
		switch(addr->lane) {
		case 4:
			reg_ctrl |= SPINOR_CTRL_ADDR4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_ADDR2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_ADDR1LANE;
			break;
		}
	}

	if (data != NULL) {
		if (data->len > SPINOR_MAX_DATA_LENGTH) {
			putstr("spinor: data length is too large.\r\n");
			return -1;
		}

		reg_length |= SPINOR_LENGTH_DATA(data->len);

		reg_ctrl |= data->is_dtr ? SPINOR_CTRL_DATADTR : 0;
		switch(data->lane) {
		case 8:
			reg_ctrl |= SPINOR_CTRL_DATA8LANE;
			break;
		case 4:
			reg_ctrl |= SPINOR_CTRL_DATA4LANE;
			break;
		case 2:
			reg_ctrl |= SPINOR_CTRL_DATA2LANE;
			break;
		case 1:
		default:
			reg_ctrl |= SPINOR_CTRL_DATA1LANE;
			break;
		}

		if (data->is_read)
			reg_ctrl |= SPINOR_CTRL_RDEN;
		else
			reg_ctrl |= SPINOR_CTRL_WREN;

		if (!data->is_io)
			reg_ctrl |= SPINOR_CTRL_RXLANE_TXRX;

		if (data->is_dma && data->is_read && data->len < 32)
			data->is_dma = 0;
	}

	if (dummy != NULL) {
		reg_length |= SPINOR_LENGTH_DUMMY(dummy->len);
		reg_ctrl |= dummy->is_dtr ? SPINOR_CTRL_DUMMYDTR : 0;
	}

	writel(SPINOR_LENGTH_REG, reg_length);
	writel(SPINOR_CTRL_REG, reg_ctrl);

	/* setup cmd id */
	val = 0;
	for (i = 0; i < cmd->len; i++)
		val |= (cmd->buf[i] << (i << 3));
	writel(SPINOR_CMD_REG, val);

	/* setup address */
	val = 0;
	for (i = 0; addr && i < addr->len; i++) {
		if (i >= 4)
			break;
		val |= (addr->buf[i] << (i << 3));
	}
	writel(SPINOR_ADDRLO_REG, val);

	val = 0;
	for (; addr && i < addr->len; i++)
		val |= (addr->buf[i] << ((i - 4) << 3));
	writel(SPINOR_ADDRHI_REG, val);

	/* setup dma if data phase is existed and dma is required.
	 * Note: for READ, dma will just transfer the length multiple of
	 * 32Bytes, the residual data in FIFO need to be read manually. */
	if (data != NULL && data->is_dma) {
		if (data->is_read)
			spinor_setup_dma_devmem(data);
		else
			spinor_setup_dma_memdev(data);
	} else if (data != NULL && !data->is_read) {
		if (data->len >= 0x7f) {
			putstr("spinor: tx length exceeds fifo size.\r\n");
			return -1;
		}
		for (i = 0; i < data->len; i++) {
			writeb(SPINOR_TXDATA_REG, data->buf[i]);
		}
	}

	/* clear all pending interrupts */
	writel(SPINOR_CLRINTR_REG, SPINOR_INTR_ALL);

	/* start tx/rx transaction */
	writel(SPINOR_START_REG, 0x1);

	/* wait for spi done */
	while((readl(SPINOR_RAWINTR_REG) & SPINOR_INTR_DATALENREACH) == 0x0);

	if (data != NULL && data->is_dma) {
		u32 chan, n;

		if (data->is_read)
			chan = NOR_SPI_RX_DMA_CHAN;
		else
			chan = NOR_SPI_TX_DMA_CHAN;
		/* wait for dma done */
		while(!(readl(DMA_REG(DMA_INT_OFFSET)) & DMA_INT_CHAN(chan)));
		writel(DMA_REG(DMA_INT_OFFSET), 0x0);	/* clear */

		if (data->is_read) {
			clean_flush_d_cache(data->buf, data->len);
			/* read the residual data in FIFO manually */
			n = data->len & (~0x1f);
			for (i = 0; i < (data->len & 0x1f); i++)
				data->buf[n + i] = readb(SPINOR_RXDATA_REG);
		}
		/* disable spi-nor dma */
		writel(SPINOR_DMACTRL_REG, 0x0);
	} else if (data != NULL && data->is_read) {
		for (i = 0; i < data->len; i++) {
			data->buf[i] = readb(SPINOR_RXDATA_REG);
		}
	}

	return 0;
}

int spinor_send_alone_cmd(u8 cmd_id)
{
	struct spinor_ctrl cmd;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, NULL, NULL, NULL);

	return rval;
}

int spinor_read_reg(u8 cmd_id, void *buf, u8 len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl data;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	data.buf = buf;
	data.len = len;
	data.lane = 2;
	data.is_read = 1;
	data.is_io = 0;
	data.is_dtr = 0;
	data.is_dma = 0;

	rval = spinor_send_cmd(&cmd, NULL, &data, NULL);

	return rval;
}

/* spinor_write_reg is also called when issuing operation cmd, e.g., SE */
int spinor_write_reg(u8 cmd_id, void *buf, u8 len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	int rval;

	cmd.buf = &cmd_id;
	cmd.len = 1;
	cmd.lane = 1;
	cmd.is_dtr = 0;

	addr.buf = buf;
	addr.len = len;
	addr.lane = 1;
	addr.is_dtr = 0;

	rval = spinor_send_cmd(&cmd, &addr, NULL, NULL);

	return rval;
}

int spinor_erase_chip(void)
{
	u8 status;

	spinor_send_alone_cmd(SPINOR_CMD_WREN);
	spinor_send_alone_cmd(SPINOR_CMD_CE);

	do {
		spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
	} while (status & 0x1);

	if (status & 0x20)
		return -1;

	return 0;
}

int spinor_erase_sector(u32 sector)
{
	u32 address;
	u8 status;

	spinor_send_alone_cmd(SPINOR_CMD_WREN);

	address = sector * SPINOR_SECTOR_SIZE;
	spinor_write_reg(SPINOR_CMD_SE, &address, spi_nor_addr_mode);

	do {
		spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
	} while (status & 0x1);

	if (status & 0x20)
		return -1;

	return 0;
}

int spinor_erase_sector_4K(u32 sector)
{
	u32 address;
	u8 status;

	spinor_send_alone_cmd(SPINOR_CMD_WREN);

	address = sector * SPINOR_SECTOR_SIZE;
	spinor_write_reg(SPINOR_CMD_SE_4K, &address, spi_nor_addr_mode);

	do {
		spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
	} while (status & 0x1);

	if (status & 0x20)
		return -1;

	return 0;
}

int spinor_read_data_diofr(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 status, cmd_id, is_dma;
	int n, rval;

	while (len > 0) {
		cmd_id = SPINOR_CMD_DIOR_4B;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		addr.buf = (u8 *)&address;
		addr.len = spi_nor_addr_mode;
		addr.lane = 2;
		addr.is_dtr = 0;

		if ((u32)buf & 0x7) {
			n = 8 - ((u32)buf & 0x7);
			is_dma = 0;
		} else {
			n = min(SPINOR_MAX_DATA_LENGTH, len);
			is_dma = 1;
		}
		data.buf = buf;
		data.len = n;
		data.lane = 2;
		data.is_dtr = 0;
		data.is_read = 1;
		data.is_io = 1;
		data.is_dma = is_dma;

		dummy.len = 4;
		dummy.is_dtr = 0;

		rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

int spinor_read_data_ddrdior(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 status, cmd_id, is_dma;
	int n, rval;

	while (len > 0) {
		cmd_id = SPINOR_CMD_DDRDIOR;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		addr.buf = (u8 *)&address;
		addr.len = spi_nor_addr_mode;
		addr.lane = 2;
		addr.is_dtr = 1;

		if ((u32)buf & 0x7) {
			n = 8 - ((u32)buf & 0x7);
			is_dma = 0;
		} else {
			n = min(SPINOR_MAX_DATA_LENGTH, len);
			is_dma = 1;
		}
		data.buf = buf;
		data.len = n;
		data.lane = 2;
		data.is_dtr = 1;
		data.is_read = 1;
		data.is_io = 1;
		data.is_dma = is_dma;

		dummy.len = 6;
		dummy.is_dtr = 0;

		rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

int spinor_read_data_dor(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 status, cmd_id, is_dma;
	int n, rval;

	while (len > 0) {
		cmd_id = SPINOR_CMD_DOR;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		addr.buf = (u8 *)&address;
		addr.len = spi_nor_addr_mode;
		addr.lane = 1;
		addr.is_dtr = 0;

		if ((u32)buf & 0x7) {
			n = 8 - ((u32)buf & 0x7);
			is_dma = 0;
		} else {
			n = min(SPINOR_MAX_DATA_LENGTH, len);
			is_dma = 1;
		}
		data.buf = buf;
		data.len = n;
		data.lane = 2;
		data.is_dtr = 0;
		data.is_read = 1;
		data.is_io = 0;
		data.is_dma = is_dma;

		dummy.len = 8;
		dummy.is_dtr = 0;

		rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

int spinor_read_data_dior(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 status, cmd_id, is_dma;
	int n, rval;
	u32 address_fix = 0;

	while (len > 0) {
		cmd_id = SPINOR_CMD_DIOR;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		switch (JEDEC_MFR(spi_nor_id)) {
		case CFI_MFR_MICRON:
			//micron DIOR mode is 3 bytes
			addr.len = 3;
			dummy.len = 8; //8 for micron n25q256a
			addr.buf = (u8 *)&address;
			break;
		case CFI_MFR_WINBOND:
		default:
			//Winbond DIOR mode is 4 bytes mode with 1-Mode-Bytes
			address_fix = address << 8;
			addr.len = 4;
			dummy.len = 0;
			addr.buf = (u8 *)&address_fix;
		}
		addr.lane = 2;
		addr.is_dtr = 0;

		if ((u32)buf & 0x7) {
			n = 8 - ((u32)buf & 0x7);
			is_dma = 0;
		} else {
			n = min(SPINOR_MAX_DATA_LENGTH, len);
			is_dma = 1;
		}
		data.buf = buf;
		data.len = n;
		data.lane = 2;
		data.is_dtr = 0;
		data.is_read = 1;
		data.is_io = 1;
		data.is_dma = is_dma;

		dummy.is_dtr = 0;

		rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

int spinor_read_data_read(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	struct spinor_ctrl dummy;
	u8 status, cmd_id, is_dma;
	int n, rval;

	while (len > 0) {
		cmd_id = SPINOR_CMD_READ;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		addr.buf = (u8 *)&address;
		addr.len = spi_nor_addr_mode;
		addr.lane = 1;
		addr.is_dtr = 0;

		if ((u32)buf & 0x7) {
			n = 8 - ((u32)buf & 0x7);
			is_dma = 0;
		} else {
			n = min(SPINOR_MAX_DATA_LENGTH, len);
			is_dma = 1;
		}
		data.buf = buf;
		data.len = n;
		data.lane = 2;
		data.is_dtr = 0;
		data.is_read = 1;
		data.is_io = 1;
		data.is_dma = is_dma;

		dummy.len = 0;
		dummy.is_dtr = 0;

		rval = spinor_send_cmd(&cmd, &addr, &data, &dummy);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

/* erase cmd must be issued before write corresponding sectors */
int spinor_write_data(u32 address, void *buf, int len)
{
	struct spinor_ctrl cmd;
	struct spinor_ctrl addr;
	struct spinor_ctrl data;
	u8 status, cmd_id, is_dma;
	int n, bound, rval;

	while (len > 0) {
		spinor_send_alone_cmd(SPINOR_CMD_WREN);

		cmd_id = SPINOR_CMD_PP;
		cmd.buf = &cmd_id;
		cmd.len = 1;
		cmd.lane = 1;
		cmd.is_dtr = 0;

		addr.buf = (u8 *)&address;
		addr.len = spi_nor_addr_mode;
		addr.lane = 1;
		addr.is_dtr = 0;

		/* Use dma transfer as default setting */
		is_dma = 1;

		bound = SPINOR_PAGE_SIZE - (address % SPINOR_PAGE_SIZE);
		n = min(len, bound);

		if (n % 4){
			if (n > 4) {
				n -= n % 4;
			} else {
				is_dma = 0;
			}
		}
		data.buf = buf;
		data.len = n;
		data.lane = 1;
		data.is_dtr = 0;
		data.is_read = 0;
		data.is_io = 0;
		data.is_dma = is_dma;

		rval = spinor_send_cmd(&cmd, &addr, &data, NULL);
		if (rval < 0)
			return rval;

		do {
			spinor_read_reg(SPINOR_CMD_RDSR, &status, sizeof(status));
		} while (status & 0x1);

		if (status & 0x40)
			return -1;

		address += n;
		len -= n;
		buf = (u8 *)buf + n;
	}

	return 0;
}

int spinor_write_boot_header(void)
{
	struct spinor_boot_header header;
	int rval;

	memzero(&header, sizeof(struct spinor_boot_header));

	/* SPINOR_LENGTH_REG */
	header.data_len = AMBOOT_BST_FIXED_SIZE;
	header.clk_divider = 16;
	header.dummy_len = 0;
	header.addr_len = 3;
	header.cmd_len = 1;
	/* SPINOR_CTRL_REG */
	header.read_en = 1;
	header.write_en = 0;
	header.rsvd0 = 0;
	header.rxlane = 1;
	header.data_lane = 0x1;
	header.addr_lane = 0x0;
	header.cmd_lane = 0x0;
	header.rsvd1 = 0;
	header.data_dtr = 0;
	header.dummy_dtr = 0;
	header.addr_dtr = 0;
	header.cmd_dtr = 0;
	/* SPINOR_CFG_REG */
	header.rxsampdly = 1;
	header.rsvd2 = 0;
	header.chip_sel = (~(1 << (SPINOR_FLASH_CHIP_SEL))) & 0xff;
	header.hold_timing = 0;
	header.rsvd3 = 0;
	header.hold_pin = 3;
	header.flow_ctrl = 1;
	/* SPINOR_CMD_REG */
	header.cmd = SPINOR_CMD_READ;
	/* SPINOR_ADDRHI_REG */
	header.addr_hi = 0x0;
	/* SPINOR_ADDRLO_REG */
	header.addr_lo = 0x0 + sizeof(struct spinor_boot_header);

	rval = spinor_write_data(0x00000000, &header, sizeof(header));
	if (rval < 0)
		putstr("spinor: failed to write boot header\r\n");

	return rval;
}

int spinor_init(void)
{
	int part_size[TOTAL_FW_PARTS];
	u32 divider, val, i;
	u32 ssec, nsec;
	int ret = 0;

	rct_set_ssi3_pll();

	/* make sure spinor occupy dma channel 0 */
	val = readl(AHB_SCRATCHPAD_REG(0x0c));
	val &= ~(1 << 21);
	writel(AHB_SCRATCHPAD_REG(0x0c), val);

	divider = get_ssi3_freq_hz() / SPINOR_SPI_CLOCK;

	/* global configuration for spinor controller. Note: if the
	 * data received is invalid, we need to tune rxsampldly.*/
	val = SPINOR_CFG_FLOWCTRL_EN |
	      SPINOR_CFG_HOLDPIN(3) |
	      SPINOR_CFG_CHIPSEL(SPINOR_FLASH_CHIP_SEL) |
	      SPINOR_CFG_CLKDIV(divider) |
	      SPINOR_CFG_RXSAMPDLY(SPINOR_RX_SAMPDLY);
	writel(SPINOR_CFG_REG, val);

	/* mask all interrupts */
	writel(SPINOR_INTRMASK_REG, SPINOR_INTR_ALL);

	/* reset tx/rx fifo */
	writel(SPINOR_TXFIFORST_REG, 0x1);
	writel(SPINOR_RXFIFORST_REG, 0x1);
	/* SPINOR_RXFIFOLV_REG may have invalid value, so we read rx fifo
	 * manually to clear tx fifo. This is a hardware bug. */
	while (readl(SPINOR_RXFIFOLV_REG) != 0) {
		val = readl(SPINOR_RXDATA_REG);
	}

	writel(SPINOR_TXFIFOTHLV_REG, 31);
	writel(SPINOR_RXFIFOTHLV_REG, 31);

	flspinor.page_size = SPINOR_PAGE_SIZE;
	flspinor.sector_size = SPINOR_SECTOR_SIZE;
	flspinor.chip_size = SPINOR_CHIP_SIZE;
	flspinor.sectors_per_chip = SPINOR_CHIP_SIZE / SPINOR_SECTOR_SIZE;

	ssec = nsec = 0;
	get_part_size(part_size);

	for (i = 0; i < TOTAL_FW_PARTS -1; i++) {
		nsec = (part_size[i] + flspinor.sector_size - 1) / flspinor.sector_size;

		flspinor.ssec[i] = (nsec == 0) ? 0 : ssec;
		flspinor.nsec[i] = nsec;
		ssec += nsec;
	}

	if (ssec * flspinor.sector_size >= flspinor.chip_size) {
		putstr("partition length in total exceeds spinor size\r\n");
		return -1;
	}

	spinor_flash_reset();

	val = 0;
	spinor_read_reg(SPINOR_CMD_READID, &val, sizeof(val));

	putstr("spinor flash ID is 0x");
	puthex(val);
	putstr("\r\n");
	spi_nor_id = val;

	if (flspinor.chip_size > 0x1000000) {
		ret = spinor_flash_4b_mode();
		if (SUPPORT_DTR_DUAL)
			spinor_read_data = spinor_read_data_ddrdior;
		else
			spinor_read_data = spinor_read_data_diofr;
	} else {
		spi_nor_addr_mode = 3;
		spinor_read_data = spinor_read_data_dior;
	}
	return ret;
}

