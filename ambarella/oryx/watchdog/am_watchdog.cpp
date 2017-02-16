/*******************************************************************************
 * watchdog_service_instance.cpp
 *
 * History:
 *   2014年5月14日 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_watchdog_semaphore.h"
#include "am_define.h"
#include "am_base_include.h"
#include "am_log.h"
#include "am_pid_lock.h"
#include "am_service_manager.h"
#include "am_watchdog.h"

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
#define INIT_TIMEOUT    50
#define WORK_TIMEOUT    15

#define SRV_CTRL_R      mSrvCtrl[0]
#define SRV_CTRL_W      mSrvCtrl[1]

struct SrvData
{
  const char* sem_name;
  const char* srv_name;
  sem_t*      sem;
  bool enable;
};

struct WdData
{
  AMWatchdogService* self;
  void* data;
  WdData() :
    self(NULL),
    data(NULL)
  {}
};

struct DspData
{
  const char* intr_name;
  uint32_t    intr_value;
  DspData() :
    intr_name(NULL),
    intr_value(0)
  {}
};

AMWatchdogService::AMWatchdogService() :
  mWdFeedingThread(0),
  mWdFd(-1),
  mInitTimeout(INIT_TIMEOUT),
  mWorkTimeout(WORK_TIMEOUT),
  mIsInited(false),
  mIsDspEncoding(true),
  mIsFeeding(false),
  mRun(true),
  mNeedReboot(false),
  mSrvDataList(NULL),
  mDspData(NULL),
  mWdData(NULL)
{
  SRV_CTRL_R = -1;
  SRV_CTRL_W = -1;
}

AMWatchdogService::~AMWatchdogService()
{
  stop();
  /* Need to close log file before system rebooting */
  close_log_file();
  delete[] mSrvDataList;
  delete   mWdData;

  if (AM_LIKELY(SRV_CTRL_R >= 0)) {
    close(SRV_CTRL_R);
  }
  if (AM_LIKELY(SRV_CTRL_W >= 0)) {
    close(SRV_CTRL_W);
  }
}

