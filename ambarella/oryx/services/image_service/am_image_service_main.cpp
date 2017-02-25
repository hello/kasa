/*******************************************************************************
 * am_image_service_main.cpp
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
#include "am_configure.h"
#include "am_thread.h"
#include "am_image_service_msg_map.h"
#include "am_pid_lock.h"
#include "am_service_frame_if.h"
#include "am_image_quality_if.h"
#include "am_signal.h"
#include "am_api_image.h"
#include "am_timer.h"
#include "commands/am_api_cmd_media.h"

AM_SERVICE_STATE   g_service_state = AM_SERVICE_STATE_NOT_INIT;
AMIImageQualityPtr g_image_quality = nullptr;
AMIServiceFrame   *g_service_frame = nullptr;
AMIPCSyncCmdServer g_ipc;
AMTimer           *g_timer = nullptr;
am_image_3A_info_s g_image_3A;
am_image_3A_info_s g_image_3A_prev;

#define IMAGE_3A_NOTIFY_TIMER    4000

static bool image_3A_notify(void *data)
{
  am_service_result_t ae_result;
  am_service_result_t awb_result;
  am_service_notify_payload payload;
  memset(&payload, 0, sizeof(am_service_notify_payload));
  SET_BIT(payload.dest_bits, AM_SERVICE_TYPE_MEDIA);
  payload.msg_id = AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO;
  payload.data_size = sizeof(am_image_3A_info_s);

  memset(&g_image_3A, 0, sizeof(g_image_3A));
  memset(&ae_result, 0, sizeof(ae_result));
  ON_IMAGE_AE_SETTING_GET(nullptr, 0, &ae_result, sizeof(ae_result));
  if (ae_result.ret != 0) {
    ERROR("get ae setting failed");
  } else {
    memcpy(&g_image_3A.ae, ae_result.data, sizeof(am_ae_config_s));
  }

  memset(&awb_result, 0, sizeof(awb_result));
  ON_IMAGE_AWB_SETTING_GET(nullptr, 0, &awb_result, sizeof(awb_result));
  if (awb_result.ret != 0) {
    ERROR("get awb setting failed");
  } else {
    memcpy(&g_image_3A.awb, awb_result.data, sizeof(am_awb_config_s));
  }

  if ((g_image_3A.ae.sensor_gain != g_image_3A_prev.ae.sensor_gain)
     || (g_image_3A.ae.sensor_shutter != g_image_3A_prev.ae.sensor_shutter)
     || (g_image_3A.ae.ae_metering_mode != g_image_3A_prev.ae.ae_metering_mode)
     || (g_image_3A.awb.wb_mode != g_image_3A_prev.awb.wb_mode)) {
    memcpy(&g_image_3A_prev, &g_image_3A, sizeof(am_image_3A_info_s));
    memcpy(payload.data, &g_image_3A, payload.data_size);
    g_ipc.notify(AM_IPC_SERVICE_NOTIF, &payload,
                 payload.header_size() + payload.data_size);
  } else {
    INFO("image 3A info does not changed, no need to notify media_svc");
  }
  return true;
}

static int image_3a_notify_start()
{
  int ret = 0;
  do {
    g_timer = AMTimer::create("3A_notify.timer",
                         IMAGE_3A_NOTIFY_TIMER,
                         image_3A_notify,
                         nullptr);
    if (!g_timer) {
      ERROR("AMTimer create failed");
      ret = -1;
      break;
    }
    if (!g_timer->start()) {
      ERROR("AMTimer start failed");
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

static void image_3a_notify_stop()
{
  if (g_timer) {
    g_timer->stop();
  }
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

  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  register_critical_error_signal_handler();
  do {
    AMPIDLock lock;

    g_service_frame = AMIServiceFrame::create(argv[0]);
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

    if (AM_UNLIKELY((argc > 1) && is_str_equal(argv[1], "debug"))) {
      NOTICE("Running Image Service in debug mode, press 'q' to exit!");
      g_service_frame->set_user_input_callback(user_input_callback);
    } else {
      if (lock.try_lock() < 0) {
        ERROR("Unable to lock PID, Event.Service should be running already");
        ret = -4;
        g_service_state = AM_SERVICE_STATE_ERROR;
        break;
      } else if (g_ipc.create(AM_IPC_IMAGE_NAME) < 0) {
        ret = -5;
        g_service_state = AM_SERVICE_STATE_ERROR;
        break;
      } else {
        g_ipc.REGISTER_MSG_MAP(API_PROXY_TO_IMAGE_SERVICE);
        g_ipc.complete();
        DEBUG("IPC create done for API_PROXY TO IMAGE_SERVICE, name is %s\n",
              AM_IPC_IMAGE_NAME);
      }
    }
    if (!g_image_quality->start()) {
      ERROR("Image Service: start failed\n");
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STARTED;
    }

    if (g_image_quality->need_notify()) {
      if (image_3a_notify_start() != 0) {
        WARN("image_3a_notify_start failed");
      }
    }

    NOTICE("Entering Image.Service main loop!");
    g_service_frame->run(); /* block here */

    if (g_image_quality->need_notify()) {
      image_3a_notify_stop();
    }
    g_image_quality = nullptr;
    ret = 0;
  }while (0);

  if (AM_LIKELY(g_service_frame)) {
    g_service_frame->destroy();
    g_service_frame = nullptr;
  }
  PRINTF("Image Service destroyed!");

  return ret;
}
