/*
 * arch/arm/plat-ambarella/include/plat/audio.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_AUDIO_H__
#define __PLAT_AMBARELLA_AUDIO_H__

/* ==========================================================================*/
#if (CHIP_REV == S2L) || (CHIP_REV == S3)
#define I2S_OFFSET			0x1A000
#else
#define I2S_OFFSET			0xA000
#endif
#define I2S_BASE			(AHB_BASE + I2S_OFFSET)
#define I2S_BASE_PHYS 			(AHB_PHYS_BASE + I2S_OFFSET)
#define I2S_REG(x)			(I2S_BASE + (x))
#define I2S_REG_PHYS(x)  		(I2S_BASE_PHYS + (x))

/* ==========================================================================*/
#define I2S_MODE_OFFSET				0x00
#define I2S_RX_CTRL_OFFSET			0x04
#define I2S_TX_CTRL_OFFSET			0x08
#define I2S_WLEN_OFFSET				0x0c
#define I2S_WPOS_OFFSET				0x10
#define I2S_SLOT_OFFSET				0x14
#define I2S_TX_FIFO_LTH_OFFSET			0x18
#define I2S_RX_FIFO_GTH_OFFSET			0x1c
#define I2S_CLOCK_OFFSET			0x20
#define I2S_INIT_OFFSET				0x24
#define I2S_TX_STATUS_OFFSET			0x28
#define I2S_TX_LEFT_DATA_OFFSET			0x2c
#define I2S_TX_RIGHT_DATA_OFFSET		0x30
#define I2S_RX_STATUS_OFFSET			0x34
#define I2S_RX_DATA_OFFSET			0x38
#define I2S_TX_FIFO_CNTR_OFFSET			0x3c
#define I2S_RX_FIFO_CNTR_OFFSET			0x40
#define I2S_TX_INT_ENABLE_OFFSET		0x44
#define I2S_RX_INT_ENABLE_OFFSET		0x48
#define I2S_RX_ECHO_OFFSET			0x4c
#define I2S_24BITMUX_MODE_OFFSET		0x50
#define I2S_GATEOFF_OFFSET			0x54
#define I2S_CHANNEL_SELECT_OFFSET		0x58
#define I2S_RX_DATA_DMA_OFFSET			0x80
#define I2S_TX_LEFT_DATA_DMA_OFFSET		0xc0

#define I2S_MODE_REG				I2S_REG(0x00)
#define I2S_RX_CTRL_REG				I2S_REG(0x04)
#define I2S_TX_CTRL_REG				I2S_REG(0x08)
#define I2S_WLEN_REG				I2S_REG(0x0c)
#define I2S_WPOS_REG				I2S_REG(0x10)
#define I2S_SLOT_REG				I2S_REG(0x14)
#define I2S_TX_FIFO_LTH_REG			I2S_REG(0x18)
#define I2S_RX_FIFO_GTH_REG			I2S_REG(0x1c)
#define I2S_CLOCK_REG				I2S_REG(0x20)
#define I2S_INIT_REG				I2S_REG(0x24)
#define I2S_TX_STATUS_REG			I2S_REG(0x28)
#define I2S_TX_LEFT_DATA_REG			I2S_REG(0x2c)
#define I2S_TX_RIGHT_DATA_REG			I2S_REG(0x30)
#define I2S_RX_STATUS_REG			I2S_REG(0x34)
#define I2S_RX_DATA_REG				I2S_REG(0x38)
#define I2S_TX_FIFO_CNTR_REG			I2S_REG(0x3c)
#define I2S_RX_FIFO_CNTR_REG			I2S_REG(0x40)
#define I2S_TX_INT_ENABLE_REG			I2S_REG(0x44)
#define I2S_RX_INT_ENABLE_REG			I2S_REG(0x48)
#define I2S_RX_ECHO_REG				I2S_REG(0x4c)
#define I2S_24BITMUX_MODE_REG			I2S_REG(0x50)
#define I2S_GATEOFF_REG				I2S_REG(0x54)
#define I2S_CHANNEL_SELECT_REG			I2S_REG(0x58)
#define I2S_RX_DATA_DMA_REG			I2S_REG_PHYS(0x80)
#define I2S_TX_LEFT_DATA_DMA_REG		I2S_REG_PHYS(0xc0)

#define I2S_TX_FIFO_RESET_BIT		(1 << 4)
#define I2S_RX_FIFO_RESET_BIT		(1 << 3)
#define I2S_TX_ENABLE_BIT		(1 << 2)
#define I2S_RX_ENABLE_BIT		(1 << 1)
#define I2S_FIFO_RESET_BIT		(1 << 0)

#define I2S_RX_LOOPBACK_BIT		(1 << 3)
#define I2S_RX_ORDER_BIT		(1 << 2)
#define I2S_RX_WS_MST_BIT		(1 << 1)
#define I2S_RX_WS_INV_BIT		(1 << 0)

#define I2S_TX_LOOPBACK_BIT		(1 << 7)
#define I2S_TX_ORDER_BIT		(1 << 6)
#define I2S_TX_WS_MST_BIT		(1 << 5)
#define I2S_TX_WS_INV_BIT		(1 << 4)
#define I2S_TX_UNISON_BIT		(1 << 3)
#define I2S_TX_MUTE_BIT			(1 << 2)
#define I2S_TX_MONO_MASK		0xfffffffc
#define I2S_TX_MONO_RIGHT		(1 << 1)
#define I2S_TX_MONO_LEFT		(1 << 0)

