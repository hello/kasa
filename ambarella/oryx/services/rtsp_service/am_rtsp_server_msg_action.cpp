/*******************************************************************************
 * am_rtsp_server_msg_action.cpp
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
#include "am_log.h"
#include "commands/am_service_impl.h"
#include "am_rtsp_server_if.h"
#include "am_service_frame_if.h"

#include <signal.h>

extern AM_SERVICE_STATE g_service_state;
extern AMIRtspServerPtr g_rtsp_server;
extern AMIServiceFrame *g_service_frame;

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("RTSP service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize RTSP service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("RTSP service is still initializing...");
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
  PRINTF("ON RTSP SERVICE DESTROY");
  if (!g_rtsp_server) {
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -1;
    ERROR("RTSP service: Failed to get AMRtspServer instance!");
  } else {
    g_rtsp_server->stop();
    g_service_state = AM_SERVICE_STATE_STOPPED;
    g_service_frame->quit(); /* make run() in main function quit */
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  if (!g_rtsp_server) {
    ret = -1;
    g_service_state = AM_SERVICE_STATE_ERROR;
    ERROR("RTSP service: Failed to get AMRtspServer instance!");
  } else if (!g_rtsp_server->start()) {
    ERROR("RTSP Server: start failed!");
    ret = -2;
  } else {
    g_service_state = AM_SERVICE_STATE_STARTED;
  }

  service_result->ret = ret;
  service_result->state = g_service_state;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  if (!g_rtsp_server) {
    ret = -1;
    g_service_state = AM_SERVICE_STATE_ERROR;
    ERROR("RTSP service: Failed to get AMRtspServer instance!");
  } else {
    g_rtsp_server->stop();
    g_service_state = AM_SERVICE_STATE_STOPPED;
  }
  service_result->ret = ret;
  service_result->state = g_service_state;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  INFO("RTSP server restart");
  g_rtsp_server->stop();
  g_service_state = AM_SERVICE_STATE_STOPPED;
  if (g_rtsp_server->start()) {
    g_service_state = AM_SERVICE_STATE_STARTED;
    service_result->ret = 0;
  } else {
    g_service_state = AM_SERVICE_STATE_ERROR;
    service_result->ret = -1;
  }
  service_result->state = g_service_state;
}

void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  PRINTF("ON RTSP SERVICE STATUS");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}
