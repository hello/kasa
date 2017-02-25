/*
 * test_playback_service_air_api.h
 *
 * @Author: Zhi He
 * @Email : zhe@ambarella.com
 * @Time  : 18/04/2016 [Created]
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

#include <signal.h>
#include "am_base_include.h"
//#include "am_log.h"

#include "am_api_helper.h"
#include "am_api_playback.h"

#define DMAX_UT_VOUT_STRING_LENGTH 32

static bool g_pb_service_running = true;

static void sigstop (int32_t arg)
{
  printf ("Signal has been captured, test_playback_service_air_api quits!\n");
  g_pb_service_running = false;
}

typedef struct {
    const char  *name;
    int width;
    int height;
} _vout_resolution_t;

typedef struct {
  const char  *model;
} _lcd_mode_t;

static _vout_resolution_t __g_vout_resolution_list[] = {
    //Typically for Analog and HDMI
    {"480i", 720, 480},
    {"576i", 720, 576},
    {"480p", 720, 480},
    {"576p", 720, 576},
    {"720p", 1280, 720},
    {"720p50", 1280, 720},
    {"720p30", 1280, 720},
    {"720p25", 1280, 720},
    {"720p24", 1280, 720},
    {"1080i", 1920, 1080},
    {"1080i50", 1920, 1080},
    {"1080p24", 1920, 1080},
    {"1080p25", 1920, 1080},
    {"1080p30", 1920, 1080},
    {"1080p", 1920, 1080},
    {"1080p50", 1920, 1080},
    {"2160p30", 3840, 2160},
    {"2160p25", 3840, 2160},
    {"2160p24", 3840, 2160},
    {"2160p24se", 4096, 2160},

    // Typically for LCD
    {"D480I", 720, 480},
    {"D576I", 720, 576},
    {"D480P", 720, 480},
    {"D576P", 720, 576},
    {"D720P", 1280, 720},
    {"D720P50", 1280, 720},
    {"D1080I", 1920, 1080},
    {"D1080I50", 1920, 1080},
    {"D1080P24", 1920, 1080},
    {"D1080P25", 1920, 1080},
    {"D1080P30", 1920, 1080},
    {"D1080P", 1920, 1080},
    {"D1080P50", 1920, 1080},
    {"D960x240", 960, 240},    //AUO27
    {"D320x240", 320, 240},    //AUO27
    {"D320x288", 320, 288},    //AUO27
    {"D360x240", 360, 240},    //AUO27
    {"D360x288", 360, 288},    //AUO27
    {"D480x640", 480, 640},    //P28K
    {"D480x800", 480, 800},    //TPO648
    {"hvga", 320, 480},   //TPO489
    {"vga", 640, 480},
    {"wvga",	 800, 480}, //TD043
    {"D240x320", 240, 320},    //ST7789V
    {"D240x400", 240, 400},    //WDF2440
    {"xga", 1024, 768},   //EJ080NA
    {"wsvga", 1024, 600},    //AT070TNA2
    {"D960x540", 960, 540},    //E330QHD
};

static void __print_available_video_output_mode()
{
    unsigned int i;
    printf("Available video output mode:\n");
    for (i = 0; i < sizeof(__g_vout_resolution_list) / sizeof(__g_vout_resolution_list[0]); i++) {
        printf("\t%s:\t\t%dx%d\n", __g_vout_resolution_list[i].name, __g_vout_resolution_list[i].width, __g_vout_resolution_list[i].height);
    }
}

static _lcd_mode_t g_lcd_mode_list[] = {
    {"digital"},
    {"digital601"},
    {"td043"},
    {"td043_16"},
    {"st7789v"},
    {"ili8961"},
};

static void __print_possible_lcd_model()
{
    unsigned int i;
    printf("possible LCD Model:\n");
    for (i = 0; i < sizeof(g_lcd_mode_list) / sizeof(g_lcd_mode_list[0]); i++) {
        printf("\t%s.\n", g_lcd_mode_list[i].model);
    }
}

static void __print_ut_options()
{
  printf ("\n=============== oryx playback service test =================\n\n");
  printf("\t'-p [filename]': '-p' specify playback file name\n");
  printf("\t'--vd-dsp': select amba dsp video decoder\n");
  printf("\t'--vd-ffmpeg': select ffmpeg video decoder\n");
  printf("\t'--vo-dsp': select dsp video output\n");
  printf("\t'--vo-fb': select linux fb video output\n");
  printf("\t'--rtsp-tcp': choose TCP mode for RTSP\n");
  printf("\t'--rtsp-udp': choose UDP mode for RTSP\n");
  printf("\t'--digital': choose digital vout\n");
  printf("\t'--hdmi': choose hdmi vout\n");
  printf("\t'--cvbs': choose cvbs vout\n");
  printf("\t'-V': specify HDMI or CVBS video output mode\n");
  printf("\t'-v': specify Digital (LCD) video output mode\n");
  __print_available_video_output_mode();
  __print_possible_lcd_model();
  printf("\t'--help': print help\n\n");
  printf ("\n=========================================================\n\n");
}

static void __print_ut_cmds()
{
  printf("test_playback_service_air_api runtime cmds: press cmd + Enter\n");
  printf("\t'q': Quit unit test\n");
  printf("\t'g%%d': Seek to %%d ms\n");
  printf("\t' ': pause/resume\n");
  printf("\t's': step play\n");
  printf("\t'f%%d': fast forward, %%d is speed, will choose I frame only mode\n");
  printf("\t'b%%d': fast backward, %%d is speed, will choose I frame only mode\n");
  printf("\t'F%%d': fast forward from file's begining\n");
  printf("\t'B%%d': fast backward from file's end\n");
}

struct test_playack_service_air_api_setting_t {
  unsigned char use_digital;
  unsigned char use_hdmi;
  unsigned char use_cvbs;
  unsigned char rtsp_tcp_mode;

  unsigned char use_demuxer_native_mp4;
  unsigned char use_demuxer_native_rtsp;
  unsigned char use_demuxer_ffmpeg;
  unsigned char reserved0;

  unsigned char use_vd_dsp;
  unsigned char use_vd_ffmpeg;
  unsigned char use_vo_dsp;
  unsigned char use_vo_linuxfb;

  unsigned char b_specify_vout0;
  unsigned char b_specify_vout1;
  unsigned char reserved1;
  unsigned char reserved2;

  char vout0_mode_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_mode_string[DMAX_UT_VOUT_STRING_LENGTH];

  char vout0_sinktype_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_sinktype_string[DMAX_UT_VOUT_STRING_LENGTH];

  char vout0_device_string[DMAX_UT_VOUT_STRING_LENGTH];
  char vout1_device_string[DMAX_UT_VOUT_STRING_LENGTH];
};

static int __init_test_playback_air_api_params(int argc, char **argv, am_playback_context_t *ctx, test_playack_service_air_api_setting_t *setting)
{
  int i = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp("-p", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(ctx->url, AM_PLAYBACK_MAX_URL_LENGTH, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] -p: playback %s.\n", ctx->url);
      } else {
        printf("[input argument] -p: should follow playback filename.\n");
      }
    } else if (!strcmp("--digital", argv[i])) {
      setting->use_digital = 1;
      printf("[input argument] --digital\n");
    } else if (!strcmp("--hdmi", argv[i])) {
      setting->use_hdmi = 1;
      printf("[input argument] --hdmi\n");
    } else if (!strcmp("--cvbs", argv[i])) {
      setting->use_cvbs = 1;
      printf("[input argument] --cvbs\n");
    } else if (!strcmp("--demuxer-mp4", argv[i])) {
      setting->use_demuxer_native_mp4 = 1;
      printf("[input argument] --demuxer-mp4\n");
    } else if (!strcmp("--demuxer-rtsp", argv[i])) {
      setting->use_demuxer_native_rtsp = 1;
      printf("[input argument] --demuxer-rtsp\n");
    } else if (!strcmp("--demuxer-ffmpeg", argv[i])) {
      setting->use_demuxer_ffmpeg = 1;
      printf("[input argument] --demuxer-ffmpeg\n");
    } else if (!strcmp("--vd-dsp", argv[i])) {
      setting->use_vd_dsp = 1;
      printf("[input argument] --vd-dsp\n");
    } else if (!strcmp("--vd-ffmpeg", argv[i])) {
      setting->use_vd_ffmpeg = 1;
      printf("[input argument] --vd-ffmpeg\n");
    } else if (!strcmp("--vo-dsp", argv[i])) {
      setting->use_vo_dsp = 1;
      printf("[input argument] --vo-dsp\n");
    } else if (!strcmp("--vo-fb", argv[i])) {
      setting->use_vo_linuxfb = 1;
      printf("[input argument] --vo-fb\n");
    } else if (!strcmp("--rtsp-tcp", argv[i])) {
      setting->rtsp_tcp_mode = 1;
      printf("[input argument] --rtsp-tcp\n");
    } else if (!strcmp("--rtsp-udp", argv[i])) {
      setting->rtsp_tcp_mode = 0;
      printf("[input argument] --rtsp-udp\n");
    } else if (!strncmp("-V", argv[i], strlen("-V"))) {
      int len = strlen(argv[i]);
      if (len == strlen("-V")) {
        if ((i + 2) < argc) {
          len = strlen(argv[i + 1]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument] -V: vout mode string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(setting->vout1_mode_string, argv[i + 1]);
          len = strlen(argv[i + 2]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument]: vout device string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(setting->vout1_sinktype_string, argv[i + 2] + 2);
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
          strcpy(setting->vout1_mode_string, argv[i] + 2);
          len = strlen(argv[i + 1]);
          if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
            printf("[input argument]: vout device string name length too long, %d\n", len);
            return (-1);
          }
          strcpy(setting->vout1_sinktype_string, argv[i + 1] + 2);
          i ++;
        } else {
          printf("[input argument] -V: should follow video mode and video device.\n");
          return (-1);
        }
      }
      printf("[input argument] -V: %s, %s.\n", setting->vout1_mode_string, setting->vout1_sinktype_string);
      setting->b_specify_vout1 = 1;
      if (!strcmp("digital", setting->vout1_sinktype_string)) {
        setting->use_digital = 1;
      } else if (!strcmp("hdmi", setting->vout1_sinktype_string)) {
        setting->use_hdmi = 1;
      } else if (!strcmp("cvbs", setting->vout1_sinktype_string)) {
        setting->use_cvbs = 1;
      } else {
        printf("not known vout1 sink type %s\n", setting->vout1_sinktype_string);
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
          strcpy(setting->vout0_mode_string, argv[i + 1]);
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
              strcpy(setting->vout0_device_string, argv[i + 3]);
            } else {
              printf("[input argument] --lcd: should with lcd device name\n");
              return (-1);
            }
            strcpy(setting->vout0_sinktype_string, "digital");
            i += 3;
          } else {
            len = strlen(argv[i + 1]);
            if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
              printf("[input argument]: vout device string name length too long, %d\n", len);
              return (-1);
            }
            strcpy(setting->vout0_sinktype_string, argv[i + 1] + 2);
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
          strcpy(setting->vout0_mode_string, argv[i] + 2);
          if (!strcmp(argv[i + 1], "--lcd")) {
            if ((i + 2) < argc) {
              len = strlen(argv[i + 2]);
              if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
                printf("[input argument]: lcd device string name length too long, %d\n", len);
                return (-1);
              }
              strcpy(setting->vout0_device_string, argv[i + 2]);
            } else {
              printf("[input argument] --lcd: should with lcd device name\n");
              return (-1);
            }
            strcpy(setting->vout0_sinktype_string, "digital");
            i += 2;
          } else {
            len = strlen(argv[i + 1]);
            if (DMAX_UT_VOUT_STRING_LENGTH <= len) {
              printf("[input argument]: vout device string name length too long, %d\n", len);
              return (-1);
            }
            strcpy(setting->vout0_sinktype_string, argv[i + 1] + 2);
            i ++;
          }
        } else {
          printf("[input argument] -v: should follow video mode and video device.\n");
          return (-1);
        }
      }
      printf("[input argument] -v: %s, %s, %s.\n", setting->vout0_mode_string, setting->vout0_sinktype_string, setting->vout0_device_string);
      setting->b_specify_vout0 = 1;
      if (!strcmp("digital", setting->vout0_sinktype_string)) {
        setting->use_digital = 1;
      } else if (!strcmp("hdmi", setting->vout0_sinktype_string)) {
        setting->use_hdmi = 1;
      } else if (!strcmp("cvbs", setting->vout0_sinktype_string)) {
        setting->use_cvbs = 1;
      } else {
        printf("not known vout0 sink type %s\n", setting->vout0_sinktype_string);
      }
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

static int __start_play(am_playback_context_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_START_PLAY,
      ctx,
      sizeof (am_playback_context_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("start play fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __stop_play(am_playback_instance_id_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY,
      ctx,
      sizeof (am_playback_instance_id_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("stop play fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __pause(am_playback_instance_id_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_PAUSE,
      ctx,
      sizeof (am_playback_instance_id_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("pause fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __resume(am_playback_instance_id_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_RESUME,
      ctx,
      sizeof (am_playback_instance_id_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("resume fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __step_play(am_playback_instance_id_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_STEP,
      ctx,
      sizeof (am_playback_instance_id_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("step play fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __seek(am_playback_seek_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_SEEK,
      ctx,
      sizeof (am_playback_seek_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("seek fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __fast_forward(am_playback_fast_forward_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD,
      ctx,
      sizeof (am_playback_fast_forward_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf("fast forward fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __fast_backward(am_playback_fast_backward_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD,
      ctx,
      sizeof (am_playback_fast_backward_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf("fast backward fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __fast_forward_from_begin(am_playback_fast_forward_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN,
      ctx,
      sizeof (am_playback_fast_forward_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("fast forward from begin fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __fast_backward_from_end(am_playback_fast_backward_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END,
      ctx,
      sizeof (am_playback_fast_backward_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf("fast backward from end fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __pb_configure_vout(am_playback_vout_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = nullptr;
  if ((api_helper = AMAPIHelper::get_instance ()) == nullptr) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_PLAYBACK_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT,
      ctx,
      sizeof (am_playback_vout_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("configure vout fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
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

static void __mainloop(am_playback_context_t * ctx)
{
  char buffer_old[128] = {0};
  char buffer[128] = {0};
  char *p_buffer = buffer;
  int flag_stdin = 0;
  int ret = 0;
  int is_paused = 0, is_step = 0;
  am_playback_instance_id_t instance;

  ret = __start_play(ctx);
  if (ret) {
    printf("start play fail, ret %d\n", ret);
    return;
  }

  instance.instance_id = ctx->instance_id;

  flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
  if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
    printf("[error]: stdin_fileno set error.\n");
  }

  while (g_pb_service_running) {
    usleep(100000);
    if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
      continue;
    }
    if (buffer[0] == '\n') {
      p_buffer = buffer_old;
    } else if (buffer[0] == 'l') {
      continue;
    } else {
      ret = __replace_enter(buffer, (128 - 1));
      if (ret) {
        printf("no enter found\n");
        continue;
      }
      p_buffer = buffer;
      strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
      buffer_old[sizeof(buffer_old) - 1] = 0x0;
    }

    switch (p_buffer[0]) {
      case 'q':
        printf("[user cmd]: 'q', Quit\n");
        g_pb_service_running = 0;
        break;
      case 'g': {
          unsigned int target = 0;
          am_playback_seek_t seek_ctx;
          sscanf(p_buffer + 1, "%d", &target);
          seek_ctx.instance_id = ctx->instance_id;
          seek_ctx.target = target;
          ret = __seek(&seek_ctx);
        }
        break;
      case 'f': {
          unsigned int speed;
          am_playback_fast_forward_t ff_ctx;
          sscanf(p_buffer + 1, "%d", &speed);
          ff_ctx.instance_id = ctx->instance_id;
          ff_ctx.scan_mode = 0;
          ff_ctx.playback_speed = speed << 8;
          ret = __fast_forward(&ff_ctx);
        }
        break;
      case 'b': {
          unsigned int speed;
          am_playback_fast_backward_t fb_ctx;
          sscanf(p_buffer + 1, "%d", &speed);
          fb_ctx.instance_id = ctx->instance_id;
          fb_ctx.scan_mode = 0;
          fb_ctx.playback_speed = speed << 8;
          ret = __fast_backward(&fb_ctx);
        }
        break;
      case 'F': {
          unsigned int speed;
          am_playback_fast_forward_t ff_ctx;
          sscanf(p_buffer + 1, "%d", &speed);
          ff_ctx.instance_id = ctx->instance_id;
          ff_ctx.scan_mode = 0;
          ff_ctx.playback_speed = speed << 8;
          ret = __fast_forward_from_begin(&ff_ctx);
        }
        break;
      case 'B': {
          unsigned int speed;
          am_playback_fast_backward_t fb_ctx;
          sscanf(p_buffer + 1, "%d", &speed);
          fb_ctx.instance_id = ctx->instance_id;
          fb_ctx.scan_mode = 0;
          fb_ctx.playback_speed = speed << 8;
          ret = __fast_backward_from_end(&fb_ctx);
        }
        break;
      case ' ': {
          if ((!is_paused) && (!is_step)) {
            ret = __pause(&instance);
            is_paused = 1;
          } else {
            ret = __resume(&instance);
            is_paused = 0;
            is_step = 0;
          }
        }
        break;
      case 's': {
            ret = __step_play(&instance);
            is_step = 1;
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

  __stop_play(&instance);

  return;
}

int main (int argc, char **argv)
{
  am_playback_context_t ctx;
  test_playack_service_air_api_setting_t setting;
  int ret = 0;

  signal (SIGINT , sigstop);
  signal (SIGTERM, sigstop);
  signal (SIGQUIT, sigstop);

  memset(&setting, 0x0, sizeof(setting));
  memset(&ctx, 0x0, sizeof(ctx));
  ctx.instance_id = 0;

  ret = __init_test_playback_air_api_params(argc, argv, &ctx, &setting);
  if (ret) {
    return (-1);
  }

  if (!ctx.url[0]) {
    printf("no url specified\n");
    return (-2);
  }

  if (setting.use_hdmi) {
    ctx.use_hdmi = 1;
    ctx.use_digital = 0;
    ctx.use_cvbs = 0;
  } else if (setting.use_digital) {
    ctx.use_hdmi = 0;
    ctx.use_digital = 1;
    ctx.use_cvbs = 0;
  } else if (setting.use_cvbs) {
    ctx.use_hdmi = 0;
    ctx.use_digital = 0;
    ctx.use_cvbs = 1;
  } else {
    printf("no vout device specified\n");
    return (-3);
  }

  if (setting.b_specify_vout0) {
    am_playback_vout_t vout_config;
    memset(&vout_config, 0x0, sizeof(vout_config));
    vout_config.instance_id = ctx.instance_id;
    vout_config.vout_id = 0;
    snprintf(vout_config.vout_mode, sizeof(vout_config.vout_mode), "%s", setting.vout0_mode_string);
    snprintf(vout_config.vout_sinktype, sizeof(vout_config.vout_sinktype), "%s", setting.vout0_sinktype_string);
    snprintf(vout_config.vout_device, sizeof(vout_config.vout_device), "%s", setting.vout0_device_string);
    ret = __pb_configure_vout(&vout_config);
    if (AM_PLAYBACK_SERVICE_RETCODE_OK != ret) {
      printf("configure vout 0 fail, ret %d\n", ret);
      return ret;
    }
  }

  if (setting.b_specify_vout1) {
    am_playback_vout_t vout_config;
    memset(&vout_config, 0x0, sizeof(vout_config));
    vout_config.instance_id = ctx.instance_id;
    vout_config.vout_id = 1;
    snprintf(vout_config.vout_mode, sizeof(vout_config.vout_mode), "%s", setting.vout1_mode_string);
    snprintf(vout_config.vout_sinktype, sizeof(vout_config.vout_sinktype), "%s", setting.vout1_sinktype_string);
    snprintf(vout_config.vout_device, sizeof(vout_config.vout_device), "%s", setting.vout1_device_string);
    ret = __pb_configure_vout(&vout_config);
    if (AM_PLAYBACK_SERVICE_RETCODE_OK != ret) {
      printf("configure vout 1 fail, ret %d\n", ret);
      return ret;
    }
  }

  if (setting.use_demuxer_native_mp4) {
    strcpy(ctx.prefer_demuxer, "PRMP4");
  } else if (setting.use_demuxer_native_rtsp) {
    strcpy(ctx.prefer_demuxer, "RTSP");
  } else if (setting.use_demuxer_ffmpeg) {
    strcpy(ctx.prefer_demuxer, "FFMpeg");
  } else {
    strcpy(ctx.prefer_demuxer, "AUTO");
  }

  if (setting.use_vd_dsp || setting.use_vo_dsp) {
    strcpy(ctx.prefer_video_decoder, "AMBA");
    strcpy(ctx.prefer_video_output, "AMBA");
  } else if (setting.use_vd_ffmpeg || setting.use_vo_linuxfb) {
    strcpy(ctx.prefer_video_decoder, "FFMpeg");
    strcpy(ctx.prefer_video_output, "LinuxFB");
  } else {
    strcpy(ctx.prefer_video_decoder, "AMBA");
    strcpy(ctx.prefer_video_output, "AMBA");
    printf("no video decoder/output module specified, use DSP as default\n");
  }

  strcpy(ctx.prefer_audio_decoder, "LIBAAC");
  strcpy(ctx.prefer_audio_output, "ALSA");

  __mainloop(&ctx);

  return 0;
}
