/*******************************************************************************
 * test_oryx_record.cpp
 *
 * History:
 *   2015-1-14 - [ypchang] created file
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

#include "am_record_if.h"

#include "am_io.h"
#include "am_thread.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int ctrl[2];
bool stop = false;
bool isrunning = true;

#define CTRL_R ctrl[0]
#define CTRL_W ctrl[1]

static inline void show_menu()
{
  PRINTF("===================Test Oryx Record===================");
  PRINTF(" r -- start recording");
  PRINTF(" s -- stop recording");
  PRINTF(" e -- send event");
  PRINTF(" q -- quit this program");
  PRINTF("======================================================");
}

static void record_callback(AMRecordMsg &msg)
{
  switch(msg.msg) {
    case AM_RECORD_MSG_START_OK:
      NOTICE("Start OK!");
      break;
    case AM_RECORD_MSG_STOP_OK:
      NOTICE("Stop OK!");
      break;
    case AM_RECORD_MSG_ERROR:
      NOTICE("Error Occurred!");
      break;
    case AM_RECORD_MSG_ABORT:
      NOTICE("Recording Aborted!");
      break;
    case AM_RECORD_MSG_EOS:
      NOTICE("Recording Finished!");
      break;
    case AM_RECORD_MSG_TIMEOUT:
      NOTICE("Operation Timeout!");
      break;
    case AM_RECORD_MSG_OVER_FLOW:
    default: break;
  }
}

static void sigstop(int sig_num)
{
  stop = true;
}

static void mainloop(AMIRecord &record)
{
  fd_set allset;
  fd_set fdset;
  int maxfd = -1;

  FD_ZERO(&allset);
  FD_ZERO(&fdset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_R,       &allset);
  maxfd = AM_MAX(STDIN_FILENO, CTRL_R);

  show_menu();
  while(isrunning) {
    char ch = '\n';
    fdset = allset;
    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
      if (AM_LIKELY(FD_ISSET(STDIN_FILENO, &fdset))) {
        if (AM_LIKELY(read(STDIN_FILENO, &ch, 1) < 0)) {
          ERROR("Failed to read command from user input, quit this program!");
          ch = 'q';
          isrunning = false;
        }
      } else if (AM_LIKELY(FD_ISSET(CTRL_R, &fdset))) {
        char cmd[1] = { 0 };
        if (AM_LIKELY(read(CTRL_R, cmd, sizeof(cmd)) < 0)) {
          ERROR("Failed to read command, quit this program!");
          ch = 'q';
          isrunning = false;
        } else if (AM_LIKELY(cmd[0] == 'e')) {
          PRINTF("Exit mainloop!");
          ch = 'q';
          isrunning = false;
        }
      }

      switch (ch) {
        case 'R':
        case 'r': {
          if (AM_LIKELY(!record.is_recording())) {
            if (AM_UNLIKELY(!record.start())) {
              ERROR("Failed to start recording!");
            }
          } else {
            PRINTF("Recording is already started!");
          }
        }
          break;
        case 'q':
        case 'Q':
        case 's':
        case 'S': {
          if (record.is_recording() && !record.stop()) {
            ERROR("Failed to stop recording!");
          }
          if ((ch == 'q') || (ch == 'Q')) {
            NOTICE("Quit!");
            isrunning = false;
          }
        }
          break;
        case 'e':
        case 'E': {
          std::string param;
          AMEventStruct event;
          printf("please input the event type : 'h' for h264 and h265 video,"
              " 'm' for mjpeg : ");
          std::cin >> param;
          if (param[0] == 'h') {
            event.attr = AM_EVENT_H26X;
            printf("Please input event id : ");
            param.clear();
            std::cin >> param;
            event.h26x.stream_id = atoi(param.c_str());
            printf("Please input history duration.");
            std::cin >> param;
            event.h26x.history_duration = atoi(param.c_str());
            printf("Please input future duration.");
            std::cin >> param;
            event.h26x.future_duration = atoi(param.c_str());
          } else if (param[0] == 'm') {
            event.attr = AM_EVENT_MJPEG;
            printf("Please input stream id : ");
            param.clear();
            std::cin >> param;
            event.mjpeg.stream_id = atoi(param.c_str());
            printf("Please input the jpeg number of pre-current pts. "
                "If you want to set closest current pts jpeg, This should"
                "be set to zero : ");
            param.clear();
            std::cin >> param;
            event.mjpeg.pre_cur_pts_num = atoi(param.c_str());
            printf("Please input the jpeg number of after current pts. "
                "If you want to set closest current pts jpeg, This should"
                "be set to zero : ");
            param.clear();
            std::cin >> param;
            event.mjpeg.after_cur_pts_num = atoi(param.c_str());
            printf("Please input the jpeg number of closest to current pts : ");
            param.clear();
            std::cin >> param;
            event.mjpeg.closest_cur_pts_num = atoi(param.c_str());
          } else if (param[0] == 's') {
            event.attr = AM_EVENT_STOP_CMD;
            printf("Please input stream id :");
            param.clear();
            std::cin >> param;
            event.stop_cmd.stream_id = atoi(param.c_str());
          } else {
            ERROR("Event type '%s'is error", param.c_str());
            break;
          }
          if (record.is_recording()) {
            if (record.is_ready_for_event(event)) {
              if (!record.send_event(event)) {
                ERROR("Failed to send event!");
              }
            } else {
              ERROR("It's not available for sending event!");
            }
          } else {
            ERROR("Please start recording first!");
          }
        } break;
        case '\n':
          continue; /* skip "show_menu()" */
        default:
          break;
      }
      show_menu();
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        isrunning = false;
      }
      continue;
    }
  }
  stop = true;
}

static void thread_mainloop(void *data)
{
  mainloop(*((AMIRecord*)data));
}

int main()
{
  int ret = 0;
  if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP, ctrl) < 0)) {
    PERROR("socketpair");
    ret = -1;
  } else {
    AMIRecordPtr record = AMIRecord::create();
    AMThread *work_thread = nullptr;
    if (AM_LIKELY((record != nullptr) && record->init())) {
      signal(SIGINT,  sigstop);
      signal(SIGQUIT, sigstop);
      signal(SIGTERM, sigstop);
      record->set_msg_callback(record_callback, nullptr);
      work_thread = AMThread::create("Record.work",
                                     thread_mainloop, record.raw());
      if (work_thread) {
        while(!stop) {
          usleep(100000);
        }
        if (isrunning) {
          ssize_t ret = am_write(CTRL_W, "e", 1, 5);
          if (AM_UNLIKELY(ret != 1)) {
            ERROR("Failed to write exit command to mainloop! %s",
                  strerror(errno));
          }
        }
        work_thread->destroy();
      }
    } else if (AM_LIKELY(!record)) {
      ERROR("Failed to create AMRecord object!");
      ret = -2;
    } else {
      ERROR("Failed to initialize AMRecord!");
      ret = -3;
    }
    close(CTRL_R);
    close(CTRL_W);
  }

  return ret;
}
