/*
 * uac_stream.c - ALSA audio utilities for Gadget stack
 *
 * Copyright (C) 2015 Jorney <qtu@ambarella.com>
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/random.h>
#include <linux/syscalls.h>

#include "u_uac1.h"

/*
 * This component encapsulates the ALSA devices for USB audio gadget
 */

static char *fn_play = FILE_PCM_PLAYBACK;
static char *fn_cap  = FILE_PCM_CAPTURE;
static char *fn_cntl = FILE_CONTROL;

/*-------------------------------------------------------------------------*/
#if AUDIO_DEBUG_PLAY
int audio_get_frame(struct f_audio *audio)
{
	struct file *file = audio->play_file;
	char *buf = audio->debug_frame;
	int size, ret;
	mm_segment_t fs;
	loff_t pos;

	size = vfs_llseek(file, 0 ,SEEK_END);

	if(size > AUDIO_DEBUG_PLAY_SIZE)
		size = AUDIO_DEBUG_PLAY_SIZE;

	audio->debug_avail = size;

	pos = 0;
	fs  = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_read(file, buf, size, &pos);
	set_fs(fs);
	return 0;
}

int audio_close_play(struct f_audio *audio)
{
	if(audio->play_file > 0)
		filp_close(audio->play_file, NULL);

	return 0;
}

int audio_open_play(struct f_audio *audio)
{
	audio->play_file = filp_open(AUDIO_DEBUG_PLAY_FILE, O_RDWR, 0);
	if(IS_ERR(audio->play_file)){
		printk("%s unable open video device\n", __FUNCTION__);
		return PTR_ERR(audio->play_file);
	}
	return 0;
}

static int audio_play_debug(struct f_audio *audio)
{
	if (audio_open_play(audio))
		return 0;

	audio_get_frame(audio);
	audio_close_play(audio);

	return 0;
}


#endif

#if AUDIO_DEBUG_CAPTURE

struct file *file = NULL;
static int offset = 0;

int audio_open_capture(void)
{
	file = filp_open(AUDIO_DEBUG_CAPTURE_FILE, O_RDWR, 0);
	if(IS_ERR(file)){
		printk("%s unable open video device\n", __FUNCTION__);
		return PTR_ERR(file);
	}
	return 0;
}

int audio_close_capture(void)
{
	if(file > 0)
		filp_close(file, NULL);
	file = NULL;

	return 0;
}

int audio_record(char *buf, int size, loff_t pos)
{
	mm_segment_t fs;
	int ret;

	fs  = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_write(file, buf, size, &pos);

	set_fs(fs);

	return 0;
}

#endif

int stereo_to_mono(struct f_audio *audio, char *buf, int size)
{
	int unused ;

	int *s = (int *)buf;
	short int *d;
	int i;


	if(size != FRAME_SIZE * 2)
		BUG();

	unused = (audio->current_used == 0) ? 1 : 0;

	if (audio->current_used == -1) {
		audio->current_used = 0;

		d = (short int *)audio->frame[0].buf;

		for (i = 0; i < FRAME_SIZE / 2 ; i++)
			d[i] = s[i];

		audio->frame[0].total_size = size / 2;
		audio->frame[0].bused_size = 0;
		audio->frame[0].nodata = 0;

		return 0;
	}

	d = (short int *)audio->frame[unused].buf;

	for (i = 0; i < FRAME_SIZE / 2 ; i++)
		d[i] = s[i];

#if AUDIO_DEBUG_CAPTURE
	if (offset < AUDIO_DEBUG_CAPTURE_SIZE){
		audio_record((char *)d, size / 2, offset);
		offset += size;
	}
#endif

	audio->frame[unused].total_size = size / 2;
	audio->frame[unused].bused_size = 0;
	audio->frame[unused].nodata = 0;

	return 0;

}

