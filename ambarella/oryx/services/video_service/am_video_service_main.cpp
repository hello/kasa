/*******************************************************************************
 * am_video_service_main.cpp
 *
 * History:
 *   2014-9-16 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_pid_lock.h"
#include "am_service_frame_if.h"
#include "am_video_camera_if.h"
#include "am_video_service_msg_map.h"

enum {
  IPC_API_PROXY = 0,
  IPC_COUNT
};

AM_SERVICE_STATE g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIPCBase *g_ipc_base_obj[IPC_COUNT] = {NULL};
AMIServiceFrame *g_service_frame = nullptr;
AMIVideoCameraPtr g_video_camera = nullptr;

static void sigstop(int arg)
{
  INFO("video_service got signal\n");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_VIDEO_NAME)  < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_VIDEO_SERVICE);
  ipc->complete();
  g_service_state = AM_SERVICE_STATE_INIT_IPC_CONNECTED;
  DEBUG("IPC create done for API_PROXY TO VIDEO_SERVICE, name is %s \n",
        AM_IPC_VIDEO_NAME);
  return 0;
}

int clean_up()
{
  for (uint32_t i = 0 ;
      i < sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]);
      i++) {
    delete g_ipc_base_obj[i];
  }

  return 0;
}

static void user_input_callback(char ch)
{
  switch (ch) {
    case 'Q':
    case 'q':
      NOTICE("Quit Video Service!");
      g_service_frame->quit();
      break;
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;

  do {
    if (!(g_service_frame = AMIServiceFrame::create("Video.Service"))) {
      ERROR("Failed to create service framework for Video Service!");
      ret = -1;
      break;
    }

    if (!(g_video_camera = AMIVideoCamera::get_instance())) {
      ERROR("Fail to get AMVideoCamera instance\n");
      ret = -1;
      break;
    }
    if (g_video_camera->init() != AM_RESULT_OK) {
      ERROR("Video Camera init failed\n");
      ret = -1;
      break;
    }

    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    AMPIDLock lock;
    if ((argc > 1) && is_str_equal(argv[1], "debug")) {
      NOTICE("Running Video Service in debug mode, press 'q' to exit!");
      g_service_frame->set_user_input_callback(user_input_callback);
    } else {
      if (lock.try_lock() < 0) {
        ERROR("Unable to lock PID, Video.Service should be running already");
        ret = -1;
        break;
      }
      create_control_ipc();
      g_service_state = AM_SERVICE_STATE_INIT_DONE;
    }
    NOTICE("Entering Video Service main loop!");
    if (g_video_camera->start() != AM_RESULT_OK) {
      ERROR("Video Service: start failed\n");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STARTED;
    }
    g_service_frame->run();
    g_video_camera = nullptr;
    clean_up();
    NOTICE("Exit Video Service main loop!");
  } while (0);

  if (g_service_frame) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("Video Service destroyed!");

  return ret;
}
