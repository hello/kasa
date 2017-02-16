/*******************************************************************************
 * am_network_service_main.cpp
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_network_service_msg_map.h"
#include "am_pid_lock.h"
#include "am_network_service_priv.h"
#include "am_service_frame_if.h"
enum {
  IPC_API_PROXY = 0,
  IPC_COUNT
};

AMIPCBase *g_ipc_base_obj[] = {NULL};
AM_SERVICE_STATE g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIServiceFrame *g_service_frame  = nullptr;

static void sigstop(int arg)
{
  INFO("network_service got signal\n");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_NETWORK_NAME)  < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_NETWORK_SERVICE);
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO NETWORK_SERVICE, name is %s \n", AM_IPC_NETWORK_NAME);
  return 0;
}

int clean_up()
{
    uint32_t i;
    for (i = 0 ; i < sizeof(g_ipc_base_obj)/sizeof(g_ipc_base_obj[0]); i++)
      delete g_ipc_base_obj[i];
    exit(1);
    return 0;
}

int main()
{
  int ret = 0;
  do {
    g_service_frame = AMIServiceFrame::create("Network.Service");
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for Network.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    AMPIDLock lock;
    if (lock.try_lock() < 0) {
       ERROR("unable to lock PID, same name process should be running already\n");
       ret = -1;
       break;
    }
    create_control_ipc();
    //sleep(2);   //simulate the extra delay needed,  TODO: anything that must
              //take time to do for init process
    g_service_state = AM_SERVICE_STATE_INIT_DONE;
    g_service_frame->run(); /* block here */
    clean_up();
    ret = 0;
  } while(0);
  if (AM_LIKELY(g_service_frame)) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("Network Service destroyed!");
  //just use sigstop to capture SIGNAL and trigger pause
  //main thread just pause and do nothing
  // while(1)
    // pause();

  return ret;
}
