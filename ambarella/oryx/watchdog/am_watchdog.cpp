/*******************************************************************************
 * watchdog_service_instance.cpp
 *
 * History:
 *   2014年5月14日 - [ypchang] created file
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
#include "am_service_manager.h"
#include "am_watchdog.h"
#include "am_watchdog_semaphore.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <linux/wireless.h>


#define INTR_FILE       (char*)"/proc/interrupts"


#define SRV_CTRL_R      mSrvCtrl[0]
#define SRV_CTRL_W      mSrvCtrl[1]

struct SrvData
{
  sem_t      *sem      = SEM_FAILED;
  bool        enable   = false;
  std::string sem_name;
  std::string srv_name;
  ~SrvData()
  {
    if (sem != SEM_FAILED) {
      sem_close(sem);
      sem = SEM_FAILED;
    }
  }
};

struct WdData
{
  AMWatchdogService* self;
  void* data;
  WdData() :
    self(nullptr),
    data(nullptr)
  {}
};

AMWatchdogService::AMWatchdogService()
{
  mServiceDataList.clear();
}

AMWatchdogService::~AMWatchdogService()
{
  stop();
  /* Need to close log file before system rebooting */
  close_log_file();
  mServiceDataList.clear();
  delete   mWdData;

  if (AM_LIKELY(SRV_CTRL_R >= 0)) {
    close(SRV_CTRL_R);
  }
  if (AM_LIKELY(SRV_CTRL_W >= 0)) {
    close(SRV_CTRL_W);
  }
}

bool AMWatchdogService::init(
    const std::list<am_service_attribute> &service_list)
{
  if (AM_LIKELY(!mIsInited)) {
    do {
      pthread_mutex_init(&mSrvLock, nullptr);

      if (AM_UNLIKELY(!set_timestamp_enabled(true))) {
        ERROR("Failed to enable log time stamp!");
      }
      if (AM_UNLIKELY(pipe(mSrvCtrl) < 0)) {
        PERROR("pipe");
        break;
      }
      mServiceDataList.clear();
      for (auto &srv : service_list) {
        SrvData srv_data;
        srv_data.enable   = srv.enable,
        srv_data.sem_name = "/" + std::string(srv.filename) + ".watchdog",
        srv_data.srv_name = std::string(srv.name),

        INFO("[%32s], sem_name: %32s, %s",
             srv_data.srv_name.c_str(),
             srv_data.sem_name.c_str(),
             srv_data.enable ? "Enabled" : "Disabled");
        mServiceDataList.push_back(srv_data);
      }

      if (AM_LIKELY(!mWdData)) {
        mWdData = new WdData();
      }
      if (AM_UNLIKELY(!mWdData)) {
        ERROR("Failed to allocate data for watchdog data!");
      }

      mIsInited = true;
    }while(0);
  }

  return mIsInited;
}

bool AMWatchdogService::start()
{
  bool ret = true;

  pthread_mutex_lock(&mSrvLock);
  if (AM_LIKELY(mIsInited)) {
    do {
      if (AM_LIKELY(mWdFeedingThread > 0)) {
        char threadName[128] = {0};
        if (AM_UNLIKELY(pthread_getname_np(mWdFeedingThread,
                                           threadName,
                                           sizeof(threadName)) != 0)) {
          PERROR("pthread_getname_np");
          strncpy(threadName, "AMWatchdogService", sizeof(threadName) - 1);
        }
        PRINTF("%s is already running!", threadName);
        break;
      }


      if (AM_LIKELY(init_service_data() && start_feeding_thread())) {
        PRINTF(">>> ======= [DONE] mw_start_watchdog  =======\n");
      } else {
        PRINTF("watchdog: started failed!");
        ret = false;
        break;
      }

    } while(0);
  }
  pthread_mutex_unlock(&mSrvLock);

  return ret;
}

