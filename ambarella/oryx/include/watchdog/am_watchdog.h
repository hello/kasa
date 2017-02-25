/*******************************************************************************
 * watchdog_service_instance.h
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
#ifndef WATCHDOG_SERVICE_INSTANCE_H_
#define WATCHDOG_SERVICE_INSTANCE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
#include "../services/commands/am_service_impl.h"
#include <list>

#define SERVICE_TIMEOUT_SECONDS 10
#define WATCHDOG_IOCTL_BASE 'W'

#define WDIOC_GETSUPPORT    _IOR(WATCHDOG_IOCTL_BASE, 0, struct watchdog_info)
#define WDIOC_GETSTATUS     _IOR(WATCHDOG_IOCTL_BASE, 1, int)
#define WDIOC_GETBOOTSTATUS _IOR(WATCHDOG_IOCTL_BASE, 2, int)
#define WDIOC_GETTEMP       _IOR(WATCHDOG_IOCTL_BASE, 3, int)
#define WDIOC_SETOPTIONS    _IOR(WATCHDOG_IOCTL_BASE, 4, int)
#define WDIOC_KEEPALIVE     _IOR(WATCHDOG_IOCTL_BASE, 5, int)
#define WDIOC_SETTIMEOUT    _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
#define WDIOC_GETTIMEOUT    _IOR(WATCHDOG_IOCTL_BASE, 7, int)
#define WDIOC_SETPRETIMEOUT _IOWR(WATCHDOG_IOCTL_BASE, 8, int)
#define WDIOC_GETPRETIMEOUT _IOR(WATCHDOG_IOCTL_BASE, 9, int)
#define WDIOC_GETTIMELEFT   _IOR(WATCHDOG_IOCTL_BASE, 10, int)

#define WDIOF_UNKNOWN   -1  /* Unknown flag error */
#define WDIOS_UNKNOWN   -1  /* Unknown status error */

#define WDIOF_OVERHEAT    0x0001  /* Reset due to CPU overheat */
#define WDIOF_FANFAULT    0x0002  /* Fan failed */
#define WDIOF_EXTERN1   0x0004  /* External relay 1 */
#define WDIOF_EXTERN2   0x0008  /* External relay 2 */
#define WDIOF_POWERUNDER  0x0010  /* Power bad/power fault */
#define WDIOF_CARDRESET   0x0020  /* Card previously reset the CPU */
#define WDIOF_POWEROVER   0x0040  /* Power over voltage */
#define WDIOF_SETTIMEOUT  0x0080  /* Set timeout (in seconds) */
#define WDIOF_MAGICCLOSE  0x0100  /* Supports magic close char */
#define WDIOF_PRETIMEOUT  0x0200  /* Pretimeout (in seconds), get/set */
#define WDIOF_KEEPALIVEPING 0x8000  /* Keep alive ping reply */

#define WDIOS_DISABLECARD 0x0001  /* Turn off the watchdog timer */
#define WDIOS_ENABLECARD  0x0002  /* Turn on the watchdog timer */
#define WDIOS_TEMPPANIC   0x0004  /* Kernel panic on temperature trip */

#define WATCHDOG_DEVICE "/dev/watchdog"

#define INIT_TIMEOUT    50
#define WORK_TIMEOUT    15

struct SrvData;
struct WdData;

typedef std::list<SrvData> AMSrvDataList;
class AMWatchdogService
{
  public:
    AMWatchdogService();
    virtual ~AMWatchdogService();

  public:
    bool init(const std::list<am_service_attribute> &m_service_list);
    bool start();
    bool stop();
    void run();
    void quit();

  private:
    bool start_feeding_thread();
    bool stop_feeding_thread();
    bool set_watchdog_timeout(int sec);
    bool enable_watchdog(int sec);
    bool disable_watchdog();
    bool init_semaphore(SrvData& data);
    bool init_service_data();
    bool clean_service_data();
    bool check_service_timeout();
    void abort();
    void* watchdog_feeding_thread(void* data);
    static void* static_watchdog_feeding_thread(void* data);

  private:
    WdData         *mWdData          = nullptr;
    pthread_t       mWdFeedingThread = 0;
    int             mSrvCtrl[2]      = {-1};
    int             mWdFd            = -1;
    int             mInitTimeout     = INIT_TIMEOUT;
    int             mWorkTimeout     = WORK_TIMEOUT;
    bool            mIsInited        = false;
    bool            mIsDspEncoding   = true;
    bool            mIsFeeding       = false;
    bool            mRun             = true;
    bool            mNeedReboot      = false;
    pthread_mutex_t mSrvLock;
    AMSrvDataList   mServiceDataList;
};

#endif /* WATCHDOG_SERVICE_INSTANCE_H_ */
