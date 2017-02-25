/*******************************************************************************
 * oryx_doorbell_demo.cpp
 *
 * History:
 *    April 8, 2016- [smdong] created file
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
#include "am_event_types.h"
#include "am_playback_if.h"
#include "am_define.h"
#include "am_event_monitor_if.h"
#include "am_api_helper.h"
#include "am_air_api.h"
#include "am_api_video.h"

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fstream>

static AMAPIHelperPtr g_api_helper = nullptr;
static AMIPlaybackPtr g_player = nullptr;
static AMIEventMonitorPtr g_event_instance = nullptr;
static int g_socket_fd = -1;
static int g_conn_fd = -1;

static int g_maxvalue_c_pos;
static int g_maxvalue_c_neg;
static int g_yaw_step;
static uint32_t g_change_time = 10;

static bool g_running;
static bool g_recving;
static bool g_enable_run;

#define SOCKET_PORT 6666
#define MAX_LISTEN_NUM 1
#define KEY_VALUE 116

#ifndef VOICE_PATH
#define VOICE_PATH "/usr/share/"
#endif

static const char *sound_path = VOICE_PATH"soundfx/wav/success3.wav";
static const char *guide_path = "/usr/share/oryx/doc/oryx_doorbell_demo.txt";

enum SEND_FLAG {
  NONE_FLAG = 0,
  RING_FLAG = 1,
  COMMON_FLAG = 2,
};

enum CONTROL_MSG {
  RING = 0,
  UP = 1,
  DOWN = 2,
  RESET = 3,
  FDET = 4,
  TOP = 5,
  END =6,
  NONE = 7,
};

enum SEND_STATUS {
  NOT_TRIGGER = 0,
  TRIGGER = 1,
  SEND_FAIL = 2,
  SEND_SUC =3,
};
static SEND_STATUS g_send_data;

enum RECV_STATUS {
  RECV_SUCCESSFULLY = 0,
  RECV_FAILURE = 1,
  RECV_DISCONNECT = 2,
};

static int init_param(int argc, char *argv[])
{
  int ret = 0;
  int opt = 0;

  while ((opt = getopt(argc, argv, "rt:")) != -1) {
    switch(opt) {
      case 'r': {
        g_enable_run = true;
      }break;
      case 't': {
        int time = atoi(optarg);
        if (time > 0) {
          g_change_time = time;
        } else {
          WARN("Invalid time value %d", time);
          ret = -2;
        }
      }break;
      default : {
        ERROR("Invalid option!");
        ret = -1;
      }break;
    }
  }

  return ret;
}

static bool timer_set(int chang_time)
{
  bool ret = true;
  itimerval tick;
  memset(&tick, 0, sizeof(tick));
  tick.it_value.tv_sec = chang_time;
  tick.it_value.tv_usec = 0;
  tick.it_interval.tv_sec = chang_time;
  tick.it_interval.tv_usec = 0;
  if(AM_UNLIKELY(setitimer(ITIMER_REAL, &tick, nullptr) < 0)) {
    ERROR("Set timer failed!\n");
    ret = false;
  }
  return ret;
}

void back_status(int signo)
{
  g_send_data = NOT_TRIGGER;
}

static bool play_audio()
{
  bool ret = true;
  AMPlaybackUri play_uri;
  memcpy(play_uri.media.file, sound_path, strlen(sound_path));
  play_uri.type = AM_PLAYBACK_URI_FILE;
  if (AM_UNLIKELY(!g_player->play(&play_uri))) {
    ERROR("Failed to playback audio!");
    ret = false;
  }
  return ret;
}

static int32_t event_callback(AM_EVENT_MESSAGE *msg)
{
  int ret = 0;
  if (msg->event_type == EV_KEY_INPUT_DECT &&
      msg->key_event.key_value == KEY_VALUE) {
    switch (msg->key_event.key_state) {
      case AM_KEY_CLICKED: {
        NOTICE("Key:%d clicked!", KEY_VALUE);
        play_audio();
        g_send_data = TRIGGER;
        if(AM_UNLIKELY(!timer_set(g_change_time))) {
          ERROR("Failed to set timer!");
          ret = false;
          break;
        }
      }break;
      case AM_KEY_LONG_PRESSED: {
        NOTICE("Key:%d long pressed!", KEY_VALUE);
      }break;
      default:
        break;
    }
  }
  return ret;
}

static int init_socket(uint16_t port)
{
  int reuse = 1;
  int sock_fd = -1;

  signal(SIGPIPE, SIG_IGN);
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port        = htons(port);

  if (AM_UNLIKELY((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )) {
    PERROR("socket");
  } else {
    if (AM_LIKELY(setsockopt(sock_fd, SOL_SOCKET,
                             SO_REUSEADDR, &reuse, sizeof(reuse)) == 0)) {
      if (AM_UNLIKELY(bind(sock_fd, (struct sockaddr*)&server_addr,
                           sizeof(server_addr)) != 0)) {
        PERROR("bind");
      } else if (AM_UNLIKELY(listen(sock_fd, MAX_LISTEN_NUM) < 0)) {
        PERROR("listen");
      }
    } else {
      PERROR("setsockopt");
    }
  }

  return sock_fd;
}

static bool init()
{
  bool ret = true;
  do {
    if (AM_UNLIKELY(!(g_api_helper = AMAPIHelper::get_instance()))) {
      ERROR("Failed to get AMAPIHelper instance!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(!(g_player = AMIPlayback::create()))) {
      ERROR("Failed to create AMIPlayback!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(!g_player->init())) {
      ERROR("Failed to init AMPlayback!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(!(g_event_instance = AMIEventMonitor::get_instance()))) {
      ERROR("Failed to get AMIEventMonitor instance!");
      ret = false;
      break;
    }

    EVENT_MODULE_CONFIG module_config;
    AM_KEY_INPUT_CALLBACK key_callback;

    key_callback.callback = event_callback;
    module_config.key = AM_KEY_CALLBACK;
    module_config.value = (void*)&key_callback;

    key_callback.key_value = KEY_VALUE;
    g_event_instance->set_monitor_config(EV_KEY_INPUT_DECT, &module_config);

    if (!g_event_instance->start_event_monitor(EV_KEY_INPUT_DECT)) {
      ERROR("Failed to start event monitor!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY((g_socket_fd = init_socket(SOCKET_PORT)) < 0)) {
      ERROR("Failed to init socket");
      ret = false;
      break;
    }

    signal(SIGALRM, back_status);
    if(AM_UNLIKELY(!timer_set(g_change_time))) {
      ERROR("Failed to set timer!");
      ret = false;
      break;
    }

  } while(0);

  return ret;
}

static bool recv_client_config()
{
  bool ret = true;
  static int maxvalue, step_ori, num;
  do {
    if ((num = recv(g_conn_fd, &maxvalue, sizeof(int), 0) ) == -1) {
      PERROR("recv client maxvalue_c_pos error");
      ret = false;
      break;
    } else {
      g_maxvalue_c_pos = ntohl(maxvalue);
    }
    if ((num = recv(g_conn_fd, &maxvalue, sizeof(int), 0) ) == -1) {
      PERROR("recv client maxvalue_c_neg error");
      ret = false;
      break;
    } else {
      g_maxvalue_c_neg = ntohl(maxvalue);
    }
    if ((num = recv(g_conn_fd, &step_ori, sizeof(int), 0) ) == -1) {
      PERROR("recv client yaw_step error");
      ret = false;
      break;
    } else {
      g_yaw_step =  ntohl(step_ori);
    }
  } while(0);

  return ret;
}

static RECV_STATUS recv_client_control(SEND_FLAG &send_flag, int &yaw)
{
  RECV_STATUS ret = RECV_SUCCESSFULLY;
  send_flag = NONE_FLAG;

  fd_set fdset;
  FD_ZERO(&fdset);
  FD_SET(g_conn_fd, &fdset);

  if (AM_LIKELY(select(g_conn_fd + 1, &fdset, nullptr, nullptr, nullptr) > 0)) {
    if (FD_ISSET(g_conn_fd, &fdset)) {
      int recv_num = -1;
      uint32_t recv_cnt = 0;
      int msg;
      do {
        recv_num = recv(g_conn_fd, &msg, sizeof(int), 0);
      } while((++ recv_cnt < 5) && ((recv_num == 0) || ((recv_num < 0) &&
              ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
               (errno == EINTR)))));
      if (recv_num == 0) {
        NOTICE("The client disconnect the socket");
        ret = RECV_DISCONNECT;
      } else if (AM_UNLIKELY(recv_num < 0)) {
        PERROR("recv");
        ret = RECV_FAILURE;
      } else {
        CONTROL_MSG ctr_msg = CONTROL_MSG(ntohl(msg));
        switch (ctr_msg) {
          case RING: {
            send_flag = RING_FLAG;
          }break;
          case UP: {
            send_flag = COMMON_FLAG;
            yaw += g_yaw_step;
            if (yaw < g_maxvalue_c_neg) {
              yaw = g_maxvalue_c_neg;
            } else if (yaw > g_maxvalue_c_pos) {
              yaw = g_maxvalue_c_pos;
            }
          }break;
          case DOWN: {
            send_flag = COMMON_FLAG;
            yaw -= g_yaw_step;
            if (yaw < g_maxvalue_c_neg) {
              yaw = g_maxvalue_c_neg;
            } else if (yaw > g_maxvalue_c_pos) {
              yaw = g_maxvalue_c_pos;
            }
          }break;
          case RESET: {
            send_flag = COMMON_FLAG;
            yaw = 0;
          }break;
          case TOP: {
            send_flag = COMMON_FLAG;
            yaw = g_maxvalue_c_pos;
          }break;
          case END: {
            send_flag = COMMON_FLAG;
            yaw = g_maxvalue_c_neg;
          }break;
          case FDET:
          case NONE:
          default: {
            ERROR("Not supported control message!");
            break;
          }
        }
      }
    }
  } else {
    if (AM_LIKELY(errno != EINTR)) {
      PERROR("select");
      ret = RECV_FAILURE;
    }
  }

  return ret;
}

static bool send_status(int status)
{
  bool ret = true;
  int shift_status;
  shift_status = htonl(status);
  if (send(g_conn_fd, &shift_status, sizeof(int), 0) < 0) {
    PERROR("Send_status failed\n");
    ret = false;
  }
  return ret;
}


static void main_loop()
{
  g_running = true;
  while(g_running) {
    if ((g_conn_fd = accept(g_socket_fd, (sockaddr*)nullptr, nullptr)) < 0) {
      PERROR("accept");
      break;
    } else {
      if (!recv_client_config()) {
        ERROR("receive client config error");
        break;
      }
    }
    g_recving = true;
    int warp_region_yaw = 0;
    SEND_FLAG send_flag = NONE_FLAG;
    RECV_STATUS recv_ret = RECV_FAILURE;
    while (g_recving) {
      recv_ret = recv_client_control(send_flag, warp_region_yaw);
      switch (recv_ret) {
        case RECV_FAILURE: {
          ERROR("Recv client control msg error");
          g_recving = false;
        }break;
        case RECV_DISCONNECT: {
          NOTICE("The Client disconnected");
          g_recving = false;
        }break;
        case RECV_SUCCESSFULLY: {
          INFO("The current yaw is %d", warp_region_yaw);
          uint32_t ret = 0;
          am_warp_t warp_cfg = {0};
          am_service_result_t service_result;
          SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_YAW_PITCH_EN_BIT);
          warp_cfg.warp_region_yaw = warp_region_yaw;

          g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,
                                    &warp_cfg, sizeof(warp_cfg),
                                    &service_result, sizeof(service_result));
          if ((ret = service_result.ret) != 0) {
            ERROR("failed to set warp config!\n");
            g_recving = false;
            break;
          }

          if (send_flag == RING_FLAG) {
            if (!send_status(g_send_data)) {
              close(g_conn_fd);
              g_recving = false;
              ERROR("send ring error.");
              break;
            }
          } else if (send_flag == COMMON_FLAG) {
            if (!send_status(SEND_SUC)) {
              close(g_conn_fd);
              g_recving = false;
              ERROR("send suc error.");
              break;
            }
          }
        }break;
        default :{
          ERROR("Invalid recv status");
          g_recving = false;
        }break;
      }
    }
    send_status(SEND_FAIL);
    close(g_conn_fd);
  }

  close(g_socket_fd);

}

static void sigstop(int32_t arg)
{
  INFO("signal comes!\n");
  g_recving = false;
  g_running = false;
  exit(1);
}

void user_guide(const char *filename)
{
  std::ifstream file;
  file.open(filename, std::ios::in);
  if (AM_UNLIKELY(file.fail())) {
    ERROR("Failed to open file %s", filename);
    file.close();
  } else {
    std::string line_str;
    while(std::getline(file, line_str)) {
      PRINTF("\t%s", line_str.c_str());
    }
    file.close();
  }
}

int main(int argc, char *argv[])
{
  int ret = 0;

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  do {

    if (0 != init_param(argc, argv)) {
      ERROR("Init param error");
      user_guide(guide_path);
      break;
    }

    if (!g_enable_run) {
      user_guide(guide_path);
      break;
    }

    if (AM_UNLIKELY(!init())) {
      ERROR("Init failed");
      ret = -1;
      break;
    }
    /* enter main loop */
    main_loop();

  } while(0);

  return ret;

}
