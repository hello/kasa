/**
 * test_oryx_playback_new.cpp
 *
 * History:
 *    2015/07/29 - [Zhi He] create file
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
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "am_playback_new_if.h"

#define DMAX_UT_URL_LENGTH 512
#define DMAX_UT_VOUT_STRING_LENGTH 32

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
  unsigned char b_rtsp_tcp_mode;

  unsigned char b_enable_fffbbw;
  unsigned char b_specify_vout0;
  unsigned char b_specify_vout1;
  unsigned char reserved0;

  unsigned char log_level;
  unsigned char b_log_to_file;
  unsigned char reserved1;
  unsigned char reserved2;

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

  char vout0_mode_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_mode_string[DMAX_UT_VOUT_STRING_LENGTH];

  char vout0_sinktype_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_sinktype_string[DMAX_UT_VOUT_STRING_LENGTH];

  char vout0_device_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_device_string[DMAX_UT_VOUT_STRING_LENGTH];
} STestOryxPlaybackNewContext;

static int g_test_oryx_playback_new_running = 1;
static IGenericEngineControl *g_test_engine_control = NULL;
static STestOryxPlaybackNewContext *g_test_media_context = NULL;
static TGenericID g_test_playback_pipeline_id = 0;

static void __print_ut_options()
{
  printf("test_oryx_playback_new options:\n");
  printf("\t'-p [filename]': '-p' specify playback file name\n");
  printf("\t'--enableaudio': enable audio path\n");
  printf("\t'--disableaudio': disable audio path\n");
  printf("\t'--enableff': enable fast fw/bw, bw playback\n");
  printf("\t'--disableff': disable fast fw/bw, bw playback\n");
  printf("\t'--vd-dsp': select amba dsp video decoder\n");
  printf("\t'--vd-ffmpeg': select ffmpeg video decoder\n");
  printf("\t'--vo-dsp': select dsp video output\n");
  printf("\t'--vo-fb': select linux fb video output\n");
  printf("\t'--rtsp-tcp': choose TCP mode for RTSP\n");
  printf("\t'--rtsp-udp': choose UDP mode for RTSP\n");
  printf("\t'--audiodevice %%s': choose audio device, like 'hw:0,0', 'hw:0,1', etc\n");
  printf("\t'--digital': choose digital vout\n");
  printf("\t'--hdmi': choose hdmi vout\n");
  printf("\t'--cvbs': choose cvbs vout\n");
  printf("\t'-V': specify HDMI or CVBS video output mode\n");
  printf("\t'-v': specify Digital (LCD) video output mode\n");
  gfPrintAvailableVideoOutputMode();
  gfPrintAvailableLCDModel();
  printf("\t'--loglevel %%d': set log level\n");
  printf("\t'--log2file': log to file\n");
  printf("\t'--help': print help\n\n");
}

static void __print_ut_cmds()
{
  printf("test_oryx_playback_new runtime cmds: press cmd + Enter\n");
  printf("\t'q': Quit unit test\n");
  printf("\t'p': Print status\n");
  printf("\t'g%%d': Seek to %%d ms\n");
  printf("\t' ': pause/resume\n");
  printf("\t's': step play\n");
  printf("\t'f%%d': fast forward, %%d is speed, will choose I frame only mode\n");
  printf("\t'b%%d': fast backward, %%d is speed, will choose I frame only mode\n");
  printf("\t'F%%d': fast forward from file's begining\n");
  printf("\t'B%%d': fast backward from file's end\n");
  printf("\t'r': resume to 1x forward normal playback. (from current position)\n");
  printf("\t'R': resume to 1x forward normal playback. (from file's begining)\n");
}

static int __replace_enter(char *p_buffer, unsigned int size)
{
  while (size) {
    if (('\n') == (*p_buffer)) {
      *p_buffer = 0x0;
      return 0;
    }
    p_buffer ++;
    size --;
  }
  printf("no enter found, should be error\n");
  return 1;
}

static void __safe_resume(STestOryxPlaybackNewContext *context, IGenericEngineControl *engine, TGenericID pipelind_id)
{
  if ((context->b_playback_pasued) || (context->b_playback_step)) {
    SUserParamResume resume;
    resume.check_field = EGenericEngineConfigure_Resume;
    resume.component_id = pipelind_id;
    engine->generic_control(EGenericEngineConfigure_Resume, &resume);
    context->b_playback_pasued = 0;
    context->b_playback_step = 0;
  }
}

static int __test_playback(STestOryxPlaybackNewContext *context, IGenericEngineControl *engine)
{
  EECode err = EECode_OK;
  int ret = 0;
  int playback_rtsp = 0;
  TGenericID demuxer_id = 0, video_decoder_id = 0, audio_decoder_id = 0, video_renderer_id = 0, audio_renderer_id = 0;
  TGenericID connection_id = 0;
  TGenericID playback_pipeline_id = 0;

  if (!strncmp(context->playback_url, "rtsp://", strlen("rtsp://"))) {
    playback_rtsp = 1;
  }

  err = engine->begin_config_process_pipeline();
  if (playback_rtsp) {
    err = engine->new_component(EGenericComponentType_Demuxer, demuxer_id, "RTSP");
  } else {
    err = engine->new_component(EGenericComponentType_Demuxer, demuxer_id, "PRMP4");
  }

  if (context->use_vd_dsp) {
    err = engine->new_component(EGenericComponentType_VideoDecoder, video_decoder_id, "AMBA");
  } else if (context->use_vd_ffmpeg) {
    err = engine->new_component(EGenericComponentType_VideoDecoder, video_decoder_id, "FFMpeg");
  } else {
    printf("do not sepcify video decoder?\n");
    return (-1);
  }

  if (context->use_vo_dsp) {
    err = engine->new_component(EGenericComponentType_VideoRenderer, video_renderer_id, "AMBA");
  } else if (context->use_vo_linuxfb) {
    err = engine->new_component(EGenericComponentType_VideoRenderer, video_renderer_id, "LinuxFB");
  } else {
    printf("do not sepcify video output?\n");
    return (-1);
  }

  if (!context->b_disable_audio) {
    err = engine->new_component(EGenericComponentType_AudioDecoder, audio_decoder_id, "LIBAAC");
    err = engine->new_component(EGenericComponentType_AudioRenderer, audio_renderer_id, "ALSA");
  } else {
    audio_decoder_id = 0;
    audio_renderer_id = 0;
  }
  err = engine->connect_component(connection_id, demuxer_id, video_decoder_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect demuxer and video decoder fail\n");
    return (-1);
  }
  err = engine->connect_component(connection_id, video_decoder_id, video_renderer_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video decoder and video renderer fail\n");
    return (-1);
  }
  if (!context->b_disable_audio) {
    err = engine->connect_component(connection_id, demuxer_id, audio_decoder_id,  StreamType_Audio);
    if (EECode_OK != err) {
      printf("connect demuxer and audio decoder fail\n");
      return (-1);
    }
    err = engine->connect_component(connection_id, audio_decoder_id, audio_renderer_id,  StreamType_Audio);
    if (EECode_OK != err) {
      printf("connect audio decoder and audio renderer fail\n");
      return (-1);
    }
  }
  err = engine->setup_playback_pipeline(playback_pipeline_id, demuxer_id, demuxer_id, video_decoder_id, audio_decoder_id, video_renderer_id, audio_renderer_id);
  if (EECode_OK != err) {
    printf("setup playback pipeline fail\n");
    return (-1);
  }

  g_test_playback_pipeline_id = playback_pipeline_id;

  SConfigVout config_vout;
  config_vout.check_field = EGenericEngineConfigure_ConfigVout;
  config_vout.b_digital_vout = context->b_digital_vout;
  config_vout.b_hdmi_vout = context->b_hdmi_vout;
  config_vout.b_cvbs_vout = context->b_cvbs_vout;
  printf("[vout config]: digital %d, hdmi %d, cvbs %d\n", config_vout.b_digital_vout, config_vout.b_hdmi_vout, config_vout.b_cvbs_vout);
  err = engine->generic_control(EGenericEngineConfigure_ConfigVout, &config_vout);

  if (playback_rtsp && context->b_rtsp_tcp_mode) {
    err = engine->generic_control(EGenericEngineConfigure_RTSPClientTryTCPModeFirst, NULL);
  }

  if (context->audio_device[0]) {
    SConfigAudioDevice audio_device;
    audio_device.check_field = EGenericEngineConfigure_ConfigAudioDevice;
    snprintf(audio_device.audio_device_name, sizeof(audio_device.audio_device_name), "%s", context->audio_device);
    printf("[audio device]: %s\n", audio_device.audio_device_name);
    err = engine->generic_control(EGenericEngineConfigure_ConfigAudioDevice, &audio_device);
  }

  if (context->b_enable_fffbbw) {
    err = engine->generic_control(EGenericEngineConfigure_EnableFastFWFastBWBackwardPlayback, NULL);
  } else {
    err = engine->generic_control(EGenericEngineConfigure_DisableFastFWFastBWBackwardPlayback, NULL);
  }

  err = engine->finalize_config_process_pipeline();
  if (EECode_OK != err) {
    printf("finalize_config_process_pipeline fail\n");
    return (-1);
  }
  err = engine->set_source_url(demuxer_id, context->playback_url);
  if (EECode_OK != err) {
    printf("fiinalize_config_process_pipeline fail\n");
    return (-1);
  }
  err = engine->run_processing();
  if (EECode_OK != err) {
    printf("run_processing fail\n");
    return (-1);
  }
  err = engine->start();
  if (EECode_OK != err) {
    printf("Start fail\n");
    return (-1);
  }
  char buffer_old[128] = {0};
  char buffer[128] = {0};
  char *p_buffer = buffer;
  int flag_stdin = 0;
  flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
  if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
    printf("stdin_fileno set error.\n");
  }
  //process user cmd line
  while (g_test_oryx_playback_new_running) {
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
      if (ret) {
        printf("no enter found\n");
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
        g_test_engine_control = NULL;
        g_test_media_context = NULL;
        g_test_playback_pipeline_id = 0;
        if ((context->b_playback_pasued) || (context->b_playback_step)) {
          printf("safe resume before seek\n");
          __safe_resume(context, engine, playback_pipeline_id);
        }
        g_test_oryx_playback_new_running = 0;
        break;
      case 'p':
        engine->print_current_status();
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
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_Seek, &seek);
        }
        break;
      case 'f': {
          SPbFeedingRule ff;
          unsigned int speed;
          sscanf(p_buffer + 1, "%d", &speed);
          ff.check_field = EGenericEngineConfigure_FastForward;
          ff.component_id = playback_pipeline_id;
          ff.direction = 0;
          ff.feeding_rule = DecoderFeedingRule_IDROnly;
          ff.speed = speed << 8;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before fast fw\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_FastForward, &ff);
        }
        break;
      case 'b': {
          SPbFeedingRule fb;
          unsigned int speed;
          sscanf(p_buffer + 1, "%d", &speed);
          fb.check_field = EGenericEngineConfigure_FastBackward;
          fb.component_id = playback_pipeline_id;
          fb.direction = 1;
          fb.feeding_rule = DecoderFeedingRule_IDROnly;
          fb.speed = speed << 8;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before fast bw\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_FastBackward, &fb);
        }
        break;
      case 'F': {
          SPbFeedingRule ff;
          unsigned int speed;
          sscanf(p_buffer + 1, "%d", &speed);
          ff.check_field = EGenericEngineConfigure_FastForwardFromBegin;
          ff.component_id = playback_pipeline_id;
          ff.direction = 0;
          ff.feeding_rule = DecoderFeedingRule_IDROnly;
          ff.speed = speed << 8;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before fast fw\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_FastForwardFromBegin, &ff);
        }
        break;
      case 'B': {
          SPbFeedingRule fb;
          unsigned int speed;
          sscanf(p_buffer + 1, "%d", &speed);
          fb.check_field = EGenericEngineConfigure_FastBackwardFromEnd;
          fb.component_id = playback_pipeline_id;
          fb.direction = 1;
          fb.feeding_rule = DecoderFeedingRule_IDROnly;
          fb.speed = speed << 8;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before fast fb\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_FastBackwardFromEnd, &fb);
        }
        break;
      case 'r': {
          SResume1xPlayback r;
          r.check_field = EGenericEngineConfigure_Resume1xFromCurrent;
          r.component_id = playback_pipeline_id;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before return to normal playback\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_Resume1xFromCurrent, &r);
        }
        break;
      case 'R': {
          SResume1xPlayback r;
          r.check_field = EGenericEngineConfigure_Resume1xFromBegin;
          r.component_id = playback_pipeline_id;
          if ((context->b_playback_pasued) || (context->b_playback_step)) {
            printf("safe resume before return to normal playback\n");
            __safe_resume(context, engine, playback_pipeline_id);
          }
          err = engine->generic_control(EGenericEngineConfigure_Resume1xFromBegin, &r);
        }
        break;
      case ' ': {
          if ((!context->b_playback_pasued) && (!context->b_playback_step)) {
            SUserParamPause pause;
            pause.check_field = EGenericEngineConfigure_Pause;
            pause.component_id = playback_pipeline_id;
            err = engine->generic_control(EGenericEngineConfigure_Pause, &pause);
            context->b_playback_pasued = 1;
          } else {
            SUserParamResume resume;
            resume.check_field = EGenericEngineConfigure_Resume;
            resume.component_id = playback_pipeline_id;
            err = engine->generic_control(EGenericEngineConfigure_Resume, &resume);
            context->b_playback_pasued = 0;
            context->b_playback_step = 0;
          }
          if (EECode_OK != err) {
            printf("EUserParamType_PlaybackTrickplay(pause resume) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            break;
          }
        }
        break;
      case 's': {
          SUserParamResume step;
          step.check_field = EGenericEngineConfigure_StepPlay;
          step.component_id = playback_pipeline_id;
          err = engine->generic_control(EGenericEngineConfigure_StepPlay, &step);
          context->b_playback_step = 1;
          if (EECode_OK != err) {
            printf("EUserParamType_PlaybackTrickplay(step) fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
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
    printf("stdin_fileno set error\n");
  }
  err = engine->stop();
  if (EECode_OK != err) {
    printf("stop fail\n");
    return (-1);
  }
  err = engine->exit_processing();
  if (EECode_OK != err) {
    printf("exit_processing fail\n");
    return (-1);
  }
  return 0;
}

static int __init_test_oryx_playback_new_params(int argc, char **argv, STestOryxPlaybackNewContext *context)
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
        printf("[input argument] -p: should follow playback filename.\n");
      }
    } else if (!strcmp("--enableaudio", argv[i])) {
      context->b_disable_audio = 0;
    } else if (!strcmp("--disableaudio", argv[i])) {
      context->b_disable_audio = 1;
    } else if (!strncmp("-V", argv[i], strlen("-V"))) {
      int len = strlen(argv[i]);
      if (len == strlen("-V")) {
        if ((i + 2) < argc) {
          len = strlen(argv[i + 1]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument] -V: vout mode string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout1_mode_string, argv[i + 1]);
          len = strlen(argv[i + 2]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument]: vout device string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout1_sinktype_string, argv[i + 2] + 2);
          i += 2;
        } else {
          printf("[input argument] -V: should follow video mode and video device.\n");
          return (-1);
        }
      } else {
        if ((i + 1) < argc) {
          len = strlen(argv[i]) - 2;
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument] -V: vout mode string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout1_mode_string, argv[i] + 2);
          len = strlen(argv[i + 1]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument]: vout device string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout1_sinktype_string, argv[i + 1] + 2);
          i ++;
        } else {
          printf("[input argument] -V: should follow video mode and video device.\n");
          return (-1);
        }
      }
      printf("[input argument] -V: %s, %s.\n", context->vout1_mode_string, context->vout1_sinktype_string);
      context->b_specify_vout1 = 1;
      if (!strcmp("digital", context->vout1_sinktype_string)) {
        context->b_digital_vout = 1;
      } else if (!strcmp("hdmi", context->vout1_sinktype_string)) {
        context->b_hdmi_vout = 1;
      } else if (!strcmp("cvbs", context->vout1_sinktype_string)) {
        context->b_cvbs_vout = 1;
      } else {
        printf("not known vout1 sink type %s\n", context->vout1_sinktype_string);
      }
    } else if (!strncmp("-v", argv[i], strlen("-v"))) {
      int len = strlen(argv[i]);
      if (len == strlen("-v")) {
        if ((i + 2) < argc) {
          len = strlen(argv[i + 1]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument] -v: vout mode string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout0_mode_string, argv[i + 1]);
          len = strlen(argv[i + 2]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument]: vout device string name length too long, %d\n", len);
            return (-1);
          }
          if (!strcmp(argv[i + 2], "--lcd")) {
            if ((i + 3) < argc) {
              len = strlen(argv[i + 3]);
              if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
                printf("[input argument]: lcd device string name length too long, %d\n", len);
                return (-1);
              }
              strcpy(context->vout0_device_string, argv[i + 3]);
            } else {
              printf("[input argument] --lcd: should with lcd device name\n");
              return (-1);
            }
            strcpy(context->vout0_sinktype_string, "digital");
            i += 3;
          } else {
            len = strlen(argv[i + 1]);
            if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
              printf("[input argument]: vout device string name length too long, %d\n", len);
              return (-1);
            }
            strcpy(context->vout0_sinktype_string, argv[i + 1] + 2);
            i += 2;
          }
        } else {
          printf("[input argument] -v: should follow video mode and video device.\n");
          return (-1);
        }
      } else {
        if ((i + 1) < argc) {
          len = strlen(argv[i]) - 2;
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument] -v: vout mode string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(context->vout0_mode_string, argv[i] + 2);
          if (!strcmp(argv[i + 1], "--lcd")) {
            if ((i + 2) < argc) {
              len = strlen(argv[i + 2]);
              if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
                printf("[input argument]: lcd device string name length too long, %d\n", len);
                return (-1);
              }
              strcpy(context->vout0_device_string, argv[i + 2]);
            } else {
              printf("[input argument] --lcd: should with lcd device name\n");
              return (-1);
            }
            strcpy(context->vout0_sinktype_string, "digital");
            i += 2;
          } else {
            len = strlen(argv[i + 1]);
            if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
              printf("[input argument]: vout device string name length too long, %d\n", len);
              return (-1);
            }
            strcpy(context->vout0_sinktype_string, argv[i + 1] + 2);
            i ++;
          }
        } else {
          printf("[input argument] -v: should follow video mode and video device.\n");
          return (-1);
        }
      }
      printf("[input argument] -v: %s, %s, %s.\n", context->vout0_mode_string, context->vout0_sinktype_string, context->vout0_device_string);
      context->b_specify_vout0 = 1;
      if (!strcmp("digital", context->vout0_sinktype_string)) {
        context->b_digital_vout = 1;
      } else if (!strcmp("hdmi", context->vout0_sinktype_string)) {
        context->b_hdmi_vout = 1;
      } else if (!strcmp("cvbs", context->vout0_sinktype_string)) {
        context->b_cvbs_vout = 1;
      } else {
        printf("not known vout0 sink type %s\n", context->vout0_sinktype_string);
      }
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
    } else if (!strcmp("--rtsp-tcp", argv[i])) {
      context->b_rtsp_tcp_mode = 1;
    } else if (!strcmp("--rtsp-udp", argv[i])) {
      context->b_rtsp_tcp_mode = 0;
    } else if (!strcmp("--disableff", argv[i])) {
      context->b_enable_fffbbw = 0;
    } else if (!strcmp("--enableff", argv[i])) {
      context->b_enable_fffbbw = 1;
    } else if (!strcmp("--audiodevice", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(context->audio_device, 32, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] --audiodevice: %s.\n", context->audio_device);
      } else {
        printf("[input argument] --audiodevice: should follow audio device name.\n");
      }
    } else if (!strcmp("--loglevel", argv[i])) {
      if ((i + 1) < argc) {
        int v = 0;
        int ret = sscanf(argv[i + 1], "%d", &v);
        if (1 == ret) {
          context->log_level = v;
          printf("[input argument] --loglevel: %d.\n", v);
        } else {
          printf("[input argument] --loglevel: should follow 'log level'.\n");
          return (-1);
        }
        i ++;
      } else {
        printf("[input argument] --loglevel: should follow 'log level'.\n");
        return (-1);
      }
    } else if (!strcmp("--log2file", argv[i])) {
      context->b_log_to_file = 1;
      printf("[input argument] --log2file: log to file\n");
    } else if (!strcmp("--help", argv[i])) {
      __print_ut_options();
      __print_ut_cmds();
    } else {
      printf("error: NOT processed option(%s).\n", argv[i]);
      __print_ut_options();
      __print_ut_cmds();
      return (-1);
    }
  }
  return 0;
}

static void __test_oryx_playback_new_sig_stop(int a)
{
  if ((g_test_media_context->b_playback_pasued) || (g_test_media_context->b_playback_step)) {
    printf("safe resume before seek\n");
    if (g_test_media_context && g_test_engine_control && g_test_playback_pipeline_id) {
      __safe_resume(g_test_media_context, g_test_engine_control, g_test_playback_pipeline_id);
      g_test_engine_control = NULL;
      g_test_media_context = NULL;
      g_test_playback_pipeline_id = 0;
    }
  }
  g_test_oryx_playback_new_running = 0;
}

int main(int argc, char **argv)
{
  STestOryxPlaybackNewContext context;
  int ret = 0;
  IGenericEngineControl *engine_control = NULL;
  signal(SIGINT, __test_oryx_playback_new_sig_stop);
  signal(SIGQUIT, __test_oryx_playback_new_sig_stop);
  signal(SIGTERM, __test_oryx_playback_new_sig_stop);
  if (argc > 1) {
    memset(&context, 0x0, sizeof(context));
    context.b_disable_audio = 1;
    context.b_rtsp_tcp_mode = 1;
    context.b_enable_fffbbw = 1;
    ret = __init_test_oryx_playback_new_params(argc, argv, &context);
    if (ret) {
      return (-2);
    }
  } else {
    __print_ut_options();
    __print_ut_cmds();
    return 0;
  }
  if ((!context.b_digital_vout) && (!context.b_hdmi_vout) && (!context.b_cvbs_vout)) {
    printf("you should specify vout device\n");
    __print_ut_options();
    return (-3);
  }

  if (context.log_level) {
    gfPreSetAllLogLevel((ELogLevel) context.log_level);
  }

  if (context.b_log_to_file) {
    gfOpenLogFile((TChar *) "test_media.log");
  }

  if (context.b_specify_vout0) {
    SVideoOutputConfigure config;

    memset(&config, 0x0, sizeof(config));
    config.vout_id = 0;
    config.mode_string = context.vout0_mode_string;
    config.sink_type_string = context.vout0_sinktype_string;
    config.device_string = context.vout0_device_string;

    if (!strcmp(gfGetDSPPlatformName(), "S2L")) {
      config.b_config_mixer = 1;
      config.mixer_flag = 1;
    }
    printf("platform %s: b_config_mixer %d, mixer_flag %d\n", gfGetDSPPlatformName(), config.b_config_mixer, config.mixer_flag);

    EECode err = gfAmbaDSPConfigureVout(&config);
    if (EECode_OK != err) {
      printf("config vout0 fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return (-1);
    }
  }

  if (context.b_specify_vout1) {
    SVideoOutputConfigure config;

    memset(&config, 0x0, sizeof(config));
    config.vout_id = 1;
    config.mode_string = context.vout1_mode_string;
    config.sink_type_string = context.vout1_sinktype_string;
    config.device_string = context.vout1_device_string;

    if (!strcmp(gfGetDSPPlatformName(), "S2L")) {
      config.b_config_mixer = 1;
      config.mixer_flag = 1;
    }
    printf("platform %s: b_config_mixer %d, mixer_flag %d\n", gfGetDSPPlatformName(), config.b_config_mixer, config.mixer_flag);

    EECode err = gfAmbaDSPConfigureVout(&config);
    if (EECode_OK != err) {
      printf("config vout1 fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return (-1);
    }
  }

  engine_control = CreateGenericMediaEngine();
  if (!engine_control) {
    printf("CreateGenericMediaEngine() fail\n");
    return (-1);
  }

  g_test_engine_control = engine_control;
  g_test_media_context = &context;
  g_test_playback_pipeline_id = 0;

  if (context.b_playback) {
    if ((!context.use_vd_dsp) && (!context.use_vd_ffmpeg)) {
      printf("use dsp video decoder as default\n");
      context.use_vd_dsp = 1;
      context.use_vo_dsp = 1;
    }
    __test_playback(&context, engine_control);
    g_test_engine_control = NULL;
    g_test_media_context = NULL;
    g_test_playback_pipeline_id = 0;
  }
  engine_control->destroy();
  return 0;
}

