/*******************************************************************************
 * am_audio_service_main.cpp
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
#include "am_audio_service_msg_map.h"
#include "am_pid_lock.h"
#include "am_audio_device_if.h"
#include "am_service_frame_if.h"
#include <signal.h>
enum
{
  IPC_API_PROXY = 0,
  IPC_COUNT
};

AM_SERVICE_STATE  g_service_state  = AM_SERVICE_STATE_NOT_INIT;
AMIAudioDevicePtr g_audio_device   = nullptr;
AMIServiceFrame  *g_service_frame  = nullptr;
AMIPCBase        *g_ipc_base_obj[] = {nullptr};

static void sigstop(int arg)
{
  INFO("audio_service got signal\n");
}

static int create_control_ipc()
{
  DEBUG("Audio service create_control_ipc\n");
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_AUDIO_NAME)  < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_AUDIO_SERVICE);
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO AUDIO_SERVICE, name is %s \n",
        AM_IPC_AUDIO_NAME);
  return 0;
}

int clean_up()
{
  for (uint32_t i = 0; i < sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]);
      i ++) {
    delete g_ipc_base_obj[i];
  }
  return 0;
}

int main(int argc, char *argv[])
{
  int ret = 0;
  g_service_frame = AMIServiceFrame::create("Audio.Service");

  if (AM_LIKELY(g_service_frame)) {
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    g_audio_device = AMIAudioDevice::create("pulse");
    if (AM_LIKELY(g_audio_device != nullptr)) {
      AMPIDLock lock;

      if (AM_LIKELY(lock.try_lock() < 0)) {
        ERROR("Unable to lock PID, RTSP service is already running!");
        ret = -1;
      } else {
        g_service_state = AM_SERVICE_STATE_INIT_PROCESS_CREATED;
        create_control_ipc();
        g_service_state = AM_SERVICE_STATE_INIT_DONE;
        NOTICE("Entering Audio service main loop!");
        g_service_frame->run(); /* Block here */
        NOTICE("Exit Audio service main loop!");
        /* quit() of service frame is called, start destruction */
        clean_up();
      }
      g_audio_device = nullptr;
    } else {
      g_service_state = AM_SERVICE_STATE_ERROR;
      ERROR("Failed to create Audio server instance!");
      ret = -2;
    }
    g_service_frame->destroy();
    g_service_frame = nullptr;
    PRINTF("Audio service destroyed!");
  } else {
    ERROR("Failed to create service framework for Audio service!");
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -3;
  }

  return ret;
}
