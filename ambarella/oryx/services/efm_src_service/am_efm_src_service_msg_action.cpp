/*******************************************************************************
 * am_video_efm_src_msg_action.cpp
 *
 * History:
 *   2016-05-04 - [Zhi He] created file
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

#include "am_base_include.h"

#include <mutex>

#include "am_log.h"
#include "am_service_frame_if.h"
#include "am_api_efm_src.h"

#include "am_playback_new_if.h"
#include "am_efm_src_service_instance.h"

extern AM_SERVICE_STATE  g_service_state;
extern AMIServiceFrame  *g_service_frame;

AMEFMSourceServiceInstance *g_efm_src_service_instances[AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER] = {NULL};

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      int i = 0;
      INFO("EFM source service Init Done...");
      for (i = 0; i < AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
        g_efm_src_service_instances[i] = new AMEFMSourceServiceInstance(i);
      }
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize EFM Source service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("EFM source service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON EFM SOURCE SERVICE DESTROY.");
  for (int i = 0; i < AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
    if (g_efm_src_service_instances[i]) {
      delete g_efm_src_service_instances[i];
      g_efm_src_service_instances[i] = NULL;
    }
  }
  g_service_frame->quit();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  PRINTF("ON EFM SOURCE SERVICE START.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  g_service_state = AM_SERVICE_STATE_STARTED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("EFM source service has been started!");
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON EMF SOURCE SERVICE STOP.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  for (int i = 0; i < AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
    if (g_efm_src_service_instances[i]) {
      delete g_efm_src_service_instances[i];
      g_efm_src_service_instances[i] = NULL;
    }
  }
  g_service_state = AM_SERVICE_STATE_STOPPED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("EFM source service has been stopped!");
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  // nothing needs to be done.
}

void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  service_result->ret = 0;
  service_result->state = g_service_state;
  INFO("EFM Source Service status is: %d\n", service_result->state);
}

void ON_FEED_EFM_FROM_LOCAL_ES_FILE(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_context_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_context_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_context_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_context_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      service_result->ret = instance->feed_efm_from_es(ctx->input_url, ctx->stream_index);
      if (service_result->ret) {
        ERROR("feed EFM from local es file fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_FEED_EFM_FROM_USB_CAMERA(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_context_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_context_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_context_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_context_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      if ((ctx->b_dump_efm_src) && (ctx->dump_efm_src_url[0])) {
        service_result->ret = instance->feed_efm_from_usb_camera(ctx->input_url, ctx->stream_index,
          (char *) ctx->dump_efm_src_url, ctx->prefer_format,
          ctx->prefer_video_width, ctx->prefer_video_height, ctx->prefer_video_fps);
      } else {
        service_result->ret = instance->feed_efm_from_usb_camera(ctx->input_url, ctx->stream_index,
          NULL, ctx->prefer_format,
          ctx->prefer_video_width, ctx->prefer_video_height, ctx->prefer_video_fps);
      }
      if (service_result->ret) {
        ERROR("feed EFM from usb camera fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_END_FEED_EFM(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_instance_id_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_instance_id_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_instance_id_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      service_result->ret = instance->end_feed();
      if (service_result->ret) {
        ERROR("end feed EFM fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}


void ON_SETUP_YUV_CAPTURE_PIPELINE(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_context_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_context_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_context_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_context_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      service_result->ret = instance->setup_yuv_capture_pipeline(
          ctx->yuv_source_buffer_id, ctx->stream_index);
      if (service_result->ret) {
        ERROR("setup yuv capture pipeline fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_DESTROY_YUV_CAPTURE_PIPELINE(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_instance_id_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_instance_id_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_instance_id_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      service_result->ret = instance->destroy_yuv_capture_pipeline();
      if (service_result->ret) {
        ERROR("setup yuv capture pipeline fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_YUV_CAPTURE_FOR_EFM(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    INFO("EFM Source service has not been started!");
    return;
  } else {
    struct am_efm_src_instance_id_t *ctx = NULL;
    if ((msg_data == NULL) || (msg_data_size != sizeof(am_efm_src_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_efm_src_instance_id_t)");
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_efm_src_instance_id_t *) msg_data;
    if (AM_EFM_SRC_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMEFMSourceServiceInstance *instance = g_efm_src_service_instances[ctx->instance_id];
      uint8_t num = ctx->number_of_yuv_frames;
      if (3 < num) {
        num = 3;
      }
      service_result->ret = instance->yuv_capture_for_efm(num);
      if (service_result->ret) {
        ERROR("setup yuv capture pipeline fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

