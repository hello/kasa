/*******************************************************************************
 * am_event_service_msg_action.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#include "am_define.h"
#include "am_log.h"

#include "am_event_monitor_if.h"
#include "am_service_frame_if.h"
#include "am_event_service_priv.h"
#include "am_api_event.h"
#include "am_api_video.h"
#include "commands/am_api_cmd_common.h"

extern AMIEventMonitorPtr g_event_monitor;
extern AM_SERVICE_STATE g_service_state;
extern AMIServiceFrame *g_service_frame;
extern AMIPCSyncCmdServer g_ipc;

static int32_t key_input_event_callback(AM_EVENT_MESSAGE *event_msg)
{
  const char *key_state[AM_KEY_STATE_NUM] =
  {" ", "UP", "DOWN", "CLICKED", "LONG_PRESS" };
  INFO("\npts: %llu, event number: %llu, key code: %d, key state: %s\n",
       event_msg->pts,
       event_msg->seq_num,
       event_msg->key_event.key_value,
       key_state[event_msg->key_event.key_state]);

  am_service_notify_payload payload;
  memset(&payload, 0, sizeof(am_service_notify_payload));
  SET_BIT(payload.dest_bits, AM_SERVICE_TYPE_GENERIC);
  payload.type = AM_EVENT_SERVICE_NOTIFY;
  payload.msg_id = AM_IPC_MW_CMD_COMMON_GET_EVENT;
  payload.data_size = sizeof(AM_EVENT_MESSAGE);
  memcpy(payload.data, event_msg, payload.data_size);

  g_ipc.notify(AM_IPC_SERVICE_NOTIF, &payload,
               payload.header_size() + payload.data_size);
  return 0;
}

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  //if it's init done, then
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch (g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("Event service Init Done...");
    }
      break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize Evet service...");
    }
      break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Evet service is still initializing...");
    }
      break;
    default:
      break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON EVENT SERVICE DESTROY.");
  ((am_service_result_t*) result_addr)->ret = 0;
  ((am_service_result_t*) result_addr)->state = g_service_state;
  g_service_frame->quit(); /* make run() in main() quit */
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int32_t ret = 0;
  PRINTF("ON EVENT SERVICE START.");
  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->start_all_event_monitor()) {
        ERROR("start_all_event_monitor error\n");
        ret = -1;
        break;
      }
    }

    //Set key input callback function when service starting.
    if (g_event_monitor->is_module_registered(EV_KEY_INPUT_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      AM_KEY_INPUT_CALLBACK key_callback;
      key_callback.key_value = -1;
      key_callback.callback = key_input_event_callback;
      module_config.key = AM_KEY_CALLBACK;
      module_config.value = (void *) &key_callback;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
      key_callback.key_value = 238;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int32_t ret = 0;

  PRINTF("ON EVENT SERVICE STOP.");
  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->stop_all_event_monitor()) {
        ERROR("failed to stop all event monitor\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  int32_t ret = 0;
  INFO("event service restart all event modules\n");

  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->stop_all_event_monitor()) {
        ERROR("failed to stop all event monitor\n");
        ret = -1;
        break;
      }
      if (!g_event_monitor->start_all_event_monitor()) {
        ERROR("start all event module failed!");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("event service get status\n");
  ((am_service_result_t*) result_addr)->ret = 0;
  ((am_service_result_t*) result_addr)->state = g_service_state;
}

void ON_SERVICE_REGISTER_MODULES(void *msg_data,
                                 int msg_data_size,
                                 void *result_addr,
                                 int result_max_size)
{
  int32_t ret = 0;
  INFO("event service register module \n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!\n");
      break;
    }

    am_event_module_path *module_path = (am_event_module_path *) msg_data;
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->register_module((EVENT_MODULE_ID) module_path->id,
                                            module_path->path_name)) {
        ret = -1;
        ERROR("Call register_module failed!\n");
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_IS_MODULE_RESGISTER(void *msg_data,
                                    int msg_data_size,
                                    void *result_addr,
                                    int result_max_size)
{
  int32_t ret = 0;
  am_event_module_register_state module_register_state;

  INFO("event service is module register\n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      module_register_state.state = g_event_monitor->is_module_registered(id);
      module_register_state.id = id;

      break;
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
  memcpy(service_result->data,
         &module_register_state,
         sizeof(module_register_state));
}

void ON_SERVICE_START_ALL_EVENT_MODULE(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  int32_t ret = 0;
  INFO("event service start all event module\n");
  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->start_all_event_monitor()) {
        ERROR("failed to start all event monitor!\n");
        ret = -1;
        break;
      }
    }

    //Set key input callback function when service starting.
    if (g_event_monitor->is_module_registered(EV_KEY_INPUT_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      AM_KEY_INPUT_CALLBACK key_callback;
      key_callback.key_value = 116;
      key_callback.callback = key_input_event_callback;
      module_config.key = AM_KEY_CALLBACK;
      module_config.value = (void *) &key_callback;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
      key_callback.key_value = 238;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP_ALL_EVENT_MODULE(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  int32_t ret = 0;
  INFO("event service stop all event modules\n");
  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->stop_all_event_monitor()) {
        ERROR("failed to stop all event monitor!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_DESTROY_ALL_EVENT_MODULE(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size)
{
  int32_t ret = 0;
  INFO("event service destroy all event modules!\n");

  do {
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->destroy_all_event_monitor()) {
        ERROR("failed to destroy all event monitor\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_START_EVENT_MODULE(void *msg_data,
                                   int msg_data_size,
                                   void *result_addr,
                                   int result_max_size)
{
  int32_t ret = 0;
  INFO("event service start event module\n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->start_event_monitor(id)) {
        ERROR("failed to start event monitor id : %d\n", id);
        ret = -1;
        break;
      }
    }

    //Set key input callback function when service starting.
    if (g_event_monitor->is_module_registered(EV_KEY_INPUT_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      AM_KEY_INPUT_CALLBACK key_callback;
      key_callback.key_value = 116;
      key_callback.callback = key_input_event_callback;
      module_config.key = AM_KEY_CALLBACK;
      module_config.value = (void *) &key_callback;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
      key_callback.key_value = 238;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP_EVENT_MODULE(void *msg_data,
                                  int msg_data_size,
                                  void *result_addr,
                                  int result_max_size)
{
  int32_t ret = 0;
  INFO("event service stop event module\n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->stop_event_monitor(id)) {
        ERROR("failed to stop event monitor id : %d\n", id);
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_DESTROY_EVENT_MODULE(void *msg_data,
                                     int msg_data_size,
                                     void *result_addr,
                                     int result_max_size)
{
  int32_t ret = 0;
  INFO("event service destroy event module\n");

  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    } else {
      if (!g_event_monitor->destroy_event_monitor(id)) {
        ERROR("failed to destroy event monitor id : %d\n", id);
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_SET_EVENT_AUDIO_ALERT_CONFIG(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size)
{
  INFO("event service set event audio alert config\n");
  int32_t ret = 0;

  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    }

    am_event_audio_alert_config_s *aa_config =
        (am_event_audio_alert_config_s*) msg_data;
    EVENT_MODULE_CONFIG config =
    {0};
    config.value = malloc(sizeof(int32_t));

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_CHANNEL_NUM)) {
      config.key = AM_CHANNEL_NUM;
      memcpy(config.value, &aa_config->channel_num, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_RATE)) {
      config.key = AM_SAMPLE_RATE;
      memcpy(config.value, &aa_config->sample_rate, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_CHUNK_BYTES)) {
      config.key = AM_CHUNK_BYTES;
      memcpy(config.value, &aa_config->chunk_bytes, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_FORMAT)) {
      config.key = AM_SAMPLE_FORMAT;
      memcpy(config.value, &aa_config->sample_format, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_ENABLE)) {
      config.key = AM_ALERT_ENABLE;
      memcpy(config.value, &aa_config->enable, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_SENSITIVITY)) {
      config.key = AM_ALERT_SENSITIVITY;
      memcpy(config.value, &aa_config->sensitivity, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_THRESHOLD)) {
      config.key = AM_ALERT_THRESHOLD;
      memcpy(config.value, &aa_config->threshold, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    if (TEST_BIT(aa_config->enable_bits,
                 AM_EVENT_AUDIO_ALERT_CONFIG_DIRECTION)) {
      config.key = AM_ALERT_DIRECTION;
      memcpy(config.value, &aa_config->direction, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
        ret = -1;
        break;
      }
    }

    //sync and save to config file
    config.key = AM_ALERT_SYNC_CONFIG;
    *(uint32_t*) config.value = 0;
    if (!g_event_monitor->set_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to set monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    free(config.value);

  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_SET_EVENT_KEY_INPUT_CONFIG(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  INFO("event service set event key input config\n");
  int32_t ret = 0;

  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    }

    am_event_key_input_config_s *ki_config =
        (am_event_key_input_config_s*) msg_data;
    EVENT_MODULE_CONFIG config =
    {0};

    if (TEST_BIT(ki_config->enable_bits, AM_EVENT_KEY_INPUT_CONFIG_LPT)) {
      config.key = AM_LONG_PRESSED_TIME;
      config.value = malloc(sizeof(int32_t));
      memcpy(config.value, &ki_config->long_press_time, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_KEY_INPUT_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }

    //sync and save to config file
    config.key = AM_KEY_INPUT_SYNC_CONFIG;
    config.value = malloc(sizeof(uint32_t));
    *(uint32_t*) config.value = 0;
    if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT, &config)) {
      ERROR("failed to set monitor config id: %d\n", EV_KEY_INPUT_DECT);
      ret = -1;
      break;
    }
    free(config.value);
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_GET_EVENT_AUDIO_ALERT_CONFIG(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size)
{
  INFO("event service get event audio alert config\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    }

    am_event_audio_alert_config_s *audio_alert_config =
        (am_event_audio_alert_config_s*) service_result->data;
    EVENT_MODULE_CONFIG config =
    {0};
    config.value = malloc(sizeof(uint32_t));

    config.key = AM_CHANNEL_NUM;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->channel_num = *(uint32_t *) config.value;

    config.key = AM_SAMPLE_RATE;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->sample_rate = *(uint32_t *) config.value;

    config.key = AM_CHUNK_BYTES;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->chunk_bytes = *(uint32_t *) config.value;

    config.key = AM_SAMPLE_FORMAT;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->sample_format = *(uint32_t *) config.value;

    config.key = AM_ALERT_ENABLE;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->enable = *(bool *) config.value;

    config.key = AM_ALERT_SENSITIVITY;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->sensitivity = *(uint32_t *) config.value;

    config.key = AM_ALERT_THRESHOLD;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->threshold = *(uint32_t *) config.value;

    config.key = AM_ALERT_DIRECTION;
    if (!g_event_monitor->get_monitor_config(EV_AUDIO_ALERT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_AUDIO_ALERT_DECT);
      ret = -1;
      break;
    }
    audio_alert_config->direction = *(uint32_t *) config.value;
    free(config.value);

  } while (0);

  service_result->ret = ret;
}

void ON_SERVICE_GET_EVENT_KEY_INPUT_CONFIG(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  INFO("event service get event key input config\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    if (!g_event_monitor) {
      ret = -1;
      ERROR("g_event_monitor is null!\n");
      break;
    }
    am_event_key_input_config_s *key_input_config =
        (am_event_key_input_config_s*) service_result->data;
    EVENT_MODULE_CONFIG config =
    {0};
    config.value = malloc(sizeof(uint32_t));

    config.key = AM_LONG_PRESSED_TIME;
    if (!g_event_monitor->get_monitor_config(EV_KEY_INPUT_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_KEY_INPUT_DECT);
      ret = -1;
      break;
    }
    key_input_config->long_press_time = *(uint32_t *) config.value;
    free(config.value);
    break;

  } while (0);

  service_result->ret = ret;
}

void ON_COMMON_START_EVENT_MODULE(void *msg_data,
                                  int msg_data_size,
                                  void *result_addr,
                                  int result_max_size)
{
  int32_t ret = 0;
  INFO("event service start common event module\n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      break;
    } else {
      if (!g_event_monitor->start_event_monitor(id)) {
        ERROR("failed to start event monitor id : %d\n", id);
        ret = -1;
        break;
      }
    }

    //Set key input callback function when service starting.
    if (g_event_monitor->is_module_registered(EV_KEY_INPUT_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      AM_KEY_INPUT_CALLBACK key_callback;
      key_callback.key_value = 116;
      key_callback.callback = key_input_event_callback;
      module_config.key = AM_KEY_CALLBACK;
      module_config.value = (void *) &key_callback;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
      key_callback.key_value = 238;
      if (!g_event_monitor->set_monitor_config(EV_KEY_INPUT_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set key input callback failed!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_COMMON_STOP_EVENT_MODULE(void *msg_data,
                                 int msg_data_size,
                                 void *result_addr,
                                 int result_max_size)
{
  int32_t ret = 0;
  INFO("event service stop common event module\n");
  do {
    if (!msg_data || msg_data_size == 0) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }

    EVENT_MODULE_ID id = *(EVENT_MODULE_ID *) msg_data;
    if (!g_event_monitor) {
      break;
    } else {
      if (!g_event_monitor->stop_event_monitor(id)) {
        ERROR("failed to stop event monitor id : %d\n", id);
        ret = -1;
        break;
      }
    }
  } while (0);

  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}
