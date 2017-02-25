/*******************************************************************************
 * am_playback_service_msg_action.cpp
 *
 * History:
 *   2016-04-14 - [Zhi He] created file
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
#include "am_api_playback.h"

#include "am_playback_new_if.h"
#include "am_playback_service_instance.h"

extern AM_SERVICE_STATE  g_service_state;
extern AMIServiceFrame  *g_service_frame;

AMPlaybackServiceInstance *g_playback_service_instances[AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER] = {nullptr};

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
      INFO("Playback service Init Done...");
      for (i = 0; i < AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
        g_playback_service_instances[i] = new AMPlaybackServiceInstance(i);
      }
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize playback service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Playabck service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON PLAYBACK SERVICE DESTROY.");
  for (int i = 0; i < AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER; i ++) {
    if (g_playback_service_instances[i]) {
      delete g_playback_service_instances[i];
      g_playback_service_instances[i] = nullptr;
    }
  }
  g_service_frame->quit();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  PRINTF("ON PLAYBACK SERVICE START.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  g_service_state = AM_SERVICE_STATE_STARTED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Playback service has been started!");
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON PLAYBACK SERVICE STOP.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  g_service_state = AM_SERVICE_STATE_STOPPED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Playback service has been stopped!");
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
  INFO("Playback Service status is: %d\n", service_result->state);
}

void ON_PLAYBACK_CHECK_DSP_MODE(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
  } else {
    //struct am_playback_context_t *ctx = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_context_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_context_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    //ctx = (struct am_playback_context_t *) msg_data;
  }
}

void ON_PLAYBACK_EXIT_DSP_PLAYBACK_MODE(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  service_result->ret = -1;
}

void ON_PLAYBACK_START_PLAY(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_context_t *ctx = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_context_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_context_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ctx = (struct am_playback_context_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > ctx->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[ctx->instance_id];
      service_result->ret = instance->play(ctx->url, 1, ctx->prefer_demuxer,
        ctx->prefer_video_decoder, ctx->prefer_video_output,
        ctx->prefer_audio_decoder, ctx->prefer_audio_output,
        ctx->use_hdmi, ctx->use_digital, ctx->use_cvbs, 1);
      if (service_result->ret) {
        ERROR("play fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ctx->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_STOP_PLAY(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_instance_id_t *id = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_instance_id_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    id = (struct am_playback_instance_id_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > id->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[id->instance_id];
      service_result->ret = instance->end_play();
      if (service_result->ret) {
        ERROR("end_play fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", id->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_PAUSE(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_instance_id_t *id = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_instance_id_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    id = (struct am_playback_instance_id_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > id->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[id->instance_id];
      service_result->ret = instance->pause();
      if (service_result->ret) {
        ERROR("pause fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", id->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_RESUME(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_instance_id_t *id = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_instance_id_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    id = (struct am_playback_instance_id_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > id->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[id->instance_id];
      service_result->ret = instance->resume();
      if (service_result->ret) {
        ERROR("resume fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", id->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_STEP(void *msg_data,
                                        int msg_data_size,
                                        void *result_addr,
                                        int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_instance_id_t *id = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_instance_id_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_instance_id_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    id = (struct am_playback_instance_id_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > id->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[id->instance_id];
      service_result->ret = instance->step();
      if (service_result->ret) {
        ERROR("step fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", id->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_QUERY_CURRENT_POSITION(void *msg_data,
                                        int msg_data_size,
                                        void *result_addr,
                                        int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_position_t *pos = nullptr;
    unsigned long long cur_pos = 0;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_position_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_position_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    pos = (struct am_playback_position_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > pos->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[pos->instance_id];
      service_result->ret = instance->query_current_position(&cur_pos);
      if (service_result->ret) {
        ERROR("query_current_position fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", pos->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_SEEK(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_seek_t *seek = nullptr;
    unsigned long long target = 0;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_seek_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_seek_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    seek = (struct am_playback_seek_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > seek->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[seek->instance_id];
      target = seek->target;
      service_result->ret = instance->seek(target);
      if (service_result->ret) {
        ERROR("seek fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", seek->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_FAST_FORWARD(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_fast_forward_t *ff = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_fast_forward_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_fast_forward_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ff = (struct am_playback_fast_forward_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > ff->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[ff->instance_id];
      service_result->ret = instance->fast_forward(ff->playback_speed, ff->scan_mode, 0);
      if (service_result->ret) {
        ERROR("fast forward fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ff->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_FAST_BACKWARD(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_fast_backward_t *fb = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_fast_backward_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_fast_backward_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    fb = (struct am_playback_fast_backward_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > fb->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[fb->instance_id];
      service_result->ret = instance->fast_backward(fb->playback_speed, fb->scan_mode, 0);
      if (service_result->ret) {
        ERROR("fast backward fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", fb->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_FAST_FORWARD_FROM_BEGIN(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_fast_forward_t *ff = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_fast_forward_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_fast_forward_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ff = (struct am_playback_fast_forward_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > ff->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[ff->instance_id];
      service_result->ret = instance->fast_forward(ff->playback_speed, ff->scan_mode, 1);
      if (service_result->ret) {
        ERROR("fast forward fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", ff->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_FAST_BACKWARD_FROM_END(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_fast_backward_t *fb = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_fast_backward_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_fast_backward_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    fb = (struct am_playback_fast_backward_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > fb->instance_id) {
      AMPlaybackServiceInstance *instance = g_playback_service_instances[fb->instance_id];
      service_result->ret = instance->fast_backward(fb->playback_speed, fb->scan_mode, 1);
      if (service_result->ret) {
        ERROR("fast backward fail, ret %d", service_result->ret);
      }
    } else {
      ERROR("Invalid argument: instance id %d", fb->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_CONFIGURE_VOUT(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    struct am_playback_vout_t *vout = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_vout_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_vout_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    vout = (struct am_playback_vout_t *) msg_data;
    if (AM_PLAYBACK_SERVICE_MAX_INSTANCE_NUMBER > vout->instance_id) {
      SVideoOutputConfigure config;
      memset(&config, 0x0, sizeof(config));
      config.vout_id = vout->vout_id;
      config.mode_string = vout->vout_mode;
      config.sink_type_string = vout->vout_sinktype;
      config.device_string = vout->vout_device;

      if (!strcmp(gfGetDSPPlatformName(), "S2L")) {
        config.b_config_mixer = 1;
        config.mixer_flag = 1;
      }

      EECode err = gfAmbaDSPConfigureVout(&config);
      if (EECode_OK != err) {
        ERROR("config vout %d fail, ret 0x%08x, %s\n", vout->vout_id, err, gfGetErrorCodeString(err));
        service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
      } else {
        service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_OK;
      }
    } else {
      ERROR("Invalid argument: instance id %d", vout->instance_id);
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
    }
  }
}

void ON_PLAYBACK_HALT_VOUT(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
    INFO("Playback service has not been started!");
    return;
  } else {
    //struct am_playback_vout_t *vout = nullptr;
    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_playback_vout_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_playback_vout_t)");
      service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
      return;
    }
    ERROR("not support halt now\n");
    service_result->ret = AM_PLAYBACK_SERVICE_RETCODE_NOT_SUPPORT;
  }
}

