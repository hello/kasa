/*******************************************************************************
 * am_service_frame.cpp
 *
 * History:
 *   2015-1-26 - [ypchang] created file
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

#include "am_service_frame.h"
#include "version.h"
#include "am_watchdog_semaphore.h"
#include "am_thread.h"

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

AMIServiceFrame* AMIServiceFrame::create(const std::string& service_name)
{
  return AMServiceFrame::create(service_name);
}

AMIServiceFrame* AMServiceFrame::create(const std::string& name)
{
  AMServiceFrame *result = new AMServiceFrame(name);
  if (AM_UNLIKELY(result && !result->init())) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMServiceFrame::destroy()
{
  delete this;
}

void AMServiceFrame::run()
{
  fd_set allset;
  fd_set fdset;
  sigset_t mask;
  sigset_t mask_orig;

  int maxfd = -1;
  bool is_ok = true;
  std::string sem_thread_name = m_name + ".sem";

  FD_ZERO(&allset);
  FD_SET(CTRL_SOCK_R, &allset);

  /* Block out interrupts */
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);

  if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig) < 0)) {
    PERROR("sigprocmask");
  }

  if (AM_LIKELY(m_user_input_callback)) {
    FD_SET(STDIN_FILENO, &allset);
    maxfd = AM_MAX(CTRL_SOCK_R, STDIN_FILENO);
  } else {
    maxfd = CTRL_SOCK_R;
  }
  m_thread_feed_dog = AMThread::create(sem_thread_name.c_str(),
                                       static_send_heartbeat_to_watchdog,
                                       (void*)this);
  if (AM_UNLIKELY(!m_thread_feed_dog)) {
    ERROR("Failed to create thread %s: %s",
          sem_thread_name.c_str(),
          strerror(errno));
  } else {
    m_thread_created = true;
  }

  m_run = true;

  while(m_run && is_ok) {
    char ch[1]  = {SERVICE_CMD_NULL};
    char cmd[1] = {SERVICE_CMD_NULL};
    fdset = allset;
    if (AM_LIKELY(pselect(maxfd + 1, &fdset, nullptr, nullptr, nullptr,
                          &mask) > 0)) {
      if (AM_LIKELY(FD_ISSET(CTRL_SOCK_R, &fdset))) {
        if (AM_LIKELY(read(CTRL_SOCK_R, cmd, sizeof(cmd)) < 0)) {
          ERROR("Failed to read Service Control Command of %s! Quit!",
                m_name.c_str());
          cmd[0] = SERVICE_CMD_ABORT;
        } else {
          INFO("%s received command from control socket: %c",
               m_name.c_str(), cmd[0]);
        }
      } else if (AM_LIKELY(m_user_input_callback &&
                           FD_ISSET(STDIN_FILENO, &fdset))) {
        if (AM_LIKELY(read(STDIN_FILENO, ch, sizeof(ch)) < 0)) {
          ERROR("Failed to read user input for %s from stdin! Quit!",
                m_name.c_str());
          cmd[0] = SERVICE_CMD_ABORT;
        } else {
          INFO("%s received command from std input: %c",
               m_name.c_str(), ch[0]);
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        break;
      }
    }

    switch(AM_SERVICE_FRAME_CMD(cmd[0])) {
      case SERVICE_CMD_QUIT: {
        NOTICE("Quit is called for service %s!", m_name.c_str());
        m_run = false;
      }break;
      case SERVICE_CMD_ABORT: {
        is_ok = false;
      }break;
      case SERVICE_CMD_NULL:
      default:break;
    }

    if (AM_LIKELY(AM_SERVICE_FRAME_CMD(cmd[0]) != SERVICE_CMD_NULL)) {
      send_wd_ctrl_cmd(AM_SERVICE_FRAME_CMD(cmd[0]));
    }

    if (AM_LIKELY(m_run && is_ok &&
                  (cmd[0] == SERVICE_CMD_NULL) &&
                  (ch[0]  != SERVICE_CMD_NULL) &&
                  m_user_input_callback)) {
      send_wd_ctrl_cmd(SERVICE_CMD_QUIT);
      m_user_input_callback(ch[0]);
    }
  }

  if (AM_LIKELY(!m_run)) {
    NOTICE("Service %s exits mainloop!", m_name.c_str());
  } else {
    m_run = false;
    NOTICE("Service %s exits mainloop due to error!", m_name.c_str());
  }
  AM_DESTROY(m_thread_feed_dog);
}

bool AMServiceFrame::quit()
{
  return send_ctrl_cmd(SERVICE_CMD_QUIT);
}

void AMServiceFrame::set_user_input_callback(AMServiceFrameCallback cb)
{
  m_user_input_callback = cb;
}

uint32_t AMServiceFrame::version()
{
  return SERVICE_FRAME_LIB_VERSION;
}

AMServiceFrame::AMServiceFrame(const std::string& name) :
    m_user_input_callback(nullptr),
    m_thread_feed_dog(nullptr),
    m_semaphore(nullptr),
    m_name(name),
    m_run(false),
    m_thread_running(false),
    m_thread_created(false)
{
  CTRL_SOCK_R = -1;
  CTRL_SOCK_W = -1;
  WD_CTRL_SOCK_R = -1;
  WD_CTRL_SOCK_W = -1;
}

