/*
 * sound/soc/ambarella_i2s.c
 *
 * History:
 *	2008/03/03 - [Eric Lee] created file
 *	2008/04/16 - [Eric Lee] Removed the compiling warning
 *	2009/01/22 - [Anthony Ginger] Port to 2.6.28
 *	2009/03/05 - [Cao Rongrong] Update from 2.6.22.10
 *	2009/06/10 - [Cao Rongrong] Port to 2.6.29
 *	2009/06/29 - [Cao Rongrong] Support more mclk and fs
 *	2010/10/25 - [Cao Rongrong] Port to 2.6.36+
 *	2011/03/20 - [Cao Rongrong] Port to 2.6.38
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <plat/dma.h>
#include <plat/audio.h>
#include "ambarella_pcm.h"
#include "ambarella_i2s.h"

static unsigned int bclk_reverse = 0;
module_param(bclk_reverse, uint, 0644);
MODULE_PARM_DESC(bclk_reverse, "bclk_reverse.");

static DEFINE_MUTEX(clock_reg_mutex);
static int enable_ext_i2s = 1;

static u32 DAI_Clock_Divide_Table[MAX_OVERSAMPLE_IDX_NUM][2] = {
	{ 1, 0 }, // 128xfs
	{ 3, 1 }, // 256xfs
	{ 5, 2 }, // 384xfs
	{ 7, 3 }, // 512xfs
	{ 11, 5 }, // 768xfs
	{ 15, 7 }, // 1024xfs
	{ 17, 8 }, // 1152xfs
	{ 23, 11 }, // 1536xfs
	{ 35, 17 } // 2304xfs
};

static inline void dai_tx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_TX_EN);
}

static inline void dai_rx_enable(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_RX_EN);
}

static inline void dai_tx_disable(void)
{
	amba_clrbitsl(I2S_INIT_REG, DAI_TX_EN);
}

static inline void dai_rx_disable(void)
{
	amba_clrbitsl(I2S_INIT_REG, DAI_RX_EN);
}

static inline void dai_tx_fifo_rst(void)
{
	amba_setbitsl(I2S_INIT_REG, I2S_TX_FIFO_RESET_BIT);
}

static inline void dai_rx_fifo_rst(void)
{
	amba_setbitsl(I2S_INIT_REG, I2S_RX_FIFO_RESET_BIT);
}

static inline void dai_fifo_rst(void)
{
	amba_setbitsl(I2S_INIT_REG, DAI_FIFO_RST);
}

static int ambarella_i2s_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		dai_rx_disable();
		dai_rx_fifo_rst();
	} else {
		dai_tx_disable();
		dai_tx_fifo_rst();
	}
	return 0;
}

static int ambarella_i2s_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret = 0;

	/* Add a rule to enforce the DMA buffer align. */
	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 32);
	if (ret)
		goto ambarella_i2s_startup_exit;

	ret = snd_pcm_hw_constraint_step(runtime, 0,
		SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 32);
	if (ret)
		goto ambarella_i2s_startup_exit;

	ret = snd_pcm_hw_constraint_integer(runtime,
		SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0)
		goto ambarella_i2s_startup_exit;

	return 0;
ambarella_i2s_startup_exit:
	return ret;
}

