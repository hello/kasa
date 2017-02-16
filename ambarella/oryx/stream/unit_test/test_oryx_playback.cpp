/*******************************************************************************
 * test_oryx_playback.cpp
 *
 * History:
 *   2014-11-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
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

#include "am_playback_if.h"
#include "am_playback_info.h"

#include "am_thread.h"

#include <signal.h>
#include <unistd.h>

int  ctrl[2];
bool stop = false;
bool isrunning = true;

#define CTRL_R ctrl[0]
#define CTRL_W ctrl[1]

struct UserData
{
    AMIPlayback *playback;
    const char **files;
    uint32_t     count;
    UserData(AMIPlayback *p, const char **f, uint32_t c) :
      playback(p),
      files(f),
      count(c)
    {}
};

static inline void show_menu()
{
  PRINTF("===================Test Oryx Playback===================");
  PRINTF(" r -- start/resume playing");
  PRINTF(" p -- pause");
  PRINTF(" s -- stop");
  PRINTF(" q -- quit this program");
  PRINTF("========================================================");
}

static void playback_callback(AMPlaybackMsg& msg)
{
  switch(msg.msg) {
    case AM_PLAYBACK_MSG_START_OK:
      NOTICE("Start OK!");
      break;
    case AM_PLAYBACK_MSG_PAUSE_OK:
      NOTICE("Pauase OK!");
      break;
    case AM_PLAYBACK_MSG_STOP_OK:
      NOTICE("Stop OK!");
      break;
    case AM_PLAYBACK_MSG_ERROR:
      NOTICE("Error occurred!");
      break;
    case AM_PLAYBACK_MSG_ABORT:
      NOTICE("Abort due to error!");
      break;
    case AM_PLAYBACK_MSG_EOS:
      NOTICE("Playing back finished successfully!");
      break;
    case AM_PLAYBACK_MSG_TIMEOUT:
      NOTICE("Timeout!");
      break;
    case AM_PLAYBACK_MSG_NULL:
    default:break;
  }
}

static void sigstop(int sig_num)
{
  stop = true;
}

static void mainloop(AMIPlayback &playback, const char *file[], int count)
{
  fd_set allset;
  fd_set fdset;
  int maxfd = -1;

  FD_ZERO(&allset);
  FD_ZERO(&fdset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_R,       &allset);
  maxfd = (STDIN_FILENO > CTRL_R) ? STDIN_FILENO : CTRL_R;

  show_menu();
  while(isrunning) {
    char ch = '\n';
    fdset = allset;
    if (AM_LIKELY(select(maxfd + 1, &fdset, nullptr, nullptr, nullptr) > 0)) {
      if (AM_LIKELY(FD_ISSET(STDIN_FILENO, &fdset))) {
        if (AM_LIKELY(read(STDIN_FILENO, &ch, 1)) < 0) {
          ERROR("Failed to read command from user input, quit this program!");
          ch = 'q';
          isrunning = false;
        }
      } else if (AM_LIKELY(FD_ISSET(CTRL_R, &fdset))) {
        char cmd[1] = {0};
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

      switch(ch) {
        case 'R':
        case 'r': {
          if (playback.is_paused()) {
            if (!playback.pause(false)) {
              ERROR("Failed to resume!");
            }
          } else if (playback.is_playing()) {
            NOTICE("Already started playing!");
          } else {
            bool enable = false;
            for (int i = 0; i < count; ++ i) {
              AMPlaybackUri uri;
              uri.type = AM_PLAYBACK_URI_FILE;
              if(AM_UNLIKELY(strlen(file[i]) >= PATH_MAX)) {
                ERROR("The name of %s is too long.");
                enable = false;
                break;
              }
              memcpy(uri.media.file, file[i], strlen(file[i]));
              if (!playback.add_uri(uri)) {
                ERROR("Failed to add %s to play list!", file[i]);
              } else {
                enable = true;
              }
            }
            if (AM_LIKELY(enable && !playback.play())) {
              ERROR("Failed to start playing!");
            }
          }
        }break;
        case 'P':
        case 'p': {
          if (!playback.pause(true)) {
            ERROR("Failed to pause!");
          }
        }break;
        case 'Q':
        case 'S':
        case 'q':
        case 's': {
          if (!playback.stop()) {
            ERROR("Failed to stop playing!");
          }
          if ((ch == 'q') || (ch == 'Q')) {
            NOTICE("Quit!");
            isrunning = false;
          }
        }break;
        case '\n': continue; /* skip "show_menu()" */
        default:break;
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
  UserData *d = ((UserData*)data);
  mainloop(*d->playback, d->files, d->count);
}

int main(int argc, char *argv[])
{
  int ret = 0;
  if (AM_UNLIKELY(argc < 2)) {
    PRINTF("Usage: %s media1 [ media2 | media3 | ...]", argv[0]);
    ret = -1;
  } else {
    if (AM_UNLIKELY(pipe(ctrl) < 0)) {
      PERROR("pipe");
      ret = -2;
    } else {
      AMIPlaybackPtr playback = AMIPlayback::create();
      AMThread *work_thread = nullptr;
      if (AM_LIKELY((playback != nullptr) && playback->init())) {
        UserData data(playback.raw(), (const char**)&argv[1], argc - 1);
        signal(SIGINT,  sigstop);
        signal(SIGQUIT, sigstop);
        signal(SIGTERM, sigstop);
        playback->set_msg_callback(playback_callback, nullptr);
        work_thread = AMThread::create("Playback.work",
                                       thread_mainloop, &data);
        if (work_thread) {
          while(!stop) {
            usleep(500);
          }
          if (isrunning) {
            write(CTRL_W, "e", 1);
          }
          work_thread->destroy();
        }
      } else if (AM_LIKELY(!playback)) {
        ERROR("Failed to create AMPlayback object!");
        ret = -3;
      } else {
        ERROR("Failed to initialize AMPlayback!");
        ret = -4;
      }
      close(CTRL_R);
      close(CTRL_W);
    }
  }

  return ret;
}
