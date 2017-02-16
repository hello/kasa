/*
 * audio_al.h
 *
 * History:
 *    2015/07/31 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_AL_H__
#define __AUDIO_AL_H__

typedef void *(*TFAudioALOpenCapture)(TChar *device_name, TU32 sample_rate, TU32 channels, TU32 frame_size);
typedef void *(*TFAudioALOpenPlayback)(TChar *device_name, TU32 sample_rate, TU32 channels, TU32 frame_size);
typedef void (*TFAudioALClose)(void *p_handle);
typedef TInt(*TFAudioALStartCapture)(void *p_handle);
typedef TInt(*TFAudioALRenderData)(void *p_handle, TU8 *pData, TUint dataSize, TUint *pNumFrames);
typedef TInt(*TFAudioALReadData)(void *p_handle, TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStampUs);

typedef struct {
    TFAudioALOpenCapture f_open_capture;
    TFAudioALOpenPlayback f_open_playback;
    TFAudioALClose f_close;

    TFAudioALStartCapture f_start_capture;
    TFAudioALRenderData f_render_data;
    TFAudioALReadData f_read_data;
} SFAudioAL;

extern void gfSetupAlsaAlContext(SFAudioAL *al);

#endif