int audio_kthread_get_frame(void *arg)
{
	struct f_audio *audio = (struct f_audio *)arg;
	struct gaudio *card = &audio->card;

	struct gaudio_snd_dev	*snd = &card->capture;
	struct snd_pcm_substream *substream = snd->substream;
	struct snd_pcm_runtime *runtime = substream->runtime;
	mm_segment_t old_fs;
	ssize_t result;
	snd_pcm_sframes_t frames;

	char *buffer;
	int count = FRAME_SIZE * 2;

	int ret = 0;

#if AUDIO_DEBUG_CAPTURE
	if (audio_open_capture())
		return 0;
#endif

#if AUDIO_DEBUG_PLAY
	audio_play_debug(audio);
#endif

	buffer = kzalloc(FRAME_SIZE * 2, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	while(1){

#if AUDIO_DEBUG_CAPTURE
		if (!audio->ep_enabled)
			offset = 0;
#endif

		memset(buffer, 0, FRAME_SIZE * 2);
retry:
		if (runtime->status->state == SNDRV_PCM_STATE_XRUN ||
				runtime->status->state == SNDRV_PCM_STATE_SUSPENDED) {
			result = snd_pcm_kernel_ioctl(substream,
					SNDRV_PCM_IOCTL_PREPARE, NULL);
			if (result < 0) {
				printk("Preparing sound card failed: %d\n",
						(int)result);
				ret = result;
				goto out;
			}
		}

		frames = bytes_to_frames(runtime, count);

		old_fs = get_fs();
		set_fs(KERNEL_DS);

		result = snd_pcm_lib_read(snd->substream, buffer, frames);
		if (result != frames) {
			printk("Capture error: %d\n", (int)result);
			set_fs(old_fs);
			goto retry;
		}

		set_fs(old_fs);

		/* FIXME: only support MONO */
		stereo_to_mono(audio, buffer, count);
	}

out:

#if AUDIO_DEBUG_CAPTURE
	audio_close_capture();
#endif
	kfree(buffer);
	return ret;

}
EXPORT_SYMBOL(audio_kthread_get_frame);

/**
 * Some ALSA internal helper functions
 */
static int snd_interval_refine_set(struct snd_interval *i, unsigned int val)
{
	struct snd_interval t;
	t.empty = 0;
	t.min = t.max = val;
	t.openmin = t.openmax = 0;
	t.integer = 1;
	return snd_interval_refine(i, &t);
}

static int _snd_pcm_hw_param_set(struct snd_pcm_hw_params *params,
				 snd_pcm_hw_param_t var, unsigned int val,
				 int dir)
{
	int changed;
	if (hw_is_mask(var)) {
		struct snd_mask *m = hw_param_mask(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_mask_none(m);
		} else {
			if (dir > 0)
				val++;
			else if (dir < 0)
				val--;
			changed = snd_mask_refine_set(
					hw_param_mask(params, var), val);
		}
	} else if (hw_is_interval(var)) {
		struct snd_interval *i = hw_param_interval(params, var);
		if (val == 0 && dir < 0) {
			changed = -EINVAL;
			snd_interval_none(i);
		} else if (dir == 0)
			changed = snd_interval_refine_set(i, val);
		else {
			struct snd_interval t;
			t.openmin = 1;
			t.openmax = 1;
			t.empty = 0;
			t.integer = 0;
			if (dir < 0) {
				t.min = val - 1;
				t.max = val;
			} else {
				t.min = val;
				t.max = val+1;
			}
			changed = snd_interval_refine(i, &t);
		}
	} else
		return -EINVAL;
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	return changed;
}
/*-------------------------------------------------------------------------*/

/**
 * Set default hardware params
 */
static int playback_default_hw_params(struct gaudio_snd_dev *snd, int channels)
{
	struct snd_pcm_substream *substream = snd->substream;
	struct snd_pcm_hw_params *params;
	snd_pcm_sframes_t result;

       /*
	* SNDRV_PCM_ACCESS_RW_INTERLEAVED,
	* SNDRV_PCM_FORMAT_S16_LE
	* CHANNELS: 2
	* RATE: 48000
	*/
	snd->access = SNDRV_PCM_ACCESS_RW_INTERLEAVED;
	snd->format = SNDRV_PCM_FORMAT_S16_LE;
	snd->channels = channels;
	snd->rate = 48000;

	params = kzalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	_snd_pcm_hw_params_any(params);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_ACCESS,
			snd->access, 0);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_FORMAT,
			snd->format, 0);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_CHANNELS,
			snd->channels, 0);
	_snd_pcm_hw_param_set(params, SNDRV_PCM_HW_PARAM_RATE,
			snd->rate, 0);

	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_DROP, NULL);
	snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_HW_PARAMS, params);

	result = snd_pcm_kernel_ioctl(substream, SNDRV_PCM_IOCTL_PREPARE, NULL);
	if (result < 0) {
		printk("Preparing sound card failed: %d\n", (int)result);
		kfree(params);
		return result;
	}

	/* Store the hardware parameters */
	snd->access = params_access(params);
	snd->format = params_format(params);
	snd->channels = params_channels(params);
	snd->rate = params_rate(params);

	kfree(params);

	printk("Hardware params: access %x, format %x, channels %d, rate %d\n",
		snd->access, snd->format, snd->channels, snd->rate);

	return 0;
}