static int ambarella_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *cpu_dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);
	u8 slots, word_len, word_pos, oversample, double_rate;
	u32 clock_divider, clock_reg, channels, tx_ctrl = 0, multi24 = 0;

	/* Disable tx/rx before initializing */
	dai_tx_disable();
	dai_rx_disable();

	channels = params_channels(params);
	/* Set channels */
	switch (channels) {
	case 2:
		amba_writel(I2S_REG(I2S_CHANNEL_SELECT_OFFSET), I2S_2CHANNELS_ENB);
		break;
	case 4:
		amba_writel(I2S_REG(I2S_CHANNEL_SELECT_OFFSET), I2S_4CHANNELS_ENB);
		break;
	case 6:
		amba_writel(I2S_REG(I2S_CHANNEL_SELECT_OFFSET), I2S_6CHANNELS_ENB);
		break;
	}
	priv_data->amb_i2s_intf.ch = channels;

	/* Set format */
	double_rate = 0;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		priv_data->amb_i2s_intf.word_len = DAI_16bits;
		word_len = 0x0f;
		tx_ctrl |= 0x08; /* set unison bit (LR both in TX_LEFT_DATA) */
		if (priv_data->amb_i2s_intf.mode == DAI_DSP_Mode) {
			slots = channels - 1;
			word_pos = 0x0f;
			priv_data->amb_i2s_intf.slots = slots;
		} else {
			slots = 0;
			word_pos = 0;
			priv_data->amb_i2s_intf.slots = DAI_32slots;
		}
		priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		double_rate = 1;
		priv_data->amb_i2s_intf.word_len = DAI_24bits;
		word_len = 0x17;
		multi24 = 0x1; /* multi_24_en bit */
		if (priv_data->amb_i2s_intf.mode == DAI_DSP_Mode) {
			slots = channels - 1;
			word_pos = 0x00; /* ignored, but set it to something */
			priv_data->amb_i2s_intf.slots = slots;
		} else {
			slots = 0;
			word_pos = 0; /* ignored, but set it to something */
			priv_data->amb_i2s_intf.slots = DAI_32slots;
		}
		priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		break;
	default:
		goto hw_params_exit;
	}

	priv_data->amb_i2s_intf.word_pos = word_pos;
	priv_data->amb_i2s_intf.word_order = DAI_MSB_FIRST;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (priv_data->amb_i2s_intf.ms_mode != DAI_SLAVE)
			tx_ctrl |= 0x20;
		amba_writel(I2S_TX_CTRL_REG, tx_ctrl);
		amba_writel(I2S_TX_FIFO_LTH_REG, 0x10);
	} else {
		if (priv_data->amb_i2s_intf.ms_mode == DAI_SLAVE)
			amba_writel(I2S_RX_CTRL_REG, 0x00);
		else
			amba_writel(I2S_RX_CTRL_REG, 0x02);
		amba_writel(I2S_RX_FIFO_GTH_REG, 0x20);
	}

	amba_writel(I2S_MODE_REG, priv_data->amb_i2s_intf.mode);
	amba_writel(I2S_WLEN_REG, word_len);
	amba_writel(I2S_WPOS_REG, word_pos);
	amba_writel(I2S_SLOT_REG, slots);
	amba_writel(I2S_24BITMUX_MODE_REG, multi24);

	switch (params_rate(params)) {
	case 8000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_8000;
		break;
	case 11025:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_11025;
		break;
	case 16000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_16000;
		break;
	case 22050:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_22050;
		break;
	case 32000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_32000;
		break;
	case 44100:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_44100;
		break;
	case 48000:
		priv_data->amb_i2s_intf.sfreq = AUDIO_SF_48000;
		break;
	default:
		goto hw_params_exit;
	}

	/* Set clock */
	clk_set_rate(priv_data->mclk, priv_data->amb_i2s_intf.mclk);

	mutex_lock(&clock_reg_mutex);
	clock_reg = amba_readl(I2S_CLOCK_REG);
	oversample = priv_data->amb_i2s_intf.oversample;

	/* In 16 bit mode, the channel width is 16 bits, and in 24 bit
	 * mode then channel width is 32 bits (see Ambarella S2L Hardware
         * Programming Manual, Table 9-1); consequently we need to adjust
	 * the clock divider accordingly, by referencing 'double_rate' here.
	 */

	clock_divider = DAI_Clock_Divide_Table[oversample][double_rate];

	clock_reg &= ~DAI_CLOCK_MASK;
	clock_reg |= clock_divider;
	if (priv_data->amb_i2s_intf.ms_mode == DAI_MASTER)
		clock_reg |= I2S_CLK_MASTER_MODE;
	else
		clock_reg &= (~I2S_CLK_MASTER_MODE);
	/* Disable output BCLK and LRCLK to disable external codec */
	if (enable_ext_i2s == 0)
		clock_reg &= ~(I2S_CLK_WS_OUT_EN | I2S_CLK_BCLK_OUT_EN);

	if (bclk_reverse)
		clock_reg &= ~(1<< 6);
	else
		clock_reg |= (1<< 6);

	amba_writel(I2S_CLOCK_REG, clock_reg);
	mutex_unlock(&clock_reg_mutex);

	dai_rx_enable();
	dai_tx_enable();

	msleep(1);

	/* Notify HDMI that the audio interface is changed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_SETHWPARAMS);
	return 0;

hw_params_exit:
	dai_rx_enable();
	dai_tx_enable();
	return -EINVAL;
}

static int ambarella_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
		struct snd_soc_dai *cpu_dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			dai_tx_enable();
		else
			dai_rx_enable();
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
			dai_tx_disable();
			dai_tx_fifo_rst();
		}else{
			dai_rx_disable();
			dai_rx_fifo_rst();
		}
		break;
	default:
		break;
	}

	return 0;
}

/*
 * Set Ambarella I2S DAI format
 */