bool AMWatchdogService::stop()
{
  bool ret = true;

  pthread_mutex_lock(&mSrvLock);
  do {
    if (AM_LIKELY(0 == mWdFeedingThread)) {
      PRINTF("watchdog is not running!");
      break;
    }

    if (AM_LIKELY(stop_feeding_thread() && clean_service_data() )) {
      PRINTF(">>> ======= [DONE] mw_stop_watchdog  =======\n");
    } else {
      ERROR("Failed to remove : %s",  strerror(errno));
      break;
    }

  } while (0);
  pthread_mutex_unlock(&mSrvLock);

  return ret;
}

void AMWatchdogService::run()
{
  fd_set allset;
  fd_set fdset;
  int maxfd = -1;

  FD_ZERO(&allset);
  FD_SET(SRV_CTRL_R, &allset);
  maxfd = SRV_CTRL_R;
  mRun = true;

  if (AM_UNLIKELY(false == (mRun = start()))) {
      ERROR("Failed to run watchdog service!");
  }

  while (mRun) {
    fdset = allset;
    if (AM_LIKELY(select(maxfd + 1, &fdset, nullptr, nullptr, nullptr) > 0)) {
      if (AM_LIKELY(FD_ISSET(SRV_CTRL_R, &fdset))) {
        char cmd[1] = {0};
        if (AM_LIKELY(read(SRV_CTRL_R, cmd, sizeof(cmd)) < 0)) {
          PERROR("read");
          mRun = false;
          continue;
        } else {
          switch(cmd[0]) {
            case 'e': {
              NOTICE("Quit watchdog service!");
              stop();
              mRun = false;
            }break;
            case 'a': {
              WARN("watchdog service aborted, due to service timeout!");
              mRun = false;
            }break;
            default: break;
          }
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        mRun = false;
      }
    }
  }
}

void AMWatchdogService::quit()
{
  write(SRV_CTRL_W, "e", 1);
}

bool AMWatchdogService::start_feeding_thread()
{
  bool ret = false;
  if (AM_LIKELY(enable_watchdog(mWorkTimeout))) {
    mIsFeeding = true;
    mWdData->self = this;
    if (AM_UNLIKELY(pthread_create(&mWdFeedingThread,
                                   nullptr,
                                   static_watchdog_feeding_thread,
                                   (void*)mWdData) < 0)) {
      PERROR("Failed to create watchdog_feeding_thread");
    } else {
      ret = true;
    }
  }

  return ret;
}

bool AMWatchdogService::stop_feeding_thread()
{
  if (AM_LIKELY(mWdFeedingThread > 0)) {
    mIsFeeding = false;
    pthread_join(mWdFeedingThread, nullptr);
    disable_watchdog();
    mWdFeedingThread = 0;
  }

  return true;
}

bool AMWatchdogService::set_watchdog_timeout(int sec)
{
  bool ret = false;

  if (AM_LIKELY(mWdFd >= 0)) {
    if (AM_UNLIKELY(ioctl(mWdFd, WDIOC_SETTIMEOUT, &sec) < 0)) {
      PERROR("WDIOC_SETTIMEOUT");
    } else {
      ret = true;
    }
  }

  return ret;
}

bool AMWatchdogService::enable_watchdog(int sec)
{
  bool ret = true;
  if (AM_LIKELY(mWdFd < 0)) {
    if (AM_UNLIKELY((mWdFd = open(WATCHDOG_DEVICE, O_RDWR)) < 0)) {
      ERROR("Open %s error: %s", WATCHDOG_DEVICE, strerror(errno));
      ret = false;
    }
  }

  if (AM_LIKELY(ret)) {
    ret = set_watchdog_timeout(sec);
  }

  return ret;
}

bool AMWatchdogService::disable_watchdog()
{
  bool ret = true;

  if (AM_LIKELY(mWdFd >= 0)) {
    if (AM_LIKELY(!mNeedReboot)) {
      int off = WDIOS_DISABLECARD;
      if (AM_UNLIKELY(ioctl(mWdFd, WDIOC_SETOPTIONS, &off) < 0)) {
        PERROR("WDIOC_SETOPTIONS");
        ERROR("Close watchdog file descriptor anyway!");
        ret = false;
      }
    }
    close(mWdFd);
    mWdFd = -1;
  }

  return ret;
}

inline bool AMWatchdogService::init_semaphore(SrvData& data)
{
  if (AM_LIKELY(!data.sem)) {
    data.sem = sem_open(data.sem_name.c_str(), O_CREAT|O_EXCL,0644, 0);
    if (AM_UNLIKELY((data.sem == SEM_FAILED) && (errno == EEXIST))) {
      data.sem = sem_open(data.sem_name.c_str(), 0);
    }
    if (AM_UNLIKELY(data.sem == SEM_FAILED)) {
      ERROR("Service[%s]: %s - %s",
            data.srv_name.c_str(), data.sem_name.c_str(), strerror(errno));
    }
  }

  return (data.sem != SEM_FAILED);
}

bool AMWatchdogService::init_service_data()
{
  bool ret= true;
  for (auto &svc_data : mServiceDataList) {
    if (svc_data.enable) {
      ret = init_semaphore(svc_data);
      if (!ret) {
        break;
      }
    }
  }

  return ret;
}

bool AMWatchdogService::clean_service_data()
{
  for (auto &svc_data : mServiceDataList) {
    if (svc_data.enable) {
      if (svc_data.sem != SEM_FAILED) {
        sem_close(svc_data.sem);
        svc_data.sem = nullptr;
      }
    }
  }

  return true;
}

bool AMWatchdogService::check_service_timeout()
{
  bool ret = true;
  for (auto &svc_data : mServiceDataList) {
    int value = 0;
    if (svc_data.enable) {
      if (sem_getvalue(svc_data.sem, &value) < 0) {
        ERROR("sem_getvalue");
        ret = (errno != EINVAL);
      } else {
        if (value >= SERVICE_TIMEOUT_SECONDS) {
          ERROR("%s is timeout! Value is %d.",
                svc_data.srv_name.c_str(), value);
          ret = false;
        } else if (sem_post(svc_data.sem) < 0) {
          switch(errno) {
            case EOVERFLOW: {
              ERROR("[%s] is overflow!", svc_data.srv_name.c_str());
            }break;
            default: {
              ERROR("sem_poist [%s]: %s",
                    svc_data.srv_name.c_str(), strerror(errno));
            }break;
          }
          ret = false;
        }
      }
    }
  }

  return ret;
}

void AMWatchdogService::abort()
{
  write(SRV_CTRL_W, "a", 1);
}

void* AMWatchdogService::watchdog_feeding_thread(void* data)
{
  struct timeval feed_interval;

   while(mIsFeeding) {
    feed_interval.tv_sec = HEART_BEAT_INTERVAL;
    feed_interval.tv_usec = 0;
    if (AM_UNLIKELY(select(0, nullptr, nullptr, nullptr, &feed_interval) < 0)) {
      ERROR("select error.\n");
      break;
    }
    if (AM_UNLIKELY(ioctl(mWdFd, WDIOC_KEEPALIVE, 0) < 0)) {
      ERROR("WDIOC_KEEPALIVE");
      break;
    }
    if (AM_UNLIKELY(!check_service_timeout())) {
      ERROR("Found check_service_timeout!\n");
      break;
    }
  }

  if (AM_LIKELY(mIsFeeding)) {
    mNeedReboot = true;
    ERROR("Waiting for Watchdog reboot.....\n");
    /* Abort run() function, make watchdog_service exit */
    abort();
  }

  return nullptr;
}

void* AMWatchdogService::static_watchdog_feeding_thread(void* data)
{
  WdData* threadData = (WdData*)data;
  return threadData->self->watchdog_feeding_thread(threadData->data);
}
