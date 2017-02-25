
/**
 * test_media.cpp
 *
 * History:
 *    2015/07/29 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "lwmd_if.h"
#include "lwmd_log.h"

#define DMAX_UT_URL_LENGTH 512

typedef struct {
    unsigned char b_playback;
    unsigned char b_record;
    unsigned char b_streaming;
    unsigned char b_watermarker;

    unsigned char b_playback_pasued;
    unsigned char b_playback_step;
    unsigned char b_disable_audio;
    unsigned char b_disable_video;

    unsigned char b_digital_vout;
    unsigned char b_hdmi_vout;
    unsigned char b_cvbs_vout;
    unsigned char reserved0;

    unsigned char use_vd_dsp;
    unsigned char use_vd_ffmpeg;
    unsigned char use_vo_dsp;
    unsigned char use_vo_linuxfb;

    unsigned char playback_direction;
    unsigned char video_streaming_id;
    unsigned short playback_speed;

    char playback_url[DMAX_UT_URL_LENGTH];
    char record_url[DMAX_UT_URL_LENGTH];
    char streaming_url[DMAX_UT_URL_LENGTH];

    char audio_device[32];
} STestMediaContext;

static int g_test_media_running = 1;

static void __print_ut_options()
{
    printf("test_media options:\n");
    printf("\t'-p [filename]': '-p' specify playback file name\n");
    printf("\t'-s [streaming url]': '-p' specify streaming url\n");

    printf("'\t'-A: stream 0\n");
    printf("'\t'-B: stream 1\n");
    printf("'\t'-C: stream 2\n");
    printf("'\t'-D: stream 3\n");

    printf("\t'--enableaudio': enable audio path\n");
    printf("\t'--disableaudio': disable audio path\n");
    printf("\t'--digital': choose digital vout\n");
    printf("\t'--hdmi': choose hdmi vout\n");
    printf("\t'--cvbs': choose cvbs vout\n");
    printf("\t'--vd-dsp': select amba dsp video decoder\n");
    printf("\t'--vd-ffmpeg': select ffmpeg video decoder\n");
    printf("\t'--vo-dsp': select dsp video output\n");
    printf("\t'--vo-fb': select linux fb video output\n");
    printf("\t'--audiodevice %%s': choose audio device, like 'hw:0,0', 'hw:0,1', etc\n");
    printf("\t'--help': print help\n\n");
}

static void __print_ut_cmds()
{
    printf("test_media runtime cmds: press cmd + Enter\n");
    printf("\t'q': Quit unit test\n");
    printf("\t'p': Print status\n");
    printf("\t'g%%d': Seek to %%d ms\n");
    printf("\t' ': pause/resume\n");
    printf("\t's': step play\n");
    printf("\t'f%%x': fast forward, %%x use speed.speed_frac format, for example, 400 = 4x, 800 = 8x, 180 = 1.5x, will choose I frame only mode\n");
    printf("\t'b%%x': fast backward, %%x use speed.speed_frac format, for example, 400 = 4x, 800 = 8x, 180 = 1.5x, will choose I frame only mode\n");
}

static TInt __replace_enter(TChar *p_buffer, TUint size)
{
    while (size) {
        if (('\n') == (*p_buffer)) {
            *p_buffer = 0x0;
            return 0;
        }
        p_buffer ++;
        size --;
    }

    LOG_FATAL("no enter found, should be error\n");

    return 1;
}
static void __safe_resume_before_seek_fast_fwbw(STestMediaContext *context, IGenericEngineControl *engine, TGenericID pipelind_id)
{
    if ((context->b_playback_pasued) || (context->b_playback_step)) {
        SUserParamResume resume;
        resume.check_field = EGenericEngineConfigure_Resume;
        resume.component_id = pipelind_id;
        engine->GenericControl(EGenericEngineConfigure_Resume, &resume);
        context->b_playback_pasued = 0;
        context->b_playback_step = 0;
    }
}

static int __test_playback(STestMediaContext *context, IGenericEngineControl *engine)
{
    EECode err = EECode_OK;
    TInt ret = 0;
    TGenericID demuxer_id = 0, video_decoder_id = 0, audio_decoder_id = 0, video_renderer_id = 0, audio_renderer_id = 0;
    TGenericID connection_id = 0;
    TGenericID playback_pipeline_id = 0;

    err = engine->BeginConfigProcessPipeline();
    DASSERT(EECode_OK == err);

    err = engine->NewComponent(EGenericComponentType_Demuxer, demuxer_id, "PRMP4");
    DASSERT(EECode_OK == err);

    if (context->use_vd_dsp) {
        err = engine->NewComponent(EGenericComponentType_VideoDecoder, video_decoder_id, "AMBA");
    } else if (context->use_vd_ffmpeg) {
        err = engine->NewComponent(EGenericComponentType_VideoDecoder, video_decoder_id, "FFMpeg");
    } else {
        LOG_FATAL("do not sepcify video decoder?\n");
        return (-1);
    }
    DASSERT(EECode_OK == err);

    if (context->use_vo_dsp) {
        err = engine->NewComponent(EGenericComponentType_VideoRenderer, video_renderer_id, "AMBA");
    } else if (context->use_vo_linuxfb) {
        err = engine->NewComponent(EGenericComponentType_VideoRenderer, video_renderer_id, "LinuxFB");
    } else {
        LOG_FATAL("do not sepcify video output?\n");
        return (-1);
    }
    DASSERT(EECode_OK == err);

    if (!context->b_disable_audio) {
        err = engine->NewComponent(EGenericComponentType_AudioDecoder, audio_decoder_id, "AAC");
        DASSERT(EECode_OK == err);

        err = engine->NewComponent(EGenericComponentType_AudioRenderer, audio_renderer_id, "ALSA");
        DASSERT(EECode_OK == err);
    } else {
        audio_decoder_id = 0;
        audio_renderer_id = 0;
    }

    err = engine->ConnectComponent(connection_id, demuxer_id, video_decoder_id,  StreamType_Video);
    if (EECode_OK != err) {
        LOG_FATAL("connect demuxer and video decoder fail\n");
        return (-1);
    }

    err = engine->ConnectComponent(connection_id, video_decoder_id, video_renderer_id,  StreamType_Video);
    if (EECode_OK != err) {
        LOG_FATAL("connect video decoder and video renderer fail\n");
        return (-1);
    }

    if (!context->b_disable_audio) {
        err = engine->ConnectComponent(connection_id, demuxer_id, audio_decoder_id,  StreamType_Audio);
        if (EECode_OK != err) {
            LOG_FATAL("connect demuxer and audio decoder fail\n");
            return (-1);
        }

        err = engine->ConnectComponent(connection_id, audio_decoder_id, audio_renderer_id,  StreamType_Audio);
        if (EECode_OK != err) {
            LOG_FATAL("connect audio decoder and audio renderer fail\n");
            return (-1);
        }
    }

    err = engine->SetupPlaybackPipeline(playback_pipeline_id, demuxer_id, demuxer_id, video_decoder_id, audio_decoder_id, video_renderer_id, audio_renderer_id);
    if (EECode_OK != err) {
        LOG_FATAL("setup playback pipeline fail\n");
        return (-1);
    }

    SConfigVout config_vout;
    config_vout.check_field = EGenericEngineConfigure_ConfigVout;
    config_vout.b_digital_vout = context->b_digital_vout;
    config_vout.b_hdmi_vout = context->b_hdmi_vout;
    config_vout.b_cvbs_vout = context->b_cvbs_vout;
    printf("[vout config]: digital %d, hdmi %d, cvbs %d\n", config_vout.b_digital_vout, config_vout.b_hdmi_vout, config_vout.b_cvbs_vout);
    err = engine->GenericControl(EGenericEngineConfigure_ConfigVout, &config_vout);

    if (context->audio_device[0]) {
        SConfigAudioDevice audio_device;
        audio_device.check_field = EGenericEngineConfigure_ConfigAudioDevice;
        snprintf(audio_device.audio_device_name, sizeof(audio_device.audio_device_name), "%s", context->audio_device);
        printf("[audio device]: %s\n", audio_device.audio_device_name);
        err = engine->GenericControl(EGenericEngineConfigure_ConfigAudioDevice, &audio_device);
    }

    err = engine->FinalizeConfigProcessPipeline();
    if (EECode_OK != err) {
        LOG_FATAL("FinalizeConfigProcessPipeline fail\n");
        return (-1);
    }

    err = engine->SetSourceUrl(demuxer_id, context->playback_url);
    if (EECode_OK != err) {
        LOG_FATAL("SetSourceUrl fail\n");
        return (-1);
    }

    err = engine->RunProcessing();
    if (EECode_OK != err) {
        LOG_FATAL("RunProcessing fail\n");
        return (-1);
    }

    err = engine->Start();
    if (EECode_OK != err) {
        LOG_FATAL("Start fail\n");
        return (-1);
    }

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;

    int flag_stdin = 0;
    flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
        LOG_FATAL("stdin_fileno set error.\n");
    }

    //process user cmd line
    while (g_test_media_running) {
        //add sleep to avoid affecting the performance
        usleep(100000);
        //memset(buffer, 0x0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            continue;
        }

        printf("[user cmd debug]: read input '%s'\n", buffer);

        if (buffer[0] == '\n') {
            p_buffer = buffer_old;
            printf("repeat last cmd:\n\t\t%s\n", buffer_old);
        } else if (buffer[0] == 'l') {
            printf("show last cmd:\n\t\t%s\n", buffer_old);
            continue;
        } else {

            ret = __replace_enter(buffer, (128 - 1));
            DASSERT(0 == ret);
            if (ret) {
                LOG_FATAL("no enter found\n");
                continue;
            }
            p_buffer = buffer;

            //record last cmd
            strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
            buffer_old[sizeof(buffer_old) - 1] = 0x0;
        }

        printf("[user cmd debug]: '%s'\n", p_buffer);

        switch (p_buffer[0]) {

            case 'q':   // exit
                printf("[user cmd]: 'q', Quit\n");
                g_test_media_running = 0;
                break;

            case 'p':
                engine->PrintCurrentStatus();
                break;

            case 'g': {
                    SPbSeek seek;
                    unsigned int target = 0;
                    sscanf(p_buffer + 1, "%d", &target);

                    seek.check_field = EGenericEngineConfigure_Seek;
                    seek.component_id = playback_pipeline_id;
                    seek.target = target;
                    seek.position = ENavigationPosition_Begining;

                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before seek\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }

                    err = engine->GenericControl(EGenericEngineConfigure_Seek, &seek);
                }
                break;

            case 'f': {
                    SPbFeedingRule ff;
                    unsigned int speed;
                    sscanf(p_buffer + 1, "%x", &speed);

                    ff.check_field = EGenericEngineConfigure_FastForward;
                    ff.component_id = playback_pipeline_id;
                    ff.direction = 0;
                    ff.feeding_rule = DecoderFeedingRule_IDROnly;
                    ff.speed = speed;

                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before fast fw\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }

                    err = engine->GenericControl(EGenericEngineConfigure_FastForward, &ff);
                }
                break;

            case 'b': {
                    SPbFeedingRule fb;
                    unsigned int speed;
                    sscanf(p_buffer + 1, "%x", &speed);

                    fb.check_field = EGenericEngineConfigure_FastBackward;
                    fb.component_id = playback_pipeline_id;
                    fb.direction = 1;
                    fb.feeding_rule = DecoderFeedingRule_IDROnly;
                    fb.speed = speed;

                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before fast bw\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }

                    err = engine->GenericControl(EGenericEngineConfigure_FastBackward, &fb);
                }
                break;

            case 'F': {
                    SPbFeedingRule ff;
                    unsigned int speed;
                    sscanf(p_buffer + 1, "%x", &speed);
                    ff.check_field = EGenericEngineConfigure_FastForwardFromBegin;
                    ff.component_id = playback_pipeline_id;
                    ff.direction = 0;
                    ff.feeding_rule = DecoderFeedingRule_IDROnly;
                    ff.speed = speed;

                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before fast fw\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }

                    err = engine->GenericControl(EGenericEngineConfigure_FastForwardFromBegin, &ff);
                }
                break;

            case 'B': {
                    SPbFeedingRule fb;
                    unsigned int speed;
                    sscanf(p_buffer + 1, "%x", &speed);
                    fb.check_field = EGenericEngineConfigure_FastBackwardFromEnd;
                    fb.component_id = playback_pipeline_id;
                    fb.direction = 1;
                    fb.feeding_rule = DecoderFeedingRule_IDROnly;
                    fb.speed = speed;

                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before fast fb\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }

                    err = engine->GenericControl(EGenericEngineConfigure_FastBackwardFromEnd, &fb);
                }
                break;

            case 'r': {
                    SResume1xPlayback r;
                    r.check_field = EGenericEngineConfigure_Resume1xFromCurrent;
                    r.component_id = playback_pipeline_id;
                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before return to normal playback\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }
                    err = engine->GenericControl(EGenericEngineConfigure_Resume1xFromCurrent, &r);
                }
                break;

            case 'R': {
                    SResume1xPlayback r;
                    r.check_field = EGenericEngineConfigure_Resume1xFromBegin;
                    r.component_id = playback_pipeline_id;
                    if ((context->b_playback_pasued) || (context->b_playback_step)) {
                        printf("safe resume before return to normal playback\n");
                        __safe_resume_before_seek_fast_fwbw(context, engine, playback_pipeline_id);
                    }
                    err = engine->GenericControl(EGenericEngineConfigure_Resume1xFromBegin, &r);
                }
                break;

            case ' ': {
                    if ((!context->b_playback_pasued) && (!context->b_playback_step)) {
                        SUserParamPause pause;
                        pause.check_field = EGenericEngineConfigure_Pause;
                        pause.component_id = playback_pipeline_id;
                        err = engine->GenericControl(EGenericEngineConfigure_Pause, &pause);
                        context->b_playback_pasued = 1;
                    } else {
                        SUserParamResume resume;
                        resume.check_field = EGenericEngineConfigure_Resume;
                        resume.component_id = playback_pipeline_id;
                        err = engine->GenericControl(EGenericEngineConfigure_Resume, &resume);
                        context->b_playback_pasued = 0;
                        context->b_playback_step = 0;
                    }

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_FATAL("EUserParamType_PlaybackTrickplay(pause resume) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }
                }
                break;

            case 's': {
                    SUserParamResume step;
                    step.check_field = EGenericEngineConfigure_StepPlay;
                    step.component_id = playback_pipeline_id;
                    err = engine->GenericControl(EGenericEngineConfigure_StepPlay, &step);
                    context->b_playback_step = 1;

                    if (DUnlikely(EECode_OK != err)) {
                        LOG_FATAL("EUserParamType_PlaybackTrickplay(step) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        break;
                    }
                }
                break;

            case 'h':   // help
                __print_ut_options();
                __print_ut_cmds();
                break;

            default:
                break;
        }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
        LOG_FATAL("stdin_fileno set error\n");
    }

    err = engine->Stop();
    if (EECode_OK != err) {
        LOG_FATAL("Stop fail\n");
        return (-1);
    }

    err = engine->ExitProcessing();
    if (EECode_OK != err) {
        LOG_FATAL("ExitProcessing fail\n");
        return (-1);
    }

    return 0;
}

static int __test_live_streaming(STestMediaContext *context, IGenericEngineControl *engine)
{
    EECode err = EECode_OK;
    TInt ret = 0;
    TGenericID video_source_id = 0, audio_source_id = 0, rtsp_transmiter_id = 0, server_id = 0;
    TGenericID connection_id = 0;
    TGenericID streaming_pipeline_id = 0;

    SConfigVideoStreamID config_stream_id;
    //pre configs
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = context->video_streaming_id;
    printf("stream index %d\n", context->video_streaming_id);
    err = engine->GenericControl(EGenericEngineConfigure_VideoStreamID, &config_stream_id);

    err = engine->BeginConfigProcessPipeline();
    DASSERT(EECode_OK == err);

    err = engine->NewComponent(EGenericComponentType_VideoEncoder, video_source_id, "AMBA");
    DASSERT(EECode_OK == err);

    err = engine->NewComponent(EGenericComponentType_StreamingTransmiter, rtsp_transmiter_id, "AUTO");
    DASSERT(EECode_OK == err);

    err = engine->NewComponent(EGenericComponentType_StreamingServer, server_id, "RTSP");
    DASSERT(EECode_OK == err);

    if (!context->b_disable_audio) {
        err = engine->NewComponent(EGenericComponentType_AudioEncoder, audio_source_id, "AAC");
        DASSERT(EECode_OK == err);
    }

    err = engine->ConnectComponent(connection_id, video_source_id, rtsp_transmiter_id,  StreamType_Video);
    if (EECode_OK != err) {
        LOG_FATAL("connect video source and rtsp transmiter fail\n");
        return (-1);
    }

    if ((!context->b_disable_audio) && audio_source_id) {
        err = engine->ConnectComponent(connection_id, audio_source_id, rtsp_transmiter_id,  StreamType_Audio);
        if (EECode_OK != err) {
            LOG_FATAL("connect audio source and rtsp transmiter fail\n");
            return (-1);
        }
    }

    err = engine->SetupStreamingPipeline(streaming_pipeline_id, rtsp_transmiter_id, server_id, video_source_id, audio_source_id);
    if (EECode_OK != err) {
        LOG_FATAL("setup streaming pipeline fail\n");
        return (-1);
    }

    err = engine->FinalizeConfigProcessPipeline();
    if (EECode_OK != err) {
        LOG_FATAL("FinalizeConfigProcessPipeline fail\n");
        return (-1);
    }

    if (0x0 == context->streaming_url[0]) {
        strcpy(context->streaming_url, "local");
    }
    err = engine->SetStreamingUrl(streaming_pipeline_id, context->streaming_url);
    printf("streaming url: %s\n", context->streaming_url);
    if (EECode_OK != err) {
        LOG_FATAL("SetStreamingUrl fail\n");
        return (-1);
    }

    err = engine->RunProcessing();
    if (EECode_OK != err) {
        LOG_FATAL("RunProcessing fail\n");
        return (-1);
    }

    err = engine->Start();
    if (EECode_OK != err) {
        LOG_FATAL("Start fail\n");
        return (-1);
    }

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;

    int flag_stdin = 0;
    flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
        LOG_FATAL("stdin_fileno set error.\n");
    }

    //process user cmd line
    while (g_test_media_running) {
        //add sleep to avoid affecting the performance
        usleep(100000);
        //memset(buffer, 0x0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            continue;
        }

        printf("[user cmd debug]: read input '%s'\n", buffer);

        if (buffer[0] == '\n') {
            p_buffer = buffer_old;
            printf("repeat last cmd:\n\t\t%s\n", buffer_old);
        } else if (buffer[0] == 'l') {
            printf("show last cmd:\n\t\t%s\n", buffer_old);
            continue;
        } else {

            ret = __replace_enter(buffer, (128 - 1));
            DASSERT(0 == ret);
            if (ret) {
                LOG_FATAL("no enter found\n");
                continue;
            }
            p_buffer = buffer;

            //record last cmd
            strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
            buffer_old[sizeof(buffer_old) - 1] = 0x0;
        }

        printf("[user cmd debug]: '%s'\n", p_buffer);

        switch (p_buffer[0]) {

            case 'q':   // exit
                printf("[user cmd]: 'q', Quit\n");
                g_test_media_running = 0;
                break;

            case 'p':
                engine->PrintCurrentStatus();
                break;

            case 'h':   // help
                __print_ut_options();
                __print_ut_cmds();
                break;

            default:
                break;
        }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
        LOG_FATAL("stdin_fileno set error\n");
    }

    err = engine->Stop();
    if (EECode_OK != err) {
        LOG_FATAL("Stop fail\n");
        return (-1);
    }

    err = engine->ExitProcessing();
    if (EECode_OK != err) {
        LOG_FATAL("ExitProcessing fail\n");
        return (-1);
    }

    return 0;
}

static int __init_test_media_params(int argc, char **argv, STestMediaContext *context)
{
    int i = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp("-p", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->playback_url, DMAX_UT_URL_LENGTH, "%s", argv[i + 1]);
                i ++;
                printf("[input argument] -p: playback %s.\n", context->playback_url);
                context->b_playback = 1;
            } else {
                LOG_ERROR("[input argument] -p: should follow playback filename.\n");
            }
        } else if (!strcmp("-s", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->streaming_url, DMAX_UT_URL_LENGTH, "%s", argv[i + 1]);
                i ++;
                printf("[input argument] -s: url %s.\n", context->streaming_url);
                context->b_streaming = 1;
            } else {
                LOG_ERROR("[input argument] -s: should follow treaming url.\n");
            }
        } else if (!strcmp("--enableaudio", argv[i])) {
            context->b_disable_audio = 0;
        } else if (!strcmp("--disableaudio", argv[i])) {
            context->b_disable_audio = 1;
        } else if (!strcmp("-A", argv[i])) {
            context->video_streaming_id = 0;
        } else if (!strcmp("-B", argv[i])) {
            context->video_streaming_id = 1;
        } else if (!strcmp("-C", argv[i])) {
            context->video_streaming_id = 2;
        } else if (!strcmp("-D", argv[i])) {
            context->video_streaming_id = 3;
        } else if (!strcmp("--digital", argv[i])) {
            context->b_digital_vout = 1;
        } else if (!strcmp("--hdmi", argv[i])) {
            context->b_hdmi_vout = 1;
        } else if (!strcmp("--cvbs", argv[i])) {
            context->b_cvbs_vout = 1;
        } else if (!strcmp("--vd-dsp", argv[i])) {
            context->use_vd_dsp = 1;
        } else if (!strcmp("--vd-ffmpeg", argv[i])) {
            context->use_vd_ffmpeg = 1;
        } else if (!strcmp("--vo-dsp", argv[i])) {
            context->use_vo_dsp = 1;
        } else if (!strcmp("--vo-fb", argv[i])) {
            context->use_vo_linuxfb = 1;
        } else if (!strcmp("--audiodevice", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(context->audio_device, 32, "%s", argv[i + 1]);
                i ++;
                printf("[input argument] --audiodevice: %s.\n", context->audio_device);
            } else {
                printf("[input argument] --audiodevice: should follow audio device name.\n");
            }
        } else if (!strcmp("--help", argv[i])) {
            __print_ut_options();
            __print_ut_cmds();
        } else {
            LOG_ERROR("error: NOT processed option(%s).\n", argv[i]);
            __print_ut_options();
            __print_ut_cmds();
            return (-1);
        }
    }

    return 0;
}

static void __test_media_sig_stop(int a)
{
    g_test_media_running = 0;
}

int main(int argc, char **argv)
{
    STestMediaContext context;
    TInt ret = 0;
    IGenericEngineControl *engine_control = NULL;

    signal(SIGINT, __test_media_sig_stop);
    signal(SIGQUIT, __test_media_sig_stop);
    signal(SIGTERM, __test_media_sig_stop);

    if (argc > 1) {
        memset(&context, 0x0, sizeof(context));
        context.b_disable_audio = 1;
        ret = __init_test_media_params(argc, argv, &context);
        if (ret) {
            return (-2);
        }
    } else {
        __print_ut_options();
        __print_ut_cmds();
        return 0;
    }

    //gfOpenLogFile((TChar*) "./lwmedia.txt");

    engine_control = CreateLWMDEngine();

    if (!engine_control) {
        LOG_FATAL("CreateLWMDEngine() fail\n");
        return (-1);
    }

    if (context.b_playback) {
        if ((!context.use_vd_dsp) && (!context.use_vd_ffmpeg)) {
            printf("use dsp video decoder as default\n");
            context.use_vd_dsp = 1;
            context.use_vo_dsp = 1;
        }
        __test_playback(&context, engine_control);
    } else if (context.b_streaming) {
        __test_live_streaming(&context, engine_control);
    }

    engine_control->Destroy();

    return 0;
}