AMServiceFrame::~AMServiceFrame()
{
  quit();
  if (AM_LIKELY(CTRL_SOCK_R >= 0)) {
    close(CTRL_SOCK_R);
  }
  if (AM_LIKELY(CTRL_SOCK_W >= 0)) {
    close(CTRL_SOCK_W);
  }
  if (AM_LIKELY(WD_CTRL_SOCK_R >= 0)) {
    close(WD_CTRL_SOCK_R);
  }
  if (AM_LIKELY(WD_CTRL_SOCK_W >= 0)) {
    close(WD_CTRL_SOCK_W);
  }
}

bool AMServiceFrame::init()
{
  bool ret = false;
  m_sem_name = "/" + std::string(basename(m_name.c_str())) + ".watchdog";
  INFO("Watchdog semaphore name: %s", m_sem_name.c_str());
  do {
    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP,
                               m_ctrl_sock) < 0)) {
      PERROR("socketpair");
      break;
    }
    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP,
                               m_wd_ctrl_sock) < 0)) {
      PERROR("socketpair");
      break;
    }
    signal(SIGPIPE, SIG_IGN);
    ret = true;
  }while(0);

  return ret;
}

bool AMServiceFrame::send_ctrl_cmd(AM_SERVICE_FRAME_CMD cmd)
{
  int ret = 0;
  int count = 0;
  char command[1] = {cmd};

  do {
    ret = write(CTRL_SOCK_W, command, sizeof(command));
    if (AM_UNLIKELY(ret <= 0)) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        ERROR("Failed to send service control command \'%c\'!", cmd);
        break;
      }
    }
  }while((++ count < 5) && ((ret > 0) && (ret < (int)sizeof(command))));

  return (ret == sizeof(command));
}

bool AMServiceFrame::send_wd_ctrl_cmd(AM_SERVICE_FRAME_CMD cmd)
{
  int ret = 0;
  int count = 0;
  char command[1] = {cmd};

  do {
    ret = write(WD_CTRL_SOCK_W, command, sizeof(command));
    if (AM_UNLIKELY(ret <= 0)) {
      if (AM_LIKELY((errno != EAGAIN) &&
                    (errno != EWOULDBLOCK) &&
                    (errno != EINTR))) {
        ERROR("Failed to send service control command \'%c\'!", cmd);
        break;
      }
    }
  }while((++ count < 5) && ((ret > 0) && (ret < (int)sizeof(command))));

  return (ret == sizeof(command));
}

bool AMServiceFrame::init_semaphore(const char* sem_name)
{
  m_semaphore = sem_open(sem_name, O_CREAT|O_EXCL,0644, 0);
  if (AM_UNLIKELY((m_semaphore == SEM_FAILED) && (errno == EEXIST))) {
    m_semaphore = sem_open(sem_name, 0);
  }
  if (AM_UNLIKELY(m_semaphore == SEM_FAILED)) {
    PERROR(sem_name);
  }
  return (m_semaphore != SEM_FAILED);
}

void AMServiceFrame::static_send_heartbeat_to_watchdog(void *data)
{
  ((AMServiceFrame*)data)->send_heartbeat_to_watchdog();
}

void AMServiceFrame::send_heartbeat_to_watchdog()
{
  struct timespec heart_beat_interval;
  bool semOk = init_semaphore(m_sem_name.c_str());
  bool run = true;

  sigset_t sigmask;
  sigset_t sigmask_orig;

  fd_set allset;
  fd_set fdset;
  int maxfd = -1;

  FD_ZERO(&allset);
  FD_SET(WD_CTRL_SOCK_R, &allset);

  /* Block out interrupt */
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGTERM);

  if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &sigmask, &sigmask_orig) < 0)) {
    PERROR("sigprocmask");
  }

  maxfd = WD_CTRL_SOCK_R;

  m_thread_running=true;
  if (AM_UNLIKELY(!semOk)) {
    ERROR("Failed to initialize semaphore %s\n", m_sem_name.c_str());
  }

  while (run && semOk && m_run) {
    if (AM_LIKELY(sem_trywait(m_semaphore) == 0)) {
      continue;
    }
    fdset = allset;
    heart_beat_interval.tv_sec = HEART_BEAT_INTERVAL;
    heart_beat_interval.tv_nsec = 0;

    int ret = pselect(maxfd + 1, &fdset, nullptr, nullptr,
                      &heart_beat_interval,
                      &sigmask);
    char cmd[1] = {SERVICE_CMD_NULL};
    if (AM_UNLIKELY(ret < 0)) { /* Error Occurs */
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("pselect");
        break;
      }
    } else if (ret > 0) { /* Get Control Command */
      if (AM_LIKELY(FD_ISSET(WD_CTRL_SOCK_R, &fdset))) {
        if (AM_LIKELY(read(WD_CTRL_SOCK_R, cmd, sizeof(cmd)) < 0)) {
          ERROR("Failed to read WatchDog Thread Control Command of %s! Quit!",
                m_thread_feed_dog->name());
          cmd[0] = SERVICE_CMD_ABORT;
        } else {
          INFO("%s received command from WatchDog Thread control socket: %c",
                 m_thread_feed_dog->name(), cmd[0]);
        }
      }
    } /* ret == 0 means timeout */

    switch(AM_SERVICE_FRAME_CMD(cmd[0])) {
      case SERVICE_CMD_ABORT:
      case SERVICE_CMD_QUIT: {
        run = false;
        continue;
      }break;
      case SERVICE_CMD_NULL:
      default:break;
    }
  }

  while (sem_trywait(m_semaphore) == 0) {/*Clear all the semaphore*/}
  sem_close(m_semaphore);
  m_semaphore = nullptr;
}
