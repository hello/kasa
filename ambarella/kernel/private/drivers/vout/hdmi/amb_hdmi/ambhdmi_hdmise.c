/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_hdmise.c
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
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
        .avi[0]		= {HDMI_PACKET_AVI0_OFFSET, 0},
        .avi[1]		= {HDMI_PACKET_AVI1_OFFSET, 0},
        .avi[2]		= {HDMI_PACKET_AVI2_OFFSET, 0},
        .avi[3]		= {HDMI_PACKET_AVI3_OFFSET, 0},
        .avi[4]		= {HDMI_PACKET_AVI4_OFFSET, 0},
        .avi[5]		= {HDMI_PACKET_AVI5_OFFSET, 0},
        .avi[6]		= {HDMI_PACKET_AVI6_OFFSET, 0},
        .avi[7]		= {HDMI_PACKET_AVI7_OFFSET, 0},
        .avi[8]		= {HDMI_PACKET_AVI8_OFFSET, 0},

        .audio[0]	= {HDMI_PACKET_AUDIO0_OFFSET, 0},
        .audio[1]	= {HDMI_PACKET_AUDIO1_OFFSET, 0},
        .audio[2]	= {HDMI_PACKET_AUDIO2_OFFSET, 0},
        .audio[3]	= {HDMI_PACKET_AUDIO3_OFFSET, 0},
        .audio[4]	= {HDMI_PACKET_AUDIO4_OFFSET, 0},
        .audio[5]	= {HDMI_PACKET_AUDIO5_OFFSET, 0},
        .audio[6]	= {HDMI_PACKET_AUDIO6_OFFSET, 0},
        .audio[7]	= {HDMI_PACKET_AUDIO7_OFFSET, 0},
        .audio[8]	= {HDMI_PACKET_AUDIO8_OFFSET, 0},

        .vendor[0]	= {HDMI_PACKET_VS0_OFFSET, 0},
        .vendor[1]	= {HDMI_PACKET_VS1_OFFSET, 0},
        .vendor[2]	= {HDMI_PACKET_VS2_OFFSET, 0},
        .vendor[3]	= {HDMI_PACKET_VS3_OFFSET, 0},
        .vendor[4]	= {HDMI_PACKET_VS4_OFFSET, 0},
        .vendor[5]	= {HDMI_PACKET_VS5_OFFSET, 0},
        .vendor[6]	= {HDMI_PACKET_VS6_OFFSET, 0},
        .vendor[7]	= {HDMI_PACKET_VS7_OFFSET, 0},
#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
        .vendor[8]	= {HDMI_PACKET_VS8_OFFSET},
#endif
};

#define PACKET_NUM(x)   (sizeof(HDMI_PACKETS.x) / sizeof(hdmi_packet_t))

static u32 pixel_clock;

