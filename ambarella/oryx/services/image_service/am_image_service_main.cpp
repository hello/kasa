/*******************************************************************************
 * am_image_service_main.cpp
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
#include "am_configure.h"
#include "am_thread.h"
#include "am_image_service_msg_map.h"
#include "am_pid_lock.h"
#include "am_service_frame_if.h"
#include "am_image_quality_if.h"

#include <signal.h>

enum
{
  IPC_API_PROXY = 0,
  IPC_COUNT
};
AM_SERVICE_STATE   g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIImageQualityPtr g_image_quality = nullptr;
AMIServiceFrame   *g_service_frame = nullptr;
static AMIPCBase *g_ipc_base_obj[] = { NULL };

static void sigstop(int arg) //ignore signal, do real quit job in clean_up()
{
  INFO("img_svc received signal\n");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_IMAGE_NAME) < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_IMAGE_SERVICE);
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO IMAGE_SERVICE, name is %s\n",
        AM_IPC_IMAGE_NAME);

  return 0;
}

int clean_up(void)
{
  for (uint32_t i = 0; i < sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]);
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
      NOTICE("Quit Image.Service!");
      g_service_frame->quit();
    }break;
    default: break;
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;
  do {
    AMPIDLock lock;

    g_service_frame = AMIServiceFrame::create("Image.Service");
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for Image.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    g_image_quality = AMIImageQuality::get_instance();
    if (AM_UNLIKELY(!g_image_quality)) {
      ERROR("Failed to get AMImageQuality instance!");
      ret = -2;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    if (AM_UNLIKELY((argc > 1) && is_str_equal(argv[1], "debug"))) {
      NOTICE("Running Image Service in debug mode, press 'q' to exit!");
      g_service_frame->set_user_input_callback(user_input_callback);
    } else {
      if (lock.try_lock() < 0) {
        ERROR("Unable to lock PID, Event.Service should be running already");
        ret = -4;
        break;
      }
      create_control_ipc();
    }
    if (!g_image_quality->start()) {
      ERROR("Image Service: start failed\n");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STARTED;
    }
    NOTICE("Entering Event.Service main loop!");
    g_service_frame->run(); /* block here */
    g_image_quality = nullptr;
    clean_up();
    ret = 0;
  }while (0);

  if (AM_LIKELY(g_service_frame)) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("Image Service destroyed!");

  return ret;
}
