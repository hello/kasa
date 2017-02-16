/**
 * audio_platform_interface.cpp
 *
 * History:
 * 2013/5/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "audio_platform_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

#ifdef BUILD_MODULE_ALSA
extern IAudioHAL *gfCreateAlsaAuidoHAL(const volatile SPersistMediaConfig *p_config);
#endif

#ifdef BUILD_MODULE_PULSEAUDIO
extern IAudioHAL *gfCreatePulseAuidoHAL(const volatile SPersistMediaConfig *p_config);
#endif

IAudioHAL *gfAudioHALFactory(EAudioPlatform audio_platform, const volatile SPersistMediaConfig *p_config)
{
    IAudioHAL *api = NULL;

    switch (audio_platform) {
        case EAudioPlatform_Alsa:
#ifdef BUILD_MODULE_ALSA
            api = gfCreateAlsaAuidoHAL(p_config);
#else
            LOG_FATAL("not compiled with alsa\n");
#endif
            break;

        case EAudioPlatform_PulseAudio:
#ifdef BUILD_MODULE_PULSEAUDIO
            api = gfCreatePulseAuidoHAL(p_config);
#else
            LOG_FATAL("not compiled with pulse audio\n");
#endif
            break;

        case EAudioPlatform_Android:
            LOG_ERROR("please implement EAudioPlatform_Android\n");
            break;

        default:
            LOG_FATAL("BAD EDSPPlatform %d\n", audio_platform);
            break;
    }

    return api;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

