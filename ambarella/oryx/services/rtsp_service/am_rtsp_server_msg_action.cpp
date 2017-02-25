/*******************************************************************************
 * am_rtsp_server_msg_action.cpp
 *
 * History:
 *   2015-1-19 - [Dong Shiming] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  PRINTF("ON RTSP SERVICE DESTROY.");
  if (!g_rtsp_server) {
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -1;
    ERROR("RTSP service: Failed to get AMRtspServer instance!");
  } else {
    g_rtsp_server->stop();
    g_service_state = AM_SERVICE_STATE_STOPPED;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
  g_service_frame->quit(); /* make run() in main function quit */
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  PRINTF("ON RTSP SERVICE START.");
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
  PRINTF("ON RTSP SERVICE STOP.");
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
  PRINTF("ON RTSP SERVICE RESTART.");
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
  PRINTF("ON RTSP SERVICE STATUS.");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}