static int ambarella_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_LEFT_J:
		priv_data->amb_i2s_intf.mode = DAI_leftJustified_Mode;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		priv_data->amb_i2s_intf.mode = DAI_rightJustified_Mode;
		break;
	case SND_SOC_DAIFMT_I2S:
		priv_data->amb_i2s_intf.mode = DAI_I2S_Mode;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		priv_data->amb_i2s_intf.mode = DAI_DSP_Mode;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		priv_data->amb_i2s_intf.ms_mode = DAI_MASTER;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		if (priv_data->amb_i2s_intf.mode != DAI_I2S_Mode) {
			printk("DAI can't work in slave mode without standard I2S format!\n");
			return -EINVAL;
		}
		priv_data->amb_i2s_intf.ms_mode = DAI_SLAVE;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
		int clk_id, unsigned int freq, int dir)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (clk_id) {
	case AMBARELLA_CLKSRC_ONCHIP:
		priv_data->amb_i2s_intf.clksrc = clk_id;
		priv_data->amb_i2s_intf.mclk = freq;
		break;

	case AMBARELLA_CLKSRC_EXTERNAL:
	default:
		printk("CLK SOURCE (%d) is not supported yet\n", clk_id);
		return -EINVAL;
	}

	return 0;
}

static int ambarella_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
		int div_id, int div)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(cpu_dai);

	switch (div_id) {
	case AMBARELLA_CLKDIV_LRCLK:
		priv_data->amb_i2s_intf.oversample = div;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int external_i2s_get_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	mutex_lock(&clock_reg_mutex);
	ucontrol->value.integer.value[0] = enable_ext_i2s;
	mutex_unlock(&clock_reg_mutex);

	return 0;
}

static int external_i2s_set_status(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	u32 clock_reg;

	mutex_lock(&clock_reg_mutex);
	clock_reg = amba_readl(I2S_CLOCK_REG);

	enable_ext_i2s = ucontrol->value.integer.value[0];
	if (enable_ext_i2s)
		clock_reg |= (I2S_CLK_WS_OUT_EN | I2S_CLK_BCLK_OUT_EN);
	else
		clock_reg &= ~(I2S_CLK_WS_OUT_EN | I2S_CLK_BCLK_OUT_EN);

	amba_writel(I2S_CLOCK_REG, clock_reg);
	mutex_unlock(&clock_reg_mutex);

	return 1;
}

static const char *i2s_status_str[] = {"Off", "On"};

static const struct soc_enum external_i2s_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(i2s_status_str), i2s_status_str),
};

static const struct snd_kcontrol_new external_i2s_controls[] = {
	SOC_ENUM_EXT("External I2S Switch", external_i2s_enum[0],
			external_i2s_get_status, external_i2s_set_status),
};

int ambarella_i2s_add_controls(struct snd_soc_codec *codec)
{
	return snd_soc_add_codec_controls(codec, external_i2s_controls,
			ARRAY_SIZE(external_i2s_controls));
}
EXPORT_SYMBOL_GPL(ambarella_i2s_add_controls);


#ifdef CONFIG_PM
static int ambarella_i2s_dai_suspend(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	priv_data->clock_reg = amba_readl(I2S_CLOCK_REG);

	return 0;
}



static int ambarella_i2s_dai_resume(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	amba_writel(I2S_CLOCK_REG, priv_data->clock_reg);

	return 0;
}
#else /* CONFIG_PM */
#define ambarella_i2s_dai_suspend	NULL
#define ambarella_i2s_dai_resume	NULL
#endif /* CONFIG_PM */



static int ambarella_i2s_dai_probe(struct snd_soc_dai *dai)
{
	u32 sfreq, clock_divider;
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &priv_data->capture_dma_data;
	dai->playback_dma_data = &priv_data->playback_dma_data;

	if (priv_data->default_mclk == 12288000) {
		sfreq = AUDIO_SF_48000;
	} else if (priv_data->default_mclk == 11289600){
		sfreq = AUDIO_SF_44100;
	} else {
		dev_warn(dai->dev, "Please sepcify the default mclk\n");
		priv_data->default_mclk = 12288000;
		sfreq = AUDIO_SF_48000;
	}

	clk_set_rate(priv_data->mclk, priv_data->default_mclk);

	/* Dai default smapling rate, polarity configuration.
	 * Note: Just be configured, actually BCLK and LRCLK will not
	 * output to outside at this time. */
	clock_divider = DAI_Clock_Divide_Table[AudioCodec_256xfs][DAI_32slots >> 6];
	amba_writel(I2S_CLOCK_REG, clock_divider | I2S_CLK_TX_PO_FALL);

	priv_data->amb_i2s_intf.mode = DAI_I2S_Mode;
	priv_data->amb_i2s_intf.clksrc = AMBARELLA_CLKSRC_ONCHIP;
	priv_data->amb_i2s_intf.ms_mode = DAI_MASTER;
	priv_data->amb_i2s_intf.mclk = priv_data->default_mclk;
	priv_data->amb_i2s_intf.oversample = AudioCodec_256xfs;
	priv_data->amb_i2s_intf.word_order = DAI_MSB_FIRST;
	priv_data->amb_i2s_intf.sfreq = sfreq;
	priv_data->amb_i2s_intf.word_len = DAI_16bits;
	priv_data->amb_i2s_intf.word_pos = 0;
	priv_data->amb_i2s_intf.slots = DAI_32slots;
	priv_data->amb_i2s_intf.ch = 2;

	/* reset fifo */
	dai_tx_enable();
	dai_rx_enable();
	dai_fifo_rst();

	/* Notify HDMI that the audio interface is initialized */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf, AUDIO_NOTIFY_INIT);

	return 0;
}

