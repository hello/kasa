/*******************************************************************************
 * am_audio_service_msg_action.cpp
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_audio_device_if.h"
#include "am_service_frame_if.h"
#include "am_api_audio.h"

extern AM_SERVICE_STATE  g_service_state;
extern AMIAudioDevicePtr g_audio_device;
extern AMIServiceFrame  *g_service_frame;

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  if (g_audio_device == nullptr) {
    ERROR("Failed to initialize audio service!");
    g_service_state = AM_SERVICE_STATE_ERROR;
  }
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("Audio service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize Audio service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Audio service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  PRINTF("ON AUDIO SERVICE DESTROY.");
  g_service_frame->quit(); /* Make run() in main function quit */
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  /*
   * Audio service is just a auxiliary service, so there
   * are no additional operations need to be done in
   * this command.
   */
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  g_service_state = AM_SERVICE_STATE_STARTED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Audio service has been started!");
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON AUDIO SERVICE STOP.");
  /* The same to ON_SERVICE_START. */
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  g_service_state = AM_SERVICE_STATE_STOPPED;
  service_result->ret = 0;
  service_result->state = g_service_state;

  INFO("Audio service has been stopped!");
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
  INFO("Audio Service status is: %d\n", service_result->state);
}

void ON_AUDIO_SAMPLE_RATE_GET(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_format_t audio_format;

  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR) ||
      (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = -1;
    INFO("Audio service has not been started!");
  } else {
    if (g_audio_device != nullptr) {
      memset(&audio_format, 0, sizeof(audio_format));
      if (!g_audio_device->get_sample_rate(&audio_format.sample_rate)) {
        ERROR("Failed to get sample rate");
        service_result->ret = -1;
      } else {
        service_result->ret = 0;
        memcpy(service_result->data, &audio_format, sizeof(audio_format));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  }
}

void ON_AUDIO_SAMPLE_RATE_SET(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  service_result->ret = -1;
  INFO("Pulsoaudio server's sample rate can't be changed.");
}

void ON_AUDIO_CHANNEL_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_format_t audio_format;

  service_result = (am_service_result_t *) result_addr;
  if ((g_service_state == AM_SERVICE_STATE_ERROR)
      || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
    service_result->ret = -1;
    INFO("Audio service has not been started!");
  } else {
    if (g_audio_device != nullptr) {
      memset(&audio_format, 0, sizeof(audio_format));
      if (!g_audio_device->get_channels(&audio_format.channels)) {
        ERROR("Failed to get sample rate");
        service_result->ret = -1;
      } else {
        service_result->ret = 0;
        memcpy(service_result->data, &audio_format, sizeof(audio_format));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  }
}

void ON_AUDIO_CHANNEL_SET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;

  service_result->ret = -1;
  INFO("Pulsoaudio server's channels can't be changed.");
}

void ON_AUDIO_DEVICE_VOLUME_GET_BY_INDEX(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->get_volume_by_index(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          &audio_device->volume,
          audio_device->index)) {
        ERROR("Failed to get volume by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
        memcpy(service_result->data, audio_device, sizeof(am_audio_device_t));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_VOLUME_SET_BY_INDEX(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->set_volume_by_index(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          audio_device->volume,
          audio_device->index)) {
        ERROR("Failed to set volume by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_VOLUME_GET_BY_NAME(void *msg_data,
                                        int msg_data_size,
                                        void *result_addr,
                                        int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->get_volume_by_name(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          &audio_device->volume,
          audio_device->name)) {
        ERROR("Failed to get volume by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
        memcpy(service_result->data, audio_device, sizeof(am_audio_device_t));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_VOLUME_SET_BY_NAME(void *msg_data,
                                        int msg_data_size,
                                        void *result_addr,
                                        int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->set_volume_by_name(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          audio_device->volume,
          audio_device->name)) {
        ERROR("Failed to get volume by device's name");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_MUTE_GET_BY_INDEX(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;
  bool mute = false;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->is_muted_by_index(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          &mute,
          audio_device->index)) {
        ERROR("Failed to get mute info by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
        audio_device->mute = mute ? 1 : 0;
        memcpy(service_result->data, audio_device, sizeof(am_audio_device_t));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_MUTE_SET_BY_INDEX(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!audio_device->mute) {
        if (!g_audio_device->unmute_by_index(
            AM_AUDIO_DEVICE_TYPE(audio_device->type),
            audio_device->index)) {
          ERROR("Failed to unmute audio device by index!");
          service_result->ret = -1;
          break;
        } else {
          service_result->ret = 0;
        }
      } else {
        if (!g_audio_device->mute_by_index(
            AM_AUDIO_DEVICE_TYPE(audio_device->type),
            audio_device->index)) {
          ERROR("Failed to mute audio device by index!");
          service_result->ret = -1;
          break;
        } else {
          service_result->ret = 0;
        }
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_MUTE_GET_BY_NAME(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;
  bool mute = false;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->is_muted_by_name(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          &mute,
          audio_device->name)) {
        ERROR("Failed to get mute info by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
        audio_device->mute = mute ? 1 : 0;
        memcpy(service_result->data, audio_device, sizeof(am_audio_device_t));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_MUTE_SET_BY_NAME(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!audio_device->mute) {
        if (!g_audio_device->unmute_by_name(
            AM_AUDIO_DEVICE_TYPE(audio_device->type),
            audio_device->name)) {
          ERROR("Failed to unmute audio device by index!");
          service_result->ret = -1;
          break;
        } else {
          service_result->ret = 0;
        }
      } else {
        if (!g_audio_device->mute_by_name(
            AM_AUDIO_DEVICE_TYPE(audio_device->type),
            audio_device->name)) {
          ERROR("Failed to mute audio device by index!");
          service_result->ret = -1;
          break;
        } else {
          service_result->ret = 0;
        }
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_NAME_GET_BY_INDEX(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      if (!g_audio_device->get_name_by_index(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          audio_device->index,
          audio_device->name,
          AM_AUDIO_DEVICE_NAME_MAX_LEN)) {
        ERROR("Failed to get name by device's index");
        service_result->ret = -1;
        break;
      } else {
        service_result->ret = 0;
        memcpy(service_result->data, audio_device, sizeof(am_audio_device_t));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

void ON_AUDIO_DEVICE_INDEX_LIST_GET(void *msg_data,
                                    int msg_data_size,
                                    void *result_addr,
                                    int result_max_size)
{
  am_service_result_t *service_result = nullptr;
  am_audio_device_t *audio_device = nullptr;
  int dev_index_list[AM_AUDIO_DEVICE_NUM_MAX];

  do {
    service_result = (am_service_result_t *) result_addr;
    if ((g_service_state == AM_SERVICE_STATE_ERROR)
        || (g_service_state == AM_SERVICE_STATE_STOPPED)) {
      ERROR("Audio service has not been started!");
      service_result->ret = -1;
      break;
    }

    if ((msg_data == nullptr) || (msg_data_size != sizeof(am_audio_device_t))) {
      ERROR("Invalid argument: msg_data_size != sizeof (am_audio_device_t)");
      service_result->ret = -1;
      break;
    }

    audio_device = (am_audio_device_t *) msg_data;
    if (audio_device->type > ((uint8_t) AM_AUDIO_DEVICE_SOURCE_OUTPUT)) {
      ERROR("No such audio device: %d", audio_device->type);
      service_result->ret = -1;
      break;
    }

    if (g_audio_device != nullptr) {
      service_result->ret = AM_AUDIO_DEVICE_NUM_MAX;
      if (!g_audio_device->get_index_list(
          AM_AUDIO_DEVICE_TYPE(audio_device->type),
          dev_index_list,
          &service_result->ret)) {
        ERROR("Failed to get list of audio device: %d!", audio_device->type);
        service_result->ret = -1;
        break;
      } else {
        memcpy(service_result->data,
               dev_index_list,
               service_result->ret * sizeof(int));
      }
    } else {
      ERROR("Audio service is not initialized!");
      service_result->ret = -1;
    }
  } while (0);
}

#if 0
void ON_AUDIO_INPUT_SELECT_GET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  // To be done;
  INFO("on audio input select get.");
}

void ON_AUDIO_INPUT_SELECT_SET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  // To be done;
  INFO("on audio input select set.");
}

void ON_AUDIO_AEC_GET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  // To be done;
  INFO("on audio aec get.");
}

void ON_AUDIO_AEC_SET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  // To be done;
  INFO("on audio aec set.");
}

void ON_AUDIO_EFFECT_GET(void *msg_data,
                         int msg_data_size,
                         void *result_addr,
                         int result_max_size)
{
  // To be done;
  INFO("on audio effect get.");
}

void ON_AUDIO_EFFECT_SET(void *msg_data,
                         int msg_data_size,
                         void *result_addr,
                         int result_max_size)
{
  // To be done;
  INFO("on audio effect set.");
}
#endif
