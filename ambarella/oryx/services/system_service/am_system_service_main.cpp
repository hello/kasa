/*******************************************************************************
 * am_system_service_main.cpp
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

#include "am_base_include.h"
#include <signal.h>
#include <thread>
#include "am_log.h"
#include "am_define.h"
#include "am_service_frame_if.h"
#include "am_system_service_msg_map.h"
#include "am_api_system.h"
#include "am_pid_lock.h"
#include "am_system_service_priv.h"
#include "am_upgrade_if.h"
#include "am_led_handler_if.h"

enum {
  IPC_API_PROXY = 0,
  IPC_STREAM,
  IPC_EVENT,
  IPC_IMAGE,
  IPC_COUNT
};

AMIPCBase *g_ipc_base_obj[] = {NULL, NULL, NULL, NULL};
AMIServiceFrame   *g_service_frame = NULL;
AM_SERVICE_STATE g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIFWUpgradePtr g_upgrade_fw = NULL;
AMILEDHandlerPtr g_ledhandler = NULL;
am_mw_ntp_cfg g_ntp_cfg = {{"",""}, 0, 0, 0, 0, false};
std::thread *g_th_ntp = NULL;

static void sigstop(int arg)
{
  INFO("system_service got signal\n");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_SYSTEM_NAME)  < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[IPC_API_PROXY] = ipc;
  }

  ipc->REGISTER_MSG_MAP(API_PROXY_TO_SYSTEM_SERVICE);
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO SYSTEM_SERVICE\n");

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
  int ret  = 0;
  do {
    AMPIDLock lock;

    g_service_frame = AMIServiceFrame::create("System.Service");
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for System.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    if (lock.try_lock() < 0) {
      ERROR("Unable to lock PID, System.Service should be running already");
      ret = -4;
      break;
    }

    create_control_ipc();
    g_service_state = AM_SERVICE_STATE_INIT_DONE;
    NOTICE("Entering System.Service main loop!");
    g_service_frame->run(); /* block here */
    g_upgrade_fw = nullptr;
    g_ledhandler = nullptr;
    /* quit ntp thread if running */
    if (g_ntp_cfg.enable) {
      g_ntp_cfg.enable = false;
    }
    if (g_th_ntp) {
      g_th_ntp->join();
      delete g_th_ntp;
      g_th_ntp = NULL;
    }
    clean_up();
    ret = 0;
  } while (0);

  if (AM_LIKELY(g_service_frame)) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("System Service destroyed!");

  return ret;
}


