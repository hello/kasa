/*******************************************************************************
 * am_timer.cpp
 *
 * History:
 *   Jul 21, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_thread.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_timer.h"

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string>
#include <map>

#define TIMER_CTRL_R m_ctrl_fd[0]
#define TIMER_CTRL_W m_ctrl_fd[1]

typedef std::map<std::string, AMTimerData*> AMTimerDataMap;
enum AM_TIMER_CMD
{
  AM_TIMER_CMD_START   = 'B',
  AM_TIMER_CMD_STOP    = 'S',
  AM_TIMER_CMD_RESET   = 'R',
  AM_TIMER_CMD_EXIT    = 'E',
  AM_TIMER_CMD_ONETIME = 'O',
  AM_TIMER_CMD_NULL    = 0,
};

class AMTimerData
{
    friend class AMTimer;
  public:
    AMTimerData() = delete;
    AMTimerData(uint32_t timeout, AMTimerCallback cb, void *data) :
      m_data(data),
      m_callback(cb),
      m_timeout(timeout)
    {}
    virtual ~AMTimerData()
    {
      m_callback = nullptr;
      m_data = nullptr;
      send_cmd(AM_TIMER_CMD_EXIT);
      if (AM_LIKELY(m_master)) {
        m_timer_map_lock.lock();
        for (auto &v : m_single_timer_map) {
          delete v.second;
        }
        m_single_timer_map.clear();
        m_timer_map_lock.unlock();
      }
      AM_DESTROY(m_thread);
      AM_DESTROY(m_event);
      if (AM_LIKELY(TIMER_CTRL_R >= 0)) {
        close(TIMER_CTRL_R);
      }
      if (AM_LIKELY(TIMER_CTRL_W >= 0)) {
        close(TIMER_CTRL_W);
      }
    }

    bool init(const char *name, bool single = false)
    {
      bool ret = false;

      do {
        m_name = name ? std::string(name) : "null";
        m_master = single;
        if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM,
                                   IPPROTO_IP, m_ctrl_fd) < 0)) {
          PERROR("socketpair");
          break;
        }

        m_event = AMEvent::create();
        if (AM_UNLIKELY(!m_event)) {
          ERROR("Failed to create event!");
          break;
        }
        signal(SIGPIPE, SIG_IGN);

        m_thread = AMThread::create(name, static_timer_thread, this);
        if (AM_UNLIKELY(!m_thread)) {
          ERROR("Failed to create timer thread %s: %s!", name, strerror(errno));
          break;
        }
        m_event->wait();
        ret = true;
      }while(0);

      return ret;

    }

    const char* name()
    {
      return m_name.c_str();
    }

    bool is_running()
    {
      return (m_thread && m_thread->is_running());
    }

    bool add_to_monitor(std::string name, AMTimerData *data)
    {
      bool ret = false;
      if (AM_LIKELY(m_master)) {
        AMTimerDataMap::iterator iter;
        bool start_monitor = m_single_timer_map.empty();
        m_timer_map_lock.lock();
        iter = m_single_timer_map.find(name);
        if (AM_LIKELY(iter != m_single_timer_map.end())) {
          if (AM_LIKELY(iter->second == data)) {
            WARN("Timer %s is already added!", name.c_str());
          } else {
            delete m_single_timer_map[name];
            m_single_timer_map[name] = nullptr;
          }
        }
        m_single_timer_map[name] = data;
        if (AM_LIKELY(start_monitor && !m_single_timer_map.empty())) {
          m_timeout = data->m_timeout + 100;
          send_cmd(AM_TIMER_CMD_START);
        }
        m_timer_map_lock.unlock();
        ret = true;
      } else {
        ERROR("Timer %s is not running in single timer mode!", m_name.c_str());
      }

      return ret;
    }

    void* remove_from_monitor(std::string name)
    {
      void *data = nullptr;
      if (AM_LIKELY(m_master)) {
        AMTimerDataMap::iterator iter;
        m_timer_map_lock.lock();
        iter = m_single_timer_map.find(name);
        if (AM_LIKELY(iter != m_single_timer_map.end())) {
          data = m_single_timer_map[name]->m_data;
          delete m_single_timer_map[name];
          m_single_timer_map[name] = nullptr;
          m_single_timer_map.erase(name);
        }
        m_timer_map_lock.unlock();
      }
      return data;
    }

  private:
    static bool static_single_timer_monitor(void *data)
    {
      return ((AMTimerData*)data)->single_timer_monitor();
    }

    static void static_timer_thread(void *data)
    {
      ((AMTimerData*)data)->timer_thread();
    }

    bool single_timer_monitor()
    {
      bool ret = true;
      AMTimerDataMap::iterator iter;

      m_timer_map_lock.lock();
      iter = m_single_timer_map.begin();
      while (iter != m_single_timer_map.end()) {
        if (AM_LIKELY(!iter->second->is_running())) {
          iter = m_single_timer_map.erase(iter);
        } else {
          ++ iter;
        }
      }
      ret = !m_single_timer_map.empty();
      m_timer_map_lock.unlock();

      return ret;
    }

    void timer_thread()
    {
      fd_set allset;
      fd_set fds;
      sigset_t mask;
      sigset_t mask_orig;
      bool run = true;
      bool exit_after_timeout = false;
      timespec *tm = nullptr;
      timespec timeout = {0};

      FD_ZERO(&allset);
      FD_SET(TIMER_CTRL_R, &allset);

      /* Block out interrupts */
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      sigaddset(&mask, SIGTERM);
      sigaddset(&mask, SIGQUIT);

      if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig) < 0)) {
        PERROR("sigprocmask");
      }
      m_event->signal();

      while(run) {
        char cmd[1] = {AM_TIMER_CMD_NULL};
        int ret = -1;
        fds = allset;

        ret = pselect(TIMER_CTRL_R + 1, &fds, nullptr, nullptr, tm, &mask);
        if (ret > 0) {
          if (AM_LIKELY(FD_ISSET(TIMER_CTRL_R, &fds))) {
            if (AM_UNLIKELY(read(TIMER_CTRL_R, cmd, sizeof(cmd)) < 0)) {
              ERROR("Failed to read timer control command! Timer quit!");
              break;
            }
          } else {
            WARN("Received message from unknown fd!");
            continue;
          }
        } else if (0 == ret) {
          INFO("%s time out!", m_name.c_str());
          if (AM_LIKELY(m_callback)) {
            INFO("Calling timeout callback of timer %s", m_name.c_str());
            cmd[0] = m_callback(m_data) ?
                AM_TIMER_CMD_START : AM_TIMER_CMD_STOP;
          }
          if (AM_LIKELY(exit_after_timeout)) {
            m_callback = nullptr;
            m_data = nullptr;
            cmd[0] = AM_TIMER_CMD_EXIT;
            INFO("%s is going to exit!", m_name.c_str());
          }
        } else {
          if (AM_LIKELY(errno != EINTR)) {
            PERROR("pselect");
            break;
          }
        }
        switch(AM_TIMER_CMD(cmd[0])) {
          case AM_TIMER_CMD_RESET:
          case AM_TIMER_CMD_START: {
            INFO("%s time out is set to %u.%u", m_thread->name(),
                  m_timeout / 1000, m_timeout % 1000);
            timeout.tv_sec  = m_timeout / 1000;
            timeout.tv_nsec = (m_timeout % 1000) * 1000;
            tm = &timeout;
          }break;
          case AM_TIMER_CMD_STOP: {
            tm = nullptr;
          }break;
          case AM_TIMER_CMD_EXIT: {
            timeout.tv_sec  = 0;
            timeout.tv_nsec = 0;
            tm = &timeout;
            run = false;
          }break;
          case AM_TIMER_CMD_ONETIME: {
            INFO("%s time out is set to %u.%u", m_thread->name(),
                  m_timeout / 1000, m_timeout % 1000);
            timeout.tv_sec  = m_timeout / 1000;
            timeout.tv_nsec = (m_timeout % 1000) * 1000;
            tm = &timeout;
            exit_after_timeout = true;
          }break;
          case AM_TIMER_CMD_NULL:
          default: break;
        }
      }

      if (AM_LIKELY(!run)) {
        NOTICE("Timer %s thread exits!", m_name.c_str());
      } else {
        NOTICE("Timer %s thread exits due to error!", m_name.c_str());
      }
      if (AM_LIKELY(exit_after_timeout)) {
        NOTICE("One shot timer %s now exits!", m_name.c_str());
      }
    }

    bool send_cmd(AM_TIMER_CMD cmd)
    {
      std::lock_guard<AMMemLock> cmd_guard(m_lock);
      char command[1] = {cmd};
      int count = 0;
      int ret = 0;

      do {
        ret = write(TIMER_CTRL_W, command, sizeof(command));
        if (AM_UNLIKELY(ret <= 0)) {
          if (AM_LIKELY((errno != EAGAIN) && (errno != EINTR))) {
            ERROR("Failed to send timer control command \'%c\'!", cmd);
            break;
          }
        }
      } while((++ count < 5) && ((ret > 0) && (ret < (int)sizeof(command))));

      return (ret == sizeof(command));
    }

    void set_callback(AMTimerCallback cb, void *data)
    {
      std::lock_guard<AMMemLock> cmd_guard(m_lock);
      m_callback = cb;
      m_data = data;
    }

  private:
    AMEvent        *m_event      = nullptr;
    AMThread       *m_thread     = nullptr;
    void           *m_data       = nullptr;
    AMTimerCallback m_callback   = nullptr;
    uint32_t        m_timeout    = 0;
    bool            m_master     = false;
    int             m_ctrl_fd[2] = {-1};
    std::string     m_name;
    AMMemLock       m_lock;
    AMMemLock       m_timer_map_lock;
    AMTimerDataMap  m_single_timer_map;
};

