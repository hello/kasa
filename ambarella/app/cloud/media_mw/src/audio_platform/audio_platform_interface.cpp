/*******************************************************************************
 * audio_platform_interface.cpp
 *
 * History:
 * 2013/5/07 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

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

