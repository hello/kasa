/*******************************************************************************
 * test_oryx_record.cpp
 *
 * History:
 *   2015-1-14 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_record_if.h"

#include "am_thread.h"

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
        case 'E':
          if (record.is_recording()) {
            if (record.is_ready_for_event()) {
              if (!record.send_event()) {
                ERROR("Failed to send event!");
              }
            } else {
              ERROR("It's not ready to send event, please try later!");
            }
          } else {
            ERROR("Please start recording first!");
          }
          break;
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
          write(CTRL_W, "e", 1);
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
