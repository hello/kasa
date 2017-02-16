/*******************************************************************************
 * am_media_service_main.cpp
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

#include "am_pid_lock.h"
#include "am_service_frame_if.h"
#include "am_media_service_msg_map.h"
#include "am_media_service_instance.h"

#include <signal.h>

enum
{
  IPC_API_PROXY = 0,
  IPC_COUNT
};

AM_SERVICE_STATE g_service_state  = AM_SERVICE_STATE_NOT_INIT;
AMMediaService  *g_media_instance = nullptr;
AMIPCBase       *g_ipc_base_obj[] = {nullptr};
AMIServiceFrame *g_service_frame  = nullptr;

static void sigstop(int arg)
{
  INFO("Media.Service got signal");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_MEDIA_NAME) < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_MEDIA_SERVICE);
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO MEDIA_SERVICE, name is %s \n",
        AM_IPC_MEDIA_NAME);
  return 0;
}

int clean_up()
{
  for (uint32_t i = 0;
      i < sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]); ++ i) {
    delete g_ipc_base_obj[i];
  }
  return 0;
}

static void user_input_callback(char ch)
{
  switch(ch) {
    case 'Q':
    case 'q': {
      NOTICE("Quit Media.Service!");
      g_service_frame->quit();
    }break;
    default: break;
  }
}

int main(int argc, char *argv[])
{
  int ret = -1;
  g_service_frame = AMIServiceFrame::create("Media.Service");

  if (AM_LIKELY(g_service_frame)) {
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    g_media_instance = AMMediaService::create(NULL);

    if (AM_LIKELY(g_media_instance)) {
      AMPIDLock lock;
      bool run = true;
      if (AM_UNLIKELY((argc > 1) && is_str_equal(argv[1], "debug"))) {
        NOTICE("Running Media service in debug mode, press 'q' to exit!");
        g_service_frame->set_user_input_callback(user_input_callback);
      } else {
        if (AM_UNLIKELY(lock.try_lock() < 0)) {
          ERROR("Unable to lock PID, Media service is already running!");
          ret = -1;
          run = false;
        } else {
          create_control_ipc();
          g_service_state = AM_SERVICE_STATE_INIT_DONE;
        }
      }
      if (AM_LIKELY(run)) {
        if (!g_media_instance->start_media()) {
          ERROR("Media service: start media failed!");
          g_service_state = AM_SERVICE_STATE_ERROR;
        } else {
          g_service_state = AM_SERVICE_STATE_STARTED;
        }
        g_service_frame->run(); /* Block here */
        /* quit() of service frame is called, start destruction */
        g_media_instance->stop_media();
        clean_up();
      }
      g_media_instance->destroy();
      g_media_instance = nullptr;
    } else {
      g_service_state = AM_SERVICE_STATE_ERROR;
      ERROR("Failed to create AMMediaService instance!");
      ret = -2;
    }
    g_service_frame->destroy();
    g_service_frame = nullptr;
    PRINTF("Media service destroyed!");
  } else {
    ERROR("Failed to create serivce framework for Media service!");
    g_service_state = AM_SERVICE_STATE_ERROR;
  }

  return ret;
}
