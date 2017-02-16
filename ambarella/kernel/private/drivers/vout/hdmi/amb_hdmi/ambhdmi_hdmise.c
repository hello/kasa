/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_hdmise.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include "dsp_cmd_msg.h"

typedef struct {
        u32		reg;		//Address
        u32		val;		//Value
} hdmi_packet_t;

static struct {
        hdmi_packet_t		avi[9];
        hdmi_packet_t		audio[9];
#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
        hdmi_packet_t		vendor[9];
#else
        hdmi_packet_t		vendor[8];	//A5S
#endif
} HDMI_PACKETS = {
        .avi[0]		= {HDMI_PACKET_AVI0_REG, 0},
        .avi[1]		= {HDMI_PACKET_AVI1_REG, 0},
        .avi[2]		= {HDMI_PACKET_AVI2_REG, 0},
        .avi[3]		= {HDMI_PACKET_AVI3_REG, 0},
        .avi[4]		= {HDMI_PACKET_AVI4_REG, 0},
        .avi[5]		= {HDMI_PACKET_AVI5_REG, 0},
        .avi[6]		= {HDMI_PACKET_AVI6_REG, 0},
        .avi[7]		= {HDMI_PACKET_AVI7_REG, 0},
        .avi[8]		= {HDMI_PACKET_AVI8_REG, 0},

        .audio[0]	= {HDMI_PACKET_AUDIO0_REG, 0},
        .audio[1]	= {HDMI_PACKET_AUDIO1_REG, 0},
        .audio[2]	= {HDMI_PACKET_AUDIO2_REG, 0},
        .audio[3]	= {HDMI_PACKET_AUDIO3_REG, 0},
        .audio[4]	= {HDMI_PACKET_AUDIO4_REG, 0},
        .audio[5]	= {HDMI_PACKET_AUDIO5_REG, 0},
        .audio[6]	= {HDMI_PACKET_AUDIO6_REG, 0},
        .audio[7]	= {HDMI_PACKET_AUDIO7_REG, 0},
        .audio[8]	= {HDMI_PACKET_AUDIO8_REG, 0},

        .vendor[0]	= {HDMI_PACKET_VS0_REG, 0},
        .vendor[1]	= {HDMI_PACKET_VS1_REG, 0},
        .vendor[2]	= {HDMI_PACKET_VS2_REG, 0},
        .vendor[3]	= {HDMI_PACKET_VS3_REG, 0},
        .vendor[4]	= {HDMI_PACKET_VS4_REG, 0},
        .vendor[5]	= {HDMI_PACKET_VS5_REG, 0},
        .vendor[6]	= {HDMI_PACKET_VS6_REG, 0},
        .vendor[7]	= {HDMI_PACKET_VS7_REG, 0},
#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
        .vendor[8]	= {HDMI_PACKET_VS8_REG},
#endif
};

#define PACKET_NUM(x)   (sizeof(HDMI_PACKETS.x) / sizeof(hdmi_packet_t))

static u32 pixel_clock;