/**
 * Open ALSA PCM and control device files
 * Initial the PCM or control device
 */
static int gaudio_open_snd_dev(struct gaudio *card)
{
	struct snd_pcm_file *pcm_file;
	struct gaudio_snd_dev *snd;

	if (!card)
		return -ENODEV;


	/* Open control device */
	snd = &card->control;
	snd->filp = filp_open(fn_cntl, O_RDWR, 0);
	if (IS_ERR(snd->filp)) {
		int ret = PTR_ERR(snd->filp);
		printk("unable to open sound control device file: %s\n",
				fn_cntl);
		snd->filp = NULL;
		return ret;
	}
	snd->card = card;

	/* Open PCM playback device and setup substream */
	snd = &card->playback;
	snd->filp = filp_open(fn_play, O_WRONLY, 0);
	if (IS_ERR(snd->filp)) {
		int ret = PTR_ERR(snd->filp);

		printk( "No such PCM playback device: %s\n", fn_play);
		snd->filp = NULL;
		return ret;
	}
	pcm_file = snd->filp->private_data;
	snd->substream = pcm_file->substream;
	snd->card = card;

	playback_default_hw_params(snd, 2);

	/* Open PCM capture device and setup substream */
	snd = &card->capture;
	snd->filp = filp_open(fn_cap, O_RDONLY, 0);
	if (IS_ERR(snd->filp)) {
		printk("No such PCM capture device: %s\n", fn_cap);
		snd->substream = NULL;
		snd->card = NULL;
		snd->filp = NULL;
	} else {
		int ret = 0;
		pcm_file = snd->filp->private_data;
		snd->substream = pcm_file->substream;
		snd->card = card;

		ret =playback_default_hw_params(snd, 2);
		if(ret)
			return ret;
	}

	return 0;
}

/**
 * Close ALSA PCM and control device files
 */
static int gaudio_close_snd_dev(struct gaudio *gau)
{
	struct gaudio_snd_dev	*snd;

	/* Close control device */
	snd = &gau->control;
	if (snd->filp)
		filp_close(snd->filp, NULL);

	/* Close PCM playback device and setup substream */
	snd = &gau->playback;
	if (snd->filp)
		filp_close(snd->filp, NULL);

	/* Close PCM capture device and setup substream */
	snd = &gau->capture;
	if (snd->filp)
		filp_close(snd->filp, NULL);

	return 0;
}

/**
 * gaudio_setup - setup ALSA interface and preparing for USB transfer
 *
 * This sets up PCM, mixer or MIDI ALSA devices fore USB gadget using.
 *
 * Returns negative errno, or zero on success
 */
int  gaudio_setup(struct gaudio *card)
{
	int	ret = 0;

	ret = gaudio_open_snd_dev(card);
	if (ret)
		printk("we need at least one control device\n");

	return ret;

}

/**
 * gaudio_cleanup - remove ALSA device interface
 *
 * This is called to free all resources allocated by @gaudio_setup().
 */
void gaudio_cleanup(struct gaudio *card)
{
	gaudio_close_snd_dev(card);
}