/* AMTimer */
AMTimer* AMTimer::create()
{
  AMTimer *result = new AMTimer();
  if (AM_UNLIKELY(result && !result->init("SingleTimer", 100,
                                          nullptr, nullptr, true))) {
    delete result;
    result = nullptr;
  }

  return result;
}
AMTimer* AMTimer::create(const char *name,
                         uint32_t timeout,
                         AMTimerCallback cb,
                         void *data)
{
  AMTimer *result = new AMTimer();
  if (AM_UNLIKELY(result && !result->init(name, timeout, cb, data, false))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMTimer::destroy()
{
  delete this;
}

bool AMTimer::start(uint32_t timeout)
{
  if (AM_LIKELY(timeout > 0)) {
    m_timer->m_timeout = timeout;
  }

  return !m_single && m_timer->send_cmd(AM_TIMER_CMD_START);
}

bool AMTimer::stop()
{
  return !m_single && m_timer->send_cmd(AM_TIMER_CMD_STOP);
}

bool AMTimer::reset()
{
  return !m_single && m_timer->send_cmd(AM_TIMER_CMD_RESET);
}

void AMTimer::set_timer_callback(AMTimerCallback cb, void *data)
{
  m_timer->set_callback(cb, data);
}

bool AMTimer::single_shot(const char *name,
                          uint32_t timeout,
                          AMTimerCallback cb,
                          void *data)
{
  bool ret = false;
  do {
    if (AM_LIKELY(!m_single)) {
      ERROR("Timer is not a single timer!");
      break;
    }
    AMTimerData *timer = new AMTimerData(timeout, cb, data);
    if (AM_UNLIKELY(!timer || (timer && !timer->init(name)))) {
      ERROR("Failed to create timer!");
      delete timer;
    }
    if (AM_UNLIKELY(!timer->send_cmd(AM_TIMER_CMD_ONETIME))) {
      ERROR("Failed to start timer!");
      delete timer;
      break;
    }
    if (AM_UNLIKELY(!m_timer->add_to_monitor(std::string(name), timer))) {
      ERROR("Failed to monitor single timer %s", name);
    }

    ret = true;
  }while(0);

  return ret;
}

void* AMTimer::single_stop(const char *name)
{
  return m_timer->remove_from_monitor(std::string(name));
}

AMTimer::~AMTimer()
{
  delete m_timer;
}

bool AMTimer::init(const char *name,
                   uint32_t timeout,
                   AMTimerCallback cb,
                   void *data,
                   bool single)
{
  bool ret = false;

  do {
    m_timer = new AMTimerData(timeout, cb, data);
    m_single = single;
    if (AM_UNLIKELY(m_timer && !m_timer->init(name, m_single))) {
      delete m_timer;
      m_timer = nullptr;
      break;
    }
    if (AM_LIKELY(m_single)) {
      m_timer->set_callback(AMTimerData::static_single_timer_monitor, m_timer);
      m_timer->m_timeout = 500;
    }
    ret = true;
  }while(0);

  return ret;
}

