/*
 * kernel/private/drivers/uvc_camera/defconfig.h
 *
 * History:
 *    2015/11/10 - qtu@ambarella.com
 *
 * Copyright (C) 2004-2020, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __LINUX_UVA__
#define __LINUX_UVA__

#include <config.h>

#ifdef CONFIG_AMBARELLA_UVC_WITH_UAC
#define AUDIO_OPEN  1
#else
#define AUDIO_OPEN  0
#endif

int audio_bind(struct usb_composite_dev *cdev);
int audio_unbind(struct usb_composite_dev *cdev);
int audio_do_config(struct usb_configuration *c);
int uvc_stream_on(void);
int uvc_stream_off(void);

#define UVC_DELAY (1000 * 8)

/* AUDIO_DEBUG */
#define     AUDIO_DEBUG_CAPTURE     0
#define     AUDIO_DEBUG_PLAY        0

#define     AUDIO_DEBUG_CAPTURE_FILE    "/mnt/audio_record.wav"
#define     AUDIO_DEBUG_CAPTURE_SIZE     (4 * 1024 * 1024)

#define     AUDIO_DEBUG_PLAY_FILE	     "/mnt/audio_play.wav"
#define     AUDIO_DEBUG_PLAY_SIZE         (1 * 1024 * 1024)

/* AUDIO_DEBUG END*/

/* VIDEO_DEBUG */

#define VIDEO_DEBUG           0
#define VIDEO_DEBUG_FILE      "/mnt/sample.mjpg"

/* VIDEO_DEBUG END*/

/* FIXME: Host is LINUX OS */
#define LINUX              0

/* WINDOWNS solution */
#define UAC_EX_SOLUTION    0

/* fix video flicker */
#define UVC_FLICKER_FIXED    1

#endif
