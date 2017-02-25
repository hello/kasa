/*******************************************************************************
 * test_oryx_playback.cpp
 *
 * History:
 *   2014-11-5 - [ypchang] created file
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

#include "am_playback_if.h"
#include "am_playback_info.h"

#include "am_thread.h"
#include "am_signal.h"
#include "am_io.h"

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
    case AM_PLAYBACK_MSG_EOF:
      NOTICE("One file is finished playing!");
      break;
    case AM_PLAYBACK_MSG_IDLE:
      NOTICE("Player's play list is empty now!");
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
  register_critical_error_signal_handler();
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
            ssize_t ret = am_write(CTRL_W, "e", 1, 5);
            if (AM_UNLIKELY(ret != 1)) {
              ERROR("Failed to write exit command to mainloop! %s",
                    strerror(errno));
            }
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
