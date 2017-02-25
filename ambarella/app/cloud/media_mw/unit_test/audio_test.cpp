/*******************************************************************************
 * atest.cpp
 *
 * History:
 *    2014/01/07 - [Zhi He] create file
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

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

static TU8 g_config_log_level = ELogLevel_Notice;

#define CHECK_RET(err)  \
    do { \
        if (err != EECode_OK) { \
            goto ERROR; \
        } \
    } \
    while (0)

////////////////////////////////////////////////////////////////////////////
void RunMainLoop()
{
    char buffer_old[1024] = {0};
    char buffer[1024];
    bool flag_run = true;
    int flag_stdin = 0;

    char name[128];
    int index = -1;
    int type = -1;
    int ret = -1;
    flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1)
    { printf("stdin_fileno set error.\n"); }

    while (flag_run) {
        usleep(1000000);
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
        { continue; }

        if (buffer[0] == '\n')
        { buffer[0] = buffer_old[0]; }

        switch (buffer[0]) {
            case 'q':
                printf("Quit!\n");
                flag_run = false;
                break;
            default:
                break;
        }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1)

    { return; }
}

void InitParams(TInt argc, TChar **argv, SAudioParams &audioParams)
{
    audioParams.sample_format = AudioSampleFMT_S16;
    audioParams.is_big_endian = 0;
    audioParams.sample_rate = DDefaultAudioSampleRate;
    audioParams.channel_number = DDefaultAudioChannelNumber;

    //audio_test --sample_format %d --sample_rate %d --channel %d --loglevel %d
    if (argc > 1) {
        for (TInt i = 1; i < argc; i++) {
            if (!strcmp("--sample_format", argv[i])) {
                if (argc > (i + 1)) {
                    audioParams.sample_format = (AudioSampleFMT)atoi(argv[i + 1]);
                } else {
                    //
                }
            } else if (!strcmp("--sample_rate", argv[i])) {
                if (argc > (i + 1)) {
                    audioParams.sample_rate = atoi(argv[i + 1]);
                } else {
                    //
                }
            } else if (!strcmp("--channel", argv[i])) {
                if (argc > (i + 1)) {
                    audioParams.channel_number = atoi(argv[i + 1]);
                } else {
                    //
                }
            } else if (!strcmp("--loglevel", argv[i])) {
                if (argc > (i + 1)) {
                    g_config_log_level = atoi(argv[i + 1]);
                } else {
                    //
                }
            }
        }
    }
    return;
}

int main(TInt argc, TChar **argv)
{
    EECode err = EECode_NotInitilized;
    SAudioParams audioParams;
    memset(&audioParams, 0, sizeof(audioParams));

    InitParams(argc, argv, audioParams);

    TGenericID source_id, render_id, connection_id, playback_pipeline_id;
    IGenericEngineControlV2 *p_engine = gfMediaEngineFactoryV2(EFactory_MediaEngineType_Generic);

    p_engine->SetEngineLogLevel((ELogLevel)g_config_log_level);

    SGenericAudioParam genericAudioParam;
    genericAudioParam.check_field = EGenericEnginePreConfigure_AudioCapturer_Params;
    genericAudioParam.set_or_get = 1;
    genericAudioParam.audio_params = audioParams;
    err = p_engine->GenericPreConfigure(EGenericEnginePreConfigure_AudioCapturer_Params, (void *)&genericAudioParam);

    err = p_engine->BeginConfigProcessPipeline(1);

    err = p_engine->NewComponent(EGenericComponentType_AudioCapture, source_id);

    CHECK_RET(err);

    err = p_engine->NewComponent(EGenericComponentType_AudioRenderer, render_id);
    if (err != EECode_OK) {
        goto ERROR;
    }

    err = p_engine->ConnectComponent(connection_id, source_id, render_id, StreamType_Audio);
    if (err != EECode_OK) {
        goto ERROR;
    }

    err = p_engine->SetupPlaybackPipeline(playback_pipeline_id, 0, source_id, 0, 0, 0, render_id, 0, 1);
    if (err != EECode_OK) {
        goto ERROR;
    }

    err = p_engine->FinalizeConfigProcessPipeline();
    if (err != EECode_OK) {
        goto ERROR;
    }

    err = p_engine->StartProcess();
    if (err != EECode_OK) {
        goto ERROR;
    }

    RunMainLoop();

    err = p_engine->StopProcess();

#if 0
    SPersistMediaConfig mPersistMediaConfig;

    IFilter *p_AudioCapturer = gfCreateAudioCapturerFilter("CAudioCapturer", &mPersistMediaConfig, NULL, 0);

    if (!p_AudioCapturer) {
        LOGM_FATAL("failed to gfCreateAudioCapturerFilter!\n");
        goto ERROR;
    }

    EECode ret = EECode_OK;
    SAudioParams audioParams;
    audioParams.sample_rate = DDefaultAudioSampleRate;
    audioParams.sample_format = AudioSampleFMT_S16;
    audioParams.channel_number = DDefaultAudioChannelNumber;
    ret = p_AudioCapturer->FilterProperty(EFilterPropertyType_audio_parameters, 1, (void *)audioParams);
    if (ret != EECode_OK) {
        LOGM_FATAL("failed to set EFilterPropertyType_audio_parameters!\n");
        goto ERROR;
    }

    ret = p_AudioCapturer->Initialize("ALSA");
    if (ret != EECode_OK) {
        LOGM_FATAL("failed to Initialize!\n");
        goto ERROR;
    }

    p_AudioCapturer->Run();
    p_AudioCapturer->Start();


    RunMainLoop();

    p_AudioCapturer->Stop();
    p_AudioCapturer->Finalize();
    p_AudioCapturer->Delete();
    p_AudioCapturer = NULL;
    return 0;
#endif
ERROR:
    if (p_engine) {
        p_engine->Destroy();
        p_engine = NULL;
    }
    return -1;
}