static void ambhdmi_hdmise_flush_packets(void)
{
        int     i, j;
        u32     reg;
        u8      sum, check_sum;

        /* AVI */
        sum = 0;
        HDMI_PACKETS.avi[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(avi); i++) {
                reg = HDMI_PACKETS.avi[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (reg & 0xFF);
                        reg >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.avi[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(avi); i++) {
                amba_writel(HDMI_PACKETS.avi[i].reg, HDMI_PACKETS.avi[i].val);
        }

        /* Audio */
        sum = 0;
        HDMI_PACKETS.audio[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(audio); i++) {
                reg = HDMI_PACKETS.audio[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (reg & 0xFF);
                        reg >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.audio[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(audio); i++) {
                amba_writel(HDMI_PACKETS.audio[i].reg, HDMI_PACKETS.audio[i].val);
        }

        /* Vendor Specific */
        sum = 0;
        HDMI_PACKETS.vendor[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(vendor); i++) {
                reg = HDMI_PACKETS.vendor[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (reg & 0xFF);
                        reg >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.vendor[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(vendor); i++) {
                amba_writel(HDMI_PACKETS.vendor[i].reg, HDMI_PACKETS.vendor[i].val);
        }
}

static void ambhdmi_hdmise_config_audio(struct ambarella_i2s_interface *i2s_config)
{
        u32		mclk, n, cts, src, layout, mode, order, len, pos;
        u8		cc;

        /* MCLK */
        switch (i2s_config->oversample) {
        case AudioCodec_128xfs:
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_128FS;
                break;

        case AudioCodec_256xfs:
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_256FS;
                break;

        case AudioCodec_384xfs:
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_384FS;
                break;

        case AudioCodec_512xfs:
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_512FS;
                break;

        case AudioCodec_768xfs:
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_768FS;
                break;

        default:
                vout_notice("Incorrect MCLK!\n");
                mclk = HDMI_AUNIT_MCLK_INPUT_FREQ_MODE_128FS;
                break;
        }

        /* n = 128 * fs / 1000, cts = fp / 1000 */
        switch (i2s_config->sfreq) {
        case AUDIO_SF_8000:
                n	= 128 * 8000 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_11025:
                n	= 128 * 11025 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_12000:
                n	= 128 * 12000 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_16000:
                n	= 128 * 16000 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_22050:
                n	= 128 * 22050 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_24000:
                n	= 128 * 24000 / 1000;
                cts	= pixel_clock;
                break;

        case AUDIO_SF_32000:
                switch (pixel_clock) {
                case 27000:
                        n	= 4096;
                        cts	= 27000;
                        break;

                case 74176:
                        n	= 11648;
                        cts	= 210937;
                        break;

                case 74250:
                        n	= 4096;
                        cts	= 74250;
                        break;

                case 148352:
                        n	= 11648;
                        cts	= 148500;
                        break;

                case 148500:
                        n	= 4096;
                        cts	= 148500;
                        break;

                default:
                        n	= 128 * 32000 / 1000;
                        cts	= pixel_clock;
                        break;
                }
                break;

        case AUDIO_SF_44100:
                switch (pixel_clock) {
                case 27000:
                        n	= 6272;
                        cts	= 30000;
                        break;

                case 74176:
                        n	= 17836;
                        cts	= 234375;
                        break;

                case 74250:
                        n	= 6272;
                        cts	= 82500;
                        break;

                case 148352:
                        n	= 8918;
                        cts	= 234375;
                        break;

                case 148500:
                        n	= 6272;
                        cts	= 165000;
                        break;

                default:
                        n	= 128 * 44100 / 1000;
                        cts	= pixel_clock;
                        break;
                }
                break;

        case AUDIO_SF_48000:
                switch (pixel_clock) {
                case 27000:
                        n	= 6144;
                        cts	= 27000;
                        break;

                case 74176:
                        n	= 11648;
                        cts	= 140625;
                        break;

                case 74250:
                        n	= 6144;
                        cts	= 74250;
                        break;

                case 148352:
                        n	= 5824;
                        cts	= 140625;
                        break;

                case 148500:
                        n	= 6144;
                        cts	= 148500;
                        break;

                default:
                        n	= 128 * 48000 / 1000;
                        cts	= pixel_clock;
                        break;
                }
                break;

        case AUDIO_SF_96000:
                switch (pixel_clock) {
                case 27000:
                        n	= 12288;
                        cts	= 27000;
                        break;

                case 74176:
                        n	= 23296;
                        cts	= 140625;
                        break;

                case 74250:
                        n	= 12288;
                        cts	= 74250;
                        break;

                case 148352:
                        n	= 11648;
                        cts	= 140625;
                        break;

                case 148500:
                        n	= 12288;
                        cts	= 148500;
                        break;

                default:
                        n	= 128 * 96000 / 1000;
                        cts	= pixel_clock;
                        break;
                }
                break;

        default:
                vout_notice("%s: Incorrect Fs!\n", __func__);
                n	= 128 * 44100 / 1000;
                cts	= pixel_clock;
                break;
        }

        /* SRC, LAYOUT */
        switch (i2s_config->ch) {
        case 2:
                src	= HDMI_AUNIT_SRC_I2S0_EN;
                layout	= HDMI_AUNIT_LAYOUT_LAYOUT0;
                cc	= 2 - 1;
                break;

        case 4:
                src	= HDMI_AUNIT_SRC_I2S0_EN | HDMI_AUNIT_SRC_I2S1_EN;
                layout	= HDMI_AUNIT_LAYOUT_LAYOUT1;
                cc	= 4 - 1;
                break;

        case 6:
                src	= HDMI_AUNIT_SRC_I2S0_EN | HDMI_AUNIT_SRC_I2S1_EN |
                          HDMI_AUNIT_SRC_I2S2_EN;
                layout	= HDMI_AUNIT_LAYOUT_LAYOUT1;
                cc	= 6 - 1;
                break;

        default:
                src	= HDMI_AUNIT_SRC_I2S0_EN;
                layout	= HDMI_AUNIT_LAYOUT_LAYOUT0;
                cc	= 2 - 1;
                break;
        }

        /* MODE */
        switch (i2s_config->mode) {
        case DAI_leftJustified_Mode:
                mode = HDMI_I2S_MODE_DAI_MODE_LEFT_JUSTIFY;
                break;

        case DAI_rightJustified_Mode:
                mode = HDMI_I2S_MODE_DAI_MODE_RIGHT_JUSTIFY;
                break;

        case DAI_MSBExtend_Mode:
                mode = HDMI_I2S_MODE_DAI_MODE_MSB_EXT;
                break;

        case DAI_I2S_Mode:
                mode = HDMI_I2S_MODE_DAI_MODE_I2S;
                break;

        default:
                vout_notice("Incorrect I2S mode!\n");
                mode = HDMI_I2S_MODE_DAI_MODE_I2S;
                break;
        }

        /* ORDER */
        switch (i2s_config->word_order) {
        case DAI_MSB_FIRST:
                order = 0;
                break;

        case DAI_LSB_FIRST:
                order = 0x1 << 2;
                break;

        default:
                vout_notice("Incorrect MSB/LSB order!\n");
                order = 0;
                break;
        }

        /* LEN, POS */
        switch (i2s_config->word_len) {
        case DAI_16bits:
                len = 16 - 1;
                pos = 16 - 1;
                break;

        case DAI_18bits:
                len = 18 - 1;
                pos = 18 - 1;
                break;

        case DAI_20bits:
                len = 20 - 1;
                pos = 20 - 1;
                break;

        case DAI_24bits:
                len = 24 - 1;
                pos = 24 - 1;
                break;

        case DAI_32bits:
                len = 32 - 1;
                pos = 32 - 1;
                break;

        default:
                vout_notice("Incorrect WLEN!\n");
                len = 16 - 1;
                pos = 16 - 1;
                break;
        }

        amba_writel(HDMI_AUNIT_MCLK_REG, mclk);
        amba_writel(HDMI_AUNIT_N_REG, HDMI_AUNIT_N(n));
#if 0
        amba_writel(HDMI_AUNIT_NCTS_CTRL_REG, HDMI_AUNIT_NCTS_CTRL_NCTS_EN |
                    HDMI_AUNIT_NCTS_CTRL_CTS_SEL_SW_MODE);
        amba_writel(HDMI_AUNIT_CTS_REG, HDMI_AUNIT_CTS(cts));
#else
        amba_writel(HDMI_AUNIT_NCTS_CTRL_REG, HDMI_AUNIT_NCTS_CTRL_NCTS_EN |
                    HDMI_AUNIT_NCTS_CTRL_CTS_SEL_HW_MODE);
        amba_writel(HDMI_AUNIT_CTS_REG, HDMI_AUNIT_CTS(cts));
#endif
        amba_writel(HDMI_AUNIT_SRC_REG,	src);
        amba_writel(HDMI_AUNIT_LAYOUT_REG, layout);
        amba_writel(HDMI_I2S_MODE_REG, mode);
        amba_writel(HDMI_I2S_RX_CTRL_REG, order);
        amba_writel(HDMI_I2S_WLEN_REG, len);
        amba_writel(HDMI_I2S_WPOS_REG, pos);
        amba_writel(HDMI_I2S_SLOT_REG, 0);
        amba_writel(HDMI_I2S_RX_FIFO_GTH_REG, 3);
        amba_writel(HDMI_I2S_RX_CLOCK_REG, HDMI_I2S_RX_CLOCK_RX_I2S_CLK_POL(0));
        amba_writel(HDMI_I2S_GATE_OFF_REG, ~HDMI_I2S_GATE_OFF_RX_GATE_OFF_EN);

        /* Audio Info Frame */
        HDMI_PACKETS.audio[0].val	=
                HDMI_PACKET_AUDIO0_AUDIO_HB0(0x84) |
                HDMI_PACKET_AUDIO0_AUDIO_HB1(0x01) |
                HDMI_PACKET_AUDIO0_AUDIO_HB2(0x0a);
        HDMI_PACKETS.audio[1].val	=
                HDMI_PACKET_AUDIO1_AUDIO_PB1((0x00 | cc)) |
                HDMI_PACKET_AUDIO1_AUDIO_PB2(0x00) |
                HDMI_PACKET_AUDIO1_AUDIO_PB3(0x00);
        HDMI_PACKETS.audio[2].val	=
                HDMI_PACKET_AUDIO2_AUDIO_PB4(0x00) |
                HDMI_PACKET_AUDIO2_AUDIO_PB5(0x00) |
                HDMI_PACKET_AUDIO2_AUDIO_PB6(0x00);
}

/* ========================================================================== */
static void ambhdmi_hdmise_stop(struct ambhdmi_sink *phdmi_sink)
{
        amba_writel(HDMI_OP_MODE_REG, ~HDMI_OP_MODE_OP_EN);
}

static int ambhdmi_hdmise_audio_transition(struct notifier_block *nb,
                                           unsigned long val, void *data)
{
        struct ambarella_i2s_interface *i2s_config = data;
        u32 state;
        u32 _val, _hdmise_en;

        if (val != AUDIO_NOTIFY_SETHWPARAMS)
                return 0;

        state = amba_readl(HDMI_OP_MODE_REG);
        if (!(state & HDMI_OP_MODE_OP_EN))
                return 0;

        /* Enable HDMISE Clock */
        _val = amba_readl(HDMI_CLOCK_GATED_REG);
        _hdmise_en = _val & HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
        if (!_hdmise_en) {
                _val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                amba_writel(HDMI_CLOCK_GATED_REG, _val);
        }

        mdelay(100);
        if (!(amba_readl(HDMI_I2S_INIT_REG) & 0x2)) {
                //amba_writel(HDMI_I2S_INIT_REG, HDMI_I2S_INIT_DAI_RESET(1));
                amba_writel(HDMI_I2S_INIT_REG, HDMI_I2S_INIT_RX_ENABLE);
                mdelay(100);
        }

        ambhdmi_hdmise_config_audio(i2s_config);
        ambhdmi_hdmise_flush_packets();
        amba_writel(HDMI_PACKET_TX_CTRL_REG,
                    HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN |
                    HDMI_PACKET_TX_CTRL_AUD_RPT |
                    HDMI_PACKET_TX_CTRL_AUD_EN |
                    HDMI_PACKET_TX_CTRL_AVI_RPT |
                    HDMI_PACKET_TX_CTRL_AVI_EN |
                    HDMI_PACKET_TX_CTRL_GEN_RPT |
                    HDMI_PACKET_TX_CTRL_GEN_EN |
                    HDMI_PACKET_TX_CTRL_VS_RPT |
                    HDMI_PACKET_TX_CTRL_VS_EN
                   );


        /* Clock Gating */
        if (!_hdmise_en) {
                _val = amba_readl(HDMI_CLOCK_GATED_REG);
                _val &= ~HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                amba_writel(HDMI_CLOCK_GATED_REG, _val);
        }

        return 0;
}

static void ambhdmi_hdmise_set(struct ambhdmi_sink *phdmi_sink,
                               int _cs, enum ddd_structure ddd_structure,
                               enum amba_vout_hdmi_overscan _overscan,
                               const amba_hdmi_video_timing_t *vt)
{
        struct ambarella_i2s_interface	i2s_config;
        u32				i, cts, _ar;
        u8				cs, vic, pr, ar, overscan;// cm;

        /* Use HDCP & EESS */
        amba_writel(HDMI_EESS_CTL_REG, HDMI_HDCPCE_CTL_USE_EESS(1) |
                    HDMI_HDCPCE_CTL_HDCPCE_EN);

        /*PHY*/
        amba_writel(HDMI_PHY_CTRL_REG, 0);
        if (vt->pixel_clock >= 148352 && vt->pixel_clock <= 148500) {
                amba_writel(HDMI_PHY_CTRL_REG,
                            HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
                            HDMI_PHY_CTRL_NON_RESET_HDMI_PHY |
                            HDMI_PHY_CTRL_10_PRE_EMPHASIS |
                            HDMI_PHY_CTRL_1P2_MA_SINK_CURRENT
                           );
        } else {
                amba_writel(HDMI_PHY_CTRL_REG,
                            HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
                            HDMI_PHY_CTRL_NON_RESET_HDMI_PHY
                           );
        }

        /*AFIFO*/
        amba_writel(HDMI_P2P_AFIFO_LEVEL_REG,
                    HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(12) |
                    HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(4) |
                    HDMI_P2P_AFIFO_LEVEL_MAX_USAGE_LEVEL(7)	|
                    HDMI_P2P_AFIFO_LEVEL_MIN_USAGE_LEVEL(7) |
                    HDMI_P2P_AFIFO_LEVEL_CURRENT_USAGE_LEVEL(7)
                   );
        amba_writel(HDMI_P2P_AFIFO_CTRL_REG, HDMI_P2P_AFIFO_CTRL_EN);

        /*Video*/
#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
        amba_writel(HDMI_VUNIT_VBLANK_VFRONT_REG, vt->vsync_offset);
        amba_writel(HDMI_VUNIT_VBLANK_PULSE_WIDTH_REG, vt->vsync_width);
        amba_writel(HDMI_VUNIT_VBLANK_VBACK_REG,
                    vt->v_blanking - vt->vsync_offset - vt->vsync_width);

        if (ddd_structure == DDD_SIDE_BY_SIDE_FULL) {
                amba_writel(HDMI_VUNIT_HBLANK_HFRONT_REG, vt->hsync_offset * 2);
                amba_writel(HDMI_VUNIT_HBLANK_PULSE_WIDTH_REG, vt->hsync_width * 2);
                amba_writel(HDMI_VUNIT_HBLANK_HBACK_REG,
                            vt->h_blanking * 2 - vt->hsync_offset * 2 - vt->hsync_width * 2);
        } else {
                amba_writel(HDMI_VUNIT_HBLANK_HFRONT_REG, vt->hsync_offset);
                amba_writel(HDMI_VUNIT_HBLANK_PULSE_WIDTH_REG, vt->hsync_width);
                amba_writel(HDMI_VUNIT_HBLANK_HBACK_REG,
                            vt->h_blanking - vt->hsync_offset - vt->hsync_width);
        }
#else
        amba_writel(HDMI_VUNIT_VBLANK_REG,
                    HDMI_VUNIT_VBLANK_LEFT_OFFSET(vt->vsync_offset) |
                    HDMI_VUNIT_VBLANK_PULSE_WIDTH(vt->vsync_width) |
                    HDMI_VUNIT_VBLANK_RIGHT_OFFSET(vt->v_blanking -
                                                   vt->vsync_offset - vt->vsync_width)
                   );

        amba_writel(HDMI_VUNIT_HBLANK_REG,
                    HDMI_VUNIT_HBLANK_LEFT_OFFSET(vt->hsync_offset) |
                    HDMI_VUNIT_HBLANK_PULSE_WIDTH(vt->hsync_width) |
                    HDMI_VUNIT_HBLANK_RIGHT_OFFSET(vt->h_blanking -
                                                   vt->hsync_offset - vt->hsync_width)
                   );
#endif

        if (ddd_structure == DDD_FRAME_PACKING) {
                amba_writel(HDMI_VUNIT_VACTIVE_REG,
                            vt->v_active * 2 + vt->v_blanking);
        } else {
                amba_writel(HDMI_VUNIT_VACTIVE_REG, vt->v_active);
        }
        if (ddd_structure == DDD_SIDE_BY_SIDE_FULL) {
                amba_writel(HDMI_VUNIT_HACTIVE_REG, vt->h_active * 2);
        } else {
                amba_writel(HDMI_VUNIT_HACTIVE_REG, vt->h_active);
        }
        amba_writel(HDMI_VUNIT_CTRL_REG,
                    HDMI_VUNIT_CTRL_VIDEO_MODE(vt->interlace) |
                    HDMI_VUNIT_CTRL_HSYNC_POL(~vt->hsync_polarity) |
                    HDMI_VUNIT_CTRL_VSYNC_POL(~vt->vsync_polarity)
                   );
        amba_writel(HDMI_VUNIT_VSYNC_DETECT_REG, HDMI_VUNIT_VSYNC_DETECT_EN);

        /*Audio*/
        pixel_clock = vt->pixel_clock;
        i2s_config = get_audio_i2s_interface();
        ambhdmi_hdmise_config_audio(&i2s_config);

        cts = vt->pixel_clock;
        amba_writel(HDMI_AUNIT_CTS_REG, HDMI_AUNIT_CTS(cts));

        if (phdmi_sink->edid.interface != HDMI)
                amba_writel(HDMI_I2S_GATE_OFF_REG,
                            HDMI_I2S_GATE_OFF_RX_GATE_OFF_EN);

        /*Misc Packet*/
        amba_writel(HDMI_PACKET_GENERAL_CTRL_REG,
                    HDMI_PACKET_GENERAL_CTRL_CLR_AVMUTE_EN);
        amba_writel(HDMI_PACKET_MISC_REG, 0);

        /* AVI Info Frame */
        switch (_cs) {
        case AMBA_VOUT_HDMI_CS_RGB:
                cs = 0;
                break;

        case AMBA_VOUT_HDMI_CS_YCBCR_444:
                cs = 2;
                break;

        case AMBA_VOUT_HDMI_CS_YCBCR_422:
                cs = 1;
                break;

        default:
                cs = 0;
                break;
        }
        vic	= 0;
        pr	= 1;
        _ar	= AMBA_VIDEO_RATIO_AUTO;
        //cm	= COLORIMETRY_NO_DATA;
        for (i = 0; i < MAX_CEA_TIMINGS; i++) {
                if (CEA_Timings[i].vmode == vt->vmode) {
                        vic	= i;
                        pr	= CEA_Timings[i].pixel_repetition;
                        _ar	= CEA_Timings[i].aspect_ratio;
                        //cm	= CEA_Timings[i].colorimetry;
                        break;
                }
        }
        switch (_ar) {
        case AMBA_VIDEO_RATIO_AUTO:
                ar = 0;
                break;

        case AMBA_VIDEO_RATIO_4_3:
                ar = 1;
                break;

        case AMBA_VIDEO_RATIO_16_9:
                ar = 2;
                break;

        default:
                ar = 0;
                break;
        }
        switch (_overscan) {
        case AMBA_VOUT_HDMI_FORCE_OVERSCAN:
                overscan = 1;
                break;

        case AMBA_VOUT_HDMI_NON_FORCE_OVERSCAN:
                overscan = 2;
                break;

        default:
                overscan = 0;
                break;
        }

        HDMI_PACKETS.avi[0].val		=
                HDMI_PACKET_AVI0_AVI_HB0(0x82) |
                HDMI_PACKET_AVI0_AVI_HB1(0x02) |
                HDMI_PACKET_AVI0_AVI_HB2(0x0d);
        HDMI_PACKETS.avi[1].val		=
                HDMI_PACKET_AVI1_AVI_PB0(0x00) |
                HDMI_PACKET_AVI1_AVI_PB1((cs << 5) | overscan) |
                HDMI_PACKET_AVI1_AVI_PB2((ar << 4) | 0x08) |
                HDMI_PACKET_AVI1_AVI_PB3(0x00);
        HDMI_PACKETS.avi[2].val		=
                HDMI_PACKET_AVI2_AVI_PB4(vic) |
                HDMI_PACKET_AVI2_AVI_PB5(pr - 1) |
                HDMI_PACKET_AVI2_AVI_PB6(0x00);

        /* Vendor Specific Info Frame */
        HDMI_PACKETS.vendor[0].val	=
                HDMI_PACKET_VS0_VS_HB0(0x81) |
                HDMI_PACKET_VS0_VS_HB1(0x01) |
                HDMI_PACKET_VS0_VS_HB2(0x06);
        HDMI_PACKETS.vendor[1].val	=
                HDMI_PACKET_VS1_VS_PB1(0x03) |
                HDMI_PACKET_VS1_VS_PB2(0x0c) |
                HDMI_PACKET_VS1_VS_PB3(0x00);
        if ((ddd_structure == DDD_RESERVED || ddd_structure == DDD_UNSUPPORTED)
            && !phdmi_sink->edid.ddd_structure.ddd_num) {
                HDMI_PACKETS.vendor[2].val	=
                        HDMI_PACKET_VS2_VS_PB4(0x00)	|
                        HDMI_PACKET_VS2_VS_PB5(0x00)	|
                        HDMI_PACKET_VS2_VS_PB6(0x00);
        } else {
                HDMI_PACKETS.vendor[2].val	=
                        HDMI_PACKET_VS2_VS_PB4(0x40)                 |
                        HDMI_PACKET_VS2_VS_PB5((ddd_structure << 4)) |
                        HDMI_PACKET_VS2_VS_PB6(0x00);
        }

        ambhdmi_hdmise_flush_packets();

        if (phdmi_sink->edid.interface == HDMI) {
                amba_writel(HDMI_PACKET_TX_CTRL_REG,
                            HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN |
                            HDMI_PACKET_TX_CTRL_AUD_RPT |
                            HDMI_PACKET_TX_CTRL_AUD_EN |
                            HDMI_PACKET_TX_CTRL_AVI_RPT |
                            HDMI_PACKET_TX_CTRL_AVI_EN |
                            HDMI_PACKET_TX_CTRL_GEN_RPT |
                            HDMI_PACKET_TX_CTRL_GEN_EN |
                            HDMI_PACKET_TX_CTRL_VS_RPT |
                            HDMI_PACKET_TX_CTRL_VS_EN
                           );
        } else {
                amba_writel(HDMI_PACKET_TX_CTRL_REG,
                            HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN |
                            HDMI_PACKET_TX_CTRL_AVI_RPT |
                            HDMI_PACKET_TX_CTRL_AVI_EN |
                            HDMI_PACKET_TX_CTRL_GEN_RPT |
                            HDMI_PACKET_TX_CTRL_GEN_EN |
                            HDMI_PACKET_TX_CTRL_VS_RPT |
                            HDMI_PACKET_TX_CTRL_VS_EN
                           );
        }

        /*Test*/
        amba_writel(HDMI_HDMISE_TM_REG, 0);
}

static void ambhdmi_hdmise_run(struct ambhdmi_sink *phdmi_sink)
{
        u32			mode;

        vout_notice("%s: %d @ 0x%x!\n", __func__,
                    phdmi_sink->edid.interface, phdmi_sink->regbase);

        if (phdmi_sink->edid.interface == HDMI) {
                mode = HDMI_OP_MODE_OP_MODE_HDMI | HDMI_OP_MODE_OP_EN;
        } else {
                mode = HDMI_OP_MODE_OP_MODE_DVI | HDMI_OP_MODE_OP_EN;
        }

        amba_writel(HDMI_OP_MODE_REG, mode);
}

static void ambhdmi_hdmise_init(struct ambhdmi_sink *phdmi_sink,
                                int cs, enum ddd_structure ddd_structure,
                                enum amba_vout_hdmi_overscan overscan,
                                const amba_hdmi_video_timing_t *vt)
{
        u32 val;

        /* Soft Reset HDMISE */
        amba_writel(HDMI_HDMISE_SOFT_RESET_REG,	HDMI_HDMISE_SOFT_RESET);
        amba_writel(HDMI_HDMISE_SOFT_RESET_REG,	~HDMI_HDMISE_SOFT_RESET);

        /* Reset CEC */
        amba_writel(CEC_CTRL_REG, 0x1 << 31);

        /* Clock Gating */
        amba_writel(HDMI_CLOCK_GATED_REG, 0);

        /* Enable Hotplug detect and loss interrupt */
        amba_writel(HDMI_INT_ENABLE_REG,
                    HDMI_INT_ENABLE_PHY_RX_SENSE_REMOVE_EN |
                    HDMI_INT_ENABLE_PHY_RX_SENSE_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_LOSS_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_DETECT_EN
                   );

        msleep(10);

        disable_irq(phdmi_sink->irq);

        /* Enable HDMISE Clock */
        val = amba_readl(HDMI_CLOCK_GATED_REG);
        val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
        amba_writel(HDMI_CLOCK_GATED_REG, val);

        ambhdmi_hdmise_stop(phdmi_sink);
        ambhdmi_hdmise_set(phdmi_sink, cs, ddd_structure, overscan, vt);
        ambhdmi_hdmise_run(phdmi_sink);

        /* Clock Gating */
        if (phdmi_sink->video_sink.hdmi_plug == AMBA_VOUT_SINK_REMOVED) {
                val = amba_readl(HDMI_CLOCK_GATED_REG);
                val &= ~HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                amba_writel(HDMI_CLOCK_GATED_REG, val);
        }

        enable_irq(phdmi_sink->irq);
}

