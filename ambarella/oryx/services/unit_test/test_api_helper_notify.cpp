/*******************************************************************************
 * test_api_helper_notify.cpp
 *
 * History:
 *   2015-12-1 - [zfgong] created file
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_api_helper.h"
#include "am_air_api.h"
#include "am_api_event.h"
#include "event/am_event_types.h"
#include "video/am_video_types.h"
#include "jpeg_snapshot/am_jpeg_snapshot_if.h"
#include "jpeg_encoder/am_jpeg_encoder_if.h"


static AMAPIHelperPtr     g_api_helper = nullptr;
static AMIApiMediaEvent*  g_media_event = nullptr;
static AMIJpegSnapshotPtr g_sw_jpeg     = nullptr;
static AMIJpegEncoderPtr  g_sw_jpeg_encoder = nullptr;
static bool               g_long_pressed = false;
static bool               g_use_hw_jpeg = false;
static bool               g_use_sw_jpeg = false;

static int jpeg_capture();
static int jpeg_capture_stop();

static void sigstop(int arg)
{
  PRINTF("catch sigstop, exit.");
  exit(1);
}

static int on_snapshot(void *data, int len)
{
  INFO("on_snapshot len = %d, add your data handle code to here", len);
  if (g_long_pressed == true) {
    g_long_pressed = false;
    jpeg_capture_stop();
  }
  //there is no need to free data here!
  return 0;
}

static int audio_alert_handle(AM_EVENT_MESSAGE *event_msg)
{
  PRINTF("add your handle code");
  return 0;
}

static int audio_analysis_handle(AM_EVENT_MESSAGE *event_msg)
{
  PRINTF("add your handle code");
  return 0;
}

static int face_handle(AM_EVENT_MESSAGE *event_msg)
{
  PRINTF("add your handle code");
  return 0;
}

static int sw_jpeg_init()
{
  g_sw_jpeg = AMIJpegSnapshot::get_instance();
  if (!g_sw_jpeg) {
    ERROR("unable to get AMIJpegSnapshot instance\n");
    return -1;
  }
  g_sw_jpeg_encoder = AMIJpegEncoder::get_instance();
  if (!g_sw_jpeg_encoder) {
    ERROR("unable to get AMIJpegEncoder instance\n");
    return -1;
  }
  string path("/tmp/jpeg");
  g_sw_jpeg->set_source_buffer(AM_SOURCE_BUFFER_MAIN);
  g_sw_jpeg->set_file_max_num(100);
  g_sw_jpeg->set_file_path(path);
  g_sw_jpeg->set_fps(1.0);
  g_sw_jpeg->save_file_enable();
  //if handle snapped data by yourself, please call set_data_cb
  g_sw_jpeg->set_data_cb(on_snapshot);
  if (g_sw_jpeg->run() != AM_RESULT_OK) {
    ERROR("AMEncodeDevice: jpeg encoder start failed \n");
    return -1;
  }
  return 0;
}

static int sw_jpeg_capture()
{
  int ret = -1;
  if (g_sw_jpeg) {
    ret = g_sw_jpeg->capture_start();
    ret += g_sw_jpeg->set_source_buffer(AM_SOURCE_BUFFER_2ND);
  }
  return ret;
}

static int sw_jpeg_capture_stop()
{
  int ret = -1;
  if (g_sw_jpeg) {
    ret = g_sw_jpeg->capture_stop();
  }
  return ret;
}

static int hw_jpeg_init()
{
  int ret = 0;
  do {
    g_media_event = AMIApiMediaEvent::create();
    if (!g_media_event) {
      ERROR("Failed to create AMIApiMediaEventStruct");
      ret = -1;
      break;
    }

    g_media_event->set_attr_mjpeg();
    if (!g_media_event->set_stream_id(2) ||
        !g_media_event->set_pre_cur_pts_num(0) ||
        !g_media_event->set_after_cur_pts_num(0) ||
        !g_media_event->set_closest_cur_pts_num(1)) {
      ERROR("Failed to set g_media_event paraments");
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

static int hw_jpeg_capture()
{
  int ret = 0;
  am_service_result_t result = {0};
  do {
    if (!g_api_helper || !g_media_event) {
      ERROR("invalid paraments!");
      ret = -1;
      break;
    }
    g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                              g_media_event->get_data(), g_media_event->get_data_size(),
                              &result, sizeof(result));
    if (result.ret < 0) {
      ERROR("Failed to start g_media_event recording!");
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}


static int jpeg_capture()
{
  int ret = 0;
  if (g_use_sw_jpeg) {
    PRINTF("capture software jpeg");
    ret = sw_jpeg_capture();
  }
  if (g_use_hw_jpeg) {
    PRINTF("capture hardware jpeg");
    ret = hw_jpeg_capture();
  }

  return ret;
}

static int jpeg_capture_stop()
{
  int ret = 0;
  if (g_use_sw_jpeg) {
    ret = sw_jpeg_capture_stop();
  }
  return ret;
}

static int key_handle(AM_EVENT_MESSAGE *event_msg)
{
  const char *key_state[AM_KEY_STATE_NUM] =
  {" ", "UP", "DOWN", "CLICKED", "LONG_PRESS" };
  PRINTF("\npts: %llu, event number: %llu, key code: %d, key state: %s\n",
       event_msg->pts,
       event_msg->seq_num,
       event_msg->key_event.key_value,
       key_state[event_msg->key_event.key_state]);

  switch (event_msg->key_event.key_state) {
  case AM_KEY_UP:
    PRINTF("key up");
    break;
  case AM_KEY_DOWN:
    PRINTF("key down");
    break;
  case AM_KEY_CLICKED:
    PRINTF("key clicked");
    break;
  case AM_KEY_LONG_PRESSED:
    PRINTF("key long pressed");
    //key_value comes from "input_adckey amb,keymap",
    //or "gpio_keys linux,code" in dts file, which depend on the board.
    if (event_msg->key_event.key_value == 100 ||
        event_msg->key_event.key_value == 116) {
      g_long_pressed = true;
      jpeg_capture();
    }
    break;
  default:
    ERROR("unknown key event state");
    break;
  }

  return 0;
}

static int on_notify(const void *msg_data, int msg_data_size)
{
  am_service_notify_payload *payload = (am_service_notify_payload *)msg_data;
  if (payload->type == AM_EVENT_SERVICE_NOTIFY) {
    AM_EVENT_MESSAGE *event_msg = (AM_EVENT_MESSAGE *)payload->data;
    switch(event_msg->event_type) {
      case EV_AUDIO_ALERT_DECT:
        audio_alert_handle(event_msg);
        break;
      case EV_AUDIO_ANALYSIS_DECT:
        audio_analysis_handle(event_msg);
        break;
      case EV_FACE_DECT:
        face_handle(event_msg);
        break;
      case EV_KEY_INPUT_DECT:
        key_handle(event_msg);
        break;
      default:
        ERROR("unknown event_type");
        break;
    }
  }
  return 0;
}

static struct option long_options[] =
{
  {"help",           no_argument,       0,  'h'},

  {"both",           no_argument,       0,  'b' },
  {"hardware",       no_argument,       0,  'H' },
  {"software",       no_argument,       0,  'S' },
  {0,                0,                 0,   0  }
};

static const char *short_options = "bhHS";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
  { "",    "\t\t\t"   "Show usage" },
  { "",    "\t\t\t"   "both SW and HW jpeg capture." },
  { "",    "\t\t\t"   "hardware jpeg capture." },
  { "",    "\t\t\t"   "software jpeg capture." },
};

void usage(int argc, char **argv)
{
  printf("\n%s usage:\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1;
      ++ i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c,  ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
}

int main(int argc, char **argv)
{
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  usage(argc, argv);

  int opt = 0;
  while((opt = getopt_long(argc, argv, short_options, long_options,
                           nullptr)) != -1) {
    switch (opt) {
      case 'b':
        g_use_sw_jpeg = g_use_hw_jpeg = true;
        break;
      case 'H':
        g_use_hw_jpeg = true;
        break;
      case 'S':
        g_use_sw_jpeg = true;
        break;
      case 'h':
      default:
        usage(argc, argv);
        break;
    }
  }

  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }
  g_api_helper->register_notify_cb(on_notify);

  if (g_use_sw_jpeg) {
    if (-1 == sw_jpeg_init()) {
      ERROR("sw_jpeg_init failed");
      return -1;
    }
  }
  if (g_use_hw_jpeg) {
    if (-1 == hw_jpeg_init()) {
      ERROR("hw_jpeg_init failed");
      return -1;
    }
  }

  pause();

  return 0;
}
