/*******************************************************************************
 * am_video_edit_service_msg_action.cpp
 *
 * History:
 *   2016-04-28 - [Zhi He] created file
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
#include "am_api_video_edit.h"

#include "am_playback_new_if.h"
#include "am_video_edit_service_instance.h"

extern AM_SERVICE_STATE  g_service_state;
extern AMIServiceFrame  *g_service_frame;

AMVideoEditServiceInstance *g_video_edit_service_instances[AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER] = {NULL};

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
      INFO("Video Edit service Init Done...");
      for (i = 0; i < AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
        g_video_edit_service_instances[i] = new AMVideoEditServiceInstance(i);
      }
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize video edit service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Video Edit service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON VIDEO EDIT SERVICE DESTROY.");
  for (int i = 0; i < AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
    if (g_video_edit_service_instances[i]) {
      delete g_video_edit_service_instances[i];
      g_video_edit_service_instances[i] = NULL;
    }
  }
  g_service_frame->quit();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  PRINTF("ON VIDEO EDIT SERVICE START.");
  g_service_state = AM_SERVICE_STATE_STARTED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Video Edit service has been started!");
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  PRINTF("ON VIDEO EDIT SERVICE STOP.");
  g_service_state = AM_SERVICE_STATE_STOPPED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Video Edit service has been stopped!");
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
  INFO("Video Edit Service status is: %d\n", service_result->state);
}

void ON_VIDEO_EDIT_ES_2_ES(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_STATE;
    INFO("Video edit service has not been started!");
    return;
  } else {
    struct am_video_edit_context_t *ctx = NULL;
    if ((msg_data == NULL) ||
        (msg_data_size != sizeof(am_video_edit_context_t))) {
      ERROR("Invalid argument: msg_data_size != "
            "sizeof (am_video_edit_context_t)");
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_video_edit_context_t *) msg_data;
    if (AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMVideoEditServiceInstance *instance =
          g_video_edit_service_instances[ctx->instance_id];
      service_result->ret = instance->es_2_es(ctx->input_url,
                                              ctx->output_url,
                                              ctx->stream_index);
      if (service_result->ret) {
        ERROR("es2es fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_VIDEO_EDIT_ES_2_MP4(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_STATE;
    INFO("Video edit service has not been started!");
    return;
  } else {
    struct am_video_edit_context_t *ctx = NULL;
    if ((msg_data == NULL) ||
        (msg_data_size != sizeof(am_video_edit_context_t))) {
      ERROR("Invalid argument: msg_data_size != "
            "sizeof (am_video_edit_context_t)");
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_video_edit_context_t *) msg_data;
    if (AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMVideoEditServiceInstance *instance =
          g_video_edit_service_instances[ctx->instance_id];
      service_result->ret = instance->es_2_mp4(ctx->input_url, ctx->output_url, ctx->stream_index);
      if (service_result->ret) {
        ERROR("es2mp4 fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_VIDEO_EDIT_FEED_EFM_FROM_ES(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_STATE;
    INFO("Video edit service has not been started!");
    return;
  } else {
    struct am_video_edit_context_t *ctx = NULL;
    if ((msg_data == NULL) ||
        (msg_data_size != sizeof(am_video_edit_context_t))) {
      ERROR("Invalid argument: msg_data_size != "
            "sizeof (am_video_edit_context_t)");
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_video_edit_context_t *) msg_data;
    if (AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMVideoEditServiceInstance *instance =
          g_video_edit_service_instances[ctx->instance_id];
      service_result->ret = instance->feed_efm(ctx->input_url, ctx->stream_index);
      if (service_result->ret) {
        ERROR("feed efm fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_VIDEO_EDIT_END_PROCESSING(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = NULL;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_BAD_STATE;
    INFO("Video edit service has not been started!");
    return;
  } else {
    struct am_video_edit_instance_id_t *id = NULL;
    if ((msg_data == NULL) ||
        (msg_data_size != sizeof(am_video_edit_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != "
            "sizeof (am_video_edit_instance_id_t)");
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    id = (struct am_video_edit_instance_id_t *) msg_data;
    if (AM_VIDEO_EDIT_SERVICE_MAX_INSTANCE_NUMBER > id->instance_id) {
      AMVideoEditServiceInstance *instance =
          g_video_edit_service_instances[id->instance_id];
      service_result->ret = instance->end_processing();
      if (service_result->ret) {
        ERROR("end_processing fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", id->instance_id);
      service_result->ret = AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