#define I2S_CLK_WS_OUT_EN		(1 << 9)
#define I2S_CLK_BCLK_OUT_EN		(1 << 8)
#define I2S_CLK_BCLK_OUTPUT		(1 << 7)
#define I2S_CLK_MASTER_MODE		(I2S_CLK_WS_OUT_EN	|	\
					 I2S_CLK_BCLK_OUT_EN	|	\
					 I2S_CLK_BCLK_OUTPUT)
#define I2S_CLK_TX_PO_FALL		(1 << 6)
#define I2S_CLK_RX_PO_FALL		(1 << 5)
#define I2S_CLK_DIV_MASK		0xffffffe0

#define I2S_RX_SHIFT_ENB		(1 << 1)
#define I2S_TX_SHIFT_ENB		(1 << 0)

#define I2S_2CHANNELS_ENB		0x00
#define I2S_4CHANNELS_ENB		0x01
#define I2S_6CHANNELS_ENB		0x02

#define I2S_FIFO_THRESHOLD_INTRPT	0x08
#define I2S_FIFO_FULL_INTRPT		0x02
#define I2S_FIFO_EMPTY_INTRPT		0x01

/* I2S_24BITMUX_MODE_REG */
#define I2S_24BITMUX_MODE_ENABLE		0x1
#define I2S_24BITMUX_MODE_FDMA_BURST_DIS	0x2
#define I2S_24BITMUX_MODE_RST_CHAN0		0x4
#define I2S_24BITMUX_MODE_DMA_BOOTSEL		0x8

/* ==========================================================================*/
#define DAI_CLOCK_MASK			0x0000001f

#define MAX_MCLK_IDX_NUM		15

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct notifier_block;

struct ambarella_i2s_interface {
	u8 state;
	u8 mode;
	u8 sfreq;
	u32 clksrc;
	u8 ms_mode;
	u32 mclk;
	u8 ch;
	u8 oversample;
	u8 word_order;
	u8 word_len;
	u8 word_pos;
	u8 slots;
};

#define MAX_OVERSAMPLE_IDX_NUM	9
enum AudioCodec_OverSample {
	AudioCodec_128xfs = 0,
	AudioCodec_256xfs = 1,
	AudioCodec_384xfs = 2,
	AudioCodec_512xfs = 3,
	AudioCodec_768xfs = 4,
	AudioCodec_1024xfs = 5,
	AudioCodec_1152xfs = 6,
	AudioCodec_1536xfs = 7,
	AudioCodec_2304xfs = 8
};

enum AudioCodec_MCLK {
	AudioCodec_18_432M = 0,
	AudioCodec_16_9344M = 1,
	AudioCodec_12_288M = 2,
	AudioCodec_11_2896M = 3,
	AudioCodec_9_216M = 4,
	AudioCodec_8_4672M = 5,
	AudioCodec_8_192M = 6,
	AudioCodec_6_144 = 7,
	AudioCodec_5_6448M = 8,
	AudioCodec_4_608M = 9,
	AudioCodec_4_2336M = 10,
	AudioCodec_4_096M = 11,
	AudioCodec_3_072M = 12,
	AudioCodec_2_8224M = 13,
	AudioCodec_2_048M = 14
};

enum audio_in_freq_e
{
	AUDIO_SF_reserved = 0,
	AUDIO_SF_96000,
	AUDIO_SF_48000,
	AUDIO_SF_44100,
	AUDIO_SF_32000,
	AUDIO_SF_24000,
	AUDIO_SF_22050,
	AUDIO_SF_16000,
	AUDIO_SF_12000,
	AUDIO_SF_11025,
	AUDIO_SF_8000,
};

enum Audio_Notify_Type
{
	AUDIO_NOTIFY_UNKNOWN,
	AUDIO_NOTIFY_INIT,
	AUDIO_NOTIFY_SETHWPARAMS,
	AUDIO_NOTIFY_REMOVE
};

enum DAI_Mode
{
	DAI_leftJustified_Mode = 0,
	DAI_rightJustified_Mode = 1,
	DAI_MSBExtend_Mode = 2,
	DAI_I2S_Mode = 4,
	DAI_DSP_Mode = 6
};

enum DAI_MS_MODE
{
	DAI_SLAVE = 0,
	DAI_MASTER = 1
};

enum DAI_resolution
{
	DAI_16bits = 0,
	DAI_18bits = 1,
	DAI_20bits = 2,
	DAI_24bits = 3,
	DAI_32bits = 4

};

enum DAI_ifunion
{
	DAI_union = 0,
	DAI_nonunion = 1
};

enum DAI_WordOrder
{
	DAI_MSB_FIRST = 0,
	DAI_LSB_FIRST = 1
};

enum DAI_INIT_CTL
{
	DAI_FIFO_RST = 1,
	DAI_RX_EN = 2,
	DAI_TX_EN = 4
};

#define DAI_32slots	32
#define DAI_64slots	64
#define DAI_48slots	48

#define AMBARELLA_CLKSRC_ONCHIP			0x0
#define AMBARELLA_CLKSRC_SP_CLK			0x1
#define AMBARELLA_CLKSRC_CLK_SI			0x2
#define AMBARELLA_CLKSRC_EXTERNAL		0x3
#define AMBARELLA_CLKSRC_LVDS_IDSP_SCLK		0x4

#define AMBARELLA_CLKDIV_LRCLK	0

/* ==========================================================================*/
extern int ambarella_init_audio(void);

extern void ambarella_audio_notify_transition(
	struct ambarella_i2s_interface *data, unsigned int type);
extern int ambarella_audio_register_notifier(struct notifier_block *nb);
extern int ambarella_audio_unregister_notifier(struct notifier_block *nb);

extern struct ambarella_i2s_interface get_audio_i2s_interface(void);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_AUDIO_H__ */