bool AMWatchdogService::init(am_service_attribute *m_service_list)
{
  if (AM_LIKELY(!mIsInited)) {
    do {

      pthread_mutex_init(&mDspLock, NULL);
      pthread_mutex_init(&mSrvLock, NULL);

      if (AM_UNLIKELY(!set_timestamp_enabled(true))) {
        ERROR("Failed to enable log time stamp!");
      }
      if (AM_UNLIKELY(pipe(mSrvCtrl) < 0)) {
        PERROR("pipe");
        break;
      }
      if (AM_LIKELY(!mSrvDataList)) {
        mSrvDataList = new SrvData[MAX_SERVICE_NUM - 1];
      }
      if (AM_LIKELY(mSrvDataList)) {
        memset(mSrvDataList, 0, sizeof(SrvData)*(MAX_SERVICE_NUM - 1));
        mSrvDataList[SYSTEM_SERVICE].sem_name = SEM_SYS_SERVICE;
        mSrvDataList[SYSTEM_SERVICE].srv_name = SYS_SERVICE_NAME;
        mSrvDataList[SYSTEM_SERVICE].enable = m_service_list[SYSTEM_SERVICE].enable;

        mSrvDataList[MEDIA_SERVICE].sem_name = SEM_MED_SERVICE;
        mSrvDataList[MEDIA_SERVICE].srv_name = MED_SERVICE_NAME;
        mSrvDataList[MEDIA_SERVICE].enable = m_service_list[MEDIA_SERVICE].enable;

        mSrvDataList[EVENT_SERVICE].sem_name = SEM_EVT_SERVICE;
        mSrvDataList[EVENT_SERVICE].srv_name = EVT_SERVICE_NAME;
        mSrvDataList[EVENT_SERVICE].enable = m_service_list[EVENT_SERVICE].enable;

        mSrvDataList[IMAGE_SERVICE].sem_name = SEM_IMG_SERVICE;
        mSrvDataList[IMAGE_SERVICE].srv_name = IMG_SERVICE_NAME;
        mSrvDataList[IMAGE_SERVICE].enable = m_service_list[IMAGE_SERVICE].enable;

        mSrvDataList[VIDEO_CONTROL_SERVICE].sem_name = SEM_VCTRL_SERVICE;
        mSrvDataList[VIDEO_CONTROL_SERVICE].srv_name  = VCTRL_SERVICE_NAME;
        mSrvDataList[VIDEO_CONTROL_SERVICE].enable = m_service_list[VIDEO_CONTROL_SERVICE].enable;

        mSrvDataList[NETWORK_CONTROL_SERVICE].sem_name = SEM_NET_SERVICE;
        mSrvDataList[NETWORK_CONTROL_SERVICE].srv_name = NET_SERVICE_NAME;
        mSrvDataList[NETWORK_CONTROL_SERVICE].enable = m_service_list[NETWORK_CONTROL_SERVICE].enable;

        mSrvDataList[AUDIO_CONTROL_SERVICE].sem_name = SEM_AUD_SERVICE;
        mSrvDataList[AUDIO_CONTROL_SERVICE].srv_name = AUD_SERVICE_NAME;
        mSrvDataList[AUDIO_CONTROL_SERVICE].enable = m_service_list[AUDIO_CONTROL_SERVICE].enable;

        mSrvDataList[RTSP_CONTROL_SERVICE].sem_name = SEM_RTSP_SERVICE;
        mSrvDataList[RTSP_CONTROL_SERVICE].srv_name = RTSP_SERVICE_NAME;
        mSrvDataList[RTSP_CONTROL_SERVICE].enable = m_service_list[RTSP_CONTROL_SERVICE].enable;

        mSrvDataList[SIP_SERVICE].sem_name = SEM_SIP_SERVICE;
        mSrvDataList[SIP_SERVICE].srv_name = SIP_SERVICE_NAME;
        mSrvDataList[SIP_SERVICE].enable = m_service_list[SIP_SERVICE].enable;

        mSrvDataList[USER_SERVICE].enable = m_service_list[USER_SERVICE].enable;
      } else {
        ERROR("Failed to allocate data for service data list!");
        break;
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
    if (AM_LIKELY(select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0)) {
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
                                   NULL,
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
    pthread_join(mWdFeedingThread, NULL);
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
    data.sem = sem_open(data.sem_name, O_CREAT|O_EXCL,0644, 0);
    if (AM_UNLIKELY((data.sem == SEM_FAILED) && (errno == EEXIST))) {
      data.sem = sem_open(data.sem_name, 0);
    }
    if (AM_UNLIKELY(data.sem == SEM_FAILED)) {
      PERROR(data.sem_name);
    }
  }

  return (data.sem != SEM_FAILED);
}

bool AMWatchdogService::init_service_data()
{
  bool ret= true;
  for (int i = 0; i < (MAX_SERVICE_NUM - 1); ++ i) {
    if (mSrvDataList[i].enable) {
      if (AM_UNLIKELY(false == (ret = init_semaphore(mSrvDataList[i])))) {
        break;
      }
    }
  }
  return ret;
}

bool AMWatchdogService::clean_service_data()
{
  for (int i = 0; i < (MAX_SERVICE_NUM - 1); ++ i) {
    if (mSrvDataList[i].enable) {
      if (mSrvDataList[i].sem != SEM_FAILED) {
        sem_close(mSrvDataList[i].sem);
        mSrvDataList[i].sem = NULL;
      }
    }
  }

  return true;
}

bool AMWatchdogService::check_service_timeout()
{
  bool ret = true;
  for (int i = 0; i < (MAX_SERVICE_NUM - 1); ++ i) {
    int value = 0;
    if (mSrvDataList[i].enable) {
      if (AM_UNLIKELY(sem_getvalue(mSrvDataList[i].sem, &value) < 0)) {
        ERROR("sem_getvalue");
        ret = (errno != EINVAL);
      } else {
        if (AM_UNLIKELY( (value >= SERVICE_TIMEOUT_SECONDS))) {
          ERROR("%s is timeout!value is %d.", mSrvDataList[i].srv_name,value);
          ret=false;
        } else if (AM_UNLIKELY(sem_post(mSrvDataList[i].sem) < 0)) {
          if (AM_LIKELY(errno == EOVERFLOW)) {
            ERROR("%s is overflow!", mSrvDataList[i].srv_name);
          } else {
            ERROR("sem_post %s: %s", mSrvDataList[i].srv_name, strerror(errno));
          }
          ret = false;
        }
      }
    }
  }

  return ret;
}
bool AMWatchdogService::get_dsp_intr_value(FILE* intr,
                                         const char* intr_name,
                                         uint32_t& intr_val)
{
  bool ret = false;

  if (AM_LIKELY(intr && intr_name)) {
    if (fseek(intr, 0, SEEK_SET) < 0) {
      PERROR("fseek");
    } else {
      char* begin = NULL;
      intr_val = 0;
      while (!feof(intr)) {
        char line[1024] = {0};
        if (fscanf(intr, "%[^\n]\n", line) > 0) {
          char section[128] = {0};
          begin = strstr(line, intr_name);

          if (begin && (1 == sscanf(begin, "%127s", section)) &&
              (strlen(section) == strlen(intr_name))) {
            char intrnum[128] = {0};
            if (sscanf(line, "%*[0-9]:%*[ ]%[0-9] ", intrnum) == 1) {
              intr_val += strtoul((const char*)intrnum, (char**)NULL, 10);
              ret = true;
            }
          }
        }
      }
      if (AM_UNLIKELY(!begin && (0 == intr_val))) {
        ERROR("Interrupt %s NOT FOUND!", intr_name);
      }
    }
  }

  return ret;
}

bool AMWatchdogService::check_dsp_intr(FILE* intr)
{
  bool ret = true;
  uint32_t value = 0;

  for (int i = 0; i < AMBA_DSP_INTR_NUM; ++ i) {
    pthread_mutex_lock(&mDspLock);
    if (AM_LIKELY(get_dsp_intr_value(intr, mDspData[i].intr_name, value))) {
      if (AM_LIKELY(value != mDspData[i].intr_value)) {
        mDspData[i].intr_value = value;
      } else if (((i == AMBA_VDSP) || mIsDspEncoding) &&
                 (value != 0) && (value == mDspData[i].intr_value)) {
        ERROR("%s interrupt remains the same, DSP is not working, "
              "current %u, last %u!",
              mDspData[i].intr_name, value, mDspData[i].intr_value);
        ret = false;
        break;
      }
    } else {
      ret = false;
    }
    pthread_mutex_unlock(&mDspLock);
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
    if (AM_UNLIKELY(select(0, NULL, NULL, NULL, &feed_interval) < 0)) {
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

  return NULL;
}

void* AMWatchdogService::static_watchdog_feeding_thread(void* data)
{
  WdData* threadData = (WdData*)data;
  return threadData->self->watchdog_feeding_thread(threadData->data);
}
