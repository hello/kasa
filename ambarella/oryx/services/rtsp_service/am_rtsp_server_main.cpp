/*******************************************************************************
 * am_rtsp_server_main.cpp
 *
 * History:
 *   2015-1-19 - [Dong Shiming] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co,Ltd.
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
#include "am_rtsp_server_if.h"
#include "am_service_frame_if.h"
#include "am_rtsp_server_msg_map.h"

#include <signal.h>

enum
{
  IPC_API_PROXY = 0,
  IPC_COUNT
};

AM_SERVICE_STATE g_service_state  = AM_SERVICE_STATE_NOT_INIT;
AMIRtspServerPtr g_rtsp_server    = nullptr;
AMIPCBase       *g_ipc_base_obj[] = {nullptr};
AMIServiceFrame *g_service_frame  = nullptr;

static void sigstop(int arg)
{
  INFO("RTSP service got signal!");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_RTSP_NAME)  < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_RTSP_SERVER);
  ipc->complete();
  g_service_state = AM_SERVICE_STATE_INIT_IPC_CONNECTED;
  DEBUG("IPC create done for API_PROXY TO RTSP_SERVER, name is %s \n",
          AM_IPC_RTSP_NAME);
  return 0;
}

int clean_up()
{
  for (uint32_t i = 0;
      i < (sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]));
      ++ i) {
    delete g_ipc_base_obj[i];
  }
  return 0;
}

static void user_input_callback(char ch)
{
  switch(ch) {
    case 'Q':
    case 'q': {
      NOTICE("Quit RTSP.Service!");
      g_service_frame->quit();
    }break;
    default: break;
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;
  g_service_frame = AMIServiceFrame::create("RTSP.Service");

  if (AM_LIKELY(g_service_frame)) {
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    g_rtsp_server = AMIRtspServer::create();
    if (AM_LIKELY(g_rtsp_server != nullptr)) {
      AMPIDLock lock;
      bool run = true;
      if (AM_UNLIKELY((argc > 1) && is_str_equal(argv[1], "debug"))) {
        NOTICE("Running RTSP service in debug mode, press 'q' to exit!");
        g_service_frame->set_user_input_callback(user_input_callback);
        run = g_rtsp_server->start();
      } else {
        if (AM_LIKELY(lock.try_lock() < 0)) {
          ERROR("Unable to lock PID, RTSP service is already running!");
          ret = -1;
          run = false;
        } else {
          g_service_state = AM_SERVICE_STATE_INIT_PROCESS_CREATED;
          create_control_ipc();
          g_service_state = AM_SERVICE_STATE_INIT_DONE;
        }
      }
      if (AM_LIKELY(run)) {
        NOTICE("Entering RTSP service main loop!");
        g_service_frame->run(); /* Block here */
        NOTICE("Exit RTSP service main loop!");
        /* quit() of service frame is called, start destruction */
        g_rtsp_server->stop();
        clean_up();
      }
      g_rtsp_server = nullptr;
    } else {
      g_service_state = AM_SERVICE_STATE_ERROR;
      ERROR("Failed to create RTSP server instance!");
      ret = -2;
    }
    g_service_frame->destroy();
    g_service_frame = nullptr;
    PRINTF("RTSP service destroyed!");
  } else {
    ERROR("Failed to create service framework for RTSP service!");
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -3;
  }

  return ret;
}

