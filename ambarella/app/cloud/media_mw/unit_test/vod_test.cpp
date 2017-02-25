/*******************************************************************************
 * vod_test.cpp
 *
 * History:
 *    2014/05/07 - [Zhi He] create file
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

#include "storage_lib_if.h"
#include "media_mw_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

static TU8 g_config_log_level = ELogLevel_Info;
static TU8 g_enable_video = 1;
static TU8 g_enable_audio = 0;
TChar g_path[256] = {0};
TChar g_local_file[256] = {0};

void InitParames(TInt argc, TChar **argv)
{
    //vod_test --path path
    if (argc > 1) {
        for (TInt i = 1; i < argc; i++) {
            if (!strcmp("--path", argv[i])) {
                if (argc > (i + 1)) {
                    strncpy(g_path, argv[i + 1], 256);
                    i++;
                }
            } else if (!strcmp("-f", argv[i])) {
                if (argc > (i + 1)) {
                    snprintf(g_local_file, 256, "%s", argv[i + 1]);
                    i++;
                }
            }
        }
    }
}

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

int main(TInt argc, TChar **argv)
{
    EECode err = EECode_NotInitilized;

    InitParames(argc, argv);

    TGenericID source_id, server_id, flow_controller_id, transmiter_id, connection_id, vod_pipeline_id;

#if 0
    IGenericEngineControlV2 *p_engine = NULL;
    do {
        //create engine
        p_engine = gfMediaEngineFactoryV2(EFactory_MediaEngineType_Generic);
        if (!p_engine) { break; }
        p_engine->SetEngineLogLevel((ELogLevel)g_config_log_level);

        //start process pipeline
        err = p_engine->BeginConfigProcessPipeline(1);
        if (err != EECode_OK) { break; }

        //create component
        err = p_engine->NewComponent(EGenericComponentType_Demuxer, source_id);
        if (err != EECode_OK) { break; }

        err = p_engine->NewComponent(EGenericComponentType_FlowController, flow_controller_id);
        if (err != EECode_OK) { break; }

        err = p_engine->NewComponent(EGenericComponentType_StreamingServer, server_id);
        if (err != EECode_OK)  { break; }

        err = p_engine->NewComponent(EGenericComponentType_StreamingTransmiter, transmiter_id);
        if (err != EECode_OK)  { break; }

        //connet component
        if (g_enable_video) {
            err = p_engine->ConnectComponent(connection_id, source_id, flow_controller_id,  StreamType_Video);
            if (err != EECode_OK) { break; }

            err = p_engine->ConnectComponent(connection_id, flow_controller_id, transmiter_id,  StreamType_Video);
            if (err != EECode_OK) { break; }
        }

        if (g_enable_audio) {
            err = p_engine->ConnectComponent(connection_id, source_id, flow_controller_id,  StreamType_Audio);
            if (err != EECode_OK) { break; }

            err = p_engine->ConnectComponent(connection_id, flow_controller_id, transmiter_id,  StreamType_Audio);
            if (err != EECode_OK) { break; }
        }

        //setup pipeline
        TGenericID video_source_id = g_enable_video ? source_id : 0;
        TGenericID audio_source_id = g_enable_audio ? source_id : 0;
        err = p_engine->SetupVODPipeline(vod_pipeline_id, video_source_id, audio_source_id, flow_controller_id, transmiter_id, server_id, 1);
        //err = p_engine->SetupStreamingPipeline(vod_pipeline_id, transmiter_id, server_id, video_source_id, audio_source_id);
        if (err != EECode_OK) { break; }

        //const TChar* url = "vod_ts";
        //err = p_engine->SetVodUrl(source_id, vod_pipeline_id, const_cast<TChar*>(url));
        //if (err != EECode_OK) break;

        //end process pipeline
        err = p_engine->FinalizeConfigProcessPipeline();
        if (err != EECode_OK) { break; }

        //TChar url[] = "vod_ts";
        err = p_engine->SetVodUrl(source_id, vod_pipeline_id, g_local_file);
        if (err != EECode_OK) { break; }

        //run
        err = p_engine->StartProcess();
        if (err != EECode_OK) { break; }

        RunMainLoop();
    } while (0);
#else
    IGenericEngineControlV3 *p_engine = NULL;

    do {
        //create engine
        p_engine = gfMediaEngineFactoryV3(EFactory_MediaEngineType_Generic, NULL);
        if (!p_engine) { break; }
        p_engine->SetEngineLogLevel((ELogLevel)g_config_log_level);

        //start process pipeline
        err = p_engine->BeginConfigProcessPipeline(1);
        if (err != EECode_OK) { break; }

        //create component
        err = p_engine->NewComponent(EGenericComponentType_Demuxer, source_id);
        if (err != EECode_OK) { break; }

        err = p_engine->NewComponent(EGenericComponentType_FlowController, flow_controller_id);
        if (err != EECode_OK) { break; }

        err = p_engine->NewComponent(EGenericComponentType_StreamingServer, server_id);
        if (err != EECode_OK)  { break; }

        err = p_engine->NewComponent(EGenericComponentType_StreamingTransmiter, transmiter_id);
        if (err != EECode_OK)  { break; }

        //connet component
        if (g_enable_video) {
            err = p_engine->ConnectComponent(connection_id, source_id, flow_controller_id,  StreamType_Video);
            if (err != EECode_OK) { break; }

            err = p_engine->ConnectComponent(connection_id, flow_controller_id, transmiter_id,  StreamType_Video);
            if (err != EECode_OK) { break; }
        }

        if (g_enable_audio) {
            err = p_engine->ConnectComponent(connection_id, source_id, flow_controller_id,  StreamType_Audio);
            if (err != EECode_OK) { break; }

            err = p_engine->ConnectComponent(connection_id, flow_controller_id, transmiter_id,  StreamType_Audio);
            if (err != EECode_OK) { break; }
        }

        //setup pipeline
        TGenericID video_source_id = g_enable_video ? source_id : 0;
        TGenericID audio_source_id = g_enable_audio ? source_id : 0;
        err = p_engine->SetupVODPipeline(vod_pipeline_id, video_source_id, audio_source_id, flow_controller_id, transmiter_id, server_id, 1);
        //err = p_engine->SetupStreamingPipeline(vod_pipeline_id, transmiter_id, server_id, video_source_id, audio_source_id);
        if (err != EECode_OK) { break; }

        err = p_engine->SetupVodContent(/*channel*/0, g_local_file, /*localfile*/1, /*enable_video*/1, /*enable_audio*/0);
        if (err != EECode_OK) { break; }

        //const TChar* url = "vod_ts";
        //err = p_engine->SetVodUrl(source_id, vod_pipeline_id, const_cast<TChar*>(url));
        //if (err != EECode_OK) break;

        //end process pipeline
        err = p_engine->FinalizeConfigProcessPipeline();
        if (err != EECode_OK) { break; }

        //run
        err = p_engine->StartProcess();
        if (err != EECode_OK) { break; }

        RunMainLoop();
    } while (0);
#endif
    if (err != EECode_OK) {
        LOG_ERROR("get error, check me!\n");
    }

    if (p_engine) {
        p_engine->Destroy();
        p_engine = NULL;
    }

    return err;
}

