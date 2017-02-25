/*******************************************************************************
 * am_media_service_main.cpp
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

#include "am_pid_lock.h"
#include "am_service_frame_if.h"
#include "am_media_service_msg_map.h"
#include "am_media_service_instance.h"
#include "am_signal.h"

AM_SERVICE_STATE   g_service_state  = AM_SERVICE_STATE_NOT_INIT;
AMMediaService    *g_media_instance = nullptr;
AMIServiceFrame   *g_service_frame  = nullptr;
AMIPCSyncCmdServer g_ipc;

static void user_input_callback(char ch)
{
  switch(ch) {
    case 'Q':
    case 'q': {
      NOTICE("Quit Media.Service!");
      g_service_frame->quit();
    }break;
    default: break;
  }
}

int main(int argc, char *argv[])
{
  int ret = -1;
  g_service_frame = AMIServiceFrame::create(argv[0]);

  if (AM_LIKELY(g_service_frame)) {
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    register_critical_error_signal_handler();

    g_media_instance = AMMediaService::create(nullptr);

    if (AM_LIKELY(g_media_instance)) {
      AMPIDLock lock;
      bool run = true;
      if (AM_UNLIKELY((argc > 1) && is_str_equal(argv[1], "debug"))) {
        NOTICE("Running Media service in debug mode, press 'q' to exit!");
        g_service_frame->set_user_input_callback(user_input_callback);
      } else {
        if (AM_UNLIKELY(lock.try_lock() < 0)) {
          ERROR("Unable to lock PID, Media service is already running!");
          ret = -1;
          run = false;
        } else if (g_ipc.create(AM_IPC_MEDIA_NAME) < 0) {
          g_service_state = AM_SERVICE_STATE_ERROR;
          run = false;
        } else {
          g_ipc.REGISTER_MSG_MAP(API_PROXY_TO_MEDIA_SERVICE);
          g_ipc.complete();
          DEBUG("IPC create done for API_PROXY TO MEDIA_SERVICE, name is %s \n",
                AM_IPC_MEDIA_NAME);
          g_service_state = AM_SERVICE_STATE_INIT_DONE;
        }
      }
      if (AM_LIKELY(run)) {
        if (!g_media_instance->start_media()) {
          ERROR("Media service: start media failed!");
          g_service_state = AM_SERVICE_STATE_ERROR;
        } else {
          g_service_state = AM_SERVICE_STATE_STARTED;
        }
        g_service_frame->run(); /* Block here */
        /* quit() of service frame is called, start destruction */
        g_media_instance->stop_media();
      }
      g_media_instance->destroy();
      g_media_instance = nullptr;
    } else {
      g_service_state = AM_SERVICE_STATE_ERROR;
      ERROR("Failed to create AMMediaService instance!");
      ret = -2;
    }
    g_service_frame->destroy();
    g_service_frame = nullptr;
    PRINTF("Media service destroyed!");
  } else {
    ERROR("Failed to create serivce framework for Media service!");
    g_service_state = AM_SERVICE_STATE_ERROR;
  }

  return ret;
}
