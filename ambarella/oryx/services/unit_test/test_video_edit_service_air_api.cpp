/*
 * test_video_edit_service_air_api.h
 *
 * @Author: Zhi He
 * @Email : zhe@ambarella.com
 * @Time  : 29/04/2016 [Created]
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
#include "am_api_video_edit.h"

static bool g_ve_service_running = true;

static void sigstop (int32_t arg)
{
  printf ("Signal has been captured, test_video_edit_service_air_api quits!\n");
  g_ve_service_running = false;
}

#if 0
static int __on_notify(void *msg_data, int msg_data_size)
{
  am_service_notify_payload *payload = (am_service_notify_payload *)msg_data;
  am_video_edit_msg_t *msg = (am_video_edit_msg_t *)payload->data;

  switch(msg->msg_type) {
  case VE_APP_MSG_FINISH:
    NOTICE("app receive VE_APP_MSG_FINISH, video edit finished");
    g_ve_service_running = false;
    break;
  default:
    ERROR("unknown msg_type %d", msg->msg_type);
    break;
  }

  return 0;
}
#endif

static void __print_ut_options()
{
  printf ("\n=============== oryx video edit service test =================\n\n");
  printf("\t'-i [filename]': '-i' specify input file name (can be .mjpeg, .h264, .h265)\n");
  printf("\t'-o [filename]': '-o' specify output file name (h264 from efm)\n");
  printf("\t'--es2es': es --> yuv --> efm --> es\n");
  printf("\t'--es2mp4': es --> yuv --> efm --> es --> mp4\n");
  printf("\t'--feedefm': es --> yuv -->efm\n");
  printf("\t'--end': end processing\n");
  printf("\t'-A': use stream 0 as EFM\n");
  printf("\t'-B': use stream 1 as EFM\n");
  printf("\t'-C': use stream 2 as EFM\n");
  printf("\t'-D': use stream 3 as EFM\n");
  printf ("\n=========================================================\n\n");
}

static void __print_ut_cmds()
{
  printf("test_video_edit_service_air_api runtime cmds: press cmd + Enter\n");
  printf("\t'q': Quit unit test\n");
}

enum {
  EVIDEO_EDIT_OP_INVALID = 0x00,
  EVIDEO_EDIT_OP_MODE_ES2ES = 0x01,
  EVIDEO_EDIT_OP_MODE_ES2MP4 = 0x02,
  EVIDEO_EDIT_OP_MODE_FEED_EFM = 0x03,
  EVIDEO_EDIT_OP_MODE_END_PROCESSING = 0x04,
};

struct test_video_edit_service_air_api_setting_t {
  unsigned char ve_operation_mode;
  unsigned char reserved0;
  unsigned char reserved1;
  unsigned char reserved2;
};

static int __init_test_video_edit_air_api_params(int argc, char **argv, am_video_edit_context_t *ctx, test_video_edit_service_air_api_setting_t *setting)
{
  int i = 0;
  for (i = 1; i < argc; i++) {
    if (!strcmp("-i", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(ctx->input_url, AM_VIDEO_EDIT_MAX_URL_LENGTH, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] -i: input url: %s.\n", ctx->input_url);
      } else {
        printf("[input argument] -i: should follow input filename.\n");
      }
    } else if (!strcmp("-o", argv[i])) {
      if ((i + 1) < argc) {
        snprintf(ctx->output_url, AM_VIDEO_EDIT_MAX_URL_LENGTH, "%s", argv[i + 1]);
        i ++;
        printf("[input argument] -o: output url: %s.\n", ctx->output_url);
      } else {
        printf("[input argument] -o: should follow output filename.\n");
      }
    } else if (!strcmp("--es2es", argv[i])) {
      setting->ve_operation_mode = EVIDEO_EDIT_OP_MODE_ES2ES;
      printf("[input argument] --es2es\n");
    } else if (!strcmp("--es2mp4", argv[i])) {
      setting->ve_operation_mode = EVIDEO_EDIT_OP_MODE_ES2MP4;
      printf("[input argument] --es2mp4\n");
    } else if (!strcmp("--feedefm", argv[i])) {
      setting->ve_operation_mode = EVIDEO_EDIT_OP_MODE_FEED_EFM;
      printf("[input argument] --feedefm\n");
    } else if (!strcmp("--end", argv[i])) {
      setting->ve_operation_mode = EVIDEO_EDIT_OP_MODE_END_PROCESSING;
      printf("[input argument] --end\n");
    } else if (!strcmp("-A", argv[i])) {
      ctx->stream_index = 0;
    } else if (!strcmp("-B", argv[i])) {
      ctx->stream_index = 1;
    } else if (!strcmp("-C", argv[i])) {
      ctx->stream_index = 2;
    } else if (!strcmp("-D", argv[i])) {
      ctx->stream_index = 3;
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

static int __start_es2es(am_video_edit_context_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = NULL;
  if ((api_helper = AMAPIHelper::get_instance ()) == NULL) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_VIDEO_EDIT_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_ES,
      ctx,
      sizeof (am_video_edit_context_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("es2es fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __start_es2mp4(am_video_edit_context_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = NULL;
  if ((api_helper = AMAPIHelper::get_instance ()) == NULL) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_VIDEO_EDIT_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_VIDEO_EDIT_ES_2_MP4,
      ctx,
      sizeof (am_video_edit_context_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("es2mp4 fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __start_feedefm(am_video_edit_context_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = NULL;
  if ((api_helper = AMAPIHelper::get_instance ()) == NULL) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_VIDEO_EDIT_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_VIDEO_EDIT_FEED_EFM_FROM_ES,
      ctx,
      sizeof (am_video_edit_context_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("feedefm fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

static int __end_processing(am_video_edit_instance_id_t *ctx)
{
  am_service_result_t service_result;

  AMAPIHelperPtr api_helper = NULL;
  if ((api_helper = AMAPIHelper::get_instance ()) == NULL) {
    printf ("Failed to get an instance of AMAPIHelper!");
    return AM_VIDEO_EDIT_SERVICE_RETCODE_NOT_AVIALABLE;
  }

  api_helper->method_call(
      AM_IPC_MW_CMD_VIDEO_EDIT_END_PROCESSING,
      ctx,
      sizeof (am_video_edit_instance_id_t),
      &service_result,
      sizeof (am_service_result_t));

  if (service_result.ret < 0) {
    printf ("end processing fail, ret %d\n", service_result.ret);
  }

  return service_result.ret;
}

int main (int argc, char **argv)
{
  am_video_edit_context_t ctx;
  test_video_edit_service_air_api_setting_t setting;
  int ret = 0;

  signal (SIGINT , sigstop);
  signal (SIGTERM, sigstop);
  signal (SIGQUIT, sigstop);

  memset(&setting, 0x0, sizeof(setting));
  memset(&ctx, 0x0, sizeof(ctx));
  ctx.instance_id = 0;

  if (argc < 2) {
    __print_ut_options();
    return 0;
  }

  ret = __init_test_video_edit_air_api_params(argc, argv, &ctx, &setting);
  if (ret) {
    return (-1);
  }

  if (EVIDEO_EDIT_OP_MODE_ES2ES == setting.ve_operation_mode) {
    printf("EFM stream index %d\n", ctx.stream_index);
    ret = __start_es2es(&ctx);
  } else if (EVIDEO_EDIT_OP_MODE_ES2MP4 == setting.ve_operation_mode) {
    printf("EFM stream index %d\n", ctx.stream_index);
    ret = __start_es2mp4(&ctx);
  } else if (EVIDEO_EDIT_OP_MODE_FEED_EFM == setting.ve_operation_mode) {
    printf("EFM stream index %d\n", ctx.stream_index);
    ret = __start_feedefm(&ctx);
  } else if (EVIDEO_EDIT_OP_MODE_END_PROCESSING == setting.ve_operation_mode) {
    am_video_edit_instance_id_t instance_id;
    instance_id.instance_id = ctx.instance_id;
    ret = __end_processing(&instance_id);
    return ret;
  }

#if 0
  AMAPIHelperPtr api_helper = AMAPIHelper::get_instance();
  if (!api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }
  api_helper->register_notify_cb(__on_notify);
  while (g_ve_service_running) {
    sleep(1);
  }

  {
    am_video_edit_instance_id_t instance_id;
    printf("end processing after recieve video edit finished msg\n");
    instance_id.instance_id = ctx.instance_id;
    ret = __end_processing(&instance_id);
  }
#endif

  return ret;
}