static int ambarella_i2s_dai_remove(struct snd_soc_dai *dai)
{
	struct amb_i2s_priv *priv_data = snd_soc_dai_get_drvdata(dai);

	/* Disable I2S clock output */
	amba_writel(I2S_CLOCK_REG, 0x0);

	/* Notify that the audio interface is removed */
	ambarella_audio_notify_transition(&priv_data->amb_i2s_intf,
		AUDIO_NOTIFY_REMOVE);

	return 0;
}

static const struct snd_soc_dai_ops ambarella_i2s_dai_ops = {
	.prepare = ambarella_i2s_prepare,
	.startup = ambarella_i2s_startup,
	.hw_params = ambarella_i2s_hw_params,
	.trigger = ambarella_i2s_trigger,
	.set_fmt = ambarella_i2s_set_fmt,
	.set_sysclk = ambarella_i2s_set_sysclk,
	.set_clkdiv = ambarella_i2s_set_clkdiv,
};

static struct snd_soc_dai_driver ambarella_i2s_dai = {
	.probe = ambarella_i2s_dai_probe,
	.remove = ambarella_i2s_dai_remove,
	.suspend = ambarella_i2s_dai_suspend,
	.resume = ambarella_i2s_dai_resume,
	.playback = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE),
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 0, // initialized in ambarella_i2s_probe function
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE),
	},
	.ops = &ambarella_i2s_dai_ops,
	.symmetric_rates = 1,
};

static const struct snd_soc_component_driver ambarella_i2s_component = {
	.name		= "ambarella-i2s",
};

static int ambarella_i2s_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct amb_i2s_priv *priv_data;
	struct pinctrl *pinctrl;
	int channels, rval;

	priv_data = devm_kzalloc(&pdev->dev, sizeof(*priv_data), GFP_KERNEL);
	if (priv_data == NULL)
		return -ENOMEM;

	priv_data->playback_dma_data.addr = I2S_TX_LEFT_DATA_DMA_REG;
	priv_data->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	priv_data->playback_dma_data.maxburst = 32;
	priv_data->playback_dma_data.filter_data = (void *)I2S_TX_DMA_CHAN;

	priv_data->capture_dma_data.addr = I2S_RX_DATA_DMA_REG;
	priv_data->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	priv_data->capture_dma_data.maxburst = 32;
	priv_data->capture_dma_data.filter_data = (void *)I2S_RX_DMA_CHAN;
	priv_data->mclk = clk_get(&pdev->dev, "gclk_audio");
	if (IS_ERR(priv_data->mclk)) {
		dev_err(&pdev->dev, "Get audio clk failed!\n");
		return PTR_ERR(priv_data->mclk);
	}

	dev_set_drvdata(&pdev->dev, priv_data);

	rval = of_property_read_u32(np, "amb,i2s-channels", &channels);
	if (rval < 0) {
		dev_err(&pdev->dev, "Get channels failed! %d\n", rval);
		return -ENXIO;
	}

	of_property_read_u32(np, "amb,default-mclk", &priv_data->default_mclk);

	ambarella_i2s_dai.playback.channels_max = channels;
	ambarella_i2s_dai.capture.channels_max = channels;

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl))
		return PTR_ERR(pinctrl);



	rval = snd_soc_register_component(&pdev->dev,
			&ambarella_i2s_component,  &ambarella_i2s_dai, 1);
	if (rval < 0){
		dev_err(&pdev->dev, "register DAI failed\n");
		return rval;
	}

	rval = ambarella_pcm_platform_register(&pdev->dev);
	if (rval) {
		dev_err(&pdev->dev, "register PCM failed: %d\n", rval);
		snd_soc_unregister_component(&pdev->dev);
		return rval;
	}

	return 0;
}

static int ambarella_i2s_remove(struct platform_device *pdev)
{
	ambarella_pcm_platform_unregister(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);

	return 0;
}

static const struct of_device_id ambarella_i2s_dt_ids[] = {
	{ .compatible = "ambarella,i2s", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ambarella_i2s_dt_ids);

static struct platform_driver ambarella_i2s_driver = {
	.probe = ambarella_i2s_probe,
	.remove = ambarella_i2s_remove,

	.driver = {
		.name = "ambarella-i2s",
		.owner = THIS_MODULE,
		.of_match_table = ambarella_i2s_dt_ids,
	},
};

module_platform_driver(ambarella_i2s_driver);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella Soc I2S Interface");

MODULE_LICENSE("GPL");

