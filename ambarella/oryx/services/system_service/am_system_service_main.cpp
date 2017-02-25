/*******************************************************************************
 * am_system_service_main.cpp
 *
 * History:
 *   2014-9-16 - [lysun] created file
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

#include "am_service_frame_if.h"
#include "am_system_service_msg_map.h"
#include "am_api_system.h"
#include "am_pid_lock.h"
#include "am_system_service_priv.h"
#include "am_upgrade_if.h"
#include "am_led_handler_if.h"
#include "am_signal.h"

#include <thread>

AMIServiceFrame   *g_service_frame = nullptr;
AM_SERVICE_STATE g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIFWUpgradePtr g_upgrade_fw = nullptr;
AMILEDHandlerPtr g_ledhandler = nullptr;
am_mw_ntp_cfg g_ntp_cfg = {{"",""}, 0, 0, 0, 0, false};
std::thread *g_th_ntp = nullptr;

int main(int argc, char *argv[])
{
  int ret  = 0;
  AMIPCSyncCmdServer ipc;

  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  register_critical_error_signal_handler();

  do {
    AMPIDLock lock;

    g_service_frame = AMIServiceFrame::create(argv[0]);
    if (AM_UNLIKELY(!g_service_frame)) {
      ERROR("Failed to create service framework object for System.Service!");
      ret = -1;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    if (lock.try_lock() < 0) {
      ERROR("Unable to lock PID, System.Service should be running already");
      ret = -4;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    }

    if (ipc.create(AM_IPC_SYSTEM_NAME) < 0) {
      ret = -5;
      g_service_state = AM_SERVICE_STATE_ERROR;
      break;
    } else {
      ipc.REGISTER_MSG_MAP(API_PROXY_TO_SYSTEM_SERVICE);
      ipc.complete();
      DEBUG("IPC create done for API_PROXY TO SYSTEM_SERVICE\n");
    }

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
      g_th_ntp = nullptr;
    }
    ret = 0;
  } while (0);
  AM_DESTROY(g_service_frame);
  PRINTF("System service destroyed!");

  return ret;
}


