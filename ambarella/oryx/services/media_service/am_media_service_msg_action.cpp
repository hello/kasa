/*******************************************************************************
 * am_media_service_msg_action.cpp
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
#include "am_log.h"
#include "am_service_impl.h"
#include "am_service_frame_if.h"
#include "am_media_service_instance.h"
#include "am_media_service_data_structure.h"

extern AM_SERVICE_STATE g_service_state;
extern AMMediaService  *g_media_instance;
extern AMIServiceFrame *g_service_frame;

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  //if it's init done, then
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("Media service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize Media service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("Media service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int ret = 0;
  PRINTF("ON MEDIA SERVICE DESTROY.");
  if (g_media_instance) {
    g_media_instance->stop_media();
    g_service_state = AM_SERVICE_STATE_STOPPED;
    g_service_frame->quit();
  } else {
    ERROR("Media service: Failed to get AMMediaService instance!");
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -1;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  PRINTF("ON MEDIA SERVICE START.");
  if (!g_media_instance) {
    ERROR("media instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if (g_service_state == AM_SERVICE_STATE_STARTED) {
      INFO("media instance is already started.");
    } else {
      if (!g_media_instance->start_media()) {
        ERROR("Media service: start media failed!");
        g_service_state = AM_SERVICE_STATE_ERROR;
      } else {
        g_service_state = AM_SERVICE_STATE_STARTED;
      }
    }
  }
  //if it's init done, then
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE STOP.");
  if (!g_media_instance) {
    ERROR("AMMediaInstance instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if(!g_media_instance->stop_media()) {
      ERROR("Media service: stop media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STOPPED;
    }
  }
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_RESTART(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE RESTART.");
  if (!g_media_instance) {
    ERROR("media instance is not created.");
    g_service_state = AM_SERVICE_STATE_ERROR;
  } else {
    if (!g_media_instance->stop_media()) {
      ERROR("Media service: stop media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else if(!g_media_instance->start_media()) {
      ERROR("Media service: start media failed!");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STARTED;
    }
  }
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_STATUS(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  PRINTF("ON MEDIA SERVICE STATUS.");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START(void* msg_data,
                                                  int msg_data_size,
                                                  void *result_addr,
                                                  int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START");
  int32_t ret = !g_media_instance->send_event();
  if (result_addr) {
     memcpy(result_addr, &ret, sizeof(int32_t));
   }
}

void ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE(void* msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE");
  int ret = 0;
  AudioFileList* audio_file = (AudioFileList*)msg_data;
  AMIPlaybackPtr g_playback = g_media_instance->get_playback_instance();
  do{
    if(audio_file->file_number == 0) {
      ERROR("file list is empty\n");
      ret = -1;
      break;
    }
    if(g_playback->is_paused() || g_playback->is_playing()) {
      break;
    }
    INFO("audio file num: %d", audio_file->file_number);
    for(uint32_t i = 0; i< audio_file->file_number; ++ i) {
      INFO("file name :%s",audio_file->file_list[i]);
      AMPlaybackUri uri;
      uri.type = AM_PLAYBACK_URI_FILE;
      memcpy(uri.media.file, audio_file->file_list[i],
             sizeof(audio_file->file_list[i]));
      if(!g_playback->add_uri(uri)) {
        ERROR("Failed to add %s to play list",
               audio_file->file_list[i]);
        ret = -1;
        break;
      }
    }
    if(ret == -1) {
      break;
    }
  }while(0);
  if (result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  AMIPlaybackPtr g_playback = g_media_instance->get_playback_instance();
  do{
    if(g_playback->is_paused()) {
      if(!g_playback->pause(false)) {
        PRINTF("Failed to resume playback.");
        ret = -1;
        break;
      }
    } else if(g_playback->is_playing()) {
      PRINTF("Playback is already playing.");
      break;
    } else {
      if(!g_playback->play()) {
        PRINTF("Failed to start playing.");
        ret = -1;
        break;
      }
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  AMIPlaybackPtr g_playback = g_media_instance->get_playback_instance();
  do{
    if(!g_playback->pause(true)) {
      PRINTF("Failed to pause playing.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                     int msg_data_size,
                                                     void *result_addr,
                                                     int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE");
  int ret = 0;
  AMIPlaybackPtr g_playback = g_media_instance->get_playback_instance();
  do {
    if(!g_playback->stop()) {
      PRINTF("Failed to stop playing.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if(g_record->is_recording()) {
      PRINTF("The media service is already recording.");
      break;
    }
    if(!g_record->start()) {
      PRINTF("Failed to start recording.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if(!g_record->is_recording()) {
      PRINTF("The media service recording is already stopped.");
      break;
    }
    if(!g_record->stop()) {
      PRINTF("Failed to stop recording.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING(void *msg_data,
                                                 int msg_data_size,
                                                 void *result_addr,
                                                 int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if(!g_record->is_recording()) {
      PRINTF("The media service is not recording, please start recording first");
      break;
    }
    if(!g_record->start_file_recording()) {
      PRINTF("Failed to start file recording.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}

void ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING(void *msg_data,
                                                int msg_data_size,
                                                void *result_addr,
                                                int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING");
  int ret = 0;
  AMIRecordPtr g_record = g_media_instance->get_record_instance();
  do {
    if(!g_record->is_recording()) {
      PRINTF("The media service is not recording, please start recording first");
      break;
    }
    if(!g_record->stop_file_recording()) {
      PRINTF("Failed to stop file recording.");
      ret = -1;
      break;
    }
  }while(0);
  if(result_addr) {
    memcpy(result_addr, &ret, sizeof(int));
  }
}
