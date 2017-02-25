/*
 * u_uac1.h -- interface to USB gadget "ALSA AUDIO" utilities
 *
 * Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 * Copyright (C) 2008 Analog Devices, Inc
 *
 * Enter bugs at http://blackfin.uclinux.org/
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef __U_AUDIO_H
#define __U_AUDIO_H

#include <linux/device.h>
#include <linux/err.h>
#include <linux/usb/audio.h>
#include <linux/usb/composite.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "gadget_chips.h"
#include "../defconfig.h"

#define FILE_PCM_PLAYBACK	"/dev/snd/pcmC0D0p"
#define FILE_PCM_CAPTURE	"/dev/snd/pcmC0D0c"
#define FILE_CONTROL		"/dev/snd/controlC0"

#if LINUX   /* host is linux OS */
#define UAC1_OUT_EP_MAX_PACKET_SIZE	 (96)
#define UAC1_REQ_COUNT				  2
#elif UAC_EX_SOLUTION
#define UAC1_OUT_EP_MAX_PACKET_SIZE	 (96 / 8)
#define UAC1_REQ_COUNT				  2
#else
#define UAC1_OUT_EP_MAX_PACKET_SIZE	 (96 * 2)
#define UAC1_REQ_COUNT				  1
#endif

#define UAC1_AUDIO_BUF_SIZE			  48000
/* audio buffer size */
#define FRAME_SIZE  				 (UAC1_OUT_EP_MAX_PACKET_SIZE * 10)

/*
 * This represents the USB side of an audio card device, managed by a USB
 * function which provides control and stream interfaces.
 */

struct gaudio_snd_dev {
	struct gaudio			*card;
	struct file			*filp;
	struct snd_pcm_substream	*substream;
	int				access;
	int				format;
	int				channels;
	int				rate;
};

struct gaudio {
	struct usb_function		func;
	struct usb_gadget		*gadget;

	/* ALSA sound device interfaces */
	struct gaudio_snd_dev		control;
	struct gaudio_snd_dev		playback;
	struct gaudio_snd_dev		capture;

	/* TODO */
};

struct f_uac1_opts {
	struct usb_function_instance	func_inst;
	int				req_buf_size;
	int				req_count;
	int				audio_buf_size;
	char				*fn_play;
	char				*fn_cap;
	char				*fn_cntl;
	unsigned			bound:1;
	unsigned			fn_play_alloc:1;
	unsigned			fn_cap_alloc:1;
	unsigned			fn_cntl_alloc:1;
	struct mutex			lock;
	int				refcnt;
};
/* --------------------------------------------------------------------- */

struct frame_info
{
	char buf[FRAME_SIZE];
	int total_size;
	int bused_size;
	int nodata;
};

struct f_audio {
	struct gaudio			card;

	/* endpoints handle full and/or high speeds */
	struct usb_ep			*out_ep;

	spinlock_t			lock;
	struct f_audio_buf *copy_buf;
	struct work_struct playback_work;
	struct list_head play_queue;

	/* Control Set command */
	struct list_head cs;
	u8 set_cmd;
	struct usb_audio_control *set_con;

	struct usb_request *req[UAC1_REQ_COUNT];
	bool ep_enabled;
	int current_used;
	struct frame_info frame[2];

#if  AUDIO_DEBUG_PLAY
	struct file *play_file;
	char debug_frame[AUDIO_DEBUG_PLAY_SIZE];
	int debug_avail;
#endif


};

int gaudio_setup(struct gaudio *card);
void gaudio_cleanup(struct gaudio *the_card);

int audio_kthread_get_frame(void *arg);


#endif /* __U_AUDIO_H */
