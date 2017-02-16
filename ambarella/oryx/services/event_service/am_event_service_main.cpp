/*******************************************************************************
 * am_event_service_main.cpp
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

#include "am_event_monitor_if.h"
#include "am_service_frame_if.h"
#include "am_pid_lock.h"
#include "am_event_service_msg_map.h"
#include "am_event_service_priv.h"

#include <signal.h>

AM_SERVICE_STATE   g_service_state               = AM_SERVICE_STATE_NOT_INIT;
AMIEventMonitorPtr g_event_monitor               = nullptr;
AMIServiceFrame   *g_service_frame               = nullptr;
AMIPCBase         *g_ipc_base_obj[EVT_IPC_COUNT] = {NULL};

static void sigstop(int arg)
{
  INFO("event_service got signal\n");
}

static int create_control_ipc()
{
  AMIPCSyncCmdServer *ipc = new AMIPCSyncCmdServer();
  if (ipc && ipc->create(AM_IPC_EVENT_NAME) < 0) {
    ERROR("receiver create failed \n");
    delete ipc;
    return -1;
  } else {
    g_ipc_base_obj[EVT_IPC_API_PROXY] = ipc;
  }
  ipc->REGISTER_MSG_MAP(API_PROXY_TO_EVENT_SERVICE)
  ipc->complete();
  DEBUG("IPC create done for API_PROXY TO EVENT_SERVICE, name is %s \n",
        AM_IPC_EVENT_NAME);
  return 0;
}

int clean_up()
{
  for (uint32_t i = 0;
      i < sizeof(g_ipc_base_obj) / sizeof(g_ipc_base_obj[0]);
      i ++) {
    delete g_ipc_base_obj[i];
  }
  return 0;
}

int main()
{
  int ret = 0;
  do {
    AMPIDLock lock;
    g_service_frame = AMIServiceFrame::create("Event.Service");
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for Event.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    g_event_monitor = AMIEventMonitor::get_instance();
    if (AM_UNLIKELY(!g_event_monitor)) {
      ERROR("Failed to get AMEventMonitor instance!");
      ret = -2;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    if (lock.try_lock() < 0) {
      ERROR("Unable to lock PID, Event.Service should be running already");
      ret = -3;
      break;
    }
    create_control_ipc();
    g_service_state = AM_SERVICE_STATE_INIT_DONE;
    NOTICE("Entering Event.Service main loop!");
    g_service_frame->run(); /* block here */
    if (AM_UNLIKELY(!g_event_monitor->destroy_all_event_monitor())) {
      ERROR("Failed to destroy all event modules!");
      ret = -4;
    } else {
      ret = 0;
    }
    g_event_monitor = nullptr;
    clean_up();
  } while (0);

  if (AM_LIKELY(g_service_frame)) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("Event Service destroyed!");

  return ret;
}