static void ambhdmi_hdmise_flush_packets(struct ambhdmi_sink *phdmi_sink)
{
	void __iomem  *reg;
	u32 val;
        int     i, j;
        u8      sum, check_sum;

        /* AVI */
        sum = 0;
        HDMI_PACKETS.avi[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(avi); i++) {
                val = HDMI_PACKETS.avi[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (val & 0xFF);
                        val >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.avi[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(avi); i++) {
		reg = phdmi_sink->regbase + HDMI_PACKETS.avi[i].reg;
                writel_relaxed(HDMI_PACKETS.avi[i].val, reg);
        }

        /* Audio */
        sum = 0;
        HDMI_PACKETS.audio[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(audio); i++) {
                val = HDMI_PACKETS.audio[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (val & 0xFF);
                        val >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.audio[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(audio); i++) {
		reg = phdmi_sink->regbase + HDMI_PACKETS.audio[i].reg;
                writel_relaxed(HDMI_PACKETS.audio[i].val, reg);
        }

        /* Vendor Specific */
        sum = 0;
        HDMI_PACKETS.vendor[1].val &= 0xFFFFFF00;
        for (i = 0; i < PACKET_NUM(vendor); i++) {
                val = HDMI_PACKETS.vendor[i].val;
                for (j = 0; j < 4; j++) {
                        sum += (val & 0xFF);
                        val >>= 8;
                }
        }
        check_sum = 256 - sum;
        HDMI_PACKETS.vendor[1].val |= check_sum;

        for (i = 0; i < PACKET_NUM(vendor); i++) {
		reg = phdmi_sink->regbase + HDMI_PACKETS.vendor[i].reg;
                writel_relaxed(HDMI_PACKETS.vendor[i].val, reg);
        }
}

static void ambhdmi_hdmise_config_audio(struct ambhdmi_sink *phdmi_sink,
		struct ambarella_i2s_interface *i2s_config)
{
	u32 mclk_conf, oversample = i2s_config->mclk / i2s_config->sfreq;
        u32 n, cts, src, layout;
        u8 cc;

	switch (oversample) {
	case 768:
		mclk_conf = 4;
		break;
	case 192:
		mclk_conf = 5;
		break;
	case 64:
		mclk_conf = 6;
		break;
	case 32:
		mclk_conf = 7;
		break;
	default:
		mclk_conf = oversample / 128 - 1;
		break;
	}

        /* n = 128 * fs / 1000, cts = fp / 1000 */
        switch (i2s_config->sfreq) {
        case 8000:
        case 11025:
        case 12000:
        case 16000:
        case 22050:
        case 24000:
                n	= 128 * i2s_config->sfreq / 1000;
                cts	= pixel_clock;
                break;

        case 32000:
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

        case 44100:
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

        case 48000:
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

        case 96000:
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
        switch (i2s_config->channels) {
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

        writel_relaxed(mclk_conf, phdmi_sink->regbase + HDMI_AUNIT_MCLK_OFFSET);
        writel_relaxed(HDMI_AUNIT_N(n), phdmi_sink->regbase + HDMI_AUNIT_N_OFFSET);
#if 0
        writel_relaxed(HDMI_AUNIT_NCTS_CTRL_NCTS_EN | HDMI_AUNIT_NCTS_CTRL_CTS_SEL_SW_MODE,
        	phdmi_sink->regbase + HDMI_AUNIT_NCTS_CTRL_OFFSET);
        writel_relaxed(HDMI_AUNIT_CTS(cts), phdmi_sink->regbase + HDMI_AUNIT_CTS_OFFSET);
#else
        writel_relaxed(HDMI_AUNIT_NCTS_CTRL_NCTS_EN | HDMI_AUNIT_NCTS_CTRL_CTS_SEL_HW_MODE,
	        phdmi_sink->regbase + HDMI_AUNIT_NCTS_CTRL_OFFSET);
        writel_relaxed(HDMI_AUNIT_CTS(cts), phdmi_sink->regbase + HDMI_AUNIT_CTS_OFFSET);
#endif
        writel_relaxed(src, phdmi_sink->regbase + HDMI_AUNIT_SRC_OFFSET);
        writel_relaxed(layout, phdmi_sink->regbase + HDMI_AUNIT_LAYOUT_OFFSET);
        writel_relaxed(i2s_config->mode, phdmi_sink->regbase + HDMI_I2S_MODE_OFFSET);
        writel_relaxed(0, phdmi_sink->regbase + HDMI_I2S_RX_CTRL_OFFSET);
        writel_relaxed(i2s_config->word_len, phdmi_sink->regbase + HDMI_I2S_WLEN_OFFSET);
        writel_relaxed(i2s_config->word_pos, phdmi_sink->regbase + HDMI_I2S_WPOS_OFFSET);
        writel_relaxed(0, phdmi_sink->regbase + HDMI_I2S_SLOT_OFFSET);
        writel_relaxed(3, phdmi_sink->regbase + HDMI_I2S_RX_FIFO_GTH_OFFSET);
        writel_relaxed(HDMI_I2S_RX_CLOCK_RX_I2S_CLK_POL(0), phdmi_sink->regbase + HDMI_I2S_RX_CLOCK_OFFSET);
        writel_relaxed(~HDMI_I2S_GATE_OFF_RX_GATE_OFF_EN, phdmi_sink->regbase + HDMI_I2S_GATE_OFF_OFFSET);

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
        writel_relaxed(~HDMI_OP_MODE_OP_EN, phdmi_sink->regbase + HDMI_OP_MODE_OFFSET);
}

static int ambhdmi_hdmise_audio_transition(struct notifier_block *nb,
                                           unsigned long val, void *data)
{
	struct ambhdmi_sink *phdmi_sink;
        struct ambarella_i2s_interface *i2s_config = data;
        u32 state;
        u32 _val, _hdmise_en;

        if (val != AUDIO_NOTIFY_SETHWPARAMS)
                return 0;

	phdmi_sink = container_of(nb, struct ambhdmi_sink, audio_transition);

        state = readl_relaxed(phdmi_sink->regbase + HDMI_OP_MODE_OFFSET);
        if (!(state & HDMI_OP_MODE_OP_EN))
                return 0;

        /* Enable HDMISE Clock */
        _val = readl_relaxed(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
        _hdmise_en = _val & HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
        if (!_hdmise_en) {
                _val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                writel_relaxed(_val, phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
        }

        mdelay(100);
        if (!(readl_relaxed(phdmi_sink->regbase + HDMI_I2S_INIT_OFFSET) & 0x2)) {
                //writel_relaxed(HDMI_I2S_INIT_DAI_RESET(1), phdmi_sink->regbase + HDMI_I2S_INIT_OFFSET);
                writel_relaxed(HDMI_I2S_INIT_RX_ENABLE, phdmi_sink->regbase + HDMI_I2S_INIT_OFFSET);
                mdelay(100);
        }

        ambhdmi_hdmise_config_audio(phdmi_sink, i2s_config);
        ambhdmi_hdmise_flush_packets(phdmi_sink);
        writel_relaxed(HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN | HDMI_PACKET_TX_CTRL_AUD_RPT |
                    HDMI_PACKET_TX_CTRL_AUD_EN | HDMI_PACKET_TX_CTRL_AVI_RPT |
                    HDMI_PACKET_TX_CTRL_AVI_EN | HDMI_PACKET_TX_CTRL_GEN_RPT |
                    HDMI_PACKET_TX_CTRL_GEN_EN | HDMI_PACKET_TX_CTRL_VS_RPT |
                    HDMI_PACKET_TX_CTRL_VS_EN,
                    phdmi_sink->regbase + HDMI_PACKET_TX_CTRL_OFFSET);

        /* Clock Gating */
        if (!_hdmise_en) {
                _val = readl_relaxed(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
                _val &= ~HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                writel_relaxed(_val, phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
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
        writel_relaxed(HDMI_HDCPCE_CTL_USE_EESS(1) | HDMI_HDCPCE_CTL_HDCPCE_EN,
        	phdmi_sink->regbase + HDMI_EESS_CTL_OFFSET);

        /*PHY*/
        writel_relaxed(0, phdmi_sink->regbase + HDMI_PHY_CTRL_OFFSET);
        if (vt->pixel_clock >= 148352 && vt->pixel_clock <= 148500) {
                writel_relaxed(HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
                            HDMI_PHY_CTRL_NON_RESET_HDMI_PHY |
                            HDMI_PHY_CTRL_10_PRE_EMPHASIS |
                            HDMI_PHY_CTRL_1P2_MA_SINK_CURRENT,
                            phdmi_sink->regbase + HDMI_PHY_CTRL_OFFSET);
        } else {
                writel_relaxed(HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
                            HDMI_PHY_CTRL_NON_RESET_HDMI_PHY,
                            phdmi_sink->regbase + HDMI_PHY_CTRL_OFFSET);
        }

        /*AFIFO*/
        writel_relaxed(HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(12) |
                    HDMI_P2P_AFIFO_LEVEL_UPPER_BOUND(4) |
                    HDMI_P2P_AFIFO_LEVEL_MAX_USAGE_LEVEL(7)	|
                    HDMI_P2P_AFIFO_LEVEL_MIN_USAGE_LEVEL(7) |
                    HDMI_P2P_AFIFO_LEVEL_CURRENT_USAGE_LEVEL(7),
                    phdmi_sink->regbase + HDMI_P2P_AFIFO_LEVEL_OFFSET);
        writel_relaxed(HDMI_P2P_AFIFO_CTRL_EN, phdmi_sink->regbase + HDMI_P2P_AFIFO_CTRL_OFFSET);

        /*Video*/
#if (VOUT_HDMI_REGS_OFFSET_GROUP == 3)
        writel_relaxed(vt->vsync_offset, phdmi_sink->regbase + HDMI_VUNIT_VBLANK_VFRONT_OFFSET);
        writel_relaxed(vt->vsync_width, phdmi_sink->regbase + HDMI_VUNIT_VBLANK_PULSE_WIDTH_OFFSET);
        writel_relaxed(vt->v_blanking - vt->vsync_offset - vt->vsync_width,
		phdmi_sink->regbase + HDMI_VUNIT_VBLANK_VBACK_OFFSET);

        if (ddd_structure == DDD_SIDE_BY_SIDE_FULL) {
                writel_relaxed(vt->hsync_offset * 2, phdmi_sink->regbase + HDMI_VUNIT_HBLANK_HFRONT_OFFSET);
                writel_relaxed(vt->hsync_width * 2, phdmi_sink->regbase + HDMI_VUNIT_HBLANK_PULSE_WIDTH_OFFSET);
                writel_relaxed(vt->h_blanking * 2 - vt->hsync_offset * 2 - vt->hsync_width * 2,
			phdmi_sink->regbase + HDMI_VUNIT_HBLANK_HBACK_OFFSET);
        } else {
                writel_relaxed(vt->hsync_offset, phdmi_sink->regbase + HDMI_VUNIT_HBLANK_HFRONT_OFFSET);
                writel_relaxed(vt->hsync_width, phdmi_sink->regbase + HDMI_VUNIT_HBLANK_PULSE_WIDTH_OFFSET);
                writel_relaxed(vt->h_blanking - vt->hsync_offset - vt->hsync_width,
			phdmi_sink->regbase + HDMI_VUNIT_HBLANK_HBACK_OFFSET);
        }
#else
        writel_relaxed(HDMI_VUNIT_VBLANK_LEFT_OFFSET(vt->vsync_offset) |
                    HDMI_VUNIT_VBLANK_PULSE_WIDTH(vt->vsync_width) |
                    HDMI_VUNIT_VBLANK_RIGHT_OFFSET(vt->v_blanking - vt->vsync_offset - vt->vsync_width),
                    phdmi_sink->regbase + HDMI_VUNIT_VBLANK_OFFSET);

        writel_relaxed(HDMI_VUNIT_HBLANK_LEFT_OFFSET(vt->hsync_offset) |
                    HDMI_VUNIT_HBLANK_PULSE_WIDTH(vt->hsync_width) |
                    HDMI_VUNIT_HBLANK_RIGHT_OFFSET(vt->h_blanking - vt->hsync_offset - vt->hsync_width),
                    phdmi_sink->regbase + HDMI_VUNIT_HBLANK_OFFSET);
#endif

        if (ddd_structure == DDD_FRAME_PACKING) {
                writel_relaxed(vt->v_active * 2 + vt->v_blanking,
			phdmi_sink->regbase + HDMI_VUNIT_VACTIVE_OFFSET);
        } else {
                writel_relaxed(vt->v_active, phdmi_sink->regbase + HDMI_VUNIT_VACTIVE_OFFSET);
        }
        if (ddd_structure == DDD_SIDE_BY_SIDE_FULL) {
                writel_relaxed(vt->h_active * 2, phdmi_sink->regbase + HDMI_VUNIT_HACTIVE_OFFSET);
        } else {
                writel_relaxed(vt->h_active, phdmi_sink->regbase + HDMI_VUNIT_HACTIVE_OFFSET);
        }
        writel_relaxed(HDMI_VUNIT_CTRL_VIDEO_MODE(vt->interlace) |
                    HDMI_VUNIT_CTRL_HSYNC_POL(~vt->hsync_polarity) |
                    HDMI_VUNIT_CTRL_VSYNC_POL(~vt->vsync_polarity),
                    phdmi_sink->regbase + HDMI_VUNIT_CTRL_OFFSET);
        writel_relaxed(HDMI_VUNIT_VSYNC_DETECT_EN, phdmi_sink->regbase + HDMI_VUNIT_VSYNC_DETECT_OFFSET);

        /*Audio*/
        pixel_clock = vt->pixel_clock;
        i2s_config = get_audio_i2s_interface();
        ambhdmi_hdmise_config_audio(phdmi_sink, &i2s_config);

        cts = vt->pixel_clock;
        writel_relaxed(HDMI_AUNIT_CTS(cts), phdmi_sink->regbase + HDMI_AUNIT_CTS_OFFSET);

        if (phdmi_sink->edid.interface != HDMI)
                writel_relaxed(HDMI_I2S_GATE_OFF_RX_GATE_OFF_EN,
                	phdmi_sink->regbase + HDMI_I2S_GATE_OFF_OFFSET);

        /*Misc Packet*/
        writel_relaxed(HDMI_PACKET_GENERAL_CTRL_CLR_AVMUTE_EN,
        	phdmi_sink->regbase + HDMI_PACKET_GENERAL_CTRL_OFFSET);
        writel_relaxed(0, phdmi_sink->regbase + HDMI_PACKET_MISC_OFFSET);

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

        ambhdmi_hdmise_flush_packets(phdmi_sink);

        if (phdmi_sink->edid.interface == HDMI) {
                writel_relaxed(HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN |
                            HDMI_PACKET_TX_CTRL_AUD_RPT |
                            HDMI_PACKET_TX_CTRL_AUD_EN |
                            HDMI_PACKET_TX_CTRL_AVI_RPT |
                            HDMI_PACKET_TX_CTRL_AVI_EN |
                            HDMI_PACKET_TX_CTRL_GEN_RPT |
                            HDMI_PACKET_TX_CTRL_GEN_EN |
                            HDMI_PACKET_TX_CTRL_VS_RPT |
                            HDMI_PACKET_TX_CTRL_VS_EN,
                            phdmi_sink->regbase + HDMI_PACKET_TX_CTRL_OFFSET);
        } else {
                writel_relaxed(HDMI_PACKET_TX_CTRL_BUF_SWITCH_EN |
                            HDMI_PACKET_TX_CTRL_AVI_RPT |
                            HDMI_PACKET_TX_CTRL_AVI_EN |
                            HDMI_PACKET_TX_CTRL_GEN_RPT |
                            HDMI_PACKET_TX_CTRL_GEN_EN |
                            HDMI_PACKET_TX_CTRL_VS_RPT |
                            HDMI_PACKET_TX_CTRL_VS_EN,
                            phdmi_sink->regbase + HDMI_PACKET_TX_CTRL_OFFSET);
        }

        /*Test*/
        writel_relaxed(0, phdmi_sink->regbase + HDMI_HDMISE_TM_OFFSET);
}

static void ambhdmi_hdmise_run(struct ambhdmi_sink *phdmi_sink)
{
        u32			mode;

        vout_notice("%s: %d @ 0x%p!\n", __func__,
                    phdmi_sink->edid.interface, phdmi_sink->regbase);

        if (phdmi_sink->edid.interface == HDMI) {
                mode = HDMI_OP_MODE_OP_MODE_HDMI | HDMI_OP_MODE_OP_EN;
        } else {
                mode = HDMI_OP_MODE_OP_MODE_DVI | HDMI_OP_MODE_OP_EN;
        }

        writel_relaxed(mode, phdmi_sink->regbase + HDMI_OP_MODE_OFFSET);
}

static void ambhdmi_hdmise_init(struct ambhdmi_sink *phdmi_sink,
                                int cs, enum ddd_structure ddd_structure,
                                enum amba_vout_hdmi_overscan overscan,
                                const amba_hdmi_video_timing_t *vt)
{
        u32 val;

        /* Soft Reset HDMISE */
        writel_relaxed(HDMI_HDMISE_SOFT_RESET, phdmi_sink->regbase + HDMI_HDMISE_SOFT_RESET_OFFSET);
        writel_relaxed(~HDMI_HDMISE_SOFT_RESET, phdmi_sink->regbase + HDMI_HDMISE_SOFT_RESET_OFFSET);

        /* Reset CEC */
        writel_relaxed(0x1 << 31, phdmi_sink->regbase + CEC_CTRL_OFFSET);

        /* Clock Gating */
        writel_relaxed(0, phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);

        /* Enable Hotplug detect and loss interrupt */
        writel_relaxed(HDMI_INT_ENABLE_PHY_RX_SENSE_REMOVE_EN |
                    HDMI_INT_ENABLE_PHY_RX_SENSE_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_LOSS_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_DETECT_EN,
                    phdmi_sink->regbase + HDMI_INT_ENABLE_OFFSET  );

        msleep(10);

        disable_irq(phdmi_sink->irq);

        /* Enable HDMISE Clock */
        val = readl_relaxed(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
        val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
        writel_relaxed(val, phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);

        ambhdmi_hdmise_stop(phdmi_sink);
        ambhdmi_hdmise_set(phdmi_sink, cs, ddd_structure, overscan, vt);
        ambhdmi_hdmise_run(phdmi_sink);

        /* Clock Gating */
        if (phdmi_sink->video_sink.hdmi_plug == AMBA_VOUT_SINK_REMOVED) {
                val = readl_relaxed(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
                val &= ~HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                writel_relaxed(val, phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET);
        }

        enable_irq(phdmi_sink->irq);
}

