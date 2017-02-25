/**
 * test_oryx_video_edit.cpp
 *
 * History:
 *    2016/04/28 - [Zhi He] create file
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

typedef struct {
  unsigned char b_ve_es_2_yuv;
  unsigned char b_ve_es_2_es_efm;
  unsigned char b_ve_es_2_mp4_efm;
  unsigned char reserved0;

  char ve_input_file[DMAX_UT_URL_LENGTH];
  char ve_output_file[DMAX_UT_URL_LENGTH];
} STestVideoEditContext;

static int g_test_video_edit_running = 1;

static void __print_test_video_edit_options()
{
  printf("test_oryx_video_edit options:\n");
  printf("\t'--ve-es2yuv': video edit, decode es to yuv file\n");
  printf("\t'--ve-es2es-efm': video edit, decode es (mjpeg, h264, h265) and re-encode to es(h264), via efm\n");
  printf("\t'--ve-es2mp4-efm': video edit, decode es (mjpeg, h264, h265) and re-encode to es(h264), via efm, then mux to mp4\n");
  printf("\t'-i [filename]': '-i' specify input file name, for video edit\n");
  printf("\t'-o [filename]': '-o' specify output file name, for video edit\n");
  printf("\t'--help': print help\n\n");
}

static void __print_test_video_edit_cmds()
{
  printf("test_oryx_video_edit runtime cmds: press cmd + Enter\n");
  printf("\t'q': Quit unit test\n");
  printf("\t'p': Print status\n");
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

static int __test_edit_es_2_yuv(STestVideoEditContext *context, IGenericEngineControl *engine)
{
  EECode err = EECode_OK;
  int ret = 0;
  TGenericID demuxer_id = 0, video_decoder_id = 0, video_raw_sink_id = 0;
  TGenericID connection_id = 0;

  err = engine->begin_config_process_pipeline();

  err = engine->new_component(EGenericComponentType_Demuxer, demuxer_id, DNameTag_PrivateVideoES);

  err = engine->new_component(EGenericComponentType_VideoDecoder, video_decoder_id, DNameTag_FFMpeg);

  err = engine->new_component(EGenericComponentType_VideoRawSink, video_raw_sink_id, "");

  err = engine->connect_component(connection_id, demuxer_id, video_decoder_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect demuxer and video decoder fail\n");
    return (-1);
  }

  err = engine->connect_component(connection_id, video_decoder_id, video_raw_sink_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video decoder and video raw sink fail\n");
    return (-1);
  }

  err = engine->finalize_config_process_pipeline();
  if (EECode_OK != err) {
    printf("finalize_config_process_pipeline fail\n");
    return (-1);
  }

  err = engine->set_source_url(demuxer_id, context->ve_input_file);
  if (EECode_OK != err) {
    printf("set_source_url fail\n");
    return (-1);
  }

  err = engine->set_sink_url(video_raw_sink_id, context->ve_output_file);
  if (EECode_OK != err) {
    printf("set_sink_url fail\n");
    return (-1);
  }

  err = engine->run_processing();
  if (EECode_OK != err) {
    printf("run_processing fail\n");
    return (-1);
  }

  err = engine->start();
  if (EECode_OK != err) {
    printf("start fail\n");
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
  while (g_test_video_edit_running) {
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
        g_test_video_edit_running = 0;
        break;

      case 'p':
        engine->print_current_status();
        break;

      case 'h':   // help
        __print_test_video_edit_options();
        __print_test_video_edit_cmds();
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

static int __test_edit_es_2_es_efm(STestVideoEditContext *context, IGenericEngineControl *engine)
{
  EECode err = EECode_OK;
  int ret = 0;
  TGenericID demuxer_id = 0, video_decoder_id = 0, video_efm_injecter_id = 0, video_encoder_id = 0, video_es_sink_id;
  TGenericID connection_id = 0;

  err = engine->begin_config_process_pipeline();

  err = engine->new_component(EGenericComponentType_Demuxer, demuxer_id, DNameTag_PrivateVideoES);

  err = engine->new_component(EGenericComponentType_VideoDecoder, video_decoder_id, DNameTag_FFMpeg);

  err = engine->new_component(EGenericComponentType_VideoInjecter, video_efm_injecter_id, DNameTag_AMBAEFM);

  err = engine->new_component(EGenericComponentType_VideoEncoder, video_encoder_id, DNameTag_AMBA);

  err = engine->new_component(EGenericComponentType_VideoESSink, video_es_sink_id, "");

  err = engine->connect_component(connection_id, demuxer_id, video_decoder_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect demuxer and video decoder fail\n");
    return (-1);
  }

  err = engine->connect_component(connection_id, video_decoder_id, video_efm_injecter_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video decoder and video efm injecter fail\n");
    return (-1);
  }

  err = engine->connect_component(connection_id, video_encoder_id, video_es_sink_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video encoder and video es sink fail\n");
    return (-1);
  }

  err = engine->finalize_config_process_pipeline();
  if (EECode_OK != err) {
    printf("finalize_config_process_pipeline fail\n");
    return (-1);
  }

  err = engine->set_source_url(demuxer_id, context->ve_input_file);
  if (EECode_OK != err) {
    printf("set_source_url fail\n");
    return (-1);
  }

  err = engine->set_sink_url(video_es_sink_id, context->ve_output_file);
  if (EECode_OK != err) {
    printf("set_sink_url fail\n");
    return (-1);
  }

  err = engine->run_processing();
  if (EECode_OK != err) {
    printf("run_processing fail\n");
    return (-1);
  }

  err = engine->start();
  if (EECode_OK != err) {
    printf("start fail\n");
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
  while (g_test_video_edit_running) {
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
        g_test_video_edit_running = 0;
        break;

      case 'p':
        engine->print_current_status();
        break;

      case 'h':   // help
        __print_test_video_edit_options();
        __print_test_video_edit_cmds();
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

static int __test_edit_es_2_mp4_efm(STestVideoEditContext *context, IGenericEngineControl *engine)
{
  EECode err = EECode_OK;
  int ret = 0;
  TGenericID demuxer_id = 0, video_decoder_id = 0, video_efm_injecter_id = 0, video_encoder_id = 0, muxer_id = 0;
  TGenericID connection_id = 0;

  err = engine->begin_config_process_pipeline();

  err = engine->new_component(EGenericComponentType_Demuxer, demuxer_id, DNameTag_PrivateVideoES);

  err = engine->new_component(EGenericComponentType_VideoDecoder, video_decoder_id, DNameTag_FFMpeg);

  err = engine->new_component(EGenericComponentType_VideoInjecter, video_efm_injecter_id, DNameTag_AMBAEFM);

  err = engine->new_component(EGenericComponentType_VideoEncoder, video_encoder_id, DNameTag_AMBA);

  err = engine->new_component(EGenericComponentType_Muxer, muxer_id, DNameTag_PrivateMP4);

  err = engine->connect_component(connection_id, demuxer_id, video_decoder_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect demuxer and video decoder fail\n");
    return (-1);
  }

  err = engine->connect_component(connection_id, video_decoder_id, video_efm_injecter_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video decoder and video efm injecter fail\n");
    return (-1);
  }

  err = engine->connect_component(connection_id, video_encoder_id, muxer_id,  StreamType_Video);
  if (EECode_OK != err) {
    printf("connect video efm encoder and muxer fail\n");
    return (-1);
  }

  err = engine->finalize_config_process_pipeline();
  if (EECode_OK != err) {
    printf("finalize_config_process_pipeline fail\n");
    return (-1);
  }

  err = engine->set_source_url(demuxer_id, context->ve_input_file);
  if (EECode_OK != err) {
    printf("set_source_url fail\n");
    return (-1);
  }

  err = engine->set_sink_url(muxer_id, context->ve_output_file);
  if (EECode_OK != err) {
    printf("set_sink_url fail\n");
    return (-1);
  }

  err = engine->run_processing();
  if (EECode_OK != err) {
    printf("run_processing fail\n");
    return (-1);
  }

  err = engine->start();
  if (EECode_OK != err) {
    printf("start fail\n");
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
  while (g_test_video_edit_running) {
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
        g_test_video_edit_running = 0;
        break;

      case 'p':
        engine->print_current_status();
        break;

      case 'h':   // help
        __print_test_video_edit_options();
        __print_test_video_edit_cmds();
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

static int __init_test_video_edit_params(int argc, char **argv, STestVideoEditContext *context)
{
  int i = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp("-i", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(context->ve_input_file, DMAX_UT_URL_LENGTH, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] -i: url %s.\n", context->ve_input_file);
      } else {
        printf("[input argument] -i: should follow input file.\n");
      }
    } else if (!strcmp("-o", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(context->ve_output_file, DMAX_UT_URL_LENGTH, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] -o: url %s.\n", context->ve_input_file);
      } else {
        printf("[input argument] -o: should follow output file.\n");
      }
    } else if (!strcmp("--ve-es2yuv", argv[i])) {
      context->b_ve_es_2_yuv = 1;
    } else if (!strcmp("--ve-es2es-efm", argv[i])) {
      context->b_ve_es_2_es_efm = 1;
    } else if (!strcmp("--ve-es2mp4-efm", argv[i])) {
      context->b_ve_es_2_mp4_efm = 1;
    } else if (!strcmp("--help", argv[i])) {
      __print_test_video_edit_options();
      __print_test_video_edit_cmds();
    } else {
      printf("error: NOT processed option(%s).\n", argv[i]);
      __print_test_video_edit_options();
      __print_test_video_edit_cmds();
      return (-1);
    }
  }
  return 0;
}

static void __test_video_edit_sig_stop(int a)
{
  g_test_video_edit_running = 0;
}

int main(int argc, char **argv)
{
  STestVideoEditContext context;
  int ret = 0;
  IGenericEngineControl *engine_control = NULL;
  signal(SIGINT, __test_video_edit_sig_stop);
  signal(SIGQUIT, __test_video_edit_sig_stop);
  signal(SIGTERM, __test_video_edit_sig_stop);
  if (argc > 1) {
    memset(&context, 0x0, sizeof(context));
    ret = __init_test_video_edit_params(argc, argv, &context);
    if (ret) {
      return (-2);
    }
  } else {
    __print_test_video_edit_options();
    __print_test_video_edit_cmds();
    return 0;
  }

  engine_control = CreateGenericMediaEngine();
  if (!engine_control) {
    printf("CreateGenericMediaEngine() fail\n");
    return (-1);
  }

  if (context.b_ve_es_2_yuv) {
    __test_edit_es_2_yuv(&context, engine_control);
  } else if (context.b_ve_es_2_es_efm) {
    __test_edit_es_2_es_efm(&context, engine_control);
  } else if (context.b_ve_es_2_mp4_efm) {
    __test_edit_es_2_mp4_efm(&context, engine_control);
  }

  engine_control->destroy();
  return 0;
}

