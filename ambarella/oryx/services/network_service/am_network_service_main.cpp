/*******************************************************************************
 * am_network_service_main.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#include "am_define.h"
#include "am_log.h"
#include "am_network_service_msg_map.h"
#include "am_pid_lock.h"
#include "am_network_service_priv.h"
#include "am_service_frame_if.h"
#include "am_signal.h"

AM_SERVICE_STATE g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIServiceFrame *g_service_frame  = nullptr;

int main(int argc, char *argv[])
{
  int ret = 0;
  AMIPCSyncCmdServer ipc;

  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  register_critical_error_signal_handler();
  do {
    g_service_frame = AMIServiceFrame::create(argv[0]);
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for Network.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    AMPIDLock lock;
    if (lock.try_lock() < 0) {
       ERROR("unable to lock PID, same name process should be running already\n");
       ret = -1;
       break;
    }

    if (ipc.create(AM_IPC_NETWORK_NAME) < 0) {
      ret = -2;
      break;
    } else {
      ipc.REGISTER_MSG_MAP(API_PROXY_TO_NETWORK_SERVICE);
      ipc.complete();
      DEBUG("IPC create done for API_PROXY TO NETWORK_SERVICE, name is %s \n",
            AM_IPC_NETWORK_NAME);
    }
    //sleep(2);   //simulate the extra delay needed,  TODO: anything that must
              //take time to do for init process
    g_service_state = AM_SERVICE_STATE_INIT_DONE;
    g_service_frame->run(); /* block here */
    ret = 0;
  } while(0);

  AM_DESTROY(g_service_frame);
  PRINTF("Network service destroyed!");

  return ret;
}
