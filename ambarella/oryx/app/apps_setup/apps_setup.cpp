/**
 * apps_setup.cpp
 *
 *  History:
 *    Jul 14, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "am_event_types.h"
#include "am_playback_if.h"
#include "am_playback_msg.h"
#include "am_event_monitor_if.h"
#include "am_api_helper.h"
#include "am_air_api.h"
#include "am_event.h"

#define KEY1_VALUE 116
#define KEY2_VALUE 238

static AMIPlaybackPtr g_player = nullptr;
static AMAPIHelperPtr g_api_helper = nullptr;
static AMIEventMonitorPtr g_event_instance = nullptr;
static AMEvent *g_event = nullptr;

enum  {
  APPS_RUNNING = 0,
  APPS_STOPED
} apps_state = APPS_STOPED;

#ifndef VOICE_PATH
#define VOICE_PATH "/usr/share/"
#endif

#define START_VOICE_INDEX   0
#define STOP_VOICE_INDEX    1

static const char *sound_path[] = {
  VOICE_PATH"soundfx/wav/start_rec.wav",
  VOICE_PATH"soundfx/wav/stop_rec.wav",
  VOICE_PATH"soundfx/wav/success3.wav",
};

static bool play_audio(int index)
{
  bool ret = true;
  AMPlaybackUri play_uri;
  memcpy(play_uri.media.file, sound_path[index], strlen(sound_path[index]));
  play_uri.type = AM_PLAYBACK_URI_FILE;
  if (!g_player->play(&play_uri)) {
    ERROR("Failed to playback audio!");
    ret = false;
  } else {
    g_event->wait();
  }
  return ret;
}

static void sound_callback(AMPlaybackMsg &msg)
{
  switch (msg.msg) {
    case AM_PLAYBACK_MSG_START_OK:
      NOTICE("Effect playback engine started successfully!\n");
      break;
    case AM_PLAYBACK_MSG_PAUSE_OK:
      NOTICE("Effect playback engine paused successfully!\n");
      break;
    case AM_PLAYBACK_MSG_STOP_OK:
      NOTICE("Effect playback engine stopped successfully!\n");
      break;
    case AM_PLAYBACK_MSG_EOS:
      NOTICE("Effect playback finished successfully!\n");
      g_event->signal();
      break;
    case AM_PLAYBACK_MSG_ABORT:
      NOTICE("Effect playback engine abort!\n");
      break;
    case AM_PLAYBACK_MSG_ERROR:
      NOTICE("Effect playback engine abort due to fatal error occurred!\n");
      break;
    case AM_PLAYBACK_MSG_TIMEOUT:
      NOTICE("Effect playback engine is timeout!\n");
      break;
    case AM_PLAYBACK_MSG_NULL:
    default:
      break;
  }
}

static int32_t event_callback(AM_EVENT_MESSAGE *msg)
{
  int ret = 0;
  uint32_t muxer_id = 1 << 0;
  if (msg->event_type == EV_KEY_INPUT_DECT &&
      msg->key_event.key_value == KEY1_VALUE) {
    switch (msg->key_event.key_state) {
      case AM_KEY_LONG_PRESSED:
        NOTICE("Key:%d long pressed!", KEY1_VALUE);
        if (apps_state == APPS_STOPED) {
          play_audio(START_VOICE_INDEX);
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
                                    &muxer_id, sizeof(muxer_id),
                                    &ret, sizeof(int));
          if (ret < 0) {
            ERROR("Failed to start file recording!");
            break;
          }
          apps_state = APPS_RUNNING;
        } else {
          play_audio(STOP_VOICE_INDEX);
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
                                    &muxer_id, sizeof(muxer_id),
                                    &ret, sizeof(int));
          if (ret < 0) {
            ERROR("Failed to stop file recording!");
            break;
          }
          apps_state = APPS_STOPED;
        }
        break;
      case AM_KEY_CLICKED:
        NOTICE("Key:%d clicked!", KEY1_VALUE);
        break;
      default:
        break;
    }
  }
  return ret;
}

static bool init()
{
  bool ret = false;
  do {
    if (!(g_event = AMEvent::create())) {
      ERROR("Failed to create AMEvent!");
      break;
    }

    if (!(g_api_helper = AMAPIHelper::get_instance())) {
      ERROR("Failed to get AMAPIHelper instance!");
      break;
    }

    if (!(g_player = AMIPlayback::create())) {
      ERROR("Failed to create AMIPlayback!");
      break;
    }
    if (!g_player->init()) {
      ERROR("Failed to init AMPlayback!");
      break;
    } else {
      g_player->set_msg_callback(sound_callback, nullptr);
    }

    if (!(g_event_instance = AMIEventMonitor::get_instance())) {
      ERROR("Failed to get AMIEventMonitor instance!");
      break;
    }

    EVENT_MODULE_CONFIG module_config;
    AM_KEY_INPUT_CALLBACK key_callback;

    key_callback.callback = event_callback;
    module_config.key = AM_KEY_CALLBACK;
    module_config.value = (void*)&key_callback;

    key_callback.key_value = KEY1_VALUE;
    g_event_instance->set_monitor_config(EV_KEY_INPUT_DECT, &module_config);
    key_callback.key_value = KEY2_VALUE;
    g_event_instance->set_monitor_config(EV_KEY_INPUT_DECT, &module_config);
    if (!g_event_instance->start_event_monitor(EV_KEY_INPUT_DECT)) {
      ERROR("Failed to start event monitor!");
      break;
    }
    ret = true;
  } while (0);
  return ret;
}

static bool deinit()
{
  bool ret = true;
  g_event->destroy();
  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  if (!init()) {
    ret = -1;
  }
  pause();
  deinit();
  return ret;
}
