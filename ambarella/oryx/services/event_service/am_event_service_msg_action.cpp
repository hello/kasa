/*******************************************************************************
 * am_event_service_msg_action.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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
extern AMIPCBase *g_ipc_base_obj[];

static int32_t send_motion_event_to_video_module(AM_EVENT_MESSAGE *event_msg)
{
  int32_t cmd_result = 0;
  am_service_notify_payload payload;

  const char *motion_type[AM_MD_MOTION_TYPE_NUM] =
  {"MOTION_NONE", "MOTION_START", "MOTION_LEVEL_CHANGED", "MOTION_END"};
  const char *motion_level[AM_MOTION_L_NUM] =
  {"MOTION_LEVEL_0", "MOTION_LEVEL_1", "MOTION_LEVEL_2"};
  INFO("pts: %llu, event number: %llu, %s, %s, ROI#%d",
         event_msg->pts,
         event_msg->seq_num,
         motion_type[event_msg->md_msg.mt_type],
         motion_level[event_msg->md_msg.mt_level],
         event_msg->md_msg.roi_id);

  SET_BIT(payload.dest_bits, AM_SERVICE_TYPE_VIDEO);
  payload.msg_id = AM_IPC_MW_CMD_COMMON_GET_EVENT;
  payload.data_size = sizeof(AM_EVENT_MESSAGE);
  memcpy(payload.data, event_msg, payload.data_size);

  ((AMIPCSyncCmdServer*)g_ipc_base_obj[EVT_IPC_API_PROXY])->\
      notify(AM_IPC_SERVICE_NOTIF, &payload,
             payload.header_size() + payload.data_size);
  return cmd_result;
}

static int32_t key_input_event_callback(AM_EVENT_MESSAGE *event_msg)
{
  const char *key_state[AM_KEY_STATE_NUM] =
  { "UP", "DOWN", "CLICKED", "LONG_PRESS" };
  INFO("\npts: %llu, event number: %llu, key code: %d, key state: %s\n",
       event_msg->pts,
       event_msg->seq_num,
       event_msg->key_event.key_value,
       key_state[event_msg->key_event.key_state]);
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
  PRINTF("ON EVENT SERVICE DESTROY");
  g_service_frame->quit(); /* make run() in main() quit */
  ((am_service_result_t*) result_addr)->ret = 0;
  ((am_service_result_t*) result_addr)->state = g_service_state;
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int32_t ret = 0;
  INFO("event service start all event modules\n");
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
    //Set motion detect callback function when service starting.
    //Make sure motion event can send to LBR ASAP
    if (g_event_monitor->is_module_registered(EV_MOTION_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      module_config.key = AM_MD_CALLBACK;
      module_config.value = (void*) send_motion_event_to_video_module;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set motion detect callback failed!\n");
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

void ON_SERVICE_STOP(void *msg_data,
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

void ON_SERVICE_START_ALL_EVENT_MOUDLE(void *msg_data,
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
    //Set motion detect callback function when service starting.
    //Make sure motion event can send to LBR ASAP
    if (g_event_monitor->is_module_registered(EV_MOTION_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      module_config.key = AM_MD_CALLBACK;
      module_config.value = (void*) send_motion_event_to_video_module;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set motion detect callback failed!\n");
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

void ON_SERVICE_STOP_ALL_EVENT_MOUDLE(void *msg_data,
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

void ON_SERVICE_DESTROY_ALL_EVENT_MOUDLE(void *msg_data,
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

void ON_SERVICE_START_EVENT_MOUDLE(void *msg_data,
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
    //Set motion detect callback function when service starting.
    //Make sure motion event can send to LBR ASAP
    if (g_event_monitor->is_module_registered(EV_MOTION_DECT)) {
      EVENT_MODULE_CONFIG module_config;
      module_config.key = AM_MD_CALLBACK;
      module_config.value = (void*) send_motion_event_to_video_module;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT,
                                               &module_config)) {
        ERROR("set_monitor_config error, set motion detect callback failed!\n");
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

void ON_SERVICE_STOP_EVENT_MOUDLE(void *msg_data,
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

void ON_SERVICE_DESTROY_EVENT_MOUDLE(void *msg_data,
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

void ON_SERVICE_SET_EVENT_MOTION_DETECT_CONFIG(void *msg_data,
                                               int msg_data_size,
                                               void *result_addr,
                                               int result_max_size)
{
  INFO("event service set event motion detect config\n");
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

    am_event_md_config_s *md_config = (am_event_md_config_s*) msg_data;
    EVENT_MODULE_CONFIG config =
    {0};
    memset(&config, 0, sizeof(EVENT_MODULE_CONFIG));

    if (TEST_BIT(md_config->enable_bits, AM_EVENT_MD_CONFIG_ENABLE)) {
      config.key = AM_MD_ENABLE;
      config.value = malloc(sizeof(int32_t));
      memcpy(config.value, &md_config->enable, sizeof(uint32_t));
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }

    if (TEST_BIT(md_config->enable_bits, AM_EVENT_MD_CONFIG_THRESHOLD0)) {
      config.key = AM_MD_THRESHOLD;
      config.value = malloc(sizeof(AM_EVENT_MD_THRESHOLD));
      if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      uint32_t th1 = ((AM_EVENT_MD_THRESHOLD *) config.value)->threshold[1];
      memcpy(config.value,
             &md_config->threshold,
             sizeof(AM_EVENT_MD_THRESHOLD));
      ((AM_EVENT_MD_THRESHOLD *) config.value)->threshold[1] = th1;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }

    if (TEST_BIT(md_config->enable_bits, AM_EVENT_MD_CONFIG_THRESHOLD1)) {
      config.key = AM_MD_THRESHOLD;
      config.value = malloc(sizeof(AM_EVENT_MD_THRESHOLD));
      if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      uint32_t th0 = ((AM_EVENT_MD_THRESHOLD *) config.value)->threshold[0];
      memcpy(config.value,
             &md_config->threshold,
             sizeof(AM_EVENT_MD_THRESHOLD));
      ((AM_EVENT_MD_THRESHOLD *) config.value)->threshold[0] = th0;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }

    if (TEST_BIT(md_config->enable_bits, AM_EVENT_MD_CONFIG_LEVEL0_CHANGE_DELAY)) {
      config.key = AM_MD_LEVEL_CHANGE_DELAY;
      config.value = malloc(sizeof(AM_EVENT_MD_LEVEL_CHANGE_DELAY));
      if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      uint32_t lc1_delay =
          ((AM_EVENT_MD_LEVEL_CHANGE_DELAY *)config.value)->mt_level_change_delay[1];
      memcpy(config.value,
             &md_config->level_change_delay,
             sizeof(md_config->level_change_delay));
      ((AM_EVENT_MD_LEVEL_CHANGE_DELAY *)config.value)->mt_level_change_delay[1] =
          lc1_delay;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }

    if (TEST_BIT(md_config->enable_bits, AM_EVENT_MD_CONFIG_LEVEL1_CHANGE_DELAY)) {
      config.key = AM_MD_LEVEL_CHANGE_DELAY;
      config.value = malloc(sizeof(AM_EVENT_MD_LEVEL_CHANGE_DELAY));
      if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      uint32_t lc0_delay =
          ((AM_EVENT_MD_LEVEL_CHANGE_DELAY *)config.value)->mt_level_change_delay[0];
      memcpy(config.value,
             &md_config->level_change_delay,
             sizeof(md_config->level_change_delay));
      ((AM_EVENT_MD_LEVEL_CHANGE_DELAY *)config.value)->mt_level_change_delay[0] =
          lc0_delay;
      if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
        ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
        ret = -1;
        break;
      }
      free(config.value);
    }
    //sync and save to config file
    config.key = AM_MD_SYNC_CONFIG;
    config.value = malloc(sizeof(uint32_t));
    *(uint32_t*) config.value = 0;
    if (!g_event_monitor->set_monitor_config(EV_MOTION_DECT, &config)) {
      ERROR("failed to set monitor config id: %d\n", EV_MOTION_DECT);
      ret = -1;
      break;
    }
    free(config.value);

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

void ON_SERVICE_GET_EVENT_MOTION_DETECT_CONFIG(void *msg_data,
                                               int msg_data_size,
                                               void *result_addr,
                                               int result_max_size)
{
  INFO("event service get event motion detect config\n");
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

    am_event_md_config_s *md_config =
        (am_event_md_config_s*) service_result->data;
    EVENT_MODULE_CONFIG config =
    {0};
    uint32_t roi_id = *(uint32_t*) msg_data;

    config.key = AM_MD_ENABLE;
    config.value = malloc(sizeof(bool));
    if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
      ret = -1;
      break;
    }
    md_config->enable = *(bool *) config.value;
    free(config.value);

    config.key = AM_MD_THRESHOLD;
    config.value = malloc(sizeof(AM_EVENT_MD_THRESHOLD));
    ((AM_EVENT_MD_THRESHOLD*) config.value)->roi_id = roi_id;
    if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
      ret = -1;
      break;
    }
    memcpy(&md_config->threshold, config.value, sizeof(AM_EVENT_MD_THRESHOLD));
    free(config.value);

    config.key = AM_MD_LEVEL_CHANGE_DELAY;
    config.value = malloc(sizeof(AM_EVENT_MD_LEVEL_CHANGE_DELAY));
    ((AM_EVENT_MD_LEVEL_CHANGE_DELAY*) config.value)->roi_id = roi_id;
    if (!g_event_monitor->get_monitor_config(EV_MOTION_DECT, &config)) {
      ERROR("failed to get monitor config id: %d\n", EV_MOTION_DECT);
      ret = -1;
      break;
    }
    memcpy(&md_config->level_change_delay,
           config.value,
           sizeof(AM_EVENT_MD_LEVEL_CHANGE_DELAY));
    free(config.value);
  } while (0);

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
